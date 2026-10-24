// The upgrade-droid and workbench panels.
#pragma once
#include "k2stdint.h"

// The per-row widget the item-upgrade merchant screen builds per upgrade slot. It is a button
// carrying the row's item id, the two selector arguments and the flags that drive its name suffix,
// plus the borders and text it draws.
#ifndef CSWUPGRADEITEMENTRY_H
#define CSWUPGRADEITEMENTRY_H

#include "swguicontrol.h"

class CSWGuiBorder;
class CSWGuiBorderParams;
struct CSWGuiExtent;
class CSWGuiText;

class CSWUpgradeItemEntry : public CSWGuiButton {
public:
    CSWUpgradeItemEntry();
    virtual ~CSWUpgradeItemEntry();
    virtual void SetExtent(const CSWGuiExtent& extent);
    virtual void Draw(float f);
    // Empty overrides of the base's pair.
    virtual void Initialize(CSWGuiExtent const&, CSWGuiTextParams const&, CSWGuiBorderParams const&, CSWGuiBorderParams const&);
    virtual void Initialize(CSWGuiExtent const&, CSWGuiButton*);
    // The row builder CSWGuiUpgrade::CreateItemEntry dispatches.
#if KOTOR1  // K1 has no locked-row flag argument
    virtual void Initialize(CSWGuiText&, CSWGuiBorder&, CSWGuiBorder&, int);
#else
    virtual void Initialize(CSWGuiText&, CSWGuiBorder&, CSWGuiBorder&, int, int);
#endif
    void SetItem(unsigned long long oidItem, int nMenu, unsigned long long a, unsigned int b);

    unsigned long long m_oidItem;
    unsigned long long m_oidEquippedBy;
    unsigned int       m_nEquippedSlot;
    unsigned char     m_nFlags;
#if KOTOR1  // K1's slot index is a single byte right after the flags
    unsigned char     m_nUpgradeRow;
    char              _pad25e[2];
#else
    char              _pad27d[0x280 - 0x27d];
    int               m_nUpgradeRow;
#endif
    CSWGuiBorder  m_borderFrame;
    CSWGuiBorder  m_borderFrameHilight;
    CSWGuiBorder  m_borderIcon;
    CSWGuiText    m_textCount;
};

#if KOTOR1  // K1 size and first-field offsets; CSWGuiUpgrade::CreateItemEntry news 0x480
K2_LAYOUT_ASSERT(sizeof(CSWUpgradeItemEntry) == 0x480, "CSWUpgradeItemEntry K1 size drift");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWUpgradeItemEntry, m_oidItem) == 0x248, "K1 layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWUpgradeItemEntry, m_borderFrame) == 0x260, "K1 layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWUpgradeItemEntry, m_textCount) == 0x3f8, "K1 layout");
#else
K2_LAYOUT_ASSERT(sizeof(CSWUpgradeItemEntry) == 0x4c0, "CSWUpgradeItemEntry size drift");
#endif

#endif // CSWUPGRADEITEMENTRY_H

// The medical workbench, where components are turned into health and stim items.
#ifndef CSWGUICREATEMEDICALITEM_H
#define CSWGUICREATEMEDICALITEM_H

#include "swgui.h"
#include "CExoArrayListDtors.h"
#include "exostring.h"

class CSWGuiControl;
class CSWGuiManager;
class CSWGuiStoreItemEntry;
class CSWSCreature;
class CSWSItem;
class CSWSStore;

class CSWGuiCreateMedicalItem : public CSWGuiPanel {
public:
    // The second argument is the creature whose sheet is shown, or the invalid-object sentinel for the player
    // character; the resolved creature's own object id is kept in m_oidPlayer and the
    // argument itself in m_oidOwner.
    CSWGuiCreateMedicalItem(CSWGuiManager* pGuiManager, unsigned long long oidOwner);
    virtual ~CSWGuiCreateMedicalItem();

    virtual void HandleInputEvent(int type, int val);
    virtual void OnPanelAdded();
    virtual void OnPanelRemoved();

