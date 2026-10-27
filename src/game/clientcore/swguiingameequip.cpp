#include "k2stdint.h"
#include "strref.h"
#include "swguiingameequip.h"

#include "swguicontrol.h"
#include "exoresman.h"
#include "swguidrawable.h"
#include "Vector.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "swguiingame.h"
#include "nwcarea.h"
#include "nwsarea.h"
#include "guiingame.h"
#include "swparty.h"
#include "nwtlktable.h"
#include "CExoArrayListImpl.h"
#include "nwccreature.h"
#include "nwccreaturestats.h"
#include "nwccreatureappearance.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "nwrace.h"
#include "partytables.h"
#include "nwsitem.h"
#include "nwscreature.h"
#include "nwitem.h"
#include "nwscombatround.h"
#include "nwcitem.h"
#include "serverexoapp.h"
#include "nwsmodule.h"
#include "gameobjectarray.h"
#include "clientoptions.h"
#include "swgui.h"
#include "nwchandlemessage.h"
#include "twodimarray.h"
#include "nwsplaceable.h"
#include "nwscriptconst.h"
#include "exoinput.h"
#include "tutorial.h"


float GetResolutionTextScalar();

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/k2qolapi.h"
#include "exostringinline.h"

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);
#if KOTOR1
#include "serverinfo.h"
#include "port/k1gui.h"
#include "aurgui.h"
void GameMouseMove(bool bAbsolute, int nX, int nY);
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);
bool IosIsIphone(int& nScreenWidth, int& nScreenHeight);
#endif

// The attack animation's length, in milliseconds, of the attack an equip or unequip re-queues.
enum { ATTACK_ANIMATION_TIME_MS = 1500 };

#if KOTOR1
const CSWGuiInGameEquip::SlotInfo CSWGuiInGameEquip::SLOT_INFO[9] = {
#else
const CSWGuiInGameEquip::SlotInfo CSWGuiInGameEquip::SLOT_INFO[11] = {
#endif
    { CSWInventory::SLOT_LEFTWEAPON,     "INV_WEAP_L",  "weap_l",    STRREF_SLOT_LEFT_WEAPON, {}, STRREF_SLOT_LEFT_WEAPON },
    { CSWInventory::SLOT_RIGHTWEAPON,    "INV_WEAP_R",  "weap_r",    STRREF_SLOT_RIGHT_WEAPON, {}, STRREF_SLOT_RIGHT_WEAPON },
    { CSWInventory::SLOT_HEAD,           "INV_HEAD",    "head",      STRREF_SLOT_HEAD, {}, STRREF_SLOT_DROID_SENSOR },
    { CSWInventory::SLOT_LEFTARM,        "INV_ARM_L",   "forearm_l", STRREF_SLOT_LEFT_ARM, {}, STRREF_SLOT_DROID_SPECIAL_WEAPON },
    { CSWInventory::SLOT_RIGHTARM,       "INV_ARM_R",   "forearm_r", STRREF_SLOT_RIGHT_ARM, {}, STRREF_SLOT_DROID_SPECIAL_WEAPON },
    { CSWInventory::SLOT_BODY,           "INV_BODY",    "armor",     STRREF_SLOT_BODY, {}, STRREF_SLOT_DROID_PLATING },
    { CSWInventory::SLOT_HANDS,          "INV_HANDS",   "hands",     STRREF_SLOT_HANDS, {}, STRREF_SLOT_DROID_UTILITY },
    { CSWInventory::SLOT_IMPLANT,        "INV_IMPLANT", "implant",   STRREF_SLOT_IMPLANT, {}, STRREF_SLOT_DROID_UTILITY },
    { CSWInventory::SLOT_BELT,           "INV_BELT",    "belt",      STRREF_SLOT_BELT, {}, STRREF_SLOT_DROID_SHIELD },
#if !KOTOR1
    { CSWInventory::SLOT_LEFTWEAPON2,    "INV_WEAP_L2", "weap_l",    STRREF_SLOT_LEFT_WEAPON, {}, STRREF_SLOT_LEFT_WEAPON },
    { CSWInventory::SLOT_RIGHTWEAPON2,   "INV_WEAP_R2", "weap_r",    STRREF_SLOT_RIGHT_WEAPON, {}, STRREF_SLOT_RIGHT_WEAPON },
#endif
};

CSWGuiInGameEquip::CSWGuiInGameEquip(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1
    // K1: the "equip" layout, nine slots, party-change and character-switch buttons.
    m_pCreature = 0;
    m_slot.m_bFlags &= ~SLOTSTATE_SELECTING_ITEM;
    m_slot.m_nShownNPC = -1;

    StartLoadFromLayout(CResRef("equip"));

    CExoString sBtnPrefix("BTN_");
    CExoString sLblPrefix("LBL_");
    for (int i = 0; i < 9; i++) {
        InitControl(m_abtnSlot[i], sBtnPrefix + CExoString(SLOT_INFO[i].m_pName), 1);
        InitControl(m_apSlotLabels[i], sLblPrefix + CExoString(SLOT_INFO[i].m_pName), 1);
        m_abtnSlot[i].AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameEquip::OnSelectSlot));
        m_abtnSlot[i].AddEvent(SWGUIEVENT_FOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameEquip::OnEnterSlot));
        m_abtnSlot[i].m_tail.m_nUserData = i;
        m_abtnSlot[i].m_borderHilight.m_params.SetPulsingAlpha(1, 0, 1);
        m_slot.m_aoidEquipped[i] = OBJECT_INVALID;
    }

    InitControl(m_lblAtkL, CExoString("LBL_ATKL"), 1);
    InitControl(m_lblToHitL, CExoString("LBL_TOHITL"), 1);
    InitControl(m_lblAtkR, CExoString("LBL_ATKR"), 1);
    InitControl(m_lblToHitR, CExoString("LBL_TOHITR"), 1);
    InitControl(m_lblDef, CExoString("LBL_DEF"), 1);
    InitControl(m_lblVitality, CExoString("LBL_VITALITY"), 1);
    InitControl(m_lbItems, CExoString("LB_ITEMS"), 1);
    InitControl(m_lbDesc, CExoString("LB_DESC"), 1);
    int nScreenW, nScreenH;
    if (IosIsIphone(nScreenW, nScreenH)) {
        CSWGuiExtent extent = m_lbDesc.m_extent;
        extent.w -= 5;
        m_lbDesc.SetExtent(extent);
        m_lbDesc.SetPadding(10);
    }
    InitControl(m_lblPortrait, CExoString("LBL_PORTRAIT"), 1);
    InitControl(m_lblAttackInfo, CExoString("LBL_ATTACK_INFO"), 1);
    InitControl(m_lblDefInfo, CExoString("LBL_DEF_INFO"), 1);
    InitControl(m_lblTxtBar, CExoString("LBL_TXTBAR"), 1);
    InitControl(m_lblPortBord, CExoString("LBL_PORT_BORD"), 1);
    InitControl(m_lblToHit, CExoString("LBL_TOHIT"), 1);
    InitControl(m_lblDamage, CExoString("LBL_DAMAGE"), 1);
    InitControl(m_lblSlotName, CExoString("LBL_SLOTNAME"), 1);
    InitControl(m_lblTitle, CExoString("LBL_TITLE"), 1);
    InitControl(m_lblSelectTitle, CExoString("LBL_SELECTTITLE"), 1);
    InitControl(m_btnEquip, CExoString("BTN_EQUIP"), 1);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1);
    InitControl(m_btnChange1, CExoString("BTN_CHANGE1"), 1);
    InitControl(m_btnChange2, CExoString("BTN_CHANGE2"), 1);
    InitControl(m_btnCharLeft, CExoString("BTN_CHARLEFT"), 1);
    InitControl(m_btnCharRight, CExoString("BTN_CHARRIGHT"), 1);
    InitControl(m_lblCantEquip, CExoString("LBL_CANTEQUIP"), 1);
    StopLoadFromLayout();
    ScaleExtentForResolution();

    m_lblAtkL.m_text.m_params.SetText(CExoString(""));
    m_lblAtkR.m_text.m_params.SetText(CExoString(""));
    m_lblToHitL.m_text.m_params.SetText(CExoString(""));
    m_lblToHitR.m_text.m_params.SetText(CExoString(""));
    m_lblDef.m_text.m_params.SetText(CExoString(""));

    CSWGuiControl* pProto = m_lbDesc.m_pProtoItem;
    m_lblDescription.Initialize(pProto->m_extent, static_cast<CSWGuiLabel*>(pProto), 1.0f);

    ShowDescription(0);

    m_btnEquip.m_tail.bFocusOnHover = 0;
    m_btnEquip.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameEquip::OnOKPressed));
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnChange1.m_tail.bFocusOnHover = 0;
    m_btnChange1.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameEquip::OnChangeCharacter));
    m_btnChange2.m_tail.bFocusOnHover = 0;
    m_btnChange2.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameEquip::OnChangeCharacter));
    m_btnEquip.m_tail.bVisible = 0;
    m_btnChange1.m_nToolTipStrRef = STRREF_CHANGE_CHARACTER;
    m_btnChange1.m_nBoundInputEventId = INPUTEVENT_CHANGE_CHARACTER;
    m_btnChange2.m_nToolTipStrRef = STRREF_CHANGE_CHARACTER;
    m_btnCharLeft.m_tail.bFocusOnHover = 0;
    m_btnCharLeft.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameEquip::OnSwitchLeft));
    m_btnCharRight.m_tail.bFocusOnHover = 0;
    m_btnCharRight.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameEquip::OnSwitchRight));
    m_btnCharRight.m_border.m_params.m_fRotate = 180.0f;
    m_btnCharRight.m_borderHilight.m_params.m_fRotate = 180.0f;
    m_lbItems.SetEnabled(0);
    m_lblCantEquip.m_tail.bVisible = 0;

    SetActiveControl(static_cast<CSWGuiControl*>(&m_abtnSlot[GUI_SLOT_BODY]), 0);

    m_slot.m_nPendingSlot = 0;
    m_slot.m_oidPendingItem = OBJECT_INVALID;
    m_slot.m_oidSavedItem = 0;
    m_slot.m_pSavedItemCopy = 0;
    m_slot.m_oidSavedOffHandItem = 0;
    m_slot.m_pSavedOffHandCopy = 0;

#ifndef K2_UI_TOUCH
#ifdef K1_UI_XBOX
    m_iconChange.InitAndAddCorner(this, 'r', m_btnChange1.m_extent);   // RB: next party member
#else
    m_iconChange.InitAndAddCorner(this, 'y', m_btnChange1.m_extent);   // corner badge
#endif
#else
    m_iconChange.InitAndAdd(this, 'y', m_btnChange1.m_extent, -30, -30);
#endif
    IosAddButtonIcon(&m_btnEquip, 'a');
#else
    m_pCreature = 0;
    m_slot.m_bFlags &= ~SLOTSTATE_SELECTING_ITEM;

    StartLoadFromLayout(CResRef("equip_p"), 1, 1, 1);
    m_stateFlags |= 1;

    CExoString sBtnPrefix("BTN_");
    CExoString sLblPrefix("LBL_");
    for (int i = 0; i < 11; i++) {
        InitControl(m_abtnSlot[i], sBtnPrefix + CExoString(SLOT_INFO[i].m_pName), 1, 1, 0, 0, 0);
        InitControl(m_apSlotLabels[i], sLblPrefix + CExoString(SLOT_INFO[i].m_pName), 1, 1, 0, 0, 0);
        m_abSlotLocked[i] = 0;
        m_abtnSlot[i].AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameEquip::OnSelectSlot));
        m_abtnSlot[i].AddEvent(SWGUIEVENT_FOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameEquip::OnEnterSlot));
        m_abtnSlot[i].m_tail.m_nUserData = i;
        m_abtnSlot[i].m_borderHilight.m_params.SetPulsingAlpha(0, 0, 0);
        m_slot.m_aoidEquipped[i] = OBJECT_INVALID;
    }

    InitControl(m_lblAtkL2, CExoString("LBL_ATKL2"), 1, 1, 0, 0, 0);
    InitControl(m_lblToHitL2, CExoString("LBL_TOHITL2"), 1, 1, 0, 0, 0);
    InitControl(m_lblAtkR2, CExoString("LBL_ATKR2"), 1, 1, 0, 0, 0);
    InitControl(m_lblToHitR2, CExoString("LBL_TOHITR2"), 1, 1, 0, 0, 0);
    InitControl(m_lblAtkL, CExoString("LBL_ATKL"), 1, 1, 0, 0, 0);
    InitControl(m_lblToHitL, CExoString("LBL_TOHITL"), 1, 1, 0, 0, 0);
    InitControl(m_lblAtkR, CExoString("LBL_ATKR"), 1, 1, 0, 0, 0);
    InitControl(m_lblToHitR, CExoString("LBL_TOHITR"), 1, 1, 0, 0, 0);
    InitControl(m_lbDesc, CExoString("LB_DESC"), 1, 1, 0, 0, 0);
    m_lbDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_lblAttackInfo, CExoString("LBL_ATTACK_INFO"), 1, 1, 0, 0, 0);
    InitControl(m_lblDefInfo, CExoString("LBL_DEF_INFO"), 1, 1, 0, 0, 0);
    InitControl(m_lblDefText, CExoString("LBL_DEF_TEXT"), 1, 1, 0, 0, 0);
    InitControl(m_lblToHit, CExoString("LBL_TOHIT"), 1, 1, 0, 0, 0);
    InitControl(m_lblDamage, CExoString("LBL_DAMAGE"), 1, 1, 0, 0, 0);
    InitControl(m_lblSlotName, CExoString("LBL_SLOTNAME"), 1, 1, 0, 0, 0);
    InitControl(m_lblTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_btnEquip, CExoString("BTN_EQUIP"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);
    InitControl(m_lblCantEquip, CExoString("LBL_CANTEQUIP"), 1, 1, 0, 0, 0);
    InitControl(m_lbItems, CExoString("LB_ITEMS"), 1, 1, 0, 0, 0);
    InitControl(m_alblBar[0], CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_alblBar[1], CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_alblBar[2], CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
    InitControl(m_alblBar[3], CExoString("LBL_BAR4"), 1, 1, 0, 0, 0);
    InitControl(m_alblBar[4], CExoString("LBL_BAR5"), 1, 1, 0, 0, 0);
    InitControl(m_lblBack1, CExoString("LBL_BACK1"), 1, 1, 0, 0, 0);
    InitControl(m_lblAttackMod, CExoString("LBL_ATTACKMOD"), 1, 1, 0, 0, 0);
    InitControl(m_lblDamText, CExoString("LBL_DAMTEXT"), 1, 1, 0, 0, 0);
    InitControl(m_lblDef, CExoString("LBL_DEF"), 1, 1, 0, 0, 0);
    InitControl(m_lblDefBack, CExoString("LBL_DEF_BACK"), 1, 1, 0, 0, 0);
    InitControl(m_btnSwapWeapons, CExoString("BTN_SWAPWEAPONS"), 1, 1, 0, 0, 0);
    AspyrAddButtonIcon(&m_btnSwapWeapons, 'x');
    InitControl(m_btnPrevNPC, CExoString("BTN_PREVNPC"), 1, 1, 0, 0, 0);
    InitControl(m_btnNextNPC, CExoString("BTN_NEXTNPC"), 1, 1, 0, 0, 0);
    StopLoadFromLayout();

    m_lblAtkL.m_text.m_params.SetText(CExoString(""));
    m_lblAtkR.m_text.m_params.SetText(CExoString(""));
    m_lblToHitL.m_text.m_params.SetText(CExoString(""));
    m_lblToHitR.m_text.m_params.SetText(CExoString(""));
    m_lblAtkL2.m_text.m_params.SetText(CExoString(""));
    m_lblToHitL2.m_text.m_params.SetText(CExoString(""));
    m_lblAtkR2.m_text.m_params.SetText(CExoString(""));
    m_lblToHitR2.m_text.m_params.SetText(CExoString(""));
    m_lblDef.m_text.m_params.SetText(CExoString(""));

    CSWGuiControl* pProto = m_lbDesc.m_pProtoItem;
    CSWGuiExtent extent = pProto->m_extent;
    extent.h = static_cast<int>(static_cast<float>(extent.h) * GetResolutionTextScalar());
    m_lblDescription.Initialize(extent, static_cast<CSWGuiLabel*>(pProto), 1.0f);

    ShowDescription(0);

    m_btnEquip.m_tail.bFocusOnHover = 0;
    m_btnEquip.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameEquip::OnOKPressed));
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnEquip.m_tail.bVisible = 0;
    m_lbItems.SetEnabled(0);
    m_lblCantEquip.m_tail.bVisible = 0;

    m_slot.m_nPendingSlot = 0;
    m_slot.m_oidPendingItem = OBJECT_INVALID;
    m_slot.m_oidSavedItem = 0;
    m_slot.m_pSavedItemCopy = 0;
    m_slot.m_oidSavedOffHandItem = 0;
    m_slot.m_pSavedOffHandCopy = 0;

    m_btnSwapWeapons.m_tail.bFocusOnHover = 0;
    m_btnSwapWeapons.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnXButtonPressed));

    SetActiveControl(static_cast<CSWGuiControl*>(&m_abtnSlot[GUI_SLOT_BODY]), 0);

    m_slot.m_fSwapWeaponsCooldown = 0;
    m_slot.m_bNPCOverride = 0;
    m_slot.m_nOverrideNPC = NPC_PLAYER;

    m_btnPrevNPC.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameEquip::OnPrevNPCClicked));
    m_btnPrevNPC.m_tail.bFocusOnHover = 0;
    m_btnNextNPC.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameEquip::OnNextNPCClicked));
    m_btnNextNPC.m_tail.bFocusOnHover = 0;
#endif
}

CSWGuiInGameEquip::~CSWGuiInGameEquip() {
    m_lbDesc.ClearItems();
    m_lbItems.ClearItems();

    for (int i = 0; i < m_lstItemEntries.m_count; i++) {
        CSWGuiControl* pControl = static_cast<CSWGuiControl*>(m_lstItemEntries.m_pArray[i]);
        if (pControl != 0)
            delete pControl;
        m_lstItemEntries.m_pArray[i] = 0;
    }

    m_lstItemEntries.RemoveAll(1);

    if (m_slot.m_pSavedItemCopy != 0) {
        delete m_slot.m_pSavedItemCopy;
        m_slot.m_pSavedItemCopy = 0;
    }
    if (m_slot.m_pSavedOffHandCopy != 0) {
        delete m_slot.m_pSavedOffHandCopy;
        m_slot.m_pSavedOffHandCopy = 0;
    }
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWGuiInGameEquip::ShutdownCleanup() {
    m_slot.m_pSavedItemCopy = 0;
    m_slot.m_pSavedOffHandCopy = 0;
}
#endif // !KOTOR1

#if KOTOR1
void CSWGuiInGameEquip::OnChangeCharacter(CSWGuiControl* pControl) {
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    if (!g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bTutorialModule) {
        unsigned char nMember = pControl == &m_btnChange2 ? 2 : 1;
        if (!g_pAppManager->m_pClientExoApp->ChangeCharacterToNextLivingPartyMember(0, 0, nMember))
            return;
        m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
        SetCharacter(g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter());
    } else {
        signed char nNPC = static_cast<signed char>(pControl->m_tail.m_nUserData);
        CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
        CSWCCreature* pCreature;
        if (nNPC == -1)
            pCreature = pClient->GetPlayerCreature();
        else
            pCreature = pClient->GetCreatureByGameObjectID(pClient->ServerToClientObjectId(
                g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(nNPC, 0, 1)));
        if (pCreature != 0) {
            if (!m_btnCharRight.m_tail.bVisible)
                m_btnChange1.m_tail.m_nUserData = static_cast<uint64_t>(-2);
            else if (pControl == &m_btnChange1)
                m_btnChange1.m_tail.m_nUserData = m_btnChange2.m_tail.m_nUserData;
            m_slot.m_nShownNPC = nNPC;
            SetCharacter(pCreature);
        }
    }

    int nGuiSlot = MapGlobalSlotToGuiSlot(m_slot.m_nSlotFlag);
    if (nGuiSlot != GUI_SLOT_NONE)
        SetActiveControl(&m_abtnSlot[nGuiSlot], 0);
    OnEnterSlot(m_pActiveControl);
    UpdatePortraits(1);
}

void CSWGuiInGameEquip::OnSwitchLeft(CSWGuiControl* pControl) {
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    signed char nCur = static_cast<signed char>(m_btnChange1.m_tail.m_nUserData);
    if (nCur == -2)
        return;

    m_btnChange2.m_border.m_params.SetFillImage(m_btnChange1.m_border.m_params.m_fillImage, 0);
    m_btnChange2.m_borderHilight.m_params.SetFillImage(m_btnChange1.m_borderHilight.m_params.m_fillImage, 0);
    m_btnChange2.m_tail.m_nUserData = m_btnChange1.m_tail.m_nUserData;

    CSWCCreature* pCreature;
    int i;
    for (i = nCur - 1; i >= 0; i--) {
        if (i == m_slot.m_nShownNPC)
            continue;
        if (!g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i))
            continue;
        pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(
                g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1)));
        if (pCreature != 0)
            goto found;
    }
    if (m_slot.m_nShownNPC != -1 && static_cast<signed char>(m_btnChange2.m_tail.m_nUserData) != -1) {
        m_btnChange1.m_border.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange1.m_borderHilight.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange1.m_tail.m_nUserData = static_cast<uint64_t>(-1);
        return;
    }
    for (i = 8; i > nCur; i--) {
        if (i == m_slot.m_nShownNPC)
            continue;
        if (!g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i))
            continue;
        pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(
                g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1)));
        if (pCreature != 0)
            goto found;
    }
    return;

found:
    m_btnChange1.m_border.m_params.SetFillImage(pCreature->GetPortrait(1), 0);
    m_btnChange1.m_borderHilight.m_params.SetFillImage(pCreature->GetPortrait(1), 0);
    m_btnChange1.m_tail.m_nUserData = i;
}

