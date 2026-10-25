#include "k2stdint.h"
#include "nwsdoor.h"
#include "nwcdoor.h"
#include "worldtimer.h"
#include "exobase.h"
#include "globalvartables.h"
#include "nwscript.h"
#include "nwseffect.h"
#include "nwseffectlist.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "serveraimaster.h"
#include "nwsscriptevent.h"
#include "nwsobject.h"
#include "nwmessage.h"
#include "nwscombatround.h"
#include "nwscreature.h"
#include "nwscreaturestats.h"
#include "nwsitem.h"
#include "nwsplayer.h"
#include "nwstrigger.h"
#include "partytables.h"
#include "swparty.h"
#include "guiingame.h"
#include "swguiexamine.h"
#include "exotlktable.h"
#include "exolocstringinternal.h"
#include "nwsaiaction.h"
#include "nwsscriptevent.h"
#include "serveranimation.h"
#include "restype.h"
#include "factionmanager.h"
#include "nwscriptconst.h"
#include "strref.h"

// The type a feedback or combat-log line is filed under (CGuiInGame's message buffers).
enum {
    kMessageTypeFeedback = 0x80,
};

// The portrait a door shows when its template names none.
enum {
    PORTRAIT_ID_DOOR_DEFAULT = 558,
};

int nRenderDoorBBox;

extern int g_bEnableCombatDebugging;

#include "port/libcstring.h"
#include "exostringinline.h"

CSWSDoor* CSWSDoor::AsSWSDoor() {
    return this;
}

CExoLocString& CSWSDoor::GetFirstName() {
    return m_firstName;
}

CResRef CSWSDoor::GetDialogResref() {
    return m_dialogResref;
}

// Runs the door's OnHeartbeat script at most every six seconds of world time, then ages the effect
// list and advances the action queue. The gate is on the time-of-day delta alone, so a heartbeat
// straddling midnight is skipped, and a first tick after load only stamps the timestamps.
void CSWSDoor::AIUpdate() {
    unsigned long long timerSnapshot = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    unsigned int day, tod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->GetWorldTime(&day, &tod);
    unsigned int elapsedDay, elapsedTod;
    g_pAppManager->m_pServerExoApp->GetWorldTimer()->SubtractWorldTimes(
        day, tod, m_lastUpdateDay, m_lastUpdateTod, &elapsedDay, &elapsedTod);
    if ((elapsedTod >> 4) >= HEARTBEAT_INTERVAL_16MS) {
        if (m_lastUpdateTod != 0 && GetDead() == 0) {
#if !KOTOR1  // K1: no PC-level-at-spawn snapshot
            if (m_nPCLevelAtSpawn == 0) {
                char level = 0;
                CSWGlobalVariableTable* pTable =
                    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable();
                {
                    CExoString name("G_PC_LEVEL");
                    pTable->GetValueNumber(name, level);
                }
                m_nPCLevelAtSpawn = level;
            }
#endif
            g_pVirtualMachine->RunScript(&m_scriptSlots[5], m_objectId, 1);
        }
        m_lastUpdateDay = day;
        m_lastUpdateTod = tod;
    }
    UpdateEffectList(day, tod);
    RunActions(day, tod, timerSnapshot);
}

