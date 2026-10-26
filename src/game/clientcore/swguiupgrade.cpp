// The upgrade droid's merchant panels are CSWGuiUpgrade, CSWGuiSwoopUpgrade, CSWUpgradeItemEntry,
// CSWGuiCreateItem, CSWGuiCreateMedicalItem, CSWGuiUpgradeSelection and CSWGuiUpgradeItemSelect.
// All seven were one source file.


#include "k2stdint.h"
#include "port/sdlapi.h"
#include "swguiupgrade.h"

#include "guiingame.h"
#include "swguiexamine.h"
#include "clientexoapp.h"
#include "appmanager.h"
#include "nwsitem.h"
#include "swguicontrol.h"
#include "swgui.h"
#include "exoresman.h"
#include "nwsrules.h"
#include "serverexoapp.h"
#include "partytables.h"
#include "swparty.h"
#include "nwtlktable.h"
#include "exotlktable.h"
#include "nwitem.h"
#include "gameobjectarray.h"
#include "swguimaininterfacepc2.h"
#include "CExoArrayListImpl.h"

#include "port/crtapi.h"
#include "AURORA/aurmodel.h"
#include "aurcamera.h"
#include "aurscene.h"
#include "Quaternion.h"
#include "Vector.h"
#include "twodimarrays.h"
#include "swguiCreateItem.h"
#include "swguidrawable.h"
#include "swguiingameequip.h"
#include "swguiingame.h"
#include "nwsstore.h"
#include "nwscreature.h"
#include "nwccreature.h"
#include "nwskill.h"
#include "twodimarray.h"
#include "nwsplayer.h"
#include "CExoLinkedList.h"
#include "exostring.h"
#include <stddef.h>
#include "nwscreaturestats.h"
#include "exolinkedlistinternal.h"

#include "port/libcstring.h"
#include "exoinput.h"
#include "restype.h"
#include "nwscriptconst.h"
#include "itemtype.h"
#include "strref.h"
#include "exostringinline.h"

// The create-item filter's slot groups: weapons, and everything worn but the weapons.
enum UpgradeFilterSlots {
    FILTER_SLOTS_WEAPON = CSWInventory::SLOT_RIGHTWEAPON | CSWInventory::SLOT_LEFTWEAPON,
    FILTER_SLOTS_WORN = CSWInventory::SLOT_HEAD | CSWInventory::SLOT_BODY | CSWInventory::SLOT_HANDS
                        | CSWInventory::SLOT_LEFTARM | CSWInventory::SLOT_RIGHTARM
                        | CSWInventory::SLOT_IMPLANT | CSWInventory::SLOT_BELT,
};

float GetResolutionTextScalar();

bool IsJoystickConnected();

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);

extern int g_bDo3DGUIs;

#if KOTOR1
const CSWGuiUpgrade::SSlotInfo CSWGuiUpgrade::SLOT_INFO[4][4] = {
    {
        { 0, "i_powerc", STRREF_UPGRADE_SLOT_POWER_CRYSTAL },
        { static_cast<unsigned>(-1), "i_colorc", STRREF_UPGRADE_SLOT_COLOR_CRYSTAL },
        { 0, "i_powerc", STRREF_UPGRADE_SLOT_POWER_CRYSTAL },
        { static_cast<unsigned>(-1), "", 0 },
    },
    {
        { 4, "i_scope", STRREF_UPGRADE_SLOT_SCOPE },
        { 5, "i_energy", STRREF_UPGRADE_SLOT_IMPROVED_ENERGY_CELL },
        { 6, "i_beam", STRREF_UPGRADE_SLOT_BEAM_SPLITTER },
        { 7, "i_hair", STRREF_UPGRADE_SLOT_HAIR_TRIGGER },
    },
    {
        { 1, "i_vcell", STRREF_UPGRADE_SLOT_VIBRATION_CELL },
        { 2, "i_durasteel", STRREF_UPGRADE_SLOT_DURASTEEL_BONDING_ALLOY },
        { 3, "i_imp_eng", STRREF_UPGRADE_SLOT_ENERGY_PROJECTOR },
        { static_cast<unsigned>(-1), "", 0 },
    },
    {
        { static_cast<unsigned>(-1), "", 0 },
        { 8, "i_armorrein", STRREF_UPGRADE_SLOT_ARMOR_REINFORCEMENT },
        { 9, "i_armorrein", STRREF_UPGRADE_SLOT_MESH_UNDERLAY },
        { static_cast<unsigned>(-1), "", 0 },
    },
};
#else
const CSWGuiUpgrade::SSlotInfo CSWGuiUpgrade::SLOT_INFO[4][6] = {
    {
        { 0, "i_powerc", STRREF_UPGRADE_SLOT_POWER_CRYSTAL },
        { static_cast<unsigned>(-1), "i_powerc", STRREF_UPGRADE_SLOT_COLOR_CRYSTAL },
        { 0, "i_powerc", STRREF_UPGRADE_SLOT_POWER_CRYSTAL },
        { 1, "i_lens", STRREF_UPGRADE_SLOT_LENS },
        { 2, "i_emitter", STRREF_UPGRADE_SLOT_EMITTER },
        { 3, "i_lsenergy", STRREF_UPGRADE_SLOT_ENERGY_CELL },
    },
    {
        { 4, "i_scope", STRREF_UPGRADE_SLOT_TARGETING },
        { 5, "i_energy", STRREF_UPGRADE_SLOT_POWER_PACK },
        { 6, "i_imp_eng", STRREF_UPGRADE_SLOT_FIRING_CHAMBER },
        { static_cast<unsigned>(-1), "", 0 },
        { static_cast<unsigned>(-1), "", 0 },
        { static_cast<unsigned>(-1), "", 0 },
    },
    {
        { 7, "i_grip", STRREF_UPGRADE_SLOT_GRIP },
        { 8, "i_edge", STRREF_UPGRADE_SLOT_EDGE },
        { 9, "i_vcell", STRREF_UPGRADE_SLOT_ENERGY_CELL },
        { static_cast<unsigned>(-1), "", 0 },
        { static_cast<unsigned>(-1), "", 0 },
        { static_cast<unsigned>(-1), "", 0 },
    },
    {
        { 10, "i_armorrein", STRREF_UPGRADE_SLOT_OVERLAY },
        { 11, "i_armorrein", STRREF_UPGRADE_SLOT_UNDERLAY },
        { static_cast<unsigned>(-1), "", 0 },
        { static_cast<unsigned>(-1), "", 0 },
        { static_cast<unsigned>(-1), "", 0 },
        { static_cast<unsigned>(-1), "", 0 },
    },
};
#endif

#if KOTOR1  // K1 has no assemble key; the scroll keys step the description ten rows; menu 1's description is LB_DESC_LS
void CSWGuiUpgrade::HandleInputEvent(int type, int val)
{
    if (val != 0)
    {
        switch (type)
        {
            case INPUTEVENT_GUI_B:
            case INPUTEVENT_GUI_BACK:
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                if (m_bItemsOpen & 1)
                {
                    ShowItems(0);
                    m_bItemsOpen &= 0xfe;
                    SetActiveControl(m_pActive, 0);
                    UpdateItems();
                    OnEnterSlot(m_pActiveControl);
                }
                else
                {
                    CancelPanel();
                }
                break;
            case INPUTEVENT_GUI_DPAD_UP:
            case INPUTEVENT_GUI_DPAD_DOWN:
                if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER)
                {
                    for (int i = 0; i < 10; i++)
                        m_lbDescLS.HandleInputEvent(type, 1);
                }
                else
                {
                    for (int i = 0; i < 10; i++)
                        m_lbDesc.HandleInputEvent(type, 1);
                }
                break;
            case INPUTEVENT_GUI_SCROLL_UP:
                if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER)
                    m_lbDescLS.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
                else
                    m_lbDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
                break;
            case INPUTEVENT_GUI_SCROLL_DOWN:
                if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER)
                    m_lbDescLS.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
                else
                    m_lbDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
                break;
            default:
                break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}
#else
void CSWGuiUpgrade::HandleInputEvent(int type, int val)
{
    if (val != 0)
    {
        switch (type)
        {
            case INPUTEVENT_GUI_B:
            case INPUTEVENT_GUI_BACK:  // shares the previous case's body
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                if (m_bItemsOpen & 1)
                {
                    ShowItems(0);
                    m_bItemsOpen &= 0xfe;
                    SetActiveControl(m_pActive, 0);
                    UpdateItems();
                    OnEnterSlot(m_pActiveControl);
                }
                else
                {
                    CancelPanel();
                }
                break;
            case INPUTEVENT_GUI_X:
                m_pItemSelect->FinishUpgrading();
                {
#if KOTOR1  // K1: the panel state bits sit one higher
                    unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
                    if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
                        nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#else
                    unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
                    if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
                        nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#endif
                    m_stateFlags = nFlags;
                }
                m_pGuiManager->PopModalPanel();
                break;
            case INPUTEVENT_GUI_SCROLL_UP:
                m_lbDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
                break;
            case INPUTEVENT_GUI_SCROLL_DOWN:
                m_lbDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
                break;
            default:
                break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}
#endif
void CSWGuiUpgrade::CloseItems()
{
    ShowItems(0);
    m_bItemsOpen &= 0xfe;
    SetActiveControl(m_pActive, 0);
    UpdateItems();
    OnEnterSlot(m_pActiveControl);
}

// Re-textures the upgrade slot labels' borders for the current menu regime.

#if KOTOR1  // K1: menus 1 and 3 fill the three-slot labels, 2 the four-slot ones, 4 their middle two
void CSWGuiUpgrade::UpdateItems()
{
    CResRef rIcon;
    CExoString sUnused;

    CSWGuiLabel* pLabels K2_UNSET;
    int nStart = 0;
    switch (m_nMenuType)
    {
    case CSWSItem::UPGRADABLE_TYPE_LIGHTSABER:
    case CSWSItem::UPGRADABLE_TYPE_MELEE:
        nStart = 0;
        pLabels = m_aslotLbl;
        break;
    case CSWSItem::UPGRADABLE_TYPE_ARMOR:
        nStart = 1;
    case CSWSItem::UPGRADABLE_TYPE_RANGED:
        pLabels = m_apRows;
        break;
    }
    int nSlots = 0;
    switch (m_nMenuType)
    {
    case CSWSItem::UPGRADABLE_TYPE_LIGHTSABER: nSlots = 3; break;
    case CSWSItem::UPGRADABLE_TYPE_RANGED: nSlots = 4; break;
    case CSWSItem::UPGRADABLE_TYPE_MELEE: nSlots = 3; break;
    case CSWSItem::UPGRADABLE_TYPE_ARMOR: nSlots = 2; break;
    }

    for (int i = nStart; i < nStart + nSlots; i++)
    {
        K2_ASSUME(pLabels != 0);   // nSlots is 0 for any menu type that leaves it unset
        CSWGuiLabel* pLabel = &pLabels[i];
        CSWSItem* pItem = m_apaItems[i];
        if (pItem)
        {
            g_pRules->GetBaseItem(pItem->m_baseItem);  // result unused
            rIcon = m_apaItems[i]->GetIcon();
        }
        else
        {
            rIcon = const_cast<char*>(SLOT_INFO[m_nMenuType - 1][i].m_szIcon);
        }
        pLabel->m_border.m_params.SetFillImage(rIcon, 0);
    }

    CExoString rDesc = m_pItem->GetPropertyDescription();
    SetDescription(rDesc);
}
#else
void CSWGuiUpgrade::UpdateItems()
{
    CResRef rIcon;
    CExoString sUnused;

    CSWGuiLabel* pLabels = NULL;
    int nSlots = 0;
    switch (m_nMenuType)
    {
    case CSWSItem::UPGRADABLE_TYPE_LIGHTSABER:
        pLabels = m_apRows;
        nSlots = 6;
        break;
    case CSWSItem::UPGRADABLE_TYPE_RANGED:
    case CSWSItem::UPGRADABLE_TYPE_ARMOR:
        pLabels = m_aslotLbl;
        nSlots = (m_nMenuType <= CSWSItem::UPGRADABLE_TYPE_MELEE) ? 3 : 2;
        break;
    case CSWSItem::UPGRADABLE_TYPE_MELEE:
        pLabels = m_aslotLbl;
        nSlots = 3;
        break;
    default:
        break;
    }

    for (int i = 0; i < nSlots; i++)
    {
        CSWGuiLabel* pLabel = &pLabels[i];
        CSWSItem* pItem = m_apaItems[i];
        if (pItem)
        {
            g_pRules->GetBaseItem(pItem->m_baseItem);  // result unused
            rIcon = pItem->GetIcon();
            pLabel->m_border.m_params.m_fAlpha = 1.0f;
        }
        else
        {
            rIcon = const_cast<char*>(SLOT_INFO[m_nMenuType - 1][i].m_szIcon);
        }
        pLabel->m_border.m_params.SetFillImage(rIcon, 0);
    }


    CExoString rDesc = m_pItem->GetPropertyDescription();
    SetDescription(rDesc);
}
#endif

// Rebuilds the 3D preview for the current menu regime.

void CSWGuiUpgrade::Update3DView()
{
    if (!g_bDo3DGUIs) {
        return;
    }
    CSWGui3DSceneView* pView = &m_viewItem;
    if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER) {
        pView = &m_viewChar;
    }
    CSWGuiScene* pScene = &pView->m_scene;
    pScene->RemoveModel(1, 1);

    CExoString sModel;
    CExoString sTexture;
    CSWBaseItem* pBase = m_pItem->GetBaseItem();
    if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_ARMOR) {
        GetArmorModelAndTextureResRef(DISGUISE_TYPE_P_MAL_A_MED_01, pBase->m_nBodyVar, m_pItem->m_nTextureVar,
                                      sModel, sTexture);
    }
    else {
        pBase->GetModelResRef(m_pItem->m_nModelVariation, 'm').CopyToString(sModel);
    }

    CExoString sCameraHook;
    CExoString sRotateHook;
    sCameraHook.Format(const_cast<char*>("camerahook%d"), pBase->m_nItemType);
    sRotateHook.Format(const_cast<char*>("rotatehook%d"), pBase->m_nItemType);
#if KOTOR1  // K1: no lightsaber-hilt hook remap
    switch (pBase->m_nItemType) {
    case ITEM_TYPE_RIFLE:
        if (m_pItem->m_baseItem == BASE_ITEM_BLASTER_CARBINE) {
            sCameraHook = sCameraHook + "a";
            sRotateHook = sRotateHook + "a";
        }
        break;
    case ITEM_TYPE_DOUBLE_WEAPON:
        if (m_pItem->m_baseItem == BASE_ITEM_STUN_BATON) {
            sCameraHook = sCameraHook + "a";
            sRotateHook = sRotateHook + "a";
        }
        break;
    }
#else
    switch (pBase->m_nItemType) {
    case ITEM_TYPE_ARMOR_CLASS_9: case ITEM_TYPE_CLOTHING: case ITEM_TYPE_JEDI_ROBE:
        sCameraHook = "camerahook31";
        sRotateHook = "rotatehook31";
        break;
    case ITEM_TYPE_RIFLE:
        if (m_pItem->m_baseItem == BASE_ITEM_BLASTER_CARBINE) {
            sCameraHook = sCameraHook + "a";
            sRotateHook = sRotateHook + "a";
        }
        break;
    case ITEM_TYPE_DOUBLE_WEAPON:
        if (m_pItem->m_baseItem == BASE_ITEM_STUN_BATON) {
            sCameraHook = sCameraHook + "a";
            sRotateHook = sRotateHook + "a";
        }
        break;
    }
#endif

    CAurCamera* pCamera = pView->m_scene.m_pCamera;
    CAurObject* pModel = pScene->GetModel(0);
    if (pModel->GetPart(static_cast<char*>(sCameraHook.CStr())) == 0) {
        sCameraHook = "camerahook";
    }
    pCamera->AttachToObject(pScene->GetModel(0), static_cast<char*>(sCameraHook.CStr()), 0);
    pCamera->SetViewAngle(22.726f);

    CAurObject* pNewModel = pScene->AddModel(sModel, -1);
    if (pNewModel != 0) {
        if (!sTexture.IsEmpty()) {
            pNewModel->ReplaceTexture(const_cast<char*>(""), static_cast<char*>(sTexture.CStr()));
        }
        if (pModel->GetPart(static_cast<char*>(sRotateHook.CStr())) != 0) {
            pNewModel->AttachToObject(pModel, static_cast<char*>(sRotateHook.CStr()), 0);
            pModel->PlayAnimation(const_cast<char*>("rotate"), 1.0f, 0, 0.0f);
        }
        if (m_pItem->GetBaseItem()->m_bPoweredItem != 0) {
            pNewModel->PlayAnimation(const_cast<char*>("powered"), 1.0f, 0, 0.0f);
        }
        if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_ARMOR) {
            pNewModel->PlayAnimation(const_cast<char*>("neutral"), 1.0f, 0, 0.0f);
        }
    }
}

#if KOTOR1  // K1: no null check, and the accept event plays a sound
void CSWGuiUpgrade::OnAssemble(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_START)
        m_pGuiManager->PlayGuiSound(pControl->m_tail.m_nClickSound);
    else if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_X)
        m_pGuiManager->PlayGuiSound(GUISOUND_UPGRADE);
    m_pItemSelect->FinishUpgrading();
    m_stateFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
    if ((m_stateFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
        m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
    m_pGuiManager->PopModalPanel();
}
#else
void CSWGuiUpgrade::OnAssemble(CSWGuiControl* pControl)
{
    if (pControl)
    {
        if (pControl->m_tail.nCurrentEventArg == 0)
            return;
        if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_START)
            m_pGuiManager->PlayGuiSound(pControl->m_tail.m_nClickSound);
    }
    m_pItemSelect->FinishUpgrading();
    m_stateFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
    if ((m_stateFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
        m_stateFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
    m_pGuiManager->PopModalPanel();
}
#endif

#if KOTOR1  // K1: no OK button or locked rows; menu 1 rebuilds the lightsaber from its upcrystals
void CSWGuiUpgrade::OnUpgradeSelected(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_START)
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
    CSWUpgradeItemEntry* pRow = static_cast<CSWUpgradeItemEntry*>(pControl);
    CServerExoApp* pApp = g_pAppManager->m_pServerExoApp;
    CItemRepository* pRepo = pApp->GetPartyTable()->GetPartyItems();
    int nMenu = static_cast<int>(m_pActive->m_tail.m_nUserData);

    if (nMenu == 1)
    {
        if (pRow->m_nUpgradeRow == 0xff)
            goto FINALIZE;
        CExoString sOut;
        CExoString sKey;
        CSWBaseItem* pBase = m_pItem->GetBaseItem();
        switch (pBase->m_nItemType)
        {
        case ITEM_TYPE_DOUBLE_LIGHTSABER:
            sKey = "DoubleMdlVar";
            break;
        case ITEM_TYPE_SHORT_LIGHTSABER:
            sKey = "ShortMdlVar";
            break;
        case ITEM_TYPE_LIGHTSABER:
            sKey = "LongMdlVar";
            break;
        }
        m_pUpgradeC2DA2->GetCExoStringEntry(pRow->m_nUpgradeRow, sKey, &sOut);
        CSWSItem* pNew = new CSWSItem(OBJECT_INVALID);
        {
            CResRef rr(sOut);
            pNew->LoadFromTemplate(rr);
        }
        pNew->SetPossessor(pApp->GetPlayerCreatureId(), 0, 0, OBJECT_INVALID);
        pNew->SetIdentified(1);
        pNew->m_flags = static_cast<unsigned short>((pNew->m_flags & ~static_cast<unsigned int>(CSWSItem::FLAG_STOLEN))
                                                    | (m_pItem->m_flags & static_cast<unsigned int>(CSWSItem::FLAG_STOLEN)));
        pNew->m_nUpgrades = m_pItem->m_nUpgrades;
        delete m_pItem;
        m_pItem = pNew;
        Update3DView();
    }
    else if (pRow->m_nUpgradeRow == 0xff && pRow->m_oidItem != OBJECT_INVALID)
    {
        goto FINALIZE;
    }

    {
        CSWSItem** ppSlot = &m_apaItems[nMenu];
        if (*ppSlot != 0)
        {
            (*ppSlot)->SetPossessor(pApp->GetPlayerCreatureId(), 0, 0, OBJECT_INVALID);
            int nFound = m_lstItemsTakenFromRepo.IndexOf(*ppSlot);
            if (nFound != -1)
            {
                m_lstItemsTakenFromRepo.DelIndex(nFound);
                pRepo->AddItem(ppSlot, 1, 0, 0);
            }
            else
            {
                pRepo->AddItem(ppSlot, 1, 0, 0);
                m_lstItemsAddedToRepo.Add(*ppSlot);
            }
            *ppSlot = 0;
            if (nMenu != 1)
                m_pItem->m_nUpgrades &= ~(1u << m_anUpgradeRow[nMenu]);
        }

        CSWSItem* pItem = pApp->GetItemByGameObjectID(pRow->m_oidItem);
        if (pItem == 0)
            goto FINALIZE;
        int nFound = m_lstItemsAddedToRepo.IndexOf(pItem);
        if (nFound != -1)
        {
            m_lstItemsAddedToRepo.DelIndex(nFound);
            if (pItem->m_stackCount >= 2)
                pItem = pItem->SplitItem(1);
            else
                pRepo->RemoveItem(pItem);
        }
        else
        {
            if (pItem->m_stackCount > 1)
                pItem = pItem->SplitItem(1);
            else
                pRepo->RemoveItem(pItem);
            m_lstItemsTakenFromRepo.Add(pItem);
        }
        *ppSlot = pItem;
        if (nMenu != 1)
        {
            m_anUpgradeRow[nMenu] = pRow->m_nUpgradeRow;
            m_pItem->m_nUpgrades |= 1u << m_anUpgradeRow[nMenu];
        }
    }

FINALIZE:
    ShowItems(0);
    m_bItemsOpen = static_cast<unsigned char>(m_bItemsOpen & ~1u);
    SetActiveControl(m_pActive, 0);
    UpdateItems();
    OnEnterSlot(m_pActiveControl);
}
#else
// On a slot click, builds a new item in assemble mode, otherwise moves the slot's item back to the
// party.

void CSWGuiUpgrade::OnUpgradeSelected(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_START)
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
    CSWUpgradeItemEntry* pRow = static_cast<CSWUpgradeItemEntry*>(pControl);
    if (pControl == &m_btnOk)
    {
        pRow = static_cast<CSWUpgradeItemEntry*>(m_lbItems.GetSelectedControl());
        if (!pRow)
            return;
    }
    if (pRow->m_nFlags & 2)
        return;
    CServerExoApp* pApp = g_pAppManager->m_pServerExoApp;
    CItemRepository* pRepo = pApp->GetPartyTable()->GetPartyItems();
    int nMenu = static_cast<int>(m_pActive->m_tail.m_nUserData);

    if (nMenu == 1 && m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER)
    {
        if (pRow->m_nUpgradeRow == -1)
            goto FINALIZE;
        CExoString sOut;
        CExoString sKey;
        CSWBaseItem* pBase = m_pItem->GetBaseItem();
        if (pBase->m_nItemType == ITEM_TYPE_DOUBLE_LIGHTSABER)
            sKey = "DoubleMdlVar";
        else if (pBase->m_nItemType == ITEM_TYPE_SHORT_LIGHTSABER)
            sKey = "ShortMdlVar";
        else if (pBase->m_nItemType == ITEM_TYPE_LIGHTSABER)
            sKey = "LongMdlVar";
        m_pUpgradeC2DA2->GetCExoStringEntry(pRow->m_nUpgradeRow, sKey, &sOut);
        CSWSItem* pNew = new CSWSItem(OBJECT_INVALID);
        {
            CResRef rr(sOut);
            pNew->LoadFromTemplate(rr);
        }
        pNew->SetPossessor(pApp->GetPlayerCreatureId(), 0, 0, OBJECT_INVALID);
        pNew->SetIdentified(1);
        pNew->m_nUpgrades = m_pItem->m_nUpgrades;
        pNew->AddUpgrade(m_apaItems[0], 0);
        pNew->m_anUpgVars[0] = m_pItem->m_anUpgVars[0];
        pNew->AddUpgrade(m_apaItems[2], 2);
        pNew->m_anUpgVars[2] = m_pItem->m_anUpgVars[2];
        pNew->AddUpgrade(m_apaItems[3], 3);
        pNew->m_anUpgVars[3] = m_pItem->m_anUpgVars[3];
        pNew->AddUpgrade(m_apaItems[4], 4);
        pNew->m_anUpgVars[4] = m_pItem->m_anUpgVars[4];
        pNew->AddUpgrade(m_apaItems[5], 5);
        pNew->m_anUpgVars[5] = m_pItem->m_anUpgVars[5];
        pNew->m_flags = static_cast<unsigned short>((pNew->m_flags & ~static_cast<unsigned int>(CSWSItem::FLAG_STOLEN))
                                                    | (m_pItem->m_flags & static_cast<unsigned int>(CSWSItem::FLAG_STOLEN)));
        delete m_pItem;
        m_pItem = pNew;
        Update3DView();
    }
    else if (pRow->m_nUpgradeRow == -1 && pRow->m_oidItem != OBJECT_INVALID)
    {
        goto FINALIZE;
    }
    goto P8;

// Hand the currently slotted item back to the party and re-stock the slot.

P8:
    {
        CSWSItem** ppSlot = &m_apaItems[nMenu];
    if (*ppSlot != 0)
    {
        (*ppSlot)->SetPossessor(pApp->GetPlayerCreatureId(), 0, 0, OBJECT_INVALID);
        int nFound = m_lstItemsTakenFromRepo.IndexOf(*ppSlot);
        if (nFound != -1)
        {
            m_lstItemsTakenFromRepo.DelIndex(nFound);
            pRepo->AddItem(ppSlot, 1, 0, 0);
        }
        else
        {
            pRepo->AddItem(ppSlot, 1, 0, 0);
            m_lstItemsAddedToRepo.Add(*ppSlot);
        }
        if (nMenu != 1 || m_nMenuType != CSWSItem::UPGRADABLE_TYPE_LIGHTSABER)
        {
            m_pItem->RemoveUpgrade(m_apaItems[nMenu], nMenu);
            m_pItem->m_anUpgVars[nMenu] = -1;
            m_apaItems[nMenu]->m_anUpgVars[nMenu] = -1;
        }
        *ppSlot = 0;
    }

    CSWSItem* pItem = pApp->GetItemByGameObjectID(pRow->m_oidItem);
    if (pItem == 0)
        goto FINALIZE;
    int nFound = m_lstItemsAddedToRepo.IndexOf(pItem);
    if (nFound != -1)
    {
        m_lstItemsAddedToRepo.DelIndex(nFound);
        if (pItem->m_stackCount >= 2)
            pItem = pItem->SplitItem(1);
        else
            pRepo->RemoveItem(pItem);
    }
    else
    {
        if (pItem->m_stackCount > 1)
            pItem = pItem->SplitItem(1);
        else
            pRepo->RemoveItem(pItem);
        m_lstItemsTakenFromRepo.Add(pItem);
    }

    int nRowMatch = 0;
    int nRows = m_pUpgradeC2DA->m_nRows;
    for (int r = 0; r < nRows; r++)
    {
        CExoString val;
        m_pUpgradeC2DA->GetCExoStringEntry(r, CExoString("Template"), &val);
        if (_stricmp(pItem->m_sTag.CStr(), val.CStr()) == 0)
        {
            nRowMatch = r;
            break;
        }
    }
    *ppSlot = pItem;
    if (nMenu == 1 && m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER)
        goto FINALIZE;
    m_anUpgradeRow[nMenu] = pRow->m_nUpgradeRow;
    m_pItem->AddUpgrade(pItem, nMenu);
    m_pItem->m_anUpgVars[nMenu] = nRowMatch;
    }

FINALIZE:
    ShowItems(0);
    m_bItemsOpen = static_cast<unsigned char>(m_bItemsOpen & ~1u);
    SetActiveControl(m_pActive, 0);
    UpdateItems();
    OnEnterSlot(m_pActiveControl);
}
#endif

#if KOTOR1  // K1: only menu 1 opens the item list
void CSWGuiUpgrade::OnSlotSelected(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_START)
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);

    int n = static_cast<int>(pControl->m_tail.m_nUserData);
    CItemRepository* pRepo = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems();
    if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER)
    {
        CExoArrayList<CSWGuiControl*> list;
        if (n == 1)
        {
            CExoString sVal;
            {
                list.Add(CreateItemEntry(m_apaItems[1]->m_objectId, -1));
                for (int r = 0; r < m_pUpgradeC2DA2->m_nRows; r++)
                {
                    m_pUpgradeC2DA2->GetCExoStringEntry(r, CExoString("Template"), &sVal);
                    sVal = sVal.LowerCase();
                    if (sVal != m_apaItems[1]->m_sTag)
                    {
                        unsigned long long oid = pRepo->FindItemWithTag(sVal);
                        if (oid != OBJECT_INVALID)
                            list.Add(CreateItemEntry(oid, r));
                    }
                }
            }
        }
        else
        {
            CExoString sVal;
            int nOther = 2 * (n == 0);
            {
                list.Add(CreateItemEntry(OBJECT_INVALID, -1));
                if (m_apaItems[n])
                    list.Add(CreateItemEntry(m_apaItems[n]->m_objectId, m_anUpgradeRow[n]));
                for (int r = 0; r < m_pUpgradeC2DA->m_nRows; r++)
                {
                    int nType;
                    m_pUpgradeC2DA->GetINTEntry(r, CExoString("UpgradeType"), &nType);
                    if (nType != 0)
                        continue;
                    m_pUpgradeC2DA->GetCExoStringEntry(r, CExoString("Template"), &sVal);
                    unsigned long long oid = pRepo->FindItemWithTag(sVal.LowerCase());
                    if (oid == OBJECT_INVALID)
                        continue;
                    if (m_apaItems[nOther] && m_anUpgradeRow[nOther] == r)
                        continue;
                    if (m_apaItems[n] && m_anUpgradeRow[n] == r)
                        continue;
                    list.Add(CreateItemEntry(oid, r));
                }
            }
        }
        m_pActive = pControl;
        m_lbItems.AddControls(list, 1, 0, 0);
        m_viewItem.m_tail.bVisible = 0;
        m_viewChar.m_tail.bVisible = 0;
        m_lblSlotName.m_tail.bVisible = 0;
        m_lblLSSlotName.m_tail.bVisible = 0;
        m_lblUpgradeCount.m_tail.bVisible = 0;
        m_lblUpgrades.m_tail.bVisible = 0;
        m_lblProperty.m_tail.bVisible = 0;
        m_btnAssemble.m_tail.bVisible = 0;
        m_abtnRow[0].m_tail.bVisible = 0;
        m_apRows[0].m_tail.bVisible = 0;
        m_abtnSlot[0].m_tail.bVisible = 0;
        m_aslotLbl[0].m_tail.bVisible = 0;
        m_abtnRow[1].m_tail.bVisible = 0;
        m_apRows[1].m_tail.bVisible = 0;
        m_abtnSlot[1].m_tail.bVisible = 0;
        m_aslotLbl[1].m_tail.bVisible = 0;
        m_abtnRow[2].m_tail.bVisible = 0;
        m_apRows[2].m_tail.bVisible = 0;
        m_abtnSlot[2].m_tail.bVisible = 0;
        m_aslotLbl[2].m_tail.bVisible = 0;
        m_abtnRow[3].m_tail.bVisible = 0;
        m_apRows[3].m_tail.bVisible = 0;
        m_lbItems.m_tail.bVisible = 1;
        SetActiveControl(&m_lbItems, 0);
        m_bItemsOpen |= 1;
        return;
    }

    CSWSItem** ppSlot = &m_apaItems[n];
    if (*ppSlot != 0)
    {
        m_pItem->m_nUpgrades &= ~(1u << m_anUpgradeRow[n]);
        int nFound = m_lstItemsTakenFromRepo.IndexOf(*ppSlot);
        if (nFound != -1)
        {
            m_lstItemsTakenFromRepo.DelIndex(nFound);
            pRepo->AddItem(ppSlot, 1, 0, 0);
        }
        else
        {
            pRepo->AddItem(ppSlot, 1, 0, 0);
            m_lstItemsAddedToRepo.Add(*ppSlot);
        }
        *ppSlot = 0;
    }
    else
    {
        unsigned long long oid = pRepo->FindItemWithTag(m_sCaption[n]);
        CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oid);
        if (pItem == 0)
            return;
        unsigned nEquippedSlot = 0;
        CSWSCreature* pSpeaker = 0;
        const CSWUpgradeItemEntry* pView = m_pItemSelect->m_pActive;
        if (pView && (pView->m_nFlags & 1))
        {
            nEquippedSlot = pView->m_nEquippedSlot;
            pSpeaker = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pView->m_oidEquippedBy);
        }
        m_pItem->m_nUpgrades = (1u << m_anUpgradeRow[n]) | m_pItem->m_nUpgrades;
        if (pSpeaker && !pSpeaker->CanEquipItem(m_pItem, &nEquippedSlot, 0, 0, 0))
        {
            CSWGuiMessageBox* pBox =
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
            pBox->SetAllowCancel(1);
            pBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::VerifyUpgrade), reinterpret_cast<CSWGuiControl*>(pItem));
            pBox->SetMessage(STRREF_UPGRADE_BLOCKS_REEQUIP);
            m_pGuiManager->AddPanel(pBox, 1, 1);
            return;
        }
        InsertUpgrade(pItem, n);
    }
    UpdateItems();
    OnEnterSlot(pControl);
}
#else
// On a slot click, fills the item list for the clicked slot from the upgrade table, or applies the
// selected upgrade when the regime has no slot list.

