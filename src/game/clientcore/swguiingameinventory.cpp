// The inventory panel lists equipped items, usable items and quest items, with seven filter tabs.

#include "k2stdint.h"
#include "swguiingameinventory.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "swgui.h"
#include "swguicontrol.h"
#include "swparty.h"
#include "nwcitem.h"
#include "swguidrawable.h"
#include "swguiingame.h"
#include "nwscreature.h"
#include "nwitem.h"
#include "nwrace.h"
#include "nwccreature.h"
#include "nwccreaturestats.h"
#include "serverexoapp.h"
#include "nwsitem.h"
#include "partytables.h"
#include "exostring.h"
#include "exoresman.h"
#include "Vector.h"
#include "nwtlktable.h"
#if KOTOR1
#include "serverinfo.h"
#include "exotlktable.h"
#include "port/k1gui.h"
#include "aurgui.h"
bool IosIsIphone(int& nScreenWidth, int& nScreenHeight);
extern CTlkTable* g_pTlkTable;
#endif

#define CEXO_ARRAYLIST_KEEP_ADDUNIQUE_OUT_OF_LINE
#include "CExoArrayListImpl.h"


#include "port/libcstring.h"
#include "port/k2qolapi.h"
#include "port/asllanguage.h"
#include "nwscriptconst.h"
#include "exoinput.h"
#include "itemtype.h"
#include "tutorial.h"
#include "strref.h"

// CSWCCreature::IsLeaderUsableItem's check mask: every check.
enum {
    USABLE_CHECK_ALL = 0xff,
};

// The worn slots that are not weapon slots.
enum {
    SLOTS_WORN_NOT_WEAPON = CSWInventory::SLOT_HEAD | CSWInventory::SLOT_BODY | CSWInventory::SLOT_HANDS |
                            CSWInventory::SLOT_LEFTARM | CSWInventory::SLOT_RIGHTARM |
                            CSWInventory::SLOT_IMPLANT | CSWInventory::SLOT_BELT,
};

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);

#if KOTOR1

const uint64_t CSWGuiInGameInventory::m_nFilterName[6] = {
    0xa35e, 0xa4b5, 0xa35a, 0xa35d, 0xa35b, 0xa35c,
};

CSWGuiInGameInventory::CSWGuiInGameInventory(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
    , m_lists()
{
    m_bRebuildItemList = 0;
    m_bRestoreListPosition = 0;
    m_oidSelected = OBJECT_INVALID;
    m_nNPC = -1;

    StartLoadFromLayout(CResRef("inventory"));

    InitControl(m_lbDescription,     CExoString("LB_DESCRIPTION"),    1);
    InitControl(m_lbItems,           CExoString("LB_ITEMS"),          1);
    InitControl(m_labelInv,          CExoString("LBL_INV"),           1);
    InitControl(m_labelCredits,      CExoString("LBL_CREDITS"),       1);
    InitControl(m_labelCreditsValue, CExoString("LBL_CREDITS_VALUE"), 1);
    InitControl(m_labelVit,          CExoString("LBL_VIT"),           1);
    InitControl(m_labelDef,          CExoString("LBL_DEF"),           1);
    InitControl(m_labelBgStats,      CExoString("LBL_BGSTATS"),       1);
    InitControl(m_labelBgPort,       CExoString("LBL_BGPORT"),        1);
    InitControl(m_labelPort,         CExoString("LBL_PORT"),          1);
    InitControl(m_btnExit,           CExoString("BTN_EXIT"),          1);
    InitControl(m_btnUseItem,        CExoString("BTN_USEITEM"),       1);
    InitControl(m_btnQuestItems,     CExoString("BTN_QUESTITEMS"),    1);
    InitControl(m_btnChange1,        CExoString("BTN_CHANGE1"),       1);
    InitControl(m_btnChange2,        CExoString("BTN_CHANGE2"),       1);
    StopLoadFromLayout();
    ScaleExtentForResolution();
#if !defined(K2_UI_TOUCH)
    // pc / xbox: the PC layout centres the party portraits at their own size, a corner of the
    // scaled button; stretched, as the equip / character / abilities layouts draw them.
    m_btnChange1.m_border.m_params.m_flags.nFillStyle = 2;
    m_btnChange1.m_borderHilight.m_params.m_flags.nFillStyle = 2;
    m_btnChange2.m_border.m_params.m_flags.nFillStyle = 2;
    m_btnChange2.m_borderHilight.m_params.m_flags.nFillStyle = 2;
#endif

    m_btnCharLeft.m_tail.bFocusOnHover = 0;
    m_btnCharLeft.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                           &CSWGuiInGameInventory::OnSwitchLeft));
    m_btnCharRight.m_tail.bFocusOnHover = 0;
    m_btnCharRight.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                            &CSWGuiInGameInventory::OnSwitchRight));
    m_btnCharRight.m_border.m_params.m_fRotate = 180.0f;
    m_btnCharRight.m_borderHilight.m_params.m_fRotate = 180.0f;

    CSWGuiLabel* pProtoLabel = static_cast<CSWGuiLabel*>(m_lbDescription.m_pProtoItem);
    m_labelDescription.Initialize(pProtoLabel->m_extent, pProtoLabel, 1.0f);
    m_lbItems.bShowScrollBar = 1;
    SetActiveControl(&m_lbItems, 0);

    unsigned char nFilter = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nInventoryFilter;
    CExoString sText = g_pTlkTable->GetSimpleString(STRREF_SHOW) + CExoString(" ") +
                       g_pTlkTable->GetSimpleString(m_nFilterName[nFilter > 4 ? 0 : nFilter + 1]);
    m_btnQuestItems.m_text.m_params.SetText(sText);
    IosAddButtonIcon(&m_btnQuestItems, 'x');
    sText = g_pTlkTable->GetSimpleString(STRREF_PARTY_INVENTORY) + CExoString(" - ") +
            g_pTlkTable->GetSimpleString(
                m_nFilterName[g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nInventoryFilter]);
    m_labelInv.m_text.m_params.SetText(sText);

    m_btnExit.m_tail.bFocusOnHover = 0;
    m_btnExit.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiPanel::OnBButtonPressed));
    m_btnUseItem.m_tail.bFocusOnHover = 0;
    m_btnUseItem.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                          &CSWGuiPanel::OnAButtonPressed));
    m_btnQuestItems.m_tail.bFocusOnHover = 0;
    m_btnQuestItems.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                             &CSWGuiPanel::OnXButtonPressed));
    m_btnChange1.m_tail.bFocusOnHover = 0;
    m_btnChange1.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                          &CSWGuiInGameInventory::OnChangeCharacter));
    m_btnChange2.m_tail.bFocusOnHover = 0;
    m_btnChange2.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                          &CSWGuiInGameInventory::OnChangeCharacter));
    m_btnChange1.m_nToolTipStrRef = STRREF_CHANGE_CHARACTER;
    m_btnChange1.m_nBoundInputEventId = INPUTEVENT_CHANGE_CHARACTER;
    m_btnChange2.m_nToolTipStrRef = STRREF_CHANGE_CHARACTER;
#ifndef K2_UI_TOUCH
#ifdef K1_UI_XBOX
    m_iconChange.InitAndAddCorner(this, 'r', m_btnChange1.m_extent);   // RB: next party member
