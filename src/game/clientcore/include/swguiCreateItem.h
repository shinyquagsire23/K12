#pragma once

#ifndef CSWGUICREATEITEM_H
#define CSWGUICREATEITEM_H

#include "swgui.h"
#include "swguicontrol.h"
#include "CExoArrayListDtors.h"
#include "exostring.h"

class CSWGuiControl;
class CSWGuiManager;
class CSWGuiStoreItemEntry;
class CSWGuiUpgradeSelection;
class CSWSCreature;
class CSWSItem;
class CSWSStore;

// The item creation and breakdown workbench panel. It shares the store panel's .gui control names
// and its CSWGuiStoreItemEntry row type, but the currency here is the player's item components, and
// the panel owns a temporary CSWSStore holding the items the player builds.
class CSWGuiCreateItem : public CSWGuiPanel {
public:
    // m_nCreateFilter: the BTN_CREATE_* button order, which is also the recipe table's "group".
    enum {
        CREATE_FILTER_ARMOR = 0,
        CREATE_FILTER_RANGED = 1,
        CREATE_FILTER_MELEE = 2,
        CREATE_FILTER_LIGHTSABER = 3,
        CREATE_FILTER_MISC = 4,
    };
    // m_nBreakdownFilter: the BTN_BREAK_* button order.
    enum {
        BREAKDOWN_FILTER_ALL = 0,
        BREAKDOWN_FILTER_WEAPONS = 1,
        BREAKDOWN_FILTER_ARMOR = 2,
        BREAKDOWN_FILTER_USEABLE = 3,
        BREAKDOWN_FILTER_MISC = 4,
    };

    // oidOwner is the creature whose sheet is shown, or the invalid-object id for the player
    // character.
    CSWGuiCreateItem(CSWGuiManager* pGuiManager, unsigned long long oidOwner);
    virtual ~CSWGuiCreateItem();

    virtual void HandleInputEvent(int type, int val);
    virtual void OnPanelAdded();
    virtual void OnPanelRemoved();

    void CloseCreateItem(CSWGuiControl* pControl);
    void OnControlXButton(CSWGuiControl* pControl);
    void OnCreateFilterPicked(CSWGuiControl* pControl);
    void OnBreakdownFilterPicked(CSWGuiControl* pControl);
    void OnAcceptPicked(CSWGuiControl* pControl);
    void OnControlStoreAButton(CSWGuiControl* pControl);
    void OnControlInvAButton(CSWGuiControl* pControl);
    void OnControlEntered(CSWGuiControl* pControl);
    void DeleteItemEntries();
    // Callers reach these through their own symbol, so they stay out of line.
    __attribute__((noinline)) void UpdateCreateFilters();
    __attribute__((noinline)) void UpdateBreakdownFilters();
    void SetItemList();
    void InitializeStoreType();
    void PopulateStore();
    void ShowBuyGUI();
    void ShowSellGUI();
    void PopulateInventoryItemListBox();
    void PopulateStoreItemListBox();
    int  CheckBreakdownFilter(CSWSItem* pItem, CSWSCreature* pCreature, int nUnused);
    int CreateItemEntry(int& nIndex, CExoArrayList<CSWGuiControl*>& aControls,
                        unsigned long long oidItem, int bInventory, CExoString sTag);
    int  GetItemSellValue(CSWSItem* pItem);
    int  GetItemBuyValue(CSWSItem* pItem);
    void SellItemCallback(CSWGuiControl* pControl);
    void BuyItemCallback(CSWGuiControl* pControl);
    void SellItem(CSWSItem* pItem);
    void BuyItem(CSWSItem* pItem);
    void SetDescription(const CExoString& sDescription);

    int m_nReserved1;
    CExoArrayList<CSWGuiStoreItemEntry*> m_aStoreEntries;
    CExoArrayList<CSWGuiStoreItemEntry*> m_aInvEntries;

