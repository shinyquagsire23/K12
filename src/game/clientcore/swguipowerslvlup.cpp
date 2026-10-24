#include "k2stdint.h"
#include "swguipowerslvlup.h"
#include "swgui.h"
#include "swguicontrol.h"
#include "swguidrawable.h"
#include "swguiingame.h"
#include "swguiingametutorialbox.h"
#include "swguiskillflowchart.h"
#include "swguileveluppanel.h"
#include "nwccreature.h"
#include "nwccreaturestats.h"
#include "nwrules.h"
#include "guiingame.h"
#include "clientexoapp.h"
#include "appmanager.h"
#include "nwscreature.h"
#include "exoresman.h"
#include "exotlktable.h"
#include "exolocstringinternal.h"
#include "Vector.h"

#define CEXO_ARRAYLIST_KEEP_ADDUNIQUE_OUT_OF_LINE
#include "CExoArrayListImpl.h"
#include "tutorial.h"

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#if KOTOR1
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#endif
#include "port/libcstring.h"
#include "port/k2qolapi.h"
#include "nwscriptconst.h"
#include "exoinput.h"
#include "strref.h"

// The index of a power in the list, or -1.
static int FindPowerIndex(CExoArrayList<unsigned int>& aList, unsigned int nPower)
{
    int nIndex = -1;
    for (int64_t i = 0; i < aList.m_count; i++)
    {
        if (aList.m_pArray[i] == nPower)
        {
            nIndex = static_cast<int>(i);
            break;
        }
    }
    return nIndex;
}

// The same scan with the bound test inside the body, which AddChosenPower needs.
static int FindPowerIndexInMutator(CExoArrayList<unsigned int>& aList, unsigned int nPower)
{
    int nIndex = -1;
    int64_t i = 0;
    while (aList.m_pArray[i] != nPower)
    {
        i++;
        if (aList.m_count == i)
            return nIndex;
    }
    return static_cast<int>(i);
}

// The power id the chart's cursor is on, or -1 when the chart is empty.
static int GetChartPowerId(CSWGuiSkillFlowChart& chart)
{
    if (chart.m_aFlows.m_count < 1)
        return -1;
    return chart.m_aFlows.m_pArray[chart.m_nSelectedFlow]->m_aSlots[chart.m_nSelectedSlot].m_nId;
}

// Binds the PWRLVLUP_p layout by name, builds the two pre-formatted caption strings out of the TLK,
// stands the description row up off the prototype item, hands the chart the server stats block, then
// lets InitiatePowers build the four power lists.
CSWGuiPowersLevelUp::CSWGuiPowersLevelUp(CSWGuiManager* pGuiManager, unsigned long long objectId)
    : CSWGuiPanel(pGuiManager)
{
    m_pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
    m_pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(objectId);
    m_bFlags = static_cast<unsigned char>((m_pCreature->GetServerCreature()->m_pStats->m_bIsPC & 1)
                               | (m_bFlags & ~POWERS_FLAG_IS_PC));
    m_nSelectedPower = 0xffffffff;

#if KOTOR1
    StartLoadFromLayout(CResRef("pwrlvlup"));
#else
    StartLoadFromLayout(CResRef("pwrlvlup_p"), 1, 1, 1);
#endif

    InitControl(m_lblTitle,        CExoString("MAIN_TITLE_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblSubTitle,     CExoString("SUB_TITLE_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblRemainingBox, CExoString("REMAINING_BOX_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblSelections,   CExoString("REMAINING_SELECTIONS_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblSelRemaining, CExoString("SELECTIONS_REMAINING_LBL"), 1, 1, 0, 0, 0);
#if KOTOR1
    InitControl(m_lblDesc,         CExoString("DESC_LBL"), 1);
#endif
    InitControl(m_listBoxDesc,     CExoString("LB_DESC"), 1, 1, 0, 0, 0);
    m_listBoxDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_listBoxPowers,   CExoString("LB_POWERS"), 1, 1, 0, 0, 0);
    InitControl(m_lblPower,        CExoString("LBL_POWER"), 1, 1, 0, 0, 0);
#ifdef K2_QOL_IMPROVEMENTS
    K2QolMarkFitText(&m_lblPower.m_text);   // the selected power's name, above its description
#endif
    InitControl(m_btnRecommended,  CExoString("RECOMMENDED_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_btnSelect,       CExoString("SELECT_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_btnAccept,       CExoString("ACCEPT_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack,         CExoString("BACK_BTN"), 1, 1, 0, 0, 0);
#if KOTOR1  // K1 trades ACCEPT_BTN's and BACK_BTN's extents before the layout closes
    CSWGuiExtent extAccept = m_btnAccept.m_extent;
    CSWGuiExtent extBack = m_btnBack.m_extent;
    m_btnBack.SetExtent(extAccept);
    m_btnAccept.SetExtent(extBack);

    StopLoadFromLayout();
    ScaleExtentForResolution();
#else
    InitControl(m_aBars[0],         CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[1],         CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);

    StopLoadFromLayout();
#endif

    // Each of the two captions is a cost word, a space, then the shared tail word, fetched once.
    CExoString sTail = g_pTlkTable->GetSimpleString(STRREF_POWER);
    m_sPowerAvailableText = g_pTlkTable->GetSimpleString(STRREF_ADD) + CExoString(" ") + sTail;
    m_sPowerTakenText = g_pTlkTable->GetSimpleString(STRREF_REMOVE) + CExoString(" ") + sTail;

    // The description row takes the prototype's extent as-is, with no copy and no resolution
    // scaling, unlike the feats page.
    CSWGuiLabel* pProto = static_cast<CSWGuiLabel*>(m_listBoxDesc.m_pProtoItem);
    m_lblDescRow.Initialize(pProto->m_extent, pProto, 1.0f);

    m_powerChart.CreatePowerChart(m_pCreature->GetServerCreature()->m_pStats, 1);

    InitiatePowers();

    m_listBoxPowers.AddEvent(SWGUIEVENT_SELECTION_CHANGED, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPowersLevelUp::OnPowerSelectionChanged));
    m_listBoxPowers.AddEvent(SWGUIEVENT_DOUBLE_CLICK, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPowersLevelUp::OnDoubleClick));
    m_listBoxPowers.bSelectOnLeftClick = 1;

    m_btnRecommended.m_tail.bFocusOnHover = 0;
    m_btnRecommended.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnYButtonPressed));
    m_btnSelect.m_tail.bFocusOnHover = 0;
    m_btnSelect.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnXButtonPressed));
    m_btnAccept.m_tail.bFocusOnHover = 0;
    m_btnAccept.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnAButtonPressed));
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));

    SetActiveControl(&m_listBoxPowers, 0);

