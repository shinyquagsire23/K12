#include "k2stdint.h"
#include "swguiportraitchargen.h"
#include "nwscriptconst.h"

#define CEXO_ARRAYLIST_KEEP_ADDUNIQUE_OUT_OF_LINE
#include "CExoArrayListImpl.h"

#include "swgui.h"
#include "swguicustompanel.h"
#include "swguiquickpanel.h"
#include "guiingame.h"
#include "nwccreature.h"
#include "nwccreaturestats.h"
#include "exoresman.h"
#include "Quaternion.h"
#include "Vector.h"
#include "aurscene.h"
#include "aurcamera.h"
#include "AURORA/aurmodel.h"
#include "twodimarray.h"
#include "nwccreatureappearance.h"
#include "nwcanimbase.h"
#include "swguicontrol.h"
#include "swguidrawable.h"
#include "AURORA/aurbehavior.h"

extern int g_bDo3DGUIs;

#include "port/crtapi.h"
#include "exoinput.h"

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#if KOTOR1
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#endif

CSWGuiPortraitCharGen::CSWGuiPortraitCharGen(CSWGuiManager* pGuiManager,
                                                  CSWCCreature* pCreature)
    : CSWGuiPanel(pGuiManager)
{
    m_pCreature = pCreature;

#if KOTOR1
    StartLoadFromLayout(CResRef("PORTCUST"));
#else
    StartLoadFromLayout(CResRef("PORTCUST_p"), 1, 1, 1);
#endif

    InitControl(m_lblMainTitle,    CExoString("MAIN_TITLE_LBL"), 1, 1, 0, 0, 0);
    InitControl(m_lblSubTitle,     CExoString("SUB_TITLE_LBL"),  1, 1, 0, 0, 0);
    InitControl(m_lblPortrait,     CExoString("LBL_PORTRAIT"),   1, 1, 0, 0, 0);
    InitControl(m_view3DChar,      CExoString("LBL_HEAD"),       1, 1, 0, 0, 0);
    InitControl(m_lblBevel[0],      CExoString("LBL_BEVEL_L"),    1, 1, 0, 0, 0);
    InitControl(m_lblBevel[1],      CExoString("LBL_BEVEL_M"),    1, 1, 0, 0, 0);
    InitControl(m_lblBevel[2],      CExoString("LBL_BEVEL_R"),    1, 1, 0, 0, 0);
    InitControl(m_lblBevel[3],      CExoString("LBL_BEVEL_T"),    1, 1, 0, 0, 0);
    InitControl(m_lblBevel[4],      CExoString("LBL_BEVEL_B"),    1, 1, 0, 0, 0);
    InitControl(m_btnBack,         CExoString("BTN_BACK"),       1, 1, 0, 0, 0);
    InitControl(m_btnAccept,       CExoString("BTN_ACCEPT"),     1, 1, 0, 0, 0);
    InitControl(m_btnArrL,         CExoString("BTN_ARRL"),       1, 1, 0, 0, 0);
    InitControl(m_btnArrR,         CExoString("BTN_ARRR"),       1, 1, 0, 0, 0);
#if KOTOR1
    // K1 fades the bevel frames and swaps the accept and back button extents.
    for (int i = 0; i < 5; i++)
        m_lblBevel[i].m_border.m_params.m_fAlpha = 0.3f;
    m_lblPortrait.m_border.m_params.m_fAlpha = 0.7f;

    CSWGuiExtent extAccept = m_btnAccept.m_extent;
    CSWGuiExtent extBack = m_btnBack.m_extent;
    m_btnBack.SetExtent(extAccept);
    m_btnAccept.SetExtent(extBack);

    StopLoadFromLayout();
    ScaleExtentForResolution();
#else
    InitControl(m_aBarLabels[0],   CExoString("LBL_BAR1"),       1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[1],   CExoString("LBL_BAR2"),       1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[2],   CExoString("LBL_BAR3"),       1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[3],   CExoString("LBL_BAR4"),       1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[4],   CExoString("LBL_BAR5"),       1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[5],   CExoString("LBL_BAR6"),       1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[6],   CExoString("LBL_BAR7"),       1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[7],   CExoString("LBL_BAR8"),       1, 1, 0, 0, 0);

    StopLoadFromLayout();
#endif

    m_nCurrentPortraitId = m_pCreature->m_pStats->m_portraitId;
    m_nCurrentPortrait = 0;
    GetPortraitsList(m_pCreature->m_pStats->m_nGender);

    CAurCamera* pCamera = m_view3DChar.m_scene.m_pCamera;
    CAurScene* pScene = m_view3DChar.m_scene.m_pScene;
#if !KOTOR1  // K1 only sets the close-up view angle once the head model is attached
    if (pCamera != 0)
        pCamera->SetViewAngle(65.0f);
#endif

    if (g_bDo3DGUIs != 0) {
        CExoString sCameraHook;
        sCameraHook.Format(const_cast<char*>("camerahook%c"),
                           m_pCreature->m_pStats->m_nGender != GENDER_MALE ? 'f' : 'm');

        pScene->SpawnRoom(const_cast<char*>("gui3D_room"), Vector(0, 0, 0), Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
        CAurObject* pModel = m_view3DChar.m_scene.AddModel(CExoString("cghead_light"), -1);
        if (pModel != 0) {
            pModel->PlayAnimation(const_cast<char*>("default"), 1.0f, 0, 0.0f);
            pCamera->AttachToObject(pModel, sCameraHook.CStr(), 1);
            pCamera->SetViewAngle(22.726f);
        }
    }

    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnAccept.m_tail.bFocusOnHover = 0;
    m_btnAccept.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnAButtonPressed));
    m_btnArrR.m_tail.bFocusOnHover = 0;
    m_btnArrR.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPortraitCharGen::OnRightArrowPressed));
    m_btnArrL.m_tail.bFocusOnHover = 0;
    m_btnArrL.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPortraitCharGen::OnLeftArrowPressed));

    UpdatePortraitButton();
    m_fIdleAnimTimer = 0.0f;

