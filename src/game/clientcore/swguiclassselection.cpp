
#include "k2stdint.h"
#include "port/sdlapi.h"
#include "swguiclassselection.h"
#include "swguiingameareatransition.h"
#include "swgui.h"
#include "swguicontrol.h"
#include "swguidrawable.h"
#include "guiingame.h"
#include "clientexoapp.h"
#include "appmanager.h"
#include "exoresman.h"
#include "exoinputinternal.h"
#include "swparty.h"
#include "nwccreature.h"
#include "swguimainchargen.h"
#include "nwcanimbase.h"
#include "AURORA/aurmodel.h"
#include "twodimarray.h"
#include "List.h"
#include "aurscene.h"
#include "aurcamera.h"
#include "nwclass.h"
#include "exotlktable.h"
#include "nwctrigger.h"
#include "nwcdoor.h"
#include "netlayer.h"
#include "serverexoapp.h"
#include "globalvartables.h"
#include "nwchandlemessage.h"
#include "clientoptions.h"
#include "nwccreaturestats.h"
#include "restype.h"
#include "serverexoappinternal.h"
#include "nwscriptconst.h"

extern CTlkTable* g_pTlkTable;

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);
float GetResolutionTextScalar();
extern int g_bDo3DGUIs;
bool ASLPlat_IsControllerPlugged();
#include "port/libcstring.h"

void UpdateScreen(float deltaTime, int a, int b);
#include "port/crtapi.h"
#include "port/k2qolapi.h"
#include "exoinput.h"
#include "strref.h"

#if KOTOR1
// The six class rows this screen offers, each a class id, gender, sound set and name strref. The
// male rows come first, and K1's table is the scoundrel/scout/soldier triple.
CSWGuiClassSelection::ClassEntry CSWGuiClassSelection::CHAR_TYPES[6] = {
    { 2, 0, 85, {0}, STRREF_SCOUNDREL_DESCRIPTION },
    { 1, 0, 85, {0}, STRREF_SCOUT_DESCRIPTION },
    { 0, 0, 85, {0}, STRREF_SOLDIER_DESCRIPTION },
    { 0, 1, 83, {0}, STRREF_SOLDIER_DESCRIPTION },
    { 1, 1, 83, {0}, STRREF_SCOUT_DESCRIPTION },
    { 2, 1, 83, {0}, STRREF_SCOUNDREL_DESCRIPTION },
};
#else
CSWGuiClassSelection::ClassEntry CSWGuiClassSelection::CHAR_TYPES[6] = {
    { 4, 0, 85, {0}, STRREF_JEDI_CONSULAR_DESCRIPTION },
    { 5, 0, 85, {0}, STRREF_JEDI_SENTINEL_DESCRIPTION },
    { 3, 0, 85, {0}, STRREF_JEDI_GUARDIAN_DESCRIPTION },
    { 3, 1, 83, {0}, STRREF_JEDI_GUARDIAN_DESCRIPTION },
    { 5, 1, 83, {0}, STRREF_JEDI_SENTINEL_DESCRIPTION },
    { 4, 1, 83, {0}, STRREF_JEDI_CONSULAR_DESCRIPTION },
};
#endif

// Fills the class tiles: one portrait-id list per gender from the portraits table, then one tile per
// class with its model scene, camera, portrait, appearance snapshot and label.
#if defined(K2_HOST_BUILD) && !KOTOR1 && !defined(K2_UI_PC) && !defined(K2_UI_TOUCH)
// Where the unselected tiles rest, in the layout's units.
static float s_fRestY = 121.0f;
#endif

