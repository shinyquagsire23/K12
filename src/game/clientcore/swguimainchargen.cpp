#include "swguimainchargen.h"
#include "swgui.h"
#include "swguiclassselection.h"
#include "nwccreature.h"
#include "nwccreaturestats.h"
#include "nwsrules.h"
#include "nwclass.h"
#include "swguiquickorcustompanel.h"
#include "swguiquickpanel.h"
#include "swguicustompanel.h"
#include "swguidrawable.h"
#include "aurscene.h"
#include "aurcamera.h"
#include "AURORA/aurbehavior.h"
#include "nwcanimbase.h"
#include "AURORA/aurmodel.h"
#include "Vector.h"
#include "Quaternion.h"
#include "exoresman.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "port/k2qolapi.h"

extern int g_bDo3DGUIs;

// Binds the 33 caption rows to the MAINCG_p layout, builds the three sub-pages the flow switches
// between, seeds the name and class rows and, with 3D GUIs on, stands up the character preview's
// scene, model and camera hook. The manager handed to the base constructor is immediately replaced
// by the one from g_pAppManager.
CSWGuiMainCharGen::CSWGuiMainCharGen(CSWGuiManager* pGuiManager, CSWCCreature* pCreature,
                                     CSWGuiClassSelection* pClassSelection)
    : CSWGuiPanel(pGuiManager)
{
    m_pClassSelection = pClassSelection;
    m_pGuiManager = g_pAppManager->m_pClientExoApp->GetSWGuiManager();
    m_pCreature = pCreature;
    DetermineClass();

#if KOTOR1
    StartLoadFromLayout(CResRef("MAINCG"));
#else
    StartLoadFromLayout(CResRef("MAINCG_p"), 1, 1, 1);
#endif

    // Layout control names, in bind order.
    InitControl(m_lblTitle,   CExoString("MAIN_TITLE_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblName,  CExoString("LBL_NAME"), 1, 1, 0, 0, 0);
#ifdef K2_QOL_IMPROVEMENTS
    K2QolMarkFitText(&m_lblName.m_text);   // the character's name row
#endif
    InitControl(m_lblClass,  CExoString("LBL_CLASS"), 1, 1, 0, 0, 0);
    InitControl(m_lblStr,  CExoString("STR_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblDex,  CExoString("DEX_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblCon,  CExoString("CON_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblInt,  CExoString("INT_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblWis,  CExoString("WIS_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblCha,  CExoString("CHA_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblStrAb, CExoString("STR_AB_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblDexAb, CExoString("DEX_AB_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblConAb, CExoString("CON_AB_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblIntAb, CExoString("INT_AB_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblWisAb, CExoString("WIS_AB_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblChaAb, CExoString("CHA_AB_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblNewWill, CExoString("NEW_WILL_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblNewRefl, CExoString("NEW_REFL_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblNewFort, CExoString("NEW_FORT_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblBevelL, CExoString("LBL_BEVEL_L"), 1, 1, 0, 0, 0);
    InitControl(m_lblBevelM, CExoString("LBL_BEVEL_M"), 1, 1, 0, 0, 0);
    InitControl(m_lblBevelR, CExoString("LBL_BEVEL_R"), 1, 1, 0, 0, 0);
    InitControl(m_lblFortitude, CExoString("LBL_FORTITUDE"), 1, 1, 0, 0, 0);
    InitControl(m_lblReflex, CExoString("LBL_REFLEX"), 1, 1, 0, 0, 0);
    InitControl(m_lblWill, CExoString("LBL_WILL"), 1, 1, 0, 0, 0);
    InitControl(m_lblVit, CExoString("LBL_VIT"), 1, 1, 0, 0, 0);
    InitControl(m_lblDef, CExoString("LBL_DEF"), 1, 1, 0, 0, 0);
    InitControl(m_sceneView,      CExoString("MODEL_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblPortrait, CExoString("PORTRAIT_LBL"), 1, 1, 0, 0, 0);
#if KOTOR1
    // K1: the stat captions and values are drawn at 70% alpha.
    m_lblDex.m_text.m_params.m_fAlpha = 0.7f;
    m_lblCon.m_text.m_params.m_fAlpha = 0.7f;
    m_lblInt.m_text.m_params.m_fAlpha = 0.7f;
    m_lblWis.m_text.m_params.m_fAlpha = 0.7f;
    m_lblCha.m_text.m_params.m_fAlpha = 0.7f;
    m_lblStrAb.m_text.m_params.m_fAlpha = 0.7f;
    m_lblDexAb.m_text.m_params.m_fAlpha = 0.7f;
    m_lblConAb.m_text.m_params.m_fAlpha = 0.7f;
    m_lblIntAb.m_text.m_params.m_fAlpha = 0.7f;
    m_lblWisAb.m_text.m_params.m_fAlpha = 0.7f;
    m_lblChaAb.m_text.m_params.m_fAlpha = 0.7f;
    m_lblNewWill.m_text.m_params.m_fAlpha = 0.7f;
    m_lblNewRefl.m_text.m_params.m_fAlpha = 0.7f;
    m_lblNewFort.m_text.m_params.m_fAlpha = 0.7f;
    m_lblFortitude.m_text.m_params.m_fAlpha = 0.7f;
    m_lblReflex.m_text.m_params.m_fAlpha = 0.7f;
    m_lblWill.m_text.m_params.m_fAlpha = 0.7f;
    m_lblVit.m_text.m_params.m_fAlpha = 0.7f;
    m_lblStr.m_text.m_params.m_fAlpha = 0.7f;
    m_lblDef.m_text.m_params.m_fAlpha = 0.7f;
    StopLoadFromLayout();
    ScaleExtentForResolution();
#else
    InitControl(m_lblStatsBack, CExoString("LBL_STATSBACK"), 1, 1, 0, 0, 0);
    InitControl(m_lblStatsBorder, CExoString("LBL_STATSBORDER"), 1, 1, 0, 0, 0);
    InitControl(m_lblPortBorder, CExoString("LBL_PORTBORDER"), 1, 1, 0, 0, 0);
    InitControl(m_lblVitName, CExoString("LBL_VIT_NAME"), 1, 1, 0, 0, 0);
    InitControl(m_lblDefName, CExoString("LBL_DEF_NAME"), 1, 1, 0, 0, 0);
    InitControl(m_lblBar, CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);

    StopLoadFromLayout();
#endif

    // The sub-pages the flow switches between. The concrete class has to be named again to reach
    // m_pParentPanel, which exists only there.
    m_pQuickOrCustomPanel = new CSWGuiQuickOrCustomPanel(m_pGuiManager, m_pCreature);
    static_cast<CSWGuiQuickOrCustomPanel*>(m_pQuickOrCustomPanel)->m_pParentPanel = this;

    m_pQuickPanel = new CSWGuiQuickPanel(m_pGuiManager, m_pCreature);
    static_cast<CSWGuiQuickPanel*>(m_pQuickPanel)->m_pParentPanel = this;

    // CSWGuiCustomPanel is modelled without its CSWGuiPanel base.
    m_pCustomPanel = static_cast<CSWGuiPanel*>(new CSWGuiCustomPanel(m_pGuiManager, m_pCreature));
    static_cast<CSWGuiCustomPanel*>(m_pCustomPanel)->SetParentPanel(this);

    m_nVisiblePanel = VISIBLE_PANEL_QUICK_OR_CUSTOM;

    // Seed the name and class rows. The name starts empty; the class row tracks m_sClasses, which
    // DetermineClass has already resolved.
    SetNameString(CExoString(""));

    // The portrait page's border image comes from the creature's own portrait.
    m_lblPortrait.m_border.m_params.SetFillImage(m_pCreature->GetPortrait(0), 0);

    if (g_bDo3DGUIs)
    {
        // Held in locals because the camera is used again at the end of the block.
        CAurScene* pScene = m_sceneView.m_scene.m_pScene;
        CAurCamera* pCamera = m_sceneView.m_scene.m_pCamera;
        // A 180-degree rotation about Z, not the identity quaternion.
        pScene->SpawnRoom(const_cast<char*>("gui3D_room"), Vector(0.0f, 0.0f, 0.0f), Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
        m_sceneView.m_scene.AddModel(CExoString("charrec_light"), -1);
        m_pCreature->Setup3DScene(m_sceneView, 0, 0, 0);

        m_pCreature->AttachToScene(pScene);
        m_pCreature->SetPosition(Vector(0.0f, 0.0f, 0.0f));

        // Put the camera's attach-translate hook at head height, unrotated, the same correction
        // CSWCCreature::Setup3DScene applies to its own camera.
        CAurBehaviorAttachTranslate* pBehavior =
            static_cast<CAurBehaviorAttachTranslate*>(pCamera->GetBehavior(kBehaviorAttachTranslate));
        if (pBehavior != 0)
        {
            pBehavior->m_bUnrotatedTranslation = 1;
            float fHeadHeight = m_pCreature->GetHeadHeightOffset();
            pBehavior->m_vTranslation.x = 0.0f;
            pBehavior->m_vTranslation.y = 0.0f;
            pBehavior->m_vTranslation.z = fHeadHeight;
        }
    }
}

CSWGuiMainCharGen::~CSWGuiMainCharGen()
{
}

void CSWGuiMainCharGen::ClearFeats()
{
    m_pCreature->m_pStats->ClearFeats();
}

void CSWGuiMainCharGen::BeginPlayGame()
{
    RemoveSubPanels();
    m_pClassSelection->BeginPlayGame();
}

// Adds the first page together with the panel, then runs the base hook.
void CSWGuiMainCharGen::OnPanelAdded()
{
    m_pGuiManager->AddPanel(m_pQuickOrCustomPanel, 1, 1);
    CSWGuiPanel::OnPanelAdded();
}

// The name goes to m_sName and to the name row; the class row tracks m_sClasses instead.
void CSWGuiMainCharGen::SetNameString(const CExoString& sName)
{
    m_sName = sName;
    m_lblName.m_text.m_params.SetText(m_sName);
    m_lblClass.m_text.m_params.SetText(m_sClasses);
}

// Blanks the caption rows, puts every base stat back to 8, recomputes the three saving throws and
// drops the auto-selected level-up package. The name and class rows are not blanked; SetNameString
// owns those. m_pCreature->m_pStats is re-read at each call rather than cached in a local.
void CSWGuiMainCharGen::ClearAbilities()
{
    m_lblChaAb.m_text.m_params.SetText(CExoString(""));
    m_lblStrAb.m_text.m_params.SetText(CExoString(""));
    m_lblIntAb.m_text.m_params.SetText(CExoString(""));
    m_lblWisAb.m_text.m_params.SetText(CExoString(""));
    m_lblConAb.m_text.m_params.SetText(CExoString(""));
    m_lblDexAb.m_text.m_params.SetText(CExoString(""));

    // Chargen's stats object is always a CSWCLevelUpStats.
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetSTRBase(8);
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetINTBase(8);
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetWISBase(8);
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetCONBase(8);
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetDEXBase(8, 1);
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetCHABase(8);
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetFortSavingThrow();
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetWillSavingThrow();
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetReflexSavingThrow();

    m_lblDef.m_text.m_params.SetText(CExoString(""));
    m_lblNewFort.m_text.m_params.SetText(CExoString(""));
    m_lblNewRefl.m_text.m_params.SetText(CExoString(""));
    m_lblVit.m_text.m_params.SetText(CExoString(""));
    m_lblNewWill.m_text.m_params.SetText(CExoString(""));

    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->ClearPackage();
}

// Cuts the class list at the sheet's '(' annotation, so the caption reads e.g. "Scout" rather than
// "Scout (1)".
void CSWGuiMainCharGen::DetermineClass()
{
    m_sClasses = m_pCreature->m_pStats->GetClassesString(' ');
    m_sClasses = m_sClasses.Left(m_sClasses.Find('(', 0));
}

// Repaints the derived-stat rows after the player spends a point: the three saving throws, the six
// base scores, the armor class (10 + DEX modifier + every class's armour bonus) and the hit-point
// total. pClass is assigned inside the loop but read after it, so that read is undefined when the
// creature has no class; chargen always has one.
void CSWGuiMainCharGen::AbilitiesChanged()
{
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetFortSavingThrow();
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetWillSavingThrow();
    static_cast<CSWCLevelUpStats*>(m_pCreature->m_pStats)->SetReflexSavingThrow();

    m_lblChaAb.m_text.m_params.SetText(CExoString(m_pCreature->m_pStats->m_abScoresBase[5]));
    m_lblStrAb.m_text.m_params.SetText(CExoString(m_pCreature->m_pStats->m_abScoresBase[0]));
    m_lblIntAb.m_text.m_params.SetText(CExoString(m_pCreature->m_pStats->m_abScoresBase[3]));
    m_lblWisAb.m_text.m_params.SetText(CExoString(m_pCreature->m_pStats->m_abScoresBase[4]));
    m_lblConAb.m_text.m_params.SetText(CExoString(m_pCreature->m_pStats->m_abScoresBase[2]));
    m_lblDexAb.m_text.m_params.SetText(CExoString(m_pCreature->m_pStats->m_abScoresBase[1]));

    int nArmorClass = 10 + m_pCreature->m_pStats->GetDEXMod();

    CSWClass* pClass;
#ifdef K2_FIX_ENGINE_BUGS
    pClass = 0;
#endif
    for (unsigned int i = 0; i < m_pCreature->m_pStats->m_nNumClasses; i++)
    {
        pClass = &g_pRules->m_paClasses[m_pCreature->m_pStats->GetClass(static_cast<unsigned char>(i))];
        nArmorClass += static_cast<signed char>(pClass->GetArmorBonus(m_pCreature->m_pStats->GetClassLevel(static_cast<unsigned char>(i))));
    }
    m_pCreature->m_pStats->m_nArmorClass = static_cast<unsigned short>(nArmorClass);

#ifdef K2_FIX_ENGINE_BUGS
    int nHitPoints = m_pCreature->m_pStats->m_nHitPointsBase + (pClass != 0 ? pClass->m_nHitDie : 0)
                     + m_pCreature->m_pStats->GetCONMod();
#else
    int nHitPoints = m_pCreature->m_pStats->m_nHitPointsBase + pClass->m_nHitDie
                     + m_pCreature->m_pStats->GetCONMod();
#endif

    m_lblDef.m_text.m_params.SetText(CExoString(static_cast<short>(m_pCreature->m_pStats->GetArmorClass(0))));
    m_lblNewFort.m_text.m_params.SetText(CExoString(m_pCreature->m_pStats->m_nFortSavingThrow));
    m_lblNewRefl.m_text.m_params.SetText(CExoString(m_pCreature->m_pStats->m_nReflexSavingThrow));
    m_lblVit.m_text.m_params.SetText(CExoString(nHitPoints));
    m_lblNewWill.m_text.m_params.SetText(CExoString(m_pCreature->m_pStats->m_nWillSavingThrow));
}

// Tears the three pages down: a page the manager still shows is marked for deletion, one it has
// already dropped is deleted and its slot cleared, in the order custom, quick-or-custom, quick.
void CSWGuiMainCharGen::RemoveSubPanels()
{
    m_pGuiManager->PopModalPanel();

    if (m_pGuiManager->PanelExists(m_pCustomPanel))
    {
        m_pCustomPanel->SetToBeDeleted(0);
    }
    else
    {
        delete m_pCustomPanel;
        m_pCustomPanel = 0;
    }

    if (m_pGuiManager->PanelExists(m_pQuickOrCustomPanel))
    {
        m_pQuickOrCustomPanel->SetToBeDeleted(0);
    }
    else
    {
        delete m_pQuickOrCustomPanel;
        m_pQuickOrCustomPanel = 0;
    }

    if (m_pGuiManager->PanelExists(m_pQuickPanel))
    {
        m_pQuickPanel->SetToBeDeleted(0);
    }
    else
    {
        delete m_pQuickPanel;
        m_pQuickPanel = 0;
    }

    m_pGuiManager->PopModalPanel();
    SetToBeDeleted(0);
}

// An unknown page lands on VISIBLE_PANEL_NONE, the "nothing showing" sentinel.
void CSWGuiMainCharGen::SetVisiblePanel(int nPanel)
{
    if (m_nVisiblePanel == nPanel)
        return;

    m_pGuiManager->PopModalPanel();

    switch (m_nVisiblePanel)
    {
    case VISIBLE_PANEL_QUICK_OR_CUSTOM: m_pGuiManager->RemovePanel(m_pQuickOrCustomPanel); break;
    case VISIBLE_PANEL_QUICK: m_pGuiManager->RemovePanel(m_pQuickPanel); break;
    case VISIBLE_PANEL_CUSTOM: m_pGuiManager->RemovePanel(m_pCustomPanel); break;
    default: m_nVisiblePanel = VISIBLE_PANEL_NONE; return;
    }

    if (m_nVisiblePanel == VISIBLE_PANEL_NONE)
        return;

    switch (nPanel)
    {
    case VISIBLE_PANEL_QUICK_OR_CUSTOM: m_pGuiManager->AddPanel(m_pQuickOrCustomPanel, 1, 1); break;
    case VISIBLE_PANEL_QUICK: m_pGuiManager->AddPanel(m_pQuickPanel, 1, 1); break;
    case VISIBLE_PANEL_CUSTOM: m_pGuiManager->AddPanel(m_pCustomPanel, 1, 1); break;
    default: m_nVisiblePanel = VISIBLE_PANEL_NONE; return;
    }

    if (m_nVisiblePanel == VISIBLE_PANEL_NONE)
        return;

    m_nVisiblePanel = nPanel;
}

// Ticks the character preview each frame. When 3D GUIs are on and the layout's "3D" state bit is
// set, it re-attaches the creature's model to this screen's scene if it is attached elsewhere, then
// cycle-pauses the creature so the preview keeps animating under the panel's own clock.
void CSWGuiMainCharGen::Draw(float f)
{
    if (m_pCreature != 0 && g_bDo3DGUIs && (m_stateFlags & PANEL_STATE_DRAW))
    {
        Gob* pModel = m_pCreature->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY);
        if (pModel->GetScene() != m_sceneView.m_scene.m_pScene)
        {
            m_pCreature->AttachToScene(m_sceneView.m_scene.m_pScene);

            CAurBehaviorAttachTranslate* pBehavior =
                static_cast<CAurBehaviorAttachTranslate*>(m_sceneView.m_scene.m_pCamera->GetBehavior(kBehaviorAttachTranslate));
            if (pBehavior != 0)
            {
                pBehavior->m_bUnrotatedTranslation = 1;
                float fHeadHeight = m_pCreature->GetHeadHeightOffset();
                pBehavior->m_vTranslation.x = 0.0f;
                pBehavior->m_vTranslation.y = 0.0f;
                pBehavior->m_vTranslation.z = fHeadHeight;
            }
        }
        m_pCreature->CGPauseCycle(f, 0);
    }

    CSWGuiPanel::Draw(f);
}

// Re-images the portrait row after the player picks a new portrait, re-dresses the creature from a
// copy of its appearance block with the appearance id forced to the new portrait's, and re-stands
// the preview scene.
void CSWGuiMainCharGen::PortraitChanged()
{
    m_lblPortrait.m_border.m_params.SetFillImage(m_pCreature->GetPortrait(0), 0);
    m_lblPortrait.m_text.m_params.SetText(CExoString(""));

    CAurCamera* pCamera = m_sceneView.m_scene.m_pCamera;

    // Copy the creature's appearance block into a local and override the appearance id, leaving the
    // creature's own block alone. The cast names the shared leading block, which is modelled field
    // by field and so has no member to reference here.
    CCAppearanceInfo appearanceInfo;
    appearanceInfo.core = *reinterpret_cast<CCAppearanceCore*>(m_pCreature->m_pCreatureAppearance);
    appearanceInfo.core.m_nAppearanceId = m_pCreature->m_pStats->m_nAppearanceType;

    if (g_bDo3DGUIs)
    {
        m_pCreature->SetAppearance(&appearanceInfo, 3, 1, 0);
        m_pCreature->AttachToScene(m_sceneView.m_scene.m_pScene);
        m_pCreature->SetPosition(Vector(0.0f, 0.0f, 0.0f));

        CAurBehaviorAttachTranslate* pBehavior =
            static_cast<CAurBehaviorAttachTranslate*>(pCamera->GetBehavior(kBehaviorAttachTranslate));
        if (pBehavior != 0)
        {
            pBehavior->m_bUnrotatedTranslation = 1;
            float fHeadHeight = m_pCreature->GetHeadHeightOffset();
            pBehavior->m_vTranslation.x = 0.0f;
            pBehavior->m_vTranslation.y = 0.0f;
            pBehavior->m_vTranslation.z = fHeadHeight;
        }
    }

    // Park the camera 2.5 units in front of the hook and look back at it: the direction vector is
    // (2.5, 0, headHeight - 1) normalized, and the camera faces the other way (yaw + 180).
    float fHeadHeight = m_pCreature->GetHeadHeightOffset();
    Vector vPos(2.5f, 0.0f, fHeadHeight);
    pCamera->SetPosition(vPos);
    vPos.z = fHeadHeight - 1.0f;
    vPos.Normalize();
    pCamera->SetOrientation(YawPitchRoll(Yaw(vPos) + 180.0f, 90.0f - Pitch(vPos), 0.0f));
}
