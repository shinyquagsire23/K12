// CSWSObject's AI slice, holding the action-queue dispatcher and the AIAction* executors it
// dispatches to, with the action-id classification and queue-building helpers that serve them.

#include "nwsobject.h"
#include "nwscreature.h"
#include "appmanager.h"
#include "serverexoapp.h"
#include "nwssendplayermessage.h"
#include "gameobjectarray.h"
#include "nwsitem.h"
#include "nwscript.h"
#include "worldtimer.h"
#include "exolocstringinternal.h"
#include "exotlktable.h"
#include "clientexoapp.h"
#include "nwsarea.h"
#include "serverexoappinternal.h"
#include "factionmanager.h"
#include "nwsspellimpact.h"
#include "nwscombatround.h"
#include "nwsplaceable.h"
#include "nwsplayer.h"
#include "exolinkedlistinternal.h"
#include "guiingame.h"
#include "CExoArrayListImpl.h"
#include "nwrules.h"
#include "nwitem.h"
#include "serveraimaster.h"
#include "nwsdoor.h"
#include "nwsscriptevent.h"
#include "nwmessage.h"
#include "nwsitemproperty.h"
#include "partytables.h"

#include "exobase.h"

#include "clientoptions.h"
#include "swparty.h"
#include "nwcmodule.h"
#include "pathfindinfo.h"
#include "nwscriptconst.h"

#include "nwseffect.h"

#include "nwccreature.h"

#include "exoinput.h"
#include "nwseffectlist.h"
#include "nwsaiaction.h"
#include "serveranimation.h"
#include "soundprioritygroup.h"
#include "strref.h"

// The overflow watchdog's limits: past the first an action queue is reported, past the second
// it is erased down to what the object will give up, past the third erased outright; an effect
// list is reported past the first effect limit and its temporary and permanent effects culled
// past the second.
enum {
    ACTION_LIST_REPORT_LIMIT = 75,
    ACTION_LIST_ERASE_LIMIT  = 500,
    ACTION_LIST_HARD_LIMIT   = 1000,
    EFFECT_LIST_REPORT_LIMIT = 500,
    EFFECT_LIST_ERASE_LIMIT  = 1000,
};

// The highest lock DC a take-20 failure still reports as merely failed; above it the lock is
// reported as impossible.
enum {
    LOCK_DC_POSSIBLE_MAX = 59,
};

// animations.2da row of a placeable's open transition ("close2open").
enum {
    ANIMATION_ROW_CLOSE2OPEN = 312,
};

int RenderAi;

int OverrideReputation = -1;

#include "port/crtapi.h"

float DegToRad(float degrees);

extern int quaternionidentityoptimization;

// Rotate v by q, skipping the matrix build when the rotation is a pure sign flip.
static inline Vector RotateByQuaternion(const Vector& v, const Quaternion& q)
{
    float w = q.w;
    if (quaternionidentityoptimization != 0 && (w == 1.0f || w == -1.0f))
        return v;

    float x = q.x, y = q.y, z = q.z;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = x * w, wy = y * w, wz = z * w;
    Vector r;
    r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) + v.z * ((xz + wy) + (xz + wy));
    r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) + v.z * ((yz - wx) + (yz - wx));
    r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) + v.z * (1.0f - ((xx + yy) + (xx + yy)));
    return r;
}

extern int RenderCombat;
extern int g_bShowIndicator;
#include "nwcobject.h"
#include "exosound.h"
#include "exoresman.h"
#include "nwcitem.h"

void CSWSObject::RunActions(unsigned int nDay, unsigned int nTod, unsigned long long nSliceStart) {
    int nResult = ACTION_FAILED;
    unsigned char nObjectType = m_objectType;

    if (RenderCombat != 0) {
        CExoString sQueue;
        m_sTag.CStr();
        if (AsSWSCreature() != 0) {
            AsSWSCreature();
            AsSWSCreature();
            AsSWSCreature();
            AsSWSCreature();
            AsSWSCreature();
            AsSWSCreature();
            AsSWSCreature();
            AsSWSCreature();
            if (AsSWSCreature()->m_pCombatRound->m_bRoundPaused != 0)
                AsSWSCreature();
            AsSWSCreature();
        }
        CExoLinkedListNode* pPos = m_actionList.m_pInternal->m_pHead;
        if (pPos != 0) {
            sQueue = " ";
            while (pPos != 0) {
                CExoString sEntry;
                CSWSObjectActionNode* pQueued =
                    static_cast<CSWSObjectActionNode*>(m_actionList.GetAtPos(pPos));
                sEntry.Format(const_cast<char*>(" %d(%d)"), pQueued->m_nActionId, pQueued->m_nGroupId);
                sQueue = sQueue + sEntry;
                m_actionList.GetNext(pPos);
            }
            sQueue.CStr();
        }
    }

    if (m_actionList.m_pInternal->m_pHead != 0) {
        if (g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId) ==
                g_pAppManager->m_pClientExoApp->GetSWParty()->m_members[0].id &&
            g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK) {
            g_bShowIndicator = 1;
            g_pAppManager->m_pClientExoApp->GetModule()->RestoreCamera();
            g_pAppManager->m_pClientExoApp->SetInputClass(0, 1);
        }
    }

    unsigned int nNowDay;
    unsigned int nNowTod;
    int nActionId = 0;
    do {
        if (m_actionList.m_pInternal->m_pHead == 0) {
            // Nothing left to do, so drop every combat/targeting latch back to none.
            if (AsSWSCreature() != 0) {
                CSWSCreature* pCreature = AsSWSCreature();
                if (pCreature->m_attemptedAttackTargetId != OBJECT_INVALID)
                    pCreature->SetAttemptedAttackTarget(OBJECT_INVALID);
                if (pCreature->m_oidAttackTarget != OBJECT_INVALID)
                    pCreature->m_oidAttackTarget = OBJECT_INVALID;
                if (pCreature->m_oidAttemptedSpellTarget != OBJECT_INVALID)
                    pCreature->m_oidAttemptedSpellTarget = OBJECT_INVALID;
                if (pCreature->m_oidSpellTarget != OBJECT_INVALID)
                    pCreature->m_oidSpellTarget = OBJECT_INVALID;
                AsSWSCreature()->m_oidAttemptedMovementTarget = OBJECT_INVALID;
                CPathfindInformation* pPath = AsSWSCreature()->m_pPathfindInfo;
                pPath->m_oidPathTarget = OBJECT_INVALID;
                pPath->m_vPathTarget.x = 0.0f;
                pPath->m_vPathTarget.y = 0.0f;
                pPath->m_vPathTarget.z = 0.0f;
            }
        } else {
            CSWSObjectActionNode* pNode =
                static_cast<CSWSObjectActionNode*>(m_actionList.RemoveHead());
            if (m_nActionStartDay == 0 && m_nActionStartTod == 0) {
                m_nActionStartDay = nDay;
                m_nActionStartTod = nTod;
            }
            m_pCurrentActionNode = pNode;
            nActionId = pNode->m_nActionId;
            m_nCurrentActionId = static_cast<unsigned short>(nActionId);
            nResult = ACTION_FAILED;
            switch (nActionId) {
            case AIACTION_MOVE_TO_POINT:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionMoveToPoint(pNode);
                break;
            case AIACTION_CHECK_MOVE_TO_OBJECT:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionCheckMoveToObject(pNode);
                break;
            case AIACTION_CHECK_MOVE_AWAY_FROM_OBJECT:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionCheckMoveAwayFromObject(pNode);
                break;
            case AIACTION_CHECK_INTER_AREA_PATHFINDING:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionCheckInterAreaPathfinding(pNode);
                break;
            case AIACTION_JUMP_TO_POINT:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionJumpToPoint(pNode);
                break;
            case AIACTION_PLAY_ANIMATION:
                nResult = AIActionPlayAnimation(pNode);
                break;
            case AIACTION_PICK_UP_ITEM:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionPickUpItem(pNode);
                break;
            case AIACTION_EQUIP_ITEM:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionEquipItem(pNode);
                break;
            case AIACTION_DROP_ITEM:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionDropItem(pNode);
                break;
            case AIACTION_CHECK_MOVE_TO_POINT:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionCheckMoveToPoint(pNode);
                break;
            case AIACTION_UNEQUIP_ITEM:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionUnequipItem(pNode);
                break;
            // The attack action publishes its victim on the object for the duration of the swing.
            case AIACTION_PHYSICAL_ATTACKS:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE) {
                    m_attackTargetId = pNode->m_args.m_attackTargetId;
                    nResult = AsSWSCreature()->AIActionPhysicalAttacks(pNode);
                    m_attackTargetId = OBJECT_INVALID;
                    if (nResult != ACTION_IN_PROGRESS)
                        g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nNowDay,
                                                                                      &nNowTod);
                }
                break;
            case AIACTION_SPEAK:
                nResult = AIActionSpeak(pNode);
                break;
            // Placeables cast spells too (mines, emitters), through their own executor.
            case AIACTION_CAST_SPELL:
                if (nObjectType == GAME_OBJECT_TYPE_PLACEABLE) {
                    nResult = AsSWSPlaceable()->AIActionCastSpell(pNode);
                } else if (nObjectType == GAME_OBJECT_TYPE_CREATURE) {
                    nResult = AsSWSCreature()->AIActionCastSpell(pNode);
                    if (nResult == ACTION_FAILED || nResult == ACTION_COMPLETE)
                        AsSWSCreature()->m_pCombatRound->GetSpellActionPending();
                }
                break;
            case AIACTION_WAIT_FOR_END_OF_ROUND:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionWaitForEndOfRound(pNode);
                break;
            case AIACTION_CHECK_MOVE_TO_OBJECT_RADIUS:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionCheckMoveToObjectRadius(pNode);
                break;
            case AIACTION_CHECK_MOVE_TO_POINT_RADIUS:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionCheckMoveToPointRadius(pNode);
                break;
            case AIACTION_CHANGE_FACING_OBJECT:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionChangeFacingObject(pNode);
                break;
            case AIACTION_OPEN_DOOR:
                nResult = AIActionOpenDoor(pNode);
                break;
            case AIACTION_CLOSE_DOOR:
                nResult = AIActionCloseDoor(pNode);
                break;
            case AIACTION_ORIENT_CAMERA:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionOrientCamera(pNode);
                break;
            case AIACTION_PLAY_SOUND:
                nResult = AIActionPlaySound(pNode);
                break;
            case AIACTION_DIALOG_OBJECT:
                nResult = AIActionDialogObject(pNode);
                break;
            case AIACTION_DISARM_MINE:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionDisarmMine(pNode);
                break;
            case AIACTION_RECOVER_MINE:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionRecoverMine(pNode);
                break;
            case AIACTION_FLAG_MINE:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionFlagMine(pNode);
                break;
            case AIACTION_EXAMINE_MINE:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionExamineMine(pNode);
                break;
            case AIACTION_SET_MINE:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionSetMine(pNode);
                break;
            case AIACTION_WAIT:
                nResult = AIActionWait(pNode);
                break;
            case AIACTION_PAUSE_DIALOG:
                nResult = AIActionPauseDialog(pNode);
                break;
            case AIACTION_RESUME_DIALOG:
                nResult = AIActionResumeDialog(pNode);
                break;
            case AIACTION_SPEAK_STRREF:
                nResult = AIActionSpeakStrRef(pNode);
                break;
            case AIACTION_GIVE_ITEM:
                nResult = AIActionGiveItem(pNode);
                break;
            case AIACTION_TAKE_ITEM:
                if (nObjectType == GAME_OBJECT_TYPE_PLACEABLE || nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AIActionTakeItem(pNode);
                break;
            case AIACTION_ENCOUNTER_CREATURE_DESTROY_SELF:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionEncounterCreatureDestroySelf();
                break;
            case AIACTION_DO_COMMAND:
                nResult = AIActionDoCommand(pNode);
                break;
            case AIACTION_UNLOCK_OBJECT:
                nResult = AIActionUnlockObject(pNode);
                break;
            case AIACTION_LOCK_OBJECT:
                nResult = AIActionLockObject(pNode);
                break;
            case AIACTION_USE_OBJECT:
                nResult = AIActionUseObject(pNode);
                break;
            case AIACTION_ANIMAL_EMPATHY:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionAnimalEmpathy(pNode);
                break;
            case AIACTION_REST:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionRest(pNode);
                break;
            case AIACTION_TAUNT:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionTaunt(pNode);
                break;
            case AIACTION_CHECK_MOVE_AWAY_FROM_LOCATION:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionCheckMoveAwayFromLocation(pNode);
                break;
            case AIACTION_RANDOM_WALK:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionRandomWalk(pNode);
                break;
            case AIACTION_ITEM_CAST_SPELL:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE) {
                    nResult = AsSWSCreature()->AIActionItemCastSpell(pNode);
                    if (nResult != ACTION_IN_PROGRESS) {
                        AsSWSCreature()->m_oidAttemptedSpellTarget = OBJECT_INVALID;
                        AsSWSCreature()->m_oidSpellTarget = OBJECT_INVALID;
                    }
                }
                break;
            case AIACTION_SET_COMMANDABLE:
                nResult = AIActionSetCommandable(pNode);
                break;
            case AIACTION_JUMP_TO_OBJECT:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionJumpToObject(pNode);
                break;
            case AIACTION_CHANGE_FACING_POINT:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionChangeFacingPoint(pNode);
                break;
            case AIACTION_COUNTER_SPELL:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionCounterSpell(pNode);
                break;
            case AIACTION_DRIVE:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionDrive(pNode);
                break;
            case AIACTION_APPEAR:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionAppear(pNode);
                break;
            case AIACTION_DISAPPEAR:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionDisappear(pNode);
                break;
            case AIACTION_PICK_POCKET:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionPickPocket(pNode);
                break;
            case AIACTION_FORCE_FOLLOW_OBJECT:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionForceFollowObject(pNode);
                break;
            case AIACTION_HEAL:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionHeal(pNode);
                break;
            case AIACTION_CHECK_FORCE_FOLLOW_OBJECT:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionCheckForceFollowObject(pNode);
                break;
            case AIACTION_AREA_WAIT:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionAreaWait(pNode);
                break;
            // Party-follow only means anything once there is a player creature; before that the action
            // reports "still working" so the queue keeps it.
            case AIACTION_PARTY_FOLLOW_LEADER:
                if (g_pAppManager->m_pClientExoApp->GetPlayerCreatureId() == OBJECT_INVALID) {
                    nResult = ACTION_IN_PROGRESS;
                    break;
                }
                if (AsSWSCreature() != 0)
                    AsSWSCreature();
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE && AsSWSCreature() != 0 &&
                    AsSWSCreature()->m_bInParty != 0)
                    nResult = AsSWSCreature()->AIActionPartyFollowLeader(pNode);
                break;
            case AIACTION_BARK_STRING:
                nResult = AIActionBarkString(pNode);
                break;
            case AIACTION_COMBAT:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionCombat(pNode);
                break;
            case AIACTION_CHECK_MOVE_TO_FOLLOW_RADIUS:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE && AsSWSCreature() != 0 &&
                    AsSWSCreature()->m_bInParty != 0)
                    nResult = AsSWSCreature()->AIActionCheckMoveToFollowRadius(pNode);
                break;
            case AIACTION_SURRENDER_TO_ENEMIES:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionSurrenderToEnemies(pNode);
                break;
#if !KOTOR1  // K1's action ids end at 65
            case AIACTION_DOOR_SABER:
                nResult = AIActionDoorSaber(pNode);
                break;
            case AIACTION_STANCE_TOTAL_DEFENSE:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionStanceTotalDefense(pNode);
                break;
            case AIACTION_STANCE_MEDITATIVE:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE)
                    nResult = AsSWSCreature()->AIActionStanceMeditative(pNode);
                break;
            // Only a puppet follows an owner.
            case AIACTION_FOLLOW_OWNER:
                if (nObjectType == GAME_OBJECT_TYPE_CREATURE && AsSWSCreature() != 0 &&
                    AsSWSCreature()->m_isPuppet != 0)
                    nResult = AsSWSCreature()->AIActionFollowOwner(pNode);
                break;
            case AIACTION_SWITCH_WEAPONS:
                nResult = AIActionSwitchWeapons(pNode);
                break;
