#include "k2stdint.h"
#include "serveraimaster.h"
#include "nwscreature.h"
#include "nwsobject.h"
#include "nwsarea.h"
#include "nwsmodule.h"
#include "CExoArrayListImpl.h"
#include "nwsaiaction.h"

int g_bLogEventAdd;

extern CAppManager* g_pAppManager;
CVirtualMachine* g_pVirtualMachine;

#if KOTOR1
const long long CServerAIMaster::AI_UPDATE_TIME_HI_RES = 3000;
#else
const long long CServerAIMaster::AI_UPDATE_TIME_HI_RES = 10000;
#endif
const int CServerAIMaster::EVENT_TIMED_EVENT = 1;
const int CServerAIMaster::EVENT_ENTERED_TRIGGER = 2;
const int CServerAIMaster::EVENT_LEFT_TRIGGER = 3;
const int CServerAIMaster::EVENT_REMOVE_FROM_AREA = 4;
const int CServerAIMaster::EVENT_APPLY_EFFECT = 5;
const int CServerAIMaster::EVENT_CLOSE_OBJECT = 6;
const int CServerAIMaster::EVENT_OPEN_OBJECT = 7;
const int CServerAIMaster::EVENT_SPELL_IMPACT = 8;
const int CServerAIMaster::EVENT_PLAY_ANIMATION = 9;
const int CServerAIMaster::EVENT_SIGNAL_EVENT = 10;
const int CServerAIMaster::EVENT_DESTROY_OBJECT = 11;
const int CServerAIMaster::EVENT_UNLOCK_OBJECT = 12;
const int CServerAIMaster::EVENT_LOCK_OBJECT = 13;
const int CServerAIMaster::EVENT_REMOVE_EFFECT = 14;
const int CServerAIMaster::EVENT_ON_MELEE_ATTACKED = 15;
const int CServerAIMaster::EVENT_DECREMENT_STACKSIZE = 16;
const int CServerAIMaster::EVENT_SPAWN_BODY_BAG = 17;
const int CServerAIMaster::EVENT_FORCED_ACTION = 18;
const int CServerAIMaster::EVENT_ITEM_ON_HIT_SPELL_IMPACT = 19;
const int CServerAIMaster::EVENT_BROADCAST_AOO = 20;
const int CServerAIMaster::EVENT_BROADCAST_SAFE_PROJECTILE = 21;
const int CServerAIMaster::EVENT_FEEDBACK_MESSAGE = 22;
const int CServerAIMaster::EVENT_ABILITY_EFFECT_APPLIED = 23;
const int CServerAIMaster::EVENT_SUMMON_CREATURE = 24;
const int CServerAIMaster::EVENT_ACQUIRE_ITEM = 25;
const int CServerAIMaster::EVENT_AREA_TRANSITION = 26;
const int CServerAIMaster::EVENT_CONTROLLER_RUMBLE = 27;

// The GFF struct ids SaveEventQueue tags each queued event node, and each kind of event payload,
// with.
enum {
    GFF_STRUCT_ID_EVENT_NODE            = 0xabcd,
    GFF_STRUCT_ID_EVENT_DATA_EFFECT     = 0x1111,
    GFF_STRUCT_ID_EVENT_DATA_ATTACK     = 0x2222,
    GFF_STRUCT_ID_EVENT_DATA_VALUE      = 0x3333,
    GFF_STRUCT_ID_EVENT_DATA_SCRIPT     = 0x4444,
    GFF_STRUCT_ID_EVENT_DATA_BODY_BAG   = 0x5555,
    GFF_STRUCT_ID_EVENT_DATA_SPELL      = 0x6666,
    GFF_STRUCT_ID_EVENT_DATA_SITUATION  = 0x7777,
    GFF_STRUCT_ID_EVENT_DATA_FORCED     = 0x8888,
    GFF_STRUCT_ID_EVENT_DATA_AREA_FLAG  = 0x9999,
    GFF_STRUCT_ID_EVENT_DATA_FEEDBACK   = 0xcccc,
    GFF_STRUCT_ID_EVENT_DATA_SUMMON     = 0xdddd,
    GFF_STRUCT_ID_EVENT_DATA_TRANSITION = 0xeeee,
    GFF_STRUCT_ID_EVENT_DATA_RUMBLE     = 0xffff,
};

// A log-only helper that fills the id and type-name out-parameters for an object.
static void LogEventAddGetObject(unsigned long long id, CExoString& sId, CExoString& sType);

CServerAIList::CServerAIList()
{
    m_nIterIndex = 0;
}

CServerAIList::~CServerAIList()
{
}

// Adds an object id to the list and tells the caller whether it was already there.
int CServerAIList::AddObject(unsigned long long id)
{
    if (m_aObjectIds.Contains(id)) {
        return 1;
    }
    m_aObjectIds.Add(id);
    return 1;
}

CGameObject* CServerAIList::GetObjectAtPosition(int index)
{
    if (m_aObjectIds.m_count == 0) {
        return 0;
    }
    unsigned long long id = m_aObjectIds[index];
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(id);
    if (pObject == 0) {
        return 0;
    }
    return (pObject->m_objectType > CGameObject::OBJECT_TYPE_AREA) ? pObject : 0;
}

// Removes an object id from the list; returns 0 when it was not there.
int CServerAIList::RemoveObject(unsigned long long id)
{
    if (m_aObjectIds.Contains(id)) {
        int nFound = m_aObjectIds.IndexOf(id);
        if (nFound != -1) {
            m_aObjectIds.DelIndex(nFound);
        }
    }
    return 1;
}

CGameObject* CServerAIList::GetNextObject()
{
    if (m_aObjectIds.m_count == 0) {
        return 0;
    }
    int index = (m_nIterIndex + 1 < m_aObjectIds.m_count) ? (m_nIterIndex + 1) : 0;
    m_nIterIndex = index;
    unsigned long long id = m_aObjectIds[index];
    CGameObject* pObject = g_pAppManager->m_pServerExoApp->GetGameObject(id);
    if (pObject == 0) {
        return 0;
    }
    return (pObject->m_objectType > CGameObject::OBJECT_TYPE_AREA) ? pObject : 0;
}

// The lists, the event queue and m_sScriptsRun construct implicitly; the body only zeroes the rest.
// m_bRanInterAreaPath is deliberately not zeroed.
CServerAIMaster::CServerAIMaster()
{
    CSWSEffectListHandler* pEffectListHandler = new CSWSEffectListHandler;
    m_pEffectListHandler = pEffectListHandler;
    pEffectListHandler->InitializeEffects();

    m_pItemPropertyHandler = new CSWSItemPropertyHandler;
    m_pItemPropertyHandler->InitializeItemProperties();

    // Ascending order, so the four stores merge into one vector store.
    m_pRepAdjustAttack = 0;
    m_pRepAdjustTheft = 0;
    m_pRepAdjustKill = 0;
    m_pRepAdjustHelp = 0;
}