void CSWSDoor::EventHandler(unsigned int nEventType, unsigned long long nCallerId, void* pData,
                            unsigned int a4, unsigned int a5) {
    if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_FEEDBACK_MESSAGE)) {
        CSWCCMessageData* pMsg = static_cast<CSWCCMessageData*>(pData);
        if (pMsg == 0) return;
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nCallerId);
        // A switch rather than an if/else chain, since the second case shares the default's block.
        switch (pMsg->m_nType) {
        case CSWCCMessageData::MESSAGE_TYPE_SAVING_THROW:
            if (pCreature != 0) {
                pCreature->BroadcastSavingThrowData(pMsg);
            }
            break;
        case CSWCCMessageData::MESSAGE_TYPE_FEEDBACK_TO_CALLER:
            if (pCreature != 0) {
                // The message is handed to the creature and not destroyed on this path.
                pCreature->SendFeedbackMessage(static_cast<unsigned short>(pMsg->GetInteger(0)), pMsg);
                return;
            }
            break;
        default:
            if (pCreature != 0) {
                pCreature->SendFeedbackMessage(static_cast<unsigned short>(pMsg->GetInteger(0)), pMsg);
                return;
            }
            break;
        }
        pMsg->~CSWCCMessageData();
        operator delete(pMsg);
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_ON_MELEE_ATTACKED)) {
        CSWSItem* pWeapon = 0;
        CSWSCombatAttackData* pAttack = static_cast<CSWSCombatAttackData*>(pData);
        CSWSCreature* pAttacker = 0;
        if (pAttack != 0) {
            CSWCCMessageData* pMsg = new CSWCCMessageData();
            pMsg->SetObjectID(0, nCallerId);
            pMsg->SetObjectID(1, m_objectId);
            if (g_bEnableCombatDebugging) {
                pMsg->SetString(0, pAttack->m_sAttackDebugText);
            }
            pAttacker = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nCallerId);
            if (pAttacker != 0) {
                pAttacker->BroadcastAttackDataToParty(pMsg, pAttack);
                pAttacker->RemoveCombatInvisibilityEffects();
            }
            pMsg->~CSWCCMessageData();
            operator delete(pMsg);
            m_nLastAttackFeatId = pAttack->m_nFeatId;
            m_nLastAttackMode = pAttack->m_nAttackMode;
            // The attacker is not re-checked here, so a melee event from a non-creature crashes.
#ifdef K2_FIX_ENGINE_BUGS
            if (pAttacker != 0)
#endif
            switch (pAttack->m_nWeaponAttackType) {
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_ONHAND:
                pWeapon = static_cast<CSWSItem*>(pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
                break;
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_OFFHAND:
                pWeapon = static_cast<CSWSItem*>(pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
                break;
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON1:
                pWeapon = static_cast<CSWSItem*>(pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_CWEAPON_L));
                break;
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON2:
                pWeapon = static_cast<CSWSItem*>(pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_CWEAPON_R));
                break;
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON3:
                pWeapon = static_cast<CSWSItem*>(pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_CWEAPON_B));
                break;
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_EXTRA:
                pWeapon = static_cast<CSWSItem*>(pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
                break;
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_UNARMED:
                pWeapon = static_cast<CSWSItem*>(pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_HANDS));
                break;
            case CSWSCombatRound::WEAPON_ATTACK_TYPE_UNARMED_EXTRA:
                pWeapon = static_cast<CSWSItem*>(pAttacker->m_pInventory->GetItemInSlot(CSWInventory::SLOT_HANDS));
                break;
            default: break;
            }
        }
        m_oidLastAttacker = nCallerId;
        SetLastHostileActor(nCallerId, 0);
        if (pAttacker != 0) {
            pAttacker->BroadcastCombatStateToParty(OBJECT_INVALID);
            pAttacker->RemoveCombatInvisibilityEffects();
            if (pWeapon != 0) {
                pAttacker->m_oidLastWeaponUsed = pWeapon->m_objectId;
            } else {
                pAttacker->m_oidLastWeaponUsed = OBJECT_INVALID;
            }
        }
        g_pVirtualMachine->RunScript(&m_scriptSlots[7], m_objectId, 1);  // OnMeleeAttacked
        // A trapped door struck from within four metres arms itself: schedules the delayed
        // open-lock and trap event back at the attacker.
        if (pAttacker != 0 && m_bTrapFlag == 1) {
            Vector vToAttacker = pAttacker->m_position - m_position;
            if (vToAttacker.x * vToAttacker.x + vToAttacker.y * vToAttacker.y +
                    vToAttacker.z * vToAttacker.z <=
                16.0f) {
                CScriptEvent* pEvent = new CScriptEvent();
                pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_MINE_TRIGGERED;
                g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                    0, 0, nCallerId, m_objectId, CServerAIMaster::EVENT_SIGNAL_EVENT, pEvent);
            }
        }
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_SIGNAL_EVENT)) {
        CScriptEvent* pEvent = static_cast<CScriptEvent*>(pData);
        switch (pEvent->m_eventType) {
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_HEARTBEAT:
            m_lastUpdateDay = a4;
            m_lastUpdateTod = a5;
            g_pVirtualMachine->RunScript(&m_scriptSlots[5], m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_SPELLCASTAT:
            m_oidLastSpellCaster = pEvent->GetObjectID(0);
            m_nLastSpellId = pEvent->GetInteger(0);
            m_bLastSpellHarmful = pEvent->GetInteger(1);
            if (m_bLastSpellHarmful != 0) {
                SetLastHostileActor(m_oidLastSpellCaster, 0);
                CSWSCreature* pCaster =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_oidLastSpellCaster);
                if (pCaster != 0) {
                    pCaster->BroadcastCombatStateToParty(OBJECT_INVALID);
                    pCaster->SetExcitedState(2);
                    pCaster->RemoveCombatInvisibilityEffects();
                }
            }
            g_pVirtualMachine->RunScript(&m_scriptSlots[8], m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_DAMAGED:
            m_lastDamagerId = nCallerId;
            SetLastHostileActor(nCallerId, 0);
            g_pVirtualMachine->RunScript(&m_scriptSlots[2], m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_DIALOGUE: {
            m_oidLastSpeaker = nCallerId;
            m_nListenPatternNumber = pEvent->GetInteger(1);
            m_sLastConversation = pEvent->GetString(0);
            g_pAppManager->m_pServerExoApp->SetLastConversation(pEvent->GetString(0));
            m_bPrivateConversation = pEvent->GetInteger(2);
            g_pAppManager->m_pServerExoApp->SetActionConversation(1);
            g_pAppManager->m_pServerExoApp->SetIgnoreConversationStartRange(pEvent->GetInteger(3));
            ClearMatchedExpressionStrings();
            for (int i = 1; i < pEvent->GetInteger(0); i++) {
                AddMatchedExpressionString(pEvent->GetString(i));
            }
            if (m_scriptSlots[13] == "default") {
                m_scriptSlots[13] = "k_hen_dialogue01";
            }
            g_pVirtualMachine->RunScript(&m_scriptSlots[13], m_objectId, 1);
            break;
        }
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_DEATH:
            m_killerId = nCallerId;
            g_pVirtualMachine->RunScript(&m_scriptSlots[3], m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_USER_DEFINED_EVENT:
            m_nUserDefinedEventNumber = pEvent->GetInteger(0);
            g_pVirtualMachine->RunScript(&m_scriptSlots[11], m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_DISARM:
            if (m_bTrapFlag != 0) {
                m_bTrapFlag = 0;
                m_oidLastDisarmedBy = nCallerId;
                g_pVirtualMachine->RunScript(&m_scriptSlots[4], m_objectId, 1);
            }
            if (m_objectsAtDoor.m_count > 0) {
                m_objectsAtDoor.Clear();
            }
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_MINE_TRIGGERED: {  // the open-lock / trap-trigger attempt
#if KOTOR1  // K1: no open / lock-pick arms, only the trap trigger
            int nUnused = pEvent->GetInteger(0);
            {
#else
            int nUnused = pEvent->GetInteger(0);
            int nInt1 = pEvent->GetInteger(1);
            int nInt2 = pEvent->GetInteger(2);
            if (pEvent->GetInteger(4) != 0) {
                // The delayed "the lock finally gave" follow-up this arm schedules on itself.
                m_bLocked = 0;
                OpenDoor(m_objectId);
            } else if (nInt1 == 0 && nInt2 != 0) {
                // Security-skill lock pick.
                if (m_bTrapFlag == 0) break;
                CSWSCreature* pPicker =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nCallerId);
                int nCheck = pPicker->m_pStats->GetSkillRank(SKILL_DEMOLITIONS, 0, 0) / 2 +
                             pEvent->GetInteger(3) + 20;
                int bOpened = 0;
                if (nCheck >= static_cast<int>(static_cast<unsigned char>(GetOpenLockDC()))) {
                    CScriptEvent* pOpenEvent = new CScriptEvent();
                    pOpenEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_MINE_TRIGGERED;
                    pOpenEvent->SetInteger(4, 1);
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                        0, 1000, nCallerId, m_objectId, nEventType, pOpenEvent);
                    CScriptEvent* pUnlockEvent = new CScriptEvent();
                    pUnlockEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_UNLOCKED;
                    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                        0, 0, pPicker->m_objectId, m_objectId, nEventType, pUnlockEvent);
                    bOpened = 1;
                }
                CExoString sResult;
                if (bOpened) {
                    sResult = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SUCCEEDED);
                } else {
                    sResult = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_FAILED);
                }
                g_pTlkTable->SetCustomToken(1, sResult);
                g_pTlkTable->SetCustomToken(2, CExoString(nCheck));
                g_pTlkTable->SetCustomToken(3, CExoString("20"));
                g_pTlkTable->SetCustomToken(4, CExoString(pPicker->m_pStats->GetSkillRank(SKILL_DEMOLITIONS, 0, 0)));
                g_pTlkTable->SetCustomToken(5, CExoString(pEvent->GetInteger(3)));
                g_pTlkTable->SetCustomToken(6, CExoString(static_cast<int>(static_cast<unsigned char>(GetOpenLockDC()))));
                CExoString sFeedback = g_pSWTlkTable->Fetch(STRREF_BLAST_LOCK_ATTEMPT, 0, 0, 0);
                g_pAppManager->m_pClientExoApp->GetInGameGui()->AppendToMsgBuffer(sFeedback, kMessageTypeFeedback, 0);
                m_oidEnteringObject = nCallerId;
                m_bTrapFlag = 0;
                if (GetArea() != 0) {
                    if (static_cast<CSWSArea*>(GetArea())->m_mineObjects.Contains(m_objectId)) {
                        static_cast<CSWSArea*>(GetArea())->m_mineObjects.Remove(m_objectId);
                    }
                }
                if (m_objectsAtDoor.m_count > 0) {
                    m_objectsAtDoor.Clear();
                }
                // Cached in a local so all three calls read the same trigger.
#if !KOTOR1
                CSWSObject* pOwnedTrigger = m_pMineTrigger;
                if (pOwnedTrigger != 0) {
                    pOwnedTrigger->SetFadeOutTime(1);
                    static_cast<CSWSTrigger*>(pOwnedTrigger)->RemoveFromArea();
                    delete pOwnedTrigger;
                }
#endif
            } else {
#endif
                CSWSCreature* pTripper =
                    g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nCallerId);
                if (pTripper != 0) {
                    if (pTripper->m_pStats->GetEffectImmunity(
                            static_cast<unsigned char>(CSWSEffectListHandler::IMMUNITY_TYPE_MINE), 0) != 0) {
                        break;
                    }
                }
                if (nUnused == 0) {
                    // pTripper is dereferenced below without the null check just above.
                    if (GetReputation(nCallerId, 0) > CFactionManager::REPUTATION_FRIENDLY_ABOVE) break;
#ifdef K2_FIX_ENGINE_BUGS
                    if (pTripper != 0 &&
                        m_nFaction == static_cast<unsigned int>(pTripper->m_pStats->m_factionId)) break;
#else
                    if (m_nFaction == static_cast<unsigned int>(pTripper->m_pStats->m_factionId)) break;
#endif
                }
                if (m_bTrapFlag == 0) break;
                if (pTripper != 0) {
                    pTripper->SendFeedbackMessage(FEEDBACK_TRIGGERED_MINE, 0);
                }
                m_oidEnteringObject = nCallerId;
                g_pVirtualMachine->RunScript(&m_scriptSlots[9], m_objectId, 1);  // OnTrapTriggered
                if (m_bTrapOneShot != 0) {
                    m_bTrapFlag = 0;
#if KOTOR1  // K1: one more (discarded) GetArea() and a `== 1` count test
                    if (GetArea() != 0) {
                        GetArea();
                        if (static_cast<CSWSArea*>(GetArea())->m_mineObjects.Contains(m_objectId) == 1) {
                            static_cast<CSWSArea*>(GetArea())->m_mineObjects.Remove(m_objectId);
                        }
                    }
#else
                    if (GetArea() != 0) {
                        if (static_cast<CSWSArea*>(GetArea())->m_mineObjects.Contains(m_objectId)) {
                            static_cast<CSWSArea*>(GetArea())->m_mineObjects.Remove(m_objectId);
                        }
                    }
#endif
                }
                if (m_objectsAtDoor.m_count > 0) {
                    m_objectsAtDoor.Clear();
                }
                // Cached in a local so all three calls read the same trigger.
#if !KOTOR1
                CSWSObject* pOwnedTrigger = m_pMineTrigger;
                if (pOwnedTrigger != 0) {
                    pOwnedTrigger->SetFadeOutTime(1);
                    static_cast<CSWSTrigger*>(pOwnedTrigger)->RemoveFromArea();
                    delete pOwnedTrigger;
                }
#endif
            }
            break;
        }
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_LOCKED:
            m_oidLastLockedBy = nCallerId;
            g_pVirtualMachine->RunScript(&m_scriptSlots[6], m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_UNLOCKED:
            m_oidLastUnlockedBy = nCallerId;
            g_pVirtualMachine->RunScript(&m_scriptSlots[10], m_objectId, 1);
            break;
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_CLICKED: {  // Also the area/module transition trigger
            m_oidEnteringObject = pEvent->GetObjectID(0);
            CSWSCreature* pClicker = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                pEvent->GetObjectID(0));
            if (GetIsLinked()) {
                CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
                if (pClicker != 0 &&
                    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState == 0) {
                    if (pArea == 0) break;
#if KOTOR1  // K1: the out-of-line GetLength
                    if (m_linkedToModule.GetLength() < 1) break;
#else
                    if (m_linkedToModule.m_sString == 0) break;
                    if (static_cast<int>(strlen(m_linkedToModule.m_sString)) <= 0) break;
#endif
                    if (pArea->m_bTransitionPending != 0) {
                        // A transition is already armed: only the event carrying its own id wins.
                        if (pEvent->GetInteger(0) != pArea->m_activeTransitionID) break;
#if KOTOR1  // K1: the dying test is GetDeadTemp()
                        if (pClicker->GetDead() == 0 && pClicker->GetDeadTemp() == 0) {
#else
                        if (pClicker->GetDead() == 0 &&
                            (pClicker->m_bInParty == 0 ||
                             pClicker->GetCurrentHitPoints(0) > 0)) {
#endif
                            CServerExoApp* pServerExoApp = g_pAppManager->m_pServerExoApp;
                            pServerExoApp->SetMoveToModulePending(1);
                            pServerExoApp->SetMoveToModuleString(m_linkedToModule);
                            pServerExoApp->SetMoveToModuleStartWaypoint(m_linkedTo);
                        } else {
                            g_pAppManager->m_pClientExoApp->GetInGameGui()->StopGlobalFade();
                        }
                    } else {
                        if (pClicker->m_bPlayerCharacter == 0) {
                            if (g_pAppManager->m_pServerExoApp->GetPartyTable()->GetIsLeader(
                                    pClicker->m_objectId) == 0) {
                                break;
                            }
                        }
#if KOTOR1  // K1: the party must be together, not healthy
                        if (g_pAppManager->m_pClientExoApp->GetSWParty()->IsPartyTogether() == 0) {
#else
                        if (g_pAppManager->m_pClientExoApp->GetSWParty()->IsPartyHealthy() == 0) {
#endif
                            // The party is in no state to travel, so run the fail script instead.
                            CExoString sFailScript("k_trg_transfail");
                            g_pVirtualMachine->RunScript(&sFailScript, pClicker->m_objectId, 1);
                            break;
                        }
                        CScriptEvent* pTransition = new CScriptEvent();
                        unsigned char nPendingID = pArea->GetNextTransitionPendingID();
                        pTransition->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_CLICKED;
                        pTransition->SetObjectID(0, pEvent->GetObjectID(0));
                        pTransition->SetInteger(0, nPendingID);
                        g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                            0, 500, m_objectId, m_objectId, nEventType, pTransition);
                        Vector vFade;  // zeroed by Vector's default ctor
                        g_pAppManager->m_pClientExoApp->GetInGameGui()->StartGlobalFade(
                            0, 0.0f, 0.5f, vFade);
                        pArea->SetTransitionPending(1, nPendingID);
                        g_pAppManager->m_pServerExoApp->SetPauseState(2, 0);
                        break;
                    }
                }
                if (pArea != 0) {
                    pArea->SetTransitionPending(0, 0);
                }
            } else {
#if KOTOR1  // K1: empty OR "default"
                if ((m_scriptSlots[12].IsEmpty() || m_scriptSlots[12] == "default") && GetIsLinked()) {
#else
                if (m_scriptSlots[12].m_sString != 0 && m_scriptSlots[12].m_sString[0] != 0 &&
                    m_scriptSlots[12] == "default" && GetIsLinked()) {
#endif
                    m_scriptSlots[12] = "NW_G0_Transition";
                }
                CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(
                    pEvent->GetObjectID(0));
                if (pPlayer != 0 && m_loadScreenId != 0) {
                    pPlayer->SetAreaTransitionBMP(m_loadScreenId, CExoString(""));
                }
                g_pVirtualMachine->RunScript(&m_scriptSlots[12], m_objectId, 1);
            }
            break;
        }
        case CSWSSCRIPTEVENT_EVENTTYPE_ON_FAIL_TO_OPEN:
            m_oidEnteringObject = nCallerId;
            g_pVirtualMachine->RunScript(&m_scriptSlots[14], m_objectId, 1);
            if (m_bLocked != 0) {
                // Suppress the "it's locked" nag when an open action for this door is already queued.
                CExoLinkedListNode* pNode = m_actionList.m_pInternal->m_pHead;
                while (pNode != 0) {
                    CSWSObjectActionNode* pAction =
                        static_cast<CSWSObjectActionNode*>(m_actionList.GetAtPos(pNode));
                    if (pAction != 0 && pAction->m_nActionId == AIACTION_OPEN_DOOR &&
                        pAction->m_args.m_attackTargetId == m_objectId) {
                        goto signal_done;
                    }
                    m_actionList.GetNext(pNode);
                }
#if KOTOR1  // K1: no event-integer gate on the feedback
                {
#else
                if (pEvent->GetInteger(0) == 0) {
#endif
                    CSWSCreature* pCreature =
                        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nCallerId);
                    if (pCreature != 0) {
                        // The feedback message is never freed.
                        CSWCCMessageData* pMsg = new CSWCCMessageData();
                        pMsg->SetObjectID(0, m_objectId);
                        pCreature->SendFeedbackMessage(FEEDBACK_OBJECT_LOCKED, pMsg);
                    }
                }
            }
            break;
        default:
            break;
        }
    signal_done:
        delete pEvent;
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_TIMED_EVENT)) {
        if (pData != 0) {
            g_pVirtualMachine->RunScriptSituation(pData, m_objectId, 1);
        }
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_OPEN_OBJECT)) {
        OpenDoor(nCallerId);
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_CLOSE_OBJECT)) {
        m_oidLastClosedBy = nCallerId;
        SetOpenState(CSWCDoor::DOOR_STATE_CLOSED, 1);
        CSWSObject* pLinked = GetLinkedObject();
        if (pLinked != 0 && pLinked->AsSWSDoor() != 0) {
            pLinked->AsSWSDoor()->SetOpenState(CSWCDoor::DOOR_STATE_CLOSED, 1);
        }
        g_pVirtualMachine->RunScript(&m_scriptSlots[1], m_objectId, 1);  // OnClosed
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_APPLY_EFFECT)) {
        if (pData != 0) {
            ApplyEffect(static_cast<CGameEffect*>(pData), 0, 0);
        }
    } else if (nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_DESTROY_OBJECT)) {
        if (m_bDestroyable != 0) {
            if (GetIsLinked()) {
                // A linked door never actually leaves: it goes to the "gone" mesh state and drops its
                // static collision volume.
                if (GetArea() != 0) {
                    GetArea()->RemoveStaticBoundingBox(m_objectId);
                }
                SetOpenState(CSWCDoor::DOOR_STATE_DESTROYED, 1);
            } else {
                RemoveFromArea();
                delete this;
            }
        }
    } else if (pData != 0 && nEventType == static_cast<unsigned int>(CServerAIMaster::EVENT_PLAY_ANIMATION)) {
        if (GetDead() == 0) {
            SetAnimation(static_cast<int>(reinterpret_cast<uintptr_t>(pData)));
        }
    }
}

