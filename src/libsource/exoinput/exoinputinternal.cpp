#include "k2stdint.h"
#include "port/sdlapi.h"
#include "exoinputinternal.h"
#include "exobase.h"
#include "exotimersinternal.h"
#include "exofile.h"
#include "CExoArrayListImpl.h"
#include "port/k2qolapi.h"
#include "port/win32const.h"

int g_cursorShouldBeVisible = 1;
float fMaxMickies = 100.0f;

CExoInputClass::CExoInputClass()
{
}

CExoInputClass::~CExoInputClass()
{
}

CExoInputInternal::~CExoInputInternal()
{
    // An explicit destructor call plus a separate free rather than a delete-expression, which would
    // also free on the destructor's unwind path.
    if (m_pMainDevice != 0) {
        CExoRawInputInternal* pDevice = m_pMainDevice;
        pDevice->~CExoRawInputInternal();
        ::operator delete(pDevice);
        m_pMainDevice = 0;
    }
    // The descriptors have virtual functions, but their destructors are never called; they are
    // freed directly. Then the index array is released and its count and capacity reset.
    for (int i = 0, nCount = m_aEvents.m_count; i < nCount; i++) {
        CExoInputEventDesc* pDesc = m_aEvents.m_pArray[i];
        if (pDesc != 0)
            ::operator delete(pDesc);
    }
    m_aEvents.Clear();
    // The repeat-timer list nodes belong to the member's own destructor; the records do not.
    CExoLinkedListNode* pNode = m_repeatList.m_pInternal->m_pHead;
    while (pNode != 0) {
        CExoInputRepeatEvent* pRec = static_cast<CExoInputRepeatEvent*>(m_repeatList.m_pInternal->GetAtPos(pNode));
        if (pRec != 0)
            ::operator delete(pRec);
        m_repeatList.m_pInternal->GetNext(pNode);
    }
    while (m_mouseEventList.m_pInternal->m_pHead != 0) {
        CExoInputEvent* pEvent = static_cast<CExoInputEvent*>(m_mouseEventList.m_pInternal->RemoveHead());
        if (pEvent != 0) {
            if (pEvent->m_pEvent != 0)
                ::operator delete(pEvent->m_pEvent);
            ::operator delete(pEvent);
        }
    }
}

#if KOTOR1
CExoInputInternal::CExoInputInternal(void* hInst, void* hWnd)
#else
CExoInputInternal::CExoInputInternal(HINSTANCE__* hInst, HWND__* hWnd)
#endif
{
    uint64_t nTimer;
    m_pMainDevice = 0;
    m_nKeyboardMode = 0;
    m_bCaptureKeyboardKeys = 0;
    m_nEventQueueCount = 0;
    m_bActive = 0;
    __builtin_memset(m_aReserved5, 0, sizeof(m_aReserved5));

#if KOTOR1 && !defined(K2_UI_PC)
    // K1: no DirectInput scan-code table; only Esc gets its code
    m_keyArray[CExoInput::KEYBOARD_ESC] = DIK_ESCAPE;
#else
#if !KOTOR1  // KOTOR 1's pc UI: the keyboard and mouse codes below (its pad is SDL_main's bitmask)
    m_keyArray[CExoInput::JOYSTICK_BUTTON0] = DIJOFS_BUTTON0;
    m_keyArray[CExoInput::JOYSTICK_BUTTON1] = DIJOFS_BUTTON0 + 1;
    m_keyArray[CExoInput::JOYSTICK_BUTTON2] = DIJOFS_BUTTON0 + 2;
    m_keyArray[CExoInput::JOYSTICK_BUTTON3] = DIJOFS_BUTTON0 + 3;
    m_keyArray[CExoInput::JOYSTICK_BUTTON4] = DIJOFS_BUTTON0 + 4;
    m_keyArray[CExoInput::JOYSTICK_BUTTON5] = DIJOFS_BUTTON0 + 5;
    m_keyArray[CExoInput::JOYSTICK_BUTTON6] = DIJOFS_BUTTON0 + 6;
    m_keyArray[CExoInput::JOYSTICK_BUTTON7] = DIJOFS_BUTTON0 + 7;
    m_keyArray[CExoInput::JOYSTICK_BUTTON8] = DIJOFS_BUTTON0 + 8;
    m_keyArray[CExoInput::JOYSTICK_BUTTON9] = DIJOFS_BUTTON0 + 9;
    m_keyArray[CExoInput::JOYSTICK_BUTTON10] = DIJOFS_BUTTON0 + 10;
    m_keyArray[CExoInput::JOYSTICK_BUTTON11] = POVCODE_UP;
    m_keyArray[CExoInput::JOYSTICK_BUTTON12] = POVCODE_DOWN;
    m_keyArray[CExoInput::JOYSTICK_BUTTON13] = POVCODE_RIGHT;
    m_keyArray[CExoInput::JOYSTICK_BUTTON14] = POVCODE_LEFT;
    m_keyArray[CExoInput::JOYSTICK_XAXIS] = DIJOFS_X;
    m_keyArray[CExoInput::JOYSTICK_YAXIS] = DIJOFS_Y;
    m_keyArray[CExoInput::JOYSTICK_HAT] = DIJOFS_POV0;
    m_keyArray[CExoInput::JOYSTICK_SLIDER0] = DIJOFS_SLIDER0;
    m_keyArray[CExoInput::JOYSTICK_SLIDER1] = DIJOFS_SLIDER1;
    m_keyArray[CExoInput::JOYSTICK_SLIDER2] = DIJOFS_POV0;
    m_keyArray[CExoInput::JOYSTICK_RXAXIS] = DIJOFS_RX;
    m_keyArray[CExoInput::JOYSTICK_RYAXIS] = DIJOFS_RY;
    m_keyArray[CExoInput::JOYSTICK_RZAXIS] = DIJOFS_RZ;
#endif
    m_keyArray[CExoInput::MOUSE_BUTTON0] = DIMOFS_BUTTON0;
    m_keyArray[CExoInput::MOUSE_BUTTON1] = DIMOFS_BUTTON1;
    m_keyArray[CExoInput::MOUSE_BUTTON2] = DIMOFS_BUTTON2;
    m_keyArray[CExoInput::MOUSE_XAXIS] = DIMOFS_X;
    m_keyArray[CExoInput::MOUSE_YAXIS] = DIMOFS_Y;
    m_keyArray[CExoInput::MOUSE_ZAXIS] = DIMOFS_Z;
    m_keyArray[CExoInput::KEYBOARD_RETURN] = DIK_RETURN;
    m_keyArray[CExoInput::KEYBOARD_SPACE] = DIK_SPACE;
    m_keyArray[CExoInput::KEYBOARD_LEFT_ARROW] = DIK_LEFT;
    m_keyArray[CExoInput::KEYBOARD_RIGHT_ARROW] = DIK_RIGHT;
    m_keyArray[CExoInput::KEYBOARD_UP_ARROW] = DIK_UP;
    m_keyArray[CExoInput::KEYBOARD_DOWN_ARROW] = DIK_DOWN;
    m_keyArray[CExoInput::KEYBOARD_NUMPAD1] = DIK_NUMPAD1;
    m_keyArray[CExoInput::KEYBOARD_NUMPAD2] = DIK_NUMPAD2;
    m_keyArray[CExoInput::KEYBOARD_NUMPAD3] = DIK_NUMPAD3;
    m_keyArray[CExoInput::KEYBOARD_NUMPAD4] = DIK_NUMPAD4;
    m_keyArray[CExoInput::KEYBOARD_NUMPAD5] = DIK_NUMPAD5;
    m_keyArray[CExoInput::KEYBOARD_NUMPAD6] = DIK_NUMPAD6;
    m_keyArray[CExoInput::KEYBOARD_NUMPAD7] = DIK_NUMPAD7;
    m_keyArray[CExoInput::KEYBOARD_NUMPAD8] = DIK_NUMPAD8;
    m_keyArray[CExoInput::KEYBOARD_NUMPAD9] = DIK_NUMPAD9;
    m_keyArray[CExoInput::KEYBOARD_NUMPAD0] = DIK_NUMPAD0;
    m_keyArray[CExoInput::KEYBOARD_NUMPADMINUS] = DIK_SUBTRACT;
    m_keyArray[CExoInput::KEYBOARD_NUMPADPLUS] = DIK_ADD;
    m_keyArray[CExoInput::KEYBOARD_NUMPADDECIMAL] = DIK_DECIMAL;
    m_keyArray[CExoInput::KEYBOARD_LEFTSHIFT] = DIK_LSHIFT;
    m_keyArray[CExoInput::KEYBOARD_RIGHTSHIFT] = DIK_RSHIFT;
    m_keyArray[CExoInput::KEYBOARD_LEFTALT] = DIK_LMENU;
    m_keyArray[CExoInput::KEYBOARD_RIGHTALT] = DIK_RMENU;
    m_keyArray[CExoInput::KEYBOARD_LEFTCTRL] = DIK_LCONTROL;
    m_keyArray[CExoInput::KEYBOARD_RIGHTCTRL] = DIK_RCONTROL;
    m_keyArray[CExoInput::KEYBOARD_TAB] = DIK_TAB;
    m_keyArray[CExoInput::KEYBOARD_ESC] = DIK_ESCAPE;
    m_keyArray[CExoInput::KEYBOARD_HOME] = DIK_HOME;
    m_keyArray[CExoInput::KEYBOARD_END] = DIK_END;
    m_keyArray[CExoInput::KEYBOARD_PAGEUP] = DIK_PRIOR;
    m_keyArray[CExoInput::KEYBOARD_PAGEDOWN] = DIK_NEXT;
    m_keyArray[CExoInput::KEYBOARD_INSERT] = DIK_INSERT;
    m_keyArray[CExoInput::KEYBOARD_DELETE] = DIK_DELETE;
    m_keyArray[CExoInput::KEYBOARD_PRINTSCREEN] = DIK_SYSRQ;
    m_keyArray[CExoInput::KEYBOARD_F1] = DIK_F1;
    m_keyArray[CExoInput::KEYBOARD_F2] = DIK_F2;
    m_keyArray[CExoInput::KEYBOARD_F3] = DIK_F3;
    m_keyArray[CExoInput::KEYBOARD_F4] = DIK_F4;
    m_keyArray[CExoInput::KEYBOARD_F5] = DIK_F5;
    m_keyArray[CExoInput::KEYBOARD_F6] = DIK_F6;
    m_keyArray[CExoInput::KEYBOARD_F7] = DIK_F7;
    m_keyArray[CExoInput::KEYBOARD_F8] = DIK_F8;
    m_keyArray[CExoInput::KEYBOARD_F9] = DIK_F9;
    m_keyArray[CExoInput::KEYBOARD_F10] = DIK_F10;
    m_keyArray[CExoInput::KEYBOARD_F11] = DIK_F11;
    m_keyArray[CExoInput::KEYBOARD_F12] = DIK_F12;
    m_keyArray[CExoInput::KEYBOARD_F13] = DIK_F13;
    m_keyArray[CExoInput::KEYBOARD_F14] = DIK_F14;
    m_keyArray[CExoInput::KEYBOARD_F15] = DIK_F15;
    m_keyArray[CExoInput::KEYBOARD_A] = DIK_A;
    m_keyArray[CExoInput::KEYBOARD_B] = DIK_B;
    m_keyArray[CExoInput::KEYBOARD_C] = DIK_C;
    m_keyArray[CExoInput::KEYBOARD_D] = DIK_D;
    m_keyArray[CExoInput::KEYBOARD_E] = DIK_E;
    m_keyArray[CExoInput::KEYBOARD_F] = DIK_F;
    m_keyArray[CExoInput::KEYBOARD_G] = DIK_G;
    m_keyArray[CExoInput::KEYBOARD_H] = DIK_H;
    m_keyArray[CExoInput::KEYBOARD_I] = DIK_I;
    m_keyArray[CExoInput::KEYBOARD_J] = DIK_J;
    m_keyArray[CExoInput::KEYBOARD_K] = DIK_K;
    m_keyArray[CExoInput::KEYBOARD_L] = DIK_L;
    m_keyArray[CExoInput::KEYBOARD_M] = DIK_M;
    m_keyArray[CExoInput::KEYBOARD_N] = DIK_N;
    m_keyArray[CExoInput::KEYBOARD_O] = DIK_O;
    m_keyArray[CExoInput::KEYBOARD_P] = DIK_P;
    m_keyArray[CExoInput::KEYBOARD_Q] = DIK_Q;
    m_keyArray[CExoInput::KEYBOARD_R] = DIK_R;
    m_keyArray[CExoInput::KEYBOARD_S] = DIK_S;
    m_keyArray[CExoInput::KEYBOARD_T] = DIK_T;
    m_keyArray[CExoInput::KEYBOARD_U] = DIK_U;
    m_keyArray[CExoInput::KEYBOARD_V] = DIK_V;
    m_keyArray[CExoInput::KEYBOARD_W] = DIK_W;
    m_keyArray[CExoInput::KEYBOARD_X] = DIK_X;
    m_keyArray[CExoInput::KEYBOARD_Y] = DIK_Y;
    m_keyArray[CExoInput::KEYBOARD_Z] = DIK_Z;
    m_keyArray[CExoInput::KEYBOARD_1] = DIK_1;
    m_keyArray[CExoInput::KEYBOARD_2] = DIK_2;
    m_keyArray[CExoInput::KEYBOARD_3] = DIK_3;
    m_keyArray[CExoInput::KEYBOARD_4] = DIK_4;
    m_keyArray[CExoInput::KEYBOARD_5] = DIK_5;
    m_keyArray[CExoInput::KEYBOARD_6] = DIK_6;
    m_keyArray[CExoInput::KEYBOARD_7] = DIK_7;
    m_keyArray[CExoInput::KEYBOARD_8] = DIK_8;
    m_keyArray[CExoInput::KEYBOARD_9] = DIK_9;
    m_keyArray[CExoInput::KEYBOARD_0] = DIK_0;
    m_keyArray[CExoInput::KEYBOARD_MINUS] = DIK_MINUS;
    m_keyArray[CExoInput::KEYBOARD_EQUALS] = DIK_EQUALS;
    m_keyArray[CExoInput::KEYBOARD_BACK] = DIK_BACK;
    m_keyArray[CExoInput::KEYBOARD_LBRACKET] = DIK_LBRACKET;
    m_keyArray[CExoInput::KEYBOARD_RBRACKET] = DIK_RBRACKET;
    m_keyArray[CExoInput::KEYBOARD_SEMICOLON] = DIK_SEMICOLON;
    m_keyArray[CExoInput::KEYBOARD_APOSTROPHE] = DIK_APOSTROPHE;
    m_keyArray[CExoInput::KEYBOARD_GRAVE] = DIK_GRAVE;
    m_keyArray[CExoInput::KEYBOARD_BACKSLASH] = DIK_BACKSLASH;
    m_keyArray[CExoInput::KEYBOARD_COMMA] = DIK_COMMA;
    m_keyArray[CExoInput::KEYBOARD_PERIOD] = DIK_PERIOD;
    m_keyArray[CExoInput::KEYBOARD_SLASH] = DIK_SLASH;
    m_keyArray[CExoInput::KEYBOARD_MULTIPLY] = DIK_MULTIPLY;
    m_keyArray[CExoInput::KEYBOARD_NUMPADCOMMA] = DIK_NUMPADCOMMA;
    m_keyArray[CExoInput::KEYBOARD_DIVIDE] = DIK_DIVIDE;
    m_keyArray[CExoInput::KEYBOARD_NUMPADENTER] = DIK_NUMPADENTER;
    m_keyArray[CExoInput::KEYBOARD_CAPSLOCK] = DIK_CAPITAL;
    m_keyArray[CExoInput::KEYBOARD_PAUSE] = DIK_PAUSE;
    m_keyArray[CExoInput::KEYBOARD_OEM_102] = DIK_OEM_102;
#endif

    // Creates the device port, bails if it failed to initialise, then sizes the class tables.
    CExoRawInputInternal* pDevice = new CExoRawInputInternal(hInst, hWnd);
    m_pMainDevice = pDevice;
    if (pDevice->m_bInitialized == 0) {
        pDevice->~CExoRawInputInternal();
        ::operator delete(pDevice);
        m_pMainDevice = 0;
        return;
    }
    m_nDeviceCount = (pDevice->m_nNumJoysticks > 4) ? 6 : pDevice->m_nNumJoysticks + 2;
    m_aEvents.SetSize(1);
    m_aEvents.m_pArray[0] = 0;
    for (int i = 0; i < 6; i++) {
        m_aEventClass[i].m_sName = "Unnamed Input Class";
        // Reserves one slot but leaves the list empty, setting the count back to zero.
        m_aEventClass[i].m_aEventBits.SetSize(1);
        m_aEventClass[i].m_aEventBits.m_count = 0;
    }
    // The timer is kept in milliseconds, and the two position pairs below are separate fields.
    nTimer = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    m_nReserved6 = 0;
    m_nMouseMode = 0;
    m_nLastTimer = static_cast<int>(nTimer / 1000);
    m_nMouseX = -1;
    m_nMouseY = -1;
    // The store order matters.
    m_bMousePosForced = 0;
    m_nCursorId = MOUSECURSOR_NONE;
    m_nReserved7 = 0;
    m_nLastMouseDownState = 0;
}