#endif
            }

            // The second half of the combat-debug dump covers our own round, then our victim's.
            if (RenderCombat != 0 && AsSWSCreature() != 0 && AsSWSCreature()->m_bInParty != 0) {
                AsSWSCreature()->m_pCombatRound->DEBUGDumpCombatRoundToScreen(
                    10, AsSWSCreature()->m_sTag);
                unsigned long long oidVictim = AsSWSCreature()->m_attemptedAttackTargetId;
                if (oidVictim != OBJECT_INVALID) {
                    CGameObject* pTarget =
                        g_pAppManager->m_pServerExoApp->GetGameObject(oidVictim);
                    if (pTarget != 0 && pTarget->AsSWSCreature() != 0) {
                        CSWSCreature* pOther = pTarget->AsSWSCreature();
                        pOther->m_pCombatRound->DEBUGDumpCombatRoundToScreen(30, pOther->m_sTag);
                    }
                }
            }

            m_pCurrentActionNode = 0;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->m_nLastActionId = m_nCurrentActionId;
            m_nCurrentActionId = AIACTION_NONE;
            m_nLastAIUpdateDay = nDay;
            m_nLastAIUpdateTod = nTod;
            if (nResult == ACTION_IN_DELAY) {
                m_actionList.AddTail(pNode);
            } else if (nResult == ACTION_IN_PROGRESS) {
                m_actionList.AddHead(pNode);
            } else {
                if (GetLockOrientationToObject() != OBJECT_INVALID && nActionId != AIACTION_PHYSICAL_ATTACKS) {
                    GetLockOrientationToObject();
                    SetLockOrientationToObject(OBJECT_INVALID);
                }
                delete pNode;
                m_nActionStartDay = 0;
                m_nActionStartTod = 0;
            }
        }
        if (nResult == ACTION_IN_PROGRESS)
            break;
        if (g_pExoBase->m_pTimers->GetHighResolutionTimer() - nSliceStart >
                static_cast<unsigned int>(AI_TIME_SLICE))
            break;
        if (TerminateAISliceAfterAction(nActionId) != 0)
            break;
    } while (m_actionList.m_pInternal->m_pHead != 0);

    // Runaway-queue and runaway-effect-list watchdogs.
    if (m_actionList.m_pInternal->m_nCount > ACTION_LIST_REPORT_LIMIT) {
        ReportOverflow(FEEDBACK_ACTION_LIST_OVERFLOW, m_actionList.m_pInternal->m_nCount, ACTION_LIST_REPORT_LIMIT, 0);
        if (m_actionList.m_pInternal->m_nCount > ACTION_LIST_ERASE_LIMIT) {
            ReportOverflow(FEEDBACK_ACTION_LIST_ERASED, m_actionList.m_pInternal->m_nCount, ACTION_LIST_ERASE_LIMIT, 1);
            // Drop everything the object is willing to abandon...
            while (m_actionList.m_pInternal->m_pHead != 0) {
                CSWSObjectActionNode* pDrop =
                    static_cast<CSWSObjectActionNode*>(m_actionList.RemoveHead());
                if (ClearAction(pDrop, 1) != 1) {
                    m_actionList.AddHead(pDrop);
                    break;
                }
                delete pDrop;
            }
            // ...and if that is not enough, drop the rest regardless.
            if (m_actionList.m_pInternal->m_nCount > ACTION_LIST_HARD_LIMIT) {
                while (m_actionList.m_pInternal->m_pHead != 0)
                    delete static_cast<CSWSObjectActionNode*>(m_actionList.RemoveHead());
            }
        }
    }
    if (m_effects.m_count > EFFECT_LIST_REPORT_LIMIT) {
        ReportOverflow(FEEDBACK_EFFECT_LIST_OVERFLOW, m_effects.m_count, EFFECT_LIST_REPORT_LIMIT, 0);
        if (m_effects.m_count > EFFECT_LIST_ERASE_LIMIT) {
            ReportOverflow(FEEDBACK_EFFECT_LIST_ERASED, m_effects.m_count, EFFECT_LIST_ERASE_LIMIT, 1);
            RemoveSomeEffectsOfDurationType(CGameEffect::DURATION_TYPE_TEMPORARY);
            RemoveSomeEffectsOfDurationType(CGameEffect::DURATION_TYPE_PERMANENT);
        }
    }
}

int CSWSObject::AIActionSetCommandable(CSWSObjectActionNode* pNode) {
#if KOTOR1  // K1: the dying tests are GetDeadTemp() / GetIsPCDying()
    int bDying = 0;
    if (AsSWSCreature() != 0)
        bDying = AsSWSCreature()->GetDeadTemp();
    if (GetDead() == 0 && GetIsPCDying() == 0 && bDying == 0) {
#else
    int bAlive = 1;
    if (AsSWSCreature() != 0) {
        CSWSCreature* pCreature = AsSWSCreature();
        bAlive = pCreature->GetDeadTemp() == 0;
    }
    int bDead = GetDead();
    if (bAlive && bDead == 0) {
#endif
        m_bCommandable = pNode->m_args.m_bCommandable;
        return ACTION_COMPLETE;
    }
    return ACTION_FAILED;
}

int CSWSObject::AIActionSpeakStrRef(CSWSObjectActionNode* pNode) {
    if (AsSWSCreature() != 0) {
        AsSWSCreature()->ActivityManager(2);
        AsSWSCreature()->SetActivity(CSWSCreature::ACTIVITY_STEALTH, 0, 0);
    }
    unsigned int nStrRef = pNode->m_args.m_speakStrRef;
    unsigned int nVolume = pNode->m_args.m_speakVolume;
    g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerChatStrRefMessage(
        static_cast<unsigned char>(nVolume), m_objectId, nStrRef);
    return ACTION_COMPLETE;
}

int CSWSObject::AIActionDoCommand(CSWSObjectActionNode* pNode) {
#if KOTOR1  // K1: the dying tests are GetDeadTemp() / GetIsPCDying()
    int bDying = 0;
    if (AsSWSCreature() != 0)
        bDying = AsSWSCreature()->GetDeadTemp();
    if (GetDead() == 1 || GetIsPCDying() == 1 || bDying == 1)
#else
    int bDying = 0;
    if (AsSWSCreature() != 0) {
        CSWSCreature* pCreature = AsSWSCreature();
        bDying = pCreature->GetDeadTemp();
    }
    int bDead = GetDead();
    if (bDying || bDead == 1)
#endif
        return ACTION_FAILED;
    void* pSituation = pNode->m_args.m_pScriptSituation;
    g_pVirtualMachine->RunScriptSituation(pSituation, m_objectId, 1);
    pNode->m_args.m_pScriptSituation = 0;
    return ACTION_COMPLETE;
}

int CSWSObject::AIActionPauseDialog(CSWSObjectActionNode* pNode) {
    K2_UNUSED(pNode);
#if KOTOR1  // K1: the dying tests are GetDeadTemp() / GetIsPCDying()
    int bDying = 0;
    if (AsSWSCreature() != 0)
        bDying = AsSWSCreature()->GetDeadTemp();
    if (GetDead() == 1 || GetIsPCDying() == 1 || bDying == 1) {
#else
    int bDying = 0;
    if (AsSWSCreature() != 0) {
        CSWSCreature* pCreature = AsSWSCreature();
        bDying = pCreature->GetDeadTemp();
    }
    int bDead = GetDead();
    if (bDying || bDead == 1) {
#endif
        SetLockOrientationToObject(CGameObjectArray::INVALIDOBJID);
        return ACTION_FAILED;
    }
    m_bDialogPaused = 1;
    if (AsSWSCreature() != 0)
        AsSWSCreature()->m_nLockedActivityFlags |= 4;
    return ACTION_COMPLETE;
}

int CSWSObject::AIActionResumeDialog(CSWSObjectActionNode* pNode) {
    K2_UNUSED(pNode);
#if KOTOR1  // K1: the dying tests are GetDeadTemp() / GetIsPCDying()
    int bDying = 0;
    if (AsSWSCreature() != 0)
        bDying = AsSWSCreature()->GetDeadTemp();
    if (GetDead() == 1 || GetIsPCDying() == 1 || bDying == 1) {
#else
    int bDying = 0;
    if (AsSWSCreature() != 0) {
        CSWSCreature* pCreature = AsSWSCreature();
        bDying = pCreature->GetDeadTemp();
    }
    int bDead = GetDead();
    if (bDying || bDead == 1) {
#endif
        SetLockOrientationToObject(CGameObjectArray::INVALIDOBJID);
        return ACTION_FAILED;
    }
    m_bDialogPaused = 0;
    if (AsSWSCreature() != 0) {
        AsSWSCreature()->ActivityManager(2);
        AsSWSCreature()->m_nLockedActivityFlags &= ~4;
        AsSWSCreature()->SetActivity(CSWSCreature::ACTIVITY_STEALTH, 0, 0);
    }
    return ACTION_COMPLETE;
}

int CSWSObject::AIActionBarkString(CSWSObjectActionNode* pNode) {
    unsigned int nStrRef = pNode->m_args.m_barkStrRef;
    STR_RES strRes;
    g_pTlkTable->Fetch(nStrRef, strRes, CTlkTable::GENDER_MALE);
    CResRef    resSound = strRes.soundResRef;
    CExoString sText = strRes.text;
    unsigned long long oidClient =
        g_pAppManager->m_pClientExoApp->ServerToClientObjectId(m_objectId);
    g_pAppManager->m_pClientExoApp->BarkString(oidClient, sText, resSound, 0);
    return ACTION_COMPLETE;
}

int CSWSObject::AIActionWait(CSWSObjectActionNode* pNode) {
#if KOTOR1  // K1: the dying tests are GetDeadTemp() / GetIsPCDying()
    int bDying = 0;
    if (AsSWSCreature() != 0)
        bDying = AsSWSCreature()->GetDeadTemp();
    if (GetDead() == 1 || GetIsPCDying() == 1 || bDying != 0)
#else
    int bDying = 0;
    if (AsSWSCreature() != 0) {
        CSWSCreature* pCreature = AsSWSCreature();
        bDying = pCreature->GetDeadTemp();
    }
    int bDead = GetDead();
    if (bDying || bDead == 1)
#endif
        return ACTION_FAILED;
    unsigned int nWaitMs = static_cast<unsigned int>(pNode->m_args.m_waitSeconds * 1000.0f);
    unsigned int nDay, nTod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);
    unsigned int nElapsedDay, nElapsedTod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTod, m_nActionStartDay, m_nActionStartTod, &nElapsedDay, &nElapsedTod);
    if (nElapsedTod < nWaitMs)
        return ACTION_IN_PROGRESS;
    return ACTION_COMPLETE;
}

int CSWSObject::AIActionCloseDoor(CSWSObjectActionNode* pNode) {
#if KOTOR1  // K1: the dying tests are GetDeadTemp() / GetIsPCDying()
    int bDying = 0;
    if (AsSWSCreature() != 0)
        bDying = AsSWSCreature()->GetDeadTemp();
    if (GetDead() == 1 || GetIsPCDying() == 1 || bDying == 1)
#else
    int bDying = 0;
    if (AsSWSCreature() != 0) {
        CSWSCreature* pCreature = AsSWSCreature();
        bDying = pCreature->GetDeadTemp();
    }
    int bDead = GetDead();
    if (bDying || bDead == 1)
#endif
        return ACTION_FAILED;
    if (AsSWSCreature() != 0)
        AsSWSCreature()->ActivityManager(2);
    unsigned long long oidTarget;
    int nRun = 0;
    oidTarget = pNode->m_args.m_aqTargetId;
    if (pNode->m_nNumParameters >= 2 && pNode->m_args.m_closeDoorRun != 0)
        nRun = static_cast<int>(pNode->m_args.m_closeDoorRun);
    CGameObject* pObject;
    Vector vPos;
    float fRange;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget, &pObject)
            != CGameObjectArray::SUCCESS)
        return ACTION_FAILED;
    if (AsSWSCreature() != 0) {
        if (pObject->AsSWSDoor() == 0 && pObject->AsSWSPlaceable() == 0)
            return ACTION_FAILED;
        static_cast<CSWSObject*>(pObject)->GetArea();
        AsSWSCreature()->GetUseRange(oidTarget, vPos, fRange, 0, 0);
        if (GetArea() != pObject->AsSWSObject()->GetArea() ||
            AsSWSCreature()->GetIsInUseRange(oidTarget, 0.0f, 0, 0) == 0) {
            AsSWSCreature()->AddActionToFront(AIACTION_CLOSE_DOOR, ACTION_GROUP_NEW,
                                              ACTION_PARAM_OBJECT, &oidTarget,
                                              ACTION_PARAM_INT, &nRun, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            AsSWSCreature()->AddMoveToPointActionToFront(
                pNode->m_nGroupId, vPos, pObject->AsSWSObject()->m_cachedAreaId, OBJECT_INVALID,
                nRun, fRange, 0.0f, 0, 0, 0, 0, 0, 0, Vector{}, 0, 0, 0);
            return ACTION_COMPLETE;
        }
    }
    SetLockOrientationToObject(oidTarget);
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
        0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_CLOSE_OBJECT, 0);
    float fDelay = 0.5f;
    AddActionToFront(AIACTION_WAIT, pNode->m_nGroupId, ACTION_PARAM_FLOAT, &fDelay, 0, 0, 0, 0, 0,
                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return ACTION_COMPLETE;
}

