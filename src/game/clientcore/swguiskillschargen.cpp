#include "swguiskillschargen.h"
#include "swgui.h"
#include "swguicontrol.h"
#include "swguidrawable.h"
#include "swguiingame.h"
#include "swguicustompanel.h"
#include "swguileveluppanel.h"
#include "nwccreature.h"
#include "nwccreaturestats.h"
#include "nwclass.h"
#include "nwskill.h"
#include "nwrace.h"
#include "nwscreature.h"
#include "nwscreaturestats.h"
#include "partytables.h"
#include "serverexoapp.h"
#include "exoresman.h"
#include "exotlktable.h"
#include "exolocstringinternal.h"
#include "nwsrules.h"
#include "guiingame.h"
#include "clientexoapp.h"
#include "appmanager.h"
#include "port/k2qolapi.h"
#include "tutorial.h"
#include "nwscriptconst.h"

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#if KOTOR1
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#endif
#ifdef K2_QOL_IMPROVEMENTS
static void K2QolFitCaption(CSWGuiLabel& label)
{
    K2QolMarkFitText(&label.m_text);
    K2QolFitText(label.m_text, label.m_text.m_extent.h);
}
#endif

#include "port/libcstring.h"
#include "exoinput.h"
#include "strref.h"
#include "exostringinline.h"

// CSWSkill::m_flags: skill.2da's NPCCanUse and DroidCanUse columns.
enum {
    SKILL_FLAG_NPC_CAN_USE = 0x10,
    SKILL_FLAG_DROID_CAN_USE = 0x20,
};