void CSWGuiUpgrade::OnSlotSelected(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_START)
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);

    CServerExoApp* pApp = g_pAppManager->m_pServerExoApp;
    CItemRepository* pRepo = pApp->GetPartyTable()->GetPartyItems();
    int n = static_cast<int>(pControl->m_tail.m_nUserData);
    if (m_nMenuType >= CSWSItem::UPGRADABLE_TYPE_RANGED && m_nMenuType <= CSWSItem::UPGRADABLE_TYPE_ARMOR)
    {
        CExoArrayList<CSWGuiControl*> list;
        int nBase2 = 0;
        if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_RANGED)
            nBase2 = n + 4;
        else if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_MELEE)
            nBase2 = n + 7;
        else if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_ARMOR)
            nBase2 = n + 10;
        {
            CExoString sVal;
            list.Add(CreateItemEntry(OBJECT_INVALID, -1, 0));
            if (m_apaItems[n])
                list.Add(CreateItemEntry(m_apaItems[n]->m_objectId, -1, 0));
            for (int r = 0; r < m_pUpgradeC2DA->m_nRows; r++)
            {
                int nType;
                m_pUpgradeC2DA->GetINTEntry(r, CExoString("UpgradeType"), &nType);
                if (nType != nBase2)
                    continue;
                m_pUpgradeC2DA->GetCExoStringEntry(r, CExoString("Template"), &sVal);
                unsigned long long oid = pRepo->FindItemWithTag(sVal.LowerCase());
                if (oid == OBJECT_INVALID)
                    continue;
                if (m_apaItems[2 * (n == 0)] && m_anUpgradeRow[2 * (n == 0)] == r)
                    continue;
                if (m_apaItems[n] && m_anUpgradeRow[n] == r)
                    continue;
                CSWSItem* pCheck = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oid);
                if (pCheck->m_nUpgradeLevel > m_pItem->m_nUpgradeLevel)
                    list.Add(CreateItemEntry(oid, r, 1));
                else
                    list.Add(CreateItemEntry(oid, r, 0));
            }
        }
        m_lbItems.AddControls(list, 1, 0, 0);
        m_viewItem.m_tail.bVisible = 0;
        m_viewChar.m_tail.bVisible = 0;
        m_lblSlotName.m_tail.bVisible = 0;
        m_lblProperty.m_tail.bVisible = 0;
        m_btnAssemble.m_tail.bVisible = 0;
        m_btnOk.m_tail.bVisible = 1;
        m_abtnSlot[0].m_tail.bVisible = 0;
        m_aslotLbl[0].m_tail.bVisible = 0;
        m_abtnSlot[1].m_tail.bVisible = 0;
        m_aslotLbl[1].m_tail.bVisible = 0;
        m_abtnSlot[2].m_tail.bVisible = 0;
        m_aslotLbl[2].m_tail.bVisible = 0;
        m_abtnRow[0].m_tail.bVisible = 0;
        m_apRows[0].m_tail.bVisible = 0;
        m_abtnRow[1].m_tail.bVisible = 0;
        m_apRows[1].m_tail.bVisible = 0;
        m_abtnRow[2].m_tail.bVisible = 0;
        m_apRows[2].m_tail.bVisible = 0;
        m_abtnRow[3].m_tail.bVisible = 0;
        m_apRows[3].m_tail.bVisible = 0;
        m_abtnRow[4].m_tail.bVisible = 0;
        m_apRows[4].m_tail.bVisible = 0;
        m_abtnRow[5].m_tail.bVisible = 0;
        m_apRows[5].m_tail.bVisible = 0;
        m_lbItems.m_tail.bVisible = 1;
        SetActiveControl(&m_lbItems, 0);
    }
    else if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER)
    {
        CExoArrayList<CSWGuiControl*> list;
        int nBase = (n & ~2) == 0 ? 0 : n - 2;
        if (n == 1)
        {
            CExoString sVal;
            unsigned long long oid;
            list.Add(CreateItemEntry(m_apaItems[n]->m_objectId, -1, 0));
            for (int r = 0; r < m_pUpgradeC2DA2->m_nRows; r++)
            {
                m_pUpgradeC2DA2->GetCExoStringEntry(r, CExoString("Template"), &sVal);
                sVal = sVal.LowerCase();
                if (sVal != m_apaItems[n]->m_sTag)
                {
                    oid = pRepo->FindItemWithTag(sVal);
                    if (oid != OBJECT_INVALID)
                        list.Add(CreateItemEntry(oid, r, 0));
                }
            }
        }
        else if (n > 2)
        {
            CExoString sVal;
            list.Add(CreateItemEntry(OBJECT_INVALID, -1, 0));
            if (m_apaItems[n])
                list.Add(CreateItemEntry(m_apaItems[n]->m_objectId, -1, 0));
            for (int r = 0; r < m_pUpgradeC2DA->m_nRows; r++)
            {
                int nType;
                m_pUpgradeC2DA->GetINTEntry(r, CExoString("UpgradeType"), &nType);
                if (nType != nBase)
                    continue;
                m_pUpgradeC2DA->GetCExoStringEntry(r, CExoString("Template"), &sVal);
                unsigned long long oid = pRepo->FindItemWithTag(sVal.LowerCase());
                if (oid == OBJECT_INVALID)
                    continue;
                if (m_apaItems[2 * (n == 0)] && m_anUpgradeRow[2 * (n == 0)] == r)
                    continue;
                if (m_apaItems[n] && m_anUpgradeRow[n] == r)
                    continue;
                list.Add(CreateItemEntry(oid, r, 0));
            }
        }
        else
        {
            CExoString sVal;
            list.Add(CreateItemEntry(OBJECT_INVALID, -1, 0));
            if (m_apaItems[n])
                list.Add(CreateItemEntry(m_apaItems[n]->m_objectId, -1, 0));
            for (int r = 0; r < m_pUpgradeC2DA->m_nRows; r++)
            {
                int nType;
                m_pUpgradeC2DA->GetINTEntry(r, CExoString("UpgradeType"), &nType);
                if (nType != 0)
                    continue;
                m_pUpgradeC2DA->GetCExoStringEntry(r, CExoString("Template"), &sVal);
                unsigned long long oid = pRepo->FindItemWithTag(sVal.LowerCase());
                if (oid == OBJECT_INVALID)
                    continue;
                if (m_apaItems[2 * (n == 0)] && m_anUpgradeRow[2 * (n == 0)] == r)
                    continue;
                if (m_apaItems[n] && m_anUpgradeRow[n] == r)
                    continue;
                list.Add(CreateItemEntry(oid, r, 0));
            }
        }
        m_lbItems.AddControls(list, 1, 0, 0);
        m_viewItem.m_tail.bVisible = 0;
        m_viewChar.m_tail.bVisible = 0;
        m_lblSlotName.m_tail.bVisible = 0;
        m_lblProperty.m_tail.bVisible = 0;
        m_btnAssemble.m_tail.bVisible = 0;
        m_btnOk.m_tail.bVisible = 1;
        m_abtnSlot[0].m_tail.bVisible = 0;
        m_aslotLbl[0].m_tail.bVisible = 0;
        m_abtnSlot[1].m_tail.bVisible = 0;
        m_aslotLbl[1].m_tail.bVisible = 0;
        m_abtnSlot[2].m_tail.bVisible = 0;
        m_aslotLbl[2].m_tail.bVisible = 0;
        m_abtnRow[0].m_tail.bVisible = 0;
        m_apRows[0].m_tail.bVisible = 0;
        m_abtnRow[1].m_tail.bVisible = 0;
        m_apRows[1].m_tail.bVisible = 0;
        m_abtnRow[2].m_tail.bVisible = 0;
        m_apRows[2].m_tail.bVisible = 0;
        m_abtnRow[3].m_tail.bVisible = 0;
        m_apRows[3].m_tail.bVisible = 0;
        m_abtnRow[4].m_tail.bVisible = 0;
        m_apRows[4].m_tail.bVisible = 0;
        m_abtnRow[5].m_tail.bVisible = 0;
        m_apRows[5].m_tail.bVisible = 0;
        m_lbItems.m_tail.bVisible = 1;
        SetActiveControl(&m_lbItems, 0);
    }
    else
    {
        CSWSItem** ppSlot = &m_apaItems[n];
        if (*ppSlot != 0)
        {
            m_pItem->m_nUpgrades &= ~(1u << (m_anUpgradeRow[n] & 0xff));
            int nFound = m_lstItemsTakenFromRepo.IndexOf(*ppSlot);
            if (nFound != -1)
            {
                m_lstItemsTakenFromRepo.DelIndex(nFound);
                pRepo->AddItem(ppSlot, 1, 0, 0);
            }
            else
            {
                pRepo->AddItem(ppSlot, 1, 0, 0);
                m_lstItemsAddedToRepo.Add(*ppSlot);
            }
            *ppSlot = 0;
        }
        else
        {
            unsigned long long oid = pRepo->FindItemWithTag(m_sCaption[n]);
            CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oid);
            if (pItem == 0)
                return;
            if (static_cast<unsigned>(pItem->m_nUpgradeLevel) >= static_cast<unsigned>(m_pItem->m_nUpgradeLevel))
                return;
            unsigned nEquippedSlot = 0;
            CSWSCreature* pSpeaker = 0;
            const CSWUpgradeItemEntry* pView = m_pItemSelect->m_pActive;
            if (pView && (pView->m_nFlags & 1))
            {
                nEquippedSlot = pView->m_nEquippedSlot;
                pSpeaker = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pView->m_oidEquippedBy);
                m_pItem->AddUpgrade(pItem, 0);
                if (pSpeaker && !pSpeaker->CanEquipItem(m_pItem, &nEquippedSlot, 0, 0, 0))
                {
                    CSWGuiMessageBox* pBox =
                        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
                    pBox->SetAllowCancel(1);
                    pBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::VerifyUpgrade), reinterpret_cast<CSWGuiControl*>(pItem));
                    pBox->SetMessage(STRREF_UPGRADE_BLOCKS_REEQUIP);
                    m_pGuiManager->AddPanel(pBox, 1, 1);
                    return;
                }
            }
            else
                m_pItem->AddUpgrade(pItem, 0);
            InsertUpgrade(pItem, n);
        }
        UpdateItems();
        OnEnterSlot(pControl);
        return;
    }

    m_pActive = pControl;
    m_bItemsOpen |= 1;
}
#endif

// On slot hover, repaints the two caption labels from the slot's item, resolving the server item by
// tag when the slot list does not hold one.

#if KOTOR1  // K1: no TLK token pass
void CSWGuiUpgrade::OnEnterSlot(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    int n = static_cast<int>(pControl->m_tail.m_nUserData);
    uint64_t nIconStrRef = SLOT_INFO[m_nMenuType - 1][n].m_nStrRef;

    if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER)
    {
        if (m_apaItems[n])
        {
            CExoString sText;
            CExoLocString loc = m_apaItems[n]->m_localizedName;
            loc.GetString(g_pAppManager->m_pClientExoApp->GetClientLanguage(), &sText, 0);
            m_lblLSSlotName.m_text.m_params.SetText(sText);
        }
        else
        {
            m_lblLSSlotName.m_text.m_params.SetStrRef(nIconStrRef);
        }
    }
    else
    {
        CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
        CSWSItem* pFound = pServer->GetItemByGameObjectID(
            pServer->GetPartyTable()->GetPartyItems()->FindItemWithTag(m_sCaption[n]));
        int nCount = pFound ? pFound->m_stackCount : 0;
        m_lblSlotName.m_text.m_params.SetStrRef(nIconStrRef);
        m_lblUpgradeCount.m_text.m_params.SetText(CExoString(nCount));
        m_lblUpgrades.m_text.m_params.SetStrRef(STRREF_UPGRADES);
        m_lblProperty.m_text.m_params.SetText(m_pItem->GetKeyedPropertyString(m_anUpgradeRow[n]));
    }
}
#else
void CSWGuiUpgrade::OnEnterSlot(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    int n = static_cast<int>(pControl->m_tail.m_nUserData);
    uint64_t nIconStrRef = SLOT_INFO[m_nMenuType - 1][n].m_nStrRef;

    if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER)
    {
        if (m_apaItems[n])
        {
            CExoString sText;
            CExoLocString loc = m_apaItems[n]->m_localizedName;
            loc.GetString(g_pAppManager->m_pClientExoApp->GetClientLanguage(), &sText, 0);
            if ((sText.Find('<', 0) & sText.Find('{', 0)) != -1)
            {
                CSWCCreature* pPC = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
                g_pSWTlkTable->ParseStr(sText, pPC, 1, 1);
            }
            m_lblSlotName.m_text.m_params.SetText(sText);
            CExoString sDesc = m_apaItems[n]->GetPropertiesOnlyDescription();
            m_lblProperty.m_text.m_params.SetText(sDesc);
        }
        else
        {
            m_lblSlotName.m_text.m_params.SetStrRef(nIconStrRef);
            m_lblProperty.m_text.m_params.SetText(CExoString(""));
        }
    }
    else
    {
        g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems()
                ->FindItemWithTag(m_sCaption[n]));
        if (!m_apaItems[n])
        {
            m_lblProperty.m_text.m_params.SetText(
                m_pItem->GetKeyedPropertyString(static_cast<unsigned char>(m_anUpgradeRow[n])));
            m_lblSlotName.m_text.m_params.SetStrRef(nIconStrRef);
        }
        else
        {
            CExoString sText;
            CExoLocString loc = m_apaItems[n]->m_localizedName;
            loc.GetString(g_pAppManager->m_pClientExoApp->GetClientLanguage(), &sText, 0);
            if ((sText.Find('<', 0) & sText.Find('{', 0)) != -1)
            {
                CSWCCreature* pPC = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
                g_pSWTlkTable->ParseStr(sText, pPC, 1, 1);
            }
            m_lblSlotName.m_text.m_params.SetText(sText);
            CExoString sDesc = m_apaItems[n]->GetPropertiesOnlyDescription();
            m_lblProperty.m_text.m_params.SetText(sDesc);
        }
    }
}
#endif

// Shows the merchandise list or re-lays the panel's slot widgets.

#if KOTOR1  // K1: the item list hides every caption
void CSWGuiUpgrade::ShowItems(int bShow)
{
    if (bShow)
    {
        m_viewItem.m_tail.bVisible = 0;
        m_viewChar.m_tail.bVisible = 0;
        m_lblSlotName.m_tail.bVisible = 0;
        m_lblLSSlotName.m_tail.bVisible = 0;
        m_lblUpgradeCount.m_tail.bVisible = 0;
        m_lblUpgrades.m_tail.bVisible = 0;
        m_lblProperty.m_tail.bVisible = 0;
        m_btnAssemble.m_tail.bVisible = 0;
        m_abtnRow[0].m_tail.bVisible = 0;
        m_apRows[0].m_tail.bVisible = 0;
        m_abtnSlot[0].m_tail.bVisible = 0;
        m_aslotLbl[0].m_tail.bVisible = 0;
        m_abtnRow[1].m_tail.bVisible = 0;
        m_apRows[1].m_tail.bVisible = 0;
        m_abtnSlot[1].m_tail.bVisible = 0;
        m_aslotLbl[1].m_tail.bVisible = 0;
        m_abtnRow[2].m_tail.bVisible = 0;
        m_apRows[2].m_tail.bVisible = 0;
        m_abtnSlot[2].m_tail.bVisible = 0;
        m_aslotLbl[2].m_tail.bVisible = 0;
        m_abtnRow[3].m_tail.bVisible = 0;
        m_apRows[3].m_tail.bVisible = 0;
        m_lbItems.m_tail.bVisible = 1;
        return;
    }

    unsigned char nMenuType = m_nMenuType;
    if (nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER)
    {
        m_viewItem.m_tail.bVisible = 0;
        m_lblSlotName.m_tail.bVisible = 0;
        m_lblUpgradeCount.m_tail.bVisible = 0;
        m_lblUpgrades.m_tail.bVisible = 0;
        m_lblProperty.m_tail.bVisible = 0;
        m_lblDescBg.m_tail.bVisible = 0;
        m_lbDesc.m_tail.bVisible = 0;
        m_viewChar.m_tail.bVisible = 1;
        m_lblLSSlotName.m_tail.bVisible = 1;
        m_btnAssemble.m_tail.bVisible = 1;
        m_lblDescBgLS.m_tail.bVisible = 1;
        m_lbDescLS.m_tail.bVisible = 1;
        m_abtnRow[0].m_tail.bVisible = 0;
        m_apRows[0].m_tail.bVisible = 0;
        m_abtnSlot[0].m_tail.bVisible = 1;
        m_aslotLbl[0].m_tail.bVisible = 1;
        m_abtnRow[1].m_tail.bVisible = 0;
        m_apRows[1].m_tail.bVisible = 0;
        m_abtnSlot[1].m_tail.bVisible = 1;
        m_aslotLbl[1].m_tail.bVisible = 1;
        m_abtnRow[2].m_tail.bVisible = 0;
        m_apRows[2].m_tail.bVisible = 0;
        m_abtnSlot[2].m_tail.bVisible = 1;
        m_aslotLbl[2].m_tail.bVisible = 1;
        m_abtnRow[3].m_tail.bVisible = 0;
        m_apRows[3].m_tail.bVisible = 0;
        SetActiveControl(&m_abtnSlot[0], 0);
    }
    else
    {
        m_viewItem.m_tail.bVisible = 1;
        m_lblSlotName.m_tail.bVisible = 1;
        m_lblUpgradeCount.m_tail.bVisible = 1;
        m_lblUpgrades.m_tail.bVisible = 1;
        m_lblProperty.m_tail.bVisible = 1;
        m_btnAssemble.m_tail.bVisible = 1;
        m_lblDescBg.m_tail.bVisible = 1;
        m_lbDesc.m_tail.bVisible = 1;
        m_viewChar.m_tail.bVisible = 0;
        m_lblLSSlotName.m_tail.bVisible = 0;
        m_lblDescBgLS.m_tail.bVisible = 0;
        m_lbDescLS.m_tail.bVisible = 0;

        int bSlots = 0;
        switch (nMenuType)
        {
        case CSWSItem::UPGRADABLE_TYPE_RANGED:
            m_abtnRow[0].m_tail.bVisible = 1;
            m_apRows[0].m_tail.bVisible = 1;
            m_abtnRow[1].m_tail.bVisible = 1;
            m_apRows[1].m_tail.bVisible = 1;
            m_abtnRow[2].m_tail.bVisible = 1;
            m_apRows[2].m_tail.bVisible = 1;
            m_abtnRow[3].m_tail.bVisible = 1;
            m_apRows[3].m_tail.bVisible = 1;
            SetActiveControl(&m_abtnRow[0], 0);
            break;
        case CSWSItem::UPGRADABLE_TYPE_ARMOR:
            m_abtnRow[0].m_tail.bVisible = 0;
            m_apRows[0].m_tail.bVisible = 0;
            m_abtnRow[1].m_tail.bVisible = 1;
            m_apRows[1].m_tail.bVisible = 1;
            m_abtnRow[2].m_tail.bVisible = 1;
            m_apRows[2].m_tail.bVisible = 1;
            m_abtnRow[3].m_tail.bVisible = 0;
            m_apRows[3].m_tail.bVisible = 0;
            SetActiveControl(&m_abtnRow[1], 0);
            break;
        case CSWSItem::UPGRADABLE_TYPE_MELEE:
            m_abtnRow[0].m_tail.bVisible = 0;
            m_apRows[0].m_tail.bVisible = 0;
            m_abtnRow[1].m_tail.bVisible = 0;
            m_apRows[1].m_tail.bVisible = 0;
            m_abtnRow[2].m_tail.bVisible = 0;
            m_apRows[2].m_tail.bVisible = 0;
            m_abtnRow[3].m_tail.bVisible = 0;
            m_apRows[3].m_tail.bVisible = 0;
            SetActiveControl(&m_abtnRow[0], 0);
            SetActiveControl(&m_abtnSlot[0], 0);
            m_abtnSlot[0].m_tail.nCurrentEventArg = 1;
            OnEnterSlot(&m_abtnSlot[0]);
            bSlots = 1;
            break;
        }
        m_abtnSlot[0].m_tail.bVisible = bSlots;
        m_aslotLbl[0].m_tail.bVisible = bSlots;
        m_abtnSlot[1].m_tail.bVisible = bSlots;
        m_aslotLbl[1].m_tail.bVisible = bSlots;
        m_abtnSlot[2].m_tail.bVisible = bSlots;
        m_aslotLbl[2].m_tail.bVisible = bSlots;
    }
    m_lbItems.m_tail.bVisible = 0;
}
#else
void CSWGuiUpgrade::ShowItems(int bShow)
{
    if (bShow)
    {
        m_viewItem.m_tail.bVisible = 0;
        m_viewChar.m_tail.bVisible = 0;
        m_lblSlotName.m_tail.bVisible = 0;
        m_lblProperty.m_tail.bVisible = 0;
        m_btnAssemble.m_tail.bVisible = 0;
        m_btnOk.m_tail.bVisible = 1;
        m_abtnSlot[0].m_tail.bVisible = 0;
        m_aslotLbl[0].m_tail.bVisible = 0;
        m_abtnSlot[1].m_tail.bVisible = 0;
        m_aslotLbl[1].m_tail.bVisible = 0;
        m_abtnSlot[2].m_tail.bVisible = 0;
        m_aslotLbl[2].m_tail.bVisible = 0;
        m_abtnRow[0].m_tail.bVisible = 0;
        m_apRows[0].m_tail.bVisible = 0;
        m_abtnRow[1].m_tail.bVisible = 0;
        m_apRows[1].m_tail.bVisible = 0;
        m_abtnRow[2].m_tail.bVisible = 0;
        m_apRows[2].m_tail.bVisible = 0;
        m_abtnRow[3].m_tail.bVisible = 0;
        m_apRows[3].m_tail.bVisible = 0;
        m_abtnRow[4].m_tail.bVisible = 0;
        m_apRows[4].m_tail.bVisible = 0;
        m_abtnRow[5].m_tail.bVisible = 0;
        m_apRows[5].m_tail.bVisible = 0;
        m_lbItems.m_tail.bVisible = 1;
        return;
    }

    unsigned char nMenuType = m_nMenuType;
    if (nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER)
    {
        m_viewItem.m_tail.bVisible = 0;
        m_lblSlotName.m_tail.bVisible = 1;
        m_lblProperty.m_tail.bVisible = 1;
        m_lblDescBg.m_tail.bVisible = 0;
        m_viewChar.m_tail.bVisible = 1;
        m_btnAssemble.m_tail.bVisible = 1;
        m_btnOk.m_tail.bVisible = 0;
        m_lblDescBgLS.m_tail.bVisible = 1;

        static_cast<CSWGuiNavigable*>(&m_abtnRow[0])->SetMoveToControl(3, &m_abtnRow[3]);
        static_cast<CSWGuiNavigable*>(&m_abtnRow[3])->SetMoveToControl(1, &m_abtnRow[0]);
        static_cast<CSWGuiNavigable*>(&m_abtnRow[1])->SetMoveToControl(3, &m_abtnRow[4]);
        static_cast<CSWGuiNavigable*>(&m_abtnRow[4])->SetMoveToControl(1, &m_abtnRow[1]);
        static_cast<CSWGuiNavigable*>(&m_abtnRow[2])->SetMoveToControl(3, &m_abtnRow[5]);
        static_cast<CSWGuiNavigable*>(&m_abtnRow[5])->SetMoveToControl(1, &m_abtnRow[2]);
        static_cast<CSWGuiNavigable*>(&m_abtnRow[0])->SetMoveToControl(2, &m_abtnRow[1]);
        static_cast<CSWGuiNavigable*>(&m_abtnRow[1])->SetMoveToControl(0, &m_abtnRow[0]);
        static_cast<CSWGuiNavigable*>(&m_abtnRow[1])->SetMoveToControl(2, &m_abtnRow[2]);
        static_cast<CSWGuiNavigable*>(&m_abtnRow[2])->SetMoveToControl(0, &m_abtnRow[1]);
        static_cast<CSWGuiNavigable*>(&m_abtnRow[3])->SetMoveToControl(2, &m_abtnRow[4]);
        static_cast<CSWGuiNavigable*>(&m_abtnRow[4])->SetMoveToControl(0, &m_abtnRow[3]);
        static_cast<CSWGuiNavigable*>(&m_abtnRow[4])->SetMoveToControl(2, &m_abtnRow[5]);
        static_cast<CSWGuiNavigable*>(&m_abtnRow[5])->SetMoveToControl(0, &m_abtnRow[4]);

        m_abtnRow[0].m_tail.bVisible = 1;
        m_apRows[0].m_tail.bVisible = 1;
        m_abtnRow[1].m_tail.bVisible = 1;
        m_apRows[1].m_tail.bVisible = 1;
        m_abtnRow[2].m_tail.bVisible = 1;
        m_apRows[2].m_tail.bVisible = 1;
        m_abtnRow[3].m_tail.bVisible = 1;
        m_apRows[3].m_tail.bVisible = 1;
        m_abtnRow[4].m_tail.bVisible = 1;
        m_apRows[4].m_tail.bVisible = 1;
        m_abtnRow[5].m_tail.bVisible = 1;
        m_apRows[5].m_tail.bVisible = 1;
        m_abtnSlot[0].m_tail.bVisible = 0;
        m_aslotLbl[0].m_tail.bVisible = 0;
        m_abtnSlot[1].m_tail.bVisible = 0;
        m_aslotLbl[1].m_tail.bVisible = 0;
        m_abtnSlot[2].m_tail.bVisible = 0;
        m_aslotLbl[2].m_tail.bVisible = 0;

        if (IsJoystickConnected())
            SetActiveControl(&m_abtnRow[0], 0);
    }
    else
    {
        m_viewItem.m_tail.bVisible = 1;
        m_lblSlotName.m_tail.bVisible = 1;
        m_lblProperty.m_tail.bVisible = 1;
        m_btnAssemble.m_tail.bVisible = 1;
        m_btnOk.m_tail.bVisible = 0;
        m_lblDescBg.m_tail.bVisible = 1;
        m_lbDesc.m_tail.bVisible = 1;
        m_viewChar.m_tail.bVisible = 0;
        m_lblDescBgLS.m_tail.bVisible = 0;

        bool bSlots = (static_cast<unsigned>(nMenuType - CSWSItem::UPGRADABLE_TYPE_RANGED) < 3);
        m_abtnSlot[0].m_tail.bVisible = bSlots;
        m_aslotLbl[0].m_tail.bVisible = bSlots;
        m_abtnSlot[1].m_tail.bVisible = bSlots;
        m_aslotLbl[1].m_tail.bVisible = bSlots;
        m_abtnSlot[2].m_tail.bVisible = bSlots;
        m_aslotLbl[2].m_tail.bVisible = bSlots;

        m_abtnRow[0].m_tail.bVisible = 0;
        m_apRows[0].m_tail.bVisible = 0;
        m_abtnRow[1].m_tail.bVisible = 0;
        m_apRows[1].m_tail.bVisible = 0;
        m_abtnRow[2].m_tail.bVisible = 0;
        m_apRows[2].m_tail.bVisible = 0;
        m_abtnRow[3].m_tail.bVisible = 0;
        m_apRows[3].m_tail.bVisible = 0;
        m_abtnRow[4].m_tail.bVisible = 0;
        m_apRows[4].m_tail.bVisible = 0;
        m_abtnRow[5].m_tail.bVisible = 0;
        m_apRows[5].m_tail.bVisible = 0;

        if (IsJoystickConnected())
            SetActiveControl(&m_abtnSlot[0], 0);
    }

    if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_ARMOR)
    {
        m_aslotLbl[2].m_tail.bVisible = 0;
        m_abtnSlot[2].m_tail.bVisible = 0;
        CSWGuiButton* pSlotA = &m_abtnSlot[0];
        CSWGuiButton* pSlotB = &m_abtnSlot[1];
        static_cast<CSWGuiNavigable*>(pSlotA)->SetMoveToControl(3, pSlotB);
        static_cast<CSWGuiNavigable*>(pSlotB)->SetMoveToControl(1, pSlotA);
    }
    else
    {
        CSWGuiButton* pSlotC = &m_abtnSlot[2];
        CSWGuiButton* pSlotB = &m_abtnSlot[1];
        CSWGuiButton* pSlotA = &m_abtnSlot[0];
        static_cast<CSWGuiNavigable*>(pSlotB)->SetMoveToControl(1, pSlotC);
        static_cast<CSWGuiNavigable*>(pSlotA)->SetMoveToControl(3, pSlotC);
        static_cast<CSWGuiNavigable*>(pSlotC)->SetMoveToControl(3, pSlotB);
        static_cast<CSWGuiNavigable*>(pSlotC)->SetMoveToControl(1, pSlotA);
    }
    m_lbItems.m_tail.bVisible = 0;
}
#endif

#if KOTOR1  // K1: clears five captions
void CSWGuiUpgrade::OnEnterAssemble(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    m_lblSlotName.m_text.m_params.SetText(CExoString(""));
    m_lblLSSlotName.m_text.m_params.SetText(CExoString(""));
    m_lblUpgradeCount.m_text.m_params.SetText(CExoString(""));
    m_lblProperty.m_text.m_params.SetText(CExoString(""));
    m_lblUpgrades.m_text.m_params.SetText(CExoString(""));
}
#else
void CSWGuiUpgrade::OnEnterAssemble(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    m_lblSlotName.m_text.m_params.SetText(CExoString(""));
    m_lblProperty.m_text.m_params.SetText(CExoString(""));
}
#endif

// On panel teardown, releases the slot item pointers and nulls the item handles.

void CSWGuiUpgrade::OnPanelRemoved()
{
    m_nMenuType = CSWSItem::UPGRADABLE_TYPE_NONE;
    m_pItem = 0;
    if (m_pItemSlot) {
        delete m_pItemSlot;
        m_pItemSlot = 0;
    }

    if (m_apaItems[0]) {
        delete m_apaItems[0];
        m_apaItems[0] = 0;
    }
    if (m_apaItems[1]) {
        delete m_apaItems[1];
        m_apaItems[1] = 0;
    }
    if (m_apaItems[2]) {
        delete m_apaItems[2];
        m_apaItems[2] = 0;
    }
    if (m_apaItems[3]) {
        delete m_apaItems[3];
        m_apaItems[3] = 0;
    }
#if !KOTOR1  // K1 has four slots
    if (m_apaItems[4]) {
        delete m_apaItems[4];
        m_apaItems[4] = 0;
    }
    if (m_apaItems[5]) {
        delete m_apaItems[5];
        m_apaItems[5] = 0;
    }
#endif
    m_lstItemsAddedToRepo.m_count = 0;
    m_lstItemsTakenFromRepo.m_count = 0;
    CSWGuiPanel::OnPanelRemoved();
}

#if !KOTOR1
static const int s_anDimCounts[4] = { 6, 3, 3, 2 };  // border-dim run length per menu type
#endif

