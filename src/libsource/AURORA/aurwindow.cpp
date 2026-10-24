// Binding is the debug console's key-binding table (see Binding.h). This translation unit holds the
// whole windowing and console cluster: CalcFPSDeltaT, ComputeMouseVector, Reshape and reshape,
// doconsole, the TimeTrigger cluster, AurWndProc, AurCreateWindow, AurPrintStats, AurWindowUp,
// AurCloseWindow and AurSwapBuffers.
#include "k2stdint.h"
#include "port/aslglapi.h"
#if KOTOR1
#define K1_AURORA_LIST  // this TU is K1's AURORA library (List.h grows from 8)
#endif
#include "aurwindow.h"
#include "include/aurora.h"
#include "aurinternal.h"
#include "port/win32api.h"
#include "port/win32const.h"

// The window-class values AurCreateWindow uses that port/win32const.h does not name, as winuser.h
// spells them (the engine never includes it).
enum {
    CS_OWNDC = 0x0020,
    kIdiWinLogo = 32517,                // IDI_WINLOGO
    kOverlappedClippedWindow = 0x06CF0000,  // WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN
};

// A mouse coordinate arrives as a 16-bit word: its sign bit, the word's span, and the high
// half that sign-extends it.
enum
{
    kCoordSignBit = 0x8000,
    kCoordSpan = 0x10000,
};
enum : unsigned int
{
    kCoordSignFill = 0xffff0000,
};

float GlobalTime;
float FPS;
int MouseY;
int MouseXold;
int MouseYold;
int MouseLeft;
int MouseRight;
void* hDC;
int entertext;
char comstring[512];
int BindingActivated;
int aurlite;
HWND__* aurlitehwnd;

extern float FPS;
extern float DeltaT;
extern float GlobalTime;

// Maintains the frame-timing globals. The statics are TU-local: a one-shot init latch, the FPS
// window's base time and two frame counters, the current sample, and the previous frame's tick.
// The FPS arm works in double and only narrows to float at the end; the delta arm is
// single-precision throughout and substitutes 0.002f when two frames land on the same tick.
void CalcFPSDeltaT()
{
    static bool bInit;
    static int nLastFPS;
    static int nT2;
    static int nCur;
    static int nPrevFrame;
    static int nFrames;
    static int nFrames2;

    if (!bInit) {
        nFrames = 0;
        int t = timeGetTime();
        nPrevFrame = t;
        nLastFPS = t;
        bInit = true;
    }
    nFrames++;
    nFrames2++;
    int now = timeGetTime();
    double dDiff = now - nLastFPS;
    double dElapsed = dDiff / 1000000.0;
    nCur = now;
    float fFPS;
    if (dElapsed != 0.0)
        fFPS = static_cast<float>(nFrames / dElapsed);
    else
        fFPS = -1.0f;
    if (dElapsed < 0.0 || (dElapsed > 2.0 && nFrames >= 11)) {
        nLastFPS = nT2;
        nFrames = nFrames2;
        nT2 = timeGetTime();
        now = nCur;
        nFrames2 = 0;
    }
    int nOldFrame = nPrevFrame;
    nPrevFrame = now;
    FPS = fFPS;
    float fDelta = (now - nOldFrame) / 1000000.0f;
    DeltaT = (now == nOldFrame) ? 0.002f : fDelta;
    GlobalTime = GlobalTime + DeltaT;
}

// The arguments are raw 16-bit mouse coordinates zero-extended into ints, where bit 15 means
// negative, so the value is sign-extended by hand rather than with a halfword load. Y is flipped
// around the screen height.
extern int MouseX;
int MouseX;
extern int MouseY;
extern int g_nScreenHeight;
void ComputeMouseVector(int nX, int nY)
{
    nX = (nX & kCoordSignBit) ? nX - kCoordSpan : nX;
    nY = (nY & kCoordSignBit) ? nY - kCoordSpan : nY;
    MouseX = nX;
    MouseY = g_nScreenHeight - nY;
}

// The matrix-stack selector table and the window and viewport globals this touches.
extern int g_nScreenWidth;
extern float ViewAngle;
float ViewAngle = 45.0f;
extern float clipnear;
float clipnear = 0.100000001f;
extern float clipfar;
float clipfar = 50.0f;
extern const int transformTranslationTable[3];

extern "C" void gluPerspective(double fovy, double aspect, double zNear, double zFar);

