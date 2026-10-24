// CSWSEffectListHandler has one handler per effect id. Each OnApply* re-derives the effects an
// id implies and applies them to the target; each OnRemove* undoes it.
#include "k2stdint.h"
#include "nwseffectlist.h"
#include "nwscriptconst.h"
#include "nwmessagetypes.h"
#include "CExoArrayListImpl.h"
#include "enginevfx.h"

// CGameEffect::m_subType values, the engine's effect types. Same numbering as the
// CSWSEffectListHandler::EFFECT_* constants, which are out-of-line data.
enum {
    GAME_EFFECT_HASTE                     = 1,
    GAME_EFFECT_DAMAGE_RESISTANCE         = 2,
    GAME_EFFECT_SLOW                      = 3,
    GAME_EFFECT_RESURRECTION              = 4,
    GAME_EFFECT_DISEASE                   = 5,
    GAME_EFFECT_SETSTATE                  = 8,
    GAME_EFFECT_SETSTATE_INTERNAL         = 9,
    GAME_EFFECT_ATTACK_DECREASE           = 11,
    GAME_EFFECT_DAMAGE_INCREASE           = 13,
    GAME_EFFECT_TEMPORARY_HITPOINTS       = 15,
    GAME_EFFECT_DAMAGE_IMMUNITY_INCREASE  = 16,
    GAME_EFFECT_DAMAGE_IMMUNITY_DECREASE  = 17,
    GAME_EFFECT_DEATH                     = 19,
    GAME_EFFECT_IMMUNITY                  = 22,
    GAME_EFFECT_SET_AI_STATE              = 23,
    GAME_EFFECT_ENEMY_ATTACK_BONUS        = 24,
    GAME_EFFECT_ARCANE_SPELL_FAILURE      = 25,
    GAME_EFFECT_SAVING_THROW_DECREASE     = 27,
    GAME_EFFECT_MOVEMENT_SPEED_INCREASE   = 28,
    GAME_EFFECT_MOVEMENT_SPEED_DECREASE   = 29,
    GAME_EFFECT_VISUALEFFECT              = 30,
    GAME_EFFECT_FORCE_RESISTANCE_INCREASE = 33,
    GAME_EFFECT_FORCE_RESISTANCE_DECREASE = 34,
    GAME_EFFECT_POISON                    = 35,
    GAME_EFFECT_ABILITY_INCREASE          = 36,
    GAME_EFFECT_ABILITY_DECREASE          = 37,
    GAME_EFFECT_DAMAGE                    = 38,
    GAME_EFFECT_HEAL                      = 39,
    GAME_EFFECT_HASTE_INTERNAL            = 41,
    GAME_EFFECT_SLOW_INTERNAL             = 42,
    GAME_EFFECT_INVISIBILITY              = 47,
    GAME_EFFECT_AC_INCREASE               = 48,
    GAME_EFFECT_AC_DECREASE               = 49,
    GAME_EFFECT_SKILL_DECREASE            = 56,
    GAME_EFFECT_HITPOINTCHANGEWHENDYING   = 57,
    GAME_EFFECT_SETWALKANIMATION          = 58,
    GAME_EFFECT_LIMIT_MOVEMENT_SPEED      = 59,
    GAME_EFFECT_DISGUISE                  = 62,
    GAME_EFFECT_SANCTUARY                 = 63,
    GAME_EFFECT_TIMESTOP                  = 64,
    GAME_EFFECT_EFFECT_ICON               = 67,
    GAME_EFFECT_VISION                    = 69,
    GAME_EFFECT_SEEINVISIBLE              = 70,
    GAME_EFFECT_ULTRAVISION               = 71,
    GAME_EFFECT_TRUESEEING                = 72,
    GAME_EFFECT_BLINDNESS                 = 73,
    GAME_EFFECT_MISS_CHANCE               = 75,
    GAME_EFFECT_WOUNDING                  = 84,
    GAME_EFFECT_FORCEJUMP_DELAYED         = 103,
    GAME_EFFECT_FORCERESISTED             = 105,
    GAME_EFFECT_FORCESHIELD               = 107,
    GAME_EFFECT_FURY                      = 111,
};

// The last effect type the OnApply/OnRemove tables hold (CSWSEffectListHandler::MAX_EFFECTS - 1).
enum {
#if KOTOR1
    GAME_EFFECT_LAST = 109,
#else
    GAME_EFFECT_LAST = 117,
#endif
};

// effecticon.2da rows of the alignment-mastery icons.
enum {
    EFFECT_ICON_MAX_GOOD = 60,
#if KOTOR1
    EFFECT_ICON_MAX_EVIL = 61,
#else
    EFFECT_ICON_MAX_EVIL = 62,
#endif
};

// The physical damage-type mask: bludgeoning, piercing, slashing and the base-damage bit.
enum {
    DAMAGE_TYPES_PHYSICAL = 0x4007,
};

// The skill id a skill-decrease effect uses for every skill at once.
enum {
    SKILL_ALL = 0xff,
};

// CSWVisibilityNode::m_flags bits 2-3: what a pacify attempt left on the observer.
enum {
    VISIBILITY_PACIFIED          = 1 << 2,
    VISIBILITY_PACIFY_RESISTED   = 1 << 3,
    VISIBILITY_PACIFY_STATE_MASK = VISIBILITY_PACIFIED | VISIBILITY_PACIFY_RESISTED,
};

// Placeable animation ids (the ANIMATION_PLACEABLE_* script ids map onto these).
enum {
    PLACEABLE_ANIM_DEAD  = 10072,
    PLACEABLE_ANIM_OPEN  = 10075,
    PLACEABLE_ANIM_CLOSE = 10076,
};

// AddMoveToPointActionToFront's group argument: start a new action group.
enum {
    ACTION_GROUP_NEW = 0xffff,
};

// m_nAIState bits the SET_AI_STATE effects clear (the effect's integer is the mask to keep):
// silence takes speech, deafness hearing.
enum {
    AI_STATE_CAN_SPEAK = 1 << 3,
    AI_STATE_CAN_HEAR  = 1 << 4,
};

// visualeffects.2da ids the blood-colour branches (BLOODCOLR R, G, Y) and OnApplyDisease apply.
// Neither game's visualeffects.2da has these rows, so the visual finds no row.
enum {
    VFX_COM_BLOOD_RED    = 158,
    VFX_COM_BLOOD_GREEN  = 159,
    VFX_COM_BLOOD_YELLOW = 160,
    VFX_IMP_DISEASE      = 51,
};

// m_ambientAnimationState values of a creature already lying dead (RecomputeAmbientAnimationState
// keeps 3, 4 and 14).
enum {
    AMBIENT_STATES_DEAD = (1 << 3) | (1 << 4) | (1 << 14),
};

const int CSWSEffectListHandler::DELETE_EFFECT = 1;
const int CSWSEffectListHandler::EFFECT_INVALIDEFFECT = 0;
const int CSWSEffectListHandler::EFFECT_HASTE = 1;
const int CSWSEffectListHandler::EFFECT_DAMAGE_RESISTANCE = 2;
const int CSWSEffectListHandler::EFFECT_SLOW = 3;
const int CSWSEffectListHandler::EFFECT_RESURRECTION = 4;
const int CSWSEffectListHandler::EFFECT_DISEASE = 5;
const int CSWSEffectListHandler::EFFECT_SUMMON_CREATURE = 6;
const int CSWSEffectListHandler::EFFECT_REGENERATE = 7;
const int CSWSEffectListHandler::EFFECT_SETSTATE = 8;
const int CSWSEffectListHandler::EFFECT_SETSTATE_INTERNAL = 9;
const int CSWSEffectListHandler::EFFECT_ATTACK_INCREASE = 10;
const int CSWSEffectListHandler::EFFECT_ATTACK_DECREASE = 11;
const int CSWSEffectListHandler::EFFECT_DAMAGE_REDUCTION = 12;
const int CSWSEffectListHandler::EFFECT_DAMAGE_INCREASE = 13;
const int CSWSEffectListHandler::EFFECT_DAMAGE_DECREASE = 14;
const int CSWSEffectListHandler::EFFECT_TEMPORARY_HITPOINTS = 15;
const int CSWSEffectListHandler::EFFECT_DAMAGE_IMMUNITY_INCREASE = 16;
const int CSWSEffectListHandler::EFFECT_DAMAGE_IMMUNITY_DECREASE = 17;
const int CSWSEffectListHandler::EFFECT_ENTANGLE = 18;
const int CSWSEffectListHandler::EFFECT_DEATH = 19;
const int CSWSEffectListHandler::EFFECT_KNOCKDOWN = 20;
const int CSWSEffectListHandler::EFFECT_DEAF = 21;
const int CSWSEffectListHandler::EFFECT_IMMUNITY = 22;
const int CSWSEffectListHandler::EFFECT_SET_AI_STATE = 23;
const int CSWSEffectListHandler::EFFECT_ENEMY_ATTACK_BONUS = 24;
const int CSWSEffectListHandler::EFFECT_ARCANE_SPELL_FAILURE = 25;
const int CSWSEffectListHandler::EFFECT_SAVING_THROW_INCREASE = 26;
const int CSWSEffectListHandler::EFFECT_SAVING_THROW_DECREASE = 27;
const int CSWSEffectListHandler::EFFECT_MOVEMENT_SPEED_INCREASE = 28;
const int CSWSEffectListHandler::EFFECT_MOVEMENT_SPEED_DECREASE = 29;
const int CSWSEffectListHandler::EFFECT_VISUALEFFECT = 30;
const int CSWSEffectListHandler::EFFECT_AREA_OF_EFFECT = 31;
const int CSWSEffectListHandler::EFFECT_BEAM = 32;
const int CSWSEffectListHandler::EFFECT_FORCE_RESISTANCE_INCREASE = 33;
const int CSWSEffectListHandler::EFFECT_FORCE_RESISTANCE_DECREASE = 34;
const int CSWSEffectListHandler::EFFECT_POISON = 35;
const int CSWSEffectListHandler::EFFECT_ABILITY_INCREASE = 36;
const int CSWSEffectListHandler::EFFECT_ABILITY_DECREASE = 37;
const int CSWSEffectListHandler::EFFECT_DAMAGE = 38;
const int CSWSEffectListHandler::EFFECT_HEAL = 39;
const int CSWSEffectListHandler::EFFECT_LINK = 40;
const int CSWSEffectListHandler::EFFECT_MODIFYNUMATTACKS = 44;
const int CSWSEffectListHandler::EFFECT_CURSE = 45;
const int CSWSEffectListHandler::EFFECT_SILENCE = 46;
const int CSWSEffectListHandler::EFFECT_INVISIBILITY = 47;
const int CSWSEffectListHandler::EFFECT_AC_INCREASE = 48;
const int CSWSEffectListHandler::EFFECT_AC_DECREASE = 49;
const int CSWSEffectListHandler::EFFECT_SPELL_IMMUNITY = 50;
const int CSWSEffectListHandler::EFFECT_DISPEL_ALL_MAGIC = 51;
const int CSWSEffectListHandler::EFFECT_DISPEL_BEST_MAGIC = 52;
const int CSWSEffectListHandler::EFFECT_LIGHT = 54;
const int CSWSEffectListHandler::EFFECT_SKILL_INCREASE = 55;
const int CSWSEffectListHandler::EFFECT_SKILL_DECREASE = 56;
const int CSWSEffectListHandler::EFFECT_HITPOINTCHANGEWHENDYING = 57;
const int CSWSEffectListHandler::EFFECT_SETWALKANIMATION = 58;
const int CSWSEffectListHandler::EFFECT_LIMIT_MOVEMENT_SPEED = 59;
const int CSWSEffectListHandler::EFFECT_FORCEPUSHED = 60;
const int CSWSEffectListHandler::EFFECT_DAMAGE_SHIELD = 61;
const int CSWSEffectListHandler::EFFECT_DISGUISE = 62;
const int CSWSEffectListHandler::EFFECT_SANCTUARY = 63;
const int CSWSEffectListHandler::EFFECT_TIMESTOP = 64;
const int CSWSEffectListHandler::EFFECT_SPELL_LEVEL_ABSORPTION = 65;
const int CSWSEffectListHandler::EFFECT_EFFECT_ICON = 67;
const int CSWSEffectListHandler::EFFECT_RACIAL_TYPE = 68;
const int CSWSEffectListHandler::EFFECT_SEEINVISIBLE = 70;
const int CSWSEffectListHandler::EFFECT_ULTRAVISION = 71;
const int CSWSEffectListHandler::EFFECT_TRUESEEING = 72;
const int CSWSEffectListHandler::EFFECT_BLINDNESS = 73;
const int CSWSEffectListHandler::EFFECT_DARKNESS = 74;
const int CSWSEffectListHandler::EFFECT_MISS_CHANCE = 75;
const int CSWSEffectListHandler::EFFECT_CONCEALMENT = 76;
const int CSWSEffectListHandler::EFFECT_APPEAR = 81;
const int CSWSEffectListHandler::EFFECT_NEGATIVE_LEVEL = 82;
const int CSWSEffectListHandler::EFFECT_BONUSFEAT = 83;
const int CSWSEffectListHandler::EFFECT_WOUNDING = 84;
const int CSWSEffectListHandler::EFFECT_SUMMON_PARTY = 89;
const int CSWSEffectListHandler::EFFECT_FORCEDRAIN = 90;
const int CSWSEffectListHandler::EFFECT_TEMPORARY_FORCE = 91;
const int CSWSEffectListHandler::EFFECT_BLASTERDEFLECTIONINCREASE = 92;
const int CSWSEffectListHandler::EFFECT_BLASTERDEFLECTIONDECREASE = 93;
const int CSWSEffectListHandler::EFFECT_DAMAGEFORCEPOINTS = 95;
const int CSWSEffectListHandler::EFFECT_HEALFORCEPOINTS = 96;
const int CSWSEffectListHandler::EFFECT_BODYFUEL = 98;
const int CSWSEffectListHandler::EFFECT_PSYCHICSTATIC = 99;
const int CSWSEffectListHandler::EFFECT_LIGHTSABERTHROW = 100;
const int CSWSEffectListHandler::EFFECT_ASSUREDHIT = 101;
const int CSWSEffectListHandler::EFFECT_FORCEJUMP = 102;
const int CSWSEffectListHandler::EFFECT_ASSUREDDEFLECTION = 104;
const int CSWSEffectListHandler::EFFECT_FORCERESISTED = 105;
const int CSWSEffectListHandler::EFFECT_FORCEFIZZLE = 106;
const int CSWSEffectListHandler::EFFECT_FORCESHIELD = 107;
const int CSWSEffectListHandler::EFFECT_PUREGOODPOWERS = 108;
const int CSWSEffectListHandler::EFFECT_PUREEVILPOWERS = 109;
const int CSWSEffectListHandler::EFFECT_FORCE_BODY = 110;
const int CSWSEffectListHandler::EFFECT_FURY = 111;
const int CSWSEffectListHandler::EFFECT_FPREGENMODIFIER = 112;
const int CSWSEffectListHandler::EFFECT_VPREGENMODIFIER = 113;
const int CSWSEffectListHandler::EFFECT_FORCE_SIGHT = 114;
const int CSWSEffectListHandler::EFFECT_FACTION_MODIFIER = 115;
#if !KOTOR1
const int CSWSEffectListHandler::EFFECT_DESTROY_SHIELDS = 116;
const int CSWSEffectListHandler::EFFECT_ASSASSINATE = 117;
#endif
#if KOTOR1
const int CSWSEffectListHandler::MAX_EFFECTS = 110;
#else
const int CSWSEffectListHandler::MAX_EFFECTS = 118;
#endif
const int CSWSEffectListHandler::EFFECT_ABILITY_STRENGTH = 0;
const int CSWSEffectListHandler::EFFECT_ABILITY_DEXTERITY = 1;
const int CSWSEffectListHandler::EFFECT_ABILITY_CONSTITUTION = 2;
const int CSWSEffectListHandler::EFFECT_ABILITY_INTELLIGENCE = 3;
const int CSWSEffectListHandler::EFFECT_ABILITY_WISDOM = 4;
const int CSWSEffectListHandler::EFFECT_ABILITY_CHARISMA = 5;
const int CSWSEffectListHandler::DAMAGE_FLAG_BLUDGEONING = 1;
const int CSWSEffectListHandler::DAMAGE_FLAG_PIERCING = 2;
const int CSWSEffectListHandler::DAMAGE_FLAG_SLASHING = 4;
const int CSWSEffectListHandler::DAMAGE_FLAG_UNIVERSAL = 8;
const int CSWSEffectListHandler::DAMAGE_FLAG_ACID = 16;
const int CSWSEffectListHandler::DAMAGE_FLAG_COLD = 32;
const int CSWSEffectListHandler::DAMAGE_FLAG_LIGHT_SIDE = 64;
const int CSWSEffectListHandler::DAMAGE_FLAG_ELECTRICAL = 128;
const int CSWSEffectListHandler::DAMAGE_FLAG_FIRE = 256;
const int CSWSEffectListHandler::DAMAGE_FLAG_DARK_SIDE = 512;
const int CSWSEffectListHandler::DAMAGE_FLAG_SONIC = 1024;
const int CSWSEffectListHandler::DAMAGE_FLAG_ION = 2048;
const int CSWSEffectListHandler::DAMAGE_FLAG_BLASTER = 4096;
const int CSWSEffectListHandler::DAMAGE_FLAG_POISON = 8192;
const int CSWSEffectListHandler::DAMAGE_FLAG_BASE_DAMAGE = 16384;
const int CSWSEffectListHandler::DAMAGE_FLAG_PHYSICAL = 16391;
const int CSWSEffectListHandler::DAMAGE_FLAG_ALL = 16383;
const int CSWSEffectListHandler::DAMAGE_POWER_NORMAL = 0;
const int CSWSEffectListHandler::DAMAGE_POWER_ENERGY = 6;
const int CSWSEffectListHandler::AC_DODGE = 0;
const int CSWSEffectListHandler::AC_NATURAL = 1;
const int CSWSEffectListHandler::AC_ARMOUR_BONUS = 2;
const int CSWSEffectListHandler::AC_SHIELD_BONUS = 3;
const int CSWSEffectListHandler::AC_DEFLECTION = 4;
const int CSWSEffectListHandler::ATTACK_BONUS_MISC = 0;
const int CSWSEffectListHandler::ATTACK_BONUS_ONHAND = 1;
const int CSWSEffectListHandler::ATTACK_BONUS_OFFHAND = 2;
const int CSWSEffectListHandler::ATTACK_BONUS_CWEAPON1 = 3;
const int CSWSEffectListHandler::ATTACK_BONUS_CWEAPON2 = 4;
const int CSWSEffectListHandler::ATTACK_BONUS_CWEAPON3 = 5;
const int CSWSEffectListHandler::ATTACK_BONUS_UNARMED = 7;
const int CSWSEffectListHandler::DAMAGE_BONUS_MISC = 0;
const int CSWSEffectListHandler::DAMAGE_BONUS_ONHAND = 1;
const int CSWSEffectListHandler::DAMAGE_BONUS_OFFHAND = 2;
const int CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON1 = 3;
const int CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON2 = 4;
const int CSWSEffectListHandler::DAMAGE_BONUS_CWEAPON3 = 5;
const int CSWSEffectListHandler::DAMAGE_BONUS_UNARMED = 7;
const int CSWSEffectListHandler::IMMUNITY_TYPE_ALL = 0;
const int CSWSEffectListHandler::IMMUNITY_TYPE_MIND_AFFECTING = 1;
const int CSWSEffectListHandler::IMMUNITY_TYPE_POISON = 2;
const int CSWSEffectListHandler::IMMUNITY_TYPE_DISEASE = 3;
const int CSWSEffectListHandler::IMMUNITY_TYPE_FEAR = 4;
const int CSWSEffectListHandler::IMMUNITY_TYPE_MINE = 5;
const int CSWSEffectListHandler::IMMUNITY_TYPE_PARALYSIS = 6;
const int CSWSEffectListHandler::IMMUNITY_TYPE_BLINDNESS = 7;
const int CSWSEffectListHandler::IMMUNITY_TYPE_DEAFNESS = 8;
const int CSWSEffectListHandler::IMMUNITY_TYPE_SLOW = 9;
const int CSWSEffectListHandler::IMMUNITY_TYPE_ENTANGLE = 10;
const int CSWSEffectListHandler::IMMUNITY_TYPE_SILENCE = 11;
const int CSWSEffectListHandler::IMMUNITY_TYPE_STUN = 12;
const int CSWSEffectListHandler::IMMUNITY_TYPE_SLEEP = 13;
const int CSWSEffectListHandler::IMMUNITY_TYPE_CONFUSED = 16;
const int CSWSEffectListHandler::IMMUNITY_TYPE_CURSED = 17;
const int CSWSEffectListHandler::IMMUNITY_TYPE_DAZED = 18;
const int CSWSEffectListHandler::IMMUNITY_TYPE_ABILITY_DECREASE = 19;
const int CSWSEffectListHandler::IMMUNITY_TYPE_MOVEMENT_SPEED_DECREASE = 24;
const int CSWSEffectListHandler::IMMUNITY_TYPE_KNOCKDOWN = 28;
const int CSWSEffectListHandler::IMMUNITY_TYPE_NEGATIVE_LEVEL = 29;
const int CSWSEffectListHandler::IMMUNITY_TYPE_BACKSTAB = 30;
const int CSWSEffectListHandler::IMMUNITY_TYPE_CRITICAL_HITS = 31;
const int CSWSEffectListHandler::IMMUNITY_TYPE_DEATH_MAGIC = 32;
#if !KOTOR1
const int CSWSEffectListHandler::IMMUNITY_TYPE_DROID_CONFUSED = 33;
#endif
#if KOTOR1
const int CSWSEffectListHandler::IMMUNITY_TYPE_MAX = 33;
#else
const int CSWSEffectListHandler::IMMUNITY_TYPE_MAX = 34;
#endif
const int CSWSEffectListHandler::MISS_CHANCE_TYPE_NORMAL = 0;

CSWSEffectListHandler::~CSWSEffectListHandler()
{
    if (m_paOnApply) {
        delete[] m_paOnApply;
        m_paOnApply = 0;
    }
    if (m_paOnRemove) {
        delete[] m_paOnRemove;
        m_paOnRemove = 0;
    }
}

#include "nwsobject.h"
#include "nwscreature.h"
#include "nwscreaturestats.h"
#include "nwsplayer.h"
#include "nwseffect.h"
#include "nwclass.h"
#include "appmanager.h"
#include "serverexoapp.h"
#include "nwsrules.h"
#include "worldtimer.h"
#include "nwscombatround.h"
#include "nwsmodule.h"
#include "nwrace.h"
#include "gameobjectarray.h"
#include "serveraimaster.h"
#include "nwscript.h"
#include "nwmessage.h"
#include "twodimarrays.h"
#include "exobase.h"
#include "nwsencounter.h"
#include "twodimarray.h"
#include "nwupdatestats.h"
#include "nwsarea.h"
#include "CSWVisibilityNode.h"
#include "nwssendplayermessage.h"
#include "nwsitem.h"
#include "nwsdoor.h"
#include "nwspell.h"
#include "nwsscriptevent.h"
#include "nwccreature.h"

extern int g_bDisableDamage;
extern int g_bEnableCombatDebugging;

#include "port/crtapi.h"

// "Pure good powers": the light-side prestige perk. Only the controlled player creature gets
// it; what it grants depends on which classes the creature carries.
int CSWSEffectListHandler::OnApplyPureGoodPowers(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature == 0)
        return 1;
    CSWSCreatureStats* pStats = pCreature->m_pStats;
    if (pStats == 0)
        return 1;

    CSWSPlayer* pPlayer =
        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(pCreature->m_objectId);
    if (pPlayer != 0 && pPlayer->m_gameObjectId == pCreature->m_objectId) {
#if KOTOR1  // no prestige classes in K1; the Consular bonus is ability 5
        for (unsigned char i = 0; i < pStats->m_numClasses; i++) {
            unsigned char nClass = pStats->GetClass(i);
            if (nClass == CSWClass::CCLASS_JEDICONSULAR) {
                int nOldMax = pCreature->GetMaxForcePoints();
                CGameEffect* pNewEffect = new CGameEffect(pEffect);
                pNewEffect->m_subType = GAME_EFFECT_ABILITY_INCREASE;
                pNewEffect->SetInteger(0, 5);
                pNewEffect->SetInteger(1, 3);
                pCreature->ApplyEffect(pNewEffect, 0, 0);
                pStats->m_nCurrentForcePoints =
                    static_cast<unsigned short>(pCreature->GetMaxForcePoints() - nOldMax + pStats->m_nCurrentForcePoints);
            } else if (nClass == CSWClass::CCLASS_JEDIGUARDIAN) {
                CGameEffect* pNewEffect = new CGameEffect(pEffect);
                pNewEffect->m_subType = GAME_EFFECT_ABILITY_INCREASE;
                pNewEffect->SetInteger(0, 0);
                pNewEffect->SetInteger(1, 3);
                pCreature->ApplyEffect(pNewEffect, 0, 0);
            } else if (nClass == CSWClass::CCLASS_JEDISENTINEL) {
                int nOldMax = pCreature->GetMaxHitPoints(0);
                CGameEffect* pNewEffect = new CGameEffect(pEffect);
                pNewEffect->m_subType = GAME_EFFECT_ABILITY_INCREASE;
                pNewEffect->SetInteger(0, 2);
                pNewEffect->SetInteger(1, 3);
                pCreature->ApplyEffect(pNewEffect, 0, 0);
                short nGained = static_cast<short>(pCreature->GetMaxHitPoints(0) - nOldMax);
                short nCurrent = pCreature->GetCurrentHitPoints(1);
                pCreature->SetCurrentHitPoints(nCurrent + nGained);
            }
        }
#else
        for (int i = 0; i < pStats->m_numClasses; i++) {
            unsigned char nClass = pStats->m_classInfo[i].m_class;
            if (nClass == CSWClass::CCLASS_JEDICONSULAR ||
                nClass == CSWClass::CCLASS_JEDIMASTER ||
                nClass == CSWClass::CCLASS_SITHLORD) {
                int nOldMax = pCreature->GetMaxForcePoints();
                CGameEffect* pNewEffect = new CGameEffect(pEffect);
                pNewEffect->m_subType = GAME_EFFECT_ABILITY_INCREASE;
                pNewEffect->SetInteger(0, 4);
                pNewEffect->SetInteger(1, 3);
                pCreature->ApplyEffect(pNewEffect, 0, 0);
                pStats->m_nCurrentForcePoints =
                    static_cast<unsigned short>(pCreature->GetMaxForcePoints() - nOldMax + pStats->m_nCurrentForcePoints);
            } else if (nClass == CSWClass::CCLASS_JEDIGUARDIAN ||
                       nClass == CSWClass::CCLASS_JEDIWEAPONMASTER ||
                       nClass == CSWClass::CCLASS_SITHMARAUDER) {
                CGameEffect* pNewEffect = new CGameEffect(pEffect);
                pNewEffect->m_subType = GAME_EFFECT_ABILITY_INCREASE;
                pNewEffect->SetInteger(0, 0);
                pNewEffect->SetInteger(1, 3);
                pCreature->ApplyEffect(pNewEffect, 0, 0);
            } else if (nClass == CSWClass::CCLASS_JEDISENTINEL ||
                       nClass == CSWClass::CCLASS_JEDIWATCHMAN ||
                       nClass == CSWClass::CCLASS_SITHASSASSIN) {
                int nOldMax = pCreature->GetMaxHitPoints(0);
                CGameEffect* pNewEffect = new CGameEffect(pEffect);
                pNewEffect->m_subType = GAME_EFFECT_ABILITY_INCREASE;
                pNewEffect->SetInteger(0, 2);
                pNewEffect->SetInteger(1, 3);
                pCreature->ApplyEffect(pNewEffect, 0, 0);
                short nGained = static_cast<short>(pCreature->GetMaxHitPoints(0) - nOldMax);
                short nCurrent = pCreature->GetCurrentHitPoints(1);
                pCreature->SetCurrentHitPoints(nCurrent + nGained);
            }
        }
#endif
    }

    CGameEffect* pMarker = new CGameEffect(pEffect);
    pMarker->m_subType = GAME_EFFECT_EFFECT_ICON;
    pMarker->SetInteger(0, EFFECT_ICON_MAX_GOOD);
    pCreature->ApplyEffect(pMarker, 0, 0);
    return 0;
}

// "Pure evil powers": the dark-side counterpart, over the same nine classes in a different
// order.
int CSWSEffectListHandler::OnApplyPureEvilPowers(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature == 0)
        return 1;
    CSWSCreatureStats* pStats = pCreature->m_pStats;
    if (pStats == 0)
        return 1;

    CSWSPlayer* pPlayer =
        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(pCreature->m_objectId);
    if (pPlayer != 0 && pPlayer->m_gameObjectId == pCreature->m_objectId) {
#if KOTOR1  // no prestige classes in K1; the Sentinel gets effect 22
        for (unsigned char i = 0; i < pStats->m_numClasses; i++) {
            unsigned char nClass = pStats->GetClass(i);
            if (nClass == CSWClass::CCLASS_JEDICONSULAR) {
                pStats->m_nCurrentForcePoints += 50;
            } else if (nClass == CSWClass::CCLASS_JEDIGUARDIAN) {
                CGameEffect* pFirst = new CGameEffect(pEffect);
                pFirst->m_subType = GAME_EFFECT_DAMAGE_INCREASE;
                pFirst->SetInteger(0, 8);
                pFirst->SetInteger(1, 3);
                pFirst->SetInteger(2, g_pRules->m_nNumRaces);
                pFirst->SetInteger(5, 1);
                pCreature->ApplyEffect(pFirst, 0, 0);

                CGameEffect* pSecond = new CGameEffect(pEffect);
                pSecond->m_subType = GAME_EFFECT_DAMAGE_INCREASE;
                pSecond->SetInteger(0, 8);
                pSecond->SetInteger(1, 3);
                pSecond->SetInteger(2, g_pRules->m_nNumRaces);
                pSecond->SetInteger(5, 2);
                pCreature->ApplyEffect(pSecond, 0, 0);
            } else if (nClass == CSWClass::CCLASS_JEDISENTINEL) {
                CGameEffect* pNewEffect = new CGameEffect(pEffect);
                pNewEffect->m_subType = GAME_EFFECT_IMMUNITY;
                pNewEffect->SetInteger(0, 2);
                pNewEffect->SetInteger(1, g_pRules->m_nNumRaces);
                pCreature->ApplyEffect(pNewEffect, 0, 0);
            }
        }
#else
        for (int i = 0; i < pStats->m_numClasses; i++) {
            unsigned char nClass = pStats->m_classInfo[i].m_class;
            if (nClass == CSWClass::CCLASS_JEDICONSULAR ||
                nClass == CSWClass::CCLASS_SITHLORD ||
                nClass == CSWClass::CCLASS_JEDIMASTER) {
                pStats->m_nCurrentForcePoints += 50;
            } else if (nClass == CSWClass::CCLASS_JEDIGUARDIAN ||
                       nClass == CSWClass::CCLASS_SITHMARAUDER ||
                       nClass == CSWClass::CCLASS_JEDIWEAPONMASTER) {
                CGameEffect* pFirst = new CGameEffect(pEffect);
                pFirst->m_subType = GAME_EFFECT_DAMAGE_INCREASE;
                pFirst->SetInteger(0, 8);
                pFirst->SetInteger(1, 3);
                pFirst->SetInteger(2, g_pRules->m_nNumRaces);
                pFirst->SetInteger(5, 1);
                pCreature->ApplyEffect(pFirst, 0, 0);

                CGameEffect* pSecond = new CGameEffect(pEffect);
                pSecond->m_subType = GAME_EFFECT_DAMAGE_INCREASE;
                pSecond->SetInteger(0, 8);
                pSecond->SetInteger(1, 3);
                pSecond->SetInteger(2, g_pRules->m_nNumRaces);
                pSecond->SetInteger(5, 2);
                pCreature->ApplyEffect(pSecond, 0, 0);
            } else if (nClass == CSWClass::CCLASS_JEDISENTINEL ||
                       nClass == CSWClass::CCLASS_SITHASSASSIN ||
                       nClass == CSWClass::CCLASS_JEDIWATCHMAN) {
                CGameEffect* pNewEffect = new CGameEffect(pEffect);
                pNewEffect->m_subType = GAME_EFFECT_ABILITY_INCREASE;
                pNewEffect->SetInteger(0, 1);
                pNewEffect->SetInteger(1, 3);
                pCreature->ApplyEffect(pNewEffect, 0, 0);
            }
        }
#endif
    }

    CGameEffect* pMarker = new CGameEffect(pEffect);
    pMarker->m_subType = GAME_EFFECT_EFFECT_ICON;
    pMarker->SetInteger(0, EFFECT_ICON_MAX_EVIL);
    pCreature->ApplyEffect(pMarker, 0, 0);
    return 0;
}

// Per-effect hooks, in effect-id order.
//
// The ability-score effects are applied by CSWSCreatureStats' lazy recompute, not here; these two
// hooks only evaluate their operands.
int CSWSEffectListHandler::OnRemoveAbilityIncrease(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        pObject->AsSWSCreature();
        pEffect->GetInteger(0);
        pEffect->GetInteger(1);
    }
    return 1;
}

int CSWSEffectListHandler::OnRemoveAbilityDecrease(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        pObject->AsSWSCreature();
        pEffect->GetInteger(0);
    }
    return 1;
}

// The effect's object slot 1 names the summon; it unsummons itself, which tears down its own
// effect and the association link to its master.
int CSWSEffectListHandler::OnRemoveSummonCreature(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject);
    CSWSCreature* pSummon =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->GetObjectID(1));
    if (pSummon != 0)
        pSummon->UnsummonMyself();
    return 1;
}

// Consumed by CSWSCreature::DoDamage's own effect-list scan; nothing to do here.
int CSWSEffectListHandler::OnApplyDamageResistance(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect); K2_UNUSED(a);
    return 0;
}

// A bonus of zero or less is rejected; a real one just marks the derived combat data dirty.
int CSWSEffectListHandler::OnApplySavingThrowIncrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pEffect != 0) {
            if (pEffect->GetInteger(0) <= 0)
                return 1;
            pCreature->m_bUpdateCombatInformation = 1;
        }
    }
    return 0;
}

int CSWSEffectListHandler::OnRemoveSavingThrowIncrease(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    if (pObject->AsSWSCreature() != 0)
        pObject->AsSWSCreature()->m_bUpdateCombatInformation = 1;
    return 1;
}

int CSWSEffectListHandler::OnRemoveSavingThrowDecrease(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    if (pObject->AsSWSCreature() != 0)
        pObject->AsSWSCreature()->m_bUpdateCombatInformation = 1;
    return 1;
}

int CSWSEffectListHandler::OnApplyAttackIncrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pEffect != 0 && pCreature != 0) {
        if (pEffect->GetInteger(0) <= 0)
            return 1;
        pObject->AsSWSCreature()->m_bUpdateCombatInformation = 1;
    }
    return 0;
}

int CSWSEffectListHandler::OnRemoveAttackIncrease(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    if (pObject->AsSWSCreature() != 0)
        pObject->AsSWSCreature()->m_bUpdateCombatInformation = 1;
    return 1;
}

int CSWSEffectListHandler::OnRemoveAttackDecrease(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    if (pObject->AsSWSCreature() != 0)
        pObject->AsSWSCreature()->m_bUpdateCombatInformation = 1;
    return 1;
}

int CSWSEffectListHandler::OnApplyDamageIncrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pEffect); K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0)
        pObject->AsSWSCreature()->m_bUpdateCombatInformation = 1;
    return 0;
}

int CSWSEffectListHandler::OnRemoveDamageIncrease(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    if (pObject->AsSWSCreature() != 0)
        pObject->AsSWSCreature()->m_bUpdateCombatInformation = 1;
    return 1;
}

int CSWSEffectListHandler::OnRemoveDamageDecrease(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    if (pObject->AsSWSCreature() != 0)
        pObject->AsSWSCreature()->m_bUpdateCombatInformation = 1;
    return 1;
}

// Temporary hit points accumulate in the bonus pool.
int CSWSEffectListHandler::OnApplyTemporaryHitpoints(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pEffect != 0) {
            short nBonus = static_cast<short>(pCreature->m_hitPointBonus);
            pCreature->m_hitPointBonus = nBonus + pEffect->GetInteger(0);
        }
    }
    return 0;
}

int CSWSEffectListHandler::OnRemoveDisarm(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

int CSWSEffectListHandler::OnApplyDamageShield(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect); K2_UNUSED(a);
    return 0;
}