// Everything after the four boxed slots destroys implicitly, in reverse declaration order. The two
// handler teardowns are `delete p`, not an explicit destructor call.
CServerAIMaster::~CServerAIMaster()
{
    CExoLinkedListInternal* pQueue = m_eventQueue.m_pInternal;
    CExoLinkedListNode* pNode = pQueue->m_pHead;
    while (pNode != 0) {
        CServerAIEventNode* pEvt = static_cast<CServerAIEventNode*>(pQueue->RemoveHead());
        DeleteEventData(pEvt->m_nType, pEvt->m_pData);
        operator delete(pEvt);
        pQueue = m_eventQueue.m_pInternal;
        pNode = pQueue->m_pHead;
    }

    if (m_pEffectListHandler != 0) {
        delete m_pEffectListHandler;
        m_pEffectListHandler = 0;
    }
    if (m_pItemPropertyHandler != 0) {
        delete m_pItemPropertyHandler;
        m_pItemPropertyHandler = 0;
    }

    CExoArrayList<int>* pRepAdjustAttack = m_pRepAdjustAttack;
    if (pRepAdjustAttack != 0) {
        if (pRepAdjustAttack->m_pArray != 0) {
            operator delete[](pRepAdjustAttack->m_pArray);
        }
        operator delete(pRepAdjustAttack);
        m_pRepAdjustAttack = 0;
    }
    CExoArrayList<int>* pRepAdjustTheft = m_pRepAdjustTheft;
    if (pRepAdjustTheft != 0) {
        if (pRepAdjustTheft->m_pArray != 0) {
            operator delete[](pRepAdjustTheft->m_pArray);
        }
        operator delete(pRepAdjustTheft);
        m_pRepAdjustTheft = 0;
    }
    CExoArrayList<int>* pRepAdjustKill = m_pRepAdjustKill;
    if (pRepAdjustKill != 0) {
        if (pRepAdjustKill->m_pArray != 0) {
            operator delete[](pRepAdjustKill->m_pArray);
        }
        operator delete(pRepAdjustKill);
        m_pRepAdjustKill = 0;
    }
    CExoArrayList<int>* pRepAdjustHelp = m_pRepAdjustHelp;
    if (pRepAdjustHelp != 0) {
        if (pRepAdjustHelp->m_pArray != 0) {
            operator delete[](pRepAdjustHelp->m_pArray);
        }
        operator delete(pRepAdjustHelp);
        m_pRepAdjustHelp = 0;
    }
    // The remaining members destruct implicitly from here.
}

void CServerAIMaster::SetExoAppInternal(CServerExoAppInternal* pApp)
{
    m_pExoAppInternal = pApp;
}

// The per-frame AI tick. Walks the five per-AI-level rosters from 4 down to 0, and for each one
// runs a time-sliced loop:
//
// * scan every object by position and hand each live non-area creature a quick-walk location update
// * drain the due head of the event queue, dispatching each record to its target's event handler
// * with budget left, advance the list's round-robin cursor and run one object's AI update, with the
//   scripts-run counters reset around it and a long-running watchdog reported through
//   ReportOverflow
//
// The per-list slice is 60% of the remaining budget for lists 4..1, and the whole remaining budget
// for list 0. The walkable action set is the basic walk, follow-leader and follow-owner move, plus
// the three forced-move walk states.
//
// K1: the walk set has no follow-owner move and there is no watchdog.
void CServerAIMaster::UpdateState()
{
    unsigned int nDay;
    unsigned int nTimeOfDay;
    m_pExoAppInternal->m_pWorldTimer->GetWorldTime(&nDay, &nTimeOfDay);

    unsigned long long nRemaining = AI_UPDATE_TIME_HI_RES;
    for (int i = 4; i >= 0; i--) {
        unsigned long long nSlice = nRemaining;
        if (i > 0) {
            nSlice = nSlice * 60 / 100;
        }
        CServerAIList* pList = &m_lists[i];
        uint64_ll_t nBegin = g_pExoBase->m_pTimers->GetHighResolutionTimer();
        uint64_ll_t nNow = nBegin;
        int bFirstPass = 1;
        int bExhausted = 0;
        CSWSObject* pLastObject = 0;
        do {
            for (int j = 0; j < pList->m_aObjectIds.m_count; j++) {
                CSWSObject* pObject = static_cast<CSWSObject*>(pList->GetObjectAtPosition(j));
                if (pObject == 0) {
                    continue;
                }
                if (pObject->m_bVisibleToPlayer == 0) {
                    continue;
                }
                if (pObject->AsSWSCreature() == 0) {
                    continue;
                }
                CSWSCreature* pCreature = pObject->AsSWSCreature();
                CSWSObjectActionNode* pAction = 0;
                if (pCreature->m_actionList.m_pInternal->m_pHead != 0 &&
                    pCreature->m_actionList.m_pInternal->m_pHead->m_pData != 0) {
                    pAction = static_cast<CSWSObjectActionNode*>(pCreature->m_actionList.m_pInternal->m_pHead->m_pData);
                }
#if KOTOR1  // K1: no follow-owner move action
                if ((pAction != 0 && (pAction->m_nActionId == AIACTION_MOVE_TO_POINT || pAction->m_nActionId == AIACTION_PARTY_FOLLOW_LEADER)) ||
#else
                if ((pAction != 0 && (pAction->m_nActionId == AIACTION_MOVE_TO_POINT || pAction->m_nActionId == AIACTION_PARTY_FOLLOW_LEADER ||
                                      pAction->m_nActionId == AIACTION_FOLLOW_OWNER)) ||
#endif
                    pObject->AsSWSCreature()->m_nMoveState == 4 ||
                    pObject->AsSWSCreature()->m_nMoveState == 5 ||
                    pObject->AsSWSCreature()->m_nMoveState == 6) {
                    if (pObject->AsSWSCreature()->WalkUpdateLocation_QuickWalk(0) == CSWSObject::ACTION_COMPLETE) {
                        pObject->AsSWSCreature()->m_nMoveState = 1;
                    }
                }
            }
            while (EventPending(nDay, nTimeOfDay) == 1) {
                unsigned int nEventDay;
                unsigned int nEventTimeOfDay;
                unsigned long long oidCaller;
                unsigned long long oidObject;
                unsigned int nEventType;
                void* pEventData;
                if (GetPendingEvent(&nEventDay, &nEventTimeOfDay, &oidCaller, &oidObject,
                                    &nEventType, &pEventData) == 1) {
                    CSWSObject* pEventObject = 0;
                    if (m_pExoAppInternal->m_pObjectArray->GetGameObject(
                            oidObject, reinterpret_cast<CGameObject**>(&pEventObject)) != CGameObjectArray::SUCCESS) {
                        DeleteEventData(nEventType, pEventData);
                    } else if (pEventObject != 0 &&
                               pEventObject->m_objectType > CGameObject::OBJECT_TYPE_AREA) {
                        pEventObject->EventHandler(nEventType, oidCaller, pEventData, nEventDay,
                                                   nEventTimeOfDay);
#ifdef K2_FIX_ENGINE_BUGS
                    } else if (pEventObject == 0) {
                        DeleteEventData(nEventType, pEventData);
#endif
                    } else if (pEventObject->m_objectType == CGameObject::OBJECT_TYPE_AREA) {
                        pEventObject->AsSWSArea()->EventHandler(nEventType, oidCaller, pEventData,
                                                                nEventDay, nEventTimeOfDay);
                    } else if (pEventObject->m_objectType == CGameObject::OBJECT_TYPE_MODULE) {
                        pEventObject->AsSWSModule()->EventHandler(nEventType, oidCaller, pEventData,
                                                                  nEventDay, nEventTimeOfDay);
                    } else {
                        DeleteEventData(nEventType, pEventData);
                    }
                }
            }
            if (nNow - nBegin <= nSlice && bExhausted == 0) {
                CSWSObject* pObject = static_cast<CSWSObject*>(pList->GetNextObject());
                bool bFirst = (bFirstPass == 1);
                if (bFirst) {
                    pLastObject = pObject;
                }
#if !KOTOR1  // K1 has no per-object 75 ms watchdog
                nNow = g_pExoBase->m_pTimers->GetHighResolutionTimer();
#endif
                m_sScriptsRun = "";
                __builtin_memset(&m_bRanInterAreaPath, 0, 5 * sizeof(int));
                m_nLastActionId = -1;
                bExhausted = 1;
                bool bDoUpdate = (bFirst || pObject != pLastObject);
                if (pObject != 0 && bDoUpdate) {
                    pObject->AIUpdate();
                    bExhausted = 0;
                }
#if !KOTOR1
                unsigned int nElapsed =
                    static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() - nNow);
                if (pObject != 0 && nElapsed >= 75000) {
                    pObject->ReportOverflow(FEEDBACK_AI_UPDATE_TIME_OVERFLOW, static_cast<int>(nElapsed), 75000, 0);
                }
#endif
                nNow = g_pExoBase->m_pTimers->GetHighResolutionTimer();
                bFirstPass = 0;
            }
        } while (EventPending(nDay, nTimeOfDay) == 0 && bExhausted == 0 &&
                 nNow - nBegin <= nSlice);
        nRemaining -= (nNow - nBegin < nSlice) ? (nNow - nBegin) : nSlice;
    }

    // The client-sync pass walks the same five rosters again, this time pushing whichever dirty
    // flags a live object has set onto its client twin.
    for (int i = 4; i >= 0; i--) {
        CServerAIList* pList = &m_lists[i];
        for (int j = 0; j < pList->m_aObjectIds.m_count; j++) {
            CSWSObject* pObject = static_cast<CSWSObject*>(pList->GetObjectAtPosition(j));
            if (pObject == 0) {
                continue;
            }
            if (pObject->m_bVisibleToPlayer == 0) {
                continue;
            }
            CGameObject* pClientObject = pObject->GetClientObject();
            if (pClientObject == 0) {
                continue;
            }
            if (pClientObject->AsSWCObject() == 0) {
                continue;
            }
            if (pObject->IsDirtyFlagSet(0)) {
                pClientObject->AsSWCObject()->ClientServerSyncHandler_Animation(
                    pObject->m_nAnimation, pObject->m_fAnimationSpeed);
                pObject->ClearDirtyFlag(0);
            }
            if (pObject->IsDirtyFlagSet(1)) {
                pClientObject->AsSWCObject()->ClientServerSyncHandler_Position(
                    pObject->m_position, pObject->m_nAnimation, pObject->m_fLastSpeedFactor);
                pObject->ClearDirtyFlag(1);
            }
            if (pObject->IsDirtyFlagSet(2)) {
                pClientObject->AsSWCObject()->ClientServerSyncHandler_Orientation(
                    pObject->m_orientation);
                pObject->ClearDirtyFlag(2);
            }
        }
    }

    if (m_pExoAppInternal != 0 && m_pExoAppInternal->GetModule() != 0) {
        m_pExoAppInternal->GetModule()->DoUpdate();
    }
}

