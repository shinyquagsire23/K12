#include "k2stdint.h"
#include "swguifeatschargen.h"
#include "swgui.h"
#include "swguicontrol.h"
#include "swguidrawable.h"
#include "swguiingame.h"
#include "swguiingametutorialbox.h"
#include "swguiskillflowchart.h"
#include "swguicustompanel.h"
#include "swguileveluppanel.h"
#include "nwccreature.h"
#include "nwccreaturestats.h"
#include "nwrules.h"
#include "guiingame.h"
#include "clientexoapp.h"
#include "appmanager.h"
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
void IosAddStringIcon(CExoString& sText, char cIcon);
#endif
float GetResolutionTextScalar();
#include "port/libcstring.h"
#include "port/k2qolapi.h"
#include "nwscreaturestats.h"
#include "exoinput.h"
#include "strref.h"

// The index of nFeat in aList, or -1.
static int FindFeatIndex(CExoArrayList<unsigned short>& aList, unsigned short nFeat)
{
    int nIndex = -1;
    for (int64_t i = 0; i < aList.m_count; i++)
    {
        if (aList.m_pArray[i] == nFeat)
        {
            nIndex = static_cast<int>(i);
            break;
        }
    }
    return nIndex;
}

// The feat id the chart's cursor is on, or -1 when the chart is empty.
static int GetChartFeatId(CSWGuiSkillFlowChart& chart)
{
    if (chart.m_aFlows.m_count < 1)
        return -1;
    return chart.m_aFlows.m_pArray[chart.m_nSelectedFlow]->m_aSlots[chart.m_nSelectedSlot].m_nId;
}

// Binds the feats layout, builds the row captions out of the TLK, then hands the rest to
// InitiateFeats.
CSWGuiFeatsCharGen::CSWGuiFeatsCharGen(CSWGuiManager* pGuiManager, CSWCCreature* pCreature,
                                       int nMode, int nBonus)
    : CSWGuiPanel(pGuiManager)
{
    m_pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
    m_pCreature = pCreature;
    m_bFlags = static_cast<unsigned char>((m_bFlags & ~(FEATS_FLAG_LEVEL_UP | FEATS_FLAG_BONUS)) | (nMode & 1));
    m_bFlags = static_cast<unsigned char>(m_bFlags | ((nBonus & 1) << 1));
    m_nSelectedFeat = 0xffff;

#if KOTOR1
    StartLoadFromLayout(CResRef("FTCHRGEN"));
#else
    StartLoadFromLayout(CResRef("FTCHRGEN_p"), 1, 1, 1);
#endif

    InitControl(m_lblTitle,        CExoString("MAIN_TITLE_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblSubTitle,     CExoString("SUB_TITLE_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblRemainingBox, CExoString("REMAINING_BOX_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblSelections,   CExoString("STD_REMAINING_SELECTIONS_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblSelRemaining, CExoString("STD_SELECTIONS_REMAINING_LBL"), 1, 1, 0, 0, 0);
#if KOTOR1
    InitControl(m_lblDesc,         CExoString("DESC_LBL"), 1);
#endif
    InitControl(m_listBoxDesc,     CExoString("LB_DESC"), 1, 1, 0, 0, 0);
    m_listBoxDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_listBoxFeats,    CExoString("LB_FEATS"), 1, 1, 0, 0, 0);
    InitControl(m_lblName,         CExoString("LBL_NAME"), 1, 1, 0, 0, 0);
#ifdef K2_QOL_IMPROVEMENTS
    K2QolMarkFitText(&m_lblName.m_text);
#endif
    InitControl(m_btnAccept,       CExoString("BTN_ACCEPT"), 1, 1, 0, 0, 0);
    InitControl(m_btnRecommended,  CExoString("BTN_RECOMMENDED"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack,         CExoString("BTN_BACK"), 1, 1, 0, 0, 0);
    InitControl(m_btnSelect,       CExoString("BTN_SELECT"), 1, 1, 0, 0, 0);
#if KOTOR1
    // K1: BTN_ACCEPT and BTN_BACK trade extents before the layout closes.
    CSWGuiExtent extAccept = m_btnAccept.m_extent;
    CSWGuiExtent extBack = m_btnBack.m_extent;
    m_btnBack.SetExtent(extAccept);
    m_btnAccept.SetExtent(extBack);

    StopLoadFromLayout();
    ScaleExtentForResolution();
#else
    InitControl(m_aBars[0],       CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aBars[1],       CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);

    StopLoadFromLayout();
#endif

    // Each of the two caption strings is a cost word, a space, then the shared tail word (fetched
    // once).
    CExoString sTail = g_pTlkTable->GetSimpleString(STRREF_FEAT);
    m_sFeatAvailableText = g_pTlkTable->GetSimpleString(STRREF_ADD) + CExoString(" ") + sTail;
    m_sFeatTakenText = g_pTlkTable->GetSimpleString(STRREF_REMOVE) + CExoString(" ") + sTail;

    if (nMode != 0)
        m_lblTitle.m_text.m_params.SetStrRef(STRREF_LEVEL_UP);
    if (nBonus != 0)
        m_lblSubTitle.m_text.m_params.SetStrRef(STRREF_BONUS_FEATS);

    CSWGuiLabel* pProto = static_cast<CSWGuiLabel*>(m_listBoxDesc.m_pProtoItem);
#if KOTOR1
    m_lblDescRow.Initialize(pProto->m_extent, pProto, 1.0f);  // K1: no text-scale stretch
#else
    CSWGuiExtent extent = pProto->m_extent;
    extent.h = static_cast<int>(GetResolutionTextScalar() * static_cast<float>(extent.h));
    m_lblDescRow.Initialize(extent, pProto, 1.0f);
#endif

    InitiateFeats();

    // LB_FEATS is the page's own list: a selection change re-points the chart, a double click
    // commits the row.
    m_listBoxFeats.AddEvent(SWGUIEVENT_SELECTION_CHANGED, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiFeatsCharGen::OnFeatSelectionChanged));
    m_listBoxFeats.AddEvent(SWGUIEVENT_DOUBLE_CLICK, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiFeatsCharGen::OnDoubleClick));
    m_listBoxFeats.bSelectOnLeftClick = 1;

    m_btnAccept.m_tail.bFocusOnHover = 0;
    m_btnAccept.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnAButtonPressed));
    m_btnRecommended.m_tail.bFocusOnHover = 0;
    m_btnRecommended.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnYButtonPressed));
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnSelect.m_tail.bFocusOnHover = 0;
    m_btnSelect.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnXButtonPressed));