#if KOTOR1
    IosAddButtonIcon(&m_btnAccept, 'a');
    IosAddButtonIcon(&m_btnBack, 'b');
    IosAddButtonIcon(&m_btnRecommended, 'y');
    OnEnterPower(0);  // K1 describes the first power straight away
#else
    AspyrAddButtonIcon(&m_btnBack, 'b');
    AspyrAddButtonIcon(&m_btnRecommended, 'y');
    AspyrAddButtonIcon(&m_btnAccept, 'a');
#endif
}

CSWGuiPowersLevelUp::~CSWGuiPowersLevelUp()
{
    m_listBoxDesc.ClearItems();
    m_listBoxPowers.ClearItems();
}


// The level-up driver can ask for a tutorial pop-up first, keyed off the granted-box bit that
// InitiatePowers sets when it pushes the granted-powers box. If it is up, the granted list reappears
// through the box's own callback instead of the power-info pane.
void CSWGuiPowersLevelUp::OnPanelAdded()
{
    CSWGuiPanel::OnPanelAdded();

#if KOTOR1  // K1's tutorial pop-up is unconditional and there is no granted-powers box
    g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_ENTER_FORCE_POWER_SCREEN_CHAR_GEN, OBJECT_INVALID, OBJECT_INVALID, 0);
#else
    if ((m_bGrantedPushed & POWERS_GRANTED_BOX_PUSHED) == 0)
        return;

    if (g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_ENTER_FORCE_POWER_SCREEN_CHAR_GEN, OBJECT_INVALID, OBJECT_INVALID, 0) != 0)
    {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pTutorialBox->SetCallback(this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPowersLevelUp::ShowGranted), 0);
        return;
    }

    m_pGuiManager->AddPanel(
        static_cast<CSWGuiPanel*>(g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSkillInfoBoxPanel), 1, 1);
#endif
}

#if !KOTOR1
// The granted-powers pane's callback.
void CSWGuiPowersLevelUp::ShowGranted(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    m_pGuiManager->AddPanel(
        static_cast<CSWGuiPanel*>(g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSkillInfoBoxPanel), 1, 1);
}
#endif