int CSWSEffectListHandler::OnRemoveDamageShield(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

int CSWSEffectListHandler::OnApplyDamageReduction(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect); K2_UNUSED(a);
    return 0;
}

// Detaches the looping visual named by the effect's first integer.
int CSWSEffectListHandler::OnRemoveVisualEffect(CSWSObject* pObject, CGameEffect* pEffect) {
    pObject->RemoveLoopingVisualEffect(static_cast<unsigned short>(pEffect->GetInteger(0)));
    return 1;
}

int CSWSEffectListHandler::OnRemoveBeam(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

int CSWSEffectListHandler::OnApplySpellLevelAbsorption(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect); K2_UNUSED(a);
    return 0;
}

int CSWSEffectListHandler::OnRemoveSpellLevelAbsorption(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

int CSWSEffectListHandler::OnApplySpellImmunity(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect); K2_UNUSED(a);
    return 0;
}

int CSWSEffectListHandler::OnRemoveSpellImmunity(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

// A negative amount is rejected; otherwise the amount folds into the damage type's total.
int CSWSEffectListHandler::OnApplyDamageImmunityIncrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pEffect != 0) {
        int nFlags = pEffect->GetInteger(0);
        int nAmount = pEffect->GetInteger(1);
        if (nAmount < 0)
            return 1;
        pObject->SetDamageImmunity(static_cast<unsigned short>(nFlags), static_cast<char>(pObject->GetDamageImmunityByFlags(static_cast<unsigned short>(nFlags)) + nAmount));
    }
    return 0;
}

// Re-runs the creature's perception pass unless a player switch is already pending.
int CSWSEffectListHandler::OnRemoveEntangled(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    if (pObject != 0 && pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature->m_bPlayerCharacter == 0) {
            pCreature->ClearVisibleList();
            pCreature->UpdateVisibleList(0);
        }
    }
    return 1;
}

// The effect's integer 0 is an AND mask applied to the creature's animation-state word.
int CSWSEffectListHandler::OnApplySetAIState(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pEffect != 0) {
            unsigned short nState = pCreature->m_nAIState;
            pCreature->m_nAIState = nState & pEffect->GetInteger(0);
        }
    }
    return 0;
}

// Plot creatures refuse the bonus; otherwise it accumulates in the stats block.
int CSWSEffectListHandler::OnApplyEnemyAttackBonus(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0) {
        if (pObject->AsSWSCreature()->m_bPlot != 0)
            return 1;
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pEffect != 0) {
            CSWSCreatureStats* pStats = pCreature->m_pStats;
            unsigned char nBonus = pStats->m_nEnemyAttackBonus;
            pStats->m_nEnemyAttackBonus = static_cast<unsigned char>(nBonus + pEffect->GetInteger(0));
        }
    }
    return 0;
}

// Drains levels. Applies the derived penalties, charges them to the creature's highest class,
// and refunds a living caster.
int CSWSEffectListHandler::OnApplyNegativeLevel(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    if (pEffect->GetInteger(0) > 100)
        return 1;
    if (pObject->AsSWSCreature() == 0)
        return 0;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    CSWSCreature* pCreator =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
    if (pCreature->m_pStats->GetEffectImmunity(::IMMUNITY_TYPE_NEGATIVE_LEVEL, pCreator) != 0) {
        CSWCCMessageData* pData = new CSWCCMessageData();
        pData->SetObjectID(0, pObject->m_objectId);
        if (pCreator != 0) {
            CSWCCMessageData* pCasterData = new CSWCCMessageData();
            pData->CopyTo(pCasterData);
            pCreator->SendFeedbackMessage(FEEDBACK_IMMUNE_NEGATIVE_LEVELS, pCasterData);
        }
        pCreature->SendFeedbackMessage(FEEDBACK_IMMUNE_NEGATIVE_LEVELS, pData);
        return 1;
    }
    CGameEffect* pDrain = new CGameEffect(pEffect);
    pDrain->m_subType = GAME_EFFECT_ATTACK_DECREASE;
    pDrain->SetInteger(0, pEffect->GetInteger(0));
    pDrain->SetInteger(1, 0);
    pDrain->SetInteger(2, g_pRules->m_nNumRaces);
    pCreature->ApplyEffect(pDrain, a, 0);
    CGameEffect* pDrain2 = new CGameEffect(pEffect);
    pDrain2->m_subType = GAME_EFFECT_SAVING_THROW_DECREASE;
    pDrain2->SetInteger(0, pEffect->GetInteger(0));
    pDrain2->SetInteger(1, 0);
    pDrain2->SetInteger(3, g_pRules->m_nNumRaces);
    pCreature->ApplyEffect(pDrain2, a, 0);
    CGameEffect* pDrain3 = new CGameEffect(pEffect);
    pDrain3->m_subType = GAME_EFFECT_SKILL_DECREASE;
    pDrain3->SetInteger(0, SKILL_ALL);
    pDrain3->SetInteger(1, pEffect->GetInteger(0));
    pDrain3->SetInteger(2, g_pRules->m_nNumRaces);
    pCreature->ApplyEffect(pDrain3, a, 0);
    if (pCreature->m_pStats->m_numClasses != 0) {
        int nBestClass = 0;
        int nBestLevel = 0;
        for (unsigned char i = 0; i < pCreature->m_pStats->m_numClasses; i++) {
            if (nBestLevel < pCreature->m_pStats->GetClassLevel(i, 0)) {
                nBestLevel = pCreature->m_pStats->GetClassLevel(i, 0);
                nBestClass = i;
            }
        }
        if (nBestLevel > 0) {
            unsigned char nOld = pCreature->m_pStats->GetClassNegativeLevels(static_cast<unsigned char>(nBestClass));
            pCreature->m_pStats->SetClassNegativeLevels(static_cast<unsigned char>(nBestClass), static_cast<unsigned char>(nOld + pEffect->GetInteger(0)));
            pEffect->SetInteger(1, nBestClass);
            if (pCreature->m_pStats->GetLevel(1) == 0) {
                CGameEffect* pDeath = new CGameEffect(1);
                pDeath->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                             (pDeath->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
                pDeath->m_subType = GAME_EFFECT_DEATH;
                pDeath->SetInteger(0, 1);
                pDeath->SetInteger(1, 1);
                pDeath->SetCreator(pEffect->m_creatorId);
                pCreature->ApplyEffect(pDeath, a, 0);
            }
        }
    }
    if (pCreator != 0) {
        CGameEffect* pGain = new CGameEffect(1);
        pGain->m_durationTypeBits =
            CGameEffect::DURATION_TYPE_PERMANENT | (pGain->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pGain->m_subType = GAME_EFFECT_TEMPORARY_HITPOINTS;
        pGain->SetInteger(0, pEffect->GetInteger(0) * 5);
        pCreator->ApplyEffect(pGain, a, 0);
    }
    return 0;
}

// Gives the charged negative levels back to the class the effect recorded.
int CSWSEffectListHandler::OnRemoveNegativeLevel(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        int nClass = pEffect->GetInteger(1);
        int nLevels = pCreature->m_pStats->GetClassNegativeLevels(static_cast<unsigned char>(nClass));
        pCreature->m_pStats->SetClassNegativeLevels(static_cast<unsigned char>(nClass), static_cast<unsigned char>(nLevels - pEffect->GetInteger(0)));
    }
    return 1;
}

// Stamps the world time it was applied so the next tick can be scheduled.
int CSWSEffectListHandler::OnApplyRegenerate(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0) {
        unsigned int nDay;
        unsigned int nTime;
        g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTime);
        pEffect->SetInteger(2, nDay);
        pEffect->SetInteger(3, nTime);
    }
    return 0;
}

int CSWSEffectListHandler::OnRemoveDisease(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    if (pObject != 0 && pObject->AsSWSCreature() != 0)
        pObject->AsSWSCreature()->m_bDiseased = 0;
    return 1;
}

// Only the effect that inflicted the poison may clear the poisoned state.
int CSWSEffectListHandler::OnRemovePoison(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject != 0 && pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature->m_bPoisoned == 1 && pCreature->m_idPoisonEffect == pEffect->m_effectId) {
            pCreature->m_bPoisoned = 0;
            pCreature->m_idPoisonEffect = 0xffffffffffffffffULL;
        }
    }
    return 1;
}

int CSWSEffectListHandler::OnRemoveHasteInternal(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0)
        pCreature->m_bHasted = 0;
    return 1;
}

int CSWSEffectListHandler::OnRemoveSlowInternal(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0)
        pCreature->m_bSlowed = 0;
    return 1;
}

// Applies both linked children in turn and then unlinks them.
int CSWSEffectListHandler::OnApplyLink(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    pEffect->UpdateLinked();
    if (pEffect->m_pLinkedEffect1 != 0)
        pObject->ApplyEffect(pEffect->m_pLinkedEffect1, a, 0);
    if (pEffect->m_pLinkedEffect2 != 0)
        pObject->ApplyEffect(pEffect->m_pLinkedEffect2, a, 0);
    pEffect->SetLinked(0, 0);
    return 1;
}

int CSWSEffectListHandler::OnApplyModifyNumAttacks(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0) {
        CSWSCombatRound* pRound = pCreature->m_pCombatRound;
#if KOTOR1  // refused once above 2 extra attacks, otherwise clamped to 0..2
        int nCurrent = pRound->m_bonusEffectAttacks;
        if (nCurrent > 2)
            return 1;
        int nTotal = pEffect->GetInteger(0) + nCurrent;
        if (nTotal > 2)
            nTotal = 2;
        if (nTotal < 0)
            nTotal = 0;
        pRound->m_bonusEffectAttacks = nTotal;
#else
        int nCurrent = pRound->GetActualBonusEffectAttacks();
        pRound->SetBonusEffectAttacks(pEffect->GetInteger(0) + nCurrent);
#endif
    }
    return 0;
}

int CSWSEffectListHandler::OnRemoveModifyNumAttacks(CSWSObject* pObject, CGameEffect* pEffect) {
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0) {
        CSWSCombatRound* pRound = pCreature->m_pCombatRound;
#if KOTOR1  // the raw count, clamped to 0..2
        int nTotal = pRound->m_bonusEffectAttacks - pEffect->GetInteger(0);
        if (nTotal > 2)
            nTotal = 2;
        if (nTotal < 0)
            nTotal = 0;
        pRound->m_bonusEffectAttacks = nTotal;
#else
        int nCurrent = pRound->GetActualBonusEffectAttacks();
        pRound->SetBonusEffectAttacks(nCurrent - pEffect->GetInteger(0));
#endif
    }
    return 1;
}

int CSWSEffectListHandler::OnRemoveCurse(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

int CSWSEffectListHandler::OnRemoveSilence(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    if (pObject->AsSWSCreature() != 0)
        pObject->AsSWSCreature()->m_bSilenced = 0;
    return 1;
}

int CSWSEffectListHandler::OnApplyInvisibility(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pEffect != 0)
            g_pAppManager->m_pServerExoApp->GetModule()->m_playerList.AddUnique(pCreature->m_objectId);
    }
    return 0;
}

// Sets the see-invisible vision bit.
int CSWSEffectListHandler::OnApplySeeInvisible(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pEffect); K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0)
        pObject->AsSWSCreature()->m_nVisionFlags |= 1;
    return 0;
}

int CSWSEffectListHandler::OnRemoveDarkness(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

// A miss chance outside 1..100 percent is rejected.
int CSWSEffectListHandler::OnApplyMissChance(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pObject); K2_UNUSED(a);
    if (pEffect->GetInteger(0) > 100)
        return 1;
    return pEffect->GetInteger(0) < 1;
}

int CSWSEffectListHandler::OnRemoveMissChance(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

int CSWSEffectListHandler::OnApplyConcealment(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pObject); K2_UNUSED(a);
    if (pEffect->GetInteger(0) > 100)
        return 1;
    return pEffect->GetInteger(0) < 1;
}

int CSWSEffectListHandler::OnRemoveConcealment(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

// Plays the "appear" cutscene. The creature drops what it was doing and queues the appear
// animation chain. The type test is on the object tag, not on the creature type.
int CSWSEffectListHandler::OnApplyAppear(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pEffect); K2_UNUSED(a);
    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        pCreature->ClearAllActions(1);
        pCreature->AddAppearActions();
    }
    return 1;
}

int CSWSEffectListHandler::OnApplyBonusSpellOfLevel(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect); K2_UNUSED(a);
    return 0;
}

int CSWSEffectListHandler::OnRemoveBonusSpellOfLevel(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

// Applies nothing; the creature fetch is the only work this hook does.
int CSWSEffectListHandler::OnApplyEffectImmunity(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pEffect); K2_UNUSED(a);
    pObject->AsSWSCreature();
    return 0;
}

int CSWSEffectListHandler::OnRemoveEffectImmunity(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

int CSWSEffectListHandler::OnApplyArcaneSpellFailure(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pEffect != 0) {
            CSWSCreatureStats* pStats = pCreature->m_pStats;
            if (pObject->AsSWSCreature() != 0)
                pObject->AsSWSCreature()->m_bUpdateCombatInformation = 1;
            unsigned char nFailure = pStats->m_nArcaneSpellFailure;
            pStats->SetArcaneSpellFailure(static_cast<unsigned char>(nFailure + pEffect->GetInteger(0)));
        }
    }
    return 0;
}

int CSWSEffectListHandler::OnRemoveTaunt(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    if (pObject->AsSWSCreature() != 0)
        pObject->AsSWSCreature()->m_bTaunted = 0;
    return 1;
}

int CSWSEffectListHandler::OnRemoveLight(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

// All four integers are read and three discarded; only the rank delta in slot 1 is used.
int CSWSEffectListHandler::OnApplySkillIncrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    pEffect->GetInteger(0);
    int nAmount = pEffect->GetInteger(1);
    pEffect->GetInteger(2);
    pEffect->GetInteger(3);
    if (nAmount < 0)
        return 1;
    return pObject->AsSWSCreature() == 0;
}

int CSWSEffectListHandler::OnRemoveSkillIncrease(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

int CSWSEffectListHandler::OnRemoveSkillDecrease(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

int CSWSEffectListHandler::OnApplySpecialWalkAnimation(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    int nAnimation = pEffect->GetInteger(0);
    if (pObject == 0)
        return 1;
    if (pObject->AsSWSCreature() == 0)
        return 1;
    pObject->AsSWSCreature()->m_nSpecialWalkAnimation = static_cast<unsigned char>(nAnimation);
    return 0;
}

// Plot creatures refuse the cap.
int CSWSEffectListHandler::OnApplyLimitMovementSpeed(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pEffect); K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0) {
        if (pObject->AsSWSCreature()->m_bPlot != 0)
            return 1;
        pObject->AsSWSCreature()->m_bMovementSpeedLimited = 1;
    }
    return 0;
}

// Un-pauses the server and takes the object off the exclusion list.
int CSWSEffectListHandler::OnRemoveTimestop(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    g_pAppManager->m_pServerExoApp->TogglePauseState(1);
    g_pAppManager->m_pServerExoApp->RemoveFromExclusionList(pObject->m_objectId, 1);
    return 1;
}

// The effect only sticks if racialtypes.2da has the row it names.
int CSWSEffectListHandler::OnApplyRacialType(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() == 0)
        return 1;
    unsigned short nRace = static_cast<unsigned short>(pEffect->GetInteger(0));
    return &g_pRules->m_paRaces[nRace] == 0;
}

int CSWSEffectListHandler::OnRemoveRacialType(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

// The visual-effect clone is built and freed without ever being applied.
int CSWSEffectListHandler::OnApplyVision(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0) {
        pEffect->GetInteger(0);
        CGameEffect* pNewEffect = new CGameEffect(pEffect);
        pNewEffect->m_subType = GAME_EFFECT_VISUALEFFECT;
        delete pNewEffect;
    }
    return 1;
}

int CSWSEffectListHandler::OnRemoveVision(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

int CSWSEffectListHandler::OnApplyTurnResistance(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pEffect); K2_UNUSED(a);
    if (pObject != 0) {
        if (pObject->AsSWSCreature() != 0) {
            if (pObject->AsSWSCreature()->m_pStats->m_bIsPC == 0)
                return 0;
        }
    }
    return 1;
}

int CSWSEffectListHandler::OnRemoveTurnResistance(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

// Same as OnRemoveSummonCreature; the summon unsummons itself.
int CSWSEffectListHandler::OnRemoveSummonParty(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject);
    CSWSCreature* pSummon =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->GetObjectID(1));
    if (pSummon != 0)
        pSummon->UnsummonMyself();
    return 1;
}

// Takes the amount off the creature's force pool, clamped at zero.
int CSWSEffectListHandler::OnApplyForceDrain(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    int nAmount = pEffect->GetInteger(0);
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreatureStats* pStats = pObject->AsSWSCreature()->m_pStats;
        unsigned short nCurrent = pStats->m_nCurrentForcePoints;
        short nPool = pStats->m_nTemporaryForcePoints + nCurrent;
        int nLeft = nPool - nAmount;
        if (nLeft < 0)
            nLeft = 0;
        pStats->m_nCurrentForcePoints = static_cast<unsigned short>(nLeft);
    }
    return 1;
}

int CSWSEffectListHandler::OnApplyTemporaryForce(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0) {
        CSWSCreatureStats* pStats = pCreature->m_pStats;
        if (pStats != 0) {
            unsigned short nBonus = pStats->m_nTemporaryForcePoints;
            pStats->m_nTemporaryForcePoints = static_cast<unsigned short>(nBonus + pEffect->GetInteger(0));
        }
    }
    return 0;
}

int CSWSEffectListHandler::OnRemoveTemporaryForce(CSWSObject* pObject, CGameEffect* pEffect) {
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0) {
        CSWSCreatureStats* pStats = pCreature->m_pStats;
        if (pStats != 0) {
            unsigned short nBonus = pStats->m_nTemporaryForcePoints;
            pStats->m_nTemporaryForcePoints = static_cast<unsigned short>(nBonus - pEffect->GetInteger(0));
        }
    }
    return 1;
}

int CSWSEffectListHandler::OnApplyDroidStun(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pEffect); K2_UNUSED(a);
    pObject->AsSWSCreature();
    return 0;
}

int CSWSEffectListHandler::OnRemoveDroidStun(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    pObject->AsSWSCreature();
    return 1;
}

int CSWSEffectListHandler::OnApplyWhirlWind(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pEffect); K2_UNUSED(a);
    pObject->AsSWSCreature();
    return 0;
}

int CSWSEffectListHandler::OnRemoveWhirlWind(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    pObject->AsSWSCreature();
    return 1;
}

int CSWSEffectListHandler::OnApplyBlasterDeflectionIncrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pEffect); K2_UNUSED(a);
    pObject->AsSWSCreature();
    return 0;
}

int CSWSEffectListHandler::OnRemoveBlasterDeflectionIncrease(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    pObject->AsSWSCreature();
    return 1;
}

int CSWSEffectListHandler::OnApplyBlasterDeflectionDecrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pEffect); K2_UNUSED(a);
    pObject->AsSWSCreature();
    return 0;
}

int CSWSEffectListHandler::OnRemoveBlasterDeflectionDecrease(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    pObject->AsSWSCreature();
    return 1;
}

int CSWSEffectListHandler::OnApplyHorrified(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pEffect); K2_UNUSED(a);
    pObject->AsSWSCreature();
    return 0;
}

int CSWSEffectListHandler::OnRemoveHorrified(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    pObject->AsSWSCreature();
    return 1;
}

// Drains the same combined pool as force drain, but an overdraw zeroes the pool outright.
int CSWSEffectListHandler::OnApplyDamageForcePoints(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0 && pCreature->m_pStats != 0) {
        int nDamage = pEffect->GetInteger(0);
        unsigned short nCurrent = pCreature->m_pStats->m_nCurrentForcePoints;
        short nPool = pCreature->m_pStats->m_nTemporaryForcePoints + nCurrent;
        pCreature->m_pStats->m_nCurrentForcePoints = static_cast<unsigned short>(nDamage < nPool ? nPool - nDamage : 0);
    }
    return 1;
}

int CSWSEffectListHandler::OnApplyChoke(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pEffect); K2_UNUSED(a);
    pObject->AsSWSCreature();
    return 0;
}

int CSWSEffectListHandler::OnRemoveChoke(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    pObject->AsSWSCreature();
    return 1;
}

// Flips the creature's "spend hit points instead of force points" latch.
int CSWSEffectListHandler::OnApplyBodyFuel(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pEffect); K2_UNUSED(a);
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0)
        pCreature->m_bBodyFuel = 1;
    return 0;
}

int CSWSEffectListHandler::OnRemoveBodyFuel(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0)
        pCreature->m_bBodyFuel = 0;
    return 1;
}

int CSWSEffectListHandler::OnApplyPsychicStatic(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pEffect); K2_UNUSED(a);
    pObject->AsSWSCreature();
    return 0;
}

int CSWSEffectListHandler::OnRemovePsychicStatic(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    pObject->AsSWSCreature();
    return 1;
}

int CSWSEffectListHandler::OnRemoveLightsaberThrow(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0)
        pCreature->m_bLightsaberThrowActive = 0;
    return 1;
}

// The flag only takes if it is currently clear.
int CSWSEffectListHandler::OnApplyAssuredHit(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject != 0 && pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pEffect != 0) {
            if (pCreature->m_bAssuredHit != 0)
                return 1;
            pCreature->m_bAssuredHit = 1;
            return 0;
        }
    }
    return 1;
}

int CSWSEffectListHandler::OnRemoveAssuredHit(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    if (pObject != 0 && pObject->AsSWSCreature() != 0)
        pObject->AsSWSCreature()->m_bAssuredHit = 0;
    return 1;
}

int CSWSEffectListHandler::OnRemoveForceJump(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

int CSWSEffectListHandler::OnRemoveForceJumpDelayed(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

int CSWSEffectListHandler::OnApplyAssuredDeflection(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject == 0 || pObject->AsSWSCreature() == 0)
        return 1;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature->m_bAssuredDeflectionRedirect != 0)
        return 1;
    if (pCreature->m_bAssuredDeflection != 0)
        return 1;
    if (pEffect == 0)
        return 1;
    pCreature->m_bAssuredDeflection = 1;
    if (pEffect->GetInteger(0) != 0)
        pCreature->m_bAssuredDeflectionRedirect = 1;
    return 0;
}

int CSWSEffectListHandler::OnRemoveAssuredDeflection(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    if (pObject != 0 && pObject->AsSWSCreature() != 0) {
        pObject->AsSWSCreature()->m_bAssuredDeflection = 0;
        pObject->AsSWSCreature()->m_bAssuredDeflectionRedirect = 0;
    }
    return 1;
}

int CSWSEffectListHandler::OnRemoveForceShield(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

int CSWSEffectListHandler::OnRemovePureGoodPowers(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

int CSWSEffectListHandler::OnRemovePureEvilPowers(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

int CSWSEffectListHandler::OnApplyForceBody(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect); K2_UNUSED(a);
    return 0;
}

int CSWSEffectListHandler::OnRemoveForceBody(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

// Fury is a stacking buff, so the stats it parks are only restored when the last one is
// dropped.
#if !KOTOR1  // K2 only
int CSWSEffectListHandler::OnRemoveFury(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject != 0 && pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        int bOtherFury = 0;
        for (int i = 0; i < pCreature->m_effects.m_count; i++) {
            CGameEffect* pOther = pCreature->m_effects.m_pArray[i];
            if (pOther->m_subType == GAME_EFFECT_FURY && pOther != pEffect)
                bOtherFury = 1;
        }
        if (bOtherFury == 0) {
            pCreature->m_pStats->m_nFuryDamageBonus = -1;
            pCreature->m_pStats->m_nVideoEffectType = 0;
        }
    }
    return 1;
}
#endif

int CSWSEffectListHandler::OnApplyFPRegenModifier(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect); K2_UNUSED(a);
    return 0;
}

int CSWSEffectListHandler::OnRemoveFPRegenModifier(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

int CSWSEffectListHandler::OnApplyVPRegenModifier(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect); K2_UNUSED(a);
    return 0;
}

int CSWSEffectListHandler::OnRemoveVPRegenModifier(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

int CSWSEffectListHandler::OnApplyForceSight(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect); K2_UNUSED(a);
    return 0;
}

int CSWSEffectListHandler::OnRemoveForceSight(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pObject); K2_UNUSED(pEffect);
    return 1;
}

// The immunity-gated hooks ask the target's stats whether it is immune to the effect's
// category, passing the effect's creator as the attacker, and refuse the effect when it is.
//
// The `a` parameter is the already-applied-once flag. On a fresh application a dead target,
// or a party member already at zero hit points, refuses the effect.
int CSWSEffectListHandler::OnApplyAbilityIncrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    if (pEffect == 0)
        return 1;
    if (pObject->AsSWSCreature() == 0)
        return 1;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    int nAmount = pEffect->GetInteger(1);
    if (a == 0) {
        if (pCreature->GetDead() != 0)
            return 1;
#if KOTOR1  // a dying PC or a downed party member is skipped
        if (pCreature->GetIsPCDying() != 0)
            return 1;
        if (pCreature->GetDeadTemp() != 0)
            return 1;
#else
        if (pCreature->m_bInParty != 0) {
            if (pCreature->GetCurrentHitPoints(0) < 1)
                return 1;
        }
#endif
    }
    return nAmount < 1;
}

int CSWSEffectListHandler::OnApplyAbilityDecrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    if (pEffect == 0)
        return 1;
    if (pObject->AsSWSCreature() == 0)
        return 1;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    K2_ASSUME(pCreature != 0);
    CSWSCreatureStats* pStats K2_UNSET;
    if (pCreature != 0)
        pStats = pCreature->m_pStats;
    CSWSCreature* pCreator =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
    if (pStats->GetEffectImmunity(::IMMUNITY_TYPE_ABILITY_DECREASE, pCreator) != 0)
        return 1;
    int nAmount = pEffect->GetInteger(1);
    int bDown = 0;
    if (a == 0) {
        if (pCreature->GetDead() != 0)
            return 1;
#if KOTOR1  // a dying PC is skipped; "down" is GetDeadTemp
        if (pCreature->GetIsPCDying() != 0)
            return 1;
        bDown = pCreature->GetDeadTemp();
#else
        if (pCreature->m_bInParty != 0)
            bDown = pCreature->GetCurrentHitPoints(0) < 1;
#endif
    }
    if (nAmount < 1)
        return 1;
    if (bDown != 0)
        return 1;
    return pCreature->m_bPlot != 0;
}

int CSWSEffectListHandler::OnApplySavingThrowDecrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        CSWSCreature* pCreator =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
        if (pCreature->m_pStats->GetEffectImmunity(::IMMUNITY_TYPE_SAVING_THROW_DECREASE, pCreator) != 0)
            return 1;
        if (pEffect->GetInteger(0) < 1)
            return 1;
        if (pCreature->m_bPlot != 0)
            return 1;
        pCreature->m_bUpdateCombatInformation = 1;
    }
    return 0;
}

int CSWSEffectListHandler::OnApplyAttackDecrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pEffect != 0 && pCreature != 0) {
        if (pEffect->GetInteger(0) < 1)
            return 1;
        if (pObject->m_bPlot != 0)
            return 1;
        CSWSCreature* pCreator =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
        if (pObject->AsSWSCreature()->m_pStats->GetEffectImmunity(::IMMUNITY_TYPE_ATTACK_DECREASE,
                                                                  pCreator) != 0)
            return 1;
        pObject->AsSWSCreature()->m_bUpdateCombatInformation = 1;
    }
    return 0;
}

int CSWSEffectListHandler::OnApplyDamageImmunityDecrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pEffect == 0)
        return 0;
    int nFlags = pEffect->GetInteger(0);
    int nAmount = pEffect->GetInteger(1);
    if (nAmount < 0)
        return 1;
    if (pObject->m_bPlot != 0)
        return 1;
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreator =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
        if (pObject->AsSWSCreature()->m_pStats->GetEffectImmunity(::IMMUNITY_TYPE_DAMAGE_IMMUNITY_DECREASE,
                                                                  pCreator) != 0)
            return 1;
    }
    pObject->SetDamageImmunity(static_cast<unsigned short>(nFlags), static_cast<char>(pObject->GetDamageImmunityByFlags(static_cast<unsigned short>(nFlags)) - nAmount));
    return 0;
}

// Re-totals every live immunity effect for the same damage flag; the departing amount comes
// off at the end.
int CSWSEffectListHandler::OnRemoveDamageImmunityIncrease(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pEffect != 0) {
        int nFlags = pEffect->GetInteger(0);
        int nAmount = pEffect->GetInteger(1);
        pObject->GetDamageImmunityByFlags(static_cast<unsigned short>(nFlags));
        int64_t i = 0;
        int nTotal = 0;
        while (i < pObject->m_effects.m_count) {
            CGameEffect* pOther = pObject->m_effects.m_pArray[i];
            if ((pOther->m_subType == GAME_EFFECT_DAMAGE_IMMUNITY_INCREASE && pOther->GetInteger(0) == nFlags) ||
                (pOther->m_subType == GAME_EFFECT_DAMAGE_IMMUNITY_DECREASE && pOther->GetInteger(0) == nFlags)) {
                if (pOther->m_subType == GAME_EFFECT_DAMAGE_IMMUNITY_INCREASE)
                    nTotal = pOther->GetInteger(1) + nTotal;
                else
                    nTotal = nTotal - pOther->GetInteger(1);
            }
            i++;
        }
        pObject->SetDamageImmunity(static_cast<unsigned short>(nFlags), static_cast<char>(nTotal - nAmount));
    }
    return 1;
}

int CSWSEffectListHandler::OnRemoveDamageImmunityDecrease(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pEffect != 0) {
        int nFlags = pEffect->GetInteger(0);
        int nAmount = pEffect->GetInteger(1);
        pObject->GetDamageImmunity(static_cast<unsigned char>(nFlags));
        int64_t i = 0;
        int nTotal = 0;
        while (i < pObject->m_effects.m_count) {
            CGameEffect* pOther = pObject->m_effects.m_pArray[i];
            if ((pOther->m_subType == GAME_EFFECT_DAMAGE_IMMUNITY_INCREASE && pOther->GetInteger(0) == nFlags) ||
                (pOther->m_subType == GAME_EFFECT_DAMAGE_IMMUNITY_DECREASE && pOther->GetInteger(0) == nFlags)) {
                if (pOther->m_subType == GAME_EFFECT_DAMAGE_IMMUNITY_INCREASE)
                    nTotal = pOther->GetInteger(1) + nTotal;
                else
                    nTotal = nTotal - pOther->GetInteger(1);
            }
            i++;
        }
        pObject->SetDamageImmunity(static_cast<unsigned short>(nFlags), static_cast<char>(nTotal - nAmount));
    }
    return 1;
}

// Releases the paused combat round; a living creature gets up and becomes commandable again.
int CSWSEffectListHandler::OnRemoveKnockdown(CSWSObject* pObject, CGameEffect* pEffect) {
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pEffect != 0 && pCreature != 0) {
        if (pCreature->m_pCombatRound->m_bRoundPaused != 0) {
            pCreature->m_pCombatRound->SetRoundPaused(0, OBJECT_INVALID);
            pCreature->m_pCombatRound->SetPauseTimer(0, 0);
        }
        if (pCreature->GetDead() == 0) {
#if KOTOR1  // the dying tests are GetIsPCDying() / GetDeadTemp()
            if (pCreature->GetIsPCDying() == 0 && pCreature->GetDeadTemp() == 0) {
#else
            if (pCreature->m_bInParty == 0 || pCreature->GetCurrentHitPoints(0) >= 1) {
#endif
                pCreature->SetAnimation(SERVER_ANIMATION_READY);
                pObject->m_bCommandable = 1;
            }
        }
    }
    return 1;
}

// Rebuilds the animation-state mask from every live SET_AI_STATE effect.
int CSWSEffectListHandler::OnRemoveSetAIState(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature->m_nAIState != 0) {
            unsigned short nMask = 0xffff;
            if (pCreature->m_effects.m_count >= 1) {
                for (int i = pCreature->m_pStats->m_iFirstSetAIState;
                     i < pCreature->m_effects.m_count; i++) {
                    CGameEffect* pOther = pCreature->m_effects.m_pArray[i];
                    if (pOther->m_subType != GAME_EFFECT_SET_AI_STATE)
                        break;
                    if (pOther != pEffect)
                        nMask = nMask & pOther->GetInteger(0);
                }
            }
            pCreature->m_nAIState = nMask;
        }
    }
    return 1;
}

// Rebuilds the byte by totalling every other live effect of the same kind.
int CSWSEffectListHandler::OnRemoveEnemyAttackBonus(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        int64_t i = 0;
        int nTotal = 0;
        if (pEffect != 0) {
            while (i < pCreature->m_effects.m_count) {
                CGameEffect* pOther = pCreature->m_effects.m_pArray[i];
                if (pOther->m_subType > GAME_EFFECT_ENEMY_ATTACK_BONUS)
                    break;
                if (pOther != pEffect && pOther->m_subType == GAME_EFFECT_ENEMY_ATTACK_BONUS)
                    nTotal = pOther->GetInteger(0) + nTotal;
                i++;
            }
        }
        pCreature->m_pStats->m_nEnemyAttackBonus = static_cast<unsigned char>(nTotal);
    }
    return 1;
}

// The percentage is rebased by adding 100 and applied as a multiplier on the rate factor.
int CSWSEffectListHandler::OnApplyMovementSpeedIncrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pEffect->GetInteger(0) <= 99)
            pEffect->SetInteger(0, pEffect->GetInteger(0) + 100);
        float fScale = pEffect->GetInteger(0) / 100.0f;
        pCreature->SetMovementRateFactor(pCreature->GetMovementRateFactor(0) * fScale);
    }
    return 0;
}

// Rebuilds the rate factor from 1.0 over every live speed effect.
int CSWSEffectListHandler::OnRemoveMovementSpeedIncrease(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        pObject->AsSWSCreature();
        if (pEffect != 0) {
            CSWSCreature* pCreature = pObject->AsSWSCreature();
            int64_t i = 0;
            float fFactor = 1.0f;
            while (i < pCreature->m_effects.m_count) {
                CGameEffect* pOther = pCreature->m_effects.m_pArray[i];
                if (pOther->m_subType > GAME_EFFECT_MOVEMENT_SPEED_DECREASE)
                    break;
                if (pOther->m_effectId != pEffect->m_effectId) {
                    if (pOther->m_subType == GAME_EFFECT_MOVEMENT_SPEED_INCREASE)
                        fFactor = fFactor + pOther->GetInteger(0) / 100.0f;
                    else if (pOther->m_subType == GAME_EFFECT_MOVEMENT_SPEED_DECREASE)
                        fFactor = fFactor + pOther->GetInteger(0) / -100.0f;
                }
                i++;
            }
            pCreature->SetMovementRateFactor(fFactor);
        }
    }
    return 1;
}

// Immunity-gated and refused for plot creatures; the percentage is rebased so that 50 halves
// the speed.
int CSWSEffectListHandler::OnApplyMovementSpeedDecrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        CSWSCreature* pCreator =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
        if (pCreature->m_pStats->GetEffectImmunity(::IMMUNITY_TYPE_MOVEMENT_SPEED_DECREASE, pCreator) != 0)
            return 1;
        if (pEffect->GetInteger(0) > 99)
            return 1;
        if (pCreature->m_bPlot != 0)
            return 1;
        float fScale = pEffect->GetInteger(0) / -100.0f + 1.0f;
        pCreature->SetMovementRateFactor(pCreature->GetMovementRateFactor(0) * fScale);
        return 0;
    }
    return 0;
}

int CSWSEffectListHandler::OnRemoveMovementSpeedDecrease(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        pObject->AsSWSCreature();
        if (pEffect != 0) {
            CSWSCreature* pCreature = pObject->AsSWSCreature();
            int64_t i = 0;
            float fFactor = 1.0f;
            while (i < pCreature->m_effects.m_count) {
                CGameEffect* pOther = pCreature->m_effects.m_pArray[i];
                if (pOther->m_subType > GAME_EFFECT_MOVEMENT_SPEED_DECREASE)
                    break;
                if (pOther->m_effectId != pEffect->m_effectId) {
                    if (pOther->m_subType == GAME_EFFECT_MOVEMENT_SPEED_INCREASE)
                        fFactor = fFactor + pOther->GetInteger(0) / 100.0f;
                    else if (pOther->m_subType == GAME_EFFECT_MOVEMENT_SPEED_DECREASE)
                        fFactor = fFactor + pOther->GetInteger(0) / -100.0f;
                }
                i++;
            }
            pCreature->SetMovementRateFactor(fFactor);
        }
    }
    return 1;
}

