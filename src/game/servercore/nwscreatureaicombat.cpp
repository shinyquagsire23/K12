#include "nwscreature.h"
#include "nwscriptconst.h"
#include "Log_Base2.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "serverexoapp.h"
#include "partytables.h"
#include "nwsaiaction.h"
#include "enginevfx.h"
#include "strref.h"

// effecticon.2da rows of the on-hit effects' icons, named after the power whose icon they borrow.
enum {
    EFFECT_ICON_FORCE_POWER_STASIS      = 15,
    EFFECT_ICON_FORCE_POWER_INSANITY    = 17,
    EFFECT_ICON_FORCE_POWER_PLAGUE      = 23,
    EFFECT_ICON_FORCE_POWER_MASS_STASIS = 27,
    EFFECT_ICON_FORCE_POWER_SLOW        = 28,
    EFFECT_ICON_FORCE_POWER_STUN        = 29,
};

// visualeffects.2da ids the on-hit item-property effects play, named after the effect that plays
// them. Neither game's visualeffects.2da has these rows (both start at 1001), so the visuals find
// no row.
enum {
    VFX_ONHIT_SLAY_RACIAL_GROUP = 50,
    VFX_ONHIT_HOLD_LINK         = 82,
    VFX_ONHIT_ABILITY_DAMAGE    = 91,
    VFX_ONHIT_SLEEP             = 94,
    VFX_ONHIT_SLOW              = 95,
    VFX_ONHIT_DISABLED_LINK     = 207,   // linked into every disabling on-hit effect
    VFX_ONHIT_STUN_CONFUSION    = 208,
    VFX_ONHIT_FEAR              = 218,
    VFX_ONHIT_HOLD              = 232,
};

// g_anFlatFootedAIState's length: one entry per m_nAIStateAction value.
enum {
    NUM_FLAT_FOOTED_AI_STATES = 32,
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

extern int g_bEnableCombatDebugging;
extern int RenderHP;
extern int RenderQA;
extern int RenderDEV;
int ShowPath;
extern int HurtPlayers;
int RenderCombat;
int ZeroForce;

extern int g_nJournalSortType;
#include "nwsitem.h"
#include "nwsplaceable.h"
#include "nwsitemproperty.h"
#include "nwcobject.h"
#include "CExoArrayListDtors.h"
#include "CExoArrayListImpl.h"
#include "exostring.h"
#include "nwcreaturelevelstats.h"
#include "nwclass.h"
#include "nwrace.h"
#include "nwseffectlist.h"
#include "nwseffect.h"
#include "nwitem.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "SWCollisionMesh.h"
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
#include "nwcanimbase.h"
#include "nwccreatureappearance.h"
#include "nwmessage.h"
#include "resgff.h"
#include "nwspell.h"
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

int g_bDisableDamage;

#include "port/crtapi.h"

extern float g_fInjuredLevel;


#include "port/libcmath.h"
#include "exostringinline.h"


int CSWSCreature::IsAttackSuccessful(int a) {
    if (a == -1)
        return m_attackResult == 0;
    return static_cast<unsigned int>(a - 1) < 3;
}

int CSWSCreature::IsAttackParried(int a) {
    if (a == -1)
        return m_attackResult == 2;
    return static_cast<unsigned int>(a - 8) < 3;
}

int CSWSCreature::IsAttackMiss(int a) {
    if (a == -1)
        return m_attackResult == 1;
    return static_cast<unsigned int>(a - 4) < 3;
}

void CSWSCreature::ApplyOnHitSlayAlignmentGroup(CSWSObject* pObject, CSWItemProperty* pProperty)
{
    K2_UNUSED(pObject); K2_UNUSED(pProperty);
}

// Wraps the poison subtype and an effect-icon marker into a linked effect, marks it permanent,
// and hangs it on the current attack.
void CSWSCreature::ApplyOnHitPoison(CSWSObject* pObject, CSWItemProperty* pProperty)
{
    K2_UNUSED(pObject);
    CGameEffect* pEffect = CreateLinkedEffect(CSWSEffectListHandler::EFFECT_POISON,
                                              pProperty->m_nParam1Value,
                                              CSWSEffectListHandler::EFFECT_EFFECT_ICON, EFFECT_ICON_FORCE_POWER_PLAGUE);
    if (pEffect != 0) {
        pEffect->m_durationTypeBits = (CGameEffect::DURATION_TYPE_PERMANENT & ~SUBTYPE_SUPERNATURAL) |
                                      (pEffect->m_durationTypeBits & CGameEffect::DURATION_SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
        m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_effects.Add(pEffect);
    }
}

// Applies the on-hit ability drain, a bare visual effect plus an ability-decrease effect wrapped in
// an effect icon, and a client message, all on the current attack.
void CSWSCreature::ApplyOnHitAbilityDamage(CSWSObject* pObject, CSWItemProperty* pProperty)
{
    CGameEffect* pVisual = new CGameEffect(1);
    pVisual->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
    pVisual->SetInteger(0, VFX_ONHIT_ABILITY_DAMAGE);
    pVisual->SetCreator(m_objectId);
    pVisual->m_durationTypeBits =
        CGameEffect::DURATION_TYPE_TEMPORARY | (pVisual->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pVisual->m_fDuration = 30.0f;
    m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_effects.Add(pVisual);

    CGameEffect* pDrain = new CGameEffect(1);
    pDrain->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ABILITY_DECREASE);
    pDrain->SetInteger(0, pProperty->m_nParam1Value);
    pDrain->SetInteger(1, 1);
    CGameEffect* pLinked = CreateLinkedEffect(pDrain,
                                              CSWSEffectListHandler::EFFECT_EFFECT_ICON, EFFECT_ICON_FORCE_POWER_SLOW);
    int nBits =
            (pLinked->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) | CGameEffect::DURATION_TYPE_TEMPORARY;
    pLinked->m_fDuration = 30.0f;
    if (pProperty->m_type == ITEM_PROPERTY_ON_MONSTER_HIT)
        pLinked->m_durationTypeBits = (nBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_SUPERNATURAL;
    else
        pLinked->m_durationTypeBits = static_cast<unsigned short>(nBits | SUBTYPE_EXTRAORDINARY);
    pLinked->UpdateLinked();
    m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_effects.Add(pLinked);

    CSWCCMessageData* pMessage = new CSWCCMessageData;
    pMessage->SetObjectID(0, pObject->m_objectId);
    pMessage->SetInteger(0, FEEDBACK_ATTRIBUTE_REDUCED);
    pMessage->SetInteger(1, pProperty->m_nParam1Value);
    pMessage->SetInteger(2, 1);
    pMessage->SetFloat(0, 30.0f);
    m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_messages.Add(pMessage);
}

// Maps a weapon type to its row label in weapondischarge.2da. A type flagged "Droid" gets a
// "<race>_<type>" row read from droiddischarge.2da; appearance row 3 gets "3_<type>".
CExoString CSWSCreature::GetWeaponDischargeRow(int nWeaponType)
{
    CExoString sRow(nWeaponType);
    int bDroid = 0;
    g_pRules->m_p2DArrays->m_pWeaponDischarge->GetINTEntry(sRow, CExoString("Droid"), &bDroid);
    if (bDroid != 0) {
        CExoString sType(nWeaponType);
        CExoString sRace;
        CExoString sPrefix;
        g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
            m_appearance.m_headId, CExoString("RACE"), &sRace);
        g_pRules->m_p2DArrays->m_pDroidDischarge->GetCExoStringEntry(sRace, sType, &sPrefix);
        sRow = sPrefix + "_" + sRow;
    } else if (m_appearance.m_headId == 3) {
        sRow = CExoString("3_") + CExoString(nWeaponType);
    }
    return sRow;
}

void CSWSCreature::ResolveSituationalModifiers(CSWSCreature* pCreature)
{
#if !KOTOR1
    K2_UNUSED(pCreature);
#endif
#if KOTOR1  // K1: coup de grace against a helpless or dying low-level non-party target.
    if (pCreature == NULL) return;
    if (pCreature->m_nState == EFFECT_STATE_NONE && pCreature->GetDeadTemp() == 0) return;
    if (pCreature->m_pStats->GetLevel(0) > 4) return;
    if (pCreature->m_bInParty != 0) return;
    m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_bCoupDeGrace = 1;
#endif
}

#if KOTOR1
// K1: no unarmed Force jab, no power attack, no shield breaker and no result out-param; the
// flurry dodge bonus is 4/1/2 for the three ranked feats.
void CSWSCreature::ResolveMeleeSpecialAttack(int nAttackIndex, int b, CSWSObject* pTarget, int d)
{
    K2_UNUSED(b); K2_UNUSED(d);
    if (nAttackIndex != 0 || pTarget == 0)
        return;
    CSWSCombatAttackData* pAttack = m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
    g_pAppManager->m_pServerExoApp->GetServerAIMaster();

    if (m_pStats->GetFeatRemainingUses(pAttack->m_nFeatId) == 0) {
        pAttack->m_nFeatId = 0;
        return;
    }
    unsigned short nFeat = pAttack->m_nFeatId;
    int nDodgeBonus;
    switch (nFeat) {
    case FEAT_CRITICAL_STRIKE:
    case FEAT_IMPROVED_CRITICAL_STRIKE:
    case FEAT_MASTER_CRITICAL_STRIKE: {
        CGameEffect* pDodge = new CGameEffect(1);
        pDodge->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_AC_DECREASE);
        pDodge->m_durationTypeBits =
            CGameEffect::DURATION_TYPE_TEMPORARY | (pDodge->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pDodge->m_fDuration = 3.0f;
        pDodge->SetCreator(m_objectId);
        pDodge->SetInteger(0, CSWSEffectListHandler::AC_DODGE);
        pDodge->SetInteger(1, 5);
        pDodge->SetInteger(2, g_pRules->m_nNumRaces);
        pDodge->SetInteger(5, CSWSEffectListHandler::DAMAGE_FLAG_PHYSICAL);
        ApplyEffect(pDodge, 0, 0);

        if (static_cast<unsigned char>(pAttack->m_nAttackResult - 1) < 3 && pTarget->AsSWSCreature() != 0) {
            CGameEffect* pStun = new CGameEffect(1);
            pStun->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETSTATE);
            pStun->SetInteger(0, 4);
            pStun->m_durationTypeBits =
                (pStun->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) | CGameEffect::DURATION_TYPE_TEMPORARY;
            pStun->m_fDuration = 6.0f;
            pStun->SetCreator(m_objectId);
            CSWSCreature* pStunned = pTarget->AsSWSCreature();
            unsigned char nLevel = m_pStats->GetLevel(0);
            int nStrMod = static_cast<signed char>(m_pStats->m_nSTRMod);
            if (pStunned->m_pStats->GetEffectLinkImmunity(pStun) == 0
                && pStunned->SavingThrowRoll(SAVING_THROW_FORT, static_cast<unsigned short>(nStrMod + nLevel), SAVING_THROW_TYPE_NONE, m_objectId, 0, 0, 1) == 0) {
                pAttack->m_effects.Add(pStun);
                pAttack->m_effect.m_nFeedbackEffectType = EFFECT_STATE_STUNNED;
            } else {
                delete pStun;
            }
            pAttack->m_effect.m_bHasApplicationData = 1;
        }
        break;
    }
    case FEAT_FLURRY:
        nDodgeBonus = 4;
        goto flurry;
    case FEAT_WHIRLWIND_ATTACK:
        nDodgeBonus = 1;
        goto flurry;
    case FEAT_IMPROVED_FLURRY:
        nDodgeBonus = 2;
    flurry: {
        CGameEffect* pDodge = new CGameEffect(1);
        pDodge->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_AC_DECREASE);
        pDodge->m_durationTypeBits =
            CGameEffect::DURATION_TYPE_TEMPORARY | (pDodge->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pDodge->m_fDuration = 3.0f;
        pDodge->SetCreator(m_objectId);
        pDodge->SetInteger(0, CSWSEffectListHandler::AC_DODGE);
        pDodge->SetInteger(1, nDodgeBonus);
        pDodge->SetInteger(2, g_pRules->m_nNumRaces);
        pDodge->SetInteger(5, CSWSEffectListHandler::DAMAGE_FLAG_PHYSICAL);
        ApplyEffect(pDodge, 0, 0);
        break;
    }
    }
}
#else
// Runs the two extra passes the round's first melee attack gets: the unarmed "Force jab" (a ranked
// feat, d6 per rank, a percentage roll and a failed save for a movement-speed debuff), then the
// payload named by the attack's own feat id, which is spent here - a dodge bonus, a stun, a Force
// push or a shield strip. nResult reports that a special attack landed on the target.
void CSWSCreature::ResolveMeleeSpecialAttack(int nAttackIndex, int b, CSWSObject* pTarget, int d,
                                             int& nResult)
{
    K2_UNUSED(b); K2_UNUSED(d);
    nResult = 0;
    if (nAttackIndex != 0 || pTarget == 0)
        return;
    CSWSCombatAttackData* pAttack = m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
    g_pAppManager->m_pServerExoApp->GetServerAIMaster();

    // Unsigned range check.
    if (static_cast<unsigned char>(pAttack->m_nWeaponAttackType - CSWSCombatRound::WEAPON_ATTACK_TYPE_UNARMED) < 2) {
        // The three ranks of the unarmed-jab feat, best first; the rank doubles as the d6 count.
        int nRank;
        if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_GRAVITONIC_STRIKE) != 0) {
            nRank = 3;
        } else if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_ELECTRICAL_STRIKE) != 0) {
            nRank = 2;
        } else if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_REPULSOR_STRIKE) != 0) {
            nRank = 1;
        } else {
            goto noJab;
        }
        if (static_cast<unsigned char>(pAttack->m_nAttackResult - 1) < 3 && pTarget->AsSWSCreature() != 0) {
            unsigned short nDamage = g_pRules->RollDice(static_cast<unsigned char>(nRank), 6);

            CGameEffect* pDamage = new CGameEffect(1);
            pDamage->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE);
            pDamage->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS
                                          | (pDamage->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pDamage->SetCreator(m_objectId);
            pDamage->SetNumIntegers(22);
            for (int i = 0; i < 15; i++)
                pDamage->SetInteger(i, -1);
            pDamage->SetInteger(7, nDamage);
            pDamage->SetInteger(14, nDamage);
            pDamage->SetInteger(16, 0);
            // Damage flag 8 maps to sub-type index 3.
            pDamage->SetInteger(17, Log_Base2(8));
            pDamage->SetInteger(19, 1);
            pDamage->SetInteger(20, 1);
            pDamage->SetInteger(21, 0);
            pAttack->m_effects.Add(pDamage);

            CGameEffect* pSlow = new CGameEffect(1);
            pSlow->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_MOVEMENT_SPEED_DECREASE);
            pSlow->m_durationTypeBits =
                    (pSlow->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
            pSlow->SetInteger(0, 50);
            pSlow->SetCreator(m_objectId);
            pSlow->m_fDuration = 6.0f;
            pSlow->m_durationTypeBits =
                    (pSlow->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) | DURATION_TYPE_TEMPORARY;

            CSWSCreature* pJabbed = pTarget->AsSWSCreature();
            unsigned char nLevel = m_pStats->GetLevel(0);
            if (pJabbed->m_pStats->GetEffectLinkImmunity(pSlow) == 0) {
                // The chance that the debuff lands, checked before the save.
                int nRoll = rand_wincompatible() % 100;
                if (nRank == 1) {
                    if (nRoll >= 10)
                        goto dropSlow;
                } else if (nRank == 2) {
                    if (nRoll >= 15)
                        goto dropSlow;
                } else if (nRank == 3) {
                    if (nRoll > 19)
                        goto dropSlow;
                } else {
                    goto dropSlow;
                }
                if (pJabbed->SavingThrowRoll(SAVING_THROW_FORT, nLevel + 15, SAVING_THROW_TYPE_NONE, m_objectId, 0, 0, 1) == 0) {
                    pAttack->m_effects.Add(pSlow);
                    goto slowDone;
                }
            }
        dropSlow:
            delete pSlow;
        slowDone:;
        }
    }
noJab:

    if (pAttack->m_nFeatId == 0)
        return;
    if (m_pStats->GetFeatRemainingUses(pAttack->m_nFeatId) == 0) {
        pAttack->m_nFeatId = 0;
        return;
    }
    unsigned short nFeat = pAttack->m_nFeatId;
    switch (nFeat) {
    // The critical-strike line puts a dodge-AC buff on self, then a stun on the target.
    case FEAT_CRITICAL_STRIKE:
    case FEAT_IMPROVED_CRITICAL_STRIKE:
    case FEAT_MASTER_CRITICAL_STRIKE: {
        CGameEffect* pDodge = new CGameEffect(1);
        pDodge->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_AC_DECREASE);
        pDodge->m_durationTypeBits =
            CGameEffect::DURATION_TYPE_TEMPORARY | (pDodge->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pDodge->m_fDuration = 3.0f;
        pDodge->SetCreator(m_objectId);
        pDodge->SetInteger(0, CSWSEffectListHandler::AC_DODGE);
        pDodge->SetInteger(1, 5);
        pDodge->SetInteger(2, g_pRules->m_nNumRaces);
        pDodge->SetInteger(5, CSWSEffectListHandler::DAMAGE_FLAG_PHYSICAL);
        ApplyEffect(pDodge, 0, 0);

        if (static_cast<unsigned char>(pAttack->m_nAttackResult - 1) < 3 && pTarget->AsSWSCreature() != 0) {
            CGameEffect* pStun = new CGameEffect(1);
            pStun->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETSTATE);
            pStun->SetInteger(0, 4);
            pStun->m_durationTypeBits =
                (pStun->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) | CGameEffect::DURATION_TYPE_TEMPORARY;
            pStun->m_fDuration = 6.0f;
            pStun->SetCreator(m_objectId);
            CSWSCreature* pStunned = pTarget->AsSWSCreature();
            unsigned char nLevel = m_pStats->GetLevel(0);
            int nStrMod = static_cast<signed char>(m_pStats->m_nSTRMod);
            if (pStunned->m_pStats->GetEffectLinkImmunity(pStun) == 0
                && pStunned->SavingThrowRoll(SAVING_THROW_FORT, static_cast<unsigned short>(nStrMod + nLevel), SAVING_THROW_TYPE_NONE, m_objectId, 0, 0, 1) == 0) {
                pAttack->m_effects.Add(pStun);
                pAttack->m_effect.m_nFeedbackEffectType = EFFECT_STATE_STUNNED;
                nResult = 1;
            } else {
                delete pStun;
            }
            pAttack->m_effect.m_bHasApplicationData = 1;
        }
        break;
    }
    // The power-attack line is a Force push, and only on a critical hit (m_nAttackResult == 2).
    case FEAT_IMPROVED_POWER_ATTACK:
    case FEAT_POWER_ATTACK:
    case FEAT_MASTER_POWER_ATTACK:
        if (static_cast<unsigned char>(pAttack->m_nAttackResult - 1) < 3 && pTarget->AsSWSCreature() != 0
            && pAttack->m_nAttackResult == 2) {
            CGameEffect* pPush = new CGameEffect(1);
            pPush->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FORCEPUSHED);
            pPush->m_durationTypeBits =
                CGameEffect::DURATION_TYPE_TEMPORARY | (pPush->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pPush->m_fDuration = 0.1f;
            pPush->SetCreator(m_objectId);
            CSWSCreature* pPushed = pTarget->AsSWSCreature();
            unsigned char nLevel = m_pStats->GetLevel(0);
            int nStrMod = static_cast<signed char>(m_pStats->m_nSTRMod);
            if (pPushed->m_pStats->GetEffectLinkImmunity(pPush) == 0
                && pPushed->SavingThrowRoll(SAVING_THROW_FORT, static_cast<unsigned short>(nStrMod * 2 + nLevel), SAVING_THROW_TYPE_NONE, m_objectId, 0, 0, 1) == 0) {
                pAttack->m_effects.Add(pPush);
                pAttack->m_effect.m_nFeedbackEffectType = EFFECT_STATE_KNOCKDOWN;
                nResult = 1;
            } else {
                delete pPush;
            }
            pAttack->m_effect.m_bHasApplicationData = 1;
        }
        break;
    // The flurry line gives the same dodge-AC buff, with the bonus taken from the feat's own rank.
    case FEAT_FLURRY:
    case FEAT_IMPROVED_FLURRY: {
        int nDodgeBonus = (nFeat == FEAT_FLURRY) ? 2 : (nFeat == FEAT_IMPROVED_FLURRY ? 1 : 0);
        CGameEffect* pDodge = new CGameEffect(1);
        pDodge->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_AC_DECREASE);
        pDodge->m_durationTypeBits =
            CGameEffect::DURATION_TYPE_TEMPORARY | (pDodge->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pDodge->m_fDuration = 3.0f;
        pDodge->SetCreator(m_objectId);
        pDodge->SetInteger(0, CSWSEffectListHandler::AC_DODGE);
        pDodge->SetInteger(1, nDodgeBonus);
        pDodge->SetInteger(2, g_pRules->m_nNumRaces);
        pDodge->SetInteger(5, CSWSEffectListHandler::DAMAGE_FLAG_PHYSICAL);
        ApplyEffect(pDodge, 0, 0);
        break;
    }
    // The shield-breaker is a visual effect plus the shield strip, both hung on the attack.
    case CSWSCreatureStats::FEAT_SHIELD_BREAKER:
        if (static_cast<unsigned char>(pAttack->m_nAttackResult - 1) < 3 && pTarget->AsSWSCreature() != 0) {
            CGameEffect* pVfx = new CGameEffect(1);
            pVfx->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
            pVfx->m_durationTypeBits =
                    (pVfx->m_durationTypeBits & CGameEffect::SUBTYPE_CLEAR_MASK) | SUBTYPE_MAGICAL;
            pVfx->SetInteger(0, VFX_DUR_CLOAK_REMOVAL);
            pVfx->SetCreator(m_objectId);
            pAttack->m_effects.Add(pVfx);

            CGameEffect* pStrip = new CGameEffect(1);
            pStrip->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DESTROY_SHIELDS);
            pStrip->SetCreator(m_objectId);
            pAttack->m_effects.Add(pStrip);
        }
        break;
    }
}
#endif

// The ranged half of the special-attack pair runs a sniper-shot prelude gated on the client
// creature's current weapon animation class, then a switch over the attack's queued feat id.
void CSWSCreature::ResolveRangedSpecialAttack(CSWSObject* pTarget, int nSubAttack)
{
    if (pTarget == 0 || nSubAttack != 0)
        return;
    CSWSCombatAttackData* pAttack = m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);

#if !KOTOR1  // K1: no sniper shot and no power blast, only the feat switch.
    CSWCCreature* pClient = GetClientCreature();
    if (pClient != 0) {
        CSWCAnimBase* pAnim = static_cast<CSWCObject*>(pClient)->GetAnimationBase();
        // Weapon animation classes 5, 6 and 7 are the ranged families.
        if (static_cast<unsigned char>(pAnim->m_nWeaponClass - 5) < 3) {
            // The three ranks of the sniper-shot feat, best first; the rank is the effect parameter.
            int nRank;
            if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_ASSASSIN_PROTOCOL_III) != 0) {
                nRank = 3;
            } else if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_ASSASSIN_PROTOCOL_II) != 0) {
                nRank = 2;
            } else if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_ASSASSIN_PROTOCOL_I) != 0) {
                nRank = 1;
            } else {
                goto noSnipe;
            }
            // Critical hits only, and then a flat 20% chance.
            if (static_cast<unsigned char>(pAttack->m_nAttackResult - 1) < 3 && pTarget->AsSWSCreature() != 0
                && pAttack->m_nAttackResult == 2 && rand_wincompatible() % 100 <= 19) {
                CGameEffect* pKill = new CGameEffect(1);
                pKill->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_ASSASSINATE);
                pKill->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS
                                            | (pKill->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
                pKill->SetInteger(0, nRank);
                pKill->SetCreator(m_objectId);

                CSWSCreature* pVictim = pTarget->AsSWSCreature();
                unsigned char nLevel = m_pStats->GetLevel(0);
                if (pVictim->m_pStats->GetEffectLinkImmunity(pKill) == 0
                    && pVictim->SavingThrowRoll(SAVING_THROW_FORT, nLevel + 10, SAVING_THROW_TYPE_NONE, m_objectId, 0, 0, 1) == 0) {
                    pAttack->m_effects.Add(pKill);
                } else {
                    delete pKill;
                }
            }
        }
    }
noSnipe:

    if (pAttack->m_nFeatId == 0)
        return;
