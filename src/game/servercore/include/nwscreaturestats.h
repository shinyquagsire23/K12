#pragma once
#include "k2stdint.h"

struct CSWSStats_FeatRemainingUses {
    unsigned short m_nFeat;
    unsigned char  m_nUsesUsed;
};

#include "exolocstring.h"
#include "exoresman.h"
#include "CExoArrayListDtors.h"
#include "nwsobject.h"
#include "nwclass.h"

class CResGFF;
struct CResStruct;
struct CSWSCreatureAppearanceInfo;

class CSWLevelStats;
class CSWCreatureStatsUpdate;
class CCombatInformation;
class CSWSCreature;
class CSWSItem;
class CSWSpell;
class CSWFeat;
class CSWSStats_SpellLikeAbility;

// One per-class record of a multiclassed creature, indexed by class slot.
struct CSWSCreatureStats_ClassInfo {
    CExoArrayList<unsigned int> m_knownSpells[2];
    unsigned char m_nReserved1;
    unsigned char m_spellsPerDayLeft;
    unsigned char m_maxSpellsPerDayLeft;
    unsigned char m_class;
    unsigned char m_level;
    unsigned char m_negativeLevels;
    unsigned char m_nReserved2;
    unsigned char m_nReserved3;
    unsigned char m_nReserved4;
    unsigned char _pad29[0x2c - 0x29];
    int           m_hasLostClassAbilities;

    unsigned char GetSpellsPerDayLeft(unsigned char level);
    void          SetSpellsPerDayLeft(unsigned char level, unsigned char value);
    unsigned char GetMaxSpellsPerDayLeft(unsigned char level);
    void          SetMaxSpellsPerDayLeft(unsigned char level, unsigned char value);
    void          DecrementSpellsPerDayLeft(unsigned char level);
    void          IncrementSpellsPerDayLeft(unsigned char level);
    void          RemoveKnownSpell(unsigned char level, unsigned int nSpellId);
    int           GetKnownSpell(unsigned char nLevel, unsigned char nIndex);
    void          ResetSpellsPerDayLeft(unsigned char nLevel, unsigned char nValue);
    unsigned int  GetNumberKnownSpells(unsigned char nLevel);
    int           HasSpell(unsigned char nLevel, unsigned int nSpellId);

    CSWSCreatureStats_ClassInfo();
    ~CSWSCreatureStats_ClassInfo();

    // Adds a level-0 spell unless the creature already knows it; other levels are ignored.
    void          AddKnownSpell(unsigned char nLevel, unsigned int nSpellId);
};

class CSWSCreatureStats {
public:
    // Feat ids (feat.2da rows) the engine tests that nwscript.nss does not name; the rest are the
    // FEAT_* constants of nwscriptconst.h. Named after feat.2da's LABEL column.
    enum {
        FEAT_WOOKIEE_TOUGHNESS        = 95,
        FEAT_BLASTER_INTEGRATION      = 96,
        FEAT_FORCE_CAMOFLAGE          = 97,
        FEAT_FORCE_IMMUNITY_FEAR      = 98,
        FEAT_FORCE_IMMUNITY_STUN      = 99,
        FEAT_FORCE_IMMUNITY_PARALYSIS = 100,
        FEAT_FORCE_JUMP               = 101,
        FEAT_FORCE_JUMP_ADVANCED      = 102,
        FEAT_FORCE_JUMP_MASTERY       = 103,
        FEAT_DUELING                  = 113,
        FEAT_ADVANCED_DUELING         = 114,
        FEAT_MASTER_DUELING           = 115,
        FEAT_FORCE_SENSITIVE          = 116,
        FEAT_IMPROVED_CAUTION         = 117,
        FEAT_MASTER_CAUTION           = 118,
        FEAT_GEAR_HEAD_ADEPT          = 119,
        FEAT_GEAR_HEAD_MASTER         = 120,
        FEAT_IMPROVED_EMPATHY         = 121,
        FEAT_MASTER_EMPATHY           = 122,
        FEAT_MASTER_TOUGHNESS         = 124,
        FEAT_COMPLEX_UNARMED_ANIMS    = 207,
        FEAT_ECHANI_STRIKE_I          = 209,
        FEAT_ECHANI_STRIKE_II         = 210,
        FEAT_ECHANI_STRIKE_III        = 211,
        FEAT_UNARMED_SPECIALIST_I     = 212,
        FEAT_UNARMED_SPECIALIST_II    = 213,
        FEAT_UNARMED_SPECIALIST_III   = 214,
        FEAT_UNARMED_SPECIALIST_IV    = 215,
        FEAT_UNARMED_SPECIALIST_V     = 216,
        FEAT_UNARMED_SPECIALIST_VI    = 217,
        FEAT_UNARMED_SPECIALIST_VII   = 218,
        FEAT_UNARMED_SPECIALIST_VIII  = 219,
        FEAT_SHIELD_BREAKER           = 220,
        FEAT_REPULSOR_STRIKE          = 221,
        FEAT_ELECTRICAL_STRIKE        = 222,
        FEAT_GRAVITONIC_STRIKE        = 223,
        FEAT_WOOKIEE_TOUGHNESS_II     = 224,
        FEAT_WOOKIEE_TOUGHNESS_III    = 225,
        FEAT_ASSASSIN_PROTOCOL_I      = 228,
        FEAT_ASSASSIN_PROTOCOL_II     = 229,
        FEAT_ASSASSIN_PROTOCOL_III    = 230,
        // KOTOR 2's rows; KOTOR 1's feat.2da ends before them, but its build compiles the
        // radial-menu builders (CSWCCreature::GetWookieeRage, GetDroidConfusion) that test them.
        FEAT_WOOKIEE_RAGE_I           = 231,
        FEAT_WOOKIEE_RAGE_II          = 232,
        FEAT_WOOKIEE_RAGE_III         = 233,
        FEAT_DROID_TRICK              = 234,
        FEAT_DROID_CONFUSION          = 235,
        FEAT_MINE_IMMUNITY            = 238,
#if KOTOR1
        // KOTOR 2's class-skill feat rows, which HasClassSkillFeat tests in both games; KOTOR 1's
        // feat.2da ends before them. KOTOR 2 has these names from nwscript.nss.
        FEAT_CLASS_SKILL_AWARENESS    = 183,
        FEAT_CLASS_SKILL_COMPUTER_USE = 184,
        FEAT_CLASS_SKILL_DEMOLITIONS  = 185,
        FEAT_CLASS_SKILL_REPAIR       = 186,
        FEAT_CLASS_SKILL_SECURITY     = 187,
        FEAT_CLASS_SKILL_STEALTH      = 188,
        FEAT_CLASS_SKILL_TREAT_INJURY = 189,
#endif
    };

