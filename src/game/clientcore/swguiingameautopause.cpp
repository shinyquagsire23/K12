#include "k2stdint.h"
#include "swguiingameautopause.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "clientoptions.h"
#include "swgui.h"
#include "swguicontrol.h"
#include "swguidrawable.h"

#include "port/libcstring.h"
#include "exoinput.h"
#include "strref.h"
#include "exostringinline.h"

void ExpandButtonToFitText(CSWGuiButton& button);
void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#if KOTOR1
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#endif

// Auto-pause never takes the mouse away from an added panel.
#if !KOTOR1
bool CSWGuiInGameAutoPause::ShouldFreeMouseOnAdd() {
    return 0;
}
#endif

CSWGuiInGameAutoPause::~CSWGuiInGameAutoPause()
{
}

void CSWGuiInGameAutoPause::OnEnterSlot(CSWGuiControl* pControl) {
    SetDescription(pControl->m_tail.m_nUserData);
}

// Resets the auto-pause options to their shipped defaults, then re-syncs every row.
void CSWGuiInGameAutoPause::OnDefault(CSWGuiControl* pControl) {
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultAutopauseOptions();
    SetupOptions();
}

// The (CSWGuiControl*) cast on each receiver is required.
void CSWGuiInGameAutoPause::SetupOptions() {
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions == 0)
        return;

    CSWGuiOptionsCheckbox* pRow;
    pRow = static_cast<CSWGuiOptionsCheckbox*>(static_cast<CSWGuiControl*>(&m_aCheckbox[kCbEndRound])->AsButtonToggle());
    pRow->bToggledOn = pOptions->m_bAutoPauseEndOfCombatRound;
    pRow = static_cast<CSWGuiOptionsCheckbox*>(static_cast<CSWGuiControl*>(&m_aCheckbox[kCbEnemySighted])->AsButtonToggle());
    pRow->bToggledOn = pOptions->m_bAutoPauseEnemySighted;
    pRow = static_cast<CSWGuiOptionsCheckbox*>(static_cast<CSWGuiControl*>(&m_aCheckbox[kCbMineSighted])->AsButtonToggle());
    pRow->bToggledOn = pOptions->m_bAutoPauseMineSighted;
    pRow = static_cast<CSWGuiOptionsCheckbox*>(static_cast<CSWGuiControl*>(&m_aCheckbox[kCbPartyKilled])->AsButtonToggle());
    pRow->bToggledOn = pOptions->m_bAutoPausePartyKilled;
    pRow = static_cast<CSWGuiOptionsCheckbox*>(static_cast<CSWGuiControl*>(&m_aCheckbox[kCbActionMenu])->AsButtonToggle());
    pRow->bToggledOn = pOptions->m_bAutoPauseActionMenu;
    pRow = static_cast<CSWGuiOptionsCheckbox*>(static_cast<CSWGuiControl*>(&m_aCheckbox[kCbTriggers])->AsButtonToggle());
    pRow->bToggledOn = pOptions->m_bAutoPauseNewTarget;
}

void CSWGuiInGameAutoPause::ClosePanel() {
    SaveOptions();
    m_pGuiManager->PopModalPanel();

    SetToBeDeleted(0);
}

// Retargets the description listbox's single row, which is m_labelDesc.
void CSWGuiInGameAutoPause::SetDescription(uint64_t nStrRef) {
    m_labelDesc.m_text.m_params.SetStrRef(nStrRef);
    CExoString str = m_labelDesc.m_text.m_params.m_sText;
    m_listBoxDetails.ClearItems();
    CSWGuiText* pText = &m_labelDesc.m_text;
    CSWGuiExtent extent = m_labelDesc.m_extent;
    if (str.GetLength() > 0)
        extent.h = pText->GetIdealHeight();
    else
        extent.h = m_labelDesc.m_text.GetFontHeight();
    m_labelDesc.SetExtent(extent);
    // The one-element array must stay declared at the call site.
    CSWGuiControl* apRows[1] = { &m_labelDesc };
    m_listBoxDetails.AddControls(apRows, 1, 0, 0, 0);
    m_listBoxDetails.SetSelectedControl(&m_labelDesc, 0);
}