#if KOTOR1
    SetActiveControl(&m_listBoxFeats, 0);

    IosAddButtonIcon(&m_btnRecommended, 'y');
    IosAddButtonIcon(&m_btnAccept, 'a');
    IosAddButtonIcon(&m_btnBack, 'b');
    IosAddStringIcon(m_sFeatAvailableText, 'x');
    IosAddStringIcon(m_sFeatTakenText, 'x');
    OnEnterFeat(m_nSelectedFeat);
#else
    AspyrAddButtonIcon(&m_btnBack, 'b');
    // The recommended button is nudged wider on the non-tablet layout.
    if (!IsTablet())
    {
        extent = m_btnRecommended.m_extent;
        extent.w += 30;
        m_btnRecommended.SetExtent(extent);
    }
    AspyrAddButtonIcon(&m_btnRecommended, 'y');
    AspyrAddButtonIcon(&m_btnAccept, 'a');

    SetActiveControl(&m_listBoxFeats, 0);
#endif
}

CSWGuiFeatsCharGen::~CSWGuiFeatsCharGen()
{
    m_listBoxDesc.ClearItems();
    m_listBoxFeats.ClearItems();
}


// The level-up driver can ask for a tutorial pop-up first (state bit 2, set by InitiateFeats when
// it pushed the granted-feats box). While it is up, the granted list reappears through the box's own
// callback instead of the skill-info pane.
void CSWGuiFeatsCharGen::OnPanelAdded()
{
    CSWGuiPanel::OnPanelAdded();

    if (((m_bFlags >> 2) & 1) == 0)
        return;

    if (g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_ENTER_FEAT_SCREEN_CHAR_GEN, OBJECT_INVALID, OBJECT_INVALID, 0) != 0)
    {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pTutorialBox->SetCallback(this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiFeatsCharGen::ShowGranted), 0);
        return;
    }

#if !KOTOR1
    if ((m_bFlags & FEATS_FLAG_LEVEL_UP) == 0)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 2, 0);
#endif
    m_pGuiManager->AddPanel(
        static_cast<CSWGuiPanel*>(g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSkillInfoBoxPanel), 1, 1);
}

// Shows the list the tutorial box was opened for (the granted-feats pane's own callback).
void CSWGuiFeatsCharGen::ShowGranted(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
#if !KOTOR1
    if ((m_bFlags & FEATS_FLAG_LEVEL_UP) == 0)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 2, 0);
#endif
    m_pGuiManager->AddPanel(
        static_cast<CSWGuiPanel*>(g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSkillInfoBoxPanel), 1, 1);
}