int CSWSObject::AIActionOpenDoor(CSWSObjectActionNode* pNode) {
#if KOTOR1  // K1: the dying tests are GetDeadTemp() / GetIsPCDying()
    int bDying = 0;
    if (AsSWSCreature() != 0)
        bDying = AsSWSCreature()->GetDeadTemp();
    if (GetDead() == 1 || GetIsPCDying() == 1 || bDying == 1) {
#else
    int bDying = 0;
    if (AsSWSCreature() != 0) {
        CSWSCreature* pCreature = AsSWSCreature();
        bDying = pCreature->GetDeadTemp();
    }
    int bDead = GetDead();
    if (bDying || bDead == 1) {
#endif
        m_bOpenDoorPending = 0;
        return ACTION_FAILED;
    }
    if (AsSWSCreature() != 0)
        AsSWSCreature()->ActivityManager(2);
    unsigned long long oidTarget;
    int nRun = 1;
    oidTarget = pNode->m_args.m_aqTargetId;
    if (pNode->m_nNumParameters >= 2 && pNode->m_args.m_closeDoorRun != 0)
        nRun = static_cast<int>(pNode->m_args.m_closeDoorRun);
    CGameObject* pObject;
    Vector vPos;
    float fRange;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget, &pObject)
            != CGameObjectArray::SUCCESS) {
        m_bOpenDoorPending = 0;
        return ACTION_FAILED;
    }
    if (AsSWSCreature() != 0) {
        CSWSDoor* pDoor = pObject->AsSWSDoor();
        if (pDoor == 0) {
            m_bOpenDoorPending = 0;
            return ACTION_FAILED;
        }
        static_cast<CSWSObject*>(pObject)->GetArea();
        AsSWSCreature()->GetUseRange(oidTarget, vPos, fRange, 0, 0);
        if (pDoor->m_nAnimation != SERVER_ANIMATION_CLOSED)
            return ACTION_COMPLETE;
        if (GetArea() == pObject->AsSWSObject()->GetArea() &&
            AsSWSCreature()->GetIsInUseRange(oidTarget, 0.0f, 0, 0) != 0) {
            if (pDoor->m_bTrapFlag != 0 && pDoor->GetReputation(m_objectId, 0) <= CFactionManager::REPUTATION_FRIENDLY_ABOVE &&
                pDoor->m_nFaction != static_cast<unsigned int>(AsSWSCreature()->m_pStats->m_factionId)) {
                CScriptEvent* pEvent = new CScriptEvent();
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_MINE_TRIGGERED;
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
                return ACTION_FAILED;
            }
            if (AsSWSCreature()->EvaluateLock(pObject->AsSWSObject(), 0) == 0) {
                CScriptEvent* pEvent = new CScriptEvent();
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_FAIL_TO_OPEN;
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
                m_bOpenDoorPending = 0;
                return ACTION_FAILED;
            }
            SetLockOrientationToObject(oidTarget);
            if (m_bOpenDoorPending == 0) {
                AsSWSCreature()->AddActionToFront(AIACTION_OPEN_DOOR, ACTION_GROUP_NEW,
                                                  ACTION_PARAM_OBJECT, &oidTarget,
                                                  ACTION_PARAM_INT, &nRun, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                m_bOpenDoorPending = 1;
                return ACTION_COMPLETE;
            }
        } else {
            AsSWSCreature()->AddActionToFront(AIACTION_OPEN_DOOR, ACTION_GROUP_NEW,
                                              ACTION_PARAM_OBJECT, &oidTarget,
                                              ACTION_PARAM_INT, &nRun, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            if (pDoor->m_bPreciseUse != 0 && pDoor->m_bLocked != 0) {
                float fWait = 0.5f;
                AsSWSCreature()->AddActionToFront(AIACTION_WAIT, ACTION_GROUP_PREVIOUS,
                                                  ACTION_PARAM_FLOAT, &fWait, 0, 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0);
                AsSWSCreature()->AddActionToFront(AIACTION_CHANGE_FACING_OBJECT,
                                                  ACTION_GROUP_PREVIOUS,
                                                  ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0);
            }
            AsSWSCreature()->AddMoveToPointActionToFront(
                ACTION_GROUP_PREVIOUS, vPos, pObject->AsSWSObject()->m_cachedAreaId, OBJECT_INVALID, nRun, fRange,
                0.0f, 0, 0, 0, 0, 0, 0, Vector{}, 0, 0, 0);
            m_bOpenDoorPending = 1;
            return ACTION_COMPLETE;
        }
    }
    m_bOpenDoorPending = 0;
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
        0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_OPEN_OBJECT, 0);
    float fDelay = 0.5f;
    AddActionToFront(AIACTION_WAIT, pNode->m_nGroupId, ACTION_PARAM_FLOAT, &fDelay, 0, 0, 0, 0, 0,
                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return ACTION_COMPLETE;
}

int CSWSObject::AIActionDialogObject(CSWSObjectActionNode* pNode)
{
    // This early exit returns before sDialog exists, so it is the only path that skips the
    // string destructor.
#if KOTOR1  // K1: a dying PC also fails; "down" party members are GetDeadTemp()
    if (GetDead() == 1 || GetIsPCDying() == 1) {
#else
    if (GetDead() == 1) {
#endif
        SetLockOrientationToObject(CGameObjectArray::INVALIDOBJID);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalDialogState(0);
        return ACTION_FAILED;
    }

    unsigned long long oidTarget = pNode->m_args.m_dialogTargetId;
    CExoString sDialog;
    sDialog = *pNode->m_args.m_pDialogResRef;
    int nPrivate         = pNode->m_args.m_bDialogPrivate;
    int nApproachPending = pNode->m_args.m_bDialogApproachPending;
    int nIgnoreRange     = pNode->m_args.m_bDialogIgnoreStartRange;
    unsigned long long oidSpeaker = pNode->m_args.m_dialogSpeakerId;

    // Is anyone left standing? m_bInParty is the party-slot flag, so a character that is not in
    // the party at all passes trivially; one that is has to still have hit points.
    int nCharacters = g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount;
    if (nCharacters > 0) {
        for (int i = 0; i < nCharacters; i++) {
            CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
            CSWCCreature* pMember = pParty->GetCharacter(i);
            if (pMember != 0 && pMember->GetServerCreature() != 0) {
                CSWSCreature* pServer = pMember->GetServerCreature();
                if (pServer->GetDeadTemp() == 0)
                    goto partyReady;
            }
        }
        SetLockOrientationToObject(CGameObjectArray::INVALIDOBJID);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalDialogState(0);
        return ACTION_FAILED;
    }
partyReady:

    if (oidSpeaker == OBJECT_INVALID) {
        CGameObject* pTargetGO;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget, &pTargetGO)
                != CGameObjectArray::SUCCESS) {
            SetLockOrientationToObject(CGameObjectArray::INVALIDOBJID);
            g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalDialogState(0);
            return ACTION_FAILED;
        }
        CSWSObject* pTarget = pTargetGO->AsSWSObject();
        if (pTarget->m_bCommandable == 0 || pTarget->GetDead() == 1) {
            SetLockOrientationToObject(CGameObjectArray::INVALIDOBJID);
            g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalDialogState(0);
            return ACTION_FAILED;
        }

        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
        CSWSCreature* pPlayer = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
        if (pPlayer == 0)
            return ACTION_IN_PROGRESS;
        CGameObject* pPlayerClient = g_pAppManager->m_pClientExoApp->GetGameObject(
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(pPlayer->m_objectId));
        if (pPlayerClient == 0)
            return ACTION_IN_PROGRESS;
        if (pPlayerClient->AsSWCObject() == 0)
            return ACTION_IN_PROGRESS;

#ifdef K2_HOST_BUILD
        {
            static int s_nLastHeld;
            CClientExoApp* pClientApp = g_pAppManager->m_pClientExoApp;
            int nHeld = (pClientApp->GetAreaNotReadyToDisplay() != 0) | (pClientApp->GetPausedByCombat() != 0) << 1
                        | (pClientApp->GetAutoPaused() != 0) << 2;
            if (nHeld != s_nLastHeld)
                fprintf(stderr, "k2: dialog start %s (area not ready %d, paused by combat %d, auto-paused %d)\n",
                        nHeld ? "held" : "free", nHeld & 1, (nHeld >> 1) & 1, (nHeld >> 2) & 1);
            s_nLastHeld = nHeld;
        }
#endif
        // Not a good moment to start talking, so put the action back and try again next tick.
        if (g_pAppManager->m_pClientExoApp->GetAreaNotReadyToDisplay() != 0
            || g_pAppManager->m_pClientExoApp->GetPausedByCombat() != 0
            || g_pAppManager->m_pClientExoApp->GetAutoPaused() != 0) {
            AddActionToFront(AIACTION_DIALOG_OBJECT, pNode->m_nGroupId,
                             ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_STRING, &sDialog,
                             ACTION_PARAM_INT, &nPrivate, ACTION_PARAM_INT, &nApproachPending,
                             ACTION_PARAM_INT, &nIgnoreRange, ACTION_PARAM_OBJECT, &oidSpeaker, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            return ACTION_COMPLETE;
        }

        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalDialogState(1);
        if (AsSWSCreature() != 0) {
            CSWSCreature* pMe = AsSWSCreature();
            pMe->ActivityManager(2);
            pMe->SetActivity(CSWSCreature::ACTIVITY_CONVERSATION, 0, 0);
            if (nIgnoreRange == 0) {
                if (nApproachPending != 0) {
                    // First tick: clear the latch so the re-queued node takes the flat distance test.
                    nApproachPending = 0;
                    if (AsSWSCreature()->GetIsInUseRange(oidTarget, 1.0f, 0, 0) == 0) {
                        unsigned long long oidArea = pTargetGO->AsSWSObject()->m_cachedAreaId;
                        Vector vDest = {0.0f, 0.0f, 0.0f};
                        float fRange;
                        AsSWSCreature()->GetUseRange(oidTarget, vDest, fRange, 0, 0);
                        fRange = fRange + 1.0f;
                        vDest = (vDest + m_position) * 0.5f;
                        AddActionToFront(AIACTION_DIALOG_OBJECT, pNode->m_nGroupId,
                                         ACTION_PARAM_OBJECT, &oidTarget,
                                         ACTION_PARAM_STRING, &sDialog, ACTION_PARAM_INT, &nPrivate,
                                         ACTION_PARAM_INT, &nApproachPending,
                                         ACTION_PARAM_INT, &nIgnoreRange,
                                         ACTION_PARAM_OBJECT, &oidSpeaker, 0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0);
                        pMe->AddMoveToPointActionToFront(pNode->m_nGroupId, vDest, oidArea,
                                                         oidTarget, 1, fRange, 0.0f,
                                                         0, 0, 0, 0, 0, 0, Vector{}, 0, 0, 0);
                        return ACTION_COMPLETE;
                    }
                } else {
                    // Later ticks: a flat straight-line gate, no pathfinding involved.
                    float fDx = m_position.x - pTarget->m_position.x;
                    float fDy = m_position.y - pTarget->m_position.y;
                    float fDz = m_position.z - pTarget->m_position.z;
                    if (fDx * fDx + fDy * fDy + fDz * fDz
                            > CSWSArea::MAX_OBJECT_START_CONVERSATION_DISTANCE
                              * CSWSArea::MAX_OBJECT_START_CONVERSATION_DISTANCE) {
                        unsigned long long oidArea = pTargetGO->AsSWSObject()->m_cachedAreaId;
                        Vector vDest = {0.0f, 0.0f, 0.0f};
                        float fRange;
                        AsSWSCreature()->GetUseRange(oidTarget, vDest, fRange, 0, 0);
                        fRange = fRange + 1.0f;
                        vDest = (vDest + m_position) * 0.5f;
                        // The speaker slot is tagged as an int here, unlike the other four re-queues in this
                        // function (an engine bug).
                        AddActionToFront(AIACTION_DIALOG_OBJECT, pNode->m_nGroupId,
                                         ACTION_PARAM_OBJECT, &oidTarget,
                                         ACTION_PARAM_STRING, &sDialog, ACTION_PARAM_INT, &nPrivate,
                                         ACTION_PARAM_INT, &nApproachPending,
                                         ACTION_PARAM_INT, &nIgnoreRange,
                                         ACTION_PARAM_INT, &oidSpeaker, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0);
                        pMe->AddMoveToPointActionToFront(pNode->m_nGroupId, vDest, oidArea,
                                                         oidTarget, 1, fRange, 0.0f,
                                                         0, 0, 0, 0, 0, 0, Vector{}, 0, 0, 0);
                        return ACTION_COMPLETE;
                    }
                }
            }
        }

        // In range (or told not to care), so work out whether the player should be holding this
        // conversation, and if not, hand it over.
        CSWSCreature* pOwner = 0;
        pTarget->AsSWSCreature();
        CSWSPlayer* pPlayerClientObj = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(
            g_pAppManager->m_pServerExoApp->GetPlayerCreatureId());
        if (pTarget != 0 && pPlayerClientObj != 0 && GetClientObject() != 0
            && GetClientObject()->AsSWCCreature() != 0
            && static_cast<CSWCCreature*>(GetClientObject()->AsSWCCreature())->m_bInParty != 0
            && m_objectId != pPlayerClientObj->m_gameObjectId) {
            unsigned long long oidPlayer = pPlayerClientObj->m_gameObjectId;
            if (pTarget->m_bPartyInteract == 0 && pTarget->GetDialogResref().IsValid()
                && pTarget->m_objectId != oidPlayer) {
                // Someone other than the player owns this conversation.
                pOwner = g_pAppManager->m_pServerExoApp->GetGameObject(oidPlayer)->AsSWSCreature();
                if (pOwner != 0 && pOwner->m_bInParty != 0) {
                    if (pOwner->m_bCombatState != 0 && pOwner->m_nCombatType == 1) {
                        // Mid-combat, refuse rather than yank the party around.
                        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalDialogState(0);
                        return ACTION_FAILED;
                    }
                    goto handOver;
                }
            } else if (pTarget->m_objectId == oidPlayer) {
                if (pTarget->GetClientObject() != 0
                    && pTarget->GetClientObject()->AsSWCCreature() != 0
                    && static_cast<CSWCCreature*>(pTarget->GetClientObject()->AsSWCCreature())->m_bInParty != 0)
                    g_pAppManager->m_pClientExoApp->ChangeCharacter(-3, 0);
            }
        }

#if !KOTOR1  // K1: no pre-dialog facing snapshot
        {
        CSWSCreature* pTargetCreature = pTarget->AsSWSCreature();
        if (pTargetCreature != 0 && pTargetCreature->m_bInParty != 0) {
            pTargetCreature->m_bRestoreDialogPose = 1;
            pTargetCreature->m_vDialogStartPosition = pTargetCreature->m_position;
            pTargetCreature->m_vDialogStartOrientation = pTargetCreature->m_orientation;
        }
        }
#endif
        goto signalEvent;

    handOver:
        {
            // Re-seat the whole party around the leader, fade, and re-queue this action on the
            // creature that owns the conversation.
            CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
            CSWSCreature* pLeaderServer =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                    g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pLeader->m_objectId));
            Vector vLeaderPos = pLeaderServer->m_position;
            static_cast<CSWCObject*>(pLeader)->ClearAllActions();
            pLeaderServer->ClearAllActions(1);
            for (int i = 1; i < 3; i++) {
                CSWCCreature* pMember =
                    g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(i);
                if (pMember == 0)
                    continue;
                CGameObject* pMemberGO = g_pAppManager->m_pServerExoApp->GetGameObject(
                    g_pAppManager->m_pServerExoApp->ClientToServerObjectId(pMember->m_objectId));
#ifdef K2_FIX_ENGINE_BUGS
                if (pMemberGO == 0)
                    continue;
#endif
                if (pMemberGO != 0 && pMemberGO->AsSWSCreature() != 0) {
                    pMemberGO->AsSWSCreature()->ClearAllActions(1);
                    static_cast<CSWCObject*>(pMember)->ClearAllActions();
                }
                float fDx = vLeaderPos.x - pMember->m_position.x;
                float fDy = vLeaderPos.y - pMember->m_position.y;
                float fDz = vLeaderPos.z - pMember->m_position.z;
                if (!(fDx * fDx + fDy * fDy + fDz * fDz >= 900.0f))
                    continue;
                CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
                Vector vOffset = pParty->m_members[i].m_formationOffset;
                Vector vFollow = pParty->m_members[i].m_followPoint;
                int nSample = pParty->m_members[i].m_bFollowPointValid;
                CSWPartyFollowData* pPathData = pParty->m_followData.GetPathData(nSample);
                Quaternion q(Vector{0.0f, 0.0f, 1.0f}, DegToRad(pPathData->m_fLeaderYaw));
                vOffset = RotateByQuaternion(vOffset, q);
                Vector vDest = vFollow + vOffset;
                g_pAppManager->m_pServerExoApp->ComputeSafeLocation(vDest, 10.0f,
                                                                   pMemberGO->m_objectId, 0,
                                                                   &vDest);
                pMember->SetPosition(vDest);
            }
            g_pAppManager->m_pClientExoApp->GetInGameGui()->StartGlobalFade(
                0, 0.0f, 0.75f, Vector{0.0f, 0.0f, 0.0f});
            g_pExoInput->SetActive(0);
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bHoldFadeInForDialog = 1;
            oidSpeaker = pLeaderServer->m_objectId;
            if (pOwner->GetDeadTemp() != 0) {
                CGameEffect* pEffect = new CGameEffect(1);
                pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_RESURRECTION);
                pEffect->m_durationTypeBits = (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK)
                                              | CGameEffect::DURATION_TYPE_INSTANTANEOUS;
                pOwner->ApplyEffect(pEffect, 0, 0);
                pOwner->m_bRaiseable = 1;
            }
            pOwner->AddActionToFront(AIACTION_DIALOG_OBJECT, pNode->m_nGroupId,
                                     ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_STRING, &sDialog,
                                     ACTION_PARAM_INT, &nPrivate,
                                     ACTION_PARAM_INT, &nApproachPending,
                                     ACTION_PARAM_INT, &nIgnoreRange,
                                     ACTION_PARAM_OBJECT, &oidSpeaker, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0);
            return ACTION_FAILED;
        }
    }

    // An explicit speaker was queued. Wait for any fade to settle first.
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->IsGlobalFading() != 0) {
        AddActionToFront(AIACTION_DIALOG_OBJECT, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidTarget,
                         ACTION_PARAM_STRING, &sDialog, ACTION_PARAM_INT, &nPrivate,
                         ACTION_PARAM_INT, &nApproachPending, ACTION_PARAM_INT, &nIgnoreRange,
                         ACTION_PARAM_OBJECT, &oidSpeaker, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0);
        return ACTION_FAILED;
    }

    {
    CGameObject* pSpeakerGO = g_pAppManager->m_pServerExoApp->GetGameObject(oidSpeaker);
    if (pSpeakerGO != 0 && pSpeakerGO->AsSWSCreature() != 0) {
        CSWSCreature* pSpeaker = pSpeakerGO->AsSWSCreature();
        Vector vMyPos = m_position;
        Vector vSpeakerPos = pSpeaker->m_position;
        Vector vSpeakerFacing = pSpeaker->m_orientation;
#if !KOTOR1
        if (pSpeaker->m_bInParty != 0) {
            pSpeaker->m_bRestoreDialogPose = 1;
            pSpeaker->m_vDialogStartPosition = vSpeakerPos;
            pSpeaker->m_vDialogStartOrientation = vSpeakerFacing;
        }
#endif
        CGameObject* pTargetGO = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
        if (pTargetGO != 0 && pTargetGO->AsSWSObject() != 0)
            vSpeakerFacing = pTargetGO->AsSWSObject()->m_position - vSpeakerPos;
        // Swap places, so we take the speaker's place and the speaker takes ours.
        SetPosition(vSpeakerPos, 1, 1, 0);
        SetOrientation(vSpeakerFacing);
        pSpeakerGO->AsSWSCreature()->SetPosition(vMyPos, 1, 1, 0);
        if (GetClientObject() != 0 && GetClientObject()->AsSWCCreature() != 0) {
            static_cast<CSWCCreature*>(GetClientObject()->AsSWCCreature())->SetPosition(vSpeakerPos);
            static_cast<CSWCCreature*>(GetClientObject()->AsSWCCreature())->SetOrientation(vSpeakerFacing);
        }
        g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            g_pAppManager->m_pClientExoApp->ServerToClientObjectId(
                pSpeakerGO->AsSWSCreature()->m_objectId));
        if (pSpeakerGO->AsSWSCreature() != 0
            && pSpeakerGO->AsSWSCreature()->GetClientCreature() != 0)
            pSpeakerGO->AsSWSCreature()->GetClientCreature()->SetPosition(vMyPos);
        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(
            g_pAppManager->m_pServerExoApp->GetPlayerCreatureId());
        g_pAppManager->m_pClientExoApp->ChangeCharacter(-3, 0);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_oidPostDialogCharacterSwitch = oidSpeaker;
        g_pExoInput->SetActive(1);
    }
    }

