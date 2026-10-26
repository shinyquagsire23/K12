#include "swguileveluppanel.h"
#include "swguilevelupchargen.h"
#include "swguicontrol.h"
#include "nwccreature.h"
#include "nwccreaturestats.h"
#include "swgui.h"
#include "swguiabilitieschargen.h"
#include "swguiskillschargen.h"
#include "swguifeatschargen.h"
#include "swguipowerslvlup.h"
#include "swguiskillflowchart.h"
#include "guiingame.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "CExoArrayListDtors.h"
#include "swguiingame.h"
#include "exoinput.h"
#include "strref.h"

#if KOTOR1 && !defined(K2_UI_TOUCH)
#include "port/k1gui.h"
#endif

void CSWGuiLevelUpPanel::ClearPowers()
{
    m_pCharGenPanel->ClearPowers();
}

void CSWGuiLevelUpPanel::ClearAbilities()
{
    m_pCharGenPanel->ClearAbilities();
}

void CSWGuiLevelUpPanel::ClearSkills()
{
    m_pCharGenPanel->ClearSkills();
}

void CSWGuiLevelUpPanel::ClearFeats()
{
    m_pCharGenPanel->ClearFeats();
}

// Commits the level-up once a choice has been made: a non-zero event argument means a pending
// selection.
void CSWGuiLevelUpPanel::OnSelectAcceptButton(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
        PlayGame();
}

void CSWGuiLevelUpPanel::NewAbilityScores(int nStr, int nCha, int nInt, int nWis, int nCon,
                                          int nDex, int nExtra)
{
    m_pCharGenPanel->NewAbilityScores(nStr, nCha, nInt, nWis, nCon, nDex, nExtra);
}

// Drops one open child page, lets the panel re-pick its live page, then re-shows the owning
// char-gen screen.
void CSWGuiLevelUpPanel::CancelledByChild()
{
    m_nCurrentPage = m_nCurrentPage - 1;
    ChangeState(0);
    m_pCharGenPanel->SetVisible(1);
}

// Shows the screen, tells it the ability grid changed, then re-picks the live page.
void CSWGuiLevelUpPanel::AbilitySelectionCompleted()
{
    m_pCharGenPanel->SetVisible(1);
    m_pCharGenPanel->AbilitiesChanged();
    ChangeState(0);
}

// The same as AbilitySelectionCompleted without the data-changed call.
void CSWGuiLevelUpPanel::SkillSelectionCompleted()
{
    m_pCharGenPanel->SetVisible(1);
    ChangeState(0);
}

// Backs out of the level-up: only a page with something pending has anything to drop, and it
// drops all four lists through the char-gen screen's own Clear* methods. The open-page counter then
// resets and the panel moves to state 1.
void CSWGuiLevelUpPanel::OnCancelPressed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (m_bCanCancel == 0)
        return;

    m_pCharGenPanel->ClearPowers();
    m_pCharGenPanel->ClearFeats();
    m_pCharGenPanel->ClearSkills();
    m_pCharGenPanel->ClearAbilities();
    m_nCurrentPage = LEVELUP_STEP_ABILITIES;
    ChangeState(1);
}

// Shows the owning screen and re-picks the live page.
void CSWGuiLevelUpPanel::PowersSelectionCompleted()
{
    m_pCharGenPanel->SetVisible(1);
    ChangeState(0);
}

void CSWGuiLevelUpPanel::FeatSelectionCompleted()
{
    m_pCharGenPanel->SetVisible(1);
    ChangeState(0);
}