// Destroys the spawned effect object if it is still in the object array.
int CSWSEffectListHandler::OnRemoveAreaOfEffect(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        pObject->AsSWSCreature();
        unsigned long long oidAreaOfEffect = pEffect->GetObjectID(0);
        CGameObject* pAreaOfEffect;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(
                oidAreaOfEffect, &pAreaOfEffect) == CGameObjectArray::SUCCESS &&
            pAreaOfEffect != 0) {
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, oidAreaOfEffect, oidAreaOfEffect,
                CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
        }
    }
    return 1;
}

// Attaches only to objects that can carry a visual; the effect is cloned as a visual-effect and
// that clone is what gets applied.
int CSWSEffectListHandler::OnApplyBeam(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE ||
        pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE ||
#if KOTOR1  // no beams on triggers
        pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR) {
#else
        pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR ||
        pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER) {
#endif
        CGameEffect* pVisual = new CGameEffect(pEffect);
        pVisual->m_subType = GAME_EFFECT_VISUALEFFECT;
        pVisual->SetObjectID(0, pEffect->GetObjectID(0));
        pVisual->SetInteger(0, pEffect->GetInteger(0));
        pVisual->SetInteger(1, pEffect->GetInteger(1));
        pVisual->SetInteger(2, pEffect->GetInteger(2));
        pObject->ApplyEffect(pVisual, a, 0);
    }
    return 0;
}

// Capped, and a negative amount is rejected; only an improvement is stored.
int CSWSEffectListHandler::OnApplyForceResistanceIncrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreatureStats* pStats = pObject->AsSWSCreature()->m_pStats;
        int nAmount = pEffect->GetInteger(0);
        if (nAmount > 128)
            nAmount = 128;
        else if (nAmount < 0)
            return 1;
        if (pObject->AsSWSCreature() != 0)
            pObject->AsSWSCreature()->m_bUpdateCombatInformation = 1;
        if (nAmount > pStats->GetForceResistance())
            pStats->SetForceResistance(static_cast<char>(nAmount));
    }
    return 0;
}

int CSWSEffectListHandler::OnApplyForceResistanceDecrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreatureStats* pStats = pObject->AsSWSCreature()->m_pStats;
        CSWSCreature* pCreator =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
        if (pStats->GetEffectImmunity(::IMMUNITY_TYPE_FORCE_RESISTANCE_DECREASE, pCreator) != 0)
            return 1;
        int nAmount = pEffect->GetInteger(0);
        if (nAmount < 0)
            return 1;
        if (pObject->m_bPlot != 0)
            return 1;
        int nPenalty = nAmount < 128 ? nAmount : 128;
        if (pObject->AsSWSCreature() != 0)
            pObject->AsSWSCreature()->m_bUpdateCombatInformation = 1;
        if (nPenalty >= pStats->m_forceResistancePenalty)
            pStats->SetForceResistancePenalty(static_cast<char>(nPenalty));
        return 0;
    }
    return 0;
}

// Applies the movement and combat effects and arms the haste/slow latches.
int CSWSEffectListHandler::OnApplyHasteInternal(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0) {
        CGameEffect* pSpeed = new CGameEffect(pEffect);
        pSpeed->m_subType = GAME_EFFECT_MOVEMENT_SPEED_INCREASE;
        pSpeed->SetInteger(0, 150);
        pObject->ApplyEffect(pSpeed, a, 0);

        CGameEffect* pCombat = new CGameEffect(pEffect);
        pCombat->SetInteger(0, 0);
        pCombat->SetInteger(1, 4);
        pCombat->SetInteger(2, g_pRules->m_nNumRaces);
        pCombat->SetInteger(5, DAMAGE_TYPES_PHYSICAL);
        pCombat->m_subType = GAME_EFFECT_AC_INCREASE;
        pObject->ApplyEffect(pCombat, a, 0);

        pCreature->m_bHasted = 1;
        pCreature->m_bSlowed = 0;
    }
    return 0;
}

// The vision bit stays set while any other see-invisible effect is live.
int CSWSEffectListHandler::OnRemoveSeeInvisible(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        int nFlags = pCreature->m_nVisionFlags;
        nFlags &= (0xff & ~CSWSCreature::VISION_SEE_INVISIBLE);
        for (int i = pCreature->m_pStats->m_iFirstSeeInvisible;
             i < pCreature->m_effects.m_count; i++) {
            CGameEffect* pOther = pCreature->m_effects.m_pArray[i];
            if (pOther->m_subType > GAME_EFFECT_SEEINVISIBLE)
                break;
            if (pOther->m_subType == GAME_EFFECT_SEEINVISIBLE)
                nFlags = nFlags | (pOther != pEffect);
        }
        pCreature->m_nVisionFlags = static_cast<unsigned char>(nFlags);
    }
    return 1;
}

// The dispatch entry points bounds-check the effect's subtype, index the handler table and
// tail-call through it. The bounds are asymmetric, so subtype 0 is rejected by one and
// accepted by the other.
int CSWSEffectListHandler::OnEffectApplied(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    if (pEffect->m_subType >= 1 && pEffect->m_subType <= GAME_EFFECT_LAST
        && m_paOnApply[pEffect->m_subType] != 0)
        return (this->*m_paOnApply[pEffect->m_subType])(pObject, pEffect, a);
    return 1;
}

int CSWSEffectListHandler::OnEffectRemoved(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pEffect->m_subType <= GAME_EFFECT_LAST && m_paOnRemove[pEffect->m_subType] != 0)
        return (this->*m_paOnRemove[pEffect->m_subType])(pObject, pEffect);
    return 1;
}

// Arms the creature's damage-table recompute; immunity to the effect's category rejects it.
int CSWSEffectListHandler::OnApplyDamageDecrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreator =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
        if (pObject->AsSWSCreature()->m_pStats->GetEffectImmunity(::IMMUNITY_TYPE_DAMAGE_DECREASE,
                                                                  pCreator) != 0)
            return 1;
        pObject->AsSWSCreature()->m_bUpdateCombatInformation = 1;
    }
    return 0;
}

// The first surviving walk-animation effect wins; the field falls back to zero when the
// removed effect was the only one.
int CSWSEffectListHandler::OnRemoveSpecialWalkAnimation(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject != 0 && pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        unsigned char nAnimation = 0;
        for (int i = 0; i < pCreature->m_effects.m_count; i++) {
            CGameEffect* pOther = pCreature->m_effects.m_pArray[i];
            if (pOther != pEffect) {
                if (pOther->m_subType == GAME_EFFECT_SETWALKANIMATION) {
                    nAnimation = static_cast<unsigned char>(pOther->GetInteger(0));
                    break;
                }
                if (pOther->m_subType > GAME_EFFECT_SETWALKANIMATION)
                    break;
            }
        }
        pCreature->m_nSpecialWalkAnimation = nAnimation;
    }
    return 1;
}

// Spends the effect and restores the creature's own appearance.
int CSWSEffectListHandler::OnRemoveDisguise(CSWSObject* pObject, CGameEffect* pEffect) {
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0) {
        if (pCreature->GetDead() == 0 || pCreature->m_bPlayerCharacter != 0 ||
            pCreature->m_bDestroyable == 0) {
            if (pEffect->GetInteger(1) == 0) {
                pEffect->SetInteger(1, 1);
                pEffect->m_durationTypeBits =
                    CGameEffect::DURATION_TYPE_TEMPORARY | (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
                pEffect->m_fDuration = 0.0f;
            }
            pCreature->UnDisguise(pEffect);
        }
    }
    return 1;
}

// The cap stays armed while any other effect of the same kind survives.
int CSWSEffectListHandler::OnRemoveLimitMovementSpeed(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        int bStillLimited = 0;
        for (int i = 0; i < pCreature->m_effects.m_count; i++) {
            CGameEffect* pOther = pCreature->m_effects.m_pArray[i];
            if (pOther->m_subType > GAME_EFFECT_LIMIT_MOVEMENT_SPEED)
                break;
            if (pOther != pEffect && pOther->m_subType == GAME_EFFECT_LIMIT_MOVEMENT_SPEED) {
                bStillLimited = 1;
                break;
            }
        }
        pObject->AsSWSCreature()->m_bMovementSpeedLimited = bStillLimited;
    }
    return 1;
}

// Applies a fresh feedback effect carrying the "power fizzles" string.
int CSWSEffectListHandler::OnApplyForceFizzle(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pEffect);
    CGameEffect* pFizzle = new CGameEffect(1);
    pFizzle->m_subType = GAME_EFFECT_VISUALEFFECT;
    pFizzle->m_durationTypeBits =
        CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pFizzle->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pFizzle->SetInteger(0, VFX_COM_JEDI_FORCE_FIZZLE);
    pFizzle->SetCreator(pObject->m_objectId);
    pObject->ApplyEffect(pFizzle, a, 0);
    return 1;
}

// Strips any existing shield, looks the visual up in forceshields.2da, and applies the visual
// and the shield state.
int CSWSEffectListHandler::OnApplyForceShield(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject == 0 || pObject->AsSWSCreature() == 0 || pEffect == 0)
        return 0;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    unsigned short nHeadId = pCreature->m_appearance.m_headId;
    if (pCreature->GetClientCreature() != 0) {
        if (pCreature->GetClientCreature()->GetForceShield() != 0) {
            CGameEffect** ppEffect = pCreature->m_effects.m_pArray;
            CGameEffect* pOld;
            do {
                pOld = *ppEffect++;
            } while (pOld->m_subType != GAME_EFFECT_FORCESHIELD);
            pCreature->RemoveEffectById(pOld->m_effectId);
        }
    }
    CExoString sRow(pEffect->GetInteger(0));
    C2DA* pForceShields = g_pRules->m_p2DArrays->m_pForceShields;
    int n = 0;
    int nVisualEffect = 0;
    CExoString sColumn("VisualEffectDef");
    if (pForceShields->GetINTEntry(sRow, CExoString("Appearance_01"), &n) && n == nHeadId) {
            sColumn = "VisualEffect_01";
            goto noMoreAppearance;
    }
    if (pForceShields->GetINTEntry(sRow, CExoString("Appearance_02"), &n) && n == nHeadId) {
            sColumn = "VisualEffect_02";
            goto noMoreAppearance;
    }
    if (pForceShields->GetINTEntry(sRow, CExoString("Appearance_03"), &n) && n == nHeadId) {
            sColumn = "VisualEffect_03";
            goto noMoreAppearance;
    }
    if (pForceShields->GetINTEntry(sRow, CExoString("Appearance_04"), &n) && n == nHeadId) {
            sColumn = "VisualEffect_04";
    }
noMoreAppearance:
    pForceShields->GetINTEntry(sRow, sColumn, &nVisualEffect);
    int nDamageFlags = 0;
    int nVulnerFlags = 0;
    int nResistance = 0;
    int nAmount = 0;
    int nPermanent = 0;
    pForceShields->GetINTEntry(sRow, CExoString("DamageFlags"), &nDamageFlags);
    pForceShields->GetINTEntry(sRow, CExoString("VulnerFlags"), &nVulnerFlags);
    pForceShields->GetINTEntry(sRow, CExoString("Resistance"), &nResistance);
    pForceShields->GetINTEntry(sRow, CExoString("Amount"), &nAmount);
    pForceShields->GetINTEntry(sRow, CExoString("Permanent"), &nPermanent);

    CGameEffect* pVisual = new CGameEffect(pEffect);
    pVisual->SetCreator(pObject->m_objectId);
    pVisual->m_subType = GAME_EFFECT_VISUALEFFECT;
    pVisual->SetInteger(0, nVisualEffect);

    CGameEffect* pShield = new CGameEffect(pEffect);
    pShield->SetCreator(pObject->m_objectId);
    pShield->m_subType = GAME_EFFECT_DAMAGE_RESISTANCE;
    pShield->m_durationTypeBits = (pShield->m_durationTypeBits & ~SUBTYPE_SUPERNATURAL) | SUBTYPE_MAGICAL;
    pShield->SetInteger(0, nDamageFlags);
    pShield->SetInteger(1, nResistance);
    pShield->SetInteger(2, nAmount);
    pShield->SetInteger(3, nVulnerFlags);
#if !KOTOR1  // K1 leaves the shield's integer 4 at 0
    pShield->SetInteger(4, 1);
#endif

    pObject->ApplyEffect(pVisual, 0, 0);
    pObject->ApplyEffect(pShield, 0, 0);
    return 0;
}

// Applies Force Fury. Clears the stats block's fury latch and re-stamps its tier from the
// spell that cast it.
#if !KOTOR1  // K2 only
int CSWSEffectListHandler::OnApplyFury(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject != 0 && pObject->AsSWSCreature() != 0) {
        CSWSCreatureStats* pStats = pObject->AsSWSCreature()->m_pStats;
        pStats->m_nFuryDamageBonus = 0;
        switch (pEffect->m_spellId) {
        case FORCE_POWER_FURY:
        case FORCE_POWER_WOOKIEE_RAGE_I:
            pStats->m_nVideoEffectType = 7;
            break;
        case FORCE_POWER_IMPROVED_FURY:
        case FORCE_POWER_WOOKIEE_RAGE_II:
            pStats->m_nVideoEffectType = 8;
            break;
        case FORCE_POWER_MASTER_FURY:
        case FORCE_POWER_WOOKIEE_RAGE_III:
            pStats->m_nVideoEffectType = 9;
            break;
        }
    }
    return 0;
}
#endif

// Applies nothing itself; the clone is retagged as a feedback-string effect.
int CSWSEffectListHandler::OnApplyLight(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    if (pObject->AsSWSCreature() != 0) {
        CGameEffect* pFeedback = new CGameEffect(pEffect);
        pFeedback->m_subType = GAME_EFFECT_VISUALEFFECT;
        pFeedback->SetInteger(0, pEffect->GetInteger(0));
        pObject->AsSWSCreature()->ApplyEffect(pFeedback, a, 0);
    }
    return 0;
}

// Runs the state machine the fresh setstate effect arms into. Every state rebuilds its own
// derived effect set from scratch.
int CSWSEffectListHandler::OnApplySetStateInternal(CSWSObject* pObject, CGameEffect* pEffect,
                                                   int nParam) {
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    int nFeedbackState;
    bool bFeedback = true;
    if (pCreature != 0 && pEffect != 0) {
        int nState = pEffect->GetInteger(0);
        pCreature->m_nLastHeartbeatDay = 0;
        pCreature->m_nLastHeartbeatTod = 1;
        if (nState == 9) {
            CGameEffect* pState = new CGameEffect(pEffect);
            pState->m_subType = GAME_EFFECT_SET_AI_STATE;
            pState->SetInteger(0, ~CSWSCreature::AI_STATE_MOBILE);
            pCreature->ApplyEffect(pState, nParam, 0);
            pCreature->RecomputeAmbientAnimationState();
            nFeedbackState = 9;
        } else {
            nFeedbackState = nState;
            pCreature->m_bCommandable = 1;
            pCreature->ClearAllActions(1);
            pCreature->m_pCombatRound->RemoveAllActions();
            pCreature->m_bCommandable = 0;
            bFeedback = false;
            CGameEffect* pApply = 0;
            switch (nState) {
            case EFFECT_STATE_STUNNED:
#if !KOTOR1  // K1 lacks states 18/19 and 14/15/17
            case EFFECT_STATE_MIND_TRICK:
            case EFFECT_STATE_DROID_SCRAMBLE:
#endif
                pApply = new CGameEffect(pEffect);
                pApply->m_subType = GAME_EFFECT_SET_AI_STATE;
                unsigned int nExpireTod;
                unsigned int nExpireDay;
                pApply->SetInteger(0, ~CSWSCreature::AI_STATE_MOBILE);
                pEffect->GetExpiryTime(&nExpireDay, &nExpireTod);
                pApply->SetExpiryTime(nExpireDay, nExpireTod);
                // The carrier is applied after the expiry copy.
                pCreature->ApplyEffect(pApply, nParam, 0);
                pApply = new CGameEffect(pEffect);
                pApply->m_subType = GAME_EFFECT_VISUALEFFECT;
                pApply->SetInteger(0, VFX_DUR_STUN);
                pApply->SetExpiryTime(nExpireDay, nExpireTod);
                break;
#if !KOTOR1
            case EFFECT_STATE_KNOCKDOWN:
            case EFFECT_STATE_FORCE_CRUSH:
            case 17:
                pApply = new CGameEffect(pEffect);
                pApply->m_subType = GAME_EFFECT_SET_AI_STATE;
                pApply->SetInteger(0, ~CSWSCreature::AI_STATE_MOBILE);
                break;
#endif
            case EFFECT_STATE_FRIGHTENED: {
                CGameEffect* pSubAttack = new CGameEffect(pEffect);
                pSubAttack->m_subType = GAME_EFFECT_SAVING_THROW_DECREASE;
                nFeedbackState = EFFECT_STATE_FRIGHTENED;
                pSubAttack->SetInteger(0, 2);
                pSubAttack->SetInteger(1, 0);
                pSubAttack->SetInteger(3, g_pRules->m_nNumRaces);
                pCreature->ApplyEffect(pSubAttack, nParam, 0);
                break;
            }
            case EFFECT_STATE_DROID_STUN: {
                CGameEffect* pState = new CGameEffect(pEffect);
                pState->m_subType = GAME_EFFECT_VISUALEFFECT;
                pState->SetInteger(0, VFX_PRO_DROID_DISABLE);
                pCreature->ApplyEffect(pState, nParam, 0);
                pState = new CGameEffect(pEffect);
                pState->m_subType = GAME_EFFECT_SAVING_THROW_DECREASE;
                pState->SetInteger(0, 2);
                pState->SetInteger(1, 0);
                pState->SetInteger(3, g_pRules->m_nNumRaces);
                pCreature->ApplyEffect(pState, nParam, 0);
                pState = new CGameEffect(pEffect);
                pState->m_subType = GAME_EFFECT_SET_AI_STATE;
                pState->SetInteger(0, ~CSWSCreature::AI_STATE_MOBILE);
                pCreature->ApplyEffect(pState, nParam, 0);
                pCreature->RecomputeAmbientAnimationState();
                nFeedbackState = EFFECT_STATE_DROID_STUN;
                break;
            }
            case EFFECT_STATE_PARALYZED: {
                CGameEffect* pState = new CGameEffect(pEffect);
                pState->m_subType = GAME_EFFECT_SET_AI_STATE;
                pState->SetInteger(0, -0xf);
                pCreature->ApplyEffect(pState, nParam, 0);
                pCreature->RecomputeAmbientAnimationState();
                nFeedbackState = EFFECT_STATE_PARALYZED;
                break;
            }
            case EFFECT_STATE_SLEEP: {
                CGameEffect* pState = new CGameEffect(pEffect);
                pState->m_subType = GAME_EFFECT_SET_AI_STATE;
                pState->SetInteger(0, -0x15f);
                pCreature->ApplyEffect(pState, nParam, 0);
                pCreature->RecomputeAmbientAnimationState();
                nFeedbackState = EFFECT_STATE_SLEEP;
                break;
            }
            case EFFECT_STATE_CHOKE: {
                CGameEffect* pState = new CGameEffect(pEffect);
                pState->m_subType = GAME_EFFECT_SET_AI_STATE;
                pState->SetInteger(0, ~CSWSCreature::AI_STATE_MOBILE);
                pCreature->ApplyEffect(pState, nParam, 0);
                pCreature->RecomputeAmbientAnimationState();
                nFeedbackState = EFFECT_STATE_CHOKE;
                break;
            }
            case EFFECT_STATE_HORRIFIED: {
                CGameEffect* pState = new CGameEffect(pEffect);
                pState->m_subType = GAME_EFFECT_SET_AI_STATE;
                pState->SetInteger(0, ~CSWSCreature::AI_STATE_MOBILE);
                pCreature->ApplyEffect(pState, nParam, 0);
                pCreature->RecomputeAmbientAnimationState();
                nFeedbackState = EFFECT_STATE_HORRIFIED;
                break;
            }
            case EFFECT_STATE_WHIRLWIND: {
                CGameEffect* pState = new CGameEffect(pEffect);
                pState->m_subType = GAME_EFFECT_SET_AI_STATE;
                pState->SetInteger(0, ~CSWSCreature::AI_STATE_MOBILE);
                pCreature->ApplyEffect(pState, nParam, 0);
                pCreature->RecomputeAmbientAnimationState();
                nFeedbackState = EFFECT_STATE_WHIRLWIND;
                break;
            }
            }
            if (pApply != 0) {
                pCreature->ApplyEffect(pApply, nParam, 0);
                pCreature->RecomputeAmbientAnimationState();
            }
        }
    }
#ifdef K2_FIX_ENGINE_BUGS
    else
        return 0;
#endif
    CSWSCreature* pCaster =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
    if (bFeedback && pCaster != 0) {
        CSWSCombatAttackData* pAttack =
            pCaster->m_pCombatRound->GetAttack(pCaster->m_pCombatRound->m_currentAttack);
        if (pAttack != 0) {
            pAttack->m_effect.m_bHasApplicationData = 1;
            pAttack->m_effect.m_nFeedbackEffectType = static_cast<unsigned char>(nFeedbackState);
            pCaster->SetAttackSummaryFeedbackEffectData(nFeedbackState, pCreature->m_objectId);
        }
    }
    return 0;
}

// What has to be re-run when a state ends depends on the state id. Most fire the end-of-round
// script and recompute the ambient animation, two only recompute, three do neither.
int CSWSEffectListHandler::OnRemoveSetStateInternal(CSWSObject* pObject, CGameEffect* pEffect) {
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pEffect != 0 && pCreature != 0) {
        switch (pEffect->GetInteger(0)) {
        case EFFECT_STATE_CONFUSED:
        case EFFECT_STATE_DROID_STUN:
        case EFFECT_STATE_STUNNED:
        case EFFECT_STATE_PARALYZED:
        case EFFECT_STATE_SLEEP:
        case EFFECT_STATE_CHOKE:
        case EFFECT_STATE_HORRIFIED:
        case 9:
        case EFFECT_STATE_WHIRLWIND:
#if !KOTOR1  // K1 has no states 14..19
        case EFFECT_STATE_KNOCKDOWN:
        case EFFECT_STATE_FORCE_CRUSH:
        case EFFECT_STATE_DROID_CONFUSED:
        case 17:
        case EFFECT_STATE_MIND_TRICK:
        case EFFECT_STATE_DROID_SCRAMBLE:
#endif
            g_pVirtualMachine->RunScript(&pCreature->m_scripts[CSWSCreature::CREATURE_SCRIPT_ENDROUND],
                                         pCreature->m_objectId, 1);
            // Falls through, since every scripted state also re-derives its ambient animation.
        case 12:
        case EFFECT_STATE_KNOCKBACK:
            pCreature->RecomputeAmbientAnimationState();
            break;
        }
        if (pCreature->m_bPlayerCharacter == 0) {
            pCreature->ClearVisibleList();
            pCreature->UpdateVisibleList(0);
        }
    }
    return 1;
}

// A creature plays it out through its action queue; a placeable is destroyed after a delay.
int CSWSEffectListHandler::OnApplyDisappear(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        pCreature->ClearAllActions(1);
        pCreature->AddDisappearActions(0);
    } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
            0, 2000, pEffect->m_creatorId, pObject->m_objectId,
            CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
    }
    return 1;
}

// Drops every live shield-flagged damage-resistance effect.
int CSWSEffectListHandler::OnApplyDestroyShields(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pEffect); K2_UNUSED(a);
    if (pObject->m_effects.m_count >= 1) {
        int i;
        if (pObject->AsSWSCreature() != 0)
            i = pObject->AsSWSCreature()->m_pStats->m_iFirstDamageResistance;
        else
            i = 0;
        for (; i < pObject->m_effects.m_count; i++) {
            CGameEffect* pOther = pObject->m_effects.m_pArray[i];
            if (pOther->m_subType > GAME_EFFECT_DAMAGE_RESISTANCE)
                break;
            if (pOther->m_subType == GAME_EFFECT_DAMAGE_RESISTANCE && pOther->GetInteger(4) == 1)
                pObject->RemoveEffectById(pOther->m_effectId);
        }
    }
    return 1;
}

// Releases the forced facing and applies a fresh get-up effect.
int CSWSEffectListHandler::OnRemoveForcePush(CSWSObject* pObject, CGameEffect* pEffect) {
    K2_UNUSED(pEffect);
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0) {
        pCreature->SetLockOrientationToObject(OBJECT_INVALID, 0);
        CGameEffect* pRecover = new CGameEffect(1);
        pRecover->m_durationTypeBits =
            CGameEffect::DURATION_TYPE_TEMPORARY | (pRecover->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pRecover->m_fDuration = 2.55f;
        pRecover->SetCreator(pRecover->m_creatorId);
        pRecover->m_subType = GAME_EFFECT_SET_AI_STATE;
        pRecover->SetInteger(0, -0x15f);
        pCreature->ApplyEffect(pRecover, 0, 0);
    }
    return 1;
}

// All four integers are read; only the amount decides whether the effect is refused.
int CSWSEffectListHandler::OnApplySkillDecrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    pEffect->GetInteger(0);
    int nAmount = pEffect->GetInteger(1);
    pEffect->GetInteger(2);
    pEffect->GetInteger(3);
    if (nAmount < 0)
        return 1;
    if (pObject->m_bPlot != 0)
        return 1;
    if (pObject->AsSWSCreature() == 0)
        return 1;
    CSWSCreature* pCreator =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
    return pObject->AsSWSCreature()->m_pStats->GetEffectImmunity(::IMMUNITY_TYPE_SKILL_DECREASE,
                                                                 pCreator) != 0;
}

// The vision-flag byte is a bitmask, so clear this effect's bits, then rebuild it from the
// surviving effects.
int CSWSEffectListHandler::OnRemoveBlindness(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        int nFlags = pCreature->m_nVisionFlags;
        nFlags = nFlags & ~pEffect->GetInteger(0);
        for (int i = pCreature->m_pStats->m_iFirstBlindness;
             i < pCreature->m_effects.m_count; i++) {
            CGameEffect* pOther = pCreature->m_effects.m_pArray[i];
            if (pOther->m_subType > GAME_EFFECT_BLINDNESS)
                break;
            if (pOther != pEffect && pOther->m_subType == GAME_EFFECT_BLINDNESS)
                nFlags = pOther->GetInteger(0) | nFlags;
        }
        pCreature->m_nVisionFlags = static_cast<unsigned char>(nFlags);
    }
    return 1;
}

int CSWSEffectListHandler::OnApplyForceResisted(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    CGameEffect* pFeedback = new CGameEffect(1);
    pFeedback->m_subType = GAME_EFFECT_VISUALEFFECT;
    pFeedback->m_durationTypeBits =
        CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pFeedback->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pFeedback->SetCreator(pObject->m_objectId);
    pFeedback->SetInteger(0, VFX_COM_FORCE_RESISTED);
    pFeedback->SetObjectID(0, pEffect->GetObjectID(0));
    pObject->ApplyEffect(pFeedback, a, 0);
    pObject->SetAnimation(SERVER_ANIMATION_FORCE_BLOCK);
    return 1;
}

// Turns the instantaneous trigger into the temporary bleed that ticks; the trigger itself is
// never listed.
int CSWSEffectListHandler::OnApplyWounding(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    if (pObject->AsSWSCreature() != 0) {
        if ((pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_INSTANTANEOUS) {
            CGameEffect* pBleed = new CGameEffect(1);
            pBleed->SetCreator(pEffect->m_creatorId);
            pBleed->m_subType = GAME_EFFECT_WOUNDING;
            pBleed->m_durationTypeBits =
                CGameEffect::DURATION_TYPE_TEMPORARY | (pBleed->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pBleed->m_fDuration = 6.0f;
            pObject->ApplyEffect(pBleed, a, 0);
            return 0;
        }
    }
    return 1;
}

// Raises the force pool by the effect's amount, clamped to the maximum, and tells the client
// what was gained.
int CSWSEffectListHandler::OnApplyHealForcePoints(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0 && pCreature->m_pStats != 0) {
        int nAmount = pEffect->GetInteger(0);
        short nNew = pCreature->m_pStats->m_nCurrentForcePoints + pCreature->m_pStats->m_nTemporaryForcePoints;
        int nTotal = nAmount + nNew;
        if (nTotal > pCreature->GetMaxForcePoints())
            nTotal = pCreature->GetMaxForcePoints();
        pCreature->m_pStats->m_nCurrentForcePoints = static_cast<unsigned short>(nTotal);

#if !KOTOR1  // K1 sends no force-points-healed feedback
        CSWCCMessageData* pData = new CSWCCMessageData();
        pData->SetObjectID(0, pCreature->m_objectId);
        pData->SetInteger(0, nAmount);
        pCreature->SendFeedbackMessage(FEEDBACK_FORCE_POINTS_REGAINED, pData);
#endif
    }
    return 1;
}

// Only one disguise may be live at a time, so any other is spent and dropped first; a dead
// creature refuses a fresh disguise, but one restored from a save is applied unconditionally.
int CSWSEffectListHandler::OnApplyDisguise(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature == 0)
        return 1;
    if (a == 0 && pCreature->GetDead() != 0)
        return 1;
    for (int i = 0; i < pCreature->m_effects.m_count; i++) {
        CGameEffect* pOther = pCreature->m_effects.m_pArray[i];
        if (pOther->m_subType == GAME_EFFECT_DISGUISE && pOther->m_effectId != pEffect->m_effectId) {
            pOther->SetInteger(1, 1);
            pCreature->RemoveEffectById(pCreature->m_effects.m_pArray[i]->m_effectId);
            break;
        }
    }
    pCreature->Disguise(pEffect->GetInteger(0), pEffect, a, 0);
    return 0;
}

// Heals the effect's caster, not the target.
int CSWSEffectListHandler::OnApplyVampiricRegeneration(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pObject);
    CSWSCreature* pCreator =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
    if (pCreator != 0) {
        int nDamage = pEffect->GetInteger(0);
        int nCap = pEffect->GetInteger(1);
        CGameEffect* pHeal = new CGameEffect(1);
        pHeal->m_subType = GAME_EFFECT_HEAL;
        pHeal->m_durationTypeBits =
            CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pHeal->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pHeal->SetCreator(pEffect->m_creatorId);
        pHeal->SetInteger(0, nCap < nDamage ? nCap : nDamage);
        pCreator->ApplyEffect(pHeal, a, 0);
    }
    return 1;
}

// Rebuilds the total from every other live effect of the same kind and re-arms the
// damage-table flag.
int CSWSEffectListHandler::OnRemoveArcaneSpellFailure(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        int64_t i = 0;
        int nTotal = 0;
        if (pEffect != 0) {
            while (i < pCreature->m_effects.m_count) {
                CGameEffect* pOther = pCreature->m_effects.m_pArray[i];
                if (pOther->m_subType > GAME_EFFECT_ARCANE_SPELL_FAILURE)
                    break;
                if (pOther != pEffect && pOther->m_subType == GAME_EFFECT_ARCANE_SPELL_FAILURE)
                    nTotal = pOther->GetInteger(0) + nTotal;
                i++;
            }
        }
        if (pObject->AsSWSCreature() != 0)
            pObject->AsSWSCreature()->m_bUpdateCombatInformation = 1;
        pCreature->m_pStats->SetArcaneSpellFailure(static_cast<unsigned char>(nTotal));
    }
    return 1;
}

// Applies a miss-chance effect and a vision-flags effect.
int CSWSEffectListHandler::OnApplyDarkness(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        CGameEffect* pMissChance = new CGameEffect(pEffect);
        pMissChance->m_subType = GAME_EFFECT_INVISIBILITY;
        pMissChance->SetInteger(0, 2);
        pMissChance->SetInteger(1, g_pRules->m_nNumRaces);
        pCreature->ApplyEffect(pMissChance, a, 0);

        CGameEffect* pVision = new CGameEffect(pEffect);
        pVision->m_subType = GAME_EFFECT_BLINDNESS;
        pVision->SetInteger(0, 8);
        pCreature->ApplyEffect(pVision, a, 0);
    }
    return 0;
}

// Rebuilds the true-seeing bit, then re-applies every blindness/darkness effect so those
// recompute.
int CSWSEffectListHandler::OnRemoveTrueSeeing(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        unsigned char nFlags = pCreature->m_nVisionFlags;
        nFlags &= (0xff & ~CSWSCreature::VISION_TRUE_SEEING);
        for (int i = pCreature->m_pStats->m_iFirstUltravision;
             i < pCreature->m_effects.m_count; i++) {
            CGameEffect* pOther = pCreature->m_effects.m_pArray[i];
            if (pOther->m_subType > GAME_EFFECT_TRUESEEING)
                break;
            if (pOther->m_subType == GAME_EFFECT_TRUESEEING)
#ifdef K2_FIX_ENGINE_BUGS
                nFlags = pOther != pEffect ? nFlags | CSWSCreature::VISION_TRUE_SEEING : nFlags;
#else
                nFlags = pOther != pEffect ? nFlags | CSWSCreature::VISION_ULTRAVISION : nFlags;
#endif
        }
        pCreature->m_nVisionFlags = nFlags;

        for (int j = pCreature->m_pStats->m_iFirstBlindness;
             j < pCreature->m_effects.m_count; j++) {
            CGameEffect* pOther = pCreature->m_effects.m_pArray[j];
            if (pOther->m_subType > GAME_EFFECT_BLINDNESS)
                break;
            if (pOther->m_subType == GAME_EFFECT_BLINDNESS) {
                pCreature->RemoveEffect(pOther);
                pCreature->ApplyEffect(pOther, 0, 0);
            }
        }
    }
    return 1;
}

// Rebuilds the penalty from the largest surviving effect.
int CSWSEffectListHandler::OnRemoveForceResistanceDecrease(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreatureStats* pStats = pObject->AsSWSCreature()->m_pStats;
        int64_t i = 0;
        int nPenalty = 0;
        while (i < pObject->m_effects.m_count) {
            CGameEffect* pOther = pObject->m_effects.m_pArray[i];
            unsigned short nSubType = pOther->m_subType;
            if (nSubType == GAME_EFFECT_FORCE_RESISTANCE_INCREASE
                || nSubType == GAME_EFFECT_FORCE_RESISTANCE_DECREASE) {
                if (pOther != pEffect) {
                    int nValue = pOther->GetInteger(0);
#ifdef K2_FIX_ENGINE_BUGS
                    if (nSubType == GAME_EFFECT_FORCE_RESISTANCE_DECREASE && nValue > nPenalty)
#else
                    if (nSubType != GAME_EFFECT_FORCE_RESISTANCE_DECREASE && nValue > nPenalty)
#endif
                        nPenalty = pObject->m_effects.m_pArray[i]->GetInteger(0);
                }
            } else if (nSubType > GAME_EFFECT_FORCE_RESISTANCE_DECREASE) {
                break;
            }
            i++;
        }
        if (pObject->AsSWSCreature() != 0)
            pObject->AsSWSCreature()->m_bUpdateCombatInformation = 1;
        pStats->SetForceResistancePenalty(static_cast<char>(nPenalty));
    }
    return 1;
}

// Sets the true-seeing vision bit and tears down every darkness effect that blocked it.
int CSWSEffectListHandler::OnApplyTrueSeeing(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(pEffect); K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        pCreature->m_nVisionFlags = pCreature->m_nVisionFlags | 4;
        for (int i = pCreature->m_pStats->m_iFirstBlindness;
             i < pCreature->m_effects.m_count; i++) {
            CGameEffect* pDarkness = pCreature->m_effects.m_pArray[i];
            if (pDarkness->m_subType > GAME_EFFECT_BLINDNESS)
                break;
            if (pDarkness->m_subType == GAME_EFFECT_BLINDNESS && pDarkness->GetInteger(0) == 8) {
                int64_t j = 0;
                while (j < pCreature->m_effects.m_count) {
                    CGameEffect* pOther = pCreature->m_effects.m_pArray[j];
                    if (pOther->m_subType > GAME_EFFECT_MISS_CHANCE)
                        break;
                    if (pOther != pDarkness && pOther->m_effectId == pDarkness->m_effectId) {
                        pCreature->RemoveEffect(pCreature->m_effects.m_pArray[i]);
                        i = pCreature->m_pStats->m_iFirstBlindness;
                    }
                    j++;
                }
            }
        }
    }
    return 0;
}

int CSWSEffectListHandler::OnRemoveBonusFeat(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        unsigned short nFeat = static_cast<unsigned short>(pEffect->GetInteger(0));
        pObject->AsSWSCreature()->m_pStats->m_bonusFeats.Remove(nFeat);
    }
    return 1;
}

