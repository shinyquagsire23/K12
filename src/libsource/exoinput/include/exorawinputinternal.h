#ifndef CEXORAWINPUTINTERNAL_H
#define CEXORAWINPUTINTERNAL_H
#include "k2stdint.h"

#include "exostring.h"

// Per-port DirectInput-emulation device driver. Keyboard mode releases the exclusive raw-input grab
// so UI and OS text entry receive keystrokes, and re-acquires it when text entry ends.

struct HINSTANCE__;
struct HWND__;
struct DIDEVICEINSTANCEA;
// Only the first word, which GetKeyboardState clears, is recovered.
struct CExoKeyboardState {
    int m_nKeyStates;
};
struct CExoMouseState;
struct CExoDeviceBuffer;

class IDirectInputDeviceA;
class IDirectInputA {
public:
    virtual int64_t QueryInterface(const void* riid, void** ppv);
    virtual uint64_t AddRef();
    virtual uint64_t Release() noexcept;
    virtual int64_t CreateDevice(const void* rguid, IDirectInputDeviceA** ppDevice,
                              void* pOuterObject);
    virtual int64_t EnumDevices(unsigned int nDevType, void* pCallback, void* pRef,
                             unsigned int nFlags);
    virtual int64_t GetDeviceStatus(const void* rguid);
    virtual int64_t RunControlPanel(void* hWnd, unsigned int nFlags);
    virtual int64_t Initialize(void* hInst, unsigned int nVersion);
    virtual ~IDirectInputA();
};
class IDirectInputDeviceA {
public:
    virtual int64_t QueryInterface(const void* riid, void** ppv);
    virtual uint64_t AddRef();
    virtual uint64_t Release() noexcept;
    virtual int64_t GetCapabilities(void* pCaps);
    virtual int64_t EnumObjects(void* pCallback, void* pRef, unsigned int nFlags);
    // The engine passes the property id as a 32-bit DIPROP_* value, not a pointer.
    virtual int64_t GetProperty(unsigned int nPropId, void* pPropHeader);
    virtual int64_t SetProperty(unsigned int nPropId, const void* pPropHeader);
    virtual int64_t Acquire();
    virtual int64_t Unacquire() noexcept;
    virtual int64_t GetDeviceState(unsigned int nSize, void* pData);
    virtual int64_t GetDeviceData(unsigned int nObjSize, void* paData,
                               unsigned int* pnInOut, unsigned int nFlags);
    virtual int64_t SetDataFormat(const void* pFormat);
    virtual int64_t SetEventNotification(void* hEvent);
    virtual int64_t SetCooperativeLevel(void* hWnd, unsigned int nFlags);
    virtual int64_t GetObjectInfo(void* pObjInstance, unsigned int nObj,
                               unsigned int nHow);
    virtual int64_t GetDeviceInfo(void* pInfo);
    virtual int64_t RunControlPanel(void* hWnd, unsigned int nFlags);
    virtual int64_t Initialize(void* hInst, unsigned int nVersion, const void* rguid);
    virtual int64_t CreateEffect(const void* rguid, const void* pEffect, void** ppEffect,
                              void* pOuterObject);
    virtual int64_t EnumEffects(void* pCallback, void* pRef, unsigned int nType);
    virtual int64_t GetEffectInfo(void* pInfo, const void* rguid);
    virtual int64_t GetForceFeedbackState(unsigned int* pnState);
    virtual int64_t SendForceFeedbackCommand(unsigned int nCommand);
    virtual int64_t EnumCreatedEffectObjects(void* pCallback, void* pRef,
                                          unsigned int nFlags);
    virtual int64_t Escape(void* pEscape);
    virtual int64_t Poll();
    virtual int64_t SendDeviceData(unsigned int nObjSize, const void* paData,
                                unsigned int* pnInOut, unsigned int nFlags);
    virtual int64_t EnumEffectsInFile(const char* pFileName, void* pCallback, void* pRef,
                                   unsigned int nFlags);
    virtual int64_t WriteEffectToFile(const char* pFileName, unsigned int nEntries,
                                   void* paFileEffect, unsigned int nFlags);
    virtual ~IDirectInputDeviceA();
};

// Same shape as the Win32 type, but Data1 is 8 bytes (an unsigned long on LP64), which widens the struct.
struct GUID {
    uint64_t  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
};
struct DIDATAFORMAT {
    unsigned int dwSize;
    unsigned int dwObjSize;
    unsigned int dwFlags;
    unsigned int dwDataSize;
    unsigned int dwNumObjs;
    void*        rgodf;
};
extern const GUID IID_IDirectInput8A;
extern const GUID IID_IDirectInputDevice8A;
extern const GUID GUID_SysKeyboard;
extern const GUID GUID_SysMouse;
extern DIDATAFORMAT c_dfDIKeyboard;
extern DIDATAFORMAT c_dfDIMouse;
extern DIDATAFORMAT c_dfDIJoystick2;
extern bool s_isDeviceValid[6];