// A click in the powers list. The row is a flow-chart widget, and the hit test comes from the list
// box's own drag-state words rather than from the control handed in.
void CSWGuiPowersLevelUp::OnPowerSelectionChanged(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
#if KOTOR1  // K1 guards on a selected row
    CSWGuiSkillFlow* pFlow = static_cast<CSWGuiSkillFlow*>(m_listBoxPowers.GetSelectedControl());
    if (pFlow == 0)
        return;
    unsigned int nPower =
        static_cast<unsigned int>(pFlow->SkillHitCheckMouse(m_listBoxPowers.m_nHitLocalX, m_listBoxPowers.m_nHitLocalY));
#else
    unsigned int nPower =
        static_cast<unsigned int>(static_cast<CSWGuiSkillFlow*>(m_listBoxPowers.GetSelectedControl())
            ->SkillHitCheckMouse(m_listBoxPowers.m_nHitLocalX, m_listBoxPowers.m_nHitLocalY));
#endif
    m_powerChart.SetSelectedSkill(nPower);
    OnEnterPower(static_cast<unsigned int>(nPower & 0xffff));
}

// A double click commits the row, but only when the chart's cursor and the hit test agree.
void CSWGuiPowersLevelUp::OnDoubleClick(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    int nChartId = GetChartPowerId(m_powerChart);

#if KOTOR1  // K1 guards on a selected row
    CSWGuiSkillFlow* pFlow = static_cast<CSWGuiSkillFlow*>(m_listBoxPowers.GetSelectedControl());
    if (pFlow == 0)
        return;
    int nHit = pFlow->SkillHitCheckMouse(m_listBoxPowers.m_nHitLocalX, m_listBoxPowers.m_nHitLocalY);
#else
    int nHit = static_cast<CSWGuiSkillFlow*>(m_listBoxPowers.GetSelectedControl())
                   ->SkillHitCheckMouse(m_listBoxPowers.m_nHitLocalX, m_listBoxPowers.m_nHitLocalY);
#endif
    if (nHit == nChartId)
        OnPowerPicked(static_cast<unsigned int>(nChartId & 0xffff));
}

// Rebuilds everything from the creature's current stats: the owned list out of every class's
// known-spell row, insert-sorted by name; the granted pool minus anything already owned; the
// selections-left budget; and the available column with the button states. Unlike the feats page
// there is no cleanup equivalent, because the powers page never wipes the creature's spell tables.
void CSWGuiPowersLevelUp::InitiatePowers()
{
    CExoString sPowerName;

    CSWCLevelUpStats* pStats = static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats);

    for (unsigned char nClass = 0; nClass < pStats->m_nNumClasses; nClass++)
    {
        for (unsigned char i = 0;
             i < pStats->GetNumberKnownSpells(nClass, 0); i++)
        {
            unsigned int nPower = static_cast<unsigned int>(pStats->GetKnownSpell(nClass, 0, i));
            sPowerName = g_pRules->m_pSpellArray->GetSpell(static_cast<int>(nPower))->GetSpellNameText();

            int nInsert = 0;
            int bFound = 0;
            while (static_cast<int64_t>(m_aOwnedPowers.m_count) > static_cast<int64_t>(nInsert) && !bFound)
            {
                CExoString sExistingName =
                    g_pRules->m_pSpellArray->GetSpell(static_cast<int>(m_aOwnedPowers.m_pArray[nInsert]))
                        ->GetSpellNameText();
                if (sExistingName > sPowerName)
                    bFound = 1;
                else
                    nInsert++;
            }
            if (bFound)
                m_aOwnedPowers.Insert(nPower, nInsert);
            else
                m_aOwnedPowers.Add(nPower);
        }
    }

#if !KOTOR1
    m_bGrantedPushed = static_cast<unsigned char>(m_bGrantedPushed & ~POWERS_GRANTED_BOX_PUSHED);

    CExoArrayList<unsigned int> aGranted;
    pStats->AddGrantedPowers(&aGranted);

    if (aGranted.m_count > 0)
    {
        for (int64_t i = 0; i < aGranted.m_count; i++)
            m_aNewPowers.Add(aGranted.m_pArray[i]);

        // A granted power the creature already owns is not new: drop it from the granted list too.
        int nGranted = aGranted.m_count;
        while (0 < nGranted)
        {
            nGranted--;
            unsigned int nPower = aGranted.m_pArray[nGranted];
            if (m_aOwnedPowers.Contains(nPower) != 0)
            {
                aGranted.DelIndex(nGranted);
            }
        }

        CSWGuiSkillInfoBox* pInfoBox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSkillInfoBoxPanel;
        pInfoBox->SetSkillList(aGranted, CSWGuiInGameSkillEntry::ENTRY_POWER);
        pInfoBox->m_lblMessage.m_text.m_params.SetStrRef(STRREF_GRANTED_POWERS);
        m_bGrantedPushed = static_cast<unsigned char>(m_bGrantedPushed | POWERS_GRANTED_BOX_PUSHED);
    }
