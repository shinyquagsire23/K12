#pragma once

#include "exostring.h"
#include "Vector.h"
#include "CExoArrayListDtors.h"

class CGameEffect;
class CResGFF;
struct CResStruct;
class CSWCCMessageData;
class CSWSSpellScriptData;

// One attack's attack-modifier breakdown, filled in by GetAttackModifierVersus: the ability mods used
// (STR or DEX by ranged or finesse), the base attack bonus, the dual-wield flag, the two-weapon
// penalty bytes, the dueling pair, good aim, offensive training versus race, weapon and force focus,
// the effect attack bonus, battle meditation, the targeting, crush-opposition and inspire-followers
// pairs, the PC charisma bonus and the lightsaber-form pair.
class CSWSCombatSubAttackAttack {
public:
    CSWSCombatSubAttackAttack() { Clear(); }
    void Clear();

    unsigned char m_nReserved1;
    unsigned char m_nAttackRoll;
    signed char   m_nAttackModifier;
    signed char   m_nStrAttackMod;
    signed char   m_nDexAttackMod;
    signed char   m_nReserved2;
    signed char   m_nSpecialAttackBonus;
    signed char   m_nBaseAttackBonus;
    int           m_bOffhandAttack;
    signed char   m_nTwoWeaponPenalty;
    unsigned char m_nLightOffhandBonus;
    unsigned char m_nTwoWeaponFeatBonus;
    unsigned char m_nTwoWeaponFeat;
    unsigned char m_nDuelingBonus;
    unsigned char m_nDuelingFeat;
    unsigned char m_nGoodAimBonus;
    unsigned char m_nOffensiveTrainingBonus;
    unsigned char m_nWeaponFocusBonus;
    signed char   m_nEffectAttackBonus;
    // Real padding: Clear() skips it, and the following int needs the alignment.
    int           m_bAutomaticHit;
    int           m_bAutomaticMiss;
#if !KOTOR1  // K1's record ends here, with no battle-meditation or targeting fields
    unsigned char m_nBattleMeditationBonus;
    unsigned char m_nTargetingFeat;
    unsigned char m_nTargetingBonus;
    unsigned char m_nCrushOppositionSpell;
    unsigned char m_nCrushOppositionPenalty;
    unsigned char m_nInspireFollowersSpell;
    unsigned char m_nInspireFollowersBonus;
    unsigned char m_nPCCharismaBonus;
    int           m_nSaberFormId;
    unsigned char m_nSaberFormBonus;
#endif
    // No trailing pad member: the tail padding is implicit, and declaring it would change the data
    // size the implicit copy assignment copies.
};

// The threat breakdown CSWSCreature::SignalDamage ships.
class CSWSCombatSubAttackThreat {
public:
    CSWSCombatSubAttackThreat() { Clear(); }
    void Clear();

    unsigned char m_nThreatRange;
    char  _pad01[0x04 - 0x01];
    int           m_bCriticalThreat;
    unsigned char m_nConfirmRoll;
    char  _pad09[0x0c - 0x09];
    int           m_bCriticalConfirmed;
    unsigned char m_nReserved1;
    signed char   m_nCriticalMultiplier;
};

// The per-attack armor-class breakdown GetArmorClassVersus fills in. All nine slots are plain bytes,
// so the record is alignment 1 and nine bytes long.
class CSWSCombatSubAttackDefense {
public:
    CSWSCombatSubAttackDefense() { Clear(); }
    void Clear();

    signed char m_nArmorClass;
    signed char m_nACArmor;
    signed char m_nACDex;
    signed char m_nACDexUncapped;
    signed char m_nACClass;
    signed char m_nACNatural;
    signed char m_nACDodge;
    signed char m_nACFeatMod;
    signed char m_nACDebilitatedPenalty;
};

// The per-attack damage breakdown GetDamageRoll / GetDamageBonus / DoDamageResistance fill in.
class CSWSCombatSubAttackDamage {
public:
    CSWSCombatSubAttackDamage() { Clear(); }
    void Clear();

    unsigned char m_nDamageRoll;
    unsigned char m_nDamageBonus;
    unsigned char m_nReserved1;
    unsigned char m_nSneakAttackDamage;
    signed char   m_nStrDamageBonus;
    unsigned char m_nWeaponSpecializationBonus;
    unsigned char m_nFeatSoakAmount;
    unsigned char m_nFeatFlatDRAmount;
    unsigned char m_nDamageTypeBits;
    unsigned char m_nDamageAfterSoak;
    unsigned char m_nDamageAfterFlatDR;
#if KOTOR1  // K1 has no roll slot; the effect damage bonus sits here and the array follows
    signed char   m_nEffectDamageBonus;
#else
    unsigned char m_nSpellDamageReduction;
    int           m_nReductionSpellId;
    // Signed: an effect can carry a negative damage bonus.
    signed char   m_nEffectDamageBonus;
    char  _pad11[0x12 - 0x11];
#endif
    short         m_anReserved[15];
};