void CSWSDoor::DoDamage(int amount) {
    if (m_bPlot) return;
    CSWSObject::DoDamage(amount);
}

int CSWSDoor::GetIsLinked() {
    return static_cast<unsigned>(m_linkedToFlags - 1) < 2;
}

int CSWSDoor::GetDead() {
    if (m_bPlot) return 0;
    return CSWSObject::GetDead();
}

void* CSWSDoor::GetClientDoor() {
    if (GetClientObject() == 0) return 0;
    CGameObject* pClientObj = GetClientObject();
    return pClientObj->AsSWCDoor();
}

#if !KOTOR1  // K2-only (no K1 counterpart)
unsigned char CSWSDoor::GetDisarmDC() {
    unsigned char base = m_baseDisarmDC;
    unsigned char diff = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_difficultyLevel;
    int adj = (diff == 0) ? -5 : ((diff == 2) ? 5 : 0);
    unsigned char dc = static_cast<unsigned char>(adj + base);
    return dc ? dc : 1;
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
unsigned char CSWSDoor::GetDetectDC() {
    unsigned char base = m_baseDetectDC;
    unsigned char diff = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_difficultyLevel;
    int adj = (diff == 0) ? -5 : ((diff == 2) ? 5 : 0);
    int dc = adj + base;
    return static_cast<unsigned char>(dc > 1 ? dc : 1);
}
#endif // !KOTOR1

Vector CSWSDoor::GetActionPoint(int state, unsigned char pointIndex) {
    int meshIdx;
    if (state == SERVER_ANIMATION_DOOR_OPEN2) {
        meshIdx = CSWDoorSurfaceMesh::DOOR_STATE_OPEN2;
    } else if (state == SERVER_ANIMATION_DOOR_OPEN1) {
        meshIdx = CSWDoorSurfaceMesh::DOOR_STATE_OPEN1;
    } else if (state == SERVER_ANIMATION_CLOSED) {
        meshIdx = CSWDoorSurfaceMesh::DOOR_STATE_CLOSED;
    } else {
        return Vector{-9999999.0f, -9999999.0f, -9999999.0f};
    }
    CSWCollisionMesh* pMesh = m_meshVariants[meshIdx];
    Vector* pPoint = &pMesh->m_headerVec[pointIndex];
    return pMesh->LocalToWorld(*pPoint);
}

#if !KOTOR1  // K1 has no difficulty-scaled GetOpenLockDC
unsigned int CSWSDoor::GetOpenLockDC() {
    unsigned char state2 = m_openLockDiff;
    unsigned int result;
    if (state2 == 4) {
        unsigned char diff = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_difficultyLevel;
        // Written positively, so difficulty 3 and up keep the plain value.
        int adj = 0;
        if (diff < 3) {
            adj = diff * 5 + -5;
        }
        int val = adj + m_openLockDC;
        result = val & ~(val >> 31);
    } else {
        unsigned char diff = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_difficultyLevel;
        int adj = 21;
        if (diff < 3) {
            adj = diff + diff * 4 + 16;
        }
        int extra = m_nPCLevelAtSpawn + 7;
        if (state2 != 3) {
            extra = 0;
        }
        // Read unsigned even though the declared field is a signed char.
        result = static_cast<unsigned char>(m_openLockDiffMod) + adj + extra;
    }
    return result;
}
#endif

void CSWSDoor::SetOpenState(unsigned char newState, int immediate) {
    unsigned char state = (m_bVisibleModel != 0) ? newState : CSWCDoor::DOOR_STATE_DESTROYED;
    m_pendingOpenState = state;
    if (immediate == 0) {
        m_curOpenState = state;
    }
    switch (state) {
    case CSWCDoor::DOOR_STATE_CLOSED:    return SetAnimation(SERVER_ANIMATION_CLOSED);
    case CSWCDoor::DOOR_STATE_OPEN1:     return SetAnimation(SERVER_ANIMATION_DOOR_OPEN1);
    case CSWCDoor::DOOR_STATE_OPEN2:     return SetAnimation(SERVER_ANIMATION_DOOR_OPEN2);
    case CSWCDoor::DOOR_STATE_DESTROYED: return SetAnimation(SERVER_ANIMATION_DESTROYED);
    default: return;
    }
}

// Swings the door open away from whoever asked and drags its linked twin along. The side comes from
// the sign of the dot product of "opener minus door" with the door's own facing: standing in front of
// the facing direction gives open state 1, behind it or with no opener at all gives 2. The twin is
// reached through AsSWSDoor, and the script fires from the OnOpen slot.
void CSWSDoor::OpenDoor(unsigned long long objectId) {
    unsigned char nOpenState = CSWCDoor::DOOR_STATE_OPEN2;
    CGameObject* pOpener = g_pAppManager->m_pServerExoApp->GetGameObject(objectId);
    if (pOpener) {
        // CSWSDoor's own m_orientation shadows the base's facing vector name here.
        Vector vToOpener = pOpener->AsSWSObject()->m_position - m_position;
        if (vToOpener.x * CSWSObject::m_orientation.x +
                vToOpener.y * CSWSObject::m_orientation.y +
                vToOpener.z * CSWSObject::m_orientation.z >
            0.0f) {
            nOpenState = CSWCDoor::DOOR_STATE_OPEN1;
        }
    }
    m_oidLastOpenedBy = objectId;
    SetOpenState(nOpenState, 1);

    CSWSObject* pLinked = GetLinkedObject();
    if (pLinked && pLinked->AsSWSDoor()) {
        pLinked->AsSWSDoor()->SetOpenState(nOpenState, 1);
    }
    g_pVirtualMachine->RunScript(&m_scriptSlots[0], m_objectId, 1);
}

// Fixes the door up once after load. It turns the GFF "Hardness" into a permanent damage-resistance
// effect, forces a broken or unopenable door shut and plot-protected, and gives a static door its
// genericdoors.2da idle animation.
//
// m_bApplyingResistanceEffect is raised across the ApplyEffect call, which is what makes the effect
// stamp m_bSkipOnLoad so it is re-derived on every load instead of being saved. The "StaticAnim"
// cell is read into a default-constructed string that survives the tail, and a failed lookup assigns
// "" to the destination rather than leaving it untouched. CompareNoCase returns non-zero for a
// mismatch, so the animation fires for every StaticAnim value except "busted".
void CSWSDoor::PostProcess() {
    if (m_hardness) {
        m_bApplyingResistanceEffect = 1;
        CGameEffect* pEffect = new CGameEffect(1);
        pEffect->m_subType = static_cast<unsigned short>(CSWSEffectListHandler::EFFECT_DAMAGE_RESISTANCE);
        pEffect->m_durationTypeBits =
            CGameEffect::DURATION_TYPE_PERMANENT | (pEffect->m_durationTypeBits & CGameEffect::DURATION_TYPE_CLEAR_MASK);
        pEffect->m_bIsExposed = 0;
        pEffect->SetInteger(0, CSWSEffectListHandler::DAMAGE_FLAG_ALL);
        pEffect->SetInteger(1, m_hardness);
        ApplyEffect(pEffect, 0, 0);
        m_bApplyingResistanceEffect = 0;
    }

    if (m_bVisibleModel == 0) {
        SetOpenState(CSWCDoor::DOOR_STATE_DESTROYED, 1);
        m_bPlot = 1;
    }

    if (m_bStatic) {
        CExoString sStaticAnim;
        if (g_pRules->m_p2DArrays->m_pGenericDoors->GetCExoStringEntry(
                m_genericType, CExoString("StaticAnim"), &sStaticAnim) == 0) {
            sStaticAnim = "";
        }
        if (sStaticAnim.CompareNoCase("busted")) {
            SetAnimation(SERVER_ANIMATION_DOOR_BUSTED);
        }
    }
}

void CSWSDoor::MoveToNextOpenState() {
    unsigned char cur = m_curOpenState;
    unsigned char next = m_pendingOpenState;
    m_curOpenState = next;
    if (next != cur && next != CSWCDoor::DOOR_STATE_DESTROYED) {
        CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
        if (pArea != 0) {
            int isClosing = (m_curOpenState == CSWCDoor::DOOR_STATE_CLOSED);
            pArea->BudgeCreatures(m_position, m_budgeVecA, m_budgeVecB, m_objectId, isClosing);
        }
    }
}

CSWSObject* CSWSDoor::GetLinkedObject() {
    if (static_cast<unsigned>(m_linkedToFlags - 1) >= 2) {
        return 0;
    }
    CSWSObject* pDoor = static_cast<CSWSObject*>(g_pAppManager->m_pServerExoApp->GetDoorByGameObjectID(m_cachedLinkedObjectId));
    if (pDoor != 0) {
        return pDoor;
    }
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    unsigned long long objectId = pModule->FindObjectByTagOrdinal(m_linkedTo, 0);
    m_cachedLinkedObjectId = objectId;
    CGameObject* pObject = static_cast<CGameObject*>(g_pAppManager->m_pServerExoApp->GetGameObject(objectId));
    if (pObject == 0) {
        return 0;
    }
    return pObject->AsSWSObject();
}

// One Vector serves as both the raw facing and the result.
void CSWSDoor::SetOrientation(Quaternion q) {
    m_orientation.w = q.w;
    m_orientation.x = q.x;
    m_orientation.y = q.y;
    m_orientation.z = q.z;
    // The rotated Y axis, the door's facing direction, is column 1 of the rotation matrix. Only the Y
    // and X components are needed.
    float ax = 2.0f * (q.x * q.y - q.w * q.z);
    float ay = 1.0f - 2.0f * (q.x * q.x + q.z * q.z);
    Vector axis;
    axis.x = ax;
    axis.y = ay;
    axis.z = 0.0f;
    if (axis.x * axis.x + axis.y * axis.y + axis.z * axis.z > 0.0001f) {
        axis = normalize(axis);
    } else {
        axis.x = 0.0f;
        axis.y = 1.0f;
        axis.z = 0.0f;
    }
    CSWSObject::SetOrientation(axis);
}

void CSWSDoor::GetMeshOutline(Vector p1, Vector p2, Vector& out1, Vector& out2,
                               Vector& out3, Vector& out4) {
    K2_UNUSED(p2);
    m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_CLOSED]->m_translation = p1;
    m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_CLOSED]->m_rotation = m_orientation;
    m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_CLOSED]->GetMeshOutline(out1, out2, out3, out4);
}

