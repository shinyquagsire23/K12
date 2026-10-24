// CSWGuiInGameGameplay, the "Gameplay" options page.

#include "k2stdint.h"
#include "port/sdlapi.h"
#include "swguiingamegameplay.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "clientoptions.h"
#include "guiingame.h"
#include "swguicontrol.h"
#include "swgui.h"
#include "swguidrawable.h"
#include "SwGuiInOptKeyMapping.h"
#include "swguioptions.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "twodimarray.h"

// Adds a controller glyph to a button, restyling it for the current controller scheme.
void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);

// Widens BTN_DEFAULT until its caption fits.
void ExpandButtonToFitText(CSWGuiButton& btn);

float GetResolutionTextScalar();
#if KOTOR1
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);
bool HasTouchScreen();
#endif

#include "port/libcstring.h"
#include "port/k2qolapi.h"
#include "exoinput.h"
#include "strref.h"

// Loads "optgameplay_p", binds the 19 widgets the layout carries, wires every handler, and hands
// the difficulty and every checkbox over to SetUpOptions().
CSWGuiInGameGameplay::CSWGuiInGameGameplay(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1
    // K1: layout "optgameplay", BTN_KEYMAP and BTN_MOUSE bound from the layout, K1 help strrefs and
    // pad navigation order, the mouse-sensitivity row pushed down, and without a touch screen the
    // autorun and mouse-sensitivity rows are hidden.
    StartLoadFromLayout(CResRef("optgameplay"), 1, 1, 1);

    InitControl(m_btnDifficulty,  CExoString("BTN_DIFFICULTY"), 1, 1, 0, 0, 0);
    InitControl(m_btnDiffRight,   CExoString("BTN_DIFFRIGHT"),  1, 1, 0, 0, 0);
    InitControl(m_btnDiffLeft,    CExoString("BTN_DIFFLEFT"),   1, 1, 0, 0, 0);
    InitControl(m_cbInvertCam,    CExoString("CB_INVERTCAM"),   1, 1, 0, 0, 0);
    InitControl(m_cbLevelUp,      CExoString("CB_LEVELUP"),     1, 1, 0, 0, 0);
    InitControl(m_cbAutoSave,     CExoString("CB_AUTOSAVE"),    1, 1, 0, 0, 0);
    InitControl(m_cbReverse,      CExoString("CB_REVERSE"),     1, 1, 0, 0, 0);
    InitControl(m_cbDisableMove,  CExoString("CB_DISABLEMOVE"), 1, 1, 0, 0, 0);
    InitControl(m_cbAutoRun,      CExoString("CB_AUTORUN"),     1, 1, 0, 0, 0);
    InitControl(m_sliderMouseSen, CExoString("SLI_MOUSESEN"),   1, 1, 0, 0, 0);
    InitControl(m_labelMouseSen,  CExoString("LBL_MOUSESEN"),   1, 1, 0, 0, 0);
    InitControl(m_labelTitle,     CExoString("LBL_TITLE"),      1, 1, 0, 0, 0);
    InitControl(m_listBoxDesc,    CExoString("LB_DESC"),        1, 1, 0, 0, 0);
    InitControl(m_btnBack,        CExoString("BTN_BACK"),       1, 1, 0, 0, 0);
    InitControl(m_btnDefault,     CExoString("BTN_DEFAULT"),    1, 1, 0, 0, 0);
    InitControl(m_btnKeyMap,      CExoString("BTN_KEYMAP"),     1, 1, 0, 0, 0);
    InitControl(m_btnMouse,       CExoString("BTN_MOUSE"),      1, 1, 0, 0, 0);

    m_listBoxDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    StopLoadFromLayout();
    ScaleExtentForResolution();

    m_labelDesc.Initialize(m_listBoxDesc.m_pProtoItem->m_extent,
                           static_cast<CSWGuiLabel*>(m_listBoxDesc.m_pProtoItem), 1.0f);

    m_btnDifficulty.AddEvent(INPUTEVENT_GUI_DPAD_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDifficultyLeft));
    m_btnDifficulty.AddEvent(INPUTEVENT_GUI_KEY_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDifficultyLeft));
    m_btnDifficulty.AddEvent(INPUTEVENT_GUI_DPAD_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDifficultyRight));
    m_btnDifficulty.AddEvent(INPUTEVENT_GUI_KEY_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDifficultyRight));
    m_btnDifficulty.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnEnterDiff));
    m_btnDifficulty.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnExitDiff));

    m_btnDiffLeft.m_tail.bFocusOnHover = 0;
    m_btnDiffLeft.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDifficultyLeft));
    m_btnDiffRight.m_tail.bFocusOnHover = 0;
    m_btnDiffRight.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDifficultyRight));

    m_cbLevelUp.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnAutoLevelUp));
    m_cbLevelUp.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnAutoLevelUp));
    m_cbLevelUp.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnEnterSlot));
    m_cbInvertCam.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnInvertCam));
    m_cbInvertCam.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnInvertCam));
    m_cbInvertCam.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnEnterSlot));
    m_cbAutoSave.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnAutoSave));
    m_cbAutoSave.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnAutoSave));
    m_cbAutoSave.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnEnterSlot));
    m_cbReverse.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnReverse));
    m_cbReverse.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnReverse));
    m_cbReverse.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnEnterSlot));
    m_cbDisableMove.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnCombatMovement));
    m_cbDisableMove.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnCombatMovement));
    m_cbDisableMove.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnEnterSlot));
    m_cbAutoRun.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnAutoRun));
    m_cbAutoRun.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnAutoRun));
    m_cbAutoRun.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnEnterSlot));

    m_btnKeyMap.m_tail.bFocusOnHover = 0;
    m_btnKeyMap.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnKeyMap));
    m_btnMouse.m_tail.bFocusOnHover = 0;
    m_btnMouse.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnMouse));
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnDefault.m_tail.bFocusOnHover = 0;
    m_btnDefault.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDefault));

    m_cbLevelUp.m_tail.m_nUserData     = STRREF_DESC_AUTO_LEVEL_UP;
    m_cbAutoSave.m_tail.m_nUserData    = STRREF_DESC_AUTOSAVE;
    m_cbInvertCam.m_tail.m_nUserData   = STRREF_DESC_MOUSE_LOOK;
    m_cbReverse.m_tail.m_nUserData     = STRREF_DESC_REVERSE_UP_KEY;
    m_cbAutoRun.m_tail.m_nUserData     = STRREF_DESC_MOUSE_LOOK;
    m_cbDisableMove.m_tail.m_nUserData = STRREF_DESC_COMBAT_MOVEMENT;
    m_btnMouse.m_tail.m_nUserData      = STRREF_DESC_MOUSE_SETTINGS;
    m_btnKeyMap.m_tail.m_nUserData     = STRREF_DESC_KEY_MAPPINGS;

    SetActiveControl(&m_btnDifficulty, 0);
    SetUpOptions();

    m_sliderMouseSen.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnMoveMouseSenSlider));
    m_sliderMouseSen.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnEnterSlider));
    m_sliderMouseSen.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnExitSlider));
    m_sliderMouseSen.AddEvent(INPUTEVENT_GUI_DPAD_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnMoveMouseSenSlider));
    m_sliderMouseSen.AddEvent(INPUTEVENT_GUI_DPAD_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnMoveMouseSenSlider));
    m_sliderMouseSen.AddEvent(INPUTEVENT_GUI_KEY_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnMoveMouseSenSlider));
    m_sliderMouseSen.AddEvent(INPUTEVENT_GUI_KEY_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnMoveMouseSenSlider));

    m_sliderMouseSen.m_tail.m_nUserData = STRREF_DESC_MOUSE_SENSITIVITY;
    m_labelMouseSen.m_tail.m_nUserData = STRREF_DESC_MOUSE_SENSITIVITY;