// Which effects survive the creature's death, driven by the "removefxondeath" 2D array.
int CSWSEffectListHandler::IsEffectToIgnoreOnDeath(CGameEffect* pEffect) {
    C2DA* p2DA = g_pRules->m_p2DArrays->m_pRemoveFXOnDeath;
    for (int i = 0; i < p2DA->m_nRows; i++) {
        int nEffectType;
        if (p2DA->GetINTEntry(i, CExoString("EffectType"), &nEffectType) != 0) {
            if (pEffect->GetEffectTypeScriptConstant() == nEffectType)
                return 1;
            if (nEffectType == EFFECT_TYPE_BEAM && pEffect->m_subType == GAME_EFFECT_VISUALEFFECT &&
                CLoopingVisualEffect::GetIsBeam(static_cast<unsigned short>(pEffect->GetInteger(0))) != 0)
                return 1;
        }
    }
    return 0;
}

// Rebuilds the net resistance from the largest surviving increase and the last decrease.
int CSWSEffectListHandler::OnRemoveForceResistanceIncrease(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreatureStats* pStats = pObject->AsSWSCreature()->m_pStats;
        int64_t i = 0;
        int nDecrease = 0;
        int nIncrease = 0;
        while (i < pObject->m_effects.m_count) {
            CGameEffect* pOther = pObject->m_effects.m_pArray[i];
            if (pOther->m_effectId != pEffect->m_effectId) {
                unsigned short nSubType = pOther->m_subType;
                if (nSubType == GAME_EFFECT_FORCE_RESISTANCE_DECREASE) {
                    nDecrease = pOther->GetInteger(0);
                } else if (nSubType == GAME_EFFECT_FORCE_RESISTANCE_INCREASE) {
                    if (pOther->GetInteger(0) > nIncrease)
                        nIncrease = pObject->m_effects.m_pArray[i]->GetInteger(0);
                } else if (nSubType > GAME_EFFECT_FORCE_RESISTANCE_DECREASE) {
                    break;
                }
            }
            i++;
        }
        if (pObject->AsSWSCreature() != 0)
            pObject->AsSWSCreature()->m_bUpdateCombatInformation = 1;
        pStats->SetForceResistance(static_cast<char>(nIncrease - nDecrease));
    }
    return 1;
}

// Drops the icon unless another live effect still asks for it.
int CSWSEffectListHandler::OnRemoveEffectIcon(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        unsigned short i = pObject->AsSWSCreature()->m_pStats->m_iFirstEffectIcon;
        unsigned char nIcon = static_cast<unsigned char>(pEffect->GetInteger(0));
        int bStillWanted = 0;
        for (; i < pObject->m_effects.m_count; i++) {
            CGameEffect* pOther = pObject->m_effects.m_pArray[i];
            if (pOther->m_subType != GAME_EFFECT_EFFECT_ICON)
                break;
            if (pOther != pEffect && pOther->GetInteger(0) == nIcon)
                bStillWanted = 1;
        }
        unsigned short j = static_cast<unsigned short>(pCreature->m_effectIcons.m_count);
        do {
            j--;
        } while (nIcon != pCreature->m_effectIcons.m_pArray[j]->m_nIconId);
        if (bStillWanted == 0) {
            delete pCreature->m_effectIcons.m_pArray[j];
            pCreature->m_effectIcons.DelIndex(j);
        }
    }
    return 1;
}

// Rebuilds the ultravision bit, then re-runs every darkness/blindness effect through a fresh
// copy.
int CSWSEffectListHandler::OnRemoveUltraVision(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        unsigned char nFlags = pCreature->m_nVisionFlags;
        nFlags &= (0xff & ~CSWSCreature::VISION_ULTRAVISION);
        for (int i = pCreature->m_pStats->m_iFirstUltravision;
             i < pCreature->m_effects.m_count; i++) {
            CGameEffect* pOther = pCreature->m_effects.m_pArray[i];
            if (pOther->m_subType > GAME_EFFECT_ULTRAVISION)
                break;
            if (pOther->m_subType == GAME_EFFECT_ULTRAVISION)
                nFlags = pOther != pEffect ? nFlags | 2 : nFlags;
        }
        pCreature->m_nVisionFlags = nFlags;

        for (int j = pCreature->m_pStats->m_iFirstBlindness;
             j < pCreature->m_effects.m_count; j++) {
            CGameEffect* pOther = pCreature->m_effects.m_pArray[j];
            if (pOther->m_subType > GAME_EFFECT_BLINDNESS)
                break;
            if (pOther->m_subType == GAME_EFFECT_BLINDNESS) {
                CGameEffect* pCopy = new CGameEffect(1);
                pCopy->CopyEffect(pOther, 0);
                pCreature->RemoveEffect(pOther);
                pCreature->ApplyEffect(pCopy, 0, 0);
            }
        }
    }
    return 1;
}

// Turns the per-tick hit-point delta into the repeating dying-drift effect.
int CSWSEffectListHandler::OnApplyHitPointChangeWhenDying(CSWSObject* pObject, CGameEffect* pEffect,
                                                          int a) {
    if (pObject->AsSWSCreature() == 0)
        return 1;
    if (pObject->AsSWSCreature()->m_pStats->m_bIsPC == 0)
        return 1;
    if ((pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_INSTANTANEOUS &&
        pEffect->GetFloat(0) >= 0.1f) {
        CGameEffect* pTick = new CGameEffect(1);
        pTick->SetCreator(pEffect->m_creatorId);
        pTick->m_subType = GAME_EFFECT_HITPOINTCHANGEWHENDYING;
        pTick->m_durationTypeBits =
            CGameEffect::DURATION_TYPE_TEMPORARY | (pTick->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pTick->SetFloat(0, pEffect->GetFloat(0));
        pTick->m_fDuration = __builtin_fabsf(6.0f / pEffect->GetFloat(0));
        pObject->ApplyEffect(pTick, a, 0);
    }
    return 0;
}

template <> inline void CExoArrayList<CEffectIconObject*>::Allocate(int n) {
    K2_ASSUME(m_count <= n);
    CEffectIconObject** pOld = m_pArray;
    m_capacity = n;
    m_pArray = new CEffectIconObject*[n];
    for (int64_t i = 0; i < m_count; i++)
        m_pArray[i] = pOld[i];
    delete[] pOld;
}

#if KOTOR1  // K1 keeps this instantiation out of line
template <> inline __attribute__((noinline)) void CExoArrayList<CEffectIconObject*>::Insert(CEffectIconObject* v, int index) {
#else
template <> inline void CExoArrayList<CEffectIconObject*>::Insert(CEffectIconObject* v, int index) {
#endif
#if KOTOR1
    int i = m_count;
    Add(v);
    for (; i > index; i--)
        m_pArray[i] = m_pArray[i - 1];
#else
    int64_t i = m_count;
    Add(v);
    if (static_cast<int>(i) > index) {
        do {
            m_pArray[i] = m_pArray[i - 1];
            i--;
        } while (i > index);
    }
#endif
    m_pArray[index] = v;
}

int CSWSEffectListHandler::OnApplyBonusFeat(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() == 0)
        return 1;
    unsigned short nFeat = static_cast<unsigned short>(pEffect->GetInteger(0));
    pObject->AsSWSCreature()->m_pStats->m_bonusFeats.Add(nFeat);
    return 0;
}

// Drops the target's attack for a while and plays the taunt visual.
int CSWSEffectListHandler::OnApplyTaunt(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    if (pObject->AsSWSCreature() == 0)
        return 1;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature->m_bPlot != 0)
        return 1;
    pCreature->m_bTaunted = 1;

    CGameEffect* pAttackDecrease = new CGameEffect(pEffect);
    pAttackDecrease->m_subType = GAME_EFFECT_AC_DECREASE;
    pAttackDecrease->SetInteger(0, 0);
    pAttackDecrease->SetInteger(1, pEffect->GetInteger(0));
    pAttackDecrease->SetInteger(2, g_pRules->m_nNumRaces);
    pAttackDecrease->SetInteger(5, DAMAGE_TYPES_PHYSICAL);
    pObject->ApplyEffect(pAttackDecrease, a, 0);

    CGameEffect* pVisual = new CGameEffect(pEffect);
    pVisual->m_subType = GAME_EFFECT_ARCANE_SPELL_FAILURE;
    pVisual->SetInteger(0, 30);
    pObject->ApplyEffect(pVisual, a, 0);
    return 0;
}

// Queues every temporary or permanent spell effect whose caster loses the dispel contest.
int CSWSEffectListHandler::OnApplyDispelAllMagic(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CGameObject* pCasterObject;
    if (pObjectArray->GetGameObject(pEffect->m_creatorId, &pCasterObject) !=
            CGameObjectArray::SUCCESS ||
        pCasterObject == 0)
        return 1;
    if (pCasterObject->AsSWSCreature() == 0)
        return 1;
    int nCasterLevel = pCasterObject->AsSWSCreature()->m_pStats->GetLevel(0);

    CSWCCMessageData* pData = new CSWCCMessageData();
    pData->SetObjectID(0, pObject->m_objectId);
    int nCount = 0;
    CExoArrayList<unsigned long long> l_removed;
    if (pObject->m_effects.m_count >= 1) {
        for (int64_t i = 0; i < pObject->m_effects.m_count; i++) {
            CGameEffect* pEf = pObject->m_effects.m_pArray[i];
            if ((pEf->m_durationTypeBits & CGameEffect::SUBTYPE_MASK) != SUBTYPE_MAGICAL ||
                ((pEf->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_TEMPORARY &&
                 (pEf->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_PERMANENT))
                continue;
            unsigned long long nEffectId = pEf->m_effectId;
            int nSpellId = pEf->m_spellId;
            if (l_removed.Contains(nEffectId) != 0)
                continue;
            int nEfLevel = 0;
            CGameObject* pEfCasterObject;
            if (pObjectArray->GetGameObject(
                    pEf->m_creatorId, &pEfCasterObject) == CGameObjectArray::SUCCESS &&
                pEfCasterObject->AsSWSCreature() != 0)
                nEfLevel = pEfCasterObject->AsSWSCreature()->m_pStats->GetLevel(0);
            unsigned short nRoll = g_pRules->RollDice(1, 20);
            if (nCasterLevel + nRoll > nEfLevel + 11) {
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, pEffect->m_creatorId, pObject->m_objectId,
                    CServerAIMaster::EVENT_REMOVE_EFFECT, pEf);
                if (pEf->m_spellId != -1)
                    pData->SetInteger(++nCount, nSpellId);
            }
            l_removed.AddUnique(nEffectId);
        }
    }
    pData->SetInteger(0, nCount);
    if (nCount >= 1) {
        CSWSPlayer* pTargetPlayer =
            g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(pObject->m_objectId);
        if (pTargetPlayer != 0)
            g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerCCMessage(
                pTargetPlayer->m_nPlayerId, CC_MESSAGE_DISPEL, pData, 0);
        if (pCasterObject->m_objectId != pObject->m_objectId) {
            CSWSPlayer* pCasterPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(
                pCasterObject->m_objectId);
            if (pCasterPlayer != 0)
                g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerCCMessage(
                    pCasterPlayer->m_nPlayerId, CC_MESSAGE_DISPEL, pData, 0);
        }
    }
    delete pData;
    return 1;
}

// Most gate failures queue this effect's removal instead of rejecting quietly. A surviving
// application rolls the poison.2da fortitude save and applies the poisoned state, or just the
// removal.
int CSWSEffectListHandler::OnApplyPoison(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0) {
#if KOTOR1  // a downed party member (GetDeadTemp) is refused in the same test
        if (pCreature->m_bPlot != 0 || pCreature->GetDead() != 0 || pCreature->GetDeadTemp() != 0) {
#else
        if (pCreature->m_bPlot != 0 || pCreature->GetDead() != 0) {
#endif
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, pEffect->m_creatorId, pCreature->m_objectId,
                CServerAIMaster::EVENT_REMOVE_EFFECT, pEffect);
            return 0;
        }
#if !KOTOR1
        if (pCreature->m_bInParty != 0 && pCreature->GetCurrentHitPoints(0) <= 0) {
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, pEffect->m_creatorId, pCreature->m_objectId,
                CServerAIMaster::EVENT_REMOVE_EFFECT, pEffect);
            return 0;
        }
#endif
        if (pCreature->m_bPoisoned == 1) {
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, pEffect->m_creatorId, pCreature->m_objectId,
                CServerAIMaster::EVENT_REMOVE_EFFECT, pEffect);
            return 0;
        }
        if (a == 0) {
            CSWSCreature* pCaster =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
            if (pCreature->m_pStats->GetEffectImmunity(::IMMUNITY_TYPE_POISON, pCaster) != 0) {
                CSWCCMessageData* pData = new CSWCCMessageData();
                pData->SetObjectID(0, pObject->m_objectId);
                pData->SetInteger(0, 0);
                pData->SetInteger(1, 0);
                pData->SetInteger(2, DAMAGE_TYPE_POISON);
                if (pCaster != 0) {
                    CSWCCMessageData* pCasterData = new CSWCCMessageData();
                    pData->CopyTo(pCasterData);
                    pCaster->SendFeedbackMessage(FEEDBACK_DAMAGE_IMMUNITY, pCasterData);
                }
                pCreature->SendFeedbackMessage(FEEDBACK_DAMAGE_IMMUNITY, pData);
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, pEffect->m_creatorId, pCreature->m_objectId,
                    CServerAIMaster::EVENT_REMOVE_EFFECT, pEffect);
                return 0;
            }
            unsigned int nDay;
            unsigned int nTime;
            g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTime);
            pEffect->SetInteger(1, nDay);
            pEffect->SetInteger(2, nTime);
            C2DA* pPoison = g_pRules->m_p2DArrays->m_pPoison;
            if (pPoison == 0) {
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, pEffect->m_creatorId, pCreature->m_objectId,
                    CServerAIMaster::EVENT_REMOVE_EFFECT, pEffect);
                return 0;
            }
            int nRow = pEffect->GetInteger(0);
            int nSaveDC;
            pPoison->GetINTEntry(nRow, CExoString("DC_SAVE"), &nSaveDC);
            if (pCreature->SavingThrowRoll(SAVING_THROW_FORT, static_cast<unsigned short>(nSaveDC), SAVING_THROW_TYPE_POISON,
                                           pEffect->m_creatorId, 1, 0, 0) != 0) {
#if !KOTOR1  // K1 has no resisted-poison marker
                if (pEffect->m_spellId == FORCE_POWER_AFFLICTION
                    || pEffect->m_spellId == FORCE_POWER_PLAGUE) {
                    CGameEffect* pMarker = new CGameEffect(1);
                    pMarker->m_subType = GAME_EFFECT_FORCERESISTED;
                    pMarker->SetObjectID(0, pEffect->m_creatorId);
                    pCreature->ApplyEffect(pMarker, 0, 0);
                }
#endif
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, pEffect->m_creatorId, pCreature->m_objectId,
                    CServerAIMaster::EVENT_REMOVE_EFFECT, pEffect);
                return 0;
            }
            int nName;
            pPoison->GetINTEntry(nRow, CExoString("Name"), &nName);
            CSWCCMessageData* pData = new CSWCCMessageData();
            pData->SetInteger(0, nName);
            pData->SetObjectID(0, pCreature->m_objectId);
            pCreature->BroadcastPoisoned(pData);
            delete pData;
            pCreature->BroadcastVoiceChat(CSWSCreature::VOICE_CHAT_POISONED);
            pEffect->SetFloat(0, 2.0f);
            pCreature->ApplyPoisonDamage(nRow, pEffect, 0, 1.0f);
            pEffect->SetInteger(3, nDay);
            pEffect->SetInteger(4, nTime);
            int nDuration;
            pPoison->GetINTEntry(nRow, CExoString("DURATION"), &nDuration);
            pEffect->SetInteger(5, nDuration);
            pEffect->m_durationTypeBits =
                CGameEffect::DURATION_TYPE_TEMPORARY | (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pEffect->m_fDuration = nDuration * 1000.0f;
            int nPeriod;
            pPoison->GetINTEntry(nRow, CExoString("PERIOD"), &nPeriod);
            pEffect->SetInteger(6, nPeriod);
            pEffect->SetInteger(7, 0);
        }
        CGameEffect* pFeedback = new CGameEffect(1);
        pFeedback->m_subType = GAME_EFFECT_VISUALEFFECT;
        pFeedback->m_durationTypeBits =
            CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pFeedback->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pFeedback->SetInteger(0, VFX_PRO_AFFLICT);
        pFeedback->SetInteger(2, 0);
        pFeedback->SetObjectID(0, pEffect->m_creatorId);
        pCreature->ApplyEffect(pFeedback, 0, 0);
        pCreature->m_bPoisoned = 1;
        pCreature->m_idPoisonEffect = pEffect->m_effectId;
    }
    return 0;
}

// Every other creature that can see the target rolls a will save and is pacified on failure.
int CSWSEffectListHandler::OnApplySanctuary(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
        if (pArea != 0) {
            unsigned char nDC = static_cast<unsigned char>(pEffect->GetInteger(4));
            CExoArrayList<unsigned long long>& aObjects = pArea->m_aObjectsByX;
            for (int64_t i = 0; i < aObjects.m_count; i++) {
                CSWSCreature* pOther = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                    aObjects[static_cast<int>(i)]);
                if (pOther != 0 && pOther->m_objectId != pCreature->m_objectId) {
                    CSWVisibilityNode* pNode = pOther->GetVisibleListElement(pCreature->m_objectId);
                    if (pNode != 0) {
#if KOTOR1  // the failed-save arm comes first
                        if (pOther->SavingThrowRoll(SAVING_THROW_WILL, nDC, SAVING_THROW_TYPE_NONE, pCreature->m_objectId, 1, 0, 0) == 0) {
                            pCreature->PacifyCreature(pOther->m_objectId);
                            pNode->m_flags =
                                    (pNode->m_flags & (0xff & ~VISIBILITY_PACIFY_RESISTED)) | VISIBILITY_PACIFIED;
                        } else {
                            pNode->m_flags =
                                    (pNode->m_flags & (0xff & ~VISIBILITY_PACIFY_STATE_MASK)) | VISIBILITY_PACIFY_RESISTED;
                        }
#else
                        if (pOther->SavingThrowRoll(SAVING_THROW_WILL, nDC, SAVING_THROW_TYPE_NONE, pCreature->m_objectId, 1, 0, 0) != 0) {
                            pNode->m_flags =
                                    (pNode->m_flags & (0xff & ~VISIBILITY_PACIFY_STATE_MASK)) | VISIBILITY_PACIFY_RESISTED;
                        } else {
                            pCreature->PacifyCreature(pOther->m_objectId);
                            pNode->m_flags =
                                    (pNode->m_flags & (0xff & ~VISIBILITY_PACIFY_RESISTED)) | VISIBILITY_PACIFIED;
                        }
#endif
                    }
                }
            }
            g_pAppManager->m_pServerExoApp->GetModule()->m_playerList.AddUnique(
                pCreature->m_objectId);
        }
    }
    return 0;
}

// Applies the teleport half. The target is sent to the destination object and plays the
// disappear animation.
int CSWSEffectListHandler::OnApplyDisappearAppear(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    pEffect->GetFloat(0);
    pEffect->GetFloat(1);
    pEffect->GetFloat(2);
    unsigned long long oidDestination = pEffect->GetObjectID(0);
    if (g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(oidDestination) != 0) {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE) {
            CSWSCreature* pCreature = pObject->AsSWSCreature();
            pCreature->ClearAllActions(1);
            pCreature->PacifyCreature();
            pCreature->m_currentAreaId = oidDestination;
            pCreature->m_vDesiredAreaLocation = pObject->AsSWSCreature()->m_position;
            pCreature->m_bDesiredAreaUpdateComplete = 0;
            pCreature->m_pCombatRound->SetPauseTimer(0, 1);
            pCreature->m_pCombatRound->SetRoundPaused(1, pObject->m_objectId);
            pCreature->AddDisappearActions(1);
        } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 2000, pEffect->m_creatorId, pObject->m_objectId,
                CServerAIMaster::EVENT_REMOVE_FROM_AREA, 0);
        } else {
            return 1;
        }
        return 0;
    }
    return 1;
}

// An immune target gets the feedback line; otherwise the creature is stunned and its silence
// flag goes up.
int CSWSEffectListHandler::OnApplySilence(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature == 0)
        return 0;
    CSWSCreature* pCaster =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
    if (pCreature->m_pStats->GetEffectImmunity(::IMMUNITY_TYPE_SILENCE, pCaster) != 0) {
        CSWCCMessageData* pData = new CSWCCMessageData();
        pData->SetObjectID(0, pObject->m_objectId);
        if (pCaster != 0) {
            CSWCCMessageData* pCasterData = new CSWCCMessageData();
            pData->CopyTo(pCasterData);
            pCaster->SendFeedbackMessage(FEEDBACK_IMMUNE_SILENCE, pCasterData);
        }
        pObject->AsSWSCreature()->SendFeedbackMessage(FEEDBACK_IMMUNE_SILENCE, pData);
        return 1;
    }
    if (pCreature->m_bPlot != 0)
        return 1;
    CGameEffect* pStun = new CGameEffect(pEffect);
    pStun->m_subType = GAME_EFFECT_SET_AI_STATE;
    pStun->SetInteger(0, ~AI_STATE_CAN_SPEAK);
    pCreature->ApplyEffect(pStun, a, 0);
    pCreature->m_bSilenced = 1;
    return 0;
}

// Drives the active pause, or extends the existing one when the player already paused.
int CSWSEffectListHandler::OnApplyTimestop(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (g_pAppManager->m_pServerExoApp->GetActivePauseState() == 0) {
        g_pAppManager->m_pServerExoApp->AddToExclusionList(pObject->m_objectId, 1);
        g_pAppManager->m_pServerExoApp->TogglePauseState(1);
        return 0;
    }
    if (g_pAppManager->m_pServerExoApp->GetActivePauseState() == 1) {
        int nDurationDay =
            g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetCalendarDayFromSeconds(
                pEffect->m_fDuration);
        int nDurationTod =
            g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetTimeOfDayFromSeconds(
                pEffect->m_fDuration);
        unsigned int nExpireDay;
        unsigned int nExpireTod;
        unsigned int nDay;
        unsigned int nTod;
        g_pAppManager->m_pServerExoApp->GetActiveTimer(pObject->m_objectId)->GetWorldTime(&nDay,
                                                                                         &nTod);
        g_pAppManager->m_pServerExoApp->GetWorldTimer()->AddWorldTimes(
            nDay, nTod, nDurationDay, nDurationTod, &nExpireDay, &nExpireTod);
        for (int i = 0; i < pObject->m_effects.m_count; i++) {
            CGameEffect* pOther = pObject->m_effects.m_pArray[i];
            if (pOther->m_subType == GAME_EFFECT_TIMESTOP) {
                pOther->SetExpiryTime(nExpireDay, nExpireTod);
                return 1;
            }
        }
    }
    return 0;
}

// Queues the leap as a delayed effect and sends the feedback line immediately.
int CSWSEffectListHandler::OnApplyForceJump(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0) {
        CSWSCreature* pTarget = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
            pEffect->GetObjectID(0));
        if (pTarget != 0) {
            pCreature->SetLockOrientationToObject(pTarget->m_objectId, 0);

            CGameEffect* pJump = new CGameEffect(pEffect);
            pJump->m_subType = GAME_EFFECT_FORCEJUMP_DELAYED;
            pJump->m_durationTypeBits =
                CGameEffect::DURATION_TYPE_TEMPORARY | (pJump->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pJump->SetObjectID(0, pTarget->m_objectId);
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 300, pCreature->m_objectId, pCreature->m_objectId,
                CServerAIMaster::EVENT_APPLY_EFFECT, pJump);

            CGameEffect* pFeedback = new CGameEffect(pEffect);
            pFeedback->m_subType = GAME_EFFECT_VISUALEFFECT;
            pFeedback->m_durationTypeBits =
                CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pFeedback->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pFeedback->SetInteger(0, VFX_IMP_FORCE_JUMP_ADVANCED);
            pCreature->ApplyEffect(pFeedback, 0, 0);
        }
    }
    return 0;
}

// Grants the see-invisible companion effect, raises the vision bit, then clears any darkness.
int CSWSEffectListHandler::OnApplyUltraVision(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        CGameEffect* pSeeInvisible = new CGameEffect(pEffect);
        pSeeInvisible->m_subType = GAME_EFFECT_VISION;
        pSeeInvisible->SetInteger(0, 3);
        pCreature->ApplyEffect(pSeeInvisible, a, 0);
        pCreature->m_nVisionFlags = pCreature->m_nVisionFlags | 2;
        for (int i = pCreature->m_pStats->m_iFirstBlindness;
             i < pCreature->m_effects.m_count; i++) {
            CGameEffect* pDarkness = pCreature->m_effects.m_pArray[i];
            if (pDarkness->m_subType > GAME_EFFECT_BLINDNESS)
                break;
            if (pDarkness->m_subType == GAME_EFFECT_BLINDNESS && pDarkness->GetInteger(0) == 8) {
                int64_t j = 0;
                while (j < pCreature->m_effects.m_count) {
                    CGameEffect* pOther = pCreature->m_effects.m_pArray[j];
                    if (pOther->m_subType > GAME_EFFECT_MISS_CHANCE)
                        break;
                    if (pOther != pDarkness && pOther->m_effectId == pDarkness->m_effectId) {
                        pCreature->RemoveEffect(pOther);
                        i = pCreature->m_pStats->m_iFirstBlindness;
                        j = 0;
                        continue;
                    }
                    j++;
                }
            }
        }
    }
    return 0;
}

// Revives a downed creature at a percentage of its maximum hit points.
int CSWSEffectListHandler::OnApplyResurrection(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
#if KOTOR1
    K2_UNUSED(pEffect);
#endif
    if (pObject->m_bRaiseable != 0 && pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
#if KOTOR1  // always back to 1 HP (no percentage)
        if (pCreature->GetCurrentHitPoints(0) <= 0)
            pCreature->SetCurrentHitPoints(1);
#else
        if (pCreature->GetCurrentHitPoints(0) <= 0) {
            int nPercent = pEffect->GetInteger(0);
            int nHitPoints = 1;
            if (nPercent >= 1) {
                nHitPoints = nPercent * pCreature->GetMaxHitPoints(1) / 100;
                if (nHitPoints == 0)
                    nHitPoints = 1;
            }
            pCreature->SetCurrentHitPoints(nHitPoints);
        }
#endif
        pCreature->ClearAllActions(1);
        pCreature->m_bListening = 1;
        pCreature->m_nAIState = 0xffff;
        pCreature->m_bCommandable = 1;
        pCreature->RecomputeAmbientAnimationState();

        CSWSPlayer* pPlayer =
            g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(pObject->m_objectId);
        if (pPlayer != 0)
            g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerDestroyDeathGUI(
                pPlayer->m_nPlayerId);

        int64_t i = 0;
        while (i < pCreature->m_effects.m_count) {
            CGameEffect* pOther = pCreature->m_effects.m_pArray[i];
            if (pOther->m_subType > GAME_EFFECT_HITPOINTCHANGEWHENDYING)
                break;
            if (pOther->m_subType == GAME_EFFECT_HITPOINTCHANGEWHENDYING &&
                (pOther->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_TEMPORARY)
                pCreature->RemoveEffectById(pOther->m_effectId);
            i++;
        }
    }
    return 1;
}

// Shares its hook with blindness; the effect's integer selects the vision-flag bit.
int CSWSEffectListHandler::OnApplyDeaf(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    if (pObject->AsSWSCreature() == 0)
        return 0;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    CSWSCreature* pCaster =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
    if (pCreature->m_pStats->GetEffectImmunity(::IMMUNITY_TYPE_DEAFNESS, pCaster) != 0) {
        CSWCCMessageData* pData = new CSWCCMessageData();
        pData->SetObjectID(0, pObject->m_objectId);
        if (pCaster != 0) {
            CSWCCMessageData* pCasterData = new CSWCCMessageData();
            pData->CopyTo(pCasterData);
            pCaster->SendFeedbackMessage(FEEDBACK_IMMUNE_DEAFNESS, pCasterData);
        }
        pObject->AsSWSCreature()->SendFeedbackMessage(FEEDBACK_IMMUNE_DEAFNESS, pData);
        return 1;
    }
    if (pCreature->m_bPlot != 0)
        return 1;
    CGameEffect* pStun = new CGameEffect(pEffect);
    CGameEffect* pVisual = new CGameEffect(pEffect);
    pStun->m_subType = GAME_EFFECT_SET_AI_STATE;
    pStun->SetInteger(0, ~AI_STATE_CAN_HEAR);
    pCreature->ApplyEffect(pStun, a, 0);
    pVisual->m_subType = GAME_EFFECT_ARCANE_SPELL_FAILURE;
    pVisual->SetInteger(0, 20);
    pCreature->ApplyEffect(pVisual, a, 0);
    return 0;
}

// Integer 0 selects which of the five AC penalty bytes is fed; only dodge accumulates.
int CSWSEffectListHandler::OnApplyACDecrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() == 0)
        return 0;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    CSWSCreature* pCaster =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
    if (pCreature->m_pStats->GetEffectImmunity(::IMMUNITY_TYPE_AC_DECREASE, pCaster) != 0)
        return 1;
    int nACType = pEffect->GetInteger(0);
    int nAmount = pEffect->GetInteger(1);
    int nRace = pEffect->GetInteger(2);
    int nAlignmentGroup = pEffect->GetInteger(3);
    int nAlignment = pEffect->GetInteger(4);
    pEffect->GetInteger(5);
    if (nAmount < 1)
        return 1;
    if (pCreature->m_bPlot != 0)
        return 1;
    if (nRace != g_pRules->m_nNumRaces)
        return 0;
    if (nAlignment != 0 || nAlignmentGroup != 0)
        return 0;
    switch (nACType) {
    case AC_DODGE_BONUS:
        pCreature->m_pStats->m_nACDodgeNeg = static_cast<signed char>(pCreature->m_pStats->m_nACDodgeNeg + nAmount);
        break;
    case AC_NATURAL_BONUS:
        if (nAmount > static_cast<signed char>(pCreature->m_pStats->m_nACNaturalNeg))
            pCreature->m_pStats->m_nACNaturalNeg = static_cast<unsigned char>(nAmount);
        break;
    case AC_ARMOUR_ENCHANTMENT_BONUS:
        if (nAmount > static_cast<signed char>(pCreature->m_pStats->m_nACArmorNeg))
            pCreature->m_pStats->m_nACArmorNeg = static_cast<unsigned char>(nAmount);
        break;
    case AC_SHIELD_ENCHANTMENT_BONUS:
        if (nAmount > static_cast<signed char>(pCreature->m_pStats->m_nACShieldNeg))
            pCreature->m_pStats->m_nACShieldNeg = static_cast<unsigned char>(nAmount);
        break;
    case AC_DEFLECTION_BONUS:
        if (nAmount > static_cast<signed char>(pCreature->m_pStats->m_nACDeflectionNeg))
            pCreature->m_pStats->m_nACDeflectionNeg = static_cast<unsigned char>(nAmount);
        break;
    }
    return 0;
}

// Rebuilds the penalty from the largest surviving effect of the same AC type; dodge gives its
// accumulated share back.
int CSWSEffectListHandler::OnRemoveACDecrease(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pEffect != 0) {
            int nACType = pEffect->GetInteger(0);
            int nAmount = pEffect->GetInteger(1);
            int nRace = pEffect->GetInteger(2);
            int nAlignmentGroup = pEffect->GetInteger(3);
            if (pEffect->GetInteger(4) != 0 || nAlignmentGroup != 0)
                return 1;
            if (nRace != g_pRules->m_nNumRaces)
                return 1;
            if (nACType == AC_DODGE_BONUS) {
                pCreature->m_pStats->m_nACDodgeNeg = static_cast<signed char>(pCreature->m_pStats->m_nACDodgeNeg - nAmount);
                return 1;
            }
            int nMax = 0;
            for (int i = pCreature->m_pStats->m_iFirstACIncrease;
                 i < pCreature->m_effects.m_count; i++) {
                CGameEffect* pOther = pCreature->m_effects.m_pArray[i];
                if (pOther->m_subType != GAME_EFFECT_AC_DECREASE)
                    break;
                if (pOther != pEffect && pOther->GetInteger(0) == nACType &&
                    pCreature->m_effects.m_pArray[i]->GetInteger(2) == g_pRules->m_nNumRaces &&
                    pCreature->m_effects.m_pArray[i]->GetInteger(1) > nMax)
                    nMax = pCreature->m_effects.m_pArray[i]->GetInteger(1);
            }
            switch (nACType) {
            case AC_NATURAL_BONUS:
                pCreature->m_pStats->m_nACNaturalNeg = static_cast<unsigned char>(nMax);
                break;
            case AC_ARMOUR_ENCHANTMENT_BONUS:
                pCreature->m_pStats->m_nACArmorNeg = static_cast<unsigned char>(nMax);
                break;
            case AC_SHIELD_ENCHANTMENT_BONUS:
                pCreature->m_pStats->m_nACShieldNeg = static_cast<unsigned char>(nMax);
                break;
            case AC_DEFLECTION_BONUS:
                pCreature->m_pStats->m_nACDeflectionNeg = static_cast<unsigned char>(nMax);
                break;
            }
        }
    }
    return 1;
}

// Integer 0 selects which of the five AC bonus bytes is fed; dodge only counts for the two
// dodge spell ids the effect carries.
int CSWSEffectListHandler::OnApplyACIncrease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pEffect != 0) {
            int nACType = pEffect->GetInteger(0);
            int nAmount = pEffect->GetInteger(1);
            int nRace = pEffect->GetInteger(2);
            int nAlignmentGroup = pEffect->GetInteger(3);
            int nAlignment = pEffect->GetInteger(4);
            int nDamageType = pEffect->GetInteger(5);
            if (nAmount < 1)
                return 1;
            if (nRace != g_pRules->m_nNumRaces)
                return 0;
            if (nAlignment != 0 || nAlignmentGroup != 0)
                return 0;
            switch (nACType) {
            case AC_DODGE_BONUS:
#if KOTOR1  // every dodge bonus stacks; integer 5 is read but unused
                (void)nDamageType;
                pCreature->m_pStats->m_nACDodgeMod = static_cast<signed char>(pCreature->m_pStats->m_nACDodgeMod + nAmount);
#else
                if (nDamageType == DAMAGE_TYPES_PHYSICAL || nDamageType == AC_VS_DAMAGE_TYPE_ALL)
                    pCreature->m_pStats->m_nACDodgeMod = static_cast<signed char>(pCreature->m_pStats->m_nACDodgeMod + nAmount);
#endif
                break;
            case AC_NATURAL_BONUS:
                if (static_cast<signed char>(pCreature->m_pStats->m_nACNaturalMod) >= 1)
                    pCreature->SendFeedbackMessage(FEEDBACK_NATURAL_DEFENSE_NO_STACK, 0);
                if (nAmount > static_cast<signed char>(pCreature->m_pStats->m_nACNaturalMod))
                    pCreature->m_pStats->m_nACNaturalMod = static_cast<unsigned char>(nAmount);
                break;
            case AC_ARMOUR_ENCHANTMENT_BONUS:
                if (static_cast<signed char>(pCreature->m_pStats->m_nACArmorMod) >= 1)
                    pCreature->SendFeedbackMessage(FEEDBACK_ARMOR_DEFENSE_NO_STACK, 0);
                if (nAmount > static_cast<signed char>(pCreature->m_pStats->m_nACArmorMod))
                    pCreature->m_pStats->m_nACArmorMod = static_cast<unsigned char>(nAmount);
                break;
            case AC_SHIELD_ENCHANTMENT_BONUS:
                if (static_cast<signed char>(pCreature->m_pStats->m_nACShieldMod) >= 1)
                    pCreature->SendFeedbackMessage(FEEDBACK_SHIELD_DEFENSE_NO_STACK, 0);
                if (nAmount > static_cast<signed char>(pCreature->m_pStats->m_nACShieldMod))
                    pCreature->m_pStats->m_nACShieldMod = static_cast<unsigned char>(nAmount);
                break;
            case AC_DEFLECTION_BONUS:
                if (static_cast<signed char>(pCreature->m_pStats->m_nACDeflectionMod) >= 1)
                    pCreature->SendFeedbackMessage(FEEDBACK_DEFLECTION_DEFENSE_NO_STACK, 0);
                if (nAmount > static_cast<signed char>(pCreature->m_pStats->m_nACDeflectionMod))
                    pCreature->m_pStats->m_nACDeflectionMod = static_cast<unsigned char>(nAmount);
                break;
            }
        }
    }
    return 0;
}