int CSWSDoor::NoNonWalkPolysInDoor(const Vector& a, const Vector& b, const float& c,
                                    const float& d, const float& e, CWalkHitInfo* pInfo) {
    if (m_meshVariants[m_curOpenState] == 0) return 0;
    m_meshVariants[m_curOpenState]->m_translation = m_position;
    m_meshVariants[m_curOpenState]->m_rotation = m_orientation;
    return m_meshVariants[m_curOpenState]->NoNonWalkPolys(a, b, c, d, e, pInfo);
}

// Accumulates a running min/max box over the mesh variants: the first iteration copies the local
// min/max directly and later ones compare and replace per axis. Each switch arm ends in its own
// RenderMesh, since a single one after the switch would let the arms merge.
unsigned int CSWSDoor::GetMeshBoundingBox(Vector p1, Vector p2, Vector& outMin, Vector& outMax) {
    K2_UNUSED(p2);
    unsigned int result = 1;
    Vector localMin = {0.0f, 0.0f, 0.0f};
    Vector localMax = {0.0f, 0.0f, 0.0f};
    for (int state = CSWDoorSurfaceMesh::DOOR_STATE_CLOSED; state <= CSWDoorSurfaceMesh::DOOR_STATE_OPEN2; state++) {
        m_meshVariants[state]->m_translation = p1;
        m_meshVariants[state]->m_rotation = m_orientation;
        result &= m_meshVariants[state]->GetMeshBoundingBox(localMin, localMax);
        if (nRenderDoorBBox != 0) {
            switch (state) {
            case 0:
                m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_CLOSED]->LocalToWorld(
                    m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_CLOSED]->m_headerVec[0]);
                m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_CLOSED]->LocalToWorld(
                    m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_CLOSED]->m_headerVec[1]);
                nRenderDoorBBox = 2;
                m_meshVariants[state]->RenderMesh(0, DEBUG_COLOR_RED, DEBUG_COLOR_GREEN);
                break;
            case 1:
                m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_OPEN1]->LocalToWorld(
                    m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_OPEN1]->m_headerVec[0]);
                m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_OPEN1]->LocalToWorld(
                    m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_OPEN1]->m_headerVec[1]);
                nRenderDoorBBox = 3;
                m_meshVariants[state]->RenderMesh(0, DEBUG_COLOR_RED, DEBUG_COLOR_GREEN);
                break;
            case 2:
                m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_OPEN2]->LocalToWorld(
                    m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_OPEN2]->m_headerVec[0]);
                m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_OPEN2]->LocalToWorld(
                    m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_OPEN2]->m_headerVec[1]);
                nRenderDoorBBox = 4;
                m_meshVariants[state]->RenderMesh(0, DEBUG_COLOR_RED, DEBUG_COLOR_GREEN);
                break;
            }
        }
        if (state == CSWDoorSurfaceMesh::DOOR_STATE_CLOSED) {
            outMin = localMin;
            outMax = localMax;
        } else {
            if (localMin.x < outMin.x) outMin.x = localMin.x;
            if (localMin.y < outMin.y) outMin.y = localMin.y;
            if (localMin.z < outMin.z) outMin.z = localMin.z;
            if (localMax.x > outMax.x) outMax.x = localMax.x;
            if (localMax.y > outMax.y) outMax.y = localMax.y;
            if (localMax.z > outMax.z) outMax.z = localMax.z;
        }
    }
    return result;
}

