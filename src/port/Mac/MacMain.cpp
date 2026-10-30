
#include "k2stdint.h"
#include "port/sdlapi.h"
#include "port/sdlconst.h"
#include "MacMain.h"

#include <map>
#include <string>
#include <jni.h>

extern "C" JNIEnv* Android_JNI_GetEnv();
int ASLPlat_GetControllerPlugged();
namespace ASL { namespace SDL { bool SetUseCoreProfile(bool bCore); } }

#include "port/libcbase.h"
#include "port/libcstdlib.h"

bool SWGuiGetAutoRun();
int  GetMiniGamePlayingType();
void GameGainedFocus();
void GameLostFocus();
void GameEnteredBackground();
extern char g_GamePauseNeeded;


int  gResetController_Mac;
#if !KOTOR1
int  gDontMoveCursor_Mac;
int  gUseRevComps_Mac;
bool gIosFocusLost;
bool gAndroidDidEnterBackgroundTriggered;
static char s_szOverridePath[256];
float ios_left_right;
float ios_controller_left_right;
float ios_gamepad_analog0X;
float ios_gamepad_analog0Y;
float ios_gamepad_analog1X;
float ios_gamepad_analog1Y;
bool  Ios_minigamePaused;
int   Ios_escPress;
int   Ios_spacePress;
int   Ios_forfeitPress;
#endif
bool  g_BackButtonEvent;
#if !KOTOR1
static std::map<int64_t, TrackedTouch> s_mapTrackedTouches;
static int   s_nTempDisableInteraction;
static float s_fIosTouchLR;
static float s_fIosMaxUD;
static float s_fIosMaxLR;
static bool  s_bAndroidWindowRevealed;
static float s_fAutoRunTouchUD;
static bool  s_bAutoRunTouchMoved;
static int64_t  s_nPrimaryTouchId = -1;
#endif

static const char* const s_apszLanguageFolders[7] = {
    "French", "German", "Italian", "English", "English", "English", "Spanish"
};

#if !KOTOR1
static void MakeOverridePath(char* pszUnusedBuffer, uint64_t nUnusedSize,
                             const char* pszFormat, ...)
{
    K2_UNUSED(pszUnusedBuffer); K2_UNUSED(nUnusedSize);
    __builtin_va_list ap;
    __builtin_va_start(ap, pszFormat);
    __vsprintf_chk(s_szOverridePath, 0, sizeof(s_szOverridePath), pszFormat, ap);
    __builtin_va_end(ap);
}

// The out-parameters are seeded with the tablet resolution the rest of the port scales against: the
// Android build is never an iPhone.
bool IosIsIphone(int& nScreenWidth, int& nScreenHeight)
{
    nScreenWidth = 1024;
    nScreenHeight = 768;
    return false;
}
#endif

// The localized asset folder for the current language. ASL language ids start at 3 (English), so the
// table index is nLanguage - 3; anything outside 3..9 falls back to English.
const char* GetLocalizationFolderName()
{
    int nIndex = ASL::GetLanguage() - 3;
    if (static_cast<unsigned int>(nIndex) <= 6)
        return s_apszLanguageFolders[nIndex];
    return "English";
}

#if !KOTOR1
// The five override roots, as (format, language) pairs:
//   0  user documents dlc\mods_<lang>   the mods folder for the current language
//   1  user documents dlc\mods_english  its English fallback
//   2  user documents dlc\<lang>        the DLC folder for the current language
//   3  game data localized\<lang>       the localized asset root
//   4  game data localized\English      its English fallback
// bFlag picks the index or its mirror (4 - nIndex), so a caller can ask for either end of the list
// by number; any other value returns null.
const char* GetOverridePath(int nIndex, bool bFlag)
{
    int nType = bFlag ? nIndex : 4 - nIndex;
    switch (nType) {
    case 0:
        MakeOverridePath(s_szOverridePath, sizeof(s_szOverridePath), "DOCUMENTS:dlc\\mods_%s",
                         GetLocalizationFolderName());
        return s_szOverridePath;
    case 1:
        MakeOverridePath(s_szOverridePath, sizeof(s_szOverridePath), "DOCUMENTS:dlc\\mods_%s", "english");
        return s_szOverridePath;
    case 2:
        MakeOverridePath(s_szOverridePath, sizeof(s_szOverridePath), "DOCUMENTS:dlc\\%s",
                         GetLocalizationFolderName());
        return s_szOverridePath;
    case 3:
        MakeOverridePath(s_szOverridePath, sizeof(s_szOverridePath), "HD0:localized\\%s",
                         GetLocalizationFolderName());
        return s_szOverridePath;
    case 4:
        MakeOverridePath(s_szOverridePath, sizeof(s_szOverridePath), "HD0:localized\\%s", "English");
        return s_szOverridePath;
    }
    return 0;
}

bool IosAnyTouches()
{
    return s_mapTrackedTouches.size() != 0;
}

void IosTemporaryDisableInteraction(int nDisable)
{
    s_nTempDisableInteraction = nDisable;
}

int IosGetTemporaryDisableInteraction()
{
    return s_nTempDisableInteraction;
}

// The mean of every tracked touch, normalized to the screen; 0 when nothing is tracked.
int IosGetRelativeTouch(float& fX, float& fY)
{
    uint64_t nCount = s_mapTrackedTouches.size();
    float fSumX = 0.0f;
    float fSumY = 0.0f;
    for (std::map<int64_t, TrackedTouch>::iterator it = s_mapTrackedTouches.begin();
         it != s_mapTrackedTouches.end(); ++it) {
        fSumX += static_cast<float>(it->second.m_nX);
        fSumY += static_cast<float>(it->second.m_nY);
    }
    if (nCount == 0)
        return 0;
    fX = fSumX / static_cast<float>(nCount) * (1.0f / 1024.0f);
    fY = fSumY / static_cast<float>(nCount) / 768.0f;
    return 1;
}

// One axis of the tilt/analog input, remapped to [-1, 1] through the dead zone. While the value
// stays saturated the raw input is recorded per axis, which is the calibration the dead zone is
// sized from.
float GetIosControl(bool bUpDown, float fScale, float fDeadZone, bool bAnalog)
{
    float fValue = bUpDown ? s_fIosTouchLR : ios_left_right;
    float fResult = fValue / fScale;
    if (__builtin_fabsf(fResult) >= 1.0f) {
        if (bUpDown) {
            if (s_fIosMaxUD < fScale)
                s_fIosMaxUD = fScale;
        } else {
            if (s_fIosMaxLR < fScale)
                s_fIosMaxLR = fScale;
        }
    }
    if (bAnalog) {
        if (ASLPlat_GetControllerPlugged() == 2)
            fResult += bUpDown ? ios_gamepad_analog0Y : ios_gamepad_analog0X;
        else
            fResult += bUpDown ? 0.0f : ios_controller_left_right;
    }
    float fOut = 0.0f;
    if (fResult > fDeadZone) {
        fOut = fResult - fDeadZone;
        if (fOut > 1.0f)
            fOut = 1.0f;
    } else if (fResult < -fDeadZone) {
        fOut = fResult + fDeadZone;
        if (fOut < -1.0f)
            fOut = -1.0f;
    }
    return fOut;
}

// The same axis read without the analog gate.
float GetIosControl(bool bUpDown, float fScale, float fDeadZone)
{
    float fValue = bUpDown ? s_fIosTouchLR : ios_left_right;
    float fResult = fValue / fScale;
    if (__builtin_fabsf(fResult) >= 1.0f) {
        if (bUpDown) {
            if (s_fIosMaxUD < fScale)
                s_fIosMaxUD = fScale;
        } else {
            if (s_fIosMaxLR < fScale)
                s_fIosMaxLR = fScale;
        }
    }
    int nPlugged = ASLPlat_GetControllerPlugged();
    float fPadY = (nPlugged == 2) ? ios_gamepad_analog0Y : 0.0f;
    float fPadX = (nPlugged == 2) ? ios_gamepad_analog0X : ios_controller_left_right;
    fResult += bUpDown ? fPadY : fPadX;
    float fOut = 0.0f;
    if (fResult > fDeadZone) {
        fOut = fResult - fDeadZone;
        if (fOut > 1.0f)
            fOut = 1.0f;
    } else if (fResult < -fDeadZone) {
        fOut = fResult + fDeadZone;
        if (fOut < -1.0f)
            fOut = -1.0f;
    }
    return fOut;
}