#else
    m_iconChange.InitAndAddCorner(this, 'y', m_btnChange1.m_extent);
#endif
#else
    m_iconChange.InitAndAdd(this, 'y', m_btnChange1.m_extent, -30, -30);
#endif
    IosAddButtonIcon(&m_btnExit, 'b');
    IosAddButtonIcon(&m_btnUseItem, 'a');
}
#else
CSWGuiInGameInventory::CSWGuiInGameInventory(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager)
    , m_lists()
{
    CSWGuiButton btnUnused;

    m_bRestoreListPosition = 0;
    m_bRebuildItemList = 0;
    m_oidSelected = OBJECT_INVALID;

    StartLoadFromLayout(CResRef("inventory_p"), 1, 1, 1);

    InitControl(m_lbDescription,     CExoString("LB_DESCRIPTION"),    1, 1, 0, 0, 0);
    InitControl(m_lbItems,           CExoString("LB_ITEMS"),          1, 1, 0, 0, 0);
    InitControl(m_labelInv,          CExoString("LBL_INV"),           1, 1, 0, 0, 0);
    InitControl(m_labelCredits,      CExoString("LBL_CREDITS"),       1, 1, 0, 0, 0);
    InitControl(m_labelCreditsValue, CExoString("LBL_CREDITS_VALUE"), 1, 1, 0, 0, 0);
    InitControl(m_btnExit,           CExoString("BTN_EXIT"),          1, 1, 0, 0, 0);
    InitControl(m_btnUseItem,        CExoString("BTN_USEITEM"),       1, 1, 0, 0, 0);
    AspyrAddButtonIcon(&m_btnUseItem, 'a');
    InitControl(m_aLabelBar[0],      CExoString("LBL_BAR1"),          1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[1],      CExoString("LBL_BAR2"),          1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[2],      CExoString("LBL_BAR3"),          1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[3],      CExoString("LBL_BAR4"),          1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[4],      CExoString("LBL_BAR5"),          1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[5],      CExoString("LBL_BAR6"),          1, 1, 0, 0, 0);
    InitControl(m_labelFilter,       CExoString("LBL_FILTER"),        1, 1, 0, 0, 0);
    InitControl(m_btnAll,            CExoString("BTN_ALL"),           1, 1, 0, 0, 0);
    InitControl(m_btnDatapads,       CExoString("BTN_DATAPADS"),      1, 1, 0, 0, 0);

    float fAspect = static_cast<float>(g_nScreenWidth) / static_cast<float>(g_nScreenHeight);
    if (fAspect > 1.3333f) {
        int nLang = ASLPlat_GetCurrentLanguage();
        switch (nLang) {
        case ASL_LANGUAGE_ITALIAN:
            InitControl(m_btnWeapons, CExoString("BTN_WEAPONS"), 1, 1, 0, -20, -10);
            InitControl(m_btnArmor,   CExoString("BTN_ARMOR"),   1, 1, 0, -25,  15);
            InitControl(m_btnUseable, CExoString("BTN_USEABLE"), 1, 1, 0, -10,   15);
            InitControl(m_btnQuests,  CExoString("BTN_QUESTS"),  1, 1, 0,  0,     5);
            InitControl(m_btnMisc,    CExoString("BTN_MISC"),    1, 1, 0,  0,     0);
            break;
        case ASL_LANGUAGE_GERMAN:
            InitControl(m_btnWeapons, CExoString("BTN_WEAPONS"), 1, 1, 0, -10,  0);
            InitControl(m_btnArmor,   CExoString("BTN_ARMOR"),   1, 1, 0, -20, 15);
            InitControl(m_btnUseable, CExoString("BTN_USEABLE"), 1, 1, 0, -15,  15);
            InitControl(m_btnQuests,  CExoString("BTN_QUESTS"),  1, 1, 0,  0,    15);
            InitControl(m_btnMisc,    CExoString("BTN_MISC"),    1, 1, 0,  0,    0);
            break;
        case ASL_LANGUAGE_SPANISH:
            InitControl(m_btnWeapons, CExoString("BTN_WEAPONS"), 1, 1, 0, 0, 0);
            InitControl(m_btnArmor,   CExoString("BTN_ARMOR"),   1, 1, 0, 0, 0);
            InitControl(m_btnUseable, CExoString("BTN_USEABLE"), 1, 1, 0, 0, 0);
            InitControl(m_btnQuests,  CExoString("BTN_QUESTS"),  1, 1, 0, 0, 0);
            InitControl(m_btnMisc,    CExoString("BTN_MISC"),    1, 1, 0, 0, 5);
            break;
        default:
            InitControl(m_btnWeapons, CExoString("BTN_WEAPONS"), 1, 1, 0, 0, 0);
            InitControl(m_btnArmor,   CExoString("BTN_ARMOR"),   1, 1, 0, 0, 0);
            InitControl(m_btnUseable, CExoString("BTN_USEABLE"), 1, 1, 0, 0, 0);
            InitControl(m_btnQuests,  CExoString("BTN_QUESTS"),  1, 1, 0, 0, 0);
            InitControl(m_btnMisc,    CExoString("BTN_MISC"),    1, 1, 0, 0, 0);
            break;
        }
    } else {
        InitControl(m_btnWeapons, CExoString("BTN_WEAPONS"), 1, 1, 0, 0, 0);
        InitControl(m_btnArmor,   CExoString("BTN_ARMOR"),   1, 1, 0, 0, 0);
        InitControl(m_btnUseable, CExoString("BTN_USEABLE"), 1, 1, 0, 0, 0);
        InitControl(m_btnQuests,  CExoString("BTN_QUESTS"),  1, 1, 0, 0, 0);
        InitControl(m_btnMisc,    CExoString("BTN_MISC"),    1, 1, 0, 0, 0);
    }

    StopLoadFromLayout();

#if !KOTOR1

    CGuiInGame* pInGameGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    int nFilter = pInGameGui->m_nInventoryFilter;
#ifdef K2_FIX_ENGINE_BUGS

    int nMiscFilter = INVENTORY_FILTER_MISC;
    int nQuestsFilter = INVENTORY_FILTER_QUEST_TAB;
    if (m_btnMisc.m_text.m_params.m_nStrRef == STRREF_INVENTORY_QUESTS || m_btnQuests.m_text.m_params.m_nStrRef == STRREF_INVENTORY_MISC) {
        nMiscFilter = INVENTORY_FILTER_QUEST_TAB;
        nQuestsFilter = INVENTORY_FILTER_MISC;
    }
#else
    const int nMiscFilter = INVENTORY_FILTER_QUEST_TAB;
    const int nQuestsFilter = INVENTORY_FILTER_MISC;
#endif

    m_btnAll.m_tail.bFocusOnHover = 0;
    m_btnAll.m_tail.m_nUserData = INVENTORY_FILTER_ALL;
    m_btnAll.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameInventory::SetNextFilter));
    if (nFilter == INVENTORY_FILTER_ALL)
        m_btnAll.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
    else
        m_btnAll.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);

    m_btnDatapads.m_tail.bFocusOnHover = 0;
    m_btnDatapads.m_tail.m_nUserData = INVENTORY_FILTER_DATAPADS;
    m_btnDatapads.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                           &CSWGuiInGameInventory::SetNextFilter));
    if (nFilter == INVENTORY_FILTER_DATAPADS)
        m_btnDatapads.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
    else
        m_btnDatapads.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);

    m_btnWeapons.m_tail.bFocusOnHover = 0;
    m_btnWeapons.m_tail.m_nUserData = INVENTORY_FILTER_WEAPONS;
    m_btnWeapons.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                          &CSWGuiInGameInventory::SetNextFilter));
    if (nFilter == INVENTORY_FILTER_WEAPONS)
        m_btnWeapons.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
    else
        m_btnWeapons.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);

    m_btnArmor.m_tail.bFocusOnHover = 0;
    m_btnArmor.m_tail.m_nUserData = INVENTORY_FILTER_ARMOR;
    m_btnArmor.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                        &CSWGuiInGameInventory::SetNextFilter));
    if (nFilter == INVENTORY_FILTER_ARMOR)
        m_btnArmor.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
    else
        m_btnArmor.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);

    m_btnUseable.m_tail.bFocusOnHover = 0;
    m_btnUseable.m_tail.m_nUserData = INVENTORY_FILTER_USEABLE_TAB;
    m_btnUseable.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                          &CSWGuiInGameInventory::SetNextFilter));
    if (nFilter == INVENTORY_FILTER_USEABLE_TAB)
        m_btnUseable.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
    else
        m_btnUseable.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);

    m_btnMisc.m_tail.bFocusOnHover = 0;
    m_btnMisc.m_tail.m_nUserData = nMiscFilter;
    m_btnMisc.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiInGameInventory::SetNextFilter));
    if (nFilter == nMiscFilter)
        m_btnMisc.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
    else
        m_btnMisc.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);

    m_btnQuests.m_tail.bFocusOnHover = 0;
    m_btnQuests.m_tail.m_nUserData = nQuestsFilter;
    m_btnQuests.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                         &CSWGuiInGameInventory::SetNextFilter));
    if (nFilter == nQuestsFilter)
        m_btnQuests.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
    else
        m_btnQuests.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
