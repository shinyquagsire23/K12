#pragma once

// One spell or force-power definition row: name, level, prerequisites, force-point cost and the
// sub-radial spell list the radial menu fans a force power out into. CSWSpellArray owns the rows.

#include <stddef.h>
#include "exostring.h"
#include "exoresman.h"

class CSWSpell {
public:
    // GetSpellLevel and the m_nSpellLevel* columns for a class that cannot cast the power.
    enum {
        SPELL_LEVEL_NONE = 0xff,
    };

    // m_nAlignmentCode: spells.2da's GoodEvil column, a light-side or dark-side power (anything
    // else is neutral).
    enum {
        ALIGNMENT_CODE_GOOD = 'G',
        ALIGNMENT_CODE_EVIL = 'E',
    };

    // m_nConjureAnimationKind and m_nCastAnimationKind: spells.2da's CastAnim string ("point" and
    // "touch" share POINT; anything unknown is NONE).
    enum CastAnimationKind {
        CAST_ANIMATION_NONE         = 0,
        CAST_ANIMATION_SELF         = 1,
        CAST_ANIMATION_DARK         = 2,
        CAST_ANIMATION_UP           = 3,
        CAST_ANIMATION_AREA         = 4,
        CAST_ANIMATION_POINT        = 6,
        CAST_ANIMATION_THROW        = 7,
        CAST_ANIMATION_JUMP         = 8,
        CAST_ANIMATION_MONSTER      = 9,
#if !KOTOR1  // K1's CastAnim stops at "monster"
        CAST_ANIMATION_FURY         = 10,
        CAST_ANIMATION_CRUSH        = 11,
        CAST_ANIMATION_MONSTER_FURY = 12,
#endif
    };

    static const int USERTYPE_NONE;
    static const int USERTYPE_FORCEPOWER;
    static const int USERTYPE_MONSTERPOWER;
    static const int USERTYPE_FEAT;
    static const int USERTYPE_ITEM;
    static const int USERTYPE_MASTER;
    static const int USERTYPE_FORM;

#if !KOTOR1  // K1 rows carry no spell id
    int           m_nSpellId;
#endif
    CExoString    m_sName;
    int           m_nNameStrRef;
    int           m_nDescriptionStrRef;
    CResRef       m_iconResRef;
    CExoString    m_sRangeCode;
    CExoString    m_sReserved1;
    unsigned short m_nTargetType;
    CExoString    m_sImpactScript;
    unsigned char m_nSpellLevelClass3;
    unsigned char m_nSpellLevelClass4;
    unsigned char m_nSpellLevelClass5;
    unsigned char m_nSpellLevelOther;
#if !KOTOR1  // K1 has four level bytes (Guardian, Consular, Sentinel, Innate) and no prestige classes
    unsigned char m_nSpellLevelClass11;
    unsigned char m_nSpellLevelClass12;
    unsigned char m_nSpellLevelClass13;
    unsigned char m_nSpellLevelClass14;
    unsigned char m_nSpellLevelClass15;
    unsigned char m_nSpellLevelClass16;
    char          _pad72[0x74 - 0x72];
#endif
    int           m_nConjureTimeMS;
    unsigned short m_nConjureAnimationKind;
    CResRef       m_conjHeadVisual;
    CResRef       m_conjHandVisual;
    CResRef       m_conjGrndVisual;
    CResRef       m_conjSoundMale;
    CResRef       m_conjSoundFemale;
    CResRef       m_conjSoundVFX;
    unsigned short m_nCastAnimationKind;
    int           m_nCastPauseMS;
    CResRef       m_castHeadVisual;
    CResRef       m_castHandVisual;
    CResRef       m_castGrndVisual;
    CResRef       m_castSound;
    int           m_nCastRecoveryMS;
    unsigned short m_nCatchAnimation;
    char          _pad126[0x128 - 0x126];
    int           m_bHasProjectile;
    CResRef       m_projectileResRef;
    int           m_nProjectileType;
    unsigned char m_nCastAttachNode;
    CResRef       m_projSoundResRef;
    unsigned char m_nProjOrientation;
    unsigned char m_nReserved1;
    int           m_nReserved2;
    int           m_nCategory;
    int           m_nMaxCR;
    unsigned int* m_paSubRadialSpells;
    unsigned char m_nSubRadialCount;
char          _pad169[0x16c - 0x169];
    int           m_bUseConcentration;
    int           m_nCounter1;
    int           m_nCounter2;
    unsigned char m_nUserType;
    char          _pad179[0x17c - 0x179];
    int           m_nAltMessage;
    int           m_bHostileActivity;
    unsigned short m_nFeatId;
    unsigned char m_nBaseForcePoints;
    unsigned char m_nAlignmentCode;
    CExoString    m_sPrerequisites;
    CExoString    m_sMasterSpells;
    int           m_anPrereq1[5];
    int           m_anPrereq2[5];

    void SetPrerequisites(const CExoString& sPrereq);
    void SetMasterSpells(const CExoString& sMaster);

    unsigned int  m_nExclusionMask;
    unsigned int  m_nForbidItemMask;
    unsigned int  m_nRequireItemMask;
#if !KOTOR1  // K1 spells.2da has no FormMask column
    unsigned int  m_nSpellFormMask;
#endif
    unsigned char m_nItemTargeting;

    CSWSpell();
    ~CSWSpell();
    CSWSpell(const CSWSpell& other) = default;

    unsigned char GetSpellLevel(unsigned char nClassId);

