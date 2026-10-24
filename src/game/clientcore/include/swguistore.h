// The merchant buy/sell panel. It owns the CSWGuiStoreItemEntry rows and the item list boxes they
// live in.

#ifndef CSWGUISTORE_H
#define CSWGUISTORE_H

#include "swgui.h"
#include "swguicontrol.h"
#include "CExoArrayListDtors.h"

class CExoString;
class CSWGuiStoreItemEntry;
class CSWSItem;
class CSWGuiManager;

class CSWGuiStore : public CSWGuiPanel {
public:
    CSWGuiStore(CSWGuiManager* pGuiManager);
    virtual ~CSWGuiStore();

#if !KOTOR1  // K1 does not override this slot
    virtual void Draw(float f);
#endif
    virtual void HandleInputEvent(int type, int val);
    virtual void OnPanelAdded();
    virtual void OnPanelRemoved();

    void OnControlXButton(CSWGuiControl* pControl);
#if !KOTOR1  // K1's store has no item filter
    void OnFilterPressed(CSWGuiControl* pControl);
#endif
    void OnControlStoreAButton(CSWGuiControl* pControl);
    void OnControlInvAButton(CSWGuiControl* pControl);
    void OnControlEntered(CSWGuiControl* pControl);

#if !KOTOR1
    void UpdateFilters();
#endif
    void DeleteItemEntries();
    void InitializeStoreType();
    void PopulateStore();
    void ShowBuyGUI();
    void ShowSellGUI();
    void PopulateInventoryItemListBox();
    void PopulateStoreItemListBox();
#if !KOTOR1
    int  CheckFilter(CSWSItem* pItem);
#endif
    void SetDescription(const CExoString& sDescription);
    int  GetItemSellValue(CSWSItem* pItem);
    int  GetItemBuyValue(CSWSItem* pItem);
    void SellItemCallback(CSWGuiControl* pControl);
    void BuyItemCallback(CSWGuiControl* pControl);
    void SellItem(CSWSItem* pItem);
    void BuyItem(CSWSItem* pItem);
#if KOTOR1
    int CreateItemEntry(int& nIndex, CExoArrayList<CSWGuiControl*>& aControls,
                         unsigned long long oidItem, int bInventory);
#else
    int CreateItemEntry(int& nIndex, CExoArrayList<CSWGuiControl*>& aControls,
                         unsigned long long oidItem, int bInventory, int nEquippedBy);
#endif

    int m_nReserved1;
    CExoArrayList<CSWGuiStoreItemEntry*> m_aStoreEntries;
    CExoArrayList<CSWGuiStoreItemEntry*> m_aInvEntries;

    CSWGuiLabel m_lblReserved4;
    CSWGuiLabel m_lblAcceptCaption;
    CSWGuiLabel m_lblReserved1;
    CSWGuiLabel m_lblReserved2;
    CSWGuiLabel m_lblReserved3;
    CSWGuiLabel m_lblExamineCaption;
    CSWGuiLabel m_lblExamineIcon;
    CSWGuiLabel m_lblDescription;
    CSWGuiLabel m_lblCost;
    CSWGuiLabel m_lblCostValue;
    CSWGuiLabel m_lblStock;
    CSWGuiLabel m_lblStockValue;
    CSWGuiLabel m_lblDescriptionScroll;
    CSWGuiLabel m_lblCredits;
    CSWGuiLabel m_lblCreditsValue;
    CSWGuiLabel m_lblBuySell;

    CSWGuiListBox m_lbShopItems;
    CSWGuiListBox m_lbInvItems;
    CSWGuiListBox m_lbDescription;

    CSWGuiButton m_btnCancel;
    CSWGuiButton m_btnExamine;
    CSWGuiButton m_btnAccept;

#if !KOTOR1  // K1 has no filter bar or on-hand readout
    CSWGuiLabel m_aLabelBar[5];
    CSWGuiLabel m_lblFilter;
    CSWGuiLabel m_lblOnHand;
    CSWGuiLabel m_lblOnHandValue;

    CSWGuiButton m_aFilterButtons[4];
#endif

    unsigned long long m_nReserved2;
    int m_nGold;
    int m_bPopulatePending;
    unsigned long long m_oidStore;
    unsigned long long m_oidPlayer;
#if !KOTOR1
    // m_nFilter: the m_aFilterButtons order (BTN_ALL, BTN_WEAPONS, BTN_ARMOR, BTN_MISC).
    enum {
        FILTER_ALL = 0,
        FILTER_WEAPONS = 1,
        FILTER_ARMOR = 2,
        FILTER_MISC = 3,
    };
    unsigned char m_nFilter;
    unsigned char m_bRepopulateInventory;
#endif
};

#if !KOTOR1  // K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiStore) == 0x47e8, "CSWGuiStore size drift");
#else
K2_LAYOUT_ASSERT(sizeof(CSWGuiStore) == 0x2de0, "CSWGuiStore K1 size drift");
#endif

#endif
