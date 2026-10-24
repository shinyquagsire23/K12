// shim_display.cpp -- the desktop display the Fullscreen option and the display-mode list are built
// on (the KOTOR II shim's EnumDisplaySettingsA / ChangeDisplaySettingsA, K1's window in k1sdl.cpp).
//
// SDL's desktop mode is the whole display, in points. On a notched MacBook a fullscreen window does
// not get all of it: macOS puts it below the camera housing, so a game drawing at the desktop's size
// is cut off at the top and its mouse is read that many points too high. The size a fullscreen
// window gets is the screen's frame less NSScreen.safeAreaInsets' top, asked through the
// Objective-C runtime (no .mm in the build). The pixels per point are NSScreen.backingScaleFactor.
// This file declares its own entry points rather than including k2shim.h: its Win32 BOOL is not
// objc.h's.
#include <stdint.h>
void k2shim_fullscreen_points(int* pW, int* pH);
int  k2shim_pixel_scale();
float k2shim_pixel_scalef();
struct SDL_Window;
void k2shim_keep_composited(SDL_Window* pWindow);
void k2shim_window_icon(SDL_Window* pWindow);

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <TargetConditionals.h>
#include <dlfcn.h>
#include <objc/message.h>
#include <objc/runtime.h>

namespace {
struct NsRect { double x, y, w, h; };
struct NsEdgeInsets { double top, left, bottom, right; };

id MainScreen()
{
    Class cls = objc_getClass("NSScreen");
    if (!cls) {   // SDL's video backend may not have brought AppKit in yet
        dlopen("/System/Library/Frameworks/AppKit.framework/AppKit", RTLD_LAZY);
        cls = objc_getClass("NSScreen");
    }
    if (!cls) return nil;
    return (reinterpret_cast<id (*)(Class, SEL)>(objc_msgSend))(cls, sel_registerName("mainScreen"));
}

// objc_msgSend hands back a 32-byte struct through the _stret entry on x86-64 only.
template <class T>
T SendStruct(id obj, const char* pszSel)
{
#if defined(__x86_64__)
    return ((T (*)(id, SEL))objc_msgSend_stret)(obj, sel_registerName(pszSel));
#else
    return (reinterpret_cast<T (*)(id, SEL)>(objc_msgSend))(obj, sel_registerName(pszSel));
#endif
}

bool Responds(id obj, const char* pszSel)
{
    return (reinterpret_cast<BOOL (*)(id, SEL, SEL)>(objc_msgSend))(obj, sel_registerName("respondsToSelector:"),
                                                  sel_registerName(pszSel));
}
} // namespace
#endif

void k2shim_fullscreen_points(int* pW, int* pH)
{
    SDL_DisplayMode desktop;
    *pW = 1280;
    *pH = 720;
    if (SDL_GetDesktopDisplayMode(0, &desktop) == 0) {
        *pW = desktop.w;
        *pH = desktop.h;
    }
#ifdef __APPLE__
    if (id screen = MainScreen()) {
        NsRect frame = SendStruct<NsRect>(screen, "frame");
        double fTop = 0.0;
        if (Responds(screen, "safeAreaInsets"))   // macOS 12
            fTop = SendStruct<NsEdgeInsets>(screen, "safeAreaInsets").top;
        // The inset can be fractional (38.5 points); the window gets the whole points below it.
        if (frame.w > 0 && frame.h > 0) {
            *pW = static_cast<int>(frame.w);
            *pH = static_cast<int>(frame.h - fTop);
        }
    }
#endif
    static bool s_bLogged;
    if (!s_bLogged) {
        s_bLogged = true;
        fprintf(stderr, "k2: fullscreen %dx%d points, %dx pixels\n", *pW, *pH, k2shim_pixel_scale());
    }
}

int k2shim_pixel_scale()
{
    static int s_nScale = 0;
    if (s_nScale == 0) {
        s_nScale = 1;
        const char* p = getenv("K2_HIDPI");
#ifdef __APPLE__
        if (!(p && *p == '0')) {
            if (id screen = MainScreen()) {
                double fScale = (reinterpret_cast<double (*)(id, SEL)>(objc_msgSend))(screen,
                                                                     sel_registerName("backingScaleFactor"));
                if (fScale >= 1.5) s_nScale = static_cast<int>(fScale + 0.5);
            }
        }
#else
        (void)p;
#endif
    }
    return s_nScale;
}