// Moves the panel onto a selectable page. A non-zero state with an unmoved page index is the
// cancel arm: it drops the level-up wholesale, taking the shared skill info box down with it when
// the manager still holds it. Otherwise the walk searches m_anState in the direction nState selects
// for the next selectable page, discards that page's pending selection, repaints the tab strip and
// re-gates the back button.
void CSWGuiLevelUpPanel::ChangeState(int nState)
{
    if (nState != 0 && m_nCurrentPage == m_nFirstPage)
    {
        if (m_bCanCancel == 0)
            return;

        m_pCharGenPanel->CancelLevelUp();

        CSWGuiSkillInfoBox* pBox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSkillInfoBoxPanel;
        if (m_pGuiManager->PanelExists(static_cast<CSWGuiPanel*>(pBox)))
            m_pGuiManager->RemovePanel(static_cast<CSWGuiPanel*>(pBox));
        return;
    }

    int nDir = (nState == 0) ? 1 : -1;
    int i = m_nCurrentPage + nDir;
    while (static_cast<unsigned>(i) < 5 && m_anState[i] != 1)
        i += nDir;

    if (static_cast<unsigned>(i) < 5)
    {
        // On leaving this page, drop whatever the player had staged on it (the accept step is the
        // summary and stages nothing).
        switch (i)
        {
        case LEVELUP_STEP_ABILITIES: m_pCharGenPanel->ClearAbilities(); break;
        case LEVELUP_STEP_SKILLS:    m_pCharGenPanel->ClearSkills();    break;
        case LEVELUP_STEP_FEATS:     m_pCharGenPanel->ClearFeats();     break;
        case LEVELUP_STEP_POWERS:    m_pCharGenPanel->ClearPowers();    break;
        }
        m_nCurrentPage = i;

        for (int j = 0; j < 5; j++)
        {
            if (j == m_nCurrentPage)
            {
                m_aLabels[j].SetEnabled(1);
                m_aButtons[j].SetEnabled(1);
                m_aPages[j].SetActive(1);
#if KOTOR1
                // K1: the global yellow / blue pair.
                m_aLabels[j].m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
                m_aButtons[j].m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
#else
                m_aLabels[j].m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
                m_aButtons[j].m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
#endif
                m_aLabels[j].m_text.m_params.bHilight = 1;
                m_aButtons[j].m_text.m_params.bHilight = 1;
            }
            else
            {
                m_aPages[j].SetActive(0);
                m_aLabels[j].m_text.m_params.bHilight = 0;
                m_aButtons[j].m_text.m_params.bHilight = 0;
                m_aLabels[j].SetEnabled(0);
                m_aButtons[j].SetEnabled(0);
            }
        }

        SetActiveControl(&m_aButtons[m_nCurrentPage], 0);
    }

    bool bEnabled = (m_nCurrentPage != m_nFirstPage) || (m_bCanCancel != 0);
    m_btnBack.CSWGuiButton::SetEnabled(bEnabled);
#if KOTOR1
    m_btnBack.m_text.m_params.SetColor(bEnabled ? CGuiInGame::COLOR_BLUE
                                                    : CGuiInGame::DISABLED_TEXT);
#else
    m_btnBack.m_text.m_params.SetColor(bEnabled ? CGuiInGame::COLOR_NEW_TEXT
                                                    : CGuiInGame::DISABLED_TEXT);
#endif
}

// The accept and cancel keys play the click sound; accept and cancel also commit or back out.
// Everything else, including a zero argument, falls through to the base handler.
void CSWGuiLevelUpPanel::HandleInputEvent(int nEvent, int nArg)
{
    if (nArg != 0)
    {
        switch (nEvent)
        {
        case INPUTEVENT_GUI_START:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            break;

        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            ChangeState(1);
            break;
        }
    }

    CSWGuiPanel::HandleInputEvent(nEvent, nArg);
}

// Commits the level-up. A page the player was never offered (its m_anState flag is 0) has its
// automatic grant flushed; feat and power are both flushed, then the owning screen is handed back.
void CSWGuiLevelUpPanel::PlayGame()
{
#if KOTOR1
    // K1: only the feat page is flushed; there is no power grant.
    if (m_anState[LEVELUP_STEP_FEATS] == 0) {
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddGrantedFeats(0);
        m_pCharGenPanel->SetVisible(1);
        ChangeState(0);
    }
#else
    if (m_anState[LEVELUP_STEP_FEATS] == 0) {
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddGrantedFeats(0, 0);
        m_pCharGenPanel->SetVisible(1);
        ChangeState(0);
    }
    if (m_anState[LEVELUP_STEP_POWERS] == 0) {
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddGrantedPowers(0);
        m_pCharGenPanel->SetVisible(1);
        ChangeState(0);
    }
#endif
    m_pCharGenPanel->BeginPlayGame();
}