    void CloseChemical(CSWGuiControl* pControl);
    void OnControlXButton(CSWGuiControl* pControl);
    void OnCreateFilterPicked(CSWGuiControl* pControl);
    __attribute__((noinline)) void UpdateCreateFilters();
    void OnAcceptPicked(CSWGuiControl* pControl);
    void OnControlStoreAButton(CSWGuiControl* pControl);
    void OnControlInvAButton(CSWGuiControl* pControl);
    void OnControlEntered(CSWGuiControl* pControl);
    void DeleteItemEntries();
    void SetItemList();
    void InitializeStoreType();
    void PopulateStore();
    void ShowBuyGUI();
    void ShowSellGUI();
    void PopulateInventoryItemListBox();
    void PopulateStoreItemListBox();
    int  CreateItemEntry(int& nIndex, CExoArrayList<CSWGuiControl*>& aControls,
                         unsigned long long oidItem, int bInventory,
                         CExoString sTag);
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

    // The sheet labels; m_lblDescriptionText is the row SetDescription drives, and m_lblExamine /
    // m_lblX are toggled by InitializeStoreType.
    CSWGuiLabel m_lblA;
    CSWGuiLabel m_lblAccept;
    CSWGuiLabel m_lblB;
    CSWGuiLabel m_lblCancel;
    CSWGuiLabel m_lblReserved1;
    CSWGuiLabel m_lblExamine;
    CSWGuiLabel m_lblX;
    CSWGuiLabel m_lblDescriptionText;

    CSWGuiLabel m_lblTitle;
    CSWGuiLabel m_lblTitle2;
    CSWGuiLabel m_lblRelatedSkill;

    CSWGuiListBox m_lbDescriptionInv;

    CSWGuiButton m_btnAccept;
    CSWGuiButton m_btnCancel;
    CSWGuiButton m_btnExamine;

    CSWGuiLabel m_aBarLabels[3];

    // The ctor's InitControl order is health, stims, grenades, mines, so this array order differs
    // from the statement order.
    CSWGuiButton m_aCreateButtons[4];

    CSWGuiLabel m_lblCost;
    CSWGuiLabel m_lblCostValue;
    CSWGuiLabel m_lblStock;
    CSWGuiLabel m_lblStockValue;
    CSWGuiLabel m_lblRandAnalog;
    CSWGuiLabel m_lblCredits;
    CSWGuiLabel m_lblCreditsValue;

    CSWGuiListBox m_lbShopItems;
    CSWGuiListBox m_lbInvItems;
    CSWGuiListBox m_lbDescription;

    unsigned int m_nReserved2;
    unsigned int m_nReserved3;
    float m_fReserved1;
    int m_nComponent;
    int m_nReserved4;
    char m_aReserved1[0x46f8 - 0x46f4];
    unsigned long long m_oidStore;
    unsigned long long m_oidPlayer;
    // min(GetSkillRank(Treat Injury) / 20.0f, 1.0f), computed by the constructor.
    float m_fBreakdownYield;
    int m_bViewSwitched;
    unsigned long long m_oidOwner;
    CSWSStore* m_pStore;
    int m_nCreateFilter;
    int m_bCreateMode;
    char m_aReserved2[0x4730 - 0x4728];
};

#if !KOTOR1  // K1's GUI base classes are smaller, so the K1 size is not derived from this one
K2_LAYOUT_ASSERT(sizeof(CSWGuiCreateMedicalItem) == 0x4730, "CSWGuiCreateMedicalItem size drift");
#endif

#endif // CSWGUICREATEMEDICALITEM_H

// The swoop-bike upgrade panel, opened by CGuiInGame::ShowSwoopUpgradeScreen. Its widget wall is
// four lone buttons, the six upgrade buttons and their six labels, three list boxes, sixteen named
// labels, three more buttons and the two 3D preview views; the state tail holds the item being
// upgraded, the per-slot tag/item tables and the pending upgrade rows.
class CSWGuiManager;
class CSWGuiControl;
struct CSWGuiExtent;
class CSWSItem;
class CSWUpgradeItemEntry;
class C2DA;

class CSWGuiSwoopUpgrade : public CSWGuiPanel {
public:
    // <VTABLE-GEN: binary ground truth (_ZTV) — regenerated by tools/hoist_headers.py; do not edit inside markers */
    //   primary vtable, 30 slots
    virtual ~CSWGuiSwoopUpgrade();
    virtual void Draw(float f);
    virtual void HandleInputEvent(int type, int val);
    virtual void OnPanelAdded();
    virtual void OnPanelRemoved();
    // </VTABLE-GEN>
    CSWGuiSwoopUpgrade(CSWGuiManager* pGuiManager);

