#ifndef CEXOINPUT_H
#define CEXOINPUT_H
// idiom-exempt: canonical

#include "exostring.h"
#include "CExoArrayListDtors.h"

class CExoInputInternal;
class CExoFile;
class CExoInputEventDesc;

// Input event ids. An id names an action, not a control: SetEventDescriptions binds each one to a
// key, button or axis in one or more input classes, and GetEvents reports the ids that fired. The
// client's HandleInputEvent, the GUI manager and the panels switch on them. Ids 200-268 are
// keymap.2da's `Action<id>` rows (the ini's [Keymapping] section saves a binding under the same
// name), 280-286 its `Action<id>A`/`Action<id>B` key pairs bound as one axis.
enum ExoInputEventId {
    // Gamepad events of the gameplay class, the console layout's ids.
    INPUTEVENT_PAD_FREELOOK = 1,          // free-look camera toggle; KOTOR 2 also binds the RZ axis
    INPUTEVENT_PAD_PAUSE = 2,
    INPUTEVENT_FREELOOK_STICK_X = 3,      // the stick as a button in free look, which leaves it
    INPUTEVENT_FREELOOK_STICK_Y = 4,
    INPUTEVENT_PAD_SELECT_NEXT = 5,
    INPUTEVENT_PAD_SELECT_PREV = 6,
    INPUTEVENT_PAD_MOVE_Y = 7,            // stick axes: walk forward and back, strafe
    INPUTEVENT_PAD_MOVE_X = 8,
    INPUTEVENT_PAD_CHANGE_CHARACTER = 9,
    INPUTEVENT_PAD_PARTY_ACTIVE = 10,     // the solo-mode query
    INPUTEVENT_PAD_MENU = 11,             // the in-game menu
    INPUTEVENT_PAD_CAMERA_TILT = 12,
    INPUTEVENT_PAD_CAMERA_TURN = 13,
    INPUTEVENT_FREELOOK_EXIT = 15,
    // Movie class: any of the four face buttons skips the movie.
    INPUTEVENT_MOVIE_SKIP_A = 16,
    INPUTEVENT_MOVIE_SKIP_B = 17,
    // Mini-game class.
    INPUTEVENT_MINIGAME_ACCELERATE = 22,  // or fire, in a turret game
    INPUTEVENT_MINIGAME_MOUSE_X = 23,
    INPUTEVENT_MINIGAME_MOUSE_Y = 24,
    INPUTEVENT_MINIGAME_MOUSE_LEFT = 25,
    INPUTEVENT_MINIGAME_MOUSE_RIGHT = 26,
    // GUI and dialog classes: the pad's buttons and sticks.
    INPUTEVENT_GUI_A = 39,                // accept
    INPUTEVENT_GUI_B = 40,                // cancel
    INPUTEVENT_GUI_X = 41,
    INPUTEVENT_GUI_Y = 42,
    INPUTEVENT_GUI_BLACK = 43,            // the Xbox pad's Black button (JOYSTICK_BUTTON4)
    INPUTEVENT_GUI_WHITE = 44,            // the Xbox pad's White button (JOYSTICK_BUTTON5)
    INPUTEVENT_GUI_START = 45,
    INPUTEVENT_GUI_BACK = 46,
    INPUTEVENT_GUI_DPAD_LEFT = 47,
    INPUTEVENT_GUI_DPAD_RIGHT = 48,
    INPUTEVENT_GUI_DPAD_UP = 49,
    INPUTEVENT_GUI_DPAD_DOWN = 50,
    INPUTEVENT_GUI_STICK_Y = 51,
    INPUTEVENT_GUI_STICK_X = 52,
    INPUTEVENT_GUI_LEFT_SHOULDER = 53,
    INPUTEVENT_GUI_RIGHT_SHOULDER = 54,
    INPUTEVENT_GUI_RIGHT_STICK_Y = 55,
    INPUTEVENT_GUI_RIGHT_STICK_X = 56,
    INPUTEVENT_GUI_SCROLL_UP = 57,
    INPUTEVENT_GUI_SCROLL_DOWN = 58,
    // The right stick's horizontal pair the GUI manager makes of INPUTEVENT_GUI_RIGHT_STICK_X.
    INPUTEVENT_GUI_SCROLL_LEFT = 59,
    INPUTEVENT_GUI_SCROLL_RIGHT = 60,
    // The GUI manager's translation of the keyboard arrow keys.
    INPUTEVENT_GUI_KEY_UP = 61,
    INPUTEVENT_GUI_KEY_DOWN = 62,
    INPUTEVENT_GUI_KEY_LEFT = 63,
    INPUTEVENT_GUI_KEY_RIGHT = 64,
    // The mouse, in every class but the mini-game's.
    INPUTEVENT_MOUSE_X = 65,
    INPUTEVENT_MOUSE_Y = 66,
    INPUTEVENT_MOUSE_LEFT = 67,
    INPUTEVENT_MOUSE_RIGHT = 68,
    INPUTEVENT_MOUSE_MIDDLE = 69,
    INPUTEVENT_MOUSE_WHEEL = 70,
    // More ids whose only handler leaves free look.
    INPUTEVENT_FREELOOK_EXIT_ALT = 72,
    INPUTEVENT_FREELOOK_EXIT_KEY1 = 73,
    INPUTEVENT_FREELOOK_EXIT_KEY2 = 74,
    INPUTEVENT_FREELOOK_EXIT_KEY3 = 75,
    INPUTEVENT_FREELOOK_EXIT_KEY4 = 76,
    INPUTEVENT_MOVIE_SKIP_X = 77,
    INPUTEVENT_MOVIE_SKIP_Y = 78,
    INPUTEVENT_SCREENSHOT = 80,
    // Fixed keyboard keys of the GUI classes.
    INPUTEVENT_KEY_ESCAPE = 180,
    INPUTEVENT_KEY_RETURN = 181,
    INPUTEVENT_KEY_UP = 182,
    INPUTEVENT_KEY_DOWN = 183,
    INPUTEVENT_KEY_LEFT = 184,
    INPUTEVENT_KEY_RIGHT = 185,
    INPUTEVENT_KEY_SPACE = 186,
    INPUTEVENT_KEY_NUMPAD_ENTER = 187,
    INPUTEVENT_KEY_CAPSLOCK = 188,        // KOTOR 1's free-look key
    // keymap.2da's rows, by their `name` column.
    INPUTEVENT_MOVE_FORWARD = 200,
    INPUTEVENT_MOVE_BACK = 201,
    INPUTEVENT_STRAFE_LEFT = 202,
    INPUTEVENT_STRAFE_RIGHT = 203,
    INPUTEVENT_SELECT_PREV = 204,
    INPUTEVENT_SELECT_NEXT = 205,
    INPUTEVENT_CHANGE_CHARACTER = 206,
    INPUTEVENT_PARTY_ACTIVE = 207,
    INPUTEVENT_FREELOOK = 208,
    INPUTEVENT_PANEL_EQUIP = 209,         // 209-216: the in-game panels, in panel order
    INPUTEVENT_PANEL_INVENTORY = 210,
    INPUTEVENT_PANEL_CHARACTER = 211,
    INPUTEVENT_PANEL_ABILITIES = 212,
    INPUTEVENT_PANEL_MESSAGES = 213,
    INPUTEVENT_PANEL_JOURNAL = 214,
    INPUTEVENT_PANEL_MAP = 215,
    INPUTEVENT_PANEL_OPTIONS = 216,
    INPUTEVENT_MG_SHOOT = 217,
    INPUTEVENT_QUICKSAVE = 218,
    INPUTEVENT_LEFT_LOOKABOUT = 219,
    INPUTEVENT_RIGHT_LOOKABOUT = 220,
    INPUTEVENT_ALTERNATE_ACTIONS = 221,
    INPUTEVENT_ALTERNATE_ACTIONS2 = 222,
    INPUTEVENT_GUI = 223,                 // Escape: the in-game menu
    INPUTEVENT_PAUSE = 224,
    INPUTEVENT_TOOLTIPS = 225,
    INPUTEVENT_TARGET_LEFT_ACTION = 226,
    INPUTEVENT_DEFAULT_CHANGE = 227,
    INPUTEVENT_TARGET_MIDDLE_ACTION = 228,
    INPUTEVENT_FEATS_SKILL_CHANGE = 229,
    INPUTEVENT_TARGET_RIGHT_ACTION = 230,
    INPUTEVENT_HOSTILE_POWER_CHANGE = 231,
    INPUTEVENT_PERSONAL_POWER_ACTION = 232,
    INPUTEVENT_HOSTILE_ITEM_CHANGE = 233,
    INPUTEVENT_PERSONAL_MEDICAL_ACTION = 234,
    INPUTEVENT_FRIENDLY_POWER_CHANGE = 235,
    INPUTEVENT_PERSONAL_MINES_ACTION = 236,
    INPUTEVENT_MINES_MENU_CHANGE = 237,
    INPUTEVENT_PERSONAL_OTHER_ACTION = 238,
    INPUTEVENT_DEFAULT_ACTION = 239,
    INPUTEVENT_CANCEL_COMBAT = 240,
    INPUTEVENT_PAUSE_ALT = 241,
    INPUTEVENT_FLOURISH = 242,
    INPUTEVENT_PREV_MENU = 243,
    INPUTEVENT_NEXT_MENU = 244,
    INPUTEVENT_CLEAR_ONE_ACTION = 245,
    INPUTEVENT_ACTION_MENU_LEFT = 246,
    INPUTEVENT_ACTION_MENU_RIGHT = 247,
    INPUTEVENT_ACTION_MENU_UP = 248,
    INPUTEVENT_ACTION_MENU_DOWN = 249,
    INPUTEVENT_ACTION_MENU_QUEUE = 250,
    INPUTEVENT_ACTION_MENU_REMOVE_QUEUED = 251,
    INPUTEVENT_ACTION_MENU_SHOW_MENU = 252,
    INPUTEVENT_PAUSE_MINIGAME = 253,
    INPUTEVENT_DIALOG_1 = 254,            // 254-262: the dialog reply keys 1-9
    INPUTEVENT_DIALOG_9 = 262,
    INPUTEVENT_QUICKLOAD = 263,
    INPUTEVENT_STEALTH = 264,
    INPUTEVENT_SWITCH_WEAPONS = 265,
#if KOTOR1
    INPUTEVENT_WALK_MODIFY = 265,         // K1 mobile patch.erf keymap.2da row action265 "WALKMODIFY"
#else
    INPUTEVENT_WALK_MODIFY = 268,
#endif
    // keymap.2da's key-pair axes.
    INPUTEVENT_AXIS_ACTION_UP_DOWN = 280,
    INPUTEVENT_AXIS_ACTION_LEFT_RIGHT = 281,
    INPUTEVENT_AXIS_MG_UP_DOWN = 282,
    INPUTEVENT_AXIS_MG_LEFT_RIGHT = 283,
    INPUTEVENT_AXIS_CAMERA_ROTATE = 284,
    INPUTEVENT_AXIS_ARROW_UP_DOWN = 285,
    INPUTEVENT_AXIS_ARROW_LEFT_RIGHT = 286,