#if KOTOR1  // K1: no caption or TLK pass; a lightsaber needs its upcrystals hilt or the panel closes
void CSWGuiUpgrade::OnPanelAdded()
{
    if (m_pItem == NULL) {
        m_stateFlags = (m_stateFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE
                           ? (m_stateFlags & ~PANEL_REMOVE_DEFERRED)
                           : (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
        return;
    }

    m_pItemSlot = new CSWSItem(OBJECT_INVALID);
    m_pItemSlot->CopyItem(m_pItem);

    CSWGuiLabel* pRow;
    int nStart = 0;
    switch (m_nMenuType) {
    case CSWSItem::UPGRADABLE_TYPE_LIGHTSABER: {
        CExoString sCol, sVal, sTmp;
        switch (m_pItem->GetBaseItem()->m_nItemType) {
        case ITEM_TYPE_DOUBLE_LIGHTSABER:
            sCol = "DoubleMdlVar";
            break;
        case ITEM_TYPE_SHORT_LIGHTSABER:
            sCol = "ShortMdlVar";
            break;
        case ITEM_TYPE_LIGHTSABER:
            sCol = "LongMdlVar";
            break;
        default:
            m_stateFlags = (m_stateFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE
                               ? (m_stateFlags & ~PANEL_REMOVE_DEFERRED)
                               : (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
            return;
        }
        for (int nRow = 0; nRow < m_pUpgradeC2DA2->m_nRows; nRow++) {
            if (!m_pUpgradeC2DA2->GetCExoStringEntry(nRow, sCol, &sVal))
                continue;
            if (sVal.CompareNoCase(m_pItem->m_sTag) == 0)
                continue;
            if (!m_pUpgradeC2DA2->GetCExoStringEntry(nRow, CExoString("Template"), &sTmp))
                continue;
            m_apaItems[1] = new CSWSItem(OBJECT_INVALID);
            m_apaItems[1]->LoadFromTemplate(CResRef(sTmp));
            break;
        }
        if (m_apaItems[1] == NULL) {
            m_stateFlags = (m_stateFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE
                               ? (m_stateFlags & ~PANEL_REMOVE_DEFERRED)
                               : (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
            return;
        }
        pRow = m_aslotLbl;
        break;
    }
    case CSWSItem::UPGRADABLE_TYPE_RANGED:
        static_cast<CSWGuiNavigable*>(&m_abtnRow[1])->SetMoveToControl(1, &m_abtnRow[0]);
        static_cast<CSWGuiNavigable*>(&m_abtnRow[2])->SetMoveToControl(3, &m_abtnRow[3]);
        pRow = m_apRows;
        break;
    case CSWSItem::UPGRADABLE_TYPE_MELEE:
        pRow = m_aslotLbl;
        break;
    case CSWSItem::UPGRADABLE_TYPE_ARMOR:
        static_cast<CSWGuiNavigable*>(&m_abtnRow[1])->SetMoveToControl(1, &m_abtnRow[2]);
        static_cast<CSWGuiNavigable*>(&m_abtnRow[2])->SetMoveToControl(3, &m_abtnRow[1]);
        nStart = 1;
        pRow = m_apRows;
        break;
    default:
        m_stateFlags = (m_stateFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE
                           ? (m_stateFlags & ~PANEL_REMOVE_DEFERRED)
                           : (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
        return;
    }

    ShowItems(0);
    int nSlots = 0;
    switch (m_nMenuType) {
    case CSWSItem::UPGRADABLE_TYPE_LIGHTSABER: nSlots = 3; break;
    case CSWSItem::UPGRADABLE_TYPE_RANGED: nSlots = 4; break;
    case CSWSItem::UPGRADABLE_TYPE_MELEE: nSlots = 3; break;
    case CSWSItem::UPGRADABLE_TYPE_ARMOR: nSlots = 2; break;
    }

    CExoString sTag;
    for (int nRow = 0; nRow < m_pUpgradeC2DA->m_nRows; nRow++) {
        int nType;
        m_pUpgradeC2DA->GetINTEntry(nRow, CExoString("UpgradeType"), &nType);
        m_pUpgradeC2DA->GetCExoStringEntry(nRow, CExoString("Template"), &sTag);
        for (int nSlot = nStart; nSlot < nStart + nSlots; nSlot++) {
            if (m_apaItems[nSlot] != NULL)
                continue;
            if (nType != static_cast<int>(SLOT_INFO[m_nMenuType - 1][nSlot].m_nIconId))
                continue;
            if (m_nMenuType != CSWSItem::UPGRADABLE_TYPE_LIGHTSABER) {
                m_sCaption[nSlot] = sTag.LowerCase();
                pRow[nSlot].m_border.m_params.m_fAlpha = 1.0f;
            }
            m_anUpgradeRow[nSlot] = static_cast<unsigned char>(nRow);
            if (static_cast<unsigned char>(nRow) == 0xff || (m_pItem->m_nUpgrades & (1u << nRow))) {
                m_apaItems[nSlot] = new CSWSItem(OBJECT_INVALID);
                m_apaItems[nSlot]->LoadFromTemplate(CResRef(sTag));
            } else if (m_nMenuType != CSWSItem::UPGRADABLE_TYPE_LIGHTSABER &&
                       g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems()
                               ->FindItemWithTag(m_sCaption[nSlot]) == OBJECT_INVALID) {
                pRow[nSlot].m_border.m_params.m_fAlpha = 0.25f;
            }
            break;
        }
    }
    Update3DView();
    UpdateItems();
    CSWGuiPanel::OnPanelAdded();
}
#else
// Frames the selected item and fills the upgrade slots for the current menu regime.

void CSWGuiUpgrade::OnPanelAdded()
{
    if (m_pItem == NULL) {
        m_stateFlags = (m_stateFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE
                           ? (m_stateFlags & ~PANEL_REMOVE_DEFERRED)
                           : (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
        return;
    }

    CExoString sText;
    m_pItem->m_localizedName.GetString(g_pAppManager->m_pClientExoApp->GetClientLanguage(), &sText, 0);
    int nLt = sText.Find('<', 0);
    int nLb = sText.Find('{', 0);
    if ((nLt & nLb) != -1) {
        CSWTlkTable* pTlk = g_pSWTlkTable;
        pTlk->m_pSpeakerObj =
            g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
        pTlk->m_gender = pTlk->GetObjectGender();
        pTlk->m_bSuppressEnabled = static_cast<unsigned char>(pTlk->m_bSuppressEnabled | 1);
        pTlk->ParseStr(sText);
    }
    m_lblTitle.m_text.m_params.SetText(sText);
    m_lblSlotName.m_text.m_params.SetText(CExoString(""));
    m_lblProperty.m_text.m_params.SetText(CExoString(""));
    m_pItemSlot = new CSWSItem(OBJECT_INVALID);
    m_pItemSlot->CopyItem(m_pItem);

    CSWGuiLabel* pRow;
    switch (m_nMenuType) {
    case CSWSItem::UPGRADABLE_TYPE_RANGED:
    case CSWSItem::UPGRADABLE_TYPE_MELEE:
    case CSWSItem::UPGRADABLE_TYPE_ARMOR:
        pRow = m_aslotLbl;
        break;
    case CSWSItem::UPGRADABLE_TYPE_LIGHTSABER: {
        CExoString sCol, sVal, sTmp;
        bool bKeep = true;
        switch (m_pItem->GetBaseItem()->m_nItemType) {
        case ITEM_TYPE_DOUBLE_LIGHTSABER:
            sCol = "DoubleMdlVar";
            break;
        case ITEM_TYPE_SHORT_LIGHTSABER:
            sCol = "ShortMdlVar";
            break;
        case ITEM_TYPE_LIGHTSABER:
            sCol = "LongMdlVar";
            break;
        default:
            m_stateFlags = (m_stateFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE
                               ? (m_stateFlags & ~PANEL_REMOVE_DEFERRED)
                               : (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
            bKeep = false;
            break;
        }
        if (bKeep) {
            for (int nRow = 0; nRow < m_pUpgradeC2DA2->m_nRows; nRow++) {
                if (!m_pUpgradeC2DA2->GetCExoStringEntry(nRow, sCol, &sVal)) {
                    continue;
                }
                if (sVal.CompareNoCase(m_pItem->m_sTag) == 0) {
                    continue;
                }
                if (!m_pUpgradeC2DA2->GetCExoStringEntry(nRow, CExoString("Template"), &sTmp)) {
                    continue;
                }
                m_apaItems[1] = new CSWSItem(OBJECT_INVALID);
                m_apaItems[1]->LoadFromTemplate(CResRef(sTmp));
// First matching row wins, so the loop stops here.

                break;
            }
        }
        if (!bKeep) {
            return;
        }
        pRow = m_apRows;
        break;
    }
    default:
        m_stateFlags = (m_stateFlags & PANEL_REMOVE_MASK) == PANEL_REMOVE_AND_DELETE
                           ? (m_stateFlags & ~PANEL_REMOVE_DEFERRED)
                           : (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
        return;
    }

    ShowItems(0);
    int nDim = (static_cast<unsigned>(m_nMenuType - 1) <= 3) ? s_anDimCounts[m_nMenuType - 1] : 0;

    CExoString sTag;
    int anFound[6] = {};
    CSWGuiLabel* pLabel = pRow;
    if (nDim != 0) {
        for (int i = nDim; i != 0; --i) {
            pLabel->m_border.m_params.m_fAlpha = 0.25f;
            ++pLabel;
        }
    }
    for (int nRow = 0; nRow < m_pUpgradeC2DA->m_nRows; nRow++) {
        int nType;
        m_pUpgradeC2DA->GetINTEntry(nRow, CExoString("UpgradeType"), &nType);
        m_pUpgradeC2DA->GetCExoStringEntry(nRow, CExoString("Template"), &sTag);
        for (int64_t nSlot = 0; nSlot < nDim; nSlot++) {
            if (m_apaItems[nSlot] != NULL) {
                continue;
            }
            if (SLOT_INFO[m_nMenuType - 1][nSlot].m_nIconId != static_cast<uint32_t>(nType)) {
                continue;
            }
            m_sCaption[nSlot] = sTag.LowerCase();
            m_anUpgradeRow[nSlot] = nRow;
            if (m_pItem->m_anUpgVars[nSlot] == nRow) {
                m_apaItems[nSlot] = new CSWSItem(OBJECT_INVALID);
                m_apaItems[nSlot]->LoadFromTemplate(CResRef(sTag));
                m_apaItems[nSlot]->m_anUpgVars[nSlot] = nRow;
            } else if (nSlot == 0 && m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER && m_pItem->m_anUpgVars[2] == nRow) {
                m_apaItems[2] = new CSWSItem(OBJECT_INVALID);
                m_apaItems[2]->LoadFromTemplate(CResRef(sTag));
                m_apaItems[2]->m_anUpgVars[2] = nRow;
            } else {
                unsigned long long oidItem =
                    g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems()->FindItemWithTag(m_sCaption[nSlot]);
                if (oidItem == OBJECT_INVALID) {
                    if (anFound[nSlot] == 0) {
                        pRow[nSlot].m_border.m_params.m_fAlpha = 0.25f;
                    }
                } else {
                    CGameObject* pGameObj = g_pAppManager->m_pServerExoApp->GetGameObject(oidItem);
                    if (pGameObj != NULL) {
                        pGameObj->AsSWSItem();
                    }
                    if (pGameObj->AsSWSItem()->m_nUpgradeLevel <= m_pItem->m_nUpgradeLevel) {
                        anFound[nSlot] = 1;
                        pRow[nSlot].m_border.m_params.m_fAlpha = 1.0f;
                        if (nSlot == 0 && m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER) {
                            pRow[2].m_border.m_params.m_fAlpha = 1.0f;
                        }
                    }
                }
            }
            break;
        }
    }
    CSWGuiLabel* pLabel2 = pRow;
    CSWSItem** ppItem = m_apaItems;
    if (nDim != 0) {
        for (int i = 0; i < nDim; i++) {
            if (*ppItem != NULL) {
                pLabel2->m_border.m_params.m_fAlpha = 1.0f;
            }
            ++ppItem;
            ++pLabel2;
        }
    }
    if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER) {
        pRow[1].m_border.m_params.m_fAlpha = 1.0f;
    }
    Update3DView();
    UpdateItems();
    if (IsJoystickConnected()) {
        SetActiveControl(&m_btnAssemble, 0);
        ShowItems(0);
    } else {
        SetActiveControl(pRow, 0);
    }
    CSWGuiPanel::OnPanelAdded();
}
#endif

// The confirm-box callback runs the upgrade when it was accepted and clears the item's verify mask
// otherwise.

void CSWGuiUpgrade::VerifyUpgrade(CSWGuiControl* pControl)
{
    int n = static_cast<int>(m_pActiveControl->m_tail.m_nUserData);
    CSWGuiMessageBox* pBox =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    if (pBox->m_bAccepted)
    {
        InsertUpgrade(reinterpret_cast<CSWSItem*>(pControl), n);
        UpdateItems();
        OnEnterSlot(m_pActiveControl);
    }
    else
    {
        CSWSItem* pItem = reinterpret_cast<CSWSItem*>(pControl);
        pItem->m_nUpgrades &= ~(1u << (m_anUpgradeRow[n] & 0xff));
    }
}

// Installs one unit of an item into the panel, splitting the stack or removing the item from the
// party repository first.

void CSWGuiUpgrade::InsertUpgrade(CSWSItem* pItem, int nSlot)
{
    if (!pItem)
        return;
    int nFound = m_lstItemsAddedToRepo.IndexOf(pItem);
    if (nFound != -1)
    {
        m_lstItemsAddedToRepo.DelIndex(nFound);
        if (pItem->m_stackCount >= 2)
            pItem = pItem->SplitItem(1);
        else
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems()->RemoveItem(pItem);
        m_apaItems[nSlot] = pItem;
        return;
    }

    if (pItem->m_stackCount > 1)
        pItem = pItem->SplitItem(1);
    else
        g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems()->RemoveItem(pItem);

    m_lstItemsTakenFromRepo.Add(pItem);
    m_apaItems[nSlot] = pItem;
}

#if !KOTOR1  // K2-only
void CSWGuiUpgrade::OnDisabledSelected(CSWGuiControl* pControl)
{
    if (pControl == 0 || pControl->m_tail.nCurrentEventArg == 0)
        return;
    CSWGuiMessageBox* pBox =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    pBox->SetAllowCancel(0);
    pBox->SetMessage(STRREF_UPGRADE_INCOMPATIBLE);
    pBox->SetCallback(0, 0, 0);
    pBox->SetMessageColors(CGuiInGame::COLOR_UPGRADE_BORDER, CGuiInGame::COLOR_NEW_HIGHLIGHT);
    g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
}
#endif

// On row hover, builds the item's property description and pre-processes its TLK tokens.

#if KOTOR1  // K1: the item's identified description, prefixed on menu 1 by the row's keyed property
void CSWGuiUpgrade::OnControlEntered(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    CSWUpgradeItemEntry* pEntry = static_cast<CSWUpgradeItemEntry*>(pControl);
    if (pEntry->m_oidItem == OBJECT_INVALID)
    {
        CExoString s("");
        SetDescription(s);
        return;
    }
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pEntry->m_oidItem);
    CExoString s;
    pItem->m_descriptionIdentified.GetString(g_pAppManager->m_pClientExoApp->GetClientLanguage(),
                                             &s, 0);
    if (s == "")
        s = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NO_DESCRIPTION_SET);
    if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER && static_cast<int>(m_pActive->m_tail.m_nUserData) != 1)
        s = m_pItem->GetKeyedPropertyString(pEntry->m_nUpgradeRow) + s;
    SetDescription(s);
}
#else
void CSWGuiUpgrade::OnControlEntered(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    if (static_cast<CSWUpgradeItemEntry*>(pControl)->m_oidItem == OBJECT_INVALID)
    {
        CExoString s("");
        SetDescription(s);
    }
    else
    {
        CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
            static_cast<CSWUpgradeItemEntry*>(pControl)->m_oidItem);
        CExoString s;
        {
            CExoString prop = pItem->GetPropertyDescription();
            s = prop;
        }
        int nAngle = s.Find('<', 0);
        int nBrace = s.Find('{', 0);
        if ((nAngle & nBrace) != -1)
        {
            CSWCCreature* pSpeaker = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
            g_pSWTlkTable->ParseStr(s, pSpeaker, 1, 1);
        }
        if (s != "")
        {
            SetDescription(s);
        }
        else
        {
            CExoString t = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NO_DESCRIPTION_SET);
            SetDescription(t);
        }
    }
}
#endif

// Re-texts and re-fits the description label, then re-pins it into the list.

#if KOTOR1  // K1: menu 1 describes into LB_DESC_LS
void CSWGuiUpgrade::SetDescription(const CExoString& s)
{
    m_lblDescription.m_text.m_params.SetText(s);
    CSWGuiListBox* pList = &m_lbDesc;
    if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER)
        pList = &m_lbDescLS;
    pList->ClearItems();
    CSWGuiControl* pCaption;
    CSWGuiText* pText = &m_lblDescription.m_text;
    CSWGuiExtent ext = m_lblDescription.m_extent;
    if (s.GetLength() >= 1)
        ext.h = pText->GetIdealHeight();
    else
        ext.h = pText->GetFontHeight();
    m_lblDescription.SetExtent(ext);
    pCaption = &m_lblDescription;
    pList->AddControls(&pCaption, 1, 0, 0, 0);
    pList->SetSelectedControl(&m_lblDescription, 0);
}
#else
void CSWGuiUpgrade::SetDescription(const CExoString& s)
{
    m_lblDescription.m_text.m_params.SetText(s);
    m_lbDesc.ClearItems();
    CSWGuiControl* pCaption;
    CSWGuiText* pText = &m_lblDescription.m_text;
    CSWGuiExtent ext = m_lblDescription.m_extent;
    if (s.GetLength() >= 1)
        ext.h = pText->GetIdealHeight();
    else
        ext.h = pText->GetFontHeight();
    m_lblDescription.SetExtent(ext);
    pCaption = &m_lblDescription;
    m_lbDesc.AddControls(&pCaption, 1, 0, 0, 0);
    m_lbDesc.SetSelectedControl(&m_lblDescription, 0);
}
#endif

// Per-frame spin of the previewed item's model.

void CSWGuiUpgrade::Draw(float f)
{
    if (g_bDo3DGUIs)
    {
// The timer scene by default; menu type 1 shows the character view.

        CSWGuiScene* pScene = &m_viewItem.m_scene;
        if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER)
            pScene = &m_viewChar.m_scene;
        CAurObject* pModel = pScene->GetModel(1);
        if (pModel != 0 && pModel->GetBehavior(-1) == 0)
        {
            pModel->SetOrientation(pModel->GetOrientation() *
                                   YawPitchRoll(f * -70.0f, 0.0f, 0.0f));
        }
    }
    CSWGuiPanel::Draw(f);
}

// Appearance-table model and texture resrefs for the armor slot.

void CSWGuiUpgrade::GetArmorModelAndTextureResRef(int a, int b, int c,
                                                 CExoString& rModel, CExoString& rTexture)
{
    char szModelCol[7] = "ModelA";  // the trailing 'A' is replaced with the slot letter
    char szTexCol[5] = "TEXA";
// 'A' for slot < 1, otherwise 'A' + (slot - 1); both strings use the same letter.

#if KOTOR1  // K1: ten armour model letters (A..J)
    int nSlot = (b < 10) ? b : 10;
    int n = (c > 0) ? c : 1;
    char cLetter = static_cast<char>(((nSlot > 1) ? nSlot : 1) + ('A' - 1));
#else
    int nSlot = (b < 14) ? b : 14;
    char cLetter = (nSlot < 1) ? 'A' : static_cast<char>(nSlot + ('A' - 1));
#endif
    szModelCol[5] = cLetter;
    szTexCol[3] = cLetter;
#if !KOTOR1
    int n = (c > 0) ? c : 1;
#endif

    if (!g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(a, CExoString(szModelCol), &rModel))
        rModel = "";
    if (!g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(a, CExoString(szTexCol), &rTexture))
    {
        rTexture = "";
    }
    else
    {
        rTexture.Format(const_cast<char*>("%s%02d"), rTexture.CStr(), n);
        if (!g_pExoResMan->Exists(CResRef(rTexture), 3, static_cast<unsigned int*>(0)) &&
            !g_pExoResMan->Exists(CResRef(rTexture), RESTYPE_TPC, static_cast<unsigned int*>(0)))
            rTexture.Format(const_cast<char*>("%s%02d"), rModel.CStr(), 1);
    }
}

// Hands every pending upgrade item back to the party repository.

void CSWGuiUpgrade::CancelPanel()
{
    delete m_pItem;  // owned only here: the panel teardown just nulls it
    m_pItem = m_pItemSlot;
    m_pItemSlot = 0;

    CItemRepository* pRepo =
        g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems();
    CSWSItem* pItem;
    for (int i = 0; i < m_lstItemsTakenFromRepo.m_count; i++)
    {
        pItem = m_lstItemsTakenFromRepo.m_pArray[i];
// First matching slot wins.

        if (m_apaItems[0] == pItem)
            m_apaItems[0] = 0;
        else if (m_apaItems[1] == pItem)
            m_apaItems[1] = 0;
        else if (m_apaItems[2] == pItem)
            m_apaItems[2] = 0;
        else if (m_apaItems[3] == pItem)
            m_apaItems[3] = 0;
#if !KOTOR1  // K1 has four slots
        else if (m_apaItems[4] == pItem)
            m_apaItems[4] = 0;
        else if (m_apaItems[5] == pItem)
            m_apaItems[5] = 0;
#endif
        pRepo->AddItem(&pItem, 1, 0, 0);
    }
    for (int i = 0; i < m_lstItemsAddedToRepo.m_count; i++)
    {
        pItem = m_lstItemsAddedToRepo.m_pArray[i];
        if (pItem->m_stackCount >= 2)
        {
            pItem->m_stackCount--;
        }
        else
        {
            pRepo->RemoveItem(pItem);
            delete pItem;
        }
    }

    m_pItemSelect->FinishUpgrading();
    {
#if KOTOR1  // K1: the panel state bits sit one higher
        unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
        if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
            nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#else
        unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
        if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
            nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#endif
        m_stateFlags = nFlags;
    }
    m_pGuiManager->PopModalPanel();
}

// Builds one row widget for an upgrade slot.

#if KOTOR1  // K1: no locked rows, so no disabled tint and no disabled handler
CSWUpgradeItemEntry* CSWGuiUpgrade::CreateItemEntry(unsigned long long oidItem, int nSlot)
{
    CSWGuiButton* pProto = static_cast<CSWGuiButton*>(m_lbItems.m_pProtoItem);
    int nWide = m_lbItems.m_innerExtent.w - 2 * static_cast<int>(m_lbItems.m_nPadding);
    CSWUpgradeItemEntry* pEntry = new CSWUpgradeItemEntry;
    pEntry->Initialize(pProto->m_text, pProto->m_border, pProto->m_borderHilight, nWide);
    pEntry->AddEvent(SWGUIEVENT_FOCUS, this,
                     static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnControlEntered));
    pEntry->AddEvent(INPUTEVENT_GUI_A, this,
                     static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnUpgradeSelected));
    pEntry->AddEvent(INPUTEVENT_GUI_START, this,
                     static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnUpgradeSelected));
    pEntry->SetItem(oidItem, 0, OBJECT_INVALID, 0);
    pEntry->m_nUpgradeRow = static_cast<unsigned char>(nSlot);
    return pEntry;
}
#else
CSWUpgradeItemEntry* CSWGuiUpgrade::CreateItemEntry(unsigned long long oidItem, int nSlot, int nMenuType)
{
    CSWGuiButton* pProto = static_cast<CSWGuiButton*>(m_lbItems.m_pProtoItem);
    int nWide = m_lbItems.m_innerExtent.w - 2 * static_cast<int>(m_lbItems.m_nPadding);
    CSWUpgradeItemEntry* pEntry = new CSWUpgradeItemEntry;
    pEntry->Initialize(pProto->m_text, pProto->m_border, pProto->m_borderHilight,
                       nWide, nMenuType);
    pEntry->AddEvent(SWGUIEVENT_FOCUS, this,
                     static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnControlEntered));
    pEntry->m_nFlags =
        static_cast<unsigned char>((pEntry->m_nFlags & ~2u) | ((static_cast<unsigned>(nMenuType) & 1u) << 1));
    void (CSWGuiObject::*pFn)(CSWGuiControl*);
    if (nMenuType != 0)
    {
        pEntry->m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_DARK_TAN);
        pEntry->m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_DARK_TAN);
        pFn = static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnDisabledSelected);
    }
    else
    {
        pFn = static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnUpgradeSelected);
    }
    pEntry->AddEvent(INPUTEVENT_GUI_A, this, pFn);
    pEntry->AddEvent(INPUTEVENT_GUI_START, this, pFn);
    pEntry->SetItem(oidItem, 0, OBJECT_INVALID, 0);
    pEntry->m_nUpgradeRow = nSlot;
    return pEntry;
}
#endif

// Only the two 2DA tables are released explicitly; the widget wall tears down implicitly.

CSWGuiUpgrade::~CSWGuiUpgrade()
{
    m_lbItems.ClearItems();
    m_lbDesc.ClearItems();
    if (m_pUpgradeC2DA) {
        delete m_pUpgradeC2DA;
        m_pUpgradeC2DA = 0;
    }
    if (m_pUpgradeC2DA2) {
        delete m_pUpgradeC2DA2;
        m_pUpgradeC2DA2 = 0;
    }
}

// Slot counts for menu types 1 to 4, one byte per type; 0 for anything else.

#if KOTOR1
unsigned char CSWGuiUpgrade::GetNumUpgrades()
#else
int CSWGuiUpgrade::GetNumUpgrades()
#endif
{
#if KOTOR1  // K1: 3, 4, 3, 2 slots
    switch (m_nMenuType)
    {
    case CSWSItem::UPGRADABLE_TYPE_LIGHTSABER: return 3;
    case CSWSItem::UPGRADABLE_TYPE_RANGED: return 4;
    case CSWSItem::UPGRADABLE_TYPE_MELEE: return 3;
    case CSWSItem::UPGRADABLE_TYPE_ARMOR: return 2;
    }
    return 0;
#else
    int n = m_nMenuType - 1;
    if (static_cast<unsigned>(n) <= 3)
        return 0x02030306u >> ((static_cast<unsigned long long>(n) & 0xffu) * 8);
    return 0;
#endif
}

#if KOTOR1  // K1: layout "upgrade", LB_DESC_LS, blue and yellow tints, no OK button
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);
CSWGuiUpgrade::CSWGuiUpgrade(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    CExoString sName;

    C2DA* pC2DA = new C2DA(CResRef("upgrade"), 0);
    m_pUpgradeC2DA = pC2DA;
    m_pUpgradeC2DA2 = new C2DA(CResRef("upcrystals"), 0);
    m_pUpgradeC2DA->Load2DArray();
    m_pUpgradeC2DA2->Load2DArray();

    StartLoadFromLayout(CResRef("upgrade"));

    InitControl(m_lblTitle, CExoString("LBL_TITLE"), 1);
    InitControl(m_lbItems, CExoString("LB_ITEMS"), 1);
    InitControl(m_lbDesc, CExoString("LB_DESC"), 1);
    m_lbDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_lbDescLS, CExoString("LB_DESC_LS"), 1);
    m_lbDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_lblDescBg, CExoString("LBL_DESCBG"), 1);
    InitControl(m_lblDescBgLS, CExoString("LBL_DESCBG_LS"), 1);
    {
        CSWGuiExtent ext = m_lbDesc.m_extent;
        ext.w -= 30;
        m_lbDesc.SetExtent(ext);
    }
    {
        CSWGuiExtent ext = m_lbDescLS.m_extent;
        ext.w -= 30;
        m_lbDescLS.SetExtent(ext);
    }
    InitControl(m_viewItem, CExoString("3D_MODEL"), 1);
    InitControl(m_viewChar, CExoString("3D_MODEL_LS"), 1);
    InitControl(m_lblSlotName, CExoString("LBL_SLOTNAME"), 1);
    InitControl(m_lblLSSlotName, CExoString("LBL_LSSLOTNAME"), 1);
    InitControl(m_lblUpgrades, CExoString("LBL_UPGRADES"), 1);
    InitControl(m_lblUpgradeCount, CExoString("LBL_UPGRADE_COUNT"), 1);
    InitControl(m_lblProperty, CExoString("LBL_PROPERTY"), 1);
    InitControl(m_btnAssemble, CExoString("BTN_ASSEMBLE"), 1);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1);

    m_btnAssemble.m_tail.m_nClickSound = GUISOUND_UPGRADE;
    m_btnAssemble.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnAssemble));
    m_btnAssemble.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnAssemble));
    m_btnAssemble.m_tail.bFocusOnHover = 0;

    for (int i = 0; i < 4; i++) {
        if (i != 3) {
            sName.Format(const_cast<char*>("LBL_UPGRADE3%d"), i + 1);
            InitControl(m_aslotLbl[i], sName, 1);
            sName.Format(const_cast<char*>("BTN_UPGRADE3%d"), i + 1);
            InitControl(m_abtnSlot[i], sName, 1);
            m_abtnSlot[i].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnSlotSelected));
            m_abtnSlot[i].AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnSlotSelected));
            m_abtnSlot[i].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnEnterSlot));
            m_abtnSlot[i].m_tail.m_nUserData = i;
            m_abtnSlot[i].m_borderHilight.m_params.SetPulsingAlpha(1, 0, 1);
        }
        sName.Format(const_cast<char*>("LBL_UPGRADE4%d"), i + 1);
        InitControl(m_apRows[i], sName, 1);
        sName.Format(const_cast<char*>("BTN_UPGRADE4%d"), i + 1);
        InitControl(m_abtnRow[i], sName, 1);
        m_abtnRow[i].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnSlotSelected));
        m_abtnRow[i].AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnSlotSelected));
        m_abtnRow[i].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnEnterSlot));
        m_abtnRow[i].m_tail.m_nUserData = i;
        m_abtnRow[i].m_borderHilight.m_params.SetPulsingAlpha(1, 0, 1);
        if (i != 3)
            m_abtnSlot[i].AddEvent(INPUTEVENT_GUI_X, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnAssemble));
        m_abtnRow[i].AddEvent(INPUTEVENT_GUI_X, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnAssemble));
    }
    StopLoadFromLayout();
    ScaleExtentForResolution();

    m_lblSlotName.m_border.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    m_lblLSSlotName.m_border.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    m_lblUpgradeCount.m_border.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    m_lblUpgrades.m_border.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    m_lblProperty.m_border.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    m_lbItems.m_border.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    m_lblDescription.Initialize(m_lbDescLS.m_pProtoItem->m_extent,
                           static_cast<CSWGuiLabel*>(m_lbDescLS.m_pProtoItem), 1.0f);
    CSWGuiButton* pBtn = static_cast<CSWGuiButton*>(m_lbItems.m_pProtoItem);
    pBtn->m_border.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    pBtn->m_borderHilight.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
    pBtn->m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);

    m_nMenuType = CSWSItem::UPGRADABLE_TYPE_NONE;
    m_pItem = 0;
    m_pItemSlot = 0;
    m_bItemsOpen &= 0xfe;
    __builtin_memset(m_apaItems, 0, sizeof(m_apaItems));

    if (g_bDo3DGUIs) {
        m_viewItem.m_scene.m_pScene->SpawnRoom(const_cast<char*>("gui3D_room"), Vector(0, 0, 0), Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
        m_viewItem.m_scene.AddModel(CExoString("upgitem_light"), -1);
        m_viewChar.m_scene.m_pScene->SpawnRoom(const_cast<char*>("gui3D_room"), Vector(0, 0, 0), Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
        m_viewChar.m_scene.AddModel(CExoString("upgitem_light"), -1);
    }

    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    IosAddButtonIcon(&m_btnAssemble, 'x');
    IosAddButtonIcon(&m_btnBack, 'b');
}
#else
CSWGuiUpgrade::CSWGuiUpgrade(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    CExoString sName;  // format scratch

    C2DA* pC2DA = new C2DA(CResRef("upgrade"), 0);
    m_pUpgradeC2DA = pC2DA;
    m_pUpgradeC2DA2 = new C2DA(CResRef("upcrystals"), 0);
    m_pUpgradeC2DA->Load2DArray();
    m_pUpgradeC2DA2->Load2DArray();

    StartLoadFromLayout(CResRef("upgrade_p"), 1, 1, 0);

    InitControl(m_lblTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_lblTitle2, CExoString("LBL_TITLE2"), 1, 1, 0, 0, 0);
    InitControl(m_lbItems, CExoString("LB_ITEMS"), 1, 1, 0, 0, 0);
    InitControl(m_lbDesc, CExoString("LB_DESC"), 1, 1, 0, 0, 0);
    m_lbDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_lblDescBg, CExoString("LBL_DESCBG"), 1, 1, 0, 0, 0);
    InitControl(m_lblDescBgLS, CExoString("LBL_DESCBG_LS"), 1, 1, 0, 0, 0);
    InitControl(m_viewItem, CExoString("3D_MODEL"), 1, 1, 0, 0, 0);
    InitControl(m_viewChar, CExoString("3D_MODEL_LS"), 1, 1, 0, 0, 0);
    InitControl(m_lblSlotName, CExoString("LBL_SLOTNAME"), 1, 1, 0, 0, 0);
    InitControl(m_lblProperty, CExoString("LBL_PROPERTY"), 1, 1, 0, 0, 0);
    InitControl(m_btnAssemble, CExoString("BTN_ASSEMBLE"), 1, 1, 0, 0, 0);
    InitControl(m_btnOk, CExoString("BTN_OK"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);

    m_btnAssemble.m_tail.m_nClickSound = GUISOUND_UPGRADE;
    m_btnAssemble.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnAssemble));
    m_btnAssemble.AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnAssemble));
    m_btnAssemble.m_tail.bFocusOnHover = 0;
    m_btnOk.m_tail.bFocusOnHover = 0;
    m_btnOk.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnUpgradeSelected));

    for (int i = 0; i < 3; i++) {  // the three upgrade-slot widgets
        sName.Format(const_cast<char*>("LBL_UPGRADE3%d"), i + 1);
        InitControl(m_aslotLbl[i], sName, 1, 1, 0, 0, 0);
        sName.Format(const_cast<char*>("BTN_UPGRADE3%d"), i + 1);
        InitControl(m_abtnSlot[i], sName, 1, 1, 0, 0, 0);
        m_abtnSlot[i].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnSlotSelected));
        m_abtnSlot[i].AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnSlotSelected));
        m_abtnSlot[i].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnEnterSlot));
        m_abtnSlot[i].m_tail.m_nUserData = i;
        m_abtnSlot[i].m_borderHilight.m_params.SetPulsingAlpha(1, 0, 1);
    }
    for (int i = 0; i < 6; i++) {  // the six _LS rows
        sName.Format(const_cast<char*>("LBL_UPGRADE3%d_LS"), i + 1);
        InitControl(m_apRows[i], sName, 1, 1, 0, 0, 0);
        sName.Format(const_cast<char*>("BTN_UPGRADE3%d_LS"), i + 1);
        InitControl(m_abtnRow[i], sName, 1, 1, 0, 0, 0);
        m_abtnRow[i].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnSlotSelected));
        m_abtnRow[i].AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnSlotSelected));
        m_abtnRow[i].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgrade::OnEnterSlot));
        m_abtnRow[i].m_tail.m_nUserData = i;
        m_abtnRow[i].m_borderHilight.m_params.SetPulsingAlpha(1, 0, 1);
    }
    StopLoadFromLayout();

    m_lbItems.bSelectOnLeftClick = 1;
    m_lblSlotName.m_border.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_UPGRADE_BORDER);
    m_lblProperty.m_border.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_UPGRADE_BORDER);
    m_lbItems.m_border.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_UPGRADE_BORDER);
    CSWGuiLabel* pProto = static_cast<CSWGuiLabel*>(m_lbDesc.m_pProtoItem);
    m_lblDescription.Initialize(pProto->m_extent, pProto, 1.0f);
    CSWGuiButton* pBtn = static_cast<CSWGuiButton*>(m_lbItems.m_pProtoItem);
    pBtn->m_border.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    pBtn->m_borderHilight.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
    pBtn->m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);

    m_nMenuType = CSWSItem::UPGRADABLE_TYPE_NONE;
    m_pItem = 0;
    m_pItemSlot = 0;
    m_bItemsOpen &= 0xfe;
    __builtin_memset(m_apaItems, 0, sizeof(m_apaItems));

    if (g_bDo3DGUIs) {
        m_viewItem.m_scene.m_pScene->SpawnRoom(const_cast<char*>("gui3D_room"), Vector(0, 0, 0), Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
        m_viewItem.m_scene.AddModel(CExoString("upgitem_light"), -1);
        m_viewChar.m_scene.m_pScene->SpawnRoom(const_cast<char*>("gui3D_room"), Vector(0, 0, 0), Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
        m_viewChar.m_scene.AddModel(CExoString("upgitem_light"), -1);
    }

    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    AspyrAddButtonIcon(&m_btnBack, 'b');
    AspyrAddButtonIcon(&m_btnAssemble, 'x');
    AspyrAddButtonIcon(&m_btnOk, 'a');
    SetActiveControl(&m_lbItems, 0);
}
#endif


// Stamps the row's item state, then picks the icon and the count backdrop for the stack size.

#if KOTOR1  // K1: lbl_hex_3/6/7 backdrops, blue count text, "inone" for an empty row
void CSWUpgradeItemEntry::SetItem(unsigned long long oidItem, int nMenu, unsigned long long a,
                                  unsigned int b) {
    CResRef refIcon;
    m_oidItem = oidItem;
    m_oidEquippedBy = a;
    m_nEquippedSlot = b;
    m_nFlags = static_cast<unsigned char>((m_nFlags & ~1u) | (static_cast<unsigned>(nMenu) & 1u));

    CSWSItem* pItem;
    if (oidItem == OBJECT_INVALID) {
        m_borderFrame.m_params.SetFillImage(CResRef("lbl_hex_3"), 0);
        m_borderFrameHilight.m_params.SetFillImage(CResRef("lbl_hex_3"), 0);
        m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NONE_LABEL));
        m_textCount.m_params = CSWGuiTextParams(CExoString(), CResRef("fnt_d16x16"),
                                                CGuiInGame::COLOR_BLUE, SWGUIALIGN_BOTTOM | SWGUIALIGN_CENTER, 1.0f);
        refIcon = const_cast<char*>("inone");
    } else {
      CClientExoApp* pClientApp = g_pAppManager->m_pClientExoApp;
      CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
      pItem = pServerApp->GetItemByGameObjectID(oidItem);
      if (pItem == 0) {
        m_borderFrame.m_params.SetFillImage(CResRef("lbl_hex_3"), 0);
        m_borderFrameHilight.m_params.SetFillImage(CResRef("lbl_hex_3"), 0);
        m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NONE_LABEL));
        m_textCount.m_params = CSWGuiTextParams(CExoString(), CResRef("fnt_d16x16"),
                                                CGuiInGame::COLOR_BLUE, SWGUIALIGN_BOTTOM | SWGUIALIGN_CENTER, 1.0f);
        refIcon = const_cast<char*>("inone");
    } else {
        CExoString sName;
        pItem->m_localizedName.GetString(pClientApp->GetClientLanguage(), &sName, 0);
        if (m_nFlags & 1)
            sName = sName + CExoString(" (") +
                    g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_EQUIPPED) + CExoString(")");
        m_text.m_params.SetText(sName);

        CExoString sCount;
        int nCount = pItem->m_stackCount;
        if (nCount >= 2) {
            if (nCount >= 100) {
                m_borderFrame.m_params.SetFillImage(CResRef("lbl_hex_7"), 0);
                m_borderFrameHilight.m_params.SetFillImage(CResRef("lbl_hex_7"), 0);
            } else {
                m_borderFrame.m_params.SetFillImage(CResRef("lbl_hex_6"), 0);
                m_borderFrameHilight.m_params.SetFillImage(CResRef("lbl_hex_6"), 0);
            }
            sCount.Format(const_cast<char*>("%d"), nCount);
        } else {
            m_borderFrame.m_params.SetFillImage(CResRef("lbl_hex_3"), 0);
            m_borderFrameHilight.m_params.SetFillImage(CResRef("lbl_hex_3"), 0);
        }
        m_textCount.m_params = CSWGuiTextParams(sCount, CResRef("fnt_d16x16"),
                                                CGuiInGame::COLOR_BLUE, SWGUIALIGN_BOTTOM | SWGUIALIGN_CENTER, 1.0f);
        refIcon = pItem->GetIcon();
      }
    }
    m_borderIcon.m_params.m_flags.nFillStyle = 2;
    m_borderIcon.m_params.SetFillImage(refIcon, 0);
}
#else
void CSWUpgradeItemEntry::SetItem(unsigned long long oidItem, int nMenu, unsigned long long a,
                                  unsigned int b) {
    CResRef refIcon;
    m_oidItem = oidItem;
    m_oidEquippedBy = a;
    m_nEquippedSlot = b;
    m_nFlags = static_cast<unsigned char>((m_nFlags & ~1u) | (static_cast<unsigned>(nMenu) & 1u));

    if (oidItem == OBJECT_INVALID) {
        m_borderFrame.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM1"), 0);
        m_borderFrameHilight.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM1"), 0);
        m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NONE_LABEL));
        m_textCount.m_params = CSWGuiTextParams(CExoString(), CResRef("fnt_d16x16"),
                                                CGuiInGame::COLOR_BLUE, SWGUIALIGN_BOTTOM | SWGUIALIGN_CENTER, 1.0f);
        refIcon = const_cast<char*>("inone_blue");
    } else {
        CClientExoApp* pClientApp = g_pAppManager->m_pClientExoApp;
        CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
        CSWSItem* pItem = pServerApp->GetItemByGameObjectID(oidItem);
        if (pItem == 0) {
            m_borderFrame.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM1"), 0);
            m_borderFrameHilight.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM1"), 0);
            m_text.m_params.SetText(g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NONE_LABEL));
            m_textCount.m_params = CSWGuiTextParams(CExoString(), CResRef("fnt_d16x16"),
                                                    CGuiInGame::COLOR_UPGRADE_TEXT, SWGUIALIGN_BOTTOM | SWGUIALIGN_CENTER, 1.0f);
            m_textCount.m_params.SetText(CExoString(""));
            refIcon = const_cast<char*>("inone_blue");
        } else {
            CExoString sName;
            pItem->m_localizedName.GetString(pClientApp->GetClientLanguage(), &sName, 0);
            if (m_nFlags & 1)
                sName = sName + CExoString(" (") +
                        g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_EQUIPPED) + CExoString(")");
            int nAngle = sName.Find('<', 0);
            int nBrace = sName.Find('{', 0);
            if (nAngle != -1 || nBrace != -1) {
                CSWCCreature* pPC = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
                g_pSWTlkTable->ParseStr(sName, pPC, 1, 1);
            }
            m_text.m_params.SetText(sName);

            CExoString sCount;
            int nCount = pItem->m_stackCount;
            if (nCount >= 2) {
                if (nCount >= 100) {
                    m_borderFrame.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM3"), 0);
                    m_borderFrameHilight.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM3"), 0);
                } else {
                    m_borderFrame.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM2"), 0);
                    m_borderFrameHilight.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM2"), 0);
                }
                sCount.Format(const_cast<char*>("%d"), nCount);
            } else {
                m_borderFrame.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM1"), 0);
                m_borderFrameHilight.m_params.SetFillImage(CResRef("UIBIT_EQP_ITM1"), 0);
            }
            m_textCount.m_params = CSWGuiTextParams(sCount, CResRef("fnt_d16x16"),
                                                    CGuiInGame::COLOR_UPGRADE_TEXT, SWGUIALIGN_BOTTOM | SWGUIALIGN_CENTER, 1.0f);
            m_textCount.m_params.SetText(sCount);
            refIcon = pItem->GetIcon();
        }
    }
    m_borderIcon.m_params.m_flags.nFillStyle = 2;
    m_borderIcon.m_params.SetFillImage(refIcon, 0);
}
#endif

CSWUpgradeItemEntry::CSWUpgradeItemEntry() {
    m_oidItem = OBJECT_INVALID;  // the empty-slot sentinel
    m_nUpgradeRow = static_cast<decltype(m_nUpgradeRow)>(-1);
    m_nFlags &= 0xfe;
}

void CSWUpgradeItemEntry::Draw(float fAlpha) {
    if (!m_tail.bActive) {
        m_border.Draw(fAlpha);
        m_borderFrame.Draw(fAlpha);
    } else {
        m_borderHilight.Draw(fAlpha);
        m_borderFrameHilight.Draw(fAlpha);
    }
    m_borderIcon.Draw(fAlpha);
    m_text.Draw(fAlpha);
    m_textCount.Draw(fAlpha);
}