    CExoArrayList<unsigned short> m_feats;
    CExoArrayList<CSWSStats_FeatRemainingUses*> m_featRemainingUses;
    CExoArrayList<unsigned short> m_bonusFeats;
    CSWSCreature* m_pOwner;
    CExoArrayList<CSWLevelStats*> m_levelStats;
    CExoLocString m_firstName;
    CExoLocString m_lastName;
    CResRef m_conversation;
    int  m_nInterruptable;
    unsigned char _pad7c[0x80-0x7c];
    CExoLocString m_description;
    int  m_nAge;
    unsigned char m_nGender;
    unsigned char _pad95[0x98-0x95];
    int  m_nExperience;
    int  m_bIsPC;
    int  m_nReserved1;
    int  m_nReserved2;
    int  m_factionId;
    int  m_nJoinedFactionId;
    unsigned char _padb0[0xb4 - 0xb0];
#if !KOTOR1  // K1 has no "BlindSpot" GFF field and no member here.
    float m_fBlindSpot;
#endif
    float m_fChallengeRating;
    unsigned char m_nStartingPackage;
    unsigned char m_numClasses;
#if KOTOR1
    unsigned char _padba[0xc0 - 0xba];
#else
    unsigned char _padbe[0xc0 - 0xbe];
#endif
    // One entry per class, and KOTOR2 multiclassing tops out at two. An index past the bound aliases
    // a later member (index 2 lands on m_nCHAMod), so those reads are misnamed but harmless.
    CSWSCreatureStats_ClassInfo m_classInfo[2];
    unsigned short m_nRace;
    unsigned char _pad122[0x128 - 0x122];
    CExoString m_sSubrace;
    unsigned char m_nSubraceIndex;
    unsigned char m_nSTRBase;
    unsigned char m_nSTRMod;
    unsigned char m_nDEXBase;
    signed char   m_nDEXMod;
    unsigned char m_nCONBase;
    signed char   m_nCONMod;
    unsigned char m_nINTBase;
    unsigned char m_nINTMod;
    unsigned char m_nWISBase;
    signed char   m_nWISMod;
    unsigned char m_nCHABase;
    unsigned char m_nCHAMod;
    unsigned char m_nACNaturalBase;
    unsigned char m_nACArmorBase;
    unsigned char m_nACShieldBase;
    unsigned char m_nACArmorMod;
    unsigned char m_nACArmorNeg;
    unsigned char m_nACDeflectionMod;
    unsigned char m_nACDeflectionNeg;
    unsigned char m_nACShieldMod;
    unsigned char m_nACShieldNeg;
    unsigned char m_nACNaturalMod;
    unsigned char m_nACNaturalNeg;
    signed char   m_nACDodgeMod;
    signed char   m_nACDodgeNeg;
    unsigned char m_nOverrideBaseAttackBonus;
    unsigned char m_nEnemyAttackBonus;
    unsigned char _pad154[0x160 - 0x154];
    int  m_nReserved3;
    int  m_nReserved4;
    int  m_nReserved5;
    unsigned char m_nReserved6;
    unsigned char _pad16d[0x170 - 0x16d];
    CCombatInformation* m_pCombatInformation;
#if !KOTOR1  // K1 has neither the "FuryDamageBonus" byte nor the video-effect word.
    signed char   m_nFuryDamageBonus;
    unsigned char _pad179[0x17c - 0x179];
    int  m_nVideoEffectType;
#endif
    unsigned char m_nArcaneSpellFailure;
    unsigned char m_nReserved7;
    unsigned char m_nReserved8;
    unsigned char m_forceResistance;
    signed char   m_forceResistancePenalty;
    unsigned char _pad185[0x186 - 0x185];
    unsigned short m_nBaseForcePoints;
    unsigned short m_nCurrentForcePoints;
    unsigned short m_nTemporaryForcePoints;
    unsigned char _pad18c[0x190 - 0x18c];
    CExoArrayList<CSWSStats_SpellLikeAbility>* m_pSpellLikeAbilityList;
    unsigned short m_iFirstTempHitPoints;
    unsigned short m_iFirstACIncrease;
    unsigned short m_iFirstAttackIncrease;
    unsigned short m_iFirstDamageReduction;
    unsigned short m_iFirstDamageResistance;
    unsigned short m_iFirstDamageIncrease;
    unsigned short m_iFirstSetAIState;
    unsigned short m_aiState;
    unsigned short m_iFirstEffectIcon;
    unsigned short m_iFirstSanctuary;
    unsigned short m_iFirstInvisibility;
    unsigned short m_iFirstForceResistanceIncrease;
    unsigned short m_iFirstAbilityIncrease;
    unsigned short m_iFirstSeeInvisible;
    unsigned short m_iFirstUltravision;
    unsigned short m_iFirstMissChance;
    unsigned short m_iFirstConcealment;
    unsigned short m_iFirstBlindness;
    unsigned short m_iFirstSetWalkAnimation;
    unsigned short m_iFirstSpellLevelAbsorption;
    unsigned short m_iFirstSpellImmunity;
    unsigned short m_iFirstDamageShield;
    unsigned short m_iFirstSkillIncrease;
    unsigned short m_iFirstSavingThrowIncrease;
    unsigned short m_iFirstHaste;
    unsigned short m_iFirstImmunity;
    unsigned short m_iFirstLimitMovementSpeed;
    unsigned short m_iFirstMovementSpeedDecrease;
    unsigned short m_nSkillPoints;
    unsigned char _pad1d2[0x1d8 - 0x1d2];
    char* m_pSkillRanks;
    unsigned short m_nArmorCheckPenalty;
    CResRef m_portrait;
    short m_nAlignment;
#if KOTOR1  // K1 has no "BaseCNPCAlignment" field.
    unsigned char _pad1ec[0x1ee - 0x1ec];
#else
    short m_nBaseNPCAlignment;
    unsigned char _pad1f6[0x1f8 - 0x1f6];
#endif
    unsigned char m_nColorSkin;
    unsigned char m_nColorHair;
    unsigned char m_nColorTattoo1;
    unsigned char m_nColorTattoo2;
    unsigned short m_nAppearanceType;
    unsigned char m_nPhenotype;
    unsigned char m_nAppearanceHead;
    signed char   m_bDuplicatingHead;
#if KOTOR1
    unsigned char _pad1f7[0x1f8 - 0x1f7];
#else
    unsigned char _pad201[0x204 - 0x201];
#endif
    int  m_nUseBackupHead;
    unsigned char m_bTail;
    unsigned char m_bWings;
    unsigned char _pad20a[0x20c - 0x20a];
    int   m_nMovementRate;
    float m_fRunRate;
    float m_fWalkRate;
    signed char   m_nFortSaveBonus;
    signed char   m_nWillSaveBonus;
    signed char   m_nReflexSaveBonus;
#if KOTOR1
    unsigned char _pad20f[0x210 - 0x20f];
#else
    unsigned char _pad21b[0x220 - 0x21b];
#endif
    CExoString m_sDeity;
    float m_hpAccumulator;
    float m_fpAccumulator;
    unsigned char _pad238[0x240 - 0x238];

