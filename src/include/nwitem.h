// Item definition types shared by the client and server item classes.
#pragma once
#include "k2stdint.h"

// The shared item base, common to the client CSWCItem and the server CSWSItem. Both derived classes
// place their second base at the end of this one, which pins sizeof(CSWItem).
class CSWBaseItem;

class CSWItem {
public:
    CSWItem();
    virtual ~CSWItem();

    unsigned char m_aModelParts[6];
    unsigned char _pad0e[0x10 - 0x0e];
    int           m_baseItem;
    unsigned char _pad14[0x18 - 0x14];

    CSWBaseItem* GetBaseItem();
};

K2_LAYOUT_ASSERT(sizeof(CSWItem) == 0x18, "CSWItem must be 0x18 so the secondary base lands at 0x18");

// One row of baseitems.2da, the static definition of an item base type (wield category, required
// feats, name, icon and model resrefs). CSWBaseItemArray holds them in one array of fixed-size rows.
#include "exostring.h"
#include "exoresman.h"

class CSWBaseItem {
public:
    // m_weaponWieldType, baseitems.2da's WeaponWield: how the weapon is held, the stun baton, a
    // one-handed or two-handed melee weapon, a pistol, a rifle or a heavy weapon. NONE (an empty
    // cell) is not a weapon.
    enum {
        WEAPON_WIELD_NONE       = 0,
        WEAPON_WIELD_STUN_BATON = 1,
        WEAPON_WIELD_ONE_HANDED = 2,
        WEAPON_WIELD_TWO_HANDED = 3,
        WEAPON_WIELD_PISTOL     = 4,
        WEAPON_WIELD_RIFLE      = 5,
        WEAPON_WIELD_HEAVY      = 6,
    };
    // m_nWeaponSize, baseitems.2da's WeaponSize, on the creature-size scale it is measured against:
    // the stun baton, the short blades and pistols, the long blades and lightsabers, and the
    // two-handed weapons, rifles and heavy weapons.
    enum {
        WEAPON_SIZE_TINY   = 1,
        WEAPON_SIZE_SMALL  = 2,
        WEAPON_SIZE_MEDIUM = 3,
        WEAPON_SIZE_LARGE  = 4,
    };

    CSWBaseItem();

    ~CSWBaseItem();

    int            m_nNameStrRef;
    int            m_nEquipableSlots;
    unsigned char  m_weaponWieldType;
    unsigned char  m_nWeaponType;
    unsigned char  _pad0a[0x0c - 0x0a];
    union {
        int            m_nDamageFlags;
        unsigned char  m_abDamageFlagBytes[4];
    };
    int            m_nMinRange;
    int            m_nMaxRange;
    unsigned char  m_nModelType;
    unsigned char  m_nChargesStarting;
    unsigned char  m_bRangedWeapon;
    unsigned char  m_nWeaponSize;
    unsigned char  m_nNumDice;
    unsigned char  m_nDieToRoll;
    unsigned char  m_nCritThreat;
    unsigned char  m_nCritMultiplier;
    float          m_fBaseCost;
    unsigned short m_nStacking;
    int            m_nDescription;
    unsigned char  m_nMinProps;
    unsigned char  m_nMaxProps;
    unsigned char  m_nPropColumn;
    unsigned char  m_nStorePanel;
    unsigned char  m_nStorePanelSort;
    unsigned char  _pad31[0x34 - 0x31];
    float          m_fMaxRange;
    float          m_fPrefAttackDist;
    char           m_resRefStem[0x48 - 0x3c];
    int            m_bHasEnvMap;
    CResRef        m_defaultIconResRef;
    CResRef        m_defaultModelResRef;
    int            m_bCanRotateIcon;
    int            m_bContainer;
    int            m_bHasTextureVariation;
    unsigned char  m_nInvSoundType;
    unsigned char  _pad79[0x80 - 0x79];
    unsigned short* m_pRequiredFeats;
    unsigned char  m_nRequiredFeats;
    unsigned char  _pad89[0x90 - 0x89];
    int64_t           m_nBaseItemStatRef;
    unsigned char  m_bRotateOnGround;
    unsigned char  m_nTenthLbs;
    unsigned char  m_nBaseAC;
    unsigned char  m_nACEnchant;
    unsigned char  m_bIsPC;
    unsigned char  m_nArmorCheckPen;
    unsigned char  m_nAmmunitionType;
    unsigned char  _pad9f[0xa0 - 0x9f];
    int            m_bPoweredItem;
    unsigned char  _pada4[0xa8 - 0xa4];
    CExoString     m_sPoweredSnd;
    CExoString     m_sPowerUpSnd;
    CExoString     m_sPowerDownSnd;
    unsigned char  m_nItemType;
    unsigned char  m_nMaxDexBonus;
    unsigned char  _pada[0xdb - 0xda];
    unsigned char  m_nBodyVar;
    unsigned short m_nWeaponSpecializationFeat;
    unsigned short m_nWeaponFocusFeat;
    unsigned char  m_nRaceRestriction;
    unsigned char  _pade1[0xe4 - 0xe1];
    int            m_nDenySubrace;
    CExoString     m_sArmorType;
    float          m_costMultiplier;
    unsigned char  _padfc[0x100 - 0xfc];

    unsigned short GetRequiredFeat(unsigned char nIndex);
    void           SetRequiredFeat(unsigned char nIndex, unsigned short value);
    void           SetRequiredFeatCount(unsigned char count);
    int            GetIsOneHanded();
    int            GetIsTwoHanded();

    // Builds the art resref for one appearance of this base item from m_resRefStem:
    //   icon "i%s_%c_%03d" / "i%s_%03d"    model "%s_%c_%03d" / "%s_%03d"
    // The lettered form is used only when a texture variation is asked for and the base item has
    // per-texture art.
    CResRef        GetIconResRef(short nModelVariation, char nTextureVariation);
    CResRef        GetModelResRef(short nModelVariation, char nTextureVariation);

    // The base item's display name, a TLK fetch off this row's name strref.
    CExoString     GetNameText();
};

K2_LAYOUT_ASSERT(sizeof(CSWBaseItem) == 0x100, "CSWBaseItem stride is pinned at 0x100 by GetBaseItem's x8<<8");

// The baseitems.2da table, a count plus a heap array of equally sized CSWBaseItem rows.
class CSWBaseItemArray {
public:
    int          m_count;
    unsigned char _pad04[4];
    CSWBaseItem* m_pItems;

    CSWBaseItemArray();
    ~CSWBaseItemArray();
    CSWBaseItem* GetBaseItem(int index) const;

    void Load();
};