// The effect-application feedback record. SetAttackSummaryFeedbackData fills it in and SignalDamage's
// BroadcastEffectApplicationBreakdown ships it when the application-data flag is set.
class CSWSCombatSubAttackEffect {
public:
    CSWSCombatSubAttackEffect() { Clear(); }
    void Clear();

    // Cleared to OBJECT_INVALID, the same sentinel the target id gets. Also the member that gives the
    // record its 8-byte alignment.
    unsigned long long m_objectId;
    // The saving-throw breakdown, written in this order by CSWSCreature::SavingThrowRoll.
    unsigned char m_nSaveType;
    // Selects which immunity or feat check runs against the target.
    unsigned char m_nFeedbackEffectType;
    unsigned char m_nSaveVsType;
    unsigned char m_nSaveRoll;
    unsigned char m_nSaveEffectBonus;
    unsigned char m_nSaveBase;
    unsigned char m_nSaveTotal;
    unsigned char m_nSaveDC;
    // Verdict, 1 = saved, 2 = immune, 0 = failed.
    signed char   m_nSaveResult;
    char  _pad11[0x14 - 0x11];
    int           m_bFeedbackEffectData;
    int           m_bHasApplicationData;
    // No trailing pad member: the tail padding is implicit.
};

// Deflect breakdown, shipped by SignalDamage only when the attack result is 8, 9 or 10. It is a
// per-attack audit trail of a blaster deflection, with the roll, the attacker's total to beat, then
// one (source id, amount) pair per contributor. The pair order is not uniform: the feat, class and
// attacker pairs are (id, amount), the power and stance pairs (amount, id).
class CSWSCombatSubAttackDeflect {
public:
    CSWSCombatSubAttackDeflect() { Clear(); }
    void Clear();

#if KOTOR1  // K1's record is nine bytes, with no power, stance, class or attacker pairs
    unsigned char m_nDeflectRoll;
    unsigned char m_nAttackTotal;
    unsigned char m_nDeflectFeat;
    unsigned char m_nDeflectFeatBonus;
    signed char   m_nEffectBonus;
    unsigned char m_nDeflectResult;
    unsigned char m_nBaseAttackBonus;
    unsigned char m_nArmourPenalty;
    unsigned char m_nDeflectTotal;
#else
    unsigned char m_nDeflectRoll;
    char  _pad01[0x04 - 0x01];
    int           m_nAttackTotal;
    unsigned char m_nDeflectFeat;
    unsigned char m_nDeflectFeatBonus;
    signed char   m_nEffectBonus;
    unsigned char m_nDeflectResult;
    unsigned char m_nBaseAttackBonus;
    unsigned char m_nArmourPenalty;
    int           m_nDeflectTotal;
    unsigned char m_nPowerBonus;
    unsigned char m_nPowerId;
    int           m_nStanceBonus;
    int           m_nStanceId;
    unsigned char m_nClassFeat;
    signed char   m_nClassFeatBonus;
    unsigned char m_nAttackerFeat;
    signed char   m_nAttackerFeatPenalty;
#endif
};

// One queued sub-attack, the record a ranged or melee attack is staged in before it resolves.
class CSWSCombatSubAttack {
public:
    // Value-initialises the Vector and default-constructs the five breakdown records; the head fields
    // are left for the Add*SubAttack builders to fill in.
    CSWSCombatSubAttack();
    // Empty, but must stay declared: the by-value GetTopSubAttack temporary calls it.
    ~CSWSCombatSubAttack();

    int   m_nAttackIndex;
    unsigned char  m_nAttackResult;
    char  _pad05[0x06 - 0x05];
    unsigned short m_nFeatId;
    unsigned char  m_nWeaponAttackType;
    char  _pad09[0x0c - 0x09];
    // Ready delay, compared unsigned against the elapsed time.
    unsigned int   m_nReadyDelay;
    // Set when the projectile has resolved; gates the damage fan-out.
    int   m_bAttackRolled;
    int   m_bProjectileHit;
    int   m_bRangedAttack;
    int m_nReaxnDelay;
    Vector m_vMissLocation;
    int   m_nDischargeHand;
    short m_anDamage[15];
    // The same five record types as CSWSCombatAttackData, in the same order.
    CSWSCombatSubAttackAttack  m_attack;
    CSWSCombatSubAttackThreat  m_threat;
    CSWSCombatSubAttackDefense m_defense;
    CSWSCombatSubAttackDamage  m_damage;
    CSWSCombatSubAttackEffect  m_effect;
};