int CSWSDoor::LoadFromTemplate(const CResRef& resRef) {
    CResStruct topStruct;
    CResGFF* pGff = new CResGFF(RESTYPE_UTD, const_cast<char*>("UTD "), resRef);
    int result;
    if (pGff->m_nValid) {
        pGff->GetTopLevelStruct(&topStruct);
        LoadDoor(pGff, &topStruct, 1);
        delete pGff;
        result = 1;
    } else {
        delete pGff;
        result = 0;
    }
    return result;
}

Vector CSWSDoor::GetNearestActionPoint(const Vector& pos, int stateOverride) {
    int curState = m_nAnimation;
    CSWSArea* pArea = static_cast<CSWSArea*>(GetArea());
    int state = (stateOverride == 0) ? curState : SERVER_ANIMATION_CLOSED;
    int meshIdxClosed = CSWDoorSurfaceMesh::DOOR_STATE_CLOSED;

    int meshIdx = meshIdxClosed;
    int bHaveMesh = 1;
    if (state == SERVER_ANIMATION_DOOR_OPEN2) {
        meshIdx = CSWDoorSurfaceMesh::DOOR_STATE_OPEN2;
    } else if (state == SERVER_ANIMATION_DOOR_OPEN1) {
        meshIdx = CSWDoorSurfaceMesh::DOOR_STATE_OPEN1;
    } else if (state == SERVER_ANIMATION_CLOSED) {
        meshIdx = meshIdxClosed;
    } else {
        bHaveMesh = 0;
    }

    Vector candA, candB;
    if (bHaveMesh) {
        CSWCollisionMesh* pMesh = m_meshVariants[meshIdx];
        candA = pMesh->LocalToWorld(pMesh->m_headerVec[0]);
        pMesh = m_meshVariants[meshIdx];
        candB = pMesh->LocalToWorld(pMesh->m_headerVec[1]);
    } else {
        candA.x = candA.y = candA.z = -9999999.0f;
        candB.x = candB.y = candB.z = -9999999.0f;
    }

    float distSqB = (pos.x - candB.x) * (pos.x - candB.x)
                   + (pos.y - candB.y) * (pos.y - candB.y)
                   + (pos.z - candB.z) * (pos.z - candB.z);

    Vector chosen;
    float chosenDist;
    float distSqA = (pos.x - candA.x) * (pos.x - candA.x)
                   + (pos.y - candA.y) * (pos.y - candA.y)
                   + (pos.z - candA.z) * (pos.z - candA.z);
    if (pArea != 0 && distSqA < distSqB && pArea->PositionWalkable(candA)) {
        chosen = candA;
        chosenDist = distSqA;
    } else if (pArea != 0) {
        if (pArea->PositionWalkable(candB)) {
            chosen = candB;
            chosenDist = distSqB;
        } else {
            chosen.x = chosen.y = chosen.z = 0.0f;
            chosenDist = 1000.0f;
        }
    } else {
        chosen.x = chosen.y = chosen.z = 0.0f;
        chosenDist = 1000.0f;
    }

    Vector result;
    if (state == SERVER_ANIMATION_CLOSED) {
        result = chosen;
    } else {
        CSWCollisionMesh* pClosedMesh = m_meshVariants[meshIdxClosed];
        Vector candC = pClosedMesh->LocalToWorld(pClosedMesh->m_headerVec[0]);
        pClosedMesh = m_meshVariants[meshIdxClosed];
        Vector candD = pClosedMesh->LocalToWorld(pClosedMesh->m_headerVec[1]);

        float distSqC = (pos.x - candC.x) * (pos.x - candC.x)
                       + (pos.y - candC.y) * (pos.y - candC.y)
                       + (pos.z - candC.z) * (pos.z - candC.z);
        float distSqD = (pos.x - candD.x) * (pos.x - candD.x)
                       + (pos.y - candD.y) * (pos.y - candD.y)
                       + (pos.z - candD.z) * (pos.z - candD.z);
        if (pArea != 0 && distSqC < chosenDist && pArea->PositionWalkable(candC)) {
            chosen = candC;
        } else if (pArea != 0 && distSqD < chosenDist && pArea->PositionWalkable(candD)) {
            chosen = candD;
        }
        result = chosen;
    }
    return result;
}

int CSWSDoor::LoadDoorExternal(CResGFF* pGFF, CResStruct* pStruct, int external) {
    int success;
    if (external == 0) {
        LoadDoor(pGFF, pStruct, external);
    } else {
        CResRef emptyResRef("");
        CResRef templateResRef = pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("TemplateResRef"), success, emptyResRef);
        if (success == 0 || LoadFromTemplate(templateResRef) == 0) {
            return 0;
        }
        {
            CExoLocString result = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("TransitionDestination"), success, m_transitionDestination);
            m_transitionDestination = result;
        }
        {
            CExoString linkedToDefault("");
            CExoString result = pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("LinkedTo"), success, linkedToDefault);
            m_linkedTo = result;
        }
        m_linkedToFlags = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("LinkedToFlags"), success, 0);
        {
            CExoString str;
            {
                CResRef emptyResRef2("");
                CResRef linkedToModuleResRef = pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("LinkedToModule"), success, emptyResRef2);
                linkedToModuleResRef.CopyToString(str);
            }
            m_linkedToModule = str;
#if !KOTOR1  // K1: no tweak colour and no "Tag" override
            m_bUseTweakColor = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("UseTweakColor"), success, 0);
            m_tweakColor = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("TweakColor"), success, 0);
            {
                CExoString tagDefault("");
                CExoString result = pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("Tag"), success, tagDefault);
                str = result;
            }
            if (success) {
                m_sTag = str.LowerCase();
            }
#endif
        }
    }
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    pModule->AddObjectToLookupTable(m_sTag, m_objectId);
    return 1;
}

