#pragma once

// idiom-exempt: canonical

// The server-side AI event scheduler. It owns a per-AI-level object roster and a time-ordered queue of
// event records used to schedule delayed AI callbacks.

#include "exostring.h"
#include "CExoLinkedList.h"
#include "CExoArrayListDtors.h"

struct CServerAIEventNode;

class CServerExoAppInternal;
class CExoLinkedListInternal;
class CSWSObject;
class CGameObject;
class CGameEffect;
class CSWItemProperty;
class CSWSItem;
class CSWSCreature;
class CResGFF;
struct CResStruct;
class CSWSEffectListHandler;
class CSWSItemPropertyHandler;

// One AI-level roster: a dynamic array of game object ids plus a saved iteration cursor. Five of these
// are embedded by value.
class CServerAIList {
public:
    CExoArrayList<unsigned long long> m_aObjectIds;
    int                 m_nIterIndex;

    CServerAIList();
    ~CServerAIList();

    int          AddObject(unsigned long long id);
    CGameObject* GetObjectAtPosition(int index);
    int          RemoveObject(unsigned long long id);
    CGameObject* GetNextObject();
};

class CServerAIMaster {
public:
    // Event-type tags for the queued event records, defined in the unit as real objects.
    static const int EVENT_TIMED_EVENT;               // 1
    static const int EVENT_ENTERED_TRIGGER;           // 2
    static const int EVENT_LEFT_TRIGGER;              // 3
    static const int EVENT_REMOVE_FROM_AREA;          // 4
    static const int EVENT_APPLY_EFFECT;              // 5
    static const int EVENT_CLOSE_OBJECT;              // 6
    static const int EVENT_OPEN_OBJECT;               // 7
    static const int EVENT_SPELL_IMPACT;              // 8
    static const int EVENT_PLAY_ANIMATION;            // 9
    static const int EVENT_SIGNAL_EVENT;              // 10
    static const int EVENT_DESTROY_OBJECT;            // 11
    static const int EVENT_UNLOCK_OBJECT;             // 12
    static const int EVENT_LOCK_OBJECT;               // 13
    static const int EVENT_REMOVE_EFFECT;             // 14
    static const int EVENT_ON_MELEE_ATTACKED;         // 15
    static const int EVENT_DECREMENT_STACKSIZE;       // 16
    static const int EVENT_SPAWN_BODY_BAG;            // 17
    static const int EVENT_FORCED_ACTION;             // 18
    static const int EVENT_ITEM_ON_HIT_SPELL_IMPACT;  // 19
    static const int EVENT_BROADCAST_AOO;             // 20
    static const int EVENT_BROADCAST_SAFE_PROJECTILE; // 21
    static const int EVENT_FEEDBACK_MESSAGE;          // 22
    static const int EVENT_ABILITY_EFFECT_APPLIED;    // 23
    static const int EVENT_SUMMON_CREATURE;           // 24
    static const int EVENT_ACQUIRE_ITEM;              // 25
    static const int EVENT_AREA_TRANSITION;           // 26
    static const int EVENT_CONTROLLER_RUMBLE;         // 27

    // The AI update's time slice, in high-resolution timer units.
    static const long long AI_UPDATE_TIME_HI_RES;     // 10000

    CServerExoAppInternal*   m_pExoAppInternal;
    CServerAIList            m_lists[5];
    CExoLinkedList<CServerAIEventNode> m_eventQueue;
    CSWSEffectListHandler*   m_pEffectListHandler;
    CSWSItemPropertyHandler* m_pItemPropertyHandler;
    // A comma-separated list of the scripts run this update.
    CExoString               m_sScriptsRun;
    int                      m_bRanInterAreaPath;
    int                      m_bRanGridPath;
    int                      m_bRanVisibilityUpdate;
    int                      m_bReserved1;
    int                      m_fChallengeRating;
    int                      m_nLastActionId;
    // One list per repadjust.2da row (Attack, Theft, Kill, Help), holding that row's eleven
    // reputation columns in column order.
    CExoArrayList<int>*      m_pRepAdjustAttack;
    CExoArrayList<int>*      m_pRepAdjustTheft;
    CExoArrayList<int>*      m_pRepAdjustKill;
    CExoArrayList<int>*      m_pRepAdjustHelp;