// DirectInput keyboard scan codes (dinput.h's DIK_*): a keyboard device's state index and the
// dwOfs of its buffered data.
enum DirectInputKey {
    DIK_ESCAPE = 0x01,
    DIK_1 = 0x02,
    DIK_2 = 0x03,
    DIK_3 = 0x04,
    DIK_4 = 0x05,
    DIK_5 = 0x06,
    DIK_6 = 0x07,
    DIK_7 = 0x08,
    DIK_8 = 0x09,
    DIK_9 = 0x0A,
    DIK_0 = 0x0B,
    DIK_MINUS = 0x0C,
    DIK_EQUALS = 0x0D,
    DIK_BACK = 0x0E,
    DIK_TAB = 0x0F,
    DIK_Q = 0x10,
    DIK_W = 0x11,
    DIK_E = 0x12,
    DIK_R = 0x13,
    DIK_T = 0x14,
    DIK_Y = 0x15,
    DIK_U = 0x16,
    DIK_I = 0x17,
    DIK_O = 0x18,
    DIK_P = 0x19,
    DIK_LBRACKET = 0x1A,
    DIK_RBRACKET = 0x1B,
    DIK_RETURN = 0x1C,
    DIK_LCONTROL = 0x1D,
    DIK_A = 0x1E,
    DIK_S = 0x1F,
    DIK_D = 0x20,
    DIK_F = 0x21,
    DIK_G = 0x22,
    DIK_H = 0x23,
    DIK_J = 0x24,
    DIK_K = 0x25,
    DIK_L = 0x26,
    DIK_SEMICOLON = 0x27,
    DIK_APOSTROPHE = 0x28,
    DIK_GRAVE = 0x29,
    DIK_LSHIFT = 0x2A,
    DIK_BACKSLASH = 0x2B,
    DIK_Z = 0x2C,
    DIK_X = 0x2D,
    DIK_C = 0x2E,
    DIK_V = 0x2F,
    DIK_B = 0x30,
    DIK_N = 0x31,
    DIK_M = 0x32,
    DIK_COMMA = 0x33,
    DIK_PERIOD = 0x34,
    DIK_SLASH = 0x35,
    DIK_RSHIFT = 0x36,
    DIK_MULTIPLY = 0x37,
    DIK_LMENU = 0x38,
    DIK_SPACE = 0x39,
    DIK_CAPITAL = 0x3A,
    DIK_F1 = 0x3B,
    DIK_F2 = 0x3C,
    DIK_F3 = 0x3D,
    DIK_F4 = 0x3E,
    DIK_F5 = 0x3F,
    DIK_F6 = 0x40,
    DIK_F7 = 0x41,
    DIK_F8 = 0x42,
    DIK_F9 = 0x43,
    DIK_F10 = 0x44,
    DIK_NUMPAD7 = 0x47,
    DIK_NUMPAD8 = 0x48,
    DIK_NUMPAD9 = 0x49,
    DIK_SUBTRACT = 0x4A,
    DIK_NUMPAD4 = 0x4B,
    DIK_NUMPAD5 = 0x4C,
    DIK_NUMPAD6 = 0x4D,
    DIK_ADD = 0x4E,
    DIK_NUMPAD1 = 0x4F,
    DIK_NUMPAD2 = 0x50,
    DIK_NUMPAD3 = 0x51,
    DIK_NUMPAD0 = 0x52,
    DIK_DECIMAL = 0x53,
    DIK_OEM_102 = 0x56,
    DIK_F11 = 0x57,
    DIK_F12 = 0x58,
    DIK_F13 = 0x64,
    DIK_F14 = 0x65,
    DIK_F15 = 0x66,
    DIK_NUMPADENTER = 0x9C,
    DIK_RCONTROL = 0x9D,
    DIK_NUMPADCOMMA = 0xB3,
    DIK_DIVIDE = 0xB5,
    DIK_SYSRQ = 0xB7,
    DIK_RMENU = 0xB8,
    DIK_PAUSE = 0xC5,
    DIK_HOME = 0xC7,
    DIK_UP = 0xC8,
    DIK_PRIOR = 0xC9,
    DIK_LEFT = 0xCB,
    DIK_RIGHT = 0xCD,
    DIK_END = 0xCF,
    DIK_DOWN = 0xD0,
    DIK_NEXT = 0xD1,
    DIK_INSERT = 0xD2,
    DIK_DELETE = 0xD3,
};

// DirectInput mouse state offsets (DIMOFS_*), which are also the mouse's control codes.
enum DirectInputMouseOffset {
    DIMOFS_X = 0,
    DIMOFS_Y = 4,
    DIMOFS_Z = 8,
    DIMOFS_BUTTON0 = 12,
    DIMOFS_BUTTON1 = 13,
    DIMOFS_BUTTON2 = 14,
};