    void OnCloseSwoopUpgrade(CSWGuiControl* pControl);
    void OnAssemble(CSWGuiControl* pControl);
    void OnEnterAssemble(CSWGuiControl* pControl);
    void OnSlotSelected(CSWGuiControl* pControl);
    void OnEnterSlot(CSWGuiControl* pControl);
    int  GetNumUpgrades();
    void CloseItems();
    void ShowItems(int n);
    void UpdateItems();
    void OnControlEntered(CSWGuiControl* pControl);
    void SetDescription(const CExoString& sDesc);
    void GetArmorModelAndTextureResRef(int a, int b, int c, CExoString& d, CExoString& e);
    void Update3DView();
    void OnUpgradeSelected(CSWGuiControl* pControl);
    CSWUpgradeItemEntry* CreateItemEntry(unsigned long long oid, int n);
    void VerifyUpgrade(CSWGuiControl* pControl);
    void InsertUpgrade(CSWSItem* pItem, int n);
    void CancelPanel();

    // The per-slot default table OnEnterSlot reads for an empty slot. m_nStrRef goes straight into
    // CSWGuiTextParams::SetStrRef; m_szIcon is the fill-image resref UpdateItems uses for an empty
    // slot.
    struct SSlotInfo {
        unsigned int  m_nIconId;
        const char*   m_szIcon;
        uint64_t m_nStrRef;
    };
    static const SSlotInfo SWOOP_SLOT_INFO[6];

    CSWGuiButton m_abtnLone[4];
    CSWGuiButton m_abtnSlot[6];
    CSWGuiLabel  m_alblReserved1[4];
    CSWGuiLabel  m_apRows[6];
    CSWGuiListBox m_lbItems;
    CSWGuiListBox m_lbDesc;
    CSWGuiListBox m_lbDescLS;
    CSWGuiLabel  m_lblTitle;
    CSWGuiLabel  m_lblDescription;
    CSWGuiLabel  m_lblReserved1;
    CSWGuiLabel  m_lblDescBg;
    CSWGuiLabel  m_lblDescBgLS;
    CSWGuiLabel  m_lblProperty;
    CSWGuiLabel  m_lblSlotName;
    CSWGuiLabel  m_lblLSSlotName;
    CSWGuiLabel  m_lblUpgrades;
    CSWGuiLabel  m_lblUpgradeCount;
    CSWGuiLabel  m_lblA;
    CSWGuiLabel  m_lblB;
    CSWGuiLabel  m_lblAAction;
    CSWGuiLabel  m_lblBack;
    CSWGuiLabel  m_lblRanalog;
    CSWGuiLabel  m_lblRanalogLS;
    CSWGuiButton m_btnAssemble;
    CSWGuiButton m_btnReserved1;
    CSWGuiButton m_btnBack;
    CSWGui3DSceneView m_viewItem;
    CSWGui3DSceneView m_viewItemLS;

    // bit 0 is set while the item list is showing.
    unsigned char m_bItemsOpen;
    // Always 0: only the constructor and OnPanelRemoved write it, so the == 1 (3D_MODEL_LS) and
    // == 4 branches carried over from CSWGuiUpgrade::m_nMenuType never run.
    unsigned char m_nMenuType;
    char          _pad5b4a[0x5b50 - 0x5b4a];
    // m_pItemSlot is the owned in-flight copy of m_pItem.
    CSWSItem*     m_pItem;
    CSWSItem*     m_pItemSlot;
    CExoArrayList<CSWSItem*> m_lstItemsAddedToRepo;
    CExoArrayList<CSWSItem*> m_lstItemsTakenFromRepo;
    // Indexed by the active control's m_nUserData.
    CSWSItem*     m_apaItems[6];
    CExoString    m_aslotTag[6];
    int           m_anUpgradeRow[6];
    int           m_aswoopRow[6];
    C2DA*         m_pUpgradeC2DA;
    C2DA*         m_pUpgradeC2DA2;
    C2DA*         m_pC2DA;
    CSWGuiControl* m_pActiveSlotButton;
    void*         m_pReserved1;
};

