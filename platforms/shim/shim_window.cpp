// Win32 windows, messages, device contexts, pixel formats, WGL and display modes on SDL2.
//
// Model, after the stock Mac build's HWND_Mac/HDC_Mac: an HWND is a K2Window wrapping one
// SDL_Window, with its HDC embedded. One message queue serves the main thread, the only thread the
// engine pumps. SDL events become the messages the engine's AurWndProc handles
// (WM_ACTIVATE/SETFOCUS/KILLFOCUS/SIZE/PAINT/CLOSE, WM_KEYDOWN/UP, WM_SYSKEYDOWN, WM_CHAR and the
// mouse messages); every event also feeds the DirectInput devices, which is where gameplay input is
// read.
#include <stdint.h>
#include <inttypes.h>
#include "k2shim.h"
#include "port/win32api.h"
#include "port/ASL2/Win/PortDevMode.h"
#include "aslgl.gen.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <deque>
#include <map>
#include <set>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

typedef int64_t (*WndProcFn)(HWND__*, unsigned int, unsigned long long, int64_t);

namespace {

// The Win32 structs are port/win32api.h's, 64-bit (the shim is always built as KOTOR II).
static_assert(sizeof(MSG) == 0x30 && offsetof(MSG, time) == 0x20, "MSG is 0x30");
static_assert(offsetof(WNDCLASSA, lpszClassName) == 0x40, "WNDCLASSA");

struct CreateStructA {             // CREATESTRUCTA, handed to WM_CREATE
    void* lpCreateParams;
    void* hInstance;
    void* hMenu;
    HWND__* hwndParent;
    int cy, cx, y, x;
    int64_t style;
    const char* lpszName;
    const char* lpszClass;
    unsigned int dwExStyle;
};

static_assert(sizeof(PIXELFORMATDESCRIPTOR) == 40 && offsetof(PIXELFORMATDESCRIPTOR, cDepthBits) == 0x17,
              "PIXELFORMATDESCRIPTOR");

// DEVMODEA, which is the engine's PortDevMode: 156 bytes, of which it reads back only
// bpp/width/height/Hz.
enum { DEVMODEA_SIZE = 0x9c };

enum : unsigned int {
    WM_CREATE = 0x01, WM_DESTROY = 0x02, WM_SIZE = 0x05, WM_ACTIVATE = 0x06, WM_SETFOCUS = 0x07,
    WM_KILLFOCUS = 0x08, WM_PAINT = 0x0f, WM_CLOSE = 0x10, WM_QUIT = 0x12, WM_ACTIVATEAPP = 0x1c,
    WM_SETCURSOR = 0x20, WM_LBUTTONDBLCLK = 0x203, WM_XBUTTONDOWN = 0x20b, WM_XBUTTONUP = 0x20c,
    WM_KEYDOWN = 0x100, WM_KEYUP = 0x101, WM_CHAR = 0x102, WM_SYSKEYDOWN = 0x104, WM_SYSKEYUP = 0x105,
    WM_MOUSEMOVE = 0x200, WM_LBUTTONDOWN = 0x201, WM_LBUTTONUP = 0x202, WM_RBUTTONDOWN = 0x204,
    WM_RBUTTONUP = 0x205, WM_MBUTTONDOWN = 0x207, WM_MBUTTONUP = 0x208, WM_MOUSEWHEEL = 0x20a,
};

// windows and DCs.

struct K2Window;
struct K2Dc {
    uint32_t magic;                // 'K2DC'
    K2Window* wnd;                 // NULL for the screen DC
    int pixelFormat;
};
struct K2Window {
    uint32_t magic;                // 'K2WN'
    SDL_Window* sdl;
    WndProcFn proc;
    std::string cls;
    int64_t userData;
    K2Dc dc;
    Uint32 sdlFlags;
    float scale;                   // drawable pixels per window point: 1, or the display's for a
                                   // HiDPI window (native-resolution fullscreen; iOS: always)
    int modeH;                     // the height the engine draws at
};
const uint32_t kWndMagic = 0x4b32574e, kDcMagic = 0x4b324443;

std::map<std::string, WndProcFn> g_classes;   // lower-cased class name -> window procedure
std::set<const void*> g_liveHandles;           // every K2Window and K2Dc we handed out
std::map<uint32_t, K2Window*> g_bySdlId;
K2Window* g_mainWindow;
K2Dc g_screenDc = { kDcMagic, 0, 0 };

// Handles are validated by lookup, not dereference: aurglcore declares wglCreateContext with no
// parameter, so its "HDC" is whatever it was handed.
K2Window* W(void* h)
{
    if (!h || !g_liveHandles.count(h)) return 0;
    K2Window* w = static_cast<K2Window*>(h);
    return w->magic == kWndMagic ? w : 0;
}
K2Dc* Dc(void* h)
{
    if (h == &g_screenDc) return &g_screenDc;
    if (!h || !g_liveHandles.count(h)) return 0;
    K2Dc* d = static_cast<K2Dc*>(h);
    return d->magic == kDcMagic ? d : 0;
}
std::string Lower(const char* s)
{
    std::string r(s ? s : "");
    for (char& c : r) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return r;
}

// the message queue.

// K2_INPUT's synthetic cursor, in client coordinates.
bool g_synthCursor;
int g_synthX, g_synthY;
bool g_appSuspended;
std::mutex g_qlock;
std::deque<MSG> g_queue;

void LandscapePoint(const K2Window* w, int* pX, int* pY);

void Post(HWND__* hwnd, unsigned int msg, unsigned long long wp, long long lp)
{
    MSG m;
    m.hwnd = hwnd;
    m.message = msg;
    m.wParam = wp;
    m.lParam = lp;
    m.time = static_cast<unsigned int>(SDL_GetTicks());
    int x = 0, y = 0;
    SDL_GetGlobalMouseState(&x, &y);
    LandscapePoint(g_mainWindow, &x, &y);
    m.pt_x = x;
    m.pt_y = y;
    std::lock_guard<std::mutex> g(g_qlock);
    g_queue.push_back(m);
}

// Removes a still-undelivered message; true if one was found.
bool Unpost(HWND__* hwnd, unsigned int msg, unsigned long long wp)
{
    std::lock_guard<std::mutex> g(g_qlock);
    for (auto it = g_queue.begin(); it != g_queue.end(); ++it) {
        if (it->hwnd == hwnd && it->message == msg && it->wParam == wp) {
            g_queue.erase(it);
            return true;
        }
    }
    return false;
}

// Converts a window size in points to the client's pixels, always landscape on iOS: on every
// return from the background UIKit lays the landscape-only view out again, as landscape and then
// once more as portrait, and that last size is the one SDL keeps.
void ClientPixels(const K2Window* w, int nPointsW, int nPointsH, int* pW, int* pH)
{
    const float fScale = w ? w->scale : 1.0f;
    *pW = static_cast<int>(nPointsW * fScale + 0.5f);
    *pH = static_cast<int>(nPointsH * fScale + 0.5f);
#ifdef K2P_IOS
    if (*pH > *pW) std::swap(*pW, *pH);
#endif
}

// Converts a pointer position from SDL to window points of the landscape view. iOS: SDL makes a
// touch's mouse position as the touch's position in the view (0..1, against the view's real
// landscape bounds) times the window size it last heard of, which after a return from the
// background is that portrait layout, so x is squeezed and y stretched; undo it.
void LandscapePoint(const K2Window* w, int* pX, int* pY)
{
#ifdef K2P_IOS
    int ww = 0, wh = 0;
    if (w && w->sdl) SDL_GetWindowSize(w->sdl, &ww, &wh);
    if (ww > 0 && wh > ww) {
        *pX = static_cast<int>(static_cast<long long>(*pX) * wh / ww);
        *pY = static_cast<int>(static_cast<long long>(*pY) * ww / wh);
    }
#else
    (void)w; (void)pX; (void)pY;
#endif
}

long long PackXY(int x, int y) { return static_cast<long long>((static_cast<uint32_t>(static_cast<uint16_t>(y)) << 16) | static_cast<uint16_t>(x)); }
// Converts window points to the engine's client pixels. A drawable shorter than the engine's mode
// (a fullscreen window that came out below its expected size) shows the bottom of the frame, where
// GL's origin is, so the rows it cuts off are at the top and the pointer is that much further down.
long long PackClientXY(const K2Window* w, int x, int y)
{
    if (!w) return PackXY(x, y);
    LandscapePoint(w, &x, &y);
    x = static_cast<int>(x * w->scale);
    y = static_cast<int>(y * w->scale);
    int nDrawW = 0, nDrawH = 0;
    SDL_GL_GetDrawableSize(w->sdl, &nDrawW, &nDrawH);
    if (w->modeH > 0 && nDrawH > 0 && nDrawH < w->modeH) y += w->modeH - nDrawH;
    return PackXY(x, y);
}

// K2_INPUT's synthetic left button, kept here because pushed SDL events do not move SDL's own
// button state.
Uint32 g_synthButtons;

// Whether the last left-button press or release came from a touch screen (SDL's touch-to-mouse
// events, SDL_TOUCH_MOUSEID) rather than a mouse or trackpad.
bool g_bTouchClick;

Uint32 MouseButtons() { return SDL_GetMouseState(0, 0) | g_synthButtons; }

unsigned long long MouseKeys()
{
    Uint32 b = MouseButtons();
    const Uint8* k = SDL_GetKeyboardState(0);
    unsigned long long mk = 0;
    if (b & SDL_BUTTON_LMASK) mk |= 0x01;
    if (b & SDL_BUTTON_RMASK) mk |= 0x02;
    if (k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_RSHIFT]) mk |= 0x04;
    if (k[SDL_SCANCODE_LCTRL] || k[SDL_SCANCODE_RCTRL]) mk |= 0x08;
    if (b & SDL_BUTTON_MMASK) mk |= 0x10;
    return mk;
}

// Maps an SDL keycode to a Win32 virtual-key code.
unsigned int VkFromSdl(SDL_Keycode k)
{
    if (k >= 'a' && k <= 'z') return static_cast<unsigned int>(k - 'a' + 'A');
    if (k >= '0' && k <= '9') return static_cast<unsigned int>(k);
    if (k >= SDLK_F1 && k <= SDLK_F12) return 0x70 + static_cast<unsigned int>(k - SDLK_F1);
    if (k >= SDLK_KP_1 && k <= SDLK_KP_9) return 0x61 + static_cast<unsigned int>(k - SDLK_KP_1);
    switch (k) {
    case SDLK_BACKSPACE: return 0x08; case SDLK_TAB: return 0x09; case SDLK_RETURN: return 0x0d;
    case SDLK_KP_ENTER: return 0x0d; case SDLK_LSHIFT: case SDLK_RSHIFT: return 0x10;
    case SDLK_LCTRL: case SDLK_RCTRL: return 0x11; case SDLK_LALT: case SDLK_RALT: return 0x12;
    case SDLK_PAUSE: return 0x13; case SDLK_CAPSLOCK: return 0x14; case SDLK_ESCAPE: return 0x1b;
    case SDLK_SPACE: return 0x20; case SDLK_PAGEUP: return 0x21; case SDLK_PAGEDOWN: return 0x22;
    case SDLK_END: return 0x23; case SDLK_HOME: return 0x24; case SDLK_LEFT: return 0x25;
    case SDLK_UP: return 0x26; case SDLK_RIGHT: return 0x27; case SDLK_DOWN: return 0x28;
    case SDLK_PRINTSCREEN: return 0x2c; case SDLK_INSERT: return 0x2d; case SDLK_DELETE: return 0x2e;
    case SDLK_KP_0: return 0x60; case SDLK_KP_MULTIPLY: return 0x6a; case SDLK_KP_PLUS: return 0x6b;
    case SDLK_KP_MINUS: return 0x6d; case SDLK_KP_PERIOD: return 0x6e; case SDLK_KP_DIVIDE: return 0x6f;
    case SDLK_NUMLOCKCLEAR: return 0x90; case SDLK_SCROLLLOCK: return 0x91;
    case SDLK_SEMICOLON: return 0xba; case SDLK_EQUALS: return 0xbb; case SDLK_COMMA: return 0xbc;
    case SDLK_MINUS: return 0xbd; case SDLK_PERIOD: return 0xbe; case SDLK_SLASH: return 0xbf;
    case SDLK_BACKQUOTE: return 0xc0; case SDLK_LEFTBRACKET: return 0xdb; case SDLK_BACKSLASH: return 0xdc;
    case SDLK_RIGHTBRACKET: return 0xdd; case SDLK_QUOTE: return 0xde;
    default: return 0;
    }
}