    CSWSCreatureStats(CSWSCreature* pOwner);
    ~CSWSCreatureStats();

    void    SetPortrait(CResRef cPortrait) { m_portrait = cPortrait; }
    CResRef GetPortrait() { return m_portrait; }
    CResRef GetConversation() { return m_conversation; }

    unsigned char GetPotentialLevel();
    unsigned char GetEffectiveCRForPotentialLevel();
    // The six effective ability scores, each the base byte plus whatever effects and equipment add.
    unsigned char GetSTRStat();
    unsigned char GetDEXStat();
    unsigned char GetCONStat();
    unsigned char GetINTStat();
    unsigned char GetWISStat();
    unsigned char GetCHAStat();
    void UpdateSTRModifier() { m_nSTRMod = static_cast<unsigned char>(CalcStatModifier(GetSTRStat())); }
    void UpdateDEXModifier() { m_nDEXMod = static_cast<signed char>(CalcStatModifier(GetDEXStat())); }
    void UpdateCONModifier() { m_nCONMod = static_cast<signed char>(CalcStatModifier(GetCONStat())); }
    void UpdateINTModifier() { m_nINTMod = static_cast<unsigned char>(CalcStatModifier(GetINTStat())); }
    void UpdateWISModifier() { m_nWISMod = static_cast<signed char>(CalcStatModifier(GetWISStat())); }
    void UpdateCHAModifier() { m_nCHAMod = static_cast<unsigned char>(CalcStatModifier(GetCHAStat())); }
    // The base failure plus the two modifiers after it, capped at 100 (percent).
    unsigned char GetTotalArcaneSpellFailure()
    {
        unsigned int nTotal = m_nArcaneSpellFailure + m_nReserved7 + m_nReserved8;
        return static_cast<unsigned char>(nTotal >= 100 ? 100 : nTotal);
    }
    signed char GetTotalACNaturalMod() { return static_cast<signed char>(m_nACNaturalMod - m_nACNaturalNeg + GetACNaturalBase()); }
    void SetDialog(const CResRef& cDialog) { m_conversation = cDialog; }
    int  CanLevelUp();
    int  GetIsInKnownSpellList(unsigned char classIndex, unsigned int nSpellId);
    int  CheckInclusionMatch(int a1, int a2, int a3);
    unsigned char GetSpellUsesLeft(unsigned int nSpellId, unsigned char classIndex,
                          unsigned char a3, unsigned char a4);
    // Walks the class slots and asks each Jedi class's known-power variant for a random talent.
    int  GetCreatureTalentRandomPower(int a1, int a2, int& a3, int& a4,
                                      unsigned char& a5, int a6, int a7);
    int  GetCreatureTalentRandomKnownPower(int a1, int a2, int& a3, int& a4,
                                           unsigned char& a5, int a6, int a7, int a8);
    // Re-rolls between the power path and the feat path until one returns a talent.
    int  GetCreatureTalentRandom(int a1, int a2, int& a3, int& a4, unsigned char& a5,
                                 unsigned long long& a6, int& a7, unsigned char& a8,
                                 int a9, int a10, int a11);
    int  GetCreatureTalentRandomSpecialAbility(int a1, int a2, int& a3, int& a4,
                                               unsigned char& a5, unsigned char& a6,
                                               int a7, int a8);
    int  GetCreatureTalentRandomFeat(int a1, int a2, int& a3, int& a4,
                                     unsigned char& a5, int a6, int a7);
    int  GetCreatureTalentRandomPowerFromItem(int a1, int a2, int& a3, int& a4, unsigned char& a5,
                                              unsigned long long& a6, int& a7,
                                              int a8, int a9);
    // For a player character, queues a script event on the module's AI master, applies the level
    // and refreshes combat state. Returns 1 when the level was applied.
    int  ValidateLevelUp(CSWLevelStats* pLevelStats);
    void LevelUp(CSWLevelStats* pLevelStats, int bAllowRecalc);
    void LevelDown(CSWLevelStats* pLevelStats);
    // Copies the stats groups named by the bits of nUpdateFields into the update object the
    // server ships to the client.
    void UpdateLastStatsObject(unsigned long long nUpdateFields,
                               CSWCreatureStatsUpdate* pUpdate);
    void AutoLevelup(int a);
    unsigned char GetLevel(int a);
    // The advancement record for a 0-based character level, or null for a player character or a
    // level past the end of the array.
    CSWLevelStats* GetLevelStats(unsigned char nLevel);
    void SetMovementRate(int rate);
    int  GetHasPowerFromItem(int nPower);
    // Like GetHasPowerFromItem, but returns the item's object id and the property's index in the
    // item's list on a hit.
    int  GetItemDetailsForPower(int nPower, unsigned long long& rItemId, int& rPropIndex);