#if !KOTOR1  // K1's GUI base classes are smaller, so the K1 size is not derived from this one
K2_LAYOUT_ASSERT(sizeof(CSWGuiSwoopUpgrade) == 0x5c68, "CSWGuiSwoopUpgrade size drift");
#endif

// The item-upgrade merchant panel shows the upgrade slots, and builds the actual upgrade through
// the sibling CSWGuiUpgradeItemSelect and CSWGuiUpgradeSelection panels.
#ifndef CSWGUIUPGRADE_H
#define CSWGUIUPGRADE_H

#include <stdint.h>
#include "twodimarray.h"
#include "nwitem.h"

class CSWSItem;
class CSWGuiControl;
struct CSWGuiExtent;
class CSWBaseItemArray;
class CSWGuiUpgradeItemSelect;

class CSWGuiUpgrade : public CSWGuiPanel {
public:
    CSWGuiUpgrade(CSWGuiManager* pGuiManager);
    virtual ~CSWGuiUpgrade();

    virtual void Draw(float f);
    virtual void HandleInputEvent(int type, int val);
    virtual void OnPanelAdded();
    virtual void OnPanelRemoved();
    void OnEnterAssemble(CSWGuiControl* pControl);
    void OnAssemble(CSWGuiControl* pControl);

#if KOTOR1  // K1's panel is smaller: a four-slot and a three-slot button set and one button pair
    CSWGuiButton m_abtnRow[4];
    CSWGuiButton m_abtnSlot[3];
    CSWGuiLabel  m_apRows[4];
    CSWGuiLabel  m_aslotLbl[3];
    CSWGuiListBox m_lbItems;
    CSWGuiListBox m_lbDesc;
    CSWGuiListBox m_lbDescLS;
    CSWGuiLabel  m_lblTitle;
    CSWGuiLabel  m_lblDescription;
    CSWGuiLabel  m_lblTitle2;
    CSWGuiLabel  m_lblDescBg;
    CSWGuiLabel  m_lblDescBgLS;
    CSWGuiLabel  m_lblProperty;
    CSWGuiLabel  m_lblSlotName;
    CSWGuiLabel  m_lblLSSlotName;
    CSWGuiLabel  m_lblUpgrades;
    CSWGuiLabel  m_lblUpgradeCount;
    CSWGuiButton m_btnAssemble;
    CSWGui3DSceneView m_viewItem;
    CSWGui3DSceneView m_viewChar;
    CSWGuiButton m_btnBack;
    unsigned char  m_bItemsOpen;
    unsigned char  m_nMenuType;
    char           _pad3ea2[6];
    CSWSItem*      m_pItem;
    CSWSItem*      m_pItemSlot;
    CExoArrayList<CSWSItem*> m_lstItemsAddedToRepo;
    CExoArrayList<CSWSItem*> m_lstItemsTakenFromRepo;
    CSWSItem*      m_apaItems[4];
    CExoString     m_sCaption[4];
    unsigned char  m_anUpgradeRow[4];
    char           _pad3f3c[4];
    C2DA*          m_pUpgradeC2DA;
    C2DA*          m_pUpgradeC2DA2;
    CSWGuiControl*         m_pActive;
    CSWGuiUpgradeItemSelect* m_pItemSelect;
#else
    // One array, not four scalars, because the destructor tears the run down as a unit.
    CSWGuiButton m_abtnHead[4];
    CSWGuiButton m_abtnRow[6];
    CSWGuiButton m_abtnSlot[3];

    // One array, since the destructor destroys the six in a loop. The three per-slot labels after
    // them are destroyed individually.
    CSWGuiLabel  m_apRows[6];
    CSWGuiLabel  m_aslotLbl[3];

    CSWGuiListBox m_lbItems;
    CSWGuiListBox m_lbDesc;
    CSWGuiListBox m_lbDescLS;

    CSWGuiLabel  m_lblTitle;
    CSWGuiLabel  m_lblTitle2;
    CSWGuiLabel  m_lblDescription;
    CSWGuiLabel  m_lblReserved1;
    CSWGuiLabel  m_lblDescBg;
    CSWGuiLabel  m_lblDescBgLS;
    CSWGuiLabel  m_lblProperty;
    CSWGuiLabel  m_lblSlotName;

    CSWGuiButton m_btnAssemble;
    CSWGuiButton m_btnOk;

