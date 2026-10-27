
#include "k2stdint.h"
#include "port/sdlapi.h"
#define CSWGUI_FORCE_WIDGET_INLINE 1
#include "swguiquickpanel.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "swguimainchargen.h"
#include "nwccreature.h"
#include "swguiingame.h"
#include "guiingame.h"
#include "swgui.h"
#include "CExoArrayListImpl.h"
#include "swguicontrol.h"
#include "swguiportraitchargen.h"
#if KOTOR1
#include "port/k1gui.h"
#endif
#include "swguinamechargen.h"
#include "nwccreaturestats.h"
#include "nwsrules.h"
#include "nwclass.h"
#include "nwcanimbase.h"
#include "AURORA/aurmodel.h"
#include "exoresman.h"
#include "exoinput.h"
#include "strref.h"

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);

// Builds the three-row widget wall: a loop fills the parallel highlight, button and caption arrays
// off the shared quick-panel layout, then the trailing buttons are bound to their handlers.
#if KOTOR1
// K1 binds the smaller quick-panel layout and rescales then centres the panel in the 4:3 frame.
CSWGuiQuickPanel::CSWGuiQuickPanel(CSWGuiManager* pGuiManager, CSWCCreature* pCreature)
    : CSWGuiPanel(pGuiManager)
{
    CExoString s;

    m_pCreature = pCreature;

    StartLoadFromLayout(CResRef("QUICKPNL"));
    InitControl(m_lblDecoration, CExoString("LBL_DECORATION"), 1);

    for (int i = 0; i < 3; i++)
    {
        s.Format(const_cast<char*>("LBL_%d"), i + 1);
        InitControl(m_aHilights[i], s, 1);
        m_aHilights[i].m_hilightBorder.m_params.SetPulsingAlpha(1, 0, 1);

        s.Format(const_cast<char*>("BTN_STEPNAME%d"), i + 1);
        InitControl(m_aButtons[i], s, 1);

        s.Format(const_cast<char*>("LBL_NUM%d"), i + 1);
        InitControl(m_aLabels[i], s, 1);
    }

    InitControl(m_btnBack, CExoString("BTN_BACK"), 1);
    InitControl(m_btnCancel, CExoString("BTN_CANCEL"), 1);

    StopLoadFromLayout();
    ScaleExtentForResolution();

    CSWGuiExtent extent = m_extent;
    extent.x += (g_SDL_mode.w - static_cast<int>(static_cast<float>(g_SDL_mode.h) / 768.0f * 1024.0f)) / 2;
    SetExtent(extent);

    m_aButtons[0].AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickPanel::OnSelectPortraitButton));
    m_aButtons[1].AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickPanel::OnSelectNameButton));
    m_aButtons[2].AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickPanel::OnSelectPlayButton));

    m_aButtons[0].AddEvent(INPUTEVENT_GUI_START, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickPanel::OnSelectPortraitButton));
    m_aButtons[1].AddEvent(INPUTEVENT_GUI_START, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickPanel::OnSelectNameButton));
    m_aButtons[2].AddEvent(INPUTEVENT_GUI_START, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickPanel::OnCancelPressed));

    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnCancel.m_tail.bFocusOnHover = 0;
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickPanel::OnCancelPressed));

    SetState(0);

    IosAddButtonIcon(&m_btnBack, 'b');
#if !defined(K1_UI_XBOX) && !defined(K2_QOL_CONTROLLER)  // B backs out here, so it is Cancel's glyph
    IosAddButtonIcon(&m_btnCancel, 'b');
#endif
}
#else
CSWGuiQuickPanel::CSWGuiQuickPanel(CSWGuiManager* pGuiManager, CSWCCreature* pCreature)
    : CSWGuiPanel(pGuiManager)
{
    CExoString s;

    m_pCreature = pCreature;

    StartLoadFromLayout(CResRef("QUICKPNL_p"), 0, 1, 1);

    for (int i = 0; i < 3; i++)
    {
        s.Format(const_cast<char*>("LBL_%d"), i + 1);
        InitControl(m_aHilights[i], s, 1, 1, 0, 0, 0);
        m_aHilights[i].m_hilightBorder.m_params.SetPulsingAlpha(1, 0, 1);

        s.Format(const_cast<char*>("BTN_STEPNAME%d"), i + 1);
        InitControl(m_aButtons[i], s, 1, 1, 0, 0, 0);

        s.Format(const_cast<char*>("LBL_NUM%d"), i + 1);
        InitControl(m_aLabels[i], s, 1, 1, 0, 0, 0);
    }

    InitControl(m_btnCancel, CExoString("BTN_CANCEL"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);

    StopLoadFromLayout();

    m_aButtons[0].AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickPanel::OnSelectPortraitButton));
    m_aButtons[1].AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickPanel::OnSelectNameButton));
    m_aButtons[2].AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickPanel::OnSelectPlayButton));

    m_aButtons[0].AddEvent(INPUTEVENT_GUI_START, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickPanel::OnSelectPortraitButton));
    m_aButtons[1].AddEvent(INPUTEVENT_GUI_START, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickPanel::OnSelectNameButton));
    m_aButtons[2].AddEvent(INPUTEVENT_GUI_START, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickPanel::OnCancelPressed));

    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnCancel.m_tail.bFocusOnHover = 0;
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickPanel::OnCancelPressed));

    SetState(0);

    AspyrAddButtonIcon(&m_btnBack, 'b');
    AspyrAddButtonIcon(&m_btnCancel, 'x');
}
#endif

