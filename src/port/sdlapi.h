#ifndef SDLAPI_H
#define SDLAPI_H

// The SDL2 surface the engine and its port layer call, declared once. The records spell only the
// members the tree reads, with padding that pins each one to SDL2's size; the host links all of it
// against the real SDL2 (K1's SDL_RWops stream is include/sdlrwops.h).
#include "k2stdint.h"

struct SDL_Window;
struct SDL_mutex;
struct SDL_cond;
struct _SDL_Joystick;
typedef _SDL_Joystick SDL_Joystick;

struct SDL_DisplayMode {
    unsigned int format;
    int          w;
    int          h;
    int          refresh_rate;
    void*        driverdata;
};

struct SDL_WindowEvent {
    unsigned int  type;
    unsigned int  timestamp;
    unsigned int  windowID;
    unsigned char event;
    unsigned char padding1, padding2, padding3;
    int           data1;
    int           data2;
};
struct SDL_Keysym {
    int            scancode;
    int            sym;
    unsigned short mod;
    unsigned int   unused;
};
struct SDL_KeyboardEvent {
    unsigned int  type;
    unsigned int  timestamp;
    unsigned int  windowID;
    unsigned char state, repeat, padding2, padding3;
    SDL_Keysym    keysym;
};
struct SDL_TextInputEvent {
    unsigned int type;
    unsigned int timestamp;
    unsigned int windowID;
    char         text[32];
};
struct SDL_MouseMotionEvent {
    unsigned int type;
    unsigned int timestamp;
    unsigned int windowID;
    unsigned int which;
    unsigned int state;
    int          x;
    int          y;
    int          xrel;
    int          yrel;
};
struct SDL_MouseButtonEvent {
    unsigned int  type;
    unsigned int  timestamp;
    unsigned int  windowID;
    unsigned int  which;
    unsigned char button, state, clicks, padding1;
    int           x;
    int           y;
};
// Aspyr's SDL2 put the pointer position before `direction`; stock SDL2 (2.26 on) appends preciseX/Y
// and the position after it.
struct SDL_MouseWheelEvent {
    unsigned int type;
    unsigned int timestamp;
    unsigned int windowID;
    unsigned int which;
    int          x;
    int          y;
#ifdef K2_HOST_BUILD
    unsigned int direction;
    float        preciseX;
    float        preciseY;
    int          mouseX;
    int          mouseY;
#else
    unsigned int mouseX;
    unsigned int mouseY;
    unsigned int direction;
#endif
};
struct SDL_JoyAxisEvent {
    unsigned int  type;
    unsigned int  timestamp;
    int           which;
    unsigned char axis, padding1, padding2, padding3;
    short         value;
};
struct SDL_JoyHatEvent {
    unsigned int  type;
    unsigned int  timestamp;
    int           which;
    unsigned char hat;
    unsigned char value;
};
struct SDL_JoyButtonEvent {
    unsigned int  type;
    unsigned int  timestamp;
    int           which;
    unsigned char button;
    unsigned char state;
};
struct SDL_JoyDeviceEvent {
    unsigned int type;
    unsigned int timestamp;
    int          which;
};
struct SDL_TouchFingerEvent {
    unsigned int type;
    unsigned int timestamp;
    int64_t      touchId;
    int64_t      fingerId;
    float        x;
    float        y;
    float        dx, dy, pressure;
};
struct SDL_UserEvent {
    unsigned int type;
    unsigned int timestamp;
    unsigned int windowID;
    int          code;
    void*        data1;
    void*        data2;
};
union SDL_Event {
    unsigned int         type;
    SDL_WindowEvent      window;
    SDL_KeyboardEvent    key;
    SDL_TextInputEvent   text;
    SDL_MouseMotionEvent motion;
    SDL_MouseButtonEvent button;
    SDL_MouseWheelEvent  wheel;
    SDL_JoyAxisEvent     jaxis;
    SDL_JoyHatEvent      jhat;
    SDL_JoyButtonEvent   jbutton;
    SDL_JoyDeviceEvent   jdevice;
    SDL_TouchFingerEvent tfinger;
    SDL_UserEvent        user;
    unsigned char        padding[56];
};
static_assert(sizeof(SDL_Event) == 56 && alignof(SDL_Event) == 8, "SDL2's SDL_Event");
static_assert(sizeof(SDL_DisplayMode) == 24, "SDL2's SDL_DisplayMode");
#ifdef K2_HOST_BUILD
static_assert(__builtin_offsetof(SDL_MouseWheelEvent, direction) == 24 &&
              __builtin_offsetof(SDL_MouseWheelEvent, mouseX) == 36, "stock SDL2's SDL_MouseWheelEvent");
