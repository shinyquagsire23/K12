// k1port.cpp -- KOTOR 1's libandroid_port.so, the part the engine links against (K2P_GAME=k1).
//
// The KOTOR 1 engine imports its port library, which holds Win32 shims with C++ linkage, the
// android_port_glX GL layer (k1gl.gen.cpp), the zip OBB reader, MacDecompress, the Ios*/touch
// globals SDL_main and the input code share, and the FMOD wrapper (FModAudioSystem, whose body src/
// already models: port/fmod/FModAudioSystem.cpp compiles it into the host K1 engine). This file is
// the rest.
//
// Where the shipped build has a stub, this one is the same stub, so the engine takes the paths it
// takes on the device: FindFirstFileA answers NULL, DeleteFileA 0, IosIsIphone 1 with 1136x640,
// GlobalMemoryStatus a fixed 256 MB machine. Only the OS-facing ones (time, threads, directories,
// rename) do real work, through the KOTOR II shim's C entry points (platforms/shim), which have the
// same Win32 semantics under plain-C names.
//
// Built with the engine's flags (KOTOR1, the host prelude): it includes src/port/k1obb.h, whose
// ObbFile / ObbEntry layouts the engine reads directly (GetDirectoryList walks m_ppEntries).
#include <stdint.h>
#include <inttypes.h>
#include "port/k1obb.h"
#include "k1port.h"
#include "port/win32api.h"
#include "port/ASL2/Win/PortDevMode.h"
#include "port/crtapi.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <map>
#include <string>
#include <vector>

// The KOTOR II shim's C entry points are declared under k2c_ names (k1shimc.cpp), since K1's own
// declarations of the same names have C++ linkage.
extern "C" {
void          k2c_GetSystemTimeAsFileTime(void* pFt);
void*         k2c_CreateThread(void* pSecAttr, uint64_t nStackSize, uint64_t (*pfnStart)(void*),
                               void* pParam, uint64_t nFlags, unsigned int* pId);
uint64_t      k2c_ResumeThread(void* h);
uint64_t      k2c_SuspendThread(void* h);
uint64_t      k2c_WaitForSingleObject(void* h, unsigned int nMs);
int           k2c_CloseHandle(void* h);
int           k2c_gluBuild2DMipmaps(unsigned int, int, int, int, unsigned int, unsigned int,
                                    const void*);
unsigned int  SDL_GetTicks(void);
void          SDL_Delay(unsigned int);
}

void* k2shim_gl_proc(const char* pszName);   // shim/shim_path.cpp
void* k2shim_cursor_load(uint64_t nId);  // shim/shim_cursor.cpp
void* k2shim_cursor_select(void* h);
void  k2shim_cursor_show(bool bShow);

// The port's types, forward-declared (only pointers are used here).
struct tagPOINT;
struct SDL_Window;
struct SDL_DisplayMode { unsigned int format; int w, h, refresh_rate; void* driverdata; };
class CExoSound;
class FTTextureFont;

// the port library's globals, typed as src/ declares them
ObbFile* g_mainObb;
ObbFile* g_patchObb;
ObbFile* g_fmod_mainObb;
ObbFile* g_fmod_patchObb;
volatile bool g_obbMounted;
volatile bool g_patchObbMounted;
CExoSound* g_pExoSound;
SDL_Window* g_SDL_Window;
SDL_DisplayMode g_SDL_mode;
bool g_bIsInBackground;
bool g_bRenderWindowActive;
bool g_cloudSaveChanged;
volatile bool g_GooglePlay;
int g_RenderSkip;
bool gamepadConnected;
bool s_useAlphaTestShader;
int auto_run_active;
float mouseCameraMovement;
float ios_controller_left_right;
float ios_gamepad_analog0X, ios_gamepad_analog0Y, ios_gamepad_analog1X, ios_gamepad_analog1Y;
float last_touch_x, last_touch_y, current_touch_x, current_touch_y;
float touch_x_tolerence, touch_y_tolerence, touch_x_maxRange, touch_y_maxRange;
FTTextureFont* textureFont;
// GL_OES_mapbuffer, which the port resolved at ASLgl_Init (aurglcore's VBO path)
void* (*glMapBufferOES)(unsigned int eTarget, unsigned int eAccess);
unsigned char (*glUnmapBufferOES)(unsigned int eTarget);