#endif
    if (m_pStats->GetFeatRemainingUses(pAttack->m_nFeatId) == 0) {
        pAttack->m_nFeatId = 0;
        return;
    }
    unsigned short nFeat = pAttack->m_nFeatId;
    int nDodgeBonus;
    switch (nFeat) {
#if !KOTOR1
    // The power-blast line is a Force push on a critical hit only. The DC leans on Dex where the
    // melee twin's leans on Str.
    case FEAT_IMPROVED_POWER_BLAST:
    case FEAT_POWER_BLAST:
    case FEAT_MASTER_POWER_BLAST:
        if (static_cast<unsigned char>(pAttack->m_nAttackResult - 1) < 3 && pTarget->AsSWSCreature() != 0
            && pAttack->m_nAttackResult == 2) {
            CGameEffect* pPush = new CGameEffect(1);
            pPush->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FORCEPUSHED);
            pPush->m_durationTypeBits =
                CGameEffect::DURATION_TYPE_TEMPORARY | (pPush->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pPush->m_fDuration = 0.1f;
            pPush->SetCreator(m_objectId);
            CSWSCreature* pPushed = pTarget->AsSWSCreature();
            unsigned char nLevel = m_pStats->GetLevel(0);
            signed char nDexMod = static_cast<signed char>(m_pStats->GetDEXMod(0));
            if (pPushed->m_pStats->GetEffectLinkImmunity(pPush) == 0
                && pPushed->SavingThrowRoll(SAVING_THROW_FORT, nDexMod * 2 + nLevel, SAVING_THROW_TYPE_NONE, m_objectId, 0, 0, 1) == 0) {
                pAttack->m_effects.Add(pPush);
                pAttack->m_effect.m_nFeedbackEffectType = EFFECT_STATE_KNOCKDOWN;
            } else {
                delete pPush;
            }
            pAttack->m_effect.m_bHasApplicationData = 1;
        }
        break;
#endif
#if KOTOR1  // K1: the rapid-shot cases come first.
    case FEAT_RAPID_SHOT:
        nDodgeBonus = 4;
        goto rapidShot;
    case FEAT_MULTI_SHOT:
        nDodgeBonus = 1;
        goto rapidShot;
    case FEAT_IMPROVED_RAPID_SHOT:
        nDodgeBonus = 2;
    rapidShot: {
        CGameEffect* pDodge = new CGameEffect(1);
        pDodge->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_AC_DECREASE);
        pDodge->m_durationTypeBits =
            CGameEffect::DURATION_TYPE_TEMPORARY | (pDodge->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pDodge->m_fDuration = 3.0f;
        pDodge->SetCreator(m_objectId);
        pDodge->SetInteger(0, CSWSEffectListHandler::AC_DODGE);
        pDodge->SetInteger(1, nDodgeBonus);
        pDodge->SetInteger(2, g_pRules->m_nNumRaces);
        pDodge->SetInteger(5, CSWSEffectListHandler::DAMAGE_FLAG_PHYSICAL);
        ApplyEffect(pDodge, 0, 0);
        break;
    }
#endif
    // The critical-strike line puts a dodge-AC buff on self (bonus 5), then a stun on the target.
    // The stun's DC is the Int modifier, read as a signed byte.
    case FEAT_IMPROVED_SNIPER_SHOT:
    case FEAT_SNIPER_SHOT:
    case FEAT_MASTER_SNIPER_SHOT: {
        CGameEffect* pDodge = new CGameEffect(1);
        pDodge->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_AC_DECREASE);
        pDodge->m_durationTypeBits =
            CGameEffect::DURATION_TYPE_TEMPORARY | (pDodge->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pDodge->m_fDuration = 3.0f;
        pDodge->SetCreator(m_objectId);
        pDodge->SetInteger(0, CSWSEffectListHandler::AC_DODGE);
        pDodge->SetInteger(1, 5);
        pDodge->SetInteger(2, g_pRules->m_nNumRaces);
        pDodge->SetInteger(5, CSWSEffectListHandler::DAMAGE_FLAG_PHYSICAL);
        ApplyEffect(pDodge, 0, 0);

        if (static_cast<unsigned char>(pAttack->m_nAttackResult - 1) < 3 && pTarget->AsSWSCreature() != 0) {
            CGameEffect* pStun = new CGameEffect(1);
            pStun->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_SETSTATE);
            pStun->SetInteger(0, 4);
            pStun->m_durationTypeBits =
                (pStun->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK) | CGameEffect::DURATION_TYPE_TEMPORARY;
            pStun->m_fDuration = 6.0f;
            pStun->SetCreator(m_objectId);
            unsigned char nLevel = m_pStats->GetLevel(0);
            int nIntMod = static_cast<signed char>(m_pStats->m_nINTMod);
            CSWSCreature* pStunned = pTarget->AsSWSCreature();
            if (pStunned->m_pStats->GetEffectLinkImmunity(pStun) == 0
                && pStunned->SavingThrowRoll(SAVING_THROW_FORT, static_cast<unsigned short>(nIntMod + nLevel), SAVING_THROW_TYPE_NONE, m_objectId, 0, 0, 1) == 0) {
                pAttack->m_effects.Add(pStun);
                pAttack->m_effect.m_nFeedbackEffectType = EFFECT_STATE_STUNNED;
            } else {
                delete pStun;
            }
            pAttack->m_effect.m_bHasApplicationData = 1;
        }
        break;
    }
#if !KOTOR1
    // The rapid-shot line gives the same dodge-AC buff on self, with the bonus taken from the feat.
    // One body is shared by the three arms.
    case FEAT_RAPID_SHOT:
        nDodgeBonus = 4;
        goto rapidShot;
    case FEAT_MULTI_SHOT:
        nDodgeBonus = 1;
        goto rapidShot;
    case FEAT_IMPROVED_RAPID_SHOT:
        nDodgeBonus = 2;
    rapidShot: {
        CGameEffect* pDodge = new CGameEffect(1);
        pDodge->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_AC_DECREASE);
        pDodge->m_durationTypeBits =
            CGameEffect::DURATION_TYPE_TEMPORARY | (pDodge->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pDodge->m_fDuration = 3.0f;
        pDodge->SetCreator(m_objectId);
        pDodge->SetInteger(0, CSWSEffectListHandler::AC_DODGE);
        pDodge->SetInteger(1, nDodgeBonus);
        pDodge->SetInteger(2, g_pRules->m_nNumRaces);
        pDodge->SetInteger(5, CSWSEffectListHandler::DAMAGE_FLAG_PHYSICAL);
        ApplyEffect(pDodge, 0, 0);
        break;
    }
#endif
    }
}

void CSWSCreature::ResolveElementalDamage(CSWSObject* pObject)
{
    K2_UNUSED(pObject);
}

void CSWSCreature::ApplyOnHitKnockdown(CSWSObject* pObject, CSWItemProperty* pProperty)
{
    K2_UNUSED(pObject); K2_UNUSED(pProperty);
    // Both parameters are unused; the creator and the round come from this.
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_fDuration = 0.1f;
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FORCEPUSHED);
    pEffect->m_durationTypeBits =
        ((pEffect->m_durationTypeBits | CGameEffect::DURATION_TYPE_TEMPORARY) & CGameEffect::DURATION_TYPE_CLEAR_MASK)
        | CGameEffect::DURATION_TYPE_TEMPORARY;
    pEffect->SetCreator(m_objectId);
    m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_effects.Add(pEffect);
}

#if !KOTOR1  // K2 only
int CSWSCreature::IsInTotalDefense()
{
    if (m_nStancePosture == STANCE_POSTURE_TOTAL_DEFENSE)
        return 1;
    if (m_pCombatRound != 0 && m_pCombatRound->m_nExecutingActionType == CSWSCombatRoundAction::TYPE_STANCE)
        return 1;
    return 0;
}
#endif // !KOTOR1

// Applies one attack's damage to the target; skipped for plot creatures and when damage is
// globally disabled.
void CSWSCreature::ResolveDamage(CSWSObject* pTarget, int a, int b)
{
    CSWSCombatAttackData* pAttackData = m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
    int nSneakAttack = pAttackData->m_bSneakAttack;
    unsigned char nCritState = pAttackData->m_nAttackResult;
    int bCritical = (nCritState == 2);
    int bOffhand = m_pCombatRound->GetOffHandAttack();
    int bGuiFlag = 0;
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState != 0)
        bGuiFlag = (m_bInParty != 0);
    if (bGuiFlag == 0 && pTarget->m_bPlot == 0 && a == 0 && g_bDisableDamage == 0) {
        int nDamage = m_pStats->GetDamageRoll(pTarget, bOffhand, bCritical, nSneakAttack, 0, 1);
        if (pTarget->AsSWSCreature() != 0)
            pTarget->AsSWSCreature()->GetCurrentHitPoints(0);  // result unused
        pAttackData->SetBaseDamage(nDamage);
        ResolveOnHitEffect(pTarget, a, b);
        ResolveOnHitVisuals(pTarget);
        if (nCritState == 2) {
            BroadcastVoiceChat(VOICE_CHAT_CRITICAL_HIT);
            return;
        }
        return;
    }
    int nDamage = (a == 0) ? 0 : b;
    pAttackData->SetBaseDamage(nDamage);
    pAttackData->SetDamage(static_cast<unsigned short>(GetDamageFlags()), nDamage);
}

void CSWSCreature::ResolveAttack(unsigned long long oidTarget, int a2, int a3, int a4, int a5,
                                 int a6, int a7)
{
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    if (pObject != 0 &&
        (pObject->AsSWSCreature() != 0 || pObject->AsSWSDoor() != 0 ||
         pObject->AsSWSPlaceable() != 0)) {
        CSWSObject* pTarget = pObject->AsSWSObject();
        m_oidAttackTarget = oidTarget;
        ActivityManager(4);
        CSWSItem* pItem = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
        if (pItem != 0 &&
            g_pRules->GetBaseItem(pItem->m_baseItem)->m_bRangedWeapon != 0) {
            ResolveRangedAttack(pTarget, a2, a3, a4, a5, a6, a7);
        } else {
            ResolveMeleeAttack(pTarget, a2, a3, a4, a5, a6, a7);
        }
        if (pTarget->AsSWSCreature() != 0) {
            CSWSCreature* pTargetCreature = pTarget->AsSWSCreature();
            if (m_nStealthXPTimer != 0 &&
                (pTargetCreature->m_pStats->m_bIsPC != 0 ||
                 g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsLeader(
                     pTargetCreature->m_objectId))) {
                m_nStealthXPTimer = -1;
            }
        }
    } else {
        m_pCombatRound->SetRoundPaused(0, OBJECT_INVALID);
        m_pCombatRound->SetPauseTimer(0, 0);
        SetAnimation(SERVER_ANIMATION_READY);
    }
}

// After a melee attack's damage is settled: flag the attack lethal if it will finish the
// target, then queue an instantaneous death effect when the attack carries one.
void CSWSCreature::ResolvePostMeleeDamage(CSWSObject* pObject)
{
    if (pObject == 0)
        return;
    CSWSCombatAttackData* pAttack =
        m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
    if (pAttack->GetTotalDamage(1) >= pObject->GetCurrentHitPoints(0))
        pAttack->m_bKillingBlow = 1;
    if (pObject->AsSWSCreature() == 0)
        return;
    pObject->AsSWSCreature();
    if (pAttack->m_bCoupDeGrace == 0)
        return;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DEATH);
    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                  (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(m_objectId);
    pEffect->SetInteger(0, 0);
    pEffect->SetInteger(1, 1);
    pAttack->m_effects.Add(pEffect);
}

// The ranged twin of the melee post-damage pass. The lethal-blow flag is decided against a
// creature target's hit points and against the plain object's otherwise; an attack that lands
// for no damage queues a once-per-round "weapon ineffective" client message; the death effect
// is additionally gated on the target being within 10 units.
void CSWSCreature::ResolvePostRangedDamage(CSWSObject* pObject)
{
    if (pObject == 0)
        return;
    CSWSCombatAttackData* pAttack =
        m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
    int nTotalDamage = pAttack->GetTotalDamage(1);
    int bOffhand = (pAttack->m_nWeaponAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_OFFHAND);
    if (pObject->AsSWSCreature() != 0) {
        pObject->AsSWSCreature();
        if (nTotalDamage >= pObject->AsSWSCreature()->GetCurrentHitPoints(0))
            pAttack->m_bKillingBlow = 1;
        if (pAttack->m_bCoupDeGrace == 1) {
            pAttack->m_bKillingBlow = 1;
            Vector vDelta = m_position - pObject->m_position;
            if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z <= 100.0f) {
                CGameEffect* pEffect = new CGameEffect(1);
                pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DEATH);
                pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                              (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
                pEffect->SetCreator(m_objectId);
                pEffect->SetInteger(0, 0);
                pEffect->SetInteger(1, 1);
                pAttack->m_effects.Add(pEffect);
            }
        }
    } else if (nTotalDamage >= pObject->GetCurrentHitPoints(0)) {
        pAttack->m_bKillingBlow = 1;
    }
    if (nTotalDamage <= 0 && m_pCombatRound->m_bWeaponSucks == 0 &&
        GetIsWeaponEffective(pObject->m_objectId, bOffhand) == 0) {
        CSWCCMessageData* pMessage = new CSWCCMessageData();
        pMessage->m_nType = CSWCCMessageData::MESSAGE_TYPE_FEEDBACK_TO_CALLER;
        pMessage->SetInteger(0, FEEDBACK_CANNOT_DAMAGE_TARGET);
        pAttack->m_messages.Add(pMessage);
        m_pCombatRound->m_bWeaponSucks = 1;
    }
}

void CSWSCreature::ResolveMeleeAnimations(int a1, int a2, CSWSObject* pTarget, int nTime, int a5)
{
    K2_UNUSED(a1); K2_UNUSED(a2);
    if (pTarget == 0)
        return;
    unsigned char nAttack = m_pCombatRound->m_currentAttack;
    CSWSCombatAttackData* pAttack = m_pCombatRound->GetAttack(nAttack);
    pAttack->m_nAnimationLength = static_cast<unsigned short>(nTime);
    m_pCombatRound->DecrementRoundLength(nTime, 0);
    CSWSCreature* pTargetCreature = pTarget->AsSWSCreature();
    if (pTargetCreature == 0)
        return;
    unsigned char bResult = pAttack->m_nAttackResult;
    if (static_cast<unsigned char>(bResult - 1) < 3) {
        pAttack->m_nReaxnAnimation = SERVER_ANIMATION_DAMAGE;
    } else if (static_cast<unsigned char>(bResult - 8) < 3) {
        pAttack->m_nReaxnAnimation = SERVER_ANIMATION_PARRY;
    } else if (static_cast<unsigned char>(bResult - 4) < 3) {
        pAttack->m_nReaxnAnimation = SERVER_ANIMATION_DODGE;
    }
#if KOTOR1  // K1: only an idle (state 0), non-dying target reacts.
    CSWSCombatRound* pCombatRound = m_pCombatRound;
    int nCanCounter;
    if (pTargetCreature->m_nState == EFFECT_STATE_NONE && pTargetCreature->GetDeadTemp() == 0) {
        nCanCounter = 0;
    } else {
        nCanCounter = 1;
    }
    pCombatRound->m_bTargetCannotReact = nCanCounter;
    if (pTargetCreature->m_nState == EFFECT_STATE_NONE && pTargetCreature->GetDeadTemp() == 0 &&
        (pTargetCreature->m_pCombatRound->CheckActionLengthAtTime(m_objectId, nTime, 0) != 0 ||
         a5 != 0) &&
#else
    int nCanCounter = 1;
    if ((pTargetCreature->m_nState == EFFECT_STATE_NONE ||
         pTargetCreature->m_nState == EFFECT_STATE_CONFUSED ||
         pTargetCreature->m_nState == EFFECT_STATE_DROID_CONFUSED) &&
        (pTargetCreature->m_bInParty == 0 || pTargetCreature->GetCurrentHitPoints(0) > 0)) {
        nCanCounter = 0;
    }
    if (pTargetCreature->m_nStancePosture == STANCE_POSTURE_MEDITATIVE) {
        nCanCounter = 1;
    }
    m_pCombatRound->m_bTargetCannotReact = nCanCounter;
    if ((pTargetCreature->m_nState == EFFECT_STATE_NONE ||
         pTargetCreature->m_nState == EFFECT_STATE_CONFUSED ||
         pTargetCreature->m_nState == EFFECT_STATE_DROID_CONFUSED) &&
        (pTargetCreature->m_bInParty == 0 || pTargetCreature->GetCurrentHitPoints(0) >= 1) &&
        pTargetCreature->m_nStancePosture != STANCE_POSTURE_MEDITATIVE &&
        (pTargetCreature->m_pCombatRound->CheckActionLengthAtTime(m_objectId, nTime, 0) != 0 ||
         a5 != 0) &&
#endif
        m_pCombatRound->m_bEngaged != 0) {
        m_pCombatRound->GetAttack(nAttack)->m_nReaxnAnimLength = static_cast<short>(nTime);
        m_pCombatRound->GetAttack(nAttack)->m_nReaxnDelay = 0;
        pTargetCreature->SetAnimation(pAttack->m_nReaxnAnimation);
        pTargetCreature->m_pCombatRound->DecrementRoundLength(nTime, 0);
        return;
    }
    pAttack->m_nReaxnAnimation = SERVER_ANIMATION_READY;
}

unsigned long long CSWSCreature::GetFirstAttacker()
{
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (pArea != 0) {
        CExoArrayList<unsigned long long>* pList = m_pAttackerList;
        if (pList != 0) {
            if (pList->m_pArray != 0)
                delete[] pList->m_pArray;
            operator delete(pList);
        }
        m_pAttackerList = new CExoArrayList<unsigned long long>();
        if (pArea->m_aObjectsByX.m_count >= 1) {
            for (int i = 0; i < pArea->m_aObjectsByX.m_count; i++) {
                unsigned long long oid = pArea->m_aObjectsByX.m_pArray[i];
                CSWSCreature* pCreature =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oid);
                if (pCreature != 0 && pCreature->m_oidAttackTarget == m_objectId)
                    m_pAttackerList->Add(oid);
            }
        }
        if (m_pAttackerList->m_count >= 1) {
            m_nNextAttackerIdx = 1;
            return m_pAttackerList->m_pArray[0];
        }
    }
    return OBJECT_INVALID;
}

unsigned long long CSWSCreature::GetNextAttacker()
{
    if (m_pAttackerList == 0)
        return OBJECT_INVALID;
    if (m_nNextAttackerIdx >= m_pAttackerList->m_count)
        return OBJECT_INVALID;
    int nIdx = m_nNextAttackerIdx;
    m_nNextAttackerIdx = nIdx + 1;
    return m_pAttackerList->m_pArray[nIdx];
}

void CSWSCreature::SetCombatReactionTime(int a, int b, int c)
{
    m_pCombatRound->GetAttack(a)->m_nReaxnAnimLength = static_cast<short>(b);
    m_pCombatRound->GetAttack(a)->m_nReaxnDelay = static_cast<unsigned short>(c);
}

int CSWSCreature::GetClientRangedAnimation(int a, int b, int c)
{
    return g_pAppManager->m_pClientExoApp->GetClientRangedAnimation(m_objectId, a, b, c);
}

int CSWSCreature::GetClientMeleeAnimation(unsigned long long a, unsigned short b, int c, int d, int e)
{
    return g_pAppManager->m_pClientExoApp->GetClientMeleeAnimation(m_objectId, a, b, c, d, e);
}

void CSWSCreature::SetCombatPaused(unsigned long long objectId, int bPaused)
{
    m_pCombatRound->SetRoundPaused(1, objectId);
    m_pCombatRound->SetPauseTimer(bPaused, 0);
}

void CSWSCreature::ClearAttackerList()
{
    CExoArrayList<unsigned long long>* pList = m_pAttackerList;
    if (pList != 0) {
        if (pList->m_pArray != 0)
            delete[] pList->m_pArray;
        operator delete(pList);
        m_pAttackerList = 0;
        m_nNextAttackerIdx = 0;
    }
}

void CSWSCreature::InitializeNewAttackTarget(unsigned long long objectId,
                                             CSWSObjectActionNode* pNode)
{
    if (pNode != 0)
        pNode->m_args.m_attackTargetId = objectId;
    SetLockOrientationToObject(objectId, 0);
    m_attackTargetId = objectId;
    SetAttemptedAttackTarget(objectId);
    m_pCombatRound->UpdateAttackTargetForAllActions(objectId);
    m_bPassiveAttackBehaviour = 1;
}

int CSWSCreature::IsSpellCastingAnimation(unsigned short nAnimation)
{
    int bCasting = 0;
    if (nAnimation == SERVER_ANIMATION_CONJURE_1 || nAnimation == SERVER_ANIMATION_CONJURE_2 || nAnimation == SERVER_ANIMATION_CAST_1 ||
        nAnimation == SERVER_ANIMATION_CAST_2 || nAnimation == SERVER_ANIMATION_CAST_3 || nAnimation == SERVER_ANIMATION_CAST_4 ||
        nAnimation == SERVER_ANIMATION_THROW_SABER_LOOP || nAnimation == SERVER_ANIMATION_INJECT || nAnimation == 10071 ||
        nAnimation == SERVER_ANIMATION_CONJURE_3 || nAnimation == SERVER_ANIMATION_POWER_ATTACK)
        bCasting = 1;
    return bCasting;
}

// Executes the physical-attack action. It re-derives where the attacker has to stand, asks the area
// whether the line of sight is clear, and then either resolves the attack in place or re-queues
// itself behind a move-to-point.
int CSWSCreature::AIActionPhysicalAttacks(CSWSObjectActionNode* pNode)
{
    Vector vDest = {};
    float fMaxRange = 0.0f;
    int nOne = 1;

    m_pPathfindInfo->m_fDesiredOffset = 0.0f;
    m_pPathfindInfo->m_oidTarget = OBJECT_INVALID;
    m_nEndRoundScriptTimer = 3000;

    // The node's arguments 7..9 default to 0, 4, 0 and are only read when its first argument is
    // set. They are ResolveAttack's last three arguments and nothing else reads them.
    int nCutsceneAttackResult = 4;
    int nCutsceneAnimation = 0;
    int nCutsceneDamage = 0;

    int bCutsceneAttack = pNode->m_args.m_bAttackCutscene;
    unsigned long long oidTarget = pNode->m_args.m_attackQueuedTargetId;
    int nAttackKind = pNode->m_args.m_attackKind;
    int nAnimationId = pNode->m_args.m_attackAnimationId;
    int nPauseTime = pNode->m_args.m_attackPauseTime;
    int nNumAttacks = pNode->m_args.m_attackNumAttacks;
    unsigned short nFeatId = pNode->m_args.m_attackFeatId;
    if (bCutsceneAttack != 0) {
        nCutsceneAnimation = pNode->m_args.m_attackCutsceneAnimation;
        nCutsceneAttackResult = pNode->m_args.m_attackCutsceneResult;
        nCutsceneDamage = pNode->m_args.m_attackCutsceneDamage;
    }

    if (m_bInParty != 0) {
        CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
        int nIndex = pClient->GetSWParty()->GetIndex(
            pClient->ServerToClientObjectId(m_objectId));
        if (nIndex >= 0)
#if KOTOR1  // K1: strref 48208
            pClient->GetSWParty()->m_members[nIndex].SetCombatMessage(STRREF_USE_LIGHTSABER);
#else
            pClient->GetSWParty()->m_members[nIndex].SetCombatMessage(STRREF_COMBAT_MODE_ENGAGED);
#endif
    }

    if (GetDead() != 0
#if KOTOR1
            || GetIsPCDying() != 0 || GetDeadTemp() != 0
#else
            || GetDeadTemp() != 0
#endif
            || (m_nAIState & 0x84) != 0x84
            || oidTarget == m_objectId) {
        CancelAllCombatModes(1);
        return ACTION_FAILED;
    }

    CGameObject* pTarget = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    if (pTarget == 0 || pTarget->AsSWSObject() == 0)
        return ACTION_COMPLETE;
    CSWSObject* pObject = pTarget->AsSWSObject();

    int bTargetDown = 0;
    if (pObject->AsSWSCreature() != 0) {
        CSWSCreature* pTargetCreature = pObject->AsSWSCreature();
        bTargetDown = pTargetCreature->GetDeadTemp();
    }
    if (pObject->GetDead() != 0 || bTargetDown != 0)
        return ACTION_COMPLETE;
    if (HasTargetBeenDetected(oidTarget, 1) == 0)
        return ACTION_COMPLETE;

    vDest = pObject->m_position;
    CSWSArea* pTargetArea = static_cast<CSWSArea*>(pObject->GetArea());
    int nChoreographed = m_pCombatRound->ResolveChoreographedState(oidTarget);
    if (nChoreographed != 0)
        nAnimationId = SERVER_ANIMATION_ATTACK_2;
    if (pObject->AsSWSDoor() != 0) {
        float fUseRange = 0.0f;
        m_pPathfindInfo->m_oidTarget = oidTarget;
        GetUseRange(oidTarget, vDest, fUseRange, 1, 0);
        m_pPathfindInfo->m_fDesiredOffset = fUseRange;
    }
    fMaxRange = MaxAttackRange(oidTarget, nChoreographed, 0);
    float fDesired = DesiredAttackRange(oidTarget, nChoreographed);

    unsigned long long oidBlocker = OBJECT_INVALID;
    Vector vFrom = m_position;
    vFrom.z = vFrom.z + 1.5f;
    Vector vTo = {};
    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR
            || pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
        float fPointRange;
        GetUseRange(pObject->m_objectId, vTo, fPointRange, 0, 0);
        vDest = vTo;
    } else {
        vTo = pObject->m_position;
    }
    vTo.z = vTo.z + 1.5f;

    Vector vHit = {};
    int nLosBlocked = 0;
    if (GetArea() != 0) {
        vFrom.z = vFrom.z + 0.01f;
        vTo.z = vTo.z + -0.01f;
        nLosBlocked = static_cast<CSWSArea*>(GetArea())->ClearLineOfSight(
            vFrom, vTo, &vHit, &oidBlocker, m_objectId, oidTarget, RangeWeaponEquipped(), 0);
        // result unused
        if (RangeWeaponEquipped() == 0 && nLosBlocked != 0)
            pObject->AsSWSCreature();
        if (nLosBlocked != 0) {
            m_vLastBlockedPos.x = 0.0f;
            m_vLastBlockedPos.y = 0.0f;
            m_vLastBlockedPos.z = 0.0f;
        } else if (equal(m_vLastBlockedPos, m_position) != 0 && m_bPlayerCharacter != 0) {
            // A clear line, and no perceived movement since the last time it was blocked: give
            // up on this target and re-acquire.
            SendFeedbackMessage(FEEDBACK_TARGET_UNREACHABLE, NULL);
            m_vLastBlockedPos.x = 0.0f;
            m_vLastBlockedPos.y = 0.0f;
            m_vLastBlockedPos.z = 0.0f;
            m_oidAttackTarget = OBJECT_INVALID;
            unsigned long long oidNew =
                GetNearestEnemy(MaxAttackRange(m_objectId, 0, 1), oidTarget, OBJECT_INVALID);
            if (oidNew == OBJECT_INVALID) {
                CancelAllCombatModes(1);
                return ACTION_COMPLETE;
            }
            CGameObject* pNewTarget = g_pAppManager->m_pServerExoApp->GetGameObject(oidNew);
            if (pNewTarget == 0
                    || pNewTarget->m_objectType != CGameObject::OBJECT_TYPE_CREATURE) {
                CancelAllCombatModes(1);
                return ACTION_COMPLETE;
            }
            InitializeNewAttackTarget(oidNew, pNode);
            return ACTION_IN_PROGRESS;
        } else {
            m_vLastBlockedPos = m_position;
        }
    }

    // The flat (XY-only) span to the attack point.
    Vector vDelta = m_position - vDest;
    vDelta.z = 0.0f;
    float fDistSq = vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z;

    Vector vApproach = {};