    char GetTotalSTRBonus();
    char GetTotalDEXBonus();
    char GetTotalCONBonus();
    char GetTotalINTBonus();
    char GetTotalWISBonus();
    char GetTotalCHABonus();

    // SetCONBase's second argument is the adjust-hit-points flag.
    void SetSTRBase(unsigned char nScore);
    void SetDEXBase(unsigned char nScore);
    void SetINTBase(unsigned char nScore);
    void SetWISBase(unsigned char nScore);
    void SetCONBase(unsigned char nScore, int bAdjustHP);
    void SetCHABase(unsigned char nScore);
#if KOTOR1
    // K1: one argument, with no companion base-alignment arm.
    void ModifyAlignment(short nDelta);
    void ModifyAlignment(short nDelta, int) { ModifyAlignment(nDelta); }
    int  GetSimpleAlignmentGoodEvil();
#else
    void ModifyAlignment(short nDelta, int a);
#endif

    void SetClass(unsigned char classIndex, unsigned char classId);
    void SetClassNegativeLevels(unsigned char classIndex, unsigned char negativeLevels);
    void SetHasLostClassAbilities(unsigned char classIndex, int hasLost);
    // Returns 255 for an out-of-range class slot.
#if KOTOR1
    unsigned char GetClass(unsigned char classIndex);
#else
    unsigned char GetClass(unsigned char classIndex) {
        return (classIndex < m_numClasses) ? m_classInfo[classIndex].m_class : 0xff;
    }
#endif
    // Unlike the client class's version, this one takes a negative-levels flag.
    unsigned char GetClassLevel(unsigned char classIndex, int bApplyNegativeLevels);
    unsigned char GetSpellGainWithBonus(unsigned char classIndex, unsigned char spellLevel);