CSWGuiClassSelection::CSWGuiClassSelection(CSWGuiManager* pGuiManager, const CExoString& sModule)
    : CSWGuiPanel(pGuiManager)
{
    CExoString sTemp;
    Vector vPos;
    Quaternion qRot(0.0f, 0.0f, 0.0f, 1.0f);

    g_pAppManager->m_pClientExoApp->SetLoadScreenByModuleName(CExoString("classsel"));
#if KOTOR1
    g_pAppManager->m_pClientExoApp->SetLoadScreenHint(0);
    g_pAppManager->m_pClientExoApp->ShowLoadScreen(1, 1, 0);
#else
    g_pAppManager->m_pClientExoApp->SetLoadMusic(
        g_pAppManager->m_pClientExoApp->GetLoadMusicByModuleName(CExoString("classsel")));
    g_pAppManager->m_pClientExoApp->SetLoadScreenHint(0);
    g_pAppManager->m_pClientExoApp->ShowLoadScreen(1, 1, 0, 1);
#endif
    UpdateScreen(0.033333335f, 0, 0);

    if (g_pExoResMan->Exists(CResRef("CHARGEN"), RESTYPE_RIM, static_cast<unsigned int*>(0)) != 0)
        g_pExoResMan->AddResourceImageFile(CExoString("RIMS:CHARGEN"));

    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(6, 1);
    UpdateScreen(0.033333335f, 0, 0);
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(12, 1);
    UpdateScreen(0.033333335f, 0, 0);

#if KOTOR1
    StartLoadFromLayout(CResRef("ClassSel"));
#else
    StartLoadFromLayout(CResRef("ClassSel_p"), 1, 1, 1);
#endif

    C2DA* pPortraits = new C2DA(CResRef("PORTRAITS"), 0);
    pPortraits->Load2DArray();
    int nRows = pPortraits->m_nRows;

    List<int> aPortraitsFemale;
    List<int> aPortraitsMale;
    int nSex;
    for (int i = 0; i < nRows; i++)
    {
        int nForPC;
        pPortraits->GetINTEntry(i, CExoString("ForPC"), &nForPC);
        if (nForPC != 0)
        {
            pPortraits->GetINTEntry(i, CExoString("Sex"), &nSex);
            if (nSex == GENDER_FEMALE)
                aPortraitsFemale.Add(i);
            else if (nSex == GENDER_MALE)
                aPortraitsMale.Add(i);
        }
    }

    int nProgress = 4;
    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(18, 1);
    UpdateScreen(0.033333335f, 0, 0);

    for (int i = 0; i < 6; i++)
    {
        ClassOption& option = m_aOptions[i];
        CAurScene* pScene;
        CAurCamera* pCamera;
        CAurObject* pModel;
        CSWCCreature* pCreature;
        CSWCCreatureStats* pStats;
        CCAppearanceInfo appearance;
        int nPortraitId K2_UNSET;
        int nIndex;
        int nAppearance;
        unsigned char nClass;

        sTemp.Format(const_cast<char*>("BTN_SEL%d"), i + 1);
        InitControl(option.m_button, sTemp, 1, 1, 0, 0, 0);

        sTemp.Format(const_cast<char*>("3D_MODEL%d"), i + 1);
        InitControl(option.m_sceneView, sTemp, 1, 1, 0, 0, 0);

#if KOTOR1
        option.m_button.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
        option.m_button.m_borderHilight.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
#endif
        option.m_button.m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);

        option.m_button.AddEvent(
            SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiClassSelection::OnEnterButton));
        option.m_button.AddEvent(
            INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiClassSelection::OnSelectButton));
        option.m_button.AddEvent(
            INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiClassSelection::OnSelectButton));

        option.m_nSlideState = 0;

        pScene = option.m_sceneView.m_scene.m_pScene;
        if (g_bDo3DGUIs)
        {
            pCamera = option.m_sceneView.m_scene.m_pCamera;
            pScene->SpawnRoom(const_cast<char*>("gui3D_room"), vPos, qRot);
            pModel = option.m_sceneView.m_scene.AddModel(CExoString("cgbody_light"), -1);
            if (pModel != 0)
            {
                pCamera->AttachToObject(pModel, const_cast<char*>("camerahook"), 0);
                pCamera->SetViewAngle(22.726f);
            }
        }

        g_pAppManager->m_pClientExoApp->SetLoadBarProgress(
            static_cast<int>(static_cast<float>(nProgress) * 0.0625f * 100.0f), 1);
        UpdateScreen(0.033333335f, 0, 0);

        pCreature = new CSWCCreature();
        pStats = pCreature->m_pStats;
        pStats->m_nGender = CHAR_TYPES[i].m_nGender;
        pStats->SetClass(0, CHAR_TYPES[i].m_nClass);
        pCreature->SetSoundSet(CHAR_TYPES[i].m_nSoundSet);

        appearance.core = *reinterpret_cast<CCAppearanceCore*>(pCreature->m_pCreatureAppearance);
        appearance.core.m_nGender = pStats->m_nGender;

        if (pStats->m_nGender == GENDER_FEMALE)
        {
            nIndex = rand_wincompatible() % aPortraitsFemale.m_count;
            nPortraitId = aPortraitsFemale[nIndex];
            aPortraitsFemale.DelIndex(nIndex);
        }
        else if (pStats->m_nGender == GENDER_MALE)
        {
            nIndex = rand_wincompatible() % aPortraitsMale.m_count;
            nPortraitId = aPortraitsMale[nIndex];
            aPortraitsMale.DelIndex(nIndex);
        }

        nClass = CHAR_TYPES[i].m_nClass;