void CSWGuiInGameEquip::OnSwitchRight(CSWGuiControl* pControl) {
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    signed char nCur = static_cast<signed char>(m_btnChange2.m_tail.m_nUserData);
    if (nCur == -2)
        return;

    m_btnChange1.m_border.m_params.SetFillImage(m_btnChange2.m_border.m_params.m_fillImage, 0);
    m_btnChange1.m_borderHilight.m_params.SetFillImage(m_btnChange2.m_borderHilight.m_params.m_fillImage, 0);
    m_btnChange1.m_tail.m_nUserData = m_btnChange2.m_tail.m_nUserData;

    CSWCCreature* pCreature;
    int i;
    for (i = nCur + 1; i < 9; i++) {
        if (i == m_slot.m_nShownNPC)
            continue;
        if (!g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i))
            continue;
        pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(
                g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1)));
        if (pCreature != 0)
            goto found;
    }
    if (m_slot.m_nShownNPC != -1) {
        m_btnChange2.m_border.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange2.m_borderHilight.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange2.m_tail.m_nUserData = static_cast<uint64_t>(-1);
        return;
    }
    for (i = 0; i < nCur; i++) {
        if (i == m_slot.m_nShownNPC)
            continue;
        if (!g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i))
            continue;
        pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(
                g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1)));
        if (pCreature != 0)
            goto found;
    }
    return;

found:
    m_btnChange2.m_border.m_params.SetFillImage(pCreature->GetPortrait(1), 0);
    m_btnChange2.m_borderHilight.m_params.SetFillImage(pCreature->GetPortrait(1), 0);
    m_btnChange2.m_tail.m_nUserData = i;
}
#else
void CSWGuiInGameEquip::OnChangeCharacter(CSWGuiControl* pControl) { K2_UNUSED(pControl); }
#endif

void CSWGuiInGameEquip::Draw(float f) {
#if KOTOR1
    // K1: no NPC override and no swap-weapons timer.
    UpdateInventory();
    CSWGuiPanel::Draw(f);
#else
    if (m_slot.m_bNPCOverride == 0)
        SetCharacter(g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter());

    UpdateInventory();

    float fTimer = m_slot.m_fSwapWeaponsCooldown;
    if (fTimer > 0.0f) {
        fTimer = fTimer - f;
        m_slot.m_fSwapWeaponsCooldown = fTimer;
    }
    bool bRowsEnabled = !(fTimer > 0.0f);
    m_btnSwapWeapons.SetEnabled(bRowsEnabled);

    CSWGuiPanel::Draw(f);
#endif
}

#if KOTOR1
void CSWGuiInGameEquip::Update(float f) {
    K2_UNUSED(f);
    if (m_nFocusFrames == 0)
        return;
    m_nFocusFrames--;
    GameMouseMove(1, 0, 0);
    SetActiveControl(0, 0);
    SetActiveControl(&m_abtnSlot[GUI_SLOT_BODY], 0);
    OnEnterSlot(m_pActiveControl);
}
#endif

void CSWGuiInGameEquip::SetActiveSlot(int nSlot) {
#if KOTOR1  // K1: slots 1..8
    if (static_cast<unsigned int>(nSlot - 1) <= 7)
#else
    if (static_cast<unsigned int>(nSlot - 1) <= 9)
#endif
        SetActiveControl(&m_abtnSlot[nSlot], 0);
}

#if KOTOR1
void CSWGuiInGameEquip::UpdatePortraits(int bShow) {
    if (!bShow) {
        m_btnChange1.m_tail.bVisible = 0;
        m_btnChange2.m_tail.bVisible = 0;
        m_btnCharLeft.m_tail.bVisible = 0;
        m_btnCharRight.m_tail.bVisible = 0;
        return;
    }

    if (!g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bTutorialModule) {
        CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
        if (pParty->m_nCharacterCount >= 2) {
            m_btnChange1.m_tail.bVisible = 1;
            m_btnChange1.m_border.m_params.SetFillImage(pParty->GetCharacter(1)->GetPortrait(1), 0);
            m_btnChange1.m_borderHilight.m_params.SetFillImage(pParty->GetCharacter(1)->GetPortrait(1), 0);
        } else {
            m_btnChange1.m_tail.bVisible = 0;
        }
        if (pParty->m_nCharacterCount >= 3) {
            m_btnChange2.m_tail.bVisible = 1;
            m_btnChange2.m_border.m_params.SetFillImage(pParty->GetCharacter(2)->GetPortrait(1), 0);
            m_btnChange2.m_borderHilight.m_params.SetFillImage(pParty->GetCharacter(2)->GetPortrait(1), 0);
        } else {
            m_btnChange2.m_tail.bVisible = 0;
        }
        return;
    }

    signed char nStart;
    if (static_cast<signed char>(m_btnChange1.m_tail.m_nUserData) == -2)
        nStart = m_slot.m_nShownNPC + 1;
    else
        nStart = static_cast<signed char>(m_btnChange1.m_tail.m_nUserData);

    int nShown = 0;
    int i;
    for (i = nStart; i < 9; i++) {
        if (i == m_slot.m_nShownNPC)
            continue;
        if (!g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i))
            continue;
        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(
                g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1)));
        if (pCreature == 0)
            continue;
        if (nShown == 0) {
            nShown = 1;
            m_btnChange1.m_border.m_params.SetFillImage(pCreature->GetPortrait(1), 0);
            m_btnChange1.m_borderHilight.m_params.SetFillImage(pCreature->GetPortrait(1), 0);
            m_btnChange1.m_tail.m_nUserData = i;
        } else {
            m_btnChange2.m_border.m_params.SetFillImage(pCreature->GetPortrait(1), 0);
            m_btnChange2.m_borderHilight.m_params.SetFillImage(pCreature->GetPortrait(1), 0);
            m_btnChange2.m_tail.m_nUserData = i;
            goto both;
        }
    }
    if (m_slot.m_nShownNPC != -1) {
        if (nShown != 0) {
            m_btnChange2.m_border.m_params.SetFillImage(
                g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
            m_btnChange2.m_borderHilight.m_params.SetFillImage(
                g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
            m_btnChange2.m_tail.m_nUserData = static_cast<uint64_t>(-1);
            goto both;
        }
        nShown = 1;
        m_btnChange1.m_border.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange1.m_borderHilight.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange1.m_tail.m_nUserData = static_cast<uint64_t>(-1);
    }
    for (i = 0; i < nStart; i++) {
        if (i == m_slot.m_nShownNPC)
            continue;
        if (!g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i))
            continue;
        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(
                g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1)));
        if (pCreature == 0)
            continue;
        if (nShown == 0) {
            nShown = 1;
            m_btnChange1.m_border.m_params.SetFillImage(pCreature->GetPortrait(1), 0);
            m_btnChange1.m_borderHilight.m_params.SetFillImage(pCreature->GetPortrait(1), 0);
            m_btnChange1.m_tail.m_nUserData = i;
        } else {
            m_btnChange2.m_border.m_params.SetFillImage(pCreature->GetPortrait(1), 0);
            m_btnChange2.m_borderHilight.m_params.SetFillImage(pCreature->GetPortrait(1), 0);
            m_btnChange2.m_tail.m_nUserData = i;
            goto both;
        }
    }
    m_btnChange1.m_tail.bVisible = static_cast<unsigned char>(nShown);
    m_btnChange2.m_tail.bVisible = 0;
    goto arrows;
both:
    m_btnChange1.m_tail.bVisible = static_cast<unsigned char>(nShown);
    m_btnChange2.m_tail.bVisible = 1;
arrows:
    m_btnCharLeft.m_tail.bVisible =
        static_cast<unsigned char>(g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNumNPCAvailable()) > 2;
    m_btnCharRight.m_tail.bVisible =
        static_cast<unsigned char>(g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNumNPCAvailable()) > 2;
}
#else
void CSWGuiInGameEquip::UpdatePortraits(int bShow) {
    bool bShowButtons = (bShow != 0);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowGameMenuChangeButtons(bShowButtons);
}
#endif

void CSWGuiInGameEquip::CloseDescription() {
    ShowDescription(0);

    m_slot.m_bFlags = m_slot.m_bFlags & ~SLOTSTATE_SELECTING_ITEM;
    SetActiveControl(m_slot.m_pActiveSlotButton, 0);
    OnEnterSlot(m_pActiveControl);
#if KOTOR1  // K1: also blanks LBL_SELECTTITLE
    m_lblSelectTitle.m_text.m_params.SetText(CExoString(""));
#endif
    m_lbItems.SetEnabled(0);
}

void CSWGuiInGameEquip::OnOKPressed(CSWGuiControl* pControl) {
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    if ((m_slot.m_bFlags & SLOTSTATE_SELECTING_ITEM) == 0)
        return;

    m_slot.m_oidSavedItem = 0;
    m_slot.m_oidSavedOffHandItem = 0;

    if (m_slot.m_pSavedItemCopy != 0) {
        delete m_slot.m_pSavedItemCopy;
        m_slot.m_pSavedItemCopy = 0;
    }
    if (m_slot.m_pSavedOffHandCopy != 0) {
        delete m_slot.m_pSavedOffHandCopy;
        m_slot.m_pSavedOffHandCopy = 0;
    }

#if KOTOR1  // K1: CloseDescription stays a call
    CloseDescription();
#else
    ShowDescription(0);

    m_slot.m_bFlags = m_slot.m_bFlags & ~SLOTSTATE_SELECTING_ITEM;
    SetActiveControl(m_slot.m_pActiveSlotButton, 0);
    OnEnterSlot(m_pActiveControl);
    m_lbItems.SetEnabled(0);
#endif
}

void CSWGuiInGameEquip::OnSelectSlot(CSWGuiControl* pControl) {
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    if (m_pCreature->m_pArea->GetServerArea()->m_nPlayerRestrictMode != 0) {
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetCallback(0, 0, 0);
        pBox->SetMessage(STRREF_CANNOT_EQUIP_IN_ENVIRO_SUIT);
        m_pGuiManager->AddPanel(pBox, 1, 1);
        return;
        return;
    }

#if !KOTOR1  // K1: no slot locks
    int nSlot = MapGlobalSlotToGuiSlot(m_slot.m_nSlotFlag);
    if (nSlot != GUI_SLOT_NONE && m_abSlotLocked[nSlot] != 0) {
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetCallback(0, 0, 0);
        pBox->SetMessage(STRREF_SLOT_LOCKED);
        m_pGuiManager->AddPanel(pBox, 1, 1);
        return;
    }
#endif

    if (m_slot.m_nSlotFlag == CSWInventory::SLOT_BODY
        && m_pCreature->GetServerCreature()->m_bCombatState == 1
        && m_pCreature->GetServerCreature()->m_nCombatType == 1) {
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetCallback(0, 0, 0);
        pBox->SetMessage(STRREF_CANNOT_CHANGE_ARMOR_IN_COMBAT);
        m_pGuiManager->AddPanel(pBox, 1, 1);
        return;
        return;
    }

    if (m_slot.m_nSlotFlag == CSWInventory::SLOT_LEFTWEAPON && m_slot.m_aoidEquipped[GUI_SLOT_WEAP_R] != OBJECT_INVALID) {
        CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
            g_pAppManager->m_pServerExoApp->ClientToServerObjectId(m_slot.m_aoidEquipped[GUI_SLOT_WEAP_R]));
        if (pItem != 0 && pItem->GetBaseItem()->m_nWeaponSize == CSWBaseItem::WEAPON_SIZE_LARGE) {
            CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
            pBox->SetAllowCancel(0);
            pBox->SetCallback(0, 0, 0);
            pBox->SetMessage(STRREF_SLOT_BLOCKED_BY_TWO_HANDED);
            m_pGuiManager->AddPanel(pBox, 1, 1);
            return;
            return;
        }
    }

    if (m_lbItems.m_aControls.m_count == 1) {
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetCallback(0, 0, 0);
        pBox->SetMessage(STRREF_NO_ITEMS_FOR_SLOT);
        m_pGuiManager->AddPanel(pBox, 1, 1);
        return;
        return;
    }

    m_slot.m_pActiveSlotButton = pControl;
    ShowDescription(1);
    SetActiveControl(&m_lbItems, 0);
    if (m_slot.m_aoidEquipped[pControl->m_tail.m_nUserData] != OBJECT_INVALID)
        m_lbItems.SetSelectedControl(1, 0);
    m_slot.m_bFlags |= SLOTSTATE_SELECTING_ITEM;
#if KOTOR1  // K1: retitle LBL_SELECTTITLE
    m_lblSelectTitle.m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SELECT_ITEM_TO_EQUIP));
#endif
    g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_PRESS_A_ON_EQUIP_SCREEN, OBJECT_INVALID, OBJECT_INVALID, 0);
    m_lbItems.SetEnabled(1);
}

void CSWGuiInGameEquip::OnEnterSlot(CSWGuiControl* pControl) {
    if (m_pCreature == 0)
        return;

    int nSlot = static_cast<int>(pControl->m_tail.m_nUserData);
    m_slot.m_nSlotFlag = SLOT_INFO[nSlot].m_nSlotFlag;

    CSWSCreature* pServerCreature = m_pCreature->GetServerCreature();
    CItemRepository* pRepo = pServerCreature->GetItemRepository(1);

    unsigned int nFailedSlot;
    CExoArrayList<CSWGuiControl*> aControls;
    int bChanged = 0;
    int nIndex = 1;

    CSWGuiInGameItemEntry* pEntry = AddItemEntryToList(0);
    pEntry->SetItem(OBJECT_INVALID, 0, 0);
    pEntry->SetCanUse(CSWGuiInGameItemEntry::CAN_USE_YES);
    pEntry->m_tail.m_nClickSound = GUISOUND_NONE;
    aControls.Add(pEntry);

    if (m_slot.m_aoidEquipped[nSlot] != OBJECT_INVALID) {
        pEntry = AddItemEntryToList(1);
        if (pEntry->m_oidItem == m_slot.m_aoidEquipped[nSlot] && pEntry->bEquipped != 0
            && pEntry->m_nCanUse == CSWGuiInGameItemEntry::CAN_USE_EQUIPPED)
            bChanged = 0;
        else
            bChanged = 1;
        pEntry->SetItem(m_slot.m_aoidEquipped[nSlot], 1, 0);
        pEntry->SetCanUse(CSWGuiInGameItemEntry::CAN_USE_EQUIPPED);
        pEntry->m_tail.m_nClickSound = GUISOUND_NONE;
        aControls.Add(pEntry);
        nIndex = 2;
    }

    int bScanItems;
    CSWSItem* pItem;
    if (m_slot.m_nSlotFlag == CSWInventory::SLOT_LEFTWEAPON && m_slot.m_aoidEquipped[GUI_SLOT_WEAP_R] != OBJECT_INVALID) {
        pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
            g_pAppManager->m_pServerExoApp->ClientToServerObjectId(m_slot.m_aoidEquipped[GUI_SLOT_WEAP_R]));
        bScanItems = pItem == 0 || pItem->GetBaseItem()->m_nWeaponSize != CSWBaseItem::WEAPON_SIZE_LARGE;
    } else {
        bScanItems = 1;
    }

#if !KOTOR1  // K1: no second weapon set to fold onto the first
    int nSavedSlotFlag = m_slot.m_nSlotFlag;
    if (m_slot.m_nSlotFlag == CSWInventory::SLOT_RIGHTWEAPON2)
        m_slot.m_nSlotFlag = CSWInventory::SLOT_RIGHTWEAPON2 | CSWInventory::SLOT_RIGHTWEAPON;
    else if (m_slot.m_nSlotFlag == CSWInventory::SLOT_LEFTWEAPON2)
        m_slot.m_nSlotFlag = CSWInventory::SLOT_LEFTWEAPON2 | CSWInventory::SLOT_LEFTWEAPON;
#endif

    if (bScanItems != 0 && pRepo->ItemListGetCount() != 0) {
        for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++) {
            pItem = pRepo->ItemListGetItem(i);
            if (pItem == 0)
                continue;
            if ((pItem->GetBaseItem()->m_nEquipableSlots & m_slot.m_nSlotFlag) == 0)
                continue;
            unsigned char nRaceRestrict = pItem->GetBaseItem()->m_nRaceRestriction;
            if (nRaceRestrict == 1 || nRaceRestrict == 2) {
                if (pServerCreature->m_pStats->m_nRace
                    != (nRaceRestrict == 1 ? CSWRace::CRACE_HUMAN : CSWRace::CRACE_DROID))
                    continue;
            }

            nFailedSlot = m_slot.m_nSlotFlag;
            unsigned char bCanEquip = pServerCreature->CanEquipItem(pItem, &nFailedSlot, 0, 0, 0);
            CClientOptions* pOptions;
            int bProficient;
            CSWCItem* pClientItem;
            pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
            if (pOptions != 0 && pOptions->m_bFeedback0) {
                bProficient = pServerCreature->CheckProficiencies(pItem, m_slot.m_nSlotFlag);
                nFailedSlot = m_slot.m_nSlotFlag;
                if (pServerCreature->CanEquipItem(pItem, &nFailedSlot, 0, 0, 0) == 0)
                    continue;
                if (bProficient == 0)
                    continue;
            }

            pClientItem = pItem->GetClientItem();
            if (pClientItem == 0)
                continue;

            unsigned char nCanUse = bCanEquip == 0;
            nCanUse *= 2;

            // Both arms compare the same two items; only which one is the client item differs.
            if (m_slot.m_nSlotFlag == CSWInventory::SLOT_RIGHTWEAPON || m_slot.m_nSlotFlag == CSWInventory::SLOT_LEFTWEAPON) {
                CSWItem* pCompare;
                CSWItem* pOther;
                if (m_slot.m_nSlotFlag == CSWInventory::SLOT_RIGHTWEAPON) {
                    pCompare = pClientItem;
                    pOther = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
                        m_pCreature->GetEquippedItemID(CSWInventory::SLOT_LEFTWEAPON));
                } else {
                    pCompare = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
                        m_pCreature->GetEquippedItemID(CSWInventory::SLOT_RIGHTWEAPON));
                    pOther = pClientItem;
                }
                if (pCompare != 0 && pOther != 0
                    && (pCompare != pClientItem
                        || pItem->GetBaseItem()->m_nWeaponSize != CSWBaseItem::WEAPON_SIZE_LARGE)) {
                    unsigned char nWield = pCompare->GetBaseItem()->m_weaponWieldType;
                    unsigned char nOtherWield = pOther->GetBaseItem()->m_weaponWieldType;
                    if (nWield != nOtherWield
                        || (nWield != CSWBaseItem::WEAPON_WIELD_ONE_HANDED && nWield != CSWBaseItem::WEAPON_WIELD_PISTOL))
                        nCanUse = CSWGuiInGameItemEntry::CAN_USE_INCOMPATIBLE_OFF_HAND;
                }
            }

            // The second weapon set, checked against the other set's hand the same way.
#if !KOTOR1
            if ((m_slot.m_nSlotFlag & (CSWInventory::SLOT_RIGHTWEAPON2 | CSWInventory::SLOT_LEFTWEAPON2)) != 0) {
                CSWItem* pCompare;
                CSWItem* pOther;
                if ((m_slot.m_nSlotFlag & CSWInventory::SLOT_RIGHTWEAPON2) != 0) {
                    pCompare = pClientItem;
                    pOther = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
                        m_pCreature->GetEquippedItemID(CSWInventory::SLOT_LEFTWEAPON2));
                } else {
                    pCompare = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
                        m_pCreature->GetEquippedItemID(CSWInventory::SLOT_RIGHTWEAPON2));
                    pOther = pClientItem;
                }
                if (pCompare != 0 && pOther != 0
                    && (pCompare != pClientItem
                        || pItem->GetBaseItem()->m_nWeaponSize != CSWBaseItem::WEAPON_SIZE_LARGE)) {
                    unsigned char nWield = pCompare->GetBaseItem()->m_weaponWieldType;
                    unsigned char nOtherWield = pOther->GetBaseItem()->m_weaponWieldType;
                    if (nWield != nOtherWield
                        || (nWield != CSWBaseItem::WEAPON_WIELD_ONE_HANDED && nWield != CSWBaseItem::WEAPON_WIELD_PISTOL))
                        nCanUse = CSWGuiInGameItemEntry::CAN_USE_INCOMPATIBLE_OFF_HAND;
                }
            }
#endif

            K2_ASSUME(pClientItem != 0);   // a null item was skipped at the top of the loop
            pEntry = AddItemEntryToList(nIndex);
            if (pEntry->m_oidItem != pClientItem->m_objectId || pEntry->bEquipped != 0
                || pEntry->m_nCanUse != nCanUse)
                bChanged = 1;
            pEntry->SetItem(pClientItem->m_objectId, 0, 0);
            pEntry->SetCanUse(nCanUse);
            pEntry->m_tail.m_nClickSound = GUISOUND_NONE;
            aControls.Add(pEntry);
            nIndex++;
        }
    }

#if !KOTOR1
    m_slot.m_nSlotFlag = nSavedSlotFlag;
#endif
    if (bChanged != 0 || aControls.m_count != m_lbItems.m_aControls.m_count) {
        m_lbItems.AddControls(aControls, 0, 1, 0);
        m_lbItems.m_scrollBar.SetCurValue(0);
    }

    m_lblSlotName.m_text.m_params.SetText(
        g_pAppManager->m_pClientExoApp->GetGUIString(
            m_pCreature->m_pStats->m_nRace == CSWRace::CRACE_DROID
                ? SLOT_INFO[nSlot].m_nDroidStrRef
                : SLOT_INFO[nSlot].m_nStrRef));
}