// One attack's data within a combat round.
class CSWSCombatAttackData {
public:
    CSWSCombatAttackData();
    ~CSWSCombatAttackData();
    // Returns a copy of the queued sub-attack at the head of the queue.
    CSWSCombatSubAttack GetTopSubAttack();

    void ClearAttackData();
    // Copies the scalar head of another attack, not the strings, the lists or the breakdowns.
    void Copy(CSWSCombatAttackData* pSrc);
    int GetBaseDamage();
    void SetBaseDamage(int amount);
    int  GetDamage(unsigned short nFlags);
    void SetDamage(unsigned short nFlags, int nAmount);
    // Records nAmount of one damage-flags category into the breakdown.
    void AddDamage(unsigned short nFlags, int nAmount, int a);
    // Sub-attack queue operations.
    int  IsSubAttackReady(int nTime);
    void ResolveTopSubAttackToAttack();
    // Promotes a queued sub-attack into the attack proper: six head fields, the damage breakdown, then
    // the five shared records as whole-struct assignments.
    void ResolveSubAttackToAttack(const CSWSCombatSubAttack& subAttack);
    // Stages this attack's current state as a queued sub-attack; the ranged form carries the extra
    // projectile parameters.
    void ResolveAttackToMeleeSubAttack(int nAttackIndex, unsigned int nDelay);
    void ResolveAttackToRangedSubAttack(int nAttackIndex, unsigned int nDelay, int a, int b,
                                        int nProjectileType);
    // Unused, but exported, and it is what types the sub-attack head.
    void AddMeleeSubAttack(unsigned int nDelay);
    void AddRangedSubAttack(int nAttackIndex, unsigned char nAttackResult, unsigned int nDelay,
                            int a, Vector vMissLocation, unsigned int nReaxnDelay,
                            int nProjectileType);
    void RemoveTopSubAttack();
    void ClearSubAttacks();
    int  GetTotalDamage(int a);
    // GFF persistence pair, covering 26 labelled fields plus the "DamageList" array.
    int SaveData(CResGFF* pGFF, CResStruct* pStruct);
    int LoadData(CResGFF* pGFF, CResStruct* pStruct);

    // Defaults to -1, which reads as "no value yet".
    unsigned char m_nAttackGroup;
    char  _pad01[0x04 - 0x01];
    // Marks a ranged attack whose projectile has actually resolved.
    int   m_bAttackRolled;
    // The attack's scheduled time.
    unsigned short m_nAnimationLength;
    unsigned long long m_targetObjectId;
    unsigned short m_nReaxnDelay;
    unsigned short m_nReaxnAnimation;
    short m_nReaxnAnimLength;
    int   m_nMissedBy;
    // The per-damage-type breakdown. The last element is also the base, untyped, damage slot.
    short m_anDamage[15];
    // The attack-type byte GetDamageBonus feeds to GetCurrentAttackWeapon.
    unsigned char m_nWeaponAttackType;
    // Copied into the target's last-attack-mode byte by the on-melee-attacked handlers.
    unsigned char m_nAttackMode;
    unsigned char m_nConcealment;
    // Nonzero selects the ranged power-attack feat chains, zero the melee pairs.
    int   m_bRangedAttack;
    int   m_bSneakAttack;
    // Set when the attack's total damage meets or exceeds the target's remaining hit points.
    int   m_bKillingBlow;
    int   m_bCoupDeGrace;
    int   m_bCriticalThreat;
    unsigned char m_nReserved1;
    int   m_bAttackDeflected;
    unsigned char m_nAttackResult;
    int   m_nReserved2;
    // The feats.2da row this attack was made with.
    unsigned short m_nFeatId;
    unsigned short m_nSpecialAttackId;
    // Where a missed ranged attack's projectile ends up.
    Vector m_vMissLocation;
    unsigned long long m_oidAmmoItem;
    CExoString m_sAttackDebugText;
    CExoString m_sDamageDebugText;
    CExoArrayList<CGameEffect*> m_effects;
    CExoArrayList<CSWSSpellScriptData*> m_onHitSpellImpacts;
    CExoArrayList<CSWCCMessageData*> m_messages;
    CExoArrayList<CSWSCombatSubAttack> m_subAttacks;
    int m_bForcedAnimation;
    int m_nForcedAnimation;
    CSWSCombatSubAttackAttack  m_attack;
    CSWSCombatSubAttackThreat  m_threat;
    CSWSCombatSubAttackDefense m_defense;
    CSWSCombatSubAttackDamage  m_damage;
    CSWSCombatSubAttackEffect  m_effect;
    CSWSCombatSubAttackDeflect m_deflect;
};