// iOS: the screen's pixels per point, which SDL gives a HiDPI window's drawable (UIScreen's
// nativeScale: 2 or 3, and 2.88 / 2.61 on some iPhones). Elsewhere, the desktop's integer scale.
float k2shim_pixel_scalef()
{
#if defined(__APPLE__) && TARGET_OS_IPHONE
    static float s_fScale = 0.0f;
    if (s_fScale == 0.0f) {
        s_fScale = 1.0f;
        const char* p = getenv("K2_HIDPI");
        Class cls = objc_getClass("UIScreen");
        if (cls && !(p && *p == '0')) {
            id screen = (reinterpret_cast<id (*)(Class, SEL)>(objc_msgSend))(cls, sel_registerName("mainScreen"));
            double fScale = screen ? (reinterpret_cast<double (*)(id, SEL)>(objc_msgSend))(screen, sel_registerName("nativeScale")) : 0.0;
            if (fScale >= 1.0) s_fScale = static_cast<float>(fScale);
        }
        fprintf(stderr, "k2: screen scale %.3f pixels per point\n", s_fScale);
    }
    return s_fScale;
#else
    return static_cast<float>(k2shim_pixel_scale());
#endif
}

#if defined(__linux__) && !defined(__ANDROID__)
#include <SDL_syswm.h>
#include <X11/Xatom.h>
#include <dlfcn.h>

// On X11 (GNOME, KDE), the window asks the compositor never to unredirect it. SDL marks every
// window _NET_WM_BYPASS_COMPOSITOR 1, so a fullscreen one is scanned out directly and its swaps
// tear whatever the swap interval says (Mutter on the NVIDIA driver); 2 keeps it composited, which
// is a borderless window the compositor presents on its own vsync. libX11 is the one SDL loaded.
void k2shim_keep_composited(SDL_Window* pWindow)
{
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (!pWindow || !SDL_GetWindowWMInfo(pWindow, &info) || info.subsystem != SDL_SYSWM_X11) return;
    static void* s_pX11 = dlopen("libX11.so.6", RTLD_LAZY);
    if (!s_pX11) return;
    typedef Atom (*InternAtomFn)(Display*, const char*, Bool);
    typedef int (*ChangePropertyFn)(Display*, Window, Atom, Atom, int, int, const unsigned char*, int);
    typedef int (*FlushFn)(Display*);
    InternAtomFn pIntern = (InternAtomFn)dlsym(s_pX11, "XInternAtom");
    ChangePropertyFn pChange = (ChangePropertyFn)dlsym(s_pX11, "XChangeProperty");
    FlushFn pFlush = (FlushFn)dlsym(s_pX11, "XFlush");
    if (!pIntern || !pChange || !pFlush) return;
    Display* pDisplay = info.info.x11.display;
    int64_t nKeepComposited = 2;
    pChange(pDisplay, info.info.x11.window, pIntern(pDisplay, "_NET_WM_BYPASS_COMPOSITOR", False),
            XA_CARDINAL, 32, PropModeReplace, reinterpret_cast<const unsigned char*>(&nKeepComposited), 1);
    pFlush(pDisplay);
}
#else
void k2shim_keep_composited(SDL_Window*) {}
#endif

// Puts the game's icon on its windows, macOS's Dock icon (the executable is not an app bundle) and
// the window manager's on Linux, from resources/icons/window/<game>/window_icon.bmp compiled in by
// the embed step. Windows' windows have the executable's icon resource, which SDL registers its
// window class with; a phone's app has its own.
#if (defined(__APPLE__) && !TARGET_OS_IPHONE) || (defined(__linux__) && !defined(__ANDROID__))
extern "C" const unsigned char* k2embedded_find(const char* pszName, size_t* pnSize);

void k2shim_window_icon(SDL_Window* pWindow)
{
    size_t nSize = 0;
    const unsigned char* pData = k2embedded_find("window_icon.bmp", &nSize);
    if (!pWindow || !pData) return;
    if (SDL_Surface* pIcon = SDL_LoadBMP_RW(SDL_RWFromConstMem(pData, static_cast<int>(nSize)), 1)) {
        SDL_SetWindowIcon(pWindow, pIcon);
        SDL_FreeSurface(pIcon);
    }
}
#else
void k2shim_window_icon(SDL_Window*) {}
#endif
