#include "exoinputinternal.h"
#include "exofile.h"
#include "restype.h"

int CExoInput::KEYBOARD_START = 6;
int CExoInput::JOYSTICK_START = 110;

const int CExoInput::INPUTDEVICE_NONE          = -1;
const int CExoInput::INPUTDEVICE_KEYBOARD      = 0;
const int CExoInput::INPUTDEVICE_MOUSE         = 1;
const int CExoInput::INPUTDEVICE_JOYSTICKSTART = 2;
const unsigned int CExoInput::KEYBOARD_RETURN       = 6;
const unsigned int CExoInput::KEYBOARD_LEFT_ARROW   = 7;
const unsigned int CExoInput::KEYBOARD_RIGHT_ARROW  = 8;
const unsigned int CExoInput::KEYBOARD_UP_ARROW     = 9;
const unsigned int CExoInput::KEYBOARD_DOWN_ARROW   = 10;
const unsigned int CExoInput::KEYBOARD_PRINTSCREEN  = 38;
const unsigned int CExoInput::KEYBOARD_SPACE        = 87;
const unsigned int CExoInput::KEYBOARD_NUMPADENTER  = 88;
const unsigned int CExoInput::MOUSE_BUTTON0 = 0;
const unsigned int CExoInput::MOUSE_BUTTON1 = 1;
const unsigned int CExoInput::MOUSE_BUTTON2 = 2;
const unsigned int CExoInput::MOUSE_XAXIS   = 3;
const unsigned int CExoInput::MOUSE_YAXIS   = 4;
const unsigned int CExoInput::MOUSE_ZAXIS   = 5;

