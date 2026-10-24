// The per-port DirectInput-emulation device driver creates the keyboard, mouse and joystick
// devices, acquires and releases them, and turns their snapshots into input events.

#include "k2stdint.h"
#include "exorawinputinternal.h"

#include "exoinputinternal.h"
#include "exobase.h"
#include <stddef.h>

// The DirectInput 8 API values this driver passes and tests (dinput.h's names).
enum DirectInputResult : unsigned int {
    E_PENDING = 0x8000000A,
    DIERR_NOTBUFFERED = 0x80040207,
    DIERR_NOTACQUIRED = 0x8007000C,
    DIERR_NOTINITIALIZED = 0x80070015,
    DIERR_INPUTLOST = 0x8007001E,
    DIERR_INVALIDPARAM = 0x80070057,
};
enum {
    DI_OK = 0,
    DI_BUFFEROVERFLOW = 1,
    DIRECTINPUT_VERSION_8 = 0x0800,
    DIRECTINPUT_VERSION_3 = 0x0300,
    // SetCooperativeLevel flags.
    DISCL_NONEXCLUSIVE = 0x2,
    DISCL_FOREGROUND = 0x4,
    DISCL_BACKGROUND = 0x8,
    // Property ids, and how a property header names its object.
    DIPROP_BUFFERSIZE = 1,
    DIPROP_KEYNAME = 20,
    DIPH_DEVICE = 0,
    DIPH_BYOFFSET = 1,
    // EnumDevices: game controllers, attached ones only; GetCapabilities' attached flag.
    DI8DEVCLASS_GAMECTRL = 4,
    DIEDFL_ATTACHEDONLY = 1,
    DIDC_ATTACHED = 1,
    // Device-state sizes: the keyboard's key array, a DIJOYSTATE2, a DIDEVICEOBJECTDATA (LP64).
    KEYBOARD_STATE_SIZE = 256,
    DIJOYSTATE2_SIZE = 272,
    DIDEVICEOBJECTDATA_SIZE = 24,
    MAX_PATH = 260,
};

// The records a device buffer holds, which is also the DIPROP_BUFFERSIZE the devices are given.
enum { kDeviceBufferEntries = 256 };
// The sequence number stamped on a key event the port injects.
enum { kInjectedKeySequence = 282 };
// override/custom.txt's names for the keys DirectInput names poorly.
enum { CUSTOMSTRING_SPACEBAR = 14, CUSTOMSTRING_ESCAPE = 15, CUSTOMSTRING_CAPS_LOCK = 16 };
// A key name's buffer.
enum { kKeyNameChars = 80 };
// The joystick state words (offset / 4) GetLastState reads: X, Y, RX, RY, the sliders and POV 0.
enum {
    kLastStateWords = (1 << (DIJOFS_X / 4)) | (1 << (DIJOFS_Y / 4)) | (1 << (DIJOFS_RX / 4)) |
                      (1 << (DIJOFS_RY / 4)) | (1 << (DIJOFS_SLIDER0 / 4)) |
                      (1 << (DIJOFS_SLIDER1 / 4)) | (1 << (DIJOFS_POV0 / 4)),
};

bool s_isDeviceValid[6];

int CExoRawInputInternal::GetPortBinding()
{
    return 0;
}

int CExoRawInputInternal::GetControlThreshold(unsigned int)
{
    return 0;
}

int CExoRawInputInternal::SetAutoCalInfo(unsigned int)
{
    return 0;
}

int CExoRawInputInternal::GetMainDeviceLost()
{
    return 0;
}

int CExoRawInputInternal::GetAutoCalInfo()
{
    return 0;
}

int CExoRawInputInternal::SetPortBinding(unsigned int)
{
    return 0;
}

void CExoRawInputInternal::ClearJoystickBuffer(int)
{
}

void CExoRawInputInternal::ClearMouseBuffer()
{
}

#if !KOTOR1
CExoJoystickState* CExoRawInputInternal::GetJoystickState()
{
    return m_pJoystickState;
}
#endif

CExoRawInputInternal::~CExoRawInputInternal()
{
#if KOTOR1

#else
    ShutDownDirectInput();
#endif
}

    // Enters text-entry mode by dropping the exclusive grab so the OS can deliver keystrokes.
void CExoRawInputInternal::KeyboardModeOn()
{
#if KOTOR1
    m_nKeyboardMode = 1;
#else
    m_nKeyboardMode = 1;
    if (m_pKeyboardDevice != 0) {
        m_pKeyboardDevice->Unacquire();
    }
#endif
}

    // Returns to gameplay by re-acquiring for exclusive capture.
void CExoRawInputInternal::KeyboardModeOff()
{
#if KOTOR1
    m_nKeyboardMode = 0;
#else
    m_nKeyboardMode = 0;
    if (m_pKeyboardDevice != 0) {
        m_pKeyboardDevice->Acquire();
    }
#endif
}

void CExoRawInputInternal::ShutDownDirectInputMouse()
{
#if KOTOR1
#else
    if (m_pMouseDevice != 0) {
        m_pMouseDevice->Unacquire();
        m_pMouseDevice->Release();
        m_pMouseDevice = 0;
    }
#endif
}

struct DIPropDwordLocal {
    unsigned int dwSize;
    unsigned int dwHeaderSize;
    unsigned int dwObj;
    unsigned int dwHow;
    unsigned int dwData;
};