// Layout guards. The records' tail padding is load-bearing: an explicit trailing pad member raises the
// record's data size, and the implicit copy assignment copies the data size, so
// ResolveSubAttackToAttack would copy more bytes than the target expects while every sizeof assertion
// below still held.
#define KOTOR_OFFSETOF(T, m) __builtin_offsetof(T, m)
#define KOTOR_DSIZE(T, last) (KOTOR_OFFSETOF(T, last) + sizeof((static_cast<T*>(0))->last))

#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CSWSCombatSubAttackAttack)  == 0x20, "K1 Attack record size");
K2_LAYOUT_ASSERT(sizeof(CSWSCombatSubAttackThreat)  == 0x14, "K1 Threat record size");
K2_LAYOUT_ASSERT(sizeof(CSWSCombatSubAttackDefense) == 0x09, "K1 Defense record size");
K2_LAYOUT_ASSERT(sizeof(CSWSCombatSubAttackDamage)  == 0x2a, "K1 Damage record size");
K2_LAYOUT_ASSERT(sizeof(CSWSCombatSubAttackEffect)  == 0x20, "K1 Effect record size");
K2_LAYOUT_ASSERT(sizeof(CSWSCombatSubAttackDeflect) == 0x09, "K1 Deflect record size");
K2_LAYOUT_ASSERT(KOTOR_DSIZE(CSWSCombatSubAttackAttack, m_bAutomaticMiss) == 0x20, "K1 Attack dsize");
K2_LAYOUT_ASSERT(KOTOR_DSIZE(CSWSCombatSubAttackThreat, m_nCriticalMultiplier) == 0x12, "K1 Threat dsize");
K2_LAYOUT_ASSERT(KOTOR_DSIZE(CSWSCombatSubAttackEffect, m_bHasApplicationData) == 0x1c, "K1 Effect dsize");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatSubAttackDamage, m_anReserved) == 0x0c, "K1 Damage array");
K2_LAYOUT_ASSERT(sizeof(CSWSCombatSubAttack) == 0xd8, "K1 CSWSCombatSubAttack size");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatSubAttack, m_attack)  == 0x50, "K1 sub m_attack");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatSubAttack, m_threat)  == 0x70, "K1 sub m_threat");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatSubAttack, m_defense) == 0x84, "K1 sub m_defense");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatSubAttack, m_damage)  == 0x8e, "K1 sub m_damage");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatSubAttack, m_effect)  == 0xb8, "K1 sub m_effect");
K2_LAYOUT_ASSERT(sizeof(CSWSCombatAttackData) == 0x188, "K1 CSWSCombatAttackData size");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatAttackData, m_subAttacks) == 0xd8, "K1 attack m_subAttacks");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatAttackData, m_attack)  == 0xf0, "K1 attack m_attack");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatAttackData, m_threat)  == 0x110, "K1 attack m_threat");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatAttackData, m_defense) == 0x124, "K1 attack m_defense");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatAttackData, m_damage)  == 0x12e, "K1 attack m_damage");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatAttackData, m_effect)  == 0x158, "K1 attack m_effect");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatAttackData, m_deflect) == 0x178, "K1 attack m_deflect");
#else
K2_LAYOUT_ASSERT(sizeof(CSWSCombatSubAttackAttack)  == 0x30, "Attack record size");
K2_LAYOUT_ASSERT(sizeof(CSWSCombatSubAttackThreat)  == 0x14, "Threat record size");
K2_LAYOUT_ASSERT(sizeof(CSWSCombatSubAttackDefense) == 0x09, "Defense record size");
K2_LAYOUT_ASSERT(sizeof(CSWSCombatSubAttackDamage)  == 0x30, "Damage record size");
K2_LAYOUT_ASSERT(sizeof(CSWSCombatSubAttackEffect)  == 0x20, "Effect record size");
K2_LAYOUT_ASSERT(sizeof(CSWSCombatSubAttackDeflect) == 0x24, "Deflect record size");