#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
    CSWGuiExtent extent = m_sliderMouseSen.m_extent;
    extent.y = static_cast<int>(s_fScale * 330.0f + static_cast<float>(extent.y));
    m_sliderMouseSen.SetExtent(extent);
    extent = m_labelMouseSen.m_extent;
    extent.y = static_cast<int>(s_fScale * 330.0f + static_cast<float>(extent.y));
    m_labelMouseSen.SetExtent(extent);

    m_btnDifficulty.SetMoveToControl(0, &m_sliderMouseSen);
    m_btnDifficulty.SetMoveToControl(2, &m_cbLevelUp);
    m_cbLevelUp.SetMoveToControl(0, &m_btnDifficulty);
    m_cbLevelUp.SetMoveToControl(2, &m_cbAutoSave);
    m_cbAutoSave.SetMoveToControl(0, &m_cbLevelUp);
    m_cbAutoSave.SetMoveToControl(2, &m_cbReverse);
    m_cbReverse.SetMoveToControl(0, &m_cbAutoSave);
    m_cbReverse.SetMoveToControl(2, &m_cbDisableMove);
    m_cbDisableMove.SetMoveToControl(0, &m_cbReverse);
    m_cbDisableMove.SetMoveToControl(2, &m_cbAutoRun);
    m_cbAutoRun.SetMoveToControl(0, &m_cbDisableMove);
    m_cbAutoRun.SetMoveToControl(2, &m_sliderMouseSen);
    m_sliderMouseSen.SetMoveToControl(0, &m_cbAutoRun);
    m_sliderMouseSen.SetMoveToControl(2, &m_btnDifficulty);

    if (!HasTouchScreen()) {
        m_btnDifficulty.SetMoveToControl(0, &m_cbDisableMove);
        m_cbDisableMove.SetMoveToControl(2, &m_btnDifficulty);
        m_cbReverse.m_tail.m_nUserData = STRREF_DESC_REVERSE_THUMBSTICK;
        m_cbAutoRun.m_tail.bVisible = 0;
        m_sliderMouseSen.m_tail.bVisible = 0;
        m_labelMouseSen.m_tail.bVisible = 0;
    }

    m_btnDifficulty.AddEvent(INPUTEVENT_GUI_Y, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDefault));
    m_cbLevelUp.AddEvent(INPUTEVENT_GUI_Y, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDefault));
    m_cbAutoSave.AddEvent(INPUTEVENT_GUI_Y, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDefault));
    m_cbReverse.AddEvent(INPUTEVENT_GUI_Y, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDefault));
    m_cbDisableMove.AddEvent(INPUTEVENT_GUI_Y, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDefault));
    m_cbAutoRun.AddEvent(INPUTEVENT_GUI_Y, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDefault));
    m_sliderMouseSen.AddEvent(INPUTEVENT_GUI_Y, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDefault));

    IosAddButtonIcon(&m_btnDefault, 'y');
    IosAddButtonIcon(&m_btnBack, 'b');