// Stores the RAW args to the globals first, bails on degenerate sizes, and only then clamps
// to >= 1 for the viewport/aspect math (the `csinc` pairs: `w > 1 ? w : 1`).
void Reshape(int nWidth, int nHeight)
{
    int w = (nWidth > 1) ? nWidth : 1;
    int h = (nHeight > 1) ? nHeight : 1;
    g_nScreenWidth = nWidth;
    g_nScreenHeight = nHeight;
    if (nWidth < 2 || nHeight < 2)
        return;
    ASLgl::glViewport(0, 0, w, h);
    ASLgl::glMatrixMode(transformTranslationTable[0]);
    ASLgl::glLoadIdentity();
    gluPerspective(ViewAngle, static_cast<float>(w) / static_cast<float>(h), clipnear, clipfar);
    ASLgl::glMatrixMode(transformTranslationTable[2]);
    ASLgl::glLoadIdentity();
}

// The console command re-applies the current screen size (its arguments are never parsed) and
// reports the view parameters into a TU-local buffer. Its name formatter is a file-local fortified
// varargs wrapper.
#include "port/libcbase.h"
#include "port/libcstring.h"
static void snprintff(char* buf, uint64_t n, const char* fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    __vsprintf_chk(buf, 0, n, fmt, ap);
    __builtin_va_end(ap);
}
const char* reshape(char* pszArgs)
{
    K2_UNUSED(pszArgs);
    static char szBuf[256];
    Reshape(g_nScreenWidth, g_nScreenHeight);
    snprintff(szBuf, 256, "win=%dx%d fov=%2.0f clip=%4.3f,%4.1f",
              g_nScreenWidth, g_nScreenHeight, ViewAngle, clipnear, clipfar);
    return szBuf;
}

// The console's command interpreter and its pre-interp sibling, both returning a string that
// doconsole posts and DoBindings discards.
char* FuncInterp(char* pszCommand);
char* FuncPreInterp(char* pszCommand);
// Posts one line to the on-screen debug text list.
void AurPostString(char* pszString, int nX, int nY, float fLife);
// Handles tab completion, rewriting the console line and filling the out-list with the candidate
// strings.
char* CommandCompletion(char* pszPartial, List<char*>& matches);

extern char comstring[512];   // the console input line
extern int entertext;         // set when the console's enter key was hit

// Runs the console's per-character state machine. The Enter key echoes the line and runs the
// interpreter, Tab completes (and with multiple matches prints up to 15 of them, scrolling by 5 per
// press), backspace deletes the last character, and anything else appends to the line, clamped at
// 500 characters.
void doconsole(char c)
{
    static int nScroll;   // the completion list's scroll offset
    switch (static_cast<unsigned char>(c)) {
    case '\r':
    case '\n': {
        entertext = 0;
        char szBuf[1024];
        snprintff(szBuf, 1024, "* %s", comstring);
        AurPostString(szBuf, 0, 0, 5.0f);
        AurPostString(FuncInterp(comstring), 0, 1, 5.0f);
        break;
    }
    case '\t': {
        List<char*> matches;
        char* pCom = comstring;
        __strcpy_chk(pCom, CommandCompletion(pCom, matches), 512);
        AurPostString(FuncPreInterp(pCom), 0, 1, 1.0f);
        if (matches.m_count >= 2) {
            float fSize = 1.0f;
            if (matches.m_count > 15)
                fSize = 4.0f;
            if (matches.m_count < 15)
                nScroll = 0;
            for (int i = 0; i < 15 && i < matches.m_count; i++)
                AurPostString(matches.m_pData[(nScroll + i) % matches.m_count], 4, i + 6, fSize);
            nScroll += 5;
        }
        break;
    }
    case '\b': {
        char* pCom = comstring;
        if (__strlen_chk(pCom, 512) != 0)
            pCom[__strlen_chk(pCom, 512) - 1] = '\0';
        break;
    }
    default: {
        char* pCom = comstring;
        int n = static_cast<int>(__strlen_chk(pCom, 512));
        if (n > 500)
            n = 500;
        pCom[n] = c;
        pCom[n + 1] = '\0';
        break;
    }
    }
}

List<Binding*> bindings;          // the binding table
extern int BindingActivated;      // set around the FuncInterp call: 1 on the press

Binding::Binding()
{
    m_cKey = '\0';
    m_szCommand[0] = '\0';
    bindings.Add(this);
    m_bDown = 0;
    m_bRepeat = 0;
}

Binding::~Binding()
{
    bindings.Remove(this);
}