// DirectInput joystick state offsets into a DIJOYSTATE2 (DIJOFS_*), which are also the joystick's
// control codes: the six axes, the two sliders, the four POV hats and the first 32 buttons.
enum DirectInputJoystickOffset {
    DIJOFS_X = 0,
    DIJOFS_Y = 4,
    DIJOFS_Z = 8,
    DIJOFS_RX = 12,
    DIJOFS_RY = 16,
    DIJOFS_RZ = 20,
    DIJOFS_SLIDER0 = 24,
    DIJOFS_SLIDER1 = 28,
    DIJOFS_POV0 = 32,
    DIJOFS_POV_END = 48,
    DIJOFS_BUTTON0 = 48,
    DIJOFS_BUTTON_END = 80,
};

// The port's control codes for the four directions of POV hat 0 (hat n adds n): GetJoystickBuffer
// reports a hat press as one of these, so a direction can be bound like a button.
enum JoystickPovDirectionCode {
    POVCODE_UP = 900,
    POVCODE_RIGHT = 904,
    POVCODE_DOWN = 908,
    POVCODE_LEFT = 912,
    POVCODE_COUNT = 16,
};

// EnumDevices callbacks (exorawinputwin32.cpp).
int CountJoysticksExt(const DIDEVICEINSTANCEA* pInstance, void* pContext);
int InitJoysticksExt(const DIDEVICEINSTANCEA* pInstance, void* pContext);

// One record in the joystick event buffer: device code, new value, then two cleared words.
struct CJoystickInputEvent {
    int                m_code;
    int                m_value;
    unsigned long long m_nTimeStampAndSequence;
    unsigned long long m_nAppData;
};

// Per-joystick axis and button snapshot, beginning with a raw joystick-state copy.
struct CExoJoystickState {
    int           m_lX;
    int           m_lY;
    int           m_lZ;
    int           m_lRx;
    int           m_lRy;
    int           m_lRz;
    int           m_lSlider0;
    unsigned char m_aJoyStateRest[0x110-0x1c];
    int           m_nLastUpdateMs;
    // Axis deadzone bounds, reset to {-32768, 32767}.
    int           m_axisBoundLo;
    int           m_axisBoundHi;
    unsigned char m_aReserved1[0x124-0x11c];
    int           m_axisBound2Lo;
    int           m_axisBound2Hi;
    unsigned char m_aReserved2[0x134-0x12c];
};

class CExoRawInputInternal {
public:
#if KOTOR1
    CExoRawInputInternal(void* hInst, void* hWnd);  // K1 takes untyped handles
#else
    CExoRawInputInternal(HINSTANCE__* hInst, HWND__* hWnd);
#endif
    ~CExoRawInputInternal();

    int    m_bInitialized;
    int    m_bActive;
    int    m_nKeyboardMode;
    int    m_bRumbleEnabled;
    int    m_bRumblePaused;
    int    m_nReserved1;
    int    m_nNumJoysticks;
    unsigned char m_nControllerTypeLo;
    unsigned char m_bIsDualShockLike;
    unsigned char m_aReserved2[0x20-0x1e];
    IDirectInputA*        m_pDirectInput;
    IDirectInputDeviceA*  m_pKeyboardDevice;
    IDirectInputDeviceA*  m_pMouseDevice;
    IDirectInputDeviceA** m_paJoystickDevices;
    CExoJoystickState* m_pJoystickState;
    CExoJoystickState* m_pLastJoystickState;

    int    GetPortBinding();
    int    GetControlThreshold(unsigned int);
    int    SetAutoCalInfo(unsigned int);
    int    GetMainDeviceLost();
    CExoJoystickState* GetJoystickState();
    int    GetAutoCalInfo();
    int    SetPortBinding(unsigned int);
    void   KeyboardModeOff();
    void   KeyboardModeOn();
    void   ShutDownDirectInputMouse();
    float  GetMinUseable(unsigned int, unsigned int);
    float  GetMaxUseable(unsigned int, unsigned int);
    int    GetKeyboardState(CExoKeyboardState&);
    int    IsDigital(unsigned int, unsigned int);
    int    GetMouseState(CExoMouseState&);
    void   ClearKeyboardBuffer();
    void   CheckControllerType(int);
    void   InitJoysticks(const DIDEVICEINSTANCEA*);
    int    InitializeDirectInputMouse();
    void   ShutDownDirectInput() noexcept;
    CExoString GetDisplayCharacterDirectInputValue(unsigned int);
    int    GetLastState(unsigned int, unsigned int);
    int    GetMouseBuffer(CExoDeviceBuffer&);
    void   SetActive(int);
    int    GetKeyboardBuffer(CExoDeviceBuffer&);
    void   ReinitJoysticks();
    int    GetJoystickBuffer(int, CExoDeviceBuffer&);
    void   ClearJoystickBuffer(int);
    void   ClearMouseBuffer();
};

#endif
