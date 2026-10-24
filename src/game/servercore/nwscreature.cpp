#include "k2stdint.h"
#include "CExoArrayListImpl.h"
#include "nullname.h"
#include "nwscreature.h"
#include "nwscriptconst.h"
#include "nwmessagetypes.h"
#include "enginevfx.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "serverexoapp.h"
#include "partytables.h"
#include "itemtype.h"
#include "nwsaiaction.h"
#include "restype.h"
#include "handmaidenappearance.h"
#include "strref.h"

// GetForceItemMask bits, one per kind of equipped item that hampers the Force.
enum {
    FORCE_ITEM_ARMOR_CLASS_4 = 1 << 0,
    FORCE_ITEM_ARMOR_CLASS_5 = 1 << 1,
    FORCE_ITEM_ARMOR_CLASS_6 = 1 << 2,
    FORCE_ITEM_ARMOR_CLASS_7 = 1 << 3,
    FORCE_ITEM_ARMOR_CLASS_8 = 1 << 4,
    FORCE_ITEM_ARMOR_CLASS_9 = 1 << 5,
    FORCE_ITEM_LIGHTSABER    = 1 << 6,
};

// spells.2da rows of the thrown grenades and of the wrist-launched versions ConvertGrenadeSpellID
// swaps them for.
enum {
    SPELL_GRENADE_FRAGMENTATION              = 87,
    SPELL_GRENADE_STUN                       = 88,
    SPELL_GRENADE_THERMAL_DETONATOR          = 89,
    SPELL_GRENADE_POISON                     = 90,
    SPELL_GRENADE_SONIC                      = 91,
    SPELL_GRENADE_ADHESIVE                   = 92,
    SPELL_GRENADE_CRYOBAN                    = 93,
    SPELL_GRENADE_FIRE                       = 94,
    SPELL_GRENADE_ION                        = 95,
    SPELL_GRENADE_SONIC_2                    = 186,
    SPELL_GRENADE_SONIC_3                    = 187,
    SPELL_GRENADE_LAUNCHED_FRAGMENTATION     = 246,
    SPELL_GRENADE_LAUNCHED_STUN              = 247,
    SPELL_GRENADE_LAUNCHED_THERMAL_DETONATOR = 248,
    SPELL_GRENADE_LAUNCHED_POISON            = 249,
    SPELL_GRENADE_LAUNCHED_SONIC             = 250,
    SPELL_GRENADE_LAUNCHED_ADHESIVE          = 251,
    SPELL_GRENADE_LAUNCHED_CRYOBAN           = 252,
    SPELL_GRENADE_LAUNCHED_FIRE              = 253,
    SPELL_GRENADE_LAUNCHED_ION               = 254,
    SPELL_GRENADE_LAUNCHED_SONIC_2           = 255,
    SPELL_GRENADE_LAUNCHED_SONIC_3           = 256,
};

// The user-defined event number the area is signalled with when a party member breaks one of the
// party table's forfeit conditions.
enum {
    USER_EVENT_FORFEIT_VIOLATED = 4001,
};

// npc.2da rows past the companions: the kill-XP percentage and the per-member bonus.
enum {
#if KOTOR1
    NPC_ROW_GAME_XP_GENERAL = 9,
    NPC_ROW_PER_NPC_BONUS   = 10,
#else
    NPC_ROW_GAME_XP_GENERAL = 13,
    NPC_ROW_PER_NPC_BONUS   = 14,
#endif
};

// ranges.2da rows.
enum {
    RANGES_ROW_PERCEPTION_DEFAULT = 11,
    RANGES_ROW_PARTY_COMBAT       = 18,
};

// CSWVisibilityNode::m_flags: seen (bit 0), heard (bit 1), the pacify state (bits 2-3) and
// invisible (bit 4).
enum {
    VISIBILITY_SEEN              = 1 << 0,
    VISIBILITY_HEARD             = 1 << 1,
    VISIBILITY_PACIFIED          = 1 << 2,
    VISIBILITY_PACIFY_RESISTED   = 1 << 3,
    VISIBILITY_PACIFY_STATE_MASK = VISIBILITY_PACIFIED | VISIBILITY_PACIFY_RESISTED,
    VISIBILITY_INVISIBLE         = 1 << 4,
};

// classes.2da "AlignRestrict" bits GetIsAlignmentRestricted tests.
enum {
    ALIGN_RESTRICT_NEUTRAL = 1 << 0,
    ALIGN_RESTRICT_GOOD    = 1 << 3,
    ALIGN_RESTRICT_EVIL    = 1 << 4,
};

// Flag bits AddCastSpellActions ors into the cast action's path-type word.
enum : unsigned int {
    CAST_FLAG_ATTACK     = 1u << 30,
    CAST_FLAG_PROJECTILE = 1u << 31,
};

// The skill filter value meaning every skill.
enum {
    SKILL_ALL = 0xff,
};

// GUI action-list ids with this bit set name an item (the low bits are its object id).
enum {
    GUI_ACTION_ITEM_FLAG = 1 << 30,
};

// The GFF struct id of the "CombatRoundData" struct.
enum {
    GFF_STRUCT_COMBAT_ROUND = 0xcada,
};

// The visualeffects.2da id ApplyDiseasePayload plays with each payload. Neither game's
// visualeffects.2da has the row.
enum {
    VFX_IMP_DISEASE_PAYLOAD = 84,
};

// Placeable animation ids (the ANIMATION_PLACEABLE_* script ids map onto these).
enum {
    PLACEABLE_ANIM_OPEN  = 10075,
    PLACEABLE_ANIM_CLOSE = 10076,
};

const float CSWSCreature::MOVE_AWAY_RADIUS = 40.0f;

extern int g_nJournalSortType;

#include "port/libcbase.h"
#include "port/libcmath.h"

#include "nwsitem.h"
#include "nwsplaceable.h"
#include "nwsitemproperty.h"
#include "nwcobject.h"
#include "CExoArrayListDtors.h"
#include "exostring.h"
#include "nwcreaturelevelstats.h"
#include "nwclass.h"
#include "nwrace.h"
#include "nwseffectlist.h"
#include "nwseffect.h"
#include "Log_Base2.h"
#include "nwitem.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "nwsarea.h"
#include "nwsambientsound.h"
#include "nwsspellimpact.h"
#include "gameobjectarray.h"
#include "serveraimaster.h"
#include "nwsscriptevent.h"
#include "nwssendplayermessage.h"
#include "factionmanager.h"
#include "serverexoappinternal.h"
#include "exolinkedlistinternal.h"
#include "nwsobject.h"
#include "worldtimer.h"
#include "nwscombatround.h"
#include "nwsplayer.h"
#include "nwsplayerluo.h"
#include "nwccreature.h"
#include "nwccreatureappearance.h"
#include "nwmessage.h"
#include "resgff.h"
#include "nwspell.h"
#include "nwskill.h"
#include "nwscript.h"
#include "nwsstore.h"
#include "nwsencounter.h"
#include "swparty.h"
#include "nwsmodule.h"
#include "nwsjournal.h"
#include "nwsparty.h"
#include "nwsdoor.h"
#include "nwstrigger.h"
#include "serverinfo.h"
#include "nwsforcedaction.h"
#include "clientoptions.h"
#include "globalvartables.h"
#include "nwcmodule.h"
#include "exobase.h"

int HurtPlayers;
extern int RenderAi;
int RenderHP;
int RenderDEV;
int RenderQA;
#if KOTOR1
extern int ZeroForce;
#endif

int g_bEnableMovementSpeedDebugging = 1;

extern int g_bDisableDamage;

#include "port/crtapi.h"



float g_fInjuredLevel = 0.200000003f;

int g_nIncreaseDamage = 1;

int g_bEnableCombatDebugging;


#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWSCreature::SetAttemptedAttackTarget(unsigned long long objectId) {
    m_attemptedAttackTargetId = objectId;
}
#endif  // !KOTOR1

unsigned int CSWSCreature::GetEffectSpellId() {
    return m_effectSpellId;
}

void CSWSCreature::SetEffectSpellId(unsigned int nSpellId) {
    m_effectSpellId = nSpellId;
}

unsigned long long CSWSCreature::GetLockOrientationToObject() {
    return m_lockOrientationToObjectId;
}

int CSWSCreature::GetActivity(int type) {
    return (m_activityFlags & type) != 0;
}

void CSWSCreature::ResolveInitiative() {
    m_bInitiativeExpired = (m_bInParty == 0);
}

CExoLocString& CSWSCreature::GetFirstName() {
    if (m_pStats == 0)
        return m_sName;
    return m_pStats->m_firstName;
}

CExoLocString& CSWSCreature::GetLastName() {
    if (m_pStats == 0)
        return m_sName;
    return m_pStats->m_lastName;
}

void CSWSCreature::ResetUpdateTimes(unsigned int day, unsigned int tod) {
    m_nLastHeartbeatDay = day;
    m_nLastHeartbeatTod = tod;
    m_nLastVisibilityUpdateDay = day;
    m_nLastVisibilityUpdateTod = tod;
    m_nLastAIUpdateDay = day;
    m_nLastAIUpdateTod = tod;
}

void CSWSCreature::SetBroadcastedAOOTo(int a) {
    m_broadcastedAOO = a;
    if (a == 1)
        m_nBroadcastedAOOTimer = 1500;
}

void CSWSCreature::SetSilentMove(int bSilent) {
    m_silentMoveCount += bSilent ? 1 : -1;
}

void CSWSCreature::SetLastInstantEquip(unsigned int slot, int value) {
    if (slot == CSWInventory::SLOT_RIGHTWEAPON)
        m_bLastInstantEquipRight = value;
    else if (slot == CSWInventory::SLOT_LEFTWEAPON)
        m_bLastInstantEquipLeft = value;
}

int CSWSCreature::GetLastInstantEquip(unsigned int slot) {
    if (slot == CSWInventory::SLOT_RIGHTWEAPON)
        return m_bLastInstantEquipRight;
    if (slot == CSWInventory::SLOT_LEFTWEAPON)
        return m_bLastInstantEquipLeft;
    return 0;
}

unsigned char CSWSCreature::GetDetectMode() {
    unsigned char mode = m_detectMode;
    if (m_nAnimation != SERVER_ANIMATION_PAUSE)
        return mode;
    if (mode == 1)
        return mode;
    return 2;
}

// Computes the maximum force points as either the per-level advancement record's formula (WIS
// modifier * level + base points) or a sum of max(WIS modifier + level gain, 1) over the levels
// that grant force points, then adds the feat, bonus-point and dark-side-multiclass bonuses.
#if !KOTOR1
short CSWSCreature::GetMaxForcePoints() {
    if (m_pStats->m_nRace == CSWRace::CRACE_DROID)
        return 0;

    unsigned char nLevel = m_pStats->GetLevel(0);

    if (m_pStats->m_bIsPC == 0 ||
        g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nControlledNPC != -1) {
        short nForcePoints = static_cast<short>(m_pStats->m_nBaseForcePoints + m_pStats->m_nWISMod * nLevel);
        if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_FORCE_SENSITIVE) != 0)
            nForcePoints += 40;
#if !KOTOR1  // K1: no bonus force points
        nForcePoints = static_cast<short>(nForcePoints + m_nBonusForcePoints);
#endif
        for (int i = 0; i < m_pStats->m_numClasses; i++) {
            unsigned char nClass = m_pStats->m_classInfo[i].m_class;
            if (nClass == CSWClass::CCLASS_JEDICONSULAR ||
                nClass == CSWClass::CCLASS_SITHLORD ||
                nClass == CSWClass::CCLASS_JEDIMASTER) {
                CGameEffect** ppEffect = m_effects.m_pArray;
                int64_t j = m_effects.m_count;
                if (j > 0) {
                    do {
                        if (CSWSEffectListHandler::EFFECT_PUREEVILPOWERS == (*ppEffect)->m_subType) {
                            nForcePoints += 50;
                            goto legacyDone;
                        }
                        ppEffect++;
                    } while (--j != 0);
                }
            }
        }
    legacyDone:
        if (m_pStats->GetIsJedi() == 0)
            return 0;
        return nLevel > nForcePoints ? nLevel : nForcePoints;
    }

    if (CSWClass::IsJedi(m_pStats->GetClass(m_pStats->m_numClasses - 1)) == 0)
        return 0;

    int nForcePoints = 0;
    for (int nCurLevel = 0; nCurLevel < nLevel; nCurLevel++) {
        unsigned char nGain = m_pStats->GetLevelStats(static_cast<unsigned char>(nCurLevel))->m_nForcePointGain;
        if (nGain != 0) {
            int nAdd = m_pStats->m_nWISMod + nGain;
            if (nAdd < 1)
                nAdd = 1;
            nForcePoints += nAdd;
        }
    }
    if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_FORCE_SENSITIVE) != 0)
        nForcePoints += 40;
#if !KOTOR1  // K1: no bonus force points
    nForcePoints += m_nBonusForcePoints;
#endif
    for (int i = 0; i < m_pStats->m_numClasses; i++) {
        unsigned char nClass = m_pStats->m_classInfo[i].m_class;
        if (nClass == CSWClass::CCLASS_JEDICONSULAR ||
            nClass == CSWClass::CCLASS_SITHLORD ||
            nClass == CSWClass::CCLASS_JEDIMASTER) {
            CGameEffect** ppEffect = m_effects.m_pArray;
            int64_t j = m_effects.m_count;
            if (j > 0) {
                do {
                    if (CSWSEffectListHandler::EFFECT_PUREEVILPOWERS == (*ppEffect)->m_subType) {
                        nForcePoints += 50;
                        goto perLevelDone;
                    }
                    ppEffect++;
                } while (--j != 0);
            }
        }
    }
perLevelDone:
    return static_cast<short>(nForcePoints < 0 ? 0 : nForcePoints);
}
#else  // K1: WIS and CHA modifiers both count; NPC totals truncate to a byte
short CSWSCreature::GetMaxForcePoints() {
    if (m_pStats->m_nRace == CSWRace::CRACE_DROID)
        return 0;

    if (m_pStats->m_bIsPC == 0 ||
        g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nControlledNPC != -1) {
        short nForcePoints = static_cast<short>(m_pStats->m_nBaseForcePoints
                                                  + m_pStats->m_nWISMod * m_pStats->GetLevel(0)
                                                  + static_cast<signed char>(m_pStats->m_nCHAMod) * m_pStats->GetLevel(0));
        if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_FORCE_SENSITIVE) != 0)
            nForcePoints += 40;
        for (unsigned char i = 0; i < m_pStats->m_numClasses; i++) {
            if (m_pStats->GetClass(i) == CSWClass::CCLASS_JEDICONSULAR) {
                CGameEffect** ppEffect = m_effects.m_pArray;
                int64_t j = m_effects.m_count;
                if (j > 0) {
                    do {
                        if (CSWSEffectListHandler::EFFECT_PUREEVILPOWERS == (*ppEffect)->m_subType) {
                            nForcePoints += 50;
                            goto legacyDone;
                        }
                        ppEffect++;
                    } while (--j != 0);
                }
            }
        }
    legacyDone:
        if (m_pStats->GetIsJedi() == 0)
            return 0;
        unsigned char nLevel = m_pStats->GetLevel(0);
        unsigned char nMax = static_cast<unsigned char>(nLevel > nForcePoints ? nLevel : nForcePoints);
        return nMax;
    }

    if (CSWClass::IsJedi(m_pStats->GetClass(m_pStats->m_numClasses - 1)) == 0)
        return 0;

    short nForcePoints = 0;
    for (int nCurLevel = 0; nCurLevel < m_pStats->GetLevel(0); nCurLevel++) {
        unsigned char nGain = m_pStats->GetLevelStats(static_cast<unsigned char>(nCurLevel))->m_nForcePointGain;
        if (nGain != 0) {
            int nAdd = m_pStats->m_nWISMod + nGain + static_cast<signed char>(m_pStats->m_nCHAMod);
            if (nAdd < 1)
                nAdd = 1;
            nForcePoints = static_cast<short>(nForcePoints + nAdd);
        }
    }
    if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_FORCE_SENSITIVE) != 0)
        nForcePoints += 40;
    for (unsigned char i = 0; i < m_pStats->m_numClasses; i++) {
        if (m_pStats->GetClass(i) == CSWClass::CCLASS_JEDICONSULAR) {
            CGameEffect** ppEffect = m_effects.m_pArray;
            int64_t j = m_effects.m_count;
            if (j > 0) {
                do {
                    if (CSWSEffectListHandler::EFFECT_PUREEVILPOWERS == (*ppEffect)->m_subType) {
                        nForcePoints += 50;
                        return nForcePoints;
                    }
                    ppEffect++;
                } while (--j != 0);
            }
        }
    }
    return nForcePoints;
}
#endif

short CSWSCreature::GetArmorClass()
{
#if KOTOR1  // K1: a short total seeded with 10; debilitation subtracts 4
    short nAC = 10;
    for (int i = 0; i < m_pStats->m_numClasses; i++) {
        CSWRules* pRules = g_pRules;
        unsigned char nClass = m_pStats->GetClass(static_cast<unsigned char>(i));
        nAC += static_cast<signed char>(pRules->m_paClasses[nClass].GetArmorBonus(
            m_pStats->GetClassLevel(static_cast<unsigned char>(i), 0)));
    }
    nAC += static_cast<signed char>(m_pStats->m_nACArmorMod + m_pStats->m_nACArmorBase - m_pStats->m_nACArmorNeg);
    nAC += static_cast<signed char>(m_pStats->m_nACShieldMod + m_pStats->m_nACShieldBase - m_pStats->m_nACShieldNeg);
    nAC += static_cast<signed char>(m_pStats->m_nACDeflectionMod - m_pStats->m_nACDeflectionNeg);
    int nDodge = m_pStats->m_nACDodgeMod - m_pStats->m_nACDodgeNeg;
    nAC += m_pStats->GetTotalACNaturalMod();
    if (nDodge > 10)
        nDodge = 10;
    nAC += static_cast<signed char>(nDodge);
    nAC += static_cast<signed char>(m_pStats->GetDEXMod(1));
    nAC += static_cast<signed char>(m_pStats->GetTotalACFeatMod());
    if (IsDebilitated())
        nAC -= 4;
    return nAC;
#else
    int nAC = 0;
    if (m_pStats->m_numClasses != 0) {
        for (int64_t i = 0; i < m_pStats->m_numClasses; i++) {
            nAC += static_cast<signed char>(g_pRules->m_paClasses[m_pStats->GetClass(static_cast<unsigned char>(i))]
                       .GetArmorBonus(m_pStats->GetClassLevel(static_cast<unsigned char>(i), 0)));
        }
    }

#if !KOTOR1
    if (m_bInParty == 0 && m_nMultiplierSet != 0) {
        float fMult;
        g_pRules->m_p2DArrays->m_pAutoBalance->GetFLOATEntry(
            m_nMultiplierSet, CExoString("ArmorMult"), &fMult);
        signed char nLevel = (m_nPCLevelAtSpawn != 0) ? m_nPCLevelAtSpawn : 1;
        nAC += static_cast<int>(fMult * static_cast<float>(nLevel));
    }
#endif

    nAC += static_cast<signed char>(m_pStats->m_nACArmorMod + m_pStats->m_nACArmorBase - m_pStats->m_nACArmorNeg);
    nAC += static_cast<signed char>(m_pStats->m_nACShieldMod + m_pStats->m_nACShieldBase - m_pStats->m_nACShieldNeg);
    nAC += static_cast<signed char>(m_pStats->m_nACDeflectionMod - m_pStats->m_nACDeflectionNeg);
    int nDodge = m_pStats->m_nACDodgeMod - m_pStats->m_nACDodgeNeg;
    nAC += m_pStats->GetTotalACNaturalMod();
    if (nDodge > 10)
        nDodge = 10;
    nAC += static_cast<signed char>(nDodge);
    nAC += static_cast<signed char>(m_pStats->GetDEXMod(1));
    nAC += static_cast<signed char>(m_pStats->GetTotalACFeatMod());

    if (IsDebilitated(1))
        return static_cast<short>(nAC + 6);
    return static_cast<short>(nAC + 10);
#endif
}

short CSWSCreature::GetMaxHitPoints(int a)
{
    if (m_pStats->m_bIsPC == 1
        && g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nControlledNPC == -1) {
        int nTotal = 0;
#if KOTOR1  // K1: the level is re-read per pass
        {
            for (int i = 0; i < m_pStats->GetLevel(0); i++) {
#else
        unsigned char nLevels = m_pStats->GetLevel(0);
        if (nLevels != 0) {
            for (int i = 0; i < nLevels; i++) {
#endif
                int nHp = m_pStats->GetLevelStats(static_cast<unsigned char>(i))->m_nHitPointGain
                          + m_pStats->m_nCONMod;
                if (nHp < 1)
                    nHp = 1;
                if (a != 0) {
                    if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_MASTER_TOUGHNESS)) {
                        nHp += 2;
                    } else if (m_pStats->HasFeat(FEAT_TOUGHNESS)) {
                        nHp += 1;
                    }
                }
                nTotal += nHp;
            }
        }
#if !KOTOR1  // K1: no +25 feat bonus
        if (m_pStats->HasFeat(FEAT_WAR_VETERAN))
            nTotal += 25;
#endif
        return static_cast<short>(nTotal);
    }

    int nBonus;
    if (a == 0) {
        nBonus = 0;
    } else if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_MASTER_TOUGHNESS)) {
        nBonus = 2;
    } else {
        nBonus = (m_pStats->HasFeat(FEAT_TOUGHNESS) != 0);
    }
#if KOTOR1  // K1: one feat (+2); K2 three variants
    int nPerLevel = nBonus;
    if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_WOOKIEE_TOUGHNESS))
        nPerLevel += 2;
#else
    int nPerLevel = nBonus;
    if (m_pStats->HasFeat(FEAT_WAR_VETERAN))
        nPerLevel = nBonus + 25;
    if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_WOOKIEE_TOUGHNESS_III)) {
        nPerLevel += 4;
    } else if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_WOOKIEE_TOUGHNESS_II)) {
        nPerLevel += 3;
    } else if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_WOOKIEE_TOUGHNESS)) {
        nPerLevel += 2;
    }
#endif

    int nBase = static_cast<short>(m_hitPoints) + m_pStats->m_nCONMod * m_pStats->GetLevel(0);
    if (nBase < m_pStats->GetLevel(0)) {
        return static_cast<short>(m_pStats->GetLevel(0) + nPerLevel * m_pStats->GetLevel(0));
    }
    return static_cast<short>(static_cast<unsigned short>(m_hitPoints)
                   + (nPerLevel + m_pStats->m_nCONMod) * m_pStats->GetLevel(0));
}

void CSWSCreature::ReceiveExperience(int xp)
{
    int nPercent = 100;
    int nPercentGlobal = 100;

#if !KOTOR1  // K1: no tag exclusions
    if (m_sTag.CompareNoCase("204_b4d4") != 0)
        return;
    if (m_sTag.CompareNoCase("remote") != 0)
        return;
#endif

    float fXPMult = 1.0f;
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    if (pPartyTable != 0) {
        C2DA* pNPC2DA = g_pRules->m_p2DArrays->m_pNPC;
        int nNPCId = pPartyTable->GetNPCID(m_objectId);
        if (pNPC2DA != 0 && nNPCId != -1) {
            pNPC2DA->GetINTEntry(nNPCId, CExoString("PercentXP"), &nPercent);
            m_sTag.CStr();  // result discarded
            fXPMult = static_cast<float>(nPercent) / 100.0f;
        }
#if KOTOR1  // K1: the global row is 9
        pNPC2DA->GetINTEntry(9, CExoString("PercentXP"), &nPercentGlobal);
#else
        pNPC2DA->GetINTEntry(13, CExoString("PercentXP"), &nPercentGlobal);
#endif
        fXPMult *= static_cast<float>(nPercentGlobal) / 100.0f;
    }
    m_sTag.CStr();  // result discarded
    m_pStats->AddExperience(static_cast<unsigned int>(__builtin_ceilf(fXPMult * static_cast<float>(xp))));

#if !KOTOR1  // K1: no G_PC_LEVEL global
    if (m_bPlayerMadeCharacter != 0) {
        unsigned char nLevel = m_pStats->GetPotentialLevel();
        CSWGlobalVariableTable* pGlobals =
            g_pAppManager->m_pServerExoApp->GetGlobalVariableTable();
        CExoString sName("G_PC_LEVEL");
        pGlobals->SetValueNumber(sName, nLevel);
    }
#endif
}

void CSWSCreature::SetMovementRateFactor(float f) {
    float v = 0.125f;
    if (!(f < 0.125f)) {
        v = f;
        if (f > 1.5f)
            v = 1.5f;
    }
    m_movementRateFactor = v;
}

void CSWSCreature::GetSpikeDetails(CExoArrayList<CSWGuiInterfaceAction>& list, unsigned char b)
{
    CItemRepository* pRepo;
#if KOTOR1  // K1: the out-of-line GetItemRepository(1)
    pRepo = GetItemRepository(1);
#else
    if ((g_pAppManager->m_pClientExoApp->GetInGameGui()->GetTopMenuOverrideCharacter() == -1) &
        (m_bInParty == 0)) {
        pRepo = m_pItemRepository;
    } else {
        pRepo = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems();
    }
#endif
    CExoString sName;
    CSWItemProperty* pProp;
    int nLanguage = g_pAppManager->m_pClientExoApp->GetClientLanguage();
    if (pRepo != 0 && pRepo->ItemListGetCount() != 0) {
        int nIdx = list.m_count;
        for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++) {
            CSWSItem* pItem = pRepo->ItemListGetItem(i);
            if (pItem != 0 && static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType != ITEM_TYPE_SPIKES)
                continue;
#ifdef K2_FIX_ENGINE_BUGS
            if (pItem == 0)
                continue;
#endif
            switch (b) {
            case 1: {
                if (!pItem->GetPropertyByTypeExists(ITEM_PROPERTY_SECURITY_SPIKE, 0))
                    continue;
                if (!pItem->GetPropertyByType(&pProp, ITEM_PROPERTY_SECURITY_SPIKE, 0))
                    continue;
                if (pProp == 0)
                    continue;
                list.SetSize(nIdx + 1);
                pItem->m_localizedName.GetString(nLanguage, &sName, 0);
                list.m_pArray[nIdx].m_sName = sName;
                list.m_pArray[nIdx].m_nActionId =
                        static_cast<unsigned int>(pItem->m_objectId) | GUI_ACTION_ITEM_FLAG;
                list.m_pArray[nIdx].m_resRef = pItem->GetIcon();
                list.m_pArray[nIdx].m_nCount = pItem->m_stackCount > pItem->m_nCharges
                                                  ? pItem->m_stackCount
                                                  : pItem->m_nCharges;
                nIdx++;
                break;
            }
            case 0: {
                if (!pItem->GetPropertyByTypeExists(ITEM_PROPERTY_COMPUTER_SPIKE, 0))
                    continue;
                if (!pItem->GetPropertyByType(&pProp, ITEM_PROPERTY_COMPUTER_SPIKE, 0))
                    continue;
                if (pProp == 0)
                    continue;
                list.SetSize(nIdx + 1);
                pItem->m_localizedName.GetString(nLanguage, &sName, 0);
                list.m_pArray[nIdx].m_sName = sName;
                list.m_pArray[nIdx].m_nActionId =
                        static_cast<unsigned int>(pItem->m_objectId) | GUI_ACTION_ITEM_FLAG;
                list.m_pArray[nIdx].m_resRef = pItem->GetIcon();
                nIdx++;
                break;
            }
            }
        }
    }
}

#if KOTOR1
void CSWSCreature::SetCombatState(int bCombat, unsigned char nCombatType)
#else
void CSWSCreature::SetCombatState(int bCombat, unsigned char nCombatType, int bFakeCombat)
#endif
{
    if (GetFaction()->m_nFactionId == 5)
        bCombat = 0;
#if !KOTOR1  // K1: no party local number 8 reset
    if (m_bInParty != 0 && bCombat == 0)
        m_varTable.SetLocalNumber(8, 0);
#endif
    if (m_bCombatState != bCombat && bCombat == 1 && nCombatType == 1 && m_bInParty == 0
            && m_bPlayerCharacter == 0) {
        if (rand_wincompatible() % 10 <= 2)
            BroadcastBattleCry();
    }
    m_bCombatState = bCombat;
#if !KOTOR1
    m_bFakeCombat = bFakeCombat;
#endif
    if (bCombat != 0) {
        m_nCombatTimer = 8000;
        if (m_nCombatType != 1)
            m_nCombatType = nCombatType;
    } else {
        m_nCombatTimer = 0;
        m_nCombatType = 0;
#if !KOTOR1
        m_bFakeCombat = 0;
#endif
        m_bInitiativeExpired = 1;
        m_nCurrentAttackAction = -1;
        m_nLastAttackAction = -1;
        m_oidLastHostileTarget = OBJECT_INVALID;
        SetLastHostileActor(OBJECT_INVALID, 1);
        m_nCurrentForcePower = -1;
        m_nLastForcePowerUsed = -1;
        m_nCurrentCombatFeat = 0;
        m_nLastCombatFeatUsed = 0;
        m_pCombatRound->ClearAllAttacks();
        m_pCombatRound->RemoveAllActions();
        m_attemptedAttackTargetId = OBJECT_INVALID;
        m_oidAttackTarget = OBJECT_INVALID;
        m_oidAttemptedSpellTarget = OBJECT_INVALID;
        m_oidSpellTarget = OBJECT_INVALID;
        m_pCombatRound->m_oidAttackTarget = OBJECT_INVALID;
        CSWSCombatRound* pRound = m_pCombatRound;
        if (pRound->m_bRoundStarted != 0) {
            CExoString sWarning;
            CExoString sTargetTag;
            CGameObject* pObject =
                g_pAppManager->m_pServerExoApp->GetGameObject(pRound->m_oidMaster);
            if (pObject != 0 && pObject->AsSWSObject() != 0) {
                sTargetTag = pObject->AsSWSObject()->m_sTag;
#if KOTOR1  // K1: CExoString::IsEmpty is out of line
                if (sTargetTag.IsEmpty())
#else
                if (sTargetTag.m_sString == 0 || sTargetTag.m_sString[0] == 0)
#endif
                    sTargetTag = NULL_NAME;
            }
            sWarning.Format(const_cast<char*>("WARNING:  %x %s %d %d %d %d %x %s %x"), m_objectId, m_sTag.CStr(),
                            pRound->m_nTimer, pRound->m_nRoundLength, pRound->m_bEngaged,
                            pRound->m_bMaster, pRound->m_oidMaster, sTargetTag.CStr(), pObject);
            m_pCombatRound->EndCombatRound(0);
        }
    }
    if (GetClientCreature() != 0 && nCombatType == 1)
        GetClientCreature()->SetCombatState(bCombat);
}

#if !KOTOR1
// Applies passive hit-point regeneration at rate = OutOfCombatHPBase (or InCombatHPBase in combat)
// + ConModBonus * CON modifier + SkillRankBons * (skill 7, or 5 for droids), doubled per
// applied EFFECT_VPREGENMODIFIER and scaled by the tick's elapsed seconds / TimePerHP.
void CSWSCreature::HandleHPRegeneration() {
    if (GetMaxHitPoints(1) == GetCurrentHitPoints(1))
        return;
    if (g_pAppManager->m_pServerExoApp->GetPartyTable()->m_disableRegen != 0)
        return;

    float fRate = 0.0f;
    float fTimePerHP = 0.0f;
    float fConModBonus = 0.0f;
    float fSkillRankBonus = 0.0f;
    float fNew;
    int nHitPoints;

    C2DA* p2DA = g_pRules->m_p2DArrays->m_pRegeneration;
    CExoString sValue("VALUE");
    p2DA->GetFLOATEntry(CExoString("TimePerHP"), sValue, &fTimePerHP);
    if (m_bCombatState == 0 || m_nCombatType == 2)
        p2DA->GetFLOATEntry(CExoString("OutOfCombatHPBase"), sValue, &fRate);
    else
        p2DA->GetFLOATEntry(CExoString("InCombatHPBase"), sValue, &fRate);
    p2DA->GetFLOATEntry(CExoString("ConModBonus"), sValue, &fConModBonus);
    p2DA->GetFLOATEntry(CExoString("SkillRankBons"), sValue, &fSkillRankBonus);

    fRate = fRate + fConModBonus * static_cast<float>(m_pStats->m_nCONMod);

    char nSkillRank;
    if (m_pStats->m_nRace == CSWRace::CRACE_DROID)
        nSkillRank = m_pStats->GetSkillRank(SKILL_REPAIR, 0, 0);
    else
        nSkillRank = m_pStats->GetSkillRank(SKILL_TREAT_INJURY, 0, 0);
    fRate = fRate + fSkillRankBonus * static_cast<float>(nSkillRank);

    if (m_pStats->HasFeat(FEAT_REGENERATE_VITALITY_POINTS))
        fRate = fRate + 0.5f;

    for (int64_t i = 0; i < m_effects.m_count; i++) {
        CGameEffect* pEffect = m_effects.m_pArray[i];
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_VPREGENMODIFIER) {
            pEffect->GetInteger(0);
            fRate = fRate + fRate;
        }
    }

    fTimePerHP = fTimePerHP <= 0.0001f ? 0.0001f : fTimePerHP;
    float fPerSecond = 1.0f / fTimePerHP;
    fPerSecond = fPerSecond <= 0.0001f ? 0.0001f : fPerSecond;
    float fSeconds = static_cast<float>(m_nAIUpdateDeltaMs) / 1000.0f;
    float fGain = fRate * (fSeconds * fPerSecond);

    if (fGain != 0.0f) {
        short nCurrent = GetCurrentHitPoints(1);
        short nWhole = m_pStats->AdjustHPAccumulatorBy(fGain);
        fNew = static_cast<float>(nWhole + nCurrent);
        if (static_cast<float>(GetMaxHitPoints(1)) < fNew)
            fNew = static_cast<float>(GetMaxHitPoints(1));
        nHitPoints = static_cast<int>(fNew);
        SetCurrentHitPoints(nHitPoints > 1 ? nHitPoints : 1);
    }
}

void CSWSCreature::HandleFPRegeneration()
{
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (pArea == 0)
        return;
    short nMaxFP = GetMaxForcePoints();
    if (nMaxFP <= 0)
        return;

    int nTotalFP = static_cast<short>(m_pStats->m_nTemporaryForcePoints);
    int nRoomRating = pArea->GetRoomForceRating(m_position);

    C2DA* p2DA = g_pRules->m_p2DArrays->m_pRegeneration;
    CExoString sValue("VALUE");
    float fFPRegenTime = 0.0f;
    float fBase = 0.0f;
    float fWisModBonus = 0.0f;
    p2DA->GetFLOATEntry(CExoString("FPRegenTime"), sValue, &fFPRegenTime);
    nTotalFP = nTotalFP + nMaxFP;

    if (m_bCombatState == 0 || m_nCombatType == 2) {
        p2DA->GetFLOATEntry(CExoString("OutOfCombatFPBase"), sValue, &fBase);
    } else if (m_stanceId == FORM_SABER_VI_NIMAN && IsWieldingLightsaber() != 0) {
        p2DA->GetFLOATEntry(CExoString("OutOfCombatFPBase"), sValue, &fBase);
        fBase = fBase * 0.3f;
    } else if (m_stanceId == FORM_FORCE_III_AFFINITY) {
        p2DA->GetFLOATEntry(CExoString("OutOfCombatFPBase"), sValue, &fBase);
        fBase = fBase * 0.6f;
    } else {
        p2DA->GetFLOATEntry(CExoString("InCombatFPBase"), sValue, &fBase);
    }
    p2DA->GetFLOATEntry(CExoString("WisModBonus"), sValue, &fWisModBonus);

    unsigned char nAlignment;
    if (m_pStats->m_nAlignment < 41)
        nAlignment = 3;
    else
        nAlignment = m_pStats->m_nAlignment > 59 ? 2 : 1;

    fBase = fBase + fWisModBonus * static_cast<float>(m_pStats->m_nWISMod);

    if (m_pStats->HasFeat(FEAT_REGENERATE_FORCE_POINTS))
        fBase = fBase + 0.25f;

    for (int64_t i = 0; i < m_effects.m_count; i++) {
        CGameEffect* pEffect = m_effects.m_pArray[i];
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_FPREGENMODIFIER)
            fBase = static_cast<float>(pEffect->GetInteger(0)) / 100.0f + fBase;
    }

    if (m_stanceId == FORM_FORCE_I_FOCUS)
        fBase = fBase + 0.5f;

    float fRoomDelta = fBase * static_cast<float>(nRoomRating) / 100.0f;
    if (nAlignment == 3)
        fBase = fBase - fRoomDelta;
    else if (nAlignment == 2)
        fBase = fBase + fRoomDelta;

    fFPRegenTime = fFPRegenTime <= 0.0001f ? 0.0001f : fFPRegenTime;
    float fSeconds = static_cast<float>(m_nAIUpdateDeltaMs) * 0.001f;
    float fGain = fBase / fFPRegenTime * static_cast<float>(nTotalFP) * fSeconds;

    if (fGain != 0.0f) {
        short nCurrent = m_pStats->m_nCurrentForcePoints + m_pStats->m_nTemporaryForcePoints;
        int nNew = nCurrent + m_pStats->AdjustFPAccumulatorBy(fGain);
        if (nNew < 0)
            nNew = 0;
        if (nNew > nTotalFP)
            nNew = nTotalFP;
        m_pStats->m_nCurrentForcePoints = static_cast<short>(nNew);
    }
}
#endif

void CSWSCreature::AddToArea(CSWSArea* pArea, float x, float y, float z, int a, int b)
{
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
    CSWSArea* pOldArea = static_cast<CSWSArea*>(GetArea());
    if (a == 1 || pOldArea != pArea)
        RemoveFromArea(0);

    CServerAIMaster* pAIMaster = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    for (int i = 0; i < m_areaOfEffectIds.m_count; i++)
    {
        CGameObject* pObject;
        if (pObjectArray->GetGameObject(m_areaOfEffectIds.m_pArray[i], &pObject) == CGameObjectArray::SUCCESS
            && pObject->AsSWSAreaOfEffectObject() != 0)
        {
            pObject->AsSWSAreaOfEffectObject()->AddToArea(pArea, x, y, z, 1);
        }
    }

    if (pArea == 0) return;

#if !KOTOR1  // K1: no VFX_DUR_BREATH (applied outside 261tel, removed inside it)
    CExoString sModule = g_pAppManager->m_pServerExoApp->GetModuleName();
    int nCmp = sModule.CompareNoCase("261tel");
    if (nCmp != 0)
    {
        if (m_pStats->m_nRace != CSWRace::CRACE_DROID)
        {
            int bFound = 0;
            for (int i = 0; i < m_effects.m_count; i++)
            {
                CGameEffect* pEffect = m_effects.m_pArray[i];
                if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_VISUALEFFECT
                    && pEffect->GetInteger(0) == VFX_DUR_BREATH)
                {
                    bFound = 1;
                    break;
                }
            }
            if (bFound == 0)
            {
                CGameEffect* pEffect = new CGameEffect(1);
                pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
                pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_PERMANENT |
                                              (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
                pEffect->SetCreator(m_objectId);
                pEffect->SetInteger(0, VFX_DUR_BREATH);
                ApplyEffect(pEffect, 0, 0);
            }
        }
    }
    else
    {
        if (m_pStats->m_nRace != CSWRace::CRACE_DROID)
        {
            for (int i = 0; i < m_effects.m_count; i++)
            {
                CGameEffect* pEffect = m_effects.m_pArray[i];
                if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_VISUALEFFECT
                    && pEffect->GetInteger(0) == VFX_DUR_BREATH)
                {
                    RemoveEffectById(pEffect->m_effectId);
                    break;
                }
            }
        }
    }
#endif

    CSWSMessage* pMsg = 0;
    if (static_cast<CSWSArea*>(GetArea()) != pArea && pPlayer != 0
        && pArea->m_objectId != m_currentAreaId)
    {
        pMsg = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    }

    if (pMsg != 0)
    {
        m_vDesiredAreaLocation.x = x;
        m_vDesiredAreaLocation.y = y;
        m_vDesiredAreaLocation.z = z;
        m_bDesiredAreaUpdateComplete = 0;
        m_currentAreaId = pArea->m_objectId;
        pMsg->SendServerToPlayerArea_ClientArea(pPlayer, pArea, x, y, z, m_orientation, 0);
        SetArea(0);
        Vector vPos;
        vPos.x = x;
        vPos.y = y;
        vPos.z = z;
        SetPosition(vPos, 0, 1, 0);
    }
    else
    {
        Vector vPos;
        vPos.x = x;
        vPos.y = y;
        vPos.z = z;
        SetPosition(vPos, 1, 1, 0);
        CSWSArea* pCurArea = static_cast<CSWSArea*>(GetArea());
        if (a == 1 || pCurArea != pArea)
            pArea->AddObjectToArea(m_objectId, a == 0);
        SetArea(pArea);

        if (m_aiLevel == 0)
        {
            if (pAIMaster != 0 && pArea->m_nPlayersInArea > 0)
                pAIMaster->SetAILevel(this, 1);
        }
        if (m_aiLevel == 1)
        {
            if (pAIMaster != 0 && pArea->m_nPlayersInArea == 0)
                pAIMaster->SetAILevel(this, 0);
        }

        if (b != 0)
        {
            UpdateSubareasOnJumpPosition(m_position, pArea->m_objectId,
                g_pAppManager->m_pServerExoApp->GetLoadFromSaveGame() == 0);
        }
    }
}

// A shipped stub that returns 0, since KOTOR2 cut the monk unarmed-combat rules.
// Its four callers on CSWSCreatureStats keep their now-dead monk branch.
int CSWSCreature::GetUseMonkAbilities()
{
    return 0;
}

CSWSCreature* CSWSCreature::AsSWSCreature()
{
    return this;
}

int CSWSCreature::TestAIStateAsMode(int mode)
{
    return mode < 10;
}

int CSWSCreature::GetDialogInterruptable()
{
    if (m_pStats != 0)
        return m_pStats->m_nInterruptable;
    return 0;
}

unsigned char CSWSCreature::GetGender()
{
    if (m_pStats != 0)
        return m_pStats->m_nGender;
    return CSWSObject::GetGender();
}

int CSWSCreature::GetDead()
{
#if KOTOR1  // K1: a member of the client party is never dead (no in-party flag)
    int nCount = g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount;
    for (int i = 0; i < nCount; i++) {
        CSWCCreature* pMember = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(i);
        unsigned long long oidClient = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId);
        if (pMember != 0 && pMember->m_objectId == oidClient)
            return 0;
    }
    return CSWSObject::GetDead();
#else
    if (m_bInParty == 0)
        return CSWSObject::GetDead();
    return 0;
#endif
}

#if KOTOR1
int CSWSCreature::GetDeadTemp()
{
    int nCount = g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount;
    for (int i = 0; i < nCount; i++) {
        CSWCCreature* pMember = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(i);
        unsigned long long oidClient = g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId);
        if (pMember != 0 && pMember->m_objectId == oidClient)
            return GetCurrentHitPoints(0) < 1;
    }
    return 0;
}
#endif

float CSWSCreature::MaxCleaveRange(unsigned long long objectId, int a, int b)
{
    return MaxAttackRange(objectId, a, b) + 2.0f;
}

int CSWSCreature::GetBlind()
{
    if ((m_nVisionFlags & VISION_BLINDNESS) != 0)
        return 1;
    return (m_nVisionFlags & (VISION_ULTRAVISION | VISION_DARKNESS)) == VISION_DARKNESS;
}

CSWSItem* CSWSCreature::PayToIdentifyItem(unsigned long long objectId)
{
    return g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(objectId);
}

unsigned long long CSWSCreature::GetNearestEnemy(float fRange, unsigned long long oidExclude,
                                                 unsigned long long oidRelativeTo)
{
#if KOTOR1  // K1: the area is fetched again after the null test
    unsigned long long oidNearest = OBJECT_INVALID;
    if (GetArea() == 0)
        return oidNearest;
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
#else
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    unsigned long long oidNearest = OBJECT_INVALID;
    if (pArea == 0)
        return oidNearest;
#endif

    // Our own slot in the X-sorted index is the scan's origin. Not being in it at all (a creature
    // mid-area-transition) means there is nothing to scan outward from.
    int nIndex;
    for (nIndex = 0; nIndex < pArea->m_aObjectsByX.m_count; nIndex++) {
        if (pArea->m_aObjectsByX.m_pArray[nIndex] == m_objectId)
            break;
    }
    if (nIndex >= pArea->m_aObjectsByX.m_count)
        return oidNearest;

    unsigned long long oidHit;
    Vector vHit = {};
    int nDown = nIndex;
    int nUp = nIndex;
    int bDownActive = 1;
    int bUpActive = 1;

    while (nDown >= 0 || nUp < pArea->m_aObjectsByX.m_count) {
        if (nDown >= 0 && bDownActive) {
            unsigned long long oid = pArea->m_aObjectsByX.m_pArray[nDown];
            CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oid);
            if (pObject != 0 && oid != m_objectId && oid != oidExclude) {
                int bEnemy;
                if (oidRelativeTo == OBJECT_INVALID)
                    bEnemy = GetAIStateReputation(oid) == 2;
                else if (pObject->AsSWSCreature() != 0)
                    bEnemy = pObject->AsSWSCreature()->GetAIStateReputation(oidRelativeTo) == 2;
                else
                    bEnemy = 0;
                if (pObject->AsSWSCreature() != 0 && bEnemy
                    && pObject->AsSWSCreature()->GetDead() == 0) {
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp()
                    if (pObject->AsSWSCreature()->GetIsPCDying() == 0
                        && pObject->AsSWSCreature()->GetDeadTemp() == 0) {
#else
                    pObject->AsSWSCreature();
                    CSWSCreature* pTarget = pObject->AsSWSCreature();
                    if (pTarget->m_bInParty == 0 || pTarget->GetCurrentHitPoints(0) > 0) {
#endif
                        CSWVisibilityNode* pNode = GetVisibleListElement(pObject->m_objectId);
                        if (pNode != 0 && (pNode->m_flags & VISIBILITY_SEEN)) {
                            Vector vFrom;
                            if (oidRelativeTo == OBJECT_INVALID) {
                                vFrom = m_position;
                            } else {
                                CSWSCreature* pRelative = g_pAppManager->m_pServerExoApp->
                                    GetCreatureByGameObjectID(oidRelativeTo);
                                if (pRelative != 0) {
                                    vFrom = pRelative->m_position;
                                } else {
                                    vFrom.x = 0.0f;
                                    vFrom.y = 0.0f;
                                    vFrom.z = 0.0f;
                                }
                            }
                            float fDist = magnitude(vFrom - pObject->AsSWSCreature()->m_position)
                                - m_pPathfindInfo->m_fCrePersonalSpace
                                - pObject->AsSWSCreature()->m_pPathfindInfo->m_fCrePersonalSpace;
                            if (fDist < fRange) {
                                // Both probe endpoints rise 1.5m off the floor, to eye height.
                                Vector vStart = m_position;
                                vStart.z += 1.5f;
                                Vector vEnd = pObject->AsSWSCreature()->m_position;
                                vEnd.z += 1.5f;
#if KOTOR1  // K1: line of sight through a re-fetched, null-tested GetArea()
                                if (GetArea() != 0
                                    && static_cast<CSWSArea*>(GetArea())->ClearLineOfSight(vStart, vEnd,
                                           &vHit, &oidHit, m_objectId, oid, 1, 0)) {
#else
                                if (pArea->ClearLineOfSight(vStart, vEnd, &vHit, &oidHit,
                                                            m_objectId, oid, 1, 0)) {
#endif
                                    fRange = fDist;
                                    oidNearest = oid;
                                }
                            }
                        }
                    }
                }
                // Retire once the index's X ordering puts this candidate further away in X alone
                // than the best distance so far, since everything past it is further still.
                if (pObject->AsSWSObject() != 0
                    && m_position.x - pObject->AsSWSObject()->m_position.x > fRange)
                    bDownActive = 0;
            }
        } else {
            bDownActive = 0;
        }
        if (bUpActive && nUp < pArea->m_aObjectsByX.m_count) {
            unsigned long long oid = pArea->m_aObjectsByX.m_pArray[nUp];
            CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oid);
            if (pObject != 0 && oid != m_objectId && oid != oidExclude) {
                int bEnemy;
                if (oidRelativeTo == OBJECT_INVALID)
                    bEnemy = GetAIStateReputation(oid) == 2;
                else if (pObject->AsSWSCreature() != 0)
                    bEnemy = pObject->AsSWSCreature()->GetAIStateReputation(oidRelativeTo) == 2;
                else
                    bEnemy = 0;
                if (pObject->AsSWSCreature() != 0 && bEnemy
                    && pObject->AsSWSCreature()->GetDead() == 0) {
#if KOTOR1
                    if (pObject->AsSWSCreature()->GetIsPCDying() == 0
                        && pObject->AsSWSCreature()->GetDeadTemp() == 0) {
#else
                    pObject->AsSWSCreature();
                    CSWSCreature* pTarget = pObject->AsSWSCreature();
                    if (pTarget->m_bInParty == 0 || pTarget->GetCurrentHitPoints(0) > 0) {
#endif
                        CSWVisibilityNode* pNode = GetVisibleListElement(pObject->m_objectId);
                        if (pNode != 0 && (pNode->m_flags & VISIBILITY_SEEN)) {
                            Vector vFrom;
                            if (oidRelativeTo == OBJECT_INVALID) {
                                vFrom = m_position;
                            } else {
                                CSWSCreature* pRelative = g_pAppManager->m_pServerExoApp->
                                    GetCreatureByGameObjectID(oidRelativeTo);
                                if (pRelative != 0) {
                                    vFrom = pRelative->m_position;
                                } else {
                                    vFrom.x = 0.0f;
                                    vFrom.y = 0.0f;
                                    vFrom.z = 0.0f;
                                }
                            }
                            float fDist = magnitude(vFrom - pObject->AsSWSCreature()->m_position)
                                - m_pPathfindInfo->m_fCrePersonalSpace
                                - pObject->AsSWSCreature()->m_pPathfindInfo->m_fCrePersonalSpace;
                            if (fDist < fRange) {
                                // vFrom in the up half, m_position in the down half.
                                Vector vStart = vFrom;
                                vStart.z += 1.5f;
                                Vector vEnd = pObject->AsSWSCreature()->m_position;
                                vEnd.z += 1.5f;
#if KOTOR1
                                if (GetArea() != 0
                                    && static_cast<CSWSArea*>(GetArea())->ClearLineOfSight(vStart, vEnd,
                                           &vHit, &oidHit, m_objectId, oid, 1, 0)) {
#else
                                if (pArea->ClearLineOfSight(vStart, vEnd, &vHit, &oidHit,
                                                            m_objectId, oid, 1, 0)) {
#endif
                                    fRange = fDist;
                                    oidNearest = oid;
                                }
                            }
                        }
                    }
                }
                if (pObject->AsSWSObject() != 0
                    && pObject->AsSWSObject()->m_position.x - m_position.x > fRange)
                    bUpActive = 0;
            }
        } else {
            bUpActive = 0;
        }
        nUp++;
        nDown--;
    }
    return oidNearest;
}

// Shipped stubs, cut features left as `return 0` / empty bodies, out of line so their callers keep
// the call.

int CSWSCreature::AddPickPocketActions(unsigned long long a)
{
    K2_UNUSED(a);
    return 0;
}

int CSWSCreature::UpdateSpecialAttacks()
{
    return 0;
}

int CSWSCreature::GetTurnResistanceHD()
{
    return 0;
}

int CSWSCreature::AddAnimalEmpathyAction(unsigned long long a)
{
    K2_UNUSED(a);
    return 0;
}

int CSWSCreature::AddTauntActions(unsigned long long a)
{
    K2_UNUSED(a);
    return 0;
}

int CSWSCreature::LearnScroll(unsigned long long a)
{
    K2_UNUSED(a);
    return 0;
}

int CSWSCreature::UseLoreOnItem(unsigned long long a)
{
    K2_UNUSED(a);
    return 0;
}

void CSWSCreature::BroadcastAttackOfOpportunity(unsigned long long a)
{
    K2_UNUSED(a);
}

void CSWSCreature::BroadcastAttackDataToParty(CSWCCMessageData* pData, CSWSCombatAttackData* pAttackData)
{
    K2_UNUSED(pData); K2_UNUSED(pAttackData);
}

void CSWSCreature::BroadcastMineDetectionData(CSWCCMessageData* pData)
{
    unsigned long long* paMembers;
    GetFaction()->GetMemberList(&paMembers);
    for (int i = 0; i < GetFaction()->m_members.m_count; i++) {
        CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(paMembers[i]);
        if (pPlayer != 0) {
            CSWSObject* pObject = pPlayer->GetGameObject();
            if (pObject != 0 && m_cachedAreaId == pObject->m_cachedAreaId) {
                g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerCCMessage(
                    pPlayer->m_nPlayerId, CC_MESSAGE_MINE_DETECTION, pData, 0);
            }
        }
    }
}

void CSWSCreature::BroadcastStealthDetectionData(CSWCCMessageData* pData)
{
    unsigned long long* paMembers;
    GetFaction()->GetMemberList(&paMembers);
    for (int i = 0; i < GetFaction()->m_members.m_count; i++) {
        CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(paMembers[i]);
        if (pPlayer != 0) {
            CSWSObject* pObject = pPlayer->GetGameObject();
            if (pObject != 0 && m_cachedAreaId == pObject->m_cachedAreaId) {
                g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerCCMessage(
                    pPlayer->m_nPlayerId, CC_MESSAGE_STEALTH_DETECTION, pData, 0);
            }
        }
    }
}

void CSWSCreature::BroadcastForcePointUsageData(CSWCCMessageData* pData)
{
    unsigned long long* paMembers;
    GetFaction()->GetMemberList(&paMembers);
    for (int i = 0; i < GetFaction()->m_members.m_count; i++) {
        CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(paMembers[i]);
        if (pPlayer != 0) {
            CSWSObject* pObject = pPlayer->GetGameObject();
            if (pObject != 0 && m_cachedAreaId == pObject->m_cachedAreaId) {
                g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerCCMessage(
                    pPlayer->m_nPlayerId, CC_MESSAGE_FORCE_POINT_USAGE, pData, 0);
            }
        }
    }
}

void CSWSCreature::BroadcastSpellCast(unsigned int nSpell, int a2, unsigned char a3,
                                      unsigned short a4, unsigned long long oidTarget)
{
    K2_UNUSED(a2); K2_UNUSED(a3); K2_UNUSED(a4);
    CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(nSpell);
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (pArea != 0) {
        m_aSpellCastBroadcastList.Clear();
        CSWCCMessageData* pData = new CSWCCMessageData;
        pData->SetObjectID(0, m_objectId);
        pData->SetObjectID(1, OBJECT_INVALID);
        pData->SetObjectID(2, oidTarget);
        pData->SetInteger(0, nSpell);
        pData->SetInteger(2, 1);
        for (int i = 0; i < pArea->m_aObjectsByX.m_count; i++) {
            CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->
                GetCreatureByGameObjectID(pArea->m_aObjectsByX.m_pArray[i]);
            if (pCreature == 0)
                continue;
            Vector vDelta = pCreature->m_position - m_position;
            if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z >
                g_pRules->m_fSpellRange[4] * g_pRules->m_fSpellRange[4])
                continue;
            if (pSpell->m_nUserType == 1 && pSpell->m_nAltMessage == 0) {
                pCreature->GetVisibleListElement(m_objectId);
            } else {
                m_aSpellCastBroadcastList.Add(pCreature->m_objectId);
            }
        }
        delete pData;
    }
}

void CSWSCreature::BroadcastAttackSummaryToParty(CSWCCMessageData* pData)
{
    CSWSFaction* pFaction = GetFaction();
    if (pFaction == 0)
        return;
    unsigned long long* paMembers = 0;
    int nCount = pFaction->GetMemberList(&paMembers);
    for (int i = 0; i < nCount; i++) {
        CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(paMembers[i]);
        if (pPlayer != 0) {
            CSWSObject* pObject = pPlayer->GetGameObject();
            if (pObject != 0 && m_cachedAreaId == pObject->m_cachedAreaId) {
                Vector vDelta = pObject->m_position - m_position;
                if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z <= 900.0f) {
                    g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerCCMessage(
                        pPlayer->m_nPlayerId, CC_MESSAGE_ATTACK_SUMMARY, pData, 0);
                }
            }
        }
    }
}

void CSWSCreature::BroadcastPoisoned(CSWCCMessageData* pData)
{
    CSWSFaction* pFaction = GetFaction();
    if (pFaction == 0)
        return;
    unsigned long long* paMembers = 0;
    int nCount = pFaction->GetMemberList(&paMembers);
    for (int i = 0; i < nCount; i++) {
        CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(paMembers[i]);
        if (pPlayer != 0) {
            CSWSObject* pObject = pPlayer->GetGameObject();
            if (pObject != 0 && m_cachedAreaId == pObject->m_cachedAreaId) {
                Vector vDelta = pObject->m_position - m_position;
                if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z <= 900.0f) {
                    g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerCCMessage(
                        pPlayer->m_nPlayerId, CC_MESSAGE_POISONED, pData, 0);
                }
            }
        }
    }
}

void CSWSCreature::BroadcastBreakdownMessage(unsigned char nId, CSWCCMessageData* pData)
{
    CSWSFaction* pFaction = GetFaction();
    if (pFaction == 0)
        return;
    unsigned long long* paMembers = 0;
    int nCount = pFaction->GetMemberList(&paMembers);
    for (int i = 0; i < nCount; i++) {
        CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(paMembers[i]);
        if (pPlayer != 0) {
            CSWSObject* pObject = pPlayer->GetGameObject();
            if (pObject != 0 && m_cachedAreaId == pObject->m_cachedAreaId) {
                Vector vDelta = pObject->m_position - m_position;
                if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z <= 900.0f) {
                    g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerCCMessage(
                        pPlayer->m_nPlayerId, nId, pData, 0);
                }
            }
        }
    }
}

void CSWSCreature::BroadcastFloatyData(CSWCCMessageData* pData)
{
    unsigned long long* paMembers;
    GetFaction()->GetMemberList(&paMembers);
    for (int i = 0; i < GetFaction()->m_members.m_count; i++) {
        CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(paMembers[i]);
        if (pPlayer != 0) {
            CSWSObject* pObject = pPlayer->GetGameObject();
            if (pObject != 0 && m_cachedAreaId == pObject->m_cachedAreaId) {
                Vector vDelta = pPlayer->GetGameObject()->m_position - m_position;
                if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z <= 900.0f) {
                    g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerCCMessage(
                        pPlayer->m_nPlayerId, CC_MESSAGE_FEEDBACK, pData, 0);
                }
            }
        }
    }
}

void CSWSCreature::BroadcastDamageDataToParty(CSWCCMessageData* pData, int nDamageFlag)
{
    unsigned long long* paMembers;
    GetFaction()->GetMemberList(&paMembers);
    for (int i = 0; i < GetFaction()->m_members.m_count; i++) {
        CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(paMembers[i]);
        if (pPlayer != 0) {
            CSWSObject* pObject = pPlayer->GetGameObject();
            if (pObject != 0 && m_cachedAreaId == pObject->m_cachedAreaId) {
                Vector vDelta = pObject->m_position - m_position;
                if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z <= 900.0f) {
                    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
                    if (nDamageFlag == 0)
                        pMessage->SendServerToPlayerCCMessage(pPlayer->m_nPlayerId,
                                                              CC_MESSAGE_DAMAGE_FROM_ATTACKER, pData, 0);
                    else
                        pMessage->SendServerToPlayerCCMessage(pPlayer->m_nPlayerId, CC_MESSAGE_DAMAGE,
                                                              pData, 0);
                }
            }
        }
    }
}

// Computes the XP for killing this creature. The XPTable 2DA is rowed by the player's level and
// columned by this creature's challenge rating, then the result is scaled by the NPC 2DA's
// "PercentXP" row 13.
float CSWSCreature::CalculateDeathExperience()
{
    float fExperience = 0.0f;
    CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(
        g_pAppManager->m_pServerExoApp->GetPlayerList()->m_pInternal->m_pHead->m_pData);
    CSWSCreature* pCreature =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pPlayer->m_gameObjectId);
    if (pCreature != 0) {
        unsigned int nEarned = pCreature->m_pStats->m_nExperience;
        int nPlayerLevel;
#if KOTOR1
        for (nPlayerLevel = 20; nPlayerLevel >= 0; nPlayerLevel--) {
#else
        for (nPlayerLevel = 50; nPlayerLevel >= 0; nPlayerLevel--) {
#endif
            if (nEarned >= static_cast<unsigned int>(g_pRules->m_aExpTable[static_cast<unsigned char>(nPlayerLevel)]))
                break;
        }
#if KOTOR1
        float fBase = 0.0f;
        g_pRules->m_p2DArrays->m_pXpBase->GetFLOATEntry(
            nPlayerLevel, static_cast<int>(m_pStats->m_fChallengeRating) + 1, &fBase);
        float fPercent = 0.0f;
        g_pRules->m_p2DArrays->m_pNPC->GetFLOATEntry(NPC_ROW_GAME_XP_GENERAL, CExoString("PercentXP"), &fPercent);
        fExperience = fBase * (fPercent / 100.0f);
#else
        int nChallengeRating = static_cast<int>(m_pStats->m_fChallengeRating);
        int nCRMod;
        if (m_nMultiplierSet != 0 && m_bInParty == 0) {
            g_pRules->m_p2DArrays->m_pAutoBalance->GetINTEntry(m_nMultiplierSet, CExoString("CRMod"),
                                                              &nCRMod);
            int nBonus = static_cast<signed char>(m_nPCLevelAtSpawn);
            if (nBonus == 0)
                nBonus = 1;
            nChallengeRating = nCRMod + nBonus;
            if (nChallengeRating < 0)
                nChallengeRating = 0;
        }
        float fBase = 0.0f;
        g_pRules->m_p2DArrays->m_pXpBase->GetFLOATEntry(
            nPlayerLevel < 30 ? nPlayerLevel : 30,
            nChallengeRating > 30 ? 31 : nChallengeRating + 1, &fBase);
        float fPercent = 0.0f;
        g_pRules->m_p2DArrays->m_pNPC->GetFLOATEntry(NPC_ROW_GAME_XP_GENERAL, CExoString("PercentXP"), &fPercent);
        fExperience = fBase * (fPercent / 100.0f);
#endif
    }
    return fExperience;
}

// Pays out the party XP for this creature's death. Nothing is awarded for
// the switched-in PC, for a party member, or for a creature whose NPC faction still likes the
// player (reputation > 10). The base award from CalculateDeathExperience is scaled up by
// npc.2da row 14's "PercentXP" per party member, rounded up, and distributed; the killer then
// gets a floaty XP number. A trigger killer (mine/trap) is re-credited to its creator.
void CSWSCreature::ApplyDeathExperience()
{
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    if (m_bPlayerCharacter != 0)
        return;
    if (pPartyTable != 0 && pPartyTable->GetIsMember(m_objectId) != 0)
        return;
    if (g_pAppManager->m_pServerExoApp->GetFactionManager()->GetNPCFactionReputation(
            m_pStats->m_factionId, 0) > 10)
        return;
    int nPercentXP = 0;
    float fExperience = CalculateDeathExperience();
    g_pRules->m_p2DArrays->m_pNPC->GetINTEntry(NPC_ROW_PER_NPC_BONUS, CExoString("PercentXP"), &nPercentXP);
    K2_ASSUME(pPartyTable != 0);
    if (nPercentXP > 0) {
        fExperience = fExperience
            * (static_cast<float>(nPercentXP) / 100.0f * static_cast<float>(pPartyTable->m_nNumPartyMembers) + 1.0f);
    }
    int nExperience = static_cast<int>(__builtin_ceilf(fExperience));
    pPartyTable->DistributeExperience(nExperience, 0);

    CSWSCreature* pKiller = 0;
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(m_killerId, &pObject)
            == CGameObjectArray::SUCCESS) {
        if (pObject->AsSWSTrigger() != 0
                && pObject->AsSWSTrigger()->m_creatorId != OBJECT_INVALID) {
            pObject = static_cast<CGameObject*>(g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                pObject->AsSWSTrigger()->m_creatorId));
        }
        if (pObject != 0)
            pKiller = pObject->AsSWSCreature();
    }
    CSWCCMessageData* pData = 0;
    if (pKiller != 0) {
        pData = new CSWCCMessageData;
    } else {
        pKiller = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
            g_pAppManager->m_pServerExoApp->GetPlayerCreatureId());
        if (pKiller != 0)
            pData = new CSWCCMessageData;
    }
    if (pData != 0) {
        pData->SetObjectID(0, m_objectId);
        pData->SetObjectID(1, pKiller->m_objectId);
        pData->SetInteger(0, static_cast<int>(fExperience));
        pKiller->BroadcastDeathDataToParty(pData);
        delete pData;
    }
    g_pAppManager->m_pClientExoApp->AddFloatyText(
        g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId), CClientExoApp::FLOATY_XP,
        nExperience);
}

void CSWSCreature::BroadcastDeathDataToParty(CSWCCMessageData* pData)
{
    if (GetFaction() == 0)
        return;
    unsigned long long* paMembers;
    GetFaction()->GetMemberList(&paMembers);
    for (int i = 0; i < GetFaction()->m_members.m_count; i++) {
        CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(paMembers[i]);
        if (pPlayer != 0) {
            CSWSObject* pObject = pPlayer->GetGameObject();
            if (pObject != 0 && m_cachedAreaId == pObject->m_cachedAreaId) {
                Vector vDelta = pPlayer->GetGameObject()->m_position - m_position;
                if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z <= 900.0f) {
                    g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerCCMessage(
                        pPlayer->m_nPlayerId, CC_MESSAGE_DEATH, pData, 0);
                }
            }
        }
    }
}

int CSWSCreature::IsWieldingLightsaber()
{
    CSWSItem* pItem = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    if (pItem != 0) {
        if (pItem->IsLightsaber() != 0)
            return 1;
    }
    return 0;
}

// While m_bInParty is set the creature's gold/components/chemicals live on the party table
// (m_nGold/1b4/1b8); otherwise the creature's own fields are the live ones.
int CSWSCreature::GetGold()
{
    if (m_bInParty != 0)
        return g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nGold;
    return m_nGold;
}

int CSWSCreature::TransferGold(unsigned long long oidTarget, const Vector& vPosition,
                               unsigned int nAmount)
{
    if (nAmount == 0)
        return 0;
    if (static_cast<unsigned int>(GetGold()) < nAmount)
        nAmount = GetGold();
    CSWSItem* pItem = new CSWSItem(OBJECT_INVALID);
    pItem->LoadFromTemplate(CResRef("g_i_credits001"));
    pItem->m_stackCount = nAmount;
    pItem->SetPossessor(m_objectId, 0, 0, OBJECT_INVALID);
    if (oidTarget == OBJECT_INVALID) {
        AddDropItemActions(pItem->m_objectId, vPosition, 1);
        return 1;
    }
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    if (pObject != 0) {
        if (pObject->AsSWSCreature() != 0) {
            GetItemRepository(1)->AddItem(&pItem, 1, 0, 1);
            RemoveGold(nAmount, 1);
            return 1;
        }
        if (pObject->AsSWSPlaceable() != 0) {
            if (pObject->AsSWSPlaceable()->m_bLocked == 0) {
                AddGiveItemActions(pItem->m_objectId, oidTarget, -1, 0);
                return 1;
            }
        } else if (pObject->AsSWSItem() != 0
                   && g_pRules->m_pBaseItemArray->GetBaseItem(
                          pObject->AsSWSItem()->m_baseItem)->m_bContainer != 0) {
            AddGiveItemActions(pItem->m_objectId, oidTarget, -1, 0);
            return 1;
        }
    }
    delete pItem;
    pItem = 0;
    return 0;
}

int CSWSCreature::EquipMostDamagingMeleeWeapon(unsigned long long oidTarget, int bStrict,
                                               int bNoRetry)
{
    if (GetItemRepository(1) == 0)
        return 0;
    CSWSObject* pTargetObject = 0;
    CSWSCreature* pTarget = 0;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
        pTargetObject = pObject->AsSWSObject();
        if (pTargetObject->AsSWSCreature() != 0)
            pTarget = pTargetObject->AsSWSCreature();
    }
    unsigned int nSlot = CSWInventory::SLOT_LEFTWEAPON;
    if (bStrict == 0) {
        switch (m_pCombatRound->GetWeaponAttackType()) {
        case CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON1: nSlot = CSWInventory::SLOT_CWEAPON_L;   break;
        case CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON2: nSlot = CSWInventory::SLOT_CWEAPON_R;   break;
        case CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON3: nSlot = CSWInventory::SLOT_CWEAPON_B;   break;
        default:                                           nSlot = CSWInventory::SLOT_RIGHTWEAPON; break;
        }
    }
    CSWSItem* pWeapon = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(nSlot));
    if (pWeapon != 0
        && g_pRules->m_pBaseItemArray->GetBaseItem(pWeapon->m_baseItem)->m_bRangedWeapon != 0) {
        UnequipItem(pWeapon);
        GetItemRepository(1)->AddItem(&pWeapon, 0, 0, 1);
        pWeapon = 0;
        if (nSlot == CSWInventory::SLOT_RIGHTWEAPON) {
            CSWSItem* pOffhand =
                    static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
            if (pOffhand != 0) {
                UnequipItem(pOffhand);
                GetItemRepository(1)->AddItem(&pOffhand, 0, 0, 1);
                pOffhand = 0;
            }
        }
    }
    // Strict mode with both hands already empty still wants a baseline to beat, so it adopts
    // whatever is in the right hand provided its base row is the unarmed wield type.
    if (bStrict == 1 && pWeapon == 0) {
        CSWSItem* pFists =
                static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
        if (pFists != 0
            && g_pRules->m_pBaseItemArray->GetBaseItem(pFists->m_baseItem)->m_weaponWieldType
                   == 3) {
            pWeapon = pFists;
            nSlot = CSWInventory::SLOT_RIGHTWEAPON;
        }
    }
    if (pWeapon != 0) {
        UnequipItem(pWeapon);
        GetItemRepository(1)->AddItem(&pWeapon, 0, 0, 1);
    }
    m_pStats->GetDamageRoll(pTargetObject, 0, 0, 0, 1, 1);
    CalculateMaxElementalDamage(pTarget, 0);
    CSWSItem* pBest = 0;
    int nBestDamage = 0;
    for (unsigned int i = 0; i < GetItemRepository(1)->ItemListGetCount(); i++) {
        CSWSItem* pItem = GetItemRepository(1)->ItemListGetItem(i);
        if (pItem == 0)
            continue;
        CSWBaseItem* pBaseItem = g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem);
        if (pBaseItem == 0 || pBaseItem->m_nWeaponType == 0)
            continue;
        unsigned int nTrySlot = nSlot;
        // Per GetRelativeWeaponSize, out of the [-2, 1] band the candidate is unwieldable and the
        // difference collapses to the -10 sentinel that the strict test below rejects.
        int nCreatureSize = m_nCreatureSize;
        int nSizeDiff = g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem)->m_nWeaponSize
                        - nCreatureSize;
        if (nSizeDiff < -2 || nSizeDiff > 1)
            nSizeDiff = -10;
        unsigned char bCanEquip = CanEquipItem(pItem, &nTrySlot, 0, 0, 1);
        if (bStrict != 0) {
            if (bCanEquip != CAN_EQUIP_YES)
                continue;
        } else if (bCanEquip == CAN_EQUIP_NO) {
            continue;
        }
        if (pBaseItem->m_bRangedWeapon != 0)
            continue;
        if (pBaseItem->m_nWeaponType == 0)
            continue;
        if (bStrict != 0) {
            if (nSizeDiff > 0)
                continue;
            if (nSizeDiff == -10)
                continue;
        }
        EquipItem(nTrySlot, pItem, 1, 0, 0);
        int nDamage = m_pStats->GetDamageRoll(pTargetObject, bStrict, 0, 0, 1, 1)
                    + CalculateMaxElementalDamage(pTarget, bStrict);
        if (nDamage >= nBestDamage) {
            nBestDamage = nDamage;
            pBest = pItem;
        }
        UnequipItem(pItem);
    }
    if (pWeapon != 0) {
        GetItemRepository(1)->RemoveItem(pWeapon);
        EquipItem(nSlot, pWeapon, 1, 0, 0);
    }
    if (pBest != 0) {
        if (pWeapon == 0 || pWeapon->m_objectId != pBest->m_objectId)
            AddEquipItemActions(pBest, nSlot, 0, 0, 0, 0, 0, 0);
        return 1;
    }
    if (pWeapon != 0 && oidTarget != OBJECT_INVALID && bNoRetry == 0)
        EquipMostDamagingMeleeWeapon(OBJECT_INVALID, bStrict, 1);
    return 1;
}

int CSWSCreature::EquipMostDamagingRangedWeapon(unsigned long long oidTarget)
{
    CSWSItem* pWeapon = 0;
    if (GetItemRepository(1) == 0)
        return 0;
    CSWSCreature* pTarget = 0;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
        CSWSObject* pSWSObject = pObject->AsSWSObject();
        if (pSWSObject->AsSWSCreature() != 0)
            pTarget = pSWSObject->AsSWSCreature();
    }
    CSWSItem* pBest = 0;
    int nBestModifier = -30;
    pWeapon = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    if (pWeapon != 0) {
        int nBaseItem = pWeapon->m_baseItem;

        CSWBaseItem* pBaseItem = g_pRules->m_pBaseItemArray->GetBaseItem(nBaseItem);
        if (pBaseItem != 0 && pBaseItem->m_bRangedWeapon != 0) {
            nBestModifier = m_pStats->GetAttackModifierVersus(pTarget);
            pBest = pWeapon;
        }
        UnequipItem(pWeapon);
        GetItemRepository(1)->AddItem(&pWeapon, 0, 0, 1);
        CSWSItem* pOffhand =
                static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
        if (pOffhand != 0) {
            UnequipItem(pOffhand);
            GetItemRepository(1)->AddItem(&pOffhand, 0, 0, 1);
            pOffhand = 0;
        }
    }
    for (unsigned int i = 0; i < GetItemRepository(1)->ItemListGetCount(); i++) {
        CSWSItem* pItem = GetItemRepository(1)->ItemListGetItem(i);
        if (pItem == 0)
            continue;
        int nBaseItem = pItem->m_baseItem;

        CSWBaseItem* pBaseItem = g_pRules->m_pBaseItemArray->GetBaseItem(nBaseItem);
        if (pBaseItem == 0 || pBaseItem->m_nWeaponType == 0)
            continue;
        unsigned int nSlot = CSWInventory::SLOT_RIGHTWEAPON;
        if (CanEquipItem(pItem, &nSlot, 0, 0, 1) == CAN_EQUIP_NO)
            continue;
        if (pBaseItem->m_bRangedWeapon == 0)
            continue;
        EquipItem(nSlot, pItem, 1, 0, 0);
        int nModifier = m_pStats->GetAttackModifierVersus(pTarget);
        if (nModifier > nBestModifier) {
            nBestModifier = nModifier;
            pBest = pItem;
        }
        UnequipItem(pItem);
    }
    if (pWeapon != 0) {
        GetItemRepository(1)->RemoveItem(pWeapon);
        EquipItem(CSWInventory::SLOT_RIGHTWEAPON, pWeapon, 1, 0, 0);
    }
    if (pBest != 0 && pWeapon != pBest) {
        AddEquipItemActions(pBest, CSWInventory::SLOT_RIGHTWEAPON, 0, 0, 0, 0, 0, 0);
        return 1;
    }
    return 0;
}

#if !KOTOR1
int CSWSCreature::GetComponent()
{
    if (m_bInParty != 0)
        return g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nItemComponent;
    return m_nComponent;
}
#endif

#if !KOTOR1
int CSWSCreature::GetChemical()
{
    if (m_bInParty != 0)
        return g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nItemChemical;
    return m_nChemical;
}
#endif

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWSCreature::AddComponent(int nComponent, int bFeedback)
{
    int nNew = GetComponent() + nComponent;
    if (nNew > 999999999)
        nNew = 999999999;
    int nDelta = nNew - GetComponent();
    SetComponent(nNew);
    if (bFeedback == 1 && nDelta != 0) {
        CSWCCMessageData* pData = new CSWCCMessageData();
        pData->SetInteger(0, nDelta);
        SendFeedbackMessage(FEEDBACK_COMPONENTS_ACQUIRED, pData);
    }
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWSCreature::AddChemical(int nChemical, int bFeedback)
{
    int nNew = GetChemical() + nChemical;
    if (nNew > 999999999)
        nNew = 999999999;
    int nDelta = nNew - GetChemical();
    SetChemical(nNew);
    if (bFeedback == 1 && nDelta != 0) {
        CSWCCMessageData* pData = new CSWCCMessageData();
        pData->SetInteger(0, nDelta);
        SendFeedbackMessage(FEEDBACK_CHEMICALS_ACQUIRED, pData);
    }
}
#endif // !KOTOR1

void CSWSCreature::RemoveGold(int nGold, int bFeedback)
{
    int nNew = GetGold() - nGold;
    if (nNew < 0)
        nNew = 0;
    int nDelta = GetGold() - nNew;
    SetGold(nNew);
    if (bFeedback == 1 && nDelta != 0) {
        CSWCCMessageData* pData = new CSWCCMessageData();
        pData->SetInteger(0, nDelta);
        SendFeedbackMessage(FEEDBACK_CREDITS_LOST, pData);
    }
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWSCreature::RemoveComponent(int nComponent, int bFeedback)
{
    int nNew = GetComponent() - nComponent;
    if (nNew < 0)
        nNew = 0;
    int nDelta = GetComponent() - nNew;
    SetGold(nNew);
    if (bFeedback == 1 && nDelta != 0) {
        CSWCCMessageData* pData = new CSWCCMessageData();
        pData->SetInteger(0, nDelta);
        SendFeedbackMessage(FEEDBACK_CREDITS_LOST, pData);
    }
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWSCreature::RemoveChemical(int nChemical, int bFeedback)
{
    int nNew = GetChemical() - nChemical;
    if (nNew < 0)
        nNew = 0;
    int nDelta = GetChemical() - nNew;
    SetGold(nNew);
    if (bFeedback == 1 && nDelta != 0) {
        CSWCCMessageData* pData = new CSWCCMessageData();
        pData->SetInteger(0, nDelta);
        SendFeedbackMessage(FEEDBACK_CREDITS_LOST, pData);
    }
}
#endif // !KOTOR1

void CSWSCreature::SetActivity(int activity, int a, int b)
{
    if ((activity & ~m_nLockedActivityFlags) == 0)
        return;
    if (((activity & ACTIVITY_EXCLUSIVE_MASK) != 0 && a == 1)
        || (activity == ACTIVITY_EXCLUSIVE_MASK && a == 0))
        ClearActivities(ACTIVITY_EXCLUSIVE_MASK);
    if (((activity & ACTIVITY_COMBAT_MODE_MASK) != 0 && a == 1)
        || (activity == ACTIVITY_COMBAT_MODE_MASK && a == 0))
        ClearActivities(ACTIVITY_COMBAT_MODE_MASK);
    unsigned int nFlags = m_activityFlags;
    unsigned int nChanged = nFlags & activity;
    if (a != 1) {
        nFlags &= ~activity;
    } else {
        nChanged = (nChanged == 0);
        nFlags |= activity;
    }
    m_activityFlags = nFlags;
    if ((activity & ACTIVITY_STEALTH) != 0 && nChanged != 0) {
        if (a == 1) {
            if ((nFlags & (ACTIVITY_CONVERSATION | ACTIVITY_RESTING)) != 0) {
                m_activityFlags = nFlags & ~activity;
            } else {
                SetStealthMode(1, b);
            }
        } else if (a == 0) {
            SetStealthMode(0, b);
        } else {
            m_activityFlags = nFlags & ~activity;
        }
    }
    if (((activity & ACTIVITY_DETECT) != 0) & (nChanged != 0))
        SetDetectMode(a != 0);
    if (nChanged != 0 && (activity & ACTIVITY_CONVERSATION) != 0 && a == 0)
        StopDialog();
}

#if KOTOR1
int CSWSCreature::AddUnequipActions(CSWSItem* pItem, unsigned long long a2, int bFront, int a4,
                                    int a5)
#else
int CSWSCreature::AddUnequipActions(CSWSItem* pItem, unsigned long long a2, int bFront, int a4,
                                    int a5, int a6, int a7)
#endif
{
    unsigned long long oidItem = pItem->m_objectId;
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    if (m_bCommandable == 0)
        return 0;
    if (m_bCombatState == 1 && m_nCombatType == 1) {
        CSWSItem* pEquipped = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_BODY));
        if (pEquipped != 0 && pEquipped->m_objectId == oidItem) {
            if (pPlayer != 0 && pMessage != 0)
                pMessage->SendServerToPlayerInventory_UnequipCancel(pPlayer->m_nPlayerId, oidItem);
            SendFeedbackMessage(FEEDBACK_ARMOR_UNEQUIP_IN_COMBAT, 0);
            return 0;
        }
    }
    if (ReplaceUnequipItemActions(pItem, a2, a4, a5) == 0) {
        if (m_bCombatState == 1 && m_nCombatType == 1 && a4 == 0) {
            m_pCombatRound->AddUnequipAction(oidItem, a2, a5);
        } else {
#if KOTOR1  // K1: the action carries the item, the target and the slot only
            int nSlot = a5;
            if (bFront != 0) {
                AddActionToFront(AIACTION_UNEQUIP_ITEM, ACTION_GROUP_NEW,
                                 ACTION_PARAM_OBJECT, &oidItem, ACTION_PARAM_OBJECT, &a2,
                                 ACTION_PARAM_INT, &nSlot, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0);
            } else {
                AddAction(AIACTION_UNEQUIP_ITEM, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidItem,
                          ACTION_PARAM_OBJECT, &a2, ACTION_PARAM_INT, &nSlot, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            }
#else
            int nSlot = a5;
            int n6 = a6;
            int n7 = a7;
            if (bFront != 0) {
                AddActionToFront(AIACTION_UNEQUIP_ITEM, ACTION_GROUP_NEW,
                                 ACTION_PARAM_OBJECT, &oidItem, ACTION_PARAM_OBJECT, &a2,
                                 ACTION_PARAM_INT, &nSlot, ACTION_PARAM_INT, &n6,
                                 ACTION_PARAM_INT, &n7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0);
            } else {
                AddAction(AIACTION_UNEQUIP_ITEM, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidItem,
                          ACTION_PARAM_OBJECT, &a2, ACTION_PARAM_INT, &nSlot, ACTION_PARAM_INT, &n6,
                          ACTION_PARAM_INT, &n7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            }
#endif
        }
    }
    return 1;
}

#if KOTOR1
int CSWSCreature::AddEquipItemActions(CSWSItem* pItem, unsigned int nSlot, int bFront, int a4,
                                      int a5, int a6, int a7)
{
    if (m_pInventory != 0
#else
int CSWSCreature::AddEquipItemActions(CSWSItem* pItem, unsigned int nSlot, int bFront, int a4,
                                      int a5, int a6, int a7, int a8)
{
    if (m_pInventory != 0 && a8 == 0
#endif
        && (nSlot == CSWInventory::SLOT_HEAD || nSlot == CSWInventory::SLOT_BODY || nSlot == CSWInventory::SLOT_RIGHTWEAPON || nSlot == CSWInventory::SLOT_LEFTWEAPON) && pItem != 0) {
        if (m_pInventory->GetItemInInventory(pItem) != 0)
            return 0;
    }
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    K2_ASSUME(pItem != 0);
    unsigned long long oidItem = pItem->m_objectId;
    if (m_bCommandable == 0) {
        if (pPlayer != 0 && pMessage != 0)
            pMessage->SendServerToPlayerInventory_EquipCancel(pPlayer->m_nPlayerId, oidItem, nSlot);
        return 0;
    }
    if (m_bCombatState == 1 && m_nCombatType == 1 && nSlot == 2) {
        if (pPlayer != 0 && pMessage != 0)
            pMessage->SendServerToPlayerInventory_EquipCancel(pPlayer->m_nPlayerId, oidItem, nSlot);
        SendFeedbackMessage(FEEDBACK_ARMOR_EQUIP_IN_COMBAT, 0);
        return 0;
    }
    if (a7 == 0) {
        if (ReplaceEquipItemActions(pItem, nSlot, a4, a5) != 0)
            return 1;
    }
    if (m_bCombatState == 1 && m_nCombatType == 1 && a4 == 0) {
        m_pCombatRound->AddEquipAction(oidItem, nSlot, a5, a6);
        return 1;
    }
    if (bFront != 0) {
        AddActionToFront(AIACTION_EQUIP_ITEM, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidItem,
                         ACTION_PARAM_INT, &nSlot, ACTION_PARAM_INT, &a5, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    } else {
        AddAction(AIACTION_EQUIP_ITEM, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidItem,
                  ACTION_PARAM_INT, &nSlot, ACTION_PARAM_INT, &a5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                  0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    return 1;
}

int CSWSCreature::EquipMostEffectiveArmor()
{
    if (m_pInventory == 0)
        return 0;
    if (m_pItemRepository == 0)
        return 0;

    CSWSItem* pEquipped = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_BODY));
    CSWSItem* pBest = 0;
    unsigned int nBestArmorClass = 0;
    if (pEquipped != 0
            && g_pRules->m_pBaseItemArray->GetBaseItem(pEquipped->m_baseItem) != 0) {
        nBestArmorClass = pEquipped->m_nBaseArmorClass;
        for (int nProperty = 0; nProperty < pEquipped->m_nPassiveProperties; nProperty++) {
            CSWItemProperty* pProperty = pEquipped->GetPassiveProperty(nProperty);
            if (pProperty != 0
                    && (pProperty->m_nUpgradeType == CSWItemProperty::UPGRADE_TYPE_NONE
                        || (pEquipped->m_nUpgrades & (1 << pProperty->m_nUpgradeType)) != 0)
                    && pProperty->m_type == ITEM_PROPERTY_AC_BONUS)
                nBestArmorClass += pProperty->m_costTableValue;
        }
        pBest = pEquipped;
    }

    for (unsigned int nItem = 0; nItem < m_pItemRepository->ItemListGetCount(); nItem++) {
        CSWSItem* pItem = m_pItemRepository->ItemListGetItem(nItem);
        if (pItem == 0)
            continue;
        unsigned int nSlot = 2;
        if (pItem->GetBaseItem()->m_nItemType < ITEM_TYPE_ARMOR_CLASS_4)
            continue;
        if (pItem->GetBaseItem()->m_nItemType > ITEM_TYPE_JEDI_ROBE)
            continue;
        if (CanEquipItem(pItem, &nSlot, 0, 0, 1) == CAN_EQUIP_NO)
            continue;
        if (g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem) == 0)
            continue;
        unsigned int nArmorClass = pItem->m_nBaseArmorClass;
        for (int nProperty = 0; nProperty < pItem->m_nPassiveProperties; nProperty++) {
            CSWItemProperty* pProperty = pItem->GetPassiveProperty(nProperty);
            if (pProperty != 0
                    && (pProperty->m_nUpgradeType == CSWItemProperty::UPGRADE_TYPE_NONE
                        || (pItem->m_nUpgrades & (1 << pProperty->m_nUpgradeType)) != 0)
                    && pProperty->m_type == ITEM_PROPERTY_AC_BONUS)
                nArmorClass += pProperty->m_costTableValue;
        }
        if (nArmorClass > nBestArmorClass) {
            nBestArmorClass = nArmorClass;
            pBest = pItem;
        }
    }

    if (pBest == 0)
        return 0;
    if (pEquipped != pBest)
        AddEquipItemActions(pBest, 2, 0, 0, 0, 0, 0, 0);
    return 1;
}


int CSWSCreature::AddStanceActions(int nStance, unsigned long long oidTarget, int a3,
                                   int bClearActions, int bFront, int a6)
{
    g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
    if (m_bCommandable == 0)
        return 0;
    switch (nStance) {
    case STANCE_POSTURE_TOTAL_DEFENSE:
        if (a6 != 0) {
            ComputeAIState();
            if (m_bCommandable == 0)
                return 0;
            if (bClearActions != 0)
                ClearAllActions(1);
            if (bFront != 0) {
                AddActionToFront(AIACTION_STANCE_TOTAL_DEFENSE, ACTION_GROUP_NEW,
                                 ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            } else {
                AddAction(AIACTION_STANCE_TOTAL_DEFENSE, ACTION_GROUP_NEW,
                          ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            }
            BroadcastCombatStateToParty(m_objectId);
        } else {
            m_pCombatRound->AddStanceAction(STANCE_POSTURE_TOTAL_DEFENSE, oidTarget, a3);
            if (m_bCombatState == 0
                && g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId) != 0) {
                ComputeAIState();
                ClearAllActions(1);
            }
            if (GetNodeById(AIACTION_COMBAT) == 0) {
                int nOne = 1;
                AddActionToFront(AIACTION_COMBAT, ACTION_GROUP_NEW, ACTION_PARAM_INT, &nOne, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            }
        }
        break;
    case STANCE_POSTURE_MEDITATIVE:
        AddAction(AIACTION_STANCE_MEDITATIVE, ACTION_GROUP_NEW, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        break;
    }
    return 1;
}

int CSWSCreature::AddMineActions(unsigned long long oidItem, unsigned char nAction,
                                 unsigned long long oidTarget, const Vector& vPosition,
                                 unsigned long long a5)
{
    K2_UNUSED(a5);
    if (m_bCommandable == 0)
        return 0;
    if (nAction == AIACTION_SET_MINE && GetArea() != 0
            && static_cast<CSWSArea*>(GetArea())->GetPlayerCanSetMines() == 0)
        return 0;
    ActionManager(2);
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidItem);
    if (pObject == 0)
        return 0;
#if !KOTOR1  // K1: no discarded target lookup
    g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
#endif
    if (static_cast<unsigned char>(nAction - AIACTION_DISARM_MINE) <= 1) {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER
                && pObject->AsSWSTrigger()->m_bTrapDisarmable == 0
                && pObject->AsSWSTrigger()->m_bTrap == 1)
            return 0;
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR
                && pObject->AsSWSDoor()->m_bTrapDisarmable == 0
                && pObject->AsSWSDoor()->m_bTrapFlag == 1)
            return 0;
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE
                && pObject->AsSWSPlaceable()->m_bTrapDisarmable == 0
                && pObject->AsSWSPlaceable()->m_bTrapFlag == 1)
            return 0;
    }
    if (m_pStats->GetCanUseSkill(SKILL_DEMOLITIONS) == 0)
        return 0;
    if (nAction == AIACTION_SET_MINE) {
        float fX = vPosition.x;
        float fY = vPosition.y;
        float fZ = vPosition.z;
        AddAction(nAction, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidItem,
                  ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_FLOAT, &fX, ACTION_PARAM_FLOAT, &fY,
                  ACTION_PARAM_FLOAT, &fZ, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    } else {
        AddAction(nAction, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidItem, 0, 0, 0, 0, 0, 0, 0, 0,
                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    return 1;
}

int CSWSCreature::UseSkill(unsigned char nSkill, unsigned char nMineType,
                           unsigned long long oidTarget, const Vector& vPosition,
                           unsigned long long a5, unsigned long long a6, int a7)
{
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget, &pObject)
            != CGameObjectArray::SUCCESS) {
        Vector vZero;
        if (equal(vPosition, vZero))
            return 1;
    }
    if (m_pStats->GetCanUseSkill(nSkill) == 0) {
        SendFeedbackMessage(FEEDBACK_CANNOT_USE_SKILL, 0);
        return 0;
    }
    if (pObject != 0 && pObject->AsSWSCreature() != 0
            && (g_pRules->m_paSkills[nSkill].m_flags & 1) != 0
            && GetAIStateReputation(oidTarget) == 1) {
        SendFeedbackMessage(FEEDBACK_SKILL_TARGET_HOSTILE, 0);
        return 0;
    }
    switch (nSkill) {
    case SKILL_DEMOLITIONS:
        if (nMineType == SUBSKILL_FLAGTRAP)
            AddMineActions(oidTarget, AIACTION_FLAG_MINE, OBJECT_INVALID, vPosition, a5);
        else if (nMineType == SUBSKILL_RECOVERTRAP)
            AddMineActions(oidTarget, AIACTION_RECOVER_MINE, OBJECT_INVALID, vPosition, a5);
        else if (nMineType == SUBSKILL_EXAMINETRAP)
            AddMineActions(oidTarget, AIACTION_EXAMINE_MINE, OBJECT_INVALID, vPosition, a5);
        else
            AddMineActions(oidTarget, AIACTION_DISARM_MINE, OBJECT_INVALID, vPosition, a5);
        break;
    case SKILL_STEALTH:
        if (m_pStats->GetCanUseSkill(SKILL_STEALTH) != 0)
            ToggleMode(TOGGLE_MODE_STEALTH);
        break;
#if KOTOR1  // K1: awareness toggles detect mode, which ToggleMode ignores
    case SKILL_AWARENESS:
        ToggleMode(TOGGLE_MODE_DETECT);
        break;
#else
    case SKILL_AWARENESS:
        if (GetDead() == 0 && m_bInParty != 0)
            GetCurrentHitPoints(0);
        break;
#endif
    case SKILL_SECURITY:
        ActionManager(2);
        AddUnlockObjectAction(oidTarget, a6, a7);
        break;
    case SKILL_TREAT_INJURY:
        AddHealActions(oidTarget, a6, a7);
        break;
    }
    return 1;
}

int CSWSCreature::UseItem(unsigned long long oidItem, unsigned char nProperty, unsigned char a3,
                          unsigned long long oidTarget, const Vector& vTarget,
                          unsigned long long a6, int a7)
{
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    if (pItem == 0)
        return 0;
    if (CanUseItem(pItem, 0, 0) == 0)
        return 0;
    CSWItemProperty* pProperty = pItem->GetActiveProperty(nProperty);
    if (pProperty == 0)
        return 0;
    int nResult = 0;
    if ((pProperty->m_nUpgradeType == CSWItemProperty::UPGRADE_TYPE_NONE
         || (pItem->m_nUpgrades & (1 << pProperty->m_nUpgradeType)) != 0)
        && pProperty->m_type == ITEM_PROPERTY_ACTIVATE_ITEM) {
        if (pProperty->m_bUseableByte != 0) {
            if (pObject != 0 && pObject->AsSWSCreature() != 0) {
                CSWVisibilityNode* pNode = GetVisibleListElement(oidTarget);
                if (pNode != 0 && (pNode->m_flags & VISIBILITY_PACIFY_STATE_MASK) == VISIBILITY_PACIFIED)
                    return CSWSObject::ACTION_COMPLETE;
            }
            g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem);
            Vector vPos = vTarget;
            int bNoTarget = oidTarget == OBJECT_INVALID;
            int bRoundPending = 0;
            if (m_bCombatState != 0)
                bRoundPending = GetNodeById(AIACTION_PHYSICAL_ATTACKS) != 0;
            nResult = AddItemCastSpellActions(oidItem, nProperty, a3, vPos, oidTarget, a7,
                                              bNoTarget, bRoundPending, 0);
        }
    } else if (pProperty->m_type == ITEM_PROPERTY_TRAP) {
        nResult = AddMineActions(pItem->m_objectId, AIACTION_SET_MINE, oidTarget, vTarget, a6);
    } else if (pProperty->m_type == ITEM_PROPERTY_SECURITY_SPIKE) {
        nResult = UseSkill(SKILL_SECURITY, 0, oidTarget, vTarget, a6, oidItem, nProperty);
    }
#if !KOTOR1
    if (m_bInParty != 0) {
        CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
        if (pPartyTable != 0) {
            if ((pPartyTable->m_forfeitConditions & FORFEIT_NO_ITEMS) != 0) {
                pPartyTable->m_forfeitViolations = FORFEIT_NO_ITEMS;
            } else if ((pPartyTable->m_forfeitConditions & FORFEIT_NO_ITEM_BUT_SHIELD) != 0
                       && pItem->GetBaseItem()->m_nItemType != ITEM_TYPE_FOREARM_BANDS) {
                pPartyTable->m_forfeitViolations = FORFEIT_NO_ITEM_BUT_SHIELD;
            } else {
                return nResult;
            }
            CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
            if (pArea != 0) {
                CScriptEvent* pEvent = new CScriptEvent();
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_USER_DEFINED_EVENT;
                pEvent->SetInteger(0, USER_EVENT_FORFEIT_VIOLATED);
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, pArea->m_objectId, pArea->m_objectId,
                    CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            }
        }
    }
#endif
    return nResult;
}

int CSWSCreature::ReplaceUnequipItemActions(CSWSItem* pItem, unsigned long long objectId,
                                            int bSkipCombatRound, int nSlot)
{
    K2_UNUSED(objectId);
    unsigned long long oidItem = pItem->m_objectId;
    if (bSkipCombatRound == 0) {
        if (m_pCombatRound->m_pRoundActionList != 0) {
            CExoLinkedListNode* pNode =
                m_pCombatRound->m_pRoundActionList->m_pInternal->m_pHead;
            while (pNode != 0) {
                CSWSCombatRoundAction* pAction = static_cast<CSWSCombatRoundAction*>(
                    m_pCombatRound->m_pRoundActionList->GetAtPos(pNode));
                if (pAction != 0 && pAction->m_nActionType == CSWSCombatRoundAction::TYPE_UNEQUIP &&
                    pAction->m_oidTarget == oidItem) {
                    pAction->m_oidTarget = oidItem;
                    return 1;
                }
                m_pCombatRound->m_pRoundActionList->GetNext(pNode);
            }
        }
    }
    CExoLinkedListNode* pNode = m_actionList.m_pInternal->m_pHead;
    while (pNode != 0) {
        CSWSObjectActionNode* pAction = static_cast<CSWSObjectActionNode*>(m_actionList.GetAtPos(pNode));
        if (pAction != 0 && pAction->m_nActionId == AIACTION_UNEQUIP_ITEM
            && pAction->m_args.m_itemId == oidItem) {
            pAction->m_args.m_itemId = oidItem;
            pAction->m_args.m_equipSlot = nSlot;
            return 1;
        }
        m_actionList.GetNext(pNode);
    }
    return 0;
}

#if !KOTOR1
int CSWSCreature::GetUseRange(unsigned long long objectId, Vector& vPos, float& fRange,
                              int a, int b)
{
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(objectId);
    fRange = m_pPathfindInfo->m_fPersonalSpace;
    if (pObject == 0)
        return 0;
    if (pObject->AsSWSObject() == 0)
        return 0;
    if (pObject->AsSWSObject()->GetArea() == 0)
        return 0;

    switch (pObject->m_objectType) {
        case GAME_OBJECT_TYPE_CREATURE:  // OBJECT_TYPE_CREATURE
            // Both fRange stores are real. The first is dead by value, but fRange is a float&
            // into the calling scope and the AsSWSCreature() virtual in between could read it,
            // so it cannot be dropped.
            if (m_pPathfindInfo->m_oidTarget == objectId
                    && m_pPathfindInfo->m_oidTarget != OBJECT_INVALID)
                fRange = m_pPathfindInfo->m_fDesiredOffset;
            else
                fRange = m_pPathfindInfo->m_fCrePersonalSpace + 0.3f;
            vPos = pObject->AsSWSObject()->m_position;
            pObject->AsSWSCreature();
            fRange = 1.4f;
            break;

        case GAME_OBJECT_TYPE_TRIGGER: {  // OBJECT_TYPE_TRIGGER
            CSWSTrigger* pTrigger = pObject->AsSWSTrigger();
            // An area transition is "used" from its centre; any other trigger is entered at
            // whatever point of its polygon is nearest us.
            if (pTrigger->m_bAreaTransition != 0) {
                vPos = pTrigger->m_position;
                break;
            }
            pTrigger->CalculateNearestPoint(m_position, &vPos);
            fRange += 0.5f;
            break;
        }

        case GAME_OBJECT_TYPE_PLACEABLE:  // OBJECT_TYPE_PLACEABLE
            vPos = pObject->AsSWSPlaceable()->GetNearestActionPoint(m_position);
            if (pObject->AsSWSPlaceable()->m_bPreciseUse != 0 && a == 0
                    && static_cast<CSWSArea*>(GetArea())->TestSafeLocationPoint(vPos, m_pPathfindInfo) == 1)
                fRange = 0.1f;
            else
                fRange += 0.75f;
            // Corpses are looted from much further out.
            if (pObject->AsSWSPlaceable()->m_corpse.m_bIsCorpse != 0)
                fRange += 5.0f;
            break;

        case GAME_OBJECT_TYPE_DOOR: {  // OBJECT_TYPE_DOOR
            CSWSObject* pSelf = AsSWSObject();
            int bOpeningDoor = 0;
            CSWSObjectActionNode* pAction = pSelf->m_actionList.GetHead();
            CExoLinkedListNode* pNode = pSelf->m_actionList.GetHeadPos();
            while (pAction != 0) {
                if (static_cast<unsigned short>(pAction->m_nActionId) == AIACTION_DOOR_SABER) {
                    bOpeningDoor = 1;
                    break;
                }
                pAction = static_cast<CSWSObjectActionNode*>(pSelf->m_actionList.GetNext(pNode));
            }

            vPos = pObject->AsSWSDoor()->GetNearestActionPoint(m_position, 0);
            vPos.z = static_cast<CSWSArea*>(pObject->AsSWSDoor()->GetArea())->ComputeHeight(vPos, 1, 0, 0);

            if (m_pPathfindInfo->m_oidTarget == objectId
                    && m_pPathfindInfo->m_oidTarget != OBJECT_INVALID)
                fRange = m_pPathfindInfo->m_fDesiredOffset + fRange;
            else if (bOpeningDoor != 0 || b != 0)
                fRange = 1.0f;
            else if (pObject->AsSWSDoor()->m_bPreciseUse != 0
                    && pObject->AsSWSDoor()->m_bLocked != 0 && a == 0
                    && static_cast<CSWSArea*>(GetArea())->TestSafeLocationPoint(vPos, m_pPathfindInfo) == 1)
                fRange = 0.1f;
            else
                fRange += 0.75f;
            break;
        }

        default:
            vPos = pObject->AsSWSObject()->m_position;
            break;
    }
    return 1;
}
#else
int CSWSCreature::GetUseRange(unsigned long long objectId, Vector& vPos, float& fRange, int a)
{
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(objectId);
    fRange = m_pPathfindInfo->m_fPersonalSpace;
    if (pObject == 0)
        return 0;
    if (pObject->AsSWSObject() == 0)
        return 0;
    if (pObject->AsSWSObject()->GetArea() == 0)
        return 0;

    switch (pObject->m_objectType) {
        case GAME_OBJECT_TYPE_CREATURE:  // OBJECT_TYPE_CREATURE
            if (m_pPathfindInfo->m_oidTarget == objectId
                    && m_pPathfindInfo->m_oidTarget != OBJECT_INVALID)
                fRange = m_pPathfindInfo->m_fDesiredOffset;
            else
                fRange = m_pPathfindInfo->m_fCrePersonalSpace;
            vPos = pObject->AsSWSObject()->m_position;
            fRange = pObject->AsSWSCreature()->m_pPathfindInfo->m_fCrePersonalSpace + fRange + 0.3f;
            break;

        case GAME_OBJECT_TYPE_TRIGGER: {  // OBJECT_TYPE_TRIGGER
            CSWSTrigger* pTrigger = pObject->AsSWSTrigger();
            if (pTrigger->m_bAreaTransition != 0) {
                vPos = pTrigger->m_position;
                break;
            }
            pTrigger->CalculateNearestPoint(m_position, &vPos);
            fRange += 0.5f;
            break;
        }

        case GAME_OBJECT_TYPE_PLACEABLE:  // OBJECT_TYPE_PLACEABLE
            vPos = pObject->AsSWSPlaceable()->GetNearestActionPoint(m_position);
            if (pObject->AsSWSPlaceable()->m_bPreciseUse != 0 && a == 0
                    && static_cast<CSWSArea*>(GetArea())->TestSafeLocationPoint(vPos, m_pPathfindInfo) == 1)
                fRange = 0.1f;
            else
                fRange += 0.75f;
            if (pObject->AsSWSPlaceable()->m_corpse.m_bIsCorpse != 0)
                fRange += 5.0f;
            break;

        case GAME_OBJECT_TYPE_DOOR:  // OBJECT_TYPE_DOOR
            vPos = pObject->AsSWSDoor()->GetNearestActionPoint(m_position, 0);
            vPos.z = static_cast<CSWSArea*>(pObject->AsSWSDoor()->GetArea())->ComputeHeight(vPos, 1, 0);
            if (m_pPathfindInfo->m_oidTarget == objectId
                    && m_pPathfindInfo->m_oidTarget != OBJECT_INVALID)
                fRange = m_pPathfindInfo->m_fDesiredOffset + fRange;
            else if (pObject->AsSWSDoor()->m_bPreciseUse != 0
                    && pObject->AsSWSDoor()->m_bLocked != 0 && a == 0
                    && static_cast<CSWSArea*>(GetArea())->TestSafeLocationPoint(vPos, m_pPathfindInfo) == 1)
                fRange = 0.1f;
            else
                fRange += 0.75f;
            break;

        default:
            vPos = pObject->AsSWSObject()->m_position;
            break;
    }
    return 1;
}
#endif

float CSWSCreature::DesiredAttackRange(unsigned long long objectId, int a)
{
    float fRange = 0.0f;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(objectId);
    if (pObject != 0) {
        if (pObject->AsSWSCreature() != 0) {
            CSWSCreature* pCreature = pObject->AsSWSCreature();
            if (a != 0) {
#if KOTOR1  // K1: both personal spaces + 0.7
                fRange = m_pPathfindInfo->m_fCrePersonalSpace + 0.5f
                         + pCreature->m_pPathfindInfo->m_fCrePersonalSpace + 0.2f;
#else
                fRange = 1.4f;
#endif
            } else {
                float fRadius = 0.0f;
                g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
                    m_appearance.m_headId, CTwoDimArrays::COL2DA_APPEARANCETABLE_HITRADIUS,
                    &fRadius);
                fRange = fRadius + 1.6f;
                g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
                    pCreature->m_appearance.m_headId,
                    CTwoDimArrays::COL2DA_APPEARANCETABLE_HITRADIUS, &fRadius);
                fRange = fRange + fRadius;
            }
        } else if (pObject->AsSWSDoor() != 0) {
            Vector vPos = pObject->AsSWSDoor()->m_position;
            GetUseRange(objectId, vPos, fRange, 0, 0);
        } else if (pObject->AsSWSPlaceable() != 0) {
            Vector vPos = pObject->AsSWSPlaceable()->m_position;
            GetUseRange(objectId, vPos, fRange, 0, 0);
        } else {
            fRange = 1.5f;
        }
    }
    return fRange;
}

#if KOTOR1
int CSWSCreature::GetIsInUseRange(unsigned long long objectId, float fExtra, int a)
#else
int CSWSCreature::GetIsInUseRange(unsigned long long objectId, float fExtra, int a, int b)
#endif
{
    int nInRange = 0;
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(objectId);
    Vector vPos = {0.0f, 0.0f, 0.0f};
    float fRange = 0.0f;
    if (pObject != 0 && pObject->AsSWSObject() != 0
        && pObject->AsSWSObject()->GetArea() != 0) {
        if (m_cachedAreaId != pObject->AsSWSObject()->m_cachedAreaId) {
            return 0;
        }
        if (pObject->AsSWSTrigger() != 0 && pObject->AsSWSTrigger()->m_bAreaTransition != 0) {
            nInRange = pObject->AsSWSTrigger()->InTrigger(m_position);
        } else {
#if KOTOR1
            GetUseRange(objectId, vPos, fRange, 0);
#else
            GetUseRange(objectId, vPos, fRange, 0, b);
#endif
            Vector vOut = {0.0f, 0.0f, 0.0f};
            unsigned long long idOut;
            Vector vSelf(m_position);
            vSelf.z += 1.5f;
            Vector vTarget(vPos);
            vTarget.z += 1.5f;
            if (static_cast<CSWSArea*>(pObject->AsSWSObject()->GetArea())->ClearLineOfSight(
                    vSelf, vTarget, &vOut, &idOut, m_objectId, objectId, a, 0)) {
                float fLimit = fExtra + 0.1f + fRange;
                vTarget.z = 0.0f;
                vSelf.z = 0.0f;
                Vector vDelta = vTarget - vSelf;
                nInRange = vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z
                           <= fLimit * fLimit;
            }
        }
    }
    return nInRange;
}

int CSWSCreature::ToggleMode(unsigned char nMode)
{
    if (GetDead())
        return 0;
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp()
    if (GetIsPCDying() || GetDeadTemp())
        return 0;
#else
    if (m_bInParty != 0 && GetCurrentHitPoints(0) < 1)
        return 0;
#endif
    switch (nMode) {
    case TOGGLE_MODE_STEALTH:
        if (m_pStats->GetCanUseSkill(SKILL_STEALTH) == 0)
            return 1;
        if (m_stealthMode == 1) {
            SetActivity(ACTIVITY_STEALTH, 0, 0);
        } else if (m_bCombatState != 0) {
            SendFeedbackMessage(FEEDBACK_STEALTH_IN_COMBAT, 0);
        } else {
            SetActivity(ACTIVITY_STEALTH, 1, 0);
        }
        return 1;
    case TOGGLE_MODE_PARRY:
        if (m_combatMode == CREATURE_COMBAT_MODE_PARRY)
            SetCombatMode(CREATURE_COMBAT_MODE_NONE, 1);
        else
            SetCombatMode(CREATURE_COMBAT_MODE_PARRY, 0);
        return 1;
    case TOGGLE_MODE_POWER_ATTACK:
        if (m_combatMode == CREATURE_COMBAT_MODE_POWER_ATTACK)
            SetCombatMode(CREATURE_COMBAT_MODE_NONE, 1);
        else
            SetCombatMode(CREATURE_COMBAT_MODE_POWER_ATTACK, 0);
        return 1;
    case TOGGLE_MODE_IMPROVED_POWER_ATTACK:
        if (m_combatMode == CREATURE_COMBAT_MODE_IMPROVED_POWER_ATTACK)
            SetCombatMode(CREATURE_COMBAT_MODE_NONE, 1);
        else
            SetCombatMode(CREATURE_COMBAT_MODE_IMPROVED_POWER_ATTACK, 0);
        return 1;
    case TOGGLE_MODE_COUNTERSPELL:
        if (m_combatMode == CREATURE_COMBAT_MODE_COUNTERSPELL)
            SetCombatMode(CREATURE_COMBAT_MODE_NONE, 1);
        else
            SetCombatMode(CREATURE_COMBAT_MODE_COUNTERSPELL, 0);
        return 1;
    case TOGGLE_MODE_FLURRY:
        if (m_combatMode == CREATURE_COMBAT_MODE_FLURRY)
            SetCombatMode(CREATURE_COMBAT_MODE_NONE, 1);
        else
            SetCombatMode(CREATURE_COMBAT_MODE_FLURRY, 0);
        return 1;
    case TOGGLE_MODE_RAPID_SHOT:
        if (m_combatMode == CREATURE_COMBAT_MODE_RAPID_SHOT)
            SetCombatMode(CREATURE_COMBAT_MODE_NONE, 1);
        else
            SetCombatMode(CREATURE_COMBAT_MODE_RAPID_SHOT, 0);
        return 1;
    }
    return 1;
}

#if !KOTOR1
int CSWSCreature::Disguise(int nAppearance, CGameEffect* pEffect, int bNoMove, int bKeepOld)
{
    K2_UNUSED(pEffect);
    CExoString sString;  // constructed and destroyed, never read
    if (m_bDisguised != 0)
        return 0;
    CSWSCreatureStats* pStats = m_pStats;
    if (bKeepOld != 0) {
        pStats->m_nAppearanceType = static_cast<unsigned short>(nAppearance);
        m_appearance.m_headId = static_cast<unsigned short>(nAppearance);
        m_nDisguiseAppearance = pStats->m_nAppearanceType;
        m_bDisguised = 0;
    } else {
        unsigned short nOldAppearance = pStats->m_nAppearanceType;
        pStats->m_nAppearanceType = static_cast<unsigned short>(nAppearance);
        m_appearance.m_headId = static_cast<unsigned short>(nAppearance);
        m_nDisguiseAppearance = nOldAppearance;
        m_bDisguised = 1;
    }
    UpdatePersonalSpace();
    UpdateAppearanceDependantInfo(1);
    int nRet = 1;
    if (bNoMove == 0) {
        CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
        if (pArea != 0) {
            Vector vSafePos = { 0.0f, 0.0f, 0.0f };
            if (pArea->ComputeSafeLocation(m_position, 20.0f, m_pPathfindInfo, 1, &vSafePos)
                    != 0) {
                SetPosition(vSafePos, 1, 1, 0);
            } else {
                vSafePos = m_position;  // dead store
            }
        }
    }
    return nRet;
}
#else
int CSWSCreature::Disguise(int nAppearance, CGameEffect* pEffect, int bNoMove)
{
    K2_UNUSED(pEffect);
    CExoString sString;
    if (m_bDisguised != 0)
        return 0;
    m_nDisguiseAppearance = m_pStats->m_nAppearanceType;
    m_pStats->m_nAppearanceType = static_cast<unsigned short>(nAppearance);
    m_appearance.m_headId = static_cast<unsigned short>(nAppearance);
    UpdatePersonalSpace();
    UpdateAppearanceDependantInfo(1);
    m_bDisguised = 1;
    int nRet = 1;
    if (bNoMove == 0) {
        CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
        if (pArea != 0) {
            Vector vSafePos = { 0.0f, 0.0f, 0.0f };
            if (pArea->ComputeSafeLocation(m_position, 20.0f, m_pPathfindInfo, 1, &vSafePos)
                    != 0) {
                SetPosition(vSafePos, 1, 1, 0);
            } else {
                vSafePos = m_position;
            }
        }
    }
    return nRet;
}
#endif

int CSWSCreature::CancelAction(unsigned short nActionId, unsigned short nGroupId)
{
    K2_UNUSED(nActionId);
    // arg1 (nActionId) is unused; only the group id is matched.
    CExoLinkedListNode* pNode = m_actionList.m_pInternal->m_pHead;
    if (pNode == 0)
        return 0;
    CSWSObjectActionNode* pAction = static_cast<CSWSObjectActionNode*>(pNode->m_pData);
    if (pAction == 0)
        return 0;
    int nRemoved = 0;
    do {
        if (pAction->m_nGroupId == nGroupId) {
            CExoLinkedListNode* pCur = pNode;
            pAction = static_cast<CSWSObjectActionNode*>(m_actionList.GetNext(pNode));
            CSWSObjectActionNode* pCurAction =
                static_cast<CSWSObjectActionNode*>(m_actionList.GetAtPos(pCur));
            CExoLinkedListNode* pHead = m_actionList.m_pInternal->m_pHead;
            void* pHeadData = 0;
            if (pHead != 0)
                pHeadData = pHead->m_pData;
            if (ClearAction(pCurAction, pHeadData == pCurAction) == 1) {
                CSWSObjectActionNode* pRemoved =
                    static_cast<CSWSObjectActionNode*>(m_actionList.Remove(pCur));
                if (pRemoved != 0)
                    delete pRemoved;
                nRemoved = 1;
            }
        } else {
            pAction = static_cast<CSWSObjectActionNode*>(m_actionList.GetNext(pNode));
        }
    } while (pAction != 0);
    return nRemoved;
}

float CSWSCreature::MaxAttackRange(unsigned long long objectId, int a, int b)
{
#if KOTOR1  // K1: no skill-rank range bonus and no use-range arm
    float fRange;
    CSWItem* pItem = m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON);
    if (pItem == 0 || pItem->GetBaseItem()->m_bRangedWeapon == 0) {
        fRange = DesiredAttackRange(objectId, a) + 0.5f;
    } else if (b == 0) {
        fRange = pItem->GetBaseItem()->m_fMaxRange;
        if (fRange == 0.0f)
            fRange = 30.0f;
    } else {
        fRange = 20.0f;
    }
    return fRange;
#else
    float fRange = 0.0f;
    CSWItem* pItem = m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON);
    if (pItem != 0 && pItem->GetBaseItem()->m_bRangedWeapon != 0) {
        if (b != 0) {
            fRange = 20.0f;
        } else {
            fRange = pItem->GetBaseItem()->m_fMaxRange;
            fRange = (fRange == 0.0f) ? 30.0f : fRange;
            char nRank = m_pStats->GetSkillRank(SKILL_AWARENESS, 0, 0);
            if (nRank >= 1)
                fRange = fRange + static_cast<float>(nRank);
        }
    } else if (a != 0) {
        Vector vPos = { 0.0f, 0.0f, 0.0f };
        GetUseRange(objectId, vPos, fRange, 0, 0);
        fRange = fRange + 0.1f;
    } else {
        fRange = DesiredAttackRange(objectId, 0) + 0.5f;
    }
    return fRange;
#endif
}

void CSWSCreature::SetExcitedState(unsigned char nState)
{
    int nDuration;
    int nRet = g_pRules->m_p2DArrays->m_pExcitedDuration->GetINTEntry(
        nState, CExoString("DURATION"), &nDuration);
    if (nRet != 0 && static_cast<unsigned int>(nDuration) > static_cast<unsigned int>(m_nExcitedStateTimer)) {
        m_nExcitedStateTimer = nDuration;
        CSWSPlayer* pPlayer =
            g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
        CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
        if (pPlayer != 0 && pArea != 0
                && static_cast<CSWSArea*>(GetArea())->m_pAmbientSound != 0
                && GetNearestEnemy(30.0f, OBJECT_INVALID, OBJECT_INVALID) != OBJECT_INVALID) {
            static_cast<CSWSArea*>(GetArea())->m_pAmbientSound->PlayBattleMusic(1);
        }
    }
}

CSWCCreature* CSWSCreature::GetClientCreature()
{
    if (GetClientObject() == 0)
        return 0;
    CGameObject* pClientObj = GetClientObject();
    return static_cast<CSWCCreature*>(pClientObj->AsSWCCreature());
}

int CSWSCreature::GetRelativeWeaponSize(CSWSItem* pItem)
{
    int nSize = m_nCreatureSize;
    int nDiff = g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem)->m_nWeaponSize - nSize;
    if (static_cast<unsigned int>(nDiff + 2) > 3)
        return -10;
    return nDiff;
}

void CSWSCreature::SetId(unsigned long long id)
{
    unsigned long long oldId = m_objectId;
    AddToVisibleList(id, 1, 1, 0, 0);
    m_objectId = id;
    RemoveFromVisibleList(oldId);
}

void CSWSCreature::UpdateCombatRoundTimer()
{
    if (m_pCombatRound == 0)
        return;
    if (g_pAppManager->m_pServerExoApp->GetPauseState(2) != 0)
        return;
    if (m_pCombatRound->m_bRoundPaused == 0) {
        m_pCombatRound->IncrementTimer(m_nAIUpdateDeltaMs);
        return;
    }
    if (m_pCombatRound->m_bInfinitePause != 0) {
        m_pCombatRound->DecrementPauseTimer(m_nAIUpdateDeltaMs);
        return;
    }
    CSWSCombatAttackData* pAttack = m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
    int nRemaining = m_pCombatRound->m_nPauseTimer - m_nAIUpdateDeltaMs;
    if (nRemaining > 0) {
        m_pCombatRound->DecrementPauseTimer(m_nAIUpdateDeltaMs);
        if (pAttack->m_subAttacks.m_count > 0 &&
            pAttack->IsSubAttackReady(m_pCombatRound->m_nPauseTimer) != 0) {
            pAttack->ResolveTopSubAttackToAttack();
            SignalDamage(pAttack->m_bRangedAttack, pAttack->m_targetObjectId);
            pAttack->RemoveTopSubAttack();
        }
        return;
    }
    while (pAttack->m_subAttacks.m_count > 0) {
        pAttack->ResolveTopSubAttackToAttack();
        SignalDamage(pAttack->m_bRangedAttack, pAttack->m_targetObjectId);
        pAttack->RemoveTopSubAttack();
    }
    nRemaining = -nRemaining;
    ResetCombatAnimations(nRemaining);
    if (m_pCombatRound->m_bMaster != 0) {
        CSWSCreature* pOther = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
            m_pCombatRound->m_oidSlave);
        if (pOther != 0)
            pOther->ResetCombatAnimations(nRemaining);
    }
    CGameObject* pObject =
        g_pAppManager->m_pServerExoApp->GetGameObject(m_pCombatRound->m_oidAttackTarget);
    if (pObject == 0)
        return;
    if (pObject->AsSWSObject() == 0)
        return;
    CSWSObject* pTarget = pObject->AsSWSObject();
#if KOTOR1  // K1: a downed target is GetDeadTemp()
    if (pTarget->GetDead() != 0 ||
        (pTarget->AsSWSCreature() != 0 && pTarget->AsSWSCreature()->GetDeadTemp() != 0)) {
#else
    CSWSCreature* pTargetCreature;
    if (pTarget->GetDead() != 0 ||
        (pTarget->AsSWSCreature() != 0 &&
         (pTargetCreature = pTarget->AsSWSCreature())->m_bInParty != 0 &&
         pTargetCreature->GetCurrentHitPoints(0) <= 0)) {
#endif
        m_pCombatRound->EndCombatRound(1);
    }
}

int CSWSCreature::HasEffect(int nEffectType)
{
    for (int64_t i = 0; i < m_effects.m_count; i++) {
        if (m_effects.m_pArray[i]->m_subType == nEffectType)
            return 1;
    }
    return 0;
}

int CSWSCreature::GetCurrentlyFollowingLeader()
{
    CExoLinkedListNode* pHead = m_actionList.m_pInternal->m_pHead;
    if (pHead != 0) {
        CSWSObjectActionNode* pAction = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->GetAtPos(pHead));
        if (pAction != 0) {
            if (pAction->m_nActionId == AIACTION_PARTY_FOLLOW_LEADER)
                return 1;
        }
    }
    return 0;
}

void CSWSCreature::InstantUseItem(unsigned long long itemId)
{
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(itemId);
    for (int i = 0; i < pItem->m_nActiveProperties; i++) {
        CSWItemProperty* pProperty = pItem->GetActiveProperty(i);
        if (pProperty->m_type == ITEM_PROPERTY_ACTIVATE_ITEM) {
            SpellCastAndImpact(pProperty->m_subType, Vector{}, m_objectId, 0, itemId, 0, 0, 0, 0);
            pItem->DecrementUses(i, m_objectId);
            if (m_bCombatState != 0)
                m_nItemUseCooldown = 3000;
            break;
        }
    }
#if !KOTOR1
    if (m_bInParty != 0) {
        CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
        if (pPartyTable != 0 && (pPartyTable->m_forfeitConditions & FORFEIT_NO_ITEMS)) {
            pPartyTable->m_forfeitViolations = FORFEIT_NO_ITEMS;
            CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
            if (pArea != 0) {
                CScriptEvent* pEvent = new CScriptEvent();
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_USER_DEFINED_EVENT;
                pEvent->SetInteger(0, USER_EVENT_FORFEIT_VIOLATED);
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, pArea->m_objectId, pArea->m_objectId,
                    CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            }
        }
    }
#endif
}

void CSWSCreature::ForceEquipClothing()
{
    if (m_bForceEquipClothing == 0)
        return;
    CItemRepository* pRepo = GetItemRepository(1);
    unsigned long long oidItem;
    {
        CExoString sTag("g_a_clothes01");
        oidItem = pRepo->FindItemWithTag(sTag);
    }
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
    if (pItem == 0) {
        pItem = new CSWSItem(OBJECT_INVALID);
        CResRef resRef("g_a_clothes01");
        if (pItem->LoadFromTemplate(resRef) == 0) {
            delete pItem;
            m_bForceEquipClothing = 0;
            return;
        }
        pItem->m_flags |= 8;
        pItem->SetPossessor(m_objectId, 0, 0, OBJECT_INVALID);
    }
    if (pRepo->GetItemInRepository(pItem, 0) != 0)
        pRepo->RemoveItem(pItem);
    EquipItem(2, pItem, 1, 0, 0);
    m_bForceEquipClothing = 0;
}

void CSWSCreature::SetInvitedToParty(int bInvited)
{
    m_bInvitedToParty = bInvited;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&m_nInvitedToPartyDay, &m_nInvitedToPartyTime);
}

CSWSJournal* CSWSCreature::GetJournal()
{
    return g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyJournal();
}

void CSWSCreature::SetInTransit(int bInTransit)
{
    if (m_inTransit == bInTransit)
        return;
    m_inTransit = bInTransit;
    SendFeedbackMessage(bInTransit != 0 ? FEEDBACK_TRANSIT_STARTED : FEEDBACK_TRANSIT_ENDED, 0);
}

void CSWSCreature::RemoveSpellActionFromRound()
{
    if (m_pCombatRound->GetSpellActionPending() == 1)
        m_pCombatRound->RemoveSpellAction();
}

int CSWSCreature::DecrementSpellReadyCount(unsigned int nSpellId, unsigned char a, unsigned char b,
                                           unsigned char c, unsigned char d)
{
    K2_UNUSED(b); K2_UNUSED(c);
    if (m_bSpellChargeSpent == 0) {
        if (a == CSWSObject::CAST_CLASS_SPELL_LIKE_ABILITY) {
            m_pStats->SetSpellLikeAbilityReady(nSpellId, 0, d);
            return 1;
        }
        return m_pStats->UnReadySpell(nSpellId);
    }
    return 1;
}

int CSWSCreature::RangeWeaponEquipped()
{
    CSWSItem* pItem = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    if (pItem != 0) {
        int nBaseItem = pItem->m_baseItem;
        return g_pRules->m_pBaseItemArray->GetBaseItem(nBaseItem)->m_bRangedWeapon != 0;
    }
    return 0;
}

void CSWSCreature::RemovePureGoodPowers()
{
    for (int64_t i = 0; i < m_effects.m_count; i++) {
        if (m_effects.m_pArray[i]->m_subType == CSWSEffectListHandler::EFFECT_PUREGOODPOWERS) {
            RemoveEffectById(m_effects.m_pArray[i]->m_effectId);
            return;
        }
    }
}

void CSWSCreature::RemovePureEvilPowers()
{
    for (int64_t i = 0; i < m_effects.m_count; i++) {
        if (m_effects.m_pArray[i]->m_subType == CSWSEffectListHandler::EFFECT_PUREEVILPOWERS) {
            RemoveEffectById(m_effects.m_pArray[i]->m_effectId);
            return;
        }
    }
}

#if !KOTOR1
// Clamped at 999999999; while m_bInParty is set the party table's field is the live one.
void CSWSCreature::SetChemical(int nChemical)
{
    if (nChemical > 999999999)
        nChemical = 999999999;
    if (m_bInParty != 0)
        g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nItemChemical = nChemical;
    else
        m_nChemical = nChemical;
}
#endif

int CSWSCreature::GetDamageFlags()
{
    CSWSCombatAttackData* pAttack = m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
    int nAttackType = pAttack->m_nWeaponAttackType;
    CSWSItem* pWeapon = m_pCombatRound->GetCurrentAttackWeapon(nAttackType);
    if (pWeapon != 0)
        return pWeapon->GetDamageFlags();
    return CSWSEffectListHandler::DAMAGE_FLAG_BLUDGEONING;
}

#if !KOTOR1
void CSWSCreature::SetComponent(int nComponent)
{
    if (nComponent > 999999999)
        nComponent = 999999999;
    if (m_bInParty != 0)
        g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nItemComponent = nComponent;
    else
        m_nComponent = nComponent;
}
#endif


void CSWSCreature::BroadcastAttackBreakdown(unsigned long long objectId, CSWCCMessageData* pData)
{
    BroadcastBreakdownMessage(CC_MESSAGE_ATTACK_BREAKDOWN, pData);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objectId);
    if (pCreature != 0)
        pCreature->BroadcastBreakdownMessage(CC_MESSAGE_ATTACK_BREAKDOWN, pData);
}

void CSWSCreature::BroadcastThreatBreakdown(unsigned long long objectId, CSWCCMessageData* pData)
{
    BroadcastBreakdownMessage(CC_MESSAGE_THREAT_BREAKDOWN, pData);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objectId);
    if (pCreature != 0)
        pCreature->BroadcastBreakdownMessage(CC_MESSAGE_THREAT_BREAKDOWN, pData);
}

void CSWSCreature::BroadcastDefenseBreakdown(unsigned long long objectId, CSWCCMessageData* pData)
{
    BroadcastBreakdownMessage(CC_MESSAGE_DEFENSE_BREAKDOWN, pData);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objectId);
    if (pCreature != 0)
        pCreature->BroadcastBreakdownMessage(CC_MESSAGE_DEFENSE_BREAKDOWN, pData);
}

void CSWSCreature::BroadcastDamageBreakdown(unsigned long long objectId, CSWCCMessageData* pData)
{
    BroadcastBreakdownMessage(CC_MESSAGE_DAMAGE_BREAKDOWN, pData);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objectId);
    if (pCreature != 0)
        pCreature->BroadcastBreakdownMessage(CC_MESSAGE_DAMAGE_BREAKDOWN, pData);
}

void CSWSCreature::BroadcastCriticalHitBreakdown(unsigned long long objectId,
                                                 CSWCCMessageData* pData)
{
    BroadcastBreakdownMessage(CC_MESSAGE_CRITICAL_HIT_BREAKDOWN, pData);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objectId);
    if (pCreature != 0)
        pCreature->BroadcastBreakdownMessage(CC_MESSAGE_CRITICAL_HIT_BREAKDOWN, pData);
}

void CSWSCreature::BroadcastEffectApplicationBreakdown(unsigned long long objectId,
                                                       CSWCCMessageData* pData)
{
    BroadcastBreakdownMessage(CC_MESSAGE_EFFECT_BREAKDOWN, pData);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objectId);
    if (pCreature != 0)
        pCreature->BroadcastBreakdownMessage(CC_MESSAGE_EFFECT_BREAKDOWN, pData);
}

void CSWSCreature::BroadcastDeflectBreakdown(unsigned long long objectId, CSWCCMessageData* pData)
{
    BroadcastBreakdownMessage(CC_MESSAGE_DEFLECT_BREAKDOWN, pData);
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objectId);
    if (pCreature != 0)
        pCreature->BroadcastBreakdownMessage(CC_MESSAGE_DEFLECT_BREAKDOWN, pData);
}

int CSWSCreature::GetPauseReadyAnimation()
{
    CSWCCreature* pClientCreature = GetClientCreature();
    if (pClientCreature != 0)
        return pClientCreature->GetPauseReadyAnimation();
    return m_bCombatState != 0 ? SERVER_ANIMATION_READY : SERVER_ANIMATION_PAUSE;
}

void CSWSCreature::CutWaypointPath()
{
    m_pPathfindInfo->m_bPathCut = 1;
    m_pPathfindInfo->m_nPathCutCount = m_pPathfindInfo->m_nPathCutCount + 1;
    m_pPathfindInfo->m_nReserved11 = 0;
    m_pPathfindInfo->m_nPathCutWaypointsLeft = (m_pPathfindInfo->m_nWayPointFloats - m_pPathfindInfo->m_nWayPointCursor) >> 1;
    m_pPathfindInfo->m_vPathCutPosition = m_position;
    m_pPathfindInfo->m_vPathCutOrientation = m_orientation;
}

void CSWSCreature::SetPortrait(const CResRef& r)
{
    if (m_pStats != 0) {
        m_pStats->SetPortrait(r);
    }
}

int CSWSCreature::EvaluateLock(CSWSObject* pObject, int bNewLockState)
{
    CServerAIMaster* pAIMaster = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
    if (pObject->AsSWSDoor() != 0) {
        CSWSDoor* pDoor = pObject->AsSWSDoor();
        if (pDoor->m_bLocked == bNewLockState)
            return 1;
#if KOTOR1  // K1: the door key tag is matched as stored (no LowerCase)
        if (pDoor->m_keyName.IsEmpty())
            return 0;
        unsigned long long oidKey = GetItemRepository(1)->FindItemWithTag(pDoor->m_keyName);
#else
        if (pDoor->m_keyName.m_sString == 0 || *pDoor->m_keyName.m_sString == 0)
            return 0;
        unsigned long long oidKey =
            GetItemRepository(1)->FindItemWithTag(pDoor->m_keyName.LowerCase());
#endif
        CSWSItem* pKey = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidKey);
        if (pKey != 0) {
            if (static_cast<unsigned char>(pDoor->m_bAutoRemoveKey))
                pAIMaster->AddEventDeltaTime(0, 0, m_objectId, pKey->m_objectId,
                                             CServerAIMaster::EVENT_DECREMENT_STACKSIZE, 0);
            pDoor->m_bLocked = bNewLockState;
            SendFeedbackMessage(FEEDBACK_USED_KEY, 0);
            return 1;
        }
#if KOTOR1  // K1: 18 equipment slots
        for (int nSlot = 0; nSlot < 18; nSlot++) {
#else
        for (int nSlot = 0; nSlot < 20; nSlot++) {
#endif
            CSWSItem* pSlotKey = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(1 << nSlot));
            if (pSlotKey != 0 && pSlotKey->m_sTag == pDoor->m_keyName) {
                if (static_cast<unsigned char>(pDoor->m_bAutoRemoveKey))
                    pAIMaster->AddEventDeltaTime(0, 0, m_objectId, pSlotKey->m_objectId,
                                                 CServerAIMaster::EVENT_DECREMENT_STACKSIZE, 0);
                pDoor->m_bLocked = bNewLockState;
                SendFeedbackMessage(FEEDBACK_USED_KEY, 0);
                return 1;
            }
        }
        return 0;
    }
    if (pObject->AsSWSPlaceable() != 0) {
        CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
        if (pPlaceable->m_bLocked == bNewLockState)
            return 1;
#if KOTOR1
        if (pPlaceable->m_keyName.IsEmpty())
            return 0;
#else
        if (pPlaceable->m_keyName.m_sString == 0 || *pPlaceable->m_keyName.m_sString == 0)
            return 0;
#endif
        unsigned long long oidKey =
            GetItemRepository(1)->FindItemWithTag(pPlaceable->m_keyName);
        CSWSItem* pKey = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidKey);
        if (pKey != 0) {
            if (pPlaceable->m_bAutoRemoveKey)
                pAIMaster->AddEventDeltaTime(0, 0, m_objectId, pKey->m_objectId,
                                             CServerAIMaster::EVENT_DECREMENT_STACKSIZE, 0);
            pPlaceable->m_bLocked = bNewLockState;
            SendFeedbackMessage(FEEDBACK_USED_KEY, 0);
            return 1;
        }
#if KOTOR1  // K1: 18 equipment slots
        for (int nSlot = 0; nSlot < 18; nSlot++) {
#else
        for (int nSlot = 0; nSlot < 20; nSlot++) {
#endif
            CSWSItem* pSlotKey = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(1 << nSlot));
            if (pSlotKey != 0 && pSlotKey->m_sTag == pPlaceable->m_keyName) {
                if (pPlaceable->m_bAutoRemoveKey)
                    pAIMaster->AddEventDeltaTime(0, 0, m_objectId, pSlotKey->m_objectId,
                                                 CServerAIMaster::EVENT_DECREMENT_STACKSIZE, 0);
                pPlaceable->m_bLocked = bNewLockState;
                SendFeedbackMessage(FEEDBACK_USED_KEY, 0);
                return 1;
            }
        }
        return 0;
    }
    return 0;
}

void CSWSCreature::SetAnimation(int nAnimation)
{
#if KOTOR1  // K1: 13 ambient states
    static const int s_ambientAnimationMap[13] = {
        10058, 10008, 10006, 10150, 10117, 10124, 10118, 10400,
        10401, 10137, 10138, 10402, 10156
    };
#else
    static const int s_ambientAnimationMap[16] = {
        10058, 10008, 10006, 10150, 10117, 10124, 10118, 10400,
        10401, 10137, 10138, 10402, 10156, 10139, 10416, 10418
    };
#endif
    int nNewAnimation;
    if ((nAnimation | 1) == SERVER_ANIMATION_READY) {
        unsigned char nIndex = static_cast<unsigned char>(m_ambientAnimationState) - 2;
#if KOTOR1
        if (nIndex < 13) {
#else
        if (nIndex < 16) {
#endif
            nNewAnimation = s_ambientAnimationMap[static_cast<signed char>(nIndex)];
        } else {
            nNewAnimation = SERVER_ANIMATION_PAUSE;
            if (m_bCombatState != 0)
                nNewAnimation = SERVER_ANIMATION_READY;
        }
    } else {
        nNewAnimation = SERVER_ANIMATION_PAUSE;
        if (nAnimation != -1)
            nNewAnimation = nAnimation;
    }

    int bRemap = g_pAppManager->m_pServerExoApp->IsInjured(m_objectId) != 0;
#if KOTOR1  // K1: the options are read first and the leader is fetched per use
    if (bRemap != 0) {
        if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK &&
            (g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0) == 0 ||
             g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId) ==
                 g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0)->m_objectId))
            bRemap = 0;
    }
#else
    if (bRemap != 0) {
        CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
        CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
        if (pOptions->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK &&
            (pLeader == 0 ||
             g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId) ==
                 pLeader->m_objectId))
            bRemap = 0;
    }
#endif
    if (bRemap != 0) {
        if (nNewAnimation == SERVER_ANIMATION_WALK)
            nNewAnimation = SERVER_ANIMATION_WALK_INJURED;
        else if (nNewAnimation == SERVER_ANIMATION_RUN)
            nNewAnimation = SERVER_ANIMATION_RUN_INJURED;
    } else {
        if ((nNewAnimation == SERVER_ANIMATION_RUN || nNewAnimation == SERVER_ANIMATION_WALK) &&
            (static_cast<unsigned char>(m_activityFlags) & 1) != 0) {
#if KOTOR1  // K1: no feat exemption
            nNewAnimation = SERVER_ANIMATION_STEALTH;
#else
            if (m_pStats->HasFeat(FEAT_STEALTH_RUN) == 0)
                nNewAnimation = SERVER_ANIMATION_STEALTH;
#endif
        }
    }

#if KOTOR1
    if ((GetDead() != 0 || GetDeadTemp() != 0) &&
#else
    if ((GetDead() != 0 || (m_bInParty != 0 && GetCurrentHitPoints(0) <= 0)) &&
#endif
        nNewAnimation != SERVER_ANIMATION_DEAD && nNewAnimation != SERVER_ANIMATION_DEAD_PRONE && nNewAnimation != SERVER_ANIMATION_DEAD_2)
        return;

    int bFired = 0;
    if (GetClientObject() != 0) {
        CSWCObject* pClient = static_cast<CSWCObject*>(GetClientObject()->AsSWCCreature());
        if (pClient != 0)
            bFired = pClient->AnimationFireAndForget(static_cast<unsigned short>(nNewAnimation));
    }
    if (bFired == 0 || m_nAnimation != nNewAnimation)
        SetDirtyFlag(0);
    m_nAnimation = nNewAnimation;
}

int CSWSCreature::AddUseTalentOnObjectActions(int nTalentType, int nTalentId,
                                              unsigned long long oidTarget,
                                              unsigned char a,
                                              unsigned long long oidItem,
                                              int nItemProp,
                                              unsigned char b, unsigned char c)
{
    CSWSCreature* pTarget =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
    if (pTarget == 0)
        return 0;
    if (m_bCommandable == 0)
        return 0;
    if (GetArea() == 0)
        return 0;

    Vector vTargetPos = pTarget->m_position;
    GetArea();  // result discarded

    switch (nTalentType) {
    case TALENT_TYPE_FORCE:
        if (oidItem == OBJECT_INVALID || nItemProp == -1) {
            unsigned long long oidUseItem;
            int nUseItemProp;
            if (m_pStats->GetItemDetailsForPower(nTalentId, oidUseItem,
                                                 nUseItemProp) != 1) {
                AddCastSpellActions(nTalentId, a, 0, c, 0, vTargetPos,
                                    oidTarget, 0, 0, 0, 0, 0, 0, -1, b, 0);
                break;
            }
            UseItem(oidUseItem, static_cast<unsigned char>(nUseItemProp), 0, oidTarget,
                    vTargetPos, 0, 0);
        } else {
            UseItem(oidItem, static_cast<unsigned char>(nItemProp), 0, oidTarget,
                    vTargetPos, 0, 0);
        }
        break;
    case TALENT_TYPE_FEAT:
        AddAttackActions(oidTarget, static_cast<unsigned short>(nTalentId), 0, 0, 1, 0, 0,
                         1, SERVER_ANIMATION_ATTACK, 1500, 1, 0, 0, 4, 0);
        break;
    case TALENT_TYPE_SKILL:
        UseSkill(static_cast<unsigned char>(nTalentId), 0, oidTarget, vTargetPos,
                 OBJECT_INVALID, OBJECT_INVALID, 0);
        break;
    }
    return 1;
}

// The location-targeted twin of AddUseTalentOnObjectActions. Only talent type 0 (a spell/item
// power) has a location form, so every other type falls straight through to the `return 1`.
int CSWSCreature::AddUseTalentAtLocationActions(int nTalentType, int nTalentId,
                                                const Vector& vTarget, unsigned char a,
                                                unsigned long long oidItem, int nItemProp,
                                                unsigned char b, unsigned char c)
{
    if (m_bCommandable == 0)
        return 0;
    if (GetArea() == 0)
        return 0;
    if (static_cast<CSWSArea*>(GetArea())->m_objectId == OBJECT_INVALID)
        return 0;
    if (nTalentType != TALENT_TYPE_FORCE)
        return 1;

    if (oidItem == OBJECT_INVALID || nItemProp == -1) {
        unsigned long long oidUseItem;
        int nUseItemProp;
        if (m_pStats->GetItemDetailsForPower(nTalentId, oidUseItem, nUseItemProp) != 1) {
            AddCastSpellActions(nTalentId, a, 0, c, 0, vTarget, OBJECT_INVALID, 0, 1, 0, 0, 0, 0,
                                -1, b, 0);
            return 1;
        }
        ClearAllActions(1);
        UseItem(oidUseItem, static_cast<unsigned char>(nUseItemProp), 0, OBJECT_INVALID, vTarget,
                static_cast<CSWSArea*>(GetArea())->m_objectId, 0);
    } else {
        ClearAllActions(1);
        UseItem(oidItem, static_cast<unsigned char>(nItemProp), 0, OBJECT_INVALID, vTarget,
                static_cast<CSWSArea*>(GetArea())->m_objectId, 0);
    }
    return 1;
}

int CSWSCreature::ForceMoveToPoint(CSWSForcedAction* pAction)
{
    CSWSObjectActionNode* pNode = new CSWSObjectActionNode();
    int nOne = 1;
    float fOne = 1.0f;
    int nZero = 0;
    pNode->m_bInterruptable = 1;
    if (pAction->m_nTargetObject == OBJECT_INVALID) {
        pNode->m_nActionId = AIACTION_JUMP_TO_POINT;
        pNode->m_nGroupId = pAction->m_nGroupId;
        unsigned long long nObjectId = pAction->m_nTargetArea;
        Vector vPosition = pAction->m_vPoint;
        AddActionNodeParameter(pNode, 0, ACTION_PARAM_FLOAT, &vPosition.x);
        AddActionNodeParameter(pNode, 1, ACTION_PARAM_FLOAT, &vPosition.y);
        AddActionNodeParameter(pNode, 2, ACTION_PARAM_FLOAT, &vPosition.z);
        AddActionNodeParameter(pNode, 3, ACTION_PARAM_OBJECT, &nObjectId);
        AddActionNodeParameter(pNode, 4, ACTION_PARAM_INT, &nOne);
        AddActionNodeParameter(pNode, 5, ACTION_PARAM_FLOAT, &fOne);
        AddActionNodeParameter(pNode, 6, ACTION_PARAM_FLOAT, &nZero);
        AIActionJumpToPoint(pNode);
    } else {
        pNode->m_nActionId = AIACTION_JUMP_TO_OBJECT;
        pNode->m_nGroupId = pAction->m_nGroupId;
        unsigned long long nTargetId = pAction->m_nTargetObject;
        AddActionNodeParameter(pNode, 0, ACTION_PARAM_OBJECT, &nTargetId);
        AddActionNodeParameter(pNode, 1, ACTION_PARAM_INT, &nOne);
        AIActionJumpToObject(pNode);
    }
    delete pNode;
    return 1;
}

#if KOTOR1
float CSWSCreature::GetMovementRateFactor()
{
    float fFactor = m_movementRateFactor;
    float v = 0.125f;
    if (!(fFactor < 0.125f)) {
        v = fFactor;
        if (fFactor > 1.5f)
            v = 1.5f;
    }
    return v;
}
#else
float CSWSCreature::GetMovementRateFactor(int a)
{
    float fFactor = m_movementRateFactor;
    if (a != 0 && m_pStats->HasFeat(FEAT_MOBILITY) != 0)
        fFactor = fFactor + 0.1f;
    float v = 0.125f;
    if (!(fFactor < 0.125f)) {
        v = fFactor;
        if (fFactor > 1.5f)
            v = 1.5f;
    }
    return v;
}
#endif

int CSWSCreature::RemovePazaakCard(CSWSItem* pItem)
{
    unsigned char nCard = pItem->m_nModelVariation;
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    int nCount = pPartyTable->m_pazaakCards[nCard - 1];
    int nRemove = pItem->m_stackCount;
    if (nCount < nRemove)
        nRemove = nCount;
    pPartyTable->m_pazaakCards[nCard - 1] = nCount - nRemove;
    return nRemove;
}

void CSWSCreature::ComputeArmourClass(CSWSItem* pItem, int a, int b)
{
    K2_UNUSED(pItem); K2_UNUSED(a); K2_UNUSED(b);
    unsigned char nArmourClass = 0;
    CSWSItem* pArmour = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_BODY));
    if (pArmour != 0) {
        CSWBaseItem* pBaseItem = pArmour->GetBaseItem();
        if (pBaseItem != 0)
            nArmourClass = pBaseItem->m_nBaseAC;
    }
    m_pStats->m_nACArmorBase = nArmourClass;
}

int CSWSCreature::EquipItem(unsigned int nSlot, CSWSItem* pItem, int a, int b, int c)
{
    if (m_pInventory->GetItemInInventory(pItem) != 0)
        UnequipItem(pItem);

#if KOTOR1
    if (a != 0)
        pItem->ApplyItemProperties(this, nSlot, b);
#else
    switch (nSlot) {
        case CSWInventory::SLOT_RIGHTWEAPON2:
        case CSWInventory::SLOT_LEFTWEAPON2:
            break;
        default:
            if (a != 0)
                pItem->ApplyItemProperties(this, nSlot, b);
            break;
    }
#endif

    m_pInventory->PutItemInSlot(nSlot, pItem);
    ComputeArmourClass(pItem, 1, 0);
    m_pStats->UpdateCombatInformation();

    if ((g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState | c | b) == 0)
    {
        if (nSlot == CSWInventory::SLOT_RIGHTWEAPON || nSlot == CSWInventory::SLOT_LEFTWEAPON)
#if KOTOR1
            SetCombatState(1, 2);
#else
            SetCombatState(1, 2, 0);
#endif
    }

#if !KOTOR1
    if (nSlot != CSWInventory::SLOT_RIGHTWEAPON2 && nSlot != CSWInventory::SLOT_LEFTWEAPON2)
#endif
    {
        m_nEquippedWeight = m_nEquippedWeight + pItem->GetWeight();

        CScriptEvent* pEvent = new CScriptEvent;
        CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
        pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_EQUIP_ITEM;
        pEvent->SetObjectID(0, pItem->m_objectId);
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
            0, 0, m_objectId, pModule->m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
    }

#if !KOTOR1
    if (m_bInParty != 0
        && (nSlot == CSWInventory::SLOT_BODY || nSlot == CSWInventory::SLOT_RIGHTWEAPON || nSlot == CSWInventory::SLOT_LEFTWEAPON))
    {
        CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
        if (pPartyTable != 0)
        {
            int nViolation;
            switch (nSlot)
            {
            case CSWInventory::SLOT_RIGHTWEAPON:
            case CSWInventory::SLOT_LEFTWEAPON:
                if (pPartyTable->m_forfeitConditions & FORFEIT_NO_WEAPONS)
                    nViolation = FORFEIT_NO_WEAPONS;
                else if ((pPartyTable->m_forfeitConditions & FORFEIT_DXUN_SWORD_ONLY)
                         && pItem->m_sTag.CompareNoCase("pl_sword") == 0)
                    nViolation = FORFEIT_DXUN_SWORD_ONLY;
                else if ((pPartyTable->m_forfeitConditions & FORFEIT_NO_RANGED)
                         && static_cast<CSWItem*>(pItem)->GetBaseItem()->m_bRangedWeapon != 0)
                    nViolation = FORFEIT_NO_RANGED;
                else if ((pPartyTable->m_forfeitConditions & FORFEIT_NO_LIGHTSABER)
                         && pItem->IsLightsaber() != 0)
                    nViolation = FORFEIT_NO_LIGHTSABER;
                else
                    goto disguise;
                break;
            case CSWInventory::SLOT_BODY:
                if ((pPartyTable->m_forfeitConditions & FORFEIT_NO_ARMOR) == 0)
                    goto disguise;
                nViolation = FORFEIT_NO_ARMOR;
                break;
            default:
                goto disguise;
            }

            pPartyTable->m_forfeitViolations = nViolation;

            // The area's own object id is both the event's target and its subject here.
            CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
            if (pArea != 0)
            {
                CScriptEvent* pEvent = new CScriptEvent;
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_USER_DEFINED_EVENT;
                pEvent->SetInteger(0, USER_EVENT_FORFEIT_VIOLATED);
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, pArea->m_objectId, pArea->m_objectId,
                    CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            }
        }
    }

disguise:
    if (nSlot == CSWInventory::SLOT_BODY)
    {
        unsigned char nModelPart = pItem->m_nBodyVariation;
        short nAlignment = m_pStats->m_nAlignment;
        if (m_sTag.CompareNoCase("handmaiden") != 0)
        {
            if (nModelPart == 2)
            {
                if (nAlignment <= 40)
                {
                    if (m_pStats->m_nAppearanceType != APPEARANCE_HANDMAIDEN_CLOTHING_EVIL)
                        Disguise(APPEARANCE_HANDMAIDEN_CLOTHING_EVIL, 0, 0, 1);
                }
                else
                {
                    if (m_pStats->m_nAppearanceType != APPEARANCE_HANDMAIDEN)
                        Disguise(APPEARANCE_HANDMAIDEN, 0, 0, 1);
                }
                return 1;
            }
            if (m_pStats->m_nAppearanceType != APPEARANCE_HANDMAIDEN_HEAD)
                Disguise(APPEARANCE_HANDMAIDEN_HEAD, 0, 0, 1);
        }

        char szColumn[7] = "ModelA";
        int nLetter = nModelPart;
        if (nLetter > 12)
            nLetter = 12;
        szColumn[5] = nLetter < 1 ? 'A' : static_cast<char>(nLetter + ('A' - 1));

        CExoString sEntry;
        if (g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
                m_pStats->m_nAppearanceType, szColumn, &sEntry) == 0)
            sEntry = "";

        if (sEntry[0] == '&')
        {
            sEntry = sEntry.SubString(1, -1);
            int nAppearance = atoi(sEntry.CStr());
            if (nAppearance != 0 && nAppearance != m_pStats->m_nAppearanceType)
                Disguise(nAppearance, 0, 0, 1);
        }
    }
#endif
    return 1;
}

__attribute__((no_stack_protector)) int CSWSCreature::CalculateDamagePower(CSWSObject* pObject)
{
    int nBonus = 0;
#if KOTOR1
    return GetTotalEffectBonus(EFFECT_BONUS_ATTACK, pObject, nBonus, 0, 0, 0, 0, EFFECT_BONUS_ANY,
                               EFFECT_BONUS_ANY, 0);
#else
    return GetTotalEffectBonus(EFFECT_BONUS_ATTACK, pObject, nBonus, 0, 0, 0, 0, EFFECT_BONUS_ANY,
                               EFFECT_BONUS_ANY, 0, 0);
#endif
}

#if !KOTOR1
static inline int WoundedSaveBonusBand(float fPercent)
{
    if (fPercent < 50.0f && fPercent >= 40.0f)
        return 1;
    if (fPercent < 40.0f && fPercent >= 30.0f)
        return 2;
    if (fPercent < 30.0f && fPercent >= 20.0f)
        return 3;
    if (fPercent < 20.0f && fPercent >= 10.0f)
        return 4;
    if (fPercent < 10.0f && fPercent >= 0.0f)
        return 5;
    return 0;
}

static inline __attribute__((always_inline)) void StackEffectValue(CGameEffect* pEffect, int nSlot,
                                                                   unsigned long long* paItem,
                                                                   int* paValue, int* paSpell)
{
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pEffect->m_creatorId);
    int bFound = 0;
    if (pItem != 0) {
        for (int j = 0; j < 108; j++) {
            if (paItem[j] == pItem->m_objectId) {
                bFound = 1;
                if (paValue[j] < pEffect->GetInteger(nSlot)) {
                    paValue[j] = pEffect->GetInteger(nSlot);
                    return;
                }
            }
        }
        if (bFound == 0) {
            for (int j = 0; j < 108; j++) {
                if (paValue[j] == -1) {
                    paItem[j] = pItem->m_objectId;
                    paValue[j] = pEffect->GetInteger(nSlot);
                    return;
                }
            }
        }
        return;
    }
    if (pEffect->m_spellId != -1) {
        for (int j = 0; j < 108; j++) {
            if (paSpell[j] == pEffect->m_spellId) {
                bFound = 1;
                if (paValue[j] < pEffect->GetInteger(nSlot)) {
                    paValue[j] = pEffect->GetInteger(nSlot);
                    return;
                }
            }
        }
    }
    if (bFound == 0) {
        for (int j = 0; j < 108; j++) {
            if (paValue[j] == -1) {
                paSpell[j] = pEffect->m_spellId;
                paValue[j] = pEffect->GetInteger(nSlot);
                return;
            }
        }
    }
}

static inline __attribute__((always_inline)) void StackEffectValueSum(CGameEffect* pEffect, int nSlot,
                                                                      unsigned long long* paItem,
                                                                      int* paValue, int* paSpell)
{
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pEffect->m_creatorId);
    int bFound = 0;
    if (pItem != 0) {
        for (int j = 0; j < 108; j++) {
            if (paItem[j] == pItem->m_objectId) {
                bFound = 1;
                paValue[j] += pEffect->GetInteger(nSlot);
            }
        }
        if (bFound == 0) {
            for (int j = 0; j < 108; j++) {
                if (paValue[j] == -1) {
                    paItem[j] = pItem->m_objectId;
                    paValue[j] = pEffect->GetInteger(nSlot);
                    return;
                }
            }
        }
        return;
    }
    if (pEffect->m_spellId != -1) {
        for (int j = 0; j < 108; j++) {
            if (paSpell[j] == pEffect->m_spellId) {
                bFound = 1;
                paValue[j] += pEffect->GetInteger(nSlot);
            }
        }
    }
    if (bFound == 0) {
        for (int j = 0; j < 108; j++) {
            if (paValue[j] == -1) {
                paSpell[j] = pEffect->m_spellId;
                paValue[j] = pEffect->GetInteger(nSlot);
                return;
            }
        }
    }
}

// Sums every active effect of one kind: nEffectType selects one of five families off a jump table,
// and each family walks m_effects from the per-family first index CSWSCreatureStats caches, stopping
// when m_subType steps past that family's decrease id (the list is sorted by subtype).
//
// Stacking rules, which is why the class carries six 108-entry scratch arrays:
// - "misc" bonuses do not accumulate. Two that share a key overwrite, so the same stim drunk twice
//   does not double its effect. Ability-score penalties are the exception and do add, see
//   StackEffectValueSum.
// - The key is the spell id for attack and damage, and the creator item's object id for
//   ability/skill/saving throw, so an item's bonus stacks with a spell's but not with itself.
// - A -1 spell id means "not from a spell" and skips the search, since the key arrays are seeded
//   to -1 and every empty slot would otherwise match.
//
// Every family returns min(increases, cap) - min(decreases, cap), with caps that are per family and
// not symmetric: attack and saving throw 60, skill 90, damage 108, ability 60 up against 90 down.
int CSWSCreature::GetTotalEffectBonus(unsigned char nEffectType, CSWSObject* pVersus, int& nOut,
                                      int bElemental, int bMaxDamage, unsigned char nSaveType,
                                      unsigned char nSaveVs, unsigned char nSkill,
                                      unsigned char nAbility, int bOffHand, int* pOut)
{
    nOut = 0;

    // Which attack this bonus is being asked about. The combat round's answer wins unless the
    // caller is running a hypothetical (bMaxDamage), in which case it is derived from the hands:
    // main hand holding a weapon ONHAND, main hand empty UNARMED, off hand OFFHAND.
    CSWSCombatAttackData* pAttack = m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
    int nAttackType = CSWSCombatRound::WEAPON_ATTACK_TYPE_NONE;
    if (bMaxDamage == 0)
        nAttackType = pAttack->m_nWeaponAttackType;
    if (nAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_NONE) {
        if (bOffHand == 0 && m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON) != 0)
            nAttackType = CSWSCombatRound::WEAPON_ATTACK_TYPE_ONHAND;
        else
            nAttackType = (bOffHand == 0) ? CSWSCombatRound::WEAPON_ATTACK_TYPE_UNARMED
                                          : CSWSCombatRound::WEAPON_ATTACK_TYPE_OFFHAND;
    }

    // The target's racial type and alignment band, which effects may be restricted to. The bands
    // are the alignment scale's thirds: below 41 dark, above 59 light, between them neutral. -1 in
    // either means "no creature target", which matches nothing.
    int nAlignmentGroup = -1;
    int nTargetRace = -1;
    if (pVersus != 0 && pVersus->AsSWSCreature() != 0) {
        nTargetRace = pVersus->AsSWSCreature()->m_pStats->m_nRace;
        short nAlignment = pVersus->AsSWSCreature()->m_pStats->m_nAlignment;
        nAlignmentGroup = (nAlignment < 41) ? ALIGNMENT_DARK_SIDE
                          : ((nAlignment > 59) ? ALIGNMENT_LIGHT_SIDE : ALIGNMENT_NEUTRAL);
    }

    for (int i = 0; i < 108; i++) {
        m_aStackIncValue[i] = -1;
        m_aStackDecValue[i] = -1;
        m_aStackIncSpell[i] = -1;
        m_aStackDecSpell[i] = -1;
        m_aStackIncItem[i] = OBJECT_INVALID;
        m_aStackDecItem[i] = OBJECT_INVALID;
    }

    switch (nEffectType) {
    case EFFECT_BONUS_ATTACK: {
        // Attack bonus. Integer slots: 0 magnitude, 1 which attack the bonus applies to,
        // 2 racial restriction, 4 alignment restriction.
        int nSumInc = 0;
        int nSumDec = 0;
        if (m_effects.m_count > 0 && m_pStats->m_iFirstAttackIncrease < m_effects.m_count) {
            int nAttackDecrease = CSWSEffectListHandler::EFFECT_ATTACK_DECREASE;
            int nBonusMisc = CSWSEffectListHandler::ATTACK_BONUS_MISC;
            int nBonusOnHand = CSWSEffectListHandler::ATTACK_BONUS_ONHAND;
            int nBonusCWeapon1 = CSWSEffectListHandler::ATTACK_BONUS_CWEAPON1;
            int nBonusUnarmed = CSWSEffectListHandler::ATTACK_BONUS_UNARMED;
            int nAttackIncrease = CSWSEffectListHandler::EFFECT_ATTACK_INCREASE;
            for (int i = m_pStats->m_iFirstAttackIncrease; i < m_effects.m_count; i++) {
                CGameEffect* pEffect = m_effects.m_pArray[i];
                if (pEffect->m_subType > nAttackDecrease)
                    break;
                int nBonusType = pEffect->GetInteger(1);
                int nRace = pEffect->GetInteger(2);
                int nAlign = pEffect->GetInteger(4);
                int bApplies = (nBonusType == nBonusMisc || nBonusType == nAttackType);
                if (nAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_EXTRA &&
                    (nBonusType == nBonusOnHand || nBonusType == nBonusCWeapon1))
                    bApplies = 1;
                if (nAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_UNARMED_EXTRA &&
                    nBonusType == nBonusUnarmed)
                    bApplies = 1;
                if ((nRace == g_pRules->m_nNumRaces || nRace == nTargetRace) &&
                    (nAlign == ALIGNMENT_ALL || nAlign == nAlignmentGroup) && bApplies == 1) {
                    if (pEffect->m_subType == nAttackIncrease) {
                        if (nBonusType != nBonusMisc) {
                            nSumInc += pEffect->GetInteger(0);
                        } else {
                            int bFound = 0;
                            if (pEffect->m_spellId != -1) {
                                for (int j = 0; j < 108; j++) {
                                    if (m_aStackIncSpell[j] == pEffect->m_spellId) {
                                        bFound = 1;
                                        if (m_aStackIncValue[j] < pEffect->GetInteger(0)) {
                                            m_aStackIncValue[j] = pEffect->GetInteger(0);
                                            goto nextAttackEffect;
                                        }
                                    }
                                }
                            }
                            if (bFound == 0) {
                                for (int j = 0; j < 108; j++) {
                                    if (m_aStackIncValue[j] == -1) {
                                        m_aStackIncSpell[j] = pEffect->m_spellId;
                                        m_aStackIncValue[j] = pEffect->GetInteger(0);
                                        break;
                                    }
                                }
                            }
                        }
                    } else if (pEffect->m_subType == nAttackDecrease) {
                        if (nBonusType != nBonusMisc) {
                            nSumDec += pEffect->GetInteger(0);
                        } else {
                            int bFound = 0;
                            if (pEffect->m_spellId != -1) {
                                for (int j = 0; j < 108; j++) {
                                    if (m_aStackDecSpell[j] == pEffect->m_spellId) {
                                        bFound = 1;
                                        if (m_aStackDecValue[j] < pEffect->GetInteger(0)) {
                                            m_aStackDecValue[j] = pEffect->GetInteger(0);
                                            goto nextAttackEffect;
                                        }
                                    }
                                }
                            }
                            if (bFound == 0) {
                                for (int j = 0; j < 108; j++) {
                                    if (m_aStackDecValue[j] == -1) {
                                        m_aStackDecSpell[j] = pEffect->m_spellId;
                                        m_aStackDecValue[j] = pEffect->GetInteger(0);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            nextAttackEffect:;
            }
        }
        for (int j = 0; j < 108; j++) {
            if (m_aStackIncValue[j] != -1)
                nSumInc += m_aStackIncValue[j];
            if (m_aStackDecValue[j] != -1)
                nSumDec += m_aStackDecValue[j];
        }
        if (nSumInc > 60)
            nSumInc = 60;
        if (nSumDec > 60)
            nSumDec = 60;
        return nSumInc - nSumDec;
    }

    case EFFECT_BONUS_DAMAGE: {
        // Damage bonus is the widest arm, because a damage effect carries a damage type and each
        // type is applied separately. Integer slots: 0 magnitude (or, at 6 and above, a row in
        // iprp_damagecost.2da holding NumDice/Die/Rank), 1 the damage-type bit mask, 2 racial
        // restriction, 4 alignment restriction, 5 which attack the bonus applies to. Two effects
        // stack here only when they share both a spell id and a damage type.
        int nIncomingRank = 0;
        int nStoredRank = 0;
        int nNumDice = 0;
        int nDie = 0;
        int anDecrease[15] = {};
        int anDamage[15] = {};
        int anDecType[108] = {};
        int anIncType[108] = {};
        C2DA* p2DA = g_pRules->m_p2DArrays->m_pIPRPDamage;
        if (m_effects.m_count > 0 && m_pStats->m_iFirstDamageIncrease < m_effects.m_count) {
            int nDamageDecrease = CSWSEffectListHandler::EFFECT_DAMAGE_DECREASE;
            int nBonusMisc = CSWSEffectListHandler::DAMAGE_BONUS_MISC;
            int nBonusOnHand = CSWSEffectListHandler::DAMAGE_BONUS_ONHAND;
            int nBonusCWeapon1 = CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON1;
            int nBonusUnarmed = CSWSEffectListHandler::DAMAGE_BONUS_UNARMED;
            int nDamageIncrease = CSWSEffectListHandler::EFFECT_DAMAGE_INCREASE;
            // This is not DAMAGE_BONUS_MISC; the decrease half actually tests the attack constant.
            // The two are separate symbols that share the value 0, so the mix-up is harmless at
            // runtime.
            int nAttackBonusMisc = CSWSEffectListHandler::ATTACK_BONUS_MISC;
            for (int i = m_pStats->m_iFirstDamageIncrease; i < m_effects.m_count; i++) {
                CGameEffect* pEffect = m_effects.m_pArray[i];
                if (pEffect->m_subType > nDamageDecrease)
                    break;
                int nDamageType = pEffect->GetInteger(1);
                int nRace = pEffect->GetInteger(2);
                int nAlign = pEffect->GetInteger(4);
                int nBonusType = pEffect->GetInteger(5);
                int bApplies = (nBonusType == nBonusMisc || nBonusType == nAttackType);
                if (pVersus != 0 && pVersus->AsSWSItem() != 0 &&
                    pVersus->m_objectId != pEffect->m_creatorId)
                    bApplies = 0;
                if (nAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_EXTRA &&
                    (nBonusType == nBonusOnHand || nBonusType == nBonusCWeapon1))
                    bApplies = 1;
                if (nAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_UNARMED_EXTRA &&
                    nBonusType == nBonusUnarmed)
                    bApplies = 1;
                if ((nRace == g_pRules->m_nNumRaces || nRace == nTargetRace) &&
                    (nAlign == ALIGNMENT_ALL || nAlign == nAlignmentGroup) && bApplies == 1) {
                    if (bElemental == 0) {
                        if (nDamageType != 0)
                            goto damageApplies;
                    } else if (static_cast<unsigned int>(nDamageType) > 7) {
                        goto damageApplies;
                    }
                }
                // The elemental query takes anything that is not one of the three physical bits,
                // whether or not it passed the restriction tests above.
                if (bElemental != 1 || (nDamageType & 1) != 0 || (nDamageType & 2) != 0 ||
                    (nDamageType & 4) != 0)
                    continue;
            damageApplies:
                int nTypeIndex = Log_Base2(nDamageType);
                if (pEffect->m_subType == nDamageIncrease) {
                    if (nBonusType == nBonusMisc) {
                        int bFound = 0;
                        if (pEffect->m_spellId != -1) {
                            for (int j = 0; j < 108; j++) {
                                if (m_aStackIncSpell[j] == pEffect->m_spellId &&
                                    anIncType[j] == nDamageType) {
                                    p2DA->GetINTEntry(m_aStackIncValue[j], CExoString("Rank"),
                                                      &nStoredRank);
                                    bFound = 1;
                                    if (nIncomingRank > nStoredRank) {
                                        m_aStackIncValue[j] = pEffect->GetInteger(0);
                                        continue;
                                    }
                                }
                            }
                        }
                        if (bFound == 0) {
                            for (int j = 0; j < 108; j++) {
                                if (m_aStackIncValue[j] == -1) {
                                    m_aStackIncSpell[j] = pEffect->m_spellId;
                                    m_aStackIncValue[j] = pEffect->GetInteger(0);
                                    anIncType[j] = nDamageType;
                                    break;
                                }
                            }
                        }
                    } else {
                        // As written, nIncomingRank is assigned only here, yet it is read by the
                        // stacking search above and by the decrease half below, so on the first
                        // misc-bonus effect of a call the comparison runs against a stale rank (0
                        // on the first tick). Effects sort increase-before-decrease, which usually
                        // hides it.
                        p2DA->GetINTEntry(pEffect->GetInteger(0), CExoString("Rank"),
                                          &nIncomingRank);
                        int nAmount = pEffect->GetInteger(0);
                        nStoredRank = nAmount;
                        int nDamage = nAmount;
                        if (nAmount >= 6) {
                            p2DA->GetINTEntry(nAmount, CExoString("NumDice"), &nNumDice);
                            p2DA->GetINTEntry(nStoredRank, CExoString("Die"), &nDie);
                            if (bMaxDamage != 0) {
                                nDamage = nNumDice * nDie;
                                nOut += nNumDice - nDamage;
                            } else {
                                int nTimes = 1;
                                if (pAttack->m_nAttackResult == 2)
                                    nTimes = m_pStats->GetCriticalHitMultiplier(bOffHand);
                                nDamage = 0;
                                for (int t = 0; t < nTimes; t++)
                                    nDamage += g_pRules->RollDice(static_cast<unsigned char>(nNumDice), static_cast<unsigned char>(nDie));
                            }
                        }
                        anDamage[nTypeIndex] += nDamage;
                    }
                } else if (pEffect->m_subType == nDamageDecrease) {
                    if (nBonusType == nAttackBonusMisc) {
                        int bFound = 0;
                        if (pEffect->m_spellId != -1) {
                            for (int j = 0; j < 108; j++) {
                                if (m_aStackDecSpell[j] == pEffect->m_spellId &&
                                    anDecType[j] == nDamageType) {
                                    p2DA->GetINTEntry(m_aStackDecValue[j], CExoString("Rank"),
                                                      &nStoredRank);
                                    bFound = 1;
                                    if (nIncomingRank > nStoredRank) {
                                        m_aStackDecValue[j] = pEffect->GetInteger(0);
                                        continue;
                                    }
                                }
                            }
                        }
                        if (bFound == 0) {
                            for (int j = 0; j < 108; j++) {
                                if (m_aStackDecValue[j] == -1) {
                                    m_aStackDecSpell[j] = pEffect->m_spellId;
                                    m_aStackDecValue[j] = pEffect->GetInteger(0);
                                    anDecType[j] = nDamageType;
                                    break;
                                }
                            }
                        }
                    } else {
                        int nRank = 0;
                        p2DA->GetINTEntry(anDecrease[nTypeIndex], CExoString("Rank"), &nRank);
                        if (nIncomingRank > nRank)
                            anDecrease[nTypeIndex] = pEffect->GetInteger(0);
                    }
                }
            }
        }

        // Apply the per-type immediate damage the walk accumulated. Each of the fifteen damage
        // types is its own bit, and each goes through the target's immunity then its resistance
        // before it is recorded on the attack.
        int nTotalInc = 0;
        int nTotalDec = 0;
        for (int k = 0; k < 15; k++) {
            unsigned short nFlag = static_cast<unsigned short>(1 << k);
            int nAmount = anDamage[k];
            if (nAmount > 0) {
                if (pVersus != 0) {
                    int nAfterImmunity =
                        pVersus->DoDamageImmunity(this, nAmount, nFlag, bMaxDamage, 1);
                    nAmount = pVersus->DoDamageResistance(this, nAfterImmunity, nFlag, bMaxDamage,
                                                          1, 0);
                }
                if (bMaxDamage == 0)
                    pAttack->AddDamage(nFlag, nAmount, 1);
                nTotalInc += nAmount;
            }
            if (anDecrease[k] > 0) {
                pAttack->AddDamage(nFlag, -anDecrease[k], 1);
                nTotalDec += anDecrease[k];
            }
        }

        // Then apply whatever survived the stacking tables.
        int nStackInc = 0;
        int nStackDec = 0;
        for (int j = 0; j < 108; j++) {
            if (m_aStackIncValue[j] != -1) {
                int nType = anIncType[j];
                unsigned int nTypeIndex = Log_Base2(nType);
                int nAmount = m_aStackIncValue[j];
                if (nAmount >= 6) {
                    p2DA->GetINTEntry(nAmount, CExoString("NumDice"), &nNumDice);
                    p2DA->GetINTEntry(m_aStackIncValue[j], CExoString("Die"), &nDie);
                    int nRolled;
                    if (bMaxDamage == 1)
                        nRolled = nDie * nNumDice;
                    else
                        nRolled = g_pRules->RollDice(static_cast<unsigned char>(nNumDice), static_cast<unsigned char>(nDie));
                    if (bElemental == 1) {
                        if (nTypeIndex >= 3) {
                            if (pVersus != 0) {
                                int nAfterImmunity = pVersus->DoDamageImmunity(this, nRolled,
                                                                               DAMAGE_TYPE_BASE, bMaxDamage, 1);
                                nRolled = pVersus->DoDamageResistance(this, nAfterImmunity, DAMAGE_TYPE_BASE,
                                                                      bMaxDamage, 1, 0);
                            }
                            if (bMaxDamage == 0)
                                pAttack->AddDamage(static_cast<unsigned short>(nType), nRolled, 1);
                            nStackInc += nRolled;
                        }
                    } else {
                        nStackInc += nRolled;
                    }
                } else if (bElemental == 1) {
                    if (nTypeIndex >= 3) {
                        if (pVersus != 0) {
                            int nAfterImmunity =
                                pVersus->DoDamageImmunity(this, nAmount, DAMAGE_TYPE_BASE, bMaxDamage, 1);
                            nAmount = pVersus->DoDamageResistance(this, nAfterImmunity, DAMAGE_TYPE_BASE,
                                                                  bMaxDamage, 1, 0);
                        }
                        if (bMaxDamage == 0)
                            pAttack->AddDamage(static_cast<unsigned short>(nType), nAmount, 1);
                        nStackInc += nAmount;
                    }
                } else {
                    nStackInc += nAmount;
                }
            }
            if (m_aStackDecValue[j] != -1) {
                int nType = anDecType[j];
                unsigned int nTypeIndex = Log_Base2(nType);
                int nAmount = m_aStackDecValue[j];
                if (nAmount >= 6) {
                    p2DA->GetINTEntry(nAmount, CExoString("NumDice"), &nNumDice);
                    p2DA->GetINTEntry(m_aStackDecValue[j], CExoString("Die"), &nDie);
                    int nRolled;
                    if (bMaxDamage == 1)
                        nRolled = nDie * nNumDice;
                    else
                        nRolled = g_pRules->RollDice(static_cast<unsigned char>(nNumDice), static_cast<unsigned char>(nDie));
                    if (bElemental == 1) {
                        if (nTypeIndex >= 3)
                            pAttack->AddDamage(static_cast<unsigned short>(nType), -nRolled, 1);
                    } else {
                        nStackDec += nRolled;
                    }
                } else if (bElemental == 1) {
                    if (nTypeIndex >= 3)
                        pAttack->AddDamage(static_cast<unsigned short>(nType), -nAmount, 1);
                } else {
                    nStackDec += nAmount;
                }
            }
        }

        int nSumInc = nTotalInc + nStackInc;
        int nSumDec = nTotalDec + nStackDec;
        if (pOut != 0)
            *pOut = nStackInc - nStackDec;
        if (nSumInc > 108)
            nSumInc = 108;
        if (nSumDec > 108)
            nSumDec = 108;
        return nSumInc - nSumDec;
    }

    case EFFECT_BONUS_SAVING_THROW: {
        // Saving throw. The integer parameters are, in order: magnitude, which save, what the save
        // is against, a racial restriction and an alignment restriction. A zero for the save or the
        // "against" parameter matches anything.
        if (m_effects.m_count > 0 && m_pStats->m_iFirstSavingThrowIncrease < m_effects.m_count) {
            int nSaveDecrease = CSWSEffectListHandler::EFFECT_SAVING_THROW_DECREASE;
            int nWantSave = nSaveType;
            int nWantVs = nSaveVs;
            int nSaveIncrease = CSWSEffectListHandler::EFFECT_SAVING_THROW_INCREASE;
            for (int i = m_pStats->m_iFirstSavingThrowIncrease; i < m_effects.m_count; i++) {
                CGameEffect* pEffect = m_effects.m_pArray[i];
                if (pEffect->m_subType > nSaveDecrease)
                    break;
                pEffect->GetInteger(0);
                int nWhich = pEffect->GetInteger(1);
                int nVs = pEffect->GetInteger(2);
                int nRace = pEffect->GetInteger(3);
                int nAlign = pEffect->GetInteger(5);
                if ((nWhich == 0 || nWhich == nWantSave) && (nVs == 0 || nVs == nWantVs) &&
                    (nRace == g_pRules->m_nNumRaces || nRace == nTargetRace) &&
                    (nAlign == ALIGNMENT_ALL || nAlign == nAlignmentGroup)) {
                    if (pEffect->m_subType == nSaveIncrease) {
                        StackEffectValue(pEffect, 0, m_aStackIncItem, m_aStackIncValue,
                                         m_aStackIncSpell);
                    } else if (pEffect->m_subType == nSaveDecrease) {
                        StackEffectValue(pEffect, 0, m_aStackDecItem, m_aStackDecValue,
                                         m_aStackDecSpell);
                    }
                }
            }
        }
        int nSumInc = 0;
        int nSumDec = 0;
        for (int j = 0; j < 108; j++) {
            if (m_aStackIncValue[j] != -1)
                nSumInc += m_aStackIncValue[j];
            if (m_aStackDecValue[j] != -1)
                nSumDec += m_aStackDecValue[j];
        }
        // The wounded-save feat's band rides on top of the increases before the cap.
        if (m_pStats->HasFeat(FEAT_SURVIVAL)) {
            float fCurrent = static_cast<float>(GetCurrentHitPoints(0));
            float fMax = static_cast<float>(GetMaxHitPoints(1));
            nSumInc += WoundedSaveBonusBand(fCurrent / fMax * 100.0f);
        }
        if (nSumInc > 60)
            nSumInc = 60;
        if (nSumDec > 60)
            nSumDec = 60;
        return nSumInc - nSumDec;
    }

    case EFFECT_BONUS_ABILITY: {
        // Ability score. The first parameter is the ability index and must match exactly; this is
        // the one family with no wildcard.
        if (m_effects.m_count > 0 && m_pStats->m_iFirstAbilityIncrease < m_effects.m_count) {
            int nAbilityDecrease = CSWSEffectListHandler::EFFECT_ABILITY_DECREASE;
            int nWantAbility = nAbility;
            int nAbilityIncrease = CSWSEffectListHandler::EFFECT_ABILITY_INCREASE;
            for (int i = m_pStats->m_iFirstAbilityIncrease; i < m_effects.m_count; i++) {
                CGameEffect* pEffect = m_effects.m_pArray[i];
                if (pEffect->m_subType > nAbilityDecrease)
                    break;
                int nWhich = pEffect->GetInteger(0);
                pEffect->GetInteger(1);
                if (nWhich == nWantAbility) {
                    if (pEffect->m_subType == nAbilityIncrease) {
                        StackEffectValue(pEffect, 1, m_aStackIncItem, m_aStackIncValue,
                                         m_aStackIncSpell);
                    } else if (pEffect->m_subType == nAbilityDecrease) {
                        StackEffectValueSum(pEffect, 1, m_aStackDecItem, m_aStackDecValue,
                                            m_aStackDecSpell);
                    }
                }
            }
        }
        int nSumInc = 0;
        int nSumDec = 0;
        for (int j = 0; j < 108; j++) {
            if (m_aStackIncValue[j] != -1)
                nSumInc += m_aStackIncValue[j];
            if (m_aStackDecValue[j] != -1)
                nSumDec += m_aStackDecValue[j];
        }
        if (nSumInc > 60)
            nSumInc = 60;
        if (nSumDec > 90)
            nSumDec = 90;
        return nSumInc - nSumDec;
    }

    case EFFECT_BONUS_SKILL: {
        if (m_effects.m_count > 0 && m_pStats->m_iFirstSkillIncrease < m_effects.m_count) {
            int nSkillDecrease = CSWSEffectListHandler::EFFECT_SKILL_DECREASE;
            int nWantSkill = nSkill;
            int nSkillIncrease = CSWSEffectListHandler::EFFECT_SKILL_INCREASE;
            for (int i = m_pStats->m_iFirstSkillIncrease; i < m_effects.m_count; i++) {
                CGameEffect* pEffect = m_effects.m_pArray[i];
                if (pEffect->m_subType > nSkillDecrease)
                    break;
                int nWhich = pEffect->GetInteger(0);
                pEffect->GetInteger(1);
                int nRace = pEffect->GetInteger(2);
                int nAlign = pEffect->GetInteger(4);
                if ((nWhich == SKILL_ALL || nWhich == nWantSkill) &&
                    (nRace == g_pRules->m_nNumRaces || nRace == nTargetRace) &&
                    (nAlign == ALIGNMENT_ALL || nAlign == nAlignmentGroup)) {
                    if (pEffect->m_subType == nSkillIncrease) {
                        StackEffectValue(pEffect, 1, m_aStackIncItem, m_aStackIncValue,
                                         m_aStackIncSpell);
                    } else if (pEffect->m_subType == nSkillDecrease) {
                        StackEffectValue(pEffect, 1, m_aStackDecItem, m_aStackDecValue,
                                         m_aStackDecSpell);
                    }
                }
            }
        }
        int nSumInc = 0;
        int nSumDec = 0;
        for (int j = 0; j < 108; j++) {
            if (m_aStackIncValue[j] != -1)
                nSumInc += m_aStackIncValue[j];
            if (m_aStackDecValue[j] != -1)
                nSumDec += m_aStackDecValue[j];
        }
        if (nSumInc > 90)
            nSumInc = 90;
        if (nSumDec > 90)
            nSumDec = 90;
        return nSumInc - nSumDec;
    }
    }
    return 0;
}
#endif  // !KOTOR1

#if KOTOR1
static inline __attribute__((always_inline)) void K1StackEffectValue(CGameEffect* pEffect, int nSlot,
                                                                   unsigned long long* paItem,
                                                                   int* paValue, int* paSpell)
{
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pEffect->m_creatorId);
    int bFound = 0;
    if (pItem != 0) {
        for (int j = 0; j < 36; j++) {
            if (paItem[j] == pItem->m_objectId) {
                bFound = 1;
                if (paValue[j] < pEffect->GetInteger(nSlot)) {
                    paValue[j] = pEffect->GetInteger(nSlot);
                    return;
                }
            }
        }
        if (bFound == 0) {
            for (int j = 0; j < 36; j++) {
                if (paValue[j] == -1) {
                    paItem[j] = pItem->m_objectId;
                    paValue[j] = pEffect->GetInteger(nSlot);
                    return;
                }
            }
        }
        return;
    }
    if (pEffect->m_spellId != -1) {
        for (int j = 0; j < 36; j++) {
            if (paSpell[j] == pEffect->m_spellId) {
                bFound = 1;
                if (paValue[j] < pEffect->GetInteger(nSlot)) {
                    paValue[j] = pEffect->GetInteger(nSlot);
                    return;
                }
            }
        }
    }
    if (bFound == 0) {
        for (int j = 0; j < 36; j++) {
            if (paValue[j] == -1) {
                paSpell[j] = pEffect->m_spellId;
                paValue[j] = pEffect->GetInteger(nSlot);
                return;
            }
        }
    }
}

static inline __attribute__((always_inline)) void K1StackEffectValueSum(CGameEffect* pEffect, int nSlot,
                                                                      unsigned long long* paItem,
                                                                      int* paValue, int* paSpell)
{
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pEffect->m_creatorId);
    int bFound = 0;
    if (pItem != 0) {
        for (int j = 0; j < 36; j++) {
            if (paItem[j] == pItem->m_objectId) {
                bFound = 1;
                paValue[j] += pEffect->GetInteger(nSlot);
            }
        }
        if (bFound == 0) {
            for (int j = 0; j < 36; j++) {
                if (paValue[j] == -1) {
                    paItem[j] = pItem->m_objectId;
                    paValue[j] = pEffect->GetInteger(nSlot);
                    return;
                }
            }
        }
        return;
    }
    if (pEffect->m_spellId != -1) {
        for (int j = 0; j < 36; j++) {
            if (paSpell[j] == pEffect->m_spellId) {
                bFound = 1;
                paValue[j] += pEffect->GetInteger(nSlot);
            }
        }
    }
    if (bFound == 0) {
        for (int j = 0; j < 36; j++) {
            if (paValue[j] == -1) {
                paSpell[j] = pEffect->m_spellId;
                paValue[j] = pEffect->GetInteger(nSlot);
                return;
            }
        }
    }
}

int CSWSCreature::GetTotalEffectBonus(unsigned char nEffectType, CSWSObject* pVersus, int& nOut,
                                      int bElemental, int bMaxDamage, unsigned char nSaveType,
                                      unsigned char nSaveVs, unsigned char nSkill,
                                      unsigned char nAbility, int bOffHand)
{
    nOut = 0;

    CSWSCombatAttackData* pAttack = m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
    int nAttackType = pAttack->m_nWeaponAttackType;
    if (nAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_NONE || bMaxDamage != 0)
        nAttackType = (bOffHand == 0) ? CSWSCombatRound::WEAPON_ATTACK_TYPE_ONHAND
                                      : CSWSCombatRound::WEAPON_ATTACK_TYPE_OFFHAND;

    int nAlignmentGroup = -1;
    int nTargetRace = -1;
    if (pVersus != 0 && pVersus->AsSWSCreature() != 0) {
        nTargetRace = pVersus->AsSWSCreature()->m_pStats->m_nRace;
        nAlignmentGroup =
            static_cast<unsigned char>(pVersus->AsSWSCreature()->m_pStats->GetSimpleAlignmentGoodEvil());
    }

    for (int i = 0; i < 36; i++) {
        m_aStackIncValue[i] = -1;
        m_aStackDecValue[i] = -1;
        m_aStackIncSpell[i] = -1;
        m_aStackDecSpell[i] = -1;
        m_aStackIncItem[i] = OBJECT_INVALID;
        m_aStackDecItem[i] = OBJECT_INVALID;
    }

    switch (nEffectType) {
    case EFFECT_BONUS_ATTACK: {
        int nSumInc = 0;
        int nSumDec = 0;
        if (m_effects.m_count > 0 && m_pStats->m_iFirstAttackIncrease < m_effects.m_count) {
            int nAttackDecrease = CSWSEffectListHandler::EFFECT_ATTACK_DECREASE;
            int nBonusMisc = CSWSEffectListHandler::ATTACK_BONUS_MISC;
            int nBonusOnHand = CSWSEffectListHandler::ATTACK_BONUS_ONHAND;
            int nBonusCWeapon1 = CSWSEffectListHandler::ATTACK_BONUS_CWEAPON1;
            int nBonusUnarmed = CSWSEffectListHandler::ATTACK_BONUS_UNARMED;
            int nAttackIncrease = CSWSEffectListHandler::EFFECT_ATTACK_INCREASE;
            for (int i = m_pStats->m_iFirstAttackIncrease; i < m_effects.m_count; i++) {
                CGameEffect* pEffect = m_effects.m_pArray[i];
                if (pEffect->m_subType > nAttackDecrease)
                    break;
                int nBonusType = pEffect->GetInteger(1);
                int nRace = pEffect->GetInteger(2);
                int nAlign = pEffect->GetInteger(4);
                int bApplies = (nBonusType == nBonusMisc || nBonusType == nAttackType);
                if (nAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_EXTRA &&
                    (nBonusType == nBonusOnHand || nBonusType == nBonusCWeapon1))
                    bApplies = 1;
                if (nAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_UNARMED_EXTRA &&
                    nBonusType == nBonusUnarmed)
                    bApplies = 1;
                if ((nRace == g_pRules->m_nNumRaces || nRace == nTargetRace) &&
                    (nAlign == ALIGNMENT_ALL || nAlign == nAlignmentGroup) && bApplies == 1) {
                    if (pEffect->m_subType == nAttackIncrease) {
                        if (nBonusType != nBonusMisc) {
                            nSumInc += pEffect->GetInteger(0);
                        } else {
                            int bFound = 0;
                            if (pEffect->m_spellId != -1) {
                                for (int j = 0; j < 36; j++) {
                                    if (m_aStackIncSpell[j] == pEffect->m_spellId) {
                                        bFound = 1;
                                        if (m_aStackIncValue[j] < pEffect->GetInteger(0)) {
                                            m_aStackIncValue[j] = pEffect->GetInteger(0);
                                            goto nextAttackEffect;
                                        }
                                    }
                                }
                            }
                            if (bFound == 0) {
                                for (int j = 0; j < 36; j++) {
                                    if (m_aStackIncValue[j] == -1) {
                                        m_aStackIncSpell[j] = pEffect->m_spellId;
                                        m_aStackIncValue[j] = pEffect->GetInteger(0);
                                        break;
                                    }
                                }
                            }
                        }
                    } else if (pEffect->m_subType == nAttackDecrease) {
                        if (nBonusType != nBonusMisc) {
                            nSumDec += pEffect->GetInteger(0);
                        } else {
                            int bFound = 0;
                            if (pEffect->m_spellId != -1) {
                                for (int j = 0; j < 36; j++) {
                                    if (m_aStackDecSpell[j] == pEffect->m_spellId) {
                                        bFound = 1;
                                        if (m_aStackDecValue[j] < pEffect->GetInteger(0)) {
                                            m_aStackDecValue[j] = pEffect->GetInteger(0);
                                            goto nextAttackEffect;
                                        }
                                    }
                                }
                            }
                            if (bFound == 0) {
                                for (int j = 0; j < 36; j++) {
                                    if (m_aStackDecValue[j] == -1) {
                                        m_aStackDecSpell[j] = pEffect->m_spellId;
                                        m_aStackDecValue[j] = pEffect->GetInteger(0);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            nextAttackEffect:;
            }
        }
        for (int j = 0; j < 36; j++) {
            if (m_aStackIncValue[j] != -1)
                nSumInc += m_aStackIncValue[j];
            if (m_aStackDecValue[j] != -1)
                nSumDec += m_aStackDecValue[j];
        }
        if (nSumInc > 20)
            nSumInc = 20;
        if (nSumDec > 20)
            nSumDec = 20;
        return nSumInc - nSumDec;
    }

    case EFFECT_BONUS_DAMAGE: {
        int nIncomingRank = 0;
        int nStoredRank = 0;
        int nNumDice = 0;
        int nDie = 0;
        int anDecrease[15] = {};
        int anDamage[15] = {};
        int anDecType[36] = {};
        int anIncType[36] = {};
        C2DA* p2DA = g_pRules->m_p2DArrays->m_pIPRPDamage;
        if (m_effects.m_count > 0 && m_pStats->m_iFirstDamageIncrease < m_effects.m_count) {
            int nDamageDecrease = CSWSEffectListHandler::EFFECT_DAMAGE_DECREASE;
            int nBonusMisc = CSWSEffectListHandler::DAMAGE_BONUS_MISC;
            int nBonusOnHand = CSWSEffectListHandler::DAMAGE_BONUS_ONHAND;
            int nBonusCWeapon1 = CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON1;
            int nBonusUnarmed = CSWSEffectListHandler::DAMAGE_BONUS_UNARMED;
            int nDamageIncrease = CSWSEffectListHandler::EFFECT_DAMAGE_INCREASE;
            int nAttackBonusMisc = CSWSEffectListHandler::ATTACK_BONUS_MISC;
            for (int i = m_pStats->m_iFirstDamageIncrease; i < m_effects.m_count; i++) {
                CGameEffect* pEffect = m_effects.m_pArray[i];
                if (pEffect->m_subType > nDamageDecrease)
                    break;
                int nDamageType = pEffect->GetInteger(1);
                int nRace = pEffect->GetInteger(2);
                int nAlign = pEffect->GetInteger(4);
                int nBonusType = pEffect->GetInteger(5);
                int bApplies = (nBonusType == nBonusMisc || nBonusType == nAttackType);
                if (pVersus != 0 && pVersus->AsSWSItem() != 0 &&
                    pVersus->m_objectId != pEffect->m_creatorId)
                    bApplies = 0;
                if (nAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_EXTRA &&
                    (nBonusType == nBonusOnHand || nBonusType == nBonusCWeapon1))
                    bApplies = 1;
                if (nAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_UNARMED_EXTRA &&
                    nBonusType == nBonusUnarmed)
                    bApplies = 1;
                if ((nRace == g_pRules->m_nNumRaces || nRace == nTargetRace) &&
                    (nAlign == ALIGNMENT_ALL || nAlign == nAlignmentGroup) && bApplies == 1) {
                    if (bElemental == 0) {
                        if (nDamageType != 0)
                            goto damageApplies;
                    } else if (static_cast<unsigned int>(nDamageType) > 7) {
                        goto damageApplies;
                    }
                }
                if (bElemental != 1 || (nDamageType & 1) != 0 || (nDamageType & 2) != 0 ||
                    (nDamageType & 4) != 0)
                    continue;
            damageApplies:
                int nTypeIndex = Log_Base2(nDamageType);
                if (pEffect->m_subType == nDamageIncrease) {
                    if (nBonusType == nBonusMisc) {
                        int bFound = 0;
                        if (pEffect->m_spellId != -1) {
                            for (int j = 0; j < 36; j++) {
                                if (m_aStackIncSpell[j] == pEffect->m_spellId &&
                                    anIncType[j] == nDamageType) {
                                    p2DA->GetINTEntry(m_aStackIncValue[j], CExoString("Rank"),
                                                      &nStoredRank);
                                    bFound = 1;
                                    if (nIncomingRank > nStoredRank) {
                                        m_aStackIncValue[j] = pEffect->GetInteger(0);
                                        goto nextDamageEffect;
                                    }
                                }
                            }
                        }
                        if (bFound == 0) {
                            for (int j = 0; j < 36; j++) {
                                if (m_aStackIncValue[j] == -1) {
                                    m_aStackIncSpell[j] = pEffect->m_spellId;
                                    m_aStackIncValue[j] = pEffect->GetInteger(0);
                                    anIncType[j] = nDamageType;
                                    break;
                                }
                            }
                        }
                    } else {
                        p2DA->GetINTEntry(pEffect->GetInteger(0), CExoString("Rank"),
                                          &nIncomingRank);
                        int nAmount = pEffect->GetInteger(0);
                        nStoredRank = nAmount;
                        int nDamage = nAmount;
                        if (nAmount >= 6) {
                            p2DA->GetINTEntry(nAmount, CExoString("NumDice"), &nNumDice);
                            p2DA->GetINTEntry(nStoredRank, CExoString("Die"), &nDie);
                            if (bMaxDamage != 0) {
                                nDamage = nNumDice * nDie;
                                nOut += nNumDice - nDamage;
                            } else {
                                int nTimes = 1;
                                if (pAttack->m_nAttackResult == 2)
                                    nTimes = m_pStats->GetCriticalHitMultiplier(bOffHand);
                                nDamage = 0;
                                for (int t = 0; t < nTimes; t++)
                                    nDamage += g_pRules->RollDice(static_cast<unsigned char>(nNumDice), static_cast<unsigned char>(nDie));
                            }
                        }
                        anDamage[nTypeIndex] += nDamage;
                    }
                } else if (pEffect->m_subType == nDamageDecrease) {
                    if (nBonusType == nAttackBonusMisc) {
                        int bFound = 0;
                        if (pEffect->m_spellId != -1) {
                            for (int j = 0; j < 36; j++) {
                                if (m_aStackDecSpell[j] == pEffect->m_spellId &&
                                    anDecType[j] == nDamageType) {
                                    p2DA->GetINTEntry(m_aStackDecValue[j], CExoString("Rank"),
                                                      &nStoredRank);
                                    bFound = 1;
                                    if (nIncomingRank > nStoredRank) {
                                        m_aStackDecValue[j] = pEffect->GetInteger(0);
                                        goto nextDamageEffect;
                                    }
                                }
                            }
                        }
                        if (bFound == 0) {
                            for (int j = 0; j < 36; j++) {
                                if (m_aStackDecValue[j] == -1) {
                                    m_aStackDecSpell[j] = pEffect->m_spellId;
                                    m_aStackDecValue[j] = pEffect->GetInteger(0);
                                    anDecType[j] = nDamageType;
                                    break;
                                }
                            }
                        }
                    } else {
                        int nRank = 0;
                        p2DA->GetINTEntry(anDecrease[nTypeIndex], CExoString("Rank"), &nRank);
                        if (nIncomingRank > nRank)
                            anDecrease[nTypeIndex] = pEffect->GetInteger(0);
                    }
                }
            nextDamageEffect:;
            }
        }

        // K1 bug: nFlag is only set by a positive increase, so a decrease is applied (and a stacked
        // elemental bonus resisted) with the flag of the last increased type.
#ifdef K2_FIX_ENGINE_BUGS
        unsigned short nFlag = 0;
#else
        unsigned short nFlag;
#endif
        int nSumInc = 0;
        int nSumDec = 0;
        for (int k = 0; k < 15; k++) {
#ifdef K2_FIX_ENGINE_BUGS
            nFlag = static_cast<unsigned short>(1 << k);
#endif
            int nAmount = anDamage[k];
            if (nAmount > 0) {
                nFlag = static_cast<unsigned short>(1 << k);
                if (pVersus != 0) {
                    int nAfterImmunity =
                        pVersus->DoDamageImmunity(this, nAmount, nFlag, bMaxDamage, 1);
                    nAmount = pVersus->DoDamageResistance(this, nAfterImmunity, nFlag, bMaxDamage,
                                                          1, 0);
                }
                if (bMaxDamage == 0)
                    pAttack->AddDamage(nFlag, nAmount, 1);
                nSumInc += nAmount;
            }
            if (anDecrease[k] > 0) {
                pAttack->AddDamage(nFlag, -anDecrease[k], 1);
                nSumDec += anDecrease[k];
            }
        }

        for (int j = 0; j < 36; j++) {
            // K1: one running amount per slot; a decrease's critical multiply adds onto whatever
            // the increase left in it.
            int nDamage = 0;
            if (m_aStackIncValue[j] != -1) {
                int nType = anIncType[j];
                unsigned int nTypeIndex = Log_Base2(nType);
                if (m_aStackIncValue[j] >= 6) {
                    p2DA->GetINTEntry(m_aStackIncValue[j], CExoString("NumDice"), &nNumDice);
                    p2DA->GetINTEntry(m_aStackIncValue[j], CExoString("Die"), &nDie);
                    if (bMaxDamage == 1) {
                        nDamage = nDie * nNumDice;
                    } else if (pAttack->m_nAttackResult == 2) {
                        for (int t = 0; t < m_pStats->GetCriticalHitMultiplier(bOffHand); t++)
                            nDamage += g_pRules->RollDice(static_cast<unsigned char>(nNumDice), static_cast<unsigned char>(nDie));
                    } else {
                        nDamage = g_pRules->RollDice(static_cast<unsigned char>(nNumDice), static_cast<unsigned char>(nDie));
                    }
                    if (bElemental == 1) {
                        if (nTypeIndex >= 3) {
                            if (pVersus != 0) {
                                int nAfterImmunity = pVersus->DoDamageImmunity(this, nDamage,
                                                                               nFlag, bMaxDamage, 1);
                                nDamage = pVersus->DoDamageResistance(this, nAfterImmunity, nFlag,
                                                                      bMaxDamage, 1, 0);
                            }
                            if (bMaxDamage == 0)
                                pAttack->AddDamage(static_cast<unsigned short>(nType), nDamage, 1);
                            else
                                nSumInc += nDamage;
                        }
                    } else {
                        nSumInc += nDamage;
                    }
                } else {
                    if (pAttack->m_nAttackResult == 2) {
                        for (int t = 0; t < m_pStats->GetCriticalHitMultiplier(bOffHand); t++)
                            nDamage += m_aStackIncValue[j];
                    } else {
                        nDamage = m_aStackIncValue[j];
                    }
                    if (bElemental == 1) {
                        if (nTypeIndex >= 3) {
                            if (pVersus != 0) {
                                int nAfterImmunity = pVersus->DoDamageImmunity(this, nDamage,
                                                                               nFlag, bMaxDamage, 1);
                                nDamage = pVersus->DoDamageResistance(this, nAfterImmunity, nFlag,
                                                                      bMaxDamage, 1, 0);
                            }
                            // K1 bug: a maximum-damage query doubles the amount instead of
                            // counting it.
                            if (bMaxDamage == 0)
                                pAttack->AddDamage(static_cast<unsigned short>(nType), nDamage, 1);
                            else
                                nDamage += nDamage;
                        }
                    } else {
                        nSumInc += nDamage;
                    }
                }
            }
            if (m_aStackDecValue[j] != -1) {
                int nType = anDecType[j];
                unsigned int nTypeIndex = Log_Base2(nType);
                if (m_aStackDecValue[j] >= 6) {
                    p2DA->GetINTEntry(m_aStackDecValue[j], CExoString("NumDice"), &nNumDice);
                    p2DA->GetINTEntry(m_aStackDecValue[j], CExoString("Die"), &nDie);
                    if (bMaxDamage == 1) {
                        nDamage = nDie * nNumDice;
                    } else if (pAttack->m_nAttackResult == 2) {
                        for (int t = 0; t < m_pStats->GetCriticalHitMultiplier(bOffHand); t++)
                            nDamage += g_pRules->RollDice(static_cast<unsigned char>(nNumDice), static_cast<unsigned char>(nDie));
                    } else {
                        nDamage = g_pRules->RollDice(static_cast<unsigned char>(nNumDice), static_cast<unsigned char>(nDie));
                    }
                } else {
                    if (pAttack->m_nAttackResult == 2) {
                        for (int t = 0; t < m_pStats->GetCriticalHitMultiplier(bOffHand); t++)
                            nDamage += m_aStackDecValue[j];
                    } else {
                        nDamage = m_aStackDecValue[j];
                    }
                }
                if (bElemental == 1) {
                    if (nTypeIndex >= 3)
                        pAttack->AddDamage(static_cast<unsigned short>(nType), -nDamage, 1);
                } else {
                    nSumDec += nDamage;
                }
            }
        }

        if (nSumInc > 36)
            nSumInc = 36;
        if (nSumDec > 36)
            nSumDec = 36;
        return nSumInc - nSumDec;
    }

    case EFFECT_BONUS_SAVING_THROW: {
        if (m_effects.m_count > 0 && m_pStats->m_iFirstSavingThrowIncrease < m_effects.m_count) {
            int nSaveDecrease = CSWSEffectListHandler::EFFECT_SAVING_THROW_DECREASE;
            int nWantSave = nSaveType;
            int nWantVs = nSaveVs;
            int nSaveIncrease = CSWSEffectListHandler::EFFECT_SAVING_THROW_INCREASE;
            for (int i = m_pStats->m_iFirstSavingThrowIncrease; i < m_effects.m_count; i++) {
                CGameEffect* pEffect = m_effects.m_pArray[i];
                if (pEffect->m_subType > nSaveDecrease)
                    break;
                pEffect->GetInteger(0);
                int nWhich = pEffect->GetInteger(1);
                int nVs = pEffect->GetInteger(2);
                int nRace = pEffect->GetInteger(3);
                int nAlign = pEffect->GetInteger(5);
                if ((nWhich == 0 || nWhich == nWantSave) && (nVs == 0 || nVs == nWantVs) &&
                    (nRace == g_pRules->m_nNumRaces || nRace == nTargetRace) &&
                    (nAlign == ALIGNMENT_ALL || nAlign == nAlignmentGroup)) {
                    if (pEffect->m_subType == nSaveIncrease) {
                        K1StackEffectValue(pEffect, 0, m_aStackIncItem, m_aStackIncValue,
                                         m_aStackIncSpell);
                    } else if (pEffect->m_subType == nSaveDecrease) {
                        K1StackEffectValue(pEffect, 0, m_aStackDecItem, m_aStackDecValue,
                                         m_aStackDecSpell);
                    }
                }
            }
        }
        int nSumInc = 0;
        int nSumDec = 0;
        for (int j = 0; j < 36; j++) {
            if (m_aStackIncValue[j] != -1)
                nSumInc += m_aStackIncValue[j];
            if (m_aStackDecValue[j] != -1)
                nSumDec += m_aStackDecValue[j];
        }
        if (nSumInc > 20)
            nSumInc = 20;
        if (nSumDec > 20)
            nSumDec = 20;
        return nSumInc - nSumDec;
    }

    case EFFECT_BONUS_ABILITY: {
        if (m_effects.m_count > 0 && m_pStats->m_iFirstAbilityIncrease < m_effects.m_count) {
            int nAbilityDecrease = CSWSEffectListHandler::EFFECT_ABILITY_DECREASE;
            int nWantAbility = nAbility;
            int nAbilityIncrease = CSWSEffectListHandler::EFFECT_ABILITY_INCREASE;
            for (int i = m_pStats->m_iFirstAbilityIncrease; i < m_effects.m_count; i++) {
                CGameEffect* pEffect = m_effects.m_pArray[i];
                if (pEffect->m_subType > nAbilityDecrease)
                    break;
                int nWhich = pEffect->GetInteger(0);
                pEffect->GetInteger(1);
                if (nWhich == nWantAbility) {
                    if (pEffect->m_subType == nAbilityIncrease) {
                        K1StackEffectValue(pEffect, 1, m_aStackIncItem, m_aStackIncValue,
                                         m_aStackIncSpell);
                    } else if (pEffect->m_subType == nAbilityDecrease) {
                        K1StackEffectValueSum(pEffect, 1, m_aStackDecItem, m_aStackDecValue,
                                            m_aStackDecSpell);
                    }
                }
            }
        }
        int nSumInc = 0;
        int nSumDec = 0;
        for (int j = 0; j < 36; j++) {
            if (m_aStackIncValue[j] != -1)
                nSumInc += m_aStackIncValue[j];
            if (m_aStackDecValue[j] != -1)
                nSumDec += m_aStackDecValue[j];
        }
        if (nSumInc > 20)
            nSumInc = 20;
        if (nSumDec > 30)
            nSumDec = 30;
        return nSumInc - nSumDec;
    }

    case EFFECT_BONUS_SKILL: {
        if (m_effects.m_count > 0 && m_pStats->m_iFirstSkillIncrease < m_effects.m_count) {
            int nSkillDecrease = CSWSEffectListHandler::EFFECT_SKILL_DECREASE;
            int nWantSkill = nSkill;
            int nSkillIncrease = CSWSEffectListHandler::EFFECT_SKILL_INCREASE;
            for (int i = m_pStats->m_iFirstSkillIncrease; i < m_effects.m_count; i++) {
                CGameEffect* pEffect = m_effects.m_pArray[i];
                if (pEffect->m_subType > nSkillDecrease)
                    break;
                int nWhich = pEffect->GetInteger(0);
                pEffect->GetInteger(1);
                int nRace = pEffect->GetInteger(2);
                int nAlign = pEffect->GetInteger(4);
                if ((nWhich == SKILL_ALL || nWhich == nWantSkill) &&
                    (nRace == g_pRules->m_nNumRaces || nRace == nTargetRace) &&
                    (nAlign == ALIGNMENT_ALL || nAlign == nAlignmentGroup)) {
                    if (pEffect->m_subType == nSkillIncrease) {
                        K1StackEffectValue(pEffect, 1, m_aStackIncItem, m_aStackIncValue,
                                         m_aStackIncSpell);
                    } else if (pEffect->m_subType == nSkillDecrease) {
                        K1StackEffectValue(pEffect, 1, m_aStackDecItem, m_aStackDecValue,
                                         m_aStackDecSpell);
                    }
                }
            }
        }
        int nSumInc = 0;
        int nSumDec = 0;
        for (int j = 0; j < 36; j++) {
            if (m_aStackIncValue[j] != -1)
                nSumInc += m_aStackIncValue[j];
            if (m_aStackDecValue[j] != -1)
                nSumDec += m_aStackDecValue[j];
        }
        if (nSumInc > 30)
            nSumInc = 30;
        if (nSumDec > 30)
            nSumDec = 30;
        return nSumInc - nSumDec;
    }
    }
    return 0;
}
#endif

void CSWSCreature::SetAmbientAnimationState(unsigned char nState)
{
    if (static_cast<unsigned char>(nState - 3) <= 1) {
        if (m_nAnimation == SERVER_ANIMATION_WHIRLWIND || m_nAnimation == SERVER_ANIMATION_KNOCKED_DOWN_LOOP
            || m_nAnimation == SERVER_ANIMATION_PRONE)
            nState = 14;
    }
    m_ambientAnimationState = nState;
}

CResRef CSWSCreature::GetDialogResref()
{
    if (m_pStats != 0)
        return m_pStats->GetConversation();
    return CResRef("");
}

CResRef CSWSCreature::GetPortrait()
{
    if (m_pStats != 0)
        return m_pStats->GetPortrait();
    return CResRef("");
}

int CSWSCreature::ComputeTotalWeightCarried()
{
    int nWeight = m_nEquippedWeight;
#if KOTOR1  // K1: always the creature's own inventory view (GetItemRepository(1))
    return GetItemRepository(1)->CalculateContentsWeight() + nWeight;
#else
    CItemRepository* pRepo;
    if ((g_pAppManager->m_pClientExoApp->GetInGameGui()->GetTopMenuOverrideCharacter() == -1) &
        (m_bInParty == 0)) {
        pRepo = m_pItemRepository;
    } else {
        pRepo = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems();
    }
    return pRepo->CalculateContentsWeight() + nWeight;
#endif
}

void CSWSCreature::SetGold(int nGold)
{
    if (nGold > 999999999)
        nGold = 999999999;
    if (m_bInParty != 0) {
        g_pAppManager->m_pServerExoApp->GetPartyTable()->m_nGold = nGold;
#if !KOTOR1  // K1: no achievements
        if (nGold >= 10000) {
            if (m_bPlayerMadeCharacter != 0)
                Achievements::UnlockAchievement(Achievements::ACHIEVEMENT_TEN_THOUSAND_CREDITS);
        }
#endif
    } else {
        m_nGold = nGold;
    }
}

void CSWSCreature::StopGuiTimingBar()
{
    if (m_bPlayerCharacter == 1) {
        CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
        CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
        if (pPlayer != 0 && pMessage != 0)
            pMessage->SendServerToPlayerGuiTimingEvent(pPlayer, 0, 0, 0);
    }
}

void CSWSCreature::Rest(int a)
{
    K2_UNUSED(a);
    CExoString sTemp;
    int nIsPlayerCharacter;
    int nActionParam2 = 0;
    CServerAIMaster* pAI;

#if KOTOR1  // K1: a dying PC cannot rest
    if (GetDead() || GetIsPCDying())
        return;
#else
    if (GetDead())
        return;
#endif

    nIsPlayerCharacter = m_bPlayerCharacter;
    if (nIsPlayerCharacter != 0 && GetArea() != 0 && GetArea()->m_noRest != 0) {
        SendFeedbackMessage(FEEDBACK_REST_NOT_ALLOWED_IN_AREA, 0);
        return;
    }

    if (GetNearestEnemy(30.0f, OBJECT_INVALID, OBJECT_INVALID) != OBJECT_INVALID) {
        SendFeedbackMessage(FEEDBACK_REST_ENEMIES_NEARBY, 0);
        return;
    }

    ClearAllActions(1);

    if (nIsPlayerCharacter != 0 && m_nExcitedStateTimer != 0) {
        SendFeedbackMessage(FEEDBACK_REST_EXCITED, 0);
        return;
    }

    m_nRestStartDay = 0;
    m_nRestStartTime = 0;
    m_nRestEndDay = 0;
    m_nRestEndTime = 0;

    ClearActivities((ACTIVITY_STEALTH | ACTIVITY_DETECT | ACTIVITY_CONVERSATION | ACTIVITY_RESTING));
    SetActivity(ACTIVITY_RESTING, 1, 0);
    ActionManager(8);

    if (nIsPlayerCharacter != 0) {
        SendFeedbackMessage(FEEDBACK_RESTING, 0);
        CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
        CScriptEvent* pEvent = new CScriptEvent();
        pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_REST;
        pEvent->SetInteger(0, 1);
        pAI = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
        pAI->AddEventAbsoluteTime(0, 0, m_objectId, pModule->m_objectId,
                                  CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
    }

    AddAction(AIACTION_REST, ACTION_GROUP_NEW, ACTION_PARAM_INT, &nIsPlayerCharacter,
              ACTION_PARAM_INT, &nActionParam2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
              0, 0, 0, 0, 0);
}

void CSWSCreature::CancelRest(unsigned short nFeedbackMessage)
{
    SetAnimation(SERVER_ANIMATION_PAUSE);
    SendFeedbackMessage(nFeedbackMessage, 0);
    if ((m_nLockedActivityFlags & ACTIVITY_RESTING) == 0)
        m_activityFlags &= ~static_cast<unsigned int>(ACTIVITY_RESTING);
    if (m_bPlayerCharacter != 0) {
        CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
        CScriptEvent* pEvent = new CScriptEvent();
        pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_REST;
        pEvent->SetInteger(0, 3);
        CServerAIMaster* pAI = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
        pAI->AddEventAbsoluteTime(0, 0, m_objectId, pModule->m_objectId,
                                  CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        StopGuiTimingBar();
    }
}

void CSWSCreature::ResetHeartbeatTimer()
{
    unsigned int nDay;
    unsigned int nTimeOfDay;
    g_pAppManager->m_pServerExoApp->GetActiveTimer(m_objectId)->GetWorldTime(&nDay, &nTimeOfDay);
    m_nLastHeartbeatDay = nDay;
    m_nLastHeartbeatTod = nTimeOfDay;
}

void CSWSCreature::BroadcastSavingThrowData(CSWCCMessageData* pData)
{
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
    if (pPlayer != 0)
        g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerCCMessage(
            pPlayer->m_nPlayerId, CC_MESSAGE_SAVING_THROW, pData, 0);
}

#if !KOTOR1  // K2-only (no K1 counterpart)
void CSWSCreature::SetIsPuppet(int isPuppet)
{
    m_isPuppet = isPuppet;
    if (isPuppet != 0) {
        if (m_aiLevel == 4)
            return;
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->SetAILevel(this, 4);
    } else {
        if (m_aiLevel == 2)
            return;
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->SetAILevel(this, 2);
    }
}
#endif  // !KOTOR1

int CSWSCreature::IsInjured()
{
    if (m_bDisableInjuredAnim != 0)
        return 0;
    return static_cast<float>(GetCurrentHitPoints(0)) / static_cast<float>(GetMaxHitPoints(1)) < g_fInjuredLevel;
}

bool CSWSSpellScriptData::SaveData(CResGFF* pGff, CResStruct* pStruct)
{
    pGff->WriteFieldINT(pStruct, m_nSpellId, const_cast<char*>("SpellId"));
    pGff->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_casterId), const_cast<char*>("CasterId"));
    pGff->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_targetId), const_cast<char*>("TargetId"));
    pGff->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_areaId), const_cast<char*>("AreaId"));
    pGff->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_itemId), const_cast<char*>("ItemId"));
    pGff->WriteFieldCExoString(pStruct, m_sScript, const_cast<char*>("Script"));
    pGff->WriteFieldFLOAT(pStruct, m_targetPosition.x, const_cast<char*>("TargetPosX"));
    pGff->WriteFieldFLOAT(pStruct, m_targetPosition.y, const_cast<char*>("TargetPosY"));
    pGff->WriteFieldFLOAT(pStruct, m_targetPosition.z, const_cast<char*>("TargetPosZ"));
#if !KOTOR1  // K1: no FinalForceCost
    pGff->WriteFieldINT(pStruct, m_nFinalForceCost, const_cast<char*>("FinalForceCost"));
#endif
    return true;
}

bool CSWSSpellScriptData::LoadData(CResGFF* pGff, CResStruct* pStruct)
{
    int bSuccess;
    m_nSpellId = pGff->ReadFieldINT(pStruct, const_cast<char*>("SpellId"), bSuccess, 0);
    m_casterId = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("CasterId"), bSuccess, 0);
    m_targetId = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("TargetId"), bSuccess, 0);
    m_areaId = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("AreaId"), bSuccess, 0);
    m_itemId = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("ItemId"), bSuccess, 0);
    {
        CExoString sScriptDef("");
        CExoString sScript = pGff->ReadFieldCExoString(pStruct, const_cast<char*>("Script"), bSuccess, sScriptDef);
        m_sScript = sScript;
    }
    m_targetPosition.x = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("TargetPosX"), bSuccess, 0.0f);
    m_targetPosition.y = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("TargetPosY"), bSuccess, 0.0f);
    m_targetPosition.z = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("TargetPosZ"), bSuccess, 0.0f);
#if !KOTOR1  // K1: no FinalForceCost
    m_nFinalForceCost = pGff->ReadFieldINT(pStruct, const_cast<char*>("FinalForceCost"), bSuccess, 0);
#endif
    return true;
}

int CSWSCreature::LoadCreature(CResGFF* pGff, CResStruct* pStruct, int bFromSaveGame)
{
    int bSuccess;

    m_cachedAreaId = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("AreaId"), bSuccess, 0);

    if (m_pStats->ReadStatsFromGff(pGff, pStruct, &m_appearance) != 0)
        return 0;

    pGff->ReadFieldBYTE(pStruct, const_cast<char*>("DetectMode"), bSuccess, 0);
    SetActivity(ACTIVITY_DETECT, 1, 0);
    SetActivity(ACTIVITY_STEALTH, pGff->ReadFieldBYTE(pStruct, const_cast<char*>("StealthMode"), bSuccess, 0), 0);

    m_nCreatureSize        = pGff->ReadFieldINT(pStruct, const_cast<char*>("CreatureSize"), bSuccess, 3);
    m_bDestroyable         = pGff->ReadFieldBYTE(pStruct, const_cast<char*>("IsDestroyable"), bSuccess, 1);
    m_bRaiseable           = pGff->ReadFieldBYTE(pStruct, const_cast<char*>("IsRaiseable"), bSuccess, 1);
    m_bDeadSelectable      = pGff->ReadFieldBYTE(pStruct, const_cast<char*>("DeadSelectable"), bSuccess, 1);
#if !KOTOR1  // K1: no BonusForcePoints / AssignedPup / PlayerCreated
    m_nBonusForcePoints    = pGff->ReadFieldINT(pStruct, const_cast<char*>("BonusForcePoints"), bSuccess, 7);
    m_nAssignedPup         = pGff->ReadFieldINT(pStruct, const_cast<char*>("AssignedPup"), bSuccess, -1);
    m_bPlayerMadeCharacter = pGff->ReadFieldINT(pStruct, const_cast<char*>("PlayerCreated"), bSuccess, 0);
#endif

    // The two "idle/talk" ambient states are remapped to 14 for three specific animation ids:
    // the creature is already playing an animation those states would fight with.
    unsigned char nAmbientState = pGff->ReadFieldBYTE(pStruct, const_cast<char*>("AmbientAnimState"), bSuccess, 0);
    if ((nAmbientState == 3 || nAmbientState == 4)
        && (m_nAnimation == SERVER_ANIMATION_WHIRLWIND || m_nAnimation == SERVER_ANIMATION_PRONE || m_nAnimation == SERVER_ANIMATION_KNOCKED_DOWN_LOOP))
        nAmbientState = 14;
    m_ambientAnimationState = nAmbientState;

    SetAnimation(pGff->ReadFieldINT(pStruct, const_cast<char*>("Animation"), bSuccess, 10000));

    // A save game that predates the field (bSuccess == 0) still counts as "spawn script already
    // fired" when we are loading a save rather than instantiating a template.
    unsigned char bScriptFired = pGff->ReadFieldBYTE(pStruct, const_cast<char*>("CreatnScrptFird"), bSuccess, 0);
    if (bSuccess == 1)
        m_bSpawnPerceptionDone = bScriptFired;
    else if (bFromSaveGame == 1)
        m_bSpawnPerceptionDone = 1;

    ReadScriptsFromGff(pGff, pStruct);
    ReadItemsFromGff(pGff, pStruct, 1, bFromSaveGame == 0, 0);
    m_pStats->ReadSpellsFromGff(pGff, pStruct);

    m_bDisguised = pGff->ReadFieldBYTE(pStruct, const_cast<char*>("PM_IsDisguised"), bSuccess, 0);
    if (m_bDisguised != 0)
        m_nDisguiseAppearance = pGff->ReadFieldWORD(pStruct, const_cast<char*>("PM_Appearance"), bSuccess, 0);

    m_bListening = pGff->ReadFieldBYTE(pStruct, const_cast<char*>("Listening"), bSuccess, 0);
    LoadListenData(pGff, pStruct);
#if !KOTOR1  // K1: no ForceAlwaysUpdate
    m_bForceAlwaysUpdate = pGff->ReadFieldBYTE(pStruct, const_cast<char*>("ForceAlwaysUpdate"), bSuccess, 0);
#endif

    PostProcess();
    LoadFollowInfo(pGff, pStruct);
    LoadObjectState(pGff, pStruct);
    return 1;
}

int CSWSCreature::SaveCreature(CResGFF* pGff, CResStruct* pStruct)
{
    int bWasInParty = m_bInParty;
    SetInParty(0, 1);

    m_pStats->SaveStats(pGff, pStruct);

    pGff->WriteFieldBYTE(pStruct, m_detectMode, const_cast<char*>("DetectMode"));
    pGff->WriteFieldBYTE(pStruct, m_stealthMode, const_cast<char*>("StealthMode"));
    pGff->WriteFieldINT(pStruct, m_nCreatureSize, const_cast<char*>("CreatureSize"));
    pGff->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bDestroyable), const_cast<char*>("IsDestroyable"));
    pGff->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bRaiseable), const_cast<char*>("IsRaiseable"));
    pGff->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bDeadSelectable), const_cast<char*>("DeadSelectable"));

    // The 14 per-event scripts, in CREATURE_SCRIPT_* order, which is the GFF key order
    // ReadScriptsFromGff reads them back in.
    pGff->WriteFieldCResRef(pStruct, CResRef(m_scripts[CREATURE_SCRIPT_HEARTBEAT]), const_cast<char*>("ScriptHeartbeat"));
    pGff->WriteFieldCResRef(pStruct, CResRef(m_scripts[CREATURE_SCRIPT_ONNOTICE]), const_cast<char*>("ScriptOnNotice"));
    pGff->WriteFieldCResRef(pStruct, CResRef(m_scripts[CREATURE_SCRIPT_SPELLAT]), const_cast<char*>("ScriptSpellAt"));
    pGff->WriteFieldCResRef(pStruct, CResRef(m_scripts[CREATURE_SCRIPT_ATTACKED]), const_cast<char*>("ScriptAttacked"));
    pGff->WriteFieldCResRef(pStruct, CResRef(m_scripts[CREATURE_SCRIPT_DAMAGED]), const_cast<char*>("ScriptDamaged"));
    pGff->WriteFieldCResRef(pStruct, CResRef(m_scripts[CREATURE_SCRIPT_DISTURBED]), const_cast<char*>("ScriptDisturbed"));
    pGff->WriteFieldCResRef(pStruct, CResRef(m_scripts[CREATURE_SCRIPT_ENDROUND]), const_cast<char*>("ScriptEndRound"));
    pGff->WriteFieldCResRef(pStruct, CResRef(m_scripts[CREATURE_SCRIPT_DIALOGUE]), const_cast<char*>("ScriptDialogue"));
    pGff->WriteFieldCResRef(pStruct, CResRef(m_scripts[CREATURE_SCRIPT_SPAWN]), const_cast<char*>("ScriptSpawn"));
    pGff->WriteFieldCResRef(pStruct, CResRef(m_scripts[CREATURE_SCRIPT_RESTED]), const_cast<char*>("ScriptRested"));
    pGff->WriteFieldCResRef(pStruct, CResRef(m_scripts[CREATURE_SCRIPT_DEATH]), const_cast<char*>("ScriptDeath"));
    pGff->WriteFieldCResRef(pStruct, CResRef(m_scripts[CREATURE_SCRIPT_USERDEFINE]), const_cast<char*>("ScriptUserDefine"));
    pGff->WriteFieldCResRef(pStruct, CResRef(m_scripts[CREATURE_SCRIPT_ONBLOCKED]), const_cast<char*>("ScriptOnBlocked"));
    pGff->WriteFieldCResRef(pStruct, CResRef(m_scripts[CREATURE_SCRIPT_ENDDIALOGUE]), const_cast<char*>("ScriptEndDialogue"));

    CResList   itemList;
    CResStruct itemStruct;

    // For the equipped items, the 20 equipment slots are a bitmask, one bit per slot, and the slot
    // mask (not the loop index) is what goes into the list element's type field.
    pGff->AddList(&itemList, pStruct, const_cast<char*>("Equip_ItemList"));
    unsigned int nSlot = 1;
#if KOTOR1  // K1: 18 equipment slots
    for (int i = 0; i < 18; i++)
#else
    for (int i = 0; i < 20; i++)
#endif
    {
        CSWSItem* pItem = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(nSlot));
        if (pItem != 0)
        {
            pGff->AddListElement(&itemStruct, &itemList, nSlot);
            pGff->WriteFieldDWORD(&itemStruct, static_cast<unsigned int>(pItem->m_objectId), const_cast<char*>("ObjectId"));
            pItem->SaveItem(pGff, &itemStruct);
        }
        nSlot = nSlot + nSlot;
    }

    // Carried items. GetItemRepository(1) is re-called in the condition and in the body: the
    // repository the creature saves out of switches to the shared party inventory while it is a
    // party member, so it is never cached across the SaveItem call.
    pGff->AddList(&itemList, pStruct, const_cast<char*>("ItemList"));
    unsigned int nItem = 0;
    while (nItem < GetItemRepository(1)->ItemListGetCount())
    {
        CSWSItem* pItem = GetItemRepository(1)->ItemListGetItem(nItem);
        pGff->AddListElement(&itemStruct, &itemList, 0);
        pGff->WriteFieldDWORD(&itemStruct, static_cast<unsigned int>(pItem->m_objectId), const_cast<char*>("ObjectId"));
        pItem->SaveItem(pGff, &itemStruct);
        nItem++;
    }

    // Who the creature currently perceives. NULL slots in the visibility list are skipped without
    // emitting an element, and the saved byte repacks the live flags into the persisted layout:
    // the low nibble verbatim plus bit 4 folded down into bit 1.
    CResList   perceptionList;
    CResStruct perceptionStruct;
    pGff->AddList(&perceptionList, pStruct, const_cast<char*>("PerceptionList"));
    for (int64_t i = 0; i < m_visibilityList.m_count; i++)
    {
        CSWVisibilityNode* pNode = m_visibilityList.m_pArray[i];
        if (pNode != 0)
        {
            pGff->AddListElement(&perceptionStruct, &perceptionList, 0);
            pGff->WriteFieldDWORD(&perceptionStruct, static_cast<unsigned int>(pNode->m_objectId), const_cast<char*>("ObjectId"));
            unsigned char flags = pNode->m_flags;
            pGff->WriteFieldBYTE(&perceptionStruct,
                                 (flags & (VISIBILITY_SEEN | VISIBILITY_HEARD | VISIBILITY_PACIFY_STATE_MASK)) | ((flags >> 3) & 2),
                                 const_cast<char*>("PerceptionData"));
        }
    }

    // The combat-round struct is added unconditionally (so the reader always finds it), but only
    // filled in while the round is actually live.
    CResStruct combatStruct;
    pGff->AddStructToStruct(&combatStruct, pStruct, const_cast<char*>("CombatRoundData"), GFF_STRUCT_COMBAT_ROUND);
    if (m_pCombatRound->m_bRoundStarted == 1)
        m_pCombatRound->SaveCombatRound(pGff, &combatStruct);

    pGff->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_cachedAreaId), const_cast<char*>("AreaId"));
    pGff->WriteFieldBYTE(pStruct, m_ambientAnimationState, const_cast<char*>("AmbientAnimState"));
    pGff->WriteFieldINT(pStruct, m_nAnimation, const_cast<char*>("Animation"));
    pGff->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bSpawnPerceptionDone), const_cast<char*>("CreatnScrptFird"));
    pGff->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bDisguised), const_cast<char*>("PM_IsDisguised"));
    if (m_bDisguised == 1)
        pGff->WriteFieldWORD(pStruct, m_nDisguiseAppearance, const_cast<char*>("PM_Appearance"));
    pGff->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bListening), const_cast<char*>("Listening"));
    SaveListenData(pGff, pStruct);
#if !KOTOR1
    pGff->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bForceAlwaysUpdate), const_cast<char*>("ForceAlwaysUpdate"));
#endif
    pGff->WriteFieldFLOAT(pStruct, m_position.x, const_cast<char*>("XPosition"));
    pGff->WriteFieldFLOAT(pStruct, m_position.y, const_cast<char*>("YPosition"));
    pGff->WriteFieldFLOAT(pStruct, m_position.z, const_cast<char*>("ZPosition"));
    pGff->WriteFieldFLOAT(pStruct, m_orientation.x, const_cast<char*>("XOrientation"));
    pGff->WriteFieldFLOAT(pStruct, m_orientation.y, const_cast<char*>("YOrientation"));
    pGff->WriteFieldFLOAT(pStruct, m_orientation.z, const_cast<char*>("ZOrientation"));
    pGff->WriteFieldINT(pStruct, m_nJoiningXP, const_cast<char*>("JoiningXP"));
#if !KOTOR1
    pGff->WriteFieldINT(pStruct, m_nBonusForcePoints, const_cast<char*>("BonusForcePoints"));
    pGff->WriteFieldINT(pStruct, m_nAssignedPup, const_cast<char*>("AssignedPup"));
    pGff->WriteFieldINT(pStruct, m_bPlayerMadeCharacter, const_cast<char*>("PlayerCreated"));
#endif

    SetInParty(bWasInParty, 1);
    SaveFollowInfo(pGff, pStruct);
    SaveObjectState(pGff, pStruct);
    return 1;
}

void CSWSCreature::SaveFollowInfo(CResGFF* pGff, CResStruct* pStruct)
{
    if (m_pFollowInfo != 0) {
        CResStruct resStruct;
        pGff->AddStructToStruct(&resStruct, pStruct, const_cast<char*>("FollowInfo"), 0);
        m_pFollowInfo->Save(pGff, &resStruct);
    }
}

float CSWSCreature::GetRunRate()
{
    float fRate = m_pStats->m_fRunRate * 1000.0f * GetMovementRateFactor(1);
    if (fRate < 1000.0f)
        fRate = 1000.0f;
    return fRate;
}

int CSWSCreature::IsPartyVisible()
{
#if KOTOR1  // K1: a discarded client-party fetch
    g_pAppManager->m_pClientExoApp->GetSWParty();
#endif
    for (int64_t i = 0; i < m_visibilityList.m_count; i++) {
        CSWVisibilityNode* pNode = m_visibilityList.m_pArray[i];
        if ((pNode->m_flags & VISIBILITY_SEEN) == 0)
            continue;
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pNode->m_objectId);
        if (pCreature != 0 && pCreature->m_bInParty != 0)
            return 1;
    }
    return 0;
}

int CSWSCreature::RunEquip(unsigned long long objectId, unsigned int nSlot, int a)
{
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(objectId);
    if (pItem == 0)
    {
        if (pPlayer != 0 && pMessage != 0)
            pMessage->SendServerToPlayerInventory_EquipCancel(pPlayer->m_nPlayerId, objectId,
                                                              nSlot);
        return 0;
    }

    CItemRepository* pRepository;
    CSWSItem* pSwap;
    if (pItem->m_possessorId == m_objectId)
    {
        pRepository = GetItemRepository(1);
    }
    else
    {
        CSWSItem* pContainer =
            g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pItem->m_possessorId);
        if (pContainer != 0 && pContainer->m_possessorId == m_objectId)
        {
            pRepository = pContainer->m_pItemRepository;
        }
        else
        {
            CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
            CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
            CSWParty* pParty = pClient->GetSWParty();
            if (pParty->IsPartyMember(pClient->ServerToClientObjectId(m_objectId)) == 0
                && pPartyTable->GetIsAvailable(m_objectId) == 0)
                goto cancel;

            if (pParty->IsPartyMember(pClient->ServerToClientObjectId(pItem->m_possessorId)) != 0
                || pPartyTable->GetIsAvailable(pItem->m_possessorId) != 0)
            {
                pRepository = GetItemRepository(1);
            }
            else
            {
                CSWSItem* pOwner =
                    g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pItem->m_possessorId);
                if (pOwner == 0 || pPartyTable->GetIsAvailable(pOwner->m_possessorId) == 0)
                    goto cancel;
                pRepository = pOwner->m_pItemRepository;
            }
        }
    }

    switch (CanEquipItem(pItem, &nSlot, 1, 0, 1))
    {
    case CAN_EQUIP_YES:
    {
        // This body is deliberately a duplicate of the post-switch one.
        if (pItem->m_stackCount >= 2)
            pItem = pItem->SplitItem(1);
        if (pRepository->GetItemInRepository(pItem, 0) == 1)
            pRepository->RemoveItem(pItem);
        if (pItem->m_possessorId != m_objectId)
            pItem->SetPossessor(m_objectId, 0, 0, OBJECT_INVALID);
        EquipItem(nSlot, pItem, 1, 0, a);
        unsigned int nEquipped = m_pInventory->GetSlotFromItem(pItem);
        if (nEquipped == CSWInventory::SLOT_LEFTWEAPON)
            m_bLastInstantEquipLeft = a;
        else if (nEquipped == CSWInventory::SLOT_RIGHTWEAPON)
            m_bLastInstantEquipRight = a;
        goto equipped;
    }
    case CAN_EQUIP_SWAP:
    {
        pSwap = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(nSlot));
        if (pSwap == 0)
            goto refuse;
        if (g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem)->m_nStacking >= 2
            && pSwap->CompareItem(pItem) != 0)
            goto refuse;
        UnequipItem(pSwap);
        if (GetItemRepository(1)->AddItem(&pSwap, 1, 0, 0) == 0)
        {
            RemoveItem(pSwap, 0, 1, 0);
            pSwap->AddToArea(static_cast<CSWSArea*>(GetArea()), m_position.x, m_position.y,
                             m_position.z + 0.2f, 1);
        }
        break;
    }
    case CAN_EQUIP_CLEAR_WEAPONS:
    {
        int bUnequipped = 0;
        pSwap = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
        if (pSwap != 0)
        {
            UnequipItem(pSwap);
            bUnequipped = 1;
            if (GetItemRepository(1)->AddItem(&pSwap, 1, 0, 0) == 0)
            {
                RemoveItem(pSwap, 0, 1, 0);
                pSwap->AddToArea(static_cast<CSWSArea*>(GetArea()), m_position.x, m_position.y,
                                 m_position.z + 0.2f, 1);
            }
        }
        pSwap = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
        if (pSwap == 0)
        {
            if (bUnequipped == 0)
                goto refuse;
            break;
        }
        UnequipItem(pSwap);
        if (GetItemRepository(1)->AddItem(&pSwap, 1, 0, 0) == 0)
        {
            RemoveItem(pSwap, 0, 1, 0);
            pSwap->AddToArea(static_cast<CSWSArea*>(GetArea()), m_position.x, m_position.y,
                             m_position.z + 0.2f, 1);
        }
        break;
    }
#if !KOTOR1  // K1: no alternate weapon set
    case CAN_EQUIP_CLEAR_WEAPONS2:
    {
        int bUnequipped = 0;
        pSwap = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON2));
        if (pSwap != 0)
        {
            UnequipItem(pSwap);
            bUnequipped = 1;
            if (GetItemRepository(1)->AddItem(&pSwap, 1, 0, 0) == 0)
            {
                RemoveItem(pSwap, 0, 1, 0);
                pSwap->AddToArea(static_cast<CSWSArea*>(GetArea()), m_position.x, m_position.y,
                                 m_position.z + 0.2f, 1);
            }
        }
        pSwap = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON2));
        if (pSwap == 0)
        {
            if (bUnequipped == 0)
                goto refuse;
            break;
        }
        UnequipItem(pSwap);
        if (GetItemRepository(1)->AddItem(&pSwap, 1, 0, 0) == 0)
        {
            RemoveItem(pSwap, 0, 1, 0);
            pSwap->AddToArea(static_cast<CSWSArea*>(GetArea()), m_position.x, m_position.y,
                             m_position.z + 0.2f, 1);
        }
        break;
    }
#endif
    default:
        goto refuse;
    }

    if (pItem->m_stackCount >= 2)
        pItem = pItem->SplitItem(1);
    if (pRepository->GetItemInRepository(pItem, 0) == 1)
        pRepository->RemoveItem(pItem);
    if (pItem->m_possessorId != m_objectId)
        pItem->SetPossessor(m_objectId, 0, 0, OBJECT_INVALID);
    EquipItem(nSlot, pItem, 1, 0, 0);
    {
        unsigned int nEquipped = m_pInventory->GetSlotFromItem(pItem);
        if (nEquipped == CSWInventory::SLOT_LEFTWEAPON)
            m_bLastInstantEquipLeft = a;
        else if (nEquipped == CSWInventory::SLOT_RIGHTWEAPON)
            m_bLastInstantEquipRight = a;
    }

equipped:
    {
        CSWSItem* pRight =
                static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
        CSWSItem* pLeft = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
        unsigned char nMode = m_combatMode;
        if (nMode == CREATURE_COMBAT_MODE_POWER_ATTACK ||
            nMode == CREATURE_COMBAT_MODE_IMPROVED_POWER_ATTACK)
        {
            if (pRight != 0
                && g_pRules->m_pBaseItemArray->GetBaseItem(pRight->m_baseItem)->m_bRangedWeapon
                       != 0)
                SetCombatMode(CREATURE_COMBAT_MODE_NONE, 1);
        }
        else if (nMode == CREATURE_COMBAT_MODE_RAPID_SHOT)
        {
            if (pRight == 0
                || g_pRules->m_pBaseItemArray->GetBaseItem(pRight->m_baseItem)->m_bRangedWeapon
                       == 0)
                SetCombatMode(CREATURE_COMBAT_MODE_NONE, 1);
        }
        else if (nMode == CREATURE_COMBAT_MODE_FLURRY)
        {
            if (pRight != 0 || pLeft != 0)
                SetCombatMode(CREATURE_COMBAT_MODE_NONE, 1);
        }
        else if (nMode == CREATURE_COMBAT_MODE_PARRY)
        {
            if (pRight != 0
                && g_pRules->m_pBaseItemArray->GetBaseItem(pRight->m_baseItem)->m_bRangedWeapon
                       != 0)
                SetCombatMode(CREATURE_COMBAT_MODE_NONE, 1);
        }
        return 1;
    }

cancel:
    if (pPlayer != 0 && pMessage != 0)
        pMessage->SendServerToPlayerInventory_EquipCancel(pPlayer->m_nPlayerId, objectId, nSlot);
    return 0;

refuse:
    if (pPlayer != 0 && pMessage != 0)
        pMessage->SendServerToPlayerInventory_EquipCancel(pPlayer->m_nPlayerId, objectId, nSlot);
    return 0;
}

CItemRepository* CSWSCreature::GetItemRepository(int flag)
{
#if KOTOR1  // K1: a non-member shares the party items only in the intro module with the party-access panel up
    if (flag != 0) {
        if (m_bInParty != 0)
            return g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems();
        if (g_pAppManager->m_pServerExoApp->GetServerInfo()->m_bTutorialModule &&
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsAvailable(m_objectId) &&
            g_pAppManager->m_pClientExoApp->GetInGameGui()->GetPartyAccessPanelUp(0))
            return g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems();
    }
    return m_pItemRepository;
#else
    int nOverride = g_pAppManager->m_pClientExoApp->GetInGameGui()->GetTopMenuOverrideCharacter();
    int bPartyItems = (m_bInParty != 0) & (flag != 0);
    if (nOverride == -1 && !bPartyItems)
        return m_pItemRepository;
    return g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems();
#endif
}

void CSWSCreature::BroadcastSkillData(CSWCCMessageData* pData)
{
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
    if (pPlayer != 0)
        g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerCCMessage(
            pPlayer->m_nPlayerId, CC_MESSAGE_SKILL, pData, 0);
    if (pData != 0)
        delete pData;
}

void CSWSCreature::StartGuiTimingBar(unsigned int nType, unsigned char a)
{
    if (m_bPlayerCharacter == 1) {
        CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
        CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
        if (pPlayer != 0 && pMessage != 0)
            pMessage->SendServerToPlayerGuiTimingEvent(pPlayer, 1, a, nType);
    }
}

void CSWSCreature::RemoveSleepEffects()
{
    for (int i = 0; i < m_effects.m_count; i++)
    {
        CGameEffect* pEffect = m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_SETSTATE)
            return;
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_SETSTATE
            && pEffect->GetInteger(0) == EFFECT_STATE_SLEEP)
        {
            RemoveEffectById(pEffect->m_effectId);
        }
    }
}

// Resets the combat round, then, unless the creature is dead, dying or debilitated, or a round
// action is pending, plays the client's pause-ready animation, or the 10000/10001 stealth and
// ambient pair.
int CSWSCreature::ResetCombatAnimations(int a)
{
    m_pCombatRound->SetRoundPaused(0, OBJECT_INVALID);
    m_pCombatRound->SetPauseTimer(0, 0);
    m_pCombatRound->IncrementTimer(a);
    m_pCombatRound->SetCurrentAction(0);
    m_bInitiativeExpired = 0;
    if (GetDead() != 0)
        return 0;
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp()
    if (GetIsPCDying() || GetDeadTemp())
        return 0;
#else
    if (m_bInParty != 0 && GetCurrentHitPoints(0) < 1)
        return 0;
#endif
    if (!AnimationStationary(static_cast<unsigned short>(m_nAnimation)))
        return 0;
    if (IsDebilitated(1))
        return 0;
    if (m_pCombatRound->m_bCutsceneAttack != 0)
        return 0;
    int nAnimation;
    CSWCCreature* pCCreature = 0;
    if (GetClientObject() != 0)
        pCCreature = static_cast<CSWCCreature*>(GetClientObject()->AsSWCCreature());
    if (pCCreature != 0) {
        nAnimation = pCCreature->GetPauseReadyAnimation();
    } else {
        nAnimation = m_bCombatState != 0 ? SERVER_ANIMATION_READY : SERVER_ANIMATION_PAUSE;
    }
    SetAnimation(nAnimation);
    return 1;
}

int CSWSCreature::GetIsAlignmentRestricted(unsigned char nClassSlot, unsigned char nAlignType)
{
    if (m_pStats->m_numClasses <= nClassSlot)
        return 0xff;
#if KOTOR1  // K1: through the out-of-line GetClass
    CSWRules* pRules = g_pRules;
    unsigned char nClass = m_pStats->GetClass(nClassSlot);
    CSWClass* pClass = &pRules->m_paClasses[nClass];
#else
    unsigned char nClass = m_pStats->m_classInfo[nClassSlot].m_class;
    CSWClass* pClass = &g_pRules->m_paClasses[nClass];
#endif
    if (pClass != 0) {
        unsigned char bRestrict = pClass->m_alignRestrict;
        if (nAlignType == 1) {
            if ((bRestrict & ALIGN_RESTRICT_NEUTRAL) != 0)
                return 1;
        } else if (nAlignType == 2) {
            if ((bRestrict & ALIGN_RESTRICT_GOOD) != 0)
                return 1;
        } else if (nAlignType == 3) {
            if ((bRestrict & ALIGN_RESTRICT_EVIL) != 0)
                return 1;
        }
    }
    return 0;
}

// Applies incoming damage from the creature's side, in order:
// - the difficulty 2DA's multiplier column, for party members and the switched-in PC only
// - the g_nIncreaseDamage cheat, for non-party creatures and only from 2 up
// - the temporary-hit-point effect pool
// - the "party members survive on the easiest setting" floor
//
// What survives goes to CSWSObject::DoDamage. m_hitPointBonus is read through a truncating `(short)`
// but written as a full int, and the `short nBonus` local reproduces both. The effect scan starts at
// m_pStats->m_iFirstTempHitPoints and stops once the sorted subType passes
// EFFECT_TEMPORARY_HITPOINTS.
void CSWSCreature::DoDamage(int nDamage, int bSuppressFloatyText)
{
    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    if ((pPartyTable != 0 && pPartyTable->GetIsMember(m_objectId) != 0) || m_bPlayerCharacter != 0) {
        unsigned char nDifficulty =
            g_pAppManager->m_pClientExoApp->GetClientOptions()->m_difficultyLevel;
        float fMultiplier;
        if (g_pRules->m_p2DArrays->m_pDifficultyOptions->GetFLOATEntry(
                nDifficulty, CExoString("MULTIPLIER"), &fMultiplier) != 0)
            nDamage = static_cast<int>(nDamage * fMultiplier);
    }
    if (m_bInParty == 0 && g_nIncreaseDamage >= 2)
        nDamage = nDamage * g_nIncreaseDamage;
    if (m_nHitPointBonus16 > 0) {
        CSWSCreature* pCreature = AsSWSCreature();
        int nRemaining = 0;
        if (pCreature->m_effects.m_count > 0) {
            for (int i = m_pStats->m_iFirstTempHitPoints; i < pCreature->m_effects.m_count; i++) {
                CGameEffect* pEffect = pCreature->m_effects.m_pArray[i];
                if (CSWSEffectListHandler::EFFECT_TEMPORARY_HITPOINTS < pEffect->m_subType)
                    break;
                if (CSWSEffectListHandler::EFFECT_TEMPORARY_HITPOINTS == pEffect->m_subType) {
                    int nPool = pEffect->GetInteger(0);
                    if (nPool > nDamage) {
                        int nLeft = nPool - nDamage;
                        pEffect->SetInteger(0, nLeft);
                        nDamage = 0;
                        nRemaining = nRemaining + nLeft;
                    } else {
                        nDamage = nDamage - nPool;
                        AsSWSCreature()->RemoveEffectById(pEffect->m_effectId);
                    }
                }
            }
        }
        m_hitPointBonus = nRemaining;
    }
    if (g_pAppManager->m_pServerExoApp->GetDifficultyOption(5) == 1 && m_bPlayerCharacter == 1
            && static_cast<short>(GetCurrentHitPoints(1)) - nDamage <= 0) {
        nDamage = static_cast<short>(GetCurrentHitPoints(1)) - 1;
        if (nDamage < 0)
            nDamage = 0;
    }
    CClientExoApp* pClientExoApp = g_pAppManager->m_pClientExoApp;
    unsigned long long oidClient = pClientExoApp->ServerToClientObjectId(m_objectId);
    if (pClientExoApp->GetPlayerCreatureId() == oidClient && bSuppressFloatyText == 0)
        pClientExoApp->AddFloatyText(oidClient, CClientExoApp::FLOATY_DAMAGE, nDamage);
    CSWSObject::DoDamage(nDamage);
}

void CSWSCreature::BroadcastCombatStateToParty(unsigned long long oidAttacker)
{
    unsigned long long oidAttackTarget = OBJECT_INVALID;
    unsigned long long oidAttemptedSpellTarget = OBJECT_INVALID;
    CSWSCreature* pAttacker =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidAttacker);
    if (pAttacker != 0) {
        oidAttackTarget = pAttacker->m_attemptedAttackTargetId;
        oidAttemptedSpellTarget = pAttacker->m_oidAttemptedSpellTarget;
    }
    if (GetReputation(oidAttacker, 0) < 11 || m_objectId == oidAttacker) {
#if KOTOR1
        SetCombatState(1,
                       (oidAttackTarget == m_objectId || oidAttemptedSpellTarget == m_objectId) ? 1 : 2);
#else
        SetCombatState(1,
                       (oidAttackTarget == m_objectId || oidAttemptedSpellTarget == m_objectId) ? 1 : 2, 0);
#endif
    }
    if (m_bPlot != 0)
        return;
    unsigned long long* paMembers;
    GetFaction()->GetMemberList(&paMembers);
    for (int i = 0; i < GetFaction()->m_members.m_count; i++) {
        CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(paMembers[i]);
        if (pObject != 0 && pObject->AsSWSObject() != 0) {
            CSWSObject* pMember = pObject->AsSWSObject();
            Vector vDelta = m_position - pMember->m_position;
            float fRange = 0.0f;
            if (m_bInParty != 0) {
                fRange = 30.0f;
            } else if (pMember->AsSWSCreature() != 0) {
                fRange = pMember->AsSWSCreature()->GetSpotCheckDistance();
            } else {
                g_pRules->m_p2DArrays->m_pRanges->GetFLOATEntry(
                    RANGES_ROW_PERCEPTION_DEFAULT, CExoString("PrimaryRange"), &fRange);
            }
#if KOTOR1  // K1: no ForceAlwaysUpdate override
            if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z
                        <= fRange * fRange) {
#else
            if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z
                        <= fRange * fRange
                    || m_bForceAlwaysUpdate != 0) {
#endif
                if (pMember->AsSWSCreature()->GetReputation(oidAttacker, 0) <= 10) {
                    unsigned long long oidMember = pMember->m_objectId;
#if KOTOR1
                    pMember->AsSWSCreature()->SetCombatState(
                        1, (oidAttackTarget == oidMember || oidAttemptedSpellTarget == oidMember) ? 1 : 2);
#else
                    pMember->AsSWSCreature()->SetCombatState(
                        1, (oidAttackTarget == oidMember || oidAttemptedSpellTarget == oidMember) ? 1 : 2, 0);
#endif
                    pMember->AsSWSCreature()->SetExcitedState(1);
                }
            }
        }
    }
}

void CSWSCreature::ActionManager(unsigned int nAction)
{
    if (g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId) != 0) {
        ComputeAIState();
        if (nAction - 1 <= 1 && m_nAIStateAction <= 9)
            ClearAllActions(1);
        if (nAction == 4 || nAction == 8)
            ClearAllActions(1);
    }
}

void CSWSCreature::UpdateExcitedStateTimer()
{
    if (m_nExcitedStateTimer != 0) {
        if (static_cast<unsigned int>(m_nExcitedStateTimer) > m_nAIUpdateDeltaMs) {
            m_nExcitedStateTimer = m_nExcitedStateTimer - m_nAIUpdateDeltaMs;
        } else {
            m_nExcitedStateTimer = 0;
            CSWSPlayer* pPlayer =
                g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
            CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
            if (pPlayer != 0 && pArea != 0
                    && static_cast<CSWSArea*>(GetArea())->m_pAmbientSound != 0) {
                static_cast<CSWSArea*>(GetArea())->m_pAmbientSound->PlayBattleMusic(0);
            }
        }
    }
}

int CSWSCreature::CalculateSpellSaveDC(int nSpell)
{
    int nDC = m_pStats->GetLevel(1) + static_cast<signed char>(m_pStats->m_nCHAMod) + m_pStats->m_nWISMod;
#if KOTOR1  // K1: no +10 for spells; the three feats add 4 / 2 / 1
    K2_UNUSED(nSpell);
    nDC += 5;
    int nBonus = 0;
    if (m_pStats->HasFeat(FEAT_FORCE_FOCUS_MASTERY) != 0)
        nBonus = 4;
    else if (m_pStats->HasFeat(FEAT_FORCE_FOCUS_ADVANCED) != 0)
        nBonus = 2;
    else if (m_pStats->HasFeat(FEAT_FORCE_FOCUS_SENSE) != 0)
        nBonus = 1;
    return nDC + nBonus;
#else
    if (static_cast<unsigned int>(nSpell - FORCE_POWER_FORCE_SCREAM) < 3)
        nDC += 10;
    else
        nDC += 5;
    if (m_pStats->HasFeat(FEAT_FORCE_FOCUS_MASTERY) != 0)
        return nDC + 4;
    if (m_pStats->HasFeat(FEAT_FORCE_FOCUS_ADVANCED) != 0)
        return nDC + 3;
    if (m_pStats->HasFeat(FEAT_FORCE_FOCUS_SENSE) != 0)
        return nDC + 2;
    return nDC;
#endif
}

void CSWSCreature::RemoveFromArea(int a)
{
    if (GetArea() != 0)
        static_cast<CSWSArea*>(GetArea())->RemoveObjectFromArea(m_objectId);

    if (a == 0) {
        ClearVisibleList();
        if (GetArea() != 0) {
            CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
            CServerAIMaster* pAIMaster = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
            for (int i = 0; i < pArea->m_aObjectsByX.m_count; i++) {
                CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->
                    GetCreatureByGameObjectID(pArea->m_aObjectsByX.m_pArray[i]);
                if (pCreature != 0) {
                    CSWVisibilityNode* pNode = pCreature->GetVisibleListElement(m_objectId);
                    if (pNode != 0) {
                        if ((pNode->m_flags & VISIBILITY_HEARD) != 0) {
                            CScriptEvent* pEvent = new CScriptEvent();
                            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PERCEPTION;
                            pEvent->SetInteger(0, PERCEPTION_EVENT_INAUDIBLE);
                            pEvent->SetObjectID(0, m_objectId);
                            pAIMaster->AddEventDeltaTime(0, 0, m_objectId, pCreature->m_objectId,
                                                         CServerAIMaster::EVENT_SIGNAL_EVENT,
                                                         pEvent);
                        }
                        if ((pNode->m_flags & VISIBILITY_SEEN) != 0) {
                            CScriptEvent* pEvent = new CScriptEvent();
                            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PERCEPTION;
                            pEvent->SetInteger(0, PERCEPTION_EVENT_VANISHED);
                            pEvent->SetObjectID(0, m_objectId);
                            pAIMaster->AddEventDeltaTime(0, 0, m_objectId, pCreature->m_objectId,
                                                         CServerAIMaster::EVENT_SIGNAL_EVENT,
                                                         pEvent);
                        }
                        pCreature->RemoveFromVisibleList(m_objectId);
                    }
                }
            }
        }
    }

    // pObject is declared here, not inside the loop.
    CGameObject* pObject;
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    for (int i = 0; i < m_areaOfEffectIds.m_count; i++)
    {
        if (pObjectArray->GetGameObject(m_areaOfEffectIds.m_pArray[i], &pObject) == CGameObjectArray::SUCCESS
            && pObject->AsSWSAreaOfEffectObject() != 0)
        {
            pObject->AsSWSAreaOfEffectObject()->RemoveFromArea();
        }
    }

    SetArea(0);
    Vector vZero = {};
    SetPosition(vZero, 1, 1, 0);
}

void CSWSCreature::RemoveMindTrickEffects()
{
    for (int64_t i = 0; i < m_effects.m_count; i++) {
        CGameEffect* pEffect = m_effects.m_pArray[i];
        if (CSWSEffectListHandler::EFFECT_SETSTATE < pEffect->m_subType)
            return;
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_SETSTATE &&
            (pEffect->GetInteger(0) == EFFECT_STATE_MIND_TRICK || pEffect->GetInteger(0) == EFFECT_STATE_DROID_SCRAMBLE))
            RemoveEffectById(pEffect->m_effectId);
    }
}

void CSWSCreature::DumpToLog()
{
    m_pStats->GetFullName().CStr();
    m_pStats->GetSTRStat();
    m_pStats->GetINTStat();
    m_pStats->GetWISStat();
    m_pStats->GetDEXStat();
    m_pStats->GetCONStat();
    m_pStats->GetCHAStat();
}

int CSWSCreature::GetTotalACEffectBonus()
{
    int nTotal = 0;
    for (int64_t i = 0; i < m_effects.m_count; i++) {
        CGameEffect* pEffect = m_effects.m_pArray[i];
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_AC_INCREASE)
            nTotal += pEffect->GetInteger(1);
        else if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_AC_DECREASE)
            nTotal -= pEffect->GetInteger(1);
    }
    return nTotal;
}

int CSWSCreature::IsInLightsaberCombat(CSWSCreature* pOther, int bCheckEngaged)
{
    if (pOther != 0) {
        CSWSItem* pMyWeapon =
                static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
        CSWSItem* pTheirWeapon =
                static_cast<CSWSItem*>(pOther->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
        if (pMyWeapon != 0) {
            int bMineIsSaber = pMyWeapon->IsLightsaber();
            if (pTheirWeapon != 0 && bMineIsSaber != 0 && pTheirWeapon->IsLightsaber() != 0) {
                if (bCheckEngaged != 0)
                    return m_pCombatRound->ResolveEngagedState(pOther->m_objectId) == 1;
                return 1;
            }
        }
    }
    return 0;
}

void CSWSCreature::CancelCombat(int bEndRound)
{
    ClearAllActions(1);
#if KOTOR1
    SetCombatState(0, 1);
#else
    SetCombatState(0, 1, 0);
#endif
    SetLastHostileActor(OBJECT_INVALID, 1);
    ClearHostileActionsVersus(0);
    m_pCombatRound->EndCombatRound(bEndRound);
    m_pCombatRound->m_oidAttackTarget = OBJECT_INVALID;
    CSWCCreature* pClient = GetClientCreature();
    if (pClient != 0)
        pClient->SetCombatMode(0);
}

void CSWSCreature::AddSpellWithPrerequisites(unsigned int nSpell)
{
    CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(nSpell);
    if (pSpell != 0) {
        int nPrereq = pSpell->m_anPrereq1[0];
        if (nPrereq != -1) {
            int* pPrereq = &pSpell->m_anPrereq1[1];
            do {
                if (m_pStats->HasSpell(0, nPrereq, 1) == 0)
                    AddSpellWithPrerequisites(nPrereq);
                nPrereq = *pPrereq++;
            } while (nPrereq != -1);
        }
        m_pStats->AddKnownSpell(m_pStats->m_numClasses - 1, nSpell);
    }
}

void CSWSCreature::DoPerceptionUpdateOnCreature(CSWSCreature* pTarget)
{
    Vector vMe;
    Vector vTarget;
    Vector vHit;

    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    CServerAIMaster* pAIMaster = pServerApp->GetServerAIMaster();

    unsigned int nDay;
    unsigned int nTimeOfDay;
    g_pAppManager->m_pServerExoApp->GetActiveTimer(OBJECT_INVALID)->GetWorldTime(&nDay, &nTimeOfDay);
    unsigned int nDeltaDay;
    unsigned int nDeltaTod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTimeOfDay, m_nLastSkillRollDay, m_nLastSkillRollTod, &nDeltaDay, &nDeltaTod);
    if (nDeltaTod >= 20000) {
        if (m_stealthMode == 1) {
            m_nStealthHideRoll = rand_wincompatible() % 10 + 11;
            m_nStealthMoveRoll = rand_wincompatible() % 10 + 11;
        }
        unsigned char nModeIndex = m_detectMode - 1;
        int nVehicleState = m_nAnimation;
        int nDie = (nVehicleState == SERVER_ANIMATION_PAUSE || nModeIndex < 2) ? 20 : 10;
        m_nSpotRoll = static_cast<unsigned char>(rand_wincompatible() % nDie + 1);
        m_nListenRoll = static_cast<unsigned char>(rand_wincompatible() % nDie + 1);
        m_nLastSkillRollDay = nDay;
        m_nLastSkillRollTod = nTimeOfDay;
    }

    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (pArea == 0)
        return;
    if (pTarget->m_objectId == m_objectId)
        return;

#if KOTOR1  // K1: the winning distance is fetched again (a max() that re-evaluates)
    float fRange = GetSpotCheckDistance() >= GetListenCheckDistance() ? GetSpotCheckDistance()
                                                                      : GetListenCheckDistance();
#else
    float fSpotRange = GetSpotCheckDistance();
    float fListenRange = GetListenCheckDistance();
    float fRange = fSpotRange >= fListenRange ? fSpotRange : fListenRange;
#endif
    unsigned long long oidTarget = pTarget->m_objectId;
    vMe = m_position;
    vMe.z += 1.5f;
    vTarget = pTarget->m_position;
    vTarget.z += 1.5f;

    int bSeen = 0;
    int bInvisible = 0;
    if (static_cast<CSWSArea*>(pTarget->GetArea()) == pArea) {
        bSeen = 1;
        if (m_objectId != pServerApp->GetPlayerCreatureId()) {
            float dx = vMe.x - vTarget.x;
            float dy = vMe.y - vTarget.y;
            float dz = vMe.z - vTarget.z;
#if KOTOR1  // K1: no ForceAlwaysUpdate override
            if (!(dx * dx + dy * dy + dz * dz >= fRange * fRange)) {
#else
            if (!(dx * dx + dy * dy + dz * dz >= fRange * fRange) ||
                pTarget->m_bForceAlwaysUpdate != 0) {
#endif
                unsigned long long oidHit;
                bSeen = pArea->ClearLineOfSight(vMe, vTarget, &vHit, &oidHit,
                                                m_objectId, oidTarget, 1, 0) == 1;
            } else {
                bSeen = 0;
            }
        }

        // Is any of the module's player creatures standing in this area? A creature nobody can
        // observe skips the (expensive) invisibility query and is simply treated as unseen.
        CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
        int bPlayerInArea = 0;
        for (int64_t i = 0; i < pModule->m_playerList.m_count; i++) {
            CSWSCreature* pPlayer = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                pModule->m_playerList.m_pArray[i]);
#if KOTOR1  // K1: both areas re-fetched per test
            if (pPlayer != 0 && GetArea() != 0 && pPlayer->GetArea() != 0
                && static_cast<CSWSArea*>(GetArea())->m_objectId == static_cast<CSWSArea*>(pPlayer->GetArea())->m_objectId) {
                bPlayerInArea = 1;
                break;
            }
#else
            if (pPlayer != 0) {
                CSWSArea* pPlayerArea = static_cast<CSWSArea*>(pPlayer->GetArea());
                if (pPlayerArea != 0 &&
                    pArea->m_objectId == pPlayerArea->m_objectId) {
                    bPlayerInArea = 1;
                    break;
                }
            }
#endif
        }

        if (GetBlind() != 0)
            bInvisible = 1;
        else if (bPlayerInArea != 0)
            bInvisible = pTarget->GetInvisible(AsSWSObject()) & 1;
        else
            bInvisible = 0;
    }

    int nSpot;
    int nListen;
    int bChanged;
    CSWVisibilityNode* pNode = GetVisibleListElement(oidTarget);
    if (pNode != 0) {
        if (bInvisible != ((pNode->m_flags >> 4) & 1)) {
            if (bInvisible != 0)
                ClearHostileActionsVersus(pTarget);
            pNode->m_flags =
                    static_cast<unsigned char>((pNode->m_flags & (0xff & ~VISIBILITY_INVISIBLE)) | (bInvisible << 4));
        }
        nListen = (pNode->m_flags >> 1) & 1;
        nSpot = pNode->m_flags & 1;
        if (bSeen != 0) {
            bChanged = 0;
            int bNewlyDetected = 0;
            if (nSpot == 0) {
                DoStealthDetection(pTarget, bSeen, &nSpot, &nListen, bInvisible);
                bNewlyDetected = ((pNode->m_flags & VISIBILITY_SEEN) == 0 && nSpot != 0) ||
                                 ((pNode->m_flags & VISIBILITY_HEARD) == 0 && nListen != 0);
            }
            if ((pNode->m_flags & VISIBILITY_SEEN) != 0 && bInvisible != 0) {
                nSpot = 0;
                bChanged = 1;
            }
            if ((pNode->m_flags & VISIBILITY_HEARD) != 0 && pTarget->m_bSilenced != 0) {
                nListen = 0;
                bChanged = 1;
            }
            bSeen = bNewlyDetected;
            if (bInvisible != 0 && (pNode->m_flags & 3) != 0)
                bChanged = 1;
        } else {
            bChanged = 0;
            if ((pNode->m_flags & VISIBILITY_SEEN) != 0) {
                nSpot = 0;
                bChanged = 1;
            }
            DoStealthDetection(pTarget, 0, &nSpot, &nListen, bInvisible);
            if (nListen == 0)
                bChanged = 1;
            bSeen = nListen != 0;
        }
    } else {
        DoStealthDetection(pTarget, bSeen, &nSpot, &nListen, bInvisible);
        if (bSeen != 0)
            bSeen = (nSpot | nListen) != 0;
        else
            bSeen = nListen != 0;
        bChanged = 0;
    }

    // Party members always perceive each other, whatever the rolls said.
    unsigned long long oidClientTarget =
        g_pAppManager->m_pClientExoApp->ServerToClientObjectId(oidTarget);
    unsigned long long oidClientMe =
        g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId);
    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    if (pParty != 0 && pParty->IsPartyMember(oidClientMe) != 0 &&
        pParty->IsPartyMember(oidClientTarget) != 0) {
        nSpot = 1;
        nListen = 1;
        bInvisible = 0;
        bChanged = 0;
        bSeen = 1;
    }

    // Perceiving a mind-tricked-away creature at point-blank range breaks the trick.
#if !KOTOR1  // K1: no mind trick
    if ((nSpot | nListen) != 0 && (m_nState & 0xfe) == EFFECT_STATE_MIND_TRICK &&
        GetReputation(pTarget->m_objectId, 0) <= 10 && pTarget->m_stealthMode != 1) {
        float dx = vMe.x - vTarget.x;
        float dy = vMe.y - vTarget.y;
        float dz = vMe.z - vTarget.z;
        if (dx * dx + dy * dy + dz * dz <= 10.0f)
            RemoveMindTrickEffects();
    }
#endif

    if (bChanged != 0) {
        CSWVisibilityNode* pLost = GetVisibleListElement(oidTarget);
        if (pLost != 0) {
            if (nListen == 0 && (pLost->m_flags & VISIBILITY_HEARD) != 0 && m_bSuppressPerceptionEvents == 0) {
                CScriptEvent* pEvent = new CScriptEvent();
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PERCEPTION;
                pEvent->SetInteger(0, PERCEPTION_EVENT_INAUDIBLE);
                pEvent->SetObjectID(0, oidTarget);
                pAIMaster->AddEventDeltaTime(0, 0, oidTarget, m_objectId,
                                             CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            }
            if (nSpot == 0 && (pLost->m_flags & VISIBILITY_SEEN) != 0 && m_bSuppressPerceptionEvents == 0) {
                CScriptEvent* pEvent = new CScriptEvent();
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PERCEPTION;
                pEvent->SetInteger(0, PERCEPTION_EVENT_VANISHED);
                pEvent->SetObjectID(0, oidTarget);
                pAIMaster->AddEventDeltaTime(0, 0, oidTarget, m_objectId,
                                             CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            }
            pLost->m_flags =
                    static_cast<unsigned char>((pLost->m_flags & (0xff & ~(VISIBILITY_HEARD | VISIBILITY_SEEN))) |
                                             ((nListen & 1) << 1) | (nSpot & 1));
            if ((nListen & 1) == 0 && ((nSpot & 1) | bInvisible) == 0)
                RemoveFromVisibleList(oidTarget);
        }
    }

    if (bSeen == 0)
        return;

    int bNewlySpotted = 0;
    CSWVisibilityNode* pGained = GetVisibleListElement(oidTarget);
    if (pGained != 0) {
        if (nSpot != 0 && (pGained->m_flags & VISIBILITY_SEEN) == 0) {
            pGained->m_flags |= VISIBILITY_SEEN;
            bNewlySpotted = 1;
            if (m_bSuppressPerceptionEvents == 0) {
                CScriptEvent* pEvent = new CScriptEvent();
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PERCEPTION;
                pEvent->SetInteger(0, PERCEPTION_EVENT_SEEN);
                pEvent->SetObjectID(0, oidTarget);
                pAIMaster->AddEventDeltaTime(0, 0, oidTarget, m_objectId,
                                             CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            }
        }
        if (nListen != 0 && (pGained->m_flags & VISIBILITY_HEARD) == 0) {
            pGained->m_flags |= VISIBILITY_HEARD;
            if (m_bSuppressPerceptionEvents == 0) {
                CScriptEvent* pEvent = new CScriptEvent();
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PERCEPTION;
                pEvent->SetInteger(0, PERCEPTION_EVENT_HEARD);
                pEvent->SetObjectID(0, oidTarget);
                pAIMaster->AddEventDeltaTime(0, 0, oidTarget, m_objectId,
                                             CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            }
        }
    } else {
        AddToVisibleList(oidTarget, nSpot, nListen, 0, bInvisible);
        if (nListen != 0 && m_bSuppressPerceptionEvents == 0) {
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PERCEPTION;
            pEvent->SetInteger(0, PERCEPTION_EVENT_HEARD);
            pEvent->SetObjectID(0, oidTarget);
            pAIMaster->AddEventDeltaTime(0, 0, oidTarget, m_objectId,
                                         CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        }
        if (nSpot != 0 && m_bSuppressPerceptionEvents == 0) {
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PERCEPTION;
            pEvent->SetInteger(0, PERCEPTION_EVENT_SEEN);
            pEvent->SetObjectID(0, oidTarget);
            pAIMaster->AddEventDeltaTime(0, 0, oidTarget, m_objectId,
                                         CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            bNewlySpotted = 1;
        }
    }

    // Newly spotting a PC (or the party leader) arms the "someone is watching" timer.
    if (bNewlySpotted != 0 &&
        (pTarget->m_pStats->m_bIsPC != 0 ||
         g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsLeader(pTarget->m_objectId) != 0))
        m_nStealthXPTimer = 6000;
}

// GetItemInSlot takes a slot BIT, but the loop passes the indexes 0..NUM_INVENTORY_SLOTS-1, so only
// the indexes that are themselves slot bits find an item: 1 head, 2 body, 4 boots, 8 hands and
// 16 right weapon. Left as the original has it: a bit-correct loop would also count the inactive
// weapon set and the creature slots, and which slots were meant is unclear.
unsigned int CSWSCreature::GetForceItemMask()
{
    unsigned int nMask = 0;
    for (int i = 0; i < NUM_INVENTORY_SLOTS; i++) {
        CSWItem* pItem = m_pInventory->GetItemInSlot(i);
        if (pItem == 0)
            continue;
        switch (pItem->GetBaseItem()->m_nItemType) {
        case ITEM_TYPE_ARMOR_CLASS_4: nMask |= FORCE_ITEM_ARMOR_CLASS_4;  break;
        case ITEM_TYPE_ARMOR_CLASS_5: nMask |= FORCE_ITEM_ARMOR_CLASS_5;  break;
        case ITEM_TYPE_ARMOR_CLASS_6: nMask |= FORCE_ITEM_ARMOR_CLASS_6;  break;
        case ITEM_TYPE_ARMOR_CLASS_7: nMask |= FORCE_ITEM_ARMOR_CLASS_7;  break;
        case ITEM_TYPE_ARMOR_CLASS_8: nMask |= FORCE_ITEM_ARMOR_CLASS_8; break;
        case ITEM_TYPE_ARMOR_CLASS_9: nMask |= FORCE_ITEM_ARMOR_CLASS_9; break;
        case ITEM_TYPE_DOUBLE_LIGHTSABER:
        case ITEM_TYPE_SHORT_LIGHTSABER:
        case ITEM_TYPE_LIGHTSABER: nMask |= FORCE_ITEM_LIGHTSABER; break;
        }
    }
    return nMask;
}

void CSWSCreature::ClearActivities(int nClearMask)
{
    if ((nClearMask & ACTIVITY_STEALTH) && (m_activityFlags & ACTIVITY_STEALTH)
        && !(m_nLockedActivityFlags & ACTIVITY_STEALTH))
        SetActivity(ACTIVITY_STEALTH, 0, 0);
    if ((nClearMask & ACTIVITY_CONVERSATION) && (m_activityFlags & ACTIVITY_CONVERSATION)
        && !(m_nLockedActivityFlags & ACTIVITY_CONVERSATION))
        SetActivity(ACTIVITY_CONVERSATION, 0, 0);
    if ((nClearMask & ACTIVITY_RESTING) && (m_activityFlags & ACTIVITY_RESTING)
        && !(m_nLockedActivityFlags & ACTIVITY_RESTING))
        SetActivity(ACTIVITY_RESTING, 0, 0);
    if ((nClearMask & 0x10) && (m_activityFlags & 0x10) && !(m_nLockedActivityFlags & 0x10))
        SetActivity(0x10, 0, 0);
    if ((nClearMask & 0x20) && (m_activityFlags & 0x20) && !(m_nLockedActivityFlags & 0x20))
        SetActivity(0x20, 0, 0);
    if ((nClearMask & 0x40) && (m_activityFlags & 0x40) && !(m_nLockedActivityFlags & 0x40))
        SetActivity(0x40, 0, 0);
    if ((nClearMask & 0x80) && (m_activityFlags & 0x80) && !(m_nLockedActivityFlags & 0x80))
        SetActivity(0x80, 0, 0);
    if ((nClearMask & ACTIVITY_POWER_ATTACK) && (m_activityFlags & ACTIVITY_POWER_ATTACK)
        && !(m_nLockedActivityFlags & ACTIVITY_POWER_ATTACK))
        SetActivity(ACTIVITY_POWER_ATTACK, 0, 0);
    if ((nClearMask & ACTIVITY_IMPROVED_POWER_ATTACK) && (m_activityFlags & ACTIVITY_IMPROVED_POWER_ATTACK)
        && !(m_nLockedActivityFlags & ACTIVITY_IMPROVED_POWER_ATTACK))
        SetActivity(ACTIVITY_IMPROVED_POWER_ATTACK, 0, 0);
    if ((nClearMask & ACTIVITY_PARRY) && (m_activityFlags & ACTIVITY_PARRY)
        && !(m_nLockedActivityFlags & ACTIVITY_PARRY))
        SetActivity(ACTIVITY_PARRY, 0, 0);
    if ((nClearMask & ACTIVITY_COUNTERSPELL) && (m_activityFlags & ACTIVITY_COUNTERSPELL)
        && !(m_nLockedActivityFlags & ACTIVITY_COUNTERSPELL))
        SetActivity(ACTIVITY_COUNTERSPELL, 0, 0);
    if ((nClearMask & ACTIVITY_FLURRY) && (m_activityFlags & ACTIVITY_FLURRY)
        && !(m_nLockedActivityFlags & ACTIVITY_FLURRY))
        SetActivity(ACTIVITY_FLURRY, 0, 0);
    if ((nClearMask & ACTIVITY_RAPID_SHOT) && (m_activityFlags & ACTIVITY_RAPID_SHOT)
        && !(m_nLockedActivityFlags & ACTIVITY_RAPID_SHOT))
        SetActivity(ACTIVITY_RAPID_SHOT, 0, 0);
}

float CSWSCreature::ComputeSpellRange(CExoString sRange, unsigned long long oidTarget)
{
    if (sRange == "P")
        sRange = "T";
    float fRange;
    if (sRange == "P")
        fRange = g_pRules->m_fSpellRange[0];
    else if (sRange == "T")
        fRange = g_pRules->m_fSpellRange[1];
    else if (sRange == "S")
        fRange = g_pRules->m_fSpellRange[2];
    else if (sRange == "M")
        fRange = g_pRules->m_fSpellRange[3];
    else if (sRange == "L")
        fRange = g_pRules->m_fSpellRange[4];
    else if (sRange == "W")
        fRange = g_pRules->m_fSpellRange[5];
    else
        fRange = 0.0f;
    fRange = fRange + (m_pPathfindInfo->m_fCrePersonalSpace + -0.1f);
    CSWSCreature* pTarget =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
    if (pTarget != 0)
        fRange = fRange + (pTarget->m_pPathfindInfo->m_fCrePersonalSpace + -0.1f);
    return fRange;
}

float CSWSCreature::ComputeMinSpellRange(CExoString sRange, unsigned long long oidTarget)
{
    if (sRange == "P")
        sRange = "T";
    float fRange;
    if (sRange == "P")
        fRange = g_pRules->m_fMinSpellRange[0];
    else if (sRange == "T")
        fRange = g_pRules->m_fMinSpellRange[1];
    else if (sRange == "S")
        fRange = g_pRules->m_fMinSpellRange[2];
    else if (sRange == "M")
        fRange = g_pRules->m_fMinSpellRange[3];
    else if (sRange == "L")
        fRange = g_pRules->m_fMinSpellRange[4];
    else if (sRange == "W")
        fRange = g_pRules->m_fMinSpellRange[5];
    else
        fRange = 0.0f;
    fRange = fRange + (m_pPathfindInfo->m_fCrePersonalSpace + -0.1f);
    CSWSCreature* pTarget =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
    if (pTarget != 0)
        fRange = fRange + (pTarget->m_pPathfindInfo->m_fCrePersonalSpace + -0.1f);
    return fRange;
}

void CSWSCreature::ComputeAIState()
{
    m_nAIStateAction = 0;
    m_oidAIStateTarget = OBJECT_INVALID;
    CExoLinkedListNode* pNode = m_actionList.m_pInternal->m_pHead;
    while (pNode != 0) {
        CSWSObjectActionNode* pAction = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->GetAtPos(pNode));
        unsigned long long oidTarget = OBJECT_INVALID;
        if (pAction->m_nActionId == AIACTION_PHYSICAL_ATTACKS)
            oidTarget = pAction->m_args.m_attackTargetId;
        ComputeAIStateOnAction(pAction->m_nActionId, oidTarget);
        m_actionList.m_pInternal->GetNext(pNode);
    }
    ComputeAIStateOnAction(m_nCurrentActionId, m_attackTargetId);
}

void CSWSCreature::RemoveCombatInvisibilityEffects()
{
    int i = m_pStats->m_iFirstInvisibility;
    while (i < m_effects.m_count) {
        CGameEffect* pEffect = m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_SANCTUARY)
            return;
        if ((pEffect->m_subType == CSWSEffectListHandler::EFFECT_INVISIBILITY
             && pEffect->GetInteger(0) == INVISIBILITY_TYPE_NORMAL)
            || pEffect->m_subType == CSWSEffectListHandler::EFFECT_SANCTUARY) {
            RemoveEffectById(pEffect->m_effectId);
        } else {
            i++;
        }
    }
}

int CSWSCreature::GetInvisible(CSWSObject* pObject)
{
    if (pObject->AsSWSCreature() == 0)
        return 0;
    CSWSCreature* pObserver = pObject->AsSWSCreature();
    int i = m_pStats->m_iFirstInvisibility;
    short nAlignment = pObserver->m_pStats->m_nAlignment;
    unsigned short nRace = pObserver->m_pStats->m_nRace;
    while (i < m_effects.m_count) {
        CGameEffect* pEffect = m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_SANCTUARY)
            return 0;
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_INVISIBILITY) {
            if ((pEffect->GetInteger(1) == nRace
                 || g_pRules->m_nNumRaces == pEffect->GetInteger(1))
                && (pEffect->GetInteger(3) == nAlignment
                    || pEffect->GetInteger(3) == 0)) {
                if (pEffect->GetInteger(0) == INVISIBILITY_TYPE_NORMAL || pEffect->GetInteger(0) == INVISIBILITY_TYPE_IMPROVED) {
                    if ((pObserver->m_nVisionFlags & (VISION_SEE_INVISIBLE | VISION_TRUE_SEEING)) != 0)
                        return 0;
                } else if (pEffect->GetInteger(0) == INVISIBILITY_TYPE_DARKNESS) {
                    if ((pObserver->m_nVisionFlags & (VISION_ULTRAVISION | VISION_TRUE_SEEING)) != 0)
                        return 0;
                }
                return 1;
            }
        } else if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_SANCTUARY
                   && pObserver->GetAIStateReputation(m_objectId) == 2
                   && (pEffect->GetInteger(1) == nRace
                       || g_pRules->m_nNumRaces == pEffect->GetInteger(1))
                   && (pEffect->GetInteger(3) == nAlignment
                       || pEffect->GetInteger(3) == 0)) {
            CSWVisibilityNode* pNode = pObserver->GetVisibleListElement(m_objectId);
            if (pNode == 0 || ((pNode->m_flags >> 2) & 3) == 0) {
                if (pObserver->SavingThrowRoll(SAVING_THROW_WILL, static_cast<unsigned short>(pEffect->GetInteger(4)), SAVING_THROW_TYPE_NONE, OBJECT_INVALID,
                                               1, 0, 0) == 0) {
                    PacifyCreature(pObserver->m_objectId);
                    pObserver->AddToVisibleList(m_objectId, 0, 0, 1, 0);
                    return 1;
                }
                pObserver->AddToVisibleList(m_objectId, 0, 0, 2, 0);
            } else if (((pNode->m_flags >> 2) & 3) == 2) {
                return 0;
            } else if (((pNode->m_flags >> 2) & 3) == 1) {
                return ((pObserver->m_nVisionFlags >> 2) & 1) == 0;
            }
        }
        i++;
    }
    return 0;
}

void CSWSCreature::LoadFollowInfo(CResGFF* pGff, CResStruct* pStruct)
{
    CResStruct resStruct;
    if (pGff->GetStructFromStruct(&resStruct, pStruct, const_cast<char*>("FollowInfo")) != 0
        && m_pFollowInfo == 0) {
        m_pFollowInfo = new CSWSCreaturePartyFollowInfo();
        m_pFollowInfo->Load(pGff, &resStruct);
    }
}

int CSWSCreature::DoStealthDetection(CSWSCreature* pTarget, int bDoSpot, int* pnSpot,
                                     int* pnListen, int a)
{
    if (pTarget == 0 || pTarget->GetArea() != GetArea()) {
        *pnSpot = 0;
        *pnListen = 0;
        return 0;
    }
    int nListen = DoListenDetection(pTarget, a);
    int nSpot = 0;
    if (bDoSpot != 0)
        nSpot = DoSpotDetection(pTarget, a);
    *pnListen = nListen;
    *pnSpot = nSpot;
    return (nSpot | nListen) != 0;
}

int CSWSCreature::DoListenDetection(CSWSCreature* pTarget, int a)
{
    Vector vSelf;
    Vector vTarget;
    Vector vHitFwd;
    Vector vHitRev;
    vSelf.x = 0.0f;
    vSelf.y = 0.0f;
    vSelf.z = 0.0f;
    vTarget.x = 0.0f;
    vTarget.y = 0.0f;
    vTarget.z = 0.0f;
    vHitFwd.x = 0.0f;
    vHitFwd.y = 0.0f;
    vHitFwd.z = 0.0f;
    vHitRev.x = 0.0f;
    vHitRev.y = 0.0f;
    vHitRev.z = 0.0f;

    if (pTarget->m_bSilenced != 0)
        return 0;

    // Both endpoints are lifted 1.5 m (ear height) before every range test.
    vSelf = m_position;
    vSelf.z += 1.5f;
    vTarget = pTarget->m_position;
    vTarget.z += 1.5f;

    float fDeltaX = vSelf.x - vTarget.x;
    float fDeltaY = vSelf.y - vTarget.y;
    float fDeltaZ = vSelf.z - vTarget.z;
#if KOTOR1  // K1: the distance sum is formed separately in each arm.
    float fDistSq;
    if (a != 0 || (m_nVisionFlags & VISION_BLINDNESS) != 0
        || (m_nVisionFlags & (VISION_ULTRAVISION | VISION_DARKNESS)) == VISION_DARKNESS) {
        fDistSq = fDeltaX * fDeltaX + fDeltaY * fDeltaY + fDeltaZ * fDeltaZ;
        float fReach = MaxAttackRange(pTarget->m_objectId, 0, 0);
        if (fDistSq > fReach * fReach)
            return 0;
        CSWVisibilityNode* pNode = GetVisibleListElement(pTarget->m_objectId);
        if (pNode != 0 && (pNode->m_flags & VISIBILITY_PACIFY_STATE_MASK) == VISIBILITY_PACIFIED)
            return 0;
    } else {
        fDistSq = fDeltaX * fDeltaX + fDeltaY * fDeltaY + fDeltaZ * fDeltaZ;
    }
    float fRange = GetListenCheckDistance();
    if (fDistSq > fRange * fRange)
        return 0;
#else
    float fDistSq = fDeltaX * fDeltaX + fDeltaY * fDeltaY + fDeltaZ * fDeltaZ;

    if (a != 0 || (m_nVisionFlags & VISION_BLINDNESS) != 0
        || (m_nVisionFlags & (VISION_ULTRAVISION | VISION_DARKNESS)) == VISION_DARKNESS) {
        float fReach = MaxAttackRange(pTarget->m_objectId, 0, 0);
        if (fDistSq > fReach * fReach)
            return 0;
        CSWVisibilityNode* pNode = GetVisibleListElement(pTarget->m_objectId);
        if (pNode != 0 && (pNode->m_flags & VISIBILITY_PACIFY_STATE_MASK) == VISIBILITY_PACIFIED)
            return 0;
    }
    float fRange = GetListenCheckDistance();
    if (fDistSq > fRange * fRange)
        return 0;
#endif

    // Probe the walkmesh from both ends: each call reports where the line stopped and what it hit.
#if KOTOR1  // K1 holds no area local: every use re-calls GetArea() (7 more calls)
    unsigned long long oidBlockFwd;
    unsigned long long oidBlockRev;
    static_cast<CSWSArea*>(GetArea())->ClearLineOfSight(vSelf, vTarget, &vHitFwd, &oidBlockFwd, OBJECT_INVALID,
                                             OBJECT_INVALID, 0, 0);
    static_cast<CSWSArea*>(GetArea())->ClearLineOfSight(vTarget, vSelf, &vHitRev, &oidBlockRev, OBJECT_INVALID,
                                             OBJECT_INVALID, 0, 0);
#else
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    unsigned long long oidBlockFwd;
    unsigned long long oidBlockRev;
    pArea->ClearLineOfSight(vSelf, vTarget, &vHitFwd, &oidBlockFwd, OBJECT_INVALID, OBJECT_INVALID, 0, 0);
    pArea->ClearLineOfSight(vTarget, vSelf, &vHitRev, &oidBlockRev, OBJECT_INVALID, OBJECT_INVALID, 0, 0);
#endif

    float fBlockedSq;
    if (oidBlockRev != OBJECT_INVALID && oidBlockFwd != OBJECT_INVALID && oidBlockFwd == oidBlockRev) {
        fBlockedSq = 0.4f * 0.4f;
    } else {
        if (oidBlockFwd != oidBlockRev) {
            if (oidBlockFwd != OBJECT_INVALID) {
                CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidBlockFwd);
                if (pObject != 0 && pObject->AsSWSObject() != 0)
                    vHitFwd = pObject->AsSWSObject()->m_position;
            }
            if (oidBlockRev != OBJECT_INVALID) {
                CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidBlockFwd);
                if (pObject != 0 && pObject->AsSWSObject() != 0)
                    vHitRev = pObject->AsSWSObject()->m_position;
            }
        }
        float fGapX = vHitRev.x - vHitFwd.x;
        float fGapY = vHitRev.y - vHitFwd.y;
        float fGapZ = vHitRev.z - vHitFwd.z;
        fBlockedSq = fGapX * fGapX + fGapY * fGapY + fGapZ * fGapZ;
    }

    int nSoundPenalty = 0;
    if (fBlockedSq > 0.0f) {
#if KOTOR1
        if ((static_cast<CSWSArea*>(GetArea())->m_nAreaFlags & 1) != 0
            || (static_cast<CSWSArea*>(GetArea())->m_nAreaFlags & 2) != 0) {
            CPathfindInformation* pPathInfo = static_cast<CSWSArea*>(GetArea())->m_pSoundPathInfo;
            pPathInfo->m_vStartPoint.x = m_position.x;
            pPathInfo->m_vStartPoint.y = m_position.y;
            pPathInfo->m_goal.m_vPoint.x = pTarget->m_position.x;
            pPathInfo->m_goal.m_vPoint.y = pTarget->m_position.y;
            if (static_cast<CSWSArea*>(GetArea())->PlotSoundPath(pPathInfo) != 1)
                return 0;
            nSoundPenalty = -2;
        } else {
            nSoundPenalty = static_cast<int>(sqrtf(fBlockedSq)) * -5;
        }
#else
        if ((pArea->m_nAreaFlags & 3) != 0) {
            CPathfindInformation* pPathInfo = pArea->m_pSoundPathInfo;
            pPathInfo->m_vStartPoint.x = m_position.x;
            pPathInfo->m_vStartPoint.y = m_position.y;
            pPathInfo->m_goal.m_vPoint.x = pTarget->m_position.x;
            pPathInfo->m_goal.m_vPoint.y = pTarget->m_position.y;
            if (pArea->PlotSoundPath(pPathInfo) != 1)
                return 0;
            nSoundPenalty = -2;
        } else {
            nSoundPenalty = static_cast<int>(sqrtf(fBlockedSq)) * -5;
        }
#endif
    }

    // A target with no Move Silently training at all, or not sneaking, is simply heard.
    if (pTarget->m_pStats->GetSkillRank(SKILL_STEALTH, AsSWSObject(), 0) == 0)
        return 1;
    if (pTarget->m_stealthMode == 0)
        return 1;
    if (pTarget->m_stealthMode == 1) {
        nSoundPenalty -= pTarget->m_pStats->GetSkillRank(SKILL_STEALTH, AsSWSObject(), 0);
        nSoundPenalty -= pTarget->m_nStealthMoveRoll;
    }

    unsigned char nDetectMode = m_detectMode - 1;
    int nAnim = m_nAnimation;
    char nRawListen = m_pStats->GetSkillRank(SKILL_AWARENESS, pTarget->AsSWSObject(), 0);
    char nListenRank = (nDetectMode <= 1 || nAnim == 10000)
                           ? nRawListen : static_cast<char>(nRawListen / 2);
#if KOTOR1
    int nDetect = nSoundPenalty + nListenRank + m_nListenRoll
                  + static_cast<CSWSArea*>(GetArea())->m_nListenModifier;
#else
    int nDetect = nSoundPenalty + nListenRank + m_nListenRoll + pArea->m_nListenModifier;
#endif

    CSWCCreature* pClientCreature = GetClientCreature();
    int bStill;
    if (pClientCreature != 0 && m_pStats->m_bIsPC != 0
            && pClientCreature->m_switchMode == CSWCCreature::MODE_SWDRIVE)
        bStill = (pClientCreature->m_bRunning == 0);
    else
        bStill = (m_bRunning == 0);
    if (!bStill)
        nDetect -= 5;
    if (m_bCombatState != 0)
        nDetect -= 10;

    // A stationary target is harder to hear; a stationary listener hears better.
    if (pTarget->AnimationStationary(static_cast<unsigned short>(pTarget->m_nAnimation)) != 0)
        nDetect -= 5;
    if (AnimationStationary(static_cast<unsigned short>(m_nAnimation)) != 0)
        nDetect += 5;

    Vector vRange = m_position - pTarget->m_position;
    nDetect -= static_cast<int>(magnitude(vRange) / 3.0f);

    // Bigger creatures are noisier. A real 5-arm jump table over m_nCreatureSize 1..5 with size 3
    // (medium) sharing the default target, i.e. no entry of its own.
    switch (pTarget->m_nCreatureSize) {
    case CREATURE_SIZE_TINY:
        nDetect -= 8;
        break;
    case CREATURE_SIZE_SMALL:
        nDetect -= 4;
        break;
    case CREATURE_SIZE_LARGE:
        nDetect += 4;
        break;
    case CREATURE_SIZE_HUGE:
        nDetect += 8;
        break;
    }
    return m_pStats->GetFavoredEnemyBonus(pTarget) + nDetect > 0;
}

int CSWSCreature::DoSpotDetection(CSWSCreature* pTarget, int a)
{
    if ((m_nVisionFlags & VISION_BLINDNESS) != 0)
        return 0;
    if (a != 0 || (m_nVisionFlags & (VISION_ULTRAVISION | VISION_DARKNESS)) == VISION_DARKNESS)
        return 0;

    // Both positions are lifted a metre before the range test (eye height).
    Vector vSelf = m_position;
    vSelf.z += 1.0f;
    Vector vTarget = pTarget->m_position;
    vTarget.z += 1.0f;
    Vector vDelta = vSelf - vTarget;
    float fDistSq = vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z;
    float fRange = GetSpotCheckDistance();
#if KOTOR1  // K1: no ForceAlwaysUpdate override
    if (fDistSq > fRange * fRange)
        return 0;
#else
    if (fDistSq > fRange * fRange && pTarget->m_bForceAlwaysUpdate == 0)
        return 0;
#endif

    // Re-read because GetSpotCheckDistance is opaque, so the flags byte is genuinely loaded twice.
    if ((m_nVisionFlags & VISION_TRUE_SEEING) != 0)
        return 1;
    if (pTarget->m_pStats->GetSkillRank(SKILL_STEALTH, AsSWSObject(), 0) == 0)
        return 1;
    if (pTarget->m_stealthMode == 0)
        return 1;

    int nHideRank = 0;
    int nHidePenalty = 0;
    if (pTarget->m_stealthMode == 1) {
#if KOTOR1  // K1: the rank reported to the client is a second GetSkillRank call
        nHidePenalty = -(pTarget->m_pStats->GetSkillRank(SKILL_STEALTH, AsSWSObject(), 0)
                         + pTarget->m_nStealthHideRoll);
        nHideRank = pTarget->m_pStats->GetSkillRank(SKILL_STEALTH, AsSWSObject(), 0);
#else
        nHideRank = pTarget->m_pStats->GetSkillRank(SKILL_STEALTH, AsSWSObject(), 0);
        nHidePenalty = -(pTarget->m_nStealthHideRoll + nHideRank);
#endif
    }

#if KOTOR1
    int nAnim = m_nAnimation;
    unsigned char nDetectMode = m_detectMode - 1;
#else
    unsigned char nDetectMode = m_detectMode - 1;
    int nAnim = m_nAnimation;
#endif
    char nRawSpot = m_pStats->GetSkillRank(SKILL_AWARENESS, pTarget->AsSWSObject(), 0);
#if KOTOR1  // K1 fetches the reported spot rank separately in each arm
    char nSpotRank;
    char nSpotRankMsg;
    if (nAnim == 10000 || nDetectMode <= 1) {
        nSpotRank = nRawSpot;
        nSpotRankMsg = m_pStats->GetSkillRank(SKILL_AWARENESS, pTarget->AsSWSObject(), 0);
    } else {
        nSpotRank = static_cast<char>(nRawSpot / 2);
        nSpotRankMsg = static_cast<char>(m_pStats->GetSkillRank(SKILL_AWARENESS, pTarget->AsSWSObject(), 0) / 2);
    }
    int nDetect = nHidePenalty + nSpotRank + m_nSpotRoll;
#else
    char nSpotRank = (nDetectMode <= 1 || nAnim == 10000)
                         ? nRawSpot : static_cast<char>(nRawSpot / 2);
    int nDetect = nHidePenalty + m_nSpotRoll + nSpotRank;
#endif

    CSWCCreature* pClientCreature = GetClientCreature();
    int bStill;
    if (pClientCreature != 0 && m_pStats->m_bIsPC != 0
            && pClientCreature->m_switchMode == CSWCCreature::MODE_SWDRIVE)
        bStill = (pClientCreature->m_bRunning == 0);
    else
        bStill = (m_bRunning == 0);
    int nMovePenalty = 0;
    if (!bStill) {
        nMovePenalty = -5;
        nDetect -= 5;
    }

    CSWSPlayer* pSelfClient =
        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
    CSWSPlayer* pTargetClient =
        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(pTarget->m_objectId);

    // The rear blind arc, centred on 180 degrees and m_fBlindSpot wide (GFF "BlindSpot", 120.0f).
    // Only applied when the spotter is server-side but the target has a client object.
    int nArcPenalty = 0;
    if (pSelfClient == 0 && pTargetClient != 0) {
        Vector vDir = pTarget->m_position - m_position;
        if (vDir.x * vDir.x + vDir.y * vDir.y + vDir.z * vDir.z > 0.0f)
            vDir = normalize(vDir);
        else
            vDir = m_orientation;
        Vector vFacing = normalize(m_orientation);
#if KOTOR1
        float fDot = vFacing.x * vDir.x + vFacing.y * vDir.y + vFacing.z * vDir.z;
        if (fDot <= -0.707f) {
            nArcPenalty = -5;
            nDetect -= 5;
        }
#else
        float fAngle = acosf(vFacing.x * vDir.x + vFacing.y * vDir.y + vFacing.z * vDir.z);
        float fHalfArc = m_pStats->m_fBlindSpot * 0.5f;
        float fArcLo = 180.0f - fHalfArc;
        float fArcHi = fHalfArc + 180.0f;
        if (fArcLo < 0.0f)
            fArcLo = 0.0f;
        if (fArcHi > 360.0f)
            fArcHi = 360.0f;
        fArcLo *= 0.017453292f;
        fArcHi *= 0.017453292f;
        if (fAngle >= fArcLo && fAngle <= fArcHi) {
            nArcPenalty = -10;
            nDetect -= 10;
        }
#endif
    }

    int nCombatPenalty = 0;
    if (m_bCombatState != 0) {
        nCombatPenalty = -10;
        nDetect -= 10;
    }

    // A stationary target is harder to see; a stationary spotter sees better.
    // Oddly, the detection message's target-stationary field is set to +5 even though the score is
    // decreased by 5.
    int nTargetStationary = 0;
    if (pTarget->AnimationStationary(static_cast<unsigned short>(pTarget->m_nAnimation)) != 0) {
        nTargetStationary = 5;
        nDetect -= 5;
    }
    int nSelfStationary = 0;
    if (AnimationStationary(static_cast<unsigned short>(m_nAnimation)) != 0) {
        nSelfStationary = 5;
        nDetect += 5;
    }

    Vector vRange = m_position - pTarget->m_position;
    float fSteps = magnitude(vRange) / 3.0f;
    int nDistancePenalty = 0;
    if (fSteps > 2.0f) {
        int nSteps = static_cast<int>(fSteps - 2.0f);
        nDistancePenalty = -nSteps;
#if KOTOR1  // K1: the distance is subtracted with the right sign
        nDetect -= nSteps;
#else
        nDetect -= nDistancePenalty;
#endif
    }
    if (nDetect <= 0)
        return 0;

    CSWCCMessageData* pData = new CSWCCMessageData;
    pData->SetObjectID(0, m_objectId);
    pData->SetObjectID(1, pTarget->m_objectId);
    pData->SetInteger(0, 1);
    pData->SetInteger(1, m_nSpotRoll);
    pData->SetInteger(2, pTarget->m_nStealthHideRoll);
#if KOTOR1
    pData->SetInteger(3, nSpotRankMsg);
#else
    pData->SetInteger(3, nSpotRank);
#endif
    pData->SetInteger(4, nHideRank);
    pData->SetInteger(5, nSelfStationary);
    pData->SetInteger(6, nTargetStationary);
    pData->SetInteger(7, nDistancePenalty);
    pData->SetInteger(8, nArcPenalty);
    pData->SetInteger(9, nMovePenalty);
    pData->SetInteger(10, nCombatPenalty);
    if (g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId) != 0)
        BroadcastStealthDetectionData(pData);
    else if (g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(pTarget->m_objectId) != 0)
        pTarget->BroadcastStealthDetectionData(pData);
    delete pData;
    return 1;
}

int CSWSCreature::AddAppearActions()
{
    if (m_bCommandable == 0)
        return 0;
    AddAction(AIACTION_APPEAR, ACTION_GROUP_NEW, 0,
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    SetGroupInterruptable(m_prevGroupId, 0);
    return 1;
}

// Mirrors AddAppearActions above and additionally queues the remove-from-area (a != 0) or
// destroy-object event 2000 ms out.
int CSWSCreature::AddDisappearActions(int a)
{
    if (m_bCommandable == 0)
        return 0;
    AddAction(AIACTION_DISAPPEAR, ACTION_GROUP_NEW, 0,
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    SetGroupInterruptable(m_prevGroupId, 0);
    CServerAIMaster* pAIMaster = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
    if (a != 0)
        pAIMaster->AddEventDeltaTime(0, 2000, m_objectId, m_objectId,
                                     CServerAIMaster::EVENT_REMOVE_FROM_AREA,
                                     reinterpret_cast<void*>(static_cast<long long>(a)));
    else
        pAIMaster->AddEventDeltaTime(0, 2000, m_objectId, m_objectId,
                                     CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
    return 1;
}

#if KOTOR1
int CSWSCreature::AddPathfindingWaitActionToFront(unsigned short nGroupActionId)
{
    float fWaitTime = 0.3f;
#else
int CSWSCreature::AddPathfindingWaitActionToFront(unsigned short nGroupActionId, float fWaitTime)
{
    fWaitTime = fWaitTime != 0.0f ? fWaitTime : 0.3f;  // NaN keeps fWaitTime either way
#endif
    if (g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId) != 0)
        fWaitTime = 0.1f;
    AddActionToFront(AIACTION_WAIT, nGroupActionId, ACTION_PARAM_FLOAT, &fWaitTime, 0, 0, 0, 0, 0,
                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return 1;
}

int CSWSCreature::AddPickUpItemActions(unsigned long long oidItem, unsigned long long oidContainer,
                                       unsigned char bFlag)
{
    if (g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem) == 0)
        return 0;
    if (GetArea() == 0)
        return 0;
    if (m_bCommandable == 0)
        return 0;
    AddAction(AIACTION_PICK_UP_ITEM, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidItem,
              ACTION_PARAM_OBJECT, &oidContainer, ACTION_PARAM_INT, &bFlag, 0, 0, 0, 0, 0, 0, 0, 0,
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return 1;
}

int CSWSCreature::AddHealActions(unsigned long long oidItem, unsigned long long oidTarget, int nSpell)
{
    if (m_pStats->GetCanUseSkill(SKILL_TREAT_INJURY) == 0) {
        SendFeedbackMessage(FEEDBACK_CANNOT_USE_SKILL, NULL);
        return 0;
    }
    if (m_bCommandable == 0)
        return 0;
    int bMayApproach = 1;
    AddAction(AIACTION_HEAL, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidItem,
              ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nSpell,
              ACTION_PARAM_INT, &bMayApproach, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
              0);
    return 1;
}

int CSWSCreature::AddCutsceneAttackActions(unsigned long long oidTarget, int a2, int a3, int a4)
{
    m_pCombatRound->AddCutsceneAttackAction(oidTarget, a2, a3, a4);
    if (m_bCombatState == 0)
        ActionManager(4);
    if (GetNodeById(AIACTION_COMBAT) == 0) {
        int bCombatPending = 1;
        AddAction(AIACTION_COMBAT, ACTION_GROUP_NEW, ACTION_PARAM_INT, &bCombatPending, 0, 0, 0, 0,
                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    return 1;
}

int CSWSCreature::AddCutsceneMoveActions(unsigned long long oidTarget, const Vector& vPos, int nRun)
{
    m_pCombatRound->AddCutsceneMoveActions(oidTarget, vPos, nRun);
    if (m_bCombatState == 0)
        ActionManager(4);
    if (GetNodeById(AIACTION_COMBAT) == 0) {
        int bCombatPending = 1;
        AddAction(AIACTION_COMBAT, ACTION_GROUP_NEW, ACTION_PARAM_INT, &bCombatPending, 0, 0, 0, 0,
                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    return 1;
}

int CSWSCreature::AddAttackActions(unsigned long long oidTarget, unsigned short a,
                                   int b, int c, int d, int e, int f, int g, int h,
                                   int i, int j, int k, int l, int m, int n)
{
    if (f != 0) {
        ComputeAIState();
#if KOTOR1  // K1: no AI-state exemptions
        if (m_bCommandable == 0)
            return 0;
#else
        if (m_bCommandable == 0 && m_nState != EFFECT_STATE_CONFUSED
            && m_nState != EFFECT_STATE_DROID_CONFUSED)
            return 0;
#endif
        CSWVisibilityNode* pNode = GetVisibleListElement(oidTarget);
        if (pNode != 0 && (pNode->m_flags & VISIBILITY_PACIFY_STATE_MASK) == VISIBILITY_PACIFIED)
            return 0;
        if (oidTarget != OBJECT_INVALID) {
            CGameObject* pObject =
                g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
            if (pObject != 0 && pObject->AsSWSCreature() != 0
#if KOTOR1
                    && GetAIStateReputation(oidTarget) == 1) {
#else
                    && GetAIStateReputation(oidTarget) == 1
                    && m_nState != EFFECT_STATE_CONFUSED && m_nState != EFFECT_STATE_DROID_CONFUSED) {
#endif
                SendFeedbackMessage(FEEDBACK_SKILL_TARGET_HOSTILE, NULL);
                return 0;
            }
        }
        if (d != 0)
            ClearAllActions(1);
        if (oidTarget != OBJECT_INVALID) {
#if KOTOR1 || defined(K2_FIX_ENGINE_BUGS)
            int nA = a;
            if (e != 0) {
                AddActionToFront(AIACTION_PHYSICAL_ATTACKS, ACTION_GROUP_NEW, ACTION_PARAM_INT, &k,
                                 ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &g,
                                 ACTION_PARAM_INT, &h, ACTION_PARAM_INT, &i, ACTION_PARAM_INT, &j,
                                 ACTION_PARAM_INT, &nA, ACTION_PARAM_INT, &l, ACTION_PARAM_INT, &m,
                                 ACTION_PARAM_INT, &n, 0, 0, 0, 0, 0, 0);
            } else {
                AddAction(AIACTION_PHYSICAL_ATTACKS, ACTION_GROUP_NEW, ACTION_PARAM_INT, &k,
                          ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &g,
                          ACTION_PARAM_INT, &h, ACTION_PARAM_INT, &i, ACTION_PARAM_INT, &j,
                          ACTION_PARAM_INT, &nA, ACTION_PARAM_INT, &l, ACTION_PARAM_INT, &m,
                          ACTION_PARAM_INT, &n, 0, 0, 0, 0, 0, 0);
            }
#else
            if (e != 0) {
                AddActionToFront(AIACTION_PHYSICAL_ATTACKS, ACTION_GROUP_NEW, ACTION_PARAM_INT, &k,
                                 ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &g,
                                 ACTION_PARAM_INT, &h, ACTION_PARAM_INT, &i, ACTION_PARAM_INT, &j,
                                 ACTION_PARAM_INT, &a, ACTION_PARAM_INT, &l, ACTION_PARAM_INT, &m,
                                 ACTION_PARAM_INT, &n, 0, 0, 0, 0, 0, 0);
            } else {
                AddAction(AIACTION_PHYSICAL_ATTACKS, ACTION_GROUP_NEW, ACTION_PARAM_INT, &k,
                          ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &g,
                          ACTION_PARAM_INT, &h, ACTION_PARAM_INT, &i, ACTION_PARAM_INT, &j,
                          ACTION_PARAM_INT, &a, ACTION_PARAM_INT, &l, ACTION_PARAM_INT, &m,
                          ACTION_PARAM_INT, &n, 0, 0, 0, 0, 0, 0);
            }
#endif
            CSWSCreature* pTarget =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
            if (pTarget != 0) {
                if (pTarget->m_bPlayerCharacter == 1
                        || g_pAppManager->m_pServerExoApp->GetPartyTable()->
                               GetIsLeader(pTarget->m_objectId) == 1)
                    pTarget->m_oidGoingToBeAttackedBy = m_objectId;
                BroadcastCombatStateToParty(m_objectId);
                m_bInitiativeExpired = (m_bInParty == 0);
            } else {
                BroadcastCombatStateToParty(m_objectId);
            }
            if (m_bCombatState == 0 && m_bPlayerCharacter == 1) {
                if (rand_wincompatible() % 4 <= 2)
                    BroadcastBattleCry();
            } else if (m_bInParty == 1 && rand_wincompatible() % 10 == 0) {
                BroadcastBattleCry();
            }
            if (m_attemptedAttackTargetId == OBJECT_INVALID)
                m_attemptedAttackTargetId = oidTarget;
            m_bPassiveAttackBehaviour = c;
        }
        CSWSDoor* pDoor = g_pAppManager->m_pServerExoApp->GetDoorByGameObjectID(oidTarget);
        if (pDoor != 0 && pDoor->m_bPlot == 1) {
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_FAIL_TO_OPEN;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        }
#if !KOTOR1  // K1: only a plot door is signalled
        CSWSPlaceable* pPlaceable =
            g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(oidTarget);
        if (pPlaceable != 0 && pPlaceable->m_bPlot == 1) {
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_FAIL_TO_OPEN;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        }
#endif
        return 1;
    }

    if (b != 0 && d != 0) {
        ClearAllActions(1);
        m_pCombatRound->RemoveAllActions();
        m_pCombatRound->ClearAllAttacks();
    }
    m_pCombatRound->AddAttackAction(oidTarget, a, b, 1500, 1);
    if (m_bCombatState == 0)
        ActionManager(4);
    if (GetNodeById(AIACTION_COMBAT) == 0) {
        int nOne = 1;
#if KOTOR1  // K1: the combat action goes to the back of the queue
        AddAction(AIACTION_COMBAT, ACTION_GROUP_NEW, ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0, 0, 0,
                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#else
        AddActionToFront(AIACTION_COMBAT, ACTION_GROUP_NEW, ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#endif
    }
    return 1;
}

void CSWSCreature::BroadcastVoiceChat(unsigned char nVoiceChat)
{
    if (GetArea() == 0)
        return;
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    for (int i = 0; i < pArea->m_aObjectsByX.m_count; i++) {
        CSWSPlayer* pPlayer =
            g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(pArea->m_aObjectsByX.m_pArray[i]);
        if (pPlayer != 0) {
            CSWSObject* pObject = pPlayer->GetGameObject();
            if (pObject != 0 && m_cachedAreaId == pObject->m_cachedAreaId) {
                Vector vDelta = pObject->m_position - m_position;
                if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z <= 900.0f) {
                    g_pAppManager->m_pServerExoApp->GetSWSMessage()->
                        SendServerToPlayerVoiceChat_Play(pPlayer, m_objectId, nVoiceChat);
                }
            }
        }
    }
}

void CSWSCreature::BroadcastBattleCry()
{
    if (GetArea() == 0)
        return;
    unsigned long long nPlayerId =
        g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
    CSWSPlayer* pPlayer =
        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(nPlayerId);
    if (pPlayer != 0) {
        CSWSObject* pObject = pPlayer->GetGameObject();
        if (pObject != 0 && m_cachedAreaId == pObject->m_cachedAreaId) {
            Vector vDelta = pObject->m_position - m_position;
            if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z <= 900.0f) {
                unsigned char nCry = 5;
                unsigned int nRoll = rand_wincompatible() % 5;
                if (nRoll < 4)
                    nCry = static_cast<unsigned char>(nRoll + 1);
                g_pAppManager->m_pServerExoApp->GetSWSMessage()->
                    SendServerToPlayerVoiceChat_Play(pPlayer, m_objectId, nCry);
            }
        }
    }
}

int CSWSCreature::AddDriveAction(unsigned short nGroupId, const Vector& vPosition,
                                 int a, int b, int c, int d)
{
    if (GetArea() == 0)
        return 0;
#if KOTOR1  // K1's flag operand is a 4-byte int
    int bFlag = 1;
#else
    unsigned char bFlag = 1;
#endif
    if (m_bMovementSpeedLimited != 0)
        c &= 0xfffd;
    Vector* pPos = const_cast<Vector*>(&vPosition);
    AddAction(AIACTION_DRIVE, nGroupId, ACTION_PARAM_FLOAT, static_cast<void*>(pPos),
              ACTION_PARAM_FLOAT, static_cast<void*>(&pPos->y), ACTION_PARAM_INT, &a,
              ACTION_PARAM_INT, &b, ACTION_PARAM_INT, &c, ACTION_PARAM_INT, &d,
              ACTION_PARAM_INT, &bFlag, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return 1;
}

void CSWSCreature::MergeItem(CSWSItem* pItem, CSWSItem* pMergeItem)
{
    if (g_pRules->m_pBaseItemArray->GetBaseItem(pItem->m_baseItem)->m_nStacking >= 2) {
        if (pItem->MergeItem(pMergeItem) != 0) {
            RemoveItem(pMergeItem, 0, 0, 0);
            if (pMergeItem != 0)
                delete pMergeItem;
        } else if (m_pInventory->GetItemInInventory(pItem) == 1) {
#if KOTOR1  // K1: 18 equipment slots
            for (int i = 0; i < 18; i++) {
#else
            for (int i = 0; i < 20; i++) {
#endif
                unsigned int nSlot = 1 << i;
                CSWSItem* pSlotItem = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(nSlot));
                if (pSlotItem != 0 && pSlotItem->m_objectId == pItem->m_objectId) {
                    AddEquipItemActions(pMergeItem, nSlot, 0, 0, 0, 0, 0, 0);
                    return;
                }
            }
        }
    }
}

int CSWSCreature::GetItemCount(int a, int b, int c)
{
    int i = 0;
    int nCount = 0;
    if (b == 0 && m_pInventory != 0) {
#if KOTOR1  // K1: the first 14 equipment slots
        for (; i < 14; i++) {
#else
        for (; i < 16; i++) {
#endif
            CSWSItem* pItem = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(1 << i));
            if (pItem != 0 &&
                (a == 0 || (pItem->m_nFlagsLow & 8) != 0) &&
                (c == 0 || pItem->m_bPlot == 0)) {
                nCount++;
            }
        }
    }
    CItemRepository* pRepo;
#if KOTOR1  // K1: the out-of-line GetItemRepository(1)
    pRepo = GetItemRepository(1);
#else
    if ((g_pAppManager->m_pClientExoApp->GetInGameGui()->GetTopMenuOverrideCharacter() == -1) &
        (m_bInParty == 0)) {
        pRepo = m_pItemRepository;
    } else {
        pRepo = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems();
    }
#endif
    if (pRepo != 0 && pRepo->ItemListGetCount() != 0) {
        for (unsigned int nItem = 0; nItem < pRepo->ItemListGetCount(); nItem++) {
            CSWSItem* pItem = pRepo->ItemListGetItem(nItem);
            if (pItem != 0 &&
                (a == 0 || (pItem->m_nFlagsLow & 8) != 0) &&
                (c == 0 || pItem->m_bPlot == 0)) {
                nCount++;
            }
        }
    }
    return nCount;
}

float CSWSCreature::GetWalkRate()
{
    float fWalkRate = m_pStats->m_fWalkRate * 1000.0f;
    fWalkRate *= GetMovementRateFactor(1);
    if (!(fWalkRate <= 100.0f)) {
        unsigned long long nClientId =
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId);
        CSWCCreature* pCreature =
            g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(nClientId);
        g_pAppManager->m_pClientExoApp->GetSWParty();  // result discarded
#if KOTOR1  // K1: no feat exemption; it tests bit 0 of the activity mask
        if (pCreature != 0 && (m_activityFlags & 1) != 0)
#else
        if ((m_activityFlags & 1) != 0 && !m_pStats->HasFeat(FEAT_STEALTH_RUN) && pCreature != 0)
#endif
            return pCreature->m_pCreatureAppearance->m_walkSpeed * 1000.0f;
        return fWalkRate;
    }
    return 0.0f;
}

int CSWSCreature::UseFeat(unsigned short nFeat, unsigned short a,
                          unsigned long long objectId, unsigned long long b)
{
    K2_UNUSED(a); K2_UNUSED(b);
    if (!m_pStats->HasFeat(nFeat))
        return 0;
    CSWFeat* pFeat;
    unsigned short nUseFeat = nFeat;
    int nSuccessor = nFeat;
    do {
        nUseFeat = static_cast<unsigned short>(nSuccessor);
        pFeat = g_pRules->GetFeat(nUseFeat);
        nSuccessor = pFeat->m_nSuccessor;
    } while (nSuccessor >= 1 && m_pStats->HasFeat(static_cast<unsigned short>(nSuccessor)));
    int bUsed = 0;
    switch (nUseFeat) {
    case FEAT_CRITICAL_STRIKE:
    case FEAT_FLURRY:
    case FEAT_IMPROVED_POWER_ATTACK:
    case FEAT_IMPROVED_POWER_BLAST:
    case FEAT_IMPROVED_CRITICAL_STRIKE:
    case FEAT_IMPROVED_SNIPER_SHOT:
    case FEAT_MULTI_SHOT:
    case FEAT_POWER_ATTACK:
    case FEAT_POWER_BLAST:
    case FEAT_RAPID_SHOT:
    case FEAT_SNIPER_SHOT:
    case FEAT_WHIRLWIND_ATTACK:
    case FEAT_MASTER_SNIPER_SHOT:
    case FEAT_MASTER_CRITICAL_STRIKE:
    case FEAT_MASTER_POWER_BLAST:
    case FEAT_MASTER_POWER_ATTACK:
    case FEAT_IMPROVED_FLURRY:
    case FEAT_IMPROVED_RAPID_SHOT:
#if !KOTOR1  // K1: no feat
    case CSWSCreatureStats::FEAT_SHIELD_BREAKER:
#endif
        bUsed = 1;
        m_pCombatRound->AddAttackAction(objectId, 0, nUseFeat, 1500, 1);
        break;
    }
    if (nUseFeat == FEAT_ADVANCED_GUARD_STANCE || nUseFeat == FEAT_MASTER_GUARD_STANCE
        || nUseFeat == FEAT_GUARD_STANCE) {
        if (m_nGuardStanceFeat != 0)
            m_nGuardStanceFeat = 0;
        else
            m_nGuardStanceFeat = nUseFeat;
    }
    return bUsed;
}

void CSWSCreature::ClearHostileActionsVersus(CSWSCreature* pCreature)
{
    int nGroups = GetNumActionGroups();
    for (int i = 0; i < nGroups; i++) {
        CExoLinkedListNode* pPos = GetPositionByGroupIndex(i);
        if (pPos == 0)
            continue;
        CSWSObjectActionNode* pGroupNode = static_cast<CSWSObjectActionNode*>(m_actionList.m_pInternal->GetAtPos(pPos));
        if (pGroupNode == 0)
            continue;
        unsigned short nGroupId = pGroupNode->m_nGroupId;
        CSWSObjectActionNode* pNode;
        if (GetActionByGroupId(nGroupId, &pNode) == AIACTION_NONE)
            continue;
        if (pNode == 0)
            continue;
#if KOTOR1  // K1: each action kind removes its group in its own arm
        if (pNode->m_nActionId == AIACTION_PHYSICAL_ATTACKS) {
            if (pCreature == 0 || pNode->m_args.m_attackTargetId == pCreature->m_objectId) {
                RemoveGroup(nGroupId);
                i = 0;
            }
        } else if (pNode->m_nActionId == AIACTION_CAST_SPELL) {
            if (pCreature == 0 || pNode->m_args.m_spellTargetId == pCreature->m_objectId) {
                RemoveGroup(nGroupId);
                i = 0;
            }
        } else if (pNode->m_nActionId == AIACTION_ITEM_CAST_SPELL) {
            if (pCreature == 0 || pNode->m_args.m_itemCastTargetId == pCreature->m_objectId) {
                RemoveGroup(nGroupId);
                i = 0;
            }
        }
    }
#else
        if (pNode->m_nActionId == AIACTION_PHYSICAL_ATTACKS) {
            if (pCreature != 0 && pNode->m_args.m_attackTargetId != pCreature->m_objectId)
                continue;
        } else if (pNode->m_nActionId == AIACTION_CAST_SPELL) {
            if (pCreature != 0 && pNode->m_args.m_spellTargetId != pCreature->m_objectId)
                continue;
        } else if (pNode->m_nActionId == AIACTION_ITEM_CAST_SPELL) {
            if (pCreature != 0 && pNode->m_args.m_itemCastTargetId != pCreature->m_objectId)
                continue;
        } else {
            continue;
        }
        RemoveGroup(nGroupId);
        i = 0;
    }
#endif
}

int CSWSCreature::GetHasPlotItemUseableOnObject()
{
    CItemRepository* pRepo;
#if KOTOR1  // K1: the out-of-line GetItemRepository(1)
    pRepo = GetItemRepository(1);
#else
    if ((g_pAppManager->m_pClientExoApp->GetInGameGui()->GetTopMenuOverrideCharacter() == -1)
        & (m_bInParty == 0)) {
        pRepo = m_pItemRepository;
    } else {
        pRepo = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems();
    }
#endif
    if (pRepo->ItemListGetCount() != 0) {
        for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++) {
            CSWSItem* pItem = pRepo->ItemListGetItem(i);
            if (pItem == 0 || pItem->m_bPlot == 0)
                continue;
            CSWItemProperty* pProp;
            if (!pItem->GetPropertyByType(&pProp, ITEM_PROPERTY_ACTIVATE_ITEM, 0))
                continue;
            unsigned short nSubType = pProp->m_subType;
            CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(nSubType);
            if (pSpell != 0 && pSpell->m_nItemTargeting == 2)
                return 1;
        }
    }
    return 0;
}

void CSWSCreature::AutoCloseGUIPanels(int a)
{
    K2_UNUSED(a);
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
    if (pPlayer == 0)
        return;
    if (pPlayer->m_pContainerGUI->m_objectId == CGameObjectArray::INVALIDOBJID)
        return;
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(
            pPlayer->m_pContainerGUI->m_objectId, &pObject) != CGameObjectArray::SUCCESS)
        return;
    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_ITEM) {
        CSWSItem* pItem = static_cast<CSWSItem*>(pObject->AsSWSItem());
        if (pItem != 0)
            pItem->CloseInventory(m_objectId, 1);
    } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
        CSWSPlaceable* pPlaceable = static_cast<CSWSPlaceable*>(pObject->AsSWSPlaceable());
        if (pPlaceable != 0)
            pPlaceable->CloseInventory(m_objectId, 1);
    }
}

void CSWSCreature::RemoveBadEffects()
{
    int i = 0;
    while (i < m_effects.m_count) {
        CGameEffect* pEffect = m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_WOUNDING)
            return;
        if ((pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_TEMPORARY
            || ((pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_PERMANENT
                && ((pEffect->m_durationTypeBits & SUBTYPE_MAGICAL) | SUBTYPE_SUPERNATURAL) == SUBTYPE_EXTRAORDINARY)) {
            RemoveEffectById(pEffect->m_effectId);
            i = 0;
        } else {
            i++;
        }
    }
}

void CSWSCreature::OnEndDialog(CSWSObject* pSpeaker)
{
    K2_UNUSED(pSpeaker);
    g_pVirtualMachine->RunScript(&m_scripts[CREATURE_SCRIPT_ENDDIALOGUE], m_objectId, 1);
#if !KOTOR1  // K1: no pre-dialog facing restore
    if (m_bRestoreDialogPose != 0) {
        m_bRestoreDialogPose = 0;
        if (equal(m_vDialogStartPosition, m_position)) {
            SetOrientation(m_vDialogStartOrientation);
            if (GetClientObject() != 0 && GetClientObject()->AsSWCCreature() != 0)
                static_cast<CSWCCreature*>(GetClientObject()->AsSWCCreature())->SetOrientation(m_vDialogStartOrientation);
        }
    }
#endif
}

void CSWSCreature::PacifyCreature(unsigned long long objectId)
{
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(objectId);
    if (pCreature != 0) {
        if (pCreature->m_attemptedAttackTargetId == m_objectId ||
            pCreature->m_oidAttackTarget == m_objectId ||
            pCreature->m_oidAttemptedSpellTarget == m_objectId ||
            pCreature->m_oidSpellTarget == m_objectId ||
            pCreature->m_lastHostileActorId == m_objectId) {
#if KOTOR1
            pCreature->SetCombatState(0, 1);
#else
            pCreature->SetCombatState(0, 1, 0);
#endif
            if (pCreature->m_lastHostileActorId == m_objectId)
                pCreature->SetLastHostileActor(OBJECT_INVALID, 1);
            pCreature->ClearHostileActionsVersus(this);
        }
    }
}

void CSWSCreature::PacifyCreature()
{
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (pArea != 0 && pArea->m_aObjectsByX.m_count >= 1) {
        for (int i = 0; i < pArea->m_aObjectsByX.m_count; i++) {
            CSWSCreature* pCr =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pArea->m_aObjectsByX.m_pArray[i]);
            if (pCr != 0 &&
                (pCr->m_attemptedAttackTargetId == m_objectId ||
                 pCr->m_oidAttackTarget == m_objectId ||
                 pCr->m_oidAttemptedSpellTarget == m_objectId ||
                 pCr->m_oidSpellTarget == m_objectId ||
                 pCr->m_lastHostileActorId == m_objectId)) {
#if KOTOR1
                pCr->SetCombatState(0, 1);
#else
                pCr->SetCombatState(0, 1, 0);
#endif
                if (pCr->m_lastHostileActorId == m_objectId) {
                    pCr->SetLastHostileActor(OBJECT_INVALID, 1);
                }
                pCr->ClearHostileActionsVersus(this);
            }
        }
    }
}

void CSWSCreature::CancelGoldTransfer(unsigned long long objectId)
{
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CGameObject* pObject;
    if (pObjectArray->GetGameObject(objectId, &pObject) == CGameObjectArray::SUCCESS) {
        CSWSItem* pItem = pObject->AsSWSItem();
        if (pItem != 0 && static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_CREDITS) {
            CItemRepository* pRepo;
#if KOTOR1  // K1: the out-of-line GetItemRepository(1)
            pRepo = GetItemRepository(1);
#else
            if ((g_pAppManager->m_pClientExoApp->GetInGameGui()->GetTopMenuOverrideCharacter() == -1) &
                (m_bInParty == 0)) {
                pRepo = m_pItemRepository;
            } else {
                pRepo = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems();
            }
#endif
            pRepo->AddItem(&pItem, 1, 0, 1);
        }
    }
}

int CSWSCreature::RequestBuy(unsigned long long itemId, unsigned long long storeId)
{
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(itemId);
    if (pItem == 0)
        return 0;
    CSWSStore* pStore = g_pAppManager->m_pServerExoApp->GetStoreByGameObjectID(storeId);
    if (pStore == 0)
        return 0;
    if (pStore->GetItemInInventory(itemId) == 0)
        return 0;
    if (pItem->m_pItemRepository != 0 && pItem->m_pItemRepository->ItemListGetCount() != 0) {
        SendFeedbackMessage(FEEDBACK_STORE_CONTAINER_NOT_EMPTY, 0);
        return 0;
    }
    if (static_cast<unsigned int>(m_nGold) < static_cast<unsigned int>(pStore->CalculateItemSellPrice(pItem, m_objectId))) {
        SendFeedbackMessage(FEEDBACK_STORE_NOT_ENOUGH_GOLD, 0);
        return 0;
    }
    return 1;
}

int CSWSCreature::GetIsWeaponEffective(unsigned long long targetId, int bOffhand)
{
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(targetId);
    CSWSObject* pTarget = 0;
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
        pTarget = pObject->AsSWSObject();
        if (pTarget != 0 && pTarget->m_bPlot == 1)
            return 0;
    }
    int nDamage = m_pStats->GetDamageRoll(pTarget, bOffhand, 0, 0, 1, 0);
    return CalculateMaxElementalDamage(pTarget, bOffhand) + nDamage > 0;
}

void CSWSCreature::SendFeedbackMessage(unsigned short nFeedbackMessage, CSWCCMessageData* pData)
{
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
    if (pPlayer != 0) {
        if (pData == 0)
            pData = new CSWCCMessageData();
        pData->SetInteger(9, nFeedbackMessage);
        g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerCCMessage(
            pPlayer->m_nPlayerId, CC_MESSAGE_FEEDBACK, pData, 0);
    }
    if (pData != 0)
        delete pData;
}

void CSWSCreature::SetInParty(int partyFlag, int a)
{
    m_bInParty = partyFlag;
    if (a != 0) {
        if (partyFlag != 0) {
            if (m_pFollowInfo == 0) {
                m_pFollowInfo = new CSWSCreaturePartyFollowInfo();
            }
            if (m_aiLevel == 4)
                return;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->SetAILevel(this, 4);
        } else {
            if (m_pFollowInfo != 0) {
                delete m_pFollowInfo;
                m_pFollowInfo = 0;
            }
            if (m_aiLevel == 2)
                return;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->SetAILevel(this, 2);
        }
    }
}

void CSWSCreature::RestoreItemProperties()
{
#if KOTOR1  // K1: 18 equipment slots
    for (int i = 0; i < 18; i++) {
#else
    for (int i = 0; i < 20; i++) {
#endif
        CSWItem* pItem = m_pInventory->GetItemInSlot(1 << i);
        if (pItem != 0) {
            static_cast<CSWSItem*>(pItem)->RestoreUsedActiveProperties(0);
            static_cast<CSWSItem*>(pItem)->UpdateUsedActiveProperties();
        }
    }
    CItemRepository* pRepo;
#if KOTOR1  // K1: the out-of-line GetItemRepository(1)
    pRepo = GetItemRepository(1);
#else
    if ((g_pAppManager->m_pClientExoApp->GetInGameGui()->GetTopMenuOverrideCharacter() == -1) &
        (m_bInParty == 0)) {
        pRepo = m_pItemRepository;
    } else {
        pRepo = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems();
    }
#endif
    while (pRepo->ItemListGetCount() != 0) {
        CSWSItem* pItem = pRepo->ItemListGetItem(0);
        if (pItem != 0) {
            pItem->RestoreUsedActiveProperties(0);
            pItem->UpdateUsedActiveProperties();
        }
    }
}

void CSWSCreature::ActivityManager(unsigned int nActivity)
{
    if (nActivity == 4) {
        if ((m_activityFlags & ACTIVITY_STEALTH) && !(m_nLockedActivityFlags & ACTIVITY_STEALTH)) {
            m_activityFlags &= ~ACTIVITY_STEALTH;
            SetStealthMode(0, 0);
        }
        if ((m_activityFlags & ACTIVITY_CONVERSATION) && !(m_nLockedActivityFlags & ACTIVITY_CONVERSATION)) {
            m_activityFlags &= ~ACTIVITY_CONVERSATION;
            StopDialog();
        }
        if ((m_activityFlags & ACTIVITY_RESTING) && !(m_nLockedActivityFlags & ACTIVITY_RESTING)) {
            m_activityFlags &= ~ACTIVITY_RESTING;
        }
    } else {
        if (~m_nLockedActivityFlags & ACTIVITY_COMBAT_MODE_MASK) {
            ClearActivities(ACTIVITY_COMBAT_MODE_MASK);
            m_activityFlags &= ~ACTIVITY_COMBAT_MODE_MASK;
        }
        if (nActivity == 1) {
            ClearActivities(0xff);
        }
    }
}

int CSWSCreature::RemoveAction(unsigned int nActionId)
{
    int nRemoved = 0;
    CExoLinkedListNode* pPos = m_actionList.m_pInternal->m_pHead;
    if (pPos != 0) {
        do {
            CSWSObjectActionNode* pNode = static_cast<CSWSObjectActionNode*>(m_actionList.GetAtPos(pPos));
            if (static_cast<unsigned int>(pNode->m_nActionId) == nActionId && ClearAction(pNode, 1) == 1) {
                m_actionList.Remove(pPos);
                nRemoved = 1;
                delete pNode;
                break;
            }
            m_actionList.GetNext(pPos);
        } while (pPos != 0);
    }
    return nRemoved;
}

int CSWSCreature::ConvertGrenadeSpellID(int nSpellId)
{
    CSWItem* pItem = m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTARM);
    if ((pItem != 0 && pItem->GetBaseItem()->m_nItemType == ITEM_TYPE_WRIST_LAUNCHER) ||
        ((pItem = m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTARM)) != 0 &&
         pItem->GetBaseItem()->m_nItemType == ITEM_TYPE_WRIST_LAUNCHER)) {
        switch (nSpellId) {
        case SPELL_GRENADE_FRAGMENTATION: nSpellId = SPELL_GRENADE_LAUNCHED_FRAGMENTATION; break;
        case SPELL_GRENADE_STUN: nSpellId = SPELL_GRENADE_LAUNCHED_STUN; break;
        case SPELL_GRENADE_THERMAL_DETONATOR: nSpellId = SPELL_GRENADE_LAUNCHED_THERMAL_DETONATOR; break;
        case SPELL_GRENADE_POISON: nSpellId = SPELL_GRENADE_LAUNCHED_POISON; break;
        case SPELL_GRENADE_SONIC: nSpellId = SPELL_GRENADE_LAUNCHED_SONIC; break;
        case SPELL_GRENADE_ADHESIVE: nSpellId = SPELL_GRENADE_LAUNCHED_ADHESIVE; break;
        case SPELL_GRENADE_CRYOBAN: nSpellId = SPELL_GRENADE_LAUNCHED_CRYOBAN; break;
        case SPELL_GRENADE_FIRE: nSpellId = SPELL_GRENADE_LAUNCHED_FIRE; break;
        case SPELL_GRENADE_ION: nSpellId = SPELL_GRENADE_LAUNCHED_ION; break;
        case SPELL_GRENADE_SONIC_2: nSpellId = SPELL_GRENADE_LAUNCHED_SONIC_2; break;
        case SPELL_GRENADE_SONIC_3: nSpellId = SPELL_GRENADE_LAUNCHED_SONIC_3; break;
        }
    }
    return nSpellId;
}

int CSWSCreature::UnDisguise(CGameEffect* pEffect)
{
    K2_UNUSED(pEffect);
    if (m_bDisguised != 0) {
        m_pStats->m_nAppearanceType = m_nDisguiseAppearance;
        m_appearance.m_headId = m_nDisguiseAppearance;
        UpdatePersonalSpace();
        UpdateAppearanceDependantInfo(1);
        m_bDisguised = 0;
        CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
        if (pArea != 0) {
            Vector vNewPosition = {};
            if (pArea->ComputeSafeLocation(m_position, 20.0f, m_pPathfindInfo, 1, &vNewPosition) != 0) {
                SetPosition(vNewPosition, 1, 1, 0);
            } else {
                vNewPosition = m_position;
            }
        }
    }
    return 1;
}

void CSWSCreature::SaveAttackerList(CResGFF* pGFF, CResStruct* pStruct)
{
    if (m_pAttackerList != 0) {
        CResList list;
        pGFF->AddList(&list, pStruct, const_cast<char*>("AssociateList"));
        for (int i = 0; i < m_pAttackerList->m_count; i++) {
            unsigned long long attackerId = m_pAttackerList->m_pArray[i];
            CSWSCreature* pAttacker =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(attackerId);
            if (pAttacker != 0) {
                CResStruct elem;
                pGFF->WriteFieldDWORD(&elem, static_cast<unsigned int>(pAttacker->m_objectId), const_cast<char*>("ObjectId"));
            }
        }
    }
}

void CSWSCreature::LoadAttackerList(CResGFF* pGFF, CResStruct* pStruct)
{
    CResList list;
    if (pGFF->GetList(&list, pStruct, const_cast<char*>("AttackerList")) == 1) {
        m_pAttackerList = new CExoArrayList<unsigned long long>();
        if (pGFF->GetListCount(&list) != 0) {
            unsigned int i = 0;
            do {
                CResStruct elem;
                pGFF->GetListElement(&elem, &list, i);
                int bSuccess;
                unsigned long long attackerId =
                    pGFF->ReadFieldDWORD(&elem, const_cast<char*>("ObjectId"), bSuccess, OBJECT_INVALID);
                m_pAttackerList->Add(attackerId);
                i++;
            } while (i < pGFF->GetListCount(&list));
        }
    }
}

float CSWSCreature::GetSpotCheckDistance()
{
    float fDistance = m_fPerceptionPrimaryRange;
    if (m_bCombatState != 0) {
        CServerExoApp* pServerExoApp = g_pAppManager->m_pServerExoApp;
        if (pServerExoApp->GetCreatureByGameObjectID(pServerExoApp->GetPlayerCreatureId()) != this) {
            C2DA* pRanges = g_pRules->m_p2DArrays->m_pRanges;
            CExoString col("PrimaryRange");
            pRanges->GetFLOATEntry(RANGES_ROW_PARTY_COMBAT, col, &fDistance);
        }
    }
    return fDistance;
}

float CSWSCreature::GetListenCheckDistance()
{
    float fDistance = m_fPerceptionSecondaryRange;
    if (m_bCombatState != 0) {
        CServerExoApp* pServerExoApp = g_pAppManager->m_pServerExoApp;
        if (pServerExoApp->GetCreatureByGameObjectID(pServerExoApp->GetPlayerCreatureId()) != this) {
            C2DA* pRanges = g_pRules->m_p2DArrays->m_pRanges;
            CExoString col("SecondaryRange");
            pRanges->GetFLOATEntry(RANGES_ROW_PARTY_COMBAT, col, &fDistance);
        }
    }
    return fDistance;
}

void CSWSCreature::UpdateAppearanceForEquippedItems()
{
    CSWSItem* pItem = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
    unsigned long long id0 = (pItem != 0) ? pItem->m_objectId : CGameObjectArray::INVALIDOBJID;
    pItem = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    unsigned long long id1 = (pItem != 0) ? pItem->m_objectId : CGameObjectArray::INVALIDOBJID;
    pItem = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_BODY));
    unsigned long long id2 = (pItem != 0) ? pItem->m_objectId : CGameObjectArray::INVALIDOBJID;
    pItem = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_HEAD));
    unsigned long long id3 = (pItem != 0) ? pItem->m_objectId : CGameObjectArray::INVALIDOBJID;

    if (id0 != m_appearance.m_itemId[0].m_a || id1 != m_appearance.m_itemId[0].m_b ||
        id2 != m_appearance.m_itemId[1].m_a || id3 != m_appearance.m_itemId[1].m_b) {
        m_appearance.m_itemId[0].m_a = id0;
        m_appearance.m_itemId[0].m_b = id1;
        m_appearance.m_itemId[1].m_a = id2;
        m_appearance.m_itemId[1].m_b = id3;
    }
}

void CSWSCreature::DisplayFloatyDataToSelf(uint64_t nData)
{
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
    if (pPlayer != 0) {
        CSWCCMessageData* pData = new CSWCCMessageData();
        pData->SetObjectID(0, m_objectId);
        pData->SetInteger(9, FEEDBACK_FLOATING_TEXT_STRREF);
        pData->SetInteger(0, static_cast<int>(nData));
        g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerCCMessage(
            pPlayer->m_nPlayerId, CC_MESSAGE_FEEDBACK, pData, 0);
        delete pData;
    }
}

void CSWSCreature::AddPureGoodPowers()
{
    RemovePureGoodPowers();
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_PUREGOODPOWERS);
    pEffect->m_durationTypeBits =
        CGameEffect::DURATION_TYPE_INNATE | (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(m_objectId);
    pEffect->m_spellId = -1;
    pEffect->m_durationTypeBits &= ~CGameEffect::SUBTYPE_MASK;
    ApplyEffect(pEffect, 0, 0);
}

void CSWSCreature::AddPureEvilPowers()
{
    RemovePureEvilPowers();
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_PUREEVILPOWERS);
    pEffect->m_durationTypeBits =
        CGameEffect::DURATION_TYPE_INNATE | (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(m_objectId);
    pEffect->m_spellId = -1;
    pEffect->m_durationTypeBits &= ~CGameEffect::SUBTYPE_MASK;
    ApplyEffect(pEffect, 0, 0);
}

int CSWSCreature::GetHasSpike(unsigned char bParam)
{
    CItemRepository* pRepo;
#if KOTOR1  // K1: the out-of-line GetItemRepository(1)
    pRepo = GetItemRepository(1);
#else
    if ((g_pAppManager->m_pClientExoApp->GetInGameGui()->GetTopMenuOverrideCharacter() == -1) &
        (m_bInParty == 0)) {
        pRepo = m_pItemRepository;
    } else {
        pRepo = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems();
    }
#endif
    if (pRepo == 0)
        return 0;
    for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++) {
            CSWSItem* pItem = pRepo->ItemListGetItem(i);
            if (pItem != 0 && static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_SPIKES) {
                unsigned short nType;
#if KOTOR1
                if (bParam == 1)
                    nType = ITEM_PROPERTY_SECURITY_SPIKE;
                else if (bParam == 0)
                    nType = ITEM_PROPERTY_COMPUTER_SPIKE;
                else
                    continue;
#else
                if (bParam == 0) {
                    nType = ITEM_PROPERTY_COMPUTER_SPIKE;
                } else {
                    if (bParam != 1)
                        continue;
                    nType = ITEM_PROPERTY_SECURITY_SPIKE;
                }
#endif
                if (pItem->GetPropertyByTypeExists(nType, 0) != 0)
                    return 1;
            }
    }
    return 0;
}

CSWSCreature::CSWSCreature(unsigned long long objectId, int a)
    : CSWSObject(CGameObject::OBJECT_TYPE_CREATURE, objectId, a)
    , m_vDesiredAreaLocation{0.0f, 0.0f, 0.0f}
    , m_vLastBlockedPos{0.0f, 0.0f, 0.0f}
#if !KOTOR1
    , m_vDialogStartPosition()
    , m_vDialogStartOrientation()
#endif
{
#if !KOTOR1  // K1 ctor: none of the K2-only members below
    m_lastRoomHint = -1;
    m_nAssignedPup = -1;
    m_isPuppet = 0;
#endif
    m_bInParty = 0;
    m_nSoundSetFile = 0xffff;

    m_pStats = new CSWSCreatureStats(this);
    m_pPathfindInfo = new CPathfindInformation();
    m_pPathfindInfo->m_oidCreature = m_objectId;
    m_pCombatRound = new CSWSCombatRound(this);
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddObject(this, 0);

    m_ambientAnimationState = 0;
    m_bCombatState = 0;
    SetAnimation(SERVER_ANIMATION_READY);

    for (int i = 0; i < 14; i++)
        m_scripts[i] = "default";

    m_nLastHeartbeatDay = 0;
    m_nLastHeartbeatTod = 0;
    m_nHeartbeatDelay = rand_wincompatible() % 1200 + 3000;
    m_nLastPerceptionDay = 0;
    m_nLastPerceptionTod = 0;
    m_nPerceptionDelay = rand_wincompatible() % 400 + 300;

    m_pInventory = new CSWInventory();
    m_pItemRepository = new CItemRepository(m_objectId, 0);

    m_appearance = CSWSCreatureAppearanceInfo();
    m_appearance.m_itemId[0].m_a = OBJECT_INVALID;
    m_appearance.m_itemId[0].m_b = OBJECT_INVALID;
    m_appearance.m_itemId[1].m_a = OBJECT_INVALID;
    m_appearance.m_itemId[1].m_b = OBJECT_INVALID;
    m_appearance.m_nAppearanceHead = 1;
    m_appearance.m_bUseBackupHead = 0;
    m_appearance.m_nDuplicatingHead = 0xff;

    m_lockOrientationToObjectId = CGameObjectArray::INVALIDOBJID;
    m_castLevel = 0;
    m_nReserved5 = 0;
    m_nItemIteratorIndex = 0;
    m_nContainerItemIteratorIndex = 0;
    m_oidItemIteratorContainer = CGameObjectArray::INVALIDOBJID;
    m_activityFlags = 0;
    m_nLockedActivityFlags = 0;
    m_bJumpPending = 0;
    m_bLightsaberThrowActive = 0;
    m_bDriving = 0;
    m_movementRateFactor = 1.0f;
    m_fDriveSpeedFactor = 1.0f;
    m_nAIState = 0xffff;
    m_nState = EFFECT_STATE_NONE;
    m_nLastMineCheckDay = 0;
    m_nLastMineCheckTod = 0;
    m_nReserved7 = 0;
    m_nReserved8 = 0;
    m_bDisarmable = 1;
    m_silentMoveCount = 0;
    m_nCombatTimer = 0;
#if !KOTOR1
    m_bFakeCombat = 0;
#endif
    m_nActionQueueUpdateDay = 0;
    m_nActionQueueUpdateTod = 0;
    m_nReserved9 = 0;
    m_detectMode = 0;
    m_stealthMode = 0;
    m_combatMode = CREATURE_COMBAT_MODE_NONE;
    m_pendingCombatMode = CREATURE_COMBAT_MODE_NONE;
    m_bInitiativeExpired = 1;
    m_currentAreaId = OBJECT_INVALID;
    m_vDesiredAreaLocation.x = 0.0f;
    m_vDesiredAreaLocation.y = 0.0f;
    m_vDesiredAreaLocation.z = 0.0f;
    m_bDesiredAreaUpdateComplete = 1;
    m_bSpawnPerceptionDone = 0;
    m_nReserved12 = 0;
    m_nAIStateAction = 0;
    m_oidAIStateTarget = OBJECT_INVALID;
    m_nCurrentActionId = AIACTION_NONE;
    m_attackTargetId = OBJECT_INVALID;

    unsigned int nDay;
    unsigned int nTod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);
    m_nLastAIUpdateDay = nDay;
    m_nLastAIUpdateTod = nTod;

    m_pStandardFactionReputations = 0;
    m_oidEncounter = OBJECT_INVALID;
    m_effectSpellId = static_cast<unsigned int>(-1);
    AddToVisibleList(m_objectId, 1, 1, 0, 0);

    m_nLastVisibilityUpdateDay = 0;
    m_nLastVisibilityUpdateTod = 0;
    m_nEndRoundScriptTimer = 3000;
    m_nAnimationLoopTimer = 1000;
    m_broadcastedAOO = 0;
    m_nBroadcastedAOOTimer = 0;
    m_nUserDefinedEventNumber = -1;
    m_nVisionFlags = 0;
    m_bTaunted = 0;
    m_bPoisoned = 0;
    m_bDiseased = 0;
    m_nGuardStanceFeat = 0;
    m_bAssuredDeflection = 0;
    m_bAssuredDeflectionRedirect = 0;
    m_bAssuredHit = 0;
    m_bSlowed = 0;
    m_bHasted = 0;
    m_bSilenced = 0;
    m_idPoisonEffect = 0xffffffffffffffffULL;
#if !KOTOR1
    m_nStancePosture = STANCE_POSTURE_NONE;
    m_stanceId = 0;
#endif
    m_bHasArms = 1;
    m_bHasLegs = 1;
    m_nCreatureSize = CREATURE_SIZE_INVALID;

    m_sModelType = "";

    m_fWeaponScale = 0.0f;
    m_fPrefAttackDist = 0.0f;
    m_counterSpellTargetId = OBJECT_INVALID;
#if !KOTOR1
    m_bDoorSaberStarted = 0;
#endif
    m_nLastSkillRollDay = 0;
    m_nLastSkillRollTod = 0;
    m_nReserved11 = 0;
    m_bDropItemStarted = 0;
    m_bPickUpItemStarted = 0;
    m_bUnlockStarted = 0;
    m_bLockStarted = 0;
    m_bHealAnimPlayed = 0;
    m_bMineActionStarted = 0;
    m_bPickPocketStarted = 0;
    m_bPickPocketApproachDone = 0;

    m_nStealthMoveRoll = rand_wincompatible() % 10 + 1;
    m_nStealthHideRoll = rand_wincompatible() % 10 + 1;
    m_nSpotRoll = rand_wincompatible() % 10 + 1;
    m_nListenRoll = rand_wincompatible() % 10 + 1;

    for (int i = 0; i < 8; i++)
        m_anTimers[i] = 0;
    m_nExcitedStateTimer = 0;
    m_nMovementSpeedDebugTimer = 0;
    m_nRestEndDay = 0;
    m_nRestEndTime = 0;
    m_nRestStartDay = 0;
    m_nRestStartTime = 0;

    m_oidAttackTarget = OBJECT_INVALID;
    m_oidAttemptedMovementTarget = OBJECT_INVALID;
    m_attemptedAttackTargetId = OBJECT_INVALID;
    m_oidPendingAttackTarget = OBJECT_INVALID;
    m_vLastBlockedPos.x = 0.0f;
    m_vLastBlockedPos.y = 0.0f;
    m_vLastBlockedPos.z = 0.0f;
    m_oidGoingToBeAttackedBy = OBJECT_INVALID;
    m_oidAttemptedSpellTarget = OBJECT_INVALID;
    m_oidSpellTarget = OBJECT_INVALID;
    m_oidLastHostileTarget = OBJECT_INVALID;
    m_nCurrentForcePower = -1;
    m_nLastForcePowerUsed = -1;
    m_nCurrentAttackAction = -1;
    m_nLastAttackAction = -1;
    m_nCurrentCombatFeat = 0;
    m_nLastCombatFeatUsed = 0;
    m_nReserved10 = 0;
    m_bWasForcePowerSuccessful = -1;
#if !KOTOR1
    m_oidHealTarget = OBJECT_INVALID;
#endif
    m_bMovementSpeedLimited = 0;
    m_nSpecialWalkAnimation = 0;
    m_bInvitedToParty = 0;
    m_oidInvitedToPartyBy = OBJECT_INVALID;
    m_bPassiveAttackBehaviour = 0;
    m_bSpellChargeSpent = 0;
    m_pJournal = 0;
    m_scriptVars.m_pOwner = this;
    m_nGold = 0;
    m_bPlayerCharacter = 0;
#if !KOTOR1
    m_nComponent = 0;
    m_nChemical = 0;
#endif
    m_pInvitationsIgnored = 0;
    m_pInvitationsOffered = 0;
    m_nLastCastSpellId = -1;
    m_nLastSpellCastClass = CSWSObject::CAST_CLASS_NONE;
    m_nLastSpellCastMetaType = 0;
    m_bEncounterSlotReleased = 0;
    m_nItemCastSpellLevel = -1;
    m_bItemCastSpell = 0;
    m_lastSpellItemId = OBJECT_INVALID;
    m_oidLastMineDetected = OBJECT_INVALID;
    m_bItemCastStarted = 0;
    m_fPerceptionSecondaryRange = 0.0f;
    m_fPerceptionPrimaryRange = 0.0f;
    m_nEquippedWeight = 0;
    m_bForcePlayerUpdate = 1;
    m_nEncumbranceState = 0;

    m_nAILevel0PerceptionSkips = rand_wincompatible() % 100;
    m_nAILevel0HeartbeatSkips = rand_wincompatible() % 100;

    m_inTransit = 0;
    m_bDisguised = 0;
    m_nDisguiseAppearance = 0;
    m_bSkipPartyAIStateUpdate = 0;
    m_bUpdateCombatInformation = 0;
    m_nCharSheetViewers = 0;
    m_bBodyBag = 0;
    m_bActionApproachDone = 0;
    m_nReserved13 = 0;
    m_nReserved14 = 0;
    m_pFollowInfo = 0;
    m_nMoveState = -1;
    m_nNextAttackerIdx = 0;
    m_pAttackerList = 0;

    SetFadeOutTime(8000);

    m_fForcePushDistance = 0.0f;
    m_fForcePushHeight = 0.0f;
    m_bRunning = 0;
    m_nStealthXPTimer = 0;
    m_bBodyFuel = 0;
    m_bSuppressPerceptionEvents = 0;

    SetActivity(ACTIVITY_DETECT, 1, 0);

    m_nJoiningXP = 0;
    m_bLastInstantEquipLeft = 0;
    m_bLastInstantEquipRight = 0;
    m_nItemUseCooldown = 0;
    m_lightSaberThrowRate = 37.0f;
    m_bForceEquipClothing = 0;
    m_bStealthNoAnimation = 0;
    m_nCombatType = 0;
    m_bDisableInjuredAnim = 0;
#if !KOTOR1
    m_nReserved15 = 0;
    m_nBonusForcePoints = 0;
    m_nStanceStartDay = 0;
    m_nStanceStartTime = 0;
#endif
    m_bIgnoreCrePath = 0;
#if !KOTOR1
    m_vDialogStartOrientation.x = 0.0f;
    m_vDialogStartOrientation.y = 0.0f;
    m_vDialogStartOrientation.z = 0.0f;
    m_vDialogStartPosition.x = 0.0f;
    m_vDialogStartPosition.y = 0.0f;
    m_vDialogStartPosition.z = 0.0f;
    m_bPlayerMadeCharacter = 0;
    m_bRestoreDialogPose = 0;
#endif
}

// Tears the creature down in order: leaves the area, deletes every repository and equipped item,
// leaves the faction (guarded by the m_bPlayerCharacter is-the-player latch, which stamps
// m_pStats->m_factionId = -1), deletes the owned members, drains the visibility list, and removes
// it from the AI master and the module lookup. The embedded list and string destructors follow,
// then the CSWSObject base call.
CSWSCreature::~CSWSCreature()
{
    if (GetArea() != NULL)
        RemoveFromArea(0);

    for (int i = m_pItemRepository->ItemListGetCount() - 1; i >= 0; i--) {
        CSWSItem* pItem = m_pItemRepository->ItemListGetItem(i);
        if (pItem != NULL)
            delete pItem;
    }

#if KOTOR1  // K1: 18 equipment slots
    for (int i = 0; i < 18; i++) {
#else
    for (int i = 0; i < 20; i++) {
#endif
        CSWItem* pItem = m_pInventory->GetItemInSlot(1 << i);
        if (pItem != NULL)
            delete pItem;
    }

    CSWSFaction* pFaction = GetFaction();
    if (pFaction != NULL && m_bPlayerCharacter == 0) {
        pFaction->RemoveMember(m_objectId);
        m_pStats->m_factionId = -1;
    }

    if (m_pStats != NULL) {
        delete m_pStats;
        m_pStats = NULL;
    }

    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->RemoveObject(this);

    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule != NULL)
        pModule->RemoveObjectFromLookupTable(m_sTag, m_objectId);

    if (m_pPathfindInfo != NULL) {
        delete m_pPathfindInfo;
        m_pPathfindInfo = NULL;
    }

    if (m_pCombatRound != NULL) {
        delete m_pCombatRound;
        m_pCombatRound = NULL;
    }

    if (m_pItemRepository != NULL) {
        delete m_pItemRepository;
        m_pItemRepository = NULL;
    }

    if (m_pInventory != NULL) {
        delete m_pInventory;
        m_pInventory = NULL;
    }

    if (m_pStandardFactionReputations != NULL) {
        delete m_pStandardFactionReputations;
        m_pStandardFactionReputations = NULL;
    }

    for (int i = m_visibilityList.m_count - 1; i >= 0; i--) {
        CSWVisibilityNode* pNode = m_visibilityList.m_pArray[i];
        m_visibilityList.DelIndex(i);
        if (pNode != 0)
            operator delete(pNode);
    }

    if (m_pJournal != NULL) {
        delete m_pJournal;
        m_pJournal = NULL;
    }

    if (m_pFollowInfo != NULL) {
        delete m_pFollowInfo;
        m_pFollowInfo = NULL;
    }

    if (m_pAttackerList != NULL) {
        delete m_pAttackerList;
        m_pAttackerList = NULL;
    }
}

void CSWSCreature::SetLockOrientationToObject(unsigned long long objectId, int a)
{
#if KOTOR1  // K1: the dying test is GetDeadTemp()
    if (a != 0 || (GetDead() == 0 && GetDeadTemp() == 0)) {
#else
    if (a != 0 || (GetDead() == 0 && (m_bInParty == 0 || GetCurrentHitPoints(0) >= 1))) {
#endif
        if (objectId == CGameObjectArray::INVALIDOBJID &&
            m_lockOrientationToObjectId != CGameObjectArray::INVALIDOBJID &&
            m_lockOrientationToObjectId != m_objectId) {
            CGameObject* pObject;
            if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(
                    m_lockOrientationToObjectId, &pObject) == CGameObjectArray::SUCCESS &&
                pObject->m_objectType > CGameObject::OBJECT_TYPE_AREA) {
                Vector vDir;
                vDir.x = static_cast<CSWSObject*>(pObject)->m_position.x - m_position.x;
                vDir.y = static_cast<CSWSObject*>(pObject)->m_position.y - m_position.y;
                vDir.z = static_cast<CSWSObject*>(pObject)->m_position.z - m_position.z;
                vDir = normalize(vDir);
                SetOrientation(vDir);
            }
        }
        m_lockOrientationToObjectId = objectId;
    }
}

void CSWSCreature::SplitItem(CSWSItem* pItem, int nCount)
{
    CItemRepository* pRepo;
#if KOTOR1  // K1: the out-of-line GetItemRepository(1)
    pRepo = GetItemRepository(1);
#else
    if ((g_pAppManager->m_pClientExoApp->GetInGameGui()->GetTopMenuOverrideCharacter() == -1) &
        (m_bInParty == 0))
        pRepo = m_pItemRepository;
    else
        pRepo = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems();
#endif
    if (pItem->m_possessorId != m_objectId) {
        CGameObject* pPossessor = g_pAppManager->m_pServerExoApp->GetGameObject(pItem->m_possessorId);
        if (pPossessor->AsSWSItem() == 0)
            return;
        CSWSItem* pContainer = pPossessor->AsSWSItem();
        if (pContainer->m_possessorId == m_objectId && pContainer->m_pItemRepository != 0)
            pRepo = pContainer->m_pItemRepository;
    }
    CSWSItem* pNewItem = pItem->SplitItem(nCount);
    if (pNewItem != 0 && pRepo->AddItem(&pNewItem, 0, 0, 1) == 0)
        pNewItem->AddToArea(static_cast<CSWSArea*>(GetArea()), m_position.x, m_position.y, m_position.z, 1);
}

int CSWSCreature::GetBodyBagAppearance()
{
    int nAppearance;
    int bGot;
    bGot = g_pRules->m_p2DArrays->m_pBodyBag->GetINTEntry(m_bBodyBag, CExoString("Appearance"),
                                                          &nAppearance);
    if (!bGot) {
        int nBodyBag;
        bGot = g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(m_appearance.m_headId,
                                                                 CExoString("Body_Bag"),
                                                                 &nBodyBag);
        if (!bGot)
            nBodyBag = 0;
        bGot = g_pRules->m_p2DArrays->m_pBodyBag->GetINTEntry(nBodyBag, CExoString("Appearance"),
                                                              &nAppearance);
        if (!bGot)
            nAppearance = 3;
    }
    return nAppearance;
}

int CSWSCreature::AddDropItemActions(unsigned long long oidItem, const Vector& vPosition, int a)
{
    if (g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem) == 0)
        return 0;
    if (GetArea() == 0)
        return 0;
    if (m_bCombatState == 1) {
        CSWSItem* pEquipped = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_BODY));
        if (pEquipped != 0 && pEquipped->m_objectId == oidItem) {
            SendFeedbackMessage(FEEDBACK_ARMOR_UNEQUIP_IN_COMBAT, 0);
            return 0;
        }
    }
    if (m_bCommandable == 0)
        return 0;
    Vector* pPos = const_cast<Vector*>(&vPosition);
    AddAction(AIACTION_DROP_ITEM, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidItem,
              ACTION_PARAM_FLOAT, static_cast<void*>(pPos),
              ACTION_PARAM_FLOAT, static_cast<void*>(&pPos->y),
              ACTION_PARAM_FLOAT, static_cast<void*>(&pPos->z), ACTION_PARAM_INT, &a, 0, 0, 0, 0, 0,
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return 1;
}

int CSWSCreature::ReplaceEquipItemActions(CSWSItem* pItem, unsigned int nSlot,
                                          int bSkipCombatRound, int nEquipFlag)
{
    unsigned long long oidItem = pItem->m_objectId;
    if (bSkipCombatRound == 0) {
        if (m_pCombatRound->m_pRoundActionList != 0) {
            CExoLinkedListNode* pNode =
                m_pCombatRound->m_pRoundActionList->m_pInternal->m_pHead;
            while (pNode != 0) {
                CSWSCombatRoundAction* pAction = static_cast<CSWSCombatRoundAction*>(
                    m_pCombatRound->m_pRoundActionList->GetAtPos(pNode));
                if (pAction != 0 && pAction->m_nActionType == CSWSCombatRoundAction::TYPE_EQUIP &&
                    (pAction->m_nInventorySlot == nSlot || pAction->m_oidTarget == oidItem)) {
                    pAction->m_oidTarget = oidItem;
                    pAction->m_nInventorySlot = nSlot;
                    return 1;
                }
                m_pCombatRound->m_pRoundActionList->GetNext(pNode);
            }
        }
    }
    CExoLinkedListNode* pNode = m_actionList.m_pInternal->m_pHead;
    while (pNode != 0) {
        CSWSObjectActionNode* pAction = static_cast<CSWSObjectActionNode*>(m_actionList.GetAtPos(pNode));
        if (pAction != 0 && pAction->m_nActionId == AIACTION_EQUIP_ITEM &&
            (pAction->m_args.m_equipSlot == nSlot || pAction->m_args.m_itemId == oidItem)) {
            pAction->m_args.m_itemId = oidItem;
            pAction->m_args.m_equipSlot = nSlot;
            pAction->m_args.m_equipFlag = nEquipFlag;
            return 1;
        }
        m_actionList.GetNext(pNode);
    }
    return 0;
}

void CSWSCreature::SetPortraitId(unsigned short nPortraitId)
{
    CExoString sPo("po_");  // constructed but never read; the ctor/dtor calls are deliberate
    if (nPortraitId >= PORTRAIT_ID_CUSTOM) {
        SetPortrait(CResRef(""));
    } else {
        CExoString sEntry;
        C2DA* pPortraits = g_pRules->m_p2DArrays->m_pPortrait;
        pPortraits->GetCExoStringEntry(nPortraitId, CExoString("BaseResRef"), &sEntry);
        sEntry.operator=(sEntry);  // deliberate self-assignment
        SetPortrait(CResRef(static_cast<char*>(sEntry.CStr())));
    }
    m_portraitId = nPortraitId;
}

int CSWSCreature::RequestSell(unsigned long long itemId, unsigned long long storeId)
{
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(itemId);
    if (pItem == 0)
        return 0;
    CSWSStore* pStore = g_pAppManager->m_pServerExoApp->GetStoreByGameObjectID(storeId);
    if (pStore == 0)
        return 0;
    if (pItem->m_pItemRepository != 0 && pItem->m_pItemRepository->ItemListGetCount() != 0) {
        SendFeedbackMessage(FEEDBACK_STORE_CONTAINER_NOT_EMPTY, 0);
        return 0;
    } else if (pItem->m_bPlot == 1) {
        SendFeedbackMessage(FEEDBACK_STORE_PLOT_ITEM, 0);
        return 0;
    }
    CItemRepository* pRepo;
#if KOTOR1  // K1: the out-of-line GetItemRepository(1)
    pRepo = GetItemRepository(1);
#else
    if ((g_pAppManager->m_pClientExoApp->GetInGameGui()->GetTopMenuOverrideCharacter() == -1) &
        (m_bInParty == 0))
        pRepo = m_pItemRepository;
    else
        pRepo = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems();
#endif
    if (pRepo->GetItemInRepository(pItem, 0) != 0)
        return 1;
    if (m_pInventory->GetItemInInventory(pItem) == 1)
        return 1;
    pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pItem->m_possessorId);
    if (pItem == 0)
        return 0;
#if KOTOR1  // K1: the out-of-line GetItemRepository(1)
    pRepo = GetItemRepository(1);
#else
    if ((g_pAppManager->m_pClientExoApp->GetInGameGui()->GetTopMenuOverrideCharacter() == -1) &
        (m_bInParty == 0))
        pRepo = m_pItemRepository;
    else
        pRepo = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyItems();
#endif
    return pRepo->GetItemInRepository(pItem, 0) != 0;
}

void CSWSCreature::AddGold(int nGold, int bFeedback)
{
    int nNew = GetGold() + nGold;
    if (nNew > 999999999)
        nNew = 999999999;
    int nDelta = nNew - GetGold();
    SetGold(nNew);
    if (bFeedback == 1 && nDelta != 0) {
        CSWCCMessageData* pData = new CSWCCMessageData();
        pData->SetInteger(0, nDelta);
        SendFeedbackMessage(FEEDBACK_CREDITS_ACQUIRED, pData);
    }
}

#if !KOTOR1
void CSWSCreature::HandleAutoBalancing()
{
    if (m_nMultiplierSet != 0 && m_bInParty == 0) {
        char cLevel;
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->GetValueNumber(
            CExoString("G_PC_LEVEL"), cLevel);
        m_nPCLevelAtSpawn = cLevel;
        int nHP = static_cast<short>(m_hitPoints);
        float fMult;
        g_pRules->m_p2DArrays->m_pAutoBalance->GetFLOATEntry(
            m_nMultiplierSet, CExoString("VPMult"), &fMult);
        float fLevel = 0.0f;
        if (cLevel >= 2) {
            cLevel--;
            fLevel = static_cast<float>(cLevel - 1);
        }
        m_hitPoints = static_cast<int>(fMult * fLevel) + 1;
        m_hitPoints *= nHP;
        SetCurrentHitPoints(GetMaxHitPoints(1));
    }
}
#endif

void CSWSCreature::AIUpdate()
{
    // The "hurt players" cheat is a one-shot console switch that drops the next creature to tick
    // down to 1 hit point. It disarms itself, so exactly one party member takes the hit.
    if (HurtPlayers != 0 && m_bInParty != 0) {
        HurtPlayers = 0;
        SetCurrentHitPoints(1);
    }

    if (RenderAi != 0) {
        CSWSCreature* pPlayer = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
            g_pAppManager->m_pServerExoApp->GetPlayerCreatureId());
        m_sTag.CStr();
        if (pPlayer != NULL)
            magnitude(m_position - pPlayer->m_position);
    }

    long long nHighResTime = g_pExoBase->m_pTimers->GetHighResolutionTimer();

    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    CServerAIMaster* pAIMaster = pServerApp->GetServerAIMaster();
    CWorldTimer* pWorldTimer = pServerApp->GetWorldTimer();

    // The player character and anything flagged party-side is pinned to the busiest AI bucket.
    if ((m_pStats->m_bIsPC == 1 || m_bInParty != 0) && m_aiLevel != 4)
        pAIMaster->SetAILevel(this, 4);

    SpawnInHeartbeatPerception(1);

    unsigned int nDay = 0;
    unsigned int nTimeOfDay = 0;
    unsigned int nDaysSince = 0;
    unsigned int nMsSince = 0;

    pServerApp->GetActiveTimer(m_objectId)->GetWorldTime(&nDay, &nTimeOfDay);
    pWorldTimer->SubtractWorldTimes(nDay, nTimeOfDay, m_nLastAIUpdateDay, m_nLastAIUpdateTod,
                                    &nDaysSince, &nMsSince);
    m_nAIUpdateDeltaMs = nMsSince;

    UpdateCombatRoundTimer();
    UpdateExcitedStateTimer();
    UpdateEffectList(nDay, nTimeOfDay);
    RunActions(nDay, nTimeOfDay, nHighResTime);
    m_nLastAIUpdateDay = nDay;
    m_nLastAIUpdateTod = nTimeOfDay;

#if KOTOR1  // K1 holds no area local: every use below re-calls GetArea() (16 calls)
    if (GetArea() != NULL) {
#else
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (pArea != NULL) {
#endif
        // Re-snap to the walkmesh every frame. ComputeHeight also refreshes m_lastRoomHint and
        // hands back the room it landed in; a NULL room means the creature has drifted off the
        // mesh entirely.
        CSWSRoom* pRoom = NULL;
        Vector vPos = m_position;
#if KOTOR1
        vPos.z = static_cast<CSWSArea*>(GetArea())->ComputeHeight(m_position, 1, &pRoom);
#else
        vPos.z = pArea->ComputeHeight(m_position, 1, &pRoom, &m_lastRoomHint);
#endif

        // When off the mesh and party-side, rather than leave the member stranded, teleport it to a
        // safe spot near the party leader, but only when nothing on screen would notice (no
        // cutscene, no dialogue), and never for the player's own creature.
        if (pRoom == NULL && m_bInParty != 0
            && m_objectId != g_pAppManager->m_pServerExoApp->GetPlayerCreatureId()
            && g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState == 0
            && g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bAnimatedCut == 0) {
            int nIndex = g_pAppManager->m_pClientExoApp->GetSWParty()->GetIndex(
                g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId));
            if (nIndex != -1) {
                vPos = g_pAppManager->m_pClientExoApp->GetSWParty()->m_members[nIndex].m_vPathPoint;
#if KOTOR1
                if (static_cast<CSWSArea*>(GetArea())->ComputeSafeLocation(vPos, 5.0f, m_pPathfindInfo, 1,
                                                                &vPos) != 0)
                    vPos.z = static_cast<CSWSArea*>(GetArea())->ComputeHeight(m_position, 1, &pRoom);
#else
                if (pArea->ComputeSafeLocation(vPos, 5.0f, m_pPathfindInfo, 1, &vPos) != 0)
                    vPos.z = pArea->ComputeHeight(m_position, 1, &pRoom, NULL);
#endif
            }
        }

        if ((m_nMoveState & ~1) == 4)
            vPos.z += m_fForcePushHeight;
        SetPosition(vPos, 1, 0, 0);
#if KOTOR1  // K1: the mine check re-tests GetArea() after the AI-state pass
    }
    ComputeAIState();
    if (GetArea() != NULL)
        UpdateMineCheck();
#else
        ComputeAIState();
        UpdateMineCheck();
    } else {
        ComputeAIState();
    }
#endif

    if (m_bCombatState == 1) {
        if (m_nCombatType == 1)
            SetActivity(ACTIVITY_STEALTH, 0, 0);
        if (m_nCombatTimer <= 0) {
#if KOTOR1
            SetCombatState(0, m_nCombatType);
#else
            SetCombatState(0, m_nCombatType, 0);
#endif
#if KOTOR1  // K1: no SetCombatState hold flag
        } else {
#else
        } else if (m_bFakeCombat == 0) {
#endif
            m_nCombatTimer -= m_nAIUpdateDeltaMs;
        }
    }

    if (m_nItemUseCooldown != 0)
        m_nItemUseCooldown = (static_cast<unsigned int>(m_nItemUseCooldown) > m_nAIUpdateDeltaMs) ? (m_nItemUseCooldown - m_nAIUpdateDeltaMs) : 0;

    if (m_bCombatState == 1) {
        int nEndRound = m_nEndRoundScriptTimer;
        if (nEndRound <= 0) {
#if KOTOR1  // K1: the timer runs, but the end-round script is never fired (GetDead kept)
            GetDead();
#else
            if (GetDead() == 0)
                g_pVirtualMachine->RunScript(&m_scripts[CREATURE_SCRIPT_ENDROUND], m_objectId, 1);
#endif
            m_nEndRoundScriptTimer = 3000;
            nEndRound = 3000;
        }
        m_nEndRoundScriptTimer = nEndRound - m_nAIUpdateDeltaMs;
    }

#if KOTOR1
    if (GetDead() == 0) {
        int bDying = 0;
        int nCount = g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount;
        for (int i = 0; i < nCount; i++) {
            CSWCCreature* pCharacter = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(i);
            unsigned long long nClientId =
                g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId);
            if (pCharacter != 0 && pCharacter->m_objectId == nClientId) {
                if (GetCurrentHitPoints(0) < 1)
                    bDying = 1;
                break;
            }
        }
        if (bDying == 0 && m_bInParty != 0) {
            int nRow = (m_bCombatState == 0) ? 1 : (m_nCombatType == 2);
            float fHealthRegen = 0.0f;
            g_pRules->m_p2DArrays->m_pRegeneration->GetFLOATEntry(
                nRow, CTwoDimArrays::COL2DA_REGENERATION_HEALTHREGEN, &fHealthRegen);
            short nMaxHP = GetMaxHitPoints(1);
            double dHP = static_cast<double>(fHealthRegen) / 100.0 * static_cast<double>(nMaxHP);
            float fHPRate = static_cast<float>(dHP < 0.0001f ? 0.0001f : dHP);
            float fHPGain = static_cast<float>(m_nAIUpdateDeltaMs) / 1000.0f * fHPRate;
            short nCurrent = GetCurrentHitPoints(1);
            short nWhole = m_pStats->AdjustHPAccumulatorBy(fHPGain);
            float fNew = static_cast<float>(nWhole + nCurrent);
            float fMax = static_cast<float>(GetMaxHitPoints(1));
            SetCurrentHitPoints(static_cast<int>(fNew > fMax ? fMax : fNew));

            int nTotalFP = GetMaxForcePoints() + static_cast<short>(m_pStats->m_nTemporaryForcePoints);
            if (GetMaxForcePoints() > 0) {
                float fForceRegen = 0.0f;
                g_pRules->m_p2DArrays->m_pRegeneration->GetFLOATEntry(
                    nRow, CTwoDimArrays::COL2DA_REGENERATION_FORCEREGEN, &fForceRegen);
                double dFP = static_cast<double>(fForceRegen) / 100.0 * static_cast<double>(nTotalFP);
                float fFPRate = static_cast<float>(dFP < 0.0001f ? 0.0001f : dFP);
                float fFPGain = static_cast<float>(m_nAIUpdateDeltaMs) / 1000.0f * fFPRate;
                short nCurrentFP = m_pStats->m_nCurrentForcePoints + m_pStats->m_nTemporaryForcePoints;
                short nWholeFP = m_pStats->AdjustFPAccumulatorBy(fFPGain);
                int nNewFP;
                if (ZeroForce != 0) {
                    ZeroForce = 0;
                    nNewFP = 0;
                } else {
                    nNewFP = nCurrentFP + nWholeFP;
                }
                m_pStats->m_nCurrentForcePoints = static_cast<unsigned short>(nTotalFP < nNewFP ? nTotalFP : nNewFP);
            }
        }
    }
#else
    if (GetDead() == 0 && m_bInParty != 0 && GetCurrentHitPoints(0) > 0 && m_bInParty != 0) {
        HandleHPRegeneration();
        HandleFPRegeneration();
    }
#endif

    if (RenderHP != 0) {
        m_sTag.CStr();
        GetCurrentHitPoints(0);
        GetMaxHitPoints(1);
        GetMaxForcePoints();
    }

    unsigned long long oidPartyLeader = 0;
#if KOTOR1  // K1: GetSWParty() / GetCharacter(0) re-called per test
    if (g_pAppManager->m_pClientExoApp != NULL
        && g_pAppManager->m_pClientExoApp->GetSWParty() != NULL
        && g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0) != NULL) {
        CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
        if (pLeader->m_objectId != 0)
            oidPartyLeader = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(
                pLeader->m_objectId);
    }
#else
    if (g_pAppManager->m_pClientExoApp != NULL) {
        CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
        if (pParty != NULL) {
            CSWCCreature* pLeader = pParty->GetCharacter(0);
            if (pLeader != NULL && pLeader->m_objectId != 0)
                oidPartyLeader = g_pAppManager->m_pServerExoApp->ClientToServerObjectId(
                    pLeader->m_objectId);
        }
    }
#endif

    if ((RenderDEV | RenderQA) != 0 && m_objectId == oidPartyLeader
        && g_pAppManager->m_pClientExoApp->GetModule() != NULL) {
        g_pAppManager->m_pClientExoApp->GetModule()->m_sModuleName.CStr();
    }

    if (m_broadcastedAOO == 1) {
        if (m_nBroadcastedAOOTimer <= 0) {
            m_broadcastedAOO = 0;
            m_nBroadcastedAOOTimer = 0;
        } else {
            m_nBroadcastedAOOTimer -= m_nAIUpdateDeltaMs;
        }
    }

    if (m_nAnimation == SERVER_ANIMATION_RUN_STRAFE_LEFT || m_nAnimation == SERVER_ANIMATION_RUN_STRAFE_RIGHT || m_nAnimation == SERVER_ANIMATION_RUN) {
        if (m_nAnimationLoopTimer > 0)
            m_nAnimationLoopTimer -= m_nAIUpdateDeltaMs;
        else
            m_nAnimationLoopTimer = 1500;
    }

    // The action-queue snapshot is diffed against the client's copy, so it only needs refreshing a
    // few times a second, and off the pause timer, so a paused game stops re-sending it.
    if (m_bPlayerCharacter == 1) {
        unsigned int nPauseDay;
        unsigned int nPauseTimeOfDay;
        unsigned int nPauseDaysSince;
        unsigned int nPauseMsSince;

        static_cast<CWorldTimer*>(pServerApp->GetPauseTimer())->GetWorldTime(&nPauseDay, &nPauseTimeOfDay);
        pWorldTimer->SubtractWorldTimes(nPauseDay, nPauseTimeOfDay, m_nActionQueueUpdateDay,
                                        m_nActionQueueUpdateTod, &nPauseDaysSince, &nPauseMsSince);
        if (nPauseMsSince >= 300) {
            UpdateActionQueue();
            m_nActionQueueUpdateDay = nPauseDay;
            m_nActionQueueUpdateTod = nPauseTimeOfDay;
        }
    }

    for (int i = 0; i < 8; i++) {
        if (m_anTimers[i] != 0)
            m_anTimers[i] = (static_cast<unsigned int>(m_anTimers[i]) > m_nAIUpdateDeltaMs) ? (m_anTimers[i] - m_nAIUpdateDeltaMs) : 0;
    }

    if (g_bEnableMovementSpeedDebugging == 1) {
        if (static_cast<unsigned int>(m_nMovementSpeedDebugTimer) > m_nAIUpdateDeltaMs)
            m_nMovementSpeedDebugTimer -= m_nAIUpdateDeltaMs;
        else
            m_nMovementSpeedDebugTimer = 8000;
    }

    if (m_bUpdateCombatInformation == 1) {
        m_pStats->UpdateCombatInformation();
        m_bUpdateCombatInformation = 0;
    }

    // Demote out of the combat bucket once combat is over: creatures standing in an area whose
    // own gate is set stay one notch up, everything else drops to the idle bucket.
    if (m_bCombatState == 1) {
        if (m_aiLevel == 0 || m_aiLevel == 1)
            pAIMaster->SetAILevel(this, 2);
#if KOTOR1
    } else if (m_aiLevel == 2 && GetArea() != NULL) {
        if (static_cast<CSWSArea*>(GetArea())->m_nPlayersInArea != 0)
#else
    } else if (m_aiLevel == 2 && pArea != NULL) {
        if (pArea->m_nPlayersInArea != 0)
#endif
            pAIMaster->SetAILevel(this, 1);
        else
            pAIMaster->SetAILevel(this, 0);
    }

    // The stealth-XP bleed. -1 is the "armed but never expires" sentinel, so it is tested apart
    // from the countdown; both routes award and re-disarm.
#if KOTOR1  // K1: one shared expiry block, the current XP read once.
    if (GetArea() != NULL) {
        if (static_cast<CSWSArea*>(GetArea())->m_bStealthXPEnabled == 0) {
            m_nStealthXPTimer = 0;
        } else if (m_nStealthXPTimer != 0) {
            if (m_nStealthXPTimer != -1 && static_cast<unsigned int>(m_nStealthXPTimer) > m_nAIUpdateDeltaMs) {
                m_nStealthXPTimer -= m_nAIUpdateDeltaMs;
            } else {
                unsigned int nXP = static_cast<CSWSArea*>(GetArea())->m_currentStealthXP;
                if (nXP < static_cast<unsigned int>(static_cast<CSWSArea*>(GetArea())->m_stealthXPDecrement))
                    nXP = 0;
                else
                    nXP -= static_cast<CSWSArea*>(GetArea())->m_stealthXPDecrement;
                static_cast<CSWSArea*>(GetArea())->SetCurrentStealthXP(nXP);
                m_nStealthXPTimer = 0;
            }
        }
    }
#else
    if (pArea != NULL) {
        if (pArea->m_bStealthXPEnabled == 0) {
            m_nStealthXPTimer = 0;
        } else switch (m_nStealthXPTimer) {
            case -1:
                pArea->SetCurrentStealthXP(
                    pArea->m_currentStealthXP > static_cast<unsigned int>(pArea->m_stealthXPDecrement)
                        ? pArea->m_currentStealthXP - pArea->m_stealthXPDecrement
                        : 0);
                m_nStealthXPTimer = 0;
                break;
            // 0 means "not armed": nothing to bleed and no XP to award.
            case 0:
                break;
            default:
                if (static_cast<unsigned int>(m_nStealthXPTimer) > m_nAIUpdateDeltaMs) {
                    m_nStealthXPTimer -= m_nAIUpdateDeltaMs;
                } else {
                    pArea->SetCurrentStealthXP(
                        pArea->m_currentStealthXP > static_cast<unsigned int>(pArea->m_stealthXPDecrement)
                            ? pArea->m_currentStealthXP - pArea->m_stealthXPDecrement
                            : 0);
                    m_nStealthXPTimer = 0;
                }
                break;
        }
    }
#endif
}

void CSWSCreature::SpawnInHeartbeatPerception(int bFromHeartbeat)
{
    unsigned int nDay = 0;
    unsigned int nTimeOfDay = 0;
    unsigned int nDaysSince = 0;
    unsigned int nMsSince = 0;
    unsigned int nAIDaysSince = 0;
    unsigned int nAIMsSince = 0;

    CServerAIMaster* pAIMaster = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
    g_pAppManager->m_pServerExoApp->GetActiveTimer(m_objectId)->GetWorldTime(&nDay, &nTimeOfDay);
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTimeOfDay, m_nLastHeartbeatDay, m_nLastHeartbeatTod, &nDaysSince, &nMsSince);

    // Once ever, before anything else, rescale to the party's level, then fire OnSpawn.
    if (m_bSpawnPerceptionDone == 0) {
#if !KOTOR1
        HandleAutoBalancing();
#endif
        g_pVirtualMachine->RunScript(&m_scripts[CREATURE_SCRIPT_SPAWN], m_objectId, 1);
#if KOTOR1
        if (m_oidEncounter == OBJECT_INVALID && m_bInParty == 0)
            m_bSpawnPerceptionDone = 1;
#endif
    }

    // Bucket A: the heartbeat script pass
    if ((bFromHeartbeat == 1 && nMsSince >= m_nHeartbeatDelay) || m_bSpawnPerceptionDone == 0) {
        // An m_aiLevel == 0 creature only gets a heartbeat every (delay >> 6) skipped ticks.
        m_nAILevel0HeartbeatSkips++;
        if (m_aiLevel != 0 || m_nAILevel0HeartbeatSkips >= (m_nHeartbeatDelay >> 6)
                || m_bSpawnPerceptionDone == 0) {
            m_nAILevel0HeartbeatSkips = 0;
            if (m_bSpawnPerceptionDone == 0 || m_nLastHeartbeatTod != 0) {
#if KOTOR1  // K1: no heartbeat for a dying PC
                if (GetDead() == 0 && GetIsPCDying() == 0) {
#else
                if (GetDead() == 0) {
#endif
                    // States 1 and 16 are the debilitated pair IsDebilitated tests for; a creature
                    // that enters the heartbeat in one of them loses its commandable flag first so
                    // the script cannot queue anything on it.
#if !KOTOR1
                    if (m_nState == EFFECT_STATE_CONFUSED || m_nState == EFFECT_STATE_DROID_CONFUSED)
                        m_bCommandable = 0;
#endif
                    g_pVirtualMachine->RunScript(&m_scripts[CREATURE_SCRIPT_HEARTBEAT], m_objectId,
                                                 1);
                    // ...then the state's own override script, out of StateScripts.2da. The four
                    // live states map onto three rows; every other state has no state script and
                    // skips the lookup entirely (the default arm jumps straight past the block,
                    // which is why nRow's fallback value is materialized before the switch).
                    int nRow = 1;
#if KOTOR1  // K1: no state with a row-3 twin
                    switch (m_nState) {
                    case EFFECT_STATE_FRIGHTENED:
                        nRow = 2;
                        break;
                    case EFFECT_STATE_CONFUSED:
                        nRow = 3;
                        break;
                    case EFFECT_STATE_PSYCHIC_STATIC:
                        break;
                    default:
                        nRow = 0;
                        break;
                    }
#else
                    switch (m_nState) {
                    case EFFECT_STATE_CONFUSED:
                    case EFFECT_STATE_DROID_CONFUSED:
                        nRow = 3;
                        break;
                    case EFFECT_STATE_FRIGHTENED:
                        nRow = 2;
                        break;
                    case EFFECT_STATE_PSYCHIC_STATIC:
                        break;
                    default:
                        nRow = 0;
                        break;
                    }
#endif
                    if (nRow != 0) {
                        CExoString sScript;
                        C2DA* pStateScripts = g_pRules->m_p2DArrays->m_pStateScripts;
                        pStateScripts->GetCExoStringEntry(nRow, CExoString("SCRIPTNAME"), &sScript);
                        g_pVirtualMachine->RunScript(&sScript, m_objectId, 1);
                        m_bCommandable = 0;
                    }
                    CleanInvitationLists();
                }
            }
            m_nLastHeartbeatDay = nDay;
            m_nLastHeartbeatTod = nTimeOfDay;
            m_nHeartbeatDelay = rand_wincompatible() % 1200 + 3000;
        }
    }

    // Bucket C: the perception / visible-list pass
    g_pAppManager->m_pServerExoApp->GetActiveTimer(m_objectId)->GetWorldTime(&nDay, &nTimeOfDay);
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTimeOfDay, m_nLastPerceptionDay, m_nLastPerceptionTod, &nDaysSince, &nMsSince);

    // Party members (m_bPlayerCharacter) perceive on their own short delay; everyone else waits 4 seconds.
    // m_bInParty suppresses the pass outright. Nothing restamps m_nLastPerceptionDay/Tod after the
    // constructor zeroes them, so both delays measure from world time 0 and every heartbeat call passes.
    // Left as the original has it: restoring the throttle would make NPCs slower to react.
    if (m_bSpawnPerceptionDone == 0
            || (m_bInParty == 0
                && ((m_bPlayerCharacter != 0 && bFromHeartbeat == 1 && nMsSince >= m_nPerceptionDelay)
                    || (m_bPlayerCharacter == 0 && bFromHeartbeat == 1 && nMsSince >= 4000)))) {
        UpdateVisibleList(1);
        m_nPerceptionDelay = rand_wincompatible() % 400 + 300;
    }

    // Bucket B: the AI-level pass
    // There is no fresh world-time sample; this one reuses bucket C's.
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTimeOfDay, m_nLastVisibilityUpdateDay, m_nLastVisibilityUpdateTod, &nAIDaysSince, &nAIMsSince);

    if (m_bSpawnPerceptionDone == 0
            || (m_bPlayerCharacter != 0 && bFromHeartbeat == 1 && nAIMsSince > 199)
            || (m_bPlayerCharacter == 0 && bFromHeartbeat == 1 && nAIMsSince >= 4000)) {
        m_nAILevel0PerceptionSkips++;
        if (m_aiLevel != 0 || m_nAILevel0PerceptionSkips >= 150
                || m_bSpawnPerceptionDone == 0) {
            m_nAILevel0PerceptionSkips = rand_wincompatible() % 50;
            // Flag 0 for a party member, 2 for anyone else -- read before m_nLastVisibilityUpdateTod is restamped.
            if (m_nLastVisibilityUpdateTod != 0 || m_bSpawnPerceptionDone == 0)
                UpdateVisibleList(m_bInParty != 0 ? 0 : 2);
            m_nLastVisibilityUpdateDay = nDay;
            m_nLastVisibilityUpdateTod = nTimeOfDay;
#if KOTOR1
            if (GetArea() != 0 && static_cast<CSWSArea*>(GetArea())->m_nPlayersInArea == 0 && m_aiLevel != 0)
                pAIMaster->SetAILevel(this, 0);
#else
            CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
            if (pArea != 0 && pArea->m_nPlayersInArea == 0 && m_aiLevel != 0)
                pAIMaster->SetAILevel(this, 0);
#endif
        }
    }

    if (m_bSpawnPerceptionDone == 0)
        m_bSpawnPerceptionDone = 1;
}

void CSWSCreature::UpdateMineCheck()
{
    Vector vMinePoint;
    Vector vSelfPos;
    vMinePoint.x = 0.0f;
    vMinePoint.y = 0.0f;
    vMinePoint.z = 0.0f;
    vSelfPos.x = 0.0f;
    vSelfPos.y = 0.0f;
    vSelfPos.z = 0.0f;
    unsigned int nDay;
    unsigned int nTimeOfDay;
    unsigned int nDaysSince;
    unsigned int nMsSince;

    g_pAppManager->m_pServerExoApp->GetActiveTimer(m_objectId)->GetWorldTime(&nDay, &nTimeOfDay);
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTimeOfDay, m_nLastMineCheckDay, m_nLastMineCheckTod, &nDaysSince, &nMsSince);

    unsigned char nDetectMode = m_detectMode;
    if (m_nAnimation == SERVER_ANIMATION_PAUSE && nDetectMode != 1)
        nDetectMode = 2;
    unsigned int nInterval = nDetectMode == 0 ? 3000 : 100;
    float fRangeSq = nDetectMode == 0 ? 9.0f : 400.0f;
    if (nMsSince < nInterval)
        return;

    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->m_fChallengeRating = 1;
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (pArea != 0) {
        CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
        CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
        vSelfPos = m_position;
        int bSpotForWholeParty = m_bInParty;
        for (int i = 0; i < pArea->m_mineObjects.m_count; i++) {
            CGameObject* pObject = pServerApp->GetGameObject(pArea->m_mineObjects.m_pArray[i]);
            if (pObject == 0)
                continue;

            unsigned char nModeIndex = m_detectMode - 1;
            int nVehicleState = m_nAnimation;
            int nBaseRoll = g_pRules->RollDice(1, 10);
            int nRoll = (nVehicleState == 10000 || nModeIndex < 2)
                            ? nBaseRoll + 10 : nBaseRoll;
            CSWCCreature* pClientCreature = GetClientCreature();
            int bNotMoving;
            if (pClientCreature != 0 && m_pStats->m_bIsPC == 1
                    && pClientCreature->m_switchMode == CSWCCreature::MODE_SWDRIVE)
                bNotMoving = pClientCreature->m_bRunning == 0;
            else
                bNotMoving = m_bRunning == 0;
            int nDetect = bNotMoving ? nRoll : nRoll - 5;

            if (pObject->AsSWSTrigger() != 0) {
                CSWSTrigger* pTrigger = pObject->AsSWSTrigger();
                pTrigger->CalculateNearestPoint(vSelfPos, &vMinePoint);
                if (pTrigger->m_objectsInTrigger.Contains(m_objectId) == 0
                        && pTrigger->m_bTrapDetectable == 1) {
                    Vector vDelta = vSelfPos - vMinePoint;
                    if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z <= fRangeSq
                            && pTrigger->GetReputation(m_objectId, 0) <= 89
                            && pTrigger->m_nFaction != static_cast<unsigned int>(m_pStats->m_factionId)) {
                        int nCheck = nDetect + m_pStats->GetSkillRank(SKILL_AWARENESS, 0, 0);
                        int nDC = pTrigger->GetDetectDC();
                        if (nCheck >= nDC || pTrigger->m_bMineFlagged == 1) {
                            if (nCheck >= nDC && pTrigger->m_bMineFlagged == 0) {
                                CSWCCMessageData* pData = new CSWCCMessageData;
                                pData->SetObjectID(0, m_objectId);
                                pData->SetObjectID(1, pTrigger->m_objectId);
                                pData->SetInteger(0, nDetect);
                                pData->SetInteger(1, m_pStats->GetSkillRank(SKILL_AWARENESS, 0, 0));
                                pData->SetInteger(2, pTrigger->GetDetectDC());
                                BroadcastMineDetectionData(pData);
                                delete pData;
                            }
                            if (bSpotForWholeParty == 1) {
                                for (int j = 0; j < pParty->m_nCharacterCount; j++)
                                    pTrigger->m_objectsInTrigger.AddUnique(
                                        pServerApp->ClientToServerObjectId(
                                            pParty->GetCharacter(j)->m_objectId));
                            } else {
                                pTrigger->m_objectsInTrigger.AddUnique(m_objectId);
                            }
                        }
                    }
                }
            }

            if (pObject->AsSWSDoor() != 0) {
                CSWSDoor* pDoor = pObject->AsSWSDoor();
                if (pDoor->m_objectsAtDoor.Contains(m_objectId) == 0
                        && pDoor->m_bTrapDetectable == 1) {
                    Vector vDelta = vSelfPos - pDoor->m_position;
                    if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z <= fRangeSq
                            && pObject->AsSWSDoor()->GetReputation(m_objectId, 0) <= 89
                            && pObject->AsSWSDoor()->m_nFaction
                                   != static_cast<unsigned int>(m_pStats->m_factionId)) {
                        int nCheck = nDetect + m_pStats->GetSkillRank(SKILL_AWARENESS, 0, 0);
                        if (nCheck >= pDoor->GetDetectDC()
                                || static_cast<unsigned char>(pDoor->m_bMineFlagged) == 1) {
                            if (bSpotForWholeParty == 1) {
                                for (int j = 0; j < pParty->m_nCharacterCount; j++)
                                    pDoor->m_objectsAtDoor.AddUnique(
                                        pServerApp->ClientToServerObjectId(
                                            pParty->GetCharacter(j)->m_objectId));
                            } else {
                                pDoor->m_objectsAtDoor.AddUnique(m_objectId);
                            }
                        }
                    }
                }
            }

            if (pObject->AsSWSPlaceable() != 0) {
                CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
                // m_objectsOnPlaceable is the same CExoArrayList<unsigned long long> as the trigger's
                // and door's lists.
                CExoArrayList<unsigned long long>* pList =
                    &pPlaceable->m_objectsOnPlaceable;
                if (pPlaceable->m_objectsOnPlaceable.Contains(m_objectId) == 0
                        && pPlaceable->m_bTrapDetectable == 1) {
                    Vector vDelta = vSelfPos - pPlaceable->m_position;
                    if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z <= fRangeSq
                            && pObject->AsSWSPlaceable()->GetReputation(m_objectId, 0) <= 89
                            && pObject->AsSWSPlaceable()->m_nFaction
                                   != static_cast<unsigned int>(m_pStats->m_factionId)) {
                        int nCheck = nDetect + m_pStats->GetSkillRank(SKILL_AWARENESS, 0, 0);
                        if (nCheck >= pPlaceable->GetDetectDC()
                                || static_cast<unsigned char>(pPlaceable->m_bMineFlagged) == 1) {
                            if (bSpotForWholeParty == 1) {
                                for (int j = 0; j < pParty->m_nCharacterCount; j++)
                                    pList->AddUnique(pServerApp->ClientToServerObjectId(
                                        pParty->GetCharacter(j)->m_objectId));
                            } else {
                                pList->AddUnique(m_objectId);
                            }
                        }
                    }
                }
            }
        }
    }

    m_nLastMineCheckDay = nDay;
    m_nLastMineCheckTod = nTimeOfDay;
}

int CSWSCreature::AddCounterSpellActions(unsigned long long oidTarget)
{
    unsigned long long oidCaster = oidTarget;
    Vector vCaster = {};
    int nResult = 0;
    if (oidTarget != OBJECT_INVALID) {
        CSWSCreature* pCaster =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
        if (pCaster != 0 && GetArea() != 0 && m_objectId != oidTarget
                && pCaster->GetArea() != 0) {
            CSWSArea* pArea = static_cast<CSWSArea*>(pCaster->GetArea());
            if (pArea == 0)
                return CSWSObject::ACTION_IN_PROGRESS;
            unsigned long long oidArea = pArea->m_objectId;
            vCaster = pCaster->m_position;
            float fRange = g_pRules->m_fSpellRange[4];
            int nOne = 1;
            int nOneAlso = 1;
            if ((m_nAIState & AI_STATE_MOBILE) != 0) {
                AddActionToFront(AIACTION_COUNTER_SPELL, ACTION_GROUP_NEW,
                                 ACTION_PARAM_OBJECT, &oidCaster, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                nResult = 1;
                AddActionToFront(AIACTION_CHECK_MOVE_TO_OBJECT_RADIUS, ACTION_GROUP_PREVIOUS,
                                 ACTION_PARAM_OBJECT, &oidCaster, ACTION_PARAM_INT, &nOne,
                                 ACTION_PARAM_FLOAT, &fRange, ACTION_PARAM_FLOAT, &fRange,
                                 ACTION_PARAM_INT, &nOneAlso, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0);
                Vector vUp = {};
                AddMoveToPointActionToFront(ACTION_GROUP_PREVIOUS, vCaster, oidArea, OBJECT_INVALID, nOne,
                                            fRange, 0.0f, 0, 0, 0, 0, 0, 0, vUp, 0, 0, 0);
                m_counterSpellTargetId = oidCaster;
            }
        }
    }
    return nResult;
}

int CSWSCreature::AddMoveToPointActionToFront(unsigned short nGroupActionId,
                                              const Vector& vPosition,
                                              unsigned long long oid1,
                                              unsigned long long oid2,
                                              int a, float f1, float f2,
                                              int b, int c, int d, int e, int f,
                                              int g, const Vector& vUp,
                                              unsigned int h, unsigned int i, int j)
{
    int nFlags = a | (b << 1);
    if (f2 != 0.0f)
        nFlags |= 4;
    nFlags = ((d << 4) & 0x70) | nFlags;
    nFlags |= (f << 3);
    if (e != 0)
        nFlags |= (e & 3) << 7;
    nFlags |= ((g & 1) << 9) | ((j & 1) << 10);
    m_nMoveState = 2;
    m_pPathfindInfo->m_bForceMoveDeadline = 0;
    m_pPathfindInfo->m_nForceMoveDeadlineDay = 0;
    m_pPathfindInfo->m_nForceMoveDeadlineTimeOfDay = 0;
    Vector* pPos = const_cast<Vector*>(&vPosition);
    Vector* pUp = const_cast<Vector*>(&vUp);
    AddActionToFront(AIACTION_MOVE_TO_POINT, nGroupActionId,
                     ACTION_PARAM_FLOAT, static_cast<void*>(pPos),
                     ACTION_PARAM_FLOAT, static_cast<void*>(&pPos->y),
                     ACTION_PARAM_FLOAT, static_cast<void*>(&pPos->z), ACTION_PARAM_OBJECT, &oid1,
                     ACTION_PARAM_OBJECT, &oid2, ACTION_PARAM_INT, &nFlags, ACTION_PARAM_FLOAT, &f1,
                     ACTION_PARAM_INT, &c, ACTION_PARAM_FLOAT, &f2,
                     ACTION_PARAM_FLOAT, static_cast<void*>(pUp),
                     ACTION_PARAM_FLOAT, static_cast<void*>(&pUp->y),
                     ACTION_PARAM_INT, static_cast<void*>(&h),
                     ACTION_PARAM_INT, static_cast<void*>(&i));
    SetLockOrientationToObject(OBJECT_INVALID, 0);
    return 1;
}

int CSWSCreature::AddMoveToPointAction(unsigned short nGroupActionId,
                                       const Vector& vPosition,
                                       unsigned long long oid1,
                                       unsigned long long oid2,
                                       int a, float f1, float f2,
                                       int b, int c, int d, int e, int f,
                                       int g, const Vector& vUp,
                                       unsigned int h, unsigned int i, int j)
{
    int nFlags = a | (b << 1);
    if (f2 != 0.0f)
        nFlags |= 4;
    nFlags = ((d << 4) & 0x70) | nFlags;
    nFlags |= (f << 3);
    if (e != 0)
        nFlags |= (e & 3) << 7;
    nFlags |= ((g & 1) << 9) | ((j & 1) << 10);
    m_nMoveState = 2;
    m_pPathfindInfo->m_bForceMoveDeadline = 0;
    m_pPathfindInfo->m_nForceMoveDeadlineDay = 0;
    m_pPathfindInfo->m_nForceMoveDeadlineTimeOfDay = 0;
    Vector* pPos = const_cast<Vector*>(&vPosition);
    Vector* pUp = const_cast<Vector*>(&vUp);
    AddAction(AIACTION_MOVE_TO_POINT, nGroupActionId, ACTION_PARAM_FLOAT, static_cast<void*>(pPos),
              ACTION_PARAM_FLOAT, static_cast<void*>(&pPos->y),
              ACTION_PARAM_FLOAT, static_cast<void*>(&pPos->z), ACTION_PARAM_OBJECT, &oid1,
              ACTION_PARAM_OBJECT, &oid2, ACTION_PARAM_INT, &nFlags, ACTION_PARAM_FLOAT, &f1,
              ACTION_PARAM_INT, &c, ACTION_PARAM_FLOAT, &f2,
              ACTION_PARAM_FLOAT, static_cast<void*>(pUp),
              ACTION_PARAM_FLOAT, static_cast<void*>(&pUp->y),
              ACTION_PARAM_INT, static_cast<void*>(&h), ACTION_PARAM_INT, static_cast<void*>(&i));
    SetLockOrientationToObject(OBJECT_INVALID, 0);
    return 1;
}

int CSWSCreature::AddItemCastSpellActions(unsigned long long oidItem, int nProperty, int a3,
                                          Vector vTarget, unsigned long long oidTarget,
                                          int a6, int bNoTarget, int bAddToFront,
                                          int bQueueActions)
{
    float              fPointX;
    float              fPointY;
    float              fPointZ;
    int                nOne = 1;
    unsigned long long oidArea;
    float              fRange;
    int                nFlagA;
    int                nFlagB;

    if (bQueueActions == 0) {
        m_pCombatRound->AddSWItemSpellAction(oidItem, nProperty, a3, vTarget, oidTarget,
                                             bNoTarget, a6);
        if (m_bCombatState == 0 && m_pCombatRound->GetActionPending() != 0)
            ActionManager(4);
        if (GetNodeById(AIACTION_COMBAT) != 0)
            return 1;
        int nMarker = 1;
        AddAction(AIACTION_COMBAT, ACTION_GROUP_NEW, ACTION_PARAM_INT, &nMarker);
        return 1;
    }

    m_oidAttemptedSpellTarget = OBJECT_INVALID;
    m_oidSpellTarget = OBJECT_INVALID;

    fPointX = vTarget.x;
    fPointY = vTarget.y;
    fPointZ = vTarget.z;

    if (oidTarget != OBJECT_INVALID) {
        CGameObject* pObj = static_cast<CGameObject*>(g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget));
        if (pObj == 0)
            return 0;
        if (static_cast<CSWSObject*>(pObj)->GetArea() != GetArea())
            return 0;
        // Retarget onto the object's live position; the pack keeps the point it was given.
        vTarget = static_cast<CSWSObject*>(pObj)->m_position;
        if (pObj->AsSWSCreature() != 0 && static_cast<CSWSObject*>(pObj)->m_objectId != m_objectId
                && m_bInParty == 0) {
            CSWVisibilityNode* pNode = GetVisibleListElement(oidTarget);
            if (pNode == 0)
                return 0;
            if ((pNode->m_flags & VISIBILITY_PACIFY_STATE_MASK) == VISIBILITY_PACIFIED)
                return 0;
            unsigned char nMasked = pNode->m_flags & (VISIBILITY_INVISIBLE | VISIBILITY_SEEN);
            if (nMasked == VISIBILITY_INVISIBLE)
                return 0;
        }
    }

    CSWSItem* pItem = static_cast<CSWSItem*>(g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem));
    if (pItem == 0)
        return 0;
    int nSubType;
    CSWItemProperty* pProperty = pItem->GetActiveProperty(nProperty);
    if (pProperty != 0)
        nSubType = pProperty->m_subType;
#ifdef K2_FIX_ENGINE_BUGS
    else
        return 0;
#endif
#if KOTOR1  // K1: the property subtype is the spell id (no grenade conversion)
    int nSpellId = nSubType;
#else
    int nSpellId = ConvertGrenadeSpellID(nSubType);
#endif
    CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(nSpellId);
    if (pSpell == 0)
        return 0;
    if (GetArea() == 0)
        return 0;
    if (m_bCommandable == 0)
        return 0;
    oidArea = static_cast<CSWSArea*>(GetArea())->m_objectId;
    fRange = ComputeSpellRange(pSpell->m_sRangeCode, oidTarget);
    nFlagA = 1;
    nFlagB = 1;

    if (bAddToFront != 0) {
        AddActionToFront(AIACTION_ITEM_CAST_SPELL, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidItem,
                         ACTION_PARAM_INT, &nProperty, ACTION_PARAM_INT, &a3,
                         ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_FLOAT, &fPointX,
                         ACTION_PARAM_FLOAT, &fPointY, ACTION_PARAM_FLOAT, &fPointZ, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0);
        if (oidTarget == OBJECT_INVALID) {
            AddActionToFront(AIACTION_CHANGE_FACING_POINT, ACTION_GROUP_PREVIOUS,
                             ACTION_PARAM_FLOAT, &fPointX, ACTION_PARAM_FLOAT, &fPointY,
                             ACTION_PARAM_FLOAT, &fPointZ, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0);
            AddActionToFront(AIACTION_CHECK_MOVE_TO_POINT_RADIUS, ACTION_GROUP_PREVIOUS,
                             ACTION_PARAM_FLOAT, &fPointX, ACTION_PARAM_FLOAT, &fPointY,
                             ACTION_PARAM_FLOAT, &fPointZ, ACTION_PARAM_OBJECT, &oidArea,
                             ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nFlagA,
                             ACTION_PARAM_FLOAT, &fRange, ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0);
        } else {
            AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, ACTION_GROUP_PREVIOUS,
                             ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            int nZero = 0;
            AddActionToFront(AIACTION_CHECK_MOVE_TO_OBJECT_RADIUS, ACTION_GROUP_PREVIOUS,
                             ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nFlagA,
                             ACTION_PARAM_FLOAT, &fRange, ACTION_PARAM_FLOAT, &fRange,
                             ACTION_PARAM_INT, &nFlagB, ACTION_PARAM_INT, &nZero,
                             ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        }

        Vector vDelta = vTarget - m_position;
        float fDist = vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z;
        if (fDist > fRange * fRange) {
            Vector vZero = {};
            AddMoveToPointActionToFront(ACTION_GROUP_PREVIOUS, vTarget, oidArea, oidTarget, nFlagA, fRange,
                                        0.0f,
                                        0, 0, 0, 0, 0, 0, vZero, 0, 0, nOne);
            if (fDist > 0.25f && m_bInParty != 0) {
                CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
                int nIndex = pClient->GetSWParty()->GetIndex(
                    pClient->ServerToClientObjectId(m_objectId));
                if (nIndex >= 0)
                    pClient->GetSWParty()->m_members[nIndex].SetCombatMessage(STRREF_CLOSING_TO_ATTACK_RANGE);
            }
        }
    } else {
        int bMoved;
        int nGroup;
        Vector vDelta = vTarget - m_position;
        float fDist = vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z;
        if (fDist > fRange * fRange) {
            Vector vZero = {};
            AddMoveToPointAction(ACTION_GROUP_NEW, vTarget, oidArea, oidTarget, nFlagA, fRange, 0.0f,
                                 0, 0, 0, 0, 0, 0, vZero, 0, 0, nOne);
            bMoved = 1;
            nGroup = ACTION_GROUP_PREVIOUS;
        } else {
            bMoved = 0;
            nGroup = ACTION_GROUP_NEW;
        }

        if (oidTarget == OBJECT_INVALID) {
            AddAction(AIACTION_CHECK_MOVE_TO_POINT_RADIUS, static_cast<unsigned short>(nGroup),
                      ACTION_PARAM_FLOAT, &fPointX, ACTION_PARAM_FLOAT, &fPointY,
                      ACTION_PARAM_FLOAT, &fPointZ, ACTION_PARAM_OBJECT, &oidArea,
                      ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nFlagA,
                      ACTION_PARAM_FLOAT, &fRange, ACTION_PARAM_INT, &nOne);
            nGroup = bMoved != 0 ? nGroup : ACTION_GROUP_PREVIOUS;
            AddAction(AIACTION_CHANGE_FACING_POINT, static_cast<unsigned short>(nGroup),
                      ACTION_PARAM_FLOAT, &fPointX, ACTION_PARAM_FLOAT, &fPointY,
                      ACTION_PARAM_FLOAT, &fPointZ);
        } else {
            int nZero = 0;
            AddAction(AIACTION_CHECK_MOVE_TO_OBJECT_RADIUS, static_cast<unsigned short>(nGroup),
                      ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nFlagA,
                      ACTION_PARAM_FLOAT, &fRange, ACTION_PARAM_FLOAT, &fRange,
                      ACTION_PARAM_INT, &nFlagB, ACTION_PARAM_INT, &nZero, ACTION_PARAM_INT, &nOne);
            nGroup = bMoved != 0 ? nGroup : ACTION_GROUP_PREVIOUS;
            AddAction(AIACTION_CHANGE_FACING_OBJECT, static_cast<unsigned short>(nGroup),
                      ACTION_PARAM_OBJECT, &oidTarget);
        }
        AddAction(AIACTION_ITEM_CAST_SPELL, static_cast<unsigned short>(nGroup),
                  ACTION_PARAM_OBJECT, &oidItem, ACTION_PARAM_INT, &nProperty,
                  ACTION_PARAM_INT, &a3, ACTION_PARAM_OBJECT, &oidTarget,
                  ACTION_PARAM_FLOAT, &fPointX, ACTION_PARAM_FLOAT, &fPointY,
                  ACTION_PARAM_FLOAT, &fPointZ);
    }

    m_oidAttemptedSpellTarget = bNoTarget != 0 ? static_cast<CSWSArea*>(GetArea())->m_objectId : oidTarget;
    return 1;
}

int CSWSCreature::AddCastSpellActions(unsigned int nSpellId, int nClassIndex, int nDomainLevel,
                                      int nMetaMagic, int bSkipReadyCount, Vector vTarget,
                                      unsigned long long oidTarget, int a8, int bNoTarget,
                                      int bAddToFront, int bProjectile, unsigned char nPathType,
                                      int bAttackCast, int nFeatId, unsigned char nReadyKind,
                                      int bQueueActions)
{
    float              fPointX;
    float              fPointY;
    float              fPointZ;
    int                nCastReady;
    unsigned long long oidArea;
    float              fRange;
    int                nFlagA;
    unsigned int       nFlags;
    int                nFlagB;
    int                nOne;

    if (bQueueActions == 0) {
        m_pCombatRound->AddSWSpellAction(nSpellId, nClassIndex, nDomainLevel, nMetaMagic,
                                         bSkipReadyCount, vTarget, oidTarget, bNoTarget,
                                         bProjectile, nPathType, bAttackCast, nFeatId,
                                         nReadyKind, a8);
        if (m_bCombatState == 0 && m_pCombatRound->GetActionPending() == 0)
            ActionManager(4);
        if (GetNodeById(AIACTION_COMBAT) != 0)
            return 1;
        int nMarker = 1;
        AddAction(AIACTION_COMBAT, ACTION_GROUP_NEW, ACTION_PARAM_INT, &nMarker);
        return 1;
    }

    m_oidAttemptedSpellTarget = OBJECT_INVALID;
    m_oidSpellTarget = OBJECT_INVALID;
    nCastReady = nReadyKind;

    if (nClassIndex != CSWSObject::CAST_CLASS_NONE && bProjectile == 0 && bSkipReadyCount == 0) {
        if (nClassIndex == CSWSObject::CAST_CLASS_SPELL_LIKE_ABILITY) {
            if (m_pStats->GetIsInSpellLikeAbilityList(nSpellId) == 0)
                return 0;
        } else if (m_pStats->GetSpellUsesLeft(nSpellId, static_cast<unsigned char>(nClassIndex),
                                              static_cast<unsigned char>(nDomainLevel), static_cast<unsigned char>(nMetaMagic)) == 0) {
            return 0;
        }
    }

    CGameObject* pObj = static_cast<CGameObject*>(g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget));
    if (bProjectile == 0 && pObj != 0 && pObj->AsSWSCreature() != 0
            && static_cast<CSWSObject*>(pObj)->m_objectId != m_objectId && m_bInParty == 0) {
        CSWVisibilityNode* pNode = GetVisibleListElement(oidTarget);
        if (pNode == 0)
            return 0;
        if ((pNode->m_flags & VISIBILITY_PACIFY_STATE_MASK) == VISIBILITY_PACIFIED)
            return 0;
        unsigned char nMasked = pNode->m_flags & (VISIBILITY_INVISIBLE | VISIBILITY_SEEN);
        if (nMasked == VISIBILITY_INVISIBLE)
            return 0;
    }

    fPointX = vTarget.x;
    fPointY = vTarget.y;
    fPointZ = vTarget.z;

    if (oidTarget != OBJECT_INVALID) {
        CGameObject* pTargetObj;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget, &pTargetObj)
                != CGameObjectArray::SUCCESS)
            return 0;
        if (pTargetObj->m_objectType <= CGameObject::OBJECT_TYPE_AREA)
            return 0;
        if (static_cast<CSWSObject*>(pTargetObj)->GetArea() != GetArea())
            return 0;
        // Retarget onto the object's live position; the pack keeps the point it was given.
        vTarget = static_cast<CSWSObject*>(pTargetObj)->m_position;
    }

    CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(nSpellId);
    if (pSpell == 0)
        return 0;
    if (GetArea() == 0)
        return 0;
    if (m_bCommandable == 0)
        return 0;
    oidArea = static_cast<CSWSArea*>(GetArea())->m_objectId;
    fRange = ComputeSpellRange(pSpell->m_sRangeCode, oidTarget);
    nFlags = bProjectile != 0 ? (nPathType | CAST_FLAG_PROJECTILE) : nPathType;
    nFlagA = 1;

    if (bAttackCast != 0) {
        nFlags |= CAST_FLAG_ATTACK;
        nFlagB = 1;
        if (bAttackCast == 1) {
            if (bAddToFront != 0) {
                AddActionToFront(AIACTION_CAST_SPELL, ACTION_GROUP_NEW, ACTION_PARAM_INT, &nSpellId,
                                 ACTION_PARAM_INT, &nClassIndex, ACTION_PARAM_INT, &nDomainLevel,
                                 ACTION_PARAM_INT, &nMetaMagic, ACTION_PARAM_INT, &bSkipReadyCount,
                                 ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_FLOAT, &fPointX,
                                 ACTION_PARAM_FLOAT, &fPointY, ACTION_PARAM_FLOAT, &fPointZ,
                                 ACTION_PARAM_INT, &nFlags, ACTION_PARAM_INT, &nFeatId,
                                 ACTION_PARAM_INT, &nCastReady, 0, 0);
            } else {
                AddAction(AIACTION_CAST_SPELL, ACTION_GROUP_NEW, ACTION_PARAM_INT, &nSpellId,
                          ACTION_PARAM_INT, &nClassIndex, ACTION_PARAM_INT, &nDomainLevel,
                          ACTION_PARAM_INT, &nMetaMagic, ACTION_PARAM_INT, &bSkipReadyCount,
                          ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_FLOAT, &fPointX,
                          ACTION_PARAM_FLOAT, &fPointY, ACTION_PARAM_FLOAT, &fPointZ,
                          ACTION_PARAM_INT, &nFlags, ACTION_PARAM_INT, &nFeatId,
                          ACTION_PARAM_INT, &nCastReady);
            }
            return 1;
        }
    } else {
        nFlagB = 1;
    }
    nOne = 1;

    if (bAddToFront != 0) {
        AddActionToFront(AIACTION_CAST_SPELL, ACTION_GROUP_NEW, ACTION_PARAM_INT, &nSpellId,
                         ACTION_PARAM_INT, &nClassIndex, ACTION_PARAM_INT, &nDomainLevel,
                         ACTION_PARAM_INT, &nMetaMagic, ACTION_PARAM_INT, &bSkipReadyCount,
                         ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_FLOAT, &fPointX,
                         ACTION_PARAM_FLOAT, &fPointY, ACTION_PARAM_FLOAT, &fPointZ,
                         ACTION_PARAM_INT, &nFlags, ACTION_PARAM_INT, &nFeatId,
                         ACTION_PARAM_INT, &nCastReady, 0, 0);
        int nZero = 0;
        if (oidTarget == OBJECT_INVALID) {
            AddActionToFront(AIACTION_CHANGE_FACING_POINT, ACTION_GROUP_PREVIOUS,
                             ACTION_PARAM_FLOAT, &fPointX, ACTION_PARAM_FLOAT, &fPointY,
                             ACTION_PARAM_FLOAT, &fPointZ, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0);
            AddActionToFront(AIACTION_CHECK_MOVE_TO_POINT_RADIUS, ACTION_GROUP_PREVIOUS,
                             ACTION_PARAM_FLOAT, &fPointX, ACTION_PARAM_FLOAT, &fPointY,
                             ACTION_PARAM_FLOAT, &fPointZ, ACTION_PARAM_OBJECT, &oidArea,
                             ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nFlagA,
                             ACTION_PARAM_FLOAT, &fRange, ACTION_PARAM_FLOAT, &nOne, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0);
        } else {
            AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, ACTION_GROUP_PREVIOUS,
                             ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            AddActionToFront(AIACTION_CHECK_MOVE_TO_OBJECT_RADIUS, ACTION_GROUP_PREVIOUS,
                             ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nFlagA,
                             ACTION_PARAM_FLOAT, &fRange, ACTION_PARAM_FLOAT, &fRange,
                             ACTION_PARAM_INT, &nFlagB, ACTION_PARAM_INT, &nZero,
                             ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        }

        Vector vDelta = vTarget - m_position;
        float fDist = vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z;
        if (fDist > fRange * fRange) {
            Vector vZero = {};
            AddMoveToPointActionToFront(ACTION_GROUP_PREVIOUS, vTarget, oidArea, oidTarget, nFlagA, fRange,
                                        0.0f,
                                        0, 0, 0, 0, 0, 0, vZero, 0, 0, nOne);
            if (fDist > 0.25f && m_bInParty != 0) {
                CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
                int nIndex = pClient->GetSWParty()->GetIndex(
                    pClient->ServerToClientObjectId(m_objectId));
                if (nIndex >= 0)
                    pClient->GetSWParty()->m_members[nIndex].SetCombatMessage(STRREF_CLOSING_TO_ATTACK_RANGE);
            }
        }
    } else {
        int bMoved;
        int nGroup;
        Vector vDelta = vTarget - m_position;
        float fDist = vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z;
        if (fDist > fRange * fRange) {
            Vector vZero = {};
            AddMoveToPointAction(ACTION_GROUP_NEW, vTarget, oidArea, oidTarget, nFlagA, fRange, 0.0f,
                                 0, 0, 0, 0, 0, 0, vZero, 0, 0, nOne);
            // 1 here where the to-front arm's matching slot is 0.
            int nOne2 = 1;
            if (oidTarget == OBJECT_INVALID) {
                AddAction(AIACTION_CHECK_MOVE_TO_POINT_RADIUS, ACTION_GROUP_PREVIOUS,
                          ACTION_PARAM_FLOAT, &fPointX, ACTION_PARAM_FLOAT, &fPointY,
                          ACTION_PARAM_FLOAT, &fPointZ, ACTION_PARAM_OBJECT, &oidArea,
                          ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nFlagA,
                          ACTION_PARAM_FLOAT, &fRange);
            } else {
                AddAction(AIACTION_CHECK_MOVE_TO_OBJECT_RADIUS, ACTION_GROUP_PREVIOUS,
                          ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nFlagA,
                          ACTION_PARAM_FLOAT, &fRange, ACTION_PARAM_FLOAT, &fRange,
                          ACTION_PARAM_INT, &nFlagB, ACTION_PARAM_INT, &nOne2,
                          ACTION_PARAM_INT, &nOne);
            }
            bMoved = 1;
            nGroup = ACTION_GROUP_PREVIOUS;
        } else {
            bMoved = 0;
            nGroup = ACTION_GROUP_NEW;
        }

        if (oidTarget == OBJECT_INVALID)
            AddAction(AIACTION_CHANGE_FACING_POINT, static_cast<unsigned short>(nGroup),
                      ACTION_PARAM_FLOAT, &fPointX, ACTION_PARAM_FLOAT, &fPointY,
                      ACTION_PARAM_FLOAT, &fPointZ);
        else
            AddAction(AIACTION_CHANGE_FACING_OBJECT, static_cast<unsigned short>(nGroup),
                      ACTION_PARAM_OBJECT, &oidTarget);

        nGroup = bMoved != 0 ? nGroup : ACTION_GROUP_PREVIOUS;
        AddAction(AIACTION_CAST_SPELL, static_cast<unsigned short>(nGroup),
                  ACTION_PARAM_INT, &nSpellId, ACTION_PARAM_INT, &nClassIndex,
                  ACTION_PARAM_INT, &nDomainLevel, ACTION_PARAM_INT, &nMetaMagic,
                  ACTION_PARAM_INT, &bSkipReadyCount, ACTION_PARAM_OBJECT, &oidTarget,
                  ACTION_PARAM_FLOAT, &fPointX, ACTION_PARAM_FLOAT, &fPointY,
                  ACTION_PARAM_FLOAT, &fPointZ, ACTION_PARAM_INT, &nFlags,
                  ACTION_PARAM_INT, &nFeatId, ACTION_PARAM_INT, &nCastReady);
    }

    m_oidAttemptedSpellTarget = bNoTarget != 0 ? static_cast<CSWSArea*>(GetArea())->m_objectId : oidTarget;
    return 1;
}

void CSWSCreature::ComputeModifiedMovementRate()
{
    int bApplyLimit = 0;
    if (m_stealthMode == 1) {
#if KOTOR1  // K1: no feat exemption
        bApplyLimit = 1;
#else
        bApplyLimit = (m_pStats->HasFeat(FEAT_STEALTH_RUN) == 0);
#endif
    }

    int i = m_pStats->m_iFirstLimitMovementSpeed;
    while (i < m_effects.m_count) {
        CGameEffect* pEffect = m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_LIMIT_MOVEMENT_SPEED)
            break;
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_LIMIT_MOVEMENT_SPEED
            && (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_INNATE
            && pEffect->GetInteger(0) == 1) {
            RemoveEffectById(pEffect->m_effectId);
            i = m_pStats->m_iFirstLimitMovementSpeed;
        } else {
            i++;
        }
    }

    int j = m_pStats->m_iFirstMovementSpeedDecrease;
    while (j < m_effects.m_count) {
        CGameEffect* pEffect = m_effects.m_pArray[j];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_MOVEMENT_SPEED_DECREASE)
            break;
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_MOVEMENT_SPEED_DECREASE
            && (pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_INNATE
            && pEffect->m_creatorId == m_objectId) {
            RemoveEffectById(pEffect->m_effectId);
            j = m_pStats->m_iFirstMovementSpeedDecrease;
        } else {
            j++;
        }
    }

    if (bApplyLimit) {
        CGameEffect* pEffect = new CGameEffect(1);
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_LIMIT_MOVEMENT_SPEED);
        pEffect->m_durationTypeBits =
            (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) | CGameEffect::DURATION_TYPE_INNATE;
        pEffect->m_bIsExposed = 0;
        pEffect->SetCreator(m_objectId);
        pEffect->SetInteger(0, 1);
        ApplyEffect(pEffect, 0, 0);
    }
}

#if !KOTOR1
static inline int AlignmentInfluenceTier(unsigned char nClassLevel)
{
    if (nClassLevel > 18)
        return 5;
    if (nClassLevel > 14)
        return 4;
    if (nClassLevel > 10)
        return 3;
    if (nClassLevel > 6)
        return 2;
    return nClassLevel > 0;
}

void CSWSCreature::HandleAlignmentInfluence()
{
    if (m_sTag.CompareNoCase("kreia")) {
        int nBase = static_cast<signed char>(m_pStats->m_nBaseNPCAlignment);
        if (nBase < 0)
            nBase = 0;
        if (nBase > 100)
            nBase = 100;
        m_pStats->m_nAlignment = static_cast<short>(nBase);
        return;
    }

    C2DA* pInfluence = g_pRules->m_p2DArrays->m_pInfluence;
    if (pInfluence == 0)
        return;

    CExoString sEntry;
    int nRow = -1;
    for (int i = 0; i < pInfluence->m_nRows; i++) {
        pInfluence->GetCExoStringEntry(i, CExoString("Tag"), &sEntry);
        if (_stricmp(sEntry.CStr(), m_sTag.CStr()) == 0) {
            nRow = i;
            break;
        }
    }
    if (nRow == -1)
        return;

    char cPCAlignment = 0;
    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->GetValueNumber(
        CExoString("G_PC_Align_Val"), cPCAlignment);

    unsigned short nBaseAlignment = m_pStats->m_nBaseNPCAlignment;

    // The player's own creature is the first party slot flagged as the character-creation PC, or
    // failing that whatever the server calls the player creature.
    CSWSCreature* pPlayer = 0;
    for (int i = 0; i < 3; i++) {
        CSWCCreature* pCharacter =
            g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(i);
        if (pCharacter != 0) {
            pPlayer = pCharacter->GetServerCreature();
            if (pPlayer != 0 && pPlayer->m_bPlayerMadeCharacter != 0)
                goto havePlayer;
        }
    }
    pPlayer = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
        g_pAppManager->m_pServerExoApp->GetPlayerCreatureId());
havePlayer:

    float fPrestigeScale;
    char cCharismaBonus;
    if (pPlayer != 0) {
        signed char nCHAMod = static_cast<signed char>(pPlayer->m_pStats->m_nCHAMod);
        int nTier = 0;
        if (pPlayer->m_pStats->HasFeat(FEAT_DARK_SIDE_CORRUPTION)) {
            if (pPlayer->m_pStats->m_nAlignment <= 40) {
                for (int i = 0; i < 2; i++) {
                    if (i < pPlayer->m_pStats->m_numClasses &&
                            pPlayer->m_pStats->m_classInfo[i].m_class == CLASS_TYPE_SITHLORD) {
                        nTier = AlignmentInfluenceTier(pPlayer->m_pStats->GetClassLevel(static_cast<unsigned char>(i), 0));
                        break;
                    }
                }
            }
        } else if (pPlayer->m_pStats->HasFeat(FEAT_LIGHT_SIDE_ENLIGHTENMENT)) {
            if (pPlayer->m_pStats->m_nAlignment >= 60) {
                for (int i = 0; i < 2; i++) {
                    if (i < pPlayer->m_pStats->m_numClasses &&
                            pPlayer->m_pStats->m_classInfo[i].m_class == CLASS_TYPE_JEDIMASTER) {
                        nTier = AlignmentInfluenceTier(pPlayer->m_pStats->GetClassLevel(static_cast<unsigned char>(i), 0));
                        break;
                    }
                }
            }
        }
        fPrestigeScale = nTier * 0.2f + 1.0f;
        cCharismaBonus = static_cast<char>(nCHAMod * 3);
    } else {
        fPrestigeScale = 1.0f;
        cCharismaBonus = 0;
    }

    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    int nNPC = pPartyTable->GetNPCID(m_objectId);
    if (nNPC == -1) {
        if (m_sTag.CompareNoCase("atton"))
            nNPC = 0;
        else if (m_sTag.CompareNoCase("baodur"))
            nNPC = 1;
        else if (m_sTag.CompareNoCase("mand"))
            nNPC = 2;
        else if (m_sTag.CompareNoCase("g0t0"))
            nNPC = 3;
        else if (m_sTag.CompareNoCase("handmaiden"))
            nNPC = 4;
        else if (m_sTag.CompareNoCase("hk47"))
            nNPC = 5;
        else if (m_sTag.CompareNoCase("kreia"))
            nNPC = 6;
        else if (m_sTag.CompareNoCase("mira"))
            nNPC = 7;
        else if (m_sTag.CompareNoCase("t3m4"))
            nNPC = 8;
        else if (m_sTag.CompareNoCase("visasmarr"))
            nNPC = 9;
        else if (m_sTag.CompareNoCase("hanharr"))
            nNPC = 10;
        else if (m_sTag.CompareNoCase("disciple"))
            nNPC = 11;
        else
            return;
    }

    char cInfluence = static_cast<char>(pPartyTable->GetInfluence(nNPC));
    if (cInfluence == -1) {
        int nBaseInfluence;
        pInfluence->GetINTEntry(nRow, CExoString("BaseInfluence"), &nBaseInfluence);
        cInfluence = static_cast<char>(nBaseInfluence);
        pPartyTable->SetInfluence(nNPC, cInfluence);
    }

    int nTotal = cInfluence + cCharismaBonus;
    if (nTotal < 0)
        nTotal = 0;
    if (nTotal > 100)
        nTotal = 100;
    float fInfluence = fPrestigeScale * (nTotal - 50) / 50.0f;
    float fShift = (cPCAlignment - 50) / 50.0f * fInfluence;

    int nOld = static_cast<char>(nBaseAlignment);
    float fOld = static_cast<float>(nOld);
    float fNew;
    if (!(fShift <= 0.0f))
        fNew = fShift * (100 - nOld) + fOld;
    else
        fNew = (fShift + 1.0f) * fOld;

    if (fNew > 100.0f) {
        m_pStats->m_nAlignment = 100;
    } else if (fNew < 0.0f) {
        m_pStats->m_nAlignment = 0;
    } else {
        int nNew = static_cast<int>(fNew);
        if (nNew < 0)
            nNew = 0;
        if (nNew > 100)
            nNew = 100;
        m_pStats->m_nAlignment = static_cast<short>(nNew);
    }
    UpdatePureGoodEvilPowers();
}
#endif

void CSWSCreature::UpdatePureGoodEvilPowers()
{
    if (m_pStats != 0) {
        if (m_pStats->m_nAlignment == 100) {
            RemovePureEvilPowers();
            AddPureGoodPowers();
        } else if (m_pStats->m_nAlignment == 0) {
            RemovePureGoodPowers();
            AddPureEvilPowers();
        } else {
            RemovePureGoodPowers();
            RemovePureEvilPowers();
        }
    }
}

int bDisableBumping = 1;

CExoArrayList<CGameEffect*> CSWSCreature::GetFilteredEffectList(unsigned char nType)
{
    CExoArrayList<CGameEffect*> list;

    if (nType == 2)
    {
        if (m_effects.m_count > 0)
        {
            for (int i = m_pStats->m_iFirstDamageIncrease; i < m_effects.m_count; i++)
            {
                CGameEffect* pEffect = m_effects.m_pArray[i];
                if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_DAMAGE_DECREASE)
                    break;
                if (pEffect->m_subType != CSWSEffectListHandler::EFFECT_DAMAGE_INCREASE
                    && pEffect->m_subType != CSWSEffectListHandler::EFFECT_DAMAGE_DECREASE)
                    continue;

                CGameEffect* pOther;
                int j;
                for (j = 0; j < list.m_count; j++)
                {
                    pOther = list.m_pArray[j];
                    if (pOther->m_subType == pEffect->m_subType
                        && pOther->GetInteger(1) == pEffect->GetInteger(1)
                        && pOther->GetInteger(2) == pEffect->GetInteger(2)
                        && pOther->GetInteger(3) == pEffect->GetInteger(3)
                        && pOther->GetInteger(4) == pEffect->GetInteger(4)
                        && pOther->GetInteger(5) == pEffect->GetInteger(5)
                        && (pOther->m_spellId == -1 || pEffect->m_spellId == -1
                            || pOther->m_spellId == pEffect->m_spellId))
                    {
                        goto rankDamage;
                    }
                }

                list.Add(pEffect);
                continue;

            rankDamage:
                {
                    int nRankNew;
                    int nRankOld;
                    Log_Base2(pEffect->GetInteger(1));
                    g_pRules->m_p2DArrays->m_pIPRPDamage->GetINTEntry(
                        pEffect->GetInteger(0), "Rank", &nRankNew);
                    g_pRules->m_p2DArrays->m_pIPRPDamage->GetINTEntry(
                        pOther->GetInteger(0), "Rank", &nRankOld);
                    if (nRankNew > nRankOld)
                        list.m_pArray[j] = pEffect;
                }
            }
        }
    }
    else if (nType == 1)
    {
        int bFound = 0;
        CGameEffect* pOther K2_UNSET;
        int j;
        if (m_effects.m_count > 0)
        {
            for (int i = m_pStats->m_iFirstAttackIncrease; i < m_effects.m_count; i++)
            {
                CGameEffect* pEffect = m_effects.m_pArray[i];
                if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_ATTACK_DECREASE)
                    break;
                if (pEffect->m_subType != CSWSEffectListHandler::EFFECT_ATTACK_INCREASE
                    && pEffect->m_subType != CSWSEffectListHandler::EFFECT_ATTACK_DECREASE)
                    continue;

                for (j = 0; j < list.m_count; j++)
                {
                    pOther = list.m_pArray[j];
                    bFound = 0;
                    if (pOther->m_subType == pEffect->m_subType
                        && pOther->GetInteger(1) == pEffect->GetInteger(1)
                        && pOther->GetInteger(2) == pEffect->GetInteger(2)
                        && pOther->GetInteger(3) == pEffect->GetInteger(3)
                        && pOther->GetInteger(4) == pEffect->GetInteger(4)
                        && (pOther->m_spellId == -1 || pEffect->m_spellId == -1
                            || pOther->m_spellId == pEffect->m_spellId))
                    {
                        bFound = 1;
                        break;
                    }
                }

                if (bFound == 1)
                {
                    if (pEffect->GetInteger(0) > pOther->GetInteger(0))
                        list.m_pArray[j] = pEffect;
                }
                else
                {
                    list.Add(pEffect);
                }
            }
        }
    }
    return list;
}


int CSWSCreature::GetIsCreatureBumpable(CSWSCreature* pCreature)
{
#if !KOTOR1  // K1: no leader-bumps-party shortcut
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState != 0)
        return 0;
    CSWCCreature* pCharacter = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
    if (pCharacter == 0)
        return 0;
    if (g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId) ==
            pCharacter->m_objectId) {
        if (pCreature->m_bInParty != 0 || pCreature->m_isPuppet != 0)
            return 1;
    }
#endif
    if (bDisableBumping != 0)
        return 0;
    if (GetReputation(pCreature->m_objectId, 0) < 10)
        return 0;
    if (pCreature->m_bInParty != 0)
        return 0;
    CSWSCreature* pPlayer = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
        g_pAppManager->m_pServerExoApp->GetPlayerCreatureId());
    if (pPlayer == this) {
        if (pPlayer != pCreature) {
            CExoLinkedListNode* pHead = pCreature->m_actionList.m_pInternal->m_pHead;
            if (pHead != 0) {
                CSWSObjectActionNode* pAction = static_cast<CSWSObjectActionNode*>(pHead->m_pData);
                if (pAction != 0) {
                    switch (pAction->m_nActionId) {
                    case AIACTION_PHYSICAL_ATTACKS:
                    case AIACTION_CAST_SPELL:
                    case AIACTION_ITEM_CAST_SPELL:
                    case AIACTION_COMBAT:
                        return 0;
                    }
                }
            }
            return pCreature->m_bInParty;  // unreachable (gated 0 above) but the load is deliberate
        }
    } else if (pPlayer == pCreature) {
        return 0;
    }
    if (pPlayer == pCreature || pPlayer == this)  // redundant re-test
        return 0;
    if (pCreature->m_actionList.m_pInternal->m_pHead != 0)
        return 0;
    if (pCreature->m_bCombatState != 0)
        return 0;
    return pCreature->m_pCombatRound->m_bRoundPaused == 0;
}

int CSWSCreature::BumpFriends(CSWSCreature* pOther, const Vector& vFrom, const Vector& vTo)
{
    if (GetIsCreatureBumpable(pOther) == 0)
        return 0;

    CExoString sMessage;
    sMessage.Format(const_cast<char*>("Bump! %x (%s) bumps %x (%s)\n"), m_objectId, m_sTag.CStr(),
                    pOther->m_objectId, pOther->m_sTag.CStr());
    sMessage.CStr();

    Vector vOtherPos = pOther->m_position;
    float fPush = pOther->m_pPathfindInfo->m_fCrePersonalSpace + 0.1f
                + m_pPathfindInfo->m_fCrePersonalSpace;
    Vector vProjected = lineproject(vFrom, vTo, vOtherPos);
    Vector vBumped = vOtherPos - vProjected;
    vBumped = vProjected + normalize(vBumped) * fPush;

    Vector vSaved = m_position;
    SetPosition(vProjected, 1, 0, 0);

    float fFloor = vBumped.z < vOtherPos.z ? vBumped.z : vOtherPos.z;
    float fCeiling = fFloor + pOther->m_pPathfindInfo->m_fHeight;
    int nSafe = static_cast<CSWSArea*>(GetArea())->ComputeSafeLocation(vBumped, 5.0f,
                                                            pOther->m_pPathfindInfo, 1, &vBumped);
    unsigned long long oidHit;
    int nClear = static_cast<CSWSArea*>(GetArea())->NoNonWalkPolysInDoors(
        vOtherPos, vBumped, fFloor - 0.1f, fCeiling, pOther->m_pPathfindInfo->m_fPersonalSpace,
        OBJECT_INVALID, oidHit, 0, 1, 0);

    if (nSafe != 0 && nClear == 1) {
#if KOTOR1  // K1: the bumped creature is moved there at once (no move-to action)
        pOther->SetPosition(vBumped, 1, 1, 0);
#else
        CSWSObjectActionNode* pAction = pOther->m_actionList.GetHead();
        CExoLinkedListNode* pPos = pOther->m_actionList.GetHeadPos();
        while (pAction != 0) {
            int nActionId = pAction->m_nActionId & 0xffff;
            if (nActionId == AIACTION_MOVE_TO_POINT || nActionId == AIACTION_CHECK_MOVE_TO_OBJECT_RADIUS)
                goto report;
            pAction = pOther->m_actionList.GetNext(pPos);
        }
        {
            pOther->ClearAllActions(1);
            pOther->AddMoveToPointActionToFront(
                ACTION_GROUP_NEW, vBumped, static_cast<CSWSArea*>(pOther->GetArea())->m_objectId,
                OBJECT_INVALID, 1, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0, Vector(), 0, 0, 0);
        }
report:
#endif
        sMessage.Format(const_cast<char*>("Bumped %x ends up at %f,%f\n"), pOther->m_objectId,
                        vBumped.x, vBumped.y);
        sMessage.CStr();
        SetPosition(vSaved, 1, 1, 0);
        return 1;
    }

    // Plan B -- retreat the blocker to safe ground behind us.
    Vector vSafe = {};
    if (static_cast<CSWSArea*>(GetArea())->ComputeSafeLocationInDirection(vTo, m_orientation * -1.0f, 4.0f,
                                                               pOther->m_pPathfindInfo, 1,
                                                               &vSafe) == 1) {
        pOther->SetPosition(vSafe, 1, 1, 0);
        if (pOther->m_bInParty != 0) {
            int nIndex = g_pAppManager->m_pClientExoApp->GetSWParty()->GetIndex(
                g_pAppManager->m_pClientExoApp->ServerToClientObjectId(pOther->m_objectId));
            g_pAppManager->m_pClientExoApp->GetSWParty()->m_members[nIndex].m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_FIND_PATH;
        }
        sMessage.Format(const_cast<char*>("Bumped %x ends up at %f,%f\n"), pOther->m_objectId,
                        vSafe.x, vSafe.y);
        sMessage.CStr();
        SetPosition(vSaved, 1, 1, 0);
        return 1;
    }
    SetPosition(vSaved, 1, 1, 0);
    return 0;
}

int CSWSCreature::ApplyDiseasePayload(CGameEffect* pEffect, unsigned int nCalendarDay,
                                      unsigned int nTimeOfDay)
{
    int bPayloadLanded = 0;
    if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_DISEASE)
    {
        CWorldTimer* pTimer = g_pAppManager->m_pServerExoApp->GetActiveTimer(m_objectId);
        C2DA* p2DA = g_pRules->m_p2DArrays->m_pDisease;
        int nRow = pEffect->GetInteger(0);
        int nSaveDC;
        p2DA->GetINTEntry(nRow, CExoString("Subs_Save"), &nSaveDC);
        if (SavingThrowRoll(SAVING_THROW_FORT, static_cast<unsigned short>(nSaveDC), SAVING_THROW_TYPE_DISEASE, pEffect->m_creatorId, 1, 0, 0))
        {
            int nLanded = pEffect->GetInteger(1);
            DisplayFloatyDataToSelf(0);
            if (nLanded == 1)
            {
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, m_objectId, m_objectId, CServerAIMaster::EVENT_REMOVE_EFFECT, pEffect);
            }
            else
            {
                pEffect->SetInteger(1, 1);
            }
            if (pEffect->GetInteger(2) == 0)
            {
                pEffect->SetInteger(2, 1);
            }
        }
        else
        {
            CExoString sDice("Dice_");
            CExoString sDam("Dam_");
            CExoString sType("Type_");
            for (int i = 1; i < 4; i++)
            {
                CExoString sIndex(i);
                int nDice;
                int nDam K2_UNSET;
                int nAbility K2_UNSET;
                if (p2DA->GetINTEntry(nRow, sDice + sIndex, &nDice) &&
                    p2DA->GetINTEntry(nRow, sDam + sIndex, &nDam) &&
                    p2DA->GetINTEntry(nRow, sType + sIndex, &nAbility))
                {
                    unsigned short nAmount =
                        g_pRules->RollDice(static_cast<unsigned char>(nDam), static_cast<unsigned char>(nDice));
                    CGameEffect* pDamage = new CGameEffect(1);
                    pDamage->SetCreator(pEffect->m_creatorId);
                    pDamage->m_fDuration = pEffect->m_fDuration;
                    pDamage->m_durationTypeBits =
                            (pDamage->m_durationTypeBits & CGameEffect::DURATION_SUBTYPE_CLEAR_MASK) |
                                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_AND_SUBTYPE_MASK);
                    pDamage->m_spellId = pEffect->m_spellId;
                    pDamage->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ABILITY_DECREASE);
                    pDamage->SetInteger(0, nAbility);
                    pDamage->SetInteger(1, nAmount);
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                        0, 0, m_objectId, m_objectId, CServerAIMaster::EVENT_APPLY_EFFECT,
                        pDamage);
                }
            }
            CGameEffect* pVisual = new CGameEffect(pEffect);
            pVisual->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
            pVisual->SetInteger(0, VFX_IMP_DISEASE_PAYLOAD);
            ApplyEffect(pVisual, 0, 0);
            if (pEffect->GetInteger(2) != 0)
            {
                CExoString sScript;
                if (p2DA->GetCExoStringEntry(nRow, CExoString("24_Hour_Script"), &sScript))
                {
                    g_pVirtualMachine->RunScript(&sScript, m_objectId, 1);
                }
            }
            else
            {
                CExoString sScript;
                if (p2DA->GetCExoStringEntry(nRow, CExoString("End_Incu_Script"), &sScript))
                {
                    g_pVirtualMachine->RunScript(&sScript, m_objectId, 1);
                }
                pEffect->SetInteger(2, 1);
            }
            DisplayFloatyDataToSelf(0);
            bPayloadLanded = 1;
        }
        unsigned int nDelta =
            g_pAppManager->m_pServerExoApp->GetModule()->m_nMinutesPerHour *
            CWorldTimer::MILLISECONDS_IN_MINUTE * 24;
        unsigned int nNextDay;
        unsigned int nNextTime;
        pTimer->AddWorldTimes(nCalendarDay, nTimeOfDay, 0, nDelta, &nNextDay, &nNextTime);
        pEffect->SetInteger(3, nNextDay);
        pEffect->SetInteger(4, nNextTime);
    }
    return bPayloadLanded;
}

int CSWSCreature::ApplyPoisonDamage(int nRow, CGameEffect* pEffect, int nApplyFlags, float fTick)
{
    C2DA* p2DA = g_pRules->m_p2DArrays->m_pPoison;
    if (p2DA == 0)
        return 0;

    int nSpellId = pEffect->m_spellId;

    int nDamHP;
    p2DA->GetINTEntry(nRow, CExoString("DAM_HP"), &nDamHP);
    int nDamFP;
    p2DA->GetINTEntry(nRow, CExoString("DAM_FP"), &nDamFP);
    int nDamStr;
    p2DA->GetINTEntry(nRow, CExoString("DAM_STR"), &nDamStr);
    int nDamDex;
    p2DA->GetINTEntry(nRow, CExoString("DAM_DEX"), &nDamDex);
    int nDamCon;
    p2DA->GetINTEntry(nRow, CExoString("DAM_CON"), &nDamCon);
    int nDamInt;
    p2DA->GetINTEntry(nRow, CExoString("DAM_INT"), &nDamInt);
    int nDamWis;
    p2DA->GetINTEntry(nRow, CExoString("DAM_WIS"), &nDamWis);
    int nTicks = static_cast<int>(fTick);
    int nDamChr;
    p2DA->GetINTEntry(nRow, CExoString("DAM_CHR"), &nDamChr);

    int nDuration = 0;
    int nPeriod = 0;
    int nAbilityDuration = 0;
    // Only pay for the duration/period lookup when there is an ability drain to time.
    if (nDamStr > 0 || nDamDex > 0 || nDamCon > 0 || nDamInt > 0 || nDamWis > 0 || nDamChr > 0)
    {
        p2DA->GetINTEntry(nRow, CExoString("DURATION"), &nDuration);
        p2DA->GetINTEntry(nRow, CExoString("PERIOD"), &nPeriod);
        nAbilityDuration = nDuration - (nTicks - 1) * nPeriod;
        CExoString sTrace;
        sTrace.Format(const_cast<char*>("POISONTRACE: Ability Damage duration = %d seconds\n"), nAbilityDuration);
        sTrace.CStr();
    }

    if (nDamHP > 0)
    {
        CGameEffect* pDamage = new CGameEffect(pEffect);
        int nDamageFlags = CSWSEffectListHandler::DAMAGE_FLAG_POISON;
        int nDamagePower = CSWSEffectListHandler::DAMAGE_POWER_NORMAL;
        pDamage->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE);
        pDamage->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                      (pDamage->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pDamage->SetNumIntegers(22);
        for (int i = 0; i < 15; i++)
            pDamage->SetInteger(i, -1);
        pDamage->SetInteger(Log_Base2(nDamageFlags), nDamHP);
        pDamage->SetInteger(14, nDamHP);
        pDamage->SetInteger(16, 1000);
        pDamage->SetInteger(17, nDamageFlags);
        pDamage->SetInteger(18, nDamagePower);
        pDamage->SetInteger(21, 1);
        CExoString sTrace;
        sTrace.Format(const_cast<char*>("POISONTRACE: Applying HP damage: %d\n"), nDamHP);
        sTrace.CStr();
        // Queued through the AI master, unlike every other block here, and the event's target is
        // the effect's creator, not us.
        CServerAIMaster* pMaster = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
        pMaster->AddEventDeltaTime(0, 0, pDamage->m_creatorId, m_objectId,
                                   CServerAIMaster::EVENT_APPLY_EFFECT, pDamage);
    }

    if (nDamFP > 0)
    {
        CGameEffect* pDamage = new CGameEffect(pEffect);
        pDamage->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGEFORCEPOINTS);
        pDamage->SetInteger(0, nDamFP);
        pDamage->m_durationTypeBits = (pDamage->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) |
                                      CGameEffect::DURATION_TYPE_INSTANTANEOUS;
        CExoString sTrace;
        sTrace.Format(const_cast<char*>("POISONTRACE: Applying FP damage: %d\n"), nDamFP);
        sTrace.CStr();
        ApplyEffect(pDamage, nApplyFlags, 0);
    }

    if (nAbilityDuration > 0 && nDamStr > 0)
    {
        CGameEffect* pAbility = new CGameEffect(pEffect);
        pAbility->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ABILITY_DECREASE);
        pAbility->SetInteger(0, 0);
        pAbility->SetInteger(1, nDamStr * nTicks);
        pAbility->m_durationTypeBits = (pAbility->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) |
                                       CGameEffect::DURATION_TYPE_TEMPORARY;
        pAbility->m_fDuration = static_cast<float>(nAbilityDuration);
        pAbility->m_bIsExposed = 0;
        RemoveEffectByType(nSpellId, CSWSEffectListHandler::EFFECT_ABILITY_DECREASE, 0);
        CExoString sTrace;
        sTrace.Format(const_cast<char*>("POISONTRACE: Applying STR damage: %d\n"), nDamStr * nTicks);
        sTrace.CStr();
        ApplyEffect(pAbility, nApplyFlags, 0);
    }

    if (nAbilityDuration > 0 && nDamDex > 0)
    {
        CGameEffect* pAbility = new CGameEffect(pEffect);
        pAbility->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ABILITY_DECREASE);
        pAbility->SetInteger(0, 1);
        pAbility->SetInteger(1, nDamDex * nTicks);
        pAbility->m_durationTypeBits = (pAbility->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) |
                                       CGameEffect::DURATION_TYPE_TEMPORARY;
        pAbility->m_fDuration = static_cast<float>(nAbilityDuration);
        pAbility->m_bIsExposed = 0;
        RemoveEffectByType(nSpellId, CSWSEffectListHandler::EFFECT_ABILITY_DECREASE, 1);
        CExoString sTrace;
        sTrace.Format(const_cast<char*>("POISONTRACE: Applying DEX damage: %d\n"), nDamDex * nTicks);
        sTrace.CStr();
        ApplyEffect(pAbility, nApplyFlags, 0);
    }

    if (nAbilityDuration > 0 && nDamCon > 0)
    {
        CGameEffect* pAbility = new CGameEffect(pEffect);
        pAbility->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ABILITY_DECREASE);
        pAbility->SetInteger(0, 2);
        pAbility->SetInteger(1, nDamCon * nTicks);
        pAbility->m_durationTypeBits = (pAbility->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) |
                                       CGameEffect::DURATION_TYPE_TEMPORARY;
        pAbility->m_fDuration = static_cast<float>(nAbilityDuration);
        pAbility->m_bIsExposed = 0;
        RemoveEffectByType(nSpellId, CSWSEffectListHandler::EFFECT_ABILITY_DECREASE, 2);
        CExoString sTrace;
        sTrace.Format(const_cast<char*>("POISONTRACE: Applying CON damage: %d\n"), nDamCon * nTicks);
        sTrace.CStr();
        ApplyEffect(pAbility, nApplyFlags, 0);
    }

    if (nAbilityDuration > 0 && nDamInt > 0)
    {
        CGameEffect* pAbility = new CGameEffect(pEffect);
        pAbility->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ABILITY_DECREASE);
        pAbility->SetInteger(0, 3);
        pAbility->SetInteger(1, nDamInt * nTicks);
        pAbility->m_durationTypeBits = (pAbility->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) |
                                       CGameEffect::DURATION_TYPE_TEMPORARY;
        pAbility->m_fDuration = static_cast<float>(nAbilityDuration);
        pAbility->m_bIsExposed = 0;
        RemoveEffectByType(nSpellId, CSWSEffectListHandler::EFFECT_ABILITY_DECREASE, 3);
        CExoString sTrace;
        sTrace.Format(const_cast<char*>("POISONTRACE: Applying INT damage: %d\n"), nDamInt * nTicks);
        sTrace.CStr();
        ApplyEffect(pAbility, nApplyFlags, 0);
    }

    if (nAbilityDuration > 0 && nDamWis > 0)
    {
        CGameEffect* pAbility = new CGameEffect(pEffect);
        pAbility->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ABILITY_DECREASE);
        pAbility->SetInteger(0, 4);
        pAbility->SetInteger(1, nDamWis * nTicks);
        pAbility->m_durationTypeBits = (pAbility->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) |
                                       CGameEffect::DURATION_TYPE_TEMPORARY;
        pAbility->m_fDuration = static_cast<float>(nAbilityDuration);
        pAbility->m_bIsExposed = 0;
        RemoveEffectByType(nSpellId, CSWSEffectListHandler::EFFECT_ABILITY_DECREASE, 4);
        CExoString sTrace;
        sTrace.Format(const_cast<char*>("POISONTRACE: Applying WIS damage: %d\n"), nDamWis * nTicks);
        sTrace.CStr();
        ApplyEffect(pAbility, nApplyFlags, 0);
    }

    if (nAbilityDuration > 0 && nDamChr > 0)
    {
        CGameEffect* pAbility = new CGameEffect(pEffect);
        pAbility->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ABILITY_DECREASE);
        pAbility->SetInteger(0, 5);
        pAbility->SetInteger(1, nDamChr * nTicks);
        pAbility->m_durationTypeBits = (pAbility->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) |
                                       CGameEffect::DURATION_TYPE_TEMPORARY;
        pAbility->m_fDuration = static_cast<float>(nAbilityDuration);
        pAbility->m_bIsExposed = 0;
        RemoveEffectByType(nSpellId, CSWSEffectListHandler::EFFECT_ABILITY_DECREASE, 5);
        CExoString sTrace;
        sTrace.Format(const_cast<char*>("POISONTRACE: Applying CHR damage: %d\n"), nDamChr * nTicks);
        sTrace.CStr();
        ApplyEffect(pAbility, nApplyFlags, 0);
    }

    return 1;
}

void CSWSCreature::ProcessPendingCombatActions()
{
    if (m_pCombatRound->GetActionPending() == 0)
        return;
    if (m_pCombatRound->m_pRoundActionList == 0)
        return;
    CExoLinkedListInternal* pInternal = m_pCombatRound->m_pRoundActionList->m_pInternal;
    CExoLinkedListNode* pNode = pInternal->m_pTail;
    if (pNode == 0)
        return;
    CSWSCombatRoundAction* pAction = static_cast<CSWSCombatRoundAction*>(pInternal->GetAtPos(pNode));
    while (pNode != 0) {
        if (pAction->m_nActionType == 6) {
            CSWSItem* pItem =
                g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pAction->m_oidTarget);
            if (pItem != 0) {
                AddEquipItemActions(pItem, pAction->m_nInventorySlot, 1, 1, pAction->m_nActionParam,
                                    pAction->m_bIsPC, 0, 0);
                CExoLinkedListNode* pOldNode = pNode;
                CSWSCombatRoundAction* pOldAction = pAction;
                pAction = static_cast<CSWSCombatRoundAction*>(pInternal->GetPrev(pNode));
                if (pOldNode != 0) {
                    pInternal->Remove(pOldNode);
                    delete pOldAction;
                }
                continue;
            }
        } else if (pAction->m_nActionType == 7) {
            CSWSItem* pItem =
                g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pAction->m_oidTarget);
            if (pItem != 0) {
                AddUnequipActions(pItem, pAction->m_oidTargetRepository, 1, 1, pAction->m_nActionParam, 0, 0);
                CExoLinkedListNode* pOldNode = pNode;
                CSWSCombatRoundAction* pOldAction = pAction;
                pAction = static_cast<CSWSCombatRoundAction*>(pInternal->GetPrev(pNode));
                if (pOldNode != 0) {
                    pInternal->Remove(pOldNode);
                    delete pOldAction;
                }
                continue;
            }
        }
        pAction = static_cast<CSWSCombatRoundAction*>(pInternal->GetPrev(pNode));
    }
}

void CSWSCreature::RecomputeAmbientAnimationState()
{
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp()
    if (GetDead() != 0 || GetIsPCDying() != 0 || GetDeadTemp() != 0) {
#else
    if (GetDead() != 0 || (m_bInParty != 0 && GetCurrentHitPoints(0) <= 0)) {
#endif
        if (m_ambientAnimationState != 3 && m_ambientAnimationState != 4 &&
            m_ambientAnimationState != 14) {
            if (m_nAnimation == SERVER_ANIMATION_WHIRLWIND || m_nAnimation == SERVER_ANIMATION_KNOCKED_DOWN_LOOP
                || m_nAnimation == SERVER_ANIMATION_PRONE)
                m_ambientAnimationState = 14;
            else
                m_ambientAnimationState = 4;
        }
    } else {
        switch (m_nState) {
        case EFFECT_STATE_DROID_STUN:      m_ambientAnimationState = 8;   break;
        case EFFECT_STATE_STUNNED:         m_ambientAnimationState = 2;   break;
        case EFFECT_STATE_PARALYZED:       m_ambientAnimationState = 12;  break;
        case EFFECT_STATE_SLEEP:           m_ambientAnimationState = 11;  break;
        case EFFECT_STATE_CHOKE:           m_ambientAnimationState = 5;   break;
        case EFFECT_STATE_HORRIFIED:       m_ambientAnimationState = 7;   break;
        case 9:                            m_ambientAnimationState = 9;   break;
        case EFFECT_STATE_WHIRLWIND:       m_ambientAnimationState = 6;   break;
        case 12:                           m_ambientAnimationState = 10;  break;
        case EFFECT_STATE_KNOCKBACK:       m_ambientAnimationState = 13;  break;
#if !KOTOR1  // K1: no additional ambient states
        case EFFECT_STATE_KNOCKDOWN:       m_ambientAnimationState = 15;  break;
        case EFFECT_STATE_FORCE_CRUSH:     m_ambientAnimationState = 16;  break;
        case EFFECT_STATE_MIND_TRICK:
        case EFFECT_STATE_DROID_SCRAMBLE:  m_ambientAnimationState = 2;   break;
#endif
        default:                           m_ambientAnimationState = 0;   break;
        }
    }
    SetAnimation(SERVER_ANIMATION_PAUSE);
}

void CSWSCreature::EventHandler(unsigned int nEventType, unsigned long long nCallerId, void* pData,
                                unsigned int a, unsigned int b)
{
    K2_UNUSED(a); K2_UNUSED(b);
    // Spawn-in perception is lazy, so every event, not just the heartbeat, pays this check once.
    if (m_bSpawnPerceptionDone == 0)
        SpawnInHeartbeatPerception(0);

    if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_APPLY_EFFECT)) {
        if (pData != 0) {
            CGameEffect* pEffect = static_cast<CGameEffect*>(pData);
            if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_LINK) {
                m_bApplyingLinkEffect = 1;
                ApplyEffect(pEffect, 0, 1);
                m_bApplyingLinkEffect = 0;
            } else {
                ApplyEffect(pEffect, 0, 0);
            }
        }
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_REMOVE_EFFECT)) {
        if (pData != 0)
            RemoveEffectById(*static_cast<unsigned long long*>(pData));
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_REMOVE_FROM_AREA)) {
        if (pData != 0)
            RemoveFromArea(static_cast<int>(reinterpret_cast<uintptr_t>(pData)));
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_PLAY_ANIMATION)) {
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp()
        if (pData != 0 && GetDead() == 0 && GetIsPCDying() == 0 && GetDeadTemp() == 0)
#else
        if (pData != 0 && GetDead() == 0 &&
            (m_bInParty == 0 || GetCurrentHitPoints(0) >= 1))
#endif
            SetAnimation(static_cast<int>(reinterpret_cast<uintptr_t>(pData)));
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_TIMED_EVENT)) {
        if (pData != 0)
            g_pVirtualMachine->RunScriptSituation(pData, m_objectId, 1);
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_BROADCAST_AOO)) {
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_BROADCAST_SAFE_PROJECTILE)) {
        if (pData != 0) {
            CSWSCombatAttackData* pAttack = static_cast<CSWSCombatAttackData*>(pData);
            CSWSCreature* pAttacker =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nCallerId);
            CSWSCreature* pTarget =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pAttack->m_targetObjectId);
            if (pAttacker != 0 && pTarget != 0) {
                CGameObject* pObject =
                    g_pAppManager->m_pServerExoApp->GetGameObject(pAttack->m_oidAmmoItem);
                if (pObject != 0) {
                    CSWBaseItem* pBaseItem = static_cast<CSWSItem*>(pObject)->GetBaseItem();
                    BroadcastSafeProjectile(nCallerId, pTarget->m_objectId,
                        pAttacker->m_position, pTarget->m_position, pAttack->m_nReaxnDelay,
                        pAttack->m_nAttackResult, 2, pBaseItem->m_nAmmunitionType, 0, 0);
                }
            }
            delete pAttack;
        }
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_FEEDBACK_MESSAGE)) {
        if (pData != 0) {
            CSWCCMessageData* pMessage = static_cast<CSWCCMessageData*>(pData);
            CSWSCreature* pCaller =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nCallerId);
            // Only the saving-throw and default arms own the payload; the caller-only and
            // self-only arms return without freeing it.
            switch (pMessage->m_nType) {
            case CSWCCMessageData::MESSAGE_TYPE_FEEDBACK_TO_SELF:
                SendFeedbackMessage(static_cast<unsigned short>(pMessage->GetInteger(0)), pMessage);
                break;
            case CSWCCMessageData::MESSAGE_TYPE_FEEDBACK_TO_CALLER:
                if (pCaller != 0)
                    pCaller->SendFeedbackMessage(static_cast<unsigned short>(pMessage->GetInteger(0)), pMessage);
                break;
            case CSWCCMessageData::MESSAGE_TYPE_SAVING_THROW: {
                BroadcastSavingThrowData(pMessage);
                if (pCaller != 0)
                    pCaller->BroadcastSavingThrowData(pMessage);
                delete pMessage;
                break;
            }
            default: {
                CSWCCMessageData* pCopy = new CSWCCMessageData();
                pMessage->CopyTo(pCopy);
                SendFeedbackMessage(static_cast<unsigned short>(pMessage->GetInteger(0)), pMessage);
                if (pCaller != 0)
                    // pCopy is handed over and leaked; only the branch with no recipient below
                    // frees it.
                    pCaller->SendFeedbackMessage(static_cast<unsigned short>(pCopy->GetInteger(0)), pCopy);
                else
                    delete pCopy;
                break;
            }
            }
        }
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_ON_MELEE_ATTACKED)) {
        CSWSCreature* pAttacker =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nCallerId);
        CSWSItem* pWeapon = 0;
        if (pData != 0) {
            CSWSCombatAttackData* pAttack = static_cast<CSWSCombatAttackData*>(pData);
            // The message is allocated, filled in and destroyed without ever being sent.
            CSWCCMessageData* pMessage = new CSWCCMessageData();
            pMessage->SetObjectID(0, nCallerId);
            pMessage->SetObjectID(1, m_objectId);
            if (g_bEnableCombatDebugging != 0)
                pMessage->SetString(0, pAttack->m_sAttackDebugText);
            delete pMessage;
            m_nLastAttackFeatId = pAttack->m_nFeatId;
            m_nLastAttackMode = pAttack->m_nAttackMode;
            // The attacking weapon is whichever equipment slot the attack type names.
            switch (pAttack->m_nWeaponAttackType) {
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_ONHAND:
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_EXTRA:
                pWeapon = static_cast<CSWSItem*>(
                    pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
                break;
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_OFFHAND:
                pWeapon = static_cast<CSWSItem*>(
                    pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
                break;
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON1:
                pWeapon = static_cast<CSWSItem*>(
                    pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_CWEAPON_L));
                break;
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON2:
                pWeapon = static_cast<CSWSItem*>(
                    pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_CWEAPON_R));
                break;
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON3:
                pWeapon = static_cast<CSWSItem*>(
                    pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_CWEAPON_B));
                break;
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_UNARMED:
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_UNARMED_EXTRA:
                pWeapon = static_cast<CSWSItem*>(
                    pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_HANDS));
                break;
            default: pWeapon = 0; break;
            }
        }
        m_oidLastAttacker = nCallerId;
        SetLastHostileActor(nCallerId, 0);
        if (pAttacker != 0) {
            pAttacker->BroadcastCombatStateToParty(m_objectId);
            pAttacker->RemoveCombatInvisibilityEffects();
            if (pWeapon != 0)
                pAttacker->m_oidLastWeaponUsed = pWeapon->m_objectId;
            else
                pAttacker->m_oidLastWeaponUsed = OBJECT_INVALID;
        }
        BroadcastCombatStateToParty(nCallerId);
        m_bInitiativeExpired = (m_bInParty == 0);
#if KOTOR1  // K1: GetDeadTemp(), and no mind-trick removal
        if (GetDead() == 0 && GetDeadTemp() == 0)
            g_pVirtualMachine->RunScript(&m_scripts[CREATURE_SCRIPT_ATTACKED], m_objectId, 1);
#else
        if (GetDead() == 0 && (m_bInParty == 0 || GetCurrentHitPoints(0) >= 1)) {
            g_pVirtualMachine->RunScript(&m_scripts[CREATURE_SCRIPT_ATTACKED], m_objectId, 1);
            RemoveMindTrickEffects();
        }
#endif
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_SPELL_IMPACT) ||
               nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_ITEM_ON_HIT_SPELL_IMPACT)) {
        // The creature twin of CSWSPlaceable::EventHandler's SPELL_IMPACT arm: same payload, same
        // late-arrival drop, plus the item rider and the two script slots the placeable lacks.
        CSWSSpellScriptData* pImpact = static_cast<CSWSSpellScriptData*>(pData);
        if (pImpact == 0) return;
        if (pImpact->m_casterId == nCallerId) {
            m_castSpellId = pImpact->m_nSpellId;
            m_castTargetId = pImpact->m_targetId;
            m_castPosition = pImpact->m_targetPosition;
            m_lastSpellItemId = pImpact->m_itemId;
#if !KOTOR1  // K1: no remembered force-point cost
            m_nLastSpellForcePointCost = pImpact->m_nFinalForceCost;
#endif
            if (pImpact->m_targetId != OBJECT_INVALID) {
                CGameObject* pTarget;
                if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(
                        pImpact->m_targetId, &pTarget) == CGameObjectArray::SUCCESS &&
                    pTarget->m_objectType > CGameObject::OBJECT_TYPE_AREA &&
                    GetArea() == static_cast<CSWSObject*>(pTarget)->GetArea()) {
                    m_castPosition = static_cast<CSWSObject*>(pTarget)->m_position;
                }
            }
            SetEffectSpellId(pImpact->m_nSpellId);
            m_nImpactSpellId = pImpact->m_nSpellId;
            // Only the ITEM_ON_HIT flavour arms the on-hit rider pair, and only for the duration
            // of the impact script (both are reset to their sentinels below).
            if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_ITEM_ON_HIT_SPELL_IMPACT)) {
                m_bItemCastSpell = 1;
                m_nItemCastSpellLevel = pImpact->m_nCasterLevel;
                CalculateLastSpellProjectileTime(0);
            }
            g_pVirtualMachine->RunScript(&pImpact->m_sScript, m_objectId, 1);
            SetEffectSpellId(static_cast<unsigned int>(-1));
            m_nImpactSpellId = -1;
            m_bItemCastSpell = 0;
            m_nItemCastSpellLevel = -1;
        }
        // No destructor, the same engine bug the placeable arm carries.
        operator delete(pImpact);
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_DESTROY_OBJECT)) {
        // Three stripped debug-log lines, whose CStr() calls survive with their results discarded.
        if (m_bPlayerCharacter == 1)
            m_sTag.CStr();
        if (m_bDestroyable == 0)
            m_sTag.CStr();
        if (m_bInParty != 0)
            m_sTag.CStr();
        // Player characters, party members and objects not yet marked destroyable never die here.
        if (m_bPlayerCharacter != 0 || m_bDestroyable == 0 || m_bInParty != 0)
            return;
        if (GetDead() != 0) {
            // Hand the client-side creature the body bag it should leave behind.
            unsigned long long clientId =
                g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId);
            unsigned long long bodyBagId =
                g_pAppManager->m_pClientExoApp->ServerToClientObjectId(SpawnBodyBag());
            CSWCCreature* pClientCreature =
                g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(clientId);
            if (pClientCreature != 0)
                pClientCreature->m_oidBodyBag = bodyBagId;
        }
        // pData doubles as a "skip the corpse-fade decision" flag on this arm.
        if (static_cast<int>(reinterpret_cast<uintptr_t>(pData)) == 0) {
            int nFadeDelay;
            if (g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
                    m_appearance.m_headId, CExoString("FadeDelayOnDeath"), &nFadeDelay) != 0) {
                SetFadeOutTime(nFadeDelay);
                SetKeepCorpse(0);
            } else {
                SetKeepCorpse(1);
            }
        }
        RemoveFromArea(0);
        // Detach from every armed mine/trap object in the area that is still holding this
        // creature's id: the three trappable kinds each keep their own "who is standing on me"
        // list, so the walk re-dispatches per object type.
        if (GetArea() != 0) {
            CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
            for (int64_t i = 0; i < pArea->m_mineObjects.m_count; i++) {
                CGameObject* pObject =
                    g_pAppManager->m_pServerExoApp->GetGameObject(pArea->m_mineObjects.m_pArray[i]);
                if (pObject != 0) {
                    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER) {
                        if (pObject->AsSWSTrigger()->m_objectsInTrigger.Contains(m_objectId))
                            pObject->AsSWSTrigger()->m_objectsInTrigger.Remove(m_objectId);
                    } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR) {
                        if (pObject->AsSWSDoor()->m_objectsAtDoor.Contains(m_objectId))
                            pObject->AsSWSDoor()->m_objectsAtDoor.Remove(m_objectId);
                    } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
                        if (pObject->AsSWSPlaceable()->m_objectsOnPlaceable.Contains(m_objectId))
                            pObject->AsSWSPlaceable()->m_objectsOnPlaceable.Remove(m_objectId);
                    }
                }
            }
        }
        // Give the spawning encounter its slot back, exactly once.
        if (m_bEncounterSlotReleased == 0) {
            CSWSEncounter* pEncounter =
                g_pAppManager->m_pServerExoApp->GetEncounterByGameObjectID(m_oidEncounter);
            if (pEncounter != 0) {
                pEncounter->RemoveFromActiveCreatureCount(m_pStats->m_fChallengeRating, 0);
                m_bEncounterSlotReleased = 1;
            }
        }
        delete this;
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_FORCED_ACTION)) {
        CSWSForcedAction* pAction = static_cast<CSWSForcedAction*>(pData);
        if (CancelAction(0, pAction->m_nGroupId) == 1 && pAction->m_nActionId == AIACTION_MOVE_TO_POINT)
            ForceMoveToPoint(pAction);
        delete pAction;
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_SIGNAL_EVENT)) {
        // There is no null check on pData; this arm dereferences it immediately.
        CScriptEvent* pEvent = static_cast<CScriptEvent*>(pData);
        switch (pEvent->m_eventType) {
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_PERCEPTION: {
            // "I have noticed you": remember who, then record what about them in a one-hot set of
            // four flags, then run the notice script.
            m_oidLastPerceived = pEvent->GetObjectID(0);
            if (pEvent->GetInteger(0) == PERCEPTION_EVENT_INAUDIBLE) {
                m_bLastPerceptionHeard = 0; m_bLastPerceptionSeen = 0; m_bLastPerceptionInaudible = 1; m_bLastPerceptionVanished = 0;
            } else if (pEvent->GetInteger(0) == PERCEPTION_EVENT_HEARD) {
                m_bLastPerceptionHeard = 1; m_bLastPerceptionSeen = 0; m_bLastPerceptionInaudible = 0; m_bLastPerceptionVanished = 0;
            } else if (pEvent->GetInteger(0) == PERCEPTION_EVENT_VANISHED) {
                m_bLastPerceptionHeard = 0; m_bLastPerceptionSeen = 0; m_bLastPerceptionInaudible = 0; m_bLastPerceptionVanished = 1;
            } else if (pEvent->GetInteger(0) == PERCEPTION_EVENT_SEEN) {
                m_bLastPerceptionHeard = 0; m_bLastPerceptionSeen = 1; m_bLastPerceptionInaudible = 0; m_bLastPerceptionVanished = 0;
            }
            // Party members only run their notice script if they are actually in the party.
            if (m_bInParty == 0 ||
                g_pAppManager->m_pClientExoApp->GetSWParty()->IsPartyMember(
                    g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId)) != 0)
                g_pVirtualMachine->RunScript(&m_scripts[CREATURE_SCRIPT_ONNOTICE], m_objectId, 1);
            break;
        }
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_SPELLCASTAT: {
            // "A spell/force power was cast at me." The caster is normally the event's object-0
            // parameter, but an area-of-effect object re-credits the hit to whoever spawned it.
            CGameObject* pObject =
                g_pAppManager->m_pServerExoApp->GetGameObject(pEvent->GetObjectID(0));
            if (pObject != 0 && pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE) {
                m_oidLastSpellCaster = pEvent->GetObjectID(0);
            } else if (pObject != 0 &&
                       pObject->m_objectType == CGameObject::OBJECT_TYPE_AREAOFEFFECTOBJECT) {
                CSWSCreature* pCreator =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                        pObject->AsSWSAreaOfEffectObject()->m_oidCreator);
                if (pCreator != 0)
                    m_oidLastSpellCaster = pCreator->m_objectId;
                else
                    m_oidLastSpellCaster = OBJECT_INVALID;
            } else {
                m_oidLastSpellCaster = OBJECT_INVALID;
            }
            m_nLastSpellId = pEvent->GetInteger(0);
            m_bLastSpellHarmful = pEvent->GetInteger(1);
            if (m_bLastSpellHarmful == 1)
                SetLastHostileActor(m_oidLastSpellCaster, 0);
#if KOTOR1
            if (GetDead() == 0 && GetDeadTemp() == 0)
                g_pVirtualMachine->RunScript(&m_scripts[CREATURE_SCRIPT_SPELLAT], m_objectId, 1);
#else
            if (GetDead() == 0 && (m_bInParty == 0 || GetCurrentHitPoints(0) >= 1))
                g_pVirtualMachine->RunScript(&m_scripts[CREATURE_SCRIPT_SPELLAT], m_objectId, 1);
#endif
            CSWSCreature* pCaster =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidLastSpellCaster);
            if (pCaster != 0) {
                if (m_bLastSpellHarmful != 0) {
                    // A hostile cast drags both sides into combat.
                    pCaster->BroadcastCombatStateToParty(m_objectId);
                    pCaster->m_bInitiativeExpired = (pCaster->m_bInParty == 0);
                    pCaster->SetExcitedState(2);
                    pCaster->RemoveCombatInvisibilityEffects();
                    BroadcastCombatStateToParty(pCaster->m_objectId);
                    m_bInitiativeExpired = (m_bInParty == 0);
                    SetExcitedState(1);
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()
                        ->AdjustTargetAndWitnessReputations(m_objectId, m_oidLastSpellCaster, 0);
                } else {
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()
                        ->AdjustTargetAndWitnessReputations(m_objectId, m_oidLastSpellCaster, 3);
                }
            }
            break;
        }
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_DIALOGUE: {
            // Dialogue. The event carries the conversation resref, the listen-pattern expression
            // strings, and three conversation-mode ints that go to the server app.
            m_oidLastSpeaker = nCallerId;
            m_nListenPatternNumber = pEvent->GetInteger(1);
            m_sLastConversation = pEvent->GetString(0);
            g_pAppManager->m_pServerExoApp->SetLastConversation(pEvent->GetString(0));
            m_bPrivateConversation = pEvent->GetInteger(2);
            g_pAppManager->m_pServerExoApp->SetActionConversation(1);
            g_pAppManager->m_pServerExoApp->SetIgnoreConversationStartRange(pEvent->GetInteger(3));
            ClearMatchedExpressionStrings();
            // Integer 0 is the total string count; string 0 is the conversation, 1..n-1 are the
            // matched listen-pattern expressions.
            for (int i = 1; i < pEvent->GetInteger(0); i++)
                AddMatchedExpressionString(pEvent->GetString(i));
            // An empty or literally "default" dialogue slot falls back to the generic henchman
            // conversation rather than running nothing.
#if KOTOR1  // K1: CExoString::IsEmpty is out of line
            if (m_scripts[CREATURE_SCRIPT_DIALOGUE].IsEmpty() ||
                m_scripts[CREATURE_SCRIPT_DIALOGUE] == "default")
#else
            if (m_scripts[CREATURE_SCRIPT_DIALOGUE].m_sString == 0 ||
                *m_scripts[CREATURE_SCRIPT_DIALOGUE].m_sString == 0 ||
                m_scripts[CREATURE_SCRIPT_DIALOGUE] == "default")
#endif
                m_scripts[CREATURE_SCRIPT_DIALOGUE] = "k_hen_dialogue01";
            m_scripts[CREATURE_SCRIPT_DIALOGUE].CStr();  // stripped debug line
            g_pVirtualMachine->RunScript(&m_scripts[CREATURE_SCRIPT_DIALOGUE], m_objectId, 1);
            break;
        }
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_USER_DEFINED_EVENT:
            m_nUserDefinedEventNumber = pEvent->GetInteger(0);
            g_pVirtualMachine->RunScript(&m_scripts[CREATURE_SCRIPT_USERDEFINE], m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_INVENTORY_DISTURBED:
            m_oidLastDisturbedBy = nCallerId;
            SetLastHostileActor(nCallerId, 0);
            m_nLastDisturbType = pEvent->GetInteger(0);
            m_oidLastDisturbItem = pEvent->GetObjectID(0);
            g_pVirtualMachine->RunScript(&m_scripts[CREATURE_SCRIPT_DISTURBED], m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_PATH_BLOCKED:
            // Blocked-by-object, and only for creatures no player is driving.
            if (g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId) == 0) {
                m_oidBlockingObject = nCallerId;
                g_pVirtualMachine->RunScript(&m_scripts[CREATURE_SCRIPT_ONBLOCKED], m_objectId, 1);
            }
            break;
        }
        delete pEvent;
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_CONTROLLER_RUMBLE)) {
        g_pAppManager->m_pClientExoApp->PlayRumblePattern(static_cast<int>(reinterpret_cast<uintptr_t>(pData)));
    }
}

void CSWSCreature::AddPazaakCard(CSWSItem* pItem)
{
    unsigned char nCard = pItem->m_nModelVariation;
#if KOTOR1  // K1: 18 side-deck cards, no special cards
    unsigned int nIndex = (nCard + 11) % 18;
#else
    static const long long nPazaakCardMap[5] = { 22, 19, 18, 20, 21 };
    long long nIndex;
    if (nCard >= 1 && nCard <= 18)
        nIndex = static_cast<unsigned char>(nCard + 11) % 18;
    else if (static_cast<unsigned char>(nCard - 19) <= 4) {
        K2_ASSUME(static_cast<signed char>(nCard - 19) >= 0 && static_cast<signed char>(nCard - 19) <= 4);
        nIndex = nPazaakCardMap[static_cast<signed char>(nCard - 19)];
    }
    else
        nIndex = 0;
#endif
    CSWPartyTable* pTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    int nOld = pTable->m_pazaakCards[nIndex];
    pTable->m_pazaakCards[nIndex] = pItem->m_stackCount + nOld;
    CItemRepository* pRepo = GetItemRepository(1);
    if (pRepo->FindItemWithBaseItemType(ITEM_TYPE_PAZAAK_SIDEBOARD, 0) == OBJECT_INVALID) {
        CSWSItem* pCard = new CSWSItem(OBJECT_INVALID);
        CResRef templateRef("g_I_PazSidebd001");
        pCard->LoadFromTemplate(templateRef);
        pRepo->AddItem(&pCard, 1, 0, 1);
    }
}

void CSWSCreature::UpdatePersonalSpace()
{
    float fValue = 0.6f;
#if KOTOR1  // K1: one pathfind read per field, each field one store of a selected value
    int bFound = g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
        m_appearance.m_headId, CTwoDimArrays::COL2DA_APPEARANCETABLE_PERSPACE, &fValue);
    CPathfindInformation* pPathfind = m_pPathfindInfo;
    if (bFound == 1) {
        pPathfind->m_fPersonalSpace = fValue;
        pPathfind->ComputeStepTolerance();
    } else {
        pPathfind->m_fPersonalSpace = 0.6f;
        pPathfind->ComputeStepTolerance();
    }
    g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
        m_appearance.m_headId, CTwoDimArrays::COL2DA_APPEARANCETABLE_CREPERSPACE, &fValue);
    pPathfind = m_pPathfindInfo;
    pPathfind->m_fCrePersonalSpace = fValue;
    bFound = g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
        m_appearance.m_headId, CTwoDimArrays::COL2DA_APPEARANCETABLE_CAMERASPACE, &fValue);
    pPathfind = m_pPathfindInfo;
    if (bFound == 1)
        pPathfind->m_fCameraSpace = fValue;
    else
        pPathfind->m_fCameraSpace = pPathfind->m_fCrePersonalSpace;
    bFound = g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
        m_appearance.m_headId, CTwoDimArrays::COL2DA_APPEARANCETABLE_HITDIST, &fValue);
    pPathfind = m_pPathfindInfo;
    if (bFound == 1)
        pPathfind->m_fHitDistance = fValue;
    else
        pPathfind->m_fHitDistance = 0.5f;
    bFound = g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
        m_appearance.m_headId, CTwoDimArrays::COL2DA_APPEARANCETABLE_HEIGHT, &fValue);
    pPathfind = m_pPathfindInfo;
    if (bFound == 1)
        pPathfind->m_fHeight = fValue;
    else
        pPathfind->m_fHeight = 1.0f;
#else
    if (g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
            m_appearance.m_headId, CTwoDimArrays::COL2DA_APPEARANCETABLE_PERSPACE,
            &fValue) == 1) {
        m_pPathfindInfo->m_fPersonalSpace = fValue;
        m_pPathfindInfo->ComputeStepTolerance();
    } else {
        m_pPathfindInfo->m_fPersonalSpace = 0.6f;
        m_pPathfindInfo->ComputeStepTolerance();
    }
    g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
        m_appearance.m_headId, CTwoDimArrays::COL2DA_APPEARANCETABLE_CREPERSPACE, &fValue);
    m_pPathfindInfo->m_fCrePersonalSpace = fValue;
    if (g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
            m_appearance.m_headId, CTwoDimArrays::COL2DA_APPEARANCETABLE_CAMERASPACE,
            &fValue) == 1)
        m_pPathfindInfo->m_fCameraSpace = fValue;
    else
        m_pPathfindInfo->m_fCameraSpace = m_pPathfindInfo->m_fCrePersonalSpace;
    if (g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
            m_appearance.m_headId, CTwoDimArrays::COL2DA_APPEARANCETABLE_HITDIST,
            &fValue) == 1)
        m_pPathfindInfo->m_fHitDistance = fValue;
    else
        m_pPathfindInfo->m_fHitDistance = 0.5f;
    if (g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
            m_appearance.m_headId, CTwoDimArrays::COL2DA_APPEARANCETABLE_HEIGHT,
            &fValue) == 1)
        m_pPathfindInfo->m_fHeight = fValue;
    else
        m_pPathfindInfo->m_fHeight = 1.0f;
#endif
}

int CSWSCreature::UnequipItem(CSWSItem* pItem)
{
#if !KOTOR1
    int nSlot = m_pInventory->FindSlot(pItem->m_baseItem);
#endif
    unsigned int nSlotFromItem = m_pInventory->GetSlotFromItem(pItem);
    pItem->RemoveItemProperties(this, nSlotFromItem);
    m_pInventory->RemoveItem(pItem);

    CSWItem* pEquipped = m_pInventory->GetItemInSlot(CSWInventory::SLOT_BODY);
    CSWBaseItem* pEquippedBase = pEquipped == NULL ? NULL : pEquipped->GetBaseItem();
    if (pEquippedBase == NULL)
        m_pStats->m_nACArmorBase = 0;
    else
        m_pStats->m_nACArmorBase = pEquippedBase->m_nBaseAC;

    int nWeight = m_nEquippedWeight;
    m_bUpdateCombatInformation = 1;
    m_nEquippedWeight = nWeight - pItem->GetWeight();

    if (static_cast<CSWItem*>(pItem)->GetBaseItem() != NULL &&
        static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_STEALTH_UNIT &&
        m_stealthMode == 1)
    {
        ToggleMode(TOGGLE_MODE_STEALTH);
    }

#if !KOTOR1  // K1: no armour-appearance disguise
    if (nSlot == 2) {
        char sModelA[] = "ModelA";
        CExoString sAppearance;
        unsigned short nAppearance = m_pStats->m_nAppearanceType;
        C2DA* pAppearance2DA = g_pRules->m_p2DArrays->m_pAppearance;
        int bFound;
        {
            CExoString sColumn(sModelA);
            bFound = pAppearance2DA->GetCExoStringEntry(nAppearance, sColumn, &sAppearance);
        }
        if (!bFound)
            sAppearance = "";
        if (sAppearance[0] == '&') {
            sAppearance = sAppearance.SubString(1, -1);
            int nDisguise = atoi(sAppearance.CStr());
            if (nDisguise != 0 && nDisguise != m_pStats->m_nAppearanceType)
                Disguise(nDisguise, NULL, 0, 1);
        }
    }
#endif
    return 1;
}

#if !KOTOR1
int CSWSCreature::RunUnequip(unsigned long long itemId, unsigned long long targetId, int b,
                             CSWSItem** ppItem, int c, int d)
{
    CSWSPlayer* pPlayer;
    if (g_pAppManager->m_pClientExoApp->GetInGameGui() != 0)
    {
        int nOverride =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->GetTopMenuOverrideCharacter();
        CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
        if (nOverride != -1)
            pPlayer = pServer->GetClientObjectByObjectId(
                pServer->GetCreatureByGameObjectID(pServer->GetPlayerCreatureId())->m_objectId);
        else
            pPlayer = pServer->GetClientObjectByObjectId(m_objectId);
    }
    else
    {
        pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
    }
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    if (ppItem != 0)
        *ppItem = 0;

    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(itemId);
    if (pItem != 0 && pItem->m_possessorId == m_objectId &&
        m_pInventory->GetItemInInventory(pItem) == 1)
    {
        CItemRepository* pRepository = 0;
        if (targetId == OBJECT_INVALID)
        {
            pRepository = GetItemRepository(1);
        }
        else
        {
            CSWSItem* pContainer = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(targetId);
            if (pContainer != 0 && pContainer->m_possessorId == m_objectId)
                pRepository = pContainer->m_pItemRepository;
        }
        if (pRepository != 0)
        {
            unsigned char nVerdict;
            CSWSItem* pRightHand =
                    static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
            CSWSItem* pLeftHand =
                    static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
            if ((pRightHand != 0 && pRightHand->m_objectId == pItem->m_objectId) ||
                (pLeftHand != 0 && pLeftHand->m_objectId == pItem->m_objectId))
            {
                nVerdict = CanUnEquipWeapon(pItem);
            }
            else
            {
                CSWSItem* pAltRightHand =
                        static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON2));
                CSWSItem* pAltLeftHand =
                        static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON2));
                if ((pAltRightHand != 0 && pAltRightHand->m_objectId == pItem->m_objectId) ||
                    (pAltLeftHand != 0 && pAltLeftHand->m_objectId == pItem->m_objectId))
                    nVerdict = CanUnEquipWeapon2(pItem);
                else
                    nVerdict = CanUnEquipWeapon(pItem);
            }

            if (nVerdict == 2 || nVerdict == 3)
            {
                unsigned int nSlot = m_pInventory->GetSlotFromItem(pItem);
                if (nSlot == CSWInventory::SLOT_LEFTWEAPON)
                    m_bLastInstantEquipLeft = c;
                else if (nSlot == CSWInventory::SLOT_RIGHTWEAPON)
                    m_bLastInstantEquipRight = c;

                UnequipItem(pItem);
                if (pRepository->AddItem(&pItem, b, 0, 0) == 0)
                    pItem->AddToArea(static_cast<CSWSArea*>(GetArea()), m_position.x, m_position.y,
                                     m_position.z + 0.2f, 1);

                CSWSItem* pPartner =
                    static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(nVerdict == 3 ? CSWInventory::SLOT_LEFTWEAPON2 : CSWInventory::SLOT_LEFTWEAPON));
                if (pPartner != 0)
                {
                    UnequipItem(pPartner);
                    EquipItem(nVerdict == 3 ? CSWInventory::SLOT_RIGHTWEAPON2 : CSWInventory::SLOT_RIGHTWEAPON, pPartner, 1, 0, 0);
                }

                if (ppItem != 0)
                    *ppItem = pItem;
                pItem->SetPossessor(targetId == OBJECT_INVALID ? m_objectId : targetId, 0, 0,
                                    OBJECT_INVALID);
                if (pPlayer != 0 && pMessage != 0)
                {
                    K2_ASSUME(pPartner != 0);
                    pMessage->SendServerToPlayerInventory_Unequip(pPlayer->m_nPlayerId,
                                                                  pPartner->m_objectId, c, d);
                    pMessage->SendServerToPlayerInventory_Unequip(pPlayer->m_nPlayerId,
                                                                  pItem->m_objectId, c, d);
                    return 1;
                }
            }
            else if (nVerdict == 1)
            {
                unsigned int nSlot = m_pInventory->GetSlotFromItem(pItem);
                if (nSlot == CSWInventory::SLOT_LEFTWEAPON)
                    m_bLastInstantEquipLeft = c;
                else if (nSlot == CSWInventory::SLOT_RIGHTWEAPON)
                    m_bLastInstantEquipRight = c;

                UnequipItem(pItem);
                if (pRepository->AddItem(&pItem, b, 0, 0) == 0)
                    pItem->AddToArea(static_cast<CSWSArea*>(GetArea()), m_position.x, m_position.y,
                                     m_position.z + 0.2f, 1);

                if (ppItem != 0)
                    *ppItem = pItem;
                pItem->SetPossessor(targetId == OBJECT_INVALID ? m_objectId : targetId, 0, 0,
                                    OBJECT_INVALID);
                if (pPlayer != 0 && pMessage != 0)
                {
                    pMessage->SendServerToPlayerInventory_Unequip(pPlayer->m_nPlayerId, itemId,
                                                                  c, d);
                    return 1;
                }
            }
        }
    }

    if (pPlayer != 0 && pMessage != 0)
        pMessage->SendServerToPlayerInventory_UnequipCancel(pPlayer->m_nPlayerId, itemId);
    return 0;
}
#else
int CSWSCreature::RunUnequip(unsigned long long itemId, unsigned long long targetId, int b,
                             CSWSItem** ppItem, int c)
{
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    if (ppItem != 0)
        *ppItem = 0;

    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(itemId);
    if (pItem != 0 && pItem->m_possessorId == m_objectId &&
        m_pInventory->GetItemInInventory(pItem) == 1)
    {
        CItemRepository* pRepository = 0;
        if (targetId == OBJECT_INVALID)
        {
            pRepository = GetItemRepository(1);
        }
        else
        {
            CSWSItem* pContainer = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(targetId);
            if (pContainer != 0 && pContainer->m_possessorId == m_objectId)
                pRepository = pContainer->m_pItemRepository;
        }
        if (pRepository != 0)
        {
            unsigned char nVerdict = CanUnEquipWeapon(pItem);
            if (nVerdict == 1)
            {
                unsigned int nSlot = m_pInventory->GetSlotFromItem(pItem);
                if (nSlot == CSWInventory::SLOT_RIGHTWEAPON)
                    m_bLastInstantEquipRight = c;
                else if (nSlot == CSWInventory::SLOT_LEFTWEAPON)
                    m_bLastInstantEquipLeft = c;

                UnequipItem(pItem);
                if (pRepository->AddItem(&pItem, b, 0, 0) == 0)
                    pItem->AddToArea(static_cast<CSWSArea*>(GetArea()), m_position.x, m_position.y,
                                     m_position.z + 0.2f, 1);

                if (ppItem != 0)
                    *ppItem = pItem;
                pItem->SetPossessor(targetId == OBJECT_INVALID ? m_objectId : targetId, 0, 0,
                                    OBJECT_INVALID);
                if (pPlayer != 0 && pMessage != 0)
                {
                    pMessage->SendServerToPlayerInventory_Unequip(pPlayer->m_nPlayerId, itemId,
                                                                  c);
                    return 1;
                }
            }
            else if (nVerdict == 2)
            {
                unsigned int nSlot = m_pInventory->GetSlotFromItem(pItem);
                if (nSlot == CSWInventory::SLOT_RIGHTWEAPON)
                    m_bLastInstantEquipRight = c;
                else if (nSlot == CSWInventory::SLOT_LEFTWEAPON)
                    m_bLastInstantEquipLeft = c;

                UnequipItem(pItem);
                if (pRepository->AddItem(&pItem, b, 0, 0) == 0)
                    pItem->AddToArea(static_cast<CSWSArea*>(GetArea()), m_position.x, m_position.y,
                                     m_position.z + 0.2f, 1);

                CSWSItem* pPartner =
                        static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
                if (pPartner != 0)
                {
                    UnequipItem(pPartner);
                    EquipItem(CSWInventory::SLOT_RIGHTWEAPON, pPartner, 1, 0, 0);
                }

                if (ppItem != 0)
                    *ppItem = pItem;
                pItem->SetPossessor(targetId == OBJECT_INVALID ? m_objectId : targetId, 0, 0,
                                    OBJECT_INVALID);
                if (pPlayer != 0 && pMessage != 0)
                {
                    K2_ASSUME(pPartner != 0);
                    pMessage->SendServerToPlayerInventory_Unequip(pPlayer->m_nPlayerId,
                                                                  pPartner->m_objectId, c);
                    pMessage->SendServerToPlayerInventory_Unequip(pPlayer->m_nPlayerId,
                                                                  pItem->m_objectId, c);
                    return 1;
                }
            }
        }
    }

    if (pPlayer != 0 && pMessage != 0)
        pMessage->SendServerToPlayerInventory_UnequipCancel(pPlayer->m_nPlayerId, itemId);
    return 0;
}
#endif

int CSWSCreature::ValidNearestObjectType(CGameObject* pObject, int& nOut)
{
    if (pObject == NULL)
        return 0;
    nOut = 0;
    switch (pObject->m_objectType) {
    case GAME_OBJECT_TYPE_CREATURE: {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
#if KOTOR1  // K1: the dying test is GetDeadTemp()
        if (pCreature->GetDead() == 0 && pCreature->GetDeadTemp() == 0) {
#else
        if (pCreature->GetDead() == 0 &&
            (pCreature->m_bInParty == 0 || pCreature->GetCurrentHitPoints(0) >= 1)) {
#endif
            CSWVisibilityNode* pNode = GetVisibleListElement(pCreature->m_objectId);
            if ((pNode == NULL || (pNode->m_flags & VISIBILITY_SEEN) == 0) &&
                !DoSpotDetection(pCreature, 0))
                return 0;
            if (GetAIStateReputation(pCreature->m_objectId) != 2)
                return 1;
            nOut = 1;
            return 1;
        }
        break;
    }
    case GAME_OBJECT_TYPE_TRIGGER: {  // OBJECT_TYPE_TRIGGER
        CSWSTrigger* pTrigger = pObject->AsSWSTrigger();
        if (pTrigger == NULL)
            return 0;
        if (pTrigger->m_bTrap == 0)
            return 0;
        int nFound = 0;
        for (int i = 0; i < pTrigger->m_objectsInTrigger.m_count; i++)
            nFound += (pTrigger->m_objectsInTrigger.m_pArray[i] == m_objectId);
        if (nFound != 0)
            return 1;
        if (pTrigger->GetReputation(m_objectId, 0) > 89)
            return 1;
        if (pTrigger->m_nFaction == static_cast<unsigned int>(m_pStats->m_factionId))
            return 1;
        break;
    }
    case GAME_OBJECT_TYPE_PLACEABLE: {  // OBJECT_TYPE_PLACEABLE
        CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
        if (pPlaceable == NULL)
            return 0;
        if (pPlaceable->m_bUseable == 0)
            return 0;
        int nHostile = 0;
        if (pPlaceable->m_hostileState != 0)
            nHostile = (pPlaceable->GetReputation(m_objectId, 0) < 11);
        nOut = nHostile;
        return 1;
    }
    case GAME_OBJECT_TYPE_DOOR: {  // OBJECT_TYPE_DOOR
        CSWSDoor* pDoor = pObject->AsSWSDoor();
        if (pDoor == NULL)
            return 0;
        if (pDoor->m_curOpenState != 0)
            return 0;
        return pDoor->m_bStatic == 0;
    }
    }
    return 0;
}

void CSWSCreature::GetNearestObjects(float fHostileRange, float fRange,
                                     CExoArrayList<unsigned long long>& listInFront,
                                     CExoArrayList<unsigned long long>& listAround)
{
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());

    // Our own slot in the X-sorted index is the scan's origin; not being in it means there is
    // nothing to scan outward from.
    int nIndex;
    for (nIndex = 0; nIndex < pArea->m_aObjectsByX.m_count; nIndex++) {
        if (pArea->m_aObjectsByX.m_pArray[nIndex] == m_objectId)
            break;
    }
    listAround.Clear();
    if (nIndex >= pArea->m_aObjectsByX.m_count)
        return;

    CExoArrayList<float> listBearing;
    CExoArrayList<float> listBearingDistSq;
    CExoArrayList<float> listFrontDistSq;

    Vector vDelta;

    Vector vDir = normalize(m_orientation);
    Vector vFrom = m_position + vDir * -4.0f;
    float fSelfYaw = Yaw(vDir);

    int bDownActive = 1;
    int bUpActive = 1;
    for (int k = 1; bDownActive || bUpActive; k++) {
        if (bDownActive) {
            int i = nIndex - k;
            if (i < 0) {
                bDownActive = 0;
            } else {
                unsigned long long oid = pArea->m_aObjectsByX.m_pArray[i];
                if (oid != m_objectId) {
                    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oid);
                    int bHostile;
                    if (ValidNearestObjectType(pObject, bHostile)) {
                        CSWSObject* pOther = pObject->AsSWSObject();
                        vDelta.x = pOther->m_position.x - m_position.x;
                        vDelta.y = pOther->m_position.y - m_position.y;
                        vDelta.z = pOther->m_position.z - m_position.z;
                        float fDistSq = vDelta.x * vDelta.x + vDelta.y * vDelta.y
                                      + vDelta.z * vDelta.z;
                        float fLimit = bHostile != 0 ? fHostileRange : fRange;
                        if (fDistSq < fLimit * fLimit) {
                            float fBearing = Yaw(vDelta) - fSelfYaw;
                            while (fBearing < 0.0f)
                                fBearing += 360.0f;
                            while (fBearing > 359.0f)
                                fBearing -= 360.0f;

                            int j;
                            for (j = 0; j < listBearing.m_count; j++) {
                                if (fBearing < listBearing.m_pArray[j])
                                    break;
                            }
                            if (j < listBearing.m_count) {
                                listBearing.Insert(fBearing, j);
                                listBearingDistSq.Insert(fDistSq, j);
                                listAround.Insert(oid, j);
                            } else {
                                listBearing.Add(fBearing);
                                listBearingDistSq.Add(fDistSq);
                                listAround.Add(oid);
                            }

                            float fFacing = m_orientation.x * vDelta.x
                                          + m_orientation.y * vDelta.y
                                          + m_orientation.z * vDelta.z;
                            if (fFacing >= 0.0f) {
                                CSWSObject* pTo = pObject->AsSWSObject();
                                vDelta.x = pTo->m_position.x - vFrom.x;
                                vDelta.y = pTo->m_position.y - vFrom.y;
                                vDelta.z = pTo->m_position.z - vFrom.z;
                                vDelta.Normalize();
                                float fCone = m_orientation.x * vDelta.x
                                            + m_orientation.y * vDelta.y
                                            + m_orientation.z * vDelta.z;
                                if (fCone > 0.866) {
                                    int m;
                                    for (m = 0; m < listFrontDistSq.m_count; m++) {
                                        if (fDistSq < listFrontDistSq.m_pArray[m])
                                            break;
                                    }
                                    if (m < listFrontDistSq.m_count) {
                                        listFrontDistSq.Insert(fDistSq, m);
                                        listInFront.Insert(oid, m);
                                    } else {
                                        listFrontDistSq.Add(fDistSq);
                                        listInFront.Add(oid);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if (bUpActive) {
            int i = nIndex + k;
            if (i >= pArea->m_aObjectsByX.m_count) {
                bUpActive = 0;
            } else {
                unsigned long long oid = pArea->m_aObjectsByX.m_pArray[i];
                if (oid != m_objectId) {
                    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oid);
                    int bHostile;
                    if (ValidNearestObjectType(pObject, bHostile)) {
                        CSWSObject* pOther = pObject->AsSWSObject();
                        vDelta.x = pOther->m_position.x - m_position.x;
                        vDelta.y = pOther->m_position.y - m_position.y;
                        vDelta.z = pOther->m_position.z - m_position.z;
                        float fDistSq = vDelta.x * vDelta.x + vDelta.y * vDelta.y
                                      + vDelta.z * vDelta.z;
                        float fLimit = bHostile != 0 ? fHostileRange : fRange;
                        if (fDistSq < fLimit * fLimit) {
                            float fBearing = Yaw(vDelta) - fSelfYaw;
                            while (fBearing < 0.0f)
                                fBearing += 360.0f;
                            while (fBearing > 359.0f)
                                fBearing -= 360.0f;

                            int j;
                            for (j = 0; j < listBearing.m_count; j++) {
                                if (fBearing < listBearing.m_pArray[j])
                                    break;
                            }
                            if (j < listBearing.m_count) {
                                listBearing.Insert(fBearing, j);
                                listBearingDistSq.Insert(fDistSq, j);
                                listAround.Insert(oid, j);
                            } else {
                                listBearing.Add(fBearing);
                                listBearingDistSq.Add(fDistSq);
                                listAround.Add(oid);
                            }

                            float fFacing = m_orientation.x * vDelta.x
                                          + m_orientation.y * vDelta.y
                                          + m_orientation.z * vDelta.z;
                            if (fFacing >= 0.0f) {
                                CSWSObject* pTo = pObject->AsSWSObject();
                                vDelta.x = pTo->m_position.x - vFrom.x;
                                vDelta.y = pTo->m_position.y - vFrom.y;
                                vDelta.z = pTo->m_position.z - vFrom.z;
                                vDelta.Normalize();
                                float fCone = m_orientation.x * vDelta.x
                                            + m_orientation.y * vDelta.y
                                            + m_orientation.z * vDelta.z;
                                if (fCone > 0.866) {
                                    int m;
                                    for (m = 0; m < listFrontDistSq.m_count; m++) {
                                        if (fDistSq < listFrontDistSq.m_pArray[m])
                                            break;
                                    }
                                    if (m < listFrontDistSq.m_count) {
                                        listFrontDistSq.Insert(fDistSq, m);
                                        listInFront.Insert(oid, m);
                                    } else {
                                        listFrontDistSq.Add(fDistSq);
                                        listInFront.Add(oid);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

CResRef CSWSCreature::GetPortraitBasedOnAlignment()
{
    CExoString sPortrait;
    C2DA* pPortraits = g_pRules->m_p2DArrays->m_pPortrait;
    if (pPortraits == NULL)
        return GetPortrait();
    sPortrait = "";
    short nAlignment = m_pStats->m_nAlignment;
    int bFound;
    if (nAlignment <= 10) {
        bFound = pPortraits->GetCExoStringEntry(m_portraitId, CExoString("BaseResRefVVVE"),
                                               &sPortrait);
        if (!bFound)
            return GetPortrait();
    } else if (nAlignment <= 20) {
        bFound = pPortraits->GetCExoStringEntry(m_portraitId, CExoString("BaseResRefVVE"),
                                               &sPortrait);
        if (!bFound)
            return GetPortrait();
    } else if (nAlignment <= 30) {
        bFound = pPortraits->GetCExoStringEntry(m_portraitId, CExoString("BaseResRefVE"),
                                               &sPortrait);
        if (!bFound)
            return GetPortrait();
    } else if (nAlignment <= 40) {
        bFound = pPortraits->GetCExoStringEntry(m_portraitId, CExoString("BaseResRefE"),
                                               &sPortrait);
        if (!bFound)
            return GetPortrait();
    } else {
        return GetPortrait();
    }
    return CResRef(sPortrait);
}

void CSWSCreature::UpdateActionQueue()
{
    for (int i = 0; i < NUM_ACTION_QUEUE_ENTRIES; i++)
    {
        m_actionQueue[i].m_nGroupId = 0xffff;
        m_actionQueue[i].m_nActionId = AIACTION_NONE;
        m_actionQueue[i].m_nAltActionId = AIACTION_NONE;
        m_actionQueue[i].m_vTarget.x = 0.0f;
        m_actionQueue[i].m_vTarget.y = 0.0f;
        m_actionQueue[i].m_vTarget.z = 0.0f;
        m_actionQueue[i].m_targetId = OBJECT_INVALID;
        m_actionQueue[i].m_nSpellId = -1;
    }

    int nEntry;
    int nSkipped = 0;
    for (nEntry = 0; nEntry < NUM_ACTION_QUEUE_ENTRIES; nEntry++)
    {
        CSWSObjectActionNode* pNode;
        unsigned short nActionId;
        for (;;)
        {
            CExoLinkedListNode* pPos = GetPositionByGroupIndex(nEntry + nSkipped);
            if (pPos == 0)
                goto done;
            pNode = static_cast<CSWSObjectActionNode*>(m_actionList.GetAtPos(pPos));
            if (pNode == 0)
                continue;
            nActionId = GetActionByGroupId(pNode->m_nGroupId, &pNode);
            if (nActionId != AIACTION_NONE)
                break;
            nSkipped++;
        }
        if (pNode == 0)
            goto done;

        switch (nActionId)
        {
        case AIACTION_MOVE_TO_POINT:
            if ((pNode->m_args.m_moveToParam & 0xc0) == 0)
            {
                unsigned long long oidArea = pNode->m_args.m_moveToAreaId;
                float fX = pNode->m_args.m_moveToX;
                float fY = pNode->m_args.m_moveToY;
                float fZ = pNode->m_args.m_moveToZ;
                m_actionQueue[nEntry].m_vTarget.x = fX;
                m_actionQueue[nEntry].m_vTarget.y = fY;
                m_actionQueue[nEntry].m_vTarget.z = fZ;
                m_actionQueue[nEntry].m_targetId = oidArea;
            }
            break;
        case AIACTION_DROP_ITEM:
        {
            float fX = pNode->m_args.m_dropPointX;
            float fY = pNode->m_args.m_dropPointY;
            float fZ = pNode->m_args.m_dropPointZ;
            m_actionQueue[nEntry].m_vTarget.x = fX;
            m_actionQueue[nEntry].m_vTarget.y = fY;
            m_actionQueue[nEntry].m_vTarget.z = fZ;
            break;
        }
        case AIACTION_CAST_SPELL:
        {
            unsigned int nSpell = pNode->m_args.m_spellId;
            unsigned long long oidTarget = pNode->m_args.m_spellTargetId;
            float fX = pNode->m_args.m_aqSpellPointX;
            float fY = pNode->m_args.m_aqSpellPointY;
            float fZ = pNode->m_args.m_aqSpellPointZ;
            m_actionQueue[nEntry].m_nSpellId = nSpell;
            m_actionQueue[nEntry].m_targetId = oidTarget;
            m_actionQueue[nEntry].m_vTarget.x = fX;
            m_actionQueue[nEntry].m_vTarget.y = fY;
            m_actionQueue[nEntry].m_vTarget.z = fZ;
            break;
        }
        case AIACTION_SET_MINE:
        {
            unsigned long long oidTarget = pNode->m_args.m_aqObjectId;
            float fX = pNode->m_args.m_aqObjPointX;
            float fY = pNode->m_args.m_aqObjPointY;
            float fZ = pNode->m_args.m_aqObjPointZ;
            m_actionQueue[nEntry].m_targetId = oidTarget;
            m_actionQueue[nEntry].m_vTarget.x = fX;
            m_actionQueue[nEntry].m_vTarget.y = fY;
            m_actionQueue[nEntry].m_vTarget.z = fZ;
            break;
        }
        case AIACTION_ITEM_CAST_SPELL:
        {
            unsigned long long oidItem = pNode->m_args.m_itemId;
            int nProperty = pNode->m_args.m_aqItemPropertyIndex;
            unsigned long long oidTarget = pNode->m_args.m_itemCastTargetId;
            float fTargetX = pNode->m_args.m_aqItemPointX;
            float fTargetY = pNode->m_args.m_aqItemPointY;
            float fTargetZ = pNode->m_args.m_aqItemPointZ;
            CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
            if (pItem == 0)
                continue;
            CSWItemProperty* pProperty = pItem->GetActiveProperty(nProperty);
            if (pProperty == 0)
                continue;
            m_actionQueue[nEntry].m_targetId = oidTarget;
            m_actionQueue[nEntry].m_vTarget.x = fTargetX;
            m_actionQueue[nEntry].m_vTarget.y = fTargetY;
            m_actionQueue[nEntry].m_vTarget.z = fTargetZ;
            m_actionQueue[nEntry].m_nSpellId = pProperty->m_subType;
            break;
        }
        case AIACTION_PICK_UP_ITEM:    case AIACTION_PHYSICAL_ATTACKS:  case AIACTION_OPEN_DOOR: case AIACTION_CLOSE_DOOR: case AIACTION_DIALOG_OBJECT: case AIACTION_DISARM_MINE: case AIACTION_RECOVER_MINE:
        case AIACTION_FLAG_MINE: case AIACTION_EXAMINE_MINE: case AIACTION_UNLOCK_OBJECT: case AIACTION_LOCK_OBJECT: case AIACTION_USE_OBJECT: case AIACTION_ANIMAL_EMPATHY: case AIACTION_TAUNT:
        case AIACTION_COUNTER_SPELL: case AIACTION_PICK_POCKET: case AIACTION_FORCE_FOLLOW_OBJECT: case AIACTION_HEAL: case AIACTION_BARK_STRING:
#if !KOTOR1  // K1: no action
        case AIACTION_DOOR_SABER:
#endif
        {
            unsigned long long oidTarget = pNode->m_args.m_aqTargetId;
            m_actionQueue[nEntry].m_targetId = oidTarget;
            break;
        }
        default:
            break;
        }

        m_actionQueue[nEntry].m_nGroupId = pNode->m_nGroupId;
        m_actionQueue[nEntry].m_nActionId = GetAQActionIDByID(nActionId, 0);
    }

done:
    for (; nEntry < NUM_ACTION_QUEUE_ENTRIES; nEntry++)
    {
        m_actionQueue[nEntry].m_nGroupId = 0xffff;
        m_actionQueue[nEntry].m_nActionId = AIACTION_NONE;
        m_actionQueue[nEntry].m_nAltActionId = AIACTION_NONE;
        m_actionQueue[nEntry].m_vTarget.x = 0.0f;
        m_actionQueue[nEntry].m_vTarget.y = 0.0f;
        m_actionQueue[nEntry].m_vTarget.z = 0.0f;
        m_actionQueue[nEntry].m_targetId = OBJECT_INVALID;
        m_actionQueue[nEntry].m_nSpellId = -1;
    }
}

int CSWSCreature::LoadFromTemplate(const CResRef& resRef, int flag)
{
    CResGFF* pGff = new CResGFF(RESTYPE_UTC, const_cast<char*>("UTC "), resRef);
    if (!pGff->m_nValid)
    {
        delete pGff;
        CExoString sMessage;
        sMessage.Format(const_cast<char*>("Creature template '%s' doesn't exist.\n"), resRef.GetResRefStr());
        pGff = new CResGFF(RESTYPE_UTC, const_cast<char*>("UTC "), CResRef("NW_BADGER"));
        if (!pGff->m_nValid)
        {
            delete pGff;
            return 0;
        }
    }
    CResStruct topStruct;
    pGff->GetTopLevelStruct(&topStruct);
    if (m_pStats->ReadStatsFromGff(pGff, &topStruct, &m_appearance))
    {
        delete pGff;
        return 0;
    }
    ReadScriptsFromGff(pGff, &topStruct);
    ReadItemsFromGff(pGff, &topStruct, 0, 1, flag);
    m_pStats->ReadSpellsFromGff(pGff, &topStruct);

    int bSuccess;
    m_bDisguised = pGff->ReadFieldBYTE(&topStruct, const_cast<char*>("PM_IsDisguised"), bSuccess, 0);
    if (m_bDisguised)
        m_nDisguiseAppearance = pGff->ReadFieldWORD(&topStruct, const_cast<char*>("PM_Appearance"), bSuccess, 0);
    SetActivity(ACTIVITY_STEALTH, pGff->ReadFieldBYTE(&topStruct, const_cast<char*>("StealthMode"), bSuccess, 0), 0);
    LoadObjectState(pGff, &topStruct);

    Vector vPosition;
    vPosition.x = pGff->ReadFieldFLOAT(&topStruct, const_cast<char*>("XPosition"), bSuccess, 0.0f);
    vPosition.y = pGff->ReadFieldFLOAT(&topStruct, const_cast<char*>("YPosition"), bSuccess, 0.0f);
    vPosition.z = pGff->ReadFieldFLOAT(&topStruct, const_cast<char*>("ZPosition"), bSuccess, 0.0f);
    Vector vOrientation;
    vOrientation.x = pGff->ReadFieldFLOAT(&topStruct, const_cast<char*>("XOrientation"), bSuccess, 0.0f);
    vOrientation.y = pGff->ReadFieldFLOAT(&topStruct, const_cast<char*>("YOrientation"), bSuccess, 0.0f);
    vOrientation.z = pGff->ReadFieldFLOAT(&topStruct, const_cast<char*>("ZOrientation"), bSuccess, 0.0f);
    m_nJoiningXP = pGff->ReadFieldINT(&topStruct, const_cast<char*>("JoiningXP"), bSuccess, 0);

    SetPosition(vPosition, 1, 1, 0);
    if (vOrientation.x != 0.0f || vOrientation.y != 0.0f || vOrientation.z != 0.0f)
        SetOrientation(vOrientation);
    PostProcess();
    delete pGff;
    return 1;
}

void CSWSCreature::ReadScriptsFromGff(CResGFF* pGff, CResStruct* pStruct)
{
    CExoString sScript;
    int bSuccess;

    pGff->ReadFieldCResRef(pStruct, const_cast<char*>("ScriptHeartbeat"), bSuccess,
                           CResRef(m_scripts[CREATURE_SCRIPT_HEARTBEAT])).CopyToString(sScript);
    m_scripts[CREATURE_SCRIPT_HEARTBEAT] = sScript;

    pGff->ReadFieldCResRef(pStruct, const_cast<char*>("ScriptOnNotice"), bSuccess,
                           CResRef(m_scripts[CREATURE_SCRIPT_ONNOTICE])).CopyToString(sScript);
    m_scripts[CREATURE_SCRIPT_ONNOTICE] = sScript;

    pGff->ReadFieldCResRef(pStruct, const_cast<char*>("ScriptSpellAt"), bSuccess,
                           CResRef(m_scripts[CREATURE_SCRIPT_SPELLAT])).CopyToString(sScript);
    m_scripts[CREATURE_SCRIPT_SPELLAT] = sScript;

    pGff->ReadFieldCResRef(pStruct, const_cast<char*>("ScriptAttacked"), bSuccess,
                           CResRef(m_scripts[CREATURE_SCRIPT_ATTACKED])).CopyToString(sScript);
    m_scripts[CREATURE_SCRIPT_ATTACKED] = sScript;

    pGff->ReadFieldCResRef(pStruct, const_cast<char*>("ScriptDamaged"), bSuccess,
                           CResRef(m_scripts[CREATURE_SCRIPT_DAMAGED])).CopyToString(sScript);
    m_scripts[CREATURE_SCRIPT_DAMAGED] = sScript;

    pGff->ReadFieldCResRef(pStruct, const_cast<char*>("ScriptDisturbed"), bSuccess,
                           CResRef(m_scripts[CREATURE_SCRIPT_DISTURBED])).CopyToString(sScript);
    m_scripts[CREATURE_SCRIPT_DISTURBED] = sScript;

    pGff->ReadFieldCResRef(pStruct, const_cast<char*>("ScriptEndRound"), bSuccess,
                           CResRef(m_scripts[CREATURE_SCRIPT_ENDROUND])).CopyToString(sScript);
    m_scripts[CREATURE_SCRIPT_ENDROUND] = sScript;

    pGff->ReadFieldCResRef(pStruct, const_cast<char*>("ScriptDialogue"), bSuccess,
                           CResRef(m_scripts[CREATURE_SCRIPT_DIALOGUE])).CopyToString(sScript);
    m_scripts[CREATURE_SCRIPT_DIALOGUE] = sScript;

    pGff->ReadFieldCResRef(pStruct, const_cast<char*>("ScriptSpawn"), bSuccess,
                           CResRef(m_scripts[CREATURE_SCRIPT_SPAWN])).CopyToString(sScript);
    m_scripts[CREATURE_SCRIPT_SPAWN] = sScript;

    pGff->ReadFieldCResRef(pStruct, const_cast<char*>("ScriptRested"), bSuccess,
                           CResRef(m_scripts[CREATURE_SCRIPT_RESTED])).CopyToString(sScript);
    m_scripts[CREATURE_SCRIPT_RESTED] = sScript;

    pGff->ReadFieldCResRef(pStruct, const_cast<char*>("ScriptDeath"), bSuccess,
                           CResRef(m_scripts[CREATURE_SCRIPT_DEATH])).CopyToString(sScript);
    m_scripts[CREATURE_SCRIPT_DEATH] = sScript;

    pGff->ReadFieldCResRef(pStruct, const_cast<char*>("ScriptUserDefine"), bSuccess,
                           CResRef(m_scripts[CREATURE_SCRIPT_USERDEFINE])).CopyToString(sScript);
    m_scripts[CREATURE_SCRIPT_USERDEFINE] = sScript;

    pGff->ReadFieldCResRef(pStruct, const_cast<char*>("ScriptOnBlocked"), bSuccess,
                           CResRef(m_scripts[CREATURE_SCRIPT_ONBLOCKED])).CopyToString(sScript);
    m_scripts[CREATURE_SCRIPT_ONBLOCKED] = sScript;

    pGff->ReadFieldCResRef(pStruct, const_cast<char*>("ScriptEndDialogue"), bSuccess,
                           CResRef(m_scripts[CREATURE_SCRIPT_ENDDIALOGUE])).CopyToString(sScript);
    m_scripts[CREATURE_SCRIPT_ENDDIALOGUE] = sScript;
}

void CSWSCreature::ReadItemsFromGff(CResGFF* pGff, CResStruct* pStruct, int bUseObjectIds,
                                    int nPossessorFlag, int bFromTemplate)
{
    CResList list;
    CResStruct elem;
    int bSuccess;
    CSWSItem* pItem;
    unsigned int nSlot;

    if (m_nCreatureSize == CREATURE_SIZE_INVALID)
    {
        int nSizeCategory;
        g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
            m_appearance.m_headId, CTwoDimArrays::COL2DA_APPEARANCETABLE_SIZECATEGORY,
            &nSizeCategory);
        m_nCreatureSize = nSizeCategory;
    }

    CSWPartyTable* pPartyTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
    int nRepository = (pPartyTable != 0 && pPartyTable->m_bPartyCreated != 0);
    m_bForceEquipClothing = 0;

    unsigned long long oidItem = OBJECT_INVALID;

    if (pGff->GetList(&list, pStruct, const_cast<char*>("Equip_ItemList")) == 1)
    {
        for (unsigned int i = 0; i < pGff->GetListCount(&list); i++)
        {
            pGff->GetListElement(&elem, &list, i);
            if (bUseObjectIds)
                oidItem = pGff->ReadFieldDWORD(&elem, const_cast<char*>("ObjectId"), bSuccess, OBJECT_INVALID);

            if (g_pAppManager->m_pServerExoApp->GetGameObject(oidItem) != 0)
            {
                pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
                if (pItem->m_possessorId == m_objectId)
                {
                    nSlot = pGff->GetElementType(&elem);
                    pItem->SetPossessor(m_objectId, nPossessorFlag, 0, OBJECT_INVALID);
                    if (CanEquipItem(pItem, &nSlot, 1, 1, 1))
                        EquipItem(nSlot, pItem, 1, 1, 0);
                    else
                        GetItemRepository(1)->AddItem(&pItem, 1, 0, 1);
                }
            }
            else
            {
                pItem = new CSWSItem(oidItem);
                if (bFromTemplate)
                {
                    CResRef itemRes = pGff->ReadFieldCResRef(&elem, const_cast<char*>("EquippedRes"), bSuccess,
                                                             CResRef(""));
                    if (bSuccess && pItem->LoadFromTemplate(itemRes))
                    {
                        pItem->SetDropable(pGff->ReadFieldBYTE(&elem, const_cast<char*>("Dropable"),
                                           bSuccess, 0));
                    }
                    else
                    {
                        delete pItem;
                        pItem = 0;
                    }
                }
                else if (!pItem->LoadItem(pGff, &elem))
                {
                    delete pItem;
                    pItem = 0;
                }

                if (pItem != 0)
                {
                    nSlot = pGff->GetElementType(&elem);
                    pItem->SetPossessor(m_objectId, nPossessorFlag, 0, OBJECT_INVALID);
                    if (CanEquipItem(pItem, &nSlot, 1, 1, 1))
                    {
                        EquipItem(nSlot, pItem, 1, 1, 0);
                    }
                    else
                    {
                        if (nSlot == 2)
                            m_bForceEquipClothing = 1;
                        GetItemRepository(nRepository)->AddItem(&pItem, 1, 0, 0);
                    }
                }
            }
        }
    }

    if (pGff->GetList(&list, pStruct, const_cast<char*>("ItemList")) == 1)
    {
        for (unsigned int i = 0; i < pGff->GetListCount(&list); i++)
        {
            pGff->GetListElement(&elem, &list, i);
            if (bUseObjectIds)
                oidItem = pGff->ReadFieldDWORD(&elem, const_cast<char*>("ObjectId"), bSuccess, OBJECT_INVALID);

            if (g_pAppManager->m_pServerExoApp->GetGameObject(oidItem) != 0)
            {
                pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
                if (pItem->m_possessorId == m_objectId)
                    GetItemRepository(1)->AddItem(&pItem, 0, 0, 1);
            }
            else
            {
                pItem = new CSWSItem(oidItem);
                if (bFromTemplate)
                {
                    CResRef itemRes = pGff->ReadFieldCResRef(&elem, const_cast<char*>("InventoryRes"), bSuccess,
                                                             CResRef(""));
                    if (bSuccess && pItem->LoadFromTemplate(itemRes))
                    {
                        pItem->SetDropable(pGff->ReadFieldBYTE(&elem, const_cast<char*>("Dropable"),
                                           bSuccess, 0));
                    }
                    else
                    {
                        delete pItem;
                        pItem = 0;
                    }
                }
                else if (!pItem->LoadItem(pGff, &elem))
                {
                    delete pItem;
                    pItem = 0;
                }

                // AddItem takes CSWSItem** and nulls it out when the item merges into an existing
                // stack, so the possessor fix-up re-tests pItem after the call.
                if (pItem != 0 && GetItemRepository(1)->AddItem(&pItem, 0, 0, 1) != 0 && pItem != 0)
                    pItem->SetPossessor(m_objectId, nPossessorFlag, 0, OBJECT_INVALID);
            }
        }
    }
}

void CSWSCreature::LoadJournal(CResGFF* pGff, CResStruct* pStruct)
{
    int state;
    unsigned int date;
    unsigned int time;
    CSWSJournal* pJournal = g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyJournal();
    if (pJournal != 0 && pJournal->m_entries.m_count == 0)
    {
        CResList entryList;
        if (pGff->GetList(&entryList, pStruct, const_cast<char*>("JNL_Entries")))
        {
            int bSuccess;
            g_nJournalSortType = pGff->ReadFieldINT(pStruct, const_cast<char*>("JNL_SortOrder"), bSuccess, g_nJournalSortType);

            CExoString plotId;
            int count = pGff->GetListCount(&entryList);
            for (int i = 0; i < count; i++)
            {
                CResStruct elemStruct;
                pGff->GetListElement(&elemStruct, &entryList, i);

                plotId = pGff->ReadFieldCExoString(&elemStruct, const_cast<char*>("JNL_PlotID"), bSuccess, CExoString(""));

                state = pGff->ReadFieldINT(&elemStruct, const_cast<char*>("JNL_State"), bSuccess, 0);
                g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyJournal()->SetState(plotId, state, 1);

                date = pGff->ReadFieldDWORD(&elemStruct, const_cast<char*>("JNL_Date"), bSuccess, 0);
                g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyJournal()->SetDate(plotId, date, 1);

                time = pGff->ReadFieldDWORD(&elemStruct, const_cast<char*>("JNL_Time"), bSuccess, 0);
                g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPartyJournal()->SetTime(plotId, time, 1);
            }
        }
    }
}

void CSWSCreature::UpdateAppearanceDependantInfo(int bUpdateMovementRate)
{
    int nValue;
    g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(m_appearance.m_headId,
                                                      CExoString("haslegs"), &nValue);
    m_bHasLegs = nValue;

    g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(m_appearance.m_headId,
                                                      CExoString("hasarms"), &nValue);
    m_bHasArms = nValue;

    int nDisableInjuredAnim;
    g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
        m_appearance.m_headId, CTwoDimArrays::COL2DA_APPEARANCETABLE_DISABLEINJUREDANIM,
        &nDisableInjuredAnim);
    m_bDisableInjuredAnim = nDisableInjuredAnim;

    int nSizeCategory;
    g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
        m_appearance.m_headId, CTwoDimArrays::COL2DA_APPEARANCETABLE_SIZECATEGORY, &nSizeCategory);
    m_nCreatureSize = nSizeCategory;

    CExoString sModelType;
    g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
        m_appearance.m_headId, CTwoDimArrays::COL2DA_APPEARANCETABLE_MODELTYPE, &sModelType);
    m_sModelType = sModelType;

    float fValue;
    int bFound;
    bFound = g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
        m_appearance.m_headId, CExoString("weaponscale"), &fValue);
    if (bFound == 1)
        m_fWeaponScale = fValue;
    else
        m_fWeaponScale = 1.0f;

    if (bUpdateMovementRate != 0)
        m_pStats->SetMovementRate(7);

    g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(m_appearance.m_headId,
                                                        CExoString("PREFATCKDIST"), &fValue);
    m_fPrefAttackDist = fValue;
    m_bUpdateCombatInformation = 1;
}

void CSWSCreature::UpdateAttributesOnEffect(CGameEffect* pEffect)
{
    if (pEffect == 0)
        return;
    if (pEffect->m_subType != CSWSEffectListHandler::EFFECT_ABILITY_INCREASE &&
        pEffect->m_subType != CSWSEffectListHandler::EFFECT_ABILITY_DECREASE)
        return;

    switch (pEffect->GetInteger(0))
    {
        case ABILITY_STRENGTH:
        {
            CSWSCreatureStats* pStats = m_pStats;
            pStats->m_nSTRMod = static_cast<unsigned char>(pStats->CalcStatModifier(pStats->GetSTRStat()));
            m_bUpdateCombatInformation = 1;
            break;
        }
        case ABILITY_DEXTERITY:
        {
            CSWSCreatureStats* pStats = m_pStats;
            pStats->m_nDEXMod = static_cast<signed char>(pStats->CalcStatModifier(pStats->GetDEXStat()));
            m_bUpdateCombatInformation = 1;
            break;
        }
        case ABILITY_CONSTITUTION:
        {
#if KOTOR1  // K1: the down test is GetDeadTemp() (and a discarded GetIsPCDying())
            GetIsPCDying();
            int bWasDead = GetDead();
            int bWasDown = GetDeadTemp();
#else
            int bWasDead = GetDead();
            int bWasDown = 0;
            if (m_bInParty != 0)
                bWasDown = (GetCurrentHitPoints(0) <= 0);
#endif

            short nOldMaxHitPoints = GetMaxHitPoints(1);
            CSWSCreatureStats* pStats = m_pStats;
            pStats->m_nCONMod = static_cast<signed char>(pStats->CalcStatModifier(pStats->GetCONStat()));
            SetCurrentHitPoints(GetCurrentHitPoints(1) + (GetMaxHitPoints(1) - nOldMaxHitPoints));

            // Either the CON loss just killed it outright, or it was already down and the
            // re-based pool left a party member at or below zero.
            if ((bWasDead == 0 && GetDead() != 0) ||
#if KOTOR1
                (bWasDown == 0 && GetDeadTemp() != 0))
#else
                (bWasDown == 0 && m_bInParty != 0 && GetCurrentHitPoints(0) <= 0))
#endif
            {
                CGameEffect* pDeathEffect = new CGameEffect(1);
                pDeathEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DEATH);
                pDeathEffect->m_durationTypeBits =
                    CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                    (pDeathEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
                pDeathEffect->SetCreator(pEffect->m_creatorId);
                pDeathEffect->SetInteger(1, 1);
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, pDeathEffect->m_creatorId, m_objectId,
                    CServerAIMaster::EVENT_APPLY_EFFECT, pDeathEffect);
            }
            break;
        }
        case ABILITY_INTELLIGENCE:
        {
            CSWSCreatureStats* pStats = m_pStats;
            pStats->m_nINTMod = static_cast<unsigned char>(pStats->CalcStatModifier(pStats->GetINTStat()));
            m_pStats->AdjustSpellUsesPerDay();
            break;
        }
        case ABILITY_WISDOM:
        {
            short nOldMaxForcePoints = GetMaxForcePoints();
            CSWSCreatureStats* pStats = m_pStats;
            pStats->m_nWISMod = static_cast<signed char>(pStats->CalcStatModifier(pStats->GetWISStat()));

            CSWSCreatureStats* pStats2 = m_pStats;
            pStats2->m_nCurrentForcePoints =
                static_cast<unsigned short>(pStats2->m_nCurrentForcePoints + (GetMaxForcePoints() - nOldMaxForcePoints));
            m_pStats->AdjustSpellUsesPerDay();
            break;
        }
        case ABILITY_CHARISMA:
        {
            short nOldMaxForcePoints = GetMaxForcePoints();
            CSWSCreatureStats* pStats = m_pStats;
            pStats->m_nCHAMod = static_cast<unsigned char>(pStats->CalcStatModifier(pStats->GetCHAStat()));

            CSWSCreatureStats* pStats2 = m_pStats;
            pStats2->m_nCurrentForcePoints =
                static_cast<unsigned short>(pStats2->m_nCurrentForcePoints + (GetMaxForcePoints() - nOldMaxForcePoints));
            m_pStats->AdjustSpellUsesPerDay();
            break;
        }
    }
}

void CSWSCreature::ComputeAIStateOnAction(int nActionId, unsigned long long objectId)
{
    switch (nActionId)
    {
        case AIACTION_MOVE_TO_POINT:
        case AIACTION_DRIVE:
            if (m_nAIStateAction == 0)
            {
                m_nAIStateAction = AI_STATE_ACTION_MOVING;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_CAST_SPELL:
        case AIACTION_ITEM_CAST_SPELL:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_CASTING;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_PICK_UP_ITEM:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_PICKING_UP_ITEM;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_EQUIP_ITEM:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_EQUIPPING;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_DROP_ITEM:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_DROPPING_ITEM;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_UNEQUIP_ITEM:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_UNEQUIPPING;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_PHYSICAL_ATTACKS:
            if (m_nAIStateAction <= 1)
            {
                m_nAIStateAction = AI_STATE_ACTION_ATTACKING;
                m_oidAIStateTarget = static_cast<int>(objectId);
            }
            break;
        case AIACTION_OPEN_DOOR:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_OPENING_DOOR;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_CLOSE_DOOR:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_CLOSING_DOOR;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_DISARM_MINE:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_DISARMING_MINE;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_RECOVER_MINE:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_RECOVERING_MINE;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_FLAG_MINE:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_FLAGGING_MINE;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_EXAMINE_MINE:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_EXAMINING_MINE;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_SET_MINE:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_SETTING_MINE;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_GIVE_ITEM:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_GIVING_ITEM;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_TAKE_ITEM:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_TAKING_ITEM;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_UNLOCK_OBJECT:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_UNLOCKING;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_LOCK_OBJECT:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_LOCKING;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_USE_OBJECT:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_USING_OBJECT;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_ANIMAL_EMPATHY:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_ANIMAL_EMPATHY;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_REST:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_RESTING;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_PICK_POCKET:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_PICKING_POCKET;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
        case AIACTION_HEAL:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_HEALING;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
#if !KOTOR1  // K1: no additional action ids
        case AIACTION_DOOR_SABER:
            if (m_nAIStateAction <= 9)
            {
                m_nAIStateAction = AI_STATE_ACTION_CUTTING_DOOR;
                m_oidAIStateTarget = OBJECT_INVALID;
            }
            break;
#endif
    }

    if (nActionId == AIACTION_COUNTER_SPELL && m_nAIStateAction <= 1)
    {
        m_nAIStateAction = AI_STATE_ACTION_ATTACKING;
        m_oidAIStateTarget = OBJECT_INVALID;
    }
#if !KOTOR1
    if (nActionId == AIACTION_STANCE_TOTAL_DEFENSE && m_nAIStateAction <= 1)
    {
        m_nAIStateAction = AI_STATE_ACTION_IN_STANCE;
        m_oidAIStateTarget = OBJECT_INVALID;
    }
    if (nActionId == AIACTION_STANCE_MEDITATIVE && m_nAIStateAction == 0)
    {
        m_nAIStateAction = AI_STATE_ACTION_IN_STANCE;
        m_oidAIStateTarget = OBJECT_INVALID;
    }
#endif
}

void CSWSCreature::PostProcess()
{
    CServerAIMaster* pAIMaster = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
    if (m_pStats->m_bIsPC == 1)
    {
        pAIMaster->SetAILevel(this, 4);

        // The 20 equipment slots are a bitmask, one bit per slot.
        unsigned int nSlot = 1;
#if KOTOR1  // K1: 18 equipment slots
        for (int i = 0; i < 18; i++)
#else
        for (int i = 0; i < 20; i++)
#endif
        {
            CSWSItem* pItem = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(nSlot));
            if (pItem != 0)
                pItem->m_flags |= 8;
            nSlot = nSlot + nSlot;
        }

        unsigned int nItem = 0;
        while (nItem < GetItemRepository(1)->ItemListGetCount())
        {
            GetItemRepository(1)->ItemListGetItem(nItem)->m_flags |= 8;
            nItem++;
        }
    }

    g_pAppManager->m_pServerExoApp->GetModule()->AddObjectToLookupTable(m_sTag, m_objectId);
    m_bPlayerCharacter = m_pStats->m_bIsPC;

    CFactionManager* pFactionManager = g_pAppManager->m_pServerExoApp->GetFactionManager();
    if (m_bPlayerCharacter == 1)
    {
        // Party members always join the player faction directly.
        if (pFactionManager != 0 && pFactionManager->m_pFactionPlayer != 0)
            pFactionManager->m_pFactionPlayer->AddMember(m_objectId, 0);
        m_pStats->m_nJoinedFactionId = 0;
    }
    else
    {
        int nFactionId = m_pStats->m_factionId;
        m_pStats->m_factionId = -1;
        CSWSFaction* pFaction = pFactionManager->GetFaction(nFactionId);
        if (pFaction != 0)
        {
            pFaction->AddMember(m_objectId, 0);
            m_pStats->m_nJoinedFactionId = nFactionId;
        }
        else
        {
            // The warning is formatted but never shipped anywhere: the log call it fed is gone
            // from the release build, and only the CExoString construct/Format/destroy survives.
            CExoString sWarning;
            sWarning.Format(const_cast<char*>("Cannot set creature %s to faction %d because faction does not exist! "
                                               "Setting to Hostile1."),
                            m_sTag.CStr(), nFactionId);
            CSWSFaction* pFactionOne = pFactionManager->GetFaction(1);
            pFactionOne->AddMember(m_objectId, 0);
            m_pStats->m_nJoinedFactionId = 1;
        }
    }

    UpdatePersonalSpace();

    m_bApplyingResistanceEffect = 1;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_RACIAL_TYPE);
    pEffect->m_durationTypeBits =
        CGameEffect::DURATION_TYPE_INNATE | (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(m_objectId);
    pEffect->SetInteger(0, m_pStats->m_nRace);
    ApplyEffect(pEffect, 0, 0);
    m_bApplyingResistanceEffect = 0;

    if (GetCurrentHitPoints(0) <= 0)
    {
        SetAnimation(SERVER_ANIMATION_DEAD_2);
        m_bCommandable = 0;
        if (m_bPlayerCharacter == 0 || GetCurrentHitPoints(0) <= -10)
            m_nAIState = 0;
    }
    if (m_bPlayerCharacter == 1)
        m_bRaiseable = 1;

    UpdateAppearanceDependantInfo(0);
}

void CSWSCreature::UpdateEffectPtrs()
{
    int nPrev = CSWSEffectListHandler::MAX_EFFECTS;
    for (int i = 0; i < m_effects.m_count; i++)
    {
        int nLast = nPrev;
        nPrev = m_effects.m_pArray[i]->m_subType;
        unsigned short nSubType = static_cast<unsigned short>(nPrev);
        if (nLast == nPrev)  // still inside the previous effect's run: keep its first index
            continue;
        if (nSubType == CSWSEffectListHandler::EFFECT_AC_INCREASE)
            m_pStats->m_iFirstACIncrease = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_ATTACK_INCREASE)
            m_pStats->m_iFirstAttackIncrease = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_DAMAGE_REDUCTION)
            m_pStats->m_iFirstDamageReduction = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_DAMAGE_RESISTANCE)
            m_pStats->m_iFirstDamageResistance = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_DAMAGE_INCREASE)
            m_pStats->m_iFirstDamageIncrease = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_SET_AI_STATE)
            m_pStats->m_iFirstSetAIState = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_EFFECT_ICON)
            m_pStats->m_iFirstEffectIcon = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_INVISIBILITY)
            m_pStats->m_iFirstInvisibility = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_SANCTUARY)
            m_pStats->m_iFirstSanctuary = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_ABILITY_INCREASE)
            m_pStats->m_iFirstAbilityIncrease = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_FORCE_RESISTANCE_INCREASE)
            m_pStats->m_iFirstForceResistanceIncrease = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_SEEINVISIBLE)
            m_pStats->m_iFirstSeeInvisible = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_ULTRAVISION)
            m_pStats->m_iFirstUltravision = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_MISS_CHANCE)
            m_pStats->m_iFirstMissChance = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_CONCEALMENT)
            m_pStats->m_iFirstConcealment = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_BLINDNESS)
            m_pStats->m_iFirstBlindness = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_SETWALKANIMATION)
            m_pStats->m_iFirstSetWalkAnimation = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_SPELL_LEVEL_ABSORPTION)
            m_pStats->m_iFirstSpellLevelAbsorption = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_SPELL_IMMUNITY)
            m_pStats->m_iFirstSpellImmunity = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_DAMAGE_SHIELD)
            m_pStats->m_iFirstDamageShield = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_SKILL_INCREASE)
            m_pStats->m_iFirstSkillIncrease = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_SAVING_THROW_INCREASE)
            m_pStats->m_iFirstSavingThrowIncrease = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_HASTE)
            m_pStats->m_iFirstHaste = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_IMMUNITY)
            m_pStats->m_iFirstImmunity = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_LIMIT_MOVEMENT_SPEED)
            m_pStats->m_iFirstLimitMovementSpeed = static_cast<unsigned short>(i);
        else if (nSubType == CSWSEffectListHandler::EFFECT_MOVEMENT_SPEED_DECREASE)
            m_pStats->m_iFirstMovementSpeedDecrease = static_cast<unsigned short>(i);
    }
}

int CSWSCreature::ClearAction(CSWSObjectActionNode* pNode, int bIsCurrentAction)
{
    if (pNode->m_bInterruptable == 0)
        return 0;

    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    // This is the internal accessor, not CServerExoApp's same-named wrapper; both exist and must
    // stay distinct, since StopGuiTimingBar below calls the wrapper.
    CSWSPlayer* pPlayer =
        g_pAppManager->m_pServerExoApp->m_pInternal->GetClientObjectByObjectId(m_objectId);
    if (pMessage != 0 && pPlayer != 0) {
        switch (pNode->m_nActionId) {
        case AIACTION_PICK_UP_ITEM: {
            unsigned long long oidItem = pNode->m_params[0].m_n;
            pMessage->SendServerToPlayerInventory_PickupCancel(pPlayer->m_nPlayerId, oidItem);
            break;
        }
        case AIACTION_EQUIP_ITEM: {
            unsigned long long oidItem = pNode->m_params[0].m_n;
            unsigned int nSlot = pNode->m_args.m_equipSlot;
            pMessage->SendServerToPlayerInventory_EquipCancel(pPlayer->m_nPlayerId, oidItem,
                                                              nSlot);
            break;
        }
        case AIACTION_DROP_ITEM: {
            unsigned long long oidItem = pNode->m_params[0].m_n;
            pMessage->SendServerToPlayerInventory_DropCancel(pPlayer->m_nPlayerId, oidItem);
            break;
        }
        case AIACTION_UNEQUIP_ITEM: {
            unsigned long long oidItem = pNode->m_params[0].m_n;
            pMessage->SendServerToPlayerInventory_UnequipCancel(pPlayer->m_nPlayerId, oidItem);
            break;
        }
        }
    }

    switch (pNode->m_nActionId) {
    case AIACTION_MOVE_TO_POINT:
#if !KOTOR1  // K1: no such action id
    case AIACTION_FOLLOW_OWNER:
#endif
        m_pPathfindInfo->m_oidGoalObject = OBJECT_INVALID;
        m_pPathfindInfo->m_oidTarget = OBJECT_INVALID;
        m_nMoveState = 1;
        break;
    case AIACTION_PICK_UP_ITEM:
        SetAnimation(SERVER_ANIMATION_PAUSE);
        m_bPickUpItemStarted = 0;
        break;
    case AIACTION_DROP_ITEM:
        SetAnimation(SERVER_ANIMATION_PAUSE);
        m_bDropItemStarted = 0;
        break;
    case AIACTION_PHYSICAL_ATTACKS:
        m_oidAttackTarget = OBJECT_INVALID;
        m_attemptedAttackTargetId = OBJECT_INVALID;
        SetAnimation(SERVER_ANIMATION_PAUSE);
        ProcessPendingCombatActions();
        break;
    case AIACTION_CAST_SPELL:
        m_oidAttemptedSpellTarget = OBJECT_INVALID;
        m_oidSpellTarget = OBJECT_INVALID;
        m_pCombatRound->RemoveSpellAction();
        m_bSpellChargeSpent = 0;
        SetAnimation(SERVER_ANIMATION_PAUSE);
        ProcessPendingCombatActions();
        break;
    case AIACTION_OPEN_DOOR:
        SetAnimation(SERVER_ANIMATION_PAUSE);
        m_bOpenDoorPending = 0;
        break;
    case AIACTION_DIALOG_OBJECT:
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalDialogState(0);
        break;
    case AIACTION_DISARM_MINE:
    case AIACTION_RECOVER_MINE:
    case AIACTION_FLAG_MINE:
    case AIACTION_EXAMINE_MINE:
    case AIACTION_SET_MINE:
        SetAnimation(SERVER_ANIMATION_PAUSE);
        StopGuiTimingBar();
        m_bMineActionStarted = 0;
        m_bActionApproachDone = 0;
        break;
    case AIACTION_RESUME_DIALOG:
        m_bDialogPaused = 0;
        break;
    case AIACTION_UNLOCK_OBJECT:
        SetAnimation(SERVER_ANIMATION_PAUSE);
        m_bUnlockStarted = 0;
        StopGuiTimingBar();
        break;
    case AIACTION_LOCK_OBJECT:
        SetAnimation(SERVER_ANIMATION_PAUSE);
        m_bLockStarted = 0;
        StopGuiTimingBar();
        break;
    case AIACTION_USE_OBJECT: {
        CSWSPlaceable* pPlaceable =
            g_pAppManager->m_pServerExoApp->GetPlaceableByGameObjectID(pNode->m_params[0].m_n);
        if (pPlaceable != 0 && pPlaceable->m_corpse.m_bUseAnimationPending != 0) {
            pPlaceable->m_corpse.m_bUseAnimationPending = 0;
            if (pPlaceable->m_nAnimation == PLACEABLE_ANIM_OPEN)
                pPlaceable->SetAnimation(PLACEABLE_ANIM_CLOSE);
            pPlaceable->m_fAnimationSpeed = 1.0f;
        }
        break;
    }
    case AIACTION_REST:
        CancelRest(FEEDBACK_REST_CANCELLED);
        break;
    case AIACTION_ITEM_CAST_SPELL:
        m_oidAttemptedSpellTarget = OBJECT_INVALID;
        m_oidSpellTarget = OBJECT_INVALID;
        m_pCombatRound->RemoveSpellAction();
        SetAnimation(SERVER_ANIMATION_PAUSE);
        m_bItemCastStarted = 0;
        ProcessPendingCombatActions();
        break;
    case AIACTION_COUNTER_SPELL:
        SetCombatMode(CREATURE_COMBAT_MODE_NONE, 1);
        break;
    case AIACTION_PICK_POCKET:
        SetAnimation(SERVER_ANIMATION_PAUSE);
        m_bPickPocketStarted = 0;
        m_bPickPocketApproachDone = 0;
        break;
    case AIACTION_HEAL:
        SetAnimation(SERVER_ANIMATION_PAUSE);
        m_bHealAnimPlayed = 0;
        break;
    case AIACTION_PARTY_FOLLOW_LEADER:
        if (m_bInParty != 0) {
            unsigned long long oidClient =
                g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId);
            int nIndex = g_pAppManager->m_pClientExoApp->GetSWParty()->GetIndex(oidClient);
            if (nIndex >= 0 && nIndex <= 2) {
                CSWPartyMemberData* pMember =
                    &g_pAppManager->m_pClientExoApp->GetSWParty()->m_members[nIndex];
                pMember->m_nFollowState = CSWPartyMemberData::FOLLOW_STATE_NEW;
                pMember->m_nStateSteps = 0;
                pMember->m_followDistance = 0.0f;
                pMember->m_bReseatSafePosition = 0;
                pMember->m_followOrientation.x = 0.0f;
                pMember->m_nReserved1 = 0;
                pMember->m_nReserved2 = 0;
                pMember->m_nReserved3 = 0;
                pMember->m_vPathPoint.x = 0.0f;
                pMember->m_vPathPoint.y = 0.0f;
                pMember->m_vPathPoint.z = 0.0f;
                pMember->m_followPoint.x = 0.0f;
                pMember->m_followPoint.y = 0.0f;
                pMember->m_followPoint.z = 0.0f;
                pMember->m_bFollowPointValid = 0;
                pMember->m_followOrientation.y = 1.0f;
                pMember->m_followOrientation.z = 0.0f;
                pMember->m_fPathSpanRatio = 1.0f;
                pMember->m_oidLastTarget = OBJECT_INVALID;
                pMember->m_nCombatMessageStrRef = STRREF_COMBAT_MODE_ENGAGED;
            }
        }
        break;
#if !KOTOR1
    case AIACTION_DOOR_SABER:
        SetAnimation(SERVER_ANIMATION_PAUSE);
        m_bDoorSaberStarted = 0;
        StopGuiTimingBar();
        break;
    case AIACTION_STANCE_TOTAL_DEFENSE:
        m_nStancePosture = STANCE_POSTURE_NONE;
        SetAnimation(SERVER_ANIMATION_READY);
        ProcessPendingCombatActions();
        break;
    case AIACTION_STANCE_MEDITATIVE:
        m_nStancePosture = STANCE_POSTURE_NONE;
        RecomputeAmbientAnimationState();
        SetAnimation(SERVER_ANIMATION_PAUSE);
        break;
    case AIACTION_SWITCH_WEAPONS:
        SetAnimation(SERVER_ANIMATION_PAUSE);
        break;
#endif
    }

    if (pMessage != 0 && bIsCurrentAction != 0 && pNode->m_nActionId == AIACTION_MOVE_TO_POINT)
        SetAnimation(SERVER_ANIMATION_PAUSE);

    if (GetLockOrientationToObject() != CGameObjectArray::INVALIDOBJID) {
        GetLockOrientationToObject();
        SetLockOrientationToObject(CGameObjectArray::INVALIDOBJID, 0);
    }

    if (bIsCurrentAction == 1) {
        m_nActionStartDay = 0;
        m_nActionStartTod = 0;
    }
    return 1;
}




void CSWSCreaturePartyFollowInfo::Save(CResGFF* pGff, CResStruct* pStruct)
{
    pGff->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_leaderId), const_cast<char*>("FollowObject"));
    pGff->WriteFieldVector(pStruct, m_followLocation, const_cast<char*>("FollowLocation"));
    pGff->WriteFieldVector(pStruct, m_lastLeaderPos, const_cast<char*>("LastLeaderPos"));
    pGff->WriteFieldVector(pStruct, m_lastFollowerPos, const_cast<char*>("LastFollowerPos"));
    pGff->WriteFieldFLOAT(pStruct, m_maxSpeed, const_cast<char*>("MaxSpeed"));
    pGff->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_stickToPos), const_cast<char*>("StickToPos"));
    pGff->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_followState), const_cast<char*>("Result"));
    pGff->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_nTimeElapsed), const_cast<char*>("TimeElapsed"));
    pGff->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bWithin5m), const_cast<char*>("InSafetyRange"));
}

void CSWSCreaturePartyFollowInfo::Load(CResGFF* pGff, CResStruct* pStruct)
{
    int nSuccess;

    m_leaderId = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("FollowObject"), nSuccess, static_cast<unsigned int>(m_leaderId));
    m_followLocation = pGff->ReadFieldVector(pStruct, const_cast<char*>("FollowLocation"), nSuccess, m_followLocation);
    m_lastLeaderPos = pGff->ReadFieldVector(pStruct, const_cast<char*>("LastLeaderPos"), nSuccess, m_lastLeaderPos);
    m_lastFollowerPos = pGff->ReadFieldVector(pStruct, const_cast<char*>("LastFollowerPos"), nSuccess, m_lastFollowerPos);
    m_maxSpeed = pGff->ReadFieldFLOAT(pStruct, const_cast<char*>("MaxSpeed"), nSuccess, m_maxSpeed);
    m_stickToPos = pGff->ReadFieldBYTE(pStruct, const_cast<char*>("StickToPos"), nSuccess, static_cast<unsigned char>(m_stickToPos));
    m_followState = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("Result"), nSuccess, static_cast<unsigned int>(m_followState));
    m_nTimeElapsed = pGff->ReadFieldDWORD(pStruct, const_cast<char*>("TimeElapsed"), nSuccess, static_cast<unsigned int>(m_nTimeElapsed));
    m_bWithin5m = pGff->ReadFieldBYTE(pStruct, const_cast<char*>("InSafetyRange"), nSuccess, static_cast<unsigned char>(m_bWithin5m));
}