signalEvent:
    {
        CScriptEvent* pEvent = new CScriptEvent();
        pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_DIALOGUE;
        pEvent->SetInteger(0, -1);
        pEvent->SetInteger(1, -1);
        pEvent->SetInteger(2, nPrivate);
        pEvent->SetInteger(3, nIgnoreRange);
        pEvent->SetString(0, sDialog);
        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
            0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
    }
    return ACTION_COMPLETE;
}

int CSWSObject::AIActionLockObject(CSWSObjectActionNode* pNode) {
    CSWSCreature* pCreature;
#if KOTOR1  // K1: the dying tests are GetDeadTemp() / GetIsPCDying()
    int bDying = 0;
    if (AsSWSCreature() != 0)
        bDying = AsSWSCreature()->GetDeadTemp();
    if (GetDead() == 1 || GetIsPCDying() == 1 || bDying == 1) {
        pCreature = AsSWSCreature();
        goto failed;
    }
    pCreature = AsSWSCreature();
#else
    int bDying = 0;
    if (AsSWSCreature() != 0) {
        pCreature = AsSWSCreature();
        bDying = pCreature->GetDeadTemp();
    }
    int bDead = GetDead();
    pCreature = AsSWSCreature();
    if (bDying || bDead == 1)
        goto failed;
#endif
    if (pCreature != 0)
        AsSWSCreature()->ActivityManager(2);
    {
        unsigned long long oidTarget = pNode->m_args.m_aqTargetId;
        CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
        CSWSObject* pTarget = pObject->AsSWSObject();
        pCreature = AsSWSCreature();
        if (pTarget == 0)
            goto failed;
        if (pCreature != 0) {
            Vector vPos;
            float fRange;
            if (AsSWSCreature()->GetIsInUseRange(oidTarget, 0.0f, 0, 0) == 0) {
                // Out of range, so re-queue ourselves behind a walk. The re-entry goes on first so
                // it runs last.
                AsSWSCreature()->GetUseRange(oidTarget, vPos, fRange, 0, 0);
                AddActionToFront(AIACTION_LOCK_OBJECT, pNode->m_nGroupId,
                                 ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR ||
                    pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
                    AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                                     ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                } else {
                    AddActionToFront(AIACTION_CHANGE_FACING_POINT, pNode->m_nGroupId,
                                     ACTION_PARAM_FLOAT, &vPos.x, ACTION_PARAM_FLOAT, &vPos.y,
                                     ACTION_PARAM_FLOAT, &vPos.z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0);
                }
                AsSWSCreature()->AddMoveToPointActionToFront(
                    pNode->m_nGroupId, vPos, pTarget->m_cachedAreaId, oidTarget, 1, fRange, 0.0f, 0,
                    0, 0, 0, 0, 0, Vector{}, 0, 0, 0);
                return ACTION_COMPLETE;
            }
        }
        if (AsSWSCreature() != 0 && AsSWSCreature()->m_bLockStarted == 0) {
            // Tick 1: latch, play the animation, run the timing bar.
            AsSWSCreature()->m_bLockStarted = 1;
            int nOne = 1;
            int nAnim;
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
                nAnim = SERVER_ANIMATION_UNLOCK_DOOR;
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
                nAnim = SERVER_ANIMATION_UNLOCK_CONTAINER;
            float fIn = 1.0f;
            float fOut = 1.5f;
            AddActionToFront(AIACTION_LOCK_OBJECT, pNode->m_nGroupId,
                             ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            AddActionToFront(AIACTION_PLAY_ANIMATION, pNode->m_nGroupId, ACTION_PARAM_INT, &nAnim,
                             ACTION_PARAM_FLOAT, &fIn, ACTION_PARAM_FLOAT, &fOut,
                             ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0);
            AsSWSCreature()->StartGuiTimingBar(1500, 8);
            return ACTION_COMPLETE;
        }
        if (AsSWSCreature() != 0 && AsSWSCreature()->EvaluateLock(pTarget, 1) == 1) {
            // The bar is still running, so come back next tick.
            AsSWSCreature()->StopGuiTimingBar();
            AsSWSCreature()->m_bLockStarted = 0;
            return ACTION_COMPLETE;
        }
        if (AsSWSCreature() != 0 && AsSWSCreature()->GetIsInUseRange(oidTarget, 0.0f, 0, 0) == 0)
            goto complete;
        {
            int bDidLock;
            CScriptEvent* pEvent;
#if !KOTOR1
            CSWSCreature* pFbCreature;
            unsigned short nFbMessage;
            CSWCCMessageData* pFbData;
#endif
            if (pTarget->AsSWSDoor() != 0) {
                if (pTarget->AsSWSDoor()->m_bLocked != 0) {
                    if (AsSWSCreature() == 0)
                        goto returnFailed;
                    CSWCCMessageData* pData = new CSWCCMessageData();
                    pData->SetObjectID(0, pTarget->m_objectId);
#if KOTOR1
                    // K1: the two feedback calls are separate sites
                    AsSWSCreature()->SendFeedbackMessage(FEEDBACK_OBJECT_LOCKED, pData);
                    goto failedInner;
#else
                    pFbCreature = AsSWSCreature();
                    nFbMessage = FEEDBACK_OBJECT_LOCKED;
                    pFbData = pData;
                    goto sendFeedback;
#endif
                }
                if (pTarget->AsSWSDoor()->m_bKeyRequired != 0) {
                    pEvent = new CScriptEvent();
                    goto keyRequired;
                }
                bDidLock = 0;
                if (pTarget->m_objectId == m_objectId) {
                    pTarget->AsSWSDoor()->m_bLocked = 1;
                    bDidLock = 1;
                }
            } else if (pTarget->AsSWSPlaceable() != 0) {
                if (pTarget->AsSWSPlaceable()->m_bLocked != 0) {
                    if (AsSWSCreature() == 0)
                        goto returnFailed;
                    CSWCCMessageData* pData = new CSWCCMessageData();
                    pData->SetObjectID(0, pTarget->m_objectId);
#if KOTOR1
                    // K1: the two feedback calls are separate sites
                    AsSWSCreature()->SendFeedbackMessage(FEEDBACK_OBJECT_LOCKED, pData);
                    goto failedInner;
#else
                    pFbCreature = AsSWSCreature();
                    nFbMessage = FEEDBACK_OBJECT_LOCKED;
                    pFbData = pData;
                    goto sendFeedback;
#endif
                }
                if (pTarget->AsSWSPlaceable()->m_bKeyRequired != 0) {
                    pEvent = new CScriptEvent();
                    goto keyRequired;
                }
                bDidLock = 0;
                if (pTarget->m_objectId == m_objectId) {
                    pTarget->AsSWSPlaceable()->m_bLocked = 1;
                    bDidLock = 1;
                }
            } else {
                bDidLock = 0;
            }

            {
#ifdef K2_FIX_ENGINE_BUGS
            int nRoll = 0;
            int nSkillRank = 0;
            int bTake20 = 0;
            int nDC = 0;
#else
            int nRoll;
            int nSkillRank;
            int bTake20 = 0;
            int nDC;  // deliberately uninitialized
#endif
            if (AsSWSCreature() != 0) {
                if (AsSWSCreature()->m_bCombatState != 0) {
                    nRoll = rand_wincompatible() % 20 + 1;
                } else {
                    nRoll = 20;
                    bTake20 = 1;
                }
                nSkillRank = AsSWSCreature()->m_pStats->GetSkillRank(SKILL_SECURITY, 0, 0);
                if (pTarget->AsSWSDoor() != 0) {
#if KOTOR1
                    // K1: the DC byte is read once
                    nDC = pTarget->AsSWSDoor()->m_closeLockDC;
                    if (nDC == 0)
                        nDC = 1;
#else
                    nDC = 1;
                    if (pTarget->AsSWSDoor()->m_closeLockDC != 0)
                        nDC = pTarget->AsSWSDoor()->m_closeLockDC;
#endif
                    if (nRoll + nSkillRank >= nDC) {
                        pTarget->AsSWSDoor()->m_bLocked = 1;
                        bDidLock = 1;
                    }
                } else if (pTarget->AsSWSPlaceable() != 0) {
#if KOTOR1
                    // K1: the DC byte is read once
                    nDC = pTarget->AsSWSPlaceable()->m_closeLockDC;
                    if (nDC == 0)
                        nDC = 1;
#else
                    nDC = 1;
                    if (pTarget->AsSWSPlaceable()->m_closeLockDC != 0)
                        nDC = pTarget->AsSWSPlaceable()->m_closeLockDC;
#endif
                    if (nRoll + nSkillRank >= nDC) {
                        pTarget->AsSWSPlaceable()->m_bLocked = 1;
                        bDidLock = 1;
                    }
                }
            }
            if (bDidLock) {
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_LOCK_OBJECT, 0);
                CScriptEvent* pLockEvent = new CScriptEvent();
                pLockEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_LOCKED;
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, m_objectId, pTarget->m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT,
                    pLockEvent);
            }
            if (AsSWSCreature() != 0) {
                CSWCCMessageData* pData = new CSWCCMessageData();
                pData->SetObjectID(0, m_objectId);
                pData->SetInteger(0, STRREF_SECURITY);
                pData->SetInteger(1, nRoll);
                pData->SetInteger(2, nSkillRank);
                pData->SetInteger(3, nDC);
                pData->SetInteger(4, bTake20);
                pData->SetInteger(5, nRoll + nSkillRank >= nDC ? 1 : (bTake20 != 0 ? 3 : 0));
                pData->SetInteger(6, STRREF_SECURITY);
                AsSWSCreature()->BroadcastSkillData(pData);
            }
            }
            goto complete;

        keyRequired:
            // A key-only lock cannot be worked with the Security skill at all: signal the script hook
            // and refuse.
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_FAIL_TO_OPEN;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            if (AsSWSCreature() == 0)
                goto returnFailed;
#if KOTOR1
            AsSWSCreature()->SendFeedbackMessage(FEEDBACK_CANNOT_OPEN_CONVENTIONALLY, 0);
            goto failedInner;
#else
            pFbCreature = AsSWSCreature();
            nFbMessage = FEEDBACK_CANNOT_OPEN_CONVENTIONALLY;
            pFbData = 0;
        sendFeedback:
            pFbCreature->SendFeedbackMessage(nFbMessage, pFbData);
            goto failedInner;
#endif
        }
    }
complete:
    if (AsSWSCreature() != 0) {
        AsSWSCreature()->StopGuiTimingBar();
        AsSWSCreature()->m_bLockStarted = 0;
    }
    return ACTION_COMPLETE;
failed:
    if (pCreature == 0)
        goto returnFailed;
failedInner:
    AsSWSCreature()->StopGuiTimingBar();
    AsSWSCreature()->m_bLockStarted = 0;
returnFailed:
    return ACTION_FAILED;
}