#if KOTOR1  // K1: no door approach point, no run-distance shortcut, no door distance gate.
    if (fDistSq < fDesired * fDesired + -0.2f) {
        {
#else
    if (fDistSq < fDesired * fDesired + -0.2f
            || (pObject != 0 && pObject->AsSWSDoor() != 0)) {
        if (pObject->AsSWSDoor() != 0) {
            Vector vAway = vDest - pObject->m_position;
            vAway = normalize(vAway);
            vApproach = vDest + vAway * fDesired * 2.0f;
        } else {
#endif
            Vector vToward = vDelta * -1.0f;
            vToward = normalize(vToward);
            vApproach = pObject->m_position - vToward * fDesired;
        }
        CWalkHitInfo hitInfo = {};
        hitInfo.m_hitObjectId = OBJECT_INVALID;
        hitInfo.m_hitSubType = 0;
        if (g_pAppManager->m_pServerExoApp->TestDirectLine(
                m_objectId, static_cast<CSWSArea*>(GetArea())->m_resRef, m_position, vApproach,
                static_cast<float>(m_pPathfindInfo->m_fPersonalSpace), m_pPathfindInfo->m_fHeight, 1,
                &hitInfo) != 1) {
            vApproach = hitInfo.m_hitPosition;
            fDesired = magnitude(pObject->m_position - vApproach);
        }
    }

#if KOTOR1
    int bInRange = 0;
    int nForceJump = 0;
    if (pTargetArea == static_cast<CSWSArea*>(GetArea())
            && !(fDistSq > fMaxRange * fMaxRange)
            && (!(fDistSq < fDesired * fDesired + -0.2f) || m_bInParty != 0)
            && nLosBlocked != 0) {
        bInRange = 1;
        goto combatRound;
    }
#else
    float fDoorDistSq;
    if (pObject->AsSWSDoor() != 0) {
        float fGapX = m_position.x - vApproach.x;
        float fGapY = m_position.y - vApproach.y;
        float fGapZ = 0.0f;
        fDoorDistSq = fGapX * fGapX + fGapY * fGapY + fGapZ * fGapZ;
    } else {
        fDoorDistSq = 0.0f;
    }

    int bInRange = 0;
    int nForceJump = 0;
    if (pObject->AsSWSCreature() != 0) {
        float fRunDist = 0.0f;
        g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
            m_appearance.m_headId, CTwoDimArrays::COL2DA_APPEARANCETABLE_RUNDIST, &fRunDist);
        if (fRunDist < 0.0001f)
            goto combatRound;
    }

    if (pTargetArea == static_cast<CSWSArea*>(GetArea())
            && !(fDistSq > (fMaxRange + 0.1f) * (fMaxRange + 0.1f))
            && (!(fDistSq < fDesired * fDesired + -0.2f) || m_bInParty != 0)
            && nLosBlocked != 0) {
        bInRange = 1;
        if (pObject == 0 || pObject->AsSWSDoor() == 0 || !(fDoorDistSq > fDesired * fDesired))
            goto combatRound;
    }
#endif

    {
        int bRangeWeapon = RangeWeaponEquipped();
#if KOTOR1
        if (fDistSq < fDesired * fDesired + -0.2f)
            bInRange = 1;
#else
        if (fDistSq < fDesired * fDesired + -0.2f
                || (pObject != 0 && pObject->AsSWSDoor() != 0
                    && fDoorDistSq > fDesired * fDesired))
            bInRange = 1;
#endif
        if (m_bPassiveAttackBehaviour != 0) {
            CancelAllCombatModes(1);
            return ACTION_FAILED;
        }

        unsigned long long oidArea;
        if (pTargetArea != 0) {
            oidArea = pTargetArea->m_objectId;
        } else {
            if (pObject->AsSWSCreature() == 0 || m_oidEncounter != OBJECT_INVALID) {
                CancelAllCombatModes(1);
                return ACTION_FAILED;
            }
            oidArea = pObject->AsSWSCreature()->m_currentAreaId;
            vDest = pObject->AsSWSCreature()->m_vDesiredAreaLocation;
        }
        if (nLosBlocked != 0) {
            nForceJump = ResolveMoveToForceJump(pObject, nFeatId);
            if (nForceJump != 0) {
                bInRange = 1;
                goto combatRound;
            }
        }

#if !KOTOR1  // K1: no party-leader or AI-state gate on the approach actions.
        int nAiState = m_pStats->m_aiState;
        unsigned long long oidLeader = OBJECT_INVALID;
        CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
        if (pLeader != 0)
            oidLeader = pLeader->GetServerCreature()->m_objectId;
#endif

        if (pTarget->AsSWSDoor() != 0) {
            float fDoorRange;
            GetUseRange(oidTarget, vDest, fDoorRange, 1, 0);
            fDesired = fDoorRange;
            vDest = pTarget->AsSWSDoor()->GetNearestActionPoint(m_position, 0);
            m_pPathfindInfo->m_oidTarget = oidTarget;
            m_pPathfindInfo->m_fDesiredOffset = fDoorRange;
        } else if (pTarget->AsSWSPlaceable() != 0) {
            float fPlaceRange;
            bInRange = 0;
            GetUseRange(oidTarget, vDest, fPlaceRange, 1, 0);
            fDesired = fPlaceRange;
            vDest = pTarget->AsSWSPlaceable()->GetNearestActionPoint(m_position);
        }

        // Re-queue ourselves behind whatever we are about to add in front of it.
#ifdef K2_FIX_ENGINE_BUGS
        int nFeatParam = nFeatId;   // an int parameter (tag 1) is read as 4 bytes
#else
        unsigned short& nFeatParam = nFeatId;
#endif
        AddActionToFront(AIACTION_PHYSICAL_ATTACKS, pNode->m_nGroupId,
                         ACTION_PARAM_INT, &bCutsceneAttack, ACTION_PARAM_OBJECT, &oidTarget,
                         ACTION_PARAM_INT, &nAttackKind, ACTION_PARAM_INT, &nAnimationId,
                         ACTION_PARAM_INT, &nPauseTime, ACTION_PARAM_INT, &nNumAttacks,
                         ACTION_PARAM_INT, &nFeatParam, ACTION_PARAM_INT, &nCutsceneAnimation,
                         ACTION_PARAM_INT, &nCutsceneAttackResult,
                         ACTION_PARAM_INT, &nCutsceneDamage, 0, 0, 0, 0, 0, 0);
        int nOneAgain = 1;
        if (bInRange != 0) {
            int bJump = 0;
#if !KOTOR1
            if (pObject != 0)
                bJump = pObject->AsSWSDoor() != 0 && fDoorDistSq > 3.0f;
#endif
            Vector vZero = {};
            AddMoveToPointActionToFront(pNode->m_nGroupId, vApproach, oidArea, OBJECT_INVALID,
                                        bJump, 0.0f, 0.0f, 0, 0, 0, 0, 0, 0, vZero, 0, 0, 0);
            if (m_bInParty != 0 && fDistSq > 0.25f) {
                CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
                int nIndex = pClient->GetSWParty()->GetIndex(
                    pClient->ServerToClientObjectId(m_objectId));
                if (nIndex >= 0)
                    pClient->GetSWParty()->m_members[nIndex].SetCombatMessage(STRREF_CLOSING_TO_ATTACK_RANGE);
            }
            return ACTION_COMPLETE;
        }

        int nZero = 0;
        float fRange = fDesired;
        if (bRangeWeapon != 0)
            fRange = fMaxRange;
#if !KOTOR1
        if (static_cast<unsigned int>(nAiState - NPC_AISTYLE_PARTY_RANGED) > 1 || oidLeader == m_objectId)
#endif
        {
            AddActionToFront(AIACTION_CHECK_MOVE_TO_OBJECT_RADIUS, pNode->m_nGroupId,
                             ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nOneAgain,
                             ACTION_PARAM_FLOAT, &fRange, ACTION_PARAM_FLOAT, &fMaxRange,
                             ACTION_PARAM_INT, &nOne, ACTION_PARAM_INT, &nZero,
                             ACTION_PARAM_INT, &bRangeWeapon, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        }
        AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                         ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

        float fMoveRange = fDesired;
        if (bRangeWeapon != 0)
            fMoveRange = fMaxRange;
        if (pObject->AsSWSCreature() != 0) {
            CPathfindInformation* pPathInfo = m_pPathfindInfo;
            pPathInfo->m_fDesiredOffset =
                fDesired - pObject->AsSWSCreature()->m_pPathfindInfo->m_fCrePersonalSpace;
            m_pPathfindInfo->m_oidTarget = oidTarget;
        }
#if !KOTOR1
        if (static_cast<unsigned int>(nAiState - NPC_AISTYLE_PARTY_RANGED) > 1 || oidLeader == m_objectId)
#endif
        {
            Vector vZero = {};
            AddMoveToPointActionToFront(pNode->m_nGroupId, vDest, oidArea, oidTarget, 1,
                                        fMoveRange, 0.0f, 0, 0, 0, 0, 0, 0, vZero, 0, 0,
                                        bRangeWeapon);
            if (m_bInParty != 0 && fDistSq > 0.25f) {
                CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
                int nIndex = pClient->GetSWParty()->GetIndex(
                    pClient->ServerToClientObjectId(m_objectId));
                if (nIndex >= 0)
                    pClient->GetSWParty()->m_members[nIndex].SetCombatMessage(STRREF_CLOSING_TO_ATTACK_RANGE);
            }
        }
        return ACTION_COMPLETE;
    }

combatRound:
    if (m_pCombatRound == 0) {
        CancelAllCombatModes(1);
        return ACTION_FAILED;
    }
    if (g_pAppManager->m_pServerExoApp->GetPauseState(2) != 0
            || m_pCombatRound->m_bRoundPaused != 0)
        return ACTION_IN_PROGRESS;

    {
        int nEngaged = m_pCombatRound->ResolveEngagedState(oidTarget);
        int nMaster = m_pCombatRound->ResolveMasterState(oidTarget, nEngaged);
        if (nEngaged != 0) {
            if (IsCreature() != 0
                    || (pObject->AsSWSCreature() != 0
                        && pObject->AsSWSCreature()->IsCreature() != 0))
                nAnimationId = SERVER_ANIMATION_ATTACK_2;
            if (nMaster != 0) {
                m_pCombatRound->StartCombatRound(oidTarget, nEngaged, nMaster, nFeatId,
                                                 bCutsceneAttack);
                CSWSCreature* pOther =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
                if (pOther != 0) {
                    pOther->m_pCombatRound->StartCombatRound(m_objectId, nEngaged, 0, 0,
                                                             bCutsceneAttack);
                    pOther->m_bInitiativeExpired = 1;
                }
            }
        } else {
            nAnimationId = SERVER_ANIMATION_ATTACK;
            m_pCombatRound->StartCombatRound(oidTarget, 0, nMaster, nFeatId, bCutsceneAttack);
        }

        if (m_pCombatRound->m_bRoundStarted == 0)
            return ACTION_IN_PROGRESS;
        if ((nForceJump != 0 || nAttackKind == CSWSCombatRoundAction::TYPE_FEAT_ATTACK)
            && m_nGuardStanceFeat == 0) {
            if (m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON) != 0) {
                m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_nFeatId = nFeatId;
#if !KOTOR1  // K1: only an armed attacker's feat is queued.
            } else if (nAttackKind == CSWSCombatRoundAction::TYPE_FEAT_ATTACK
                       && nFeatId == CSWSCreatureStats::FEAT_SHIELD_BREAKER) {
                m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_nFeatId = CSWSCreatureStats::FEAT_SHIELD_BREAKER;
            } else if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_COMPLEX_UNARMED_ANIMS) != 0
                       && nAttackKind == CSWSCombatRoundAction::TYPE_FEAT_ATTACK && nForceJump == 0) {
                m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_nFeatId = nFeatId;
#endif
            }
        }

        SetAnimation(nAnimationId);
        SetAttemptedAttackTarget(oidTarget);
        m_pCombatRound->SetRoundPaused(1, m_objectId);
        m_pCombatRound->SetPauseTimer(nPauseTime, 0);
        m_pCombatRound->m_nCastsThisRound = m_pCombatRound->m_nCastsThisRound + 1;
        if (nEngaged != 0) {
            if (nMaster != 0) {
                CSWSCreature* pOther =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                        m_pCombatRound->m_oidSlave);
                if (pOther != 0) {
                    pOther->m_pCombatRound->SetRoundPaused(1, pOther->m_pCombatRound->m_oidMaster);
                    pOther->m_pCombatRound->SetPauseTimer(nPauseTime, 1);
                }
            } else {
                CSWSCreature* pOther =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
                if (pOther != 0) {
                    pOther->m_pCombatRound->SetRoundPaused(1, pOther->m_pCombatRound->m_oidSlave);
                    pOther->m_pCombatRound->SetPauseTimer(nPauseTime, 0);
                }
            }
        }

        unsigned long long oidSwap = m_pCombatRound->m_oidPendingTargetSwap;
        if (oidSwap == OBJECT_INVALID) {
            oidSwap = oidTarget;
        } else {
            InitializeNewAttackTarget(oidSwap, pNode);
            m_pCombatRound->m_oidPendingTargetSwap = OBJECT_INVALID;
            oidTarget = oidSwap;
        }
        SetLockOrientationToObject(oidSwap, 0);
#if KOTOR1  // K1: the attack always resolves here.
        ResolveAttack(oidTarget, nNumAttacks, nPauseTime, bCutsceneAttack, nCutsceneAnimation,
                      nCutsceneAttackResult, nCutsceneDamage);
#else
        if (bInRange != 0
                || (m_sTag.CompareNoCase("g_turretbl002") == 0
                    && fDistSq < (fMaxRange + 0.1f) * (fMaxRange + 0.1f)))
            ResolveAttack(oidTarget, nNumAttacks, nPauseTime, bCutsceneAttack, nCutsceneAnimation,
                          nCutsceneAttackResult, nCutsceneDamage);
#endif
        return ACTION_COMPLETE;
    }
}

void CSWSCreature::CancelAllCombatModes(int a)
{
    K2_UNUSED(a);
    SetLockOrientationToObject(OBJECT_INVALID, 0);
    SetAnimation(SERVER_ANIMATION_PAUSE);
    SetCombatMode(CREATURE_COMBAT_MODE_NONE, 1);
    m_pCombatRound->ClearAllSpecialAttacks();
}

int CSWSCreature::IsLightsaberEquipped()
{
    int bEquipped = 0;
    CSWSItem* pItem = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    if (pItem != 0)
        bEquipped = pItem->IsLightsaber() != 0;
    pItem = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
    if (pItem != 0) {
        if (pItem->IsLightsaber() != 0)
            bEquipped = 1;
    }
    return bEquipped;
}

void CSWSCreature::ResolvePoweredItems(int bPowered)
{
    CSWSItem* pItem = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    if (pItem != 0)
        pItem->PowerItem(bPowered, 0, 1);
    pItem = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
    if (pItem != 0)
        pItem->PowerItem(bPowered, 0, 1);
}

__attribute__((no_stack_protector)) int CSWSCreature::ResolveRangedMissToParry(CSWSObject* pObject, int a, int b)
{
    int nOut1 = 0;
#if KOTOR1  // K1: no redirect out-param.
    int bResult = IsProjectileParryable(pObject, a, 1, nOut1, b);
#else
    int nOut2 = 1;
    int bResult = IsProjectileParryable(pObject, a, 1, nOut1, nOut2, b);
#endif
    if (bResult != 0) {
        CSWSCombatAttackData* pAttack = m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
        pAttack->m_nAttackResult = (nOut1 == 0) ? ATTACK_RESULT_PARRIED : 10;
    }
    return bResult;
}

int CSWSCreature::CalculateMaxElementalDamage(CSWSObject* pObject, int nDamageType)
{
    K2_UNUSED(nDamageType);
    if (pObject == 0 || pObject->m_bPlot != 0)
        return 0;
    int nBonus = 0;
    return GetTotalEffectBonus(EFFECT_BONUS_DAMAGE, pObject, nBonus, 1, 1, 0, 0, EFFECT_BONUS_ANY,
                               EFFECT_BONUS_ANY, 0, 0);
}

#if !KOTOR1  // K2 only
int CSWSCreature::IsProjectileParryable(CSWSObject* pObject, int a, int b,
                                        int& nOut1, int& nOut2, int c)
{
    nOut2 = 1;
    CSWSCombatAttackData* pAttack = m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
    if (pObject->AsSWSCreature() == 0)
        return 0;
    if (pAttack->m_bRangedAttack == 0)
        return 0;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    int bParryable = 0;
    if ((pCreature->m_pStats->HasFeat(FEAT_JEDI_DEFENSE) != 0 || pCreature->m_pStats->HasFeat(FEAT_ADVANCED_JEDI_DEFENSE) != 0 ||
         pCreature->m_pStats->HasFeat(FEAT_MASTER_JEDI_DEFENSE) != 0) &&
        (pCreature->m_nState == EFFECT_STATE_NONE ||
         pCreature->m_nState == EFFECT_STATE_CONFUSED ||
         pCreature->m_nState == EFFECT_STATE_DROID_CONFUSED) &&
        (pCreature->m_bInParty == 0 || pCreature->GetCurrentHitPoints(0) >= 1) &&
        pCreature->m_nStancePosture != STANCE_POSTURE_MEDITATIVE && pCreature->GetClientCreature() != 0 &&
        pCreature->IsLightsaberEquipped() != 0 && pCreature->m_bLightsaberThrowActive == 0) {
        if (c == -1) {
            if (pAttack->m_nWeaponAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_ONHAND)
                c = 0;
            else if (pAttack->m_nWeaponAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_OFFHAND)
                c = 1;
            else
                c = -1;
        }
        unsigned int nSlot =
                (c == 0) ? CSWInventory::SLOT_RIGHTWEAPON : ((c == 1) ? CSWInventory::SLOT_LEFTWEAPON : 0);
        CSWSItem* pItem = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(nSlot));
        if (pItem != 0 && (pItem->GetBaseItem()->m_abDamageFlagBytes[1] & (DAMAGE_TYPE_BLASTER >> 8)) != 0) {
            CSWCCreature* pClient = pCreature->GetClientCreature();
            Vector vTarget = pClient->m_orientation;
            Vector vSelf = m_orientation;
            float fFacing = vTarget.x * vSelf.x + vTarget.y * vSelf.y + vTarget.z * vSelf.z;
            bParryable = (fFacing <= 0.0f);
        }
    }
    if (a != 0 && b != 0 && bParryable == 0) {
        if (pCreature->GetClientCreature() != 0 &&
            pCreature->GetClientCreature()->GetForceShield() != 0) {
            unsigned int nSlot =
                    (c == 0) ? CSWInventory::SLOT_RIGHTWEAPON : ((c == 1) ? CSWInventory::SLOT_LEFTWEAPON : 0);
            CSWSItem* pItem = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(nSlot));
            if (pItem != 0) {
                unsigned char nAmmunitionType = pItem->GetBaseItem()->m_nAmmunitionType;
                int nShieldHit = 0;
                g_pRules->m_p2DArrays->m_pAmmunitionTypes->GetINTEntry(
                    nAmmunitionType, CExoString("ShieldHit"), &nShieldHit);
                bParryable = (nShieldHit != 0);
            } else {
                bParryable = 0;
            }
            nOut1 = pCreature->GetClientCreature()->GetForceShield();
        } else {
            bParryable = 0;
        }
    }
    int bHasDeflect = pCreature->m_pStats->HasSpell(0, FORCE_POWER_FORCE_REPULSION, 0);
    int bHasRedirect = pCreature->m_pStats->HasSpell(0, FORCE_POWER_FORCE_REDIRECTION, 0);
    if (bParryable != 0)
        return 1;
    if (bHasDeflect == 0 && bHasRedirect == 0)
        return 0;
    if ((pCreature->m_nState == EFFECT_STATE_NONE ||
         pCreature->m_nState == EFFECT_STATE_CONFUSED ||
         pCreature->m_nState == EFFECT_STATE_DROID_CONFUSED) &&
        (pCreature->m_bInParty == 0 || pCreature->GetCurrentHitPoints(0) >= 1) &&
        pCreature->m_nStancePosture != STANCE_POSTURE_MEDITATIVE && pCreature->m_bLightsaberThrowActive == 0) {
        CSWCCreature* pClient = pCreature->GetClientCreature();
        Vector vTarget = pClient->m_orientation;
        Vector vSelf = m_orientation;
        float fFacing = vTarget.x * vSelf.x + vTarget.y * vSelf.y + vTarget.z * vSelf.z;
        if (fFacing <= 0.0f) {
            if (bHasDeflect != 0 && bHasRedirect == 0)
                nOut2 = 0;
            return 1;
        }
    }
    return 0;
}
#else
// K1: a lightsaber deflect needs a deflect feat, an idle non-dying defender, a client creature,
// a lightsaber and the attacker in front; otherwise only the Force shield stops the bolt.
int CSWSCreature::IsProjectileParryable(CSWSObject* pObject, int a, int b, int& nOut1, int c)
{
    CSWSCombatAttackData* pAttack = m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
    if (pObject->AsSWSCreature() == 0)
        return 0;
    if (pAttack->m_bRangedAttack == 0)
        return 0;
    CSWSCreature* pCreature = pObject->AsSWSCreature();
    if ((pCreature->m_pStats->HasFeat(FEAT_JEDI_DEFENSE) != 0 || pCreature->m_pStats->HasFeat(FEAT_ADVANCED_JEDI_DEFENSE) != 0 ||
         pCreature->m_pStats->HasFeat(FEAT_MASTER_JEDI_DEFENSE) != 0) &&
        pCreature->m_nState == EFFECT_STATE_NONE && pCreature->GetDeadTemp() == 0) {
        if (pCreature->GetClientCreature() == 0 || pCreature->IsLightsaberEquipped() == 0 ||
            pCreature->m_bLightsaberThrowActive != 0)
            return 0;
        CSWCCreature* pClient = pCreature->GetClientCreature();
        Vector vTarget = pClient->m_orientation;
        Vector vSelf = m_orientation;
        float fFacing = vTarget.x * vSelf.x + vTarget.y * vSelf.y + vTarget.z * vSelf.z;
        return fFacing <= 0.0f;
    }
    if (a == 0 || b == 0)
        return 0;
    if (pCreature->GetClientCreature() == 0 ||
        pCreature->GetClientCreature()->GetForceShield() == 0)
        return 0;
    int bParryable;
    unsigned int nSlot =
            (c == 0) ? CSWInventory::SLOT_RIGHTWEAPON : ((c == 1) ? CSWInventory::SLOT_LEFTWEAPON : 0);
    CSWSItem* pItem = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(nSlot));
    if (pItem != 0) {
        unsigned char nAmmunitionType = pItem->GetBaseItem()->m_nAmmunitionType;
        int nShieldHit = 0;
        g_pRules->m_p2DArrays->m_pAmmunitionTypes->GetINTEntry(
            nAmmunitionType, CExoString("ShieldHit"), &nShieldHit);
        bParryable = (nShieldHit != 0);
    } else {
        bParryable = 0;
    }
    nOut1 = pCreature->GetClientCreature()->GetForceShield();
    return bParryable;
}
#endif // !KOTOR1

static const int g_anFlatFootedAIState[NUM_FLAT_FOOTED_AI_STATES] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

int CSWSCreature::GetFlatFooted()
{
    signed char nAIState = m_nAIStateAction;
#if KOTOR1  // K1: flat-footed for combat state 4 only.
    if (m_nState == EFFECT_STATE_STUNNED)
        return 1;
#else
    switch (m_nState) {
    case EFFECT_STATE_STUNNED:
    case EFFECT_STATE_MIND_TRICK:
    case EFFECT_STATE_DROID_SCRAMBLE:
        return 1;
    }
#endif
    if ((m_nAIState & 6) != 6 || GetBlind())
        return 1;
    if (static_cast<unsigned char>(nAIState) < NUM_FLAT_FOOTED_AI_STATES)
        return g_anFlatFootedAIState[nAIState];
    return 0;
}

