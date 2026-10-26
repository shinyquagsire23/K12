#include "k2stdint.h"
#include "swguiingameoptions.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "swguicontrol.h"
#include "swguidrawable.h"
#include "swgui.h"
#include "swguiingame.h"
#include "exoresman.h"
#include "swguisaveload.h"
#include "swguiingamegameplay.h"
#include "swguioptions.h"
#include "swguiingameautopause.h"

#include "port/libcstring.h"
#include "exoinput.h"
#include "strref.h"
#include "exostringinline.h"

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#if KOTOR1
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#endif

// Hover-description StrRefs for the eight wall buttons, in button order.
#if KOTOR1  // K1: the five page entries differ
static const uint64_t aButtonDescriptions[8] = {
    0xa53c, 0xa53d, 0xbb86, 0xbd99, 0xbdbc, 0xbb8a, 0xbb8c, 0xa53e,
};
#else
static const uint64_t aButtonDescriptions[8] = {
    0xa53c, 0xa53d, 0xa521, 0x2147a, 0xa523, 0xbe2f, 0xbe30, 0xa53e,
};
#endif

CSWGuiInGameOptions::CSWGuiInGameOptions(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1  // K1: layout "optionsingame"
    StartLoadFromLayout(CResRef("optionsingame"), 1, 1, 1);
#else
    StartLoadFromLayout(CResRef("optionsingame_p"), 1, 1, 1);
#endif

    InitControl(m_aButtons[0], CExoString("BTN_LOADGAME"),  1, 1, 0, 0, 0);
    InitControl(m_aButtons[1], CExoString("BTN_SAVEGAME"),  1, 1, 0, 0, 0);
    InitControl(m_aButtons[2], CExoString("BTN_GAMEPLAY"),  1, 1, 0, 0, 0);
    InitControl(m_aButtons[3], CExoString("BTN_FEEDBACK"),  1, 1, 0, 0, 0);
    InitControl(m_aButtons[4], CExoString("BTN_AUTOPAUSE"), 1, 1, 0, 0, 0);
    InitControl(m_aButtons[5], CExoString("BTN_GRAPHICS"),  1, 1, 0, 0, 0);
    InitControl(m_aButtons[6], CExoString("BTN_SOUND"),     1, 1, 0, 0, 0);
    InitControl(m_aButtons[7], CExoString("BTN_QUIT"),      1, 1, 0, 0, 0);

    InitControl(m_lblTitle,    CExoString("LBL_TITLE"),     1, 1, 0, 0, 0);
    InitControl(m_listBoxDesc, CExoString("LB_DESC"),       1, 1, 0, 0, 0);
    // Nonzero scroll sound: the box still scrolls with a single row.
    m_listBoxDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;

    InitControl(m_btnExit,        CExoString("BTN_EXIT"),  1, 1, 0, 0, 0);
#if !KOTOR1
    InitControl(m_aBarLabels[0],  CExoString("LBL_BAR1"),  1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[1],  CExoString("LBL_BAR2"),  1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[2],  CExoString("LBL_BAR3"),  1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[3],  CExoString("LBL_BAR4"),  1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[4],  CExoString("LBL_BAR5"),  1, 1, 0, 0, 0);
#endif
    StopLoadFromLayout();
#if KOTOR1
    ScaleExtentForResolution();
#endif

    CSWGuiLabel* pProto = static_cast<CSWGuiLabel*>(m_listBoxDesc.m_pProtoItem);
    m_lblDesc.Initialize(pProto->m_extent, pProto, 1.0f);

    // The eight page openers, bound to the pad's X event.
    m_aButtons[0].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptions::OnLoadGame));
    m_aButtons[1].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptions::OnSaveGame));
    m_aButtons[2].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptions::OnGameplay));
    m_aButtons[3].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptions::OnFeedback));
    m_aButtons[4].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptions::OnAutopause));
    m_aButtons[5].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptions::OnGraphics));
    m_aButtons[6].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptions::OnSound));
    m_aButtons[7].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptions::OnQuit));

    // Every wall button joins the hover-description scheme and gets the highlight frame.
    for (int i = 0; i < 8; i++)
    {
        m_aButtons[i].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptions::OnEnterSlot));
#ifdef K2_UI_XBOX
        // console UI: the highlight fill would read as a solid bar behind the caption.
        m_aButtons[i].m_borderHilight.m_params.SetPulsingAlpha(1, 1, 1);
#else
        m_aButtons[i].m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);
#endif
    }

    // The exit button takes the base panel's B-button handler.
    m_btnExit.m_tail.bFocusOnHover = 0;
    m_btnExit.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));

#if KOTOR1  // K1: start on the load button and use the iOS glyph helper
    SetActiveControl(&m_aButtons[0], 0);
    IosAddButtonIcon(&m_btnExit, 'b');
#else
    SetActiveControl(&m_aButtons[1], 0);
    AspyrAddButtonIcon(&m_btnExit, 'b');
#endif
}