// Returns the WM_CHAR a key press makes when SDL text input is off (no edit box has focus), as
// TranslateMessage would without an IME. The typed cheat console reads only WM_CHAR
// (HandleWMCharMessage takes the '`' that opens it and the line); CR and Esc reach it from
// WM_KEYDOWN instead (HandleWMKeyDownMessage). Printable US-layout ASCII, else 0.
unsigned int CharFromSdl(const SDL_Keysym& ks)
{
    if (ks.mod & (KMOD_CTRL | KMOD_ALT | KMOD_GUI)) return 0;
    SDL_Keycode k = ks.sym;
    if (k < 0x20 || k > 0x7e) return 0;
    bool bShift = (ks.mod & KMOD_SHIFT) != 0;
    if (k >= 'a' && k <= 'z') return static_cast<unsigned int>(bShift != ((ks.mod & KMOD_CAPS) != 0) ? k - 'a' + 'A' : k);
    if (!bShift) return static_cast<unsigned int>(k);
    static const char kFrom[] = "`1234567890-=[]\\;',./";
    static const char kTo[] = "~!@#$%^&*()_+{}|:\"<>?";
    const char* p = strchr(kFrom, static_cast<int>(k));
    return p ? static_cast<unsigned int>(static_cast<unsigned char>(kTo[p - kFrom])) : static_cast<unsigned int>(k);
}

SDL_Scancode ScancodeFromVk(int vk)
{
    for (int s = 0; s < SDL_NUM_SCANCODES; ++s)
        if (VkFromSdl(SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(s))) == static_cast<unsigned int>(vk)) return static_cast<SDL_Scancode>(s);
    return SDL_SCANCODE_UNKNOWN;
}

// the engine's SDL event hook (K2_MOUSE_TOUCH).
//
// ASL::SDL::ProcessEvents hands every SDL event to the app's ASL::AppHooks::PreProcessEvent before
// its own translation and drops the event when the hook returns false. That hook is where touches
// become the virtual stick (s_mapTrackedTouches / ios_left_right / s_fIosTouchLR, read back through
// IosGetRelativeTouch / GetIosControl) and where minigame taps hit the on-screen buttons. The host
// runs it the same way, on by default in the touch UI build (K2P_UI=touch) and off otherwise, with
// K2_MOUSE_TOUCH=1/0 forcing either way, routing:
//   * SDL_FINGER*, from a real direct (touchscreen) device plus ones synthesized from the left
//     mouse button: down, drag and up as finger down, motion and up (finger id 1, normalized to
//     the window, the way SDL reports a touch);
//   * SDL_KEYUP, for the AC_BACK branch.
// Window events are not routed: the hook's FOCUS_LOST blocks the thread until focus returns and its
// ENTER makes a JNI call, while the shim maps focus to WM_ACTIVATE itself. Mouse buttons are not
// routed either: the hook tests SDL_MOUSEBUTTONDOWN against display-sized minigame buttons, but a
// windowed host's mouse coordinates are window pixels, and the synthesized finger-down covers taps.
// Pad hot-plug is not routed. Trackpad (indirect) touches are ignored: they would steer the stick
// while the cursor moves. With the switch off nothing reaches the hook and the mouse and keyboard
// path is the plain one; with it on the mouse drives that path too, as Android's touch-to-mouse
// events do.
} // namespace
namespace ASL {
// The vtable layout of MacMain.cpp's ASL::AppHooks, declared here only to make the virtual call:
// the shim includes SDL.h, so it cannot include that TU's own SDL_Event. Six slots, no destructor.
class AppHooks {
public:
    virtual bool BeforeWinMain(std::string& sCommandLine) = 0;
    virtual void AfterWinMain() = 0;
    virtual int  BeforeExit(int nExitCode, bool bFlag) = 0;
    virtual void BeforeParentChildLauncher() = 0;
    virtual bool PreProcessEvent(SDL_Event& event) = 0;
    virtual void BeforePumpEvents() = 0;
};
}
ASL::AppHooks* k2host_GetAppHooks();   // MacMain.cpp (K2_HOST_BUILD): SDL_main's sHooks
namespace {

// On by default in the touch UI build (K2P_UI=touch), off otherwise; K2_MOUSE_TOUCH=0/1 overrides.
#ifdef K2_UI_TOUCH
const bool kMouseTouchDefault = true;
#else
const bool kMouseTouchDefault = false;
#endif
const bool g_bMouseTouch = [] {
    const char* v = getenv("K2_MOUSE_TOUCH");
    return v ? (*v && strcmp(v, "0") != 0) : kMouseTouchDefault;
}();
bool g_bMouseFingerDown;               // the left button is a synthesized finger

// Runs one event through the hook; returns false when it was consumed.
bool RunAppHook(SDL_Event& e)
{
    ASL::AppHooks* pHooks = k2host_GetAppHooks();
    return !pHooks || pHooks->PreProcessEvent(e);
}

// Returns the finger event a left-mouse event stands for; false when it stands for none.
bool MouseFinger(const SDL_Event& m, SDL_Event& f)
{
    int mx, my;
    Uint32 type, windowID;
    if (m.type == SDL_MOUSEBUTTONDOWN || m.type == SDL_MOUSEBUTTONUP) {
        if (m.button.button != SDL_BUTTON_LEFT || m.button.which == SDL_TOUCH_MOUSEID) return false;
        bool down = m.type == SDL_MOUSEBUTTONDOWN;
        if (down == g_bMouseFingerDown) return false;
        g_bMouseFingerDown = down;
        type = down ? SDL_FINGERDOWN : SDL_FINGERUP;
        mx = m.button.x; my = m.button.y; windowID = m.button.windowID;
    } else if (m.type == SDL_MOUSEMOTION) {
        if (!g_bMouseFingerDown || m.motion.which == SDL_TOUCH_MOUSEID) return false;
        type = SDL_FINGERMOTION;
        mx = m.motion.x; my = m.motion.y; windowID = m.motion.windowID;
    } else {
        return false;
    }
    K2Window* w = g_bySdlId.count(windowID) ? g_bySdlId[windowID] : g_mainWindow;
    int ww = 0, wh = 0;
    if (w && w->sdl) SDL_GetWindowSize(w->sdl, &ww, &wh);
    if (ww <= 0 || wh <= 0) return false;
    float fx = static_cast<float>(mx) / static_cast<float>(ww), fy = static_cast<float>(my) / static_cast<float>(wh);
    fx = fx < 0.0f ? 0.0f : fx > 1.0f ? 1.0f : fx;
    fy = fy < 0.0f ? 0.0f : fy > 1.0f ? 1.0f : fy;
    static float s_fLastX, s_fLastY;
    memset(&f, 0, sizeof f);
    f.tfinger.type = type;
    f.tfinger.timestamp = m.common.timestamp;
    f.tfinger.touchId = SDL_MOUSE_TOUCHID;
    f.tfinger.fingerId = 1;
    f.tfinger.x = fx;
    f.tfinger.y = fy;
    f.tfinger.dx = type == SDL_FINGERMOTION ? fx - s_fLastX : 0.0f;
    f.tfinger.dy = type == SDL_FINGERMOTION ? fy - s_fLastY : 0.0f;
    f.tfinger.pressure = type == SDL_FINGERUP ? 0.0f : 1.0f;
    s_fLastX = fx;
    s_fLastY = fy;
    return true;
}

// Runs the hook over this event and over the finger a left-mouse event stands for; false when the
// hook consumed the event itself.
bool AppHookFilter(SDL_Event& e)
{
    SDL_Event f;
    if (MouseFinger(e, f)) RunAppHook(f);
    switch (e.type) {
    case SDL_FINGERDOWN: case SDL_FINGERUP: case SDL_FINGERMOTION:
        if (e.tfinger.touchId == SDL_MOUSE_TOUCHID
            || SDL_GetTouchDeviceType(e.tfinger.touchId) != SDL_TOUCH_DEVICE_DIRECT)
            return true;
        return RunAppHook(e);
    case SDL_KEYUP:
        return RunAppHook(e);
    default:
        return true;
    }
}

// Translates every pending SDL event into queued messages and DirectInput state.
void PumpSdl()
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (g_bMouseTouch && !AppHookFilter(e)) continue;
        k2shim_dinput_event(&e);
#ifdef K2_UI_XBOX
        // A pad press hides the pointer and parks it off screen (the engine flips y: a window y
        // past the bottom lands above the top), so nothing hovers where it last was.
        if (k2shim_pad_activity(&e) && k2shim_pointer_from_pad() && g_mainWindow)
            Post(reinterpret_cast<HWND__*>(g_mainWindow), WM_MOUSEMOVE, 0,
                 PackXY(K2SHIM_POINTER_PARKED, -K2SHIM_POINTER_PARKED));
#endif
        K2Window* w = 0;
        switch (e.type) {
        case SDL_WINDOWEVENT: w = g_bySdlId.count(e.window.windowID) ? g_bySdlId[e.window.windowID] : 0; break;
        case SDL_KEYDOWN: case SDL_KEYUP: w = g_bySdlId.count(e.key.windowID) ? g_bySdlId[e.key.windowID] : 0; break;
        case SDL_TEXTINPUT: w = g_bySdlId.count(e.text.windowID) ? g_bySdlId[e.text.windowID] : 0; break;
        case SDL_MOUSEMOTION: w = g_bySdlId.count(e.motion.windowID) ? g_bySdlId[e.motion.windowID] : 0; break;
        case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP:
            w = g_bySdlId.count(e.button.windowID) ? g_bySdlId[e.button.windowID] : 0; break;
        case SDL_MOUSEWHEEL: w = g_bySdlId.count(e.wheel.windowID) ? g_bySdlId[e.wheel.windowID] : 0; break;
        default: break;
        }
        if (!w) w = g_mainWindow;
        HWND__* h = reinterpret_cast<HWND__*>(w);