// Rebuilds the bonus from the largest surviving unrestricted effect of the same AC type.
int CSWSEffectListHandler::OnRemoveACIncrease(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pEffect != 0) {
            int nACType = pEffect->GetInteger(0);
            int nAmount = pEffect->GetInteger(1);
            int nRace = pEffect->GetInteger(2);
            int nAlignmentGroup = pEffect->GetInteger(3);
            int nAlignment = pEffect->GetInteger(4);
#if KOTOR1  // integer 5 is not read; every dodge bonus is removed
            if (nAlignment != 0 || nAlignmentGroup != 0)
                return 1;
            if (nRace != g_pRules->m_nNumRaces)
                return 1;
            CSWSCreatureStats* pStats = pCreature->m_pStats;
            if (nACType == AC_DODGE_BONUS) {
                pStats->m_nACDodgeMod = static_cast<signed char>(pStats->m_nACDodgeMod - nAmount);
                return 1;
            }
#else
            int nDamageType = pEffect->GetInteger(5);
            if (nAlignment != 0 || nAlignmentGroup != 0)
                return 1;
            if (nRace != g_pRules->m_nNumRaces)
                return 1;
            CSWSCreatureStats* pStats = pCreature->m_pStats;
            if (nACType == AC_DODGE_BONUS) {
                if (nDamageType != DAMAGE_TYPES_PHYSICAL && nDamageType != AC_VS_DAMAGE_TYPE_ALL)
                    return 1;
                pStats->m_nACDodgeMod = static_cast<signed char>(pStats->m_nACDodgeMod - nAmount);
                return 1;
            }
#endif
            int nMax = 0;
            for (int i = pStats->m_iFirstACIncrease;
                 i < pCreature->m_effects.m_count; i++) {
                CGameEffect* pOther = pCreature->m_effects.m_pArray[i];
                if (pOther->m_subType != GAME_EFFECT_AC_INCREASE)
                    break;
                if (pOther != pEffect && pOther->GetInteger(0) == nACType &&
                    pOther->GetInteger(2) == g_pRules->m_nNumRaces &&
                    pOther->GetInteger(3) == 0 && pOther->GetInteger(4) == 0 &&
                    pOther->GetInteger(1) > nMax)
                    nMax = pOther->GetInteger(1);
            }
            switch (nACType) {
            case AC_NATURAL_BONUS:
                pCreature->m_pStats->m_nACNaturalMod = static_cast<unsigned char>(nMax);
                break;
            case AC_ARMOUR_ENCHANTMENT_BONUS:
                pCreature->m_pStats->m_nACArmorMod = static_cast<unsigned char>(nMax);
                break;
            case AC_SHIELD_ENCHANTMENT_BONUS:
                pCreature->m_pStats->m_nACShieldMod = static_cast<unsigned char>(nMax);
                break;
            case AC_DEFLECTION_BONUS:
                pCreature->m_pStats->m_nACDeflectionMod = static_cast<unsigned char>(nMax);
                break;
            }
        }
    }
    return 1;
}

#include "factionmanager.h"
#include "nwsparty.h"
#include "globalvartables.h"
#include "nwsplaceable.h"
#include "nwstrigger.h"

// Applies the "join another faction" effect. Only NPC factions are targetable, the pre-change
// faction is stashed in the effect so the remove side can restore it, and a creature
// target has its visibility list rebuilt. Returns 0 when the modifier landed, 1 when
// nothing happened.
#if !KOTOR1  // K2 only
int CSWSEffectListHandler::OnApplyFactionModifier(CSWSObject* pObject, CGameEffect* pEffect, int bLoadingGame) {
    if (pObject == 0)
        return 1;
    if (bLoadingGame != 0)
        return 0;
    int nOldFaction;
    g_pAppManager->m_pServerExoApp->GetFactionOfObject(pObject->m_objectId, &nOldFaction);
    pEffect->SetInteger(1, nOldFaction);
    int nNewFaction = pEffect->GetInteger(0);
    CFactionManager* pFactionManager = g_pAppManager->m_pServerExoApp->GetFactionManager();
    if (pFactionManager == 0)
        return 1;
    if (pFactionManager->GetIsNPCFaction(nNewFaction) == 0)
        return 1;
    if (pObject->AsSWSCreature() != 0 && pObject->AsSWSCreature()->m_pStats->m_bIsPC != 0)
        return 1;
    pObject->m_bConfused = 1;
    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
        pObject->AsSWSPlaceable()->m_nFaction = nNewFaction;
        return 0;
    }
    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER) {
        pObject->AsSWSTrigger()->m_nFaction = nNewFaction;
        return 0;
    }
    pFactionManager->GetFaction(nNewFaction)->AddMember(pObject->m_objectId, 0);
    pObject->ClearAllActions(1);
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        int bDown = 0;
        if (pCreature->m_bInParty != 0)
            bDown = pCreature->GetCurrentHitPoints(0) < 1;
        if ((pObject->GetDead() | bDown) == 0) {
            pCreature->ClearVisibleList();
            pCreature->UpdateVisibleList(0);
        }
    }
    return 0;
}
#endif

// Restores the faction recorded by the apply side and clears whichever "confusion
// active" flag the originating power owns.
#if !KOTOR1  // K2 only
int CSWSEffectListHandler::OnRemoveFactionModifier(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pEffect->m_spellId == FORCE_POWER_CONFUSION)
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(
            CExoString("000_Human_Conf_Active"), 0);
    else if (pEffect->m_spellId == FORCE_POWER_BEAST_CONFUSION)
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(
            CExoString("000_Beast_Conf_Active"), 0);
    else if (pEffect->m_spellId == FORCE_POWER_DROID_CONFUSION)
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(
            CExoString("000_Droid_Conf_Active"), 0);
    CFactionManager* pFactionManager = g_pAppManager->m_pServerExoApp->GetFactionManager();
    if (pFactionManager != 0) {
        pObject->m_bConfused = 0;
        int nFaction = pEffect->GetInteger(1);
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
            pObject->AsSWSPlaceable()->m_nFaction = nFaction;
        } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER) {
            pObject->AsSWSTrigger()->m_nFaction = nFaction;
        } else {
            pFactionManager->GetFaction(nFaction)->AddMember(pObject->m_objectId, 0);
            pObject->ClearAllActions(1);
            if (pObject->AsSWSCreature() != 0) {
                CSWSCreature* pCreature = pObject->AsSWSCreature();
                int bDown = 0;
                if (pCreature->m_bInParty != 0)
                    bDown = pCreature->GetCurrentHitPoints(0) < 1;
                if ((pObject->GetDead() | bDown) == 0) {
                    pCreature->ClearVisibleList();
                    pCreature->UpdateVisibleList(0);
                }
            }
        }
    }
    return 1;
}
#endif

// Only the last invisibility effect may drop the module's membership record.
int CSWSEffectListHandler::OnRemoveInvisibility(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        int bOther = 0;
        for (int i = pCreature->m_pStats->m_iFirstInvisibility;
             i < pCreature->m_effects.m_count; i++) {
            CGameEffect* pOther = pCreature->m_effects.m_pArray[i];
            if (pOther->m_subType > GAME_EFFECT_SANCTUARY)
                break;
            if ((pOther->m_subType == GAME_EFFECT_INVISIBILITY || pOther->m_subType == GAME_EFFECT_SANCTUARY) &&
                pEffect->m_effectId != pOther->m_effectId)
                bOther = 1;
        }
        if (bOther == 0) {
            CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
            if (pModule->m_playerList.Contains(pCreature->m_objectId) != 0)
                pModule->m_playerList.Remove(pCreature->m_objectId);
        }
    }
    return 1;
}

// Sweeps the area's visibility entries and the module list, dropping only the memberships no
// surviving sanctuary effect asks for.
int CSWSEffectListHandler::OnRemoveSanctuary(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
        if (pArea != 0) {
            CExoArrayList<unsigned long long> seenIds;
            if (pArea->m_aObjectsByX.m_count >= 1) {
                for (long long i = 0; i < pArea->m_aObjectsByX.m_count; i++) {
                    CSWSCreature* pCur = g_pAppManager->m_pServerExoApp
                        ->GetCreatureByGameObjectID(pArea->m_aObjectsByX.m_pArray[i]);
                    if (pCur == 0)
                        continue;
                    CSWVisibilityNode* pNode = pCur->GetVisibleListElement(pCreature->m_objectId);
                    if (pNode == 0)
                        continue;
                    if ((pNode->m_flags >> 2 & 3) == 1) {
                        seenIds.Add(pCur->m_objectId);
                    } else if ((pNode->m_flags >> 2 & 3) == 2) {
                        pNode->m_flags &= ~VISIBILITY_PACIFY_STATE_MASK;
                    }
                }
            }
            int bAnyOther = 0;
            int bOtherSanctuary = 0;
            for (int i = pCreature->m_pStats->m_iFirstInvisibility;
                 i < pCreature->m_effects.m_count; i++) {
                CGameEffect* pOther = pCreature->m_effects.m_pArray[i];
                if (pOther->m_subType > GAME_EFFECT_SANCTUARY)
                    break;
                if ((pOther->m_subType == GAME_EFFECT_INVISIBILITY || pOther->m_subType == GAME_EFFECT_SANCTUARY) &&
                        pEffect->m_effectId != pOther->m_effectId) {
                    if (pOther->m_subType == GAME_EFFECT_SANCTUARY)
                        bOtherSanctuary = 1;
                    bAnyOther = 1;
                }
            }
            if (bAnyOther == 0) {
                CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
                if (pModule->m_playerList.Contains(pCreature->m_objectId) != 0)
                    pModule->m_playerList.Remove(pCreature->m_objectId);
            }
            if (bOtherSanctuary == 0 && seenIds.m_count >= 1) {
                for (long long i = 0; i < seenIds.m_count; i++) {
                    CSWSCreature* pCur = g_pAppManager->m_pServerExoApp
                        ->GetCreatureByGameObjectID(seenIds.m_pArray[i]);
                    if (pCur != 0)
                        pCur->RemoveFromVisibleList(pCreature->m_objectId);
                }
            }
        }
    }
    return 1;
}

// The immunity and feat gates return without applying anything; otherwise the state byte is
// raised and a fresh setstate-internal effect does the real work.
int CSWSEffectListHandler::OnApplySetState(CSWSObject* pObject, CGameEffect* pEffect, int nParam) {
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature == 0 || pEffect == 0)
        return 0;
    CSWSCreatureStats* pStats = pCreature->m_pStats;
    int bIgnoreImmunity = pEffect->GetInteger(1);
    if (pStats != 0 && bIgnoreImmunity == 0 && pStats->GetEffectLinkImmunity(pEffect) != 0)
        return 1;
    CSWSCreature* pCaster = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
    int nState = pEffect->GetInteger(0);
    CSWSCombatAttackData* pAttack = 0;
    if (pCaster != 0 && pCaster->m_pCombatRound != 0) {
        pAttack = pCaster->m_pCombatRound->GetAttack(pCaster->m_pCombatRound->m_currentAttack);
        if (pAttack != 0)
            pAttack->m_effect.m_nFeedbackEffectType = static_cast<unsigned char>(nState);
    }
    if (nState == EFFECT_STATE_SLEEP) {
        if (pStats->GetEffectImmunity(::IMMUNITY_TYPE_SLEEP, pCaster) == 1) {
            CSWCCMessageData* pData = new CSWCCMessageData();
            pData->SetObjectID(0, pObject->m_objectId);
            if (pCaster != 0) {
                CSWCCMessageData* pCasterData = new CSWCCMessageData();
                pData->CopyTo(pCasterData);
                pCaster->SendFeedbackMessage(FEEDBACK_IMMUNE_SLEEP, pCasterData);
            }
            if (pAttack != 0)
                pAttack->m_effect.m_bFeedbackEffectData = 1;
            pObject->AsSWSCreature()->SendFeedbackMessage(FEEDBACK_IMMUNE_SLEEP, pData);
            return 1;
        }
    } else if (bIgnoreImmunity == 0 && nState == EFFECT_STATE_PARALYZED) {
        if (pStats->GetEffectImmunity(::IMMUNITY_TYPE_PARALYSIS,
                                      pCaster) != 0 || pStats->HasFeat(CSWSCreatureStats::FEAT_FORCE_IMMUNITY_PARALYSIS)) {
            CSWCCMessageData* pData = new CSWCCMessageData();
            pData->SetObjectID(0, pObject->m_objectId);
            if (pCaster != 0) {
                CSWCCMessageData* pCasterData = new CSWCCMessageData();
                pData->CopyTo(pCasterData);
                pCaster->SendFeedbackMessage(FEEDBACK_IMMUNE_PARALYSIS, pCasterData);
            }
            if (pAttack != 0)
                pAttack->m_effect.m_bFeedbackEffectData = 1;
            pObject->AsSWSCreature()->SendFeedbackMessage(FEEDBACK_IMMUNE_PARALYSIS, pData);
            return 1;
        }
#if !KOTOR1  // K1 has no immunity arm for that state
    } else if (bIgnoreImmunity == 0 && nState == EFFECT_STATE_MIND_TRICK) {
        if (pStats->GetEffectImmunity(::IMMUNITY_TYPE_MIND_SPELLS, pCaster) != 0) {
            CSWCCMessageData* pData = new CSWCCMessageData();
            pData->SetObjectID(0, pObject->m_objectId);
            if (pCaster != 0) {
                CSWCCMessageData* pCasterData = new CSWCCMessageData();
                pData->CopyTo(pCasterData);
                pCaster->SendFeedbackMessage(FEEDBACK_IMMUNE_MIND_EFFECTS, pCasterData);
            }
            if (pAttack != 0)
                pAttack->m_effect.m_bFeedbackEffectData = 1;
            pObject->AsSWSCreature()->SendFeedbackMessage(FEEDBACK_IMMUNE_MIND_EFFECTS, pData);
            return 1;
        }
#endif
    } else if (bIgnoreImmunity == 0 && nState == EFFECT_STATE_STUNNED) {
        if (pStats->GetEffectImmunity(::IMMUNITY_TYPE_STUN,
                                      pCaster) != 0 || pStats->HasFeat(CSWSCreatureStats::FEAT_FORCE_IMMUNITY_STUN) || pStats->HasFeat(CSWSCreatureStats::FEAT_FORCE_IMMUNITY_PARALYSIS)) {
            CSWCCMessageData* pData = new CSWCCMessageData();
            pData->SetObjectID(0, pObject->m_objectId);
            if (pCaster != 0) {
                CSWCCMessageData* pCasterData = new CSWCCMessageData();
                pData->CopyTo(pCasterData);
                pCaster->SendFeedbackMessage(FEEDBACK_IMMUNE_STUN, pCasterData);
            }
            if (pAttack != 0)
                pAttack->m_effect.m_bFeedbackEffectData = 1;
            pObject->AsSWSCreature()->SendFeedbackMessage(FEEDBACK_IMMUNE_STUN, pData);
            return 1;
        }
    } else {
        switch (nState) {
        case EFFECT_STATE_CONFUSED:
            if (pStats->GetEffectImmunity(::IMMUNITY_TYPE_CONFUSED, pCaster) == 1) {
            CSWCCMessageData* pData = new CSWCCMessageData();
            pData->SetObjectID(0, pObject->m_objectId);
            if (pCaster != 0) {
                CSWCCMessageData* pCasterData = new CSWCCMessageData();
                pData->CopyTo(pCasterData);
                pCaster->SendFeedbackMessage(FEEDBACK_IMMUNE_CONFUSION, pCasterData);
            }
            if (pAttack != 0)
                pAttack->m_effect.m_bFeedbackEffectData = 1;
            pObject->AsSWSCreature()->SendFeedbackMessage(FEEDBACK_IMMUNE_CONFUSION, pData);
            return 1;
            }
            break;
        case EFFECT_STATE_FRIGHTENED:
#if KOTOR1  // K1 has no feat exemption here
            if (pStats->GetEffectImmunity(::IMMUNITY_TYPE_FEAR, pCaster) != 0) {
#else
            if (pStats->GetEffectImmunity(::IMMUNITY_TYPE_FEAR,
                                          pCaster) != 0 || pStats->HasFeat(FEAT_MANDALORIAN_COURAGE)) {
#endif
            CSWCCMessageData* pData = new CSWCCMessageData();
            pData->SetObjectID(0, pObject->m_objectId);
            if (pCaster != 0) {
                CSWCCMessageData* pCasterData = new CSWCCMessageData();
                pData->CopyTo(pCasterData);
                pCaster->SendFeedbackMessage(FEEDBACK_IMMUNE_FEAR, pCasterData);
            }
            if (pAttack != 0)
                pAttack->m_effect.m_bFeedbackEffectData = 1;
            pObject->AsSWSCreature()->SendFeedbackMessage(FEEDBACK_IMMUNE_FEAR, pData);
            return 1;
            }
            break;
        case EFFECT_STATE_HORRIFIED:
            if (bIgnoreImmunity == 0) {
#if KOTOR1
                if (pStats->HasFeat(CSWSCreatureStats::FEAT_FORCE_IMMUNITY_FEAR)) {
#else
                if (pStats->HasFeat(CSWSCreatureStats::FEAT_FORCE_IMMUNITY_FEAR)
                    || pStats->HasFeat(FEAT_MANDALORIAN_COURAGE)) {
#endif
            CSWCCMessageData* pData = new CSWCCMessageData();
            pData->SetObjectID(0, pObject->m_objectId);
            if (pCaster != 0) {
                CSWCCMessageData* pCasterData = new CSWCCMessageData();
                pData->CopyTo(pCasterData);
                pCaster->SendFeedbackMessage(FEEDBACK_IMMUNE_FEAR_FEAT, pCasterData);
            }
            if (pAttack != 0)
                pAttack->m_effect.m_bFeedbackEffectData = 1;
            pObject->AsSWSCreature()->SendFeedbackMessage(FEEDBACK_IMMUNE_FEAR_FEAT, pData);
            return 1;
                }
            }
            break;
#if !KOTOR1  // K1 has no immunity arms for those states
        case EFFECT_STATE_DROID_CONFUSED:
            if (pStats->GetEffectImmunity(::IMMUNITY_TYPE_DROID_CONFUSED, pCaster) == 1) {
            CSWCCMessageData* pData = new CSWCCMessageData();
            pData->SetObjectID(0, pObject->m_objectId);
            if (pCaster != 0) {
                CSWCCMessageData* pCasterData = new CSWCCMessageData();
                pData->CopyTo(pCasterData);
                pCaster->SendFeedbackMessage(FEEDBACK_IMMUNE_DROID_CONFUSION, pCasterData);
            }
            if (pAttack != 0)
                pAttack->m_effect.m_bFeedbackEffectData = 1;
            pObject->AsSWSCreature()->SendFeedbackMessage(FEEDBACK_IMMUNE_DROID_CONFUSION,
                                                          pData);
            return 1;
            }
            break;
        default:
            if (bIgnoreImmunity == 0 && nState == EFFECT_STATE_KNOCKDOWN) {
                if (pStats->GetEffectImmunity(::IMMUNITY_TYPE_KNOCKDOWN, pCaster) != 0) {
            CSWCCMessageData* pData = new CSWCCMessageData();
            pData->SetObjectID(0, pObject->m_objectId);
            if (pCaster != 0) {
                CSWCCMessageData* pCasterData = new CSWCCMessageData();
                pData->CopyTo(pCasterData);
                pCaster->SendFeedbackMessage(FEEDBACK_IMMUNE_KNOCKDOWN, pCasterData);
            }
            if (pAttack != 0)
                pAttack->m_effect.m_bFeedbackEffectData = 1;
            pObject->AsSWSCreature()->SendFeedbackMessage(FEEDBACK_IMMUNE_KNOCKDOWN, pData);
            return 1;
                }
            }
            break;
#endif
        }
    }
    CGameEffect* pStateCopy = new CGameEffect(pEffect);
    pStateCopy->m_subType = GAME_EFFECT_SET_AI_STATE;
    pStateCopy->SetInteger(0, -257);
    pCreature->ApplyEffect(pStateCopy, nParam, 0);
    if (nState <= pCreature->m_nState)
        return 0;
    pCreature->m_nState = static_cast<unsigned char>(nState);
    int i = 0;
    while (i < pObject->m_effects.m_count) {
        if (pObject->m_effects.m_pArray[i]->m_subType > GAME_EFFECT_SETSTATE_INTERNAL)
            break;
        if (pObject->m_effects.m_pArray[i]->m_subType == GAME_EFFECT_SETSTATE_INTERNAL)
            pCreature->RemoveEffectById(pObject->m_effects.m_pArray[i]->m_effectId);
        i++;
    }
    CGameEffect* pState = new CGameEffect(1);
    pState->SetCreator(pEffect->m_creatorId);
    pState->m_subType = GAME_EFFECT_SETSTATE_INTERNAL;
    pState->m_durationTypeBits =
        CGameEffect::DURATION_TYPE_INNATE | (pState->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pState->m_spellId = pEffect->m_spellId;
    pState->m_bIsExposed = 0;
    pState->SetInteger(0, nState);
    pCreature->ApplyEffect(pState, nParam, 0);
    return 0;
}

// Re-derives the strongest surviving state and makes the creature commandable again.
int CSWSEffectListHandler::OnRemoveSetState(CSWSObject* pObject, CGameEffect* pEffect) {
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0) {
        pObject->m_bCommandable = 1;
        pCreature->m_nState = EFFECT_STATE_NONE;
        int nBest = 0;
        unsigned long long idCreator = OBJECT_INVALID;
        int i = 0;
        while (i < pObject->m_effects.m_count) {
            if (pObject->m_effects.m_pArray[i]->m_subType > GAME_EFFECT_SETSTATE)
                break;
            if (pObject->m_effects.m_pArray[i] != pEffect &&
                pObject->m_effects.m_pArray[i]->m_subType == GAME_EFFECT_SETSTATE &&
                pObject->m_effects.m_pArray[i]->GetInteger(0) > nBest) {
                nBest = pObject->m_effects.m_pArray[i]->GetInteger(0);
                idCreator = pObject->m_effects.m_pArray[i]->m_creatorId;
            }
            i++;
        }
        while (i < pObject->m_effects.m_count) {
            if (pObject->m_effects.m_pArray[i]->m_subType > GAME_EFFECT_SETSTATE_INTERNAL)
                break;
            if (pObject->m_effects.m_pArray[i]->m_subType == GAME_EFFECT_SETSTATE_INTERNAL)
                pCreature->RemoveEffectById(pObject->m_effects.m_pArray[i]->m_effectId);
            else
                i++;
        }
        if (nBest != 0) {
            pCreature->m_nState = static_cast<unsigned char>(nBest);
            pObject->m_bCommandable = 0;
            CGameEffect* pState = new CGameEffect(1);
            pState->SetCreator(idCreator);
            pState->m_bIsExposed = 0;
            pState->m_durationTypeBits =
                CGameEffect::DURATION_TYPE_PERMANENT | (pState->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pState->m_subType = GAME_EFFECT_SETSTATE_INTERNAL;
            pState->SetInteger(0, nBest);
            pCreature->ApplyEffect(pState, 0, 0);
        }
    }
    return 1;
}

// Re-tallies the haste/slow tally and swaps the derived effect only when the sign changes.
int CSWSEffectListHandler::OnRemoveHasteOrSlow(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        pObject->AsSWSCreature()->m_bHasted = 0;
        pObject->AsSWSCreature()->m_bSlowed = 0;
        unsigned short nRemoved = pEffect->m_subType;
        int nTally = 0;
        for (int i = 0; i < pObject->m_effects.m_count; i++) {
            unsigned short nSubType = pObject->m_effects.m_pArray[i]->m_subType;
            if (nSubType == GAME_EFFECT_HASTE)
                nTally++;
            else if (nSubType == GAME_EFFECT_SLOW)
                nTally--;
            else if (nSubType > GAME_EFFECT_SLOW)
                break;
        }
        int nNewState = nTally + (nRemoved == 1 ? -1 : 1);
        if (nNewState > 1)
            nNewState = 1;
        if (nNewState < 0)
            nNewState = -1;
        int nOldState = nTally;
        if (nOldState > 1)
            nOldState = 1;
        if (nOldState < -1)
            nOldState = -1;
        if (nOldState != nNewState) {
            if (nOldState > 0) {
                for (int i = 0; i < pObject->m_effects.m_count; i++) {
                    CGameEffect* pOther = pObject->m_effects.m_pArray[i];
                    if (pOther->m_subType == GAME_EFFECT_HASTE_INTERNAL) {
                        pObject->RemoveEffectById(pOther->m_effectId);
                        break;
                    }
                    if (pOther->m_subType > GAME_EFFECT_HASTE_INTERNAL)
                        break;
                }
            } else if (nOldState < 0) {
                for (int i = 0; i < pObject->m_effects.m_count; i++) {
                    CGameEffect* pOther = pObject->m_effects.m_pArray[i];
                    if (pOther->m_subType == GAME_EFFECT_SLOW_INTERNAL) {
                        pObject->RemoveEffectById(pOther->m_effectId);
                        break;
                    }
                    if (pOther->m_subType > GAME_EFFECT_SLOW_INTERNAL)
                        break;
                }
            } else {
                CGameEffect* pState = new CGameEffect(1);
                pState->m_durationTypeBits = (CGameEffect::DURATION_TYPE_INNATE & ~SUBTYPE_SUPERNATURAL) |
                                             (pState->m_durationTypeBits & CGameEffect::DURATION_SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
                pState->m_bIsExposed = 0;
                pState->m_subType = nNewState > 0 ? GAME_EFFECT_HASTE_INTERNAL : GAME_EFFECT_SLOW_INTERNAL;
                pObject->ApplyEffect(pState, 0, 0);
            }
        }
    }
    return 1;
}

// The apply side of the shared haste/slow tally; the derived effect inherits the spell id.
int CSWSEffectListHandler::OnApplyHasteOrSlow(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    if (pObject->AsSWSCreature() != 0) {
        unsigned short nApplied = pEffect->m_subType;
        CSWSCreature* pCaster =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
        if (nApplied != 1 &&
            pObject->AsSWSCreature()->m_pStats->GetEffectImmunity(::IMMUNITY_TYPE_SLOW, pCaster) != 0) {
            CSWCCMessageData* pData = new CSWCCMessageData();
            pData->SetObjectID(0, pObject->m_objectId);
            if (pCaster != 0) {
                CSWCCMessageData* pCasterData = new CSWCCMessageData();
                pData->CopyTo(pCasterData);
                pCaster->SendFeedbackMessage(FEEDBACK_IMMUNE_SLOW, pCasterData);
            }
            pObject->AsSWSCreature()->SendFeedbackMessage(FEEDBACK_IMMUNE_SLOW, pData);
            return 1;
        }
        int nTally = 0;
        for (int i = 0; i < pObject->m_effects.m_count; i++) {
            unsigned short nSubType = pObject->m_effects.m_pArray[i]->m_subType;
            if (nSubType == GAME_EFFECT_HASTE)
                nTally++;
            else if (nSubType == GAME_EFFECT_SLOW)
                nTally--;
            else if (nSubType > GAME_EFFECT_SLOW)
                break;
        }
        int nNewState = nTally + (nApplied == 1 ? 1 : -1);
        if (nNewState > 1)
            nNewState = 1;
        if (nNewState < 0)
            nNewState = -1;
        int nOldState = nTally;
        if (nOldState > 1)
            nOldState = 1;
        if (nOldState < -1)
            nOldState = -1;
        if (nOldState != nNewState) {
            if (nOldState > 0) {
                for (int i = 0; i < pObject->m_effects.m_count; i++) {
                    CGameEffect* pOther = pObject->m_effects.m_pArray[i];
                    if (pOther->m_subType == GAME_EFFECT_HASTE_INTERNAL) {
                        pObject->RemoveEffectById(pOther->m_effectId);
                        break;
                    }
                    if (pOther->m_subType > GAME_EFFECT_HASTE_INTERNAL)
                        break;
                }
            } else if (nOldState < 0) {
                for (int i = 0; i < pObject->m_effects.m_count; i++) {
                    CGameEffect* pOther = pObject->m_effects.m_pArray[i];
                    if (pOther->m_subType == GAME_EFFECT_SLOW_INTERNAL) {
                        pObject->RemoveEffectById(pOther->m_effectId);
                        break;
                    }
                    if (pOther->m_subType > GAME_EFFECT_SLOW_INTERNAL)
                        break;
                }
            } else {
                CGameEffect* pState = new CGameEffect(1);
                pState->m_durationTypeBits = (CGameEffect::DURATION_TYPE_INNATE & ~SUBTYPE_SUPERNATURAL) |
                                             (pState->m_durationTypeBits & CGameEffect::DURATION_SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
                pState->m_spellId = pEffect->m_spellId;
                pState->m_bIsExposed = 0;
                pState->m_subType = nNewState > 0 ? GAME_EFFECT_HASTE_INTERNAL : GAME_EFFECT_SLOW_INTERNAL;
                pObject->ApplyEffect(pState, a, 0);
            }
        }
    }
    return 0;
}

// Applies the derived slow set of icon, AC and attack penalties, save penalty and speed cap.
int CSWSEffectListHandler::OnApplySlowInternal(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0) {
        CGameEffect* pIcon = new CGameEffect(pEffect);
        pIcon->m_subType = GAME_EFFECT_MOVEMENT_SPEED_DECREASE;
        pIcon->SetInteger(0, 50);
        pObject->ApplyEffect(pIcon, a, 0);

        CGameEffect* pAC = new CGameEffect(pEffect);
        pAC->SetInteger(0, 0);
        pAC->SetInteger(1, 2);
        pAC->SetInteger(2, g_pRules->m_nNumRaces);
        pAC->SetInteger(5, DAMAGE_TYPES_PHYSICAL);
        pAC->m_subType = GAME_EFFECT_AC_DECREASE;
        pObject->ApplyEffect(pAC, a, 0);

        CGameEffect* pSave = new CGameEffect(pEffect);
        pSave->m_subType = GAME_EFFECT_ATTACK_DECREASE;
        pSave->SetInteger(0, 2);
        pSave->SetInteger(1, 0);
        pSave->SetInteger(2, g_pRules->m_nNumRaces);
        pObject->ApplyEffect(pSave, a, 0);

        CGameEffect* pAttack = new CGameEffect(pEffect);
        pAttack->SetInteger(0, 2);
        pAttack->SetInteger(1, 2);
        pAttack->SetInteger(3, g_pRules->m_nNumRaces);
        pAttack->m_subType = GAME_EFFECT_SAVING_THROW_DECREASE;
        pObject->ApplyEffect(pAttack, a, 0);

        CGameEffect* pSpeed = new CGameEffect(pEffect);
        pSpeed->m_subType = GAME_EFFECT_LIMIT_MOVEMENT_SPEED;
        pSpeed->SetInteger(0, 0);
        pObject->ApplyEffect(pSpeed, a, 0);

        pCreature->m_bHasted = 0;
        pCreature->m_bSlowed = 1;
    }
    return 0;
}

#include "nwsspellimpact.h"

// Spawns a CSWSAreaOfEffectObject and anchors it on the target.
int CSWSEffectListHandler::OnApplyAreaOfEffect(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature == 0)
        return 1;
    CSWSAreaOfEffectObject* pAoE = new CSWSAreaOfEffectObject(OBJECT_INVALID);
    pAoE->SetCreator(pEffect->m_creatorId);
    pAoE->LoadAreaEffect(static_cast<unsigned char>(pEffect->GetInteger(0)));
    if (pEffect->GetString(0) != "")
        pAoE->m_asScripts[CSWSAreaOfEffectObject::SCRIPT_SLOT_ENTER] = pEffect->GetString(0);
    if (pEffect->GetString(1) != "")
        pAoE->m_asScripts[CSWSAreaOfEffectObject::SCRIPT_SLOT_HEARTBEAT] = pEffect->GetString(1);
    if (pEffect->GetString(2) != "")
        pAoE->m_asScripts[CSWSAreaOfEffectObject::SCRIPT_SLOT_EXIT] = pEffect->GetString(2);
    pAoE->SetEffectSpellId(pEffect->m_spellId);
    CSWSArea* pArea = static_cast<CSWSArea*>(pCreature->GetArea());
    pAoE->SetTargetObjID(pCreature->m_objectId);
    pAoE->AddToArea(pArea, pCreature->m_position.x, pCreature->m_position.y,
                    pCreature->m_position.z, 1);
    pEffect->SetObjectID(0, pAoE->m_objectId);
    return 0;
}

// An immunity to entangle or plot status rejects it; the effect expands into three derived
// penalties (reflex save, movement lock, attack).
int CSWSEffectListHandler::OnApplyEntangled(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    if (pObject->AsSWSCreature() == 0)
        return 0;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    CSWSCreature* pCaster =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
    if (pCreature->m_pStats->GetEffectImmunity(::IMMUNITY_TYPE_ENTANGLE, pCaster) != 0) {
        CSWCCMessageData* pData = new CSWCCMessageData();
        pData->SetObjectID(0, pObject->m_objectId);
        if (pCaster != 0) {
            CSWCCMessageData* pCasterData = new CSWCCMessageData();
            pData->CopyTo(pCasterData);
            pCaster->SendFeedbackMessage(FEEDBACK_IMMUNE_ENTANGLE, pCasterData);
        }
        pObject->AsSWSCreature()->SendFeedbackMessage(FEEDBACK_IMMUNE_ENTANGLE, pData);
        return 1;
    }
    if (pCreature->m_bPlot != 0)
        return 1;
    pCreature->ClearAllActions(1);
    CGameEffect* pSave = new CGameEffect(pEffect);
    CGameEffect* pImmobilize = new CGameEffect(pEffect);
    CGameEffect* pAttack = new CGameEffect(pEffect);
    pSave->m_subType = GAME_EFFECT_ATTACK_DECREASE;
    pSave->SetInteger(0, 2);
    pSave->SetInteger(1, 0);
    pSave->SetInteger(2, g_pRules->m_nNumRaces);
    pCreature->ApplyEffect(pSave, a, 0);
    pImmobilize->m_subType = GAME_EFFECT_ABILITY_DECREASE;
    pImmobilize->SetInteger(0, 1);
    pImmobilize->SetInteger(1, 4);
    pCreature->ApplyEffect(pImmobilize, a, 0);
    pAttack->m_subType = GAME_EFFECT_SET_AI_STATE;
    pAttack->SetInteger(0, ~CSWSCreature::AI_STATE_MOBILE);
    pCreature->ApplyEffect(pAttack, a, 0);
    return 0;
}

// Puts the object back at the destination point and area the effect stashed.
int CSWSEffectListHandler::OnRemoveDisappearAppear(CSWSObject* pObject, CGameEffect* pEffect) {
    Vector vStored = { pEffect->GetFloat(0), pEffect->GetFloat(1), pEffect->GetFloat(2) };
    Vector vSafe;
    unsigned long long idArea = pEffect->GetObjectID(0);
    CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(idArea);
    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        pCreature->ClearAllActions(1);
        pCreature->m_pCombatRound->SetRoundPaused(0, OBJECT_INVALID);
        pCreature->m_pCombatRound->SetPauseTimer(2000, 0);
        pCreature->m_pCombatRound->SetRoundPaused(1, OBJECT_INVALID);
        pCreature->AddAppearActions();
        if (pArea->ComputeSafeLocation(vStored, 20.0f, pCreature->m_pPathfindInfo, 0, &vSafe) != 0) {
            vSafe.z = pArea->ComputeHeight(vSafe, 0, 0, 0);
            pCreature->AddToArea(pArea, vSafe.x, vSafe.y, vSafe.z, 0, 0);
        } else {
            pEffect->SetFloat(2, pArea->ComputeHeight(vStored, 0, 0, 0));
            pCreature->AddToArea(pArea, pEffect->GetFloat(0), pEffect->GetFloat(1),
                                 pEffect->GetFloat(2), 0, 0);
        }
    } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
        CSWSPlaceable* pPlaceable = pObject->AsSWSPlaceable();
        Vector vWhere = { pEffect->GetFloat(0), pEffect->GetFloat(1), pEffect->GetFloat(2) };
        pPlaceable->AddToArea(pArea, vWhere, 0);
    }
    return 1;
}