#endif

    CSWGuiLabel* pProtoLabel = static_cast<CSWGuiLabel*>(m_lbDescription.m_pProtoItem);
    m_labelDescription.Initialize(pProtoLabel->m_extent, pProtoLabel, 1.0f);
    m_lbItems.bShowScrollBar = 1;
    SetActiveControl(&m_lbItems, 0);

    CGuiInGame* pInGameGui2 = g_pAppManager->m_pClientExoApp->GetInGameGui();
    (void)pInGameGui2;
    CExoString sUnused;
    m_labelInv.m_text.m_params.SetStrRef(STRREF_INVENTORY);

    m_btnExit.m_tail.bFocusOnHover = 0;
    m_btnExit.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                       &CSWGuiPanel::OnBButtonPressed));
    AspyrAddButtonIcon(&m_btnExit, 'b');
    m_btnUseItem.m_tail.bFocusOnHover = 0;
    m_btnUseItem.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                          &CSWGuiPanel::OnAButtonPressed));
}
#endif

#if KOTOR1

void CSWGuiInGameInventory::PopulateItemListBox()
{
    CSWCCreature* pPlayer;
    if (m_nNPC == -1 || !g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bTutorialModule) {
        pPlayer = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    } else {
        CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
        pPlayer = pClient->GetCreatureByGameObjectID(pClient->ServerToClientObjectId(
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(m_nNPC, 0, 1)));
    }
    CSWSCreature* pServerCreature = pPlayer->GetServerCreature();
    CItemRepository* pRepo = pServerCreature->GetItemRepository(1);

    m_labelCreditsValue.m_text.m_params.SetText(CExoString(pServerCreature->GetGold()));

    CExoArrayList<CSWGuiControl*> aControls;
    int nIndex = 0;
    int nSelected = -1;
    CSWCItem* pEquipped;
    CSWSItem* pItem;
    int nFlag;

    for (int i = 0; i < 14; i++) {
        pEquipped = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
            pPlayer->GetEquippedItemID(1 << i));
        if (pEquipped == 0)
            continue;

        pItem = pEquipped->GetServerItem();
        nFlag = (i == 7 || i == 8) | (i == 10);
        if (pItem == 0)
            continue;
        if (pItem->m_bits.m_bDeleting)
            continue;
        if (!CheckFilter(pItem, pServerCreature, nFlag))
            continue;

        if (m_oidSelected == pItem->m_objectId) {
            nSelected = nIndex;
            m_oidSelected = OBJECT_INVALID;
        }
        CreateItemEntry(pPlayer, nIndex, aControls, pItem, nFlag, 1);
    }

    for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++) {
        pItem = pRepo->ItemListGetItem(i);
        if (pItem == 0)
            continue;
        if (pItem->m_bits.m_bDeleting)
            continue;
        if (!CheckFilter(pItem, pServerCreature, 0))
            continue;

        if (m_oidSelected == pItem->m_objectId) {
            nSelected = nIndex;
            m_oidSelected = OBJECT_INVALID;
        }
        CreateItemEntry(pPlayer, nIndex, aControls, pItem, 0, 0);
    }

    m_lbItems.AddControls(aControls, 0, 1, 0);

    if (aControls.m_count < 1) {
        m_btnUseItem.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        m_btnUseItem.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
        m_lbItems.SetSelectedControl(-1, 0);
        m_labelDescription.m_text.m_params.SetText(CExoString(""));
    } else if (m_bRestoreListPosition) {
        m_lbItems.SetTopVisible(m_nSavedTopRow);
        int nLast = aControls.m_count - 1;
        m_lbItems.SetSelectedControl(static_cast<short>(nLast > m_nSavedSelectedIndex ? m_nSavedSelectedIndex : nLast), 0);
        m_bRestoreListPosition = 0;
    } else {
        m_lbItems.SetSelectedControl(nSelected == -1 ? 0 : nSelected, 0);
    }

    m_bRebuildItemList = 0;
}
#else
void CSWGuiInGameInventory::PopulateItemListBox()
{
    // Two passes, the player's equipped slots (14..17 have none), then the item repository. Both go
    // through CheckFilter; the equipped pass also computes a per-slot edit-mode flag.
    CSWCCreature* pPlayer = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    CSWSCreature* pServerCreature = pPlayer->GetServerCreature();
    CItemRepository* pRepo = pServerCreature->GetItemRepository(1);

    m_labelCreditsValue.m_text.m_params.SetText(CExoString(pServerCreature->GetGold()));

    CExoArrayList<CSWGuiControl*> aControls;
    int nIndex = 0;
    int nSelected = -1;
    CSWCItem* pEquipped;
    CSWSItem* pItem;
    int nFlag;

    for (int i = 0; i < 20; i++) {
        if (i >= 14 && i < 18)
            continue;

        pEquipped = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
            pPlayer->GetEquippedItemID(1 << i));
        if (pEquipped == 0)
            continue;

        pItem = pEquipped->GetServerItem();
        nFlag = (i == 7 || i == 8) | (i == 10);
        if (pItem == 0)
            continue;
        if (pItem->m_bits.m_bDeleting)
            continue;
        if (!CheckFilter(pItem, pServerCreature, nFlag))
            continue;

        if (m_oidSelected == pItem->m_objectId) {
            nSelected = nIndex;
            m_oidSelected = OBJECT_INVALID;
        }
        CreateItemEntry(pPlayer, nIndex, aControls, pItem, nFlag, 1);
    }

    for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++) {
        pItem = pRepo->ItemListGetItem(i);
        if (pItem == 0)
            continue;
        if (pItem->m_bits.m_bDeleting)
            continue;
        if (!CheckFilter(pItem, pServerCreature, 0))
            continue;

        if (m_oidSelected == pItem->m_objectId) {
            nSelected = nIndex;
            m_oidSelected = OBJECT_INVALID;
        }
        CreateItemEntry(pPlayer, nIndex, aControls, pItem, 0, 0);
    }

    m_lbItems.AddControls(aControls, 0, 1, 0);

    int nLast = aControls.m_count - 1;
    if (nLast < 0) {

        m_btnUseItem.m_text.m_params.SetColor(CGuiInGame::DISABLED_TEXT);
        m_btnUseItem.m_border.m_params.m_color =
            *reinterpret_cast<const CSWGuiColor*>(&CGuiInGame::DISABLED_ICON_COLOR);
        m_lbItems.SetSelectedControl(-1, 0);
        m_labelDescription.m_text.m_params.SetText(CExoString(""));
    } else if (m_bRestoreListPosition) {

        m_lbItems.SetTopVisible(m_nSavedTopRow);
        m_lbItems.SetSelectedControl(nLast > m_nSavedSelectedIndex ? m_nSavedSelectedIndex : nLast, 0);
        m_bRestoreListPosition = 0;
    } else {
        m_lbItems.SetSelectedControl(nSelected == -1 ? 0 : nSelected, 0);
    }

    m_bRebuildItemList = 0;
}
#endif

