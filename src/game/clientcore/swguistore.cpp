#include "k2stdint.h"
#include "swguistore.h"

#include "appmanager.h"
#include "clientexoapp.h"
#include "serverexoapp.h"
#include "guiingame.h"
#include "swguiingame.h"
#include "swgui.h"
#include "swguidrawable.h"
#include "swguiingameequip.h"
#include "nwscreature.h"
#include "nwsstore.h"
#include "nwsitem.h"
#include "nwitem.h"
#include "nwcitem.h"
#include "nwccreature.h"
#include "swparty.h"
#include "exoresman.h"
#include "exostring.h"
#include "CExoArrayListImpl.h"
#include "nwtlktable.h"
#include "exotlktable.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "nwccreatureappearance.h"
#if KOTOR1
#include "port/k1gui.h"
#include "aurgui.h"
bool IosIsIphone(int& nScreenWidth, int& nScreenHeight);
#endif

extern CTlkTable* g_pTlkTable;

#include <stddef.h>

void AspyrAddButtonIcon(CSWGuiButton* pButton, char nIcon);

// SetDescription compares strlen's result signed, as CExoString::GetLength does.
#include "port/libcstring.h"
#include "exoinput.h"
#include "nwscriptconst.h"
#include "strref.h"
#include "exostringinline.h"

// Merchant panel with the buy and sell lists, the item description box and the two refusal paths.

#if KOTOR1
// K1 uses the "store" layout: no filter bar and no on-hand controls, and the panel rescales itself
// right after the layout load.
CSWGuiStore::CSWGuiStore(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager), m_nReserved2(0)
{
    StartLoadFromLayout(CResRef("store"));

    InitControl(m_lblBuySell, CExoString("LBL_BUYSELL"), 1);
    InitControl(m_lbShopItems, CExoString("LB_SHOPITEMS"), 1);
    InitControl(m_lbInvItems, CExoString("LB_INVITEMS"), 1);
    InitControl(m_lblCredits, CExoString("LBL_CREDITS"), 1);
    InitControl(m_lblCreditsValue, CExoString("LBL_CREDITS_VALUE"), 1);
    InitControl(m_lbDescription, CExoString("LB_DESCRIPTION"), 1);
    m_lbDescription.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_lblCost, CExoString("LBL_COST"), 1);
    InitControl(m_lblCostValue, CExoString("LBL_COST_VALUE"), 1);
    InitControl(m_lblStock, CExoString("LBL_STOCK"), 1);
    InitControl(m_lblStockValue, CExoString("LBL_STOCK_VALUE"), 1);
    InitControl(m_btnCancel, CExoString("BTN_Cancel"), 1);
    InitControl(m_btnExamine, CExoString("BTN_Examine"), 1);
    InitControl(m_btnAccept, CExoString("BTN_Accept"), 1);

    StopLoadFromLayout();
    ScaleExtentForResolution();

    {
        CSWGuiLabel* pProto = static_cast<CSWGuiLabel*>(m_lbDescription.m_pProtoItem);
        m_lblDescription.Initialize(pProto->m_extent, pProto, 1.0f);
    }

    m_lbShopItems.bShowScrollBar = 1;
    m_lbInvItems.bShowScrollBar = 1;
    SetActiveControl(&m_lbShopItems, 0);
    m_lbInvItems.m_tail.bVisible = 0;
    m_nReserved1 = 0;
    m_nGold = 0;
    m_bPopulatePending = 0;
    m_btnCancel.m_tail.bFocusOnHover = 0;
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnExamine.m_tail.bFocusOnHover = 0;
    m_btnExamine.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiStore::OnControlXButton));
    m_btnAccept.m_tail.bFocusOnHover = 0;
    m_lbInvItems.bSelectOnLeftClick = 1;
    m_lbShopItems.bSelectOnLeftClick = 1;
    m_nFullScreenBGIndex = 1;
    IosAddButtonIcon(&m_btnCancel, 'b');
}
#else
CSWGuiStore::CSWGuiStore(CSWGuiManager* pGuiManager)
    : CSWGuiPanel(pGuiManager), m_nReserved2(0)
{
    StartLoadFromLayout(CResRef("store_p"), 1, 1, 0);

    InitControl(m_lblBuySell, CExoString("LBL_BUYSELL"), 1, 1, 0, 0, 0);
    InitControl(m_lbShopItems, CExoString("LB_SHOPITEMS"), 1, 1, 0, 0, 0);
    InitControl(m_lbInvItems, CExoString("LB_INVITEMS"), 1, 1, 0, 0, 0);
    InitControl(m_lblCredits, CExoString("LBL_CREDITS"), 1, 1, 0, 0, 0);
    InitControl(m_lblCreditsValue, CExoString("LBL_CREDITS_VALUE"), 1, 1, 0, 0, 0);
    InitControl(m_lbDescription, CExoString("LB_DESCRIPTION"), 1, 1, 0, 0, 0);
    m_lbDescription.m_nScrollSound = GUISOUND_ENTERED_DEFAULT;
    InitControl(m_lblCost, CExoString("LBL_COST"), 1, 1, 0, 0, 0);
    InitControl(m_lblCostValue, CExoString("LBL_COST_VALUE"), 1, 1, 0, 0, 0);
    InitControl(m_lblStock, CExoString("LBL_STOCK"), 1, 1, 0, 0, 0);
    InitControl(m_lblStockValue, CExoString("LBL_STOCK_VALUE"), 1, 1, 0, 0, 0);
    InitControl(m_btnCancel, CExoString("BTN_Cancel"), 1, 1, 0, 0, 0);
    InitControl(m_btnExamine, CExoString("BTN_Examine"), 1, 1, 0, 0, 0);
    InitControl(m_btnAccept, CExoString("BTN_Accept"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[0], CExoString("LBL_BAR1"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[1], CExoString("LBL_BAR2"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[2], CExoString("LBL_BAR3"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[3], CExoString("LBL_BAR4"), 1, 1, 0, 0, 0);
    InitControl(m_aLabelBar[4], CExoString("LBL_BAR5"), 1, 1, 0, 0, 0);
    InitControl(m_lblFilter, CExoString("LBL_FILTER"), 1, 1, 0, 0, 0);
    InitControl(m_lblOnHand, CExoString("LBL_ONHAND"), 1, 1, 0, 0, 0);
    InitControl(m_lblOnHandValue, CExoString("LBL_ONHAND_VALUE"), 1, 1, 0, 0, 0);
    InitControl(m_aFilterButtons[0], CExoString("BTN_ALL"), 1, 1, 0, 0, 0);
    InitControl(m_aFilterButtons[1], CExoString("BTN_WEAPONS"), 1, 1, 0, 0, 0);
    InitControl(m_aFilterButtons[2], CExoString("BTN_ARMOR"), 1, 1, 0, 0, 0);
    InitControl(m_aFilterButtons[3], CExoString("BTN_MISC"), 1, 1, 0, 0, 0);

    StopLoadFromLayout();

    {
        CSWGuiLabel* pProto = static_cast<CSWGuiLabel*>(m_lbDescription.m_pProtoItem);
        m_lblDescription.Initialize(pProto->m_extent, pProto, 1.0f);
    }

    m_lbShopItems.bShowScrollBar = 1;
    m_lbInvItems.bShowScrollBar = 1;
    SetActiveControl(&m_lbShopItems, 0);
    m_lbInvItems.m_tail.bVisible = 0;
    m_nReserved1 = 0;
    m_nGold = 0;
    m_bPopulatePending = 0;
    m_btnCancel.m_tail.bFocusOnHover = 0;
    m_btnCancel.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiPanel::OnBButtonPressed));
    m_btnExamine.m_tail.bFocusOnHover = 0;
    m_btnExamine.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiStore::OnControlXButton));
    m_btnAccept.m_tail.bFocusOnHover = 0;
    m_lbInvItems.bSelectOnLeftClick = 1;
    m_lbShopItems.bSelectOnLeftClick = 1;
    for (int i = 0; i < 4; i++)
    {
        m_aFilterButtons[i].m_tail.bFocusOnHover = 0;
        m_aFilterButtons[i].m_tail.m_nUserData = i;
        m_aFilterButtons[i].AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiStore::OnFilterPressed));
    }
    m_nFullScreenBGIndex = 1;
    m_nFilter = FILTER_ALL;
    m_bRepopulateInventory = 0;
    AspyrAddButtonIcon(&m_btnCancel, 'b');
}
#endif