#else
    StartLoadFromLayout(CResRef("optgameplay_p"), 1, 1, 1);

    InitControl(m_btnDifficulty,   CExoString("BTN_DIFFICULTY"),    1, 1, 0, 0, 0);
    InitControl(m_cbLevelUp,       CExoString("CB_LEVELUP"),        1, 1, 0, 0, 0);
    InitControl(m_cbAutoSave,      CExoString("CB_AUTOSAVE"),       1, 1, 0, 0, 0);
    InitControl(m_cbReverse,       CExoString("CB_REVERSE"),        1, 1, 0, 0, 0);
#if !KOTOR1
    InitControl(m_cbReverseIngame, CExoString("CB_REVERSE_INGAME"), 1, 1, 0, 0, 0);
#endif
    InitControl(m_cbDisableMove,   CExoString("CB_DISABLEMOVE"),    1, 1, 0, 0, 0);
    InitControl(m_labelTitle,      CExoString("LBL_TITLE"),         1, 1, 0, 0, 0);
    InitControl(m_listBoxDesc,     CExoString("LB_DESC"),           1, 1, 0, 0, 0);
    InitControl(m_cbAutoRun,       CExoString("CB_AUTORUN"),        1, 1, 0, 0, 0);
    InitControl(m_cbAutoRun,       CExoString("CB_INVERTCAM"),      1, 1, 0, 0, 0);
    InitControl(m_btnDiffRight,    CExoString("BTN_DIFFRIGHT"),     1, 1, 0, 0, 0);
    InitControl(m_btnDiffLeft,     CExoString("BTN_DIFFLEFT"),      1, 1, 0, 0, 0);
    InitControl(m_sliderMouseSen,  CExoString("SLI_MOUSESEN"),      1, 1, 0, 0, 0);
    InitControl(m_labelMouseSen,   CExoString("LBL_MOUSESEN"),      1, 1, 0, 0, 0);
    InitControl(m_btnBack,         CExoString("BTN_BACK"),          1, 1, 0, 0, 0);
    InitControl(m_btnDefault,      CExoString("BTN_DEFAULT"),       1, 1, 0, 0, 0);