int CExoRawInputInternal::InitializeDirectInputMouse()
{
#if KOTOR1
    return 1;
#else

    int ok;
    int64_t result = m_pDirectInput->CreateDevice(&GUID_SysMouse, &m_pMouseDevice, 0);
    if (result != DI_OK) goto fail;

    result = m_pMouseDevice->SetDataFormat(&c_dfDIMouse);
    if (result != DI_OK) goto fail;

    result = m_pMouseDevice->SetCooperativeLevel(g_hRenderWnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
    if (result != DI_OK) goto fail;

    {
        DIPropDwordLocal prop;
        prop.dwSize = sizeof(prop);
        prop.dwHeaderSize = offsetof(DIPropDwordLocal, dwData);
        prop.dwObj = 0;
        prop.dwHow = DIPH_DEVICE;
        prop.dwData = kDeviceBufferEntries;
        result = m_pMouseDevice->SetProperty(DIPROP_BUFFERSIZE, &prop);
    }
    if (result != DI_OK) goto fail;

    result = m_pMouseDevice->Acquire();
    if (result != DI_OK) {
        ShutDownDirectInput();
    }
    ok = 1;
    goto done;

fail:
    ShutDownDirectInput();
    ok = 0;
done:
    return ok;
#endif
}

void CExoRawInputInternal::ShutDownDirectInput() noexcept
{
#if KOTOR1
#else
    if (m_pDirectInput != 0) {
        if (m_pKeyboardDevice != 0) {
            m_pKeyboardDevice->Unacquire();
            m_pKeyboardDevice->Release();
            m_pKeyboardDevice = 0;
        }
        if (m_pMouseDevice != 0) {
            m_pMouseDevice->Unacquire();
            m_pMouseDevice->Release();
            m_pMouseDevice = 0;
        }
        int n = m_nNumJoysticks;
        for (int i = 0; i < n; i++) {
            if (m_paJoystickDevices[i] != 0) {
                m_paJoystickDevices[i]->Unacquire();
                m_paJoystickDevices[i]->Release();
                m_paJoystickDevices[i] = 0;
                n = m_nNumJoysticks;
            }
        }
        if (m_pJoystickState != 0) {
            operator delete[](m_pJoystickState);
            m_pJoystickState = 0;
        }

        if (m_pLastJoystickState != 0) {
            operator delete[](m_pLastJoystickState);
        }
        if (m_paJoystickDevices != 0) {
            operator delete[](m_paJoystickDevices);
            m_paJoystickDevices = 0;
        }
        m_pDirectInput->Release();
        m_pDirectInput = 0;
    }
#endif
}

    // Per-axis usable range, indexed by the field's byte offset divided by four.
float CExoRawInputInternal::GetMinUseable(unsigned int deviceClass, unsigned int fieldOffset)
{
#if KOTOR1
    K2_UNUSED(deviceClass); K2_UNUSED(fieldOffset);
    return 0.0f;
#else
    if (deviceClass != static_cast<unsigned int>(CExoInput::INPUTDEVICE_KEYBOARD) &&
        deviceClass != static_cast<unsigned int>(CExoInput::INPUTDEVICE_MOUSE)) {
        switch ((fieldOffset >> 2) | (fieldOffset << 30)) {
            case 0: return -32768.0f;
            case 1: return -32768.0f;
            case 2: return 0.0f;
            case 3: return 0.0f;
            case 4: return 0.0f;
            case 5: return 0.0f;
            case 6: return -32768.0f;
            case 7: return -32768.0f;
        }
    }
    return 0.0f;
#endif
}

float CExoRawInputInternal::GetMaxUseable(unsigned int deviceClass, unsigned int fieldOffset)
{
#if KOTOR1
    K2_UNUSED(deviceClass); K2_UNUSED(fieldOffset);
    return 0.0f;
#else
    if (deviceClass != static_cast<unsigned int>(CExoInput::INPUTDEVICE_KEYBOARD) &&
        deviceClass != static_cast<unsigned int>(CExoInput::INPUTDEVICE_MOUSE)) {
        switch ((fieldOffset >> 2) | (fieldOffset << 30)) {
            case 0: return 32767.0f;
            case 1: return 32767.0f;
            case 2: return 0.0f;
            case 3: return 0.0f;
            case 4: return 0.0f;
            case 5: return 0.0f;
            case 6: return 32767.0f;
            case 7: return 32767.0f;
        }
    }
    return 0.0f;
#endif
}

int CExoRawInputInternal::IsDigital(unsigned int deviceClass, unsigned int fieldOffset)
{
#if KOTOR1
    K2_UNUSED(deviceClass); K2_UNUSED(fieldOffset);
    return 1;
#else
    if (deviceClass == static_cast<unsigned int>(CExoInput::INPUTDEVICE_KEYBOARD)) {
        return 1;
    }
    if (deviceClass == static_cast<unsigned int>(CExoInput::INPUTDEVICE_MOUSE)) {
        if ((fieldOffset & ~3u) == DIMOFS_BUTTON0) {
            goto isTrue;
        }
        return fieldOffset < 9;
    }
    if (CExoInput::INPUTDEVICE_JOYSTICKSTART <= static_cast<int>(deviceClass)) {
        if (fieldOffset == DIJOFS_POV0) {
            goto isTrue;
        }
        if (fieldOffset - DIJOFS_BUTTON0 < DIJOFS_BUTTON_END - DIJOFS_BUTTON0) {
            goto isTrue;
        }
        if (fieldOffset - POVCODE_UP < POVCODE_COUNT) {
            goto isTrue;
        }
        return fieldOffset == DIJOFS_RZ;
    }
    return 0;
isTrue:
    return 1;
#endif
}

int CExoRawInputInternal::GetLastState(unsigned int deviceClass, unsigned int fieldOffset)
{
#if KOTOR1
    K2_UNUSED(deviceClass); K2_UNUSED(fieldOffset);
    return 0;
#else
    if (deviceClass > 5) {
        return 0;
    }
    int result = 1;
    if (deviceClass != static_cast<unsigned int>(CExoInput::INPUTDEVICE_KEYBOARD) && deviceClass != static_cast<unsigned int>(CExoInput::INPUTDEVICE_MOUSE)) {
        if (CExoInput::INPUTDEVICE_JOYSTICKSTART <= static_cast<int>(deviceClass)) {
            result = 0;
            if (fieldOffset != DIJOFS_POV0 &&
                fieldOffset - DIJOFS_BUTTON0 >= DIJOFS_BUTTON_END - DIJOFS_BUTTON0) {
                unsigned int idx = (fieldOffset >> 2) | (fieldOffset << 30);
                result = 0;
                if (idx <= DIJOFS_POV0 / 4 && ((1 << idx) & kLastStateWords) != 0) {
                    result = 0;
                    if (idx < 8) {
                        CExoJoystickState* p = m_pLastJoystickState + (deviceClass - 2);
                        switch (idx) {
                        case 0: return p->m_lX;
                        case 1: return p->m_lY;
                        case 3: return p->m_lRx;
                        case 4: result = p->m_lRy; break;
                        case 6: return p->m_lSlider0;
                        case 7: return p->m_lRz;
                        }
                    }
                }
            }
        } else {
            return 0;
        }
    }
    return result;
#endif
}

int CExoRawInputInternal::GetKeyboardState(CExoKeyboardState& state)
{
#if KOTOR1

    if (m_pKeyboardDevice == 0) {
        return 0;
    }
    if (m_bActive == 0) {
        return 0;
    }
    if (m_nKeyboardMode == 0) {
        return 0;
    }
    state.m_nKeyStates = 0;
    return 1;
#else
    if (m_pKeyboardDevice == 0) {
        return 0;
    }
    if (m_bActive == 0) {
        return 0;
    }
    if (m_nKeyboardMode != 0) {
        state.m_nKeyStates = 0;
        goto trueRet;
    }
    {
        int64_t result = m_pKeyboardDevice->GetDeviceState(KEYBOARD_STATE_SIZE, &state);
        if (result == DI_OK) {
            goto trueRet;
        }
        if (result == DIERR_INPUTLOST) {
            int64_t r = m_pKeyboardDevice->Acquire();
            return r == DI_OK;
        }
    }
    return 0;
trueRet:
    return 1;
#endif
}

#if KOTOR1
extern int mouseDeltaX;
extern int mouseDeltaY;
extern int Ios_accumulatedDeltaX;
extern int Ios_accumulatedDeltaY;
#endif

#if KOTOR1

    // Drain an eighth of the accumulator, rounded away from zero, without crossing zero.
void ModifyAccumulatedDelta(int& nAccum, int& nDelta)
{
    if (nAccum > 0) {
        nDelta = (nAccum + 7) / 8;
        int n = nAccum - nDelta;
        nAccum = n > 0 ? n : 0;
    } else if (nAccum < 0) {
        nDelta = (nAccum - 7) / 8;
        int n = nAccum - nDelta;
        nAccum = n < 0 ? n : 0;
    } else {
        nDelta = 0;
    }
}

#endif
int CExoRawInputInternal::GetMouseState(CExoMouseState& state)
{
#if KOTOR1 && !defined(K2_UI_TOUCH)

    state.m_dx = mouseDeltaX;
    state.m_dy = mouseDeltaY;
    mouseDeltaX = 0;
    mouseDeltaY = 0;
    return m_bActive != 0;
#elif KOTOR1

    Ios_accumulatedDeltaX += mouseDeltaX;
    Ios_accumulatedDeltaY += mouseDeltaY;
    if (Ios_accumulatedDeltaX > 0) {
        int d = (Ios_accumulatedDeltaX + 7) / 8;
        state.m_dx = d;
        Ios_accumulatedDeltaX -= d;
        if (Ios_accumulatedDeltaX < 0) Ios_accumulatedDeltaX = 0;
    } else if (Ios_accumulatedDeltaX < 0) {
        int d = (Ios_accumulatedDeltaX - 7) / 8;
        state.m_dx = d;
        Ios_accumulatedDeltaX -= d;
        if (Ios_accumulatedDeltaX > 0) Ios_accumulatedDeltaX = 0;
    } else {
        state.m_dx = 0;
    }
    if (Ios_accumulatedDeltaY > 0) {
        int d = (Ios_accumulatedDeltaY + 7) / 8;
        state.m_dy = d;
        Ios_accumulatedDeltaY -= d;
        if (Ios_accumulatedDeltaY < 0) Ios_accumulatedDeltaY = 0;
    } else if (Ios_accumulatedDeltaY < 0) {
        int d = (Ios_accumulatedDeltaY - 7) / 8;
        state.m_dy = d;
        Ios_accumulatedDeltaY -= d;
        if (Ios_accumulatedDeltaY > 0) Ios_accumulatedDeltaY = 0;
    } else {
        state.m_dy = 0;
    }
    mouseDeltaX = 0;
    mouseDeltaY = 0;
    return m_bActive != 0;
#else
    if (m_pMouseDevice == 0) {
        InitializeDirectInputMouse();
        if (m_pMouseDevice == 0) {
            return 0;
        }
    }
    if (m_bActive == 0) {
        return 0;
    }
    int64_t result = m_pMouseDevice->GetDeviceState(sizeof(state), &state);
    if (result == DI_OK) {
        return 1;
    }
    if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) {
        int64_t r = m_pMouseDevice->Acquire();
        return r == DI_OK;
    }
    return 0;
#endif
}