#if !KOTOR1  // K2 only
int CSWSCreature::IsDebilitated(int a)
{
    int bDebilitated = 0;
    if ((m_nState != EFFECT_STATE_NONE && m_nState != EFFECT_STATE_CONFUSED &&
         m_nState != EFFECT_STATE_DROID_CONFUSED) || GetDeadTemp() != 0)
        bDebilitated = 1;
    if (a != 0 && m_nStancePosture == STANCE_POSTURE_MEDITATIVE)
        bDebilitated = 1;
    return bDebilitated;
}
#endif // !KOTOR1

#if KOTOR1
int CSWSCreature::IsDebilitated()
{
    return m_nState != EFFECT_STATE_NONE || GetDeadTemp() != 0;
}
#endif

int CSWSCreature::CalculateProjectileTimeToTarget(const Vector& vTarget, int a)
{
    Vector diff = { vTarget.x - m_position.x,
                    vTarget.y - m_position.y,
                    vTarget.z - m_position.z };
    float dist = magnitude(diff);
    return static_cast<unsigned int>(dist / (a ? 37.0f : 42.0f) * 1000.0f);
}

int CSWSCreature::GetWeaponDischargeTime(int nWeaponType, int nColumn)
{
    CExoString sRow = GetWeaponDischargeRow(nWeaponType);
    int nTime = 0;
    g_pRules->m_p2DArrays->m_pWeaponDischarge->GetINTEntry(sRow, nColumn + 4, &nTime);
    return nTime;
}

int CSWSCreature::GetWeaponImpactTime(int nWeaponType, int nColumn)
{
    CExoString sRow(nWeaponType);
    int nTime = 0;
    g_pRules->m_p2DArrays->m_pCombatAnimations->GetINTEntry(sRow, nColumn + 1, &nTime);
    return nTime;
}

int CSWSCreature::GetFlanked(CSWSCreature* pCreature)
{
    if (pCreature == 0)
        return 0;
    if (pCreature->m_oidAttackTarget == m_objectId)
        return 0;
    if (pCreature->m_attemptedAttackTargetId == m_objectId)
        return 0;
    if (pCreature->m_oidAttemptedSpellTarget == m_objectId)
        return 0;
    if (pCreature->m_oidAttemptedSpellTarget == m_cachedAreaId)
        return 0;
    if (pCreature->m_oidAttemptedSpellTarget == pCreature->m_objectId)
        return 0;
    if (pCreature->m_oidSpellTarget == pCreature->m_objectId)
        return 0;
    if (pCreature->m_oidSpellTarget == m_objectId)
        return 0;
    if (pCreature->m_oidSpellTarget == m_cachedAreaId)
        return 0;
    int bFlanked = (pCreature->m_oidAttackTarget != OBJECT_INVALID) |
                   (pCreature->m_attemptedAttackTargetId != OBJECT_INVALID) |
                   (pCreature->m_oidAttemptedSpellTarget != OBJECT_INVALID) |
                   (pCreature->m_oidSpellTarget != OBJECT_INVALID);
    return bFlanked;
}

// Spawns an instant visual effect at the miss location and queues it through the AI master after
// nDelay milliseconds.
void CSWSCreature::ResolveProjectileMissImpact(const Vector& vLocation, int nDelay)
{
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
    pEffect->m_durationTypeBits =
        CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(m_objectId);
    pEffect->SetInteger(0, VFX_COM_BLASTER_IMPACT_GROUND);
    pEffect->SetFloat(0, vLocation.x);
    pEffect->SetFloat(1, vLocation.y);
    pEffect->SetFloat(2, vLocation.z);
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
        0, nDelay, m_objectId, static_cast<CSWSArea*>(GetArea())->m_objectId,
        CServerAIMaster::EVENT_APPLY_EFFECT, pEffect);
}

int CSWSCreature::HasTargetBeenDetected(unsigned long long targetId, int bOnlyUndetected)
{
    CGameObject* pTarget = g_pAppManager->m_pServerExoApp->GetGameObject(targetId);
    if (pTarget != 0 && (pTarget->AsSWSDoor() != 0 || pTarget->AsSWSPlaceable() != 0))
        return 1;
    if (m_bInParty != 0)
        return 1;
    CSWVisibilityNode* pNode = GetVisibleListElement(targetId);
    if (pNode == 0)
        return 0;
    return ((bOnlyUndetected == 0) | ((pNode->m_flags & VISIBILITY_PACIFY_STATE_MASK) != VISIBILITY_PACIFIED))
           & ((pNode->m_flags & (VISIBILITY_INVISIBLE | VISIBILITY_SEEN)) == VISIBILITY_SEEN);
}

int CSWSCreature::GetWeaponDischargeShots(int nWeaponType)
{
    CExoString sRow = GetWeaponDischargeRow(nWeaponType);
    int nShots = 0;
    g_pRules->m_p2DArrays->m_pWeaponDischarge->GetINTEntry(sRow, CExoString("Shots"), &nShots);
    return nShots;
}

// The "SwitchMask" column of weapondischarge.2da; the hand index selects one character of the
// mask, parsed as an integer (null or empty gives 0).
int CSWSCreature::GetWeaponDischargeHand(int nWeaponType, int nHand)
{
    CExoString sRow = GetWeaponDischargeRow(nWeaponType);
    CExoString sMask;
    g_pRules->m_p2DArrays->m_pWeaponDischarge->GetCExoStringEntry(
        sRow, CExoString("SwitchMask"), &sMask);
    CExoString sSub = sMask.SubString(nHand, 1);
    int nResult = sSub.AsINT();
    return nResult;
}

void CSWSCreature::ResolveRangedMiss(CSWSObject* pObject)
{
    if (pObject != 0) {
        CSWSCombatAttackData* pAttack = m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
        Vector vMissLocation = ResolveProjectileMissLocation(pObject);
        pAttack->m_vMissLocation = vMissLocation;
        Vector vDelta = vMissLocation - m_position;
        // Cast through a 32-bit unsigned first: narrowing straight to the 16-bit field allows a
        // signed conversion, since only the low 16 bits survive the store.
        pAttack->m_nReaxnDelay = static_cast<unsigned short>(static_cast<unsigned int>(magnitude(vDelta) / 42.0f * 1000.0f));
    }
}

int CSWSCreature::GetWeaponImpactHits(int nWeaponType)
{
    CExoString sRow(nWeaponType);
    int nHits = 0;
    g_pRules->m_p2DArrays->m_pCombatAnimations->GetINTEntry(sRow, CExoString("Hits"), &nHits);
    return nHits;
}

// The "Hits" column of weapondischarge.2da, addressed through GetWeaponDischargeRow.
int CSWSCreature::GetWeaponHitShots(int nWeaponType)
{
    CExoString sRow = GetWeaponDischargeRow(nWeaponType);
    int nHits = 0;
    g_pRules->m_p2DArrays->m_pWeaponDischarge->GetINTEntry(sRow, CExoString("Hits"), &nHits);
    return nHits;
}

// Not a cast-style test. It reads this creature's appearance.2da model-type column and matches its
// first letter against 'L', 'S', 'l' or 's'.
int CSWSCreature::IsCreature()
{
    CExoString sModelType;
    CExoString sColumn("MODELTYPE");
    g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(m_appearance.m_headId, sColumn, &sModelType);
    int bIsCreature = 0;
    char cModelType = sModelType[0];
    if (cModelType == 'L' || cModelType == 'S' || cModelType == 'l' || cModelType == 's') {
        bIsCreature = 1;
    }
    return bIsCreature;
}

// Wraps a value effect in an EFFECT_LINK created by this creature, links the pair and returns
// the link.
CGameEffect* CSWSCreature::CreateLinkedEffect(CGameEffect* pEffect, int nSubType, int nValue)
{
    CGameEffect* pValueEffect = new CGameEffect(1);
    CGameEffect* pLinkEffect = new CGameEffect(1);
    pValueEffect->m_subType = static_cast<unsigned short>(nSubType);
    pValueEffect->SetInteger(0, nValue);
    pLinkEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_LINK);
    pLinkEffect->SetCreator(m_objectId);
    pLinkEffect->SetLinked(pEffect, pValueEffect);
    return pLinkEffect;
}

// The two-value-effect overload builds one link around two value effects.
CGameEffect* CSWSCreature::CreateLinkedEffect(int nSubType1, int nValue1, int nSubType2, int nValue2)
{
    CGameEffect* pEffect1 = new CGameEffect(1);
    CGameEffect* pEffect2 = new CGameEffect(1);
    CGameEffect* pLinkEffect = new CGameEffect(1);
    pEffect1->m_subType = static_cast<unsigned short>(nSubType1);
    pEffect1->SetInteger(0, nValue1);
    pEffect2->m_subType = static_cast<unsigned short>(nSubType2);
    pEffect2->SetInteger(0, nValue2);
    pLinkEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_LINK);
    pLinkEffect->SetCreator(m_objectId);
    pLinkEffect->SetLinked(pEffect1, pEffect2);
    return pLinkEffect;
}

void CSWSCreature::ResolveSafeProjectile(int nAttack, int nLevel, int bUseAttackPathCount, int nProjectileType)
{
    unsigned int nSlot = CSWInventory::SLOT_RIGHTWEAPON;
    if (nProjectileType != 0)
        nSlot = (nProjectileType == 1) << 5;
    CSWSItem* pItem = static_cast<CSWSItem*>(m_pInventory->GetItemInSlot(nSlot));
    if (pItem != 0) {
        CSWSCombatAttackData* pAttackData = m_pCombatRound->GetAttack(nAttack);
        pAttackData->m_oidAmmoItem = pItem->m_objectId;
        unsigned char nPathCount = 4;
        if (bUseAttackPathCount != 0)
            nPathCount = pAttackData->m_nAttackResult;  // also read as a critical-hit state
        pAttackData->GetBaseDamage();  // result unused
        CSWBaseItem* pBaseItem = pItem->GetBaseItem();
        unsigned char nBaseItemByte = pBaseItem->m_nAmmunitionType;
        unsigned char nIsSpecialFeat = 0;
        if (pAttackData->m_nFeatId == FEAT_IMPROVED_POWER_BLAST
            || pAttackData->m_nFeatId == FEAT_MASTER_POWER_BLAST ||
            pAttackData->m_nFeatId == FEAT_POWER_BLAST)
            nIsSpecialFeat = 1;
        BroadcastSafeProjectile(m_objectId, pAttackData->m_targetObjectId,
            m_position, pAttackData->m_vMissLocation, nLevel, nPathCount, nProjectileType,
            nBaseItemByte, nIsSpecialFeat, pAttackData->m_nMissedBy);
    }
}

void CSWSCreature::ResolveRangedHit(CSWSObject* pObject)
{
    if (pObject != 0) {
        CSWSCombatAttackData* pAttack = m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
        unsigned char nResult = pAttack->m_nAttackResult;
        if (static_cast<unsigned char>(nResult - 1) >= 3) {
            if (static_cast<unsigned char>(nResult - 8) < 3)
                pAttack->m_nReaxnAnimation = SERVER_ANIMATION_PARRY;
        } else {
            pAttack->m_nReaxnAnimation = SERVER_ANIMATION_DAMAGE;
        }
        pAttack->m_vMissLocation = pObject->m_position;
        Vector vDelta = pObject->m_position - m_position;
        // Cast through a 32-bit unsigned first: narrowing straight to the 16-bit field allows a
        // signed conversion, since only the low 16 bits survive the store.
        pAttack->m_nReaxnDelay = static_cast<unsigned short>(static_cast<unsigned int>(magnitude(vDelta) / 42.0f * 1000.0f));
    }
}

void CSWSCreature::ApplyOnHitInstantDeath(CSWSObject* pObject, CSWItemProperty* pProperty)
{
    K2_UNUSED(pObject); K2_UNUSED(pProperty);
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DEATH);
    pEffect->m_durationTypeBits =
        CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pEffect->SetCreator(m_objectId);
    pEffect->SetInteger(0, 0);
    pEffect->SetInteger(1, 1);
    m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_effects.Add(pEffect);
}

// Applies the slow-on-hit property, an instantaneous hit flash, then a temporary slow effect
// carrying a second visual effect and an effect icon, for nRounds * 3 seconds.
void CSWSCreature::ApplyOnHitSlow(CSWSObject* pObject, int nRounds)
{
    K2_UNUSED(pObject);
    CGameEffect* pVfx = new CGameEffect(1);
    pVfx->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
    pVfx->SetInteger(0, VFX_ONHIT_SLOW);
    pVfx->SetCreator(m_objectId);
    pVfx->m_durationTypeBits =
        CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pVfx->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_effects.Add(pVfx);

    CGameEffect* pLink = CreateLinkedEffect(CSWSEffectListHandler::EFFECT_SLOW, 0,
                                            CSWSEffectListHandler::EFFECT_VISUALEFFECT, VFX_ONHIT_DISABLED_LINK);
    CGameEffect* pEffect = CreateLinkedEffect(pLink, CSWSEffectListHandler::EFFECT_EFFECT_ICON,
                                              EFFECT_ICON_FORCE_POWER_SLOW);
#if KOTOR1  // K1: the duration is 6 s per round.
    pEffect->m_fDuration = static_cast<float>(nRounds) * 6.0f;
#else
    pEffect->m_fDuration = static_cast<float>(nRounds) * 3.0f;
#endif
    pEffect->m_durationTypeBits =
        (CGameEffect::DURATION_TYPE_TEMPORARY & (0xffff & ~SUBTYPE_SUPERNATURAL)) | (pEffect->m_durationTypeBits & CGameEffect::DURATION_SUBTYPE_CLEAR_MASK)
        | SUBTYPE_MAGICAL;
    pEffect->UpdateLinked();
    m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_effects.Add(pEffect);
}

// Sleep's chain has one more stage than slow's: the set-state payload carries a visual effect,
// then a second one, then the icon.
void CSWSCreature::ApplyOnHitSleep(CSWSObject* pObject, int nRounds)
{
    K2_UNUSED(pObject);
    CGameEffect* pVfx = new CGameEffect(1);
    pVfx->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
    pVfx->SetInteger(0, VFX_ONHIT_SLEEP);
    pVfx->SetCreator(m_objectId);
    pVfx->m_durationTypeBits =
        CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pVfx->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_effects.Add(pVfx);

    CGameEffect* pLink1 = CreateLinkedEffect(CSWSEffectListHandler::EFFECT_SETSTATE, 6,
                                             CSWSEffectListHandler::EFFECT_VISUALEFFECT, 7);
    CGameEffect* pLink2 = CreateLinkedEffect(pLink1, CSWSEffectListHandler::EFFECT_VISUALEFFECT,
                                             VFX_ONHIT_DISABLED_LINK);
    CGameEffect* pEffect = CreateLinkedEffect(pLink2, CSWSEffectListHandler::EFFECT_EFFECT_ICON,
                                              EFFECT_ICON_FORCE_POWER_MASS_STASIS);
#if KOTOR1  // K1: the duration is 6 s per round.
    pEffect->m_fDuration = static_cast<float>(nRounds) * 6.0f;
#else
    pEffect->m_fDuration = static_cast<float>(nRounds) * 3.0f;
#endif
    pEffect->m_durationTypeBits =
        (CGameEffect::DURATION_TYPE_TEMPORARY & (0xffff & ~SUBTYPE_SUPERNATURAL)) | (pEffect->m_durationTypeBits & CGameEffect::DURATION_SUBTYPE_CLEAR_MASK)
        | SUBTYPE_MAGICAL;
    pEffect->UpdateLinked();
    m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_effects.Add(pEffect);
}

// Applies the "slay racial group" property. A racialtypes match on the property's parameter ships
// an instantaneous hit flash and an instantaneous death effect.
void CSWSCreature::ApplyOnHitSlayRacialGroup(CSWSObject* pObject, CSWItemProperty* pProperty)
{
    CSWSCreature* pTarget = pObject->AsSWSCreature();
    if (pTarget != 0 && pTarget->m_pStats->m_nRace == pProperty->m_nParam1Value) {
        CGameEffect* pVfx = new CGameEffect(1);
        pVfx->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
        pVfx->SetInteger(0, VFX_ONHIT_SLAY_RACIAL_GROUP);
        pVfx->SetCreator(m_objectId);
        pVfx->m_durationTypeBits =
            CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pVfx->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_effects.Add(pVfx);

        CGameEffect* pEffect = new CGameEffect(1);
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DEATH);
        pEffect->SetCreator(m_objectId);
        pEffect->m_durationTypeBits =
            CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pEffect->SetInteger(0, 0);
        pEffect->SetInteger(1, 1);
        m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_effects.Add(pEffect);
    }
}

void CSWSCreature::ApplyOnHitStun(CSWSObject* pObject, int nRounds)
{
    K2_UNUSED(pObject);
    CGameEffect* pLink1 = CreateLinkedEffect(CSWSEffectListHandler::EFFECT_SETSTATE, 4,
                                             CSWSEffectListHandler::EFFECT_VISUALEFFECT, VFX_ONHIT_STUN_CONFUSION);
    if (pLink1 == nullptr)
        return;
    CGameEffect* pLink2 = CreateLinkedEffect(pLink1, CSWSEffectListHandler::EFFECT_VISUALEFFECT,
                                             VFX_ONHIT_DISABLED_LINK);
    if (pLink2 == nullptr) {
        delete pLink1;
        return;
    }
    CGameEffect* pEffect = CreateLinkedEffect(pLink2, CSWSEffectListHandler::EFFECT_EFFECT_ICON,
                                              EFFECT_ICON_FORCE_POWER_STUN);
    if (pEffect == nullptr) {
        delete pLink2;
        return;
    }
#if KOTOR1  // K1: the duration is 6 s per round.
    pEffect->m_fDuration = static_cast<float>(nRounds) * 6.0f;
#else
    pEffect->m_fDuration = static_cast<float>(nRounds) * 3.0f;
#endif
    pEffect->m_durationTypeBits =
        (CGameEffect::DURATION_TYPE_TEMPORARY & (0xffff & ~SUBTYPE_SUPERNATURAL)) | (pEffect->m_durationTypeBits & CGameEffect::DURATION_SUBTYPE_CLEAR_MASK)
        | SUBTYPE_MAGICAL;
    pEffect->UpdateLinked();
    m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_effects.Add(pEffect);
}

// Confusion sets state 1.
void CSWSCreature::ApplyOnHitConfusion(CSWSObject* pObject, int nRounds)
{
    K2_UNUSED(pObject);
    CGameEffect* pLink1 = CreateLinkedEffect(CSWSEffectListHandler::EFFECT_SETSTATE, 1,
                                             CSWSEffectListHandler::EFFECT_VISUALEFFECT, VFX_ONHIT_STUN_CONFUSION);
    if (pLink1 == nullptr)
        return;
    CGameEffect* pLink2 = CreateLinkedEffect(pLink1, CSWSEffectListHandler::EFFECT_VISUALEFFECT,
                                             VFX_ONHIT_DISABLED_LINK);
    if (pLink2 == nullptr) {
        delete pLink1;
        return;
    }
    CGameEffect* pEffect = CreateLinkedEffect(pLink2, CSWSEffectListHandler::EFFECT_EFFECT_ICON,
                                              EFFECT_ICON_FORCE_POWER_INSANITY);
    if (pEffect == nullptr) {
        delete pLink2;
        return;
    }
#if KOTOR1  // K1: the duration is 6 s per round.
    pEffect->m_fDuration = static_cast<float>(nRounds) * 6.0f;
#else
    pEffect->m_fDuration = static_cast<float>(nRounds) * 3.0f;
#endif
    pEffect->m_durationTypeBits =
        (CGameEffect::DURATION_TYPE_TEMPORARY & (0xffff & ~SUBTYPE_SUPERNATURAL)) | (pEffect->m_durationTypeBits & CGameEffect::DURATION_SUBTYPE_CLEAR_MASK)
        | SUBTYPE_MAGICAL;
    pEffect->UpdateLinked();
    m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_effects.Add(pEffect);
}

// Fear sets state 2, with a hit-flash visual effect.
void CSWSCreature::ApplyOnHitFear(CSWSObject* pObject, int nRounds)
{
    K2_UNUSED(pObject);
    CGameEffect* pLink1 = CreateLinkedEffect(CSWSEffectListHandler::EFFECT_SETSTATE, 2,
                                             CSWSEffectListHandler::EFFECT_VISUALEFFECT, VFX_ONHIT_FEAR);
    if (pLink1 == nullptr)
        return;
    CGameEffect* pLink2 = CreateLinkedEffect(pLink1, CSWSEffectListHandler::EFFECT_VISUALEFFECT,
                                             VFX_ONHIT_DISABLED_LINK);
    if (pLink2 == nullptr) {
        delete pLink1;
        return;
    }
    CGameEffect* pEffect = CreateLinkedEffect(pLink2, CSWSEffectListHandler::EFFECT_EFFECT_ICON,
                                              6);
    if (pEffect == nullptr) {
        delete pLink2;
        return;
    }
#if KOTOR1  // K1: the duration is 6 s per round.
    pEffect->m_fDuration = static_cast<float>(nRounds) * 6.0f;
#else
    pEffect->m_fDuration = static_cast<float>(nRounds) * 3.0f;
#endif
    pEffect->m_durationTypeBits =
        (CGameEffect::DURATION_TYPE_TEMPORARY & (0xffff & ~SUBTYPE_SUPERNATURAL)) | (pEffect->m_durationTypeBits & CGameEffect::DURATION_SUBTYPE_CLEAR_MASK)
        | SUBTYPE_MAGICAL;
    pEffect->UpdateLinked();
    m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_effects.Add(pEffect);
}

// Hold has one extra link stage, sets state 5 and adds an icon sub-effect.
void CSWSCreature::ApplyOnHitHold(CSWSObject* pObject, int nRounds)
{
    K2_UNUSED(pObject);
    CGameEffect* pLink1 = CreateLinkedEffect(CSWSEffectListHandler::EFFECT_SETSTATE, 5,
                                             CSWSEffectListHandler::EFFECT_VISUALEFFECT, VFX_ONHIT_HOLD);
    if (pLink1 == nullptr)
        return;
    CGameEffect* pLink2 = CreateLinkedEffect(pLink1, CSWSEffectListHandler::EFFECT_VISUALEFFECT,
                                             VFX_ONHIT_HOLD_LINK);
    if (pLink2 == nullptr) {
        delete pLink1;
        return;
    }
    CGameEffect* pLink3 = CreateLinkedEffect(pLink2, CSWSEffectListHandler::EFFECT_VISUALEFFECT,
                                             VFX_ONHIT_DISABLED_LINK);
    if (pLink3 == nullptr) {
        delete pLink2;
        return;
    }
    CGameEffect* pEffect = CreateLinkedEffect(pLink3, CSWSEffectListHandler::EFFECT_EFFECT_ICON,
                                              EFFECT_ICON_FORCE_POWER_STASIS);
    if (pEffect == nullptr) {
        delete pLink3;
        return;
    }
#if KOTOR1  // K1: the duration is 6 s per round.
    pEffect->m_fDuration = static_cast<float>(nRounds) * 6.0f;
#else
    pEffect->m_fDuration = static_cast<float>(nRounds) * 3.0f;
#endif
    pEffect->m_durationTypeBits =
        (CGameEffect::DURATION_TYPE_TEMPORARY & (0xffff & ~SUBTYPE_SUPERNATURAL)) | (pEffect->m_durationTypeBits & CGameEffect::DURATION_SUBTYPE_CLEAR_MASK)
        | SUBTYPE_MAGICAL;
    pEffect->UpdateLinked();
    m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack)->m_effects.Add(pEffect);
}

void CSWSCreature::SetAttackSummaryFeedbackEffectData(int nAttack, unsigned long long oidTarget)
{
    K2_UNUSED(nAttack);
    CSWSCreature* pTarget =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
    if (pTarget == 0)
        return;
    CSWSCombatAttackData* pAttack =
        m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
    switch (pAttack->m_effect.m_nFeedbackEffectType) {
    case EFFECT_STATE_CONFUSED:
        if (pTarget->m_pStats->GetEffectImmunity(
                static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_CONFUSED), this) != 0)
            pAttack->m_effect.m_bFeedbackEffectData = 1;
        break;
    case EFFECT_STATE_FRIGHTENED:
        if (pTarget->m_pStats->GetEffectImmunity(
                static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_FEAR), this) != 0 ||
            pTarget->m_pStats->HasFeat(CSWSCreatureStats::FEAT_FORCE_IMMUNITY_FEAR) != 0 ||
            pTarget->m_pStats->HasFeat(CSWSCreatureStats::FEAT_FORCE_IMMUNITY_STUN) != 0 ||
#if KOTOR1  // K1: no extra feat test and no cases 8, 14, 16 or 18.
            pTarget->m_pStats->HasFeat(CSWSCreatureStats::FEAT_FORCE_IMMUNITY_PARALYSIS) != 0)
#else
            pTarget->m_pStats->HasFeat(CSWSCreatureStats::FEAT_FORCE_IMMUNITY_PARALYSIS) != 0 ||
            pTarget->m_pStats->HasFeat(FEAT_MANDALORIAN_COURAGE) != 0)
#endif
            pAttack->m_effect.m_bFeedbackEffectData = 1;
        break;
    case EFFECT_STATE_STUNNED:
        if (pTarget->m_pStats->GetEffectImmunity(
                static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_STUN), this) != 0 ||
            pTarget->m_pStats->HasFeat(CSWSCreatureStats::FEAT_FORCE_IMMUNITY_STUN) != 0 ||
            pTarget->m_pStats->HasFeat(CSWSCreatureStats::FEAT_FORCE_IMMUNITY_PARALYSIS) != 0)
            pAttack->m_effect.m_bFeedbackEffectData = 1;
        break;
    case EFFECT_STATE_PARALYZED:
        if (pTarget->m_pStats->GetEffectImmunity(
                static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_PARALYSIS), this) != 0 ||
            pTarget->m_pStats->HasFeat(CSWSCreatureStats::FEAT_FORCE_IMMUNITY_PARALYSIS) != 0)
            pAttack->m_effect.m_bFeedbackEffectData = 1;
        break;
    case EFFECT_STATE_SLEEP:
        if (pTarget->m_pStats->GetEffectImmunity(
                static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_SLEEP), this) != 0)
            pAttack->m_effect.m_bFeedbackEffectData = 1;
        break;