// The skill page runs in two modes over one layout. Each row's four controls are bound in the
// layout's own order and its description is resolved from skill.2da right after them.
#if KOTOR1
// K1 binds the same layout without the bar labels and with less padding; no class-skill colouring
// and no class-skill feat on the rank ceiling; no G0-T0 carve-out; its own button glyphs and a
// floating 'x' beside the description box.
CSWGuiSkillsCharGen::CSWGuiSkillsCharGen(CSWGuiManager* pGuiManager, CSWCCreature* pCreature,
                                         int nMode)
    : CSWGuiPanel(pGuiManager)
{
    m_pCreature = pCreature;

    m_bLevelUp = nMode;
    m_bCanModify = (nMode == 0
                    || (m_pCreature->GetServerCreature()->m_pStats->m_bIsPC != 0
                        && g_pAppManager->m_pServerExoApp != 0
                        && g_pAppManager->m_pServerExoApp->GetPartyTable() != 0
                        && g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nControlledNPC == -1));
    m_bIsDroid = (m_pCreature->m_pStats->m_nRace == CSWRace::CRACE_DROID);

    m_pCreature->m_pStats->GetClassLeveledUpIn();

    StartLoadFromLayout(CResRef("SKCHRGEN"));

    InitControl(m_lblTitle,        CExoString("MAIN_TITLE_LBL"), 1);
    if (nMode != 0)
        m_lblTitle.m_text.m_params.SetStrRef(STRREF_LEVEL_UP);
    InitControl(m_lblSubTitle,     CExoString("SUB_TITLE_LBL"), 1);
    InitControl(m_lblRemainingBox, CExoString("REMAINING_BOX_LBL"), 1);
    InitControl(m_lblSelections,   CExoString("REMAINING_SELECTIONS_LBL"), 1);
    InitControl(m_lblSelRemaining, CExoString("SELECTIONS_REMAINING_LBL"), 1);
    InitControl(m_lblDesc,         CExoString("DESC_LBL"), 1);
    InitControl(m_lblCost,         CExoString("COST_LBL"), 1);
    InitControl(m_lblCostPoints,   CExoString("COST_POINTS_LBL"), 1);
    InitControl(m_lblClassSkill,   CExoString("CLASSSKL_LBL"), 1);
    InitControl(m_listBox,         CExoString("LB_DESC"), 1);
    m_listBox.m_nScrollSound = 1;
    InitControl(m_btnAccept,       CExoString("BTN_ACCEPT"), 1);
    InitControl(m_btnRecommended,  CExoString("BTN_RECOMMENDED"), 1);
    InitControl(m_btnBack,         CExoString("BTN_BACK"), 1);

    InitControl(m_aPointsButtons[0], CExoString("COMPUTER_USE_POINTS_BTN"), 1);
    InitControl(m_aSkillLabels[0],   CExoString("COMPUTER_USE_LBL"), 1);
    m_asSkillDescriptions[0] = g_pRules->m_paSkills[0].GetDescriptionText();
    InitControl(m_aMinusButtons[0],  CExoString("COM_MINUS_BTN"), 1);
    InitControl(m_aPlusButtons[0],   CExoString("COM_PLUS_BTN"), 1);

    InitControl(m_aPointsButtons[1], CExoString("DEMOLITIONS_POINTS_BTN"), 1);
    InitControl(m_aSkillLabels[1],   CExoString("DEMOLITIONS_LBL"), 1);
    m_asSkillDescriptions[1] = g_pRules->m_paSkills[1].GetDescriptionText();
    InitControl(m_aMinusButtons[1],  CExoString("DEM_MINUS_BTN"), 1);
    InitControl(m_aPlusButtons[1],   CExoString("DEM_PLUS_BTN"), 1);

    InitControl(m_aPointsButtons[2], CExoString("STEALTH_POINTS_BTN"), 1);
    InitControl(m_aSkillLabels[2],   CExoString("STEALTH_LBL"), 1);
    m_asSkillDescriptions[2] = g_pRules->m_paSkills[2].GetDescriptionText();
    InitControl(m_aMinusButtons[2],  CExoString("STE_MINUS_BTN"), 1);
    InitControl(m_aPlusButtons[2],   CExoString("STE_PLUS_BTN"), 1);

    InitControl(m_aPointsButtons[4], CExoString("PERSUADE_POINTS_BTN"), 1);
    InitControl(m_aSkillLabels[4],   CExoString("PERSUADE_LBL"), 1);
    m_asSkillDescriptions[4] = g_pRules->m_paSkills[4].GetDescriptionText();
    InitControl(m_aMinusButtons[4],  CExoString("PER_MINUS_BTN"), 1);
    InitControl(m_aPlusButtons[4],   CExoString("PER_PLUS_BTN"), 1);

    InitControl(m_aPointsButtons[5], CExoString("REPAIR_POINTS_BTN"), 1);
    InitControl(m_aSkillLabels[5],   CExoString("REPAIR_LBL"), 1);
    m_asSkillDescriptions[5] = g_pRules->m_paSkills[5].GetDescriptionText();
    InitControl(m_aMinusButtons[5],  CExoString("REP_MINUS_BTN"), 1);
    InitControl(m_aPlusButtons[5],   CExoString("REP_PLUS_BTN"), 1);

    InitControl(m_aPointsButtons[6], CExoString("SECURITY_POINTS_BTN"), 1);
    InitControl(m_aSkillLabels[6],   CExoString("SECURITY_LBL"), 1);
    m_asSkillDescriptions[6] = g_pRules->m_paSkills[6].GetDescriptionText();
    InitControl(m_aMinusButtons[6],  CExoString("SEC_MINUS_BTN"), 1);
    InitControl(m_aPlusButtons[6],   CExoString("SEC_PLUS_BTN"), 1);

    InitControl(m_aPointsButtons[3], CExoString("AWARENESS_POINTS_BTN"), 1);
    InitControl(m_aSkillLabels[3],   CExoString("AWARENESS_LBL"), 1);
    m_asSkillDescriptions[3] = g_pRules->m_paSkills[3].GetDescriptionText();
    InitControl(m_aMinusButtons[3],  CExoString("AWA_MINUS_BTN"), 1);
    InitControl(m_aPlusButtons[3],   CExoString("AWA_PLUS_BTN"), 1);

    InitControl(m_aPointsButtons[7], CExoString("TREAT_INJURY_POINTS_BTN"), 1);
    InitControl(m_aSkillLabels[7],   CExoString("TREAT_INJURY_LBL"), 1);
    m_asSkillDescriptions[7] = g_pRules->m_paSkills[7].GetDescriptionText();
    InitControl(m_aMinusButtons[7],  CExoString("TRE_MINUS_BTN"), 1);
    InitControl(m_aPlusButtons[7],   CExoString("TRE_PLUS_BTN"), 1);

    CSWGuiExtent extAccept = m_btnAccept.m_extent;
    CSWGuiExtent extBack = m_btnBack.m_extent;
    m_btnBack.SetExtent(extAccept);
    m_btnAccept.SetExtent(extBack);

    StopLoadFromLayout();
    ScaleExtentForResolution();

    CSWGuiLabel* pProto = static_cast<CSWGuiLabel*>(m_listBox.m_pProtoItem);
    m_lblDescRow.Initialize(pProto->m_extent, pProto, 1.0f);

    m_nSelectedRow = 0;

    for (int i = 0; i < 8; i++)
    {
        m_aPointsButtons[i].AddEvent(SWGUIEVENT_FOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSkillsCharGen::OnEnterPointsButton));
        m_aPointsButtons[i].AddEvent(SWGUIEVENT_UNFOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSkillsCharGen::OnExitPointsButton));
        m_aPointsButtons[i].m_tail.m_nUserData = i;

        if (m_bLevelUp == 0)
        {
            static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetSkillRankBase(static_cast<unsigned short>(i), 0);
            if (i < g_pRules->m_nNumSkills)
                m_pCreature->m_pStats->m_pSkillRanks[i] = 0;
        }

        unsigned char nRank = (i < g_pRules->m_nNumSkills)
                                  ? m_pCreature->m_pStats->m_pSkillRanksBase[i]
                                  : 0;
        m_abSkillRank[i] = nRank;
        m_aPointsButtons[i].m_text.m_params.SetText(CExoString(static_cast<int>(nRank)));

        int nMaxRank = m_pCreature->m_pStats->GetLevel() + 3;
        for (int nClass = 0; nClass < m_pCreature->m_pStats->m_nNumClasses; nClass++)
        {
            unsigned char nClassId = m_pCreature->m_pStats->GetClass(static_cast<unsigned char>(nClass));
            if (g_pRules->m_paClasses[nClassId].IsSkillClassSkill(static_cast<unsigned short>(i)))
                goto lClassSkillRank;
        }
        nMaxRank >>= 1;
    lClassSkillRank:;

        if (m_bLevelUp == 0)
        {
            m_aMinusButtons[i].m_tail.bVisible =
                (((i < g_pRules->m_nNumSkills)
                      ? m_pCreature->m_pStats->m_pSkillRanksBase[i]
                      : 0) != 0);
        }
        else
        {
            m_aMinusButtons[i].m_tail.bVisible =
                (m_abSkillRank[i] != static_cast<signed char>((i < g_pRules->m_nNumSkills)
                                                       ? m_pCreature->m_pStats->m_pSkillRanksBase[i]
                                                       : 0));
        }
        m_aMinusButtons[i].m_tail.bFocusOnHover = 0;
        m_aPlusButtons[i].m_tail.bVisible =
            (nMaxRank != static_cast<signed char>((i < g_pRules->m_nNumSkills)
                                           ? m_pCreature->m_pStats->m_pSkillRanksBase[i]
                                           : 0));
        m_aPlusButtons[i].m_tail.bFocusOnHover = 0;

        m_aMinusButtons[i].AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSkillsCharGen::OnMinusButtonPushed));
        m_aPlusButtons[i].AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSkillsCharGen::OnPlusButtonPushed));

        if (!m_bCanModify)
        {
            unsigned char nFlags = g_pRules->m_paSkills[i].m_flags;
            if (!(nFlags & SKILL_FLAG_NPC_CAN_USE) || (m_bIsDroid && !(nFlags & SKILL_FLAG_DROID_CAN_USE)))
            {
                m_aSkillLabels[i].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
                m_aPointsButtons[i].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
                m_aPlusButtons[i].m_tail.bVisible = 0;
            }
        }
    }

    m_btnAccept.m_tail.bFocusOnHover = 0;
    m_btnAccept.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnAButtonPressed));
    m_btnRecommended.m_tail.bFocusOnHover = 0;
    m_btnRecommended.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnYButtonPressed));
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));

    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddSkillPointsPerLevel();
    m_nPoints = m_pCreature->m_pStats->m_nSkillPointsRemaining;
    m_nSkillPointsPerLevel = m_nPoints;
    m_lblSelections.m_text.m_params.SetText(CExoString(m_nPoints));

    SetActiveControl(&m_aPointsButtons[m_nSelectedRow], 0);

    IosAddButtonIcon(&m_btnRecommended, 'y');
    IosAddButtonIcon(&m_btnAccept, 'a');
    IosAddButtonIcon(&m_btnBack, 'b');

    CSWGuiExtent extScroll;
    extScroll.x = m_listBox.m_extent.x + m_listBox.m_extent.w;
    extScroll.y = m_listBox.m_extent.y + m_listBox.m_extent.h;
    extScroll.w = m_listBox.m_extent.w;
    extScroll.h = m_listBox.m_extent.h;
    m_iconDescScroll.InitAndAdd(this, 'x', extScroll, -30, -30);