// Runs the body for a null control; only a non-null control with a cleared event argument returns
// early.
void CSWGuiStore::OnControlXButton(CSWGuiControl* pControl)
{
    if (pControl != NULL && pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWSStore* pStore = g_pAppManager->m_pServerExoApp->GetStoreByGameObjectID(m_oidStore);
    if (!pStore)
        return;
    if (pStore->m_buySellFlag != CSWSStore::STORE_BUY_AND_SELL)
        return;

    if (m_lbShopItems.m_tail.bVisible)
        ShowSellGUI();
    else
        ShowBuyGUI();
}

#if !KOTOR1  // K2-only: K1's store has no item filter
void CSWGuiStore::OnFilterPressed(CSWGuiControl* pControl)
{
    m_nFilter = static_cast<unsigned char>(pControl->m_tail.m_nUserData);
    UpdateFilters();
}
#endif

CSWGuiStore::~CSWGuiStore()
{
    DeleteItemEntries();
}

void CSWGuiStore::DeleteItemEntries()
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

void CSWGuiStore::OnPanelAdded()
{
#if KOTOR1
    // K1 has no filter reset; the lists are rebuilt by ShowBuyGUI alone.
    InitializeStoreType();
    PopulateStore();
    CSWGuiPanel::OnPanelAdded();
    ShowBuyGUI();
#else
    InitializeStoreType();
    m_nFilter = FILTER_ALL;
    UpdateFilters();
    PopulateStore();
    CSWGuiPanel::OnPanelAdded();
    ShowBuyGUI();
    m_bRepopulateInventory = 0;
#endif
}

// The store's buy/sell flag selects which controls are offered: 2 is a sell-only store, while 1 and
// 3 differ in whether those controls are visible.
void CSWGuiStore::InitializeStoreType()
{
    CSWSStore* pStore = g_pAppManager->m_pServerExoApp->GetStoreByGameObjectID(m_oidStore);
    if (!pStore)
        return;

    switch (pStore->m_buySellFlag)
    {
    case CSWSStore::STORE_BUY_AND_SELL:
        m_lblExamineIcon.m_tail.bVisible = 1;
        m_lblExamineCaption.m_tail.bVisible = 1;
        m_btnExamine.m_tail.bVisible = 1;
        ShowBuyGUI();
        break;
    case CSWSStore::STORE_SELL_ONLY:
        m_lblExamineIcon.m_tail.bVisible = 0;
        m_lblExamineCaption.m_tail.bVisible = 0;
        m_btnExamine.m_tail.bVisible = 0;
        ShowSellGUI();
        break;
    case CSWSStore::STORE_BUY_ONLY:
        m_lblExamineIcon.m_tail.bVisible = 0;
        m_lblExamineCaption.m_tail.bVisible = 0;
        m_btnExamine.m_tail.bVisible = 0;
        ShowBuyGUI();
        break;
    }
}

#if !KOTOR1  // K2-only: K1's store has no item filter
void CSWGuiStore::UpdateFilters()
{
    m_aFilterButtons[0].m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_LIGHT_GOLD);
    m_aFilterButtons[1].m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_LIGHT_GOLD);
    m_aFilterButtons[2].m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_LIGHT_GOLD);
    m_aFilterButtons[3].m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_LIGHT_GOLD);
    m_aFilterButtons[m_nFilter].m_text.m_params.SetDefaultColor(CGuiInGame::COLOR_NEW_HIGHLIGHT);
    PopulateInventoryItemListBox();
    PopulateStoreItemListBox();
}
#endif

void CSWGuiStore::PopulateStore()
{
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidPlayer);
    if (!pCreature)
        return;

    int nGold = pCreature->GetGold();
    m_nGold = nGold;
    m_lblCreditsValue.m_text.m_params.SetText(CExoString(nGold));
}