        switch (e.type) {
        case SDL_QUIT:
            if (w) Post(h, WM_CLOSE, 0, 0);
            else Post(0, WM_QUIT, 0, 0);
            break;
        case SDL_WINDOWEVENT:
            switch (e.window.event) {
            // A desktop focus change is only WM_ACTIVATE and SET/KILLFOCUS, which makes the engine
            // drop and retake its input. WM_ACTIVATEAPP is Android's app suspend and resume, first
            // on resume and last on suspend; its handler treats the always-fullscreen mobile
            // renderer as exclusive mode (minimize, release, ReInitAurora on return), so here it
            // follows the window's minimize and restore instead of keyboard focus.
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                Post(h, WM_ACTIVATE, 1, 0);
                Post(h, WM_SETFOCUS, 0, 0);
                break;
            case SDL_WINDOWEVENT_FOCUS_LOST:
                Post(h, WM_KILLFOCUS, 0, 0);
                Post(h, WM_ACTIVATE, 0, 0);
                break;
            case SDL_WINDOWEVENT_MINIMIZED:
                if (!g_appSuspended) {
                    g_appSuspended = true;
                    Post(h, WM_ACTIVATEAPP, 0, 0);
                }
                break;
            case SDL_WINDOWEVENT_RESTORED:
                // An Android pause blocks the pump right after MINIMIZED, so the suspend can still
                // be queued here; delivered late, its handler minimizes the window again, which on
                // Android sends the app straight back to the home screen. Drop the pair instead.
                if (g_appSuspended) {
                    g_appSuspended = false;
                    if (!Unpost(h, WM_ACTIVATEAPP, 0))
                        Post(h, WM_ACTIVATEAPP, 1, 0);
                }
                break;
            case SDL_WINDOWEVENT_SIZE_CHANGED: {
                // client pixels, as GetClientRect reports them: the engine reads WM_SIZE as its
                // screen size and reshapes, hence the touch mapping
                int cw, ch;
                ClientPixels(w, e.window.data1, e.window.data2, &cw, &ch);
                Post(h, WM_SIZE, 0, PackXY(cw, ch));
                break;
            }
            case SDL_WINDOWEVENT_EXPOSED: Post(h, WM_PAINT, 0, 0); break;
            case SDL_WINDOWEVENT_CLOSE: Post(h, WM_CLOSE, 0, 0); break;
            default: break;
            }
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            unsigned int vk = VkFromSdl(e.key.keysym.sym);
            if (!vk) break;
            bool alt = (e.key.keysym.mod & KMOD_ALT) != 0;
            unsigned int msg = e.type == SDL_KEYDOWN ? (alt ? WM_SYSKEYDOWN : WM_KEYDOWN)
                                                     : (alt ? WM_SYSKEYUP : WM_KEYUP);
            // lParam: repeat count 1, the scan code (DIK, set 1) in bits 16-23, ALT in 29, the
            // previous state in 30, the transition in 31; 32-bit, sign-extended.
            uint32_t lp = 1u | (static_cast<uint32_t>(SDL_GetScancodeFromKey(e.key.keysym.sym)) & 0xff) << 16;
            if (alt) lp |= 1u << 29;
            if (e.type == SDL_KEYUP) lp |= 0xc0000000u;
            else if (e.key.repeat) lp |= 1u << 30;
            Post(h, msg, vk, static_cast<long long>(static_cast<int32_t>(lp)));
            if (msg == WM_KEYDOWN && !SDL_IsTextInputActive())
                if (unsigned int ch = CharFromSdl(e.key.keysym)) Post(h, WM_CHAR, ch, static_cast<long long>(static_cast<int32_t>(lp)));
            break;
        }
        case SDL_TEXTINPUT:
            // Latin-1 only, since the engine's text is 8-bit.
            for (const unsigned char* p = reinterpret_cast<const unsigned char*>(e.text.text); *p;) {
                unsigned int c = *p++;
                if (c >= 0xc0 && c < 0xe0 && (*p & 0xc0) == 0x80) c = ((c & 0x1f) << 6) | (*p++ & 0x3f);
                else if (c >= 0x80) { while ((*p & 0xc0) == 0x80) ++p; continue; }
                if (c < 0x100) Post(h, WM_CHAR, c, 1);
            }
            break;
        case SDL_MOUSEMOTION:
#ifndef K2_UI_TOUCH
            // A right-drag look (MSG_Mac::CaptureMouse: relative mode) moves the camera, not the
            // pointer, so its motion reaches the engine only as the DirectInput mouse delta.
            if (SDL_GetRelativeMouseMode()) break;
#endif
#ifdef K2_UI_XBOX
            k2shim_pointer_from_mouse();
#endif
            Post(h, WM_MOUSEMOVE, MouseKeys(), PackClientXY(w, e.motion.x, e.motion.y));
            Post(h, WM_SETCURSOR, reinterpret_cast<unsigned long long>(h), 1);   // HTCLIENT: the engine sets its cursor
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            bool down = e.type == SDL_MOUSEBUTTONDOWN;
            unsigned int msg = 0;
            unsigned long long xb = 0;
            if (e.button.button == SDL_BUTTON_LEFT)
                msg = down ? (e.button.clicks == 2 ? WM_LBUTTONDBLCLK : WM_LBUTTONDOWN) : WM_LBUTTONUP;
            else if (e.button.button == SDL_BUTTON_RIGHT) msg = down ? WM_RBUTTONDOWN : WM_RBUTTONUP;
            else if (e.button.button == SDL_BUTTON_MIDDLE) msg = down ? WM_MBUTTONDOWN : WM_MBUTTONUP;
            else if (e.button.button == SDL_BUTTON_X1 || e.button.button == SDL_BUTTON_X2) {
                msg = down ? WM_XBUTTONDOWN : WM_XBUTTONUP;
                xb = e.button.button == SDL_BUTTON_X1 ? 0x10000 : 0x20000;
            }
            if (e.button.button == SDL_BUTTON_LEFT) g_bTouchClick = e.button.which == SDL_TOUCH_MOUSEID;
#ifdef K2_UI_XBOX
            k2shim_pointer_from_mouse();
#endif
            if (msg) Post(h, msg, MouseKeys() | xb, PackClientXY(w, e.button.x, e.button.y));
            break;
        }
        case SDL_MOUSEWHEEL: {   // lParam is in screen coordinates (the engine converts with ScreenToClient)
            POINT pt;   // GetCursorPos: the pointer, or K2_INPUT's synthetic one
            GetCursorPos(&pt);
            if (e.wheel.y)
                Post(h, WM_MOUSEWHEEL, MouseKeys() | (static_cast<unsigned long long>(static_cast<uint16_t>(e.wheel.y * 120)) << 16),
                     PackXY(pt.x, pt.y));
            break;
        }
        default:
            break;
        }
    }
}

bool Matches(const MSG& m, HWND__* hwnd, unsigned int nMin, unsigned int nMax)
{
    if (hwnd && m.hwnd != hwnd && m.message != WM_QUIT) return false;
    if (nMin == 0 && nMax == 0) return true;
    return m.message >= nMin && m.message <= nMax;
}

// GL contexts.

struct K2Glrc { SDL_GLContext ctx; bool bConfigured = false; };
thread_local K2Dc* t_currentDc;
thread_local K2Glrc* t_currentRc;

} // namespace

extern "C" {

int DestroyWindow(HWND__* hWnd);

// messages.

int PeekMessageA(MSG* pMsg, HWND__* hWnd, unsigned int nMin, unsigned int nMax, unsigned int wRemove)
{
    PumpSdl();
    std::lock_guard<std::mutex> g(g_qlock);
    for (auto it = g_queue.begin(); it != g_queue.end(); ++it) {
        if (!Matches(*it, hWnd, nMin, nMax)) continue;
        if (pMsg) *pMsg = *it;
        if (wRemove & 1) g_queue.erase(it);
        return 1;
    }
    return 0;
}

// GetMessageA is a PeekMessageA(PM_REMOVE) loop that sleeps on SDL between polls.
int GetMessageA(MSG* pMsg, HWND__* hWnd, unsigned int nMin, unsigned int nMax)
{
    while (!PeekMessageA(pMsg, hWnd, nMin, nMax, 1))
        SDL_WaitEventTimeout(0, 10);
    return pMsg->message != WM_QUIT;
}

int64_t DispatchMessageA(const MSG* pMsg)
{
    K2Window* w = pMsg ? W(pMsg->hwnd) : 0;
    if (!w || !w->proc) return 0;
    return w->proc(pMsg->hwnd, pMsg->message, pMsg->wParam, static_cast<int64_t>(pMsg->lParam));
}

// Key messages report as translated (return 1), but characters come from SDL text input, so only
// VK_BACK gets a synthesized WM_CHAR.
int TranslateMessage(const MSG* pMsg)
{
    unsigned int m = pMsg->message;
    if (m == WM_KEYDOWN && pMsg->wParam == 8) Post(pMsg->hwnd, WM_CHAR, 8, pMsg->lParam);
    return m == WM_KEYDOWN || m == WM_KEYUP || m == WM_SYSKEYDOWN || m == WM_SYSKEYUP;
}

int PostMessageA(HWND__* hWnd, unsigned int nMsg, unsigned long long wParam, int64_t lParam)
{
    Post(hWnd, nMsg, wParam, lParam);
    return 1;
}

int64_t SendMessageA(HWND__* hWnd, unsigned int nMsg, unsigned long long wParam, int64_t lParam)
{
    K2Window* w = W(hWnd);
    return (w && w->proc) ? w->proc(hWnd, nMsg, wParam, lParam) : 0;
}

void PostQuitMessage(int nExitCode) { Post(0, WM_QUIT, static_cast<unsigned long long>(nExitCode), 0); }

int64_t DefWindowProcA(HWND__* hWnd, unsigned int nMsg, unsigned long long, int64_t)
{
    if (nMsg == WM_CLOSE) DestroyWindow(hWnd);
    return 0;
}

int64_t CallWindowProcA(WndProcFn pfn, HWND__* hWnd, unsigned int nMsg, unsigned long long wParam, int64_t lParam)
{
    return pfn ? pfn(hWnd, nMsg, wParam, lParam) : 0;
}
int64_t CallWindowProcW(WndProcFn pfn, HWND__* hWnd, unsigned int nMsg, unsigned long long wParam, int64_t lParam)
{
    return CallWindowProcA(pfn, hWnd, nMsg, wParam, lParam);
}

} // extern "C"

// windows.

K2_WIN32_API_BEGIN
void* GetModuleHandleA(const char*) { return reinterpret_cast<void*>(0x400000); }
K2_WIN32_API_END