    // The id ranges SetEventDescriptions walks: [FIRST, END).
    INPUTEVENT_KEYMAP_FIRST = INPUTEVENT_MOVE_FORWARD,
#if KOTOR1
    INPUTEVENT_KEYMAP_END = 266,
#else
    INPUTEVENT_KEYMAP_END = 270,
#endif
    INPUTEVENT_KEYMAP_AXIS_FIRST = INPUTEVENT_AXIS_ACTION_UP_DOWN,
    INPUTEVENT_KEYMAP_AXIS_END = INPUTEVENT_AXIS_ARROW_LEFT_RIGHT + 1,
};

// The six input classes. Each has its own event bindings; GetEvents and PollInput read one class.
enum ExoInputClass {
    INPUTCLASS_GAME = 0,
    INPUTCLASS_MINIGAME = 1,
    INPUTCLASS_GUI = 2,
    INPUTCLASS_DIALOG = 3,
    INPUTCLASS_FREELOOK = 4,
    INPUTCLASS_MOVIE = 5,
    INPUTCLASS_COUNT = 6,
};

// CreateNewEvent's event kinds. A state event only latches its control's value for PollInput; a
// button event also queues an event on each change; an axis accumulates relative motion; a button
// axis is a pair of controls read as the two directions of one axis.
enum ExoInputEventKind {
    INPUTKIND_STATE = 0,
    INPUTKIND_BUTTON = 1,
    INPUTKIND_STATE_B = 2,
    INPUTKIND_AXIS = 3,
    INPUTKIND_BUTTON_AXIS = 4,
    INPUTKIND_STATE_C = 5,
};