#if KOTOR1
    IosAddButtonIcon(static_cast<CSWGuiButton*>(&m_btnBack), 'b');
    IosAddButtonIcon(static_cast<CSWGuiButton*>(&m_btnAccept), 'a');
#else
    AspyrAddButtonIcon(static_cast<CSWGuiButton*>(&m_btnBack), 'b');
    AspyrAddButtonIcon(static_cast<CSWGuiButton*>(&m_btnAccept), 'a');
#endif
}

// Refreshes the portrait preview for the current slot: the creature's portrait and appearance, the
// button image and the 3D models.
void CSWGuiPortraitCharGen::UpdatePortraitButton()
{
    int nSlot = m_nCurrentPortrait;
    if (m_pCreature->m_pStats->m_portraitId != m_paPortraitIds.m_pArray[nSlot]) {
        m_pCreature->SetPortraitId(static_cast<unsigned short>(m_paPortraitIds.m_pArray[nSlot]));

        CCAppearanceInfo appearance;
        appearance.core = *reinterpret_cast<CCAppearanceCore*>(m_pCreature->m_pCreatureAppearance);
        nSlot = m_nCurrentPortrait;
        appearance.core.m_nAppearanceId = static_cast<unsigned short>(m_paAppearanceIds.m_pArray[nSlot]);
        m_pCreature->SetAppearance(&appearance, 2, 1, 1);
        nSlot = m_nCurrentPortrait;
        m_pCreature->m_pCreatureAppearance->m_nAppearanceId =
            static_cast<unsigned short>(m_paAppearanceIds.m_pArray[nSlot]);

        if (g_bDo3DGUIs) {
            Gob* pModel = m_pCreature->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY);
            if (pModel != 0)
                pModel->PlayAnimation(const_cast<char*>("pause1"), 1.0f, 2, 0.0f);
            pModel = m_pCreature->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD);
            if (pModel != 0)
                pModel->PlayAnimation(const_cast<char*>("pause1"), 1.0f, 2, 0.0f);
        }
    }

    m_lblPortrait.m_border.m_params.SetFillImage(m_pCreature->GetPortrait(0), 0);

    if (g_bDo3DGUIs) {
        m_pCreature->AttachToScene(m_view3DChar.m_scene.m_pScene);
        CAurBehaviorAttachTranslate* pBehavior =
            static_cast<CAurBehaviorAttachTranslate*>(m_view3DChar.m_scene.m_pCamera->GetBehavior(kBehaviorAttachTranslate));
        if (pBehavior != 0) {
            pBehavior->m_bUnrotatedTranslation = 0;
            pBehavior->m_vTranslation = Vector(0.0f, m_pCreature->GetHeadHeightOffset(), 0.0f);
        }
    }
}