// Reads the door's whole persistent state out of a UTD GFF struct, the exact mirror of SaveDoor below
// in the same field order. Almost entirely a flat run of ReadField* calls whose default argument is
// the field's own current value, so a missing GFF field leaves the constructor's default in place.
// Three places carry real logic:
//
// * CurrentHP is clamped to HP only when bClampCurrentHP is set: a fresh blueprint spawn asks for the
//   clamp, a savegame restore does not, where current > max is a legitimate buffed state
// * "Invulnerable" is the legacy spelling of "Plot": the new label is tried first and "Plot" only
//   when it was absent. A static door then forces the plot flag
// * the OnTrapTriggered slot falls back to traps.2da's "MineScript" column whenever the GFF left it
//   empty or at the constructor's default sentinel
//
// The trailing model block is what makes the appearance and the generic type mutually exclusive. A
// non-zero appearance is a doortypes.2da row, a zero one falls back to the generic door table. The
// two arms write different fields and each zeroes the other's, which is why a generic door always
// reports a visible model.
int CSWSDoor::LoadDoor(CResGFF* pGFF, CResStruct* pStruct, int bClampCurrentHP)
{
    int bSuccess;
    m_appearance = static_cast<unsigned char>(pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("Appearance"), bSuccess, m_appearance));
    m_genericType = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("GenericType"), bSuccess, m_genericType);
    SetOpenState(pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("OpenState"), bSuccess, m_curOpenState), 0);
    m_bAutoRemoveKey = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("AutoRemoveKey"), bSuccess, static_cast<unsigned char>(m_bAutoRemoveKey));
    m_bearing = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("Bearing"), bSuccess, m_bearing);
    m_nFaction = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("Faction"), bSuccess, m_nFaction);
    m_fortSave = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Fort"), bSuccess, m_fortSave);
    m_willSave = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Will"), bSuccess, m_willSave);
    m_refSave = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Ref"), bSuccess, m_refSave);
    m_hitPoints = pGFF->ReadFieldSHORT(pStruct, const_cast<char*>("HP"), bSuccess, static_cast<short>(m_hitPoints));

    short nCurrentHP = pGFF->ReadFieldSHORT(pStruct, const_cast<char*>("CurrentHP"), bSuccess,
                                             GetCurrentHitPoints(0));
    // Two calls rather than one on a clamped local, because the argument is selected by branch.
    if (bClampCurrentHP && nCurrentHP > static_cast<short>(m_hitPoints))
        nCurrentHP = static_cast<short>(m_hitPoints);
    SetCurrentHitPoints(nCurrentHP);

    unsigned char bPlot = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Invulnerable"), bSuccess, static_cast<unsigned char>(m_bPlot));
    if (bSuccess == 0)
        bPlot = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Plot"), bSuccess, static_cast<unsigned char>(m_bPlot));
    unsigned char bStatic = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Static"), bSuccess, 0);
    m_bStatic = bStatic;
    m_bPlot = bStatic ? 1 : bPlot;

#if !KOTOR1  // K1: no NotBlastable
    m_bNotBlastable = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("NotBlastable"), bSuccess, static_cast<unsigned char>(m_bNotBlastable));
#endif
    m_bMin1HP = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Min1HP"), bSuccess, static_cast<unsigned char>(m_bMin1HP));
    m_keyName = pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("KeyName"), bSuccess, m_keyName);
    m_bKeyRequired = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("KeyRequired"), bSuccess, static_cast<unsigned char>(m_bKeyRequired));
#if KOTOR1  // K1: the default is the current value
    m_openLockDC = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("OpenLockDC"), bSuccess, m_openLockDC);
#else
    m_openLockDC = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("OpenLockDC"), bSuccess, 0);
#endif
#if !KOTOR1
    m_openLockDiff = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("OpenLockDiff"), bSuccess, m_openLockDiff);
    m_openLockDiffMod = pGFF->ReadFieldCHAR(pStruct, const_cast<char*>("OpenLockDiffMod"), bSuccess,
                                             m_openLockDiffMod);
#endif
    m_closeLockDC = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("CloseLockDC"), bSuccess, m_closeLockDC);
    m_secretDoorDC = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("SecretDoorDC"), bSuccess, m_secretDoorDC);
    m_sTag = pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("Tag"), bSuccess, m_sTag).LowerCase();
    m_dialogResref = pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("Conversation"), bSuccess, CResRef(""));

    unsigned short nPortraitId = pGFF->ReadFieldWORD(pStruct, const_cast<char*>("PortraitId"), bSuccess, PORTRAIT_ID_NONE);
    if (nPortraitId >= PORTRAIT_ID_CUSTOM) {
        CResRef portrait;
        portrait = pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("Portrait"), bSuccess, CResRef(""));
        if (*portrait.GetResRef() != 0)
            SetPortrait(portrait);
        else
            SetPortraitId(PORTRAIT_ID_DOOR_DEFAULT);
    } else {
    // Two calls, not a ternary argument, because the outer else's own SetPortraitId call is the
    // third arm.
        SetPortraitId(nPortraitId == 0 ? PORTRAIT_ID_DOOR_DEFAULT : nPortraitId);
    }
    m_hardness = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Hardness"), bSuccess, 0);
    m_firstName = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("LocName"), bSuccess, CExoLocString());
    m_description = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("Description"), bSuccess,
                                                  CExoLocString());

    m_scriptSlots[1] = CExoString(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnClosed"), bSuccess,
                                                          CResRef(m_scriptSlots[1])).GetResRefStr());
    m_scriptSlots[2] = CExoString(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnDamaged"), bSuccess,
                                                          CResRef(m_scriptSlots[1])).GetResRefStr());
    m_scriptSlots[3] = CExoString(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnDeath"), bSuccess,
                                                          CResRef(m_scriptSlots[3])).GetResRefStr());
    m_scriptSlots[4] = CExoString(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnDisarm"), bSuccess,
                                                          CResRef(m_scriptSlots[4])).GetResRefStr());
    m_scriptSlots[5] = CExoString(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnHeartbeat"), bSuccess,
                                                          CResRef(m_scriptSlots[5])).GetResRefStr());
    m_scriptSlots[6] = CExoString(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnLock"), bSuccess,
                                                          CResRef(m_scriptSlots[6])).GetResRefStr());
    m_scriptSlots[7] = CExoString(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnMeleeAttacked"), bSuccess,
                                                          CResRef(m_scriptSlots[7])).GetResRefStr());
    m_scriptSlots[0] = CExoString(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnOpen"), bSuccess,
                                                          CResRef(m_scriptSlots[0])).GetResRefStr());
    m_scriptSlots[8] = CExoString(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnSpellCastAt"), bSuccess,
                                                          CResRef(m_scriptSlots[8])).GetResRefStr());
    m_scriptSlots[9] = CExoString(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnTrapTriggered"), bSuccess,
                                                          CResRef(m_scriptSlots[9])).GetResRefStr());
    m_scriptSlots[10] = CExoString(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnUnlock"), bSuccess,
                                                           CResRef(m_scriptSlots[10])).GetResRefStr());
    m_scriptSlots[11] = CExoString(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnUserDefined"), bSuccess,
                                                           CResRef(m_scriptSlots[11])).GetResRefStr());
    m_scriptSlots[12] = CExoString(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnClick"), bSuccess,
                                                           CResRef(m_scriptSlots[12])).GetResRefStr());
    m_scriptSlots[14] = CExoString(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnFailToOpen"), bSuccess,
                                                           CResRef(m_scriptSlots[14])).GetResRefStr());
    m_scriptSlots[13] = CExoString(pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("OnDialog"), bSuccess,
                                                           CResRef(m_scriptSlots[13])).GetResRefStr());

    m_trapType = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("TrapType"), bSuccess, m_trapType);
    if (m_scriptSlots[9].IsEmpty() || m_scriptSlots[9] == "default") {
        CExoString sMineScript;
        g_pRules->m_p2DArrays->m_pTraps->GetCExoStringEntry(m_trapType, CExoString("MineScript"),
                                                            &sMineScript);
        m_scriptSlots[9] = sMineScript;
    }

    m_bTrapDisarmable = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("TrapDisarmable"), bSuccess,
                                             static_cast<unsigned char>(m_bTrapDisarmable));
    m_bTrapDetectable = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("TrapDetectable"), bSuccess,
                                             static_cast<unsigned char>(m_bTrapDetectable));
    m_baseDisarmDC = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("DisarmDC"), bSuccess, GetDisarmDC());
    m_baseDetectDC = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("TrapDetectDC"), bSuccess, GetDetectDC());
#if !KOTOR1
    m_nOwnerDemolitionsSkill = pGFF->ReadFieldINT(pStruct, const_cast<char*>("OwnerDemolitionsSkill"),
                                                   bSuccess, 0);
#endif
    m_bTrapFlag = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("TrapFlag"), bSuccess, static_cast<unsigned char>(m_bTrapFlag));
    m_bTrapOneShot = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("TrapOneShot"), bSuccess, static_cast<unsigned char>(m_bTrapOneShot));
    m_bLocked = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Locked"), bSuccess, static_cast<unsigned char>(m_bLocked));
    m_bLockable = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("Lockable"), bSuccess, static_cast<unsigned char>(m_bLockable));
    m_linkedToFlags = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("LinkedToFlags"), bSuccess, 0);
    m_linkedTo = pGFF->ReadFieldCExoString(pStruct, const_cast<char*>("LinkedTo"), bSuccess, CExoString(""));