    CServerAIMaster();
    ~CServerAIMaster();

    void SetExoAppInternal(CServerExoAppInternal* pApp);
    int  SetAILevel(CSWSObject* pObj, int level);
    void ClearEventQueue();
    int  GetPendingEvent(unsigned int* pA, unsigned int* pB, unsigned long long* pC,
                         unsigned long long* pD, unsigned int* pType, void** ppData);
    void DeleteEventData(unsigned int type, void* pData);
    int  AddObject(CSWSObject* pObj, int level);
    int  RemoveObject(CSWSObject* pObj);

    // Returns the effect handler's verdict, which can veto the effect.
    int OnEffectApplied(CSWSObject* pObj, CGameEffect* pEffect, int a);
    int  OnEffectRemoved(CSWSObject* pObj, CGameEffect* pEffect);
    void OnItemPropertyApplied(CSWSItem* pItem, CSWItemProperty* pProp,
                               CSWSCreature* pCreature, unsigned int a, int b);
    void OnItemPropertyRemoved(CSWSItem* pItem, CSWItemProperty* pProp,
                               CSWSCreature* pCreature, unsigned int a);
    void AdjustTargetAndWitnessReputations(unsigned long long a, unsigned long long b, int c);

    int AddEventDeltaTime(unsigned int a, unsigned int b, unsigned long long c,
                          unsigned long long d, unsigned int type, void* pEvent);
    int AddEventAbsoluteTime(unsigned int a, unsigned int b, unsigned long long c,
                             unsigned long long d, unsigned int type, void* pEvent);
    void UpdateState();
    int  EventPending(unsigned int a, unsigned int b);
    int  LoadEventQueue(CResGFF* pGff, CResStruct* pStruct);
    int  SaveEventQueue(CResGFF* pGff, CResStruct* pStruct);
    void AppendToScriptsRun(const CExoString& script);
    void LoadReputationAdjustments();
};

#include "nwsscriptevent.h"
#include "nwscript.h"
#include "nwseffectlist.h"

#include "exolinkedlistinternal.h"
#include "serverexoapp.h"
#include "serverexoappinternal.h"
#include "appmanager.h"
#include "nwsrules.h"
#include "resgff.h"

#include "nwsobject.h"

class CGameEffect;
class CSWItemProperty;
class CSWSItem;
class CSWSCreature;

#include "nwsitemproperty.h"

#include "nwseffect.h"
#include "nwsforcedaction.h"
#include "nwsplaceable.h"

extern CVirtualMachine* g_pVirtualMachine;
// Enables the debug logging in AddEventAbsoluteTime.
extern int g_bLogEventAdd;

// One queued AI event. The first two words are the event's world time, the next two the game object ids
// the event concerns.
struct CServerAIEventNode {
    unsigned int       m_nTimeDay;
    unsigned int       m_nTimeOfDay;
    unsigned long long m_oidCaller;
    unsigned long long m_oidObject;
    unsigned int       m_nType;
    void*              m_pData;

    int SaveNode(CResGFF* pGFF, CResStruct* pStruct);
    int LoadNode(CResGFF* pGFF, CResStruct* pStruct);
};
K2_LAYOUT_ASSERT(sizeof(CServerAIEventNode) == 0x28, "CServerAIEventNode size drift");

// The payload of the spawn-body-bag event.
struct CServerAIBodyBagInfo {
    unsigned long long m_oidBodyBag;
    float              m_fX;
    float              m_fY;
    float              m_fZ;

    int SaveBodyBag(CResGFF* pGFF, CResStruct* pStruct);
    int LoadBodyBag(CResGFF* pGFF, CResStruct* pStruct);
};
#include "nwscombatround.h"
#include "nwmessage.h"