// Zeroes whichever source GetIosControl reads for that axis.
void ClearIosControl(bool bUpDown)
{
    float& rValue = bUpDown ? s_fIosTouchLR : ios_left_right;
    rValue = 0.0f;
}

// Re-shows the Android window after a movie or a background excursion, once.
void RevealAndroidWindowIfNeeded()
{
    if (s_bAndroidWindowRevealed)
        return;
    JNIEnv* pEnv = Android_JNI_GetEnv();
    jclass pClass = pEnv->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID pMid = pEnv->GetStaticMethodID(pClass, "showWindow", "()V");
    pEnv->CallStaticVoidMethod(pClass, pMid);
    pEnv->DeleteLocalRef(pClass);
    s_bAndroidWindowRevealed = true;
}
#endif


#if !KOTOR1
// SDL's event-watch filter, which latches the background state and never consumes the event.
int MainEventWatch(void* pUserData, SDL_Event* pEvent)
{
    K2_UNUSED(pUserData);
    if (pEvent->type == SDL_APP_DIDENTERBACKGROUND)
        gAndroidDidEnterBackgroundTriggered = 1;
    return 0;
}

namespace ASL {
class AppHooks {
public:
    virtual bool BeforeWinMain(std::string& sCommandLine) = 0;
    virtual void AfterWinMain() {}
    virtual int  BeforeExit(int nExitCode, bool bFlag) { K2_UNUSED(bFlag); return nExitCode; }
    virtual void BeforeParentChildLauncher() {}
    virtual bool PreProcessEvent(SDL_Event& event) = 0;
    virtual void BeforePumpEvents() {}
};
}