Binding* FindBinding(char cKey)
{
    for (int64_t i = 0; i < bindings.m_count; i++)
        if (bindings.m_pData[i]->m_cKey == cKey)
            return bindings.m_pData[i];
    return 0;
}

void DoBindings()
{
    // GetAsyncKeyState is called twice per entry with the same key and the first result is thrown
    // away: the call reports "pressed since the last call" in its low bit, so that first call is
    // what clears the state.
    for (int64_t i = 0; i < bindings.m_count; i++) {
        GetAsyncKeyState(bindings.m_pData[i]->m_cKey);
        if (GetAsyncKeyState(bindings.m_pData[i]->m_cKey)) {
            if (bindings.m_pData[i]->m_bRepeat || !bindings.m_pData[i]->m_bDown) {
                BindingActivated = !bindings.m_pData[i]->m_bDown;
                FuncInterp(bindings.m_pData[i]->m_szCommand);
                BindingActivated = 0;
            }
            bindings.m_pData[i]->m_bDown = 1;
        } else {
            bindings.m_pData[i]->m_bDown = 0;
        }
    }
}

void unbindall()
{
    while (bindings.m_count)
        delete bindings.m_pData[bindings.m_count - 1];
}

// A pending timed console command, self-registering into the sorted global trigger list at
// construction and removing itself at destruction, the same fire-and-forget pattern as Binding.
// m_szCommand's bound is unpinned: nothing in this cluster reads it.
class TimeTrigger {
public:
    float m_fTime;              // GlobalTime deadline
    char m_szCommand[512];
    TimeTrigger(float fTime, char* pszCommand);
    ~TimeTrigger();
};
List<TimeTrigger*> timetriggers;

// Inserts the trigger sorted by m_fTime. Finds the first existing trigger whose time is at least
// ours, appends (which grows the list), then if the scan found an earlier slot shifts the tail up
// by one and drops this trigger into it. The append's own store at the old end is overwritten by
// the shift, and that redundancy is real.
TimeTrigger::TimeTrigger(float fTime, char* pszCommand)
{
    m_fTime = fTime;
    strcpy(m_szCommand, pszCommand ? pszCommand : "");
    int64_t nCount = timetriggers.m_count;
    int64_t nPos = 0;
    int bFound = 0;
    for (nPos = 0; nPos < nCount; nPos++) {
        if (!(m_fTime > timetriggers.m_pData[nPos]->m_fTime)) {
            bFound = 1;
            break;
        }
    }
    timetriggers.Add(this);
    if (bFound) {
        for (int64_t j = nCount; j > nPos; j--)
            timetriggers.m_pData[j] = timetriggers.m_pData[j - 1];
        timetriggers.m_pData[nPos] = this;
    }
}

TimeTrigger::~TimeTrigger()
{
    timetriggers.Remove(this);
}

// The deadline test must be written as a negated comparison, since a plain `>=` tests the wrong
// way around when the operands are unordered.
// Fires every trigger whose deadline has passed, in list order (the list is kept sorted, so only
// element 0 is ever tested). The delete re-reads element 0 after the command ran, because the
// command is a console line that can add or cancel triggers and so is not necessarily the trigger
// just fired.
void DoTimeTriggers()
{
    while (timetriggers.m_count != 0) {
        TimeTrigger* pTrigger = timetriggers.m_pData[0];
        if (!(pTrigger->m_fTime < GlobalTime))
            return;
        FuncInterp(pTrigger->m_szCommand);
        delete timetriggers.m_pData[0];
    }
}

extern HWND__* hWnd;
HWND__* hWnd;
extern HWND__* aurlitehwnd;
extern int WindowActive;
int WindowActive = 1;
extern int MouseXold;
extern int MouseYold;
// Draws the posted debug strings during the buffer swap.
void AurRenderStrings();

// The window object AurCreateWindow creates and AurCloseWindow shuts down is a file-scope pointer,
// not the exported g_pAurora the GUI pages drive.
#if !KOTOR1
static CAurInternal* s_pAurora;
#endif

char* trace(char* pszString);
extern int aurlite;        // set while the window is up (1 create / 0 close)
extern void* hDC;

// Two more file-scope statics of this translation unit, the window-title copy and the module
// handle.
#if !KOTOR1
static char* g_pszWindowName;
static void* g_hInstance;
#endif

// The window procedure, a two-level switch over the message codes, with everything else tailing
// DefWindowProcA. The mouse coordinate sign-extension here uses a mask idiom rather than
// ComputeMouseVector's subtraction, because WndProc carries its own copy of that code.
extern int MouseLeft;
extern int MouseRight;
void AurTextureRefreshAll();