extern int Ios_escPress;
extern int Ios_spacePress;
extern int Ios_forfeitPress;

int CExoRawInputInternal::GetMouseBuffer(CExoDeviceBuffer& buffer)
{
#if KOTOR1
    if (m_pMouseDevice == 0) {
        if (buffer.m_pEntries != 0) {
            operator delete[](buffer.m_pEntries);
            buffer.m_pEntries = 0;
        }
        buffer.m_nCount = 0;
    }
    return 0;
#else

    if (m_pMouseDevice == 0) {
        if (buffer.m_pEntries != 0) {
            operator delete[](buffer.m_pEntries);
            buffer.m_pEntries = 0;
        }
        buffer.m_nCount = 0;
        return 0;
    }
    if (m_bActive == 0) {
        return 0;
    }
    if (buffer.m_pEntries != 0) {
        operator delete[](buffer.m_pEntries);
        buffer.m_pEntries = 0;
    }
    buffer.m_nCount = 0;
    CExoInputKeyEvent* pEntries = static_cast<CExoInputKeyEvent*>(operator new[](kDeviceBufferEntries * DIDEVICEOBJECTDATA_SIZE));
    __builtin_memset(pEntries, 0, kDeviceBufferEntries);  // idiom-exempt: orig-engine partial clear (0x100 BYTES of the 0x1800-byte CExoInputKeyEvent[0x100] buffer -- the DI buffer element-count constant reused as a byte count, so no sizeof honestly names it; size pinned by byte-match)
    buffer.m_pEntries = pEntries;
    unsigned int count = kDeviceBufferEntries;
    int64_t result = m_pMouseDevice->GetDeviceData(DIDEVICEOBJECTDATA_SIZE, pEntries, &count, 0);
    switch (result) {
    case DI_OK:
        buffer.m_nCount = count;
        return 1;
    case DI_BUFFEROVERFLOW:
        return 1;
    case DIERR_NOTACQUIRED:
        m_pMouseDevice->Acquire();
        return 1;
    case DIERR_INPUTLOST: {
        int64_t r = m_pMouseDevice->Acquire();
        return r == DI_OK;
    }
    case DIERR_NOTBUFFERED:
    case DIERR_NOTINITIALIZED:
    case DIERR_INVALIDPARAM:
    default:
        return 0;
    }
#endif
}

#if KOTOR1 && defined(K2_UI_PC)
extern "C" int k1pc_key_drain(unsigned char* pDik, unsigned char* pData, int nMax);
#endif
int CExoRawInputInternal::GetKeyboardBuffer(CExoDeviceBuffer& buffer)
{
#if KOTOR1
#ifdef K2_UI_PC

    {
        unsigned char aDik[kDeviceBufferEntries];
        unsigned char aData[kDeviceBufferEntries];
        int nKeys = k1pc_key_drain(aDik, aData, kDeviceBufferEntries);
        if (nKeys > 0 && m_bActive != 0) {
            static int s_nSequence;
            buffer.m_nCount = nKeys;
            if (buffer.m_pEntries != 0) {
                operator delete[](buffer.m_pEntries);
            }
            CExoInputKeyEvent* pKeys = new CExoInputKeyEvent[kDeviceBufferEntries];
            buffer.m_pEntries = pKeys;
            for (int i = 0; i < nKeys; i++) {
                pKeys[i].m_nKeyCode = aDik[i];
                pKeys[i].m_nValue = aData[i];
                pKeys[i].m_nTimeStamp = 0;
                pKeys[i].m_nSequence = ++s_nSequence;
                pKeys[i].m_nAppData = 0;
            }
            return 1;
        }
    }
#endif

    if (m_bActive == 0) {
        return 0;
    }
    if (!Ios_escPress) {
        return 0;
    }
    Ios_escPress = 0;
    buffer.m_nCount = 1;
    if (buffer.m_pEntries != 0) {
        operator delete[](buffer.m_pEntries);
    }
    CExoInputKeyEvent* pEntries = new CExoInputKeyEvent[kDeviceBufferEntries];
    buffer.m_pEntries = pEntries;
    pEntries->m_nAppData = 0;
    pEntries->m_nKeyCode = DIK_ESCAPE;
    pEntries->m_nValue = INPUTKEY_DOWN;
    pEntries->m_nTimeStamp = 0;
    pEntries->m_nSequence = kInjectedKeySequence;
    return 1;
#else

    if (Ios_escPress || Ios_spacePress || Ios_forfeitPress) {
        CExoInputKeyEvent* pOld = buffer.m_pEntries;
        buffer.m_nCount = 1;
        if (pOld != 0) {
            operator delete[](pOld);
        }
        CExoInputKeyEvent* pEntries = static_cast<CExoInputKeyEvent*>(operator new[](kDeviceBufferEntries * DIDEVICEOBJECTDATA_SIZE));
        buffer.m_pEntries = pEntries;
        if (Ios_escPress) {
            pEntries->m_nKeyCode = DIK_ESCAPE;
            Ios_escPress = 0;
        } else if (Ios_spacePress) {
            pEntries->m_nKeyCode = DIK_SPACE;
            Ios_spacePress = 0;
        } else if (Ios_forfeitPress) {
            pEntries->m_nKeyCode = DIK_F;
            Ios_forfeitPress = 0;
        }
        pEntries->m_nAppData = 0;
        pEntries->m_nSequence = kInjectedKeySequence;
        *reinterpret_cast<unsigned long long*>(&pEntries->m_nValue) = INPUTKEY_DOWN;  // raw-offset: value and m_nTimeStamp written as one 8-byte store, as the original
        return 1;
    }

    if (m_pKeyboardDevice == 0) {
        return 0;
    }
    if (m_bActive == 0) {
        return 0;
    }
    if (buffer.m_pEntries != 0) {
        operator delete[](buffer.m_pEntries);
        buffer.m_pEntries = 0;
    }
    buffer.m_nCount = 0;
    if (m_nKeyboardMode != 0) {
        return 1;
    }
    CExoInputKeyEvent* pEntries = static_cast<CExoInputKeyEvent*>(operator new[](kDeviceBufferEntries * DIDEVICEOBJECTDATA_SIZE));
    __builtin_memset(pEntries, 0, kDeviceBufferEntries);  // idiom-exempt: orig-engine partial clear (0x100 BYTES of the 0x1800-byte CExoInputKeyEvent[0x100] buffer -- the DI buffer element-count constant reused as a byte count, so no sizeof honestly names it; size pinned by byte-match)
    buffer.m_pEntries = pEntries;
    unsigned int count = kDeviceBufferEntries;
    int64_t result = m_pKeyboardDevice->GetDeviceData(DIDEVICEOBJECTDATA_SIZE, pEntries, &count, 0);
    switch (result) {
    case DI_OK:
        buffer.m_nCount = count;
        return 1;
    case DI_BUFFEROVERFLOW:
        return 1;
    case DIERR_NOTACQUIRED:
        m_pKeyboardDevice->Acquire();
        return 1;
    case DIERR_INPUTLOST: {
        int64_t r = m_pKeyboardDevice->Acquire();
        return r == DI_OK;
    }
    case DIERR_NOTBUFFERED:
    case DIERR_NOTINITIALIZED:
    case DIERR_INVALIDPARAM:
    default:
        return 0;
    }
#endif
}

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcstdlib.h"