// The three "pick a category" button handlers share one body: create the chosen sub-panel, point
// its parent pointer at this panel, give it the shared "bg_replacement" border and push it onto the
// manager.
void CSWGuiLevelUpPanel::OnSelectAbilitiesButton(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiAbilitiesCharGen* pPanel =
            new CSWGuiAbilitiesCharGen(m_pGuiManager, m_pCreature, 1);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        pPanel->m_pParentPanel = this;
        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

void CSWGuiLevelUpPanel::OnSelectSkillsButton(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiSkillsCharGen* pPanel =
            new CSWGuiSkillsCharGen(m_pGuiManager, m_pCreature, 1);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        pPanel->m_pParentPanel = this;
        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

// Feats is the odd one out: it asks the level-up stats how many feat slots the new level grants and
// passes "no normal slots" as the sub-panel constructor's fourth argument.
void CSWGuiLevelUpPanel::OnSelectFeatsButton(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        unsigned char nNormal = 0;
        unsigned char nBonus = 0;
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->CalcNumberFeats(nNormal, nBonus);
        CSWGuiFeatsCharGen* pPanel =
            new CSWGuiFeatsCharGen(m_pGuiManager, m_pCreature, 1, nNormal == 0);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        pPanel->m_pParentPanel = this;
        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

// Powers takes the creature's object id, not the creature pointer.
void CSWGuiLevelUpPanel::OnSelectPowersButton(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiPowersLevelUp* pPanel =
            new CSWGuiPowersLevelUp(m_pGuiManager, m_pCreature->m_objectId);
#if !KOTOR1
        pPanel->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        pPanel->m_pParentPanel = this;
        m_pGuiManager->AddPanel(pPanel, 3, 1);
    }
}

#if !KOTOR1
// Collects the level's granted powers and, only if any came back, hands them to the shared info
// box, retitles it and pushes it. This is the one handler that reaches the skill info box rather
// than a sub-panel.
void CSWGuiLevelUpPanel::ShowGrantedPowers(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    CExoArrayList<unsigned int> powers;
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddGrantedPowers(&powers);
    if (powers.m_count >= 1)
    {
        CSWGuiSkillInfoBox* pBox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSkillInfoBoxPanel;
        pBox->SetSkillList(powers, CSWGuiInGameSkillEntry::ENTRY_POWER);
        pBox->m_lblMessage.m_text.m_params.SetStrRef(STRREF_GRANTED_POWERS);
        m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(pBox), 1, 1);
    }
}
#endif

// Seeds the page index to -1, lets ChangeState(0) re-pick the live page and snapshots the result
// (the pair SetCanCancel compares), then hands the granted feats or powers to the shared skill info
// box.
void CSWGuiLevelUpPanel::OnPanelAdded()
{
    m_nCurrentPage = -1;
    ChangeState(0);
    m_nFirstPage = m_nCurrentPage;

    if (m_bCanCancel == 0)
    {
        m_btnBack.CSWGuiButton::SetEnabled(0);
        m_btnBack.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        m_btnCancel.m_tail.bVisible = 0;
    }

    CSWGuiPanel::OnPanelAdded();

#if !KOTOR1
    if (m_bHasGrantedFeats != 0)
    {
        CExoArrayList<unsigned int> feats;
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddGrantedFeats(&feats, 0);
        CSWGuiSkillInfoBox* pBox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSkillInfoBoxPanel;
        pBox->SetSkillList(feats, CSWGuiInGameSkillEntry::ENTRY_FEAT);
        pBox->m_lblMessage.m_text.m_params.SetStrRef(STRREF_GRANTED_FEATS);
        if (m_bHasGrantedPowers != 0)
            pBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiLevelUpPanel::ShowGrantedPowers));
        m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(pBox), 1, 1);
    }

    if (m_bHasGrantedPowers != 0 && m_bHasGrantedFeats == 0)
    {
        CExoArrayList<unsigned int> powers;
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddGrantedPowers(&powers);
        CSWGuiSkillInfoBox* pBox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSkillInfoBoxPanel;
        pBox->SetSkillList(powers, CSWGuiInGameSkillEntry::ENTRY_POWER);
        pBox->m_lblMessage.m_text.m_params.SetStrRef(STRREF_GRANTED_POWERS);
        m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(pBox), 1, 1);
    }
#endif
}

// Gates the panel's back-out: the cancel button follows the argument's low bit, and back is enabled
// (with its caption coloured) when either the argument is set or a child page is still open.
void CSWGuiLevelUpPanel::SetCanCancel(int bCanCancel)
{
    m_bCanCancel = bCanCancel;

    // A bitwise or, not a short-circuit, so both terms are always evaluated.
    int bEnabled = (m_nCurrentPage != m_nFirstPage) | (bCanCancel != 0);
    // The qualified call is deliberate, keeping this one non-virtual.
    m_btnBack.CSWGuiButton::SetEnabled(bEnabled);
#if KOTOR1
    m_btnBack.m_text.m_params.SetColor(bEnabled ? CGuiInGame::COLOR_BLUE
                                                   : CGuiInGame::DISABLED_TEXT);
#else
    m_btnBack.m_text.m_params.SetColor(bEnabled ? CGuiInGame::COLOR_NEW_TEXT
                                                   : CGuiInGame::DISABLED_TEXT);
#endif
    m_btnCancel.m_tail.bVisible = (m_bCanCancel & 1);
}

