// k2qolpage.cpp - the engine half of the Controller options page: the "Other Settings" page, the
// button that opens it, and the fitting of the game's text to the text-size setting. The settings
// themselves and what they do are in qol/k2qol.cpp.
//
// For the button, Main menu -> Options gets "Other Settings" one row below Sound. CSWGuiOptionsMain
// already carries a sixth wall button the layout never binds (m_aButtons[5]: constructed and wired
// to OnEnterSlot, never InitControl'd), so K2QolAddControllerButton styles that button off Sound,
// captions it, splices it into the pad's up/down chain and adds it. Not on the in-game Options
// screen: its eight-button column already reaches the frame's bottom bar, so a ninth row would
// overlap Close or squeeze every row.
//
// The page is built on the Gameplay page's own layout, `optgameplay_p`, the way
// CSWGuiInGameGameplay builds itself (src/game/clientcore/swguiingamegameplay.cpp is the template):
//   BTN_DIFFICULTY + BTN_DIFFLEFT/BTN_DIFFRIGHT  -> the button-prompts selector (Automatic, Xbox,
//                                                   PlayStation, Switch, GameCube), its arrows,
//                                                   events, hover colouring and hide-the-arrow-at-
//                                                   either-end
//   CB_LEVELUP                                   -> disable right stick pitch
//   (no layout control)                          -> text size (25..250 %, made to fit:
//                                                   K2QolFitText, below), rumble (Controller,
//                                                   Device or Off; Device on Android only), Swap
//                                                   A/B and X/Y (Standard or Swapped) and Faster
//                                                   Movement (Off or On): member buttons
//                                                   Initialize()d from the difficulty trio
// Text size is the top row, in the difficulty trio's place; the prompts, the checkbox, rumble, the
// swap and Faster Movement follow one row pitch apart.
//   LBL_TITLE, LB_DESC, BTN_BACK, BTN_DEFAULT, LBL_BAR1..4 as Gameplay binds them.
// The layout's other controls are not bound, so they are never created. Every caption is a literal
// (there are no TLK strrefs for any of this), so the description box uses SetText. The hybrid APK's
// fifth row, "Console Interface", is not here: it switched that APK's console-UI hooks at launch,
// while in this build the UI is what K2P_UI selected.
//
// On KOTOR 1 (K2_QOL_CONTROLLER without K2_QOL_IMPROVEMENTS), the button and the page are built the
// same way on K1's own layouts (`optionsmain`, `optgameplay`, scaled by ScaleExtentForResolution as
// K1's panels are), with five rows: the prompts selector on BTN_DIFFICULTY, then rumble, the swap,
// Faster Movement and Achievements (Off or On). K1's colours (yellow on focus, blue off), Y for defaults as a control event,
// and no bar labels. The text size is KOTOR II's (everything below K2QolNarrowSelectorCaption).
#ifdef K2_QOL_CONTROLLER

#include "swgui.h"
#include "swguicontrol.h"
#include "swguioptions.h"
#include "guiingame.h"
#include "swguiingame.h"
#include "swguiloadscreen.h"
#include "swguiingameareatransition.h"
#include "aurgui.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "exostring.h"
#include "exoresman.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <unordered_set>
#include "port/k2qolapi.h"

extern float GetResolutionTextScalar();
void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);

namespace {

enum {
    STYLE_AUTO = 0,
    STYLE_XBOX,
    STYLE_PS,
    STYLE_SWITCH,
    STYLE_GAMECUBE,
    NUM_STYLES
};

const char* const kStyleCaption[NUM_STYLES] = {
    "Prompts: Automatic",
    "Prompts: Xbox",
    "Prompts: PlayStation",
    "Prompts: Switch",
    "Prompts: GameCube",
};

const char* const kStyleDescription[NUM_STYLES] = {
#if KOTOR1
    "Button prompts: Automatic.\n\nShow PlayStation button prompts while a PlayStation controller "
    "is connected, and the game's own prompts otherwise.\n\nPrompts already on screen change the "
    "next time their screen is opened.",
#else
    "Button prompts: Automatic.\n\nShow PlayStation button prompts while a PlayStation controller "
    "is connected, and Xbox prompts otherwise. This is the game's own behaviour.\n\nPrompts "
    "already on screen change the next time their screen is opened.",
#endif
    "Button prompts: Xbox.\n\nAlways show Xbox button prompts: A, B, X and Y.\n\nPrompts already "
    "on screen change the next time their screen is opened.",
    "Button prompts: PlayStation.\n\nAlways show PlayStation button prompts: Cross, Circle, Square "
    "and Triangle.\n\nPrompts already on screen change the next time their screen is opened.",
    "Button prompts: Nintendo Switch.\n\nAlways show Nintendo Switch button prompts: A, B, X and Y, "
    "with + and - for Start and Back. If the buttons act the other way round to what they show, "
    "turn on Swap A/B and X/Y.\n\nPrompts already on screen change the next time their screen is "
    "opened.",
    "Button prompts: Nintendo GameCube.\n\nAlways show GameCube button prompts: A, B, X and Y, with "
    "ZL and Z for the shoulder buttons (as on the Nintendo Switch 2 GameCube controller), L and R "
    "for the triggers and Start. If the buttons act the other way round to what they show, turn on "
    "Swap A/B and X/Y.\n\nPrompts already on screen change the next time their screen is opened.",
};

#if KOTOR1
const char* const kNoArtDescription =
    "\n\nThis build has no art for these prompts (the game data it is made from was not available "
    "when it was built), so this choice shows the game's own prompts.";
#else
const char* const kNoArtDescription =
    "\n\nThis build has no art for these prompts (the tools or game data it is made from were not "
    "available when it was built), so this choice shows the Xbox prompts.";
#endif

const char* const kNoPitchCaption = "Disable Right Stick Pitch";
const char* const kNoPitchDescription =
    "Disable Right Stick Camera Pitch.\n\nPushing the right stick up or down no longer tilts the "
    "camera; the stick only turns it left and right. Touch camera controls and the free-look "
    "camera's own tilt are unaffected.";

const int kTextScaleMin = 25;
const int kTextScaleMax = 250;
const int kTextScaleStep = 25;
const char* const kTextScaleDescription =
    "Text Size.\n\nThe game's text already follows the screen's resolution; this makes it "
    "larger or smaller on top of that: conversations and computer screens, subtitles, "
    "loading-screen tips, pop-up messages and datapads, the action bar's caption and the target's "
    "name, menu captions and descriptions. Larger text is kept as large as its box can still show."
    "\n\nTakes effect when a screen is next opened (conversations: when a game is loaded).";

// The rumble selector steps through k2qol_rumble()'s modes in this display order; Device (2) only
// where there is a device vibrator to drive.
const int kRumbleOrderDevice[] = {1, 2, 0};   // Controller, Device, Off
const int kRumbleOrderPad[] = {1, 0};         // Controller, Off
const char* const kRumbleCaption[3] = {"Rumble: Off", "Rumble: Controller", "Rumble: Device"};
const char* const kRumbleDescription[3] = {
    "Rumble: Off.\n\nNo vibration.",
    "Rumble: Controller.\n\nThe connected controller's own rumble motors, when the system lets the "
    "game drive them (some Bluetooth controllers do not).",
    "Rumble: Device.\n\nThe phone's or handheld's own vibration motor, always. Use this on a "
    "handheld whose built-in controls have no motor of their own (the vibration is in the body), "
    "or to feel rumble with a controller that does not support it.",
};

const char* const kMoveCaption[2] = {"Faster Movement: Off", "Faster Movement: On"};
const char* const kMoveDescription =
    "Faster Movement.\n\nOn: the party leader moves 30% faster under direct control (the movement "
    "keys or the left stick), as KOTOR 1 does on Android. Off: the speed of the PC release.\n\n"
    "Clicking a destination is not affected.";
#if KOTOR1
const char* const kAchCaption[2] = {"Achievements: Off", "Achievements: On"};
const char* const kAchDescription =
    "Achievements.\n\nOn: an achievement the game awards is recorded and announced with a notice. "
    "Off: none is recorded or announced; the ones already unlocked stay unlocked.";
#endif
const char* const kSwapCaption[2] = {"Swap A/B, X/Y: Off", "Swap A/B, X/Y: On"};
const char* const kSwapDescription =
    "Swap A/B and X/Y.\n\nFor a controller whose buttons act the other way round to the prompts "
    "shown (common with Nintendo-layout controllers): pressing A acts as B and X as Y, and the "
    "other way round, everywhere in the game.\n\nTakes effect immediately.";

const int* RumbleOrder() { return k2qol_have_device_vibrator() ? kRumbleOrderDevice : kRumbleOrderPad; }
int NumRumble() { return k2qol_have_device_vibrator() ? 3 : 2; }

#if KOTOR1
const char* const kButtonDescription =
    "Button prompt style, rumble, the face buttons, movement speed and achievements.";
#define K2QOL_COLOR_FOCUS CGuiInGame::COLOR_YELLOW
#define K2QOL_COLOR_TEXT  CGuiInGame::COLOR_BLUE
#else
const char* const kButtonDescription =
    "Text size, button prompt style, right stick camera, rumble, the face buttons and movement speed.";
#define K2QOL_COLOR_FOCUS CGuiInGame::COLOR_NEW_HIGHLIGHT
#define K2QOL_COLOR_TEXT  CGuiInGame::COLOR_NEW_TEXT
#endif

// Puts one line of text in a description list box, the body CSWGuiInGameGameplay::SetDescription and
// CSWGuiOptionsMain::SetDescription share, with a literal in place of a strref.
void ShowDescription(CSWGuiListBox& listBox, CSWGuiLabel& label, const CExoString& sText)
{
    label.m_text.m_params.SetText(sText);
    listBox.ClearItems();
    CSWGuiExtent extent = label.m_extent;
    if (sText.m_sString && sText.m_sString[0])
        extent.h = label.m_text.GetIdealHeight();
    else
        extent.h = label.m_text.GetFontHeight();
    label.SetExtent(extent);
    CSWGuiControl* pLabel = &label;
    listBox.AddControls(&pLabel, 1, 0, 0, 0);
    listBox.SetSelectedControl(pLabel, 0);
}

}  // namespace