struct CExoJoystickDeviceInfo {
    unsigned int  dwSize;
    unsigned char m_aReserved1[0x140-0x004];
    signed char   tszProductName[MAX_PATH];
    unsigned char m_aReserved2[0x268-0x244];
};

    // Acquires or releases every live device, then marks the port active. Coming out of keyboard
    // mode the keyboard is polled once to see whether it was lost and re-acquired if so.
void CExoRawInputInternal::SetActive(int active)
{
#if KOTOR1
    if (m_bActive != active) {
        m_bActive = active;
    }
#else

    if (m_bActive == active) {
        return;
    }

    if (m_pKeyboardDevice != 0) {
        IDirectInputDeviceA* pDevice = m_pKeyboardDevice;
        if (active == 1) {
            if (m_nKeyboardMode == 0) {
                pDevice->Acquire();
            }
        } else {
            pDevice->Unacquire();
        }
    }

    if (m_pMouseDevice != 0) {
        IDirectInputDeviceA* pDevice = m_pMouseDevice;
        if (active == 1) {
            pDevice->Acquire();
        } else {
            pDevice->Unacquire();
        }
    }

    for (int i = 0; i < m_nNumJoysticks; i++) {
        IDirectInputDeviceA** arr = m_paJoystickDevices;
        if (arr == 0) {
            continue;
        }
        IDirectInputDeviceA* pDevice = arr[i];
        if (pDevice == 0) {
            continue;
        }
        if (active == 1) {
            pDevice->Acquire();
        } else {
            pDevice->Unacquire();
        }
    }

    m_bActive = active;
    if (active != 0 && m_nKeyboardMode == 0 && m_pKeyboardDevice != 0) {
    retry:
        unsigned int count = static_cast<unsigned int>(-1);
        int64_t result = m_pKeyboardDevice->GetDeviceData(DIDEVICEOBJECTDATA_SIZE, 0, &count, 0);
        if (result == DIERR_NOTACQUIRED) {
            int64_t acqResult = m_pKeyboardDevice->Acquire();
            if (acqResult == DIERR_INPUTLOST) {
                goto retry;
            }
        } else if (result == DIERR_INPUTLOST) {
            m_pKeyboardDevice->Acquire();
        }
    }
#endif
}

void CExoRawInputInternal::ClearKeyboardBuffer()
{
#if KOTOR1
#else

    if (m_nKeyboardMode != 0) {
        return;
    }
    if (m_pKeyboardDevice == 0) {
        return;
    }
    if (m_bActive == 0) {
        return;
    }

retry:
    unsigned int count = static_cast<unsigned int>(-1);
    int64_t result = m_pKeyboardDevice->GetDeviceData(DIDEVICEOBJECTDATA_SIZE, 0, &count, 0);
    if (result == DIERR_NOTACQUIRED) {
        int64_t acqResult = m_pKeyboardDevice->Acquire();
        if (acqResult == DIERR_INPUTLOST) {
            goto retry;
        }
    } else if (result == DIERR_INPUTLOST) {
        m_pKeyboardDevice->Acquire();
    }
#endif
}

#if !KOTOR1
void CExoRawInputInternal::CheckControllerType(int deviceIndex)
{

    CExoJoystickDeviceInfo info;
    info.dwSize = sizeof(info);
    m_nControllerTypeLo = 0;
    m_bIsDualShockLike = 0;
    IDirectInputDeviceA* pDevice = m_paJoystickDevices[deviceIndex];
    int64_t result = pDevice->GetDeviceInfo(&info);
    if (result != DI_OK) {
        return;
    }
    info.tszProductName[sizeof(info.tszProductName) - 1] = 0;
    for (unsigned int i = 0; i < sizeof(info.tszProductName); i++) {
        info.tszProductName[i] = static_cast<signed char>(tolower(info.tszProductName[i]));
    }
    char* name = reinterpret_cast<char*>(info.tszProductName);
    if (strstr(name, "ps3") || strstr(name, "ps4") || strstr(name, "playstation") || strstr(name, "dualshock")) {
        m_bIsDualShockLike = 1;
    }
    puts(name);
}
#endif

struct DIDEVICEINSTANCEA {
    unsigned char m_aReserved1[8];
    unsigned char guidInstance[16];
};

struct DIDevCapsLocal {
    unsigned int dwSize;
    unsigned int dwFlags;
    unsigned int m_aReserved1[(0x2c - 8) / 4];
};

#if !KOTOR1
static int g_nNumJoysticksFound = 0;
#endif

#if !KOTOR1

// EnumDevices callbacks; the context is the port being enumerated into. The first only counts
// attached devices, to size the arrays; the second fills them.
int CountJoysticksExt(const DIDEVICEINSTANCEA* pInstance, void* pContext)
{
    K2_UNUSED(pInstance);
    static_cast<CExoRawInputInternal*>(pContext)->m_nNumJoysticks++;
    return 1;
}
#endif