#if KOTOR1
        if (nClass == CLASS_TYPE_SOLDIER)
            pPortraits->GetINTEntry(nPortraitId, CExoString("Appearance_L"), &nAppearance);
        else if (nClass == CLASS_TYPE_SCOUT)
            pPortraits->GetINTEntry(nPortraitId, CExoString("AppearanceNumber"), &nAppearance);
        else if (nClass == CLASS_TYPE_SCOUNDREL)
            pPortraits->GetINTEntry(nPortraitId, CExoString("Appearance_S"), &nAppearance);
        else
            pPortraits->GetINTEntry(nPortraitId, CExoString("AppearanceNumber"), &nAppearance);
#else
        if (nClass == CLASS_TYPE_JEDIGUARDIAN)
        {
            pPortraits->GetINTEntry(nPortraitId, CExoString("Appearance_L"), &nAppearance);
            appearance.core.m_bTypeNumber = 1;
        }
        else if (nClass == CLASS_TYPE_JEDISENTINEL)
        {
            pPortraits->GetINTEntry(nPortraitId, CExoString("AppearanceNumber"), &nAppearance);
            appearance.core.m_bTypeNumber = 2;
        }
        else if (nClass == CLASS_TYPE_JEDICONSULAR)
        {
            pPortraits->GetINTEntry(nPortraitId, CExoString("Appearance_S"), &nAppearance);
            appearance.core.m_bTypeNumber = 3;
        }
        else
        {
            pPortraits->GetINTEntry(nPortraitId, CExoString("AppearanceNumber"), &nAppearance);
        }
#endif

        pCreature->SetPortraitId(static_cast<unsigned short>(nPortraitId));
        pStats->m_nAppearanceType = static_cast<unsigned short>(nAppearance);
        appearance.core.m_nAppearanceId = static_cast<unsigned short>(nAppearance);
        appearance.core.m_bArmorEquipped = 1;
#if KOTOR1
        appearance.core.m_bTypeNumber = 1;
#endif
        appearance.core.m_bTypeVariant = 2;

        if (g_bDo3DGUIs)
        {
#if KOTOR1
            pCreature->SetAppearance(&appearance, 3, 1);
#else
            pCreature->SetAppearance(&appearance, 3, 1, 0);
#endif
            pCreature->AttachToScene(pScene);
            pCreature->SetPosition(vPos);
            pCreature->CGPauseCycle(0.0f, 1);
        }

        option.m_pCreature = pCreature;
        option.m_fSlideTimer = -1.0f;

        g_pAppManager->m_pClientExoApp->SetLoadBarProgress(
            static_cast<int>(static_cast<float>(nProgress | 1) * 0.0625f * 100.0f), 1);
        UpdateScreen(0.033333335f, 0, 0);

        nProgress += 2;
    }

    InitControl(m_lblInstruction, CExoString("LBL_INSTRUCTION"), 1, 1, 0, 0, 0);
    InitControl(m_lblCharGen, CExoString("LBL_CHAR_GEN"), 1, 1, 0, 0, 0);
    InitControl(m_lblDesc, CExoString("LBL_DESC"), 1, 1, 0, 0, 0);
    InitControl(m_lblClass, CExoString("LBL_CLASS"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();
#if KOTOR1
    ScaleExtentForResolution();
#endif
#if defined(K2_HOST_BUILD) && !KOTOR1 && !defined(K2_UI_PC) && !defined(K2_UI_TOUCH)
    // The console band comes from the Android override (tiles at y 121); a PC data set has no override
    // and loads gui.bif's classsel_p, whose tiles rest at y 167.
    if (m_pGuiManager->m_nScreenHeight > 0)
        s_fRestY = static_cast<float>(m_aOptions[1].m_button.m_extent.y) * static_cast<float>(static_cast<unsigned int>(g_nGUIHeight)) /
                   static_cast<float>(m_pGuiManager->m_nScreenHeight);
#endif
#ifdef K2_QOL_IMPROVEMENTS
    K2QolFitLabel(&m_lblInstruction.m_text);
    K2QolFitLabel(&m_lblCharGen.m_text);
    K2QolFitLabel(&m_lblDesc.m_text);
    K2QolFitLabel(&m_lblClass.m_text);
#endif

    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(
        INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));

#if !KOTOR1
    SetActiveControl(&m_aOptions[0].m_button, 0);
#endif
    m_sModuleName = sModule;

    g_pAppManager->m_pClientExoApp->SetLoadBarProgress(
        static_cast<int>(static_cast<float>(nProgress) * 0.0625f * 100.0f), 1);
    UpdateScreen(0.033333335f, 0, 0);

    delete pPortraits;
    AspyrAddButtonIcon(&m_btnBack, 'b');
}