    CSWGui3DSceneView m_viewItem;
    CSWGui3DSceneView m_viewChar;

    CSWGuiButton m_btnBack;

    // bit 0 is set while the item list is showing.
    unsigned char  m_bItemsOpen;
    // Selects which set of upgrade slots UpdateItems fills.
    unsigned char  m_nMenuType;
    char           _pad530a[0x5310 - 0x530a];
    // m_pItem is server-side and not owned; m_pItemSlot is owned and deleted by the destructor.
    CSWSItem*      m_pItem;
    CSWSItem*      m_pItemSlot;
    // Scratch lists of item pointers.
    CExoArrayList<CSWSItem*> m_lstItemsAddedToRepo;
    CExoArrayList<CSWSItem*> m_lstItemsTakenFromRepo;
    // The candidate items for each upgrade slot, released through the virtual destructor rather
    // than through ~CSWGuiUpgrade.
    CSWSItem*      m_apaItems[6];
    CExoString     m_sCaption[6];
    // Indexed by the active control's m_nUserData; the value is a bit index into the item's upgrade
    // mask.
    int            m_anUpgradeRow[6];
    C2DA*          m_pUpgradeC2DA;
    C2DA*          m_pUpgradeC2DA2;

    // Never destroyed by the destructor.
    CSWGuiControl*         m_pActive;
    CSWGuiUpgradeItemSelect* m_pItemSelect;
#endif

    // The per-menu-type, per-slot default table UpdateItems and OnEnterSlot read for empty slots,
    // indexed [m_nMenuType][i] with a menu type of 1..4; block 0 is never used.
    struct SSlotInfo {
        uint32_t      m_nIconId;
        const char*   m_szIcon;
        uint64_t m_nStrRef;
    };
#if KOTOR1  // K1's table has four slots per menu type
    static const SSlotInfo SLOT_INFO[4][4];
#else
    static const SSlotInfo SLOT_INFO[4][6];
#endif

    void OnUpgradeSelected(CSWGuiControl* pControl);
    void OnSlotSelected(CSWGuiControl* pControl);
    void OnEnterSlot(CSWGuiControl* pControl);
    void ShowItems(int n);
#if KOTOR1  // K1 returns the count as a byte
    unsigned char GetNumUpgrades();
#else
    int  GetNumUpgrades();
#endif
    void UpdateItems();
    void Update3DView();
    void SetDescription(const CExoString& s);
    void OnControlEntered(CSWGuiControl* pControl);
    void GetArmorModelAndTextureResRef(int a, int b, int c,
                                       CExoString& rModel, CExoString& rTexture);
    void VerifyUpgrade(CSWGuiControl* pControl);
#if KOTOR1  // K1 has no locked-row argument
    CSWUpgradeItemEntry* CreateItemEntry(unsigned long long oidItem, int nSlot);
#else
    CSWUpgradeItemEntry* CreateItemEntry(unsigned long long oidItem, int nSlot, int nMenuType);
#endif
    void InsertUpgrade(CSWSItem* pItem, int n);
    void CancelPanel();
#if !KOTOR1  // K2-only: K1 has no locked rows
    void OnDisabledSelected(CSWGuiControl* pControl);
#endif
    void CloseItems();

private:
    friend class CSWGuiUpgradeItemSelect;
};

#if KOTOR1  // K1 layout, the size CSWGuiUpgradeItemSelect's constructor allocates
K2_LAYOUT_ASSERT(sizeof(CSWGuiUpgrade) == 0x3f60, "CSWGuiUpgrade K1 size drift");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWGuiUpgrade, m_lbItems) == 0x1bd8, "K1 layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWGuiUpgrade, m_lblTitle) == 0x2820, "K1 layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWGuiUpgrade, m_viewItem) == 0x3aa8, "K1 layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWGuiUpgrade, m_bItemsOpen) == 0x3ea0, "K1 layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWGuiUpgrade, m_anUpgradeRow) == 0x3f38, "K1 layout");
#else
K2_LAYOUT_ASSERT(sizeof(CSWGuiUpgrade) == 0x5408, "CSWGuiUpgrade size drift");
#endif

#endif // CSWGUIUPGRADE_H

// The item-picking panel of the upgrade droid screen lists every item the party can upgrade at
// the requested type and hands the clicked one to the CSWGuiUpgrade sub-panel it owns.
#ifndef CSWGUIUPGRADEITEMSELECT_H
#define CSWGUIUPGRADEITEMSELECT_H

