
#pragma once

#include "CExoArrayListDtors.h"
#include "itemtype.h"

class CExoString;
class CSWSItem;
template <class T> class CExoArrayList;

// An inventory container, the item list behind creatures, placeables, stores and the party
// inventory.
class CItemRepository {
public:
    // The object the repository belongs to; SetParentId rewrites it when the party inventory follows
    // the player's driver switch.
    unsigned long long m_objectId;
    // How many of the repository's items still carry the new or unseen flag.
    int  m_nNewItemCount;
    // Set for the party-wide inventory, clear for a creature-, placeable- or store-owned one.
    unsigned char m_bPartyInventory : 1;
    unsigned char _pad0c_b1   : 7;
    char _pad0d[0x10-0x0d];
    // The contents, as game-object ids.
    CExoArrayList<unsigned long long> m_aItems;

    // The count as readers outside the list see it.
    unsigned int ItemListGetCount() { return m_aItems.m_count; }

    CItemRepository(unsigned long long ownerObjectId, int flag);
    ~CItemRepository();

    void ClearItems();
    // Retires a "picked up but not yet seen" list: clears each item's new flag, drops the repository's
    // unseen counter, and erases the entry from the id array it was given. The walk runs backwards
    // because removing an entry shifts the tail down.
    void ClearNewItems(CExoArrayList<unsigned long long>& list);
    CSWSItem* ItemListGetItem(int index);
    // The id-only twin of ItemListGetItem, where an out-of-range index returns the invalid-object
    // sentinel rather than null.
    unsigned long long ItemListGetItemObjectID(int index);
    int  AddItem(CSWSItem** ppItem, int a, int b, int c);
    int  GetItemInRepository(CSWSItem* pItem, int flag);
    // How many of the repository's items match the reference item's base-item type. Callers test for
    // -1, which means nothing matched.
    int  GetMatchingItemCount(CSWSItem* pItem);
    // Returns the matching item's game-object id, or 0 if none.
    unsigned long long FindItemWithTag(const CExoString& tag);
    // Returns the matching item's game-object id, or the invalid-object sentinel if none.
    unsigned long long FindItemWithBaseItemType(unsigned char baseItemType, int a);
    void InsertItem(CSWSItem* pItem, int index);
    // Returns a status the store's sell path tests; other call sites discard it.
    int  RemoveItem(CSWSItem* pItem);
    // Total weight of the contents.
    int  CalculateContentsWeight();

    // Re-parents the whole repository onto a different owning object, so the party's shared item store
    // follows the character the client is controlling.
    void SetParentId(unsigned long long objectId);
};

#include "Vector.h"
#include "nwitem.h"
#include "nwsobject.h"
#include "nwsitemproperty.h"
#include "exolocstring.h"
#include "exostring.h"
#include "exoresman.h"

class CResGFF;
class CResRef;
class C2DA;
struct CResStruct;
template <class T> class CExoArrayList;
class CSWSArea;
class CSWBaseItem;
class CSWCItem;
class CSWSCreature;
class CSWPartyTable;

class CSWSItem : public CSWItem, public CSWSObject {
public:
    // The bits of m_flags.
    enum {
        FLAG_IDENTIFIED = 0x1,
        FLAG_INVENTORY_OPEN = 0x2,
        FLAG_INFINITE = 0x4,
        FLAG_DROPABLE = 0x8,
        FLAG_PICKPOCKETABLE = 0x10,
        FLAG_STOLEN = 0x20,
        FLAG_NON_EQUIPPABLE = 0x40,
        FLAG_NEW_ITEM = 0x80,
        FLAG_DELETING = 0x100,
    };

    // GetUpgradableType's result: the upgrade category the item belongs to, which the upgrade
    // panels keep as their menu type.
    enum UpgradableType {
        UPGRADABLE_TYPE_NONE = 0,
        UPGRADABLE_TYPE_LIGHTSABER = 1,
        UPGRADABLE_TYPE_RANGED = 2,
        UPGRADABLE_TYPE_MELEE = 3,
        UPGRADABLE_TYPE_ARMOR = 4,
    };

    virtual ~CSWSItem();

    // Declared for the GUI container message handler.
    void CloseInventory(unsigned long long objectId, int flag);

    // The open half of the same pair, used when a player uses a container item. The container is
    // toggled shut when the player already has it open. It takes no flag, unlike CloseInventory.
    void OpenInventory(unsigned long long objectId);

    // Loads the client twin's model and, for an environment-mapped base item, applies the default
    // env-map. Returns 0 on success.
    int ShowModel();