namespace {
int s_nDisableInteraction;    // IosTemporaryDisableInteraction's counter
int s_nCursorShown;           // ShowCursor / GetCursorInfo: 0 shown, -1 hidden
}

// ==== Win32 ======================================================================================
unsigned int GetTickCount() { return SDL_GetTicks(); }
void Sleep(int nMs) { SDL_Delay(static_cast<unsigned int>(nMs)); }
unsigned int GetLastError() { return 0; }
void GetSystemTimeAsFileTime(_FILETIME* pFt) { k2c_GetSystemTimeAsFileTime(pFt); }
int FileTimeToSystemTime(const _FILETIME*, _SYSTEMTIME*) { return 0; }
int FileTimeToLocalFileTime(const _FILETIME*, _FILETIME*) { return 0; }

// a fixed 256 MB machine, half of it free
void GlobalMemoryStatus(_MEMORYSTATUS* p)
{
    static const unsigned int k[8] = { 32, 50, 0x10000000, 0x8000000, 0x10000000, 0x8000000,
                                       0x10000000, 0x8000000 };
    if (p) memcpy(p, k, sizeof k);
}

namespace {
// K1's thread functions return int and the shim's return uint64_t, so each start goes through
// this trampoline rather than a function-pointer cast.
struct ThreadStart {
    int (*pStart)(void*);
    void* pParam;
};
uint64_t RunThreadStart(void* p)
{
    ThreadStart start = *static_cast<ThreadStart*>(p);
    delete static_cast<ThreadStart*>(p);
    return static_cast<unsigned int>(start.pStart(start.pParam));
}
} // namespace

void* CreateThread(void* pSecAttr, unsigned int nStackSize, int (*pStart)(void*), void* pParam,
                   unsigned int nFlags, unsigned int* pId)
{
    ThreadStart* p = new ThreadStart{pStart, pParam};
    void* h = k2c_CreateThread(pSecAttr, nStackSize, RunThreadStart, p, nFlags, pId);
    if (!h) delete p;
    return h;
}
unsigned int ResumeThread(void* h) { return static_cast<unsigned int>(k2c_ResumeThread(h)); }
unsigned int SuspendThread(void* h) { return static_cast<unsigned int>(k2c_SuspendThread(h)); }
int SetThreadPriority(void*, int) { return 1; }
unsigned int WaitForSingleObject(void* h, unsigned int nMs) { return static_cast<unsigned int>(k2c_WaitForSingleObject(h, nMs)); }
int CloseHandle(void* h) { return k2c_CloseHandle(h); }
void ExitThread(unsigned int) {}   // the thread function returns normally
void* GetCurrentProcess() { return 0; }
int SetPriorityClass(void*, unsigned int) { return 0; }

void* FindFirstFileA(const char*, _WIN32_FIND_DATAA*) { return 0; }
int FindNextFileA(void*, _WIN32_FIND_DATAA*) { return 0; }
int FindClose(void*) { return 0; }
int DeleteFileA(const char*) { return 0; }
unsigned int GetDriveTypeA(const char*) { return 0; }
int CreateProcessA(const char*, char*, void*, void*, int, unsigned int, void*, const char*, _STARTUPINFOA*,
                   _PROCESS_INFORMATION*) { return 0; }

namespace {
std::string PortPath(const char* pszPath)
{
    std::string s(pszPath ? pszPath : "");
    for (char& c : s)
        if (c == '\\') c = '/';
    return s;
}
}
int CreateDirectory(const char* pszPath, void*) { return mkdir(PortPath(pszPath).c_str(), 0777) == 0; }
int RemoveDirectory(const char* pszPath) { return rmdir(PortPath(pszPath).c_str()) == 0; }
extern "C" int _rename(const char* pszOld, const char* pszNew)
{
    return rename(PortPath(pszOld).c_str(), PortPath(pszNew).c_str());
}
// the engine's debug fopen wrapper (file, function, line, path, mode)
extern "C" FILE* _fopen(const char*, const char*, int, const char* pszPath, const char* pszMode)
{
    return fopen(pszPath, pszMode);
}