#endif

    m_nSelectionsLeft = static_cast<unsigned char>(pStats->ComputeNumberKnownSpellsLeft());
    m_nSelectionsMax = static_cast<unsigned char>(m_nSelectionsLeft + 1);

    BuildAvailableList();
    BuildButtons();
}

// Walks the whole spell table and adds every row the creature may still take: not already in any of
// the four lists, CanChooseSpell agrees, the row is not the -2 marker and it has a name.
void CSWGuiPowersLevelUp::BuildAvailableList()
{
    CExoString sUnused;

    if (g_pRules->m_pSpellArray->m_nCount < 1)
        return;

    CSWCLevelUpStats* pStats = static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats);

    for (unsigned short nSpell = 0; static_cast<int>(g_pRules->m_pSpellArray->m_nCount) > static_cast<int>(nSpell); nSpell++)
    {
        if (FindPowerIndex(m_aOwnedPowers, nSpell) != -1)
            continue;
        if (FindPowerIndex(m_aChosenPowers, nSpell) != -1)
            continue;
        if (FindPowerIndex(m_aAvailablePowers, nSpell) != -1)
            continue;
#if !KOTOR1  // K1 has no granted-powers list
        if (FindPowerIndex(m_aNewPowers, nSpell) != -1)
            continue;
#endif

        if (pStats->CanChooseSpell(nSpell, m_nSelectionsMax, &m_aChosenPowers) != 0)
        {
            CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(nSpell);
            if (pSpell->m_anPrereq2[0] != -2 && pSpell->m_nNameStrRef != 0)
            {
                sUnused = pSpell->GetSpellNameText();

                int nInsert = 0;
                int bFound = 0;
                while (static_cast<int64_t>(m_aAvailablePowers.m_count) > static_cast<int64_t>(nInsert) && !bFound)
                {
                    CExoString sExistingName =
                        g_pRules->m_pSpellArray->GetSpell(
                            static_cast<int>(m_aAvailablePowers.m_pArray[nInsert]))->GetSpellNameText();
                    if (sExistingName > sUnused)
                        bFound = 1;
                    else
                        nInsert++;
                }
                if (bFound)
                    m_aAvailablePowers.Insert(nSpell, nInsert);
                else
                    m_aAvailablePowers.Add(nSpell);
            }
        }
    }
}

// Paints the chart from the four lists, hands it to the powers list on the first build, then repaints
// the selections-left caption.
void CSWGuiPowersLevelUp::BuildButtons()
{
    m_powerChart.SetSkillStatusAll(CSWGuiSkillFlow::SKILL_STATUS_UNAVAILABLE);

    for (int64_t i = 0; i < m_aOwnedPowers.m_count; i++)
        m_powerChart.SetSkillStatus(m_aOwnedPowers.m_pArray[i], CSWGuiSkillFlow::SKILL_STATUS_OWNED);
    for (int64_t i = 0; i < m_aChosenPowers.m_count; i++)
        m_powerChart.SetSkillStatus(m_aChosenPowers.m_pArray[i], CSWGuiSkillFlow::SKILL_STATUS_CHOSEN);
    for (int64_t i = 0; i < m_aAvailablePowers.m_count; i++)
    {
        // The status argument comes from a three-way if chain rather than from `&&`/`||` over the flags:
        // the short-circuit spelling folds the whole boolean into a single argument and costs 12 bytes.
        unsigned int nPower = m_aAvailablePowers.m_pArray[i];
        unsigned char nStatus;
        if (m_bFlags & POWERS_FLAG_IS_PC)
            nStatus = CSWGuiSkillFlow::SKILL_STATUS_AVAILABLE;
        else if ((nPower | (FORCE_POWER_DOMINATE - FORCE_POWER_AFFECT_MIND)) == FORCE_POWER_DOMINATE)
            nStatus = CSWGuiSkillFlow::SKILL_STATUS_UNAVAILABLE;
        else
            nStatus = CSWGuiSkillFlow::SKILL_STATUS_AVAILABLE;
        m_powerChart.SetSkillStatus(nPower, nStatus);
    }
#if !KOTOR1  // K1 has no granted-powers list
    for (int64_t i = 0; i < m_aNewPowers.m_count; i++)
        m_powerChart.SetSkillStatus(m_aNewPowers.m_pArray[i], CSWGuiSkillFlow::SKILL_STATUS_GRANTED);
#endif

    if (m_nSelectedPower == 0xffffffff)
    {
        m_listBoxPowers.AddControls(reinterpret_cast<CExoArrayList<CSWGuiControl*>&>(m_powerChart.m_aFlows), 0, 1, 0);
        m_nSelectedPower = static_cast<unsigned int>(m_powerChart.SetTopLeft());
    }

    m_powerChart.SetSelectedSkill(m_nSelectedPower);
    m_listBoxPowers.SetSelectedControl(static_cast<int>(m_powerChart.m_nSelectedFlow), 0);
    OnEnterPower(m_nSelectedPower);

    m_lblSelections.m_text.m_params.SetText(CExoString(m_nSelectionsLeft));
}