void CSWGuiStore::ShowBuyGUI()
{
    m_lbInvItems.m_tail.bVisible = 0;
    m_lbShopItems.m_tail.bVisible = 1;
#if KOTOR1  // K1 captions the accept button with the buy/sell title itself
    m_lblAcceptCaption.m_text.m_params.SetStrRef(STRREF_BUYING_ITEMS);
    m_btnAccept.m_text.m_params.SetStrRef(STRREF_BUYING_ITEMS);
#else
    m_lblAcceptCaption.m_text.m_params.SetStrRef(STRREF_BUY_ITEM);
    m_btnAccept.m_text.m_params.SetStrRef(STRREF_BUY_ITEM);
#endif
    m_btnAccept.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiStore::OnControlStoreAButton));
    m_lblBuySell.m_text.m_params.SetStrRef(STRREF_BUYING_ITEMS);
    m_lblExamineCaption.m_text.m_params.SetStrRef(STRREF_SHOW_SELL_LIST);
    m_btnExamine.m_text.m_params.SetStrRef(STRREF_SHOW_SELL_LIST);
    m_lblCost.m_text.m_params.SetStrRef(STRREF_ITEM_COST);
    SetActiveControl(&m_lbShopItems, 0);
    PopulateStoreItemListBox();
    AspyrAddButtonIcon(&m_btnAccept, 'a');
    AspyrAddButtonIcon(&m_btnExamine, 'x');
}

void CSWGuiStore::OnPanelRemoved()
{
    DeleteItemEntries();
    CSWGuiPanel::OnPanelRemoved();
}

void CSWGuiStore::PopulateInventoryItemListBox()
{
#if KOTOR1
    // K1 has no equipped-item rows, no filter, no list clear and no on-hand caption; the repository
    // is read even when the player lookup failed.
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidPlayer);
    CItemRepository* pRepo;
    if (pCreature)
        pRepo = pCreature->GetItemRepository(1);
#ifdef K2_FIX_ENGINE_BUGS
    else
        return;
#endif

    CExoArrayList<CSWGuiControl*> aControls;
    int nIndex = 0;

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
                int nSize;
                CSWSItem* pItem = pRepo->ItemListGetItem(i);
                if (!pItem)
                    continue;
                if (pItem->m_bPlot)
                    continue;

                nSize = pItem->GetBaseItem()->m_nStorePanelSort;
                if (nSize == nGroup)
                {
                    CreateItemEntry(nIndex, aControls, pItem->m_objectId, 1);
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
            m_lblDescription.m_text.m_params.SetText(CExoString(""));
        }
    }

    m_bPopulatePending = 0;
#else
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidPlayer);
    if (!pCreature)
        return;

    CItemRepository* pRepo = pCreature->GetItemRepository(1);
    m_lbInvItems.ClearItems();

    CExoArrayList<CSWGuiControl*> aControls;
    int nIndex = 0;

    // Equipped items first, skipping the four unequippable "creature weapon" slots 14..17.
    CSWCCreature* pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
    if (pChar != NULL)
    {
        for (int i = 0; i < 20; i++)
        {
            if (i >= 14 && i < 18)
                continue;

            CSWSItem* pItem;
            CSWCItem* pCItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
                pChar->GetEquippedItemID(1 << i));
            if (!pCItem)
                continue;

            pItem = pCItem->GetServerItem();
            if (!pItem)
                continue;
            if (pItem->m_bPlot)
                continue;
            if (!CheckFilter(pItem))
                continue;

            CreateItemEntry(nIndex, aControls, pItem->m_objectId, 1, 0);
        }
    }

    // Carryable items next, grouped by the base item's m_nStorePanelSort byte so equal-stacking rows land
    // together. Two passes: count the candidates, then walk the groups.
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
                int nSize;
                CSWSItem* pItem = pRepo->ItemListGetItem(i);
                if (!pItem)
                    continue;
                if (pItem->m_bPlot)
                    continue;

                nSize = pItem->GetBaseItem()->m_nStorePanelSort;
                if (nSize == nGroup)
                {
                    if (CheckFilter(pItem))
                        CreateItemEntry(nIndex, aControls, pItem->m_objectId, 1, -1);
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
            m_lblDescription.m_text.m_params.SetText(CExoString(""));
            m_lblOnHandValue.m_text.m_params.SetText(CExoString(""));
        }
    }

    m_bPopulatePending = 0;
#endif
}

#if !KOTOR1  // K2-only: K1's store has no item filter
// Tests an item against the filter.
int CSWGuiStore::CheckFilter(CSWSItem* pItem)
{
    if (!pItem)
        return 0;

    switch (m_nFilter)
    {
    case FILTER_WEAPONS:
        return pItem->GetBaseItem() && (pItem->GetBaseItem()->m_nEquipableSlots
                   & (CSWInventory::SLOT_RIGHTWEAPON | CSWInventory::SLOT_LEFTWEAPON));
    case FILTER_ARMOR:
        return pItem->GetBaseItem() && (pItem->GetBaseItem()->m_nEquipableSlots
                   & (CSWInventory::SLOT_HEAD | CSWInventory::SLOT_BODY | CSWInventory::SLOT_HANDS
                      | CSWInventory::SLOT_LEFTARM | CSWInventory::SLOT_RIGHTARM
                      | CSWInventory::SLOT_IMPLANT | CSWInventory::SLOT_BELT));
    case FILTER_MISC:
        return !pItem->GetBaseItem() || pItem->GetBaseItem()->m_nEquipableSlots == 0;
    case FILTER_ALL:
        break;
    default:
        return 0;
    }
    return 1;
}
#endif

