#include "k2stdint.h"
#include "swguilevelupchargen.h"
#include "CExoArrayListImpl.h"

#include "swguicontrol.h"
#include "swguileveluppanel.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "nwccreature.h"
#include "nwccreaturestats.h"
#include "swguidrawable.h"
#include "exoresman.h"
#include "aurscene.h"
#include "aurcamera.h"
#include "AURORA/aurbehavior.h"
#include "Vector.h"
#include "Quaternion.h"
#include "nwsrules.h"
#include "nwclass.h"
#include "nwscreature.h"
#include "nwscreaturestats.h"
#include "swguiingamecharacter.h"
#include "nwchandlemessage.h"
#include "swguiexamine.h"
#include "exosound.h"
#include "port/k2qolapi.h"
#include "exoinput.h"
#include "strref.h"

extern int g_bDo3DGUIs;

// Builds the widget wall from the layout, then caches the GUI manager, looks up the creature,
// builds a fresh creature and copies the stats onto it, bumps the class level and fills in the
// class name.
CSWGuiLevelUpCharGen::CSWGuiLevelUpCharGen(CSWGuiManager* pGuiManager, unsigned long long oidCreature,
                                          CSWCCreature* pCreature)
    : CSWGuiPanel(pGuiManager)
{
    CSWCCreature* pOriginal = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(oidCreature);
    m_pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
    m_pCreature = new CSWCCreature();
    m_oid = oidCreature;
    CopyCreatureStats(pOriginal, m_pCreature);
    LevelUpCreature();
    DetermineClass();

#if KOTOR1
    StartLoadFromLayout(CResRef("MAINCG"));
#else
    StartLoadFromLayout(CResRef("MAINCG_p"), 1, 1, 1);
#endif

    InitControl(m_lblMainTitle, CExoString("MAIN_TITLE_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblName, CExoString("LBL_NAME"), 1, 1, 0, 0, 0);
#ifdef K2_QOL_IMPROVEMENTS
    K2QolMarkFitText(&m_lblName.m_text);   // the character's name (the tag is shared with the HUD)
#endif
    InitControl(m_lblClass, CExoString("LBL_CLASS"), 1, 1, 0, 0, 0);
#if KOTOR1
    InitControl(m_lblLevel, CExoString("LBL_LEVEL"), 1);
#endif
    InitControl(m_lblLevelVal, CExoString("LBL_LEVEL_VAL"), 1, 1, 0, 0, 0);
    InitControl(m_lblStr, CExoString("STR_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblDex, CExoString("DEX_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblCon, CExoString("CON_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblInt, CExoString("INT_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblWis, CExoString("WIS_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblCha, CExoString("CHA_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblStrAb, CExoString("STR_AB_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblDexAb, CExoString("DEX_AB_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblConAb, CExoString("CON_AB_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblIntAb, CExoString("INT_AB_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblWisAb, CExoString("WIS_AB_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblChaAb, CExoString("CHA_AB_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblVit, CExoString("LBL_VIT"), 1, 1, 0, 0, 0);
    InitControl(m_lblDef, CExoString("LBL_DEF"), 1, 1, 0, 0, 0);
    InitControl(m_lblNewWill, CExoString("NEW_WILL_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblNewRefl, CExoString("NEW_REFL_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblNewFort, CExoString("NEW_FORT_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblBevelL, CExoString("LBL_BEVEL_L"), 1, 1, 0, 0, 0);
    InitControl(m_lblBevelM, CExoString("LBL_BEVEL_M"), 1, 1, 0, 0, 0);
    InitControl(m_lblBevelR, CExoString("LBL_BEVEL_R"), 1, 1, 0, 0, 0);
    InitControl(m_lblFortitude, CExoString("LBL_FORTITUDE"), 1, 1, 0, 0, 0);
    InitControl(m_lblReflex, CExoString("LBL_REFLEX"), 1, 1, 0, 0, 0);
    InitControl(m_lblWill, CExoString("LBL_WILL"), 1, 1, 0, 0, 0);
    InitControl(m_view3DChar, CExoString("MODEL_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblPortrait, CExoString("PORTRAIT_LBL"), 1, 1, 0, 0, 0);
#if KOTOR1
    StopLoadFromLayout();
    ScaleExtentForResolution();
#else
    InitControl(m_lblStatsBack, CExoString("LBL_STATSBACK"), 1, 1, 0, 0, 0);
    InitControl(m_lblStatsBorder, CExoString("LBL_STATSBORDER"), 1, 1, 0, 0, 0);
    InitControl(m_lblPortBorder, CExoString("LBL_PORTBORDER"), 1, 1, 0, 0, 0);
    InitControl(m_lblVitName, CExoString("LBL_VIT_NAME"), 1, 1, 0, 0, 0);
    InitControl(m_lblDefName, CExoString("LBL_DEF_NAME"), 1, 1, 0, 0, 0);
    InitControl(m_lblBar1, CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
#endif

    m_lblMainTitle.m_text.m_params.SetStrRef(STRREF_LEVEL_UP);

    CSWGuiLevelUpPanel* pLevelUpPanel = new CSWGuiLevelUpPanel(m_pGuiManager, m_oid);
    m_pLevelUpPanel = pLevelUpPanel;
    pLevelUpPanel->m_pCharGenPanel = this;

    SetNameString(m_pCreature->GetName());
    AbilitiesChanged();

    m_lblPortrait.m_border.m_params.SetFillImage(pOriginal->GetPortrait(1), 0);

    if (g_bDo3DGUIs) {
        CAurScene* pScene = m_view3DChar.m_scene.m_pScene;
        CAurCamera* pCamera = m_view3DChar.m_scene.m_pCamera;

        pScene->SpawnRoom(const_cast<char*>("gui3D_room"), Vector(0, 0, 0), Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
        m_view3DChar.m_scene.AddModel(CExoString("charrec_light"), -1);
        pCreature->Setup3DScene(m_view3DChar, pOriginal, 0, 1);
        pCreature->AttachToScene(pScene);
        pCreature->SetPosition(Vector(0.0f, 0.0f, 0.0f));

        CAurBehaviorAttachTranslate* pBehavior =
            static_cast<CAurBehaviorAttachTranslate*>(pCamera->GetBehavior(kBehaviorAttachTranslate));
        if (pBehavior) {
            pBehavior->m_bUnrotatedTranslation = 1;
            pBehavior->m_vTranslation = Vector(0.0f, 0.0f, pCreature->GetHeadHeightOffset());
        }
    }

    m_nInputClass = g_pAppManager->m_pClientExoApp->GetInputClass();
}

// Seeds the level-up copy from the real creature: the six base scores go through the setter wall,
// and the class, feat and skill lists are re-created. The result is discarded by the caller. The
// two alignment clamps appear once at the top and again after the class loop, and the armor class
// is written twice.
//
// The name pair is built through two locals, and their order matters. The joined name must outlive
// the whole function while the empty string is destroyed right after the assignment. The empty
// string is the stats block's last name, cleared because the copy's own name now carries the joined
// one.
bool CSWGuiLevelUpCharGen::CopyCreatureStats(CSWCCreature* pSrc, CSWCCreature* pDst)
{
    // Both blocks are viewed as CSWCLevelUpStats; the base-attack-bonus copy below is the straight
    // class-table sum rather than the creature stats' version.
    CSWCLevelUpStats* pSrcStats = static_cast<CSWCLevelUpStats*>(pSrc->m_pStats);
    CSWCLevelUpStats* pDstStats = static_cast<CSWCLevelUpStats*>(pDst->m_pStats);

#if KOTOR1
    // K1 copies both name strings as they are: no join, no clear.
    CExoString& sSrcLast = pSrcStats->m_sLastName;
    pDstStats->m_name = pSrcStats->m_name;
    pDstStats->m_sLastName = sSrcLast;
#else
    CExoString sFullName = pSrcStats->GetFirstName() + CExoString(" ") + pSrcStats->GetLastName();
    {
        CExoString sEmpty("");
        pDstStats->m_name = sFullName;
        pDstStats->m_sLastName = sEmpty;
    }
#endif

    pDstStats->m_portrait.m_baseResRef = pSrcStats->GetPortrait(0);
    pDstStats->m_nExperience = pSrcStats->m_nExperience;
    pDstStats->m_nStartingPackage = pSrcStats->m_nStartingPackage;
    pDstStats->m_nGender = pSrcStats->m_nGender;

    int nAlignment = pSrcStats->m_alignmentGoodEvil;
    if (nAlignment < 0)
        nAlignment = 0;
    if (nAlignment > 100)
        nAlignment = 100;
    pDstStats->m_alignmentGoodEvil = static_cast<short>(nAlignment);

#if !KOTOR1
    int nNPCAlignment = pSrcStats->m_baseCNPCAlignment;
    if (nNPCAlignment < 0)
        nNPCAlignment = -1;
    if (nNPCAlignment > 100)
        nNPCAlignment = 100;
    pDstStats->m_baseCNPCAlignment = static_cast<short>(nNPCAlignment);
#endif

    pDstStats->SetSTRBase(pSrcStats->m_abScoresBase[0]);
    pDstStats->SetDEXBase(pSrcStats->m_abScoresBase[1], 1);
    pDstStats->SetCONBase(pSrcStats->m_abScoresBase[2]);
    pDstStats->SetINTBase(pSrcStats->m_abScoresBase[3]);
    pDstStats->SetWISBase(pSrcStats->m_abScoresBase[4]);
    pDstStats->SetCHABase(pSrcStats->m_abScoresBase[5]);

    pDstStats->m_nHitPointsBase = pSrcStats->m_nHitPointsBase;
    pDstStats->m_nCurrentHitPoints = pSrcStats->m_nCurrentHitPoints;
    pDstStats->m_nMaxHitPoints = pSrcStats->GetMaxHitPoints();
    pDstStats->m_nForcePoints = pSrcStats->m_nForcePoints;
    pDstStats->m_nCurrentForcePoints = static_cast<unsigned short>(pSrcStats->m_nTemporaryForcePoints + pSrcStats->m_nCurrentForcePoints);
    pDstStats->m_nMaxForcePoints = pSrcStats->m_nMaxForcePoints;

    for (unsigned char i = 0; i < pSrcStats->m_nNumClasses; i++) {
        pDstStats->m_nNumClasses = static_cast<unsigned char>(i + 1);
        pDstStats->SetClass(i, pSrcStats->GetClass(i));
        pDstStats->SetClassLevel(i, pSrcStats->GetClassLevel(i));
        pDstStats->ClearKnownSpells(i);
        for (unsigned char j = 0; j < pSrcStats->GetNumberKnownSpells(i, 0); j++)
            pDstStats->AddKnownSpell(i, pSrcStats->GetKnownSpell(i, 0, j));
    }

    pDstStats->m_nRace = pSrcStats->m_nRace;
    pDstStats->m_raceString = pSrcStats->m_raceString;
    pDstStats->m_nSubraceIndex = pSrcStats->m_nSubraceIndex;

    pDstStats->SetFortSavingThrow();
    pDstStats->SetWillSavingThrow();
    pDstStats->SetReflexSavingThrow();

    for (unsigned char k = 0; k < 4; k++)
        pDstStats->m_nBaseAttackBonus = pSrcStats->GetBaseAttackBonus();

    // The two clamps run again here.
    int nAlignment2 = pSrcStats->m_alignmentGoodEvil;
    if (nAlignment2 < 0)
        nAlignment2 = 0;
    if (nAlignment2 > 100)
        nAlignment2 = 100;
    pDstStats->m_alignmentGoodEvil = static_cast<short>(nAlignment2);

#if !KOTOR1
    int nNPCAlignment2 = pSrcStats->m_baseCNPCAlignment;
    if (nNPCAlignment2 < 0)
        nNPCAlignment2 = -1;
    if (nNPCAlignment2 > 100)
        nNPCAlignment2 = 100;
    pDstStats->m_baseCNPCAlignment = static_cast<short>(nNPCAlignment2);
#endif

    pDstStats->ClearSkills();
    pDstStats->m_nSkillPointsRemaining = pSrcStats->m_nSkillPointsRemaining;

    for (uint64_t j = 0; j < g_pRules->m_nNumSkills; j++)
        pDstStats->SetSkillRankBase(static_cast<unsigned short>(j), pSrcStats->m_pSkillRanksBase[j]);

    pDstStats->ClearFeats();

    for (unsigned short i = 0; i < static_cast<unsigned short>(pSrcStats->m_lstFeats.m_count); i++) {
        unsigned short nFeat = pSrcStats->GetFeat(i);

        pDstStats->m_lstFeats.AddUnique(nFeat);
    }

    pDstStats->m_nAbilityPointsRemaining = pSrcStats->m_nAbilityPointsRemaining;
    pDstStats->m_nArmorClass = static_cast<unsigned short>(pSrcStats->GetArmorClass(0));
    pDstStats->m_nArmorClass = static_cast<unsigned short>(pSrcStats->GetArmorClass(0));

    return true;
}

// Bumps the levelled class's level by one on the copy. The stats local is typed CSWCLevelUpStats
// rather than the CSWCCreatureStats the creature declares, which is what makes the level setter
// resolve to this class's own hider.
void CSWGuiLevelUpCharGen::LevelUpCreature()
{
    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_oid);
    if (pCreature) {
        CSWCLevelUpStats* pStats = static_cast<CSWCLevelUpStats*>(pCreature->m_pStats);
        unsigned char nClass = static_cast<unsigned char>(pStats->m_nNumClasses - 1);
        pStats->SetClassLevel(nClass, static_cast<unsigned char>(pStats->GetClassLevel(nClass) + 1));
    }
}

// Rebuilds m_sClassName from the creature's class list, after dropping the class-list line.
#include "port/libcstring.h"
void CSWGuiLevelUpCharGen::DetermineClass()
{
    m_sClassName = m_pCreature->m_pStats->GetClassesString('\n');
#if KOTOR1
    int nLen = m_sClassName.GetLength();  // K1: the out-of-line accessor
#else
    uint64_t nLen = m_sClassName.m_sString ? strlen(m_sClassName.m_sString) : 0;
#endif
    m_sClassName = m_sClassName.Right(static_cast<int>(nLen - m_sClassName.Find('\n', 0) - 1));
    m_sClassName = m_sClassName.Left(m_sClassName.Find('(', 0));
}

// Caches the accepted name, builds the "<name> N" caption from the level-up GUI string at strref
// 74760 plus the new level, and pushes all three through the labels' own text parameters.
void CSWGuiLevelUpCharGen::SetNameString(const CExoString& sName)
{
    m_sName = sName;

#if KOTOR1
    // K1: the level label is the bare number, with no GUI-string prefix.
    CExoString sLevel(m_pCreature->m_pStats->GetLevel() + 1);

    m_lblName.m_text.m_params.SetText(m_sName);
    m_lblClass.m_text.m_params.SetText(m_sClassName);
    m_lblLevelVal.m_text.m_params.SetText(sLevel);
#else
    CExoString sTitle = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_LEVEL_COLON);
    sTitle = sTitle + CExoString(m_pCreature->m_pStats->GetLevel() + 1);

    m_lblName.m_text.m_params.SetText(m_sName);
    m_lblClass.m_text.m_params.SetText(m_sClassName);
    m_lblLevelVal.m_text.m_params.SetText(sTitle);
#endif
}

// Applies the pending level-up to the real creature and repaints every row it touches: the three
// saves are recomputed first, the six base scores are echoed into the ability labels, then the
// derived stats are rebuilt as the copy's value plus this level's gain (hit points, and the three
// force-point fields for Jedi classes).
//
// Each gain is new-versus-old and so re-reads the server stats block, whose modifier fields are
// signed chars. The charisma gain is computed the same way and then never used.
void CSWGuiLevelUpCharGen::AbilitiesChanged()
{
    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_oid);
    if (pCreature == 0)
        return;

    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->SetFortSavingThrow();
    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->SetWillSavingThrow();
    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->SetReflexSavingThrow();

    m_lblChaAb.m_text.m_params.SetText(CExoString(pCreature->m_pStats->m_abScoresBase[5]));
    m_lblStrAb.m_text.m_params.SetText(CExoString(pCreature->m_pStats->m_abScoresBase[0]));
    m_lblIntAb.m_text.m_params.SetText(CExoString(pCreature->m_pStats->m_abScoresBase[3]));
    m_lblWisAb.m_text.m_params.SetText(CExoString(pCreature->m_pStats->m_abScoresBase[4]));
    m_lblConAb.m_text.m_params.SetText(CExoString(pCreature->m_pStats->m_abScoresBase[2]));
    m_lblDexAb.m_text.m_params.SetText(CExoString(pCreature->m_pStats->m_abScoresBase[1]));

    // This level's hit die and force-point gain come from the last class slot.
    unsigned char nClass = pCreature->m_pStats->GetClass(
        static_cast<unsigned char>(pCreature->m_pStats->m_nNumClasses - 1));

    char nCONGain = static_cast<char>(pCreature->m_pStats->CalcStatModifier(pCreature->m_pStats->m_abScores[2])
                           - static_cast<unsigned char>(pCreature->GetServerCreature()->m_pStats->m_nCONMod));
    char nWISGain = static_cast<char>(pCreature->m_pStats->CalcStatModifier(pCreature->m_pStats->m_abScores[4])
                           - static_cast<unsigned char>(pCreature->GetServerCreature()->m_pStats->m_nWISMod));
    char nCHAGain = static_cast<char>(pCreature->m_pStats->CalcStatModifier(pCreature->m_pStats->m_abScores[5])
                           - pCreature->GetServerCreature()->m_pStats->m_nCHAMod);  // unused
    (void)nCHAGain;

    CSWClass* pClass = &g_pRules->m_paClasses[nClass];
    char nClassGain = static_cast<char>(pClass->m_nHitDie);

    char nHPGain = static_cast<char>(nClassGain
                          + pCreature->m_pStats->CalcStatModifier(pCreature->m_pStats->m_abScores[2])
                          + m_pCreature->m_pStats->GetLevel() * nCONGain);
    if (nHPGain < 2)
        nHPGain = 1;

    CSWCCreatureStats* pDstStats = pCreature->m_pStats;
    pDstStats->m_nHitPointsBase =
        static_cast<short>(static_cast<unsigned short>(m_pCreature->m_pStats->m_nHitPointsBase) + nClassGain);
    pDstStats->m_nMaxHitPoints =
        static_cast<unsigned short>(m_pCreature->m_pStats->GetMaxHitPoints() + static_cast<unsigned char>(nHPGain));
    pCreature->m_pStats->m_nCurrentHitPoints =
        static_cast<short>(static_cast<unsigned short>(m_pCreature->m_pStats->m_nCurrentHitPoints) + static_cast<unsigned char>(nHPGain));

    if (CSWClass::IsJedi(nClass) != 0) {
        // The class row is fetched again here rather than reusing the pointer above.
        char nForceGain = static_cast<char>(g_pRules->m_paClasses[nClass].m_nForcePointGain);
#if KOTOR1
        // K1: the force-point gain counts the charisma modifier and gain as well as wisdom.
        char nFPGain = static_cast<char>(nForceGain
                              + pCreature->m_pStats->CalcStatModifier(pCreature->m_pStats->m_abScores[4])
                              + pCreature->m_pStats->CalcStatModifier(pCreature->m_pStats->m_abScores[5])
                              + (nWISGain + nCHAGain) * m_pCreature->m_pStats->GetLevel());
#else
        char nFPGain = static_cast<char>(nForceGain
                              + pCreature->m_pStats->CalcStatModifier(pCreature->m_pStats->m_abScores[4])
                              + m_pCreature->m_pStats->GetLevel() * nWISGain);
#endif
        if (nFPGain < 2)
            nFPGain = 1;

        pCreature->m_pStats->m_nForcePoints =
            static_cast<unsigned short>(m_pCreature->m_pStats->m_nForcePoints + nForceGain);
        pCreature->m_pStats->m_nMaxForcePoints =
            static_cast<unsigned short>(m_pCreature->m_pStats->m_nMaxForcePoints + static_cast<unsigned char>(nFPGain));
        pCreature->m_pStats->m_nCurrentForcePoints =
            static_cast<unsigned short>(m_pCreature->m_pStats->m_nCurrentForcePoints + m_pCreature->m_pStats->m_nTemporaryForcePoints
                             + static_cast<unsigned char>(nFPGain));
    }

    m_lblDef.m_text.m_params.SetText(CExoString(static_cast<short>(pCreature->m_pStats->GetArmorClass(0))));
    m_lblNewFort.m_text.m_params.SetText(CExoString(pCreature->m_pStats->m_nFortSavingThrow));
    m_lblNewRefl.m_text.m_params.SetText(CExoString(pCreature->m_pStats->m_nReflexSavingThrow));
    m_lblVit.m_text.m_params.SetText(CExoString(static_cast<short>(pCreature->m_pStats->GetMaxHitPoints())));
    m_lblNewWill.m_text.m_params.SetText(CExoString(pCreature->m_pStats->m_nWillSavingThrow));
}

// The class's key function; defining it emits the class vtable.
CSWGuiLevelUpCharGen::~CSWGuiLevelUpCharGen()
{
    delete m_pCreature;
    m_pCreature = 0;
}

// Adds the child panel and marks whether it can still be cancelled.
void CSWGuiLevelUpCharGen::OnPanelAdded()
{
#if KOTOR1
    m_pLevelUpPanel->SetCanCancel(0);  // K1: never cancellable
#else
    m_pLevelUpPanel->SetCanCancel(m_nInputClass != 0);
#endif
    m_pGuiManager->AddPanel(static_cast<CSWGuiPanel*>(m_pLevelUpPanel), 1, 1);
    CSWGuiPanel::OnPanelAdded();
}

// The two cancel keys back out to the in-game screen; every other event falls through to the base,
// which is why the base call sits after the `if` rather than in an `else`.
void CSWGuiLevelUpCharGen::HandleInputEvent(int type, int val)
{
    if (val != 0 && (type == INPUTEVENT_GUI_BACK || type == INPUTEVENT_GUI_B))
    {
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        m_pGuiManager->PopModalPanel();

        CSWGuiPanel* pPanel = static_cast<CSWGuiPanel*>(m_pLevelUpPanel);
#if KOTOR1
        // K1: no panel chain; one state-4 stamp per panel.
        pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
        do
        {
            pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
            pPanel = pPanel->m_pNextInChain;
        } while (pPanel != 0);
#endif

        CSWGuiPanel* pChain = this;
#if KOTOR1
        pChain->m_stateFlags = (pChain->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
        do
        {
            pChain->m_stateFlags = (pChain->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
            pChain = pChain->m_pNextInChain;
        } while (pChain != 0);
#endif

        m_pInGameChar->AttachModel();
    }

    CSWGuiPanel::HandleInputEvent(type, val);
}

// The walks and the re-attach, without the sound or the input-class re-send.
void CSWGuiLevelUpCharGen::HandleCancelButton()
{
    m_pGuiManager->PopModalPanel();

    CSWGuiPanel* pPanel = static_cast<CSWGuiPanel*>(m_pLevelUpPanel);
#if KOTOR1
    // K1: no panel chain; one state-4 stamp per panel.
    pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
    do
    {
        pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
        pPanel = pPanel->m_pNextInChain;
    } while (pPanel != 0);
#endif

    CSWGuiPanel* pChain = this;
#if KOTOR1
    pChain->m_stateFlags = (pChain->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
    do
    {
        pChain->m_stateFlags = (pChain->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
        pChain = pChain->m_pNextInChain;
    } while (pChain != 0);
#endif

    m_pInGameChar->AttachModel();
}

// The same two walks as HandleCancelButton, with nothing to re-attach.
void CSWGuiLevelUpCharGen::RemoveSubPanels()
{
    m_pGuiManager->PopModalPanel();

    CSWGuiPanel* pPanel = static_cast<CSWGuiPanel*>(m_pLevelUpPanel);
#if KOTOR1
    // K1: no panel chain; one state-4 stamp per panel.
    pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
    do
    {
        pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
        pPanel = pPanel->m_pNextInChain;
    } while (pPanel != 0);
#endif

    CSWGuiPanel* pChain = this;
#if KOTOR1
    pChain->m_stateFlags = (pChain->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
    do
    {
        pChain->m_stateFlags = (pChain->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
        pChain = pChain->m_pNextInChain;
    } while (pChain != 0);
#endif
}

// Writes the seven new ability values into the real creature's stats block, in the given order
// rather than the stats block's declaration order. The leftover points go into the trailing byte.
void CSWGuiLevelUpCharGen::NewAbilityScores(int nStr, int nCha, int nInt, int nWis, int nCon,
                                            int nDex, int nExtra)
{
    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_oid);
    if (pCreature == 0)
        return;

    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->SetSTRBase(static_cast<unsigned char>(nStr));
    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->SetCHABase(static_cast<unsigned char>(nCha));
    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->SetINTBase(static_cast<unsigned char>(nInt));
    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->SetWISBase(static_cast<unsigned char>(nWis));
    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->SetCONBase(static_cast<unsigned char>(nCon));
    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->SetDEXBase(static_cast<unsigned char>(nDex), 1);
    pCreature->m_pStats->m_nAbilityPointsRemaining = static_cast<unsigned char>(nExtra);
}

// Wipes the real creature's ability scores. The stats pointer is re-loaded at every use, with the
// downcast spelled at each call site, so no load is hoisted out of the run. The score order is STR,
// CHA, INT, WIS, CON, DEX.
void CSWGuiLevelUpCharGen::ClearAbilities()
{
    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_oid);
    if (pCreature == 0)
        return;

    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->SetSTRBase(m_pCreature->m_pStats->m_abScoresBase[0]);
    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->SetCHABase(m_pCreature->m_pStats->m_abScoresBase[5]);
    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->SetINTBase(m_pCreature->m_pStats->m_abScoresBase[3]);
    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->SetWISBase(m_pCreature->m_pStats->m_abScoresBase[4]);
    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->SetCONBase(m_pCreature->m_pStats->m_abScoresBase[2]);
    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->SetDEXBase(m_pCreature->m_pStats->m_abScoresBase[1], 1);
    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->SetFortSavingThrow();
    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->SetWillSavingThrow();
    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->SetReflexSavingThrow();

    {
        CSWCLevelUpStats* pStats = static_cast<CSWCLevelUpStats*>(pCreature->m_pStats);
        pStats->m_nHitPointsBase = m_pCreature->m_pStats->m_nHitPointsBase;
        pStats->m_nMaxHitPoints = m_pCreature->m_pStats->GetMaxHitPoints();
    }
    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->m_nCurrentHitPoints = m_pCreature->m_pStats->m_nCurrentHitPoints;
    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->m_nForcePoints = m_pCreature->m_pStats->m_nForcePoints;
    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->m_nMaxForcePoints = m_pCreature->m_pStats->m_nMaxForcePoints;
    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->m_nCurrentForcePoints =
        m_pCreature->m_pStats->m_nCurrentForcePoints + m_pCreature->m_pStats->m_nTemporaryForcePoints;
}

// Hands the real creature's stats block the copy's skill-point pool and every raw purchased rank.
// The bound is re-read on every iteration, since the call could in principle rewrite the rules
// table, and the index is a wide counter.
void CSWGuiLevelUpCharGen::ClearSkills()
{
    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_oid);
    if (pCreature == 0)
        return;

    static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->m_nSkillPointsRemaining =
        m_pCreature->m_pStats->m_nSkillPointsRemaining;

    for (uint64_t i = 0; i < g_pRules->m_nNumSkills; i++)
        static_cast<CSWCLevelUpStats*>(pCreature->m_pStats)->SetSkillRankBase(
            static_cast<unsigned short>(i), m_pCreature->m_pStats->m_pSkillRanksBase[i]);
}

// Re-learns the copy's feats onto the real creature, deduplicated with the feat list's own
// AddUnique.
void CSWGuiLevelUpCharGen::ClearFeats()
{
    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_oid);
    if (pCreature == 0)
        return;

    pCreature->m_pStats->ClearFeats();

    for (unsigned short i = 0; i < static_cast<unsigned short>(m_pCreature->m_pStats->m_lstFeats.m_count); i++) {
        CSWCCreatureStats* pStats = pCreature->m_pStats;
        unsigned short nFeat = m_pCreature->m_pStats->GetFeat(i);

        pStats->m_lstFeats.AddUnique(nFeat);
    }
}

// Wipes the real creature's known spells, then re-learns the copy's. Both loop bounds come off the
// copy's stats block, not the real creature's; only the spell-list receiver is the real one, and
// the count is re-fetched every iteration.
void CSWGuiLevelUpCharGen::ClearPowers()
{
    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_oid);
    if (pCreature == 0)
        return;

#if KOTOR1
    // K1: only the last class's known powers are restored; K2 restores classes 0 and 1.
    unsigned char nClass = static_cast<unsigned char>(m_pCreature->m_pStats->m_nNumClasses - 1);
    pCreature->m_pStats->ClearKnownSpells();

    for (unsigned char i = 0;
         i < m_pCreature->m_pStats->GetNumberKnownSpells(nClass, 0); i++)
        pCreature->m_pStats->AddKnownSpell(nClass, m_pCreature->m_pStats->GetKnownSpell(nClass, 0, i));
#else
    pCreature->m_pStats->ClearKnownSpells();

    for (unsigned char i = 0;
         i < m_pCreature->m_pStats->GetNumberKnownSpells(0, 0); i++)
        pCreature->m_pStats->AddKnownSpell(0, m_pCreature->m_pStats->GetKnownSpell(0, 0, i));

    for (unsigned char i = 0;
         i < m_pCreature->m_pStats->GetNumberKnownSpells(1, 0); i++)
        pCreature->m_pStats->AddKnownSpell(1, m_pCreature->m_pStats->GetKnownSpell(1, 0, i));
#endif
}

// Commits the level-up and hands control back to the in-game screen, walking four chained panels.
// They do not all start at the same panel; the first starts at the level-up panel, the second at
// this screen, the third and fourth at the in-game char-gen panel.
void CSWGuiLevelUpCharGen::BeginPlayGame()
{
    m_pGuiManager->PopModalPanel();

    CSWGuiPanel* pPanel = static_cast<CSWGuiPanel*>(m_pLevelUpPanel);
#if KOTOR1
    // K1: no panel chain; one state-4 stamp per panel.
    pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
    do {
        pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
        pPanel = pPanel->m_pNextInChain;
    } while (pPanel != 0);
#endif

    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_oid);
    // The result is discarded.
    pCreature->m_pStats->GetClass(pCreature->m_pStats->m_nNumClasses - 1);
    g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServer_LevelUp(
        static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats), static_cast<CSWCLevelUpStats*>(pCreature->m_pStats));
    pCreature->m_nStateFlags |= 2;

    m_pGuiManager->PopModalPanel();

    CSWGuiPanel* pChain = this;
#if KOTOR1
    pChain->m_stateFlags = (pChain->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
    do {
        pChain->m_stateFlags = (pChain->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
        pChain = pChain->m_pNextInChain;
    } while (pChain != 0);
#endif

    m_pInGameChar->AttachModel();

    if (m_nInputClass == 0 && g_pAppManager->m_pClientExoApp->GetPausedByCombat() != 0)
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);

    if (g_bCreateAllGUIs == 0 && m_nInputClass != 2) {
        CSWGuiPanel* pScreen = m_pInGameChar;
#if KOTOR1
        pScreen->m_stateFlags = (pScreen->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
        do {
            pScreen->m_stateFlags = (pScreen->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
            pScreen = pScreen->m_pNextInChain;
        } while (pScreen != 0);
#endif

        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_apScreenPanels[2] = 0;
#if KOTOR1
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC);  // K1: 1-arg form
#else
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
#endif
    }

    g_pAppManager->m_pClientExoApp->SetInputClass(m_nInputClass, 1);
}

// BeginPlayGame's twin, run instead of the commit: the copy's stats walk back onto the live
// creature, and no level-up is sent to the server.
void CSWGuiLevelUpCharGen::CancelLevelUp()
{
    if (m_nInputClass == 0)
        return;

    CopyCreatureStats(m_pCreature,
                      g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_oid));

    m_pGuiManager->PopModalPanel();
    m_pGuiManager->PopModalPanel();

    CSWGuiPanel* pPanel = static_cast<CSWGuiPanel*>(m_pLevelUpPanel);
#if KOTOR1
    // K1: no panel chain; one state-4 stamp per panel.
    pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
    do
    {
        pPanel->m_stateFlags = (pPanel->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
        pPanel = pPanel->m_pNextInChain;
    } while (pPanel != 0);
#endif

    CSWGuiPanel* pChain = this;
#if KOTOR1
    pChain->m_stateFlags = (pChain->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
    do
    {
        pChain->m_stateFlags = (pChain->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
        pChain = pChain->m_pNextInChain;
    } while (pChain != 0);
#endif

    m_pInGameChar->AttachModel();

    if (g_bCreateAllGUIs == 0 && m_nInputClass != 2)
    {
        CSWGuiPanel* pScreen = m_pInGameChar;
#if KOTOR1
        pScreen->m_stateFlags = (pScreen->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
#else
        do
        {
            pScreen->m_stateFlags = (pScreen->m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
            pScreen = pScreen->m_pNextInChain;
        } while (pScreen != 0);
#endif

        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_apScreenPanels[2] = 0;
    }

    g_pAppManager->m_pClientExoApp->SetInputClass(m_nInputClass, 1);
}

void CSWGuiLevelUpCharGen::DisplayBackLabel(int a)
{
    K2_UNUSED(a);
}