int CServerAIMaster::SetAILevel(CSWSObject* pObj, int level)
{
    if (pObj->m_aiLevel == level) {
        return 1;
    }
    if (static_cast<unsigned int>(level) > 4) {
        return 0;
    }
    return AddObject(pObj, level);
}

void CServerAIMaster::ClearEventQueue()
{
    while (m_eventQueue.m_pInternal->m_pHead != 0) {
        CServerAIEventNode* pEvent = m_eventQueue.RemoveHead();
        DeleteEventData(pEvent->m_nType, pEvent->m_pData);
        delete pEvent;
    }
}

int CServerAIMaster::GetPendingEvent(unsigned int* pA, unsigned int* pB, unsigned long long* pC,
                                      unsigned long long* pD, unsigned int* pType, void** ppData)
{
    if (m_eventQueue.m_pInternal->m_pHead == 0) {
        return 0;
    }
    CServerAIEventNode* pEvent = m_eventQueue.RemoveHead();
    *pA = pEvent->m_nTimeDay;
    *pB = pEvent->m_nTimeOfDay;
    *pC = pEvent->m_oidCaller;
    *pD = pEvent->m_oidObject;
    *pType = pEvent->m_nType;
    *ppData = pEvent->m_pData;
    pEvent->m_pData = 0;
    delete pEvent;
    return 1;
}

void CServerAIMaster::DeleteEventData(unsigned int type, void* pData)
{
    if (pData == 0) {
        return;
    }
    switch (type) {
    case EVENT_TIMED_EVENT:
        g_pVirtualMachine->DeleteScriptSituation(pData);
        return;
    case EVENT_SPELL_IMPACT:
    case EVENT_ITEM_ON_HIT_SPELL_IMPACT:
        static_cast<CSWSSpellScriptData*>(pData)->~CSWSSpellScriptData();
        break;
    case EVENT_SIGNAL_EVENT:
    case EVENT_SUMMON_CREATURE:
    case EVENT_AREA_TRANSITION:
        static_cast<CScriptEvent*>(pData)->~CScriptEvent();
        break;
    case EVENT_BROADCAST_SAFE_PROJECTILE:
        static_cast<CSWSCombatAttackData*>(pData)->~CSWSCombatAttackData();
        break;
    case EVENT_FEEDBACK_MESSAGE:
        static_cast<CSWCCMessageData*>(pData)->~CSWCCMessageData();
        break;
    default:
        return;
    }
    operator delete(pData);
}

int CServerAIMaster::OnEffectApplied(CSWSObject* pObj, CGameEffect* pEffect, int a)
{
    return m_pEffectListHandler->OnEffectApplied(pObj, pEffect, a);
}

int CServerAIMaster::OnEffectRemoved(CSWSObject* pObj, CGameEffect* pEffect)
{
    return m_pEffectListHandler->OnEffectRemoved(pObj, pEffect);
}

void CServerAIMaster::OnItemPropertyApplied(CSWSItem* pItem, CSWItemProperty* pProp,
                                             CSWSCreature* pCreature, unsigned int a, int b)
{
    m_pItemPropertyHandler->OnItemPropertyApplied(pItem, pProp, pCreature, a, b);
}

void CServerAIMaster::OnItemPropertyRemoved(CSWSItem* pItem, CSWItemProperty* pProp,
                                             CSWSCreature* pCreature, unsigned int a)
{
    m_pItemPropertyHandler->OnItemPropertyRemoved(pItem, pProp, pCreature, a);
}

