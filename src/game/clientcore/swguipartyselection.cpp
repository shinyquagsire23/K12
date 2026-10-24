
#include "k2stdint.h"
#include "swguipartyselection.h"

#include "Achievements.h"
#include "appmanager.h"
#include "guiingame.h"
#include "nwarea.h"
#include "swguiingame.h"
#include "swguimaininterfacepc2.h"
#include "swparty.h"
#include "partytables.h"
#include "exoresman.h"
#include "exotlktable.h"
#include "clientexoapp.h"
#include "serverexoapp.h"
#include "pathfindinfo.h"
#include "nwsmodule.h"
#include "nwsarea.h"
#include "nwchandlemessage.h"
#include "nwscript.h"
#include "exosound.h"
#include "math3d.h"
#include "nwccreaturestats.h"
#include "nwscreature.h"
#include "nwscreaturestats.h"
#include "aurscene.h"
#include "aurcamera.h"
#include "AURORA/aurmodel.h"
#include "restype.h"
#include "tutorial.h"
#include "handmaidenappearance.h"

extern CTlkTable* g_pTlkTable;
extern int g_bDo3DGUIs;
extern int g_scaleMipc;
extern int g_nGUIWidth;
extern int g_nGUIHeight;
extern CExoSound* g_pExoSound;
extern int g_nRunScriptVar;
extern int quaternionidentityoptimization;
#if KOTOR1
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);
extern int g_bCreateAllGUIs;
#endif

#include "serverexoappinternal.h"
#include "exoinput.h"
#include "strref.h"

CSWGuiPartySelectionButton::CSWGuiPartySelectionButton() : m_bSelected(0)
{
}
#if KOTOR1  // K1: green border and highlight when selected, yellow highlight when not
void CSWGuiPartySelectionButton::SetSelected(int bSelected)
{
    m_bSelected = bSelected;
    if (bSelected)
    {
        m_border.m_params.m_fAlpha = 1.0f;
        m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_GREEN);
        m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_GREEN);
    }
    else
    {
        m_border.m_params.m_fAlpha = 0.0f;
        m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
    }
    m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);
}
#else
void CSWGuiPartySelectionButton::SetSelected(int bSelected)
{
    m_bSelected = bSelected;
    m_border.m_params.m_fAlpha = 1.0f;

    if (bSelected)
    {
        m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_TEXT);
        m_borderHilight.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_HIGHLIGHT);
    }
    else
    {
        m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
        m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_TEXT);
    }

    m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);
}
#endif

int CSWGuiPartySelectionButton::GetSelected()
{
    return m_bSelected;
}

#if KOTOR1  // K1: nine slots off the "partyselection" layout, no 3D preview or name backdrop
CSWGuiPartySelection::CSWGuiPartySelection(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    CExoString sTag;
    m_bHasForcedNPC = 0;
    StartLoadFromLayout(CResRef("partyselection"));

    for (int i = 0; i < 9; i++)
    {
        CSWGuiPartySelectionData* pData = &m_aPartyData[i];

        sTag.Format(const_cast<char*>("LBL_NA%d"), i);  // idiom-exempt: char* literal, Format's parameter type
        InitControl(pData->m_lblChar, sTag, 1);
        sTag.Format(const_cast<char*>("LBL_CHAR%d"), i);  // idiom-exempt: char* literal, Format's parameter type
        InitControl(pData->m_lblNA, sTag, 1);
        sTag.Format(const_cast<char*>("BTN_NPC%d"), i);  // idiom-exempt: char* literal, Format's parameter type
        InitControl(pData->m_button, sTag, 1);

        pData->m_button.AddEvent(INPUTEVENT_GUI_X, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPartySelection::OnToggled));
        pData->m_button.AddEvent(INPUTEVENT_GUI_START, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPartySelection::OnToggled));
        pData->m_button.AddEvent(SWGUIEVENT_FOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPartySelection::OnEnter));
        pData->m_button.AddEvent(SWGUIEVENT_UNFOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPartySelection::OnExit));

        pData->m_button.m_tail.m_nUserData = reinterpret_cast<unsigned long long>(pData);
        pData->m_button.m_borderHilight.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
        pData->m_button.m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);

        pData->m_bForced = 0;
        pData->m_nIndex = i;
    }

    InitControl(m_lblCount,    CExoString("LBL_COUNT"),     1);
    InitControl(m_lblNpcName,  CExoString("LBL_NPC_NAME"),  1);
    InitControl(m_lblNpcLevel, CExoString("LBL_NPC_LEVEL"), 1);
    InitControl(m_view3D,      CExoString("LBL_3D"),        1);
    InitControl(m_aLabels[LBL_TITLE],     CExoString("LBL_TITLE"),     1);
    InitControl(m_aLabels[LBL_AVAILABLE], CExoString("LBL_AVAILABLE"), 1);
    InitControl(m_aLabels[LBL_BEVEL_L],   CExoString("LBL_BEVEL_L"),   1);
    InitControl(m_aLabels[LBL_BEVEL_R],   CExoString("LBL_BEVEL_R"),   1);
    InitControl(m_aLabels[LBL_BEVEL_M],   CExoString("LBL_BEVEL_M"),   1);
    InitControl(m_btnDone,   CExoString("BTN_DONE"),   1);
    InitControl(m_btnBack,   CExoString("BTN_BACK"),   1);
    InitControl(m_btnAccept, CExoString("BTN_ACCEPT"), 1);
    StopLoadFromLayout();
    ScaleExtentForResolution();

    m_aLabels[LBL_BEVEL_R].m_border.m_params.m_fRotate = 180.0f;

    m_btnDone.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPartySelection::OnDone));
    m_btnDone.AddEvent(INPUTEVENT_GUI_START, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPartySelection::OnDone));
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnAccept.m_tail.bFocusOnHover = 0;
    m_btnAccept.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPartySelection::OnToggled));

    m_pActiveData = 0;
    m_btnDone.m_tail.bFocusOnHover = 0;
    m_bFromScript = 0;
    IosAddButtonIcon(&m_btnDone, 'a');
    IosAddButtonIcon(&m_btnBack, 'b');

    for (int i = 0; i < 9; i++)
        m_aPartyData[i].m_button.AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPartySelection::OnDone));
}
#else
CSWGuiPartySelection::CSWGuiPartySelection(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    CExoString sTag;  // reused for all three tags of every roster slot
    m_bHasForcedNPC = 0;
    StartLoadFromLayout(CResRef("partyselect_p"), 1, 1, 1);

    for (int i = 0; i < 12; i++)
    {
        CSWGuiPartySelectionData* pData = &m_aPartyData[i];

        sTag.Format(const_cast<char*>("LBL_NA%d"), i);  // idiom-exempt: char* literal, Format's parameter type
        InitControl(pData->m_lblChar, sTag, 1, 1, 0, 0, 0);
        sTag.Format(const_cast<char*>("LBL_CHAR%d"), i);  // idiom-exempt: char* literal, Format's parameter type
        InitControl(pData->m_lblNA, sTag, 1, 1, 0, 0, 0);
        sTag.Format(const_cast<char*>("BTN_NPC%d"), i);  // idiom-exempt: char* literal, Format's parameter type
        InitControl(pData->m_button, sTag, 1, 1, 0, 0, 0);

        pData->m_button.AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPartySelection::OnToggled));
        pData->m_button.AddEvent(SWGUIEVENT_FOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPartySelection::OnEnter));
        pData->m_button.AddEvent(SWGUIEVENT_UNFOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPartySelection::OnExit));

        pData->m_button.m_tail.m_nUserData = reinterpret_cast<unsigned long long>(pData);
        pData->m_button.m_borderHilight.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_HIGHLIGHT);
        pData->m_button.m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);

        pData->m_bForced = 0;
        pData->m_pCreature = 0;
        pData->m_nIndex = i;
        pData->m_bHidden = 0;
    }

    InitControl(m_lblCount,    CExoString("LBL_COUNT"),     1, 1, 0, 0, 0);
    InitControl(m_lblNpcName,  CExoString("LBL_NPC_NAME"),  1, 1, 0, 0, 0);
    InitControl(m_lblNpcLevel, CExoString("LBL_NPC_LEVEL"), 1, 1, 0, 0, 0);
    InitControl(m_view3D,      CExoString("LBL_3D"),        1, 1, 0, 0, 0);
    InitControl(m_aLabels[LBL_TITLE],     CExoString("LBL_TITLE"),     1, 1, 0, 0, 0);
    InitControl(m_aLabels[LBL_AVAILABLE], CExoString("LBL_AVAILABLE"), 1, 1, 0, 0, 0);
    InitControl(m_aLabels[LBL_BEVEL_L],   CExoString("LBL_BEVEL_L"),   1, 1, 0, 0, 0);
    InitControl(m_aLabels[LBL_BEVEL_M],   CExoString("LBL_BEVEL_M"),   1, 1, 0, 0, 0);

    // Both big buttons and the extents below scale with g_scaleMipc; the desktop panel is
    // centred at 4:3 and scales like its other controls.
    int nOldMipc = g_scaleMipc;