    // The client twin of this item, or null when it has none.
    CSWCItem* GetClientItem();

    // Item icon and property-description accessors, declared for the upgrade screen. The property
    // description returns through the return slot; the icon returns a resref by value.
    CExoString GetPropertyDescription();
    void SetDropable(int bDropable) { m_bits.m_bDropable = bDropable; }
    // The property-description section builders GetPropertyDescription chains together; each appends
    // its own section text to the string handed to it.
#if !KOTOR1
    void AddAttributeRequirements(CExoString& sOut);
#endif
    void AddFeatRequirements(CExoString& sOut);
#if !KOTOR1
    void AddGenderRequirements(CExoString& sOut);
    void AddSubraceRequirements(CExoString& sOut);
    void AddPCRequirements(CExoString& sOut);
    void AddCNPCRequirements(CExoString& sOut);
#endif
    void AddDamageProperties(CExoString& sOut);
    void AddRangeProperties(CExoString& sOut);
    void AddCriticalThreatProperties(CExoString& sOut);
    void AddOnHitProperties(CExoString& sOut);
    void AddWeaponSizeProperties(CExoString& sOut);
    void AddAttackModifierProperties(CExoString& sOut);
    void AddDefenceProperties(CExoString& sOut);
    void AddMiscellaneousProperties(CExoString& sOut);
    // The full description including the item's hidden properties, and the talk-table strref of the
    // per-property-type keyed string used for empty upgrade-slot text.
    CExoString GetPropertiesOnlyDescription();
    CExoString GetKeyedPropertyString(unsigned char nItemPropertyType);
    // Sorts the accumulated keyed-property rows and joins them into one string.
    void GetSortedPropertyStrings(CExoArrayList<CExoString>& list, CExoString& sOut);
    // Renders one property row into up to four strings; every description section builder is a caller
    // and GetPropertiesOnlyDescription passes all of them. The first argument is the row index into the
    // named property-definition table, which is why the other two carry the subtype and the cost-table
    // value. The callers ignore the result.
#if KOTOR1  // no description-mode flag: K2's remapped and hidden names do not exist
    void GetPropertyStrings(unsigned short nType, unsigned short nSubType, unsigned short nValue,
                            unsigned char nSlot, CExoString& s1, CExoString& s2, CExoString& s3,
                            CExoString& s4);
    void GetPropertyStrings(unsigned short nType, unsigned short nSubType, unsigned short nValue,
                            unsigned char nSlot, CExoString& s1, CExoString& s2, CExoString& s3,
                            CExoString& s4, int)
        { GetPropertyStrings(nType, nSubType, nValue, nSlot, s1, s2, s3, s4); }
#else
    void GetPropertyStrings(unsigned short nType, unsigned short nSubType, unsigned short nValue,
                            unsigned char nSlot, CExoString& s1, CExoString& s2, CExoString& s3,
                            CExoString& s4, int nFlag);
#endif
    // One attack-bonus sub-section. Rows are grouped by subtype; only the first passive property of a
    // group renders, with the whole group's cost-table values summed into that one line.
    void AddAttackBonusStrings(int nType, CExoString& sOut);
    // The on-hit property's three per-row numbers, read out of the on-hit duration table row named by
    // the argument and the cost table row named by the property's cost-table value.
    void GetPropertyChanceDurationDC(unsigned short nCostTableValue, unsigned char nParam1,
                                     int& nChance, int& nDuration, int& nValue);
    // Renders the item's damage-type flag bits as the comma-separated clause list the damage section
    // appends. The low three bits, bludgeoning, piercing and slashing, collapse into one lead phrase;
    // bits 3..12 each append their own clause and advance the separator count; bit 13 is a trailing
    // qualifier that takes the separator but does not advance it.
    void GetDamageTypeString(unsigned int nDamageFlags, CExoString& sOut);
    // Renders one property row as the single-line form the keyed-property list stores: the type's name
    // strref, a subtype clause, where a subtype of 2 or less short-circuits to the damage-type phrase,
    // then optional cost-table and param clauses with the punctuation between them.
    void AddProperty(unsigned short nType, unsigned short nSubType, unsigned short nValue,
                     unsigned char nParam, CExoArrayList<CExoString>& list);