void CServerAIMaster::AdjustTargetAndWitnessReputations(unsigned long long a, unsigned long long b, int c)
{
    K2_UNUSED(a); K2_UNUSED(b); K2_UNUSED(c);
}

// Builds the four reputation-adjustment lookup tables from the RepAdjust 2DA: one int list per row
// 0..3, each holding that row's 11 columns in column order. The row index drives the switch, so the
// four members are spelled out individually rather than through a pointer array.
void CServerAIMaster::LoadReputationAdjustments()
{
    C2DA* pTable = g_pRules->m_p2DArrays->m_pRepAdjustments;
    if (pTable == 0)
        return;

    for (int i = 0; i < 4; i++) {
        int nPersonalRep;
        int nFactionRep;
        int nWitFria;
        int nWitFrib;
        int nWitFric;
        int nWitNeua;
        int nWitNeub;
        int nWitNeuc;
        int nWitEnea;
        int nWitEneb;
        int nWitEnec;

        pTable->GetINTEntry(i, CExoString("PERSONALREP"), &nPersonalRep);
        pTable->GetINTEntry(i, CExoString("FACTIONREP"), &nFactionRep);
        pTable->GetINTEntry(i, CExoString("WITFRIA"), &nWitFria);
        pTable->GetINTEntry(i, CExoString("WITFRIB"), &nWitFrib);
        pTable->GetINTEntry(i, CExoString("WITFRIC"), &nWitFric);
        pTable->GetINTEntry(i, CExoString("WITNEUA"), &nWitNeua);
        pTable->GetINTEntry(i, CExoString("WITNEUB"), &nWitNeub);
        pTable->GetINTEntry(i, CExoString("WITNEUC"), &nWitNeuc);
        pTable->GetINTEntry(i, CExoString("WITENEA"), &nWitEnea);
        pTable->GetINTEntry(i, CExoString("WITENEB"), &nWitEneb);
        pTable->GetINTEntry(i, CExoString("WITENEC"), &nWitEnec);

        switch (i) {
        case 0:
            m_pRepAdjustAttack = new CExoArrayList<int>;
            m_pRepAdjustAttack->Add(nPersonalRep);
            m_pRepAdjustAttack->Add(nFactionRep);
            m_pRepAdjustAttack->Add(nWitFria);
            m_pRepAdjustAttack->Add(nWitFrib);
            m_pRepAdjustAttack->Add(nWitFric);
            m_pRepAdjustAttack->Add(nWitNeua);
            m_pRepAdjustAttack->Add(nWitNeub);
            m_pRepAdjustAttack->Add(nWitNeuc);
            m_pRepAdjustAttack->Add(nWitEnea);
            m_pRepAdjustAttack->Add(nWitEneb);
            m_pRepAdjustAttack->Add(nWitEnec);
            break;
        case 1:
            m_pRepAdjustTheft = new CExoArrayList<int>;
            m_pRepAdjustTheft->Add(nPersonalRep);
            m_pRepAdjustTheft->Add(nFactionRep);
            m_pRepAdjustTheft->Add(nWitFria);
            m_pRepAdjustTheft->Add(nWitFrib);
            m_pRepAdjustTheft->Add(nWitFric);
            m_pRepAdjustTheft->Add(nWitNeua);
            m_pRepAdjustTheft->Add(nWitNeub);
            m_pRepAdjustTheft->Add(nWitNeuc);
            m_pRepAdjustTheft->Add(nWitEnea);
            m_pRepAdjustTheft->Add(nWitEneb);
            m_pRepAdjustTheft->Add(nWitEnec);
            break;
        case 2:
            m_pRepAdjustKill = new CExoArrayList<int>;
            m_pRepAdjustKill->Add(nPersonalRep);
            m_pRepAdjustKill->Add(nFactionRep);
            m_pRepAdjustKill->Add(nWitFria);
            m_pRepAdjustKill->Add(nWitFrib);
            m_pRepAdjustKill->Add(nWitFric);
            m_pRepAdjustKill->Add(nWitNeua);
            m_pRepAdjustKill->Add(nWitNeub);
            m_pRepAdjustKill->Add(nWitNeuc);
            m_pRepAdjustKill->Add(nWitEnea);
            m_pRepAdjustKill->Add(nWitEneb);
            m_pRepAdjustKill->Add(nWitEnec);
            break;
        case 3:
            m_pRepAdjustHelp = new CExoArrayList<int>;
            m_pRepAdjustHelp->Add(nPersonalRep);
            m_pRepAdjustHelp->Add(nFactionRep);
            m_pRepAdjustHelp->Add(nWitFria);
            m_pRepAdjustHelp->Add(nWitFrib);
            m_pRepAdjustHelp->Add(nWitFric);
            m_pRepAdjustHelp->Add(nWitNeua);
            m_pRepAdjustHelp->Add(nWitNeub);
            m_pRepAdjustHelp->Add(nWitNeuc);
            m_pRepAdjustHelp->Add(nWitEnea);
            m_pRepAdjustHelp->Add(nWitEneb);
            m_pRepAdjustHelp->Add(nWitEnec);
            break;
        }
    }
}

// Moves an object to the roster for the given AI level.
int CServerAIMaster::AddObject(CSWSObject* pObj, int level)
{
    if (pObj == 0) {
        return 0;
    }
    if (static_cast<unsigned int>(level) > 4) {
        return 0;
    }
    RemoveObject(pObj);
    m_lists[level].AddObject(pObj->m_objectId);
    pObj->m_aiLevel = level;
    return 1;
}

int CServerAIMaster::RemoveObject(CSWSObject* pObj)
{
    int level = pObj->m_aiLevel;
    // A level outside 0..4 returns 0 without touching the object.
    if (static_cast<unsigned int>(level) > 4)
        return 0;
    m_lists[level].RemoveObject(pObj->m_objectId);
    pObj->m_aiLevel = -1;
    return 1;
}

// Appends one script name to the comma-separated running list reported by
// CSWSObject::ReportOverflow.
void CServerAIMaster::AppendToScriptsRun(const CExoString& script)
{
    m_sScriptsRun = m_sScriptsRun + script + ",";
}

// Reports whether the head event record is due at the given world time.
int CServerAIMaster::EventPending(unsigned int a, unsigned int b)
{
    if (m_eventQueue.m_pInternal->m_pHead == 0) {
        return 0;
    }
    CServerAIEventNode* pEvent = static_cast<CServerAIEventNode*>(m_eventQueue.m_pInternal->m_pHead->m_pData);
    return m_pExoAppInternal->m_pWorldTimer->CompareWorldTimes(pEvent->m_nTimeDay, pEvent->m_nTimeOfDay,
                                                               a, b) < 1;
}

