#pragma once
#include "ClientExoAppInternal.h"
#include "nwcobject.h"

#include "gameobjectarray.h"

#include "exolinkedlistinternal.h"
#include "CExoLinkedList.h"

class CSWCObject;

#include "clientexoapp.h"
#include "appmanager.h"

// The AI roster, one intrusive linked list of game-object ids plus a round-robin iteration cursor.
class CClientAIList {
public:
    int                                m_nCount;
    CExoLinkedList<unsigned long long> m_list;
    CExoLinkedListNode*                m_pCursorNode;

    CClientAIList();
    ~CClientAIList();

    int AddObject(CSWCObject* pObject);        // 0 = already present, 1 = added
    int RemoveObject(CSWCObject* pObject);      // 1 = not present, 0 = removed
    CSWCObject* GetNextObject();
};

// The client-side AI object roster, the client mirror of the server-side CServerAIMaster scheduler.
class CClientAIMaster {
public:
    CClientExoAppInternal*  m_pExoAppInternal;
    CClientAIList             m_list;

    CClientAIMaster();
    ~CClientAIMaster();

    void SetExoAppInternal(CClientExoAppInternal* pApp);

    // Spreads the per-frame AI and animation upkeep over the roster.
    void UpdateState();
    // 1 when no creature's radius reaches the segment (fX1,fY1)-(fX2,fY2), 0 as soon as one
    // does. The walk starts at the round-robin cursor and stops when it returns to the object it
    // started from.
    int NoCreaturesOnLine(float fX1, float fY1, float fX2, float fY2,
                          unsigned long long nIgnoreId, float fExtraRadius);

    int AddObject(CSWCObject* pObject);
    int RemoveObject(CSWCObject* pObject);
};