#if !defined(K2_HOST_BUILD) || defined(K2_UI_TOUCH)
    g_scaleMipc = 2;
#endif
    InitControl(m_btnDone, CExoString("BTN_DONE"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);

    float fXMult = 1.0f;
    float fYMult = 1.0f;
    int nXOffset = 0;
    int nYOffset = 0;
    if (g_pAppManager->m_pClientExoApp->GetSWGuiManager())
    {
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->GenerateAspectMultipliers(
            m_bCenterOnNativeRect, fXMult, fYMult, nXOffset, nYOffset,
            static_cast<float>(static_cast<unsigned int>(g_nGUIWidth)), static_cast<float>(static_cast<unsigned int>(g_nGUIHeight)));
    }

    // Both render-extent buttons take this height. The done button is re-placed after the back
    // button is resized, keeping its gap to the back button's right edge.
    int nHeight = static_cast<int>(fXMult * 120.0f);
    int nDoneGap = m_btnDone.m_extent.x - m_btnBack.m_extent.x - m_btnBack.m_extent.w;
    CSWGuiExtent extent = m_btnBack.m_extent;
    extent.w = nHeight;
    m_btnBack.SetExtent(extent);
    extent = m_btnDone.m_extent;
    extent.w = nHeight;
    extent.x = nDoneGap + m_btnBack.m_extent.x + m_btnBack.m_extent.w;
    m_btnDone.SetExtent(extent);
    g_scaleMipc = nOldMipc;

    InitControl(m_btnAccept,   CExoString("BTN_ACCEPT"),  1, 1, 0, 0, 0);
    InitControl(m_aBars[0],     CExoString("LBL_BAR1"),    1, 1, 0, 0, 0);
    InitControl(m_aBars[1],     CExoString("LBL_BAR2"),    1, 1, 0, 0, 0);
    InitControl(m_aBars[2],     CExoString("LBL_BAR3"),    1, 1, 0, 0, 0);
    InitControl(m_aBars[3],     CExoString("LBL_BAR4"),    1, 1, 0, 0, 0);
    InitControl(m_aBars[4],     CExoString("LBL_BAR5"),    1, 1, 0, 0, 0);
    InitControl(m_lblNameBack, CExoString("LBL_NAMEBACK"), 1, 1, 0, 0, 0);

    if (g_bDo3DGUIs)
    {
        CAurScene* pScene = m_view3D.m_scene.m_pScene;
        CAurCamera* pCamera = m_view3D.m_scene.m_pCamera;
        pScene->SpawnRoom(const_cast<char*>("gui3D_room"), Vector(0, 0, 0), Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
        CAurObject* pModel = m_view3D.m_scene.AddModel(CExoString("charrec_light"), -1);
        if (pModel)
        {
            pCamera->AttachToObject(pModel, const_cast<char*>("camerahook"), 1);
            pCamera->SetViewAngle(22.726f);
        }
        m_view3D.m_tail.bVisible = 0;
    }
    StopLoadFromLayout();

    // Rotation of the console counterpart label, which this build never loads.
    m_aLabels[LBL_UNK5980].m_border.m_params.m_fRotate = 180.0f;

    m_btnDone.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPartySelection::OnDone));
    AspyrAddButtonIcon(&m_btnDone, 'x');
    AspyrAddButtonIcon(&m_btnBack, 'b');

    m_btnBack.m_tail.bFocusOnHover = 0;
    // The base panel's own one-button handler rather than OnToggled; the pointer-to-member needs
    // the vtable adjustment.
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnAccept.m_tail.bFocusOnHover = 0;
    m_btnAccept.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPartySelection::OnToggled));

    m_pActiveData = 0;
    m_btnDone.m_tail.bFocusOnHover = 0;
    m_bFromScript = 0;
    m_fChallengeRating = 0;
    m_bCanChangeParty = 1;
    m_nCannotChangeStrRef = STRREF_CANNOT_CHANGE_PARTY_NOW;
}
#endif

CSWGuiPartySelectionData::CSWGuiPartySelectionData()
{
}

CSWGuiPartySelectionData::~CSWGuiPartySelectionData()
{
}


void CSWGuiPartySelection::OnToggled(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg)
        return;

    // In solo mode the query box answers every toggle instead of the roster; OnPanelAdded chose
    // its StrRef.
#if !KOTOR1  // K1: no solo-mode gate
    if (m_bCanChangeParty == 0)
    {
        CSWGuiMessageBox* pBox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetCallback(0, 0, 0);
        pBox->SetMessage(m_nCannotChangeStrRef);
        m_pGuiManager->AddPanel(pBox, 1, 1);
        return;
    }
#endif

    if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_START)
        m_pGuiManager->PlayGuiSound(pControl->m_tail.m_nClickSound);

    // The event's control is normally a roster button; the accept button, which the constructor
    // also routes here, is not, and stands in for the slot OnEnter last marked active.
    CSWGuiPartySelectionButton* pButton = static_cast<CSWGuiPartySelectionButton*>(pControl);
    CSWGuiPartySelectionData* pData;
    if (pControl == &m_btnAccept)
    {
        pData = m_pActiveData;
        if (!pData)
            return;
        pButton = &pData->m_button;
    }
    else
    {
        pData = reinterpret_cast<CSWGuiPartySelectionData*>(pControl->m_tail.m_nUserData);
    }

    if (!pData->m_bSelectable)
    {
        // For an empty slot, the confirmation box reports whether the slot is forced and whether
        // the button is latched.
        CSWGuiMessageBox* pBox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetCallback(0, 0, 0);
#if KOTOR1  // K1: no "already latched" message
        if (pData->m_bForced)
            pBox->SetMessage(STRREF_MUST_STAY_IN_PARTY);
        else
            pBox->SetMessage(STRREF_UNABLE_TO_JOIN_PARTY);
#else
        if (pData->m_bForced)
            pBox->SetMessage(STRREF_MUST_STAY_IN_PARTY);
        else if (pButton->m_bSelected)
            pBox->SetMessage(STRREF_UNABLE_TO_LEAVE_PARTY);
        else
            pBox->SetMessage(STRREF_UNABLE_TO_JOIN_PARTY);
#endif
        m_pGuiManager->AddPanel(pBox, 1, 1);
        return;
    }

    // For a filled slot, the selection count caps at the pair limit of two, so taking a slot is
    // gated and dropping one is not.
    if (pButton->m_bSelected)
    {
        m_nSelectedCount--;
        pButton->SetSelected(0);
    }
    else if (m_nSelectedCount <= 1)
    {
        m_nSelectedCount++;
        pButton->SetSelected(1);
    }

    SetAddRemoveLabel(pButton->m_bSelected == 0);
    UpdateCount();
}