// A click in LB_FEATS. The hit test comes from the list box's own drag-state words, not from the
// control handed in.
void CSWGuiFeatsCharGen::OnFeatSelectionChanged(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
#if KOTOR1
    CSWGuiSkillFlow* pFlow = static_cast<CSWGuiSkillFlow*>(m_listBoxFeats.GetSelectedControl());
    if (pFlow == 0)
        return;
    unsigned int nFeat = pFlow->SkillHitCheckMouse(m_listBoxFeats.m_nHitLocalX, m_listBoxFeats.m_nHitLocalY);
#else
    unsigned int nFeat = static_cast<CSWGuiSkillFlow*>(m_listBoxFeats.GetSelectedControl())
                             ->SkillHitCheckMouse(m_listBoxFeats.m_nHitLocalX,
                                                  m_listBoxFeats.m_nHitLocalY);
#endif
    m_featChart.SetSelectedSkill(nFeat);
    OnEnterFeat(static_cast<unsigned short>(nFeat));
}

// A double click commits the row, but only when the chart's cursor and the hit test agree.
void CSWGuiFeatsCharGen::OnDoubleClick(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    int nChartId = GetChartFeatId(m_featChart);

#if KOTOR1
    CSWGuiSkillFlow* pFlow = static_cast<CSWGuiSkillFlow*>(m_listBoxFeats.GetSelectedControl());
    if (pFlow == 0)
        return;
    int nHit = pFlow->SkillHitCheckMouse(m_listBoxFeats.m_nHitLocalX, m_listBoxFeats.m_nHitLocalY);
#else
    int nHit = static_cast<CSWGuiSkillFlow*>(m_listBoxFeats.GetSelectedControl())
                   ->SkillHitCheckMouse(m_listBoxFeats.m_nHitLocalX, m_listBoxFeats.m_nHitLocalY);
#endif
    if (nHit == nChartId)
        OnFeatPicked(static_cast<unsigned short>(nChartId));
}

// Rebuilds everything from the creature's current stats: the feats it already owns, the granted
// pool, the selections-left budget, and then the chart, the available column and the button states.
void CSWGuiFeatsCharGen::InitiateFeats()
{
    CSWCLevelUpStats* pStats = static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats);

    if ((m_bFlags & (FEATS_FLAG_LEVEL_UP | FEATS_FLAG_BONUS)) == 0)
        pStats->ClearFeats();

    // The feats the creature already owns, minus the two placeholder rows.
    for (unsigned int i = 0; i < static_cast<unsigned short>(pStats->m_lstFeats.m_count); i++)
    {
#if KOTOR1
        // K1 has no placeholder-row filter: every owned feat is listed.
        m_aOwnedFeats.Add(pStats->GetFeat(static_cast<unsigned short>(i)));
#else
        if (pStats->GetFeat(static_cast<unsigned short>(i)) != CSWSCreatureStats::FEAT_COMPLEX_UNARMED_ANIMS
            && pStats->GetFeat(static_cast<unsigned short>(i)) != CSWSCreatureStats::FEAT_MINE_IMMUNITY)
        {
            m_aOwnedFeats.Add(pStats->GetFeat(static_cast<unsigned short>(i)));
        }
#endif
    }

    m_bFlags = static_cast<unsigned char>(m_bFlags & ~FEATS_FLAG_GRANTED_BOX_PUSHED);

    // The granted pool. Bit 1 (bonus mode) skips the whole info-box block.
    if (((m_bFlags >> 1) & 1) == 0)
    {
        CExoArrayList<unsigned int> aGranted;
#if KOTOR1
        pStats->AddGrantedFeats(&aGranted);
#else
        if ((m_bFlags & FEATS_FLAG_LEVEL_UP) == 0)
            pStats->AddGrantedFeats(&aGranted, true);
        else
            pStats->AddGrantedFeats(&aGranted, false);
#endif
        int nGranted = aGranted.m_count;
        if (nGranted > 0)
        {
            for (int i = 0; i < nGranted; i++)
                m_aNewFeats.Add(static_cast<unsigned short>(aGranted.m_pArray[i]));

            // A granted feat the creature already owns is not new: drop it from the granted list too.
            while (0 < nGranted)
            {
                nGranted--;
                unsigned short nFeat = static_cast<unsigned short>(aGranted.m_pArray[nGranted]);
                int nOwned = 0;
                for (int k = 0; k < m_aOwnedFeats.m_count; k++)
                {
                    if (m_aOwnedFeats.m_pArray[k] == nFeat)
                        nOwned++;
                }
                if (nOwned != 0)
                {
                    // inlined CExoArrayList<unsigned int>::DelIndex(nGranted)
                    for (int j = nGranted; j < aGranted.m_count - 1; j++)
                        aGranted.m_pArray[j] = aGranted.m_pArray[j + 1];
                    aGranted.m_count--;
                }
            }

            CSWGuiSkillInfoBox* pInfoBox =
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSkillInfoBoxPanel;
            pInfoBox->SetSkillList(aGranted, CSWGuiInGameSkillEntry::ENTRY_FEAT);
            pInfoBox->m_lblMessage.m_text.m_params.SetStrRef(STRREF_GRANTED_FEATS);
            m_bFlags = static_cast<unsigned char>(m_bFlags | FEATS_FLAG_GRANTED_BOX_PUSHED);
        }
    }

    pStats->CalcNumberFeats(m_nNormalSlots, m_nBonusSlots);

    m_nNormalLeft = m_nNormalSlots != 0 ? static_cast<unsigned char>(m_nNormalSlots + 1) : 0;
    m_nBonusLeft = m_nBonusSlots != 0 ? static_cast<unsigned char>(m_nBonusSlots + 1) : 0;

    if (((m_bFlags >> 1) & 1) == 0)
        m_nBonusSlots = 0;
    else
        m_nNormalSlots = 0;

    m_nSelectionsLeft = static_cast<unsigned char>(m_nNormalSlots + m_nBonusSlots);

    if (static_cast<signed char>(m_nSelectionsLeft) == 0)
    {
        OnAcceptButton();
        return;
    }

    m_featChart.CreateFeatChart(static_cast<CSWCCreatureStats*>(pStats), (m_bFlags >> 1) & 1);
    BuildAvailableList();
    BuildButtons();
}