#if KOTOR1  // K1: the icon is 56 units at K1's resolution scale
void CSWUpgradeItemEntry::SetExtent(const CSWGuiExtent& extent) {
#ifndef K2_UI_TOUCH
    // pc / xbox: the 480-line PC layout's units, as the inventory, store and skill rows.
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 480.0f;
#elif defined(K2_QOL_FULLSCREEN)   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
    int nIcon = static_cast<int>(s_fScale * 56.0f);
    if (extent.w <= nIcon)
        return;
    CSWGuiObject::SetExtent(extent);
    int nX = m_extent.x;
    int nY = m_extent.y;

    CSWGuiExtent eIcon(nX, nY, nIcon, nIcon);
    m_borderFrame.m_extent = eIcon;
    m_borderFrameHilight.m_extent = eIcon;
    m_borderIcon.m_extent = eIcon;
    CSWGuiExtent eText = eIcon;

    int nDigits = (m_textCount.m_params.m_sText.GetLength() > 2) ? 42 : 21;
    int nText = static_cast<int>(s_fScale * static_cast<float>(nDigits));
    eText.x = m_extent.x + nIcon - nText;
    eText.w = static_cast<int>(s_fScale * 5.0f + static_cast<float>(nText));
    eText.h = nIcon;
    m_textCount.SetExtent(eText);

    m_border.m_extent = CSWGuiExtent(nX + nIcon, nY, m_extent.w - nIcon, nIcon);
    m_borderHilight.m_extent = CSWGuiExtent(nX + nIcon, nY, m_extent.w - nIcon, nIcon);
    m_text.SetExtent(CSWGuiExtent::Intersection(m_border.GetInnerExtent(),
                                               m_borderHilight.GetInnerExtent()));
}
#else
void CSWUpgradeItemEntry::SetExtent(const CSWGuiExtent& extent) {
    if (extent.w < 37)
        return;
    m_extent = extent;
    int nX = m_extent.x;
    int nY = m_extent.y;

    float fScale = GetResolutionTextScalar();
    float fIcon = fScale * 36.0f;
#ifdef K2_QOL_IMPROVEMENTS
// A row taller than the icon is framed as tall as itself.

    if (static_cast<float>(m_extent.h) > fIcon)
        fIcon = static_cast<float>(m_extent.h);
#endif
    int nIcon = static_cast<int>(fIcon);

    CSWGuiExtent eIcon(nX, nY, nIcon, nIcon);
    m_borderFrame.m_extent = eIcon;
    m_borderFrameHilight.m_extent = eIcon;
    m_borderIcon.m_extent = eIcon;
    CSWGuiExtent eText = eIcon;

    float fTextH = fScale * static_cast<float>((m_textCount.m_params.m_sText.GetLength() > 2) ? 42 : 21);
    float fLabel = fScale * 19.0f;

    eText.x = static_cast<int>(static_cast<float>(nX) + (fIcon - fTextH));
    eText.y = static_cast<int>(static_cast<float>(nY) + (fIcon - fLabel));
    eText.w = static_cast<int>(fTextH + 1.0f);
    eText.h = static_cast<int>(fLabel + 1.0f);
    m_textCount.SetExtent(eText);

    m_border.m_extent = CSWGuiExtent(static_cast<int>(static_cast<float>(nX) + fIcon), nY,
                                     static_cast<int>(static_cast<float>(m_extent.w) - fIcon), nIcon);
    m_borderHilight.m_extent = CSWGuiExtent(static_cast<int>(static_cast<float>(nX) + fIcon), nY,
                                            static_cast<int>(static_cast<float>(m_extent.w) - fIcon), nIcon);
    m_text.SetExtent(CSWGuiExtent::Intersection(m_border.GetInnerExtent(),
                                               m_borderHilight.GetInnerExtent()));
}
#endif

#if KOTOR1  // K1: blue and yellow frames, lbl_hex_3 backdrops, row height 56 units
void CSWUpgradeItemEntry::Initialize(CSWGuiText& text, CSWGuiBorder& borderA, CSWGuiBorder& borderB,
                                     int nW) {
    m_text.m_params = text.m_params;
    m_border.m_params = borderA.m_params;
    m_border.m_params.m_color = reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_BLUE);
    m_borderHilight.m_params = borderB.m_params;
    m_borderHilight.m_params.m_color = reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_YELLOW);
    m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);

    CSWGuiBorderParams* pParams = &m_borderFrame.m_params;
    *pParams = CSWGuiBorderParams(CResRef(""), CResRef(""), CResRef("lbl_hex_3"),
                                  0, 0, 0, 1.0f, Vector(1.0f, 1.0f, 1.0f), 0, 0);
    pParams->m_color = reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_BLUE);

    pParams = &m_borderFrameHilight.m_params;
    *pParams = CSWGuiBorderParams(CResRef(""), CResRef(""), CResRef("lbl_hex_3"),
                                  0, 0, 0, 1.0f, Vector(1.0f, 1.0f, 1.0f), 0, 0);
    pParams->m_color = reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_YELLOW);
    pParams->SetPulsingAlpha(1, 0, 1);

#if !defined(K2_UI_TOUCH) || defined(K2_FIX_ENGINE_BUGS)
    // the frame squares are drawn larger than lbl_hex_3's 64 px on a large screen: stretch, do not tile
    m_borderFrame.m_params.m_flags.nFillStyle = 2;
    m_borderFrameHilight.m_params.m_flags.nFillStyle = 2;
#endif
#ifndef K2_UI_TOUCH
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 480.0f;   // pc / xbox: the 480-line layout's units
#elif defined(K2_QOL_FULLSCREEN)   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
    float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
    CSWGuiExtent extent(0, 0, nW, static_cast<int>(s_fScale * 56.0f));
    SetExtent(extent);
}
#else
void CSWUpgradeItemEntry::Initialize(CSWGuiText& text, CSWGuiBorder& borderA, CSWGuiBorder& borderB,
                                     int nW, int nFlag) {
    m_text.m_params = text.m_params;
    m_border.m_params = borderA.m_params;
    if (nFlag)
        m_border.m_params.m_color = reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_DARK_TAN);
    else
        m_border.m_params.m_color = reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_UPGRADE_BORDER);
    m_borderHilight.m_params = borderB.m_params;
    if (nFlag)
        m_borderHilight.m_params.m_color = reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_NEW_HIGHLIGHT);
    else
        m_borderHilight.m_params.m_color = reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_UPGRADE_TEXT);
    m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);

    CSWGuiBorderParams* pParams = &m_borderFrame.m_params;
    *pParams = CSWGuiBorderParams(CResRef(""), CResRef(""), CResRef("UIBIT_EQP_ITM1"),
                                  0, 0, 0, 1.0f, Vector(1.0f, 1.0f, 1.0f), 0, 0);
    if (nFlag)
        pParams->m_color = reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_DARK_TAN);
    else
        pParams->m_color = reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_UPGRADE_BORDER);

    pParams = &m_borderFrameHilight.m_params;
    *pParams = CSWGuiBorderParams(CResRef(""), CResRef(""), CResRef("UIBIT_EQP_ITM1"),
                                  0, 0, 0, 1.0f, Vector(1.0f, 1.0f, 1.0f), 0, 0);
    if (nFlag)
        pParams->m_color = reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_NEW_HIGHLIGHT);
    else
        pParams->m_color = reinterpret_cast<const CSWGuiColor&>(CGuiInGame::COLOR_UPGRADE_TEXT);
    pParams->SetPulsingAlpha(1, 0, 1);
#if !defined(K2_UI_TOUCH) || defined(K2_FIX_ENGINE_BUGS)
// Stretched frame, matching the equip and inventory item rows.

    m_borderFrame.m_params.m_flags.nFillStyle = 2;
    m_borderFrameHilight.m_params.m_flags.nFillStyle = 2;
#endif

    CSWGuiExtent extent(0, 0, nW, static_cast<int>(GetResolutionTextScalar() * 36.0f));
    SetExtent(extent);
}
#endif

CSWUpgradeItemEntry::~CSWUpgradeItemEntry() {}

void CSWUpgradeItemEntry::Initialize(const CSWGuiExtent&, const CSWGuiTextParams&,
                                     const CSWGuiBorderParams&, const CSWGuiBorderParams&) {}

void CSWUpgradeItemEntry::Initialize(const CSWGuiExtent&, CSWGuiButton*) {}

// K1 has no item-creation, chemical or swoop-upgrade panels.

#if !KOTOR1
// The item-creation and breakdown workbench panel. CSWGuiStore is its twin: same control names,
// same row type and row lists, but this sheet trades in components and owns a temporary store.
































void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);


// A macro, not a function; both arms re-evaluate their argument.

#define CSWGCREATEITEM_MAX(a, b) ((a) > (b) ? (a) : (b))


extern CTlkTable* g_pTlkTable;

CSWGuiCreateItem::CSWGuiCreateItem(CSWGuiManager* pGuiManager, unsigned long long oidOwner)
    : CSWGuiPanel(pGuiManager)
{
    m_nReserved2 = 0;

    StartLoadFromLayout(CResRef("component_p"), 1, 1, 0);

    InitControl(m_lbShopItems, CExoString("LB_SHOPITEMS"), 1, 1, 0, 0, 0);
    InitControl(m_lbInvItems, CExoString("LB_INVITEMS"), 1, 1, 0, 0, 0);
    InitControl(m_lblCredits, CExoString("LBL_CREDITS"), 1, 1, 0, 0, 0);
    InitControl(m_lblCreditsValue, CExoString("LBL_CREDITS_VALUE"), 1, 1, 0, 0, 0);
    InitControl(m_btnAccept, CExoString("BTN_Accept"), 1, 1, 0, 0, 0);
    InitControl(m_btnCancel, CExoString("BTN_Cancel"), 1, 1, 0, 0, 0);
    InitControl(m_btnExamine, CExoString("BTN_Examine"), 1, 1, 0, 0, 0);
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateItem::CloseCreateItem));
    m_btnExamine.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateItem::OnControlXButton));
    InitControl(m_aCreateButtons[0], CExoString("BTN_CREATE_ARMOR"), 1, 1, 0, 0, 0);
    InitControl(m_aCreateButtons[1], CExoString("BTN_CREATE_RANGED"), 1, 1, 0, 0, 0);
    InitControl(m_aCreateButtons[2], CExoString("BTN_CREATE_MELEE"), 1, 1, 0, 0, 0);
    InitControl(m_aCreateButtons[3], CExoString("BTN_CREATE_LIGHTSABER"), 1, 1, 0, 0, 0);
    InitControl(m_aCreateButtons[4], CExoString("BTN_CREATE_MISC"), 1, 1, 0, 0, 0);
    InitControl(m_aBreakButtons[0], CExoString("BTN_BREAK_ALL"), 1, 1, 0, 0, 0);
    InitControl(m_aBreakButtons[1], CExoString("BTN_BREAK_WEAPONS"), 1, 1, 0, 0, 0);
    InitControl(m_aBreakButtons[2], CExoString("BTN_BREAK_ARMOR"), 1, 1, 0, 0, 0);
    InitControl(m_aBreakButtons[3], CExoString("BTN_BREAK_USEABLE"), 1, 1, 0, 0, 0);
    InitControl(m_aBreakButtons[4], CExoString("BTN_BREAK_MISC"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[0], CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[1], CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[2], CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
    InitControl(m_lblTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_lblTitle2, CExoString("LBL_TITLE2"), 1, 1, 0, 0, 0);
    InitControl(m_lblRelatedSkill, CExoString("LBL_RELATEDSKILL"), 1, 1, 0, 0, 0);
    InitControl(m_lbDescriptionInv, CExoString("LB_DESCRIPTIONINV"), 1, 1, 0, 0, 0);
    InitControl(m_lbDescription, CExoString("LB_DESCRIPTION"), 1, 1, 0, 0, 0);
    m_lbDescription.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_lblCost, CExoString("LBL_COST"), 1, 1, 0, 0, 0);
    InitControl(m_lblCostValue, CExoString("LBL_COST_VALUE"), 1, 1, 0, 0, 0);
    InitControl(m_lblStock, CExoString("LBL_STOCK"), 1, 1, 0, 0, 0);
    InitControl(m_lblStockValue, CExoString("LBL_STOCK_VALUE"), 1, 1, 0, 0, 0);
    InitControl(m_lblRandAnalog, CExoString("LBL_RANALOG"), 1, 1, 0, 0, 0);

    StopLoadFromLayout();

    m_lbShopItems.bSelectOnLeftClick = 1;
    m_lbInvItems.bSelectOnLeftClick = 1;

    CSWGuiLabel* pProto = static_cast<CSWGuiLabel*>(m_lbDescription.m_pProtoItem);
    m_lblDescriptionText.Initialize(pProto->m_extent, pProto, 1.0f);

    m_lbShopItems.bShowScrollBar = 1;
    m_lbInvItems.bShowScrollBar = 1;
    SetActiveControl(&m_lbShopItems, 0);
    m_lbInvItems.m_tail.bVisible = 0;
    m_nReserved1 = 0;
    m_nReserved3 = 0;
    m_fReserved1 = 0.0f;
    m_nComponent = 0;
    m_pStore = new CSWSStore(OBJECT_INVALID);
    m_nCreateFilter = CREATE_FILTER_ARMOR;

    CServerExoApp* pServerExoApp = g_pAppManager->m_pServerExoApp;
    CSWSCreature* pCreature;
    CSWSPlayer* pPlayer;
    if (oidOwner == OBJECT_INVALID)
    {
        pPlayer = static_cast<CSWSClient*>(pServerExoApp->GetPlayerList()->m_pInternal->m_pHead->m_pData)->AsSWSPlayer();
        pCreature = pServerExoApp->GetCreatureByGameObjectID(pPlayer->m_gameObjectId);
    }
    else
    {
        pCreature = pServerExoApp->GetCreatureByGameObjectID(oidOwner);
    }

    m_oidPlayer = pCreature->m_objectId;
    float fYield = static_cast<float>(pCreature->m_pStats->GetSkillRank(SKILL_REPAIR, NULL, 0)) / 20.0f;
    m_fBreakdownYield = fYield > 1.0f ? 1.0f : fYield;
    m_bViewSwitched = 0;
    m_nFullScreenBGIndex = 1;

    int i;
    for (i = 0; i < 5; i++)
    {
        m_aCreateButtons[i].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateItem::OnCreateFilterPicked));
        m_aCreateButtons[i].AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateItem::OnCreateFilterPicked));
        m_aCreateButtons[i].m_tail.bFocusOnHover = 0;
        m_aCreateButtons[i].m_tail.m_nUserData = i;
    }
    UpdateCreateFilters();
    for (i = 0; i < 5; i++)
    {
        m_aBreakButtons[i].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateItem::OnBreakdownFilterPicked));
        m_aBreakButtons[i].AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateItem::OnBreakdownFilterPicked));
        m_aBreakButtons[i].m_tail.bFocusOnHover = 0;
        m_aBreakButtons[i].m_tail.m_nUserData = i;
    }

    m_btnAccept.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateItem::OnAcceptPicked));
    m_btnAccept.m_tail.bFocusOnHover = 0;
    m_btnExamine.m_tail.bFocusOnHover = 0;
    m_btnCancel.m_tail.bFocusOnHover = 0;
    m_nBreakdownFilter = BREAKDOWN_FILTER_ALL;
    UpdateBreakdownFilters();
}

// A component is worth a twentieth of the item's cost, and never less than one.

int CSWGuiCreateItem::GetItemSellValue(CSWSItem* pItem)
{
    int nValue = 0;
    if (pItem != NULL)
        nValue = pItem->GetCost() < 20 ? 1 : pItem->GetCost() / 20;
    return nValue;
}

int CSWGuiCreateItem::GetItemBuyValue(CSWSItem* pItem)
{
    int nValue = 0;
    if (pItem != NULL)
        nValue = pItem->GetCost() < 20 ? 1 : pItem->GetCost() / 20;
    return nValue;
}

// The count is re-read after each delete, since the deleting destructor may reallocate.

void CSWGuiCreateItem::DeleteItemEntries()
{
    m_lbInvItems.ClearItems();
    m_lbShopItems.ClearItems();

    for (int64_t i = 0; i < m_aStoreEntries.m_count; i++)
    {
        if (m_aStoreEntries.m_pArray[i])
            delete m_aStoreEntries.m_pArray[i];
        m_aStoreEntries.m_pArray[i] = 0;
    }
    m_aStoreEntries.m_count = 0;

    for (int64_t i = 0; i < m_aInvEntries.m_count; i++)
    {
        if (m_aInvEntries.m_pArray[i])
            delete m_aInvEntries.m_pArray[i];
        m_aInvEntries.m_pArray[i] = 0;
    }
    m_aInvEntries.m_count = 0;
}

void CSWGuiCreateItem::CloseCreateItem(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    if (m_pUpgradeSelection != NULL && m_pUpgradeSelection->m_bParam4 != 0)
        m_pUpgradeSelection->HandleInputEvent(INPUTEVENT_GUI_B, 1);
    else
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);

#if KOTOR1  // K1: the panel state bits sit one higher
    unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
    if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
        nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#else
    unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
    if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
        nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#endif
    m_stateFlags = nFlags;

    m_pGuiManager->PopModalPanel();
}

void CSWGuiCreateItem::OnControlXButton(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    m_bViewSwitched = (m_bViewSwitched == 0);
    m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
    if (m_lbShopItems.m_tail.bVisible)
        ShowSellGUI();
    else
        ShowBuyGUI();
}

// Create-filter click; only the shop side responds.

void CSWGuiCreateItem::OnCreateFilterPicked(CSWGuiControl* pControl)
{
    if ((pControl != NULL) & (m_bCreateMode != 0))
    {
        m_nCreateFilter = static_cast<int>(pControl->m_tail.m_nUserData);
        UpdateCreateFilters();
        SetItemList();
    }
}

// Breakdown-filter click; only the inventory side responds. The guard is a bitwise and: both
// operands are side-effect free, so the short-circuit is not observable.

void CSWGuiCreateItem::OnBreakdownFilterPicked(CSWGuiControl* pControl)
{
    if ((pControl != NULL) & (m_bCreateMode == 0))
    {
        m_nBreakdownFilter = static_cast<int>(pControl->m_tail.m_nUserData);
        UpdateBreakdownFilters();
        PopulateInventoryItemListBox();
    }
}

// Routes to whichever side's row handler is active; the row is the list box's selection.

void CSWGuiCreateItem::OnAcceptPicked(CSWGuiControl* pControl)
{
    if (m_bCreateMode != 0)
    {
        pControl = m_lbShopItems.GetSelectedControl();
        if (pControl == NULL)
            return;
        OnControlStoreAButton(pControl);
    }
    else
    {
        pControl = m_lbInvItems.GetSelectedControl();
        if (pControl == NULL)
            return;
        OnControlInvAButton(pControl);
    }
}

// One colour pair per filter button; the selected button is the one whose user data is the filter.

void CSWGuiCreateItem::UpdateCreateFilters()
{
    for (int i = 0; i < 5; i++)
        m_aCreateButtons[i].m_text.m_params.SetDefaultColor(
            m_aCreateButtons[i].m_tail.m_nUserData == static_cast<uint64_t>(m_nCreateFilter)
                ? CGuiInGame::COLOR_NEW_HIGHLIGHT : CGuiInGame::COLOR_UPGRADE_TEXT);
}

void CSWGuiCreateItem::UpdateBreakdownFilters()
{
    for (int i = 0; i < 5; i++)
        m_aBreakButtons[i].m_text.m_params.SetDefaultColor(
            m_aBreakButtons[i].m_tail.m_nUserData == static_cast<uint64_t>(m_nBreakdownFilter)
                ? CGuiInGame::COLOR_NEW_HIGHLIGHT : CGuiInGame::COLOR_UPGRADE_TEXT);
}

// The credits readout on this sheet shows components, not gold.

void CSWGuiCreateItem::PopulateStore()
{
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidPlayer);
    if (!pCreature)
        return;

    int nComponent = pCreature->GetComponent();
    m_nComponent = nComponent;
    m_lblCreditsValue.m_text.m_params.SetText(CExoString(nComponent));
}

void CSWGuiCreateItem::InitializeStoreType()
{
    CSWSStore* pStore = g_pAppManager->m_pServerExoApp->GetStoreByGameObjectID(m_oidStore);
    if (!pStore)
        return;

    switch (pStore->m_buySellFlag)
    {
    case CSWSStore::STORE_BUY_AND_SELL:
        m_lblX.m_tail.bVisible = 1;
        m_lblExamine.m_tail.bVisible = 1;
        ShowBuyGUI();
        break;
    case CSWSStore::STORE_BUY_ONLY:
        m_lblX.m_tail.bVisible = 0;
        m_lblExamine.m_tail.bVisible = 0;
        ShowBuyGUI();
        break;
    case CSWSStore::STORE_SELL_ONLY:
        m_lblX.m_tail.bVisible = 0;
        m_lblExamine.m_tail.bVisible = 0;
        ShowSellGUI();
        break;
    }
}

// Builds the buy side of the sheet; ShowSellGUI is its mirror image.

void CSWGuiCreateItem::ShowBuyGUI()
{
    m_bCreateMode = 1;

    m_lbInvItems.m_tail.bVisible = 0;
    m_lbShopItems.m_tail.bVisible = 1;
    m_lblRelatedSkill.m_tail.bVisible = 1;
    m_lblRelatedSkill.m_text.m_params.SetText(CExoString(""));
    m_lblTitle.m_text.m_params.SetStrRef(STRREF_CREATE_NEW_ITEMS);

    m_lbDescriptionInv.m_tail.bVisible = 0;
    m_lbDescription.m_tail.bVisible = 1;

    for (int i = 0; i < 5; i++)
        m_aCreateButtons[i].m_tail.bVisible = 1;
    for (int i = 0; i < 5; i++)
        m_aBreakButtons[i].m_tail.bVisible = 0;

    m_btnAccept.m_text.m_params.SetStrRef(STRREF_CREATE_ITEM);
    m_btnExamine.m_text.m_params.SetStrRef(STRREF_VIEW_BREAKDOWN_ITEMS);
    m_aBarLabels[1].m_tail.bVisible = 1;
    m_aBarLabels[2].m_tail.bVisible = 1;
    m_lblCost.m_text.m_params.SetStrRef(STRREF_COMPONENT_COST);
    m_lblAccept.m_text.m_params.SetStrRef(STRREF_CREATE_ITEM);
    m_lblExamine.m_text.m_params.SetStrRef(STRREF_VIEW_BREAKDOWN_ITEMS);
    m_lblCost.m_text.m_params.SetStrRef(STRREF_COMPONENT_COST);

    SetActiveControl(&m_lbShopItems, 0);
    PopulateStoreItemListBox();
    AspyrAddButtonIcon(&m_btnAccept, 'a');
    AspyrAddButtonIcon(&m_btnExamine, 'x');
}

void CSWGuiCreateItem::ShowSellGUI()
{
    m_bCreateMode = 0;

    m_lbInvItems.m_tail.bVisible = 1;
    m_lbShopItems.m_tail.bVisible = 0;
    m_lblRelatedSkill.m_tail.bVisible = 0;
    m_lblTitle.m_text.m_params.SetStrRef(STRREF_BREAKDOWN_INVENTORY_ITEMS);

    m_lbDescriptionInv.m_tail.bVisible = 1;
    m_lbDescription.m_tail.bVisible = 0;

    for (int i = 0; i < 5; i++)
        m_aCreateButtons[i].m_tail.bVisible = 0;
    for (int i = 0; i < 5; i++)
        m_aBreakButtons[i].m_tail.bVisible = 1;

    m_btnAccept.m_text.m_params.SetStrRef(STRREF_BREAKDOWN_ITEM);
    m_btnExamine.m_text.m_params.SetStrRef(STRREF_VIEW_CREATEABLE_ITEMS);
    m_aBarLabels[1].m_tail.bVisible = 0;
    m_aBarLabels[2].m_tail.bVisible = 0;
    m_lblCost.m_text.m_params.SetStrRef(STRREF_COMPONENT_VALUE);
    m_lblAccept.m_text.m_params.SetStrRef(STRREF_BREAKDOWN_ITEM_LABEL);
    m_lblExamine.m_text.m_params.SetStrRef(STRREF_VIEW_CREATEABLE_ITEMS);
    m_lblCost.m_text.m_params.SetStrRef(STRREF_COMPONENT_VALUE);

    SetActiveControl(&m_lbInvItems, 0);
    PopulateInventoryItemListBox();
    AspyrAddButtonIcon(&m_btnAccept, 'a');
    AspyrAddButtonIcon(&m_btnExamine, 'x');
}

// The cancel button's caption depends on whether the upgrade selection panel is live behind the sheet.

void CSWGuiCreateItem::OnPanelAdded()
{
    if (g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nItemComponent < 0)
        g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nItemComponent = 0;

    uint64_t nStrRef = STRREF_BACK;
    if (m_pUpgradeSelection != NULL && m_pUpgradeSelection->m_bParam4 != 0)
        nStrRef = STRREF_CLOSE;
    m_btnCancel.m_text.m_params.SetStrRef(nStrRef);

    m_nCreateFilter = CREATE_FILTER_MISC;
    UpdateCreateFilters();
    InitializeStoreType();
    SetItemList();
    PopulateStore();
    CSWGuiPanel::OnPanelAdded();
    ShowBuyGUI();
    AspyrAddButtonIcon(&m_btnCancel, 'b');
}

// Disposes of the row entries and every item the player created into the temporary store.

void CSWGuiCreateItem::OnPanelRemoved()
{
    DeleteItemEntries();

    CItemRepository* pRepo = m_pStore->m_pItemRepository;
    for (int i = 0; i < static_cast<int>(pRepo->ItemListGetCount()); i++)
    {
        CSWSItem* pItem = pRepo->ItemListGetItem(i);
        if (pItem != NULL)
            delete pItem;
    }

    if (m_pUpgradeSelection != NULL)
        m_pUpgradeSelection->PopulateItemList();

    CSWGuiPanel::OnPanelRemoved();
}

// The confirmation-box callbacks are the store panel's pair without its equipped-slot teardown.

void CSWGuiCreateItem::SellItemCallback(CSWGuiControl* pControl)
{
    if (!g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted)
        return;

    CSWGuiStoreItemEntry* pEntry = static_cast<CSWGuiStoreItemEntry*>(pControl);
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pEntry->m_oidItem));
    if (!pItem)
        return;

    SellItem(pItem);
}

void CSWGuiCreateItem::BuyItemCallback(CSWGuiControl* pControl)
{
    if (!g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted)
        return;

    CSWGuiStoreItemEntry* pEntry = static_cast<CSWGuiStoreItemEntry*>(pControl);
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pEntry->m_oidItem));
    if (!pItem)
        return;

    BuyItem(pItem);
}

// Points the single description row at a new string and re-fits it; which description box it
// lands in depends on the sheet's side.

void CSWGuiCreateItem::SetDescription(const CExoString& sDescription)
{
    CSWGuiListBox* pListBox = (m_bCreateMode != 0) ? &m_lbDescription : &m_lbDescriptionInv;

    m_lblDescriptionText.m_text.m_params.SetText(sDescription);
    pListBox->ClearItems();

    CSWGuiText* pText = &m_lblDescriptionText.m_text;

    CSWGuiControl* pRow;

    CSWGuiExtent extent = m_lblDescriptionText.m_extent;
    if (sDescription.GetLength() >= 1)
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
    m_lblDescriptionText.SetExtent(extent);

    pRow = &m_lblDescriptionText;
    pListBox->AddControls(&pRow, 1, 0, 0, 0);
    pListBox->SetSelectedControl(&m_lblDescriptionText, 0);

    m_lblRandAnalog.m_tail.bVisible =
        pListBox->IsOversized();
}

// Fills the shop list from the workbench's temporary store.

void CSWGuiCreateItem::PopulateStoreItemListBox()
{
    CItemRepository* pRepo = m_pStore->m_pItemRepository;

    CExoArrayList<CSWGuiControl*> aControls;
    int nIndex = 0;

    for (int i = pRepo->ItemListGetCount() - 1; i != -1; i--)
    {
        CSWSItem* pItem = pRepo->ItemListGetItem(i);
        if (pItem == NULL || pItem->m_bPlot)
            continue;

        CExoString sTag(pItem->m_sTag.CStr());
        CreateItemEntry(nIndex, aControls, pItem->m_objectId, 0, sTag);
    }

    m_lbShopItems.AddControls(aControls, 0, 1, 0);

    if (m_lbShopItems.m_tail.bVisible)
    {
        if (aControls.m_count >= 1)
        {
            m_lbShopItems.SetSelectedControl(0, 0);
        }
        else
        {
            m_lbShopItems.SetSelectedControl(-1, 0);
            m_lblCostValue.m_text.m_params.SetText(CExoString(""));
            m_lblStockValue.m_text.m_params.SetText(CExoString(""));
            m_lblDescriptionText.m_text.m_params.SetText(CExoString(""));
        }
    }

    m_nReserved3 = 0;
}

// Fills the inventory list with the items whose tag appears in the chemical breakout table
// (tutchemcreate.2da).

void CSWGuiCreateItem::PopulateInventoryItemListBox()
{
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidPlayer);

    C2DA* pArray = NULL;
    if (m_sOverride2DA.m_sString != NULL && m_sOverride2DA.m_sString[0] != 0)
    {
        pArray = new C2DA(CResRef("tutchemcreate"), 0);
        pArray->Load2DArray();
    }
    else
    {
        pArray = g_pRules->m_p2DArrays->m_pChemicalCreate;
    }

    if (pArray == NULL)
        return;

    CItemRepository* pRepo;
    if (pCreature != NULL)
        pRepo = pCreature->GetItemRepository(1);
#ifdef K2_FIX_ENGINE_BUGS
    else
        return;
#endif

    CExoArrayList<CSWGuiControl*> aControls;
    int nIndex = 0;
    CExoString sValue;

    for (unsigned i = 0; i < pRepo->ItemListGetCount(); i++)
    {
        int nMatch = 0;
        CSWSItem* pItem = pRepo->ItemListGetItem(i);

        for (int j = 0; j < pArray->m_nRows; j++)
        {
            pArray->GetCExoStringEntry(j, CExoString("label"), &sValue);

            CExoString sTag;
            sTag = pItem->m_sTag.CStr();
            if (sTag == sValue)
                nMatch = 1;
        }

        if (pItem == NULL)
            continue;
        if (pItem->m_bPlot || nMatch)
            continue;
        if (!CheckBreakdownFilter(pItem, pCreature, 0))
            continue;

        CreateItemEntry(nIndex, aControls, pItem->m_objectId, 1, CExoString(""));
    }

    m_lbInvItems.AddControls(aControls, 0, 1, 0);

    if (m_lbInvItems.m_tail.bVisible)
    {
        if (aControls.m_count >= 1)
        {
            m_lbInvItems.SetSelectedControl(0, 0);
        }
        else
        {
            m_lbInvItems.SetSelectedControl(-1, 0);
            m_lblCostValue.m_text.m_params.SetText(CExoString(""));
            m_lblStockValue.m_text.m_params.SetText(CExoString(""));
            m_lblDescriptionText.m_text.m_params.SetText(CExoString(""));
        }
    }

    if (m_sOverride2DA.m_sString != NULL && m_sOverride2DA.m_sString[0] != 0)
    {
        pArray->Unload2DArray();
        delete pArray;
    }

    m_nReserved3 = 0;
}

// Filters the breakdown side with five breakdown-button arms plus a cost gate.

int CSWGuiCreateItem::CheckBreakdownFilter(CSWSItem* pItem, CSWSCreature* pCreature, int nUnused)
{
    if (pItem == NULL || pCreature == NULL)
        return 0;

    int nResult = 1;
    switch (m_nBreakdownFilter)
    {
    case BREAKDOWN_FILTER_ALL:
        break;
    case BREAKDOWN_FILTER_WEAPONS:
        nResult = pItem->GetBaseItem() && (pItem->GetBaseItem()->m_nEquipableSlots & FILTER_SLOTS_WEAPON);
        break;
    case BREAKDOWN_FILTER_ARMOR:
        nResult = pItem->GetBaseItem() && (pItem->GetBaseItem()->m_nEquipableSlots & FILTER_SLOTS_WORN);
        break;
    case BREAKDOWN_FILTER_USEABLE:
        nResult = pCreature->GetClientCreature() != NULL
                  && pCreature->GetClientCreature()->IsLeaderUsableItem(pItem, nUnused, 0xff)
                  && pCreature->CanUseItem(pItem, 0, 1);
        break;
    case BREAKDOWN_FILTER_MISC:
        nResult = !pItem->GetBaseItem()
                  || (static_cast<int>(pItem->GetBaseItem()->m_nItemType != ITEM_TYPE_DATA_PAD)
                      & static_cast<int>(pItem->GetBaseItem()->m_nEquipableSlots == 0));
        if (pCreature->GetClientCreature() != NULL
            && pCreature->GetClientCreature()->IsLeaderUsableItem(pItem, nUnused, 0xff))
            nResult = pCreature->CanUseItem(pItem, 0, 1) ? 0 : nResult;
        break;
    default:
        nResult = 0;
        break;
    }

    return pItem->GetCost() != 0 ? nResult : 0;
}

// Destroys one unit of the item and credits the player with its converted component value.

void CSWGuiCreateItem::SellItem(CSWSItem* pItem)
{
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();

    if (pItem == NULL || pCreature == NULL)
        return;

    CItemRepository* pRepo = pCreature->GetItemRepository(1);
    if (pRepo == NULL)
        return;

    m_pGuiManager->PlayGuiSound(static_cast<char>(GUISOUND_INVENTORY_DROP));

    int nGain = CSWGCREATEITEM_MAX(1, static_cast<int>((pItem->GetCost() < 20
        ? 1.0f : static_cast<float>(pItem->GetCost() / 20)) * m_fBreakdownYield));
    m_nComponent = m_nComponent + nGain;
    pCreature->SetComponent(m_nComponent);

    if (pItem->m_stackCount >= 2)
        pItem->SplitItem(1);
    else
        pRepo->RemoveItem(pItem);

    short nSelected = m_lbInvItems.m_nSelectedIndex;

    PopulateInventoryItemListBox();

    int nControls = m_lbInvItems.m_aControls.m_count;
    m_lbInvItems.SetSelectedControl(nControls > nSelected ? nSelected : nControls - 1, 0);

    m_lblCreditsValue.m_text.m_params.SetText(CExoString(m_nComponent));
}

// Copies the item into the workbench store and charges the player for it.

void CSWGuiCreateItem::BuyItem(CSWSItem* pItem)
{
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();

    if (pCreature == NULL)
        return;

    CItemRepository* pRepo = pCreature->GetItemRepository(1);
    if (pItem == NULL || pRepo == NULL)
        return;

    m_pGuiManager->PlayGuiSound(GUISOUND_INVENTORY_ADD);

    int nCost = pItem->GetCost() < 20 ? 1 : pItem->GetCost() / 20;
    m_nComponent = m_nComponent - nCost;
    pCreature->SetComponent(m_nComponent);

    CSWSItem* pNewItem = new CSWSItem(OBJECT_INVALID);
    pNewItem->CopyItem(pItem);
    pCreature->AcquireItem(&pNewItem, m_pStore->m_objectId, OBJECT_INVALID, 1);

    int nControls = m_lbShopItems.m_aControls.m_count;
    short nSelected = m_lbShopItems.m_nSelectedIndex;
    m_lbShopItems.SetSelectedControl(nControls > nSelected ? nSelected : nControls - 1, 0);

    m_lblCreditsValue.m_text.m_params.SetText(CExoString(m_nComponent));
}

// On shop row activation, refuses the sale with a message box when the player cannot pay or is too
// low level.

void CSWGuiCreateItem::OnControlStoreAButton(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    if (pControl->m_tail.nCurrentEventType != 0)
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);

    CSWGuiStoreItemEntry* pEntry = static_cast<CSWGuiStoreItemEntry*>(pControl);

    if (pEntry->m_bCanCreate == 0)
    {
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetCallback(0, 0, 0);
        pBox->SetMessageColors(CGuiInGame::COLOR_UPGRADE_BORDER, CGuiInGame::COLOR_NEW_HIGHLIGHT);
        pBox->SetMessage(STRREF_SKILL_TOO_LOW_TO_CREATE);
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
        return;
    }

    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pEntry->m_oidItem));
    if (!pItem)
        return;

    if (static_cast<unsigned int>(m_nComponent) < (pItem->GetCost() < 20 ? 1 : pItem->GetCost() / 20))
    {
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        CExoString sMessage;
        pBox->SetAllowCancel(0);
        pBox->SetMessage(STRREF_NOT_ENOUGH_COMPONENTS);
        pBox->SetMessageColors(CGuiInGame::COLOR_UPGRADE_BORDER, CGuiInGame::COLOR_NEW_HIGHLIGHT);
        pBox->SetCallback(0, 0, 0);
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
        return;
    }

    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    int nLimit = 50;
    if (pCreature != NULL)
        nLimit = pCreature->m_pStats->GetLevel(0) * 50;

    if (static_cast<unsigned int>(pItem->GetCost() < 20 ? 1 : pItem->GetCost() / 20) > static_cast<unsigned int>(nLimit))
    {
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        CExoString sMessage;
        g_pTlkTable->SetCustomToken(0, CExoString(nLimit));

        sMessage = g_pSWTlkTable->Fetch(STRREF_CONFIRM_CREATE_COMPONENTS, 0, 0, 0);
        pBox->SetAllowCancel(1);
        pBox->SetMessage(sMessage);
        pBox->SetMessageColors(CGuiInGame::COLOR_UPGRADE_BORDER, CGuiInGame::COLOR_NEW_HIGHLIGHT);
        pBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateItem::BuyItemCallback), pControl);
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
        return;
    }

    BuyItem(pItem);
}

// Handles keys. The X key is the sheet flip, the shoulder keys step the live filter array, and the
// page-up/down pair goes to the description box.