class K2GuiOptionsController : public CSWGuiPanel {
public:
    explicit K2GuiOptionsController(CSWGuiManager* pGuiManager);
    virtual ~K2GuiOptionsController() {}

    virtual void HandleInputEvent(int type, int val);

    void SetUpOptions();
    void SetStyleText();
    void ShowStyleDescription();
    void SetTextScaleText();
    void SetRumbleText();
    void ShowRumbleDescription();
    void SetSwapText();
    void SetMoveText();
#if KOTOR1
    void SetAchText();
#endif

    void OnStyleLeft(CSWGuiControl* pControl);
    void OnStyleRight(CSWGuiControl* pControl);
    void OnEnterStyle(CSWGuiControl* pControl);
    void OnExitStyle(CSWGuiControl* pControl);
    void OnNoPitch(CSWGuiControl* pControl);
    void OnEnterNoPitch(CSWGuiControl* pControl);
    void OnTextScaleLeft(CSWGuiControl* pControl);
    void OnTextScaleRight(CSWGuiControl* pControl);
    void OnEnterTextScale(CSWGuiControl* pControl);
    void OnExitTextScale(CSWGuiControl* pControl);
    void OnRumbleLeft(CSWGuiControl* pControl);
    void OnRumbleRight(CSWGuiControl* pControl);
    void OnEnterRumble(CSWGuiControl* pControl);
    void OnExitRumble(CSWGuiControl* pControl);
    void OnSwapLeft(CSWGuiControl* pControl);
    void OnSwapRight(CSWGuiControl* pControl);
    void OnEnterSwap(CSWGuiControl* pControl);
    void OnExitSwap(CSWGuiControl* pControl);
    void OnMoveLeft(CSWGuiControl* pControl);
    void OnMoveRight(CSWGuiControl* pControl);
    void OnEnterMove(CSWGuiControl* pControl);
    void OnExitMove(CSWGuiControl* pControl);
#if KOTOR1
    void OnAchLeft(CSWGuiControl* pControl);
    void OnAchRight(CSWGuiControl* pControl);
    void OnEnterAch(CSWGuiControl* pControl);
    void OnExitAch(CSWGuiControl* pControl);
#endif
    void OnDefault(CSWGuiControl* pControl);

    // Clones the difficulty trio into a selector row's three buttons, `nOffsetY` below it.
    void CloneSelectorRow(CSWGuiButton& btn, CSWGuiButton& btnLeft, CSWGuiButton& btnRight,
                          int nOffsetY);
    // The Swap A/B and X/Y row, nOffsetY below the prompts row's layout place, and its events.
    void AddSwapRow(int nOffsetY);
    // The Faster Movement row, nOffsetY below the prompts row's layout place, and its events.
    void AddMoveRow(int nOffsetY);
#if KOTOR1
    // The Achievements row, nOffsetY below the prompts row's layout place, and its events.
    void AddAchRow(int nOffsetY);
#endif

    // CSWGuiInGameGameplay's widget set, minus the controls this page does not bind.
    CSWGuiLabel           m_labelDesc;      // unregistered: the description row LB_DESC displays
    CSWGuiLabel           m_labelTitle;     // LBL_TITLE
    CSWGuiListBox         m_listBoxDesc;    // LB_DESC
    CSWGuiButton          m_btnBack;        // BTN_BACK
    CSWGuiButton          m_btnDefault;     // BTN_DEFAULT
    CSWGuiButton          m_btnStyle;       // BTN_DIFFICULTY
    CSWGuiButton          m_btnStyleRight;  // BTN_DIFFRIGHT
    CSWGuiButton          m_btnStyleLeft;   // BTN_DIFFLEFT
#if !KOTOR1
    CSWGuiOptionsCheckbox m_cbNoPitch;      // CB_LEVELUP
    CSWGuiLabel           m_aLabelBar[4];   // LBL_BAR1..4
    CSWGuiButton          m_btnTextScale;       // cloned from BTN_DIFFICULTY, in its place
    CSWGuiButton          m_btnTextScaleRight;  // cloned from BTN_DIFFRIGHT
    CSWGuiButton          m_btnTextScaleLeft;   // cloned from BTN_DIFFLEFT
#endif
    CSWGuiButton          m_btnRumble;          // the same, three rows down (K1: one)
#if !KOTOR1
    // Moves a row down by nOffsetY, its text box with it.
    static void ShiftRow(CSWGuiControl& control, int nOffsetY);
#endif
    CSWGuiButton          m_btnRumbleRight;
    CSWGuiButton          m_btnRumbleLeft;
    CSWGuiButton          m_btnSwap;            // one row below rumble
    CSWGuiButton          m_btnSwapRight;
    CSWGuiButton          m_btnSwapLeft;
    CSWGuiButton          m_btnMove;            // one row below the swap
    CSWGuiButton          m_btnMoveRight;
    CSWGuiButton          m_btnMoveLeft;
#if KOTOR1
    CSWGuiButton          m_btnAch;             // one row below Faster Movement
    CSWGuiButton          m_btnAchRight;
    CSWGuiButton          m_btnAchLeft;
#endif
    int                   m_nStyle;
    int                   m_nTextScale;
    int                   m_nRumbleIndex;       // into RumbleOrder()
};

typedef void (CSWGuiObject::*K2GuiHandler)(CSWGuiControl*);
#define K2_HANDLER(fn) (static_cast<K2GuiHandler>(&K2GuiOptionsController::fn))