// Queues an event the given number of world-time units from now.
int CServerAIMaster::AddEventDeltaTime(unsigned int a, unsigned int b, unsigned long long c,
                                       unsigned long long d, unsigned int type, void* pEvent)
{
    g_pAppManager->m_pServerExoApp->GetActivePauseState();
    unsigned int nDay, nTimeOfDay;
    m_pExoAppInternal->m_pWorldTimer->GetWorldTime(&nDay, &nTimeOfDay);
    unsigned int nNewDay, nNewTimeOfDay;
    if (m_pExoAppInternal->m_pWorldTimer->AddWorldTimes(nDay, nTimeOfDay, a, b,
                                                        &nNewDay, &nNewTimeOfDay) != 0) {
        if (pEvent != 0) {
            operator delete(pEvent);
        }
        return 0;
    }
    AddEventAbsoluteTime(nNewDay, nNewTimeOfDay, c, d, type, pEvent);
    return 1;
}

int CServerAIMaster::SaveEventQueue(CResGFF* pGFF, CResStruct* pStruct)
{
    CResList list;
    CResStruct nodeStruct;
    pGFF->AddList(&list, pStruct, const_cast<char*>("EventQueue"));
    if (m_eventQueue.m_pInternal->m_nCount != 0) {
        CExoLinkedListNode* pNode = m_eventQueue.m_pInternal->m_pHead;
        while (pNode != 0) {
            CServerAIEventNode* pEvent =
                m_eventQueue.GetAtPos(pNode);
            pGFF->AddListElement(&nodeStruct, &list, GFF_STRUCT_ID_EVENT_NODE);
            pEvent->SaveNode(pGFF, &nodeStruct);
            m_eventQueue.GetNext(pNode);
        }
    }
    return 1;
}

// Loads the saved event queue; returns 1 on every path.
int CServerAIMaster::LoadEventQueue(CResGFF* pGFF, CResStruct* pStruct)
{
    CResList list;
    CResStruct nodeStruct;
    if (pGFF->GetList(&list, pStruct, const_cast<char*>("EventQueue")) != 0) {
        // The loop's own rotated first test is the pre-check; the bound stays unsigned.
        for (unsigned int i = 0; i < pGFF->GetListCount(&list); i++) {
            CServerAIEventNode* pEvent = new CServerAIEventNode();
            pGFF->GetListElement(&nodeStruct, &list, i);
            if (pEvent->LoadNode(pGFF, &nodeStruct) != 0) {
                m_eventQueue.AddTail(pEvent);
            } else {
                operator delete(pEvent);
            }
        }
    }
    return 1;
}

// A log-only helper. The id out-parameter gets the id, or "id,tag" when the object has a tag, and
// the type out-parameter a coarse class name.
static void LogEventAddGetObject(unsigned long long id, CExoString& sId, CExoString& sType)
{
    CGameObject* pObj = g_pAppManager->m_pServerExoApp->GetGameObject(id);
    if (pObj == 0) {
        sId.Format(const_cast<char*>("%x"), id);
        sType = "Unknown";
        return;
    }
    CSWSObject* pSWObj = pObj->AsSWSObject();
    if (pSWObj == 0) {
        sId.Format(const_cast<char*>("%x"), id);
        if (pObj->AsSWSArea() != 0) {
            sType = "Area";
        } else {
            sType = (pObj->AsSWSModule() != 0) ? "Module" : "GameObject";
        }
        return;
    }
    if (pSWObj->m_sTag != "") {
        sId.Format(const_cast<char*>("%x,%s"), id, pSWObj->m_sTag.CStr()[0]);
    } else {
        sId.Format(const_cast<char*>("%x"), id);
    }
    if (pSWObj->AsSWSAreaOfEffectObject() != 0) {
        sType = "AOE";
    } else if (pSWObj->AsSWSCreature() != 0) {
        sType = "Creature";
    } else if (pSWObj->AsSWSDoor() != 0) {
        sType = "Door";
    } else if (pSWObj->AsSWSPlaceable() != 0) {
        sType = "Placeable";
    } else if (pSWObj->AsSWSEncounter() != 0) {
        sType = "Encounter";
    } else if (pSWObj->AsSWSTrigger() != 0) {
        sType = "Trigger";
    } else if (pSWObj->AsSWSWaypoint() != 0) {
        sType = "Waypoint";
    } else if (pSWObj->AsSWSItem() != 0) {
        sType = "Item";
    } else if (pSWObj->AsSWSSoundObject() != 0) {
        sType = "Sound";
    } else {
        sType = (pSWObj->AsSWSStore() != 0) ? "Store" : "Object";
    }
}