#if KOTOR1
void CSWGuiInGameEquip::ShowDescription(int n) {
    m_lblAtkL.m_tail.bVisible = 1;
    m_lblAtkR.m_tail.bVisible = 1;
    m_lblDef.m_tail.bVisible = 1;
    m_lblToHitL.m_tail.bVisible = 1;
    m_lblToHitR.m_tail.bVisible = 1;
    m_lblVitality.m_tail.bVisible = 1;
    m_lblAttackInfo.m_tail.bVisible = 1;
    m_lblDefInfo.m_tail.bVisible = 1;
    m_lblToHit.m_tail.bVisible = 1;
    m_lblDamage.m_tail.bVisible = 1;
    m_lblCantEquip.m_tail.bVisible = 0;

    m_btnEquip.SetEnabled(1);
    m_btnEquip.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);

    UpdatePortraits(n == 0);
    if (n != 0) {
        m_lbDesc.m_tail.bVisible = 1;
        m_btnBack.m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_CANCEL));

        for (int i = 0; i < 9; i++) {
            m_abtnSlot[i].m_tail.bVisible = 0;
            m_apSlotLabels[i].m_tail.bVisible = 0;
        }

        m_lblTxtBar.m_tail.bVisible = 0;
        m_lblPortBord.m_tail.bVisible = 0;
        m_lblPortrait.m_tail.bVisible = 0;
        m_lblSlotName.m_tail.bVisible = 0;
        m_btnEquip.m_tail.bVisible = 1;

        for (int i = 0; i < m_lbItems.m_aControls.m_count; i++) {
            CSWGuiButton* pRow = m_lbItems.GetControl(i)->AsButton();
            pRow->m_border.m_params.m_fAlpha = 1.0f;
        }
    } else {
        m_lbDesc.m_tail.bVisible = 0;
        m_btnBack.m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_CLOSE));

        for (int i = 0; i < 9; i++) {
            m_abtnSlot[i].m_tail.bVisible = 1;
            m_apSlotLabels[i].m_tail.bVisible = 1;
        }

        m_lblTxtBar.m_tail.bVisible = 1;
        m_lblPortBord.m_tail.bVisible = 1;
        m_lblPortrait.m_tail.bVisible = 1;
        m_lblSlotName.m_tail.bVisible = 1;
        m_btnEquip.m_tail.bVisible = 0;

        for (int i = 0; i < m_lbItems.m_aControls.m_count; i++) {
            CSWGuiButton* pRow = m_lbItems.GetControl(i)->AsButton();
            pRow->m_border.m_params.m_fAlpha = 0.0f;
        }
    }
    IosAddButtonIcon(&m_btnBack, 'b');
}
#else
void CSWGuiInGameEquip::ShowDescription(int n) {
    m_lblCantEquip.m_tail.bVisible = 0;

    m_btnEquip.SetEnabled(1);
    m_btnEquip.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnEquip.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);

    if (n != 0) {
        UpdatePortraits(0);
        m_lbDesc.m_tail.bVisible = 1;

        m_btnBack.m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_CANCEL));
        AspyrAddButtonIcon(&m_btnBack, 'b');

        for (int i = 0; i < 11; i++) {
            m_abtnSlot[i].m_tail.bVisible = 0;
            m_apSlotLabels[i].m_tail.bVisible = 0;
        }

        m_lblSlotName.m_tail.bVisible = 0;
        m_lblAtkL.m_tail.bVisible = 0;
        m_lblAtkR.m_tail.bVisible = 0;
        m_lblAtkL2.m_tail.bVisible = 0;
        m_lblAtkR2.m_tail.bVisible = 0;
        m_lblToHitL.m_tail.bVisible = 0;
        m_lblToHitR.m_tail.bVisible = 0;
        m_lblToHitL2.m_tail.bVisible = 0;
        m_lblToHitR2.m_tail.bVisible = 0;
        m_lblToHit.m_tail.bVisible = 0;
        m_lblDamage.m_tail.bVisible = 0;
        m_lblBack1.m_tail.bVisible = 0;
        m_lblAttackMod.m_tail.bVisible = 0;
        m_lblDamText.m_tail.bVisible = 0;
        m_lblDef.m_tail.bVisible = 0;
        m_lblDefText.m_tail.bVisible = 0;
        m_lblDefBack.m_tail.bVisible = 0;

        if (m_slot.m_bNPCOverride != 0) {
            m_btnPrevNPC.m_tail.bVisible = 0;
            m_btnNextNPC.m_tail.bVisible = 0;
        } else {
            m_btnSwapWeapons.m_tail.bVisible = 0;
        }

        m_btnEquip.m_tail.bVisible = 1;

        for (int i = 0; i < m_lbItems.m_aControls.m_count; i++) {
            CSWGuiButton* pRow = m_lbItems.GetControl(i)->AsButton();
            pRow->m_border.m_params.m_fAlpha = 1.0f;
        }
    } else {
        UpdatePortraits(1);
        m_lbDesc.m_tail.bVisible = 0;

        m_btnBack.m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_CLOSE));
        AspyrAddButtonIcon(&m_btnBack, 'b');

        for (int i = 0; i < 11; i++) {
            m_abtnSlot[i].m_tail.bVisible = 1;
            m_apSlotLabels[i].m_tail.bVisible = 1;
        }

        m_lblSlotName.m_tail.bVisible = 1;
        m_lblAtkL.m_tail.bVisible = 1;
        m_lblAtkR.m_tail.bVisible = 1;
        m_lblAtkL2.m_tail.bVisible = 1;
        m_lblAtkR2.m_tail.bVisible = 1;
        m_lblToHitL.m_tail.bVisible = 1;
        m_lblToHitR.m_tail.bVisible = 1;
        m_lblToHitL2.m_tail.bVisible = 1;
        m_lblToHitR2.m_tail.bVisible = 1;
        m_lblToHit.m_tail.bVisible = 1;
        m_lblDamage.m_tail.bVisible = 1;
        m_lblBack1.m_tail.bVisible = 1;
        m_lblAttackMod.m_tail.bVisible = 1;
        m_lblDamText.m_tail.bVisible = 1;
        m_lblDef.m_tail.bVisible = 1;
        m_lblDefText.m_tail.bVisible = 1;
        m_lblDefBack.m_tail.bVisible = 1;

        if (m_slot.m_bNPCOverride != 0) {
            m_btnPrevNPC.m_tail.bVisible = 1;
            m_btnNextNPC.m_tail.bVisible = 1;
        } else {
            m_btnSwapWeapons.m_tail.bVisible = 1;
        }

        m_btnEquip.m_tail.bVisible = 0;

        for (int i = 0; i < m_lbItems.m_aControls.m_count; i++) {
            CSWGuiButton* pRow = m_lbItems.GetControl(i)->AsButton();
            pRow->m_border.m_params.m_fAlpha = 0.0f;
        }
    }
}
#endif

int CSWGuiInGameEquip::MapGlobalSlotToGuiSlot(int nGlobalSlot) {
    int nGuiSlot = 0;
    switch (nGlobalSlot) {
    case CSWInventory::SLOT_LEFTWEAPON: nGuiSlot = GUI_SLOT_WEAP_L;  break;
    case CSWInventory::SLOT_RIGHTWEAPON: nGuiSlot = GUI_SLOT_WEAP_R;  break;
    case CSWInventory::SLOT_HEAD: nGuiSlot = GUI_SLOT_HEAD;  break;
    case CSWInventory::SLOT_LEFTARM: nGuiSlot = GUI_SLOT_ARM_L;  break;
    case CSWInventory::SLOT_RIGHTARM: nGuiSlot = GUI_SLOT_ARM_R;  break;
    case CSWInventory::SLOT_BODY: nGuiSlot = GUI_SLOT_BODY;  break;
    case CSWInventory::SLOT_HANDS: nGuiSlot = GUI_SLOT_HANDS;  break;
    case CSWInventory::SLOT_IMPLANT: nGuiSlot = GUI_SLOT_IMPLANT;  break;
    case CSWInventory::SLOT_BELT: nGuiSlot = GUI_SLOT_BELT;  break;
#if KOTOR1  // K1: no second weapon set; unknown flags map to 10
    default:       nGuiSlot = GUI_SLOT_NONE; break;
#else
    case CSWInventory::SLOT_LEFTWEAPON2: nGuiSlot = GUI_SLOT_WEAP_L2;  break;
    case CSWInventory::SLOT_RIGHTWEAPON2: nGuiSlot = GUI_SLOT_WEAP_R2; break;
    default:       nGuiSlot = GUI_SLOT_NONE; break;
#endif
    }
    return nGuiSlot;
}

#if KOTOR1
void CSWGuiInGameEquip::SetDescription(const CExoString& sDescription) {
    CSWGuiControl* pRow;

    m_lblDescription.m_text.m_params.SetText(sDescription);
    m_lbDesc.ClearItems();

    CSWGuiText* pText = &m_lblDescription.m_text;
    CSWGuiExtent extent = m_lblDescription.m_extent;
    if (sDescription.GetLength() >= 1)
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
    m_lblDescription.SetExtent(extent);

    pRow = &m_lblDescription;
    m_lbDesc.AddControls(&pRow, 1, 0, 0, 0);
    m_lbDesc.SetSelectedControl(&m_lblDescription, 0);
}
#else
void CSWGuiInGameEquip::SetDescription(const CExoString& sDescription) {
    CExoString sDesc(sDescription);

    int nLt = sDesc.Find('<', 0);
    int nGt = sDesc.Find('>', 0);
    if ((nLt & nGt) != -1) {
        CSWCCreature* pPC = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
        g_pSWTlkTable->ParseStr(sDesc, pPC, 1, 1);
    }

    m_lblDescription.m_text.m_params.SetText(sDesc);
    m_lbDesc.ClearItems();

    CSWGuiText* pText = &m_lblDescription.m_text;
#ifdef K2_QOL_IMPROVEMENTS
    K2QolFitText(*pText, 0x7fffffff);   // LB_DESC scrolls its one row: no limit
#endif

    CSWGuiExtent extent = m_lblDescription.m_extent;
    if (sDescription.GetLength() >= 1)
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
#ifndef K2_FIX_ENGINE_BUGS
    float fScale = GetResolutionTextScalar();
    extent.h = static_cast<int>(extent.h * fScale);
#endif
    m_lblDescription.SetExtent(extent);

    CSWGuiControl* apRows[1] = { &m_lblDescription };
    m_lbDesc.AddControls(apRows, 1, 0, 0, 0);
    m_lbDesc.SetSelectedControl(&m_lblDescription, 0);
}
#endif

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWGuiInGameEquip::GetSlotLockedBySLOTID(int nSlotFlag) {
    switch (nSlotFlag) {
    case CSWInventory::SLOT_HEAD: return m_abSlotLocked[GUI_SLOT_HEAD];
    case CSWInventory::SLOT_BODY: return m_abSlotLocked[GUI_SLOT_BODY];
    case CSWInventory::SLOT_HANDS: return m_abSlotLocked[GUI_SLOT_HANDS];
    case CSWInventory::SLOT_RIGHTWEAPON: return m_abSlotLocked[GUI_SLOT_WEAP_R];
    case CSWInventory::SLOT_LEFTWEAPON: return m_abSlotLocked[GUI_SLOT_WEAP_L];
    case CSWInventory::SLOT_LEFTARM: return m_abSlotLocked[GUI_SLOT_ARM_L];
    case CSWInventory::SLOT_RIGHTARM: return m_abSlotLocked[GUI_SLOT_ARM_R];
    case CSWInventory::SLOT_IMPLANT: return m_abSlotLocked[GUI_SLOT_IMPLANT];
    case CSWInventory::SLOT_BELT: return m_abSlotLocked[GUI_SLOT_BELT];
    case CSWInventory::SLOT_RIGHTWEAPON2: return m_abSlotLocked[GUI_SLOT_WEAP_R2];
    case CSWInventory::SLOT_LEFTWEAPON2: return m_abSlotLocked[GUI_SLOT_WEAP_L2];
    default:       return 0;
    }
}
#endif // !KOTOR1

#if KOTOR1
void CSWGuiInGameEquip::SetCharacter(CSWCCreature* pCreature) {
    if (m_pCreature != pCreature) {
        m_pCreature = pCreature;
        if (pCreature != 0) {
            for (int i = 0; i < 9; i++)
                m_apSlotLabels[i].m_border.m_params.SetFillImage(GetIcon(i), 0);
        } else {
            m_lblPortrait.m_border.m_params.SetFillImage(CResRef(""), 0);
        }
    }
    if (m_pCreature != 0)
        m_lblPortrait.m_border.m_params.SetFillImage(m_pCreature->GetPortrait(1), 0);
}
#else
void CSWGuiInGameEquip::SetCharacter(CSWCCreature* pCreature) {
    if (m_pCreature == pCreature)
        return;
    m_pCreature = pCreature;

    if (pCreature != 0) {
        for (int i = 0; i < 11; i++) {
            CExoString sIcon;
            sIcon.Format(const_cast<char*>("i%s%s"),
                         m_pCreature->m_pStats->m_nRace == CSWRace::CRACE_DROID ? "d" : "",
                         SLOT_INFO[i].m_pModelName);
            CResRef resIcon(sIcon);
            m_apSlotLabels[i].m_border.m_params.SetFillImage(resIcon, 0);
        }
    }

    UpdateSlotLocks();
}
#endif

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWGuiInGameEquip::UpdateSlotLocks() {
    if (m_pCreature == 0)
        return;

    int nRow = m_pCreature->m_pCreatureAppearance->m_nAppearanceId;

    int nFlags = 0;
    if (g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(nRow, CExoString("EquipSlotsLocked"),
                                                         &nFlags) == 0) {
        nFlags = 0;
    }

    m_abSlotLocked[GUI_SLOT_WEAP_L] = nFlags & SLOT_INFO[GUI_SLOT_WEAP_L].m_nSlotFlag;
    m_abSlotLocked[GUI_SLOT_WEAP_R] = nFlags & SLOT_INFO[GUI_SLOT_WEAP_R].m_nSlotFlag;
    m_abSlotLocked[GUI_SLOT_HEAD] = nFlags & SLOT_INFO[GUI_SLOT_HEAD].m_nSlotFlag;
    m_abSlotLocked[GUI_SLOT_ARM_L] = nFlags & SLOT_INFO[GUI_SLOT_ARM_L].m_nSlotFlag;
    m_abSlotLocked[GUI_SLOT_ARM_R] = nFlags & SLOT_INFO[GUI_SLOT_ARM_R].m_nSlotFlag;
    m_abSlotLocked[GUI_SLOT_BODY] = nFlags & SLOT_INFO[GUI_SLOT_BODY].m_nSlotFlag;
    m_abSlotLocked[GUI_SLOT_HANDS] = nFlags & SLOT_INFO[GUI_SLOT_HANDS].m_nSlotFlag;
    m_abSlotLocked[GUI_SLOT_IMPLANT] = nFlags & SLOT_INFO[GUI_SLOT_IMPLANT].m_nSlotFlag;
    m_abSlotLocked[GUI_SLOT_BELT] = nFlags & SLOT_INFO[GUI_SLOT_BELT].m_nSlotFlag;
    m_abSlotLocked[GUI_SLOT_WEAP_L2] = nFlags & SLOT_INFO[GUI_SLOT_WEAP_L2].m_nSlotFlag;
    m_abSlotLocked[GUI_SLOT_WEAP_R2] = nFlags & SLOT_INFO[GUI_SLOT_WEAP_R2].m_nSlotFlag;

    if (m_abSlotLocked[GUI_SLOT_WEAP_L] != 0)
        m_abSlotLocked[GUI_SLOT_WEAP_L2] = 1;
    if (m_abSlotLocked[GUI_SLOT_WEAP_R] != 0)
        m_abSlotLocked[GUI_SLOT_WEAP_R2] = 1;

    for (int i = 0; i < 11; i++) {
        if (m_abSlotLocked[i] != 0)
            m_abtnSlot[i].m_border.m_params.m_color =
                *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_RED);
        else
            m_abtnSlot[i].m_border.m_params.m_color =
                *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
    }
}
#endif // !KOTOR1

CExoString CSWGuiInGameEquip::GetIcon(int nSlot) {
    CExoString sIcon;
    sIcon.Format(const_cast<char*>("i%s%s"),
                 m_pCreature->m_pStats->m_nRace == CSWRace::CRACE_DROID ? "d" : "",
                 SLOT_INFO[nSlot].m_pModelName);
    return sIcon;
}

bool CSWGuiInGameEquip::ItemExists(unsigned long long oidItem) {
    CGameObject* pObject;
    unsigned long long oidServer =
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(oidItem);
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidServer, &pObject)
            != CGameObjectArray::SUCCESS)
        return false;
    return pObject->AsSWSItem() != 0;
}

CSWSItem* CSWGuiInGameEquip::CopyItem(unsigned long long oidItem) {
    CGameObject* pObject = 0;
    unsigned long long oidServer =
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(oidItem);
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidServer, &pObject)
            != CGameObjectArray::SUCCESS)
        return 0;

    CSWSItem* pCopy = 0;
    if (pObject->AsSWSItem() != 0) {
        pCopy = new CSWSItem(OBJECT_INVALID);
        pCopy->CopyItem(pObject->AsSWSItem());
    }
    return pCopy;
}

void CSWGuiInGameEquip::OnItemSelected(CSWGuiControl* pControl) {
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    if (!m_lbDesc.m_tail.bVisible) {
        pControl->SetActive(0);
        return;
    }

    CSWGuiInGameItemEntry* pEntry = static_cast<CSWGuiInGameItemEntry*>(pControl);
    unsigned long long oidItem = pEntry->m_oidItem;
    unsigned long long oidEquipped = m_pCreature->GetEquippedItemID(m_slot.m_nSlotFlag);

    if (oidItem != OBJECT_INVALID) {
        int nServerId = static_cast<int>(g_pAppManager->m_pServerExoApp->ClientToServerObjectId(oidItem));
        CSWSItem* pRowItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(nServerId);
        if (pRowItem != 0) {
            CExoString sDescription;
            sDescription = pRowItem->GetPropertyDescription();
            if (sDescription != "")
                SetDescription(sDescription);
            else
                SetDescription(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NO_DESCRIPTION_SET));
        } else if (oidItem == static_cast<unsigned long long>(m_slot.m_oidSavedItem) && m_slot.m_pSavedItemCopy != 0
                   && m_slot.m_pSavedItemCopy->GetPropertyDescription() != "") {
            SetDescription(m_slot.m_pSavedItemCopy->GetPropertyDescription());
        } else {
            SetDescription(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NO_DESCRIPTION_SET));
        }
    } else {
        SetDescription(CExoString(""));
    }

    if (!m_lbItems.m_tail.bEnabled)
        return;
#if !KOTOR1  // K1: no null test on the shown creature
    if (m_pCreature == 0)
        return;
#endif
    if (m_pCreature->GetServerCreature() == 0)
        return;

#if defined(K2_UI_XBOX) || defined(K1_UI_XBOX) || (defined(K2_UI_TOUCH) && defined(K2_FIX_ENGINE_BUGS))
    // A pad focus or a mouse hover only describes the row, silently and without the stat readout,
    // as the Switch does: every step otherwise really equipped the row, which scripts see, and
    // Handmaiden's duel disqualifies on it. The equip runs when the row's A or click arrives.
    if (pControl->m_tail.nCurrentEventType != INPUTEVENT_GUI_A)
        return;
#endif
    m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);

    // A weapon slot whose partner slot is locked forces the row to "cannot equip".
    unsigned int nCanUse = pEntry->m_nCanUse;
#if !KOTOR1  // K1: no slot locks
    if ((m_slot.m_nSlotFlag == CSWInventory::SLOT_RIGHTWEAPON && m_abSlotLocked[GUI_SLOT_WEAP_L] != 0)
        || (m_slot.m_nSlotFlag == CSWInventory::SLOT_RIGHTWEAPON2 && m_abSlotLocked[GUI_SLOT_WEAP_L2] != 0)) {
        CSWSItem* pWeapon = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
            g_pAppManager->m_pServerExoApp->ClientToServerObjectId(oidItem));
        if (pWeapon != 0 && pWeapon->GetBaseItem()->m_nWeaponSize == CSWBaseItem::WEAPON_SIZE_LARGE)
            nCanUse = CSWGuiInGameItemEntry::CAN_USE_TWO_HANDED_LOCKED;
    }
#endif

    switch (nCanUse) {
    case CSWGuiInGameItemEntry::CAN_USE_YES:
    case CSWGuiInGameItemEntry::CAN_USE_EQUIPPED:
        break;
    case CSWGuiInGameItemEntry::CAN_USE_MISSING_PREREQUISITES:
    case CSWGuiInGameItemEntry::CAN_USE_INCOMPATIBLE_OFF_HAND:
        ShowCantEquipMessage(1, nCanUse == CSWGuiInGameItemEntry::CAN_USE_MISSING_PREREQUISITES ? STRREF_MISSING_PREREQUISITES : STRREF_INCOMPATIBLE_OFF_HAND);
        return;
#if !KOTOR1
    case CSWGuiInGameItemEntry::CAN_USE_TWO_HANDED_LOCKED:
        ShowCantEquipMessage(1, STRREF_LEFT_HAND_UNAVAILABLE_FOR_TWO_HANDED);
        return;
#endif
    default:
        return;
    }

#if KOTOR1  // K1: the stat readout comes back, COLOR_BLUE caption, no border tint
    m_lblAtkL.m_tail.bVisible = 1;
    m_lblAtkR.m_tail.bVisible = 1;
    m_lblDef.m_tail.bVisible = 1;
    m_lblToHitL.m_tail.bVisible = 1;
    m_lblToHitR.m_tail.bVisible = 1;
    m_lblVitality.m_tail.bVisible = 1;
    m_lblAttackInfo.m_tail.bVisible = 1;
    m_lblDefInfo.m_tail.bVisible = 1;
    m_lblToHit.m_tail.bVisible = 1;
    m_lblDamage.m_tail.bVisible = 1;
    m_lblCantEquip.m_tail.bVisible = 0;
    m_btnEquip.SetEnabled(1);
    m_btnEquip.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
#else
    // For a usable row, clear the cannot-equip banner, arm the equip button and repaint its two
    // colour slots.
    m_lblCantEquip.m_tail.bVisible = 0;
    m_btnEquip.SetEnabled(1);
    m_btnEquip.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnEquip.m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