#else
static_assert(__builtin_offsetof(SDL_MouseWheelEvent, mouseX) == 24 &&
              __builtin_offsetof(SDL_MouseWheelEvent, direction) == 32, "Aspyr's SDL_MouseWheelEvent");
#endif

extern "C" {
// Init, hints, errors
int         SDL_Init(unsigned int nFlags);
void        SDL_Quit();
int         SDL_SetHint(const char* pszName, const char* pszValue);
const char* SDL_GetHint(const char* pszName);
const char* SDL_GetError();

// Timers
unsigned int SDL_GetTicks();
uint64_t     SDL_GetPerformanceCounter();
uint64_t     SDL_GetPerformanceFrequency();
void         SDL_Delay(unsigned int nMs);

// Video and GL
int         SDL_GetCurrentDisplayMode(int nDisplay, SDL_DisplayMode* pMode);
int         SDL_GetDesktopDisplayMode(int nDisplay, SDL_DisplayMode* pMode);
int         SDL_GetDisplayMode(int nDisplay, int nMode, SDL_DisplayMode* pMode);
SDL_Window* SDL_CreateWindow(const char* pszTitle, int x, int y, int w, int h, unsigned int nFlags);
void        SDL_DestroyWindow(SDL_Window* pWindow);
void        SDL_GetWindowSize(SDL_Window* pWindow, int* pW, int* pH);
void        SDL_GetWindowPosition(SDL_Window* pWindow, int* pX, int* pY);
SDL_Window* SDL_GetKeyboardFocus();
int         SDL_GL_SetAttribute(int nAttr, int nValue);
void*       SDL_GL_CreateContext(SDL_Window* pWindow);
void        SDL_GL_DeleteContext(void* pContext);
int         SDL_GL_SetSwapInterval(int nInterval);
void        SDL_GL_SwapWindow(SDL_Window* pWindow);

// Events
void SDL_PumpEvents();
int  SDL_PollEvent(SDL_Event* pEvent);
int  SDL_PeepEvents(SDL_Event* pEvents, int nEvents, int nAction, unsigned int nMinType,
                    unsigned int nMaxType);
int  SDL_PushEvent(SDL_Event* pEvent);
int  SDL_AddEventWatch(int (*pfnFilter)(void*, SDL_Event*), void* pUserData);

// Mouse, keyboard, text input
int          SDL_SetRelativeMouseMode(int bEnabled);
int          SDL_GetRelativeMouseMode();
unsigned int SDL_GetGlobalMouseState(int* pX, int* pY);
void         SDL_StartTextInput();
void         SDL_StopTextInput();
int          SDL_IsTextInputActive();
int          SDL_IsScreenKeyboardShown(SDL_Window* pWindow);

// Joysticks
int           SDL_NumJoysticks();
SDL_Joystick* SDL_JoystickOpen(int nIndex);
void          SDL_JoystickClose(SDL_Joystick* pJoystick);
int           SDL_JoystickInstanceID(SDL_Joystick* pJoystick);
int           SDL_JoystickGetAttached(SDL_Joystick* pJoystick);

// Threads
SDL_mutex* SDL_CreateMutex();
void       SDL_DestroyMutex(SDL_mutex* pMutex);
int        SDL_LockMutex(SDL_mutex* pMutex);
int        SDL_UnlockMutex(SDL_mutex* pMutex);
SDL_cond*  SDL_CreateCond();
int        SDL_CondSignal(SDL_cond* pCond);
int        SDL_CondWait(SDL_cond* pCond, SDL_mutex* pMutex);

// Aspyr's Android SDL additions
int         SDL_IsChromebook();
int         SDL_IsTablet();
const char* SDL_AndroidGetExternalStoragePath();
}
// The display mode Aspyr's SDL glue fills (K2 defines it in AndroidJNI.cpp; K1 imports it).
extern SDL_DisplayMode g_SDL_mode;

#endif // SDLAPI_H