// Builds the per-slot portrait id and appearance tables from portraits.2da, keeping the rows whose
// Sex matches the creature and whose ForPC column is set.
void CSWGuiPortraitCharGen::GetPortraitsList(int nSex)
{
    C2DA* pTable = new C2DA(CResRef("PORTRAITS"), 0);
    pTable->Load2DArray();
    int nRows = pTable->m_nRows;

    CExoString sBaseResRef;
    CResRef cResRef;

    m_paAppearanceIds.m_count = 0;
    m_paPortraitIds.m_count = 0;

    for (int i = 0; i < nRows; i++) {
        int nRowSex = -1;
        int nAppearance = -1;
        int nForPC;
        unsigned char nClass;
        int nCount;

        pTable->GetINTEntry(i, "Sex", &nRowSex);
        if (nRowSex == nSex) {
            pTable->GetINTEntry(i, "ForPC", &nForPC);
            if (nForPC == 1) {
                nClass = m_pCreature->m_pStats->GetClass(0);
                switch (nClass) {
#if KOTOR1
                case CLASS_TYPE_SOLDIER: pTable->GetINTEntry(i, "Appearance_L", &nAppearance); break;
                case CLASS_TYPE_SCOUT: pTable->GetINTEntry(i, "AppearanceNumber", &nAppearance); break;
                case CLASS_TYPE_SCOUNDREL: pTable->GetINTEntry(i, "Appearance_S", &nAppearance); break;
#else
                case CLASS_TYPE_JEDIGUARDIAN: pTable->GetINTEntry(i, "Appearance_L", &nAppearance); break;
                case CLASS_TYPE_JEDICONSULAR: pTable->GetINTEntry(i, "Appearance_S", &nAppearance); break;
                case CLASS_TYPE_JEDISENTINEL: pTable->GetINTEntry(i, "AppearanceNumber", &nAppearance); break;
#endif
                default: pTable->GetINTEntry(i, "AppearanceNumber", &nAppearance); break;
                }

                pTable->GetCExoStringEntry(i, "BaseResRef", &sBaseResRef);

                nCount = m_paPortraitIds.m_count;
                if (i == m_pCreature->m_pStats->m_portraitId)
                    m_nCurrentPortrait = nCount;

                m_paPortraitIds.Add(i);
                m_paAppearanceIds.Add(nAppearance);
            }
        }
    }

    delete pTable;
}