#if KOTOR1
K2GuiOptionsController::K2GuiOptionsController(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager), m_nStyle(STYLE_AUTO), m_nTextScale(100), m_nRumbleIndex(0)
{
    StartLoadFromLayout(CResRef("optgameplay"), 1, 1, 1);
    InitControl(m_btnStyle,      CExoString("BTN_DIFFICULTY"), 1, 1, 0, 0, 0);
    InitControl(m_labelTitle,    CExoString("LBL_TITLE"),      1, 1, 0, 0, 0);
    InitControl(m_listBoxDesc,   CExoString("LB_DESC"),        1, 1, 0, 0, 0);
    InitControl(m_btnStyleRight, CExoString("BTN_DIFFRIGHT"),  1, 1, 0, 0, 0);
    InitControl(m_btnStyleLeft,  CExoString("BTN_DIFFLEFT"),   1, 1, 0, 0, 0);
    InitControl(m_btnBack,       CExoString("BTN_BACK"),       1, 1, 0, 0, 0);
    InitControl(m_btnDefault,    CExoString("BTN_DEFAULT"),    1, 1, 0, 0, 0);
    m_listBoxDesc.m_nScrollSound = 1;
    StopLoadFromLayout();
    ScaleExtentForResolution();

    m_labelTitle.m_text.m_params.SetText(CExoString("Other Settings"));
    CSWGuiControl* pProto = m_listBoxDesc.m_pProtoItem;
    m_labelDesc.Initialize(pProto->m_extent, static_cast<CSWGuiLabel*>(pProto), 1.0f);

    m_btnStyle.AddEvent(0x2f, this, K2_HANDLER(OnStyleLeft));
    m_btnStyle.AddEvent(0x30, this, K2_HANDLER(OnStyleRight));
    m_btnStyle.AddEvent(0x3f, this, K2_HANDLER(OnStyleLeft));
    m_btnStyle.AddEvent(0x40, this, K2_HANDLER(OnStyleRight));
    m_btnStyle.AddEvent(0, this, K2_HANDLER(OnEnterStyle));
    m_btnStyle.AddEvent(1, this, K2_HANDLER(OnExitStyle));
    m_btnStyle.AddEvent(0x2a, this, K2_HANDLER(OnDefault));
    m_btnStyleLeft.m_tail.bFocusOnHover = 0;
    m_btnStyleLeft.AddEvent(0x27, this, K2_HANDLER(OnStyleLeft));
    m_btnStyleRight.m_tail.bFocusOnHover = 0;
    m_btnStyleRight.AddEvent(0x27, this, K2_HANDLER(OnStyleRight));

    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(0x27, this, static_cast<K2GuiHandler>(&CSWGuiPanel::OnBButtonPressed));
    m_btnDefault.m_tail.bFocusOnHover = 0;
    m_btnDefault.AddEvent(0x27, this, K2_HANDLER(OnDefault));

    AspyrAddButtonIcon(&m_btnBack, 'b');
    AspyrAddButtonIcon(&m_btnDefault, 'y');

    // K1's rows are a button's height apart (the layout's 39 of 40).
    CloneSelectorRow(m_btnRumble, m_btnRumbleLeft, m_btnRumbleRight, m_btnStyle.m_extent.h);
    m_btnRumble.AddEvent(0x2f, this, K2_HANDLER(OnRumbleLeft));
    m_btnRumble.AddEvent(0x30, this, K2_HANDLER(OnRumbleRight));
    m_btnRumble.AddEvent(0x3f, this, K2_HANDLER(OnRumbleLeft));
    m_btnRumble.AddEvent(0x40, this, K2_HANDLER(OnRumbleRight));
    m_btnRumble.AddEvent(0, this, K2_HANDLER(OnEnterRumble));
    m_btnRumble.AddEvent(1, this, K2_HANDLER(OnExitRumble));
    m_btnRumble.AddEvent(0x2a, this, K2_HANDLER(OnDefault));
    m_btnRumbleLeft.AddEvent(0x27, this, K2_HANDLER(OnRumbleLeft));
    m_btnRumbleRight.AddEvent(0x27, this, K2_HANDLER(OnRumbleRight));

    AddSwapRow(m_btnStyle.m_extent.h * 2);
    m_btnSwap.AddEvent(0x2a, this, K2_HANDLER(OnDefault));
    AddMoveRow(m_btnStyle.m_extent.h * 3);
    m_btnMove.AddEvent(0x2a, this, K2_HANDLER(OnDefault));
    AddAchRow(m_btnStyle.m_extent.h * 4);
    m_btnAch.AddEvent(0x2a, this, K2_HANDLER(OnDefault));

    // m_moveToControls: 0 up, 1 left, 2 down, 3 right; the rows wrap like K1's Gameplay column.
    m_btnStyle.m_moveToControls[0] = &m_btnAch;
    m_btnStyle.m_moveToControls[2] = &m_btnRumble;
    m_btnRumble.m_moveToControls[0] = &m_btnStyle;
    m_btnRumble.m_moveToControls[2] = &m_btnSwap;
    m_btnSwap.m_moveToControls[0] = &m_btnRumble;
    m_btnSwap.m_moveToControls[2] = &m_btnMove;
    m_btnMove.m_moveToControls[0] = &m_btnSwap;
    m_btnMove.m_moveToControls[2] = &m_btnAch;
    m_btnAch.m_moveToControls[0] = &m_btnMove;
    m_btnAch.m_moveToControls[2] = &m_btnStyle;

    SetActiveControl(&m_btnStyle, 0);
    SetUpOptions();
    K2QolNarrowSelectorCaption(m_btnStyle, m_btnStyleLeft, m_btnStyleRight);
    K2QolNarrowSelectorCaption(m_btnRumble, m_btnRumbleLeft, m_btnRumbleRight);
    K2QolNarrowSelectorCaption(m_btnSwap, m_btnSwapLeft, m_btnSwapRight);
    K2QolNarrowSelectorCaption(m_btnMove, m_btnMoveLeft, m_btnMoveRight);
    K2QolNarrowSelectorCaption(m_btnAch, m_btnAchLeft, m_btnAchRight);
}
#else
K2GuiOptionsController::K2GuiOptionsController(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager), m_nStyle(STYLE_AUTO), m_nTextScale(kTextScaleMin),
      m_nRumbleIndex(0)
{
    StartLoadFromLayout(CResRef("optgameplay_p"), 1, 1, 1);
    InitControl(m_btnStyle,      CExoString("BTN_DIFFICULTY"), 1, 1, 0, 0, 0);
    InitControl(m_cbNoPitch,     CExoString("CB_LEVELUP"),     1, 1, 0, 0, 0);
    InitControl(m_labelTitle,    CExoString("LBL_TITLE"),      1, 1, 0, 0, 0);
    InitControl(m_listBoxDesc,   CExoString("LB_DESC"),        1, 1, 0, 0, 0);
    InitControl(m_btnStyleRight, CExoString("BTN_DIFFRIGHT"),  1, 1, 0, 0, 0);
    InitControl(m_btnStyleLeft,  CExoString("BTN_DIFFLEFT"),   1, 1, 0, 0, 0);
    InitControl(m_btnBack,       CExoString("BTN_BACK"),       1, 1, 0, 0, 0);
    InitControl(m_btnDefault,    CExoString("BTN_DEFAULT"),    1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[0],  CExoString("LBL_BAR1"),       1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[1],  CExoString("LBL_BAR2"),       1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[2],  CExoString("LBL_BAR3"),       1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[3],  CExoString("LBL_BAR4"),       1, 1, 0, 0, 0);
    m_listBoxDesc.m_nScrollSound = 1;
    StopLoadFromLayout();

    m_labelTitle.m_text.m_params.SetText(CExoString("Other Settings"));
    m_cbNoPitch.m_text.m_params.SetText(CExoString(kNoPitchCaption));

    // BTN_DEFAULT is sized to its caption the way Gameplay sizes it (ExpandButtonToFitText): the
    // ideal text width plus three times its height, never narrower than the layout's.
    CSWGuiExtent btnExtent = m_btnDefault.m_extent;
    int nComputed = btnExtent.h * 3 + m_btnDefault.m_text.GetIdealWidthAndHeight().w;
    btnExtent.w = btnExtent.w > nComputed ? btnExtent.w : nComputed;
    static_cast<CSWGuiControl*>(&m_btnDefault)->SetExtent(btnExtent);

    CSWGuiControl* pProto = m_listBoxDesc.m_pProtoItem;
    CSWGuiExtent extentDesc = pProto->m_extent;
    extentDesc.h = static_cast<int>(GetResolutionTextScalar() * static_cast<float>(extentDesc.h));
    m_labelDesc.Initialize(extentDesc, static_cast<CSWGuiLabel*>(pProto), 1.0f);

    // The difficulty row's wiring, verbatim. The four arrow keys step the style, focus colours the
    // caption and shows the description, and the two arrow buttons step it on a click.
    m_btnStyle.AddEvent(0x2f, this, K2_HANDLER(OnStyleLeft));
    m_btnStyle.AddEvent(0x30, this, K2_HANDLER(OnStyleRight));
    m_btnStyle.AddEvent(0x3f, this, K2_HANDLER(OnStyleLeft));
    m_btnStyle.AddEvent(0x40, this, K2_HANDLER(OnStyleRight));
    m_btnStyle.AddEvent(0, this, K2_HANDLER(OnEnterStyle));
    m_btnStyle.AddEvent(1, this, K2_HANDLER(OnExitStyle));
    m_btnStyleLeft.m_tail.bFocusOnHover = 0;
    m_btnStyleLeft.AddEvent(0x27, this, K2_HANDLER(OnStyleLeft));
    m_btnStyleRight.m_tail.bFocusOnHover = 0;
    m_btnStyleRight.AddEvent(0x27, this, K2_HANDLER(OnStyleRight));

    m_cbNoPitch.AddEvent(0x27, this, K2_HANDLER(OnNoPitch));
    m_cbNoPitch.AddEvent(0x2d, this, K2_HANDLER(OnNoPitch));
    m_cbNoPitch.AddEvent(0, this, K2_HANDLER(OnEnterNoPitch));

    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(0x27, this, static_cast<K2GuiHandler>(&CSWGuiPanel::OnBButtonPressed));
    m_btnDefault.m_tail.bFocusOnHover = 0;
    m_btnDefault.AddEvent(0x27, this, K2_HANDLER(OnDefault));

    AspyrAddButtonIcon(&m_btnBack, 'b');
    AspyrAddButtonIcon(&m_btnDefault, 'y');

    // The row pitch is taken between row centres, because the checkbox's box is taller than the
    // button (60 against 43 at 1920x1080), so a top-edge pitch puts the new rows visibly too close.
    int nPitch = (m_cbNoPitch.m_extent.y + m_cbNoPitch.m_extent.h / 2)
               - (m_btnStyle.m_extent.y + m_btnStyle.m_extent.h / 2);

    CloneSelectorRow(m_btnTextScale, m_btnTextScaleLeft, m_btnTextScaleRight, 0);
    m_btnTextScale.AddEvent(0x2f, this, K2_HANDLER(OnTextScaleLeft));
    m_btnTextScale.AddEvent(0x30, this, K2_HANDLER(OnTextScaleRight));
    m_btnTextScale.AddEvent(0x3f, this, K2_HANDLER(OnTextScaleLeft));
    m_btnTextScale.AddEvent(0x40, this, K2_HANDLER(OnTextScaleRight));
    m_btnTextScale.AddEvent(0, this, K2_HANDLER(OnEnterTextScale));
    m_btnTextScale.AddEvent(1, this, K2_HANDLER(OnExitTextScale));
    m_btnTextScaleLeft.AddEvent(0x27, this, K2_HANDLER(OnTextScaleLeft));
    m_btnTextScaleRight.AddEvent(0x27, this, K2_HANDLER(OnTextScaleRight));

    CloneSelectorRow(m_btnRumble, m_btnRumbleLeft, m_btnRumbleRight, nPitch * 3);
    m_btnRumble.AddEvent(0x2f, this, K2_HANDLER(OnRumbleLeft));
    m_btnRumble.AddEvent(0x30, this, K2_HANDLER(OnRumbleRight));
    m_btnRumble.AddEvent(0x3f, this, K2_HANDLER(OnRumbleLeft));
    m_btnRumble.AddEvent(0x40, this, K2_HANDLER(OnRumbleRight));
    m_btnRumble.AddEvent(0, this, K2_HANDLER(OnEnterRumble));
    m_btnRumble.AddEvent(1, this, K2_HANDLER(OnExitRumble));
    m_btnRumbleLeft.AddEvent(0x27, this, K2_HANDLER(OnRumbleLeft));
    m_btnRumbleRight.AddEvent(0x27, this, K2_HANDLER(OnRumbleRight));

    AddSwapRow(nPitch * 4);
    AddMoveRow(nPitch * 5);

    // Text size took the prompts' row: the prompts and the checkbox move down one.
    ShiftRow(m_btnStyle, nPitch);
    ShiftRow(m_btnStyleLeft, nPitch);
    ShiftRow(m_btnStyleRight, nPitch);
    ShiftRow(m_cbNoPitch, nPitch);

    // The layout's pad chain runs through rows this page leaves unbound (CB_LEVELUP's "down" is
    // CB_INVERTCAM), so the five rows are chained to each other explicitly
    // (m_moveToControls: 0 up, 1 left, 2 down, 3 right).
    m_btnTextScale.m_moveToControls[0] = 0;
    m_btnTextScale.m_moveToControls[2] = &m_btnStyle;
    m_btnStyle.m_moveToControls[0] = &m_btnTextScale;
    m_btnStyle.m_moveToControls[2] = &m_cbNoPitch;
    m_cbNoPitch.m_moveToControls[0] = &m_btnStyle;
    m_cbNoPitch.m_moveToControls[2] = &m_btnRumble;
    m_btnRumble.m_moveToControls[0] = &m_cbNoPitch;
    m_btnRumble.m_moveToControls[2] = &m_btnSwap;
    m_btnSwap.m_moveToControls[0] = &m_btnRumble;
    m_btnSwap.m_moveToControls[2] = &m_btnMove;
    m_btnMove.m_moveToControls[0] = &m_btnSwap;

    SetActiveControl(&m_btnTextScale, 0);
    SetUpOptions();
    K2QolNarrowSelectorCaption(m_btnStyle, m_btnStyleLeft, m_btnStyleRight);
    K2QolNarrowSelectorCaption(m_btnTextScale, m_btnTextScaleLeft, m_btnTextScaleRight);
    K2QolNarrowSelectorCaption(m_btnRumble, m_btnRumbleLeft, m_btnRumbleRight);
    K2QolNarrowSelectorCaption(m_btnSwap, m_btnSwapLeft, m_btnSwapRight);
    K2QolNarrowSelectorCaption(m_btnMove, m_btnMoveLeft, m_btnMoveRight);
}
#endif