void CSWGuiCreateItem::HandleInputEvent(int nEventType, int nEventArg)
{
    CSWGuiPanel::HandleInputEvent(nEventType, nEventArg);
    if (nEventArg == 0)
        return;

    switch (nEventType)
    {
    case INPUTEVENT_GUI_B:
    case INPUTEVENT_GUI_BACK:
        CloseCreateItem(NULL);
        break;

    case INPUTEVENT_GUI_X:
        m_bViewSwitched = (m_bViewSwitched == 0);
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        if (m_lbShopItems.m_tail.bVisible)
            ShowSellGUI();
        else
            ShowBuyGUI();
        break;

    case INPUTEVENT_GUI_DPAD_LEFT:
    case INPUTEVENT_GUI_DPAD_RIGHT:
        {
            int nDelta = (nEventType == INPUTEVENT_GUI_DPAD_LEFT) ? -1 : 1;
            if (m_bCreateMode == 0)
            {
                m_nBreakdownFilter = (m_nBreakdownFilter + nDelta + 5) % 5;
                UpdateBreakdownFilters();
                PopulateInventoryItemListBox();
            }
            else
            {
                m_nCreateFilter = (m_nCreateFilter + nDelta + 5) % 5;
                UpdateCreateFilters();
                SetItemList();
            }
        }
        break;

    case INPUTEVENT_GUI_SCROLL_UP:
        if (m_bCreateMode == 0)
            m_lbDescriptionInv.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
        else
            m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
        break;

    case INPUTEVENT_GUI_SCROLL_DOWN:
        if (m_bCreateMode == 0)
            m_lbDescriptionInv.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
        else
            m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
        break;
    }
}

// On inventory row activation, breaks the item down into components.

void CSWGuiCreateItem::OnControlInvAButton(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_START)
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);

    CSWGuiStoreItemEntry* pEntry = static_cast<CSWGuiStoreItemEntry*>(pControl);
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pEntry->m_oidItem));
    if (!pItem)
        return;

    if (pItem->m_anUpgVars[0] >= 0 || pItem->m_anUpgVars[1] >= 0 || pItem->m_anUpgVars[2] >= 0
        || pItem->m_anUpgVars[3] >= 0 || pItem->m_anUpgVars[4] >= 0 || pItem->m_anUpgVars[5] >= 0)
    {
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetMessage(STRREF_REMOVE_UPGRADES_FIRST);
        pBox->SetCallback(0, 0, 0);
        pBox->SetMessageColors(CGuiInGame::COLOR_UPGRADE_BORDER, CGuiInGame::COLOR_NEW_HIGHLIGHT);
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
        return;
    }

    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();

    unsigned int nLimit;
    if (pCreature == NULL || static_cast<unsigned int>(pCreature->m_pStats->GetLevel(0)) * 50 > 250)
        nLimit = 250;
    else
        nLimit = static_cast<unsigned int>(pCreature->m_pStats->GetLevel(0)) * 50;

    float fValue = (pItem->GetCost() < 20 ? 1.0f : static_cast<float>(pItem->GetCost() / 20)) * m_fBreakdownYield;
    if (fValue > static_cast<float>(nLimit))
    {
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        CExoString sMessage;
        g_pTlkTable->SetCustomToken(0, CExoString(nLimit));

        sMessage = g_pSWTlkTable->Fetch(STRREF_CONFIRM_BREAKDOWN_COMPONENTS, 0, 0, 0);
        pBox->SetAllowCancel(1);
        pBox->SetMessage(sMessage);
        pBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateItem::SellItemCallback), pControl);
        pBox->SetMessageColors(CGuiInGame::COLOR_UPGRADE_BORDER, CGuiInGame::COLOR_NEW_HIGHLIGHT);
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
        return;
    }

    SellItem(pItem);
}

// Fills the workbench store with every recipe row of the ItemCreate table the character qualifies
// for.

void CSWGuiCreateItem::SetItemList()
{
    CItemRepository* pRepo = m_pStore->m_pItemRepository;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();

    C2DA* pArray;
    if (m_sOverride2DA.m_sString != NULL && m_sOverride2DA.m_sString[0] != 0)
    {
        pArray = new C2DA(CResRef(m_sOverride2DA), 0);
        pArray->Load2DArray();
        UpdateCreateFilters();
    }
    else
    {
        pArray = g_pRules->m_p2DArrays->m_pItemCreate;
        if (pArray == NULL)
            return;
    }

    for (int i = 0; i < static_cast<int>(pRepo->ItemListGetCount()); i++)
    {
        CSWSItem* pOldItem = pRepo->ItemListGetItem(i);
        if (pOldItem != NULL)
            delete pOldItem;
    }
    pRepo->ClearItems();

    CExoString sResRef;
    int nLevel = pCreature->m_pStats->GetLevel(0);
    int nTier = 1;
    if (pCreature->m_pStats->m_nAlignment > 59)
        nTier = 2;
    if (pCreature->m_pStats->m_nAlignment < 41)
        nTier = 3;

    for (int i = 0; i < pArray->m_nRows; i++)
    {
        int nGroup;
        int nSkill;
        int nLevelReq;
        int nAlign;
        int nBaseSkill;
        CSWSItem* pItem;

        if (!pArray->GetINTEntry(i, CExoString("group"), &nGroup))
            return;
        if (!pArray->GetINTEntry(i, CExoString("skill"), &nSkill))
            return;
        if (!pArray->GetINTEntry(i, CExoString("level"), &nLevelReq))
            return;
        if (!pArray->GetINTEntry(i, CExoString("align"), &nAlign))
            return;
        if (!pArray->GetINTEntry(i, CExoString("base_skill"), &nBaseSkill))
            return;

        if (nSkill > pCreature->m_pStats->GetSkillRank(static_cast<unsigned char>(nBaseSkill), NULL, 0) + 8)
            continue;
        if (nGroup != m_nCreateFilter)
            continue;
        if (nLevelReq != 0 && nLevelReq > nLevel)
            continue;
        if (nAlign != 0 && nAlign != nTier)
            continue;

        pItem = new CSWSItem(OBJECT_INVALID);
        pArray->GetCExoStringEntry(i, CExoString("label"), &sResRef);
        if (pItem->LoadFromTemplate(CResRef(sResRef.CStr())))
            m_pStore->AcquireItem(pItem, 1);
    }

    if (m_sOverride2DA.m_sString != NULL && m_sOverride2DA.m_sString[0] != 0)
    {
        pArray->Unload2DArray();
        delete pArray;
    }

    PopulateStoreItemListBox();
}

// Builds or recycles one row in either list box; the shop side also resolves the recipe and
// disables the row when the skill is too low.

int CSWGuiCreateItem::CreateItemEntry(int& nIndex, CExoArrayList<CSWGuiControl*>& aControls,
                                      unsigned long long oidItem, int bInventory, CExoString sTag)
{
    CSWGuiStoreItemEntry* pEntry;
    CExoArrayList<CSWGuiStoreItemEntry*>& aEntries = bInventory ? m_aInvEntries : m_aStoreEntries;

    if (nIndex < aEntries.m_count)
    {
        pEntry = aEntries.m_pArray[nIndex];
    }
    else
    {
        pEntry = new CSWGuiStoreItemEntry();
        aEntries.Add(pEntry);
        pEntry->m_tail.m_nClickSound = GUISOUND_NONE;

        if (bInventory)
        {
            CSWGuiButton* pProto = m_lbInvItems.m_pProtoItem->AsButton();
            pEntry->Initialize(pProto->m_text, pProto->m_border, pProto->m_borderHilight,
                               m_lbInvItems.m_innerExtent.w - 2 * m_lbInvItems.m_nPadding);
            pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateItem::OnControlInvAButton));
        }
        else
        {
            CSWGuiButton* pProto = m_lbShopItems.m_pProtoItem->AsButton();
            pEntry->Initialize(pProto->m_text, pProto->m_border, pProto->m_borderHilight,
                               m_lbShopItems.m_innerExtent.w - 2 * m_lbShopItems.m_nPadding);
            pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateItem::OnControlStoreAButton));
        }
        pEntry->AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateItem::OnControlEntered));
    }

    if (bInventory)
    {
        pEntry->SetColors(CGuiInGame::COLOR_UPGRADE_BORDER, CGuiInGame::COLOR_UPGRADE_TEXT,
                          CGuiInGame::COLOR_NEW_HIGHLIGHT, CGuiInGame::COLOR_NEW_HIGHLIGHT);
    }
    else
    {
        C2DA* pArray;
        if (m_sOverride2DA.m_sString != NULL && m_sOverride2DA.m_sString[0] != 0)
        {
            pArray = new C2DA(CResRef(m_sOverride2DA), 0);
            pArray->Load2DArray();
        }
        else
        {
            pArray = g_pRules->m_p2DArrays->m_pItemCreate;
            if (pArray == NULL)
                return 0;
        }

        CExoString sLabel;
        int nRow = -1;
        for (int i = 0; i < pArray->m_nRows; i++)
        {
            if (!pArray->GetCExoStringEntry(i, CExoString("OverrideTag"), &sLabel))
                sLabel = "";
            if (sLabel.m_sString == NULL || sLabel.m_sString[0] == 0)
            {
                if (!pArray->GetCExoStringEntry(i, CExoString("label"), &sLabel))
                    return 0;
            }

            if (sLabel.CompareNoCase(sTag))
            {
                nRow = i;
                break;
            }
        }

        if (nRow == -1)
        {
            CExoString sError("CreateItem::CreateItemEntry() -- Could not find a row for an item. Major error: ");
            sError = sError + sTag;
        }
        else
        {
            pEntry->m_nCreateRow = nRow;

            int nBaseSkill = -1;
            int nSkill = -1;
            if (!pArray->GetINTEntry(nRow, CExoString("base_skill"), &nBaseSkill))
                return 0;
            if (!pArray->GetINTEntry(nRow, CExoString("skill"), &nSkill))
                return 0;

            CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
            if (nSkill > pCreature->m_pStats->GetSkillRank(static_cast<unsigned char>(nBaseSkill), NULL, 0))
            {
                pEntry->SetColors(CGuiInGame::DISABLED_TEXT, CGuiInGame::DISABLED_TEXT,
                                  CGuiInGame::COLOR_NEW_HIGHLIGHT, CGuiInGame::COLOR_NEW_HIGHLIGHT);
                pEntry->m_bCanCreate = 0;
            }
            else
            {
                pEntry->SetColors(CGuiInGame::COLOR_UPGRADE_BORDER, CGuiInGame::COLOR_UPGRADE_TEXT,
                                  CGuiInGame::COLOR_UPGRADE_TEXT, CGuiInGame::COLOR_NEW_HIGHLIGHT);
                pEntry->m_bCanCreate = 1;
            }
        }

        if (m_sOverride2DA.m_sString != NULL && m_sOverride2DA.m_sString[0] != 0)
        {
            pArray->Unload2DArray();
            delete pArray;
        }
    }

    pEntry->SetItem(oidItem, 0, -1);
    pEntry->SetActive(0);
    aControls.Add(pEntry);
    nIndex++;

    return 0;
}

// On row hover, fills the cost, stock and related-skill labels for the current sheet side.

void CSWGuiCreateItem::OnControlEntered(CSWGuiControl* pControl)
{
    C2DA* pArray;
    if (m_sOverride2DA.m_sString != NULL && m_sOverride2DA.m_sString[0] != 0)
    {
        pArray = new C2DA(CResRef(m_sOverride2DA), 0);
        pArray->Load2DArray();
    }
    else
        pArray = g_pRules->m_p2DArrays->m_pItemCreate;

    CExoString sUnused;
    (void)sUnused;

    CItemRepository* pRepo = NULL;

    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();

    if (pCreature != NULL)
        pRepo = pCreature->GetItemRepository(1);

    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiStoreItemEntry* pEntry = static_cast<CSWGuiStoreItemEntry*>(pControl);
        CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
            g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pEntry->m_oidItem));

        if (pItem != NULL)
        {
            if (m_lbShopItems.m_tail.bVisible)
            {
                m_lblCostValue.m_text.m_params.SetText(
                    CExoString(pItem->GetCost() < 20 ? 1 : pItem->GetCost() / 20));

                int nRow = pEntry->m_nCreateRow;
                int nBaseSkill = -1;
                if (!pArray->GetINTEntry(nRow, CExoString("base_skill"), &nBaseSkill))
                    return;

                int nRank = -1;
                CSWSCreature* pChar = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
                if (pChar != NULL)
                    nRank = pChar->m_pStats->GetSkillRank(static_cast<unsigned char>(nBaseSkill), NULL, 0);

                CExoString sLabel;
                CExoString sName = g_pRules->m_paSkills[static_cast<unsigned short>(nBaseSkill)].GetNameText();
                CExoString sRank;
                sRank.Format(const_cast<char*>("%d"), nRank);
                sLabel = sName + " (" + sRank + ")";

                m_lblRelatedSkill.m_text.m_params.SetText(sLabel);
                if (pEntry->m_bCanCreate != 0)
                    m_lblRelatedSkill.m_text.m_params.SetColor(CGuiInGame::COLOR_UPGRADE_TEXT);
                else
                    m_lblRelatedSkill.m_text.m_params.SetColor(CGuiInGame::COLOR_RED);
            }
            else
            {
                int nValue = static_cast<int>(m_fBreakdownYield *
                    static_cast<float>(pItem->GetCost() < 20 ? 1 : pItem->GetCost() / 20));
                m_lblCostValue.m_text.m_params.SetText(CExoString(CSWGCREATEITEM_MAX(nValue, 1)));
            }

            if (m_lbShopItems.m_tail.bVisible && pRepo != NULL)
            {
                int nStock = pRepo->GetMatchingItemCount(pItem);
                int nEquipped = 0;
                CSWCCreature* pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
                if (pChar != NULL)
                    nEquipped = pChar->GetMatchingEquippedItemCount(pItem);
                pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(1);
                if (pChar != NULL)
                    nEquipped += pChar->GetMatchingEquippedItemCount(pItem);
                pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(2);
                if (pChar != NULL)
                    nEquipped += pChar->GetMatchingEquippedItemCount(pItem);

                if (nStock == -1)
#ifdef K2_FIX_ENGINE_BUGS
                    m_lblStockValue.m_text.m_params.SetStrRef(STRREF_INFINITE);
#else
                    m_lblStockValue.m_text.m_params.SetText(CExoString(STRREF_INFINITE));
#endif
                else
                    m_lblStockValue.m_text.m_params.SetText(CExoString(nEquipped + nStock));
            }
            else
            {
                if ((pItem->m_nFlagsLow & 4) != 0)
                    m_lblStockValue.m_text.m_params.SetStrRef(STRREF_INFINITE);
                else
                    m_lblStockValue.m_text.m_params.SetText(CExoString(pItem->m_stackCount));
            }

            CExoString sDescription = pItem->GetPropertyDescription();
            int nFoundLt = sDescription.Find('<', 0);
            int nFoundBrace = sDescription.Find('{', 0);
            if (nFoundLt != -1 || nFoundBrace != -2)
            {
                CSWTlkTable* pTlk = g_pSWTlkTable;
                pTlk->m_pSpeakerObj =
                    g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
                pTlk->m_gender = pTlk->GetObjectGender();
                pTlk->m_bSuppressEnabled |= 1;
                pTlk->ParseStr(sDescription);
            }

            if (m_lbShopItems.m_tail.bVisible)
            {
                int nBaseSkill = -1;
                int nSkill = -1;
                int nRow = pEntry->m_nCreateRow;
                if (!pArray->GetINTEntry(nRow, CExoString("base_skill"), &nBaseSkill))
                    return;
                if (!pArray->GetINTEntry(nRow, CExoString("skill"), &nSkill))
                    return;

                CExoString sText = g_pTlkTable->GetSimpleString(STRREF_SKILL_REQUIRED_TO_CREATE);
                CExoString sName;
                CExoString sRank(nSkill);
                sName = g_pRules->m_paSkills[static_cast<unsigned short>(nBaseSkill)].GetNameText() +
                        " (" + sRank + ")";
                sDescription = sText + "\n" + sName + "\n" + sDescription;
            }

            SetDescription(sDescription);
        }
    }

    if (m_sOverride2DA.m_sString != NULL && m_sOverride2DA.m_sString[0] != 0)
    {
        pArray->Unload2DArray();
        delete pArray;
    }
}

// Only the temporary store and the two row lists are released explicitly; the member teardown
// is implicit.

CSWGuiCreateItem::~CSWGuiCreateItem()
{
    delete m_pStore;
    DeleteItemEntries();
}


// The medical workbench panel. CSWGuiCreateItem is its twin: same method set and row type, but
// half-price items, four create filters, no breakdown side and a yield driven by Treat Injury.




































// A macro, not a function; both arms re-evaluate their argument.

#define CSWGCREATEMED_MAX(a, b) ((a) > (b) ? (a) : (b))

extern CTlkTable* g_pTlkTable;

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);


CSWGuiCreateMedicalItem::CSWGuiCreateMedicalItem(CSWGuiManager* pGuiManager, unsigned long long oidOwner)
    : CSWGuiPanel(pGuiManager)
{
    m_nReserved2 = 0;
    m_nReserved3 = 0;

    StartLoadFromLayout(CResRef("chemical_p"), 1, 1, 0);

    InitControl(m_lbShopItems, CExoString("LB_SHOPITEMS"), 1, 1, 0, 0, 0);
    InitControl(m_lbInvItems, CExoString("LB_INVITEMS"), 1, 1, 0, 0, 0);
    InitControl(m_lblCredits, CExoString("LBL_CREDITS"), 1, 1, 0, 0, 0);
    InitControl(m_lblCreditsValue, CExoString("LBL_CREDITS_VALUE"), 1, 1, 0, 0, 0);
    InitControl(m_lblTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_lblTitle2, CExoString("LBL_TITLE2"), 1, 1, 0, 0, 0);
    InitControl(m_lblRelatedSkill, CExoString("LBL_RELATEDSKILL"), 1, 1, 0, 0, 0);
    InitControl(m_lbDescriptionInv, CExoString("LB_DESCRIPTIONINV"), 1, 1, 0, 0, 0);
    InitControl(m_btnAccept, CExoString("BTN_Accept"), 1, 1, 0, 0, 0);
    InitControl(m_btnCancel, CExoString("BTN_Cancel"), 1, 1, 0, 0, 0);
    InitControl(m_btnExamine, CExoString("BTN_Examine"), 1, 1, 0, 0, 0);
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateMedicalItem::CloseChemical));
    m_btnExamine.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateMedicalItem::OnControlXButton));
    InitControl(m_aBarLabels[0], CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[1], CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_aBarLabels[2], CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
    InitControl(m_lbDescriptionInv, CExoString("LB_DESCRIPTIONINV"), 1, 1, 0, 0, 0);
// The array order and the control order differ in the last two buttons (grenades, then mines).

    InitControl(m_aCreateButtons[0], CExoString("BTN_CREATE_HEALTH"), 1, 1, 0, 0, 0);
    InitControl(m_aCreateButtons[1], CExoString("BTN_CREATE_STIMS"), 1, 1, 0, 0, 0);
    InitControl(m_aCreateButtons[3], CExoString("BTN_CREATE_GRENADES"), 1, 1, 0, 0, 0);
    InitControl(m_aCreateButtons[2], CExoString("BTN_CREATE_MINES"), 1, 1, 0, 0, 0);
    InitControl(m_lbDescription, CExoString("LB_DESCRIPTION"), 1, 1, 0, 0, 0);
    m_lbDescription.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_lblCost, CExoString("LBL_COST"), 1, 1, 0, 0, 0);
    InitControl(m_lblCostValue, CExoString("LBL_COST_VALUE"), 1, 1, 0, 0, 0);
    InitControl(m_lblStock, CExoString("LBL_STOCK"), 1, 1, 0, 0, 0);
    InitControl(m_lblStockValue, CExoString("LBL_STOCK_VALUE"), 1, 1, 0, 0, 0);
    InitControl(m_lblRandAnalog, CExoString("LBL_RANALOG"), 1, 1, 0, 0, 0);

    StopLoadFromLayout();

    CSWGuiLabel* pProto = static_cast<CSWGuiLabel*>(m_lbDescription.m_pProtoItem);
    m_lblDescriptionText.Initialize(pProto->m_extent, pProto, 1.0f);

    m_lbShopItems.bSelectOnLeftClick = 1;
    m_lbShopItems.bShowScrollBar = 1;
    m_lbInvItems.bSelectOnLeftClick = 1;
    m_lbInvItems.bShowScrollBar = 1;
    SetActiveControl(&m_lbShopItems, 0);
    m_lbInvItems.m_tail.bVisible = 0;
    m_nReserved1 = 0;
    m_nReserved4 = 0;
    m_fReserved1 = 0.0f;
    m_nComponent = 0;
    m_pStore = new CSWSStore(OBJECT_INVALID);
    m_nCreateFilter = 0;
    SetItemList();
    m_oidOwner = oidOwner;

    CServerExoApp* pServerExoApp = g_pAppManager->m_pServerExoApp;
    CSWSCreature* pCreature;
    CSWSPlayer* pPlayer;
    if (oidOwner == OBJECT_INVALID)
    {
        pPlayer = static_cast<CSWSClient*>(pServerExoApp->GetPlayerList()->m_pInternal->m_pHead->m_pData)->AsSWSPlayer();
        pCreature = pServerExoApp->GetCreatureByGameObjectID(pPlayer->m_gameObjectId);
    }
    else
    {
        pCreature = pServerExoApp->GetCreatureByGameObjectID(oidOwner);
    }

    m_oidPlayer = pCreature->m_objectId;
    float fYield = static_cast<float>(pCreature->m_pStats->GetSkillRank(SKILL_TREAT_INJURY, NULL, 0)) / 20.0f;
    m_fBreakdownYield = fYield > 1.0f ? 1.0f : fYield;
    m_bViewSwitched = 0;

    int i;
    for (i = 0; i < 4; i++)
    {
        m_aCreateButtons[i].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateMedicalItem::OnCreateFilterPicked));
        m_aCreateButtons[i].AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateMedicalItem::OnCreateFilterPicked));
        m_aCreateButtons[i].m_tail.bFocusOnHover = 0;
        m_aCreateButtons[i].m_tail.m_nUserData = i;
    }
    m_nCreateFilter = 0;
    UpdateCreateFilters();

    m_btnAccept.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateMedicalItem::OnAcceptPicked));
    m_btnAccept.m_tail.bFocusOnHover = 0;
    m_btnExamine.m_tail.bFocusOnHover = 0;
    m_btnCancel.m_tail.bFocusOnHover = 0;
    m_nFullScreenBGIndex = 1;
    AspyrAddButtonIcon(&m_btnCancel, 'b');
}

// Half the item's cost, floored at one.

int CSWGuiCreateMedicalItem::GetItemSellValue(CSWSItem* pItem)
{
    int nValue = 0;
    if (pItem != NULL)
        nValue = pItem->GetCost() < 2 ? 1 : pItem->GetCost() / 2;
    return nValue;
}

int CSWGuiCreateMedicalItem::GetItemBuyValue(CSWSItem* pItem)
{
    int nValue = 0;
    if (pItem != NULL)
        nValue = pItem->GetCost() < 2 ? 1 : pItem->GetCost() / 2;
    return nValue;
}

// The count is re-read after each delete, since the deleting destructor may reallocate.

void CSWGuiCreateMedicalItem::DeleteItemEntries()
{
    m_lbInvItems.ClearItems();
    m_lbShopItems.ClearItems();

    for (int64_t i = 0; i < m_aStoreEntries.m_count; i++)
    {
        if (m_aStoreEntries.m_pArray[i])
            delete m_aStoreEntries.m_pArray[i];
        m_aStoreEntries.m_pArray[i] = 0;
    }
    m_aStoreEntries.m_count = 0;

    for (int64_t i = 0; i < m_aInvEntries.m_count; i++)
    {
        if (m_aInvEntries.m_pArray[i])
            delete m_aInvEntries.m_pArray[i];
        m_aInvEntries.m_pArray[i] = 0;
    }
    m_aInvEntries.m_count = 0;
}

// Closes the sheet through the in-game GUI.

void CSWGuiCreateMedicalItem::CloseChemical(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideChemicalUpgradeScreen();
}

void CSWGuiCreateMedicalItem::OnControlXButton(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    m_bViewSwitched = (m_bViewSwitched == 0);
    m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
    if (m_lbShopItems.m_tail.bVisible)
        ShowSellGUI();
    else
        ShowBuyGUI();
}

// Create-filter click; only the shop side responds.

void CSWGuiCreateMedicalItem::OnCreateFilterPicked(CSWGuiControl* pControl)
{
// A bitwise and. Both operands are side-effect free, so the short-circuit is not observable.

    if ((pControl != NULL) & (m_bCreateMode != 0))
    {
        m_nCreateFilter = static_cast<int>(pControl->m_tail.m_nUserData);
        UpdateCreateFilters();
        SetItemList();
    }
}

// One colour pair per filter button; four buttons here, not five.

void CSWGuiCreateMedicalItem::UpdateCreateFilters()
{
    for (int i = 0; i < 4; i++)
        m_aCreateButtons[i].m_text.m_params.SetDefaultColor(
            m_aCreateButtons[i].m_tail.m_nUserData == static_cast<uint64_t>(m_nCreateFilter)
                ? CGuiInGame::COLOR_NEW_HIGHLIGHT : CGuiInGame::COLOR_UPGRADE_TEXT);
}

// Routes to whichever side's row handler is active.

void CSWGuiCreateMedicalItem::OnAcceptPicked(CSWGuiControl* pControl)
{
    if (m_bCreateMode != 0)
    {
        pControl = m_lbShopItems.GetSelectedControl();
        if (pControl == NULL)
            return;
        OnControlStoreAButton(pControl);
    }
    else
    {
        pControl = m_lbInvItems.GetSelectedControl();
        if (pControl == NULL)
            return;
        OnControlInvAButton(pControl);
    }
}

// Clears the party table's stale chemical counter, then re-lists the sheet.

void CSWGuiCreateMedicalItem::OnPanelAdded()
{
    if (g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nItemChemical < 0)
        g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nItemChemical = 0;

    InitializeStoreType();
    PopulateStore();
    CSWGuiPanel::OnPanelAdded();
    ShowBuyGUI();
}

void CSWGuiCreateMedicalItem::InitializeStoreType()
{
    CSWSStore* pStore = g_pAppManager->m_pServerExoApp->GetStoreByGameObjectID(m_oidStore);
    if (!pStore)
        return;

    switch (pStore->m_buySellFlag)
    {
    case CSWSStore::STORE_BUY_AND_SELL:
        m_lblX.m_tail.bVisible = 1;
        m_lblExamine.m_tail.bVisible = 1;
        ShowBuyGUI();
        break;
    case CSWSStore::STORE_BUY_ONLY:
        m_lblX.m_tail.bVisible = 0;
        m_lblExamine.m_tail.bVisible = 0;
        ShowBuyGUI();
        break;
    case CSWSStore::STORE_SELL_ONLY:
        m_lblX.m_tail.bVisible = 0;
        m_lblExamine.m_tail.bVisible = 0;
        ShowSellGUI();
        break;
    }
}

// The credits readout on this sheet shows chemicals, not gold.

void CSWGuiCreateMedicalItem::PopulateStore()
{
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidPlayer);
    if (!pCreature)
        return;

    int nChemical = pCreature->GetChemical();
    m_nComponent = nChemical;
    m_lblCreditsValue.m_text.m_params.SetText(CExoString(nChemical));
}

// Disposes of the row entries and every item the player created into the temporary store.

void CSWGuiCreateMedicalItem::OnPanelRemoved()
{
    DeleteItemEntries();

    CItemRepository* pRepo = m_pStore->m_pItemRepository;
    for (int i = 0; i < static_cast<int>(pRepo->ItemListGetCount()); i++)
    {
        CSWSItem* pItem = pRepo->ItemListGetItem(i);
        if (pItem != NULL)
            delete pItem;
    }

    CSWGuiPanel::OnPanelRemoved();
}

// Confirmation-box callbacks for the buy and sell paths.

void CSWGuiCreateMedicalItem::SellItemCallback(CSWGuiControl* pControl)
{
    if (!g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted)
        return;

    CSWGuiStoreItemEntry* pEntry = static_cast<CSWGuiStoreItemEntry*>(pControl);
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pEntry->m_oidItem));
    if (!pItem)
        return;

    SellItem(pItem);
}

void CSWGuiCreateMedicalItem::BuyItemCallback(CSWGuiControl* pControl)
{
    if (!g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted)
        return;

    CSWGuiStoreItemEntry* pEntry = static_cast<CSWGuiStoreItemEntry*>(pControl);
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pEntry->m_oidItem));
    if (!pItem)
        return;

    BuyItem(pItem);
}

// Handles keys. The close keys run CloseChemical, and the filter cycle is four-wide here.

void CSWGuiCreateMedicalItem::HandleInputEvent(int nEventType, int nEventArg)
{
    CSWGuiPanel::HandleInputEvent(nEventType, nEventArg);
    if (nEventArg == 0)
        return;

    switch (nEventType)
    {
    case INPUTEVENT_GUI_B:
    case INPUTEVENT_GUI_BACK:
        CloseChemical(NULL);
        break;

    case INPUTEVENT_GUI_X:
        m_bViewSwitched = (m_bViewSwitched == 0);
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        if (m_lbShopItems.m_tail.bVisible)
            ShowSellGUI();
        else
            ShowBuyGUI();
        break;

    case INPUTEVENT_GUI_DPAD_LEFT:
    case INPUTEVENT_GUI_DPAD_RIGHT:
        {
            int nDelta = (nEventType == INPUTEVENT_GUI_DPAD_LEFT) ? -1 : 1;
            m_nCreateFilter = (m_nCreateFilter + nDelta + 4) % 4;
            UpdateCreateFilters();
            SetItemList();
        }
        break;

    case INPUTEVENT_GUI_SCROLL_UP:
        if (m_bCreateMode == 0)
            m_lbDescriptionInv.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
        else
            m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
        break;

    case INPUTEVENT_GUI_SCROLL_DOWN:
        if (m_bCreateMode == 0)
            m_lbDescriptionInv.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
        else
            m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
        break;
    }
}

// Points the single description row at a new string and re-fits it; the side test reads the
// shop list's visible flag.

void CSWGuiCreateMedicalItem::SetDescription(const CExoString& sDescription)
{
    CSWGuiListBox* pListBox =
        m_lbShopItems.m_tail.bVisible ? &m_lbDescription : &m_lbDescriptionInv;

    m_lblDescriptionText.m_text.m_params.SetText(sDescription);
    pListBox->ClearItems();

    CSWGuiText* pText = &m_lblDescriptionText.m_text;

    CSWGuiControl* pRow;

    CSWGuiExtent extent = m_lblDescriptionText.m_extent;
    if (sDescription.GetLength() >= 1)
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
    m_lblDescriptionText.SetExtent(extent);

    pRow = &m_lblDescriptionText;
    pListBox->AddControls(&pRow, 1, 0, 0, 0);
    pListBox->SetSelectedControl(&m_lblDescriptionText, 0);

    m_lblRandAnalog.m_tail.bVisible =
        m_lbDescription.IsOversized();
}

// Builds the buy side of the sheet; ShowSellGUI is its mirror image.

void CSWGuiCreateMedicalItem::ShowBuyGUI()
{
    m_bCreateMode = 1;

    m_lbInvItems.m_tail.bVisible = 0;
    m_lbShopItems.m_tail.bVisible = 1;
    m_lblRelatedSkill.m_tail.bVisible = 1;
    m_lblCost.m_text.m_params.SetStrRef(STRREF_CHEMICAL_COST);
    m_lblTitle.m_text.m_params.SetStrRef(STRREF_CREATE_NEW_ITEMS);

    m_lbDescription.m_tail.bVisible = 1;
    m_lbDescriptionInv.m_tail.bVisible = 0;

    m_btnAccept.m_text.m_params.SetStrRef(STRREF_CREATE_ITEM);
    m_btnExamine.m_text.m_params.SetStrRef(STRREF_VIEW_BREAKDOWN_ITEMS);

    for (int i = 0; i < 4; i++)
        m_aCreateButtons[i].m_tail.bVisible = 1;

    m_aBarLabels[1].m_tail.bVisible = 1;
    m_aBarLabels[2].m_tail.bVisible = 1;
    m_lblAccept.m_text.m_params.SetStrRef(STRREF_CREATE_ITEM);
    m_btnExamine.m_text.m_params.SetStrRef(STRREF_VIEW_BREAKDOWN_ITEMS);
    m_lblCost.m_text.m_params.SetStrRef(STRREF_CHEMICAL_COST);

    SetActiveControl(&m_lbShopItems, 0);
    PopulateStoreItemListBox();
    AspyrAddButtonIcon(&m_btnExamine, 'x');
    AspyrAddButtonIcon(&m_btnAccept, 'a');
}

void CSWGuiCreateMedicalItem::ShowSellGUI()
{
    m_lbShopItems.m_tail.bVisible = 0;
    m_lbInvItems.m_tail.bVisible = 1;
    m_bCreateMode = 0;

    m_lblCost.m_text.m_params.SetStrRef(STRREF_CHEMICAL_VALUE);
    m_lblRelatedSkill.m_tail.bVisible = 0;
    m_lblTitle.m_text.m_params.SetStrRef(STRREF_BREAKDOWN_INVENTORY_ITEMS);

    m_lbDescription.m_tail.bVisible = 0;
    m_lbDescriptionInv.m_tail.bVisible = 1;

    m_btnAccept.m_text.m_params.SetStrRef(STRREF_BREAKDOWN_ITEM);
    m_btnExamine.m_text.m_params.SetStrRef(STRREF_VIEW_CREATEABLE_ITEMS);

    for (int i = 0; i < 4; i++)
        m_aCreateButtons[i].m_tail.bVisible = 0;

    m_aBarLabels[1].m_tail.bVisible = 0;
    m_aBarLabels[2].m_tail.bVisible = 0;
    m_lblAccept.m_text.m_params.SetStrRef(STRREF_BREAKDOWN_ITEM_LABEL);
    m_btnExamine.m_text.m_params.SetStrRef(STRREF_VIEW_CREATEABLE_ITEMS);
    m_lblCost.m_text.m_params.SetStrRef(STRREF_CHEMICAL_VALUE);

    SetActiveControl(&m_lbInvItems, 0);
    PopulateInventoryItemListBox();
    AspyrAddButtonIcon(&m_btnExamine, 'x');
    AspyrAddButtonIcon(&m_btnAccept, 'a');
}

// Destroys one unit of the item and credits the player with its converted chemical value.

void CSWGuiCreateMedicalItem::SellItem(CSWSItem* pItem)
{
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();

    if (pItem == NULL || pCreature == NULL)
        return;

    CItemRepository* pRepo = pCreature->GetItemRepository(1);
    if (pRepo == NULL)
        return;

    m_pGuiManager->PlayGuiSound(static_cast<char>(GUISOUND_INVENTORY_DROP));

    int nGain = CSWGCREATEMED_MAX(1, static_cast<int>((pItem->GetCost() < 2
        ? 1.0f : static_cast<float>(pItem->GetCost() / 2)) * m_fBreakdownYield));
    m_nComponent = m_nComponent + nGain;
    pCreature->SetChemical(m_nComponent);

    if (pItem->m_stackCount >= 2)
        pItem->SplitItem(1);
    else
        pRepo->RemoveItem(pItem);

    short nSelected = m_lbInvItems.m_nSelectedIndex;

    PopulateInventoryItemListBox();

    int nControls = m_lbInvItems.m_aControls.m_count;
    m_lbInvItems.SetSelectedControl(nControls > nSelected ? nSelected : nControls - 1, 0);

    m_lblCreditsValue.m_text.m_params.SetText(CExoString(m_nComponent));
}

// Copies the item into the workbench store and charges the player for it.

void CSWGuiCreateMedicalItem::BuyItem(CSWSItem* pItem)
{
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();

    if (pCreature == NULL)
        return;

    CItemRepository* pRepo = pCreature->GetItemRepository(1);
    if (pItem == NULL || pRepo == NULL)
        return;

    m_pGuiManager->PlayGuiSound(GUISOUND_INVENTORY_ADD);

    int nCost = pItem->GetCost() < 2 ? 1 : pItem->GetCost() / 2;
    m_nComponent = m_nComponent - nCost;
    pCreature->SetChemical(m_nComponent);

    CSWSItem* pNewItem = new CSWSItem(OBJECT_INVALID);
    pNewItem->CopyItem(pItem);
    pCreature->AcquireItem(&pNewItem, m_pStore->m_objectId, OBJECT_INVALID, 1);

    int nControls = m_lbShopItems.m_aControls.m_count;
    short nSelected = m_lbShopItems.m_nSelectedIndex;
    m_lbShopItems.SetSelectedControl(nControls > nSelected ? nSelected : nControls - 1, 0);

    m_lblCreditsValue.m_text.m_params.SetText(CExoString(m_nComponent));
}