#if !KOTOR1  // K1's library has no Win32 window: SDL owns it (MacMain.cpp's SDL_main)
static char g_paintStruct[72];
static unsigned char g_bAltEnterQuit;

int64_t AurWndProc(HWND__* hwnd, unsigned int nMsg, unsigned long long wParam, int64_t lParam)
{
    switch (nMsg) {
    case WM_DESTROY:
        if (g_bAltEnterQuit == 1) {
            g_bAltEnterQuit = 0;
            return 0;
        }
        /* fall through */
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        if (s_pAurora != 0)
            Reshape(static_cast<int>(lParam & 0xffff), static_cast<int>((lParam >> 16) & 0xffff));
        PostMessageA(hwnd, WM_PAINT, 0, static_cast<int64_t>(0));
        return 0;
    case WM_ACTIVATE:
        if (s_pAurora != 0) {
            if (wParam != 0) {
                WindowActive = 1;
                if (s_pAurora->FullScreen() != 0) {
                    ShowWindow_Win32(hwnd, 1);
                    s_pAurora->RestoreAuroraDisplayMode();
                }
            } else {
                WindowActive = 0;
                if (s_pAurora->FullScreen() != 0) {
                    s_pAurora->RestoreDesktopDisplayMode();
                    ShowWindow_Win32(hwnd, 6);
                }
            }
        }
        return 0;
    case WM_PAINT:
        BeginPaint(hwnd, g_paintStruct);
        EndPaint(hwnd, g_paintStruct);
        return 0;
    case WM_CHAR:
        if (entertext != 0) {
            doconsole(static_cast<char>(wParam));
        } else if (wParam == '\n' || wParam == VK_RETURN) {
            entertext = 1;
            comstring[0] = '\0';
        } else if (wParam == VK_ESCAPE) {
            // Esc quits (sharing WM_CLOSE's PostQuitMessage), it does not open the console.
            PostQuitMessage(0);
        }
        return 0;
    case WM_SYSKEYDOWN:  // alt+enter toggles the display mode; anything else here
                         // falls through to DefWindowProcA, not to the handled return
        if (s_pAurora != 0 && wParam == VK_RETURN && (lParam & KEYDATA_ALTDOWN)) {
            s_pAurora->FullScreen();
            g_bAltEnterQuit = 1;
            ReleaseDC(hwnd, hDC);
            s_pAurora->ShutDown(1, 1);
            AurTextureRefreshAll();
            hDC = GetDC(hwnd);
            return 0;
        }
        break;
    case WM_MOUSEMOVE: {
        int nX = static_cast<int>(lParam);
        int nY = static_cast<int>(lParam >> 16);
        MouseX = (nX & kCoordSignBit) ? nX | static_cast<int>(kCoordSignFill) : nX & 0xffff;
        MouseY = g_nScreenHeight - ((nY & kCoordSignBit) ? nY | static_cast<int>(kCoordSignFill) : nY & 0xffff);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        SetCapture(hwnd);
        int nX = static_cast<int>(lParam);
        int nY = static_cast<int>(lParam >> 16);
        MouseX = (nX & kCoordSignBit) ? nX | static_cast<int>(kCoordSignFill) : nX & 0xffff;
        MouseY = g_nScreenHeight - ((nY & kCoordSignBit) ? nY | static_cast<int>(kCoordSignFill) : nY & 0xffff);
        MouseLeft = 1;
        return 0;
    }
    case WM_LBUTTONUP: {
        int nX = static_cast<int>(lParam);
        int nY = static_cast<int>(lParam >> 16);
        MouseX = (nX & kCoordSignBit) ? nX | static_cast<int>(kCoordSignFill) : nX & 0xffff;
        MouseY = g_nScreenHeight - ((nY & kCoordSignBit) ? nY | static_cast<int>(kCoordSignFill) : nY & 0xffff);
        MouseLeft = 0;
        if (MouseRight == 0)
            ReleaseCapture();
        return 0;
    }
    case WM_RBUTTONDOWN: {
        SetCapture(hwnd);
        int nX = static_cast<int>(lParam);
        int nY = static_cast<int>(lParam >> 16);
        MouseX = (nX & kCoordSignBit) ? nX | static_cast<int>(kCoordSignFill) : nX & 0xffff;
        MouseY = g_nScreenHeight - ((nY & kCoordSignBit) ? nY | static_cast<int>(kCoordSignFill) : nY & 0xffff);
        MouseRight = 1;
        return 0;
    }
    case WM_RBUTTONUP: {
        int nX = static_cast<int>(lParam);
        int nY = static_cast<int>(lParam >> 16);
        MouseX = (nX & kCoordSignBit) ? nX | static_cast<int>(kCoordSignFill) : nX & 0xffff;
        MouseY = g_nScreenHeight - ((nY & kCoordSignBit) ? nY | static_cast<int>(kCoordSignFill) : nY & 0xffff);
        MouseRight = 0;
        if (MouseLeft == 0)
            ReleaseCapture();
        return 0;
    }
    }
    return DefWindowProcA(hwnd, nMsg, wParam, lParam);
}