// 1024x768 and 800x600, 32 bpp, 60 Hz: the only two modes the port reports. Desktop builds list the
// host's window sizes instead (shim_window.cpp k2host_window_size; refresh 1, Win32's "default",
// which the Screen Resolution list shows without a rate), and the game restarts into the one picked
// (CSWGuiOptionsResolution::OnResolutionChosen, k1sdl.cpp).
#if defined(K2_QOL_FULLSCREEN)
extern "C" int k2host_window_size(unsigned int nIndex, int* pW, int* pH);
#endif
int EnumDisplaySettingsA(const char*, unsigned int nMode, _devicemodeA* p)
{
#if defined(K2_QOL_FULLSCREEN)
    int nW = 0, nH = 0;
    if (nMode == 0xffffffffu) nMode = 0;   // ENUM_CURRENT_SETTINGS
    if (!k2host_window_size(nMode, &nW, &nH)) return 0;
    unsigned int w = static_cast<unsigned int>(nW), h = static_cast<unsigned int>(nH), freq = 1;
#else
    if (nMode > 1) return 0;
    unsigned int w = nMode == 0 ? 1024 : 800, h = nMode == 0 ? 768 : 600, freq = 60;
#endif
    p->m_fields = 0x205C0000;
    p->m_bitsPerPel = 32;
    p->m_pelsWidth = w;
    p->m_pelsHeight = h;
    p->m_displayFlags = 0;
    p->m_displayFrequency = freq;
    return 1;
}
int ChangeDisplaySettingsA(_devicemodeA*, unsigned int) { return 0; }

// The window functions are KOTOR II's shim's (shim_window.cpp), which KOTOR 1 imports under C++
// names with 32-bit message parameters. Its window is SDL's, not one of the shim's, so the handles it
// passes are null and these act on no window.
int DestroyWindow(void* hWnd) { return k2shim::DestroyWindow(static_cast<HWND__*>(hWnd)); }
HDC__* GetDC(void* hWnd) { return k2shim::GetDC(static_cast<HWND__*>(hWnd)); }
void* SetCapture(void* hWnd) { return k2shim::SetCapture(static_cast<HWND__*>(hWnd)); }
int ClientToScreen(void* hWnd, tagPOINT* pPoint) { return k2shim::ClientToScreen(static_cast<HWND__*>(hWnd), pPoint); }
int PostMessageA(void* hWnd, unsigned int nMsg, unsigned int wParam, int lParam)
{
    return k2shim::PostMessageA(static_cast<HWND__*>(hWnd), nMsg, wParam, lParam);
}
void PostQuitMessage(int nExitCode) { k2shim::PostQuitMessage(nExitCode); }
int DefWindowProcA(void* hWnd, unsigned int nMsg, unsigned int wParam, int lParam)
{
    return static_cast<int>(k2shim::DefWindowProcA(static_cast<HWND__*>(hWnd), nMsg, wParam, lParam));
}
// The pc and xbox UIs' hardware cursors are the K2 shim's (shim_cursor.cpp).
void* LoadCursorA(void*, const char* pszName) { return k2shim_cursor_load(reinterpret_cast<uint64_t>(pszName)); }
int DestroyCursor(void* hCursor) { return k2shim::DestroyCursor(hCursor); }
void* SetCursor(void* h) { return k2shim_cursor_select(h); }
// This one is not the shim's, because K1's SetMousePos maps through ClientToScreen first, which has
// no window to map through here, so a warp would land at the client coordinates on the desktop.
int SetCursorPos(int, int) { return 0; }
int ShowCursor(int bShow)
{
    s_nCursorShown = bShow ? 0 : -1;
#ifndef K2_UI_TOUCH
    k2shim_cursor_show(bShow != 0);   // hidden unless the engine is in hardware-cursor mode
#endif
    return s_nCursorShown;
}
int GetCursorInfo(tagCURSORINFO* p)
{
    if (!p) return 0;
    p->hCursor = 0;
    p->ptScreenPos[0] = 0;
    p->ptScreenPos[1] = 0;
    p->flags = s_nCursorShown >= 0;
    return 1;
}

int IsDBCSLeadByte(unsigned char) { return 0; }
int FloatToIntSSE(float f) { return static_cast<int>(f); }
void _strdate(char*) {}