// Per-frame tick: expires the arrow flashes and the idle-animation timer, then draws the panel.
void CSWGuiPortraitCharGen::Draw(float f) {
    for (int i = m_aFlashes.m_count - 1; i >= 0; i--) {
        if ((m_aFlashes.m_pArray[i].m_fTimer -= f) <= 0.0f) {
            m_aFlashes.m_pArray[i].m_pHilight->SetActive(0);
            m_aFlashes.DelIndex(i);
        }
    }

    if (m_pCreature != 0 && g_bDo3DGUIs != 0) {
        Gob* pModel = m_pCreature->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY);
        if (pModel->GetScene() != m_view3DChar.m_scene.m_pScene) {
            m_pCreature->AttachToScene(m_view3DChar.m_scene.m_pScene);
            CAurBehaviorAttachTranslate* pBehavior =
                static_cast<CAurBehaviorAttachTranslate*>(m_view3DChar.m_scene.m_pCamera->GetBehavior(kBehaviorAttachTranslate));
            if (pBehavior != 0) {
                pBehavior->m_bUnrotatedTranslation = 0;
                pBehavior->m_vTranslation = Vector(0.0f, m_pCreature->GetHeadHeightOffset(), 0.0f);
            }
        }

        m_fIdleAnimTimer -= f;
        if (m_fIdleAnimTimer <= 0.0f) {
            CExoString sIdle;
            CExoString sPause("pause1");

            pModel = m_pCreature->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY);
            switch (rand_wincompatible() % 2) {
            case 0: sIdle = "pause2"; break;
            case 1: sIdle = "listen"; break;
            }

            if (pModel != 0) {
                pModel->AddAnimationFlag(1);
                pModel->PlayAnimation(static_cast<char*>(sIdle.CStr()), 1.0f, AnimRun::kAnimOnce | AnimRun::kAnimQueue, 0.0f);
                pModel->PlayAnimation(static_cast<char*>(sPause.CStr()), 1.0f, AnimRun::kAnimQueue | AnimRun::kAnimNoReuse, 0.0f);
            }

            pModel = m_pCreature->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD);
            if (pModel != 0) {
                pModel->AddAnimationFlag(1);
                pModel->PlayAnimation(static_cast<char*>(sIdle.CStr()), 1.0f, AnimRun::kAnimOnce | AnimRun::kAnimQueue, 0.0f);
                pModel->PlayAnimation(static_cast<char*>(sPause.CStr()), 1.0f, AnimRun::kAnimQueue | AnimRun::kAnimNoReuse, 0.0f);
            }

            m_fIdleAnimTimer = static_cast<float>(rand_wincompatible() % 300) / 100.0f + 1.0f;
        }
    }

    CSWGuiPanel::Draw(f);
}

// Lights a highlight for a quarter of a second, re-arming it when it is already lit.
void CSWGuiPortraitCharGen::FlashArrow(CSWGuiLabelHilight& hilight) {
    int nCount = m_aFlashes.m_count;
    for (int i = 0; i < nCount; i++) {
        if (m_aFlashes[i].m_pHilight == &hilight) {
            m_aFlashes[i].m_fTimer = 0.25f;
            return;
        }
    }

    m_aFlashes.SetSize(nCount + 1);
    m_aFlashes[nCount].m_pHilight = &hilight;
    m_aFlashes[nCount].m_fTimer = 0.25f;

    hilight.SetActive(1);
}

void CSWGuiPortraitCharGen::SetParentPanel(CSWGuiPanel* pPanel, int flag) {
    m_pParentPanel = pPanel;
    m_nParentKind = static_cast<unsigned char>(flag);
}

// Both arrow buttons page through the portrait list by forwarding a key press.
void CSWGuiPortraitCharGen::OnRightArrowPressed(CSWGuiControl* pControl) {
    K2_UNUSED(pControl);
    HandleInputEvent(INPUTEVENT_GUI_DPAD_RIGHT, 1);
}

void CSWGuiPortraitCharGen::OnLeftArrowPressed(CSWGuiControl* pControl) {
    K2_UNUSED(pControl);
    HandleInputEvent(INPUTEVENT_GUI_DPAD_LEFT, 1);
}