// A: accept. B: cancel. X: commit the focused row. Y: recommend. The eight arrow and trigger codes
// go to the chart, whose hit result re-focuses a row. Everything else, and every release, falls
// through to the base, which is why the base call sits after the switch.
void CSWGuiPowersLevelUp::HandleInputEvent(int nEvent, int nArg)
{
    if (nArg != 0)
    {
        switch (nEvent)
        {
        case INPUTEVENT_GUI_A:
        case INPUTEVENT_GUI_START:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            OnAcceptButton();
            break;

        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            HandleCancelButton();
            break;

        case INPUTEVENT_GUI_X:
            OnPowerPicked(m_powerChart.m_aFlows.m_count < 1
                              ? 0xffffffff
                              : static_cast<unsigned int>(m_powerChart.m_aFlows
                                    .m_pArray[m_powerChart.m_nSelectedFlow]
                                    ->m_aSlots[m_powerChart.m_nSelectedSlot]
                                    .m_nId));
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            break;

        case INPUTEVENT_GUI_Y:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            OnRecommendButton();
            break;

        case INPUTEVENT_GUI_DPAD_LEFT:
        case INPUTEVENT_GUI_DPAD_RIGHT:
        case INPUTEVENT_GUI_DPAD_UP:
        case INPUTEVENT_GUI_DPAD_DOWN:
        case INPUTEVENT_GUI_KEY_UP:
        case INPUTEVENT_GUI_KEY_DOWN:
        case INPUTEVENT_GUI_KEY_LEFT:
        case INPUTEVENT_GUI_KEY_RIGHT:
            OnEnterPower(static_cast<unsigned int>(m_powerChart.HandleInput(nEvent)));
            break;

        case INPUTEVENT_GUI_SCROLL_UP:
            m_listBoxDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
            break;

        case INPUTEVENT_GUI_SCROLL_DOWN:
            m_listBoxDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
            break;
        }
    }

    CSWGuiPanel::HandleInputEvent(nEvent, nArg);
}

// B: throws the page away. Unlike the feats page there is no creature-side cleanup, because the
// powers page never owned the stats' spell tables.
void CSWGuiPowersLevelUp::HandleCancelButton()
{
    if (m_pParentPanel == 0)
        return;

    m_pGuiManager->PopModalPanel();
    m_pParentPanel->SetVisible(1);

    CSWGuiPanel* pPanel = this;
#if KOTOR1  // K1 has no panel chain: one state stamp for this panel only
    pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
    do
    {
        pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
        pPanel = pPanel->m_pNextInChain;
    } while (pPanel != 0);
#endif

    static_cast<CSWGuiLevelUpPanel*>(m_pParentPanel)->CancelledByChild();
}

// A: with selections still unspent, asks first; otherwise commits. The commit learns every chosen
// power into the creature's last class, tears the page down and tells the owner.
void CSWGuiPowersLevelUp::OnAcceptButton()
{
    if (m_nSelectionsLeft != 0 && m_aAvailablePowers.m_count > 0)
    {
        CSWGuiMessageBox* pBox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetCallback(0, 0, 0);
        pBox->SetMessage(STRREF_MUST_SELECT_NEW_POWERS);
        m_pGuiManager->AddPanel(pBox, 1, 1);
        return;
    }

    if (m_aChosenPowers.m_count > 0)
    {
        unsigned char nClass = static_cast<unsigned char>(m_pCreature->m_pStats->m_nNumClasses - 1);
        for (int i = 0; i < m_aChosenPowers.m_count; i++)
            m_pCreature->m_pStats->AddKnownSpell(nClass, m_aChosenPowers.m_pArray[i]);
    }

    m_pGuiManager->PopModalPanel();
    m_pParentPanel->SetVisible(1);

    CSWGuiPanel* pPanel = this;
#if KOTOR1  // K1 has no panel chain: one state stamp for this panel only
    pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
    do
    {
        pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
        pPanel = pPanel->m_pNextInChain;
    } while (pPanel != 0);
#endif

    static_cast<CSWGuiLevelUpPanel*>(m_pParentPanel)->PowersSelectionCompleted();
}