#endif

    if (oidItem == oidEquipped)
        return;

    // The displayed item is whichever oid the description cache holds; refresh the copy when the
    // cache is empty.
    if (m_slot.m_oidSavedItem == 0) {
        int nSlot = GUI_SLOT_WEAP_L;
        switch (m_slot.m_nSlotFlag) {
        case CSWInventory::SLOT_LEFTWEAPON: break;
        case CSWInventory::SLOT_RIGHTWEAPON: nSlot = GUI_SLOT_WEAP_R; break;
        case CSWInventory::SLOT_HEAD: nSlot = GUI_SLOT_HEAD; break;
        case CSWInventory::SLOT_LEFTARM: nSlot = GUI_SLOT_ARM_L; break;
        case CSWInventory::SLOT_RIGHTARM: nSlot = GUI_SLOT_ARM_R; break;
        case CSWInventory::SLOT_BODY: nSlot = GUI_SLOT_BODY; break;
        case CSWInventory::SLOT_HANDS: nSlot = GUI_SLOT_HANDS; break;
        case CSWInventory::SLOT_IMPLANT: nSlot = GUI_SLOT_IMPLANT; break;
        case CSWInventory::SLOT_BELT: nSlot = GUI_SLOT_BELT; break;
#if KOTOR1
        default: nSlot = GUI_SLOT_NONE; break;
#else
        case CSWInventory::SLOT_LEFTWEAPON2: nSlot = GUI_SLOT_WEAP_L2; break;
        case CSWInventory::SLOT_RIGHTWEAPON2: nSlot = GUI_SLOT_WEAP_R2; break;
        default: nSlot = GUI_SLOT_NONE; break;
#endif
        }
#ifdef K2_FIX_ENGINE_BUGS
        if (nSlot >= static_cast<int>(sizeof(m_slot.m_aoidEquipped) / sizeof(m_slot.m_aoidEquipped[0])))
            m_slot.m_oidSavedItem = 0;
        else
#endif
        m_slot.m_oidSavedItem = static_cast<unsigned int>(m_slot.m_aoidEquipped[nSlot]);
        if (m_slot.m_pSavedItemCopy != 0)
            delete m_slot.m_pSavedItemCopy;
        m_slot.m_pSavedItemCopy = CopyItem(m_slot.m_oidSavedItem);
    }

    if (oidItem != OBJECT_INVALID) {
        // The alternate-hand item is pulled out of the same hand's oid slot into the copy member so
        // it can be re-equipped into the other hand below.
        CSWSItem* pItem K2_UNSET;
        if (m_slot.m_nSlotFlag == CSWInventory::SLOT_RIGHTWEAPON) {
            pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
                g_pAppManager->m_pServerExoApp->ClientToServerObjectId(oidItem));
            if (pItem != 0 && pItem->GetBaseItem()->m_nWeaponSize == CSWBaseItem::WEAPON_SIZE_LARGE
                && m_slot.m_aoidEquipped[GUI_SLOT_WEAP_L] != OBJECT_INVALID) {
                m_slot.m_oidSavedOffHandItem = static_cast<unsigned int>(m_slot.m_aoidEquipped[GUI_SLOT_WEAP_L]);
                if (m_slot.m_pSavedOffHandCopy != 0)
                    delete m_slot.m_pSavedOffHandCopy;
                m_slot.m_pSavedOffHandCopy = CopyItem(m_slot.m_oidSavedOffHandItem);
            }
        }
#if !KOTOR1  // K1: no second weapon set
        if (m_slot.m_nSlotFlag == CSWInventory::SLOT_RIGHTWEAPON2) {
            pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
                g_pAppManager->m_pServerExoApp->ClientToServerObjectId(oidItem));
            if (pItem != 0 && pItem->GetBaseItem()->m_nWeaponSize == CSWBaseItem::WEAPON_SIZE_LARGE
                && m_slot.m_aoidEquipped[GUI_SLOT_WEAP_L2] != OBJECT_INVALID) {
                m_slot.m_oidSavedOffHandItem = static_cast<unsigned int>(m_slot.m_aoidEquipped[GUI_SLOT_WEAP_L2]);
                if (m_slot.m_pSavedOffHandCopy != 0)
                    delete m_slot.m_pSavedOffHandCopy;
                m_slot.m_pSavedOffHandCopy = CopyItem(m_slot.m_oidSavedOffHandItem);
                m_pCreature->SetEquippedItemID(CSWInventory::SLOT_LEFTWEAPON2, OBJECT_INVALID);
            }
        }
#endif

        m_pGuiManager->PlayGuiSound(GUISOUND_INVENTORY_SELECT);

        CSWCItem* pClientItem =
            g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(oidEquipped);
        if (pClientItem != 0)
            pClientItem->PowerItem(0, 0, 0, 0);

        if (ItemExists(oidItem))
            EquipItem(static_cast<unsigned int>(oidItem), m_slot.m_nSlotFlag, 1);
        else if (oidItem == static_cast<unsigned long long>(m_slot.m_oidSavedItem) && m_slot.m_pSavedItemCopy != 0)
            EquipItem(m_slot.m_pSavedItemCopy, m_slot.m_nSlotFlag, 1);

        // A large weapon occupies its own hand only, so the other hand's item moves back into it and
        // the temporary copy is dropped.
        if (m_slot.m_nSlotFlag == CSWInventory::SLOT_RIGHTWEAPON && pItem != 0
            && pItem->GetBaseItem()->m_nWeaponSize <= CSWBaseItem::WEAPON_SIZE_MEDIUM && m_slot.m_oidSavedOffHandItem != 0) {
            if (ItemExists(m_slot.m_oidSavedOffHandItem))
                EquipItem(m_slot.m_oidSavedOffHandItem, CSWInventory::SLOT_LEFTWEAPON, 0);
            else if (m_slot.m_pSavedOffHandCopy != 0)
                EquipItem(m_slot.m_pSavedOffHandCopy, CSWInventory::SLOT_LEFTWEAPON, 0);
            m_slot.m_oidSavedOffHandItem = 0;
            if (m_slot.m_pSavedOffHandCopy != 0) {
                delete m_slot.m_pSavedOffHandCopy;
                m_slot.m_pSavedOffHandCopy = 0;
            }
        }
#if !KOTOR1
        if (m_slot.m_nSlotFlag == CSWInventory::SLOT_RIGHTWEAPON2 && pItem != 0
            && pItem->GetBaseItem()->m_nWeaponSize <= CSWBaseItem::WEAPON_SIZE_MEDIUM && m_slot.m_oidSavedOffHandItem != 0) {
            if (ItemExists(m_slot.m_oidSavedOffHandItem))
                EquipItem(m_slot.m_oidSavedOffHandItem, CSWInventory::SLOT_LEFTWEAPON2, 0);
            else if (m_slot.m_pSavedOffHandCopy != 0)
                EquipItem(m_slot.m_pSavedOffHandCopy, CSWInventory::SLOT_LEFTWEAPON2, 0);
            m_slot.m_oidSavedOffHandItem = 0;
            if (m_slot.m_pSavedOffHandCopy != 0) {
                delete m_slot.m_pSavedOffHandCopy;
                m_slot.m_pSavedOffHandCopy = 0;
            }
        }
#endif
    } else {
        // For the empty row, strip both hands of the pair and re-equip the primary hand's item with
        // the alternate hand still armed, so the swap lands in one call.
        m_pGuiManager->PlayGuiSound(GUISOUND_INVENTORY_DROP);

        int nFlag = 1;
        if (m_slot.m_nSlotFlag == CSWInventory::SLOT_RIGHTWEAPON) {
            if (m_slot.m_aoidEquipped[GUI_SLOT_WEAP_L] != OBJECT_INVALID) {
                m_slot.m_oidSavedOffHandItem = static_cast<unsigned int>(m_slot.m_aoidEquipped[GUI_SLOT_WEAP_L]);
                if (m_slot.m_pSavedOffHandCopy != 0)
                    delete m_slot.m_pSavedOffHandCopy;
                m_slot.m_pSavedOffHandCopy = CopyItem(m_slot.m_oidSavedOffHandItem);
                UnequipItem(static_cast<unsigned int>(m_slot.m_aoidEquipped[GUI_SLOT_WEAP_L]), 1);
                nFlag = 0;
            }
        }
#if !KOTOR1
        if (m_slot.m_nSlotFlag == CSWInventory::SLOT_RIGHTWEAPON2) {
            if (m_slot.m_aoidEquipped[GUI_SLOT_WEAP_L2] != OBJECT_INVALID) {
                m_slot.m_oidSavedOffHandItem = static_cast<unsigned int>(m_slot.m_aoidEquipped[GUI_SLOT_WEAP_L2]);
                if (m_slot.m_pSavedOffHandCopy != 0)
                    delete m_slot.m_pSavedOffHandCopy;
                m_slot.m_pSavedOffHandCopy = CopyItem(m_slot.m_oidSavedOffHandItem);
                UnequipItem(static_cast<unsigned int>(m_slot.m_aoidEquipped[GUI_SLOT_WEAP_L2]), 1);
                nFlag = 0;
            }
        }
#endif
        UnequipItem(static_cast<unsigned int>(oidEquipped), nFlag);
    }

    m_slot.m_nPendingSlot = m_slot.m_nSlotFlag;
    m_slot.m_oidPendingItem = static_cast<unsigned int>(oidItem);
}

#if defined(K2_UI_XBOX) || defined(K1_UI_XBOX) || (defined(K2_UI_TOUCH) && defined(K2_FIX_ENGINE_BUGS))
void CSWGuiInGameEquip::K2OnItemAccepted(CSWGuiControl* pControl) {
    OnItemSelected(pControl);
    OnOKPressed(pControl);
}
#endif

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWGuiInGameEquip::OnItemExited(CSWGuiControl* pControl) {
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWSItem* pItem =
        g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
            static_cast<CSWGuiInGameItemEntry*>(pControl)->m_oidItem);
    if (pItem == 0)
        return;
    if (!pItem->m_bits.m_bNewItem)
        return;

    pItem->m_bits.m_bNewItem = 0;

    CSWGuiInGameItemEntry* pEntry = static_cast<CSWGuiInGameItemEntry*>(pControl);
    pEntry->bNewItem = 0;
    pEntry->SetItem(pEntry->m_oidItem, pEntry->bEquipped, 0);
}
#endif // !KOTOR1

#if KOTOR1
void CSWGuiInGameEquip::UpdateInventory() {
    CResRef resRef;

    m_apSlotLabels[GUI_SLOT_WEAP_L].m_border.m_params.SetFillImage(GetIcon(GUI_SLOT_WEAP_L), 0);
    m_apSlotLabels[GUI_SLOT_WEAP_L].m_border.m_params.m_fAlpha = 1.0f;

    unsigned long long oidEquipped K2_UNSET;
    CSWCItem* pClientItem = 0;
    CSWSItem* pServerItem = 0;
    int bEmptyReadoutDone = 0;
    int bMirrored = 0;

    for (int nSlot = 0; nSlot < 9; nSlot++) {
        if (m_pCreature != 0) {
            oidEquipped = m_pCreature->GetEquippedItemID(SLOT_INFO[nSlot].m_nSlotFlag);
            pClientItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(oidEquipped);
            if (pClientItem != 0)
                pServerItem = pClientItem->GetServerItem();
        }

        if (pServerItem != 0 && pClientItem != 0) {
            m_slot.m_aoidEquipped[nSlot] = oidEquipped;
            resRef = pServerItem->GetIcon();
            unsigned char nWieldType = pServerItem->GetBaseItem()->m_weaponWieldType;

            if (nSlot == GUI_SLOT_WEAP_L || nSlot == GUI_SLOT_WEAP_R) {
                CSWGuiLabel* pDamageLabel;
                CSWGuiLabel* pAttackLabel;
                int bMirror = nWieldType == CSWBaseItem::WEAPON_WIELD_TWO_HANDED && !bMirrored;
                int bOffHand = 0;
                if (nSlot == 0) {
                    pDamageLabel = &m_lblAtkL;
                    pAttackLabel = &m_lblToHitL;
                    bOffHand = 1;
                    if (bMirror) {
                        bMirrored = 1;
                        nSlot--;
                    }
                } else if (bMirror) {
                    pDamageLabel = &m_lblAtkL;
                    pAttackLabel = &m_lblToHitL;
                    bOffHand = 1;
                    bMirrored = 1;
                    nSlot--;
                } else {
                    pDamageLabel = &m_lblAtkR;
                    pAttackLabel = &m_lblToHitR;
                }

                CSWSCreature* pServerCreature = m_pCreature->GetServerCreature();
                CExoString sDamage;
                CExoString sAttack;
                int nMin = 0;
                int nMax = 0;
                pServerCreature->m_pStats->GetTotalDamageRange(pServerCreature, pServerItem,
                                                               nMin, nMax, bOffHand, 1);
                if (nMin != 0)
                    sDamage.Format(const_cast<char*>("%d-%d"), nMin, nMax);
                int nMin2 = 0;
                int nMax2 = 0;
                pServerCreature->m_pStats->GetTotalDamageRange(pServerCreature, pServerItem,
                                                               nMin2, nMax2, bOffHand, 0);
                int bBetter = nMin > nMin2 || nMax > nMax2;

                if (nSlot == 1) {
                    if (pServerItem->GetBaseItem()->m_nWeaponSize == CSWBaseItem::WEAPON_SIZE_LARGE
                        || pServerItem->GetBaseItem()->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_TWO_HANDED) {
                        m_apSlotLabels[GUI_SLOT_WEAP_L].m_border.m_params.SetFillImage(resRef, 0);
                        m_apSlotLabels[GUI_SLOT_WEAP_L].m_border.m_params.m_fAlpha = 0.25f;
                    }
                }

                pDamageLabel->m_text.m_params.SetText(sDamage);
                if (bBetter)
                    pDamageLabel->m_text.m_params.SetColor(CGuiInGame::COLOR_GREEN);
                else
                    pDamageLabel->m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);

                int nAttack;
                if (pServerItem->GetBaseItem()->m_bRangedWeapon)
                    nAttack = pServerCreature->m_pStats->GetRangedAttackBonus(bOffHand, 1, 0);
                else
                    nAttack = pServerCreature->m_pStats->GetMeleeAttackBonus(bOffHand, 1, 0,
                                                                            nWieldType == CSWBaseItem::WEAPON_WIELD_TWO_HANDED);
                if (nAttack >= 1)
                    sAttack.Format(const_cast<char*>("+%d"), nAttack);
                else
                    sAttack.Format(const_cast<char*>("%d"), nAttack);
                pAttackLabel->m_text.m_params.SetText(sAttack);
                if (nAttack > pServerCreature->m_pStats->GetBaseAttackBonus())
                    pAttackLabel->m_text.m_params.SetColor(CGuiInGame::COLOR_GREEN);
                else
                    pAttackLabel->m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
            }

            if ((nWieldType == CSWBaseItem::WEAPON_WIELD_TWO_HANDED && nSlot == GUI_SLOT_WEAP_R) || nWieldType != CSWBaseItem::WEAPON_WIELD_TWO_HANDED)
                m_apSlotLabels[nSlot].m_border.m_params.SetFillImage(resRef, 0);
        } else {
            int bReadout = 0;
            if (nSlot == 1) {
                CSWSCreature* pServerCreature = m_pCreature->GetServerCreature();
                if (pServerCreature != 0) {
                    CExoString sDamage;
                    int nMin;
                    int nMax;
                    pServerCreature->m_pStats->GetTotalDamageRange(pServerCreature, 0, nMin, nMax, 0, 1);
                    if (nMin != 0)
                        sDamage.Format(const_cast<char*>("%d-%d"), nMin, nMax);
                    m_lblAtkR.m_text.m_params.SetText(sDamage);
                    CExoString sAttack;
                    int nAttack = pServerCreature->m_pStats->GetMeleeAttackBonus(0, 1, 0, 0);
                    if (nAttack >= 1)
                        sAttack.Format(const_cast<char*>("+%d"), nAttack);
                    else
                        sAttack.Format(const_cast<char*>("%d"), nAttack);
                    m_lblToHitR.m_text.m_params.SetText(sAttack);
                }
                bEmptyReadoutDone = 1;
                bReadout = 1;
            }

            if ((bEmptyReadoutDone || bMirrored) && m_slot.m_aoidEquipped[nSlot] == OBJECT_INVALID) {
                if (!bReadout)
                    continue;
            } else {
                m_slot.m_aoidEquipped[nSlot] = OBJECT_INVALID;

                switch (nSlot) {
                case GUI_SLOT_WEAP_L:
                    m_lblAtkL.m_text.m_params.SetText("");
                    m_lblAtkL.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
                    m_lblToHitL.m_text.m_params.SetText("");
                    m_lblToHitL.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
                    break;
                case GUI_SLOT_WEAP_R:
                    m_lblAtkR.m_text.m_params.SetText("");
                    m_lblAtkR.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
                    m_lblToHitR.m_text.m_params.SetText("");
                    m_lblToHitR.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
                    break;
                }

                m_apSlotLabels[nSlot].m_border.m_params.SetFillImage(GetIcon(nSlot), 0);
            }
        }

        if (m_pActiveControl == &m_abtnSlot[nSlot])
            OnEnterSlot(m_pActiveControl);
    }

    m_lblDef.m_text.m_params.SetText(CExoString(m_pCreature->GetServerCreature()->GetArmorClass()));

    CExoString sHP;
    sHP.Format(const_cast<char*>("%d/%d"), m_pCreature->GetServerCreature()->GetCurrentHitPoints(0),
               m_pCreature->GetServerCreature()->GetMaxHitPoints(1));
    m_lblVitality.m_text.m_params.SetText(sHP);
    if (m_lblVitality.m_text.GetIdealHeight() > m_lblVitality.m_text.GetFontHeight()) {
        sHP.Format(const_cast<char*>("%d/ %d"), m_pCreature->m_pStats->m_nCurrentHitPoints,
                   static_cast<short>(m_pCreature->m_pStats->GetMaxHitPoints()));
        m_lblVitality.m_text.m_params.SetText(sHP);
    }

    if (m_slot.m_nPendingSlot != 0) {
        int bStale;
        if (m_pCreature->GetEquippedItemID(m_slot.m_nPendingSlot) == static_cast<uint64_t>(m_slot.m_oidPendingItem)) {
            bStale = 0;
        } else {
            CSWSItem* pOld = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
                g_pAppManager->m_pServerExoApp->ClientToServerObjectId(m_slot.m_oidPendingItem));
            CSWSItem* pNew = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
                g_pAppManager->m_pServerExoApp->ClientToServerObjectId(
                    m_pCreature->GetEquippedItemID(m_slot.m_nPendingSlot)));
            bStale = pOld != 0 && (pNew == 0 || pNew->CompareItem(pOld) == 0);
        }
        if (!bStale) {
            m_slot.m_nPendingSlot = 0;
        } else {
            CExoString sDash("--");
            switch (m_slot.m_nPendingSlot) {
            case CSWInventory::SLOT_BODY:
                m_lblDef.m_text.m_params.SetText(sDash);
                break;
            case CSWInventory::SLOT_RIGHTWEAPON:
                m_lblAtkR.m_text.m_params.SetText(sDash);
                m_lblToHitR.m_text.m_params.SetText(sDash);
                if (m_lblAtkL.m_text.m_params.m_sText != "") {
                    m_lblAtkL.m_text.m_params.SetText(sDash);
                    m_lblToHitL.m_text.m_params.SetText(sDash);
                }
                break;
            case CSWInventory::SLOT_LEFTWEAPON:
                m_lblToHitR.m_text.m_params.SetText(sDash);
                m_lblAtkL.m_text.m_params.SetText(sDash);
                m_lblToHitL.m_text.m_params.SetText(sDash);
                break;
            case CSWInventory::SLOT_HEAD:
            case CSWInventory::SLOT_HANDS:
            case CSWInventory::SLOT_LEFTARM:
            case CSWInventory::SLOT_RIGHTARM:
            case CSWInventory::SLOT_IMPLANT:
            case CSWInventory::SLOT_BELT:
                m_lblAtkR.m_text.m_params.SetText(sDash);
                m_lblToHitR.m_text.m_params.SetText(sDash);
                if (m_lblAtkL.m_text.m_params.m_sText != "") {
                    m_lblAtkL.m_text.m_params.SetText(sDash);
                    m_lblToHitL.m_text.m_params.SetText(sDash);
                }
                m_lblDef.m_text.m_params.SetText(sDash);
                m_lblVitality.m_text.m_params.SetText(sDash);
                break;
            }
        }
    }
}
#else
void CSWGuiInGameEquip::UpdateInventory() {
    CResRef resRef;
    Vector colorNewText = CGuiInGame::COLOR_NEW_TEXT;

    // The two primary-hand slot labels start from their generic icon at full alpha.
    m_apSlotLabels[GUI_SLOT_WEAP_L].m_border.m_params.SetFillImage(GetIcon(GUI_SLOT_WEAP_L));
    m_apSlotLabels[GUI_SLOT_WEAP_L].m_border.m_params.m_fAlpha = 1.0f;
    m_apSlotLabels[GUI_SLOT_WEAP_L2].m_border.m_params.SetFillImage(GetIcon(GUI_SLOT_WEAP_L2));
    m_apSlotLabels[GUI_SLOT_WEAP_L2].m_border.m_params.m_fAlpha = 1.0f;

    m_lblAtkL2.m_text.m_params.SetText("");
    m_lblAtkL2.m_text.m_params.SetColor(colorNewText);
    m_lblToHitL2.m_text.m_params.SetText("");
    m_lblToHitL2.m_text.m_params.SetColor(colorNewText);

    unsigned long long oidEquipped = 0;
    CSWCItem* pClientItem = 0;
    CSWSItem* pServerItem = 0;
    int bEmptyReadoutDone = 0;
    int bMirrored = 0;

    for (int nSlot = 0; nSlot < 11; nSlot++) {
        if (m_pCreature != 0) {
            oidEquipped = m_pCreature->GetEquippedItemID(SLOT_INFO[nSlot].m_nSlotFlag);
            pClientItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(oidEquipped);
            if (pClientItem != 0)
                pServerItem = pClientItem->GetServerItem();
        }

        if (pClientItem != 0 && pServerItem != 0) {
            m_slot.m_aoidEquipped[nSlot] = oidEquipped;
            resRef = pServerItem->GetIcon();
            unsigned char nWieldType = pServerItem->GetBaseItem()->m_weaponWieldType;

            CSWGuiLabel* pDamageLabel = 0;
            CSWGuiLabel* pAttackLabel = 0;

            if (nSlot == GUI_SLOT_WEAP_L || nSlot == GUI_SLOT_WEAP_R || nSlot == GUI_SLOT_WEAP_L2 || nSlot == GUI_SLOT_WEAP_R2) {
                // A `d` item is shown on the left pair first, once per refresh: the slot index steps
                // back so the next iteration revisits the slot for the right pair.
                int bMirror = nWieldType == CSWBaseItem::WEAPON_WIELD_TWO_HANDED && !bMirrored;
                int bOffHand = 0;
                if (nSlot == 0) {
                    pDamageLabel = &m_lblAtkL;
                    pAttackLabel = &m_lblToHitL;
                    bOffHand = 1;
                    if (bMirror) {
                        bMirrored = 1;
                        nSlot--;
                    }
                } else if (nSlot == 1) {
                    if (bMirror) {
                        pDamageLabel = &m_lblAtkL;
                        pAttackLabel = &m_lblToHitL;
                        bOffHand = 1;
                        bMirrored = 1;
                        nSlot--;
                    } else {
                        pDamageLabel = &m_lblAtkR;
                        pAttackLabel = &m_lblToHitR;
                    }
                } else if (nSlot == 9) {
                    pDamageLabel = &m_lblAtkL2;
                    pAttackLabel = &m_lblToHitL2;
                    bOffHand = 1;
                    if (bMirror) {
                        bMirrored = 1;
                        nSlot--;
                    }
                } else if (bMirror) {
                    pDamageLabel = &m_lblAtkL2;
                    pAttackLabel = &m_lblToHitL2;
                    bOffHand = 1;
                    bMirrored = 1;
                    nSlot--;
                } else {
                    pDamageLabel = &m_lblAtkR2;
                    pAttackLabel = &m_lblToHitR2;
                }

                CSWSCreature* pServerCreature = m_pCreature->GetServerCreature();
                CExoString sDamage;
                CExoString sAttack;
                int nMin = 0;
                int nMax = 0;
                pServerCreature->m_pStats->GetTotalDamageRange(pServerCreature, pServerItem,
                                                               nMin, nMax, bOffHand, 1);
                if (nMin != 0)
                    sDamage.Format(const_cast<char*>("%d-%d"), nMin, nMax);
                int nMin2 = 0;
                int nMax2 = 0;
                pServerCreature->m_pStats->GetTotalDamageRange(pServerCreature, pServerItem,
                                                               nMin2, nMax2, bOffHand, 0);

                if (nSlot == 1) {
                    if (pServerItem->GetBaseItem()->m_nWeaponSize == CSWBaseItem::WEAPON_SIZE_LARGE
                        || pServerItem->GetBaseItem()->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_TWO_HANDED) {
                        m_apSlotLabels[GUI_SLOT_WEAP_L].m_border.m_params.SetFillImage(resRef);
                        m_apSlotLabels[GUI_SLOT_WEAP_L].m_border.m_params.m_fAlpha = 0.25f;
                    }
                } else if (nSlot == 10) {
                    if (pServerItem->GetBaseItem()->m_nWeaponSize == CSWBaseItem::WEAPON_SIZE_LARGE
                        || pServerItem->GetBaseItem()->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_TWO_HANDED) {
                        m_apSlotLabels[GUI_SLOT_WEAP_L2].m_border.m_params.SetFillImage(resRef);
                        m_apSlotLabels[GUI_SLOT_WEAP_L2].m_border.m_params.m_fAlpha = 0.25f;
                    }
                }

                pDamageLabel->m_text.m_params.SetText(sDamage);
                pDamageLabel->m_text.m_params.SetColor(colorNewText);

                int nAttack;
                if (pServerItem->GetBaseItem()->m_bRangedWeapon)
                    nAttack = pServerCreature->m_pStats->GetRangedAttackBonus(bOffHand, 1, 0);
                else
                    nAttack = pServerCreature->m_pStats->GetMeleeAttackBonus(bOffHand, 1, 0,
                                                                            nWieldType == CSWBaseItem::WEAPON_WIELD_TWO_HANDED);
                if (nAttack >= 1)
                    sAttack.Format(const_cast<char*>("+%d"), nAttack);
                else
                    sAttack.Format(const_cast<char*>("%d"), nAttack);
                pAttackLabel->m_text.m_params.SetText(sAttack);
                pAttackLabel->m_text.m_params.SetColor(colorNewText);
            }

            // The comparison is deliberate rather than an assignment. A `d` item on the right hand
            // is mirrored onto the left pair.
            if ((nWieldType == CSWBaseItem::WEAPON_WIELD_TWO_HANDED && nSlot == GUI_SLOT_WEAP_R) || nWieldType != CSWBaseItem::WEAPON_WIELD_TWO_HANDED)
                m_apSlotLabels[nSlot].m_border.m_params.SetFillImage(resRef);
            if ((nWieldType == CSWBaseItem::WEAPON_WIELD_TWO_HANDED && nSlot == GUI_SLOT_WEAP_R2) || nWieldType != CSWBaseItem::WEAPON_WIELD_TWO_HANDED)
                m_apSlotLabels[nSlot].m_border.m_params.SetFillImage(resRef);
        } else {
            // An empty right-hand slot shows the unarmed readout.
            int bReadout = 0;
            if (nSlot == GUI_SLOT_WEAP_R || nSlot == GUI_SLOT_WEAP_R2) {
                CSWGuiLabel* pDamageLabel2;
                CSWGuiLabel* pAttackLabel2;
                if (nSlot == 1) {
                    pDamageLabel2 = &m_lblAtkR;
                    pAttackLabel2 = &m_lblToHitR;
                } else {
                    pDamageLabel2 = &m_lblAtkR2;
                    pAttackLabel2 = &m_lblToHitR2;
                }
                CSWSCreature* pServerCreature = m_pCreature->GetServerCreature();
                if (pServerCreature != 0) {
                    CExoString sDamage;
                    int nMin = 0;
                    int nMax = 0;
                    pServerCreature->m_pStats->GetTotalDamageRange(pServerCreature, 0, nMin, nMax, 0, 1);
                    if (nMin != 0)
                        sDamage.Format(const_cast<char*>("%d-%d"), nMin, nMax);
                    pDamageLabel2->m_text.m_params.SetText(sDamage);
                    CExoString sAttack;
                    int nAttack = pServerCreature->m_pStats->GetMeleeAttackBonus(0, 1, 0, 0);
                    if (nAttack >= 1)
                        sAttack.Format(const_cast<char*>("+%d"), nAttack);
                    else
                        sAttack.Format(const_cast<char*>("%d"), nAttack);
                    pAttackLabel2->m_text.m_params.SetText(sAttack);
                }
                bEmptyReadoutDone = 1;
                bReadout = 1;
            }

            // Once a readout or a mirrored item has run, a slot already marked empty keeps its labels
            // and icon.
            if ((bEmptyReadoutDone || bMirrored) && m_slot.m_aoidEquipped[nSlot] == OBJECT_INVALID) {
                if (!bReadout)
                    continue;
            } else {
                m_slot.m_aoidEquipped[nSlot] = OBJECT_INVALID;

                switch (nSlot) {
                case GUI_SLOT_WEAP_L:
                    m_lblAtkL.m_text.m_params.SetText("");
                    m_lblAtkL.m_text.m_params.SetColor(colorNewText);
                    m_lblToHitL.m_text.m_params.SetText("");
                    m_lblToHitL.m_text.m_params.SetColor(colorNewText);
                    break;
                case GUI_SLOT_WEAP_R:
                    m_lblAtkR.m_text.m_params.SetText("");
                    m_lblAtkR.m_text.m_params.SetColor(colorNewText);
                    m_lblToHitR.m_text.m_params.SetText("");
                    m_lblToHitR.m_text.m_params.SetColor(colorNewText);
                    break;
                case GUI_SLOT_WEAP_L2:
                    m_lblAtkL2.m_text.m_params.SetText("");
                    m_lblAtkL2.m_text.m_params.SetColor(colorNewText);
                    m_lblToHitL2.m_text.m_params.SetText("");
                    m_lblToHitL2.m_text.m_params.SetColor(colorNewText);
                    break;
                case GUI_SLOT_WEAP_R2:
                    m_lblAtkR2.m_text.m_params.SetText("");
                    m_lblAtkR2.m_text.m_params.SetColor(colorNewText);
                    m_lblToHitR2.m_text.m_params.SetText("");
                    m_lblToHitR2.m_text.m_params.SetColor(colorNewText);
                    break;
                }

                m_apSlotLabels[nSlot].m_border.m_params.SetFillImage(GetIcon(nSlot));
            }
        }

        if (m_pActiveControl == &m_abtnSlot[nSlot])
            OnEnterSlot(m_pActiveControl);
    }

    if (m_slot.m_bNPCOverride != 0)
        m_lblDef.m_text.m_params.SetText(
            CExoString(m_pCreature->m_pStats->GetArmorClass(1)));
    else
        m_lblDef.m_text.m_params.SetText(
            CExoString(m_pCreature->m_pStats->GetArmorClass(1)));

    CExoString sHP;
    sHP.Format(const_cast<char*>("%d/%d"), m_pCreature->GetServerCreature()->GetCurrentHitPoints(0),
               m_pCreature->GetServerCreature()->GetMaxHitPoints(1));

    if (m_slot.m_nPendingSlot != 0) {
        int bStale;
        if (m_pCreature->GetEquippedItemID(m_slot.m_nPendingSlot) == static_cast<uint64_t>(m_slot.m_oidPendingItem)) {
            bStale = 0;
        } else {
            CSWSItem* pOld = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
                g_pAppManager->m_pServerExoApp->ClientToServerObjectId(m_slot.m_oidPendingItem));
            CSWSItem* pNew = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
                g_pAppManager->m_pServerExoApp->ClientToServerObjectId(
                    m_pCreature->GetEquippedItemID(m_slot.m_nPendingSlot)));
            bStale = pOld != 0 && (pNew == 0 || pNew->CompareItem(pOld) == 0);
        }
        if (!bStale) {
            m_slot.m_nPendingSlot = 0;
        } else {
            CExoString sDash("--");
            switch (m_slot.m_nPendingSlot) {
            case CSWInventory::SLOT_HEAD:
            case CSWInventory::SLOT_HANDS:
            case CSWInventory::SLOT_LEFTARM:
            case CSWInventory::SLOT_RIGHTARM:
            case CSWInventory::SLOT_IMPLANT:
            case CSWInventory::SLOT_BELT:
                m_lblAtkR.m_text.m_params.SetText(sDash);
                m_lblToHitR.m_text.m_params.SetText(sDash);
                if (m_lblAtkL.m_text.m_params.m_sText != "") {
                    m_lblAtkL.m_text.m_params.SetText(sDash);
                    m_lblToHitL.m_text.m_params.SetText(sDash);
                }
                break;
            }
        }
    }
}
#endif