void K2GuiOptionsController::AddSwapRow(int nOffsetY)
{
    CloneSelectorRow(m_btnSwap, m_btnSwapLeft, m_btnSwapRight, nOffsetY);
    m_btnSwap.AddEvent(0x2f, this, K2_HANDLER(OnSwapLeft));
    m_btnSwap.AddEvent(0x30, this, K2_HANDLER(OnSwapRight));
    m_btnSwap.AddEvent(0x3f, this, K2_HANDLER(OnSwapLeft));
    m_btnSwap.AddEvent(0x40, this, K2_HANDLER(OnSwapRight));
    m_btnSwap.AddEvent(0, this, K2_HANDLER(OnEnterSwap));
    m_btnSwap.AddEvent(1, this, K2_HANDLER(OnExitSwap));
    m_btnSwapLeft.AddEvent(0x27, this, K2_HANDLER(OnSwapLeft));
    m_btnSwapRight.AddEvent(0x27, this, K2_HANDLER(OnSwapRight));
}

void K2GuiOptionsController::AddMoveRow(int nOffsetY)
{
    CloneSelectorRow(m_btnMove, m_btnMoveLeft, m_btnMoveRight, nOffsetY);
    m_btnMove.AddEvent(0x2f, this, K2_HANDLER(OnMoveLeft));
    m_btnMove.AddEvent(0x30, this, K2_HANDLER(OnMoveRight));
    m_btnMove.AddEvent(0x3f, this, K2_HANDLER(OnMoveLeft));
    m_btnMove.AddEvent(0x40, this, K2_HANDLER(OnMoveRight));
    m_btnMove.AddEvent(0, this, K2_HANDLER(OnEnterMove));
    m_btnMove.AddEvent(1, this, K2_HANDLER(OnExitMove));
    m_btnMoveLeft.AddEvent(0x27, this, K2_HANDLER(OnMoveLeft));
    m_btnMoveRight.AddEvent(0x27, this, K2_HANDLER(OnMoveRight));
}

#if KOTOR1
void K2GuiOptionsController::AddAchRow(int nOffsetY)
{
    CloneSelectorRow(m_btnAch, m_btnAchLeft, m_btnAchRight, nOffsetY);
    m_btnAch.AddEvent(0x2f, this, K2_HANDLER(OnAchLeft));
    m_btnAch.AddEvent(0x30, this, K2_HANDLER(OnAchRight));
    m_btnAch.AddEvent(0x3f, this, K2_HANDLER(OnAchLeft));
    m_btnAch.AddEvent(0x40, this, K2_HANDLER(OnAchRight));
    m_btnAch.AddEvent(0, this, K2_HANDLER(OnEnterAch));
    m_btnAch.AddEvent(1, this, K2_HANDLER(OnExitAch));
    m_btnAchLeft.AddEvent(0x27, this, K2_HANDLER(OnAchLeft));
    m_btnAchRight.AddEvent(0x27, this, K2_HANDLER(OnAchRight));
}
#endif

#if !KOTOR1
void K2GuiOptionsController::ShiftRow(CSWGuiControl& control, int nOffsetY)
{
    CSWGuiExtent extent = control.m_extent;
    extent.y += nOffsetY;
    control.SetExtent(extent);
}
#endif

// CSWGuiButton::Initialize(extent, template) copies the template's border, highlight and text
// parameters, the call K2QolAddControllerButton styles its button with.
void K2GuiOptionsController::CloneSelectorRow(CSWGuiButton& btn, CSWGuiButton& btnLeft,
                                              CSWGuiButton& btnRight, int nOffsetY)
{
    CSWGuiExtent extent = m_btnStyle.m_extent;
    extent.y += nOffsetY;
    btn.Initialize(extent, &m_btnStyle);
    extent = m_btnStyleLeft.m_extent;
    extent.y += nOffsetY;
    btnLeft.Initialize(extent, &m_btnStyleLeft);
    extent = m_btnStyleRight.m_extent;
    extent.y += nOffsetY;
    btnRight.Initialize(extent, &m_btnStyleRight);
    AddControl(&btn);
    AddControl(&btnLeft);
    AddControl(&btnRight);
    for (int i = 0; i < 4; i++)
        btn.m_moveToControls[i] = 0;
    btnLeft.m_tail.bFocusOnHover = 0;
    btnRight.m_tail.bFocusOnHover = 0;
}

void K2GuiOptionsController::SetUpOptions()
{
    m_nStyle = k2qol_icons();
    if (m_nStyle < STYLE_AUTO || m_nStyle >= NUM_STYLES)
        m_nStyle = STYLE_AUTO;
    SetStyleText();
#if !KOTOR1
    m_cbNoPitch.bToggledOn = k2qol_no_pitch() != 0;
    m_nTextScale = k2qol_dialog_text_scale();
    SetTextScaleText();
#endif
    int nRumble = k2qol_rumble();
    m_nRumbleIndex = 0;
    for (int i = 0; i < NumRumble(); i++)
        if (RumbleOrder()[i] == nRumble)
            m_nRumbleIndex = i;
    SetRumbleText();
    SetSwapText();
    SetMoveText();
#if KOTOR1
    SetAchText();
#endif
}

// CSWGuiInGameGameplay::SetDifficultyText's shape: re-title the button, then hide the left arrow at
// the bottom of the range and the right one at the top.
void K2GuiOptionsController::SetStyleText()
{
    m_btnStyle.m_text.m_params.SetText(CExoString(kStyleCaption[m_nStyle]));
    m_btnStyleLeft.m_tail.bVisible = m_nStyle != STYLE_AUTO;
    m_btnStyleRight.m_tail.bVisible = m_nStyle != NUM_STYLES - 1;
}

void K2GuiOptionsController::ShowStyleDescription()
{
    const char* pText = kStyleDescription[m_nStyle];
    char szText[512];
    if (!k2qol_have_art(m_nStyle)) {
        // The choice's first sentence, then why it shows other art.
        const char* pEnd = strstr(pText, "\n\n");
        snprintf(szText, sizeof szText, "%.*s%s", pEnd ? static_cast<int>(pEnd - pText) : static_cast<int>(strlen(pText)),
                 pText, kNoArtDescription);
        pText = szText;
    }
    ShowDescription(m_listBoxDesc, m_labelDesc, CExoString(pText));
}

void K2GuiOptionsController::OnStyleLeft(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg || m_nStyle <= STYLE_AUTO)
        return;
    k2qol_set_icons(--m_nStyle);
    SetStyleText();
    ShowStyleDescription();
    m_pGuiManager->PlayGuiSound(1);
}

void K2GuiOptionsController::OnStyleRight(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg || m_nStyle >= NUM_STYLES - 1)
        return;
    k2qol_set_icons(++m_nStyle);
    SetStyleText();
    ShowStyleDescription();
    m_pGuiManager->PlayGuiSound(1);
}

void K2GuiOptionsController::OnEnterStyle(CSWGuiControl* pControl)
{
    m_btnStyle.m_text.m_params.SetColor(K2QOL_COLOR_FOCUS);
    m_btnStyle.m_text.m_params.bHilight = 1;
    SetStyleText();
    ShowStyleDescription();
}

void K2GuiOptionsController::OnExitStyle(CSWGuiControl* pControl)
{
    m_btnStyle.m_text.m_params.SetColor(K2QOL_COLOR_TEXT);
    m_btnStyle.m_text.m_params.bHilight = 0;
}

#if !KOTOR1
// The check is already flipped by the click (CSWGuiButtonToggle::HandleInputEvent); the setting is
// flipped to match and the row re-synced from it, so the two cannot disagree.
void K2GuiOptionsController::OnNoPitch(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg)
        return;
    k2qol_set_no_pitch(!k2qol_no_pitch());
    SetUpOptions();
}

void K2GuiOptionsController::OnEnterNoPitch(CSWGuiControl* pControl)
{
    ShowDescription(m_listBoxDesc, m_labelDesc, CExoString(kNoPitchDescription));
}

void K2GuiOptionsController::SetTextScaleText()
{
    CExoString sText;
    sText.Format(const_cast<char*>("Text Size: %d%%"), m_nTextScale);
    m_btnTextScale.m_text.m_params.SetText(sText);
    m_btnTextScaleLeft.m_tail.bVisible = m_nTextScale > kTextScaleMin;
    m_btnTextScaleRight.m_tail.bVisible = m_nTextScale < kTextScaleMax;
}

void K2GuiOptionsController::OnTextScaleLeft(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg || m_nTextScale <= kTextScaleMin)
        return;
    m_nTextScale -= kTextScaleStep;
    k2qol_set_dialog_text_scale(m_nTextScale);
    SetTextScaleText();
    m_pGuiManager->PlayGuiSound(1);
}

void K2GuiOptionsController::OnTextScaleRight(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg || m_nTextScale >= kTextScaleMax)
        return;
    m_nTextScale += kTextScaleStep;
    k2qol_set_dialog_text_scale(m_nTextScale);
    SetTextScaleText();
    m_pGuiManager->PlayGuiSound(1);
}

void K2GuiOptionsController::OnEnterTextScale(CSWGuiControl* pControl)
{
    m_btnTextScale.m_text.m_params.SetColor(K2QOL_COLOR_FOCUS);
    m_btnTextScale.m_text.m_params.bHilight = 1;
    SetTextScaleText();
    ShowDescription(m_listBoxDesc, m_labelDesc, CExoString(kTextScaleDescription));
}

void K2GuiOptionsController::OnExitTextScale(CSWGuiControl* pControl)
{
    m_btnTextScale.m_text.m_params.SetColor(K2QOL_COLOR_TEXT);
    m_btnTextScale.m_text.m_params.bHilight = 0;
}

#endif

void K2GuiOptionsController::SetRumbleText()
{
    m_btnRumble.m_text.m_params.SetText(CExoString(kRumbleCaption[RumbleOrder()[m_nRumbleIndex]]));
    m_btnRumbleLeft.m_tail.bVisible = m_nRumbleIndex > 0;
    m_btnRumbleRight.m_tail.bVisible = m_nRumbleIndex < NumRumble() - 1;
}

void K2GuiOptionsController::ShowRumbleDescription()
{
    ShowDescription(m_listBoxDesc, m_labelDesc,
                    CExoString(kRumbleDescription[RumbleOrder()[m_nRumbleIndex]]));
}