#if !KOTOR1

int InitJoysticksExt(const DIDEVICEINSTANCEA* pInstance, void* pContext)
{
    static_cast<CExoRawInputInternal*>(pContext)->InitJoysticks(pInstance);
    return 1;
}
#endif

void CExoRawInputInternal::InitJoysticks(const DIDEVICEINSTANCEA* pInstance)
{
#if KOTOR1
    K2_UNUSED(pInstance);
#else

    IDirectInputDeviceA* pTempDevice;
    DIDevCapsLocal caps;
    int64_t result = m_pDirectInput->CreateDevice(pInstance->guidInstance, &pTempDevice, 0);
    if (result != DI_OK) {
        return;
    }

    result = pTempDevice->QueryInterface(&IID_IDirectInputDevice8A, reinterpret_cast<void**>(&m_paJoystickDevices[g_nNumJoysticksFound]));
    pTempDevice->Release();
    if (result != DI_OK) {
        return;
    }

    caps.dwSize = sizeof(caps);
    IDirectInputDeviceA* pDevice = m_paJoystickDevices[g_nNumJoysticksFound];
    pDevice->GetCapabilities(&caps);
    if ((caps.dwFlags & DIDC_ATTACHED) != 0) {
        g_nNumJoysticksFound++;
        return;
    }
    IDirectInputDeviceA* pDevice2 = m_paJoystickDevices[g_nNumJoysticksFound];
    pDevice2->Release();
    m_paJoystickDevices[g_nNumJoysticksFound] = 0;
#endif
}

#include "swguiingame.h"

struct DIPropStringLocal {
    unsigned int  dwSize;
    unsigned int  dwHeaderSize;
    unsigned int  dwObj;
    unsigned int  dwHow;
    unsigned char wsz[MAX_PATH * 4];  // WCHAR[MAX_PATH], 4-byte wide characters
};

#if KOTOR1 && defined(K2_HOST_BUILD)
extern "C" int k2host_key_name(unsigned int nDik, char* pszOut, int nOut);   // shim_dinput.cpp
#endif

CExoString CExoRawInputInternal::GetDisplayCharacterDirectInputValue(unsigned int key)
{
#if KOTOR1
#ifdef K2_HOST_BUILD
    // K1 never opens a DirectInput keyboard (Android had none); the host names the key itself.
    char szName[kKeyNameChars];
    if (k2host_key_name(key, szName, static_cast<int>(sizeof szName)))
        return CExoString(szName);
#else
    K2_UNUSED(key);
#endif
    return CExoString("N/A");
#else

    DIPropStringLocal prop;
    prop.dwSize = sizeof(prop);
    prop.dwHeaderSize = offsetof(DIPropStringLocal, wsz);
    prop.dwObj = key;
    prop.dwHow = DIPH_BYOFFSET;
    int64_t result = m_pKeyboardDevice->GetProperty(DIPROP_KEYNAME, &prop);

    const char* pResult;
    char buf[kKeyNameChars];
    if (key == DIK_ESCAPE) {
        pResult = GetLocalizedCustomString(CUSTOMSTRING_ESCAPE);
        if (pResult != 0) goto done;
    } else if (key == DIK_CAPITAL) {
        pResult = GetLocalizedCustomString(CUSTOMSTRING_CAPS_LOCK);
        if (pResult != 0) goto done;
    } else if (key == DIK_SPACE) {
        pResult = GetLocalizedCustomString(CUSTOMSTRING_SPACEBAR);
        if (pResult != 0) goto done;
    }
    if (result < 0) {
        pResult = "N/A";
    } else {
        for (int i = 0; i < kKeyNameChars; i++)
            buf[i] = 0;
        g_pExoBase->WcsToMbs(buf, reinterpret_cast<const K2WChar*>(prop.wsz), kKeyNameChars - 1);  // raw-offset: the DIPROPSTRING wide-string bytes
        pResult = buf;
    }
done:
    return CExoString(pResult);
#endif
}

#if !KOTOR1

    // Runs two enumeration passes, counting the attached controllers into m_nNumJoysticks so the
    // device and state arrays can be sized, then filling them. A device that fails to open is not
    // counted.
void CExoRawInputInternal::ReinitJoysticks()
{

    m_nNumJoysticks = 0;
    m_pDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, reinterpret_cast<void*>(CountJoysticksExt), this, DIEDFL_ATTACHEDONLY);

    m_paJoystickDevices = 0;
    m_pJoystickState = 0;
    m_pLastJoystickState = 0;

    if (m_nNumJoysticks != 0) {
        int n = m_nNumJoysticks;
        m_paJoystickDevices = new IDirectInputDeviceA*[n];
        for (int i = 0; i < n; i++) {
            m_paJoystickDevices[i] = 0;
        }
    }
    if (m_pJoystickState != 0) {
        operator delete[](m_pJoystickState);
    }
    if (m_pLastJoystickState != 0) {
        operator delete[](m_pLastJoystickState);
    }
    if (m_nNumJoysticks != 0) {
        int n = m_nNumJoysticks;
        m_pJoystickState = new CExoJoystickState[n];
        m_pLastJoystickState = new CExoJoystickState[n];
    }

    g_nNumJoysticksFound = 0;
    m_pDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, reinterpret_cast<void*>(InitJoysticksExt), this, DIEDFL_ATTACHEDONLY);
    m_nNumJoysticks = g_nNumJoysticksFound;

    if (m_nNumJoysticks < 1) {
        return;
    }

    for (int i = 0; i < m_nNumJoysticks; i++) {
        IDirectInputDeviceA* pDevice = m_paJoystickDevices[i];
        if (pDevice != 0) {
            int64_t result = pDevice->SetDataFormat(&c_dfDIJoystick2);
            if (result != DI_OK) {
                pDevice = m_paJoystickDevices[i];
                pDevice->Release();
                m_paJoystickDevices[i] = 0;
            }
        }
        if (m_paJoystickDevices[i] == 0) {
            continue;
        }
        CheckControllerType(i);
        if (m_paJoystickDevices[i] == 0) {
            continue;
        }
        __builtin_memset(&m_pJoystickState[i], 0, offsetof(CExoJoystickState, m_nLastUpdateMs));
        __builtin_memset(&m_pLastJoystickState[i], 0, offsetof(CExoJoystickState, m_nLastUpdateMs));
        m_pJoystickState[i].m_nLastUpdateMs = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
        m_pLastJoystickState[i].m_nLastUpdateMs = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
        m_pJoystickState[i].m_axisBoundLo = -32768;
        m_pJoystickState[i].m_axisBoundHi = 32767;
        m_pLastJoystickState[i].m_axisBoundLo = -32768;
        m_pLastJoystickState[i].m_axisBoundHi = 32767;
        m_pJoystickState[i].m_axisBound2Lo = -32768;
        m_pJoystickState[i].m_axisBound2Hi = 32767;
        m_pLastJoystickState[i].m_axisBound2Lo = -32768;
        m_pLastJoystickState[i].m_axisBound2Hi = 32767;
    }
}
#endif

extern "C" int64_t DirectInput8Create(HINSTANCE__* hinst, unsigned int dwVersion, const void* riidltf, void** ppvOut, void* punkOuter);