int CSWSObject::AIActionUnlockObject(CSWSObjectActionNode* pNode) {
    CSWSCreature* pCreature;
#if KOTOR1  // K1: the dying tests are GetDeadTemp() / GetIsPCDying()
    int bDying = 0;
    if (AsSWSCreature() != 0)
        bDying = AsSWSCreature()->GetDeadTemp();
    if (GetDead() == 1 || GetIsPCDying() == 1 || bDying == 1) {
        pCreature = AsSWSCreature();
        goto failed;
    }
    pCreature = AsSWSCreature();
#else
    int bDying = 0;
    if (AsSWSCreature() != 0) {
        pCreature = AsSWSCreature();
        bDying = pCreature->GetDeadTemp();
    }
    int bDead = GetDead();
    pCreature = AsSWSCreature();
    if (bDying || bDead == 1)
        goto failed;
#endif
    if (pCreature != 0)
        AsSWSCreature()->ActivityManager(2);
    {
        unsigned long long oidTarget = pNode->m_args.m_aqTargetId;
        unsigned long long oidItem = pNode->m_args.m_unlockItemId;
        int nProperty = pNode->m_args.m_unlockItemPropertyIndex;
        CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
        if (pObject == 0) {
            pCreature = AsSWSCreature();
            goto failed;
        }
        CSWSObject* pTarget = pObject->AsSWSObject();
        pCreature = AsSWSCreature();
        if (pTarget == 0)
            goto failed;
        if (pCreature != 0) {
            Vector vPos;
            float fRange;
            int bInRange = AsSWSCreature()->GetIsInUseRange(oidTarget, 0.0f, 0, 0);
            pCreature = AsSWSCreature();
            if (bInRange == 0) {
                // Out of range, so re-queue behind a walk, as the twin does.
                pCreature->GetUseRange(oidTarget, vPos, fRange, 0, 0);
                AddActionToFront(AIACTION_UNLOCK_OBJECT, pNode->m_nGroupId,
                                 ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_OBJECT, &oidItem, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR ||
                    pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
                    AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                                     ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                } else {
                    AddActionToFront(AIACTION_CHANGE_FACING_POINT, pNode->m_nGroupId,
                                     ACTION_PARAM_FLOAT, &vPos.x, ACTION_PARAM_FLOAT, &vPos.y,
                                     ACTION_PARAM_FLOAT, &vPos.z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0);
                }
                AsSWSCreature()->AddMoveToPointActionToFront(
                    pNode->m_nGroupId, vPos, pTarget->m_cachedAreaId, oidTarget, 1, fRange, 0.0f, 0,
                    0, 0, 0, 0, 0, Vector{}, 0, 0, 0);
                return ACTION_COMPLETE;
            }
            // In range on the first tick, re-queue the same pair of actions without a walk, and
            // roll a one-in-five chance of a voice line.
            if (pCreature->m_bActionApproachDone == 0) {
                AsSWSCreature()->m_bActionApproachDone = 1;
                AsSWSCreature()->GetUseRange(oidTarget, vPos, fRange, 0, 0);
                AddActionToFront(AIACTION_UNLOCK_OBJECT, pNode->m_nGroupId,
                                 ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_OBJECT, &oidItem, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR ||
                    pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
                    AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                                     ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                } else {
                    AddActionToFront(AIACTION_CHANGE_FACING_POINT, pNode->m_nGroupId,
                                     ACTION_PARAM_FLOAT, &vPos.x, ACTION_PARAM_FLOAT, &vPos.y,
                                     ACTION_PARAM_FLOAT, &vPos.z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0);
                }
#if KOTOR1  // K1: the announcement always plays
                AsSWSCreature()->BroadcastVoiceChat(CSWSCreature::VOICE_CHAT_UNLOCK_START);
#else
                if (rand_wincompatible() % 100 <= 19)
                    AsSWSCreature()->BroadcastVoiceChat(CSWSCreature::VOICE_CHAT_UNLOCK_START);
#endif
                return ACTION_COMPLETE;
            }
        }
        if (AsSWSCreature() != 0 && AsSWSCreature()->m_bUnlockStarted == 0) {
            // Tick 2: latch, face the lock, play the animation, run the timing bar.
            AsSWSCreature()->m_bUnlockStarted = 1;
            SetLockOrientationToObject(oidTarget);
            // The placeable arm below redundantly repeats this same value.
            int nAnim = SERVER_ANIMATION_UNLOCK_CONTAINER;
            int nOne = 1;
            if (pObject->m_objectType == CGameObject::OBJECT_TYPE_DOOR)
                nAnim = SERVER_ANIMATION_UNLOCK_DOOR;
            else if (pObject->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE)
                nAnim = SERVER_ANIMATION_UNLOCK_CONTAINER;
            float fIn = 1.0f;
            float fOut = 1.5f;
            AddActionToFront(AIACTION_UNLOCK_OBJECT, pNode->m_nGroupId,
                             ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_OBJECT, &oidItem, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            AddActionToFront(AIACTION_PLAY_ANIMATION, pNode->m_nGroupId, ACTION_PARAM_INT, &nAnim,
                             ACTION_PARAM_FLOAT, &fIn, ACTION_PARAM_FLOAT, &fOut,
                             ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0);
            AsSWSCreature()->StartGuiTimingBar(1500, 7);
            return ACTION_COMPLETE;
        }
        {
            CSWSDoor* pDoor = pTarget->AsSWSDoor();
            CSWSPlaceable* pPlaceable = pTarget->AsSWSPlaceable();
            CScriptEvent* pEvent;
            CSWCCMessageData* pKeyData;
            int bDidUnlock;
            if (pDoor != 0 && pDoor->m_bTrapFlag != 0 && AsSWSCreature() != 0) {
                if (pDoor->GetReputation(m_objectId, 0) <= CFactionManager::REPUTATION_FRIENDLY_ABOVE &&
                    pDoor->m_nFaction != static_cast<unsigned int>(AsSWSCreature()->m_pStats->m_factionId)) {
                    pEvent = new CScriptEvent();
                    goto trapRefused;
                }
            } else if (pPlaceable != 0 && pPlaceable->m_bTrapFlag != 0 && AsSWSCreature() != 0) {
                if (pPlaceable->GetReputation(m_objectId, 0) <= CFactionManager::REPUTATION_FRIENDLY_ABOVE &&
                    pPlaceable->m_nFaction != static_cast<unsigned int>(AsSWSCreature()->m_pStats->m_factionId)) {
                    pEvent = new CScriptEvent();
                    goto trapRefused;
                }
            }
            // Nothing is left to pick (EvaluateLock reports the lock is already open), so just use
            // it.
            if (AsSWSCreature() != 0 && AsSWSCreature()->EvaluateLock(pTarget, 0) == 1) {
                if (pDoor != 0) {
                    pDoor->OpenDoor(m_objectId);
                } else if (pPlaceable != 0) {
                    unsigned long long oidOpen = pPlaceable->m_objectId;
                    AddActionToFront(AIACTION_USE_OBJECT, pNode->m_nGroupId,
                                     ACTION_PARAM_OBJECT, &oidOpen, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                }
                goto completeInner;
            }
            if (AsSWSCreature() != 0 &&
                AsSWSCreature()->GetIsInUseRange(oidTarget, 0.0f, 0, 0) == 0)
                goto complete;
            if (pDoor != 0) {
                if (pDoor->m_bLocked == 0)
                    goto alreadyUnlocked;
                if (pDoor->m_bKeyRequired != 0) {
                    pEvent = new CScriptEvent();
                    goto keyRequiredDoor;
                }
                bDidUnlock = 0;
                if (pTarget->m_objectId == m_objectId) {
                    pDoor->m_bLocked = 0;
                    pDoor->OpenDoor(m_objectId);
                    bDidUnlock = 1;
                }
            } else if (pPlaceable != 0) {
                if (pPlaceable->m_bLocked == 0)
                    goto alreadyUnlocked;
                if (pPlaceable->m_bKeyRequired != 0) {
                    pEvent = new CScriptEvent();
                    goto keyRequiredPlaceable;
                }
                bDidUnlock = 0;
                if (pTarget->m_objectId == m_objectId) {
                    pPlaceable->m_bLocked = 0;
                    bDidUnlock = 1;
                }
            } else {
                bDidUnlock = 0;
            }

            {
#ifdef K2_FIX_ENGINE_BUGS
            int nRoll = 0;
            int nSkill = 0;
            int nDC = 0;
            int bTake20 = 0;
            CSWSItem* pItem = 0;
#else
            int nRoll;
            int nSkill;
            int nDC;
            int bTake20 = 0;
            CSWSItem* pItem;
#endif
            if (AsSWSCreature() != 0) {
                if (AsSWSCreature()->m_bCombatState != 0) {
                    nRoll = rand_wincompatible() % 20 + 1;
                } else {
                    nRoll = 20;
                    bTake20 = 1;
                }
                int nSkillRank = AsSWSCreature()->m_pStats->GetSkillRank(SKILL_SECURITY, 0, 0);
                int nBonus = 0;
                pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
                if (pItem != 0) {
                    CSWItemProperty* pProp = pItem->GetActiveProperty(nProperty);
                    if (pProp != 0 &&
                        (pProp->m_nUpgradeType == CSWItemProperty::UPGRADE_TYPE_NONE ||
                         (pItem->m_nUpgrades & (1 << pProp->m_nUpgradeType)) != 0))
                        nBonus = pProp->m_costTableValue;
                }
                nSkill = nBonus + nSkillRank;
                if (pTarget->AsSWSDoor() != 0) {
#if KOTOR1  // K1: the raw OpenLockDC byte, read once
                    nDC = static_cast<unsigned char>(pTarget->AsSWSDoor()->GetOpenLockDC());
                    if (nDC == 0)
                        nDC = 1;
#else
                    nDC = 1;
                    if (static_cast<unsigned char>(pTarget->AsSWSDoor()->GetOpenLockDC()) != 0)
                        nDC = static_cast<unsigned char>(pTarget->AsSWSDoor()->GetOpenLockDC());
#endif
                    if (nSkill + nRoll >= nDC) {
                        pTarget->AsSWSDoor()->m_bLocked = 0;
                        pTarget->AsSWSDoor()->OpenDoor(m_objectId);
                        goto unlocked;
                    }
                } else if (pTarget->AsSWSPlaceable() != 0) {
#if KOTOR1
                    nDC = static_cast<unsigned char>(pTarget->AsSWSPlaceable()->GetOpenLockDC());
                    if (nDC == 0)
                        nDC = 1;
#else
                    nDC = 1;
                    if (static_cast<unsigned char>(pTarget->AsSWSPlaceable()->GetOpenLockDC()) != 0)
                        nDC = static_cast<unsigned char>(pTarget->AsSWSPlaceable()->GetOpenLockDC());
#endif
                    if (nSkill + nRoll >= nDC) {
                        pTarget->AsSWSPlaceable()->m_bLocked = 0;
                        // A picked container only auto-opens for the party leader.
                        if (AsSWSCreature()->m_pStats->m_bIsPC != 0 ||
                            g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsLeader(
                                m_objectId) != 0) {
                            unsigned long long oidOpen = pTarget->m_objectId;
                            AddActionToFront(AIACTION_USE_OBJECT, pNode->m_nGroupId,
                                             ACTION_PARAM_OBJECT, &oidOpen, 0, 0, 0, 0, 0, 0, 0, 0,
                                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                        }
                        goto unlocked;
                    }
                }
            }
            if (bDidUnlock == 0)
                goto rollFailed;
        unlocked:
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_UNLOCK_OBJECT, 0);
#if KOTOR1  // K1: no "opened" script event, the success line always plays, no party XP
            if (AsSWSCreature() != 0)
                AsSWSCreature()->BroadcastVoiceChat(CSWSCreature::VOICE_CHAT_UNLOCK_SUCCESS);
#else
            {
                CScriptEvent* pOpened = new CScriptEvent();
                pOpened->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_UNLOCKED;
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, m_objectId, pTarget->m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT,
                    pOpened);
            }
            if (AsSWSCreature() != 0 && rand_wincompatible() % 100 <= 19)
                AsSWSCreature()->BroadcastVoiceChat(CSWSCreature::VOICE_CHAT_UNLOCK_SUCCESS);
            if (AsSWSCreature() != 0 && AsSWSCreature()->m_bInParty != 0) {
                CSWSCreature* pPlayer = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
                int nThreshold = pPlayer->m_pStats->GetLevel(0) + 20;
                int nLevel = pPlayer->m_pStats->GetLevel(0);
                int nXP = (nDC < nThreshold ? 5 : 10) * nLevel;
                g_pAppManager->m_pServerExoApp->GetPartyTable()->DistributeExperience(nXP, 1);
            }
#endif
            goto broadcast;
        rollFailed:
            if (AsSWSCreature() != 0)
                AsSWSCreature()->BroadcastVoiceChat(CSWSCreature::VOICE_CHAT_UNLOCK_FAILED);
#if !KOTOR1  // K1 signals no failure script event
            {
                // AddEventAbsoluteTime here, AddEventDeltaTime everywhere else in this action.
                CScriptEvent* pFailed = new CScriptEvent();
                pFailed->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_FAIL_TO_OPEN;
                pFailed->SetInteger(0, 1);
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventAbsoluteTime(
                    0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pFailed);
            }
#endif
        broadcast:
            if (AsSWSCreature() != 0) {
                CSWCCMessageData* pData = new CSWCCMessageData();
                pData->SetObjectID(0, m_objectId);
                pData->SetObjectID(1, oidTarget);
                pData->SetInteger(0, STRREF_SECURITY);
                pData->SetInteger(1, nRoll);
                pData->SetInteger(2, nSkill);
                pData->SetInteger(3, nDC);
                pData->SetInteger(4, bTake20);
                // A lock DC of 60 or more is the "impossible" band: a take-20 failure reports 5 instead
                // of 3, and a success reports no result code at all.
#if KOTOR1  // K1: a success always reports 1 (no impossible-lock band on success)
                int nResult = 1;
                if (nSkill + nRoll < nDC) {
                    if (bTake20 == 0)
                        nResult = 0;
                    else
                        nResult = nDC > LOCK_DC_POSSIBLE_MAX ? 5 : 3;
                }
                pData->SetInteger(5, nResult);
#else
                if (nRoll + nSkill >= nDC && nDC <= LOCK_DC_POSSIBLE_MAX)
                    pData->SetInteger(5, 1);
                else if (bTake20 != 0 && nRoll + nSkill < nDC)
                    pData->SetInteger(5, nDC > LOCK_DC_POSSIBLE_MAX ? 5 : 3);
                else if (nRoll + nSkill < nDC)
                    pData->SetInteger(5, 0);
#endif
                pData->SetInteger(6, STRREF_SECURITY);
                AsSWSCreature()->BroadcastSkillData(pData);
                // Spend the spike, taking one unit off the stack or destroying it if that was the
                // last.
                if (pItem != 0 &&
                    AsSWSCreature()->GetItemRepository(1)->GetItemInRepository(pItem, 1) != 0) {
                    if (pItem->m_stackCount >= 2) {
                        pItem->m_stackCount = pItem->m_stackCount - 1;
                    } else if (AsSWSCreature()->RemoveItem(pItem, 1, 1, 0) != 0) {
                        CSWSPlayer* pOwner =
                            g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
                        if (pOwner != 0)
                            g_pAppManager->m_pServerExoApp->GetSWSMessage()
                                ->SendServerPlayerItemUpdate_DestroyItem(pOwner, pItem->m_objectId);
                        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                            0, 0, m_objectId, pItem->m_objectId,
                            CServerAIMaster::EVENT_DESTROY_OBJECT, 0);
                    }
                }
            }
            goto complete;
            }

        alreadyUnlocked:
            if (AsSWSCreature() == 0)
                goto returnFailed;
            AsSWSCreature()->SendFeedbackMessage(FEEDBACK_OBJECT_NOT_LOCKED, 0);
            goto failedInner;

        keyRequiredDoor:
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_FAIL_TO_OPEN;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            if (AsSWSCreature() != 0) {
                AsSWSCreature()->SendFeedbackMessage(FEEDBACK_CANNOT_OPEN_CONVENTIONALLY, 0);
                AsSWSCreature()->StopGuiTimingBar();
                AsSWSCreature()->m_bUnlockStarted = 0;
                AsSWSCreature()->m_bActionApproachDone = 0;
            }
            if (AsSWSCreature() == 0)
                goto returnFailed;
            pKeyData = new CSWCCMessageData();
            goto keyRefused;
        keyRequiredPlaceable:
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_FAIL_TO_OPEN;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            if (AsSWSCreature() != 0) {
                AsSWSCreature()->SendFeedbackMessage(FEEDBACK_CANNOT_OPEN_CONVENTIONALLY, 0);
                AsSWSCreature()->StopGuiTimingBar();
                AsSWSCreature()->m_bUnlockStarted = 0;
                AsSWSCreature()->m_bActionApproachDone = 0;
            }
            if (AsSWSCreature() == 0)
                goto returnFailed;
            pKeyData = new CSWCCMessageData();
        keyRefused:
            pKeyData->SetObjectID(0, m_objectId);
            pKeyData->SetObjectID(1, oidTarget);
            pKeyData->SetInteger(0, STRREF_SECURITY);
            pKeyData->SetInteger(1, 0);
            pKeyData->SetInteger(2, 0);
            pKeyData->SetInteger(3, 0);
            pKeyData->SetInteger(4, 0);
            pKeyData->SetInteger(5, 5);
            pKeyData->SetInteger(6, STRREF_SECURITY);
            AsSWSCreature()->BroadcastSkillData(pKeyData);
            AsSWSCreature()->BroadcastVoiceChat(CSWSCreature::VOICE_CHAT_UNLOCK_FAILED);
            goto returnFailed;

        trapRefused:
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_MINE_TRIGGERED;
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            goto clearLatches;
        }
    }
complete:
    if (AsSWSCreature() == 0)
        goto returnComplete;
completeInner:
    AsSWSCreature()->StopGuiTimingBar();
    AsSWSCreature()->m_bUnlockStarted = 0;
    AsSWSCreature()->m_bActionApproachDone = 0;
returnComplete:
    return ACTION_COMPLETE;
failed:
    if (pCreature == 0)
        goto returnFailed;
failedInner:
    AsSWSCreature()->StopGuiTimingBar();
    // The trap-refusal path enters below StopGuiTimingBar; it refuses before the bar was started.
clearLatches:
    AsSWSCreature()->m_bUnlockStarted = 0;
    AsSWSCreature()->m_bActionApproachDone = 0;
returnFailed:
    return ACTION_FAILED;
}