// Fills in the slot record the rest of the panel reads: the preview creature's stats and
// appearance, and the 3D view. A slot with no live server NPC takes the 2D portrait path with
// placeholder text.
#if KOTOR1  // K1: no preview creature, LBL_3D is a label showing the NPC's portrait
void CSWGuiPartySelection::OnEnter(CSWGuiControl* pControl)
{
    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    CSWPartyTable* pTable = pServerApp->GetPartyTable();
    CSWGuiPartySelectionData* pData = reinterpret_cast<CSWGuiPartySelectionData*>(pControl->m_tail.m_nUserData);

    if (pTable->GetIsNPCAvailable(pData->m_nIndex))
    {
        CSWSCreature* pCreature = pServerApp->GetCreatureByGameObjectID(
            pTable->GetNPCObject(pData->m_nIndex, 0, 1));

        if (!pCreature)
        {
            m_lblNpcName.m_text.m_params.SetText(CExoString("I am broken"));
            m_lblNpcLevel.m_text.m_params.SetText(CExoString("So very very broken"));
            m_lblNpcName.m_tail.bVisible = 1;
            m_lblNpcLevel.m_tail.bVisible = 1;
            m_view3D.m_tail.bVisible = 0;
            m_aLabels[LBL_ACCEPT].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
            SetAddRemoveLabel(1);
        }
        else
        {
            CExoString sPortrait;
            pCreature->GetPortrait().CopyToString(sPortrait);
            sPortrait = sPortrait + CExoString("3");
            if (pCreature->m_pStats->m_nAlignment < 50)
            {
                CExoString sDark = sPortrait + CExoString("e");
                if (g_pExoResMan->Exists(CResRef(sDark), RESTYPE_TGA, static_cast<unsigned int*>(0)) ||
                    g_pExoResMan->Exists(CResRef(sDark), RESTYPE_TPC, static_cast<unsigned int*>(0)))
                    sPortrait = sDark;
            }
            m_view3D.m_border.m_params.SetFillImage(CResRef(sPortrait), 0);

            CExoString sLevel;
            sLevel.Format(const_cast<char*>("%s %d %s"),
                          g_pTlkTable->GetSimpleString(STRREF_LEVEL).CStr(),
                          pCreature->m_pStats->GetLevel(0),
                          pCreature->m_pStats->GetClassString(
                              pCreature->m_pStats->m_numClasses - 1).CStr());
            m_lblNpcName.m_text.m_params.SetText(pCreature->m_pStats->GetFullName());
            m_lblNpcLevel.m_text.m_params.SetText(sLevel);

            m_view3D.m_tail.bVisible = 1;
            m_lblNpcName.m_tail.bVisible = 1;
            m_lblNpcLevel.m_tail.bVisible = 1;

            if (m_nSelectedCount <= 1 && pData->m_bSelectable)
                m_aLabels[LBL_ACCEPT].m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
            else
                m_aLabels[LBL_ACCEPT].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);

            SetAddRemoveLabel(static_cast<CSWGuiPartySelectionButton*>(pControl)->m_bSelected == 0);
        }

        if (m_pActiveData)
            m_pActiveData->m_button.SetActive(0);
        m_pActiveData = pData;
        pData->m_button.SetActive(1);
    }
    else
    {
        m_view3D.m_tail.bVisible = 0;
        m_lblNpcName.m_tail.bVisible = 0;
        m_lblNpcLevel.m_tail.bVisible = 0;
        m_aLabels[LBL_ACCEPT].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        SetAddRemoveLabel(1);

        if (m_pActiveData)
        {
            m_pActiveData->m_button.SetActive(0);
            m_pActiveData = 0;
        }
    }
}
#else
void CSWGuiPartySelection::OnEnter(CSWGuiControl* pControl)
{
    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    CSWPartyTable* pTable = pServerApp->GetPartyTable();
    CSWGuiPartySelectionData* pData = reinterpret_cast<CSWGuiPartySelectionData*>(pControl->m_tail.m_nUserData);

    if (pTable->GetIsNPCAvailable(pData->m_nIndex))
    {
        CSWSCreature* pCreature = pServerApp->GetCreatureByGameObjectID(
            pTable->GetNPCObject(pData->m_nIndex, 0, 1));

        // Built on first use and kept until OnExit or OnPanelRemoved releases it.
        if (!pData->m_pCreature)
            pData->m_pCreature = new CSWCCreature();

        if (!pCreature)
        {
            // Listed, but the object no longer resolves, so keep the slot usable and say so.
            m_lblNpcName.m_text.m_params.SetText(CExoString("I am broken"));
            m_lblNpcLevel.m_text.m_params.SetText(CExoString("So very very broken"));
            m_lblNpcName.m_tail.bVisible = 1;
            m_lblNpcLevel.m_tail.bVisible = 1;
            m_view3D.m_tail.bVisible = 0;
            m_aLabels[LBL_ACCEPT].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
            SetAddRemoveLabel(1);
        }
        else
        {
            CSWSCreatureStats* pServerStats = pCreature->m_pStats;
            CSWCCreatureStats* pClientStats = pData->m_pCreature->m_pStats;

            // Two different stats classes, so this is a field-by-field seed. The clamps match the
            // char-gen screen's, including the asymmetric one whose unset sentinel is -1.
            pClientStats->m_nGender = pServerStats->m_nGender;

            int nAlignment = pServerStats->m_nAlignment;
            if (nAlignment < 0)
                nAlignment = 0;
            if (nAlignment > 100)
                nAlignment = 100;
            pClientStats->m_alignmentGoodEvil = static_cast<short>(nAlignment);

            int nNPCAlignment = pServerStats->m_nBaseNPCAlignment;
            if (nNPCAlignment < 0)
                nNPCAlignment = -1;
            if (nNPCAlignment > 100)
                nNPCAlignment = 100;
            pClientStats->m_baseCNPCAlignment = static_cast<short>(nNPCAlignment);

            for (uint64_t i = 0; i < pServerStats->m_numClasses; i++)
                pClientStats->SetClass(static_cast<unsigned char>(i), pServerStats->GetClass(static_cast<unsigned char>(i)));

            if (g_bDo3DGUIs)
            {
                // Re-dress the preview creature from its live appearance block, then re-stand the
                // scene.
                CCAppearanceInfo appearance;
                appearance.core = *reinterpret_cast<CCAppearanceCore*>(pData->m_pCreature->m_pCreatureAppearance);
                appearance.core.m_nGender = pCreature->m_appearance.m_nGender;
                appearance.core.m_nAppearanceId = pCreature->m_appearance.m_headId;
                if (appearance.core.m_nAppearanceId == APPEARANCE_HANDMAIDEN_HEAD)
                    appearance.core.m_nAppearanceId = APPEARANCE_HANDMAIDEN;
                appearance.core.m_bArmorEquipped = 1;
                appearance.core.m_bTypeVariant = 2;
                appearance.core.m_bTypeNumber = 1;

                pData->m_pCreature->SetPortraitId(pCreature->GetPortraitId());
                pData->m_pCreature->SetAppearance(&appearance, 3, 1, 0);
                pData->m_pCreature->AttachToScene(m_view3D.m_scene.m_pScene);
                pData->m_pCreature->Setup3DScene(m_view3D, 0, pCreature, 1);
            }

            // "<name> <level> <class>", with the total level and the last class advanced.
            CExoString sLevel;
            sLevel.Format(const_cast<char*>("%s %d %s"),
                          g_pTlkTable->GetSimpleString(STRREF_LEVEL).CStr(),
                          pCreature->m_pStats->GetLevel(0),
                          pCreature->m_pStats->GetClassString(
                              pCreature->m_pStats->m_numClasses - 1).CStr());
            m_lblNpcName.m_text.m_params.SetText(pCreature->m_pStats->GetFullName());
            m_lblNpcLevel.m_text.m_params.SetText(sLevel);

            m_view3D.m_tail.bVisible = 1;
            m_lblNpcName.m_tail.bVisible = 1;
            m_lblNpcLevel.m_tail.bVisible = 1;

            // A filled slot the roster can still take gets the live accept colour, otherwise the
            // caption is greyed.
            if (m_nSelectedCount <= 1 && pData->m_bSelectable)
                m_aLabels[LBL_ACCEPT].m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
            else
                m_aLabels[LBL_ACCEPT].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);

            SetAddRemoveLabel(static_cast<CSWGuiPartySelectionButton*>(pControl)->m_bSelected == 0);
        }

        // To move the highlight, detach and un-latch whatever slot was active, then latch this one.
        if (m_pActiveData && m_pActiveData != pData)
        {
            if (m_pActiveData->m_pCreature)
                m_pActiveData->m_pCreature->AttachToScene(0);
            m_pActiveData->m_button.SetActive(0);
        }
        m_pActiveData = pData;
        pData->m_button.SetActive(1);
    }
    else
    {
        // With no server NPC in this slot, hide the whole info block and drop the highlight.
        m_view3D.m_tail.bVisible = 0;
        m_lblNpcName.m_tail.bVisible = 0;
        m_lblNpcLevel.m_tail.bVisible = 0;
        m_aLabels[LBL_ACCEPT].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        SetAddRemoveLabel(1);

        if (m_pActiveData)
        {
            if (m_pActiveData->m_pCreature)
                m_pActiveData->m_pCreature->AttachToScene(0);
            m_pActiveData->m_button.SetActive(0);
            m_pActiveData = 0;
        }
    }
}
#endif