// Control codes, the index space of CExoInput's KEYBOARD_*, MOUSE_* and JOYSTICK_* constants: the
// last one, and the code one past it that a binding uses for "no control".
enum ExoInputControlCode {
#if KOTOR1
    INPUTCONTROL_LAST = 131,
    INPUTCONTROL_NONE = 132,
#else
    INPUTCONTROL_LAST = 133,
    INPUTCONTROL_NONE = 134,
#endif
};

// Mouse cursor ids for SetMouseCursor. Each is the "up" image; the id after it is the same cursor
// with the button held, which SetMouseCursor picks when its flag says the button is down.
enum ExoMouseCursor {
    MOUSECURSOR_DEFAULT = 1,
    MOUSECURSOR_WALK = 3,
    MOUSECURSOR_INVALID = 5,
    MOUSECURSOR_BASH = 7,
    MOUSECURSOR_TALK = 11,
    MOUSECURSOR_DOOR = 23,
    MOUSECURSOR_USE = 25,
    MOUSECURSOR_DISABLE_MINE = 33,
    MOUSECURSOR_RECOVER_MINE = 37,
    MOUSECURSOR_SELECT = 45,
    MOUSECURSOR_ATTACK = 51,
};
// No cursor chosen yet.
enum : unsigned int {
    MOUSECURSOR_NONE = 0xffffffffu,
};