#if !KOTOR1
    InitControl(m_aLabelBar[0],    CExoString("LBL_BAR1"),          1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[1],    CExoString("LBL_BAR2"),          1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[2],    CExoString("LBL_BAR3"),          1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[3],    CExoString("LBL_BAR4"),          1, 1, 0, 0, 0);
#endif

    m_listBoxDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    StopLoadFromLayout();

    ExpandButtonToFitText(m_btnDefault);

    // The description listbox's single row, cloned from the listbox's prototype item and scaled
    // for the current resolution.
    CSWGuiControl* pProtoItem = m_listBoxDesc.m_pProtoItem;
    float fTextScalar = GetResolutionTextScalar();
    CSWGuiExtent extentDesc = pProtoItem->m_extent;
    extentDesc.h = static_cast<int>(fTextScalar * static_cast<float>(extentDesc.h));
    m_labelDesc.Initialize(extentDesc, static_cast<CSWGuiLabel*>(pProtoItem), 1.0f);

    // ---- BTN_DIFFICULTY: the arrow keys step the difficulty, focus shows the description ----
    m_btnDifficulty.AddEvent(INPUTEVENT_GUI_DPAD_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDifficultyLeft));
    m_btnDifficulty.AddEvent(INPUTEVENT_GUI_DPAD_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDifficultyRight));
    m_btnDifficulty.AddEvent(INPUTEVENT_GUI_KEY_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDifficultyLeft));
    m_btnDifficulty.AddEvent(INPUTEVENT_GUI_KEY_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDifficultyRight));
    m_btnDifficulty.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnEnterDiff));
    m_btnDifficulty.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnExitDiff));

    m_btnDiffLeft.m_tail.bFocusOnHover = 0;
    m_btnDiffLeft.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDifficultyLeft));
    m_btnDiffRight.m_tail.bFocusOnHover = 0;
    m_btnDiffRight.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDifficultyRight));

    m_cbLevelUp.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnAutoLevelUp));
    m_cbLevelUp.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnAutoLevelUp));
    m_cbLevelUp.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnEnterSlot));

    m_cbAutoRun.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnAutoRun));
    m_cbAutoRun.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnAutoRun));
    m_cbAutoRun.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnEnterSlot));

    m_cbAutoSave.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnAutoSave));
    m_cbAutoSave.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnAutoSave));
    m_cbAutoSave.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnEnterSlot));

    m_cbReverse.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnReverse));
    m_cbReverse.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnReverse));
    m_cbReverse.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnEnterSlot));

#if !KOTOR1
    m_cbReverseIngame.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnReverseIngame));
    m_cbReverseIngame.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnReverseIngame));
    m_cbReverseIngame.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnEnterSlot));
#endif

    m_cbDisableMove.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnCombatMovement));
    m_cbDisableMove.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnCombatMovement));
    m_cbDisableMove.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnEnterSlot));

    m_btnKeyMap.m_tail.bFocusOnHover = 0;
    m_btnKeyMap.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnKeyMap));
    m_btnMouse.m_tail.bFocusOnHover = 0;
    m_btnMouse.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnMouse));
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnDefault.m_tail.bFocusOnHover = 0;
    m_btnDefault.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnDefault));

    AspyrAddButtonIcon(&m_btnBack, 'b');
    AspyrAddButtonIcon(&m_btnDefault, 'y');

    m_cbLevelUp.m_tail.m_nUserData       = STRREF_DESC_AUTO_LEVEL_UP;
    m_cbAutoSave.m_tail.m_nUserData      = STRREF_DESC_AUTOSAVE;
    m_cbReverse.m_tail.m_nUserData       = STRREF_DESC_REVERSE_UP_KEY;
#if !KOTOR1
    m_cbReverseIngame.m_tail.m_nUserData = STRREF_DESC_REVERSE_MOUSE;