#if !KOTOR1
    case EFFECT_STATE_HORRIFIED:
        if (pTarget->m_pStats->HasFeat(FEAT_MANDALORIAN_COURAGE) != 0)
            pAttack->m_effect.m_bFeedbackEffectData = 1;
        break;
    case EFFECT_STATE_KNOCKDOWN:
        if (pTarget->m_pStats->GetEffectImmunity(
                static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_KNOCKDOWN), this) != 0)
            pAttack->m_effect.m_bFeedbackEffectData = 1;
        break;
    case EFFECT_STATE_DROID_CONFUSED:
        if (pTarget->m_pStats->GetEffectImmunity(
                static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_DROID_CONFUSED), this) != 0)
            pAttack->m_effect.m_bFeedbackEffectData = 1;
        break;
    case EFFECT_STATE_MIND_TRICK:
        if (pTarget->m_pStats->GetEffectImmunity(
                static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_MIND_AFFECTING), this) != 0)
            pAttack->m_effect.m_bFeedbackEffectData = 1;
        break;
#endif
    }
}

int CSWSCreature::ResolveMoveToForceJump(CSWSObject* pObject, unsigned short& nFeatId)
{
    if (pObject == 0)
        return 0;
    if (pObject->AsSWSCreature() == 0)
        return 0;
    if (nFeatId != 0)
        return 0;
    if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_FORCE_JUMP) == 0)
        return 0;
    if (IsLightsaberEquipped() == 0)
        return 0;
    float fDx = m_position.x - pObject->m_position.x;
    float fDy = m_position.y - pObject->m_position.y;
    float fDz = m_position.z - pObject->m_position.z;
    if (!(fDx * fDx + fDy * fDy + fDz * fDz >= 100.0f))
        return 0;
    CPathfindInformation* pPathfind = m_pPathfindInfo;
    pPathfind->m_oidGoalObject = pObject->m_objectId;
    if (g_pAppManager->m_pServerExoApp->TestDirectLine(m_objectId, CResRef(""), m_position,
                                                       pObject->m_position,
                                                       static_cast<float>(m_pPathfindInfo->m_fPersonalSpace),
                                                       m_pPathfindInfo->m_fHeightAlias, 0, 0) != 1)
        return 0;
    CGameEffect* pEffect = new CGameEffect(1);
    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FORCEJUMP);
    pEffect->SetObjectID(0, pObject->m_objectId);
    pEffect->SetInteger(0, 0);
    ApplyEffect(pEffect, 0, 0);
    if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_FORCE_JUMP_MASTERY) != 0) {
        nFeatId = CSWSCreatureStats::FEAT_FORCE_JUMP_MASTERY;
    } else if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_FORCE_JUMP_ADVANCED) != 0) {
        nFeatId = CSWSCreatureStats::FEAT_FORCE_JUMP_ADVANCED;
    } else {
        nFeatId = CSWSCreatureStats::FEAT_FORCE_JUMP;
    }
    return 1;
}

void CSWSCreature::ResolveRangedAnimations(CSWSObject* pTarget, int nTime, int a3)
{
    if (pTarget == 0)
        return;
    unsigned char nAttack = m_pCombatRound->m_currentAttack;
    CSWSCombatAttackData* pAttack = m_pCombatRound->GetAttack(nAttack);
    pAttack->m_nAnimationLength = static_cast<unsigned short>(nTime);
    pAttack->m_nReaxnDelay = 0;
    m_pCombatRound->DecrementRoundLength(nTime, 0);
    CSWSCreature* pTargetCreature = pTarget->AsSWSCreature();
    if (pTargetCreature == 0)
        return;
    CSWSCombatRound* pCombatRound = m_pCombatRound;
    int nCanCounter;
#if KOTOR1  // K1: only an idle (state 0), non-dying target reacts.
    if (pTargetCreature->m_nState == EFFECT_STATE_NONE && pTargetCreature->GetDeadTemp() == 0) {
        nCanCounter = 0;
    } else {
        nCanCounter = 1;
    }
#else
    if ((pTargetCreature->m_nState == EFFECT_STATE_NONE ||
         pTargetCreature->m_nState == EFFECT_STATE_CONFUSED ||
         pTargetCreature->m_nState == EFFECT_STATE_DROID_CONFUSED) &&
        (pTargetCreature->m_bInParty == 0 || pTargetCreature->GetCurrentHitPoints(0) > 0)) {
        nCanCounter = 0;
    } else {
        nCanCounter = 1;
    }
    if (pTargetCreature->m_nStancePosture == STANCE_POSTURE_MEDITATIVE) {
        nCanCounter = 1;
    }
#endif
    pCombatRound->m_bTargetCannotReact = nCanCounter;
    if (a3 == 0 && pTargetCreature->IsLightsaberEquipped() == 0 &&
        pTargetCreature->m_pCombatRound->m_bEngaged != 0) {
        pAttack->m_nReaxnAnimation = SERVER_ANIMATION_DODGE;
    } else {
        pAttack->m_nReaxnAnimation = SERVER_ANIMATION_READY;
    }
#if KOTOR1
    if (pTargetCreature->m_nState == EFFECT_STATE_NONE && pTargetCreature->GetDeadTemp() == 0) {
#else
    if ((pTargetCreature->m_nState == EFFECT_STATE_NONE ||
         pTargetCreature->m_nState == EFFECT_STATE_CONFUSED ||
         pTargetCreature->m_nState == EFFECT_STATE_DROID_CONFUSED) &&
        (pTargetCreature->m_bInParty == 0 || pTargetCreature->GetCurrentHitPoints(0) >= 1) &&
        pTargetCreature->m_nStancePosture != STANCE_POSTURE_MEDITATIVE) {
#endif
        if (pTarget->AsSWSCreature()->m_pCombatRound->CheckActionLengthAtTime(
                m_objectId, nTime, 0) == 0)
            return;
        if (m_pCombatRound->m_bEngaged == 0)
            return;
        m_pCombatRound->GetAttack(nAttack)->m_nReaxnAnimLength = static_cast<short>(nTime);
        m_pCombatRound->GetAttack(nAttack)->m_nReaxnDelay = 0;
        pTargetCreature->SetAnimation(pAttack->m_nReaxnAnimation);
        pTargetCreature->m_pCombatRound->DecrementRoundLength(nTime, 0);
        return;
    }
    pAttack->m_nReaxnAnimation = SERVER_ANIMATION_READY;
}

// Signals everything the server reports about one resolved attack: the human player's controller
// rumble, the five per-subsystem breakdown messages the combat-debug UI renders, and the melee or
// ranged damage signal.
void CSWSCreature::SignalDamage(int bRanged, unsigned long long oidTarget)
{
    CSWSCombatAttackData* pAttack = m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);

    // Rumble the controller, but only when the thing being hit is the human player's own creature
    // and only on an attack result of 2. A ranged attack schedules it at the projectile's flight
    // time so the rumble lands with the impact; a melee one fires immediately.
    if (g_pAppManager->m_pServerExoApp->GetPlayerCreatureId() == oidTarget
        && pAttack->m_nAttackResult == 2)
    {
        CServerAIMaster* pAIMaster = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
        if (bRanged != 0)
            pAIMaster->AddEventDeltaTime(0, pAttack->m_nReaxnDelay, m_objectId, oidTarget,
                                         CServerAIMaster::EVENT_CONTROLLER_RUMBLE, reinterpret_cast<void*>(20));
        else
            pAIMaster->AddEventDeltaTime(0, 0, m_objectId, oidTarget,
                                         CServerAIMaster::EVENT_CONTROLLER_RUMBLE, reinterpret_cast<void*>(20));
    }

    // A ranged attack whose projectile has not resolved yet emits no feedback at all: the
    // breakdowns are sent once, when the shot lands.
    if (pAttack->m_bRangedAttack == 0 || pAttack->m_bAttackRolled != 0)
    {
        SendAttackSummaryFeedback(oidTarget);

        // (1) the attack roll: every to-hit term, one per index.
        CSWCCMessageData* pData = new CSWCCMessageData();
        pData->SetInteger(0, pAttack->m_attack.m_nAttackRoll + pAttack->m_attack.m_nAttackModifier);
        pData->SetInteger(1, pAttack->m_attack.m_nAttackRoll);
        pData->SetInteger(2, pAttack->m_attack.m_nAttackModifier);
        pData->SetInteger(3, pAttack->m_attack.m_nStrAttackMod);
        pData->SetInteger(4, pAttack->m_attack.m_nDexAttackMod);
        pData->SetInteger(5, pAttack->m_nFeatId);
        pData->SetInteger(6, pAttack->m_attack.m_nSpecialAttackBonus);
        pData->SetInteger(7, pAttack->m_attack.m_bOffhandAttack);
        pData->SetInteger(8, static_cast<signed char>(pAttack->m_attack.m_nLightOffhandBonus));
        pData->SetInteger(9, static_cast<signed char>(pAttack->m_attack.m_nTwoWeaponFeatBonus));
        pData->SetInteger(10, pAttack->m_attack.m_nTwoWeaponFeat);
        pData->SetInteger(11, static_cast<signed char>(pAttack->m_attack.m_nDuelingBonus));
        pData->SetInteger(12, pAttack->m_attack.m_nDuelingFeat);
        pData->SetInteger(13, static_cast<signed char>(pAttack->m_attack.m_nGoodAimBonus));
        pData->SetInteger(14, static_cast<signed char>(pAttack->m_attack.m_nOffensiveTrainingBonus));
        pData->SetInteger(15, static_cast<signed char>(pAttack->m_attack.m_nWeaponFocusBonus));
        pData->SetInteger(16, pAttack->m_attack.m_nEffectAttackBonus);
        pData->SetInteger(17, static_cast<unsigned char>(pAttack->m_attack.m_nBaseAttackBonus));
        pData->SetInteger(18, pAttack->m_attack.m_nTwoWeaponPenalty);
        pData->SetInteger(19, pAttack->m_attack.m_bAutomaticHit);
        pData->SetInteger(20, pAttack->m_attack.m_bAutomaticMiss);
#if !KOTOR1
        pData->SetInteger(21, static_cast<signed char>(pAttack->m_attack.m_nBattleMeditationBonus));
        pData->SetInteger(22, pAttack->m_attack.m_nTargetingFeat);
        pData->SetInteger(23, static_cast<signed char>(pAttack->m_attack.m_nTargetingBonus));
        pData->SetInteger(24, pAttack->m_attack.m_nCrushOppositionSpell);
        pData->SetInteger(25, static_cast<signed char>(pAttack->m_attack.m_nCrushOppositionPenalty));
        pData->SetInteger(26, pAttack->m_attack.m_nInspireFollowersSpell);
        pData->SetInteger(27, static_cast<signed char>(pAttack->m_attack.m_nInspireFollowersBonus));
        pData->SetInteger(28, static_cast<signed char>(pAttack->m_attack.m_nPCCharismaBonus));
        pData->SetInteger(29, pAttack->m_attack.m_nSaberFormId);
        pData->SetInteger(30, static_cast<signed char>(pAttack->m_attack.m_nSaberFormBonus));
#endif
        BroadcastAttackBreakdown(oidTarget, pData);
        delete pData;

        // (2) threat.
        pData = new CSWCCMessageData();
        pData->SetInteger(0, pAttack->m_attack.m_nAttackRoll);
        pData->SetInteger(1, pAttack->m_threat.m_nThreatRange);
        pData->SetInteger(2, pAttack->m_threat.m_bCriticalThreat);
        pData->SetInteger(3, pAttack->m_threat.m_nConfirmRoll + pAttack->m_attack.m_nAttackModifier);
        pData->SetInteger(4, pAttack->m_defense.m_nArmorClass);
        pData->SetInteger(5, pAttack->m_threat.m_nReserved1);
        pData->SetInteger(6, pAttack->m_threat.m_bCriticalConfirmed);
        BroadcastThreatBreakdown(oidTarget, pData);
        delete pData;

        // (3) defense: the target's AC, term by term.
        pData = new CSWCCMessageData();
        pData->SetInteger(0, pAttack->m_defense.m_nArmorClass);
        pData->SetInteger(1, pAttack->m_defense.m_nACArmor);
        pData->SetInteger(2, pAttack->m_defense.m_nACDex);
        pData->SetInteger(3, pAttack->m_defense.m_nACDexUncapped);
        pData->SetInteger(4, pAttack->m_defense.m_nACClass);
        pData->SetInteger(5, pAttack->m_defense.m_nACNatural);
        pData->SetInteger(6, pAttack->m_defense.m_nACDodge);
        pData->SetInteger(7, pAttack->m_defense.m_nACFeatMod);
        pData->SetInteger(8, pAttack->m_defense.m_nACDebilitatedPenalty);
        BroadcastDefenseBreakdown(oidTarget, pData);
        delete pData;

        // (4) damage: the 15 per-type amounts first, then the terms that produced them.
        pData = new CSWCCMessageData();
        for (int i = 0; i < 15; i++)
            pData->SetInteger(i, pAttack->m_anDamage[i]);
        pData->SetInteger(15, pAttack->m_damage.m_nDamageRoll);
        pData->SetInteger(16, pAttack->m_damage.m_nStrDamageBonus);
        pData->SetInteger(17, pAttack->m_nFeatId);
        pData->SetInteger(18, pAttack->m_damage.m_nDamageBonus);
        pData->SetInteger(19, pAttack->m_damage.m_nReserved1);
        pData->SetInteger(20, pAttack->m_damage.m_nSneakAttackDamage);
        pData->SetInteger(21, pAttack->m_damage.m_nWeaponSpecializationBonus);
        pData->SetInteger(22, pAttack->m_threat.m_nCriticalMultiplier);
        pData->SetInteger(23, pAttack->m_damage.m_nFeatSoakAmount);
        pData->SetInteger(24, pAttack->m_damage.m_nFeatFlatDRAmount);
        pData->SetInteger(25, pAttack->m_damage.m_nDamageTypeBits);
        pData->SetInteger(26, pAttack->m_damage.m_nDamageAfterSoak);
        pData->SetInteger(27, pAttack->m_damage.m_nDamageAfterFlatDR);
#if !KOTOR1
        pData->SetInteger(28, pAttack->m_damage.m_nSpellDamageReduction);
        pData->SetInteger(29, pAttack->m_damage.m_nReductionSpellId);
        pData->SetInteger(30, pAttack->m_damage.m_nEffectDamageBonus);
#endif
        BroadcastDamageBreakdown(oidTarget, pData);
        delete pData;

        // (5) effect application -- only when the attack actually tried to apply one.
        if (pAttack->m_effect.m_bHasApplicationData != 0)
        {
            pData = new CSWCCMessageData();
            pData->SetObjectID(0, pAttack->m_targetObjectId);
            pData->SetInteger(0, pAttack->m_effect.m_nSaveType);
            pData->SetInteger(1, pAttack->m_effect.m_nFeedbackEffectType);
            pData->SetInteger(2, pAttack->m_effect.m_nSaveVsType);
            pData->SetInteger(3, pAttack->m_effect.m_nSaveRoll);
            pData->SetInteger(4, pAttack->m_effect.m_nSaveEffectBonus);
            pData->SetInteger(5, pAttack->m_effect.m_nSaveBase);
            pData->SetInteger(6, pAttack->m_effect.m_nSaveTotal);
            pData->SetInteger(7, pAttack->m_effect.m_nSaveDC);
            pData->SetInteger(8, pAttack->m_effect.m_nSaveResult);
            BroadcastEffectApplicationBreakdown(oidTarget, pData);
            delete pData;
        }

        // (6) deflection -- results 8, 9 and 10 are the deflect/reflect family.
        if (pAttack->m_nAttackResult >= 8 && pAttack->m_nAttackResult <= 10)
        {
            pData = new CSWCCMessageData();
            pData->SetObjectID(0, pAttack->m_targetObjectId);
            pData->SetInteger(0, pAttack->m_deflect.m_nDeflectResult);
            pData->SetInteger(1, pAttack->m_deflect.m_nAttackTotal);
            pData->SetInteger(2, pAttack->m_deflect.m_nDeflectTotal);
            pData->SetInteger(3, pAttack->m_deflect.m_nDeflectRoll);
            pData->SetInteger(4, pAttack->m_deflect.m_nDeflectFeatBonus);
            pData->SetInteger(5, pAttack->m_deflect.m_nDeflectFeat);
            pData->SetInteger(6, pAttack->m_deflect.m_nBaseAttackBonus);
            pData->SetInteger(7, pAttack->m_deflect.m_nArmourPenalty);
#if KOTOR1  // K1: the effect bonus is the last value sent, as an unsigned byte.
            pData->SetInteger(8, static_cast<unsigned char>(pAttack->m_deflect.m_nEffectBonus));
#else
            pData->SetInteger(8, pAttack->m_deflect.m_nEffectBonus);
            pData->SetInteger(9, pAttack->m_deflect.m_nPowerBonus);
            pData->SetInteger(10, pAttack->m_deflect.m_nPowerId);
            pData->SetInteger(11, pAttack->m_deflect.m_nStanceBonus);
            pData->SetInteger(12, pAttack->m_deflect.m_nStanceId);
            pData->SetInteger(13, pAttack->m_deflect.m_nClassFeatBonus);
            pData->SetInteger(14, pAttack->m_deflect.m_nClassFeat);
            pData->SetInteger(15, pAttack->m_deflect.m_nAttackerFeatPenalty);
            pData->SetInteger(16, pAttack->m_deflect.m_nAttackerFeat);
#endif
            BroadcastDeflectBreakdown(oidTarget, pData);
            delete pData;
        }
    }

    if (bRanged != 0)
        SignalRangedDamage(oidTarget);
    else
        SignalMeleeDamage(oidTarget, 1);
}

void CSWSCreature::SendAttackSummaryFeedback(unsigned long long targetId)
{
    CSWSCombatRound* pRound = m_pCombatRound;
    CSWCCMessageData* pData = new CSWCCMessageData();
    CSWSCreature* pTarget = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(targetId);
    if (pRound != 0)
    {
        CSWSCombatAttackData* pAttack = pRound->GetAttack(pRound->m_currentAttack);
        int nDamage = pAttack->GetBaseDamage();
        if (pTarget != 0)
        {
            CSWPartyTable* pTable = g_pAppManager->m_pServerExoApp->GetPartyTable();
            if ((pTable != 0 && pTable->GetIsMember(targetId) != 0) || pTarget->m_bPlayerCharacter != 0)
            {
                unsigned char nDifficulty = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_difficultyLevel;
                float fMultiplier;
                if (g_pRules->m_p2DArrays->m_pDifficultyOptions->GetFLOATEntry(nDifficulty, CExoString("MULTIPLIER"), &fMultiplier) != 0)
                    nDamage = static_cast<int>(fMultiplier * static_cast<float>(nDamage));
            }
        }
        pData->SetInteger(0, pAttack->m_nAttackResult);
        pData->SetInteger(1, pAttack->m_nFeatId);
        pData->SetInteger(2, pAttack->m_attack.m_nAttackRoll + pAttack->m_attack.m_nAttackModifier);
        pData->SetInteger(3, pAttack->m_defense.m_nArmorClass);
        pData->SetInteger(4, nDamage);
        pData->SetInteger(5, pAttack->m_threat.m_bCriticalConfirmed);
        pData->SetInteger(6, pAttack->m_bSneakAttack);
        pData->SetInteger(7, pAttack->m_bCoupDeGrace);
        pData->SetInteger(8, pAttack->m_effect.m_nFeedbackEffectType);
        pData->SetInteger(9, pAttack->m_effect.m_bFeedbackEffectData);
        pData->SetInteger(10, pAttack->m_attack.m_bAutomaticHit);
        pData->SetInteger(11, pAttack->m_attack.m_bAutomaticMiss);
        pData->SetObjectID(0, m_objectId);
        pData->SetObjectID(1, targetId);
        BroadcastAttackSummaryToParty(pData);
        if (pTarget != 0)
            pTarget->BroadcastAttackSummaryToParty(pData);
    }
    delete pData;
}

// Fans one melee attack sequence out to the AI master. Each attack the round resolved queues the
// melee-attacked signal; a hit (result 1..3) builds the instantaneous damage effect carrying the
// 15-way damage breakdown and drains the attack's three deferred queues - feedback messages,
// pending effects and on-hit item spell impacts. A miss only shows a floaty "miss" over the
// target, for the local player.
void CSWSCreature::SignalMeleeDamage(unsigned long long oidTarget, int nNumAttacks)
{
    CServerAIMaster* pAIMaster = g_pAppManager->m_pServerExoApp->GetServerAIMaster();
    CSWSCombatRound* pRound = m_pCombatRound;
    unsigned int nDelay = 0;
    for (int i = 0; i < nNumAttacks; i++)
    {
        CSWSCombatAttackData* pAttack = pRound->GetAttack(i);
        pAIMaster->AddEventDeltaTime(0, i, m_objectId, oidTarget,
                                     CServerAIMaster::EVENT_ON_MELEE_ATTACKED, pAttack);
        m_pCombatRound->GetAttack(i);
        if (pAttack->m_nAttackResult == 0 || pAttack->m_nAttackResult > 3)
        {
            CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
            if (pClient->ServerToClientObjectId(m_objectId) == pClient->GetPlayerCreatureId())
                pClient->AddFloatyText(pClient->ServerToClientObjectId(oidTarget), CClientExoApp::FLOATY_MISS, 0);
        }
        else
        {
            CGameEffect* pEffect = new CGameEffect(1);
            pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE);
            pEffect->m_durationTypeBits =
                CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pEffect->SetCreator(m_objectId);
            pEffect->SetNumIntegers(22);
            for (int j = 0; j < 15; j++)
                pEffect->SetInteger(j, pAttack->m_anDamage[j]);
            pEffect->SetInteger(15, 1);
            pEffect->SetInteger(16, static_cast<unsigned short>(pAttack->m_nAnimationLength) >> 1);
            pEffect->SetInteger(19, 1);
            pEffect->SetInteger(21, 1);
            pAIMaster->AddEventDeltaTime(0, nDelay, m_objectId, oidTarget,
                                         CServerAIMaster::EVENT_APPLY_EFFECT, pEffect);
            for (int j = 0; j < pAttack->m_messages.m_count; j++)
                pAIMaster->AddEventDeltaTime(0, nDelay, m_objectId, oidTarget,
                                             CServerAIMaster::EVENT_FEEDBACK_MESSAGE,
                                             pAttack->m_messages.m_pArray[j]);
            pAttack->m_messages.m_count = 0;
            for (int j = 0; j < pAttack->m_effects.m_count; j++)
            {
                CSWSCreature* pTarget =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
                if (pTarget != 0)
                    pTarget->ApplyEffect(pAttack->m_effects.m_pArray[j], 0, 0);
                else
                    pAIMaster->AddEventDeltaTime(0, nDelay, m_objectId, oidTarget,
                                                 CServerAIMaster::EVENT_APPLY_EFFECT,
                                                 pAttack->m_effects.m_pArray[j]);
                pAttack->m_effects.m_pArray[j] = 0;
            }
            pAttack->m_effects.m_count = 0;
            for (int j = 0; j < pAttack->m_onHitSpellImpacts.m_count; j++)
                pAIMaster->AddEventDeltaTime(0, nDelay, m_objectId, m_objectId,
                                             CServerAIMaster::EVENT_ITEM_ON_HIT_SPELL_IMPACT,
                                             pAttack->m_onHitSpellImpacts.m_pArray[j]);
            pAttack->m_onHitSpellImpacts.m_count = 0;
        }
        nDelay += static_cast<unsigned short>(pAttack->m_nReaxnAnimLength) >> 1;
    }
}