CSWGuiInGameInventory::~CSWGuiInGameInventory()
{
    m_lbDescription.ClearItems();
    m_lbItems.ClearItems();
    for (int i = 0; i < m_lists.m_aItemEntry.m_count; i++) {
        delete m_lists.m_aItemEntry.m_pArray[i];
        m_lists.m_aItemEntry.m_pArray[i] = 0;
    }

    m_lists.m_aItemEntry.m_count = 0;
}

#if KOTOR1

void CSWGuiInGameInventory::UpdatePlayerStats()
{
    CSWCCreature* pCreature;
    if (m_nNPC == -1 || !g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bTutorialModule) {
        pCreature = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    } else {
        CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
        pCreature = pClient->GetCreatureByGameObjectID(pClient->ServerToClientObjectId(
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(m_nNPC, 0, 1)));
    }
    if (pCreature == 0)
        return;

    m_labelPort.m_border.m_params.SetFillImage(pCreature->GetPortrait(1), 0);
    m_labelDef.m_text.m_params.SetText(CExoString(static_cast<short>(pCreature->m_pStats->GetArmorClass(0))));

    CExoString sVit;
    sVit.Format(const_cast<char*>("%d/%d"), pCreature->m_pStats->m_nCurrentHitPoints,
                static_cast<short>(pCreature->m_pStats->GetMaxHitPoints()));
    m_labelVit.m_text.m_params.SetText(sVit);
    if (static_cast<CSWGuiText*>(&m_labelVit.m_text)->GetIdealHeight() > m_labelVit.m_text.GetFontHeight()) {
        sVit.Format(const_cast<char*>("%d/ %d"), pCreature->m_pStats->m_nCurrentHitPoints,
                    static_cast<short>(pCreature->m_pStats->GetMaxHitPoints()));
        m_labelVit.m_text.m_params.SetText(sVit);
    }
}
#else
void CSWGuiInGameInventory::UpdatePlayerStats() {}
#endif

#if KOTOR1

void CSWGuiInGameInventory::UpdatePortraits()
{
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
}
#else
void CSWGuiInGameInventory::UpdatePortraits() {}
#endif

#if KOTOR1

void CSWGuiInGameInventory::OnChangeCharacter(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    if (!g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bTutorialModule) {
        unsigned char nMember = pControl == &m_btnChange2 ? 2 : 1;
        if (!g_pAppManager->m_pClientExoApp->ChangeCharacterToNextLivingPartyMember(0, 0, nMember))
            return;
        m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
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
            m_nNPC = nNPC;
        }
    }

    m_bRebuildItemList = 1;
    UpdatePortraits();
}

void CSWGuiInGameInventory::OnSwitchLeft(CSWGuiControl* pControl)
{
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
        if (i == m_nNPC)
            continue;
        if (!g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i))
            continue;
        pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(
                g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1)));
        if (pCreature != 0)
            goto found;
    }
    if (m_nNPC != -1 && static_cast<signed char>(m_btnChange2.m_tail.m_nUserData) != -1) {
        m_btnChange1.m_border.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange1.m_borderHilight.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange1.m_tail.m_nUserData = static_cast<uint64_t>(-1);
        return;
    }
    for (i = 8; i > nCur; i--) {
        if (i == m_nNPC)
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

void CSWGuiInGameInventory::OnSwitchRight(CSWGuiControl* pControl)
{
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
        if (i == m_nNPC)
            continue;
        if (!g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i))
            continue;
        pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(
                g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1)));
        if (pCreature != 0)
            goto found;
    }
    if (m_nNPC != -1) {
        m_btnChange2.m_border.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange2.m_borderHilight.m_params.SetFillImage(
            g_pAppManager->m_pClientExoApp->GetPlayerCreature()->GetPortrait(1), 0);
        m_btnChange2.m_tail.m_nUserData = static_cast<uint64_t>(-1);
        return;
    }
    for (i = 0; i < nCur; i++) {
        if (i == m_nNPC)
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
void CSWGuiInGameInventory::OnChangeCharacter(CSWGuiControl* pControl) { K2_UNUSED(pControl); }
#endif

void CSWGuiInGameInventory::Draw(float f)
{
    // Bit 2 asks for the list to be rebuilt first; BTN_USEITEM is tinted disabled unless bit 1 is
    // set and the player creature allows it. Every arm finishes with the panel's own draw.
#if KOTOR1

    if (m_bRebuildItemList) {
        PopulateItemListBox();
    }
    UpdatePlayerStats();

    const Vector* pColor;
    if (m_bSelectedItemUsable &&
        g_pAppManager->m_pServerExoApp->GetPlayerCreature()
                ->m_nItemUseCooldown == 0)
        pColor = &CGuiInGame::COLOR_BLUE;
    else
        pColor = &CGuiInGame::DISABLED_TEXT;
    m_btnUseItem.m_text.m_params.SetColor(*pColor);
    CSWGuiPanel::Draw(f);
#else
    if (m_bRebuildItemList) {
        PopulateItemListBox();
    }

    const Vector* pBorderColor;
    if (!m_bSelectedItemUsable ||
        g_pAppManager->m_pServerExoApp->GetPlayerCreature()
                ->m_nItemUseCooldown != 0) {
        m_btnUseItem.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_GREEN);
        pBorderColor = &CGuiInGame::DISABLED_TEXT;
    } else {
        if (m_btnUseItem.m_tail.bActive)
            m_btnUseItem.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        else
            m_btnUseItem.m_text.m_params.SetColor(CGuiInGame::COLOR_NEW_TEXT);
        pBorderColor = &CGuiInGame::COLOR_NEW_GREEN;
    }
    m_btnUseItem.m_border.m_params.m_color.r = pBorderColor->x;
    m_btnUseItem.m_border.m_params.m_color.g = pBorderColor->y;
    m_btnUseItem.m_border.m_params.m_color.b = pBorderColor->z;
    CSWGuiPanel::Draw(f);
#endif
}