// The data sizes the five struct assignments actually copy.
K2_LAYOUT_ASSERT(KOTOR_DSIZE(CSWSCombatSubAttackAttack, m_nSaberFormBonus) == 0x2d, "Attack dsize");
K2_LAYOUT_ASSERT(KOTOR_DSIZE(CSWSCombatSubAttackThreat, m_nCriticalMultiplier) == 0x12, "Threat dsize");
K2_LAYOUT_ASSERT(KOTOR_DSIZE(CSWSCombatSubAttackDefense, m_nACDebilitatedPenalty) == 0x09, "Defense dsize");
K2_LAYOUT_ASSERT(KOTOR_DSIZE(CSWSCombatSubAttackDamage, m_anReserved) == 0x30, "Damage dsize");
K2_LAYOUT_ASSERT(KOTOR_DSIZE(CSWSCombatSubAttackEffect, m_bHasApplicationData) == 0x1c, "Effect dsize");

K2_LAYOUT_ASSERT(sizeof(CSWSCombatSubAttack) == 0xf0, "CSWSCombatSubAttack size");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatSubAttack, m_nReadyDelay) == 0x0c, "sub +0x0c");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatSubAttack, m_nReaxnDelay) == 0x1c, "sub +0x1c");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatSubAttack, m_vMissLocation) == 0x20, "sub +0x20");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatSubAttack, m_anDamage) == 0x30, "sub +0x30");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatSubAttack, m_attack)  == 0x50, "sub m_attack");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatSubAttack, m_threat)  == 0x80, "sub m_threat");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatSubAttack, m_defense) == 0x94, "sub m_defense");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatSubAttack, m_damage)  == 0xa0, "sub m_damage");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatSubAttack, m_effect)  == 0xd0, "sub m_effect");

K2_LAYOUT_ASSERT(sizeof(CSWSCombatAttackData) == 0x1b8, "CSWSCombatAttackData size");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatAttackData, m_anDamage) == 0x24, "attack m_anDamage");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatAttackData, m_vMissLocation) == 0x70, "attack m_vMissLocation");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatAttackData, m_subAttacks) == 0xd8, "attack m_subAttacks");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatAttackData, m_bForcedAnimation) == 0xe8, "attack +0xe8");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatAttackData, m_nForcedAnimation) == 0xec, "attack +0xec");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatAttackData, m_attack)  == 0xf0, "attack m_attack");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatAttackData, m_threat)  == 0x120, "attack m_threat");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatAttackData, m_defense) == 0x134, "attack m_defense");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatAttackData, m_damage)  == 0x140, "attack m_damage");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatAttackData, m_effect)  == 0x170, "attack m_effect");
K2_LAYOUT_ASSERT(KOTOR_OFFSETOF(CSWSCombatAttackData, m_deflect) == 0x190, "attack m_deflect");
#endif

#undef KOTOR_DSIZE
#undef KOTOR_OFFSETOF

#ifndef CSWSCOMBATROUND_H
#define CSWSCOMBATROUND_H

#include "CExoLinkedList.h"

class CSWSCreature;
class CSWSCombatAttackData;
class CSWSItem;
class CResGFF;
struct CResStruct;

// One pending round action. The saving code names the head of the record by its GFF keys; the tail is
// the AddSWSpellAction parameter pack, one member per parameter in order.
class CSWSCombatRoundAction {
public:
    // m_nActionType: which Add*Action builder queued the record.
    enum {
        TYPE_ATTACK         = 1,   // a plain swing (also parries, cleaves and attacks of opportunity)
        TYPE_SPELL          = 2,
        TYPE_REACTION       = 3,
        TYPE_COMBAT_STEP    = 4,
        TYPE_EQUIP          = 6,
        TYPE_UNEQUIP        = 7,
        TYPE_PARRY_INDEX    = 8,
        TYPE_CAST_SPELL     = 9,
        TYPE_ITEM_SPELL     = 10,
        TYPE_FEAT_ATTACK    = 11,
        TYPE_CUTSCENE_MOVE  = 12,
        TYPE_STANCE         = 13,
        TYPE_SWITCH_WEAPONS = 14,
    };

    ~CSWSCombatRoundAction();
    // Every Add*Action builder constructs one, so it is defined here.
    CSWSCombatRoundAction();

    // Sort key; the queue is kept in ascending timer order.
    int m_nActionTimer;
    unsigned short m_nAnimation;
    int  m_nAnimationTime;
    int  m_nNumAttacks;
    unsigned char m_nActionType;
    unsigned long long m_oidTarget;
    int  m_bRetargettable;
    unsigned int  m_nInventorySlot;
    unsigned long long m_oidTargetRepository;