// The ranged twin of SignalMeleeDamage fans the current attack out to the AI master once its
// projectile has landed. The queued sub-attack record decides everything, and a deflected shot
// (results 8..10) is re-aimed rather than applied: 8 and 10 fly on to a computed miss point, while
// 9 builds the damage effect and runs it back through the shooter's own resistance and immunity.
void CSWSCreature::SignalRangedDamage(unsigned long long oidTarget)
{
    int nAttack = m_pCombatRound->m_currentAttack;
    CSWSCombatAttackData* pAttack = m_pCombatRound->GetAttack(nAttack);
    int nTime = pAttack->m_nReaxnDelay;
    CSWSCombatSubAttack sub = pAttack->GetTopSubAttack();
    int bAttacked = sub.m_bAttackRolled;
    int bResolved = sub.m_bProjectileHit;
    int nProjectileType = sub.m_nDischargeHand;
    CServerAIMaster* pAIMaster = g_pAppManager->m_pServerExoApp->GetServerAIMaster();

    if (bAttacked != 0)
        pAIMaster->AddEventDeltaTime(0, nAttack, m_objectId, oidTarget,
                                     CServerAIMaster::EVENT_ON_MELEE_ATTACKED, pAttack);
    for (int i = 0; i < pAttack->m_messages.m_count; i++)
        pAIMaster->AddEventDeltaTime(0, nTime, m_objectId, oidTarget,
                                     CServerAIMaster::EVENT_FEEDBACK_MESSAGE,
                                     pAttack->m_messages.m_pArray[i]);
    pAttack->m_messages.m_count = 0;

    if (bResolved != 0)
    {
        CSWSCreature* pTarget =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidTarget);
        int bDeflected = 0;
        if (pTarget != 0 && pAttack->m_nAttackResult >= 8 && pAttack->m_nAttackResult <= 10)
            bDeflected = 1;
        if (bDeflected != 0 && (pAttack->m_nAttackResult | 2) == 10)
        {
            // Deflected clean away: re-aim at a computed miss point and fly there at 42 u/s.
            Vector vMiss = pTarget->ResolveProjectileMissLocation(this);
            unsigned int nFlight =
                static_cast<unsigned int>(magnitude(vMiss - pTarget->m_position) / 42.0f * 1000.0f);
            pTarget->ResolveProjectileMissImpact(vMiss, nFlight);
            pAttack->m_vMissLocation = vMiss;
            pAttack->m_nMissedBy = nFlight;
        }
        else
        {
            CGameEffect* pEffect = new CGameEffect(1);
            pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE);
            pEffect->m_durationTypeBits =
                CGameEffect::DURATION_TYPE_INSTANTANEOUS | (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pEffect->SetCreator(m_objectId);
            pEffect->SetNumIntegers(22);
            for (int i = 0; i < 15; i++)
                pEffect->SetInteger(i, pAttack->m_anDamage[i]);
            pEffect->SetInteger(16, nTime);
            pEffect->SetInteger(19, 1);
            pEffect->SetInteger(20, 1);
            pEffect->SetInteger(21, 1);
            if (bDeflected != 0)
            {
                if (pTarget != 0)
                {
                    // Reflected back at the shooter: run the base damage through the shooter's own
                    // resistance and immunity, then re-tag the effect with the surviving amount
                    // under its damage-type index.
                    int nDamage = pAttack->m_anDamage[14];
                    nDamage = DoDamageResistance(this, nDamage, static_cast<unsigned short>(GetDamageFlags()), 0, 1, 1);
                    nDamage = DoDamageImmunity(this, nDamage, static_cast<unsigned short>(GetDamageFlags()), 0, 1);
                    int nDamageIndex = Log_Base2(GetDamageFlags());
                    pEffect->SetInteger(nDamageIndex, nDamage);
                    pEffect->SetInteger(14, nDamage);
                    pAIMaster->AddEventDeltaTime(0, nTime * 2, oidTarget, m_objectId,
                                                 CServerAIMaster::EVENT_APPLY_EFFECT, pEffect);
                }
            }
            else
            {
                pAIMaster->AddEventDeltaTime(0, nTime, m_objectId, oidTarget,
                                             CServerAIMaster::EVENT_APPLY_EFFECT, pEffect);
            }
        }

        for (int i = 0; i < pAttack->m_effects.m_count; i++)
        {
            pAIMaster->AddEventDeltaTime(0, nTime, m_objectId, oidTarget,
                                         CServerAIMaster::EVENT_APPLY_EFFECT,
                                         pAttack->m_effects.m_pArray[i]);
            if (pAttack->m_effects.m_pArray[i]->m_subType == CSWSEffectListHandler::EFFECT_SETSTATE)
                SetAttackSummaryFeedbackEffectData(pAttack->m_effects.m_pArray[i]->GetInteger(0),
                                                   oidTarget);
            pAttack->m_effects.m_pArray[i] = 0;
        }
        pAttack->m_effects.m_count = 0;
        for (int i = 0; i < pAttack->m_onHitSpellImpacts.m_count; i++)
            pAIMaster->AddEventDeltaTime(0, nTime, m_objectId, m_objectId,
                                         CServerAIMaster::EVENT_ITEM_ON_HIT_SPELL_IMPACT,
                                         pAttack->m_onHitSpellImpacts.m_pArray[i]);
        pAttack->m_onHitSpellImpacts.m_count = 0;
    }
    else
    {
        ResolveProjectileMissImpact(pAttack->m_vMissLocation, nTime);
    }
    ResolveSafeProjectile(nAttack, nTime, bResolved, nProjectileType);
}

// Runs the damage-shield retaliation pass. Every EFFECT_DAMAGE_SHIELD on this creature fires an
// instantaneous damage effect straight back at whoever hit it. Integer 0 is the flat amount,
// integer 1 either a small bonus or an IPRP_DAMAGECOST row to roll, and integer 2 the damage-flag
// bit that selects the slot in the 15-entry damage vector.
void CSWSCreature::ResolveDamageShields(CSWSCreature* pAttacker)
{
    if (pAttacker == 0)
        return;
    for (int i = m_pStats->m_iFirstDamageShield; i < m_effects.m_count; i++)
    {
        CGameEffect* pEffect = m_effects.m_pArray[i];
        if (pEffect->m_subType > CSWSEffectListHandler::EFFECT_DAMAGE_SHIELD)
            return;
        if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_DAMAGE_SHIELD)
        {
            int nAmount = pEffect->GetInteger(0);
            int nDamage = pEffect->GetInteger(1);
            int nDamageFlags = pEffect->GetInteger(2);
            if (nDamage >= 6)
            {
                int nNumDice;
                g_pRules->m_p2DArrays->m_pIPRPDamage->GetINTEntry(
                    nDamage, CExoString("NumDice"), &nNumDice);
                int nDie;
                g_pRules->m_p2DArrays->m_pIPRPDamage->GetINTEntry(
                    nDamage, CExoString("Die"), &nDie);
                nDamage = g_pRules->RollDice(static_cast<unsigned char>(nNumDice), static_cast<unsigned char>(nDie));
            }
            int nIndex = Log_Base2(nDamageFlags);
            int nTotal = nDamage + nAmount;

            CGameEffect* pDamage = new CGameEffect(1);
            pDamage->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE);
            pDamage->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                          (pDamage->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pDamage->SetCreator(m_objectId);
            pDamage->SetNumIntegers(21);
            // The two arms index with different subscripts, the then arm with nIndex and the else
            // arm with j.
            for (int j = 0; j < 15; j++)
            {
                if (j != nIndex)
                    pDamage->SetInteger(j, -1);
                else
                    pDamage->SetInteger(nIndex, nTotal);
            }
            pDamage->SetInteger(16, 1000);
            pAttacker->ApplyEffect(pDamage, 0, 0);

            CGameEffect* pVfx = new CGameEffect(1);
            pVfx->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
            pVfx->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                       (pVfx->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pVfx->SetCreator(m_objectId);
            pVfx->SetInteger(0, 0);
            pAttacker->ApplyEffect(pVfx, 0, 0);
        }
    }
}

// Runs the passive on-hit item-property pass. It walks every passive property on the weapon that
// carried the attack and dispatches the matching ApplyOnHit handler for each live "On Hit"
// property. Subtypes 0..5 take a round count as their second argument, subtypes 6..11 take the item
// property. The chance and duration come from the on-hit duration table when the property is
// parameterised, and the DC from the item-property cost table's "Value" column, defaulting to 20.
void CSWSCreature::ResolveOnHitEffect(CSWSObject* pObject, int a, int b)
{
    K2_UNUSED(a); K2_UNUSED(b);
    CSWSCreature* pTarget = pObject->AsSWSCreature();
    if (pTarget == 0)
        return;
    if (m_pInventory == 0)
        return;
    // Two statements, not one nested expression, because the combat round is re-read after the
    // GetAttack call.
    CSWSCombatAttackData* pAttack = m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
    unsigned char nAttackType = pAttack->m_nWeaponAttackType;
    CSWSItem* pWeapon = m_pCombatRound->GetCurrentAttackWeapon(nAttackType);
    if (pWeapon == 0)
        return;

    int nChance = 0;
    int nDurationRounds = 0;
    for (int i = 0; i < pWeapon->m_nPassiveProperties; i++)
    {
        CSWItemProperty* pProperty = pWeapon->GetPassiveProperty(i);
        if (pProperty == 0)
            continue;
        unsigned char nSlot = pProperty->m_nUpgradeType;
        if (nSlot != CSWItemProperty::UPGRADE_TYPE_NONE && (pWeapon->m_nUpgrades & (1 << nSlot)) == 0)
            continue;
        if (pProperty->m_type != ITEM_PROPERTY_ON_HIT_PROPERTIES)
            continue;

        void (CSWSCreature::*pfnRounds)(CSWSObject*, int) K2_UNSET;
        void (CSWSCreature::*pfnProperty)(CSWSObject*, CSWItemProperty*) K2_UNSET;
        unsigned char nSave K2_UNSET;
        unsigned char nSaveVs K2_UNSET;
        int bRoundsHandler;
        int bSavingThrow;
        unsigned short nSubType = pProperty->m_subType;
        switch (nSubType)
        {
        case IPRP_ONHIT_SLEEP:
            pfnRounds = &CSWSCreature::ApplyOnHitSleep;
            nSaveVs = SAVING_THROW_TYPE_MIND_AFFECTING;
            nSave = SAVING_THROW_WILL;
            bRoundsHandler = 1;
            bSavingThrow = 1;
            break;
        case IPRP_ONHIT_STUN:
            pfnRounds = &CSWSCreature::ApplyOnHitStun;
            nSaveVs = SAVING_THROW_TYPE_MIND_AFFECTING;
            nSave = SAVING_THROW_WILL;
            bRoundsHandler = 1;
            bSavingThrow = 1;
            break;
        case IPRP_ONHIT_PARALYZE:
            pfnRounds = &CSWSCreature::ApplyOnHitHold;
            nSaveVs = SAVING_THROW_TYPE_PARALYSIS;
            nSave = SAVING_THROW_FORT;
            bRoundsHandler = 1;
            bSavingThrow = 1;
            break;
        case IPRP_ONHIT_CONFUSION:
            pfnRounds = &CSWSCreature::ApplyOnHitConfusion;
            nSaveVs = SAVING_THROW_TYPE_MIND_AFFECTING;
            nSave = SAVING_THROW_WILL;
            bRoundsHandler = 1;
            bSavingThrow = 1;
            break;
        case IPRP_ONHIT_FEAR:
            pfnRounds = &CSWSCreature::ApplyOnHitFear;
            nSaveVs = SAVING_THROW_TYPE_FEAR;
            nSave = SAVING_THROW_WILL;
            bRoundsHandler = 1;
            bSavingThrow = 1;
            break;
        case IPRP_ONHIT_SLOW:
            pfnRounds = &CSWSCreature::ApplyOnHitSlow;
            nSaveVs = SAVING_THROW_TYPE_NONE;
            nSave = SAVING_THROW_WILL;
            bRoundsHandler = 1;
            bSavingThrow = 1;
            break;
        case IPRP_ONHIT_ABILITY_DRAIN:
            nChance = 100;
            bSavingThrow = 1;
            pfnProperty = &CSWSCreature::ApplyOnHitAbilityDamage;
            bRoundsHandler = 0;
            nSaveVs = SAVING_THROW_TYPE_NONE;
            nSave = SAVING_THROW_REFLEX;
            break;
        case IPRP_ONHIT_ITEM_POISON:
            nChance = 100;
            bSavingThrow = 0;
            pfnProperty = &CSWSCreature::ApplyOnHitPoison;
            bRoundsHandler = 0;
            nSaveVs = SAVING_THROW_TYPE_POISON;
            nSave = SAVING_THROW_FORT;
            break;
        case IPRP_ONHIT_SLAY_RACIAL_GROUP:
            bSavingThrow = 1;
            pfnProperty = &CSWSCreature::ApplyOnHitSlayRacialGroup;
            bRoundsHandler = 0;
            nSaveVs = SAVING_THROW_TYPE_DEATH;
            nSave = SAVING_THROW_FORT;
            break;
        case IPRP_ONHIT_SLAY_ALIGNMENT_GROUP:
            bSavingThrow = 1;
            pfnProperty = &CSWSCreature::ApplyOnHitSlayAlignmentGroup;
            bRoundsHandler = 0;
            nSaveVs = SAVING_THROW_TYPE_DEATH;
            nSave = SAVING_THROW_FORT;
            break;
        case IPRP_ONHIT_INSTANT_DEATH:
            nChance = 100;
            bSavingThrow = 0;
            pfnProperty = &CSWSCreature::ApplyOnHitInstantDeath;
            bRoundsHandler = 0;
            break;
#if !KOTOR1  // K1: no knockdown on-hit property.
        case IPRP_ONHIT_KNOCKDOWN:
            nChance = 100;
            bSavingThrow = 1;
            pfnProperty = &CSWSCreature::ApplyOnHitKnockdown;
            bRoundsHandler = 0;
            nSaveVs = SAVING_THROW_TYPE_NONE;
            nSave = SAVING_THROW_FORT;
            break;
#endif
        default:
            continue;
        }

        int nParam1ResRef;
        if (g_pRules->m_p2DArrays->m_pOnHit->GetINTEntry(
                nSubType, CExoString("Param1ResRef"), &nParam1ResRef) != 0 &&
            nParam1ResRef == 1)
        {
            g_pRules->m_p2DArrays->m_pOnHitDuration->GetINTEntry(
                pProperty->m_nParam1Value, CExoString("EffectChance"), &nChance);
            g_pRules->m_p2DArrays->m_pOnHitDuration->GetINTEntry(
                pProperty->m_nParam1Value, CExoString("DurationRounds"), &nDurationRounds);
        }
        int nDC;
        if (g_pRules->m_p2DArrays->GetIPRPCostTable(IPRP_COST_TABLE_ON_HIT_DC)->GetINTEntry(
                pProperty->m_costTableValue, CExoString("Value"), &nDC) == 0)
            nDC = 20;

        if (bRoundsHandler != 0)
        {
            if (nChance > 0 && nDurationRounds > 0 &&
                rand_wincompatible() % 100 < nChance &&
                (bSavingThrow == 0 ||
                 pTarget->SavingThrowRoll(nSave, static_cast<unsigned short>(nDC), nSaveVs, m_objectId, 0, 0, 1) == 0))
                (this->*pfnRounds)(pObject, nDurationRounds);
        }
        else if (nChance == 100 || rand_wincompatible() % 100 < nChance)
        {
            if (bSavingThrow == 0 ||
                pTarget->SavingThrowRoll(nSave, static_cast<unsigned short>(nDC), nSaveVs, m_objectId, 0, 0, 1) == 0)
                (this->*pfnProperty)(pObject, pProperty);
        }
    }
}

// One instantaneous visual effect per damage type that landed, plus a fixed flash when the attack
// result is one of the parry/deflect codes. The three physical types below the first
// damage-flag bit share the weapon's own impact visual. The 2DA column is picked per attack:
// "RangedEffect" for a ranged attack, "VisualEffect" for a melee one.
void CSWSCreature::ResolveOnHitVisuals(CSWSObject* pObject)
{
    CSWSCombatAttackData* pAttack =
        m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
    for (int i = 0; i < 14; i++)
    {
        if (i >= 3 && pAttack->GetDamage(static_cast<unsigned short>(1 << i)) > 0)
        {
            CExoString sColumn;
            int nVisualEffect = 0;
            CGameEffect* pVfx;
            if (pAttack->m_bRangedAttack != 0)
                sColumn = "RangedEffect";
            else
                sColumn = "VisualEffect";
            g_pRules->m_p2DArrays->m_pDamageHitVisual->GetINTEntry(i, sColumn, &nVisualEffect);
            pVfx = new CGameEffect(1);
            pVfx->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
            pVfx->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                       (pVfx->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
            pVfx->SetCreator(m_objectId);
            pVfx->SetInteger(0, nVisualEffect);
            pAttack->m_effects.Add(pVfx);
        }
    }
    if (pAttack->m_nAttackResult < 8 || pAttack->m_nAttackResult > 10)
        return;
    CGameEffect* pDeflect = new CGameEffect(1);
    pDeflect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_VISUALEFFECT);
    pDeflect->m_durationTypeBits = CGameEffect::DURATION_TYPE_INSTANTANEOUS |
                                   (pDeflect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
    pDeflect->SetCreator(pObject->m_objectId);
    pDeflect->SetInteger(0, VFX_COM_BLASTER_DEFLECTION);
    pAttack->m_effects.Add(pDeflect);
}

// Decides whether the attack in flight is a sneak attack. Only a creature holding one of the ten
// sneak-attack feats can qualify. The target is caught out when it is not in one of the three
// aware-and-upright states, is a downed party member, or has not spotted us; failing those, the
// attack still qualifies when it comes from outside a 90-degree frontal cone. Ranged attacks
// also need the target within 10 metres.
void CSWSCreature::ResolveSneakAttack(CSWSCreature* pTarget)
{
    if (pTarget == 0)
        return;
    int nAttack = m_pCombatRound->m_currentAttack;
    CSWSCreatureStats* pStats = m_pStats;
    int bRanged = m_pCombatRound->GetAttack(nAttack)->m_bRangedAttack;
    if (pStats->HasFeat(FEAT_SNEAK_ATTACK_1D6) || pStats->HasFeat(FEAT_SNEAK_ATTACK_2D6)
        || pStats->HasFeat(FEAT_SNEAK_ATTACK_3D6) ||
        pStats->HasFeat(FEAT_SNEAK_ATTACK_4D6) || pStats->HasFeat(FEAT_SNEAK_ATTACK_5D6) || pStats->HasFeat(FEAT_SNEAK_ATTACK_6D6) ||
        pStats->HasFeat(FEAT_SNEAK_ATTACK_7D6) || pStats->HasFeat(FEAT_SNEAK_ATTACK_8D6) || pStats->HasFeat(FEAT_SNEAK_ATTACK_9D6) ||
        pStats->HasFeat(FEAT_SNEAK_ATTACK_10D6))
    {
        float fDistance K2_UNSET;
        if (bRanged != 0)
        {
            Vector vDelta = m_position - pTarget->m_position;
            fDistance = magnitude(vDelta);
        }
        int bSneak = 0;
        switch (pTarget->m_nState)
        {
        case EFFECT_STATE_NONE:
#if KOTOR1  // K1: only state 0 can be caught unaware; the dying test is GetDeadTemp().
            if (pTarget->GetDeadTemp() != 0)
#else
        case EFFECT_STATE_CONFUSED:
        case EFFECT_STATE_DROID_CONFUSED:
            if (pTarget->m_bInParty != 0 && pTarget->GetCurrentHitPoints(0) < 1)
#endif
            {
                bSneak = 1;
            }
#if !KOTOR1  // K1: no meditation posture
            else if (pTarget->m_nStancePosture == STANCE_POSTURE_MEDITATIVE)
            {
                bSneak = 1;
            }
#endif
            else
            {
                CSWVisibilityNode* pNode = pTarget->GetVisibleListElement(m_objectId);
                if (pNode == 0 || (pNode->m_flags & VISIBILITY_SEEN) == 0)
                {
                    if (bRanged == 1 && fDistance <= 10.0f)
                        bSneak = 1;
                    else if (bRanged == 0)
                        bSneak = 1;
                }
            }
            break;
        default:
            bSneak = 1;
            break;
        }
        if (bSneak == 1)
        {
            if (pTarget->m_pStats->GetEffectImmunity(
                    static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_BACKSTAB), this) != 0 ||
                pTarget->m_pStats->GetEffectImmunity(
                    static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_CRITICAL_HITS), this) != 0)
            {
                CSWCCMessageData* pMessage = new CSWCCMessageData;
                pMessage->SetObjectID(0, pTarget->m_objectId);
                pMessage->SetInteger(0, FEEDBACK_IMMUNE_SNEAK_ATTACKS);
                m_pCombatRound->GetAttack(nAttack)->m_messages.Add(pMessage);
            }
            else
            {
                m_pCombatRound->GetAttack(nAttack)->m_bSneakAttack = 1;
            }
        }
        else if (bSneak == 0)
        {
            Vector vDelta = m_position - pTarget->m_position;
            Vector vApproach = normalize(vDelta);
            Vector vFacing = normalize(pTarget->m_orientation);
            float fDot = vApproach.x * vFacing.x + vApproach.y * vFacing.y +
                         vApproach.z * vFacing.z;
            if (static_cast<double>(fDot) < 0.707)
            {
                if (bRanged != 0 && fDistance <= 10.0f)
                    m_pCombatRound->GetAttack(nAttack)->m_bSneakAttack = 1;
                else if (bRanged == 0)
                    m_pCombatRound->GetAttack(nAttack)->m_bSneakAttack = 1;
            }
        }
    }
}

// Computes where a projectile that missed its target lands: the perpendicular to the shooter-target
// line in the ground plane, flipped to a random side, scaled by the target's personal space and by
// a spread that widens when the attack resolved as a deflect, then raised by a range-dependent arc.
// The final segment is walked with ClearLineOfSight, so the projectile stops at the first wall it
// meets.
Vector CSWSCreature::ResolveProjectileMissLocation(CSWSObject* pTarget)
{
    if (pTarget == 0) {
        Vector vZero = { 0.0f, 0.0f, 0.0f };
        return vZero;
    }

    float fSpread = 1.0f;
    float fScale;
    if (pTarget->AsSWSCreature() == 0) {
        fScale = 0.6f * 0.6667f;
    } else {
        fScale = pTarget->AsSWSCreature()->m_pPathfindInfo->m_fPersonalSpace * 0.6667f;
        pTarget->AsSWSCreature();  // result unused
        CSWSCombatRound* pRound = pTarget->AsSWSCreature()->m_pCombatRound;
        if (pRound->GetAttack(pRound->m_currentAttack)->m_nAttackResult == 10)
            fSpread = static_cast<float>(rand_wincompatible() % 3 + rand_wincompatible() % 3 +
                              rand_wincompatible() % 3);
    }

    Vector vDelta = m_position - pTarget->m_position;
    Vector vDir = normalize(vDelta);

    // Cross product against world up.
    Vector vUp = { 0.0f, 0.0f, 1.0f };
    Vector vSide;
    vSide.x = vDir.y * vUp.z - vDir.z * vUp.y;
    vSide.y = vDir.z * vUp.x - vDir.x * vUp.z;
    vSide.z = vDir.x * vUp.y - vDir.y * vUp.x;
    if ((rand_wincompatible() & 1) != 0) {
        vSide.x = -vSide.x;
        vSide.y = -vSide.y;
        vSide.z = -vSide.z;
    }

    Vector vMiss = pTarget->m_position + vSide * fSpread * fScale;

    float fArc = (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z) * 1.5f / 100.0f;
    float fArcRand = static_cast<float>(rand_wincompatible() % 1000) / 1000.0f * fArc * 0.75f;
    vMiss.z = vMiss.z + (1.5f - fArc / 1.5f + fArcRand);

    Vector vFrom = m_position;
    vFrom.z = vFrom.z + 1.5f;
    Vector vDirTo = normalize(vMiss - vFrom);
    Vector vTo = vFrom + vDirTo * 1000.0f;

    Vector vHit = { 0.0f, 0.0f, 0.0f };
    unsigned long long oidHit = OBJECT_INVALID;
    if (static_cast<CSWSArea*>(GetArea())->ClearLineOfSight(vFrom, vTo, &vHit, &oidHit,
                                                 m_objectId, pTarget->m_objectId, 1, 1) != 0)
        return vTo;
    return vHit;
}

int g_nCombatAttackRoll;