#if KOTOR1

// Slides the tiles in and out around the selected one.
void CSWGuiClassSelection::Draw(float f)
{
    // K1: the slide band scales from the screen height over the 768-pixel design height.
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif

    CSWGuiExtent extentButton;
    CSWGuiExtent extentView;

    for (int i = 0; i < 6; i++)
    {
        ClassOption& option = m_aOptions[i];

        if (g_bDo3DGUIs && (m_stateFlags & PANEL_STATE_DRAW))
        {
            CAurScene* pScene = option.m_sceneView.m_scene.m_pScene;
            if (option.m_pCreature->GetAnimationBase()
                    ->GetModel(CSWCAnimBase::ANIM_MODEL_BODY)
                    ->GetScene() != pScene)
                m_pSelectedCreature->AttachToScene(pScene);
            option.m_pCreature->CGPauseCycle(f, 0);
        }

        extentButton = option.m_button.m_extent;
        extentView = option.m_sceneView.m_extent;

        int nDelta;
        int nY;
#ifdef K2_UI_PC
        if (option.m_button.m_tail.bActive)
#else
        if (option.m_nSlideState != 0)
#endif
        {
            float fW1 = s_fScale * 193.0f;
            nDelta = static_cast<int>(extentButton.y - fW1);
            if (nDelta == 0)
            {
                option.m_fSlideTimer = -1.0f;
                continue;
            }
            if (option.m_fSlideTimer == -1.0f)
            {
                option.m_fSlideTimer = nDelta / 16 / s_fScale * 0.25f;
                continue;
            }
            option.m_fSlideTimer = option.m_fSlideTimer - f;
            if (option.m_fSlideTimer <= 0.0f)
                nY = static_cast<int>(fW1);
            else
                nY = static_cast<int>(s_fScale * 209.0f
                           - static_cast<int>(s_fScale * ((0.25f - option.m_fSlideTimer) * 4.0f * 16.0f)));
            if (fW1 >= static_cast<float>(nY))
            {
                option.m_fSlideTimer = -1.0f;
                nY = static_cast<int>(fW1);
            }
            nDelta = extentButton.y - nY;
            if (nDelta == 0)
                continue;

            extentButton.y = nY;
            extentButton.x -= nDelta;
            extentButton.w += nDelta * 2;
            extentButton.h += nDelta * 2;
            option.m_button.SetExtent(extentButton);

            extentView.x -= nDelta;
            extentView.y -= nDelta;
            extentView.w += nDelta * 2;
            extentView.h += nDelta * 2;
            option.m_sceneView.SetExtent(extentView);
        }
        else
        {
            float fW2 = s_fScale * 209.0f;
            nDelta = static_cast<int>(fW2 - extentButton.y);
            if (nDelta == 0)
            {
                option.m_fSlideTimer = -1.0f;
                continue;
            }
            if (option.m_fSlideTimer == -1.0f)
            {
                option.m_fSlideTimer = nDelta / 16 / s_fScale * 0.25f;
                continue;
            }
            option.m_fSlideTimer = option.m_fSlideTimer - f;
            if (option.m_fSlideTimer <= 0.0f)
                nY = static_cast<int>(fW2);
            else
                nY = static_cast<int>(s_fScale * 193.0f
                           + static_cast<int>(s_fScale * ((0.25f - option.m_fSlideTimer) * 4.0f * 16.0f)));
            if (fW2 <= static_cast<float>(nY))
            {
                option.m_fSlideTimer = -1.0f;
                nY = static_cast<int>(fW2);
            }
            nDelta = nY - extentButton.y;
            if (nDelta == 0)
                continue;

            extentButton.y = nY;
            extentButton.x += nDelta;
            extentButton.w -= nDelta * 2;
            extentButton.h -= nDelta * 2;
            option.m_button.SetExtent(extentButton);

            extentView.x += nDelta;
            extentView.y += nDelta;
            extentView.w -= nDelta * 2;
            extentView.h -= nDelta * 2;
            option.m_sceneView.SetExtent(extentView);
        }
    }

    CSWGuiPanel::Draw(f);
}
#else
void CSWGuiClassSelection::Draw(float f)
{
    // The slide band is scaled to the screen, 1.0 when there is no GUI manager to ask.
    float fScale = 1.0f;
    if (g_pAppManager->m_pClientExoApp->GetSWGuiManager() != 0)
        fScale = static_cast<float>(g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenHeight) /
                 static_cast<float>(static_cast<unsigned int>(g_nGUIHeight));

    CSWGuiExtent extentButton;
    CSWGuiExtent extentView;

#ifdef K2_UI_PC
    float fW1 = fScale * 157.0f;
    float fW2 = fScale * 167.0f;
#elif defined(K2_HOST_BUILD) && !defined(K2_UI_TOUCH)
    float fW1 = fScale * (s_fRestY - 10.0f);
    float fW2 = fScale * s_fRestY;
#else
    float fW1 = fScale * 111.0f;
    float fW2 = fScale * 121.0f;
#endif
    int nW1 = static_cast<int>(fW1);
    int nW2 = static_cast<int>(fW2);
    float fSpan = fW2 - fW1;

    for (int i = 0; i < 6; i++)
    {
        ClassOption& option = m_aOptions[i];

        if (g_bDo3DGUIs && (m_stateFlags & PANEL_STATE_DRAW))
        {
            CAurScene* pScene = option.m_sceneView.m_scene.m_pScene;
            if (option.m_pCreature->GetAnimationBase()
                    ->GetModel(CSWCAnimBase::ANIM_MODEL_BODY)
                    ->GetScene() != pScene)
                m_pSelectedCreature->AttachToScene(pScene);
            option.m_pCreature->CGPauseCycle(f, 0);
        }

        extentButton = option.m_button.m_extent;
        extentView = option.m_sceneView.m_extent;

        int nDelta;
        int nY;
#ifdef K2_UI_PC
        if (option.m_button.m_tail.bActive)
#else
        if (option.m_nSlideState != 0)
#endif
        {
            nDelta = extentButton.y - nW1;
            if (nDelta == 0)
            {
                option.m_fSlideTimer = -1.0f;
                continue;
            }
            if (option.m_fSlideTimer == -1.0f)
            {
                option.m_fSlideTimer = static_cast<float>(nDelta) / fSpan * 0.25f;
                continue;
            }
            option.m_fSlideTimer = option.m_fSlideTimer - f;
            nY = nW1;
            if (!(option.m_fSlideTimer <= 0.0f))
                nY = nW2 - static_cast<int>(fSpan * ((0.25f - option.m_fSlideTimer) * 4.0f));
            if (fW1 >= static_cast<float>(nY))
            {
                option.m_fSlideTimer = -1.0f;
                nY = nW1;
            }
            nDelta = extentButton.y - nY;
            if (nDelta == 0)
                continue;

            extentButton.y = nY;
            extentButton.x -= nDelta;
            extentButton.w += nDelta * 2;
            extentButton.h += nDelta * 2;
            option.m_button.SetExtent(extentButton);

            extentView.x -= nDelta;
            extentView.y -= nDelta;
            extentView.w += nDelta * 2;
            extentView.h += nDelta * 2;
            option.m_sceneView.SetExtent(extentView);
        }
        else
        {
            nDelta = nW2 - extentButton.y;
            if (nDelta == 0)
            {
                option.m_fSlideTimer = -1.0f;
                continue;
            }
            if (option.m_fSlideTimer == -1.0f)
            {
                option.m_fSlideTimer = static_cast<float>(nDelta) / fSpan * 0.25f;
                continue;
            }
            option.m_fSlideTimer = option.m_fSlideTimer - f;
            nY = nW2;
            if (!(option.m_fSlideTimer <= 0.0f))
                nY = static_cast<int>(fSpan * ((0.25f - option.m_fSlideTimer) * 4.0f)) + nW1;
            if (fW2 <= static_cast<float>(nY))
            {
                option.m_fSlideTimer = -1.0f;
                nY = nW2;
            }
            nDelta = nY - extentButton.y;
            if (nDelta == 0)
                continue;

            extentButton.y = nY;
            extentButton.x += nDelta;
            extentButton.w -= nDelta * 2;
            extentButton.h -= nDelta * 2;
            option.m_button.SetExtent(extentButton);

            extentView.x += nDelta;
            extentView.y += nDelta;
            extentView.w -= nDelta * 2;
            extentView.h -= nDelta * 2;
            option.m_sceneView.SetExtent(extentView);
        }
    }

    CSWGuiPanel::Draw(f);
}
#endif