    // In the GFF round trip, ReadStatsFromGff fills the stats block and the owning creature's
    // appearance sub-object, SaveStats writes it back. The read returns 0 on success.
    int64_t ReadStatsFromGff(CResGFF* pGFF, CResStruct* pStruct, CSWSCreatureAppearanceInfo* pAppearance);
    void SaveStats(CResGFF* pGFF, CResStruct* pStruct);
    void SaveClassInfo(CResGFF* pGFF, CResStruct* pStruct);
    void ReadSpellsFromGff(CResGFF* pGFF, CResStruct* pStruct);
    // The class-slot and spell-level accessor, distinct from the per-record one above.
    unsigned char GetSpellsPerDayLeft(unsigned char classIndex, unsigned char level);
#if KOTOR1
    // K1 and the four below have no level-up parameter.
    unsigned int ComputeNumberKnownSpellsLeft(unsigned char classIndex);
    unsigned int ComputeNumberKnownSpellsLeft(unsigned char classIndex, int)
    {
        return ComputeNumberKnownSpellsLeft(classIndex);
    }
#else
    unsigned int ComputeNumberKnownSpellsLeft(unsigned char classIndex, int bLevelUp);
#endif
    void ResetSpellsPerDayLeft(unsigned char classIndex, unsigned char nSpellLevel);
    // The live known-spell list for a class slot and spell level.
    unsigned short GetNumberKnownSpells(unsigned char classIndex, unsigned char level);
    // Returns -1 for an out-of-range index.
    int GetKnownSpell(unsigned char classIndex, unsigned char level,
                      unsigned char nIndex);
    int  GetIsJedi();
    unsigned char GetClassNegativeLevels(unsigned char classIndex);
    int  GetHasLostClassAbilities(unsigned char classIndex);
    char GetForceResistance();
    int  GetBaseAttackBonus();
    int  GetCriticalHitMultiplier(int bWieldingOffhand);
    int  GetCriticalHitRoll(int bWieldingOffhand);
    int  GetMeleeDamageBonus(int bWieldingOffhand);
    int  GetRangedDamageBonus();
    // The weapon's full damage range as an out-parameter pair, unarmed when the creature or item
    // is null. bWieldingOffhand selects the half-strength ability scale, bAddEffects adds the
    // effect bonus.
    void GetTotalDamageRange(CSWSCreature* pCreature, CSWSItem* pItem,
                             int& nMin, int& nMax, int bWieldingOffhand, int bAddEffects);
    // Rolls the per-attack damage: weapon or unarmed dice with the autobalance and difficulty
    // scalers, sneak dice, the damage bonus, massive-critical property dice and the effect bonus,
    // with side effects on the attack data and the combat log.
    int  GetDamageRoll(CSWSObject* pTarget, int bOffhand, int bCritical, int bSneakAttack,
                       int bMaxDamage, int bAddDamage);
    // Looks up a stat by id: 0 age, 1 challenge rating, 2 to 4 the Fort/Reflex/Will bonuses, 5 to
    // 10 the six ability bases in STR, DEX, CON, INT, WIS, CHA order, 11 the base attack bonus and
    // 12 the force resistance. Out-of-range returns 0.0f.
    float GetStatById(int nStatId);
    void SetStatById(int nStatId, float fValue);
    short AdjustHPAccumulatorBy(float delta);
    CExoString& GetTag();
    unsigned short GetFeat(unsigned short nFeat);
    unsigned short GetBonusFeat(unsigned short nFeat);
    unsigned char GetFeatRemainingUses(unsigned short nFeat);
    // Each of the three sums the class-table base, the paired ability modifier, the active-effect
    // total and its own per-creature bonus byte. A nonzero argument skips only the effect query.
    // Reflex alone caps its ability modifier: a positive dexterity bonus is dropped while the
    // creature is debilitated, a negative one always applies.
    char GetFortSavingThrow(int bIgnoreEffects);
    char GetWillSavingThrow(int bIgnoreEffects);
    char GetReflexSavingThrow(int bIgnoreEffects);
    // The class-table halves of the three above.
    char GetBaseFortSavingThrow();
    char GetBaseWillSavingThrow();
    char GetBaseReflexSavingThrow();
    // The effective rank in the skill; the object and the trailing int carry the situational
    // modifier context, both zero at the health-regeneration call sites.
    char GetSkillRank(unsigned char nSkill, CSWSObject* pObject, int a);
    // Maps the skills.2da row to its class-skill feat and asks HasFeat. Persuade has no such feat
    // and falls through to a sentinel HasFeat never matches.
    int  HasClassSkillFeat(unsigned short nSkill);
    int  HasFeat(unsigned short nFeat);
    int  HasSpell(unsigned char nSpellLevel, unsigned int nSpellId, int a);
    int  UnReadySpell(unsigned int nSpellId);
#if KOTOR1
    int  HasSpellPrereq(CSWSpell* pSpell, CExoArrayList<unsigned int>* pChosenSpells);
    int  HasSpellPrereq(CSWSpell* pSpell, CExoArrayList<unsigned int>* pChosenSpells, int)
    {
        return HasSpellPrereq(pSpell, pChosenSpells);
    }
#else
    int  HasSpellPrereq(CSWSpell* pSpell, CExoArrayList<unsigned int>* pChosenSpells, int bLevelUp);
#endif
    int  HasForcePowerRequirments(unsigned int nSpellId);
    int  HasFeatPrereq(CSWFeat* pFeat, CExoArrayList<unsigned short>* pFeats);
#if KOTOR1
    int  FeatRequirementsMet(unsigned short nFeat,
                             CExoArrayList<unsigned short>* pChosenFeats);
    int  FeatRequirementsMet(unsigned short nFeat, CExoArrayList<unsigned short>* pChosenFeats, int)
    {
        return FeatRequirementsMet(nFeat, pChosenFeats);
    }
    int  CanChooseFeat(unsigned short nFeat, unsigned char nNormalLeft, unsigned char nBonusLeft,
                       CExoArrayList<unsigned short>* pChosenFeats);
    int  CanChooseFeat(unsigned short nFeat, unsigned char nNormalLeft, unsigned char nBonusLeft,
                       CExoArrayList<unsigned short>* pChosenFeats, int)
    {
        return CanChooseFeat(nFeat, nNormalLeft, nBonusLeft, pChosenFeats);
    }
#else
    int  FeatRequirementsMet(unsigned short nFeat,
                             CExoArrayList<unsigned short>* pChosenFeats, int bLevelUp);
    int  CanChooseFeat(unsigned short nFeat, unsigned char nNormalLeft, unsigned char nBonusLeft,
                       CExoArrayList<unsigned short>* pChosenFeats, int bLevelUp);
#endif
    int  FeatAcquired(unsigned short nFeat, CExoArrayList<unsigned short>* pFeats);
#if KOTOR1
    int  CanChooseSpell(unsigned int nSpellId, unsigned char nSpellsLeft,
                        CExoArrayList<unsigned int>* pChosenSpells);
    int  CanChooseSpell(unsigned int nSpellId, unsigned char nSpellsLeft,
                        CExoArrayList<unsigned int>* pChosenSpells, int)
    {
        return CanChooseSpell(nSpellId, nSpellsLeft, pChosenSpells);
    }
#else
    int  CanChooseSpell(unsigned int nSpellId, unsigned char nSpellsLeft,
                        CExoArrayList<unsigned int>* pChosenSpells, int bLevelUp);
#endif
    int  GetCanUseSkill(unsigned char nSkill);