// ==== GL / GLU ===================================================================================
// The port's ASLgl_Init allocated its GLES state object; desktop GL and gl4es keep their own.
void ASLgl_Init()
{
    glMapBufferOES = reinterpret_cast<void* (*)(unsigned int, unsigned int)>(k2shim_gl_proc("glMapBuffer"));
    glUnmapBufferOES = reinterpret_cast<unsigned char (*)(unsigned int)>(k2shim_gl_proc("glUnmapBuffer"));
}

// For a one-frame GL call trace, `touch k1trace` in the game's working directory (Android: the
// app's files dir) and the next whole frame's port GL calls go to stderr, then the file is removed.
// `touch k1tracelive` instead traces every frame, unbuffered, into a fresh k1trace.log (the frame
// before goes to k1trace.prev.log): after a crash the log holds the crashing frame.
// `touch k1tracering` keeps the last 200 frames instead, one file each (k1trace.ring.NNN.log, NNN the
// frame number mod 200), for a glitch that lasts a frame or two.
bool k1gl_tracing;
static bool s_bTraceLive;
static bool s_bTraceRing;
static FILE* s_pTraceFile;   // k1trace.log next to the trigger (logcat drops most of a frame)
void k1gl_trace(const char* pszFormat, ...)
{
    FILE* f = s_pTraceFile ? s_pTraceFile : stderr;
    va_list ap;
    va_start(ap, pszFormat);
    fputs("k1: gl ", f);
    vfprintf(f, pszFormat, ap);
    fputc('\n', f);
    va_end(ap);
}
void k1gl_frame()
{
    static int s_nFrame;
    if (s_bTraceRing) {
        fclose(s_pTraceFile);
        char szName[64];
        snprintf(szName, sizeof szName, "k1trace.ring.%03d.log", ++s_nFrame % 200);
        s_pTraceFile = fopen(szName, "w");
        k1gl_trace("frame %d", s_nFrame);
        return;
    }
    if (s_bTraceLive) {
        fclose(s_pTraceFile);
        rename("k1trace.log", "k1trace.prev.log");
        s_pTraceFile = fopen("k1trace.log", "w");
        setvbuf(s_pTraceFile, 0, _IONBF, 0);
        k1gl_trace("frame %d", ++s_nFrame);
        return;
    }
    if (k1gl_tracing) {
        k1gl_tracing = false;
        if (s_pTraceFile) {
            fclose(s_pTraceFile);
            s_pTraceFile = 0;
        }
        fprintf(stderr, "k1: gl trace written to k1trace.log\n");
        fflush(stderr);
        return;
    }
    if (++s_nFrame % 60 == 0 && access("k1trace", F_OK) == 0) {
        unlink("k1trace");
        s_pTraceFile = fopen("k1trace.log", "w");
        k1gl_tracing = true;
        k1gl_trace("trace begin (mode %dx%d)", g_SDL_mode.w, g_SDL_mode.h);
    } else if (s_nFrame % 60 == 0 && access("k1tracering", F_OK) == 0) {
        s_pTraceFile = fopen("k1trace.ring.start.log", "w");
        if (s_pTraceFile) {
            s_bTraceRing = k1gl_tracing = true;
            fprintf(stderr, "k1: ring gl trace into k1trace.ring.NNN.log\n");
        }
    } else if (s_nFrame % 60 == 0 && access("k1tracelive", F_OK) == 0) {
        s_pTraceFile = fopen("k1trace.log", "w");
        if (s_pTraceFile) {
            setvbuf(s_pTraceFile, 0, _IONBF, 0);
            s_bTraceLive = k1gl_tracing = true;
            fprintf(stderr, "k1: live gl trace into k1trace.log\n");
        }
    }
}