// Toggles the focused power on X or a double click. A row in the available list becomes chosen,
// refused outright when nothing is left to spend; a row in the chosen list goes back; the other two
// lists explain themselves through the query box.
void CSWGuiPowersLevelUp::OnPowerPicked(unsigned int nPower)
{
    uint64_t nStrRef;

    if (FindPowerIndex(m_aOwnedPowers, nPower) != -1)
    {
        nStrRef = STRREF_CANNOT_DESELECT_OWNED_POWER;
    }
    else if (FindPowerIndex(m_aChosenPowers, nPower) != -1)
    {
        RemoveChosenPower(nPower);
        BuildButtons();
        return;
    }
    else if (FindPowerIndex(m_aAvailablePowers, nPower) != -1)
    {
        if ((nPower | (FORCE_POWER_DOMINATE - FORCE_POWER_AFFECT_MIND)) == FORCE_POWER_DOMINATE
            && (m_bFlags & POWERS_FLAG_IS_PC) == 0)
        {
            nStrRef = STRREF_ONLY_MAIN_CHARACTER_CAN_SELECT;
        }
        else if (m_nSelectionsLeft != 0)
        {
            AddChosenPower(nPower);
            BuildButtons();
            return;
        }
        else
        {
            nStrRef = STRREF_ALL_POWERS_SELECTED;
        }
    }
    else
    {
        // The strref pair is picked with a select rather than a branch.
#if KOTOR1
        nStrRef = STRREF_POWER_PREREQUISITES_NOT_MET;
#else
        nStrRef = FindPowerIndex(m_aNewPowers, nPower) != -1 ? STRREF_POWER_GRANTED_AT_LEVEL : STRREF_POWER_PREREQUISITES_NOT_MET;
#endif
    }

    CSWGuiMessageBox* pBox =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    pBox->SetAllowCancel(0);
    pBox->SetCallback(0, 0, 0);
    pBox->SetMessage(nStrRef);
    m_pGuiManager->AddPanel(pBox, 1, 1);
}

// Y: rebuilds the page from the character's own class recommendation. A class's suggestion for a
// power build is its default package, so the body empties the chosen list, lets the package grant
// its powers, adds each one, then repaints.
void CSWGuiPowersLevelUp::OnRecommendButton()
{
    CExoArrayList<unsigned int> aGranted;

    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddDefaultPackageSpells(&aGranted);

    while (m_aChosenPowers.m_count != 0)
        RemoveChosenPower(m_aChosenPowers.m_pArray[0]);

    for (int64_t i = 0; i < aGranted.m_count; i++)
        AddChosenPower(aGranted.m_pArray[i]);

    CSWGuiSkillInfoBox* pInfoBox =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSkillInfoBoxPanel;
    pInfoBox->SetSkillList(aGranted, CSWGuiInGameSkillEntry::ENTRY_POWER);
    pInfoBox->m_lblMessage.m_text.m_params.SetStrRef(STRREF_RECOMMENDED_POWERS);
#if !KOTOR1  // K1 adds the box without removing an existing copy first
    if (m_pGuiManager->PanelExists(static_cast<CSWGuiPanel*>(pInfoBox)) != 0)
        m_pGuiManager->RemovePanel(static_cast<CSWGuiPanel*>(pInfoBox));
#endif
    m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(pInfoBox), 1, 1);

    BuildButtons();
}