CSWGuiLevelUpPanel::~CSWGuiLevelUpPanel()
{
}

// Builds the widget wall (5 pages, 5 page titles, 7 buttons) and sets up the tab strip. Seeds the
// panel state and resolves the creature, loads the layout and names every page tab and title in one
// loop (all names 1-based), works out which pages are selectable, then binds the two tail buttons.
// Page 4 is the accept/summary page and is always live. Abilities open on every fourth level,
// skills on the pool the level grant seeded, feats on CalcNumberFeats, powers on
// ComputeNumberKnownSpellsLeft.
CSWGuiLevelUpPanel::CSWGuiLevelUpPanel(CSWGuiManager* pGuiManager, unsigned long long oid)
    : CSWGuiPanel(pGuiManager)
{
    CExoString sTmp;
#if !KOTOR1
    m_bHasGrantedFeats = 0;
    m_bHasGrantedPowers = 0;
#endif

    m_pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
    m_pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(oid);
    m_bCanCancel = 0;

#if KOTOR1
    StartLoadFromLayout(CResRef("LEVELUPPNL"));
    InitControl(m_lblBg, CExoString("LBL_BG"), 1);
#else
    StartLoadFromLayout(CResRef("LEVELUPPNL_p"), 1, 1, 1);
#endif

    int nCount = 0;
    for (int i = 0; i < 5; i++)
    {
        sTmp.Format(const_cast<char*>("LBL_%d"), i + 1);
        InitControl(m_aPages[i], sTmp, 1, 1, 0, 0, 0);
        m_aPages[i].m_hilightBorder.m_params.SetPulsingAlpha(1, 0, 1);

        sTmp.Format(const_cast<char*>("BTN_STEPNAME%d"), i + 1);
        InitControl(m_aButtons[i], sTmp, 1, 1, 0, 0, 0);

        sTmp.Format(const_cast<char*>("LBL_NUM%d"), i + 1);
        InitControl(m_aLabels[i], sTmp, 1, 1, 0, 0, 0);
    }

    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);
    InitControl(m_btnCancel, CExoString("BTN_CANCEL"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
#if KOTOR1
    ScaleExtentForResolution();
#ifndef K2_UI_TOUCH
    {
        // The steps sit on the 4:3 char-gen page behind them, centred in the slot lbl_cg_mcgbk
        // leaves blank for them.
        CSWGuiExtent extent = m_extent;
        extent.x += (g_SDL_mode.w - static_cast<int>(static_cast<float>(g_SDL_mode.h) / 768.0f * 1024.0f)) / 2;
        SetExtent(extent);
    }
#endif
#endif


    // Abilities open on every fourth level, where the ability bump lands.
    if ((m_pCreature->m_pStats->GetLevel() & 3) != 0)
    {
        m_aButtons[LEVELUP_STEP_ABILITIES].m_tail.bVisible = 0;
        m_aPages[LEVELUP_STEP_ABILITIES].m_tail.bVisible = 0;
        m_aLabels[LEVELUP_STEP_ABILITIES].m_tail.bVisible = 0;
        m_anState[LEVELUP_STEP_ABILITIES] = 0;
    }
    else
    {
        m_aButtons[LEVELUP_STEP_ABILITIES].AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiLevelUpPanel::OnSelectAbilitiesButton));
        m_aButtons[LEVELUP_STEP_ABILITIES].AddEvent(INPUTEVENT_GUI_START, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiLevelUpPanel::OnSelectAbilitiesButton));
        nCount++;
        m_anState[LEVELUP_STEP_ABILITIES] = 1;
        m_aLabels[LEVELUP_STEP_ABILITIES].m_text.m_params.SetText(CExoString(nCount));
    }

    // Skills open only if this level actually granted points.
    m_pCreature->m_pStats->m_nSkillPointsRemaining = 0;
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddSkillPointsPerLevel();
    unsigned short nSkillPoints = m_pCreature->m_pStats->m_nSkillPointsRemaining;
    if (nSkillPoints != 0)
    {
        m_aButtons[LEVELUP_STEP_SKILLS].AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiLevelUpPanel::OnSelectSkillsButton));
        m_aButtons[LEVELUP_STEP_SKILLS].AddEvent(INPUTEVENT_GUI_START, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiLevelUpPanel::OnSelectSkillsButton));
        nCount++;
        m_anState[LEVELUP_STEP_SKILLS] = 1;
        m_aLabels[LEVELUP_STEP_SKILLS].m_text.m_params.SetText(CExoString(nCount));
    }
    else
    {
        m_aButtons[LEVELUP_STEP_SKILLS].m_tail.bVisible = 0;
        m_aPages[LEVELUP_STEP_SKILLS].m_tail.bVisible = 0;
        m_aLabels[LEVELUP_STEP_SKILLS].m_tail.bVisible = 0;
        m_anState[LEVELUP_STEP_SKILLS] = 0;
    }

    // Feats open only if this level has a slot (normal or bonus).
    unsigned char nNormal = 0;
    unsigned char nBonus = 0;
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->CalcNumberFeats(nNormal, nBonus);
    if (nNormal + nBonus == 0)
    {
        m_aButtons[LEVELUP_STEP_FEATS].m_tail.bVisible = 0;
        m_aPages[LEVELUP_STEP_FEATS].m_tail.bVisible = 0;
        m_aLabels[LEVELUP_STEP_FEATS].m_tail.bVisible = 0;
        m_anState[LEVELUP_STEP_FEATS] = 0;

        CExoArrayList<unsigned int> feats;
#if KOTOR1
        // K1 shows the granted feats here rather than deferring them to OnPanelAdded.
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddGrantedFeats(&feats);
        if (feats.m_count >= 1)
        {
            CSWGuiSkillInfoBox* pBox =
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSkillInfoBoxPanel;
            pBox->SetSkillList(feats, CSWGuiInGameSkillEntry::ENTRY_FEAT);
            pBox->m_lblMessage.m_text.m_params.SetStrRef(STRREF_GRANTED_FEATS);
            m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(pBox), 1, 1);
        }