    // The charged-active-property state. Bit i of the mask means active property i consumes charges,
    // and only the first eight active properties can ever be used.
    unsigned char m_nUsedActivePropertyMask;
    unsigned char m_nUsedActivePropertyUsesLeft[8];
    unsigned char _pad351[0x354-0x351];
    // The item's own armour-class contribution, which EquipMostEffectiveArmor seeds its running
    // total with before adding every AC-bonus property on top.
    unsigned int m_nBaseArmorClass;
    int m_nActiveProperties;
    int m_nPassiveProperties;
    CSWItemProperty* m_pActiveProperties;
    CSWItemProperty* m_pPassiveProperties;
    int m_nCharges;
    // A quantity the charged-active uses text scales three ways; meaning unknown.
    int m_nMaxCharges;
    // The per-unit quantity of the stack. GetCost scales the base cost by it, and it is the number
    // SplitItem and MergeItem move.
    int m_nAddCost;
    // Compared pairwise between an equipped candidate and a reference item before CompareItem.
    unsigned char m_nModelVariation;
    // The second and third bytes of the item's appearance triple; sent only for base items using the
    // multi-part model scheme. Purposes otherwise unknown.
    unsigned char m_nBodyVariation;
    unsigned char m_nTextureVar;
    unsigned char _pad37f[0x380-0x37f];
    // The item's possessor.
    unsigned long long m_possessorId;
    // The item's own contents, for items that are containers.
    class CItemRepository* m_pItemRepository;
    // The two description strings, written under the GFF labels "DescIdentified" and "Description".
    // The examine panel prefers the identified one and falls back to the base description when it has
    // no text.
    CExoLocString m_descriptionIdentified;
    CExoLocString m_description;
    CExoLocString m_localizedName;
    // Read at both widths, since the store accesses the full 16-bit flags word while the placeable
    // inventory scan reads only the low byte. The union keeps both shapes.
    union {
        unsigned short m_flags;
        unsigned char  m_nFlagsLow;
        struct {
            unsigned short m_nLowFlagBits : 3;
            unsigned short m_bDropable : 1;   // GFF "Dropable"
            unsigned short m_bPickpocketable : 1;
            // Shipped to the client as its own flag by AddActiveItemPropertiesToMessage.
            unsigned short m_bStolen : 1;
            // Set by SetItemNonEquippable; while set, the item cannot be equipped into any slot.
            unsigned short m_bNonEquippable : 1;
            // "New since it was picked up", the per-item half of the repository's unseen counter.
            unsigned short m_bNewItem : 1;
            // The save-format "deleting" flag, set from the save on load and written back by SaveItem.
            unsigned short m_bDeleting : 1;
            unsigned short m_nReserved1 : 7;
        } m_bits;
    };
    unsigned char _pad3c2[2];
    int m_stackCount;
    // The item's own unit weight, which GetWeight adds to the container's contents weight or scales
    // by the stack count.
    int m_nWeight;
    // The GFF "Upgrades" bitmask of live upgrade slots. A property whose upgrade type is not the
    // no-upgrade sentinel applies only while its bit is set, and the upgrade screen clears the bit.
    int m_nUpgrades;
    // The next-contents cursor of the container walk over the item repository, read and written as a
    // 16-bit value.
    unsigned short m_nInventoryWalkIndex;
#if !KOTOR1  // K1 has no property-derived combat words and no per-item upgrade vars
    unsigned char _pad3d2[0x3d4-0x3d2];
    // Item-property-derived combat flags, written by the property handler's apply and remove pair.
    int m_bDoorCutting;
    int m_bDoorSabering;
    int m_bRapidShot;
    int m_bSniper;
    int m_nMaxDexBonus;
    // Compared by the upgrade screen's slot scan.
    int m_nUpgradeLevel;
    // One upgrade var per slot; -1 is the "no upgrade here" sentinel.
    int m_anUpgVars[6];
    unsigned char _pad404[0x408-0x404];
#endif

    CSWSItem(unsigned long long objectId);

    // The order here is the primary vtable's slot order: every override of a base virtual also gets a
    // primary slot, in declaration order.
    virtual void AIUpdate();
    virtual CSWSItem* AsSWSItem();
    virtual void EventHandler(unsigned int nEventType, unsigned long long nCallerId, void* pData,
                              unsigned int a, unsigned int b);
    // Returns m_localizedName, not m_sName.
    virtual CExoLocString& GetFirstName();

    int SaveItem(CResGFF* pGFF, CResStruct* pStruct);
    // Serializes the item's own contents as the "ItemList" list; the reader's twin is
    // ReadContainerItemsFromGff. Returns 1.
    int SaveContainerItems(CResGFF* pGFF, CResStruct* pStruct);
    // Serializes the item's active and passive property arrays.
    int SaveItemProperties(CResGFF* pGFF, CResStruct* pStruct);
    int LoadFromTemplate(const CResRef& templateRef);
    int LoadItem(CResGFF* pGFF, CResStruct* pStruct);
    // The full GFF read behind the three wrappers above.
    void LoadDataFromGff(CResGFF* pGFF, CResStruct* pStruct);
    // The container half of the read, SaveContainerItems' twin.
    void ReadContainerItemsFromGff(CResGFF* pGFF, CResStruct* pStruct);