// The port kept its own copy of the viewport. glViewport / glViewportRaw record it, and
// glGetIntegerv(GL_VIEWPORT) answers from the record -- starting as the whole back buffer
// (g_SDL_mode), never asking GL. AurGUIStartLayer sizes the GUI from that answer, and GL's own
// initial viewport is the surface as it was when the context was made, not the screen.
namespace {
int s_anViewport[4];
bool s_bViewportSet;
void InitViewport()
{
    if (s_bViewportSet) return;
    s_bViewportSet = true;
    s_anViewport[0] = 0;
    s_anViewport[1] = 0;
    s_anViewport[2] = g_SDL_mode.w;
    s_anViewport[3] = g_SDL_mode.h;
}
void Viewport(int x, int y, int w, int h)
{
    static void (*fn)(int, int, int, int);
    InitViewport();
    if (k1gl_tracing) k1gl_trace("glViewport(%d, %d, %d, %d)", x, y, w, h);
    // K1_VIEWPORT_LOG=1: the display mode and the first distinct viewports.
    static const bool s_bLog = getenv("K1_VIEWPORT_LOG") != 0;
    if (s_bLog) {
        static int s_nLogged, s_anLast[4] = { -1, -1, -1, -1 }, s_nModeH;
        if (s_nModeH != g_SDL_mode.h) {   // a new display mode: log its viewports too
            s_nModeH = g_SDL_mode.h;
            s_nLogged = 0;
        }
        if (s_nLogged < 60 && (x != s_anLast[0] || y != s_anLast[1] || w != s_anLast[2] || h != s_anLast[3])) {
            fprintf(stderr, "k1: viewport %d,%d %dx%d (mode %dx%d)\n", x, y, w, h, g_SDL_mode.w, g_SDL_mode.h);
            fflush(stderr);
            s_anLast[0] = x; s_anLast[1] = y; s_anLast[2] = w; s_anLast[3] = h;
            s_nLogged++;
        }
    }
    s_anViewport[0] = x;
    s_anViewport[1] = y;
    s_anViewport[2] = w;
    s_anViewport[3] = h;
    if (!fn) fn = reinterpret_cast<void (*)(int, int, int, int)>(k2shim_gl_proc("glViewport"));
    if (fn) fn(x, y, w, h);
}
}
extern "C" void android_port_glViewport(int x, int y, int w, int h) { Viewport(x, y, w, h); }
extern "C" void android_port_glViewportRaw(int x, int y, int w, int h) { Viewport(x, y, w, h); }
extern "C" void android_port_glGetIntegerv(unsigned int eName, int* pnOut)
{
    static void (*fn)(unsigned int, int*);
    if (eName == 0x0BA2) {   // GL_VIEWPORT
        InitViewport();
        memcpy(pnOut, s_anViewport, sizeof s_anViewport);
        if (k1gl_tracing) k1gl_trace("glGetIntegerv(GL_VIEWPORT) -> %d %d %d %d", pnOut[0], pnOut[1], pnOut[2], pnOut[3]);
        return;
    }
    if (!fn) fn = reinterpret_cast<void (*)(unsigned int, int*)>(k2shim_gl_proc("glGetIntegerv"));
    if (fn) fn(eName, pnOut);
}

// The ARB program entry points, glEnable / glDisable / glIsEnabled, fog, alpha, the client and
// attribute arrays and the draws are KOTOR 1's GLSL renderer layer: k1kotorgl.cpp.