#endif
    m_cbDisableMove.m_tail.m_nUserData   = STRREF_DESC_COMBAT_MOVEMENT;
    m_btnMouse.m_tail.m_nUserData        = STRREF_DESC_MOUSE_SETTINGS;
    m_btnKeyMap.m_tail.m_nUserData       = STRREF_DESC_KEY_MAPPINGS;
    m_cbAutoRun.m_tail.m_nUserData       = STRREF_DESC_MOUSE_LOOK;

    SetActiveControl(&m_btnDifficulty, 0);
    SetUpOptions();

    m_sliderMouseSen.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnMoveMouseSenSlider));
    m_sliderMouseSen.AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnEnterSlider));
    m_sliderMouseSen.AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnExitSlider));
    m_sliderMouseSen.AddEvent(INPUTEVENT_GUI_DPAD_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnMoveMouseSenSlider));
    m_sliderMouseSen.AddEvent(INPUTEVENT_GUI_DPAD_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnMoveMouseSenSlider));
    m_sliderMouseSen.AddEvent(INPUTEVENT_GUI_KEY_LEFT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnMoveMouseSenSlider));
    m_sliderMouseSen.AddEvent(INPUTEVENT_GUI_KEY_RIGHT, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameGameplay::OnMoveMouseSenSlider));

    m_sliderMouseSen.m_tail.m_nUserData = STRREF_DESC_MOUSE_SENSITIVITY;
    m_labelMouseSen.m_tail.m_nUserData = STRREF_DESC_MOUSE_SENSITIVITY;
    m_sliderMouseSen.m_tail.bVisible = 1;
#ifdef K2_QOL_IMPROVEMENTS
    K2QolNarrowSelectorCaption(m_btnDifficulty, m_btnDiffLeft, m_btnDiffRight);
#endif
#endif
}

CSWGuiInGameGameplay::~CSWGuiInGameGameplay()
{
}

// Handles a difficulty arrow: latches the new index into the panel, mirrors it into CClientOptions
// (which the server reads as the DC adjustment) and re-titles the button from difficultyopt.2da's
// name column. The arrows clamp differently: the left against 1, the right against the table's row
// count minus 2.

void CSWGuiInGameGameplay::OnDifficultyLeft(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg && m_nDifficulty >= 1) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        m_nDifficulty--;
        pOptions->m_difficultyLevel = static_cast<unsigned char>(m_nDifficulty);
        SetDifficultyText();
        m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
    }
}

void CSWGuiInGameGameplay::OnDifficultyRight(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg
        && m_nDifficulty != g_pRules->m_p2DArrays->m_pDifficultyOptions->m_nRows - 2) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        m_nDifficulty++;
        pOptions->m_difficultyLevel = static_cast<unsigned char>(m_nDifficulty);
        SetDifficultyText();
        m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
    }
}

// Handles the difficulty button's hover: the caption goes green and its text is highlighted.
void CSWGuiInGameGameplay::OnEnterDiff(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
#if KOTOR1
    m_btnDifficulty.m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
#else
    m_btnDifficulty.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
#endif
    m_btnDifficulty.m_text.m_params.bHilight = 1;
    SetDifficultyText();
    SetDescription(STRREF_DESC_DIFFICULTY);
}

// Restores the default caption colour when the pointer leaves the difficulty button.
void CSWGuiInGameGameplay::OnExitDiff(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
#if KOTOR1
    m_btnDifficulty.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
#else
    m_btnDifficulty.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
#endif
    m_btnDifficulty.m_text.m_params.bHilight = 0;
}

// ---------------------------------------------------------------------------------------------
// The six checkbox rows. Each latches the option to the negation of its own current value (the
// control was already flipped by the click), so the state survives a cancel-and-reopen.

void CSWGuiInGameGameplay::OnAutoLevelUp(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->SetAutoLevelUpNPCs(g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bAutoLevelUpNPCs == 0);
    }
}

void CSWGuiInGameGameplay::OnAutoRun(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_bAutoRun = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bAutoRun == 0;
    }
}

void CSWGuiInGameGameplay::OnAutoSave(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_bAutoSave = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bAutoSave == 0;
    }
}

void CSWGuiInGameGameplay::OnReverse(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_bGameplay3 = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGameplay3 == 0;
    }
}

#if !KOTOR1
void CSWGuiInGameGameplay::OnReverseIngame(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_bGameplay4 = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGameplay4 == 0;
    }
}
#endif

void CSWGuiInGameGameplay::OnCombatMovement(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_bGameplay5 = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGameplay5 == 0;
    }
}

// The two sub-panel buttons and BTN_DEFAULT.
void CSWGuiInGameGameplay::OnKeyMap(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CSWGuiInGameOptKeyMappings* pKeyMap = new CSWGuiInGameOptKeyMappings(m_pGuiManager);
        m_pGuiManager->AddPanel(pKeyMap, 3, 1);
    }
}