// Describes the class under the pointer: the sex and class caption and the class description.
void CSWGuiClassSelection::OnEnterButton(CSWGuiControl* pControl)
{
#ifdef K2_UI_PC
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    int nIndex = 0;
    while (nIndex <= 5 && &m_aOptions[nIndex].m_button != pControl)
        nIndex++;
    CExoString sLabel;
    if (CHAR_TYPES[nIndex].m_nGender == GENDER_MALE)
        sLabel = g_pTlkTable->GetSimpleString(STRREF_MALE);
    else
        sLabel = g_pTlkTable->GetSimpleString(STRREF_FEMALE);
#if KOTOR1
    if (CHAR_TYPES[nIndex].m_nClass == CSWClass::CCLASS_SCOUNDREL)
        sLabel = sLabel + CExoString(" ") + g_pTlkTable->GetSimpleString(STRREF_SCOUNDREL);
    else if (CHAR_TYPES[nIndex].m_nClass == CSWClass::CCLASS_SCOUT)
        sLabel = sLabel + CExoString(" ") + g_pTlkTable->GetSimpleString(STRREF_SCOUT);
    else
        sLabel = sLabel + CExoString(" ") + g_pTlkTable->GetSimpleString(STRREF_SOLDIER);
#else
    if (CHAR_TYPES[nIndex].m_nClass == CSWClass::CCLASS_JEDICONSULAR)
        sLabel = sLabel + CExoString(" ") + g_pTlkTable->GetSimpleString(STRREF_JEDI_CONSULAR);
    else if (CHAR_TYPES[nIndex].m_nClass == CSWClass::CCLASS_JEDISENTINEL)
        sLabel = sLabel + CExoString(" ") + g_pTlkTable->GetSimpleString(STRREF_JEDI_SENTINEL);
    else
        sLabel = sLabel + CExoString(" ") + g_pTlkTable->GetSimpleString(STRREF_JEDI_GUARDIAN);
#endif
    m_lblDesc.m_text.m_params.SetStrRef(CHAR_TYPES[nIndex].m_nStrRef);
    m_lblClass.m_text.m_params.SetText(sLabel);
#else
    if (ASLPlat_IsControllerPlugged())
        OnSelectButton(pControl);
#endif
}

