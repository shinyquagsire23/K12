#include "k2stdint.h"
#include "swguiabilitieschargen.h"
#include "swgui.h"
#include "swguicontrol.h"
#include "swguiingame.h"
#include "swguicustompanel.h"
#include "swguileveluppanel.h"
#include "nwccreature.h"
#include "nwccreaturestats.h"
#include "nwclass.h"
#include "nwsrules.h"
#include "nwscreature.h"
#include "nwscreaturestats.h"
#include "guiingame.h"
#include "clientexoapp.h"
#include "appmanager.h"
#include "exotlktable.h"
#include "exolocstringinternal.h"
#include "exoresman.h"
#include "port/k2qolapi.h"
#include "nwscriptconst.h"
#ifdef K2_QOL_IMPROVEMENTS
static void K2QolFitCaption(CSWGuiLabel& label)
{
    K2QolMarkFitText(&label.m_text);
    K2QolFitText(label.m_text, label.m_text.m_extent.h);
}
#endif

#include "port/libcstring.h"
#include "exoinput.h"
#include "tutorial.h"
#include "strref.h"

// Adds a controller glyph to a button.
void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#if KOTOR1
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#endif

#if KOTOR1  // this layout has no bonus column, bar labels or portrait
CSWGuiAbilitiesCharGen::CSWGuiAbilitiesCharGen(CSWGuiManager* pGuiManager, CSWCCreature* pCreature,
                                               int nMode)
    : CSWGuiPanel(pGuiManager)
{
    STR_RES strResUnused;

    m_nMode = nMode;
    m_pCreature = pCreature;

    StartLoadFromLayout(CResRef("ABCHRGEN"));

    InitControl(m_lblTitle,        CExoString("MAIN_TITLE_LBL"), 1);
    if (nMode != 0)
        m_lblTitle.m_text.m_params.SetStrRef(STRREF_LEVEL_UP);
    InitControl(m_lblSubTitle,     CExoString("SUB_TITLE_LBL"), 1);
    InitControl(m_lblRemainingBox, CExoString("REMAINING_BOX_LBL"), 1);
    InitControl(m_lblPoints,       CExoString("REMAINING_SELECTIONS_LBL"), 1);
    InitControl(m_lblSelections,   CExoString("SELECTIONS_REMAINING_LBL"), 1);
    InitControl(m_lblDesc,         CExoString("DESC_LBL"), 1);
    InitControl(m_lblCost,         CExoString("COST_LBL"), 1);
    InitControl(m_lblCostPoints,   CExoString("COST_POINTS_LBL"), 1);
    InitControl(m_listBox,         CExoString("LB_DESC"), 1);
    m_listBox.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_lblAbilityMod,   CExoString("LBL_ABILITY_MOD"), 1);
    InitControl(m_lblModifier,     CExoString("LBL_MODIFIER"), 1);
    InitControl(m_btnAccept,       CExoString("BTN_ACCEPT"), 1);
    InitControl(m_btnRecommended,  CExoString("BTN_RECOMMENDED"), 1);
    InitControl(m_btnBack,         CExoString("BTN_BACK"), 1);

    InitControl(m_aPointsButtons[0], CExoString("STR_POINTS_BTN"), 1);
    InitControl(m_aStatLabels[0],    CExoString("STR_LBL"), 1);
    InitControl(m_aMinusButtons[0],  CExoString("STR_MINUS_BTN"), 1);
    InitControl(m_aPlusButtons[0],   CExoString("STR_PLUS_BTN"), 1);
    m_anStatStrRefs[0] = STRREF_STRENGTH_DESCRIPTION;

    InitControl(m_aPointsButtons[1], CExoString("DEX_POINTS_BTN"), 1);
    InitControl(m_aStatLabels[1],    CExoString("DEX_LBL"), 1);
    InitControl(m_aMinusButtons[1],  CExoString("DEX_MINUS_BTN"), 1);
    InitControl(m_aPlusButtons[1],   CExoString("DEX_PLUS_BTN"), 1);
    m_anStatStrRefs[1] = STRREF_DEXTERITY_DESCRIPTION;

    InitControl(m_aPointsButtons[2], CExoString("CON_POINTS_BTN"), 1);
    InitControl(m_aStatLabels[2],    CExoString("CON_LBL"), 1);
    InitControl(m_aMinusButtons[2],  CExoString("CON_MINUS_BTN"), 1);
    InitControl(m_aPlusButtons[2],   CExoString("CON_PLUS_BTN"), 1);
    m_anStatStrRefs[2] = STRREF_CONSTITUTION_DESCRIPTION;

    InitControl(m_aPointsButtons[3], CExoString("WIS_POINTS_BTN"), 1);
    InitControl(m_aStatLabels[3],    CExoString("WIS_LBL"), 1);
    InitControl(m_aMinusButtons[3],  CExoString("WIS_MINUS_BTN"), 1);
    InitControl(m_aPlusButtons[3],   CExoString("WIS_PLUS_BTN"), 1);
    m_anStatStrRefs[3] = STRREF_WISDOM_DESCRIPTION;

    InitControl(m_aPointsButtons[4], CExoString("INT_POINTS_BTN"), 1);
    InitControl(m_aStatLabels[4],    CExoString("INT_LBL"), 1);
    InitControl(m_aMinusButtons[4],  CExoString("INT_MINUS_BTN"), 1);
    InitControl(m_aPlusButtons[4],   CExoString("INT_PLUS_BTN"), 1);
    m_anStatStrRefs[4] = STRREF_INTELLIGENCE_DESCRIPTION;

    InitControl(m_aPointsButtons[5], CExoString("CHA_POINTS_BTN"), 1);
    InitControl(m_aStatLabels[5],    CExoString("CHA_LBL"), 1);
    InitControl(m_aMinusButtons[5],  CExoString("CHA_MINUS_BTN"), 1);
    InitControl(m_aPlusButtons[5],   CExoString("CHA_PLUS_BTN"), 1);
    m_anStatStrRefs[5] = STRREF_CHARISMA_DESCRIPTION;

    CSWGuiExtent extAccept = m_btnAccept.m_extent;
    CSWGuiExtent extBack = m_btnBack.m_extent;
    m_btnBack.SetExtent(extAccept);
    m_btnAccept.SetExtent(extBack);

    StopLoadFromLayout();
    ScaleExtentForResolution();

    if (m_nMode == 0)
    {
        CSWCCreatureStats* pStats = m_pCreature->m_pStats;
        m_anStatValues[0] = pStats->m_abScoresBase[0];
        m_anStatValues[1] = pStats->m_abScoresBase[1];
        m_anStatValues[2] = pStats->m_abScoresBase[2];
        m_anStatValues[3] = pStats->m_abScoresBase[4];
        m_anStatValues[4] = pStats->m_abScoresBase[3];
        m_anStatValues[5] = pStats->m_abScoresBase[5];
    }
    else
    {
        int nSTR, nDEX, nCON, nWIS, nINT, nCHA;
        CSWSCreature* pServer = m_pCreature->GetServerCreature();
        if (pServer != 0)
        {
            CSWSCreatureStats* pStats = pServer->m_pStats;
            nSTR = pStats->m_nSTRBase;
            nDEX = pStats->m_nDEXBase;
            nCON = pStats->m_nCONBase;
            nWIS = pStats->m_nWISBase;
            nINT = pStats->m_nINTBase;
            nCHA = pStats->m_nCHABase;
        }
        else
        {
            CSWCCreatureStats* pStats = m_pCreature->m_pStats;
            nSTR = pStats->m_abScoresBase[0];
            nDEX = pStats->m_abScoresBase[1];
            nCON = pStats->m_abScoresBase[2];
            nWIS = pStats->m_abScoresBase[4];
            nINT = pStats->m_abScoresBase[3];
            nCHA = pStats->m_abScoresBase[5];
        }
        m_anStatValues[0] = nSTR;
        m_anStatValues[1] = nDEX;
        m_anStatValues[2] = nCON;
        m_anStatValues[3] = nWIS;
        m_anStatValues[4] = nINT;
        m_anStatValues[5] = nCHA;
        m_anBaseValues[0] = nSTR;
        m_anBaseValues[1] = nDEX;
        m_anBaseValues[2] = nCON;
        m_anBaseValues[3] = nWIS;
        m_anBaseValues[4] = nINT;
        m_anBaseValues[5] = nCHA;
    }

    for (int i = 0; i < 6; i++)
    {
        m_aPointsButtons[i].AddEvent(SWGUIEVENT_FOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiAbilitiesCharGen::OnEnterPointsButton));
        m_aPointsButtons[i].AddEvent(SWGUIEVENT_UNFOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiAbilitiesCharGen::OnExitPointsButton));
        m_aPointsButtons[i].m_text.m_params.SetText(CExoString(m_anStatValues[i]));
        m_aPointsButtons[i].m_tail.m_nUserData = i;

        if (m_nMode == 0)
        {
            m_aMinusButtons[i].m_tail.bVisible = (m_anStatValues[i] != 8);
            m_aPlusButtons[i].m_tail.bVisible = (m_anStatValues[i] != 18);
        }
        else
        {
            m_aMinusButtons[i].m_tail.bVisible = 0;
        }
        m_aMinusButtons[i].m_tail.bFocusOnHover = 0;
        m_aPlusButtons[i].m_tail.bFocusOnHover = 0;

        m_aMinusButtons[i].AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiAbilitiesCharGen::OnMinusButtonPushed));
        m_aPlusButtons[i].AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiAbilitiesCharGen::OnPlusButtonPushed));
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

    if (m_nMode == 0)
    {
        m_nPoints = m_pCreature->m_pStats->m_nAbilityPointsRemaining;
    }
    else
    {
        m_nPoints = (m_pCreature->m_pStats->GetLevel() & 3) == 0;
        m_lblSelections.m_text.m_params.SetText(CExoString("Points Remaining"));
    }
    m_lblPoints.m_text.m_params.SetText(CExoString(m_nPoints));

    CSWGuiLabel* pProto = static_cast<CSWGuiLabel*>(m_listBox.m_pProtoItem);
    m_lblDescRow.Initialize(pProto->m_extent, pProto, 1.0f);

    SetActiveControl(&m_aPointsButtons[0], 0);
    m_nHoveredRow = 0;

    IosAddButtonIcon(&m_btnRecommended, 'y');
    IosAddButtonIcon(&m_btnAccept, 'a');
    IosAddButtonIcon(&m_btnBack, 'b');
}
#else
CSWGuiAbilitiesCharGen::CSWGuiAbilitiesCharGen(CSWGuiManager* pGuiManager, CSWCCreature* pCreature,
                                               int nMode)
    : CSWGuiPanel(pGuiManager)
{
    STR_RES strResUnused;

    m_nMode = nMode;
    m_pCreature = pCreature;

    StartLoadFromLayout(CResRef("ABCHRGEN_p"), 1, 1, 1);

    InitControl(m_lblTitle,        CExoString("MAIN_TITLE_LBL"), 1, 1, 0, 0, 0);
    if (nMode != 0)
        m_lblTitle.m_text.m_params.SetStrRef(STRREF_LEVEL_UP);
    InitControl(m_lblSubTitle,     CExoString("SUB_TITLE_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblRemainingBox, CExoString("REMAINING_BOX_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblPoints,       CExoString("REMAINING_SELECTIONS_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblSelections,   CExoString("SELECTIONS_REMAINING_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblCost,         CExoString("COST_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblCostPoints,   CExoString("COST_POINTS_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_listBox,         CExoString("LB_DESC"), 1, 1, 0, 0, 0);
    m_listBox.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_btnAccept,       CExoString("BTN_ACCEPT"), 1, 1, 0, 0, 0);
    InitControl(m_btnRecommended,  CExoString("BTN_RECOMMENDED"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack,         CExoString("BTN_BACK"), 1, 1, 0, 0, 0);
    AspyrAddButtonIcon(&m_btnRecommended, 'y');

    InitControl(m_aPointsButtons[0], CExoString("STR_POINTS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aStatLabels[0],    CExoString("STR_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_aMinusButtons[0],  CExoString("STR_MINUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aPlusButtons[0],   CExoString("STR_PLUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aBonusLabels[0],   CExoString("LBL_BONUS_STR"), 1, 1, 0, 0, 0);
    m_anStatStrRefs[0] = STRREF_STRENGTH_DESCRIPTION;

    InitControl(m_aPointsButtons[1], CExoString("DEX_POINTS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aStatLabels[1],    CExoString("DEX_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_aMinusButtons[1],  CExoString("DEX_MINUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aPlusButtons[1],   CExoString("DEX_PLUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aBonusLabels[1],   CExoString("LBL_BONUS_DEX"), 1, 1, 0, 0, 0);
    m_anStatStrRefs[1] = STRREF_DEXTERITY_DESCRIPTION;

    InitControl(m_aPointsButtons[2], CExoString("CON_POINTS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aStatLabels[2],    CExoString("CON_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_aMinusButtons[2],  CExoString("CON_MINUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aPlusButtons[2],   CExoString("CON_PLUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aBonusLabels[2],   CExoString("LBL_BONUS_CON"), 1, 1, 0, 0, 0);
    m_anStatStrRefs[2] = STRREF_CONSTITUTION_DESCRIPTION;

    InitControl(m_aPointsButtons[3], CExoString("WIS_POINTS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aStatLabels[3],    CExoString("WIS_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_aMinusButtons[3],  CExoString("WIS_MINUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aPlusButtons[3],   CExoString("WIS_PLUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aBonusLabels[3],   CExoString("LBL_BONUS_WIS"), 1, 1, 0, 0, 0);
    m_anStatStrRefs[3] = STRREF_WISDOM_DESCRIPTION;

    InitControl(m_aPointsButtons[4], CExoString("INT_POINTS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aStatLabels[4],    CExoString("INT_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_aMinusButtons[4],  CExoString("INT_MINUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aPlusButtons[4],   CExoString("INT_PLUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aBonusLabels[4],   CExoString("LBL_BONUS_INT"), 1, 1, 0, 0, 0);
    m_anStatStrRefs[4] = STRREF_INTELLIGENCE_DESCRIPTION;

    InitControl(m_aPointsButtons[5], CExoString("CHA_POINTS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aStatLabels[5],    CExoString("CHA_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_aMinusButtons[5],  CExoString("CHA_MINUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aPlusButtons[5],   CExoString("CHA_PLUS_BTN"), 1, 1, 0, 0, 0);
    InitControl(m_aBonusLabels[5],   CExoString("LBL_BONUS_CHA"), 1, 1, 0, 0, 0);
    m_anStatStrRefs[5] = STRREF_CHARISMA_DESCRIPTION;

    InitControl(m_aBarLabels[0],   CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[1],   CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[2],   CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[3],   CExoString("LBL_BAR4"), 1, 1, 0, 0, 0);
    InitControl(m_lblPortrait,     CExoString("LBL_PORTRAIT"), 1, 1, 0, 0, 0);

    StopLoadFromLayout();
#ifdef K2_QOL_IMPROVEMENTS
    for (int i = 0; i < 6; i++)
    {
        K2QolFitCaption(m_aStatLabels[i]);
        K2QolFitCaption(m_aBonusLabels[i]);
    }
#endif

    // Take the three colour pairs off the first row; every row is painted from these later.
    m_avRowColors[0] = m_aStatLabels[0].m_text.m_params.m_color;
    m_avRowColors[1] = CGuiInGame::COLOR_NEW_HIGHLIGHT;
    m_avRowColors[2] = *reinterpret_cast<Vector*>(&m_aStatLabels[0].m_border.m_params.m_color);
    m_avRowColors[3] = CGuiInGame::COLOR_NEW_HIGHLIGHT;
    m_avRowColors[4] = *reinterpret_cast<Vector*>(&m_aPlusButtons[0].m_border.m_params.m_color);
    m_avRowColors[5] = CGuiInGame::COLOR_NEW_HIGHLIGHT;

    // Seed the six scores in on-screen order (STR, DEX, CON, WIS, INT, CHA). Create mode reads the
    // pre-racial base off the client stats; level-up mode reads the live scores off the server
    // creature and keeps a copy as the floor they cannot be lowered below.
    if (m_nMode == 0)
    {
        CSWCCreatureStats* pStats = m_pCreature->m_pStats;
        m_anStatValues[0] = pStats->m_abScoresBase[0];
        m_anStatValues[1] = pStats->m_abScoresBase[1];
        m_anStatValues[2] = pStats->m_abScoresBase[2];
        m_anStatValues[3] = pStats->m_abScoresBase[4];
        m_anStatValues[4] = pStats->m_abScoresBase[3];
        m_anStatValues[5] = pStats->m_abScoresBase[5];
    }
    else
    {
        int nSTR, nDEX, nCON, nWIS, nINT, nCHA;
        CSWSCreature* pServer = m_pCreature->GetServerCreature();
        if (pServer != 0)
        {
            CSWSCreatureStats* pStats = pServer->m_pStats;
            nSTR = pStats->m_nSTRBase;
            nDEX = pStats->m_nDEXBase;
            nCON = pStats->m_nCONBase;
            nWIS = pStats->m_nWISBase;
            nINT = pStats->m_nINTBase;
            nCHA = pStats->m_nCHABase;
        }
        else
        {
            CSWCCreatureStats* pStats = m_pCreature->m_pStats;
            nSTR = pStats->m_abScoresBase[0];
            nDEX = pStats->m_abScoresBase[1];
            nCON = pStats->m_abScoresBase[2];
            nWIS = pStats->m_abScoresBase[4];
            nINT = pStats->m_abScoresBase[3];
            nCHA = pStats->m_abScoresBase[5];
        }
        m_anStatValues[0] = nSTR;
        m_anStatValues[1] = nDEX;
        m_anStatValues[2] = nCON;
        m_anStatValues[3] = nWIS;
        m_anStatValues[4] = nINT;
        m_anStatValues[5] = nCHA;
        m_anBaseValues[0] = nSTR;
        m_anBaseValues[1] = nDEX;
        m_anBaseValues[2] = nCON;
        m_anBaseValues[3] = nWIS;
        m_anBaseValues[4] = nINT;
        m_anBaseValues[5] = nCHA;
    }

    for (int i = 0; i < 6; i++)
    {
        // A points row reports both enter and exit; the plus/minus buttons only the press.
        m_aPointsButtons[i].AddEvent(SWGUIEVENT_FOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiAbilitiesCharGen::OnEnterPointsButton));
        m_aPointsButtons[i].AddEvent(SWGUIEVENT_UNFOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiAbilitiesCharGen::OnExitPointsButton));
        m_aPointsButtons[i].m_text.m_params.SetText(CExoString(m_anStatValues[i]));
        m_aPointsButtons[i].m_tail.m_nUserData = i;

        if (m_nMode == 0)
        {
            m_aMinusButtons[i].m_tail.bVisible = (m_anStatValues[i] != 8);
            m_aPlusButtons[i].m_tail.bVisible = (m_anStatValues[i] != 18);
        }
        else
        {
            m_aMinusButtons[i].m_tail.bVisible = 0;
        }
        m_aMinusButtons[i].m_tail.bFocusOnHover = 0;
        m_aPlusButtons[i].m_tail.bFocusOnHover = 0;

        m_aMinusButtons[i].AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiAbilitiesCharGen::OnMinusButtonPushed));
        m_aPlusButtons[i].AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiAbilitiesCharGen::OnPlusButtonPushed));

        // The "+2" column.
        CExoString sMod;
        signed char nMod = m_pCreature->m_pStats->CalcStatModifier(static_cast<unsigned char>(m_anStatValues[i]));
        if (nMod < 0)
            sMod = CExoString(static_cast<int>(nMod));
        else if (nMod != 0)
            sMod = CExoString("+") + CExoString(static_cast<int>(nMod));
        else
            sMod = "-";
        m_aBonusLabels[i].m_text.m_params.SetText(sMod);
    }

    // The three navigation buttons get no highlight, and the base panel's own button handlers.
    m_btnAccept.m_tail.bFocusOnHover = 0;
    m_btnAccept.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnAButtonPressed));
    m_btnRecommended.m_tail.bFocusOnHover = 0;
    m_btnRecommended.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnYButtonPressed));
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));

    // The point counter. In create mode it is the creature's own unspent-points byte; in level-up
    // mode it is one point per four levels, and the caption above it switches to the "remaining
    // selections" line. Both modes land in the same local before the one SetText.
    if (m_nMode == 0)
    {
        m_nPoints = m_pCreature->m_pStats->m_nAbilityPointsRemaining;
    }
    else
    {
        m_nPoints = (m_pCreature->m_pStats->GetLevel() & 3) == 0;
        m_lblSelections.m_text.m_params.SetStrRef(STRREF_REMAINING_POINTS);
    }
    m_lblPoints.m_text.m_params.SetText(CExoString(m_nPoints));

    // Stand the description row up off the list box's prototype item, then hand it to the box as
    // its one and only row.
    CSWGuiLabel* pProto = static_cast<CSWGuiLabel*>(m_listBox.m_pProtoItem);
    m_lblDescRow.Initialize(pProto->m_extent, pProto, 1.0f);

    SetActiveControl(&m_aPointsButtons[0], 0);
    m_nHoveredRow = 0;

    m_lblPortrait.m_border.m_params.SetFillImage(m_pCreature->GetPortrait(0));
    AspyrAddButtonIcon(&m_btnAccept, 'a');
    AspyrAddButtonIcon(&m_btnBack, 'b');
}
#endif

// The description box drops its rows before the widget teardown runs.
CSWGuiAbilitiesCharGen::~CSWGuiAbilitiesCharGen()
{
    m_listBox.ClearItems();
}

void CSWGuiAbilitiesCharGen::OnPanelAdded()
{
    CSWGuiPanel::OnPanelAdded();
    // Both object ids are the "no object" sentinel.
    g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_ENTER_ATTRIBUTE_SCREEN, OBJECT_INVALID, OBJECT_INVALID, 0);
}

// Routes buttons and arrows: A accepts, B cancels, Y recommends, the triggers step the hovered
// row, page up/down scrolls the description. Everything else, including every button release,
// falls through to the base panel.
void CSWGuiAbilitiesCharGen::HandleInputEvent(int nEvent, int nArg)
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
            // Back out to the page that owns us.
            if (m_pParentPanel != 0)
            {
                m_pGuiManager->PopModalPanel();
                m_pParentPanel->SetVisible(1);

                CSWGuiPanel* pPanel = this;
#if KOTOR1  // no panel chain, so this panel is the only one stamped
                pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
                do
                {
                    pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
                    pPanel = pPanel->m_pNextInChain;
                } while (pPanel != 0);
#endif

                if (m_nMode == 0)
                {
                    m_pCreature->m_pStats->m_nAbilityPointsRemaining = 30;
                    static_cast<CSWGuiCustomPanel*>(m_pParentPanel)->CancelledByChild();
                }
                else
                {
                    static_cast<CSWGuiLevelUpPanel*>(m_pParentPanel)->CancelledByChild();
                }
            }
            break;

        case INPUTEVENT_GUI_Y:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            // The recommendation is only useful while points are left to place.
#if KOTOR1
            if (m_nPoints > 0)  // K1 requires points in create mode too
#else
            if (m_nPoints > 0 || m_nMode == 0)
#endif
                OnRecommendButton();
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

// Handles the mouse entering one of the six score rows: remembers which, paints that row and its
// caption with the highlight colour, puts the row's cost in the cost field, shows the tier
// description and lights both captions up.
void CSWGuiAbilitiesCharGen::OnEnterPointsButton(CSWGuiControl* pControl)
{
#if KOTOR1  // K1 paints the rows with the global blue / yellow pair
    Vector* pNormal = &CGuiInGame::COLOR_BLUE;
    Vector* pHighlight = &CGuiInGame::COLOR_YELLOW;
#else
    Vector* pNormal = &m_avRowColors[0];
    Vector* pHighlight = &m_avRowColors[1];
#endif
    Vector* pColor = pNormal;

    if (pControl == &m_aPointsButtons[0])
    {
        m_nHoveredRow = 0;
#if KOTOR1
        pColor = pHighlight;
#else
        pColor = &m_avRowColors[1];
#endif
    }
    m_aPointsButtons[0].m_text.m_params.SetColor(*pColor);
    m_aStatLabels[0].m_text.m_params.SetColor(*pColor);

    pColor = pNormal;
    if (pControl == &m_aPointsButtons[1])
    {
        m_nHoveredRow = 1;
        pColor = pHighlight;
    }
    m_aPointsButtons[1].m_text.m_params.SetColor(*pColor);
    m_aStatLabels[1].m_text.m_params.SetColor(*pColor);

    pColor = pNormal;
    if (pControl == &m_aPointsButtons[2])
    {
        m_nHoveredRow = 2;
        pColor = pHighlight;
    }
    m_aPointsButtons[2].m_text.m_params.SetColor(*pColor);
    m_aStatLabels[2].m_text.m_params.SetColor(*pColor);

    pColor = pNormal;
    if (pControl == &m_aPointsButtons[3])
    {
        m_nHoveredRow = 3;
        pColor = pHighlight;
    }
    m_aPointsButtons[3].m_text.m_params.SetColor(*pColor);
    m_aStatLabels[3].m_text.m_params.SetColor(*pColor);

    pColor = pNormal;
    if (pControl == &m_aPointsButtons[4])
    {
        m_nHoveredRow = 4;
        pColor = pHighlight;
    }
    m_aPointsButtons[4].m_text.m_params.SetColor(*pColor);
    m_aStatLabels[4].m_text.m_params.SetColor(*pColor);

    pColor = pNormal;
    if (pControl == &m_aPointsButtons[5])
    {
        m_nHoveredRow = 5;
        pColor = pHighlight;
    }
    m_aPointsButtons[5].m_text.m_params.SetColor(*pColor);
    m_aStatLabels[5].m_text.m_params.SetColor(*pColor);

    // The tier cost of the next point is 1 up to 13, 2 at 14/15, 3 from 16. Level-up mode always
    // charges a flat point (it has one per four levels).
#if KOTOR1  // the mode test comes first and the flat point returns immediately
    int nCost;
    if (m_nMode != 0)
        nCost = 1;
    else if (m_anStatValues[m_nHoveredRow] > 15)
        nCost = 3;
    else
        nCost = (m_anStatValues[m_nHoveredRow] > 13) ? 2 : 1;
#else
    int nCost = 1;
    if (m_nMode == 0)
    {
        int nScore = m_anStatValues[m_nHoveredRow];
        if (nScore > 15)
            nCost = 3;
        else
            nCost = (nScore > 13) ? 2 : 1;
    }
#endif

    CExoString sCost(nCost);

    CExoString sMod;
    signed char nMod = m_pCreature->m_pStats->CalcStatModifier(
        static_cast<unsigned char>(m_anStatValues[m_nHoveredRow]));
    if (nMod < 0)
        sMod = CExoString(static_cast<int>(nMod));
    else if (nMod != 0)
        sMod = CExoString("+") + CExoString(static_cast<int>(nMod));
    else
        sMod = "-";

    m_lblCostPoints.m_text.m_params.SetText(sCost);
#if KOTOR1
    m_lblAbilityMod.m_text.m_params.SetText(sMod);  // K1 has a separate modifier label
#endif

    SetDescription(static_cast<unsigned int>(m_anStatStrRefs[m_nHoveredRow]));

    m_aStatLabels[m_nHoveredRow].m_text.m_params.bHilight |= 1;
    m_aPointsButtons[m_nHoveredRow].m_text.m_params.bHilight |= 1;
}

// Handles the mouse leaving a row: drops its two highlight flags. The row index comes out of the
// control's user data, which the constructor seeded.
void CSWGuiAbilitiesCharGen::OnExitPointsButton(CSWGuiControl* pControl)
{
    m_aStatLabels[pControl->m_tail.m_nUserData].m_text.m_params.bHilight = 0;
    m_aPointsButtons[pControl->m_tail.m_nUserData].m_text.m_params.bHilight = 0;
}

// A row's minus button was pressed: re-enter as the generic "decrease" event so the trigger and the
// button share one path. The pressed control itself is not consulted.
void CSWGuiAbilitiesCharGen::OnMinusButtonPushed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    HandleInputEvent(INPUTEVENT_GUI_DPAD_LEFT, 1);
}

void CSWGuiAbilitiesCharGen::OnPlusButtonPushed(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    HandleInputEvent(INPUTEVENT_GUI_DPAD_RIGHT, 1);
}

// B: hand control back to the owning page and tell it the player cancelled. Nothing happens without
// an owner.
void CSWGuiAbilitiesCharGen::HandleCancelButton()
{
    if (m_pParentPanel == 0)
        return;

    m_pGuiManager->PopModalPanel();
    m_pParentPanel->SetVisible(1);

    CSWGuiPanel* pPanel = this;
#if KOTOR1  // no panel chain, so this panel is the only one stamped
    pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
    do
    {
        pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
        pPanel = pPanel->m_pNextInChain;
    } while (pPanel != 0);
#endif

    if (m_nMode != 0)
    {
        static_cast<CSWGuiLevelUpPanel*>(m_pParentPanel)->CancelledByChild();
        return;
    }

    m_pCreature->m_pStats->m_nAbilityPointsRemaining = 30;
    static_cast<CSWGuiCustomPanel*>(m_pParentPanel)->CancelledByChild();
}

// Lowers the hovered score, spending points back. Refused at the floor (8 in create mode, the
// level-up base in level-up mode) with the "you cannot lower this" box; otherwise the cost is
// refunded and the row repainted. The minus button then hides at the floor and the plus button
// always comes back.
void CSWGuiAbilitiesCharGen::OnMinusButton()
{
    if (m_nMode == 0)
    {
        if (m_anStatValues[m_nHoveredRow] == 8)
        {
            CSWGuiMessageBox* pBox =
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
            pBox->SetAllowCancel(0);
            pBox->SetMessage(STRREF_ATTRIBUTE_MINIMUM);
            pBox->SetCallback(0, 0, 0);
            m_pGuiManager->AddPanel(pBox, 1, 1);
            return;
        }
    }
    else if (m_anStatValues[m_nHoveredRow] == m_anBaseValues[m_nHoveredRow])
    {
        return;
    }

    if (m_anStatValues[m_nHoveredRow] > 8)
    {
        // Refund the tier cost of the point being given back.
        int nCost = -1;
        if ((m_nMode == 0) & (static_cast<unsigned int>(m_anStatValues[m_nHoveredRow] - 9) > 5))
        {
            nCost = -3;
            if (static_cast<unsigned int>(m_anStatValues[m_nHoveredRow] - 15) < 2)
                nCost = -2;
        }
        m_nPoints -= nCost;
        m_anStatValues[m_nHoveredRow] -= 1;
    }

    CExoString sScore(m_anStatValues[m_nHoveredRow]);
    m_aPointsButtons[m_nHoveredRow].m_text.m_params.SetText(sScore);

#if !KOTOR1  // K1 has no bonus column
    CExoString sMod;
    signed char nMod =
        m_pCreature->m_pStats->CalcStatModifier(static_cast<unsigned char>(m_anStatValues[m_nHoveredRow]));
    if (nMod < 0)
        sMod = CExoString(static_cast<int>(nMod));
    else if (nMod != 0)
        sMod = CExoString("+") + CExoString(static_cast<int>(nMod));
    else
        sMod = "-";
    m_aBonusLabels[m_nHoveredRow].m_text.m_params.SetText(sMod);
#endif

    CExoString sPoints(m_nPoints);
    m_lblPoints.m_text.m_params.SetText(sPoints);

    OnEnterPointsButton(&m_aPointsButtons[m_nHoveredRow]);

    // The minus button disappears exactly where the value cannot go any lower.
    if (m_nMode == 0)
    {
        if (m_anStatValues[m_nHoveredRow] == 8)
        {
            m_aMinusButtons[m_nHoveredRow].m_tail.bVisible = 0;
            m_aMinusButtons[m_nHoveredRow].SetActive(0);
        }
    }
    else if (m_anStatValues[m_nHoveredRow] == m_anBaseValues[m_nHoveredRow])
    {
        m_aMinusButtons[m_nHoveredRow].m_tail.bVisible = 0;
        m_aMinusButtons[m_nHoveredRow].SetActive(0);
    }
    m_aPlusButtons[m_nHoveredRow].m_tail.bVisible = 1;
}

// Raises the hovered score by spending a point. Refused at 18 with the "you cannot raise this"
// box, and in create mode the tier cost must also be affordable. The buttons mirror the minus
// side: the plus button hides once the row is maxed and the minus button comes back.
void CSWGuiAbilitiesCharGen::OnPlusButton()
{
    if (m_nPoints < 1)
        return;

    if (m_nMode == 0)
    {
        if (m_anStatValues[m_nHoveredRow] == 18)
        {
            CSWGuiMessageBox* pBox =
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
            pBox->SetAllowCancel(0);
            pBox->SetMessage(STRREF_ATTRIBUTE_MAXIMUM);
            pBox->SetCallback(0, 0, 0);
            m_pGuiManager->AddPanel(pBox, 1, 1);
            return;
        }
    }

    int nScore = m_anStatValues[m_nHoveredRow];
    int nCost;
    if (m_nMode == 0)
    {
        if (m_nPoints < DetermineCostToIncreaseScore(nScore) || nScore >= 18)
            nCost = 0;
        else
            nCost = DetermineCostToIncreaseScore(nScore);
    }
    else
    {
        nCost = 1;
    }

    if (nCost != 0)
    {
        m_nPoints -= nCost;
        m_anStatValues[m_nHoveredRow] += 1;
    }

    CExoString sScore(m_anStatValues[m_nHoveredRow]);
    m_aPointsButtons[m_nHoveredRow].m_text.m_params.SetText(sScore);

#if !KOTOR1  // K1 has no bonus column
    CExoString sMod;
    signed char nMod = m_pCreature->m_pStats->CalcStatModifier(static_cast<unsigned char>(m_anStatValues[m_nHoveredRow]));
    if (nMod < 0)
        sMod = CExoString(static_cast<int>(nMod));
    else if (nMod != 0)
        sMod = CExoString("+") + CExoString(static_cast<int>(nMod));
    else
        sMod = "-";
    m_aBonusLabels[m_nHoveredRow].m_text.m_params.SetText(sMod);
#endif

    CExoString sPoints(m_nPoints);
    m_lblPoints.m_text.m_params.SetText(sPoints);

    OnEnterPointsButton(&m_aPointsButtons[m_nHoveredRow]);

    if (m_nMode == 0 && m_anStatValues[m_nHoveredRow] == 18)
    {
        m_aPlusButtons[m_nHoveredRow].m_tail.bVisible = 0;
        m_aPlusButtons[m_nHoveredRow].SetActive(0);
    }
    m_aMinusButtons[m_nHoveredRow].m_tail.bVisible = 1;
}

// Handles A: says so and stays put while points are still unspent, otherwise commits the scores.
void CSWGuiAbilitiesCharGen::OnAcceptButton()
{
    if (m_nPoints > 0)
    {
        CSWGuiMessageBox* pBox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetMessage(STRREF_MUST_SPEND_ALL_POINTS);
        pBox->SetCallback(0, 0, 0);
        m_pGuiManager->AddPanel(pBox, 1, 1);
        return;
    }

    CompletePanel();
}

// Handles Y: takes the character's own class suggestion. Create mode takes the class's recommended
// score block wholesale; level-up mode puts a single free point on the class's primary ability, a
// bonus the player is never charged for. Every row is then repainted and the counter zeroed.
void CSWGuiAbilitiesCharGen::OnRecommendButton()
{
    if (m_nMode == 0)
    {
        CSWCCreatureStats* pStats = m_pCreature->m_pStats;
        CSWClass* pClass = &g_pRules->m_paClasses[pStats->GetClassLeveledUpIn()];
        m_anStatValues[0] = pClass->m_abRecommendedScores[0];
        m_anStatValues[1] = pClass->m_abRecommendedScores[1];
        m_anStatValues[2] = pClass->m_abRecommendedScores[2];
        m_anStatValues[3] = pClass->m_abRecommendedScores[4];
        m_anStatValues[4] = pClass->m_abRecommendedScores[3];
        m_anStatValues[5] = pClass->m_abRecommendedScores[5];
    }
    else
    {
        CSWCCreatureStats* pStats = m_pCreature->m_pStats;
        CSWClass* pClass = &g_pRules->m_paClasses[pStats->GetClassLeveledUpIn()];
        switch (pClass->m_nPrimaryAbility)
        {
        case ABILITY_STRENGTH: m_anStatValues[0] += 1; break;
        case ABILITY_DEXTERITY: m_anStatValues[1] += 1; break;
        case ABILITY_CONSTITUTION: m_anStatValues[2] += 1; break;
        case ABILITY_INTELLIGENCE: m_anStatValues[4] += 1; break;
        case ABILITY_WISDOM: m_anStatValues[3] += 1; break;
        case ABILITY_CHARISMA: m_anStatValues[5] += 1; break;
        }
    }

    m_nPoints = 0;
    m_lblPoints.m_text.m_params.SetText(CExoString(0));

    for (int i = 0; i < 6; i++)
    {
        m_aPointsButtons[i].m_text.m_params.SetText(CExoString(m_anStatValues[i]));

#if !KOTOR1  // K1 has no bonus column
        CExoString sMod;
        signed char nMod = m_pCreature->m_pStats->CalcStatModifier(static_cast<unsigned char>(m_anStatValues[i]));
        if (nMod < 0)
            sMod = CExoString(static_cast<int>(nMod));
        else if (nMod != 0)
            sMod = CExoString("+") + CExoString(static_cast<int>(nMod));
        else
            sMod = "-";
        m_aBonusLabels[i].m_text.m_params.SetText(sMod);
#endif

        if (m_nMode == 0)
        {
            m_aMinusButtons[i].m_tail.bVisible = (m_anStatValues[i] != 8);
            m_aPlusButtons[i].m_tail.bVisible = (m_anStatValues[i] != 18);
        }
        else
        {
            // Level-up only steers the minus buttons; the plus buttons keep their state.
            m_aMinusButtons[i].m_tail.bVisible = (m_anStatValues[i] != m_anBaseValues[i]);
        }
    }

    OnEnterPointsButton(&m_aPointsButtons[m_nHoveredRow]);
}

// Returns how many points lowering a score gives back: 1 below 15, 2 at 15 or 16, 3 from 17.
// Level-up mode is a flat 1, and scores below 9 are never refunded at all, which is what the
// unsigned underflow in the guard does.
int CSWGuiAbilitiesCharGen::DetermineCostToDecreaseScore(int nScore)
{
    if ((static_cast<unsigned int>(nScore - 9) > 5) & (m_nMode == 0))
    {
        if (static_cast<unsigned int>(nScore - 15) < 2)
            return -2;
        return (nScore > 16) ? -3 : 0;
    }
    return -1;
}

// Returns the cost of raising a score: 1 up to 13, 2 at 14/15, 3 from 16. Level-up mode is
// always 1.
int CSWGuiAbilitiesCharGen::DetermineCostToIncreaseScore(int nScore)
{
    if (m_nMode != 0)
        return 1;
    if (nScore > 15)
        return 3;
    return (nScore > 13) ? 2 : 1;
}

// Shows the tier named by a dialog.tlk strref in the description box: retargets the row's text,
// clears the box, resizes the row to the string's own ideal height (one font height on an empty
// string) and re-adds it as the box's single selected row.
void CSWGuiAbilitiesCharGen::SetDescription(unsigned int nStrRef)
{
    STR_RES strRes;
    g_pTlkTable->Fetch(nStrRef, strRes, CTlkTable::GENDER_MALE);

    m_lblDescRow.m_text.m_params.SetText(strRes.text);
    m_listBox.ClearItems();

    CSWGuiExtent extent = m_lblDescRow.m_extent;
    // Through a pointer, so the height getter stays a virtual call.
    CSWGuiText* pDescText = &m_lblDescRow.m_text;
#if KOTOR1
    if (strRes.text.GetLength() >= 1)  // K1 hides the length behind an accessor
#else
    if (strRes.text.m_sString != 0 && static_cast<int>(strlen(strRes.text.m_sString)) >= 1)
#endif
        extent.h = pDescText->GetIdealHeight();
    else
        extent.h = pDescText->GetFontHeight();
    m_lblDescRow.SetExtent(extent);

    CSWGuiControl* apRows[1] = { &m_lblDescRow };
    m_listBox.AddControls(apRows, 1, 0, 0, 0);
    m_listBox.SetSelectedControl(&m_lblDescRow, 0);
}

// Commits the six scores. Create mode writes the base values back onto the creature's own stats and
// zeroes the unspent points; level-up mode hands all six plus the remaining points to the owning
// panel, which owns the real stat write. Either way the page is popped, the owner is re-shown, the
// chain is re-seated and the owner is told the flow finished.
void CSWGuiAbilitiesCharGen::CompletePanel()
{
    if (m_nMode == 0)
    {
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetSTRBase(static_cast<unsigned char>(m_anStatValues[0]));
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetCHABase(static_cast<unsigned char>(m_anStatValues[5]));
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetINTBase(static_cast<unsigned char>(m_anStatValues[4]));
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetWISBase(static_cast<unsigned char>(m_anStatValues[3]));
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetCONBase(static_cast<unsigned char>(m_anStatValues[2]));
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetDEXBase(static_cast<unsigned char>(m_anStatValues[1]), 1);
        m_pCreature->m_pStats->m_nAbilityPointsRemaining = 0;
    }
    else
    {
        static_cast<CSWGuiLevelUpPanel*>(m_pParentPanel)->NewAbilityScores(
            m_anStatValues[0], m_anStatValues[5], m_anStatValues[4], m_anStatValues[3],
            m_anStatValues[2], m_anStatValues[1], m_nPoints);
    }

    m_pGuiManager->PopModalPanel();
    m_pParentPanel->SetVisible(1);

    CSWGuiPanel* pPanel = this;
#if KOTOR1  // no panel chain, so this panel is the only one stamped
    pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
    do
    {
        pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
        pPanel = pPanel->m_pNextInChain;
    } while (pPanel != 0);
#endif

    if (m_nMode != 0)
    {
        static_cast<CSWGuiLevelUpPanel*>(m_pParentPanel)->AbilitySelectionCompleted();
        return;
    }
    static_cast<CSWGuiCustomPanel*>(m_pParentPanel)->AbilityCreationCompleted();
}