void CSWGuiInGameInventory::OnPanelAdded()
{
#if KOTOR1

    m_nNPC = -1;
    m_btnChange1.m_tail.m_nUserData = static_cast<uint64_t>(-2);
    m_btnChange2.m_tail.m_nUserData = static_cast<uint64_t>(-2);
    UpdatePortraits();
#endif
    PopulateItemListBox();
    m_btnExit.HandleMouseOver(0);
    CSWGuiPanel::OnPanelAdded();
    g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_ENTER_INVENTORY_SCREEN, OBJECT_INVALID, OBJECT_INVALID, 0);
#if KOTOR1
    if (g_pAppManager != 0 && g_pAppManager->m_pClientExoApp != 0
        && g_pAppManager->m_pClientExoApp->GetSWParty() != 0)
#ifdef K2_UI_TOUCH
        m_iconChange.m_tail.bVisible = g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount > 1;
#else
        // the change button's own rule: a party of two or more, or in a module that browses the
        // party table (the Ebon Hawk, the hideout) any member it can show
        m_iconChange.m_tail.bVisible = m_btnChange1.m_tail.bVisible;
#endif
#endif
}

void CSWGuiInGameInventory::OnPanelRemoved()
{
    m_lbItems.ClearItems();
    for (int i = 0; i < m_lists.m_aItemEntry.m_count; i++) {
        delete m_lists.m_aItemEntry.m_pArray[i];
        m_lists.m_aItemEntry.m_pArray[i] = 0;
    }
    m_lists.m_aItemEntry.m_count = 0;
    m_lists.m_aItemEntry.Clear();
    g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems()->ClearNewItems(
        m_lists.m_aUsedOid);
}

#if KOTOR1

void CSWGuiInGameInventory::CreateItemEntry(CSWCCreature* pCreature, int& nIndex,
                                            CExoArrayList<CSWGuiControl*>& aControls,
                                            CSWSItem* pItem, int a, int b)
{
    // nIndex == count appends a new row; any other index reuses the row already there. The action
    // handler depends on whether the creature can use the item and on the base item's category.
#ifdef K2_UI_TOUCH
    static float s_fScale = static_cast<float>(g_SDL_mode.h) / 768.0f;
#endif
    CSWGuiInGameItemEntry* pEntry;
    if (m_lists.m_aItemEntry.m_count == nIndex) {
        pEntry = new CSWGuiInGameItemEntry();
        m_lists.m_aItemEntry.Add(pEntry);

        CSWGuiButton* pProto = static_cast<CSWGuiButton*>(m_lbItems.m_pProtoItem->AsButton());
        pEntry->Initialize(pProto->m_text, pProto->m_border, pProto->m_borderHilight,
                           m_lbItems.m_innerExtent.w - m_lbItems.m_nPadding * 2, 1);
#ifndef K2_UI_TOUCH

        if (pProto->m_extent.h > 0 && pProto->m_extent.h < 56) {
            pEntry->m_nBoxUnits = static_cast<unsigned char>(pProto->m_extent.h);
            CSWGuiExtent extentRow = pEntry->m_extent;
            pEntry->SetExtent(extentRow);
        }
#endif
        pEntry->AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameInventory::OnControlEntered));
    } else {
        pEntry = static_cast<CSWGuiInGameItemEntry*>(m_lists.m_aItemEntry.m_pArray[nIndex]);
    }

    if (m_nNPC != -1 && g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bTutorialModule) {
        pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                        &CSWGuiInGameInventory::NotUseableOnHawk));
        pEntry->m_tail.m_nUserData = 0;
    } else {
        int bUsable = pCreature->IsLeaderUsableItem(pItem, a, USABLE_CHECK_ALL);
        CSWBaseItem* pBase = pItem->GetBaseItem();

        if (!bUsable) {
            if (pBase->m_nEquipableSlots != 0) {
                pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                                &CSWGuiInGameInventory::CantEquip));
            } else {
                pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                                &CSWGuiInGameInventory::NotUseable));
            }
            pEntry->m_tail.m_nUserData = 0;
        } else if (pBase->m_nItemType == ITEM_TYPE_DROID_REPAIR_EQUIPMENT || pBase->m_nItemType == ITEM_TYPE_MEDICAL_EQUIPMENT) {
            if (pCreature->m_pStats->m_nCurrentHitPoints <
                static_cast<short>(pCreature->m_pStats->GetMaxHitPoints())) {
                pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                                &CSWGuiInGameInventory::OnControlSelected));
                pEntry->m_tail.m_nUserData = 1;
            } else {
                CSWSCreature* pServer = pCreature->GetServerCreature();
                if (pServer == 0 || pServer->m_bPoisoned == 0) {
                    pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                                    &CSWGuiInGameInventory::FullHealth));
                    pEntry->m_tail.m_nUserData = 0;
                }
            }
        } else if (pBase->m_nItemType == ITEM_TYPE_SQUAD_RECOVERY_KIT) {
            bool bBelow = pCreature->m_pStats->m_nCurrentHitPoints <
                          static_cast<short>(pCreature->m_pStats->GetMaxHitPoints());
            CSWCCreature* pM1 = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(1);
            if (pM1 != 0 && pM1->GetServerCreature() != 0)
                bBelow |= pM1->m_pStats->m_nCurrentHitPoints <
                          static_cast<short>(pM1->m_pStats->GetMaxHitPoints());
            CSWCCreature* pM2 = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(2);
            if (pM2 != 0 && pM2->GetServerCreature() != 0)
                bBelow |= pM2->m_pStats->m_nCurrentHitPoints <
                          static_cast<short>(pM2->m_pStats->GetMaxHitPoints());
            CSWSCreature* pServer = pCreature->GetServerCreature();
            if (!bBelow || pServer == 0) {
                pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                                &CSWGuiInGameInventory::SquadFullHealth));
                pEntry->m_tail.m_nUserData = 0;
            }
        } else {
            pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                            &CSWGuiInGameInventory::OnControlSelected));
            pEntry->m_tail.m_nUserData = 1;
        }
    }

    pEntry->SetItem(pItem->m_objectId, b, pItem->m_bits.m_bNewItem);
    pEntry->SetActive(0);
#ifdef K2_UI_TOUCH
    int nScreenW, nScreenH;
    if (IosIsIphone(nScreenW, nScreenH) && pEntry->m_text.m_pString != 0)
        pEntry->m_text.m_pString->SetFontScale(s_fScale);