int CSWSObject::AIActionGiveItem(CSWSObjectActionNode* pNode) {
    CSWSItem* pItem = 0;
    if (pNode->m_nNumParameters < 2)
        return ACTION_FAILED;
    unsigned long long oidItem = pNode->m_args.m_transferItemId;
    unsigned long long oidTarget = pNode->m_args.m_transferPeerId;
    int nStackSize = pNode->m_args.m_giveStackSize;
    int bNotify = pNode->m_args.m_bGiveItemNotify;
    CGameObject* pTarget = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    if (pTarget == 0)
        return ACTION_FAILED;
    if (AsSWSCreature() == 0 || AsSWSCreature()->GetIsInUseRange(oidTarget, 0.0f, 0, 0) != 0) {
        pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
        if (pItem == 0)
            return ACTION_FAILED;
        if (static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_CREDITS) {
            if (AsSWSCreature() != 0)
                AsSWSCreature()->RemoveGold(pItem->m_stackCount, 1);
        } else if (static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType == ITEM_TYPE_PAZAAK_CARD) {
            if (AsSWSCreature() != 0)
                AsSWSCreature()->RemovePazaakCard(pItem);
        }
        int bNotGold = static_cast<CSWItem*>(pItem)->GetBaseItem()->m_nItemType != ITEM_TYPE_CREDITS;
        if (nStackSize != -1) {
            CSWSItem* pSplit = pItem->SplitItem(nStackSize);
            if (pSplit != 0)
                pItem = pSplit;
        }
        if (pTarget->m_objectType == GAME_OBJECT_TYPE_CREATURE) {
#if !KOTOR1  // K1: no received-item name list (no CExoLocString copy)
            CExoLocString sName;
            if (pItem != 0)
                sName = pItem->m_localizedName;
#endif
            pTarget->AsSWSCreature()->AcquireItem(&pItem, pItem->m_possessorId, OBJECT_INVALID, bNotGold);
            if (bNotify != 0 && pTarget->AsSWSCreature()->m_bInParty != 0) {
#if !KOTOR1
                g_pAppManager->m_pClientExoApp->GetInGameGui()->AddReceivedItemToList(sName);
#endif
                g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_ITEM_RECEIVED, 0);
            }
        } else if (pTarget->m_objectType == GAME_OBJECT_TYPE_ITEM) {  // a container item
            CSWRules* pRules = g_pRules;
            int nBaseItem = pTarget->AsSWSItem()->m_baseItem;
            if (pRules->GetBaseItem(nBaseItem)->m_bContainer == 1)
                pTarget->AsSWSItem()->AcquireItem(&pItem, pItem->m_possessorId, bNotGold);
        } else if (pTarget->m_objectType == GAME_OBJECT_TYPE_PLACEABLE) {
            pTarget->AsSWSPlaceable()->AcquireItem(&pItem, pItem->m_possessorId, bNotGold);
        }
    } else {
        Vector vPos;
        float fRange;
        int bOutOfRange = 0;
        int nRun = 1;
        AsSWSCreature()->GetUseRange(oidTarget, vPos, fRange, 0, 0);
        float fDx = m_position.x - vPos.x;
        float fDy = m_position.y - vPos.y;
        float fDz = m_position.z - vPos.z;
        if (fDx * fDx + fDy * fDy + fDz * fDz > 25.0f)
            bOutOfRange = 1;
        AddActionToFront(AIACTION_GIVE_ITEM, pNode->m_nGroupId, ACTION_PARAM_OBJECT, &oidItem,
                         ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nStackSize,
                         ACTION_PARAM_INT, &bNotify, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0);
        AddActionToFront(AIACTION_CHANGE_FACING_POINT, pNode->m_nGroupId,
                         ACTION_PARAM_FLOAT, &vPos.x, ACTION_PARAM_FLOAT, &vPos.y,
                         ACTION_PARAM_FLOAT, &vPos.z, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0);
        AddActionToFront(AIACTION_CHECK_MOVE_TO_OBJECT_RADIUS, pNode->m_nGroupId,
                         ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &bOutOfRange,
                         ACTION_PARAM_FLOAT, &fRange, ACTION_PARAM_FLOAT, &fRange,
                         ACTION_PARAM_INT, &nRun, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        AsSWSCreature()->AddMoveToPointActionToFront(pNode->m_nGroupId, vPos, m_cachedAreaId,
                                                    oidTarget, bOutOfRange, fRange, 0.0f,
                                                    0, 0, 0, 0, 0, 0, Vector{}, 0, 0, 0);
    }
    return ACTION_COMPLETE;
}

int CSWSObject::AIActionTakeItem(CSWSObjectActionNode* pNode) {
    if (pNode->m_nNumParameters == 0)
        return ACTION_FAILED;
    unsigned long long oidItem = pNode->m_args.m_transferItemId;
    unsigned long long oidPeer = pNode->m_args.m_transferPeerId;
    int bNotify = pNode->m_args.m_bTakeItemNotify;
    CGameObject* pObject;
    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    CSWSItem* pItem = 0;
    if (pArray->GetGameObject(oidItem, &pObject) == CGameObjectArray::SUCCESS &&
        pObject != 0) {
        pItem = pObject->AsSWSItem();
        if (pItem != 0) {
            if (m_objectType == GAME_OBJECT_TYPE_CREATURE) {
                if (AsSWSCreature()->AcquireItem(&pItem, oidPeer, OBJECT_INVALID, bNotify) != 0 &&
                    bNotify != 0) {
                    CSWSCreature* pPeer =
                        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidPeer);
                    if (pPeer != 0 && pPeer->m_bInParty != 0)
                        g_pAppManager->m_pClientExoApp->GetInGameGui()->UpdateStatus(CGuiInGame::GUI_STATUS_ITEM_LOST, 0);
                }
            } else if (m_objectType == GAME_OBJECT_TYPE_PLACEABLE) {
                AsSWSPlaceable()->AcquireItem(&pItem, oidPeer, 1);
            }
            return ACTION_COMPLETE;
        }
    }
    return ACTION_FAILED;
}

int CSWSObject::AIActionSpeak(CSWSObjectActionNode* pNode) {
    unsigned int nVolume;
    CExoString sText;
    if (AsSWSCreature() != 0) {
        AsSWSCreature()->ActivityManager(2);
        AsSWSCreature()->SetActivity(CSWSCreature::ACTIVITY_STEALTH, 0, 0);
    }
    sText = *pNode->m_args.m_pSpeakText;
    nVolume = pNode->m_args.m_speakVolume;
    g_pAppManager->m_pServerExoApp->GetSWSMessage()->SendServerToPlayerChatMessage(
        static_cast<unsigned char>(nVolume), m_objectId, sText, static_cast<unsigned int>(-1), CExoString(""));
    return ACTION_COMPLETE;
}

int CSWSObject::AIActionPlaySound(CSWSObjectActionNode* pNode) {
    Vector vPos;
    CExoString sSound;
    sSound = *pNode->m_args.m_pSoundName;
    CSWSMessage* pMessage = g_pAppManager->m_pServerExoApp->GetSWSMessage();
    CExoLinkedList<CSWSClient>* ppPlayers = g_pAppManager->m_pServerExoApp->GetPlayerList();
    CExoLinkedListNode* pNodePos = ppPlayers->m_pInternal->m_pHead;
    while (pNodePos != 0) {
        float fMaxDistSq = CSWSArea::MAX_OBJECT_CHATTALK_DISTANCE *
                           CSWSArea::MAX_OBJECT_CHATTALK_DISTANCE;
        CSWSPlayer* pPlayer = static_cast<CSWSPlayer*>(ppPlayers->m_pInternal->GetAtPos(pNodePos));
        if (pPlayer->GetGameObject() != 0) {
            CSWSObject* pObject = pPlayer->GetGameObject();
            if (pObject != 0) {
                CSWArea* pTheirArea = pObject->GetArea();
                vPos = pObject->m_position;
                if (pTheirArea == GetArea()) {
                    float fDx = vPos.x - m_position.x;
                    float fDy = vPos.y - m_position.y;
                    float fDz = vPos.z - m_position.z;
                    if (fDx * fDx + fDy * fDy + fDz * fDz <= fMaxDistSq && pMessage != 0)
                        pMessage->SendServerToPlayerAIActionPlaySound(pPlayer->m_nPlayerId,
                                                                      m_objectId, sSound);
                }
            }
        }
        ppPlayers->m_pInternal->GetNext(pNodePos);
    }
    return ACTION_COMPLETE;
}

int CSWSObject::AIActionPlayAnimation(CSWSObjectActionNode* pNode) {
#if KOTOR1  // K1: the dying tests are GetDeadTemp() / GetIsPCDying()
    int bDying = 0;
    if (AsSWSCreature() != 0)
        bDying = AsSWSCreature()->GetDeadTemp();
    if (GetDead() == 1 || GetIsPCDying() == 1 || bDying == 1)
#else
    int bDying = 0;
    if (AsSWSCreature() != 0) {
        CSWSCreature* pCreature = AsSWSCreature();
        bDying = pCreature->GetDeadTemp();
    }
    int bDead = GetDead();
    if (bDying || bDead == 1)
#endif
        return ACTION_FAILED;

    int nAnimation = pNode->m_args.m_animationId;
    float fSpeed = pNode->m_args.m_fAnimSpeed;
    float fDuration = pNode->m_args.m_fAnimDuration;
    int nMode = pNode->m_args.m_nAnimMode;
    int bSoundPlayed = pNode->m_args.m_bAnimSoundPlayed;
    unsigned int nSoundDelay = pNode->m_args.m_nAnimSoundDelayMS;

    unsigned int nDay, nTod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);
    if (nMode == 1) {
        m_nAnimationStartDay = nDay;
        m_nAnimationStartTod = nTod;
        nMode = 0;
        pNode->m_args.m_nAnimMode = nMode;
        if (m_objectId == g_pAppManager->m_pServerExoApp->GetPlayerCreatureId()) {
            switch (nAnimation) {
            case SERVER_ANIMATION_UNLOCK_DOOR:
            case SERVER_ANIMATION_UNLOCK_CONTAINER:
                nSoundDelay = 250;
                break;
            case SERVER_ANIMATION_DISARM_MINE:
            case SERVER_ANIMATION_DISARM_DOOR_MINE:
            case SERVER_ANIMATION_DISARM_CONTAINER_MINE:
            case SERVER_ANIMATION_RECOVER_MINE:
                nSoundDelay = 750;
                break;
            case SERVER_ANIMATION_SET_MINE:
                nSoundDelay = 750;
                break;
            case SERVER_ANIMATION_DOOR_SABER:
                nSoundDelay = 0;
                break;
            default:
                bSoundPlayed = 1;
                pNode->m_args.m_bAnimSoundPlayed = bSoundPlayed;
                break;
            }
            pNode->m_args.m_nAnimSoundDelayMS = nSoundDelay;
        }
    }
    unsigned int nElapsedDay, nElapsedTod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTod, m_nAnimationStartDay, m_nAnimationStartTod, &nElapsedDay, &nElapsedTod);

    if (fDuration >= 30.0f) {
        int bStop = 0;
        while (((m_actionList.m_pInternal->m_pHead == 0) | bStop) == 0) {
            CSWSObjectActionNode* pQueued = static_cast<CSWSObjectActionNode*>(m_actionList.RemoveHead());
            if (pQueued != 0 && pQueued->m_nActionId == AIACTION_PLAY_ANIMATION && pQueued->m_args.m_animationId == nAnimation &&
                pQueued->m_args.m_fAnimSpeed == fSpeed) {
                delete pQueued;
                bStop = 0;
            } else {
                m_actionList.AddHead(pQueued);
                bStop = 1;
            }
        }
    }

    float fAbsSpeed = __builtin_fabsf(fSpeed);
    float fLength;
    int bClientPlayed;
    if (GetClientObject() != 0) {
        fLength = static_cast<CSWCObject*>(GetClientObject())->GetAnimationLength(static_cast<unsigned short>(nAnimation));
        fLength = fLength > 0.0f ? fLength : 1000.0f;
        bClientPlayed =
            static_cast<CSWCObject*>(GetClientObject())->AnimationFireAndForget(static_cast<unsigned short>(nAnimation)) != 0;
    } else {
        fLength = 1000.0f;
        bClientPlayed = 0;
    }
    if (fSpeed != 0.0f)
        fLength = fLength / fAbsSpeed;
    if (fDuration > 0.0f && bClientPlayed == 0)
        fLength = fDuration * 1000.0f;

    if (bSoundPlayed == 0 && nSoundDelay <= nElapsedTod) {
        bSoundPlayed = 1;
        pNode->m_args.m_bAnimSoundPlayed = bSoundPlayed;
        CExoString sSound;
        int bHasSound = 1;
        switch (nAnimation) {
        case SERVER_ANIMATION_UNLOCK_DOOR:
        case SERVER_ANIMATION_UNLOCK_CONTAINER:
            sSound = "gui_lockpick";
            break;
        case SERVER_ANIMATION_DISARM_MINE:
        case SERVER_ANIMATION_DISARM_DOOR_MINE:
        case SERVER_ANIMATION_DISARM_CONTAINER_MINE:
        case SERVER_ANIMATION_RECOVER_MINE:
            sSound = "gui_minedisarm";
            break;
        case SERVER_ANIMATION_SET_MINE:
            sSound = "gui_minearm";
            break;
#if !KOTOR1  // K1 has no door-saber animation
        case SERVER_ANIMATION_DOOR_SABER:
            sSound = "gui_doorsaber";
            break;
#endif
        default:
            bHasSound = 0;
            break;
        }
        if (bHasSound)
            g_pExoSound->PlayOneShotSound(CResRef(static_cast<char*>(sSound.CStr())), PRIORITY_GROUP_SINGLE_SHOT_POSITIONAL, 0, 0, 0.0f, 0.0f);
    }

    if (nElapsedTod < static_cast<unsigned int>(fLength)) {
        m_fAnimationSpeed = fSpeed;
        SetAnimation(static_cast<unsigned short>(nAnimation));
        return ACTION_IN_PROGRESS;
    }
    if (AsSWSCreature() != 0) {
        m_fAnimationSpeed = 1.0f;
        SetAnimation(SERVER_ANIMATION_READY);
    }
    return ACTION_COMPLETE;
}