// GLRender::CreateCubeTexture hands each cube face (the six GL_TEXTURE_CUBE_MAP_* targets) to
// gluBuild2DMipmaps. The Android GLU (android/glu_es.cpp) builds its chain with GL_GENERATE_MIPMAP, a
// texture parameter that is invalid on a face, so the cube would be left incomplete under its mipmap
// filter and sample black. Faces get their chain here on every host: a 2x2 box filter on the
// unsigned-byte texels, level by level down to 1x1.
int gluBuild2DMipmaps(unsigned int eTarget, int nComponents, int nWidth, int nHeight, unsigned int eFormat,
                      unsigned int eType, const void* pData)
{
    if (eTarget < 0x8515 || eTarget > 0x851a || eType != 0x1401 /* GL_UNSIGNED_BYTE */ || nComponents < 1 ||
        nComponents > 4 || nWidth < 1 || nHeight < 1)
        return k2c_gluBuild2DMipmaps(eTarget, nComponents, nWidth, nHeight, eFormat, eType, pData);

    typedef void (*TexImage2D)(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void*);
    typedef void (*PixelStorei)(unsigned int, int);
    typedef void (*GetIntegerv)(unsigned int, int*);
    static TexImage2D s_pTexImage2D = reinterpret_cast<TexImage2D>(k2shim_gl_proc("glTexImage2D"));
    static PixelStorei s_pPixelStorei = reinterpret_cast<PixelStorei>(k2shim_gl_proc("glPixelStorei"));
    static GetIntegerv s_pGetIntegerv = reinterpret_cast<GetIntegerv>(k2shim_gl_proc("glGetIntegerv"));
    static const int s_anInternal[] = { 0, 0x1909, 0x190a, 0x1907, 0x1908 };
    if (!s_pTexImage2D || !s_pPixelStorei || !s_pGetIntegerv)
        return 0;

    int nAlign = 4;
    s_pGetIntegerv(0x0cf5 /* GL_UNPACK_ALIGNMENT */, &nAlign);
    s_pPixelStorei(0x0cf5, 1);
    const int n = nComponents;
    std::vector<unsigned char> aLevel(static_cast<const unsigned char*>(pData),
                                      static_cast<const unsigned char*>(pData) + static_cast<size_t>(nWidth) * nHeight * n);
    std::vector<unsigned char> aNext;
    for (int nLevel = 0;; nLevel++) {
        s_pTexImage2D(eTarget, nLevel, s_anInternal[n], nWidth, nHeight, 0, eFormat, eType, aLevel.data());
        if (nWidth == 1 && nHeight == 1)
            break;
        int nW = nWidth > 1 ? nWidth / 2 : 1, nH = nHeight > 1 ? nHeight / 2 : 1;
        aNext.assign(static_cast<size_t>(nW) * nH * n, 0);
        for (int y = 0; y < nH; y++) {
            int y0 = nHeight > 1 ? y * 2 : 0, y1 = nHeight > 1 ? y * 2 + 1 : 0;
            for (int x = 0; x < nW; x++) {
                int x0 = nWidth > 1 ? x * 2 : 0, x1 = nWidth > 1 ? x * 2 + 1 : 0;
                for (int c = 0; c < n; c++) {
                    int nSum = aLevel[(static_cast<size_t>(y0) * nWidth + x0) * n + c] + aLevel[(static_cast<size_t>(y0) * nWidth + x1) * n + c] +
                               aLevel[(static_cast<size_t>(y1) * nWidth + x0) * n + c] + aLevel[(static_cast<size_t>(y1) * nWidth + x1) * n + c];
                    aNext[(static_cast<size_t>(y) * nW + x) * n + c] = static_cast<unsigned char>((nSum + 2) / 4);
                }
            }
        }
        aLevel.swap(aNext);
        nWidth = nW;
        nHeight = nH;
    }
    s_pPixelStorei(0x0cf5, nAlign);
    return 0;
}

// ==== movies =====================================================================================
void MacCreateBinkShaders() {}
// The file as given (backslashes to slashes), else the patch OBB's entry, else the main OBB's; the
// player is k1movie.cpp. The third argument is unused, and a movie that played leaves g_RenderSkip
// at 1.
void MacPlayBinkGL(const char* pszMovie, bool bSound, bool&, int nVolume)
{
    char szPath[1024];
    snprintf(szPath, sizeof szPath, "%s", pszMovie);
    for (char* p = szPath; *p; ++p)
        if (*p == '\\') *p = '/';

    SDL_RWops* rw = SDL_RWFromFile(szPath, "rb");
    if (!rw && g_patchObbMounted) rw = g_patchObb->RWFromFile(szPath);
    if (!rw && g_obbMounted) rw = g_mainObb->RWFromFile(szPath);
    if (!rw) {
        fprintf(stderr, "k1: movie %s: not found\n", szPath);
        return;
    }
    if (k1movie_play(rw, szPath, bSound, nVolume)) g_RenderSkip = 1;
    SDL_RWclose(rw);
}

// ==== resources ==================================================================================
// One .bzf resource: 5 LZMA property bytes, then the stream.
extern "C" int MacDecompress(void* pDst, uint64_t nDstSize, void* pSrc, uint64_t nSrcSize)
{
    size_t nDst = nDstSize, nSrc = nSrcSize - 5;
    const unsigned char* src = static_cast<const unsigned char*>(pSrc);
    int r = k1lzma_uncompress(static_cast<unsigned char*>(pDst), &nDst, src + 5, &nSrc, src, 5);
    if (r != 0) fprintf(stderr, "k1: MacDecompress: LZMA error %d (%" PRIu64 " -> %" PRIu64 ")\n", r, nSrcSize, nDstSize);
    return r == 0;
}