extern "C" {

void* LoadIconA(void*, uint64_t) { return reinterpret_cast<void*>(1); }
// Cursors are tokens carrying the id; the pc UI shows the PC art (shim_cursor.cpp).
void* LoadCursorA(void*, uint64_t nName) { return k2shim_cursor_load(nName); }
int DestroyCursor(void*) { return 1; }
void* GetStockObject(int) { return reinterpret_cast<void*>(1); }
void* SetCursor_Win32(void* h) { return k2shim_cursor_select(h); }

int ShowCursor_Win32(int bShow)
{
    static int s_count = 0;
    s_count += bShow ? 1 : -1;
#ifndef K2_UI_TOUCH
    k2shim_cursor_show(s_count >= 0);   // hidden unless the engine is in hardware-cursor mode
#else
    SDL_ShowCursor(s_count >= 0 ? SDL_ENABLE : SDL_DISABLE);
#endif
    return s_count;
}

unsigned short RegisterClassA(const WNDCLASSA* pwc)
{
    if (!pwc || !pwc->lpszClassName) { SetLastError(K2_ERROR_INVALID_PARAMETER); return 0; }
    g_classes[Lower(pwc->lpszClassName)] = pwc->lpfnWndProc;
    return static_cast<unsigned short>(g_classes.size());
}

HWND__* CreateWindowExA(unsigned int dwExStyle, const char* pszClass, const char* pszName,
                        unsigned int dwStyle, int x, int y, int nWidth, int nHeight,
                        HWND__* hParent, void* hMenu, void* hInstance, void* lpParam)
{
    auto it = g_classes.find(Lower(pszClass));
    if (it == g_classes.end()) { SetLastError(K2_ERROR_INVALID_PARAMETER); return 0; }
    // On CW_USEDEFAULT the stock build sizes to the display, while a desktop window is friendlier.
    // K2_WINDOW=WxH.
    if (nWidth <= 0 || nWidth == static_cast<int>(0x80000000) || nHeight <= 0 || nHeight == static_cast<int>(0x80000000)) {
        nWidth = 1280;
        nHeight = 720;
        if (const char* e = getenv("K2_WINDOW")) sscanf(e, "%dx%d", &nWidth, &nHeight);
    }
    // No SDL_WINDOW_ALLOW_HIGHDPI, because the engine assumes its window size is its framebuffer
    // size.
    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
#ifdef K2P_MOBILE_HOST
    // On Android and iOS the window is the activity's or the screen's surface (SDL sizes it), and
    // fullscreen hides the system bars as the stock build's immersive mode does.
    flags |= SDL_WINDOW_FULLSCREEN;
#ifdef K2P_IOS
    // iOS: the drawable is the screen's pixels, not its points (k2shim_pixel_scalef).
    if (k2shim_pixel_scalef() > 1.0f) flags |= SDL_WINDOW_ALLOW_HIGHDPI;
    // One window, as on Android, whose SDL refuses a second: the movies' black topmost "Knights
    // Cinematic Backdrop" never exists there, while on iOS it would stay up as a second UIWindow
    // over the game.
    if (g_mainWindow) {
        SetLastError(K2_ERROR_INVALID_PARAMETER);
        return 0;
    }
#endif
#endif
    // WS_VISIBLE; K2_HIDDEN=1 keeps every window hidden (unattended runs, K2_SHOT still works).
#ifdef K2P_IOS
    // iOS: the window is the screen. Android ignores SDL_WINDOW_HIDDEN, but a hidden UIWindow
    // stays hidden, and the engine makes its renderer window without WS_VISIBLE.
    (void)dwStyle;
#else
    if (!(dwStyle & 0x10000000) || getenv("K2_HIDDEN")) flags |= SDL_WINDOW_HIDDEN;
#endif
    SDL_Window* sdl = SDL_CreateWindow(pszName ? pszName : "", SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED, nWidth, nHeight, flags);
    if (!sdl) {
        fprintf(stderr, "k2shim: SDL_CreateWindow: %s\n", SDL_GetError());
        return 0;
    }
    k2shim_keep_composited(sdl);
    k2shim_window_icon(sdl);
#ifndef K2_UI_TOUCH
    k2shim_cursor_refresh();   // pc/xbox: the hardware cursor, or none under the software one
#endif
    // Desktop SDL2 starts text input at SDL_Init, and on macOS that makes every key press an IME
    // event: the input-source and press-and-hold bubbles pop up at the unset text rect, the
    // window's corner. The engine starts text input itself for an edit box
    // (ASLPlat_ShowVirtualKeyboard) and stops it when the box loses focus, as on Android, where it
    // starts off.
    if (!g_mainWindow) SDL_StopTextInput();
    K2Window* w = new K2Window;
    w->magic = kWndMagic;
    w->sdl = sdl;
    w->sdlFlags = flags;
#ifdef K2P_IOS
    w->scale = k2shim_pixel_scalef();
#else
    w->scale = 1;
#endif
    w->modeH = 0;
    w->proc = it->second;
    w->cls = pszClass;
    w->userData = 0;
    w->dc.magic = kDcMagic;
    w->dc.wnd = w;
    w->dc.pixelFormat = 0;
    g_bySdlId[SDL_GetWindowID(sdl)] = w;
    g_liveHandles.insert(w);
    g_liveHandles.insert(&w->dc);
    if (!g_mainWindow) g_mainWindow = w;

    CreateStructA cs = { lpParam, hInstance, hMenu, hParent, nHeight, nWidth, y, x, static_cast<int64_t>(dwStyle),
                         pszName, pszClass, dwExStyle };
    if (w->proc) w->proc(reinterpret_cast<HWND__*>(w), WM_CREATE, 0, static_cast<int64_t>(reinterpret_cast<intptr_t>(&cs)));
    return reinterpret_cast<HWND__*>(w);
}

int DestroyWindow(HWND__* hWnd)
{
    K2Window* w = W(hWnd);
    if (!w) return 0;
    if (w->proc) w->proc(hWnd, WM_DESTROY, 0, 0);
    g_bySdlId.erase(SDL_GetWindowID(w->sdl));
    if (t_currentDc == &w->dc) SDL_GL_MakeCurrent(0, 0), t_currentDc = 0;
    SDL_DestroyWindow(w->sdl);
    if (g_mainWindow == w) g_mainWindow = 0;
    {
        std::lock_guard<std::mutex> g(g_qlock);
        for (auto it = g_queue.begin(); it != g_queue.end();)
            it = it->hwnd == hWnd ? g_queue.erase(it) : it + 1;
    }
    g_liveHandles.erase(w);
    g_liveHandles.erase(&w->dc);
    w->magic = 0;
    delete w;
    return 1;
}

int ShowWindow_Win32(HWND__* hWnd, int nCmdShow)
{
    K2Window* w = W(hWnd);
    if (!w) return 0;
    bool was = (SDL_GetWindowFlags(w->sdl) & SDL_WINDOW_SHOWN) != 0;
#ifdef K2P_IOS
    // iOS: the window is the screen and is always shown (as on Android, where these do nothing).
    return was;
#endif
    switch (nCmdShow) {
    case 0: SDL_HideWindow(w->sdl); break;                                   // SW_HIDE
    case 2: case 6: case 7: SDL_MinimizeWindow(w->sdl); break;               // SW_*MINIMIZE*/MINNOACTIVE
    case 3: if (!getenv("K2_HIDDEN")) SDL_MaximizeWindow(w->sdl); break;     // SW_MAXIMIZE
    case 9: SDL_RestoreWindow(w->sdl); break;                                // SW_RESTORE
    case 4: case 8:                                                          // SW_SHOWNOACTIVATE/SHOWNA
        if (!getenv("K2_HIDDEN")) SDL_ShowWindow(w->sdl);
        break;
    default:                                                                 // SW_SHOW* that activate
        if (!getenv("K2_HIDDEN")) {
            SDL_ShowWindow(w->sdl);
            SDL_RaiseWindow(w->sdl);
        }
        break;
    }
    return was;
}

int SetWindowPos(HWND__* hWnd, HWND__*, int x, int y, int cx, int cy, unsigned int uFlags)
{
    K2Window* w = W(hWnd);
    if (!w) return 0;
#ifdef K2P_IOS
    return 1;   // the window is the screen (ShowWindow_Win32)
#endif
    if (!(uFlags & 0x1) && cx > 0 && cy > 0) SDL_SetWindowSize(w->sdl, cx, cy);   // SWP_NOSIZE
    if (!(uFlags & 0x2)) SDL_SetWindowPosition(w->sdl, x, y);                    // SWP_NOMOVE
    if (uFlags & 0x40) SDL_ShowWindow(w->sdl);                                   // SWP_SHOWWINDOW
    if (uFlags & 0x80) SDL_HideWindow(w->sdl);                                   // SWP_HIDEWINDOW
    return 1;
}

int SetWindowTextA(HWND__* hWnd, const char* psz)
{
    K2Window* w = W(hWnd);
    if (w) SDL_SetWindowTitle(w->sdl, psz ? psz : "");
    return w != 0;
}

// Client area = the whole SDL window; there is no non-client frame to subtract.
int AdjustWindowRect(RECT*, unsigned int, int) { return 1; }

int GetClientRect(HWND__* hWnd, RECT* r)
{
    K2Window* w = W(hWnd);
    if (!w || !r) return 0;
    int cw, ch;
    SDL_GetWindowSize(w->sdl, &cw, &ch);
    ClientPixels(w, cw, ch, &cw, &ch);
    r->left = r->top = 0;
    r->right = cw;
    r->bottom = ch;
    return 1;
}

// WINDOWINFO: cbSize, rcWindow, rcClient, dwStyle, dwExStyle, dwWindowStatus, cxWindowBorders,
// cyWindowBorders, atomWindowType, wCreatorVersion (60 bytes).
int GetWindowInfo(HWND__* hWnd, WINDOWINFO* pInfo)
{
    K2Window* w = W(hWnd);
    if (!w || !pInfo) return 0;
    int x, y, cw, ch;
    SDL_GetWindowPosition(w->sdl, &x, &y);
    SDL_GetWindowSize(w->sdl, &cw, &ch);
    unsigned int cbSize = pInfo->cbSize;
    memset(pInfo, 0, sizeof(*pInfo));
    pInfo->cbSize = cbSize;
    RECT win = { x, y, x + cw, y + ch };
    pInfo->rcWindow = win;
    pInfo->rcClient = win;
    return 1;
}

int ScreenToClient(HWND__* hWnd, POINT* pt)
{
    K2Window* w = W(hWnd);
    if (!w || !pt) return 0;
    int x, y;
    SDL_GetWindowPosition(w->sdl, &x, &y);
    pt->x = static_cast<int>((pt->x - x) * w->scale);
    pt->y = static_cast<int>((pt->y - y) * w->scale);
    return 1;
}

int ClientToScreen(HWND__* hWnd, POINT* pt)
{
    K2Window* w = W(hWnd);
    if (!w || !pt) return 0;
    int x, y;
    SDL_GetWindowPosition(w->sdl, &x, &y);
    pt->x = static_cast<int>(pt->x / w->scale) + x;
    pt->y = static_cast<int>(pt->y / w->scale) + y;
    return 1;
}

int PtInRect_Win32(const RECT* r, POINT pt)
{
    return r && pt.x >= r->left && pt.x < r->right && pt.y >= r->top && pt.y < r->bottom;
}

int GetCursorPos(POINT* pt)
{
    if (!pt) return 0;
    if (g_synthCursor && g_mainWindow) {   // K2_INPUT drives the cursor: client -> screen
        int x, y;
        SDL_GetWindowPosition(g_mainWindow->sdl, &x, &y);
        pt->x = x + g_synthX;
        pt->y = y + g_synthY;
        return 1;
    }
    SDL_GetGlobalMouseState(&pt->x, &pt->y);
    LandscapePoint(g_mainWindow, &pt->x, &pt->y);
    return 1;
}

int SetCursorPos(int x, int y)
{
    SDL_WarpMouseGlobal(x, y);
    return 1;
}

// Whether the left button the engine is handling now was a tap on a touch screen (the xbox UI's
// CClientExoAppInternal::HandleInputEvent: a lifted finger hovers nothing).
extern "C" int k2shim_touch_click(void) { return g_bTouchClick; }

HWND__* SetCapture(HWND__* hWnd)
{
    if (!W(hWnd)) return 0;   // no window of ours (KOTOR 1's is SDL's): nothing would release it
    SDL_CaptureMouse(SDL_TRUE);
    return hWnd;
}
int ReleaseCapture()
{
    SDL_CaptureMouse(SDL_FALSE);
    return 1;
}

// In GetKeyState / GetAsyncKeyState the high bit is "down". VK_LBUTTON/RBUTTON/MBUTTON are the
// mouse.
short GetAsyncKeyState(int vk)
{
    Uint32 b = MouseButtons();
    if (vk == 1) return (b & SDL_BUTTON_LMASK) ? static_cast<short>(0x8000) : 0;
    if (vk == 2) return (b & SDL_BUTTON_RMASK) ? static_cast<short>(0x8000) : 0;
    if (vk == 4) return (b & SDL_BUTTON_MMASK) ? static_cast<short>(0x8000) : 0;
    const Uint8* k = SDL_GetKeyboardState(0);
    if (vk == 0x10) return (k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_RSHIFT]) ? static_cast<short>(0x8000) : 0;
    if (vk == 0x11) return (k[SDL_SCANCODE_LCTRL] || k[SDL_SCANCODE_RCTRL]) ? static_cast<short>(0x8000) : 0;
    if (vk == 0x12) return (k[SDL_SCANCODE_LALT] || k[SDL_SCANCODE_RALT]) ? static_cast<short>(0x8000) : 0;
    SDL_Scancode s = ScancodeFromVk(vk);
    return (s != SDL_SCANCODE_UNKNOWN && k[s]) ? static_cast<short>(0x8000) : 0;
}
int GetKeyState(int vk)   // the engine declares int and tests bit 15
{
    if (vk == 0x14) return (SDL_GetModState() & KMOD_CAPS) ? 1 : 0;   // VK_CAPITAL toggle
    return GetAsyncKeyState(vk) ? 0x8000 : 0;
}