// A key's value while it is down: the high bit of a DirectInput key state byte, which is also
// what a key event reports.
enum {
    INPUTKEY_DOWN = 0x80,
};

// One entry in the array returned by GetEvents.
struct CExoInputEvent {
    int m_nValue;
    int m_nTimeStamp;
    int m_nSequence;
    CExoInputEventDesc* m_pEvent;
};

// A thin public wrapper over CExoInputInternal. Every method forwards to the pimpl, and the only
// data member is the pimpl pointer, so consumers avoid the event and device structures.
struct CExoInput {
    // <VTABLE-GEN: binary ground truth (_ZTV) — regenerated by tools/hoist_headers.py; do not edit inside markers */
    //   primary vtable, 3 slots
    virtual ~CExoInput();  // slots 0-1 (D2+D0)
    virtual void Configure();
    // </VTABLE-GEN>
    CExoInputInternal* m_pInternal;

    static int KEYBOARD_START;
    static int JOYSTICK_START;

    // Device-class tags, distinct from the key-map index boundaries above.
    static const int INPUTDEVICE_KEYBOARD;
    static const int INPUTDEVICE_MOUSE;
    static const int INPUTDEVICE_JOYSTICKSTART;
    static const int INPUTDEVICE_NONE;

    // Per-key scan codes, axis ids and button ids for the default binding table, declared without
    // initializers and defined out of line so each use stays a load.
    static const unsigned int KEYBOARD_RETURN;
    static const unsigned int KEYBOARD_UP_ARROW;
    static const unsigned int KEYBOARD_DOWN_ARROW;
    static const unsigned int KEYBOARD_LEFT_ARROW;
    static const unsigned int KEYBOARD_RIGHT_ARROW;
    static const unsigned int KEYBOARD_SPACE;
    static const unsigned int KEYBOARD_NUMPADENTER;
    static const unsigned int KEYBOARD_PRINTSCREEN;
    static const unsigned int JOYSTICK_YAXIS;
    static const unsigned int JOYSTICK_XAXIS;
    static const unsigned int JOYSTICK_SLIDER0;
    static const unsigned int JOYSTICK_SLIDER1;
    static const unsigned int JOYSTICK_SLIDER2;
    static const unsigned int JOYSTICK_BUTTON6;
    static const unsigned int JOYSTICK_BUTTON9;
    static const unsigned int JOYSTICK_RYAXIS;
    static const unsigned int JOYSTICK_RXAXIS;
    static const unsigned int MOUSE_XAXIS;
    static const unsigned int MOUSE_YAXIS;
    static const unsigned int MOUSE_BUTTON0;
    static const unsigned int MOUSE_BUTTON1;
    static const unsigned int MOUSE_BUTTON2;
    static const unsigned int MOUSE_ZAXIS;