void CExoInputInternal::Configure()
{
    // sic: an empty stub
}


int CExoInputInternal::SetRumble(int nPort, float fLow, float fHigh, unsigned int nMs)
{
#ifdef K2_QOL_CONTROLLER
    // The mobile build's sink is this stub; the motors belong to the platform layer. UpdateRumble
    // drives port 0 every frame, and a paused or disabled device gets a stop.
    if (nPort == 0) {
        if (m_pMainDevice && (!m_pMainDevice->m_bRumbleEnabled || m_pMainDevice->m_bRumblePaused))
            fLow = fHigh = 0.0f;
        k2qol_set_rumble_motors(fLow, fHigh, nMs);
    }
#else
    K2_UNUSED(nPort); K2_UNUSED(fLow); K2_UNUSED(fHigh); K2_UNUSED(nMs);
#endif
    return 0;
}

int CExoInputInternal::GetRumbleEnabled()
{
    return m_pMainDevice->m_bRumbleEnabled;
}

int CExoInputInternal::IsPsShowButtonsController() const
{
    return m_pMainDevice->m_bIsDualShockLike;
}

void CExoInputInternal::SetRumbleEnabled(int a)
{
    m_pMainDevice->m_bRumbleEnabled = a;
}

void CExoInputInternal::PauseRumble()
{
    m_pMainDevice->m_bRumblePaused = 1;
}

int CExoInputInternal::GetActive()
{
    if (m_pMainDevice != 0)
        return m_pMainDevice->m_bActive;
    return 0;
}

int CExoInputInternal::GetPortBinding()
{
    return m_pMainDevice->GetPortBinding();
}

void CExoInputInternal::SetAutoCalInfo(unsigned int a)
{
    m_pMainDevice->SetAutoCalInfo(a);
}

void CExoInputInternal::ClearKeyboardEvents()
{
    m_pMainDevice->ClearKeyboardBuffer();
}

void CExoInputInternal::ClearMouseEvents()
{
    while (m_mouseEventList.m_pInternal->m_pHead != 0) {
        void* p = m_mouseEventList.m_pInternal->RemoveHead();
        if (p != 0)
            ::operator delete(p);
    }
}

void CExoInputInternal::SetActive(int a)
{
    m_bActive = a;
    if (m_pMainDevice != 0)
        m_pMainDevice->SetActive(a);
}

int CExoInputInternal::GetAutoCalInfo()
{
    return m_pMainDevice->GetAutoCalInfo();
}

void CExoInputInternal::SetPortBinding(unsigned int a)
{
    m_pMainDevice->SetPortBinding(a);
}

void CExoInputInternal::EndAttemptedBind()
{
}

int CExoInputInternal::GetMainDeviceLost()
{
    return m_pMainDevice->GetMainDeviceLost();
}

void CExoInputInternal::UpdateJoysticks()
{
}

void CExoInputInternal::UnpauseRumble()
{
    m_pMainDevice->m_bRumblePaused = 0;
}

int CExoInputInternal::GetRumblePaused()
{
    return m_pMainDevice->m_bRumblePaused;
}

void CExoInputInternal::HideMouse()
{
#if KOTOR1
    tagCURSORINFO info;
    info.cbSize = sizeof(info);
    if (m_nMouseMode == 0 && GetCursorInfo(&info) && (info.flags & 1)) {
        while (ShowCursor(0) >= 0)
            ;
    }
#else
    g_cursorShouldBeVisible = 0;
#endif
}

void CExoInputInternal::ShowMouse()
{
#if KOTOR1
    tagCURSORINFO info;
    info.cbSize = sizeof(info);
    if (m_nMouseMode == 0 && GetCursorInfo(&info) && !(info.flags & 1)) {
        while (ShowCursor(1) < 0)
            ;
    }
#else
    g_cursorShouldBeVisible = 1;
#endif
}

int CExoInputInternal::IsModalKeyDown_Shift()
{
#if KOTOR1
    return 0;
#else
    return (GetKeyState(VK_SHIFT) >> 15) & 1;
#endif
}

int CExoInputInternal::IsModalKeyDown_Ctrl()
{
#if KOTOR1
    return 0;
#else
    return (GetKeyState(VK_CONTROL) >> 15) & 1;
#endif
}

int CExoInputInternal::IsModalKeyDown_Alt()
{
#if KOTOR1
    return 0;
#else
    return (GetKeyState(VK_MENU) >> 15) & 1;
#endif
}

void CExoInputInternal::KeyboardModeOn()
{
    if (m_pMainDevice != 0)
        m_pMainDevice->KeyboardModeOn();
    m_nKeyboardMode = 1;
}

void CExoInputInternal::KeyboardModeOff()
{
    if (m_pMainDevice != 0) {
        m_pMainDevice->KeyboardModeOff();
        m_nKeyboardMode = 0;
    } else {
        m_nKeyboardMode = 1;
    }
}

void* CExoInputInternal::GetEvent(unsigned int nEvent, int nType)
{
    if (static_cast<unsigned int>(nType) <= 5) {
        if (IsEventInClass(nEvent, nType))
            return m_aEvents.m_pArray[static_cast<int>(nEvent)];
        return 0;
    }
    if (nType != -1)
        return 0;
    if (nEvent >= static_cast<unsigned int>(m_aEvents.m_count))
        return 0;
    return m_aEvents.m_pArray[static_cast<int>(nEvent)];
}

void CExoInputInternal::GetMouseDelta(float* a, float* b)
{
    *a = m_fMouseDeltaX;
    *b = m_fMouseDeltaY;
}

// Scans the device's keyboard buffer backwards through the captured-key table and returns the
// matching event index, or 0.
int CExoInputInternal::GetLastCapturedKeyboardKey()
{
    CExoDeviceBuffer buffer;
    int nResult = 0;
    if (m_pMainDevice != 0 && m_bCaptureKeyboardKeys != 0) {
        m_pMainDevice->GetKeyboardBuffer(buffer);
        if (buffer.m_nCount != 0) {
            int nKbStart = CExoInput::KEYBOARD_START;
            int nJoyStart = CExoInput::JOYSTICK_START;
            for (uint64_t i = 0; i < buffer.m_nCount; i++) {
                if (buffer.m_pEntries[i].m_flags & INPUTKEY_DOWN) {
                    int nKeyCode = buffer.m_pEntries[i].m_nKeyCode;
                    for (int j = nKbStart; j < nJoyStart; j++) {
                        if (nKeyCode == m_keyArray[j]) {
                            nResult = j;
                            break;
                        }
                    }
                }
            }
        }
    }
    return nResult;
}

#if KOTOR1
extern float Ios_lastfDeltaX;
extern float Ios_lastfDeltaY;
// The weight the previous frame's mouse delta keeps, so the delta smooths across frames.
__attribute__((visibility("hidden"))) volatile float g_fMouseDeltaSmoothing = 0.7f;

// Smooths the raw delta against the previous frame's and normalises it by 70 percent of
// fMaxMickies before clamping to [-1, 1].
void CExoInputInternal::UpdateMouseDelta(float param_1)
{
    K2_UNUSED(param_1);
    CExoMouseState state = {};
    float fX = 0.0f;
    float fY = 0.0f;
    if (m_pMainDevice->GetMouseState(state) == 1) {
        float fMax = fMaxMickies * 0.7f;
        Ios_lastfDeltaX = g_fMouseDeltaSmoothing * Ios_lastfDeltaX
                          + (static_cast<float>(-state.m_dx) / fMax) * (1.0f - g_fMouseDeltaSmoothing);
        Ios_lastfDeltaY = g_fMouseDeltaSmoothing * Ios_lastfDeltaY
                          + (static_cast<float>(-state.m_dy) / fMax) * (1.0f - g_fMouseDeltaSmoothing);
        fX = Ios_lastfDeltaX > 1.0f ? 1.0f : Ios_lastfDeltaX;
        fY = Ios_lastfDeltaY > 1.0f ? 1.0f : Ios_lastfDeltaY;
        fX = fX < -1.0f ? -1.0f : fX;
        fY = fY < -1.0f ? -1.0f : fY;
    }
    m_fMouseDeltaX = fX;
    m_fMouseDeltaY = fY;
}
#else
void CExoInputInternal::UpdateMouseDelta(float param_1)
{
    K2_UNUSED(param_1);
    CExoMouseState state = {};
    float fResult;
    if (m_pMainDevice->GetMouseState(state) == 1) {
        float fMax = fMaxMickies;
        float dx = static_cast<float>(-state.m_dx) / fMaxMickies;
        float vx = 1.0f;
        if (!(dx > 1.0f)) {
            vx = -1.0f;
            if (!(dx < -1.0f))
                vx = dx;
        }
        m_fMouseDeltaX = vx;
        float dy = static_cast<float>(-state.m_dy) / fMax;
        fResult = 1.0f;
        if (!(dy > 1.0f)) {
            fResult = -1.0f;
            if (!(dy < -1.0f))
                fResult = dy;
        }
    } else {
        fResult = 0.0f;
        m_fMouseDeltaX = 0.0f;
    }
    m_fMouseDeltaY = fResult;
}
#endif

// Tail-forwards the driver's lookup, which returns the character string by value, so the result
// pointer must pass through untouched.
CExoString CExoInputInternal::GetDisplayCharacterForKey(int key)
{
    return m_pMainDevice->GetDisplayCharacterDirectInputValue(m_keyArray[key]);
}

void CExoInputInternal::ClearEventRepeat(unsigned int nEvent)
{
    if (m_aEvents.m_pArray[static_cast<int>(nEvent)] != 0 && m_aEvents.m_pArray[static_cast<int>(nEvent)]->m_nEventClass == INPUTKIND_BUTTON) {
        CExoLinkedListNode* pNode = m_repeatList.m_pInternal->m_pHead;
        if (pNode != 0) {
            CExoInputRepeatEvent* pRec = static_cast<CExoInputRepeatEvent*>(m_repeatList.m_pInternal->GetAtPos(pNode));
            while (pRec == 0 || static_cast<unsigned int>(pRec->m_nEventIndex) != nEvent) {
                m_repeatList.m_pInternal->GetNext(pNode);
                if (pNode == 0)
                    return;
                pRec = static_cast<CExoInputRepeatEvent*>(m_repeatList.m_pInternal->GetAtPos(pNode));
            }
            pRec->m_nEmittedValue = 0;
            pRec->m_nRepeatCount = 0;
            pRec->m_nRepeatState = 0;
            pRec->m_nLatchedState = 0;
        }
    }
}