#if KOTOR1
// K1: no equipped-slot argument, and the row also answers one more control event. On a phone the
// row's caption takes the resolution text scale.
int CSWGuiStore::CreateItemEntry(int& nIndex, CExoArrayList<CSWGuiControl*>& aControls,
                                 unsigned long long oidItem, int bInventory)
{
#ifdef K2_UI_TOUCH
    static float s_fScale = g_SDL_mode.h / 768.0f;
#endif
    CExoArrayList<CSWGuiStoreItemEntry*>& aEntries = bInventory ? m_aInvEntries : m_aStoreEntries;

    CSWGuiStoreItemEntry* pEntry;
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
            CSWGuiButton* pProto = static_cast<CSWGuiButton*>(m_lbInvItems.m_pProtoItem->AsButton());
            pEntry->Initialize(pProto->m_text, pProto->m_border, pProto->m_borderHilight,
                               m_lbInvItems.m_innerExtent.w - 2 * m_lbInvItems.m_nPadding);
            pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiStore::OnControlInvAButton));
            pEntry->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiStore::OnControlInvAButton));
        }
        else
        {
            CSWGuiButton* pProto = static_cast<CSWGuiButton*>(m_lbShopItems.m_pProtoItem->AsButton());
            pEntry->Initialize(pProto->m_text, pProto->m_border, pProto->m_borderHilight,
                               m_lbShopItems.m_innerExtent.w - 2 * m_lbShopItems.m_nPadding);
            pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiStore::OnControlStoreAButton));
            pEntry->AddEvent(INPUTEVENT_GUI_START, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiStore::OnControlStoreAButton));
        }
        pEntry->AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiStore::OnControlEntered));
    }

    pEntry->SetItem(oidItem, 0);
    pEntry->SetActive(0);
#ifdef K2_UI_TOUCH  // the phone caption size; pc / xbox keep the 1.3x string default
    int nScreenW, nScreenH;
    if (IosIsIphone(nScreenW, nScreenH) && pEntry->m_text.m_pString != 0)
        pEntry->m_text.m_pString->SetFontScale(s_fScale);
#endif
    aControls.Add(pEntry);
    nIndex++;
    return 0;
}
#else
int CSWGuiStore::CreateItemEntry(int& nIndex, CExoArrayList<CSWGuiControl*>& aControls,
                                 unsigned long long oidItem, int bInventory, int nEquippedBy)
{
    CExoArrayList<CSWGuiStoreItemEntry*>& aEntries = bInventory ? m_aInvEntries : m_aStoreEntries;

    CSWGuiStoreItemEntry* pEntry;
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
            CSWGuiButton* pProto = static_cast<CSWGuiButton*>(m_lbInvItems.m_pProtoItem->AsButton());
            pEntry->Initialize(pProto->m_text, pProto->m_border, pProto->m_borderHilight,
                               m_lbInvItems.m_innerExtent.w - 2 * m_lbInvItems.m_nPadding);
            pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiStore::OnControlInvAButton));
        }
        else
        {
            CSWGuiButton* pProto = static_cast<CSWGuiButton*>(m_lbShopItems.m_pProtoItem->AsButton());
            pEntry->Initialize(pProto->m_text, pProto->m_border, pProto->m_borderHilight,
                               m_lbShopItems.m_innerExtent.w - 2 * m_lbShopItems.m_nPadding);
            pEntry->AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiStore::OnControlStoreAButton));
        }
        pEntry->AddEvent(SWGUIEVENT_FOCUS, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiStore::OnControlEntered));
    }

    pEntry->SetItem(oidItem, 0, nEquippedBy);
    pEntry->SetActive(0);
    aControls.Add(pEntry);
    nIndex++;
    return 0;
}
#endif

void CSWGuiStore::PopulateStoreItemListBox()
{
#if KOTOR1
    // K1 has no valid-item pre-count, so the walk runs until it has placed as many rows as the
    // repository holds: a null or plot item in a store never lets it finish.
    CSWSStore* pStore = g_pAppManager->m_pServerExoApp->GetStoreByGameObjectID(m_oidStore);
    CItemRepository* pRepo = pStore->m_pItemRepository;

    CExoArrayList<CSWGuiControl*> aControls;
    int nIndex = 0;

    int nGroup = 0;
    unsigned nDone = 0;
#ifdef K2_FIX_ENGINE_BUGS
    unsigned nValid = 0;
    for (unsigned i = 0; i < pRepo->ItemListGetCount(); i++)
    {
        CSWSItem* pItem = pRepo->ItemListGetItem(i);
        if (pItem != NULL && !pItem->m_bPlot)
            nValid++;
    }
    while (nDone < nValid)
#else
    while (nDone < pRepo->ItemListGetCount())
#endif
    {
        int nNext = nGroup;
        for (unsigned i = 0; i < pRepo->ItemListGetCount(); i++)
        {
            int nSize;
            CSWSItem* pItem = pRepo->ItemListGetItem(i);
            if (!pItem)
                continue;
            if (pItem->m_bPlot)
                continue;

            nSize = pItem->GetBaseItem()->m_nStorePanelSort;
            if (nSize == nGroup)
            {
                CreateItemEntry(nIndex, aControls, pItem->m_objectId, 0);
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
            m_lblDescription.m_text.m_params.SetText(CExoString(""));
        }
    }

    m_bPopulatePending = 0;
#else
    CSWSStore* pStore = g_pAppManager->m_pServerExoApp->GetStoreByGameObjectID(m_oidStore);
    CItemRepository* pRepo = pStore->m_pItemRepository;

    CExoArrayList<CSWGuiControl*> aControls;
    m_lbShopItems.ClearItems();
    int nIndex = 0;

    unsigned nValid = pRepo->ItemListGetCount();
    for (unsigned i = 0; i < pRepo->ItemListGetCount(); i++)
    {
        CSWSItem* pItem = pRepo->ItemListGetItem(i);
        if (pItem == NULL || pItem->m_bPlot)
            nValid--;
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
                int nSize;
                CSWSItem* pItem = pRepo->ItemListGetItem(i);
                if (!pItem)
                    continue;
                if (pItem->m_bPlot)
                    continue;

                nSize = pItem->GetBaseItem()->m_nStorePanelSort;
                if (nSize == nGroup)
                {
                    if (CheckFilter(pItem))
                        CreateItemEntry(nIndex, aControls, pItem->m_objectId, 0, -1);
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
            m_lblDescription.m_text.m_params.SetText(CExoString(""));
            m_lblOnHandValue.m_text.m_params.SetText(CExoString(""));
        }
    }

    m_bPopulatePending = 0;
#endif
}

// The inventory side's accept handler. An equipped item opens the confirm-sale box, an item in a
// slot the appearance's EquipSlotsLocked mask covers opens the cannot-sell box, and everything else
// runs the level-gated price check and then sells outright.
void CSWGuiStore::OnControlInvAButton(CSWGuiControl* pControl)
{
#if KOTOR1
    // K1 has no equipped-item or locked-slot boxes and no message colours; only the level cap.
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    if (pControl->m_tail.nCurrentEventType == INPUTEVENT_GUI_START)
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);

    if (pControl == &m_btnAccept)
    {
        pControl = m_lbInvItems.GetSelectedControl();
        if (pControl == NULL)
            return;
    }

    CSWGuiStoreItemEntry* pEntry = static_cast<CSWGuiStoreItemEntry*>(pControl);
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pEntry->m_oidItem));
    if (!pItem)
        return;

    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    int nLimit = 250;
    if (pCreature != NULL)
    {
        int nLevelLimit = pCreature->m_pStats->GetLevel(0) * 50;
        if (nLevelLimit < 250)
            nLimit = nLevelLimit;
    }

    if (static_cast<unsigned int>(GetItemSellValue(pItem)) > static_cast<unsigned int>(nLimit))
    {
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        CExoString sMessage;
        g_pTlkTable->SetCustomToken(0, CExoString(nLimit));

        sMessage = g_pSWTlkTable->Fetch(STRREF_CONFIRM_SELL_ABOVE_PRICE, 0, 0, 0);
        pBox->SetAllowCancel(1);
        pBox->SetMessage(sMessage);
        pBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiStore::SellItemCallback), pControl);
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
        return;
    }

    SellItem(pItem);