    unsigned int m_nSpellId;
    int  m_nClassIndex;
    int  m_nDomainLevel;
    int  m_nMetaMagic;
    int  m_bSkipReadyCount;
    Vector m_vTargetPosition;
    unsigned long long m_oidSpellTarget;
    int  m_bNoTarget;
    int  m_bProjectile;
    unsigned char m_nPathType;
    unsigned char _pad61[0x64-0x61];
    int  m_nActionParam;
    int  m_nReserved1;
    int  m_nFeatId;
    unsigned char m_nReadyKind;
    unsigned long long m_oidItem;
    // Two separate 4-byte fields.
    int  m_nActivePropertyIndex;
    int  m_nSubPropertyIndex;
    int  m_nReserved2;
    int  m_bCutsceneAttack;
    int  m_nCutsceneAnimation;
    int  m_nCutsceneAttackResult;
    int  m_nCutsceneDamage;
    int  m_bIsPC;

    // Resets every scalar to its spawn value; the constructor is this plus the Vector's
    // member-initialiser.
    void ClearData();

    int  SaveData(CResGFF* pGFF, CResStruct* pStruct);
    int  LoadData(CResGFF* pGFF, CResStruct* pStruct);
};

// The per-creature combat-round state machine. Each creature owns one and keeps a back-pointer to
// it. Tracks the current action and attack, the attack-id counter, the off-hand, bonus and
// deflect-arrow attack counts and the spell-cast round.
class CSWSCombatRound {
public:
    // GetWeaponAttackType's result and an attack's m_nWeaponAttackType: the hand or creature
    // weapon slot the attack is made with. GetCurrentAttackWeapon takes NONE as "the current one".
    enum {
        WEAPON_ATTACK_TYPE_NONE = 0,
        WEAPON_ATTACK_TYPE_ONHAND = 1,
        WEAPON_ATTACK_TYPE_OFFHAND = 2,
        WEAPON_ATTACK_TYPE_CWEAPON1 = 3,
        WEAPON_ATTACK_TYPE_CWEAPON2 = 4,
        WEAPON_ATTACK_TYPE_CWEAPON3 = 5,
        WEAPON_ATTACK_TYPE_EXTRA = 6,
        WEAPON_ATTACK_TYPE_UNARMED = 7,
        WEAPON_ATTACK_TYPE_UNARMED_EXTRA = 8,
    };

    CSWSCombatRound(CSWSCreature* pOwner);
    ~CSWSCombatRound();
    // Zeroed by the constructor; nothing else touches it.
    int  m_nReserved1;
    // The round's attack slots. ClearAllAttacks walks only the first five.
    CSWSCombatAttackData m_attacks[7];
    // The feats.2da row ids of the special attacks still owed this round.
    CExoArrayList<unsigned short> m_specialAttacks;
    // The parallel list of per-attack ids. ClearAllSpecialAttacks leaves it alone.
    CExoArrayList<unsigned short> m_specialAttackIds;

    unsigned short m_newAttackId;
    char _pad_c32[0xc34 - 0xc32];
    int  m_bRoundStarted;
    int  m_bSpellCastRound;
    int  m_spellCastRound;
    int  m_bCutsceneAttack;
    int  m_nTimer;
    int  m_bTargetCannotReact;
    int  m_nRoundLength;
    int  m_nOverlapAmount;
    // The bleed tick budget.
    int  m_nBleedTimer;
    // Set while the round is paused by the object in the pause-source id.
    int  m_bRoundPaused;
    unsigned long long m_oidPauseSource;
    int  m_nPauseTimer;
    int  m_bInfinitePause;
    // How many spells and item properties have gone off this round.
    int  m_nCastsThisRound;
    unsigned char m_currentAttack;
    char _pad_c75[0xc78 - 0xc75];
    int  m_offHandAttack;
    // Rolling attack-group counter, stamped into each freshly staged attack.
    unsigned char m_nAttackGroupCounter;
    char _pad_c7d[0xc80 - 0xc7d];
    int  m_deflectArrow;
    int  m_bWeaponSucks;
    // The round's reaction counters. The parry index is the insertion cursor the parry builders walk.
    int  m_nParryIndex;
    int  m_nNumAOOs;
    int  m_nNumCleaves;
    // A pending target swap: when this is not OBJECT_INVALID the physical-attack action is re-pointed
    // at that object before the swing resolves.
    unsigned long long m_oidPendingTargetSwap;
    int  m_nOnHandAttacks;
    int  m_nOffHandAttacks;
    int  m_nOffHandTaken;
    int  m_nExtraTaken;
    int  m_nAdditAttacks;
    int  m_bonusEffectAttacks;
    // The parry allowance.
    unsigned char m_nParryActions;
    // The round's original target id, stamped from StartCombatRound's argument.
    unsigned long long m_oidDodgeTarget;
    CExoLinkedList<CSWSCombatRoundAction>* m_pRoundActionList;
    CSWSCreature* m_pCreature;
    int  m_bEngaged;
    int  m_bMaster;
    // The current target object id.
    unsigned long long m_oidMaster;
    unsigned long long m_oidSlave;
    // The queue entry currently being executed, owned.
    CSWSCombatRoundAction* m_pCurrentAction;
    // A verbatim copy of the executing entry's action type.
#if !KOTOR1  // K1 has no copy of the executing action's type
    unsigned char m_nExecutingActionType;
#endif
    unsigned long long m_oidAttackTarget;
    unsigned char m_currentAction;
    char _pad_d09[0xd0c - 0xd09];
    int  m_nReserved2;