class C2DA;
class CExoString;
class CSWSCreature;
class CSWGuiControl;
class CSWGuiManager;
class CSWGuiUpgrade;
class CSWGuiUpgradeSelection;
class CSWSItem;
class CSWUpgradeItemEntry;

class CSWGuiUpgradeItemSelect : public CSWGuiPanel {
public:
    CSWGuiUpgradeItemSelect(CSWGuiManager* pGuiManager);

    virtual ~CSWGuiUpgradeItemSelect();

    virtual void HandleInputEvent(int type, int val);
    virtual void OnPanelAdded();

    void BuildUpgradeList();
    void BuildItemList(CSWSCreature* pCreature,
                       CExoArrayList<CSWGuiControl*>& aControls,
                       C2DA* pUpgradeC2DA);
    void SetDescription(const CExoString& sDescription);
    void OnControlEntered(CSWGuiControl* pControl);
    void OnUpgradableSelected(CSWGuiControl* pControl);
    void FinishUpgrading();

    CSWGuiListBox m_lbItems;
    CSWGuiListBox m_lbDescription;
    CSWGuiLabel   m_lblTitle;
    CSWGuiLabel   m_lblDescription;
    CSWGuiButton  m_btnUpgradeItem;
    CSWGuiButton  m_btnBack;

    // 0 means "no item"; BuildItemList and BuildUpgradeList keep only rows whose type equals it.
    unsigned char m_nUpgradableType;
    // m_nUnequipFlags bits.
    enum {
        UNEQUIP_FLAG_UNEQUIPPED = 0x1,
        UNEQUIP_FLAG_SECOND_SLOT = 0x2,
        UNEQUIP_FLAG_HIGH_PAIR = 0x4,
    };
    // How the item was pulled out of the equipment slots: bit 0 "the item was unequipped", bit 1
    // "it came from the second slot of the pair", bit 2 "the pair is the high one". FinishUpgrading
    // dispatches on bits 1-2.
    unsigned char m_nUnequipFlags;
    char          _pad117a[0x1180 - 0x117a];
    // The item unequipped from the other slot of the pair, or the invalid id.
    unsigned long long m_otherItemObjId;
#if !KOTOR1  // K1's m_pItem follows m_otherItemObjId directly
    int           m_bOpenedForItem;
    char          _pad118c[0x1198 - 0x118c];
#endif
    // The item being upgraded, NULL meaning none.
    CSWSItem*     m_pItem;
    // The row widget the panel acts on, either the list box's selected row or a freshly built one.
    CSWUpgradeItemEntry* m_pActive;
    // The sub-panel this class owns and adds, whose own back-pointer points here.
    CSWGuiUpgrade* m_pUpgrade;
    // The panel that created this one.
    CSWGuiUpgradeSelection* m_pUpgradeSelection;
};

#if KOTOR1  // K1 layout, the size CSWGuiUpgradeSelection's constructor allocates
K2_LAYOUT_ASSERT(sizeof(CSWGuiUpgradeItemSelect) == 0x10b0, "CSWGuiUpgradeItemSelect K1 size drift");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWGuiUpgradeItemSelect, m_btnBack) == 0xe38, "K1 layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWGuiUpgradeItemSelect, m_nUpgradableType) == 0x1080, "K1 layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWGuiUpgradeItemSelect, m_pItem) == 0x1090, "K1 layout");
#else
K2_LAYOUT_ASSERT(sizeof(CSWGuiUpgradeItemSelect) == 0x11b8, "CSWGuiUpgradeItemSelect size drift");
#endif

#endif // CSWGUIUPGRADEITEMSELECT_H

// The character-upgrade (workbench) screen lists every item the party can upgrade and opens the
// item-picking panel.
#ifndef CSWGUIUPGRADESELECTION_H
#define CSWGUIUPGRADESELECTION_H

class CSWGuiManager;
class CSWGuiControl;
class CSWGuiCreateItem;
class CSWGuiUpgradeItemSelect;
class CSWSCreature;
class C2DA;

