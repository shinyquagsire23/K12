// k1sdl.cpp -- what Aspyr's SDL fork did for KOTOR 1 that stock SDL2 does not (K2P_GAME=k1).
//
// K1's SDL_main (src/port/Mac/MacMain.cpp) owns the window: it asks for a GLES 2 context with a 565
// back buffer, a fullscreen window the size of display mode 0, and reads the window back through
// Aspyr's exported `Android_Window`. These definitions sit in front of SDL's (the engine binds to
// them: the executable / -Bsymbolic libmain.so defines them) and forward to the real ones:
//
//   * macOS: a desktop GL 2.1 compatibility context (the engine's GL 1.x/ARB calls run natively)
//     with an 8-bit back buffer and a 24-bit depth buffer, in a window. "Display mode 0" is the
//     window size, K1_WINDOW=<w>x<h> (default 1280x720); K1_FULLSCREEN=1 keeps the fullscreen flag.
//     Graphics' Fullscreen row (swkotor.ini [Display Options] FullScreen=1) opens the next launch's
//     window fullscreen instead, at the size a fullscreen window gets (shim_display.cpp).
//   * Android and iOS: the GLES 2 context as asked; gl4es is told when it exists and before every
//     swap, as android/gl4es_glue.cpp is for KOTOR II's ASL window. iOS: display mode 0 is the
//     screen in pixels and the window gets a full-resolution drawable (SDL's sizes are points).
//   * iOS: SDL_RWFromFile goes through the shim's case-insensitive fopen (below; the desktop builds'
//     is shim/shim_stdio.cpp's).
//   * Android: SDL_RWFromFile on a relative path tries the working directory (the app's external
//     files dir, where saves/ and swkotor.ini live and where GetDirectoryList lists) before stock
//     SDL's internal-storage and APK-asset lookups, because Aspyr's SDL resolved them against
//     external storage, so a save written here is the one the Load screen finds.
//   * both: Android_Window is the window SDL_main created.
//   * both: Advanced Graphics' framerate lock, g_bFrameRateLocked. SDL_main reads it (MacMain.cpp):
//     on, 60 fps, the default; off, no cap.
#include <stdint.h>
#include <SDL.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "../shim/k2shim.h"
#include "port/k2qolapi.h"

SDL_Window* Android_Window;   // C++ names, as src/ declares them (the MSVC ABI mangles variables)
bool g_bFrameRateLocked = true;   // swguioptions.cpp's Advanced Graphics row; clientoptions.cpp saves it
extern "C" {
#ifndef __ANDROID__
// K1_CHROMEBOOK=1: SDL_main's Chromebook mode, which reads the mouse (macos/main.cpp). The pc UI
// is K1's own mouse + keyboard mode, so there it is on unless K1_CHROMEBOOK=0.
int SDL_IsChromebook(void)
{
    static int s = -1;
    if (s < 0) {
        const char* p = getenv("K1_CHROMEBOOK");
#ifdef K2_UI_PC
        s = p ? atoi(p) != 0 : 1;
#else
        s = p != 0;
#endif
    }
    return s;
}
#endif
}

namespace {
template <class Fn>
Fn Real(const char* pszName)
{
    Fn fn = reinterpret_cast<Fn>(dlsym(RTLD_NEXT, pszName));
    if (!fn) {
        fprintf(stderr, "k1: %s: no SDL definition behind ours\n", pszName);
        abort();
    }
    return fn;
}

#ifndef K2P_MOBILE_HOST
// swkotor.ini's [Display Options] <key>, as the engine wrote it (CExoIni: `key=value` lines under
// `[section]`, CRLF or LF); nDefault when it is not there.
int IniDisplayOption(const char* pszKey, int nDefault)
{
    FILE* f = fopen("swkotor.ini", "r");
    if (!f) return nDefault;
    char szLine[256];
    size_t nKey = strlen(pszKey);
    bool bSection = false;
    int nValue = nDefault;
    while (fgets(szLine, sizeof szLine, f)) {
        char* pEnd = szLine + strlen(szLine);
        while (pEnd > szLine && (pEnd[-1] == '\n' || pEnd[-1] == '\r' || pEnd[-1] == ' ')) *--pEnd = 0;
        if (szLine[0] == '[') {
            bSection = strcasecmp(szLine, "[Display Options]") == 0;
        } else if (bSection && strncasecmp(szLine, pszKey, nKey) == 0 && szLine[nKey] == '=') {
            nValue = atoi(szLine + nKey + 1);
        }
    }
    fclose(f);
    return nValue;
}

// Reads Graphics' Fullscreen row, [Display Options] FullScreen. K1_FULLSCREEN=1 forces it on.
bool IniFullscreen()
{
    const char* p = getenv("K1_FULLSCREEN");
    if (p && *p == '1') return true;
    return IniDisplayOption("FullScreen", 0) != 0;
}

int s_nFullscreen = -1;   // the setting, read once at launch, then what the row last wrote

bool HostFullscreen()
{
    if (s_nFullscreen < 0) s_nFullscreen = IniFullscreen();
    return s_nFullscreen != 0;
}

void HostWindowSize(int* pW, int* pH)
{
    if (HostFullscreen()) {
        k2shim_fullscreen_points(pW, pH);
        return;
    }
    // Screen Resolution's window size ([Display Options] WindowWidth / WindowHeight, in points,
    // swguioptions.cpp), unless K1_WINDOW says otherwise
    *pW = IniDisplayOption("WindowWidth", 1280);
    *pH = IniDisplayOption("WindowHeight", 720);
    if (*pW < 640 || *pH < 480) {
        *pW = 1280;
        *pH = 720;
    }
    if (const char* p = getenv("K1_WINDOW")) sscanf(p, "%dx%d", pW, pH);
}

// True when the window covers the display, i.e. under the Fullscreen setting or with a window size
// that is the fullscreen size (Screen Resolution lists the desktop's own size last, shim_window.cpp
// k2host_window_size).
bool HostFillsDisplay()
{
    if (HostFullscreen()) return true;
    int nW, nH, nFullW, nFullH;
    HostWindowSize(&nW, &nH);
    k2shim_fullscreen_points(&nFullW, &nFullH);
    return nW == nFullW && nH == nFullH;
}

// Returns the display's pixels per window point (2 on a Retina Mac). K1_WINDOW is the window in
// points and the game renders the window's pixels, so the GUI, its text and the cursor are drawn at
// the panel's resolution instead of being drawn at half of it and stretched by the window server.
// K1_HIDPI=0 keeps a point-sized back buffer.
int HostPixelScale()
{
    static int s_nScale = 0;
    if (s_nScale == 0) {
        s_nScale = 1;
        const char* p = getenv("K1_HIDPI");
        if (!(p && *p == '0')) {
            static auto real = Real<SDL_Window* (*)(const char*, int, int, int, int, Uint32)>("SDL_CreateWindow");
            if (SDL_Window* w = real("", 0, 0, 64, 64, SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI)) {
                int nW = 0, nH = 0;
                SDL_GL_GetDrawableSize(w, &nW, &nH);
                if (nW >= 128) s_nScale = nW / 64;
                SDL_DestroyWindow(w);
            }
        }
    }
    return s_nScale;
}
#endif
} // namespace