const unsigned int CExoInput::KEYBOARD_NUMPAD1       = 11;
const unsigned int CExoInput::KEYBOARD_NUMPAD2       = 12;
const unsigned int CExoInput::KEYBOARD_NUMPAD3       = 13;
const unsigned int CExoInput::KEYBOARD_NUMPAD4       = 14;
const unsigned int CExoInput::KEYBOARD_NUMPAD5       = 15;
const unsigned int CExoInput::KEYBOARD_NUMPAD6       = 16;
const unsigned int CExoInput::KEYBOARD_NUMPAD7       = 17;
const unsigned int CExoInput::KEYBOARD_NUMPAD8       = 18;
const unsigned int CExoInput::KEYBOARD_NUMPAD9       = 19;
const unsigned int CExoInput::KEYBOARD_NUMPAD0       = 20;
const unsigned int CExoInput::KEYBOARD_NUMPADDECIMAL = 21;
const unsigned int CExoInput::KEYBOARD_NUMPADMINUS   = 22;
const unsigned int CExoInput::KEYBOARD_NUMPADPLUS    = 23;
const unsigned int CExoInput::KEYBOARD_LEFTSHIFT     = 24;
const unsigned int CExoInput::KEYBOARD_RIGHTSHIFT    = 25;
const unsigned int CExoInput::KEYBOARD_LEFTALT       = 26;
const unsigned int CExoInput::KEYBOARD_RIGHTALT      = 27;
const unsigned int CExoInput::KEYBOARD_LEFTCTRL      = 28;
const unsigned int CExoInput::KEYBOARD_RIGHTCTRL     = 29;
const unsigned int CExoInput::KEYBOARD_TAB           = 30;
const unsigned int CExoInput::KEYBOARD_ESC           = 31;
const unsigned int CExoInput::KEYBOARD_HOME          = 32;
const unsigned int CExoInput::KEYBOARD_END           = 33;
const unsigned int CExoInput::KEYBOARD_PAGEUP        = 34;
const unsigned int CExoInput::KEYBOARD_PAGEDOWN      = 35;
const unsigned int CExoInput::KEYBOARD_INSERT        = 36;
const unsigned int CExoInput::KEYBOARD_DELETE        = 37;
const unsigned int CExoInput::KEYBOARD_F1            = 39;
const unsigned int CExoInput::KEYBOARD_F2            = 40;
const unsigned int CExoInput::KEYBOARD_F3            = 41;
const unsigned int CExoInput::KEYBOARD_F4            = 42;
const unsigned int CExoInput::KEYBOARD_F5            = 43;
const unsigned int CExoInput::KEYBOARD_F6            = 44;
const unsigned int CExoInput::KEYBOARD_F7            = 45;
const unsigned int CExoInput::KEYBOARD_F8            = 46;
const unsigned int CExoInput::KEYBOARD_F9            = 47;
const unsigned int CExoInput::KEYBOARD_F10           = 48;
const unsigned int CExoInput::KEYBOARD_F11           = 49;
const unsigned int CExoInput::KEYBOARD_F12           = 50;
const unsigned int CExoInput::KEYBOARD_A             = 51;
const unsigned int CExoInput::KEYBOARD_B             = 52;
const unsigned int CExoInput::KEYBOARD_C             = 53;
const unsigned int CExoInput::KEYBOARD_D             = 54;
const unsigned int CExoInput::KEYBOARD_E             = 55;
const unsigned int CExoInput::KEYBOARD_F             = 56;
const unsigned int CExoInput::KEYBOARD_G             = 57;
const unsigned int CExoInput::KEYBOARD_H             = 58;
const unsigned int CExoInput::KEYBOARD_I             = 59;
const unsigned int CExoInput::KEYBOARD_J             = 60;
const unsigned int CExoInput::KEYBOARD_K             = 61;
const unsigned int CExoInput::KEYBOARD_L             = 62;
const unsigned int CExoInput::KEYBOARD_M             = 63;
const unsigned int CExoInput::KEYBOARD_N             = 64;
const unsigned int CExoInput::KEYBOARD_O             = 65;
const unsigned int CExoInput::KEYBOARD_P             = 66;
const unsigned int CExoInput::KEYBOARD_Q             = 67;
const unsigned int CExoInput::KEYBOARD_R             = 68;
const unsigned int CExoInput::KEYBOARD_S             = 69;
const unsigned int CExoInput::KEYBOARD_T             = 70;
const unsigned int CExoInput::KEYBOARD_U             = 71;
const unsigned int CExoInput::KEYBOARD_V             = 72;
const unsigned int CExoInput::KEYBOARD_W             = 73;
const unsigned int CExoInput::KEYBOARD_X             = 74;
const unsigned int CExoInput::KEYBOARD_Y             = 75;
const unsigned int CExoInput::KEYBOARD_Z             = 76;
const unsigned int CExoInput::KEYBOARD_1             = 77;
const unsigned int CExoInput::KEYBOARD_2             = 78;
const unsigned int CExoInput::KEYBOARD_3             = 79;
const unsigned int CExoInput::KEYBOARD_4             = 80;
const unsigned int CExoInput::KEYBOARD_5             = 81;
const unsigned int CExoInput::KEYBOARD_6             = 82;
const unsigned int CExoInput::KEYBOARD_7             = 83;
const unsigned int CExoInput::KEYBOARD_8             = 84;
const unsigned int CExoInput::KEYBOARD_9             = 85;
const unsigned int CExoInput::KEYBOARD_0             = 86;
const unsigned int CExoInput::KEYBOARD_CAPSLOCK      = 89;
const unsigned int CExoInput::KEYBOARD_PAUSE         = 90;
const unsigned int CExoInput::KEYBOARD_F13           = 91;
const unsigned int CExoInput::KEYBOARD_F14           = 92;
const unsigned int CExoInput::KEYBOARD_F15           = 93;
const unsigned int CExoInput::KEYBOARD_MINUS         = 94;
const unsigned int CExoInput::KEYBOARD_EQUALS        = 95;
const unsigned int CExoInput::KEYBOARD_BACK          = 96;
const unsigned int CExoInput::KEYBOARD_LBRACKET      = 97;
const unsigned int CExoInput::KEYBOARD_RBRACKET      = 98;
const unsigned int CExoInput::KEYBOARD_SEMICOLON     = 99;
const unsigned int CExoInput::KEYBOARD_APOSTROPHE    = 100;
const unsigned int CExoInput::KEYBOARD_GRAVE         = 101;
const unsigned int CExoInput::KEYBOARD_BACKSLASH     = 102;
const unsigned int CExoInput::KEYBOARD_COMMA         = 103;
const unsigned int CExoInput::KEYBOARD_PERIOD        = 104;
const unsigned int CExoInput::KEYBOARD_SLASH         = 105;
const unsigned int CExoInput::KEYBOARD_MULTIPLY      = 106;
const unsigned int CExoInput::KEYBOARD_NUMPADCOMMA   = 107;
const unsigned int CExoInput::KEYBOARD_DIVIDE        = 108;
const unsigned int CExoInput::KEYBOARD_OEM_102       = 109;
const unsigned int CExoInput::JOYSTICK_HAT           = 112;
const unsigned int CExoInput::JOYSTICK_RZAXIS        = 118;
#if KOTOR1  // K1 has no RX/RY/RZ axes, so the buttons start lower
const unsigned int CExoInput::JOYSTICK_BUTTON0       = 116;
const unsigned int CExoInput::JOYSTICK_BUTTON1       = 117;
const unsigned int CExoInput::JOYSTICK_BUTTON2       = 118;
const unsigned int CExoInput::JOYSTICK_BUTTON3       = 119;
const unsigned int CExoInput::JOYSTICK_BUTTON4       = 120;
const unsigned int CExoInput::JOYSTICK_BUTTON5       = 121;
const unsigned int CExoInput::JOYSTICK_BUTTON6       = 122;
const unsigned int CExoInput::JOYSTICK_BUTTON7       = 123;
const unsigned int CExoInput::JOYSTICK_BUTTON8       = 124;
const unsigned int CExoInput::JOYSTICK_BUTTON9       = 125;
const unsigned int CExoInput::JOYSTICK_BUTTON10      = 126;
const unsigned int CExoInput::JOYSTICK_BUTTON11      = 127;
const unsigned int CExoInput::JOYSTICK_BUTTON12      = 128;
const unsigned int CExoInput::JOYSTICK_BUTTON13      = 129;
const unsigned int CExoInput::JOYSTICK_BUTTON14      = 130;
const unsigned int CExoInput::JOYSTICK_BUTTON15      = 131;
#else
const unsigned int CExoInput::JOYSTICK_BUTTON0       = 119;
const unsigned int CExoInput::JOYSTICK_BUTTON1       = 120;
const unsigned int CExoInput::JOYSTICK_BUTTON2       = 121;
const unsigned int CExoInput::JOYSTICK_BUTTON3       = 122;
const unsigned int CExoInput::JOYSTICK_BUTTON4       = 123;
const unsigned int CExoInput::JOYSTICK_BUTTON5       = 124;
const unsigned int CExoInput::JOYSTICK_BUTTON7       = 126;
const unsigned int CExoInput::JOYSTICK_BUTTON8       = 127;
const unsigned int CExoInput::JOYSTICK_BUTTON10      = 129;
const unsigned int CExoInput::JOYSTICK_BUTTON11      = 130;
const unsigned int CExoInput::JOYSTICK_BUTTON12      = 131;
const unsigned int CExoInput::JOYSTICK_BUTTON13      = 132;
const unsigned int CExoInput::JOYSTICK_BUTTON14      = 133;
#endif
const unsigned int CExoInput::JOYSTICK_XAXIS    = 110;
const unsigned int CExoInput::JOYSTICK_YAXIS    = 111;
const unsigned int CExoInput::JOYSTICK_SLIDER0  = 113;
const unsigned int CExoInput::JOYSTICK_SLIDER1  = 114;
const unsigned int CExoInput::JOYSTICK_SLIDER2  = 115;
const unsigned int CExoInput::JOYSTICK_RXAXIS   = 116;
const unsigned int CExoInput::JOYSTICK_RYAXIS   = 117;
#if !KOTOR1
const unsigned int CExoInput::JOYSTICK_BUTTON6  = 125;
#endif
#if !KOTOR1
const unsigned int CExoInput::JOYSTICK_BUTTON9  = 128;
#endif