// The arrow keys move the portrait selection and wrap at either end; select accepts, cancel backs
// out.
void CSWGuiPortraitCharGen::HandleInputEvent(int type, int val) {
    if (val != 0) {
        switch (type) {
        case INPUTEVENT_GUI_DPAD_LEFT: case INPUTEVENT_GUI_LEFT_SHOULDER: case INPUTEVENT_GUI_KEY_LEFT:
            m_pGuiManager->PlayGuiSound(1);
            if (--m_nCurrentPortrait < 0)
                m_nCurrentPortrait = m_paPortraitIds.m_count - 1;
            UpdatePortraitButton();
            break;

        case INPUTEVENT_GUI_DPAD_RIGHT: case INPUTEVENT_GUI_RIGHT_SHOULDER: case INPUTEVENT_GUI_KEY_RIGHT:
            m_pGuiManager->PlayGuiSound(1);
            if (++m_nCurrentPortrait >= m_paPortraitIds.m_count)
                m_nCurrentPortrait = 0;
            UpdatePortraitButton();
            break;

        case INPUTEVENT_GUI_A: case INPUTEVENT_GUI_START:
            m_pGuiManager->PlayGuiSound(0);
            OnSelectDisplayButton();
            break;

        case INPUTEVENT_GUI_B: case INPUTEVENT_GUI_BACK:
            m_pGuiManager->PlayGuiSound(0);
            HandleCancelButton();
            break;

        default:
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

// Starts the idle-animation timer at a random delay of one to four seconds.
void CSWGuiPortraitCharGen::OnPanelAdded() {
    m_fIdleAnimTimer = static_cast<float>(rand_wincompatible() % 300) / 100.0f + 1.0f;
}

// Selects the previous portrait, wrapping past the first slot.
void CSWGuiPortraitCharGen::HandleLeftButton() {
    m_nCurrentPortrait--;
    if (m_nCurrentPortrait < 0)
        m_nCurrentPortrait = m_paPortraitIds.m_count - 1;
    UpdatePortraitButton();
}

// Selects the next portrait, wrapping past the last slot.
void CSWGuiPortraitCharGen::HandleRightButton() {
    m_nCurrentPortrait++;
    if (m_nCurrentPortrait >= m_paPortraitIds.m_count)
        m_nCurrentPortrait = 0;
    UpdatePortraitButton();
}

// Applies the selected portrait and appearance to the creature, then closes the page.
void CSWGuiPortraitCharGen::OnSelectDisplayButton()
{
    m_pCreature->SetPortrait(CResRef(""));
    int nSlot = m_nCurrentPortrait;
    m_pCreature->SetPortraitId(static_cast<unsigned short>(m_paPortraitIds.m_pArray[nSlot]));

    CSWCCreatureStats* pStats = m_pCreature->m_pStats;
    nSlot = m_nCurrentPortrait;
    pStats->m_nAppearanceType = static_cast<unsigned short>(m_paAppearanceIds.m_pArray[nSlot]);

    m_pGuiManager->PopModalPanel();

    SetToBeDeleted(0);

    if (m_nParentKind == 1)
        static_cast<CSWGuiQuickPanel*>(m_pParentPanel)->PortraitCreationCompleted();
    else if (m_nParentKind == 2)
        static_cast<CSWGuiCustomPanel*>(m_pParentPanel)->PortraitCreationCompleted();
}

// Points the selection at the creature's current portrait, then closes the page.
void CSWGuiPortraitCharGen::HandleCancelButton() {
    if (m_pParentPanel == 0)
        return;

    m_nCurrentPortrait = m_paPortraitIds.IndexOf(m_nCurrentPortraitId);

    UpdatePortraitButton();
    m_pGuiManager->PopModalPanel();

    SetToBeDeleted(0);

    if (m_nParentKind == 1)
        static_cast<CSWGuiQuickPanel*>(m_pParentPanel)->CancelledByChild();
    else if (m_nParentKind == 2)
        static_cast<CSWGuiCustomPanel*>(m_pParentPanel)->CancelledByChild();
}
