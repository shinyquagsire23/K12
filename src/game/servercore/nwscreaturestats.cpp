// CSWSCreatureStats - server-side creature statistics.

#include "k2stdint.h"
#include "Achievements.h"
#include "nwscreaturestats.h"
#include "nwscriptconst.h"
#include "Log_Base2.h"
#include "nwupdatestats.h"
#include "nwsrules.h"
#include "nwrace.h"
#include "nwclass.h"
#include "nwskill.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "appmanager.h"
#include "serverexoapp.h"
#include "serverinfo.h"
#include "globalvartables.h"
#include "partytables.h"
#include "nwscreature.h"
#include "nwscombatround.h"
#include "nwssendplayermessage.h"
#include "nwsitem.h"
#include "nwsplaceable.h"
#include "nwspell.h"
#include "nwitem.h"
#include "nwsitemproperty.h"
#include "nwsmodule.h"
#include "serveraimaster.h"
#include "nwsscriptevent.h"
#include "nwsarea.h"
#include "nwseffectlist.h"
#include "nwseffect.h"
#include "nwmessage.h"
#include "resgff.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "nwscript.h"
#include "nwrules.h"
#include "nwcreaturelevelstats.h"
#include "swparty.h"
#include "nwccreature.h"
#include "CExoArrayListImpl.h"

// K1 uses the C library rand; K2 routes through the port's shim.
#include "port/crtapi.h"
#include "port/libcstring.h"
#include "itemtype.h"
#include "nwmessagetypes.h"
#include "strref.h"

// CGameEffect::m_subType values, the engine's effect types. Same numbering as the
// CSWSEffectListHandler::EFFECT_* constants, which are out-of-line data.
enum {
    GAME_EFFECT_HASTE                     = 1,
    GAME_EFFECT_SETSTATE                  = 8,
    GAME_EFFECT_ATTACK_DECREASE           = 11,
    GAME_EFFECT_DAMAGE_DECREASE           = 14,
    GAME_EFFECT_DAMAGE_IMMUNITY_DECREASE  = 17,
    GAME_EFFECT_ENTANGLE                  = 18,
    GAME_EFFECT_DEATH                     = 19,
    GAME_EFFECT_SAVING_THROW_DECREASE     = 27,
    GAME_EFFECT_MOVEMENT_SPEED_DECREASE   = 29,
    GAME_EFFECT_FORCE_RESISTANCE_DECREASE = 34,
    GAME_EFFECT_POISON                    = 35,
    GAME_EFFECT_ABILITY_DECREASE          = 37,
    GAME_EFFECT_AC_DECREASE               = 49,
    GAME_EFFECT_SKILL_DECREASE            = 56,
    GAME_EFFECT_BLASTERDEFLECTIONDECREASE = 93,
    GAME_EFFECT_HORRIFIED                 = 94,
    GAME_EFFECT_DAMAGEFORCEPOINTS         = 95,
    GAME_EFFECT_CHOKE                     = 97,
    GAME_EFFECT_PSYCHICSTATIC             = 99,
    GAME_EFFECT_LIGHTSABERTHROW           = 100,
};

// The feat.2da grant level of a feat a companion never receives.
enum {
    GRANT_LEVEL_NEVER = 0xff,
};

// UpdateLastStatsObject's field bits: which parts of the stats snapshot to refresh.
enum {
    STATS_UPDATE_ABILITIES          = 1 << 0,
    STATS_UPDATE_FORT_SAVE          = 1 << 1,
    STATS_UPDATE_WILL_SAVE          = 1 << 2,
    STATS_UPDATE_REFLEX_SAVE        = 1 << 3,
    STATS_UPDATE_EXPERIENCE         = 1 << 4,
    STATS_UPDATE_BASE_ATTACK_BONUS  = 1 << 5,
    STATS_UPDATE_COMBAT_INFORMATION = 1 << 6,
    STATS_UPDATE_SKILLS             = 1 << 7,
    STATS_UPDATE_FEATS              = 1 << 9,
    STATS_UPDATE_ARMOR_CLASS        = 1 << 10,
};

// ranges.2da rows a creature's "PerceptionRange" may name.
enum {
    RANGES_ROW_PERCEPTION_DEFAULT = 11,
    RANGES_ROW_PERCEPTION_PLAYER  = 12,
};

// The four hex digits of a spells.2da / feat.2da talent Category. A talent filter's digit matches
// when it equals the talent's or is all ones (any).
enum {
    TALENT_CATEGORY_DIGIT_0 = 0x000f,
    TALENT_CATEGORY_DIGIT_1 = 0x00f0,
    TALENT_CATEGORY_DIGIT_2 = 0x0f00,
    TALENT_CATEGORY_DIGIT_3 = 0xf000,
};

// The m_nAIState bit OnApplySilence clears: a silenced creature has no force-power talents.
enum {
    AI_STATE_CAN_SPEAK = 1 << 3,
};

// The GFF struct id SaveClassInfo writes on each SpellsPerDayList element.
enum {
    GFF_STRUCT_SPELLS_PER_DAY = 0x4567,
};

extern int g_bEnableCombatDebugging;

CSWSStats_SpellLikeAbility::CSWSStats_SpellLikeAbility()
{
    m_nSpellId = 0;
    m_nReady = 1;
    m_nCasterLevel = 0;
}

CSWSStats_Spell::CSWSStats_Spell()
{
    m_nSpellId = 0;
    m_nReady = 1;
    m_nCasterLevel = 0;
    m_nExtra = 0;
}

#if KOTOR1
unsigned char CSWSCreatureStats::GetClass(unsigned char classIndex)
{
    return (classIndex < m_numClasses) ? m_classInfo[classIndex].m_class : CLASS_TYPE_INVALID;
}
#endif

void CSWSCreatureStats::SetClass(unsigned char classIndex, unsigned char classId) {
    if (classIndex < m_numClasses)
        m_classInfo[classIndex].m_class = classId;
}

void CSWSCreatureStats::SetClassNegativeLevels(unsigned char classIndex, unsigned char negativeLevels) {
    if (classIndex < m_numClasses)
        m_classInfo[classIndex].m_negativeLevels = negativeLevels;
}

void CSWSCreatureStats::SetHasLostClassAbilities(unsigned char classIndex, int hasLost) {
    if (classIndex < m_numClasses)
        m_classInfo[classIndex].m_hasLostClassAbilities = hasLost;
}

unsigned char CSWSCreatureStats::GetClassNegativeLevels(unsigned char classIndex) {
    if (classIndex < m_numClasses)
        return m_classInfo[classIndex].m_negativeLevels;
    return 0;
}

int CSWSCreatureStats::GetHasLostClassAbilities(unsigned char classIndex) {
    if (classIndex < m_numClasses)
        return m_classInfo[classIndex].m_hasLostClassAbilities;
    return 0;
}

#if KOTOR1
void CSWSCreatureStats::ModifyAlignment(short nDelta)
{
    int nNew = m_nAlignment + nDelta;
    if (nNew > 100)
        nNew = 100;
    // K1 clamps twice; the second clamp is redundant.
    int v = nNew;
    if (v < 0)
        v = 0;
    if (v > 100)
        v = 100;
    m_nAlignment = static_cast<short>(v);
    if (m_pOwner != 0)
        m_pOwner->UpdatePureGoodEvilPowers();
    UpdateCombatInformation();
}

int CSWSCreatureStats::GetSimpleAlignmentGoodEvil()
{
    if (m_nAlignment < 41)
        return 3;
    if (m_nAlignment > 59)
        return 2;
    return 1;
}
#else
// Shifts the alignment axis and fires the threshold/direction achievements. The PC, party-NPC and
// other-creature regimes clamp against different axes (m_nAlignment against m_nBaseNPCAlignment)
// and wrap 16-bit on the delta; the PC arm also mirrors the value into the "G_PC_Align_Val" global
// and pings every live party NPC.
void CSWSCreatureStats::ModifyAlignment(short nDelta, int a)
{
    CSWSCreature* pOwner = m_pOwner;
    if (pOwner->m_bPlayerMadeCharacter != 0)
    {
        int nNew = m_nAlignment + nDelta;
        if (nNew > 100)
        {
            m_nAlignment = 100;
            Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_FULL_LIGHT_SIDE);
        }
        else if (nNew < 0)
        {
            m_nAlignment = 0;
            Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_FULL_DARK_SIDE);
        }
        else
        {
            m_nAlignment += nDelta;
            int v = m_nAlignment;
            if (v < 0)
                v = 0;
            if (v > 100)
                v = 100;
            m_nAlignment = static_cast<short>(v);
            if (nNew == 100)
                Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_FULL_LIGHT_SIDE);
            else if (nNew == 0)
                Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_FULL_DARK_SIDE);
        }
        if (nDelta < 0)
            Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_FIRST_DARK_SIDE_POINTS);
        else if (nDelta > 0)
            Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_FIRST_LIGHT_SIDE_POINTS);
        if (a == 0)
        {
            g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(
                CExoString("G_PC_Align_Val"), static_cast<char>(m_nAlignment));
            for (int i = 0; i < CSWPartyTable::NUM_NPCS; i++)
            {
                unsigned long long npcId =
                    g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1);
                if (npcId == OBJECT_INVALID)
                    continue;
                CSWSCreature* pNPC =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(npcId);
                if (pNPC != 0)
                    pNPC->HandleAlignmentInfluence();
            }
        }
    }
    else if (pOwner->m_bInParty != 0)
    {
        int nNew = m_nBaseNPCAlignment + nDelta;
        if (nNew > 100)
            m_nBaseNPCAlignment = 100;
        else if (nNew < 0)
            m_nBaseNPCAlignment = 0;
        else
        {
            m_nBaseNPCAlignment = static_cast<short>(nNew);
            if (m_nBaseNPCAlignment < 0)
                m_nBaseNPCAlignment = -1;
            else if (m_nBaseNPCAlignment > 99)
                m_nBaseNPCAlignment = 100;
        }
        pOwner->HandleAlignmentInfluence();
    }
    else
    {
        int nNew = m_nAlignment + nDelta;
        if (nNew > 100)
            m_nAlignment = 100;
        else if (nNew < 0)
            m_nAlignment = 0;
        else
        {
            m_nAlignment = static_cast<short>(nNew);
            if (m_nAlignment < 1)
                m_nAlignment = 0;
            else if (m_nAlignment > 99)
                m_nAlignment = 100;
        }
    }
    if (m_pOwner != 0)
        m_pOwner->UpdatePureGoodEvilPowers();
    UpdateCombatInformation();
}
#endif

CExoString& CSWSCreatureStats::GetTag() {
    return m_pOwner->m_sTag;
}

unsigned short CSWSCreatureStats::GetFeat(unsigned short nFeat) {
    if (m_feats.m_count <= nFeat)
        return 0xffff;
    return m_feats.m_pArray[nFeat];
}

unsigned short CSWSCreatureStats::GetBonusFeat(unsigned short nFeat) {
    if (m_bonusFeats.m_count <= nFeat)
        return 0xffff;
    return m_bonusFeats.m_pArray[nFeat];
}

// Rolls the full per-attack damage. Side-effects the attack data and its combat log: weapon
// selection, the difficulty/autobalance scalers, the dice source fork (a ranged property, the monk
// die, or the base item's dice), the sneak and critical feats, the massive-critical property and
// the target's immunity/resistance/reduction chain.
int CSWSCreatureStats::GetDamageRoll(CSWSObject* pTarget, int bOffhand, int bCritical,
                                     int bSneakAttack, int bMaxDamage, int bAddDamage)
{
    int anDice[2] = {0, 0};

    int nCurrentAttack = m_pOwner->m_pCombatRound->m_currentAttack;
    CSWSCombatAttackData* pAttackData = m_pOwner->m_pCombatRound->GetAttack(nCurrentAttack);

    CSWSItem* pItem = 0;
    if (bOffhand != 0)
        pItem = static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
    if (pItem == 0)
        pItem = m_pOwner->m_pCombatRound->GetCurrentAttackWeapon(0);

    int nFlagA = 0, nFlagB = 0, nPercent = 100;
    if (pTarget != 0 && pTarget->AsSWSCreature() != 0)
    {
        if (m_bIsPC == 1 && pTarget->AsSWSCreature()->m_bPlayerCharacter == 0)
        {
            nPercent = g_pAppManager->m_pServerExoApp->GetDifficultyOption(3);
            nFlagA = 1;
        }
        else if (m_bIsPC == 0 && pTarget->AsSWSCreature()->m_bPlayerCharacter == 1)
        {
            nPercent = g_pAppManager->m_pServerExoApp->GetDifficultyOption(4);
            nFlagB = 1;
        }
    }

    int nDamage = 0;
    if (pItem != 0 && pItem->GetBaseItem()->m_nItemType != ITEM_TYPE_GAUNTLETS &&
        pItem->GetBaseItem()->m_nItemType != ITEM_TYPE_FOREARM_BANDS)
    {
        int nAttackType = m_pOwner->m_pCombatRound->GetWeaponAttackType();
        if (bMaxDamage != 0)
        {
            if (nAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON1 ||
                m_pOwner->m_pCombatRound->GetWeaponAttackType() == CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON2 ||
                m_pOwner->m_pCombatRound->GetWeaponAttackType() == CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON3)
            {
                if (pItem->GetPropertyByTypeExists(ITEM_PROPERTY_MONSTER_DAMAGE, 0))
                {
                    CSWItemProperty* pProp;
                    pItem->GetPropertyByType(&pProp, ITEM_PROPERTY_MONSTER_DAMAGE, 0);
                    if (pProp != 0 && pProp->m_costTableValue != 0)
                    {
                        g_pRules->m_p2DArrays->m_pIPRPMonsterDamage->GetINTEntry(
                            pProp->m_costTableValue, CExoString("NumDice"), &anDice[0]);
                        g_pRules->m_p2DArrays->m_pIPRPMonsterDamage->GetINTEntry(
                            pProp->m_costTableValue, CExoString("Die"), &anDice[1]);
                    }
                }
            }
            else if (m_pOwner->m_pCombatRound->GetWeaponAttackType() == CSWSCombatRound::WEAPON_ATTACK_TYPE_UNARMED ||
                     m_pOwner->m_pCombatRound->GetWeaponAttackType() == CSWSCombatRound::WEAPON_ATTACK_TYPE_UNARMED_EXTRA)
            {
                m_pOwner->GetUseMonkAbilities();
                anDice[0] = 1;
                int nCreatureSize = m_pOwner->m_nCreatureSize;
#if KOTOR1
                int nDie = (nCreatureSize < CREATURE_SIZE_MEDIUM) ? 2 : 1;
#else
                int nDie = 4;
#endif
                if (m_pOwner->GetUseMonkAbilities() == 1)
                    nDie = (nCreatureSize < CREATURE_SIZE_MEDIUM) ? 4 : 6;
                anDice[1] = nDie;
            }
            else
            {
                anDice[0] = g_pRules->GetBaseItem(pItem->m_baseItem)->m_nNumDice;
                anDice[1] = g_pRules->GetBaseItem(pItem->m_baseItem)->m_nDieToRoll;
            }
            nDamage = anDice[1] * anDice[0];
        }
        else
        {
            if (nAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON1 ||
                m_pOwner->m_pCombatRound->GetWeaponAttackType() == CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON2 ||
                m_pOwner->m_pCombatRound->GetWeaponAttackType() == CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON3)
            {
                if (pItem->GetPropertyByTypeExists(ITEM_PROPERTY_MONSTER_DAMAGE, 0))
                {
                    CSWItemProperty* pProp;
                    pItem->GetPropertyByType(&pProp, ITEM_PROPERTY_MONSTER_DAMAGE, 0);
                    if (pProp != 0 && pProp->m_costTableValue != 0)
                    {
                        g_pRules->m_p2DArrays->m_pIPRPMonsterDamage->GetINTEntry(
                            pProp->m_costTableValue, CExoString("NumDice"), &anDice[0]);
                        g_pRules->m_p2DArrays->m_pIPRPMonsterDamage->GetINTEntry(
                            pProp->m_costTableValue, CExoString("Die"), &anDice[1]);

                        short nDiceTotal = static_cast<short>(anDice[1] * anDice[0]);
#if !KOTOR1
                        float fMult = 0.0f;
                        int n310 = m_pOwner->m_nMultiplierSet;
                        if (n310 != 0)
                        {
                            g_pRules->m_p2DArrays->m_pAutoBalance->GetFLOATEntry(
                                n310, CExoString("DamageMult"), &fMult);
                            if (m_pOwner->m_bInParty == 0)
                            {
                                int nLevel = m_pOwner->m_nPCLevelAtSpawn;
                                if (nLevel == 0)
                                    nLevel = 1;
                                nDiceTotal =
                                    static_cast<short>((static_cast<int>(fMult * static_cast<float>(static_cast<char>(nLevel) - 1)) + 1) * nDiceTotal);
                            }
                        }
#endif
                        if (bCritical != 0)
                        {
#if KOTOR1
                            // K1: the multiplier is re-read at the top of every pass (no guard).
                            for (int i = 0; i < GetCriticalHitMultiplier(bOffhand); i++)
                            {
                                short nRoll = g_pRules->RollDice(static_cast<unsigned char>(anDice[0]), static_cast<unsigned char>(anDice[1]));
                                nDamage += g_pRules->GetWeightedDamageAmount(
                                    nRoll, nDiceTotal, nFlagA, nFlagB, nPercent);
                            }
#else
                            if (GetCriticalHitMultiplier(bOffhand) >= 1)
                            {
                                int i = 0;
                                do {
                                    short nRoll = g_pRules->RollDice(static_cast<unsigned char>(anDice[0]), static_cast<unsigned char>(anDice[1]));
#if !KOTOR1
                                    if (n310 != 0 && m_pOwner->m_bInParty == 0)
                                    {
                                        int nLevel = m_pOwner->m_nPCLevelAtSpawn;
                                        if (nLevel == 0)
                                            nLevel = 1;
                                        nRoll = static_cast<short>((static_cast<int>(fMult * static_cast<float>(static_cast<char>(nLevel) - 1)) + 1) * nRoll);
                                    }
#endif
                                    nDamage += g_pRules->GetWeightedDamageAmount(
                                        nRoll, nDiceTotal, nFlagA, nFlagB, nPercent);
                                    i++;
                                } while (i < GetCriticalHitMultiplier(bOffhand));
                            }
#endif
                        }
                        else
                        {
                            short nRoll = g_pRules->RollDice(static_cast<unsigned char>(anDice[0]), static_cast<unsigned char>(anDice[1]));
#if !KOTOR1
                            if (n310 != 0 && m_pOwner->m_bInParty == 0)
                            {
                                int nLevel = m_pOwner->m_nPCLevelAtSpawn;
                                if (nLevel == 0)
                                    nLevel = 1;
                                nRoll = static_cast<short>((static_cast<int>(fMult * static_cast<float>(static_cast<char>(nLevel) - 1)) + 1) * nRoll);
                            }
#endif
                            nDamage = g_pRules->GetWeightedDamageAmount(
                                nRoll, nDiceTotal, nFlagA, nFlagB, nPercent);
                        }
                    }
                }
            }
            else
            {
                anDice[0] = g_pRules->GetBaseItem(pItem->m_baseItem)->m_nNumDice;
                anDice[1] = g_pRules->GetBaseItem(pItem->m_baseItem)->m_nDieToRoll;

                short nDiceTotal = static_cast<short>(anDice[1] * anDice[0]);
#if !KOTOR1
                float fMult = 0.0f;
                int n310 = m_pOwner->m_nMultiplierSet;
                if (n310 != 0)
                {
                    g_pRules->m_p2DArrays->m_pAutoBalance->GetFLOATEntry(
                        n310, CExoString("DamageMult"), &fMult);
                    if (m_pOwner->m_bInParty == 0)
                    {
                        int nLevel = m_pOwner->m_nPCLevelAtSpawn;
                        if (nLevel == 0)
                            nLevel = 1;
                        nDiceTotal =
                            static_cast<short>((static_cast<int>(fMult * static_cast<float>(static_cast<char>(nLevel) - 1)) + 1) * nDiceTotal);
                    }
                }
#endif
                if (bCritical != 0)
                {
#if KOTOR1
                    for (int i = 0; i < GetCriticalHitMultiplier(bOffhand); i++)
                    {
                        short nRoll = g_pRules->RollDice(static_cast<unsigned char>(anDice[0]), static_cast<unsigned char>(anDice[1]));
                        nDamage += g_pRules->GetWeightedDamageAmount(
                            nRoll, nDiceTotal, nFlagA, nFlagB, nPercent);
                    }
#else
                    if (GetCriticalHitMultiplier(bOffhand) >= 1)
                    {
                        int i = 0;
                        do {
                            short nRoll = g_pRules->RollDice(static_cast<unsigned char>(anDice[0]), static_cast<unsigned char>(anDice[1]));
#if !KOTOR1
                            if (n310 != 0 && m_pOwner->m_bInParty == 0)
                            {
                                int nLevel = m_pOwner->m_nPCLevelAtSpawn;
                                if (nLevel == 0)
                                    nLevel = 1;
                                nRoll = static_cast<short>((static_cast<int>(fMult * static_cast<float>(static_cast<char>(nLevel) - 1)) + 1) * nRoll);
                            }
#endif
                            nDamage += g_pRules->GetWeightedDamageAmount(
                                nRoll, nDiceTotal, nFlagA, nFlagB, nPercent);
                            i++;
                        } while (i < GetCriticalHitMultiplier(bOffhand));
                    }
#endif
                }
                else
                {
                    short nRoll = g_pRules->RollDice(static_cast<unsigned char>(anDice[0]), static_cast<unsigned char>(anDice[1]));
#if !KOTOR1
                    if (n310 != 0 && m_pOwner->m_bInParty == 0)
                    {
                        int nLevel = m_pOwner->m_nPCLevelAtSpawn;
                        if (nLevel == 0)
                            nLevel = 1;
                        nRoll = static_cast<short>((static_cast<int>(fMult * static_cast<float>(static_cast<char>(nLevel) - 1)) + 1) * nRoll);
                    }
#endif
                    nDamage = g_pRules->GetWeightedDamageAmount(
                        nRoll, nDiceTotal, nFlagA, nFlagB, nPercent);
                }
            }
        }
        if (pItem->GetPropertyByTypeExists(ITEM_PROPERTY_NO_DAMAGE, 0))
            nDamage = 0;
    }
    else
    {
        if (bCritical == 0)
        {
            nDamage = GetUnarmedDamageRoll(pTarget);
        }
#if KOTOR1
        else
        {
            for (int i = 0; i < GetCriticalHitMultiplier(bOffhand); i++)
                nDamage += GetUnarmedDamageRoll(pTarget);
        }
#else
        else if (GetCriticalHitMultiplier(bOffhand) >= 1)
        {
            int i = 0;
            do {
                nDamage += GetUnarmedDamageRoll(pTarget);
                i++;
            } while (i < GetCriticalHitMultiplier(bOffhand));
        }
        else
        {
            nDamage = 0;
        }
#endif
    }

    if (bAddDamage != 0 && static_cast<short>(nDamage) >= 1)
    {
        pAttackData->m_damage.m_nDamageRoll = static_cast<unsigned char>(nDamage);
        pAttackData->AddDamage(static_cast<unsigned short>(m_pOwner->GetDamageFlags()), static_cast<short>(nDamage), 0);
    }

    if (g_bEnableCombatDebugging != 0)
    {
        CExoString strLog = m_pOwner->m_pCombatRound->GetAttack(nCurrentAttack)->m_sDamageDebugText;
        CExoString strTemp;
        if (bCritical != 0)
            strTemp.Format(const_cast<char*>("Damage Roll: %d (Critical x%d)"), static_cast<short>(nDamage),
                           GetCriticalHitMultiplier(bOffhand));
        else
            strTemp.Format(const_cast<char*>("Damage Roll: %d"), static_cast<short>(nDamage));
        m_pOwner->m_pCombatRound->GetAttack(nCurrentAttack)->m_sDamageDebugText = strLog + strTemp;
    }

    if (bSneakAttack != 0)
    {
        int nSneakDice;
        if (HasFeat(FEAT_SNEAK_ATTACK_10D6))
            nSneakDice = 10;
        else if (HasFeat(FEAT_SNEAK_ATTACK_9D6))
            nSneakDice = 9;
        else if (HasFeat(FEAT_SNEAK_ATTACK_8D6))
            nSneakDice = 8;
        else if (HasFeat(FEAT_SNEAK_ATTACK_7D6))
            nSneakDice = 7;
        else if (HasFeat(FEAT_SNEAK_ATTACK_6D6))
            nSneakDice = 6;
        else if (HasFeat(FEAT_SNEAK_ATTACK_5D6))
            nSneakDice = 5;
        else if (HasFeat(FEAT_SNEAK_ATTACK_4D6))
            nSneakDice = 4;
        else if (HasFeat(FEAT_SNEAK_ATTACK_3D6))
            nSneakDice = 3;
        else if (HasFeat(FEAT_SNEAK_ATTACK_2D6))
            nSneakDice = 2;
        else if (HasFeat(FEAT_SNEAK_ATTACK_1D6))
            nSneakDice = 1;
        else
            nSneakDice = 0;

        int nSneak = g_pRules->RollDice(static_cast<unsigned char>(nSneakDice), 6);
        if (g_bEnableCombatDebugging != 0)
        {
            CExoString strLog =
                m_pOwner->m_pCombatRound->GetAttack(nCurrentAttack)->m_sDamageDebugText;
            CExoString strTemp;
            strTemp.Format(const_cast<char*>(" + %d (Sneak Attack Damage)"), nSneak);
            m_pOwner->m_pCombatRound->GetAttack(nCurrentAttack)->m_sDamageDebugText =
                strLog + strTemp;
        }
        nDamage += nSneak;
        pAttackData->m_damage.m_nSneakAttackDamage = static_cast<unsigned char>(nSneak);
    }

    CSWSCreature* pTargetCreature = 0;
    if (pTarget != 0 && pTarget->AsSWSCreature() != 0)
        pTargetCreature = pTarget->AsSWSCreature();
    char nBonus = GetDamageBonus(pTargetCreature, bOffhand, 0);
    pAttackData->m_damage.m_nDamageBonus = nBonus;
    pAttackData->m_damage.m_nReserved1 = 0;

    if (bCritical != 0)
    {
        int nStrSum = 0, nSpecSum = 0, nBonusSum = 0;
#if KOTOR1
        for (int i = 0; i < GetCriticalHitMultiplier(bOffhand); i++)
        {
            nDamage += nBonus;
            nStrSum += pAttackData->m_damage.m_nStrDamageBonus;
            nSpecSum += pAttackData->m_damage.m_nWeaponSpecializationBonus;
            nBonusSum += pAttackData->m_damage.m_nDamageBonus;
        }
#else
        if (GetCriticalHitMultiplier(bOffhand) >= 1)
        {
            int i = 0;
            do {
                nDamage += nBonus;
                nStrSum += pAttackData->m_damage.m_nStrDamageBonus;
                nSpecSum += pAttackData->m_damage.m_nWeaponSpecializationBonus;
                nBonusSum += pAttackData->m_damage.m_nDamageBonus;
                i++;
            } while (i < GetCriticalHitMultiplier(bOffhand));
        }
#endif
        pAttackData->m_damage.m_nDamageBonus = static_cast<unsigned char>(nBonusSum);
        nDamage += static_cast<char>(nStrSum) + static_cast<unsigned char>(nSpecSum);
        pAttackData->m_damage.m_nStrDamageBonus = static_cast<signed char>(nStrSum);
        pAttackData->m_damage.m_nWeaponSpecializationBonus = static_cast<unsigned char>(nSpecSum);

#if KOTOR1
// K1: a single item property, looked up by type, and only when its cost value is non-zero.
        if (pItem != 0 && pItem->GetPropertyByTypeExists(ITEM_PROPERTY_MASSIVE_CRITICALS, 0))
        {
            CSWItemProperty* pProp;
            pItem->GetPropertyByType(&pProp, ITEM_PROPERTY_MASSIVE_CRITICALS, 0);
            if (pProp != 0 && pProp->m_costTableValue != 0)
            {
                int nPropBonus = pProp->m_costTableValue;
                int nNumDice;
                int nDie K2_UNSET;
                g_pRules->m_p2DArrays->m_pIPRPDamage->GetINTEntry(
                    nPropBonus, CExoString("NumDice"), &nNumDice);
                if (nNumDice != 0)
                {
                    g_pRules->m_p2DArrays->m_pIPRPDamage->GetINTEntry(
                        nPropBonus, CExoString("Die"), &nDie);
                    nPropBonus = g_pRules->RollDice(static_cast<unsigned char>(nNumDice), static_cast<unsigned char>(nDie));
                }
                nDamage += nPropBonus;
                pAttackData->m_damage.m_nDamageBonus += static_cast<unsigned char>(nPropBonus);
                if (g_bEnableCombatDebugging != 0)
                {
                    CExoString strLog =
                        m_pOwner->m_pCombatRound->GetAttack(nCurrentAttack)->m_sDamageDebugText;
                    CExoString strTemp;
                    if (nNumDice != 0)
                        strTemp.Format(const_cast<char*>(" + %d (%dd%d Massive Critical) "), nPropBonus, nNumDice,
                                       nDie);
                    else
                        strTemp.Format(const_cast<char*>(" + %d (Massive Critical) "), nPropBonus);
                    m_pOwner->m_pCombatRound->GetAttack(nCurrentAttack)->m_sDamageDebugText =
                        strLog + strTemp;
                }
            }
        }
#else
        if (pItem != 0 && pItem->GetPropertyByTypeExists(ITEM_PROPERTY_MASSIVE_CRITICALS, 0) &&
            pItem->m_nPassiveProperties >= 1)
        {
            int i = 0;
            do {
                CSWItemProperty* pProp = pItem->GetPassiveProperty(i);
                if (pProp != 0 && pProp->m_type == ITEM_PROPERTY_MASSIVE_CRITICALS)
                {
                    int nPropBonus = pProp->m_costTableValue;
                    int nNumDice;
                    g_pRules->m_p2DArrays->m_pIPRPDamage->GetINTEntry(
                        nPropBonus, CExoString("NumDice"), &nNumDice);
                    if (nNumDice != 0)
                    {
                        int nDie;
                        g_pRules->m_p2DArrays->m_pIPRPDamage->GetINTEntry(
                            nPropBonus, CExoString("Die"), &nDie);
                        nPropBonus = g_pRules->RollDice(static_cast<unsigned char>(nNumDice), static_cast<unsigned char>(nDie));
                    }
                    nDamage += nPropBonus;
                    pAttackData->m_damage.m_nDamageBonus += static_cast<unsigned char>(nPropBonus);
                }
                i++;
            } while (i < pItem->m_nPassiveProperties);
        }
#endif
    }
    else
    {
        nDamage += nBonus + static_cast<char>(pAttackData->m_damage.m_nStrDamageBonus) +
                   static_cast<unsigned char>(pAttackData->m_damage.m_nWeaponSpecializationBonus);
    }

    int anEffectBonus[2] = {0, 0};
#if KOTOR1
// K1: the 10-argument query; its result is recorded, not added to the roll.
    int nEffectBonus = m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_DAMAGE, pTarget,
                                                     anEffectBonus[1], 0, bMaxDamage,
                                                     0, 0, CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, 0, 0);
    pAttackData->m_damage.m_nEffectDamageBonus = static_cast<signed char>(nEffectBonus);
#else
    int nEffectBonus = m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_DAMAGE, pTarget,
                                                     anEffectBonus[1], 0, bMaxDamage,
                                                     0, 0, CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, 0, &anEffectBonus[0]);
    pAttackData->m_damage.m_nEffectDamageBonus = static_cast<signed char>(anEffectBonus[0]);
#endif
    if (nEffectBonus != 0 && g_bEnableCombatDebugging != 0)
    {
        CExoString strLog = m_pOwner->m_pCombatRound->GetAttack(nCurrentAttack)->m_sDamageDebugText;
        CExoString strTemp;
        if (bCritical == 1)
            strTemp.Format(const_cast<char*>(" + %d (Effect Damage Bonus) (Critical x%d)"), nEffectBonus,
                           GetCriticalHitMultiplier(bOffhand));
        else
            strTemp.Format(const_cast<char*>(" + %d (Effect Damage Bonus)"), nEffectBonus);
        m_pOwner->m_pCombatRound->GetAttack(nCurrentAttack)->m_sDamageDebugText = strLog + strTemp;
    }

#if KOTOR1
    short nTotal = static_cast<short>(nDamage);
#else
    short nTotal = static_cast<short>(nDamage + anEffectBonus[0]);
#endif
    if (nTotal < 1)
    {
        nTotal = 1;
        pAttackData->m_damage.m_nDamageRoll = 1;
        pAttackData->AddDamage(static_cast<unsigned short>(m_pOwner->GetDamageFlags()), 1, 0);
    }

    int nDamageFlags = m_pOwner->GetDamageFlags();
    if (pTarget != 0)
    {
        int nPower = m_pOwner->CalculateDamagePower(pTarget);
        nTotal = static_cast<short>(pTarget->DoDamageImmunity(m_pOwner, nTotal, static_cast<unsigned short>(nDamageFlags), bMaxDamage, 1));
        nTotal = static_cast<short>(pTarget->DoDamageResistance(m_pOwner, nTotal, static_cast<unsigned short>(nDamageFlags), bMaxDamage, 1, 1));
        nTotal = static_cast<short>(pTarget->DoDamageReduction(m_pOwner, nTotal, static_cast<unsigned char>(nPower), bMaxDamage, 1));
#if !KOTOR1
        if ((pTarget->AsSWSDoor() != 0 || pTarget->AsSWSPlaceable() != 0) &&
            pItem != 0 && pItem->m_bDoorCutting != 0)
            nTotal = pTarget->GetMaxHitPoints(1);
#endif
        nTotal = nTotal & ~(nTotal >> 15);
    }
    return nTotal;
}

char CSWSCreatureStats::GetForceResistance() {
    signed char resistance = static_cast<signed char>(m_forceResistance - m_forceResistancePenalty);
    return resistance < 0 ? 0 : resistance;
}

// Adds delta to the fractional HP accumulator and returns the whole hit points earned.
short CSWSCreatureStats::AdjustHPAccumulatorBy(float delta) {
    float acc = m_hpAccumulator + delta;
    int whole = static_cast<int>(__builtin_floorf(acc));
    m_hpAccumulator = acc - static_cast<float>(whole);
    return static_cast<short>(whole);
}

unsigned char CSWSCreatureStats_ClassInfo::GetSpellsPerDayLeft(unsigned char level) {
    if (level != 0)
        return 0;
    return m_spellsPerDayLeft;
}

void CSWSCreatureStats_ClassInfo::SetSpellsPerDayLeft(unsigned char level, unsigned char value) {
    if (level != 0)
        return;
    m_spellsPerDayLeft = value;
}

unsigned char CSWSCreatureStats_ClassInfo::GetMaxSpellsPerDayLeft(unsigned char level) {
    if (level != 0)
        return 0;
    return m_maxSpellsPerDayLeft;
}

void CSWSCreatureStats_ClassInfo::SetMaxSpellsPerDayLeft(unsigned char level, unsigned char value) {
    if (level != 0)
        return;
    m_maxSpellsPerDayLeft = value;
}

void CSWSCreatureStats_ClassInfo::DecrementSpellsPerDayLeft(unsigned char level) {
    if (level != 0)
        return;
    --m_spellsPerDayLeft;
}

void CSWSCreatureStats_ClassInfo::IncrementSpellsPerDayLeft(unsigned char level) {
    if (level != 0)
        return;
    ++m_spellsPerDayLeft;
}

int CSWSCreatureStats_ClassInfo::GetKnownSpell(unsigned char nLevel, unsigned char nIndex)
{
    if (nLevel != 0)
        return -1;
    if (m_knownSpells[0].m_count <= static_cast<int>(nIndex))
        return -1;
    return static_cast<int>(m_knownSpells[0].m_pArray[nIndex]);
}

void CSWSCreatureStats_ClassInfo::ResetSpellsPerDayLeft(unsigned char nLevel, unsigned char nValue)
{
    K2_UNUSED(nLevel); K2_UNUSED(nValue);
}

unsigned int CSWSCreatureStats_ClassInfo::GetNumberKnownSpells(unsigned char nLevel)
{
    return m_knownSpells[nLevel].m_count;
}

int CSWSCreatureStats_ClassInfo::HasSpell(unsigned char nLevel, unsigned int nSpellId)
{
    unsigned short nCount = static_cast<unsigned short>(m_knownSpells[nLevel].m_count);
    if (nCount == 0)
        return 0;
    for (int64_t i = 0; i < nCount; i++)
        if (m_knownSpells[nLevel].m_pArray[i] == nSpellId)
            return 1;
    return 0;
}

// Field assignment order is deliberate; do not reorder.
CSWSCreatureStats_ClassInfo::CSWSCreatureStats_ClassInfo()
{
    m_hasLostClassAbilities = 0;
    m_nReserved3 = 0;
    m_class = 0;
    m_level = 0;
    m_negativeLevels = 0;
    m_nReserved2 = 0;
    m_nReserved4 = 1;
    m_maxSpellsPerDayLeft = 0;
    m_nReserved1 = 0;
    m_spellsPerDayLeft = 0;
}

CSWSCreatureStats_ClassInfo::~CSWSCreatureStats_ClassInfo()
{
    m_knownSpells[0].SetSize(0);
}

void CSWSCreatureStats_ClassInfo::AddKnownSpell(unsigned char level, unsigned int nSpellId) {
    if (level != 0)
        return;

    if (m_knownSpells[level].IndexOf(nSpellId) != -1)
        return;

    m_knownSpells[level].Add(nSpellId);
}

void CSWSCreatureStats_ClassInfo::RemoveKnownSpell(unsigned char level, unsigned int nSpellId) {
    if (level != 0)
        return;

    CExoArrayList<unsigned int>* pList = &m_knownSpells[level];
    if (pList->IndexOf(nSpellId) < 0)
        return;

    pList->Remove(nSpellId);
}

void CSWSCreatureStats::RemoveKnownSpell(unsigned char classIndex, unsigned int nSpellId)
{
    if (g_pRules->GetSpell(nSpellId) != 0)
        m_classInfo[classIndex].RemoveKnownSpell(0, nSpellId);
}

// Usable when the creature owns the talent and, unless the area is unavailable, each nibble of
// nMask is a wildcard or matches the spell's.
int CSWSCreatureStats::CheckPowerSuitability(int nTalentId, CSWSpell* pSpell, int nMask,
                                             int a4, int a5, int a6, int a7, int a8)
{
    K2_UNUSED(a4); K2_UNUSED(a5); K2_UNUSED(a6); K2_UNUSED(a7);
    if (nTalentId == a8)
        return 0;
    if (GetCreatureHasTalent(0, nTalentId) == 0)
        return 0;
    if (m_pOwner->GetArea() != 0)
    {
        // Evaluate all four nibble tests up front, then test them one at a time.
        int nFlags = pSpell->m_nCategory;
        int bFail0 = ((nMask & TALENT_CATEGORY_DIGIT_0) != TALENT_CATEGORY_DIGIT_0) & ((nMask & TALENT_CATEGORY_DIGIT_0) != (nFlags & TALENT_CATEGORY_DIGIT_0));
        int bFail1 = ((nMask & TALENT_CATEGORY_DIGIT_1) != TALENT_CATEGORY_DIGIT_1) & ((nMask & TALENT_CATEGORY_DIGIT_1) != (nFlags & TALENT_CATEGORY_DIGIT_1));
        int bFail2 = ((nMask & TALENT_CATEGORY_DIGIT_2) != TALENT_CATEGORY_DIGIT_2) & ((nMask & TALENT_CATEGORY_DIGIT_2) != (nFlags & TALENT_CATEGORY_DIGIT_2));
        int bFail3 = ((nMask & TALENT_CATEGORY_DIGIT_3) != TALENT_CATEGORY_DIGIT_3) & ((nMask & TALENT_CATEGORY_DIGIT_3) != (nFlags & TALENT_CATEGORY_DIGIT_3));
        if (bFail0)
            return 0;
        if (bFail1)
            return 0;
        if (bFail2)
            return 0;
        if (bFail3)
            return 0;
    }
    return 1;
}

// True when any class slot is one of the nine Force-using classes.
int CSWSCreatureStats::GetIsJedi()
{
    for (int i = 0; i < m_numClasses; i++) {
        unsigned char nClass = GetClass(static_cast<unsigned char>(i));
#if KOTOR1
        // K1: only the three base Jedi classes.
        if (nClass == CSWClass::CCLASS_JEDIGUARDIAN ||
            nClass == CSWClass::CCLASS_JEDICONSULAR ||
            nClass == CSWClass::CCLASS_JEDISENTINEL)
            return 1;
#else
        if (nClass == CSWClass::CCLASS_JEDIGUARDIAN ||
            nClass == CSWClass::CCLASS_JEDICONSULAR ||
            nClass == CSWClass::CCLASS_JEDISENTINEL ||
            nClass == CSWClass::CCLASS_JEDIWATCHMAN ||
            nClass == CSWClass::CCLASS_JEDIMASTER ||
            nClass == CSWClass::CCLASS_JEDIWEAPONMASTER ||
            nClass == CSWClass::CCLASS_SITHLORD ||
            nClass == CSWClass::CCLASS_SITHASSASSIN ||
            nClass == CSWClass::CCLASS_SITHMARAUDER)
            return 1;
#endif
    }
    return 0;
}

// Returns the slot's effective level, its raw level less its negative levels, floored at 0.
unsigned char CSWSCreatureStats::GetClassLevel(unsigned char classIndex, int bApplyNegativeLevels)
{
    if (classIndex >= m_numClasses)
        return 0;

    int nLevel = m_classInfo[classIndex].m_level;
    int nNegative = bApplyNegativeLevels ? m_classInfo[classIndex].m_negativeLevels : 0;
    int nResult = nLevel - nNegative;
    return static_cast<unsigned char>((nResult < 0) ? 0 : nResult);
}

// Bonus force-power slots from the CHA modifier, on top of the class table's gain.
unsigned char CSWSCreatureStats::GetSpellGainWithBonus(unsigned char classIndex, unsigned char spellLevel)
{
    unsigned char nClassId = GetClass(classIndex);
    CSWClass* pClass = &g_pRules->m_paClasses[nClassId];
    if (pClass == 0)
        return 0;

    int nBonus = 0;
    if (CSWClass::IsJedi(nClassId))
    {
        char nChaMod = m_nCHAMod;
        if (nChaMod >= spellLevel && spellLevel != 0 && nChaMod >= 1 &&
            nChaMod - spellLevel + 4 >= 0)
            nBonus = (nChaMod - spellLevel + 4) / 4;
    }

    unsigned char nGain = static_cast<unsigned char>(pClass->GetSpellGain(GetClassLevel(classIndex, 0), spellLevel));
    return static_cast<unsigned char>((nGain == CSWClass::LEVEL_NONE) ? 0 : nGain + nBonus);
}

// Recomputes each Jedi class slot's max spells-per-day after an ability-score change; a lower max
// drops the remaining-uses counter by the same amount, floored at 0.
void CSWSCreatureStats::AdjustSpellUsesPerDay()
{
    for (unsigned char i = 0; i < m_numClasses; i++)
    {
        if (CSWClass::IsJedi(m_classInfo[i].m_class) == 0)
            continue;

        unsigned char nOld = m_classInfo[i].m_maxSpellsPerDayLeft;
        unsigned char nNew = GetSpellGainWithBonus(i, 0);
        char nDelta = static_cast<char>(nNew - nOld);
        m_classInfo[i].m_maxSpellsPerDayLeft = nNew;
        if (nDelta < 0)
        {
            if (nDelta < static_cast<int>(m_classInfo[i].m_spellsPerDayLeft))
                m_classInfo[i].m_spellsPerDayLeft += nDelta;
            else
                m_classInfo[i].m_spellsPerDayLeft = 0;
        }
    }
}

// Both calls' results are discarded.
void CSWSCreatureStats::ResetSpellsPerDayLeft(unsigned char classIndex, unsigned char nSpellLevel)
{
    if (classIndex < m_numClasses)
    {
        CSWRules* pRules = g_pRules;
        unsigned char nClassId = m_classInfo[classIndex].m_class;
        CSWClass* pClass = &pRules->m_paClasses[nClassId];
        if (pClass != 0)
        {
            CSWClass::IsJedi(nClassId);
            pClass->GetSpellGain(GetClassLevel(classIndex, 0), nSpellLevel);
        }
    }
}

#if KOTOR1
unsigned int CSWSCreatureStats::ComputeNumberKnownSpellsLeft(unsigned char classIndex)
#else
unsigned int CSWSCreatureStats::ComputeNumberKnownSpellsLeft(unsigned char classIndex, int bLevelUp)
#endif
{
#if KOTOR1
    // K1: a Jedi class learns 2 powers at class level 1, else 1.
    unsigned char nClassId = GetClass(classIndex);
    CSWClass* pClass = &g_pRules->m_paClasses[nClassId];
    if (pClass && CSWClass::IsJedi(nClassId))
        return (GetClassLevel(classIndex, 0) == 1) ? 2 : 1;
    return 0;
#else
    unsigned char nClassId = GetClass(classIndex);
    CSWClass* pClass = &g_pRules->m_paClasses[nClassId];
    if (pClass &&
        (nClassId == CSWClass::CCLASS_JEDICONSULAR ||
         nClassId == CSWClass::CCLASS_JEDIGUARDIAN ||
         nClassId == CSWClass::CCLASS_JEDISENTINEL ||
         nClassId == CSWClass::CCLASS_JEDIWATCHMAN ||
         nClassId == CSWClass::CCLASS_JEDIWEAPONMASTER ||
         nClassId == CSWClass::CCLASS_JEDIMASTER ||
         nClassId == CSWClass::CCLASS_SITHASSASSIN ||
         nClassId == CSWClass::CCLASS_SITHMARAUDER ||
         nClassId == CSWClass::CCLASS_SITHLORD)) {
        unsigned char nLevel = GetClassLevel(classIndex, 0);
        unsigned int nGain = pClass->GetSpellGain(nLevel + (bLevelUp != 0), 0);
        return (static_cast<unsigned char>(nGain) == CSWClass::LEVEL_NONE) ? 0 : nGain;
    }
    return 0;
#endif
}

// The four stubs below are empty or return a constant.

void CSWSCreatureStats::SetArcaneSpellFailure(unsigned char nFailure)
{
    K2_UNUSED(nFailure);
}

unsigned char CSWSCreatureStats::GetAttacksPerRound()
{
    return 1;
}

// Sums the class attack bonuses, applies the autobalance scaler, then prefers m_nOverrideBaseAttackBonus.
int CSWSCreatureStats::GetBaseAttackBonus()
{
    int nAttackBonus = 0;
    for (int i = 0; i < m_numClasses; i++)
    {
        unsigned char nLevel = GetClassLevel(static_cast<unsigned char>(i), 0);
        unsigned char nClassId = GetClass(static_cast<unsigned char>(i));
        CSWRules* pRules = g_pRules;
        CSWClass* pClass = &pRules->m_paClasses[nClassId];
        if (pClass != 0)
            nAttackBonus += pClass->GetAttackBonus(nLevel);
    }

#if !KOTOR1
    if (m_pOwner->m_bInParty == 0 && m_pOwner->m_nMultiplierSet != 0)
    {
        float fToHitMult;
        g_pRules->m_p2DArrays->m_pAutoBalance->GetFLOATEntry(
            m_pOwner->m_nMultiplierSet, CExoString("ToHitMult"), &fToHitMult);

        char nAutoBalanceLevel = m_pOwner->m_nPCLevelAtSpawn;
        nAttackBonus += static_cast<int>(fToHitMult * ((nAutoBalanceLevel != 0) ? static_cast<float>(nAutoBalanceLevel) : 1.0f));
    }
#endif

    return (m_nOverrideBaseAttackBonus != 0) ? m_nOverrideBaseAttackBonus : nAttackBonus;
}

int CSWSCreatureStats::GetSpellMinAbilityMet(unsigned char a, unsigned char b)
{
    K2_UNUSED(a); K2_UNUSED(b);
    return 0;
}

int CSWSCreatureStats::GetFavoredEnemyBonus(CSWSCreature* pTarget)
{
    K2_UNUSED(pTarget);
    return 0;
}

// The current attack feat's damage bonus (7/3/12/2/4 by feat id).
int CSWSCreatureStats::ResolveSpecialAttackDamageBonus(CSWSCreature* pTarget)
{
    K2_UNUSED(pTarget);
    CSWSCombatRound* pRound = m_pOwner->m_pCombatRound;
    CSWSCombatAttackData* pAttack = pRound->GetAttack(pRound->m_currentAttack);

    int nRes = 0;
#if KOTOR1
    // K1: three pairs of feat ids score a bonus, worth 8, 5 and 10.
    switch (pAttack->m_nFeatId)
    {
    case FEAT_IMPROVED_POWER_ATTACK:
        nRes = 8;
        break;
    case FEAT_IMPROVED_POWER_BLAST:
        nRes = 8;
        break;
    case FEAT_POWER_ATTACK:
        nRes = 5;
        break;
    case FEAT_POWER_BLAST:
        nRes = 5;
        break;
    case FEAT_MASTER_POWER_BLAST:
        nRes = 10;
        break;
    case FEAT_MASTER_POWER_ATTACK:
        nRes = 10;
        break;
    case FEAT_FORCE_JUMP_ADVANCED:
        nRes = 2;
        break;
    case FEAT_FORCE_JUMP_MASTERY:
        nRes = 4;
        break;
    }
#else
    switch (pAttack->m_nFeatId)
    {
    case FEAT_IMPROVED_POWER_ATTACK:
        nRes = 7;
        break;
    case FEAT_IMPROVED_POWER_BLAST:
        nRes = 7;
        break;
    case FEAT_POWER_ATTACK:
        nRes = 3;
        break;
    case FEAT_POWER_BLAST:
        nRes = 3;
        break;
    case FEAT_MASTER_POWER_BLAST:
        nRes = 12;
        break;
    case FEAT_MASTER_POWER_ATTACK:
        nRes = 12;
        break;
    case FEAT_FORCE_JUMP_ADVANCED:
        nRes = 2;
        break;
    case FEAT_FORCE_JUMP_MASTERY:
        nRes = 4;
        break;
    }
#endif
    return nRes;
}

// The current attack feat's attack bonus, honouring the weapon's sniper and rapid-shot flags.
int CSWSCreatureStats::ResolveSpecialAttackAttackBonus(CSWSCreature* pTarget)
{
    K2_UNUSED(pTarget);
#if KOTOR1
    // K1: flat penalties only, no sniper or rapid-shot weapon terms, and one feat id maps to -1.
    int nBonus = 0;
    switch (m_pOwner->m_pCombatRound->GetAttack(m_pOwner->m_pCombatRound->m_currentAttack)->m_nFeatId)
    {
    case FEAT_IMPROVED_POWER_ATTACK:
    case FEAT_IMPROVED_POWER_BLAST:
    case FEAT_POWER_ATTACK:
    case FEAT_POWER_BLAST:
    case FEAT_MASTER_POWER_BLAST:
    case FEAT_MASTER_POWER_ATTACK:
        nBonus = -3;
        break;
    case FEAT_FLURRY:
    case FEAT_RAPID_SHOT:
        nBonus = -4;
        break;
    case FEAT_MULTI_SHOT:
    case FEAT_WHIRLWIND_ATTACK:
        nBonus = -1;
        break;
    case FEAT_IMPROVED_FLURRY:
    case FEAT_IMPROVED_RAPID_SHOT:
        nBonus = -2;
        break;
    case FEAT_FORCE_JUMP_ADVANCED:
        nBonus = 2;
        break;
    case FEAT_FORCE_JUMP_MASTERY:
        nBonus = 4;
        break;
    }
    return nBonus;
#else
    CSWSCombatAttackData* pAttack =
        m_pOwner->m_pCombatRound->GetAttack(m_pOwner->m_pCombatRound->m_currentAttack);
    CSWSItem* pWeapon;
    int nBonus = 0;
    switch (pAttack->m_nFeatId)
    {
    case FEAT_FLURRY:
        nBonus = -4;
        break;
    case FEAT_IMPROVED_POWER_ATTACK:
    case FEAT_IMPROVED_POWER_BLAST:
    case FEAT_POWER_ATTACK:
    case FEAT_POWER_BLAST:
    case FEAT_MASTER_POWER_BLAST:
    case FEAT_MASTER_POWER_ATTACK:
        nBonus = -3;
        break;
    case FEAT_IMPROVED_SNIPER_SHOT:
    case FEAT_SNIPER_SHOT:
    case FEAT_MASTER_SNIPER_SHOT:
        pWeapon = m_pOwner->m_pCombatRound->GetCurrentAttackWeapon(0);
        if (pWeapon != 0)
            nBonus = (pWeapon->m_bSniper != 0) << 2;
        break;
    case FEAT_MULTI_SHOT:
        nBonus = -1;
        pWeapon = m_pOwner->m_pCombatRound->GetCurrentAttackWeapon(0);
        if (pWeapon != 0)
            nBonus = (pWeapon->m_bRapidShot != 0) - 1;
        break;
    case FEAT_RAPID_SHOT:
        nBonus = -4;
        pWeapon = m_pOwner->m_pCombatRound->GetCurrentAttackWeapon(0);
        if (pWeapon != 0)
            nBonus += (pWeapon->m_bRapidShot != 0);
        break;
    case FEAT_IMPROVED_FLURRY:
        nBonus = -2;
        break;
    case FEAT_IMPROVED_RAPID_SHOT:
        nBonus = -2;
        pWeapon = m_pOwner->m_pCombatRound->GetCurrentAttackWeapon(0);
        if (pWeapon != 0)
            nBonus += (pWeapon->m_bRapidShot != 0);
        break;
    case FEAT_FORCE_JUMP_ADVANCED:
        nBonus = 2;
        break;
    case FEAT_FORCE_JUMP_MASTERY:
        nBonus = 4;
        break;
    }
    return nBonus;
#endif
}

void CSWSCreatureStats::SetForceResistance(char nResistance)
{
    m_forceResistance = nResistance;
    UpdateCombatInformation();
}

void CSWSCreatureStats::SetForceResistancePenalty(char nPenalty)
{
    m_forceResistancePenalty = nPenalty;
    UpdateCombatInformation();
}

// Adds delta to the fractional force-point accumulator and returns the whole points earned.
short CSWSCreatureStats::AdjustFPAccumulatorBy(float fDelta)
{
    float acc = m_fpAccumulator + fDelta;
#if KOTOR1
    // K1: always floors (a negative remainder rounds down, as the HP twin does).
    int whole = static_cast<int>(__builtin_floorf(acc));
#else
    float fRounded;
    if (acc >= 0.0f)
        fRounded = __builtin_floorf(acc);
    else
        fRounded = __builtin_ceilf(acc);
    int whole = static_cast<int>(fRounded);
#endif
    m_fpAccumulator = acc - static_cast<float>(whole);
    return static_cast<short>(whole);
}

// GetUseMonkAbilities always returns 0, so the branches that test it are dead.
unsigned char CSWSCreatureStats::GetACNaturalBase()
{
    m_pOwner->GetUseMonkAbilities();
    return m_nACNaturalBase;
}

// The DEX modifier, capped by the worn armour's max-Dex bonus when a == 1 or when debilitated.
#if KOTOR1
char CSWSCreatureStats::GetDEXMod(int a)
#else
int CSWSCreatureStats::GetDEXMod(int a)
#endif
{
    int nMaxDexBonus = 0;
    int bHasCap = 0;
    if (a == 1 && !m_pOwner->IsDebilitated(1))
    {
        CSWSItem* pItem = static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_BODY));
        if (pItem != 0 && pItem->ComputeArmorClass() > 0)
        {
#if KOTOR1
            nMaxDexBonus = pItem->GetBaseItem()->m_nMaxDexBonus;
            bHasCap = 1;
#else
            nMaxDexBonus = pItem->GetBaseItem()->m_nMaxDexBonus + pItem->m_nMaxDexBonus;
            bHasCap = (nMaxDexBonus >= 0);
#endif
        }
    }
    if (bHasCap || m_pOwner->IsDebilitated(1))
#if KOTOR1
        return static_cast<char>((m_nDEXMod < nMaxDexBonus) ? m_nDEXMod : nMaxDexBonus);
#else
        return (m_nDEXMod < nMaxDexBonus) ? m_nDEXMod : nMaxDexBonus;
#endif
    return m_nDEXMod;
}

// The class's primary ability modifier.
unsigned char CSWSCreatureStats::GetPrimaryMod(unsigned char classIndex)
{
    unsigned char nClassId = GetClass(classIndex);
    if (&g_pRules->m_paClasses[nClassId] == 0)
        return m_nSTRMod;
    switch (g_pRules->m_paClasses[nClassId].m_nPrimaryAbility)
    {
    case ABILITY_STRENGTH:
        return m_nSTRMod;
    case ABILITY_DEXTERITY:
#if KOTOR1
        return GetDEXMod(0);
#else
    {
        int bDebilitated = m_pOwner->IsDebilitated(1);
        return (m_nDEXMod >= 0 && bDebilitated) ? 0 : m_nDEXMod;
    }
#endif
    case ABILITY_CONSTITUTION:
        return m_nCONMod;
    case ABILITY_INTELLIGENCE:
        return m_nINTMod;
    case ABILITY_WISDOM:
        return m_nWISMod;
    case ABILITY_CHARISMA:
        return m_nCHAMod;
    }
    return 0;
}

int CSWSCreatureStats::GetUnarmedDamageDice()
{
    m_pOwner->GetUseMonkAbilities();
    return 1;
}

// Ranged-weapon damage bonus.
int CSWSCreatureStats::GetRangedDamageBonus()
{
    CSWSItem* pItem =
            static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    int nBonus = 0;
    int nSpec = 0;
    if (pItem != 0)
    {
        if (g_pRules->GetBaseItem(pItem->m_baseItem)->m_bRangedWeapon == 0)
            return 0;

        unsigned char nWieldType =
            g_pRules->GetBaseItem(pItem->m_baseItem)->m_weaponWieldType;
        signed char nSTRMod = static_cast<signed char>(m_nSTRMod);
        nBonus = nSTRMod;
        if (nWieldType != 7)
        {
            CSWItemProperty* pProperty;
            if (pItem->GetPropertyByType(&pProperty, ITEM_PROPERTY_MIGHTY, 0) != 0)
            {
                int nMightyMod = static_cast<signed char>(m_nSTRMod);
#if KOTOR1
// K1: the capped mod is narrowed back to the signed byte.
                nBonus = static_cast<signed char>((nMightyMod < static_cast<int>(pProperty->m_costTableValue))
                                           ? nMightyMod
                                           : static_cast<int>(pProperty->m_costTableValue));
#else
                nBonus = (nMightyMod < static_cast<int>(pProperty->m_costTableValue))
                             ? nMightyMod
                             : static_cast<int>(pProperty->m_costTableValue);
#endif
            }
            else
            {
                if (nSTRMod > 0)
                    nBonus = 0;
            }
        }
        nSpec = GetWeaponSpecialization(pItem) ? 2 : 0;
    }
    nBonus += nSpec;

#if !KOTOR1  // K1 has no feat damage bonus here.
    if (HasFeat(FEAT_INCREASE_COMBAT_DAMAGE_3))
        nBonus += 6;
    else if (HasFeat(FEAT_INCREASE_COMBAT_DAMAGE_2))
        nBonus += 4;
    else if (HasFeat(FEAT_INCREASE_COMBAT_DAMAGE_1))
        nBonus += 2;

    if (HasFeat(FEAT_PRECISE_SHOT_V))
        nBonus += 8;
    else if (HasFeat(FEAT_PRECISE_SHOT_IV))
        nBonus += 6;
    else if (HasFeat(FEAT_MASTER_PRECISE_SHOT))
        nBonus += 4;
    else if (HasFeat(FEAT_IMPROVED_PRECISE_SHOT))
        nBonus += 2;
    else if (HasFeat(FEAT_PRECISE_SHOT))
        nBonus += 1;
#endif
    return nBonus;
}

// STR-based melee damage bonus; off-hand weapons are kept only when double-bladed, and ranged
// weapons get nothing.
int CSWSCreatureStats::GetMeleeDamageBonus(int bWieldingOffhand)
{
    CSWSItem* pItem;
    if (bWieldingOffhand != 0)
    {
        pItem = static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
        if (pItem != 0 &&
            g_pRules->GetBaseItem(pItem->m_baseItem)->m_weaponWieldType !=
                CSWBaseItem::WEAPON_WIELD_TWO_HANDED)
            pItem = 0;
    }
    else
    {
        pItem = static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    }

    if (pItem != 0 &&
        g_pRules->GetBaseItem(pItem->m_baseItem)->m_bRangedWeapon != 0)
        return 0;

    int nBonus;
    if (bWieldingOffhand != 0)
    {
        nBonus = static_cast<signed char>(m_nSTRMod);
        if (nBonus >= 1)
            nBonus = static_cast<int>(static_cast<float>(nBonus) * 0.5f);
    }
    else
    {
        CSWBaseItem* pBase;
        if (pItem != 0 &&
            (pBase = g_pRules->GetBaseItem(pItem->m_baseItem))->m_nWeaponSize -
                    m_pOwner->m_nCreatureSize == 1 &&
            pBase->m_weaponWieldType != CSWBaseItem::WEAPON_WIELD_TWO_HANDED)
        {
            nBonus = static_cast<signed char>(m_nSTRMod);
            if (nBonus >= 1)
                nBonus = static_cast<int>(static_cast<float>(nBonus) * 1.5f);
        }
        else
        {
            nBonus = static_cast<signed char>(m_nSTRMod);
        }
    }

    nBonus += GetWeaponSpecialization(pItem) ? 2 : 0;

#if !KOTOR1  // K1 has no power-attack feat damage here.
    if (HasFeat(FEAT_INCREASE_COMBAT_DAMAGE_3) || HasFeat(FEAT_INCREASE_MELEE_DAMAGE_3))
        nBonus += 6;
    else if (HasFeat(FEAT_INCREASE_COMBAT_DAMAGE_2) || HasFeat(FEAT_INCREASE_MELEE_DAMAGE_2))
        nBonus += 4;
    else if (HasFeat(FEAT_INCREASE_COMBAT_DAMAGE_1) || HasFeat(FEAT_INCREASE_MELEE_DAMAGE_1))
        nBonus += 2;
#endif
    return nBonus;
}

// The weapon's damage range as a min/max pair, min in the first ref parameter.
void CSWSCreatureStats::GetTotalDamageRange(CSWSCreature* pCreature, CSWSItem* pItem,
                                            int& nMin, int& nMax, int bWieldingOffhand, int bAddEffects)
{
    int nMinBase, nMaxBase;
#if KOTOR1
    // K1: weapon dice + full STR mod (melee only) + weapon specialization; unarmed is the size die
    // + STR mod. No damage feats, no off-hand / two-handed STR scaling.
    if (pCreature != 0 && pItem != 0)
    {
        nMinBase = g_pRules->GetBaseItem(pItem->m_baseItem)->m_nNumDice;
        nMaxBase = g_pRules->GetBaseItem(pItem->m_baseItem)->m_nDieToRoll * nMinBase;
        if (pItem->GetBaseItem()->m_bRangedWeapon == 0)
        {
            nMinBase += static_cast<signed char>(m_nSTRMod);
            nMaxBase += static_cast<signed char>(m_nSTRMod);
        }
        if (GetWeaponSpecialization(pItem))
        {
            nMinBase += 2;
            nMaxBase += 2;
        }
    }
    else
    {
        m_pOwner->GetUseMonkAbilities();
        int nCreatureSize = m_pOwner->m_nCreatureSize;
        int nDie;
        if (m_pOwner->GetUseMonkAbilities() == 1)
            nDie = (nCreatureSize < CREATURE_SIZE_MEDIUM) ? 4 : 6;
        else
            nDie = (nCreatureSize < CREATURE_SIZE_MEDIUM) ? 2 : 1;
        nMaxBase = nDie;
        nMinBase = 1;
        if (pCreature != 0)
        {
            nMinBase += static_cast<signed char>(m_nSTRMod);
            nMaxBase += static_cast<signed char>(m_nSTRMod);
        }
    }
#else
    if (pCreature != 0 && pItem != 0)
    {
        nMinBase = g_pRules->GetBaseItem(pItem->m_baseItem)->m_nNumDice;
        nMaxBase = g_pRules->GetBaseItem(pItem->m_baseItem)->m_nDieToRoll * nMinBase;
        if (pItem->GetBaseItem()->m_bRangedWeapon != 0)
        {
            if (HasFeat(FEAT_INCREASE_COMBAT_DAMAGE_3))
            {
                nMinBase += 6;
                nMaxBase += 6;
            }
            else if (HasFeat(FEAT_INCREASE_COMBAT_DAMAGE_2))
            {
                nMinBase += 4;
                nMaxBase += 4;
            }
            else if (HasFeat(FEAT_INCREASE_COMBAT_DAMAGE_1))
            {
                nMinBase += 2;
                nMaxBase += 2;
            }
            if (HasFeat(FEAT_PRECISE_SHOT_V))
            {
                nMinBase += 8;
                nMaxBase += 8;
            }
            else if (HasFeat(FEAT_PRECISE_SHOT_IV))
            {
                nMinBase += 6;
                nMaxBase += 6;
            }
            else if (HasFeat(FEAT_MASTER_PRECISE_SHOT))
            {
                nMinBase += 4;
                nMaxBase += 4;
            }
            else if (HasFeat(FEAT_IMPROVED_PRECISE_SHOT))
            {
                nMinBase += 2;
                nMaxBase += 2;
            }
            else if (HasFeat(FEAT_PRECISE_SHOT))
            {
                nMinBase += 1;
                nMaxBase += 1;
            }
        }
        else
        {
            int nSTRBonus;
            if (bWieldingOffhand != 0)
            {
                nSTRBonus = static_cast<int>(static_cast<float>(static_cast<signed char>(m_nSTRMod)) * 0.5f);
            }
            else if (pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON) == 0 &&
                     (pItem->GetBaseItem()->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_TWO_HANDED ||
                      pItem->GetBaseItem()->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_ONE_HANDED))
            {
                nSTRBonus = static_cast<signed char>(m_nSTRMod);
                if (nSTRBonus >= 1)
                    nSTRBonus = static_cast<int>(static_cast<float>(nSTRBonus) * 1.5f);
            }
            else
            {
                nSTRBonus = static_cast<signed char>(m_nSTRMod);
            }
            nMinBase += nSTRBonus;
            nMaxBase += nSTRBonus;
            if (HasFeat(FEAT_INCREASE_COMBAT_DAMAGE_3) || HasFeat(FEAT_INCREASE_MELEE_DAMAGE_3))
            {
                nMinBase += 6;
                nMaxBase += 6;
            }
            else if (HasFeat(FEAT_INCREASE_COMBAT_DAMAGE_2) || HasFeat(FEAT_INCREASE_MELEE_DAMAGE_2))
            {
                nMinBase += 4;
                nMaxBase += 4;
            }
            else if (HasFeat(FEAT_INCREASE_COMBAT_DAMAGE_1) || HasFeat(FEAT_INCREASE_MELEE_DAMAGE_1))
            {
                nMinBase += 2;
                nMaxBase += 2;
            }
        }
        if (GetWeaponSpecialization(pItem))
        {
            nMinBase += 2;
            nMaxBase += 2;
        }
    }
    else
    {
        m_pOwner->GetUseMonkAbilities();
        int nCreatureSize = m_pOwner->m_nCreatureSize;
        int nDie = 4;
        if (m_pOwner->GetUseMonkAbilities() == 1)
            nDie = (nCreatureSize < CREATURE_SIZE_MEDIUM) ? 4 : 6;
        nMaxBase = nDie;
        nMinBase = 1;
        if (pCreature != 0)
        {
            nMinBase += static_cast<signed char>(m_nSTRMod);
            nMaxBase += static_cast<signed char>(m_nSTRMod);
        }
        if (HasFeat(FEAT_ECHANI_STRIKE_III))
        {
            nMinBase += 3;
            nMaxBase += 12;
        }
        else if (HasFeat(FEAT_ECHANI_STRIKE_II))
        {
            nMinBase += 2;
            nMaxBase += 8;
        }
        else if (HasFeat(FEAT_ECHANI_STRIKE_I))
        {
            nMinBase += 1;
            nMaxBase += 4;
        }
        if (HasFeat(FEAT_UNARMED_SPECIALIST_VIII))
        {
            nMinBase += 8;
            nMaxBase += 32;
        }
        else if (HasFeat(FEAT_UNARMED_SPECIALIST_VII))
        {
            nMinBase += 7;
            nMaxBase += 28;
        }
        else if (HasFeat(FEAT_UNARMED_SPECIALIST_VI))
        {
            nMinBase += 6;
            nMaxBase += 24;
        }
        else if (HasFeat(FEAT_UNARMED_SPECIALIST_V))
        {
            nMinBase += 5;
            nMaxBase += 20;
        }
        else if (HasFeat(FEAT_UNARMED_SPECIALIST_IV))
        {
            nMinBase += 4;
            nMaxBase += 16;
        }
        else if (HasFeat(FEAT_UNARMED_SPECIALIST_III))
        {
            nMinBase += 3;
            nMaxBase += 12;
        }
        else if (HasFeat(FEAT_UNARMED_SPECIALIST_II))
        {
            nMinBase += 2;
            nMaxBase += 8;
        }
        else if (HasFeat(FEAT_UNARMED_SPECIALIST_I))
        {
            nMinBase += 1;
            nMaxBase += 4;
        }
    }
#endif

    if (bAddEffects != 0)
    {
        int nEffectBonus = 0;
        int nBonus = m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_DAMAGE, 0, nEffectBonus, 0, 1,
                                                   0, 0, CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY,
                                                   bWieldingOffhand, 0);
        nMaxBase += nBonus;
        nMinBase += nBonus + nEffectBonus;
    }

    nMin = nMinBase;
    nMax = nMaxBase;
    if (nMin <= 0)
        nMin = 1;
    if (nMax <= 0)
        nMax = 1;
}

// The weapon's critical multiplier, plus the critical-strike feats and a lightsaber stance bonus.
int CSWSCreatureStats::GetCriticalHitMultiplier(int bWieldingOffhand)
{
    CSWSItem* pItem =
            static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    if (bWieldingOffhand != 0 && pItem != 0)
    {
        if (g_pRules->GetBaseItem(pItem->m_baseItem)->m_weaponWieldType !=
            CSWBaseItem::WEAPON_WIELD_TWO_HANDED)
            pItem =
                    static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
    }

    int nMult;
    if (pItem != 0)
        nMult = g_pRules->GetBaseItem(pItem->m_baseItem)->m_nCritMultiplier;
    else
        nMult = 2;
#if !KOTOR1

    unsigned short nFeatId =
        m_pOwner->m_pCombatRound->GetAttack(m_pOwner->m_pCombatRound->m_currentAttack)->m_nFeatId;
    switch (nFeatId)
    {
    case FEAT_IMPROVED_POWER_ATTACK:
    case FEAT_IMPROVED_POWER_BLAST:
    case FEAT_POWER_ATTACK:
    case FEAT_POWER_BLAST:
    case FEAT_MASTER_POWER_BLAST:
    case FEAT_MASTER_POWER_ATTACK:
        nMult++;
        break;
    }

    if (m_pOwner->IsWieldingLightsaber() && m_pOwner->m_stanceId == FORM_SABER_V_SHIEN)
        nMult++;
#endif
    return nMult;
}

// The critical threat range, adjusted by the keen property and the critical-strike feats.
int CSWSCreatureStats::GetCriticalHitRoll(int bWieldingOffhand)
{
    if (m_pOwner == 0)
        return 21;
    CSWSItem* pItem =
            static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    if (bWieldingOffhand != 0 && pItem != 0)
    {
        if (g_pRules->GetBaseItem(pItem->m_baseItem)->m_weaponWieldType !=
            CSWBaseItem::WEAPON_WIELD_TWO_HANDED)
            pItem =
                    static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
    }
    if (pItem == 0)
        pItem = static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_HANDS));

    int nThreat;
    int nRoll = 0;
    if (pItem != 0)
    {
        nThreat = g_pRules->GetBaseItem(pItem->m_baseItem)->m_nCritThreat;
        CSWItemProperty* pProperty = 0;
        // `!= 0` means the property is present; the keen case subtracts the threat, so inverting
        // the test would widen the crit range.
        if (pItem->GetPropertyByType(&pProperty, ITEM_PROPERTY_KEEN, 0) != 0)
            nRoll = -nThreat;
    }
    else
    {
        nThreat = 1;
    }

    unsigned short nFeatId =
        m_pOwner->m_pCombatRound->GetAttack(m_pOwner->m_pCombatRound->m_currentAttack)->m_nFeatId;
    int nExtra = 0;
    switch (nFeatId)
    {
    case FEAT_CRITICAL_STRIKE:
    case FEAT_SNIPER_SHOT:
        nExtra = nThreat;
        break;
    case FEAT_IMPROVED_CRITICAL_STRIKE:
        nExtra = 2 * nThreat;
        break;
    case FEAT_IMPROVED_SNIPER_SHOT:
        nExtra = 2 * nThreat;
        break;
    case FEAT_MASTER_SNIPER_SHOT:
    case FEAT_MASTER_CRITICAL_STRIKE:
        nExtra = 3 * nThreat;
        break;
    }

    return 21 + (nRoll - nThreat - nExtra);
}

// The AC feat bonus for wielding a one-handed weapon alone.
int CSWSCreatureStats::GetTotalACFeatMod()
{
    CSWSItem* pRight =
            static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    CSWSItem* pLeft =
            static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
#if KOTOR1
    // K1: a one-handed (wield type 2/4) right-hand weapon with the left hand empty; there is no
    // both-hands-empty arm.
    if (pRight == 0 || pLeft != 0)
        return 0;
    CSWBaseItem* pBase = pRight->GetBaseItem();
    if (pBase == 0)
        return 0;
    if (pBase->m_weaponWieldType != CSWBaseItem::WEAPON_WIELD_ONE_HANDED &&
        pBase->m_weaponWieldType != CSWBaseItem::WEAPON_WIELD_PISTOL)
        return 0;
#else
    if (pRight != 0 && pLeft == 0)
    {
        CSWBaseItem* pBase = pRight->GetBaseItem();
        if (pBase == 0)
            return 0;
        if (pBase->m_weaponWieldType != CSWBaseItem::WEAPON_WIELD_ONE_HANDED &&
            pBase->m_weaponWieldType != CSWBaseItem::WEAPON_WIELD_PISTOL)
            return 0;
    }
    else if (pRight == 0 && pLeft == 0)
    {
        // With both hands empty there is nothing to reject, so fall through to the feat ladder.
    }
    else
    {
        return 0;
    }
#endif
    if (HasFeat(FEAT_MASTER_DUELING))
        return 3;
    if (HasFeat(FEAT_ADVANCED_DUELING))
        return 2;
    if (HasFeat(FEAT_DUELING))
        return 1;
    return 0;
}

// The unarmed damage die for the creature's size.
int CSWSCreatureStats::GetUnarmedDamageDie()
{
    CSWSCreature* pOwner = m_pOwner;
    int nCreatureSize = pOwner->m_nCreatureSize;
    if (pOwner->GetUseMonkAbilities() == 1)
        return (nCreatureSize < CREATURE_SIZE_MEDIUM) ? 4 : 6;
#if KOTOR1
    // K1 keeps the monk test a branch with two returns.
    return (nCreatureSize < CREATURE_SIZE_MEDIUM) ? 2 : 1;
#else
    return 4;
#endif
}

int CSWSCreatureStats::GetUseMonkAttackTables(int a)
{
    K2_UNUSED(a);
    if (!m_pOwner->GetUseMonkAbilities())
        return 0;
    return m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON) == 0;
}

// Rolls unarmed damage, scaled by the difficulty option and the autobalance table.
int CSWSCreatureStats::GetUnarmedDamageRoll(CSWSObject* pObject)
{
    int nCreatureSize = m_pOwner->m_nCreatureSize;
#if KOTOR1
    int nDie;
    if (m_pOwner->GetUseMonkAbilities() == 1)
        nDie = (nCreatureSize < CREATURE_SIZE_MEDIUM) ? 4 : 6;
    else
        nDie = (nCreatureSize < CREATURE_SIZE_MEDIUM) ? 2 : 1;
#else
    int nDie = 4;
    if (m_pOwner->GetUseMonkAbilities() == 1)
        nDie = (nCreatureSize < CREATURE_SIZE_MEDIUM) ? 4 : 6;
#endif
    m_pOwner->GetUseMonkAbilities();
    int nRoll = g_pRules->RollDice(1, static_cast<unsigned char>(nDie));

    int nFlagA = 0, nFlagB = 0, nPercent = 100;
    if (pObject != 0 && pObject->AsSWSCreature() != 0)
    {
        if (m_bIsPC == 1 && pObject->AsSWSCreature()->m_bPlayerCharacter == 0)
        {
            nPercent = g_pAppManager->m_pServerExoApp->GetDifficultyOption(3);
            nFlagA = 1;
        }
        else if (m_bIsPC == 0 && pObject->AsSWSCreature()->m_bPlayerCharacter == 1)
        {
            nPercent = g_pAppManager->m_pServerExoApp->GetDifficultyOption(4);
            nFlagB = 1;
        }
    }

#if !KOTOR1
    if (m_pOwner->m_bInParty == 0 && m_pOwner->m_nMultiplierSet != 0)
    {
        float fMult;
        g_pRules->m_p2DArrays->m_pAutoBalance->GetFLOATEntry(
            m_pOwner->m_nMultiplierSet, CExoString("DamageMult"), &fMult);
        char nLevel = static_cast<char>(m_pOwner->m_nPCLevelAtSpawn);
        nRoll = (static_cast<int>(fMult * (nLevel == 0 ? 0.0f : static_cast<float>(nLevel - 1))) + 1) * nRoll;
    }
#endif

    return g_pRules->GetWeightedDamageAmount(nRoll, nDie, nFlagA, nFlagB, nPercent);
}

// Returns the D&D ability modifier, floor((nStat - 10) / 2).
int CSWSCreatureStats::CalcStatModifier(unsigned char nStat)
{
    int nDiff = nStat - 10;
    if (nStat < 10)
        nDiff--;
    return nDiff / 2;
}

// Each Set<X>Base writes the base byte, then recomputes the paired modifier byte from the
// effective score, not from the byte just written.

void CSWSCreatureStats::SetSTRBase(unsigned char nScore)
{
    m_nSTRBase = nScore;
    UpdateSTRModifier();
}

void CSWSCreatureStats::SetDEXBase(unsigned char nScore)
{
    m_nDEXBase = nScore;
    UpdateDEXModifier();
}

void CSWSCreatureStats::SetINTBase(unsigned char nScore)
{
    m_nINTBase = nScore;
    UpdateINTModifier();
    AdjustSpellUsesPerDay();
}

void CSWSCreatureStats::SetWISBase(unsigned char nScore)
{
    m_nWISBase = nScore;
    UpdateWISModifier();
    AdjustSpellUsesPerDay();
}

// The only Set<X>Base with a second parameter, bAdjustHP, which also re-bases the current hit
// points.
void CSWSCreatureStats::SetCONBase(unsigned char nScore, int bAdjustHP)
{
    m_nCONBase = nScore;
    int nOldMaxHP = 0;
    if (bAdjustHP == 1)
        nOldMaxHP = m_pOwner->GetMaxHitPoints(1);
    UpdateCONModifier();
    if (bAdjustHP == 1)
    {
        CSWSCreature* pOwner = m_pOwner;
        short nCurHitPoints = pOwner->GetCurrentHitPoints(1);
        pOwner->SetCurrentHitPoints(nCurHitPoints + (m_pOwner->GetMaxHitPoints(1) - nOldMaxHP));
    }
}

void CSWSCreatureStats::SetCHABase(unsigned char nScore)
{
    m_nCHABase = nScore;
    UpdateCHAModifier();
    AdjustSpellUsesPerDay();
}

// The skill's feat id, then the feat-list walk. Persuade has no feat and shares the default arm.
int CSWSCreatureStats::HasClassSkillFeat(unsigned short nSkill)
{
    unsigned short nFeat = 0xffff;
    switch (nSkill)
    {
    case SKILL_COMPUTER_USE:  nFeat = FEAT_CLASS_SKILL_COMPUTER_USE; break;
    case SKILL_DEMOLITIONS:   nFeat = FEAT_CLASS_SKILL_DEMOLITIONS; break;
    case SKILL_STEALTH:       nFeat = FEAT_CLASS_SKILL_STEALTH; break;
    case SKILL_AWARENESS:     nFeat = FEAT_CLASS_SKILL_AWARENESS; break;
    case SKILL_REPAIR:        nFeat = FEAT_CLASS_SKILL_REPAIR; break;
    case SKILL_SECURITY:      nFeat = FEAT_CLASS_SKILL_SECURITY; break;
    case SKILL_TREAT_INJURY:  nFeat = FEAT_CLASS_SKILL_TREAT_INJURY; break;
    }
    return HasFeat(nFeat);
}

// Computes the defender's full AC from the class, armour, shield and natural totals, the dex and
// dodge contributions, the AC effect scan, the stance and lightsaber-form bonuses, and the
// autobalance scaler.
short CSWSCreatureStats::GetArmorClassVersus(CSWSCreature* pCreature, int bVersus)
{
    unsigned char nCurAttack = pCreature->m_pCombatRound->m_currentAttack;
    CSWSCombatAttackData* pAttack = pCreature->m_pCombatRound->GetAttack(nCurAttack);

    int nClassAC = 0;
    if (m_numClasses != 0)
    {
        for (int64_t i = 0; i < m_numClasses; i++)
        {
            nClassAC += static_cast<signed char>(g_pRules->m_paClasses[GetClass(static_cast<unsigned char>(i))]
                            .GetArmorBonus(GetClassLevel(static_cast<unsigned char>(i), 0)));
        }
    }

#if !KOTOR1
    if (m_pOwner->m_bInParty == 0 && m_pOwner->m_nMultiplierSet != 0)
    {
        float fMult;
        g_pRules->m_p2DArrays->m_pAutoBalance->GetFLOATEntry(
            m_pOwner->m_nMultiplierSet, CExoString("ArmorMult"), &fMult);
        signed char nLevel = (m_pOwner->m_nPCLevelAtSpawn != 0) ? m_pOwner->m_nPCLevelAtSpawn : 1;
        nClassAC += static_cast<int>(fMult * static_cast<float>(nLevel));
    }
#endif
    pAttack->m_defense.m_nACClass = static_cast<signed char>(nClassAC);

    int nArmorAC = 0, nNaturalAC = 0, nShieldAC = 0;
    if (bVersus == 0)
    {
        nNaturalAC = GetTotalACNaturalMod();
        nArmorAC = static_cast<signed char>(m_nACArmorMod + m_nACArmorBase - m_nACArmorNeg);
        nShieldAC = static_cast<signed char>(m_nACShieldMod + m_nACShieldBase - m_nACShieldNeg);
    }
    pAttack->m_defense.m_nACArmor = static_cast<signed char>(nArmorAC);
    pAttack->m_defense.m_nACNatural = static_cast<signed char>(nNaturalAC);

    if (g_bEnableCombatDebugging != 0)
    {
        CExoString sLog = pCreature->m_pCombatRound
                              ->GetAttack(pCreature->m_pCombatRound->m_currentAttack)
                              ->m_sAttackDebugText;
        CExoString sBonus;
        if (nNaturalAC != 0)
        {
            sBonus.Format(const_cast<char*>(" + %d (Natural AC)"), nNaturalAC);
            pCreature->m_pCombatRound->GetAttack(nCurAttack)
                ->m_sAttackDebugText = sLog + sBonus;
            sLog = pCreature->m_pCombatRound->GetAttack(nCurAttack)
                       ->m_sAttackDebugText;
        }
        if (nArmorAC != 0)
        {
            sBonus.Format(const_cast<char*>(" + %d (Armor AC)"), nArmorAC);
            pCreature->m_pCombatRound->GetAttack(nCurAttack)
                ->m_sAttackDebugText = sLog + sBonus;
            sLog = pCreature->m_pCombatRound->GetAttack(nCurAttack)
                       ->m_sAttackDebugText;
        }
        if (nShieldAC != 0)
        {
            sBonus.Format(const_cast<char*>(" + %d (Shield AC)"), nShieldAC);
            pCreature->m_pCombatRound->GetAttack(nCurAttack)
                ->m_sAttackDebugText = sLog + sBonus;
            sLog = pCreature->m_pCombatRound->GetAttack(nCurAttack)
                       ->m_sAttackDebugText;
        }
    }

    signed char nDexContrib, nDexMod;
    if (pCreature->GetInvisible(m_pOwner->AsSWSObject()) != 0)
    {
        nDexContrib = 0;
        nDexMod = static_cast<signed char>(GetDEXMod(1));
        if (nDexMod < 0)
            nDexMod = static_cast<signed char>(GetDEXMod(1));
        else
            nDexMod = 0;
    }
    else
    {
        nDexContrib = 0;
        nDexMod = 0;
        CSWVisibilityNode* pVis = m_pOwner->GetVisibleListElement(pCreature->m_objectId);
        if (pVis != 0 && (pVis->m_flags & 1) != 0)
        {
            int nDodge = static_cast<signed char>(m_nACDodgeMod) - static_cast<signed char>(m_nACDodgeNeg);
            if (nDodge > 10)
                nDodge = 10;
            nDexContrib = static_cast<signed char>(nDodge);
            nDexMod = static_cast<signed char>(GetDEXMod(1));
            if (g_bEnableCombatDebugging != 0)
            {
                CExoString sLog = pCreature->m_pCombatRound
                                      ->GetAttack(pCreature->m_pCombatRound->m_currentAttack)
                                      ->m_sAttackDebugText;
                CExoString sBonus;
            }
        }
    }
    pAttack->m_defense.m_nACDodge = nDexContrib;
    pAttack->m_defense.m_nACDex = nDexMod;
    pAttack->m_defense.m_nACDexUncapped = static_cast<signed char>(GetDEXMod(0));

    if (g_bEnableCombatDebugging != 0)
    {
        CExoString sLog = pCreature->m_pCombatRound
                              ->GetAttack(pCreature->m_pCombatRound->m_currentAttack)
                              ->m_sAttackDebugText;
        CExoString sBonus;
        if (nDexMod != 0)
        {
            sBonus.Format(const_cast<char*>(" + %d (Dex Mod)"), nDexMod);
            pCreature->m_pCombatRound->GetAttack(nCurAttack)
                ->m_sAttackDebugText = sLog + sBonus;
            sLog = pCreature->m_pCombatRound->GetAttack(nCurAttack)
                       ->m_sAttackDebugText;
        }
        if (nDexContrib != 0)
        {
            sBonus.Format(const_cast<char*>(" + %d (Dodge Mod)"), nDexContrib);
            pCreature->m_pCombatRound->GetAttack(nCurAttack)
                ->m_sAttackDebugText = sLog + sBonus;
            sLog = pCreature->m_pCombatRound->GetAttack(nCurAttack)
                       ->m_sAttackDebugText;
        }
    }

    int nArmorBase = static_cast<signed char>(m_nACArmorMod);
    int nShieldBase = static_cast<signed char>(m_nACShieldMod);
    int nDeflBase = static_cast<signed char>(m_nACDeflectionMod);
    int nAlignBand =
        (pCreature->m_pStats->m_nAlignment < 41)
            ? 3
            : 1 + (pCreature->m_pStats->m_nAlignment > 59);
    int nRace = pCreature->m_pStats->m_nRace;
    int nDamageFlags = pCreature->GetDamageFlags();

    int nDodgeSum = 0, nNaturalSum = 0, nArmorEff = 0, nShieldEff = 0, nDeflEff = 0;
    CSWSCreature* pOwner = m_pOwner;
    if (pOwner->m_effects.m_count > 0)
    for (int k = static_cast<unsigned short>(m_iFirstACIncrease); k < pOwner->m_effects.m_count; k++)
    {
        CGameEffect* pEffect = pOwner->m_effects.m_pArray[k];
// The effect list is sorted by subtype; the first non-AC effect ends the scan.
        if (pEffect->m_subType != CSWSEffectListHandler::EFFECT_AC_INCREASE &&
            pEffect->m_subType != CSWSEffectListHandler::EFFECT_AC_DECREASE)
            break;
        int nACType = pEffect->GetInteger(0);
        int nAmount = pEffect->GetInteger(1);
        int nEffRace = pEffect->GetInteger(2);
        int nEffClass = pEffect->GetInteger(4);
        int nEffDmg = pEffect->GetInteger(5);
        pEffect->GetInteger(6);
        if (nEffDmg == CSWSEffectListHandler::DAMAGE_FLAG_PHYSICAL && nEffClass == 0 &&
            nEffRace == g_pRules->m_nNumRaces)
            continue;
        if ((nEffRace == g_pRules->m_nNumRaces || nEffRace == nRace) &&
            (nEffClass == 0 || nEffClass == nAlignBand) &&
            (nEffDmg == CSWSEffectListHandler::DAMAGE_FLAG_PHYSICAL ||
             (nDamageFlags == 0 && nEffDmg == CSWSEffectListHandler::DAMAGE_FLAG_BLUDGEONING) ||
             (nDamageFlags == 1 && nEffDmg == CSWSEffectListHandler::DAMAGE_FLAG_PIERCING) ||
             (nDamageFlags == 2 && nEffDmg == CSWSEffectListHandler::DAMAGE_FLAG_SLASHING)))
        {
            unsigned short nSubType = pOwner->m_effects.m_pArray[k]->m_subType;
            if (nACType == CSWSEffectListHandler::AC_DODGE)
            {
                if (nSubType == CSWSEffectListHandler::EFFECT_AC_INCREASE)
                    nDodgeSum += nAmount;
                else
                    nDodgeSum -= nAmount;
            }
            if (nACType == CSWSEffectListHandler::AC_NATURAL)
            {
                if (nSubType == CSWSEffectListHandler::EFFECT_AC_INCREASE)
                    nNaturalSum += nAmount;
                else
                    nNaturalSum -= nAmount;
            }
            if (nSubType == CSWSEffectListHandler::EFFECT_AC_INCREASE)
            {
                if (nACType == CSWSEffectListHandler::AC_ARMOUR_BONUS && bVersus == 0 &&
                    nAmount > nArmorBase)
                    nArmorBase = nAmount;
                if (nACType == CSWSEffectListHandler::AC_SHIELD_BONUS && bVersus == 0 &&
                    nAmount > nShieldBase)
                    nShieldBase = nAmount;
                if (nACType == CSWSEffectListHandler::AC_DEFLECTION && nAmount > nDeflBase)
                    nDeflBase = nAmount;
            }
            else
            {
                if (nACType == CSWSEffectListHandler::AC_ARMOUR_BONUS && bVersus == 0 &&
                    nAmount > nArmorEff)
                    nArmorEff = nAmount;
                if (nACType == CSWSEffectListHandler::AC_SHIELD_BONUS && bVersus == 0 &&
                    nAmount > nShieldEff)
                    nShieldEff = nAmount;
                if (nACType == CSWSEffectListHandler::AC_SHIELD_BONUS && bVersus == 0 &&
                    nAmount > nDeflEff)
                    nDeflEff = nAmount;
            }
        }
    }

#if !KOTOR1
    int nStanceAC = 0;
    if (m_pOwner->IsInTotalDefense() != 0)
    {
        nStanceAC = 4;
        if (m_numClasses != 0)
        {
            int nDiff = m_classInfo[0].m_level - m_classInfo[0].m_negativeLevels;
            if (nDiff < 0)
                nDiff = 0;
            if ((m_classInfo[0].m_class == CLASS_TYPE_SCOUNDREL || m_classInfo[0].m_class == CLASS_TYPE_JEDISENTINEL ||
                 m_classInfo[0].m_class == CLASS_TYPE_JEDIWATCHMAN || m_classInfo[0].m_class == CLASS_TYPE_SITHASSASSIN) &&
                static_cast<unsigned char>(nDiff) > 3)
            {
                nStanceAC = 6;
            }
            else if (m_numClasses > 1)
            {
                nDiff = m_classInfo[1].m_level - m_classInfo[1].m_negativeLevels;
                if (nDiff < 0)
                    nDiff = 0;
                if ((m_classInfo[1].m_class == CLASS_TYPE_SCOUNDREL || m_classInfo[1].m_class == CLASS_TYPE_JEDISENTINEL ||
                     m_classInfo[1].m_class == CLASS_TYPE_JEDIWATCHMAN || m_classInfo[1].m_class == CLASS_TYPE_SITHASSASSIN) &&
                    static_cast<unsigned char>(nDiff) > 3)
                    nStanceAC = 6;
            }
        }
    }

    int nSaberAC = 0;
    if (m_pOwner->IsWieldingLightsaber() != 0)
    {
        switch (m_pOwner->m_stanceId)
        {
        case FORM_SABER_I_SHII_CHO:
            if (pCreature->m_objectId == m_pOwner->m_attemptedAttackTargetId ||
                pCreature->m_objectId == m_pOwner->m_oidAttackTarget)
                nSaberAC = 0;
            else
                nSaberAC = 3;
            break;
        case FORM_SABER_III_SORESU:
            if (pCreature->m_objectId == m_pOwner->m_attemptedAttackTargetId ||
                pCreature->m_objectId == m_pOwner->m_oidAttackTarget)
                nSaberAC = 2;
            else
                nSaberAC = 0;
            break;
        case FORM_SABER_IV_ATARU:
            if (pCreature->m_objectId == m_pOwner->m_attemptedAttackTargetId ||
                pCreature->m_objectId == m_pOwner->m_oidAttackTarget)
                nSaberAC = 3;
            else
                nSaberAC = -2;
            break;
        case FORM_SABER_V_SHIEN:
            if (pCreature->m_objectId == m_pOwner->m_attemptedAttackTargetId ||
                pCreature->m_objectId == m_pOwner->m_oidAttackTarget)
                nSaberAC = -5;
            else
                nSaberAC = 0;
            break;
        case FORM_SABER_VI_NIMAN:
            nSaberAC = 1;
            break;
        case FORM_SABER_VII_JUYO:
            if (pCreature->m_objectId == m_pOwner->m_attemptedAttackTargetId ||
                pCreature->m_objectId == m_pOwner->m_oidAttackTarget)
                nSaberAC = -2;
            else
                nSaberAC = -4;
            break;
        }
    }
#endif

    if (g_bEnableCombatDebugging != 0)
    {
        CExoString sLog = pCreature->m_pCombatRound
                              ->GetAttack(pCreature->m_pCombatRound->m_currentAttack)
                              ->m_sAttackDebugText;
        CExoString sBonus;
        if (nDodgeSum != 0)
        {
            sBonus.Format(const_cast<char*>(" + %d (Effect AC Dodge Bonus)"), nDodgeSum);
            pCreature->m_pCombatRound->GetAttack(nCurAttack)
                ->m_sAttackDebugText = sLog + sBonus;
            sLog = pCreature->m_pCombatRound->GetAttack(nCurAttack)
                       ->m_sAttackDebugText;
        }
        if (nNaturalSum != 0)
        {
            sBonus.Format(const_cast<char*>(" + %d (Effect AC Natural Bonus)"), nNaturalSum);
            pCreature->m_pCombatRound->GetAttack(nCurAttack)
                ->m_sAttackDebugText = sLog + sBonus;
            sLog = pCreature->m_pCombatRound->GetAttack(nCurAttack)
                       ->m_sAttackDebugText;
        }
        if (nArmorBase != 0)
        {
            sBonus.Format(const_cast<char*>(" + %d (Effect AC Armor Bonus)"), nArmorEff + nArmorBase);
            pCreature->m_pCombatRound->GetAttack(nCurAttack)
                ->m_sAttackDebugText = sLog + sBonus;
            sLog = pCreature->m_pCombatRound->GetAttack(nCurAttack)
                       ->m_sAttackDebugText;
        }
        if (nShieldBase != 0)
        {
            sBonus.Format(const_cast<char*>(" + %d (Effect AC Shield Bonus)"), nShieldEff + nShieldBase);
            pCreature->m_pCombatRound->GetAttack(nCurAttack)
                ->m_sAttackDebugText = sLog + sBonus;
            sLog = pCreature->m_pCombatRound->GetAttack(nCurAttack)
                       ->m_sAttackDebugText;
        }
        if (nDeflBase != 0)
        {
            sBonus.Format(const_cast<char*>(" + %d (Effect AC Deflection Bonus)"), nDeflEff + nDeflBase);
            pCreature->m_pCombatRound->GetAttack(nCurAttack)
                ->m_sAttackDebugText = sLog + sBonus;
            sLog = pCreature->m_pCombatRound->GetAttack(nCurAttack)
                       ->m_sAttackDebugText;
        }
#if !KOTOR1
        if (nStanceAC != 0)
        {
            sBonus.Format(const_cast<char*>(" + %d (Total Defense Stance Bonus)"), nStanceAC);
            pCreature->m_pCombatRound->GetAttack(nCurAttack)
                ->m_sAttackDebugText = sLog + sBonus;
            sLog = pCreature->m_pCombatRound->GetAttack(nCurAttack)
                       ->m_sAttackDebugText;
        }
        if (nSaberAC != 0)
        {
            sBonus.Format(const_cast<char*>(" + %d"), nSaberAC);
            pCreature->m_pCombatRound->GetAttack(nCurAttack)
                ->m_sAttackDebugText = sLog + sBonus;
            sLog = pCreature->m_pCombatRound->GetAttack(nCurAttack)
                       ->m_sAttackDebugText;
        }
#endif
    }

#if KOTOR1
    int nStanceSum = nDeflEff + nDeflBase + nDodgeSum;
#else
    int nStanceSum = nDeflEff + nDeflBase + nDodgeSum + nStanceAC + nSaberAC;
#endif
    pAttack->m_defense.m_nACDodge += static_cast<signed char>(nStanceSum);
    int nAC = nClassAC + nShieldAC + nArmorAC + nNaturalAC + nDexContrib + nDexMod + nStanceSum + 10;

#if !KOTOR1
    if (HasSpell(0, FORCE_POWER_BATTLE_PRECOGNITION, 0) != 0)
    {
        CSWSItem* pArmor = static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_BODY));
        CSWBaseItem* pArmorBase = 0;
        if (pArmor != 0)
            pArmorBase = pArmor->GetBaseItem();
        if ((pArmor == 0 || (pArmorBase != 0 && pArmorBase->m_nBaseAC == 0)) && m_nWISMod >= 1)
        {
            nAC += m_nWISMod;
            pAttack->m_defense.m_nACDodge += m_nWISMod;
        }
    }
#endif

    if (bVersus != 0)
        nAC += nArmorBase + nArmorEff + nShieldBase + nShieldEff;

    int nFeatMod = GetTotalACFeatMod();
    pAttack->m_defense.m_nACFeatMod = static_cast<signed char>(nFeatMod);
    nAC += static_cast<signed char>(nFeatMod);
    if (m_pOwner->IsDebilitated(1) != 0)
    {
        nAC -= 4;
        pAttack->m_defense.m_nACDebilitatedPenalty = -4;
    }
    return static_cast<short>(nAC);
}


// Rebuilds m_pCombatInformation from the two filtered effect passes.
void CSWSCreatureStats::UpdateCombatInformation()
{
    CExoArrayList<CGameEffect*> effects;
    CCombatInformation* pInfo;
    CSWSItem* pOnHand;
    CSWSItem* pOffHand;
    signed char nResistance;
    int nCreatureSize;
    int nDie;
    if (m_pCombatInformation != 0)
    {
        delete m_pCombatInformation;
        m_pCombatInformation = new CCombatInformation();
    }
    K2_ASSUME(m_pCombatInformation != 0);

    {
        CExoArrayList<CGameEffect*> filtered = m_pOwner->GetFilteredEffectList(1);
        for (int64_t i = 0; i < filtered.m_count; i++)
            effects.Add(filtered.m_pArray[i]);
    }
    for (int64_t i = 0; i < effects.m_count; i++)
    {
        CGameEffect* pEffect = effects.m_pArray[i];
        CCombatInformationNode* pNode;
        if (pEffect->GetInteger(1) == CSWSEffectListHandler::ATTACK_BONUS_ONHAND ||
            pEffect->GetInteger(1) == CSWSEffectListHandler::ATTACK_BONUS_OFFHAND ||
            pEffect->GetInteger(1) == CSWSEffectListHandler::ATTACK_BONUS_UNARMED ||
            pEffect->GetInteger(1) == CSWSEffectListHandler::ATTACK_BONUS_MISC)
        {
            pNode = new CCombatInformationNode();
            if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_ATTACK_INCREASE)
                pNode->m_cValue = static_cast<signed char>(pEffect->GetInteger(0));
            else
                pNode->m_cValue = static_cast<signed char>(-pEffect->GetInteger(0));
            if (pEffect->GetInteger(1) == CSWSEffectListHandler::ATTACK_BONUS_ONHAND)
                pNode->m_nType = 1;
            else if (pEffect->GetInteger(1) == CSWSEffectListHandler::ATTACK_BONUS_OFFHAND)
                pNode->m_nType = 2;
            else if (pEffect->GetInteger(1) == CSWSEffectListHandler::ATTACK_BONUS_MISC)
                pNode->m_nType = 6;
            else if (pEffect->GetInteger(1) == CSWSEffectListHandler::ATTACK_BONUS_UNARMED)
                pNode->m_nType = 7;
            if (pEffect->GetInteger(2) != g_pRules->m_nNumRaces)
                pNode->m_nClassReq = static_cast<unsigned char>(pEffect->GetInteger(2));
            if (pEffect->GetInteger(4) != 0)
                pNode->m_nFlag = static_cast<unsigned char>(pEffect->GetInteger(4));
            m_pCombatInformation->m_attackBonusMods.Add(pNode);
        }
    }
    effects.m_count = 0;
    {
        CExoArrayList<CGameEffect*> filtered = m_pOwner->GetFilteredEffectList(2);
        for (int64_t i = 0; i < filtered.m_count; i++)
            effects.Add(filtered.m_pArray[i]);
    }
    for (int64_t i = 0; i < effects.m_count; i++)
    {
        CGameEffect* pEffect = effects.m_pArray[i];
        CCombatInformationNode* pNode;
        unsigned short nDamageFlags;
        if (pEffect->GetInteger(5) == CSWSEffectListHandler::DAMAGE_BONUS_ONHAND ||
            pEffect->GetInteger(5) == CSWSEffectListHandler::DAMAGE_BONUS_OFFHAND ||
            pEffect->GetInteger(5) == CSWSEffectListHandler::DAMAGE_BONUS_MISC ||
            pEffect->GetInteger(5) == CSWSEffectListHandler::DAMAGE_BONUS_UNARMED)
        {
            pNode = new CCombatInformationNode();
            if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_DAMAGE_INCREASE)
                pNode->m_cValue = static_cast<signed char>(pEffect->GetInteger(0));
            else
                pNode->m_cValue = static_cast<signed char>(-pEffect->GetInteger(0));
            nDamageFlags = static_cast<unsigned short>(pEffect->GetInteger(1));
            if (nDamageFlags > (CSWSEffectListHandler::DAMAGE_FLAG_PHYSICAL &
                                ~CSWSEffectListHandler::DAMAGE_FLAG_BASE_DAMAGE))
                pNode->m_nModifierType = static_cast<unsigned char>(Log_Base2(nDamageFlags));
            else if (nDamageFlags == CSWSEffectListHandler::DAMAGE_FLAG_PIERCING)
                pNode->m_nModifierType = 1;
            else if (nDamageFlags == CSWSEffectListHandler::DAMAGE_FLAG_SLASHING)
                pNode->m_nModifierType = 2;
            else if (nDamageFlags == CSWSEffectListHandler::DAMAGE_FLAG_BLUDGEONING)
                pNode->m_nModifierType = 0;
            else
                pNode->m_nModifierType = 14;
            if (pEffect->GetInteger(5) == CSWSEffectListHandler::DAMAGE_BONUS_ONHAND)
                pNode->m_nType = 1;
            else if (pEffect->GetInteger(5) == CSWSEffectListHandler::DAMAGE_BONUS_OFFHAND)
                pNode->m_nType = 2;
            else if (pEffect->GetInteger(5) == CSWSEffectListHandler::DAMAGE_BONUS_MISC)
                pNode->m_nType = 6;
            else if (pEffect->GetInteger(5) == CSWSEffectListHandler::DAMAGE_BONUS_UNARMED)
                pNode->m_nType = 7;
            if (pEffect->GetInteger(2) != g_pRules->m_nNumRaces)
                pNode->m_nClassReq = static_cast<unsigned char>(pEffect->GetInteger(2));
            if (pEffect->GetInteger(4) != 0)
                pNode->m_nFlag = static_cast<unsigned char>(pEffect->GetInteger(4));
            m_pCombatInformation->m_damageBonusMods.Add(pNode);
        }
    }

    pInfo = m_pCombatInformation;
    pInfo->m_nNumOnHandAttacks = 1;
    pOnHand = static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    pOffHand = static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
    if (pOffHand != 0 &&
        g_pRules->GetBaseItem(pOffHand->m_baseItem)->m_nWeaponType != 0)
        m_pCombatInformation->m_bOffHandWeaponEquipped = 1;
    else if (pOnHand != 0 &&
             g_pRules->GetBaseItem(pOnHand->m_baseItem)->m_weaponWieldType !=
                 CSWBaseItem::WEAPON_WIELD_TWO_HANDED)
        m_pCombatInformation->m_bOffHandWeaponEquipped = 0;

    if (pOffHand != 0)
        m_pCombatInformation->m_offHandWeaponId = pOffHand->m_objectId;
    else
        m_pCombatInformation->m_offHandWeaponId = OBJECT_INVALID;

    if (pOnHand != 0 &&
        g_pRules->GetBaseItem(pOnHand->m_baseItem)->m_bRangedWeapon != 0)
    {
        m_pCombatInformation->m_nAttackModOnHand = static_cast<signed char>(GetRangedAttackBonus(0, 0, 0));
        m_pCombatInformation->m_nStrDamageModOnHand = static_cast<signed char>(GetRangedDamageBonus());
    }
    else
    {
        m_pCombatInformation->m_nAttackModOnHand = static_cast<signed char>(GetMeleeAttackBonus(0, 0, 0, 0));
        m_pCombatInformation->m_nStrDamageModOnHand = static_cast<signed char>(GetMeleeDamageBonus(0));
    }

    if (pOnHand != 0)
        m_pCombatInformation->m_onHandWeaponId = pOnHand->m_objectId;
    else
        m_pCombatInformation->m_onHandWeaponId = OBJECT_INVALID;

    if (m_pCombatInformation->m_bOffHandWeaponEquipped == 1)
    {
        m_pCombatInformation->m_nAttackModOffHand = static_cast<signed char>(GetMeleeAttackBonus(1, 0, 0, 0));
        m_pCombatInformation->m_nStrDamageModOffHand = static_cast<signed char>(GetMeleeDamageBonus(1));
    }
    GetTotalArcaneSpellFailure();
    nResistance = static_cast<signed char>(m_forceResistance - m_forceResistancePenalty);
    if (nResistance < 0)
        nResistance = 0;
    m_pCombatInformation->m_nForceResistance = nResistance;
    m_pCombatInformation->m_bArmorCheckPenalty =
        -(reinterpret_cast<unsigned char*>(&m_nArmorCheckPenalty)[1] + reinterpret_cast<unsigned char*>(&m_nArmorCheckPenalty)[0]);
    m_pOwner->GetUseMonkAbilities();
    m_pCombatInformation->m_nDefaultDamageDie = 1;
    nCreatureSize = m_pOwner->m_nCreatureSize;
#if KOTOR1
    nDie = (nCreatureSize < CREATURE_SIZE_MEDIUM) ? 2 : 1;
#else
    nDie = 4;
#endif
    if (m_pOwner->GetUseMonkAbilities() == 1)
        nDie = (nCreatureSize < CREATURE_SIZE_MEDIUM) ? 4 : 6;
    m_pCombatInformation->m_nDefaultDamageDiceCount = static_cast<unsigned char>(nDie);
    m_pCombatInformation->m_nCritMultiplierOnHand = static_cast<unsigned char>(GetCriticalHitMultiplier(0));
    m_pCombatInformation->m_nCritRangeOnHand = static_cast<unsigned char>(21 - GetCriticalHitRoll(0));
    m_pCombatInformation->m_nCritMultiplierOffHand = static_cast<unsigned char>(GetCriticalHitMultiplier(1));
    m_pCombatInformation->m_nCritRangeOffHand = static_cast<unsigned char>(21 - GetCriticalHitRoll(1));
}

// The total damage bonus for the current attack.
char CSWSCreatureStats::GetDamageBonus(CSWSCreature* pCreature, int bWieldingOffhand, int a3)
{
    K2_UNUSED(a3);
// The attack index is read once; only the feat switch re-reads it.
    unsigned char nCurAttack = m_pOwner->m_pCombatRound->m_currentAttack;
    CSWSCombatAttackData* pAttack =
        m_pOwner->m_pCombatRound->GetAttack(nCurAttack);
    int bRanged = pAttack->m_bRangedAttack;
    unsigned char nCritical = pAttack->m_nAttackResult;
    unsigned char nAttackType = pAttack->m_nWeaponAttackType;
    CSWSItem* pWeapon = m_pOwner->m_pCombatRound->GetCurrentAttackWeapon(nAttackType);

#if KOTOR1
// K1: the STR piece is the raw mod (0 for a ranged attack) in either hand; combat modes give
// 10/5; then specialization and the special attack only.
    CSWSItem* pOnHand =
            static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    if (pOnHand != 0)
        g_pRules->GetBaseItem(pOnHand->m_baseItem);
    if (bRanged != 0)
        pAttack->m_damage.m_nStrDamageBonus = 0;
    else
        pAttack->m_damage.m_nStrDamageBonus = m_nSTRMod;

    int nBonus = (m_pOwner->m_combatMode == CSWSCreature::CREATURE_COMBAT_MODE_IMPROVED_POWER_ATTACK) ? 10
                 : (m_pOwner->m_combatMode == CSWSCreature::CREATURE_COMBAT_MODE_POWER_ATTACK) ? 5
                 : 0;
#else
    if (bWieldingOffhand != 0)
    {
        int nSTRBonus = static_cast<signed char>(m_nSTRMod);
        if (nSTRBonus >= 1)
            nSTRBonus = static_cast<int>(static_cast<float>(nSTRBonus) * 0.5f);
        pAttack->m_damage.m_nStrDamageBonus = static_cast<signed char>(nSTRBonus);
        if (g_bEnableCombatDebugging != 0)
        {
            CExoString sLog = m_pOwner->m_pCombatRound
                                  ->GetAttack(nCurAttack)
                                  ->m_sDamageDebugText;
            CExoString sBonus;
            if (nCritical == 2)
                sBonus.Format(const_cast<char*>(" + %d (Offhand Strength Bonus) (Critical x%d)"),
                              nSTRBonus * GetCriticalHitMultiplier(bWieldingOffhand),
                              GetCriticalHitMultiplier(bWieldingOffhand));
            else
                sBonus.Format(const_cast<char*>(" + %d (Offhand Strength Bonus)"), nSTRBonus);
            m_pOwner->m_pCombatRound->GetAttack(nCurAttack)
                ->m_sDamageDebugText = sLog + sBonus;
        }
    }
    else
    {
        CSWSItem* pOnHand =
                static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
        CSWBaseItem* pBase = 0;
        if (pOnHand != 0)
            pBase = pOnHand->GetBaseItem();
        CSWSItem* pOffHand =
                static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
        if (bRanged != 0)
        {
            pAttack->m_damage.m_nStrDamageBonus = 0;
        }
        else if (pOnHand != 0 && pOffHand == 0 &&
                 (pBase->m_weaponWieldType & 0xfe) == 2)
        {
            int nSTRBonus = static_cast<signed char>(m_nSTRMod);
            if (nSTRBonus >= 1)
                nSTRBonus = static_cast<int>(static_cast<float>(nSTRBonus) * 1.5f);
            pAttack->m_damage.m_nStrDamageBonus = static_cast<signed char>(nSTRBonus);
        }
        else
        {
            pAttack->m_damage.m_nStrDamageBonus = m_nSTRMod;
        }
    }

    int nBonus = (m_pOwner->m_combatMode == CSWSCreature::CREATURE_COMBAT_MODE_IMPROVED_POWER_ATTACK) ? 7
                 : (m_pOwner->m_combatMode == CSWSCreature::CREATURE_COMBAT_MODE_POWER_ATTACK) ? 3
                 : 0;
#endif
    if (g_bEnableCombatDebugging != 0 && nBonus != 0)
    {
        CExoString sLog = m_pOwner->m_pCombatRound
                              ->GetAttack(nCurAttack)
                              ->m_sDamageDebugText;
        CExoString sBonus;
        if (nCritical == 2)
            sBonus.Format(const_cast<char*>(" + %d (Power/Improved Power Attack Damage Bonus) (Critical x%d)"),
                          nBonus * GetCriticalHitMultiplier(bWieldingOffhand),
                          GetCriticalHitMultiplier(bWieldingOffhand));
        else
            sBonus.Format(const_cast<char*>(" + %d (Power/Improved Power Attack Damage Bonus)"), nBonus);
        m_pOwner->m_pCombatRound->GetAttack(nCurAttack)
            ->m_sDamageDebugText = sLog + sBonus;
    }

    int nSpec = GetWeaponSpecialization(pWeapon) ? 2 : 0;
    pAttack->m_damage.m_nWeaponSpecializationBonus = static_cast<unsigned char>(nSpec);
    if (nSpec != 0 && g_bEnableCombatDebugging != 0)
    {
        CExoString sLog = m_pOwner->m_pCombatRound
                              ->GetAttack(nCurAttack)
                              ->m_sDamageDebugText;
        CExoString sBonus;
        if (nCritical == 2)
            sBonus.Format(const_cast<char*>(" + %d (Weapon Specialization Bonus) (Critical x%d)"),
                          nSpec * GetCriticalHitMultiplier(bWieldingOffhand),
                          GetCriticalHitMultiplier(bWieldingOffhand));
        else
            sBonus.Format(const_cast<char*>(" + %d (Weapon Specialization Bonus)"), nSpec);
        m_pOwner->m_pCombatRound->GetAttack(nCurAttack)
            ->m_sDamageDebugText = sLog + sBonus;
    }

    int nSpecial = ResolveSpecialAttackDamageBonus(pCreature);
    if (nSpecial != 0)
    {
        nBonus += nSpecial;
        if (g_bEnableCombatDebugging != 0)
        {
            CExoString sLog = m_pOwner->m_pCombatRound
                                  ->GetAttack(nCurAttack)
                                  ->m_sDamageDebugText;
            CExoString sBonus;
            if (nCritical == 2)
                sBonus.Format(const_cast<char*>(" + %d (Special Attack Damage Bonus) (Critical x%d)"),
                              nSpecial * GetCriticalHitMultiplier(bWieldingOffhand),
                              GetCriticalHitMultiplier(bWieldingOffhand));
            else
                sBonus.Format(const_cast<char*>(" + %d (Special Attack Damage Bonus)"), nSpecial);
            m_pOwner->m_pCombatRound->GetAttack(nCurAttack)
                ->m_sDamageDebugText = sLog + sBonus;
        }
    }

#if !KOTOR1
    if (pAttack->m_bRangedAttack != 0)
    {
        if (HasFeat(FEAT_INCREASE_COMBAT_DAMAGE_3))
            nBonus += 6;
        else if (HasFeat(FEAT_INCREASE_COMBAT_DAMAGE_2))
            nBonus += 4;
        else if (HasFeat(FEAT_INCREASE_COMBAT_DAMAGE_1))
            nBonus += 2;
        if (HasFeat(FEAT_PRECISE_SHOT_V))
            nBonus += 8;
        else if (HasFeat(FEAT_PRECISE_SHOT_IV))
            nBonus += 6;
        else if (HasFeat(FEAT_MASTER_PRECISE_SHOT))
            nBonus += 4;
        else if (HasFeat(FEAT_IMPROVED_PRECISE_SHOT))
            nBonus += 2;
        else if (HasFeat(FEAT_PRECISE_SHOT))
            nBonus += 1;
    }
    else
    {
        if (HasFeat(FEAT_INCREASE_COMBAT_DAMAGE_3) || HasFeat(FEAT_INCREASE_MELEE_DAMAGE_3))
            nBonus += 6;
        else if (HasFeat(FEAT_INCREASE_COMBAT_DAMAGE_2) || HasFeat(FEAT_INCREASE_MELEE_DAMAGE_2))
            nBonus += 4;
        else if (HasFeat(FEAT_INCREASE_COMBAT_DAMAGE_1) || HasFeat(FEAT_INCREASE_MELEE_DAMAGE_1))
            nBonus += 2;
    }

    if (m_nFuryDamageBonus != -1)
        nBonus += m_nFuryDamageBonus;
    if (m_pOwner->IsWieldingLightsaber())
        nBonus += (m_pOwner->m_stanceId == FORM_SABER_II_MAKASHI) ? 3 : 0;

    if (nAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_UNARMED ||
        nAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_UNARMED_EXTRA)
    {
        if (HasFeat(FEAT_ECHANI_STRIKE_III))
            nBonus += g_pRules->RollDice(3, 4);
        else if (HasFeat(FEAT_ECHANI_STRIKE_II))
            nBonus += g_pRules->RollDice(2, 4);
        else if (HasFeat(FEAT_ECHANI_STRIKE_I))
            nBonus += g_pRules->RollDice(1, 4);
        if (HasFeat(FEAT_UNARMED_SPECIALIST_VIII))
            nBonus += g_pRules->RollDice(8, 4);
        else if (HasFeat(FEAT_UNARMED_SPECIALIST_VII))
            nBonus += g_pRules->RollDice(7, 4);
        else if (HasFeat(FEAT_UNARMED_SPECIALIST_VI))
            nBonus += g_pRules->RollDice(6, 4);
        else if (HasFeat(FEAT_UNARMED_SPECIALIST_V))
            nBonus += g_pRules->RollDice(5, 4);
        else if (HasFeat(FEAT_UNARMED_SPECIALIST_IV))
            nBonus += g_pRules->RollDice(4, 4);
        else if (HasFeat(FEAT_UNARMED_SPECIALIST_III))
            nBonus += g_pRules->RollDice(3, 4);
        else if (HasFeat(FEAT_UNARMED_SPECIALIST_II))
            nBonus += g_pRules->RollDice(2, 4);
        else if (HasFeat(FEAT_UNARMED_SPECIALIST_I))
            nBonus += g_pRules->RollDice(1, 4);
    }
#endif
    return static_cast<char>(nBonus);
}

int CSWSCreatureStats::GetRangedAttackBonus(int a1, int a2, int a3)
{
    K2_UNUSED(a3);
    CSWSItem* pItem =
            static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    if (pItem != 0 &&
        g_pRules->GetBaseItem(pItem->m_baseItem)->m_bRangedWeapon == 0)
        return 0;
    int nBonus = 0;
    if (a2 == 1) {
        int nFx = 0;
        nBonus = m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_ATTACK, 0, nFx, 0, 0, 0, 0,
                                               CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, a1, 0)
                 + GetBaseAttackBonus();
    }
    CSWSItem* pOff =
            static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
    int nOff = 0;
    if (a1 != 0) {
        if (pItem != 0) {
            if (HasFeat(FEAT_ADVANCED_DOUBLE_WEAPON_FIGHTING) != 0)
                nOff = -2;
            else if (HasFeat(FEAT_DOUBLE_WEAPON_FIGHTING) != 0)
                nOff = -4;
            else if (HasFeat(FEAT_AMBIDEXTERITY) != 0)
                nOff = -6;
            else
                nOff = -10;
#if !KOTOR1
            if (HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_3) != 0)
                nOff |= 1;
            else if (HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_2) != 0) {
            }
#endif
        }
    } else {
        if (pOff != 0 &&
#ifdef K2_FIX_ENGINE_BUGS
            pItem != 0 &&
#endif
            g_pRules->GetBaseItem(pOff->m_baseItem)->m_nWeaponType != 0) {
            if (m_pOwner->GetRelativeWeaponSize(pItem) < 0)
                nOff = -4;
            else if (g_pRules->GetBaseItem(pItem->m_baseItem)->m_weaponWieldType ==
                     CSWBaseItem::WEAPON_WIELD_TWO_HANDED)
                nOff = -4;
            else
                nOff = -6;
            if (HasFeat(FEAT_ADVANCED_DOUBLE_WEAPON_FIGHTING) != 0)
                nOff += 4;
            else if (HasFeat(FEAT_DOUBLE_WEAPON_FIGHTING) != 0)
                nOff += 2;
#if !KOTOR1
            if (HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_3) != 0)
                nOff += 2;
            else if (HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_2) != 0)
                nOff += 2;
            else
                nOff += HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_1);
#endif
        }
    }
    int nDex = GetDEXMod(0);
    int bFocus = (GetWeaponFocus(pItem) != 0);
    int nTier = 0;
#if !KOTOR1  // K1 has no feat tier chain here.
    if (HasFeat(FEAT_TARGETING_10) != 0)
        nTier = 10;
    else if (HasFeat(FEAT_TARGETING_9) != 0)
        nTier = 9;
    else if (HasFeat(FEAT_TARGETING_8) != 0)
        nTier = 8;
    else if (HasFeat(FEAT_TARGETING_7) != 0)
        nTier = 7;
    else if (HasFeat(FEAT_TARGETING_6) != 0)
        nTier = 6;
    else if (HasFeat(FEAT_TARGETING_5) != 0)
        nTier = 5;
    else if (HasFeat(FEAT_TARGETING_4) != 0)
        nTier = 4;
    else if (HasFeat(FEAT_TARGETING_3) != 0)
        nTier = 3;
    else if (HasFeat(FEAT_TARGETING_2) != 0)
        nTier = 2;
    else
        nTier = HasFeat(FEAT_TARGETING_1);
#endif
    int nTotal = nOff + nBonus + nDex + bFocus + nTier;
    int nDual = 0;
    CSWSItem* pRight =
            static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    CSWSItem* pLeft =
            static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
    int bChain = 0;
    if (pRight != 0 && pLeft == 0) {
        CSWBaseItem* pBaseItem = pRight->GetBaseItem();
        if (pBaseItem != 0 &&
            (pBaseItem->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_ONE_HANDED ||
             pBaseItem->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_PISTOL))
            bChain = 1;
#if !KOTOR1
    } else if (pRight == 0 && pLeft == 0) {
        bChain = 1;
#endif
    }
    if (bChain != 0) {
        if (HasFeat(FEAT_MASTER_DUELING) != 0)
            nDual = 3;
        else if (HasFeat(FEAT_ADVANCED_DUELING) != 0)
            nDual = 2;
        else
            nDual = HasFeat(FEAT_DUELING);
    }
    return nTotal + nDual;
}

// Totals the melee attack bonus from weapon tier, focus, feats and the STR/DEX modifiers.
int CSWSCreatureStats::GetMeleeAttackBonus(int a1, int a2, int a3, int a4)
{
    CSWSItem* pItem =
            static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot((a4 != 0 || a1 == 0) ? CSWInventory::SLOT_RIGHTWEAPON : CSWInventory::SLOT_LEFTWEAPON));
    if (pItem != 0 &&
        g_pRules->GetBaseItem(pItem->m_baseItem)->m_bRangedWeapon != 0)
        return 0;
    int nBonus = 0;
    if (a2 == 1) {
        int nFx = 0;
        nBonus = m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_ATTACK, 0, nFx, 0, 0, 0, 0,
                                               CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, a1, 0)
                 + GetBaseAttackBonus();
    }
    int nStr;
    int nDex;
    if (GetWeaponFinesse(pItem) != 0) {
        nStr = 0;
        int nDexMod = m_nDEXMod;
        int bDebilitated = m_pOwner->IsDebilitated(1);
        nDex = (nDexMod < 0) ? nDexMod : (bDebilitated ? 0 : nDexMod);
    } else {
        nStr = static_cast<signed char>(m_nSTRMod);
        nDex = 0;
    }
    int bFocus = (GetWeaponFocus(pItem) != 0);
    int nTier = 0;
#if !KOTOR1  // K1 has no lightsaber tier.
    if (pItem != 0 && pItem->IsLightsaber()) {
        if (HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_LIGHTSABER_3) != 0)
            nTier = 3;
        else if (HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_LIGHTSABER_2) != 0)
            nTier = 2;
        else
            nTier = HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_LIGHTSABER_1);
    }
#endif
    int nTotal = nStr + nBonus + nDex + bFocus + nTier;
    if (a3 == 0) {
        CSWSItem* pOff =
                static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot((a4 == 0) ? CSWInventory::SLOT_LEFTWEAPON : CSWInventory::SLOT_RIGHTWEAPON));
        int nOff = 0;
        if (a1 != 0) {
            if (pOff != 0) {
                if (HasFeat(FEAT_ADVANCED_DOUBLE_WEAPON_FIGHTING) != 0)
                    nOff = -2;
                else if (HasFeat(FEAT_DOUBLE_WEAPON_FIGHTING) != 0)
                    nOff = -4;
                else if (HasFeat(FEAT_AMBIDEXTERITY) != 0)
                    nOff = -6;
                else
                    nOff = -10;
#if !KOTOR1
                if (HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_3) != 0)
                    nOff |= 1;
                else if (HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_2) != 0) {
                }
#endif
            }
        } else {
            if (pOff != 0 &&
                g_pRules->GetBaseItem(pOff->m_baseItem)->m_nWeaponType != 0) {
                if (m_pOwner->GetRelativeWeaponSize(pOff) == -1)
                    nOff = -4;
                else if (g_pRules->GetBaseItem(pOff->m_baseItem)->m_weaponWieldType ==
                         CSWBaseItem::WEAPON_WIELD_TWO_HANDED)
                    nOff = -4;
                else
                    nOff = -6;
                if (HasFeat(FEAT_ADVANCED_DOUBLE_WEAPON_FIGHTING) != 0)
                    nOff += 4;
                else if (HasFeat(FEAT_DOUBLE_WEAPON_FIGHTING) != 0)
                    nOff += 2;
#if !KOTOR1
                if (HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_3) != 0)
                    nOff += 2;
                else if (HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_2) != 0)
                    nOff += 2;
                else
                    nOff += HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_1);
#endif
            }
        }
        nTotal += nOff;
    }
    int nDual = 0;
    CSWSItem* pRight =
            static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    CSWSItem* pLeft =
            static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
    int bChain = 0;
    if (pRight != 0 && pLeft == 0) {
        CSWBaseItem* pBaseItem = pRight->GetBaseItem();
        if (pBaseItem != 0 &&
            (pBaseItem->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_ONE_HANDED ||
             pBaseItem->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_PISTOL))
            bChain = 1;
#if !KOTOR1
    } else if (pRight == 0 && pLeft == 0) {
        bChain = 1;
#endif
    }
    if (bChain != 0) {
        if (HasFeat(FEAT_MASTER_DUELING) != 0)
            nDual = 3;
        else if (HasFeat(FEAT_ADVANCED_DUELING) != 0)
            nDual = 2;
        else
            nDual = HasFeat(FEAT_DUELING);
    }
    return nTotal + nDual;
}

// The full attack-modifier roll-up against a target.
int CSWSCreatureStats::GetAttackModifierVersus(CSWSCreature* pTarget)
{
    unsigned char nCurAttack = m_pOwner->m_pCombatRound->m_currentAttack;
    CSWSCombatAttackData* pAttack = m_pOwner->m_pCombatRound->GetAttack(nCurAttack);
    int bRanged = pAttack->m_bRangedAttack;
    unsigned char nAttackType = pAttack->m_nWeaponAttackType;
    CSWSItem* pWeapon = m_pOwner->m_pCombatRound->GetCurrentAttackWeapon(nAttackType);
    if (pWeapon != 0)
        g_pRules->GetBaseItem(pWeapon->m_baseItem);

    int nBAB;
    if (nAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_OFFHAND)
    {
        nBAB = GetBaseAttackBonus();
        m_pOwner->m_pCombatRound->m_nOffHandTaken++;
    }
    else if (nAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_EXTRA ||
             nAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_UNARMED_EXTRA)
    {
        nBAB = GetBaseAttackBonus();
    }
    else
    {
        nBAB = GetBaseAttackBonus();
    }
    pAttack->m_attack.m_nBaseAttackBonus = static_cast<signed char>(nBAB);
    if (g_bEnableCombatDebugging != 0)
    {
        CExoString sLog =
            m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText;
        CExoString sBonus;
        sBonus.Format(const_cast<char*>(" + %d (Base Attack Bonus)"), nBAB);
        m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText = sLog + sBonus;
    }

    CSWSItem* pOffWeapon =
            static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
    if (pOffWeapon == 0)
    {
        pOffWeapon =
                static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
        if (pOffWeapon != 0)
        {
            CSWBaseItem* pOffBase =
                g_pRules->GetBaseItem(pOffWeapon->m_baseItem);
            if (pOffBase != 0 && pOffBase->m_weaponWieldType != CSWBaseItem::WEAPON_WIELD_TWO_HANDED)
                pOffWeapon = 0;
        }
    }

    int nOffhandPenalty = 0;
    signed char nMainPenalty = 0;
    unsigned char nDoubleWeapon = 0;
    unsigned char nOffhandByte = 0;
    unsigned char nOffhandFeat = 0;
    if (nAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_OFFHAND)
    {
        pAttack->m_attack.m_bOffhandAttack = 1;
        if (pOffWeapon != 0)
        {
            unsigned char nFeat = 0;
            int nOff;
            if (HasFeat(FEAT_ADVANCED_DOUBLE_WEAPON_FIGHTING) != 0)
            {
                nFeat = FEAT_ADVANCED_DOUBLE_WEAPON_FIGHTING;
                nOff = 8;
            }
            else if (HasFeat(FEAT_DOUBLE_WEAPON_FIGHTING) != 0)
            {
                nFeat = FEAT_DOUBLE_WEAPON_FIGHTING;
                nOff = 6;
            }
            else
            {
                if (HasFeat(FEAT_AMBIDEXTERITY) != 0)
                {
                    nFeat = FEAT_AMBIDEXTERITY;
                    nOff = 4;
                }
                else
                    nOff = 0;
            }
#if KOTOR1
            nOffhandFeat = nFeat;
#else
            if (HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_3) != 0)
            {
                nOff |= 1;
                nOffhandFeat = FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_3;
            }
            else if (HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_2) != 0)
            {
                nOffhandFeat = FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_2;
            }
            else
            {
                if (HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_1) != 0)
                    nFeat = FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_1;
                nOffhandFeat = nFeat;
            }
#endif
            nOffhandPenalty = nOff - 10;
            nMainPenalty = -10;
            nOffhandByte = static_cast<unsigned char>(nOff);
        }
    }
    else if (nAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_ONHAND)
    {
        pAttack->m_attack.m_bOffhandAttack = 0;
        if (pOffWeapon != 0 &&
            g_pRules->GetBaseItem(pOffWeapon->m_baseItem)->m_nWeaponType != 0)
        {
            int nBase;
            if (m_pOwner->GetRelativeWeaponSize(pOffWeapon) == -1 ||
                g_pRules->GetBaseItem(pOffWeapon->m_baseItem)
                        ->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_TWO_HANDED)
            {
                nBase = -4;
                nDoubleWeapon = 2;
            }
            else
            {
                nBase = -6;
            }
            int nOff;
            if (HasFeat(FEAT_ADVANCED_DOUBLE_WEAPON_FIGHTING) != 0)
            {
                nOffhandFeat = FEAT_ADVANCED_DOUBLE_WEAPON_FIGHTING;
                nOff = 4;
            }
            else
            {
                if (HasFeat(FEAT_DOUBLE_WEAPON_FIGHTING) != 0)
                {
                    nOffhandFeat = FEAT_DOUBLE_WEAPON_FIGHTING;
                    nOff = 2;
                }
                else
                    nOff = 0;
            }
#if !KOTOR1
            if (HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_3) != 0)
            {
                nOff += 2;
                nOffhandFeat = FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_3;
            }
            else if (HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_2) != 0)
            {
                nOff += 2;
                nOffhandFeat = FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_2;
            }
            else
            {
                int b9f = HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_1);
                if (b9f != 0)
                    nOffhandFeat = FEAT_SUPERIOR_WEAPON_FOCUS_TWO_WEAPON_1;
                nOff += b9f;
            }
#endif
            nOffhandPenalty = nBase + nOff;
            nMainPenalty = -6;
            nOffhandByte = static_cast<unsigned char>(nOff);
        }
    }
    pAttack->m_attack.m_nTwoWeaponPenalty = nMainPenalty;
    pAttack->m_attack.m_nLightOffhandBonus = nDoubleWeapon;
    pAttack->m_attack.m_nTwoWeaponFeatBonus = nOffhandByte;
    pAttack->m_attack.m_nTwoWeaponFeat = nOffhandFeat;
    if (g_bEnableCombatDebugging != 0 && nOffhandPenalty != 0)
    {
        CExoString sLog =
            m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText;
        CExoString sBonus;
        sBonus.Format(const_cast<char*>(" + %d (Offhand Attack Penalty)"), nOffhandPenalty);
        m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText = sLog + sBonus;
    }

    int nSpecial = ResolveSpecialAttackAttackBonus(pTarget);
    pAttack->m_attack.m_nSpecialAttackBonus = static_cast<signed char>(nSpecial);
    if (g_bEnableCombatDebugging != 0 && nSpecial != 0)
    {
        CExoString sLog =
            m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText;
        CExoString sBonus;
        sBonus.Format(const_cast<char*>(" + %d (Special Attack To Hit Modifier)"), nSpecial);
        m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText = sLog + sBonus;
    }

    int nDueling = 0;
    unsigned char nDuelFeat = 0;
    CSWSItem* pRight =
            static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    CSWSItem* pLeft =
            static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
    int bChain = 0;
    if (pRight != 0 && pLeft == 0)
    {
        CSWBaseItem* pBaseItem = pRight->GetBaseItem();
        if (pBaseItem != 0 &&
            (pBaseItem->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_ONE_HANDED ||
             pBaseItem->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_PISTOL))
            bChain = 1;
    }
#if !KOTOR1
    else if (pRight == 0 && pLeft == 0)
    {
        bChain = 1;
    }
#endif
    if (bChain != 0)
    {
        if (HasFeat(FEAT_MASTER_DUELING) != 0)
        {
            nDueling = 3;
            nDuelFeat = FEAT_MASTER_DUELING;
        }
        else if (HasFeat(FEAT_ADVANCED_DUELING) != 0)
        {
            nDueling = 2;
            nDuelFeat = FEAT_ADVANCED_DUELING;
        }
        else
        {
            nDueling = HasFeat(FEAT_DUELING);
            if (nDueling != 0)
                nDuelFeat = FEAT_DUELING;
        }
    }
    pAttack->m_attack.m_nDuelingBonus = static_cast<unsigned char>(nDueling);
    pAttack->m_attack.m_nDuelingFeat = nDuelFeat;
    if (g_bEnableCombatDebugging != 0 && nSpecial != 0)
    {
        CExoString sLog =
            m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText;
        CExoString sBonus;
        sBonus.Format(const_cast<char*>(" + %d (Dueling To Hit Modifier)"), nDueling);
        m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText = sLog + sBonus;
    }

    int nGoodAim = 0;
    if (pTarget != 0 && bRanged != 0)
    {
        float fDx = m_pOwner->m_position.x - pTarget->m_position.x;
        float fDy = m_pOwner->m_position.y - pTarget->m_position.y;
        float fDz = m_pOwner->m_position.z - pTarget->m_position.z;
        if (fDx * fDx + fDy * fDy + fDz * fDz <= 25.0f)
        {
#if KOTOR1
// K1: a flat +10 inside 5 m, no feats.
            nGoodAim = 10;
#else
            if (m_pOwner->m_pStats->HasFeat(FEAT_CLOSE_COMBAT) != 0)
                nGoodAim = 1;
            if (m_pOwner->m_pStats->HasFeat(FEAT_IMPROVED_CLOSE_COMBAT) != 0)
                nGoodAim += 1;
#endif
        }
    }
    pAttack->m_attack.m_nGoodAimBonus = static_cast<unsigned char>(nGoodAim);
    if (g_bEnableCombatDebugging != 0 && nGoodAim != 0)
    {
        CExoString sLog =
            m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText;
        CExoString sBonus;
        sBonus.Format(const_cast<char*>(" + %d (Good Aim)"), nGoodAim);
        m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText = sLog + sBonus;
    }

    int nOffTraining = 0;
    if (pTarget != 0 && bRanged == 0)
    {
        CSWSItem* pTWeapon =
                static_cast<CSWSItem*>(pTarget->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
#if KOTOR1
// K1: a flat +10 against a ranged-weapon wielder, no feats.
        if (pTWeapon != 0 && pTWeapon->GetBaseItem()->m_bRangedWeapon != 0)
            nOffTraining = 10;
#else
        if (pTWeapon != 0 && pTWeapon->GetBaseItem()->m_bRangedWeapon != 0)
        {
            if (pTarget->m_pStats->HasFeat(FEAT_CLOSE_COMBAT) != 0)
                nOffTraining = 4;
            else
                nOffTraining = 6;
            if (pTarget->m_pStats->HasFeat(FEAT_IMPROVED_CLOSE_COMBAT) != 0)
                nOffTraining -= 2;
        }
#endif
    }
    pAttack->m_attack.m_nOffensiveTrainingBonus = static_cast<unsigned char>(nOffTraining);
    if (g_bEnableCombatDebugging != 0 && nOffTraining != 0)
    {
        CExoString sLog =
            m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText;
        CExoString sBonus;
        sBonus.Format(const_cast<char*>(" + %d (Offensive Training Bonus (vs Race))"), nOffTraining);
        m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText = sLog + sBonus;
    }

#if !KOTOR1
    int nBattleMed = 0;
    if (m_pOwner->m_bInParty != 0 &&
        (HasFeat(FEAT_DUAL_STRIKE) != 0 || HasFeat(FEAT_IMPROVED_DUAL_STRIKE) != 0 || HasFeat(FEAT_MASTER_DUAL_STRIKE) != 0))
    {
        int nCount = g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount;
        for (int i = 0; i < nCount; i++)
        {
            unsigned long long nServerId =
                g_pAppManager->m_pServerExoApp->ClientToServerObjectId(
                    g_pAppManager->m_pClientExoApp->GetSWParty()->m_members[i].id);
            if (nServerId == m_pOwner->m_objectId)
                continue;
            CSWSCreature* pMember =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nServerId);
            if (pMember != 0 && pTarget != 0 &&
                pMember->m_attemptedAttackTargetId == pTarget->m_objectId)
            {
                if (HasFeat(FEAT_DUAL_STRIKE) != 0)
                    nBattleMed += 2;
                if (HasFeat(FEAT_IMPROVED_DUAL_STRIKE) != 0)
                    nBattleMed += 2;
                if (HasFeat(FEAT_MASTER_DUAL_STRIKE) != 0)
                    nBattleMed += 2;
                break;
            }
        }
    }
    pAttack->m_attack.m_nBattleMeditationBonus = static_cast<unsigned char>(nBattleMed);
#endif

    int nDexUsed = 0;
    int nStrUsed = 0;
    if (bRanged != 0)
    {
        nDexUsed = GetDEXMod(0);
    }
    else if (GetWeaponFinesse(pWeapon) == 1)
    {
        nDexUsed = GetDEXMod(0);
    }
    else
    {
        nStrUsed = static_cast<signed char>(m_nSTRMod);
    }
// The current attack index is read afresh per store.
    m_pOwner->m_pCombatRound->GetAttack(m_pOwner->m_pCombatRound->m_currentAttack)
        ->m_attack.m_nStrAttackMod = static_cast<signed char>(nStrUsed);
    m_pOwner->m_pCombatRound->GetAttack(m_pOwner->m_pCombatRound->m_currentAttack)
        ->m_attack.m_nDexAttackMod = static_cast<signed char>(nDexUsed);
    if (g_bEnableCombatDebugging != 0)
    {
        CExoString sLog =
            m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText;
        CExoString sBonus;
        if (nDexUsed != 0)
        {
            sBonus.Format(const_cast<char*>(" + %d (Dex Modifier)"), nDexUsed);
            m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText = sLog + sBonus;
            sLog = m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText;
        }
        if (nStrUsed != 0)
        {
            sBonus.Format(const_cast<char*>(" + %d (Str Modifier)"), nStrUsed);
            m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText = sLog + sBonus;
            sLog = m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText;
        }
    }

    int bFocus = (GetWeaponFocus(pWeapon) != 0);
    pAttack->m_attack.m_nWeaponFocusBonus = static_cast<unsigned char>(bFocus);
#if KOTOR1
// K1: weapon focus only; no lightsaber focus, targeting, Crush Opposition, Inspire Followers,
// saber-form or PC-charisma terms.
    if (g_bEnableCombatDebugging != 0 && bFocus != 0)
    {
        CExoString sLog =
            m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText;
        CExoString sBonus;
        sBonus.Format(const_cast<char*>(" + %d (Weapon Focus Bonus)"), bFocus);
        m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText = sLog + sBonus;
    }
#else
    int nForceFocus = 0;
    if (pWeapon != 0 && pWeapon->IsLightsaber() != 0)
    {
        if (HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_LIGHTSABER_3) != 0)
            nForceFocus = 3;
        else if (HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_LIGHTSABER_2) != 0)
            nForceFocus = 2;
        else
            nForceFocus = HasFeat(FEAT_SUPERIOR_WEAPON_FOCUS_LIGHTSABER_1);
    }
    pAttack->m_attack.m_nWeaponFocusBonus += static_cast<unsigned char>(nForceFocus);

    int nTargeting = 0;
    unsigned char nTargetFeat = 0;
    if (bRanged != 0)
    {
        if (HasFeat(FEAT_TARGETING_10) != 0)
        {
            nTargetFeat = FEAT_TARGETING_10;
            nTargeting = 10;
        }
        else if (HasFeat(FEAT_TARGETING_9) != 0)
        {
            nTargetFeat = FEAT_TARGETING_9;
            nTargeting = 9;
        }
        else if (HasFeat(FEAT_TARGETING_8) != 0)
        {
            nTargetFeat = FEAT_TARGETING_8;
            nTargeting = 8;
        }
        else if (HasFeat(FEAT_TARGETING_7) != 0)
        {
            nTargetFeat = FEAT_TARGETING_7;
            nTargeting = 7;
        }
        else if (HasFeat(FEAT_TARGETING_6) != 0)
        {
            nTargetFeat = FEAT_TARGETING_6;
            nTargeting = 6;
        }
        else if (HasFeat(FEAT_TARGETING_5) != 0)
        {
            nTargetFeat = FEAT_TARGETING_5;
            nTargeting = 5;
        }
        else if (HasFeat(FEAT_TARGETING_4) != 0)
        {
            nTargetFeat = FEAT_TARGETING_4;
            nTargeting = 4;
        }
        else if (HasFeat(FEAT_TARGETING_3) != 0)
        {
            nTargetFeat = FEAT_TARGETING_3;
            nTargeting = 3;
        }
        else if (HasFeat(FEAT_TARGETING_2) != 0)
        {
            nTargetFeat = FEAT_TARGETING_2;
            nTargeting = 2;
        }
        else
        {
            nTargeting = HasFeat(FEAT_TARGETING_1);
            if (nTargeting != 0)
                nTargetFeat = FEAT_TARGETING_1;
        }
    }
    pAttack->m_attack.m_nTargetingFeat = nTargetFeat;
    pAttack->m_attack.m_nTargetingBonus += static_cast<unsigned char>(nTargeting);
    if (g_bEnableCombatDebugging != 0 && (nForceFocus | bFocus) != 0)
    {
        CExoString sLog =
            m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText;
        CExoString sBonus;
        sBonus.Format(const_cast<char*>(" + %d (Weapon Focus Bonus)"), nForceFocus + bFocus);
        m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText = sLog + sBonus;
    }
    if (g_bEnableCombatDebugging != 0 && nTargeting != 0)
    {
        CExoString sLog =
            m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText;
        CExoString sBonus;
        sBonus.Format(const_cast<char*>(" + %d (Targeting Bonus)"), nTargeting);
        m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText = sLog + sBonus;
    }

    int nCrushSpell = 0;
    int nCrushPenalty = 0;
    if (pTarget != 0 && pTarget->m_bInParty != 0)
    {
        CSWSCreatureStats* pTStats = pTarget->m_pStats;
        if (pTStats->m_bIsPC != 0 && pTStats->HasSpell(0, FORCE_POWER_CRUSH_OPPOSITION_VI, 0) != 0 &&
            pTStats->m_nAlignment < 41)
        {
            nCrushSpell = FORCE_POWER_CRUSH_OPPOSITION_VI;
            nCrushPenalty = -2;
        }
        else
        {
            if (pTStats->m_bIsPC != 0 && pTStats->HasSpell(0, FORCE_POWER_CRUSH_OPPOSITION_III, 0) != 0 &&
                pTStats->m_nAlignment < 41)
            {
                nCrushSpell = FORCE_POWER_CRUSH_OPPOSITION_III;
                nCrushPenalty = -1;
            }
            for (int i = 0; i < CSWPartyTable::NUM_NPCS; i++)
            {
                if (g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i) == 0)
                    continue;
                if (g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsMember(i) == 0)
                    continue;
                unsigned long long nNpcId =
                    g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1);
                if (nNpcId == OBJECT_INVALID)
                    continue;
                CSWSCreature* pNPC =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nNpcId);
                if (pNPC == 0)
                    continue;
                CSWSCreatureStats* pNStats = pNPC->m_pStats;
                if (pNStats->HasSpell(0, FORCE_POWER_CRUSH_OPPOSITION_VI,
                                      0) != 0 && pNStats->m_nAlignment < 41)
                {
                    nCrushSpell = FORCE_POWER_CRUSH_OPPOSITION_VI;
                    nCrushPenalty = -2;
                    break;
                }
                if (pNStats->HasSpell(0, FORCE_POWER_CRUSH_OPPOSITION_III,
                                      0) != 0 && pNStats->m_nAlignment < 41)
                {
                    nCrushSpell = FORCE_POWER_CRUSH_OPPOSITION_III;
                    nCrushPenalty = -1;
                }
            }
        }
    }
    pAttack->m_attack.m_nCrushOppositionSpell += static_cast<unsigned char>(nCrushSpell);
    pAttack->m_attack.m_nCrushOppositionPenalty += static_cast<unsigned char>(nCrushPenalty);
    if (g_bEnableCombatDebugging != 0 && nCrushPenalty != 0)
    {
        CExoString sLog =
            m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText;
        CExoString sBonus;
        sBonus.Format(const_cast<char*>(" + %d (Crush Opposition Penalty)"), nCrushPenalty);
        m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText = sLog + sBonus;
    }

    int nInspireSpell = 0;
    int nInspireBonus = 0;
    if (m_pOwner->m_bInParty != 0)
    {
        if (m_bIsPC != 0 && HasSpell(0, FORCE_POWER_INSPIRE_FOLLOWERS_VI, 0) != 0 && m_nAlignment > 59)
        {
            nInspireSpell = FORCE_POWER_INSPIRE_FOLLOWERS_VI;
            nInspireBonus = 2;
        }
        else
        {
            if (m_bIsPC != 0 && HasSpell(0, FORCE_POWER_INSPIRE_FOLLOWERS_III, 0) != 0 && m_nAlignment > 59)
            {
                nInspireSpell = FORCE_POWER_INSPIRE_FOLLOWERS_III;
                nInspireBonus = 1;
            }
            for (int i = 0; i < CSWPartyTable::NUM_NPCS; i++)
            {
                if (g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsNPCAvailable(i) == 0)
                    continue;
                if (g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsMember(i) == 0)
                    continue;
                unsigned long long nNpcId =
                    g_pAppManager->m_pServerExoApp->GetPartyTable()->GetNPCObject(i, 0, 1);
                if (nNpcId == OBJECT_INVALID)
                    continue;
                CSWSCreature* pNPC =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nNpcId);
                if (pNPC == 0)
                    continue;
                CSWSCreatureStats* pNStats = pNPC->m_pStats;
                if (pNStats->HasSpell(0, FORCE_POWER_INSPIRE_FOLLOWERS_VI,
                                      0) != 0 && pNStats->m_nAlignment > 59)
                {
                    nInspireSpell = FORCE_POWER_INSPIRE_FOLLOWERS_VI;
                    nInspireBonus = 2;
                    break;
                }
                if (pNStats->HasSpell(0, FORCE_POWER_INSPIRE_FOLLOWERS_III,
                                      0) != 0 && pNStats->m_nAlignment > 59)
                {
                    nInspireSpell = FORCE_POWER_INSPIRE_FOLLOWERS_III;
                    nInspireBonus = 1;
                }
            }
        }
    }
    pAttack->m_attack.m_nInspireFollowersSpell = static_cast<unsigned char>(nInspireSpell);
    pAttack->m_attack.m_nInspireFollowersBonus += static_cast<unsigned char>(nInspireBonus);
    if (g_bEnableCombatDebugging != 0 && nInspireBonus != 0)
    {
        CExoString sLog =
            m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText;
        CExoString sBonus;
        sBonus.Format(const_cast<char*>(" + %d (Inspire Followers Bonus)"), nInspireBonus);
        m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText = sLog + sBonus;
    }

    int nJediFeat = 0;
    int nJediDefense = 0;
    if (m_pOwner->IsWieldingLightsaber() != 0)
    {
        switch (m_pOwner->m_stanceId)
        {
        case FORM_SABER_I_SHII_CHO:
        case FORM_SABER_VI_NIMAN:
            nJediDefense = 1;
            nJediFeat = m_pOwner->m_stanceId;
            break;
        case FORM_SABER_V_SHIEN:
            nJediDefense = 2;
            nJediFeat = m_pOwner->m_stanceId;
            break;
        case FORM_SABER_II_MAKASHI:
            if (pTarget != 0 && pTarget->IsWieldingLightsaber() != 0)
            {
                nJediDefense = 3;
                nJediFeat = FORM_SABER_II_MAKASHI;
            }
            break;
        case FORM_SABER_III_SORESU:
        case FORM_SABER_IV_ATARU:
            break;
        }
    }
    pAttack->m_attack.m_nSaberFormId = nJediFeat;
    pAttack->m_attack.m_nSaberFormBonus += static_cast<unsigned char>(nJediDefense);
    if (g_bEnableCombatDebugging != 0 && nJediDefense != 0)
    {
        CExoString sLog =
            m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText;
        CExoString sBonus;
        sBonus.Format(const_cast<char*>(" + %d"), nJediDefense);
        m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText = sLog + sBonus;
    }
#endif

    int nFx = 0;
    CSWSObject* pVersus = 0;
    if (pTarget != 0)
        pVersus = pTarget->AsSWSObject();
    int nEffectBonus =
        m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_ATTACK, pVersus, nFx, 0, 0, 0, 0,
                                      CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, 0, 0);
    pAttack->m_attack.m_nEffectAttackBonus = static_cast<signed char>(nEffectBonus);
    if (g_bEnableCombatDebugging != 0 && nEffectBonus != 0)
    {
        CExoString sLog =
            m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText;
        CExoString sBonus;
        sBonus.Format(const_cast<char*>(" + %d (Effect Attack Bonus)"), nEffectBonus);
        m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText = sLog + sBonus;
    }

#if KOTOR1
    return nOffhandPenalty + nBAB + nSpecial + nDueling + nGoodAim + nOffTraining + nDexUsed +
           nStrUsed + bFocus + nEffectBonus;
#else
    int nChaBonus = 0;
    if (m_pOwner->m_bInParty != 0 && m_bIsPC == 0)
    {
        CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
        int nCount = pParty->m_nCharacterCount;
        for (int i = 0; i < nCount; i++)
        {
            CSWSCreatureStats* pMStats = pParty->GetCharacter(i)->GetServerCreature()->m_pStats;
            if (pMStats->m_bIsPC != 0)
                nChaBonus = static_cast<signed char>(pMStats->m_nCHAMod);
        }
    }
    pAttack->m_attack.m_nPCCharismaBonus += static_cast<unsigned char>(nChaBonus);
    if (g_bEnableCombatDebugging != 0 && nChaBonus != 0)
    {
        CExoString sLog =
            m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText;
        CExoString sBonus;
        sBonus.Format(const_cast<char*>(" + %d (PC Charisma Attack Bonus)"), nChaBonus);
        m_pOwner->m_pCombatRound->GetAttack(nCurAttack)->m_sAttackDebugText = sLog + sBonus;
    }

    return nOffhandPenalty + nBAB + nSpecial + nDueling + nGoodAim + nOffTraining +
           nBattleMed + nDexUsed + nStrUsed + bFocus + nForceFocus + nTargeting +
           nCrushPenalty + nInspireBonus + nJediDefense + nEffectBonus + nChaBonus;
#endif
}

// Applies the DEX-over-STR melee rule, under which a finesse-eligible weapon uses the DEX modifier.
int CSWSCreatureStats::GetWeaponFinesse(CSWSItem* pItem)
{
    if (pItem == NULL)
        return 0;
#if KOTOR1
// K1: every lightsaber, and nothing else, finesses; no feat is required.
    if (pItem->IsLightsaber() == 1 && GetDEXMod(0) > static_cast<signed char>(m_nSTRMod))
        return 1;
    return 0;
#else
    if (GetDEXMod(0) <= static_cast<signed char>(m_nSTRMod))
        return 0;
    if (pItem->IsLightsaber() && HasFeat(FEAT_FINESSE_LIGHTSABERS))
        return 1;
    if (pItem->GetBaseItem()->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_STUN_BATON ||
        pItem->GetBaseItem()->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_ONE_HANDED ||
        pItem->GetBaseItem()->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_TWO_HANDED)
    {
        if (HasFeat(FEAT_FINESSE_MELEE_WEAPONS))
            return 1;
    }
    return 0;
#endif
}

// Sorted-list immunity scan; it stops once the subtype passes the immunity sentinel.
int CSWSCreatureStats::GetEffectImmunity(unsigned char nImmunity, CSWSCreature* pCreature)
{
    int nAlignment;
    int nRace;
    if (pCreature != NULL)
    {
        short nAlignValue = pCreature->m_pStats->m_nAlignment;
        nRace = pCreature->m_pStats->m_nRace;
        nAlignment = (nAlignValue < 41) ? ALIGNMENT_DARK_SIDE
                     : ((nAlignValue > 59) ? ALIGNMENT_LIGHT_SIDE : ALIGNMENT_NEUTRAL);
    }
    else
    {
        nAlignment = -1;
        nRace = g_pRules->m_nNumRaces;
    }
    CSWSCreature* pOwner = m_pOwner;
    for (int i = m_iFirstImmunity; i < pOwner->m_effects.m_count; i++)
    {
        CGameEffect* pEffect = pOwner->m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_IMMUNITY)
            break;
        int nType = pEffect->GetInteger(0);
        int nSubType = pEffect->GetInteger(1);
        int nEffectAlign = pEffect->GetInteger(2);
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_IMMUNITY &&
            (nType == nImmunity || nType == CSWSEffectListHandler::IMMUNITY_TYPE_ALL) &&
            (nSubType == nRace || nSubType == g_pRules->m_nNumRaces) &&
            (nEffectAlign == ALIGNMENT_ALL || nEffectAlign == nAlignment))
            return 1;
    }
    return 0;
}

// Whether the creature is immune to the effect chain hanging off pEffect.
int CSWSCreatureStats::GetEffectLinkImmunity(CGameEffect* pEffect)
{
    unsigned short nSubType = pEffect->m_subType;
    if (nSubType != CSWSEffectListHandler::EFFECT_LINK &&
        nSubType != CSWSEffectListHandler::EFFECT_VISUALEFFECT)
    {
        int nRow = EffectId2Effect2DAId(nSubType, pEffect->GetInteger(0));
        if (nRow != 0)
        {
            int nImmunity = 0;
            int i = 1;
            do
            {
                int nId = i - 1;
                if (nId >= CSWSEffectListHandler::IMMUNITY_TYPE_MAX)
                    break;
                int nGranted;
                g_pRules->m_p2DArrays->m_pGameEffects->GetINTEntry(nRow, i, &nGranted);
                if (nGranted != 0)
                    nImmunity = GetEffectImmunity(static_cast<unsigned char>(nId), 0);
                i++;
            } while (nImmunity == 0);
            if (nImmunity != 0)
            {
                unsigned short nStrRef;
                if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_MIND_AFFECTING)
                    nStrRef = FEEDBACK_IMMUNE_MIND_EFFECTS;
                else if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_POISON)
                    nStrRef = FEEDBACK_IMMUNE_POISON;
                else if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_DISEASE)
                    nStrRef = FEEDBACK_IMMUNE_DISEASE;
                else if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_FEAR)
                    nStrRef = FEEDBACK_IMMUNE_FEAR;
                else if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_PARALYSIS)
                    nStrRef = FEEDBACK_IMMUNE_PARALYSIS;
                else if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_BLINDNESS)
                    nStrRef = FEEDBACK_IMMUNE_BLINDNESS;
                else if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_DEAFNESS)
                    nStrRef = FEEDBACK_IMMUNE_DEAFNESS;
                else if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_SLOW)
                    nStrRef = FEEDBACK_IMMUNE_SLOW;
                else if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_ENTANGLE)
                    nStrRef = FEEDBACK_IMMUNE_ENTANGLE;
                else if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_SILENCE)
                    nStrRef = FEEDBACK_IMMUNE_SILENCE;
                else if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_STUN)
                    nStrRef = FEEDBACK_IMMUNE_STUN;
                else if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_SLEEP)
                    nStrRef = FEEDBACK_IMMUNE_SLEEP;
                else if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_CONFUSED)
                    nStrRef = FEEDBACK_IMMUNE_CONFUSION;
                else if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_CURSED)
                    nStrRef = FEEDBACK_IMMUNE_CURSE;
                else if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_DAZED)
                    nStrRef = FEEDBACK_IMMUNE_DAZE;
                else if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_KNOCKDOWN)
                    nStrRef = FEEDBACK_IMMUNE_KNOCKDOWN;
                else if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_NEGATIVE_LEVEL)
                    nStrRef = FEEDBACK_IMMUNE_NEGATIVE_LEVELS;
                else if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_CRITICAL_HITS)
                    nStrRef = FEEDBACK_IMMUNE_CRITICAL_HITS;
                else if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_DEATH_MAGIC)
                    nStrRef = FEEDBACK_IMMUNE_DEATH;
#if !KOTOR1
                else if (i - 2 == CSWSEffectListHandler::IMMUNITY_TYPE_DROID_CONFUSED)
                    nStrRef = FEEDBACK_IMMUNE_CONFUSION;
#endif
                else
                    return nImmunity;

                CSWSCreature* pCreator =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
                CSWCCMessageData* pData = new CSWCCMessageData();
                pData->SetObjectID(0, m_pOwner->m_objectId);
                if (pCreator != 0)
                {
                    CSWCCMessageData* pCreatorData = new CSWCCMessageData();
                    pData->CopyTo(pCreatorData);
                    pCreator->SendFeedbackMessage(nStrRef, pCreatorData);
                }
                m_pOwner->SendFeedbackMessage(nStrRef, pData);
                return nImmunity;
            }
        }
    }

    int nResult = 0;
    if (pEffect->m_pLinkedEffect1 != 0)
        nResult = GetEffectLinkImmunity(pEffect->m_pLinkedEffect1);
    if (nResult != 0)
        return nResult;
    if (pEffect->m_pLinkedEffect2 != 0)
        nResult = GetEffectLinkImmunity(pEffect->m_pLinkedEffect2);
    return nResult;
}

int CSWSCreatureStats::HasFeat(unsigned short nFeat)
{
#if KOTOR1
// K1: counts every match in each list instead of stopping at the first.
    int nFound = 0;
    int64_t nCount = m_feats.m_count;
    if (nCount > 0) {
        unsigned short* pFeat = m_feats.m_pArray;
        for (int64_t i = 0; i < nCount; i++)
            nFound += (pFeat[i] == nFeat);
    }
    if (nFound == 0) {
        nCount = m_bonusFeats.m_count;
        if (nCount > 0) {
            unsigned short* pFeat = m_bonusFeats.m_pArray;
            for (int64_t i = 0; i < nCount; i++)
                nFound += (pFeat[i] == nFeat);
        }
        if (nFound == 0)
            return 0;
    }
    return 1;
#else
    if (m_feats.ContainsFast(nFeat) || m_bonusFeats.ContainsFast(nFeat))
        return 1;
    return 0;
#endif
}

// Spends one use of nFeat. Every entry for the feat is bumped; the name reads backwards,
// because the entry counts uses spent.
void CSWSCreatureStats::DecrementFeatRemainingUses(unsigned short nFeat)
{
    for (unsigned short i = 0; i < m_featRemainingUses.m_count; i++)
    {
        CSWSStats_FeatRemainingUses* pEntry = m_featRemainingUses.m_pArray[i];
        if (pEntry->m_nFeat == nFeat)
            pEntry->m_nUsesUsed++;
    }
}

// Default-constructs a starting creature's stats.
CSWSCreatureStats::CSWSCreatureStats(CSWSCreature* pOwner) : m_pOwner(NULL)
{
    m_levelStats.Add(new CSWLevelStats());
    m_nStartingPackage = 0;
    m_numClasses = 1;
    m_conversation = const_cast<char*>("");
    m_nReserved2 = 0;
    m_nGender = 0;
    m_nAge = 0;
    m_nInterruptable = 1;
    m_bIsPC = 1;
    m_nReserved1 = 0;
    m_nRace = CSWRace::CRACE_HUMAN;
    m_sSubrace = "";
    m_nSubraceIndex = 0;
    SetClassLevel(0, 1);
    SetClass(0, 0);
    m_numClasses = 2;
    CSWSCreatureStats_ClassInfo* pSlot1 = &m_classInfo[1];
    pSlot1->m_level = 0;
#if !KOTOR1
    if (m_pOwner != 0 && m_pOwner->m_bPlayerMadeCharacter != 0)
    {
        unsigned char nLevel = GetLevel(0);
        if (nLevel >= 30)
            Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_LEVEL_30);
    }
#endif
    if (1 < m_numClasses)
        pSlot1->m_class = CLASS_TYPE_INVALID;
    m_numClasses = 1;
    m_nSkillPoints = 0;
    m_nArmorCheckPenalty = 0;
    m_nReserved6 = 0;
    m_nExperience = 0;
    m_nAlignment = 0;
#if !KOTOR1
    m_nBaseNPCAlignment = -1;
    m_nFuryDamageBonus = -1;
#endif
    m_nReserved3 = 0;
    m_nReserved4 = 20;
    m_pOwner = pOwner;
    m_nReserved5 = 0;
#if !KOTOR1
    m_nVideoEffectType = 0;
#endif
    memset(&m_nACNaturalBase, 0,
           offsetof(CSWSCreatureStats, _pad154) - offsetof(CSWSCreatureStats, m_nACNaturalBase));
    memset(&m_iFirstTempHitPoints, 0,
           offsetof(CSWSCreatureStats, m_aiState) - offsetof(CSWSCreatureStats, m_iFirstTempHitPoints));
    memset(&m_iFirstEffectIcon, 0,
           offsetof(CSWSCreatureStats, m_iFirstLimitMovementSpeed) -
               offsetof(CSWSCreatureStats, m_iFirstEffectIcon));
    m_pCombatInformation = new CCombatInformation();
    m_aiState = 0;
    m_nMovementRate = 0;
    m_fRunRate = 0.0f;
    m_fWalkRate = 0.0f;
    m_pSkillRanks = new char[g_pRules->m_nNumSkills];
    memset(m_pSkillRanks, 0, g_pRules->m_nNumSkills);  // idiom-exempt: dynamic byte count == the new[] alloc above, not a struct copy
    m_nFortSaveBonus = 0;
    m_nWillSaveBonus = 0;
    m_nReflexSaveBonus = 0;
    memset(&m_nSTRBase, 0, offsetof(CSWSCreatureStats, m_nACNaturalBase) - offsetof(CSWSCreatureStats, m_nSTRBase));
    m_nArcaneSpellFailure = 0;
    m_nReserved7 = 0;
    m_nReserved8 = 0;
    m_forceResistance = 0;
    m_forceResistancePenalty = 0;
    m_portrait = const_cast<char*>("");
    m_bDuplicatingHead = -1;
    m_nUseBackupHead = 0;
    m_nColorSkin = 0;
    m_nColorHair = 0;
    m_nColorTattoo1 = 0;
    m_nColorTattoo2 = 0;
    m_nAppearanceType = 0;
    m_nPhenotype = 0;
    m_nAppearanceHead = 0;
    m_bTail = 0;
    m_bWings = 0;
    m_factionId = -1;
#if !KOTOR1
    m_fBlindSpot = 120.0f;
#endif
    m_fChallengeRating = 0.0f;
    m_pSpellLikeAbilityList = new CExoArrayList<CSWSStats_SpellLikeAbility>();
    m_nBaseForcePoints = 0;
    m_nCurrentForcePoints = 0;
    m_nTemporaryForcePoints = 0;
    m_hpAccumulator = 0.0f;
    m_fpAccumulator = 0.0f;
}

CSWSCreatureStats::~CSWSCreatureStats()
{
    if (m_pSkillRanks)
    {
        delete[] m_pSkillRanks;
        m_pSkillRanks = NULL;
    }

    ClearFeats();

    for (int i = m_levelStats.m_count - 1; i >= 0; i--)
    {
        CSWLevelStats* pLevelStats = m_levelStats.m_pArray[i];
        m_levelStats.DelIndex(i);
        delete pLevelStats;
    }

    if (m_pSpellLikeAbilityList)
    {
        delete m_pSpellLikeAbilityList;
        m_pSpellLikeAbilityList = NULL;
    }

// The guard matters, since the re-zeroing store exists only on the non-null path.
    if (m_pCombatInformation)
    {
        delete m_pCombatInformation;
        m_pCombatInformation = NULL;
    }
}

// m_feats holds values, so it clears; m_featRemainingUses holds owned pointers, so it deletes
// each entry.
void CSWSCreatureStats::ClearFeats()
{
    m_feats.SetSize(0);

    while (m_featRemainingUses.m_count != 0)
    {
        int nLast = m_featRemainingUses.m_count - 1;
        CSWSStats_FeatRemainingUses* pEntry = m_featRemainingUses.m_pArray[nLast];
        // inlined CExoArrayList<CFeatUseListEntry*>::DelIndex(m_count - 1)
        m_featRemainingUses.m_count = nLast;
        delete pEntry;
    }
}


unsigned char CSWSCreatureStats::GetSpellLikeAbilityCasterLevel(unsigned int nSpellId)
{
    for (int i = 0; i < m_pSpellLikeAbilityList->m_count; i++)
    {
        if (m_pSpellLikeAbilityList->m_pArray[i].m_nSpellId == nSpellId)
            return m_pSpellLikeAbilityList->m_pArray[i].m_nCasterLevel;
    }
    return 0xff;
}

int CSWSCreatureStats::GetIsInSpellLikeAbilityList(unsigned int nSpellId)
{
    for (int i = 0; i < m_pSpellLikeAbilityList->m_count; i++)
    {
        if (m_pSpellLikeAbilityList->m_pArray[i].m_nSpellId == nSpellId &&
            m_pSpellLikeAbilityList->m_pArray[i].m_nReady == 1)
            return 1;
    }
    return 0;
}

// Whether nPower is an item-granted property (type 0xa, the power id in m_subType).
int CSWSCreatureStats::GetHasPowerFromItem(int nPower)
{
    if (m_pOwner->m_pInventory != 0)
    {
        for (int slot = 0; slot < NUM_INVENTORY_SLOTS; slot++)
        {
            CSWSItem* pItem = static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(1 << slot));
            if (pItem != 0 && pItem->GetPropertyByTypeExists(ITEM_PROPERTY_ACTIVATE_ITEM, 0) &&
                pItem->m_nActiveProperties >= 1)
            {
                for (int j = 0; j < pItem->m_nActiveProperties; j++)
                {
                    if (pItem->GetActiveProperty(j)->m_type == ITEM_PROPERTY_ACTIVATE_ITEM &&
                        pItem->GetActiveProperty(j)->m_bUseableByte != 0)
                    {
                        unsigned char nSlot = pItem->GetActiveProperty(j)->m_nUpgradeType;
                        if ((nSlot == CSWItemProperty::UPGRADE_TYPE_NONE || (pItem->m_nUpgrades & (1 << nSlot)) != 0) &&
                            pItem->GetActiveProperty(j)->m_subType == nPower)
                            return 1;
                    }
                }
            }
        }
    }
    CItemRepository* pRepo = m_pOwner->GetItemRepository(1);
    if (pRepo == 0)
        return 0;
    for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++)
    {
        CSWSItem* pItem = pRepo->ItemListGetItem(i);
        if (pItem == 0)
            continue;
        int nBaseItem = pItem->m_baseItem;
        if (g_pRules->GetBaseItem(nBaseItem)->m_nEquipableSlots != 0)
            continue;
        if (!pItem->GetPropertyByTypeExists(ITEM_PROPERTY_ACTIVATE_ITEM, 0))
            continue;
        for (int j = 0; j < pItem->m_nActiveProperties; j++)
        {
            if (pItem->GetActiveProperty(j)->m_type == ITEM_PROPERTY_ACTIVATE_ITEM &&
                (pItem->GetActiveProperty(j)->m_bUseableByte != 0 ||
                 pItem->m_nCharges >= 1))
            {
                unsigned char nSlot = pItem->GetActiveProperty(j)->m_nUpgradeType;
                if ((nSlot == CSWItemProperty::UPGRADE_TYPE_NONE || (pItem->m_nUpgrades & (1 << nSlot)) != 0) &&
                    pItem->GetActiveProperty(j)->m_subType == nPower)
                    return 1;
            }
        }
    }
    return 0;
}

// Works like GetHasPowerFromItem, but writes the item's object id and the property index to
// out-params.
int CSWSCreatureStats::GetItemDetailsForPower(int nPower, unsigned long long& rItemId,
                                              int& rPropIndex)
{
    if (m_pOwner->m_pInventory != 0)
    {
        for (int slot = 0; slot < NUM_INVENTORY_SLOTS; slot++)
        {
            CSWSItem* pItem = static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(1 << slot));
            if (pItem != 0 && pItem->GetPropertyByTypeExists(ITEM_PROPERTY_ACTIVATE_ITEM, 0) &&
                pItem->m_nActiveProperties >= 1)
            {
                for (int j = 0; j < pItem->m_nActiveProperties; j++)
                {
                    if (pItem->GetActiveProperty(j)->m_type == ITEM_PROPERTY_ACTIVATE_ITEM &&
                        pItem->GetActiveProperty(j)->m_bUseableByte != 0)
                    {
                        unsigned char nSlot = pItem->GetActiveProperty(j)->m_nUpgradeType;
                        if ((nSlot == CSWItemProperty::UPGRADE_TYPE_NONE || (pItem->m_nUpgrades & (1 << nSlot)) != 0) &&
                            pItem->GetActiveProperty(j)->m_subType == nPower)
                        {
                            rItemId = pItem->m_objectId;
                            rPropIndex = j;
                            return 1;
                        }
                    }
                }
            }
        }
    }
    CItemRepository* pRepo = m_pOwner->GetItemRepository(1);
    if (pRepo == 0)
        return 0;
    for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++)
    {
        CSWSItem* pItem = pRepo->ItemListGetItem(i);
        if (pItem == 0)
            continue;
        if (g_pRules->GetBaseItem(pItem->m_baseItem)->m_nEquipableSlots != 0)
            continue;
        if (!pItem->GetPropertyByTypeExists(ITEM_PROPERTY_ACTIVATE_ITEM, 0))
            continue;
        if (pItem->m_nActiveProperties < 1)
            continue;
        for (int j = 0; j < pItem->m_nActiveProperties; j++)
        {
            if (pItem->GetActiveProperty(j)->m_type == ITEM_PROPERTY_ACTIVATE_ITEM &&
                (pItem->GetActiveProperty(j)->m_bUseableByte != 0 ||
                 pItem->m_nCharges >= 1))
            {
                unsigned char nSlot = pItem->GetActiveProperty(j)->m_nUpgradeType;
                if ((nSlot == CSWItemProperty::UPGRADE_TYPE_NONE || (pItem->m_nUpgrades & (1 << nSlot)) != 0) &&
                    pItem->GetActiveProperty(j)->m_subType == nPower)
                {
                    rItemId = pItem->m_objectId;
                    rPropIndex = j;
                    return 1;
                }
            }
        }
    }
    return 0;
}
// Matches the first entry with a different ready flag and the requested caster level.
void CSWSCreatureStats::SetSpellLikeAbilityReady(unsigned int nSpellId, unsigned char nReady,
                                                 unsigned char nCasterLevel)
{
    for (int i = 0; i < m_pSpellLikeAbilityList->m_count; i++)
    {
        CSWSStats_SpellLikeAbility& entry = m_pSpellLikeAbilityList->m_pArray[i];
        if (entry.m_nSpellId == nSpellId && entry.m_nReady != nReady &&
            entry.m_nCasterLevel == nCasterLevel)
        {
            entry.m_nReady = nReady;
            return;
        }
    }
}

void CSWSCreatureStats::ResetSpellLikeAbilities()
{
    for (int i = 0; i < m_pSpellLikeAbilityList->m_count; i++)
        m_pSpellLikeAbilityList->m_pArray[i].m_nReady = 1;
}


CExoString CSWSCreatureStats::GetClassString(unsigned char classIndex)
{
    if (classIndex < m_numClasses)
    {
        CSWClass* pClass = &g_pRules->m_paClasses[GetClass(classIndex)];
        if (pClass != 0)
            return pClass->GetNameText();
    }
    return CExoString("");
}

CExoString CSWSCreatureStats::GetRaceString()
{
    if (m_sSubrace == CExoString(""))
        return g_pRules->m_paRaces[m_nRace].GetNameText();

    return m_sSubrace;
}

// The alignment band as one of the three feedback strings.
CExoString CSWSCreatureStats::GetAlignmentString()
{
    CExoString sAlignment;

    int nBucket = (m_nAlignment < 41) ? ALIGNMENT_DARK_SIDE
                  : ((m_nAlignment > 59) ? ALIGNMENT_LIGHT_SIDE : ALIGNMENT_NEUTRAL);
    if (nBucket == ALIGNMENT_DARK_SIDE)
        sAlignment = "Evil";
    else if (nBucket == ALIGNMENT_LIGHT_SIDE)
        sAlignment = "Good";
    else
        sAlignment = "Neutral";

    return sAlignment;
}

// Resolves the creature's name through the server's language for the given player.
CExoString CSWSCreatureStats::GetFullName()
{
    CExoString sName;
    CSWSMessage::GetLocStringServer(MSG_PLAYERID_ALL_CLIENTS, m_firstName, m_lastName, sName, 0);
    return sName;
}

// Returns the portrait resref, the creature's portrait plus the size suffix.

CResRef CSWSCreatureStats::GetSmallPortrait()
{
    CResRef resRef = m_portrait;
    resRef += CExoString("S");
    return resRef;
}

CResRef CSWSCreatureStats::GetLargePortrait()
{
    CResRef resRef = m_portrait;
    resRef += CExoString("L");
    return resRef;
}


// The class slot's level, plus the character-level-30 achievement on a player-made creature.
void CSWSCreatureStats::SetClassLevel(unsigned char classIndex, unsigned char nLevel)
{
    if (classIndex < m_numClasses)
    {
        m_classInfo[classIndex].m_level = nLevel;
#if !KOTOR1
        if (m_pOwner != 0 && m_pOwner->m_bPlayerMadeCharacter != 0)
        {
            if (GetLevel(0) >= 30)
                Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_LEVEL_30);
        }
#endif
    }
}

// The slot's caster level, gated on classes.2da marking the class a caster.
unsigned char CSWSCreatureStats::GetCasterLevel(unsigned char classIndex)
{
    unsigned char nCasterLevel = 0;
    if (g_pRules->m_paClasses[GetClass(classIndex)].m_bSpellCaster != 0)
        nCasterLevel = GetClassLevel(classIndex, 0);
    return nCasterLevel;
}

// Appends a class slot; a third class overwrites the second.
int CSWSCreatureStats::HasForcePowerRequirments(unsigned int nSpellId)
{
    CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
    if (pSpell == 0)
        return 0;
    // Constructed and destroyed but never read.
    CExoString sPrerequisites = pSpell->m_sPrerequisites;
    if (m_bIsPC != 0 && pSpell->m_nUserType != CSWSpell::USERTYPE_MONSTERPOWER)
        return 0;
#if KOTOR1
    for (int64_t i = 0; pSpell->m_anPrereq2[i] != -1; i++)
        if (!HasSpell(0, pSpell->m_anPrereq2[i], 1))
            return 0;
    for (int64_t i = 0; pSpell->m_anPrereq1[i] != -1; i++)
        if (!HasSpell(0, pSpell->m_anPrereq1[i], 1))
            return 0;
#else
    for (int* pnPrereq = pSpell->m_anPrereq2; *pnPrereq != -1; pnPrereq++)
        if (!HasSpell(0, *pnPrereq, 1))
            return 0;
    for (int* pnPrereq = pSpell->m_anPrereq1; *pnPrereq != -1; pnPrereq++)
        if (!HasSpell(0, *pnPrereq, 1))
            return 0;
#endif
    for (int64_t i = 0; i < m_numClasses; i++) {
        unsigned char nSpellLevel = pSpell->GetSpellLevel(GetClass(static_cast<unsigned char>(i)));
        unsigned char nClassLevel = GetClassLevel(static_cast<unsigned char>(i), 0);
        if (nSpellLevel != CSWSpell::SPELL_LEVEL_NONE && nClassLevel < nSpellLevel)
            return 0;
    }
    return 1;
}

void CSWSCreatureStats::AddClass(unsigned char nClass, unsigned char nLevel)
{
    if (m_numClasses <= 1)
        m_numClasses++;

    unsigned char nSlot = m_numClasses - 1;
    m_classInfo[nSlot].m_class = nClass;
    m_classInfo[nSlot].m_level = nLevel;

#if !KOTOR1
    if (m_pOwner != 0 && m_pOwner->m_bPlayerMadeCharacter != 0)
    {
        if (GetLevel(0) >= 30)
            Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_LEVEL_30);
    }
#endif

    if (m_pOwner != 0)
        m_pOwner->UpdatePureGoodEvilPowers();
}

// The first parameter is unused; the tables are read from the last class slot.
void CSWSCreatureStats::CalcLevelUpNumberFeats(unsigned char classIndex,
                                               unsigned char& nNormalFeats,
                                               unsigned char& nBonusFeats)
{
    K2_UNUSED(classIndex);
#ifdef K2_FIX_ENGINE_BUGS
    if (m_numClasses == 0)
    {
        nNormalFeats = 0;
        nBonusFeats = 0;
        return;
    }
#endif
    CSWRules* pRules = g_pRules;
    unsigned char nClass = CLASS_TYPE_INVALID;
    unsigned char nLevel = 1;
    if (m_numClasses != 0)
    {
        unsigned char nSlot = m_numClasses - 1;
        CSWSCreatureStats_ClassInfo* pRec = &m_classInfo[nSlot];
        nLevel = pRec->m_level + 1;
#if KOTOR1
        if (nLevel < 21)
#else
        if (nLevel < 51)
#endif
        {
            nClass = pRec->m_class;
        }
        else
        {
            nNormalFeats = 0;
            nBonusFeats = 0;
            return;
        }
    }

    CSWClass* pClass = &pRules->m_paClasses[nClass];
    nNormalFeats = pClass->m_normalFeats.m_aByLevel[nLevel];
    nBonusFeats = pClass->m_bonusFeats.m_aByLevel[nLevel];
}

// Four independent nibble tests, each wildcard or equal, all required.
int CSWSCreatureStats::CheckForCategoryMatch(int nCategory, int nMask)
{
    int bMatch = ((nMask & TALENT_CATEGORY_DIGIT_3) == TALENT_CATEGORY_DIGIT_3) | ((nMask & TALENT_CATEGORY_DIGIT_3) == (nCategory & TALENT_CATEGORY_DIGIT_3));
    bMatch = (((nMask & 0x0f00) == 0x0f00) | ((nMask & 0x0f00) == (nCategory & 0x0f00))) & bMatch;
    bMatch = (((nMask & 0x00f0) == 0x00f0) | ((nMask & 0x00f0) == (nCategory & 0x00f0))) & bMatch;
    bMatch = (((nMask & 0x000f) == 0x000f) | ((nMask & 0x000f) == (nCategory & 0x000f))) & bMatch;
    return bMatch;
}


int CSWSCreatureStats::GetWeaponFocus(CSWSItem* pItem)
{
    if (pItem == 0)
        return 0;
    if (pItem->GetBaseItem() == 0)
        return 0;

    unsigned short nFeat = pItem->GetBaseItem()->m_nWeaponFocusFeat;
    if (nFeat == 0)
        return 0;
    return HasFeat(nFeat);
}

int CSWSCreatureStats::GetWeaponSpecialization(CSWSItem* pItem)
{
    if (pItem == 0)
        return 0;
    if (pItem->GetBaseItem() == 0)
        return 0;

    unsigned short nFeat = pItem->GetBaseItem()->m_nWeaponSpecializationFeat;
    if (nFeat == 0)
        return 0;
    return HasFeat(nFeat);
}

// Sets one skill's rank and pays a per-skill achievement at rank 30.
void CSWSCreatureStats::SetSkillRank(unsigned char nSkill, char nRank)
{
    if (nSkill < g_pRules->m_nNumSkills)
    {
        m_pSkillRanks[nSkill] = nRank;

#if !KOTOR1
        if (nRank >= 30)
        {
            switch (nSkill)
            {
            case SKILL_COMPUTER_USE: Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_COMPUTER_USE_30); break;
            case SKILL_DEMOLITIONS: Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_DEMOLITIONS_30); break;
            case SKILL_STEALTH: Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_STEALTH_30); break;
            case SKILL_REPAIR: Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_REPAIR_30); break;
            case SKILL_TREAT_INJURY: Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_TREAT_INJURY_30); break;
            default: break;
            }
        }
#endif
    }
}

// Whether the creature can use a skill at all.
int CSWSCreatureStats::GetCanUseSkill(unsigned char nSkill)
{
    unsigned char nSkillIdx = (static_cast<unsigned char>(nSkill - 100) < 3) ? 1 : nSkill;
    if (nSkillIdx < g_pRules->m_nNumSkills)
    {
#if !KOTOR1  // K1 has no power or feat grant of Stealth.
        if (nSkillIdx == 2)
        {
            if (HasSpell(0, FORCE_POWER_FORCE_CAMOUFLAGE, 0)) return 1;
            if (HasSpell(0, FORCE_POWER_IMPROVED_FORCE_CAMOUFLAGE, 0)) return 1;
            if (HasSpell(0, FORCE_POWER_MASTER_FORCE_CAMOUFLAGE, 0)) return 1;
            if (HasFeat(FEAT_PERSONAL_CLOAKING_SHIELD)) return 1;
        }
#endif
        if (g_pRules->m_paSkills[nSkillIdx].m_flags & CSWSkill::FLAG_UNTRAINED)
        {
            if (m_numClasses == 0)
                return 0;
            for (int i = 0; i < m_numClasses; i++)
            {
#if KOTOR1
                unsigned char nClassId = m_classInfo[i].m_class;
                if (g_pRules->m_paClasses[nClassId].IsSkillUseable(nSkill))
#else
                if (g_pRules->m_paClasses[m_classInfo[i].m_class].IsSkillUseable(nSkill))
#endif
                    return 1;
            }
            return 0;
        }
#if KOTOR1
        return GetSkillRank(nSkillIdx, 0, 1) > 0;
#else
        int nRank = m_pSkillRanks[nSkillIdx];
        nRank = (nRank < 127) ? nRank : 127;
        nRank = (nRank > -127) ? nRank : -127;
        return nRank > 0;
#endif
    }
    return 0;
}

// Resolves the movement rate and caches the run and walk speeds.
void CSWSCreatureStats::SetMovementRate(int nRate)
{
    if (nRate == 7)
    {
        CExoString sName;
        CExoString s2DAName;
        nRate = 0;
        if (g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
                m_nAppearanceType, CExoString("MOVERATE"), &sName))
        {
            int i;
            for (i = 0; i < g_pRules->m_p2DArrays->m_pCreatureSpeed->m_nRows; i++)
            {
                if (g_pRules->m_p2DArrays->m_pCreatureSpeed->GetCExoStringEntry(
                        i, CExoString("2DAName"), &s2DAName))
                {
                    if (sName.LowerCase() == s2DAName.LowerCase())
                        break;
                }
            }
            if (i < g_pRules->m_p2DArrays->m_pCreatureSpeed->m_nRows)
                nRate = i;
        }
    }
    m_nMovementRate = nRate;
    g_pRules->m_p2DArrays->m_pCreatureSpeed->GetFLOATEntry(
        nRate, CTwoDimArrays::COL2DA_CREATURESPEED_WALKRATE, &m_fWalkRate);
    g_pRules->m_p2DArrays->m_pCreatureSpeed->GetFLOATEntry(
        m_nMovementRate, CTwoDimArrays::COL2DA_CREATURESPEED_RUNRATE, &m_fRunRate);
    if (nRate == 1)
        m_pOwner->m_nAIState = 0xffff & ~CSWSCreature::AI_STATE_MOBILE;
}

// The tag lives on the owning object; the engine lower-cases it on the way in.
void CSWSCreatureStats::SetTag(const CExoString& sTag)
{
    CSWSCreature* pOwner = m_pOwner;
    pOwner->m_sTag = sTag.LowerCase();
}

// Returns the effective ability score, the base byte plus effects and equipment.

char CSWSCreatureStats::GetTotalSTRBonus()
{
    int nUnused = 0;
    return static_cast<char>(m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_ABILITY, 0, nUnused, 0,
                                                           0, 0, 0, CSWSCreature::EFFECT_BONUS_ANY, 0, 0, 0));
}

char CSWSCreatureStats::GetTotalDEXBonus()
{
    int nUnused = 0;
    return static_cast<char>(m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_ABILITY, 0, nUnused, 0,
                                                           0, 0, 0, CSWSCreature::EFFECT_BONUS_ANY, 1, 0, 0));
}

char CSWSCreatureStats::GetTotalCONBonus()
{
    int nUnused = 0;
    return static_cast<char>(m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_ABILITY, 0, nUnused, 0,
                                                           0, 0, 0, CSWSCreature::EFFECT_BONUS_ANY, 2, 0, 0));
}

char CSWSCreatureStats::GetTotalINTBonus()
{
    int nUnused = 0;
    return static_cast<char>(m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_ABILITY, 0, nUnused, 0,
                                                           0, 0, 0, CSWSCreature::EFFECT_BONUS_ANY, 3, 0, 0));
}

char CSWSCreatureStats::GetTotalWISBonus()
{
    int nUnused = 0;
    return static_cast<char>(m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_ABILITY, 0, nUnused, 0,
                                                           0, 0, 0, CSWSCreature::EFFECT_BONUS_ANY, 4, 0, 0));
}

char CSWSCreatureStats::GetTotalCHABonus()
{
    int nUnused = 0;
    return static_cast<char>(m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_ABILITY, 0, nUnused, 0,
                                                           0, 0, 0, CSWSCreature::EFFECT_BONUS_ANY, 5, 0, 0));
}


// `&` rather than `&&`: both conditions stay live and the code stays branchless.
unsigned char CSWSCreatureStats::GetSpellsPerDayLeft(unsigned char classIndex, unsigned char nLevel)
{
    unsigned char nLeft = 0;
    if ((nLevel == 0) & (classIndex < m_numClasses))
        nLeft = m_classInfo[classIndex].m_spellsPerDayLeft;
    return nLeft;
}

void CSWSCreatureStats::DecrementSpellsPerDayLeft(unsigned char classIndex, unsigned char nLevel)
{
    if (nLevel == 0 && classIndex < m_numClasses)
        m_classInfo[classIndex].m_spellsPerDayLeft--;
}

void CSWSCreatureStats::IncrementSpellsPerDayLeft(unsigned char classIndex, unsigned char nLevel)
{
    if (nLevel == 0 && classIndex < m_numClasses)
        m_classInfo[classIndex].m_spellsPerDayLeft++;
}

unsigned short CSWSCreatureStats::GetNumberKnownSpells(unsigned char classIndex, unsigned char nLevel)
{
    if (classIndex >= m_numClasses)
        return 0;
    return static_cast<unsigned short>(m_classInfo[classIndex].m_knownSpells[nLevel].m_count);
}

// Sets the remaining spells per day; the class bound is checked before the level.
int CSWSCreatureStats::GetKnownSpell(unsigned char classIndex, unsigned char nLevel,
                                     unsigned char nIndex)
{
    if (classIndex < m_numClasses)
        return m_classInfo[classIndex].GetKnownSpell(nLevel, nIndex);
    return -1;
}

// The per-level advancement record for a 0-based character level.
CSWLevelStats* CSWSCreatureStats::GetLevelStats(unsigned char nLevel)
{
    if (m_bIsPC == 0)
        return 0;
    if (m_levelStats.m_count <= nLevel)
        return 0;
    return m_levelStats.m_pArray[nLevel];
}


unsigned int CSWSCreatureStats::GetExpNeededForLevelUp()
{
    return g_pRules->m_aExpTable[GetLevel(0)];
}

// The first XP-table entry the earned XP has not reached, or 0 once it has.
unsigned int CSWSCreatureStats::CalculateExpNeededForNextLevel()
{
    unsigned int nEarned = m_nExperience;
#if KOTOR1
    for (int nLevel = 0; nLevel < 21; nLevel++)
#else
    for (int nLevel = 0; nLevel < 51; nLevel++)
#endif
    {
        unsigned int nExp = g_pRules->m_aExpTable[static_cast<unsigned char>(nLevel)];
        if (nEarned < nExp)
            return nExp;
    }
    return 0;
}

// Under the multiclass XP penalty, every class trailing the highest non-favored class by more than
// one level raises the cost of the next level.
void CSWSCreatureStats::AddExperience(unsigned int nExp)
{
    uint64_t nClasses = m_numClasses;
    if (nClasses >= 2)
    {
        unsigned char nFavored = g_pRules->m_paRaces[m_nRace].m_nFavoredClass;
        if (nFavored == CLASS_TYPE_INVALID)
        {
#if KOTOR1
// K1: the highest level of the first three class slots.
            unsigned char nBest = 0;
            unsigned char nBestLvl = m_classInfo[0].m_level;
#ifdef K2_FIX_ENGINE_BUGS
            for (unsigned char i = 1; i < 2; i++)
#else
            for (unsigned char i = 1; i < 3; i++)
#endif
            {
                unsigned char nLvl = (m_numClasses <= i) ? 0 : m_classInfo[i].m_level;
                if (nBestLvl < nLvl)
                {
                    nBest = i;
                    nBestLvl = nLvl;
                }
            }
            nFavored = GetClass(nBest);
#else
            unsigned char nBest = (m_classInfo[0].m_level < m_classInfo[1].m_level) ? 1 : 0;
#ifndef K2_FIX_ENGINE_BUGS
            unsigned char nBestLvl = (m_numClasses <= nBest) ? 0 : m_classInfo[nBest].m_level;
            unsigned char nLvl2 = (2 < m_numClasses) ? m_classInfo[2].m_level : 0;
            if (nBestLvl < nLvl2)
                nBest = 2;
#endif
            nFavored = GetClass(nBest);
#endif
        }
        unsigned char nMaxLevel = 0;
        for (int i = 0; static_cast<uint64_t>(i) < nClasses; i++)
        {
            if (nFavored != m_classInfo[i].m_class)
            {
                if (nMaxLevel <= m_classInfo[i].m_level)
                    nMaxLevel = m_classInfo[i].m_level;
            }
        }
        double fPenalty = 0.0;
        for (int i = 0; static_cast<uint64_t>(i) < nClasses; i++)
        {
            if (nFavored != m_classInfo[i].m_class && (nMaxLevel - m_classInfo[i].m_level) > 1)
                fPenalty += 1.0;
        }
        nExp = static_cast<unsigned int>(static_cast<double>(nExp) * (1.0L + static_cast<long double>(fPenalty) * -0.2L));
    }
    SetExperience(m_nExperience + nExp, 1);
}

// Sets the earned XP and runs the level-up and level-down cascades when asked.
void CSWSCreatureStats::SetExperience(unsigned int nExp, int bLevelUp)
{
    CSWSPlayer* pClient =
        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_pOwner->m_objectId);
    g_pAppManager->m_pServerExoApp->GetSWSMessage();
    int nOldExp = m_nExperience;
    CExoString sUnused;
    if (static_cast<unsigned int>(m_nExperience) > nExp)
    {
        CExoString sMsg;
        const char* szTag = "<NO TAG>";
#ifdef K2_FIX_ENGINE_BUGS
        if (m_pOwner != 0 && m_pOwner->m_sTag != "")
#else
        if (m_pOwner != 0 && m_pOwner->m_sTag.CStr() != "")
#endif
            szTag = m_pOwner->m_sTag.CStr();
        sMsg.Format(const_cast<char*>("Tried to reduce XP of creature '%s' to '%d'. Cannot reduce XP."), szTag, nExp);
        return;
    }
    m_nExperience = nExp;
// Without bLevelUp there is no cascade, and the store returns immediately.
    if (bLevelUp == 0)
        return;
    int nDelta = nExp - nOldExp;
    unsigned char nLevel;
    unsigned char nPrevLevel;
    CSWLevelStats* pLevelStats;
    if (nDelta < 0)
    {
        nLevel = GetLevel(0);
        while (g_pAppManager->m_pServerExoApp->GetServerInfo()->m_nMinCharacterLevel < nLevel
               && nLevel != 0)
        {
            nPrevLevel = nLevel - 1;
            if (static_cast<unsigned int>(g_pRules->m_aExpTable[nPrevLevel]) <= static_cast<unsigned int>(m_nExperience))
                break;
            pLevelStats = 0;
            if (m_bIsPC != 0 && m_levelStats.m_count > nPrevLevel)
                pLevelStats = m_levelStats.m_pArray[nPrevLevel];
            nLevel--;
            LevelDown(pLevelStats);
        }
    }
    else if (nDelta > 0)
    {
        while (CanLevelUp() && m_levelStats.m_count > GetLevel(0))
        {
            pLevelStats = m_levelStats.m_pArray[GetLevel(0)];
            if (pLevelStats != 0)
                LevelUp(pLevelStats, 0);
        }
    }
    if (pClient != 0 && CanLevelUp())
    {
        if (static_cast<unsigned int>(nOldExp) < static_cast<unsigned int>(g_pRules->m_aExpTable[GetLevel(0)]))
            m_pOwner->SendFeedbackMessage(FEEDBACK_LEVEL_UP_AVAILABLE, 0);
#if !KOTOR1  // K1 has no k_level_dlg "000_Level_Dlg_Fired" tutorial hook.
        if (m_pOwner->m_bPlayerMadeCharacter != 0 &&
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState == 0)
        {
            char bDlgFired = 0;
            g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->GetValueNumber(
                CExoString("000_Level_Dlg_Fired"), bDlgFired);
            if (bDlgFired == 0)
            {
                g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(
                    CExoString("000_Level_Dlg_Fired"), 1);
                CExoString sScript("k_level_dlg");
                g_pVirtualMachine->RunScript(&sScript, 0, 0);
            }
        }
#endif
    }
}

// Zeroes every limited-use feat's spend counter.
void CSWSCreatureStats::ResetFeatRemainingUses()
{
    for (unsigned short i = 0; i < m_featRemainingUses.m_count; i++)
    {
        CSWSStats_FeatRemainingUses* pEntry = m_featRemainingUses.m_pArray[i];
        if (pEntry != 0)
            pEntry->m_nUsesUsed = 0;
    }
}

// The lookup twin of ResetFeatRemainingUses; HasFeat is emitted here.
unsigned char CSWSCreatureStats::GetFeatRemainingUses(unsigned short nFeat)
{
    CSWFeat* pFeat = g_pRules->GetFeat(nFeat);
    if (pFeat == 0)
        return 0;
    if (!HasFeat(nFeat))
        return 0;
    unsigned short i = 0;
    while (i < m_featRemainingUses.m_count)
    {
        CSWSStats_FeatRemainingUses* pEntry = m_featRemainingUses.m_pArray[i++];
        if (pEntry->m_nFeat == nFeat)
        {
            if (pEntry == 0)
                return 100;
            unsigned char nRemaining = pFeat->m_maxUses - pEntry->m_nUsesUsed;
            return (nRemaining < 100) ? nRemaining : 100;
        }
    }
    return 100;
}

// Drops nFeat from the feat list.
void CSWSCreatureStats::RemoveFeat(unsigned short nFeat)
{
    m_feats.Remove(nFeat);
    // The wide temporary and the preguard are load-bearing.
    int nLast = m_featRemainingUses.m_count - 1;
    if (nLast >= 0)
    {
        for (short i = static_cast<short>(nLast); i >= 0; i--)
        {
            CSWSStats_FeatRemainingUses* pEntry = m_featRemainingUses.m_pArray[i];
            if (pEntry->m_nFeat == nFeat)
            {
                m_featRemainingUses.DelIndex(i);
                if (pEntry != 0)
                {
                    delete pEntry;
                    return;
                }
            }
        }
    }
}

// The class slot holding nClass, or the no-such-class sentinel when the creature does not have it.
unsigned char CSWSCreatureStats::GetIsClass(unsigned char nClass)
{
    for (int i = 0; i < m_numClasses; i++)
    {
        if (GetClass(static_cast<unsigned char>(i)) == nClass)
            return static_cast<unsigned char>(i);
    }
    return 0xff;
}

// Effective rank in one skill.
char CSWSCreatureStats::GetSkillRank(unsigned char nSkill, CSWSObject* pObject, int a)
{
    if (nSkill >= g_pRules->m_nNumSkills)
        return 0;
    int nRank = m_pSkillRanks[nSkill];
    if (a == 0) {
#if !KOTOR1  // K1 has no Force Camouflage stealth terms (here or in the stealth case).
        if (nSkill == SKILL_STEALTH && nRank == 0) {
            if (HasSpell(0, FORCE_POWER_FORCE_CAMOUFLAGE, 0) != 0)
                nRank = 1;
        }
#endif
        if ((nRank | ((g_pRules->m_paSkills[nSkill].m_flags >> 1) & 1)) == 0)
            return 0;
        int nFx = 0;
        nRank += m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_SKILL, pObject, nFx, 0, 0, 0, 0,
                                               nSkill, CSWSCreature::EFFECT_BONUS_ANY, 0, 0);
        if (g_pRules->m_paSkills[nSkill].m_nKeyAbility == CSWSEffectListHandler::EFFECT_ABILITY_STRENGTH) {
            nRank += static_cast<signed char>(m_nSTRMod);
            if (m_pOwner->GetBlind() != 0)
                nRank -= 4;
        } else if (g_pRules->m_paSkills[nSkill].m_nKeyAbility == CSWSEffectListHandler::EFFECT_ABILITY_DEXTERITY) {
            nRank += GetDEXMod(0);
            if (m_pOwner->GetBlind() != 0)
                nRank -= 4;
        } else if (g_pRules->m_paSkills[nSkill].m_nKeyAbility == CSWSEffectListHandler::EFFECT_ABILITY_CONSTITUTION) {
            nRank += m_nCONMod;
        } else if (g_pRules->m_paSkills[nSkill].m_nKeyAbility == CSWSEffectListHandler::EFFECT_ABILITY_INTELLIGENCE) {
            nRank += static_cast<signed char>(m_nINTMod);
        } else if (g_pRules->m_paSkills[nSkill].m_nKeyAbility == CSWSEffectListHandler::EFFECT_ABILITY_WISDOM) {
            nRank += m_nWISMod;
        } else if (g_pRules->m_paSkills[nSkill].m_nKeyAbility == CSWSEffectListHandler::EFFECT_ABILITY_CHARISMA) {
            nRank += static_cast<signed char>(m_nCHAMod);
        }
        if (g_pRules->m_paSkills[nSkill].m_flags & CSWSkill::FLAG_ARMOR_CHECK_PENALTY)
            nRank += static_cast<signed char>(reinterpret_cast<unsigned char*>(&m_nArmorCheckPenalty)[1] + reinterpret_cast<unsigned char*>(&m_nArmorCheckPenalty)[0]);
        CSWSPlaceable* pPlaceable = g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(
            m_pOwner->m_dialogOwnerId);
        switch (nSkill) {
        case SKILL_COMPUTER_USE:
            if (HasFeat(FEAT_GEAR_HEAD_MASTER) != 0)
                nRank += 3;
            else if (HasFeat(FEAT_GEAR_HEAD_ADEPT) != 0)
                nRank += 2;
            else if (HasFeat(FEAT_GEAR_HEAD) != 0)
                nRank += 1;
            if (pPlaceable != 0)
                nRank += pPlaceable->m_computerSpikeBonus;
            break;
        case SKILL_DEMOLITIONS:
            if (HasFeat(FEAT_MASTER_CAUTION) != 0)
                nRank += 3;
            else if (HasFeat(FEAT_IMPROVED_CAUTION) != 0)
                nRank += 2;
            else if (HasFeat(FEAT_CAUTIOUS) != 0)
                nRank += 1;
            break;
        case SKILL_STEALTH:
            if (HasFeat(FEAT_MASTER_CAUTION) != 0)
                nRank += 3;
            else if (HasFeat(FEAT_IMPROVED_CAUTION) != 0)
                nRank += 2;
            else if (HasFeat(FEAT_CAUTIOUS) != 0)
                nRank += 1;
#if !KOTOR1
            if (HasSpell(0, FORCE_POWER_MASTER_FORCE_CAMOUFLAGE, 0) != 0)
                nRank += 8;
            else if (HasSpell(0, FORCE_POWER_IMPROVED_FORCE_CAMOUFLAGE, 0) != 0)
                nRank += 4;
#endif
            break;
        case SKILL_AWARENESS:
            if (HasFeat(FEAT_MASTER_EMPATHY) != 0)
                nRank += 3;
            else if (HasFeat(FEAT_IMPROVED_EMPATHY) != 0)
                nRank += 2;
            else if (HasFeat(FEAT_EMPATHY) != 0)
                nRank += 1;
            break;
        case SKILL_PERSUADE: {
            if (HasFeat(FEAT_MASTER_EMPATHY) != 0)
                nRank += 3;
            else if (HasFeat(FEAT_IMPROVED_EMPATHY) != 0)
                nRank += 2;
            else if (HasFeat(FEAT_EMPATHY) != 0)
                nRank += 1;
#if !KOTOR1
            static unsigned char s_paidOutAchievement = 0;
            if (m_bIsPC != 0 && nRank >= 30 && s_paidOutAchievement == 0) {
                s_paidOutAchievement = 1;
                Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_PERSUADE_30);
            }
#endif
            break;
        }
        case SKILL_REPAIR:
            if (HasFeat(FEAT_GEAR_HEAD_MASTER) != 0)
                nRank += 3;
            else if (HasFeat(FEAT_GEAR_HEAD_ADEPT) != 0)
                nRank += 2;
            else if (HasFeat(FEAT_GEAR_HEAD) != 0)
                nRank += 1;
            break;
        case SKILL_SECURITY:
            if (HasFeat(FEAT_GEAR_HEAD_MASTER) != 0)
                nRank += 3;
            else if (HasFeat(FEAT_GEAR_HEAD_ADEPT) != 0)
                nRank += 2;
            else if (HasFeat(FEAT_GEAR_HEAD) != 0)
                nRank += 1;
            break;
        case SKILL_TREAT_INJURY:
            if (HasFeat(FEAT_MASTER_EMPATHY) != 0)
                nRank += 3;
            else if (HasFeat(FEAT_IMPROVED_EMPATHY) != 0)
                nRank += 2;
            else if (HasFeat(FEAT_EMPATHY) != 0)
                nRank += 1;
            break;
        }
    }
    nRank = (nRank < 127) ? nRank : 127;
    nRank = (nRank > -127) ? nRank : -127;
    return static_cast<char>(nRank);
}


char CSWSCreatureStats::GetFortSavingThrow(int bIgnoreEffects)
{
    int nEffectBonus = 0;
    if (bIgnoreEffects == 0)
    {
        int nUnused = 0;
        nEffectBonus = m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_SAVING_THROW, 0, nUnused, 0,
                                                     0, 1, 0, CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, 0, 0);
    }
    return static_cast<char>(m_nCONMod + nEffectBonus + (GetBaseFortSavingThrow() + m_nFortSaveBonus));
}

char CSWSCreatureStats::GetWillSavingThrow(int bIgnoreEffects)
{
    int nEffectBonus = 0;
    if (bIgnoreEffects == 0)
    {
        int nUnused = 0;
        nEffectBonus = m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_SAVING_THROW, 0, nUnused, 0,
                                                     0, 3, 0, CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, 0, 0);
    }
    return static_cast<char>(m_nWISMod + nEffectBonus + (GetBaseWillSavingThrow() + m_nWillSaveBonus));
}

// Computes the reflex save, the odd one out that alone caps its ability modifier.
char CSWSCreatureStats::GetReflexSavingThrow(int bIgnoreEffects)
{
    int nEffectBonus = 0;
    if (bIgnoreEffects == 0)
    {
        int nUnused = 0;
        nEffectBonus = m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_SAVING_THROW, 0, nUnused, 0,
                                                     0, 2, 0, CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, 0, 0);
    }
#if KOTOR1  // K1 takes the dex term from GetDEXMod(0)
    // K1: calls IsDebilitated in both arms.
    char nDexMod = GetDEXMod(0);
    return static_cast<char>(nEffectBonus + GetBaseReflexSavingThrow() + nDexMod + m_nReflexSaveBonus);
#else
    int bDebilitated = m_pOwner->IsDebilitated(1);
    int nDexMod = 0;
    if ((m_nDEXMod < 0) || (bDebilitated == 0))
        nDexMod = m_nDEXMod;
    return static_cast<char>(nEffectBonus + GetBaseReflexSavingThrow() + nDexMod + m_nReflexSaveBonus);
#endif
}

// Each of the three saving throws is the base value plus the ability modifier plus the effect bonus
// and the per-creature byte.
char CSWSCreatureStats::GetBaseFortSavingThrow()
{
    char nSave = 0;
    for (int i = 0; i < m_numClasses; i++)
    {
        CSWRules* pRules = g_pRules;
        unsigned char nClassId = GetClass(static_cast<unsigned char>(i));
        CSWClass* pClass = &pRules->m_paClasses[nClassId];
        if (pClass != 0)
            nSave += pClass->GetFortSaveBonus(GetClassLevel(static_cast<unsigned char>(i), 0));
    }

#if !KOTOR1
    if (m_pOwner->m_bInParty == 0 && m_pOwner->m_nMultiplierSet != 0)
    {
        float fSaveMult;
        g_pRules->m_p2DArrays->m_pAutoBalance->GetFLOATEntry(
            m_pOwner->m_nMultiplierSet, CExoString("SaveMult"), &fSaveMult);

        char nAutoBalanceLevel = m_pOwner->m_nPCLevelAtSpawn;
        nSave += static_cast<char>(static_cast<int>(fSaveMult * ((nAutoBalanceLevel != 0) ? static_cast<float>(nAutoBalanceLevel) : 1.0f)));
    }
#endif

    if (HasFeat(FEAT_LIGHTNING_REFLEXES))
        nSave += 3;
    else if (HasFeat(FEAT_IRON_WILL))
        nSave += 2;
    else if (HasFeat(FEAT_GREAT_FORTITUDE))
        nSave += 1;

    return nSave;
}

char CSWSCreatureStats::GetBaseWillSavingThrow()
{
    char nSave = 0;
    for (int i = 0; i < m_numClasses; i++)
    {
        CSWRules* pRules = g_pRules;
        unsigned char nClassId = GetClass(static_cast<unsigned char>(i));
        CSWClass* pClass = &pRules->m_paClasses[nClassId];
        if (pClass != 0)
            nSave += pClass->GetWillSaveBonus(GetClassLevel(static_cast<unsigned char>(i), 0));
    }

#if !KOTOR1
    if (m_pOwner->m_bInParty == 0 && m_pOwner->m_nMultiplierSet != 0)
    {
        float fSaveMult;
        g_pRules->m_p2DArrays->m_pAutoBalance->GetFLOATEntry(
            m_pOwner->m_nMultiplierSet, CExoString("SaveMult"), &fSaveMult);

        char nAutoBalanceLevel = m_pOwner->m_nPCLevelAtSpawn;
        nSave += static_cast<char>(static_cast<int>(fSaveMult * ((nAutoBalanceLevel != 0) ? static_cast<float>(nAutoBalanceLevel) : 1.0f)));
    }
#endif

    if (HasFeat(FEAT_LIGHTNING_REFLEXES))
        nSave += 3;
    else if (HasFeat(FEAT_IRON_WILL))
        nSave += 2;
    else if (HasFeat(FEAT_GREAT_FORTITUDE))
        nSave += 1;

    return nSave;
}

char CSWSCreatureStats::GetBaseReflexSavingThrow()
{
    char nSave = 0;
    for (int i = 0; i < m_numClasses; i++)
    {
        CSWRules* pRules = g_pRules;
        unsigned char nClassId = GetClass(static_cast<unsigned char>(i));
        CSWClass* pClass = &pRules->m_paClasses[nClassId];
        if (pClass != 0)
            nSave += pClass->GetRefSaveBonus(GetClassLevel(static_cast<unsigned char>(i), 0));
    }

#if !KOTOR1
    if (m_pOwner->m_bInParty == 0 && m_pOwner->m_nMultiplierSet != 0)
    {
        float fSaveMult;
        g_pRules->m_p2DArrays->m_pAutoBalance->GetFLOATEntry(
            m_pOwner->m_nMultiplierSet, CExoString("SaveMult"), &fSaveMult);

        char nAutoBalanceLevel = m_pOwner->m_nPCLevelAtSpawn;
        nSave += static_cast<char>(static_cast<int>(fSaveMult * ((nAutoBalanceLevel != 0) ? static_cast<float>(nAutoBalanceLevel) : 1.0f)));
    }
#endif

    if (HasFeat(FEAT_LIGHTNING_REFLEXES))
        nSave += 3;
    else if (HasFeat(FEAT_IRON_WILL))
        nSave += 2;
    else if (HasFeat(FEAT_GREAT_FORTITUDE))
        nSave += 1;

    return nSave;
}

// Returns the effective ability score, the base byte plus effects and equipment.

unsigned char CSWSCreatureStats::GetSTRStat()
{
#if KOTOR1
    // K1: the effect bonus is queried once.
    int nStat = m_nSTRBase + GetTotalSTRBonus() + g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[0];
    return static_cast<unsigned char>((nStat > 3) ? nStat : 3);
#else
    if (m_nSTRBase + GetTotalSTRBonus() + g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[0] >= 4)
        return static_cast<unsigned char>(m_nSTRBase + GetTotalSTRBonus() + g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[0]);
    return 3;
#endif
}

unsigned char CSWSCreatureStats::GetDEXStat()
{
#if KOTOR1
    // K1: the effect bonus is queried once.
    int nStat = m_nDEXBase + GetTotalDEXBonus() + g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[1];
    return static_cast<unsigned char>((nStat > 3) ? nStat : 3);
#else
    if (m_nDEXBase + GetTotalDEXBonus() + g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[1] >= 4)
        return static_cast<unsigned char>(m_nDEXBase + GetTotalDEXBonus() + g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[1]);
    return 3;
#endif
}

unsigned char CSWSCreatureStats::GetCONStat()
{
#if KOTOR1
    // K1: the effect bonus is queried once.
    int nStat = m_nCONBase + GetTotalCONBonus() + g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[5];
    return static_cast<unsigned char>((nStat > 3) ? nStat : 3);
#else
    if (m_nCONBase + GetTotalCONBonus() + g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[5] >= 4)
        return static_cast<unsigned char>(m_nCONBase + GetTotalCONBonus() + g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[5]);
    return 3;
#endif
}

unsigned char CSWSCreatureStats::GetINTStat()
{
#if KOTOR1
    // K1: the effect bonus is queried once.
    int nStat = m_nINTBase + GetTotalINTBonus() + g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[2];
    return static_cast<unsigned char>((nStat > 3) ? nStat : 3);
#else
    if (m_nINTBase + GetTotalINTBonus() + g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[2] >= 4)
        return static_cast<unsigned char>(m_nINTBase + GetTotalINTBonus() + g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[2]);
    return 3;
#endif
}

unsigned char CSWSCreatureStats::GetWISStat()
{
#if KOTOR1
    // K1: the effect bonus is queried once.
    int nStat = m_nWISBase + GetTotalWISBonus() + g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[4];
    return static_cast<unsigned char>((nStat > 3) ? nStat : 3);
#else
    if (m_nWISBase + GetTotalWISBonus() + g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[4] >= 4)
        return static_cast<unsigned char>(m_nWISBase + GetTotalWISBonus() + g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[4]);
    return 3;
#endif
}

unsigned char CSWSCreatureStats::GetCHAStat()
{
#if KOTOR1
    // K1: the effect bonus is queried once.
    int nStat = m_nCHABase + GetTotalCHABonus() + g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[3];
    return static_cast<unsigned char>((nStat > 3) ? nStat : 3);
#else
    if (m_nCHABase + GetTotalCHABonus() + g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[3] >= 4)
        return static_cast<unsigned char>(m_nCHABase + GetTotalCHABonus() + g_pRules->m_paRaces[m_nRace].m_anAbilityAdjust[3]);
    return 3;
#endif
}

// The uniform float view of the stat block, keyed by a small id.
float CSWSCreatureStats::GetStatById(int nStatId)
{
    switch (nStatId)
    {
    case 0:  return static_cast<float>(m_nAge);
    case 1:  return m_fChallengeRating;
    case 2:  return m_nFortSaveBonus;
    case 3:  return m_nReflexSaveBonus;
    case 4:  return m_nWillSaveBonus;
    case 5:  return m_nSTRBase;
    case 6:  return m_nDEXBase;
    case 7:  return m_nCONBase;
    case 8:  return m_nINTBase;
    case 9:  return m_nWISBase;
    case 10: return m_nCHABase;
    case 11: return static_cast<float>(GetBaseAttackBonus());
    case 12: return GetForceResistance();
    }
    return 0.0f;
}

// The write twin of GetStatById.
void CSWSCreatureStats::SetStatById(int nStatId, float fValue)
{
    switch (nStatId)
    {
    case 0:  m_nAge = static_cast<int>(fValue); break;
    case 1:  m_fChallengeRating = fValue; break;
    case 2:  m_nFortSaveBonus = static_cast<signed char>(fValue); break;
    case 3:  m_nReflexSaveBonus = static_cast<signed char>(fValue); break;
    case 4:  m_nWillSaveBonus = static_cast<signed char>(fValue); break;
    case 5:
        m_nSTRBase = static_cast<unsigned char>(fValue);
        UpdateSTRModifier();
        break;
    case 6:
        m_nDEXBase = static_cast<unsigned char>(fValue);
        UpdateDEXModifier();
        break;
    case 7:
    {
        m_nCONBase = static_cast<unsigned char>(fValue);
        short nOldMax = m_pOwner->GetMaxHitPoints(1);
        m_nCONMod = static_cast<signed char>(CalcStatModifier(GetCONStat()));
        m_pOwner->SetCurrentHitPoints(m_pOwner->GetCurrentHitPoints(1) +
                                      (m_pOwner->GetMaxHitPoints(1) - nOldMax));
        break;
    }
    case 8:
        m_nINTBase = static_cast<unsigned char>(fValue);
        m_nINTMod = static_cast<unsigned char>(CalcStatModifier(GetINTStat()));
        AdjustSpellUsesPerDay();
        break;
    case 9:
        m_nWISBase = static_cast<unsigned char>(fValue);
        m_nWISMod = static_cast<signed char>(CalcStatModifier(GetWISStat()));
        AdjustSpellUsesPerDay();
        break;
    case 10:
        m_nCHABase = static_cast<unsigned char>(fValue);
        m_nCHAMod = static_cast<unsigned char>(CalcStatModifier(GetCHAStat()));
        AdjustSpellUsesPerDay();
        break;
    case 11: m_nOverrideBaseAttackBonus = static_cast<unsigned char>(fValue); break;
    case 12:
        m_forceResistance = static_cast<char>(fValue);
        UpdateCombatInformation();
        break;
    }
}

// The level the earned XP would already support, walking the XP table up from the bottom.
void CSWSCreatureStats::LevelDown(CSWLevelStats* pLevelStats)
{
    short nOldMaxHP = m_pOwner->GetMaxHitPoints(1);

// K1: lays out the class-slot search loop the other way round.
    int nClassPos = 0;
    if (m_numClasses != 0)
    {
        while (m_classInfo[static_cast<unsigned char>(nClassPos)].m_class != pLevelStats->m_nClass)
        {
            nClassPos++;
            if (m_numClasses <= static_cast<unsigned char>(nClassPos))
                goto LNoClass;
        }
        SetClassLevel(static_cast<unsigned char>(nClassPos), m_classInfo[static_cast<unsigned char>(nClassPos)].m_level - 1);
        goto LClassDone;
    LNoClass:
        if (static_cast<unsigned char>(nClassPos) == 0xff && m_numClasses <= static_cast<unsigned char>(nClassPos))
        {
            m_numClasses--;
            nClassPos = 0xff;
        }
    LClassDone:;
    }

    m_pOwner->m_hitPoints = static_cast<short>(m_pOwner->m_hitPoints) - pLevelStats->m_nHitPointGain;

    switch (pLevelStats->m_nAbilityScoreIncrease)
    {
    case ABILITY_STRENGTH:
        m_nSTRBase--;
        UpdateSTRModifier();
        break;
    case ABILITY_DEXTERITY:
        m_nDEXBase--;
        UpdateDEXModifier();
        break;
    case ABILITY_CONSTITUTION:
        m_nCONBase--;
        UpdateCONModifier();
        break;
    case ABILITY_INTELLIGENCE:
        m_nINTBase--;
        UpdateINTModifier();
        AdjustSpellUsesPerDay();
        break;
    case ABILITY_WISDOM:
        m_nWISBase--;
        UpdateWISModifier();
        AdjustSpellUsesPerDay();
        break;
    case ABILITY_CHARISMA:
        m_nCHABase--;
        UpdateCHAModifier();
        AdjustSpellUsesPerDay();
        break;
    }

    if (g_pRules->m_nNumSkills != 0)
    {
        for (int64_t i = 0; i < g_pRules->m_nNumSkills; i++)
        {
            if (pLevelStats->GetSkillRankChange(static_cast<unsigned short>(i)) != 0)
            {
#if KOTOR1
                int nRank = GetSkillRank(static_cast<unsigned char>(i), 0, 1);
#else
                int nRank = 0;
                if (i < g_pRules->m_nNumSkills)
                {
                    nRank = m_pSkillRanks[i];
                    if (nRank > 127)
                        nRank = 127;
                    if (nRank < -127)
                        nRank = -127;
                }
#endif
                SetSkillRank(static_cast<unsigned char>(i), static_cast<char>(nRank - pLevelStats->GetSkillRankChange(static_cast<unsigned short>(i))));
            }
        }
    }

    m_nSkillPoints = 0;

    if (static_cast<unsigned short>(pLevelStats->m_featsGained.m_count) != 0)
        for (unsigned char i = 0; i < static_cast<unsigned short>(pLevelStats->m_featsGained.m_count); i++)
            RemoveFeat(pLevelStats->m_featsGained.m_pArray[i]);

    if (static_cast<unsigned char>(pLevelStats->m_spellsGained.m_count) != 0)
        for (int64_t i = 0; i < static_cast<unsigned char>(pLevelStats->m_spellsGained.m_count); i++)
            RemoveKnownSpell(static_cast<unsigned char>(nClassPos), pLevelStats->m_spellsGained.m_pArray[i]);

    if (static_cast<unsigned char>(pLevelStats->m_spellsLost.m_count) != 0)
        for (int64_t i = 0; i < static_cast<unsigned char>(pLevelStats->m_spellsLost.m_count); i++)
            AddKnownSpell(static_cast<unsigned char>(nClassPos), pLevelStats->m_spellsLost.m_pArray[i]);

    m_levelStats.Remove(pLevelStats);
    if (pLevelStats != 0)
        delete pLevelStats;

    for (int i = 0; i < NUM_INVENTORY_SLOTS; i++)
    {
        unsigned int nSlot = 1 << i;
        CSWSItem* pItem = static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(nSlot));
        if (pItem != 0)
        {
            if (m_pOwner->CanEquipItem(pItem, &nSlot, 0, 0, 1) == CSWSCreature::CAN_EQUIP_NO)
                m_pOwner->UnequipItem(pItem);
        }
    }

    short nNewMaxHP = m_pOwner->GetMaxHitPoints(1);
    m_pOwner->SetCurrentHitPoints(nNewMaxHP - nOldMaxHP + m_pOwner->GetCurrentHitPoints(1));
}

// Gates level-up; the banked XP must reach the next level's requirement.
int CSWSCreatureStats::CanLevelUp()
{
    if (GetLevel(0) < g_pAppManager->m_pServerExoApp->GetServerInfo()->m_nMaxCharacterLevel)
    {
        if (static_cast<unsigned int>(m_nExperience) >= static_cast<unsigned int>(g_pRules->m_aExpTable[GetLevel(0)]))
        {
            if (m_pOwner->GetDead() == 0 && m_pOwner->GetDeadTemp() == 0)
                return 1;
        }
    }
    return 0;
}

// Applies one pending CSWLevelStats to the live block.
void CSWSCreatureStats::LevelUp(CSWLevelStats* pLevelStats, int bAllowRecalc)
{
    m_pOwner->GetMaxHitPoints(1);
    int nOldMaxForcePoints = m_pOwner->GetMaxForcePoints();

    if (bAllowRecalc != 0)
        m_levelStats.Add(pLevelStats);

    unsigned char nClassPos = 0;
    if (m_numClasses != 0)
    {
        int bNotFound = 1;
#if KOTOR1
        for (unsigned char i = 0; i < m_numClasses; i++)
#else
        for (int64_t i = 0; i < m_numClasses; i++)
#endif
        {
            if (m_classInfo[i].m_class == pLevelStats->m_nClass)
            {
                m_classInfo[i].m_level++;
#if !KOTOR1
                if (m_pOwner != 0 && m_pOwner->m_bPlayerMadeCharacter != 0)
                {
                    if (GetLevel(0) >= 30)
                        Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_LEVEL_30);
                }
#endif
                bNotFound = 0;
                nClassPos = static_cast<unsigned char>(i);
            }
        }
        if (bNotFound != 0)
        {
            nClassPos = m_numClasses;
            unsigned int nNewCount = m_numClasses + 1;
            m_numClasses = static_cast<unsigned char>(nNewCount);
            if (nNewCount > 0xff)
            {
                nClassPos = 0xff;
                goto LClassDone;
            }
        }
        else
        {
            goto LClassDone;
        }
    }
    else
    {
        nClassPos = 0;
        m_numClasses = 1;
    }
    m_classInfo[nClassPos].m_class = pLevelStats->m_nClass;
#if KOTOR1
    m_classInfo[nClassPos].m_level = 1;
#else
    SetClassLevel(nClassPos, 1);
#endif
LClassDone:;

    m_pOwner->m_hitPoints = static_cast<short>(m_pOwner->m_hitPoints) + pLevelStats->m_nHitPointGain;
    m_nBaseForcePoints += pLevelStats->m_nForcePointGain;

    switch (pLevelStats->m_nAbilityScoreIncrease)
    {
    case ABILITY_STRENGTH:
        m_nSTRBase++;
        UpdateSTRModifier();
        break;
    case ABILITY_DEXTERITY:
        m_nDEXBase++;
        UpdateDEXModifier();
        break;
    case ABILITY_CONSTITUTION:
        m_nCONBase++;
        UpdateCONModifier();
        break;
    case ABILITY_INTELLIGENCE:
        m_nINTBase++;
        UpdateINTModifier();
        AdjustSpellUsesPerDay();
        break;
    case ABILITY_WISDOM:
        m_nWISBase++;
        UpdateWISModifier();
        AdjustSpellUsesPerDay();
        break;
    case ABILITY_CHARISMA:
        m_nCHABase++;
        UpdateCHAModifier();
        AdjustSpellUsesPerDay();
        break;
    }

    if (g_pRules->m_nNumSkills != 0)
    {
        for (int64_t i = 0; i < g_pRules->m_nNumSkills; i++)
        {
            if (pLevelStats->GetSkillRankChange(static_cast<unsigned short>(i)) != 0)
            {
#if KOTOR1
                int nRank = GetSkillRank(static_cast<unsigned char>(i), 0, 1);
#else
                int nRank = 0;
                if (i < g_pRules->m_nNumSkills)
                {
                    nRank = m_pSkillRanks[i];
                    if (nRank > 127)
                        nRank = 127;
                    if (nRank < -127)
                        nRank = -127;
                }
#endif
                SetSkillRank(static_cast<unsigned char>(i), static_cast<char>(pLevelStats->GetSkillRankChange(static_cast<unsigned short>(i)) + nRank));
            }
        }
    }

    m_nSkillPoints = pLevelStats->m_nSkillPoints;

    if (static_cast<unsigned short>(pLevelStats->m_featsGained.m_count) != 0)
        for (unsigned char i = 0; i < static_cast<unsigned short>(pLevelStats->m_featsGained.m_count); i++)
            AddFeat(pLevelStats->m_featsGained.m_pArray[i]);

    if (static_cast<unsigned char>(pLevelStats->m_spellsLost.m_count) != 0)
        for (int64_t i = 0; i < static_cast<unsigned char>(pLevelStats->m_spellsLost.m_count); i++)
            RemoveKnownSpell(nClassPos, pLevelStats->m_spellsLost.m_pArray[i]);

    if (static_cast<unsigned char>(pLevelStats->m_spellsGained.m_count) != 0)
        for (int64_t i = 0; i < static_cast<unsigned char>(pLevelStats->m_spellsGained.m_count); i++)
            AddKnownSpell(nClassPos, pLevelStats->m_spellsGained.m_pArray[i]);

    m_pOwner->SetCurrentHitPoints(m_pOwner->GetMaxHitPoints(1));
    m_nCurrentForcePoints += static_cast<unsigned short>(m_pOwner->GetMaxForcePoints() - nOldMaxForcePoints);
}

// K1 loads the XP-table entry before m_nExperience.
unsigned char CSWSCreatureStats::GetPotentialLevel()
{
    unsigned char nLevel = GetLevel(0);
    if (m_bIsPC != 1)
        return nLevel;
    while (static_cast<unsigned int>(m_nExperience) >= static_cast<unsigned int>(g_pRules->m_aExpTable[nLevel])
           && g_pAppManager->m_pServerExoApp->GetServerInfo()->m_nMaxCharacterLevel > nLevel)
        nLevel++;
    return nLevel;
}

// Returns the effective challenge rating, the party-adjusted level for a party member, else
// GetLevel(0).
unsigned char CSWSCreatureStats::GetEffectiveCRForPotentialLevel()
{
    unsigned char nLevel = GetLevel(0);
    int nExtraLevels = 0;
    int nResult = nLevel;
    if (m_bIsPC == 1)
    {
        while (static_cast<unsigned int>(m_nExperience) >= static_cast<unsigned int>(g_pRules->m_aExpTable[nLevel])
               && g_pAppManager->m_pServerExoApp->GetServerInfo()->m_nMaxCharacterLevel > nLevel)
        {
            nLevel++;
            nExtraLevels++;
        }
        if (g_pRules != 0)
        {
            nResult = 0;
            for (int i = 0; i < m_numClasses; i++)
            {
                CSWClass* pClass = &g_pRules->m_paClasses[GetClass(static_cast<unsigned char>(i))];
                unsigned char nCR = GetClassLevel(static_cast<unsigned char>(i), 0);
#if KOTOR1
                if (pClass != 0 && static_cast<unsigned char>(nCR - 2) < 19)
#else
                if (pClass != 0 && static_cast<unsigned char>(nCR - 2) < 49)
#endif
                    nCR = pClass->m_anEffCRByLevel[nCR];
                nResult += nCR;
            }
        }
    }
    return static_cast<unsigned char>(nExtraLevels + nResult);
}

// The per-field-bitmask snapshot writer for the server-to-client last-update object.
void CSWSCreatureStats::UpdateLastStatsObject(unsigned long long nUpdateFields,
                                              CSWCreatureStatsUpdate* pUpdate)
{
    if (nUpdateFields & STATS_UPDATE_ABILITIES)
    {
        pUpdate->m_anAbilityStats[0] = GetSTRStat();
        pUpdate->m_anAbilityStats[1] = GetDEXStat();
        pUpdate->m_anAbilityStats[2] = GetCONStat();
        pUpdate->m_anAbilityStats[3] = GetINTStat();
        pUpdate->m_anAbilityStats[4] = GetWISStat();
        pUpdate->m_anAbilityStats[5] = GetCHAStat();
        pUpdate->m_anAbilityBases[0] = m_nSTRBase;
        pUpdate->m_anAbilityBases[1] = m_nDEXBase;
        pUpdate->m_anAbilityBases[2] = m_nCONBase;
        pUpdate->m_anAbilityBases[3] = m_nINTBase;
        pUpdate->m_anAbilityBases[4] = m_nWISBase;
        pUpdate->m_anAbilityBases[5] = m_nCHABase;
        int nTmp;
        nTmp = 0;
        pUpdate->m_anAbilityEffectBonuses[0] =
            static_cast<unsigned char>(m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_ABILITY,
                                                                     NULL, nTmp, 0, 0, 0, 0, CSWSCreature::EFFECT_BONUS_ANY, 0, 0, NULL));
        nTmp = 0;
        pUpdate->m_anAbilityEffectBonuses[1] =
            static_cast<unsigned char>(m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_ABILITY,
                                                                     NULL, nTmp, 0, 0, 0, 0, CSWSCreature::EFFECT_BONUS_ANY, 1, 0, NULL));
        nTmp = 0;
        pUpdate->m_anAbilityEffectBonuses[2] =
            static_cast<unsigned char>(m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_ABILITY,
                                                                     NULL, nTmp, 0, 0, 0, 0, CSWSCreature::EFFECT_BONUS_ANY, 2, 0, NULL));
        nTmp = 0;
        pUpdate->m_anAbilityEffectBonuses[3] =
            static_cast<unsigned char>(m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_ABILITY,
                                                                     NULL, nTmp, 0, 0, 0, 0, CSWSCreature::EFFECT_BONUS_ANY, 3, 0, NULL));
        nTmp = 0;
        pUpdate->m_anAbilityEffectBonuses[4] =
            static_cast<unsigned char>(m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_ABILITY,
                                                                     NULL, nTmp, 0, 0, 0, 0, CSWSCreature::EFFECT_BONUS_ANY, 4, 0, NULL));
        nTmp = 0;
        pUpdate->m_anAbilityEffectBonuses[5] =
            static_cast<unsigned char>(m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_ABILITY,
                                                                     NULL, nTmp, 0, 0, 0, 0, CSWSCreature::EFFECT_BONUS_ANY, 5, 0, NULL));
        int nDexAC = GetDEXMod(1);
        int bDebilitated = m_pOwner->IsDebilitated(1);
        pUpdate->m_nDexACBonus =
            static_cast<char>(nDexAC - ((m_nDEXMod < 0) ? m_nDEXMod : (bDebilitated ? 0 : m_nDEXMod)));
    }
    if (nUpdateFields & STATS_UPDATE_FORT_SAVE)
    {
        int nTmp = 0;
        char nEffectBonus =
            static_cast<char>(m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_SAVING_THROW, NULL,
                                                            nTmp, 0, 0, 1, 0, CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, 0, NULL));
        pUpdate->m_nFortSave = m_nCONMod + nEffectBonus
            + GetBaseFortSavingThrow() + m_nFortSaveBonus;
    }
    if (nUpdateFields & STATS_UPDATE_WILL_SAVE)
    {
        int nTmp = 0;
        char nEffectBonus =
            static_cast<char>(m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_SAVING_THROW, NULL,
                                                            nTmp, 0, 0, 3, 0, CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, 0, NULL));
        pUpdate->m_nWillSave = m_nWISMod + nEffectBonus
            + GetBaseWillSavingThrow() + m_nWillSaveBonus;
    }
    if (nUpdateFields & STATS_UPDATE_REFLEX_SAVE)
    {
        int nTmp = 0;
        int nEffectBonus =
            m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_SAVING_THROW, NULL, nTmp, 0, 0, 2, 0,
                                          CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, 0, NULL);
        int bDebilitated = m_pOwner->IsDebilitated(1);
        char nDexMod = (m_nDEXMod < 0) ? m_nDEXMod : (bDebilitated ? 0 : m_nDEXMod);
        pUpdate->m_nReflexSave = static_cast<unsigned char>(nEffectBonus
            + GetBaseReflexSavingThrow() + nDexMod + m_nReflexSaveBonus);
    }
    if (nUpdateFields & STATS_UPDATE_EXPERIENCE)
    {
        pUpdate->m_nExperience = m_nExperience;
    }
    if (nUpdateFields & STATS_UPDATE_BASE_ATTACK_BONUS)
    {
        pUpdate->m_nBaseAttackBonus = static_cast<unsigned char>(GetBaseAttackBonus());
        if (m_pOwner->GetUseMonkAbilities())
            pUpdate->m_bMonkUnarmed =
                    (m_pOwner->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON) == NULL);
        else
            pUpdate->m_bMonkUnarmed = 0;
    }
    if (nUpdateFields & STATS_UPDATE_COMBAT_INFORMATION)
    {
        pUpdate->SetCombatInformation(m_pCombatInformation);
    }
    if (nUpdateFields & STATS_UPDATE_ARMOR_CLASS)
    {
        pUpdate->m_nArmorClass = m_pOwner->GetArmorClass();
    }
    if (nUpdateFields & STATS_UPDATE_SKILLS)
    {
        if (g_pRules->m_nNumSkills != 0)
        {
            for (unsigned char i = 0; i < g_pRules->m_nNumSkills; i++)
                pUpdate->m_paSkillRanks[i] = GetSkillRank(i, NULL, 0);
        }
    }
    if (nUpdateFields & STATS_UPDATE_FEATS)
    {
        pUpdate->m_objectId = m_pOwner->m_objectId;
        pUpdate->m_feats.m_count = 0;
        for (int i = 0; i < static_cast<unsigned short>(m_feats.m_count); i++)
            pUpdate->m_feats.Add(GetFeat(static_cast<unsigned short>(i)));
        pUpdate->m_bonusFeats.m_count = 0;
        for (int i = 0; i < static_cast<unsigned short>(m_bonusFeats.m_count); i++)
            pUpdate->m_bonusFeats.Add(GetBonusFeat(static_cast<unsigned short>(i)));
    }
}

// Returns the total character level, the per-class levels summed.
unsigned char CSWSCreatureStats::GetLevel(int bApplyNegativeLevels)
{
    int nLevel = 0;
#if KOTOR1
// K1: the class sum only, through the owner's GetClassLevel.
    for (int i = 0; i < m_numClasses; i++)
        nLevel += m_pOwner->m_pStats->GetClassLevel(static_cast<unsigned char>(i), bApplyNegativeLevels);
#else
    if (m_numClasses != 0)
    {
        for (int i = 0; i < m_numClasses; i++)
        {
            if (m_pOwner->m_pStats->m_numClasses > static_cast<unsigned char>(i))
            {
                int nClassLevel = m_pOwner->m_pStats->m_classInfo[i].m_level;
                if (bApplyNegativeLevels != 0)
                    nClassLevel -= m_pOwner->m_pStats->m_classInfo[i].m_negativeLevels;
                if (nClassLevel > 0)
                    nLevel += static_cast<unsigned char>(nClassLevel);
            }
        }
    }
#endif
#if !KOTOR1
    if (m_pOwner != 0 && m_pOwner->m_pStats->m_bIsPC == 0 && m_pOwner->m_bInParty == 0 &&
        m_pOwner->m_nMultiplierSet != 0)
    {
        float fMult;
        g_pRules->m_p2DArrays->m_pAutoBalance->GetFLOATEntry(
            m_pOwner->m_nMultiplierSet, CExoString("LevelMult"), &fMult);
        char nAutoBalanceLevel = m_pOwner->m_nPCLevelAtSpawn;
        if (nAutoBalanceLevel == 0)
        {
            g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->GetValueNumber(
                CExoString("G_PC_LEVEL"), nAutoBalanceLevel);
        }
        nLevel += static_cast<int>(fMult * static_cast<float>((nAutoBalanceLevel > 1) ? nAutoBalanceLevel : 1)) - 1;
        if (nLevel < 0)
            nLevel = 1;
    }
#endif
    return static_cast<unsigned char>(nLevel);
}

// Maps an effect id to its effects.2da row id.
int CSWSCreatureStats::EffectId2Effect2DAId(int nEffectId, int nSubType)
{
    switch (nEffectId)
    {
    case GAME_EFFECT_SETSTATE:
        switch (nSubType)
        {
        case EFFECT_STATE_CONFUSED: return 22;
        case EFFECT_STATE_FRIGHTENED: return 24;
        case EFFECT_STATE_DROID_STUN: return 21;
        case EFFECT_STATE_STUNNED: return 19;
        case EFFECT_STATE_PARALYZED: return 18;
        case EFFECT_STATE_SLEEP: return 23;
        case EFFECT_STATE_CHOKE: return 15;
        case EFFECT_STATE_HORRIFIED: return 13;
#if !KOTOR1  // K1: eight subtypes (no 18/19 entries)
        case EFFECT_STATE_MIND_TRICK: return 26;
        case EFFECT_STATE_DROID_SCRAMBLE: return 25;
#endif
        }
        return 0;
    case GAME_EFFECT_ENTANGLE:
        return 1;
    case GAME_EFFECT_DAMAGE_IMMUNITY_DECREASE:
        return 6;
    case GAME_EFFECT_ABILITY_DECREASE:
        return 3;
    case GAME_EFFECT_SKILL_DECREASE:
        return 11;
    case GAME_EFFECT_BLASTERDEFLECTIONDECREASE:
        return 12;
    case GAME_EFFECT_LIGHTSABERTHROW:
        return 17;
    case GAME_EFFECT_ATTACK_DECREASE:
        return 4;
    case GAME_EFFECT_DAMAGE_DECREASE:
        return 5;
    case GAME_EFFECT_DEATH:
        return 20;
    case GAME_EFFECT_SAVING_THROW_DECREASE:
        return 9;
    case GAME_EFFECT_MOVEMENT_SPEED_DECREASE:
        return 8;
    case GAME_EFFECT_FORCE_RESISTANCE_DECREASE:
        return 10;
    case GAME_EFFECT_POISON:
        return 2;
    case GAME_EFFECT_AC_DECREASE:
        return 7;
    case GAME_EFFECT_HORRIFIED:
        return 13;
    case GAME_EFFECT_DAMAGEFORCEPOINTS:
        return 14;
    case GAME_EFFECT_CHOKE:
        return 15;
    case GAME_EFFECT_PSYCHICSTATIC:
        return 16;
    }
    return 0;
}

// Splits a feat into the last class's normal and bonus lists.
void CSWSCreatureStats::SetNormalBonusFlags(unsigned short nFeat, int& bNormal, int& bBonus)
{
    bNormal = 0;
    bBonus = 0;
    if (g_pRules->GetFeat(nFeat) == 0)
        return;
    CSWRules* pRules = g_pRules;
    unsigned char nClassId = GetClass(static_cast<unsigned char>(m_numClasses - 1));
    CSWClass* pClass = &pRules->m_paClasses[nClassId];
    if (pClass == 0)
        return;
    if (pClass->IsNormalFeat(nFeat) != 0)
        bNormal = 1;
    if (pClass->IsBonusFeat(nFeat) != 0)
        bBonus = 1;
}

// Confirms a level-up, refusing when CanLevelUp() says no.
int CSWSCreatureStats::ValidateLevelUp(CSWLevelStats* pLevelStats)
{
    if (!CanLevelUp())
        return 0;
    if (m_bIsPC != 0) {
        CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
        if (pModule != 0) {
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_LEVEL_UP;
            CServerAIMaster* pAI = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
            pAI->AddEventAbsoluteTime(0, 0, m_pOwner->m_objectId, pModule->m_objectId,
                                      CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        }
    }
    LevelUp(pLevelStats, 1);
    UpdateCombatInformation();
    return 1;
}

// Random-talent probe over the creature's class slots.
int CSWSCreatureStats::GetCreatureHasTalent(int nType, int nId)
{
    if (nType == -1)
        return 0;
    if (nId == -1)
        return 0;

    CSWSCreature* pCreature = m_pOwner;
    int bDead = 0;
    if ((~pCreature->m_nAIState & 0x6) != 0)
    {
        if (pCreature->GetDead() != 0)
            bDead = 1;
        else
            pCreature = m_pOwner;
    }
#if KOTOR1  // K1 asks GetIsPCDying() and GetDeadTemp() instead
    if (bDead != 0 || pCreature->GetIsPCDying() != 0 || m_pOwner->GetDeadTemp() != 0)
#else
    if (bDead != 0 || pCreature->GetDeadTemp() != 0)
#endif
    {
        if (nType == 0)
            g_pRules->GetSpell(nId);
        return 0;
    }

    if (nType == 0)
    {
        CSWSpell* pSpell = g_pRules->GetSpell(nId);
        if (pSpell == 0)
            return 0;
        if (m_numClasses == 0)
            return 0;

        int bHas = 0;
#if KOTOR1
        for (unsigned char i = 0; i < m_numClasses; i++)
#else
        for (int64_t i = 0; i < m_numClasses; i++)
#endif
        {
            if (CSWClass::IsJedi(m_classInfo[i].m_class) != 0)
            {
                if (HasSpell(0, nId, 1) != 0)
                {
                    char nUsesLeft = GetSpellUsesLeft(nId, static_cast<unsigned char>(i), 0, 0);
                    if (nUsesLeft != 0)
                        bHas = 1;
                }
            }
            else
            {
                int bFound = 0;
                int64_t nSpellLikes = m_pSpellLikeAbilityList->m_count;
                CSWSStats_SpellLikeAbility* pSpellLike = m_pSpellLikeAbilityList->m_pArray;
                if (nSpellLikes > 0)
                {
                    do
                    {
                        if (pSpellLike->m_nSpellId == static_cast<unsigned int>(nId) && pSpellLike->m_nReady == 1)
                        {
                            bFound = 1;
                            break;
                        }
                        pSpellLike++;
                    } while (--nSpellLikes != 0);
                }
                if (bFound != 0)
                {
                    bHas = 1;
                }
                else
                {
                    char nUsesLeft = 0;
                    if (HasSpell(0, nId, 1) != 0)
                        nUsesLeft = GetSpellUsesLeft(nId, static_cast<unsigned char>(i), 0, 0);
                    if (nUsesLeft != 0)
                        bHas = 1;
                    else if (GetHasPowerFromItem(nId) != 0)
                        bHas = 1;
                }
            }
        }
        if (bHas == 0)
            return 0;
        if ((static_cast<unsigned char>(m_pOwner->m_nAIState) & AI_STATE_CAN_SPEAK) == 0)
            return 0;
        return 1;
    }
    if (nType == 2)
    {
        if (GetSkillRank(static_cast<unsigned char>(nId), 0, 0) > 0)
            return 1;
        return (g_pRules->m_paSkills[static_cast<unsigned short>(nId)].m_flags & CSWSkill::FLAG_UNTRAINED) >> 1;
    }
    if (nType == 1)
        return HasFeat(static_cast<unsigned short>(nId)) && GetFeatRemainingUses(static_cast<unsigned short>(nId)) != 0;
    return 0;
}

// The talent-dice dispatcher over the four GetCreatureTalentRandom* workers.
int CSWSCreatureStats::GetCreatureTalentRandom(int a1, int a2, int& a3, int& a4, unsigned char& a5,
                                               unsigned long long& a6, int& a7, unsigned char& a8,
                                               int a9, int a10, int a11)
{
    int bNoList = (a11 == -1);
    a3 = -1;
    a4 = -1;
    a5 = CSWSObject::CAST_CLASS_NONE;
    a6 = OBJECT_INVALID;
    a7 = -1;
    a8 = 0xff;
    int nFeatList = (a10 == 1) ? a11 : -1;
    int nPowerList = (a10 == 0) ? a11 : -1;
    int bBoth = (a10 == 0 && bNoList);

    int bPowerDone = 0;
    int bFeatDone = 0;
    for (;;)
    {
        if (bFeatDone && bPowerDone)
        {
            a3 = -1;
            a4 = -1;
            return 0;
        }
        int nRoll = rand_wincompatible() % 2;
        if (nRoll == 0)
        {
            if (bPowerDone)
                continue;
            if (GetCreatureTalentRandomSpecialAbility(a1, a2, a3, a4, a5, a8, a9, nPowerList))
                return 1;
            if (GetCreatureTalentRandomPower(a1, a2, a3, a4, a5, a9, nPowerList))
                return 1;
            int nItemResult = GetCreatureTalentRandomPowerFromItem(a1, a2, a3, a4, a5, a6, a7,
                                                                   a9, nPowerList);
            bPowerDone = bBoth | (nItemResult == 0);
            if (nItemResult != 0)
                return 1;
        }
        else if (!bFeatDone && nRoll == 1)
        {
            if (a10 == 1 && bNoList)
                bPowerDone = 1;
            int nFeatResult = GetCreatureTalentRandomFeat(a1, a2, a3, a4, a5, a9, nFeatList);
            bFeatDone = (nFeatResult == 0);
            if (nFeatResult != 0)
                return 1;
        }
    }
}

// Stops at the first class slot whose KnownPower call reports a hit.
int CSWSCreatureStats::GetCreatureTalentRandomPower(int a1, int a2, int& a3, int& a4,
                                                    unsigned char& a5, int a6, int a7)
{
    K2_UNUSED(a6);
    int nResult = 0;
    for (int i = 0; i < m_numClasses; i++) {
        if (CSWClass::IsJedi(GetClass(static_cast<unsigned char>(i))) == 0)
            continue;
        nResult = GetCreatureTalentRandomKnownPower(a1, a2, a3, a4, a5, i, a7, a7);
        if (nResult == 1)
            return 1;
    }
    return nResult;
}

// Picks a random ready, usable spell-like ability off the spell-like-ability list.
int CSWSCreatureStats::GetCreatureTalentRandomSpecialAbility(int a1, int a2, int& a3, int& a4,
                                                             unsigned char& a5, unsigned char& a6,
                                                             int a7, int a8)
{
    K2_UNUSED(a8);
    CExoArrayList<int>* pSpells = new CExoArrayList<int>;
    CExoArrayList<int>* pLevels = new CExoArrayList<int>;
    int nBestCR = -100;
    for (int64_t i = 0; i < m_pSpellLikeAbilityList->m_count; i++) {
        if (m_pSpellLikeAbilityList->m_pArray[i].m_nReady == 0)
            continue;
        int nSpellId = m_pSpellLikeAbilityList->m_pArray[i].m_nSpellId;
        unsigned char nCasterLevel = m_pSpellLikeAbilityList->m_pArray[i].m_nCasterLevel;
        CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
        if (pSpell == 0)
            continue;
        if (CheckPowerSuitability(nSpellId, pSpell, a1, 0, 0, 0, 0, -1) != 1)
            continue;
        if (CheckInclusionMatch(0, nSpellId, a7) != 1)
            continue;
        if (a2 == -1) {
            pSpells->Add(nSpellId);
            pLevels->Add(nCasterLevel);
            pSpell->m_sName.CStr();
        } else if (pSpell->m_nMaxCR > nBestCR) {
            delete pSpells;
            pSpells = new CExoArrayList<int>;
            pSpells->Add(nSpellId);
            delete pLevels;
            pLevels = new CExoArrayList<int>;
            pLevels->Add(nCasterLevel);
            pSpell->m_sName.CStr();
            nBestCR = pSpell->m_nMaxCR;
        } else if (pSpell->m_nMaxCR == nBestCR) {
            pSpells->Add(nSpellId);
            pLevels->Add(nCasterLevel);
            pSpell->m_sName.CStr();
        }
    }
    int nFound = 0;
    if (pSpells->m_count > 0) {
        int nRoll = rand_wincompatible();
        a3 = 0;
        a4 = pSpells->m_pArray[nRoll % pSpells->m_count];
        a6 = static_cast<unsigned char>(pLevels->m_pArray[nRoll % pSpells->m_count]);
        a5 = CSWSObject::CAST_CLASS_SPELL_LIKE_ABILITY;
        nFound = 1;
    }
    delete pSpells;
    delete pLevels;
    return nFound;
}

// Picks a random item-granted cast-spell power the creature could use.
int CSWSCreatureStats::GetCreatureTalentRandomPowerFromItem(int a1, int a2, int& a3, int& a4,
                                                            unsigned char& a5,
                                                            unsigned long long& a6, int& a7,
                                                            int a8, int a9)
{
    K2_UNUSED(a5); K2_UNUSED(a9);
    CExoArrayList<int>* pSpells = new CExoArrayList<int>;
    CExoArrayList<unsigned long long>* pItems = new CExoArrayList<unsigned long long>;
    CExoArrayList<int>* pProps = new CExoArrayList<int>;
    int nBestCR = -100;
    if (m_pOwner->m_pInventory != 0)
    {
        for (int slot = 0; slot < NUM_INVENTORY_SLOTS; slot++)
        {
            CSWSItem* pItem = static_cast<CSWSItem*>(m_pOwner->m_pInventory->GetItemInSlot(1 << slot));
            if (pItem != 0 && pItem->GetPropertyByTypeExists(ITEM_PROPERTY_ACTIVATE_ITEM, 0) &&
                pItem->m_nActiveProperties >= 1)
            {
                for (int j = 0; j < pItem->m_nActiveProperties; j++)
                {
                    if (pItem->GetActiveProperty(j)->m_type == ITEM_PROPERTY_ACTIVATE_ITEM &&
                        pItem->GetActiveProperty(j)->m_bUseableByte != 0)
                    {
                        unsigned char nSlot = pItem->GetActiveProperty(j)->m_nUpgradeType;
                        if (nSlot != CSWItemProperty::UPGRADE_TYPE_NONE && (pItem->m_nUpgrades & (1 << nSlot)) == 0)
                            continue;
                        unsigned short nSpellId = pItem->GetActiveProperty(j)->m_subType;
                        CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
                        if (pSpell == 0)
                            continue;
                        if (CheckPowerSuitability(nSpellId, pSpell, a1, 0, 0, 0, 0, -1) != 1)
                            continue;
                        if (CheckInclusionMatch(0, nSpellId, a8) != 1)
                            continue;
                        if (a2 == -1) {
                            pSpells->Add(nSpellId);
                            pItems->Add(pItem->m_objectId);
                            pProps->Add(j);
                        } else if (pSpell->m_nMaxCR > nBestCR) {
                            delete pSpells;
                            delete pItems;
                            delete pProps;
                            pSpells = new CExoArrayList<int>;
                            pItems = new CExoArrayList<unsigned long long>;
                            pProps = new CExoArrayList<int>;
                            pSpells->Add(nSpellId);
                            pItems->Add(pItem->m_objectId);
                            pProps->Add(j);
                            nBestCR = pSpell->m_nMaxCR;
                        } else if (pSpell->m_nMaxCR == nBestCR) {
                            pSpells->Add(nSpellId);
                            pItems->Add(pItem->m_objectId);
                            pProps->Add(j);
                        }
                    }
                }
            }
        }
    }
    CItemRepository* pRepo = m_pOwner->GetItemRepository(1);
    if (pRepo != 0 && pRepo->ItemListGetCount() != 0)
    {
        for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++)
        {
            CSWSItem* pItem = pRepo->ItemListGetItem(i);
            if (pItem == 0)
                continue;
            if (g_pRules->GetBaseItem(pItem->m_baseItem)->m_nEquipableSlots != 0)
                continue;
            if (!pItem->GetPropertyByTypeExists(ITEM_PROPERTY_ACTIVATE_ITEM, 0))
                continue;
            if (pItem->m_nActiveProperties < 1)
                continue;
            for (int j = 0; j < pItem->m_nActiveProperties; j++)
            {
                if (pItem->GetActiveProperty(j)->m_type != ITEM_PROPERTY_ACTIVATE_ITEM)
                    continue;
                if (pItem->GetActiveProperty(j)->m_bUseableByte == 0 &&
                    pItem->m_nCharges < 1)
                    continue;
                unsigned char nSlot = pItem->GetActiveProperty(j)->m_nUpgradeType;
                if (nSlot != CSWItemProperty::UPGRADE_TYPE_NONE && (pItem->m_nUpgrades & (1 << nSlot)) == 0)
                    continue;
                unsigned short nSpellId = pItem->GetActiveProperty(j)->m_subType;
                CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
                if (pSpell == 0)
                    continue;
                int nFlags = pSpell->m_nCategory;
                int bFail0 = ((a1 & TALENT_CATEGORY_DIGIT_0) != TALENT_CATEGORY_DIGIT_0) & ((a1 & TALENT_CATEGORY_DIGIT_0) != (nFlags & TALENT_CATEGORY_DIGIT_0));
                int bFail1 = ((a1 & TALENT_CATEGORY_DIGIT_1) != TALENT_CATEGORY_DIGIT_1) & ((a1 & TALENT_CATEGORY_DIGIT_1) != (nFlags & TALENT_CATEGORY_DIGIT_1));
                int bFail2 = ((a1 & TALENT_CATEGORY_DIGIT_2) != TALENT_CATEGORY_DIGIT_2) & ((a1 & TALENT_CATEGORY_DIGIT_2) != (nFlags & TALENT_CATEGORY_DIGIT_2));
                int bFail3 = ((a1 & TALENT_CATEGORY_DIGIT_3) != TALENT_CATEGORY_DIGIT_3) & ((a1 & TALENT_CATEGORY_DIGIT_3) != (nFlags & TALENT_CATEGORY_DIGIT_3));
                if (bFail0)
                    continue;
                if (bFail1)
                    continue;
                if (bFail2)
                    continue;
                if (bFail3)
                    continue;
                if (a2 == -1) {
                    pSpells->Add(nSpellId);
                    pItems->Add(pItem->m_objectId);
                    pProps->Add(j);
                } else if (pSpell->m_nMaxCR > nBestCR) {
                    delete pSpells;
                    delete pItems;
                    delete pProps;
                    pSpells = new CExoArrayList<int>;
                    pItems = new CExoArrayList<unsigned long long>;
                    pProps = new CExoArrayList<int>;
                    pSpells->Add(nSpellId);
                    pItems->Add(pItem->m_objectId);
                    pProps->Add(j);
                    nBestCR = pSpell->m_nMaxCR;
                } else if (pSpell->m_nMaxCR == nBestCR) {
                    pSpells->Add(nSpellId);
                    pItems->Add(pItem->m_objectId);
                    pProps->Add(j);
                }
            }
        }
    }
    int nFound = 0;
    if (pSpells->m_count > 0) {
        int nRoll = rand_wincompatible();
        a3 = 0;
        a4 = pSpells->m_pArray[nRoll % pSpells->m_count];
        a6 = pItems->m_pArray[nRoll % pSpells->m_count];
        a7 = pProps->m_pArray[nRoll % pSpells->m_count];
        nFound = 1;
    }
    delete pSpells;
    delete pItems;
    delete pProps;
    return nFound;
}

// Picks a random known feat with uses left whose category passes.
int CSWSCreatureStats::GetCreatureTalentRandomFeat(int a1, int a2, int& a3, int& a4,
                                                   unsigned char& a5, int a6, int a7)
{
    K2_UNUSED(a5);
    CExoArrayList<int>* pFeats = new CExoArrayList<int>;
    int nBestCR = -100;
    for (unsigned short i = 0; i < m_feats.m_count; i++) {
        unsigned short nFeat = m_feats.m_pArray[i];
        CSWFeat* pFeat = g_pRules->GetFeat(nFeat);
        if (pFeat == 0)
            continue;
        int nCategory = pFeat->m_nCategory;
        int bFail0 = ((a1 & TALENT_CATEGORY_DIGIT_0) != TALENT_CATEGORY_DIGIT_0) & ((a1 & TALENT_CATEGORY_DIGIT_0) != (nCategory & TALENT_CATEGORY_DIGIT_0));
        int bFail1 = ((a1 & TALENT_CATEGORY_DIGIT_1) != TALENT_CATEGORY_DIGIT_1) & ((a1 & TALENT_CATEGORY_DIGIT_1) != (nCategory & TALENT_CATEGORY_DIGIT_1));
        int bFail2 = ((a1 & TALENT_CATEGORY_DIGIT_2) != TALENT_CATEGORY_DIGIT_2) & ((a1 & TALENT_CATEGORY_DIGIT_2) != (nCategory & TALENT_CATEGORY_DIGIT_2));
        int bFail3 = ((a1 & TALENT_CATEGORY_DIGIT_3) != TALENT_CATEGORY_DIGIT_3) & ((a1 & TALENT_CATEGORY_DIGIT_3) != (nCategory & TALENT_CATEGORY_DIGIT_3));
        if (bFail0)
            continue;
        if (bFail1)
            continue;
        if (bFail2)
            continue;
        if (bFail3)
            continue;
        if (!CheckInclusionMatch(1, nFeat, a6))
            continue;
        if (nFeat == a7)
            continue;
        if (GetCreatureHasTalent(1, nFeat) == 0)
            continue;
        if (a2 == -1) {
            pFeats->Add(nFeat);
        } else if (pFeat->m_nMaxCR > nBestCR) {
            delete pFeats;
            pFeats = new CExoArrayList<int>;
            pFeats->Add(nFeat);
            nBestCR = pFeat->m_nMaxCR;
        } else if (pFeat->m_nMaxCR == nBestCR) {
            pFeats->Add(nFeat);
        }
    }
    int nFound = 0;
    if (pFeats->m_count > 0) {
        a3 = 1;
        a4 = pFeats->m_pArray[rand_wincompatible() % pFeats->m_count];
        nFound = 1;
    }
    delete pFeats;
    return nFound;
}

// Adds one {spell, ready, caster level} entry to the spell-like-ability list.
void CSWSCreatureStats::AddSpellLikeAbilityToList(unsigned int nSpellId, unsigned char nReady,
                                                  unsigned char nCasterLevel)
{
    CSWSStats_SpellLikeAbility entry;
    entry.m_nSpellId = nSpellId;
    entry.m_nReady = nReady;
    entry.m_nCasterLevel = nCasterLevel;
    m_pSpellLikeAbilityList->Add(entry);
}

// Grants one feat and zeroes its spend counter.
void CSWSCreatureStats::AddFeat(unsigned short nFeat)
{
    CSWFeat* pFeat = g_pRules->GetFeat(nFeat);
    if (pFeat == 0)
        return;
    m_feats.AddUnique(nFeat);
    if (pFeat->m_maxUses != 0)
    {
        unsigned short i = 0;
        while (i < m_featRemainingUses.m_count)
        {
            if (m_featRemainingUses.m_pArray[i++]->m_nFeat == nFeat)
                return;
        }
        CSWSStats_FeatRemainingUses* pEntry = new CSWSStats_FeatRemainingUses;
        pEntry->m_nFeat = nFeat;
        pEntry->m_nUsesUsed = 0;
        m_featRemainingUses.Add(pEntry);
    }
}

// Picks a random known force power of the class slot that passes the Force-Body discount.
int CSWSCreatureStats::GetCreatureTalentRandomKnownPower(int a1, int a2, int& a3, int& a4,
                                                         unsigned char& a5, int nClassIndex,
                                                         int a7, int a8)
{
    K2_UNUSED(a7);
    CExoArrayList<int>* pSpells = new CExoArrayList<int>;
    CExoArrayList<unsigned char>* pClasses = new CExoArrayList<unsigned char>;
    for (unsigned char j = 0; j < GetNumberKnownSpells(static_cast<unsigned char>(nClassIndex), 0); j++) {
        int nSpellId = GetKnownSpell(static_cast<unsigned char>(nClassIndex), 0, j);
        CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
        if (pSpell == 0)
            continue;
#if KOTOR1
// K1: plain cost test.
        int nCost = pSpell->GetAdjustedForcePoints(m_nAlignment, m_pOwner->m_bInParty, 0, 0, 0);
        if (nSpellId == a8)
            continue;
        if (nCost > static_cast<short>(m_nTemporaryForcePoints + m_nCurrentForcePoints))
            continue;
#else
        int nForceRating = 0;
        CSWSArea* pArea = static_cast<CSWSArea*>(m_pOwner->GetArea());
        if (pArea != 0)
            nForceRating = pArea->GetRoomForceRating(m_pOwner->m_position);
        int nForceBodyLevel = CSWSEffectListHandler::FORCE_BODY_LEVEL_NONE;
        for (int64_t k = 0; k < m_pOwner->m_effects.m_count; k++) {
            CGameEffect* pEffect = m_pOwner->m_effects.m_pArray[k];
            if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_FORCE_BODY) {
                nForceBodyLevel = pEffect->GetInteger(0);
                break;
            }
        }
        int nCost = pSpell->GetAdjustedForcePoints(m_nAlignment, m_pOwner->m_bInParty,
                                                   nForceRating, m_pOwner->m_stanceId,
                                                   m_pOwner->m_pStats->m_nCHAMod);
        int nPct = 0;
        switch (nForceBodyLevel) {
        case CSWSEffectListHandler::FORCE_BODY_LEVEL_NONE:
            if (nSpellId == a8)
                continue;
            if (nCost > static_cast<short>(m_nTemporaryForcePoints + m_nCurrentForcePoints))
                continue;
            goto talent;
        case CSWSEffectListHandler::FORCE_BODY_LEVEL_BASIC:    nPct = 50; break;
        case CSWSEffectListHandler::FORCE_BODY_LEVEL_IMPROVED: nPct = 40; break;
        case CSWSEffectListHandler::FORCE_BODY_LEVEL_MASTER:   nPct = 30; break;
        }
        nPct = nPct * nCost / 100;
        if (nPct > static_cast<short>(m_nTemporaryForcePoints + m_nCurrentForcePoints))
            continue;
        if (static_cast<short>(m_pOwner->GetCurrentHitPoints(0)) - nPct <= 1) {
            m_pOwner->GetCurrentHitPoints(0);
            continue;
        }
        if (nSpellId == a8)
            continue;
        goto talent;
    talent:
#endif
        if (GetCreatureHasTalent(0, nSpellId) == 0)
            continue;
        if (m_pOwner->GetArea() != 0) {
            int nFlags = pSpell->m_nCategory;
            int bOk0 = ((a1 & TALENT_CATEGORY_DIGIT_0) == TALENT_CATEGORY_DIGIT_0) | ((a1 & TALENT_CATEGORY_DIGIT_0) == (nFlags & TALENT_CATEGORY_DIGIT_0));
            int bOk1 = ((a1 & TALENT_CATEGORY_DIGIT_1) == TALENT_CATEGORY_DIGIT_1) | ((a1 & TALENT_CATEGORY_DIGIT_1) == (nFlags & TALENT_CATEGORY_DIGIT_1));
            int bOk2 = ((a1 & TALENT_CATEGORY_DIGIT_2) == TALENT_CATEGORY_DIGIT_2) | ((a1 & TALENT_CATEGORY_DIGIT_2) == (nFlags & TALENT_CATEGORY_DIGIT_2));
            int bOk3 = ((a1 & TALENT_CATEGORY_DIGIT_3) == TALENT_CATEGORY_DIGIT_3) | ((a1 & TALENT_CATEGORY_DIGIT_3) == (nFlags & TALENT_CATEGORY_DIGIT_3));
            if (!bOk0)
                continue;
            if (!bOk1)
                continue;
            if (!bOk2)
                continue;
            if (!bOk3)
                continue;
        }
        if (a2 == -1) {
            pSpells->Add(nSpellId);
            pClasses->Add(static_cast<unsigned char>(nClassIndex));
            pSpell->m_sName.CStr();
        } else if (pSpell->m_nMaxCR < -99) {
            if (pSpell->m_nMaxCR == -100) {
                pSpells->Add(nSpellId);
                pClasses->Add(static_cast<unsigned char>(nClassIndex));
                pSpell->m_sName.CStr();
            }
        } else {
            delete pSpells;
            delete pClasses;
            pSpells = new CExoArrayList<int>;
            pSpells->Add(nSpellId);
            pClasses = new CExoArrayList<unsigned char>;
            pClasses->Add(static_cast<unsigned char>(nClassIndex));
            pSpell->m_sName.CStr();
        }
    }
    int nFound = 0;
    if (pSpells->m_count > 0) {
        int nRoll = rand_wincompatible();
        a3 = 0;
        a4 = pSpells->m_pArray[nRoll % pSpells->m_count];
        a5 = pClasses->m_pArray[nRoll % pSpells->m_count];
        nFound = 1;
    }
    delete pSpells;
    delete pClasses;
    return nFound;
}

// Whether the class slot knows the given spell.
int CSWSCreatureStats::GetIsInKnownSpellList(unsigned char classIndex, unsigned int nSpellId)
{
    CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
    if (pSpell == 0)
        return 0;
    unsigned char nLevel = pSpell->GetSpellLevel(GetClass(classIndex));
    if (nLevel == CSWSpell::SPELL_LEVEL_NONE)
        return 0;
    for (int i = 0; i < GetNumberKnownSpells(classIndex, nLevel); i++) {
        if (GetKnownSpell(classIndex, nLevel, static_cast<unsigned char>(i)) == static_cast<int>(nSpellId))
            return 1;
    }
    return 0;
}

// Inclusion-group test for a spell or feat row.
int CSWSCreatureStats::CheckInclusionMatch(int a1, int a2, int a3)
{
    int nExclusion = 0;
    int nMatch;
    if (a1 == 1)
        nMatch = (g_pRules->m_p2DArrays->m_pFeat->GetINTEntry(a2, CExoString("Exclusion"),
                                                              &nExclusion) == 0);
    else if (a1 == 0)
        nMatch = (g_pRules->m_p2DArrays->m_pSpells->GetINTEntry(a2, CExoString("Exclusion"),
                                                                &nExclusion) == 0);
    else
        return 1;
    return nMatch | (nExclusion == 0) | ((nExclusion | a3) == 0);
}

// Affordability test for one casting of the spell by the class slot.
unsigned char CSWSCreatureStats::GetSpellUsesLeft(unsigned int nSpellId, unsigned char classIndex,
                                        unsigned char a3, unsigned char a4)
{
    K2_UNUSED(a3); K2_UNUSED(a4);
    if (classIndex >= m_numClasses)
        return 0;
    CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
    if (pSpell == 0)
        return 0;
#if KOTOR1
// K1: alignment and party cost only.
    int nCost = pSpell->GetAdjustedForcePoints(m_nAlignment, m_pOwner->m_bInParty, 0, 0, 0);
    return static_cast<unsigned int>(nCost) <= static_cast<unsigned int>(static_cast<short>(m_nCurrentForcePoints + m_nTemporaryForcePoints));
#else
    int nForceRating = 0;
    CSWSArea* pArea = static_cast<CSWSArea*>(m_pOwner->GetArea());
    if (pArea != 0)
        nForceRating = pArea->GetRoomForceRating(m_pOwner->m_position);
    int nCost = pSpell->GetAdjustedForcePoints(m_nAlignment, m_pOwner->m_bInParty,
                                               nForceRating, m_pOwner->m_stanceId,
                                               m_pOwner->m_pStats->m_nCHAMod);
    int nForceBodyLevel = CSWSEffectListHandler::FORCE_BODY_LEVEL_NONE;
    CExoArrayList<CGameEffect*>& aEffects = m_pOwner->m_effects;
    for (int64_t k = 0; k < aEffects.m_count; k++) {
        CGameEffect* pEffect = aEffects.m_pArray[k];
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_FORCE_BODY) {
            nForceBodyLevel = pEffect->GetInteger(0);
            break;
        }
    }
    int nPct = 0;
    switch (nForceBodyLevel) {
    case CSWSEffectListHandler::FORCE_BODY_LEVEL_NONE:
        return static_cast<unsigned int>(nCost) <= static_cast<unsigned int>(static_cast<short>(m_nCurrentForcePoints + m_nTemporaryForcePoints));
    case CSWSEffectListHandler::FORCE_BODY_LEVEL_BASIC:    nPct = 50; break;
    case CSWSEffectListHandler::FORCE_BODY_LEVEL_IMPROVED: nPct = 40; break;
    case CSWSEffectListHandler::FORCE_BODY_LEVEL_MASTER:   nPct = 30; break;
    }
    nPct = nPct * nCost / 100;
    if (static_cast<unsigned int>(nCost - nPct) <= static_cast<unsigned int>(static_cast<short>(m_nCurrentForcePoints + m_nTemporaryForcePoints)))
        return nPct < static_cast<short>(m_pOwner->GetCurrentHitPoints(0));
    return 0;
#endif
}

// Whether the creature has the spell.
int CSWSCreatureStats::HasSpell(unsigned char nSpellLevel, unsigned int nSpellId, int a)
{
    int nFound = 0;
    for (unsigned char i = 0; i < m_numClasses; i++) {
        CExoArrayList<unsigned int>* pKnown = &m_classInfo[i].m_knownSpells[nSpellLevel];
        int64_t nCount = static_cast<unsigned short>(pKnown->m_count);
        if (nCount != 0) {
            unsigned int* pEntry = pKnown->m_pArray;
            do {
                if (*pEntry == nSpellId) {
                    nFound = 1;
                    break;
                }
                pEntry++;
            } while (--nCount != 0);
        }
    }
#if KOTOR1
// K1: the affordability test is the plain alignment and party cost.
    if (a != 0 && nFound != 0) {
        CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
        int nCost = pSpell->GetAdjustedForcePoints(m_nAlignment, m_pOwner->m_bInParty, 0, 0, 0);
        return nCost <= static_cast<short>(m_nCurrentForcePoints + m_nTemporaryForcePoints);
    }
#else
    if (a != 0 && nFound != 0) {
        CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
        int nForceRating = 0;
        CSWSArea* pArea = static_cast<CSWSArea*>(m_pOwner->GetArea());
        if (pArea != 0)
            nForceRating = pArea->GetRoomForceRating(m_pOwner->m_position);
        int nForceBodyLevel = CSWSEffectListHandler::FORCE_BODY_LEVEL_NONE;
        for (int64_t k = 0; k < m_pOwner->m_effects.m_count; k++) {
            CGameEffect* pEffect = m_pOwner->m_effects.m_pArray[k];
            if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_FORCE_BODY) {
                nForceBodyLevel = pEffect->GetInteger(0);
                break;
            }
        }
        if (nForceBodyLevel == CSWSEffectListHandler::FORCE_BODY_LEVEL_NONE) {
            int nCost = pSpell->GetAdjustedForcePoints(m_nAlignment, m_pOwner->m_bInParty,
                                                       nForceRating, m_pOwner->m_stanceId,
                                                       m_nCHAMod);
            return nCost <= static_cast<short>(m_nCurrentForcePoints + m_nTemporaryForcePoints);
        }
        int nCost = pSpell->GetAdjustedForcePoints(m_nAlignment, m_pOwner->m_bInParty,
                                                   nForceRating, m_pOwner->m_stanceId,
                                                   m_pOwner->m_pStats->m_nCHAMod);
        int nPct = 0;
        switch (nForceBodyLevel) {
        case CSWSEffectListHandler::FORCE_BODY_LEVEL_BASIC:    nPct = 50; break;
        case CSWSEffectListHandler::FORCE_BODY_LEVEL_IMPROVED: nPct = 40; break;
        case CSWSEffectListHandler::FORCE_BODY_LEVEL_MASTER:   nPct = 30; break;
        }
        nPct = nPct * nCost / 100;
        if (nPct > static_cast<short>(m_nCurrentForcePoints + m_nTemporaryForcePoints))
            return 0;
        return static_cast<short>(m_pOwner->GetCurrentHitPoints(0)) - nPct > 1;
    }
#endif
    return nFound;
}

// Whether every requirement of nFeat is met.
#if KOTOR1
int CSWSCreatureStats::FeatRequirementsMet(unsigned short nFeat,
                                           CExoArrayList<unsigned short>* pChosenFeats)
#else
int CSWSCreatureStats::FeatRequirementsMet(unsigned short nFeat,
                                           CExoArrayList<unsigned short>* pChosenFeats, int bLevelUp)
#endif
{
#if KOTOR1
// K1: the level is computed only for a feat the class grants.
    CSWFeat* pFeat = g_pRules->GetFeat(nFeat);
    if (pFeat == 0)
        return 0;
    CSWClass* pHighestClass = &g_pRules->m_paClasses[GetClass(m_numClasses - 1)];
    unsigned char nGrantLevel = pHighestClass->GetLevelFeatGranted(nFeat);
    if (nGrantLevel != GRANT_LEVEL_NEVER && GetLevel(0) < nGrantLevel)
        return 0;
#else
    int nLevel = GetLevel(0);
    if (bLevelUp)
        nLevel++;
    CSWFeat* pFeat = g_pRules->GetFeat(nFeat);
    if (pFeat == 0)
        return 0;
    CSWClass* pHighestClass = &g_pRules->m_paClasses[GetClass(m_numClasses - 1)];
    unsigned char nGrantLevel = pHighestClass->GetLevelFeatGranted(nFeat);
    if (nGrantLevel != GRANT_LEVEL_NEVER && nLevel < nGrantLevel)
        return 0;
#endif
    if (pFeat->m_nMinSpellLvl != 0) {
        int bFound = 0;
        for (int64_t i = 0; !bFound; i++) {
            if (i >= m_numClasses)
                return 0;
            unsigned char nClassId = m_classInfo[i].m_class;
            CSWClass* pClass = &g_pRules->m_paClasses[nClassId];
            if (pClass->m_bSpellCaster != 0)
                bFound = pClass->GetSpellsKnownPerLevel(m_classInfo[i].m_level, pFeat->m_nMinSpellLvl,
                                                        nClassId, m_nRace, m_nCHABase) != 0;
        }
    }
    if (pFeat->m_nMinAttackBonus > GetBaseAttackBonus())
        return 0;
    if (pFeat->m_nMinSTR > m_nSTRBase)
        return 0;
    if (pFeat->m_nMinDEX > m_nDEXBase)
        return 0;
    if (pFeat->m_nMinINT > m_nINTBase)
        return 0;
    if (pFeat->m_nMinWIS > m_nWISBase)
        return 0;
    if (!HasFeatPrereq(pFeat, pChosenFeats))
        return 0;
    if (pFeat->m_nReqSkill != -1 && !GetCanUseSkill(static_cast<unsigned char>(pFeat->m_nReqSkill)))
        return 0;
    return 1;
}

// Whether the creature may pick nFeat at level-up.
#if KOTOR1
int CSWSCreatureStats::CanChooseFeat(unsigned short nFeat, unsigned char nNormalLeft,
                                     unsigned char nBonusLeft,
                                     CExoArrayList<unsigned short>* pChosenFeats)
#else
int CSWSCreatureStats::CanChooseFeat(unsigned short nFeat, unsigned char nNormalLeft,
                                     unsigned char nBonusLeft,
                                     CExoArrayList<unsigned short>* pChosenFeats, int bLevelUp)
#endif
{
#if !KOTOR1
    if (nFeat == FEAT_COMPLEX_UNARMED_ANIMS || nFeat == FEAT_MINE_IMMUNITY)
        return 0;
#endif
    CSWFeat* pFeat = g_pRules->GetFeat(nFeat);
    if (nNormalLeft == 0 && nBonusLeft == 0)
        return 0;
    if (pFeat == 0)
        return 0;
#if !KOTOR1
    if (m_pOwner->m_sTag.CompareNoCase("baodur") != 0 && pFeat->m_nBaodurGrantLevel == GRANT_LEVEL_NEVER)
        return 0;
    if (m_pOwner->m_sTag.CompareNoCase("kreia") != 0 && pFeat->m_nKreiaGrantLevel == GRANT_LEVEL_NEVER)
        return 0;
#endif
    if ((m_feats.m_count & 0xffff) != 0)
    {
        for (int64_t i = 0; i < static_cast<unsigned short>(m_feats.m_count); i++)
        {
            if (GetFeat(static_cast<unsigned short>(i)) == nFeat)
                return 0;
        }
    }
    for (unsigned short i = 0; i < pChosenFeats->m_count; i++)
    {
        if (pChosenFeats->m_pArray[i] == nFeat)
            return 0;
    }
#if KOTOR1
    if (!FeatRequirementsMet(nFeat, pChosenFeats))
        return 0;
#else
    if (!FeatRequirementsMet(nFeat, pChosenFeats, bLevelUp))
        return 0;
#endif

// Both flags start at 0 and are set only under the two null checks.
    int bNormal = 0;
    int bBonus = 0;
    CSWClass* pClass;
    pFeat = g_pRules->GetFeat(nFeat);
    if (pFeat != 0) {
        pClass = &g_pRules->m_paClasses[GetClass(m_numClasses - 1)];
        if (pClass != 0) {
            bNormal = pClass->IsNormalFeat(nFeat) != 0;
            bBonus = pClass->IsBonusFeat(nFeat) != 0;
        }
    }
    if ((bNormal | bBonus) == 0)
        return 0;

    CExoArrayList<int> aChosen;
    aChosen.SetSize(pChosenFeats->m_count);
    for (int i = 0; i < pChosenFeats->m_count; i++)
        aChosen.m_pArray[i] = 0;
    for (unsigned short i = 0; i < pChosenFeats->m_count; i++)
    {
        unsigned short nChosen = pChosenFeats->m_pArray[i];
        CSWClass* pChosenClass;
        int bChosenNormal;
        int bChosenBonus;
        CSWFeat* pChosenFeat = g_pRules->GetFeat(nChosen);
        if (pChosenFeat == 0)
            continue;
        pChosenClass = &g_pRules->m_paClasses[GetClass(m_numClasses - 1)];
        if (pChosenClass == 0)
            continue;
        bChosenNormal = pChosenClass->IsNormalFeat(nChosen);
        bChosenBonus = pChosenClass->IsBonusFeat(nChosen);
        if (bChosenBonus != 0)
        {
            if (nBonusLeft != 0 && bChosenNormal == 0)
            {
                nBonusLeft--;
                aChosen.m_pArray[i] = 1;
            }
        }
        else if (bChosenNormal != 0)
        {
            if (nNormalLeft == 0)
                return 0;
            nNormalLeft--;
            aChosen.m_pArray[i] = 1;
        }
    }

    int bCanChoose = 0;
    pFeat = g_pRules->GetFeat(nFeat);
    if (pFeat != 0)
    {
        pClass = &g_pRules->m_paClasses[GetClass(m_numClasses - 1)];
        if (pClass != 0)
        {
            bNormal = pClass->IsNormalFeat(nFeat);
            bBonus = pClass->IsBonusFeat(nFeat);
            if (bBonus != 0)
            {
                if (bNormal == 0)
                {
                    if (nBonusLeft == 0)
                        return 0;
                    nBonusLeft--;
                    bCanChoose = 1;
                }
            }
            else if (bNormal != 0)
            {
                if (nNormalLeft == 0)
                    return 0;
                nNormalLeft--;
                bCanChoose = 1;
            }
        }
    }

    for (unsigned short i = 0; i < pChosenFeats->m_count; i++)
    {
        if (aChosen.m_pArray[i] == 0)
        {
            if (nNormalLeft != 0)
                nNormalLeft--;
            else if (nBonusLeft == 0)
                return 0;
            else
            {
                nNormalLeft = 0;
                nBonusLeft--;
            }
        }
    }
    bCanChoose |= (nBonusLeft != 0 || nNormalLeft != 0);
    return bCanChoose;
}

// Whether the creature may pick the spell.
#if KOTOR1
int CSWSCreatureStats::CanChooseSpell(unsigned int nSpellId, unsigned char nSpellsLeft,
                                      CExoArrayList<unsigned int>* pChosenSpells)
#else
int CSWSCreatureStats::CanChooseSpell(unsigned int nSpellId, unsigned char nSpellsLeft,
                                      CExoArrayList<unsigned int>* pChosenSpells, int bLevelUp)
#endif
{
    unsigned char nLastClass = m_numClasses - 1;
    CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
    if (pSpell == 0)
        return 0;
    if ((pSpell->m_nUserType | 4) != 5)
        return 0;
#if KOTOR1
// K1: no spell-level test; only the last class's known powers are checked.
    if (nSpellsLeft == 0)
        return 0;
    for (unsigned short i = 0; i < GetNumberKnownSpells(nLastClass, 0); i++) {
        if (GetKnownSpell(nLastClass, 0, static_cast<unsigned char>(i)) == static_cast<int>(nSpellId))
            return 0;
    }
#else
    unsigned char nLevel = pSpell->GetSpellLevel(GetClass(nLastClass));
    if (nSpellsLeft == 0)
        return 0;
    if (nLevel == CSWSpell::SPELL_LEVEL_NONE)
        return 0;
    if (HasSpell(0, nSpellId, 0))
        return 0;
#endif
    for (unsigned short i = 0; i < pChosenSpells->m_count; i++) {
        if (pChosenSpells->m_pArray[i] == nSpellId)
            return 0;
    }
#if KOTOR1
    if (!HasSpellPrereq(pSpell, pChosenSpells))
        return 0;
#else
    if (!HasSpellPrereq(pSpell, pChosenSpells, bLevelUp))
        return 0;
#endif
    return pChosenSpells->m_count < nSpellsLeft;
}

// Whether every id in the spell's two prereq lists is known.
#if KOTOR1
int CSWSCreatureStats::HasSpellPrereq(CSWSpell* pSpell, CExoArrayList<unsigned int>* pChosenSpells)
#else
int CSWSCreatureStats::HasSpellPrereq(CSWSpell* pSpell, CExoArrayList<unsigned int>* pChosenSpells,
                                      int bLevelUp)
#endif
{
    for (int64_t k = 0; pSpell->m_anPrereq2[k] != -1; k++) {
        int nId = pSpell->m_anPrereq2[k];
        if (HasSpell(0, nId, 1))
            continue;
        if (pChosenSpells->IndexOf(static_cast<unsigned int>(nId)) == -1)
            return 0;
    }
    for (int64_t k = 0; pSpell->m_anPrereq1[k] != -1; k++) {
        int nId = pSpell->m_anPrereq1[k];
        if (HasSpell(0, nId, 1))
            continue;
        if (pChosenSpells->IndexOf(static_cast<unsigned int>(nId)) == -1)
            return 0;
    }
    unsigned char nLevel = GetLevel(0);
#if KOTOR1
    if (nLevel >= pSpell->GetSpellLevel(GetClass(m_numClasses - 1)))
        return 1;
#else
    if (bLevelUp != 0) {
        if (nLevel + 1 >= pSpell->GetSpellLevel(GetClass(m_numClasses - 1)))
            return 1;
    } else {
        if (nLevel >= pSpell->GetSpellLevel(GetClass(m_numClasses - 1)))
            return 1;
    }
#endif
    return 0;
}

// Whether the feat is acquired on any of the three lists.
int CSWSCreatureStats::FeatAcquired(unsigned short nFeat, CExoArrayList<unsigned short>* pFeats)
{
    if (nFeat == 0xffff)
        return 0;
#if KOTOR1
    if (HasFeat(nFeat))
        return 1;
#else
    if (m_feats.Contains(nFeat))
        return 1;
    if (m_bonusFeats.Contains(nFeat))
        return 1;
#endif
    if (pFeats != 0) {
        for (unsigned short i = 0; i < pFeats->m_count; i++) {
            if (pFeats->m_pArray[i] == nFeat)
                return 1;
        }
    }
    return 0;
}

// Checks a feat's prerequisites, the AND-required feats and then the OR list.
int CSWSCreatureStats::HasFeatPrereq(CSWFeat* pFeat, CExoArrayList<unsigned short>* pChosenFeats)
{
    if (pFeat->m_nPrereqFeat1 != 0xffff && !FeatAcquired(pFeat->m_nPrereqFeat1, pChosenFeats))
        return 0;
    if (pFeat->m_nPrereqFeat2 != 0xffff && !FeatAcquired(pFeat->m_nPrereqFeat2, pChosenFeats))
        return 0;
    int bFound = 0;
    int bSeenAny = 0;
    for (int64_t i = 0; i < 5 && !bFound; i++) {
        unsigned short nOrFeat = pFeat->m_anOrReqFeat[i];
        if (nOrFeat != 0xffff) {
            bSeenAny = 1;
            bFound = FeatAcquired(nOrFeat, pChosenFeats);
        }
    }
#if KOTOR1
    return bFound || !bSeenAny;
#else
    return !bSeenAny || bFound;
#endif
}

// Server-side auto level-up while the banked XP reaches the next requirement.
#if KOTOR1
// K1: every level-keyed term uses the last class's own level.
void CSWSCreatureStats::AutoLevelup(int a)
{
    CExoArrayList<unsigned short> lstFeats;
    CExoArrayList<unsigned int> lstSpells;

    unsigned char nClassPos = m_numClasses - 1;
    unsigned char nClass = (m_numClasses != 0) ? m_classInfo[nClassPos].m_class : CLASS_TYPE_INVALID;
    CSWClass* pClass = &g_pRules->m_paClasses[nClass];

    while (static_cast<unsigned int>(m_nExperience) >= static_cast<unsigned int>(g_pRules->m_aExpTable[GetLevel(0)])) {
        unsigned char nCurLevel = GetClassLevel(nClassPos, 0);
        CSWLevelStats* pLevelStats = new CSWLevelStats();
        int nNewLevel = nCurLevel + 1;
        pLevelStats->m_nClass = nClass;
        if ((nNewLevel & 3) == 0)
            pLevelStats->m_nAbilityScoreIncrease = pClass->m_nPrimaryAbility;
        pLevelStats->m_nHitPointGain = pClass->m_nHitDie;
        pLevelStats->m_nForcePointGain = pClass->m_nForcePointGain;

        int nPool;
        if (nCurLevel == 0) {
            int nPts = (pClass->m_nSkillPointsBase >> 1) + static_cast<char>(m_nINTMod);
            if (nPts < 1)
                nPts = 1;
            nPool = nPts * 4;
        } else {
            int nPts = (pClass->m_nSkillPointsBase + static_cast<char>(m_nINTMod)) / 2;
            if (nPts < 1)
                nPts = 1;
            nPool = nPts;
        }
        nPool += m_nSkillPoints;

        int bIsPC = 0;
        if (m_bIsPC != 0) {
            CSWPartyTable* pParty = g_pAppManager->m_pServerExoApp->GetPartyTable();
            bIsPC = (pParty->m_nControlledNPC == -1);
        }

        int nMaxRankBase = nCurLevel + 4;
        int nOrder = 1;
        do {
            if (nOrder > g_pRules->m_nNumSkills)
                break;
            unsigned int nSkill = pClass->GetRecommendedSkill(nOrder);
            if (nSkill == 0xffffffff)
                goto nextSkill;
            {
                CSWSkill* pSkill = &g_pRules->m_paSkills[static_cast<unsigned short>(nSkill)];
                if (pSkill == 0)
                    goto nextSkill;
                if (!(pSkill->m_flags & CSWSkill::FLAG_ALL_CLASSES_CAN_USE) && !pClass->IsSkillUseable(static_cast<unsigned short>(nSkill)))
                    goto nextSkill;

                int bClassSkill = pClass->IsSkillClassSkill(static_cast<unsigned short>(nSkill));
                unsigned char nMaxRank = static_cast<unsigned char>(nMaxRankBase) >> static_cast<int>(bClassSkill == 0);

                int nCurRank = GetSkillRank(static_cast<unsigned char>(nSkill), 0, 1);
                int nAvail = nMaxRank - nCurRank;

                if (!bIsPC) {
                    if (!(pSkill->m_flags & CSWSkill::FLAG_NPC_CAN_USE))
                        goto nextSkill;
                    if (m_nRace == CSWRace::CRACE_DROID && !(pSkill->m_flags & CSWSkill::FLAG_DROID_CAN_USE))
                        goto nextSkill;
                }
                if (nAvail < 1)
                    goto nextSkill;

                int nRank;
                int nCost;
                if (bClassSkill) {
                    nRank = (nAvail < nPool) ? nAvail : nPool;
                    nRank = static_cast<unsigned char>(nRank);
                    nCost = nRank;
                } else {
                    nRank = (nAvail < nPool / 2) ? nAvail : nPool / 2;
                    nRank = static_cast<unsigned char>(nRank);
                    nCost = nRank * 2;
                }
                pLevelStats->SetSkillRankChange(static_cast<unsigned short>(nSkill), static_cast<char>(nRank));
                nPool -= nCost;
            }
        nextSkill:
            nOrder++;
        } while (nPool > 0);
        pLevelStats->m_nSkillPoints = static_cast<unsigned short>(nPool);

        if (g_pRules->m_nFeats != 0) {
            for (unsigned int nFeat = 0; nFeat < g_pRules->m_nFeats; nFeat++) {
                unsigned char nGrantLevel;
                if (pClass->IsGrantedFeat(static_cast<unsigned short>(nFeat), nGrantLevel) &&
                    nGrantLevel == static_cast<unsigned char>(nNewLevel))
                    AddFeat(static_cast<unsigned short>(nFeat));
            }
        }

        unsigned char nNormalFeats;
        unsigned char nBonusFeats;
        CalcLevelUpNumberFeats(0, nNormalFeats, nBonusFeats);
        int nFeatsLeft = nNormalFeats + nBonusFeats;
        lstFeats.m_count = 0;
        if (nFeatsLeft != 0 && g_pRules->m_nFeats != 0) {
            int nFeatOrder = 1;
            do {
                unsigned short nFeat = static_cast<unsigned short>(pClass->GetRecommendedFeat(nFeatOrder));
                CSWFeat* pFeat = g_pRules->GetFeat(nFeat);
                if (pFeat != 0 && pFeat->m_nMinCharLevel <= static_cast<unsigned char>(nNewLevel) &&
                    CanChooseFeat(nFeat, nNormalFeats, nBonusFeats, &lstFeats, 1)) {
                    pLevelStats->AddFeat(nFeat);
                    lstFeats.Add(nFeat);
                    nFeatsLeft--;
                }
            } while (nFeatsLeft >= 1 && nFeatOrder++ < g_pRules->m_nFeats);
        }

        if (pClass->m_bSpellCaster != 0) {
            int nSpellsLeft = ComputeNumberKnownSpellsLeft(nClassPos, 1);
            int nNumSpells = g_pRules->m_pSpellArray->m_nCount;
            lstSpells.m_count = 0;
            if (nSpellsLeft != 0 && nNumSpells > 0) {
                int nSpellOrder = 1;
                do {
                    unsigned short nSpell =
                        static_cast<unsigned short>(pClass->GetRecommendedSpell(m_nAlignment > 40, nSpellOrder));
                    if (CanChooseSpell(nSpell, static_cast<unsigned char>(nSpellsLeft), &lstSpells, 1)) {
                        pLevelStats->m_spellsGained.Add(nSpell);
                        lstSpells.Add(nSpell);
                        nSpellsLeft--;
                    }
                } while (nSpellsLeft > 0 && nSpellOrder++ < nNumSpells);
            }
        }

        if (!ValidateLevelUp(pLevelStats))
            delete pLevelStats;
    }

    if (a != 0) {
        CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
        unsigned long long nClientId = pClient->ServerToClientObjectId(m_pOwner->m_objectId);
        pClient->AddFloatyText(nClientId, CClientExoApp::FLOATY_LEVEL, GetLevel(0));
    }
}
#else
void CSWSCreatureStats::AutoLevelup(int a)
{
    CExoArrayList<unsigned short> lstFeats;
    CExoArrayList<unsigned int> lstSpells;

    unsigned char nClassPos = m_numClasses - 1;
    unsigned char nClass = (m_numClasses != 0) ? m_classInfo[nClassPos].m_class : CLASS_TYPE_INVALID;
    CSWClass* pClass = &g_pRules->m_paClasses[nClass];

    while (static_cast<unsigned int>(m_nExperience) >= static_cast<unsigned int>(g_pRules->m_aExpTable[GetLevel(0)])) {
        int nCurClassLevel = GetClassLevel(nClassPos, 0);
        int nNewClassLevel = nCurClassLevel + 1;
        unsigned char nCurLevel = GetLevel(0);
        CSWLevelStats* pLevelStats = new CSWLevelStats();
        int nNewLevel = nCurLevel + 1;
        pLevelStats->m_nClass = nClass;
        if ((nNewLevel & 3) == 0)
            pLevelStats->m_nAbilityScoreIncrease = pClass->m_nPrimaryAbility;
        pLevelStats->m_nHitPointGain = pClass->m_nHitDie;
        pLevelStats->m_nForcePointGain = pClass->m_nForcePointGain;

        int nPts = pClass->m_nSkillPointsBase + static_cast<char>(CalcStatModifier(m_nINTBase));
        if (nPts < 1)
            nPts = 1;
        int nPool = (nCurLevel == 0) ? nPts * 4 : nPts;
        nPool += m_nSkillPoints;

        int bIsPC = 0;
        if (m_bIsPC != 0) {
            CSWPartyTable* pParty = g_pAppManager->m_pServerExoApp->GetPartyTable();
            bIsPC = (pParty->m_nControlledNPC == -1);
        }

        int nMaxRankBase = nCurLevel + 4;
        int nOrder = 1;
        do {
            if (nOrder > g_pRules->m_nNumSkills)
                break;
            unsigned int nSkill = pClass->GetRecommendedSkill(nOrder);
            if (nSkill == 0xffffffff)
                goto nextSkill;
            {
                CSWSkill* pSkill = &g_pRules->m_paSkills[static_cast<unsigned short>(nSkill)];
                if (pSkill == 0)
                    goto nextSkill;
                if (!(pSkill->m_flags & CSWSkill::FLAG_ALL_CLASSES_CAN_USE) && !pClass->IsSkillUseable(static_cast<unsigned short>(nSkill)))
                    goto nextSkill;

                unsigned char nMaxRank;
                int bFullCost;
                if (pClass->IsSkillClassSkill(static_cast<unsigned short>(nSkill))) {
                    bFullCost = 1;
                    nMaxRank = static_cast<unsigned char>(nMaxRankBase);
                } else {
                    bFullCost = HasClassSkillFeat(static_cast<unsigned short>(nSkill));
                    nMaxRank = static_cast<unsigned char>(nMaxRankBase) >> static_cast<int>(bFullCost == 0);
                }

                int nCurRank = 0;
                if (static_cast<unsigned char>(nSkill) < g_pRules->m_nNumSkills) {
                    nCurRank = m_pSkillRanks[static_cast<unsigned char>(nSkill)];
                    if (nCurRank > 127)
                        nCurRank = 127;
                    if (nCurRank < -127)
                        nCurRank = -127;
                }
                int nAvail = nMaxRank - nCurRank;

                if (!bIsPC) {
                    int bNotG0T0 = 0;
                    if (nSkill == SKILL_STEALTH)
                        bNotG0T0 = m_pOwner->m_sTag.CompareNoCase("g0t0") != 0;
                    if (!(pSkill->m_flags & CSWSkill::FLAG_NPC_CAN_USE))
                        goto nextSkill;
                    if (m_nRace == CSWRace::CRACE_DROID && !bNotG0T0 && !(pSkill->m_flags & CSWSkill::FLAG_DROID_CAN_USE))
                        goto nextSkill;
                }
                if (nAvail < 1)
                    goto nextSkill;

                int nRank;
                int nCost;
                if (bFullCost) {
                    nRank = (nAvail < nPool) ? nAvail : nPool;
                    nRank = static_cast<unsigned char>(nRank);
                    nCost = nRank;
                } else {
                    nRank = (nAvail < nPool / 2) ? nAvail : nPool / 2;
                    nRank = static_cast<unsigned char>(nRank);
                    nCost = nRank * 2;
                }
                pLevelStats->SetSkillRankChange(static_cast<unsigned short>(nSkill), static_cast<char>(nRank));
                nPool -= nCost;
            }
        nextSkill:
            nOrder++;
        } while (nPool > 0);
        pLevelStats->m_nSkillPoints = static_cast<unsigned short>(nPool);

        int nTotalJediLevel = 0;
        for (int64_t i = 0; i < m_numClasses; i++) {
            if (CSWClass::IsJedi(GetClass(static_cast<unsigned char>(i)))) {
                if (i == m_numClasses - 1)
                    nTotalJediLevel += GetClassLevel(static_cast<unsigned char>(i), 0) + 1;
                else
                    nTotalJediLevel += GetClassLevel(static_cast<unsigned char>(i), 0);
            }
        }

        if (g_pRules->m_nFeats != 0) {
            for (unsigned int nFeat = 0; nFeat < g_pRules->m_nFeats; nFeat++) {
                unsigned char nGrantLevel;
                if (pClass->IsGrantedFeat(static_cast<unsigned short>(nFeat), nGrantLevel)) {
                    if (nFeat - FEAT_UNARMED_SPECIALIST_I < 8) {
                        if (CSWClass::IsJedi(GetClass(m_numClasses - 1)) &&
                            nGrantLevel == nTotalJediLevel)
                            AddFeat(static_cast<unsigned short>(nFeat));
                    } else if (nGrantLevel == static_cast<unsigned char>(nNewClassLevel)) {
                        CSWFeat* pFeat = g_pRules->GetFeat(static_cast<unsigned short>(nFeat));
                        if ((pFeat == 0 || m_pOwner->m_sTag.CompareNoCase("baodur") == 0 ||
                             pFeat->m_nBaodurGrantLevel != GRANT_LEVEL_NEVER) &&
                            (pFeat == 0 || m_pOwner->m_sTag.CompareNoCase("kreia") == 0 ||
                             pFeat->m_nKreiaGrantLevel != GRANT_LEVEL_NEVER))
                            AddFeat(static_cast<unsigned short>(nFeat));
                    }
                } else if (m_pOwner->m_sTag.CompareNoCase("handmaiden")) {
                    CSWFeat* pFeat = g_pRules->GetFeat(static_cast<unsigned short>(nFeat));
                    if (pFeat != 0 && pFeat->m_nHandmaidenGrantLevel != 0 &&
                        pFeat->m_nHandmaidenGrantLevel == static_cast<unsigned char>(nNewLevel))
                        AddFeat(static_cast<unsigned short>(nFeat));
                } else if (m_pOwner->m_sTag.CompareNoCase("baodur")) {
                    CSWFeat* pFeat = g_pRules->GetFeat(static_cast<unsigned short>(nFeat));
                    if (pFeat != 0 && pFeat->m_nBaodurGrantLevel != 0 &&
                        pFeat->m_nBaodurGrantLevel == static_cast<unsigned char>(nNewLevel))
                        AddFeat(static_cast<unsigned short>(nFeat));
                } else if (m_pOwner->m_sTag.CompareNoCase("hanharr")) {
                    CSWFeat* pFeat = g_pRules->GetFeat(static_cast<unsigned short>(nFeat));
                    if (pFeat != 0 && pFeat->m_nHanharrGrantLevel != 0 &&
                        pFeat->m_nHanharrGrantLevel == static_cast<unsigned char>(nNewLevel))
                        AddFeat(static_cast<unsigned short>(nFeat));
                } else if (m_pOwner->m_sTag.CompareNoCase("hk47")) {
                    CSWFeat* pFeat = g_pRules->GetFeat(static_cast<unsigned short>(nFeat));
                    if (pFeat != 0 && pFeat->m_nHK47GrantLevel != 0 &&
                        pFeat->m_nHK47GrantLevel == static_cast<unsigned char>(nNewLevel))
                        AddFeat(static_cast<unsigned short>(nFeat));
                } else if (m_pOwner->m_sTag.CompareNoCase("g0t0")) {
                    CSWFeat* pFeat = g_pRules->GetFeat(static_cast<unsigned short>(nFeat));
                    if (pFeat != 0 && pFeat->m_nG0T0GrantLevel != 0 &&
                        pFeat->m_nG0T0GrantLevel == static_cast<unsigned char>(nNewLevel))
                        AddFeat(static_cast<unsigned short>(nFeat));
                } else if (m_pOwner->m_sTag.CompareNoCase("atton")) {
                    CSWFeat* pFeat = g_pRules->GetFeat(static_cast<unsigned short>(nFeat));
                    if (pFeat != 0 && pFeat->m_nAttonGrantLevel != 0 &&
                        pFeat->m_nAttonGrantLevel == static_cast<unsigned char>(nNewLevel))
                        AddFeat(static_cast<unsigned short>(nFeat));
                }
            }
        }

        unsigned char nNormalFeats;
        unsigned char nBonusFeats;
        CalcLevelUpNumberFeats(0, nNormalFeats, nBonusFeats);
        int nFeatsLeft = nNormalFeats + nBonusFeats;
        lstFeats.m_count = 0;
        if (nFeatsLeft != 0 && g_pRules->m_nFeats != 0) {
            int nFeatOrder = 1;
            do {
                unsigned short nFeat = static_cast<unsigned short>(pClass->GetRecommendedFeat(nFeatOrder));
                CSWFeat* pFeat = g_pRules->GetFeat(nFeat);
                if (pFeat != 0 && pFeat->m_nMinCharLevel <= static_cast<unsigned char>(nNewLevel) &&
                    CanChooseFeat(nFeat, nNormalFeats, nBonusFeats, &lstFeats, 1)) {
                    pLevelStats->AddFeat(nFeat);
                    lstFeats.Add(nFeat);
                    nFeatsLeft--;
                }
            } while (nFeatsLeft >= 1 && nFeatOrder++ < g_pRules->m_nFeats);
        }

        if (pClass->m_bSpellCaster != 0) {
            int nSpellsLeft = ComputeNumberKnownSpellsLeft(nClassPos, 1);
            int nNumSpells = g_pRules->m_pSpellArray->m_nCount;
            lstSpells.m_count = 0;
            unsigned char nSpellsKnown = static_cast<unsigned char>(nSpellsLeft);

            if (m_numClasses != 0) {
                static const unsigned int KOTOR2_JEDI_POWERS_3[4] = {258, 259, 260, 265};
                static const unsigned int KOTOR2_JEDI_POWERS_4[4] = {258, 265, 259, 260};
                static const unsigned int KOTOR2_JEDI_POWERS_5[4] = {258, 260, 265, 259};
                int nJediTier = nTotalJediLevel - 11;
                unsigned char nLevelRem = (static_cast<unsigned char>(nNewClassLevel) - 1) & 3;
                unsigned char nLevelTier = (static_cast<unsigned char>(nNewClassLevel) - 1) >> 2;
                for (int64_t i = 0; i < m_numClasses; i++) {
                    unsigned int nPower;
                    switch (m_classInfo[i].m_class) {
                    case CLASS_TYPE_JEDIGUARDIAN:
                        if (static_cast<unsigned>(nJediTier) >= 4)
                            continue;
                        nPower = KOTOR2_JEDI_POWERS_3[nJediTier];
                        break;
                    case CLASS_TYPE_JEDICONSULAR:
                        if (static_cast<unsigned>(nJediTier) >= 4)
                            continue;
                        nPower = KOTOR2_JEDI_POWERS_4[nJediTier];
                        break;
                    case CLASS_TYPE_JEDISENTINEL:
                        if (static_cast<unsigned>(nJediTier) >= 4)
                            continue;
                        nPower = KOTOR2_JEDI_POWERS_5[nJediTier];
                        break;
                    case CLASS_TYPE_JEDIMASTER:
                        if (nLevelRem != 0 || nLevelTier >= 5)
                            continue;
                        nPower = nLevelTier + FORCE_POWER_INSPIRE_FOLLOWERS_I;
                        break;
                    case CLASS_TYPE_JEDIWATCHMAN:
                    case CLASS_TYPE_SITHASSASSIN:
                        if (nCurClassLevel == 0)
                            nPower = FORCE_POWER_FORCE_CAMOUFLAGE;
                        else if (nCurClassLevel == 12)
                            nPower = FORCE_POWER_MASTER_FORCE_CAMOUFLAGE;
                        else if (nCurClassLevel == 6)
                            nPower = FORCE_POWER_IMPROVED_FORCE_CAMOUFLAGE;
                        else
                            continue;
                        break;
                    case CLASS_TYPE_SITHMARAUDER:
                        if (nCurClassLevel == 0)
                            nPower = FORCE_POWER_FURY;
                        else if (nCurClassLevel == 8)
                            nPower = FORCE_POWER_MASTER_FURY;
                        else if (nCurClassLevel == 4)
                            nPower = FORCE_POWER_IMPROVED_FURY;
                        else
                            continue;
                        break;
                    case CLASS_TYPE_SITHLORD:
                        if (nLevelRem != 0 || nLevelTier >= 5)
                            continue;
                        nPower = nLevelTier + FORCE_POWER_CRUSH_OPPOSITION_I;
                        break;
                    default:
                        continue;
                    }
                    pLevelStats->m_spellsGained.Add(nPower);
                    lstSpells.Add(nPower);
                    nSpellsKnown++;
                }
            }

            if (nSpellsLeft != 0 && nNumSpells > 0) {
                int nSpellOrder = 1;
                do {
                    unsigned short nSpell =
                        static_cast<unsigned short>(pClass->GetRecommendedSpell(m_nAlignment > 40, nSpellOrder));
                    if (CanChooseSpell(nSpell, nSpellsKnown, &lstSpells, 1)) {
                        pLevelStats->m_spellsGained.Add(nSpell);
                        lstSpells.Add(nSpell);
                        nSpellsLeft--;
                    }
                } while (nSpellsLeft > 0 && nSpellOrder++ < nNumSpells);
            }
        }

        if (!ValidateLevelUp(pLevelStats))
            delete pLevelStats;
    }

    if (m_pOwner->m_bPlayerMadeCharacter != 0) {
        for (int i = 0; i < 12; i++) {
            CSWPartyTable* pParty = g_pAppManager->m_pServerExoApp->GetPartyTable();
            unsigned long long nObject = pParty->GetNPCObject(i, 0, 1);
            if (nObject != OBJECT_INVALID) {
                CSWSCreature* pCreature =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nObject);
                if (pCreature != 0)
                    pCreature->HandleAlignmentInfluence();
            }
        }
    }
    if (a != 0) {
        CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
        unsigned long long nClientId = pClient->ServerToClientObjectId(m_pOwner->m_objectId);
        pClient->AddFloatyText(nClientId, CClientExoApp::FLOATY_LEVEL, GetLevel(0));
    }
}
#endif

// Reads the stats block out of the GFF.
int64_t CSWSCreatureStats::ReadStatsFromGff(CResGFF* pGFF, CResStruct* pStruct,
                                         CSWSCreatureAppearanceInfo* pAppearance)
{
// `success` is genuinely uninitialized here; there is no init store.
    int success;

#if KOTOR1
// K1: the names are not resolved through GetString on the way in.
    CExoLocString locTemp;
    {
        CExoLocString emptyLoc;
        locTemp = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("FirstName"), success, emptyLoc);
    }
    m_firstName = locTemp;
    {
        CExoLocString emptyLoc;
        locTemp = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("LastName"), success, emptyLoc);
    }
    m_lastName = locTemp;
#else
    CExoLocString locTemp;
    CExoString strTemp;

    int nLanguage = g_pAppManager->m_pClientExoApp->GetClientLanguage();
    {
        CExoLocString emptyLoc;
        locTemp = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("FirstName"), success, emptyLoc);
    }
    m_firstName = locTemp;
    locTemp.GetString(nLanguage, &strTemp, 0);
    {
        CExoLocString emptyLoc;
        locTemp = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("LastName"), success, emptyLoc);
    }
    m_lastName = locTemp;
    locTemp.GetString(nLanguage, &strTemp, 0);
#endif
    m_description = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("Description"), success, m_description);

    m_bIsPC = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("IsPC"), success, static_cast<unsigned char>(m_bIsPC));
    m_pOwner->m_sTag = pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("Tag"), success, m_pOwner->m_sTag)
                           .LowerCase();
    SetDialog(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("Conversation"), success, m_conversation));
    m_nInterruptable = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Interruptable"), success, static_cast<unsigned char>(m_nInterruptable));
    m_nAge = pGFF->ReadFieldINT(pStruct, const_cast<char*>("Age"), success, m_nAge);

    unsigned char nGender = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Gender"), success, m_nGender);
    if (nGender > 3)
        nGender = 4;
    m_nGender = nGender;
    m_nStartingPackage = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("StartingPackage"), success, m_nStartingPackage);

    unsigned char nRace = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Race"), success, static_cast<unsigned char>(m_nRace));
    if (g_pRules->m_nNumRaces <= nRace)
        return STRREF_INVALID_SPECIES;
    m_nRace = nRace;

    m_sSubrace = pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("Subrace"), success, m_sSubrace);
    m_nSubraceIndex = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("SubraceIndex"), success, m_nSubraceIndex);
    m_sDeity = pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("Deity"), success, m_sDeity);

    m_nSTRBase = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Str"), success, m_nSTRBase);
    UpdateSTRModifier();
    m_nDEXBase = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Dex"), success, m_nDEXBase);
    UpdateDEXModifier();
    m_nINTBase = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Int"), success, m_nINTBase);
    UpdateINTModifier();
    AdjustSpellUsesPerDay();
    m_nWISBase = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Wis"), success, m_nWISBase);
    UpdateWISModifier();
    AdjustSpellUsesPerDay();
    m_nCONBase = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Con"), success, m_nCONBase);
    UpdateCONModifier();
    m_nCHABase = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Cha"), success, m_nCHABase);
    UpdateCHAModifier();
    AdjustSpellUsesPerDay();

    m_nACNaturalBase = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("NaturalAC"), success, m_nACNaturalBase);
    m_pOwner->m_nSoundSetFile =
        pGFF->ReadFieldWORD(pStruct, const_cast<char*>("SoundSetFile"), success, m_pOwner->m_nSoundSetFile);

    if (m_pOwner->m_bInParty == 0) {
        m_pOwner->SetGold(pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("Gold"), success, m_pOwner->GetGold()));
#if !KOTOR1
        m_pOwner->SetComponent(
            pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("ItemComponent"), success, m_pOwner->GetComponent()));
        m_pOwner->SetChemical(
            pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("Chemicals"), success, m_pOwner->GetChemical()));
#endif
    }

    int nPlot = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Invulnerable"), success, static_cast<unsigned char>(m_pOwner->m_bPlot));
    if (success == 0)
        nPlot = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Plot"), success, static_cast<unsigned char>(m_pOwner->m_bPlot));
    m_pOwner->m_bPlot = nPlot;
    m_pOwner->m_bMin1HP = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Min1HP"), success, static_cast<unsigned char>(m_pOwner->m_bMin1HP));
    m_pOwner->m_bPartyInteract =
        pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("PartyInteract"), success, static_cast<unsigned char>(m_pOwner->m_bPartyInteract));
    m_pOwner->m_bOrientOnClick =
        pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("NotReorienting"), success, m_pOwner->m_bOrientOnClick == 0) == 0;
#if !KOTOR1
    m_pOwner->m_bHologram =
        pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Hologram"), success, static_cast<unsigned char>(m_pOwner->m_bHologram));
    m_pOwner->m_bIgnoreCrePath =
        pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("IgnoreCrePath"), success, m_pOwner->m_bIgnoreCrePath) != 0;
    m_pOwner->m_nMultiplierSet =
        pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("MultiplierSet"), success, m_pOwner->m_nMultiplierSet);
    m_pOwner->m_nPCLevelAtSpawn =
        pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("PCLevelAtSpawn"), success, m_pOwner->m_nPCLevelAtSpawn);
    m_pOwner->m_bWillNotRender =
        pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("WillNotRender"), success, static_cast<unsigned char>(m_pOwner->m_bWillNotRender));
    m_pOwner->m_bConfused = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Confused"), success, static_cast<unsigned char>(m_pOwner->m_bConfused));
#endif
    m_pOwner->m_bDisarmable =
        pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Disarmable"), success, static_cast<unsigned char>(m_pOwner->m_bDisarmable));

    SetExperience(pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("Experience"), success, m_nExperience), 0);

    unsigned short nPortraitId = pGFF->ReadFieldWORD(pStruct, const_cast<char*>("PortraitId"), success, CSWSObject::PORTRAIT_ID_NONE);
    if (nPortraitId >= CSWSObject::PORTRAIT_ID_CUSTOM) {
        // Calls SetPortraitId in both arms.
        m_pOwner->SetPortraitId(nPortraitId);
        SetPortrait(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("Portrait"), success, m_portrait));
        // The copy's resref string is fetched and discarded.
        CResRef portraitCopy = m_portrait;
        portraitCopy.GetResRefStr();
    } else {
        m_pOwner->SetPortraitId(nPortraitId);
    }

    unsigned short nGoodEvil =
        pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("GoodEvil"), success, static_cast<unsigned char>(m_nAlignment));
    if (nGoodEvil > 99)
        nGoodEvil = 100;
    m_nAlignment = nGoodEvil;

#if !KOTOR1
    char nBaseCNPCAlignment = pGFF->ReadFieldCHAR(pStruct, const_cast<char*>("BaseCNPCAlignment"), success, -1);
    if (static_cast<unsigned char>(nBaseCNPCAlignment) == 0xff)
        nBaseCNPCAlignment = static_cast<char>(m_nAlignment);
    int nClampedAlignment = nBaseCNPCAlignment;
    if (nClampedAlignment < 0)
        nClampedAlignment = -1;
    if (nClampedAlignment > 99)
        nClampedAlignment = 100;
    m_nBaseNPCAlignment = static_cast<short>(nClampedAlignment);

    m_nFuryDamageBonus = pGFF->ReadFieldCHAR(pStruct, const_cast<char*>("FuryDamageBonus"), success, -1);
    m_pOwner->m_stanceId = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("CurrentForm"), success, 0);
#endif

    m_nColorSkin = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Color_Skin"), success, m_nColorSkin);
    m_nColorHair = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Color_Hair"), success, m_nColorHair);
    m_nColorTattoo1 = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Color_Tattoo1"), success, m_nColorTattoo1);
    m_nColorTattoo2 = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Color_Tattoo2"), success, m_nColorTattoo2);
    m_nPhenotype = static_cast<unsigned char>(pGFF->ReadFieldINT(pStruct, const_cast<char*>("Phenotype"), success, m_nPhenotype));
    m_nAppearanceType =
        pGFF->ReadFieldWORD(pStruct, const_cast<char*>("Appearance_Type"), success, m_nAppearanceType);
    unsigned char nHead =
        pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Appearance_Head"), success, m_nAppearanceHead);
    if (nHead == 0)
        nHead = 1;
    m_nAppearanceHead = nHead;
    m_bDuplicatingHead =
        pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("DuplicatingHead"), success, m_bDuplicatingHead);
    m_nUseBackupHead =
        pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("UseBackupHead"), success, static_cast<unsigned char>(m_nUseBackupHead));
// Head, Tail and Wings are not read; both are zeroed here.
    m_bTail = 0;
    m_bWings = 0;

    m_factionId = pGFF->ReadFieldWORD(pStruct, const_cast<char*>("FactionID"), success, static_cast<unsigned short>(m_factionId));
#if !KOTOR1
    m_fBlindSpot = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("BlindSpot"), success, m_fBlindSpot);
#endif
    m_fChallengeRating = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("ChallengeRating"), success, m_fChallengeRating);
    m_aiState = static_cast<unsigned short>(pGFF->ReadFieldINT(pStruct, const_cast<char*>("AIState"), success, m_aiState));
    m_pOwner->m_bBodyBag = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("BodyBag"), success, m_pOwner->m_bBodyBag);

    float fRange;
    if (m_bIsPC == 0) {
        unsigned char nPerceptionRange = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("PerceptionRange"), success, RANGES_ROW_PERCEPTION_DEFAULT);
        if (nPerceptionRange == RANGES_ROW_PERCEPTION_DEFAULT) {
            int nPerceptionDist;
            {
                CExoString col(const_cast<char*>("PERCEPTIONDIST"));
                if (!g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
                        m_nAppearanceType, col, &nPerceptionDist))
                    return STRREF_INVALID_VISIBILITY_RANGE;
            }
            {
                CExoString col(const_cast<char*>("PrimaryRange"));
                if (!g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(nPerceptionDist, col, &fRange))
                    return STRREF_INVALID_VISIBILITY_RANGE;
            }
            m_pOwner->m_fPerceptionPrimaryRange = fRange;
            {
                CExoString col(const_cast<char*>("SecondaryRange"));
                if (!g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(nPerceptionDist, col, &fRange))
                    return STRREF_INVALID_VISIBILITY_RANGE;
            }
        } else if (nPerceptionRange != RANGES_ROW_PERCEPTION_PLAYER) {
            {
                CExoString col(const_cast<char*>("PrimaryRange"));
                if (!g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(nPerceptionRange, col, &fRange))
                    return STRREF_INVALID_VISIBILITY_RANGE;
            }
            m_pOwner->m_fPerceptionPrimaryRange = fRange;
            {
                CExoString col(const_cast<char*>("SecondaryRange"));
                if (!g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(nPerceptionRange, col, &fRange))
                    return STRREF_INVALID_VISIBILITY_RANGE;
            }
        } else {
            return STRREF_INVALID_VISIBILITY_RANGE;
        }
    } else {
        {
            CExoString col(const_cast<char*>("PrimaryRange"));
            g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(RANGES_ROW_PERCEPTION_PLAYER, col, &fRange);
        }
        m_pOwner->m_fPerceptionPrimaryRange = fRange;
        {
            CExoString col(const_cast<char*>("SecondaryRange"));
            g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(RANGES_ROW_PERCEPTION_PLAYER, col, &fRange);
        }
    }
    m_pOwner->m_fPerceptionSecondaryRange = fRange;

    CResList classList;
    if (pGFF->GetList(&classList, pStruct, const_cast<char*>("ClassList"))) {
        if (pGFF->GetListCount(&classList) == 0)
            return STRREF_INVALID_MULTICLASS_COUNT;
        unsigned char nCount = static_cast<unsigned char>(pGFF->GetListCount(&classList));
        m_numClasses = (nCount > 2) ? 2 : nCount;
        if (m_numClasses != 0) {
            for (int64_t i = 0; i < m_numClasses; i++) {
                CResStruct classElem;
                if (pGFF->GetListElement(&classElem, &classList, static_cast<unsigned int>(i))) {
                    unsigned char defaultClass =
                        (i < m_numClasses) ? m_classInfo[i].m_class : CLASS_TYPE_INVALID;
                    int nClass = pGFF->ReadFieldINT(&classElem, const_cast<char*>("Class"), success, defaultClass);
                    if (nClass != -1 && success == 1) {
                        if (nClass >= g_pRules->m_nNumClasses)
                            return STRREF_INVALID_CLASS;
                        if (i < m_numClasses)
                            m_classInfo[i].m_class = static_cast<unsigned char>(nClass);
                    }
                    if (i < m_numClasses && m_classInfo[i].m_class != CLASS_TYPE_INVALID) {
                        unsigned short nLevel = pGFF->ReadFieldSHORT(
                            &classElem, const_cast<char*>("ClassLevel"), success, m_classInfo[i].m_level);
                        if (i < m_numClasses) {
                            m_classInfo[i].m_level = static_cast<unsigned char>(nLevel);
#if !KOTOR1
                            if (m_pOwner != 0 && m_pOwner->m_bPlayerMadeCharacter != 0) {
                                if (GetLevel(0) >= 30)
                                    Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_LEVEL_30);
                            }
#endif
                        }
                        m_classInfo[i].m_negativeLevels = 0;
                        if (CSWClass::IsJedi((i < m_numClasses) ? m_classInfo[i].m_class : CLASS_TYPE_INVALID)) {
                            CResList spellsPerDayList;
                            if (pGFF->GetList(&spellsPerDayList, &classElem,
                                              const_cast<char*>("SpellsPerDayList")) == 1) {
                                unsigned char j = 0;
                                while (true) {
                                    unsigned int nSpellCount = pGFF->GetListCount(&spellsPerDayList);
                                    j = j & 0xff;
                                    if (nSpellCount <= j)
                                        break;
                                    CResStruct spdElem;
                                    pGFF->GetListElement(&spdElem, &spellsPerDayList, j);
                                    unsigned char nLeft = pGFF->ReadFieldBYTE(
                                        &spdElem, const_cast<char*>("NumSpellsLeft"), success, 0);
                                    if (j == 0) {
                                        m_classInfo[i].m_spellsPerDayLeft = nLeft;
                                        m_classInfo[i].m_maxSpellsPerDayLeft = nLeft;
                                    }
                                    j++;
                                }
                            }
                        }
                    }
                }
            }
#ifndef K2_FIX_ENGINE_BUGS
            if (m_numClasses >= 3 && m_classInfo[2].m_class == CLASS_TYPE_INVALID)
                m_numClasses = 2;
#endif
            if (m_numClasses >= 2) {
                if (m_classInfo[1].m_class == CLASS_TYPE_INVALID) {
                    m_numClasses = 1;
                } else {
                    if (m_classInfo[0].m_class == m_classInfo[1].m_class)
                        return STRREF_INVALID_CLASS;
#ifndef K2_FIX_ENGINE_BUGS
                    if (m_numClasses >= 3 && (m_classInfo[0].m_class == m_classInfo[2].m_class ||
                                              m_classInfo[1].m_class == m_classInfo[2].m_class))
                        return STRREF_INVALID_CLASS;
#endif
                }
            }
        }
    } else {
        if (m_numClasses == 0 || m_classInfo[0].m_level == 0)
            return STRREF_INVALID_CLASS_LEVEL;
        if (m_numClasses >= 2) {
            if (m_classInfo[0].m_class == m_classInfo[1].m_class)
                return STRREF_INVALID_CLASS;
#ifndef K2_FIX_ENGINE_BUGS
            if (m_numClasses >= 3 && (m_classInfo[0].m_class == m_classInfo[2].m_class ||
                                      m_classInfo[1].m_class == m_classInfo[2].m_class))
                return STRREF_INVALID_CLASS;
#endif
        }
    }

    m_nWillSaveBonus = static_cast<signed char>(pGFF->ReadFieldSHORT(pStruct, const_cast<char*>("willbonus"), success, m_nWillSaveBonus));
    m_nFortSaveBonus = static_cast<signed char>(pGFF->ReadFieldSHORT(pStruct, const_cast<char*>("fortbonus"), success, m_nFortSaveBonus));
    m_nReflexSaveBonus = static_cast<signed char>(pGFF->ReadFieldSHORT(pStruct, const_cast<char*>("refbonus"), success, m_nReflexSaveBonus));

    if (m_bIsPC != 0) {
        CResList lvlStatList;
        pGFF->GetList(&lvlStatList, pStruct, const_cast<char*>("LvlStatList"));
        for (int64_t n = 0; n < GetLevel(0); n++) {
            CSWLevelStats* pLvlStats;
            if (n == 0) {
                pLvlStats = GetLevelStats(0);
            } else {
                pLvlStats = new CSWLevelStats();
                m_levelStats.Add(pLvlStats);
            }
            CResStruct lvlElem;
            if (pGFF->GetListElement(&lvlElem, &lvlStatList, static_cast<unsigned int>(n))) {
                pLvlStats->m_nAbilityScoreIncrease = pGFF->ReadFieldBYTE(
                    &lvlElem, const_cast<char*>("LvlStatAbility"), success, pLvlStats->m_nAbilityScoreIncrease);
                pLvlStats->m_nHitPointGain = pGFF->ReadFieldBYTE(
                    &lvlElem, const_cast<char*>("LvlStatHitDie"), success, pLvlStats->m_nHitPointGain);
                pLvlStats->m_nForcePointGain = pGFF->ReadFieldBYTE(
                    &lvlElem, const_cast<char*>("LvlStatForce"), success, pLvlStats->m_nForcePointGain);
                pLvlStats->m_nClass = pGFF->ReadFieldBYTE(&lvlElem, const_cast<char*>("LvlStatClass"), success,
                                                          pLvlStats->m_nClass);

                CExoString label;
                label.Format(const_cast<char*>("%s%d"), const_cast<char*>("KnownList"), 0);
                CResList gainedList;
                if (pGFF->GetList(&gainedList, &lvlElem, static_cast<char*>(label.CStr()))) {
                    unsigned short j = 0;
                    while (true) {
                        unsigned short nSpellCount = static_cast<unsigned short>(pGFF->GetListCount(&gainedList));
                        if (nSpellCount <= j)
                            break;
                        CResStruct spellElem;
                        if (pGFF->GetListElement(&spellElem, &gainedList, j)) {
                            unsigned short nSpell =
                                pGFF->ReadFieldWORD(&spellElem, const_cast<char*>("Spell"), success, 0xffff);
                            pLvlStats->AddAddedKnownSpell(0, nSpell);
                        }
                        j++;
                    }
                }
                label.Format(const_cast<char*>("%s%d"), const_cast<char*>("KnownRemoveList"), 0);
                CResList lostList;
                if (pGFF->GetList(&lostList, &lvlElem, static_cast<char*>(label.CStr()))) {
                    unsigned short j = 0;
                    while (true) {
                        unsigned short nSpellCount = static_cast<unsigned short>(pGFF->GetListCount(&lostList));
                        if (nSpellCount <= j)
                            break;
                        CResStruct spellElem;
                        if (pGFF->GetListElement(&spellElem, &lostList, j)) {
                            unsigned short nSpell =
                                pGFF->ReadFieldWORD(&spellElem, const_cast<char*>("Spell"), success, 0xffff);
                            pLvlStats->AddRemovedKnownSpell(0, nSpell);
                        }
                        j++;
                    }
                }

                pLvlStats->m_nSkillPoints =
                    pGFF->ReadFieldWORD(&lvlElem, const_cast<char*>("SkillPoints"), success, m_nSkillPoints);

                CResList skillList;
                if (pGFF->GetList(&skillList, &lvlElem, const_cast<char*>("SkillList"))) {
                    m_pSkillRanks[0] = 0;
                    m_pSkillRanks[1] = 0;
                    m_pSkillRanks[2] = 0;
                    m_pSkillRanks[3] = 0;
                    m_pSkillRanks[4] = 0;
                    m_pSkillRanks[5] = 0;
                    m_pSkillRanks[6] = 0;
                    m_pSkillRanks[7] = 0;
                    CResStruct skillElem;
                    if (pGFF->GetListElement(&skillElem, &skillList, 0)) {
                        char nRank = pGFF->ReadFieldBYTE(&skillElem, const_cast<char*>("Rank"), success,
                                                         pLvlStats->GetSkillRankChange(SKILL_COMPUTER_USE));
                        pLvlStats->SetSkillRankChange(SKILL_COMPUTER_USE, nRank);
                    }
                    if (pGFF->GetListElement(&skillElem, &skillList, 1)) {
                        char nRank = pGFF->ReadFieldBYTE(&skillElem, const_cast<char*>("Rank"), success,
                                                         pLvlStats->GetSkillRankChange(SKILL_DEMOLITIONS));
                        pLvlStats->SetSkillRankChange(SKILL_DEMOLITIONS, nRank);
                    }
                    if (pGFF->GetListElement(&skillElem, &skillList, 2)) {
                        char nRank = pGFF->ReadFieldBYTE(&skillElem, const_cast<char*>("Rank"), success,
                                                         pLvlStats->GetSkillRankChange(SKILL_STEALTH));
                        pLvlStats->SetSkillRankChange(SKILL_STEALTH, nRank);
                    }
                    if (pGFF->GetListElement(&skillElem, &skillList, 3)) {
                        char nRank = pGFF->ReadFieldBYTE(&skillElem, const_cast<char*>("Rank"), success,
                                                         pLvlStats->GetSkillRankChange(SKILL_AWARENESS));
                        pLvlStats->SetSkillRankChange(SKILL_AWARENESS, nRank);
                    }
                    if (pGFF->GetListElement(&skillElem, &skillList, 4)) {
                        char nRank = pGFF->ReadFieldBYTE(&skillElem, const_cast<char*>("Rank"), success,
                                                         pLvlStats->GetSkillRankChange(SKILL_PERSUADE));
                        pLvlStats->SetSkillRankChange(SKILL_PERSUADE, nRank);
                    }
                    if (pGFF->GetListElement(&skillElem, &skillList, 5)) {
                        char nRank = pGFF->ReadFieldBYTE(&skillElem, const_cast<char*>("Rank"), success,
                                                         pLvlStats->GetSkillRankChange(SKILL_REPAIR));
                        pLvlStats->SetSkillRankChange(SKILL_REPAIR, nRank);
                    }
                    if (pGFF->GetListElement(&skillElem, &skillList, 6)) {
                        char nRank = pGFF->ReadFieldBYTE(&skillElem, const_cast<char*>("Rank"), success,
                                                         pLvlStats->GetSkillRankChange(SKILL_SECURITY));
                        pLvlStats->SetSkillRankChange(SKILL_SECURITY, nRank);
                    }
                    if (pGFF->GetListElement(&skillElem, &skillList, 7)) {
                        char nRank = pGFF->ReadFieldBYTE(&skillElem, const_cast<char*>("Rank"), success,
                                                         pLvlStats->GetSkillRankChange(SKILL_TREAT_INJURY));
                        pLvlStats->SetSkillRankChange(SKILL_TREAT_INJURY, nRank);
                    }
                }

                CResList featList;
                if (pGFF->GetList(&featList, &lvlElem, const_cast<char*>("FeatList"))) {
                    unsigned short nFeatCount = static_cast<unsigned short>(pGFF->GetListCount(&featList));
                    if (nFeatCount != 0) {
                        for (unsigned short j = 0; j < nFeatCount; j++) {
                            CResStruct featElem;
                            if (pGFF->GetListElement(&featElem, &featList, j)) {
                                unsigned short nFeat =
                                    pGFF->ReadFieldWORD(&featElem, const_cast<char*>("Feat"), success, 0);
                                if (success != 0)
                                    pLvlStats->AddFeat(nFeat);
                            }
                        }
                    }
                }
            }
        }
    }

    short nHitPoints =
        pGFF->ReadFieldSHORT(pStruct, const_cast<char*>("HitPoints"), success, static_cast<short>(m_pOwner->m_hitPoints));
    m_pOwner->m_hitPoints = nHitPoints;
    short nForcePoints =
        pGFF->ReadFieldSHORT(pStruct, const_cast<char*>("ForcePoints"), success, m_pOwner->m_pStats->m_nBaseForcePoints);
// The store, and every read here, goes through m_pOwner->m_pStats.
    m_pOwner->m_pStats->m_nBaseForcePoints = nForcePoints;
    short nCurHitPoints = pGFF->ReadFieldSHORT(pStruct, const_cast<char*>("CurrentHitPoints"), success,
                                               static_cast<short>(m_pOwner->m_hitPoints));
    short nCurForce =
        pGFF->ReadFieldSHORT(pStruct, const_cast<char*>("CurrentForce"), success, m_pOwner->m_pStats->m_nBaseForcePoints);
#if !KOTOR1
    m_pOwner->m_nBonusForcePoints = pGFF->ReadFieldINT(pStruct, const_cast<char*>("BonusForcePoints"), success, 0);
    m_pOwner->m_bPlayerMadeCharacter = pGFF->ReadFieldINT(pStruct, const_cast<char*>("PlayerCreated"), success, 0);
    m_pOwner->m_nAssignedPup = pGFF->ReadFieldINT(pStruct, const_cast<char*>("AssignedPup"), success, 0xffffffff);
#endif

    unsigned short nNewForcePoints;
    if (m_bIsPC == 0) {
        if (nCurHitPoints + GetLevel(0) * m_nCONMod < GetLevel(0))
            m_pOwner->SetCurrentHitPoints(GetLevel(0));
        else
            m_pOwner->SetCurrentHitPoints(nCurHitPoints + GetLevel(0) * m_nCONMod);
        if (nCurForce + GetLevel(0) * m_nWISMod < GetLevel(0))
            m_pOwner->m_pStats->m_nCurrentForcePoints = GetLevel(0);
        else
            m_pOwner->m_pStats->m_nCurrentForcePoints = nCurForce + GetLevel(0) * m_nWISMod;
        if (nHitPoints > nCurHitPoints) {
            if (nHitPoints + GetLevel(0) * m_nCONMod > GetLevel(0))
                m_pOwner->SetCurrentHitPoints(nCurHitPoints + GetLevel(0) * m_nCONMod);
            else
                m_pOwner->SetCurrentHitPoints((nCurHitPoints - nHitPoints) + GetLevel(0));
        } else {
            if (nHitPoints + GetLevel(0) * m_nCONMod > GetLevel(0))
                m_pOwner->SetCurrentHitPoints(nCurHitPoints + GetLevel(0) * m_nCONMod);
            else
                m_pOwner->SetCurrentHitPoints(GetLevel(0));
        }
        if (nCurForce + GetLevel(0) * m_nWISMod < GetLevel(0))
            m_nCurrentForcePoints = GetLevel(0);
        else
            m_nCurrentForcePoints = nCurForce + GetLevel(0) * m_nWISMod;
        if (nCurForce < nForcePoints) {
            if (nForcePoints + GetLevel(0) * m_nWISMod > GetLevel(0))
                nNewForcePoints = nCurForce + GetLevel(0) * m_nWISMod;
            else
                nNewForcePoints = (nCurForce - nForcePoints) + GetLevel(0);
        } else {
            if (nForcePoints + GetLevel(0) * m_nWISMod > GetLevel(0))
                nNewForcePoints = nCurForce + GetLevel(0) * m_nWISMod;
            else
                nNewForcePoints = GetLevel(0);
        }
    } else {
        short nMaxHitPoints = m_pOwner->GetMaxHitPoints(1);
        m_pOwner->SetCurrentHitPoints(
            static_cast<short>((nMaxHitPoints + nCurHitPoints) - static_cast<unsigned short>(m_pOwner->m_hitPoints)));
        nNewForcePoints = static_cast<unsigned short>((m_pOwner->GetMaxForcePoints() + nCurForce) - m_pOwner->m_pStats->m_nBaseForcePoints);
    }
    m_nCurrentForcePoints = nNewForcePoints;

    short nOldMaxHitPoints = m_pOwner->GetMaxHitPoints(1);
    short nOldMaxForcePoints = m_pOwner->GetMaxForcePoints();

    CResList featList;
    if (pGFF->GetList(&featList, pStruct, const_cast<char*>("FeatList"))) {
        unsigned short nFeatCount = static_cast<unsigned short>(pGFF->GetListCount(&featList));
        if (nFeatCount != 0) {
            for (unsigned short j = 0; j < nFeatCount; j++) {
                CResStruct featElem;
                if (pGFF->GetListElement(&featElem, &featList, j)) {
                    unsigned short nFeat = pGFF->ReadFieldWORD(&featElem, const_cast<char*>("Feat"), success, 0);
                    if (success != 0)
                        AddFeat(nFeat);
                }
            }
        }
    }

    short nNewMaxHitPoints = m_pOwner->GetMaxHitPoints(1);
    short nNewMaxForcePoints = m_pOwner->GetMaxForcePoints();
    if (nNewMaxHitPoints != nOldMaxHitPoints)
        m_pOwner->SetCurrentHitPoints((nNewMaxHitPoints - nOldMaxHitPoints) +
                                      m_pOwner->GetCurrentHitPoints(1));
    if (nNewMaxForcePoints != nOldMaxForcePoints)
        m_nCurrentForcePoints = static_cast<unsigned short>((nNewMaxForcePoints - nOldMaxForcePoints) + m_nCurrentForcePoints);

    CResList perceptionList;
    if (pGFF->GetList(&perceptionList, pStruct, const_cast<char*>("PerceptionList"))) {
        int nPerceptions = pGFF->GetListCount(&perceptionList);
        if (nPerceptions > 0) {
            int i = 0;
            do {
                CResStruct elem;
                if (pGFF->GetListElement(&elem, &perceptionList, i)) {
                    unsigned int nObjectId = pGFF->ReadFieldDWORD(&elem, const_cast<char*>("ObjectId"), success, 0);
                    unsigned char nData = pGFF->ReadFieldBYTE(&elem, const_cast<char*>("PerceptionData"), success, 0);
                    m_pOwner->AddToVisibleList(nObjectId, nData & 1, (nData >> 1) & 1,
                                               (nData >> 2) & 1, (nData >> 3) & 1);
                }
                i++;
            } while (nPerceptions != i);
        }
    }

    CResStruct combatRoundStruct;
    if (pGFF->GetStructFromStruct(&combatRoundStruct, pStruct, const_cast<char*>("CombatRoundData")) == 1)
        m_pOwner->m_pCombatRound->LoadCombatRound(pGFF, &combatRoundStruct);

    pAppearance->m_headId = m_nAppearanceType;
    pAppearance->m_nGender = m_nGender;
    pAppearance->m_nPhenotype = m_nPhenotype;
    pAppearance->m_nAppearanceHead = m_nAppearanceHead;
    pAppearance->m_nDuplicatingHead = m_bDuplicatingHead;
    pAppearance->m_bUseBackupHead = m_nUseBackupHead;
    pAppearance->m_nTail = m_bTail;
    pAppearance->m_nWings = m_bWings;
    pAppearance->m_nColorSkin = m_nColorSkin;
    pAppearance->m_nColorHair = m_nColorHair;
    pAppearance->m_nColorTattoo1 = m_nColorTattoo1;
    pAppearance->m_nColorTattoo2 = m_nColorTattoo2;

    m_nSkillPoints = pGFF->ReadFieldWORD(pStruct, const_cast<char*>("SkillPoints"), success, m_nSkillPoints);
    CResList skillList;
    if (pGFF->GetList(&skillList, pStruct, const_cast<char*>("SkillList"))) {
        m_pSkillRanks[0] = 0;
        m_pSkillRanks[1] = 0;
        m_pSkillRanks[2] = 0;
        m_pSkillRanks[3] = 0;
        m_pSkillRanks[4] = 0;
        m_pSkillRanks[5] = 0;
        m_pSkillRanks[6] = 0;
        m_pSkillRanks[7] = 0;
        int i = 0;
        do {
            CResStruct skillElem;
            if (pGFF->GetListElement(&skillElem, &skillList, i)) {
                char nRank = pGFF->ReadFieldBYTE(&skillElem, const_cast<char*>("Rank"), success,
                                                 GetSkillRank(static_cast<unsigned char>(i), 0, 0));
                SetSkillRank(static_cast<unsigned char>(i), nRank);
            }
            i++;
        } while (i != 8);
    }

    SetMovementRate(pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("MovementRate"), success, static_cast<unsigned char>(m_nMovementRate)));
    if (success == 0)
        SetMovementRate(pGFF->ReadFieldINT(pStruct, const_cast<char*>("WalkRate"), success, m_nMovementRate));

    CResList specAbilityList;
    if (pGFF->GetList(&specAbilityList, pStruct, const_cast<char*>("SpecAbilityList"))) {
        if (static_cast<char>(pGFF->GetListCount(&specAbilityList)) != 0) {
            unsigned char nSpecCount = static_cast<unsigned char>(pGFF->GetListCount(&specAbilityList));
            if (nSpecCount != 0) {
                unsigned int i = 0;
                do {
                    CResStruct specElem;
                    if (pGFF->GetListElement(&specElem, &specAbilityList, i)) {
                        unsigned short nSpell =
                            pGFF->ReadFieldWORD(&specElem, const_cast<char*>("Spell"), success, 0);
                        unsigned char nFlags =
                            pGFF->ReadFieldBYTE(&specElem, const_cast<char*>("SpellFlags"), success, 0);
                        unsigned char nCasterLevel =
                            pGFF->ReadFieldBYTE(&specElem, const_cast<char*>("SpellCasterLevel"), success, 0);
                        AddSpellLikeAbilityToList(nSpell, nFlags, nCasterLevel);
                    }
                    i++;
                } while (nSpecCount != i);
            }
        }
    }

    m_pCombatInformation->LoadData(pGFF, pStruct);
    return 0;
}

// Reads the per-class known-spell lists out of the GFF.
void CSWSCreatureStats::ReadSpellsFromGff(CResGFF* pGFF, CResStruct* pStruct)
{
    int bSuccess;
    CResList classList;
    if (pGFF->GetList(&classList, pStruct, const_cast<char*>("ClassList")) && m_numClasses != 0) {
        for (unsigned int i = 0; i < m_numClasses; i++) {
            CResStruct classElem;
            if (pGFF->GetListElement(&classElem, &classList, i)) {
                CExoString label;
                CResList spellList;
                label.Format(const_cast<char*>("%s%d"), const_cast<char*>("KnownList"), 0);
                if (pGFF->GetList(&spellList, &classElem, static_cast<char*>(label.CStr()))) {
                    for (unsigned short j = 0; j < pGFF->GetListCount(&spellList); j++) {
                        CResStruct spellElem;
                        if (pGFF->GetListElement(&spellElem, &spellList, j)) {
                            unsigned short nSpell = pGFF->ReadFieldWORD(&spellElem, const_cast<char*>("Spell"), bSuccess, 0xffff);
                            AddKnownSpell(static_cast<unsigned char>(i), nSpell);
                        }
                    }
                }
            }
        }
    }
}

// Adds a spell to a class slot's level-0 known-spell list.
void CSWSCreatureStats::AddKnownSpell(unsigned char nClass, unsigned int nSpell)
{
    if (nClass >= m_numClasses)
        return;
    if (g_pRules->GetSpell(nSpell) == 0)
        return;
    if (m_bIsPC != 0 && !CSWClass::IsJedi(GetClass(nClass)))
        return;
    CExoArrayList<unsigned int>& list = m_classInfo[nClass].m_knownSpells[0];
    int nCount = list.m_count;
    int nIndex = -1;
    int64_t j = 0;
    while (true) {
        if (j >= nCount)
            break;
        if (list.m_pArray[j] == nSpell) {
            nIndex = static_cast<int>(j);
            break;
        }
        j++;
    }
    if (nIndex != -1)
        return;
    list.Add(nSpell);
}

// The identity half of the .bic GFF write.
void CSWSCreatureStats::SaveStats(CResGFF* pGFF, CResStruct* pStruct)
{
    pGFF->WriteFieldCExoLocString(pStruct, m_firstName, const_cast<char*>("FirstName"));
    pGFF->WriteFieldCExoLocString(pStruct, m_lastName, const_cast<char*>("LastName"));
    pGFF->WriteFieldCExoLocString(pStruct, m_description, const_cast<char*>("Description"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bIsPC), const_cast<char*>("IsPC"));
    pGFF->WriteFieldCExoString(pStruct, m_pOwner->m_sTag, const_cast<char*>("Tag"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_conversation), const_cast<char*>("Conversation"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_nInterruptable), const_cast<char*>("Interruptable"));
    pGFF->WriteFieldINT(pStruct, m_nAge, const_cast<char*>("Age"));
    pGFF->WriteFieldBYTE(pStruct, m_nGender, const_cast<char*>("Gender"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_nRace), const_cast<char*>("Race"));
    pGFF->WriteFieldCExoString(pStruct, m_sSubrace, const_cast<char*>("Subrace"));
    pGFF->WriteFieldBYTE(pStruct, m_nSubraceIndex, const_cast<char*>("SubraceIndex"));
    pGFF->WriteFieldBYTE(pStruct, m_nStartingPackage, const_cast<char*>("StartingPackage"));
    pGFF->WriteFieldCExoString(pStruct, m_sDeity, const_cast<char*>("Deity"));
    pGFF->WriteFieldBYTE(pStruct, m_numClasses - 1, const_cast<char*>("MClassLevUpIn"));
    pGFF->WriteFieldSHORT(pStruct, m_nWillSaveBonus, const_cast<char*>("willbonus"));
    pGFF->WriteFieldSHORT(pStruct, m_nFortSaveBonus, const_cast<char*>("fortbonus"));
    pGFF->WriteFieldSHORT(pStruct, m_nReflexSaveBonus, const_cast<char*>("refbonus"));
    pGFF->WriteFieldDWORD(pStruct, m_pOwner->GetGold(), const_cast<char*>("Gold"));
#if !KOTOR1
    pGFF->WriteFieldDWORD(pStruct, m_pOwner->GetComponent(), const_cast<char*>("ItemComponent"));
    pGFF->WriteFieldDWORD(pStruct, m_pOwner->GetChemical(), const_cast<char*>("Chemicals"));
#endif
    {
        int nFx = 0;
        int nBonus = m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_SAVING_THROW, 0, nFx, 0, 0, 2,
                                                   0, CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, 0, 0);
        int nDebilitated = m_pOwner->IsDebilitated(1);
        int nDexMod = (m_nDEXMod < 0 || nDebilitated == 0) ? m_nDEXMod : 0;
        pGFF->WriteFieldCHAR(pStruct, static_cast<char>(nBonus + GetBaseReflexSavingThrow() + nDexMod + m_nReflexSaveBonus),
                             const_cast<char*>("RefSaveThrow"));
        nFx = 0;
        nBonus = m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_SAVING_THROW, 0, nFx, 0, 0, 3, 0,
                                               CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, 0, 0);
        pGFF->WriteFieldCHAR(pStruct, static_cast<char>(m_nWISMod + nBonus + GetBaseWillSavingThrow() + m_nWillSaveBonus),
                             const_cast<char*>("WillSaveThrow"));
        nFx = 0;
        nBonus = m_pOwner->GetTotalEffectBonus(CSWSCreature::EFFECT_BONUS_SAVING_THROW, 0, nFx, 0, 0, 1, 0,
                                               CSWSCreature::EFFECT_BONUS_ANY, CSWSCreature::EFFECT_BONUS_ANY, 0, 0);
        pGFF->WriteFieldCHAR(pStruct, static_cast<char>(m_nCONMod + nBonus + GetBaseFortSavingThrow() + m_nFortSaveBonus),
                             const_cast<char*>("FortSaveThrow"));
    }
    pGFF->WriteFieldSHORT(pStruct, m_pOwner->GetArmorClass(), const_cast<char*>("ArmorClass"));
    pGFF->WriteFieldBYTE(pStruct, m_nSTRBase, const_cast<char*>("Str"));
    pGFF->WriteFieldBYTE(pStruct, m_nDEXBase, const_cast<char*>("Dex"));
    pGFF->WriteFieldBYTE(pStruct, m_nINTBase, const_cast<char*>("Int"));
    pGFF->WriteFieldBYTE(pStruct, m_nWISBase, const_cast<char*>("Wis"));
    pGFF->WriteFieldBYTE(pStruct, m_nCONBase, const_cast<char*>("Con"));
    pGFF->WriteFieldBYTE(pStruct, m_nCHABase, const_cast<char*>("Cha"));
    pGFF->WriteFieldBYTE(pStruct, m_nACNaturalBase, const_cast<char*>("NaturalAC"));
    pGFF->WriteFieldWORD(pStruct, m_pOwner->m_nSoundSetFile, const_cast<char*>("SoundSetFile"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_pOwner->m_bPlot), const_cast<char*>("Plot"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_pOwner->m_bDisarmable), const_cast<char*>("Disarmable"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_pOwner->m_bMin1HP), const_cast<char*>("Min1HP"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_pOwner->m_bPartyInteract), const_cast<char*>("PartyInteract"));
    pGFF->WriteFieldBYTE(pStruct, m_pOwner->m_bOrientOnClick == 0, const_cast<char*>("NotReorienting"));
#if !KOTOR1
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_pOwner->m_bHologram), const_cast<char*>("Hologram"));
    pGFF->WriteFieldBYTE(pStruct, m_pOwner->m_bIgnoreCrePath, const_cast<char*>("IgnoreCrePath"));
    pGFF->WriteFieldBYTE(pStruct, m_pOwner->m_nMultiplierSet, const_cast<char*>("MultiplierSet"));
    pGFF->WriteFieldBYTE(pStruct, m_pOwner->m_nPCLevelAtSpawn, const_cast<char*>("PCLevelAtSpawn"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_pOwner->m_bWillNotRender), const_cast<char*>("WillNotRender"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_pOwner->m_bConfused), const_cast<char*>("Confused"));
#endif
    pGFF->WriteFieldBYTE(pStruct, m_pOwner->m_bBodyBag, const_cast<char*>("BodyBag"));
    pGFF->WriteFieldSHORT(pStruct, static_cast<short>(m_pOwner->m_hitPoints), const_cast<char*>("HitPoints"));
    pGFF->WriteFieldSHORT(pStruct,
                          static_cast<short>(static_cast<short>(m_pOwner->GetCurrentHitPoints(1) - m_pOwner->GetMaxHitPoints(1)) +
                              static_cast<unsigned short>(m_pOwner->m_hitPoints)),
                          const_cast<char*>("CurrentHitPoints"));
    pGFF->WriteFieldSHORT(pStruct, m_pOwner->GetMaxHitPoints(1), const_cast<char*>("MaxHitPoints"));
    pGFF->WriteFieldSHORT(pStruct, m_pOwner->GetCurrentHitPoints(1), const_cast<char*>("PregameCurrent"));
    pGFF->WriteFieldSHORT(pStruct, m_pOwner->m_pStats->m_nBaseForcePoints, const_cast<char*>("ForcePoints"));
    pGFF->WriteFieldSHORT(pStruct,
                          static_cast<short>(m_pOwner->m_pStats->m_nCurrentForcePoints - m_pOwner->GetMaxForcePoints() +
                              m_pOwner->m_pStats->m_nBaseForcePoints),
                          const_cast<char*>("CurrentForce"));
#if !KOTOR1
    pGFF->WriteFieldINT(pStruct, m_pOwner->m_nBonusForcePoints, const_cast<char*>("BonusForcePoints"));
    pGFF->WriteFieldINT(pStruct, m_pOwner->m_nAssignedPup, const_cast<char*>("AssignedPup"));
    pGFF->WriteFieldINT(pStruct, m_pOwner->m_bPlayerMadeCharacter, const_cast<char*>("PlayerCreated"));
#endif
    pGFF->WriteFieldSHORT(pStruct, m_pOwner->GetMaxForcePoints(), const_cast<char*>("MaxForcePoints"));
    pGFF->WriteFieldDWORD(pStruct, m_nExperience, const_cast<char*>("Experience"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_nMovementRate), const_cast<char*>("MovementRate"));
    if (m_pOwner->GetPortraitId() != CSWSObject::PORTRAIT_ID_NONE) {
        pGFF->WriteFieldWORD(pStruct, m_pOwner->GetPortraitId(), const_cast<char*>("PortraitId"));
    } else {
        CResRef portrait = m_pOwner->GetPortrait();
        pGFF->WriteFieldCResRef(pStruct, portrait, const_cast<char*>("Portrait"));
    }
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_nAlignment), const_cast<char*>("GoodEvil"));
#if !KOTOR1
    pGFF->WriteFieldCHAR(pStruct, static_cast<char>(m_nBaseNPCAlignment), const_cast<char*>("BaseCNPCAlignment"));
    pGFF->WriteFieldCHAR(pStruct, static_cast<char>(m_nFuryDamageBonus), const_cast<char*>("FuryDamageBonus"));
    pGFF->WriteFieldDWORD(pStruct, m_pOwner->m_stanceId, const_cast<char*>("CurrentForm"));
#endif
    pGFF->WriteFieldBYTE(pStruct, m_nColorSkin, const_cast<char*>("Color_Skin"));
    pGFF->WriteFieldBYTE(pStruct, m_nColorHair, const_cast<char*>("Color_Hair"));
    pGFF->WriteFieldBYTE(pStruct, m_nColorTattoo1, const_cast<char*>("Color_Tattoo1"));
    pGFF->WriteFieldBYTE(pStruct, m_nColorTattoo2, const_cast<char*>("Color_Tattoo2"));
    pGFF->WriteFieldINT(pStruct, m_nPhenotype, const_cast<char*>("Phenotype"));
    pGFF->WriteFieldWORD(pStruct, m_nAppearanceType, const_cast<char*>("Appearance_Type"));
    pGFF->WriteFieldBYTE(pStruct, m_nAppearanceHead, const_cast<char*>("Appearance_Head"));
    pGFF->WriteFieldBYTE(pStruct, m_bDuplicatingHead, const_cast<char*>("DuplicatingHead"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_nUseBackupHead), const_cast<char*>("UseBackupHead"));
    pGFF->WriteFieldBYTE(pStruct, m_bTail, const_cast<char*>("Tail"));
    pGFF->WriteFieldBYTE(pStruct, m_bWings, const_cast<char*>("Wings"));
    pGFF->WriteFieldWORD(pStruct, static_cast<unsigned short>(m_factionId), const_cast<char*>("FactionID"));
#if !KOTOR1
    pGFF->WriteFieldFLOAT(pStruct, m_fBlindSpot, const_cast<char*>("BlindSpot"));
#endif
    pGFF->WriteFieldFLOAT(pStruct, m_fChallengeRating, const_cast<char*>("ChallengeRating"));
    pGFF->WriteFieldINT(pStruct, m_aiState, const_cast<char*>("AIState"));
    SaveClassInfo(pGFF, pStruct);

    CExoString sName;
    CSWBaseItem* pBaseItem;
    CSWSItem* pItem = static_cast<CSWSItem*>(g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
        m_pCombatInformation->m_offHandWeaponId));
    if (pItem != 0)
        pItem->m_localizedName.GetString(0, &m_pCombatInformation->m_offHandWeaponName, 0);
    pItem = static_cast<CSWSItem*>(g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(
        m_pCombatInformation->m_onHandWeaponId));
    if (pItem != 0) {
        pItem->m_localizedName.GetString(0, &m_pCombatInformation->m_onHandWeaponName, 0);
        pBaseItem = g_pRules->GetBaseItem(pItem->m_baseItem);
        if (pBaseItem != 0) {
            CCombatInformation* pCombatInfo = m_pCombatInformation;
            pCombatInfo->m_nOverrideDie = pBaseItem->m_nNumDice;
            pCombatInfo->m_nOverrideNumDice = pBaseItem->m_nDieToRoll;
        }
    }
    m_pCombatInformation->SaveData(pGFF, pStruct);
}

// The class/level-up half of the .bic GFF write.
void CSWSCreatureStats::SaveClassInfo(CResGFF* pGFF, CResStruct* pStruct)
{
    CExoString label;
    CResList classList;
    pGFF->AddList(&classList, pStruct, const_cast<char*>("ClassList"));
    for (int64_t i = 0; i < m_numClasses; i++) {
        CResStruct classElem;
        pGFF->AddListElement(&classElem, &classList, 2);
        pGFF->WriteFieldINT(&classElem, m_classInfo[i].m_class, const_cast<char*>("Class"));
        pGFF->WriteFieldSHORT(&classElem,
                              (i < m_numClasses) ? m_classInfo[i].m_level : 0, const_cast<char*>("ClassLevel"));
        if (CSWClass::IsJedi((i < m_numClasses) ? m_classInfo[i].m_class : CLASS_TYPE_INVALID)) {
            CResList spellsPerDayList;
            pGFF->AddList(&spellsPerDayList, &classElem, const_cast<char*>("SpellsPerDayList"));
            CResStruct spdElem;
            pGFF->AddListElement(&spdElem, &spellsPerDayList, GFF_STRUCT_SPELLS_PER_DAY);
            pGFF->WriteFieldBYTE(&spdElem,
                                 (i < m_numClasses) ? m_classInfo[i].m_spellsPerDayLeft : 0,
                                 const_cast<char*>("NumSpellsLeft"));
        }
        if (i < m_numClasses) {
            short nKnown = static_cast<short>(m_classInfo[i].m_knownSpells[0].m_count);
            if (nKnown >= 1) {
                label.Format(const_cast<char*>("%s%d"), const_cast<char*>("KnownList"), 0);
                CResList knownList;
                pGFF->AddList(&knownList, &classElem, static_cast<char*>(label.CStr()));
                for (unsigned char j = 0; j < nKnown; j++) {
                    CResStruct spellElem;
                    pGFF->AddListElement(&spellElem, &knownList, 3);
                    pGFF->WriteFieldWORD(&spellElem, static_cast<unsigned short>(GetKnownSpell(static_cast<unsigned char>(i), 0, j)),
                                         const_cast<char*>("Spell"));
                }
            }
        }
    }

    if (m_bIsPC != 0 && g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nControlledNPC == -1) {
        CResList lvlStatList;
        pGFF->AddList(&lvlStatList, pStruct, const_cast<char*>("LvlStatList"));
        for (int64_t n = 0; n < GetLevel(0); n++) {
            CSWLevelStats* pLvl = GetLevelStats(static_cast<unsigned char>(n));
            CResStruct lvlElem;
            pGFF->AddListElement(&lvlElem, &lvlStatList, 0);
            if (n != 0 && pLvl->m_nAbilityScoreIncrease != 6)
                pGFF->WriteFieldBYTE(&lvlElem, pLvl->m_nAbilityScoreIncrease, const_cast<char*>("LvlStatAbility"));
            pGFF->WriteFieldBYTE(&lvlElem, pLvl->m_nHitPointGain, const_cast<char*>("LvlStatHitDie"));
            pGFF->WriteFieldBYTE(&lvlElem, pLvl->m_nForcePointGain, const_cast<char*>("LvlStatForce"));
            pGFF->WriteFieldBYTE(&lvlElem, pLvl->m_nClass, const_cast<char*>("LvlStatClass"));
            int nGained = pLvl->m_spellsGained.m_count;
            if (static_cast<unsigned char>(nGained) != 0) {
                CExoString lvlLabel;
                lvlLabel.Format(const_cast<char*>("%s%d"), const_cast<char*>("KnownList"), 0);
                CResList gainedList;
                pGFF->AddList(&gainedList, &lvlElem, static_cast<char*>(lvlLabel.CStr()));
                for (int j = 0; j < static_cast<unsigned char>(nGained); j++) {
                    CResStruct spellElem;
                    pGFF->AddListElement(&spellElem, &gainedList, 0);
                    pGFF->WriteFieldWORD(&spellElem, static_cast<unsigned short>(pLvl->m_spellsGained.m_pArray[j]),
                                         const_cast<char*>("Spell"));
                }
            }
            int nLost = pLvl->m_spellsLost.m_count;
            if (static_cast<unsigned char>(nLost) != 0) {
                CExoString lvlLabel;
                lvlLabel.Format(const_cast<char*>("%s%d"), const_cast<char*>("KnownRemoveList"), 0);
                CResList lostList;
                pGFF->AddList(&lostList, &lvlElem, static_cast<char*>(lvlLabel.CStr()));
                for (int j = 0; j < static_cast<unsigned char>(nLost); j++) {
                    CResStruct spellElem;
                    pGFF->AddListElement(&spellElem, &lostList, 0);
                    pGFF->WriteFieldWORD(&spellElem, static_cast<unsigned short>(pLvl->m_spellsLost.m_pArray[j]),
                                         const_cast<char*>("Spell"));
                }
            }
            pGFF->WriteFieldWORD(&lvlElem, pLvl->m_nSkillPoints, const_cast<char*>("SkillPoints"));
            CResList skillList;
            pGFF->AddList(&skillList, &lvlElem, const_cast<char*>("SkillList"));
            if (g_pRules->m_nNumSkills != 0) {
                unsigned char k = 0;
                do {
                    CResStruct skillElem;
                    pGFF->AddListElement(&skillElem, &skillList, 0);
                    pGFF->WriteFieldBYTE(&skillElem, pLvl->GetSkillRankChange(k), const_cast<char*>("Rank"));
                    k++;
                } while (k < g_pRules->m_nNumSkills);
            }
            CResList featList;
            pGFF->AddList(&featList, &lvlElem, const_cast<char*>("FeatList"));
            if (static_cast<unsigned short>(pLvl->m_featsGained.m_count) != 0) {
                for (int64_t j = 0; j < static_cast<unsigned short>(pLvl->m_featsGained.m_count); j++) {
                    CResStruct featElem;
                    pGFF->AddListElement(&featElem, &featList, 0);
                    pGFF->WriteFieldWORD(&featElem, pLvl->m_featsGained.m_pArray[j], const_cast<char*>("Feat"));
                }
            }
        }
    }

    pGFF->WriteFieldWORD(pStruct, m_nSkillPoints, const_cast<char*>("SkillPoints"));
    CResList skillList;
    pGFF->AddList(&skillList, pStruct, const_cast<char*>("SkillList"));
    for (int i = 0; i < 8; i++) {
        CResStruct skillElem;
        pGFF->AddListElement(&skillElem, &skillList, 0);
#if KOTOR1
        int nRank = GetSkillRank(static_cast<unsigned char>(i), 0, 1);
#else
        int nRank = 0;
        if (i < g_pRules->m_nNumSkills) {
            nRank = m_pSkillRanks[i];
            if (nRank > 127)
                nRank = 127;
            if (nRank < -127)
                nRank = -127;
        }
#endif
        pGFF->WriteFieldBYTE(&skillElem, static_cast<unsigned char>(nRank), const_cast<char*>("Rank"));
    }

    if (m_feats.m_count > 0) {
        CResList featList;
        pGFF->AddList(&featList, pStruct, const_cast<char*>("FeatList"));
        for (unsigned short j = 0; j < m_feats.m_count; j++) {
            CResStruct featElem;
            pGFF->AddListElement(&featElem, &featList, 1);
            pGFF->WriteFieldWORD(&featElem, GetFeat(j), const_cast<char*>("Feat"));
        }
    }

    if (m_pSpellLikeAbilityList->m_count >= 1) {
        CResList specAbilityList;
        pGFF->AddList(&specAbilityList, pStruct, const_cast<char*>("SpecAbilityList"));
        for (int64_t i = 0; i < m_pSpellLikeAbilityList->m_count; i++) {
            CResStruct specElem;
            pGFF->AddListElement(&specElem, &specAbilityList, 4);
            CSWSStats_SpellLikeAbility* pAbility = &m_pSpellLikeAbilityList->m_pArray[i];
            pGFF->WriteFieldWORD(&specElem, static_cast<unsigned short>(pAbility->m_nSpellId), const_cast<char*>("Spell"));
            pGFF->WriteFieldBYTE(&specElem, static_cast<unsigned char>(pAbility->m_nReady), const_cast<char*>("SpellFlags"));
            pGFF->WriteFieldBYTE(&specElem, pAbility->m_nCasterLevel, const_cast<char*>("SpellCasterLevel"));
        }
    }
}

// Settles the force-point cost of a spell going un-ready.
int CSWSCreatureStats::UnReadySpell(unsigned int nSpellId)
{
    CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
#if KOTOR1
// K1: the cost comes out of the bonus pool first, then the main pool.
    int bResult = 0;
    if (pSpell != 0) {
        int nCost = pSpell->GetAdjustedForcePoints(m_nAlignment, m_pOwner->m_bInParty, 0, 0, 0);
        short nBonus = m_nTemporaryForcePoints;
        if (nCost <= nBonus) {
            m_nTemporaryForcePoints = static_cast<unsigned short>(nBonus - nCost);
            return 1;
        }
        m_nTemporaryForcePoints = 0;
        short nLeft = static_cast<short>(m_nCurrentForcePoints + static_cast<unsigned short>(nBonus - nCost));
        m_nCurrentForcePoints = nLeft & ~(nLeft >> 15);
        bResult = 1;
    }
    return bResult;
#else
    if (pSpell == 0)
        return 0;
    int nForceRating = 0;
    CSWSArea* pArea = static_cast<CSWSArea*>(m_pOwner->GetArea());
    if (pArea != 0)
        nForceRating = pArea->GetRoomForceRating(m_pOwner->m_position);
    int nForceBodyLevel = CSWSEffectListHandler::FORCE_BODY_LEVEL_NONE;
    for (int64_t k = 0; k < m_pOwner->m_effects.m_count; k++) {
        CGameEffect* pEffect = m_pOwner->m_effects.m_pArray[k];
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_FORCE_BODY) {
            nForceBodyLevel = pEffect->GetInteger(0);
            break;
        }
    }
    int nCost = pSpell->GetAdjustedForcePoints(m_nAlignment, m_pOwner->m_bInParty,
                                               nForceRating, m_pOwner->m_stanceId,
                                               m_pOwner->m_pStats->m_nCHAMod);
// nPct survives the guard as the HP cost.
    int nPct = 0;
    if (nForceBodyLevel != CSWSEffectListHandler::FORCE_BODY_LEVEL_NONE) {
        switch (nForceBodyLevel) {
        case CSWSEffectListHandler::FORCE_BODY_LEVEL_BASIC:    nPct = 50; break;
        case CSWSEffectListHandler::FORCE_BODY_LEVEL_IMPROVED: nPct = 40; break;
        case CSWSEffectListHandler::FORCE_BODY_LEVEL_MASTER:   nPct = 30; break;
        }
        nCost = nPct * nCost / 100;
        nPct = nCost;
    }
    if (nCost <= static_cast<short>(m_nTemporaryForcePoints)) {
        m_nTemporaryForcePoints -= static_cast<unsigned short>(nCost);
        return 1;
    }
    short nLeft = static_cast<short>(m_nCurrentForcePoints + m_nTemporaryForcePoints - nCost);
    m_nTemporaryForcePoints = 0;
    if (nLeft < 0)
        nLeft = 0;
    m_nCurrentForcePoints = nLeft;
    short nBonus = static_cast<short>(m_pOwner->m_hitPointBonus);
    if (nBonus < nPct) {
        m_pOwner->m_hitPointBonus = 0;
        m_pOwner->SetCurrentHitPoints(nBonus - nPct + m_pOwner->GetCurrentHitPoints(1));
        if (m_pOwner->GetCurrentHitPoints(0) < 0)
            m_pOwner->SetCurrentHitPoints(0);
    } else {
        m_pOwner->m_hitPointBonus = nBonus - nPct;
    }
    return 1;
#endif
}