    // Splices one pending action into the queue and takes ownership of it, including on the reject
    // path. An index of -1 inserts in sort order; anything else is a 0-based position.
    void AddSpellAction();
    void AddAction(CSWSCombatRoundAction* pAction, int nIndex);
    // The two round-level queue-a-cast entry points whose parameter packs land in the field block
    // above.
    void AddSWItemSpellAction(unsigned long long oidItem, int a2, int a3, Vector vTarget,
                              unsigned long long oidTarget, int a6, int a7);
    void AddSWSpellAction(unsigned int nSpellId, int nClassIndex, int nDomainLevel,
                          int nMetaMagic, int bSkipReadyCount, Vector vTarget,
                          unsigned long long oidTarget, int bNoTarget, int bProjectile,
                          unsigned char nPathType, int bAttackCast, int nFeatId,
                          unsigned char nReadyKind, int a14);

    void ClearAllAttacks();
    // Recomputes the on- and off-hand attack counts and clears the two per-round tallies.
    void InitializeNumberOfAttacks(unsigned short nFeat);
    // Both return the value they just stored; the off-hand one is a yes/no flag, not a count.
    int  CalculateOnHandAttacks(unsigned short nFeat);
    int  CalculateOffHandAttacks();
    // Tells the owning creature's client that a new combat round began.
    void SignalCombatRoundStarted();
    // True only once the round is past its on-hand attack allowance.
    int  GetExtraAttack();
    // Pops the head action off the queue and returns it if its timer has come due. The node is removed
    // either way, so a head that is not yet due is dropped.
    CSWSCombatRoundAction* GetAction();
    // The same due-time test without popping.
    int  GetActionPending();
    int  GetAttackActionPending();
    // Would an action of the given length still fit in the round? Always false while the round is
    // paused by some other object, always true outside an active round.
    int  CheckActionLength(unsigned long long oid, int nLength);
    void RemoveAllActions();
    int  RemoveLastAction();