// Installs the "OpenGL" class on the first call (the window-class init order is load-bearing), then
// either adopts the aurlite harness window or creates one sized to the screen globals, and spins up
// the engine object on top.
void* AurCreateWindow(char* pszName, int nCmdShow, WNDPROC pfnWndProc)
{
    K2_UNUSED(nCmdShow);
    g_pszWindowName = pszName;
    if (g_hInstance == 0) {
        WNDCLASSA wc;
        g_hInstance = GetModuleHandleA(0);
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = pfnWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = g_hInstance;
        wc.hIcon = LoadIconA(0, kIdiWinLogo);
        wc.hCursor = LoadCursorA(0, IDC_ARROW);
        wc.hbrBackground = 0;
        wc.lpszMenuName = 0;
        wc.lpszClassName = const_cast<char*>("OpenGL");
        if (RegisterClassA(&wc) == 0) {
            MessageBoxA(0, "RegisterClass() failed:  Cannot register window class.", "Error", 0);
            return 0;
        }
    }
    if (aurlitehwnd != 0) {
        hWnd = aurlitehwnd;
    } else {
        hWnd = CreateWindowExA(0, "OpenGL", pszName, kOverlappedClippedWindow, 0, 0,
                               g_nScreenWidth, g_nScreenHeight, 0, 0, g_hInstance, 0);
        if (hWnd == 0) {
            MessageBoxA(0, "CreateWindow() failed:  Cannot create a window.", "Error", 0);
            return 0;
        }
    }
    s_pAurora = NewCAurora();
    hDC = GetDC(hWnd);
    trace(const_cast<char*>("FPS"));
    aurlite = 1;
    return hWnd;
}

// The renderer teardown callees are the trace-log clear, the string and font teardown, and the
// aurora object destructor.
char* untrace(char* pszString);
void AurStringsFree();
void AurFreeFont();

// The unbind loop is written out here rather than calling unbindall, since the reference expands
// it, and this compiler will not expand the global function body.
void AurCloseWindow()
{
    untrace(0);
    while (bindings.m_count)
        delete bindings.m_pData[bindings.m_count - 1];
    AurStringsFree();
    AurFreeFont();
    aurlite = 0;
    s_pAurora->ShutDown(1, 1);
    DestroyCAurora();
    s_pAurora = 0;
    ReleaseDC(hWnd, hDC);
    DestroyWindow(hWnd);
}
#endif

void AurPrintStats()
{
    char szBuf[1024];
    szBuf[0] = '\0';
    if (entertext != 0) {
        snprintff(szBuf, 1024, "> %s", comstring);
        AurPostString(szBuf, 0, 0, -5.0f);
    }
}

// Pumps one frame, running the fps/delta bookkeeping, a mouse snapshot, the Win32 message loop
// (skipped when running under the "aurlite" harness window), timed console commands, and key
// bindings (only while no console text entry is active and the window is focused). Returns 0 only
// on a GetMessage quit (a zero message); the buffer swap is done separately.
#if !KOTOR1
int AurWindowUp()
{
    MSG msg;
    CalcFPSDeltaT();
    MouseXold = MouseX;
    MouseYold = MouseY;
    if (aurlitehwnd == 0) {
        while (PeekMessageA(&msg, hWnd, 0, 0, 0) != 0) {
            if (GetMessageA(&msg, hWnd, 0, 0) == 0)
                return 0;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
    DoTimeTriggers();
    if (entertext == 0 && WindowActive != 0)
        DoBindings();
    return 1;
}
#endif

void AurSwapBuffers()
{
    char szBuf[1024];
    szBuf[0] = '\0';
    if (entertext != 0) {
        snprintff(szBuf, 1024, "> %s", comstring);
        AurPostString(szBuf, 0, 0, -5.0f);
    }
    AurRenderStrings();
}