#endif

    aControls.Add(pEntry);
    nIndex++;
}
#else
void CSWGuiInGameInventory::CreateItemEntry(CSWCCreature* pCreature, int& nIndex,
                                            CExoArrayList<CSWGuiControl*>& aControls,
                                            CSWSItem* pItem, int a, int b)
{
    CSWGuiInGameItemEntry* pEntry;
    if (m_lists.m_aItemEntry.m_count == nIndex) {
        pEntry = new CSWGuiInGameItemEntry();
        m_lists.m_aItemEntry.Add(pEntry);

        CSWGuiButton* pProto = static_cast<CSWGuiButton*>(m_lbItems.m_pProtoItem->AsButton());
        pEntry->Initialize(pProto->m_text, pProto->m_border, pProto->m_borderHilight,
                           m_lbItems.m_innerExtent.w - m_lbItems.m_nPadding * 2, 1);
        pEntry->AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameInventory::OnControlEntered));
        pEntry->AddEvent(SWGUIEVENT_UNFOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                      &CSWGuiInGameInventory::OnControlExited));
    } else {
        pEntry = static_cast<CSWGuiInGameItemEntry*>(m_lists.m_aItemEntry.m_pArray[nIndex]);
    }

    int bUsable = pCreature->IsLeaderUsableItem(pItem, a, USABLE_CHECK_ALL);
    CSWBaseItem* pBase = pItem->GetBaseItem();

    if (!bUsable) {
        if (pBase->m_nEquipableSlots != 0) {
            pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                            &CSWGuiInGameInventory::CantEquip));
        } else {
            pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                            &CSWGuiInGameInventory::NotUseable));
        }
        pEntry->m_tail.m_nUserData = 0;
    } else if (pBase->m_nItemType == ITEM_TYPE_DROID_REPAIR_EQUIPMENT || pBase->m_nItemType == ITEM_TYPE_MEDICAL_EQUIPMENT) {

        if (pCreature->m_pStats->m_nCurrentHitPoints <
            static_cast<short>(pCreature->m_pStats->GetMaxHitPoints())) {

            if ((pBase->m_nItemType == ITEM_TYPE_MEDICAL_EQUIPMENT) ==
                (pCreature->m_pStats->m_nRace == CSWRace::CRACE_DROID)) {
                pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                                &CSWGuiInGameInventory::NotUseable));
                pEntry->m_tail.m_nUserData = 0;
            } else {
                pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                                &CSWGuiInGameInventory::OnControlSelected));
                pEntry->m_tail.m_nUserData = 1;
            }
        } else {
            CSWSCreature* pServer = pCreature->GetServerCreature();
            if (pServer == 0 || pServer->m_bPoisoned == 0) {
                pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                                &CSWGuiInGameInventory::FullHealth));
                pEntry->m_tail.m_nUserData = 0;
            }
        }
    } else if (pBase->m_nItemType == ITEM_TYPE_SQUAD_RECOVERY_KIT) {

        if (pCreature->m_pStats->m_nRace == CSWRace::CRACE_DROID) {
            pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                            &CSWGuiInGameInventory::OnControlSelected));
            pEntry->m_tail.m_nUserData = 1;
        } else {

            bool bBelow = pCreature->m_pStats->m_nCurrentHitPoints <
                          static_cast<short>(pCreature->m_pStats->GetMaxHitPoints());
            CSWCCreature* pM1 = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(1);
            if (pM1 != 0 && pM1->GetServerCreature() != 0)
                bBelow |= pM1->m_pStats->m_nCurrentHitPoints <
                          static_cast<short>(pM1->m_pStats->GetMaxHitPoints());
            CSWCCreature* pM2 = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(2);
            if (pM2 != 0 && pM2->GetServerCreature() != 0)
                bBelow |= pM2->m_pStats->m_nCurrentHitPoints <
                          static_cast<short>(pM2->m_pStats->GetMaxHitPoints());
            CSWSCreature* pServer = pCreature->GetServerCreature();
            if (!bBelow || pServer == 0) {
                pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                                &CSWGuiInGameInventory::SquadFullHealth));
                pEntry->m_tail.m_nUserData = 0;
            }
        }
    } else {
        pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(
                                        &CSWGuiInGameInventory::OnControlSelected));
        pEntry->m_tail.m_nUserData = 1;
    }

    pEntry->SetItem(pItem->m_objectId, b, pItem->m_bits.m_bNewItem);
    pEntry->SetActive(0);

    aControls.Add(pEntry);
    nIndex++;
}
#endif

void CSWGuiInGameInventory::OnControlSelected(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWGuiControl* pSelected = m_lbItems.GetSelectedControl();
    if (pSelected == 0)
        return;
    unsigned long long oid = static_cast<CSWGuiInGameItemEntry*>(pSelected)->m_oidItem;
    if (oid == OBJECT_INVALID)
        return;

    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    CSWSCreature* pCreature = pServer->GetCreatureByGameObjectID(pServer->GetPlayerCreatureId());
    if (pCreature->m_bCombatState != 0 && pCreature->m_nCombatType == 1 &&
        pCreature->m_nItemUseCooldown != 0) {
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetMessage(STRREF_ONE_ITEM_PER_ROUND);
        pBox->SetCallback(0, 0, 0);
        m_pGuiManager->AddPanel(pBox, 1, 1);
        return;
    }

    pCreature->m_bCastBroadcastProjectile = 0;
#if !KOTOR1
    pCreature->m_oidSpellTarget = pCreature->m_objectId;
#endif
    pCreature->InstantUseItem(oid);

    m_bRestoreListPosition = 1;
    m_bRebuildItemList = 1;
    m_oidSelected = oid;
    m_nSavedTopRow = m_lbItems.m_nTopRow;
    m_nSavedSelectedIndex = m_lbItems.m_nSelectedIndex;
}

void CSWGuiInGameInventory::HandleUseItemAccept(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;
    if (!g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted)
        return;

    CSWGuiControl* pSelected = m_lbItems.GetSelectedControl();
    if (pSelected == 0)
        return;
    unsigned long long oid = static_cast<CSWGuiInGameItemEntry*>(pSelected)->m_oidItem;
    if (oid == OBJECT_INVALID)
        return;

    CSWCCreature* pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
    if (pChar == 0)
        return;
    pChar->PerformUseItem(oid, 0, pChar->m_objectId, pChar->m_position);
    g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGuiInGameInventoryDirty(1);
}