// Queues an event at an absolute world time, logging it. The two switches are nested: every arm of
// the outer one branches to the shared log block rather than to the second dispatch.
//
// The format prints name(id).
int CServerAIMaster::AddEventAbsoluteTime(unsigned int nDay, unsigned int nTimeOfDay,
                                          unsigned long long oid1, unsigned long long oid2,
                                          unsigned int nType, void* pData)
{
    if (g_bLogEventAdd) {
        CExoString sOid1Id, sOid1Name, sOid2Id, sOid2Name;
        CExoString sTypeName;
        CExoString sScriptEventTypeName;
        CExoString sMsg;
        LogEventAddGetObject(oid2, sOid2Id, sOid2Name);
        LogEventAddGetObject(oid1, sOid1Id, sOid1Name);
        switch (nType) {
        case EVENT_TIMED_EVENT:
            sTypeName = "EVENT_TIMED_EVENT";
            break;
        case EVENT_ENTERED_TRIGGER:
            sTypeName = "EVENT_ENTERED_TRIGGER";
            break;
        case EVENT_LEFT_TRIGGER:
            sTypeName = "EVENT_LEFT_TRIGGER";
            break;
        case EVENT_REMOVE_FROM_AREA:
            sTypeName = "EVENT_REMOVE_FROM_AREA";
            break;
        case EVENT_APPLY_EFFECT:
            sTypeName = "EVENT_APPLY_EFFECT";
            break;
        case EVENT_CLOSE_OBJECT:
            sTypeName = "EVENT_CLOSE_OBJECT";
            break;
        case EVENT_OPEN_OBJECT:
            sTypeName = "EVENT_OPEN_OBJECT";
            break;
        case EVENT_SPELL_IMPACT:
            sTypeName = "EVENT_SPELL_IMPACT";
            break;
        case EVENT_PLAY_ANIMATION:
            sTypeName = "EVENT_PLAY_ANIMATION";
            break;
        case EVENT_SIGNAL_EVENT: {
            sTypeName = "EVENT_SIGNAL_EVENT";
            CScriptEvent* pScriptEvent = static_cast<CScriptEvent*>(pData);
            switch (pScriptEvent->m_eventType) {
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_HEARTBEAT:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_HEARTBEAT";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_PERCEPTION:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_PERCEPTION";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_SPELLCASTAT:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_SPELLCASTAT";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_DAMAGED:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_DAMAGED";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_DISTURBED:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_DISTURBED";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_DIALOGUE:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_DIALOGUE";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_SPAWN_IN:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_SPAWN_IN";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_RESTED:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_RESTED";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_DEATH:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_DEATH";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_USER_DEFINED_EVENT:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_USER_DEFINED_EVENT";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_ENTER:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_ENTER";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_EXIT:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_EXIT";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_ENTER:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_ENTER";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_EXIT:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_EXIT";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_MODULE_START:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_MODULE_START";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_MODULE_LOAD:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_MODULE_LOAD";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_ACTIVATE_ITEM:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_ACTIVATE_ITEM";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_ACQUIRE_ITEM:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_ACQUIRE_ITEM";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_LOSE_ITEM:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_LOSE_ITEM";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_ENCOUNTER_EXHAUSTED:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_ENCOUNTER_EXHAUSTED";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_OPEN:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_OPEN";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_CLOSE:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_CLOSE";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_DISARM:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_DISARM";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_USED:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_USED";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_MINE_TRIGGERED:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_MINE_TRIGGERED";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_INVENTORY_DISTURBED:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_INVENTORY_DISTURBED";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_LOCKED:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_LOCKED";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_UNLOCKED:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_UNLOCKED";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_CLICKED:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_CLICKED";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_PATH_BLOCKED:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_PATH_BLOCKED";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_DYING:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_DYING";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_RESPAWN_BUTTON_PRESSED:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_RESPAWN_BUTTON_PRESSED";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_FAIL_TO_OPEN:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_FAIL_TO_OPEN";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_REST:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_REST";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_DESTROYPLAYERCREATURE:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_DESTROYPLAYERCREATURE";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_LEVEL_UP:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_LEVEL_UP";
                break;
            case CSWSSCRIPTEVENT_EVENTTYPE_ON_EQUIP_ITEM:
                sScriptEventTypeName = "CSWSSCRIPTEVENT_EVENTTYPE_ON_EQUIP_ITEM";
                break;
            default:
                sScriptEventTypeName.Format(const_cast<char*>("EventType(%d)"), pScriptEvent->m_eventType);
                break;
            }
            break;
        }
        case EVENT_DESTROY_OBJECT:
            sTypeName = "EVENT_DESTROY_OBJECT";
            break;
        case EVENT_UNLOCK_OBJECT:
            sTypeName = "EVENT_UNLOCK_OBJECT";
            break;
        case EVENT_LOCK_OBJECT:
            sTypeName = "EVENT_LOCK_OBJECT";
            break;
        case EVENT_REMOVE_EFFECT:
            sTypeName = "EVENT_REMOVE_EFFECT";
            break;
        case EVENT_ON_MELEE_ATTACKED:
            sTypeName = "EVENT_ON_MELEE_ATTACKED";
            break;
        case EVENT_DECREMENT_STACKSIZE:
            sTypeName = "EVENT_DECREMENT_STACKSIZE";
            break;
        case EVENT_SPAWN_BODY_BAG:
            sTypeName = "EVENT_SPAWN_BODY_BAG";
            break;
        case EVENT_FORCED_ACTION:
            sTypeName = "EVENT_FORCED_ACTION";
            break;
        case EVENT_ITEM_ON_HIT_SPELL_IMPACT:
            sTypeName = "EVENT_ITEM_ON_HIT_SPELL_IMPACT";
            break;
        case EVENT_BROADCAST_AOO:
            sTypeName = "EVENT_BROADCAST_AOO";
            break;
        case EVENT_BROADCAST_SAFE_PROJECTILE:
            sTypeName = "EVENT_BROADCAST_SAFE_PROJECTILE";
            break;
        case EVENT_FEEDBACK_MESSAGE:
            sTypeName = "EVENT_FEEDBACK_MESSAGE";
            break;
        case EVENT_ABILITY_EFFECT_APPLIED:
            sTypeName = "EVENT_ABILITY_EFFECT_APPLIED";
            break;
        case EVENT_SUMMON_CREATURE:
            sTypeName = "EVENT_SUMMON_CREATURE";
            break;
        case EVENT_ACQUIRE_ITEM:
            sTypeName = "EVENT_ACQUIRE_ITEM";
            break;
        case EVENT_AREA_TRANSITION:
            sTypeName = "EVENT_AREA_TRANSITION";
            break;
        case EVENT_CONTROLLER_RUMBLE:
            sTypeName = "EVENT_CONTROLLER_RUMBLE";
            break;
        default:
            sTypeName.Format(const_cast<char*>("Event(%d)"), nType);
            break;
        }
        sMsg.Format(const_cast<char*>("DRF Event Added: %s(%s) %s(%s) %s %s\n"),
                    sOid1Name.CStr(), sOid1Id.CStr(), sOid2Name.CStr(), sOid2Id.CStr(),
                    sTypeName.CStr(), sScriptEventTypeName.CStr());
        sMsg.CStr();
    }

    CServerAIEventNode* pEvent = new CServerAIEventNode;
    pEvent->m_nTimeDay = nDay;
    pEvent->m_nTimeOfDay = nTimeOfDay;
    pEvent->m_oidCaller = oid1;
    pEvent->m_oidObject = oid2;
    pEvent->m_nType = nType;
    pEvent->m_pData = pData;

    CExoLinkedListNode* pNode = m_eventQueue.m_pInternal->m_pHead;
    // The insert branch returns early and the append path falls out of the loop to its own return.
    while (pNode != 0) {
        CServerAIEventNode* pPending =
            static_cast<CServerAIEventNode*>(m_eventQueue.GetAtPos(pNode));
        if (m_pExoAppInternal->m_pWorldTimer->CompareWorldTimes(
                pPending->m_nTimeDay, pPending->m_nTimeOfDay, nDay, nTimeOfDay) == 1) {
            m_eventQueue.AddBefore(pEvent, pNode);
            return 1;
        }
        m_eventQueue.GetNext(pNode);
    }
    m_eventQueue.AddTail(pEvent);
    return 1;
}