int CSWSObject::AIActionUseObject(CSWSObjectActionNode* pNode) {
    unsigned long long oidTarget;
#if KOTOR1  // K1: the dying tests (ours and the target's) are GetDeadTemp() / GetIsPCDying()
    int bDying = 0;
    if (AsSWSCreature() != 0)
        bDying = AsSWSCreature()->GetDeadTemp();
    if (GetDead() == 1 || GetIsPCDying() == 1 || bDying == 1)
        goto failed;
#else
    int bDying = 0;
    if (AsSWSCreature() != 0) {
        CSWSCreature* pSelf = AsSWSCreature();
        bDying = pSelf->GetDeadTemp();
    }
    int bDead = GetDead();
    if (bDying || bDead == 1)
        goto failed;
#endif
    if (AsSWSCreature() != 0 && AsSWSCreature()->m_bInParty != 0 &&
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState != 0)
        goto failed;
    {
        oidTarget = pNode->m_args.m_aqTargetId;
        CGameObject* pObject;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(oidTarget, &pObject) !=
            CGameObjectArray::SUCCESS)
            goto failed;
        CSWSObject* pTarget = pObject->AsSWSObject();
        // The target's own "can it still be used" preamble, the same three steps as ours above.
#if KOTOR1
        int bTargetDying = 0;
        if (pTarget->AsSWSCreature() != 0)
            bTargetDying = pTarget->AsSWSCreature()->GetDeadTemp();
        if (pTarget->GetDead() == 1 || pTarget->GetIsPCDying() == 1 || bTargetDying == 1)
            goto failed;
#else
        int bTargetDying = 0;
        if (pTarget->AsSWSCreature() != 0) {
            CSWSCreature* pTargetCreature = pTarget->AsSWSCreature();
            bTargetDying = pTargetCreature->GetDeadTemp();
        }
        int bTargetDead = pTarget->GetDead();
        if (bTargetDying || bTargetDead == 1)
            goto failed;
#endif
        if (pTarget->GetArea() != 0 && AsSWSCreature() != 0) {
            CSWSCreature* pCreature = AsSWSCreature();
            pCreature->ActivityManager(2);
            pCreature->SetActivity(CSWSCreature::ACTIVITY_CONVERSATION, 0, 0);
            if (pCreature->GetIsInUseRange(oidTarget, 0.0f, 0, 0) == 0) {
                // Out of range, so re-queue the use behind a turn and a walk.
                Vector vPos;
                int nOne = 1;
                float fRange;
                pCreature->GetUseRange(oidTarget, vPos, fRange, 0, 0);
                int nOneAgain = 1;
                AddActionToFront(AIACTION_USE_OBJECT, pNode->m_nGroupId,
                                 ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                float fHalf = 0.5f;
                // This one goes on the creature, not on this, with a reserved group id.
                AsSWSCreature()->AddActionToFront(AIACTION_WAIT, ACTION_GROUP_PREVIOUS,
                                                  ACTION_PARAM_FLOAT, &fHalf, 0, 0, 0, 0, 0, 0, 0,
                                                  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                  0);
                AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, ACTION_GROUP_PREVIOUS,
                                 ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                AddActionToFront(AIACTION_CHECK_MOVE_TO_OBJECT_RADIUS, pNode->m_nGroupId,
                                 ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nOneAgain,
                                 ACTION_PARAM_FLOAT, &fRange, ACTION_PARAM_FLOAT, &fRange,
                                 ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0);
                pCreature->AddMoveToPointActionToFront(pNode->m_nGroupId, vPos, m_cachedAreaId,
                                                       oidTarget, nOneAgain, fRange, 0.0f, 0, 0, 0,
                                                       0, 0, 1, Vector{}, 0, 0, 0);
                return ACTION_COMPLETE;
            }
        }
        SetLockOrientationToObject(oidTarget);
        if (pTarget->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
            CSWSPlaceable* pPlaceable = pTarget->AsSWSPlaceable();
            if (pPlaceable->m_bUseable == 0)
                goto failed;
            if (pPlaceable->m_bTrapFlag != 0 && AsSWSCreature() != 0 &&
                pPlaceable->GetReputation(m_objectId, 0) <= CFactionManager::REPUTATION_FRIENDLY_ABOVE &&
                pPlaceable->m_nFaction != static_cast<unsigned int>(AsSWSCreature()->m_pStats->m_factionId))
                goto trapRefused;
            if (pPlaceable->m_bHasInventory == 0)
                goto signalUsed;
            if (pPlaceable->m_bLocked != 0) {
                if (AsSWSCreature() != 0 && AsSWSCreature()->EvaluateLock(pPlaceable, 0) == 0) {
                    CSWCCMessageData* pData = new CSWCCMessageData();
                    pData->SetObjectID(0, pPlaceable->m_objectId);
                    AsSWSCreature()->SendFeedbackMessage(FEEDBACK_OBJECT_LOCKED, pData);
                    goto signalUsed;
                }
                // Case A: the lock has just been evaluated open.
                CExoString sSound("");
                C2DA* p2DA = g_pRules->m_p2DArrays->m_pPlaceables;
                int nAppearance = pPlaceable->m_appearanceType;
                int nSoundRow;
                if (p2DA->GetINTEntry(nAppearance, CExoString("SoundAppType"), &nSoundRow) != 0) {
                    C2DA* pSounds = g_pRules->m_p2DArrays->m_pPlaceableSounds;
                    int nRow = nSoundRow;
                    if (pSounds->GetCExoStringEntry(nRow, CExoString("Opened"), &sSound) == 0)
                        sSound = "";
                }
                if (g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId) == 0) {
                    // Nobody is watching, so skip the GUI and just animate and make the noise.
                    if (pPlaceable->m_corpse.m_bUseAnimationPending != 0)
                        pPlaceable->m_corpse.m_bUseAnimationPending = 0;
                    SetAnimation(SERVER_ANIMATION_PLACEABLE_OPEN);
                    g_pExoSound->Play3DOneShotSound(CResRef(sSound), pPlaceable->m_position, 0.0f,
                                                    PRIORITY_GROUP_SINGLE_SHOT_POSITIONAL, 0, 0, 0.0f, 0.0f);
                    goto signalUsed;
                }
                if (pPlaceable->m_bInventoryOpen != 0)
                    return ACTION_COMPLETE;
                if (pPlaceable->m_corpse.m_bUseAnimationPending != 0) {
                    pPlaceable->OpenInventory(m_objectId, 0);
                    pPlaceable->m_corpse.m_bUseAnimationPending = 0;
                    pPlaceable->m_fAnimationSpeed = 1.0f;
                    goto signalUsed;
                }
                {
                    AddActionToFront(AIACTION_USE_OBJECT, pNode->m_nGroupId,
                                     ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                    float fWait;
                    CSWCObject* pClient = static_cast<CSWCObject*>(pPlaceable->GetClientPlaceable());
                    if (pClient == 0) {
                        fWait = 0.5f;
                    } else {
                        fWait = pClient->GetAnimationLength(ANIMATION_ROW_CLOSE2OPEN) / 1000.0f;
                        float fSound = static_cast<float>(static_cast<unsigned int>(g_pExoSound->Play3DOneShotSound(
                                           CResRef(sSound), pPlaceable->m_position, 0.0f, PRIORITY_GROUP_SINGLE_SHOT_POSITIONAL, 0,
                                           0, 0.0f, 0.0f))) /
                                       1000.0f;
                        if (fSound > fWait && fWait != 0.0f)
                            fWait = fSound;
                    }
                    AddActionToFront(AIACTION_WAIT, pNode->m_nGroupId, ACTION_PARAM_FLOAT, &fWait,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0);
                    pPlaceable->SetAnimation(SERVER_ANIMATION_PLACEABLE_OPEN);
                    pPlaceable->m_corpse.m_bUseAnimationPending = 1;
                    return ACTION_COMPLETE;
                }
            } else {
                // Case B: the container was never locked.
                CExoString sSound("");
                C2DA* p2DA = g_pRules->m_p2DArrays->m_pPlaceables;
                int nAppearance = pPlaceable->m_appearanceType;
                int nSoundRow;
                if (p2DA->GetINTEntry(nAppearance, CExoString("SoundAppType"), &nSoundRow) != 0) {
                    C2DA* pSounds = g_pRules->m_p2DArrays->m_pPlaceableSounds;
                    int nRow = nSoundRow;
                    if (pSounds->GetCExoStringEntry(nRow, CExoString("Opened"), &sSound) == 0)
                        sSound = "";
                }
                if (g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId) == 0) {
                    if (pPlaceable->m_corpse.m_bUseAnimationPending != 0)
                        pPlaceable->m_corpse.m_bUseAnimationPending = 0;
                    SetAnimation(SERVER_ANIMATION_PLACEABLE_OPEN);
                    g_pExoSound->Play3DOneShotSound(CResRef(sSound), pPlaceable->m_position, 0.0f,
                                                    PRIORITY_GROUP_SINGLE_SHOT_POSITIONAL, 0, 0, 0.0f, 0.0f);
                    goto signalUsed;
                }
                if (pPlaceable->m_bInventoryOpen != 0)
                    return ACTION_COMPLETE;
                {
                    CSWCObject* pClient = static_cast<CSWCObject*>(pPlaceable->GetClientPlaceable());
                    if (pPlaceable->m_corpse.m_bUseAnimationPending != 0) {
                        pPlaceable->OpenInventory(m_objectId, 0);
                        pPlaceable->m_corpse.m_bUseAnimationPending = 0;
                        pPlaceable->m_fAnimationSpeed = 1.0f;
                        goto signalUsed;
                    }
                    {
                        AddActionToFront(AIACTION_USE_OBJECT, pNode->m_nGroupId,
                                         ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                        float fWait;
                        if (pClient == 0) {
                            fWait = 0.5f;
                        } else {
                            fWait = pClient->GetAnimationLength(ANIMATION_ROW_CLOSE2OPEN) / 1000.0f;
                            float fSound = static_cast<float>(static_cast<unsigned int>(g_pExoSound->Play3DOneShotSound(
                                               CResRef(sSound), pPlaceable->m_position, 0.0f, PRIORITY_GROUP_SINGLE_SHOT_POSITIONAL,
                                               0, 0, 0.0f, 0.0f))) /
                                           1000.0f;
                            if (fSound > fWait && fWait != 0.0f)
                                fWait = fSound;
                        }
                        AddActionToFront(AIACTION_WAIT, pNode->m_nGroupId,
                                         ACTION_PARAM_FLOAT, &fWait, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                        pPlaceable->SetAnimation(SERVER_ANIMATION_PLACEABLE_OPEN);
                        pPlaceable->m_corpse.m_bUseAnimationPending = 1;
                        return ACTION_COMPLETE;
                    }
                }
            }
        } else if (pTarget->m_objectType == CGameObject::OBJECT_TYPE_ITEM) {
            // A container item (baseitems.2da's container flag). Using it toggles: if the player already
            // has this item open it shuts, otherwise it opens.
            CSWSItem* pItem = pTarget->AsSWSItem();
            if (g_pRules->GetBaseItem(pItem->m_baseItem)->m_bContainer != 0) {
                CSWSPlayer* pPlayer =
                    g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(m_objectId);
                if (pPlayer != 0) {
                    if (pPlayer->m_pContainerGUI->m_objectId == oidTarget)
                        pItem->CloseInventory(m_objectId, 1);
                    else
                        pItem->OpenInventory(m_objectId);
                }
            }
        }
    }
    return ACTION_COMPLETE;
trapRefused : {
    CScriptEvent* pEvent = new CScriptEvent();
    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_MINE_TRIGGERED;
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
        0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
}
failed:
    return ACTION_FAILED;
signalUsed : {
    CScriptEvent* pEvent = new CScriptEvent();
    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_USED;
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
        0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
}
    return ACTION_COMPLETE;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int CSWSObject::AIActionDoorSaber(CSWSObjectActionNode* pNode) {
    unsigned long long oidTarget;
    CSWSCreature* pCreature;
    int bDying = 0;
    if (AsSWSCreature() != 0) {
        CSWSCreature* pSelf = AsSWSCreature();
        bDying = pSelf->GetDeadTemp();
    }
    // Plain truthiness here, not the unlock twin's `bDead == 1`.
    int bDead = GetDead();
    if (bDying || bDead) {
        pCreature = AsSWSCreature();
        goto failed;
    }
    pCreature = AsSWSCreature();
    if (pCreature != 0)
        AsSWSCreature()->ActivityManager(2);
    {
        oidTarget = pNode->m_args.m_aqTargetId;
        CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
        CSWSObject* pTarget = 0;
        if (pObject != 0)
            pTarget = pObject->AsSWSObject();
        pCreature = AsSWSCreature();
        if (pObject == 0)
            goto failed;
        if (pTarget == 0)
            goto failed;
        {
            CSWItem* pRightHand = pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON);
            if (pRightHand != 0 && pRightHand->GetBaseItem() != 0 &&
                pRightHand->GetBaseItem()->m_bPoweredItem != 0)
                static_cast<CSWSItem*>(pRightHand)->GetClientItem()->PowerItem(1, 0, 1, 0);
        }
        {
            CSWItem* pLeftHand = AsSWSCreature()->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON);
            if (pLeftHand != 0 && pLeftHand->GetBaseItem() != 0 &&
                pLeftHand->GetBaseItem()->m_bPoweredItem != 0)
                static_cast<CSWSItem*>(pLeftHand)->GetClientItem()->PowerItem(1, 0, 1, 0);
        }
#if KOTOR1
        AsSWSCreature()->SetCombatState(1, 2);
#else
        AsSWSCreature()->SetCombatState(1, 2, 0);
#endif
        pCreature = AsSWSCreature();
        if (pCreature != 0) {
            Vector vPos;
            float fRange = 0.0f;
            int bInRange = AsSWSCreature()->GetIsInUseRange(oidTarget, 0.0f, 0, 1);
            pCreature = AsSWSCreature();
            if (bInRange == 0) {
                // GetUseRange's last argument is 1 here and 0 on the in-range path below.
                pCreature->GetUseRange(oidTarget, vPos, fRange, 0, 1);
                AddActionToFront(AIACTION_DOOR_SABER, pNode->m_nGroupId,
                                 ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                                 ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                AsSWSCreature()->AddMoveToPointActionToFront(pNode->m_nGroupId, vPos,
                                                             pTarget->m_cachedAreaId, oidTarget, 1,
                                                             fRange, 0.0f, 0, 0, 0, 0, 0, 0,
                                                             Vector{}, 0, 0, 0);
                return ACTION_COMPLETE;
            }
            // In range on the first tick, re-queue the pair so the use-object check turns us to
            // face the door, and latch.
            // No walk is queued on this path.
            if (pCreature->m_bActionApproachDone == 0) {
                AsSWSCreature()->m_bActionApproachDone = 1;
                AsSWSCreature()->GetUseRange(oidTarget, vPos, fRange, 0, 0);
                AddActionToFront(AIACTION_DOOR_SABER, pNode->m_nGroupId,
                                 ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                AddActionToFront(AIACTION_CHANGE_FACING_OBJECT, pNode->m_nGroupId,
                                 ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                return ACTION_COMPLETE;
            }
        }
        // Tick 2: latch, face the door, play the cut animation and run the timing bar.
        if (AsSWSCreature() != 0 && AsSWSCreature()->m_bDoorSaberStarted == 0) {
            AsSWSCreature()->m_bDoorSaberStarted = 1;
            SetLockOrientationToObject(oidTarget);
            int nAnim = SERVER_ANIMATION_DOOR_SABER;
            int nOne = 1;
            float fIn = 1.0f;
            float fOut = 2.0f;
            AddActionToFront(AIACTION_DOOR_SABER, pNode->m_nGroupId,
                             ACTION_PARAM_OBJECT, &oidTarget, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            AddActionToFront(AIACTION_PLAY_ANIMATION, pNode->m_nGroupId, ACTION_PARAM_INT, &nAnim,
                             ACTION_PARAM_FLOAT, &fIn, ACTION_PARAM_FLOAT, &fOut,
                             ACTION_PARAM_INT, &nOne, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0);
            AsSWSCreature()->StartGuiTimingBar(2000, 9);
            return ACTION_COMPLETE;
        }
        {
            CSWSDoor* pDoor = pTarget->AsSWSDoor();
            // A trapped door of a faction we are not friendly to refuses the whole action.
            if (pDoor != 0 && pDoor->m_bTrapFlag != 0) {
                pCreature = AsSWSCreature();
                if (pCreature != 0 && pDoor->GetReputation(m_objectId, 0) <= CFactionManager::REPUTATION_FRIENDLY_ABOVE &&
                    pDoor->m_nFaction != static_cast<unsigned int>(AsSWSCreature()->m_pStats->m_factionId))
                    goto trapRefused;
            }
            if (AsSWSCreature() != 0 &&
                AsSWSCreature()->GetIsInUseRange(oidTarget, 0.0f, 0, 1) == 0)
                goto complete;
            if (pDoor != 0 && pDoor->m_bLocked == 0) {
                pCreature = AsSWSCreature();
                if (pCreature != 0)
                    AsSWSCreature()->SendFeedbackMessage(FEEDBACK_OBJECT_NOT_LOCKED, 0);
                goto failed;
            }
            // This else arm is only reached with AsSWSCreature() == 0, so the repeated test is dead.
            if (AsSWSCreature() != 0) {
                pTarget->AsSWSDoor()->m_bLocked = 0;
                pTarget->AsSWSDoor()->OpenDoor(m_objectId);
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_UNLOCK_OBJECT, 0);
                if (AsSWSCreature() != 0 && rand_wincompatible() % 100 <= 19)
                    AsSWSCreature()->BroadcastVoiceChat(CSWSCreature::VOICE_CHAT_UNLOCK_SUCCESS);
            } else if (AsSWSCreature() != 0) {
                AsSWSCreature()->BroadcastVoiceChat(CSWSCreature::VOICE_CHAT_UNLOCK_FAILED);
            }
        }
    }
complete:
    if (AsSWSCreature() != 0) {
        AsSWSCreature()->StopGuiTimingBar();
        AsSWSCreature()->m_bDoorSaberStarted = 0;
        AsSWSCreature()->m_bActionApproachDone = 0;
    }
    return ACTION_COMPLETE;
trapRefused : {
    CScriptEvent* pEvent = new CScriptEvent();
    pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_MINE_TRIGGERED;
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
        0, 0, m_objectId, oidTarget, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
}
failed:
    if (pCreature != 0) {
        AsSWSCreature()->StopGuiTimingBar();
        AsSWSCreature()->m_bDoorSaberStarted = 0;
        AsSWSCreature()->m_bActionApproachDone = 0;
    }
    return ACTION_FAILED;
}
#endif // !KOTOR1

int CSWSObject::AIActionSwitchWeapons(CSWSObjectActionNode* pNode) {
    K2_UNUSED(pNode);
    CSWSCreature* pCreature = AsSWSCreature();
    ClearAllActions(1);
    if (pCreature == 0)
        return ACTION_COMPLETE;
    CSWSItem* pRightHand = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
    CSWSItem* pLeftHand = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
    CSWSItem* pAltRightHand = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON2));
    CSWSItem* pAltLeftHand = static_cast<CSWSItem*>(pCreature->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON2));
    pCreature->ClearAllActions(1);
    pCreature->m_pCombatRound->RemoveAllActions();
    int bKeepMoving = (pAltRightHand != 0 || pAltLeftHand != 0);
    if (g_pAppManager->m_pClientExoApp->GetPlayerMovementInput() != 0)
        bKeepMoving = 1;
    if (pAltLeftHand != 0)
        pCreature->RunUnequip(pAltLeftHand->m_objectId, CGameObjectArray::INVALIDOBJID, 0, 0, 0,
                              bKeepMoving);
    if (pAltRightHand != 0)
        pCreature->RunUnequip(pAltRightHand->m_objectId, CGameObjectArray::INVALIDOBJID, 0, 0, 0,
                              bKeepMoving);
    if (pLeftHand != 0)
        pCreature->RunUnequip(pLeftHand->m_objectId, CGameObjectArray::INVALIDOBJID, 0, 0, 0,
                              bKeepMoving);
    if (pRightHand != 0)
        pCreature->RunUnequip(pRightHand->m_objectId, CGameObjectArray::INVALIDOBJID, 0, 0, 0,
                              bKeepMoving);
    if (pAltRightHand != 0)
        pCreature->RunEquip(pAltRightHand->m_objectId, CSWInventory::SLOT_RIGHTWEAPON, 0);
    if (pAltLeftHand != 0)
        pCreature->RunEquip(pAltLeftHand->m_objectId, CSWInventory::SLOT_LEFTWEAPON, 0);
    if (pRightHand != 0)
        pCreature->RunEquip(pRightHand->m_objectId, CSWInventory::SLOT_RIGHTWEAPON2, 0);
    if (pLeftHand != 0)
        pCreature->RunEquip(pLeftHand->m_objectId, CSWInventory::SLOT_LEFTWEAPON2, 0);
    if (pCreature->m_bCombatState != 0 && pCreature->m_nCombatType == 1 &&
        pCreature->m_pCombatRound->m_oidAttackTarget != OBJECT_INVALID)
        pCreature->AddAttackActions(pCreature->m_pCombatRound->m_oidAttackTarget, 0, 0, 0, 0, 0, 0, 1,
                                    SERVER_ANIMATION_ATTACK, 1500, 1, 0, 0, 4, 0);
    return ACTION_COMPLETE;
}