void K2GuiOptionsController::OnRumbleLeft(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg || m_nRumbleIndex <= 0)
        return;
    k2qol_set_rumble(RumbleOrder()[--m_nRumbleIndex]);
    k2qol_test_rumble();   // feel the device the new choice drives
    SetRumbleText();
    ShowRumbleDescription();
    m_pGuiManager->PlayGuiSound(1);
}

void K2GuiOptionsController::OnRumbleRight(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg || m_nRumbleIndex >= NumRumble() - 1)
        return;
    k2qol_set_rumble(RumbleOrder()[++m_nRumbleIndex]);
    k2qol_test_rumble();
    SetRumbleText();
    ShowRumbleDescription();
    m_pGuiManager->PlayGuiSound(1);
}

void K2GuiOptionsController::OnEnterRumble(CSWGuiControl* pControl)
{
    m_btnRumble.m_text.m_params.SetColor(K2QOL_COLOR_FOCUS);
    m_btnRumble.m_text.m_params.bHilight = 1;
    SetRumbleText();
    ShowRumbleDescription();
}

void K2GuiOptionsController::OnExitRumble(CSWGuiControl* pControl)
{
    m_btnRumble.m_text.m_params.SetColor(K2QOL_COLOR_TEXT);
    m_btnRumble.m_text.m_params.bHilight = 0;
}

void K2GuiOptionsController::SetSwapText()
{
    const int bOn = k2qol_swap_face_buttons() != 0;
    m_btnSwap.m_text.m_params.SetText(CExoString(kSwapCaption[bOn]));
    m_btnSwapLeft.m_tail.bVisible = bOn;
    m_btnSwapRight.m_tail.bVisible = !bOn;
}

// Off is the left end, On the right.
void K2GuiOptionsController::OnSwapLeft(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg || !k2qol_swap_face_buttons())
        return;
    k2qol_set_swap_face_buttons(0);
    SetSwapText();
    m_pGuiManager->PlayGuiSound(1);
}

void K2GuiOptionsController::OnSwapRight(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg || k2qol_swap_face_buttons())
        return;
    k2qol_set_swap_face_buttons(1);
    SetSwapText();
    m_pGuiManager->PlayGuiSound(1);
}

void K2GuiOptionsController::OnEnterSwap(CSWGuiControl* pControl)
{
    m_btnSwap.m_text.m_params.SetColor(K2QOL_COLOR_FOCUS);
    m_btnSwap.m_text.m_params.bHilight = 1;
    SetSwapText();
    ShowDescription(m_listBoxDesc, m_labelDesc, CExoString(kSwapDescription));
}

void K2GuiOptionsController::OnExitSwap(CSWGuiControl* pControl)
{
    m_btnSwap.m_text.m_params.SetColor(K2QOL_COLOR_TEXT);
    m_btnSwap.m_text.m_params.bHilight = 0;
}

void K2GuiOptionsController::SetMoveText()
{
    const int bOn = k2qol_faster_movement() != 0;
    m_btnMove.m_text.m_params.SetText(CExoString(kMoveCaption[bOn]));
    m_btnMoveLeft.m_tail.bVisible = bOn;
    m_btnMoveRight.m_tail.bVisible = !bOn;
}

// Off is the left end, On the right.
void K2GuiOptionsController::OnMoveLeft(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg || !k2qol_faster_movement())
        return;
    k2qol_set_faster_movement(0);
    SetMoveText();
    m_pGuiManager->PlayGuiSound(1);
}

void K2GuiOptionsController::OnMoveRight(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg || k2qol_faster_movement())
        return;
    k2qol_set_faster_movement(1);
    SetMoveText();
    m_pGuiManager->PlayGuiSound(1);
}

void K2GuiOptionsController::OnEnterMove(CSWGuiControl* pControl)
{
    m_btnMove.m_text.m_params.SetColor(K2QOL_COLOR_FOCUS);
    m_btnMove.m_text.m_params.bHilight = 1;
    SetMoveText();
    ShowDescription(m_listBoxDesc, m_labelDesc, CExoString(kMoveDescription));
}

void K2GuiOptionsController::OnExitMove(CSWGuiControl* pControl)
{
    m_btnMove.m_text.m_params.SetColor(K2QOL_COLOR_TEXT);
    m_btnMove.m_text.m_params.bHilight = 0;
}

#if KOTOR1
void K2GuiOptionsController::SetAchText()
{
    const int bOn = k2qol_achievements() != 0;
    m_btnAch.m_text.m_params.SetText(CExoString(kAchCaption[bOn]));
    m_btnAchLeft.m_tail.bVisible = bOn;
    m_btnAchRight.m_tail.bVisible = !bOn;
}

// Off is the left end, On the right.
void K2GuiOptionsController::OnAchLeft(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg || !k2qol_achievements())
        return;
    k2qol_set_achievements(0);
    SetAchText();
    m_pGuiManager->PlayGuiSound(1);
}

void K2GuiOptionsController::OnAchRight(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg || k2qol_achievements())
        return;
    k2qol_set_achievements(1);
    SetAchText();
    m_pGuiManager->PlayGuiSound(1);
}

void K2GuiOptionsController::OnEnterAch(CSWGuiControl* pControl)
{
    m_btnAch.m_text.m_params.SetColor(K2QOL_COLOR_FOCUS);
    m_btnAch.m_text.m_params.bHilight = 1;
    SetAchText();
    ShowDescription(m_listBoxDesc, m_labelDesc, CExoString(kAchDescription));
}

void K2GuiOptionsController::OnExitAch(CSWGuiControl* pControl)
{
    m_btnAch.m_text.m_params.SetColor(K2QOL_COLOR_TEXT);
    m_btnAch.m_text.m_params.bHilight = 0;
}
#endif

void K2GuiOptionsController::OnDefault(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg)
        return;
    k2qol_set_icons(STYLE_AUTO);
#if !KOTOR1
    k2qol_set_no_pitch(0);
    k2qol_set_dialog_text_scale(kTextScaleMin);
#endif
    k2qol_set_rumble(RumbleOrder()[0]);
    k2qol_set_swap_face_buttons(0);
    k2qol_set_faster_movement(1);
#if KOTOR1
    k2qol_set_achievements(1);
#endif
    SetUpOptions();
    if (m_pActiveControl == &m_btnStyle)
        ShowStyleDescription();
}

// CSWGuiInGameGameplay::HandleInputEvent's arms, minus its pre-toggle on the pause event: OnNoPitch
// re-syncs the checkbox from the setting, so flipping it here first would only be undone.
void K2GuiOptionsController::HandleInputEvent(int type, int val)
{
    if (val != 0) {
        switch (type) {
        case 0x28:
        case 0x2e: {
            m_pGuiManager->PlayGuiSound(0);
            m_pGuiManager->PopModalPanel();
#if KOTOR1
            m_stateFlags = (m_stateFlags & ~0x700u) | 0x400u;   // remove + delete (K1's bits)
#else
            CSWGuiPanel* pPanel = this;
            do {
                pPanel->m_stateFlags = (pPanel->m_stateFlags & ~0x380u) | 0x200u;   // remove + delete
                pPanel = pPanel->m_pNextInChain;
            } while (pPanel);
#endif
            break;
        }
#if !KOTOR1  // K1: Y is the rows' own 0x2a event, as on its Gameplay page
        case 0x2a:
            m_pGuiManager->PlayGuiSound(0);
            m_btnDefault.m_tail.nCurrentEventArg = 1;
            OnDefault(&m_btnDefault);
            break;
#endif
        case 0x39:
            m_listBoxDesc.HandleInputEvent(0x31, 1);
            break;
        case 0x3a:
            m_listBoxDesc.HandleInputEvent(0x32, 1);
            break;
        default:
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

// Adds the Options screen's Other Settings button (CSWGuiOptionsMain's constructor calls this last).

void CSWGuiOptionsMain::K2QolAddControllerButton()
{
    CSWGuiButton& above = m_aButtons[4];   // BTN_SOUND
    CSWGuiButton& button = m_aButtons[5];
    int nPitch = above.m_extent.y - m_aButtons[3].m_extent.y;   // Graphics -> Sound
    if (nPitch <= 0)
        nPitch = above.m_extent.h + 4;
    CSWGuiExtent extent = above.m_extent;
    extent.y += nPitch;

    button.Initialize(extent, &above);
    button.m_text.m_params.SetText(CExoString("Other Settings"));
    button.AddEvent(0, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMain::K2QolOnEnterController));
    button.AddEvent(0x27, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMain::K2QolOnController));
    button.AddEvent(0x2d, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiOptionsMain::K2QolOnController));
    button.m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);

    // Spliced into the pad's up/down chain under Sound (m_moveToControls: 0 up, 1 left, 2 down,
    // 3 right).
    CSWGuiControl* pBelow = above.m_moveToControls[2];
    button.m_moveToControls[0] = &above;
    button.m_moveToControls[1] = above.m_moveToControls[1];
    button.m_moveToControls[2] = pBelow;
    button.m_moveToControls[3] = above.m_moveToControls[3];
    above.m_moveToControls[2] = &button;
    if (pBelow && pBelow->AsNavigable()) {
        CSWGuiNavigable* pNav = static_cast<CSWGuiNavigable*>(pBelow->AsNavigable());
        if (pNav->m_moveToControls[0] == &above)
            pNav->m_moveToControls[0] = &button;
    }

    AddControl(&button);

#if KOTOR1
    // K1's five buttons sit centred on the description box beside them (the constructor's 20-unit
    // push down); six would hang below it, so the column is centred on the box again.
    int nTop = m_aButtons[0].m_extent.y;
    int nBottom = button.m_extent.y + button.m_extent.h;
    int nShift = (m_listBoxDesc.m_extent.y + m_listBoxDesc.m_extent.h / 2) - (nTop + nBottom) / 2;
    for (int i = 0; i < 6; i++) {
        CSWGuiExtent buttonExtent = m_aButtons[i].m_extent;
        buttonExtent.y += nShift;
        static_cast<CSWGuiControl*>(&m_aButtons[i])->SetExtent(buttonExtent);
    }
#endif
}

// CSWGuiOptionsMain::OnFeedbackOpt's three calls, with this page.
void CSWGuiOptionsMain::K2QolOnController(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg)
        return;
    K2GuiOptionsController* pPage = new K2GuiOptionsController(m_pGuiManager);