#if KOTOR1

CExoRawInputInternal::CExoRawInputInternal(void* hInst, void* hWnd)
{
    K2_UNUSED(hInst); K2_UNUSED(hWnd);
    m_pDirectInput = 0;
    m_pKeyboardDevice = 0;
    m_bInitialized = 0;
    m_bActive = 0;
    m_nKeyboardMode = 0;
    m_pMouseDevice = 0;
    m_bRumbleEnabled = 1;
    m_bRumblePaused = 0;
    m_nReserved1 = 0;
    m_nNumJoysticks = 1;
    m_paJoystickDevices = 0;
    m_pJoystickState = 0;
    m_pLastJoystickState = 0;
    m_bInitialized = 1;
}
#else

CExoRawInputInternal::CExoRawInputInternal(HINSTANCE__* hInst, HWND__* hWnd)
{

    m_nControllerTypeLo = 0;
    m_bIsDualShockLike = 0;
    m_pDirectInput = 0;
    m_pKeyboardDevice = 0;
    m_bInitialized = 0;
    m_bActive = 0;
    m_nKeyboardMode = 0;
    m_pMouseDevice = 0;
    m_bRumbleEnabled = 1;
    m_bRumblePaused = 0;
    m_nReserved1 = 0;

    int64_t result = DirectInput8Create(hInst, DIRECTINPUT_VERSION_8, &IID_IDirectInput8A, reinterpret_cast<void**>(&m_pDirectInput), 0);
    if (result != DI_OK) {
        result = DirectInput8Create(hInst, DIRECTINPUT_VERSION_3, &IID_IDirectInput8A, reinterpret_cast<void**>(&m_pDirectInput), 0);
        if (result != DI_OK) goto fail;
    }

    result = m_pDirectInput->CreateDevice(&GUID_SysKeyboard, &m_pKeyboardDevice, 0);
    if (result != DI_OK) goto fail;

    result = m_pKeyboardDevice->SetDataFormat(&c_dfDIKeyboard);
    if (result != DI_OK) goto fail;

    result = m_pKeyboardDevice->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
    if (result != DI_OK) goto fail;

    {
        DIPropDwordLocal prop;
        prop.dwSize = sizeof(prop);
        prop.dwHeaderSize = offsetof(DIPropDwordLocal, dwData);
        prop.dwObj = 0;
        prop.dwHow = DIPH_DEVICE;
        prop.dwData = kDeviceBufferEntries;
        result = m_pKeyboardDevice->SetProperty(DIPROP_BUFFERSIZE, &prop);
    }
    if (result != DI_OK) goto fail;

    m_nNumJoysticks = 0;
    m_pDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, reinterpret_cast<void*>(CountJoysticksExt), this, DIEDFL_ATTACHEDONLY);

    m_paJoystickDevices = 0;
    m_pJoystickState = 0;
    m_pLastJoystickState = 0;

    {
        int n = m_nNumJoysticks;
        if (n != 0) {
            m_paJoystickDevices = new IDirectInputDeviceA*[n];
            for (int i = 0; i < n; i++) {
                m_paJoystickDevices[i] = 0;
            }
        }
        if (n != 0) {
            m_pJoystickState = new CExoJoystickState[n];
            m_pLastJoystickState = new CExoJoystickState[n];
        }
    }

    g_nNumJoysticksFound = 0;
    m_pDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, reinterpret_cast<void*>(InitJoysticksExt), this, DIEDFL_ATTACHEDONLY);
    m_nNumJoysticks = g_nNumJoysticksFound;

    if (m_nNumJoysticks >= 1) {
        for (int i = 0; i < m_nNumJoysticks; i++) {
            IDirectInputDeviceA* pDevice = m_paJoystickDevices[i];
            if (pDevice != 0) {
                int64_t r = pDevice->SetDataFormat(&c_dfDIJoystick2);
                if (r != DI_OK) {
                    pDevice = m_paJoystickDevices[i];
                    pDevice->Release();
                    m_paJoystickDevices[i] = 0;
                }
            }
            if (m_paJoystickDevices[i] == 0) {
                continue;
            }
            pDevice = m_paJoystickDevices[i];
            int64_t r2 = pDevice->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
            if (r2 != 0) {
                pDevice = m_paJoystickDevices[i];
                pDevice->Release();
                m_paJoystickDevices[i] = 0;
            }

            CheckControllerType(i);
            if (m_paJoystickDevices[i] == 0) {
                continue;
            }
            __builtin_memset(&m_pJoystickState[i], 0, offsetof(CExoJoystickState, m_nLastUpdateMs));
            __builtin_memset(&m_pLastJoystickState[i], 0, offsetof(CExoJoystickState, m_nLastUpdateMs));
            m_pJoystickState[i].m_nLastUpdateMs = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
            m_pLastJoystickState[i].m_nLastUpdateMs = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
            m_pJoystickState[i].m_axisBoundLo = -32768;
            m_pJoystickState[i].m_axisBoundHi = 32767;
            m_pLastJoystickState[i].m_axisBoundLo = -32768;
            m_pLastJoystickState[i].m_axisBoundHi = 32767;
            m_pJoystickState[i].m_axisBound2Lo = -32768;
            m_pJoystickState[i].m_axisBound2Hi = 32767;
            m_pLastJoystickState[i].m_axisBound2Lo = -32768;
            m_pLastJoystickState[i].m_axisBound2Hi = 32767;
        }
    }
    m_bInitialized = 1;
    return;

fail:
    ShutDownDirectInput();
}
#endif


extern float ios_gamepad_analog0X;
extern float ios_gamepad_analog0Y;
extern float ios_gamepad_analog1X;
extern float ios_gamepad_analog1Y;

#if !KOTOR1
static int g_nLeftTriggerHeld = 0;
static int g_nRightTriggerHeld = 0;
#endif

    // Turns the joystick snapshot into input events: the 32 digital buttons, the four POV hats
    // (raw value plus a per-direction decode), the analog sticks with their deadzone clamp, and the
    // two trigger axes.