#else
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddGrantedFeats(&feats, 0);
        if (feats.m_count >= 1)
            m_bHasGrantedFeats = 1;
#endif
    }
    else
    {
        m_aButtons[LEVELUP_STEP_FEATS].AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiLevelUpPanel::OnSelectFeatsButton));
        m_aButtons[LEVELUP_STEP_FEATS].AddEvent(INPUTEVENT_GUI_START, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiLevelUpPanel::OnSelectFeatsButton));
        nCount++;
        m_anState[LEVELUP_STEP_FEATS] = 1;
        m_aLabels[LEVELUP_STEP_FEATS].m_text.m_params.SetText(CExoString(nCount));
    }

    // Powers open only if a spell slot is left. The result is truncated to a byte, since only its
    // zero-ness matters.
    unsigned char nSpellsLeft =
        static_cast<unsigned char>(static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->ComputeNumberKnownSpellsLeft());
    if (nSpellsLeft != 0)
    {
        m_aButtons[LEVELUP_STEP_POWERS].AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiLevelUpPanel::OnSelectPowersButton));
        m_aButtons[LEVELUP_STEP_POWERS].AddEvent(INPUTEVENT_GUI_START, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiLevelUpPanel::OnSelectPowersButton));
        nCount++;
        m_anState[LEVELUP_STEP_POWERS] = 1;
        m_aLabels[LEVELUP_STEP_POWERS].m_text.m_params.SetText(CExoString(nCount));
    }
    else
    {
        m_aButtons[LEVELUP_STEP_POWERS].m_tail.bVisible = 0;
        m_aPages[LEVELUP_STEP_POWERS].m_tail.bVisible = 0;
        m_aLabels[LEVELUP_STEP_POWERS].m_tail.bVisible = 0;
        m_anState[LEVELUP_STEP_POWERS] = 0;

#if !KOTOR1
        CExoArrayList<unsigned int> powers;
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddGrantedPowers(&powers);
        if (powers.m_count >= 1)
            m_bHasGrantedPowers = 1;
#endif
    }

    // The accept/summary page is always live.
    m_aButtons[LEVELUP_STEP_ACCEPT].AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiLevelUpPanel::OnSelectAcceptButton));
    m_aButtons[LEVELUP_STEP_ACCEPT].AddEvent(INPUTEVENT_GUI_START, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiLevelUpPanel::OnSelectAcceptButton));
    nCount++;
    m_anState[LEVELUP_STEP_ACCEPT] = 1;
    m_aLabels[LEVELUP_STEP_ACCEPT].m_text.m_params.SetText(CExoString(nCount));

    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));

    m_btnCancel.m_tail.bFocusOnHover = 0;
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiLevelUpPanel::OnCancelPressed));

#if !KOTOR1  // K1: the back button carries no gamepad glyph
    AspyrAddButtonIcon(&m_btnBack, 'b');
#endif
}