// Clicking a tile goes straight on to character generation.
void CSWGuiClassSelection::OnSelectButton(CSWGuiControl* pControl)
{
#ifdef K2_UI_PC
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_START)
            m_pGuiManager->PlayGuiSound(pControl->m_tail.m_nClickSound);
        int nIndex = 0;
        while (nIndex <= 5 && &m_aOptions[nIndex].m_button != pControl)
            nIndex++;
        m_pSelectedCreature = m_aOptions[nIndex].m_pCreature;
        CSWGuiMainCharGen* pCharGen = new CSWGuiMainCharGen(m_pGuiManager, m_pSelectedCreature, this);
#if !KOTOR1
        pCharGen->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
        m_pGuiManager->AddPanel(pCharGen, 3, 1);
    }
#else
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_START)
            m_pGuiManager->PlayGuiSound(pControl->m_tail.m_nClickSound);

        int nIndex;
        if (&m_aOptions[0].m_button == pControl)
            nIndex = 0;
        else if (&m_aOptions[1].m_button == pControl)
            nIndex = 1;
        else if (&m_aOptions[2].m_button == pControl)
            nIndex = 2;
        else if (&m_aOptions[3].m_button == pControl)
            nIndex = 3;
        else if (&m_aOptions[4].m_button == pControl)
            nIndex = 4;
        else if (&m_aOptions[5].m_button == pControl)
            nIndex = 5;
        else
            nIndex = 6;

        m_pSelectedCreature = m_aOptions[nIndex].m_pCreature;

        if (m_aOptions[nIndex].m_nSlideState == 0)
        {
            for (int i = 0; i < 6; i++)
                m_aOptions[i].m_nSlideState = 0;

            CExoString sLabel;
            if (CHAR_TYPES[nIndex].m_nGender == GENDER_MALE)
                sLabel = g_pTlkTable->GetSimpleString(STRREF_MALE);
            else
                sLabel = g_pTlkTable->GetSimpleString(STRREF_FEMALE);

#if KOTOR1
            if (CHAR_TYPES[nIndex].m_nClass == CSWClass::CCLASS_SCOUNDREL)
                sLabel = sLabel + CExoString(" ") + g_pTlkTable->GetSimpleString(STRREF_SCOUNDREL);
            else if (CHAR_TYPES[nIndex].m_nClass == CSWClass::CCLASS_SCOUT)
                sLabel = sLabel + CExoString(" ") + g_pTlkTable->GetSimpleString(STRREF_SCOUT);
            else
                sLabel = sLabel + CExoString(" ") + g_pTlkTable->GetSimpleString(STRREF_SOLDIER);
#else
            if (CHAR_TYPES[nIndex].m_nClass == CSWClass::CCLASS_JEDICONSULAR)
                sLabel = sLabel + CExoString(" ") + g_pTlkTable->GetSimpleString(STRREF_JEDI_CONSULAR);
            else if (CHAR_TYPES[nIndex].m_nClass == CSWClass::CCLASS_JEDISENTINEL)
                sLabel = sLabel + CExoString(" ") + g_pTlkTable->GetSimpleString(STRREF_JEDI_SENTINEL);
            else
                sLabel = sLabel + CExoString(" ") + g_pTlkTable->GetSimpleString(STRREF_JEDI_GUARDIAN);
#endif

            m_lblDesc.m_text.m_params.SetStrRef(CHAR_TYPES[nIndex].m_nStrRef);
            m_lblClass.m_text.m_params.SetText(sLabel);
            m_aOptions[nIndex].m_nSlideState = 1;
        }
        else
        {
            m_aOptions[nIndex].m_nSlideState = 0;
            m_lblDesc.m_text.m_params.SetText(CExoString(""));
            m_lblClass.m_text.m_params.SetText(CExoString(""));

            CSWGuiMainCharGen* pCharGen =
                new CSWGuiMainCharGen(m_pGuiManager, m_pSelectedCreature, this);
#if !KOTOR1
            pCharGen->AddExtraBorderPanel(m_pGuiManager, CExoString("bg_replacement"));
#endif
            m_pGuiManager->AddPanel(pCharGen, 3, 1);
        }
    }