#else
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    if (pControl == &m_btnAccept)
    {
        pControl = m_lbInvItems.GetSelectedControl();
        if (pControl == NULL)
            return;
    }

    CSWGuiStoreItemEntry* pEntry = static_cast<CSWGuiStoreItemEntry*>(pControl);
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pEntry->m_oidItem));
    if (!pItem)
        return;

    // 50 credits per character level, capped at 250.
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    int nLimit;
    if (pCreature == NULL || pCreature->m_pStats->GetLevel(0) * 50 > 250)
        nLimit = 250;
    else
        nLimit = pCreature->m_pStats->GetLevel(0) * 50;

    int nSlot = pEntry->m_nEquippedBy;
    if (nSlot != -1)
    {
        CSWCCreature* pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(nSlot);
        if (pChar != NULL)
        {
            int nLocked = 0;
            g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
                pChar->m_pCreatureAppearance->m_nAppearanceId, CExoString("EquipSlotsLocked"), &nLocked);
            if (nLocked != 0
                && (pChar->GetServerCreature()->m_pInventory->GetSlotFromItem(pItem) & nLocked) != 0)
            {
                CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
                pBox->SetMessageColors(CGuiInGame::COLOR_DARK_GOLD, CGuiInGame::COLOR_NEW_HIGHLIGHT);
                pBox->SetAllowCancel(0);
                pBox->SetMessage(STRREF_EQUIPMENT_CANNOT_BE_SOLD);
                pBox->SetCallback(0, 0, 0);
                g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
                return;
            }
        }

        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        CExoString sMessage = g_pSWTlkTable->Fetch(STRREF_CONFIRM_SELL_EQUIPPED, 0, 0, 0);
        pBox->SetMessageColors(CGuiInGame::COLOR_DARK_GOLD, CGuiInGame::COLOR_NEW_HIGHLIGHT);
        pBox->SetAllowCancel(1);
        pBox->SetMessage(sMessage);
        pBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiStore::SellItemCallback), pControl);
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
        return;
    }

    if (static_cast<unsigned int>(GetItemSellValue(pItem)) > static_cast<unsigned int>(nLimit))
    {
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        CExoString sMessage;
        g_pTlkTable->SetCustomToken(0, CExoString(nLimit));

        sMessage = g_pSWTlkTable->Fetch(STRREF_CONFIRM_SELL_ABOVE_PRICE, 0, 0, 0);
        pBox->SetMessageColors(CGuiInGame::COLOR_DARK_GOLD, CGuiInGame::COLOR_NEW_HIGHLIGHT);
        pBox->SetAllowCancel(1);
        pBox->SetMessage(sMessage);
        pBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiStore::SellItemCallback), pControl);
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
        return;
    }

    // The item is not equipped: clear anything still pointing at it, then sell it outright.
    nSlot = pEntry->m_nEquippedBy;
    if (nSlot != -1)
    {
        CSWCCreature* pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(nSlot);
        if (pChar != NULL)
        {
            // The four unequippable "creature weapon" slots 14..17, as in
            // PopulateInventoryItemListBox's equipped-item walk.
            for (int i = 0; i < 20; i++)
            {
                if (i >= 14 && i < 18)
                    continue;

                unsigned int nMask = 1 << i;
                if (pChar->GetEquippedItemID(nMask) == pItem->GetClientItem()->m_objectId)
                    pChar->SetEquippedItemID(nMask, OBJECT_INVALID);
            }

            CSWSCreature* pServer = pChar->GetServerCreature();
            if (pServer != NULL)
                pServer->RunUnequip(pItem->m_objectId, OBJECT_INVALID, 0, 0, 1, 1);
        }
    }

    SellItem(pItem);
#endif
}

// Handles the accept button and a shop-row activation, including the store's two refusal paths: not
// enough credits, and above the level-gated price cap (whose cap arrives as custom token 0).
void CSWGuiStore::OnControlStoreAButton(CSWGuiControl* pControl)
{
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    if (pControl->m_tail.nCurrentEventType != 0)
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);

    if (pControl == &m_btnAccept)
    {
        pControl = m_lbShopItems.GetSelectedControl();
        if (pControl == NULL)
            return;
    }

    CSWGuiStoreItemEntry* pEntry = static_cast<CSWGuiStoreItemEntry*>(pControl);
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pEntry->m_oidItem));
    if (!pItem)
        return;

    if (static_cast<unsigned int>(m_nGold) < static_cast<unsigned int>(GetItemBuyValue(pItem)))
    {
#if KOTOR1  // K1 shows the refusal in the examine box, and no box takes message colours
        g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowExamineBox(STRREF_NOT_ENOUGH_CREDITS, 1);
        return;
#else
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetMessageColors(CGuiInGame::COLOR_DARK_GOLD, CGuiInGame::COLOR_NEW_HIGHLIGHT);
        pBox->SetAllowCancel(0);

        pBox->SetMessage(g_pSWTlkTable->Fetch(STRREF_NOT_ENOUGH_CREDITS, 0, 0, 0));
        pBox->SetCallback(0, 0, 0);
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
        return;
#endif
    }

    // The level cap is 50 credits per character level; the box shows it via custom token 0.
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    int nLimit = 50;
    if (pCreature != NULL)
        nLimit = pCreature->m_pStats->GetLevel(0) * 50;

    if (static_cast<unsigned int>(GetItemBuyValue(pItem)) > static_cast<unsigned int>(nLimit))
    {
        CSWGuiMessageBox* pBox = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        CExoString sMessage;
        g_pTlkTable->SetCustomToken(0, CExoString(nLimit));

        sMessage = g_pSWTlkTable->Fetch(STRREF_CONFIRM_BUY_ABOVE_PRICE, 0, 0, 0);
#if !KOTOR1
        pBox->SetMessageColors(CGuiInGame::COLOR_DARK_GOLD, CGuiInGame::COLOR_NEW_HIGHLIGHT);
#endif
        pBox->SetAllowCancel(1);
        pBox->SetMessage(sMessage);
        pBox->SetCallback(this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiStore::BuyItemCallback), pControl);
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
        return;
    }

    BuyItem(pItem);
}