int64_t GetWindowLongA(HWND__* hWnd, int nIndex) throw()
{
    K2Window* w = W(hWnd);
    if (!w) return 0;
    if (nIndex == -4) return static_cast<int64_t>(reinterpret_cast<intptr_t>(w->proc));   // GWL_WNDPROC
    if (nIndex == -21) return w->userData;    // GWL_USERDATA
    return 0;
}
int64_t GetWindowLongW(HWND__* hWnd, int nIndex) { return GetWindowLongA(hWnd, nIndex); }

// The engine only sets GWL_STYLE (-16), with a 32-bit value; styles have no SDL meaning here.
int SetWindowLongA(HWND__* hWnd, int, unsigned int) { return W(hWnd) ? 1 : 0; }

void* BeginPaint(HWND__* hWnd, void* pPaint)
{
    K2Window* w = W(hWnd);
    if (pPaint) memset(pPaint, 0, 72);   // PAINTSTRUCT
    return w ? &w->dc : 0;
}
int EndPaint(HWND__*, const void*) { return 1; }
int PatBlt(void*, int, int, int, int, unsigned int) { return 1; }

int MessageBoxA(HWND__*, const char* pszText, const char* pszCaption, unsigned int)
{
    fprintf(stderr, "k2: MessageBox [%s] %s\n", pszCaption ? pszCaption : "", pszText ? pszText : "");
    return 1;   // IDOK
}

// SPI_GETSCREENSAVEACTIVE and SPI_GETMOUSETRAILS report "off"; the setters succeed.
int SystemParametersInfoA(unsigned int uiAction, unsigned int, void* pvParam, unsigned int)
{
    if (pvParam && (uiAction == 0x10 || uiAction == 0x5e)) *static_cast<int*>(pvParam) = 0;
    return 1;
}

// device contexts, pixel formats, WGL.

HDC__* GetDC(HWND__* hWnd)
{
    if (!hWnd) return reinterpret_cast<HDC__*>(&g_screenDc);
    K2Window* w = W(hWnd);
    return w ? reinterpret_cast<HDC__*>(&w->dc) : 0;
}
int ReleaseDC(HWND__*, void*) { return 1; }

int ChoosePixelFormat(void* hDC, const PIXELFORMATDESCRIPTOR*) { return Dc(hDC) ? 1 : 0; }

int SetPixelFormat(void* hDC, int iPixelFormat, const PIXELFORMATDESCRIPTOR*)
{
    K2Dc* d = Dc(hDC);
    if (!d) return 0;
    d->pixelFormat = iPixelFormat;
    return 1;
}

int GetPixelFormat(void* hDC)
{
    K2Dc* d = Dc(hDC);
    return d ? d->pixelFormat : 0;
}

// Describes the one format, 32-bit RGBA, 24-bit depth, 8-bit stencil, double-buffered (what main()
// asks SDL for).
int DescribePixelFormat(void* hDC, int iPixelFormat, unsigned int nBytes, PIXELFORMATDESCRIPTOR* pfd)
{
    if (!Dc(hDC)) return 0;
    if (pfd && nBytes >= sizeof *pfd && iPixelFormat == 1) {
        memset(pfd, 0, sizeof *pfd);
        pfd->nSize = sizeof *pfd;
        pfd->nVersion = 1;
        pfd->dwFlags = 0x4 | 0x20 | 0x1;   // DRAW_TO_WINDOW | SUPPORT_OPENGL | DOUBLEBUFFER
        pfd->cColorBits = 32;
        pfd->cRedBits = pfd->cGreenBits = pfd->cBlueBits = pfd->cAlphaBits = 8;
        pfd->cRedShift = 16;
        pfd->cGreenShift = 8;
        pfd->cAlphaShift = 24;
        pfd->cDepthBits = 24;
        pfd->cStencilBits = 8;
    }
    return 1;   // the number of formats
}

int wglChoosePixelFormatARB(void* hDC, const int*, const float*, unsigned int nMax, int* piFormats,
                            unsigned int* pnNumFormats)
{
    if (!Dc(hDC)) return 0;
    if (nMax && piFormats) piFormats[0] = 1;
    if (pnNumFormats) *pnNumFormats = 1;
    return 1;
}

// Retired contexts. ReInitAurora (alt-tab back into fullscreen, a resolution change) deletes the
// context and the window and builds both again, but the engine's GL objects outlive it: ASLgl's
// KotorProgram cache keeps its GLSL programs across the re-init, as an EGL context keeps them
// across a surface change. A deleted context is parked here, and the next wglCreateContext hands it
// back, current on the new window with every object still alive.
static std::vector<SDL_GLContext> s_retiredContexts;

void* wglCreateContext(void* hDC)
{
    K2Dc* d = Dc(hDC);
    if (!d || !d->wnd) d = t_currentDc;                       // aurglcore passes no DC at all
    if ((!d || !d->wnd) && g_mainWindow) d = &g_mainWindow->dc;
    if (!d || !d->wnd) return 0;
    if (!s_retiredContexts.empty()) {
        K2Glrc* rc = new K2Glrc;
        rc->ctx = s_retiredContexts.back();
        s_retiredContexts.pop_back();
        return rc;
    }
    SDL_GLContext ctx = SDL_GL_CreateContext(d->wnd->sdl);
    if (!ctx) {
        fprintf(stderr, "k2shim: SDL_GL_CreateContext: %s\n", SDL_GetError());
        return 0;
    }
#ifdef K2P_MOBILE_HOST
    k2android_gl_context_created();   // gl4es probes the GLES context SDL just made current
#endif
    // SDL makes it current; Win32 does not. Put back whatever was current.
    if (t_currentDc && t_currentDc->wnd && t_currentRc)
        SDL_GL_MakeCurrent(t_currentDc->wnd->sdl, t_currentRc->ctx);
    else
        SDL_GL_MakeCurrent(d->wnd->sdl, 0);
    K2Glrc* rc = new K2Glrc;
    rc->ctx = ctx;
    return rc;
}

int wglMakeCurrent(void* hDC, void* hGLRC)
{
    K2Glrc* rc = static_cast<K2Glrc*>(hGLRC);
    if (!rc) {
        SDL_GL_MakeCurrent(t_currentDc && t_currentDc->wnd ? t_currentDc->wnd->sdl : 0, 0);
        t_currentDc = 0;
        t_currentRc = 0;
        return 1;
    }
    K2Dc* d = Dc(hDC);
    if (!d || !d->wnd) return 0;
    if (SDL_GL_MakeCurrent(d->wnd->sdl, rc->ctx) != 0) {
        fprintf(stderr, "k2shim: SDL_GL_MakeCurrent: %s\n", SDL_GetError());
        return 0;
    }
    t_currentDc = d;
    t_currentRc = rc;
    if (!rc->bConfigured) {
        // The engine's pixel data is tightly packed. The stock build's bundled gluBuild2DMipmaps
        // ignores the unpack alignment but the host's GLU and GL honour it, and at the default 4 an
        // odd-width RGB image (a 1023x1024 load screen) is read as padded rows, skewed and past the
        // buffer.
        k2gl::glPixelStorei(0x0cf5, 1);   // GL_UNPACK_ALIGNMENT
        rc->bConfigured = true;
    }
    return 1;
}

int wglMakeContextCurrentARB(void* hDrawDC, void*, void* hGLRC) { return wglMakeCurrent(hDrawDC, hGLRC); }

int wglDeleteContext(void* hGLRC)
{
    K2Glrc* rc = static_cast<K2Glrc*>(hGLRC);
    if (!rc) return 0;
    if (t_currentRc == rc) wglMakeCurrent(0, 0);
    s_retiredContexts.push_back(rc->ctx);   // see s_retiredContexts
    delete rc;
    return 1;
}

void* wglGetCurrentContext() { return t_currentRc; }
void* wglGetCurrentDC() { return t_currentDc; }
int wglShareLists(void*, void*) { return 1; }
int wglSwapIntervalEXT(int n) { return SDL_GL_SetSwapInterval(n) == 0; }

// No pbuffers; the engine's render-to-texture paths fall back when these fail.
void* wglCreatePBufferARB(void*, int, int, int, const int*) { return 0; }
int wglDestroyPBufferARB(void*) { return 0; }
void* wglGetPBufferDCARB(void*) { return 0; }
int wglReleasePBufferDCARB(void*, void*) { return 0; }
int wglBindTexImageARB(void*, int) { return 0; }

// Writes the back buffer (before the swap) to `szName` as a binary PPM.
static void WriteShot(K2Window* w, const char* szName)
{
    int nW, nH;
    SDL_GL_GetDrawableSize(w->sdl, &nW, &nH);
    std::string pixels(static_cast<size_t>(nW) * nH * 3, '\0');
    k2gl::glPixelStorei(0x0d05, 1);   // GL_PACK_ALIGNMENT
    k2gl::glReadPixels(0, 0, nW, nH, 0x1907, 0x1401, &pixels[0]);   // GL_RGB, GL_UNSIGNED_BYTE
    if (FILE* f = fopen(szName, "wb")) {
        fprintf(f, "P6\n%d %d\n255\n", nW, nH);
        for (int y = nH - 1; y >= 0; y--)   // GL rows run bottom-up
            fwrite(&pixels[static_cast<size_t>(y) * nW * 3], 1, static_cast<size_t>(nW) * 3, f);
        fclose(f);
        fprintf(stderr, "k2: wrote %s\n", szName);
    }
}

// K2_SHOT=<frame>[,<frame>...]: write the back buffer of those frames (counted in SwapBuffers
// calls) to k2shot_<frame>.ppm in the working directory, just before the swap.
static void MaybeScreenshot(K2Window* w)
{
    static const char* s_pszShots = getenv("K2_SHOT");
    static int s_nFrame;
    int nFrame = ++s_nFrame;
    if (!s_pszShots)
        return;
    bool bWanted = false;
    for (const char* p = s_pszShots; *p;) {
        char* end;
        long n = strtol(p, &end, 10);
        if (end == p) break;
        bWanted |= n == nFrame;
        p = *end ? end + 1 : end;
    }
    if (!bWanted)
        return;
    char szName[64];
    snprintf(szName, sizeof(szName), "k2shot_%d.ppm", nFrame);
    WriteShot(w, szName);
}