// Opens the portrait page, putting the creature's two models into their idle pose first.
void CSWGuiQuickPanel::OnSelectPortraitButton(CSWGuiControl* pControl)
{
    if (m_nState >= 0 && pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiPortraitCharGen* pPanel = new CSWGuiPortraitCharGen(m_pGuiManager, m_pCreature);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        pPanel->SetParentPanel(this, 1);

        if (m_pCreature->GetAnimationBase() != 0)
        {
            m_pCreature->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY)
                ->PlayAnimation(const_cast<char*>("pause2"), 1.0f, 0, 0.0f);
            m_pCreature->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD)
                ->PlayAnimation(const_cast<char*>("pause2"), 1.0f, 0, 0.0f);
        }

        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

// Opens the name page, which unlike its siblings adds no border panel.
void CSWGuiQuickPanel::OnSelectNameButton(CSWGuiControl* pControl)
{
    if (m_nState > 0 && pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiNameChargen* pPanel = new CSWGuiNameChargen(m_pGuiManager, m_pCreature);
        pPanel->SetParentPanel(this, 1);
#if KOTOR1  // K1: the Chromebook build opens the soft keyboard; the name page goes on layer 3
        if (SDL_IsChromebook())
            SDL_StartTextInput();
        m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(pPanel), 3, 1);  // idiom-exempt: same flat-model cast
#else
        m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(pPanel), 1, 1);  // idiom-exempt: same flat-model cast
#endif
    }
}

// Starts the game, but not from the first two rows.
void CSWGuiQuickPanel::OnSelectPlayButton(CSWGuiControl* pControl)
{
    if (m_nState > 1 && pControl->m_tail.nCurrentEventArg != 0)
        static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->BeginPlayGame();
}

// Raises the shared confirmation box, or cancels the page from the first row.
void CSWGuiQuickPanel::OnCancelPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (m_nState != 0)
    {
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(1);
        pBox->SetMessage(static_cast<uint64_t>(STRREF_CONFIRM_CANCEL_CHARACTER));
        pBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiQuickPanel::ConfirmCancel), 0);
        m_pGuiManager->AddPanel(pBox, 1, 1);
    }
    else
    {
        CancelPanel();
    }
}

// Sets the current row, which is also the active control, and repaints the three rows: the commit
// button is live in any non-zero row, the current row is highlighted, and every other row is
// disabled.
void CSWGuiQuickPanel::SetState(int nState)
{
#if KOTOR1
    // K1 recolours only the current row, in yellow; the others keep their colours.
    m_nState = nState;

    const Vector& vBack = nState == 0 ? CGuiInGame::DISABLED_TEXT : CGuiInGame::COLOR_BLUE;
    m_btnBack.SetEnabled(nState != 0);
    m_btnBack.m_text.m_params.SetColor(vBack);

    for (int i = 0; i < 3; i++)
    {
        if (i == m_nState)
        {
            m_aHilights[i].SetActive(1);
            m_aLabels[i].SetEnabled(1);
            m_aButtons[i].SetEnabled(1);
            m_aLabels[i].m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
            m_aButtons[i].m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
            m_aLabels[i].m_text.m_params.bHilight = 1;
            m_aButtons[i].m_text.m_params.bHilight = 1;
        }
        else
        {
            m_aHilights[i].SetActive(0);
            m_aLabels[i].m_text.m_params.bHilight = 0;
            m_aButtons[i].m_text.m_params.bHilight = 0;
            m_aLabels[i].SetEnabled(0);
            m_aButtons[i].SetEnabled(0);
        }
    }

    SetActiveControl(&m_aButtons[m_nState], 0);
#else
    m_nState = nState;

    m_btnBack.SetEnabled(nState != 0);
    m_btnBack.m_text.m_params.SetColor(nState == 0 ? CGuiInGame::DISABLED_TEXT
                                                   : CGuiInGame::COLOR_NEW_TEXT);

    for (int i = 0; i < 3; i++)
    {
        if (i == m_nState)
        {
            m_aHilights[i].SetActive(1);
            m_aLabels[i].SetEnabled(1);
            m_aButtons[i].SetEnabled(1);
            m_aLabels[i].m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
            m_aButtons[i].m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
            m_aButtons[i].m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_HIGHLIGHT);
            m_aLabels[i].m_text.m_params.bHilight = 1;
            m_aButtons[i].m_text.m_params.bHilight = 1;
        }
        else
        {
            m_aHilights[i].SetActive(0);
            m_aLabels[i].m_text.m_params.bHilight = 0;
            m_aButtons[i].m_text.m_params.bHilight = 0;
            if (i < m_nState)
            {
                m_aLabels[i].m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
                m_aButtons[i].m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
                m_aButtons[i].m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
            }
            else
            {
                m_aLabels[i].m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_GREEN);
                m_aButtons[i].m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_GREEN);
                m_aButtons[i].m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_TEXT);
            }
            m_aLabels[i].SetEnabled(0);
            m_aButtons[i].SetEnabled(0);
        }
    }

    SetActiveControl(&m_aButtons[m_nState], 0);