    // The first eight sheet labels are bound by name in the .gui only.
    CSWGuiLabel m_lblA;
    CSWGuiLabel m_lblAccept;
    CSWGuiLabel m_lblB;
    CSWGuiLabel m_lblCancel;
    CSWGuiLabel m_lblReserved1;
    CSWGuiLabel m_lblExamine;    // toggled visible by InitializeStoreType
    CSWGuiLabel m_lblX;    // toggled visible by InitializeStoreType
    CSWGuiLabel m_lblDescriptionText;    // the row label SetDescription drives

    CSWGuiLabel m_lblCost;
    CSWGuiLabel m_lblCostValue;
    CSWGuiLabel m_lblStock;
    CSWGuiLabel m_lblStockValue;

    CSWGuiButton m_btnAccept;
    CSWGuiButton m_btnCancel;
    CSWGuiButton m_btnExamine;
    CSWGuiButton m_aCreateButtons[5];
    CSWGuiButton m_aBreakButtons[5];

    CSWGuiLabel m_aBarLabels[3];
    CSWGuiLabel m_lblTitle;
    CSWGuiLabel m_lblTitle2;
    CSWGuiLabel m_lblRelatedSkill;

    CSWGuiListBox m_lbDescriptionInv;
    CSWGuiLabel m_lblRandAnalog;
    CSWGuiLabel m_lblCredits;
    CSWGuiLabel m_lblCreditsValue;     // shows the component count

    CSWGuiListBox m_lbShopItems;
    CSWGuiListBox m_lbInvItems;
    CSWGuiListBox m_lbDescription;

    unsigned long long m_nReserved2;
    float m_fReserved1;
    int m_nComponent;
    int m_nReserved3;
    unsigned long long m_oidStore;
    unsigned long long m_oidPlayer;
    // The panel's per-side component yield, the skill-derived factor that scales an item's cost
    // when it is sold.
    float m_fBreakdownYield;
    int m_bViewSwitched;
    CSWSStore* m_pStore;             // holds the items the player builds
    int m_nCreateFilter;             // indexes m_aCreateButtons
    int m_bCreateMode;                 // nonzero in shop/buy mode
    int m_nBreakdownFilter;          // indexes m_aBreakButtons
    CSWGuiUpgradeSelection* m_pUpgradeSelection;  // the panel this sheet was opened from
    CExoString m_sOverride2DA;
};

#if !KOTOR1  // K1's GUI base classes are smaller
K2_LAYOUT_ASSERT(sizeof(CSWGuiCreateItem) == 0x55b0, "CSWGuiCreateItem size drift");
#endif

#endif // CSWGUICREATEITEM_H

// The QA "create item" cheat panel, a scrolling list of baseitems.2da categories, where picking one
// opens the per-category sub-menu. One of five sibling debug panels that are near-verbatim copies of
// each other.
#ifndef CSWGUICREATEITEMMENU_H
#define CSWGUICREATEITEMMENU_H


class CExoString;
class CSWGuiButton;
class CSWGuiControl;
class CSWGuiObject;

class CSWGuiCreateItemMenu : public CSWGuiPanel {
public:

    CSWGuiCreateItemMenu(CSWGuiManager* pGuiManager);
    virtual void HandleInputEvent(int type, int val);

    CSWGuiListBox m_listBox;   // the scrolling option list this menu fills
    CSWGuiLabel   m_label;
    // Index into baseitems.2da. This menu wraps inclusively at the row count, where its siblings
    // wrap at count - 1.
    int  m_nSelectedItemType;

    // Builds one menu row: a button cloned from the template's geometry, captioned, and wired to
    // the method for both the press and the release event.
    CSWGuiButton* CreateOption(const CExoString& sText,
                               void (CSWGuiObject::*pMethod)(CSWGuiControl*),
                               CSWGuiButton* pTemplate);

    void Back(CSWGuiControl* pControl);
    void ItemSubTypeMenu(CSWGuiControl* pControl);
};

#endif // CSWGUICREATEITEMMENU_H