// Builds the roster: every slot gets its portrait, label visibility, selectable flags and party
// index, then the first available slot is focused and pre-entered.
void CSWGuiPartySelection::OnPanelAdded()
{
#if KOTOR1  // K1: nine slots, no solo-mode gate, the leader is reset, the portrait is the
             // creature's own
    CClientExoApp* pClientApp = g_pAppManager->m_pClientExoApp;
    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    CSWParty* pSWParty = pClientApp->GetSWParty();
    CSWPartyTable* pTable = pServerApp->GetPartyTable();

    if (m_bHasForcedNPC == 0 && m_bFromScript == 0)
    {
        m_btnBack.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
        m_btnBack.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::ENABLED_ICON_COLOR);
    }
    else
    {
        m_btnBack.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        m_btnBack.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
    }

    m_nSelectedCount = m_bHasForcedNPC ? 0 : pTable->m_nNumPartyMembers;
    pSWParty->SetLeader(-3);
    pTable->AddGameInProgress();

    for (int i = 0; i < 9; i++)
    {
        CResRef resRef;
        CSWGuiPartySelectionData* pData = &m_aPartyData[i];
        pData->m_button.SetSelected(0);

        if (pTable->GetIsNPCAvailable(i))
        {
            unsigned long long nObjectId = pTable->GetNPCObject(i, 0, 1);
            CSWSCreature* pCreature = pServerApp->GetCreatureByGameObjectID(nObjectId);
            if (pCreature != 0)
            {
                pTable->SaveMember(i, 0);
                resRef = pCreature->GetPortrait();
                pData->m_bNPCInWorld = 1;
            }
            else
            {
                nObjectId = pTable->GetNPCObject(i, 1, 1);
                CSWSCreature* pCreature2 = pServerApp->GetCreatureByGameObjectID(nObjectId);
                if (pCreature2 != 0)
                    resRef = pCreature2->GetPortrait();
                pData->m_bNPCInWorld = 0;
            }

            pData->m_bSelectable = 1;

            if (pTable->GetIsMember(i))
                pData->m_nPartyIndex =
                    pSWParty->GetIndex(pClientApp->ServerToClientObjectId(nObjectId));
            else
                pData->m_nPartyIndex = -1;

            if (m_bHasForcedNPC != 0)
            {
                if (pData->m_bForced)
                {
                    pData->m_button.SetSelected(1);
                    m_nSelectedCount++;
                    pData->m_bSelectable = 0;
                }
            }
            else if (pTable->GetIsMember(i))
            {
                pData->m_button.SetSelected(1);
            }

            if (pTable->GetNPCSelectability(i))
                pData->m_lblNA.m_border.m_params.m_fAlpha = 1.0f;
            else
            {
                pData->m_lblNA.m_border.m_params.m_fAlpha = 0.25f;
                pData->m_bSelectable = 0;
            }

            pData->m_lblNA.m_tail.bVisible = 1;
            pData->m_lblNA.m_border.m_params.SetFillImage(resRef, 0);
            pData->m_lblChar.m_tail.bVisible = 0;
        }
        else
        {
            pData->m_bSelectable = 0;
            pData->m_bNPCInWorld = 1;
            pData->m_nPartyIndex = -1;
            pData->m_lblChar.m_tail.bVisible = 1;
            pData->m_lblNA.m_tail.bVisible = 0;
        }
    }

    UpdateCount();
    pTable->RemoveGameInProgress();

    SetActiveControl(&m_btnAccept, 0);
    CSWGuiPanel::OnPanelAdded();
    g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_PARTY_SELECTION, OBJECT_INVALID, OBJECT_INVALID, 0);
    SetActiveControl(&m_aPartyData[0].m_button, 0);
#else
    CClientExoApp* pClientApp = g_pAppManager->m_pClientExoApp;
    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    CSWParty* pSWParty = pClientApp->GetSWParty();
    CSWPartyTable* pTable = pServerApp->GetPartyTable();

    // An unescapable area routes every toggle to the solo-mode query box, otherwise the roster is
    // edited directly. The box's mode and StrRef are seeded together.
    m_bCanChangeParty = static_cast<CSWSArea*>(pServerApp->GetCreatureByGameObjectID(
                      pServerApp->GetPlayerCreatureId())->GetArea())->m_nUnescapable
                      ? (m_bFromScript != 0) : 1;
    m_nCannotChangeStrRef = STRREF_CANNOT_CHANGE_PARTY_NOW;

    // A PC that has not finished character creation means the party is still being rolled; the
    // box's text then follows the party's own health and combat state.
#if !KOTOR1  // K1: no party-health / combat gate on the selection box
    unsigned long long nPCObjectId = g_pAppManager->m_pServerExoApp->GetFirstPCObject();
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(nPCObjectId);
    if (pObject == 0 || static_cast<CSWSCreature*>(pObject)->m_bPlayerMadeCharacter != 0)
    {
        if (m_bCanChangeParty != 0)
        {
            if (!g_pAppManager->m_pClientExoApp->GetSWParty()->IsPartyHealthy())
            {
                m_bCanChangeParty = 0;
                m_nCannotChangeStrRef = STRREF_CANNOT_CHANGE_PARTY_WHILE_UNCONSCIOUS;
            }
            else if (m_bCanChangeParty != 0 &&
                     g_pAppManager->m_pClientExoApp->GetSWParty()->IsPartyInCombat())
            {
                m_bCanChangeParty = 0;
                m_nCannotChangeStrRef = STRREF_CANNOT_CHANGE_PARTY_IN_COMBAT;
            }
        }
    }
    else
    {
        m_bCanChangeParty = 0;
        m_nCannotChangeStrRef = STRREF_CANNOT_CHANGE_PARTY_NOW;
    }
#endif

    if (m_bFromScript != 0)
    {
        m_bCanChangeParty = 1;
        m_nCannotChangeStrRef = STRREF_CANNOT_CHANGE_PARTY_NOW;
    }

    // The back button's caption goes grey while the roster is still free to be edited, otherwise
    // it takes the live text colour. The border colour is the opposite of the caption's.
    if ((m_bHasForcedNPC == 0 && m_bFromScript == 0) || m_fChallengeRating != 0)
    {
        m_btnBack.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
        m_btnBack.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
    }
    else
    {
        m_btnBack.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_GREEN);
        m_btnBack.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_TEXT);
    }

    m_nSelectedCount = m_bHasForcedNPC ? 0 : pTable->m_nNumPartyMembers;

    // The roster is filled in while the game-in-progress save is mounted, then unmounted again.
    pTable->AddGameInProgress();
    UpdateVisibleCharacters();
#if !KOTOR1
    pTable->SavePuppet(0, 0);
    pTable->SavePuppet(1, 0);
    pTable->SavePuppet(2, 0);

    if (pTable->GetNumNPCAvailable() >= 10)
        Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_TEN_PARTY_MEMBERS);