// Handles a hover by filling the cost, stock and on-hand readouts for the row under the cursor,
// then previews the item's property description in the description box. With the inventory list
// visible the panel is in sell mode and the readouts come off the store's mark-down, otherwise off
// its mark-up. The 4 bit of m_nFlagsLow marks a readout that is a string reference rather than a
// number.
void CSWGuiStore::OnControlEntered(CSWGuiControl* pControl)
{
#if KOTOR1
    // K1 shows the price and the item's own stack as the stock; there is no store stock count, no
    // on-hand count and no token parse.
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWGuiStoreItemEntry* pEntry = static_cast<CSWGuiStoreItemEntry*>(pControl);
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pEntry->m_oidItem));
    if (!pItem)
        return;

    if (!m_lbShopItems.m_tail.bVisible)
        m_lblCostValue.m_text.m_params.SetText(CExoString(GetItemSellValue(pItem)));
    else
        m_lblCostValue.m_text.m_params.SetText(CExoString(GetItemBuyValue(pItem)));

    if ((pItem->m_nFlagsLow & 4) != 0)
        m_lblStockValue.m_text.m_params.SetStrRef(STRREF_INFINITE);
    else
        m_lblStockValue.m_text.m_params.SetText(CExoString(pItem->m_stackCount));

    SetDescription(pItem->GetPropertyDescription());
#else
    CSWSStore* pStore = g_pAppManager->m_pServerExoApp->GetStoreByGameObjectID(m_oidStore);
    CSWSCreature* pPlayer = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidPlayer);

    CItemRepository* pStoreRepo = NULL;
    if (pStore != NULL)
        pStoreRepo = pStore->m_pItemRepository;

    if (pPlayer == NULL)
        return;

    CItemRepository* pPlayerRepo = pPlayer->GetItemRepository(1);

    if (pStore == NULL)
        return;
    if (pControl->m_tail.nCurrentEventArg == 0)
        return;

    CSWGuiStoreItemEntry* pEntry = static_cast<CSWGuiStoreItemEntry*>(pControl);
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pEntry->m_oidItem));
    if (!pItem)
        return;

    if (m_lbInvItems.m_tail.bVisible)
    {
        int nCost = GetItemSellValue(pItem);
        m_lblCostValue.m_text.m_params.SetText(CExoString(nCost));

        int nStock = pStoreRepo->GetMatchingItemCount(pItem);
        if (nStock == -1)
            m_lblStockValue.m_text.m_params.SetStrRef(STRREF_INFINITE);
        else
            m_lblStockValue.m_text.m_params.SetText(CExoString(nStock));

        if ((pItem->m_nFlagsLow & 4) == 0)
        {
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

            // The copy being sold is still in the box, so it is not counted on hand twice.
            if (pEntry->m_nEquippedBy != -1)
                nEquipped = nEquipped + pPlayerRepo->GetMatchingItemCount(pItem) - 1;

            m_lblOnHandValue.m_text.m_params.SetText(CExoString(pItem->m_stackCount + nEquipped));
        }
        else
            m_lblOnHandValue.m_text.m_params.SetStrRef(STRREF_INFINITE);
    }
    else
    {
        int nCost = GetItemBuyValue(pItem);
        m_lblCostValue.m_text.m_params.SetText(CExoString(nCost));

        if ((pItem->m_nFlagsLow & 4) != 0)
            m_lblStockValue.m_text.m_params.SetStrRef(STRREF_INFINITE);
        else
            m_lblStockValue.m_text.m_params.SetText(CExoString(pItem->m_stackCount));

        int nOnHand = pPlayerRepo->GetMatchingItemCount(pItem);
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

        if (nOnHand == -1)
            m_lblOnHandValue.m_text.m_params.SetStrRef(STRREF_INFINITE);
        else
            m_lblOnHandValue.m_text.m_params.SetText(CExoString(nEquipped + nOnHand));
    }

    {
        CExoString sDescription = pItem->GetPropertyDescription();
        if (sDescription.Find('<', 0) != -1 || sDescription.Find('{', 0) != -1)
        {
            g_pSWTlkTable->m_pSpeakerObj = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
            g_pSWTlkTable->m_gender = g_pSWTlkTable->GetObjectGender();
            g_pSWTlkTable->m_bSuppressEnabled |= 1;
            g_pSWTlkTable->ParseStr(sDescription);
        }
        SetDescription(sDescription);
    }
#endif
}