// Only the temporary store and the two row lists are released explicitly; the member teardown
// is implicit.

CSWGuiCreateMedicalItem::~CSWGuiCreateMedicalItem()
{
    delete m_pStore;
    DeleteItemEntries();
}


// Fills the shop list from the workbench's temporary store.

void CSWGuiCreateMedicalItem::PopulateStoreItemListBox()
{
    CItemRepository* pRepo = m_pStore->m_pItemRepository;

    CExoArrayList<CSWGuiControl*> aControls;
    int nIndex = 0;

    for (int i = pRepo->ItemListGetCount() - 1; i != -1; i--)
    {
        CSWSItem* pItem = pRepo->ItemListGetItem(i);
        if (pItem == NULL || pItem->m_bPlot)
            continue;

        CExoString sTag(pItem->m_sTag.CStr());
        CreateItemEntry(nIndex, aControls, pItem->m_objectId, 0, sTag);
    }

    m_lbShopItems.AddControls(aControls, 0, 1, 0);

    if (m_lbShopItems.m_tail.bVisible)
    {
        if (aControls.m_count >= 1)
        {
            m_lbShopItems.SetSelectedControl(0, 0);
        }
        else
        {
            m_lbShopItems.SetSelectedControl(-1, 0);
            m_lblCostValue.m_text.m_params.SetText(CExoString(""));
            m_lblStockValue.m_text.m_params.SetText(CExoString(""));
            m_lblDescriptionText.m_text.m_params.SetText(CExoString(""));
        }
    }

    m_nReserved4 = 0;
}

// Refills the workbench store from the ChemicalCreate rows whose group matches the live filter.

void CSWGuiCreateMedicalItem::SetItemList()
{
    CItemRepository* pRepo = m_pStore->m_pItemRepository;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    C2DA* pArray = g_pRules->m_p2DArrays->m_pChemicalCreate;

    for (int i = 0; i < static_cast<int>(pRepo->ItemListGetCount()); i++)
    {
        CSWSItem* pItem = pRepo->ItemListGetItem(i);
        if (pItem != NULL)
            delete pItem;
    }
    pRepo->ClearItems();

    if (pArray == NULL)
        return;

    CExoString sTemplate;

    for (int i = 0; i < pArray->m_nRows; i++)
    {
        CSWSItem* pItem;
        int nGroup;
        int nSkillLevel;
        int nBaseSkill;
// A failed lookup abandons the whole list rather than skipping the row.

        if (!pArray->GetINTEntry(i, CExoString("group"), &nGroup))
            return;
        if (!pArray->GetINTEntry(i, CExoString("skillLevel"), &nSkillLevel))
            return;
        if (!pArray->GetINTEntry(i, CExoString("base_skill"), &nBaseSkill))
            return;

        if (nSkillLevel > pCreature->m_pStats->GetSkillRank(static_cast<unsigned char>(nBaseSkill), NULL, 0) + 8)
            continue;
        if (nGroup != m_nCreateFilter)
            continue;

        pItem = new CSWSItem(OBJECT_INVALID);
        pArray->GetCExoStringEntry(i, CExoString("label"), &sTemplate);
        pItem->LoadFromTemplate(CResRef(sTemplate.CStr()));
        m_pStore->AcquireItem(pItem, 1);
    }

    PopulateStoreItemListBox();
}

// Fills the inventory list with the items whose tag appears in a ChemicalCreate row whose group is
// not 5.

void CSWGuiCreateMedicalItem::PopulateInventoryItemListBox()
{
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidPlayer);

    C2DA* pArray = g_pRules->m_p2DArrays->m_pChemicalCreate;
    if (pArray == NULL)
        return;

    CItemRepository* pRepo;
    if (pCreature != NULL)
        pRepo = pCreature->GetItemRepository(1);
#ifdef K2_FIX_ENGINE_BUGS
    else
        return;
#endif

    CExoArrayList<CSWGuiControl*> aControls;
    int nIndex = 0;
    CExoString sValue;

    unsigned nValid = 0;
    for (unsigned i = 0; i < pRepo->ItemListGetCount(); i++)
    {
        CSWSItem* pItem = pRepo->ItemListGetItem(i);
        if (pItem != NULL && pItem->m_bPlot == 0)
            nValid++;
    }

    if (nValid != 0)
    {
        unsigned nDone = 0;
        int nGroup = 0;
        while (nDone < nValid)
        {
            int nNext = nGroup;
            for (unsigned i = 0; i < pRepo->ItemListGetCount(); i++)
            {
                int nMatch = 0;
                int nSize;
                CSWSItem* pItem = pRepo->ItemListGetItem(i);

                for (int j = 0; j < pArray->m_nRows; j++)
                {
                    pArray->GetCExoStringEntry(j, CExoString("label"), &sValue);

                    CExoString sTag;
                    sTag = pItem->m_sTag.CStr();
                    if (sTag == sValue)
                    {
                        int nRowGroup = 0;
                        pArray->GetINTEntry(j, CExoString("group"), &nRowGroup);
                        nMatch = (nRowGroup != 5);
                        break;
                    }
                }

                if (pItem == NULL)
                    continue;
                if (pItem->m_bPlot)
                    continue;

                nSize = pItem->GetBaseItem()->m_nStorePanelSort;
                if (nSize == nGroup)
                {
                    if (nMatch && pItem->GetCost() != 0)
                        CreateItemEntry(nIndex, aControls, pItem->m_objectId, 1, CExoString(""));
                    nDone++;
                }
                else if (nSize > nGroup)
                {
                    if (nNext == nGroup || nSize < nNext)
                        nNext = nSize;
                }
            }
            nGroup = nNext;
        }
    }

    m_lbInvItems.AddControls(aControls, 0, 1, 0);

    if (m_lbInvItems.m_tail.bVisible)
    {
        if (aControls.m_count >= 1)
        {
            m_lbInvItems.SetSelectedControl(0, 0);
        }
        else
        {
            m_lbInvItems.SetSelectedControl(-1, 0);
            m_lblCostValue.m_text.m_params.SetText(CExoString(""));
            m_lblStockValue.m_text.m_params.SetText(CExoString(""));
            m_lblDescriptionText.m_text.m_params.SetText(CExoString(""));
        }
    }

    m_nReserved4 = 0;
}

// On shop row activation, refuses the sale with a message box when the player cannot pay or is too
// low level.

void CSWGuiCreateMedicalItem::OnControlStoreAButton(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    if (pControl->m_tail.nCurrentEventType != 0)
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);

    CSWGuiStoreItemEntry* pEntry = static_cast<CSWGuiStoreItemEntry*>(pControl);

    if (pEntry->m_bCanCreate == 0)
    {
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetCallback(0, 0, 0);
        pBox->SetMessageColors(CGuiInGame::COLOR_UPGRADE_BORDER, CGuiInGame::COLOR_NEW_HIGHLIGHT);
        pBox->SetMessage(STRREF_SKILL_TOO_LOW_TO_CREATE);
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
        return;
    }

    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pEntry->m_oidItem));
    if (!pItem)
        return;

    if (static_cast<unsigned int>(m_nComponent) < (pItem->GetCost() < 2 ? 1 : pItem->GetCost() / 2))
    {
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetCallback(0, 0, 0);
        pBox->SetMessage(STRREF_NOT_ENOUGH_CHEMICALS);
        pBox->SetMessageColors(CGuiInGame::COLOR_UPGRADE_BORDER, CGuiInGame::COLOR_NEW_HIGHLIGHT);
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
        return;
    }

    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    int nLimit = 50;
    if (pCreature != NULL)
        nLimit = pCreature->m_pStats->GetLevel(0) * 50;

    if (static_cast<unsigned int>(pItem->GetCost() < 2 ? 1 : pItem->GetCost() / 2) > static_cast<unsigned int>(nLimit))
    {
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        CExoString sMessage;
        g_pTlkTable->SetCustomToken(0, CExoString(nLimit));

        sMessage = g_pSWTlkTable->Fetch(STRREF_CONFIRM_CREATE_CHEMICALS, 0, 0, 0);
        pBox->SetAllowCancel(1);
        pBox->SetMessage(sMessage);
        pBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateMedicalItem::BuyItemCallback), pControl);
        pBox->SetMessageColors(CGuiInGame::COLOR_UPGRADE_BORDER, CGuiInGame::COLOR_NEW_HIGHLIGHT);
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
        return;
    }

    BuyItem(pItem);
}

// On inventory row activation, breaks the item down into chemicals.

void CSWGuiCreateMedicalItem::OnControlInvAButton(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_START)
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);

    CSWGuiStoreItemEntry* pEntry = static_cast<CSWGuiStoreItemEntry*>(pControl);
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pEntry->m_oidItem));
    if (!pItem)
        return;

    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();

    unsigned int nLimit;
    if (pCreature == NULL || static_cast<unsigned int>(pCreature->m_pStats->GetLevel(0)) * 50 > 250)
        nLimit = 250;
    else
        nLimit = static_cast<unsigned int>(pCreature->m_pStats->GetLevel(0)) * 50;

    float fValue = (pItem->GetCost() < 2 ? 1.0f : static_cast<float>(pItem->GetCost() / 2)) * m_fBreakdownYield;
    if (fValue > static_cast<float>(nLimit))
    {
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        CExoString sMessage;
        g_pTlkTable->SetCustomToken(0, CExoString(nLimit));

        sMessage = g_pSWTlkTable->Fetch(STRREF_CONFIRM_BREAKDOWN_CHEMICALS, 0, 0, 0);
        pBox->SetAllowCancel(1);
        pBox->SetMessage(sMessage);
        pBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateMedicalItem::SellItemCallback), pControl);
        pBox->SetMessageColors(CGuiInGame::COLOR_UPGRADE_BORDER, CGuiInGame::COLOR_NEW_HIGHLIGHT);
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
        return;
    }

    SellItem(pItem);
}

// Builds or recycles one row in either list box, matching the recipe by label.

int CSWGuiCreateMedicalItem::CreateItemEntry(int& nIndex, CExoArrayList<CSWGuiControl*>& aControls,
                                             unsigned long long oidItem, int bInventory,
                                             CExoString sTag)
{
    CSWGuiStoreItemEntry* pEntry;
    CExoArrayList<CSWGuiStoreItemEntry*>& aEntries = bInventory ? m_aInvEntries : m_aStoreEntries;

    if (nIndex < aEntries.m_count)
    {
        pEntry = aEntries.m_pArray[nIndex];
    }
    else
    {
        pEntry = new CSWGuiStoreItemEntry();
        aEntries.Add(pEntry);
        pEntry->m_tail.m_nClickSound = GUISOUND_NONE;

        void (CSWGuiObject::*pfnHandler)(CSWGuiControl*);

        if (bInventory)
        {
            CSWGuiButton* pProto = m_lbInvItems.m_pProtoItem->AsButton();
            pEntry->Initialize(pProto->m_text, pProto->m_border, pProto->m_borderHilight,
                               m_lbInvItems.m_innerExtent.w - 2 * m_lbInvItems.m_nPadding);
            pfnHandler = static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateMedicalItem::OnControlInvAButton);
        }
        else
        {
            CSWGuiButton* pProto = m_lbShopItems.m_pProtoItem->AsButton();
            pEntry->Initialize(pProto->m_text, pProto->m_border, pProto->m_borderHilight,
                               m_lbShopItems.m_innerExtent.w - 2 * m_lbShopItems.m_nPadding);
            pfnHandler = static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateMedicalItem::OnControlStoreAButton);
        }

        pEntry->AddEvent(INPUTEVENT_GUI_A, this, pfnHandler);
        pEntry->AddEvent(INPUTEVENT_GUI_START, this, pfnHandler);
        pEntry->AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiCreateMedicalItem::OnControlEntered));
    }

    if (bInventory)
    {
        pEntry->SetColors(CGuiInGame::COLOR_UPGRADE_BORDER, CGuiInGame::COLOR_UPGRADE_TEXT,
                          CGuiInGame::COLOR_NEW_HIGHLIGHT, CGuiInGame::COLOR_NEW_HIGHLIGHT);
    }
    else
    {
        C2DA* pArray = g_pRules->m_p2DArrays->m_pChemicalCreate;

        CExoString sLabel;
        for (int i = 0; i < pArray->m_nRows; i++)
        {
            if (!pArray->GetCExoStringEntry(i, CExoString("label"), &sLabel))
                return 0;

            if (sLabel.CompareNoCase(sTag))
            {
                pEntry->m_nCreateRow = i;

                int nSkill = -1;
                int nBaseSkill = -1;
                if (!pArray->GetINTEntry(i, CExoString("base_skill"), &nBaseSkill))
                    return 0;
                if (!pArray->GetINTEntry(i, CExoString("skillLevel"), &nSkill))
                    return 0;

                CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
                if (nSkill > pCreature->m_pStats->GetSkillRank(static_cast<unsigned char>(nBaseSkill), NULL, 0))
                {
                    pEntry->SetColors(CGuiInGame::DISABLED_TEXT, CGuiInGame::DISABLED_TEXT,
                                      CGuiInGame::COLOR_NEW_HIGHLIGHT, CGuiInGame::COLOR_NEW_HIGHLIGHT);
                    pEntry->m_bCanCreate = 0;
                }
                else
                {
                    pEntry->SetColors(CGuiInGame::COLOR_UPGRADE_BORDER, CGuiInGame::COLOR_UPGRADE_TEXT,
                                      CGuiInGame::COLOR_UPGRADE_TEXT, CGuiInGame::COLOR_NEW_HIGHLIGHT);
                    pEntry->m_bCanCreate = 1;
                }
                break;
            }
        }
    }

    pEntry->SetItem(oidItem, 0, -1);
    pEntry->SetActive(0);
    aControls.Add(pEntry);
    nIndex++;

    return 0;
}

// On row hover, fills the cost, stock and related-skill labels for the current sheet side.

void CSWGuiCreateMedicalItem::OnControlEntered(CSWGuiControl* pControl)
{
    C2DA* pArray = g_pRules->m_p2DArrays->m_pChemicalCreate;

    CItemRepository* pRepo = NULL;

    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();

    if (pCreature != NULL)
        pRepo = pCreature->GetItemRepository(1);

    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        CSWGuiStoreItemEntry* pEntry = static_cast<CSWGuiStoreItemEntry*>(pControl);
        CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
            g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pEntry->m_oidItem));

        if (pItem != NULL)
        {
            if (m_lbShopItems.m_tail.bVisible)
            {
                m_lblCostValue.m_text.m_params.SetText(
                    CExoString(pItem->GetCost() < 2 ? 1 : pItem->GetCost() / 2));

                int nRow = pEntry->m_nCreateRow;
                int nBaseSkill = -1;
                if (!pArray->GetINTEntry(nRow, CExoString("base_skill"), &nBaseSkill))
                    return;

                int nRank = -1;

                CSWSCreature* pChar = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
                if (pChar != NULL)
                    nRank = pChar->m_pStats->GetSkillRank(static_cast<unsigned char>(nBaseSkill), NULL, 0);

                CExoString sLabel;
                CExoString sName = g_pRules->m_paSkills[static_cast<unsigned short>(nBaseSkill)].GetNameText();
                CExoString sRank(nRank);
                sLabel = sName + " (" + sRank + ")";

                m_lblRelatedSkill.m_text.m_params.SetText(sLabel);
                if (pEntry->m_bCanCreate != 0)
                    m_lblRelatedSkill.m_text.m_params.SetColor(CGuiInGame::COLOR_UPGRADE_TEXT);
                else
                    m_lblRelatedSkill.m_text.m_params.SetColor(CGuiInGame::COLOR_RED);
            }
            else
            {
                int nValue = static_cast<int>(static_cast<float>(pItem->GetCost() < 2 ? 1 : pItem->GetCost() / 2) *
                    m_fBreakdownYield);
                m_lblCostValue.m_text.m_params.SetText(CExoString(CSWGCREATEMED_MAX(nValue, 1)));
            }

            if (m_lbShopItems.m_tail.bVisible && pRepo != NULL)
            {
                int nStock = pRepo->GetMatchingItemCount(pItem);
                int nEquipped = 0;
                CSWCCreature* pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
                if (pChar != NULL)
                    nEquipped = pChar->GetMatchingEquippedItemCount(pItem);
                pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(1);
                if (pChar != NULL)
                    nEquipped += pChar->GetMatchingEquippedItemCount(pItem);
                pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(2);
                if (pChar != NULL)
                    nEquipped += pChar->GetMatchingEquippedItemCount(pItem);

                if (nStock == -1)
#ifdef K2_FIX_ENGINE_BUGS
                    m_lblStockValue.m_text.m_params.SetStrRef(STRREF_INFINITE);
#else
                    m_lblStockValue.m_text.m_params.SetText(CExoString(STRREF_INFINITE));
#endif
                else
                    m_lblStockValue.m_text.m_params.SetText(CExoString(nEquipped + nStock));
            }
            else
            {
                if ((pItem->m_nFlagsLow & 4) == 0)
                    m_lblStockValue.m_text.m_params.SetText(CExoString(pItem->m_stackCount));
                else
                    m_lblStockValue.m_text.m_params.SetStrRef(STRREF_INFINITE);
            }

            CExoString sDescription = pItem->GetPropertyDescription();

            if (m_lbShopItems.m_tail.bVisible)
            {
                int nSkill = -1;
                int nBaseSkill = -1;
                int nRow = pEntry->m_nCreateRow;
                if (!pArray->GetINTEntry(nRow, CExoString("base_skill"), &nBaseSkill))
                    return;
                if (!pArray->GetINTEntry(nRow, CExoString("skillLevel"), &nSkill))
                    return;

                CExoString sText = g_pTlkTable->GetSimpleString(STRREF_SKILL_REQUIRED_TO_CREATE);
                CExoString sName;
                CExoString sRank(nSkill);
                sName = g_pRules->m_paSkills[static_cast<unsigned short>(nBaseSkill)].GetNameText() +
                        " (" + sRank + ")";
                sDescription = sText + "\n" + sName + "\n" + sDescription;
            }

            SetDescription(sDescription);
        }
    }
}

// The swoop-bike upgrade panel. Its state block mirrors CSWGuiUpgrade's, but the mode byte is
// compared against 0/1/4 rather than a menu type.
































extern int g_bDo3DGUIs;

extern CTlkTable* g_pTlkTable;



// The slot layout, an icon id and str-ref per slot. The last three rows are unreachable sentinels.

const CSWGuiSwoopUpgrade::SSlotInfo CSWGuiSwoopUpgrade::SWOOP_SLOT_INFO[6] = {
    { 0,          "i_powerc", STRREF_UPGRADE_SLOT_ENGINE },
    { 1,          "i_colorc", STRREF_UPGRADE_SLOT_ACCELERATOR },
    { 2,          "i_powerc", STRREF_UPGRADE_SLOT_FRAME },
    { static_cast<unsigned>(-1), "i_powerc", 0 },
    { static_cast<unsigned>(-1), "i_powerc", 0 },
    { static_cast<unsigned>(-1), "i_powerc", 0 },
};

CSWGuiSwoopUpgrade::CSWGuiSwoopUpgrade(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    CExoString sName;  // format scratch

    C2DA* pC2DA = new C2DA(CResRef("swoopupgrade"), 0);
    m_pC2DA = pC2DA;
    m_pC2DA->Load2DArray();

#ifdef K2_UI_XBOX
// The console layout (swoopup_x) has no BTN_BACK, so the LBL_BACK caption binds into the button
// member instead. Unused content in KOTOR II.

    const int nSavedGUIWidth  = g_nGUIWidth;
    const int nSavedGUIHeight = g_nGUIHeight;
    g_nGUIWidth  = 640;
    g_nGUIHeight = 480;
    StartLoadFromLayout(CResRef("swoopup_x"), 1, 1, 1);
#else
    StartLoadFromLayout(CResRef("swoopup_p"), 1, 1, 1);
#endif

    InitControl(m_lblTitle, CExoString("LBL_TITLE"), 1, 1, 0, 0, 0);
    InitControl(m_lbItems, CExoString("LB_ITEMS"), 1, 1, 0, 0, 0);
    InitControl(m_lbDesc, CExoString("LB_DESC"), 1, 1, 0, 0, 0);
    m_lbDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_lbDescLS, CExoString("LB_DESC"), 1, 1, 0, 0, 0);
    m_lbDesc.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_lblDescBg, CExoString("LBL_DESCBG"), 1, 1, 0, 0, 0);
    InitControl(m_lblDescBgLS, CExoString("LBL_DESCBG_LS"), 1, 1, 0, 0, 0);
    InitControl(m_viewItem, CExoString("3D_MODEL"), 1, 1, 0, 0, 0);
    InitControl(m_viewItemLS, CExoString("3D_MODEL_LS"), 1, 1, 0, 0, 0);
    InitControl(m_lblSlotName, CExoString("LBL_SLOTNAME"), 1, 1, 0, 0, 0);
    InitControl(m_lblLSSlotName, CExoString("LBL_SLOTNAME"), 1, 1, 0, 0, 0);
    InitControl(m_lblUpgrades, CExoString("LBL_UPGRADES"), 1, 1, 0, 0, 0);
    InitControl(m_lblUpgradeCount, CExoString("LBL_UPGRADE_COUNT"), 1, 1, 0, 0, 0);
    InitControl(m_lblA, CExoString("LBL_A"), 1, 1, 0, 0, 0);
    InitControl(m_lblB, CExoString("LBL_B"), 1, 1, 0, 0, 0);
    InitControl(m_lblAAction, CExoString("LBL_A_ACTION"), 1, 1, 0, 0, 0);
#ifndef K2_UI_XBOX  // console: LBL_BACK binds into BTN_BACK's member below
    InitControl(m_lblBack, CExoString("LBL_BACK"), 1, 1, 0, 0, 0);
#endif
    InitControl(m_lblProperty, CExoString("LBL_PROPERTY"), 1, 1, 0, 0, 0);
    InitControl(m_lblRanalog, CExoString("LBL_RANALOG"), 1, 1, 0, 0, 0);
    InitControl(m_lblRanalogLS, CExoString("LBL_RANALOG_LS"), 1, 1, 0, 0, 0);
    InitControl(m_btnAssemble, CExoString("BTN_ASSEMBLE"), 1, 1, 0, 0, 0);
#ifdef K2_UI_XBOX
    InitControl(m_btnBack, CExoString("LBL_BACK"), 1, 1, 0, 0, 0);
#else
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1, 1, 0, 0, 0);
#endif

    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSwoopUpgrade::OnCloseSwoopUpgrade));

    m_btnAssemble.m_tail.m_nClickSound = GUISOUND_UPGRADE;
    m_btnAssemble.AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSwoopUpgrade::OnAssemble));
    m_btnAssemble.AddEvent(INPUTEVENT_GUI_START, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSwoopUpgrade::OnAssemble));
    m_btnAssemble.AddEvent(SWGUIEVENT_FOCUS, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSwoopUpgrade::OnEnterAssemble));

    m_btnAssemble.m_border.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    m_btnAssemble.m_borderHilight.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
    m_btnAssemble.m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);

    for (int i = 0; i < 6; i++) {  // one LBL_/BTN_UPGRADE3%d pair per upgrade slot
        sName.Format(const_cast<char*>("LBL_UPGRADE3%d"), i + 1);
        InitControl(m_apRows[i], sName, 1, 1, 0, 0, 0);
        sName.Format(const_cast<char*>("BTN_UPGRADE3%d"), i + 1);
        InitControl(m_abtnSlot[i], sName, 1, 1, 0, 0, 0);
        m_abtnSlot[i].AddEvent(INPUTEVENT_GUI_A, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSwoopUpgrade::OnSlotSelected));
        m_abtnSlot[i].AddEvent(INPUTEVENT_GUI_START, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSwoopUpgrade::OnSlotSelected));
        m_abtnSlot[i].AddEvent(SWGUIEVENT_FOCUS, this,
            static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSwoopUpgrade::OnEnterSlot));
        m_abtnSlot[i].m_tail.m_nUserData = i;
        m_abtnSlot[i].m_borderHilight.m_params.SetPulsingAlpha(1, 0, 1);
    }
    StopLoadFromLayout();
#ifdef K2_UI_XBOX
    g_nGUIWidth  = nSavedGUIWidth;
    g_nGUIHeight = nSavedGUIHeight;
#endif

    m_lblSlotName.m_border.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    m_lblLSSlotName.m_border.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    m_lblUpgradeCount.m_border.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    m_lblUpgrades.m_border.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    m_lblProperty.m_border.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
    m_lbItems.m_border.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);

    CSWGuiLabel* pProtoLabel = static_cast<CSWGuiLabel*>(m_lbDescLS.m_pProtoItem);
    if (pProtoLabel)
        m_lblDescription.Initialize(pProtoLabel->m_extent, pProtoLabel, 1.0f);

    CSWGuiButton* pBtn = static_cast<CSWGuiButton*>(m_lbItems.m_pProtoItem);
    if (pBtn) {
        pBtn->m_border.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
        pBtn->m_borderHilight.m_params.m_color = *reinterpret_cast<CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
        pBtn->m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);
    }

    m_nMenuType = CSWSItem::UPGRADABLE_TYPE_NONE;
    m_bItemsOpen &= 0xfe;
    m_pItem = 0;
    m_pItemSlot = 0;
    __builtin_memset(m_apaItems, 0, sizeof(m_apaItems));
    m_aswoopRow[0] = 0;
    m_aswoopRow[1] = 0;
    m_aswoopRow[4] = 0;
    m_aswoopRow[5] = 0;
    m_aswoopRow[2] = 0;
    m_aswoopRow[3] = 0;

    if (g_bDo3DGUIs) {
        m_viewItem.m_scene.m_pScene->SpawnRoom(const_cast<char*>("gui3D_room"), Vector(0, 0, 0), Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
        m_viewItem.m_scene.AddModel(CExoString("upgitem_light"), -1);
        m_viewItemLS.m_scene.m_pScene->SpawnRoom(const_cast<char*>("gui3D_room"), Vector(0, 0, 0), Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
        m_viewItemLS.m_scene.AddModel(CExoString("upgitem_light"), -1);
    }
}

// Collapses the item list, or hides the panel.

void CSWGuiSwoopUpgrade::OnCloseSwoopUpgrade(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);

    if (m_bItemsOpen & 1) {
        ShowItems(0);
        m_bItemsOpen &= ~1;
        SetActiveControl(static_cast<CSWGuiControl*>(m_pActiveSlotButton), 0);
        UpdateItems();
        OnEnterSlot(m_pActiveControl);
    } else {
        CancelPanel();
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSwoopUpgradeScreen();
    }
}

// Handles Assemble by committing the six pending upgrade-row indices to the party's swoop table.

void CSWGuiSwoopUpgrade::OnAssemble(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_START)
        m_pGuiManager->PlayGuiSound(pControl->m_tail.m_nClickSound);

    for (int i = 0; i < 6; i++) {
        if (m_aswoopRow[i]) {
            CSWPartyTable* pParty = g_pAppManager->m_pServerExoApp->GetPartyTable();
            pParty->m_swoop[i] = m_aswoopRow[i];
        }
    }

    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSwoopUpgradeScreen();
}

// On Assemble hover, repaints the six action labels.

void CSWGuiSwoopUpgrade::OnEnterAssemble(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    m_lblAAction.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
    m_lblAAction.m_text.m_params.SetStrRef(STRREF_ASSEMBLE);

    m_lblSlotName.m_text.m_params.SetText(CExoString(""));
    m_lblLSSlotName.m_text.m_params.SetText(CExoString(""));
    m_lblUpgradeCount.m_text.m_params.SetText(CExoString(""));
    m_lblProperty.m_text.m_params.SetText(CExoString(""));
    m_lblUpgrades.m_text.m_params.SetText(CExoString(""));
}

// On a slot click, collects the items the clicked slot can take into a local list and hands the
// list to the item list box.

void CSWGuiSwoopUpgrade::OnSlotSelected(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_START)
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);

    CServerExoApp* pApp = g_pAppManager->m_pServerExoApp;
    int n = static_cast<int>(pControl->m_tail.m_nUserData);
    CItemRepository* pRepo = pApp->GetPartyTable()->GetPartyItems();

    if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER)
    {
        CExoArrayList<CSWGuiControl*> list;
        int nBase = (n & ~2) == 0 ? 0 : n - 2;

        if (n == 1)
        {
            CExoString sVal;
            unsigned long long oid;
            list.Add(CreateItemEntry(m_apaItems[1]->m_objectId, -1));
            for (int r = 0; r < m_pUpgradeC2DA2->m_nRows; r++)
            {
                m_pUpgradeC2DA2->GetCExoStringEntry(r, CExoString("Template"), &sVal);
                sVal = sVal.LowerCase();
                if (sVal != m_apaItems[1]->m_sTag)
                {
                    oid = pRepo->FindItemWithTag(sVal);
                    if (oid != OBJECT_INVALID)
                        list.Add(CreateItemEntry(oid, r));
                }
            }
        }
        else if (n > 2)
        {
            CExoString sVal;
            int nGroup;
            list.Add(CreateItemEntry(OBJECT_INVALID, -1));
            if (m_apaItems[n])
                list.Add(CreateItemEntry(m_apaItems[n]->m_objectId, -1));
            for (int r = 0; r < m_pUpgradeC2DA->m_nRows; r++)
            {
                m_pUpgradeC2DA->GetINTEntry(r, CExoString("group"), &nGroup);
                if (nGroup != nBase)
                    continue;
                m_pUpgradeC2DA->GetCExoStringEntry(r, CExoString("template"), &sVal);
                unsigned long long oid = pRepo->FindItemWithTag(sVal.LowerCase());
                if (oid == OBJECT_INVALID)
                    continue;
                if (m_apaItems[2 * (n == 0)] && m_anUpgradeRow[2 * (n == 0)] == r)
                    continue;
                if (m_apaItems[n] && m_anUpgradeRow[n] == r)
                    continue;
                list.Add(CreateItemEntry(oid, r));
            }
        }
        else
        {
            CExoString sVal;
            int nGroup;
            list.Add(CreateItemEntry(OBJECT_INVALID, -1));
            if (m_apaItems[n])
                list.Add(CreateItemEntry(m_apaItems[n]->m_objectId, -1));
            for (int r = 0; r < m_pC2DA->m_nRows; r++)
            {
                m_pC2DA->GetINTEntry(r, CExoString("group"), &nGroup);
                if (nGroup != n)
                    continue;
                m_pC2DA->GetCExoStringEntry(r, CExoString("template"), &sVal);
                unsigned long long oid = pRepo->FindItemWithTag(sVal.LowerCase());
                if (oid == OBJECT_INVALID)
                    continue;
                if (m_apaItems[2 * (n == 0)] && m_anUpgradeRow[2 * (n == 0)] == r)
                    continue;
                if (m_apaItems[n] && m_anUpgradeRow[n] == r)
                    continue;
                list.Add(CreateItemEntry(oid, r));
            }
        }

        m_lbItems.AddControls(list, 1, 0, 0);
        m_viewItem.m_tail.bVisible = 0;
        m_viewItemLS.m_tail.bVisible = 0;
        m_lblSlotName.m_tail.bVisible = 0;
        m_lblLSSlotName.m_tail.bVisible = 0;
        m_lblUpgradeCount.m_tail.bVisible = 0;
        m_lblUpgrades.m_tail.bVisible = 0;
        m_lblProperty.m_tail.bVisible = 0;
        m_btnAssemble.m_tail.bVisible = 0;
        for (int i = 0; i < 6; i++)
        {
            m_abtnSlot[i].m_tail.bVisible = 0;
            m_apRows[i].m_tail.bVisible = 0;
        }
        m_lbItems.m_tail.bVisible = 1;
        SetActiveControl(&m_lbItems, 0);
        m_pActiveSlotButton = pControl;
        m_bItemsOpen |= 1;
    }
    else
    {
        CExoArrayList<CSWGuiControl*> list;
        {
            CExoString sVal;
            int nGroup;
            list.Add(CreateItemEntry(OBJECT_INVALID, -1));
            if (m_apaItems[n])
                list.Add(CreateItemEntry(m_apaItems[n]->m_objectId, -1));
            for (int r = 0; r < m_pC2DA->m_nRows; r++)
            {
                m_pC2DA->GetINTEntry(r, CExoString("group"), &nGroup);
                if (nGroup != n)
                    continue;
                m_pC2DA->GetCExoStringEntry(r, CExoString("template"), &sVal);
                unsigned long long oid = pRepo->FindItemWithTag(sVal.LowerCase());
                if (oid == OBJECT_INVALID)
                    continue;
                if (m_apaItems[2 * (n == 0)] && m_anUpgradeRow[2 * (n == 0)] == r)
                    continue;
                if (m_apaItems[n] && m_anUpgradeRow[n] == r)
                    continue;
                g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oid);
                list.Add(CreateItemEntry(oid, r));
            }
        }

        m_lbItems.AddControls(list, 1, 0, 0);
        m_viewItem.m_tail.bVisible = 0;
        m_viewItemLS.m_tail.bVisible = 0;
        m_lblSlotName.m_tail.bVisible = 0;
        m_lblLSSlotName.m_tail.bVisible = 0;
        m_lblUpgradeCount.m_tail.bVisible = 0;
        m_lblUpgrades.m_tail.bVisible = 0;
        m_lblProperty.m_tail.bVisible = 0;
        m_btnAssemble.m_tail.bVisible = 0;
        for (int i = 0; i < 6; i++)
        {
            m_abtnSlot[i].m_tail.bVisible = 0;
            m_apRows[i].m_tail.bVisible = 0;
        }
        m_lbItems.m_tail.bVisible = 1;
        SetActiveControl(&m_lbItems, 0);
        m_pActiveSlotButton = pControl;
        m_bItemsOpen |= 1;
    }
}

// On slot hover, repaints the slot name, the property line and the action label from the slot's
// item.

void CSWGuiSwoopUpgrade::OnEnterSlot(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    int nSlot = static_cast<int>(pControl->m_tail.m_nUserData);

    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    unsigned long long oid =
        pServer->GetPartyTable()->GetPartyItems()->FindItemWithTag(m_aslotTag[nSlot]);
    CSWSItem* pItem = pServer->GetItemByGameObjectID(oid);
    int nStack = pItem ? pItem->m_stackCount : 0;

    if (m_apaItems[nSlot]) {
        CExoString sText;
        CExoLocString loc = m_apaItems[nSlot]->m_localizedName;
        loc.GetString(g_pAppManager->m_pClientExoApp->GetClientLanguage(), &sText, 0);
        int nLt = sText.Find('<', 0);
        int nLb = sText.Find('{', 0);
        if ((nLt & nLb) != -1) {
            CSWTlkTable* pTlk = g_pSWTlkTable;
            pTlk->m_pSpeakerObj =
                g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
            pTlk->m_gender = pTlk->GetObjectGender();
            pTlk->m_bSuppressEnabled = static_cast<unsigned char>(pTlk->m_bSuppressEnabled | 1);
            pTlk->ParseStr(sText);
        }
        m_lblSlotName.m_text.m_params.SetText(sText);
        m_lblProperty.m_text.m_params.SetText(sText);

        int nDescRef = 0;
        int nLevel;
        CSWSItem* pSlotItem = m_apaItems[nSlot];
        CExoString sVal;
        for (int r = 0; r < m_pC2DA->m_nRows; r++) {
            m_pC2DA->GetCExoStringEntry(r, CExoString("template"), &sVal);
            if (pSlotItem->m_sTag.LowerCase() == sVal.LowerCase()) {
                m_pC2DA->GetINTEntry(r, CExoString("level"), &nLevel);
                m_pC2DA->GetINTEntry(r, CExoString("desc_strref"), &nDescRef);
                CExoString sTlk = g_pTlkTable->GetSimpleString(nDescRef);
                m_lblProperty.m_text.m_params.SetText(sTlk + " " + CExoString(nLevel));
            }
        }
    } else {
        m_lblProperty.m_text.m_params.SetStrRef(SWOOP_SLOT_INFO[nSlot].m_nStrRef);
        m_lblSlotName.m_text.m_params.SetStrRef(SWOOP_SLOT_INFO[nSlot].m_nStrRef);
    }

    m_lblUpgradeCount.m_text.m_params.SetText(CExoString(nStack));
    m_lblUpgrades.m_text.m_params.SetStrRef(STRREF_UPGRADES);

    if (nStack == 0 && m_apaItems[nSlot] == 0) {
        m_lblAAction.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        m_lblAAction.m_text.m_params.SetStrRef(STRREF_EQUIP);
    } else {
        m_lblAAction.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
        m_lblAAction.m_text.m_params.SetStrRef(m_apaItems[nSlot] ? STRREF_UNEQUIP : STRREF_EQUIP);
    }
}

// Only the owned 2DA table and two of the three list boxes are released explicitly.