    static const unsigned int KEYBOARD_NUMPAD1;
    static const unsigned int KEYBOARD_NUMPAD2;
    static const unsigned int KEYBOARD_NUMPAD3;
    static const unsigned int KEYBOARD_NUMPAD4;
    static const unsigned int KEYBOARD_NUMPAD5;
    static const unsigned int KEYBOARD_NUMPAD6;
    static const unsigned int KEYBOARD_NUMPAD7;
    static const unsigned int KEYBOARD_NUMPAD8;
    static const unsigned int KEYBOARD_NUMPAD9;
    static const unsigned int KEYBOARD_NUMPAD0;
    static const unsigned int KEYBOARD_NUMPADDECIMAL;
    static const unsigned int KEYBOARD_NUMPADMINUS;
    static const unsigned int KEYBOARD_NUMPADPLUS;
    static const unsigned int KEYBOARD_LEFTSHIFT;
    static const unsigned int KEYBOARD_RIGHTSHIFT;
    static const unsigned int KEYBOARD_LEFTALT;
    static const unsigned int KEYBOARD_RIGHTALT;
    static const unsigned int KEYBOARD_LEFTCTRL;
    static const unsigned int KEYBOARD_RIGHTCTRL;
    static const unsigned int KEYBOARD_TAB;
    static const unsigned int KEYBOARD_ESC;
    static const unsigned int KEYBOARD_HOME;
    static const unsigned int KEYBOARD_END;
    static const unsigned int KEYBOARD_PAGEUP;
    static const unsigned int KEYBOARD_PAGEDOWN;
    static const unsigned int KEYBOARD_INSERT;
    static const unsigned int KEYBOARD_DELETE;
    static const unsigned int KEYBOARD_F1;
    static const unsigned int KEYBOARD_F2;
    static const unsigned int KEYBOARD_F3;
    static const unsigned int KEYBOARD_F4;
    static const unsigned int KEYBOARD_F5;
    static const unsigned int KEYBOARD_F6;
    static const unsigned int KEYBOARD_F7;
    static const unsigned int KEYBOARD_F8;
    static const unsigned int KEYBOARD_F9;
    static const unsigned int KEYBOARD_F10;
    static const unsigned int KEYBOARD_F11;
    static const unsigned int KEYBOARD_F12;
    static const unsigned int KEYBOARD_A;
    static const unsigned int KEYBOARD_B;
    static const unsigned int KEYBOARD_C;
    static const unsigned int KEYBOARD_D;
    static const unsigned int KEYBOARD_E;
    static const unsigned int KEYBOARD_F;
    static const unsigned int KEYBOARD_G;
    static const unsigned int KEYBOARD_H;
    static const unsigned int KEYBOARD_I;
    static const unsigned int KEYBOARD_J;
    static const unsigned int KEYBOARD_K;
    static const unsigned int KEYBOARD_L;
    static const unsigned int KEYBOARD_M;
    static const unsigned int KEYBOARD_N;
    static const unsigned int KEYBOARD_O;
    static const unsigned int KEYBOARD_P;
    static const unsigned int KEYBOARD_Q;
    static const unsigned int KEYBOARD_R;
    static const unsigned int KEYBOARD_S;
    static const unsigned int KEYBOARD_T;
    static const unsigned int KEYBOARD_U;
    static const unsigned int KEYBOARD_V;
    static const unsigned int KEYBOARD_W;
    static const unsigned int KEYBOARD_X;
    static const unsigned int KEYBOARD_Y;
    static const unsigned int KEYBOARD_Z;
    static const unsigned int KEYBOARD_1;
    static const unsigned int KEYBOARD_2;
    static const unsigned int KEYBOARD_3;
    static const unsigned int KEYBOARD_4;
    static const unsigned int KEYBOARD_5;
    static const unsigned int KEYBOARD_6;
    static const unsigned int KEYBOARD_7;
    static const unsigned int KEYBOARD_8;
    static const unsigned int KEYBOARD_9;
    static const unsigned int KEYBOARD_0;
    static const unsigned int KEYBOARD_CAPSLOCK;
    static const unsigned int KEYBOARD_PAUSE;
    static const unsigned int KEYBOARD_F13;
    static const unsigned int KEYBOARD_F14;
    static const unsigned int KEYBOARD_F15;
    static const unsigned int KEYBOARD_MINUS;
    static const unsigned int KEYBOARD_EQUALS;
    static const unsigned int KEYBOARD_BACK;
    static const unsigned int KEYBOARD_LBRACKET;
    static const unsigned int KEYBOARD_RBRACKET;
    static const unsigned int KEYBOARD_SEMICOLON;
    static const unsigned int KEYBOARD_APOSTROPHE;
    static const unsigned int KEYBOARD_GRAVE;
    static const unsigned int KEYBOARD_BACKSLASH;
    static const unsigned int KEYBOARD_COMMA;
    static const unsigned int KEYBOARD_PERIOD;
    static const unsigned int KEYBOARD_SLASH;
    static const unsigned int KEYBOARD_MULTIPLY;
    static const unsigned int KEYBOARD_NUMPADCOMMA;
    static const unsigned int KEYBOARD_DIVIDE;
    static const unsigned int KEYBOARD_OEM_102;
    static const unsigned int JOYSTICK_HAT;
    static const unsigned int JOYSTICK_RZAXIS;
    static const unsigned int JOYSTICK_BUTTON0;
    static const unsigned int JOYSTICK_BUTTON1;
    static const unsigned int JOYSTICK_BUTTON2;
    static const unsigned int JOYSTICK_BUTTON3;
    static const unsigned int JOYSTICK_BUTTON4;
    static const unsigned int JOYSTICK_BUTTON5;
    static const unsigned int JOYSTICK_BUTTON7;
    static const unsigned int JOYSTICK_BUTTON8;
    static const unsigned int JOYSTICK_BUTTON10;
    static const unsigned int JOYSTICK_BUTTON11;
    static const unsigned int JOYSTICK_BUTTON12;
    static const unsigned int JOYSTICK_BUTTON13;
    static const unsigned int JOYSTICK_BUTTON14;
#if KOTOR1
    static const unsigned int JOYSTICK_BUTTON15;
#endif