CSWGuiInGameItemEntry* CSWGuiInGameEquip::AddItemEntryToList(int nIndex) {
    CSWGuiInGameItemEntry* pEntry;
    if (m_lstItemEntries.m_count == nIndex) {
        pEntry = new CSWGuiInGameItemEntry();
        m_lstItemEntries.Add(pEntry);

        CSWGuiControl* pProto = m_lbItems.m_pProtoItem;
        pEntry->Initialize(static_cast<CSWGuiButton*>(pProto)->m_text,
                           static_cast<CSWGuiButton*>(pProto)->m_border,
                           static_cast<CSWGuiButton*>(pProto)->m_borderHilight,
                           m_lbItems.m_innerExtent.w - m_lbItems.m_nPadding * 2, 1);
        pEntry->AddEvent(SWGUIEVENT_FOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameEquip::OnItemSelected));
#if !KOTOR1  // K1: no exit handler
        pEntry->AddEvent(SWGUIEVENT_UNFOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameEquip::OnItemExited));
#endif
#if defined(K2_UI_XBOX) || defined(K1_UI_XBOX) || (defined(K2_UI_TOUCH) && defined(K2_FIX_ENGINE_BUGS))
        pEntry->AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameEquip::K2OnItemAccepted));
#else
        pEntry->AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiInGameEquip::OnOKPressed));
#endif
    } else {
        pEntry = static_cast<CSWGuiInGameItemEntry*>(m_lstItemEntries.m_pArray[nIndex]);
    }

    pEntry->m_border.m_params.m_fAlpha = 0.0f;
    return pEntry;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWGuiInGameEquip::OnPrevNPCClicked(CSWGuiControl* pControl) {
    if (pControl != 0 && pControl->m_tail.nCurrentEventArg == 0)
        return;
    if (m_slot.m_bNPCOverride == 0)
        return;

    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    if (pPartyTable == 0)
        return;

    for (int i = 0; i < 12; i++) {
        int nOld = m_slot.m_nOverrideNPC;
        m_slot.m_nOverrideNPC = nOld - 1;
        if (nOld == 0) {
            SetCharacter(g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter());
            g_pAppManager->m_pClientExoApp->GetInGameGui()->SetTopMenuOverrideCharacter(NPC_PLAYER);
            return;
        }
        if (m_slot.m_nOverrideNPC == -2)
            m_slot.m_nOverrideNPC = 11;
        if (!pPartyTable->GetIsNPCAvailable(m_slot.m_nOverrideNPC))
            continue;

        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(
                pPartyTable->GetNPCObject(m_slot.m_nOverrideNPC, 0, 1)));
        if (pCreature == 0)
            continue;
        if (m_slot.m_nOverrideNPC == NPC_PLAYER) {
            SetCharacter(g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter());
            g_pAppManager->m_pClientExoApp->GetInGameGui()->SetTopMenuOverrideCharacter(NPC_PLAYER);
            return;
        }

        SetCharacter(pCreature);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetTopMenuOverrideCharacter(m_slot.m_nOverrideNPC);
        return;
    }
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWGuiInGameEquip::OnNextNPCClicked(CSWGuiControl* pControl) {
    if (pControl != 0 && pControl->m_tail.nCurrentEventArg == 0)
        return;
    if (m_slot.m_bNPCOverride == 0)
        return;

    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    if (pPartyTable == 0)
        return;

    CExoString sTag;
    CSWCCreature* pCreature = 0;
    for (int i = 0; i < 12; i++) {
        int nOld = m_slot.m_nOverrideNPC;
        m_slot.m_nOverrideNPC = nOld + 1;
        if (m_slot.m_nOverrideNPC == 12) {
            m_slot.m_nOverrideNPC = NPC_PLAYER;
        } else {
            if (!pPartyTable->GetIsNPCAvailable(m_slot.m_nOverrideNPC))
                continue;

            switch (m_slot.m_nOverrideNPC) {
            case NPC_ATTON:       sTag = "atton";      break;
            case NPC_BAO_DUR:     sTag = "baodur";     break;
            case NPC_CANDEROUS:   sTag = "mand";       break;
            case NPC_G0T0:        sTag = "g0t0";       break;
            case NPC_HANDMAIDEN:  sTag = "handmaiden"; break;
            case NPC_HK_47:       sTag = "hk47";       break;
            case NPC_KREIA:       sTag = "kreia";      break;
            case NPC_MIRA:        sTag = "mira";       break;
            case NPC_T3_M4:       sTag = "t3m4";       break;
            case NPC_VISAS:       sTag = "visasmarr";  break;
            case NPC_HANHARR:     sTag = "hanharr";    break;
            case NPC_DISCIPLE:    sTag = "disciple";   break;
            }

            CClientExoApp* pClient;
            unsigned long long oidTag =
                g_pAppManager->m_pServerExoApp->GetModule()->FindObjectByTagOrdinal(
                    sTag.LowerCase(), 0);

            pClient = g_pAppManager->m_pClientExoApp;
            pCreature =
                pClient->GetCreatureByGameObjectID(pClient->ServerToClientObjectId(oidTag));
            if (pCreature == 0)
                continue;

            g_pAppManager->m_pServerExoApp->GetPartyTable()->SetNPCObject(m_slot.m_nOverrideNPC, oidTag);
        }

        if (m_slot.m_nOverrideNPC != NPC_PLAYER) {
            SetCharacter(pCreature);
            g_pAppManager->m_pClientExoApp->GetInGameGui()->SetTopMenuOverrideCharacter(
                m_slot.m_nOverrideNPC);
            OnEnterSlot(m_pActiveControl);
            return;
        }

        SetCharacter(g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter());
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetTopMenuOverrideCharacter(NPC_PLAYER);
        OnEnterSlot(m_pActiveControl);
        return;
    }
}
#endif // !KOTOR1

void CSWGuiInGameEquip::OnPanelAdded() {
#if KOTOR1
    // K1: shows the leader, resets the portrait cursor, refreshes the panel's own portraits; the 'y'
    // glyph only with a party.
    m_slot.m_nShownNPC = -1;
    m_btnChange1.m_tail.m_nUserData = static_cast<uint64_t>(-2);
    m_btnChange2.m_tail.m_nUserData = static_cast<uint64_t>(-2);
    SetCharacter(g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter());
    UpdatePortraits(1);
    OnEnterSlot(m_pActiveControl);
    CSWGuiPanel::OnPanelAdded();
    if (g_pAppManager != 0 && g_pAppManager->m_pClientExoApp != 0
        && g_pAppManager->m_pClientExoApp->GetSWParty() != 0)
#ifdef K2_UI_TOUCH
        m_iconChange.m_tail.bVisible = g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount > 1;
#else
        // the change button's own rule: a party of two or more, or in a module that browses the
        // party table (the Ebon Hawk, the hideout) any member it can show
        m_iconChange.m_tail.bVisible = m_btnChange1.m_tail.bVisible;
#endif
    m_nFocusFrames = 3;
#else
    CExoString sModuleName = g_pAppManager->m_pServerExoApp->GetModuleName();
    if (sModuleName.CompareNoCase("003EBO") == 0) {
        m_slot.m_bNPCOverride = 0;
        m_slot.m_nOverrideNPC = NPC_PLAYER;
        m_btnNextNPC.m_tail.bVisible = 0;
        m_btnPrevNPC.m_tail.bVisible = 0;
        m_btnSwapWeapons.m_tail.bVisible = 1;
    } else {
        m_slot.m_bNPCOverride = 1;
        m_slot.m_nOverrideNPC = NPC_PLAYER;
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetTopMenuOverrideCharacter(NPC_PLAYER);
        m_btnNextNPC.m_tail.bVisible = 1;
        m_btnPrevNPC.m_tail.bVisible = 1;
        m_btnSwapWeapons.m_tail.bVisible = 0;
    }

    SetCharacter(g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter());
    g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowGameMenuChangeButtons(1);
    OnEnterSlot(m_pActiveControl);
    CSWGuiPanel::OnPanelAdded();
#endif
}

void CSWGuiInGameEquip::OnPanelRemoved() {
#if !KOTOR1  // K1: no NPC override to drop
    if (m_slot.m_bNPCOverride != 0)
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetTopMenuOverrideCharacter(NPC_PLAYER);
#endif

    if (m_slot.m_bFlags & SLOTSTATE_SELECTING_ITEM)
        CloseDescription();
    m_lbItems.ClearItems();

    for (int i = 0; i < m_lstItemEntries.m_count; i++) {
        CSWGuiControl* pControl = static_cast<CSWGuiControl*>(m_lstItemEntries.m_pArray[i]);
        if (pControl != 0)
            delete pControl;
        m_lstItemEntries.m_pArray[i] = 0;
    }

    m_lstItemEntries.RemoveAll(1);

    m_slot.m_nPendingSlot = 0;
    m_slot.m_oidPendingItem = OBJECT_INVALID;

    if (m_slot.m_pSavedItemCopy != 0) {
        delete m_slot.m_pSavedItemCopy;
        m_slot.m_pSavedItemCopy = 0;
    }
    if (m_slot.m_pSavedOffHandCopy != 0) {
        delete m_slot.m_pSavedOffHandCopy;
        m_slot.m_pSavedOffHandCopy = 0;
    }

    CSWGuiPanel::OnPanelRemoved();
}

void CSWGuiInGameEquip::ShowCantEquipMessage(int bShow, uint64_t nStrRef) {
#if KOTOR1
    // K1: the stat readout hides while the message shows; COLOR_BLUE / DISABLED_TEXT caption.
    m_lblAtkL.m_tail.bVisible = bShow == 0;
    m_lblAtkR.m_tail.bVisible = bShow == 0;
    m_lblDef.m_tail.bVisible = bShow == 0;
    m_lblToHitL.m_tail.bVisible = bShow == 0;
    m_lblToHitR.m_tail.bVisible = bShow == 0;
    m_lblVitality.m_tail.bVisible = bShow == 0;
    m_lblAttackInfo.m_tail.bVisible = bShow == 0;
    m_lblDefInfo.m_tail.bVisible = bShow == 0;
    m_lblToHit.m_tail.bVisible = bShow == 0;
    m_lblDamage.m_tail.bVisible = bShow == 0;
    if (bShow)
        m_lblCantEquip.m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(nStrRef));
    m_lblCantEquip.m_tail.bVisible = static_cast<unsigned char>(bShow);

    m_btnEquip.SetEnabled(bShow == 0);
    m_btnEquip.m_text.m_params.SetColor(bShow ? CGuiInGame::DISABLED_TEXT : CGuiInGame::COLOR_BLUE);
#else
    if (bShow) {
        m_lblCantEquip.m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(nStrRef));
#ifdef K2_QOL_IMPROVEMENTS
        // The banner at the Text Size, as far as LBL_CANTEQUIP's box (several lines tall) holds it.
        K2QolFitText(m_lblCantEquip.m_text, m_lblCantEquip.m_text.m_extent.h);
#endif
    }
    m_lblCantEquip.m_tail.bVisible = static_cast<unsigned char>(bShow);

    m_btnEquip.SetEnabled(bShow == 0);
    m_btnEquip.m_text.m_params.SetColor(
        bShow ? CGuiInGame::COLOR_NEW_GREEN : CGuiInGame::COLOR_NEW_TEXT);
    m_btnEquip.m_border.m_params.m_color =
        *reinterpret_cast<const CSWGuiColor*>(&(bShow ? CGuiInGame::DISABLED_TEXT : CGuiInGame::COLOR_NEW_GREEN));
#endif
}