// SetupOptions' inverse: push every row's bToggledOn back into its CClientOptions bit.
void CSWGuiInGameAutoPause::SaveOptions() {
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions == 0)
        return;

    CSWGuiOptionsCheckbox* pRow;
    pRow = static_cast<CSWGuiOptionsCheckbox*>(static_cast<CSWGuiControl*>(&m_aCheckbox[kCbEndRound])->AsButtonToggle());
    pOptions->m_bAutoPauseEndOfCombatRound = pRow->bToggledOn;
    pRow = static_cast<CSWGuiOptionsCheckbox*>(static_cast<CSWGuiControl*>(&m_aCheckbox[kCbEnemySighted])->AsButtonToggle());
    pOptions->m_bAutoPauseEnemySighted = pRow->bToggledOn;
    pRow = static_cast<CSWGuiOptionsCheckbox*>(static_cast<CSWGuiControl*>(&m_aCheckbox[kCbMineSighted])->AsButtonToggle());
    pOptions->m_bAutoPauseMineSighted = pRow->bToggledOn;
    pRow = static_cast<CSWGuiOptionsCheckbox*>(static_cast<CSWGuiControl*>(&m_aCheckbox[kCbPartyKilled])->AsButtonToggle());
    pOptions->m_bAutoPausePartyKilled = pRow->bToggledOn;
    pRow = static_cast<CSWGuiOptionsCheckbox*>(static_cast<CSWGuiControl*>(&m_aCheckbox[kCbActionMenu])->AsButtonToggle());
    pOptions->m_bAutoPauseActionMenu = pRow->bToggledOn;
    pRow = static_cast<CSWGuiOptionsCheckbox*>(static_cast<CSWGuiControl*>(&m_aCheckbox[kCbTriggers])->AsButtonToggle());
    pOptions->m_bAutoPauseNewTarget = pRow->bToggledOn;
}

void CSWGuiInGameAutoPause::HandleInputEvent(int type, int val) {
    if (val != 0) {
        switch (type) {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            ClosePanel();
            break;
#if !KOTOR1  // K1 has no restore-defaults arm
        case INPUTEVENT_GUI_Y:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            m_btnDefault.m_tail.nCurrentEventArg = 1;
            g_pAppManager->m_pClientExoApp->GetClientOptions()->SetDefaultAutopauseOptions();
            SetupOptions();
            break;
#endif
        case INPUTEVENT_GUI_START:
            if (m_pActiveControl && m_pActiveControl->AsButtonToggle()) {
                m_pGuiManager->PlayGuiSound(GUISOUND_CHECKBOX_CHECK);
                CSWGuiOptionsCheckbox* pCheckA =
                    static_cast<CSWGuiOptionsCheckbox*>(m_pActiveControl->AsButtonToggle());
                CSWGuiOptionsCheckbox* pCheckB =
                    static_cast<CSWGuiOptionsCheckbox*>(m_pActiveControl->AsButtonToggle());
                pCheckA->bToggledOn = !pCheckB->bToggledOn;
            }
            break;
        default:
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

CSWGuiInGameAutoPause::CSWGuiInGameAutoPause(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1  // K1: layout "optautopause", no bars, different row StrRefs
    StartLoadFromLayout(CResRef("optautopause"));
    InitControl(m_aCheckbox[kCbEndRound], CExoString("CB_ENDROUND"), 1);
    InitControl(m_aCheckbox[kCbEnemySighted], CExoString("CB_ENEMYSIGHTED"), 1);
    InitControl(m_aCheckbox[kCbMineSighted], CExoString("CB_MINESIGHTED"), 1);
    InitControl(m_aCheckbox[kCbPartyKilled], CExoString("CB_PARTYKILLED"), 1);
    InitControl(m_aCheckbox[kCbActionMenu], CExoString("CB_ACTIONMENU"), 1);
    InitControl(m_aCheckbox[kCbTriggers], CExoString("CB_TRIGGERS"), 1);
    InitControl(m_labelTitle, CExoString("LBL_TITLE"), 1);
    InitControl(m_listBoxDetails, CExoString("LB_DETAILS"), 1);
    InitControl(m_btnDefault, CExoString("BTN_DEFAULT"), 1);
    m_listBoxDetails.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1);

    m_aCheckbox[kCbEndRound].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameAutoPause::OnEnterSlot));
    m_aCheckbox[kCbEnemySighted].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameAutoPause::OnEnterSlot));
    m_aCheckbox[kCbMineSighted].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameAutoPause::OnEnterSlot));
    m_aCheckbox[kCbPartyKilled].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameAutoPause::OnEnterSlot));
    m_aCheckbox[kCbActionMenu].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameAutoPause::OnEnterSlot));
    m_aCheckbox[kCbTriggers].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameAutoPause::OnEnterSlot));

    m_aCheckbox[kCbEndRound].m_tail.m_nUserData = STRREF_DESC_PAUSE_END_OF_ROUND;
    m_aCheckbox[kCbEnemySighted].m_tail.m_nUserData = STRREF_DESC_PAUSE_ENEMY_SIGHTED;
    m_aCheckbox[kCbMineSighted].m_tail.m_nUserData = STRREF_DESC_PAUSE_MINE_SIGHTED;
    m_aCheckbox[kCbPartyKilled].m_tail.m_nUserData = STRREF_DESC_PAUSE_PARTY_KILLED;
    m_aCheckbox[kCbActionMenu].m_tail.m_nUserData = STRREF_DESC_PAUSE_ACTION_MENU;
    m_aCheckbox[kCbTriggers].m_tail.m_nUserData = STRREF_DESC_PAUSE_TARGET_CHANGED;
    StopLoadFromLayout();
    ScaleExtentForResolution();

    m_labelDesc.Initialize(m_listBoxDetails.m_pProtoItem->m_extent,
                           static_cast<CSWGuiLabel*>(m_listBoxDetails.m_pProtoItem), 1.0f);
    m_btnDefault.m_tail.bFocusOnHover = 0;
    m_btnDefault.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameAutoPause::OnDefault));

    SetupOptions();

    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    SetActiveControl(&m_aCheckbox[kCbEndRound], 0);

    m_aCheckbox[kCbEnemySighted].SetMoveToControl(2, &m_aCheckbox[kCbMineSighted]);
    m_aCheckbox[kCbPartyKilled].SetMoveToControl(0, &m_aCheckbox[kCbMineSighted]);
    m_aCheckbox[kCbMineSighted].SetMoveToControl(0, &m_aCheckbox[kCbEnemySighted]);

    m_aCheckbox[kCbEndRound].AddEvent(INPUTEVENT_GUI_Y, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameAutoPause::OnDefault));
    m_aCheckbox[kCbEnemySighted].AddEvent(INPUTEVENT_GUI_Y, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameAutoPause::OnDefault));
    m_aCheckbox[kCbMineSighted].AddEvent(INPUTEVENT_GUI_Y, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameAutoPause::OnDefault));
    m_aCheckbox[kCbPartyKilled].AddEvent(INPUTEVENT_GUI_Y, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameAutoPause::OnDefault));
    m_aCheckbox[kCbActionMenu].AddEvent(INPUTEVENT_GUI_Y, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameAutoPause::OnDefault));
    m_aCheckbox[kCbTriggers].AddEvent(INPUTEVENT_GUI_Y, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameAutoPause::OnDefault));

    IosAddButtonIcon(&m_btnDefault, 'y');
    IosAddButtonIcon(&m_btnBack, 'b');