#if !KOTOR1  // as K1's own pages do (OnFeedbackOpt ...): no replacement border
    pPage->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
    m_pGuiManager->AddPanel(pPage, 3, 1);
}

void CSWGuiOptionsMain::K2QolOnEnterController(CSWGuiControl* pControl)
{
    ShowDescription(m_listBoxDesc, m_labelDesc, CExoString(kButtonDescription));
}

// A selector row (the Gameplay page's difficulty, this page's three) is a wide caption button with
// its two arrow buttons drawn inside its box, so a fitted caption takes the full width and runs under
// the arrows. Narrow its text box to the span between them, leaving the button, its border and its
// hit area as they are, and re-fit the caption to that.
void K2QolNarrowSelectorCaption(CSWGuiButton& button, const CSWGuiControl& left,
                                const CSWGuiControl& right)
{
    CSWGuiExtent extent = button.m_text.m_extent;
    int nLeft = left.m_extent.x + left.m_extent.w;
    int nRight = right.m_extent.x;
    int x0 = extent.x > nLeft ? extent.x : nLeft;
    int x1 = extent.x + extent.w < nRight ? extent.x + extent.w : nRight;
    if (x1 - x0 < extent.w / 3)
        return;   // the arrows are not inside the box after all: leave it
    extent.x = x0;
    extent.w = x1 - x0;
    button.m_text.SetExtent(extent);
}

#ifdef K2_QOL_IMPROVEMENTS
// Makes the dialog text size fit. A fixed cap per panel cannot work: the setting multiplies the
// game's own size, and how much room that leaves differs per box and per string (GetResolutionTextScalar
// is an integer width step, 1 below 2000 px wide and 2 from 2000, which made a cap fitted at
// 1280x720 0.67x as big against its box at 1920x1080; K2_QOL_IMPROVEMENTS makes it follow the GUI's
// height, aurgui.cpp). Text that outgrows its box does not shrink, it disappears: a list-box row
// taller than the box is never drawn (CSWGuiListBox::Draw's bClipPartialRows test), and a vertically
// centred string taller than its rect drops lines from the top (CAurGUIStringInternal::Draw).
//
// So every place that reads text is laid out -- the dialog panels, the load tip, the pop-ups and
// datapads, the HUD's action caption and target name, the Inventory, Equip and Abilities
// descriptions (which set their text after creation, so the size is applied here and not at
// NewCAurGUIString) -- and gets the largest size, in 1/16 steps of the game's own, whose wrapped text
// at the width it will draw at is at most nMaxHeight tall.
//
// A setting above 100 % never goes below the game's own size: text that does not fit even then is
// left exactly as the engine had it. One below 100 % is taken as it is. The string is reset to the
// setting first, so a long line that had to shrink does not shrink the next one. Returns the text's
// ideal height.
int K2QolFitText(CSWGuiText& text, int nMaxHeight)
{
    CAurGUIStringInternal* pString = text.m_pString;
    if (!pString)
        return 0;

    // A string centred in a box of no width carries a non-zero position with a zero wrap width.
    // WrapStrings bails only while the position is zero too, so wrapping such a string walks its scan
    // to the first byte and then reads the glyph before the text buffer (a heap-buffer-overflow one
    // byte before the string). Its box has no width, so there is nothing to measure: do not wrap.
    {
        int nPosX, nPosY, nSizeX, nSizeY;
        pString->GetPositionBoundingRect(nPosX, nPosY);
        pString->GetSizeBoundingRect(nSizeX, nSizeY);
        if (nSizeX <= 0 && nPosX != 0)
            return 0;
    }

    pString->UpdateFontScale();
    const float fBase = pString->K2QolGetResolutionScale();   // factor 1.0: the game's own size
    const float fStep = fBase / 16.0f;
    const float fWant = fBase * static_cast<float>(k2qol_dialog_text_scale()) / 100.0f;
    // A caption too wide for its box at this size (a digit in the chargen +/- boxes) wraps to no
    // lines: an ideal height of 0, which "fits" any box and draws nothing.
    const char* sText = text.m_params.m_sText.CStr();
    const bool bEmpty = !sText || !*sText;
    pString->K2QolSetSizeFactor(fWant);
    pString->WrapStrings(-1);
    int nHeight = text.GetIdealHeight();
    if (nHeight <= nMaxHeight && (nHeight > 0 || bEmpty))
        return nHeight;
    // Binary search over 1/16 steps between a floor (always accepted) and the setting (known too
    // tall): this runs on every caption change of a marked button. The floor is the game's size,
    // or, when that does not fit the box (the resolution scale can make a caption wrap past its
    // one-line box), half the smaller of it and the setting.
    float fFloor = fBase;
    if (fWant <= fBase) {
        fFloor = fWant * 0.5f;
    } else {
        pString->K2QolSetSizeFactor(fBase);
        pString->WrapStrings(-1);
        int nBaseHeight = text.GetIdealHeight();
        if (!(nBaseHeight <= nMaxHeight && (nBaseHeight > 0 || bEmpty)))
            fFloor = fBase * 0.5f;
    }
    int nLo = 0, nHi = static_cast<int>((fWant - fFloor) / fStep + 0.5f);
    while (nHi - nLo > 1) {
        int nMid = (nLo + nHi) / 2;
        pString->K2QolSetSizeFactor(fFloor + fStep * static_cast<float>(nMid));
        pString->WrapStrings(-1);
        int nMidHeight = text.GetIdealHeight();
        if (nMidHeight <= nMaxHeight && (nMidHeight > 0 || bEmpty))
            nLo = nMid;
        else
            nHi = nMid;
    }
    pString->K2QolSetSizeFactor(fFloor + fStep * static_cast<float>(nLo));
    pString->WrapStrings(-1);
    return text.GetIdealHeight();
}

// Button captions fit themselves. A caption is re-laid-out by CSWGuiText::WrapString on every change
// (SetText, SetExtent, the other params setters), so a button's text is marked here when the button
// is set up (CSWGuiButton::Load for layout-bound buttons, toggles and checkboxes included;
// CSWGuiButton::Initialize for template clones such as list rows), and WrapString then sizes a marked
// caption with K2QolFitText against its own box: as large as the setting asks while it still fits the
// button, never below the game's own size. ~CSWGuiText unmarks, so a later object at the same address
// is not mistaken for a button.
namespace {
// Never destroyed: CSWGuiText destructors of engine globals still run during exit.
std::unordered_set<const CSWGuiText*>& FitTexts()
{
    static std::unordered_set<const CSWGuiText*>* s_pSet = new std::unordered_set<const CSWGuiText*>;
    return *s_pSet;
}
}  // namespace

// The game's own height of each button list row, keyed by its caption, recorded the first time the
// row is added (before any growth), so the Text Size scales from it rather than compounding.
std::unordered_map<const CSWGuiText*, int>& RowBaseHeights()
{
    static std::unordered_map<const CSWGuiText*, int>* s_pMap = new std::unordered_map<const CSWGuiText*, int>;
    return *s_pMap;
}

void K2QolMarkFitText(CSWGuiText* pText) { FitTexts().insert(pText); }
void K2QolUnmarkFitText(CSWGuiText* pText)
{
    if (!FitTexts().empty())
        FitTexts().erase(pText);
    if (!RowBaseHeights().empty())
        RowBaseHeights().erase(pText);
}
bool K2QolIsFitText(const CSWGuiText* pText) { return FitTexts().count(pText) != 0; }

// A screen's label fits itself the same way (titles, captions, descriptions): marked, so every later
// SetText or SetStrRef re-fits it, and fitted now for the text the layout loaded.
void K2QolFitLabel(CSWGuiText* pText)
{
    K2QolMarkFitText(pText);
    if (pText->m_extent.h && pText->m_extent.w)
        K2QolFitText(*pText, pText->m_extent.h);
}