void CSWSCreature::ResolveAttackRoll(CSWSObject* pTarget, int bForceResult, int nForcedResult)
{
    if (pTarget == 0)
        return;

    CSWSCombatRound* pRound = m_pCombatRound;
    CSWSCombatAttackData* pAttack = pRound->GetAttack(pRound->m_currentAttack);
    pAttack->m_threat.Clear();
    int nOffHandAttack = pRound->GetOffHandAttack();
    pAttack->m_nWeaponAttackType = static_cast<unsigned char>(m_pCombatRound->GetWeaponAttackType());

    int nRoll = g_pRules->RollDice(1, 20);
    if (g_nCombatAttackRoll != 0)
        nRoll = g_nCombatAttackRoll;

    int nAttackMod = m_pStats->GetAttackModifierVersus(pTarget->AsSWSCreature());

    int nArmorClass = 0;
    CSWSCreature* pTargetCreature = 0;
    if (pTarget->AsSWSCreature() != 0) {
        pTargetCreature = pTarget->AsSWSCreature();
        nArmorClass = pTargetCreature->m_pStats->GetArmorClassVersus(this, 0);
        pAttack->m_defense.m_nArmorClass = static_cast<signed char>(nArmorClass);
        ResolveSneakAttack(pTargetCreature);
    }

    if (bForceResult != 0) {
        pAttack->m_nAttackResult = static_cast<unsigned char>(nForcedResult);
        return;
    }

    if (pAttack->m_bCoupDeGrace != 0) {
        pAttack->m_nAttackResult = 3;
        pAttack->m_attack.m_nAttackRoll = 20;
        pAttack->m_attack.m_nAttackModifier = static_cast<signed char>(nAttackMod);
        return;
    }

    if (m_bAssuredHit != 0) {
        pAttack->m_attack.m_nAttackRoll = static_cast<unsigned char>(nRoll);
        pAttack->m_nAttackResult = 1;
        pAttack->m_attack.m_nAttackModifier = static_cast<signed char>(nAttackMod);
        return;
    }

    int nTotal = nAttackMod + nRoll;
    pAttack->m_attack.m_nAttackRoll = static_cast<unsigned char>(nRoll);
    pAttack->m_attack.m_nAttackModifier = static_cast<signed char>(nAttackMod);
    if (ResolveDefensiveEffects(pTarget, nTotal >= nArmorClass) != 0)
        return;

    if (nRoll == 1) {
        pAttack->m_nAttackResult = 4;
        pAttack->m_attack.m_bAutomaticMiss = 1;
        return;
    }
    pAttack->m_attack.m_bAutomaticMiss = 0;
    if (nRoll != 20 && nTotal < nArmorClass) {
        pAttack->m_nAttackResult = 4;
        return;
    }
    pAttack->m_attack.m_bAutomaticHit = (nRoll == 20);

    // The threat range, widened by Makashi and narrowed by Soresu when a lightsaber is in hand.
    int nThreatRange = m_pStats->GetCriticalHitRoll(nOffHandAttack);
#if !KOTOR1  // K1: no lightsaber forms
    if (m_stanceId == FORM_SABER_III_SORESU && IsWieldingLightsaber() != 0) {
        if (nThreatRange < 20)
            nThreatRange++;
    } else if (m_stanceId == FORM_SABER_IV_ATARU) {
        if (IsWieldingLightsaber() != 0 && nThreatRange > 1)
            nThreatRange--;
    }
#endif
    pAttack->m_threat.m_nThreatRange = static_cast<unsigned char>(nThreatRange);

    if (nRoll < nThreatRange) {
        pAttack->m_threat.m_bCriticalThreat = 0;
        pAttack->m_threat.m_bCriticalConfirmed = 0;
        pAttack->m_threat.m_nConfirmRoll = 0;
    } else {
        pAttack->m_threat.m_bCriticalThreat = 1;
        int nConfirmRoll = g_pRules->RollDice(1, 20);
        pAttack->m_threat.m_nConfirmRoll = static_cast<unsigned char>(nConfirmRoll);
#if !KOTOR1
        if (m_stanceId == FORM_SABER_VII_JUYO && IsWieldingLightsaber() != 0)
            nConfirmRoll += 4;
#endif

        if (nConfirmRoll + nAttackMod >= nArmorClass) {
            pAttack->m_threat.m_bCriticalConfirmed = 1;
            pAttack->m_threat.m_nCriticalMultiplier = static_cast<signed char>(m_pStats->GetCriticalHitMultiplier(nOffHandAttack));

            if (pTargetCreature != 0 &&
                pTargetCreature->m_pStats->GetEffectImmunity(
                    static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_CRITICAL_HITS), this) != 0) {
                // Immune: the crit degrades to a plain hit and the target is told why.
                CSWCCMessageData* pMessage = new CSWCCMessageData;
                pMessage->SetObjectID(0, pTargetCreature->m_objectId);
                pMessage->SetInteger(0, FEEDBACK_IMMUNE_CRITICAL_HITS);
                pAttack->m_messages.Add(pMessage);
                pAttack->m_nAttackResult = 1;
                return;
            }

            pAttack->m_nAttackResult = 2;
            return;
        }
        pAttack->m_threat.m_bCriticalConfirmed = 0;
    }

    pAttack->m_threat.m_nCriticalMultiplier = 0;
    pAttack->m_nAttackResult = 1;
}

// Resolves blaster deflection. It runs after the plain roll-vs-AC verdict and, when the target can
// parry the incoming projectile, replaces the outcome. It builds the whole deflect breakdown record
// and returns non-zero so the attack is treated as already decided. Result 8 is a harmless deflect;
// 9 sends the bolt back at the attacker.
__attribute__((no_stack_protector)) int CSWSCreature::ResolveDefensiveEffects(CSWSObject* pTarget, int bWouldHit)
{
#if KOTOR1  // K1: no redirect out-param; the bolt goes back on a margin of 6 or more.
    CSWSCombatAttackData* pAttack =
        m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
    int nUnused = 0;
    if (IsProjectileParryable(pTarget, bWouldHit, 0, nUnused, -1) == 0)
        return 0;
#else
    int nRedirect = 1;
    CSWSCombatAttackData* pAttack =
        m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
    int nUnused = 0;
    if (IsProjectileParryable(pTarget, bWouldHit, 0, nUnused, nRedirect, -1) == 0)
        return 0;
#endif
    if (pTarget->AsSWSCreature() == 0)
        return 0;

    CSWSCreature* pDefender = pTarget->AsSWSCreature();
    int bDeflectBack;
    if (pDefender->m_bAssuredDeflection != 0) {
        bDeflectBack = pDefender->m_bAssuredDeflectionRedirect != 0;
    } else {
#if !KOTOR1
        int bCanRedirect = nRedirect;
#endif
        int nDeflect = g_pRules->RollDice(1, 20);
        pAttack->m_deflect.m_nDeflectRoll = static_cast<unsigned char>(nDeflect);

        if (pDefender->m_pStats->HasFeat(FEAT_MASTER_JEDI_DEFENSE) != 0) {
            nDeflect += 6;
            pAttack->m_deflect.m_nDeflectFeat = FEAT_MASTER_JEDI_DEFENSE;
            pAttack->m_deflect.m_nDeflectFeatBonus = 6;
        } else if (pDefender->m_pStats->HasFeat(FEAT_ADVANCED_JEDI_DEFENSE) != 0) {
            nDeflect += 3;
            pAttack->m_deflect.m_nDeflectFeat = FEAT_ADVANCED_JEDI_DEFENSE;
            pAttack->m_deflect.m_nDeflectFeatBonus = 3;
        }

#if !KOTOR1  // K1: no deflection power, Jedi class feat or attacker rapid-shot terms
        if (pDefender->m_pStats->HasSpell(0, FORCE_POWER_FORCE_REDIRECTION, 0) != 0) {
            nDeflect += 3;
            pAttack->m_deflect.m_nPowerBonus = 3;
            pAttack->m_deflect.m_nPowerId = FORCE_POWER_FORCE_REDIRECTION;
        }

        if (pDefender->m_pStats->HasFeat(FEAT_DEFLECT) != 0 && pDefender->IsWieldingLightsaber() != 0) {
            CSWSCreatureStats* pStats = pDefender->m_pStats;
            for (unsigned char i = 0; i < 2; i++) {
                if (i < pStats->m_numClasses
                    && pStats->m_classInfo[i].m_class == CLASS_TYPE_JEDIWEAPONMASTER) {
                    int nBonus = (pStats->GetClassLevel(i, 0) + 1) / 2;
                    nDeflect += nBonus;
                    pAttack->m_deflect.m_nClassFeat = FEAT_DEFLECT;
                    pAttack->m_deflect.m_nClassFeatBonus = static_cast<signed char>(nBonus);
                    break;
                }
            }
        }
#endif

#if !KOTOR1  // K1: no lightsaber forms
        if (pDefender->IsWieldingLightsaber() != 0) {
            switch (pDefender->m_stanceId) {
            case FORM_SABER_II_MAKASHI:
                nDeflect -= 5;
                pAttack->m_deflect.m_nStanceBonus = -5;
                pAttack->m_deflect.m_nStanceId = FORM_SABER_II_MAKASHI;
                break;
            case FORM_SABER_III_SORESU:
                nDeflect += 4;
                pAttack->m_deflect.m_nStanceBonus = 4;
                pAttack->m_deflect.m_nStanceId = FORM_SABER_III_SORESU;
                break;
            case FORM_SABER_IV_ATARU:
                nDeflect -= 4;
                pAttack->m_deflect.m_nStanceBonus = -4;
                pAttack->m_deflect.m_nStanceId = FORM_SABER_IV_ATARU;
                break;
            case FORM_SABER_V_SHIEN:
                nDeflect += 2;
                pAttack->m_deflect.m_nStanceBonus = 2;
                pAttack->m_deflect.m_nStanceId = FORM_SABER_V_SHIEN;
                break;
            case FORM_SABER_VI_NIMAN:
                nDeflect += 1;
                pAttack->m_deflect.m_nStanceBonus = 1;
                pAttack->m_deflect.m_nStanceId = FORM_SABER_VI_NIMAN;
                break;
            }
        }
#endif

#if !KOTOR1
        // The attacker's feats, not the defender's, because the rapid-shot family makes a shot
        // harder to deflect.
        if (m_pStats->HasFeat(FEAT_PRECISE_SHOT_V) != 0) {
            pAttack->m_deflect.m_nAttackerFeat = FEAT_PRECISE_SHOT_V;
            pAttack->m_deflect.m_nAttackerFeatPenalty = -10;
            nDeflect -= 10;
        } else if (m_pStats->HasFeat(FEAT_PRECISE_SHOT_IV) != 0) {
            pAttack->m_deflect.m_nAttackerFeat = FEAT_PRECISE_SHOT_IV;
            pAttack->m_deflect.m_nAttackerFeatPenalty = -8;
            nDeflect -= 8;
        } else if (m_pStats->HasFeat(FEAT_MASTER_PRECISE_SHOT) != 0) {
            pAttack->m_deflect.m_nAttackerFeat = FEAT_MASTER_PRECISE_SHOT;
            pAttack->m_deflect.m_nAttackerFeatPenalty = -6;
            nDeflect -= 6;
        } else if (m_pStats->HasFeat(FEAT_IMPROVED_PRECISE_SHOT) != 0) {
            pAttack->m_deflect.m_nAttackerFeat = FEAT_IMPROVED_PRECISE_SHOT;
            pAttack->m_deflect.m_nAttackerFeatPenalty = -4;
            nDeflect -= 4;
        } else if (m_pStats->HasFeat(FEAT_PRECISE_SHOT) != 0) {
            pAttack->m_deflect.m_nAttackerFeat = FEAT_PRECISE_SHOT;
            pAttack->m_deflect.m_nAttackerFeatPenalty = -2;
            nDeflect -= 2;
        }
#endif

        int nTotal = pDefender->m_pStats->GetBaseAttackBonus() + nDeflect;
        pAttack->m_deflect.m_nBaseAttackBonus = static_cast<unsigned char>(pDefender->m_pStats->GetBaseAttackBonus());
        char nArmourPenalty = reinterpret_cast<unsigned char*>(&pDefender->m_pStats->m_nArmorCheckPenalty)[1]
                            + reinterpret_cast<unsigned char*>(&pDefender->m_pStats->m_nArmorCheckPenalty)[0];
        pAttack->m_deflect.m_nArmourPenalty = nArmourPenalty;
        nTotal += nArmourPenalty;

        int nEffectBonus = 0;
        for (int i = 0; i < pDefender->m_effects.m_count; i++) {
            CGameEffect* pEffect = pDefender->m_effects.m_pArray[i];
#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // K1: the decrease effect's amount is added too.
            if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_BLASTERDEFLECTIONINCREASE
                || pEffect->m_subType == CSWSEffectListHandler::EFFECT_BLASTERDEFLECTIONDECREASE)
                nEffectBonus += pEffect->GetInteger(1);
#else
            if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_BLASTERDEFLECTIONINCREASE)
                nEffectBonus += pEffect->GetInteger(1);
            else if (pEffect->m_subType == CSWSEffectListHandler::EFFECT_BLASTERDEFLECTIONDECREASE)
                nEffectBonus -= pEffect->GetInteger(1);
#endif
        }

        int nDeflectTotal = nTotal + nEffectBonus;
        pAttack->m_deflect.m_nEffectBonus = static_cast<signed char>(nEffectBonus);
#if KOTOR1
        pAttack->m_deflect.m_nDeflectTotal = static_cast<unsigned char>(nDeflectTotal);
#else
        pAttack->m_deflect.m_nDeflectTotal = nDeflectTotal;
#endif
        int nAttackTotal =
            pAttack->m_attack.m_nAttackRoll + pAttack->m_attack.m_nAttackModifier;
#if KOTOR1
        pAttack->m_deflect.m_nAttackTotal = static_cast<unsigned char>(nAttackTotal);
#else
        pAttack->m_deflect.m_nAttackTotal = nAttackTotal;
#endif

        if (nDeflectTotal < nAttackTotal)
            return 0;

#if KOTOR1
        bDeflectBack = nDeflectTotal >= nAttackTotal + 6;
#else
        bDeflectBack = (bCanRedirect != 0) & (nDeflectTotal >= nAttackTotal + 6);
#endif
    }

    // 8 = the bolt is swatted aside, 9 = it is sent back at the shooter.
    int nResult = 8 + (bDeflectBack != 0);
    pAttack->m_nAttackResult = static_cast<unsigned char>(nResult);
    pAttack->m_deflect.m_nDeflectResult = static_cast<unsigned char>(nResult);
    return 1;
}

// Picks (or re-picks) the creature's attack target for the coming round and queues the round
// action for it. bForce queues unconditionally; bHaveTarget says the target was nominated in
// m_oidAttemptedSpellTarget.
void CSWSCreature::CreateNewAttackActions(int bForce, int bHaveTarget,
                                          unsigned long long oidAttacker)
{
    unsigned long long oidTarget = bHaveTarget ? m_oidAttemptedSpellTarget : m_pCombatRound->m_oidAttackTarget;
    CExoLinkedListNode* pPending = m_pCombatRound->m_pRoundActionList->m_pInternal->m_pHead;
    int bSelfTarget = 0;

    if (oidTarget == OBJECT_INVALID) {
        if (pPending == 0)
            bForce = 1;
        bHaveTarget = 0;
    } else if (pPending != 0) {
        bHaveTarget = 0;
    } else {
        CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
        if (pObject == 0) {
            m_pCombatRound->m_oidAttackTarget = OBJECT_INVALID;
            bHaveTarget = 0;
        } else if (pObject->AsSWSCreature() == 0) {
            bHaveTarget = 1;
        } else {
            CSWSCreature* pTarget = pObject->AsSWSCreature();
#if KOTOR1  // K1: GetDeadTemp() dying tests; a non-hostile target is dropped.
            if (pTarget->GetDead() != 0 || pTarget->GetDeadTemp() != 0
                    || GetDead() != 0 || GetDeadTemp() != 0)
                bHaveTarget = 0;
            else if (GetReputation(oidTarget, 0) >= 11)
                bHaveTarget = 0;
            else if (bHaveTarget == 0) {
#else
            if (pTarget->GetDead() != 0)
                bHaveTarget = 0;
            else if (pTarget->GetDeadTemp() != 0)
                bHaveTarget = 0;
            else if (GetDead() != 0)
                bHaveTarget = 0;
            else if (GetDeadTemp() != 0)
                bHaveTarget = 0;
            else if (GetReputation(oidTarget, 0) >= 11) {
                bHaveTarget = 1;
                bSelfTarget = (oidTarget == m_objectId);
            } else if (bHaveTarget == 0) {
#endif
                bHaveTarget = 1;
            } else {
                Vector vDelta = pTarget->m_position - m_position;
                float fRange = MaxCleaveRange(pTarget->m_objectId, 0, 1);
                if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z
                        > fRange * fRange)
                    bHaveTarget = 0;
                else
                    bHaveTarget = 1;
            }
        }
    }

    if (bForce != 0 || bSelfTarget != 0) {
        CSWSCreature* pAttacker =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidAttacker);
        oidTarget = OBJECT_INVALID;
        if (pAttacker != 0)
            oidTarget = pAttacker->GetNearestEnemy(MaxCleaveRange(m_objectId, 0, 1),
                                                   m_objectId, m_objectId);
        if (oidTarget == OBJECT_INVALID)
            oidTarget = GetNearestEnemy(MaxCleaveRange(m_objectId, 0, 1),
                                        m_objectId, OBJECT_INVALID);
        if (oidTarget == OBJECT_INVALID ||
            g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget)->AsSWSCreature() == 0) {
            SetLockOrientationToObject(OBJECT_INVALID, 0);
            SetAnimation(SERVER_ANIMATION_PAUSE);
            SetCombatMode(CREATURE_COMBAT_MODE_NONE, 1);
            m_pCombatRound->ClearAllSpecialAttacks();
            return;
        }
        SetLockOrientationToObject(oidTarget, 0);
        m_attackTargetId = oidTarget;
        SetAttemptedAttackTarget(oidTarget);
        m_pCombatRound->UpdateAttackTargetForAllActions(oidTarget);
        m_bPassiveAttackBehaviour = 1;
        CClientExoApp* pClient = g_pAppManager->m_pClientExoApp;
        pClient->SetLastTarget(pClient->ServerToClientObjectId(oidTarget), 1);
    }

    if (bForce != 0 || bHaveTarget != 0) {
#if !KOTOR1
        if (m_pCombatRound->m_nExecutingActionType == CSWSCombatRoundAction::TYPE_STANCE) {
            m_pCombatRound->AddStanceAction(STANCE_POSTURE_TOTAL_DEFENSE, oidTarget, 0);
            return;
        }
#endif
        m_pCombatRound->AddAttackAction(oidTarget, 0, 0, 1500, 1);
        if (m_oidPendingAttackTarget == oidTarget)
            m_oidPendingAttackTarget = OBJECT_INVALID;
    }
}

// The ranged half of the per-attack resolution pair. One call stages one attack of the combat
// round and fires it as nShots separate discharges: every discharge rolls its own hit or miss
// against a shrinking budget, and only the ones that land walk the attack-roll, special-attack
// and damage chain.
void CSWSCreature::ResolveRangedAttack(CSWSObject* pTarget, int a, int nTime, int bForceResult,
                                       int nForcedAnimation, int nForcedResult, int nDamageFlags)
{
    K2_UNUSED(a);
    if (pTarget == 0)
        return;

    CSWSCombatAttackData* pAttack = m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
    pAttack->m_bRangedAttack = 1;
    pAttack->m_targetObjectId = pTarget->m_objectId;
    pAttack->m_nAttackGroup = ++m_pCombatRound->m_nAttackGroupCounter;

    int nAnimation = nForcedAnimation;
    if (bForceResult == 0) {
        unsigned short nFeat = pAttack->m_nFeatId;
        nAnimation = g_pAppManager->m_pClientExoApp->GetClientRangedAnimation(
            m_objectId, nFeat, 0, 0);
    }

    int nShots = GetWeaponDischargeShots(nAnimation);
    int nHitsLeft = GetWeaponHitShots(nAnimation);
    nHitsLeft = static_cast<unsigned char>(m_pCombatRound->GetTotalAttacks());

#if !KOTOR1
    int nTotalDamage = 0;
#endif
    int nSubAttack = 0;
    int bSpecialAnimation = 0;
    // The hit/miss draw selects nHitsLeft of the nShots discharges without replacement, so their
    // positions are spread uniformly.
    int nRemainingShots = nShots;
    for (int i = 0; i < nShots; i++) {
        pAttack->m_nAttackResult = 4;
        int bShotHits = (rand_wincompatible() % nRemainingShots) < nHitsLeft;
        nHitsLeft -= bShotHits;

        int bResolved;
        if (bShotHits) {
            m_pCombatRound->SetOffHandAttack(
                nSubAttack >= static_cast<unsigned char>(m_pCombatRound->m_nOnHandAttacks));
            pAttack->m_nWeaponAttackType = (m_pCombatRound->GetOffHandAttack() != 0)
                                               ? CSWSCombatRound::WEAPON_ATTACK_TYPE_OFFHAND
                                               : CSWSCombatRound::WEAPON_ATTACK_TYPE_ONHAND;
            ResolveAttackRoll(pTarget, bForceResult, nForcedResult);
#if KOTOR1  // K1: the special attack runs only for a queued feat.
            if (pAttack->m_nFeatId != 0)
                ResolveRangedSpecialAttack(pTarget, nSubAttack);
#else
            ResolveRangedSpecialAttack(pTarget, nSubAttack);
#endif
            unsigned char nResult = pAttack->m_nAttackResult;
            bResolved = !((static_cast<unsigned char>(nResult - 1) >= 3) && (static_cast<unsigned char>(nResult - 8) >= 3));
            if (bResolved)
                bSpecialAnimation = 1;
            nSubAttack++;
        } else {
            bResolved = 0;
        }

        if (static_cast<unsigned char>(pAttack->m_nAttackResult - 4) < 3)
            bResolved = ResolveRangedMissToParry(pTarget, 1, GetWeaponDischargeHand(nAnimation, i));

        if (bResolved) {
            ResolveRangedHit(pTarget);
            unsigned char nResult = pAttack->m_nAttackResult;
            bool bDamageResult = nResult == 9 || (nResult - 1U) <= 2U;
            if (bDamageResult) {
                ResolveDamage(pTarget, bForceResult, nDamageFlags);
#if !KOTOR1  // K1: no Fury
                CSWSCombatAttackData* pDamaged =
                    m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
                if (pDamaged->GetBaseDamage() > 0)
                    nTotalDamage += pDamaged->GetBaseDamage();
#endif
                ResolvePostRangedDamage(pTarget);
            }
        } else {
            ResolveRangedMiss(pTarget);
        }

        int nDelay = GetWeaponDischargeTime(nAnimation, i);
        pAttack->ResolveAttackToRangedSubAttack(i, nDelay, bShotHits, bResolved,
                                                GetWeaponDischargeHand(nAnimation, i));
        nRemainingShots--;
    }

#if !KOTOR1  // K1: no Fury
    if (nTotalDamage >= pTarget->GetCurrentHitPoints(0)) {
        if (m_pStats->m_nFuryDamageBonus != -1 && m_pStats->m_nFuryDamageBonus <= 5)
            m_pStats->m_nFuryDamageBonus++;
    }
#endif

    ResolveRangedAnimations(pTarget, nTime, bSpecialAnimation);

    if (bForceResult != 0) {
        pAttack->m_bForcedAnimation = 1;
        pAttack->m_nForcedAnimation = nForcedAnimation;
    }
}

// The melee half of the per-attack resolution pair walks the combat round's attacks directly, and
// carries the Force-push rider a Force-wielding attacker's first unarmed attack rolls against the
// target.
void CSWSCreature::ResolveMeleeAttack(CSWSObject* pTarget, int a, int nTime, int bForceResult,
                                      int nForcedAnimation, int nForcedResult, int nDamageFlags)
{
    K2_UNUSED(a);
    if (pTarget == 0)
        return;

    CSWSCombatAttackData* pAttack = m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
    int nTotalAttacks = static_cast<unsigned char>(m_pCombatRound->GetTotalAttacks());

#if !KOTOR1
    int nTotalDamage = 0;
#endif
    for (int i = 0; i < nTotalAttacks; i++) {
        pAttack->m_targetObjectId = pTarget->m_objectId;
        pAttack->m_nWeaponAttackType = static_cast<unsigned char>(m_pCombatRound->GetWeaponAttackType());
        m_pCombatRound->SetOffHandAttack(i >= static_cast<unsigned char>(m_pCombatRound->m_nOnHandAttacks));
#if KOTOR1  // K1: coup de grace, a feat-gated special attack, no Force push or Fury.
        ResolveSituationalModifiers(pTarget->AsSWSCreature());
#else
        pTarget->AsSWSCreature();
#endif
        ResolveAttackRoll(pTarget, bForceResult, nForcedResult);
        int nAnimation = g_pAppManager->m_pClientExoApp->GetClientMeleeAnimation(
            m_objectId, pTarget->m_objectId, static_cast<unsigned short>(m_nAnimation), pAttack->m_nFeatId, bForceResult,
            nForcedAnimation);
        int nImpactTime = GetWeaponImpactTime(nAnimation, i);
#if KOTOR1
        if (pAttack->m_nFeatId != 0)
            ResolveMeleeSpecialAttack(i, 0, pTarget, 0);
#else
        int nSpecialResult = 0;
        ResolveMeleeSpecialAttack(i, 0, pTarget, 0, nSpecialResult);

        if (static_cast<unsigned char>(pAttack->m_nAttackResult - 1) < 3) {
            CSWSCreature* pVictim = pTarget->AsSWSCreature();
            if (pVictim != 0 && nSpecialResult == 0
                && (pAttack->m_nWeaponAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_UNARMED
                    || pAttack->m_nWeaponAttackType == CSWSCombatRound::WEAPON_ATTACK_TYPE_UNARMED_EXTRA)
                && i == 0) {
                // The three Force-power ranks that grant the unarmed push, best first; the value
                // each one selects is the percentage chance the push actually lands.
                int nPushChance;
                if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_ECHANI_STRIKE_III) != 0) {
                    nPushChance = 20;
                } else if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_ECHANI_STRIKE_II) != 0) {
                    nPushChance = 15;
                } else if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_ECHANI_STRIKE_I) != 0) {
                    nPushChance = 10;
                } else {
                    goto noPush;
                }
                {
                    CGameEffect* pEffect = new CGameEffect(1);
                    pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_FORCEPUSHED);
                    pEffect->m_durationTypeBits = CGameEffect::DURATION_TYPE_TEMPORARY
                                                  | (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
                    pEffect->m_fDuration = 0.1f;
                    pEffect->SetCreator(m_objectId);
                    CSWSCreature* pPushed = pTarget->AsSWSCreature();
                    unsigned char nLevel = m_pStats->GetLevel(0);
                    if (pPushed->m_pStats->GetEffectLinkImmunity(pEffect) == 0
                        && pPushed->SavingThrowRoll(SAVING_THROW_FORT, nLevel + 15, SAVING_THROW_TYPE_NONE, m_objectId, 0, 0, 1) == 0
                        && rand_wincompatible() % 100 < nPushChance) {
                        pAttack->m_effects.Add(pEffect);
                        pAttack->m_effect.m_nFeedbackEffectType = EFFECT_STATE_KNOCKDOWN;
                    } else {
                        delete pEffect;
                    }
                    pAttack->m_effect.m_bHasApplicationData = 1;
                }
            }
        }
    noPush:
#endif

        if (static_cast<unsigned char>(pAttack->m_nAttackResult - 1) < 3) {
            ResolveDamage(pTarget, bForceResult, nDamageFlags);
#if !KOTOR1
            CSWSCombatAttackData* pDamaged =
                m_pCombatRound->GetAttack(m_pCombatRound->m_currentAttack);
            if (pDamaged->GetBaseDamage() > 0)
                nTotalDamage += pDamaged->GetBaseDamage();
#endif
            ResolvePostMeleeDamage(pTarget->AsSWSCreature());
        }

        pAttack->ResolveAttackToMeleeSubAttack(i, nImpactTime);

        SetAttackResult(pAttack->m_nAttackResult);
    }