void CSWGuiInGameOptions::OnLoadGame(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        g_pAppManager->m_pClientExoApp->SaveOptions();
        CSWGuiSaveLoad* pPanel = new CSWGuiSaveLoad(m_pGuiManager, 0, 0, 0);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

// OnLoadGame with the save/load flag flipped.
void CSWGuiInGameOptions::OnSaveGame(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        g_pAppManager->m_pClientExoApp->SaveOptions();
        CSWGuiSaveLoad* pPanel = new CSWGuiSaveLoad(m_pGuiManager, 1, 0, 0);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

void CSWGuiInGameOptions::OnGameplay(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiInGameGameplay* pPanel = new CSWGuiInGameGameplay(m_pGuiManager);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

void CSWGuiInGameOptions::OnFeedback(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiOptionsFeedback* pPanel = new CSWGuiOptionsFeedback(m_pGuiManager);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

void CSWGuiInGameOptions::OnAutopause(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiInGameAutoPause* pPanel = new CSWGuiInGameAutoPause(m_pGuiManager);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

// The in-game options pass bInGame == 1; the out-of-game menu passes 0.
void CSWGuiInGameOptions::OnGraphics(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiOptionsGraphics* pPanel = new CSWGuiOptionsGraphics(m_pGuiManager, 1);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

void CSWGuiInGameOptions::OnSound(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiOptionsSound* pPanel = new CSWGuiOptionsSound(m_pGuiManager);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

// Reuses the shared solo-mode query box instead of allocating a page, and only re-arms it.
void CSWGuiInGameOptions::OnQuit(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        g_pAppManager->m_pClientExoApp->SaveOptions();
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(1);
        pBox->SetCallback(this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameOptions::ReallyQuit), 0);
        pBox->SetMessage(STRREF_CONFIRM_QUIT);
        m_pGuiManager->AddPanel(pBox, 1, 1);
    }
}

void CSWGuiInGameOptions::OnEnterSlot(CSWGuiControl* pControl)
{
    int i = 0;
    while (i < 8 && pControl != &m_aButtons[i])
        i++;
    if (i < 8)
        SetDescription(aButtonDescriptions[i]);
}

void CSWGuiInGameOptions::SetActiveButton(int nIndex)
{
    if (static_cast<unsigned int>(nIndex) <= 7u)
        SetActiveControl(&m_aButtons[nIndex], 0);
}

// m_bAccepted is set by the box's OK path, and by Cancel only when cancelling is disallowed.
void CSWGuiInGameOptions::ReallyQuit(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    if (pBox->m_bAccepted)
        g_pAppManager->m_pClientExoApp->SetEndGamePending(1, 0.0f, 1);
}

void CSWGuiInGameOptions::HandleInputEvent(int type, int val)
{
    if (val != 0)
    {
        switch (type)
        {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_START:
        case INPUTEVENT_GUI_BACK:
        case INPUTEVENT_GUI:
            if (g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSWInGameGui(0))
                g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
            // Falls through to the save arm.
        case INPUTEVENT_GUI_LEFT_SHOULDER:
        case INPUTEVENT_GUI_RIGHT_SHOULDER:
            g_pAppManager->m_pClientExoApp->SaveOptions();
            break;
#if !KOTOR1  // K1 has no next-party-member arm
        case INPUTEVENT_GUI_Y:
            m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            g_pAppManager->m_pClientExoApp->ChangeCharacterToNextLivingPartyMember(0, 0, 1);
            break;
#endif
        default:
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

// Hands the focus to the wall button under the mouse, if there is one.
void CSWGuiInGameOptions::OnPanelAdded()
{
    CSWGuiPanel::OnPanelAdded();

    CSWGuiControl* pControl = m_pGuiManager->m_pMouseOverControl;
    if (pControl != 0 && pControl->AsButton() != 0)
    {
        int nIndex = -1;
        if (pControl == &m_aButtons[0]) nIndex = 0;
        else if (pControl == &m_aButtons[1]) nIndex = 1;
        else if (pControl == &m_aButtons[2]) nIndex = 2;
        else if (pControl == &m_aButtons[3]) nIndex = 3;
        else if (pControl == &m_aButtons[4]) nIndex = 4;
        else if (pControl == &m_aButtons[5]) nIndex = 5;
        else if (pControl == &m_aButtons[6]) nIndex = 6;
        else if (pControl == &m_aButtons[7]) nIndex = 7;
        if (nIndex != -1)
            SetActiveControl(&m_aButtons[nIndex], 0);
    }

    if (GetActiveControl() == 0)
        SetActiveControl(&m_aButtons[0], 0);
}

// Releases the focus before the base teardown.
void CSWGuiInGameOptions::OnPanelRemoved()
{
    SetActiveControl(0, 0);
    CSWGuiPanel::OnPanelRemoved();
}

void CSWGuiInGameOptions::SetDescription(uint64_t nDescription)
{
    m_lblDesc.m_text.m_params.SetStrRef(nDescription);

    CExoString sText(m_lblDesc.m_text.m_params.m_sText);
    m_listBoxDesc.ClearItems();

    CSWGuiText* pText = &m_lblDesc.m_text;

    CSWGuiExtent extent = m_lblDesc.m_extent;
    if (sText.GetLength() >= 1)
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
    m_lblDesc.SetExtent(extent);

    CSWGuiControl* apRows[1] = { &m_lblDesc };
    m_listBoxDesc.AddControls(apRows, 1, 0, 0, 0);
    m_listBoxDesc.SetSelectedControl(&m_lblDesc, 0);

    // The scroll glyph shows while the description box is oversized.
    m_lblRandAnalog.m_tail.bVisible = m_listBoxDesc.IsOversized();
}