#endif

    int nFirstAvailable = -1;
    for (int i = 0; i < 12; i++)
    {
        CResRef resRef;
        CSWGuiPartySelectionData* pData = &m_aPartyData[i];
        pData->m_button.SetSelected(0);

        if (pTable->GetIsNPCAvailable(i))
        {
            if (nFirstAvailable == -1)
                nFirstAvailable = i;

            unsigned long long nObjectId = pTable->GetNPCObject(i, 0, 1);
            CSWSCreature* pCreature = pServerApp->GetCreatureByGameObjectID(nObjectId);
            if (pCreature != 0)
            {
                pTable->SaveMember(i, 0);
                resRef = pCreature->GetPortraitBasedOnAlignment();
                pData->m_bNPCInWorld = 1;
            }
            // Listed, but the object no longer resolves, so retry with creation enabled.
            else
            {
                nObjectId = pTable->GetNPCObject(i, 1, 1);
                CSWSCreature* pCreature2 = pServerApp->GetCreatureByGameObjectID(nObjectId);
                if (pCreature2 != 0)
                    resRef = pCreature2->GetPortraitBasedOnAlignment();
                pData->m_bNPCInWorld = 0;
            }

            pData->m_bSelectable = 1;

            if (pTable->GetIsMember(i))
            {
                int nIndex = pSWParty->GetIndex(
                    pClientApp->ServerToClientObjectId(nObjectId));
                pData->m_nPartyIndex = nIndex;
                if (nIndex == -1)
                {
                    CExoString sTrace;
                    sTrace.Format(const_cast<char*>("While creating the party select screen, NPC %d's ID is "
                                         "Out of Synch with what's in the Client Side Party Table."),
                                  i);
                }
            }
            else
            {
                pData->m_nPartyIndex = -1;
            }

            if (m_bHasForcedNPC != 0)
            {
                // Forced slots are pre-selected and do not count against the roster's own limit.
                if (pData->m_bForced)
                {
                    pData->m_button.SetSelected(1);
                    m_nSelectedCount++;
                    pData->m_bSelectable = 0;
                }
            }
            else if (pTable->GetIsMember(i))
            {
                pData->m_button.SetSelected(1);
            }

            if (pTable->GetNPCSelectability(i))
                pData->m_lblNA.m_border.m_params.m_fAlpha = 1.0f;
            else
            {
                pData->m_lblNA.m_border.m_params.m_fAlpha = 0.25f;
                pData->m_bSelectable = 0;
            }

            if (!pData->m_bHidden)
            {
                pData->m_lblNA.m_tail.bVisible = 1;
                pData->m_lblChar.m_tail.bVisible = 0;
            }

            // Placed at the end of the available branch, so an unavailable slot keeps its previous
            // fill image.
            pData->m_lblNA.m_border.m_params.SetFillImage(resRef, 0);
        }
        else
        {
            // With no server NPC in this slot, the character label keeps the portrait area visible
            // and the availability label is hidden.
            pData->m_bSelectable = 0;
            pData->m_bNPCInWorld = 1;
            pData->m_nPartyIndex = -1;

            if (!pData->m_bHidden)
            {
                pData->m_lblChar.m_tail.bVisible = 1;
                if (IsJoystickConnected())
                    pData->m_lblChar.m_tail.bVisible = 0;
                pData->m_lblNA.m_tail.bVisible = 0;
            }
        }
    }

    UpdateCount();
    pTable->RemoveGameInProgress();

    if (nFirstAvailable != -1)
    {
        SetActiveControl(&m_aPartyData[nFirstAvailable].m_button, 0);
        OnEnter(&m_aPartyData[nFirstAvailable].m_button);
    }
    else
    {
        SetActiveControl(&m_btnBack, 0);
    }

    if (IsJoystickConnected())
        SetActiveControl(&m_btnBack, 0);

    CSWGuiPanel::OnPanelAdded();
    g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_PARTY_SELECTION, OBJECT_INVALID, OBJECT_INVALID, 0);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->EnableCharacterChangeButtons(0);
    SetActiveControl(&m_aPartyData[0].m_button, 0);
#endif
}

#if KOTOR1  // K1: no null-control test and no solo-mode sub-panel; without a forced member the done
             // button commits at once
void CSWGuiPartySelection::OnDone(CSWGuiControl* pControl)
{
    if (!pControl->m_tail.nCurrentEventArg)
        return;

    if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_START || pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_A)
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);

    if (!m_bFromScript)
    {
        AcceptParty();
        return;
    }

    unsigned char nNumFilled = 0;
    unsigned char nNumForced = 0;
    unsigned char nNumSelected = 0;
    for (int i = 0; i < 9; i++)
    {
        nNumFilled += m_aPartyData[i].m_bSelectable;
        nNumForced += m_aPartyData[i].m_bForced;
        if (m_aPartyData[i].m_button.m_bSelected)
            nNumSelected++;
    }

    if (nNumFilled == 0 || nNumForced == 2)
    {
        AcceptParty();
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bInGameMenuShown = 0;
        return;
    }

    CSWGuiMessageBox* pBox =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    pBox->SetAllowCancel(1);
    pBox->SetMessage((nNumSelected < 2 && nNumFilled != 1) ? STRREF_CONFIRM_SMALL_PARTY : STRREF_CONFIRM_PARTY);
    pBox->SetCallback(this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPartySelection::ConfirmPartySelection), 0);
    m_pGuiManager->AddPanel(pBox, 1, 1);
}
#else
void CSWGuiPartySelection::OnDone(CSWGuiControl* pControl)
{
    if (pControl != 0)
    {
        if (!pControl->m_tail.nCurrentEventArg)
            return;

        if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_START ||
            pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_A)
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
    }

    if (m_bFromScript != 0)
    {
        int nNumFilled = 0;
        int nNumForced = 0;
        int nNumSelected = 0;
        for (int i = 0; i < 12; i++)
        {
            nNumFilled += m_aPartyData[i].m_bSelectable;
            nNumForced += m_aPartyData[i].m_bForced;
            if (m_aPartyData[i].m_button.m_bSelected)
                nNumSelected++;
        }

        // Nothing to pick from, or the roster is already the forced pair: done is a plain cancel,
        // the same four statements ConfirmPartySelection runs when the box reports accept.
        if (static_cast<unsigned char>(nNumFilled) == 0 || static_cast<unsigned char>(nNumForced) == 2)
        {
            AcceptParty(0);
            g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bInGameMenuShown = 0;

            unsigned short nFlags = m_stateFlags | PANEL_REMOVE_DEFERRED;
            if ((m_stateFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
                nFlags = (nFlags & ~PANEL_REMOVE_AND_DELETE) | PANEL_REMOVE;
            m_stateFlags = nFlags;
            return;
        }

        CSWGuiMessageBox* pBox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(1);
        pBox->SetMessage((static_cast<unsigned char>(nNumSelected) < 2 && static_cast<unsigned char>(nNumFilled) != 1)
                             ? STRREF_CONFIRM_SMALL_PARTY : STRREF_CONFIRM_PARTY);
        pBox->SetCallback(this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPartySelection::ConfirmPartySelection), 0);
        m_pGuiManager->AddPanel(pBox, 1, 1);
        return;
    }

    // With no sub-panel, re-derive every button's latch from its slot index and commit.
    if (!m_bCanChangeParty)
    {
        for (int i = 0; i < 12; i++)
            m_aPartyData[i].m_button.SetSelected(m_aPartyData[i].m_nPartyIndex != -1);
    }
    AcceptParty(1);
}
#endif

#if KOTOR1  // K1: only ACCEPT is handled; no challenge rating, no DONE or next-member keys
void CSWGuiPartySelection::HandleInputEvent(int type, int val)
{
    if (val != 0 && (type == INPUTEVENT_GUI_BACK || type == INPUTEVENT_GUI_B))
    {
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);

        if (m_bFromScript == 0 && m_bHasForcedNPC == 0)
        {
            for (int i = 0; i < 9; i++)
                m_aPartyData[i].m_button.SetSelected(m_aPartyData[i].m_nPartyIndex != -1);
            AcceptParty();
        }
        else
        {
            CSWGuiMessageBox* pBox =
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
            pBox->SetAllowCancel(0);
            pBox->SetCallback(0, 0, 0);
            pBox->SetMessage(STRREF_CANNOT_CANCEL_PARTY_SELECTION);
            m_pGuiManager->AddPanel(pBox, 1, 1);
        }
    }

    CSWGuiPanel::HandleInputEvent(type, val);
}
#else
void CSWGuiPartySelection::HandleInputEvent(int type, int val)
{
    if (val != 0)
    {
        switch (type)
        {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);

            // With a sub-panel or a forced member and no challenge rating to settle, accept only
            // re-arms the confirmation box; otherwise it commits.
            if ((m_bFromScript != 0 || m_bHasForcedNPC != 0) && m_fChallengeRating == 0)
            {
                CSWGuiMessageBox* pBox =
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
                pBox->SetAllowCancel(0);
                pBox->SetCallback(0, 0, 0);
                pBox->SetMessage(STRREF_CANNOT_CANCEL_PARTY_SELECTION);
                m_pGuiManager->AddPanel(pBox, 1, 1);
            }
            else
            {
                for (int i = 0; i < 12; i++)
                    m_aPartyData[i].m_button.SetSelected(m_aPartyData[i].m_nPartyIndex != -1);
                AcceptParty(1);
            }
            break;

        case INPUTEVENT_GUI_X:
        case INPUTEVENT_GUI_START:
            // A tail call, so the base class's own handler is not reached.
            OnDone(0);
            return;

        case INPUTEVENT_GUI_Y:
            m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            g_pAppManager->m_pClientExoApp->ChangeCharacterToNextLivingPartyMember(0, 0, 1);
            break;
        }
    }

    CSWGuiPanel::HandleInputEvent(type, val);
}
#endif