#if !KOTOR1  // K1: no tweak colour, no PCLevelAtSpawn
    m_bUseTweakColor = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("UseTweakColor"), bSuccess, 0);
    m_tweakColor = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("TweakColor"), bSuccess, 0);
    m_nPCLevelAtSpawn = pGFF->ReadFieldBYTE(pStruct, const_cast<char*>("PCLevelAtSpawn"), bSuccess, 0);
#endif
    m_transitionDestination = pGFF->ReadFieldCExoLocString(pStruct, const_cast<char*>("TransitionDestination"),
                                                            bSuccess, m_transitionDestination);
    CResRef linkedToModule;
    linkedToModule = pGFF->ReadFieldCResRef(pStruct, const_cast<char*>("LinkedToModule"), bSuccess, CResRef(""));
    if (bSuccess == 1)
        m_linkedToModule = linkedToModule.GetResRefStr();
    m_loadScreenId = pGFF->ReadFieldWORD(pStruct, const_cast<char*>("LoadScreenID"), bSuccess, 0);

    CExoString sModelName;
    if (m_appearance != 0) {
        g_pRules->m_p2DArrays->m_pDoorTypes->GetCExoStringEntry(m_appearance, CExoString("Model"),
                                                                 &sModelName);
        int bVisibleModel = 1;
        g_pRules->m_p2DArrays->m_pDoorTypes->GetINTEntry(m_appearance, CExoString("VisibleModel"),
                                                          &bVisibleModel);
        m_bVisibleModel = bVisibleModel;
        m_bPreciseUse = 0;
    } else {
        g_pRules->m_p2DArrays->m_pGenericDoors->GetCExoStringEntry(m_genericType,
                                                                    CExoString("ModelName"),
                                                                    &sModelName);
        int bPreciseUse = 0;
        g_pRules->m_p2DArrays->m_pGenericDoors->GetINTEntry(m_genericType,
                                                             CExoString("PreciseUse"),
                                                             &bPreciseUse);
        m_bPreciseUse = (bPreciseUse != 0);
    }

    for (int i = CSWDoorSurfaceMesh::DOOR_STATE_CLOSED;
         i <= CSWDoorSurfaceMesh::DOOR_STATE_OPEN2; i++) {
        m_meshVariants[i] = new CSWDoorSurfaceMesh();
        m_meshVariants[i]->SetDoorType(i);
        m_meshVariants[i]->m_resRef = CResRef(sModelName);
        m_meshVariants[i]->LoadMesh(0);
        m_meshVariants[i]->ComputeMeshNormals();
    }
    // Qualified on purpose: the virtual dispatch is not wanted here.
    CSWSDoor::PostProcess();
    return 1;
}

// Writes the door's whole persistent state back out as GFF fields, the paired writer for LoadDoor
// above. Almost entirely a flat run of WriteField* calls in source order; the only real logic is the
// two trap-DC fields, which bake the current difficulty setting into the saved value.
int CSWSDoor::SaveDoor(CResGFF* pGFF, CResStruct* pStruct)
{
    pGFF->WriteFieldDWORD(pStruct, m_appearance, const_cast<char*>("Appearance"));
    pGFF->WriteFieldBYTE(pStruct, m_genericType, const_cast<char*>("GenericType"));
    pGFF->WriteFieldBYTE(pStruct, m_curOpenState, const_cast<char*>("OpenState"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bAutoRemoveKey), const_cast<char*>("AutoRemoveKey"));
    pGFF->WriteFieldFLOAT(pStruct, m_bearing, const_cast<char*>("Bearing"));
    pGFF->WriteFieldFLOAT(pStruct, m_position.x, const_cast<char*>("X"));
    pGFF->WriteFieldFLOAT(pStruct, m_position.y, const_cast<char*>("Y"));
    pGFF->WriteFieldFLOAT(pStruct, m_position.z, const_cast<char*>("Z"));
    pGFF->WriteFieldDWORD(pStruct, m_nFaction, const_cast<char*>("Faction"));
    pGFF->WriteFieldBYTE(pStruct, m_fortSave, const_cast<char*>("Fort"));
    pGFF->WriteFieldBYTE(pStruct, m_willSave, const_cast<char*>("Will"));
    pGFF->WriteFieldBYTE(pStruct, m_refSave, const_cast<char*>("Ref"));
    pGFF->WriteFieldSHORT(pStruct, static_cast<short>(m_hitPoints), const_cast<char*>("HP"));
    pGFF->WriteFieldSHORT(pStruct, GetCurrentHitPoints(0), const_cast<char*>("CurrentHP"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bPlot), const_cast<char*>("Plot"));
#if !KOTOR1
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bNotBlastable), const_cast<char*>("NotBlastable"));
#endif
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bMin1HP), const_cast<char*>("Min1HP"));
    pGFF->WriteFieldCExoString(pStruct, m_keyName, const_cast<char*>("KeyName"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bKeyRequired), const_cast<char*>("KeyRequired"));
    pGFF->WriteFieldBYTE(pStruct, m_openLockDC, const_cast<char*>("OpenLockDC"));
#if !KOTOR1
    pGFF->WriteFieldBYTE(pStruct, m_openLockDiff, const_cast<char*>("OpenLockDiff"));
    pGFF->WriteFieldCHAR(pStruct, m_openLockDiffMod, const_cast<char*>("OpenLockDiffMod"));
#endif
    pGFF->WriteFieldBYTE(pStruct, m_closeLockDC, const_cast<char*>("CloseLockDC"));
    pGFF->WriteFieldBYTE(pStruct, m_secretDoorDC, const_cast<char*>("SecretDoorDC"));
    pGFF->WriteFieldCExoString(pStruct, m_sTag, const_cast<char*>("Tag"));
    pGFF->WriteFieldCResRef(pStruct, GetDialogResref(), const_cast<char*>("Conversation"));

    GetPortraitId();
    pGFF->WriteFieldWORD(pStruct, GetPortraitId(), const_cast<char*>("PortraitId"));
    pGFF->WriteFieldBYTE(pStruct, m_hardness, const_cast<char*>("Hardness"));

    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[1]), const_cast<char*>("OnClosed"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[2]), const_cast<char*>("OnDamaged"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[3]), const_cast<char*>("OnDeath"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[4]), const_cast<char*>("OnDisarm"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[5]), const_cast<char*>("OnHeartbeat"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[6]), const_cast<char*>("OnLock"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[7]), const_cast<char*>("OnMeleeAttacked"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[0]), const_cast<char*>("OnOpen"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[8]), const_cast<char*>("OnSpellCastAt"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[9]), const_cast<char*>("OnTrapTriggered"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[10]), const_cast<char*>("OnUnlock"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[11]), const_cast<char*>("OnUserDefined"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[12]), const_cast<char*>("OnClick"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[14]), const_cast<char*>("OnFailToOpen"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(m_scriptSlots[13]), const_cast<char*>("OnDialog"));

    pGFF->WriteFieldBYTE(pStruct, m_trapType, const_cast<char*>("TrapType"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bTrapDisarmable), const_cast<char*>("TrapDisarmable"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bTrapDetectable), const_cast<char*>("TrapDetectable"));

    // The two trap DCs are saved difficulty-adjusted, easy -5 and difficult +5, each clamped to a
    // minimum of 1, the same adjustment the getters apply when reading.
#if KOTOR1  // K1: the raw base DCs
    pGFF->WriteFieldBYTE(pStruct, m_baseDisarmDC, const_cast<char*>("DisarmDC"));
    pGFF->WriteFieldBYTE(pStruct, m_baseDetectDC, const_cast<char*>("TrapDetectDC"));
#else
    unsigned char baseDisarm = m_baseDisarmDC;
    unsigned char diffDisarm = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_difficultyLevel;
    int adjDisarm = (diffDisarm == 0) ? -5 : ((diffDisarm == 2) ? 5 : 0);
    unsigned char disarmDC = static_cast<unsigned char>(adjDisarm + baseDisarm);
    if (disarmDC == 0) disarmDC = 1;
    pGFF->WriteFieldBYTE(pStruct, disarmDC, const_cast<char*>("DisarmDC"));

    unsigned char baseDetect = m_baseDetectDC;
    unsigned char diffDetect = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_difficultyLevel;
    int adjDetect = (diffDetect == 0) ? -5 : ((diffDetect == 2) ? 5 : 0);
    int detectDC = adjDetect + baseDetect;
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(detectDC > 1 ? detectDC : 1), const_cast<char*>("TrapDetectDC"));

    pGFF->WriteFieldINT(pStruct, m_nOwnerDemolitionsSkill, const_cast<char*>("OwnerDemolitionsSkill"));
#endif
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bTrapFlag), const_cast<char*>("TrapFlag"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bTrapOneShot), const_cast<char*>("TrapOneShot"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bLocked), const_cast<char*>("Locked"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bLockable), const_cast<char*>("Lockable"));
    pGFF->WriteFieldBYTE(pStruct, m_linkedToFlags, const_cast<char*>("LinkedToFlags"));
    pGFF->WriteFieldCExoString(pStruct, m_linkedTo, const_cast<char*>("LinkedTo"));
    pGFF->WriteFieldCResRef(pStruct, CResRef(static_cast<char*>(m_linkedToModule.CStr())), const_cast<char*>("LinkedToModule"));
    pGFF->WriteFieldWORD(pStruct, m_loadScreenId, const_cast<char*>("LoadScreenID"));