namespace {

// The app's hook override. SDL_main hands it to ASL::Main, whose event pump calls
// BeforePumpEvents and then PreProcessEvent for every event.
class Hooks : public ASL::AppHooks {
public:
    virtual bool BeforeWinMain(std::string& sCommandLine);
    virtual bool PreProcessEvent(SDL_Event& event);
};

Hooks sHooks;

// Forces the GL compatibility profile before WinMain runs; the command line is left alone.
bool Hooks::BeforeWinMain(std::string& sCommandLine)
{
    K2_UNUSED(sCommandLine);
    return ASL::SDL::SetUseCoreProfile(false);
}

// Every SDL event, before the engine's own translation; returning false drops the event.
//   - window events: focus lost runs GameLostFocus, plus the background notification when the event
//     watch saw the app backgrounded, and then blocks until focus is regained; focus gained runs
//     GameGainedFocus; showing the window re-reveals it once.
//   - the back button closes the soft keyboard when it is up, otherwise latches the back-button
//     event, and Esc inside a minigame.
//   - touches: each finger is tracked in screen space. The first finger down is the primary one,
//     and its drag from where it went down is the virtual stick GetIosControl reads, clamped so it
//     never travels past full deflection. With auto-run on, a vertical drag adds to the value held
//     when the finger went down, and a large enough move marks the touch as a drag so that lifting
//     it keeps running.
//   - a tap inside a minigame hits its on-screen buttons, laid out for 1024x768 and scaled by the
//     display height: Esc at the top left, the action button bottom left, and Forfeit at the top
//     right while the turret game is paused. A hit latches the matching press flag and consumes the
//     event.
//   - joystick removal pauses; a controller added or removed re-scans the controllers.
bool Hooks::PreProcessEvent(SDL_Event& event)
{
    while (gIosFocusLost) {
        SDL_Delay(10);
        SDL_PumpEvents();
    }
    SDL_DisplayMode mode;
    SDL_GetCurrentDisplayMode(0, &mode);
    float fScreenH = static_cast<float>(mode.h);
    float fScreenW = static_cast<float>(mode.w);

    int nTapY;
    int nTapX;
    int nMiniGame;
    switch (event.type) {
    case SDL_WINDOWEVENT:
        printf("SDL_WINDOWEVENT");
        switch (event.window.event) {
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            puts("SDL_WINDOWEVENT_FOCUS_GAINED");
            GameGainedFocus();
            return false;
        case SDL_WINDOWEVENT_FOCUS_LOST: {
            puts("SDL_WINDOWEVENT_FOCUS_LOST");
            GameLostFocus();
            if (gAndroidDidEnterBackgroundTriggered) {
                gAndroidDidEnterBackgroundTriggered = 0;
                GameEnteredBackground();
            }
            SDL_Event aEvents[64];
            char bGained;
            do {
                bGained = 0;
                int nCount = SDL_PeepEvents(aEvents, 64, SDL_PEEKEVENT, SDL_WINDOWEVENT, SDL_WINDOWEVENT);
                for (int i = 0; i < nCount; i++) {
                    if (aEvents[i].window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
                        bGained = 1;
                }
                SDL_Delay(10);
                SDL_PumpEvents();
            } while (!bGained);
            return false;
        }
        case SDL_WINDOWEVENT_ENTER:
            RevealAndroidWindowIfNeeded();
            break;
        default:
            printf("Unsupported SDL Window event type: %d", static_cast<int>(event.window.event));
            break;
        }
        return true;

    case SDL_KEYUP:
        if (event.key.keysym.sym == SDLK_AC_BACK) {
            if (SDL_IsTextInputActive()) {
                SDL_StopTextInput();
            } else {
                g_BackButtonEvent = true;
                if (GetMiniGamePlayingType())
                    Ios_escPress = 1;
            }
        }
        return true;

    case SDL_MOUSEBUTTONDOWN:
        nTapX = event.button.x;
        nTapY = event.button.y;
        nMiniGame = GetMiniGamePlayingType();
        break;

    case SDL_JOYDEVICEREMOVED:
        g_GamePauseNeeded = 1;
        return true;

    case SDL_CONTROLLERDEVICEADDED:
    case SDL_CONTROLLERDEVICEREMOVED:
        gResetController_Mac = 1;
        return true;

    case SDL_FINGERDOWN: {
        for (std::map<int64_t, TrackedTouch>::iterator it = s_mapTrackedTouches.begin();
             it != s_mapTrackedTouches.end(); ++it) {
        }
        if (SDL_IsTextInputActive())
            SDL_StopTextInput();
        int64_t nFingerId = event.tfinger.fingerId;
        if (s_nPrimaryTouchId == -1)
            s_nPrimaryTouchId = nFingerId;
        TrackedTouch touch;
        touch.m_nStartX = static_cast<int>(event.tfinger.x * 1024.0f);
        touch.m_nStartY = static_cast<int>(event.tfinger.y * 768.0f);
        touch.m_nX = touch.m_nStartX;
        touch.m_nY = touch.m_nStartY;
        touch.m_nTime = event.tfinger.timestamp;
        s_mapTrackedTouches.insert(std::make_pair(nFingerId, touch));
        if (SWGuiGetAutoRun() && GetMiniGamePlayingType() == 0) {
            s_bAutoRunTouchMoved = false;
            s_fAutoRunTouchUD = s_fIosTouchLR;
        } else {
            s_fIosTouchLR = 0.0f;
        }
        s_fIosMaxLR = 0.0f;
        ios_left_right = 0.0f;
        s_fIosMaxUD = 0.0f;
        nTapX = static_cast<int>(event.tfinger.x * fScreenW);
        nTapY = static_cast<int>(event.tfinger.y * fScreenH);
        nMiniGame = GetMiniGamePlayingType();
        break;
    }

    case SDL_FINGERUP: {
        int64_t nFingerId = event.tfinger.fingerId;
        std::map<int64_t, TrackedTouch>::iterator it = s_mapTrackedTouches.find(nFingerId);
        if (it == s_mapTrackedTouches.end())
            return true;
        s_mapTrackedTouches.erase(it);
        if (nFingerId != s_nPrimaryTouchId)
            return true;
        s_nPrimaryTouchId = -1;
        if (!SWGuiGetAutoRun() || GetMiniGamePlayingType() != 0)
            s_fIosTouchLR = 0.0f;
        if (SWGuiGetAutoRun() && !s_bAutoRunTouchMoved) {
            s_fIosTouchLR = 0.0f;
            s_fAutoRunTouchUD = 0.0f;
        }
        ios_left_right = 0.0f;
        return true;
    }

    case SDL_FINGERMOTION: {
        int nX = static_cast<int>(event.tfinger.x * 1024.0f);
        int nY = static_cast<int>(event.tfinger.y * 768.0f);
        int64_t nFingerId = event.tfinger.fingerId;
        std::map<int64_t, TrackedTouch>::iterator it = s_mapTrackedTouches.find(nFingerId);
        if (it == s_mapTrackedTouches.end())
            return true;
        TrackedTouch& touch = it->second;
        touch.m_nX = nX;
        touch.m_nY = nY;
        if (nFingerId != s_nPrimaryTouchId)
            return true;
        if (s_fIosMaxLR != 0.0f) {
            int nDelta = nX - touch.m_nStartX;
            if (__builtin_fabsf(static_cast<float>(nDelta)) > s_fIosMaxLR)
                touch.m_nStartX = static_cast<int>(static_cast<float>(nX) - s_fIosMaxLR * (nDelta < 0 ? -1.0f : 1.0f));
        }
        if (s_fIosMaxUD != 0.0f) {
            int nDelta = nY - touch.m_nStartY;
            if (__builtin_fabsf(static_cast<float>(nDelta)) > s_fIosMaxUD)
                touch.m_nStartY = static_cast<int>(static_cast<float>(nY) - s_fIosMaxUD * (nDelta < 0 ? -1.0f : 1.0f));
        }
        if (SWGuiGetAutoRun() && GetMiniGamePlayingType() == 0) {
            float fLimit = s_fIosMaxUD != 0.0f ? s_fIosMaxUD : 150.0f;
            int nDX = nX - touch.m_nStartX;
            int nDY = nY - touch.m_nStartY;
            ios_left_right = static_cast<float>(nDX);
            s_fIosTouchLR = __builtin_fmaxf(-fLimit, __builtin_fminf(fLimit, s_fAutoRunTouchUD + static_cast<float>(nDY)));
            if (__builtin_fabsf(static_cast<float>(nDX)) > 15.0f || __builtin_fabsf(static_cast<float>(nDY)) > 15.0f)
                s_bAutoRunTouchMoved = true;
            return true;
        }
        s_fIosTouchLR = static_cast<float>(nY - touch.m_nStartY);
        ios_left_right = static_cast<float>(nX - touch.m_nStartX);
        return true;
    }

    default:
        return true;
    }

    if (nMiniGame == 0)
        return true;
    float fScale = fScreenH / 768.0f;
    float fCorner = fScale * 128.0f;
    if (fCorner > static_cast<float>(nTapY) && fCorner > static_cast<float>(nTapX)) {
        Ios_escPress = 1;
        return false;
    }
    if (nMiniGame == 2) {
        float fAction = fScale * 170.0f;
        if (fScreenH - fAction < static_cast<float>(nTapY) && fAction > static_cast<float>(nTapX)) {
            Ios_spacePress = 1;
            return false;
        }
        if (!Ios_minigamePaused)
            return true;
        if (fCorner > static_cast<float>(nTapY) && fScreenW - fCorner < static_cast<float>(nTapX)) {
            Ios_forfeitPress = 1;
            return false;
        }
        return true;
    }
    if (fScreenH - fCorner < static_cast<float>(nTapY) && fCorner > static_cast<float>(nTapX)) {
        Ios_spacePress = 1;
        return false;
    }
    return true;
}

}

struct HINSTANCE__;
extern "C" int WinMain(HINSTANCE__* hInstance, HINSTANCE__* hPrevInstance, char* pszCmdLine,
                       int nCmdShow);
void ASLAnalyticsTrackScreen(const char* pszScreen);
namespace ASL {
bool GetPrefAsBool(const char* pszKey, bool bDefault);
void RegisterStringFile(const std::string& sName);
int  Main(int argc, char** argv, AppHooks& hooks,
          int (*pfnWinMain)(HINSTANCE__*, HINSTANCE__*, char*, int));
namespace FsApi { namespace Obb { void initVfs(); } }
}

// The Android process entry sets up the input preferences, ASL's string table, the launch analytics
// ping, the background watch and the OBB file system, then enters ASL::Main, which owns the window
// and the GL context and runs WinMain.
extern "C" int SDL_main(int argc, char** argv)
{
    gDontMoveCursor_Mac = ASL::GetPrefAsBool("DontMoveCursor", true);
    gUseRevComps_Mac = ASL::GetPrefAsBool("UseRevComps", true);
    ASL::RegisterStringFile("Localizable");
    ASLAnalyticsTrackScreen("Launch");
    SDL_AddEventWatch(MainEventWatch, 0);
    ASL::FsApi::Obb::initVfs();
    return ASL::Main(argc, argv, sHooks, WinMain);
}
#else
#include <list>
#include <algorithm>
#include "exobase.h"
#include "exosound.h"
#include "exosoundinternal.h"
#include "port/k1obb.h"

extern "C" {
int __android_log_print(int nPrio, const char* pszTag, const char* pszFmt, ...);
int __android_log_write(int nPrio, const char* pszTag, const char* pszText);
}

enum {
    K1_SDL_GL_RED_SIZE = 0, K1_SDL_GL_GREEN_SIZE = 1, K1_SDL_GL_BLUE_SIZE = 2,
    K1_SDL_GL_DOUBLEBUFFER = 5, K1_SDL_GL_STENCIL_SIZE = 7, K1_SDL_GL_ACCELERATED_VISUAL = 15,
    K1_SDL_GL_CONTEXT_MAJOR_VERSION = 17, K1_SDL_GL_CONTEXT_PROFILE_MASK = 21,
    K1_SDL_GL_CONTEXT_PROFILE_ES = 4,
};
static const unsigned int K1_SDL_INIT_EVERYTHING = 0xf231;
static const int K1_SDL_WINDOW_FULLSCREEN_OPENGL = 3;
static const int K1_SDLK_AC_BACK = 0x4000010e;
static const int K1_ANDROID_LOG_ERROR = 6;

enum GamepadButton {
    GAMEPAD_A = 0x1, GAMEPAD_B = 0x2, GAMEPAD_X = 0x4, GAMEPAD_Y = 0x8,
    GAMEPAD_LB = 0x10, GAMEPAD_RB = 0x20, GAMEPAD_LT = 0x40, GAMEPAD_RT = 0x80,
    GAMEPAD_UP = 0x100, GAMEPAD_DOWN = 0x200, GAMEPAD_LEFT = 0x400, GAMEPAD_RIGHT = 0x800,
    GAMEPAD_PAUSE = 0x1000, GAMEPAD_RSTICK_UP = 0x2000, GAMEPAD_RSTICK_DOWN = 0x4000,
#ifdef K1_UI_XBOX
    GAMEPAD_START = 0x80000, GAMEPAD_BACK = 0x100000, GAMEPAD_LSCLICK = 0x200000,
    GAMEPAD_RSCLICK = 0x400000,
#endif
};

int BACKBUTTON_AS_GAMEPAD_PAUSE_ID = 255;
int ENTERBACKGROUND_AS_GAMEPAD_PAUSE_ID = 256;
std::map<int, GamepadButton> gamepadButtonById = {
    {0, GAMEPAD_A}, {1, GAMEPAD_B}, {2, GAMEPAD_X}, {3, GAMEPAD_Y},
    {9, GAMEPAD_LB}, {10, GAMEPAD_RB},
#ifdef K1_UI_XBOX
    {4, GAMEPAD_BACK}, {5, GAMEPAD_PAUSE}, {6, GAMEPAD_START},
    {7, GAMEPAD_LSCLICK}, {8, GAMEPAD_RSCLICK},
#else
    {5, GAMEPAD_PAUSE}, {7, GAMEPAD_PAUSE}, {8, GAMEPAD_PAUSE},
#endif
    {11, GAMEPAD_UP}, {12, GAMEPAD_DOWN}, {13, GAMEPAD_LEFT},
    {14, GAMEPAD_RIGHT},
    {BACKBUTTON_AS_GAMEPAD_PAUSE_ID, GAMEPAD_PAUSE},
    {ENTERBACKGROUND_AS_GAMEPAD_PAUSE_ID, GAMEPAD_PAUSE},
};
std::map<int, GamepadButton> gamepadButtonByHatId = {
    {1, GAMEPAD_UP}, {2, GAMEPAD_RIGHT}, {4, GAMEPAD_DOWN}, {8, GAMEPAD_LEFT},
};
unsigned int pressedGamepadButtons;
unsigned int pressedGamepadButtonsThisFrame;
int pressedKey;
std::list<SDL_Joystick*> joysticks;
unsigned long long ms_per_tick;
unsigned long long beforeFrame;
unsigned long long afterFrame;
unsigned long long frameTime;
static char s_bMouseDownPending;
#if defined(K2_QOL_ADVANCED_ROWS) && KOTOR1
extern bool g_bFrameRateLocked;   // Advanced Graphics' framerate lock (platforms/k1/k1sdl.cpp)
extern "C" void k2host_pace_frame(float fFps);   // its wait (platforms/shim/shim_time.cpp)
#endif

extern SDL_DisplayMode g_SDL_mode;
extern SDL_Window* g_SDL_Window;
extern SDL_Window* Android_Window;
extern int g_SDL_BufferGeometry_w;
extern int g_SDL_BufferGeometry_h;
extern volatile int resolutionCap;
#if KOTOR1
extern volatile bool g_done;
#else
extern bool g_done;
#endif
extern bool gamepadConnected;
extern int auto_run_active;
extern int g_RenderSkip;
extern int g_bDisableSound;
extern float g_AIUpdateTime;
extern float g_SwapTime;
extern CExoSound* g_pExoSound;
extern float IosDisableInitiateDialogTime;
extern float last_touch_x, last_touch_y, current_touch_x, current_touch_y;
extern float touch_x_tolerence, touch_y_tolerence, touch_x_maxRange, touch_y_maxRange;
extern float mouseCameraMovement;
extern float ios_gamepad_analog0X, ios_gamepad_analog0Y;
extern float ios_gamepad_analog1X, ios_gamepad_analog1Y;
extern int GAME_OPTION_DEFAULT_SHADOWS, GAME_OPTION_DEFAULT_GRASS;
extern int GAME_OPTION_DEFAULT_FRAMEBUFFER, GAME_OPTION_DEFAULT_SCREENRES;
extern int GAME_OPTION_DEFAULT_HIDEINGAMEGUI, SWGUI_NUM_FEEDBACK_OPTIONS;

void GameInit();
void GameUpdate();
void GameDeinit();
void GameMouseDown(int x, int y);
void GameMouseMove(bool bNoDrag, int x, int y);
void GameMouseUp(int x, int y);
void GameMouseWheel(int x, int y, int nDelta);
void GameClick(int x, int y);
#if defined(K2_UI_PC) || defined(K1_UI_XBOX)
void K1PcQueryExit();
void GameMouseDo(int x, int y, int nPhase, bool bLeft);
bool IsGUIInGameOn();
bool IosAnyMainPanelUp();
extern int g_bMouseCameraRotateToggle;
extern int mouseDeltaX;
extern int mouseDeltaY;
static bool s_bPcMouseLook;
static int s_nPcMouseX, s_nPcMouseY;

static void PcMousePos(int nWinX, int nWinY, int nWindowW, int nWindowH, int* pX, int* pY)
{
    *pX = s_nPcMouseX = static_cast<int>(static_cast<float>(nWinX) / static_cast<float>(nWindowW) * static_cast<float>(g_SDL_mode.w));
    *pY = s_nPcMouseY = static_cast<int>((1.0f - static_cast<float>(nWinY) / static_cast<float>(nWindowH)) * static_cast<float>(g_SDL_mode.h));
}

static void PcMouseLookUpdate()
{
    bool bLook = g_bMouseCameraRotateToggle != 0 && IsGUIInGameOn() && !IosAnyMainPanelUp() &&
                 GetMiniGamePlayingType() == 0;
    if (bLook != s_bPcMouseLook) {
        s_bPcMouseLook = bLook;
        SDL_SetRelativeMouseMode(bLook ? 1 : 0);
        mouseDeltaX = 0;
        mouseDeltaY = 0;
    }
}
#endif
#ifdef K1_UI_XBOX
// xbox UI: the mouse is a plain pointer (hover, the two buttons, the wheel) for the menus and
// panels, and right-drag looks (left + right walks) in the game as in pc. A pad press hides it and
// parks it off screen until it moves again (shim/shim_cursor.cpp).
bool k2shim_pointer_from_pad();
void k2shim_pointer_from_mouse();
bool k2shim_pad_activity(const SDL_Event* e);
enum { XBOX_POINTER_PARKED = -30000 };   // K2SHIM_POINTER_PARKED: off every edge, not the -1 "none"

extern int g_bMouseCameraRotateToggleIos;
extern int g_bLeftMouseDown;
extern "C" uint32_t SDL_GetMouseState(int* x, int* y);

// A press latches the camera to the mouse (left: PerformLButtonDownAction's latch and the left half
// of the two-button walk; right: mouse look) until its release arrives. The engine can drop a
// release (a dialog line's click cooldown, a load, a focus change), which would leave the right
// stick dead, the look captured or the walk running for the rest of the session; after half a
// second with the button up the release is sent again, off screen.
static void XboxReleaseLostPress()
{
    static int s_nLeft, s_nRight;
    const uint32_t nButtons = SDL_GetMouseState(0, 0);
    const bool bLeftStuck = (g_bMouseCameraRotateToggleIos || g_bLeftMouseDown) && !(nButtons & 1);   // SDL_BUTTON_LMASK
    const bool bRightStuck = g_bMouseCameraRotateToggle && !(nButtons & 4);                            // SDL_BUTTON_RMASK
    s_nLeft = bLeftStuck ? s_nLeft + 1 : 0;
    s_nRight = bRightStuck ? s_nRight + 1 : 0;
    if (s_nLeft >= 30) {
        s_nLeft = 0;
        GameMouseDo(XBOX_POINTER_PARKED, XBOX_POINTER_PARKED, 2, true);
    }
    if (s_nRight >= 30) {
        s_nRight = 0;
        GameMouseDo(XBOX_POINTER_PARKED, XBOX_POINTER_PARKED, 2, false);
    }
}
#endif
int  HandleWMCharMessage(unsigned short nChar);
bool IsGamePaused();
bool IsGUIInGameOn();
bool IosAnyMainPanelUp();
#ifdef K2_HOST_BUILD
bool K2HostModalPanelUp();
#endif
void IosUpdateDisableInteractionCount();
void ASLgl_Init();
void MacCreateBinkShaders();
void AurDisableVertexBufferObjects();
void ASLAnalyticsTrackScreen(const char* pszScreen);

// Opens every joystick from nIndex on that is not in the list yet, by instance id.
void OpenFirstJoystick(int nIndex)
{
    int nCount = SDL_NumJoysticks();
    for (int i = nIndex; i < nCount; i++) {
        SDL_Joystick* pJoystick = SDL_JoystickOpen(i);
        if (pJoystick) {
            if (std::find_if(joysticks.begin(), joysticks.end(),
                             [nId = SDL_JoystickInstanceID(pJoystick)](SDL_Joystick* p) {
                                 return p && SDL_JoystickInstanceID(p) == nId;
                             }) == joysticks.end())
                joysticks.push_back(pJoystick);
        }
    }
    gamepadConnected = joysticks.size() != 0;
}

// -1 closes them all; an instance id closes that one and re-scans. Either way the pad state is
// dropped.
void CloseFirstJoystick(int nId)
{
    if (nId == -1) {
        for (std::list<SDL_Joystick*>::iterator it = joysticks.begin(); it != joysticks.end(); ++it) {
            if (SDL_JoystickGetAttached(*it))
                SDL_JoystickClose(*it);
        }
        joysticks.clear();
        gamepadConnected = false;
    } else {
        std::list<SDL_Joystick*>::iterator it =
            std::find_if(joysticks.begin(), joysticks.end(), [nId](SDL_Joystick* p) {
                return p && SDL_JoystickInstanceID(p) == nId;
            });
        if (it != joysticks.end()) {
            if (SDL_JoystickGetAttached(*it))
                SDL_JoystickClose(*it);
            joysticks.erase(it);
        }
        OpenFirstJoystick(0);
    }
    ios_gamepad_analog0X = 0.0f;
    ios_gamepad_analog0Y = 0.0f;
    ios_gamepad_analog1X = 0.0f;
    ios_gamepad_analog1Y = 0.0f;
    mouseCameraMovement = 0.0f;
    pressedGamepadButtons = 0;
    pressedGamepadButtonsThisFrame = 0;
}

// While every button of nCombo is held, report nReplacement instead of the combo's own bits.
void ReplaceGamepadInputForCombo(unsigned int nCombo, unsigned int nReplacement)
{
    if ((pressedGamepadButtons & nCombo) == nCombo) {
        pressedGamepadButtons |= nReplacement;
        pressedGamepadButtonsThisFrame = (pressedGamepadButtonsThisFrame & ~nCombo) | nReplacement;
    } else {
        pressedGamepadButtons &= ~nReplacement;
    }
}

static inline void AxisAsButtons(short nValue, unsigned int nPlus, unsigned int nMinus)
{
    if (nValue > 10000) {
        if (!(pressedGamepadButtons & nPlus))
            pressedGamepadButtonsThisFrame |= nPlus;
    } else if (nValue < -10000) {
        if (!(pressedGamepadButtons & nMinus))
            pressedGamepadButtonsThisFrame |= nMinus;
    } else {
        pressedGamepadButtons &= ~(nPlus | nMinus);
    }
}

static inline float AxisValue(short nValue)
{
    return __builtin_abs(nValue) >= 10000 ? static_cast<float>(nValue) / 32767.0f : 0.0f;
}

// K1 has no ASL::Main: here SDL_main is the Android app itself. It sets the device defaults, brings
// SDL, the window and the GL context up, waits for both OBBs, and then runs WinMain's three phases
// around its own event loop, which turns touches, the mouse, keys and the pad into the engine's
// mouse calls and the pressed-gamepad-button latch.
extern "C" int SDL_main(int argc, char** argv)
{
    K2_UNUSED(argc); K2_UNUSED(argv);
    ms_per_tick = SDL_GetPerformanceFrequency() / 1000;
    AurDisableVertexBufferObjects();

    JNIEnv* env = Android_JNI_GetEnv();
    jclass clsBuild = env->FindClass("android/os/Build");
    jfieldID fid = env->GetStaticFieldID(clsBuild, "MANUFACTURER", "Ljava/lang/String;");
    jstring str = static_cast<jstring>(env->GetStaticObjectField(clsBuild, fid));
    int nLen = env->GetStringUTFLength(str);
    char* pszValue = new char[nLen + 1];
    env->GetStringUTFRegion(str, 0, nLen, pszValue);
    env->DeleteLocalRef(str);
    pszValue[nLen] = 0;
    if (strcasecmp(pszValue, "NVIDIA") == 0) {
        fid = env->GetStaticFieldID(clsBuild, "MODEL", "Ljava/lang/String;");
        str = static_cast<jstring>(env->GetStaticObjectField(clsBuild, fid));
        nLen = env->GetStringUTFLength(str);
        delete[] pszValue;
        pszValue = new char[nLen + 1];
        env->GetStringUTFRegion(str, 0, nLen, pszValue);
        env->DeleteLocalRef(str);
        pszValue[nLen] = 0;
        if (strcasestr(pszValue, "SHIELD")) {
            GAME_OPTION_DEFAULT_SHADOWS = 1;
            GAME_OPTION_DEFAULT_GRASS = 1;
            GAME_OPTION_DEFAULT_FRAMEBUFFER = 1;
            GAME_OPTION_DEFAULT_SCREENRES = 2;
        }
    }
    delete[] pszValue;
    env->DeleteLocalRef(clsBuild);
#ifndef K2_MOBILE_OPT
    GAME_OPTION_DEFAULT_SHADOWS = 1;
    GAME_OPTION_DEFAULT_GRASS = 1;
    GAME_OPTION_DEFAULT_FRAMEBUFFER = 1;
#endif

    jclass cls = env->FindClass("org/libsdl/app/SDLActivity");
    jmethodID mid = env->GetStaticMethodID(cls, "getContext", "()Landroid/content/Context;");
    jobject context = env->CallStaticObjectMethod(cls, mid);
    env->DeleteLocalRef(cls);

    cls = env->FindClass("android/app/Activity");
    mid = env->GetMethodID(cls, "getWindowManager", "()Landroid/view/WindowManager;");
    env->DeleteLocalRef(cls);
    jobject windowManager = env->CallObjectMethod(context, mid);
    env->DeleteLocalRef(context);

    cls = env->FindClass("android/view/WindowManager");
    mid = env->GetMethodID(cls, "getDefaultDisplay", "()Landroid/view/Display;");
    env->DeleteLocalRef(cls);
    jobject display = env->CallObjectMethod(windowManager, mid);
    env->DeleteLocalRef(windowManager);

    cls = env->FindClass("android/util/DisplayMetrics");
    mid = env->GetMethodID(cls, "<init>", "()V");
    jobject metrics = env->NewObject(cls, mid);

    jclass clsDisplay = env->FindClass("android/view/Display");
    jmethodID midGetMetrics =
        env->GetMethodID(clsDisplay, "getMetrics", "(Landroid/util/DisplayMetrics;)V");
    env->DeleteLocalRef(clsDisplay);
    env->CallVoidMethod(display, midGetMetrics, metrics);
    env->DeleteLocalRef(display);

    jfieldID fidXdpi = env->GetFieldID(cls, "xdpi", "F");
    jfieldID fidYdpi = env->GetFieldID(cls, "ydpi", "F");
    env->DeleteLocalRef(cls);
    const float fXdpi = env->GetFloatField(metrics, fidXdpi);
    const float fYdpi = env->GetFloatField(metrics, fidYdpi);
    env->DeleteLocalRef(metrics);

    JNIEnv* pEnv = Android_JNI_GetEnv();
    jclass clsKotor = pEnv->FindClass("com/aspyr/kotor/KOTOR");
    jmethodID midQuery = pEnv->GetStaticMethodID(clsKotor, "GetHighResolution", "()Z");
    jboolean bHighRes = pEnv->CallStaticBooleanMethod(clsKotor, midQuery);
    pEnv->DeleteLocalRef(clsKotor);

    pEnv = Android_JNI_GetEnv();
    clsKotor = pEnv->FindClass("com/aspyr/kotor/KOTOR");
    midQuery = pEnv->GetStaticMethodID(clsKotor, "GetScreenHeightPixel", "()I");
    jint nPixelHeight = pEnv->CallStaticIntMethod(clsKotor, midQuery);
    pEnv->DeleteLocalRef(clsKotor);

    pEnv = Android_JNI_GetEnv();
    clsKotor = pEnv->FindClass("com/aspyr/kotor/KOTOR");
    midQuery = pEnv->GetStaticMethodID(clsKotor, "GetScreenHeightInch", "()F");
    jfloat fScreenHeightInch = pEnv->CallStaticFloatMethod(clsKotor, midQuery);
    pEnv->DeleteLocalRef(clsKotor);

    pEnv = Android_JNI_GetEnv();
    clsKotor = pEnv->FindClass("com/aspyr/kotor/KOTOR");
    midQuery = pEnv->GetStaticMethodID(clsKotor, "HasTouchScreen", "()Z");
    jboolean bHasTouch = pEnv->CallStaticBooleanMethod(clsKotor, midQuery);
    pEnv->DeleteLocalRef(clsKotor);
    int bChromebook = SDL_IsChromebook();
    if (!(bHasTouch != 0 || bChromebook != 0)) {
        GAME_OPTION_DEFAULT_HIDEINGAMEGUI = 1;
        SWGUI_NUM_FEEDBACK_OPTIONS = 6;
    }

    if (bHighRes != 0) {
        resolutionCap = nPixelHeight;
    } else {
        resolutionCap = fScreenHeightInch < 3.0f ? 640 : 720;
        if (resolutionCap >= nPixelHeight)
            resolutionCap = 480;
    }

    SDL_SetHint("SDL_ACCELEROMETER_AS_JOYSTICK", "0");
    SDL_SetHint("SDL_RETURN_KEY_HIDES_IME", "1");
    if (SDL_IsChromebook())
        SDL_SetHint("SDL_ANDROID_SEPARATE_MOUSE_AND_TOUCH", "1");

    if (SDL_Init(K1_SDL_INIT_EVERYTHING) != 0) {
        __android_log_print(K1_ANDROID_LOG_ERROR, "KOTOR", "SDL_Init Error: %s", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(K1_SDL_GL_CONTEXT_PROFILE_MASK, K1_SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(K1_SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(K1_SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(K1_SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(K1_SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(K1_SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(K1_SDL_GL_GREEN_SIZE, 6);
    SDL_GL_SetAttribute(K1_SDL_GL_BLUE_SIZE, 5);

    if (SDL_GetDisplayMode(0, 0, &g_SDL_mode) != 0) {
        __android_log_print(K1_ANDROID_LOG_ERROR, "KOTOR", "SDL_GetDisplayMode Error: %s",
                            SDL_GetError());
        SDL_Quit();
        exit(1);
    }

    g_SDL_BufferGeometry_w = g_SDL_mode.w;
    g_SDL_BufferGeometry_h = g_SDL_mode.h;
    if (g_SDL_BufferGeometry_h > resolutionCap) {
        g_SDL_BufferGeometry_w = resolutionCap * g_SDL_BufferGeometry_w / g_SDL_BufferGeometry_h;
        g_SDL_BufferGeometry_h = resolutionCap;
    }
    g_SDL_Window = SDL_CreateWindow("KOTOR", 0, 0, g_SDL_mode.w, g_SDL_mode.h,
                                    K1_SDL_WINDOW_FULLSCREEN_OPENGL);
    g_SDL_mode.w = g_SDL_BufferGeometry_w;
    g_SDL_mode.h = g_SDL_BufferGeometry_h;
    if (!g_SDL_Window) {
        __android_log_print(K1_ANDROID_LOG_ERROR, "KOTOR", "SDL_CreateWindow Error: %s",
                            SDL_GetError());
        SDL_Quit();
        exit(1);
    }
    void* pContext = SDL_GL_CreateContext(g_SDL_Window);

    pEnv = Android_JNI_GetEnv();
    clsKotor = pEnv->FindClass("com/aspyr/kotor/KOTOR");
    midQuery = pEnv->GetStaticMethodID(clsKotor, "GetDisableSetSwapIntervalCall", "()Z");
    jboolean bNoSwapInterval = pEnv->CallStaticBooleanMethod(clsKotor, midQuery);
    pEnv->DeleteLocalRef(clsKotor);
    if (bNoSwapInterval == 0)
        SDL_GL_SetSwapInterval(0);

    ASLgl_Init();
    MacCreateBinkShaders();

    SDL_Event ev;
    while (!g_done && (!g_obbMounted || !g_patchObbMounted)) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT)
                g_done = true;
        }
        SDL_Delay(10);
    }
    if (!g_obbMounted && !g_patchObbMounted) {
        __android_log_write(K1_ANDROID_LOG_ERROR, "KOTOR", "Failed to load OBB file, aborting!");
        SDL_GL_DeleteContext(pContext);
        SDL_DestroyWindow(g_SDL_Window);
        SDL_Quit();
        exit(1);
    }

    GameInit();
    ASLAnalyticsTrackScreen("Main Menu");

    cls = env->FindClass("com/aspyr/kotor/KOTOR");
    mid = env->GetStaticMethodID(cls, "showWindow", "()V");
    env->CallStaticVoidMethod(cls, mid);
    env->DeleteLocalRef(cls);

    touch_x_tolerence = fXdpi * 20.0f / 160.0f / static_cast<float>(g_SDL_mode.w);
    touch_y_tolerence = fYdpi * 20.0f / 160.0f / static_cast<float>(g_SDL_mode.h);
    touch_x_maxRange = fXdpi * 120.0f / 160.0f / static_cast<float>(g_SDL_mode.w);
    touch_y_maxRange = fYdpi * 120.0f / 160.0f / static_cast<float>(g_SDL_mode.h);

    OpenFirstJoystick(0);
    g_done = false;

    unsigned int nLastTicks = SDL_GetTicks();
#if !defined(K2_UI_PC) && !defined(K1_UI_XBOX)
    int nMouseButton = -1;
#endif
    int64_t nFingerId = -1;
    int nDownX K2_UNSET;
    int nDownY K2_UNSET;
    while (!g_done) {
        beforeFrame = SDL_GetPerformanceCounter();
        unsigned int nTicks = SDL_GetTicks();
        if (IosDisableInitiateDialogTime > 0.0f)
            IosDisableInitiateDialogTime += static_cast<float>(nLastTicks - nTicks) / -1000.0f;
        nLastTicks = nTicks;
        IosUpdateDisableInteractionCount();
        pressedGamepadButtonsThisFrame = 0;
#ifdef K2_UI_PC
        if (SDL_IsChromebook())
            PcMouseLookUpdate();
#endif
#ifdef K1_UI_XBOX
        PcMouseLookUpdate();
        XboxReleaseLostPress();
#endif

        bool bProcessEvents = true;
        if (s_bMouseDownPending == 1) {
            s_bMouseDownPending = 0;
            bProcessEvents = false;
            if (IosGetTemporaryDisableInteraction() == 0)
                GameMouseDown(static_cast<int>(last_touch_x * static_cast<float>(g_SDL_mode.w)),
                              static_cast<int>((1.0f - last_touch_y) * static_cast<float>(g_SDL_mode.h)));
        }

        int nWindowW;
        int nWindowH;
        SDL_GetWindowSize(Android_Window, &nWindowW, &nWindowH);
        bool bLostFocus = false;
        while (SDL_PollEvent(&ev) && bProcessEvents) {
#ifdef K1_UI_XBOX
            if (k2shim_pad_activity(&ev) && k2shim_pointer_from_pad())
                GameMouseMove(true, XBOX_POINTER_PARKED, XBOX_POINTER_PARKED);
#endif
            switch (ev.type) {
#ifdef K2_UI_PC
            case SDL_QUIT:
                K1PcQueryExit();
                break;
#else
            case SDL_QUIT:
#endif
            case SDL_APP_TERMINATING:
                g_done = true;
                break;
            case SDL_APP_WILLENTERBACKGROUND:
                GameEnteredBackground();
                break;
            case SDL_APP_DIDENTERFOREGROUND: {
                JNIEnv* pEnvFg = Android_JNI_GetEnv();
                jclass clsFg = pEnvFg->FindClass("com/aspyr/kotor/KOTOR");
                jmethodID midFg =
                    pEnvFg->GetStaticMethodID(clsFg, "GetDisableSetSwapIntervalCall", "()Z");
                jboolean bNoSwap = pEnvFg->CallStaticBooleanMethod(clsFg, midFg);
                pEnvFg->DeleteLocalRef(clsFg);
                if (bNoSwap == 0)
                    SDL_GL_SetSwapInterval(0);
                break;
            }
            case SDL_WINDOWEVENT:
                if (ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                    GameLostFocus();
                    bLostFocus = true;
                } else if (ev.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                    GameGainedFocus();
                }
                break;

            case SDL_KEYDOWN: {
                int nScancode = ev.key.keysym.scancode;
                if (!SDL_IsChromebook()) {
                    switch (nScancode) {
                    case SDL_SCANCODE_RETURN:
                    case SDL_SCANCODE_RETURN2:
                        SDL_StopTextInput();
                        break;
                    case SDL_SCANCODE_BACKSPACE:
                    case SDL_SCANCODE_KP_BACKSPACE:
                        HandleWMCharMessage(8);
                        break;
                    case SDL_SCANCODE_AUDIOSTOP:
                    case SDL_SCANCODE_AUDIOPLAY:
                        pressedGamepadButtonsThisFrame |= GAMEPAD_PAUSE;
                        break;
                    }
                } else {
                    bool bMenu = (!IsGUIInGameOn() || IosAnyMainPanelUp()) &&
                                 GetMiniGamePlayingType() == 0;
                    switch (nScancode) {
                    case SDL_SCANCODE_BACKSPACE:
                    case SDL_SCANCODE_KP_BACKSPACE:
                        HandleWMCharMessage(8);
                        break;
                    case SDL_SCANCODE_RIGHT:
                        if (bMenu)
                            pressedKey |= nScancode;
                        else
                            ios_gamepad_analog0X = 1.0f;
                        break;
                    case SDL_SCANCODE_LEFT:
                        if (bMenu)
                            pressedKey |= nScancode;
                        else
                            ios_gamepad_analog0X = -1.0f;
                        break;
                    case SDL_SCANCODE_DOWN:
                        if (bMenu)
                            pressedKey |= nScancode;
                        else
                            ios_gamepad_analog0Y = 1.0f;
                        break;
                    case SDL_SCANCODE_UP:
                        if (bMenu)
                            pressedKey |= nScancode;
                        else
                            ios_gamepad_analog0Y = -1.0f;
                        break;
                    default:
                        pressedKey |= nScancode;
                        break;
                    }
                }
                if (ev.key.keysym.sym == K1_SDLK_AC_BACK)
                    pressedGamepadButtonsThisFrame |=
                        gamepadButtonById[BACKBUTTON_AS_GAMEPAD_PAUSE_ID];
                break;
            }

            case SDL_KEYUP: {
                int nScancode = ev.key.keysym.scancode;
                ios_gamepad_analog0X = 0.0f;
                ios_gamepad_analog0Y = 0.0f;
                if (nScancode == SDL_SCANCODE_AUDIOSTOP || nScancode == SDL_SCANCODE_AUDIOPLAY) {
                    pressedGamepadButtons &= ~GAMEPAD_PAUSE;
                    pressedGamepadButtonsThisFrame &= ~GAMEPAD_PAUSE;
                }
                if (ev.key.keysym.sym == K1_SDLK_AC_BACK) {
                    if (SDL_IsScreenKeyboardShown(g_SDL_Window) && !SDL_IsChromebook()) {
                        SDL_StopTextInput();
                    } else {
                        g_BackButtonEvent = true;
                        pressedGamepadButtonsThisFrame &=
                            ~gamepadButtonById[BACKBUTTON_AS_GAMEPAD_PAUSE_ID];
                        pressedGamepadButtons &= ~gamepadButtonById[BACKBUTTON_AS_GAMEPAD_PAUSE_ID];
                    }
                }
                break;
            }

            case SDL_TEXTINPUT:
                HandleWMCharMessage(ev.text.text[0]);
                break;

#ifdef K2_UI_PC
            case SDL_MOUSEMOTION:
                if (SDL_IsChromebook()) {
                    if (s_bPcMouseLook) {
                        mouseDeltaX += ev.motion.xrel;
                        mouseDeltaY += ev.motion.yrel;
                    } else {
                        int nX, nY;
                        PcMousePos(ev.motion.x, ev.motion.y, nWindowW, nWindowH, &nX, &nY);
                        GameMouseMove(true, nX, nY);
                    }
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                if (SDL_IsChromebook() &&
                    (ev.button.button == SDL_BUTTON_LEFT || ev.button.button == SDL_BUTTON_RIGHT)) {
                    int nX, nY;
                    PcMousePos(ev.button.x, ev.button.y, nWindowW, nWindowH, &nX, &nY);
                    auto_run_active = 0;
                    GameMouseDo(nX, nY, ev.type == SDL_MOUSEBUTTONDOWN ? 1 : 2, ev.button.button == SDL_BUTTON_LEFT);
                }
                break;

            case SDL_MOUSEWHEEL:
                if (SDL_IsChromebook()) {
                    GameMouseWheel(s_nPcMouseX, s_nPcMouseY, ev.wheel.y * 120);
                }
                break;
#elif defined(K1_UI_XBOX)
            case SDL_MOUSEMOTION: {
                k2shim_pointer_from_mouse();
                if (s_bPcMouseLook) {
                    mouseDeltaX += ev.motion.xrel;
                    mouseDeltaY += ev.motion.yrel;
                    break;
                }
                int nX, nY;
                PcMousePos(ev.motion.x, ev.motion.y, nWindowW, nWindowH, &nX, &nY);
                GameMouseMove(true, nX, nY);
                break;
            }

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                if (ev.button.button == SDL_BUTTON_LEFT || ev.button.button == SDL_BUTTON_RIGHT) {
                    k2shim_pointer_from_mouse();
                    int nX, nY;
                    PcMousePos(ev.button.x, ev.button.y, nWindowW, nWindowH, &nX, &nY);
                    GameMouseDo(nX, nY, ev.type == SDL_MOUSEBUTTONDOWN ? 1 : 2, ev.button.button == SDL_BUTTON_LEFT);
                }
                break;

            case SDL_MOUSEWHEEL: {
                int nX, nY;
                PcMousePos(ev.wheel.mouseX, ev.wheel.mouseY, nWindowW, nWindowH, &nX, &nY);
                GameMouseWheel(nX, nY, ev.wheel.y * 120);
                break;
            }
#else
            case SDL_MOUSEMOTION:
                if (SDL_IsChromebook() &&
                    strcmp(SDL_GetHint("SDL_ANDROID_SEPARATE_MOUSE_AND_TOUCH"), "1") == 0) {
                    float fX = static_cast<float>(ev.motion.x) / static_cast<float>(nWindowW);
                    float fY = 1.0f - static_cast<float>(ev.motion.y) / static_cast<float>(nWindowH);
                    mouseCameraMovement = 0.0f;
                    if ((!IsGUIInGameOn() || IosAnyMainPanelUp()) && GetMiniGamePlayingType() == 0) {
                        GameMouseMove(false, static_cast<int>(fX * static_cast<float>(g_SDL_mode.w)),
                                      static_cast<int>(fY * static_cast<float>(g_SDL_mode.h)));
                    } else if (IsGamePaused()) {
                        GameMouseMove(false, static_cast<int>(fX * static_cast<float>(g_SDL_mode.w)),
                                      static_cast<int>(fY * static_cast<float>(g_SDL_mode.h)));
                    } else {
                        auto_run_active = 0;
                        mouseCameraMovement = __builtin_fminf(
                            __builtin_fmaxf(static_cast<float>(ev.motion.xrel) / 80.0f, -1.0f), 1.0f);
                        if (__builtin_abs(ev.motion.xrel) <= 2) {
                            if (ev.motion.x + 2 == nWindowW)
                                mouseCameraMovement = 1.0f;
                            else if (ev.motion.x == 0)
                                mouseCameraMovement = -1.0f;
                            else
                                mouseCameraMovement = 0.0f;
                        }
                    }
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (SDL_IsChromebook() && nMouseButton == -1 &&
                    strcmp(SDL_GetHint("SDL_ANDROID_SEPARATE_MOUSE_AND_TOUCH"), "1") == 0) {
                    last_touch_x = static_cast<float>(ev.button.x) / static_cast<float>(nWindowW);
                    current_touch_y = static_cast<float>(ev.button.y) / static_cast<float>(nWindowH);
                    current_touch_x = last_touch_x;
                    last_touch_y = current_touch_y;
                    nMouseButton = ev.button.button;
                    auto_run_active = 0;
                    nDownX = static_cast<int>(last_touch_x * static_cast<float>(g_SDL_mode.w));
                    nDownY = static_cast<int>((1.0f - current_touch_y) * static_cast<float>(g_SDL_mode.h));
                    s_bMouseDownPending = 1;
                    GameMouseMove(true, nDownX, nDownY);
                    bProcessEvents = false;
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (SDL_IsChromebook() && ev.button.button == nMouseButton &&
                    strcmp(SDL_GetHint("SDL_ANDROID_SEPARATE_MOUSE_AND_TOUCH"), "1") == 0) {
                    float fX = static_cast<float>(ev.button.x) / static_cast<float>(nWindowW);
                    float fY = 1.0f - static_cast<float>(ev.button.y) / static_cast<float>(nWindowH);
                    last_touch_x = 0.0f;
                    last_touch_y = 0.0f;
                    int nUpX = static_cast<int>(fX * static_cast<float>(g_SDL_mode.w));
                    int nUpY = static_cast<int>(fY * static_cast<float>(g_SDL_mode.h));
                    current_touch_x = 0.0f;
                    current_touch_y = 0.0f;
                    GameMouseUp(nUpX, nUpY);
                    nMouseButton = -1;
                    if (static_cast<int>(__builtin_fabsf(static_cast<float>((nUpX - nDownX) * 160) / fXdpi)) < 14 &&
                        static_cast<int>(__builtin_fabsf(static_cast<float>((nUpY - nDownY) * 160) / fYdpi)) < 14) {
                        GameClick(nUpX, nUpY);
                        nMouseButton = -1;
                    }
                    bProcessEvents = false;
                }
                break;

            case SDL_MOUSEWHEEL:
                if (SDL_IsChromebook() &&
                    strcmp(SDL_GetHint("SDL_ANDROID_SEPARATE_MOUSE_AND_TOUCH"), "1") == 0) {
                    float fX = static_cast<float>(ev.wheel.mouseX) / static_cast<float>(nWindowW);
                    float fY = static_cast<float>(ev.wheel.mouseY) / static_cast<float>(nWindowH);
                    nDownX = static_cast<int>(fX * static_cast<float>(g_SDL_mode.w));
                    nDownY = static_cast<int>((1.0f - fY) * static_cast<float>(g_SDL_mode.h));
                    GameMouseWheel(nDownX, nDownY, ev.wheel.direction ? ev.wheel.x : ev.wheel.y);
                }
                break;
#endif

            case SDL_JOYAXISMOTION:
                switch (ev.jaxis.axis) {
                case 0:
                    if ((!IsGUIInGameOn() || IosAnyMainPanelUp()) && GetMiniGamePlayingType() == 0) {
                        AxisAsButtons(ev.jaxis.value, GAMEPAD_RIGHT, GAMEPAD_LEFT);
                        ios_gamepad_analog0X = 0.0f;
                    } else {
                        ios_gamepad_analog0X = AxisValue(ev.jaxis.value);
                    }
                    break;
                case 1:
                    if ((!IsGUIInGameOn() || IosAnyMainPanelUp()) && GetMiniGamePlayingType() == 0) {
                        AxisAsButtons(ev.jaxis.value, GAMEPAD_DOWN, GAMEPAD_UP);
                        ios_gamepad_analog0Y = 0.0f;
                    } else {
                        ios_gamepad_analog0Y = AxisValue(ev.jaxis.value);
                    }
                    break;
                case 2:
                    ios_gamepad_analog1X = AxisValue(ev.jaxis.value);
                    break;
                case 3:
#ifdef K2_HOST_BUILD
                    // A message box over the HUD (the combat tutorials) scrolls with the right stick.
                    if ((!IsGUIInGameOn() || IosAnyMainPanelUp() || K2HostModalPanelUp()) &&
                        GetMiniGamePlayingType() == 0) {
#else
                    if ((!IsGUIInGameOn() || IosAnyMainPanelUp()) && GetMiniGamePlayingType() == 0) {
#endif
                        AxisAsButtons(ev.jaxis.value, GAMEPAD_RSTICK_DOWN, GAMEPAD_RSTICK_UP);
                        ios_gamepad_analog1Y = 0.0f;
                    } else {
                        ios_gamepad_analog1Y = AxisValue(ev.jaxis.value);
                    }
                    break;
                case 4:
                    if (ev.jaxis.value > 0) {
                        if (!(pressedGamepadButtons & GAMEPAD_LT))
                            pressedGamepadButtonsThisFrame |= GAMEPAD_LT;
                    } else {
                        pressedGamepadButtons &= ~GAMEPAD_LT;
                    }
                    break;
                case 5:
                    if (ev.jaxis.value > 0) {
                        if (!(pressedGamepadButtons & GAMEPAD_RT))
                            pressedGamepadButtonsThisFrame |= GAMEPAD_RT;
                    } else {
                        pressedGamepadButtons &= ~GAMEPAD_RT;
                    }
                    break;
                }
                break;

            case SDL_JOYHATMOTION:
                if (gamepadButtonByHatId.count(ev.jhat.value)) {
                    pressedGamepadButtons &= ~(GAMEPAD_UP | GAMEPAD_DOWN | GAMEPAD_LEFT | GAMEPAD_RIGHT);
                    pressedGamepadButtonsThisFrame &=
                        ~(GAMEPAD_UP | GAMEPAD_DOWN | GAMEPAD_LEFT | GAMEPAD_RIGHT);
                    pressedGamepadButtonsThisFrame |= gamepadButtonByHatId[ev.jhat.value];
                }
                break;

            case SDL_JOYBUTTONDOWN:
                if (gamepadButtonById.count(ev.jbutton.button))
                    pressedGamepadButtonsThisFrame |= gamepadButtonById[ev.jbutton.button];
                break;

            case SDL_JOYBUTTONUP:
                if (gamepadButtonById.count(ev.jbutton.button)) {
                    if ((pressedGamepadButtons & gamepadButtonById[ev.jbutton.button]) == 0) {
                        pressedGamepadButtonsThisFrame |= gamepadButtonById[ev.jbutton.button];
                    } else {
                        pressedGamepadButtonsThisFrame &= ~gamepadButtonById[ev.jbutton.button];
                        pressedGamepadButtons &= ~gamepadButtonById[ev.jbutton.button];
                    }
                }
                break;

            case SDL_JOYDEVICEADDED:
                OpenFirstJoystick(ev.jdevice.which);
                break;
            case SDL_JOYDEVICEREMOVED:
                CloseFirstJoystick(ev.jdevice.which);
                break;

            case SDL_FINGERDOWN:
                if (nFingerId == -1) {
                    nFingerId = ev.tfinger.fingerId;
                    auto_run_active = 0;
                    last_touch_x = ev.tfinger.x;
                    current_touch_x = ev.tfinger.x;
                    last_touch_y = ev.tfinger.y;
                    current_touch_y = ev.tfinger.y;
                    nDownX = static_cast<int>(ev.tfinger.x * static_cast<float>(g_SDL_mode.w));
                    nDownY = static_cast<int>((1.0f - ev.tfinger.y) * static_cast<float>(g_SDL_mode.h));
                    s_bMouseDownPending = 1;
                    GameMouseMove(true, nDownX, nDownY);
                    bProcessEvents = false;
                }
                break;

            case SDL_FINGERUP:
                if (ev.tfinger.fingerId == nFingerId) {
                    if (SWGuiGetAutoRun())
                        auto_run_active = 1;
                    float fX = ev.tfinger.x * static_cast<float>(g_SDL_mode.w);
                    last_touch_x = 0.0f;
                    float fY = (1.0f - ev.tfinger.y) * static_cast<float>(g_SDL_mode.h);
                    last_touch_y = 0.0f;
                    int nUpX = static_cast<int>(fX);
                    int nUpY = static_cast<int>(fY);
                    current_touch_x = 0.0f;
                    current_touch_y = 0.0f;
                    GameMouseUp(nUpX, nUpY);
                    nFingerId = -1;
                    if (static_cast<int>(__builtin_fabsf(static_cast<float>((nUpX - nDownX) * 160) / fXdpi)) < 14 &&
                        static_cast<int>(__builtin_fabsf(static_cast<float>((nUpY - nDownY) * 160) / fYdpi)) < 14) {
                        GameClick(nUpX, nUpY);
                        nFingerId = -1;
                    }
                    bProcessEvents = false;
                }
                break;

            case SDL_FINGERMOTION:
                if (ev.tfinger.fingerId == nFingerId) {
                    current_touch_x = ev.tfinger.x;
                    current_touch_y = ev.tfinger.y;
                    GameMouseMove(false, static_cast<int>(ev.tfinger.x * static_cast<float>(g_SDL_mode.w)),
                                  static_cast<int>((1.0f - ev.tfinger.y) * static_cast<float>(g_SDL_mode.h)));
                }
                break;
            }
        }
        pressedGamepadButtons |= pressedGamepadButtonsThisFrame;
        if (bLostFocus && !IsGamePaused())
            pressedGamepadButtonsThisFrame |= gamepadButtonById[ENTERBACKGROUND_AS_GAMEPAD_PAUSE_ID];

        if (!g_done) {
            if (!g_bDisableSound)
                g_pExoSound->m_pInternal->m_audioSystem.UpdateSystem();
            if (g_RenderSkip != 0) {
                g_RenderSkip--;
            } else {
                if (g_AIUpdateTime > 133.0f)
                    g_RenderSkip = 10;
                else if (g_AIUpdateTime > 100.0f)
                    g_RenderSkip = 6;
                else if (g_AIUpdateTime > 67.0f)
                    g_RenderSkip = 3;
                else
                    g_RenderSkip = g_AIUpdateTime > 33.0f;
                g_AIUpdateTime = 0.0f;
            }
            GameUpdate();
            if (g_RenderSkip == 0) {
                uint64_ll_t nSwapStart = g_pExoBase->m_pTimers->GetHighResolutionTimer();
                SDL_GL_SwapWindow(g_SDL_Window);
                g_SwapTime = static_cast<float>(g_pExoBase->m_pTimers->GetHighResolutionTimer() - nSwapStart)
                             * 0.001f;
            }
        }
        if (g_RenderSkip == 0) {
            afterFrame = SDL_GetPerformanceCounter();
            frameTime = (afterFrame - beforeFrame) / ms_per_tick;
#if defined(K2_QOL_ADVANCED_ROWS) && KOTOR1
            // The framerate lock is KOTOR II's 60 fps, or no wait at all; the APK always held 40.
            k2host_pace_frame(g_bFrameRateLocked ? 60.0f : 0.0f);
#else
            if (frameTime < 25)
                SDL_Delay(25 - static_cast<unsigned int>(frameTime));
#endif
        }
    }

    CloseFirstJoystick(-1);
    GameDeinit();
    SDL_GL_DeleteContext(pContext);
    SDL_DestroyWindow(g_SDL_Window);
    SDL_Quit();
    return 0;
}
#endif

#if defined(K2_HOST_BUILD) && !KOTOR1
ASL::AppHooks* k2host_GetAppHooks()
{
    return &sHooks;
}
#elif defined(K2_HOST_BUILD)
// K1's SDL_main runs no ASL::Main, so there are no hooks to hand out.
namespace ASL { class AppHooks; }
ASL::AppHooks* k2host_GetAppHooks()
{
    return 0;
}
#endif