// Three control events close the store, one toggles buy and sell, three more move the filter, and
// two scroll the description.
void CSWGuiStore::HandleInputEvent(int type, int val)
{
#if KOTOR1
    // K1 has no filter keys, and it handles only two of the three close events.
    CSWGuiPanel::HandleInputEvent(type, val);
    if (val == 0)
        return;

    switch (type)
    {
    case INPUTEVENT_GUI_B:
    case INPUTEVENT_GUI_BACK:
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideStoreGui();
        break;

    case INPUTEVENT_GUI_X:
        {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            CSWSStore* pStore = g_pAppManager->m_pServerExoApp->GetStoreByGameObjectID(m_oidStore);
            if (!pStore)
                break;
            if (pStore->m_buySellFlag != CSWSStore::STORE_BUY_AND_SELL)
                break;

            if (m_lbShopItems.m_tail.bVisible)
                ShowSellGUI();
            else
                ShowBuyGUI();
        }
        break;

    case INPUTEVENT_GUI_SCROLL_UP:
        m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
        break;

    case INPUTEVENT_GUI_SCROLL_DOWN:
        m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
        break;
    }
#else
    CSWGuiPanel::HandleInputEvent(type, val);
    if (val == 0)
        return;

    switch (type)
    {
    case INPUTEVENT_GUI_B:
    case INPUTEVENT_GUI_START:
    case INPUTEVENT_GUI_BACK:
        m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->HideStoreGui();
        break;

    case INPUTEVENT_GUI_X:
        {
            m_pGuiManager->PlayGuiSound(GUISOUND_CLICKED_DEFAULT);
            CSWSStore* pStore = g_pAppManager->m_pServerExoApp->GetStoreByGameObjectID(m_oidStore);
            if (!pStore)
                break;
            if (pStore->m_buySellFlag != CSWSStore::STORE_BUY_AND_SELL)
                break;

            if (m_lbShopItems.m_tail.bVisible)
                ShowSellGUI();
            else
                ShowBuyGUI();
        }
        break;

    case INPUTEVENT_GUI_Y:
        m_nFilter = (m_nFilter + 1) & 3;
        UpdateFilters();
        break;

    case INPUTEVENT_GUI_DPAD_LEFT:
        if (m_nFilter)
            m_nFilter = m_nFilter - 1;
        UpdateFilters();
        break;

    case INPUTEVENT_GUI_DPAD_RIGHT:
        {
            unsigned char nNext = m_nFilter + 1;
            m_nFilter = nNext < 3 ? nNext : 3;
        }
        UpdateFilters();
        break;

    case INPUTEVENT_GUI_SCROLL_UP:
        m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_UP, 1);
        break;

    case INPUTEVENT_GUI_SCROLL_DOWN:
        m_lbDescription.HandleInputEvent(INPUTEVENT_GUI_DPAD_DOWN, 1);
        break;
    }
#endif
}

void CSWGuiStore::SellItemCallback(CSWGuiControl* pControl)
{
#if KOTOR1
    // K1 has no equipped-slot unequip; K1 rows carry no slot.
    if (!g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted)
        return;

    CSWGuiStoreItemEntry* pEntry = static_cast<CSWGuiStoreItemEntry*>(pControl);
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pEntry->m_oidItem));
    if (!pItem)
        return;

    SellItem(pItem);
#else
    if (!g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox->m_bAccepted)
        return;

    CSWGuiStoreItemEntry* pEntry = static_cast<CSWGuiStoreItemEntry*>(pControl);
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pEntry->m_oidItem));
    if (!pItem)
        return;

    int nSlot = pEntry->m_nEquippedBy;
    if (nSlot != -1)
    {
        CSWCCreature* pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(nSlot);
        if (pChar != NULL)
        {
            CSWSCreature* pCreature = pChar->GetServerCreature();
            if (pCreature != NULL)
            {
                pCreature->RunUnequip(pItem->m_objectId, OBJECT_INVALID, 0, 0, 1, 1);
            }
            m_bRepopulateInventory = 1;
        }
    }

    SellItem(pItem);
#endif
}

int CSWGuiStore::GetItemSellValue(CSWSItem* pItem)
{
    CSWSStore* pStore = g_pAppManager->m_pServerExoApp->GetStoreByGameObjectID(m_oidStore);
    int nValue = 0;
    if (pItem != NULL && pStore != NULL)
        nValue = pItem->GetCost() * (pStore->m_nMarkDown + pStore->m_nBonusMarkDown) / 100;
    return nValue;
}

// The deferred-populate flag (m_bRepopulateInventory) makes the sell path rebuild the inventory once:
// SellItemCallback sets it when it changed an equipped slot, and the rebuild here is skipped.
void CSWGuiStore::SellItem(CSWSItem* pItem)
{
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
        g_pAppManager->m_pServerExoApp->GetPlayerCreatureId());
    CSWSStore* pStore = g_pAppManager->m_pServerExoApp->GetStoreByGameObjectID(m_oidStore);

    if (pItem == NULL || pCreature == NULL || pStore == NULL)
        return;

    CItemRepository* pRepo = pCreature->GetItemRepository(1);
    if (!pRepo)
        return;

    m_pGuiManager->PlayGuiSound(static_cast<char>(GUISOUND_INVENTORY_DROP));

    int nPrice = 0;
    CSWSStore* pPriceStore = g_pAppManager->m_pServerExoApp->GetStoreByGameObjectID(m_oidStore);
    if (pPriceStore != NULL)
        nPrice = pItem->GetCost() * (pPriceStore->m_nMarkDown + pPriceStore->m_nBonusMarkDown) / 100;

    m_nGold = m_nGold + nPrice;
    pCreature->SetGold(m_nGold);

#if KOTOR1  // K1 makes one AcquireItem call, with no flag, and always rebuilds
    if (pItem->m_stackCount >= 2)
        pStore->AcquireItem(pItem->SplitItem(1));
    else
    {
        pRepo->RemoveItem(pItem);
        pStore->AcquireItem(pItem);
    }

    short nSelected = m_lbInvItems.m_nSelectedIndex;
    short nTopRow = m_lbInvItems.m_nTopRow;
    PopulateInventoryItemListBox();
#else
    if (pItem->m_stackCount >= 2)
        pStore->AcquireItem(pItem->SplitItem(1), 0);
    else
    {
        pRepo->RemoveItem(pItem);
        pStore->AcquireItem(pItem, 0);
    }

    short nSelected = m_lbInvItems.m_nSelectedIndex;
    short nTopRow = m_lbInvItems.m_nTopRow;
    if (!m_bRepopulateInventory)
        PopulateInventoryItemListBox();
#endif