    CExoInput();
    int  AddEvent(unsigned int a, unsigned int b);
    int  CreateNewEvent(unsigned int a, int b, int c, unsigned int d, unsigned int e);
    int  ScaleEvent(unsigned int a, float b, float c);
    int  SetEventRepeatable(unsigned int a, unsigned int b, unsigned int c);
    void SetEventNonRepeatable(unsigned int a);
    void ClearEventRepeat(unsigned int a);
    void CoolDownEvent(unsigned int a, unsigned int b);
    void* GetEvent(unsigned int a, int b);
    // Returns a float; callers compare the result directly without converting.
    float PollInput(unsigned int a, int b);
    void ClearEvents();
    void ClearKeyboardEvents();
    void ClearMouseEvents();
    // The string overload opens the file in text mode and hands it to the other one.
    void DumpEventsToFile(const CExoString& sName);
    void DumpEventsToFile(CExoFile* a);
    void WriteEventToFile(unsigned int a, int b, CExoFile* c);
    int GetActive();
    CExoArrayList<CExoInputEvent> GetEvents(int& a, int b);
    void KeyboardModeOn();
    void KeyboardModeOff();
    void SetActive(int a);
    void SetRumble(int a, float b, float c, unsigned int d);
    int GetAutoCalInfo();
    int GetPortBinding();
    void SetAutoCalInfo(unsigned int a);
    void SetPortBinding(unsigned int a);
    void EndAttemptedBind();
    int GetMainDeviceLost();
    void UpdateJoysticks();
    void SetRumbleEnabled(int a);
    int GetRumbleEnabled();
    void PauseRumble();
    void UnpauseRumble();
    int GetRumblePaused();
    int  BufferEvent(void* a);
    void SetMouseMode(unsigned char a);
    void RefreshMouseCursor(int a);
    int SetMouseCursor(int a, int b, int c);
    void HideMouse();
    void ShowMouse();
    void ClearAllInputEventDescriptions();
    void GetMouseDelta(float* a, float* b);
    void UpdateMouseDelta(float a);
    int IsModalKeyDown_Shift();
    int IsModalKeyDown_Ctrl();
    int IsModalKeyDown_Alt();
    void GetJoystickNStick(int a, float* b, float* c);
    int GetLastCapturedKeyboardKey();
    CExoString GetDisplayCharacterForKey(int a);
    int GetKeyboardMode();
    unsigned char GetMouseMode();
    void SetCaptureKeyboardKeys(int a);
    int GetLastSetMouseDownState();
#if KOTOR1
    void SetMousePos(int a, int b);
    void SetMousePos(int a, int b, bool) { SetMousePos(a, b); }
#else
    void SetMousePos(int a, int b, bool c);
#endif
    char GetNumberJoysticks();
    int IsJoystickActive();
    void GetJoystickRStick(float* a, float* b);
    int EventExists(unsigned int n);
    int CreateNewEventInClass(unsigned int a, int b, unsigned int c, int d, unsigned int e, unsigned int f);
#if !KOTOR1
    int IsPsShowButtonsController() const;
#endif
};

// The single global input object.
extern CExoInput* g_pExoInput;

#endif