#if KOTOR1
void CSWGuiInGameEquip::HandleInputEvent(int type, int val)
{
    if (val != 0)
    {
        switch (type)
        {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
        case INPUTEVENT_GUI:
            if (m_slot.m_bFlags & SLOTSTATE_SELECTING_ITEM)
            {
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);

                if (m_slot.m_oidSavedItem != 0)
                {
                    int nSlot = MapGlobalSlotToGuiSlot(m_slot.m_nSlotFlag);
                    if (m_slot.m_aoidEquipped[nSlot] != static_cast<unsigned long long>(m_slot.m_oidSavedItem))
                    {
                        if (m_slot.m_oidSavedItem != OBJECT_INVALID)
                        {
                            if (ItemExists(m_slot.m_oidSavedItem))
                                EquipItem(m_slot.m_oidSavedItem, m_slot.m_nSlotFlag, 1);
                            else if (m_slot.m_pSavedItemCopy != 0)
                                EquipItem(m_slot.m_pSavedItemCopy, m_slot.m_nSlotFlag, 1);
                        }
                        else
                        {
                            UnequipItem(static_cast<unsigned int>(m_slot.m_aoidEquipped[
                                MapGlobalSlotToGuiSlot(m_slot.m_nSlotFlag)]), 1);
                        }

                        if (m_slot.m_nSlotFlag == CSWInventory::SLOT_RIGHTWEAPON)
                        {
                            if (m_slot.m_oidSavedOffHandItem != 0)
                            {
                                if (ItemExists(m_slot.m_oidSavedOffHandItem))
                                    EquipItem(m_slot.m_oidSavedOffHandItem, CSWInventory::SLOT_LEFTWEAPON, 0);
                                else if (m_slot.m_pSavedOffHandCopy != 0)
                                    EquipItem(m_slot.m_pSavedOffHandCopy, CSWInventory::SLOT_LEFTWEAPON, 0);
                            }
                        }
                    }
                }

                m_slot.m_oidSavedItem = 0;
                if (m_slot.m_pSavedItemCopy != 0) {
                    delete m_slot.m_pSavedItemCopy;
                    m_slot.m_pSavedItemCopy = 0;
                }
                m_slot.m_oidSavedOffHandItem = 0;
                if (m_slot.m_pSavedOffHandCopy != 0) {
                    delete m_slot.m_pSavedOffHandCopy;
                    m_slot.m_pSavedOffHandCopy = 0;
                }
                CloseDescription();
            }
            else if (g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSWInGameGui(0))
            {
                g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
            }
            break;

        case INPUTEVENT_GUI_START:
            if ((m_slot.m_bFlags & SLOTSTATE_SELECTING_ITEM) == 0
                && g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSWInGameGui(0))
            {
                g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
            }
            break;

        case INPUTEVENT_GUI_SCROLL_UP:
            if (m_slot.m_bFlags & SLOTSTATE_SELECTING_ITEM)
                m_lbDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
            break;

#ifdef K1_UI_XBOX  // xbox: RB (INPUTEVENT_GUI_BLACK in menus) changes the character, not Y
        case INPUTEVENT_GUI_BLACK:
#else
        case INPUTEVENT_GUI_Y:
#endif
        case INPUTEVENT_CHANGE_CHARACTER:
            if ((m_slot.m_bFlags & SLOTSTATE_SELECTING_ITEM) == 0)
            {
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
                if (!g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bTutorialModule)
                {
                    g_pAppManager->m_pClientExoApp->ChangeCharacterToNextLivingPartyMember(0, 0, 1);
                    SetCharacter(g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter());
                }
                else
                {
                    int i;
                    for (i = m_slot.m_nShownNPC + 1; i < 9; i++)
                    {
                        if (!g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i))
                            continue;
                        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
                            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(
                                g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1)));
                        if (pCreature == 0)
                            continue;
                        SetCharacter(pCreature);
                        m_slot.m_nShownNPC = static_cast<signed char>(i);
                        break;
                    }
                    if (i >= 9)
                    {
                        m_slot.m_nShownNPC = -1;
                        SetCharacter(g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter());
                    }
                }

                int nSlot = MapGlobalSlotToGuiSlot(m_slot.m_nSlotFlag);
                if (nSlot != 10)
                    SetActiveControl(&m_abtnSlot[nSlot], 0);
                OnEnterSlot(m_pActiveControl);
                UpdatePortraits(1);
            }
        case INPUTEVENT_GUI_SCROLL_DOWN:
            if (m_slot.m_bFlags & SLOTSTATE_SELECTING_ITEM)
                m_lbDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
            break;
        }
    }

    CSWGuiPanel::HandleInputEvent(type, val);
}
#else
void CSWGuiInGameEquip::HandleInputEvent(int type, int val)
{
    if (val != 0)
    {
        switch (type)
        {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
        case INPUTEVENT_GUI:
            if (m_slot.m_bFlags & SLOTSTATE_SELECTING_ITEM)
            {
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);

                // Nothing to commit when the description panel is already blank.
                if (m_slot.m_oidSavedItem != 0)
                {
                    int nSlot = MapGlobalSlotToGuiSlot(m_slot.m_nSlotFlag);

                    // The slot already holds the shown item, so there is nothing to swap.
                    if (m_slot.m_aoidEquipped[nSlot] != static_cast<unsigned long long>(m_slot.m_oidSavedItem))
                    {
                        if (m_slot.m_oidSavedItem != OBJECT_INVALID)
                        {
                            if (ItemExists(m_slot.m_oidSavedItem))
                                EquipItem(m_slot.m_oidSavedItem, m_slot.m_nSlotFlag, 1);
                            else if (m_slot.m_pSavedItemCopy != 0)
                                EquipItem(m_slot.m_pSavedItemCopy, m_slot.m_nSlotFlag, 1);
                        }
                        else
                        {
                            // The "empty" sentinel strips the slot instead of filling it.
                            UnequipItem(static_cast<unsigned int>(m_slot.m_aoidEquipped[
                                MapGlobalSlotToGuiSlot(m_slot.m_nSlotFlag)]), 1);
                        }

                        // A two-handed weapon moves the other hand's item back into it, the same pair
                        // OnItemSelected re-equips.
                        if (m_slot.m_nSlotFlag == CSWInventory::SLOT_RIGHTWEAPON)
                        {
                            if (m_slot.m_oidSavedOffHandItem != 0)
                            {
                                if (ItemExists(m_slot.m_oidSavedOffHandItem))
                                    EquipItem(m_slot.m_oidSavedOffHandItem, CSWInventory::SLOT_LEFTWEAPON, 0);
                                else if (m_slot.m_pSavedOffHandCopy != 0)
                                    EquipItem(m_slot.m_pSavedOffHandCopy, CSWInventory::SLOT_LEFTWEAPON, 0);
                            }
                        }
                        if (m_slot.m_nSlotFlag == CSWInventory::SLOT_RIGHTWEAPON2)
                        {
                            if (m_slot.m_oidSavedOffHandItem != 0)
                            {
                                if (ItemExists(m_slot.m_oidSavedOffHandItem))
                                    EquipItem(m_slot.m_oidSavedOffHandItem, CSWInventory::SLOT_LEFTWEAPON2, 0);
                                else if (m_slot.m_pSavedOffHandCopy != 0)
                                    EquipItem(m_slot.m_pSavedOffHandCopy, CSWInventory::SLOT_LEFTWEAPON2, 0);
                            }
                        }
                    }
                }

                // Drops both per-slot copy objects and re-runs the CloseDescription tail.
                m_slot.m_oidSavedItem = 0;
                if (m_slot.m_pSavedItemCopy != 0) {
                    delete m_slot.m_pSavedItemCopy;
                    m_slot.m_pSavedItemCopy = 0;
                }
                m_slot.m_oidSavedOffHandItem = 0;
                if (m_slot.m_pSavedOffHandCopy != 0) {
                    delete m_slot.m_pSavedOffHandCopy;
                    m_slot.m_pSavedOffHandCopy = 0;
                }
                CloseDescription();
            }
            else if (g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSWInGameGui(0))
            {
                g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
            }
            break;

        case INPUTEVENT_GUI_X:
            if ((m_slot.m_bFlags & SLOTSTATE_SELECTING_ITEM) == 0 && m_slot.m_bNPCOverride == 0)
            {
                if (m_pCreature != 0)
                    m_pCreature->ActionMenuSwitchWeapons(0xffffffff, 0);
                m_slot.m_fSwapWeaponsCooldown = 1.0f;
            }
            break;

#ifndef K2_UI_XBOX  // xbox: RB alone changes the character
        case INPUTEVENT_GUI_Y:
#endif
        case INPUTEVENT_CHANGE_CHARACTER:
            if ((m_slot.m_bFlags & SLOTSTATE_SELECTING_ITEM) == 0)
            {
                m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
                if (m_slot.m_bNPCOverride != 0)
                {
                    OnNextNPCClicked(0);
                }
                else
                {
                    g_pAppManager->m_pClientExoApp->ChangeCharacterToNextLivingPartyMember(0, 0, 1);
                    SetCharacter(
                        g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter());

                    // Arms the new character's slot and tells the in-game GUI to repaint its
                    // character-change buttons. Both run even when the flag maps to no GUI slot.
                    int nSlot = MapGlobalSlotToGuiSlot(m_slot.m_nSlotFlag);
                    if (nSlot != 12)
                        SetActiveControl(&m_abtnSlot[nSlot], 0);
                    OnEnterSlot(m_pActiveControl);
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowGameMenuChangeButtons(1);
                }
            }
            break;

        case INPUTEVENT_GUI_START:
            if ((m_slot.m_bFlags & SLOTSTATE_SELECTING_ITEM) == 0
                && g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSWInGameGui(0))
            {
                g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
            }
            break;

        case INPUTEVENT_GUI_SCROLL_UP:
            if (m_slot.m_bFlags & SLOTSTATE_SELECTING_ITEM)
                m_lbDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
            break;

        case INPUTEVENT_GUI_SCROLL_DOWN:
            if (m_slot.m_bFlags & SLOTSTATE_SELECTING_ITEM)
                m_lbDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
            break;
        }
    }

    CSWGuiPanel::HandleInputEvent(type, val);
}
#endif

int CSWGuiInGameEquip::EquipItem(CSWSItem* pItem, int nSlot, int nFlag) {
#if KOTOR1  // K1: searches the shown character's inventory, not the player's
    CSWSCreature* pServerCreature = m_pCreature->GetServerCreature();
    if (pServerCreature == 0)
        return 0;
#else
    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();
    if (pCreature == 0)
        return 0;

    CSWSCreature* pServerCreature = pCreature->GetServerCreature();
    if (pServerCreature == 0)
        return 0;
#endif

    CItemRepository* pRepository = pServerCreature->GetItemRepository(1);
    if (pRepository->ItemListGetCount() == 0)
        return 0;

    for (unsigned int i = 0; i < pRepository->ItemListGetCount(); i++) {
        CSWSItem* pFound = pRepository->ItemListGetItem(i);
        if (pItem->CompareItem(pFound) != 0)
            return EquipItem(static_cast<unsigned int>(pFound->m_objectId), nSlot, nFlag);
    }
    return 0;
}

int CSWGuiInGameEquip::EquipItem(unsigned int nItemId, int nSlot, int nFlag) {
    CGameObject* pObject = 0;
    unsigned long long oidServer =
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(nItemId);
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidServer, &pObject)
            != CGameObjectArray::SUCCESS)
        return 0;

    if (pObject->AsSWSItem() == 0)
        return 0;

#if KOTOR1
    // K1: no null test on the shown creature, no second-weapon-set fold, its own
    // AddEquipItemActions flags.
    CSWSCreature* pServerCreature = m_pCreature->GetServerCreature();
    if (pServerCreature == 0)
        return 0;

    if ((g_pRules->GetBaseItem(pObject->AsSWSItem()->m_baseItem)->m_nEquipableSlots & nSlot) == 0)
        return 0;

    if (nFlag != 0) {
        pServerCreature->ClearAllActions(1);
        pServerCreature->m_pCombatRound->RemoveAllActions();
    }

    pServerCreature->AddEquipItemActions(pObject->AsSWSItem(), nSlot, 0, 1, 1, 0, nFlag == 0);

    if (pServerCreature->m_bCombatState != 0
        && pServerCreature->m_pCombatRound->m_oidAttackTarget != OBJECT_INVALID) {
        pServerCreature->AddAttackActions(pServerCreature->m_pCombatRound->m_oidAttackTarget,
                                          0, 0, 0, 0, 0, 0, 1, SERVER_ANIMATION_ATTACK,
                                          ATTACK_ANIMATION_TIME_MS, 1, 0, 0, 4, 0);
    }
    return 1;
#else
    if (m_pCreature == 0)
        return 0;

    CSWSCreature* pServerCreature = m_pCreature->GetServerCreature();
    if (pServerCreature == 0)
        return 0;

    unsigned int nWeaponSlot = nSlot;
    if (nWeaponSlot & CSWInventory::SLOT_LEFTWEAPON2)
        nWeaponSlot = CSWInventory::SLOT_LEFTWEAPON;
    if (nWeaponSlot & CSWInventory::SLOT_RIGHTWEAPON2)
        nWeaponSlot = CSWInventory::SLOT_RIGHTWEAPON;
    if ((g_pRules->GetBaseItem(pObject->AsSWSItem()->m_baseItem)->m_nEquipableSlots & nWeaponSlot) == 0)
        return 0;

    if (nFlag != 0) {
        pServerCreature->ClearAllActions(1);
        pServerCreature->m_pCombatRound->RemoveAllActions();
    }

#ifdef K2_FIX_ENGINE_BUGS
    pServerCreature->AddEquipItemActions(pObject->AsSWSItem(), nSlot, nFlag != 0, 1, 0, 0, nFlag == 0, 0);
#else
    pServerCreature->AddEquipItemActions(pObject->AsSWSItem(), nSlot, 1, 1, 0, 0, nFlag == 0, 0);
#endif

    if (pServerCreature->m_bCombatState != 0
        && pServerCreature->m_pCombatRound->m_oidAttackTarget != OBJECT_INVALID) {
        pServerCreature->AddAttackActions(pServerCreature->m_pCombatRound->m_oidAttackTarget,
                                          0, 0, 0, 0, 0, 1, 1, SERVER_ANIMATION_ATTACK,
                                          ATTACK_ANIMATION_TIME_MS, 1, 0, 0, 4, 0);
    }
    return 1;
#endif
}

void CSWGuiInGameEquip::UnequipItem(unsigned int nItemId, int nFlag) {
    unsigned long long oidServer =
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(nItemId);
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidServer);
    if (pItem == 0)
        return;

#if !KOTOR1  // K1: no null test on the shown creature
    if (m_pCreature == 0)
        return;
#endif

    CSWSCreature* pServerCreature = m_pCreature->GetServerCreature();
    if (pServerCreature == 0)
        return;

    if (nFlag != 0) {
        pServerCreature->ClearAllActions(1);
        pServerCreature->m_pCombatRound->RemoveAllActions();
    }

#if KOTOR1
    pServerCreature->AddUnequipActions(pItem, OBJECT_INVALID, 0, 1, 1);
#else
    pServerCreature->AddUnequipActions(pItem, OBJECT_INVALID, 1, 1, 0, 0, 0);
#endif

    CSWCItem* pClientItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(nItemId);
    if (pClientItem != 0)
        pClientItem->PowerItem(0, 0, 0, 0);

    if (pServerCreature->m_bCombatState != 0
        && pServerCreature->m_pCombatRound->m_oidAttackTarget != OBJECT_INVALID) {
#if KOTOR1  // K1: the sixth flag is 0, as in K1's EquipItem
        pServerCreature->AddAttackActions(pServerCreature->m_pCombatRound->m_oidAttackTarget,
                                          0, 0, 0, 0, 0, 0, 1, SERVER_ANIMATION_ATTACK,
                                          ATTACK_ANIMATION_TIME_MS, 1, 0, 0, 4, 0);
#else
        pServerCreature->AddAttackActions(pServerCreature->m_pCombatRound->m_oidAttackTarget,
                                          0, 0, 0, 0, 0, 1, 1, SERVER_ANIMATION_ATTACK,
                                          ATTACK_ANIMATION_TIME_MS, 1, 0, 0, 4, 0);
#endif
    }
}

CSWGuiInGameItemEntry::CSWGuiInGameItemEntry() {
    m_oidItem = OBJECT_INVALID;
    m_nCanUse = CAN_USE_YES;
    bSelected = 0;
    bEquipped = 0;
    bNewItem = 0;
#if KOTOR1 && !defined(K2_UI_TOUCH)
    m_nBoxUnits = 0;
#endif
}

void CSWGuiInGameItemEntry::UpdateItem()
{
    SetItem(m_oidItem, bEquipped, bNewItem);
}

#if KOTOR1
void CSWGuiInGameItemEntry::SetItem(unsigned long long oidItem, int bIsEquipped, int bIsNewItem)
{
    CResRef resRefIcon;
    m_oidItem = oidItem;
    bEquipped = static_cast<unsigned char>(bIsEquipped);
    bNewItem = static_cast<unsigned char>(bIsNewItem);

    if (oidItem == OBJECT_INVALID) {
        m_borderFrame.m_params.SetFillImage(CResRef("lbl_hex_3"), 0);
        m_borderFrameHilight.m_params.SetFillImage(CResRef("lbl_hex_3"), 0);
        m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_TEXT_NONE));
        m_textCount.m_params = CSWGuiTextParams(CExoString(), CResRef("fnt_d16x16"),
                                                CGuiInGame::COLOR_BLUE, SWGUIALIGN_BOTTOM | SWGUIALIGN_CENTER, 1.0f);
        resRefIcon = const_cast<char*>("inone");
    } else {
        CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
        CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
        CSWSItem* pItem =
            pServer->GetItemByGameObjectID(pServer->ClientToServerObjectId(oidItem));
        if (pItem == 0) {
            m_borderFrame.m_params.SetFillImage(CResRef("lbl_hex_3"), 0);
            m_borderFrameHilight.m_params.SetFillImage(CResRef("lbl_hex_3"), 0);
            m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_TEXT_NONE));
            m_textCount.m_params = CSWGuiTextParams(CExoString(), CResRef("fnt_d16x16"),
                                                    CGuiInGame::COLOR_BLUE, SWGUIALIGN_BOTTOM | SWGUIALIGN_CENTER, 1.0f);
            resRefIcon = const_cast<char*>("inone");
        } else {
            CExoString sName;
            pItem->m_localizedName.GetString(pClient->GetClientLanguage(), &sName, 0);
            if (bIsEquipped != 0)
                sName = sName + " (" + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_EQUIPPED) + ")";
            if (bIsNewItem != 0) {
                m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_PURPLE);
                m_border.m_params.m_fAlpha = 0.5f;
                m_borderFrame.m_params.m_fAlpha = 0.5f;
                m_borderFrame.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_PURPLE);
                m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_PURPLE);
                m_borderFrameHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_PURPLE);
            } else {
                m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
                m_borderFrame.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
                m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
                m_borderFrameHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
            }
            m_text.m_params.SetText(sName);
            CExoString sCount;
            int nStack = pItem->m_stackCount;
            if (nStack >= 2) {
                if (nStack >= 100) {
                    m_borderFrame.m_params.SetFillImage(CResRef("lbl_hex_7"), 0);
                    m_borderFrameHilight.m_params.SetFillImage(CResRef("lbl_hex_7"), 0);
                    sCount.Format(const_cast<char*>("%d"), nStack);
                } else {
                    m_borderFrame.m_params.SetFillImage(CResRef("lbl_hex_6"), 0);
                    m_borderFrameHilight.m_params.SetFillImage(CResRef("lbl_hex_6"), 0);
                    sCount.Format(const_cast<char*>("%d"), nStack);
                }
            } else {
                m_borderFrame.m_params.SetFillImage(CResRef("lbl_hex_3"), 0);
                m_borderFrameHilight.m_params.SetFillImage(CResRef("lbl_hex_3"), 0);
            }
            m_textCount.m_params = CSWGuiTextParams(sCount, CResRef("fnt_d16x16"),
                                                    CGuiInGame::COLOR_BLUE, SWGUIALIGN_BOTTOM | SWGUIALIGN_CENTER, 1.0f);
            resRefIcon = pItem->GetIcon();
        }
    }

    m_borderIcon.m_params.m_flags.nFillStyle = 2;
    m_borderIcon.m_params.SetFillImage(resRefIcon, 0);
#ifndef K2_UI_TOUCH
    // The new count takes the row's layout, right-aligned at the icon's corner, its font at the
    // layout's scale. This also applies to a row already laid out.
    CSWGuiExtent extent = m_extent;
    SetExtent(extent);
#endif
}
#else
void CSWGuiInGameItemEntry::SetItem(unsigned long long oidItem, int bIsEquipped, int bIsNewItem)
{
    CResRef resRefIcon;
    m_oidItem = oidItem;
    bEquipped = static_cast<unsigned char>(bIsEquipped);
    bNewItem = static_cast<unsigned char>(bIsNewItem);

    if (oidItem == OBJECT_INVALID) {
        m_borderFrame.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM1"), 0);
        m_borderFrameHilight.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM1"), 0);
        m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_TEXT_NONE));
        m_textCount.m_params = CSWGuiTextParams(CExoString(), CResRef("fnt_d16x16"),
                                                CGuiInGame::COLOR_NEW_TEXT, SWGUIALIGN_BOTTOM | SWGUIALIGN_CENTER, 1.0f);
        resRefIcon = const_cast<char*>("inone");
    } else {
        CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
        CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
        CSWSItem* pItem =
            pServer->GetItemByGameObjectID(pServer->ClientToServerObjectId(oidItem));
        if (pItem == 0) {
            m_borderFrame.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM1"), 0);
            m_borderFrameHilight.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM1"), 0);
            m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_TEXT_NONE));
            m_textCount.m_params = CSWGuiTextParams(CExoString(), CResRef("fnt_d16x16"),
                                                    CGuiInGame::COLOR_NEW_TEXT, SWGUIALIGN_BOTTOM | SWGUIALIGN_CENTER, 1.0f);
            resRefIcon = const_cast<char*>("inone");
        } else {
            CExoString sName;
            pItem->m_localizedName.GetString(pClient->GetClientLanguage(), &sName, 0);
            // A '<' or '{' in the name means it is a TLK token string; resolve it through the client
            // talk table with the party leader as the speaker.
            if (sName.Find('<', 0) != -1 || sName.Find('{', 0) != -1) {
                CSWCCreature* pPC = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
                g_pSWTlkTable->ParseStr(sName, pPC, 1, 1);
            }
            if (bIsEquipped != 0)
                sName = sName + " (" + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_EQUIPPED) + ")";
            // The third argument is the "item is an upgrade or bonus" flag.
            if (bIsNewItem != 0) {
                m_border.m_params.m_color = CGuiInGame::COLOR_NEW_BONUS;
                m_borderFrame.m_params.m_color = CGuiInGame::COLOR_NEW_BONUS;
                m_borderHilight.m_params.m_color = CGuiInGame::COLOR_NEW_BONUS;
                m_borderFrameHilight.m_params.m_color = CGuiInGame::COLOR_NEW_BONUS;
                m_border.m_params.m_fAlpha = 0.5f;
                m_borderFrame.m_params.m_fAlpha = 0.5f;
            } else {
                m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
                m_borderFrame.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
                m_borderHilight.m_params.m_color =
                    *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_HIGHLIGHT);
                m_borderFrameHilight.m_params.m_color =
                    *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_HIGHLIGHT);
                m_border.m_params.m_fAlpha = 1.0f;
                m_borderFrame.m_params.m_fAlpha = 1.0f;
            }
            m_text.m_params.SetText(sName);
            CExoString sCount;
            int nStack = pItem->m_stackCount;
            if (nStack >= 2) {
                if (nStack >= 100) {
                    m_borderFrame.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM3"), 0);
                    m_borderFrameHilight.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM3"), 0);
                    sCount.Format(const_cast<char*>("%d"), nStack);
                } else {
                    m_borderFrame.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM2"), 0);
                    m_borderFrameHilight.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM2"), 0);
                    sCount.Format(const_cast<char*>("%d"), nStack);
                }
            } else {
                m_borderFrame.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM1"), 0);
                m_borderFrameHilight.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM1"), 0);
            }
            m_textCount.m_params = CSWGuiTextParams(sCount, CResRef("fnt_d16x16"),
                                                    CGuiInGame::COLOR_NEW_TEXT, SWGUIALIGN_BOTTOM | SWGUIALIGN_CENTER, 1.0f);
            resRefIcon = pItem->GetIcon();
        }
    }

    // A stretch fill on the always-drawn frame, then the item's own icon.
    SetExtent(m_extent);
    m_borderIcon.m_params.m_flags.nFillStyle = 2;
    m_borderIcon.m_params.SetFillImage(resRefIcon, 0);
}
#endif