#if KOTOR1
    int nControls = m_lbInvItems.m_aControls.m_count;
    int nSelect = nControls > nSelected ? nSelected : nControls - 1;
    int nMaxTop = nControls - m_lbInvItems.m_nVisibleRows;
    if (nMaxTop < 0)
        nMaxTop = 0;
    m_lbInvItems.m_nTopRow = static_cast<short>(nMaxTop > nTopRow ? nTopRow : nMaxTop);
    m_lbInvItems.SetSelectedControl(nSelect, 0);
#else
    int nControls = m_lbInvItems.m_aControls.m_count;
    int nSelect = nControls > nSelected ? nSelected : nControls - 1;
    int nVisible = m_lbInvItems.m_nVisibleRows;
    int nMaxTop = nControls < nVisible ? 0 : nControls - nVisible;
    m_lbInvItems.m_nTopRow = static_cast<short>(nMaxTop > nTopRow ? nTopRow : nMaxTop);
    m_lbInvItems.SetSelectedControl(nSelect, 0);
#endif

    m_lblCreditsValue.m_text.m_params.SetText(CExoString(m_nGold));
}

int CSWGuiStore::GetItemBuyValue(CSWSItem* pItem)
{
    CSWSStore* pStore = g_pAppManager->m_pServerExoApp->GetStoreByGameObjectID(m_oidStore);
    int nValue = 0;
    if (pItem != NULL && pStore != NULL)
        nValue = pItem->GetCost() * (pStore->m_nMarkUp + pStore->m_nBonusMarkUp) / 100;
    return nValue;
}

void CSWGuiStore::BuyItemCallback(CSWGuiControl* pControl)
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

void CSWGuiStore::BuyItem(CSWSItem* pItem)
{
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
        g_pAppManager->m_pServerExoApp->GetPlayerCreatureId());
    CSWSStore* pStore = g_pAppManager->m_pServerExoApp->GetStoreByGameObjectID(m_oidStore);

    if (pCreature == NULL || pStore == NULL)
        return;

    CItemRepository* pRepo = pCreature->GetItemRepository(1);
    if (pItem == NULL || pRepo == NULL)
        return;

    m_pGuiManager->PlayGuiSound(static_cast<char>(9));

    int nPrice = 0;
    CSWSStore* pPriceStore = g_pAppManager->m_pServerExoApp->GetStoreByGameObjectID(m_oidStore);
    if (pPriceStore != NULL)
        nPrice = pItem->GetCost() * (pPriceStore->m_nMarkUp + pPriceStore->m_nBonusMarkUp) / 100;

    m_nGold = m_nGold - nPrice;
    pCreature->SetGold(m_nGold);

    pStore->SellItem(pItem, pCreature);

#if KOTOR1  // K1 restores the selection too, as on the sell side
    short nSelected = m_lbShopItems.m_nSelectedIndex;
    short nTopRow = m_lbShopItems.m_nTopRow;
    PopulateStoreItemListBox();

    int nControls = m_lbShopItems.m_aControls.m_count;
    int nSelect = nControls > nSelected ? nSelected : nControls - 1;
    int nMaxTop = nControls - m_lbShopItems.m_nVisibleRows;
    if (nMaxTop < 0)
        nMaxTop = 0;
    m_lbShopItems.m_nTopRow = static_cast<short>(nMaxTop > nTopRow ? nTopRow : nMaxTop);
    m_lbShopItems.SetSelectedControl(nSelect, 0);
#else
    short nTopRow = m_lbShopItems.m_nTopRow;
    PopulateStoreItemListBox();

    int nControls = m_lbShopItems.m_aControls.m_count;
    int nVisible = m_lbShopItems.m_nVisibleRows;
    int nMaxTop = nControls < nVisible ? 0 : nControls - nVisible;
    m_lbShopItems.m_nTopRow = static_cast<short>(nMaxTop > nTopRow ? nTopRow : nMaxTop);
#endif

    m_lblCreditsValue.m_text.m_params.SetText(CExoString(m_nGold));
}

void CSWGuiStore::ShowSellGUI()
{
    m_lbInvItems.m_tail.bVisible = 1;
    m_lbShopItems.m_tail.bVisible = 0;
#if KOTOR1  // K1 captions the accept button with the buy/sell title itself
    m_lblAcceptCaption.m_text.m_params.SetStrRef(STRREF_SELLING_ITEMS);
    m_btnAccept.m_text.m_params.SetStrRef(STRREF_SELLING_ITEMS);
#else
    m_lblAcceptCaption.m_text.m_params.SetStrRef(STRREF_SELL_ITEM);
    m_btnAccept.m_text.m_params.SetStrRef(STRREF_SELL_ITEM);
#endif
    m_btnAccept.AddEvent(INPUTEVENT_GUI_A, this, static_cast<void (CSWGuiObject::*)(CSWGuiControl*)>(&CSWGuiStore::OnControlInvAButton));
    m_lblBuySell.m_text.m_params.SetStrRef(STRREF_SELLING_ITEMS);
    m_lblExamineCaption.m_text.m_params.SetStrRef(STRREF_SHOW_BUY_LIST);
    m_btnExamine.m_text.m_params.SetStrRef(STRREF_SHOW_BUY_LIST);
    m_lblCost.m_text.m_params.SetStrRef(STRREF_ITEM_VALUE);
    SetActiveControl(&m_lbInvItems, 0);
    PopulateInventoryItemListBox();
    AspyrAddButtonIcon(&m_btnAccept, 'a');
    AspyrAddButtonIcon(&m_btnExamine, 'x');
}

// Points the description list box's single row at a new string and re-fits it: the row keeps its own
// rectangle and only its height is recomputed from the new text.
void CSWGuiStore::SetDescription(const CExoString& sDescription)
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

    // The row grows the box only when it is taller than it, and never over a box with unequal rows.
    m_lblDescriptionScroll.m_tail.bVisible =
        m_lbDescription.m_nRowHeight + m_lbDescription.m_nPadding > m_lbDescription.m_innerExtent.h
        && m_lbDescription.m_aControls.m_count > 0
        && !m_lbDescription.bUnequalRows;
}

#if !KOTOR1  // K1 does not override this slot
void CSWGuiStore::Draw(float f)
{
    if (m_bRepopulateInventory)
    {
        PopulateInventoryItemListBox();
        m_bRepopulateInventory = 0;
    }
    CSWGuiPanel::Draw(f);
}
#endif