// List rows grow with their caption. Every screen sizes its own list rows (the save list a
// hard-coded 36 units, others the prototype row's height), so a row's caption has no room to grow in
// its box; CSWGuiListBox::AddControls calls this for each new row before recording its extent
// (OrganizeControls resets every row to that record and lays the rows out from it). A button row (the
// item, store, crafting and save entries, which bear an icon) takes the game's own row height times
// the setting first and more only if its caption needs it, so a list's rows are one size; a label row
// grows by what its caption lacks. Both at most a third of the list's height, and the re-layout then
// fits the caption to the box. Idempotent: the base height is remembered per row, so a row added again
// is sized from it, not from its already-grown height. Below 100 % the captions shrink and the rows
// keep the game's height.
void K2QolGrowListRow(CSWGuiListBox* pList, CSWGuiControl* pRow, int nRows)
{
    if (k2qol_dialog_text_scale() == 100)
        return;
    // A button row's caption (item, save and tab entries; radio buttons set up by their own
    // Initialize, so they are marked here) keeps fitting itself on every change. A label row (the
    // skill lists) is sized once, unmarked: a marked label would shrink to its old box on the next
    // SetText, before its screen re-measures it.
    // A one-row list of a label is a description pane (some 27 screens' SetDescription: Options, the
    // in-game Options tab, chargen, store, upgrade, ...), its row sized to its text just before this.
    // If the text is still at the game's own size, bring it to the setting and grow the row by the
    // difference; the pane scrolls its one row. A row its screen already sized (K2QolFitText at the
    // call site: inventory, journal, terminal, pop-ups) is not at the game's size and is left exactly
    // as its screen fitted it.
    if (nRows == 1 && pRow->AsLabel()) {
        CSWGuiText& text = static_cast<CSWGuiLabel*>(pRow->AsLabel())->m_text;
        CAurGUIStringInternal* pString = text.m_pString;
        if (!pString)
            return;
        pString->UpdateFontScale();
        if (pString->K2QolGetSizeFactor() != pString->K2QolGetResolutionScale())
            return;
        int nOld = text.GetIdealHeight();
        int nNew = K2QolFitText(text, 0x7fffffff);
        if (nNew > nOld) {
            CSWGuiExtent extent = pRow->m_extent;
            extent.h += nNew - nOld;
            pRow->SetExtent(extent);
        }
        return;
    }
    CSWGuiText* pText = 0;
    bool bMark = false;
    if (pRow->AsButton()) {
        pText = &static_cast<CSWGuiButton*>(pRow->AsButton())->m_text;
        bMark = true;
    } else if (pRow->AsLabel() && nRows > 1) {
        pText = &static_cast<CSWGuiLabel*>(pRow->AsLabel())->m_text;
    }
    if (!pText || !pText->m_pString || pText->m_extent.h <= 0)
        return;
    if (bMark)
        K2QolMarkFitText(pText);
    int nBox = pText->m_extent.h;
    int nWant = K2QolFitText(*pText, 0x7fffffff);   // the caption at the setting, at its width
    CSWGuiExtent extent = pRow->m_extent;
    int nRowMax = pList->m_innerExtent.h / 3;
    // The caption needs the row less its current caption box, plus the caption at the setting.
    int nNewH = extent.h - nBox + nWant;
    int nBase = extent.h;
    if (bMark) {
        // A button row (item, store, crafting and save entries: rows with an icon) first takes the
        // game's own row height times the setting, so every row of a list, and the same row with a
        // shorter caption (an unequipped "(Equipped)"), is one size; a caption that needs more still
        // gets it, and the row shrinks back to that size but never below the game's own height.
        std::unordered_map<const CSWGuiText*, int>::iterator it = RowBaseHeights().find(pText);
        if (it == RowBaseHeights().end())
            it = RowBaseHeights().insert(std::make_pair(static_cast<const CSWGuiText*>(pText), extent.h)).first;
        nBase = it->second;
        int nScaled = static_cast<int>(static_cast<float>(nBase) * static_cast<float>(k2qol_dialog_text_scale()) / 100.0f);
        if (nNewH < nScaled)
            nNewH = nScaled;
    } else if (nNewH < extent.h) {
        nNewH = extent.h;   // a label row only grows
    }
    if (nNewH > nRowMax)
        nNewH = nRowMax;
    if (nNewH < nBase)
        nNewH = nBase;
    static const bool s_bTrace = getenv("K2_QOL_TRACE") != 0;   // K2_QOL_TRACE=1: one line a row
    if (s_bTrace)
        fprintf(stderr, "k2: qol: list row %p (list %p, %d rows): row h %d (base %d), caption box %dx%d wants %d -> row h %d (cap %d) \"%s\"\n",
                static_cast<void*>(pRow), static_cast<void*>(pList), nRows, extent.h, nBase, pText->m_extent.w, nBox, nWant, nNewH,
                nRowMax, pText->m_params.m_sText.CStr());
    if (nNewH != extent.h) {
        extent.h = nNewH;
        pRow->SetExtent(extent);   // a marked caption is fitted by WrapString here
    }
    if (!bMark)
        K2QolFitText(*pText, pText->m_extent.h);
    else if (pText->GetIdealHeight() > pText->m_extent.h)
        K2QolFitText(*pText, pText->m_extent.h);
}


// Marks the fixed readout labels: the store's and the crafting screens' (workbench, lab station,
// upgrade bench) credits, cost and stock values, the current-skill box, the slot and property
// names, the chargen and level-up pages' titles, subheadings and point captions (also the console
// save-name keyboard's), the screen and tab titles, and the stat, skill and party readouts. They
// are marked like button captions when CSWGuiPanel::InitControl binds them, before its own
// SetExtent lays them out: they hold short text in a box nothing resizes, so fitting them to that
// box is safe. Labels whose screens re-size them to their text are not marked, since a marked label
// would shrink to its old box before being re-measured; those are sized at their call sites. The
// list is kReadouts below.
void K2QolMarkLayoutLabel(CSWGuiControl& control, const CExoString& sControlName)
{
    static const char* const kReadouts[] = {
        "LBL_CREDITS", "LBL_CREDITS_VALUE", "LBL_COST", "LBL_COST_VALUE", "LBL_STOCK",
        "LBL_STOCK_VALUE", "LBL_RELATEDSKILL", "LBL_SLOTNAME", "LBL_PROPERTY", "LBL_UPGRADE_COUNT",
        "LBL_ONHAND", "LBL_ONHAND_VALUE",   // the store's "In Inventory" caption and count
        // chargen / level-up headings and point captions, shared by every page of the flow
        "MAIN_TITLE_LBL", "SUB_TITLE_LBL", "REMAINING_SELECTIONS_LBL",
        "STD_REMAINING_SELECTIONS_LBL", "COST_LBL", "SELECTIONS_REMAINING_LBL",
        "STD_SELECTIONS_REMAINING_LBL", "COST_POINTS_LBL",
        "NAME_BOX_LBL",   // the name being typed (chargen name, the console save-name keyboard)
        // screen titles: the in-game menu's tab headers (the selected screen's name at the top)
        // and the screens' own titles: LBL_TITLE (map, journal, options, save/load, ...), Inventory's
        // LBL_INV and Abilities' LBL_ABILITIES
        "LBLH_EQU", "LBLH_INV", "LBLH_CHA", "LBLH_MAP", "LBLH_ABI", "LBLH_JOU", "LBLH_OPT",
        "LBLH_MSG", "LBL_TITLE", "LBL_INV", "LBL_ABILITIES",
        // the in-game menu's party member readout (bottom right): name, classes and their levels
        "LBL_CHARNAME", "LBL_TOP_CLASS1", "LBL_TOP_CLASS2", "LBL_TOP_CLASS1LEVEL",
        "LBL_TOP_CLASS2LEVEL",
        // the area map's area name and selected map note; the save/load screen's selected-save
        // details (LBL_PLANETNAME is also the galaxy map's planet name, fitted to the same box)
        "LBL_Area", "LBL_MapNote", "LBL_PCNAME", "LBL_PLANETNAME", "LBL_AREANAME", "LBL_TIMEPLAYED",
        // the Options pages' slider captions: Sound's four volumes, Graphics' brightness, the
        // mouse / touch sensitivity (Gameplay, Mouse)
        "LBL_MUSIC", "LBL_VO", "LBL_FX", "LBL_MOVIE", "LBL_GAMMA", "LBL_MOUSESEN",
        "LBL_FILTER",   // the "Filter" caption over the category buttons (inventory, abilities, store)
        "LBL_PANELNAME",   // the save/load screen's "Save Game" / "Load Game" header
        // the Character screen's stat readout (the saving-throw captions also on the chargen /
        // level-up summaries); LBL_LIGHT / LBL_DARK are blackened labels, re-laid-out per draw: left out
        "LBL_FORCEMASTERY", "LBL_FORTITUDE", "LBL_REFLEX", "LBL_WILL", "LBL_FORTITUDE_STAT",
        "LBL_REFLEX_STAT", "LBL_WILL_STAT", "LBL_EXPERIENCE", "LBL_EXPERIENCE_STAT", "LBL_NEXT_LEVEL",
        "LBL_NEEDED_XP", "LBL_DEFENSE_STAT", "LBL_FORCE_STAT", "LBL_VITALITY_STAT", "LBL_DEFENSE",
        "LBL_FORCE", "LBL_VITALITY", "LBL_STR", "LBL_STRENGTH", "LBL_STR_MOD", "LBL_WIS", "LBL_WISDOM",
        "LBL_WIS_MOD", "LBL_CHA", "LBL_CHARISMA", "LBL_CHA_MOD", "LBL_INT", "LBL_INTELLIGENCE",
        "LBL_INT_MOD", "LBL_CON", "LBL_CONSTITUTION", "LBL_CON_MOD", "LBL_DEX", "LBL_DEXTERITY",
        "LBL_DEX_MOD",
        // the Abilities screen's skill rows
        "LBL_SKILLRANK", "LBL_RANKVAL", "LBL_BONUS", "LBL_BONUSVAL", "LBL_TOTAL", "LBL_TOTALVAL",
        // Party Selection: the available-slots count and caption, the NPC's name and class / level
        "LBL_COUNT", "LBL_AVAILABLE", "LBL_NPC_NAME", "LBL_NPC_LEVEL",
        // the chargen / level-up summary: class, level, the six attributes and their modifiers,
        // vitality / defense captions and values, the new saving throws (LBL_NAME is marked in those
        // two ctors: the HUD's target banner binds the same tag and sizes itself)
        "LBL_CLASS", "LBL_LEVEL_VAL", "STR_LBL", "DEX_LBL", "CON_LBL", "INT_LBL", "WIS_LBL", "CHA_LBL",
        "STR_AB_LBL", "DEX_AB_LBL", "CON_AB_LBL", "INT_AB_LBL", "WIS_AB_LBL", "CHA_AB_LBL", "LBL_VIT",
        "LBL_DEF", "LBL_VIT_NAME", "LBL_DEF_NAME", "NEW_WILL_LBL", "NEW_REFL_LBL", "NEW_FORT_LBL",
    };
    if (!control.AsLabel() || !sControlName.m_sString)
        return;
    for (unsigned i = 0; i < sizeof kReadouts / sizeof kReadouts[0]; i++) {
        if (!strcmp(sControlName.m_sString, kReadouts[i])) {
            K2QolMarkFitText(&static_cast<CSWGuiLabel*>(control.AsLabel())->m_text);
            return;
        }
    }
}

// An item row's stack count (inventory, container, equip and store rows) sits in the corner of the
// row's icon, in a box laid out for the game's 36-unit icon. When the row has grown, taking the icon
// with it, the box grows by the same factor, keeping its bottom-right corner, and the count is marked
// so WrapString fits it to that box at the text size.
void K2QolGrowItemCount(CSWGuiText& text, CSWGuiExtent& extent, float fGrow)
{
    K2QolMarkFitText(&text);
    if (fGrow <= 1.0f)
        return;
    int nRight = extent.x + extent.w;
    int nBottom = extent.y + extent.h;
    extent.w = static_cast<int>(static_cast<float>(extent.w) * fGrow);
    extent.h = static_cast<int>(static_cast<float>(extent.h) * fGrow);
    extent.x = nRight - extent.w;
    extent.y = nBottom - extent.h;
}