static inline Vector RotateVector(const Vector& v, const Quaternion& q)
{
    float w = q.w;
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f))
    {
        float x = q.x, y = q.y, z = q.z;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * w;
        float yz = y * z, wy = y * w;
        float wz = z * w;
        Vector r;
        r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) +
              v.z * ((xz + wy) + (xz + wy));
        r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) +
              v.z * ((yz - wx) + (yz - wx));
        r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) +
              v.z * (1.0f - ((xx + yy) + (xx + yy)));
        return r;
    }
    return v;
}

#if KOTOR1  // K1: nine slots, no index re-sync; the panel leaves the modal chain itself
void CSWGuiPartySelection::AcceptParty()
{
    Vector vPos;

    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    CClientExoApp* pClientApp = g_pAppManager->m_pClientExoApp;

    CSWSCreature* pPlayer = pServerApp->GetCreatureByGameObjectID(
        pServerApp->GetPlayerCreatureId());
    CSWParty* pParty = pClientApp->GetSWParty();
    CSWPartyTable* pTable = pServerApp->GetPartyTable();

    for (int i = 0; i < 9; i++)
    {
        CSWGuiPartySelectionData* pData = &m_aPartyData[i];
        if (pData->m_nPartyIndex == -1 || pData->m_button.m_bSelected)
            continue;

        CSWCCreature* pCreature = pParty->GetCharacter(pData->m_nPartyIndex);
        if (!pCreature)
            continue;

        for (int k = 0; k < 9; k++)
        {
            if (m_aPartyData[k].m_nPartyIndex != -1 &&
                m_aPartyData[k].m_nPartyIndex > pData->m_nPartyIndex)
                m_aPartyData[k].m_nPartyIndex--;
        }

        pCreature->SetInParty(0);

        CSWSCreature* pServerCreature = pCreature->GetServerCreature();
        if (pServerCreature)
        {
            pServerCreature->SetInParty(0, 1);
            pTable->RemoveMember(i);
            pData->m_bNPCInWorld = 0;
        }
    }

    Vector vOrientation = pPlayer->m_orientation;
    pParty->RecaulateFollowPoint();

    int nFormationSlot = 1;
    for (int i = 0; i < 9; i++)
    {
        CSWGuiPartySelectionData* pData = &m_aPartyData[i];
        if (pData->m_nPartyIndex != -1)
            continue;
        if (!pData->m_button.m_bSelected)
            continue;

        int nMember = pParty->m_nCharacterCount;
        Vector vOffset = pParty->m_members[nMember].m_formationOffset;
        Vector vFollow = pParty->m_members[nMember].m_followPoint;

        if (vFollow.x == 0.0f && vFollow.y == 0.0f && vFollow.z == 0.0f)
        {
            vPos = pParty->GetWorldFormationPosition(pPlayer->m_position, vOrientation,
                                                     nFormationSlot);
        }
        else
        {
            Quaternion qRot(Vector(0.0f, 0.0f, 1.0f),
                            DegToRad(pParty->m_followData.GetPathData(
                                         pParty->m_members[nMember].m_bFollowPointValid)
                                         ->m_fLeaderYaw));
            vPos = vFollow + RotateVector(vOffset, qRot);
        }

        unsigned long long nObjectId = pTable->SpawnNPC(i, 1, Vector(), Vector(), 1);
        pServerApp->ComputeSafeLocation(vPos, 5.0f, nObjectId, 1, &vPos);

        CSWSCreature* pNewCreature = pServerApp->GetCreatureByGameObjectID(nObjectId);
        CSWCCreature* pNewClientCreature = pNewCreature->GetClientCreature();  // before the null test, as in K2

        Vector vLeaderPos = pParty->GetCharacter(0)->m_position;
        CSWSArea* pArea = pServerApp->GetModule()->GetArea();

        int nFraction = pNewCreature->m_pPathfindInfo->m_nPersonalSpaceBits;
        int nLine = pArea->TestDirectLine(vPos, vLeaderPos, *reinterpret_cast<const float*>(&nFraction),
                                          pNewCreature->m_pPathfindInfo->m_fHeightAlias, 1, 0);
        if (nLine != -3 && nLine != 1)
        {
            vPos = pParty->m_followData.GetPathData(
                       pParty->m_members[nMember].m_bFollowPointValid)
                       ->m_pos;
            pServerApp->ComputeSafeLocation(vPos, 5.0f, nObjectId, 1, &vPos);
        }

        pNewCreature->SetPosition(vPos, 1, 1, 0);
        pNewCreature->SetOrientation(vOrientation);
        pTable->AddMember(i, nObjectId);

        if (pNewCreature)
        {
            pNewCreature->ClearAllActions(1);
            pNewCreature->m_pPathfindInfo->ResetWayPointData();
            pNewCreature->SetInParty(1, 1);
            pNewCreature->AIUpdate();

            if (pPlayer->GetVisibleListElement(pNewCreature->m_objectId) == 0)
                pPlayer->AddToVisibleList(pNewCreature->m_objectId, 1, 1, 0, 0);
        }

        if (pNewClientCreature)
            pNewClientCreature->SetInParty(1);

        pData->m_bNPCInWorld = 1;
        nFormationSlot++;
    }

    for (int i = 0; i < 9; i++)
        m_aPartyData[i].m_bForced = 0;

    pServerApp->m_pInternal->UpdateClientGameObjects(1);
    m_bHasForcedNPC = 0;
    pClientApp->SetRunScript(m_sExitScript);

    if (g_bCreateAllGUIs)
    {
        unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
        if ((m_stateFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
            nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
        m_stateFlags = nFlags;
    }
    else
    {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMessagesPanel = 0;
        m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE_AND_DELETE;
    }
    m_pGuiManager->PopModalPanel();

    if (m_sExitScript != "")
    {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bRunningGuiScript = 1;
        g_pVirtualMachine->RunScript(&m_sExitScript, 0, 0);
        m_sExitScript = "";
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bRunningGuiScript = 0;
    }

    if (m_nReturnInputClass == 0 && g_pAppManager->m_pClientExoApp->GetPausedByCombat())
        g_pAppManager->m_pClientExoApp->SetPausedByCombat(0, 0, 0);

    if (m_bFromScript)
    {
        m_pGuiManager->PlayGuiSound(GUISOUND_FULLSCREEN_PANEL_REMOVED);
        g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC);
    }

    g_pAppManager->m_pClientExoApp->SetInputClass(m_nReturnInputClass, 1);
    CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    pBox->m_nSavedInputClass = m_nReturnInputClass;
}
#else
// Commits the roster: remove the members whose button is not latched, spawn NPCs for latched
// empty slots, then run the pending script and clear the panel's state.
void CSWGuiPartySelection::AcceptParty(int nSelected)
{
    Vector vPos;

    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    CClientExoApp* pClientApp = g_pAppManager->m_pClientExoApp;

    CSWSCreature* pPlayer = pServerApp->GetCreatureByGameObjectID(
        pServerApp->GetPlayerCreatureId());
    CSWParty* pParty = pClientApp->GetSWParty();
    CSWPartyTable* pTable = pServerApp->GetPartyTable();

    int bResync = 1;
    for (int i = 0; i < 12; i++)
    {
        CSWGuiPartySelectionData* pData = &m_aPartyData[i];
        if (pData->m_nPartyIndex == -1 || pData->m_button.m_bSelected)
            continue;

        if (bResync)
        {
            pParty->SetLeader(-3);

            CSWPartyTable* pSyncTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
            for (int k = 0; k < 12; k++)
            {
                if (!pSyncTable->GetIsMember(k))
                    continue;

                unsigned long long nObjId = pSyncTable->GetNPCObject(k, 0, 0);
                m_aPartyData[k].m_nPartyIndex = pParty->GetIndex(
                    g_pAppManager->m_pClientExoApp->ServerToClientObjectId(nObjId));
            }

        // The re-sync runs once.
            bResync = 0;
        }

        CSWCCreature* pCreature = pParty->GetCharacter(pData->m_nPartyIndex);
        if (!pCreature)
        {
            bResync = 0;
            continue;
        }

        // To close the gap, every other slot waiting on a higher party index moves down one.
        for (int k = 0; k < 12; k++)
        {
            if (m_aPartyData[k].m_nPartyIndex != -1 &&
                m_aPartyData[k].m_nPartyIndex > pData->m_nPartyIndex)
                m_aPartyData[k].m_nPartyIndex--;
        }

        pCreature->SetInParty(0);

        CSWSCreature* pServerCreature = pCreature->GetServerCreature();
        if (!pServerCreature)
        {
            bResync = 0;
            continue;
        }

        pServerCreature->SetInParty(0, 1);
        pTable->RemoveMember(i);
        pData->m_bNPCInWorld = 0;
    }

    Vector vOrientation = pPlayer->m_orientation;
    pParty->RecaulateFollowPoint();

    int nFormationSlot = 1;
    for (int i = 0; i < 12; i++)
    {
        CSWGuiPartySelectionData* pData = &m_aPartyData[i];
        if (pData->m_nPartyIndex != -1)
            continue;
        if (!pData->m_button.m_bSelected)
            continue;

        // The slot OnPanelAdded left empty is filled from the next member record's own follow
        // point and formation offset; that record is the first not yet registered, so its index is
        // the character count.
        int nMember = pParty->m_nCharacterCount;
        Vector vOffset = pParty->m_members[nMember].m_formationOffset;
        Vector vFollow = pParty->m_members[nMember].m_followPoint;

        if (vFollow.x == 0.0f && vFollow.y == 0.0f && vFollow.z == 0.0f)
        {
            vPos = pParty->GetWorldFormationPosition(pPlayer->m_position, vOrientation,
                                                     nFormationSlot);
        }
        else
        {
            // The follow sample's yaw is in degrees; the rotation is about +Z.
            Quaternion qRot(Vector(0.0f, 0.0f, 1.0f),
                            DegToRad(pParty->m_followData.GetPathData(
                                         pParty->m_members[nMember].m_bFollowPointValid)
                                         ->m_fLeaderYaw));
            vPos = vFollow + RotateVector(vOffset, qRot);
        }

        unsigned long long nObjectId = pTable->SpawnNPC(i, 1, Vector(), Vector(), 1);
        pServerApp->ComputeSafeLocation(vPos, 5.0f, nObjectId, 1, &vPos);

        CSWSCreature* pNewCreature = pServerApp->GetCreatureByGameObjectID(nObjectId);
        // Called unconditionally, before the null check below: the fresh id is dereferenced even
        // when GetCreatureByGameObjectID failed.
        CSWCCreature* pNewClientCreature = pNewCreature->GetClientCreature();

        Vector vLeaderPos = pParty->GetCharacter(0)->m_position;
        CSWSArea* pArea = pServerApp->GetModule()->GetArea();

        int nFraction = pNewCreature->m_pPathfindInfo->m_nPersonalSpaceBits;
        int nLine = pArea->TestDirectLine(vPos, vLeaderPos, *reinterpret_cast<const float*>(&nFraction),
                                          pNewCreature->m_pPathfindInfo->m_fHeightAlias, 1, 0);
        if (nLine != -3 && nLine != 1)
        {
            // When blocked (or creature-blocked), fall back to the recorded follow sample's own
            // point.
            vPos = pParty->m_followData.GetPathData(
                       pParty->m_members[nMember].m_bFollowPointValid)
                       ->m_pos;
            pServerApp->ComputeSafeLocation(vPos, 5.0f, nObjectId, 1, &vPos);
        }

        pNewCreature->SetPosition(vPos, 1, 1, 0);
        pNewCreature->SetOrientation(vOrientation);
        pTable->AddMember(i, nObjectId);

        if (pNewCreature)
        {
            pNewCreature->ClearAllActions(1);
            pNewCreature->m_pPathfindInfo->ResetWayPointData();
            pNewCreature->SetInParty(1, 1);
            pNewCreature->AIUpdate();

            // Registered in the player's own visibility list so the new member renders.
            if (pPlayer->GetVisibleListElement(pNewCreature->m_objectId) == 0)
                pPlayer->AddToVisibleList(pNewCreature->m_objectId, 1, 1, 0, 0);
        }

        if (pNewClientCreature)
            pNewClientCreature->SetInParty(1);

        pData->m_bNPCInWorld = 1;
        nFormationSlot++;
    }

    for (int i = 0; i < 12; i++)
        m_aPartyData[i].m_bForced = 0;

    pServerApp->m_pInternal->UpdateClientGameObjects(1);
    m_bHasForcedNPC = 0;

    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSWInGameGui(0))
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);

    // The panel's pending script name, seeded by ShowPartySelection's text argument, is consumed
    // here: it runs, and the g_nRunScriptVar word tells the script whether this was a cancel.
    if (m_sExitScript != "")
    {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bRunningGuiScript = 1;
        g_nRunScriptVar = nSelected == 0;
        g_pVirtualMachine->RunScript(&m_sExitScript, 0, 0);
        m_sExitScript = "";
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bRunningGuiScript = 0;
    }

    if (m_bFromScript)
    {
        if (g_pAppManager->m_pClientExoApp->GetPausedByCombat())
            g_pAppManager->m_pClientExoApp->GetSWCMessage()
                ->SendPlayerToServerInput_TogglePauseRequest();

        if (m_bFromScript)
        {
            m_pGuiManager->PlayGuiSound(GUISOUND_FULLSCREEN_PANEL_REMOVED);
            g_pExoSound->SetSoundMode(CExoSound::CEXOSOUND_MODE_GENERIC, 0);
            g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);

            // State 4 (bits 8-9 == 2) just drops bit 7, anything else becomes state 2.
            unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
            if ((m_stateFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
                nFlags = (nFlags & ~PANEL_REMOVE_MASK) | PANEL_REMOVE;
            m_stateFlags = nFlags;
        }
    }
}
#endif