// K2_INPUT=<frame>:<action>[;<frame>:<action>...]: synthesize input at those frames (the same
// SwapBuffers count as K2_SHOT), for unattended runs. Actions, in client coordinates:
//   move:X,Y, click:X,Y (move, left down, and left up one frame later), tap:X,Y (the click a touch
//     screen makes, through SDL's touch-to-mouse events)
//   key:NAME (SDL key name, pressed and released), keydown:NAME, keyup:NAME (hold a key across
//     frames)
//   down:X,Y / up:X,Y (move there, then press or release the left button: a drag is down, moves,
//     up)
//   rdown:0 / rup:0 (press or release the right button where the pointer is)
//   rel:DX,DY (relative motion with the pointer held still: what a captured right-drag look
//     reports)
//   wheel:N (N mouse-wheel notches, positive away from the user)
//   text:STRING (an SDL_TEXTINPUT event; `_` stands for a space in K2_INPUT)
//   pad:NAME / paddown:NAME / padup:NAME / padaxis:NAME,VALUE (a virtual SDL game controller,
//     attached on first use; SDL_GameController names: a, dpup, leftshoulder, lefttrigger, ...)
//   shot:FILE (write the back buffer to FILE as a PPM)
//   achieve:TAG (unlock an achievement, as the engine does: shim_achievements.cpp)
//   con:CMD (an engine console command, `+` = space, as K1's k1sdl.cpp: con:warp+101PER moves to
//     a module; the cheat commands need [Game Options] EnableCheats=1 in the player's ini)
//   cov:<path> / cov:reset (K2P_COV builds only: dump / zero the engine's execution counts; there
//     is also K2_COV=<path>, dumped at exit)
// K2_CTL=<file>: the same actions, live. The file is polled every frame; when it exists its lines
// are read and it is deleted. A line is `[+N:]action:arg` (N frames from now; default the next
// frame), so a driver can look at a `shot:` and decide what to send next. `quit:0` exits at once.
static bool s_bPendingUp;
static bool s_bPendingUpTouch;   // that up is a tap's (SDL_TOUCH_MOUSEID)

// the virtual pad (pad:/paddown:/padup:/padaxis:).
// An SDL virtual game controller, attached on the first pad action, so an unattended run drives the
// whole pad path -- SDL, then shim_dinput.cpp's DirectInput device, then the engine's own pad
// bindings, and SDL to the console driver -- with no hardware. Attaching it is a real hot-plug
// (SDL_CONTROLLERDEVICEADDED), so the engine re-enumerates exactly as for a pad plugged in late.
// Names are SDL_GameController's: a b x y back guide start leftstick rightstick leftshoulder
// rightshoulder dpup dpdown dpleft dpright, plus the axes leftx lefty rightx righty lefttrigger
// righttrigger (`pad:lefttrigger` pulls it fully). `pad:NAME` holds for kPadTapFrames, because the
// console driver polls and a one-frame tap can fall between two of its ticks.
static SDL_Joystick* s_pVirtualPad;
static std::vector<std::pair<int, std::string>> s_padUps;   // (frame, name) still to release
static const int kPadTapFrames = 6;

static bool VirtualPadSet(const char* szName, int nValue)   // nValue: 1/0 for a button, raw for an axis
{
    if (!s_pVirtualPad) {
        // SDL drops pad presses while no window has keyboard focus, and an unattended run's window
        // usually has none (K2_HIDDEN, or simply behind the terminal), so the virtual button never
        // latches without this hint. A real pad in a focused window is unaffected.
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        int nIndex = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX,
                                               SDL_CONTROLLER_BUTTON_MAX, 0);
        s_pVirtualPad = nIndex >= 0 ? SDL_JoystickOpen(nIndex) : 0;
        fprintf(stderr, "k2: virtual pad: attach -> %d (%s)\n", nIndex,
                s_pVirtualPad ? "open" : SDL_GetError());
        if (!s_pVirtualPad) return false;
    }
    SDL_GameControllerButton b = SDL_GameControllerGetButtonFromString(szName);
    if (b != SDL_CONTROLLER_BUTTON_INVALID)
        return SDL_JoystickSetVirtualButton(s_pVirtualPad, b, nValue ? 1 : 0) == 0;
    SDL_GameControllerAxis a = SDL_GameControllerGetAxisFromString(szName);
    if (a != SDL_CONTROLLER_AXIS_INVALID)
        return SDL_JoystickSetVirtualAxis(s_pVirtualPad, a, static_cast<Sint16>(nValue)) == 0;
    fprintf(stderr, "k2: virtual pad: no button or axis \"%s\"\n", szName);
    return false;
}

static bool IsTrigger(const char* szName)
{
    return !strcmp(szName, "lefttrigger") || !strcmp(szName, "righttrigger");
}

#if K2P_COV   // platforms/compat/k2cov.cpp
extern "C" void k2cov_dump(const char* szPath);
extern "C" void k2cov_reset(void);
#endif
extern "C++" char* FuncInterp(char* pszExpr);   // src/libsource/AURORA/aurconsole.cpp: the console's interpreter

static void DoInputAction(K2Window* w, int nFrame, const std::string& sAction, const char* szArg)
{
    SDL_Event e;
    fprintf(stderr, "k2: input frame %d: %s %s\n", nFrame, sAction.c_str(), szArg);
    if (sAction == "move" || sAction == "click" || sAction == "tap" || sAction == "down" || sAction == "up") {
        int x, y;
        if (sscanf(szArg, "%d,%d", &x, &y) != 2) return;
        const Uint32 nWhich = sAction == "tap" ? SDL_TOUCH_MOUSEID : 0;
        memset(&e, 0, sizeof e);
        e.type = SDL_MOUSEMOTION;
        e.motion.which = nWhich;
        e.motion.windowID = SDL_GetWindowID(w->sdl);
        e.motion.x = x;
        e.motion.y = y;
        e.motion.xrel = x - g_synthX;
        e.motion.yrel = y - g_synthY;
        g_synthCursor = true;
        g_synthX = x;
        g_synthY = y;
        SDL_PushEvent(&e);
        if (sAction == "down" || sAction == "up") {
            if (sAction == "down") g_synthButtons |= SDL_BUTTON_LMASK;
            else g_synthButtons &= ~SDL_BUTTON_LMASK;
            memset(&e, 0, sizeof e);
            e.type = sAction == "down" ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
            e.button.windowID = SDL_GetWindowID(w->sdl);
            e.button.button = SDL_BUTTON_LEFT;
            e.button.clicks = 1;
            e.button.x = x;
            e.button.y = y;
            SDL_PushEvent(&e);
        }
        if (sAction == "click" || sAction == "tap") {
            memset(&e, 0, sizeof e);
            e.type = SDL_MOUSEBUTTONDOWN;
            e.button.which = nWhich;
            e.button.windowID = SDL_GetWindowID(w->sdl);
            e.button.button = SDL_BUTTON_LEFT;
            e.button.clicks = 1;
            e.button.x = x;
            e.button.y = y;
            SDL_PushEvent(&e);
            g_synthButtons |= SDL_BUTTON_LMASK;
            s_bPendingUp = true;
            s_bPendingUpTouch = nWhich != 0;
        }
    } else if (sAction == "rdown" || sAction == "rup") {
        // the right button at the current pointer (rdown with down gives both buttons)
        bool bDown = sAction == "rdown";
        if (bDown) g_synthButtons |= SDL_BUTTON_RMASK;
        else g_synthButtons &= ~SDL_BUTTON_RMASK;
        memset(&e, 0, sizeof e);
        e.type = bDown ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
        e.button.windowID = SDL_GetWindowID(w->sdl);
        e.button.button = SDL_BUTTON_RIGHT;
        e.button.clicks = 1;
        e.button.x = g_synthX;
        e.button.y = g_synthY;
        SDL_PushEvent(&e);
    } else if (sAction == "rel") {
        // relative motion with the pointer held still: a captured mouse-look drag
        int dx, dy;
        if (sscanf(szArg, "%d,%d", &dx, &dy) != 2) return;
        memset(&e, 0, sizeof e);
        e.type = SDL_MOUSEMOTION;
        e.motion.windowID = SDL_GetWindowID(w->sdl);
        e.motion.x = g_synthX;
        e.motion.y = g_synthY;
        e.motion.xrel = dx;
        e.motion.yrel = dy;
        SDL_PushEvent(&e);
    } else if (sAction == "wheel") {
        // N notches of the mouse wheel, positive away from the user, at the pointer
        memset(&e, 0, sizeof e);
        e.type = SDL_MOUSEWHEEL;
        e.wheel.windowID = SDL_GetWindowID(w->sdl);
        e.wheel.y = atoi(szArg);
        SDL_PushEvent(&e);
    } else if (sAction == "focus" || sAction == "window") {
        // focus:lost|gained and window:minimized|restored post what the SDL window-event arm posts
        // for the real event, since a pushed SDL event would not move SDL's own window state.
        HWND__* h = reinterpret_cast<HWND__*>(w);
        std::string sArg = szArg;
        if (sArg == "lost") {
            Post(h, WM_KILLFOCUS, 0, 0);
            Post(h, WM_ACTIVATE, 0, 0);
        } else if (sArg == "gained") {
            Post(h, WM_ACTIVATE, 1, 0);
            Post(h, WM_SETFOCUS, 0, 0);
        } else if (sArg == "minimized" && !g_appSuspended) {
            g_appSuspended = true;
            Post(h, WM_ACTIVATEAPP, 0, 0);
        } else if (sArg == "restored" && g_appSuspended) {
            g_appSuspended = false;
            Post(h, WM_ACTIVATEAPP, 1, 0);
        }
    } else if (sAction == "key" || sAction == "keydown" || sAction == "keyup") {
        SDL_Keycode k = SDL_GetKeyFromName(szArg);
        if (k == SDLK_UNKNOWN) return;
        for (Uint32 type : { static_cast<Uint32>(SDL_KEYDOWN), static_cast<Uint32>(SDL_KEYUP) }) {
            if ((sAction == "keydown" && type != SDL_KEYDOWN) || (sAction == "keyup" && type != SDL_KEYUP))
                continue;
            memset(&e, 0, sizeof e);
            e.type = type;
            e.key.windowID = SDL_GetWindowID(w->sdl);
            e.key.keysym.sym = k;
            e.key.keysym.scancode = SDL_GetScancodeFromKey(k);
            SDL_PushEvent(&e);
            // With text input on (an edit box has focus), a real character-key press also produces
            // SDL_TEXTINPUT and therefore a WM_CHAR, while a pushed SDL_KEYDOWN does not: an
            // injected key reaches the engine's WM_KEYDOWN arm but never its character arm. (With
            // text input off, PumpSdl makes the WM_CHAR from the key itself, CharFromSdl.) The typed
            // cheat console lives entirely on the character arm, where WinMessageHandler ->
            // HandleWMCharMessage opens it on a WM_CHAR of '`' (its nConsoleKey; VK_OEM_3 and
            // VK_NUMPAD0 are character keys on Windows and really do deliver that character), feeds
            // every later char to DoConsole and runs the line when DoConsole sees a CR. Synthesize
            // those two. Escape is deliberately left out: a Windows Esc carries no WM_CHAR, and
            // injecting one would double-fire the GUI's Esc handling, which `key:Escape` already
            // reaches through WM_KEYDOWN.
            if (type == SDL_KEYDOWN) {
                unsigned int ch = 0;
                if (k == SDLK_RETURN || k == SDLK_KP_ENTER) ch = 0x0d;
                else if (k == SDLK_BACKQUOTE) ch = 0x60;
                if (ch && SDL_IsTextInputActive()) Post(reinterpret_cast<HWND__*>(w), WM_CHAR, ch, 1);
            }
        }
    } else if (sAction == "text") {
        // WM_CHAR per character, exactly what the SDL_TEXTINPUT arm posts; pushing a synthetic
        // SDL_TEXTINPUT crashes inside sdl2-compat's SDL3 conversion.
        for (const unsigned char* q = reinterpret_cast<const unsigned char*>(szArg); *q; q++)
            Post(reinterpret_cast<HWND__*>(w), WM_CHAR, *q, 1);
    } else if (sAction == "pad" || sAction == "paddown" || sAction == "padup") {
        const bool bDown = sAction != "padup";
        VirtualPadSet(szArg, bDown ? (IsTrigger(szArg) ? 32767 : 1) : 0);
        if (sAction == "pad") s_padUps.push_back({nFrame + kPadTapFrames, szArg});
    } else if (sAction == "padaxis") {           // NAME,VALUE (-32768..32767)
        char szName[32];
        int nValue;
        if (sscanf(szArg, "%31[a-z],%d", szName, &nValue) == 2) VirtualPadSet(szName, nValue);
    } else if (sAction == "shot") {
        WriteShot(w, szArg);
    } else if (sAction == "achieve") {
        // What the engine's own unlock call does (shim_achievements.cpp; with Steam active, a real
        // unlock on the player's account).
        k2host_unlock_achievement(szArg);
    } else if (sAction == "con") {
        std::string sCmd = szArg;
        for (char& c : sCmd)
            if (c == '+') c = ' ';
        const char* pszOut = FuncInterp(&sCmd[0]);
        fprintf(stderr, "k2: con %s -> %s\n", sCmd.c_str(), pszOut ? pszOut : "(null)");
    } else if (sAction == "cov") {
#if K2P_COV
        // `cov:<path>` dumps the counts, `cov:reset` zeroes them (keeping the PCs), so a run can
        // reset, perform one action and dump.
        if (!strcmp(szArg, "reset"))
            k2cov_reset();
        else
            k2cov_dump(szArg);
#endif
    } else if (sAction == "close") {
        // what SDL_QUIT posts for the window's close button: the engine's own shutdown runs
        Post(reinterpret_cast<HWND__*>(w), WM_CLOSE, 0, 0);
    } else if (sAction == "quit") {
#if K2P_COV
        {   // _exit() below skips atexit, so dump K2_COV by hand (the close: path gets it from
            // the atexit handler k2cov.cpp registers).
            const char* szCov = getenv("K2_COV");
            if (szCov && *szCov)
                k2cov_dump(szCov);
        }
#endif
        fprintf(stderr, "k2: quit requested\n");
        _exit(0);
    }
}