int CExoInput::AddEvent(unsigned int a, unsigned int b)
{
    return m_pInternal->AddEvent(a, b);
}

int CExoInput::CreateNewEvent(unsigned int a, int b, int c, unsigned int d, unsigned int e)
{
    return m_pInternal->CreateNewEvent(a, b, c, d, e);
}

int CExoInput::ScaleEvent(unsigned int a, float b, float c)
{
    return m_pInternal->ScaleEvent(a, b, c);
}

int CExoInput::SetEventRepeatable(unsigned int a, unsigned int b, unsigned int c)
{
    return m_pInternal->SetEventRepeatable(a, b, c);
}

void CExoInput::SetEventNonRepeatable(unsigned int a)
{
    m_pInternal->SetEventNonRepeatable(a);
}

void CExoInput::ClearEventRepeat(unsigned int a)
{
    m_pInternal->ClearEventRepeat(a);
}

void CExoInput::CoolDownEvent(unsigned int a, unsigned int b)
{
    m_pInternal->CoolDownEvent(a, b);
}

void* CExoInput::GetEvent(unsigned int a, int b)
{
    return m_pInternal->GetEvent(a, b);
}

float CExoInput::PollInput(unsigned int a, int b)
{
    return m_pInternal->PollInput(a, b);
}

void CExoInput::ClearEvents()
{
    m_pInternal->ClearEvents();
}