// Walks the repeat list for this event's record and unlinks it. The null-skipping inner loop keeps
// the entry lookup out of the loop header.
void CExoInputInternal::SetEventNonRepeatable(unsigned int nEvent)
{
    if (m_aEvents.m_pArray[static_cast<int>(nEvent)] != 0 && m_aEvents.m_pArray[static_cast<int>(nEvent)]->m_nEventClass == INPUTKIND_BUTTON) {
        CExoLinkedListNode* pNode = m_repeatList.GetHeadPos();
        if (pNode != 0) {
            CExoInputRepeatEvent* pRec = m_repeatList.GetAtPos(pNode);
            while (pRec == 0 || static_cast<unsigned int>(pRec->m_nEventIndex) != nEvent) {
                do {
                    m_repeatList.GetNext(pNode);
                    if (pNode == 0)
                        return;
                    pRec = m_repeatList.GetAtPos(pNode);
                } while (pRec == 0);
            }
            m_repeatList.Remove(pNode);
            ::operator delete(pRec);
        }
    }
}

int CExoInputInternal::SetEventRepeatable(unsigned int nEvent, unsigned int p2, unsigned int p3)
{
    if (m_aEvents.m_pArray[static_cast<int>(nEvent)] == 0 || m_aEvents.m_pArray[static_cast<int>(nEvent)]->m_nEventClass != INPUTKIND_BUTTON)
        return 0;
    CExoLinkedListNode* pNode = m_repeatList.m_pInternal->m_pHead;
    if (pNode != 0) {
        CExoInputRepeatEvent* pRec = static_cast<CExoInputRepeatEvent*>(m_repeatList.m_pInternal->GetAtPos(pNode));
        while (true) {
            if (pRec != 0 && static_cast<unsigned int>(pRec->m_nEventIndex) == nEvent) {
                pRec->m_nRepeatWait = p2;
                pRec->m_nRepeatRate = p3;
                return 1;
            }
            m_repeatList.m_pInternal->GetNext(pNode);
            if (pNode == 0)
                break;
            pRec = static_cast<CExoInputRepeatEvent*>(m_repeatList.m_pInternal->GetAtPos(pNode));
        }
    }
    CExoInputRepeatEvent* pNew = static_cast<CExoInputRepeatEvent*>(::operator new(sizeof(CExoInputRepeatEvent)));
    pNew->m_nEventIndex = nEvent;
    pNew->m_nRepeatWait = p2;
    pNew->m_nRepeatRate = p3;
    pNew->m_nEmittedValue = 0;
    pNew->m_nRepeatCount = 0;
    pNew->m_nRepeatState = 0;
    pNew->m_nLatchedState = 0;
    m_repeatList.m_pInternal->AddHead(pNew);
    return 1;
}

void CExoInputInternal::CoolDownEvent(unsigned int nEvent, unsigned int p2)
{
    CExoLinkedListInternal* pList = m_cooldownList.m_pInternal;
    CExoLinkedListNode* pNode = pList->m_pHead;
    CExoInputEventDesc** ppEvent = &m_aEvents.m_pArray[static_cast<int>(nEvent)];
    if (*ppEvent != 0) {
        (*ppEvent)->m_nCooldown = p2;
        while (pNode != 0) {
            CExoInputEventDesc** ppRec = static_cast<CExoInputEventDesc**>(pList->GetAtPos(pNode));
            if (*ppRec != 0 && (*ppRec)->m_nId == (*ppEvent)->m_nId) {
                if (p2 == 0)
                    m_cooldownList.Remove(pNode);
                return;
            }
            m_cooldownList.GetNext(pNode);
            pList = m_cooldownList.m_pInternal;
        }
        pList->AddTail(ppEvent);
    }
}