// Writes the five flat header fields, then a switch over the event type whose arms serialize the
// tagged payload. Several arms share a tail but the cases stay separate, and types 9 and 20 share a
// GFF struct tag while writing different field widths.
int CServerAIEventNode::SaveNode(CResGFF* pGFF, CResStruct* pStruct)
{
    pGFF->WriteFieldDWORD(pStruct, m_nTimeDay, const_cast<char*>("Day"));
    pGFF->WriteFieldDWORD(pStruct, m_nTimeOfDay, const_cast<char*>("Time"));
    pGFF->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_oidObject), const_cast<char*>("ObjectId"));
    pGFF->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_oidCaller), const_cast<char*>("CallerId"));
    pGFF->WriteFieldDWORD(pStruct, m_nType, const_cast<char*>("EventId"));

    CResStruct dataStruct;
    switch (m_nType) {
    case CServerAIMaster::EVENT_TIMED_EVENT:
        pGFF->AddStructToStruct(&dataStruct, pStruct, const_cast<char*>("EventData"), GFF_STRUCT_ID_EVENT_DATA_SITUATION);
        g_pVirtualMachine->SaveScriptSituation(m_pData, pGFF, &dataStruct);
        break;
    case CServerAIMaster::EVENT_REMOVE_FROM_AREA:
        if (m_pData != 0) {
            pGFF->AddStructToStruct(&dataStruct, pStruct, const_cast<char*>("EventData"), GFF_STRUCT_ID_EVENT_DATA_AREA_FLAG);
            pGFF->WriteFieldBYTE(&dataStruct, static_cast<unsigned char>(reinterpret_cast<unsigned long long>(m_pData)), const_cast<char*>("Value"));
        }
        break;
    case CServerAIMaster::EVENT_APPLY_EFFECT:
    case CServerAIMaster::EVENT_REMOVE_EFFECT:
        if (m_pData != 0) {
            pGFF->AddStructToStruct(&dataStruct, pStruct, const_cast<char*>("EventData"), GFF_STRUCT_ID_EVENT_DATA_EFFECT);
            static_cast<CGameEffect*>(m_pData)->SaveGameEffect(pGFF, &dataStruct);
        }
        break;
    case CServerAIMaster::EVENT_SPELL_IMPACT:
    case CServerAIMaster::EVENT_ITEM_ON_HIT_SPELL_IMPACT:
        if (m_pData != 0) {
            pGFF->AddStructToStruct(&dataStruct, pStruct, const_cast<char*>("EventData"), GFF_STRUCT_ID_EVENT_DATA_SPELL);
            static_cast<CSWSSpellScriptData*>(m_pData)->SaveData(pGFF, &dataStruct);
        }
        break;
    case CServerAIMaster::EVENT_PLAY_ANIMATION:
        if (m_pData != 0) {
            pGFF->AddStructToStruct(&dataStruct, pStruct, const_cast<char*>("EventData"), GFF_STRUCT_ID_EVENT_DATA_VALUE);
            pGFF->WriteFieldINT(&dataStruct, static_cast<int>(reinterpret_cast<unsigned long long>(m_pData)), const_cast<char*>("Value"));
        }
        break;
    case CServerAIMaster::EVENT_SIGNAL_EVENT:
        if (m_pData != 0) {
            pGFF->AddStructToStruct(&dataStruct, pStruct, const_cast<char*>("EventData"), GFF_STRUCT_ID_EVENT_DATA_SCRIPT);
            static_cast<CScriptEvent*>(m_pData)->SaveEvent(pGFF, &dataStruct);
        }
        break;
    case CServerAIMaster::EVENT_ON_MELEE_ATTACKED:
    case CServerAIMaster::EVENT_BROADCAST_SAFE_PROJECTILE:
        if (m_pData != 0) {
            pGFF->AddStructToStruct(&dataStruct, pStruct, const_cast<char*>("EventData"), GFF_STRUCT_ID_EVENT_DATA_ATTACK);
            static_cast<CSWSCombatAttackData*>(m_pData)->SaveData(pGFF, &dataStruct);
        }
        break;
    case CServerAIMaster::EVENT_SPAWN_BODY_BAG:
        if (m_pData != 0) {
            pGFF->AddStructToStruct(&dataStruct, pStruct, const_cast<char*>("EventData"), GFF_STRUCT_ID_EVENT_DATA_BODY_BAG);
            static_cast<CServerAIBodyBagInfo*>(m_pData)->SaveBodyBag(pGFF, &dataStruct);
        }
        break;
    case CServerAIMaster::EVENT_FORCED_ACTION:
        if (m_pData != 0) {
            pGFF->AddStructToStruct(&dataStruct, pStruct, const_cast<char*>("EventData"), GFF_STRUCT_ID_EVENT_DATA_FORCED);
            static_cast<CSWSForcedAction*>(m_pData)->SaveForcedAction(pGFF, &dataStruct);
        }
        break;
    case CServerAIMaster::EVENT_BROADCAST_AOO:
        if (m_pData != 0) {
            pGFF->AddStructToStruct(&dataStruct, pStruct, const_cast<char*>("EventData"), GFF_STRUCT_ID_EVENT_DATA_VALUE);
            pGFF->WriteFieldDWORD(&dataStruct, static_cast<unsigned int>(reinterpret_cast<unsigned long long>(m_pData)), const_cast<char*>("Value"));
        }
        break;
    case CServerAIMaster::EVENT_FEEDBACK_MESSAGE:
        if (m_pData != 0) {
            pGFF->AddStructToStruct(&dataStruct, pStruct, const_cast<char*>("EventData"), GFF_STRUCT_ID_EVENT_DATA_FEEDBACK);
            static_cast<CSWCCMessageData*>(m_pData)->SaveData(pGFF, &dataStruct);
        }
        break;
    case CServerAIMaster::EVENT_SUMMON_CREATURE:
        if (m_pData != 0) {
            pGFF->AddStructToStruct(&dataStruct, pStruct, const_cast<char*>("EventData"), GFF_STRUCT_ID_EVENT_DATA_SUMMON);
            static_cast<CScriptEvent*>(m_pData)->SaveEvent(pGFF, &dataStruct);
        }
        break;
    case CServerAIMaster::EVENT_AREA_TRANSITION:
        if (m_pData != 0) {
            pGFF->AddStructToStruct(&dataStruct, pStruct, const_cast<char*>("EventData"), GFF_STRUCT_ID_EVENT_DATA_TRANSITION);
            static_cast<CScriptEvent*>(m_pData)->SaveEvent(pGFF, &dataStruct);
        }
        break;
    case CServerAIMaster::EVENT_CONTROLLER_RUMBLE:
        pGFF->AddStructToStruct(&dataStruct, pStruct, const_cast<char*>("EventData"), GFF_STRUCT_ID_EVENT_DATA_RUMBLE);
        pGFF->WriteFieldINT(&dataStruct, static_cast<int>(reinterpret_cast<unsigned long long>(m_pData)), const_cast<char*>("Value"));
        break;
    default:
        break;
    }
    return 1;
}

// Saves the body-bag payload.
int CServerAIBodyBagInfo::SaveBodyBag(CResGFF* pGFF, CResStruct* pStruct)
{
    pGFF->WriteFieldDWORD(pStruct, static_cast<unsigned int>(m_oidBodyBag), const_cast<char*>("BodyBagId"));
    pGFF->WriteFieldFLOAT(pStruct, m_fX, const_cast<char*>("PositionX"));
    pGFF->WriteFieldFLOAT(pStruct, m_fY, const_cast<char*>("PositionY"));
    pGFF->WriteFieldFLOAT(pStruct, m_fZ, const_cast<char*>("PositionZ"));
    return 1;
}

// Loads the body-bag payload; one success flag is shared by all four reads.
int CServerAIBodyBagInfo::LoadBodyBag(CResGFF* pGFF, CResStruct* pStruct)
{
    int bSuccess;
    m_oidBodyBag = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("BodyBagId"), bSuccess, 0);
    m_fX = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("PositionX"), bSuccess, 0.0f);
    m_fY = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("PositionY"), bSuccess, 0.0f);
    m_fZ = pGFF->ReadFieldFLOAT(pStruct, const_cast<char*>("PositionZ"), bSuccess, 0.0f);
    return 1;
}