CSWGuiSwoopUpgrade::~CSWGuiSwoopUpgrade()
{
    m_lbItems.ClearItems();
    m_lbDesc.ClearItems();
    if (m_pC2DA) {
        delete m_pC2DA;
        m_pC2DA = 0;
    }
}

int CSWGuiSwoopUpgrade::GetNumUpgrades()
{
    return 3;
}

void CSWGuiSwoopUpgrade::CloseItems()
{
    ShowItems(0);
    m_bItemsOpen &= ~1;
    SetActiveControl(static_cast<CSWGuiControl*>(m_pActiveSlotButton), 0);
    UpdateItems();
    OnEnterSlot(m_pActiveControl);
}

// Toggles the item list, hiding the slot widgets while the list is shown.

void CSWGuiSwoopUpgrade::ShowItems(int n)
{
    if (n) {
        m_viewItem.m_tail.bVisible = 0;
        m_viewItemLS.m_tail.bVisible = 0;
        m_lblSlotName.m_tail.bVisible = 0;
        m_lblLSSlotName.m_tail.bVisible = 0;
        m_lblUpgradeCount.m_tail.bVisible = 0;
        m_lblUpgrades.m_tail.bVisible = 0;
        m_lblProperty.m_tail.bVisible = 0;
        m_btnAssemble.m_tail.bVisible = 0;
        for (int i = 0; i < 6; i++) {
            m_abtnSlot[i].m_tail.bVisible = 0;
            m_apRows[i].m_tail.bVisible = 0;
        }
        m_lbItems.m_tail.bVisible = 1;
        return;
    }

    if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER) {
        m_viewItem.m_tail.bVisible = 0;
        m_lblSlotName.m_tail.bVisible = 0;
        m_lblUpgradeCount.m_tail.bVisible = 0;
        m_lblUpgrades.m_tail.bVisible = 0;
        m_lblProperty.m_tail.bVisible = 0;
        m_lblDescBg.m_tail.bVisible = 0;
        m_lbDesc.m_tail.bVisible = 0;
        m_lblRanalog.m_tail.bVisible = 0;
        m_viewItemLS.m_tail.bVisible = 1;
        m_lblLSSlotName.m_tail.bVisible = 1;
        m_btnAssemble.m_tail.bVisible = 1;
        m_lblDescBgLS.m_tail.bVisible = 1;
        m_lbDescLS.m_tail.bVisible = 1;
        m_lblRanalogLS.m_tail.bVisible = 1;
        m_btnAssemble.SetMoveToControl(0, &m_abtnSlot[1]);
        m_btnAssemble.SetMoveToControl(2, &m_abtnSlot[4]);
        for (int i = 0; i < 6; i++) {
            m_abtnSlot[i].m_tail.bVisible = 1;
            m_apRows[i].m_tail.bVisible = 1;
        }
    } else {
        m_viewItem.m_tail.bVisible = 1;
        m_lblSlotName.m_tail.bVisible = 1;
        m_lblUpgradeCount.m_tail.bVisible = 0;
        m_lblUpgrades.m_tail.bVisible = 0;
        m_lblProperty.m_tail.bVisible = 1;
        m_btnAssemble.m_tail.bVisible = 1;
        m_lblDescBg.m_tail.bVisible = 1;
        m_lbDesc.m_tail.bVisible = 1;
        m_lblRanalog.m_tail.bVisible = 1;
        m_viewItemLS.m_tail.bVisible = 0;
        m_lblLSSlotName.m_tail.bVisible = 0;
        m_lblDescBgLS.m_tail.bVisible = 0;
        m_lbDescLS.m_tail.bVisible = 0;
        m_lblRanalogLS.m_tail.bVisible = 0;
        m_btnAssemble.SetMoveToControl(0, &m_abtnSlot[1]);
        m_btnAssemble.SetMoveToControl(2, &m_abtnSlot[1]);
        for (int i = 0; i < 3; i++) {
            m_abtnSlot[i].m_tail.bVisible = 1;
            m_apRows[i].m_tail.bVisible = 1;
        }
        for (int i = 3; i < 6; i++) {
            m_abtnSlot[i].m_tail.bVisible = 0;
            m_apRows[i].m_tail.bVisible = 0;
        }
    }

    m_lbItems.m_tail.bVisible = 0;
}

// Re-textures the three slot labels' borders from the current per-slot item.

void CSWGuiSwoopUpgrade::UpdateItems()
{
    CResRef rIcon;
    CExoString sUnused;

    for (int i = 0; i < 3; i++)
    {
        CSWGuiLabel* pLabel = &m_apRows[i];
        if (m_apaItems[i])
        {
            g_pRules->GetBaseItem(m_apaItems[i]->m_baseItem);  // result unused
            rIcon = m_apaItems[i]->GetIcon();
        }
        else
        {
            rIcon = const_cast<char*>(SWOOP_SLOT_INFO[i].m_szIcon);
        }
        pLabel->m_border.m_params.SetFillImage(rIcon, 0);
    }
}

// When the panel is added, fills the three upgrade slots from the swoop-upgrade rows and records
// their indices.

void CSWGuiSwoopUpgrade::OnPanelAdded()
{
    m_pItemSlot = new CSWSItem(OBJECT_INVALID);
    m_pItemSlot->CopyItem(m_pItem);

    for (int i = 0; i < 3; i++)
        m_abtnSlot[i].SetMoveToControl(0, &m_abtnSlot[i]);

    ShowItems(0);

    CExoString sVal;
    for (int r = 0; r < m_pC2DA->m_nRows; r++)
    {
        int nGroup;
        int nLevel;
        m_pC2DA->GetINTEntry(r, CExoString("group"), &nGroup);
        m_pC2DA->GetCExoStringEntry(r, CExoString("template"), &sVal);
        m_pC2DA->GetINTEntry(r, CExoString("level"), &nLevel);
        if (nLevel == 0)
            continue;

        int nSlot;
        CSWSItem** ppSlot;
        if (m_apaItems[0] == 0 && nGroup == 0) {
            nSlot = 0;
            ppSlot = &m_apaItems[0];
        } else if (m_apaItems[1] == 0 && nGroup == 1) {
            nSlot = 1;
            ppSlot = &m_apaItems[1];
        } else if (m_apaItems[2] == 0 && nGroup == 2) {
            nSlot = 2;
            ppSlot = &m_apaItems[2];
        } else {
            continue;
        }

        m_aslotTag[nSlot] = sVal.LowerCase();
        m_apRows[nSlot].m_border.m_params.m_fAlpha = 1.0f;
        m_anUpgradeRow[nSlot] = r;
        if (g_pAppManager->m_pServerExoApp->GetPartyTable()->m_swoop[nSlot] == r)
        {
            *ppSlot = new CSWSItem(OBJECT_INVALID);
            (*ppSlot)->LoadFromTemplate(CResRef(sVal));
            (*ppSlot)->m_anUpgVars[nSlot] = r;
            m_aswoopRow[nSlot] = r;
        }
        else
        {
            unsigned long long oid =
                g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems()->FindItemWithTag(
                    m_aslotTag[nSlot]);
            if (oid == OBJECT_INVALID)
                m_apRows[nSlot].m_border.m_params.m_fAlpha = 0.25f;
        }
    }

    UpdateItems();
    SetActiveControl(static_cast<CSWGuiControl*>(&m_btnAssemble), 0);
    CSWGuiPanel::OnPanelAdded();
}

// On panel teardown, releases the slot item pointers and nulls the item handles.

void CSWGuiSwoopUpgrade::OnPanelRemoved()
{
    m_nMenuType = CSWSItem::UPGRADABLE_TYPE_NONE;
    m_pItem = 0;
    if (m_pItemSlot) {
        delete m_pItemSlot;
        m_pItemSlot = 0;
    }
    if (m_apaItems[0]) {
        delete m_apaItems[0];
        m_apaItems[0] = 0;
    }
    if (m_apaItems[1]) {
        delete m_apaItems[1];
        m_apaItems[1] = 0;
    }
    if (m_apaItems[2]) {
        delete m_apaItems[2];
        m_apaItems[2] = 0;
    }
    if (m_apaItems[3]) {
        delete m_apaItems[3];
        m_apaItems[3] = 0;
    }
    if (m_apaItems[4]) {
        delete m_apaItems[4];
        m_apaItems[4] = 0;
    }
    if (m_apaItems[5]) {
        delete m_apaItems[5];
        m_apaItems[5] = 0;
    }
    m_lstItemsAddedToRepo.m_count = 0;
    m_lstItemsTakenFromRepo.m_count = 0;
    CSWGuiPanel::OnPanelRemoved();
}

// On row hover, shows the item's localized name, or the panel's GUI string when it has none.

void CSWGuiSwoopUpgrade::OnControlEntered(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    if (static_cast<CSWUpgradeItemEntry*>(pControl)->m_oidItem == OBJECT_INVALID) {
        CExoString s("");
        SetDescription(s);
    } else {
        CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
            static_cast<CSWUpgradeItemEntry*>(pControl)->m_oidItem);
        CExoString sText;
        pItem->m_descriptionIdentified.GetString(
            g_pAppManager->m_pClientExoApp->GetClientLanguage(), &sText, 0);
        if (sText != "") {
            SetDescription(sText);
        } else {
            CExoString sGui = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_NO_DESCRIPTION_SET);
            SetDescription(sGui);
        }
    }
}

// Re-texts and re-fits the description label, then re-pins it into the regime's description box.

void CSWGuiSwoopUpgrade::SetDescription(const CExoString& s)
{
    m_lblDescription.m_text.m_params.SetText(s);
    CSWGuiListBox* pList = &m_lbDesc;
    CSWGuiLabel* pArrow = &m_lblRanalog;
    if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER) {
        pList = &m_lbDescLS;
        pArrow = &m_lblRanalogLS;
    }
    pList->ClearItems();
    CSWGuiControl* pCaption;
    CSWGuiText* pText = &m_lblDescription.m_text;
    CSWGuiExtent ext = m_lblDescription.m_extent;
    if (s.GetLength() >= 1)
        ext.h = pText->GetIdealHeight();
    else
        ext.h = pText->GetFontHeight();
    m_lblDescription.SetExtent(ext);
    pCaption = &m_lblDescription;
    pList->AddControls(&pCaption, 1, 0, 0, 0);
    pList->SetSelectedControl(&m_lblDescription, 0);
    pArrow->m_tail.bVisible = pList->IsOversized();
}

void CSWGuiSwoopUpgrade::Draw(float f)
{
    CSWGuiPanel::Draw(f);
}

// Appearance-table model and texture resrefs for the armor slot.

void CSWGuiSwoopUpgrade::GetArmorModelAndTextureResRef(int a, int b, int c,
                                                       CExoString& rModel, CExoString& rTexture)
{
    char szModelCol[7] = "ModelA";  // the trailing 'A' is replaced with the slot letter
    char szTexCol[5] = "TEXA";
    int nSlot = (b < 14) ? b : 14;
    char cLetter = (nSlot < 1) ? 'A' : static_cast<char>(nSlot + ('A' - 1));
    szModelCol[5] = cLetter;
    szTexCol[3] = cLetter;
    int n = (c > 0) ? c : 1;

    if (!g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(a, CExoString(szModelCol), &rModel))
        rModel = "";
    if (!g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(a, CExoString(szTexCol), &rTexture))
    {
        rTexture = "";
    }
    else
    {
        rTexture.Format(const_cast<char*>("%s%02d"), rTexture.CStr(), n);
        if (!g_pExoResMan->Exists(CResRef(rTexture), 3, static_cast<unsigned int*>(0)) &&
            !g_pExoResMan->Exists(CResRef(rTexture), RESTYPE_TXB, static_cast<unsigned int*>(0)))
            rTexture.Format(const_cast<char*>("%s%02d"), rModel.CStr(), 1);
    }
}

// Rebuilds the 3D preview for the current panel regime.

void CSWGuiSwoopUpgrade::Update3DView()
{
    if (!g_bDo3DGUIs)
        return;

    CSWGui3DSceneView* pView = &m_viewItem;
    if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER)
        pView = &m_viewItemLS;
    CSWGuiScene* pScene = &pView->m_scene;
    pScene->RemoveModel(1, 1);

    CExoString sModel;
    CExoString sTexture;
    CSWBaseItem* pBase = m_pItem->GetBaseItem();
    if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_ARMOR)
    {
        GetArmorModelAndTextureResRef(DISGUISE_TYPE_P_MAL_A_MED_01, pBase->m_nBodyVar, m_pItem->m_nTextureVar,
                                      sModel, sTexture);
    }
    else
    {
        pBase->GetModelResRef(m_pItem->m_nModelVariation, 'm').CopyToString(sModel);
    }

    CExoString sCameraHook;
    CExoString sRotateHook;
    sCameraHook.Format(const_cast<char*>("camerahook%d"), pBase->m_nItemType);
    sRotateHook.Format(const_cast<char*>("rotatehook%d"), pBase->m_nItemType);
    if (pBase->m_nItemType == ITEM_TYPE_RIFLE && m_pItem->m_baseItem == BASE_ITEM_BLASTER_CARBINE)
    {
        sCameraHook = sCameraHook + "a";
        sRotateHook = sRotateHook + "a";
    }
    else if (pBase->m_nItemType == ITEM_TYPE_DOUBLE_WEAPON && m_pItem->m_baseItem == BASE_ITEM_STUN_BATON)
    {
        sCameraHook = sCameraHook + "a";
        sRotateHook = sRotateHook + "a";
    }

    CAurCamera* pCamera = pView->m_scene.m_pCamera;
    CAurObject* pModel = pScene->GetModel(0);
    if (pModel->GetPart(static_cast<char*>(sCameraHook.CStr())) == 0)
        sCameraHook = "camerahook";
    pCamera->AttachToObject(pScene->GetModel(0), static_cast<char*>(sCameraHook.CStr()), 0);
    pCamera->SetViewAngle(22.726f);

    CAurObject* pNewModel = pScene->AddModel(sModel, -1);
    if (pNewModel != 0)
    {
        if (sTexture.m_sString != 0 && *sTexture.m_sString != 0)
            pNewModel->ReplaceTexture(const_cast<char*>(""), static_cast<char*>(sTexture.CStr()));
        if (pModel->GetPart(static_cast<char*>(sRotateHook.CStr())) != 0)
        {
            pNewModel->AttachToObject(pModel, static_cast<char*>(sRotateHook.CStr()), 0);
            pModel->PlayAnimation(const_cast<char*>("rotate"), 1.0f, 0, 0.0f);
        }
        if (m_pItem->GetBaseItem()->m_bPoweredItem != 0)
            pNewModel->PlayAnimation(const_cast<char*>("powered"), 1.0f, 0, 0.0f);
        if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_ARMOR)
            pNewModel->PlayAnimation(const_cast<char*>("neutral"), 1.0f, 0, 0.0f);
    }
}

// On a slot click, moves the slot's item back to the party and installs the row's replacement.

void CSWGuiSwoopUpgrade::OnUpgradeSelected(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_START)
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
    CSWUpgradeItemEntry* pRow = static_cast<CSWUpgradeItemEntry*>(pControl);
    CServerExoApp* pApp = g_pAppManager->m_pServerExoApp;
    CItemRepository* pRepo = pApp->GetPartyTable()->GetPartyItems();
    int nMenu = static_cast<int>(m_pActiveSlotButton->m_tail.m_nUserData);

    if (pRow->m_nUpgradeRow == -1 && pRow->m_oidItem != OBJECT_INVALID)
        goto FINALIZE;

    {
        CSWSItem** ppSlot = &m_apaItems[nMenu];
        if (*ppSlot != 0)
        {
            (*ppSlot)->SetPossessor(pApp->GetPlayerCreatureId(), 0, 0, OBJECT_INVALID);
            int nFound = m_lstItemsTakenFromRepo.IndexOf(*ppSlot);
            if (nFound != -1)
            {
                m_lstItemsTakenFromRepo.DelIndex(nFound);
                pRepo->AddItem(ppSlot, 1, 0, 0);
            }
            else
            {
                pRepo->AddItem(ppSlot, 1, 0, 0);
                m_lstItemsAddedToRepo.Add(*ppSlot);
            }
            if (nMenu != 1 || m_nMenuType != CSWSItem::UPGRADABLE_TYPE_LIGHTSABER)
                m_apaItems[nMenu]->m_anUpgVars[nMenu] = -1;
            *ppSlot = 0;
        }

        CSWSItem* pItem = pApp->GetItemByGameObjectID(pRow->m_oidItem);
        if (pItem == 0)
            goto FINALIZE;
        int nFound = m_lstItemsAddedToRepo.IndexOf(pItem);
        if (nFound != -1)
        {
            m_lstItemsAddedToRepo.DelIndex(nFound);
            if (pItem->m_stackCount >= 2)
                pItem = pItem->SplitItem(1);
            else
                pRepo->RemoveItem(pItem);
        }
        else
        {
            if (pItem->m_stackCount > 1)
                pItem = pItem->SplitItem(1);
            else
                pRepo->RemoveItem(pItem);
            m_lstItemsTakenFromRepo.Add(pItem);
        }

        int nRowMatch = 0;
        int nRows = m_pC2DA->m_nRows;
        for (int r = 0; r < nRows; r++)
        {
            CExoString val;
            m_pC2DA->GetCExoStringEntry(r, CExoString("template"), &val);
            if (_stricmp(pItem->m_sTag.CStr(), val.CStr()) == 0)
            {
                nRowMatch = r;
                break;
            }
        }
        m_aswoopRow[nMenu] = nRowMatch;
        *ppSlot = pItem;
        m_anUpgradeRow[nMenu] = pRow->m_nUpgradeRow;
    }

FINALIZE:
    ShowItems(0);
    m_bItemsOpen &= ~1;
    SetActiveControl(m_pActiveSlotButton, 0);
    UpdateItems();
    OnEnterSlot(m_pActiveControl);
}

// Builds one row widget for an upgrade slot.

CSWUpgradeItemEntry* CSWGuiSwoopUpgrade::CreateItemEntry(unsigned long long oid, int nSlot)
{
    CSWGuiButton* pProto = static_cast<CSWGuiButton*>(m_lbItems.m_pProtoItem);
    int nWide = m_lbItems.m_innerExtent.w - 2 * static_cast<int>(m_lbItems.m_nPadding);
    CSWUpgradeItemEntry* pEntry = new CSWUpgradeItemEntry;
    pEntry->Initialize(pProto->m_text, pProto->m_border, pProto->m_borderHilight, nWide, 0);
    pEntry->AddEvent(SWGUIEVENT_FOCUS, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSwoopUpgrade::OnControlEntered));
    pEntry->AddEvent(INPUTEVENT_GUI_A, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSwoopUpgrade::OnUpgradeSelected));
    pEntry->AddEvent(INPUTEVENT_GUI_START, this,
        static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiSwoopUpgrade::OnUpgradeSelected));
    pEntry->SetItem(oid, 0, OBJECT_INVALID, 0);
    pEntry->m_nUpgradeRow = nSlot;
    return pEntry;
}

// The confirm-box callback runs the upgrade when it was accepted and clears the item's verify mask
// otherwise.

void CSWGuiSwoopUpgrade::VerifyUpgrade(CSWGuiControl* pControl)
{
    int nSlot = static_cast<int>(m_pActiveControl->m_tail.m_nUserData);
    CSWGuiMessageBox* pBox =
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    if (pBox->m_bAccepted) {
        InsertUpgrade(reinterpret_cast<CSWSItem*>(pControl), nSlot);
        UpdateItems();
        OnEnterSlot(m_pActiveControl);
    } else {
        CSWSItem* pItem = reinterpret_cast<CSWSItem*>(pControl);
        pItem->m_nUpgrades &= ~(1u << (m_anUpgradeRow[nSlot] & 0xff));
    }
}

// The description scroll keys are re-issued to whichever description box the regime shows.

void CSWGuiSwoopUpgrade::HandleInputEvent(int type, int val)
{
    if (val != 0)
    {
        switch (type)
        {
            case INPUTEVENT_GUI_SCROLL_DOWN:
                if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER)
                    m_lbDescLS.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
                else
                    m_lbDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
                break;
            case INPUTEVENT_GUI_SCROLL_UP:
                if (m_nMenuType == CSWSItem::UPGRADABLE_TYPE_LIGHTSABER)
                    m_lbDescLS.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
                else
                    m_lbDesc.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
                break;
#ifdef K2_UI_XBOX
// The console build adds a B/back arm here that runs the close handler's body.

            case INPUTEVENT_GUI_B:
            case INPUTEVENT_GUI_BACK:
                OnCloseSwoopUpgrade(0);
                break;
#endif
            default:
                break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);

}

// Installs one unit of an item into the panel, splitting the stack or removing the item from the
// party repository first.

void CSWGuiSwoopUpgrade::InsertUpgrade(CSWSItem* pItem, int nSlot)
{
    if (!pItem)
        return;
    int nFound = m_lstItemsAddedToRepo.IndexOf(pItem);
    if (nFound != -1)
    {
        m_lstItemsAddedToRepo.DelIndex(nFound);
        if (pItem->m_stackCount >= 2)
            pItem = pItem->SplitItem(1);
        else
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems()->RemoveItem(pItem);
        m_apaItems[nSlot] = pItem;
        return;
    }

    if (pItem->m_stackCount > 1)
        pItem = pItem->SplitItem(1);
    else
        g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems()->RemoveItem(pItem);

    m_lstItemsTakenFromRepo.Add(pItem);
    m_apaItems[nSlot] = pItem;
}

// Hands every pending upgrade item back to the party repository.

void CSWGuiSwoopUpgrade::CancelPanel()
{
    CItemRepository* pRepo =
        g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems();
    CSWSItem* pItem;
    for (int i = 0; i < m_lstItemsTakenFromRepo.m_count; i++)
    {
        pItem = m_lstItemsTakenFromRepo.m_pArray[i];
        if (m_apaItems[0] == pItem)
            m_apaItems[0] = 0;
        else if (m_apaItems[1] == pItem)
            m_apaItems[1] = 0;
        else if (m_apaItems[2] == pItem)
            m_apaItems[2] = 0;
        else if (m_apaItems[3] == pItem)
            m_apaItems[3] = 0;
        else if (m_apaItems[4] == pItem)
            m_apaItems[4] = 0;
        else if (m_apaItems[5] == pItem)
            m_apaItems[5] = 0;
        pRepo->AddItem(&pItem, 1, 0, 0);
    }
    for (int i = 0; i < m_lstItemsAddedToRepo.m_count; i++)
    {
        pItem = m_lstItemsAddedToRepo.m_pArray[i];
        if (pItem->m_stackCount >= 2)
        {
            pItem->m_stackCount--;
        }
        else
        {
            pRepo->RemoveItem(pItem);
            delete pItem;
        }
    }
}

#endif

// The character-upgrade (workbench) screen lists every item the party can upgrade.



























void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);


#if KOTOR1  // K1: picks an item type; the item list lives in CSWGuiUpgradeItemSelect
void IosAddButtonIcon(CSWGuiButton* pButton, char nIcon);

CSWGuiUpgradeSelection::CSWGuiUpgradeSelection(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    StartLoadFromLayout(CResRef("upgradesel"));
    InitControl(m_lblTitle, CExoString("LBL_TITLE"), 1);
    InitControl(m_abtnType[0], CExoString("BTN_LIGHTSABER"), 1);
    InitControl(m_abtnType[1], CExoString("BTN_RANGED"), 1);
    InitControl(m_abtnType[2], CExoString("BTN_MELEE"), 1);
    InitControl(m_abtnType[3], CExoString("BTN_ARMOR"), 1);
    InitControl(m_alblType[0], CExoString("LBL_LSABER"), 1);
    InitControl(m_alblType[1], CExoString("LBL_RANGED"), 1);
    InitControl(m_alblType[2], CExoString("LBL_MELEE"), 1);
    InitControl(m_alblType[3], CExoString("LBL_ARMOR"), 1);
    InitControl(m_btnBack, CExoString("BTN_BACK"), 1);
    InitControl(m_btnUpgradeItems, CExoString("BTN_UPGRADEITEMS"), 1);
    StopLoadFromLayout();
    ScaleExtentForResolution();

    m_itemObjId = OBJECT_INVALID;
    m_pUpgradeItemSelect = new CSWGuiUpgradeItemSelect(pGuiManager);
    m_pUpgradeItemSelect->m_pUpgradeSelection = this;

    for (int i = 0; i < 4; i++)
    {
        m_abtnType[i].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeSelection::OnTypeSelected));
        m_abtnType[i].AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeSelection::OnTypeSelected));
        m_abtnType[i].AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeSelection::OnEnter));
        m_abtnType[i].m_tail.m_nUserData = i;
        m_abtnType[i].m_border.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_BLUE);
        m_abtnType[i].m_borderHilight.m_params.m_color = *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_YELLOW);
        m_abtnType[i].m_borderHilight.m_params.SetPulsingAlpha(1, 1, 0);
        m_alblType[i].m_tail.bVisible = 0;
    }

    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnUpgradeItems.m_tail.bFocusOnHover = 0;
    m_btnUpgradeItems.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeSelection::OnTypeSelected));
    m_pSelected = NULL;
    IosAddButtonIcon(&m_btnUpgradeItems, 'a');
    IosAddButtonIcon(&m_btnBack, 'b');
}

void CSWGuiUpgradeSelection::OnTypeSelected(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_START)
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
    if (pControl == &m_btnUpgradeItems)
    {
        pControl = m_pSelected;
        if (pControl == NULL)
            return;
    }
    int n = static_cast<int>(pControl->m_tail.m_nUserData);
    if (m_abAvailable[n])
    {
        m_pUpgradeItemSelect->m_nUpgradableType = static_cast<unsigned char>(n + 1);
        m_pGuiManager->AddPanel(m_pUpgradeItemSelect, 3, 1);
    }
}

void CSWGuiUpgradeSelection::OnEnter(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    int n = static_cast<int>(pControl->m_tail.m_nUserData);
    CSWGuiControl* pType = pControl->AsRadioButton();
    if (pType == NULL)
        return;
    if (m_abAvailable[n])
    {
        m_btnUpgradeItems.m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
        m_btnUpgradeItems.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::ENABLED_ICON_COLOR);
        m_pSelected = pType;
    }
    else
    {
        m_btnUpgradeItems.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        m_btnUpgradeItems.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
        m_pSelected = NULL;
    }
}

CSWGuiUpgradeSelection::~CSWGuiUpgradeSelection()
{
    if (m_pUpgradeItemSelect != NULL)
    {
        delete m_pUpgradeItemSelect;
        m_pUpgradeItemSelect = NULL;
    }
}

void CSWGuiUpgradeSelection::GetUpgradableItems(CSWSCreature* pCreature, int* panAvailable,
                                                C2DA* pUpgradeC2DA)
{
    for (int i = 0; i < 18; i++)
    {
        CSWSItem* pItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(1 << i));
        if (pItem == NULL)
            continue;
        unsigned char nType = pItem->GetUpgradableType(pUpgradeC2DA);
        if (nType == 0)
            continue;
        panAvailable[nType - 1] = 1;
    }
}

void CSWGuiUpgradeSelection::OnPanelAdded()
{
    C2DA* pUpgradeC2DA = new C2DA(CResRef("upgrade"), 0);
    pUpgradeC2DA->Load2DArray();

    CSWSItem* pItem = NULL;
    if (m_itemObjId != OBJECT_INVALID &&
        (pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(m_itemObjId)) != NULL)
    {
        m_pUpgradeItemSelect->m_nUpgradableType = pItem->GetUpgradableType(pUpgradeC2DA);
        m_pUpgradeItemSelect->m_pItem = pItem;
        m_pGuiManager->AddPanel(m_pUpgradeItemSelect, 3, 1);
    }
    else
    {
        CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
        CSWPartyTable* pPartyTable = pServer->GetPartyTable();
        CItemRepository* pRepo = pPartyTable->GetPartyItems();
        __builtin_memset(m_abAvailable, 0, sizeof(m_abAvailable));

        CSWSCreature* pCreature = pServer->GetCreatureByGameObjectID(
            static_cast<CSWSClient*>(pServer->GetPlayerList()->m_pInternal->m_pHead->m_pData)
                ->AsSWSPlayer()->m_gameObjectId);
        GetUpgradableItems(pCreature, m_abAvailable, pUpgradeC2DA);
        for (int i = 0; i < 9; i++)
        {
            if (pPartyTable->GetIsNPCAvailable(i) == 0)
                continue;
            if (pPartyTable->GetNPCSelectability(i) == 0)
                continue;
            CSWSCreature* pNpc =
                pServer->GetCreatureByGameObjectID(pPartyTable->GetNPCObject(i, 0, 1));
            if (pNpc == NULL)
                continue;
            GetUpgradableItems(pNpc, m_abAvailable, pUpgradeC2DA);
        }
        for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++)
        {
            CSWSItem* pRepoItem = pRepo->ItemListGetItem(i);
            if (pRepoItem == NULL)
                continue;
            unsigned char nType = pRepoItem->GetUpgradableType(pUpgradeC2DA);
            if (nType == 0)
                continue;
            m_abAvailable[nType - 1] = 1;
        }
        for (int i = 0; i < 4; i++)
        {
            m_alblType[i].m_tail.bVisible = m_abAvailable[i];
            if (m_abAvailable[i])
            {
                m_abtnType[i].m_text.m_params.SetColor(CGuiInGame::COLOR_BLUE);
                m_abtnType[i].SetEnabled(1);
            }
            else
            {
                m_abtnType[i].m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
                m_abtnType[i].SetEnabled(0);
            }
        }
        SetActiveControl(&m_abtnType[0], 0);
    }

    delete pUpgradeC2DA;
    CSWGuiPanel::OnPanelAdded();
}

void CSWGuiUpgradeSelection::HandleInputEvent(int type, int val)
{
    if (val != 0 && (type == INPUTEVENT_GUI_BACK || type == INPUTEVENT_GUI_B))
    {
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideUpgradeScreen();
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

void CSWGuiUpgradeSelection::AutoUpgradeDone()
{
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideUpgradeScreen();
}
#else
CSWGuiUpgradeSelection::CSWGuiUpgradeSelection(CSWGuiManager* pGuiManager,
                                               unsigned long long creatureObjId)
    : CSWGuiPanel(pGuiManager)
{
    StartLoadFromLayout(CResRef("upgradesel_p"), 1, 1, 0);

    InitControl(m_lblTitle,        CExoString("LBL_TITLE"),        1, 1, 0, 0, 0);
    InitControl(m_lblTitle2,       CExoString("LBL_TITLE2"),       1, 1, 0, 0, 0);
    InitControl(m_abtnFilter[0],   CExoString("BTN_ALL"),          1, 1, 0, 0, 0);
    InitControl(m_abtnFilter[1],   CExoString("BTN_LIGHTSABER"),   1, 1, 0, 0, 0);
    InitControl(m_abtnFilter[2],   CExoString("BTN_RANGED"),       1, 1, 0, 0, 0);
    InitControl(m_abtnFilter[3],   CExoString("BTN_MELEE"),        1, 1, 0, 0, 0);
    InitControl(m_abtnFilter[4],   CExoString("BTN_ARMOR"),        1, 1, 0, 0, 0);
    InitControl(m_lbUpgradeList,   CExoString("LB_UPGRADELIST"),   1, 1, 0, 0, 0);
    InitControl(m_lbDescription,   CExoString("LB_DESCRIPTION"),   1, 1, 0, 0, 0);
    InitControl(m_btnBack,         CExoString("BTN_BACK"),         1, 1, 0, 0, 0);
    InitControl(m_btnUpgradeItems, CExoString("BTN_UPGRADEITEMS"), 1, 1, 0, 0, 0);
    InitControl(m_btnCreateItems,  CExoString("BTN_CREATEITEMS"),  1, 1, 0, 0, 0);

    StopLoadFromLayout();

    m_lbUpgradeList.bSelectOnLeftClick = 1;

// The description row is a clone of the list box's prototype item.

    CSWGuiControl* pProtoItem = m_lbDescription.m_pProtoItem;
    m_lblDescription.Initialize(pProtoItem->m_extent, static_cast<CSWGuiLabel*>(pProtoItem), 1.0f);

    m_creatureObjId = creatureObjId;
    m_itemObjId = OBJECT_INVALID;  // OBJECT_INVALID

    m_pUpgradeItemSelect = new CSWGuiUpgradeItemSelect(pGuiManager);
    m_pUpgradeItemSelect->m_pUpgradeSelection = this;

    m_pCreateItem = new CSWGuiCreateItem(pGuiManager, creatureObjId);
    m_pCreateItem->m_pUpgradeSelection = this;

    for (int i = 0; i < 5; i++)
    {
        m_abtnFilter[i].m_tail.m_nUserData = i;
        m_abtnFilter[i].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeSelection::OnFilterPicked));
        m_abtnFilter[i].m_tail.bFocusOnHover = 0;
    }
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnUpgradeItems.m_tail.bFocusOnHover = 0;
    m_btnUpgradeItems.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeSelection::OnItemSelected));
    m_btnCreateItems.m_tail.bFocusOnHover = 0;
    m_btnCreateItems.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeSelection::OnCreateItem));

    m_nReserved1 = 0;
    m_nFilter = 0;
    UpdateFilterButtons();
    m_bParam3 = 0;
    m_bParam4 = 0;

    AspyrAddButtonIcon(&m_btnBack, 'b');
    AspyrAddButtonIcon(&m_btnUpgradeItems, 'a');

    SetActiveControl(&m_lbUpgradeList, 0);
}

#if !KOTOR1  // K2-only
// Rebuilds the upgrade list from every item the party can upgrade.

void CSWGuiUpgradeSelection::PopulateItemList()
{
    CExoArrayList<CSWGuiControl*> aControls;

    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    CSWPartyTable* pPartyTable = pServer->GetPartyTable();
    CItemRepository* pRepo = pPartyTable->GetPartyItems();

    CSWGuiButton* pProto = static_cast<CSWGuiButton*>(m_lbUpgradeList.m_pProtoItem);
    int nWide = m_lbUpgradeList.m_innerExtent.w - 2 * static_cast<int>(m_lbUpgradeList.m_nPadding);

    C2DA* pUpgradeC2DA = new C2DA(CResRef("upgrade"), 0);
    pUpgradeC2DA->Load2DArray();

    SetDescription(CExoString(""));

    CSWSCreature* pCreature = pServer->GetCreatureByGameObjectID(
        static_cast<CSWSClient*>(pServer->GetPlayerList()->m_pInternal->m_pHead->m_pData)
            ->AsSWSPlayer()->m_gameObjectId);

    if (pCreature->m_bPlayerMadeCharacter != 0)
    {
        GetPlayerItems(pCreature, aControls, pUpgradeC2DA);
        for (int i = 0; i < 12; i++)
        {
            CSWSCreature* pNpc;
            if (pPartyTable->GetIsNPCAvailable(i) == 0)
                continue;
            pNpc = pServer->GetCreatureByGameObjectID(pPartyTable->GetNPCObject(i, 0, 0));
            if (pNpc == NULL)
                continue;
            GetPlayerItems(pNpc, aControls, pUpgradeC2DA);
        }
    }
    else
    {
        for (int i = 0; i < 3; i++)
        {
            CSWCCreature* pPartyCreature =
                g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(i);
            if (pPartyCreature == NULL)
                continue;
            CSWSCreature* pServerCreature = pPartyCreature->GetServerCreature();
            if (pServerCreature == NULL)
                continue;
            GetPlayerItems(pServerCreature, aControls, pUpgradeC2DA);
        }
    }

    for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++)
    {
        CSWUpgradeItemEntry* pEntry;
        CSWSItem* pItem = pRepo->ItemListGetItem(i);
        if (pItem == NULL)
            continue;
        unsigned char nType = pItem->GetUpgradableType(pUpgradeC2DA);
        if (nType == 0)
            continue;
        if (m_nFilter != 0 && m_nFilter != nType)
            continue;

        pEntry = new CSWUpgradeItemEntry;
        pEntry->Initialize(pProto->m_text, pProto->m_border, pProto->m_borderHilight, nWide, 0);
        pEntry->AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeSelection::OnItemEntered));
        pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeSelection::OnItemSelected));
        pEntry->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeSelection::OnItemSelected));
        pEntry->SetItem(pItem->m_objectId, 0, OBJECT_INVALID, 0);
        aControls.Add(pEntry);
    }

    delete pUpgradeC2DA;

    m_lbUpgradeList.AddControls(aControls, 1, 1, 0);