    // The debug menu's feat, power and skill grant handlers.
    void SetSkillRank(unsigned char nSkill, char nRank);
    void AddFeat(unsigned short nFeat);
    void AddKnownSpell(unsigned char nClass, unsigned int nSpell);
    void RemoveKnownSpell(unsigned char classIndex, unsigned int nSpellId);
    int  GetCreatureHasTalent(int a, int b);
    // Arguments four to seven are unused; the eighth is compared against the first.
    int  CheckPowerSuitability(int nTalentId, CSWSpell* pSpell, int nMask,
                               int a4, int a5, int a6, int a7, int a8);
    int  EffectId2Effect2DAId(int nEffectId, int nSubType);
    void SetNormalBonusFlags(unsigned short nFeat, int& bNormal, int& bBonus);

    // The D&D ability modifier, (nStat - 10) / 2 rounded toward zero.
    int  CalcStatModifier(unsigned char nStat);
    // The three mental abilities also refresh the per-day spell uses; strength and dexterity do
    // not.
    void AdjustSpellUsesPerDay();
    // The effective dexterity modifier, optionally capped by the equipped chest armour's maximum
    // dexterity bonus (the base item's inherent cap plus the item's accumulated property).
#if KOTOR1
    char GetDEXMod(int a);
#else
    int  GetDEXMod(int a);
#endif
    // Returns the total melee attack bonus: effect bonus, base attack bonus, dexterity over
    // strength under finesse, weapon focus, the lightsaber critical-tier feats and the dual-wield
    // and off-hand penalty chains.
    int  GetMeleeAttackBonus(int a1, int a2, int a3, int a4);
    // The ranged counterpart, with dexterity always. The third argument is unused.
    int  GetRangedAttackBonus(int a1, int a2, int a3);
    // The class's primary ability modifier, selected by the classes.2da primary-ability column.
    // An out-of-range class slot falls through to the strength arm.
    unsigned char GetPrimaryMod(unsigned char classIndex);