#if defined(K2_QOL_FULLSCREEN) && !defined(K2P_MOBILE_HOST)
// Returns CSWGuiOptionsGraphics' Fullscreen setting, which the next launch opens with.
extern "C" int k2host_fullscreen(void) { return HostFullscreen(); }
extern "C" void k2host_set_fullscreen_setting(int bOn) { s_nFullscreen = bOn != 0; }

// Fullscreen and Screen Resolution from the main menu (swguioptions.cpp K1QolApplyDisplay): the
// window for the current settings, and g_SDL_mode -- the engine's screen, which SetVideoMode then
// re-initializes Aurora and the GUI at -- set to its pixels.
extern SDL_DisplayMode g_SDL_mode;          // src/port/Mac/MacMain.cpp
extern int g_SDL_BufferGeometry_w, g_SDL_BufferGeometry_h;
extern "C" void android_port_glViewport(int x, int y, int w, int h);   // k1port.cpp
extern "C" void k2host_k1_apply_display(void)
{
    if (!Android_Window) return;
    int nW, nH;
    HostWindowSize(&nW, &nH);
    if (HostFillsDisplay()) {
        SDL_SetWindowSize(Android_Window, nW, nH);
        SDL_SetWindowFullscreen(Android_Window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(Android_Window, 0);
        SDL_SetWindowSize(Android_Window, nW, nH);
        SDL_SetWindowPosition(Android_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
    g_SDL_mode.w = g_SDL_BufferGeometry_w = nW * HostPixelScale();
    g_SDL_mode.h = g_SDL_BufferGeometry_h = nH * HostPixelScale();
    // The GUI takes the whole screen from the current viewport (AurGUIStartLayer) and puts it back
    // when it closes a layer, so the old one would carry on from frame to frame (k1port.cpp seeds it
    // only once): start the new screen with the whole of it.
    android_port_glViewport(0, 0, g_SDL_mode.w, g_SDL_mode.h);
    fprintf(stderr, "k1: display %s %dx%d (%dx%d pixels)\n", HostFillsDisplay() ? "fullscreen" : "window",
            nW, nH, g_SDL_mode.w, g_SDL_mode.h);
}

// Advanced Graphics' V-Sync (aurora.cpp AurSetVSyncState, an empty stub in the APK): the swap
// interval. SDL_main sets interval 0 at startup (the port's choice); the setting overrides it once
// the options have been read.
static int s_nVSync = -1;
extern "C" void k2host_set_vsync(int bEnabled)
{
    s_nVSync = bEnabled != 0;
    static auto real = Real<int (*)(int)>("SDL_GL_SetSwapInterval");
    if (SDL_GL_GetCurrentContext()) real(s_nVSync);
}

extern "C" int SDL_GL_SetSwapInterval(int nInterval)
{
    static auto real = Real<int (*)(int)>("SDL_GL_SetSwapInterval");
    return real(s_nVSync >= 0 ? s_nVSync : nInterval);
}
#endif

extern "C" int SDL_GL_SetAttribute(SDL_GLattr attr, int value)
{
    static auto real = Real<int (*)(SDL_GLattr, int)>("SDL_GL_SetAttribute");
#ifndef K2P_MOBILE_HOST
    switch (attr) {
    case SDL_GL_CONTEXT_PROFILE_MASK:
        value = SDL_GL_CONTEXT_PROFILE_COMPATIBILITY;
        break;
    case SDL_GL_CONTEXT_MAJOR_VERSION:
        real(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        value = 2;
        break;
    case SDL_GL_RED_SIZE:
    case SDL_GL_GREEN_SIZE:
    case SDL_GL_BLUE_SIZE:
        value = 8;
        real(SDL_GL_DEPTH_SIZE, 24);
        break;
    default:
        break;
    }
#endif
    return real(attr, value);
}

extern "C" int SDL_GetDisplayMode(int nDisplay, int nMode, SDL_DisplayMode* pMode)
{
    static auto real = Real<int (*)(int, int, SDL_DisplayMode*)>("SDL_GetDisplayMode");
    int r = real(nDisplay, nMode, pMode);
#if defined(K2P_IOS)
    // iOS: SDL's modes are in points; the game renders the screen's pixels (SDL_CreateWindow below)
    if (r == 0 && nDisplay == 0 && nMode == 0) {
        if (pMode->h > pMode->w) std::swap(pMode->w, pMode->h);   // landscape-only app (shim_window.cpp)
        pMode->w = static_cast<int>(pMode->w * k2shim_pixel_scalef() + 0.5f);
        pMode->h = static_cast<int>(pMode->h * k2shim_pixel_scalef() + 0.5f);
    }
#elif !defined(K2P_MOBILE_HOST)
    if (r == 0 && nDisplay == 0 && nMode == 0) {
        HostWindowSize(&pMode->w, &pMode->h);
        pMode->w *= HostPixelScale();
        pMode->h *= HostPixelScale();
    }
#endif
    return r;
}

#ifdef K2_UI_PC
// ---- the pc UI's keyboard -------------------------------------------------------------------------
// K1's port reads no keyboard device: its GetKeyboardBuffer only ever reports the on-screen Esc. The
// pc UI gives it the real keyboard -- every press and release as its DirectInput code, which the
// engine's own key map (keymap.2da, CExoInputInternal::GetEvents) turns into events, as on the PC.
// An event watch sees what SDL_main pumps (and what the scripted input pushes); K1's
// CExoRawInputInternal::GetKeyboardBuffer drains it once per engine poll.
extern "C" unsigned char k2shim_dik_from_scancode(int nScancode);   // shim/shim_dinput.cpp
namespace {
std::mutex s_keyLock;
std::vector<std::pair<unsigned char, unsigned char>> s_keyQueue;   // (DIK code, 0x80 down / 0 up)

int KeyWatch(void*, SDL_Event* e)
{
    if ((e->type == SDL_KEYDOWN && !e->key.repeat) || e->type == SDL_KEYUP) {
        if (unsigned char nDik = k2shim_dik_from_scancode(e->key.keysym.scancode)) {
            std::lock_guard<std::mutex> g(s_keyLock);
            if (s_keyQueue.size() < 256)
                s_keyQueue.push_back({nDik, static_cast<unsigned char>(e->type == SDL_KEYDOWN ? 0x80 : 0)});
        }
    }
    return 1;
}
} // namespace

extern "C" int k1pc_key_drain(unsigned char* pDik, unsigned char* pData, int nMax)
{
    std::lock_guard<std::mutex> g(s_keyLock);
    int n = 0;
    for (; n < nMax && n < static_cast<int>(s_keyQueue.size()); n++) {
        pDik[n] = s_keyQueue[n].first;
        pData[n] = s_keyQueue[n].second;
    }
    s_keyQueue.erase(s_keyQueue.begin(), s_keyQueue.begin() + n);
    return n;
}
#endif

extern "C" SDL_Window* SDL_CreateWindow(const char* pszTitle, int x, int y, int w, int h, Uint32 flags)
{
#ifdef K2_UI_PC
    SDL_AddEventWatch(KeyWatch, 0);
#endif
    static auto real = Real<SDL_Window* (*)(const char*, int, int, int, int, Uint32)>("SDL_CreateWindow");
#if defined(K2P_IOS)
    // iOS: the mode was the screen in pixels (SDL_GetDisplayMode above); the window is the screen in
    // points with a full-resolution drawable.
    w = static_cast<int>(w / k2shim_pixel_scalef() + 0.5f);
    h = static_cast<int>(h / k2shim_pixel_scalef() + 0.5f);
    flags |= SDL_WINDOW_ALLOW_HIGHDPI;
#elif !defined(K2P_MOBILE_HOST)
    // A window, or a desktop-sized fullscreen one (never a display mode change), made as a window
    // of the fullscreen size and switched below: created fullscreen, it takes the whole display,
    // the strip a notch covers included, which the game's mode (HostWindowSize) leaves out.
    flags &= ~static_cast<Uint32>(SDL_WINDOW_FULLSCREEN_DESKTOP);
    x = y = SDL_WINDOWPOS_CENTERED;
    if (getenv("K2_HIDDEN")) flags |= SDL_WINDOW_HIDDEN;
    int nScale = HostPixelScale();
    if (nScale > 1) {
        w /= nScale;
        h /= nScale;
        flags |= SDL_WINDOW_ALLOW_HIGHDPI;
    }
#endif
#ifndef K2P_MOBILE_HOST
    // SDL_main calls its window "KOTOR"; KOTOR II's carries the game's name (g_sGameName, which is
    // this string in K1's exobaseinternal.cpp).
    if (pszTitle && strcmp(pszTitle, "KOTOR") == 0) pszTitle = "Star Wars: Knights of the Old Republic";
#endif
    Android_Window = real(pszTitle, x, y, w, h, flags);
#ifndef K2P_MOBILE_HOST
    // Desktop SDL2 starts text input at SDL_Init, which on macOS raises the IME bubbles at the
    // window's corner while playing; K1's name and save pages start it themselves (SDL_IsChromebook).
    SDL_StopTextInput();
    k2shim_keep_composited(Android_Window);
    k2shim_window_icon(Android_Window);
    if (Android_Window && HostFillsDisplay())
        SDL_SetWindowFullscreen(Android_Window, SDL_WINDOW_FULLSCREEN_DESKTOP);
#endif
    return Android_Window;
}

#ifdef K2P_MOBILE_HOST
extern "C" SDL_GLContext SDL_GL_CreateContext(SDL_Window* pWindow)
{
    static auto real = Real<SDL_GLContext (*)(SDL_Window*)>("SDL_GL_CreateContext");
    SDL_GLContext ctx = real(pWindow);
    if (ctx) k2android_gl_context_created();
    return ctx;
}
#endif

#if defined(__ANDROID__) || defined(K2P_IOS)
// iOS: every path, through the shim's fopen (shim_stdio.cpp), whose lookup ignores case as macOS's
// file system does (one that misses still reaches SDL's, which looks in the app bundle). K1's
// CExoFile reads through SDL_RWFromFile, so without it a party NPC's template, written as
// gameinprogress/AVAILNPC0.utc, is asked for as availnpc0.utc and not found: Party Selection's
// "I am broken / So very very broken" slot.
extern "C" SDL_RWops* SDL_RWFromFile(const char* pszFile, const char* pszMode)
{
    static auto real = Real<SDL_RWops* (*)(const char*, const char*)>("SDL_RWFromFile");
#ifdef __ANDROID__
    if (pszFile && *pszFile != '/') {
#else
    if (pszFile && pszMode) {
#endif
        if (FILE* fp = fopen(pszFile, pszMode)) return SDL_RWFromFP(fp, SDL_TRUE);
    }
    return real(pszFile, pszMode);
}
#endif

namespace {
void WriteShot(SDL_Window* pWindow, const char* pszName);

// K2_SHOT=<frame>[,<frame>...]: the back buffer of those frames (counted in swaps) to
// k2shot_<frame>.ppm in the working directory, just before the swap -- as the KOTOR II shim does.
void MaybeScreenshot(SDL_Window* pWindow)
{
    static const char* s_pszShots = getenv("K2_SHOT");
    static int s_nFrame;
    int nFrame = ++s_nFrame;
    if (!s_pszShots) return;
    bool bWanted = false;
    for (const char* p = s_pszShots; *p;) {
        char* end;
        long n = strtol(p, &end, 10);
        if (end == p) break;
        bWanted |= n == nFrame;
        p = *end ? end + 1 : end;
    }
    if (!bWanted) return;
    char szName[64];
    snprintf(szName, sizeof szName, "k2shot_%d.ppm", nFrame);
    WriteShot(pWindow, szName);
}

void WriteShot(SDL_Window* pWindow, const char* szName)
{
    typedef void (*ReadPixels)(int, int, int, int, unsigned int, unsigned int, void*);
    typedef void (*PixelStorei)(unsigned int, int);
    ReadPixels readPixels = reinterpret_cast<ReadPixels>(k2shim_gl_proc("glReadPixels"));
    PixelStorei pixelStorei = reinterpret_cast<PixelStorei>(k2shim_gl_proc("glPixelStorei"));
    int nW, nH;
    SDL_GL_GetDrawableSize(pWindow, &nW, &nH);
    unsigned char* pixels = static_cast<unsigned char*>(malloc(static_cast<size_t>(nW) * nH * 4));
    if (!pixels || !readPixels) return;
    if (pixelStorei) pixelStorei(0x0d05, 1);   // GL_PACK_ALIGNMENT
    readPixels(0, 0, nW, nH, 0x1908, 0x1401, pixels);   // GL_RGBA, GL_UNSIGNED_BYTE (GLES has no RGB read)
    if (FILE* f = fopen(szName, "wb")) {
        fprintf(f, "P6\n%d %d\n255\n", nW, nH);
        for (int y = nH - 1; y >= 0; y--)   // GL rows run bottom-up
            for (int x = 0; x < nW; x++) fwrite(&pixels[(static_cast<size_t>(y) * nW + x) * 4], 1, 3, f);
        fclose(f);
        fprintf(stderr, "k1: wrote %s\n", szName);
    }
    free(pixels);
}
} // namespace

char* FuncInterp(char* pszExpr);      // aurconsole.cpp: the console's interpreter
extern float ios_gamepad_analog1X;   // k1port.cpp: the right stick, as SDL_main stores it
extern bool gamepadConnected;

namespace {
// Scripted input for unattended runs, the K1 counterpart of the KOTOR II shim's (frames counted in
// swaps, coordinates in window pixels). K1's SDL_main reads the touch screen, so a tap is a finger:
//   K2_INPUT=<frame>:<action>:<arg>[;...]   actions: tap:X,Y  drag:X0,Y0,X1,Y1  key:scancode  text:STR (`_` = space)
//                                            click:X,Y  move:X,Y (the mouse; Chromebook mode / pc UI)
//                                            keydown:scancode / keyup:scancode (held until released)
//                                            joy:N (raw joystick button N, as SDL_main numbers them)
//                                            shot:NAME (k2shot_NAME.ppm)  quit:0
//                                            con:CMD (a console command, `+` = space: con:warp+end_m01ab)
//                                            achieve:TAG (unlock it, as the engine does: shim_achievements.cpp)
//   K2_CTL=<file>   the same actions, live: the file is read and removed each frame; one action per
//                   line, `+N:action:arg` delays it N frames.
std::vector<std::pair<int, std::string>> s_queue;   // (frame, "action:arg")

void PushFinger(SDL_Window* pWindow, Uint32 type, float x, float y)
{
    SDL_Event e;
    memset(&e, 0, sizeof e);
    e.type = type;
    e.tfinger.timestamp = SDL_GetTicks();
    e.tfinger.touchId = 1;
    e.tfinger.fingerId = 1;
    e.tfinger.x = x;
    e.tfinger.y = y;
    e.tfinger.pressure = 1.0f;
    e.tfinger.windowID = SDL_GetWindowID(pWindow);
    SDL_PushEvent(&e);
}

void WriteShot(SDL_Window* pWindow, const char* pszName);

void DoAction(SDL_Window* pWindow, int nFrame, const std::string& sAction, const std::string& sArg)
{
    int w, h;
    SDL_GetWindowSize(pWindow, &w, &h);
    fprintf(stderr, "k1: input frame %d: %s %s\n", nFrame, sAction.c_str(), sArg.c_str());
    if (sAction == "tap") {
        int x = 0, y = 0;
        sscanf(sArg.c_str(), "%d,%d", &x, &y);
        PushFinger(pWindow, SDL_FINGERDOWN, static_cast<float>(x) / w, static_cast<float>(y) / h);
        s_queue.push_back({nFrame + 3, "up:" + sArg});
    } else if (sAction == "up" || sAction == "fmove") {
        int x = 0, y = 0;
        sscanf(sArg.c_str(), "%d,%d", &x, &y);
        PushFinger(pWindow, sAction == "up" ? SDL_FINGERUP : SDL_FINGERMOTION, static_cast<float>(x) / w, static_cast<float>(y) / h);
    } else if (sAction == "drag") {
        // a finger that moves while down: down at X0,Y0, eight motion steps to X1,Y1, then up
        int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        sscanf(sArg.c_str(), "%d,%d,%d,%d", &x0, &y0, &x1, &y1);
        PushFinger(pWindow, SDL_FINGERDOWN, static_cast<float>(x0) / w, static_cast<float>(y0) / h);
        for (int i = 1; i <= 8; i++)
            s_queue.push_back({nFrame + i, "fmove:" + std::to_string(x0 + (x1 - x0) * i / 8) + "," +
                                               std::to_string(y0 + (y1 - y0) * i / 8)});
        s_queue.push_back({nFrame + 10, "up:" + std::to_string(x1) + "," + std::to_string(y1)});
    } else if (sAction == "move" || sAction == "click" || sAction == "mouseup") {
        // the mouse (SDL_main reads it in Chromebook mode -- the pc UI): a click is a move, a
        // press and, three frames later, the release
        int x = 0, y = 0;
        sscanf(sArg.c_str(), "%d,%d", &x, &y);
        SDL_Event e;
        memset(&e, 0, sizeof e);
        if (sAction == "move") {
            e.type = SDL_MOUSEMOTION;
            e.motion.windowID = SDL_GetWindowID(pWindow);
            e.motion.x = x;
            e.motion.y = y;
            SDL_PushEvent(&e);
            return;
        }
        e.type = sAction == "click" ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
        e.button.windowID = SDL_GetWindowID(pWindow);
        e.button.button = SDL_BUTTON_LEFT;
        e.button.state = sAction == "click" ? SDL_PRESSED : SDL_RELEASED;
        e.button.clicks = 1;
        e.button.x = x;
        e.button.y = y;
        SDL_PushEvent(&e);
        if (sAction == "click") s_queue.push_back({nFrame + 3, "mouseup:" + sArg});
    } else if (sAction == "rdown" || sAction == "rup" || sAction == "ldown" || sAction == "lup") {
        int x = 0, y = 0;
        sscanf(sArg.c_str(), "%d,%d", &x, &y);
        SDL_Event e;
        memset(&e, 0, sizeof e);
        bool bDown = sAction == "rdown" || sAction == "ldown";
        e.type = bDown ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
        e.button.windowID = SDL_GetWindowID(pWindow);
        e.button.button = sAction[0] == 'r' ? SDL_BUTTON_RIGHT : SDL_BUTTON_LEFT;
        e.button.state = bDown ? SDL_PRESSED : SDL_RELEASED;
        e.button.clicks = 1;
        e.button.x = x;
        e.button.y = y;
        SDL_PushEvent(&e);
    } else if (sAction == "rel") {
        int dx = 0, dy = 0;
        sscanf(sArg.c_str(), "%d,%d", &dx, &dy);
        SDL_Event e;
        memset(&e, 0, sizeof e);
        e.type = SDL_MOUSEMOTION;
        e.motion.windowID = SDL_GetWindowID(pWindow);
        e.motion.x = 640;
        e.motion.y = 360;
        e.motion.xrel = dx;
        e.motion.yrel = dy;
        SDL_PushEvent(&e);
    } else if (sAction == "key" || sAction == "keydown" || sAction == "keyup") {
        SDL_Event e;
        memset(&e, 0, sizeof e);
        e.type = sAction == "keyup" ? SDL_KEYUP : SDL_KEYDOWN;
        e.key.windowID = SDL_GetWindowID(pWindow);
        e.key.keysym.scancode = static_cast<SDL_Scancode>(atoi(sArg.c_str()));
        e.key.keysym.sym = SDL_GetKeyFromScancode(e.key.keysym.scancode);
        SDL_PushEvent(&e);
        if (sAction == "key") s_queue.push_back({nFrame + 2, "keyup:" + sArg});
    } else if (sAction == "text") {
        SDL_Event e;
        memset(&e, 0, sizeof e);
        e.type = SDL_TEXTINPUT;
        e.text.windowID = SDL_GetWindowID(pWindow);
        std::string t = sArg;
        for (char& c : t)
            if (c == '_') c = ' ';
        snprintf(e.text.text, sizeof e.text.text, "%s", t.c_str());
        SDL_PushEvent(&e);
    } else if (sAction == "joy" || sAction == "joyup") {
        // a raw joystick button (SDL_main's gamepadButtonById numbering: 0 A, 1 B, 2 X, 3 Y,
        // 9/10 the shoulders, 11-14 the d-pad), pressed for four frames; `joy:BUTTON,PAD` presses it
        // on scripted pad PAD (default 0), so two pads can report one press
        SDL_Event e;
        memset(&e, 0, sizeof e);
        e.type = sAction == "joy" ? SDL_JOYBUTTONDOWN : SDL_JOYBUTTONUP;
        size_t nComma = sArg.find(',');
        // no real device (an id below 0): SDL_PollEvent below keeps it
        e.jbutton.which = -1 - (nComma == std::string::npos ? 0 : atoi(sArg.c_str() + nComma + 1));
        e.jbutton.button = static_cast<Uint8>(atoi(sArg.c_str()));
        e.jbutton.state = sAction == "joy" ? SDL_PRESSED : SDL_RELEASED;
        SDL_PushEvent(&e);
        if (sAction == "joy") s_queue.push_back({nFrame + 4, "joyup:" + sArg});
    } else if (sAction == "jaxis") {
        // a raw joystick axis, SDL_main's numbering (0/1 left stick, 2/3 right, 4/5 the triggers):
        // jaxis:axis,value (-32768..32767)
        SDL_Event e;
        memset(&e, 0, sizeof e);
        e.type = SDL_JOYAXISMOTION;
        e.jaxis.which = -1;
        e.jaxis.axis = static_cast<Uint8>(atoi(sArg.c_str()));
        size_t c = sArg.find(',');
        e.jaxis.value = static_cast<Sint16>(c == std::string::npos ? 0 : atoi(sArg.c_str() + c + 1));
        SDL_PushEvent(&e);
    } else if (sAction == "pad" || sAction == "paddown" || sAction == "padup" || sAction == "padaxis") {
        // a virtual SDL game controller, attached on first use (a real hot-plug, so SDL_main opens
        // it as it opens a pad): SDL_GameController button names (a, b, dpup, dpdown, ...);
        // `pad:NAME` holds it for four frames; `padaxis:NAME,VALUE` sets an axis (leftx, lefty, ...)
        static SDL_Joystick* s_pVirtualPad;
        if (!s_pVirtualPad) {
            SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
            int nIndex = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX,
                                                   SDL_CONTROLLER_BUTTON_MAX, 0);
            static auto realOpen = Real<SDL_Joystick* (*)(int)>("SDL_JoystickOpen");
            s_pVirtualPad = nIndex >= 0 ? realOpen(nIndex) : 0;
            fprintf(stderr, "k1: virtual pad: attach -> %d (%s)\n", nIndex,
                    s_pVirtualPad ? "open" : SDL_GetError());
            if (!s_pVirtualPad) return;
        }
        if (sAction == "padaxis") {
            size_t c = sArg.find(',');
            SDL_GameControllerAxis a = SDL_GameControllerGetAxisFromString(sArg.substr(0, c).c_str());
            if (a != SDL_CONTROLLER_AXIS_INVALID && c != std::string::npos)
                SDL_JoystickSetVirtualAxis(s_pVirtualPad, a, static_cast<Sint16>(atoi(sArg.c_str() + c + 1)));
            return;
        }
        SDL_GameControllerButton b = SDL_GameControllerGetButtonFromString(sArg.c_str());
        if (b == SDL_CONTROLLER_BUTTON_INVALID) return;
        SDL_JoystickSetVirtualButton(s_pVirtualPad, b, sAction == "padup" ? 0 : 1);
        if (sAction == "pad") s_queue.push_back({nFrame + 4, "padup:" + sArg});
    } else if (sAction == "rstick") {
        // the right stick's x axis, as SDL_main stores it (0 releases), with a pad reported attached
        ios_gamepad_analog1X = static_cast<float>(atof(sArg.c_str()));
        gamepadConnected = true;
    } else if (sAction == "con") {
        std::string t = sArg;
        for (char& c : t)
            if (c == '+') c = ' ';
        fprintf(stderr, "k1: con %s -> %s\n", t.c_str(), FuncInterp(&t[0]));
    } else if (sAction == "achieve") {
        k2host_unlock_achievement(sArg.c_str());   // as the engine's unlock (shim_achievements.cpp)
    } else if (sAction == "shot") {
        WriteShot(pWindow, ("k2shot_" + sArg + ".ppm").c_str());
    } else if (sAction == "quit") {
        fprintf(stderr, "k1: quit requested\n");
        _exit(0);
    }
}

void MaybeInput(SDL_Window* pWindow)
{
    static const char* s_pszInput = getenv("K2_INPUT");
    static const char* s_pszCtl = getenv("K2_CTL");
    static int s_nFrame;
    int nFrame = ++s_nFrame;
    if (s_nFrame == 1 && s_pszInput) {
        for (const char* p = s_pszInput; *p;) {
            const char* end = strchr(p, ';');
            std::string item(p, end ? static_cast<size_t>(end - p) : strlen(p));
            p = end ? end + 1 : p + item.size();
            size_t c = item.find(':');
            if (c != std::string::npos) s_queue.push_back({atoi(item.c_str()), item.substr(c + 1)});
        }
    }
    if (s_pszCtl) {
        if (FILE* f = fopen(s_pszCtl, "r")) {
            char line[512];
            while (fgets(line, sizeof line, f)) {
                std::string s(line);
                while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
                if (s.empty() || s[0] == '#') continue;
                int nDelay = 0;
                if (s[0] == '+') {
                    nDelay = atoi(s.c_str() + 1);
                    size_t c = s.find(':');
                    s = c == std::string::npos ? "" : s.substr(c + 1);
                }
                s_queue.push_back({nFrame + nDelay, s});
            }
            fclose(f);
            // remove() fails on Windows for a Wine Z: path; an emptied file is read as nothing.
            if (remove(s_pszCtl) != 0)
                if (FILE* t = fopen(s_pszCtl, "w")) fclose(t);
        }
    }
    for (size_t i = 0; i < s_queue.size();) {
        if (s_queue[i].first > nFrame) {
            i++;
            continue;
        }
        std::string item = s_queue[i].second;
        s_queue.erase(s_queue.begin() + static_cast<int64_t>(i));
        size_t c = item.find(':');
        DoAction(pWindow, nFrame, item.substr(0, c), c == std::string::npos ? "" : item.substr(c + 1));
    }
}
} // namespace


namespace {
// A pad button as SDL_main is handed it: 0 A, 1 B, 2 X, 3 Y crossed when the setting asks.
Uint8 FaceButton(Uint8 nButton)
{
    return nButton < 4 && k2qol_swap_face_buttons() ? nButton ^ 1 : nButton;
}
} // namespace

#ifdef __ANDROID__
// Android's SDL already numbers a pad's buttons in GameController order.
extern "C" int SDL_PollEvent(SDL_Event* e)
{
    static auto real = Real<int (*)(SDL_Event*)>("SDL_PollEvent");
    int r = real(e);
    if (r && e && (e->type == SDL_JOYBUTTONDOWN || e->type == SDL_JOYBUTTONUP))
        e->jbutton.button = FaceButton(e->jbutton.button);
    return r;
}
#else
// On a host pad, SDL_main numbers joystick buttons and axes the way Android's SDL does, which is
// SDL's GameController order (0 A, 1 B, 2 X, 3 Y, 4 Back, 5 Guide, 6 Start, 7/8 the stick clicks,
// 9/10 the shoulders, 11-14 the d-pad; axes 0-3 the sticks, 4/5 the triggers). A desktop driver's
// raw numbering is its own, so every joystick SDL knows as a game controller is also opened as one,
// its raw events are dropped, and its controller events are handed on as joystick events in that
// order.
//
// Every game controller is handed on merged into one, as shim/shim_dinput.cpp presents KOTOR II's
// pad: a button is down while any pad holds it, an axis reads the pad pushed furthest from rest.
// SDL_main keeps no state per device and counts the release of a button it does not hold as a
// press (MacMain.cpp's SDL_JOYBUTTONUP), so two pads reporting one physical press -- Steam's virtual
// gamepad and the Steam Deck's own controls -- would act on it twice, on the press and again on the
// second release. Scripted pads (`joy:BUTTON,PAD`, ids below 0) are merged the same way. The
// physical controller behind Steam's virtual gamepad is not read at all (k2shim_pad_steam_owned).
extern "C" int k2shim_pad_steam_owned(int nIndex);   // shim/shim_dinput.cpp

namespace {
std::vector<SDL_JoystickID> s_controllerIds;

bool IsControllerId(SDL_JoystickID nId)
{
    for (SDL_JoystickID n : s_controllerIds)
        if (n == nId) return true;
    return false;
}

// While a game controller is attached, a joystick SDL has no controller mapping for is not read:
// its raw numbering is not SDL_main's, and it is the same pad again when SDL lists one device
// twice.
bool AnyControllerAttached()
{
    for (SDL_JoystickID n : s_controllerIds) {
        SDL_GameController* p = SDL_GameControllerFromInstanceID(n);
        if (p && SDL_GameControllerGetAttached(p)) return true;
    }
    return false;
}

// Re-listed whenever SDL's joystick count changes, since Steam's virtual gamepad can arrive after
// the controller it stands for.
bool IsSteamOwnedId(SDL_JoystickID nId)
{
    static std::vector<SDL_JoystickID> s_owned;
    static int s_nListed = -1;
    const int n = SDL_NumJoysticks();
    if (n != s_nListed) {
        s_nListed = n;
        s_owned.clear();
        for (int i = 0; i < n; ++i) {
            if (!k2shim_pad_steam_owned(i)) continue;
            s_owned.push_back(SDL_JoystickGetDeviceInstanceID(i));
            fprintf(stderr, "k1: pad %d (%s) not read: Steam Input reads it and presents it as its "
                    "virtual gamepad\n", i, SDL_JoystickNameForIndex(i));
        }
    }
    for (SDL_JoystickID o : s_owned)
        if (o == nId) return true;
    return false;
}

struct PadState {
    SDL_JoystickID id;
    uint32_t nButtons;
    Sint16 aAxes[SDL_CONTROLLER_AXIS_MAX];
};
std::vector<PadState> s_padStates;
Sint16 s_aMergedAxes[SDL_CONTROLLER_AXIS_MAX];

PadState& PadStateOf(SDL_JoystickID nId)
{
    for (PadState& s : s_padStates)
        if (s.id == nId) return s;
    s_padStates.push_back(PadState{nId, 0, {}});
    return s_padStates.back();
}

uint32_t MergedButtons()
{
    uint32_t n = 0;
    for (const PadState& s : s_padStates) n |= s.nButtons;
    return n;
}

// Whether the merged pad's button changed.
bool MergeButton(SDL_JoystickID nId, Uint8 nButton, bool bDown)
{
    if (nButton >= 32) return true;
    const uint32_t nBit = 1u << nButton;
    const bool bWas = (MergedButtons() & nBit) != 0;
    PadState& s = PadStateOf(nId);
    s.nButtons = bDown ? s.nButtons | nBit : s.nButtons & ~nBit;
    return ((MergedButtons() & nBit) != 0) != bWas;
}

int AxisFurthest(int nAxis)
{
    int nBest = 0;
    for (const PadState& s : s_padStates)
        if (abs(s.aAxes[nAxis]) > abs(nBest)) nBest = s.aAxes[nAxis];
    return nBest;
}

// An axis value as SDL_main reads it. A trigger counts as pressed past a quarter of its travel
// (SDL_main tests > 0). A stick inside SDL_main's dead zone (10000) is at rest: there its analog
// value is 0, and as menu buttons (AxisAsButtons) a stick at rest also clears the held d-pad
// direction on that axis, so every small movement of a resting stick between a d-pad press and its
// release (a Steam Deck's sticks report a steady trickle of them) makes the release count as a
// second press. A stick is therefore handed on only when it leaves, moves outside, or re-enters
// the dead zone.
Sint16 AxisAsMainReadsIt(int nAxis, int nValue)
{
    if (nAxis >= SDL_CONTROLLER_AXIS_TRIGGERLEFT) return static_cast<Sint16>(nValue < 8192 ? 0 : nValue);
    return static_cast<Sint16>(abs(nValue) < 10000 ? 0 : nValue);
}

// Whether the merged pad's axis changed as SDL_main reads it; *pnMerged is that value.
bool MergeAxis(SDL_JoystickID nId, Uint8 nAxis, Sint16 nValue, Sint16* pnMerged)
{
    *pnMerged = nValue;
    if (nAxis >= SDL_CONTROLLER_AXIS_MAX) return true;
    PadStateOf(nId).aAxes[nAxis] = nValue;
    *pnMerged = AxisAsMainReadsIt(nAxis, AxisFurthest(nAxis));
    if (*pnMerged == s_aMergedAxes[nAxis]) return false;
    s_aMergedAxes[nAxis] = *pnMerged;
    return true;
}

// A removed pad holds nothing (SDL_main resets the whole pad state on a removal).
void ForgetPad(SDL_JoystickID nId)
{
    for (size_t i = 0; i < s_padStates.size(); ++i) {
        if (s_padStates[i].id != nId) continue;
        s_padStates.erase(s_padStates.begin() + static_cast<int64_t>(i));
        break;
    }
    for (int a = 0; a < SDL_CONTROLLER_AXIS_MAX; ++a) s_aMergedAxes[a] = AxisAsMainReadsIt(a, AxisFurthest(a));
}
} // namespace

extern "C" SDL_Joystick* SDL_JoystickOpen(int nIndex)
{
    static auto real = Real<SDL_Joystick* (*)(int)>("SDL_JoystickOpen");
    // The controller behind Steam's virtual gamepad stays closed: opening a Valve controller hands
    // it to SDL's own driver, which reconfigures it under Steam, and Steam's virtual gamepad then
    // reports nothing. SDL_main skips a joystick that does not open.
    if (k2shim_pad_steam_owned(nIndex)) return 0;
    SDL_Joystick* pJoystick = real(nIndex);
    if (pJoystick && SDL_IsGameController(nIndex) && SDL_GameControllerOpen(nIndex)) {
        SDL_JoystickID nId = SDL_JoystickInstanceID(pJoystick);
        if (!IsControllerId(nId)) {
            if (s_controllerIds.empty()) {
                const char* pAllow = getenv("SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD");
                const char* pIgnore = getenv("SDL_GAMECONTROLLER_IGNORE_DEVICES");
                if (pAllow || pIgnore)
                    fprintf(stderr, "k1: Steam Input: SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD=%s "
                            "SDL_GAMECONTROLLER_IGNORE_DEVICES=%s\n", pAllow ? pAllow : "(unset)",
                            pIgnore ? pIgnore : "(unset)");
            }
            s_controllerIds.push_back(nId);
            fprintf(stderr, "k1: pad %d (%s, %04x:%04x) read as a game controller\n", nIndex,
                    SDL_GameControllerNameForIndex(nIndex), SDL_JoystickGetDeviceVendor(nIndex),
                    SDL_JoystickGetDeviceProduct(nIndex));
        }
    }
    return pJoystick;
}

namespace {
// K1_PAD_TRACE=1: every pad event SDL hands over ("sdl") and what SDL_main is handed ("main");
// =2 also a resting stick's small movements as SDL reports them.
int PadTrace()
{
    static const int s = getenv("K1_PAD_TRACE") ? atoi(getenv("K1_PAD_TRACE")) : 0;
    return s;
}

void TracePad(const char* pszWho, const SDL_Event* e)
{
    if (e->type < SDL_JOYAXISMOTION || e->type >= SDL_CONTROLLERSENSORUPDATE) return;
    int nWhich = e->jbutton.which, nA = 0, nB = 0;
    switch (e->type) {
    case SDL_JOYAXISMOTION: nA = e->jaxis.axis; nB = e->jaxis.value; break;
    case SDL_JOYHATMOTION: nA = e->jhat.hat; nB = e->jhat.value; break;
    case SDL_JOYBUTTONDOWN: case SDL_JOYBUTTONUP: nA = e->jbutton.button; nB = e->jbutton.state; break;
    case SDL_CONTROLLERAXISMOTION: nA = e->caxis.axis; nB = e->caxis.value; break;
    case SDL_CONTROLLERBUTTONDOWN: case SDL_CONTROLLERBUTTONUP: nA = e->cbutton.button; nB = e->cbutton.state; break;
    default: break;
    }
    // a resting stick's trickle, as SDL reports it (what SDL_main is handed is always logged)
    if (PadTrace() < 2 && pszWho[0] == 's' && (e->type == SDL_JOYAXISMOTION || e->type == SDL_CONTROLLERAXISMOTION) &&
        nB > -8000 && nB < 8000)
        return;
    fprintf(stderr, "k1: pad %s: t=%u type=0x%x which=%d %d %d\n", pszWho, e->common.timestamp, e->type,
            nWhich, nA, nB);
}

int PollPadEvent(SDL_Event* e)
{
    static auto real = Real<int (*)(SDL_Event*)>("SDL_PollEvent");
    for (;;) {
        int r = real(e);
        if (!r || !e) return r;
        if (PadTrace()) TracePad("sdl", e);
        switch (e->type) {
        case SDL_JOYAXISMOTION:
        case SDL_JOYBALLMOTION:
        case SDL_JOYHATMOTION:
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
            if (IsControllerId(e->jbutton.which)) continue;   // `which` sits at the same offset in all
            if (e->jbutton.which >= 0 && AnyControllerAttached()) continue;
            if (e->jbutton.which < 0 && (e->type == SDL_JOYBUTTONDOWN || e->type == SDL_JOYBUTTONUP) &&
                !MergeButton(e->jbutton.which, e->jbutton.button, e->type == SDL_JOYBUTTONDOWN))
                continue;
            return r;
        case SDL_JOYDEVICEREMOVED:
            ForgetPad(e->jdevice.which);
            return r;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP: {
            if (IsSteamOwnedId(e->cbutton.which) ||
                !MergeButton(e->cbutton.which, e->cbutton.button, e->type == SDL_CONTROLLERBUTTONDOWN))
                continue;
            SDL_ControllerButtonEvent c = e->cbutton;
            memset(e, 0, sizeof *e);
            e->type = c.type == SDL_CONTROLLERBUTTONDOWN ? SDL_JOYBUTTONDOWN : SDL_JOYBUTTONUP;
            e->jbutton.timestamp = c.timestamp;
            e->jbutton.which = c.which;
            e->jbutton.button = FaceButton(c.button);
            e->jbutton.state = c.state;
            return r;
        }
        case SDL_CONTROLLERAXISMOTION: {
            SDL_ControllerAxisEvent c = e->caxis;
            if (IsSteamOwnedId(c.which) || !MergeAxis(c.which, c.axis, c.value, &c.value)) continue;
            memset(e, 0, sizeof *e);
            e->type = SDL_JOYAXISMOTION;
            e->jaxis.timestamp = c.timestamp;
            e->jaxis.which = c.which;
            e->jaxis.axis = c.axis;
            e->jaxis.value = c.value;
            return r;
        }
        default:
            return r;
        }
    }
}
} // namespace

extern "C" int SDL_PollEvent(SDL_Event* e)
{
    int r = PollPadEvent(e);
    if (r && e && PadTrace()) TracePad("main", e);
    return r;
}
#endif

void k1gl_frame();   // k1port.cpp: the one-frame GL trace
extern "C" void SDL_GL_SwapWindow(SDL_Window* pWindow)
{
    static auto real = Real<void (*)(SDL_Window*)>("SDL_GL_SwapWindow");
    k1gl_frame();
    MaybeScreenshot(pWindow);
    MaybeInput(pWindow);
    // K2_FPS=1: log the swap rate and the slowest frame once a second.
    static const bool s_bFps = getenv("K2_FPS") != 0;
    if (s_bFps) {
        static Uint64 s_nStart = SDL_GetPerformanceCounter(), s_nLast = s_nStart, s_nWorst;
        static unsigned s_nFrames, s_nTotal;
        ++s_nFrames, ++s_nTotal;
        Uint64 nNow = SDL_GetPerformanceCounter(), nFreq = SDL_GetPerformanceFrequency();
        if (nNow - s_nLast > s_nWorst) s_nWorst = nNow - s_nLast;
        s_nLast = nNow;
        if (nNow - s_nStart >= nFreq) {
            fprintf(stderr, "k1: fps %.1f, worst %.1f ms (frame %u)\n",
                    s_nFrames * static_cast<double>(nFreq) / static_cast<double>(nNow - s_nStart),
                    s_nWorst * 1000.0 / static_cast<double>(nFreq), s_nTotal);
            s_nStart = nNow;
            s_nFrames = 0;
            s_nWorst = 0;
        }
    }
#ifdef K2P_MOBILE_HOST
    k2android_gl_pre_swap();
#endif
    k2shim_gl_present(pWindow, real);
#ifdef K2P_MOBILE_HOST
    k2android_gl_post_swap();
#endif
}