int CSWGuiInGameInventory::CheckFilter(CSWSItem* pItem, CSWSCreature* pCreature, int nFlag)
{
    // One arm per filter index: 0 is All, the rest are the six tabs. 1..5 are never reachable
    // from the UI but are real cases.
    if (pItem == 0)
        return 0;

    switch (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nInventoryFilter) {
#if KOTOR1
    default:
        return 0;
#endif
    case INVENTORY_FILTER_ALL:
        return 1;
    case INVENTORY_FILTER_NEW:
        return pItem->m_bits.m_bNewItem;
    case INVENTORY_FILTER_QUEST:
#if !KOTOR1
    case INVENTORY_FILTER_QUEST_TAB:
#endif
        return pItem->m_bPlot;
    case INVENTORY_FILTER_EQUIPPABLE:
        return pItem->GetBaseItem() != 0 && pItem->GetBaseItem()->m_nEquipableSlots != 0;
    case INVENTORY_FILTER_UTILITY:
        return pItem->GetBaseItem() != 0 && pItem->m_bPlot == 0 &&
               pItem->GetBaseItem()->m_nEquipableSlots == 0;
    case INVENTORY_FILTER_USEABLE:
        return pCreature->GetClientCreature() != 0 &&
               pCreature->GetClientCreature()->IsLeaderUsableItem(pItem, nFlag, USABLE_CHECK_ALL) &&
               pCreature->CanUseItem(pItem, 0, 1) != 0;
#if !KOTOR1
    case INVENTORY_FILTER_DATAPADS:
        return pItem->GetBaseItem() != 0 && pItem->GetBaseItem()->m_nItemType == ITEM_TYPE_DATA_PAD;
    case INVENTORY_FILTER_WEAPONS:
        return pItem->GetBaseItem() != 0 &&
               ((pItem->GetBaseItem()->m_nEquipableSlots & (CSWInventory::SLOT_RIGHTWEAPON | CSWInventory::SLOT_LEFTWEAPON)) != 0 ||
                ((pItem->GetBaseItem()->m_nEquipableSlots & (CSWInventory::SLOT_LEFTARM | CSWInventory::SLOT_RIGHTARM)) != 0 &&
                 pItem->GetBaseItem()->m_nRaceRestriction == 2));
    case INVENTORY_FILTER_ARMOR:
        return pItem->GetBaseItem() != 0 &&
               (pItem->GetBaseItem()->m_nEquipableSlots & SLOTS_WORN_NOT_WEAPON) != 0 &&
               ((pItem->GetBaseItem()->m_nEquipableSlots & (CSWInventory::SLOT_LEFTARM | CSWInventory::SLOT_RIGHTARM)) == 0 ||
                pItem->GetBaseItem()->m_nRaceRestriction != 2);
    case INVENTORY_FILTER_USEABLE_TAB:
        return pCreature->GetClientCreature() != 0 &&
               static_cast<unsigned>(pItem->m_baseItem - BASE_ITEM_ADRENALINE) < 4;
    case INVENTORY_FILTER_MISC: {

        int nResult = pItem->GetBaseItem() == 0 ||
                      (static_cast<int>(pItem->GetBaseItem()->m_nItemType != ITEM_TYPE_DATA_PAD) &
                       static_cast<int>(pItem->GetBaseItem()->m_nEquipableSlots == 0));
        if (pCreature->GetClientCreature() != 0 &&
            static_cast<unsigned>(pItem->m_baseItem - BASE_ITEM_ADRENALINE) < 4)
            nResult = 0;
        return nResult;
    }
#endif
    }
#if !KOTOR1
    return 0;
#endif
}

void CSWGuiInGameInventory::HandleInputEvent(int type, int val)
{
#if KOTOR1

    if (val != 0) {
        switch (type) {
        case INPUTEVENT_GUI_X:
            SetNextFilter();
            m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            m_bRebuildItemList = 1;
            break;

        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_START:
        case INPUTEVENT_GUI_BACK:
        case INPUTEVENT_GUI:
            if (g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSWInGameGui(0))
                g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
            break;

#ifdef K1_UI_XBOX  // xbox: RB (0x2b in menus) changes the character, not Y
        case INPUTEVENT_GUI_BLACK:
#else
        case INPUTEVENT_GUI_Y:
#endif
        case INPUTEVENT_CHANGE_CHARACTER:
            m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            g_pAppManager->m_pClientExoApp->ChangeCharacterToNextLivingPartyMember(0, 0, 1);
            m_bRebuildItemList = 1;
            UpdatePortraits();
            break;

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
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
#else
    if (val != 0) {
        switch (type) {
        case INPUTEVENT_GUI_B:
        case INPUTEVENT_GUI_START:
        case INPUTEVENT_GUI_BACK:
        case INPUTEVENT_GUI:
            if (g_pAppManager->m_pClientExoApp->GetInGameGui()->HideSWInGameGui(0))
                g_pAppManager->m_pClientExoApp->SetInputClass(INPUTCLASS_GAME, 1);
            break;

#ifndef K2_UI_XBOX  // xbox: RB alone changes the character
        case INPUTEVENT_GUI_Y:
#endif
        case INPUTEVENT_CHANGE_CHARACTER:
            g_pAppManager->m_pClientExoApp->ChangeCharacterToNextLivingPartyMember(0, 0, 1);
            m_pGuiManager->PlayGuiSound(GUISOUND_ENTERED_DEFAULT);
            m_bRebuildItemList = 1;
            break;

#ifdef K2_FIX_INVENTORY_TAB_ORDER

        // The seven filter indices do not follow the on-screen tab order. The layout draws Quests
        // left of Misc, so stepping the index walks the tabs backwards. Walk them in on-screen
        // order instead; a value outside the seven-filter set starts from All.
        case INPUTEVENT_GUI_DPAD_LEFT:
        case INPUTEVENT_GUI_DPAD_RIGHT: {

            CSWGuiButton* apButton[] = {&m_btnAll, &m_btnDatapads, &m_btnWeapons, &m_btnArmor,
                                        &m_btnUseable, &m_btnQuests, &m_btnMisc};
            const int nCount = sizeof(apButton) / sizeof(apButton[0]);
            for (int i = 1; i < nCount; ++i)
                for (int j = i; j > 0 && apButton[j]->m_extent.x < apButton[j - 1]->m_extent.x; --j) {
                    CSWGuiButton* pSwap = apButton[j];
                    apButton[j] = apButton[j - 1];
                    apButton[j - 1] = pSwap;
                }
            int kScreenOrder[nCount];
            for (int i = 0; i < nCount; ++i)
                kScreenOrder[i] = static_cast<int>(apButton[i]->m_tail.m_nUserData);
            int nFilter = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nInventoryFilter;
            int nPos = 0;
            for (int i = 0; i < nCount; ++i)
                if (kScreenOrder[i] == nFilter)
                    nPos = i;
            nPos = (type == INPUTEVENT_GUI_DPAD_RIGHT) ? (nPos + 1) % nCount : (nPos + nCount - 1) % nCount;
            SetNextFilter(kScreenOrder[nPos]);
            break;
        }
#else
        case INPUTEVENT_GUI_DPAD_LEFT: {
            int nFilter = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nInventoryFilter;
            if (nFilter == INVENTORY_FILTER_DATAPADS)
                nFilter = INVENTORY_FILTER_ALL;
            else
                nFilter = (nFilter == 0) ? 11 : nFilter - 1;
            SetNextFilter(nFilter);
            break;
        }
        case INPUTEVENT_GUI_DPAD_RIGHT: {
            int nFilter = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nInventoryFilter;
            if (nFilter == INVENTORY_FILTER_ALL)
                nFilter = INVENTORY_FILTER_DATAPADS;
            else
                nFilter = (nFilter > 10) ? 0 : nFilter + 1;
            SetNextFilter(nFilter);
            break;
        }
#endif
        case INPUTEVENT_GUI_SCROLL_UP:
            m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
            break;

        case INPUTEVENT_GUI_SCROLL_DOWN:
            m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
            break;
        }
    }
    CSWGuiPanel::HandleInputEvent(type, val);
#endif
}

#if !KOTOR1
void CSWGuiInGameInventory::OnControlExited(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWSItem* pServerItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
        static_cast<CSWGuiInGameItemEntry*>(pControl)->m_oidItem);
    if (pServerItem == 0)
        return;
    if (!pServerItem->m_bits.m_bNewItem)
        return;

    pServerItem->m_bits.m_bNewItem = 0;
    static_cast<CSWGuiInGameItemEntry*>(pControl)->bNewItem = 0;
    static_cast<CSWGuiInGameItemEntry*>(pControl)->UpdateItem();
}
#endif

void CSWGuiInGameInventory::FullHealth(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    pBox->SetAllowCancel(0);
    pBox->SetMessage(STRREF_ALREADY_FULL_HEALTH);
    pBox->SetCallback(0, 0, 0);
    m_pGuiManager->AddPanel(pBox, 1, 1);
}

void CSWGuiInGameInventory::SquadFullHealth(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    pBox->SetAllowCancel(0);
    pBox->SetMessage(STRREF_PARTY_NOT_INJURED);
    pBox->SetCallback(0, 0, 0);
    m_pGuiManager->AddPanel(pBox, 1, 1);
}

#if KOTOR1

void CSWGuiInGameInventory::NotUseableOnHawk(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    pBox->SetAllowCancel(0);
    pBox->SetMessage(STRREF_NOT_USEABLE_ON_HAWK);
    pBox->SetCallback(0, 0, 0);
    m_pGuiManager->AddPanel(pBox, 1, 1);
}
#endif

void CSWGuiInGameInventory::NotUseable(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    pBox->SetAllowCancel(0);
    pBox->SetMessage(STRREF_NOT_USEABLE_OR_EQUIPABLE);
    pBox->SetCallback(0, 0, 0);
    m_pGuiManager->AddPanel(pBox, 1, 1);
}

void CSWGuiInGameInventory::CantEquip(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
    pBox->SetAllowCancel(0);
    pBox->SetMessage(STRREF_CANNOT_EQUIP_FROM_INVENTORY);
    pBox->SetCallback(0, 0, 0);
    m_pGuiManager->AddPanel(pBox, 1, 1);
}

void CSWGuiInGameInventory::OnControlEntered(CSWGuiControl* pControl)
{
    // The row under the cursor becomes the description pane. Its property description goes through
    // the TLK token parser when it carries both markup braces, then is re-seated as the only row of
    // LB_DESCRIPTION.
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    unsigned long long oid = static_cast<CSWGuiInGameItemEntry*>(pControl)->m_oidItem;

    if (oid == OBJECT_INVALID) {
        m_bSelectedItemUsable = 0;
        m_labelDescription.m_text.m_params.SetText(CExoString("Error: Invalid item"));
    } else {
        CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oid);
        CExoString sDesc(pItem->GetPropertyDescription());

#if KOTOR1

        m_labelDescription.m_text.m_params.SetText(sDesc);

        if (m_nNPC == -1 || !g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bTutorialModule)
            m_bSelectedItemUsable = pControl->m_tail.m_nUserData & 1;
        else
            m_bSelectedItemUsable = 0;
#else

        int nLt = sDesc.Find('<', 0);
        int nLb = sDesc.Find('{', 0);
        if ((nLt & nLb) != -1) {
            g_pSWTlkTable->m_pSpeakerObj =
                g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
            g_pSWTlkTable->m_gender = g_pSWTlkTable->GetObjectGender();
            g_pSWTlkTable->m_bSuppressEnabled |= 1;
            g_pSWTlkTable->ParseStr(sDesc);
        }
        m_labelDescription.m_text.m_params.SetText(sDesc);

        m_bSelectedItemUsable = pControl->m_tail.m_nUserData & 1;
#endif

        if (pItem->m_bits.m_bNewItem) {
            if (m_lists.m_aUsedOid.Contains(oid) == 0)
                m_lists.m_aUsedOid.Add(oid);
        }
    }

    m_lbDescription.ClearItems();

    CSWGuiText* pText = &m_labelDescription.m_text;
#ifdef K2_QOL_IMPROVEMENTS
    K2QolFitText(*pText, 0x7fffffff);
#endif
    CSWGuiExtent extent = m_labelDescription.m_extent;
#if KOTOR1
    if (m_labelDescription.m_text.m_params.m_sText.GetLength() >= 1)
#else
    if (m_labelDescription.m_text.m_params.m_sText.m_sString != 0 &&
        static_cast<int>(strlen(m_labelDescription.m_text.m_params.m_sText.m_sString)) >= 1)
#endif
        extent.h = pText->GetIdealHeight();
    else
        extent.h = pText->GetFontHeight();
    m_labelDescription.SetExtent(extent);

    CSWGuiControl* apRows[1] = { &m_labelDescription };
    m_lbDescription.AddControls(apRows, 1, 0, 0, 0);
    m_lbDescription.SetSelectedControl(&m_labelDescription, 0);
}