// The OBB reader. ObbFile (k1obb.h) holds the archive header and then one ObbEntry per zip index,
// whose directory entries list their children (GetDirectoryList walks those).
//
// A directory in place of the .obb (k1port_mount_obb, when there are no expansion files: a PC data set
// such as the Steam install) is served file by file: an archive with no entries, so the engine's own
// zip lookups miss, and RWFromFile opens the loose file under that directory instead. The streamed
// sounds read only through the main OBB, so this is what lets them play off loose files.
namespace {
std::map<const ObbFile*, std::string> s_looseRoots;   // directory-backed ObbFile -> its directory
}
char* k2shim_host_path(const char* pszIn, char* pszOut, size_t nOut);   // shim: case-blind lookup

ObbFile::ObbFile(const char* pPath, bool)
{
    memset(this, 0, sizeof *this);
    struct stat st;
    if (stat(pPath, &st) == 0 && S_ISDIR(st.st_mode)) {
        k1zip_open_empty(m_zip);
        s_looseRoots[this] = pPath;
        fprintf(stderr, "k1: OBB layer: the files in %s\n", pPath);
        return;
    }
    if (!k1zip_open(m_zip, pPath)) {
        fprintf(stderr, "k1: OBB %s: cannot open\n", pPath);
        return;
    }
    unsigned int n = k1zip_count(m_zip);
    m_ppEntries = new ObbEntry*[n];
    std::vector<std::string> names(n);
    for (unsigned int i = 0; i < n; ++i) {
        ObbEntry* e = new ObbEntry();
        char name[1024];
        int bDir = 0;
        unsigned long long t = 0;
        k1zip_entry(m_zip, i, name, sizeof name, &bDir, &t);
        names[i] = name;
        std::string s(name);
        if (!s.empty() && s.back() == '/') s.pop_back();
        size_t slash = s.rfind('/');
        e->m_sName = slash == std::string::npos ? s : s.substr(slash + 1);
        e->m_bDirectory = bDir;
        e->m_nTime = t;
        m_ppEntries[i] = e;
    }
    // each entry into its parent directory's list (the directory entries are explicit: "data/")
    for (unsigned int i = 0; i < n; ++i) {
        std::string s = names[i];
        if (!s.empty() && s.back() == '/') s.pop_back();
        size_t slash = s.rfind('/');
        if (slash == std::string::npos) continue;
        int p = k1zip_locate(m_zip, (s.substr(0, slash + 1)).c_str());
        if (p >= 0) m_ppEntries[p]->m_aChildren.push_back(m_ppEntries[i]);
    }
    fprintf(stderr, "k1: OBB %s: %u entries\n", pPath, n);
}

ObbFile::~ObbFile()
{
    s_looseRoots.erase(this);
    if (m_ppEntries) {
        unsigned int n = k1zip_count(m_zip);
        for (unsigned int i = 0; i < n; ++i) delete m_ppEntries[i];
        delete[] m_ppEntries;
        m_ppEntries = 0;
    }
    k1zip_close(m_zip);
}

// The engine's paths are relative ("./data/2da.bzf", "\\CHITIN.key") and joined loosely
// ("override//confirm.gui"): leading dots and slashes of either kind go, and runs of slashes collapse
// to one. K1_OBB_TRACE=1 logs every lookup.
SDL_RWops* ObbFile::RWFromFile(const char* pName)
{
    static const bool s_bTrace = getenv("K1_OBB_TRACE") != 0;
    while (*pName == '.' || *pName == '/' || *pName == '\\') pName++;
    std::string sName;
    for (const char* p = pName; *p; ++p) {
        char c = *p == '\\' ? '/' : *p;
        if (c == '/' && !sName.empty() && sName.back() == '/') continue;
        sName += c;
    }
    std::map<const ObbFile*, std::string>::const_iterator loose = s_looseRoots.find(this);
    if (loose != s_looseRoots.end()) {
        char szHost[1024];
        k2shim_host_path((loose->second + "/" + sName).c_str(), szHost, sizeof szHost);
        SDL_RWops* rw = SDL_RWFromFile(szHost, "rb");
        if (s_bTrace) fprintf(stderr, "k1: OBB %p %s %s\n", static_cast<void*>(this), rw ? "file" : "miss", szHost);
        return rw;
    }
    int n = k1zip_locate(m_zip, sName.c_str());
    if (s_bTrace) fprintf(stderr, "k1: OBB %p %s %s\n", static_cast<void*>(this), n < 0 ? "miss" : "hit ", sName.c_str());
    if (n < 0) return 0;
    return k1zip_rwops(m_zip, sName.c_str(), n);
}