// The effect's integer 0 is the vision-flag bit being set (blindness or darkness).
int CSWSEffectListHandler::OnApplyBlindness(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    if (pObject->AsSWSCreature() == 0)
        return 0;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    int nFlags = pEffect->GetInteger(0);
    if (pCreature == 0)
        return 1;
    if (nFlags != CSWSCreature::VISION_BLINDNESS && nFlags != CSWSCreature::VISION_DARKNESS)
        return 1;
    CSWSCreature* pCaster =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
    if (nFlags == CSWSCreature::VISION_BLINDNESS
        && pCreature->m_pStats->GetEffectImmunity(::IMMUNITY_TYPE_BLINDNESS, pCaster) != 0) {
        CSWCCMessageData* pData = new CSWCCMessageData();
        pData->SetObjectID(0, pObject->m_objectId);
        if (pCaster != 0) {
            CSWCCMessageData* pCasterData = new CSWCCMessageData();
            pData->CopyTo(pCasterData);
            pCaster->SendFeedbackMessage(FEEDBACK_IMMUNE_BLINDNESS, pCasterData);
        }
        pObject->AsSWSCreature()->SendFeedbackMessage(FEEDBACK_IMMUNE_BLINDNESS, pData);
        return 1;
    }
    if (pCreature->m_bPlot != 0)
        return 1;
    int bDarkness = nFlags == CSWSCreature::VISION_DARKNESS;
    if (nFlags == CSWSCreature::VISION_BLINDNESS
        || (nFlags == CSWSCreature::VISION_DARKNESS &&
            (pCreature->m_nVisionFlags & (CSWSCreature::VISION_ULTRAVISION | CSWSCreature::VISION_TRUE_SEEING)) == 0)) {
        CGameEffect* pPerception = new CGameEffect(pEffect);
        pPerception->m_subType = GAME_EFFECT_MISS_CHANCE;
        pPerception->SetInteger(0, 50);
        pPerception->SetInteger(1, bDarkness);
        pCreature->ApplyEffect(pPerception, a, 0);

        CGameEffect* pConcealment = new CGameEffect(pEffect);
        pConcealment->m_subType = GAME_EFFECT_VISION;
        pConcealment->SetInteger(0, 4);
        pCreature->ApplyEffect(pConcealment, a, 0);

        CGameEffect* pFeedback = new CGameEffect(pEffect);
        pFeedback->m_subType = GAME_EFFECT_VISUALEFFECT;
        pFeedback->SetInteger(0, VFX_ARD_LIGHT_BLIND);
        pCreature->ApplyEffect(pFeedback, a, 0);

        pCreature->m_nVisionFlags |= nFlags;
    }
    return 0;
}

// One bleed tick; the direction comes from the effect's float slot and the colour letter from
// appearance.2da's blood-colour column.
int CSWSEffectListHandler::OnRemoveHitPointChangeWhenDying(CSWSObject* pObject,
                                                           CGameEffect* pEffect) {
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0 && pCreature->m_pStats->m_bIsPC != 0) {
        float fRate = pEffect->GetFloat(0);
        short nHitPoints = pCreature->GetCurrentHitPoints(1);
        if (fRate > 0.0f) {
            pCreature->SetCurrentHitPoints(nHitPoints + 1);
        } else {
            pCreature->SetCurrentHitPoints(nHitPoints - 1);
            CExoString sBloodColor;
            g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
                pCreature->m_appearance.m_headId, CExoString("BLOODCOLR"), &sBloodColor);
            CGameEffect* pBlood = new CGameEffect(1);
            pBlood->m_subType = GAME_EFFECT_VISUALEFFECT;
            pBlood->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                         (pBlood->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            if (sBloodColor == "R")
                pBlood->SetInteger(0, VFX_COM_BLOOD_RED);
            else if (sBloodColor == "G")
                pBlood->SetInteger(0, VFX_COM_BLOOD_GREEN);
            else if (sBloodColor == "Y")
                pBlood->SetInteger(0, VFX_COM_BLOOD_YELLOW);
            pCreature->ApplyEffect(pBlood, 0, 0);
        }
    }
#ifdef K2_FIX_ENGINE_BUGS
    if (pCreature == 0)
        return 1;
#endif
#if KOTOR1  // a dying PC gets the bleed effect re-applied; "down" is GetDeadTemp
    if (pCreature->GetIsPCDying() != 0) {
        CGameEffect* pBleed = new CGameEffect(1);
        pBleed->SetCreator(pEffect->m_creatorId);
        pBleed->m_subType = GAME_EFFECT_HITPOINTCHANGEWHENDYING;
        pBleed->m_durationTypeBits = CGameEffect::DURATION_TYPE_TEMPORARY |
                                     (pBleed->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pBleed->m_fDuration = pEffect->m_fDuration;
        pBleed->SetFloat(0, pEffect->GetFloat(0));
        pObject->ApplyEffect(pBleed, 0, 0);
    } else if (pCreature->GetDead() != 0 || pCreature->GetDeadTemp() != 0) {
#else
    if (pCreature->GetDead() != 0 ||
        (pCreature->m_bInParty != 0 && pCreature->GetCurrentHitPoints(0) <= 0)) {
#endif
        CGameEffect* pDeath = new CGameEffect(1);
        pDeath->m_subType = GAME_EFFECT_DEATH;
        pDeath->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                     (pDeath->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pDeath->SetCreator(pEffect->m_creatorId);
        pDeath->SetInteger(0, 0);
        pDeath->SetInteger(1, 1);
        pCreature->ApplyEffect(pDeath, 0, 0);
    }
    return 1;
}

// Drains the pool and fires the death effect if that is what killed the creature.
int CSWSEffectListHandler::OnRemoveTemporaryHitpoints(CSWSObject* pObject, CGameEffect* pEffect) {
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
#if KOTOR1  // a PC that starts dying signals the module; "down" is GetDeadTemp
        int bWasDying = pCreature->GetIsPCDying();
        int bWasDead = pCreature->GetDead();
        int bWasDown = pCreature->GetDeadTemp();
        if (pEffect != 0) {
            int nRemaining = pCreature->m_nHitPointBonus16 - pEffect->GetInteger(0);
            if (nRemaining < 0)
                nRemaining = 0;
            pCreature->m_hitPointBonus = nRemaining;
        }
        if (bWasDying == 0 && pCreature->GetIsPCDying() == 1) {
            pCreature->RecomputeAmbientAnimationState();
            pCreature->m_bCommandable = 0;
            unsigned long long oidModule = g_pAppManager->m_pServerExoApp->GetModule()->m_objectId;
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_DYING;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, pCreature->m_objectId, oidModule, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            pCreature->AutoCloseGUIPanels(0);
            return 1;
        }
        if ((bWasDead == 0 && pCreature->GetDead() == 1) ||
            (bWasDown == 0 && pCreature->GetDeadTemp() == 1)) {
#else
        int bWasDead = pCreature->GetDead();
        int bWasDown = 0;
        if (pCreature->m_bInParty != 0)
            bWasDown = pCreature->GetCurrentHitPoints(0) < 1;
        if (pEffect != 0) {
            int nRemaining = 0;
            if (pCreature->m_nHitPointBonus16 > pEffect->GetInteger(0))
                nRemaining = pCreature->m_nHitPointBonus16 - pEffect->GetInteger(0);
            pCreature->m_hitPointBonus = nRemaining;
        }
        if ((bWasDead == 0 && pCreature->GetDead() == 1) ||
            (bWasDown == 0 && pCreature->m_bInParty != 0 &&
             pCreature->GetCurrentHitPoints(0) <= 0)) {
#endif
            CGameEffect* pDeath = new CGameEffect(1);
            pDeath->m_subType = GAME_EFFECT_DEATH;
            pDeath->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                         (pDeath->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pDeath->SetCreator(OBJECT_INVALID);
            CExoString sBloodColor;
            g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
                pCreature->m_appearance.m_headId, CExoString("BLOODCOLR"), &sBloodColor);
            CGameEffect* pBlood = new CGameEffect(1);
            pBlood->m_subType = GAME_EFFECT_VISUALEFFECT;
            pBlood->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                         (pBlood->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            if (sBloodColor == "R")
                pBlood->SetInteger(0, VFX_COM_BLOOD_RED);
            else if (sBloodColor == "G")
                pBlood->SetInteger(0, VFX_COM_BLOOD_GREEN);
            else if (sBloodColor == "Y")
                pBlood->SetInteger(0, VFX_COM_BLOOD_YELLOW);
            pCreature->ApplyEffect(pBlood, 0, 0);
            pDeath->SetInteger(1, 1);
            pCreature->ApplyEffect(pDeath, 0, 0);
        }
    }
    return 1;
}

#include "clientexoapp.h"
#include "swparty.h"
#include "exoresman.h"

// Spawns a party-flagged creature from the resref the effect carries; a full party refuses.
int CSWSEffectListHandler::OnApplySummonParty(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount <= 2) {
        CSWSCreature* pTarget = pObject->AsSWSCreature();
        if (pEffect != 0 && pTarget != 0) {
            CSWSCreature* pSummon = new CSWSCreature(OBJECT_INVALID, 0);
            pSummon->SetInParty(1, 1);
            CResRef resRef(pEffect->GetString(0));
            pSummon->LoadFromTemplate(resRef, 0);
            if (pEffect->GetObjectID(0) == OBJECT_INVALID) {
                pEffect->SetObjectID(0, pObject->m_cachedAreaId);
                pEffect->SetFloat(0, static_cast<CSWSObject *>(pObject)->m_position.x);
                pEffect->SetFloat(1, static_cast<CSWSObject *>(pObject)->m_position.y);
                pEffect->SetFloat(2, static_cast<CSWSObject *>(pObject)->m_position.z);
            }
            CSWSArea* pArea =
                g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(pEffect->GetObjectID(0));
            if (pArea == 0) {
                delete pSummon;
                return 0;
            }
            Vector vWanted = { pEffect->GetFloat(0), pEffect->GetFloat(1), pEffect->GetFloat(2) };
            Vector vSafe;
            float fX;
            float fY;
            float fZ;
            if (pArea->ComputeSafeLocation(vWanted, 10.0f, pSummon->m_pPathfindInfo, 0, &vSafe) ==
                1) {
                fX = vSafe.x;
                fY = vSafe.y;
                fZ = pArea->ComputeHeight(vSafe, 1, 0, 0);
            } else {
                fX = vWanted.x;
                fY = vWanted.y;
                fZ = vWanted.z;
            }
            pSummon->AddToArea(pArea, fX, fY, fZ, 0, 0);
            pSummon->m_bSpawnPerceptionDone = 1;
            g_pVirtualMachine->RunScript(&pSummon->m_scripts[CSWSCreature::CREATURE_SCRIPT_SPAWN],
                                         pSummon->m_objectId, 1);
            pEffect->SetObjectID(1, pSummon->m_objectId);
        }
        return 0;
    }
    return 1;
}

#include "Log_Base2.h"

// Turns a fraction of the target's current hit points into a single damage node.
int CSWSEffectListHandler::OnApplyAssassinate(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject == 0)
        return 1;
    if (pObject->AsSWSCreature() == 0)
        return 1;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    int nFraction = pEffect->GetInteger(0);
    short nDamage;
    if (nFraction == 1)
        nDamage = pCreature->GetCurrentHitPoints(0) / 4;
    else if (nFraction == 2)
        nDamage = pCreature->GetCurrentHitPoints(0) / 3;
    else if (nFraction == 3)
        nDamage = pCreature->GetCurrentHitPoints(0) / 2;
    else
        return 1;
    if (nDamage < 1)
        return 1;
    CGameEffect* pDamage = new CGameEffect(1);
    pDamage->m_subType = GAME_EFFECT_DAMAGE;
    pDamage->m_durationTypeBits =
        CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pDamage->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pDamage->SetCreator(pEffect->m_creatorId);
    pDamage->SetNumIntegers(22);
    for (int i = 0; i < 15; i++)
        pDamage->SetInteger(i, -1);
    pDamage->SetInteger(4, nDamage);
    pDamage->SetInteger(14, nDamage);
    pDamage->SetInteger(16, 0);
    pDamage->SetInteger(17, Log_Base2(4));
    pDamage->SetInteger(19, 1);
    pDamage->SetInteger(20, 1);
    pDamage->SetInteger(21, 0);
    pObject->ApplyEffect(pDamage, 0, 1);
    return 1;
}

#include "pathfindinfo.h"
#include "SWCollisionMesh.h"

// Shoves the target away from the effect's source point, clipped against the walkmesh; a
// blocked line stops the target where it hit.
int CSWSEffectListHandler::OnApplyForcePush(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0) {
        CGameObject* pSource =
            g_pAppManager->m_pServerExoApp->GetGameObject(pEffect->m_creatorId);
        if (pSource != 0 && pSource->AsSWSObject() != 0) {
            Vector vSource;
            bool bClipToWalkmesh;
            if (pEffect->GetInteger(0) != 0) {
                vSource.x = pEffect->GetFloat(0);
                vSource.y = pEffect->GetFloat(1);
                vSource.z = pEffect->GetFloat(2);
                bClipToWalkmesh = pEffect->GetInteger(1) == 0;
            } else {
                vSource = pSource->AsSWSObject()->m_position;
                bClipToWalkmesh = 1;
            }
            float fDistance = pEffect->GetFloat(3);
            if (fDistance == 0.0f)
                fDistance = 5.0f;
            Vector vDirection = normalize(pCreature->m_position - vSource);
            Vector vDestination = pCreature->m_position + vDirection * fDistance;
            pCreature->SetOrientation(-vDirection);
            if (bClipToWalkmesh) {
                CWalkHitInfo hitInfo = {};
                hitInfo.m_hitObjectId = OBJECT_INVALID;
                hitInfo.m_hitSubType = 0;
                int nHit = g_pAppManager->m_pServerExoApp->TestDirectLine(
                    pCreature->m_objectId, static_cast<CSWSArea*>(pCreature->GetArea())->m_resRef,
                    pCreature->m_position, vDestination,
                    static_cast<float>(pCreature->m_pPathfindInfo->m_fPersonalSpace),
                    pCreature->m_pPathfindInfo->m_fHeight, 1, &hitInfo);
                if (nHit == 0)
                    vDestination = pCreature->m_position;
                else if (nHit != 1)
                    vDestination = hitInfo.m_hitPosition;
            }
            float* paPath = new float[4];
            paPath[0] = pCreature->m_position.x;
            paPath[1] = pCreature->m_position.y;
            paPath[2] = vDestination.x;
            paPath[3] = vDestination.y;
            if (pCreature->m_pPathfindInfo->m_nWayPointFloats != 0) {
                float* paOld = pCreature->m_pPathfindInfo->m_paWayPoints;
                pCreature->m_pPathfindInfo->m_paWayPoints = 0;
                delete[] paOld;
            }
            pCreature->m_pPathfindInfo->m_paWayPoints = paPath;
            pCreature->m_pPathfindInfo->m_nWayPointCursor = 0;
            pCreature->m_pPathfindInfo->m_nWayPointFloats = 4;
            pCreature->m_nMoveState = 4;
            pCreature->WalkUpdateLocation_QuickWalk(1);

            CGameEffect* pKnockback = new CGameEffect(pEffect);
            pKnockback->m_subType = GAME_EFFECT_SETSTATE;
            pKnockback->m_durationTypeBits =
                CGameEffect::DURATION_TYPE_TEMPORARY | (pKnockback->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pKnockback->SetInteger(0, 9);
            pCreature->ApplyEffect(pKnockback, 0, 0);
        }
    }
    return 0;
}

// Applies the landing half of Force Jump. The jumper stops one attack range short of its
// target, and the target is shoved further out when that spot is not standable.
int CSWSEffectListHandler::OnApplyForceJumpDelayed(CSWSObject* pObject, CGameEffect* pEffect,
                                                   int a) {
    K2_UNUSED(a);
    CSWSCreature* pJumper = pObject->AsSWSCreature();
    if (pJumper != 0) {
        CSWSCreature* pTarget =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->GetObjectID(0));
        if (pTarget != 0) {
            pJumper->SetLockOrientationToObject(pTarget->m_objectId, 0);
            Vector vToTarget = normalize(pTarget->m_position - pJumper->m_position);
            Vector vLanding =
                pTarget->m_position - vToTarget * pJumper->DesiredAttackRange(pTarget->m_objectId, 1);
            if (g_pAppManager->m_pServerExoApp->TestSafeLocation(pTarget->m_objectId, vLanding) ==
                0) {
                Vector vShoved =
                    pTarget->m_position +
                    vToTarget * pJumper->DesiredAttackRange(pTarget->m_objectId,
                                                            pJumper->m_pCombatRound->m_bEngaged);
                vLanding = pTarget->m_position;
                if (pTarget->m_pPathfindInfo->m_nWayPointFloats != 0) {
                    float* paOld = pTarget->m_pPathfindInfo->m_paWayPoints;
                    pTarget->m_pPathfindInfo->m_paWayPoints = 0;
                    delete[] paOld;
                }
                float* paShovePath = new float[4];
                paShovePath[0] = pTarget->m_position.x;
                paShovePath[1] = pTarget->m_position.y;
                paShovePath[2] = vShoved.x;
                paShovePath[3] = vShoved.y;
                pTarget->AddMoveToPointActionToFront(ACTION_GROUP_NEW, vShoved, pTarget->m_cachedAreaId,
                                                     OBJECT_INVALID, 0, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0,
                                                     Vector(), 0, 0, 0);
                pTarget->m_nMoveState = 6;
                pTarget->m_pPathfindInfo->m_nWayPointFloats = 4;
                pTarget->m_pPathfindInfo->m_paWayPoints = paShovePath;
                pTarget->m_pPathfindInfo->m_nWayPointCursor = 0;
                pTarget->WalkUpdateLocation_QuickWalk(1);

                CGameEffect* pKnockback = new CGameEffect(pEffect);
                pKnockback->m_subType = GAME_EFFECT_SETSTATE;
                pKnockback->m_durationTypeBits =
                    CGameEffect::DURATION_TYPE_TEMPORARY | (pKnockback->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
                pKnockback->SetInteger(0, EFFECT_STATE_KNOCKBACK);
                pTarget->ApplyEffect(pKnockback, 0, 0);
            }
            if (pJumper->m_pPathfindInfo->m_nWayPointFloats != 0) {
                float* paOld = pJumper->m_pPathfindInfo->m_paWayPoints;
                pJumper->m_pPathfindInfo->m_paWayPoints = 0;
                delete[] paOld;
            }
            float* paJumpPath = new float[4];
            paJumpPath[0] = pJumper->m_position.x;
            paJumpPath[1] = pJumper->m_position.y;
            paJumpPath[2] = vLanding.x;
            paJumpPath[3] = vLanding.y;
            pJumper->m_pPathfindInfo->m_paWayPoints = paJumpPath;
            pJumper->m_pPathfindInfo->m_nWayPointFloats = 4;
            pJumper->m_pPathfindInfo->m_nWayPointCursor = 0;
            pJumper->m_nMoveState = 5;
            pJumper->WalkUpdateLocation_QuickWalk(1);
        }
    }
    return 0;
}

// Runs one bleed tick: a hit point, a blood splash, then re-arm or death.
int CSWSEffectListHandler::OnRemoveWounding(CSWSObject* pObject, CGameEffect* pEffect) {
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature != 0) {
#if KOTOR1  // "down" is GetDeadTemp (a discarded GetIsPCDying first)
        pCreature->GetIsPCDying();
        int bWasDead = pCreature->GetDead();
        int bWasDown = pCreature->GetDeadTemp();
#else
        int bWasDead = pCreature->GetDead();
        int bWasDown = 0;
        if (pCreature->m_bInParty != 0)
            bWasDown = pCreature->GetCurrentHitPoints(0) < 1;
#endif
        pCreature->SetCurrentHitPoints(pCreature->GetCurrentHitPoints(1) - 1);
        CExoString sBloodColor;
        g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
            pCreature->m_appearance.m_headId, CExoString("BLOODCOLR"), &sBloodColor);
        CGameEffect* pBlood = new CGameEffect(1);
        pBlood->m_subType = GAME_EFFECT_VISUALEFFECT;
        pBlood->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                     (pBlood->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        if (sBloodColor == "R")
            pBlood->SetInteger(0, VFX_COM_BLOOD_RED);
        else if (sBloodColor == "G")
            pBlood->SetInteger(0, VFX_COM_BLOOD_GREEN);
        else if (sBloodColor == "Y")
            pBlood->SetInteger(0, VFX_COM_BLOOD_YELLOW);
        pCreature->ApplyEffect(pBlood, 0, 0);
#if KOTOR1
        if ((bWasDead == 0 && pCreature->GetDead() != 0) ||
            (bWasDown == 0 && pCreature->GetDeadTemp() != 0)) {
#else
        if ((bWasDead == 0 && pCreature->GetDead() != 0) ||
            (bWasDown == 0 && pCreature->m_bInParty != 0 &&
             pCreature->GetCurrentHitPoints(0) <= 0)) {
#endif
            CGameEffect* pDeath = new CGameEffect(1);
            pDeath->m_subType = GAME_EFFECT_DEATH;
            pDeath->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                         (pDeath->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pDeath->SetCreator(pEffect->m_creatorId);
            pDeath->SetInteger(0, 0);
            pDeath->SetInteger(1, 1);
            pCreature->ApplyEffect(pDeath, 0, 0);
        } else {
            CGameEffect* pBleed = new CGameEffect(1);
            pBleed->SetCreator(pEffect->m_creatorId);
            pBleed->m_subType = GAME_EFFECT_WOUNDING;
            pBleed->m_durationTypeBits = CGameEffect::DURATION_TYPE_TEMPORARY |
                                         (pBleed->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pBleed->m_fDuration = 6.0f;
            pObject->ApplyEffect(pBleed, 0, 0);
        }
    }
    return 1;
}

// One-shot visuals are broadcast to the client; looping ones join the object's effect list.
int CSWSEffectListHandler::OnApplyVisualEffect(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject == 0 || pEffect == 0)
        return 1;
    unsigned long long oidSource = pObject->m_objectId;
    unsigned long long oidTarget;
    if (pEffect->GetObjectID(0) == OBJECT_INVALID)
        oidTarget = pEffect->m_creatorId;
    else
        oidTarget = pEffect->GetObjectID(0);
    unsigned long long oidBeamEnd = pEffect->GetObjectID(1);
    int nVisualEffect = pEffect->GetInteger(0);
    int nBodyPart = pEffect->GetInteger(1);
    int nMode = pEffect->GetInteger(2);
    float fDuration = pEffect->m_fDuration;
    Vector vImpact;
    unsigned char nFlag = 1;
    if (nMode == 1) {
        vImpact = pObject->CalculateSpellRangedMissTarget(pEffect->m_creatorId,
                                                          pObject->m_objectId);
        nFlag = vImpact.z != static_cast<CSWSObject *>(pObject)->m_position.z + 1.0f ? 2 : 1;
        oidSource = OBJECT_INVALID;
    }
    C2DA* p2DA = g_pRules->m_p2DArrays->m_pVisualEffect;
    CExoString sType;
    CExoString sRow(nVisualEffect);
    int bFound = p2DA->GetCExoStringEntry(sRow, CExoString("Type_FD"), &sType);
    bool bFireAndForget = bFound == 1 && sType == "F";
    if (!bFireAndForget && nMode != 1 &&
        (pEffect->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_INSTANTANEOUS) {
        pObject->AddLoopingVisualEffect(static_cast<unsigned short>(nVisualEffect), oidTarget, static_cast<unsigned char>(nBodyPart));
        return 0;
    }
    float fX = static_cast<CSWSObject *>(pObject)->m_position.x;
    float fY = static_cast<CSWSObject *>(pObject)->m_position.y;
    float fZ = static_cast<CSWSObject *>(pObject)->m_position.z;
    if (pObject->GetArea() != 0 &&
        (CLoopingVisualEffect::GetIsBeam(static_cast<unsigned short>(nVisualEffect)) == 0 ||
         g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget) != 0)) {
        CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(
            g_pAppManager->m_pServerExoApp->GetPlayerCreatureId());
        if (pPlayer != 0) {
            CSWSObject* pPlayerObject = pPlayer->GetGameObject();
            float fDX = pPlayerObject->m_position.x - fX;
            float fDY = pPlayerObject->m_position.y - fY;
            float fDZ = pPlayerObject->m_position.z - fZ;
            float fRange = CSWSArea::MAX_OBJECT_VISIBLE_DISTANCE + 2.0f;
            if (fDX * fDX + fDY * fDY + fDZ * fDZ <= fRange * fRange)
                g_pAppManager->m_pServerExoApp->GetSWSMessage()
                    ->SendServerToPlayerGameObjUpdateVisEffect(pPlayer, static_cast<unsigned short>(nVisualEffect), oidSource,
                                                               oidTarget, oidBeamEnd, static_cast<unsigned char>(nBodyPart), nFlag,
                                                               vImpact, fDuration);
        }
    }
    return 1;
}

// Six ability-decrease clones, one per ability; constitution is queued through the AI master
// instead, so the hit-point pool does not change underneath the effect list being walked.
int CSWSEffectListHandler::OnApplyCurse(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature == 0)
        return 0;
    if (pCreature->m_bPlot != 0)
        return 1;
    CSWSCreature* pCaster =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
    if (pCreature->m_pStats->GetEffectImmunity(::IMMUNITY_TYPE_CURSED, pCaster) != 0) {
        CSWCCMessageData* pData = new CSWCCMessageData();
        pData->SetObjectID(0, pObject->m_objectId);
        if (pCaster != 0) {
            CSWCCMessageData* pCasterData = new CSWCCMessageData();
            pData->CopyTo(pCasterData);
            pCaster->SendFeedbackMessage(FEEDBACK_IMMUNE_CURSE, pCasterData);
        }
        pObject->AsSWSCreature()->SendFeedbackMessage(FEEDBACK_IMMUNE_CURSE, pData);
        return 1;
    }
    CGameEffect* pStrength = new CGameEffect(pEffect);
    pStrength->m_subType = GAME_EFFECT_ABILITY_DECREASE;
    pStrength->SetInteger(0, 0);
    pStrength->SetInteger(1, pEffect->GetInteger(0));
    pObject->ApplyEffect(pStrength, a, 0);

    CGameEffect* pDexterity = new CGameEffect(pEffect);
    pDexterity->m_subType = GAME_EFFECT_ABILITY_DECREASE;
    pDexterity->SetInteger(0, 1);
    pDexterity->SetInteger(1, pEffect->GetInteger(1));
    pObject->ApplyEffect(pDexterity, a, 0);

    CGameEffect* pConstitution = new CGameEffect(pEffect);
    pConstitution->m_subType = GAME_EFFECT_ABILITY_DECREASE;
    pConstitution->SetInteger(0, 2);
    pConstitution->SetInteger(1, pEffect->GetInteger(2));
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
        0, 0, pCaster->m_objectId, pObject->m_objectId, CServerAIMaster::EVENT_APPLY_EFFECT,
        pConstitution);

    CGameEffect* pIntelligence = new CGameEffect(pEffect);
    pIntelligence->m_subType = GAME_EFFECT_ABILITY_DECREASE;
    pIntelligence->SetInteger(0, 3);
    pIntelligence->SetInteger(1, pEffect->GetInteger(3));
    pObject->ApplyEffect(pIntelligence, a, 0);

    CGameEffect* pWisdom = new CGameEffect(pEffect);
    pWisdom->m_subType = GAME_EFFECT_ABILITY_DECREASE;
    pWisdom->SetInteger(0, 4);
    pWisdom->SetInteger(1, pEffect->GetInteger(4));
    pObject->ApplyEffect(pWisdom, a, 0);

    CGameEffect* pCharisma = new CGameEffect(pEffect);
    pCharisma->m_subType = GAME_EFFECT_ABILITY_DECREASE;
    pCharisma->SetInteger(0, 5);
    pCharisma->SetInteger(1, pEffect->GetInteger(5));
    pObject->ApplyEffect(pCharisma, a, 0);
    return 0;
}

// Runs the damage pipeline, with the per-type immunity/resistance/reduction chain, the party
// feedback broadcasts, the AI counter-attack and the death handlers. A creature target runs the
// back half on the creature; doors and placeables run it on the object itself.
int CSWSEffectListHandler::OnApplyDamage(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    CExoString sDebug;
    int anDamage[15];
    int nDeadGate;

    nDeadGate = 0;
#if KOTOR1  // the dying gate is GetDeadTemp()
    if (pObject != 0 && pObject->AsSWSCreature() != 0)
        nDeadGate = pObject->AsSWSCreature()->GetDeadTemp();
#else
    if (pObject != 0 && pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCre = pObject->AsSWSCreature();
        nDeadGate = pCre->m_bInParty != 0 && pCre->GetCurrentHitPoints(0) < 1;
    }
#endif
    K2_ASSUME(pObject != 0);
    if (nDeadGate | pObject->GetDead())
        return 1;

    int64_t i;
    for (i = 0; i < 15; i++) {
        int nDmg = pEffect->GetInteger(static_cast<int>(i));
        if ((pObject->m_bPlot | g_bDisableDamage) != 0 && nDmg >= 0)
            nDmg = 0;
        anDamage[i] = nDmg;
    }
    if ((anDamage[14] | pObject->m_bPlot) == 0)
        return 1;

    pEffect->GetInteger(15);
    int nPause = pEffect->GetInteger(16);
    int nFlags = pEffect->GetInteger(17);
    pEffect->GetInteger(18);
    int nGate13 = pEffect->GetInteger(19);
    int nGate14 = pEffect->GetInteger(20);
    int nNoBroadcast;
    if (pEffect->m_numIntegers >= 22)
        nNoBroadcast = pEffect->GetInteger(21);
    else
        nNoBroadcast = 0;

    if (g_bEnableCombatDebugging != 0) {
        CExoString sTemp = pEffect->GetString(0);
        sDebug = sTemp;
    }
    unsigned long long oidAttacker = pEffect->m_creatorId;

    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCr = pObject->AsSWSCreature();
        CGameObject* pAttGO = g_pAppManager->m_pServerExoApp->GetGameObject(oidAttacker);
        CSWSCreature* pAttCr = pAttGO != 0 ? pAttGO->AsSWSCreature() : 0;
#if KOTOR1  // a PC that starts dying here signals the module
        int bWasDying = pCr->GetIsPCDying();
#endif
        int nFinal = anDamage[14];
        if (nGate13 == 0) {
            int nRedArg = (nFlags & DAMAGE_TYPES_PHYSICAL) == 0 ? 6 : 0;
            nFinal = pCr->DoDamageImmunity(pAttCr, anDamage[14], static_cast<unsigned short>(nFlags), 0, 0);
            nFinal = pCr->DoDamageResistance(pAttCr, nFinal, static_cast<unsigned short>(nFlags), 0, 0, 0);
            if ((nFlags & DAMAGE_TYPES_PHYSICAL) != 0)
                nFinal = pCr->DoDamageReduction(pAttCr, nFinal, static_cast<unsigned char>(nRedArg), 0, 0);
            for (i = 0; i < 15; i++)
                if (anDamage[i] >= 0)
                    anDamage[i] = nFinal;
        }
        pCr->m_lastDamagerId = oidAttacker;
        int nFound = 0;
        if (pCr->m_bPoisoned != 0 && pCr->m_effects.m_count >= 1) {
            int64_t n;
            for (n = 0; n < pCr->m_effects.m_count; n++) {
                CGameEffect* pEf = pCr->m_effects.m_pArray[n];
                if (pEf->m_subType == GAME_EFFECT_POISON && pEf->m_effectId == pEffect->m_effectId) {
                    nFound = 1;
                    break;
                }
            }
        }
        if (nFound == 0)
            pCr->SetLastHostileActor(oidAttacker, 0);
        for (i = 0; i < 15; i++)
            pCr->m_paLastDamageAmounts[i] = anDamage[i];
        if (nGate13 != 0 && nGate14 == 0)
            pCr->ResolveDamageShields(pAttCr);
        if (nFinal > 0) {
#if !KOTOR1  // K1 has no mind-trick effects to break
            pCr->RemoveMindTrickEffects();
#endif
            CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
            if (pClient->GetPlayerCreatureId() == pClient->ServerToClientObjectId(oidAttacker)) {
                pClient->AddFloatyText(pClient->ServerToClientObjectId(pCr->m_objectId),
                                       CClientExoApp::FLOATY_DAMAGE, nFinal);
                pCr->DoDamage(nFinal, 1);
            } else {
                pCr->DoDamage(nFinal, 0);
            }
            int nAnim = pCr->m_nAnimation;
            if (static_cast<unsigned>(nAnim - SERVER_ANIMATION_CONJURE_1) < 2
                || nAnim == SERVER_ANIMATION_CONJURE_3) {
                CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(pCr->m_castSpellId);
                if (pSpell != 0 && pSpell->m_bUseConcentration == 1) {
                    unsigned int nRoll = g_pRules->RollDice(1, 20);
                    pCr->GetNearestEnemy(4.0f, OBJECT_INVALID, OBJECT_INVALID);
                    int nLevel;
                    if (pCr->m_nCastClassIndex == CSWSObject::CAST_CLASS_NONE)
                        nLevel = pSpell->m_nSpellLevelOther;
                    else
#if KOTOR1  // GetClass is out of line
                        nLevel = pSpell->GetSpellLevel(pCr->m_pStats->GetClass(pCr->m_nCastClassIndex));
#else
                        nLevel = pSpell->GetSpellLevel(
                            pCr->m_nCastClassIndex >= pCr->m_pStats->m_numClasses
                                ? CLASS_TYPE_INVALID
                                : pCr->m_pStats->m_classInfo[pCr->m_nCastClassIndex].m_class);
#endif
                    int nMax = nFinal + nLevel + 10;
                    nMax = nMax > 0 ? nMax : 1;
                    if (nMax > static_cast<int>(nRoll & 0xffff)) {
                        pCr->DeleteCurrentAIAction();
                        pCr->SetAnimation(SERVER_ANIMATION_DAMAGE);
                        pCr->DecrementSpellReadyCount(pCr->m_castSpellId, pCr->m_nCastClassIndex,
                                                      0, 0, 0xff);
                        CGameEffect* pCounter = new CGameEffect(1);
                        pCounter->m_subType = GAME_EFFECT_VISUALEFFECT;
                        pCounter->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                                       (pCounter->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
                        pCounter->SetCreator(pCr->m_objectId);
                        pCr->ApplyEffect(pCounter, a, 0);
                    }
                }
            }
            if (nGate13 == 0) {
                if (pCr->IsDebilitated(1) == 0) {
                    if (anDamage[7] > 0 || anDamage[9] >= 1) {
                        if ((pCr->m_nAnimation & ~1) == SERVER_ANIMATION_PAUSE) {
                            CSWSCombatRound* pRound = pCr->m_pCombatRound;
                            if (pRound->m_bRoundPaused == 0) {
                                pRound->SetRoundPaused(1, oidAttacker);
                                pRound->SetPauseTimer(nPause, 0);
                                pRound->DecrementRoundLength(nPause, 0);
                            }
                            pCr->SetAnimation(SERVER_ANIMATION_SPASM);
                        }
                    } else {
                        pCr->SetAnimation(SERVER_ANIMATION_DAMAGE_GENERIC);
                    }
                }
            }
        }
        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(pCr->m_objectId);
#if KOTOR1
        if (pCr->m_bPlayerCharacter == 0 && pCr->GetDead() == 0 && pCr->GetDeadTemp() == 0 &&
#else
        if (pCr->m_bPlayerCharacter == 0 && pCr->GetDead() == 0 &&
            (pCr->m_bInParty == 0 || pCr->GetCurrentHitPoints(0) >= 1) &&
#endif
            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsLeader(pCr->m_objectId) == 0)
            g_pVirtualMachine->RunScript(&pCr->m_scripts[CSWSCreature::CREATURE_SCRIPT_DAMAGED],
                                         pCr->m_objectId, 1);
        if (nNoBroadcast == 0) {
            if (pAttGO != 0) {
                CSWCCMessageData* pData = new CSWCCMessageData();
                pData->SetObjectID(0, oidAttacker);
                pData->SetObjectID(1, pCr->m_objectId);
                int64_t n;
                for (n = 0; n < 15; n++)
                    pData->SetInteger(static_cast<int>(n), anDamage[n]);
                if (g_bEnableCombatDebugging != 0)
                    pData->SetString(0, sDebug);
                pCr->BroadcastDamageDataToParty(pData, 0);
                if (pAttCr != 0 && pAttCr->m_pStats->m_factionId != pCr->m_pStats->m_factionId)
                    pAttCr->BroadcastDamageDataToParty(pData, 0);
                delete pData;
            } else {
                CSWCCMessageData* pData = new CSWCCMessageData();
                pData->SetObjectID(0, pCr->m_objectId);
                int64_t n;
                for (n = 0; n < 15; n++)
                    pData->SetInteger(static_cast<int>(n), anDamage[n]);
                pCr->BroadcastDamageDataToParty(pData, 1);
                delete pData;
            }
        }
#if KOTOR1  // the dying PC drops to a random injured idle; "down" is GetDeadTemp()
        if (bWasDying == 0 && pCr->GetIsPCDying() != 0) {
            unsigned char nState = pCr->m_ambientAnimationState;
            if (nState != EFFECT_STATE_DROID_STUN && nState != EFFECT_STATE_STUNNED
                && nState != EFFECT_STATE_KNOCKDOWN) {
                if (rand_wincompatible() & 1)
                    pCr->SetAmbientAnimationState(3);
                else
                    pCr->SetAmbientAnimationState(4);
                pCr->SetAnimation(SERVER_ANIMATION_PAUSE);
            }
            pCr->m_bCommandable = 0;
            unsigned long long oidModule = g_pAppManager->m_pServerExoApp->GetModule()->m_objectId;
            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_DYING;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, pCr->m_objectId, oidModule, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            pCr->AutoCloseGUIPanels(0);
            return 1;
        }
        if (pCr->GetDead() == 0 && pCr->GetDeadTemp() == 0)
            return 1;
#else
        if (pCr->GetDead() == 0) {
            if (pCr->m_bInParty != 0) {
                if (pCr->GetCurrentHitPoints(0) > 0)
                    return 1;
            } else {
                return 1;
            }
        }
#endif
        CGameEffect* pDeath = new CGameEffect(1);
        pDeath->m_subType = GAME_EFFECT_DEATH;
        pDeath->m_durationTypeBits =
            CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pDeath->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pDeath->SetCreator(oidAttacker);
        if (pAttCr != 0) {
            CSWSCombatRound* pRound = pAttCr->m_pCombatRound;
            if (pRound->GetAttack(pRound->m_currentAttack)->m_bRangedAttack == 0)
                pDeath->SetInteger(0, 1);
        }
        pDeath->SetInteger(1, 1);
        pCr->ApplyEffect(pDeath, a, 0);
        return 1;
    }

    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE ||
        pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR) {
        CSWSCreature* pAttCr = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidAttacker);
        int nFinal = anDamage[14];
        if (nGate13 == 0) {
            int nRedArg = (nFlags & DAMAGE_TYPES_PHYSICAL) == 0 ? 6 : 0;
            nFinal = pObject->DoDamageImmunity(pAttCr, anDamage[14], static_cast<unsigned short>(nFlags), 0, 0);
            nFinal = pObject->DoDamageResistance(pAttCr, nFinal, static_cast<unsigned short>(nFlags), 0, 0, 0);
            if ((nFlags & DAMAGE_TYPES_PHYSICAL) != 0)
                nFinal = pObject->DoDamageReduction(pAttCr, nFinal, static_cast<unsigned char>(nRedArg), 0, 0);
            if (pObject->m_nAnimation == PLACEABLE_ANIM_CLOSE
                || pObject->m_nAnimation == SERVER_ANIMATION_CLOSED) {
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, oidAttacker, pObject->m_objectId, CServerAIMaster::EVENT_PLAY_ANIMATION,
                    reinterpret_cast<void*>(static_cast<int64_t>(pObject->m_nAnimation)));
                pObject->SetAnimation(SERVER_ANIMATION_DAMAGE);
            }
        }
        CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
        if (pClient->GetPlayerCreatureId() == pClient->ServerToClientObjectId(oidAttacker))
            pClient->AddFloatyText(pClient->ServerToClientObjectId(pObject->m_objectId),
                                   CClientExoApp::FLOATY_DAMAGE, nFinal);
        pObject->DoDamage(nFinal);
        CScriptEvent* pEvent = new CScriptEvent();
        pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_DAMAGED;
        pEvent->SetInteger(0, nFinal);
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
            0, 0, oidAttacker, pObject->m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
        CSWCCMessageData* pData = new CSWCCMessageData();
        pData->SetObjectID(0, oidAttacker);
        pData->SetObjectID(1, pObject->m_objectId);
        int64_t n;
        for (n = 0; n < 15; n++)
            pData->SetInteger(static_cast<int>(n), anDamage[n]);
        if (g_bEnableCombatDebugging != 0)
            pData->SetString(0, sDebug);
        if (pAttCr != 0) {
            pAttCr->BroadcastCombatStateToParty(OBJECT_INVALID);
            pAttCr->BroadcastDamageDataToParty(pData, 0);
        }
        delete pData;
        if (pObject->GetDead() == 1) {
            if (pObject->AsSWSDoor() != 0) {
                pObject->m_bPlot = 1;
                pObject->AsSWSDoor()->m_bLocked = 0;
                pObject->AsSWSDoor()->OpenDoor(oidAttacker);
            } else if (pObject->AsSWSPlaceable() != 0) {
                if (pObject->AsSWSPlaceable()->m_bHasInventory != 0) {
                    pObject->m_bPlot = 1;
                    pObject->AsSWSPlaceable()->m_bLocked = 0;
                    unsigned long long* pCell = new unsigned long long;
                    *pCell = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
#if KOTOR1  // the broken container always opens (no player-proximity test)
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                        0, 0, *pCell, pObject->m_objectId,
                        CServerAIMaster::EVENT_PLAY_ANIMATION, reinterpret_cast<void*>(PLACEABLE_ANIM_OPEN));
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                        0, 1000, *pCell, pObject->m_objectId,
                        CServerAIMaster::EVENT_OPEN_OBJECT, pCell);
#else
                    CGameObject* pPlayerGO = g_pAppManager->m_pServerExoApp->GetGameObject(*pCell);
                    if (pPlayerGO != 0) {
                        Vector vDist = static_cast<CSWSObject *>(pObject)->m_position - static_cast<CSWSObject *>(pPlayerGO)->m_position;
                        if (magnitude(vDist) < 3.0f) {
                            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                                0, 0, *pCell, pObject->m_objectId,
                                CServerAIMaster::EVENT_PLAY_ANIMATION, reinterpret_cast<void*>(PLACEABLE_ANIM_OPEN));
                            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                                0, 1000, *pCell, pObject->m_objectId,
                                CServerAIMaster::EVENT_OPEN_OBJECT, pCell);
                        }
                    }