#if KOTOR1

void CSWGuiInGameInventory::SetNextFilter()
{
    unsigned int nFilter = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nInventoryFilter;
    nFilter = nFilter > 4 ? 0 : nFilter + 1;
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nInventoryFilter = static_cast<unsigned char>(nFilter);

    unsigned int nNext = nFilter > 4 ? 0 : nFilter + 1;
    CExoString sText = g_pTlkTable->GetSimpleString(STRREF_SHOW) + CExoString(" ") +
                       g_pTlkTable->GetSimpleString(m_nFilterName[nNext]);
    m_btnQuestItems.m_text.m_params.SetText(sText);
    IosAddButtonIcon(&m_btnQuestItems, 'x');
    sText = g_pTlkTable->GetSimpleString(STRREF_PARTY_INVENTORY) + CExoString(" - ") +
            g_pTlkTable->GetSimpleString(m_nFilterName[nFilter]);
    m_labelInv.m_text.m_params.SetText(sText);
}
#endif

#if !KOTOR1

void CSWGuiInGameInventory::SetNextFilter(CSWGuiControl* pControl)
{
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nInventoryFilter =
        static_cast<unsigned char>(pControl->m_tail.m_nUserData);

    m_btnAll.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnDatapads.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnWeapons.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnArmor.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnUseable.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnMisc.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnQuests.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);

    static_cast<CSWGuiButton*>(pControl->AsButton())->m_text.m_params.SetDefaultColor(
        CGuiInGame::COLOR_NEW_HIGHLIGHT);
    m_bRebuildItemList = 1;
}

void CSWGuiInGameInventory::SetNextFilter(int nFilter)
{
    // Repaints all seven tabs, then highlights the matching one. 1..5 are not filters and
    // highlight nothing.
    if (static_cast<unsigned>(nFilter) > 11)
        return;

    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nInventoryFilter = static_cast<unsigned char>(nFilter);

    m_btnAll.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnDatapads.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnWeapons.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnArmor.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnUseable.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnMisc.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);
    m_btnQuests.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_TEXT);

    switch (nFilter) {
    case INVENTORY_FILTER_ALL:
        m_btnAll.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        break;
    case INVENTORY_FILTER_DATAPADS:
        m_btnDatapads.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        break;
    case INVENTORY_FILTER_WEAPONS:
        m_btnWeapons.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        break;
    case INVENTORY_FILTER_ARMOR:
        m_btnArmor.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        break;
    case INVENTORY_FILTER_USEABLE_TAB:
        m_btnUseable.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        break;
#ifdef K2_FIX_ENGINE_BUGS
    case INVENTORY_FILTER_QUEST_TAB:
    case INVENTORY_FILTER_MISC:

        (m_btnMisc.m_tail.m_nUserData == static_cast<unsigned long long>(nFilter) ? m_btnMisc : m_btnQuests)
            .m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        break;
#else
    case INVENTORY_FILTER_QUEST_TAB:
        m_btnMisc.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        break;
    case INVENTORY_FILTER_MISC:
        m_btnQuests.m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
        break;
#endif
    }

    m_bRebuildItemList = 1;
}
#endif
