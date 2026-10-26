#include "k2stdint.h"
#include "port/sdlapi.h"
#include "nwscriptconst.h"
#define CSWGUI_FORCE_WIDGET_INLINE 1
#include "swguinamechargen.h"
#include "swgui.h"
#include "swguicontrol.h"
#include "exoresman.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "swguiquickpanel.h"
#include "swguicustompanel.h"
#include "nwccreature.h"
#include "nwccreaturestats.h"
#include "swguiingame.h"
#include "nwrace.h"

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);
void IosAddLabelIcon(CSWGuiLabel* pLabel, char nIcon);
void ASLPlat_ShowVirtualKeyboard(const char* sText, unsigned int nMaxLength);
bool IsJoystickConnected();
#if KOTOR1
extern unsigned int pressedGamepadButtons;
#endif

#include "port/libcstring.h"
#include "exoinput.h"
#include "strref.h"
#include "exostringinline.h"

// The longest name the edit box and the virtual keyboard take.
enum { NAME_MAX_LENGTH = 18 };

CSWGuiNameChargen::CSWGuiNameChargen(CSWGuiManager* pGuiManager, CSWCCreature* pCreature)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1
    CExoString s;

    m_pCreature = pCreature;

    StartLoadFromLayout(CResRef("NAME"), 1, 1, 1);

    InitControl(m_btnEnd, CExoString("END_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_editBox, CExoString("NAME_BOX_EDIT"), 1, 1, 0, 0, 0);
    InitControl(m_lblMainTitle, CExoString("MAIN_TITLE_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblSubTitle, CExoString("SUB_TITLE_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_btnRandom, CExoString("BTN_RANDOM"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);

    // K1 swaps the end and back buttons' places
    CSWGuiExtent extentEnd = m_btnEnd.m_extent;
    CSWGuiExtent extentBack = m_btnBack.m_extent;
    m_btnBack.SetExtent(extentEnd);
    m_btnEnd.SetExtent(extentBack);

    m_lblName = m_lblSubTitle;

    CSWGuiExtent extent = m_editBox.m_extent;
    extent.x -= 200;
    m_lblName.SetExtent(extent);
    m_lblName.m_text.m_params.SetText(CExoString());
    m_lblName.m_tail.bIgnoreMouse = 1;
    AddControl(&m_lblName);
    IosAddLabelIcon(&m_lblName, 'x');

    StopLoadFromLayout();
    ScaleExtentForResolution();

    m_btnEnd.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiNameChargen::HandleDoneButton));
    m_btnEnd.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiNameChargen::HandleDoneButton));
    m_btnRandom.m_tail.bFocusOnHover = 0;
    m_btnRandom.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnYButtonPressed));
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_editBox.m_editText.m_nMaxLength = NAME_MAX_LENGTH;
    m_btnEnd.m_tail.bFocusOnHover = 0;
    m_bNameGenerated = 0;
    m_editBox.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiNameChargen::HandleDoneButton));

    IosAddButtonIcon(&m_btnRandom, 'y');
    IosAddButtonIcon(&m_btnBack, 'b');
    IosAddButtonIcon(&m_btnEnd, 'a');
#else
    CExoString s;

    m_pCreature = pCreature;

    StartLoadFromLayout(CResRef("NAME_p"), 1, 1, 1);

#if !KOTOR1
    m_bDrawOffScreen = 1;
#endif

    InitControl(m_btnEnd, CExoString("END_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_lblMainTitle, CExoString("MAIN_TITLE_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblSubTitle, CExoString("SUB_TITLE_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_btnRandom, CExoString("BTN_RANDOM"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);
    InitControl(m_editBox, CExoString("NAME_BOX_EDIT"), 1, 1, 0, 0, 0);

    m_lblName = m_lblSubTitle;

    CSWGuiExtent extent = m_editBox.m_extent;
    extent.x = static_cast<int>(static_cast<float>(g_nScaledNativeWidth) / static_cast<float>(g_nScreenWidth) * -200.0f + extent.x);
    m_lblName.SetExtent(extent);
    m_lblName.m_text.m_params.SetText(CExoString());
    m_lblName.m_tail.bIgnoreMouse = 1;
    AddControl(&m_lblName);
    IosAddLabelIcon(&m_lblName, 'x');

    StopLoadFromLayout();

    m_btnEnd.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiNameChargen::HandleDoneButton));
    m_btnEnd.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiNameChargen::HandleDoneButton));
    m_btnRandom.m_tail.bFocusOnHover = 0;
    m_btnRandom.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnYButtonPressed));
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));

    AspyrAddButtonIcon(&m_btnEnd, 'a');
    AspyrAddButtonIcon(&m_btnBack, 'b');
    AspyrAddButtonIcon(&m_btnRandom, 'y');

    m_btnEnd.m_tail.bFocusOnHover = 0;
    m_bNameGenerated = 0;
    m_editBox.m_editText.m_nMaxLength = NAME_MAX_LENGTH;
    m_editBox.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiNameChargen::HandleDoneButton));

    CSWGuiExtent extentPanel = m_extent;
    extentPanel.y -= 50;
    SetExtent(extentPanel);
#endif
}