// Takes a power back out of the chosen list, shifts the tail down and refunds a selection; then
// re-validates what is left, because any chosen power whose prerequisites the shrunken list no longer
// meets is refunded too, which is why the outer loop restarts each trip.
void CSWGuiPowersLevelUp::RemoveChosenPower(unsigned int nPower)
{
    int nCount = m_aChosenPowers.m_count;
    if (nCount < 1)
        return;

    for (;;)
    {
        int nIndex = FindPowerIndex(m_aChosenPowers, nPower);
        if (nIndex == -1)
            return;

        m_aChosenPowers.DelIndex(nIndex);
        nCount = m_aChosenPowers.m_count;

        m_nSelectionsLeft = static_cast<unsigned char>(m_nSelectionsLeft + 1);

        if (nCount >= 1)
        {
            // The first chosen power whose prerequisites no longer hold is the next to refund.
            CSWCLevelUpStats* pStats = static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats);
            unsigned int nBad;
            if (pStats->HasSpellPrereq(
                    g_pRules->m_pSpellArray->GetSpell(m_aChosenPowers.m_pArray[0]),
                    &m_aChosenPowers)
                == 0)
            {
                nBad = 0;
            }
            else
            {
                uint64_t i = 1;
                int nOk;
                do
                {
                    if (static_cast<int64_t>(m_aChosenPowers.m_count) <= static_cast<int64_t>(i))
                    {
                        m_aAvailablePowers.m_count = 0;
                        BuildAvailableList();
                        return;
                    }
                    unsigned int nCur = m_aChosenPowers.m_pArray[i];
                    nOk = pStats->HasSpellPrereq(g_pRules->m_pSpellArray->GetSpell(nCur),
                                                 &m_aChosenPowers);
                    i++;
                } while (nOk != 0);
                nBad = static_cast<unsigned int>(i - 1);
            }

            nPower = m_aChosenPowers.m_pArray[nBad];
            nCount = m_aChosenPowers.m_count;
        }
        else
        {
            m_aAvailablePowers.m_count = 0;
            BuildAvailableList();
            return;
        }
    }
}

// Moves a power out of the available list and into the chosen one, insertion-sorted by name, and
// spends a selection. That sort is what makes the right column read alphabetically.
void CSWGuiPowersLevelUp::AddChosenPower(unsigned int nPower)
{
    int nCount = m_aAvailablePowers.m_count;
    if (nCount < 1)
        return;

    int nIndex = FindPowerIndexInMutator(m_aAvailablePowers, nPower);
    if (nIndex == -1)
        return;

    m_aAvailablePowers.DelIndex(nIndex);

    // The new power belongs in the chosen column at the first row whose name sorts after it.
    CExoString sNewName = g_pRules->m_pSpellArray->GetSpell(static_cast<int>(nPower))->GetSpellNameText();

    int bFound = 0;
    int nInsert;
    for (nInsert = 0; !bFound && nInsert < m_aChosenPowers.m_count; nInsert++)
    {
        CExoString sExistingName =
            g_pRules->m_pSpellArray->GetSpell(m_aChosenPowers.m_pArray[nInsert])
                ->GetSpellNameText();
        bFound = sExistingName > sNewName;
    }

    m_nSelectionsLeft = static_cast<unsigned char>(m_nSelectionsLeft - 1);

    if (bFound)
        m_aChosenPowers.Insert(nPower, nInsert - 1);
    else
        m_aChosenPowers.Add(nPower);

    BuildAvailableList();
}

// Which of the four lists holds this power. The status byte doubles as the draw state: 2 owned,
// 0 available, 1 chosen, 4 new, 3 none.
void CSWGuiPowersLevelUp::DeterminePower(unsigned int nPower, unsigned char& nStatus)
{
    if (FindPowerIndex(m_aOwnedPowers, nPower) != -1)
    {
        nStatus = DRAW_STATE_OWNED;
        return;
    }
    if (FindPowerIndex(m_aChosenPowers, nPower) != -1)
    {
        nStatus = DRAW_STATE_CHOSEN;
        return;
    }
    if (FindPowerIndex(m_aAvailablePowers, nPower) != -1)
    {
        nStatus = DRAW_STATE_AVAILABLE;
        return;
    }
#if !KOTOR1  // K1 has no granted-powers list
    if (FindPowerIndex(m_aNewPowers, nPower) != -1)
    {
        nStatus = DRAW_STATE_GRANTED;
        return;
    }
#endif
    nStatus = DRAW_STATE_UNAVAILABLE;
}

// Shows a power description: retargets the row's text, clears the box, resizes the row to the
// string's own ideal height and re-adds it as the box's single selected row.
void CSWGuiPowersLevelUp::SetDescription(unsigned int nStrRef)
{
    STR_RES res;
    g_pTlkTable->Fetch(nStrRef, res, CTlkTable::GENDER_MALE);
    m_lblDescRow.m_text.m_params.SetText(res.text);

    m_listBoxDesc.ClearItems();

    // Through a pointer on purpose, since a direct call on the member would be devirtualized.
    CSWGuiText* pText = &m_lblDescRow.m_text;

    // One store to the row height here, and no scaling by the resolution text scalar, unlike the
    // feats page.
    CSWGuiExtent extent = m_lblDescRow.m_extent;
#if KOTOR1  // K1 uses the out-of-line length accessor
    if (res.text.GetLength() >= 1)
#else
    if (res.text.m_sString && static_cast<int>(strlen(res.text.m_sString)) >= 1)
#endif
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
    m_lblDescRow.SetExtent(extent);

    // A separate one-element array rather than a scalar, so the two calls below get their own cleanup
    // ranges; a scalar local merges them and comes out 4 bytes short.
    CSWGuiControl* apRows[1] = { &m_lblDescRow };
    m_listBoxDesc.AddControls(apRows, 1, 0, 0, 0);
    m_listBoxDesc.SetSelectedControl(&m_lblDescRow, 0);
}