// With nothing upgradeable, the upgrade button goes dark.

    if (m_lbUpgradeList.m_aControls.m_count > 0)
    {
        m_lbUpgradeList.SetSelectedControl(m_lbUpgradeList.GetControl(0), 0);
        m_btnUpgradeItems.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_UPGRADE_BORDER);
        m_btnUpgradeItems.m_text.m_params.SetColor(CGuiInGame::COLOR_UPGRADE_TEXT);
        m_btnUpgradeItems.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_UPGRADE_TEXT);
        m_btnUpgradeItems.SetEnabled(1);
    }
    else
    {
        m_btnUpgradeItems.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::COLOR_DARK_TAN);
        m_btnUpgradeItems.m_text.m_params.SetColor(CGuiInGame::COLOR_DARK_TAN);
        m_btnUpgradeItems.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_DARK_TAN);
        m_btnUpgradeItems.SetEnabled(0);
    }
}
#endif

// Adds one creature's equipped, upgradable items to the upgrade list.

void CSWGuiUpgradeSelection::GetPlayerItems(CSWSCreature* pCreature,
                                            CExoArrayList<CSWGuiControl*>& aControls,
                                            C2DA* pUpgradeC2DA)
{
    CSWGuiButton* pProto = static_cast<CSWGuiButton*>(m_lbUpgradeList.m_pProtoItem);
    int nWide = m_lbUpgradeList.m_innerExtent.w - 2 * static_cast<int>(m_lbUpgradeList.m_nPadding);

    for (int i = 0; i < 20; i++)
    {
// The four creature-hide slots must not be listed twice.

        if (i >= 14 && i < 18)
            continue;

        CSWSItem* pItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(1 << i));
        if (pItem == NULL)
            continue;
        unsigned char nType = pItem->GetUpgradableType(pUpgradeC2DA);
        if (nType == 0)
            continue;
        if (m_nFilter != 0 && m_nFilter != nType)
            continue;

        CSWUpgradeItemEntry* pEntry = new CSWUpgradeItemEntry;
        pEntry->Initialize(pProto->m_text, pProto->m_border, pProto->m_borderHilight, nWide, 0);
        pEntry->AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeSelection::OnItemEntered));
        pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeSelection::OnItemSelected));
        pEntry->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeSelection::OnItemSelected));
        pEntry->SetItem(pItem->m_objectId, 1, pCreature->m_objectId, 1 << i);
        aControls.Add(pEntry);
    }
}

// Deletes the owned item-select sub-panel; m_pCreateItem belongs to the GUI manager.

CSWGuiUpgradeSelection::~CSWGuiUpgradeSelection()
{
    m_lbDescription.ClearItems();
    m_lbUpgradeList.ClearItems();
    if (m_pUpgradeItemSelect != NULL)
    {
        delete m_pUpgradeItemSelect;
        m_pUpgradeItemSelect = NULL;
    }
}

// Re-colours the five filter captions; the selected one is highlighted.

void CSWGuiUpgradeSelection::UpdateFilterButtons()
{
    for (int i = 0; i < 5; i++)
    {
        m_abtnFilter[i].m_text.m_params.SetDefaultColor(
            m_nFilter == i ? CGuiInGame::COLOR_NEW_HIGHLIGHT : CGuiInGame::COLOR_UPGRADE_TEXT);
    }
}

void CSWGuiUpgradeSelection::OnCreateItem(CSWGuiControl* pControl)
{
    K2_UNUSED(pControl);
    m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
    m_pGuiManager->AddPanel(m_pCreateItem, 3, 1);
}

// On a filter click, the button's user data is the new filter index.

void CSWGuiUpgradeSelection::OnFilterPicked(CSWGuiControl* pControl)
{
    m_nFilter = static_cast<int>(pControl->AsButton()->m_tail.m_nUserData);
    UpdateFilterButtons();
    PopulateItemList();
}

// Handles the upgrade-items button and the list rows, opening the item-select panel for the choice.

void CSWGuiUpgradeSelection::OnItemSelected(CSWGuiControl* pControl)
{
    if (pControl == static_cast<CSWGuiControl*>(&m_btnUpgradeItems))
        pControl = m_lbUpgradeList.GetSelectedControl();

    if (pControl != NULL)
    {
        C2DA* pUpgradeC2DA = new C2DA(CResRef("upgrade"), 0);
        pUpgradeC2DA->Load2DArray();

        CSWSItem* pItem;
        if (static_cast<CSWUpgradeItemEntry*>(pControl)->m_oidItem != OBJECT_INVALID &&
            (pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(static_cast<CSWUpgradeItemEntry*>(pControl)->m_oidItem)) != NULL)
        {
            m_pUpgradeItemSelect->m_nUpgradableType = static_cast<unsigned char>(pItem->GetUpgradableType(pUpgradeC2DA));
            m_pUpgradeItemSelect->m_pItem = pItem;
            m_pUpgradeItemSelect->m_bOpenedForItem = 0;
            m_pGuiManager->AddPanel(m_pUpgradeItemSelect, 3, 1);
        }
    }
}

// Called when the screen becomes visible. Seeds the item-creation sheet's caption and lists the
// upgradeable items.

void CSWGuiUpgradeSelection::OnPanelAdded()
{
    C2DA* pUpgradeC2DA = new C2DA(CResRef("upgrade"), 0);
    pUpgradeC2DA->Load2DArray();

    CSWGuiCreateItem* pCreateItem = m_pCreateItem;
    pCreateItem->m_sOverride2DA = CExoString(m_description);

    if (m_bParam4 != 0)
        m_pGuiManager->AddPanel(m_pCreateItem, 3, 1);

    CSWSItem* pItem = NULL;
    if (m_itemObjId != OBJECT_INVALID &&
        (pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(m_itemObjId)) != NULL)
    {
        m_pUpgradeItemSelect->m_nUpgradableType = static_cast<unsigned char>(pItem->GetUpgradableType(pUpgradeC2DA));
        m_pUpgradeItemSelect->m_pItem = pItem;
        m_pUpgradeItemSelect->m_bOpenedForItem = 1;
        m_pGuiManager->AddPanel(m_pUpgradeItemSelect, 3, 1);
    }
    else
    {
        PopulateItemList();
    }

    delete pUpgradeC2DA;

    if (m_bParam3 != 0)
        m_btnCreateItems.m_tail.bVisible = 0;

    CSWGuiPanel::OnPanelAdded();
}

// Re-lays the single description row inside LB_DESCRIPTION.

void CSWGuiUpgradeSelection::SetDescription(CExoString sDescription)
{
    CSWGuiControl* pRow;

    m_lblDescription.m_text.m_params.SetText(sDescription);
    m_lbDescription.ClearItems();

    CSWGuiText* pText = &m_lblDescription.m_text;

    CSWGuiExtent extent = m_lblDescription.m_extent;
    if (sDescription.GetLength() >= 1)
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
    m_lblDescription.SetExtent(extent);

    pRow = &m_lblDescription;
    m_lbDescription.AddControls(&pRow, 1, 0, 1, 0);
    m_lbDescription.SetSelectedControl(&m_lblDescription, 0);
}

// Handles keys. The close keys play a sound unless the screen is already dismissing, and the
// shoulder keys step the live filter.

void CSWGuiUpgradeSelection::HandleInputEvent(int type, int val)
{
    if (val != 0)
    {
        if (type == INPUTEVENT_GUI_BACK || type == INPUTEVENT_GUI_B)
        {
            if (m_bParam4 == 0)
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            g_pAppManager->m_pClientExoApp->GetInGameGui()->HideUpgradeScreen();
        }
        else if (type == INPUTEVENT_GUI_DPAD_LEFT || type == INPUTEVENT_GUI_DPAD_RIGHT)
        {
            m_nFilter = (m_nFilter + (type == INPUTEVENT_GUI_DPAD_LEFT ? 4 : 6)) % 5;
            UpdateFilterButtons();
            PopulateItemList();
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}

// On row hover, shows the item's property description, or the fallback GUI string.

void CSWGuiUpgradeSelection::OnItemEntered(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        if (static_cast<CSWUpgradeItemEntry*>(pControl)->m_oidItem != OBJECT_INVALID)
        {
            CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
                                  static_cast<CSWUpgradeItemEntry*>(pControl)->m_oidItem);
            CExoString sDescription;
            sDescription = pItem->GetPropertyDescription();
            if (sDescription != "")
                SetDescription(CExoString(sDescription));
            else
                SetDescription(g_pAppManager->m_pClientExoApp->GetGUIString(32172));
        }
        else
        {
            SetDescription(CExoString(""));
        }
    }
}

// The auto-upgrade flow finishing dismisses the screen.

void CSWGuiUpgradeSelection::AutoUpgradeDone()
{
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideUpgradeScreen();
}
#endif

#if KOTOR1  // K1: layout "upgradeitems", the item list inset by 11 scaled units
CSWGuiUpgradeItemSelect::CSWGuiUpgradeItemSelect(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    StartLoadFromLayout(CResRef("upgradeitems"));

    InitControl(m_lblTitle,       CExoString("LBL_TITLE"),       1);
    InitControl(m_lbItems,        CExoString("LB_ITEMS"),        1);
    InitControl(m_lbDescription,  CExoString("LB_DESCRIPTION"),  1);
    m_lbDescription.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_btnUpgradeItem, CExoString("BTN_UPGRADEITEM"), 1);
    InitControl(m_btnBack,        CExoString("BTN_BACK"),        1);

    StopLoadFromLayout();
    ScaleExtentForResolution();

    {
#ifdef K2_QOL_FULLSCREEN   // desktop: the window can change size (swguioptions.cpp K1QolApplyDisplay)
        float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#else
        static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
        CSWGuiExtent extent = m_lbItems.m_extent;
        extent.x = static_cast<int>(s_fScale * 11.0f + static_cast<float>(extent.x));
        extent.w = static_cast<int>(static_cast<float>(extent.w) - s_fScale * 11.0f);
        m_lbItems.SetExtent(extent);
    }
    {
        CSWGuiLabel* pProto = static_cast<CSWGuiLabel*>(m_lbDescription.m_pProtoItem);
        m_lblDescription.Initialize(pProto->m_extent, pProto, 1.0f);
    }

    m_pUpgrade = new CSWGuiUpgrade(pGuiManager);
    m_pUpgrade->m_pItemSelect = this;

    m_btnUpgradeItem.m_tail.bFocusOnHover = 0;
    m_btnUpgradeItem.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeItemSelect::OnUpgradableSelected));
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));

    m_lbItems.bSelectOnLeftClick = 1;
    m_nUpgradableType = CSWSItem::UPGRADABLE_TYPE_NONE;
    m_pItem = NULL;
    SetActiveControl(&m_lbItems, 0);
    IosAddButtonIcon(&m_btnUpgradeItem, 'a');
    IosAddButtonIcon(&m_btnBack, 'b');
}
#else
CSWGuiUpgradeItemSelect::CSWGuiUpgradeItemSelect(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
{
    StartLoadFromLayout(CResRef("upgradeitems_p"), 1, 1, 0);

    InitControl(m_lblTitle,       CExoString("LBL_TITLE"),       1, 1, 0, 0, 0);
    InitControl(m_lbItems,        CExoString("LB_ITEMS"),        1, 1, 0, 0, 0);
    InitControl(m_lbDescription,  CExoString("LB_DESCRIPTION"),  1, 1, 0, 0, 0);
    m_lbDescription.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_btnUpgradeItem, CExoString("BTN_UPGRADEITEM"), 1, 1, 0, 0, 0);
    InitControl(m_btnBack,        CExoString("BTN_BACK"),        1, 1, 0, 0, 0);

    StopLoadFromLayout();

    {
        CSWGuiLabel* pProto = static_cast<CSWGuiLabel*>(m_lbDescription.m_pProtoItem);
        m_lblDescription.Initialize(pProto->m_extent, pProto, 1.0f);
    }

    m_pUpgrade = new CSWGuiUpgrade(pGuiManager);
    m_pUpgrade->m_pItemSelect = this;

    m_btnUpgradeItem.m_tail.bFocusOnHover = 0;
    m_btnUpgradeItem.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeItemSelect::OnUpgradableSelected));
    m_btnBack.m_tail.bFocusOnHover = 0;
    m_btnBack.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));

    m_lbItems.bSelectOnLeftClick = 1;
    m_nUpgradableType = CSWSItem::UPGRADABLE_TYPE_NONE;
    m_pItem = NULL;
    m_bOpenedForItem = 0;
    SetActiveControl(&m_lbItems, 0);
// Starts the unequip record clear: bit 0 "was unequipped", bit 2 "high slot pair".

    m_nUnequipFlags &= ~5u;
    m_otherItemObjId = OBJECT_INVALID;
}
#endif

// Deletes the owned upgrade sub-panel; m_pItem is server-side and not owned.

CSWGuiUpgradeItemSelect::~CSWGuiUpgradeItemSelect()
{
    m_lbItems.ClearItems();
    m_lbDescription.ClearItems();
    if (m_pUpgrade != NULL)
    {
        delete m_pUpgrade;
        m_pUpgrade = NULL;
    }
}

// Re-lays the single description row inside LB_DESCRIPTION.

void CSWGuiUpgradeItemSelect::SetDescription(const CExoString& sDescription)
{
    CSWGuiControl* pRow;

    m_lblDescription.m_text.m_params.SetText(sDescription);
    m_lbDescription.ClearItems();

    CSWGuiText* pText = &m_lblDescription.m_text;

    CSWGuiExtent extent = m_lblDescription.m_extent;
    if (sDescription.GetLength() >= 1)
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
    m_lblDescription.SetExtent(extent);

    pRow = &m_lblDescription;
    m_lbDescription.AddControls(&pRow, 1, 0, 0, 0);
    m_lbDescription.SetSelectedControl(&m_lblDescription, 0);
}

// On row hover, shows the item's property description, or the empty string.

void CSWGuiUpgradeItemSelect::OnControlEntered(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        if (static_cast<CSWUpgradeItemEntry*>(pControl)->m_oidItem != OBJECT_INVALID)
        {
            CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
                                  static_cast<CSWUpgradeItemEntry*>(pControl)->m_oidItem);
            if (pItem != NULL)
                SetDescription(pItem->GetPropertyDescription());
            else
                SetDescription(CExoString(""));
        }
        else
        {
            SetDescription(CExoString(""));
        }
    }
}

// The close keys dismiss the panel; the scroll keys go to LB_DESCRIPTION.

#if KOTOR1  // K1: the shoulder keys page the description ten rows
void CSWGuiUpgradeItemSelect::HandleInputEvent(int type, int val)
{
    if (val != 0)
    {
        switch (type)
        {
            case INPUTEVENT_GUI_B:
            case INPUTEVENT_GUI_BACK:
            {
                m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
                unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
                if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
                    nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
                m_stateFlags = nFlags;
                m_pGuiManager->PopModalPanel();
                break;
            }
            case INPUTEVENT_GUI_DPAD_LEFT:
                for (int i = 0; i < 10; i++)
                    m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
                break;
            case INPUTEVENT_GUI_DPAD_RIGHT:
                for (int i = 0; i < 10; i++)
                    m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
                break;
            case INPUTEVENT_GUI_SCROLL_UP:
                m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
                break;
            case INPUTEVENT_GUI_SCROLL_DOWN:
                m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
                break;
            default:
                break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}
#else
void CSWGuiUpgradeItemSelect::HandleInputEvent(int type, int val)
{
    if (val != 0)
    {
        if (type == INPUTEVENT_GUI_B || type == INPUTEVENT_GUI_BACK)
        {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
// Drops the panel out of its opening state.

#if KOTOR1  // K1: the panel state bits sit one higher
            unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
            if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
                nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#else
            unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
            if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
                nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#endif
            m_stateFlags = nFlags;
            m_pGuiManager->PopModalPanel();
        }
        else if (type == INPUTEVENT_GUI_SCROLL_UP)
        {
            m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
        }
        else if (type == INPUTEVENT_GUI_SCROLL_DOWN)
        {
            m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
}
#endif

// On row activation, takes the item, and its slot-pair partner, out of the equipment and hands them
// to the upgrade panel.

void CSWGuiUpgradeItemSelect::OnUpgradableSelected(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg != 0)
    {
        if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_START)
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);

        CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;

        if (pControl == static_cast<CSWGuiControl*>(&m_btnUpgradeItem))
        {
            m_pActive = static_cast<CSWUpgradeItemEntry*>(m_lbItems.GetSelectedControl());
            if (m_pActive == NULL)
                return;
            pControl = m_pActive;
        }
        else
        {
            m_pActive = static_cast<CSWUpgradeItemEntry*>(pControl);
        }

        CSWSItem* pItem = pServer->GetItemByGameObjectID(m_pActive->m_oidItem);

        m_nUnequipFlags &= ~1u;

        if ((m_pActive->m_nFlags & 1) != 0)
        {
            CSWSCreature* pCreature = pServer->GetCreatureByGameObjectID(m_pActive->m_oidEquippedBy);
            unsigned int nSlot = m_pActive->m_nEquippedSlot;
            if (nSlot == CSWInventory::SLOT_RIGHTWEAPON || nSlot == CSWInventory::SLOT_LEFTWEAPON)
            {
                CSWSItem* pSlot10 = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
                CSWSItem* pSlot20 = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
                if (pSlot20 != NULL)
                {
                    if (pSlot10 == pItem)
                    {
                        m_nUnequipFlags = (m_nUnequipFlags & ~3u) | 1u;
                        m_otherItemObjId = pSlot20->m_objectId;
                        pCreature->UnequipItem(pSlot20);
                        pCreature->UnequipItem(pItem);
                    }
                    else if (pSlot20 == pItem)
                    {
                        m_nUnequipFlags |= 3u;
                        m_otherItemObjId = pSlot10->m_objectId;
                        pCreature->UnequipItem(pItem);
                        pCreature->UnequipItem(pSlot10);
                    }
                }
                else
                {
                    pCreature->UnequipItem(pItem);
                }
            }
            else
            {
                pCreature->UnequipItem(pItem);
            }
        }
        else if (pItem->m_stackCount >= 2)
        {
            pItem = pItem->SplitItem(1);
        }
        else
        {
            pServer->GetPartyTable()->GetPartyItems()->RemoveItem(pItem);
        }

#if KOTOR1  // K1 loads the sub-panel pointer first
        CSWGuiUpgrade* pUpgrade = m_pUpgrade;
        pUpgrade->m_nMenuType = m_nUpgradableType;
        pUpgrade->m_pItem = pItem;
        m_pGuiManager->AddPanel(pUpgrade, 3, 1);
#else
        m_pUpgrade->m_nMenuType = m_nUpgradableType;
        m_pUpgrade->m_pItem = pItem;
        m_pGuiManager->AddPanel(m_pUpgrade, 3, 1);
#endif
    }
}

// When the panel is added, rebuilds the list, or jumps to the upgrade panel for the item it was
// opened with.

#if KOTOR1  // K1: an equipped item is simply unequipped, with no weapon-pair bookkeeping
void CSWGuiUpgradeItemSelect::OnPanelAdded()
{
    if (m_nUpgradableType >= CSWSItem::UPGRADABLE_TYPE_LIGHTSABER && m_nUpgradableType <= CSWSItem::UPGRADABLE_TYPE_ARMOR)
    {
        if (m_pItem == NULL)
        {
            BuildUpgradeList();
            m_lbItems.SetSelectedControl(0, 0);
            SetActiveControl(&m_lbItems, 0);
            CSWGuiPanel::OnPanelAdded();
            return;
        }

        CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
        CSWSCreature* pCreature = pServer->GetCreatureByGameObjectID(m_pItem->m_possessorId);
        int nSlot = static_cast<int>(pCreature->m_pInventory->GetSlotFromItem(m_pItem));

        CSWUpgradeItemEntry* pEntry = new CSWUpgradeItemEntry;
        m_pActive = pEntry;

        if (nSlot > 0)
        {
            pEntry->SetItem(m_pItem->m_objectId, 1, pCreature->m_objectId, static_cast<unsigned int>(nSlot));
            pCreature->UnequipItem(m_pItem);
        }
        else if (m_pItem->m_stackCount > 1)
        {
            m_pItem = m_pItem->SplitItem(1);
        }
        else
        {
            pServer->GetPartyTable()->GetPartyItems()->RemoveItem(m_pItem);
        }

        CSWGuiUpgrade* pUpgrade = m_pUpgrade;
        pUpgrade->m_nMenuType = m_nUpgradableType;
        pUpgrade->m_pItem = m_pItem;
        m_pGuiManager->AddPanel(pUpgrade, 3, 1);
        return;
    }

    unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
    if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
        nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
    m_stateFlags = nFlags;
}
#else
void CSWGuiUpgradeItemSelect::OnPanelAdded()
{
    if (m_nUpgradableType >= CSWSItem::UPGRADABLE_TYPE_LIGHTSABER && m_nUpgradableType <= CSWSItem::UPGRADABLE_TYPE_ARMOR)
    {
    if (m_pItem == NULL)
    {
        BuildUpgradeList();
        m_lbItems.SetSelectedControl(0, 0);
        SetActiveControl(&m_lbItems, 0);
        CSWGuiPanel::OnPanelAdded();
        return;
    }

    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    CSWSCreature* pCreature = pServer->GetCreatureByGameObjectID(m_pItem->m_possessorId);
    int nSlot = static_cast<int>(pCreature->m_pInventory->GetSlotFromItem(m_pItem));

    CSWUpgradeItemEntry* pEntry = new CSWUpgradeItemEntry;
    m_pActive = pEntry;

    if (nSlot >= 1)
    {
        pEntry->SetItem(m_pItem->m_objectId, 1, pCreature->m_objectId, static_cast<unsigned int>(nSlot));

        if (m_pActive->m_nEquippedSlot == CSWInventory::SLOT_RIGHTWEAPON2 || m_pActive->m_nEquippedSlot == CSWInventory::SLOT_LEFTWEAPON2)
        {
            CSWSItem* pLow = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON2));
            CSWSItem* pHigh = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON2));
            if (pHigh != NULL)
            {
                if (pLow == m_pItem)
                {
                    m_nUnequipFlags = (m_nUnequipFlags & ~7u) | 5u;
                    m_otherItemObjId = pHigh->m_objectId;
                    pCreature->UnequipItem(pHigh);
                    pCreature->UnequipItem(m_pItem);
                }
                else if (pHigh == m_pItem)
                {
                    m_nUnequipFlags |= 7u;
                    m_otherItemObjId = pLow->m_objectId;
                    pCreature->UnequipItem(m_pItem);
                    pCreature->UnequipItem(pLow);
                }
            }
            else
            {
                m_nUnequipFlags = (m_nUnequipFlags & ~7u) | 4u;
                m_otherItemObjId = OBJECT_INVALID;
                pCreature->UnequipItem(pLow);
            }
        }
        else if (m_pActive->m_nEquippedSlot == CSWInventory::SLOT_RIGHTWEAPON || m_pActive->m_nEquippedSlot == CSWInventory::SLOT_LEFTWEAPON)
        {
            CSWSItem* pSlot10 = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
            CSWSItem* pSlot20 = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
            if (pSlot20 != NULL)
            {
                if (pSlot10 == m_pItem)
                {
                    m_nUnequipFlags = (m_nUnequipFlags & ~7u) | 1u;
                    m_otherItemObjId = pSlot20->m_objectId;
                    pCreature->UnequipItem(pSlot20);
                    pCreature->UnequipItem(m_pItem);
                }
                else if (pSlot20 == m_pItem)
                {
                    m_nUnequipFlags = (m_nUnequipFlags & ~7u) | 3u;
                    m_otherItemObjId = pSlot10->m_objectId;
                    pCreature->UnequipItem(m_pItem);
                    pCreature->UnequipItem(pSlot10);
                }
            }
            else
            {
                m_nUnequipFlags &= ~7u;
                m_otherItemObjId = OBJECT_INVALID;
                pCreature->UnequipItem(pSlot10);
            }
        }
        else
        {
            m_nUnequipFlags &= ~7u;
            pCreature->UnequipItem(m_pItem);
        }
    }
    else if (m_pItem->m_stackCount > 1)
    {
        m_pItem = m_pItem->SplitItem(1);
    }
    else
    {
        pServer->GetPartyTable()->GetPartyItems()->RemoveItem(m_pItem);
    }

    m_pUpgrade->m_nMenuType = m_nUpgradableType;
    m_pUpgrade->m_pItem = m_pItem;
    m_pGuiManager->AddPanel(m_pUpgrade, 3, 1);
        return;
    }

#if KOTOR1  // K1: the panel state bits sit one higher
    unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
    if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
        nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#else
    unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
    if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
        nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#endif
    m_stateFlags = nFlags;
}
#endif

// Rebuilds LB_ITEMS from every item the party can upgrade at the requested type.

void CSWGuiUpgradeItemSelect::BuildUpgradeList()
{
    CExoArrayList<CSWGuiControl*> aControls;

    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    CSWPartyTable* pPartyTable = pServer->GetPartyTable();
    CItemRepository* pRepo = pPartyTable->GetPartyItems();

    CSWGuiButton* pProto = static_cast<CSWGuiButton*>(m_lbItems.m_pProtoItem);
    int nInnerW = m_lbItems.m_innerExtent.w;
    int nPad = m_lbItems.m_nPadding;

    C2DA* pUpgradeC2DA = new C2DA(CResRef("upgrade"), 0);
    pUpgradeC2DA->Load2DArray();

    int nWide = nInnerW - 2 * nPad;

    BuildItemList(pServer->GetCreatureByGameObjectID(
                      static_cast<CSWSClient*>(pServer->GetPlayerList()->m_pInternal->m_pHead->m_pData)
                          ->AsSWSPlayer()->m_gameObjectId),
                  aControls, pUpgradeC2DA);

#if KOTOR1  // K1: nine party slots
    for (int i = 0; i < 9; i++)
#else
    for (int i = 0; i < 12; i++)
#endif
    {
        CSWSCreature* pNpc;
        if (pPartyTable->GetIsNPCAvailable(i) == 0)
            continue;
        if (pPartyTable->GetNPCSelectability(i) == 0)
            continue;
        pNpc = pServer->GetCreatureByGameObjectID(pPartyTable->GetNPCObject(i, 0, 1));
        if (pNpc == NULL)
            continue;
        BuildItemList(pNpc, aControls, pUpgradeC2DA);
    }

    for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++)
    {
        CSWUpgradeItemEntry* pEntry;
        CSWSItem* pItem = pRepo->ItemListGetItem(i);
        if (pItem == NULL)
            continue;
        if (pItem->GetUpgradableType(pUpgradeC2DA) != m_nUpgradableType)
            continue;

        pEntry = new CSWUpgradeItemEntry;
#if KOTOR1
        pEntry->Initialize(pProto->m_text, pProto->m_border, pProto->m_borderHilight, nWide);
#else
        pEntry->Initialize(pProto->m_text, pProto->m_border, pProto->m_borderHilight, nWide, 0);
#endif
        pEntry->AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeItemSelect::OnControlEntered));
        pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeItemSelect::OnUpgradableSelected));
        pEntry->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeItemSelect::OnUpgradableSelected));
        pEntry->SetItem(pItem->m_objectId, 0, OBJECT_INVALID, 0);
        aControls.Add(pEntry);
    }

    delete pUpgradeC2DA;

    m_lbItems.AddControls(aControls, 1, 0, 0);
}

// One creature's contribution to the upgrade list.

void CSWGuiUpgradeItemSelect::BuildItemList(CSWSCreature* pCreature,
                                            CExoArrayList<CSWGuiControl*>& aControls,
                                            C2DA* pUpgradeC2DA)
{
    CSWGuiButton* pProto = static_cast<CSWGuiButton*>(m_lbItems.m_pProtoItem);
    int nWide = m_lbItems.m_innerExtent.w - 2 * static_cast<int>(m_lbItems.m_nPadding);

#if KOTOR1  // K1: eighteen equipment slots
    for (int i = 0; i < 18; i++)
#else
    for (int i = 0; i < 20; i++)
#endif
    {
        CSWSItem* pItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(1 << i));
        if (pItem == NULL)
            continue;
        if (pItem->GetUpgradableType(pUpgradeC2DA) != m_nUpgradableType)
            continue;

        CSWUpgradeItemEntry* pEntry = new CSWUpgradeItemEntry;
#if KOTOR1
        pEntry->Initialize(pProto->m_text, pProto->m_border, pProto->m_borderHilight, nWide);
#else
        pEntry->Initialize(pProto->m_text, pProto->m_border, pProto->m_borderHilight, nWide, 0);
#endif
        pEntry->AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeItemSelect::OnControlEntered));
        pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeItemSelect::OnUpgradableSelected));
        pEntry->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiUpgradeItemSelect::OnUpgradableSelected));
        pEntry->SetItem(pItem->m_objectId, 1, pCreature->m_objectId, 1 << i);
        aControls.Add(pEntry);
    }
}

// Puts the item back where it came from and dismisses the panel, or re-lists the selection.

#if KOTOR1  // K1: no combat-state reset, only the weapon pair
void CSWGuiUpgradeItemSelect::FinishUpgrading()
{
    CSWSItem* pItem = m_pUpgrade->m_pItem;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;

    if ((m_pActive->m_nFlags & 1) == 0)
    {
        pServer->GetPartyTable()->GetPartyItems()->AddItem(&pItem, 1, 0, 0);
    }
    else
    {
        unsigned int nSlot = m_pActive->m_nEquippedSlot;
        CSWSCreature* pCreature = pServer->GetCreatureByGameObjectID(m_pActive->m_oidEquippedBy);

        int bEquipped = 1;
        if (!pCreature->CanEquipItem(pItem, &nSlot, 0, 0, 0))
        {
            pServer->GetPartyTable()->GetPartyItems()->AddItem(&pItem, 1, 0, 0);
            m_pActive->SetItem(pItem->m_objectId, 0, OBJECT_INVALID, 0);
            bEquipped = 0;
        }

        if ((m_nUnequipFlags & UNEQUIP_FLAG_UNEQUIPPED) == 0)
        {
            if (bEquipped)
                pCreature->EquipItem(nSlot, pItem, 1, 0, 0);
        }
        else if ((m_nUnequipFlags & UNEQUIP_FLAG_SECOND_SLOT) == 0)
        {
            if (bEquipped)
            {
                pCreature->EquipItem(CSWInventory::SLOT_RIGHTWEAPON, pItem, 1, 0, 0);
                pCreature->EquipItem(CSWInventory::SLOT_LEFTWEAPON, pServer->GetItemByGameObjectID(m_otherItemObjId), 1, 0, 0);
            }
            else
            {
                pCreature->EquipItem(CSWInventory::SLOT_RIGHTWEAPON, pServer->GetItemByGameObjectID(m_otherItemObjId), 1, 0, 0);
            }
        }
        else
        {
            pCreature->EquipItem(CSWInventory::SLOT_RIGHTWEAPON, pServer->GetItemByGameObjectID(m_otherItemObjId), 1, 0, 0);
            if (bEquipped)
                pCreature->EquipItem(CSWInventory::SLOT_LEFTWEAPON, pItem, 1, 0, 0);
        }
    }

    if (m_pItem == NULL)
    {
        if (m_pActive->m_oidItem != pItem->m_objectId)
            BuildUpgradeList();

        int i = 0;
        while (m_lbItems.m_aControls.m_count != 0)
        {
            CSWGuiControl* pControl = m_lbItems.GetControl(i);
            if (static_cast<CSWUpgradeItemEntry*>(pControl)->m_oidItem == pItem->m_objectId)
            {
                m_pActive = static_cast<CSWUpgradeItemEntry*>(pControl);
                break;
            }
            i++;
        }

        m_lbItems.SetSelectedControl(-1, 0);
        m_lbItems.SetSelectedControl(m_pActive, 0);
        return;
    }

    unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
    if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
        nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
    m_stateFlags = nFlags;
    m_pGuiManager->PopModalPanel();
    g_pAppManager->m_pClientExoApp->GetInGameGui()->HideUpgradeScreen();
    m_pItem = NULL;
    if (m_pActive != NULL)
        delete m_pActive;
}
#else
void CSWGuiUpgradeItemSelect::FinishUpgrading()
{
    CSWSItem* pItem = m_pUpgrade->m_pItem;
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;

    if ((m_pActive->m_nFlags & 1) == 0)
    {
        pServer->GetPartyTable()->GetPartyItems()->AddItem(&pItem, 1, 0, 0);
    }
    else
    {
        unsigned int nSlot = m_pActive->m_nEquippedSlot;
        CSWSCreature* pCreature = pServer->GetCreatureByGameObjectID(m_pActive->m_oidEquippedBy);
#if KOTOR1
        pCreature->SetCombatState(1, 2);
#else
        pCreature->SetCombatState(1, 2, 0);
#endif

        int bEquipped = 1;
        if (!pCreature->CanEquipItem(pItem, &nSlot, 0, 0, 0))
        {
            pServer->GetPartyTable()->GetPartyItems()->AddItem(&pItem, 1, 0, 0);
            m_pActive->SetItem(pItem->m_objectId, 0, OBJECT_INVALID, 0);
            bEquipped = 0;
        }

        if ((m_nUnequipFlags & UNEQUIP_FLAG_UNEQUIPPED) == 0)
        {
            if (bEquipped)
                pCreature->EquipItem(nSlot, pItem, 1, 0, 0);
        }
        else
        {
// The pair code is 0 low pair first half, 1 low pair second half, 2 high pair first half, 3 both.

            switch (m_nUnequipFlags & (UNEQUIP_FLAG_SECOND_SLOT | UNEQUIP_FLAG_HIGH_PAIR))
            {
                case 0:
                    if (bEquipped)
                    {
                        pCreature->EquipItem(CSWInventory::SLOT_RIGHTWEAPON, pItem, 1, 0, 0);
                        pCreature->EquipItem(CSWInventory::SLOT_LEFTWEAPON, pServer->GetItemByGameObjectID(m_otherItemObjId), 1, 0, 0);
                    }
                    else
                    {
                        pCreature->EquipItem(CSWInventory::SLOT_RIGHTWEAPON, pServer->GetItemByGameObjectID(m_otherItemObjId), 1, 0, 0);
                    }
                    break;
                case UNEQUIP_FLAG_SECOND_SLOT:
                    pCreature->EquipItem(CSWInventory::SLOT_RIGHTWEAPON, pServer->GetItemByGameObjectID(m_otherItemObjId), 1, 0, 0);
                    if (bEquipped)
                        pCreature->EquipItem(CSWInventory::SLOT_LEFTWEAPON, pItem, 1, 0, 0);
                    break;
                case UNEQUIP_FLAG_HIGH_PAIR:
                    if (bEquipped)
                    {
                        pCreature->EquipItem(CSWInventory::SLOT_RIGHTWEAPON2, pItem, 1, 0, 0);
                        pCreature->EquipItem(CSWInventory::SLOT_LEFTWEAPON2, pServer->GetItemByGameObjectID(m_otherItemObjId), 1, 0, 0);
                    }
                    else
                    {
                        pCreature->EquipItem(CSWInventory::SLOT_RIGHTWEAPON2, pServer->GetItemByGameObjectID(m_otherItemObjId), 1, 0, 0);
                    }
                    break;
                case UNEQUIP_FLAG_HIGH_PAIR | UNEQUIP_FLAG_SECOND_SLOT:
                    pCreature->EquipItem(CSWInventory::SLOT_RIGHTWEAPON2, pServer->GetItemByGameObjectID(m_otherItemObjId), 1, 0, 0);
                    if (bEquipped)
                        pCreature->EquipItem(CSWInventory::SLOT_LEFTWEAPON2, pItem, 1, 0, 0);
                    break;
            }
        }
    }

// With no live item the panel re-lists and re-selects, whether the item came from the inventory
// or a slot.

    if (m_pItem == NULL)
    {
        if (m_pActive->m_oidItem != pItem->m_objectId)
            BuildUpgradeList();

        int i = 0;
        while (m_lbItems.m_aControls.m_count != 0)
        {
            CSWGuiControl* pControl = m_lbItems.GetControl(i);
            if (static_cast<CSWUpgradeItemEntry*>(pControl)->m_oidItem == pItem->m_objectId)
            {
                m_pActive = static_cast<CSWUpgradeItemEntry*>(pControl);
                break;
            }
            i++;
        }

        m_lbItems.SetSelectedControl(-1, 0);
        m_lbItems.SetSelectedControl(m_pActive, 0);
        return;
    }

#if KOTOR1  // K1: the panel state bits sit one higher
    unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
    if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
        nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#else
    unsigned short nFlags = m_stateFlags & ~PANEL_REMOVE_DEFERRED;
    if ((nFlags & PANEL_REMOVE_MASK) != PANEL_REMOVE_AND_DELETE)
        nFlags = (m_stateFlags & ~PANEL_REMOVE_STATE_MASK) | PANEL_REMOVE;
#endif
    m_stateFlags = nFlags;

    int nHadItem = m_bOpenedForItem;
    m_pGuiManager->PopModalPanel();

    if (nHadItem != 0)
    {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideUpgradeScreen();
        m_pItem = NULL;
    }
    else
    {
        m_pItem = NULL;
        m_pUpgradeSelection->PopulateItemList();
    }

    if (m_pActive != NULL)
        delete m_pActive;
}
#endif