    // The shipped rules leave these empty or constant.
    void SetArcaneSpellFailure(unsigned char nFailure);
    unsigned char GetAttacksPerRound();
    int  GetSpellMinAbilityMet(unsigned char a, unsigned char b);
    int  GetFavoredEnemyBonus(CSWSCreature* pTarget);
    // Switches on the current attack's feat id; the target argument is unused.
    int  ResolveSpecialAttackDamageBonus(CSWSCreature* pTarget);
    int  ResolveSpecialAttackAttackBonus(CSWSCreature* pTarget);

    void SetForceResistance(char nResistance);
    void SetForceResistancePenalty(char nPenalty);
    void UpdateCombatInformation();
    // The total damage bonus for the current attack. The strength-modifier and weapon-
    // specialisation pieces are side-channel stores into the attack data, not part of the sum.
    char GetDamageBonus(CSWSCreature* pCreature, int bWieldingOffhand, int a3);
    // The defender's armour class against the given attacker's current attack. Also stores the
    // per-attack breakdown into the attack data and appends the combat-debug lines when
    // g_bEnableCombatDebugging is on.
    short GetArmorClassVersus(CSWSCreature* pCreature, int bVersus);
    // The attacker's attack modifier against the target with the current weapon, with the same
    // breakdown and combat-debug lines.
    int  GetAttackModifierVersus(CSWSCreature* pTarget);
    // Applies the multiclass experience penalty (uneven class levels cost 20 percent each) before
    // forwarding to SetExperience; a single-class creature tail-calls it directly.
    void AddExperience(unsigned int nExp);
    void SetExperience(unsigned int nExp, int b);
    // The force-point twin of AdjustHPAccumulatorBy.
    short AdjustFPAccumulatorBy(float fDelta);

    unsigned char GetACNaturalBase();
    int  GetTotalACFeatMod();
    int  GetUnarmedDamageDice();
    int  GetUnarmedDamageDie();
    int  GetUseMonkAttackTables(int a);
    int  GetUnarmedDamageRoll(CSWSObject* pObject);

    unsigned int GetExpNeededForLevelUp();
    unsigned int CalculateExpNeededForNextLevel();
    void ResetFeatRemainingUses();
    // Removes the feat and deletes every counter entry tracking it.
    void RemoveFeat(unsigned short nFeat);
    // The class slot holding the given class, or 255 when the creature has no such class.
    unsigned char GetIsClass(unsigned char nClass);

    void DecrementSpellsPerDayLeft(unsigned char classIndex, unsigned char nLevel);
    void IncrementSpellsPerDayLeft(unsigned char classIndex, unsigned char nLevel);