#else
    StartLoadFromLayout(CResRef("optautopause_p"), 1, 1, 1);
    InitControl(m_aCheckbox[kCbEndRound], CExoString("CB_ENDROUND"), 1, 1, 0, 0, 0);
    InitControl(m_aCheckbox[kCbEnemySighted], CExoString("CB_ENEMYSIGHTED"), 1, 1, 0, 0, 0);
    InitControl(m_aCheckbox[kCbMineSighted], CExoString("CB_MINESIGHTED"), 1, 1, 0, 0, 0);
    InitControl(m_aCheckbox[kCbPartyKilled], CExoString("CB_PARTYKILLED"), 1, 1, 0, 0, 0);
    InitControl(m_aCheckbox[kCbActionMenu], CExoString("CB_ACTIONMENU"), 1, 1, 0, 0, 0);
    InitControl(m_aCheckbox[kCbTriggers], CExoString("CB_TRIGGERS"), 1, 1, 0, 0, 0);
    InitControl(m_labelTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_listBoxDetails, CExoString("LB_DETAILS"), 1, 1, 0, 0, 0);
    // BTN_DEFAULT is bound before BTN_BACK here, which is not the member order.
    InitControl(m_btnDefault, CExoString("BTN_DEFAULT"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);
#if !KOTOR1
    InitControl(m_aLabelBar[0], CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[1], CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[2], CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[3], CExoString("LBL_BAR4"), 1, 1, 0, 0, 0);
#endif

    m_listBoxDetails.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    // The row-hover wiring precedes the StrRef stamps below.
    for (int i = 0; i < 6; i++)
        m_aCheckbox[i].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameAutoPause::OnEnterSlot));

    m_aCheckbox[kCbEndRound].m_tail.m_nUserData = STRREF_DESC_PAUSE_END_OF_ROUND;
    m_aCheckbox[kCbEnemySighted].m_tail.m_nUserData = STRREF_DESC_PAUSE_ENEMY_SIGHTED;
    m_aCheckbox[kCbMineSighted].m_tail.m_nUserData = STRREF_DESC_PAUSE_MINE_SIGHTED;
    m_aCheckbox[kCbPartyKilled].m_tail.m_nUserData = STRREF_DESC_PAUSE_PARTY_KILLED;
    m_aCheckbox[kCbActionMenu].m_tail.m_nUserData = STRREF_DESC_PAUSE_ACTION_MENU;
    m_aCheckbox[kCbTriggers].m_tail.m_nUserData = STRREF_DESC_PAUSE_TARGET_CHANGED;
    StopLoadFromLayout();

    ExpandButtonToFitText(m_btnDefault);
    CSWGuiLabel* pProto = static_cast<CSWGuiLabel*>(m_listBoxDetails.m_pProtoItem);
    m_labelDesc.Initialize(pProto->m_extent, pProto, 1.0f);
    m_btnDefault.m_tail.bFocusOnHover = 0;
    m_btnDefault.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameAutoPause::OnDefault));
    AspyrAddButtonIcon(&m_btnBack, 'b');
    AspyrAddButtonIcon(&m_btnDefault, 'y');

    SetupOptions();

    // BTN_BACK's handler is the panel's virtual B-button entry point.
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    SetActiveControl(&m_aCheckbox[kCbEndRound], 0);
#endif
}