extern "C" void k1port_mount_obb(const char* pszPath, int bPatch)
{
    if (bPatch) {
        g_patchObb = new ObbFile(pszPath, false);
        g_fmod_patchObb = new ObbFile(pszPath, true);
        g_patchObbMounted = true;
    } else {
        g_mainObb = new ObbFile(pszPath, false);
        g_fmod_mainObb = new ObbFile(pszPath, true);
        g_obbMounted = true;
    }
}

extern "C" int k1port_data_has(const char* pszName)
{
    ObbFile* apObbs[2] = { g_patchObbMounted ? g_patchObb : 0, g_obbMounted ? g_mainObb : 0 };
    for (ObbFile* pObb : apObbs)
        if (SDL_RWops* rw = pObb ? pObb->RWFromFile(pszName) : 0) {
            SDL_RWclose(rw);
            return 1;
        }
    return 0;
}

// ==== the OS glue ==================================================================================
bool IosIsIphone(int& nWidth, int& nHeight)
{
    nWidth = 1136;
    nHeight = 640;
    return true;
}
void IosTemporaryDisableInteraction(int n) { s_nDisableInteraction = n; }
int IosGetTemporaryDisableInteraction() { return s_nDisableInteraction; }
void IosUpdateDisableInteractionCount()
{
    if (s_nDisableInteraction - 1 >= 0) s_nDisableInteraction--;
}
bool IosIcloudAvailable() { return false; }
void IosWriteCloud(const char*, const char*, unsigned int) {}
void IosSetNotificiation(const char* const*) {}

// The walk stick is the drag since the finger went down (SDL_main keeps last_touch_* /
// current_touch_*), past a dead zone and eased over the full range into 0.2..1.0, or, with no drag,
// the left analog stick. Auto-run holds the last forward drag. The scale arguments are unused.
namespace {
float ios_last_delta_up_down;       // the forward drag auto-run keeps
float ios_up_down, ios_left_right;
}
float GetIosControl(bool bUpDown, float, float)
{
    float fDelta, fTolerance, fRange;
    if (bUpDown) {
        if (auto_run_active) {
            fDelta = ios_last_delta_up_down;
        } else {
            fDelta = current_touch_y - last_touch_y;
            ios_last_delta_up_down = fDelta;
        }
        fTolerance = touch_y_tolerence;
        fRange = touch_y_maxRange;
    } else {
        fDelta = current_touch_x - last_touch_x;
        fTolerance = touch_x_tolerence;
        fRange = touch_x_maxRange;
    }
    if (fDelta > fTolerance) {
        float f = fminf((fDelta - fTolerance) / fRange, 1.0f);
        return static_cast<float>(0.5 - cos(static_cast<double>(f) * 3.14159265) * 0.5) * 0.8f + 0.2f;
    }
    if (fDelta < -fTolerance) {
        float f = fminf((-fDelta - fTolerance) / fRange, 1.0f);
        return -(static_cast<float>(0.5 - cos(static_cast<double>(f) * 3.14159265) * 0.5) * 0.8f + 0.2f);
    }
    return bUpDown ? ios_gamepad_analog0Y : ios_gamepad_analog0X;
}
void ClearIosControl(bool bUpDown) { (bUpDown ? ios_up_down : ios_left_right) = 0.0f; }

// A pad SDL_main opened is a "game controller" (2): ClientInput's camera then turns on the right
// stick (ios_gamepad_analog1X). Overrides the KOTOR II shim's weak pair.
bool ASLPlat_IsControllerPlugged() { return gamepadConnected; }
int ASLPlat_GetControllerPlugged() { return gamepadConnected << 1; }