#if !KOTOR1
    pGFF->WriteFieldBYTE(pStruct, m_bUseTweakColor, const_cast<char*>("UseTweakColor"));
    pGFF->WriteFieldDWORD(pStruct, m_tweakColor, const_cast<char*>("TweakColor"));
    pGFF->WriteFieldBYTE(pStruct, m_nPCLevelAtSpawn, const_cast<char*>("PCLevelAtSpawn"));
#endif
    pGFF->WriteFieldCExoLocString(pStruct, GetFirstName(), const_cast<char*>("LocName"));
    pGFF->WriteFieldCExoLocString(pStruct, m_description, const_cast<char*>("Description"));
    pGFF->WriteFieldBYTE(pStruct, static_cast<unsigned char>(m_bStatic), const_cast<char*>("Static"));
    pGFF->WriteFieldCExoLocString(pStruct, m_transitionDestination, const_cast<char*>("TransitionDestination"));
    return 1;
}

// The six object-id slots are three two-id records, written in a non-ascending order that lets each
// adjacent pair merge into one 16-byte store.
CSWSDoor::CSWSDoor(unsigned long long objectId)
    : CSWSObject(CGameObject::OBJECT_TYPE_DOOR, objectId, 0),
      m_budgeVecA{}, m_budgeVecB{}, m_meshOutline(),
      m_orientation{1.0f, 0.0f, 0.0f, 0.0f} {
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddObject(this, 0);
    m_nAnimation = SERVER_ANIMATION_CLOSED;
    // Not ascending, and not arbitrary: see above.
    m_oidLastOpenedBy = OBJECT_INVALID;
    m_oidLastClosedBy = OBJECT_INVALID;
    m_oidLastLockedBy = OBJECT_INVALID;
    m_oidLastUnlockedBy = OBJECT_INVALID;
    m_oidEnteringObject = OBJECT_INVALID;
    m_oidLastDisarmedBy = OBJECT_INVALID;
    for (int i = 0; i < 15; i++)
        m_scriptSlots[i] = "default";
    m_appearance = 0;
    m_genericType = 0;
    m_bAutoRemoveKey = 0;
    m_baseDisarmDC = 0;
    m_baseDetectDC = 0;
    m_bTrapFlag = 0;
    m_bMineFlagged = 0;
    m_trapType = TRAP_TYPE_NONE;
    m_bTrapDisarmable = 1;
    m_bTrapDetectable = 1;
    m_bTrapOneShot = 1;
    m_bearing = 0.0f;
    m_nFaction = 0;
    m_fortSave = 0;
    m_refSave = 0;
    m_willSave = 0;
    m_keyName = "";
    m_bKeyRequired = 0;
    m_bLocked = 0;
    m_bLockable = 0;
    m_curOpenState = CSWCDoor::DOOR_STATE_CLOSED;
    m_pendingOpenState = CSWCDoor::DOOR_STATE_CLOSED;
    m_openLockDC = 0;
    m_closeLockDC = 0;
    m_secretDoorDC = 0;
    m_hardness = 0;
    m_bVisibleModel = 1;
#if !KOTOR1
    m_openLockDiff = 1;
    m_openLockDiffMod = 0;
    m_nOwnerDemolitionsSkill = 0;
    m_bUseTweakColor = 0;
    m_tweakColor = 0;
#endif
    m_lastUpdateDay = 0;
    m_lastUpdateTod = 0;
    m_linkedToFlags = 0;
    m_cachedLinkedObjectId = OBJECT_INVALID;
    m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_CLOSED] = 0;
    m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_OPEN1] = 0;
    m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_OPEN2] = 0;
    m_nMeshOutlinePoints = 0;
    m_loadScreenId = 0;
    m_bStatic = 0;
    m_bPreciseUse = 0;
#if !KOTOR1
    m_pMineTrigger = 0;
#endif
}

CSWSDoor::~CSWSDoor() {
    if (GetArea() != 0) RemoveFromArea();
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->RemoveObject(this);
    CSWSModule* pModule = g_pAppManager->m_pServerExoApp->GetModule();
    if (pModule != 0) pModule->RemoveObjectFromLookupTable(m_sTag, m_objectId);
    for (int i = CSWDoorSurfaceMesh::DOOR_STATE_CLOSED;
         i <= CSWDoorSurfaceMesh::DOOR_STATE_OPEN2; i++) {
        if (m_meshVariants[i] != 0) {
            delete m_meshVariants[i];
            m_meshVariants[i] = 0;
        }
    }
    m_nMeshOutlinePoints = 0;
}

void CSWSDoor::RemoveFromArea() {
    if (GetArea() != 0) {
        static_cast<CSWSArea*>(GetArea())->RemoveObjectFromArea(m_objectId);
        if (static_cast<CSWSArea*>(GetArea())->m_mineObjects.Contains(m_objectId) == 1)
            static_cast<CSWSArea*>(GetArea())->m_mineObjects.Remove(m_objectId);
        GetArea()->RemoveStaticBoundingBox(m_objectId);
    }
    SetArea(0);
    Vector zero = {};
    SetPosition(zero, 1, 1, 0);
}

void CSWSDoor::AddToArea(CSWSArea* pArea, float x, float y, float z, int n) {
    RemoveFromArea();
    if (pArea == 0) return;
    SetArea(pArea);
    Vector vPos;
    vPos.x = x;
    vPos.y = y;
    vPos.z = z;
    SetPosition(vPos, 0, 1, 0);
    pArea->AddObjectToArea(m_objectId, n);
    int count = pArea->m_objects.m_count;
    unsigned long long objId = m_objectId;
    if (count == pArea->m_objects.m_capacity) {
        int newCap = (count == 0) ? 16 : count * 2;
        pArea->m_objects.m_capacity = newCap;
        unsigned long long* pOld = pArea->m_objects.m_pArray;
        unsigned long long* pNew = new unsigned long long[newCap];
        K2_ASSUME(count < newCap);
        pArea->m_objects.m_pArray = pNew;
        for (int64_t i = 0; i < count; i++)
            pNew[i] = pOld[i];
        delete[] pOld;
        count = pArea->m_objects.m_count;
    }
    pArea->m_objects.m_count = count + 1;
    pArea->m_objects.m_pArray[count] = objId;
    if (GetMeshBoundingBox(m_position, m_position, m_budgeVecA, m_budgeVecB) != 0) {
        // CSWArea, not CSWSArea, because both bounding-box members live on the shared area base.
        static_cast<CSWArea*>(pArea)->AddStaticBoundingBox(m_objectId, m_budgeVecA, m_budgeVecB);
        m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_CLOSED]->m_translation = m_position;
        m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_CLOSED]->m_rotation = m_orientation;
        m_meshVariants[CSWDoorSurfaceMesh::DOOR_STATE_CLOSED]->GetMeshOutline(
            m_meshOutline[0], m_meshOutline[1], m_meshOutline[2], m_meshOutline[3]);
        m_nMeshOutlinePoints = 4;
    }
}

// Tests a point against the door's footprint outline with a crossing-number ray-cast in +y, the
// same algorithm CSWSEncounter::GetInActivateArea runs over its activation polygon, with the same
// edge walk and on-the-ray vertex special case. Returns the parity of the crossing count.
int CSWSDoor::InDoor(Vector point) {
    int64_t n = m_nMeshOutlinePoints;
    int count = 0;
    if (n >= 1) {
        float px = point.x;
        float py = point.y;
        unsigned last = m_nMeshOutlinePoints - 1;
        int64_t i = 0;
        do {
            int64_t j = (i == last) ? 0 : i + 1;
            float nextX = m_meshOutline[j].x;
            float curX = m_meshOutline[i].x;
            if (nextX != curX) {
                float curY = m_meshOutline[i].y;
                float nextY = m_meshOutline[j].y;
                float dx = nextX - curX;
                float curMinusPx = curX - px;
                float dy = nextY - curY;
                float slope = dy / dx;
                float yInt = curY - slope * curMinusPx;
                if (yInt > py) {
                    bool inside;
                    if (__builtin_fabsf(dy) > __builtin_fabsf(dx)) {
                        if (nextY > curY) inside = (yInt >= curY) && (yInt < nextY);
                        else inside = (curY > nextY) && (yInt > nextY) && (yInt <= curY);
                    } else {
                        if (nextX > curX) inside = (px >= curX) && (px < nextX);
                        else inside = (curX > nextX) && (px > nextX) && (px <= curX);
                    }
                    count += inside;
                }
            }
            if (nextX == px) {
                if (m_meshOutline[j].y > py) count++;
                else if (m_meshOutline[i].y >= py) count++;
            }
            i++;
        } while (i < n);
    }
    return (count % 2) == 1;
}