class CSWGuiUpgradeSelection : public CSWGuiPanel {
public:
    // <VTABLE-GEN: binary ground truth (_ZTV) — regenerated by tools/hoist_headers.py; do not edit inside markers */
    //   primary vtable, 30 slots
    virtual ~CSWGuiUpgradeSelection();
    virtual void HandleInputEvent(int type, int val);
    virtual void OnPanelAdded();
    // </VTABLE-GEN>
#if KOTOR1  // K1 has four item-type radio buttons instead of a list and item creation
    CSWGuiUpgradeSelection(CSWGuiManager* pGuiManager);
    void OnTypeSelected(CSWGuiControl* pControl);
    void OnEnter(CSWGuiControl* pControl);
    void AutoUpgradeDone();
    void GetUpgradableItems(CSWSCreature* pCreature, int* panAvailable, C2DA* pUpgradeC2DA);

    CSWGuiLabel        m_lblTitle;
    CSWGuiButton       m_btnUpgradeItems;
    CSWGuiButton       m_btnBack;
    CSWGuiRadioButton  m_abtnType[4];
    CSWGuiLabel        m_alblType[4];
    unsigned long long m_itemObjId;
    int                m_abAvailable[4];
    CSWGuiUpgradeItemSelect* m_pUpgradeItemSelect;
    CSWGuiControl*     m_pSelected;
#else
    CSWGuiUpgradeSelection(CSWGuiManager* pGuiManager, unsigned long long creatureObjId);

    __attribute__((noinline)) void UpdateFilterButtons();
    __attribute__((noinline)) void PopulateItemList();

    void OnFilterPicked(CSWGuiControl* pControl);
    void OnItemSelected(CSWGuiControl* pControl);
    void OnCreateItem(CSWGuiControl* pControl);
    void AutoUpgradeDone();
    void OnItemEntered(CSWGuiControl* pControl);
    // By value, unlike the sibling panel's const reference.
    void SetDescription(CExoString sDescription);
    void GetPlayerItems(CSWSCreature* pCreature, CExoArrayList<CSWGuiControl*>& aControls,
                        C2DA* pUpgradeC2DA);

    CSWGuiLabel   m_lblTitle;
    CSWGuiLabel   m_lblTitle2;
    CSWGuiButton  m_btnUpgradeItems;
    CSWGuiButton  m_btnBack;
    CSWGuiButton  m_btnCreateItems;
    // Owned by the GUI manager, not by this destructor.
    CSWGuiCreateItem* m_pCreateItem;
    // Declared as one run of five so the array size carries; each button's m_nUserData is its own
    // index 0..4.
    CSWGuiButton  m_abtnFilter[5];
    CSWGuiListBox m_lbUpgradeList;
    CSWGuiListBox m_lbDescription;
    CSWGuiLabel   m_lblDescription;

    // The item being upgraded, or the invalid id.
    unsigned long long m_itemObjId;
    // The upgrading creature, equal to the constructor's second argument.
    unsigned long long m_creatureObjId;
    char          _pad21b8[0x21c8 - 0x21b8];
    // The selected filter index, compared against 0..4.
    int           m_nFilter;
    char          _pad21cc[0x21d0 - 0x21cc];
    // Deleted by the destructor; its own constructor is handed the GUI manager only.
    CSWGuiUpgradeItemSelect* m_pUpgradeItemSelect;
    unsigned long long m_nReserved1;
    // Non-zero when the screen is live. It gates AddPanel, suppresses the close sound and is tested
    // by CSWGuiCreateItem::CloseCreateItem.
    unsigned int  m_bParam3;
    unsigned int  m_bParam4;
    CExoString    m_description;
#endif
};

#if KOTOR1  // K1 layout, the size CGuiInGame::ShowUpgradeScreen allocates
K2_LAYOUT_ASSERT(sizeof(CSWGuiUpgradeSelection) == 0x1678, "CSWGuiUpgradeSelection K1 size drift");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWGuiUpgradeSelection, m_abtnType) == 0x6b0, "K1 layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWGuiUpgradeSelection, m_itemObjId) == 0x1650, "K1 layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWGuiUpgradeSelection, m_pSelected) == 0x1670, "K1 layout");
#else
K2_LAYOUT_ASSERT(sizeof(CSWGuiUpgradeSelection) == 0x21f8, "CSWGuiUpgradeSelection size drift");
#endif

#endif // CSWGUIUPGRADESELECTION_H