#if !KOTOR1  // K1 has nine fixed slots and no spare pages
// Points each of the two spare pages at either its real roster slot (10, then 11) or a fallback
// (7, then 4), depending on whether the server says that NPC is available.
void CSWGuiPartySelection::UpdateVisibleCharacters()
{
    CSWPartyTable* pTable = g_pAppManager->m_pServerExoApp->GetPartyTable();

    // Each half is independent. The spare page gets the visibility of whichever roster slot is
    // available, and the hidden slot's flag mirrors it.
    int nShow;
    CSWGuiButton* pShowBtn;
    if (pTable->GetIsNPCAvailable(10))
    {
        m_aPartyData[7].m_lblNA.m_tail.bVisible = 0;
        m_aPartyData[7].m_lblChar.m_tail.bVisible = 0;
        m_aPartyData[7].m_button.m_tail.bVisible = 0;
        m_aPartyData[7].m_bHidden = 1;
        m_aPartyData[10].m_lblNA.m_tail.bVisible = 1;
        m_aPartyData[10].m_lblChar.m_tail.bVisible = 1;
        m_aPartyData[10].m_button.m_tail.bVisible = 1;
        nShow = 10;
        pShowBtn = &m_aPartyData[10].m_button;
    }
    else
    {
        m_aPartyData[10].m_button.m_tail.bVisible = 0;
        m_aPartyData[10].m_lblNA.m_tail.bVisible = 0;
        m_aPartyData[10].m_lblChar.m_tail.bVisible = 0;
        m_aPartyData[10].m_bHidden = 1;
        m_aPartyData[7].m_button.m_tail.bVisible = 1;
        m_aPartyData[7].m_lblNA.m_tail.bVisible = 1;
        m_aPartyData[7].m_lblChar.m_tail.bVisible = 1;
        nShow = 7;
        pShowBtn = &m_aPartyData[7].m_button;
    }
    m_aPartyData[nShow].m_bHidden = 0;

    m_aPartyData[5].m_button.SetMoveToControl(2, pShowBtn);
    m_aPartyData[0].m_button.SetMoveToControl(
        0, IsJoystickConnected() ? pShowBtn : &m_btnDone);
    m_aPartyData[8].m_button.SetMoveToControl(1, pShowBtn);
    m_aPartyData[9].m_button.SetMoveToControl(3, pShowBtn);
    m_aPartyData[nShow].m_button.SetMoveToControl(
        2, IsJoystickConnected() ? &m_aPartyData[0].m_button : &m_btnDone);
    m_aPartyData[nShow].m_button.SetMoveToControl(0, &m_aPartyData[5].m_button);
    m_aPartyData[nShow].m_button.SetMoveToControl(1, &m_aPartyData[9].m_button);
    m_aPartyData[nShow].m_button.SetMoveToControl(3, &m_aPartyData[8].m_button);

    if (pTable->GetIsNPCAvailable(11))
    {
        m_aPartyData[4].m_button.m_tail.bVisible = 0;
        m_aPartyData[4].m_lblNA.m_tail.bVisible = 0;
        m_aPartyData[4].m_lblChar.m_tail.bVisible = 0;
        m_aPartyData[4].m_bHidden = 1;
        m_aPartyData[11].m_button.m_tail.bVisible = 1;
        m_aPartyData[11].m_lblNA.m_tail.bVisible = 1;
        m_aPartyData[11].m_lblChar.m_tail.bVisible = 1;
        m_aPartyData[11].m_bHidden = 0;
        m_aPartyData[2].m_button.SetMoveToControl(2, &m_aPartyData[11].m_button);
        m_aPartyData[6].m_button.SetMoveToControl(0, &m_aPartyData[11].m_button);
        m_aPartyData[3].m_button.SetMoveToControl(1, &m_aPartyData[11].m_button);
        m_aPartyData[3].m_button.SetMoveToControl(3, &m_aPartyData[11].m_button);
        m_aPartyData[11].m_button.SetMoveToControl(0, &m_aPartyData[2].m_button);
        m_aPartyData[11].m_button.SetMoveToControl(2, &m_aPartyData[6].m_button);
        m_aPartyData[11].m_button.SetMoveToControl(1, &m_aPartyData[3].m_button);
        m_aPartyData[11].m_button.SetMoveToControl(3, &m_aPartyData[3].m_button);
    }
    else
    {
        m_aPartyData[11].m_button.m_tail.bVisible = 0;
        m_aPartyData[11].m_lblNA.m_tail.bVisible = 0;
        m_aPartyData[11].m_lblChar.m_tail.bVisible = 0;
        m_aPartyData[11].m_bHidden = 1;
        m_aPartyData[4].m_button.m_tail.bVisible = 1;
        m_aPartyData[4].m_lblNA.m_tail.bVisible = 1;
        m_aPartyData[4].m_lblChar.m_tail.bVisible = 1;
        m_aPartyData[4].m_bHidden = 0;
        m_aPartyData[2].m_button.SetMoveToControl(2, &m_aPartyData[4].m_button);
        m_aPartyData[6].m_button.SetMoveToControl(0, &m_aPartyData[4].m_button);
        m_aPartyData[3].m_button.SetMoveToControl(1, &m_aPartyData[4].m_button);
        m_aPartyData[3].m_button.SetMoveToControl(3, &m_aPartyData[4].m_button);
        m_aPartyData[4].m_button.SetMoveToControl(0, &m_aPartyData[2].m_button);
        m_aPartyData[4].m_button.SetMoveToControl(2, &m_aPartyData[6].m_button);
        m_aPartyData[4].m_button.SetMoveToControl(1, &m_aPartyData[3].m_button);
        m_aPartyData[4].m_button.SetMoveToControl(3, &m_aPartyData[3].m_button);
    }
}
#endif