int CExoInputInternal::SetMouseMode(unsigned char mode)
{
    if (m_nMouseMode != mode) {
        m_nMouseMode = mode;
        if (mode == 1) {
#if KOTOR1
            while (ShowCursor(0) >= 0)
                ;
#else
            g_cursorShouldBeVisible = 0;
#endif
        }
        unsigned int cid = m_nCursorId;
        m_nLastMouseDownState = 0;
        if (cid == MOUSECURSOR_NONE)
            cid = MOUSECURSOR_DEFAULT;
        m_nCursorId = cid;
#if KOTOR1
        if (m_nMouseMode == 0) {
#else
        if (mode == 0) {
#endif
            void* h = LoadCursorA(g_hInst, cid & 0xffff);
            if (h != 0) {
                void* prev = SetCursor_Win32(h);
                if (prev != h)
                    DestroyCursor(prev);
            }
        }
    }
    return 1;
}

unsigned int CExoInputInternal::SetMouseCursor(int nId, int p2, int flag)
{
    K2_UNUSED(p2);
    m_nLastMouseDownState = flag;
    unsigned int idx = nId;
    if (flag != 0)
        idx = nId + nId % 2;
    if (m_nCursorId != idx) {
        m_nCursorId = nId;
        if (m_nMouseMode == 0) {
            void* h = LoadCursorA(g_hInst, idx & 0xffff);
            if (h != 0) {
                void* prev = SetCursor_Win32(h);
                if (prev != h)
                    DestroyCursor(prev);
            }
        }
    }
    return idx;
}

void CExoInputInternal::RefreshMouseCursor(int p1)
{
    unsigned int cid = m_nCursorId;
    m_nCursorId = MOUSECURSOR_NONE;
    m_nLastMouseDownState = p1;
    if (cid == MOUSECURSOR_NONE)
        cid = MOUSECURSOR_DEFAULT;
    unsigned int idx = cid;
    if (p1 != 0) {
        idx = cid + static_cast<int>(cid) % 2;
        if (idx == MOUSECURSOR_NONE)
            return;
    }
    m_nCursorId = cid;
    if (m_nMouseMode == 0) {
        void* h = LoadCursorA(g_hInst, idx & 0xffff);
        if (h != 0) {
            void* prev = SetCursor_Win32(h);
            if (prev != h)
                DestroyCursor(prev);
        }
    }
}

// The port's cursor-visibility hooks, three free functions implementing the policy of hiding the
// mouse pointer once the player has been idle or is on a gamepad, on top of the Win32 shims.

#include "port/win32api.h"

// The render surface's pixel height; SetMousePos flips a bottom-left-origin y into the top-left
// origin the shims expect.
extern int g_nScreenHeight;

// The gamepad-presence query has C++ linkage, so it stays outside the C shim block above.
bool IsJoystickConnected();

// Records what has actually been pushed to the OS; the policy input it answers to is declared in
// CExoInputInternal.h alongside the render window handle.
extern int g_cursorCurrentlyVisible;
int g_cursorCurrentlyVisible = 1;

// The idle tracker.
static POINT s_lastCursorPos;
static bool  s_bLastCursorPosValid;
static float s_fCursorIdleTime;

// Always returns 1; the real query was presumably the cursor-visibility flag.
bool IsCursorCurrentlyVisible()
{
    return true;
}

// The client-rect result is discarded while the other three calls are tested.
bool IsCursorInsideWindow()
{
    POINT pt;
    pt.x = 0;
    pt.y = 0;

    HWND__* hWnd = g_hRenderWnd;

    if (GetCursorPos(&pt) == 0)
        return false;
    if (ScreenToClient(hWnd, &pt) == 0)
        return false;

    RECT rc;
    rc.left = 0;
    rc.top = 0;
    rc.right = 0;
    rc.bottom = 0;
    GetClientRect(hWnd, &rc);

    if (PtInRect_Win32(&rc, pt) == 0)
        return false;
    return true;
}

// The name is misspelled in the engine ("Cusor") and the typo is part of the exported symbol, so
// it stays. The two cursor-counter loops differ in their exit test: the show loop spins while the
// counter is still negative, the hide loop while it is still non-negative.
void UpdateCusorVisible(float fElapsed)
{
    POINT pt;
    GetCursorPos(&pt);

    if (!s_bLastCursorPosValid) {
        GetCursorPos(&s_lastCursorPos);
        s_bLastCursorPosValid = true;
    }

    float fIdle = 0.0f;
    if (pt.x == s_lastCursorPos.x && pt.y == s_lastCursorPos.y && IsJoystickConnected())
        fIdle = s_fCursorIdleTime + fElapsed;

    s_lastCursorPos = pt;

    int bVisible = (g_cursorShouldBeVisible != 0) && (fIdle < 400.0f);
    s_fCursorIdleTime = fIdle;

    if (g_cursorCurrentlyVisible != bVisible) {
        g_cursorCurrentlyVisible = bVisible;
        if (bVisible) {
            while (ShowCursor_Win32(1) < 0)
                ;
        } else {
            while (ShowCursor_Win32(0) >= 0)
                ;
        }
    }
}

int CExoInputEventDesc::IsDetailed()
{
    return 0;
}

int CExoInputEventDesc::Is2Button()
{
    return 0;
}

int CExoInputEventDescDetailed::IsDetailed()
{
    return 1;
}

int CExoInputeventDesc2ButtonAxis::Is2Button()
{
    return 1;
}

// Records (or grows the bit mask for) event id `nEvent` in class `nType`, then reports whether it is
// set. The query and the lazy grow are fused: the bit mask is an array that is resized to
// nEvent/32*2 + 1 words the first time an id past its end is asked about, so this is as much an
// ensure-capacity as a query.
int CExoInputInternal::IsEventInClass(unsigned int nEvent, int nType)
{
    if (static_cast<unsigned int>(nType) <= 5) {
        int nCount = m_aEventClass[nType].m_aEventBits.m_count;
        int nWord = static_cast<int>(nEvent) / 32;
        if (nWord >= nCount) {
            int nOld = m_aEventClass[nType].m_aEventBits.m_count;
            int nSize = nWord * 2 + 1;
            m_aEventClass[nType].m_aEventBits.SetSize(nSize);
            for (int i = nOld; i < nSize; i++)
                m_aEventClass[nType].m_aEventBits.m_pArray[i] = 0;
        }
        if (m_aEventClass[nType].m_aEventBits.m_pArray[nWord] & (1 << nEvent))
            return 1;
    }
    return 0;
}

// Finds the descriptor that already binds control `nControl` of device `nDevice` within event class
// `nType`, or NULL. A two-button-axis descriptor matches on either of its two codes, which is the
// only reason the virtual Is2Button dispatch is here. The guard order is nType, nControl, nDevice.
CExoInputEventDesc* CExoInputInternal::IsControlUsed(unsigned int nControl, int nDevice, int nType)
{
    if (static_cast<unsigned int>(nType) > 5)
        return 0;
    if (nControl > INPUTCONTROL_LAST)
        return 0;
    if (static_cast<unsigned int>(nDevice) > 5)
        return 0;
    CExoLinkedListNode* pNode = m_aEventClass[nType].m_apDeviceLists[nDevice].GetHeadPos();
    while (pNode != 0) {
        int nEvent = *reinterpret_cast<int*>(m_aEventClass[nType].m_apDeviceLists[nDevice].GetAtPos(pNode));
        CExoInputEventDesc* pDesc = m_aEvents.m_pArray[nEvent];
        if (pDesc->m_nKeyCode == static_cast<int>(nControl))
            return pDesc;
        if (pDesc->Is2Button() &&
            static_cast<CExoInputeventDesc2ButtonAxis*>(pDesc)->m_nKeyCode2 == static_cast<int>(nControl))
            return pDesc;
        m_aEventClass[nType].m_apDeviceLists[nDevice].GetNext(pNode);
    }
    return 0;
}

// Builds the descriptor for event id `nEvent`. The concrete kind is picked by `nEventClass`: 3 is
// the analogue-axis descriptor, 4 the two-button-axis one, and 0/1/2/5 the plain binding.
// sic: a class outside 0..5 stores a NULL descriptor into the slot and still reports success, so the
// event looks present when it is not.
int CExoInputInternal::CreateNewEvent(unsigned int nEvent, int nEventClass, int nDevice,
                                      unsigned int nKeyCode, unsigned int nKeyCode2)
{
    if (nDevice > 5)
        return 0;
#if KOTOR1  // K1: the map key is always bound to joystick button 3, whatever the caller passes
    if (nEvent == INPUTEVENT_PANEL_MAP)
        nKeyCode = CExoInput::JOYSTICK_BUTTON3;
#endif
    if (nEvent >= static_cast<unsigned int>(m_aEvents.m_count)) {
        int nOld = m_aEvents.m_count;
        int nSize = nEvent * 2 + 1;
        m_aEvents.SetSize(nSize);
        for (int i = nOld; i < nSize; i++)
            m_aEvents.m_pArray[i] = 0;
    }
    if (m_aEvents.m_pArray[static_cast<int>(nEvent)] != 0)
        return 0;

    CExoInputEventDesc* pDesc;
    switch (nEventClass) {
    case INPUTKIND_STATE:
    case INPUTKIND_BUTTON:
    case INPUTKIND_STATE_B:
    case INPUTKIND_STATE_C:
        pDesc = new CExoInputEventDesc(nEvent, nEventClass, nDevice);
        pDesc->m_nKeyCode = static_cast<int>(nKeyCode);
        break;
    case INPUTKIND_AXIS:
        pDesc = new CExoInputEventDescDetailed(nEvent, 3, nDevice);
        pDesc->m_nKeyCode = static_cast<int>(nKeyCode);
        break;
    case INPUTKIND_BUTTON_AXIS:
        pDesc = new CExoInputeventDesc2ButtonAxis(nEvent, 4, nDevice, nKeyCode2);
        pDesc->m_nKeyCode = static_cast<int>(nKeyCode);
        break;
    default:
        pDesc = 0;
        break;
    }
    m_aEvents.m_pArray[static_cast<int>(nEvent)] = pDesc;
    return 1;
}

// Drops every binding in every class and device list and clears every class's registration bit mask.
// sic: the event array's count is zeroed without deleting the descriptors it still owns, so every
// descriptor built by CreateNewEvent leaks here.
void CExoInputInternal::ClearAllInputEventDescriptions()
{
    for (int64_t nClass = 0; nClass < 6; nClass++) {
        for (int64_t nDevice = 0; nDevice < m_nDeviceCount; nDevice++) {
            while (m_aEventClass[nClass].m_apDeviceLists[nDevice].m_pInternal->m_pHead != 0)
                m_aEventClass[nClass].m_apDeviceLists[nDevice].m_pInternal->RemoveHead();
        }
        for (int64_t i = 0; i < m_aEventClass[nClass].m_aEventBits.m_count; i++)
            m_aEventClass[nClass].m_aEventBits.m_pArray[i] = 0;
    }
    m_aEvents.m_count = 0;
}

// ---------------------------------------------------------------------------------------------
// Event logging.

// sic: the guard is an OR where it plainly wants an AND, so the body runs whenever the file is
// non-null *or* the index is out of range. A null file with an out-of-range index walks off the
// event array and then writes to null. Only DumpEventsToFile reaches this, and it passes a non-null
// file and an in-range index, so it never fires.
void CExoInputInternal::WriteEventToFile(unsigned int nTime, int nIndex, CExoFile* pFile)
{
    if (nIndex < 0 || pFile != 0 || nIndex >= m_nEventQueueCount) {
        CExoInputEventDesc* pDesc = m_eventQueue.m_pArray[nIndex].m_pEvent;
        CExoString sLine;
        sLine.Format(const_cast<char*>("%d %c %d %d\n"), nTime, 'I', pDesc->m_nId,
                     m_eventQueue.m_pArray[nIndex].m_nValue);
        pFile->Write(sLine);
    }
}

void CExoInputInternal::DumpEventsToFile(CExoFile* pFile)
{
    if (m_eventQueue.m_count != 0 && m_nEventQueueCount != 0) {
        unsigned int nTime = g_pExoBase->m_pTimers->GetLowResolutionTimer();
        for (int i = 0; i < m_nEventQueueCount; i++)
            WriteEventToFile(nTime, i, pFile);
    }
}

// Attaches a per-axis scale to an event. If it already has one, i.e. it is a detailed descriptor,
// this is a plain field update; otherwise the plain descriptor is promoted: a detailed one is built
// with the old binding copied across, swapped into the slot, and re-registered under the device list
// the old one had (RemoveEvent hands back the device index, which AddEvent takes). Promotion is only
// offered for classes 0, 2 and 5, which is the class-bit test below.
int CExoInputInternal::ScaleEvent(unsigned int nEvent, float fScaleX, float fScaleY)
{
    int bScaled = 0;
    CExoInputEventDesc* pDesc = m_aEvents.m_pArray[static_cast<int>(nEvent)];
    if (pDesc != 0) {
        if (pDesc->IsDetailed()) {
            static_cast<CExoInputEventDescDetailed*>(pDesc)->m_fScaleX = fScaleX;
            static_cast<CExoInputEventDescDetailed*>(pDesc)->m_fScaleY = fScaleY;
            static_cast<CExoInputEventDescDetailed*>(pDesc)->m_bScaled = 1;
            bScaled = 1;
        } else if (static_cast<unsigned int>(pDesc->m_nEventClass) <= 5 &&
                   ((1 << pDesc->m_nEventClass) &
                    ((1 << INPUTKIND_STATE) | (1 << INPUTKIND_STATE_B) | (1 << INPUTKIND_STATE_C))) != 0) {
            CExoInputEventDescDetailed* pNew = new CExoInputEventDescDetailed;
            bScaled = 1;
            pNew->m_nState = pDesc->m_nState;
            pNew->m_nClassCount = pDesc->m_nClassCount;
            pNew->m_nCooldown = pDesc->m_nCooldown;
            pNew->m_nId = pDesc->m_nId;
            pNew->m_nEventClass = pDesc->m_nEventClass;
            pNew->m_nDevice = pDesc->m_nDevice;
            pNew->m_nKeyCode = pDesc->m_nKeyCode;
            pNew->m_fScaleX = fScaleX;
            pNew->m_fScaleY = fScaleY;
            pNew->m_bScaled = 1;
            pNew->m_nAxisState = 0;
            unsigned int nDevice = RemoveEvent(nEvent);
            m_aEvents.m_pArray[static_cast<int>(nEvent)] = pNew;
            AddEvent(nEvent, nDevice);
        }
    }
    return bScaled;
}

// Warp the OS cursor, then push the new position straight into whatever detailed mouse-axis
// descriptors the pending queue is holding, so the frame that is mid-flight sees the warp rather
// than the delta it would otherwise compute. `y` arrives bottom-left-origin and is flipped.
#if KOTOR1
// K1 has no keep-forced flag: every warp marks the position forced.
void CExoInputInternal::SetMousePos(int x, int y)
{
    POINT pt;
    pt.x = x;
    pt.y = g_nScreenHeight - y - 1;
    ClientToScreen(g_hRenderWnd, &pt);
    SetCursorPos(pt.x, pt.y);
    m_bMousePosForced = 1;
    for (int64_t i = 0; i < m_eventQueue.m_count; i++) {
        CExoInputEventDesc* pDesc = m_eventQueue.m_pArray[i].m_pEvent;
        if (pDesc != 0) {
            if (static_cast<unsigned int>(pDesc->m_nKeyCode) == CExoInput::MOUSE_XAXIS && pDesc->IsDetailed())
                pDesc->m_nState = x;
            if (static_cast<unsigned int>(pDesc->m_nKeyCode) == CExoInput::MOUSE_YAXIS && pDesc->IsDetailed())
                pDesc->m_nState = y;
        }
    }
}
#endif

void CExoInputInternal::SetMousePos(int x, int y, bool bKeepForcedFlag)
{
    POINT pt;
    pt.x = x;
    pt.y = g_nScreenHeight - y - 1;
    ClientToScreen(g_hRenderWnd, &pt);
    SetCursorPos(pt.x, pt.y);
    if (!bKeepForcedFlag)
        m_bMousePosForced = 1;
    for (int64_t i = 0; i < m_eventQueue.m_count; i++) {
        CExoInputEventDesc* pDesc = m_eventQueue.m_pArray[i].m_pEvent;
        if (pDesc != 0) {
            if (static_cast<unsigned int>(pDesc->m_nKeyCode) == CExoInput::MOUSE_XAXIS && pDesc->IsDetailed())
                pDesc->m_nState = x;
            if (static_cast<unsigned int>(pDesc->m_nKeyCode) == CExoInput::MOUSE_YAXIS && pDesc->IsDetailed())
                pDesc->m_nState = y;
        }
    }
}

// Files an existing descriptor into one of the six event classes: refuse if it is already there,
// refuse if either of its control codes is already taken within that class, then file it under the
// class's list for its device and light its bit in the class bit mask. The list payload is the
// descriptor's own id field, which is why every walker reads a node's data as an int pointer.
int CExoInputInternal::AddEvent(unsigned int nEvent, unsigned int nEventClass)
{
    if (nEventClass > 5)
        return 0;
    CExoInputEventDesc* pDesc = m_aEvents.m_pArray[static_cast<int>(nEvent)];
    if (pDesc == 0)
        return 0;
    if (IsEventInClass(nEvent, static_cast<int>(nEventClass)))
        return 0;

    CExoInputEventDesc* pOther = 0;
    unsigned int nControl = static_cast<unsigned int>(pDesc->m_nKeyCode);
    if (nControl != INPUTCONTROL_NONE)
        pOther = IsControlUsed(nControl, pDesc->m_nDevice, static_cast<int>(nEventClass));
    if (pOther == 0 && pDesc->Is2Button()) {
        nControl = static_cast<unsigned int>(static_cast<CExoInputeventDesc2ButtonAxis*>(pDesc)->m_nKeyCode2);
        if (nControl != INPUTCONTROL_NONE)
            pOther = IsControlUsed(nControl, pDesc->m_nDevice, static_cast<int>(nEventClass));
    }
    if (pOther != 0)
        return 0;

    pDesc->m_nClassCount++;
    if (pDesc->m_nDevice == CExoInput::INPUTDEVICE_NONE) {
        m_aEventClass[nEventClass].m_unboundList.m_pInternal->AddHead(&pDesc->m_nId);
    } else {
        m_aEventClass[nEventClass].m_apDeviceLists[pDesc->m_nDevice].m_pInternal->AddHead(&pDesc->m_nId);
        int nWord = static_cast<int>(nEvent) / 32;
        m_aEventClass[nEventClass].m_aEventBits.m_pArray[nWord] |= 1 << nEvent;
    }
    return 1;
}

// Unregisters an event: clears its slot, finds which class it is in, unlinks it from that class's
// device list, and destroys the descriptor. Returns the class it was found in, or (unsigned)-1, so
// ScaleEvent can file the promoted descriptor under the same class again.
//
// sic, two engine bugs live here:
//  (1) the node scan never compares anything: nFoundId starts at 0 and is never reassigned (the
//      GetAtPos result is dropped), so the match test collapses to `nEvent != 0` and any non-zero
//      id unlinks whatever node happens to be at the list head.
//  (2) the bit mask update stores a whole zero word rather than clearing one bit, so removing any
//      event silently deregisters the other 31 ids that share its word.
unsigned int CExoInputInternal::RemoveEvent(unsigned int nEvent)
{
    CExoInputEventDesc* pDesc = m_aEvents.m_pArray[static_cast<int>(nEvent)];
    if (pDesc == 0)
        return static_cast<unsigned int>(-1);
    m_aEvents.m_pArray[static_cast<int>(nEvent)] = 0;

    int nClass = -1;
    for (int i = 0; i <= 5; i++) {
        if (IsEventInClass(nEvent, i)) {
            nClass = i;
            break;
        }
    }
    if (nClass == -1)
        return static_cast<unsigned int>(-1);

    CExoLinkedListNode* pNode = m_aEventClass[nClass].m_apDeviceLists[pDesc->m_nDevice].GetHeadPos();
    unsigned int nFoundId = 0;
    while (pNode != 0) {
        m_aEventClass[nClass].m_apDeviceLists[pDesc->m_nDevice].GetAtPos(pNode);
        if (nFoundId != nEvent)
            break;
        m_aEventClass[nClass].m_apDeviceLists[pDesc->m_nDevice].GetNext(pNode);
    }
    if (pNode == 0)
        return static_cast<unsigned int>(-1);

    m_aEventClass[nClass].m_apDeviceLists[pDesc->m_nDevice].Remove(pNode);
    int nWord = static_cast<int>(nEvent) / 32;
    m_aEventClass[nClass].m_aEventBits.m_pArray[nWord] = 0;
    ::operator delete(pDesc);
    return static_cast<unsigned int>(nClass);
}

// The absolute value lowers to a single instruction in this TU.
#include "port/libcmath.h"

// Maps one raw control reading onto the [-1,1] (or [0,1]) range the game logic wants. Only
// descriptors carrying a scale (m_bScaled, set by ScaleEvent) are rescaled; the rest hand the raw
// count straight back. Three control families get three curves: the Return..Space key band is a
// flat 1/128, the two mouse axes are a hard clamp of value/100, and the joystick axes and sliders are
// normalised against the device's own usable range.
//
// sic: the slider test is a vacuous `&&`.
float CExoInputInternal::ScaledValue(CExoInputEventDesc* pDesc, unsigned int nControl,
                                     unsigned int nRawValue)
{
    float fValue = static_cast<float>(static_cast<int>(nRawValue));
    if (!pDesc->IsDetailed())
        return fValue;
    if (static_cast<CExoInputEventDescDetailed*>(pDesc)->m_bScaled == 0)
        return fValue;

    if (static_cast<unsigned int>(CExoInput::KEYBOARD_RETURN) <= nControl &&
        static_cast<unsigned int>(CExoInput::KEYBOARD_SPACE) >= nControl)
        return fValue * 0.0078125f;

    if (static_cast<unsigned int>(CExoInput::MOUSE_XAXIS) == nControl ||
        static_cast<unsigned int>(CExoInput::MOUSE_YAXIS) == nControl) {
        float fScaled = fValue / 100.0f;
        float fClamped;
        if (fScaled > 1.0f)
            fClamped = 1.0f;
        else if (fScaled < -1.0f)
            fClamped = -1.0f;
        else
            fClamped = fScaled;
#if KOTOR1  // K1: the mouse delta is whole steps only
        return static_cast<float>(static_cast<int>(fClamped));
#else
        return fClamped;
#endif
    }

    if (static_cast<unsigned int>(CExoInput::JOYSTICK_XAXIS) == nControl ||
        static_cast<unsigned int>(CExoInput::JOYSTICK_YAXIS) == nControl ||
        !(static_cast<unsigned int>(CExoInput::JOYSTICK_SLIDER0) > nControl &&
          static_cast<unsigned int>(CExoInput::JOYSTICK_SLIDER2) < nControl)) {
        if (static_cast<int>(nRawValue) == 0)
            return fValue;
        float fMax = m_pMainDevice->GetMaxUseable(pDesc->m_nDevice, m_keyArray[nControl]);
        float fMin = m_pMainDevice->GetMinUseable(pDesc->m_nDevice, m_keyArray[nControl]);
        float fScaled = (fabsf(fValue) - fMin + -1.0f) / (fMax - fMin);
        if (fScaled > 1.0f)
            fScaled = 1.0f;
        float fSign = 1.0f;
        if (static_cast<int>(nRawValue) < 0)
            fSign = -1.0f;
        return fSign * fScaled;
    }
    return fValue;
}

// Read one event's current value. The descriptor's class decides how its raw state becomes a
// number: a plain button or axis reports its own state, an analogue axis reports the change since the
// last poll (and latches the new value), and a two-button axis is the difference of its two
// halves. A detailed descriptor then gets its response curve applied (`powf` by m_fScaleY,
// sign-preserved, times m_fScaleX).
float CExoInputInternal::PollInput(unsigned int nEvent, int nEventClass)
{
    if (static_cast<unsigned int>(nEventClass) > 5)
        return 0.0f;
    if (!IsEventInClass(nEvent, nEventClass))
        return 0.0f;

#if KOTOR1
    CExoInputEventDesc* pDesc = static_cast<CExoInputEventDesc*>(GetEvent(nEvent, nEventClass));
#else
    CExoInputEventDesc* pDesc = 0;
    if (IsEventInClass(nEvent, nEventClass))
        pDesc = m_aEvents.m_pArray[static_cast<int>(nEvent)];
#endif
    K2_ASSUME(pDesc != 0);   // a registered event's slot is never empty (AddEvent tests it)
    float fValue = 0.0f;
    switch (pDesc->m_nEventClass) {
    case INPUTKIND_STATE:
    case INPUTKIND_STATE_B:
    case INPUTKIND_STATE_C:
        fValue = ScaledValue(pDesc, pDesc->m_nKeyCode, pDesc->m_nState);
        break;
    case INPUTKIND_BUTTON:
        return 0.0f;
    case INPUTKIND_AXIS:
        fValue += ScaledValue(pDesc, pDesc->m_nKeyCode,
                              static_cast<CExoInputEventDescDetailed*>(pDesc)->m_nAxisState - pDesc->m_nState);
        pDesc->m_nState = static_cast<CExoInputEventDescDetailed*>(pDesc)->m_nAxisState;
        break;
    case INPUTKIND_BUTTON_AXIS:
        fValue = ScaledValue(pDesc, pDesc->m_nKeyCode,
                             static_cast<CExoInputeventDesc2ButtonAxis*>(pDesc)->m_nAxisState);
        fValue -= ScaledValue(pDesc, static_cast<CExoInputeventDesc2ButtonAxis*>(pDesc)->m_nKeyCode2,
                              static_cast<CExoInputeventDesc2ButtonAxis*>(pDesc)->m_nAxisState2);
        break;
    default:
        break;
    }
    if (pDesc->IsDetailed()) {
        float fCurved = powf(fValue, static_cast<CExoInputEventDescDetailed*>(pDesc)->m_fScaleY);
        if (fCurved > 0.0f && fValue < 0.0f)
            fCurved = -fCurved;
        fValue = static_cast<CExoInputEventDescDetailed*>(pDesc)->m_fScaleX * fCurved;
    }
    return fValue;
}

// The Android port's "back button was pressed" latch, an exported byte consumed by the client's
// per-frame input pass and reset here along with everything else.
extern bool g_BackButtonEvent;
// Clears every pending input: flushes the device's own buffers, resets each descriptor's per-frame
// state (the two axis kinds carry extra state words), zeroes the repeat timers, and frees the queued
// mouse events. The per-frame state reset is repeated inside every arm of the switch rather than
// once after it.
void CExoInputInternal::ClearEvents()
{
    if (m_pMainDevice == 0)
        return;
    m_pMainDevice->ClearMouseBuffer();
    m_pMainDevice->ClearKeyboardBuffer();

    for (int64_t i = 0; i < m_aEvents.m_count; i++) {
        CExoInputEventDesc* pDesc = m_aEvents.m_pArray[i];
        if (pDesc != 0) {
            switch (pDesc->m_nEventClass) {
            case INPUTKIND_STATE:
            case INPUTKIND_BUTTON:
            case INPUTKIND_STATE_B:
            case INPUTKIND_STATE_C:
                pDesc->m_nState = 0;
                break;
            case INPUTKIND_AXIS:
                static_cast<CExoInputEventDescDetailed*>(pDesc)->m_nAxisState = 0;
                pDesc->m_nState = 0;
                break;
            case INPUTKIND_BUTTON_AXIS:
                static_cast<CExoInputeventDesc2ButtonAxis*>(pDesc)->m_nAxisState = 0;
                static_cast<CExoInputeventDesc2ButtonAxis*>(pDesc)->m_nAxisState2 = 0;
                pDesc->m_nState = 0;
                break;
            }
        }
    }

    CExoLinkedListNode* pNode = m_repeatList.m_pInternal->m_pHead;
    while (pNode != 0) {
        CExoInputRepeatEvent* pRec = static_cast<CExoInputRepeatEvent*>(m_repeatList.m_pInternal->GetAtPos(pNode));
        pRec->m_nRepeatState = 0;
        pRec->m_nRepeatCount = 0;
        m_repeatList.m_pInternal->GetNext(pNode);
    }

    while (m_mouseEventList.m_pInternal->m_pHead != 0) {
        CExoInputEvent* pEvent = static_cast<CExoInputEvent*>(m_mouseEventList.m_pInternal->RemoveHead());
        if (pEvent != 0) {
            if (pEvent->m_pEvent != 0)
                ::operator delete(pEvent->m_pEvent);
            ::operator delete(pEvent);
        }
    }
#if !KOTOR1
    g_BackButtonEvent = 0;
#endif
}

// One device's right-stick rest-position calibration. Some pads park a spring-loaded stick off
// centre, so the first sample a device produces is latched as rest and the axis stays dead until a
// sample differs from it.
struct CExoJoystickEnabledInfo {
    CExoJoystickEnabledInfo() { m_bHaveRest = false; m_bEnabled = false; }
    bool  m_bHaveRest;
    bool  m_bEnabled;
    float m_fRestX;
    float m_fRestY;
};

#if !KOTOR1  // K2-only (no K1 counterpart)
// Sums every attached joystick's stick N into one normalised [-1,1] pair: stick 0 is the first axis
// pair, stick 1 the second. Samples inside a 20 percent dead zone contribute nothing; everything
// else is scaled by 1/32768 and accumulated, clamped after each device so several pads cannot drive
// the result past 1.
void CExoInputInternal::GetJoystickNStick(int nStick, float* pfX, float* pfY)
{
    static CExoJoystickEnabledInfo s_enabledInfo[6];

    float fX = 0.0f;
    float fY = 0.0f;

    CExoJoystickState* pState = m_pMainDevice->GetJoystickState();
    if (pState != 0) {
        // sic: the bound is narrowed to a byte here, while every other reader of the same field loads
        // a full 32-bit int, so the truncation is in this expression rather than in the field's
        // declared type.
        for (int i = 0; i < static_cast<unsigned char>(m_pMainDevice->m_nNumJoysticks); i++) {
            if (!s_isDeviceValid[i])
                continue;

            int nRawX = (nStick == 1) ? pState[i].m_lRx : pState[i].m_lX;
            int nRawY = (nStick == 1) ? pState[i].m_lRy : pState[i].m_lY;
            float fRawX = static_cast<float>(nRawX);
            float fRawY = static_cast<float>(nRawY);

            if (nStick == 1) {
                if (!s_enabledInfo[i].m_bHaveRest) {
                    s_enabledInfo[i].m_fRestX = fRawX;
                    s_enabledInfo[i].m_fRestY = fRawY;
                    s_enabledInfo[i].m_bHaveRest = true;
                }
                if (!s_enabledInfo[i].m_bEnabled) {
                    if (s_enabledInfo[i].m_fRestX != fRawX || s_enabledInfo[i].m_fRestY != fRawY)
                        s_enabledInfo[i].m_bEnabled = true;
                    else
                        continue;
                }
            }

            float fAddX = 0.0f;
            float fAddY = 0.0f;
            if (fRawX > 6553.0f || fRawX < -6553.0f)
                fAddX = fRawX / 32768.0f;
            if (fRawY > 6553.0f || fRawY < -6553.0f)
                fAddY = fRawY / 32768.0f;

            fX += fAddX;
            fY += fAddY;
            if (fX > 1.0f)
                fX = 1.0f;
            if (fX < -1.0f)
                fX = -1.0f;
            if (fY > 1.0f)
                fY = 1.0f;
            if (fY < -1.0f)
                fY = -1.0f;
        }
    }

    *pfX = fX;
    *pfY = fY;
}
#endif // !KOTOR1

// Queues one input event: a CExoInputEvent carrying the raw value, owning a freshly built
// CExoInputEventDesc that records which control on which device produced it. The descriptor is class
// 0 with no id, because this queue is read back by GetEvents rather than through the binding tables,
// so only the device and key code matter.
void CExoInputInternal::BufferEvent(int nDevice, unsigned int nControl, unsigned int nValue)
{
    CExoInputEvent* pEvent = new CExoInputEvent();
    pEvent->m_nValue = static_cast<int>(nValue);
    pEvent->m_nTimeStamp = 0;
    pEvent->m_nSequence = 0;

    CExoInputEventDesc* pDesc = new CExoInputEventDesc(0, 0, nDevice);
    pDesc->m_nKeyCode = static_cast<int>(nControl);
    pEvent->m_pEvent = pDesc;

    m_mouseEventList.AddHead(pEvent);
}

// A button's value in a double-click message.
enum : unsigned int { MOUSEBUTTON_DOUBLECLICK = 0xffffffffu };

// Translates one Win32 mouse message into queued input events. Every button and wheel message first
// flushes the pointer position as an x/y axis pair so the click lands at the right place even if no
// move message arrived first, then queues the button itself (1 down, 0 up, -1 double-click). The
// forced-position flag short-circuits that: SetMousePos sets it, so the move message the OS echoes
// back from the port's own SetCursorPos is swallowed rather than reported as user motion, and a
// button message skips the position flush for the same reason.
//
// sic: the middle-button messages extract their coordinates unsigned where every other case
// sign-extends them.
int CExoInputInternal::BufferEvent(void* pMessage)
{
    MSG* pMsg = static_cast<MSG*>(pMessage);
    unsigned int nControl;
    unsigned int nValue;

    switch (pMsg->message) {
    case WM_MOUSEMOVE: {
        if (m_bMousePosForced) {
            m_bMousePosForced = 0;
        } else {
            int nPos = static_cast<int>(pMsg->lParam);
            m_nMouseX = static_cast<short>(nPos);
            m_nMouseY = nPos >> 16;
        }
        return 1;
    }

    case WM_LBUTTONDOWN: {
        if (m_bMousePosForced) {
            m_bMousePosForced = 0;
        } else {
            int nPos = static_cast<int>(pMsg->lParam);
            BufferEvent(CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_XAXIS, static_cast<short>(nPos));
            BufferEvent(CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_YAXIS, nPos >> 16);
            m_nMouseX = -1;
            m_nMouseY = -1;
        }
        nControl = CExoInput::MOUSE_BUTTON0;
        nValue = 1;
        break;
    }

    case WM_LBUTTONUP: {
        if (m_bMousePosForced) {
            m_bMousePosForced = 0;
        } else {
            int nPos = static_cast<int>(pMsg->lParam);
            BufferEvent(CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_XAXIS, static_cast<short>(nPos));
            BufferEvent(CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_YAXIS, nPos >> 16);
            m_nMouseX = -1;
            m_nMouseY = -1;
        }
        nControl = CExoInput::MOUSE_BUTTON0;
        nValue = 0;
        break;
    }

    case WM_LBUTTONDBLCLK: {
        if (m_bMousePosForced) {
            m_bMousePosForced = 0;
        } else {
            int nPos = static_cast<int>(pMsg->lParam);
            BufferEvent(CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_XAXIS, static_cast<short>(nPos));
            BufferEvent(CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_YAXIS, nPos >> 16);
            m_nMouseX = -1;
            m_nMouseY = -1;
        }
        nControl = CExoInput::MOUSE_BUTTON0;
        nValue = MOUSEBUTTON_DOUBLECLICK;
        break;
    }

    case WM_RBUTTONDOWN: {
        if (m_bMousePosForced) {
            m_bMousePosForced = 0;
        } else {
            int nPos = static_cast<int>(pMsg->lParam);
            BufferEvent(CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_XAXIS, static_cast<short>(nPos));
            BufferEvent(CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_YAXIS, nPos >> 16);
            m_nMouseX = -1;
            m_nMouseY = -1;
        }
        nControl = CExoInput::MOUSE_BUTTON1;
        nValue = 1;
        break;
    }

    case WM_RBUTTONUP: {
        if (m_bMousePosForced) {
            m_bMousePosForced = 0;
        } else {
            int nPos = static_cast<int>(pMsg->lParam);
            BufferEvent(CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_XAXIS, static_cast<short>(nPos));
            BufferEvent(CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_YAXIS, nPos >> 16);
            m_nMouseX = -1;
            m_nMouseY = -1;
        }
        nControl = CExoInput::MOUSE_BUTTON1;
        nValue = 0;
        break;
    }

    case WM_RBUTTONDBLCLK: {
        if (m_bMousePosForced) {
            m_bMousePosForced = 0;
        } else {
            int nPos = static_cast<int>(pMsg->lParam);
            BufferEvent(CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_XAXIS, static_cast<short>(nPos));
            BufferEvent(CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_YAXIS, nPos >> 16);
            m_nMouseX = -1;
            m_nMouseY = -1;
        }
        nControl = CExoInput::MOUSE_BUTTON1;
        nValue = MOUSEBUTTON_DOUBLECLICK;
        break;
    }

    case WM_MBUTTONDOWN: {
        if (m_bMousePosForced) {
            m_bMousePosForced = 0;
        } else {
            unsigned int nPos = static_cast<unsigned int>(pMsg->lParam);
            BufferEvent(CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_XAXIS, static_cast<unsigned short>(nPos));
            BufferEvent(CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_YAXIS, nPos >> 16);
            m_nMouseX = -1;
            m_nMouseY = -1;
        }
        nControl = CExoInput::MOUSE_BUTTON2;
        nValue = 1;
        break;
    }

    case WM_MBUTTONUP: {
        if (m_bMousePosForced) {
            m_bMousePosForced = 0;
        } else {
            unsigned int nPos = static_cast<unsigned int>(pMsg->lParam);
            BufferEvent(CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_XAXIS, static_cast<unsigned short>(nPos));
            BufferEvent(CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_YAXIS, nPos >> 16);
            m_nMouseX = -1;
            m_nMouseY = -1;
        }
        nControl = CExoInput::MOUSE_BUTTON2;
        nValue = 0;
        break;
    }

    // sic: the middle-button double-click message is deliberately absent, which leaves a hole in the
    // message table whose entry points straight at the return-0 exit.
    case WM_MOUSEWHEEL: {
        if (m_bMousePosForced) {
            m_bMousePosForced = 0;
        } else {
            int nPos = static_cast<int>(pMsg->lParam);
            BufferEvent(CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_XAXIS, static_cast<short>(nPos));
            BufferEvent(CExoInput::INPUTDEVICE_MOUSE, CExoInput::MOUSE_YAXIS, nPos >> 16);
            m_nMouseX = -1;
            m_nMouseY = -1;
        }
        nControl = CExoInput::MOUSE_ZAXIS;
        nValue = static_cast<int>(pMsg->wParam) >> 16;
        break;
    }

    default:
        return 0;
    }

    BufferEvent(CExoInput::INPUTDEVICE_MOUSE, nControl, nValue);
    return 1;
}

// The per-frame input pump harvests the device buffers, matches them against the binding lists,
// and queues the resulting events.

// The port's "the pad layout changed, re-enumerate the joysticks" latch.
extern int gResetController_Mac;
#if KOTOR1  // the K1 port's Chromebook key and pad button latches
extern int pressedKey;
extern unsigned int pressedGamepadButtons;
extern unsigned int pressedGamepadButtonsThisFrame;
#endif
// Set when a hot-plug removed a pad last frame; the client's per-frame pass picks it up.
extern char g_GamePauseNeeded;

#if KOTOR1
// The K1 port's pad buttons, SDL_main's bits in pressedGamepadButtons (src/port/Mac/MacMain.cpp).
enum GamepadButton {
    GAMEPAD_A = 0x1,
    GAMEPAD_B = 0x2,
    GAMEPAD_X = 0x4,
    GAMEPAD_Y = 0x8,
    GAMEPAD_LB = 0x10,
    GAMEPAD_RB = 0x20,
    GAMEPAD_LT = 0x40,
    GAMEPAD_RT = 0x80,
    GAMEPAD_UP = 0x100,
    GAMEPAD_DOWN = 0x200,
    GAMEPAD_LEFT = 0x400,
    GAMEPAD_RIGHT = 0x800,
    GAMEPAD_PAUSE = 0x1000,
    GAMEPAD_RSTICK_UP = 0x2000,
    GAMEPAD_RSTICK_DOWN = 0x4000,
    GAMEPAD_START = 0x80000,
    GAMEPAD_BACK = 0x100000,
    GAMEPAD_LSCLICK = 0x200000,
    GAMEPAD_RSCLICK = 0x400000,
};

// The SDL scancodes of the Chromebook keyboard shortcuts.
enum ChromebookScancode {
    SCANCODE_RETURN = 40,
    SCANCODE_ESCAPE = 41,
    SCANCODE_TAB = 43,
    SCANCODE_RIGHT = 79,
    SCANCODE_LEFT = 80,
    SCANCODE_DOWN = 81,
    SCANCODE_UP = 82,
    SCANCODE_RETURN2 = 158,
};
#else
// The pad's buttons, as the joystick device numbers them (rgbButtons indices).
enum PadButton {
    PADBUTTON_A = 0,
    PADBUTTON_B = 1,
    PADBUTTON_X = 2,
    PADBUTTON_Y = 3,
    PADBUTTON_LEFT_SHOULDER = 4,
    PADBUTTON_RIGHT_SHOULDER = 5,
    PADBUTTON_BACK = 6,
    PADBUTTON_START = 7,
    PADBUTTON_LEFT_STICK = 8,
    PADBUTTON_RIGHT_STICK = 9,
    PADBUTTON_DPAD_UP = 12,
    PADBUTTON_DPAD_RIGHT = 13,
    PADBUTTON_DPAD_DOWN = 14,
    PADBUTTON_DPAD_LEFT = 15,
};

// A POV hat's direction in the joystick buffer: degrees clockwise from up.
enum PovDegrees {
    POVDEGREES_UP = 0,
    POVDEGREES_RIGHT = 90,
    POVDEGREES_DOWN = 180,
    POVDEGREES_LEFT = 270,
};

// The input classes the pad mapping reports to: all but free look.
enum {
    kPadInputClasses = (1 << INPUTCLASS_GAME) | (1 << INPUTCLASS_MINIGAME) | (1 << INPUTCLASS_GUI) |
                       (1 << INPUTCLASS_DIALOG) | (1 << INPUTCLASS_MOVIE),
};
#endif

CExoArrayList<CExoInputEvent> CExoInputInternal::GetEvents(int& nCount, int nInputClass)
{
#if !KOTOR1  // K1 has no pad hot-plug re-enumeration
    if (gResetController_Mac != 0) {
        int nOldCount = m_pMainDevice->m_nNumJoysticks;
        m_pMainDevice->ReinitJoysticks();
        gResetController_Mac = 0;
        int nNewCount = m_pMainDevice->m_nNumJoysticks;
        m_nDeviceCount = nNewCount + 2;
        if (nNewCount < nOldCount)
            g_GamePauseNeeded = 1;
    }
#endif

    CExoDeviceBuffer aBuffers[6];

    nCount = 0;
    m_eventQueue.m_count = 0;
    if (m_pMainDevice == 0)
        return CExoArrayList<CExoInputEvent>();
    if (nInputClass > 5) {
        g_pExoBase->m_pDebug->Warning(
#if KOTOR1
            1619,
            const_cast<char*>("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/libsource/exoinput/exoinputinternal.cpp"),
#else
            1543,
            const_cast<char*>("C:/Users/Vu/Documents/AndroidStudioProjects/KOTOR1-2/KOTOR2_Android/app/src/main/cpp/kotor2/CODE/libsource/exoinput/exoinputinternal.cpp"),
#endif
            "CExoInputInternal::GetEvents() Invalid InputClass parameter");
        return CExoArrayList<CExoInputEvent>();
    }

    // Per-device binding-list cursors.
    CExoLinkedListNode* apNodes[6];
    int nDevices = m_nDeviceCount;
    for (int i = 0; i < nDevices; i++)
        apNodes[i] = m_aEventClass[nInputClass].m_apDeviceLists[i].GetHeadPos();

    // Harvest the keyboard buffer (unless it was captured up-stack) and then one buffer per pad.
    if (!m_bCaptureKeyboardKeys) {
        m_pMainDevice->GetKeyboardBuffer(aBuffers[CExoInput::INPUTDEVICE_KEYBOARD]);
        nDevices = m_nDeviceCount;
    }
    if (nDevices > 2) {
        for (int i = 0; i < nDevices - 2; i++) {
            m_pMainDevice->GetJoystickBuffer(i, aBuffers[i + 2]);
            nDevices = m_nDeviceCount;
        }
    }

    unsigned int aConsumed[6];
    bool bAllEmpty = true;
    int nTotal = 0;
    if (nDevices > 0) {
        __builtin_memset(aConsumed, 0, sizeof(unsigned int) * nDevices);
        for (int i = 0; i < nDevices; i++) {
            if (aBuffers[i].m_nCount != 0)
                bAllEmpty = false;
        }
        m_nEventQueueCount = 0;
        for (int i = 0; i < nDevices; i++)
            nTotal += aBuffers[i].m_nCount;
    }
    else {
        m_nEventQueueCount = 0;
    }
    if (nDevices > 0 && nTotal != 0 && !bAllEmpty) {
        // The per-frame key pump. The device buffers hold raw scancodes and the binding lists hold
        // event ids whose descriptors name the control each id is bound to. Pick a device, walk its
        // binding list for a descriptor whose control maps to the device's next buffered scancode,
        // and let that descriptor absorb the buffered value. nBest is the device consumed last pass:
        // while its buffer still holds entries every other device is skipped, which is what makes the
        // drain round-robin instead of device-first.
        int nBest = -1;
        int nLastKey = 0;
        bool bAllConsumed;
        do {
            // Every early exit from a pass is a goto to the next pass, never a continue. A continue
            // would re-test the stale all-consumed flag and can spin forever when a buffered key has
            // no binding in this class.
            {
            if (nDevices < 1)
                goto next_pass;
            // The selection pass finds the first device (subject to nBest) with a matching
            // descriptor.
            bool bFound = false;
            for (int i = 0; i < nDevices; nDevices = m_nDeviceCount, i++) {
                if (nBest != -1 && i != nBest && aConsumed[nBest] < aBuffers[nBest].m_nCount)
                    continue;
                if (!bFound) {
                    if (aConsumed[i] < aBuffers[i].m_nCount) {
                        CExoLinkedList<int*>& rList = m_aEventClass[nInputClass].m_apDeviceLists[i];
                        bool bMatch;
                        do {
                            bMatch = false;
                            apNodes[i] = rList.GetHeadPos();
                            CExoLinkedListNode* pNode = apNodes[i];
                            if (pNode != 0) {
                                CExoInputKeyEvent* pEntry = &aBuffers[i].m_pEntries[aConsumed[i]];
                                int* pnId = reinterpret_cast<int*>(rList.GetHead());
                                do {
                                    CExoInputEventDesc* pDesc = m_aEvents.m_pArray[*pnId];
                                    if (m_keyArray[static_cast<unsigned int>(pDesc->m_nKeyCode)] == pEntry->m_nKeyCode ||
                                        (pDesc->Is2Button() != 0 &&
                                         m_keyArray[static_cast<unsigned int>(static_cast<CExoInputeventDesc2ButtonAxis*>(pDesc)->m_nKeyCode2)] == pEntry->m_nKeyCode)) {
                                        bMatch = true;
                                        break;
                                    }
                                    pnId = reinterpret_cast<int*>(rList.GetNext(apNodes[i]));
                                } while (apNodes[i] != 0);
                            }
                            if (bMatch) {
                                bFound = true;
                                break;
                            }
                            aConsumed[i] = aConsumed[i] + 1;
                            bFound = false;
                        } while (aConsumed[i] < aBuffers[i].m_nCount);
                    }
                    else {
                        bFound = false;
                    }
                }
            }
            if (!bFound)
                goto next_pass;
            // The second pass re-finds the device, honouring the key latched last pass.
            int nIdx = 0;
            for (; nIdx < nDevices; nIdx++) {
                if (aConsumed[nIdx] >= aBuffers[nIdx].m_nCount)
                    continue;
                if (nBest == -1)
                    nLastKey = aBuffers[nIdx].m_pEntries[aConsumed[nIdx]].m_nSequence;
                else if (nLastKey != aBuffers[nIdx].m_pEntries[aConsumed[nIdx]].m_nSequence)
                    continue;
                break;
            }
            if (nIdx >= nDevices) {
                nLastKey = nLastKey + 1;
                nBest = -1;
                goto next_pass;
            }
            // Let the bound descriptor absorb the buffered value.
            {
                CExoInputKeyEvent* pEntry = &aBuffers[nIdx].m_pEntries[aConsumed[nIdx]];
                int* pnId = reinterpret_cast<int*>(m_aEventClass[nInputClass].m_apDeviceLists[nIdx].GetAtPos(apNodes[nIdx]));
                CExoInputEventDesc* pDesc = m_aEvents.m_pArray[*pnId];
                if (pDesc->m_nCooldown == 0 && static_cast<unsigned int>(pDesc->m_nEventClass) <= 5) {
                    switch (pDesc->m_nEventClass) {
                    case INPUTKIND_STATE:
                    case INPUTKIND_STATE_B:
                    case INPUTKIND_STATE_C:
                        pDesc->m_nState = pEntry->m_nValue;
                        break;
                    case INPUTKIND_BUTTON:
                        if (m_pMainDevice->IsDigital(nIdx, pEntry->m_nKeyCode) != 0) {
                            pDesc->m_nState = pEntry->m_nValue;
                            CExoInputEvent sEvent;
                            sEvent.m_nValue = pEntry->m_nValue;
                            sEvent.m_nTimeStamp = pEntry->m_nTimeStamp;
                            sEvent.m_nSequence = pEntry->m_nSequence;
                            sEvent.m_pEvent = pDesc;
                            m_eventQueue.Add(sEvent);
                            ++m_nEventQueueCount;
                        }
                        else {
                            int nThreshold = m_pMainDevice->GetControlThreshold(pEntry->m_nKeyCode);
                            int nValue = pEntry->m_nValue;
                            int nState = nValue >= -nThreshold ? static_cast<int>(nValue > nThreshold) : -1;
                            int nLastValue = m_pMainDevice->GetLastState(nIdx, pEntry->m_nKeyCode);
                            int nLastState = nLastValue >= -nThreshold ? static_cast<int>(nLastValue > nThreshold) : -1;
                            if (nState != nLastState) {
                                pDesc->m_nState = nState;
                                CExoInputEvent sEvent;
                                sEvent.m_nValue = nState;
                                sEvent.m_nTimeStamp = pEntry->m_nTimeStamp;
                                sEvent.m_nSequence = pEntry->m_nSequence;
                                sEvent.m_pEvent = pDesc;
                                m_eventQueue.Add(sEvent);
                                ++m_nEventQueueCount;
                            }
                        }
                        break;
                    case INPUTKIND_AXIS:
#if KOTOR1  // K1 accumulates every axis sample (no digital pass-through)
                        static_cast<CExoInputEventDescDetailed*>(pDesc)->m_nAxisState += pEntry->m_nValue;
#else
                        if (m_pMainDevice->IsDigital(nIdx, pEntry->m_nKeyCode) != 0) {
                            pDesc->m_nState = pEntry->m_nValue;
                            CExoInputEvent sEvent;
                            sEvent.m_nValue = pEntry->m_nValue;
                            sEvent.m_nTimeStamp = pEntry->m_nTimeStamp;
                            sEvent.m_nSequence = pEntry->m_nSequence;
                            sEvent.m_pEvent = pDesc;
                            m_eventQueue.Add(sEvent);
                            ++m_nEventQueueCount;
                        }
                        else {
                            static_cast<CExoInputEventDescDetailed*>(pDesc)->m_nAxisState += pEntry->m_nValue;
                        }
#endif
                        break;
                    case INPUTKIND_BUTTON_AXIS:
                        if (pEntry->m_nKeyCode == m_keyArray[static_cast<unsigned int>(pDesc->m_nKeyCode)])
                            static_cast<CExoInputEventDescDetailed*>(pDesc)->m_nAxisState = pEntry->m_nValue;
                        else
                            static_cast<CExoInputeventDesc2ButtonAxis*>(pDesc)->m_nAxisState2 = pEntry->m_nValue;
                        break;
                    }
                }
            }
            aConsumed[nIdx] = aConsumed[nIdx] + 1;
            nBest = nIdx;
            // Done when every device buffer is fully consumed.
            }
        next_pass:
            bAllConsumed = true;
            for (int i = 0; i < nDevices; i++)
                bAllConsumed = bAllConsumed && (aConsumed[i] >= aBuffers[i].m_nCount);
        } while (!bAllConsumed);
    }

    // Synthesise the pending mouse-axis events.
    int* pnEventQueueCount = &m_nEventQueueCount;
    if (m_pMainDevice != 0 && m_pMainDevice->m_bActive != 0) {
        if (m_nMouseX != -1) {
            CExoInputEvent* pEvent = static_cast<CExoInputEvent*>(::operator new(sizeof(CExoInputEvent)));
            pEvent->m_nValue = 0;
            pEvent->m_nTimeStamp = 0;
            pEvent->m_nSequence = 0;
            pEvent->m_nReserved1 = 0;
            pEvent->m_pEvent = 0;
            pEvent->m_nValue = m_nMouseX;
            CExoInputEventDesc* pDesc = new CExoInputEventDesc(0, 0, CExoInput::INPUTDEVICE_MOUSE);
            pEvent->m_pEvent = pDesc;
            pDesc->m_nKeyCode = CExoInput::MOUSE_XAXIS;
            m_mouseEventList.AddHead(pEvent);
        }
        if (m_nMouseY != -1) {
            CExoInputEvent* pEvent = static_cast<CExoInputEvent*>(::operator new(sizeof(CExoInputEvent)));
            pEvent->m_nValue = 0;
            pEvent->m_nTimeStamp = 0;
            pEvent->m_nSequence = 0;
            pEvent->m_nReserved1 = 0;
            pEvent->m_pEvent = 0;
            pEvent->m_nValue = m_nMouseY;
            CExoInputEventDesc* pDesc = new CExoInputEventDesc(0, 0, CExoInput::INPUTDEVICE_MOUSE);
            pEvent->m_pEvent = pDesc;
            pDesc->m_nKeyCode = CExoInput::MOUSE_YAXIS;
            m_mouseEventList.AddHead(pEvent);
        }
    }

    // Drain the queued mouse events into the per-device bound descriptors.
    CExoLinkedListInternal* pMouseList = m_mouseEventList.m_pInternal;
    while (pMouseList->m_pHead != 0) {
        CExoInputEvent* pEvent = static_cast<CExoInputEvent*>(pMouseList->RemoveTail());
        CExoLinkedListInternal* pBound = m_aEventClass[nInputClass].m_apDeviceLists[pEvent->m_pEvent->m_nDevice].m_pInternal;
        CExoLinkedListNode* pNode = pBound->m_pHead;
        while (pNode != 0) {
            int* pnId = static_cast<int*>(pBound->GetAtPos(pNode));
            CExoInputEventDesc* pFound = m_aEvents.m_pArray[*pnId];
            if (static_cast<int>(static_cast<unsigned char>(pFound->m_nKeyCode)) == pEvent->m_pEvent->m_nKeyCode) {
                if (pFound->IsDetailed() != 0)
                    static_cast<CExoInputEventDescDetailed*>(pFound)->m_nAxisState = pEvent->m_nValue;
                CExoInputEvent sEvent;
                sEvent.m_nValue = pEvent->m_nValue;
                sEvent.m_nTimeStamp = pEvent->m_nTimeStamp;
                sEvent.m_nSequence = pEvent->m_nSequence;
                sEvent.m_pEvent = pFound;
                m_eventQueue.Add(sEvent);
                *pnEventQueueCount = *pnEventQueueCount + 1;
                break;
            }
            pBound->GetNext(pNode);
        }
        if (pEvent->m_pEvent != 0)
            ::operator delete(pEvent->m_pEvent);
        ::operator delete(pEvent);
        pMouseList = m_mouseEventList.m_pInternal;
    }
    m_nMouseX = -1;
    m_nMouseY = -1;

    // Decay the cooldown timers.
    unsigned int nNow = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
    int& nLastTimer = m_nLastTimer;
    unsigned int nElapsed = nNow - nLastTimer;
    nLastTimer = static_cast<int>(nNow);
    CExoLinkedListInternal* pCooldown = m_cooldownList.m_pInternal;
    CExoLinkedListNode* pCd = pCooldown->m_pHead;
    while (pCd != 0) {
        CExoLinkedListNode* pCur = pCd;
        // The cooldown list stores pointers into the event array (see CoolDownEvent), so what is
        // decayed here is the descriptor's own cooldown field.
        CExoInputEventDesc** ppRec = static_cast<CExoInputEventDesc**>(pCooldown->GetAtPos(pCd));
        CExoInputEventDesc* pRec = *ppRec;
        if (pRec == 0) {
            pCooldown->GetNext(pCd);
        }
        else {
            unsigned int nLeft = static_cast<unsigned int>(pRec->m_nCooldown) - nElapsed;
            if (static_cast<unsigned int>(pRec->m_nCooldown) <= nElapsed) {
                pRec->m_nCooldown = 0;
                pCooldown->GetNext(pCd);
                pCooldown->Remove(pCur);
            }
            else {
                pRec->m_nCooldown = static_cast<int>(nLeft);
                pCooldown->GetNext(pCd);
            }
        }
        if (pCd == 0)
            break;
        pCooldown = m_cooldownList.m_pInternal;
    }

    // Walk the repeat list, latching each repeat-timer record.
    CExoLinkedListInternal* pRepeat = m_repeatList.m_pInternal;
    CExoLinkedListNode* pRep = pRepeat->m_pHead;
    while (pRep != 0) {
        CExoInputRepeatEvent* pRec = static_cast<CExoInputRepeatEvent*>(pRepeat->GetAtPos(pRep));
        if (pRec != 0 && IsEventInClass(pRec->m_nEventIndex, nInputClass) != 0) {
            // The repeat record mirrors the bound descriptor's current state word. A nonzero state
            // arms the timer, a state change restarts it, and the two thresholds (the first-repeat and
            // subsequent-repeat delays) decide when a repeat event is due.
            CExoInputEventDesc* pDesc = m_aEvents.m_pArray[pRec->m_nEventIndex];
            int nState = pDesc->m_nState;
            if (nState == 0) {
                pRec->m_nRepeatState = 0;
                pRec->m_nLatchedState = 0;
                pRec->m_nEmittedValue = 0;
                pRec->m_nRepeatCount = 0;
            }
            else if (pRec->m_nRepeatState == 0 && pRec->m_nRepeatCount == 0) {
                pRec->m_nRepeatState = 1;
                pRec->m_nLatchedState = nState;
                pRec->m_nEmittedValue = nState;
            }
            else if (pRec->m_nLatchedState != nState) {
                pRec->m_nRepeatState = 1;
                pRec->m_nLatchedState = nState;
                pRec->m_nEmittedValue = nState;
                pRec->m_nRepeatCount = 0;
                break;
            }
            else {
                unsigned int nRepeat = static_cast<unsigned int>(pRec->m_nRepeatState) + nElapsed;
                pRec->m_nRepeatState = static_cast<int>(nRepeat);
                if (pRec->m_nRepeatCount == 0) {
                    if (static_cast<unsigned int>(pRec->m_nRepeatWait) < nRepeat) {
                        pRec->m_nEmittedValue = 0;
                        CExoInputEvent sEvent;
                        sEvent.m_nValue = 0;
                        sEvent.m_nTimeStamp = 0;
                        sEvent.m_nSequence = 0;
                        sEvent.m_pEvent = pDesc;
                        m_eventQueue.Add(sEvent);
                        *pnEventQueueCount = *pnEventQueueCount + 1;
                        pRec->m_nRepeatCount = 1;
                        pRec->m_nRepeatState = 0;
                    }
                }
                else if (static_cast<unsigned int>(pRec->m_nRepeatRate) < nRepeat) {
                    int nValue = pRec->m_nEmittedValue != nState ? nState : 0;
                    pRec->m_nEmittedValue = nValue;
                    CExoInputEvent sEvent;
                    sEvent.m_nValue = nValue;
                    sEvent.m_nTimeStamp = 0;
                    sEvent.m_nSequence = 0;
                    sEvent.m_pEvent = pDesc;
                    m_eventQueue.Add(sEvent);
                    *pnEventQueueCount = *pnEventQueueCount + 1;
                    pRec->m_nRepeatState = 0;
                }
            }
        }
        pRepeat->GetNext(pRep);
        if (pRep == 0)
            break;
        pRepeat = m_repeatList.m_pInternal;
    }

    // The hardcoded pad mapping is the port's mobile layout for a game controller. aKeys holds five
    // press/release event-id pairs; the switch fills the pair each joystick code drives, then the
    // per-input-class slot in cClassPairBase selects which pair this class reports.
    // The buffer read is the first joystick's, which is the one the harvest above fills.
    {
#if KOTOR1
        // K1's port: one Chromebook keyboard key and the pad's button masks become press/release
        // event-id pairs, and the input class picks the pair.
        static const int64_t cClassPairBase[4] = { 1, 2, 0, 0 };
        unsigned int nThisFrame = pressedGamepadButtonsThisFrame;
        unsigned int nHeld = pressedGamepadButtons;
#ifdef K2_UI_PC
        // The keyboard reaches the key map as itself through GetKeyboardBuffer; the Chromebook
        // shortcuts would answer the same keys a second time.
        pressedKey = 0;
#endif
        while (SDL_IsChromebook() != 0 && pressedKey != 0) {
            int aKeys[6] = { -1, -1, -1, -1, -1, -1 };
            switch (pressedKey) {
            case SCANCODE_RETURN: case SCANCODE_RETURN2:
                aKeys[0] = INPUTEVENT_KEY_RETURN;
                aKeys[3] = INPUTEVENT_KEY_RETURN;
                aKeys[4] = INPUTEVENT_MG_SHOOT;
                break;
            case SCANCODE_ESCAPE:
                aKeys[0] = INPUTEVENT_GUI;
                aKeys[2] = INPUTEVENT_GUI;
                break;
            case SCANCODE_TAB:
                aKeys[0] = INPUTEVENT_PREV_MENU;
                aKeys[2] = INPUTEVENT_PREV_MENU;
                break;
            case SCANCODE_RIGHT:
                aKeys[0] = INPUTEVENT_KEY_RIGHT;
                aKeys[2] = INPUTEVENT_KEY_RIGHT;
                break;
            case SCANCODE_LEFT:
                aKeys[0] = INPUTEVENT_KEY_LEFT;
                aKeys[2] = INPUTEVENT_KEY_LEFT;
                break;
            case SCANCODE_DOWN:
                aKeys[0] = INPUTEVENT_KEY_DOWN;
                aKeys[2] = INPUTEVENT_KEY_DOWN;
                break;
            case SCANCODE_UP:
                aKeys[0] = INPUTEVENT_KEY_UP;
                aKeys[2] = INPUTEVENT_KEY_UP;
                break;
            }
            if (static_cast<unsigned int>(nInputClass) < 4) {
                int* paPair = &aKeys[cClassPairBase[nInputClass] * 2];
                for (int i = 1; i >= 0; i--) {
                    int nKey = paPair[i];
                    if (nKey >= 0) {
                        static CExoInputEventDesc sKeyDescs[2];
                        sKeyDescs[i].m_nId = nKey;
                        CExoInputEvent sEvent;
                        sEvent.m_nValue = 1;
                        sEvent.m_nTimeStamp = 0;
                        sEvent.m_nSequence = 0;
                        sEvent.m_pEvent = &sKeyDescs[i];
                        m_eventQueue.Add(sEvent);
                        ++m_nEventQueueCount;
                    }
                }
            }
            pressedKey = 0;
        }
        for (unsigned int nBit = 1; (nThisFrame | nHeld) != 0; nBit <<= 1) {
            if (((nThisFrame | nHeld) & nBit) != 0) {
                int bPressed = (nThisFrame & nBit) != 0;
                int aKeys[6] = { -1, -1, -1, -1, -1, -1 };
                switch (nBit) {
                case GAMEPAD_A:
                    if (bPressed) {
                        aKeys[0] = INPUTEVENT_GUI_A;
                        aKeys[3] = INPUTEVENT_GUI_A;
                        aKeys[4] = INPUTEVENT_MG_SHOOT;
                    }
                    break;
                case GAMEPAD_B:
                    if (bPressed) {
                        aKeys[0] = INPUTEVENT_GUI_B;
                        aKeys[2] = INPUTEVENT_GUI_B;
                        aKeys[4] = INPUTEVENT_MINIGAME_ACCELERATE;
                    }
                    break;
                case GAMEPAD_X:
                    if (bPressed) {
                        aKeys[0] = INPUTEVENT_GUI_X;
                        aKeys[2] = INPUTEVENT_GUI_X;
                    }
                    break;
                case GAMEPAD_Y:
                    if (bPressed) {
                        aKeys[0] = INPUTEVENT_GUI_Y;
                        aKeys[2] = INPUTEVENT_GUI_Y;
                    }
                    break;
#ifdef K1_UI_XBOX
                // The console UI's switch table has shoulder and trigger paging, Start for the
                // in-game menu, Back for solo mode, and the stick click for free look. In menus the
                // triggers page and the shoulders take the two adjacent codes. X and Y reach the
                // HUD as the queue and clear-queue keys.
                case GAMEPAD_LB:
                    if (bPressed) {
                        aKeys[0] = INPUTEVENT_GUI_WHITE;
                        aKeys[2] = INPUTEVENT_PAUSE;
                    }
                    break;
                case GAMEPAD_RB:
                    if (bPressed) {
                        aKeys[0] = INPUTEVENT_GUI_BLACK;
                        aKeys[2] = INPUTEVENT_CHANGE_CHARACTER;
                    }
                    break;
                case GAMEPAD_LT:
                    if (bPressed) {
                        aKeys[0] = INPUTEVENT_GUI_LEFT_SHOULDER;
                        aKeys[2] = INPUTEVENT_SELECT_PREV;
                    }
                    break;
                case GAMEPAD_RT:
                    if (bPressed) {
                        aKeys[0] = INPUTEVENT_GUI_RIGHT_SHOULDER;
                        aKeys[2] = INPUTEVENT_SELECT_NEXT;
                    }
                    break;
                case GAMEPAD_START:
                    if (bPressed) {
                        aKeys[0] = INPUTEVENT_GUI_START;
                        aKeys[2] = INPUTEVENT_GUI;
                        aKeys[4] = INPUTEVENT_PAUSE_MINIGAME;
                    }
                    break;
                case GAMEPAD_BACK:
                    if (bPressed) {
                        aKeys[0] = INPUTEVENT_GUI_BACK;
                        aKeys[2] = INPUTEVENT_PARTY_ACTIVE;
                    }
                    break;
                case GAMEPAD_LSCLICK:
                    if (bPressed)
                        aKeys[2] = INPUTEVENT_FREELOOK_EXIT_ALT;
                    break;
                case GAMEPAD_RSCLICK:
                    if (bPressed)
                        aKeys[2] = INPUTEVENT_KEY_CAPSLOCK;
                    break;
#else
                case GAMEPAD_LB:
                    if (bPressed) {
                        aKeys[0] = INPUTEVENT_GUI_LEFT_SHOULDER;
                        aKeys[2] = INPUTEVENT_SELECT_PREV;
                    }
                    break;
                case GAMEPAD_RB:
                    if (bPressed) {
                        aKeys[0] = INPUTEVENT_GUI_RIGHT_SHOULDER;
                        aKeys[2] = INPUTEVENT_SELECT_NEXT;
                    }
                    break;
                case GAMEPAD_LT:
                    if (bPressed)
                        aKeys[2] = INPUTEVENT_FLOURISH;
                    break;
                case GAMEPAD_RT:
                    if (bPressed)
                        aKeys[2] = INPUTEVENT_CHANGE_CHARACTER;
                    break;
#endif
                case GAMEPAD_UP:
                    if (bPressed) {
                        aKeys[0] = INPUTEVENT_GUI_DPAD_UP;
                        aKeys[2] = INPUTEVENT_GUI_DPAD_UP;
                    }
                    break;
                case GAMEPAD_DOWN:
                    if (bPressed) {
                        aKeys[0] = INPUTEVENT_GUI_DPAD_DOWN;
                        aKeys[2] = INPUTEVENT_GUI_DPAD_DOWN;
                    }
                    break;
                case GAMEPAD_LEFT:
                    if (bPressed) {
                        aKeys[0] = INPUTEVENT_GUI_DPAD_LEFT;
                        aKeys[2] = INPUTEVENT_GUI_DPAD_LEFT;
                    }
                    break;
                case GAMEPAD_RIGHT:
                    if (bPressed) {
                        aKeys[0] = INPUTEVENT_GUI_DPAD_RIGHT;
                        aKeys[2] = INPUTEVENT_GUI_DPAD_RIGHT;
                    }
                    break;
                case GAMEPAD_PAUSE:
                    if (bPressed) {
                        aKeys[2] = INPUTEVENT_PAUSE;
                        aKeys[4] = INPUTEVENT_PAUSE_MINIGAME;
                    }
                    break;
                case GAMEPAD_RSTICK_UP:
                    aKeys[0] = INPUTEVENT_GUI_SCROLL_UP;
                    break;  // held: repeats every frame
                case GAMEPAD_RSTICK_DOWN:
                    aKeys[0] = INPUTEVENT_GUI_SCROLL_DOWN;
                    break;
                }
                if (static_cast<unsigned int>(nInputClass) < 4) {
                    int* paPair = &aKeys[cClassPairBase[nInputClass] * 2];
                    for (int i = 1; i >= 0; i--) {
                        int nKey = paPair[i];
                        if (nKey >= 0) {
                            static CExoInputEventDesc sButtonDescs[2];
                            sButtonDescs[i].m_nId = nKey;
                            CExoInputEvent sEvent;
                            sEvent.m_nValue = 1;
                            sEvent.m_nTimeStamp = 0;
                            sEvent.m_nSequence = 0;
                            sEvent.m_pEvent = &sButtonDescs[i];
                            m_eventQueue.Add(sEvent);
                            ++m_nEventQueueCount;
                        }
                    }
                }
            }
            nThisFrame &= ~nBit;
            nHeld &= ~nBit;
        }
#else
        int aKeys[10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
        if (aBuffers[2].m_nCount != 0) {
            for (int i = 0; i < static_cast<int>(aBuffers[2].m_nCount); i++) {
                CExoInputKeyEvent* pEntry = &aBuffers[2].m_pEntries[i];
                int nKey = pEntry->m_nKeyCode;
                int nValue = pEntry->m_nValue;
                if (static_cast<unsigned int>(nKey - DIJOFS_BUTTON0) < DIJOFS_BUTTON_END - DIJOFS_BUTTON0) {
                    if (nValue != 0) {
                        switch (nKey - DIJOFS_BUTTON0) {
                        case PADBUTTON_A:
                            aKeys[0] = INPUTEVENT_GUI_A;
                            aKeys[3] = INPUTEVENT_GUI_A;
                            aKeys[4] = INPUTEVENT_MG_SHOOT;
                            aKeys[8] = INPUTEVENT_MOVIE_SKIP_A;
                            break;
                        case PADBUTTON_B:
                            aKeys[0] = INPUTEVENT_GUI_B;
                            aKeys[2] = INPUTEVENT_GUI_B;
                            aKeys[4] = INPUTEVENT_MINIGAME_ACCELERATE;
                            aKeys[8] = INPUTEVENT_MOVIE_SKIP_B;
                            break;
                        case PADBUTTON_X:
                            aKeys[0] = INPUTEVENT_GUI_X;
                            aKeys[2] = INPUTEVENT_GUI_X;
                            aKeys[8] = INPUTEVENT_MOVIE_SKIP_X;
                            break;
                        case PADBUTTON_Y:
                            aKeys[0] = INPUTEVENT_GUI_Y;
                            aKeys[2] = INPUTEVENT_GUI_Y;
                            aKeys[4] = INPUTEVENT_CANCEL_COMBAT;
                            aKeys[8] = INPUTEVENT_MOVIE_SKIP_Y;
                            break;
                        case PADBUTTON_LEFT_SHOULDER:
                            aKeys[0] = INPUTEVENT_GUI_LEFT_SHOULDER;
                            aKeys[2] = INPUTEVENT_SELECT_PREV;
                            break;
                        case PADBUTTON_RIGHT_SHOULDER:
                            aKeys[0] = INPUTEVENT_GUI_RIGHT_SHOULDER;
                            aKeys[2] = INPUTEVENT_SELECT_NEXT;
                            break;
                        case PADBUTTON_BACK:
                            aKeys[0] = INPUTEVENT_GUI_BACK;
                            break;
                        case PADBUTTON_START:
                            aKeys[0] = INPUTEVENT_GUI_START;
                            aKeys[2] = INPUTEVENT_PAUSE;
                            aKeys[4] = INPUTEVENT_PAUSE_MINIGAME;
                            break;
                        case PADBUTTON_LEFT_STICK:
                            aKeys[2] = INPUTEVENT_FLOURISH;
                            break;
                        case PADBUTTON_RIGHT_STICK:
                            aKeys[2] = INPUTEVENT_PAD_FREELOOK;
                            aKeys[8] = INPUTEVENT_PAD_FREELOOK;
                            break;
                        case PADBUTTON_DPAD_UP:
                            aKeys[0] = INPUTEVENT_GUI_DPAD_UP;
                            aKeys[2] = INPUTEVENT_GUI_DPAD_UP;
                            break;
                        case PADBUTTON_DPAD_RIGHT:
                            aKeys[0] = INPUTEVENT_GUI_DPAD_RIGHT;
                            aKeys[2] = INPUTEVENT_GUI_DPAD_RIGHT;
                            break;
                        case PADBUTTON_DPAD_DOWN:
                            aKeys[0] = INPUTEVENT_GUI_DPAD_DOWN;
                            aKeys[2] = INPUTEVENT_GUI_DPAD_DOWN;
                            break;
                        case PADBUTTON_DPAD_LEFT:
                            aKeys[0] = INPUTEVENT_GUI_DPAD_LEFT;
                            aKeys[2] = INPUTEVENT_GUI_DPAD_LEFT;
                            break;
                        }
                    }
                }
                else {
                    switch (nKey) {
                    case DIJOFS_Z:
                        aKeys[2] = INPUTEVENT_FLOURISH;
                        break;
                    case DIJOFS_RZ:
                        aKeys[2] = INPUTEVENT_PAD_FREELOOK;
                        aKeys[8] = INPUTEVENT_PAD_FREELOOK;
                        break;
                    case DIJOFS_POV0:
                        switch (nValue) {
                        case POVDEGREES_UP:
                            aKeys[0] = INPUTEVENT_GUI_DPAD_UP;
                            aKeys[2] = INPUTEVENT_GUI_DPAD_UP;
                            break;
                        case POVDEGREES_RIGHT:
                            aKeys[0] = INPUTEVENT_GUI_DPAD_RIGHT;
                            aKeys[2] = INPUTEVENT_GUI_DPAD_RIGHT;
                            break;
                        case POVDEGREES_DOWN:
                            aKeys[0] = INPUTEVENT_GUI_DPAD_DOWN;
                            aKeys[2] = INPUTEVENT_GUI_DPAD_DOWN;
                            break;
                        case POVDEGREES_LEFT:
                            aKeys[0] = INPUTEVENT_GUI_DPAD_LEFT;
                            aKeys[2] = INPUTEVENT_GUI_DPAD_LEFT;
                            break;
                        }
                        break;
                    }
                }
            }
        }
        if (static_cast<unsigned int>(nInputClass) < INPUTCLASS_COUNT &&
            ((kPadInputClasses >> nInputClass) & 1) != 0) {
            static const int64_t cClassPairBase[6] = { 1, 2, 0, 0, 0, 4 };
            int* paPair = &aKeys[cClassPairBase[nInputClass] * 2];
            for (int i = 1; i >= 0; i--) {
                int nKey = paPair[i];
                if (nKey >= 0) {
                    // The event id belongs in m_nId, because input dispatch reads it from there.
                    // Writing the key code field instead leaves every pad, back and pause event
                    // arriving as id 0.
                    static CExoInputEventDesc sPairDescs[2];
                    sPairDescs[i].m_nId = nKey;
                    CExoInputEvent sEvent;
                    sEvent.m_nValue = 1;
                    sEvent.m_nTimeStamp = 0;
                    sEvent.m_nSequence = 0;
                    sEvent.m_pEvent = &sPairDescs[i];
                    m_eventQueue.Add(sEvent);
                    ++m_nEventQueueCount;
                }
            }
        }
#endif
#if KOTOR1  // K1 injects only the back button (no pad-removed pause)
        if (g_BackButtonEvent != 0) {
            {
#else
        if (g_BackButtonEvent != 0 || g_GamePauseNeeded != 0) {
            if (g_BackButtonEvent != 0) {
#endif
                static CExoInputEventDesc sBackButtonDesc;
                sBackButtonDesc.m_nId = INPUTEVENT_GUI_BACK;
                CExoInputEvent sEvent;
                sEvent.m_nValue = 1;
                sEvent.m_nTimeStamp = 0;
                sEvent.m_nSequence = 0;
                sEvent.m_pEvent = &sBackButtonDesc;
                m_eventQueue.Add(sEvent);
                ++m_nEventQueueCount;
                g_BackButtonEvent = 0;
            }
#if !KOTOR1
            if (nInputClass == 0) {
                static CExoInputEventDesc sPauseDesc;
                sPauseDesc.m_nId = INPUTEVENT_PAUSE;
                CExoInputEvent sEvent;
                sEvent.m_nValue = 1;
                sEvent.m_nTimeStamp = 0;
                sEvent.m_nSequence = 0;
                sEvent.m_pEvent = &sPauseDesc;
                m_eventQueue.Add(sEvent);
                ++m_nEventQueueCount;
            }
            g_GamePauseNeeded = 0;
#endif
        }
    }

    // Copy the collected queue out; the copy constructor builds the result in place.
    nCount = m_nEventQueueCount;
    return m_eventQueue;
}