// Walks feat.2da's compacted index list and adds every row the creature may still take: one that
// is not already in any of the four lists, and that CanChooseFeat agrees to.
void CSWGuiFeatsCharGen::BuildAvailableList()
{
    CExoString sUnused;

    if (g_pRules->m_nFeatIndices != 0)
    {
        CSWCLevelUpStats* pStats = static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats);

        for (unsigned int i = 0; i < static_cast<unsigned int>(g_pRules->m_nFeatIndices); i++)
        {
            unsigned short nFeat = static_cast<unsigned short>(g_pRules->m_paFeatIndices[i]);

            if (FindFeatIndex(m_aOwnedFeats, nFeat) != -1)
                continue;
            if (FindFeatIndex(m_aNewFeats, nFeat) != -1)
                continue;
            if (FindFeatIndex(m_aChosenFeats, nFeat) != -1)
                continue;
            if (FindFeatIndex(m_aAvailableFeats, nFeat) != -1)
                continue;

            if (pStats->CanChooseFeat(nFeat, m_nNormalLeft, m_nBonusLeft, &m_aChosenFeats) != 0)
                m_aAvailableFeats.Add(nFeat);
        }
    }
}

// Paints the chart from the four lists, hands it to LB_FEATS on the first build, then repaints the
// selections-left caption.
void CSWGuiFeatsCharGen::BuildButtons()
{
    m_featChart.SetSkillStatusAll(CSWGuiSkillFlow::SKILL_STATUS_UNAVAILABLE);

    for (int i = 0; i < m_aOwnedFeats.m_count; i++)
        m_featChart.SetSkillStatus(m_aOwnedFeats.m_pArray[i], CSWGuiSkillFlow::SKILL_STATUS_OWNED);
    for (int i = 0; i < m_aChosenFeats.m_count; i++)
        m_featChart.SetSkillStatus(m_aChosenFeats.m_pArray[i], CSWGuiSkillFlow::SKILL_STATUS_CHOSEN);
    for (int i = 0; i < m_aAvailableFeats.m_count; i++)
        m_featChart.SetSkillStatus(m_aAvailableFeats.m_pArray[i], CSWGuiSkillFlow::SKILL_STATUS_AVAILABLE);
    for (int i = 0; i < m_aNewFeats.m_count; i++)
        m_featChart.SetSkillStatus(m_aNewFeats.m_pArray[i], CSWGuiSkillFlow::SKILL_STATUS_GRANTED);

    if (m_nSelectedFeat == 0xffff)
    {
        m_listBoxFeats.AddControls(reinterpret_cast<CExoArrayList<CSWGuiControl*>&>(m_featChart.m_aFlows), 0, 1, 0);
        m_nSelectedFeat = static_cast<unsigned short>(m_featChart.SetTopLeft());
    }

    m_featChart.SetSelectedSkill(m_nSelectedFeat);
    m_listBoxFeats.SetSelectedControl(static_cast<unsigned int>(m_featChart.m_nSelectedFlow), 0);
    OnEnterFeat(m_nSelectedFeat);

    m_lblSelections.m_text.m_params.SetText(CExoString(m_nSelectionsLeft));
}