void CSWGuiPartySelection::OnExit(CSWGuiControl* pControl)
{
#if KOTOR1
    K2_UNUSED(pControl);
#endif
    // The roster record the constructor parked in the button's control payload; it owns the
    // lazily built 3D-preview creature.
#if !KOTOR1
    CSWGuiPartySelectionData* pData = reinterpret_cast<CSWGuiPartySelectionData*>(pControl->m_tail.m_nUserData);
    if (pData->m_pCreature)
    {
        delete pData->m_pCreature;
        pData->m_pCreature = 0;
    }
#endif
}

void CSWGuiPartySelection::OnEnterDone(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    // Drop the info block back to its idle state: the 3D preview, then the name and level pair.
    m_view3D.m_tail.bVisible = 0;
    m_lblNpcName.m_tail.bVisible = 0;
    m_lblNpcLevel.m_tail.bVisible = 0;

#if KOTOR1
    m_aLabels[LBL_ACCEPT].m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
#else
    m_aLabels[LBL_ACCEPT].m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_GREEN);
#endif
    m_aLabels[LBL_ACCEPT].m_text.m_params.SetStrRef(STRREF_ACCEPT);
}

void CSWGuiPartySelection::ConfirmPartySelection(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    // Only when the "are you sure?" box the top bar popped is reporting accept.
#if KOTOR1  // K1: AcceptParty does its own input class and modal-chain teardown
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted)
        AcceptParty();
#else
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted)
    {
        AcceptParty(0);
        g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bInGameMenuShown = 0;

        // Same state-flag write CSWGuiMessageBox::HandleInputEvent ends with.
        unsigned short nFlags = m_stateFlags | PANEL_REMOVE_DEFERRED;
        if ((m_stateFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
            nFlags = (nFlags & ~PANEL_REMOVE_AND_DELETE) | PANEL_REMOVE;
        m_stateFlags = nFlags;
    }
#endif
}

void CSWGuiPartySelection::OnPanelRemoved()
{
    CSWPartyTable* pParty = g_pAppManager->m_pServerExoApp->GetPartyTable();
#if KOTOR1  // K1: kills the unspawned NPCs and nothing else
    for (int i = 0; i < 9; i++)
    {
        if (!m_aPartyData[i].m_bNPCInWorld)
            pParty->KillNPCObject(i, 1);
    }
#else

    // The result is unused, but GetSWParty is not pure, so the call stays.
    g_pAppManager->m_pClientExoApp->GetSWParty();

    for (int i = 0; i < 12; i++)
    {
        // Only the slots without this flag hold a server-side NPC to kill.
        if (!m_aPartyData[i].m_bNPCInWorld)
            pParty->KillNPCObject(i, 1);

        if (m_aPartyData[i].m_pCreature)
        {
            delete m_aPartyData[i].m_pCreature;
            m_aPartyData[i].m_pCreature = 0;
        }
    }

    g_pAppManager->m_pClientExoApp->GetInGameGui()->EnableCharacterChangeButtons(1);
    m_bFromScript = 0;
    m_fChallengeRating = 0;
#endif

    CSWGuiPanel::OnPanelRemoved();
}

void CSWGuiPartySelection::UpdateCount()
{
    CExoString sCount;
    sCount.Format(const_cast<char*>("%d"), 2 - m_nSelectedCount);
#if !KOTOR1
    sCount = sCount + CExoString(" ") + g_pTlkTable->GetSimpleString(STRREF_AVAILABLE_SLOTS);
#endif
    m_lblCount.m_text.m_params.SetText(sCount);
}

void CSWGuiPartySelection::SetAddRemoveLabel(int bAdd)
{
    if (bAdd)
    {
        m_aLabels[LBL_ACCEPT].m_text.m_params.SetStrRef(STRREF_ADD);
        m_btnAccept.m_text.m_params.SetStrRef(STRREF_ADD);
    }
    else
    {
        m_aLabels[LBL_ACCEPT].m_text.m_params.SetStrRef(STRREF_REMOVE);
        m_btnAccept.m_text.m_params.SetStrRef(STRREF_REMOVE);
    }

    // The two-selected check lives in the add arm; removing a label never reads the selection
    // count. Each arm writes its own colour.
#if KOTOR1  // K1: no unescapable-area gate, and different colours
    if (!bAdd || m_nSelectedCount <= 1)
    {
        m_btnAccept.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
        m_btnAccept.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::ENABLED_ICON_COLOR);
    }
    else
    {
        m_btnAccept.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        m_btnAccept.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
    }

    IosAddButtonIcon(&m_btnAccept, 'x');
#else
    if ((!bAdd || m_nSelectedCount <= 1) && m_bCanChangeParty != 0)
    {
        m_btnAccept.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
        m_btnAccept.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
    }
    else
    {
        m_btnAccept.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_GREEN);
        m_btnAccept.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_TEXT);
    }

    AspyrAddButtonIcon(&m_btnAccept, 'a');
#endif
}

void CSWGuiPartySelection::SetForcedNPC(int nIndex, int nSecond)
{
    // Both arguments are roster indices, range-checked the same way.
#if KOTOR1  // K1: nine slots
    if (nIndex >= 0 && nIndex <= 8)
    {
        m_bHasForcedNPC = 1;
        m_aPartyData[nIndex].m_bForced = 1;
    }
    if (nSecond >= 0 && nSecond <= 8)
#else
    if (nIndex >= 0 && nIndex <= 11)
    {
        m_bHasForcedNPC = 1;
        m_aPartyData[nIndex].m_bForced = 1;
    }
    if (nSecond >= 0 && nSecond <= 11)
#endif
    {
        m_bHasForcedNPC = 1;
        m_aPartyData[nSecond].m_bForced = 1;
    }
}