#endif
}

// Hands the selected creature and module over to the game proper, pushes the panel state and starts
// the module.
void CSWGuiClassSelection::BeginPlayGame()
{
#if KOTOR1
    g_pAppManager->m_pClientExoApp->BusyWaitMenuMusic();
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalUnpauseableState(1);

    CExoString sLoadScreen;

    CClientExoApp* pClientExoApp = g_pAppManager->m_pClientExoApp;
    CNetLayer* pNet;
    m_pSelectedCreature->m_pStats->m_nForcePoints = 0;

    g_pAppManager->CreateServer(0);

    m_pSelectedCreature->SaveCharGenCreature();

    pNet = pClientExoApp->GetNetLayer();
    {
        CExoString sClientName = pClientExoApp->GetClientName();
        pNet->StartConnectToSession(0, sClientName, CExoString(""), 10,
                                    CNetLayer::CONNECTTYPE_PLAYER);
    }

    pClientExoApp->GetSWCMessage()->SendPlayerToServerStatus_RequestStatus();

    sLoadScreen.Format(const_cast<char*>("%c%s.%s %s"), 's', "Module", "Load", m_sModuleName.CStr());

    pClientExoApp->GetNetLayer()->SendMessageToPlayer(
        CNetLayer::PLAYERID_SERVER, reinterpret_cast<unsigned char*>(sLoadScreen.CStr()), sLoadScreen.GetLength(), 0);

    pClientExoApp->SetPlayerCharacterName(CExoString("temp"));

    SetToBeDeleted(0);

    pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
    pClientExoApp->SetLoadScreenByModuleName(m_sModuleName);
    pClientExoApp->SetLoadMusic(
        pClientExoApp->GetLoadMusicByModuleName(m_sModuleName));
    pClientExoApp->ShowLoadScreen(1, 1, 0);
    pClientExoApp->SetLoadScreenHint(
        pClientExoApp->GetNextLoadScreenHintSTRREF(m_sModuleName, 0));
#else
    g_pAppManager->m_pClientExoApp->BusyWaitMenuMusic();
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalUnpauseableState(1);

    CExoString sLoadScreen;

    CClientExoApp* pClientExoApp = g_pAppManager->m_pClientExoApp;
    m_pSelectedCreature->m_pStats->m_nForcePoints = 0;

    g_pAppManager->CreateServer(0);

    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(CExoString("G_PC_LEVEL"), 1);
    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(CExoString("G_PC_Align_Val"), 50);
    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(
        CExoString("GBL_GAME_COMPLETE"),
        g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nGamesCompleted);

    m_pSelectedCreature->SaveCharGenCreature();

    pClientExoApp->GetNetLayer()->StartConnectToSession(0, pClientExoApp->GetClientName(), CExoString(""), 10,
                                                        CNetLayer::CONNECTTYPE_PLAYER);

    pClientExoApp->GetSWCMessage()->SendPlayerToServerStatus_RequestStatus();

    sLoadScreen.Format(const_cast<char*>("%c%s.%s %s"), 's', "Module", "Load", m_sModuleName.CStr());

    pClientExoApp->GetNetLayer()->SendMessageToPlayer(
        CNetLayer::PLAYERID_SERVER, reinterpret_cast<unsigned char*>(sLoadScreen.CStr()),
        static_cast<unsigned int>(sLoadScreen.m_sString ? strlen(sLoadScreen.m_sString) : 0), 0);

    pClientExoApp->SetPlayerCharacterName(CExoString("temp"));

    SetToBeDeleted(0);

    pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
    pClientExoApp->StopLoadMusic();
    pClientExoApp->SetLoadScreenByModuleName(m_sModuleName);
    pClientExoApp->SetLoadMusic(
        pClientExoApp->GetLoadMusicByModuleName(m_sModuleName));
    pClientExoApp->ShowLoadScreen(1, 1, 0, 1);
    pClientExoApp->SetLoadScreenHint(
        pClientExoApp->GetNextLoadScreenHintSTRREF(m_sModuleName, 0));
#endif
}

