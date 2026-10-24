#pragma once

#include "CExoArrayListDtors.h"

// CScriptEvent::m_eventType: the script hook an EVENT_SIGNAL_EVENT fires on its target, named as
// CServerAIMaster's event dump spells them.
enum {
    CSWSSCRIPTEVENT_EVENTTYPE_ON_HEARTBEAT              = 0,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_PERCEPTION             = 1,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_SPELLCASTAT            = 2,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_DAMAGED                = 4,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_DISTURBED              = 5,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_DIALOGUE               = 7,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_SPAWN_IN               = 8,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_RESTED                 = 9,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_DEATH                  = 10,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_USER_DEFINED_EVENT     = 11,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_ENTER           = 12,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_OBJECT_EXIT            = 13,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_ENTER           = 14,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_EXIT            = 15,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_MODULE_START           = 16,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_MODULE_LOAD            = 17,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_ACTIVATE_ITEM          = 18,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_ACQUIRE_ITEM           = 19,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_LOSE_ITEM              = 20,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_ENCOUNTER_EXHAUSTED    = 21,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_OPEN                   = 22,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_CLOSE                  = 23,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_DISARM                 = 24,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_USED                   = 25,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_MINE_TRIGGERED         = 26,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_INVENTORY_DISTURBED    = 27,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_LOCKED                 = 28,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_UNLOCKED               = 29,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_CLICKED                = 30,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_PATH_BLOCKED           = 31,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_DYING           = 32,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_RESPAWN_BUTTON_PRESSED = 33,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_FAIL_TO_OPEN           = 34,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_REST            = 35,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_DESTROYPLAYERCREATURE  = 36,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_PLAYER_LEVEL_UP        = 37,
    CSWSSCRIPTEVENT_EVENTTYPE_ON_EQUIP_ITEM             = 38,
};

// Integer 0 of an ON_PERCEPTION event: what the perceiver noticed about object 0 (the
// GetLastPerceptionHeard/Inaudible/Seen/Vanished flag it sets).
enum {
    PERCEPTION_EVENT_HEARD     = 0,
    PERCEPTION_EVENT_INAUDIBLE = 1,
    PERCEPTION_EVENT_SEEN      = 2,
    PERCEPTION_EVENT_VANISHED  = 3,
};

class CExoString;
class CResGFF;
struct CResStruct;

// The four parameter families as one aggregate, so the ctor can zero the whole block at once.
struct CScriptEventParams {
    CExoArrayList<int>                m_aIntegers;
    CExoArrayList<float>              m_aFloats;
    CExoArrayList<CExoString>         m_aStrings;
    CExoArrayList<unsigned long long> m_aObjects;

    CScriptEventParams() { __builtin_memset(static_cast<void*>(this), 0, sizeof(*this)); }
};

class CScriptEvent {
public:
    unsigned short m_eventType;
    CScriptEventParams m_params;

    CScriptEvent();
    ~CScriptEvent();
    void SetObjectID(int index, unsigned long long objectId);
    void SetInteger(int index, int value);
    void SetString(int index, const CExoString& sValue);
    CExoString GetString(int index);
    int  GetInteger(int index);
    unsigned long long GetObjectID(int index);
    void  SetFloat(int index, float value);
    float GetFloat(int index);

    void CopyScriptEvent(CScriptEvent* pSource);
    int  SaveEvent(CResGFF* pGff, CResStruct* pStruct);
    int  LoadEvent(CResGFF* pGff, CResStruct* pStruct);
    int  operator!=(const CScriptEvent& other) const;
    int  operator==(const CScriptEvent& other) const;
};