void CExoInput::ClearKeyboardEvents()
{
    m_pInternal->ClearKeyboardEvents();
}

void CExoInput::ClearMouseEvents()
{
    m_pInternal->ClearMouseEvents();
}


void CExoInput::DumpEventsToFile(const CExoString& sName)
{
    CExoFile* pFile = new CExoFile(sName, RESTYPE_TXT, CExoString("w+"));
    if (pFile->FileOpened())
        m_pInternal->DumpEventsToFile(pFile);
    delete pFile;
}

void CExoInput::DumpEventsToFile(CExoFile* a)
{
    m_pInternal->DumpEventsToFile(a);
}

void CExoInput::WriteEventToFile(unsigned int a, int b, CExoFile* c)
{
    m_pInternal->WriteEventToFile(a, b, c);
}

int CExoInput::GetActive()
{
    return m_pInternal->GetActive();
}

CExoArrayList<CExoInputEvent> CExoInput::GetEvents(int& a, int b)
{
    return m_pInternal->GetEvents(a, b);
}

void CExoInput::KeyboardModeOn()
{
    m_pInternal->KeyboardModeOn();
}

void CExoInput::KeyboardModeOff()
{
    m_pInternal->KeyboardModeOff();
}

void CExoInput::SetActive(int a)
{
    m_pInternal->SetActive(a);
}

void CExoInput::SetRumble(int a, float b, float c, unsigned int d)
{
    m_pInternal->SetRumble(a, b, c, d);
}

int CExoInput::GetAutoCalInfo()
{
    return m_pInternal->GetAutoCalInfo();
}

int CExoInput::GetPortBinding()
{
    return m_pInternal->GetPortBinding();
}

void CExoInput::SetAutoCalInfo(unsigned int a)
{
    m_pInternal->SetAutoCalInfo(a);
}

void CExoInput::SetPortBinding(unsigned int a)
{
    m_pInternal->SetPortBinding(a);
}

void CExoInput::EndAttemptedBind()
{
    m_pInternal->EndAttemptedBind();
}

int CExoInput::GetMainDeviceLost()
{
    return m_pInternal->GetMainDeviceLost();
}

void CExoInput::UpdateJoysticks()
{
    m_pInternal->UpdateJoysticks();
}

void CExoInput::SetRumbleEnabled(int a)
{
    m_pInternal->SetRumbleEnabled(a);
}

int CExoInput::GetRumbleEnabled()
{
    return m_pInternal->GetRumbleEnabled();
}

void CExoInput::PauseRumble()
{
    m_pInternal->PauseRumble();
}

void CExoInput::UnpauseRumble()
{
    m_pInternal->UnpauseRumble();
}

int CExoInput::GetRumblePaused()
{
    return m_pInternal->GetRumblePaused();
}