#if !KOTOR1  // K1: no Fury
    if (nTotalDamage >= pTarget->GetCurrentHitPoints(0)) {
        if (m_pStats->m_nFuryDamageBonus != -1 && m_pStats->m_nFuryDamageBonus <= 5)
            m_pStats->m_nFuryDamageBonus++;
    }
#endif

    m_nCurrentCombatFeat = pAttack->m_nFeatId;
    ResolveMeleeAnimations(0, 0, pTarget, nTime, bForceResult);

    if (bForceResult != 0) {
        pAttack->m_bForcedAnimation = 1;
        pAttack->m_nForcedAnimation = nForcedAnimation;
    }
}

// Rolls one saving throw for this creature and records the whole breakdown: the base save from
// the stats block, the effect total, the room's force rating signed by the saver's alignment,
// and the wounded-underdog feat ladder, capped at 60 against a d20.
unsigned char CSWSCreature::SavingThrowRoll(unsigned char nSave, unsigned short nDC,
                                            unsigned char nSaveVs, unsigned long long oidSource,
                                            int a, unsigned short b, int bQueueToAttack)
{
    K2_UNUSED(a); K2_UNUSED(b);
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;

    char cSaveBase;
    if (nSave == SAVING_THROW_FORT)
        cSaveBase = m_pStats->GetFortSavingThrow(1);
    else if (nSave == SAVING_THROW_REFLEX)
        cSaveBase = m_pStats->GetReflexSavingThrow(1);
    else if (nSave == SAVING_THROW_WILL)
        cSaveBase = m_pStats->GetWillSavingThrow(1);
    else
        return 0;

    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidSource);
    int nOut = 0;
    int nBonus;
    CSWSCreature* pSource = 0;
    if (pObject != 0) {
        nBonus = GetTotalEffectBonus(EFFECT_BONUS_SAVING_THROW, pObject->AsSWSObject(), nOut, 0, 0, nSave,
                                     nSaveVs,
                                     EFFECT_BONUS_ANY, EFFECT_BONUS_ANY, 0, 0);
        if (pObject->AsSWSCreature() != 0)
            pSource = pObject->AsSWSCreature();
    } else {
        nBonus = GetTotalEffectBonus(EFFECT_BONUS_SAVING_THROW, 0, nOut, 0, 0, nSave, nSaveVs,
                                     EFFECT_BONUS_ANY, EFFECT_BONUS_ANY, 0, 0);
    }

#if KOTOR1  // K1: no room force rating or feat terms; the effect bonus is capped at 20.
    if (nBonus > 20)
        nBonus = 20;
#else
    int nForceRating = 0;
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    if (pArea != 0)
        nForceRating = pArea->GetRoomForceRating(m_position);
    int nAlignmentAdjust = m_pStats->m_nAlignment < 41 ? -nForceRating : 0;
    if (m_pStats->m_nAlignment >= 60)
        nAlignmentAdjust = nForceRating;
    nBonus += nAlignmentAdjust;

    if (m_pStats->HasFeat(FEAT_SURVIVAL) != 0) {
        float fPercent = static_cast<float>(GetCurrentHitPoints(0)) / static_cast<float>(GetMaxHitPoints(1)) * 100.0f;
        if (fPercent < 50.0f && fPercent >= 40.0f)
            nBonus += 1;
        else if (fPercent < 40.0f && fPercent >= 30.0f)
            nBonus += 2;
        else if (fPercent < 30.0f && fPercent >= 20.0f)
            nBonus += 3;
        else if (fPercent < 20.0f && fPercent >= 10.0f)
            nBonus += 4;
        else if (fPercent < 10.0f && fPercent >= 0.0f)
            nBonus += 5;
    }
    if (nBonus > 60)
        nBonus = 60;
#endif

    unsigned short nRoll = g_pRules->RollDice(1, 20);

    CSWCCMessageData* pData = new CSWCCMessageData;
    pData->m_nType = CSWCCMessageData::MESSAGE_TYPE_SAVING_THROW;
    pData->SetInteger(0, nSave);
    pData->SetInteger(1, nSaveVs);
    pData->SetInteger(2, nRoll);
    pData->SetInteger(3, nBonus);
    pData->SetInteger(4, cSaveBase);
    pData->SetInteger(5, nDC);
    pData->SetInteger(6, static_cast<int>(m_objectId));
#ifdef K2_FIX_ENGINE_BUGS
    if (bQueueToAttack != 0 && pSource != 0) {
#else
    if (bQueueToAttack != 0) {
#endif
        pSource->m_pCombatRound->GetAttack(pSource->m_pCombatRound->m_currentAttack)->
            m_messages.Add(pData);
    } else {
        if (pServer->GetClientObjectByObjectId(m_objectId) != 0)
            BroadcastSavingThrowData(pData);
        if (pSource != 0 && m_objectId != oidSource)
            pSource->BroadcastSavingThrowData(pData);
        delete pData;
    }

    int nTotal = nBonus + cSaveBase + nRoll;

    CSWSCombatAttackData* pAttack = 0;
    if (pSource != 0) {
        pAttack = pSource->m_pCombatRound->GetAttack(pSource->m_pCombatRound->m_currentAttack);
        pAttack->m_effect.m_nSaveType = nSave;
        pAttack->m_effect.m_nSaveVsType = nSaveVs;
        pAttack->m_effect.m_nSaveRoll = static_cast<unsigned char>(nRoll);
        pAttack->m_effect.m_nSaveEffectBonus = static_cast<unsigned char>(nBonus);
        pAttack->m_effect.m_nSaveBase = cSaveBase;
        pAttack->m_effect.m_nSaveTotal = static_cast<unsigned char>(nTotal);
        pAttack->m_effect.m_nSaveDC = static_cast<unsigned char>(nDC);
    }

    if (nTotal >= nDC) {
        if (pAttack != 0)
            pAttack->m_effect.m_nSaveResult = 1;
        return 1;
    }

    switch (nSaveVs) {
    case SAVING_THROW_TYPE_DEATH: {
        if (m_nImpactSpellId == -1)
            break;
        CSWSpell* pSpell = g_pRules->GetSpell(m_nImpactSpellId);
        if (pSpell == 0)
            break;
        if (static_cast<unsigned char>(pSpell->m_nUserType - 1) > 1)
            break;
        if (m_pStats->GetEffectImmunity(
                static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_DEATH_MAGIC), pSource) != 0) {
            if (pAttack != 0)
                pAttack->m_effect.m_nSaveResult = 2;
            return 2;
        }
        break;
    }
    case SAVING_THROW_TYPE_DISEASE:
        if (m_pStats->GetEffectImmunity(
                static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_DISEASE), pSource) != 0) {
            if (pAttack != 0)
                pAttack->m_effect.m_nSaveResult = 2;
            return 2;
        }
        break;
    case SAVING_THROW_TYPE_FEAR:
        if (m_pStats->GetEffectImmunity(
                static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_FEAR), pSource) != 0) {
            if (pAttack != 0)
                pAttack->m_effect.m_nSaveResult = 2;
            return 2;
        }
        break;
    case SAVING_THROW_TYPE_MIND_AFFECTING:
        if (m_pStats->GetEffectImmunity(
                static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_MIND_AFFECTING), pSource) != 0) {
            if (pAttack != 0)
                pAttack->m_effect.m_nSaveResult = 2;
            return 2;
        }
        break;
    case SAVING_THROW_TYPE_POISON:
        if (m_pStats->GetEffectImmunity(
                static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_POISON), pSource) != 0) {
            if (pAttack != 0)
                pAttack->m_effect.m_nSaveResult = 2;
            return 2;
        }
        break;
    case SAVING_THROW_TYPE_TRAP:
        if (m_pStats->GetEffectImmunity(
                static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_MINE), pSource) != 0) {
            if (pAttack != 0)
                pAttack->m_effect.m_nSaveResult = 2;
            return 2;
        }
        break;
    }
    if (pAttack != 0)
        pAttack->m_effect.m_nSaveResult = 0;
    return 0;
}

// Executes the combat action, creature-only. It does no combat itself: it pumps one entry off the
// creature's own combat round and re-expresses it as an ordinary action-layer request, so every arm
// that queues something returns ACTION_IN_DELAY and every arm that only pauses the round returns
// ACTION_IN_PROGRESS.
int CSWSCreature::AIActionCombat(CSWSObjectActionNode* pNode)
{
    if (m_pCombatRound->m_bRoundPaused != 0)
        return ACTION_IN_PROGRESS;
    if (g_pAppManager->m_pServerExoApp->GetPauseState(2) != 0)
        return ACTION_IN_PROGRESS;

    if (m_pCombatRound->m_pCurrentAction != 0) {
        delete m_pCombatRound->m_pCurrentAction;
        m_pCombatRound->m_pCurrentAction = 0;
    }

    if (m_pCombatRound->GetActionPending() != 0 && m_pCombatRound->m_nCastsThisRound <= 0) {
        pNode->m_args.m_bCombatPending = 0;

        CSWSCombatRoundAction* pAction = m_pCombatRound->GetAction();
        if (pAction == 0)
            return ACTION_IN_PROGRESS;

        unsigned short nAnimation = pAction->m_nAnimation;
        unsigned long long oidTarget = pAction->m_oidTarget;
        int nAnimationTime = pAction->m_nAnimationTime;
        int nNumAttacks = pAction->m_nNumAttacks;
        unsigned char nActionType = pAction->m_nActionType;
        unsigned short nFeat = static_cast<unsigned short>(pAction->m_nFeatId);
        int nHasAttackPoint = pAction->m_bCutsceneAttack;

        m_pCombatRound->m_pCurrentAction = pAction;
#if !KOTOR1
        m_pCombatRound->m_nExecutingActionType = nActionType;
#endif
        m_pCombatRound->SetCurrentAction(nActionType);

        // Entering combat also switches the creature's activity to the fighting flavour, except
        // for the two action types that are themselves a cast (10 == item cast, 13 == stance),
        // which take it from the spell instead, and except for the six spell ids exempted below.
#if KOTOR1  // K1: every action type but the item cast sets the fighting activity.
        if (nActionType != CSWSCombatRoundAction::TYPE_ITEM_SPELL)
            SetActivity(ACTIVITY_STEALTH, 0, 0);
#else
        if (nActionType != CSWSCombatRoundAction::TYPE_ITEM_SPELL &&
            nActionType != CSWSCombatRoundAction::TYPE_STANCE) {
            switch (pAction->m_nSpellId) {
            case FORCE_POWER_MIND_TRICK:
            case FORCE_POWER_BEAST_TRICK:
            case FORCE_POWER_BEAST_CONFUSION:
            case FORCE_POWER_CONFUSION:
            case FORCE_POWER_DROID_TRICK:
            case FORCE_POWER_DROID_CONFUSION:
                break;
            default:
                SetActivity(ACTIVITY_STEALTH, 0, 0);
                break;
            }
        }
#endif

        switch (nActionType) {
        case CSWSCombatRoundAction::TYPE_ATTACK:
        case CSWSCombatRoundAction::TYPE_FEAT_ATTACK: {
            // The world time is read and both out-params are discarded.
            unsigned int nNow = 0;
            unsigned int nTod = 0;
            g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nNow, &nTod);
            if (nHasAttackPoint != 0)
                AddAttackActions(oidTarget, 0, 0, 0, 0, 1, 1, nActionType, nAnimation,
                                 nAnimationTime, nNumAttacks, 1, pAction->m_nCutsceneAnimation,
                                 pAction->m_nCutsceneAttackResult, pAction->m_nCutsceneDamage);
            else
                AddAttackActions(oidTarget, nFeat, 0, 0, 0, 1, 1, nActionType, nAnimation,
                                 nAnimationTime, nNumAttacks, 0, 0, 4, 0);
            return ACTION_IN_DELAY;
        }
        case CSWSCombatRoundAction::TYPE_REACTION:
            m_pCombatRound->SetRoundPaused(1, m_objectId);
            m_pCombatRound->SetPauseTimer(nAnimationTime, 0);
            return ACTION_IN_PROGRESS;
        case CSWSCombatRoundAction::TYPE_EQUIP:
            m_pCombatRound->SetRoundPaused(1, m_objectId);
            m_pCombatRound->SetPauseTimer(nAnimationTime, 0);
            RunEquip(oidTarget, pAction->m_nInventorySlot, pAction->m_nActionParam);
            return ACTION_IN_PROGRESS;
        case CSWSCombatRoundAction::TYPE_UNEQUIP:
            m_pCombatRound->SetRoundPaused(1, m_objectId);
            m_pCombatRound->SetPauseTimer(nAnimationTime, 0);
            RunUnequip(oidTarget, pAction->m_oidTargetRepository, 1, 0, pAction->m_nActionParam, 0);
            return ACTION_IN_PROGRESS;
        case CSWSCombatRoundAction::TYPE_CAST_SPELL:
            AddCastSpellActions(pAction->m_nSpellId, pAction->m_nClassIndex, pAction->m_nDomainLevel,
                                pAction->m_nMetaMagic, pAction->m_bSkipReadyCount, pAction->m_vTargetPosition,
                                pAction->m_oidSpellTarget, pAction->m_bIsPC, pAction->m_bNoTarget, 1,
                                pAction->m_bProjectile, pAction->m_nPathType, pAction->m_nActionParam,
                                pAction->m_nFeatId, pAction->m_nReadyKind, 1);
            break;
#if !KOTOR1
        case CSWSCombatRoundAction::TYPE_STANCE:
            AddStanceActions(nNumAttacks, oidTarget, 0, 0, 1, 1);
            return ACTION_IN_DELAY;
#endif
        case CSWSCombatRoundAction::TYPE_ITEM_SPELL: {
            CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(pAction->m_oidSpellTarget);
            if (pObject != 0 && pObject->AsSWSObject() != 0 && pObject->AsSWSCreature() != 0
                && pObject->AsSWSCreature()->GetDead() != 0)
                return ACTION_IN_PROGRESS;

            CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(pAction->m_oidItem);
            if (pItem != 0) {
                CSWItemProperty* pProperty = pItem->GetActiveProperty(pAction->m_nActivePropertyIndex);
                if (pProperty != 0) {
                    int nSpellId = pProperty->m_subType;
                    int nForceHostile = 0;
                    g_pRules->m_p2DArrays->m_pSpells->GetINTEntry(
                        nSpellId, CExoString("FORCEHOSTILE"), &nForceHostile);
#if KOTOR1  // K1: no grenade spell-id conversion; any non-hostile cast on another target drops it.
                    CSWSpell* pSpell = g_pRules->GetSpell(nSpellId);
                    if (pSpell != 0 && pSpell->m_bHostileActivity != 0)
                        SetActivity(ACTIVITY_STEALTH, 0, 0);
                    if (nForceHostile <= 0 && pAction->m_oidSpellTarget != m_objectId)
                        pAction->m_oidSpellTarget = OBJECT_INVALID;
#else
                    CSWSpell* pSpell =
                        g_pRules->GetSpell(ConvertGrenadeSpellID(nSpellId));
                    if (pSpell != 0 && pSpell->m_bHostileActivity != 0)
                        SetActivity(ACTIVITY_STEALTH, 0, 0);
                    // The spell pointer is re-dereferenced here without the null guard the
                    // statement above uses, so a grenade whose converted spell id misses the array
                    // faults when the force-hostile column is not positive and the target is not the
                    // caster.
                    if (nForceHostile <= 0 && pAction->m_oidSpellTarget != m_objectId
#ifdef K2_FIX_ENGINE_BUGS
                        && pSpell != 0
#endif
                        && pSpell->m_bHostileActivity != 0
                        && static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType != ITEM_TYPE_ROCKET
                        && static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType != ITEM_TYPE_GRENADE)
                        pAction->m_oidSpellTarget = OBJECT_INVALID;
#endif
                }
            }
            AddItemCastSpellActions(pAction->m_oidItem, pAction->m_nActivePropertyIndex, pAction->m_nSubPropertyIndex,
                                    pAction->m_vTargetPosition, pAction->m_oidSpellTarget,
                                    pAction->m_bIsPC, pAction->m_bNoTarget, 1, 1);
            break;
        }
        case CSWSCombatRoundAction::TYPE_CUTSCENE_MOVE: {
            unsigned long long oidArea = static_cast<CSWSArea*>(GetArea())->m_objectId;
            CGameObject* pTarget =
                g_pAppManager->m_pServerExoApp->GetGameObject(pAction->m_oidTarget);
            if (pTarget != 0 && pTarget->AsSWSObject() != 0
                && pAction->m_oidTarget != OBJECT_INVALID) {
                // A live target object: chase it, and let the move stop at the object's own extents.
                float fSpeed = 0.2f;
                int nOne = 1;
                int nZeroA = 0;
                int nZeroB = 0;
                AddActionToFront(AIACTION_CHECK_MOVE_TO_OBJECT_RADIUS, pNode->m_nGroupId,
                                 ACTION_PARAM_OBJECT, &pAction->m_oidTarget,
                                 ACTION_PARAM_INT, &pAction->m_nActionParam,
                                 ACTION_PARAM_FLOAT, &fSpeed, ACTION_PARAM_FLOAT, &fSpeed,
                                 ACTION_PARAM_INT, &nOne, ACTION_PARAM_INT, &nZeroA,
                                 ACTION_PARAM_INT, &nZeroB, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                Vector vZero;
                AddMoveToPointActionToFront(pNode->m_nGroupId, pTarget->AsSWSObject()->m_position,
                                            oidArea, pAction->m_oidTarget, pAction->m_nActionParam,
                                            0.2f, 0.1f, 0, 0, 0, 0, 0, 0, vZero, 0, 0, 0);
            } else {
                // No target object (or the id is the invalid sentinel): chase the stored point.
                unsigned long long oidInvalid = OBJECT_INVALID;
                int nZero = 0;
                float fSpeed = 0.2f;
                AddActionToFront(AIACTION_CHECK_MOVE_TO_POINT_RADIUS, pNode->m_nGroupId,
                                 ACTION_PARAM_FLOAT, &pAction->m_vTargetPosition.x,
                                 ACTION_PARAM_FLOAT, &pAction->m_vTargetPosition.y,
                                 ACTION_PARAM_FLOAT, &pAction->m_vTargetPosition.z,
                                 ACTION_PARAM_OBJECT, &oidArea, ACTION_PARAM_OBJECT, &oidInvalid,
                                 ACTION_PARAM_INT, &pAction->m_nActionParam,
                                 ACTION_PARAM_FLOAT, &fSpeed, ACTION_PARAM_INT, &nZero, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0);
                Vector vZero;
                AddMoveToPointActionToFront(pNode->m_nGroupId, pAction->m_vTargetPosition,
                                            oidArea, OBJECT_INVALID, pAction->m_nActionParam,
                                            0.2f, 0.1f, 0, 0, 0, 0, 0, 0, vZero, 0, 0, 0);
            }
            break;
        }
#if !KOTOR1
        case CSWSCombatRoundAction::TYPE_SWITCH_WEAPONS:
            AsSWSObject()->AddSwitchWeaponsAction(1);
            return ACTION_IN_DELAY;
#endif
        default:
            return ACTION_IN_PROGRESS;
        }

        if (m_pCombatRound->m_pCurrentAction != 0)
            delete m_pCombatRound->m_pCurrentAction;
        m_pCombatRound->m_pCurrentAction = 0;
        return ACTION_IN_DELAY;
    }

    // This second GetActionPending() call is dead: the callee is a pure query, so the call has no
    // effect.
    m_pCombatRound->GetActionPending();
    if (m_pCombatRound->m_bRoundStarted == 0)
        return ACTION_COMPLETE;
    return ACTION_IN_PROGRESS;
}

// Does the combat-round step, the repositioning shuffle a creature does between attacks - close in,
// back off, or strafe a quarter-circle left or right around its target - and the animation that
// plays while it does. Step kinds 0 and 1 pick the direction from the current separation; 2..5 are
// explicit requests that map onto the four step animations.
void CSWSCreature::DoCombatStep(unsigned char nStep, int nUnused, unsigned long long oidTarget)
{
    K2_UNUSED(nUnused);
    Vector vStep;

    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    if (pObject == 0) goto pauseRound;
    if (pObject->AsSWSObject() == 0) goto pauseRound;
    if (GetArea() == 0) goto pauseRound;
    {
        CSWSObject* pTarget = pObject->AsSWSObject();
        if ((m_nAIState & AI_STATE_MOBILE) == 0) goto pauseRound;

        float fDist = magnitude(m_position - pTarget->m_position);
        Vector vDir = normalize(m_position - pTarget->m_position);

        float fStep = 0.8f;
        float fRange = 0.0f;
        int   nAnim;

        switch (nStep) {
        case 0:
        case 1: {
            fRange = DesiredAttackRange(oidTarget, 0);
            float fTargetRange;
            if (pTarget->AsSWSCreature() != 0) {
                // The (m_objectId, 0) call's result is discarded by the call that follows.
                pTarget->AsSWSCreature()->DesiredAttackRange(m_objectId, 0);
                fTargetRange = pTarget->AsSWSCreature()->DesiredAttackRange(m_objectId, 1);
            } else {
                fTargetRange = 1.0f;
            }
            int bRanged = RangeWeaponEquipped();
            if ((fDist < fRange - 0.25f && bRanged == 0) ||
                (fDist < fTargetRange && bRanged == 1)) {
                nAnim = SERVER_ANIMATION_COMBAT_STEP_BACKWARD;
            } else {
                if (fDist > fRange + 0.25f) {
                    nAnim = SERVER_ANIMATION_COMBAT_STEP_FORWARD;
                } else {
                    if (nStep == 0) goto pauseRound;
                    nAnim = SERVER_ANIMATION_COMBAT_STEP_RIGHT ^ (rand_wincompatible() & 1);
                }
                if (bRanged == 1 && nAnim != SERVER_ANIMATION_COMBAT_STEP_BACKWARD) goto pauseRound;
            }
            break;
        }
        case 2: nAnim = SERVER_ANIMATION_COMBAT_STEP_FORWARD; break;
        case 3: nAnim = SERVER_ANIMATION_COMBAT_STEP_BACKWARD; break;
        case 4: nAnim = SERVER_ANIMATION_COMBAT_STEP_LEFT; break;
        case 5: nAnim = SERVER_ANIMATION_COMBAT_STEP_RIGHT; break;
        default: goto pauseRound;
        }

        switch (nAnim) {
        case SERVER_ANIMATION_COMBAT_STEP_FORWARD:  // close in
            if (nStep <= 1) fStep = !(fDist - fRange >= fStep) ? fDist - fRange : fStep;
            vStep = m_position - vDir * fStep;
            break;
        case SERVER_ANIMATION_COMBAT_STEP_BACKWARD: // back off
            if (nStep <= 1) fStep = !(fRange - fDist >= fStep) ? fRange - fDist : fStep;
            vStep = m_position + vDir * fStep;
            break;
        case SERVER_ANIMATION_COMBAT_STEP_LEFT: { // strafe one way around the target, keeping the separation
            float fAngle = atan2f(vDir.y, vDir.x) - 2.0f * asinf(0.4f / fDist);
            if (fAngle < 0.0f) fAngle = fAngle + 6.2831855f;
            vStep.x = cosf(fAngle);
            vStep.y = sinf(fAngle);
            // vStep.z is still the zero from the top of the body, so this is a pure XY arc.
            vStep = pTarget->m_position + vStep * fDist;
            break;
        }
        case SERVER_ANIMATION_COMBAT_STEP_RIGHT: { // ...and the other way
            float fAngle = atan2f(vDir.y, vDir.x) + 2.0f * asinf(0.4f / fDist);
            // The wrap is 2pi - a, not a - 2pi, so an overshoot mirrors the bearing instead of
            // normalising it.
            if (fAngle > 6.2831855f) fAngle = 6.2831855f - fAngle;
            vStep.x = cosf(fAngle);
            vStep.y = sinf(fAngle);
            vStep = pTarget->m_position + vStep * fDist;
            break;
        }
        }

        float fPersonalSpace = m_pPathfindInfo->m_fCrePersonalSpace;
        float fHeight        = m_pPathfindInfo->m_fHeight;
        static_cast<CSWSArea*>(GetArea())->m_pActivePathfindInfo = m_pPathfindInfo;
        unsigned long long oidSavedTarget = m_pPathfindInfo->m_oidGoalObject;
        m_pPathfindInfo->m_oidGoalObject = OBJECT_INVALID;
        int nClear = static_cast<CSWSArea*>(GetArea())->TestDirectLine(m_position, vStep, fPersonalSpace, fHeight, 0, 0);
        m_pPathfindInfo->m_oidGoalObject = oidSavedTarget;

        // Keep the step inside the area's tile grid (10 metres per tile).
        if (vStep.x < 0.0f) goto pauseRound;
        if (vStep.x > static_cast<float>(static_cast<CSWSArea*>(GetArea())->m_nWidth) * 10.0f) goto pauseRound;
        if (vStep.y < 0.0f) goto pauseRound;
        if (vStep.y > static_cast<float>(static_cast<CSWSArea*>(GetArea())->m_nHeight) * 10.0f) goto pauseRound;
        if (nClear != 1) goto pauseRound;

        UpdateSubareasOnMoveTo(m_position, vStep, 0, 0, 0);
        SetPosition(vStep, 1, 1, 0);
        return;
    }

pauseRound:
    m_pCombatRound->SetRoundPaused(0, OBJECT_INVALID);
    m_pCombatRound->SetPauseTimer(0, 0);
    SetAnimation(SERVER_ANIMATION_READY);
}