#endif
}

// Builds the quick stats, resyncs the owning character-generation panel, then chains to the base.
void CSWGuiQuickPanel::OnPanelAdded()
{
    DoQuickStats();
    static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->AbilitiesChanged();
    CSWGuiPanel::OnPanelAdded();
}

// Applies the class's recommended package to the scratch stats: the ability bases from the
// classes.2da row, then the granted feats and powers, the default-package feats and skills, and the
// derived skill ranks.
void CSWGuiQuickPanel::DoQuickStats()
{
    uint64_t nClass = m_pCreature->m_pStats->GetClassLeveledUpIn();

    CSWClass* pClass = &g_pRules->m_paClasses[nClass & 0xff];

    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetSTRBase(pClass->m_abRecommendedScores[0]);
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetDEXBase(pClass->m_abRecommendedScores[1], 1);
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetCONBase(pClass->m_abRecommendedScores[2]);
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetWISBase(pClass->m_abRecommendedScores[4]);
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetINTBase(pClass->m_abRecommendedScores[3]);
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetCHABase(pClass->m_abRecommendedScores[5]);

    CExoArrayList<unsigned short> aFeats;
#if KOTOR1
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddGrantedFeats(0);
#else
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddGrantedFeats(0, true);
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddGrantedPowers(0);
#endif
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddDefaultPackageFeats(static_cast<int>(nClass), &aFeats, 1, 1);

    for (int i = 0; i < aFeats.m_count; i++)
    {
        CSWCCreatureStats* pStats = m_pCreature->m_pStats;
        unsigned short nFeat = aFeats.m_pArray[i];

        pStats->m_lstFeats.AddUnique(nFeat);
    }

    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->m_nSkillPointsRemaining = 0;
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddSkillPointsPerLevel();
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddDefaultPackageSkills(static_cast<int>(nClass), 1);
    m_pCreature->m_pStats->UpdateAbilityModifiedSkillRanks();
}

// Handles keys. The arrow keys step the row back one and re-apply the state, cancelling from the
// first row; page-up opens the confirm-cancel box; the middle key only plays the click sound.
// Every path chains to the base handler.
void CSWGuiQuickPanel::HandleInputEvent(int type, int val)
{
    if (val != 0)
    {
        switch (type)
        {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            if (m_nState == 0)
            {
                CancelPanel();
            }
            else
            {
                m_nState = m_nState - 1;
                SetState(m_nState);
            }
            break;

#if !KOTOR1
        case INPUTEVENT_GUI_X:
        {
            CSWGuiControl* pControl;  // deliberately uninitialised
#ifdef K2_FIX_ENGINE_BUGS
            pControl = nullptr;
#endif
            OnCancelPressed(pControl);
            break;
        }
#endif

        case INPUTEVENT_GUI_START:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            break;

        default:
            break;
        }
    }

    CSWGuiPanel::HandleInputEvent(type, val);
}

void CSWGuiQuickPanel::CancelPanel()
{
    if (m_pParentPanel)
    {
        static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->SetVisiblePanel(CSWGuiMainCharGen::VISIBLE_PANEL_QUICK_OR_CUSTOM);
        static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->ClearAbilities();
    }

    SetState(0);
}

void CSWGuiQuickPanel::PlayGame()
{
    static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->BeginPlayGame();
}

CExoString CSWGuiQuickPanel::GetCurrentName()
{
    return static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->m_sName;
}

// Runs when a page was backed out of without confirming. Resyncs the portrait when the first row is
// showing and re-highlights the current row.
void CSWGuiQuickPanel::CancelledByChild()
{
    if (m_nState == 0)
        static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->PortraitChanged();

#if KOTOR1
    m_aButtons[m_nState].m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
#else
    m_aButtons[m_nState].m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
#endif
    m_aButtons[m_nState].m_text.m_params.bHilight |= 1;
}

// Runs when the portrait page hands control back; resyncs the preview and advances to the second
// row.
void CSWGuiQuickPanel::PortraitCreationCompleted()
{
    static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->PortraitChanged();
    SetState(1);
}

// Runs when the name page hands control back; commits the entered name and advances to the third
// row.
void CSWGuiQuickPanel::NameCreationCompleted()
{
    static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->NameChanged();
    SetState(2);
}

// Cancels the panel once the shared confirmation box has been accepted.
void CSWGuiQuickPanel::ConfirmCancel(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (!g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted)
        return;

    if (m_pParentPanel) {
        static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->SetVisiblePanel(CSWGuiMainCharGen::VISIBLE_PANEL_QUICK_OR_CUSTOM);
        static_cast<CSWGuiMainCharGen*>(m_pParentPanel)->ClearAbilities();
    }
    SetState(0);
}