void CSWGuiInGameItemEntry::SetCanUse(unsigned char nCanUse) {
    switch (nCanUse) {
    case CAN_USE_YES:
    case CAN_USE_EQUIPPED:
#if KOTOR1  // K1: blue / yellow
        m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
        m_borderFrame.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
        m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
        m_borderFrameHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
#else
        m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
        m_borderFrame.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
        m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_HIGHLIGHT);
        m_borderFrameHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_HIGHLIGHT);
#endif
        break;
    case CAN_USE_MISSING_PREREQUISITES:
    case CAN_USE_INCOMPATIBLE_OFF_HAND:
        m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_RED);
        m_borderFrame.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_RED);
        m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_RED);
        m_borderFrameHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_RED);
        break;
    }

    m_borderHilight.m_params.SetPulsingAlpha(bSelected, 1, 0);
    m_borderFrameHilight.m_params.SetPulsingAlpha(bSelected, 0, 1);
    m_nCanUse = nCanUse;
}

void CSWGuiInGameItemEntry::Draw(float f) {
    if (!m_tail.bActive) {
        m_border.Draw(f);
        m_borderFrame.Draw(f);
    } else {
        m_borderHilight.Draw(f);
        m_borderFrameHilight.Draw(f);
    }
    m_borderIcon.Draw(f);
    m_text.Draw(f);
    m_textCount.Draw(f);
}

int CSWGuiInGameItemEntry::GetNumItems() {
    int nNumItems;
    CExoString sCount = m_text.m_params.m_sText;
    if (sCount.IsEmpty())
        nNumItems = 1;
    else
        nNumItems = sCount.AsINT();
    return nNumItems;
}

void CSWGuiInGameItemEntry::Initialize(CSWGuiText& text, CSWGuiBorder& borderNormal,
                                       CSWGuiBorder& borderHilight, int nWidth, int bSelected_)
{
#if KOTOR1
    // K1: blue / yellow frames on lbl_hex_3; the row is 56 of 768 design lines high.
    m_text.m_params = text.m_params;
    CSWGuiBorderParams& paramsNormal = m_border.m_params;
    paramsNormal = borderNormal.m_params;
    m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    CSWGuiBorderParams& paramsHilight = m_borderHilight.m_params;
    paramsHilight = borderHilight.m_params;
    m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
    m_borderHilight.m_params.SetPulsingAlpha(bSelected_, 1, 0);
    CSWGuiBorderParams& paramsFrame = m_borderFrame.m_params;
    paramsFrame = CSWGuiBorderParams("", "", "lbl_hex_3", 0, 0, 0, 1.0f,
                                     Vector(1.0f, 1.0f, 1.0f), 0, 0);
    m_borderFrame.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    CSWGuiBorderParams& paramsFrameHilight = m_borderFrameHilight.m_params;
    paramsFrameHilight = CSWGuiBorderParams("", "", "lbl_hex_3", 0, 0, 0, 1.0f,
                                            Vector(1.0f, 1.0f, 1.0f), 0, 0);
    m_borderFrameHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
    m_borderFrameHilight.m_params.SetPulsingAlpha(bSelected_, 0, 1);
#if !defined(K2_UI_TOUCH) || defined(K2_FIX_ENGINE_BUGS)
    // The frame squares are drawn larger than the 64 px source on a large screen, so stretch
    // rather than tile.
    m_borderFrame.m_params.m_flags.nFillStyle = 2;
    m_borderFrameHilight.m_params.m_flags.nFillStyle = 2;
#endif
#ifndef K2_UI_TOUCH
    // The 56 units are the 480-line PC layout's row height; the touch build uses 768.
    m_nBoxUnits = 0;
    CSWGuiExtent extent(0, 0, nWidth, static_cast<int>(static_cast<float>(g_SDL_mode.h) / 480.0f * 56.0f));
#else
    CSWGuiExtent extent(0, 0, nWidth, static_cast<int>(g_SDL_mode.h / 768.0f * 56.0f));
#endif
    SetExtent(extent);
    bSelected = static_cast<unsigned char>(bSelected_);
#else
    m_text.m_params = text.m_params;
    CSWGuiBorderParams& paramsNormal = m_border.m_params;
    paramsNormal = borderNormal.m_params;
    m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
    CSWGuiBorderParams& paramsHilight = m_borderHilight.m_params;
    paramsHilight = borderHilight.m_params;
    m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_HIGHLIGHT);
    m_borderHilight.m_params.SetPulsingAlpha(bSelected_, 1, 0);
    CSWGuiBorderParams& paramsFrame = m_borderFrame.m_params;
    paramsFrame = CSWGuiBorderParams("", "", "UIBIT_EQP_ITM1", 0, 0, 0, 1.0f,
                                     Vector(1.0f, 1.0f, 1.0f), 0, 0);
    m_borderFrame.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_GREEN);
    CSWGuiBorderParams& paramsFrameHilight = m_borderFrameHilight.m_params;
    paramsFrameHilight = CSWGuiBorderParams("", "", "UIBIT_EQP_ITM1", 0, 0, 0, 1.0f,
                                            Vector(1.0f, 1.0f, 1.0f), 0, 0);
    m_borderFrameHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_HIGHLIGHT);
    m_borderFrameHilight.m_params.SetPulsingAlpha(bSelected_, 0, 1);
#if !defined(K2_UI_TOUCH) || defined(K2_FIX_ENGINE_BUGS)
    // The frame squares are drawn larger than the source bitmap on a large screen, so stretch
    // rather than tile.
    m_borderFrame.m_params.m_flags.nFillStyle = 2;
    m_borderFrameHilight.m_params.m_flags.nFillStyle = 2;
#endif
    CSWGuiExtent extent(0, 0, nWidth, static_cast<int>(GetResolutionTextScalar() * 36.0f));
    SetExtent(extent);
    bSelected = static_cast<unsigned char>(bSelected_);
#endif
}

void CSWGuiInGameItemEntry::SetExtent(const CSWGuiExtent& extent)
{
#if KOTOR1
    // K1: a 56-of-768-line icon square, the count text sized to its own ideal width at the
    // icon's bottom-right; on an iPhone the caption string is rescaled.
#ifndef K2_UI_TOUCH
    // The 480-line PC and console layouts' units, like the row height above.
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 480.0f;
#else
    static float s_fScale = g_SDL_mode.h / 480.0f;
#endif
#else
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = g_SDL_mode.h / 768.0f;
#else
    static float s_fScale = g_SDL_mode.h / 768.0f;
#endif
#endif
    if (static_cast<float>(extent.w) <= s_fScale * 56.0f)
        return;
    CSWGuiObject::SetExtent(extent);

    int nX = m_extent.x;
    int nY = m_extent.y;
    int nIcon = static_cast<int>(s_fScale * 56.0f);
#ifndef K2_UI_TOUCH
    // A shorter caption box sits centred in the row the list stretched, with the icon centred on it.
    int nBox = nIcon;
    if (m_nBoxUnits != 0) {
        nBox = static_cast<int>(s_fScale * m_nBoxUnits);
        nY += (m_extent.h - nBox) / 2;
        nY -= (nIcon - nBox) / 2;
    }
#endif
    CSWGuiExtent extentIcon;
    extentIcon.x = nX;
    extentIcon.y = nY;
    extentIcon.w = nIcon;
    extentIcon.h = nIcon;
    m_borderFrame.m_extent = extentIcon;
    m_borderFrameHilight.m_extent = extentIcon;
    m_borderIcon.m_extent = extentIcon;

    float fPad = m_textCount.m_params.m_sText.GetLength() > 1 ? 4.0f : 2.0f;
    fPad = s_fScale * fPad;
#ifndef K2_UI_TOUCH
    // Lay the count out at the PC's proportions, its font 1:1 with the 480-line layout, in a box
    // the icon's width, right- and bottom-aligned at the icon's corner.
    if (m_textCount.m_pString != 0)
        m_textCount.m_pString->SetFontScale(s_fScale);
    m_textCount.m_params.SetAlignment(36);
    CSWGuiExtent extentText;
    extentText.x = nX;
    extentText.y = static_cast<int>(s_fScale * 32.0f + static_cast<float>(nY));
    extentText.w = static_cast<int>(s_fScale * 56.0f - fPad * 0.5f);
    extentText.h = static_cast<int>(s_fScale * 24.0f);
    m_textCount.SetExtent(extentText);
    if (m_textCount.m_pString != 0)
        m_textCount.m_pString->SetFontScale(s_fScale);
#else
    CSWGuiExtent ideal = m_textCount.m_params.m_pTextObject->GetIdealWidthAndHeight();
    CSWGuiExtent extentText;
    extentText.x = static_cast<int>(fPad + (s_fScale * 56.0f - static_cast<float>(ideal.w)) + static_cast<float>(nX));
    extentText.y = static_cast<int>(s_fScale * 32.0f + static_cast<float>(nY));
    extentText.w = ideal.w;
    extentText.h = static_cast<int>(s_fScale * 24.0f);
    m_textCount.SetExtent(extentText);
#endif

    extentIcon.x = static_cast<int>(s_fScale * 56.0f + static_cast<float>(nX));
    extentIcon.w = static_cast<int>(static_cast<float>(m_extent.w) - s_fScale * 56.0f);
#ifndef K2_UI_TOUCH
    if (m_nBoxUnits != 0) {
        extentIcon.y = nY + (nIcon - nBox) / 2;
        extentIcon.h = nBox;
    }
#endif
    m_border.m_extent = extentIcon;
    m_borderHilight.m_extent = extentIcon;
    m_text.SetExtent(CSWGuiExtent::Intersection(m_border.GetInnerExtent(),
                                                m_borderHilight.GetInnerExtent()));
#ifdef K2_UI_TOUCH  // the phone caption size; pc / xbox keep the 1.3x string default
    int nScreenW, nScreenH;
    if (IosIsIphone(nScreenW, nScreenH) && m_text.m_pString != 0)
        m_text.m_pString->SetFontScale(s_fScale);
#endif
#else
    if (extent.w < 37)
        return;
    m_extent = extent;

    int nX = m_extent.x;
    int nY = m_extent.y;
    float fScale = GetResolutionTextScalar();
    float fIcon = fScale * 36.0f;
#ifdef K2_QOL_IMPROVEMENTS
    // A row the Text Size grew gets an icon and frames as tall as itself; at the fixed 36 they sat
    // in a gap under a large caption.
    if (static_cast<float>(m_extent.h) > fIcon)
        fIcon = static_cast<float>(m_extent.h);
#endif
    int nIcon = static_cast<int>(fIcon);

    CSWGuiExtent extentIcon;
    extentIcon.x = nX;
    extentIcon.y = nY;
    extentIcon.w = nIcon;
    extentIcon.h = nIcon;
    m_borderFrame.m_extent = extentIcon;
    m_borderFrameHilight.m_extent = extentIcon;
    m_borderIcon.m_extent = extentIcon;

    CSWGuiExtent extentText = extentIcon;
    // The count string's presence picks the label's line height (55 or 21) and its alignment.
    if (m_textCount.m_params.m_sText.GetLength() > 2) {
        extentText.w = 55;
        m_textCount.m_params.SetAlignment(36);
    } else {
        extentText.w = 21;
        m_textCount.m_params.SetAlignment(34);
    }
    float fLine = fScale * static_cast<float>(extentText.w);
    extentText.x = static_cast<int>((fIcon - fLine) + static_cast<float>(extentText.x));
    extentText.y = static_cast<int>((fIcon - fScale * 19.0f) + static_cast<float>(extentText.y));
    extentText.h = 19;
    extentText.w = static_cast<int>(fLine + 1.0f);
    extentText.h = static_cast<int>(static_cast<float>(extentText.h) * fScale + 1.0f);
#ifdef K2_QOL_IMPROVEMENTS
    // The stack count grows with the icon it sits on, its text to fit the grown box.
    K2QolGrowItemCount(m_textCount, extentText, fIcon / (fScale * 36.0f));
#endif
    m_textCount.SetExtent(extentText);

    // extentIcon is reused for the name's border: x moves right by the icon, w shrinks by it.
    extentIcon.x = static_cast<int>(static_cast<float>(extentIcon.x) + fIcon);
    extentIcon.w = static_cast<int>(static_cast<float>(m_extent.w) - fIcon);
    m_border.m_extent = extentIcon;
    m_borderHilight.m_extent = extentIcon;
    m_text.SetExtent(CSWGuiExtent::Intersection(m_border.GetInnerExtent(),
                                                m_borderHilight.GetInnerExtent()));
#endif
}

extern int g_bShiftActionState;
bool IsJoystickConnected();
extern int gDontMoveCursor_Mac;

void CSWGuiContainer::Update(float f)
{
    K2_UNUSED(f);
    if (bGiveArmed) {
        bGiveArmed = 0;
    } else if (m_nState == 2) {
        SetupGiveMode(1);
        m_nState &= ~2;
    }
}

void CSWGuiContainer::OnPanelRemoved()
{
    g_pGuiMan->RestoreOldMousePosition();
    CSWGuiPanel::OnPanelRemoved();
}

// EFFECTIVE: the two centre sums load m_extent before the button extent and issue g_pGuiMan's GOT load
// first in the orig (registers rename); same loads and calls, armv7 exact.
void CSWGuiContainer::OnPanelAdded()
{
#if !KOTOR1  // K1: an empty override (no cursor parking, not even the base call)
    if (IsJoystickConnected())
        return;
    if (gDontMoveCursor_Mac != 0)
        return;

    int nX = m_extent.x + m_btnOk.m_extent.x + m_btnOk.m_extent.w / 2;
    int nY = m_extent.y + m_btnOk.m_extent.y + m_btnOk.m_extent.h / 2;
    g_pGuiMan->StoreCurrentMousePosition();
    g_pGuiMan->MoveMouseToPosition(nX, nY);
    CSWGuiPanel::OnPanelAdded();
#endif
}

CSWGuiContainer::~CSWGuiContainer()
{
}

void CSWGuiContainer::HandleInputEvent(int nType, int nVal)
{
    if (nVal != 0) {
        switch (nType) {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_BACK:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            g_pAppManager->m_pClientExoApp->GetSWCMessage()
                ->SendPlayerToServerGuiContainer_Close(m_oidContainer, 0);
            g_pAppManager->m_pClientExoApp->GetInGameGui()->HideContainerGui();
            break;
        case INPUTEVENT_GUI_X:
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            if (m_nState & 1)
                SetupGiveMode(0);
            else
                SetContainer(m_oidContainer, m_bHasItems);
            break;
        case INPUTEVENT_GUI_START:
            if (!(m_nState & 1))
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            // fall through: the first event is the sound prologue, the second the sound plus the
            // close.
        case INPUTEVENT_GUI_A:
            if (m_nState & 1) {
                int bCancelled = (m_bHasItems == 1);
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                g_pAppManager->m_pClientExoApp->GetSWCMessage()
                    ->SendPlayerToServerGuiContainer_Close(m_oidContainer, bCancelled);
                g_pAppManager->m_pClientExoApp->GetInGameGui()->HideContainerGui();
            }
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(nType, nVal);
}

void CSWGuiContainer::GiveItem(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    if (pControl->m_tail.nCurrentEventType != 0)
        m_pGuiManager->PlayGuiSound(pControl->m_tail.m_nClickSound);

    CSWGuiInGameItemEntry* pEntry = static_cast<CSWGuiInGameItemEntry*>(pControl);

    int nAmount = 1;
    if (g_bShiftActionState)
        nAmount = pEntry->GetNumItems();

    g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter()
        ->PerformGiveItem(pEntry->m_oidItem, m_oidContainer, nAmount);

    m_nState |= 2;
    bGiveArmed = 1;
}

void CSWGuiContainer::SetupGiveMode(int bSelect)
{
    CExoArrayList<CSWGuiControl*> aControls;
    CSWGuiLabelHilight* pProto = static_cast<CSWGuiLabelHilight*>(m_lstItems.m_pProtoItem);
    CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
    CItemRepository* pRepo = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems();

    if (pRepo->ItemListGetCount() != 0) {
        int nWidth = m_lstItems.m_innerExtent.w - m_lstItems.m_nPadding * 2;
        int nRow = 0;
        for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++) {
            unsigned long long oid;
            CSWSItem* pItem;
            oid = pClient->ServerToClientObjectId(pRepo->ItemListGetItemObjectID(i));
            pItem = pRepo->ItemListGetItem(i);
            if (pItem == 0)
                continue;
            if (pItem->m_bPlot != 0)
                continue;

            // nRow counts the rows actually added, so the `continue`s make it lag the repository index.
            aControls.Add(new CSWGuiInGameItemEntry());
            static_cast<CSWGuiInGameItemEntry*>(aControls.m_pArray[nRow])
                ->Initialize(pProto->m_text, pProto->m_border, pProto->m_hilightBorder,
                             nWidth, 1);
            static_cast<CSWGuiInGameItemEntry*>(aControls.m_pArray[nRow])->SetItem(oid, 0, 0);
            static_cast<CSWGuiInGameItemEntry*>(aControls.m_pArray[nRow])->AddEvent(INPUTEVENT_GUI_A, this,
                static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiContainer::GiveItem));
            static_cast<CSWGuiInGameItemEntry*>(aControls.m_pArray[nRow])->AddEvent(INPUTEVENT_GUI_START, this,
                static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiContainer::GiveItem));
            nRow++;
        }
    }

    CAppManager* pApp = g_pAppManager;
    CExoString sTitle = pApp->m_pClientExoApp->GetGUIString(STRREF_SWITCH_TO) + CExoString(" ")
                      + pApp->m_pClientExoApp->GetGUIString(STRREF_GET_ITEMS);  // Get Items here, Give Items on the buttons
    m_lblMessage.m_text.m_params.SetStrRef(STRREF_ITEMS_TO_PLACE_IN_CONTAINER);
    m_lblOk.m_text.m_params.SetStrRef(STRREF_GIVE_ITEMS);
    m_btnOk.m_text.m_params.SetStrRef(STRREF_GIVE_ITEMS);
    m_btnGiveItems.m_text.m_params.SetText(sTitle);

    m_nState &= ~1;
    // Cleared here and set by SetContainer; the two modes want opposite selection behaviour.
    m_lstItems.bNoSelection = 0;
    m_lstItems.SetEnabled(1);

    int nSelect = 0;
    if (bSelect != 0) {
        int n = aControls.m_count - 1;
        if (n >= 0)
            nSelect = n > m_lstItems.m_nSelectedIndex ? m_lstItems.m_nSelectedIndex : n;
    }

    m_lstItems.AddControls(aControls, 1, 0, 0);
    SetActiveControl(0, 0);
    SetActiveControl(&m_lstItems, 0);
    if (aControls.m_count > 0)
        m_lstItems.SetSelectedControl(nSelect, 0);
#if KOTOR1  // K1: the port's glyphs, X first
    IosAddButtonIcon(&m_btnGiveItems, 'x');
    IosAddButtonIcon(&m_btnOk, 'a');
#else
    AspyrAddButtonIcon(&m_btnOk, 'a');
    AspyrAddButtonIcon(&m_btnGiveItems, 'x');
#endif
}

void CSWGuiContainer::SetContainer(unsigned long long objId, int flag)
{
    m_oidContainer = objId;
    m_bHasItems = static_cast<unsigned char>(flag);

    CExoArrayList<CSWGuiControl*> aControls;
    CSWGuiLabelHilight* pProto = static_cast<CSWGuiLabelHilight*>(m_lstItems.m_pProtoItem);
    CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    CSWSPlaceable* pPlaceable =
        pServer->GetPlaceableByGameObjectID(pServer->ClientToServerObjectId(objId));
    CItemRepository* pRepo = pPlaceable->m_pItemRepository;

    aControls.SetSize(pRepo->ItemListGetCount());
    int nWidth = m_lstItems.m_innerExtent.w - m_lstItems.m_nPadding * 2;
    if (pRepo->ItemListGetCount() != 0) {
        for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++) {
            unsigned long long oid =
                pClient->ServerToClientObjectId(pRepo->ItemListGetItemObjectID(i));

            aControls.m_pArray[i] = new CSWGuiInGameItemEntry();
            static_cast<CSWGuiInGameItemEntry*>(aControls.m_pArray[i])
                ->Initialize(pProto->m_text, pProto->m_border, pProto->m_hilightBorder, nWidth, 0);
            static_cast<CSWGuiInGameItemEntry*>(aControls.m_pArray[i])->SetItem(oid, 0, 0);
        }
    }

    m_lstItems.AddControls(aControls, 1, 0, 0);

#if KOTOR1  // K1: no search-style placeables, so only the two captions
    uint64_t nMsgStrRef = m_bHasItems == 0 ? STRREF_CONTAINER_EMPTY : STRREF_CONTAINER_INVENTORY;
#else
    // A body bag's own flag answers the "search" question directly; only a plain placeable probes
    // placeables.2da's "UseSearch" column, whose value lands in the out-parameter rather than the
    // return.
    int bSearch = pPlaceable->m_bIsBodyBag;
    if (bSearch == 0)
        g_pRules->m_p2DArrays->m_pPlaceables->GetINTEntry(
            pPlaceable->m_appearanceType, CExoString("UseSearch"), &bSearch);

    uint64_t nMsgStrRef = STRREF_CONTAINER_EMPTY;
    if (m_bHasItems != 0 || bSearch != 0)
        nMsgStrRef = m_bHasItems == 0 && bSearch != 0 ? STRREF_TARGET_INVENTORY_EMPTY : STRREF_CONTAINER_INVENTORY;