    unsigned int GetSubRadialSpell(unsigned char idx);
    void         SetSubRadialSpell(unsigned char idx, unsigned int spellId);

    void         SetSubRadialSpellCount(unsigned char nCount);

    CExoString   GetSpellNameText();

#if KOTOR1
    // K1: alignment and in-party only, no room force rating, stance or charisma term.
    int GetAdjustedForcePoints(unsigned int nAlignment, int bInParty);
    int GetAdjustedForcePoints(unsigned int nAlignment, int bInParty, int, int, char)
        { return GetAdjustedForcePoints(nAlignment, bInParty); }
#else
    int GetAdjustedForcePoints(unsigned int nAlignment, int bInParty, int nRoomForceRating,
                                int nStanceId, char cCHAMod);
#endif

    void SetSpellIcon(const CResRef& r)                { m_iconResRef = r; }
    void SetSpellConjureHeadVisual(const CResRef& r)   { m_conjHeadVisual = r; }
    void SetSpellConjureHandVisual(const CResRef& r)   { m_conjHandVisual = r; }
    void SetSpellConjureGroundVisual(const CResRef& r) { m_conjGrndVisual = r; }
    void SetSpellConjureSoundMale(const CResRef& r)    { m_conjSoundMale = r; }
    void SetSpellConjureSoundFemale(const CResRef& r)  { m_conjSoundFemale = r; }
    void SetSpellConjureSoundVFX(const CResRef& r)     { m_conjSoundVFX = r; }
    void SetSpellCastHeadVisual(const CResRef& r)      { m_castHeadVisual = r; }
    void SetSpellCastHandVisual(const CResRef& r)      { m_castHandVisual = r; }
    void SetSpellCastGroundVisual(const CResRef& r)    { m_castGrndVisual = r; }
    void SetSpellCastSound(const CResRef& r)           { m_castSound = r; }
    void SetSpellProjectileModel(const CResRef& r)     { m_projectileResRef = r; }
    void SetSpellProjectileSound(const CResRef& r)     { m_projSoundResRef = r; }
    void SetSpellAlignment(char cAlignment)            { m_nAlignmentCode = cAlignment; }
    void SetSpellRange(const CExoString& sRange)       { m_sRangeCode = sRange; }

    // menu * 1000 + slot * 10 + priority
    int GetMenu(int n)     { return static_cast<int>(GetSubRadialSpell(static_cast<unsigned char>(n))) / 1000; }
    int GetSlot(int n)     { return static_cast<int>(GetSubRadialSpell(static_cast<unsigned char>(n)) - GetSubRadialSpell(static_cast<unsigned char>(n)) / 1000 * 1000) / 10; }
    int GetPriority(int n) { return GetSubRadialSpell(static_cast<unsigned char>(n)) - GetSubRadialSpell(static_cast<unsigned char>(n)) / 10 * 10; }
};

#if KOTOR1
K2_LAYOUT_ASSERT(offsetof(CSWSpell, m_nNameStrRef) == 0x10, "K1 CSWSpell::m_nNameStrRef");
K2_LAYOUT_ASSERT(offsetof(CSWSpell, m_sImpactScript) == 0x50, "K1 CSWSpell::m_sImpactScript");
K2_LAYOUT_ASSERT(offsetof(CSWSpell, m_nConjureTimeMS) == 0x64, "K1 CSWSpell::m_nConjureTimeMS");
K2_LAYOUT_ASSERT(offsetof(CSWSpell, m_nCastPauseMS) == 0xcc, "K1 CSWSpell::m_nCastPauseMS");
K2_LAYOUT_ASSERT(offsetof(CSWSpell, m_nCategory) == 0x148, "K1 CSWSpell::m_nCategory");
K2_LAYOUT_ASSERT(offsetof(CSWSpell, m_paSubRadialSpells) == 0x150, "K1 CSWSpell::m_paSubRadialSpells");
K2_LAYOUT_ASSERT(offsetof(CSWSpell, m_nUserType) == 0x168, "K1 CSWSpell::m_nUserType");
K2_LAYOUT_ASSERT(offsetof(CSWSpell, m_nBaseForcePoints) == 0x176, "K1 CSWSpell::m_nBaseForcePoints");
K2_LAYOUT_ASSERT(offsetof(CSWSpell, m_sPrerequisites) == 0x178, "K1 CSWSpell::m_sPrerequisites");
K2_LAYOUT_ASSERT(offsetof(CSWSpell, m_anPrereq1) == 0x198, "K1 CSWSpell::m_anPrereq1");
K2_LAYOUT_ASSERT(offsetof(CSWSpell, m_nExclusionMask) == 0x1c0, "K1 CSWSpell::m_nExclusionMask");
K2_LAYOUT_ASSERT(offsetof(CSWSpell, m_nItemTargeting) == 0x1cc, "K1 CSWSpell::m_nItemTargeting");
K2_LAYOUT_ASSERT(sizeof(CSWSpell) == 0x1d0, "K1 CSWSpell is 0x1d0 (CSWSpellArray::Load's new[] stride)");
#else
K2_LAYOUT_ASSERT(sizeof(CSWSpell) == 0x1e8, "CSWSpell must be 0x1e8 (CSWSpellArray::GetSpell's stride)");
#endif

class CSWSpellArray {
public:
    int       m_nCount;
    char      _pad04[4];
    CSWSpell* m_paSpells;

    CSWSpellArray();

    ~CSWSpellArray();

    CSWSpell* GetSpell(int spellId) const;

    void Load();

    CSWSpell operator[](int index) const;
};