// A: accept. B: cancel. X: commit the focused row. Y: recommend. The arrow and trigger codes go to
// the chart, whose hit result re-focuses a row. Everything else, and every release, falls through to
// the base, which is why the base call sits after the switch.
void CSWGuiFeatsCharGen::HandleInputEvent(int nEvent, int nArg)
{
    if (nArg != 0)
    {
        switch (nEvent)
        {
        case INPUTEVENT_GUI_A:
#if !KOTOR1
        case INPUTEVENT_GUI_START:
#endif
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            OnAcceptButton();
            break;

        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            HandleCancelButton();
            break;

        case INPUTEVENT_GUI_X:
#if KOTOR1
        case INPUTEVENT_GUI_START:  // K1: Start commits the focused row, it does not accept
#endif
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            OnFeatPicked(m_featChart.m_aFlows.m_count < 1
                             ? 0xffff
                             : static_cast<unsigned short>(m_featChart.m_aFlows
                                   .m_pArray[m_featChart.m_nSelectedFlow]
                                   ->m_aSlots[m_featChart.m_nSelectedSlot]
                                   .m_nId));
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
            OnEnterFeat(static_cast<unsigned short>(m_featChart.HandleInput(nEvent)));
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

// B: throw the page away. In create mode the creature's own feat array is wiped first (this page is
// the only thing that ever fills it); in level-up mode the real stats belong to the server.
void CSWGuiFeatsCharGen::HandleCancelButton()
{
#if KOTOR1
    // K1: no ClearFeats, and K1's state bits sit one higher.
    if (m_pParentPanel == 0)
        return;

    m_pGuiManager->PopModalPanel();
    m_pParentPanel->SetVisible(1);

    m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
    if (m_pCreature != 0 && m_pCreature->m_pStats != 0 && (m_bFlags & FEATS_FLAG_LEVEL_UP) == 0)
        m_pCreature->m_pStats->ClearFeats();

    if (m_pParentPanel == 0)
        return;

    m_pGuiManager->PopModalPanel();
    m_pParentPanel->SetVisible(1);

    CSWGuiPanel* pPanel = this;
    do
    {
        pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
        pPanel = pPanel->m_pNextInChain;
    } while (pPanel != 0);
#endif

    if ((m_bFlags & FEATS_FLAG_LEVEL_UP) == 0)
    {
        static_cast<CSWGuiCustomPanel*>(m_pParentPanel)->CancelledByChild();
        return;
    }
    static_cast<CSWGuiLevelUpPanel*>(m_pParentPanel)->CancelledByChild();
}

// With selections still unspent, ask first; otherwise commit. The commit path pushes every chosen
// feat into the creature's own array, tears the page down, and reopens a fresh bonus page for the
// remainder when the level granted bonus slots.
void CSWGuiFeatsCharGen::OnAcceptButton()
{
    if (m_nSelectionsLeft != 0 && m_aAvailableFeats.m_count >= 1)
    {
        // With selections left, push the query box instead of committing.
        CSWGuiMessageBox* pBox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetCallback(0, 0, 0);
        pBox->SetMessage(STRREF_MUST_SELECT_NEW_FEATS);
        m_pGuiManager->AddPanel(pBox, 1, 1);
    }
    else
    {
        for (int i = 0; i < m_aChosenFeats.m_count; i++)
        {
            CSWCCreatureStats* pStats = m_pCreature->m_pStats;
            unsigned short nFeat = m_aChosenFeats.m_pArray[i];
            CExoArrayList<unsigned short>& aFeats = pStats->m_lstFeats;
            if (aFeats.Contains(nFeat) == 0)
                aFeats.Add(nFeat);
        }

        m_pGuiManager->PopModalPanel();
        m_pParentPanel->SetVisible(1);

        CSWGuiPanel* pPanel = this;
#if KOTOR1
        // K1: no panel chain; one state stamp per panel.
        pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
        do
        {
            pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
            pPanel = pPanel->m_pNextInChain;
        } while (pPanel != 0);
#endif

        m_listBoxFeats.ClearItems();
        m_featChart.ClearChart();

        unsigned char nNormal;
        unsigned char nBonus;
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->CalcNumberFeats(nNormal, nBonus);

        if (((m_bFlags >> 1) & 1) != 0 || nBonus == 0)
        {
            if ((m_bFlags & FEATS_FLAG_LEVEL_UP) == 0)
                static_cast<CSWGuiCustomPanel*>(m_pParentPanel)->FeatCreationCompleted();
            else
                static_cast<CSWGuiLevelUpPanel*>(m_pParentPanel)->FeatSelectionCompleted();
            return;
        }

        CSWGuiFeatsCharGen* pNew =
            new CSWGuiFeatsCharGen(m_pGuiManager, m_pCreature, m_bFlags & FEATS_FLAG_LEVEL_UP, 1);
        pNew->m_pParentPanel = m_pParentPanel;
        m_pGuiManager->AddPanel(pNew, 3, 1);
        return;
    }
}

// X (or a double click): toggle the focused feat. A row in the available list becomes chosen
// (refused outright when nothing is left to spend), a row in the chosen list goes back; the other two
// lists explain themselves through the query box.
void CSWGuiFeatsCharGen::OnFeatPicked(unsigned short nFeat)
{
    uint64_t nStrRef;

    if (FindFeatIndex(m_aAvailableFeats, nFeat) != -1)
    {
        if (m_nSelectionsLeft != 0)
        {
            AddChosenFeat(nFeat);
            BuildButtons();
            return;
        }
        nStrRef = STRREF_ALL_FEATS_SELECTED;
    }
    else if (FindFeatIndex(m_aChosenFeats, nFeat) != -1)
    {
        RemoveChosenFeat(nFeat);
        BuildButtons();
        return;
    }
    else if (FindFeatIndex(m_aOwnedFeats, nFeat) != -1)
    {
        nStrRef = STRREF_CANNOT_DESELECT_OWNED_FEAT;
    }
    else if (FindFeatIndex(m_aNewFeats, nFeat) != -1)
    {
        nStrRef = STRREF_FEAT_GRANTED_AT_LEVEL;
    }
    else
    {
        nStrRef = STRREF_FEAT_PREREQUISITES_NOT_MET;
    }

    CSWGuiMessageBox* pBox =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    pBox->SetAllowCancel(0);
    pBox->SetCallback(0, 0, 0);
    pBox->SetMessage(nStrRef);
    m_pGuiManager->AddPanel(pBox, 1, 1);
}

// Focus a feat row: classify it (the status byte is also the draw state), retitle and recolour the
// select button accordingly, then hand the feat's own name strref and description to the readout
// column.
void CSWGuiFeatsCharGen::OnEnterFeat(unsigned short nFeat)
{
    unsigned char nStatus;
    if (FindFeatIndex(m_aAvailableFeats, nFeat) != -1)
        nStatus = DRAW_STATE_AVAILABLE;
    else if (FindFeatIndex(m_aChosenFeats, nFeat) != -1)
        nStatus = DRAW_STATE_CHOSEN;
    else if (FindFeatIndex(m_aOwnedFeats, nFeat) != -1)
        nStatus = DRAW_STATE_OWNED;
    else if (FindFeatIndex(m_aNewFeats, nFeat) != -1)
        nStatus = DRAW_STATE_GRANTED;
    else
        nStatus = DRAW_STATE_UNAVAILABLE;

    m_nDrawState = nStatus;
    m_nSelectedFeat = nFeat;

    CSWFeat* pFeat = g_pRules->GetFeat(nFeat);

#if KOTOR1
    // K1: takeable and taken rows use the global blue caption and the enabled icon colour, the
    // others the disabled text; no glyph is re-added here.
    switch (m_nDrawState)
    {
    case DRAW_STATE_AVAILABLE:
        m_btnSelect.m_text.m_params.SetText(m_sFeatAvailableText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
        m_btnSelect.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::ENABLED_ICON_COLOR);
        break;

    case DRAW_STATE_CHOSEN:
        m_btnSelect.m_text.m_params.SetText(m_sFeatTakenText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
        m_btnSelect.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::ENABLED_ICON_COLOR);
        break;

    case DRAW_STATE_GRANTED:
    case DRAW_STATE_OWNED:
        m_btnSelect.m_text.m_params.SetText(m_sFeatTakenText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        m_btnSelect.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
        break;

    case DRAW_STATE_UNAVAILABLE:
        m_btnSelect.m_text.m_params.SetText(m_sFeatAvailableText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        m_btnSelect.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
        break;
    }

    m_lblName.m_text.m_params.SetStrRef(pFeat->m_nNameStrRef);
    SetDescription(pFeat->m_nDescriptionStrRef);
#else
    switch (m_nDrawState)
    {
    case DRAW_STATE_AVAILABLE:
        m_btnSelect.m_text.m_params.SetText(m_sFeatAvailableText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        m_btnSelect.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
        break;

    case DRAW_STATE_CHOSEN:
        m_btnSelect.m_text.m_params.SetText(m_sFeatTakenText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        m_btnSelect.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
        break;

    case DRAW_STATE_GRANTED:
    case DRAW_STATE_OWNED:
        m_btnSelect.m_text.m_params.SetText(m_sFeatTakenText);
        m_btnSelect.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
        m_btnSelect.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
        break;

    case DRAW_STATE_UNAVAILABLE:
        m_btnSelect.m_text.m_params.SetText(m_sFeatAvailableText);
        m_btnSelect.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_GREEN);
        m_btnSelect.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
        break;
    }

    m_lblName.m_text.m_params.SetStrRef(pFeat->m_nNameStrRef);
    SetDescription(pFeat->m_nDescriptionStrRef);
    AspyrAddButtonIcon(&m_btnSelect, 'x');
#endif
}

// Y: rebuild the page from the character's own class recommendation. A class's suggestion for a feat
// build is its default package, so the body empties the chosen list, lets the package grant its
// feats, adds each one, then repaints.
void CSWGuiFeatsCharGen::OnRecommendButton()
{
    CExoArrayList<unsigned short> aGranted;
    CExoArrayList<unsigned int> aGrantedIds;
    CSWCCreatureStats* pStats = m_pCreature->m_pStats;

    unsigned int nClass = pStats->GetClass(static_cast<unsigned char>(pStats->m_nNumClasses - 1));

    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)
        ->AddDefaultPackageFeats(static_cast<int>(nClass & 0xff), &aGranted,
                                 ((m_bFlags >> 1) & 1) ^ 1, (m_bFlags >> 1) & 1);

    while (m_aChosenFeats.m_count != 0)
        RemoveChosenFeat(m_aChosenFeats.m_pArray[0]);

    aGrantedIds.SetSize(aGranted.m_count);
    for (int i = 0; i < aGranted.m_count; i++)
    {
        AddChosenFeat(aGranted.m_pArray[i]);
        aGrantedIds.m_pArray[i] = aGranted.m_pArray[i];
    }

    CSWGuiSkillInfoBox* pInfoBox =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSkillInfoBoxPanel;
    pInfoBox->SetSkillList(aGrantedIds, CSWGuiInGameSkillEntry::ENTRY_FEAT);
    pInfoBox->m_lblMessage.m_text.m_params.SetStrRef(STRREF_RECOMMENDED_FEATS);
    m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(pInfoBox), 1, 1);

    BuildButtons();
}

// Takes a feat back out of the chosen list, shifts the tail down and refunds a selection, then
// re-validates what is left: any chosen feat whose prerequisites the shrunken list no longer meets is
// refunded too, which is why the outer loop re-runs from the top each trip.
void CSWGuiFeatsCharGen::RemoveChosenFeat(unsigned short nFeat)
{
    int nCount = m_aChosenFeats.m_count;
    if (nCount < 1)
        return;

    for (;;)
    {
        int nIndex = FindFeatIndex(m_aChosenFeats, nFeat);
        if (nIndex == -1)
            return;

        m_aChosenFeats.DelIndex(nIndex);
        nCount = m_aChosenFeats.m_count;

        m_nSelectionsLeft = static_cast<unsigned char>(m_nSelectionsLeft + 1);

        if (nCount >= 1)
        {
            // The first chosen feat whose prerequisites no longer hold is the next to refund.
            CSWCLevelUpStats* pStats = static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats);
            unsigned int nBad;
            if (pStats->HasFeatPrereq(g_pRules->GetFeat(m_aChosenFeats.m_pArray[0]),
                                      &m_aChosenFeats)
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
                    if (static_cast<int64_t>(m_aChosenFeats.m_count) <= static_cast<int64_t>(i))
                    {
                        m_aAvailableFeats.m_count = 0;
                        BuildAvailableList();
                        return;
                    }
                    unsigned short nCur = m_aChosenFeats.m_pArray[i];
                    nOk = pStats->HasFeatPrereq(g_pRules->GetFeat(nCur), &m_aChosenFeats);
                    i++;
                } while (nOk != 0);
                nBad = static_cast<unsigned int>(i - 1);
            }

            nFeat = m_aChosenFeats.m_pArray[nBad];
            nCount = m_aChosenFeats.m_count;
        }
        else
        {
            m_aAvailableFeats.m_count = 0;
            BuildAvailableList();
            return;
        }
    }
}

// Moves a feat out of the available list and into the chosen one, insertion-sorted by name, and
// spends a selection. The sort is what makes the right column read alphabetically.
void CSWGuiFeatsCharGen::AddChosenFeat(unsigned short nFeat)
{
    int nCount = m_aAvailableFeats.m_count;
    if (nCount < 1)
        return;

    int nIndex = FindFeatIndex(m_aAvailableFeats, nFeat);
    if (nIndex == -1)
        return;

    // inlined CExoArrayList<unsigned short>::DelIndex(nIndex)
    nCount--;
    m_aAvailableFeats.m_count = nCount;
    if (nIndex < nCount)
    {
        for (int i = nIndex; i < nCount; i++)
            m_aAvailableFeats.m_pArray[i] = m_aAvailableFeats.m_pArray[i + 1];
    }

    // Where the new feat belongs in the chosen column: the first row whose name sorts after it.
    CExoString sNewName = g_pRules->GetFeat(nFeat)->GetNameText();

    int bFound = 0;
    int nInsert;
    for (nInsert = 0; !bFound && nInsert < m_aChosenFeats.m_count; nInsert++)
    {
        CExoString sExistingName =
            g_pRules->GetFeat(m_aChosenFeats.m_pArray[nInsert])->GetNameText();
        bFound = sExistingName > sNewName;
    }

    if (bFound)
    {
        m_nSelectionsLeft = static_cast<unsigned char>(m_nSelectionsLeft - 1);

        // The list's own Add is what grows it; walk the tail right one slot so the appended copy is
        // overwritten by its predecessor down to this row.
        m_aChosenFeats.Add(nFeat);
        for (int i = m_aChosenFeats.m_count - 1; i > nInsert - 1; i--)
            m_aChosenFeats.m_pArray[i] = m_aChosenFeats.m_pArray[i - 1];
        m_aChosenFeats.m_pArray[nInsert - 1] = nFeat;
    }
    else
    {
        // Nothing sorts after it, so append.
        m_nSelectionsLeft = static_cast<unsigned char>(m_nSelectionsLeft - 1);
        m_aChosenFeats.Add(nFeat);
    }

    BuildAvailableList();
}

// Which of the four lists holds this feat: the status byte is the list's own index, 4 meaning none.
void CSWGuiFeatsCharGen::DetermineFeat(unsigned short nFeat, unsigned char& nStatus)
{
    if (FindFeatIndex(m_aAvailableFeats, nFeat) != -1)
    {
        nStatus = 0;
        return;
    }
    if (FindFeatIndex(m_aChosenFeats, nFeat) != -1)
    {
        nStatus = 1;
        return;
    }
    if (FindFeatIndex(m_aOwnedFeats, nFeat) != -1)
    {
        nStatus = 3;
        return;
    }
    if (FindFeatIndex(m_aNewFeats, nFeat) != -1)
    {
        nStatus = 2;
        return;
    }
    nStatus = 4;
}

// Shows a feat description in LB_DESC: retarget the row's text, clear the box, resize the row to the
// string's own ideal height (falling back to one font height on an empty string) and re-add it as the
// box's single selected row.
void CSWGuiFeatsCharGen::SetDescription(unsigned int nStrRef)
{
    STR_RES res;
    g_pTlkTable->Fetch(nStrRef, res, CTlkTable::GENDER_MALE);
    m_lblDescRow.m_text.m_params.SetText(res.text);

    m_listBoxDesc.ClearItems();

    CSWGuiText* pText = &m_lblDescRow.m_text;

    CSWGuiExtent extent = m_lblDescRow.m_extent;
#if KOTOR1
    // K1: no resolution scaling.
    if (res.text.GetLength() >= 1)
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
#else
    if (res.text.m_sString && static_cast<int>(strlen(res.text.m_sString)) >= 1)
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
#ifndef K2_FIX_ENGINE_BUGS
    float fScalar = GetResolutionTextScalar();
    extent.h = static_cast<int>(fScalar * static_cast<float>(extent.h));
#endif
#endif
    m_lblDescRow.SetExtent(extent);

    CSWGuiControl* apRows[1] = { &m_lblDescRow };
    m_listBoxDesc.AddControls(apRows, 1, 0, 0, 0);
    m_listBoxDesc.SetSelectedControl(&m_lblDescRow, 0);
}

// The select button's caption follows the focused row's class: an available or chosen row shows the
// plain "available" string, a granted or owned one the "already have it" one, and both grey the
// icon.
void CSWGuiFeatsCharGen::Draw(float f)
{
#if KOTOR1
    // K1: the caption goes to the disabled text, and there is no button-icon call.
    if (m_nDrawState == DRAW_STATE_GRANTED || m_nDrawState == DRAW_STATE_OWNED)
    {
        m_btnSelect.m_text.m_params.SetText(m_sFeatTakenText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        m_btnSelect.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
    }
    else if (m_nDrawState == DRAW_STATE_UNAVAILABLE)
    {
        m_btnSelect.m_text.m_params.SetText(m_sFeatAvailableText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        m_btnSelect.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
    }
#else
    if (m_nDrawState == DRAW_STATE_GRANTED || m_nDrawState == DRAW_STATE_OWNED)
    {
        m_btnSelect.m_text.m_params.SetText(m_sFeatTakenText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_GREEN);
        m_btnSelect.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
        AspyrAddButtonIcon(&m_btnSelect, 'x');
    }
    else if (m_nDrawState == DRAW_STATE_UNAVAILABLE)
    {
        m_btnSelect.m_text.m_params.SetText(m_sFeatAvailableText);
        m_btnSelect.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_GREEN);
        m_btnSelect.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
    }
#endif

    CSWGuiPanel::Draw(f);
}