int CExoRawInputInternal::GetJoystickBuffer(int deviceIndex, CExoDeviceBuffer& buffer)
{
#if KOTOR1
    K2_UNUSED(deviceIndex); K2_UNUSED(buffer);
    return 1;
#else

    if (buffer.m_pEntries != 0) {
        operator delete[](buffer.m_pEntries);
        buffer.m_pEntries = 0;
    }
    buffer.m_nCount = 0;
    unsigned char* pEntries = static_cast<unsigned char*>(operator new[](kDeviceBufferEntries * DIDEVICEOBJECTDATA_SIZE));
    __builtin_memset(pEntries, 0, kDeviceBufferEntries);  // idiom-exempt: orig-engine partial clear (0x100 BYTES of the 0x1800-byte CExoInputKeyEvent[0x100] buffer -- the DI buffer element-count constant reused as a byte count, so no sizeof honestly names it; size pinned by byte-match)
    buffer.m_pEntries = reinterpret_cast<CExoInputKeyEvent*>(pEntries);

    int ok = 0;
    if (m_paJoystickDevices != 0) {
        IDirectInputDeviceA* pDevice = m_paJoystickDevices[deviceIndex];
        if (pDevice != 0 && m_pJoystickState != 0 && m_pLastJoystickState != 0) {
            pDevice->Poll();

            unsigned char newState[DIJOYSTATE2_SIZE];
            int64_t result = pDevice->GetDeviceState(DIJOYSTATE2_SIZE, newState);
            if (result != DI_OK) {
                s_isDeviceValid[deviceIndex] = 0;
                if (result < DIERR_NOTINITIALIZED) {
                    if (result != E_PENDING && result == DIERR_NOTACQUIRED) {
                        pDevice->Acquire();
                    }
                } else if (result != DIERR_NOTINITIALIZED && result != DIERR_INVALIDPARAM &&
                           result == DIERR_INPUTLOST) {
                    int64_t r = pDevice->Acquire();
                    if (r == DI_OK) goto success;
                }
                m_nNumJoysticks = 0;
                goto done;
            }

            m_nNumJoysticks = 1;
            s_isDeviceValid[deviceIndex] = 1;
            {
                unsigned char* pCur = reinterpret_cast<unsigned char*>(m_pJoystickState + deviceIndex);

                unsigned long long timer = g_pExoBase->m_pTimers->GetHighResolutionTimer();

                for (int off = DIJOFS_BUTTON0; off != DIJOFS_BUTTON_END; off++) {
                    unsigned char oldB = pCur[off];  // raw-offset: DirectInput state offsets are its DIJOFS event codes
                    unsigned char newB = newState[off];
                    if (static_cast<signed char>(oldB ^ newB) < 0) {
                        unsigned int idx = buffer.m_nCount;
                        CJoystickInputEvent* entry = reinterpret_cast<CJoystickInputEvent*>(
                            pEntries + static_cast<uint64_t>(idx) * sizeof(CJoystickInputEvent));
                        entry->m_code = off;
                        entry->m_value = newB >> 7;
                        entry->m_nTimeStampAndSequence = 0;
                        buffer.m_nCount = idx + 1;
                    }
                }

                for (int i = 0; i < 4; i++) {
                    unsigned int off = DIJOFS_POV0 + i * 4;
                    unsigned int newVal = *reinterpret_cast<unsigned int*>(newState + off);
                    unsigned int oldVal = *reinterpret_cast<unsigned int*>(pCur + off);  // raw-offset: DirectInput state offsets are its DIJOFS event codes
                    if (newVal != oldVal) {
                        unsigned int v;
                        if (((newVal ^ 0xffffffffu) & 0xffffu) == 0) {
                            v = 0xffffffffu;
                        } else {
                            v = newVal / 100u;
                        }
                        unsigned int idx = buffer.m_nCount;
                        CJoystickInputEvent* entry = reinterpret_cast<CJoystickInputEvent*>(
                            pEntries + static_cast<uint64_t>(idx) * sizeof(CJoystickInputEvent));
                        entry->m_code = DIJOFS_POV0 + i;
                        entry->m_value = v;
                        entry->m_nTimeStampAndSequence = 0;
                        buffer.m_nCount = idx + 1;
                    }
                }

                unsigned int newDirBits = 0, oldDirBits = 0;
                for (unsigned int axis = 0, off2 = DIJOFS_POV0; off2 != DIJOFS_POV_END; off2 += 4, axis++) {
                    unsigned int newVal = *reinterpret_cast<unsigned int*>(newState + off2);
                    unsigned int oldVal = *reinterpret_cast<unsigned int*>(pCur + off2);  // raw-offset: DirectInput state offsets are its DIJOFS event codes
                    if (newVal == oldVal) {
                        continue;
                    }
                    if ((~newVal & 0xffffu) != 0) {
                        unsigned int bNearUp   = (newVal - 4500u) > 99u;
                        unsigned int bNearLeft = (newVal - 31500u) > 99u;
                        unsigned int bUp = 1u << (axis & 31u);
                        if (bNearLeft && newVal > 99u && bNearUp) bUp = 0;
                        unsigned int bNearRight = (newVal - 13500u) > 99u;
                        unsigned int bRight = 2u << (axis & 31u);
                        if (bNearRight && bNearUp && (newVal - 9000u) > 99u) bRight = 0;
                        bNearUp = (newVal - 22500u) > 99u;
                        unsigned int bDown = 4u << (axis & 31u);
                        if (bNearUp && bNearRight && (newVal - 18000u) > 99u) bDown = 0;
                        unsigned int bLeft = 8u << (axis & 31u);
                        if (bNearLeft && bNearUp && (newVal - 27000u) > 99u) bLeft = 0;
                        newDirBits = bUp | newDirBits | bRight | bDown | bLeft;
                    }
                    if ((~oldVal & 0xffffu) != 0) {
                        if ((oldVal - 31500u < 100u) || (oldVal < 100u) || (oldVal - 4500u < 100u)) {
                            oldDirBits |= 1u << (off2 & 31u);
                        }
                        if ((oldVal - 13500u < 100u) || (oldVal - 4500u < 100u) || (oldVal - 9000u < 100u)) {
                            oldDirBits |= 1u << ((off2 - 31u) & 31u);
                        }
                        if ((oldVal - 22500u < 100u) || (oldVal - 13500u < 100u) || (oldVal - 18000u < 100u)) {
                            oldDirBits |= 1u << ((off2 - 30u) & 31u);
                        }
                        if ((oldVal - 31500u < 100u) || (oldVal - 22500u < 100u) || (oldVal - 27000u < 100u)) {
                            oldDirBits |= 1u << ((off2 - 29u) & 31u);
                        }
                    }
                }

                newDirBits &= 0xffffu;
                oldDirBits &= 0xffffu;

                int code = POVCODE_LEFT;
                for (unsigned int off3 = DIJOFS_POV0; off3 != DIJOFS_POV_END; off3 += 4, code++) {
                    unsigned int newVal = *reinterpret_cast<unsigned int*>(newState + off3);
                    unsigned int oldVal = *reinterpret_cast<unsigned int*>(pCur + off3);  // raw-offset: DirectInput state offsets are its DIJOFS event codes
                    if (newVal == oldVal) {
                        continue;
                    }
                    unsigned int bit0 = 1u << (off3 & 31u);
                    unsigned int a0 = bit0 & newDirBits;
                    if (a0 != (bit0 & oldDirBits)) {
                        unsigned int idx = buffer.m_nCount;
                        CJoystickInputEvent* entry = reinterpret_cast<CJoystickInputEvent*>(
                            pEntries + static_cast<uint64_t>(idx) * sizeof(CJoystickInputEvent));
                        entry->m_code = code - (POVCODE_LEFT - POVCODE_UP);
                        entry->m_value = (a0 != 0);
                        entry->m_nTimeStampAndSequence = 0;
                        buffer.m_nCount = idx + 1;
                    }
                    unsigned int bit1 = 1u << ((off3 - 31u) & 31u);
                    unsigned int a1 = bit1 & newDirBits;
                    if (a1 != (bit1 & oldDirBits)) {
                        unsigned int idx = buffer.m_nCount;
                        CJoystickInputEvent* entry = reinterpret_cast<CJoystickInputEvent*>(
                            pEntries + static_cast<uint64_t>(idx) * sizeof(CJoystickInputEvent));
                        entry->m_code = code - (POVCODE_LEFT - POVCODE_DOWN);
                        entry->m_value = (a1 != 0);
                        entry->m_nTimeStampAndSequence = 0;
                        buffer.m_nCount = idx + 1;
                    }
                    unsigned int bit2 = 1u << ((off3 - 30u) & 31u);
                    unsigned int a2 = bit2 & newDirBits;
                    if (a2 != (bit2 & oldDirBits)) {
                        unsigned int idx = buffer.m_nCount;
                        CJoystickInputEvent* entry = reinterpret_cast<CJoystickInputEvent*>(
                            pEntries + static_cast<uint64_t>(idx) * sizeof(CJoystickInputEvent));
                        entry->m_code = code - (POVCODE_LEFT - POVCODE_RIGHT);
                        entry->m_value = (a2 != 0);
                        entry->m_nTimeStampAndSequence = 0;
                        buffer.m_nCount = idx + 1;
                    }
                    unsigned int bit3 = 1u << ((off3 - 29u) & 31u);
                    unsigned int a3 = bit3 & newDirBits;
                    if (a3 != (bit3 & oldDirBits)) {
                        unsigned int idx = buffer.m_nCount;
                        CJoystickInputEvent* entry = reinterpret_cast<CJoystickInputEvent*>(
                            pEntries + static_cast<uint64_t>(idx) * sizeof(CJoystickInputEvent));
                        entry->m_code = code;
                        entry->m_value = (a3 != 0);
                        entry->m_nTimeStampAndSequence = 0;
                        buffer.m_nCount = idx + 1;
                    }
                }

                float loBound = static_cast<float>(reinterpret_cast<CExoJoystickState*>(pCur)->m_axisBoundLo) * 0.25f;
                float hiBound = static_cast<float>(reinterpret_cast<CExoJoystickState*>(pCur)->m_axisBoundHi) * 0.25f;

                int* pLX = reinterpret_cast<int*>(newState + DIJOFS_X);
                if (static_cast<float>(*pLX) < hiBound && loBound < static_cast<float>(*pLX)) *pLX = 0;
                {
                    unsigned int idx = buffer.m_nCount;
                    CJoystickInputEvent* entry = reinterpret_cast<CJoystickInputEvent*>(
                        pEntries + static_cast<uint64_t>(idx) * sizeof(CJoystickInputEvent));
                    entry->m_code = DIJOFS_X;
                    entry->m_value = *pLX;
                    entry->m_nTimeStampAndSequence = 0;
                    buffer.m_nCount = idx + 1;
                }

                int* pLY = reinterpret_cast<int*>(newState + DIJOFS_Y);
                if (static_cast<float>(*pLY) < hiBound && loBound < static_cast<float>(*pLY)) *pLY = 0;
                {
                    unsigned int idx = buffer.m_nCount;
                    CJoystickInputEvent* entry = reinterpret_cast<CJoystickInputEvent*>(
                        pEntries + static_cast<uint64_t>(idx) * sizeof(CJoystickInputEvent));
                    entry->m_code = DIJOFS_Y;
                    entry->m_value = *pLY;
                    entry->m_nTimeStampAndSequence = 0;
                    buffer.m_nCount = idx + 1;
                }

                int* pRX = reinterpret_cast<int*>(newState + DIJOFS_RX);
                if (static_cast<float>(*pRX) < hiBound && loBound < static_cast<float>(*pRX)) *pRX = 0;
                {
                    unsigned int idx = buffer.m_nCount;
                    CJoystickInputEvent* entry = reinterpret_cast<CJoystickInputEvent*>(
                        pEntries + static_cast<uint64_t>(idx) * sizeof(CJoystickInputEvent));
                    entry->m_code = DIJOFS_RX;
                    entry->m_value = *pRX;
                    entry->m_nTimeStampAndSequence = 0;
                    buffer.m_nCount = idx + 1;
                }

                int* pRY = reinterpret_cast<int*>(newState + DIJOFS_RY);
                if (static_cast<float>(*pRY) < hiBound && loBound < static_cast<float>(*pRY)) *pRY = 0;
                {
                    unsigned int idx = buffer.m_nCount;
                    CJoystickInputEvent* entry = reinterpret_cast<CJoystickInputEvent*>(
                        pEntries + static_cast<uint64_t>(idx) * sizeof(CJoystickInputEvent));
                    entry->m_code = DIJOFS_RY;
                    entry->m_value = *pRY;
                    entry->m_nTimeStampAndSequence = 0;
                    buffer.m_nCount = idx + 1;
                }

                ios_gamepad_analog0X = static_cast<float>(*pLX) / 32767.0f;
                ios_gamepad_analog0Y = static_cast<float>(*pLY) / 32767.0f;
                ios_gamepad_analog1X = static_cast<float>(*pRX) / 32767.0f;
                ios_gamepad_analog1Y = static_cast<float>(*pRY) / 32767.0f;

                int* pLZ = reinterpret_cast<int*>(newState + DIJOFS_Z);
                *pLZ -= 300;
                if (*pLZ < 1) {
                    g_nLeftTriggerHeld = 0;
                } else if (g_nLeftTriggerHeld == 0) {
                    unsigned int idx = buffer.m_nCount;
                    CJoystickInputEvent* entry = reinterpret_cast<CJoystickInputEvent*>(
                        pEntries + static_cast<uint64_t>(idx) * sizeof(CJoystickInputEvent));
                    entry->m_code = DIJOFS_Z;
                    entry->m_value = *pLZ;
                    entry->m_nTimeStampAndSequence = 0;
                    buffer.m_nCount = idx + 1;
                    g_nLeftTriggerHeld = *pLZ;
                }

                int* pRZ = reinterpret_cast<int*>(newState + DIJOFS_RZ);
                *pRZ -= 300;
                if (*pRZ < 1) {
                    g_nRightTriggerHeld = 0;
                } else if (g_nRightTriggerHeld == 0) {
                    unsigned int idx = buffer.m_nCount;
                    CJoystickInputEvent* entry = reinterpret_cast<CJoystickInputEvent*>(
                        pEntries + static_cast<uint64_t>(idx) * sizeof(CJoystickInputEvent));
                    entry->m_code = DIJOFS_RZ;
                    entry->m_value = *pRZ;
                    entry->m_nTimeStampAndSequence = 0;
                    buffer.m_nCount = idx + 1;
                    g_nRightTriggerHeld = *pRZ;
                }

                memcpy(m_pLastJoystickState + deviceIndex, m_pJoystickState + deviceIndex, offsetof(CExoJoystickState, m_nLastUpdateMs));
                memcpy(m_pJoystickState + deviceIndex, newState, offsetof(CExoJoystickState, m_nLastUpdateMs));
                (m_pJoystickState + deviceIndex)->m_nLastUpdateMs = static_cast<unsigned int>(timer / 1000);
            }
        success:
            ok = 1;
        }
    }
done:
    return ok;
#endif
}

CExoDeviceBuffer::CExoDeviceBuffer()
{
    __builtin_memset(static_cast<void*>(this), 0, sizeof(*this));
}

CExoDeviceBuffer::~CExoDeviceBuffer()
{
    delete[] m_pEntries;
    m_pEntries = 0;
}