// Mirrors SaveNode, reading back the same five flat header fields, then the payload switch. The
// asymmetries with SaveNode are real and should not be corrected:
//
// * EVENT_FORCED_ACTION has no load arm at all, so LoadNode fails for it and the queue deletes the
//   node, while SaveNode does write one
// * ids 2, 3, 6, 7, 11, 12, 13 and 16 share the null-payload arm
// * ids 19, 23, 25 and anything outside 1..27 return success with no store
// * EVENT_SUMMON_CREATURE and EVENT_AREA_TRANSITION are two separate arms, as in SaveNode
int CServerAIEventNode::LoadNode(CResGFF* pGFF, CResStruct* pStruct)
{
    int bSuccess;
    m_nTimeDay = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("Day"), bSuccess, 0);
    m_nTimeOfDay = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("Time"), bSuccess, 0);
    m_oidObject = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("ObjectId"), bSuccess, 0);
    m_oidCaller = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("CallerId"), bSuccess, 0);
    m_nType = pGFF->ReadFieldDWORD(pStruct, const_cast<char*>("EventId"), bSuccess, 0);

    CResStruct dataStruct;
    switch (m_nType) {
    case CServerAIMaster::EVENT_TIMED_EVENT:
        pGFF->GetStructFromStruct(&dataStruct, pStruct, const_cast<char*>("EventData"));
        g_pVirtualMachine->LoadScriptSituation(&m_pData, pGFF, &dataStruct);
        return 1;
    case CServerAIMaster::EVENT_REMOVE_FROM_AREA:
        m_pData = 0;
        if (pGFF->GetStructFromStruct(&dataStruct, pStruct, const_cast<char*>("EventData")) != 0) {
            m_pData = reinterpret_cast<void*>(static_cast<unsigned long long>(pGFF->ReadFieldBYTE(
                &dataStruct, const_cast<char*>("Value"), bSuccess, 0)));
        }
        return 1;
    case CServerAIMaster::EVENT_APPLY_EFFECT:
    case CServerAIMaster::EVENT_REMOVE_EFFECT:
        m_pData = new CGameEffect(1);
        pGFF->GetStructFromStruct(&dataStruct, pStruct, const_cast<char*>("EventData"));
        static_cast<CGameEffect*>(m_pData)->LoadGameEffect(pGFF, &dataStruct);
        return 1;
    case CServerAIMaster::EVENT_SPELL_IMPACT:
        // The parentheses are load-bearing, because value-init zeroes the whole object before the
        // string constructor runs.
        m_pData = new CSWSSpellScriptData();
        pGFF->GetStructFromStruct(&dataStruct, pStruct, const_cast<char*>("EventData"));
        static_cast<CSWSSpellScriptData*>(m_pData)->LoadData(pGFF, &dataStruct);
        return 1;
    case CServerAIMaster::EVENT_PLAY_ANIMATION:
        m_pData = 0;
        if (pGFF->GetStructFromStruct(&dataStruct, pStruct, const_cast<char*>("EventData")) != 0) {
            m_pData = reinterpret_cast<void*>(static_cast<long long>(pGFF->ReadFieldINT(
                &dataStruct, const_cast<char*>("Value"), bSuccess, 0)));
        }
        return 1;
    case CServerAIMaster::EVENT_SIGNAL_EVENT:
        m_pData = new CScriptEvent;
        pGFF->GetStructFromStruct(&dataStruct, pStruct, const_cast<char*>("EventData"));
        static_cast<CScriptEvent*>(m_pData)->LoadEvent(pGFF, &dataStruct);
        return 1;
    case CServerAIMaster::EVENT_ON_MELEE_ATTACKED:
    case CServerAIMaster::EVENT_BROADCAST_SAFE_PROJECTILE:
        m_pData = 0;
        if (pGFF->GetStructFromStruct(&dataStruct, pStruct, const_cast<char*>("EventData")) != 0) {
            CSWSCombatAttackData* pData = new CSWSCombatAttackData;
            m_pData = pData;
            pData->LoadData(pGFF, &dataStruct);
        }
        return 1;
    case CServerAIMaster::EVENT_SPAWN_BODY_BAG:
        m_pData = new CServerAIBodyBagInfo();
        pGFF->GetStructFromStruct(&dataStruct, pStruct, const_cast<char*>("EventData"));
        static_cast<CServerAIBodyBagInfo*>(m_pData)->LoadBodyBag(pGFF, &dataStruct);
        return 1;
    case CServerAIMaster::EVENT_FORCED_ACTION:
        // No payload load: this type loads as a failure, so the queue deletes the node.
        return 0;
    case CServerAIMaster::EVENT_ITEM_ON_HIT_SPELL_IMPACT:
    case CServerAIMaster::EVENT_ABILITY_EFFECT_APPLIED:
    case CServerAIMaster::EVENT_ACQUIRE_ITEM:
        // These three share the plain success return with no payload handling.
        return 1;
    case CServerAIMaster::EVENT_BROADCAST_AOO:
        m_pData = 0;
        if (pGFF->GetStructFromStruct(&dataStruct, pStruct, const_cast<char*>("EventData")) != 0) {
            m_pData = reinterpret_cast<void*>(static_cast<unsigned long long>(pGFF->ReadFieldDWORD(
                &dataStruct, const_cast<char*>("Value"), bSuccess, 0)));
        }
        return 1;
    case CServerAIMaster::EVENT_FEEDBACK_MESSAGE:
        m_pData = 0;
        if (pGFF->GetStructFromStruct(&dataStruct, pStruct, const_cast<char*>("EventData")) != 0) {
            CSWCCMessageData* pData = new CSWCCMessageData;
            m_pData = pData;
            pData->LoadData(pGFF, &dataStruct);
        }
        return 1;
    case CServerAIMaster::EVENT_AREA_TRANSITION:
        m_pData = new CScriptEvent;
        pGFF->GetStructFromStruct(&dataStruct, pStruct, const_cast<char*>("EventData"));
        static_cast<CScriptEvent*>(m_pData)->LoadEvent(pGFF, &dataStruct);
        return 1;
    case CServerAIMaster::EVENT_SUMMON_CREATURE:
        m_pData = new CScriptEvent;
        pGFF->GetStructFromStruct(&dataStruct, pStruct, const_cast<char*>("EventData"));
        static_cast<CScriptEvent*>(m_pData)->LoadEvent(pGFF, &dataStruct);
        return 1;
    case CServerAIMaster::EVENT_CONTROLLER_RUMBLE:
        m_pData = 0;
        if (pGFF->GetStructFromStruct(&dataStruct, pStruct, const_cast<char*>("EventData")) != 0) {
            m_pData = reinterpret_cast<void*>(static_cast<long long>(pGFF->ReadFieldINT(
                &dataStruct, const_cast<char*>("Value"), bSuccess, 0)));
        }
        return 1;
    // Only these eight in-range ids zero the payload slot.
    case CServerAIMaster::EVENT_ENTERED_TRIGGER:
    case CServerAIMaster::EVENT_LEFT_TRIGGER:
    case CServerAIMaster::EVENT_CLOSE_OBJECT:
    case CServerAIMaster::EVENT_OPEN_OBJECT:
    case CServerAIMaster::EVENT_DESTROY_OBJECT:
    case CServerAIMaster::EVENT_UNLOCK_OBJECT:
    case CServerAIMaster::EVENT_LOCK_OBJECT:
    case CServerAIMaster::EVENT_DECREMENT_STACKSIZE:
        m_pData = 0;
        return 1;
    default:
        return 1;
    }
}