    // Shuts the container UI on every player who currently has this item's inventory open.
    void CloseItemForAllPlayers();

    void SetPossessor(unsigned long long newOwnerId, int a, int b,
                      unsigned long long prevOwnerId);
    // Gives a container-flagged item a fresh empty repository, deleting any it already has.
    void InitRepository(unsigned long long ownerObjectId);
    void DecrementUses(int nIndex, unsigned long long userId);
    void AddToArea(CSWSArea* pArea, float x, float y, float z, int flag);
    void RemoveFromArea();
    void RemoveItemProperties(CSWSCreature* pCreature, unsigned int a);
    int  GetPropertyByTypeExists(unsigned short type, unsigned short subType);
    // Resolves one property row's damage dice into the two out-params, reading the named "NumDice" and
    // "DieToRoll" columns for the subtype.
    void GetPropertyDamageRange(unsigned short nSubType, int& nMin, int& nMax);
    int  GetPropertyByType(CSWItemProperty** ppOut, unsigned short type,
                           unsigned short subType);
    unsigned int GetCost();
    // The item's own armour class; only items with a positive value impose a max-Dex cap.
    int  ComputeArmorClass();
    // Splits count units off this stack and returns the new item.
    CSWSItem* SplitItem(int count);
    // The container-item side of an item transfer; an item can only hold contents when its base item
    // row is flagged as a container.
    int  AcquireItem(CSWSItem** ppItem, unsigned long long possessorId, int flag);
    // Absorbs the other stack into this one; nonzero means absorbed.
    int  MergeItem(CSWSItem* pOther);
    // Copies the source's flags and registers the copy in the module lookup table under the source's
    // tag. Returns 0 for a null or undrained-container source, 1 otherwise.
    int  CopyItem(CSWSItem* pSrc);
    int  CompareItem(CSWSItem* pOther);

    void SetIdentified(int flag);       // sets flags bit 0
    // Per-slot upgrade wiring, driven by the upgrade screen.
    void AddUpgrade(CSWSItem* pUpgrade, unsigned int nSlot);
    void RemoveUpgrade(CSWSItem* pUpgrade, unsigned int nSlot);
    int  IsLightsaber();
    // Forwards the powered flag to the client twin.
    void PowerItem(int a, int b, int c);
    CSWItemProperty* GetActiveProperty(int nIndex);
    CSWItemProperty* GetPassiveProperty(int nIndex);
    // True when this property type is charge consuming.
    int  ActiveProperty(unsigned short nType);
    void SetNumActiveProperties(int nCount);
    void SetNumPassiveProperties(int nCount);
    // True when any passive property matches the pair with a cost-table value above the given one.
    int  GetExistsHigherValuedProperty(unsigned short type, unsigned short subType,
                                       unsigned short value);

    // The minimum character level required to equip this item.
    unsigned char GetMinEquipLevel();
    int GetWeight();
    // Returns the weapon damage-type family: 0 bludgeoning, 1 piercing, 2 slashing.
    int GetDamageFlags();

    // Uses remaining on the nIndex-th active property, 0 meaning expended. The body switches on the
    // property's use-frequency category. Callers read the result as a 16-bit count.
    unsigned short GetUsedActivePropertyUsesLeft(unsigned char nIndex);

    CResRef GetIcon();
    // Which UPGRADABLE_TYPE the base item's category byte maps to. The table argument is never read.
    unsigned char GetUpgradableType(C2DA* pTable);

    // Applies every one of this item's properties to the creature that just equipped it into the given
    // slot.
    void ApplyItemProperties(CSWSCreature* pCreature, unsigned int nSlot, int a);

    // Restores the item's used active properties.
    void RestoreUsedActiveProperties(int a);
    void UpdateUsedActiveProperties();
};

#if KOTOR1
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSItem, m_nUsedActivePropertyMask) == 0x308, "K1 CSWSItem layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSItem, m_possessorId) == 0x340, "K1 CSWSItem layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSItem, m_nUpgrades) == 0x38c, "K1 CSWSItem layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSItem, m_nInventoryWalkIndex) == 0x390, "K1 CSWSItem layout");
K2_LAYOUT_ASSERT(sizeof(CSWSItem) == 0x398, "K1 CSWSItem size");
#endif