extern int OverrideReputation;

int CSWSObject::GetReputation(unsigned long long objectId, int a) {
    int nTheirFaction;
    int nMyFaction;
    if (OverrideReputation != -1)
        return OverrideReputation;
    CGameObject* pObject;
    if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(objectId, &pObject)
            != CGameObjectArray::SUCCESS)
        return 50;
    CFactionManager* pFactions =
        g_pAppManager->m_pServerExoApp->m_pInternal->m_pFactionManager;
    if (pFactions == 0)
        return 50;
    if (pObject->m_objectType == CGameObject::OBJECT_TYPE_AREAOFEFFECTOBJECT &&
        pObject->AsSWSAreaOfEffectObject() != 0)
        objectId = pObject->AsSWSAreaOfEffectObject()->m_oidCreator;
    if (AsSWSAreaOfEffectObject() != 0) {
        CGameObject* pCreator;
        if (g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(
                AsSWSAreaOfEffectObject()->m_oidCreator, &pCreator)
                != CGameObjectArray::SUCCESS)
            return 50;
        return pCreator->AsSWSObject()->GetReputation(objectId, a);
    }
    if (g_pAppManager->m_pServerExoApp->GetFactionOfObject(objectId, &nTheirFaction) == 0)
        return 50;
    if (g_pAppManager->m_pServerExoApp->GetFactionOfObject(m_objectId, &nMyFaction) == 0)
        return 50;
    if (objectId == m_objectId)
        return 100;
    if (pFactions->GetIsNPCFaction(nTheirFaction) == 0 &&
        pFactions->GetIsNPCFaction(nMyFaction) == 1)
        return pObject->AsSWSObject()->GetReputation(m_objectId, 0);
    int nReputation;
    if (AsSWSCreature() != 0) {
        nReputation = AsSWSCreature()->GetCreatureReputation(objectId, nTheirFaction);
    } else if (AsSWSPlaceable() != 0 || AsSWSEncounter() != 0 || AsSWSDoor() != 0 ||
               AsSWSTrigger() != 0) {
        nReputation = pFactions->GetNPCFactionReputation(nTheirFaction, nMyFaction);
    } else {
        nReputation = 0;
    }
    if (nReputation < 0)
        nReputation = 0;
    else if (nReputation > 100)
        nReputation = 100;
    return nReputation;
}

int CSWSObject::GetAcceptableAction(unsigned int nActionId) {
    switch (nActionId) {
#if KOTOR1  // K1 has no action ids above 63
    case AIACTION_MOVE_TO_POINT: case AIACTION_PICK_UP_ITEM: case AIACTION_DROP_ITEM:
    case AIACTION_PHYSICAL_ATTACKS: case AIACTION_CAST_SPELL: case AIACTION_OPEN_DOOR:
    case AIACTION_CLOSE_DOOR: case AIACTION_DIALOG_OBJECT: case AIACTION_DISARM_MINE:
    case AIACTION_RECOVER_MINE: case AIACTION_FLAG_MINE: case AIACTION_EXAMINE_MINE:
    case AIACTION_SET_MINE: case AIACTION_WAIT: case AIACTION_UNLOCK_OBJECT:
    case AIACTION_LOCK_OBJECT: case AIACTION_USE_OBJECT: case AIACTION_ANIMAL_EMPATHY:
    case AIACTION_REST: case AIACTION_TAUNT: case AIACTION_ITEM_CAST_SPELL:
    case AIACTION_COUNTER_SPELL: case AIACTION_PICK_POCKET: case AIACTION_FORCE_FOLLOW_OBJECT:
    case AIACTION_HEAL: case AIACTION_PARTY_FOLLOW_LEADER: case AIACTION_COMBAT:
#else
    case AIACTION_MOVE_TO_POINT: case AIACTION_PICK_UP_ITEM: case AIACTION_DROP_ITEM:
    case AIACTION_PHYSICAL_ATTACKS: case AIACTION_CAST_SPELL: case AIACTION_OPEN_DOOR:
    case AIACTION_CLOSE_DOOR: case AIACTION_DIALOG_OBJECT: case AIACTION_DISARM_MINE:
    case AIACTION_RECOVER_MINE: case AIACTION_FLAG_MINE: case AIACTION_EXAMINE_MINE:
    case AIACTION_SET_MINE: case AIACTION_WAIT: case AIACTION_UNLOCK_OBJECT:
    case AIACTION_LOCK_OBJECT: case AIACTION_USE_OBJECT: case AIACTION_ANIMAL_EMPATHY:
    case AIACTION_REST: case AIACTION_TAUNT: case AIACTION_ITEM_CAST_SPELL:
    case AIACTION_COUNTER_SPELL: case AIACTION_PICK_POCKET: case AIACTION_FORCE_FOLLOW_OBJECT:
    case AIACTION_HEAL: case AIACTION_PARTY_FOLLOW_LEADER: case AIACTION_COMBAT:
    case AIACTION_DOOR_SABER: case AIACTION_STANCE_TOTAL_DEFENSE: case AIACTION_STANCE_MEDITATIVE:
    case AIACTION_FOLLOW_OWNER: case AIACTION_SWITCH_WEAPONS:
#endif
        return 1;
    }
    return 0;
}

int CSWSObject::TerminateAISliceAfterAction(unsigned int nActionId) {
    switch (nActionId) {
    case AIACTION_MOVE_TO_POINT: case AIACTION_JUMP_TO_POINT: case AIACTION_JUMP_TO_OBJECT:
    case AIACTION_DRIVE:
        return 1;
    }
    return 0;
}

unsigned short CSWSObject::GetAQActionIDByID(unsigned short nActionId, int a) {
    if (a != 0) {
        if (nActionId == AIACTION_WAIT)
            return AQACTION_WAIT_ALT;
        if (nActionId == AIACTION_UNEQUIP_ITEM)
            return AQACTION_UNEQUIP_ITEM;
        return AQACTION_NONE;
    }
    switch (nActionId) {
    case AIACTION_MOVE_TO_POINT: return AQACTION_MOVE_TO_POINT;
    case AIACTION_PICK_UP_ITEM: return AQACTION_PICK_UP_ITEM;
    case AIACTION_DROP_ITEM: return AQACTION_DROP_ITEM;
    case AIACTION_PHYSICAL_ATTACKS: return AQACTION_PHYSICAL_ATTACKS;
    case AIACTION_CAST_SPELL: return AQACTION_CAST_SPELL;
    case AIACTION_OPEN_DOOR: return AQACTION_OPEN_DOOR;
    case AIACTION_CLOSE_DOOR: return AQACTION_CLOSE_DOOR;
    case AIACTION_DIALOG_OBJECT: return AQACTION_DIALOG_OBJECT;
    case AIACTION_DISARM_MINE: return AQACTION_DISARM_MINE;
    case AIACTION_RECOVER_MINE: return AQACTION_RECOVER_MINE;
    case AIACTION_FLAG_MINE: return AQACTION_FLAG_MINE;
    case AIACTION_EXAMINE_MINE: return AQACTION_EXAMINE_MINE;
    case AIACTION_SET_MINE: return AQACTION_SET_MINE;
    case AIACTION_WAIT: return AQACTION_WAIT;
    case AIACTION_UNLOCK_OBJECT: return AQACTION_UNLOCK_OBJECT;
    case AIACTION_LOCK_OBJECT: return AQACTION_LOCK_OBJECT;
    case AIACTION_USE_OBJECT: return AQACTION_USE_OBJECT;
    case AIACTION_ANIMAL_EMPATHY: return AQACTION_ANIMAL_EMPATHY;
    case AIACTION_REST: return AQACTION_REST;
    case AIACTION_TAUNT: return AQACTION_TAUNT;
    case AIACTION_ITEM_CAST_SPELL: return AQACTION_ITEM_CAST_SPELL;
    case AIACTION_COUNTER_SPELL: return AQACTION_COUNTER_SPELL;
    case AIACTION_PICK_POCKET: return AQACTION_PICK_POCKET;
    case AIACTION_FORCE_FOLLOW_OBJECT: return AQACTION_FORCE_FOLLOW_OBJECT;
    case AIACTION_HEAL: return AQACTION_HEAL;
    case AIACTION_PARTY_FOLLOW_LEADER: return AQACTION_PARTY_FOLLOW_LEADER;
    case AIACTION_COMBAT: return AQACTION_COMBAT;
#if !KOTOR1  // K1 has no actions 67..71
    case AIACTION_DOOR_SABER: return AQACTION_DOOR_SABER;
    case AIACTION_STANCE_TOTAL_DEFENSE: return AQACTION_STANCE_TOTAL_DEFENSE;
    case AIACTION_STANCE_MEDITATIVE: return AQACTION_STANCE_MEDITATIVE;
    case AIACTION_FOLLOW_OWNER: return AQACTION_FOLLOW_OWNER;
    case AIACTION_SWITCH_WEAPONS: return AQACTION_SWITCH_WEAPONS;
#endif
    }
    return AQACTION_NONE;
}

// The inverse of GetAQActionIDByID; queue id 34 (pick pocket) has no way back.
unsigned short CSWSObject::GetIDByAQActionID(unsigned short nAQActionId) {
    switch (nAQActionId) {
    case AQACTION_MOVE_TO_POINT: return AIACTION_MOVE_TO_POINT;
    case AQACTION_PICK_UP_ITEM: return AIACTION_PICK_UP_ITEM;
    case AQACTION_DROP_ITEM: return AIACTION_DROP_ITEM;
    case AQACTION_PHYSICAL_ATTACKS: return AIACTION_PHYSICAL_ATTACKS;
    case AQACTION_CAST_SPELL: return AIACTION_CAST_SPELL;
    case AQACTION_OPEN_DOOR: return AIACTION_OPEN_DOOR;
    case AQACTION_CLOSE_DOOR: return AIACTION_CLOSE_DOOR;
    case AQACTION_DIALOG_OBJECT: return AIACTION_DIALOG_OBJECT;
    case AQACTION_DISARM_MINE: return AIACTION_DISARM_MINE;
    case AQACTION_RECOVER_MINE: return AIACTION_RECOVER_MINE;
    case AQACTION_FLAG_MINE: return AIACTION_FLAG_MINE;
    case AQACTION_EXAMINE_MINE: return AIACTION_EXAMINE_MINE;
    case AQACTION_SET_MINE: return AIACTION_SET_MINE;
    case AQACTION_UNLOCK_OBJECT: return AIACTION_UNLOCK_OBJECT;
    case AQACTION_LOCK_OBJECT: return AIACTION_LOCK_OBJECT;
    case AQACTION_USE_OBJECT: return AIACTION_USE_OBJECT;
    case AQACTION_ANIMAL_EMPATHY: return AIACTION_ANIMAL_EMPATHY;
    case AQACTION_REST: return AIACTION_REST;
    case AQACTION_TAUNT: return AIACTION_TAUNT;
    case AQACTION_ITEM_CAST_SPELL: return AIACTION_ITEM_CAST_SPELL;
    case AQACTION_WAIT_ALT: return AIACTION_WAIT;
    case AQACTION_UNEQUIP_ITEM: return AIACTION_UNEQUIP_ITEM;
    case AQACTION_COUNTER_SPELL: return AIACTION_COUNTER_SPELL;
    case AQACTION_HEAL: return AIACTION_HEAL;
    case AQACTION_FORCE_FOLLOW_OBJECT: return AIACTION_FORCE_FOLLOW_OBJECT;
    case AQACTION_WAIT: return AIACTION_WAIT;
    case AQACTION_PARTY_FOLLOW_LEADER: return AIACTION_PARTY_FOLLOW_LEADER;
    case AQACTION_COMBAT: return AIACTION_COMBAT;
#if !KOTOR1  // K1 has no AQ actions 40..44
    case AQACTION_DOOR_SABER: return AIACTION_DOOR_SABER;
    case AQACTION_STANCE_TOTAL_DEFENSE: return AIACTION_STANCE_TOTAL_DEFENSE;
    case AQACTION_STANCE_MEDITATIVE: return AIACTION_STANCE_MEDITATIVE;
    case AQACTION_FOLLOW_OWNER: return AIACTION_FOLLOW_OWNER;
    case AQACTION_SWITCH_WEAPONS: return AIACTION_SWITCH_WEAPONS;
#endif
    }
    return AQACTION_NONE;
}

int CSWSObject::AddUseObjectAction(unsigned long long objectId) {
    if (m_bCommandable == 0)
        return 0;
    AddAction(AIACTION_USE_OBJECT, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &objectId, 0, 0, 0, 0, 0,
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return 1;
}

int CSWSObject::AddGiveItemActions(unsigned long long oidItem, unsigned long long oidTarget,
                                   int nStackSize, int a4) {
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    if (pObject == 0 || pObject->AsSWSObject() == 0)
        return 0;
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
    if (pItem == 0)
        return 0;
    CSWSCreature* pSelf = AsSWSCreature();
    CSWSCreature* pPossessor =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(pItem->m_possessorId);
    if (!(pSelf != 0 && pPossessor != 0 && pPossessor->m_bInParty != 0 &&
          pSelf->m_bInParty != 0) &&
        pItem->m_possessorId != m_objectId)
        return 0;
    if (m_bCommandable == 0)
        return 0;
    if (pItem->m_stackCount < nStackSize)
        nStackSize = pItem->m_stackCount;
    AddAction(AIACTION_GIVE_ITEM, ACTION_GROUP_PREVIOUS, ACTION_PARAM_OBJECT, &oidItem,
              ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &nStackSize, ACTION_PARAM_INT, &a4,
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return 1;
}

int CSWSObject::AddTakeItemActions(unsigned long long oidItem, unsigned long long oidTarget,
                                   int a3) {
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(oidTarget);
    if (pObject == 0)
        return 0;
    CSWSObject* pTarget = pObject->AsSWSObject();
    if (pTarget == 0)
        return 0;
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(oidItem);
    if (pItem == 0)
        return 0;
    if (pItem->m_possessorId == m_objectId)
        return 0;
    if (m_bCommandable == 0)
        return 0;
    if (AsSWSCreature() != 0) {
        float fDx = m_position.x - pTarget->m_position.x;
        float fDy = m_position.y - pTarget->m_position.y;
        float fDz = m_position.z - pTarget->m_position.z;
        int bOutOfRange = fDx * fDx + fDy * fDy + fDz * fDz > 25.0f;
        AsSWSCreature()->AddMoveToPointAction(
            ACTION_GROUP_NEW, pTarget->m_position,
            static_cast<CSWSArea*>(pTarget->GetArea())->m_objectId, oidTarget,
            bOutOfRange, 1.0f, 0.0f,
            0, 0, 0, 0, 0, 0, Vector{}, 0, 0, 0);
    }
    AddAction(AIACTION_TAKE_ITEM, ACTION_GROUP_NEW, ACTION_PARAM_OBJECT, &oidItem,
              ACTION_PARAM_OBJECT, &oidTarget, ACTION_PARAM_INT, &a3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return 1;
}

int CSWSObject::AddDoCommandAction(void* pScriptSituation) {
    if (m_bCommandable == 0) {
        g_pVirtualMachine->DeleteScriptSituation(pScriptSituation);
        return 0;
    }
    AddAction(AIACTION_DO_COMMAND, ACTION_GROUP_NEW, ACTION_PARAM_POINTER, pScriptSituation, 0, 0,
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return 1;
}

unsigned char CSWSObject::GetAIStateReputation(unsigned long long objectId) {
    GetArea();
    CGameObject* pObject;
    g_pAppManager->m_pServerExoApp->GetObjectArray()->GetGameObject(objectId, &pObject);
    unsigned char nState = 0;
    if (pObject != 0 && pObject->AsSWSObject() != 0) {
        int nReputation = pObject->AsSWSObject()->GetReputation(m_objectId, 0);
        if (nReputation < 11)
            nState = 2;
        else if (nReputation > 89)
            nState = 1;
    }
    return nState;
}