static void MaybeInput(K2Window* w)
{
    static const char* s_pszInput = getenv("K2_INPUT");
    static const char* s_pszCtl = getenv("K2_CTL");
    static std::vector<std::pair<int, std::string>> s_queue;   // K2_CTL: (frame, "action:arg")
    static int s_nFrame;
    int nFrame = ++s_nFrame;
    SDL_Event e;
    for (size_t i = 0; i < s_padUps.size();) {
        if (s_padUps[i].first > nFrame) { i++; continue; }
        VirtualPadSet(s_padUps[i].second.c_str(), 0);
        fprintf(stderr, "k2: input frame %d: pad %s up\n", nFrame, s_padUps[i].second.c_str());
        s_padUps.erase(s_padUps.begin() + static_cast<int64_t>(i));
    }
    if (s_bPendingUp) {
        s_bPendingUp = false;
        g_synthButtons &= ~SDL_BUTTON_LMASK;
        memset(&e, 0, sizeof e);
        e.type = SDL_MOUSEBUTTONUP;
        e.button.which = s_bPendingUpTouch ? SDL_TOUCH_MOUSEID : 0;
        e.button.windowID = SDL_GetWindowID(w->sdl);
        e.button.button = SDL_BUTTON_LEFT;
        e.button.clicks = 1;
        e.button.x = g_synthX;
        e.button.y = g_synthY;
        SDL_PushEvent(&e);
        fprintf(stderr, "k2: input frame %d: left up%s\n", nFrame, s_bPendingUpTouch ? " (tap)" : "");
    }
    if (s_pszCtl) {
        if (FILE* f = fopen(s_pszCtl, "r")) {
            char szLine[512];
            while (fgets(szLine, sizeof szLine, f)) {
                std::string sLine(szLine);
                while (!sLine.empty() && (sLine.back() == '\n' || sLine.back() == '\r'))
                    sLine.pop_back();
                if (sLine.empty() || sLine[0] == '#')
                    continue;
                int nDelay = 0;
                if (sLine[0] == '+') {
                    nDelay = atoi(sLine.c_str() + 1);
                    size_t c = sLine.find(':');
                    sLine = c == std::string::npos ? "" : sLine.substr(c + 1);
                }
                s_queue.push_back({nFrame + (nDelay > 0 ? nDelay : 0), sLine});
            }
            fclose(f);
            remove(s_pszCtl);
        }
        for (size_t i = 0; i < s_queue.size();) {
            if (s_queue[i].first > nFrame) { i++; continue; }
            std::string sItem = s_queue[i].second;
            s_queue.erase(s_queue.begin() + static_cast<int64_t>(i));
            size_t c = sItem.find(':');
            if (c == std::string::npos) continue;
            if (s_bPendingUp && sItem.compare(0, c, "click") == 0) {   // one click per frame
                s_queue.insert(s_queue.begin(), {nFrame + 1, sItem});
                break;
            }
            DoInputAction(w, nFrame, sItem.substr(0, c), sItem.c_str() + c + 1);
        }
    }
    if (!s_pszInput)
        return;
    for (const char* p = s_pszInput; *p;) {
        const char* end = strchr(p, ';');
        std::string item(p, end ? static_cast<size_t>(end - p) : strlen(p));
        p = end ? end + 1 : p + item.size();
        char szAction[16] = {}, szArg[64] = {};
        int n;
        if (sscanf(item.c_str(), "%d:%15[a-z]:%63s", &n, szAction, szArg) != 3 || n != nFrame)
            continue;
        for (char* q = szArg; *q; q++)
            if (*q == '_' && strcmp(szAction, "text") == 0) *q = ' ';
        DoInputAction(w, nFrame, szAction, szArg);
    }
}


#ifdef K2P_IOS
extern "C" void k2ios_gpu_capture(int bBegin);   // ios/ios_gpucapture.mm
#endif
// K2_GL_TRACE=menu: the engine reports each main menu it builds (DisplayMainMenu) and the three
// frames after it are traced, the ones a return from a menu draws first.
static int s_nTraceMenuFrames;
extern "C" void k2host_gl_trace_main_menu(void)
{
    static const char* s_pszTrace = getenv("K2_GL_TRACE");
    if (s_pszTrace && strcmp(s_pszTrace, "menu") == 0) {
        fprintf(stderr, "k2: trace: main menu built; tracing the next 3 frames\n");
        s_nTraceMenuFrames = 3;
    }
}

int SwapBuffers(void* hDC)
{
    K2Dc* d = Dc(hDC);
    if (!d || !d->wnd) return 0;
    MaybeScreenshot(d->wnd);
    {
        static const char* s_pszTrace = getenv("K2_GL_TRACE");
        static int64_t s_nFrame;
        ++s_nFrame;
        if (k2shim_gl_tracing) {
            // where the frame's last draws went: a framebuffer object left bound, a scissor box or
            // a colour mask would keep them off the presented image
            int nFbo = 0, nDrawBuffer = 0, anMask[4] = {}, anScissor[4] = {};
            k2gl::glGetIntegerv(0x8ca6, &nFbo);           // GL_FRAMEBUFFER_BINDING
            k2gl::glGetIntegerv(0x0c01, &nDrawBuffer);    // GL_DRAW_BUFFER
            k2gl::glGetIntegerv(0x0c23, anMask);          // GL_COLOR_WRITEMASK
            k2gl::glGetIntegerv(0x0c10, anScissor);       // GL_SCISSOR_BOX
            fprintf(stderr, "k2: trace: at swap: framebuffer %d draw buffer 0x%x colour mask %d%d%d%d "
                    "scissor %s %d %d %d %d\n", nFbo, nDrawBuffer, anMask[0], anMask[1], anMask[2],
                    anMask[3], k2gl::glIsEnabled(0x0c11) ? "on" : "off", anScissor[0], anScissor[1],
                    anScissor[2], anScissor[3]);
            fprintf(stderr, "k2: trace: end of frame %" PRId64 "\n", s_nFrame);
        }
        k2shim_gl_tracing = s_pszTrace && strtol(s_pszTrace, 0, 10) == s_nFrame + 1;
        if (s_nTraceMenuFrames > 0) {
            --s_nTraceMenuFrames;
            fprintf(stderr, "k2: trace: frame %" PRId64 " (main menu)\n", s_nFrame + 1);
            k2shim_gl_tracing = true;
        }
        // K2_GL_TRACE=file: a k2gltrace file in the game directory traces the next frame, for a
        // device with no console (drop the file in with devicectl, read k2.log).
#ifdef K2P_IOS
        // ... and a k2gputrace file asks for a Metal GPU capture of the next frame
        // (ios/ios_gpucapture.mm).
        static bool s_bGpuCapturing;
        if (s_bGpuCapturing) {
            k2ios_gpu_capture(0);
            s_bGpuCapturing = false;
        }
        if (s_pszTrace && strcmp(s_pszTrace, "file") == 0 && remove("k2gputrace") == 0) {
            k2ios_gpu_capture(1);
            s_bGpuCapturing = true;
        }
#endif
        if (s_pszTrace && strcmp(s_pszTrace, "file") == 0 && remove("k2gltrace") == 0) {
            fprintf(stderr, "k2: trace: frame %" PRId64 " (k2gltrace)\n", s_nFrame + 1);
            k2shim_gl_tracing = true;
        }
    }
    MaybeInput(d->wnd);
    // K2_FPS=1 logs the SwapBuffers rate once a second.
    static const bool s_bFps = getenv("K2_FPS") != 0;
    if (s_bFps) {
        static Uint32 s_nStart = SDL_GetTicks(), s_nFrames, s_nTotal;
        ++s_nFrames, ++s_nTotal;
        Uint32 nNow = SDL_GetTicks();
        if (nNow - s_nStart >= 1000) {
            fprintf(stderr, "k2: fps %.1f (frame %u)\n", s_nFrames * 1000.0 / (nNow - s_nStart), s_nTotal);
            s_nStart = nNow;
            s_nFrames = 0;
        }
    }
#ifdef K2P_MOBILE_HOST
    k2android_gl_pre_swap();   // gl4es: flush what it batched into the back buffer
#endif
    k2shim_gl_present(d->wnd->sdl, SDL_GL_SwapWindow);
#ifdef K2P_MOBILE_HOST
    k2android_gl_post_swap();
#endif
    return 1;
}

// display modes (DEVMODEA).

#ifndef K2P_MOBILE_HOST
// On the desktop the "display modes" are window sizes -- the usual ones that fit the display's
// usable area with room for a title bar, then the size a fullscreen window gets (shim_display.cpp
// reports the desktop less a notch), which ChangeDisplaySettingsA turns into a fullscreen window,
// and on a HiDPI display that size in pixels for the native-resolution fullscreen (the window asks
// SDL for its full-resolution drawable and the engine's client coordinates are its pixels). A
// desktop window keeps the display's refresh, so each reports 1, Win32's "the hardware's default",
// which the engine's mode matchers all accept.
static void FullscreenSizes(int* pW, int* pH, int* pPixelW, int* pPixelH)
{
    k2shim_fullscreen_points(pW, pH);
    *pPixelW = *pW * k2shim_pixel_scale();
    *pPixelH = *pH * k2shim_pixel_scale();
}