int CExoInput::BufferEvent(void* a)
{
    return m_pInternal->BufferEvent(a);
}

void CExoInput::SetMouseMode(unsigned char a)
{
    m_pInternal->SetMouseMode(a);
}

void CExoInput::RefreshMouseCursor(int a)
{
    m_pInternal->RefreshMouseCursor(a);
}

int CExoInput::SetMouseCursor(int a, int b, int c)
{
    return m_pInternal->SetMouseCursor(a, b, c);
}

void CExoInput::HideMouse()
{
    m_pInternal->HideMouse();
}

void CExoInput::ShowMouse()
{
    m_pInternal->ShowMouse();
}

void CExoInput::ClearAllInputEventDescriptions()
{
    m_pInternal->ClearAllInputEventDescriptions();
}

void CExoInput::GetMouseDelta(float* a, float* b)
{
    m_pInternal->GetMouseDelta(a, b);
}

void CExoInput::UpdateMouseDelta(float a)
{
    m_pInternal->UpdateMouseDelta(a);
}

int CExoInput::IsModalKeyDown_Shift()
{
    return m_pInternal->IsModalKeyDown_Shift();
}

int CExoInput::IsModalKeyDown_Ctrl()
{
    return m_pInternal->IsModalKeyDown_Ctrl();
}

int CExoInput::IsModalKeyDown_Alt()
{
    return m_pInternal->IsModalKeyDown_Alt();
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CExoInput::GetJoystickNStick(int a, float* b, float* c)
{
    m_pInternal->GetJoystickNStick(a, b, c);
}
#endif // !KOTOR1

int CExoInput::GetLastCapturedKeyboardKey()
{
    return m_pInternal->GetLastCapturedKeyboardKey();
}

CExoString CExoInput::GetDisplayCharacterForKey(int a)
{
    return m_pInternal->GetDisplayCharacterForKey(a);
}

int CExoInput::GetKeyboardMode()
{
    return m_pInternal->m_nKeyboardMode;
}

unsigned char CExoInput::GetMouseMode()
{
    return m_pInternal->m_nMouseMode;
}

#if !KOTOR1
int CExoInput::IsPsShowButtonsController() const
{
    return m_pInternal->IsPsShowButtonsController();
}
#endif

void CExoInput::SetCaptureKeyboardKeys(int a)
{
    m_pInternal->m_bCaptureKeyboardKeys = a;
}

int CExoInput::GetLastSetMouseDownState()
{
    return m_pInternal->m_nLastMouseDownState;
}

#if KOTOR1
void CExoInput::SetMousePos(int a, int b)
{
    m_pInternal->SetMousePos(a, b);
}
#else
void CExoInput::SetMousePos(int a, int b, bool c)
{
    m_pInternal->SetMousePos(a, b, c);
}
#endif

char CExoInput::GetNumberJoysticks()
{
    return static_cast<char>(m_pInternal->m_pMainDevice->m_nNumJoysticks);
}

int CExoInput::IsJoystickActive()
{
    return m_pInternal->m_pMainDevice->m_nNumJoysticks > 0;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CExoInput::GetJoystickRStick(float* a, float* b)
{
    m_pInternal->GetJoystickNStick(1, a, b);
}
#endif // !KOTOR1

int CExoInput::EventExists(unsigned int n)
{
    if (m_pInternal->m_aEvents.m_count <= static_cast<int>(n))
        return 0;
    return m_pInternal->m_aEvents.m_pArray[static_cast<int>(n)] != 0;
}

void CExoInput::Configure()
{
    m_pInternal->Configure();
}

int CExoInput::CreateNewEventInClass(unsigned int a, int b, unsigned int c, int d, unsigned int e, unsigned int f)
{
    m_pInternal->CreateNewEvent(a, b, d, e, f);
    return m_pInternal->AddEvent(a, c);
}

CExoInput::CExoInput()
{
    m_pInternal = new CExoInputInternal(g_hInst, g_hRenderWnd);
}

CExoInput::~CExoInput()
{
    delete m_pInternal;
    m_pInternal = 0;
}