    // Sets the class slot's level and, for a player-made character only, unlocks the level-30
    // achievement on the way past.
    void SetClassLevel(unsigned char classIndex, unsigned char nLevel);
    // Drops the feat list and deletes every limited-use-feat counter. m_feats is released with
    // the list's clear, while m_featRemainingUses is drained back to front because its element
    // type is a pointer the block owns.
    void ClearFeats();
    // Writes the owning object's tag, lowercased: tags are case-insensitive and are normalised on
    // the way in rather than on lookup.
    void SetTag(const CExoString& sTag);
    // Returns the class slot's caster level, its class level when classes.2da marks the class a
    // caster, otherwise 0.
    unsigned char GetCasterLevel(unsigned char classIndex);
    // Spends one use of the feat, which increments the entry's uses-spent counter; every entry
    // for the feat is bumped, not just the first match.
    void DecrementFeatRemainingUses(unsigned short nFeat);
    // The caster level recorded for a spell-like ability, or 255 when it is not in the list.
    unsigned char GetSpellLikeAbilityCasterLevel(unsigned int nSpellId);
    // True when the spell is in the list and currently ready.
    int  GetIsInSpellLikeAbilityList(unsigned int nSpellId);
    // Marks one spell-like ability ready or not, matched on spell id and caster level and skipped
    // when the flag already holds the requested value.
    void SetSpellLikeAbilityReady(unsigned int nSpellId, unsigned char nReady,
                                  unsigned char nCasterLevel);
    void AddSpellLikeAbilityToList(unsigned int nSpellId, unsigned char nReady,
                                   unsigned char nCasterLevel);
    // Refreshes on rest, so every spell-like ability is ready again.
    void ResetSpellLikeAbilities();

    // The class slot's display name from classes.2da, empty for an out-of-range slot.
    CExoString GetClassString(unsigned char classIndex);
    // The subrace string if set, otherwise the racialtypes.2da row's own name.
    CExoString GetRaceString();
    // The alignment string from the three buckets of m_nAlignment; plain literals, not
    // text-table lookups.
    CExoString GetAlignmentString();
    // First and last name joined through the server's localised-string resolver.
    CExoString GetFullName();
    // The portrait resref with the size suffix appended; portraits ship as a stem plus S or L.
    CResRef GetSmallPortrait();
    CResRef GetLargePortrait();

    // Appends a class slot, capped at two, sets its level, then repeats SetClassLevel's level-30
    // achievement check and refreshes the owner's alignment-gated powers.
    void AddClass(unsigned char nClass, unsigned char nLevel);
    // How many normal and bonus feats the next character level grants, read from the last class
    // slot's classes.2da per-level tables. The first parameter is unused: the slot is always the
    // last class, never the one passed in.
    void CalcLevelUpNumberFeats(unsigned char classIndex, unsigned char& nNormalFeats,
                                unsigned char& nBonusFeats);
    // Matches a category nibble by nibble. For each of the four nibbles the mask either holds the
    // wildcard or must equal the category's nibble.
    int  CheckForCategoryMatch(int nCategory, int nMask);
    // Whether the creature has the Weapon Focus or Weapon Specialization feat for the item's base
    // item. Both return 0 for a null item, a null base item, or a base item with no such feat.
    int  GetWeaponFocus(CSWSItem* pItem);    int  GetWeaponSpecialization(CSWSItem* pItem);
    // Whether dexterity may replace strength for this weapon's attack rolls: the effective
    // dexterity modifier must beat the strength one, and the creature must hold the finesse feat
    // matching the weapon.
    int  GetWeaponFinesse(CSWSItem* pItem);
    // Whether the creature is immune to the given effect family. Scans the owner's sorted effect
    // list from the first-immunity index; an effect matches on type, race and alignment band. A
    // null creature means use the rules defaults.
    int  GetEffectImmunity(unsigned char nImmunity, CSWSCreature* pCreature);
    int  GetEffectLinkImmunity(CGameEffect* pEffect);
};
#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CSWSCreatureStats) == 0x230, "K1 CSWSCreatureStats size");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_fChallengeRating) == 0xb4, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_numClasses) == 0xb9, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_classInfo) == 0xc0, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_pCombatInformation) == 0x170, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_nArcaneSpellFailure) == 0x178, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_forceResistance) == 0x17b, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_nBaseForcePoints) == 0x17e, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_pSpellLikeAbilityList) == 0x188, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_iFirstTempHitPoints) == 0x190, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_aiState) == 0x19e, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_nSkillPoints) == 0x1c8, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_pSkillRanks) == 0x1d0, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_portrait) == 0x1da, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_nAlignment) == 0x1ea, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_nColorSkin) == 0x1ee, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_bDuplicatingHead) == 0x1f6, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_nUseBackupHead) == 0x1f8, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_nMovementRate) == 0x200, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_nFortSaveBonus) == 0x20c, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_sDeity) == 0x210, "K1");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCreatureStats, m_hpAccumulator) == 0x220, "K1");
#endif

// idiom-exempt: canonical
// A per-spell stats entry on the server side.
class CSWSStats_Spell {
public:
    CSWSStats_Spell();

    unsigned int m_nSpellId;
    int m_nReady;
    unsigned char m_nCasterLevel;
    int m_nExtra;
};

// idiom-exempt: canonical
// One entry in a creature's spell-like-ability list.
class CSWSStats_SpellLikeAbility {
public:
    CSWSStats_SpellLikeAbility();

    unsigned int m_nSpellId;
    int m_nReady;
    unsigned char m_nCasterLevel;
};