#endif
                } else {
                    CGameEffect* pPlain = new CGameEffect(1);
                    pPlain->m_subType = GAME_EFFECT_DEATH;
                    pPlain->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                                 (pPlain->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
                    pPlain->SetCreator(oidAttacker);
                    pPlain->SetInteger(0, 0);
                    pPlain->SetInteger(1, 1);
                    pObject->ApplyEffect(pPlain, a, 0);
                }
            }
            if (pAttCr != 0)
                pAttCr->ClearAllActions(1);
        }
    }
    return 1;
}

// An immunity to knockdown or plot status refuses it; a dead or downed creature is skipped.
// The fall animation is picked from the dot product of the attacker direction against the
// creature's facing.
int CSWSEffectListHandler::OnApplyKnockdown(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    unsigned long long oidCreator = pEffect->m_creatorId;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature == 0)
        return 0;
    CSWSCreature* pCaster =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreator);
    if (pObject->AsSWSCreature()->m_pStats->GetEffectImmunity(::IMMUNITY_TYPE_KNOCKDOWN, pCaster) != 0) {
        CSWCCMessageData* pData = new CSWCCMessageData();
        pData->SetObjectID(0, pObject->m_objectId);
        if (pCaster != 0) {
            CSWCCMessageData* pCasterData = new CSWCCMessageData();
            pData->CopyTo(pCasterData);
            pCaster->SendFeedbackMessage(FEEDBACK_IMMUNE_KNOCKDOWN, pCasterData);
        }
        pObject->AsSWSCreature()->SendFeedbackMessage(FEEDBACK_IMMUNE_KNOCKDOWN, pData);
        return 1;
    }
    if (pCreature->m_bPlot != 0)
        return 1;
#if !KOTOR1  // K1 has no test for that state
    if (pCreature->m_nState == EFFECT_STATE_KNOCKDOWN)
        return 1;
#endif
    if (pCreature->GetDead() != 0)
        return 0;
#if KOTOR1  // the dying tests are GetIsPCDying() / GetDeadTemp()
    if (pCreature->GetIsPCDying() != 0 || pCreature->GetDeadTemp() != 0)
#else
    if (pCreature->m_bInParty != 0 && pCreature->GetCurrentHitPoints(0) < 1)
#endif
        return 0;
    pCreature->m_pCombatRound->SetRoundPaused(1, oidCreator);
    pCreature->m_pCombatRound->SetPauseTimer(0, 1);
    CGameEffect* pStun = new CGameEffect(pEffect);
    pStun->m_fDuration = pEffect->m_fDuration + 1.5f;
    pStun->m_subType = GAME_EFFECT_SET_AI_STATE;
    pStun->SetInteger(0, -0x101);
    pObject->ApplyEffect(pStun, a, 0);
    pObject->m_bCommandable = 0;
    CGameObject* pSource;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidCreator, &pSource) ==
        CGameObjectArray::SUCCESS) {
        Vector vToward = normalize(pSource->AsSWSCreature()->m_position - pCreature->m_position);
        Vector vFacing = normalize(pCreature->m_orientation);
        if (static_cast<double>(vToward.x * vFacing.x + vToward.y * vFacing.y + vToward.z * vFacing.z) >=
            0.707)
            pCreature->SetAnimation(SERVER_ANIMATION_KNOCKDOWN_FROM_FRONT);
        else
            pCreature->SetAnimation(SERVER_ANIMATION_KNOCKDOWN_FROM_BEHIND);
    } else {
        pCreature->SetAnimation(SERVER_ANIMATION_KNOCKDOWN_FROM_FRONT);
    }
#if !KOTOR1  // K1 applies no knock-back marker effect
    CGameEffect* pKnock = new CGameEffect(pEffect);
    pKnock->m_subType = GAME_EFFECT_SETSTATE;
    pKnock->m_durationTypeBits =
        CGameEffect::DURATION_TYPE_TEMPORARY | (pKnock->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pKnock->SetInteger(0, EFFECT_STATE_KNOCKDOWN);
    pCreature->ApplyEffect(pKnock, 0, 0);
#endif
    return 0;
}

// Promotes the instantaneous effect to permanent, runs the contraction gauntlet and schedules
// the first incubation tick.
int CSWSEffectListHandler::OnApplyDisease(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    if (pObject->AsSWSCreature() == 0)
        return 1;
    if (a == 0) {
        if ((pEffect->m_durationTypeBits & 7) == CGameEffect::DURATION_TYPE_INSTANTANEOUS)
            pEffect->m_durationTypeBits =
                CGameEffect::DURATION_TYPE_PERMANENT | (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        C2DA* pDisease = g_pRules->m_p2DArrays->m_pDisease;
        CWorldTimer* pTimer = g_pAppManager->m_pServerExoApp->GetWorldTimer();
        int nRow = pEffect->GetInteger(0);
        if (pObject->AsSWSCreature()->m_bPlot != 0)
            return 1;
        CSWSCreature* pCaster =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEffect->m_creatorId);
        if (pObject->AsSWSCreature()->m_pStats->GetEffectImmunity(::IMMUNITY_TYPE_DISEASE, pCaster) != 0) {
            CSWCCMessageData* pData = new CSWCCMessageData();
            pData->SetObjectID(0, pObject->m_objectId);
            if (pCaster != 0) {
                CSWCCMessageData* pCasterData = new CSWCCMessageData();
                pData->CopyTo(pCasterData);
                pCaster->SendFeedbackMessage(FEEDBACK_IMMUNE_DISEASE, pCasterData);
            }
            pObject->AsSWSCreature()->SendFeedbackMessage(FEEDBACK_IMMUNE_DISEASE, pData);
            return 1;
        }
        for (int64_t i = 0; i < pObject->m_effects.m_count; i++) {
            if (pObject->m_effects.m_pArray[i]->m_subType == GAME_EFFECT_DISEASE)
                return 1;
        }
        int nFirstSave;
        pDisease->GetINTEntry(nRow, CExoString("First_Save"), &nFirstSave);
        if (pObject->AsSWSCreature()->SavingThrowRoll(SAVING_THROW_FORT, static_cast<unsigned short>(nFirstSave), SAVING_THROW_TYPE_DISEASE, pEffect->m_creatorId, 1, 0,
                                                      0) != 0)
            return 1;
        int nName;
        pDisease->GetINTEntry(nRow, CExoString("Name"), &nName);
        if (pObject->AsSWSCreature() != 0)
            pObject->AsSWSCreature()->DisplayFloatyDataToSelf(0);
        unsigned int nDay;
        unsigned int nTime;
        pTimer->GetWorldTime(&nDay, &nTime);
        unsigned int nDueDay;
        unsigned int nDueTime;
        int nIncubationHours;
        pDisease->GetINTEntry(nRow, CExoString("Incu_Hours"), &nIncubationHours);
        unsigned int nDelta = nIncubationHours *
                              g_pAppManager->m_pServerExoApp->GetModule()->m_nMinutesPerHour *
                              CWorldTimer::MILLISECONDS_IN_MINUTE;
        pTimer->AddWorldTimes(nDay, nTime, 0, nDelta, &nDueDay, &nDueTime);
        pEffect->SetInteger(3, nDueDay);
        pEffect->SetInteger(4, nDueTime);
    }
    CGameEffect* pVfx = new CGameEffect(pEffect);
    pVfx->m_subType = GAME_EFFECT_VISUALEFFECT;
    pVfx->SetInteger(0, VFX_IMP_DISEASE);
    pObject->ApplyEffect(pVfx, a, 0);
    return 0;
}

// Queues the spawn on the AI master; that event is what places the creature.
int CSWSEffectListHandler::OnApplySummonCreature(CSWSObject* pObject, CGameEffect* pEffect,
                                                 int a) {
    if (a == 1)
        return 0;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pEffect != 0 && pCreature != 0) {
        CSWSCreature* pSummon = new CSWSCreature(OBJECT_INVALID, 0);
        if (pEffect->GetObjectID(0) == OBJECT_INVALID) {
            pEffect->SetObjectID(0, pObject->m_cachedAreaId);
            pEffect->SetFloat(0, static_cast<CSWSObject *>(pObject)->m_position.x);
            pEffect->SetFloat(1, static_cast<CSWSObject *>(pObject)->m_position.y);
            pEffect->SetFloat(2, static_cast<CSWSObject *>(pObject)->m_position.z);
        }
        CSWSArea* pArea =
            g_pAppManager->m_pServerExoApp->GetAreaByGameObjectID(pEffect->GetObjectID(0));
        if (pArea == 0) {
            delete pSummon;
            return 0;
        }
        float fWantedX = pEffect->GetFloat(0);
        float fWantedY = pEffect->GetFloat(1);
        float fWantedZ = pEffect->GetFloat(2);
        unsigned int nDelay = 0;
        if (pEffect->GetFloat(3) > 0.0f)
            nDelay = static_cast<unsigned int>(pEffect->GetFloat(3)) * 1000;
        CResRef resRef(pEffect->GetString(0));
        pSummon->LoadFromTemplate(resRef, 0);
        Vector vWanted = { fWantedX, fWantedY, fWantedZ };
        Vector vSafe;
        float fX;
        float fY;
        float fZ;
        if (pArea->ComputeSafeLocation(vWanted, 20.0f, pSummon->m_pPathfindInfo, 0, &vSafe) == 1) {
            fX = vSafe.x;
            fY = vSafe.y;
            fZ = pArea->ComputeHeight(vSafe, 0, 0, 0);
        } else {
            fX = vWanted.x;
            fY = vWanted.y;
            fZ = vWanted.z;
        }
        int nVisualEffect = pEffect->GetInteger(0);
        if (nVisualEffect != -1) {
            CGameEffect* pVfx = new CGameEffect(1);
            pVfx->m_subType = GAME_EFFECT_VISUALEFFECT;
            pVfx->m_durationTypeBits =
                CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pVfx->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pVfx->SetInteger(0, nVisualEffect);
            Vector vAt = { fX, fY, fZ };
            pArea->ApplyEffect(pVfx, vAt, pCreature->m_orientation);
        }
        unsigned long long oidSummon = pSummon->m_objectId;
        CScriptEvent* pEvent = new CScriptEvent();
        pEvent->SetFloat(0, fX);
        pEvent->SetFloat(1, fY);
        pEvent->SetFloat(2, fZ);
        pEvent->SetObjectID(0, oidSummon);
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
            0, nDelay, pObject->m_objectId, pObject->m_objectId,
            CServerAIMaster::EVENT_SUMMON_CREATURE, pEvent);
        pEffect->SetObjectID(1, oidSummon);
    }
    return 0;
}

// Heals the target's hit points, or its force pool for one subtype.
int CSWSEffectListHandler::OnApplyHeal(CSWSObject* pObject, CGameEffect* pEffect, int a) {
#if !KOTOR1  // K1 has no silent-heal flag
    K2_UNUSED(a);
    int bSilent = pEffect->GetInteger(2);
#endif
    if (pObject->AsSWSCreature() == 0)
        return 1;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pEffect == 0)
        return 1;
    int nHeal = pEffect->GetInteger(0);
    if (pEffect->GetInteger(1) == ITEM_PROPERTY_REGENERATION_FORCE_POINTS) {
                CSWSCreatureStats* pStats = pCreature->m_pStats;
                if (pStats != 0) {
                    int nBoth = pStats->m_nCurrentForcePoints + pStats->m_nTemporaryForcePoints;
                    short nCur = static_cast<short>(nBoth);
                    short nMax = static_cast<short>(pCreature->GetMaxForcePoints());
                    int nGain = nHeal;
                    if (nHeal + nCur > nMax) {
                        nGain = nMax - nCur;
                    }
                    pCreature->m_pStats->m_nCurrentForcePoints = static_cast<unsigned short>(nBoth + nGain);

                    CSWCCMessageData* pData = new CSWCCMessageData();
                    pData->SetObjectID(0, pCreature->m_objectId);
                    pData->SetInteger(0, nGain);
                    pCreature->SendFeedbackMessage(FEEDBACK_FORCE_POINTS_REGAINED, pData);
                }
            } else {
                if (pCreature->GetDead() != 0)
                    return 1;
#if KOTOR1  // "down" is GetDeadTemp; a PC that stops dying is revived below
                if (pCreature->GetDeadTemp() != 0)
                    return 1;
                int bWasDying = pCreature->GetIsPCDying();
#else
                if (pCreature->m_bInParty != 0) {
                    if (pCreature->GetCurrentHitPoints(0) < 1)
                        return 1;
                }
#endif
                int nTotal = nHeal + static_cast<short>(pCreature->GetCurrentHitPoints(1));
                if (nTotal > static_cast<short>(pCreature->GetMaxHitPoints(1))) {
                    nHeal = static_cast<short>(pCreature->GetMaxHitPoints(1)) -
                            static_cast<short>(pCreature->GetCurrentHitPoints(1));
                }

                CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
                unsigned long long oidClient =
                    pClient->ServerToClientObjectId(pCreature->m_objectId);
                if (pClient->GetLastTarget() == oidClient ||
                    pClient->GetSWParty()->GetIndex(oidClient) != -1) {
                    pClient->AddFloatyText(oidClient, CClientExoApp::FLOATY_HEALED, nHeal);
                }
                pCreature->SetCurrentHitPoints(nHeal + static_cast<short>(pCreature->GetCurrentHitPoints(1)));

#if KOTOR1
                {
#else
                if (bSilent == 0) {
#endif
                    CSWCCMessageData* pData = new CSWCCMessageData();
                    CSWCCMessageData* pCasterData = new CSWCCMessageData();
                    pData->SetObjectID(0, pCreature->m_objectId);
                    pData->SetInteger(0, nHeal);
                    pData->CopyTo(pCasterData);
                    pCreature->SendFeedbackMessage(FEEDBACK_HEALED, pData);
                    CSWSCreature* pCaster =
                        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                            pEffect->m_creatorId);
                    if (pCaster != 0 && pCaster != pCreature) {
                        pCaster->SendFeedbackMessage(FEEDBACK_HEALED, pCasterData);
                    } else {
                        delete pCasterData;
                    }
                }

                int nCount = pCreature->m_effects.m_count;
                if (nCount > 0) {
                    int i = 0;
                    while (i < nCount) {
                        CGameEffect* pSub = pCreature->m_effects.m_pArray[i];
                        if (pSub->m_subType > GAME_EFFECT_WOUNDING)  // the wounding band ends here
                            break;
                        if (pSub->m_subType != GAME_EFFECT_WOUNDING ||
                            (pSub->m_durationTypeBits & 7) !=
                                CGameEffect::DURATION_TYPE_TEMPORARY) {
                            i++;
                            continue;
                        }
                        pCreature->RemoveEffectById(pSub->m_effectId);
                        nCount = pCreature->m_effects.m_count;
                        i = 0;
                    }
                }
#if KOTOR1
                if (bWasDying == 1 && pCreature->GetIsPCDying() == 0) {
                    CGameEffect* pRevive = new CGameEffect(1);
                    pRevive->m_subType = GAME_EFFECT_RESURRECTION;
                    pRevive->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                                  (pRevive->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
                    pCreature->ApplyEffect(pRevive, a, 0);
                }
#endif
    }
    return 1;
}

// The death bookend, split per concrete class.
int CSWSEffectListHandler::OnApplyDeath(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    if (pObject->m_bPlot != 0)
        return 1;
    if (pObject->m_bMin1HP != 0) {
        pObject->SetCurrentHitPoints(1);
        return 1;
    }
    unsigned long long oidCreator = pEffect->m_creatorId;
    CGameObject* pClientObject = pObject->GetClientObject();
    if (pClientObject != 0 && pClientObject->AsSWCCreature() != 0)
        pClientObject->AsSWCCreature()->SetLockOrientationToObject(OBJECT_INVALID, 1);
    if (pObject->AsSWSCreature() != 0)
        pObject->AsSWSCreature()->SetLockOrientationToObject(OBJECT_INVALID, 1);
    pObject->ClearAllActions(1);
    CSWSCreature* pKiller = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidCreator);
    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_CREATURE) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        if (pCreature != 0 && pCreature->m_nAIState != 0) {
            g_pAppManager->m_pClientExoApp->SetLastPartyMemberTempKilled(pCreature->m_objectId);
            if ((pEffect->m_durationTypeBits & CGameEffect::SUBTYPE_MASK) == SUBTYPE_MAGICAL &&
                    pCreature->m_pStats->GetEffectImmunity(::IMMUNITY_TYPE_DEATH, pKiller) != 0 &&
                    pEffect->m_spellId != -1) {
                CSWCCMessageData* pData = new CSWCCMessageData();
                pData->SetObjectID(0, pObject->m_objectId);
                if (pKiller != 0) {
                    CSWCCMessageData* pKillerData = new CSWCCMessageData();
                    pData->CopyTo(pKillerData);
                    pKiller->SendFeedbackMessage(FEEDBACK_IMMUNE_DEATH, pKillerData);
                }
                pCreature->SendFeedbackMessage(FEEDBACK_IMMUNE_DEATH, pData);
                return 1;
            }
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AdjustTargetAndWitnessReputations(
                pCreature->m_objectId, oidCreator, 2);
            int bSpectacularDeath = pEffect->GetInteger(0);
            pEffect->GetInteger(1);
#if KOTOR1  // no flag 2: death experience and the corpse destroy event always happen
            const bool bNoXpKeepCorpse = false;
            pCreature->m_killerId = oidCreator;
            pCreature->ApplyDeathExperience();
#else
            bool bNoXpKeepCorpse = pEffect->GetInteger(2) != 0;
            pCreature->m_killerId = oidCreator;
            if (!bNoXpKeepCorpse)
                pCreature->ApplyDeathExperience();
#endif
            if (pCreature->m_bInParty == 0)
                g_pAppManager->m_pServerExoApp->GetFactionManager()->m_pFactionNeutral->AddMember(
                    pCreature->m_objectId, 0);
            int bDestroyable = pCreature->m_bDestroyable;
            pCreature->m_pCombatRound->EndCombatRound(0);
            pCreature->m_pCombatRound->m_oidAttackTarget = OBJECT_INVALID;
            g_pVirtualMachine->RunScript(&pCreature->m_scripts[CSWSCreature::CREATURE_SCRIPT_DEATH],
                                         pCreature->m_objectId, 1);
            unsigned char nAmbient = pCreature->m_ambientAnimationState;
            if (nAmbient > 14 || ((1 << nAmbient) & AMBIENT_STATES_DEAD) == 0) {
                int nAmbientNew = 4;
                if ((rand_wincompatible() & 1) != 0)
                    nAmbientNew = 3;
                pCreature->SetAmbientAnimationState(static_cast<unsigned char>(nAmbientNew));
                pCreature->SetAnimation(SERVER_ANIMATION_PAUSE);
            }
            pCreature->m_bListening = 0;
            pCreature->m_nAIState = 0;
            if (static_cast<short>(pCreature->GetCurrentHitPoints(1)) >= -10)
                pCreature->SetCurrentHitPoints(-11);
            pCreature->AutoCloseGUIPanels(0);
            pCreature->UpdateActionQueue();
            for (int i = 0; i < pCreature->m_effects.m_count;) {
                CGameEffect* pScan = pCreature->m_effects.m_pArray[i];
                int nDuration = pScan->m_durationTypeBits & 7;
                if (nDuration == CGameEffect::DURATION_TYPE_INNATE ||
                    nDuration == CGameEffect::DURATION_TYPE_EQUIPPED ||
                    IsEffectToIgnoreOnDeath(pScan) != 0) {
                    i++;
                    continue;
                }
                pCreature->RemoveEffectById(pScan->m_effectId);
                i = 0;
            }
            if (static_cast<short>(pCreature->GetCurrentHitPoints(1)) >= -10)
                pCreature->SetCurrentHitPoints(-11);
            float fDelay;
            int bHave = g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
                pCreature->m_appearance.m_headId, CExoString("DestroyObjectDelay"), &fDelay);
            if (bHave == 0)
                fDelay = 3.0f;
            fDelay *= 1000.0f;
#if KOTOR1
            if (pCreature->GetDeadTemp() != 0) {
#else
            if (pCreature->m_bInParty != 0 && static_cast<short>(pCreature->GetCurrentHitPoints(0)) < 1) {
#endif
                CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
                CScriptEvent* pEvent = new CScriptEvent();
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_DEATH;
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, pCreature->m_objectId, pModule->m_objectId,
                    CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            } else if (bSpectacularDeath != 0 && bDestroyable != 0) {
                CGameEffect* pVfx = new CGameEffect(1);
                pVfx->m_subType = GAME_EFFECT_VISUALEFFECT;
                pVfx->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                    (pVfx->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
                pVfx->SetInteger(0, VFX_IMP_OVERKILL);
                pCreature->AsSWSCreature()->ApplyEffect(pVfx, a, 0);
                if (!bNoXpKeepCorpse)
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                        0, static_cast<unsigned int>(fDelay), pEffect->m_creatorId, pObject->m_objectId,
                        CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
                if (pCreature->m_oidEncounter != OBJECT_INVALID) {
                    CSWSEncounter* pEncounter =
                        g_pAppManager->m_pServerExoApp->GetEncounterByGameObjectID(
                            pCreature->m_oidEncounter);
                    if (pEncounter != 0) {
                        pEncounter->RemoveFromActiveCreatureCount(pCreature->m_pStats->m_fChallengeRating, 0);
                        pCreature->m_bEncounterSlotReleased = 1;
                    }
                }
            } else if (!bNoXpKeepCorpse) {
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, static_cast<unsigned int>(fDelay), pEffect->m_creatorId, pObject->m_objectId,
                    CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
            }
            pCreature->BroadcastVoiceChat(CSWSCreature::VOICE_CHAT_DEATH);
            CExoString sTimestamp;
            CExoString sName;
            g_pExoBase->m_pDebug->GetCurrentTimestamp(sTimestamp);
            sName = pCreature->m_pStats->GetFullName();
            if (pCreature->m_bPlayerCharacter != 0)
                sName = sName + "(PC)";
            if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bAutoPausePartyKilled != 0 &&
                pCreature->m_bInParty != 0) {
                int bLivingFound = 0;
                for (int nIdx = 0;
                     nIdx < g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount; nIdx++) {
                    CSWCCreature* pPartyMember =
                        g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(nIdx);
                    if (pPartyMember == 0)
                        continue;
                    if (pPartyMember->GetServerCreature() == 0)
                        continue;
                    CSWSCreature* pMemberServer = pPartyMember->GetServerCreature();
#if KOTOR1
                    if (pMemberServer->GetDeadTemp() == 0)
                        bLivingFound = 1;
#else
                    if (pMemberServer->m_bInParty == 0 ||
                        static_cast<short>(pMemberServer->GetCurrentHitPoints(0)) >= 1)
                        bLivingFound = 1;
#endif
                }
                if (bLivingFound != 0)
                    g_pAppManager->m_pClientExoApp->SetAutoPaused(1, 9);
            }
            if (g_pAppManager->m_pServerExoApp->GetPlayerCreatureId() == pCreature->m_objectId) {
                g_pAppManager->m_pClientExoApp->ChangeCharacterToNextLivingPartyMember(1, 1, 1);
                g_pAppManager->m_pClientExoApp->GetInGameGui()->RemoveGUIPopUps();
            }
            sTimestamp.CStr();
            sName.CStr();
        }
        rand_wincompatible();
        return 1;
    } else {
        if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
            if (pObject->AsSWSPlaceable() != 0)
                pObject->AsSWSPlaceable()->ClosePlaceableForAllPlayers();
            pObject->SetAnimation(PLACEABLE_ANIM_DEAD);
        } else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR) {
            pObject->AsSWSDoor()->SetOpenState(3, 1);
            CSWSObject* pLinked = pObject->AsSWSDoor()->GetLinkedObject();
            if (pLinked != 0 && pLinked->AsSWSDoor() != 0 && pLinked->m_nAnimation != PLACEABLE_ANIM_DEAD) {
                CGameEffect* pOpen = new CGameEffect(pEffect);
                pOpen->m_subType = GAME_EFFECT_DEATH;
                pOpen->SetInteger(0, 0);
                pOpen->SetInteger(1, 1);
                pLinked->ApplyEffect(pOpen, a, 0);
            }
        } else {
            return 1;
        }
        CScriptEvent* pEvent = new CScriptEvent();
        pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_DEATH;
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
            0, 0, pEffect->m_creatorId, pObject->m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT,
            pEvent);
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
            0, 2000, pEffect->m_creatorId, pObject->m_objectId,
            CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
    }
    return 1;
}

// Performs the throw itself; the saber flies from the thrower along the resolved target path.
int CSWSEffectListHandler::OnApplyLightsaberThrow(CSWSObject* pObject, CGameEffect* pEffect, int a) {
#if KOTOR1
    // K1: the throw is always flagged active, the three target ids are resolved, and the
    // flight returns to the thrower from the last valid target.
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature == 0)
        return 0;
    pCreature->m_bLightsaberThrowActive = 1;
    if (pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON) == 0)
        return 0;
    unsigned long long nId0 = pEffect->GetObjectID(0);
    unsigned long long nId1 = pEffect->GetObjectID(1);
    unsigned long long nId2 = pEffect->GetObjectID(2);
    int nSpellId =
            (pEffect->GetInteger(0) == 0) ? FORCE_POWER_LIGHT_SABER_THROW : FORCE_POWER_LIGHT_SABER_THROW_ADVANCED;
    CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(nSpellId);
    int nPauseMS = pSpell->m_nCastPauseMS;
    CGameObject* apTarget[3];
    apTarget[0] = 0;
    if (nId0 != OBJECT_INVALID)
        apTarget[0] = g_pAppManager->m_pServerExoApp->GetGameObject(nId0);
    apTarget[1] = 0;
    if (nId1 != OBJECT_INVALID)
        apTarget[1] = g_pAppManager->m_pServerExoApp->GetGameObject(nId1);
    apTarget[2] = 0;
    if (nId2 != OBJECT_INVALID)
        apTarget[2] = g_pAppManager->m_pServerExoApp->GetGameObject(nId2);
    CGameObject* pObj1 = apTarget[0];
    CGameObject* pObj2 = apTarget[1];
    CGameObject* pObj3 = apTarget[2];
    float fClose = 0.0f;
    float fLeg = 0.0f;
    if (pObj1 != 0) {
        fClose = magnitude(pCreature->m_position - pObj1->AsSWSObject()->m_position) + 0.0f;
        fLeg = fClose;
        if (pObj2 == 0)
            goto lPostDistance;
        fLeg = fClose + magnitude(pObj1->AsSWSObject()->m_position -
                                  pObj2->AsSWSObject()->m_position);
        fClose = magnitude(pCreature->m_position - pObj2->AsSWSObject()->m_position);
    } else if (pObj2 == 0) {
        goto lPostDistance;
    }
    if (pObj3 != 0) {
        fLeg += magnitude(pObj2->AsSWSObject()->m_position - pObj3->AsSWSObject()->m_position);
        fClose = magnitude(pCreature->m_position - pObj3->AsSWSObject()->m_position);
    }