    void SetDeflectArrow(int deflectArrow);
    void SetOffHandAttack(int offHandAttack);
    int  GetOffHandAttack();
    unsigned char GetCurrentAction();
    void SetCurrentAction(unsigned char action);
    void SetCurrentAttack(unsigned char attack);
    CSWSCombatAttackData* GetAttack(int nAttack);
    void StartCombatRound(unsigned long long oidTarget, int bAttack, int bDefend,
                          unsigned short nAttacks, int a5);
    // Opens a round for a queued spell, mirroring the participant ids into the target's round.
    void StartCombatRoundCast(int a1, int a2, unsigned long long oidTarget, int a4, int a5);
    // Closes the round: zeroes the timing and pause state, mirrors the teardown into the other
    // participant's round, re-acquires an attack target and runs the party and auto-pause epilogue.
    void EndCombatRound(int a);
    // True when this round is engaged with the named creature.
    int  ResolveEngagedState(unsigned long long objectId);
    // True when the named creature is the one this round is mastered to.
    int  ResolveMasterState(unsigned long long objectId, int& bCheckMaster);
    // True when both sides are unarmed with the same feat, or both wield a duel-capable weapon.
    int  ResolveChoreographedState(unsigned long long objectId);
    // The current attack's weapon-type code.
    int GetWeaponAttackType();
    CSWSItem* GetCurrentAttackWeapon(int nAttackType);
    void SetBonusEffectAttacks(int bonusEffectAttacks);
    int  GetActualBonusEffectAttacks();
    int  GetTotalAttacks();
    int  GetFunctionalBonusEffectAttacks();
    void SetSpellCastRound(int spell, int round);
    int  GetSpellActionPending();
    // Queues the round's own half of a weapon swap.
    void AddSwitchWeaponsAction();
    void RemoveSpellAction();
    void SetRoundPaused(int bPaused, unsigned long long objectId);
    // The timer comes first, the paused flag second.
    void SetPauseTimer(int nTime, int bPaused);
    void UpdateAttackTargetForAllActions(unsigned long long objectId);
    void ClearAllSpecialAttacks();
    // GetSpecialAttack yields the feat id of every remaining slot; GetSpecialAttackID yields only the
    // slots that have a feat, plus the paired per-attack id, and returns the maximum unsigned short
    // when it runs out.
    void InsertSpecialAttack(unsigned short nFeat, int nIndex);
    unsigned short GetSpecialAttack(int nIndex);
    unsigned short GetSpecialAttackID(int nIndex);
    unsigned short GetNewAttackID();
    // Reads the "CombatRoundData" GFF struct back in.
    int  LoadCombatRound(CResGFF* pGFF, CResStruct* pStruct);
    // The paired writer, called only while m_bRoundStarted == 1.
    int  SaveCombatRound(CResGFF* pGFF, CResStruct* pStruct);
    void AddCutsceneAttackAction(unsigned long long oid, int a2, int a3, int a4);
    void AddCutsceneMoveActions(unsigned long long oid, Vector vPos, int nRun);
    // Queues a feat attack action.
    void AddAttackAction(unsigned long long objectId, unsigned short nFeat,
                         int a, int b, int c);
    // Queues the in-combat stance change.
    void AddStanceAction(int nStance, unsigned long long oidTarget, int a);
    // Queues the bare parry marker, with no payload beyond the action type.
    void AddParryIndex();
    // The three reaction-time attack builders. Their animation length is the same formula in all three:
    // min(nAttacks*750, 2500) plus 500, divided by (nAttacks-3)*0.5 + 3 and truncated, for three parry,
    // cleave or opportunity attacks per round budget unit. The parry and cleave builders are unguarded
    // and splice their node at the head of the queue rather than going through AddAction's sorted
    // insert, so they never see its count cap.
    void AddParryAttack(unsigned long long oid);
    void AddCleaveAttack(unsigned long long oid);
    void AddAttackOfOpportunity(unsigned long long oid);
    // Queues a reaction nDelay ticks after the round's current time, lasting nAnimationTime. The only
    // builder in the family that picks the queue position explicitly.
    void AddReaction(int nDelay, int nAnimationTime, int nIndex);
    // Queues a 500 ms combat step toward oidTarget, then charges the round for it.
    void AddCombatStepAction(unsigned int nTimer, unsigned long long oidTarget);
    void AddUnequipAction(unsigned long long oidItem, unsigned long long a2, int nSlot);
    void AddEquipAction(unsigned long long oidItem, unsigned int nSlot, int a, int b);
    void IncrementTimer(int a);
    void DecrementRoundLength(int nTime, int a);
    // True when the creature is outside the band around its desired attack range to the target and so
    // has to take a combat step before it can attack.
    int  GetCombatStepRequired(unsigned long long objectId);
    // Debug-only queue dump.
    void DEBUGDumpCombatRoundToScreen(int nMode, const CExoString& sLabel);
    // CheckActionLength asked at an offset into the round: the remaining budget is reduced by nTime,
    // and the pause guard rejects only while the pause has more than nTime left.
    int  CheckActionLengthAtTime(unsigned long long oid, int nLength, int nTime);
    void DecrementPauseTimer(int nTime);
    // True only when both real weapon slots are empty and at least one creature-weapon slot is filled.
    int  HasCreatureWeapons();
};

#if KOTOR1
// K1's attack slots are smaller, so the record is shorter.
K2_LAYOUT_ASSERT(sizeof(CSWSCombatRound) == 0xbb8, "K1 CSWSCombatRound size");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCombatRound, m_specialAttacks) == 0xac0, "K1 round +0xac0");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCombatRound, m_currentAttack) == 0xb24, "K1 round +0xb24");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCombatRound, m_pCreature) == 0xb80, "K1 round +0xb80");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCombatRound, m_pCurrentAction) == 0xba0, "K1 round +0xba0");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCombatRound, m_oidAttackTarget) == 0xba8, "K1 round +0xba8");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSCombatRound, m_nReserved2) == 0xbb4, "K1 round +0xbb4");
#endif

#endif