#if defined(K2_HOST_BUILD) && !defined(K2_UI_TOUCH)
    // X pages the description. The pc layout's scroll bar lies inside the box, under the corner the
    // glyph hangs on, so it goes at the right end of the Description header instead and appears only
    // when the description scrolls.
    {
        const CSWGuiExtent& header = m_lblDesc.m_extent;
        CSWGuiExtent glyph;
        glyph.w = glyph.h = header.h;
        glyph.x = header.x + header.w - glyph.w - header.h / 4;
        glyph.y = header.y;
        m_iconDescScroll.SetExtent(glyph);
        m_iconDescScroll.m_tail.bVisible =
            m_listBox.m_scrollBar.m_nMaxValue > m_listBox.m_scrollBar.m_nVisibleValue;
    }
#endif
}
#else
CSWGuiSkillsCharGen::CSWGuiSkillsCharGen(CSWGuiManager* pGuiManager, CSWCCreature* pCreature,
                                         int nMode)
    : CSWGuiPanel(pGuiManager)
{
    m_pCreature = pCreature;

    m_bLevelUp = nMode;
    // Level-up mode is only modifiable for a single PC mid-chargen: the party table has not yet
    // selected an NPC to control and the creature really is the PC.
    m_bCanModify = (nMode == 0
                    || (m_pCreature->GetServerCreature()->m_pStats->m_bIsPC != 0
                        && g_pAppManager->m_pServerExoApp != 0
                        && g_pAppManager->m_pServerExoApp->GetPartyTable() != 0
                        && g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nControlledNPC == -1));
    m_bIsDroid = (m_pCreature->m_pStats->m_nRace == CSWRace::CRACE_DROID);

    // Called for its side effect.
    m_pCreature->m_pStats->GetClassLeveledUpIn();

    StartLoadFromLayout(CResRef("SKCHRGEN_p"), 1, 1, 1);

    InitControl(m_lblTitle,        CExoString("MAIN_TITLE_LBL"), 1, 1, 0, 0, 0);
    if (nMode != 0)
        m_lblTitle.m_text.m_params.SetStrRef(STRREF_LEVEL_UP);
    InitControl(m_lblSubTitle,     CExoString("SUB_TITLE_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblRemainingBox, CExoString("REMAINING_BOX_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblSelections,   CExoString("REMAINING_SELECTIONS_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblSelRemaining, CExoString("SELECTIONS_REMAINING_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblCost,         CExoString("COST_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblCostPoints,   CExoString("COST_POINTS_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblClassSkill,   CExoString("CLASSSKL_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_listBox,         CExoString("LB_DESC"), 1, 1, 0, 0, 0);
    m_listBox.m_nScrollSound = 1;
    InitControl(m_btnAccept,       CExoString("BTN_ACCEPT"), 1, 1, 0, 0, 0);
    InitControl(m_btnRecommended,  CExoString("BTN_RECOMMENDED"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack,         CExoString("BTN_BACK"), 1, 1, 0, 0, 0);
    InitControl(m_lblBar1,         CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_lblBar2,         CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    AspyrAddButtonIcon(&m_btnAccept, 'a');
    AspyrAddButtonIcon(&m_btnBack, 'b');
    AspyrAddButtonIcon(&m_btnRecommended, 'y');

    InitControl(m_aPointsButtons[0], CExoString("COMPUTER_USE_POINTS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aSkillLabels[0],   CExoString("COMPUTER_USE_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_aMinusButtons[0],  CExoString("COM_MINUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aPlusButtons[0],   CExoString("COM_PLUS_BTN"), 1, 1, 0, 0, 0);
    m_asSkillDescriptions[0] = g_pRules->m_paSkills[0].GetDescriptionText();

    InitControl(m_aPointsButtons[1], CExoString("DEMOLITIONS_POINTS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aSkillLabels[1],   CExoString("DEMOLITIONS_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_aMinusButtons[1],  CExoString("DEM_MINUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aPlusButtons[1],   CExoString("DEM_PLUS_BTN"), 1, 1, 0, 0, 0);
    m_asSkillDescriptions[1] = g_pRules->m_paSkills[1].GetDescriptionText();

    InitControl(m_aPointsButtons[2], CExoString("STEALTH_POINTS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aSkillLabels[2],   CExoString("STEALTH_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_aMinusButtons[2],  CExoString("STE_MINUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aPlusButtons[2],   CExoString("STE_PLUS_BTN"), 1, 1, 0, 0, 0);
    m_asSkillDescriptions[2] = g_pRules->m_paSkills[2].GetDescriptionText();

    InitControl(m_aPointsButtons[4], CExoString("PERSUADE_POINTS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aSkillLabels[4],   CExoString("PERSUADE_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_aMinusButtons[4],  CExoString("PER_MINUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aPlusButtons[4],   CExoString("PER_PLUS_BTN"), 1, 1, 0, 0, 0);
    m_asSkillDescriptions[4] = g_pRules->m_paSkills[4].GetDescriptionText();

    InitControl(m_aPointsButtons[5], CExoString("REPAIR_POINTS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aSkillLabels[5],   CExoString("REPAIR_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_aMinusButtons[5],  CExoString("REP_MINUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aPlusButtons[5],   CExoString("REP_PLUS_BTN"), 1, 1, 0, 0, 0);
    m_asSkillDescriptions[5] = g_pRules->m_paSkills[5].GetDescriptionText();

    InitControl(m_aPointsButtons[3], CExoString("AWARENESS_POINTS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aSkillLabels[3],   CExoString("AWARENESS_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_aMinusButtons[3],  CExoString("AWA_MINUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aPlusButtons[3],   CExoString("AWA_PLUS_BTN"), 1, 1, 0, 0, 0);
    m_asSkillDescriptions[3] = g_pRules->m_paSkills[3].GetDescriptionText();

    InitControl(m_aPointsButtons[6], CExoString("SECURITY_POINTS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aSkillLabels[6],   CExoString("SECURITY_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_aMinusButtons[6],  CExoString("SEC_MINUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aPlusButtons[6],   CExoString("SEC_PLUS_BTN"), 1, 1, 0, 0, 0);
    m_asSkillDescriptions[6] = g_pRules->m_paSkills[6].GetDescriptionText();

    InitControl(m_aPointsButtons[7], CExoString("TREAT_INJURY_POINTS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aSkillLabels[7],   CExoString("TREAT_INJURY_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_aMinusButtons[7],  CExoString("TRE_MINUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aPlusButtons[7],   CExoString("TRE_PLUS_BTN"), 1, 1, 0, 0, 0);
    m_asSkillDescriptions[7] = g_pRules->m_paSkills[7].GetDescriptionText();

    StopLoadFromLayout();
#ifdef K2_QOL_IMPROVEMENTS
    K2QolFitCaption(m_lblClassSkill);
    for (int i = 0; i < 8; i++)
        K2QolFitCaption(m_aSkillLabels[i]);
#endif

    // Stands the description row up off the list box's prototype item, then hands it to the box as
    // its only row.
    CSWGuiLabel* pProto = static_cast<CSWGuiLabel*>(m_listBox.m_pProtoItem);
    m_lblDescRow.Initialize(pProto->m_extent, pProto, 1.0f);

    m_nSelectedRow = 0;

    for (int i = 0; i < 8; i++)
    {
        // A points row reports both enter and exit; the plus and minus buttons only the press.
        m_aPointsButtons[i].AddEvent(SWGUIEVENT_FOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSkillsCharGen::OnEnterPointsButton));
        m_aPointsButtons[i].AddEvent(SWGUIEVENT_UNFOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSkillsCharGen::OnExitPointsButton));
        m_aPointsButtons[i].m_tail.m_nUserData = i;

        // Single-point-buy rows are the class skills; their caption and points button get the bonus
        // tint, the rest plain text.
        int nCost = static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)
                        ->GetSkillPointCostForSkillRank(static_cast<unsigned short>(i), 1);
        if (nCost == 1)
        {
            m_aSkillLabels[i].m_text.m_params.SetDefaultColor(
                *reinterpret_cast<const Vector*>(&CGuiInGame::COLOR_NEW_BONUS));
            m_aPointsButtons[i].m_text.m_params.SetDefaultColor(
                *reinterpret_cast<const Vector*>(&CGuiInGame::COLOR_NEW_BONUS));
        }
        else
        {
            m_aSkillLabels[i].m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
            m_aPointsButtons[i].m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
        }

        // In creation mode the page owns the ranks, so the row is wiped back to zero before it is
        // read. Level-up mode keeps whatever the player already bought.
        if (m_bLevelUp == 0)
        {
            static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetSkillRankBase(static_cast<unsigned short>(i), 0);
            if (i < g_pRules->m_nNumSkills)
                m_pCreature->m_pStats->m_pSkillRanks[i] = 0;
        }

        unsigned char nRank = (i < g_pRules->m_nNumSkills)
                                  ? m_pCreature->m_pStats->m_pSkillRanksBase[i]
                                  : 0;
        m_abSkillRank[i] = nRank;
        m_aPointsButtons[i].m_text.m_params.SetText(CExoString(static_cast<int>(nRank)));

        // The ceiling for this row is (level + 3), halved unless the creature may buy the skill as
        // a class skill.
        int nMaxRank = m_pCreature->m_pStats->GetLevel() + 3;
        if (!m_pCreature->m_pStats->HasClassSkillFeat(static_cast<unsigned short>(i)))
        {
            for (int nClass = 0; nClass < m_pCreature->m_pStats->m_nNumClasses; nClass++)
            {
                unsigned char nClassId = m_pCreature->m_pStats->GetClass(static_cast<unsigned char>(nClass));
                if (g_pRules->m_paClasses[nClassId].IsSkillClassSkill(static_cast<unsigned short>(i)))
                    goto lClassSkillRank;
            }
            nMaxRank >>= 1;
        }
    lClassSkillRank:;

        // The arrows are shown only on a row that can move; the minus needs a rank to give back,
        // the plus needs headroom under the ceiling.
        if (m_bLevelUp == 0)
        {
            m_aMinusButtons[i].m_tail.bVisible =
                (((i < g_pRules->m_nNumSkills)
                      ? m_pCreature->m_pStats->m_pSkillRanksBase[i]
                      : 0) != 0);
        }
        else
        {
            m_aMinusButtons[i].m_tail.bVisible =
                (m_abSkillRank[i] != static_cast<signed char>((i < g_pRules->m_nNumSkills)
                                                       ? m_pCreature->m_pStats->m_pSkillRanksBase[i]
                                                       : 0));
        }
        m_aMinusButtons[i].m_tail.bFocusOnHover = 0;
        m_aPlusButtons[i].m_tail.bVisible =
            (nMaxRank != static_cast<signed char>((i < g_pRules->m_nNumSkills)
                                           ? m_pCreature->m_pStats->m_pSkillRanksBase[i]
                                           : 0));
        m_aPlusButtons[i].m_tail.bFocusOnHover = 0;

        m_aMinusButtons[i].AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSkillsCharGen::OnMinusButtonPushed));
        m_aPlusButtons[i].AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSkillsCharGen::OnPlusButtonPushed));

        // Only when the page may not be modified, and only for a skill this character cannot buy at
        // all: skill.2da's grant flag is clear, or it is a droid-only skill on a droid. G0-T0 may buy
        // skill 2 (Stealth) despite being a droid, the one hardcoded exception.
        if (!m_bCanModify)
        {
            bool bIsG0T0 = false;
            if (i == 2)
                bIsG0T0 = m_pCreature->GetServerCreature()->m_sTag.CompareNoCase("g0t0") != 0;

            unsigned char nFlags = g_pRules->m_paSkills[i].m_flags;
            if (!(nFlags & SKILL_FLAG_NPC_CAN_USE) || (m_bIsDroid && !(bIsG0T0 | ((nFlags >> 5) & 1))))
            {
                m_aSkillLabels[i].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
                m_aPointsButtons[i].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
                m_aPlusButtons[i].m_tail.bVisible = 0;
            }
        }
    }

    // The three navigation buttons get no highlight and the base panel's own button handlers.
    m_btnAccept.m_tail.bFocusOnHover = 0;
    m_btnAccept.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnAButtonPressed));
    m_btnRecommended.m_tail.bFocusOnHover = 0;
    m_btnRecommended.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnYButtonPressed));
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));

    // For the point counter, the level's allowance and the pool it seeds are one number, and the
    // caption above it is the running total.
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddSkillPointsPerLevel();
    m_nPoints = m_pCreature->m_pStats->m_nSkillPointsRemaining;
    m_nSkillPointsPerLevel = m_nPoints;
    m_lblSelections.m_text.m_params.SetText(CExoString(m_nPoints));

    SetActiveControl(&m_aPointsButtons[m_nSelectedRow], 0);
}
#endif

CSWGuiSkillsCharGen::~CSWGuiSkillsCharGen()
{
    m_listBox.ClearItems();
}

void CSWGuiSkillsCharGen::OnPanelAdded()
{
    CSWGuiPanel::OnPanelAdded();
    // An app-wide window, so both object ids are the invalid-object sentinel.
    g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_ENTER_SKILL_SCREEN_CHAR_GEN, OBJECT_INVALID, OBJECT_INVALID, 0);
}

int CSWGuiSkillsCharGen::IsClassSkill(unsigned short nSkill)
{
#if !KOTOR1  // K1 has no class-skill feats: only the classes' own lists count
    if (m_pCreature->m_pStats->HasClassSkillFeat(nSkill))
        return 1;
#endif

    for (int i = 0; i < m_pCreature->m_pStats->m_nNumClasses; i++)
    {
        unsigned char nClass = m_pCreature->m_pStats->GetClass(static_cast<unsigned char>(i));
        if (g_pRules->m_paClasses[nClass].IsSkillClassSkill(nSkill))
            return 1;
    }
    return 0;
}

void CSWGuiSkillsCharGen::OnExitPointsButton(CSWGuiControl* pControl)
{
    m_aSkillLabels[pControl->m_tail.m_nUserData].m_text.m_params.bHilight = 0;
    m_aPointsButtons[pControl->m_tail.m_nUserData].m_text.m_params.bHilight = 0;
}

// Routes the button through the same "decrease" event the trigger sends.
void CSWGuiSkillsCharGen::OnMinusButtonPushed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    HandleInputEvent(INPUTEVENT_GUI_DPAD_LEFT, 1);
}

void CSWGuiSkillsCharGen::OnPlusButtonPushed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    HandleInputEvent(INPUTEVENT_GUI_DPAD_RIGHT, 1);
}

void CSWGuiSkillsCharGen::ConfirmAccept(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (!g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted)
        return;

    CompletePanel();
}

// Routes the A, B and Y buttons, the two triggers onto the plus and minus buttons, and the
// page-up/page-down events onto the description box. Every button release falls through to the
// base, which is why the base call sits after the switch rather than in a default arm.
void CSWGuiSkillsCharGen::HandleInputEvent(int nEvent, int nArg)
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

        case INPUTEVENT_GUI_DPAD_LEFT:
        case INPUTEVENT_GUI_KEY_LEFT:
            m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            OnMinusButton();
            break;

        case INPUTEVENT_GUI_DPAD_RIGHT:
        case INPUTEVENT_GUI_KEY_RIGHT:
            m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            OnPlusButton();
            break;

        case INPUTEVENT_GUI_Y:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            OnRecommendButton();
            break;

#if KOTOR1
        case INPUTEVENT_GUI_X:
            // K1: page down scrolls the description box ten lines at a time, and back to the top
            // from the bottom.
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            if (m_listBox.m_scrollBar.m_nCurValue == m_listBox.m_scrollBar.m_nMaxValue)
            {
                m_listBox.HandleInputEvent(SWGUIEVENT_SCROLL_PAGE_UP, 1);
            }
            else
            {
                for (int i = 0; i < 10; i++)
                    m_listBox.HandleInputEvent(SWGUIEVENT_SCROLL_LINE_DOWN, 1);
            }
            break;
#endif

        case INPUTEVENT_GUI_SCROLL_UP:
            m_listBox.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
            break;

        case INPUTEVENT_GUI_SCROLL_DOWN:
            m_listBox.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
            break;
        }
    }

    CSWGuiPanel::HandleInputEvent(nEvent, nArg);
}

// Throws the page away on B. Create mode undoes every skill the player bought, each rank back to 0
// with the point pool emptied, before handing control to the owning page; level-up mode leaves the
// server's real stats alone.
void CSWGuiSkillsCharGen::HandleCancelButton()
{
    if (m_pParentPanel == 0)
        return;

    if (m_bLevelUp == 0)
    {
        for (int i = 0; i < 8; i++)
        {
            static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetSkillRankBase(static_cast<unsigned short>(i), 0);
            // The raw rank array is only as long as skill.2da; the setter above already clamps.
            if (i < g_pRules->m_nNumSkills)
                m_pCreature->m_pStats->m_pSkillRanks[i] = 0;
        }
        m_pCreature->m_pStats->m_nSkillPointsRemaining = 0;
    }

    m_pGuiManager->PopModalPanel();
    m_pParentPanel->SetVisible(1);

    SetToBeDeleted(0);

    if (m_bLevelUp != 0)
    {
        static_cast<CSWGuiLevelUpPanel*>(m_pParentPanel)->CancelledByChild();
        return;
    }
    static_cast<CSWGuiCustomPanel*>(m_pParentPanel)->CancelledByChild();
}

// Gives a skill point back on the selected row. The floor is the creature's purchased rank, not
// the rank the row displays, which carries the class-skill bonus and the level-up allowance.
// Lowering a row to that floor is the no-op the early return checks. The caption, the point
// counter, the description and both arrows are repainted afterwards.
void CSWGuiSkillsCharGen::OnMinusButton()
{
    CSWCLevelUpStats* pStats = static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats);

    signed char nBase = static_cast<unsigned short>(m_nSelectedRow) < g_pRules->m_nNumSkills
                            ? static_cast<signed char>(pStats->m_pSkillRanksBase[static_cast<unsigned short>(m_nSelectedRow)])
                            : 0;

    if (nBase == m_abSkillRank[m_nSelectedRow])
        return;

    pStats->DecreaseSkillRank(static_cast<unsigned short>(m_nSelectedRow), m_abSkillRank[m_nSelectedRow]);

    m_nPoints = m_pCreature->m_pStats->m_nSkillPointsRemaining;

    CExoString sRank(static_cast<unsigned short>(m_nSelectedRow) < g_pRules->m_nNumSkills
                         ? static_cast<signed char>(m_pCreature->m_pStats->m_pSkillRanksBase[static_cast<unsigned short>(m_nSelectedRow)])
                         : 0);
    m_aPointsButtons[m_nSelectedRow].m_text.m_params.SetText(sRank);

    CExoString sPoints(m_nPoints);
    m_lblSelections.m_text.m_params.SetText(sPoints);

    OnEnterPointsButton(&m_aPointsButtons[m_nSelectedRow]);

    m_aPlusButtons[m_nSelectedRow].m_tail.bVisible = 1;

    if ((static_cast<unsigned short>(m_nSelectedRow) < g_pRules->m_nNumSkills
             ? static_cast<signed char>(m_pCreature->m_pStats->m_pSkillRanksBase[static_cast<unsigned short>(m_nSelectedRow)])
             : 0) == m_abSkillRank[m_nSelectedRow])
    {
        m_aMinusButtons[m_nSelectedRow].m_tail.bVisible = 0;
        m_aMinusButtons[m_nSelectedRow].SetActive(0);
    }
}

// Handles the mouse entering one of the eight skill rows by repainting every row first (the hovered
// one highlighted, the others default or greyed out), then describing the hovered one. A row is
// greyed out when the skill cannot be bought at all: skill.2da's grant flag is clear, or it is a
// droid-only skill on a droid. G0-T0 may buy skill 2 (Stealth) despite the droid gate. In level-up
// mode nothing is greyed, since the rows are the level-up allowance.
#if KOTOR1
// K1 paints from one yellow (hovered) / blue pair or greys out, with no G0-T0 carve-out and no
// single-point-buy recolouring; the class-skill caption only changes its strref.
void CSWGuiSkillsCharGen::OnEnterPointsButton(CSWGuiControl* pControl)
{
    for (int i = 0; i < 8; i++)
    {
        int bDisabled = 0;
        if (!m_bCanModify)
        {
            unsigned char nFlags = g_pRules->m_paSkills[i].m_flags;
            if (!(nFlags & SKILL_FLAG_NPC_CAN_USE))
                bDisabled = 1;
            else if (m_bIsDroid)
                bDisabled = !((nFlags >> 5) & 1);
        }

        if (pControl == &m_aPointsButtons[i])
        {
            m_nSelectedRow = i;
            if (bDisabled)
            {
                m_aPointsButtons[i].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
                m_aSkillLabels[i].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
            }
            else
            {
                m_aPointsButtons[i].m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
                m_aSkillLabels[i].m_text.m_params.SetColor(CGuiInGame::COLOR_YELLOW);
            }
        }
        else if (bDisabled)
        {
            m_aPointsButtons[i].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
            m_aSkillLabels[i].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        }
        else
        {
            m_aPointsButtons[i].m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
            m_aSkillLabels[i].m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
        }
    }

    if (IsClassSkill(static_cast<unsigned short>(m_nSelectedRow)))
        m_lblClassSkill.m_text.m_params.SetStrRef(STRREF_CLASS_SKILL);
    else
        m_lblClassSkill.m_text.m_params.SetStrRef(STRREF_CROSS_CLASS_SKILL);

    int nCost = static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)
                    ->GetSkillPointCostForSkillRank(static_cast<unsigned short>(m_nSelectedRow), 1);
    CExoString sCost(nCost);
    m_lblCostPoints.m_text.m_params.SetText(sCost);

    SetDescription(m_asSkillDescriptions[m_nSelectedRow]);

    m_aSkillLabels[m_nSelectedRow].m_text.m_params.bHilight |= 1;
    m_aPointsButtons[m_nSelectedRow].m_text.m_params.bHilight |= 1;
}
#else
void CSWGuiSkillsCharGen::OnEnterPointsButton(CSWGuiControl* pControl)
{
    for (int i = 0; i < 8; i++)
    {
        int bDisabled = 0;
        if (!m_bCanModify)
        {
            CSWSkill* pSkill = &g_pRules->m_paSkills[i];
            bool bIsG0T0 = false;
            if (i == 2)
                bIsG0T0 = m_pCreature->GetServerCreature()->m_sTag.CompareNoCase("g0t0") != 0;

            unsigned char nFlags = pSkill->m_flags;
            if (!(nFlags & SKILL_FLAG_NPC_CAN_USE))
                bDisabled = 1;
            else if (m_bIsDroid)
                bDisabled = !(bIsG0T0 | ((nFlags >> 5) & 1));
        }

        if (pControl == &m_aPointsButtons[i])
        {
            m_nSelectedRow = i;
            if (bDisabled)
            {
                m_aPointsButtons[i].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
                m_aSkillLabels[i].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
            }
            else
            {
                m_aPointsButtons[i].m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
                m_aSkillLabels[i].m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
            }
        }
        else if (bDisabled)
        {
            m_aPointsButtons[i].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
            m_aSkillLabels[i].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        }
        else
        {
            m_aPointsButtons[i].m_text.m_params.RestoreDefaultColor();
            m_aSkillLabels[i].m_text.m_params.RestoreDefaultColor();
        }
    }

    // A class skill for this creature gets the "bonus" caption and colour.
    if (IsClassSkill(static_cast<unsigned short>(m_nSelectedRow)))
    {
        m_lblClassSkill.m_text.m_params.SetStrRef(STRREF_CLASS_SKILL);
        m_lblClassSkill.m_text.m_params.SetColor(*reinterpret_cast<const Vector*>(&CGuiInGame::COLOR_NEW_BONUS));
    }
    else
    {
        m_lblClassSkill.m_text.m_params.SetStrRef(STRREF_CROSS_CLASS_SKILL);
        m_lblClassSkill.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
    }

    // What the next rank costs.
    int nCost = static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)
                    ->GetSkillPointCostForSkillRank(static_cast<unsigned short>(m_nSelectedRow), 1);
    CExoString sCost(nCost);
    m_lblCostPoints.m_text.m_params.SetText(sCost);

    // The description was resolved in the constructor, so this only retargets the box.
    SetDescription(m_asSkillDescriptions[m_nSelectedRow]);

    m_aSkillLabels[m_nSelectedRow].m_text.m_params.bHilight |= 1;
    m_aPointsButtons[m_nSelectedRow].m_text.m_params.bHilight |= 1;

    if (nCost == 1)
    {
        m_aSkillLabels[m_nSelectedRow].m_text.m_params.SetColor(*reinterpret_cast<const Vector*>(&CGuiInGame::COLOR_NEW_BONUS));
        m_aPointsButtons[m_nSelectedRow].m_text.m_params.SetColor(*reinterpret_cast<const Vector*>(&CGuiInGame::COLOR_NEW_BONUS));
    }
}
#endif

// Spends a point to raise the selected skill by one rank. Three refusals come first, each with
// its own message: Stealth and Treat Injury are closed to droids, with Stealth reopened for the
// one tagged "g0t0"; Persuade is closed in level-up mode; and the cost must be affordable with the
// row below its ceiling. The ceiling is (level + 3), halved for a skill that is not a class
// skill.
void CSWGuiSkillsCharGen::OnPlusButton()
{
    int nRefuseMsg = 0;
    if (m_nSelectedRow == SKILL_STEALTH || m_nSelectedRow == SKILL_TREAT_INJURY)
    {
#if KOTOR1
        if (m_bIsDroid)  // K1: no G0-T0 carve-out
            nRefuseMsg = STRREF_DROIDS_CANNOT_USE_SKILL;
#else
        bool bIsG0T0 = false;
        if (m_nSelectedRow == SKILL_STEALTH)
        {
            if (m_pCreature->GetServerCreature() != 0)
                bIsG0T0 = m_pCreature->GetServerCreature()->m_sTag.CompareNoCase("g0t0") != 0;
        }
        if ((bIsG0T0 == false) & (m_bIsDroid != 0))
            nRefuseMsg = STRREF_DROIDS_CANNOT_USE_SKILL;
#endif
    }
    else if (m_nSelectedRow == SKILL_PERSUADE && !m_bCanModify)
    {
        nRefuseMsg = STRREF_ONLY_MAIN_CHARACTER_PERSUADE;
    }

    if (nRefuseMsg != 0)
    {
        CSWGuiMessageBox* pBox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetCallback(0, 0, 0);
        pBox->SetMessage(nRefuseMsg);
        m_pGuiManager->AddPanel(pBox, 1, 1);
        return;
    }

    // A cross-class skill only reaches half the ceiling.
    int nMaxRank = m_pCreature->m_pStats->GetLevel() + 3;
    bool bCrossClass = !IsClassSkill(static_cast<unsigned short>(m_nSelectedRow));
    if (bCrossClass)
        nMaxRank >>= 1;

    int nPointsLeft = m_pCreature->m_pStats->m_nSkillPointsRemaining;
    int nCost = static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)
                    ->GetSkillPointCostForSkillRank(static_cast<unsigned short>(m_nSelectedRow), 1);

    signed char nBase =
        static_cast<signed char>(static_cast<unsigned short>(m_nSelectedRow) < g_pRules->m_nNumSkills
                          ? m_pCreature->m_pStats->m_pSkillRanksBase[static_cast<unsigned short>(m_nSelectedRow)]
                          : 0);

    if ((nCost > nPointsLeft) | (nMaxRank <= nBase))
    {
        // Two refusals share the box, not enough points or the row already at its ceiling.
        CSWGuiMessageBox* pBox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        CExoString sMsg;
        if (nCost > nPointsLeft)
        {
            sMsg = g_pTlkTable->GetSimpleString(STRREF_NOT_ENOUGH_SKILL_POINTS);
        }
        else if (nMaxRank <= nBase)
        {
            if (bCrossClass)
                sMsg = g_pTlkTable->GetSimpleString(STRREF_CROSS_CLASS_SKILL_RANK_LIMIT);
            else
                sMsg = g_pTlkTable->GetSimpleString(STRREF_CLASS_SKILL_RANK_LIMIT);

            g_pTlkTable->SetCustomToken(0, CExoString(nMaxRank));
            g_pTlkTable->ParseStr(sMsg);
        }

        pBox->SetAllowCancel(0);
        pBox->SetCallback(0, 0, 0);
        pBox->SetMessage(sMsg);
        m_pGuiManager->AddPanel(pBox, 1, 1);
        return;
    }

    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->IncreaseSkillRank(static_cast<unsigned short>(m_nSelectedRow));

    m_nPoints = m_pCreature->m_pStats->m_nSkillPointsRemaining;

    CExoString sRank(
        static_cast<signed char>(static_cast<unsigned short>(m_nSelectedRow) < g_pRules->m_nNumSkills
                          ? m_pCreature->m_pStats->m_pSkillRanksBase[static_cast<unsigned short>(m_nSelectedRow)]
                          : 0));
    m_aPointsButtons[m_nSelectedRow].m_text.m_params.SetText(sRank);

    CExoString sPoints(m_nPoints);
    m_lblSelections.m_text.m_params.SetText(sPoints);

    OnEnterPointsButton(&m_aPointsButtons[m_nSelectedRow]);

    // Raising a rank makes the minus arrow legal; reaching the ceiling retires the plus arrow, the
    // mirror of OnMinusButton.
    m_aMinusButtons[m_nSelectedRow].m_tail.bVisible = 1;

    if (nMaxRank == static_cast<signed char>(static_cast<unsigned short>(m_nSelectedRow) < g_pRules->m_nNumSkills
                                      ? m_pCreature->m_pStats->m_pSkillRanksBase[static_cast<unsigned short>(m_nSelectedRow)]
                                      : 0))
    {
        m_aPlusButtons[m_nSelectedRow].m_tail.bVisible = 0;
        m_aPlusButtons[m_nSelectedRow].SetActive(0);
    }
}

// Shows a skill description: retargets the row's text, clears the box, sizes the row to the
// string's ideal height and re-adds it as the box's only row.
void CSWGuiSkillsCharGen::SetDescription(const CExoString& sDescription)
{
    CSWGuiControl* pRow;

    m_lblDescRow.m_text.m_params.SetText(sDescription);
    m_listBox.ClearItems();

    CSWGuiText* pText = &m_lblDescRow.m_text;

    CSWGuiExtent extent = m_lblDescRow.m_extent;
    if (sDescription.GetLength() >= 1)
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
    m_lblDescRow.SetExtent(extent);

    pRow = &m_lblDescRow;
    m_listBox.AddControls(&pRow, 1, 0, 0, 0);
    m_listBox.SetSelectedControl(&m_lblDescRow, 0);
#if KOTOR1 && defined(K2_HOST_BUILD) && !defined(K2_UI_TOUCH)
    m_iconDescScroll.m_tail.bVisible =
        m_listBox.m_scrollBar.m_nMaxValue > m_listBox.m_scrollBar.m_nVisibleValue;
#endif
}

// On A, asks the player to confirm when points are unspent; otherwise commits straight away.
void CSWGuiSkillsCharGen::OnAcceptButton()
{
    if (m_nPoints > 0)
    {
        CSWGuiMessageBox* pBox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(1);
        pBox->SetMessage(STRREF_UNSPENT_SKILL_POINTS);
        pBox->SetCallback(this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSkillsCharGen::ConfirmAccept), 0);
        m_pGuiManager->AddPanel(pBox, 1, 1);
        return;
    }

    CompletePanel();
}

// Rebuilds the page from the class's recommendation (Y). A skill build's suggestion is the class's
// default package, so the body resets the pool, seeds the purchased ranks from the page's own
// snapshot, lets the package grant its skills and then repaints every row.
void CSWGuiSkillsCharGen::OnRecommendButton()
{
    unsigned char nClass = m_pCreature->m_pStats->GetClassLeveledUpIn();

    // Create mode throws the pool away and lets the level's allowance re-seed it; level-up mode
    // restores the allowance the constructor recorded and keeps the ranks already bought.
    CSWCLevelUpStats* pStats = static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats);

    if (m_bLevelUp == 0)
    {
        pStats->m_nSkillPointsRemaining = 0;
        pStats->AddSkillPointsPerLevel();
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->ClearSkills();
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->UpdateAbilityModifiedSkillRanks();
    }
    else
    {
        pStats->m_nSkillPointsRemaining = static_cast<unsigned short>(m_nSkillPointsPerLevel);
    }

    // The page's displayed ranks become the purchased ranks the package may extend.
    for (int i = 0; i < g_pRules->m_nNumSkills; i++)
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetSkillRankBase(static_cast<unsigned short>(i),
                                                                     m_abSkillRank[i]);

    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->AddDefaultPackageSkills(nClass, m_bCanModify);

    // Then every row is rebuilt as OnEnterPointsButton would: its caption, its arrows and, only in
    // the non-modifiable flow, its grey-out.
    for (int i = 0; i < g_pRules->m_nNumSkills; i++)
    {
        // The row's ceiling is (level + 3), halved for a skill that is not a class skill.
        int nMaxRank = m_pCreature->m_pStats->GetLevel() + 3;
        if (!IsClassSkill(static_cast<unsigned short>(i)))
            nMaxRank >>= 1;

        m_aPointsButtons[i].m_text.m_params.SetText(CExoString(
            static_cast<signed char>((i < g_pRules->m_nNumSkills)
                              ? m_pCreature->m_pStats->m_pSkillRanksBase[i]
                              : 0)));

        // The arrows follow the two cases the constructor uses, except that the plus arrow here is
        // driven by this row's computed ceiling.
        bool bLevelUp = (m_bLevelUp != 0);
        signed char nRank = (i < g_pRules->m_nNumSkills)
                                ? m_pCreature->m_pStats->m_pSkillRanksBase[i]
                                : 0;
        if (!bLevelUp)
        {
            m_aMinusButtons[i].m_tail.bVisible = (nRank != 0);
        }
        else
        {
            m_aMinusButtons[i].m_tail.bVisible = (m_abSkillRank[i] != nRank);
        }
        m_aPlusButtons[i].m_tail.bVisible =
            (nMaxRank != static_cast<signed char>((i < g_pRules->m_nNumSkills)
                                           ? m_pCreature->m_pStats->m_pSkillRanksBase[i]
                                           : 0));

        // The plus arrow is hidden here, not merely de-highlighted; the greyed arm clears the
        // visibility just set.
        if (!m_bCanModify)
        {
#if KOTOR1
            // K1 has no G0-T0 carve-out.
            unsigned char nFlags = g_pRules->m_paSkills[i].m_flags;
            if (!(nFlags & SKILL_FLAG_NPC_CAN_USE) || (m_bIsDroid && !(nFlags & SKILL_FLAG_DROID_CAN_USE)))
#else
            CSWSkill* pSkill = &g_pRules->m_paSkills[i];

            bool bIsG0T0 = false;
            if (i == 2)
                bIsG0T0 = m_pCreature->GetServerCreature()->m_sTag.CompareNoCase("g0t0") != 0;

            unsigned char nFlags = pSkill->m_flags;
            if (!(nFlags & SKILL_FLAG_NPC_CAN_USE) || (m_bIsDroid && !(bIsG0T0 | ((nFlags >> 5) & 1))))
#endif
            {
                m_aSkillLabels[i].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
                m_aPointsButtons[i].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
                m_aPlusButtons[i].m_tail.bVisible = 0;
            }
        }
    }

    m_nPoints = static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->m_nSkillPointsRemaining;
    m_lblSelections.m_text.m_params.SetText(CExoString(m_nPoints));

    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->UpdateAbilityModifiedSkillRanks();
}

// Commits the page: pops it, re-shows the owner, re-seats the panel chain and tells the owner the
// flow finished. The two modes report to different owners.
void CSWGuiSkillsCharGen::CompletePanel()
{
    m_pGuiManager->PopModalPanel();
    m_pParentPanel->SetVisible(1);

    SetToBeDeleted(0);

    if (m_bLevelUp != 0)
    {
        static_cast<CSWGuiLevelUpPanel*>(m_pParentPanel)->SkillSelectionCompleted();
        return;
    }
    static_cast<CSWGuiCustomPanel*>(m_pParentPanel)->SkillCreationCompleted();
}