lPostDistance:
    float fRate = (fClose + fLeg) / nPauseMS * 2000.0f;
    pCreature->m_lightSaberThrowRate = fRate;
    CServerAIMaster* pAI = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
    int nDelay = 0;
    int bLast;
    unsigned int nIdx = 0;
    do {
        CGameObject* pCur = pCreature;
        CGameObject* pNext = apTarget[0];
        bLast = 0;
        if (nIdx != 0) {
            pCur = apTarget[nIdx - 1];
            pNext = pCreature;
            if (nIdx != 3) {
                bLast = (apTarget[nIdx] == 0);
                if (!bLast)
                    pNext = apTarget[nIdx];
            }
        }
        if (pCur != 0 && pCur->AsSWSObject() != 0 && pNext != 0 && pNext->AsSWSObject() != 0) {
            CGameEffect* pFxMove = new CGameEffect(1);
            pFxMove->m_subType = GAME_EFFECT_VISUALEFFECT;
            pFxMove->m_durationTypeBits = CGameEffect::DURATION_TYPE_TEMPORARY |
                (pFxMove->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pFxMove->SetInteger(0, VFX_IMP_MIRV);
            pFxMove->SetInteger(2, 0);
            pFxMove->SetObjectID(0, pCur->AsSWSObject()->m_objectId);
            pFxMove->SetObjectID(1, pCreature->m_objectId);
            int nLegMS = static_cast<int>(magnitude(pCur->AsSWSObject()->m_position -
                                pNext->AsSWSObject()->m_position) / fRate * 2000.0f);
            pAI->AddEventDeltaTime(0, nDelay, pCur->AsSWSObject()->m_objectId,
                                   pNext->AsSWSObject()->m_objectId,
                                   CServerAIMaster::EVENT_APPLY_EFFECT, pFxMove);
            if (pNext != pCreature) {
                CGameEffect* pFxDmg = new CGameEffect(1);
                pFxDmg->m_subType = GAME_EFFECT_DAMAGE;
                pFxDmg->m_durationTypeBits = CGameEffect::DURATION_TYPE_TEMPORARY |
                    (pFxDmg->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
                pFxDmg->SetCreator(pCreature->m_objectId);
                pFxDmg->SetNumIntegers(21);
                for (int i = 0; i < 15; i++)
                    pFxDmg->SetInteger(i, -1);
                int nDmg = g_pRules->RollDice(static_cast<unsigned char>(static_cast<int>(pCreature->m_pStats->GetLevel(0) * 0.5f)), 6) & 0xffff;
                if (pNext->AsSWSCreature() != 0) {
                    CSWSCreature* pNextCreature = pNext->AsSWSCreature();
                    nDmg = pNextCreature->DoDamageResistance(pCreature, nDmg, DAMAGE_TYPE_BLASTER, 0, 0, 1);
                    nDmg = pNextCreature->DoDamageImmunity(pCreature, nDmg, DAMAGE_TYPE_BLASTER, 0, 0);
                }
                pFxDmg->SetInteger(12, nDmg);
                pFxDmg->SetInteger(14, nDmg);
                pFxDmg->SetInteger(16, 0);
                pFxDmg->SetInteger(19, 1);
                pFxDmg->SetInteger(20, 1);
                CGameEffect* pFxBack = new CGameEffect(1);
                pFxBack->m_subType = GAME_EFFECT_VISUALEFFECT;
                pFxBack->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                    (pFxBack->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
                pFxBack->SetInteger(0, VFX_IMP_MIRV_IMPACT);
                pFxBack->SetInteger(2, 0);
                pFxBack->SetObjectID(0, pCreature->m_objectId);
                int nHitDelay = nDelay + nLegMS;
                pAI->AddEventDeltaTime(0, nHitDelay, pCur->AsSWSObject()->m_objectId,
                                       pNext->AsSWSObject()->m_objectId,
                                       CServerAIMaster::EVENT_APPLY_EFFECT, pFxDmg);
                pAI->AddEventDeltaTime(0, nHitDelay, pCur->AsSWSObject()->m_objectId,
                                       pNext->AsSWSObject()->m_objectId,
                                       CServerAIMaster::EVENT_APPLY_EFFECT, pFxBack);
            }
            nDelay += nLegMS;
        }
        nIdx++;
    } while (nIdx < 4 && !bLast);
    K2_UNUSED(a);
    return 0;
#else
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if (pCreature == 0)
        return 0;
    if (pCreature->m_pStats->HasSpell(0, FORCE_POWER_FORCE_REPULSION, 0) == 0) {
        if (pCreature->m_pStats->HasSpell(0, FORCE_POWER_FORCE_REDIRECTION, 0) == 0)
            pCreature->m_bLightsaberThrowActive = 1;
    }
    if (pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON) == 0)
        return 0;
    unsigned long long anIds[4];
    CGameObject* apObj[4];
    anIds[0] = pObject->m_objectId;
    anIds[1] = pEffect->GetObjectID(0);
    anIds[2] = pEffect->GetObjectID(1);
    anIds[3] = pEffect->GetObjectID(2);
    int nSpellId =
            (pEffect->GetInteger(0) == 0) ? FORCE_POWER_LIGHT_SABER_THROW : FORCE_POWER_LIGHT_SABER_THROW_ADVANCED;
    CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(nSpellId);
    int nPauseMS = pSpell->m_nCastPauseMS;
    apObj[0] = 0;
    if (anIds[0] != OBJECT_INVALID) {
        apObj[0] = g_pAppManager->m_pServerExoApp->GetGameObject(anIds[0]);
        apObj[1] = g_pAppManager->m_pServerExoApp->GetGameObject(anIds[1]);
        apObj[2] = g_pAppManager->m_pServerExoApp->GetGameObject(anIds[2]);
        apObj[3] = g_pAppManager->m_pServerExoApp->GetGameObject(anIds[3]);
    } else {
        apObj[1] = 0;
        apObj[2] = 0;
        apObj[3] = 0;
    }
    float fClose;
    float fLeg;
    CGameObject* pObj0 = apObj[0];
    CGameObject* pObj1 = apObj[1];
    CGameObject* pObj2 = apObj[2];
    CGameObject* pObj3 = apObj[3];
    if (pObj1 != 0) {
        fClose = magnitude(pObj0->AsSWSObject()->m_position -
                           pObj1->AsSWSObject()->m_position) + 0.0f;
        if (pObj2 == 0) {
            fLeg = fClose;
            goto lPostDistance;
        } else {
            fLeg = fClose + magnitude(pObj1->AsSWSObject()->m_position -
                                      pObj2->AsSWSObject()->m_position);
            fClose = magnitude(pObj0->AsSWSObject()->m_position -
                               pObj2->AsSWSObject()->m_position);
            if (pObj3 == 0)
                goto lPostDistance;
        }
    } else {
        fClose = 0.0f;
        fLeg = 0.0f;
        if (pObj2 == 0)
            goto lPostDistance;
        if (pObj3 == 0)
            goto lPostDistance;
    }
    fLeg += magnitude(pObj2->AsSWSObject()->m_position -
                      pObj3->AsSWSObject()->m_position);
    fClose = magnitude(pObj0->AsSWSObject()->m_position -
                       pObj3->AsSWSObject()->m_position);
lPostDistance:
    float fRate = (fClose + fLeg) / nPauseMS * 2000.0f;
    pCreature->m_lightSaberThrowRate = fRate;
    CServerAIMaster* pAI = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
    if (apObj[0] == 0)
        return 0;
    int nDelay = 0;
    for (unsigned int nIdx = 0; nIdx < 4 && apObj[nIdx] != 0; nIdx++) {
        CGameObject* pCur = apObj[nIdx];
        int nNext = nIdx + 1 < 4 && anIds[nIdx + 1] != OBJECT_INVALID ? static_cast<int>(nIdx) + 1 : 0;
        CGameObject* pNext = apObj[nNext];
        if (pNext == 0 || pCur->AsSWSObject() == 0 || pNext->AsSWSObject() == 0)
            continue;
        CGameEffect* pFxMove = new CGameEffect(1);
        pFxMove->m_subType = GAME_EFFECT_VISUALEFFECT;
        pFxMove->m_durationTypeBits = CGameEffect::DURATION_TYPE_TEMPORARY |
            (pFxMove->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pFxMove->SetInteger(0, VFX_IMP_MIRV);
        pFxMove->SetInteger(2, 0);
        pFxMove->SetObjectID(0, anIds[nIdx]);
        pFxMove->SetObjectID(1, pCreature->m_objectId);
        int nLegMS = static_cast<int>(magnitude(pCur->AsSWSObject()->m_position -
                            pNext->AsSWSObject()->m_position) / fRate * 2000.0f);
        pAI->AddEventDeltaTime(0, nDelay, anIds[nIdx], anIds[nNext],
                               CServerAIMaster::EVENT_APPLY_EFFECT, pFxMove);
        if (nNext != 0) {
            CGameEffect* pFxDmg = new CGameEffect(1);
            pFxDmg->m_subType = GAME_EFFECT_DAMAGE;
            pFxDmg->m_durationTypeBits = CGameEffect::DURATION_TYPE_TEMPORARY |
                (pFxDmg->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pFxDmg->SetCreator(anIds[0]);
            pFxDmg->SetNumIntegers(21);
            for (int i = 0; i < 15; i++)
                pFxDmg->SetInteger(i, -1);
            int nDmg = g_pRules->RollDice(static_cast<unsigned char>(static_cast<int>(pCreature->m_pStats->GetLevel(0) * 0.5f)), 6) & 0xffff;
            if (pNext->AsSWSCreature() != 0) {
                CSWSCreature* pNextCreature = pNext->AsSWSCreature();
                nDmg = pNextCreature->DoDamageResistance(pCreature, nDmg, DAMAGE_TYPE_BLASTER, 0, 0, 1);
                nDmg = pNextCreature->DoDamageImmunity(pCreature, nDmg, DAMAGE_TYPE_BLASTER, 0, 0);
            }
            pFxDmg->SetInteger(12, nDmg);
            pFxDmg->SetInteger(14, nDmg);
            pFxDmg->SetInteger(16, 0);
            pFxDmg->SetInteger(19, 1);
            pFxDmg->SetInteger(20, 1);
            CGameEffect* pFxBack = new CGameEffect(1);
            pFxBack->m_subType = GAME_EFFECT_VISUALEFFECT;
            pFxBack->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                (pFxBack->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pFxBack->SetInteger(0, VFX_IMP_MIRV_IMPACT);
            pFxBack->SetInteger(2, 0);
            pFxBack->SetObjectID(0, pCreature->m_objectId);
            nDelay += nLegMS;
            pAI->AddEventDeltaTime(0, nDelay, anIds[nIdx], anIds[nNext],
                                   CServerAIMaster::EVENT_APPLY_EFFECT, pFxDmg);
            pAI->AddEventDeltaTime(0, nDelay, anIds[nIdx], anIds[nNext],
                                   CServerAIMaster::EVENT_APPLY_EFFECT, pFxBack);
        } else {
            nDelay += nLegMS;
        }
    }
    K2_UNUSED(a);
    return 0;
#endif
}

void CSWSEffectListHandler::InitializeEffects() {
#if KOTOR1  // K1 has 110 effect types
    m_paOnApply = new ApplyFn[110];
    m_paOnRemove = new RemoveFn[110];
    for (int i = 0; i < 110; i++) {
#else
    m_paOnApply = new ApplyFn[118];
    m_paOnRemove = new RemoveFn[118];
    for (int i = 0; i < 118; i++) {
#endif
        m_paOnApply[i] = 0;
        m_paOnRemove[i] = 0;
    }
    m_paOnApply[38] = &CSWSEffectListHandler::OnApplyDamage;
    m_paOnApply[39] = &CSWSEffectListHandler::OnApplyHeal;
    m_paOnApply[19] = &CSWSEffectListHandler::OnApplyDeath;
    m_paOnApply[4] = &CSWSEffectListHandler::OnApplyResurrection;
    m_paOnApply[36] = &CSWSEffectListHandler::OnApplyAbilityIncrease;
    m_paOnRemove[36] = &CSWSEffectListHandler::OnRemoveAbilityIncrease;
    m_paOnApply[37] = &CSWSEffectListHandler::OnApplyAbilityDecrease;
    m_paOnRemove[37] = &CSWSEffectListHandler::OnRemoveAbilityDecrease;
    m_paOnApply[6] = &CSWSEffectListHandler::OnApplySummonCreature;
    m_paOnRemove[6] = &CSWSEffectListHandler::OnRemoveSummonCreature;
    m_paOnApply[2] = &CSWSEffectListHandler::OnApplyDamageResistance;
    m_paOnApply[26] = &CSWSEffectListHandler::OnApplySavingThrowIncrease;
    m_paOnRemove[26] = &CSWSEffectListHandler::OnRemoveSavingThrowIncrease;
    m_paOnApply[27] = &CSWSEffectListHandler::OnApplySavingThrowDecrease;
    m_paOnRemove[27] = &CSWSEffectListHandler::OnRemoveSavingThrowDecrease;
    m_paOnApply[48] = &CSWSEffectListHandler::OnApplyACIncrease;
    m_paOnRemove[48] = &CSWSEffectListHandler::OnRemoveACIncrease;
    m_paOnApply[49] = &CSWSEffectListHandler::OnApplyACDecrease;
    m_paOnRemove[49] = &CSWSEffectListHandler::OnRemoveACDecrease;
    m_paOnApply[10] = &CSWSEffectListHandler::OnApplyAttackIncrease;
    m_paOnRemove[10] = &CSWSEffectListHandler::OnRemoveAttackIncrease;
    m_paOnApply[11] = &CSWSEffectListHandler::OnApplyAttackDecrease;
    m_paOnRemove[11] = &CSWSEffectListHandler::OnRemoveAttackDecrease;
    m_paOnApply[13] = &CSWSEffectListHandler::OnApplyDamageIncrease;
    m_paOnRemove[13] = &CSWSEffectListHandler::OnRemoveDamageIncrease;
    m_paOnApply[14] = &CSWSEffectListHandler::OnApplyDamageDecrease;
    m_paOnRemove[14] = &CSWSEffectListHandler::OnRemoveDamageDecrease;
    m_paOnApply[15] = &CSWSEffectListHandler::OnApplyTemporaryHitpoints;
    m_paOnRemove[15] = &CSWSEffectListHandler::OnRemoveTemporaryHitpoints;
    m_paOnApply[16] = &CSWSEffectListHandler::OnApplyDamageImmunityIncrease;
    m_paOnRemove[16] = &CSWSEffectListHandler::OnRemoveDamageImmunityIncrease;
    m_paOnApply[17] = &CSWSEffectListHandler::OnApplyDamageImmunityDecrease;
    m_paOnRemove[17] = &CSWSEffectListHandler::OnRemoveDamageImmunityDecrease;
    m_paOnApply[18] = &CSWSEffectListHandler::OnApplyEntangled;
    m_paOnRemove[18] = &CSWSEffectListHandler::OnRemoveEntangled;
    m_paOnApply[20] = &CSWSEffectListHandler::OnApplyKnockdown;
    m_paOnRemove[20] = &CSWSEffectListHandler::OnRemoveKnockdown;
    m_paOnApply[87] = &CSWSEffectListHandler::OnApplyDisarm;
    m_paOnRemove[87] = &CSWSEffectListHandler::OnRemoveDisarm;
    m_paOnApply[23] = &CSWSEffectListHandler::OnApplySetAIState;
    m_paOnRemove[23] = &CSWSEffectListHandler::OnRemoveSetAIState;
    m_paOnApply[21] = &CSWSEffectListHandler::OnApplyDeaf;
    m_paOnApply[24] = &CSWSEffectListHandler::OnApplyEnemyAttackBonus;
    m_paOnRemove[24] = &CSWSEffectListHandler::OnRemoveEnemyAttackBonus;
    m_paOnApply[8] = &CSWSEffectListHandler::OnApplySetState;
    m_paOnRemove[8] = &CSWSEffectListHandler::OnRemoveSetState;
    m_paOnApply[82] = &CSWSEffectListHandler::OnApplyNegativeLevel;
    m_paOnRemove[82] = &CSWSEffectListHandler::OnRemoveNegativeLevel;
    m_paOnApply[61] = &CSWSEffectListHandler::OnApplyDamageShield;
    m_paOnRemove[61] = &CSWSEffectListHandler::OnRemoveDamageShield;
    m_paOnApply[7] = &CSWSEffectListHandler::OnApplyRegenerate;
    m_paOnApply[5] = &CSWSEffectListHandler::OnApplyDisease;
    m_paOnRemove[5] = &CSWSEffectListHandler::OnRemoveDisease;
    m_paOnApply[35] = &CSWSEffectListHandler::OnApplyPoison;
    m_paOnRemove[35] = &CSWSEffectListHandler::OnRemovePoison;
    m_paOnApply[12] = &CSWSEffectListHandler::OnApplyDamageReduction;
    m_paOnApply[28] = &CSWSEffectListHandler::OnApplyMovementSpeedIncrease;
    m_paOnRemove[28] = &CSWSEffectListHandler::OnRemoveMovementSpeedIncrease;
    m_paOnApply[29] = &CSWSEffectListHandler::OnApplyMovementSpeedDecrease;
    m_paOnRemove[29] = &CSWSEffectListHandler::OnRemoveMovementSpeedDecrease;
    m_paOnApply[30] = &CSWSEffectListHandler::OnApplyVisualEffect;
    m_paOnRemove[30] = &CSWSEffectListHandler::OnRemoveVisualEffect;
    m_paOnApply[31] = &CSWSEffectListHandler::OnApplyAreaOfEffect;
    m_paOnRemove[31] = &CSWSEffectListHandler::OnRemoveAreaOfEffect;
    m_paOnApply[32] = &CSWSEffectListHandler::OnApplyBeam;
    m_paOnRemove[32] = &CSWSEffectListHandler::OnRemoveBeam;
    m_paOnApply[33] = &CSWSEffectListHandler::OnApplyForceResistanceIncrease;
    m_paOnRemove[33] = &CSWSEffectListHandler::OnRemoveForceResistanceIncrease;
    m_paOnApply[34] = &CSWSEffectListHandler::OnApplyForceResistanceDecrease;
    m_paOnRemove[34] = &CSWSEffectListHandler::OnRemoveForceResistanceDecrease;
    m_paOnApply[65] = &CSWSEffectListHandler::OnApplySpellLevelAbsorption;
    m_paOnRemove[65] = &CSWSEffectListHandler::OnRemoveSpellLevelAbsorption;
    m_paOnApply[50] = &CSWSEffectListHandler::OnApplySpellImmunity;
    m_paOnRemove[50] = &CSWSEffectListHandler::OnRemoveSpellImmunity;
    m_paOnApply[1] = &CSWSEffectListHandler::OnApplyHasteOrSlow;
    m_paOnRemove[1] = &CSWSEffectListHandler::OnRemoveHasteOrSlow;
    m_paOnApply[3] = &CSWSEffectListHandler::OnApplyHasteOrSlow;
    m_paOnRemove[3] = &CSWSEffectListHandler::OnRemoveHasteOrSlow;
    m_paOnApply[41] = &CSWSEffectListHandler::OnApplyHasteInternal;
    m_paOnRemove[41] = &CSWSEffectListHandler::OnRemoveHasteInternal;
    m_paOnApply[42] = &CSWSEffectListHandler::OnApplySlowInternal;
    m_paOnRemove[42] = &CSWSEffectListHandler::OnRemoveSlowInternal;
    m_paOnApply[40] = &CSWSEffectListHandler::OnApplyLink;
    m_paOnApply[44] = &CSWSEffectListHandler::OnApplyModifyNumAttacks;
    m_paOnRemove[44] = &CSWSEffectListHandler::OnRemoveModifyNumAttacks;
    m_paOnApply[45] = &CSWSEffectListHandler::OnApplyCurse;
    m_paOnRemove[45] = &CSWSEffectListHandler::OnRemoveCurse;
    m_paOnApply[46] = &CSWSEffectListHandler::OnApplySilence;
    m_paOnRemove[46] = &CSWSEffectListHandler::OnRemoveSilence;
    m_paOnApply[47] = &CSWSEffectListHandler::OnApplyInvisibility;
    m_paOnRemove[47] = &CSWSEffectListHandler::OnRemoveInvisibility;
    m_paOnApply[63] = &CSWSEffectListHandler::OnApplySanctuary;
    m_paOnRemove[63] = &CSWSEffectListHandler::OnRemoveSanctuary;
    m_paOnApply[70] = &CSWSEffectListHandler::OnApplySeeInvisible;
    m_paOnRemove[70] = &CSWSEffectListHandler::OnRemoveSeeInvisible;
    m_paOnApply[71] = &CSWSEffectListHandler::OnApplyUltraVision;
    m_paOnRemove[71] = &CSWSEffectListHandler::OnRemoveUltraVision;
    m_paOnApply[72] = &CSWSEffectListHandler::OnApplyTrueSeeing;
    m_paOnRemove[72] = &CSWSEffectListHandler::OnRemoveTrueSeeing;
    m_paOnApply[73] = &CSWSEffectListHandler::OnApplyBlindness;
    m_paOnRemove[73] = &CSWSEffectListHandler::OnRemoveBlindness;
    m_paOnApply[74] = &CSWSEffectListHandler::OnApplyDarkness;
    m_paOnRemove[74] = &CSWSEffectListHandler::OnRemoveDarkness;
    m_paOnApply[75] = &CSWSEffectListHandler::OnApplyMissChance;
    m_paOnRemove[75] = &CSWSEffectListHandler::OnRemoveMissChance;
    m_paOnApply[76] = &CSWSEffectListHandler::OnApplyConcealment;
    m_paOnRemove[76] = &CSWSEffectListHandler::OnRemoveConcealment;
    m_paOnApply[79] = &CSWSEffectListHandler::OnApplyDisappearAppear;
    m_paOnRemove[79] = &CSWSEffectListHandler::OnRemoveDisappearAppear;
    m_paOnApply[80] = &CSWSEffectListHandler::OnApplyDisappear;
    m_paOnApply[81] = &CSWSEffectListHandler::OnApplyAppear;
    m_paOnApply[78] = &CSWSEffectListHandler::OnApplyBonusSpellOfLevel;
    m_paOnRemove[78] = &CSWSEffectListHandler::OnRemoveBonusSpellOfLevel;
    m_paOnApply[22] = &CSWSEffectListHandler::OnApplyEffectImmunity;
    m_paOnRemove[22] = &CSWSEffectListHandler::OnRemoveEffectImmunity;
    m_paOnApply[9] = &CSWSEffectListHandler::OnApplySetStateInternal;
    m_paOnRemove[9] = &CSWSEffectListHandler::OnRemoveSetStateInternal;
    m_paOnApply[25] = &CSWSEffectListHandler::OnApplyArcaneSpellFailure;
    m_paOnRemove[25] = &CSWSEffectListHandler::OnRemoveArcaneSpellFailure;
    m_paOnApply[51] = &CSWSEffectListHandler::OnApplyDispelAllMagic;
    m_paOnApply[52] = &CSWSEffectListHandler::OnApplyDispelBestMagic;

    m_paOnApply[53] = &CSWSEffectListHandler::OnApplyTaunt;
    m_paOnRemove[53] = &CSWSEffectListHandler::OnRemoveTaunt;
    m_paOnApply[54] = &CSWSEffectListHandler::OnApplyLight;
    m_paOnRemove[54] = &CSWSEffectListHandler::OnRemoveLight;
    m_paOnApply[55] = &CSWSEffectListHandler::OnApplySkillIncrease;
    m_paOnRemove[55] = &CSWSEffectListHandler::OnRemoveSkillIncrease;
    m_paOnApply[56] = &CSWSEffectListHandler::OnApplySkillDecrease;
    m_paOnRemove[56] = &CSWSEffectListHandler::OnRemoveSkillDecrease;
    m_paOnApply[57] = &CSWSEffectListHandler::OnApplyHitPointChangeWhenDying;
    m_paOnRemove[57] = &CSWSEffectListHandler::OnRemoveHitPointChangeWhenDying;
    m_paOnApply[58] = &CSWSEffectListHandler::OnApplySpecialWalkAnimation;
    m_paOnRemove[58] = &CSWSEffectListHandler::OnRemoveSpecialWalkAnimation;
    m_paOnApply[59] = &CSWSEffectListHandler::OnApplyLimitMovementSpeed;
    m_paOnRemove[59] = &CSWSEffectListHandler::OnRemoveLimitMovementSpeed;
    m_paOnApply[67] = &CSWSEffectListHandler::OnApplyEffectIcon;
    m_paOnRemove[67] = &CSWSEffectListHandler::OnRemoveEffectIcon;
    m_paOnApply[62] = &CSWSEffectListHandler::OnApplyDisguise;
    m_paOnRemove[62] = &CSWSEffectListHandler::OnRemoveDisguise;
    m_paOnApply[64] = &CSWSEffectListHandler::OnApplyTimestop;
    m_paOnRemove[64] = &CSWSEffectListHandler::OnRemoveTimestop;
    m_paOnApply[68] = &CSWSEffectListHandler::OnApplyRacialType;
    m_paOnRemove[68] = &CSWSEffectListHandler::OnRemoveRacialType;
    m_paOnApply[69] = &CSWSEffectListHandler::OnApplyVision;
    m_paOnRemove[69] = &CSWSEffectListHandler::OnRemoveVision;
    m_paOnApply[77] = &CSWSEffectListHandler::OnApplyTurnResistance;
    m_paOnRemove[77] = &CSWSEffectListHandler::OnRemoveTurnResistance;
    m_paOnApply[88] = &CSWSEffectListHandler::OnApplyTurnResistance;
    m_paOnRemove[88] = &CSWSEffectListHandler::OnRemoveTurnResistance;
    m_paOnApply[83] = &CSWSEffectListHandler::OnApplyBonusFeat;
    m_paOnRemove[83] = &CSWSEffectListHandler::OnRemoveBonusFeat;
    m_paOnApply[84] = &CSWSEffectListHandler::OnApplyWounding;
    m_paOnRemove[84] = &CSWSEffectListHandler::OnRemoveWounding;
    m_paOnApply[86] = &CSWSEffectListHandler::OnApplyVampiricRegeneration;
    m_paOnApply[89] = &CSWSEffectListHandler::OnApplySummonParty;
    m_paOnRemove[89] = &CSWSEffectListHandler::OnRemoveSummonParty;
    m_paOnApply[90] = &CSWSEffectListHandler::OnApplyForceDrain;
    m_paOnApply[91] = &CSWSEffectListHandler::OnApplyTemporaryForce;
    m_paOnRemove[91] = &CSWSEffectListHandler::OnRemoveTemporaryForce;
    m_paOnApply[43] = &CSWSEffectListHandler::OnApplyDroidStun;
    m_paOnRemove[43] = &CSWSEffectListHandler::OnRemoveDroidStun;
    m_paOnApply[60] = &CSWSEffectListHandler::OnApplyForcePush;
    m_paOnRemove[60] = &CSWSEffectListHandler::OnRemoveForcePush;
    m_paOnApply[66] = &CSWSEffectListHandler::OnApplyWhirlWind;
    m_paOnRemove[66] = &CSWSEffectListHandler::OnRemoveWhirlWind;
    m_paOnApply[92] = &CSWSEffectListHandler::OnApplyBlasterDeflectionIncrease;
    m_paOnRemove[92] = &CSWSEffectListHandler::OnRemoveBlasterDeflectionIncrease;
    m_paOnApply[93] = &CSWSEffectListHandler::OnApplyBlasterDeflectionDecrease;
    m_paOnRemove[93] = &CSWSEffectListHandler::OnRemoveBlasterDeflectionDecrease;
    m_paOnApply[94] = &CSWSEffectListHandler::OnApplyHorrified;
    m_paOnRemove[94] = &CSWSEffectListHandler::OnRemoveHorrified;
    m_paOnApply[95] = &CSWSEffectListHandler::OnApplyDamageForcePoints;
    m_paOnApply[96] = &CSWSEffectListHandler::OnApplyHealForcePoints;
    m_paOnApply[97] = &CSWSEffectListHandler::OnApplyChoke;
    m_paOnRemove[97] = &CSWSEffectListHandler::OnRemoveChoke;
    m_paOnApply[98] = &CSWSEffectListHandler::OnApplyBodyFuel;
    m_paOnRemove[98] = &CSWSEffectListHandler::OnRemoveBodyFuel;
    m_paOnApply[99] = &CSWSEffectListHandler::OnApplyPsychicStatic;
    m_paOnRemove[99] = &CSWSEffectListHandler::OnRemovePsychicStatic;
    m_paOnApply[100] = &CSWSEffectListHandler::OnApplyLightsaberThrow;
    m_paOnRemove[100] = &CSWSEffectListHandler::OnRemoveLightsaberThrow;
    m_paOnApply[101] = &CSWSEffectListHandler::OnApplyAssuredHit;
    m_paOnRemove[101] = &CSWSEffectListHandler::OnRemoveAssuredHit;
    m_paOnApply[102] = &CSWSEffectListHandler::OnApplyForceJump;
    m_paOnRemove[102] = &CSWSEffectListHandler::OnRemoveForceJump;
    m_paOnApply[103] = &CSWSEffectListHandler::OnApplyForceJumpDelayed;
    m_paOnRemove[103] = &CSWSEffectListHandler::OnRemoveForceJumpDelayed;
    m_paOnApply[104] = &CSWSEffectListHandler::OnApplyAssuredDeflection;
    m_paOnRemove[104] = &CSWSEffectListHandler::OnRemoveAssuredDeflection;
    m_paOnApply[105] = &CSWSEffectListHandler::OnApplyForceResisted;
    m_paOnApply[106] = &CSWSEffectListHandler::OnApplyForceFizzle;
    m_paOnApply[107] = &CSWSEffectListHandler::OnApplyForceShield;
    m_paOnRemove[107] = &CSWSEffectListHandler::OnRemoveForceShield;
    m_paOnApply[108] = &CSWSEffectListHandler::OnApplyPureGoodPowers;
    m_paOnRemove[108] = &CSWSEffectListHandler::OnRemovePureGoodPowers;
    m_paOnApply[109] = &CSWSEffectListHandler::OnApplyPureEvilPowers;
    m_paOnRemove[109] = &CSWSEffectListHandler::OnRemovePureEvilPowers;
#if !KOTOR1
    m_paOnApply[110] = &CSWSEffectListHandler::OnApplyForceBody;
    m_paOnRemove[110] = &CSWSEffectListHandler::OnRemoveForceBody;
    m_paOnApply[111] = &CSWSEffectListHandler::OnApplyFury;
    m_paOnRemove[111] = &CSWSEffectListHandler::OnRemoveFury;
    m_paOnApply[112] = &CSWSEffectListHandler::OnApplyFPRegenModifier;
    m_paOnRemove[112] = &CSWSEffectListHandler::OnRemoveFPRegenModifier;
    m_paOnApply[113] = &CSWSEffectListHandler::OnApplyVPRegenModifier;
    m_paOnRemove[113] = &CSWSEffectListHandler::OnRemoveVPRegenModifier;
    m_paOnApply[114] = &CSWSEffectListHandler::OnApplyForceSight;
    m_paOnRemove[114] = &CSWSEffectListHandler::OnRemoveForceSight;
    m_paOnApply[115] = &CSWSEffectListHandler::OnApplyFactionModifier;
    m_paOnRemove[115] = &CSWSEffectListHandler::OnRemoveFactionModifier;
    m_paOnApply[116] = &CSWSEffectListHandler::OnApplyDestroyShields;
    m_paOnApply[117] = &CSWSEffectListHandler::OnApplyAssassinate;
#endif
}

// Like DispelAllMagic, but removes at most one effect.
int CSWSEffectListHandler::OnApplyDispelBestMagic(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CGameObject* pCasterObject;
    if (pObjectArray->GetGameObject(pEffect->m_creatorId, &pCasterObject) !=
            CGameObjectArray::SUCCESS ||
        pCasterObject == 0)
        return 1;
    if (pCasterObject->AsSWSCreature() == 0)
        return 1;
    int nCasterLevel = pCasterObject->AsSWSCreature()->m_pStats->GetLevel(0);
    int nBest = -1;
    CExoArrayList<unsigned long long> l_removed;
    if (pObject->m_effects.m_count >= 1) {
        do {
            int bFound = 0;
            for (int64_t i = 0; i < pObject->m_effects.m_count; i++) {
                CGameEffect* pEf = pObject->m_effects.m_pArray[i];
                if ((pEf->m_durationTypeBits & CGameEffect::SUBTYPE_MASK) != SUBTYPE_MAGICAL ||
                    ((pEf->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_TEMPORARY &&
                     (pEf->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_PERMANENT) ||
                    l_removed.Contains(pEf->m_effectId) != 0)
                    continue;
                CSWSCreature* pEfCaster =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pEf->m_creatorId);
                if (pEfCaster == 0 || (pEfCaster->m_pStats->GetLevel(0) & 0xff) != 0) {
                    nBest = static_cast<int>(i);
                    bFound = 1;
                    break;
                }
            }
            if (!bFound)
                break;
            CGameEffect* pBest = pObject->m_effects.m_pArray[nBest];
            unsigned short nRoll = g_pRules->RollDice(1, 20);
            if ((nCasterLevel & 0xff) + nRoll > 11) {
                if (pBest->m_spellId != -1) {
                    CSWCCMessageData* pData = new CSWCCMessageData();
                    pData->SetObjectID(0, pObject->m_objectId);
                    pData->SetInteger(0, 1);
                    pData->SetInteger(1, pBest->m_spellId);
                    CSWSPlayer* pTargetPlayer =
                        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(
                            pObject->m_objectId);
                    if (pTargetPlayer != 0)
                        g_pAppManager->m_pServerExoApp->GetSWSMessage()
                            ->SendServerToPlayerCCMessage(
                                pTargetPlayer->m_nPlayerId, CC_MESSAGE_DISPEL, pData, 0);
                    delete pData;
                }
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, pEffect->m_creatorId, pObject->m_objectId,
                    CServerAIMaster::EVENT_REMOVE_EFFECT, pBest);
                // A successful roll ends the dispel, so at most one effect is removed.
                break;
            }
            l_removed.Add(pBest->m_effectId);
        } while (pObject->m_effects.m_count > 0);
    }
    return 1;
}

// Adds one row to the creature's live status-icon strip, kept sorted by priority.
int CSWSEffectListHandler::OnApplyEffectIcon(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject != 0 && pObject->AsSWSCreature() != 0) {
        short nIcon = static_cast<short>(pEffect->GetInteger(0));
        if ((pEffect->m_durationTypeBits & 7) != CGameEffect::DURATION_TYPE_INSTANTANEOUS) {
            CSWSCreature* pCreature = pObject->AsSWSCreature();
            for (int64_t i = 0; i < pCreature->m_effectIcons.m_count; i++)
                if (pCreature->m_effectIcons.m_pArray[i]->m_nIconId == nIcon)
                    return 0;
            CEffectIconObject* pIcon = new CEffectIconObject;
            CExoString sResRef;
            int nRet = 1;
            int nGood;
            int nPriority K2_UNSET;
#if KOTOR1  // icon and priority are required; no name strref
            if (g_pRules->m_p2DArrays->m_pEffectIcon->GetCExoStringEntry(
                    nIcon, CExoString("IconResRef"), &sResRef) != 0 &&
                g_pRules->m_p2DArrays->m_pEffectIcon->GetINTEntry(
                    nIcon, CExoString("Priority"), &nPriority) != 0) {
                pIcon->m_nGood = g_pRules->m_p2DArrays->m_pEffectIcon->GetINTEntry(
                    nIcon, CExoString("Good"), &nGood);
                pIcon->m_nIconId = nIcon;
                pIcon->m_resIcon = sResRef;
                pIcon->m_bGood = nGood != 0;
                pIcon->m_nPriority = static_cast<unsigned short>(nPriority);
                int64_t i = 0;
                for (; i < pCreature->m_effectIcons.m_count; i++)
                    if (nPriority < pCreature->m_effectIcons.m_pArray[i]->m_nPriority)
                        break;
                pCreature->m_effectIcons.Insert(pIcon, static_cast<int>(i));
                nRet = 0;
            }
#else
            int nStrRef;
            if (g_pRules->m_p2DArrays->m_pEffectIcon->GetCExoStringEntry(
                    nIcon, CExoString("IconResRef"), &sResRef)
                != 0) {
                if (g_pRules->m_p2DArrays->m_pEffectIcon->GetINTEntry(
                        nIcon, CExoString("Good"), &nGood)
                    != 0) {
                    if (g_pRules->m_p2DArrays->m_pEffectIcon->GetINTEntry(
                            nIcon, CExoString("Priority"), &nPriority)
                        != 0) {
                        if (g_pRules->m_p2DArrays->m_pEffectIcon->GetINTEntry(
                                nIcon, CExoString("NameStrRef"), &nStrRef)
                            != 0) {
                            pIcon->m_nGood = g_pRules->m_p2DArrays->m_pEffectIcon->GetINTEntry(
                                nIcon, CExoString("Good"), &nGood);
                            pIcon->m_nIconId = nIcon;
                            pIcon->m_resIcon = sResRef;
                            pIcon->m_nPriority = static_cast<unsigned short>(nPriority);
                            pIcon->m_bGood = nGood != 0;
                            pIcon->m_nNameStrRef = nStrRef;
                            int64_t i = 0;
                            for (; i < pCreature->m_effectIcons.m_count; i++)
                                if (nPriority
                                    < pCreature->m_effectIcons.m_pArray[i]->m_nPriority)
                                    break;
                            pCreature->m_effectIcons.Insert(pIcon, static_cast<int>(i));
                            nRet = 0;
                        }
                    }
                }
            }
#endif
            if (nRet != 0)
                delete pIcon;
            return nRet;
        }
    }
    return 1;
}

// Removes the target's hand weapon and either hands it to the disarmer's item repository or
// drops it on the ground; a non-droppable weapon may be scheduled for destruction instead.
int CSWSEffectListHandler::OnApplyDisarm(CSWSObject* pObject, CGameEffect* pEffect, int a) {
    K2_UNUSED(a);
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pCreature = pObject->AsSWSCreature();
        CSWSItem* pItem;
        Vector vPos;
        Vector vAt;
        if (pCreature->m_bDisarmable != 0) {
            CSWSCreature* pDisarmer = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                pEffect->m_creatorId);
            int nX = pEffect->GetInteger(0);
            if (pDisarmer != 0 && nX == 1
                && pDisarmer->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON) == 0
                && (pItem = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON))) != 0) {
                pCreature->RemoveItem(pItem, 0, 1, 0);
                if (pItem->m_bits.m_bDropable) {
                    CItemRepository* pRepo = pDisarmer->GetItemRepository(1);
                    if (pRepo->AddItem(&pItem, 1, 0, 1) == 0 && pItem != 0) {
                        vAt = pDisarmer->m_position;
                        vAt.z += 0.1f;
                        pItem->AddToArea(static_cast<CSWSArea*>(pCreature->GetArea()), vAt.x, vAt.y, vAt.z, 1);
                    }
                    if (pItem != 0)
                        pItem->SetPossessor(pDisarmer->m_objectId, 1, 1, OBJECT_INVALID);
                } else if (pItem->m_bDestroyable == 1) {
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                        0, 0, pDisarmer->m_objectId, pItem->m_objectId,
                        CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
                }
            } else {
                pItem =
                        static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
                if (pItem != 0) {
                    pCreature->RemoveItem(pItem, 0, 1, 0);
                    if (pItem->m_bits.m_bDropable) {
                        vPos = pCreature->m_position;
                        int nA = rand_wincompatible();
                        int nB = rand_wincompatible();
                        float fJ = static_cast<float>(nB % 25) / 10.0f;
                        float fN = -fJ;
                        vPos.x += (nA & 1) ? fJ : fN;
                        nA = rand_wincompatible();
                        nB = rand_wincompatible();
                        fJ = static_cast<float>(nB % 25) / 10.0f;
                        fN = -fJ;
                        vPos.y += (nA & 1) ? fJ : fN;
                        pItem->SetPossessor(OBJECT_INVALID, 1, 1, OBJECT_INVALID);
                        static_cast<CSWSArea*>(pCreature->GetArea())->ComputeSafeLocation(
                            vPos, 20.0f, pCreature->m_pPathfindInfo, 1, &vAt);
                        vAt.z = static_cast<CSWSArea*>(pCreature->GetArea())->ComputeHeight(vAt, 0, 0, 0) + 0.2f;
                        pItem->AddToArea(static_cast<CSWSArea*>(pCreature->GetArea()), vAt.x, vAt.y, vAt.z, 1);
                    } else if (pItem->m_bDestroyable == 1) {
                        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
#ifdef K2_FIX_ENGINE_BUGS
                            0, 0, pEffect->m_creatorId, pItem->m_objectId,
#else
                            0, 0, pDisarmer->m_objectId, pItem->m_objectId,
#endif
                            CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
                    }
                }
            }
        }
    }
    return 1;
}