void CSWGuiInGameGameplay::OnMouse(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CSWGuiOptionsMouse* pMouseOpt = new CSWGuiOptionsMouse(m_pGuiManager);
        m_pGuiManager->AddPanel(pMouseOpt, 3, 1);
    }
}

void CSWGuiInGameGameplay::OnDefault(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultGameplayOptions();
#if KOTOR1  // K1 resets every mouse option, not only the sensitivity
        g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultMouseOptions();
#else
        g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultMouseSen();
#endif
        SetUpOptions();
    }
}

// Re-syncs every widget from CClientOptions. The panel's difficulty is written first and re-titled
// before any row is touched, and every row's toggle comes straight from one bit of the options
// record.

void CSWGuiInGameGameplay::SetUpOptions()
{
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions == 0)
        return;

    m_nDifficulty = pOptions->m_difficultyLevel;
    SetDifficultyText();

#if KOTOR1
    m_cbLevelUp.bToggledOn       = pOptions->m_bAutoLevelUpNPCs;
    m_cbInvertCam.bToggledOn     = pOptions->m_bGameplay1;
    m_cbAutoSave.bToggledOn      = pOptions->m_bAutoSave;
    m_cbReverse.bToggledOn       = pOptions->m_bGameplay3;
    m_cbDisableMove.bToggledOn   = pOptions->m_bGameplay5;
    m_cbAutoRun.bToggledOn       = pOptions->m_bAutoRun;
#else
    m_cbLevelUp.bToggledOn       = pOptions->m_bAutoLevelUpNPCs;
    m_cbAutoRun.bToggledOn       = pOptions->m_bGameplay1;
    m_cbAutoSave.bToggledOn      = pOptions->m_bAutoSave;
    m_cbReverse.bToggledOn       = pOptions->m_bGameplay3;
    m_cbReverseIngame.bToggledOn = pOptions->m_bGameplay4;
    m_cbDisableMove.bToggledOn   = pOptions->m_bGameplay5;
    m_cbAutoRun.bToggledOn       = pOptions->m_bAutoRun;
#endif
    m_sliderMouseSen.SetCurValue(pOptions->m_nMouseSensitivity);
}

// Handles a mouse-sensitivity slider move. The slider is queried twice, as the engine does: the
// first AsSlider() result is dead.
void CSWGuiInGameGameplay::OnMoveMouseSenSlider(CSWGuiControl* pControl)
{
    CSWGuiSlider* pSlider = static_cast<CSWGuiSlider*>(pControl->AsSlider());
    int nCurValue = static_cast<CSWGuiSlider*>(pControl->AsSlider())->m_nCurValue;
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    pOptions->m_nMouseSensitivity = static_cast<unsigned char>(nCurValue);
    (void)pSlider;
}

void CSWGuiInGameGameplay::OnEnterSlider(CSWGuiControl* pControl)
{
    if (&m_sliderMouseSen == pControl)
        SetDescription(STRREF_DESC_MOUSE_SENSITIVITY);
    static_cast<CSWGuiSlider*>(pControl->AsSlider())->m_imageThumb.m_params.bHilight = 1;
}

void CSWGuiInGameGameplay::OnExitSlider(CSWGuiControl* pControl)
{
    static_cast<CSWGuiSlider*>(pControl->AsSlider())->m_imageThumb.m_params.bHilight = 0;
}

// Sets the description listbox: retargets the shared label, measures it and hands it to the listbox
// as its single row. The height is scaled by the resolution scalar after the measure, and the
// measure goes through a pointer so it stays a virtual dispatch (see
// CSWGuiOptionsGraphics::SetDescription).
void CSWGuiInGameGameplay::SetDescription(uint64_t nDescription)
{
    m_labelDesc.m_text.m_params.SetStrRef(nDescription);

    CExoString sText(m_labelDesc.m_text.m_params.m_sText);
    m_listBoxDesc.ClearItems();

    CSWGuiText* pText = &m_labelDesc.m_text;
    CSWGuiExtent extent = m_labelDesc.m_extent;
    // Reads m_sString directly rather than going through CStr().
#if KOTOR1
    if (sText.GetLength() >= 1)
#else
    if (sText.m_sString && static_cast<int>(strlen(sText.m_sString)) >= 1)
#endif
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
#if !KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // K1 does not scale the height by the resolution text scalar
    extent.h = static_cast<int>(GetResolutionTextScalar() * static_cast<float>(extent.h));
#endif
    m_labelDesc.SetExtent(extent);

    CSWGuiControl* apRows[1] = { &m_labelDesc };
    m_listBoxDesc.AddControls(apRows, 1, 0, 0, 0);
    m_listBoxDesc.SetSelectedControl(&m_labelDesc, 0);
}