static const std::vector<SDL_DisplayMode>& DesktopWindowModes()
{
    static std::vector<SDL_DisplayMode> s_modes;
    if (!s_modes.empty()) return s_modes;
    int nFullW, nFullH, nPixelW, nPixelH;
    FullscreenSizes(&nFullW, &nFullH, &nPixelW, &nPixelH);
    static const int s_sizes[][2] = {
        {800, 600},   {1024, 768},  {1152, 864},  {1280, 720},  {1280, 800},  {1280, 960},
        {1280, 1024}, {1366, 768},  {1440, 900},  {1600, 900},  {1600, 1200}, {1680, 1050},
        {1920, 1080}, {1920, 1200}, {2560, 1080}, {2560, 1440}, {2560, 1600}, {3440, 1440},
        {3840, 2160}, {5120, 2880},
    };
    SDL_DisplayMode desktop;
    if (SDL_GetDesktopDisplayMode(0, &desktop) != 0) return s_modes;
    SDL_Rect usable = {0, 0, desktop.w, desktop.h};
    SDL_GetDisplayUsableBounds(0, &usable);
    for (const auto& s : s_sizes) {
        if (s[0] > usable.w || s[1] + 40 > usable.h) continue;
        if (s[0] == nFullW && s[1] == nFullH) continue;
        SDL_DisplayMode m = desktop;
        m.w = s[0];
        m.h = s[1];
        s_modes.push_back(m);
    }
    SDL_DisplayMode full = desktop;
    full.w = nFullW;
    full.h = nFullH;
    s_modes.push_back(full);
    if (nPixelW != nFullW) {
        full.w = nPixelW;
        full.h = nPixelH;
        s_modes.push_back(full);
    }
    for (auto& m : s_modes) m.refresh_rate = 1;
    fprintf(stderr, "k2shim: desktop %dx%d, usable %dx%d, fullscreen %dx%d (%dx%d pixels): %d sizes\n",
            desktop.w, desktop.h, usable.w, usable.h, nFullW, nFullH, nPixelW, nPixelH, static_cast<int>(s_modes.size()));
    return s_modes;
}

// KOTOR 1's Screen Resolution list (k1/k1port.cpp EnumDisplaySettingsA) holds the window sizes
// above in points, then the fullscreen size (the desktop's, which k1sdl.cpp makes a fullscreen
// window whatever the Fullscreen row says), but not the native-resolution one, as K1 renders a
// window's pixels.
extern "C" int k2host_window_size(unsigned int nIndex, int* pW, int* pH)
{
    const std::vector<SDL_DisplayMode>& modes = DesktopWindowModes();
    int nFullW, nFullH, nPixelW, nPixelH;
    FullscreenSizes(&nFullW, &nFullH, &nPixelW, &nPixelH);
    unsigned int nWindowed = static_cast<unsigned int>(modes.size()) - (nPixelW != nFullW ? 1 : 0);
    if (modes.empty() || nIndex >= nWindowed) return 0;
    *pW = modes[nIndex].w;
    *pH = modes[nIndex].h;
    return 1;
}

// The last windowed size ChangeDisplaySettingsA set, which is the Fullscreen option's way back, and
// whether that mode was a fullscreen one.
static int s_nWindowedW = 1280, s_nWindowedH = 720;
static bool s_bFullscreenMode;

// Reports the largest fullscreen size (the native one on a HiDPI display) as the engine's current
// display mode, so that CAuroraInterface's desktop-size filter keeps every mode listed.
extern "C" void k2host_desktop_size(int* pW, int* pH)
{
    int nFullW, nFullH;
    FullscreenSizes(&nFullW, &nFullH, pW, pH);
}

// The shim's half of the Fullscreen option (CSWGuiOptionsGraphics::OnFullscreen): the mode to switch
// to -- on, the native-resolution fullscreen; off, the last window.
#ifndef K2_QOL_K1   // K1's is the next launch's setting (k1/k1sdl.cpp)
extern "C" int k2host_fullscreen(void) { return s_bFullscreenMode; }
#endif
extern "C" void k2host_fullscreen_mode(int bOn, int* pW, int* pH)
{
    int nFullW, nFullH;
    if (bOn) {
        FullscreenSizes(&nFullW, &nFullH, pW, pH);
        return;
    }
    *pW = s_nWindowedW;
    *pH = s_nWindowedH;
}

// Rebuilds a window's SDL window with or without a full-resolution drawable, which SDL decides when
// the window is made. ChangeDisplaySettingsA runs before the engine makes its GL context, so the new
// window is the one the context is made current on.
static void SetWindowHiDpi(K2Window* w, bool bHiDpi)
{
    Uint32 flags = (w->sdlFlags & ~static_cast<Uint32>(SDL_WINDOW_ALLOW_HIGHDPI)) |
                   (bHiDpi ? static_cast<Uint32>(SDL_WINDOW_ALLOW_HIGHDPI) : 0);
    if (flags == w->sdlFlags) return;
    int cw, ch;
    SDL_GetWindowSize(w->sdl, &cw, &ch);
    bool bShown = (SDL_GetWindowFlags(w->sdl) & SDL_WINDOW_SHOWN) != 0;
    SDL_Window* sdl = SDL_CreateWindow(SDL_GetWindowTitle(w->sdl), SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED, cw, ch, flags | SDL_WINDOW_HIDDEN);
    if (!sdl) {
        fprintf(stderr, "k2shim: SDL_CreateWindow (HiDPI %d): %s\n", bHiDpi, SDL_GetError());
        return;
    }
    k2shim_window_icon(sdl);
    g_bySdlId.erase(SDL_GetWindowID(w->sdl));
    SDL_DestroyWindow(w->sdl);
    w->sdl = sdl;
    w->sdlFlags = flags;
    g_bySdlId[SDL_GetWindowID(sdl)] = w;
    if (bShown && !getenv("K2_HIDDEN")) {
        SDL_ShowWindow(sdl);
        SDL_RaiseWindow(sdl);
    }
#ifndef K2_UI_TOUCH
    k2shim_cursor_refresh();
#endif
}
#endif

// iModeNum: ENUM_CURRENT_SETTINGS (-1), ENUM_REGISTRY_SETTINGS (-2), or an index into the list.
int EnumDisplaySettingsA(const char*, unsigned int iModeNum, PortDevMode* pDevMode)
{
    if (!pDevMode) return 0;
    SDL_DisplayMode m;
#ifdef K2P_IOS
    // iOS: one mode, the screen in pixels (SDL's modes are in points) and landscape. The app is
    // landscape-only, but a scene can still report the portrait screen before it has rotated.
    int r = (iModeNum >= 0xfffffffeu || iModeNum == 0) ? SDL_GetDesktopDisplayMode(0, &m) : -1;
    if (r == 0 && m.h > m.w) std::swap(m.w, m.h);
    if (r == 0) {
        m.w = static_cast<int>(m.w * k2shim_pixel_scalef() + 0.5f);
        m.h = static_cast<int>(m.h * k2shim_pixel_scalef() + 0.5f);
    }
#elif defined(K2P_MOBILE_HOST)
    int r = (iModeNum >= 0xfffffffeu) ? SDL_GetDesktopDisplayMode(0, &m)
                                       : SDL_GetDisplayMode(0, static_cast<int>(iModeNum), &m);
#else
    int r = -1;
    if (iModeNum >= 0xfffffffeu) {
        r = SDL_GetDesktopDisplayMode(0, &m);
        if (r == 0) k2host_desktop_size(&m.w, &m.h);
    } else if (iModeNum < DesktopWindowModes().size()) {
        m = DesktopWindowModes()[iModeNum];
        r = 0;
    }
#endif
    // K2_DISPLAY_MODE=WxH: the display has exactly that one mode, so the engine picks it (unless
    // swkotor2.ini's Width/Height name another), which simulates a phone's screen on a desktop.
    static int s_nForcedW = -1, s_nForcedH;
    if (s_nForcedW < 0) {
        s_nForcedW = 0;
        if (const char* e = getenv("K2_DISPLAY_MODE")) sscanf(e, "%dx%d", &s_nForcedW, &s_nForcedH);
    }
    if (s_nForcedW > 0 && s_nForcedH > 0) {
        if (iModeNum < 0xfffffffeu && iModeNum != 0) return 0;
        SDL_zero(m);
        m.w = s_nForcedW;
        m.h = s_nForcedH;
        r = 0;
    }
    if (r != 0) return 0;
    pDevMode->m_size = DEVMODEA_SIZE;
    pDevMode->m_fields = 0x205c0000;   // what the stock macOS build writes
    pDevMode->m_displayFlags = 0;
    pDevMode->m_bitsPerPel = 32;
    pDevMode->m_pelsWidth = static_cast<uint32_t>(m.w);
    pDevMode->m_pelsHeight = static_cast<uint32_t>(m.h);
    pDevMode->m_displayFrequency = static_cast<uint32_t>(m.refresh_rate ? m.refresh_rate : 60);
    return 1;
}

// The engine switches display modes for fullscreen, while a host window just takes the size and
// never does a real mode change (CDS_TEST, 2, and a null mode both succeed without touching
// anything). On the desktop the desktop's own size is a fullscreen window and any other size a
// centred one.
int ChangeDisplaySettingsA(PortDevMode* pDevMode, unsigned int dwFlags)
{
    if (!pDevMode || (dwFlags & 2) || !g_mainWindow) return 0;
    int w = static_cast<int>(pDevMode->m_pelsWidth), h = static_cast<int>(pDevMode->m_pelsHeight);
    if (w <= 0 || h <= 0) return 0;
#ifdef K2P_IOS
    g_mainWindow->modeH = h;   // iOS: the screen is the only mode, and the window already has it
#elif defined(K2P_MOBILE_HOST)
    SDL_SetWindowSize(g_mainWindow->sdl, w, h);
#else
    // SDL defers fullscreen until the window is shown (K2_HIDDEN never shows it), so the size is
    // set either way. The native-resolution fullscreen is the fullscreen window with a HiDPI
    // drawable: its points are the fullscreen size and the engine's pixels twice that.
    int nFullW, nFullH, nPixelW, nPixelH;
    FullscreenSizes(&nFullW, &nFullH, &nPixelW, &nPixelH);
    bool bNative = nPixelW != nFullW && w == nPixelW && h == nPixelH;
    bool bFullscreen = bNative || (w == nFullW && h == nFullH);
    K2Window* pWnd = g_mainWindow;
    if (!bFullscreen) SDL_SetWindowFullscreen(pWnd->sdl, 0);
    SetWindowHiDpi(pWnd, bNative);
    pWnd->scale = bNative ? k2shim_pixel_scale() : 1;
    pWnd->modeH = h;
    SDL_SetWindowSize(pWnd->sdl, bFullscreen ? nFullW : w, bFullscreen ? nFullH : h);
    if (bFullscreen) {
        SDL_SetWindowFullscreen(pWnd->sdl, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowPosition(pWnd->sdl, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        s_nWindowedW = w;
        s_nWindowedH = h;
    }
    s_bFullscreenMode = bFullscreen;
    int nWinW = 0, nWinH = 0, nDrawW = 0, nDrawH = 0;
    SDL_GetWindowSize(pWnd->sdl, &nWinW, &nWinH);
    SDL_GL_GetDrawableSize(pWnd->sdl, &nDrawW, &nDrawH);
    fprintf(stderr, "k2shim: display mode %dx%d -> %s window %dx%d, drawable %dx%d\n", w, h,
            bNative ? "native fullscreen" : bFullscreen ? "fullscreen" : "windowed", nWinW, nWinH,
            nDrawW, nDrawH);
#endif
    return 0;   // DISP_CHANGE_SUCCESSFUL
}

// COM.
int64_t CoInitialize(void*) { return 0; }
void CoUninitialize() {}

} // extern "C"