void CSWGuiClassSelection::HandleCancelButton()
{
    SetToBeDeleted(0);
#if KOTOR1
    g_pAppManager->m_pClientExoApp->DisplayMainMenu(0);
#else
    g_pAppManager->m_pClientExoApp->StopLoadMusic();
    g_pAppManager->m_pClientExoApp->SetStartMenuMusicOnMovieEnd(1);
    g_pAppManager->m_pClientExoApp->DisplayMainMenu(0);
    g_pAppManager->DestroyServer();
#endif
}

// The pad moves between tiles and selects one; the controller probe decides the starting tile.
void CSWGuiClassSelection::HandleInputEvent(int type, int val)
{
    if (val != 0)
    {
        switch (type)
        {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
        {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            HandleCancelButton();
            break;
        }
        case INPUTEVENT_GUI_LEFT_SHOULDER:
            HandleInputEvent(INPUTEVENT_GUI_DPAD_LEFT, val);
            break;
        case INPUTEVENT_GUI_RIGHT_SHOULDER:
            HandleInputEvent(INPUTEVENT_GUI_DPAD_RIGHT, val);
            break;
        }
    }

    CSWGuiPanel::HandleInputEvent(type, val);
}

void CSWGuiClassSelection::OnPanelAdded()
{
    g_pExoResMan->CleanDirectory(CExoString("GAMEINPROGRESS:"), 0, 0);
    g_pExoInput->CoolDownEvent(INPUTEVENT_GUI_B, 500);
    g_pExoInput->CoolDownEvent(INPUTEVENT_GUI_BACK, 500);
#if KOTOR1
    g_pAppManager->m_pClientExoApp->HideLoadScreen(1, 1);

    CSWGuiPanel::OnPanelAdded();

#ifdef K2_FIX_ENGINE_BUGS
    g_pAppManager->m_pClientExoApp->SetStartMenuMusicOnMovieEnd(1);
#endif
    g_pAppManager->m_pClientExoApp->StartMenuMusic(0, 127, 2);
#else
    g_pAppManager->m_pClientExoApp->HideLoadScreen(1, 0, 1);

    CSWGuiPanel::OnPanelAdded();
#endif

    m_lblDesc.m_text.m_params.SetText(CExoString(""));
    m_lblClass.m_text.m_params.SetText(CExoString(""));
    m_pGuiManager->HandleMouseMove(0, 0);

#if KOTOR1
    if (ASLPlat_IsControllerPlugged() || SDL_IsChromebook())
#else
    if (ASLPlat_IsControllerPlugged())
#endif
    {
#if !KOTOR1 && defined(K2_FIX_ENGINE_BUGS)
        for (int i = 0; i < 6; i++)
            m_aOptions[i].m_nSlideState = 0;
        SetActiveControl(0, 0);
#endif
        SetActiveControl(&m_aOptions[0].m_button, 0);
    }
    else
        SetActiveControl(&m_btnAccept, 0);
}

CSWGuiClassSelection::~CSWGuiClassSelection()
{
    for (int i = 0; i < 6; i++)
    {
        if (m_aOptions[i].m_pCreature != 0)
            delete m_aOptions[i].m_pCreature;
        m_aOptions[i].m_pCreature = 0;
    }

    if (g_pExoResMan->Exists(CResRef("CHARGEN"), RESTYPE_RIM, static_cast<unsigned int*>(0)) != 0)
        g_pExoResMan->bChargenImageFileLoaded = 1;
}