// Draws the page. The select button's caption follows the focused row's status: a granted or new row
// shows the "already have it" string, a "none" row the plain one, and both grey the icon.
void CSWGuiPowersLevelUp::Draw(float f)
{
#if KOTOR1  // K1 has no "new" state and no glyph; the caption is greyed with DISABLED_TEXT
    if (m_nDrawState == DRAW_STATE_OWNED)
    {
        m_btnSelect.m_text.m_params.SetText(m_sPowerTakenText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        m_btnSelect.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
    }
    else if (m_nDrawState == DRAW_STATE_UNAVAILABLE)
    {
        m_btnSelect.m_text.m_params.SetText(m_sPowerAvailableText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        m_btnSelect.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
    }
#else
    if (m_nDrawState == DRAW_STATE_OWNED || m_nDrawState == DRAW_STATE_GRANTED)
    {
        m_btnSelect.m_text.m_params.SetText(m_sPowerTakenText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_GREEN);
        m_btnSelect.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
        AspyrAddButtonIcon(&m_btnSelect, 'x');
    }
    else if (m_nDrawState == DRAW_STATE_UNAVAILABLE)
    {
        m_btnSelect.m_text.m_params.SetText(m_sPowerAvailableText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_GREEN);
        m_btnSelect.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
    }
#endif

    CSWGuiPanel::Draw(f);
}

// Focuses a power row: classifies it, retitles and recolours the select button accordingly, then
// hands the power's name strref and description to the readout column.
void CSWGuiPowersLevelUp::OnEnterPower(unsigned int nPower)
{
    // The member is the classifier's out-parameter, so the store lands in the object rather than in a
    // temporary.
    DeterminePower(nPower, m_nDrawState);

    m_nSelectedPower = nPower;

    CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(static_cast<int>(nPower));

#if KOTOR1  // K1 uses the global blue caption and the enabled icon colour for takeable and taken
             // rows, DISABLED_TEXT for the rest, and re-adds the 'x' glyph after the readout
    switch (m_nDrawState)
    {
    case DRAW_STATE_AVAILABLE:
        m_btnSelect.m_text.m_params.SetText(m_sPowerAvailableText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
        m_btnSelect.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::ENABLED_ICON_COLOR);
        break;

    case DRAW_STATE_CHOSEN:
        m_btnSelect.m_text.m_params.SetText(m_sPowerTakenText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
        m_btnSelect.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::ENABLED_ICON_COLOR);
        break;

    case DRAW_STATE_OWNED:
        m_btnSelect.m_text.m_params.SetText(m_sPowerTakenText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        m_btnSelect.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
        break;

    case DRAW_STATE_UNAVAILABLE:
        m_btnSelect.m_text.m_params.SetText(m_sPowerAvailableText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        m_btnSelect.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
        break;
    }

    m_lblPower.m_text.m_params.SetStrRef(pSpell->m_nNameStrRef);
    SetDescription(pSpell->m_nDescriptionStrRef);
    IosAddButtonIcon(&m_btnSelect, 'x');
#else
    switch (m_nDrawState)
    {
    case DRAW_STATE_AVAILABLE:
        m_btnSelect.m_text.m_params.SetText(m_sPowerAvailableText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        m_btnSelect.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
        AspyrAddButtonIcon(&m_btnSelect, 'x');
        break;

    case DRAW_STATE_CHOSEN:
        m_btnSelect.m_text.m_params.SetText(m_sPowerTakenText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        m_btnSelect.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
        AspyrAddButtonIcon(&m_btnSelect, 'x');
        break;

    case DRAW_STATE_OWNED:
    case DRAW_STATE_GRANTED:
        m_btnSelect.m_text.m_params.SetText(m_sPowerTakenText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        m_btnSelect.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
        break;

    case DRAW_STATE_UNAVAILABLE:
        m_btnSelect.m_text.m_params.SetText(m_sPowerAvailableText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_GREEN);
        m_btnSelect.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
        break;
    }

    m_lblPower.m_text.m_params.SetStrRef(pSpell->m_nNameStrRef);
    SetDescription(pSpell->m_nDescriptionStrRef);
#endif
}