// In the input handler the jump table covers the A, B, X, Y and arrow codes, with two of them
// sharing the pop-modal arm, and the whole switch guarded on a nonzero value.
//
void CSWGuiInGameGameplay::HandleInputEvent(int type, int val)
{
    if (val != 0) {
        switch (type) {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK: {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_pGuiManager->PopModalPanel();
#if KOTOR1
            // K1: no panel chain; one remove-and-delete stamp.
            m_stateFlags = (m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE_AND_DELETE;
#else
            CSWGuiPanel* pPanel = this;
            do {
                pPanel->m_stateFlags = (pPanel->m_stateFlags & ~(PANEL_REMOVE_MASK | PANEL_REMOVE_DEFERRED)) | PANEL_REMOVE_AND_DELETE;
                pPanel = pPanel->m_pNextInChain;
            } while (pPanel);
#endif
            break;
        }
#if !KOTOR1  // K1 has no 0x2a (restore-defaults) arm
        case INPUTEVENT_GUI_Y:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_btnDefault.m_tail.nCurrentEventArg = 1;
            g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultGameplayOptions();
            g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultMouseSen();
            SetUpOptions();
            break;
#endif
        case INPUTEVENT_GUI_START: {
            // The triple AsButtonToggle is a may-alias idiom: the dispatch cannot be commoned, and
            // the source really does issue all three calls.
            if (m_pActiveControl && m_pActiveControl->AsButtonToggle()) {
                m_pGuiManager->PlayGuiSound(GUISOUND_CHECKBOX_CHECK);
                CSWGuiOptionsCheckbox* pCheckA =
                    static_cast<CSWGuiOptionsCheckbox*>(m_pActiveControl->AsButtonToggle());
                CSWGuiOptionsCheckbox* pCheckB =
                    static_cast<CSWGuiOptionsCheckbox*>(m_pActiveControl->AsButtonToggle());
                pCheckA->bToggledOn = !pCheckB->bToggledOn;
            }
            break;
        }
        case INPUTEVENT_GUI_SCROLL_UP:
            m_listBoxDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
            break;
        case INPUTEVENT_GUI_SCROLL_DOWN:
            m_listBoxDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
            break;
        default:
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

void CSWGuiInGameGameplay::OnPanelAdded()
{
    CSWGuiPanel::OnPanelAdded();
}

void CSWGuiInGameGameplay::OnInvertCam(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg) {
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        pOptions->m_bGameplay1 = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGameplay1 == 0;
    }
}

// Reads the "Auto Run" gameplay option, bit 0 of CClientOptions' 16-bit flag word.
bool SWGuiGetAutoRun()
{
    return g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bAutoRun;
}

// Re-titles BTN_DIFFICULTY from difficultyopt.2da's name column, then shows or hides the two
// arrows: the left one disappears at the bottom of the range, the right one at the top.

void CSWGuiInGameGameplay::SetDifficultyText()
{
    int nStrRef;
    if (g_pRules->m_p2DArrays->m_pDifficultyOptions->GetINTEntry(m_nDifficulty, CExoString("NAME"), &nStrRef))
        m_btnDifficulty.m_text.m_params.SetStrRef(nStrRef);

    m_btnDiffLeft.m_tail.bVisible = 1;
    m_btnDiffRight.m_tail.bVisible = 1;
    if (m_nDifficulty == 0)
        m_btnDiffLeft.m_tail.bVisible = 0;
    else if (m_nDifficulty == g_pRules->m_p2DArrays->m_pDifficultyOptions->m_nRows - 2)
        m_btnDiffRight.m_tail.bVisible = 0;
}

void CSWGuiInGameGameplay::OnEnterSlot(CSWGuiControl* pControl) {
    SetDescription(pControl->m_tail.m_nUserData);
}