// Dismisses the modal and hands control to whichever page pushed it.
void CSWGuiNameChargen::HandleCancelButton()
{
    if (m_pParentPanel != 0)
    {
        m_pGuiManager->PopModalPanel();
        m_pParentPanel->SetVisible(1);

        SetToBeDeleted(0);
#if KOTOR1
        // K1 also closes the on-screen keyboard.
        if (SDL_IsChromebook())
            SDL_StopTextInput();
#endif

        if (m_nParentKind == 1)
        {
            static_cast<CSWGuiQuickPanel*>(m_pParentPanel)->CancelledByChild();
            return;
        }
        if (m_nParentKind == 2)
        {
            static_cast<CSWGuiCustomPanel*>(m_pParentPanel)->CancelledByChild();
            return;
        }
    }
}

// Rolls a human first and last name off the embedded generator, within a 15-letter budget; the
// gender picks the first-name part set (6 = female first + last, 5 = male first + last).
void CSWGuiNameChargen::HandleRandomName()
{
    CExoString sName;
    if (m_pCreature->m_pStats->m_nGender == GENDER_FEMALE)
        sName = m_nameGen.GetRandomName(CSWRace::CRACE_HUMAN, 6, 0xf);
    else
        sName = m_nameGen.GetRandomName(CSWRace::CRACE_HUMAN, 5, 0xf);

    m_editBox.m_editText.SetText(sName);
    m_bNameGenerated = 1;
}

#if defined(K2_UI_XBOX) || defined(K1_UI_XBOX)
void k2namekeyboard_push_chargen(CSWGuiNameChargen* pPanel);
#endif

// Focuses the name field and seeds it from the parent page's current name.
void CSWGuiNameChargen::OnPanelAdded()
{
    CExoString sName;

    m_editBox.SetFocus();

#if KOTOR1
    CSWGuiPanel::OnPanelAdded();  // K1 always calls the base
#else
    if (!IsJoystickConnected())
        CSWGuiPanel::OnPanelAdded();
#endif

    if (m_nParentKind == 1)
        sName = static_cast<CSWGuiQuickPanel*>(m_pParentPanel)->GetCurrentName();
    else if (m_nParentKind == 2)
        sName = static_cast<CSWGuiCustomPanel*>(m_pParentPanel)->GetCurrentName();

    if (sName.GetLength() != 0)
        m_editBox.m_editText.SetText(sName);
    else
        HandleRandomName();
#if defined(K2_UI_XBOX) || defined(K1_UI_XBOX)
    k2namekeyboard_push_chargen(this);
#endif
}

// Commits the typed name, trimming its trailing spaces; an empty name re-raises the message box.
void CSWGuiNameChargen::HandleDoneButton(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CSWGuiMessageBox* pBox;
    CExoString sName;
    sName = m_editBox.m_editText.m_sText;

#if KOTOR1
    // K1: the page closes itself before the parent is told.
    if (sName.GetLength() != 0)
    {
        while (sName[sName.GetLength() - 1] == ' ')
            sName = sName.Left(sName.GetLength() - 1);

        m_pCreature->SetName(sName, CExoString(""));

        m_pGuiManager->PopModalPanel();
        m_pParentPanel->SetVisible(1);
        SetToBeDeleted(0);
        if (SDL_IsChromebook())
            SDL_StopTextInput();

        if (m_nParentKind == 1)
            static_cast<CSWGuiQuickPanel*>(m_pParentPanel)->NameCreationCompleted();
        else if (m_nParentKind == 2)
            static_cast<CSWGuiCustomPanel*>(m_pParentPanel)->NameCreationCompleted();
    }
#else
    if (sName.GetLength() != 0)
    {
        while (sName[sName.GetLength() - 1] == ' ')
            sName = sName.Left(sName.GetLength() - 1);

        m_pCreature->SetName(sName, CExoString(""));

        if (m_nParentKind == 1)
            static_cast<CSWGuiQuickPanel*>(m_pParentPanel)->NameCreationCompleted();
        else if (m_nParentKind == 2)
            static_cast<CSWGuiCustomPanel*>(m_pParentPanel)->NameCreationCompleted();

        // inlined CSWGuiPanel::SetToBeDeleted(0): the call moves one landing pad (LENGTH -4)
        CSWGuiPanel* pChain = this;
        do {
            pChain->m_stateFlags = static_cast<unsigned short>((pChain->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE);
            pChain = pChain->m_pNextInChain;
        } while (pChain != 0);

        m_pGuiManager->PopModalPanel();
    }
#endif
    else
    {
        pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetCallback(0, 0, 0);
        pBox->SetMessage(static_cast<uint64_t>(STRREF_MUST_SPECIFY_NAME));
        m_pGuiManager->AddPanel(pBox, 1, 1);
    }
}

void CSWGuiNameChargen::HandleInputEvent(int nType, int nArg)
{
    if (nArg != 0)
    {
        switch (nType)
        {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            HandleCancelButton();
            break;
        case INPUTEVENT_GUI_X:
#if KOTOR1
            // K1: the keyboard key clears the pad latch first.
            pressedGamepadButtons = 0;
            if (SDL_IsChromebook())
                m_editBox.m_editText.SetText(CExoString(""));
#endif
            ASLPlat_ShowVirtualKeyboard(m_editBox.m_editText.m_sText.CStr(), NAME_MAX_LENGTH);
            break;
        case INPUTEVENT_GUI_Y:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            HandleRandomName();
            break;
        case INPUTEVENT_GUI_START:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            break;
        }
    }

    CSWGuiPanel::HandleInputEvent(nType, nArg);
}

void CSWGuiNameChargen::SetParentPanel(CSWGuiPanel* pPanel, int flag)
{
    m_pParentPanel = pPanel;
    m_nParentKind = static_cast<unsigned char>(flag);
}