// A floaty text (the damage and healing numbers, "Experience", "Level up" over a creature) is laid
// out in a 200-wide box one line of its font tall, its bottom 32 above the creature's reticle point
// (CSWGuiFloatyText::CreateEntry). The box grows with the setting, keeping its centre and its bottom
// (the older lines of the same creature step up by the box's height), and the text is marked so
// WrapString fits it to that box.
void K2QolGrowFloatyText(CSWGuiText& text, CSWGuiExtent& extent)
{
    K2QolMarkFitText(&text);
    int nScale = k2qol_dialog_text_scale();
    if (nScale <= 100)
        return;
    int nW = extent.w * nScale / 100;
    int nH = (extent.h * nScale + 99) / 100 + 2;   // the scaled line, its leading rounded up
    extent.x -= (nW - extent.w) / 2;
    extent.y -= nH - extent.h;
    extent.w = nW;
    extent.h = nH;
}

// The status summary ("Journal entry added", credits, XP, items received or lost, influence, ...)
// lays its own rows out in CSWGuiStatusSummary::OnPanelAdded from fixed one-line sizes (a 32-unit
// icon, a 31-unit description, a 37-unit row step) and widens a description until it is one line of
// the game's font, so a larger text has no room. Above 100 % the visible rows are laid out again
// after that pass: icons and row gaps by the setting, each description at the setting and as wide as
// it needs (up to most of the screen) beside its icon, the panel and its OK button around them. The
// description column moves right with the icons, so the layout's own x is put back first thing on
// every show (K2QolResetStatusSummary) and the game's pass never sees a moved one.
namespace {
int s_aStatusValueX[13], s_aStatusDescX[13];
int s_nStatusOkW = 0, s_nStatusOkH = 0;   // the OK button's own size, which the layout scales
bool s_bStatusLayoutKnown = false;
}  // namespace

void K2QolResetStatusSummary(CSWGuiStatusSummary& summary)
{
    for (int i = 0; i < 13; i++) {
        CSWGuiLabel& desc = summary.m_aDescLabels[i];
        if (!s_bStatusLayoutKnown) {
            s_aStatusValueX[i] = summary.m_aValueLabels[i].m_extent.x;
            s_aStatusDescX[i] = desc.m_extent.x;
        } else if (desc.m_extent.x != s_aStatusDescX[i]) {
            CSWGuiExtent extent = desc.m_extent;
            extent.x = s_aStatusDescX[i];
            desc.SetExtent(extent);
        }
    // The game's own pass measures at the game's own size; the layout below applies the setting.
        CAurGUIStringInternal* pString = desc.m_text.m_pString;
        if (pString) {
            pString->UpdateFontScale();
            pString->K2QolSetSizeFactor(pString->K2QolGetResolutionScale());
            pString->WrapStrings(-1);
        }
    }
    CSWGuiButton& ok = summary.m_btnAck;
    if (!s_bStatusLayoutKnown) {
        s_nStatusOkW = ok.m_extent.w;
        s_nStatusOkH = ok.m_extent.h;
    } else if (ok.m_extent.w != s_nStatusOkW || ok.m_extent.h != s_nStatusOkH) {
        CSWGuiExtent extent = ok.m_extent;
        extent.w = s_nStatusOkW;
        extent.h = s_nStatusOkH;
        ok.SetExtent(extent);
    }
    s_bStatusLayoutKnown = true;
}

void K2QolLayoutStatusSummary(CSWGuiStatusSummary& summary)
{
    const int nScale = k2qol_dialog_text_scale();
    if (nScale <= 100 || !s_bStatusLayoutKnown)
        return;
    const float fRes = GetResolutionTextScalar();
    const float fGrow = static_cast<float>(nScale) / 100.0f;
    const int nIconBase = static_cast<int>(fRes * 32.0f);
    const int nIcon = static_cast<int>(static_cast<float>(nIconBase) * fGrow);
    const int nGap = static_cast<int>(fRes * 5.0f * fGrow);
#ifdef K2_FIX_ENGINE_BUGS
    const int nMargin = static_cast<int>(fRes * 1.5f * 20.0f);
#else
    const int nMargin = static_cast<int>(fRes * 1.5f * 20.0f * fRes);
#endif
    const int nScreenW = summary.m_pGuiManager->m_nScreenWidth;
    int nRowY = static_cast<int>(fRes * 1.8f * 10.0f * fGrow);
    int nRight = 0;
    for (int i = 0; i < 13; i++) {
        CSWGuiLabel& value = summary.m_aValueLabels[i];
        CSWGuiLabel& desc = summary.m_aDescLabels[i];
        if (!value.m_tail.bVisible || !desc.m_text.m_pString)
            continue;
        const int nDescX = s_aStatusValueX[i] + nIcon + (s_aStatusDescX[i] - s_aStatusValueX[i] - nIconBase);
        int nAvail = nScreenW * 9 / 10 - nMargin - nDescX;
        if (nAvail < nIconBase * 4)
            nAvail = nIconBase * 4;
        // The text at the setting, wrapped at the room there is, then as narrow as keeps its lines.
        CSWGuiExtent extent(nDescX, nRowY, nAvail, 0x7fff);
        desc.SetExtent(extent);
        K2QolFitText(desc.m_text, 0x7fffffff);
        const int nSavedWrap = desc.m_text.m_nWrapWidth;
        desc.m_text.m_nWrapWidth = static_cast<int>(static_cast<float>(nAvail) / fRes);
        CSWGuiExtent ideal = desc.m_text.GetIdealWidthAndHeight();
        desc.m_text.m_nWrapWidth = nSavedWrap;
        if (ideal.w > nAvail)
            ideal.w = nAvail;
        const int nRowH = ideal.h > nIcon ? ideal.h : nIcon;
        extent = CSWGuiExtent(s_aStatusValueX[i], nRowY + (nRowH - nIcon) / 2, nIcon, nIcon);
        value.SetExtent(extent);
        extent = CSWGuiExtent(nDescX, nRowY + (nRowH - ideal.h) / 2, ideal.w, ideal.h);
        desc.SetExtent(extent);
        if (nDescX + ideal.w > nRight)
            nRight = nDescX + ideal.w;
        nRowY += nRowH + nGap;
    }
    if (nRight == 0)
        return;
    // The OK button grows with the setting (its caption fits itself to the button), from its own size
    // so a repeated layout does not compound.
    CSWGuiButton& ok = summary.m_btnAck;
    {
        CSWGuiExtent extent = ok.m_extent;
        extent.w = static_cast<int>(static_cast<float>(s_nStatusOkW) * fGrow);
        extent.h = static_cast<int>(static_cast<float>(s_nStatusOkH) * fGrow);
        ok.SetExtent(extent);
    }
    CSWGuiExtent panel;
    panel.w = nRight + nMargin;
    if (panel.w < ok.m_extent.w + nMargin * 2)
        panel.w = ok.m_extent.w + nMargin * 2;
    panel.h = nRowY + ok.m_extent.h + 0x14;
    panel.x = (nScreenW - panel.w) / 2;
    panel.y = (summary.m_pGuiManager->m_nScreenHeight - panel.h) / 2;
    summary.SetExtent(panel);
    CSWGuiExtent button = ok.m_extent;
    button.x = (panel.w - button.w) / 2;
    button.y = nRowY;
    ok.SetExtent(button);
}

// The area-exit banner (the running-man icon over the destination's name) is a fixed 400 x 82 unit
// panel: its text plate, the name inside it and the icon below. It is laid out again in the same shape
// scaled by the setting, as far as the screen's width allows, whenever it is pointed at a new exit,
// and the name fits itself to its box. At the game's own size this is the constructor's own layout, so
// a setting turned back down puts the banner back.
void K2QolLayoutAreaTransition(CSWGuiInGameAreaTransition& banner)
{
    K2QolMarkFitText(&banner.m_lblDescription.m_text);
    const float fRes = GetResolutionTextScalar();
    const int nScreenW = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenWidth;
    float fGrow = static_cast<float>(k2qol_dialog_text_scale()) / 100.0f;
    const float fFit = static_cast<float>(nScreenW) * 0.95f / (fRes * 400.0f);
    if (fGrow > fFit)
        fGrow = fFit;
    if (fGrow < 1.0f)
        fGrow = 1.0f;
    const float f = fRes * fGrow;
    CSWGuiExtent extent = banner.m_extent;
    extent.w = static_cast<int>(f * 400.0f);
    extent.h = static_cast<int>(f * 82.0f);
    const int nHalfWidth = extent.w / 2;
    extent.x = nScreenW / 2 - nHalfWidth;
    banner.SetExtent(extent);
    const int nIcon = static_cast<int>(f * 50.0f);
    banner.m_lblIcon.SetExtent(CSWGuiExtent(nHalfWidth - nIcon / 2, static_cast<int>(f * 27.0f), nIcon, nIcon));
    const int nDescW = static_cast<int>(f * 348.0f);
    banner.m_lblDescription.SetExtent(
        CSWGuiExtent(nHalfWidth - nDescW / 2, static_cast<int>(f * 6.0f), nDescW, static_cast<int>(f * 20.0f)));
    banner.m_lblTextBG.SetExtent(CSWGuiExtent(0, 0, static_cast<int>(f * 400.0f), static_cast<int>(f * 32.0f)));
}

// A feat or force power row (CSWGuiSkillFlow: the chargen and level-up charts) is three icons with an
// arrow between each pair, laid out across a fixed width (the xbox layout's list is exactly that
// wide). Its height takes the setting, as far as the three icons and two arrows (32/44 of an icon
// each) still fit side by side.
int K2QolSkillFlowHeight(int nWidth, int nHeight)
{
    int nWant = nHeight * k2qol_dialog_text_scale() / 100;
    int nMax = static_cast<int>(static_cast<float>(nWidth) / (3.0f + 2.0f * 32.0f / 44.0f));
    if (nWant > nMax)
        nWant = nMax;
    return nWant > nHeight ? nWant : nHeight;
}

// Returns the tallest row a list box will draw. CSWGuiListBox::Draw draws a row only while it lies
// entirely inside the inner rect (bClipPartialRows), below the padding. `nSlack` is room the caller
// knows the box will lose afterwards (the conversation trims LB_REPLIES to whole lines).
int K2QolListRowLimit(const CSWGuiListBox& listBox, int nSlack)
{
    return listBox.m_innerExtent.h - listBox.m_nPadding * 2 - 1 - nSlack;
}

#endif  // K2_QOL_IMPROVEMENTS

#endif  // K2_QOL_CONTROLLER