#endif

    CExoString sTitle = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SWITCH_TO) + CExoString(" ")
                      + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_GIVE_ITEMS);
    m_lblMessage.m_text.m_params.SetStrRef(nMsgStrRef);
    m_lblOk.m_text.m_params.SetStrRef(STRREF_GET_ITEMS);
    m_btnOk.m_text.m_params.SetStrRef(STRREF_GET_ITEMS);
    m_btnGiveItems.m_text.m_params.SetText(sTitle);

    m_nState |= 1;
    m_lstItems.bNoSelection = 1;
    m_lstItems.SetEnabled(0);
    SetActiveControl(0, 0);
    SetActiveControl(&m_lstItems, 0);
#if KOTOR1
    IosAddButtonIcon(&m_btnGiveItems, 'x');
    IosAddButtonIcon(&m_btnOk, 'a');
#else
    AspyrAddButtonIcon(&m_btnOk, 'a');
    AspyrAddButtonIcon(&m_btnGiveItems, 'x');
#endif
}

CSWGuiContainer::CSWGuiContainer(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
#if KOTOR1
    // K1: "container", rescaled once for the resolution, no button growth; the glyph goes on before
    // the panel is centred.
    StartLoadFromLayout(CResRef("container"));
    InitControl(m_lblMessage, CExoString("LBL_MESSAGE"), 1);
    InitControl(m_lblButton, CExoString("LBL_BUTTON"), 1);
    InitControl(m_lblOk, CExoString("LBL_OK"), 1);
    InitControl(m_lstItems, CExoString("LB_ITEMS"), 1);
    InitControl(m_lblB, CExoString("LBL_B"), 1);
    InitControl(m_lblX, CExoString("LBL_X"), 1);
    InitControl(m_lblCancel, CExoString("LBL_CANCEL"), 1);
    InitControl(m_btnOk, CExoString("BTN_OK"), 1);
    InitControl(m_btnGiveItems, CExoString("BTN_GIVEITEMS"), 1);
    InitControl(m_btnCancel, CExoString("BTN_CANCEL"), 1);
    StopLoadFromLayout();
    ScaleExtentForResolution();

    m_nState = 1;
    bGiveArmed = 0;
    m_lstItems.bNoSelection = 1;
    m_lstItems.SetEnabled(0);
    m_lstItems.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    m_btnOk.m_tail.bFocusOnHover = 0;
    m_btnOk.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiContainer::OnAButtonPressed));
    m_btnGiveItems.m_tail.bFocusOnHover = 0;
    m_btnGiveItems.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiContainer::OnXButtonPressed));
    m_btnCancel.m_tail.bFocusOnHover = 0;
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiContainer::OnBButtonPressed));
    IosAddButtonIcon(&m_btnCancel, 'b');
    CenterPanel();
#else
    StartLoadFromLayout(CResRef("container_p"), 0, 1, 1);
    m_stateFlags |= 1;

#ifdef K2_QOL_IMPROVEMENTS
    // The title line, either "Container" or the give-items prompt, sits in a fixed box, so fit it
    // to the Text Size within that box.
    K2QolMarkFitText(&m_lblMessage.m_text);
#endif
    InitControl(m_lblMessage, CExoString("LBL_MESSAGE"), 1, 1, 0, 0, 0);
    InitControl(m_lblButton, CExoString("LBL_BUTTON"), 1, 1, 0, 0, 0);
    InitControl(m_lblOk, CExoString("LBL_OK"), 1, 1, 0, 0, 0);
    InitControl(m_lstItems, CExoString("LB_ITEMS"), 1, 1, 0, 0, 0);
    InitControl(m_lblB, CExoString("LBL_B"), 1, 1, 0, 0, 0);
    InitControl(m_lblX, CExoString("LBL_X"), 1, 1, 0, 0, 0);
    InitControl(m_lblCancel, CExoString("LBL_CANCEL"), 1, 1, 0, 0, 0);
    InitControl(m_btnOk, CExoString("BTN_OK"), 1, 1, 0, 0, 0);
    InitControl(m_btnGiveItems, CExoString("BTN_GIVEITEMS"), 1, 1, 0, 0, 0);
    InitControl(m_btnCancel, CExoString("BTN_CANCEL"), 1, 1, 0, 0, 0);

    float fScale = (GetResolutionTextScalar() - 1.0f) * 0.2f + 1.0f;

    CSWGuiExtent extent = m_btnOk.m_extent;
    float fOldHOk = static_cast<float>(extent.h);
    int nNewHOk = static_cast<int>(fScale * fOldHOk);
    extent.h = nNewHOk;
    m_btnOk.SetExtent(extent);

    extent = m_btnGiveItems.m_extent;
    float fOldH = static_cast<float>(extent.h);
    extent.h = static_cast<int>(fScale * fOldH);
    float fGrow = static_cast<float>(extent.h) - fOldH;
    extent.y = static_cast<int>(fGrow + static_cast<float>(extent.y));
    m_btnGiveItems.SetExtent(extent);

    extent = m_btnCancel.m_extent;
    fOldH = static_cast<float>(extent.h);
    extent.h = static_cast<int>(fScale * fOldH);
    fGrow += static_cast<float>(extent.h) - fOldH;
    extent.y = static_cast<int>(fGrow + static_cast<float>(extent.y));
    m_btnCancel.SetExtent(extent);

    // The member sits inside the float sum, so this assigns from the float expression.
    m_extent.h = static_cast<int>(static_cast<float>(nNewHOk) - fOldHOk + fGrow + static_cast<float>(m_extent.h));

    StopLoadFromLayout();

    m_nState = 1;
    bGiveArmed = 0;
    m_lstItems.bNoSelection = 1;
    m_lstItems.SetEnabled(0);
    m_lstItems.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    m_btnOk.m_tail.bFocusOnHover = 0;
    m_btnOk.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiContainer::OnAButtonPressed));
    m_btnGiveItems.m_tail.bFocusOnHover = 0;
    m_btnGiveItems.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiContainer::OnXButtonPressed));
    m_btnCancel.m_tail.bFocusOnHover = 0;
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiContainer::OnBButtonPressed));

    m_extent.x = 0;
    m_extent.y = 0;
    CenterPanel();
    AspyrAddButtonIcon(&m_btnCancel, 'b');
#endif
}

void CSWGuiInGameItemEntry::Initialize(const CSWGuiExtent&, const CSWGuiTextParams&,
                                       const CSWGuiBorderParams&, const CSWGuiBorderParams&)
{
}

void CSWGuiInGameItemEntry::Initialize(const CSWGuiExtent&, CSWGuiButton*)
{
}

float GetResolutionTextScalar();

CSWGuiStoreItemEntry::CSWGuiStoreItemEntry()
{
    m_oidItem = OBJECT_INVALID;
#if !KOTOR1  // K1: the entry ends after m_textCount
    m_nCreateRow = -1;
    m_bCanCreate = 0;
#endif
}

void CSWGuiStoreItemEntry::Draw(float fAlpha)
{
    if (m_tail.bActive) {
        m_borderHilight.Draw(fAlpha);
        m_borderFrameHilight.Draw(fAlpha);
    } else {
        m_border.Draw(fAlpha);
        m_borderFrame.Draw(fAlpha);
    }
    m_borderIcon.Draw(fAlpha);
    m_text.Draw(fAlpha);
    m_textCount.Draw(fAlpha);
}

void CSWGuiStoreItemEntry::SetExtent(const CSWGuiExtent& extent)
{
#if KOTOR1
    // K1: the CSWGuiInGameItemEntry layout (56-line icon, count at its ideal width).
#ifndef K2_UI_TOUCH
    float fScale = static_cast<float>(g_SDL_mode.h) / 480.0f;   // pc / xbox: the 480-line layout's units
#else
    float fScale = g_SDL_mode.h / 768.0f;
#endif
    if (static_cast<float>(extent.w) <= fScale * 56.0f)
        return;
    float fIcon = fScale * 56.0f;
    CSWGuiObject::SetExtent(extent);

    int nX = m_extent.x;
    int nY = m_extent.y;
    int nIcon = static_cast<int>(fIcon);
    CSWGuiExtent extentIcon;
    extentIcon.x = nX;
    extentIcon.y = nY;
    extentIcon.w = nIcon;
    extentIcon.h = nIcon;
    m_borderFrame.m_extent = extentIcon;
    m_borderFrameHilight.m_extent = extentIcon;
    m_borderIcon.m_extent = extentIcon;

    float fPad = fScale * (m_textCount.m_params.m_sText.GetLength() > 1 ? 4.0f : 2.0f);
#ifndef K2_UI_TOUCH
    // The count at the PC's proportions, as in the item entry's own SetExtent.
    if (m_textCount.m_pString != 0)
        m_textCount.m_pString->SetFontScale(fScale);
    m_textCount.m_params.SetAlignment(36);
    CSWGuiExtent extentText;
    extentText.x = nX;
    extentText.y = static_cast<int>(fScale * 32.0f + static_cast<float>(nY));
    extentText.w = static_cast<int>(fIcon - fPad * 0.5f);
    extentText.h = static_cast<int>(fScale * 24.0f);
    m_textCount.SetExtent(extentText);
    if (m_textCount.m_pString != 0)
        m_textCount.m_pString->SetFontScale(fScale);
#else
    CSWGuiExtent ideal = m_textCount.m_params.m_pTextObject->GetIdealWidthAndHeight();
    CSWGuiExtent extentText;
    extentText.x = static_cast<int>(fPad + (fIcon - static_cast<float>(ideal.w)) + static_cast<float>(nX));
    extentText.y = static_cast<int>(fScale * 32.0f + static_cast<float>(nY));
    extentText.w = ideal.w;
    extentText.h = static_cast<int>(fScale * 24.0f);
    m_textCount.SetExtent(extentText);
#endif

    extentIcon.x = static_cast<int>(fIcon + static_cast<float>(nX));
    extentIcon.w = static_cast<int>(static_cast<float>(m_extent.w) - fIcon);
    m_border.m_extent = extentIcon;
    m_borderHilight.m_extent = extentIcon;
    m_text.SetExtent(CSWGuiExtent::Intersection(m_border.GetInnerExtent(),
                                                m_borderHilight.GetInnerExtent()));
#else
    if (extent.w < 37)
        return;
    m_extent = extent;

    int nX = m_extent.x;
    int nY = m_extent.y;
    float fScale = GetResolutionTextScalar();
    float fIcon = fScale * 36.0f;
#ifdef K2_QOL_IMPROVEMENTS
    // A row the Text Size grew (K2QolGrowListRow, CSWGuiListBox::AddControls) gets an icon and
    // frames as tall as itself; at the fixed 36 they sat in a gap under a large caption.
    if (static_cast<float>(m_extent.h) > fIcon)
        fIcon = static_cast<float>(m_extent.h);
#endif
    int nIcon = static_cast<int>(fIcon);

    CSWGuiExtent extentIcon;
    extentIcon.x = nX;
    extentIcon.y = nY;
    extentIcon.w = nIcon;
    extentIcon.h = nIcon;
    m_borderFrame.m_extent = extentIcon;
    m_borderFrameHilight.m_extent = extentIcon;
    m_borderIcon.m_extent = extentIcon;

    CSWGuiExtent extentText = extentIcon;
    // The count string's presence picks the label's line height (55 or 21) and its alignment.
    if (m_textCount.m_params.m_sText.GetLength() > 2) {
        extentText.w = 55;
        m_textCount.m_params.SetAlignment(36);
    } else {
        extentText.w = 21;
        m_textCount.m_params.SetAlignment(34);
    }
    float fLine = fScale * static_cast<float>(extentText.w);
    extentText.x = static_cast<int>((fIcon - fLine) + static_cast<float>(extentText.x));
    extentText.y = static_cast<int>((fIcon - fScale * 19.0f) + static_cast<float>(extentText.y));
    extentText.h = 19;
    extentText.w = static_cast<int>(fLine + 1.0f);
    extentText.h = static_cast<int>(static_cast<float>(extentText.h) * fScale + 1.0f);
#ifdef K2_QOL_IMPROVEMENTS
    // The stack count grows with the icon it sits on, its text to fit the grown box.
    K2QolGrowItemCount(m_textCount, extentText, fIcon / (fScale * 36.0f));
#endif
    m_textCount.SetExtent(extentText);

    // extentIcon is reused for the name's border: x moves right by the icon, w shrinks by it.
    extentIcon.x = static_cast<int>(static_cast<float>(extentIcon.x) + fIcon);
    extentIcon.w = static_cast<int>(static_cast<float>(m_extent.w) - fIcon);
    m_border.m_extent = extentIcon;
    m_borderHilight.m_extent = extentIcon;
    m_text.SetExtent(CSWGuiExtent::Intersection(m_border.GetInnerExtent(),
                                                m_borderHilight.GetInnerExtent()));
#endif
}

void CSWGuiStoreItemEntry::Initialize(const CSWGuiExtent&, const CSWGuiTextParams&,
                                      const CSWGuiBorderParams&, const CSWGuiBorderParams&)
{
}

void CSWGuiStoreItemEntry::Initialize(const CSWGuiExtent&, CSWGuiButton*)
{
}

void CSWGuiStoreItemEntry::Initialize(CSWGuiText& text, CSWGuiBorder& borderInactive,
                                      CSWGuiBorder& borderActive, int nHeight)
{
#if KOTOR1
    // K1: blue / yellow frames on lbl_hex_3, a 56-of-768-line row.
    m_text.m_params = text.m_params;
    CSWGuiBorderParams& paramsNormal = m_border.m_params;
    paramsNormal = borderInactive.m_params;
    m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    CSWGuiBorderParams& paramsHilight = m_borderHilight.m_params;
    paramsHilight = borderActive.m_params;
    m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
    m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);
    CSWGuiBorderParams& paramsFrame = m_borderFrame.m_params;
    paramsFrame = CSWGuiBorderParams("", "", "lbl_hex_3", 0, 0, 0, 1.0f,
                                     Vector(1.0f, 1.0f, 1.0f), 0, 0);
    m_borderFrame.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    CSWGuiBorderParams& paramsFrameHilight = m_borderFrameHilight.m_params;
    paramsFrameHilight = CSWGuiBorderParams("", "", "lbl_hex_3", 0, 0, 0, 1.0f,
                                            Vector(1.0f, 1.0f, 1.0f), 0, 0);
    m_borderFrameHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
    m_borderFrameHilight.m_params.SetPulsingAlpha(1, 0, 1);
#if !defined(K2_UI_TOUCH) || defined(K2_FIX_ENGINE_BUGS)
    m_borderFrame.m_params.m_flags.nFillStyle = 2;         // stretched, as the item row's
    m_borderFrameHilight.m_params.m_flags.nFillStyle = 2;
#endif
#ifndef K2_UI_TOUCH
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 480.0f;   // pc / xbox: the 480-line layout's units
#else
    static float s_fScale = g_SDL_mode.h / 480.0f;   // pc / xbox: the 480-line layout's units
#endif
#else
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = g_SDL_mode.h / 768.0f;
#else
    static float s_fScale = g_SDL_mode.h / 768.0f;
#endif
#endif
    CSWGuiExtent extent(0, 0, nHeight, static_cast<int>(s_fScale * 56.0f));
    SetExtent(extent);
#else
    m_text.m_params = text.m_params;
    CSWGuiBorderParams& paramsNormal = m_border.m_params;
    paramsNormal = borderInactive.m_params;
    m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_DARK_GOLD);
    CSWGuiBorderParams& paramsHilight = m_borderHilight.m_params;
    paramsHilight = borderActive.m_params;
    m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_HIGHLIGHT);
    m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);
    CSWGuiBorderParams& paramsFrame = m_borderFrame.m_params;
    paramsFrame = CSWGuiBorderParams("", "", "UIBIT_EQP_ITM1", 0, 0, 0, 1.0f,
                                     Vector(1.0f, 1.0f, 1.0f), 0, 0);
    m_borderFrame.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_DARK_GOLD);
    CSWGuiBorderParams& paramsFrameHilight = m_borderFrameHilight.m_params;
    paramsFrameHilight = CSWGuiBorderParams("", "", "UIBIT_EQP_ITM1", 0, 0, 0, 1.0f,
                                            Vector(1.0f, 1.0f, 1.0f), 0, 0);
    m_borderFrameHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_NEW_HIGHLIGHT);
    m_borderFrameHilight.m_params.SetPulsingAlpha(1, 0, 1);
#if !defined(K2_UI_TOUCH) || defined(K2_FIX_ENGINE_BUGS)
    m_borderFrame.m_params.m_flags.nFillStyle = 2;         // stretched, as the item row's
    m_borderFrameHilight.m_params.m_flags.nFillStyle = 2;
#endif
    CSWGuiExtent extent(0, 0, nHeight, static_cast<int>(GetResolutionTextScalar() * 36.0f));
    SetExtent(extent);
#endif
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWGuiStoreItemEntry::SetColors(Vector colorFrame, Vector colorText, Vector colorHilight,
                                     Vector colorUnused)
{
    K2_UNUSED(colorUnused);
    m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&colorFrame);
    m_borderFrame.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&colorFrame);
    m_text.m_params.SetDefaultColor(colorText);
    m_textCount.m_params.SetDefaultColor(colorText);
    m_textCount.m_params.SetColor(colorText);
    m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&colorHilight);
    m_borderFrameHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&colorHilight);
}
#endif // !KOTOR1

#if KOTOR1
void CSWGuiStoreItemEntry::SetItem(unsigned long long oidItem, int bIsEquipped)
{
    K2_UNUSED(bIsEquipped);
    CResRef resRefIcon;
    CExoString sName;
    CExoString sCount;
    CExoString sReserved1;
    CExoString sReserved2;
    (void)sReserved1;
    (void)sReserved2;

    int nStack;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    CSWSItem* pItem = pServer->GetItemByGameObjectID(oidItem);
    m_oidItem = oidItem;
    if (oidItem == OBJECT_INVALID) {
        m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_TEXT_NONE));
        resRefIcon = const_cast<char*>("inone");
    } else {
        pItem = pServer->GetItemByGameObjectID(oidItem);
        if (pItem == 0) {
            m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_TEXT_NONE));
            resRefIcon = const_cast<char*>("inone");
        } else {
            resRefIcon = pItem->GetIcon();
            pItem->m_localizedName.GetString(g_pAppManager->m_pClientExoApp->GetClientLanguage(),
                                             &sName, 0);
            m_text.m_params.SetText(sName);
            nStack = pItem->m_stackCount;
            if (nStack >= 2) {
                if (nStack >= 100) {
                    m_borderFrame.m_params.SetFillImage(CResRef("lbl_hex_7"), 0);
                    m_borderFrameHilight.m_params.SetFillImage(CResRef("lbl_hex_7"), 0);
                } else {
                    m_borderFrame.m_params.SetFillImage(CResRef("lbl_hex_6"), 0);
                    m_borderFrameHilight.m_params.SetFillImage(CResRef("lbl_hex_6"), 0);
                }
                sCount.Format(const_cast<char*>("%d"), nStack);
            } else {
                m_borderFrame.m_params.SetFillImage(CResRef("lbl_hex_3"), 0);
                m_borderFrameHilight.m_params.SetFillImage(CResRef("lbl_hex_3"), 0);
            }
            m_textCount.m_params = CSWGuiTextParams(sCount, CResRef("fnt_d16x16"),
                                                    CGuiInGame::COLOR_BLUE, SWGUIALIGN_BOTTOM | SWGUIALIGN_CENTER, 1.0f);
        }
    }
    CSWGuiBorderParams& iconParams = m_borderIcon.m_params;
    iconParams.m_flags.nFillStyle = 2;
    iconParams.SetFillImage(resRefIcon, 0);
}
#else

void CSWGuiStoreItemEntry::SetItem(unsigned long long oidItem, int bIsEquipped, int nEquippedBy)
{
    K2_UNUSED(bIsEquipped);
    CResRef resRefIcon;
    CExoString sName;
    CExoString sCount;
    CExoString sReserved1;
    CExoString sReserved2;
    (void)sReserved1;
    (void)sReserved2;

    int nStack;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    CSWSItem* pItem = pServer->GetItemByGameObjectID(oidItem);
    m_nEquippedBy = nEquippedBy;
    m_oidItem = oidItem;
    if (oidItem == OBJECT_INVALID) {
        m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_TEXT_NONE));
        resRefIcon = const_cast<char*>("inone");
    } else {
        pItem = pServer->GetItemByGameObjectID(oidItem);
        if (pItem == 0) {
            m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_TEXT_NONE));
            resRefIcon = const_cast<char*>("inone");
        } else {
            resRefIcon = pItem->GetIcon();
            pItem->m_localizedName.GetString(g_pAppManager->m_pClientExoApp->GetClientLanguage(),
                                             &sName, 0);
            // A '<' or '{' in the name means it is a TLK token string; resolve it through the
            // client talk-table with the party leader as the speaker.
            if (sName.Find('<', 0) != -1 || sName.Find('{', 0) != -1) {
                CGameObject* pSpeaker = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
                g_pSWTlkTable->ParseStr(sName, pSpeaker, 1, 1);
            }
            if (m_nEquippedBy != -1)
                sName = sName + " (" + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_EQUIPPED) + ")";
            m_text.m_params.SetText(sName);
            nStack = pItem->m_stackCount;
            if (nStack >= 2) {
                if (nStack >= 100) {
                    m_borderFrame.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM3"), 0);
                    m_borderFrameHilight.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM3"), 0);
                } else {
                    m_borderFrame.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM2"), 0);
                    m_borderFrameHilight.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM2"), 0);
                }
                sCount.Format(const_cast<char*>("%d"), nStack);
            } else {
                m_borderFrame.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM1"), 0);
                m_borderFrameHilight.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM1"), 0);
            }
            m_textCount.m_params = CSWGuiTextParams(sCount, CResRef("fnt_d16x16"),
                                                    m_text.m_params.m_color, SWGUIALIGN_BOTTOM | SWGUIALIGN_CENTER, 1.0f);
        }
    }
    // A stretch fill on the icon border, then the item's own icon.
    CSWGuiBorderParams& iconParams = m_borderIcon.m_params;
    iconParams.m_flags.nFillStyle = 2;
    iconParams.SetFillImage(resRefIcon, 0);
}
#endif

