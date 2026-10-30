#include "clientaimaster.h"

#include "exobase.h"
#include "nwccreature.h"
#include "nwccreatureappearance.h"
#include "worldtimer.h"

extern CAppManager* g_pAppManager;

float g_AIModuleTime2;
float g_AIObjectTime;
static uint64_ll_t g_nAITimerSnapshot;

CClientAIList::CClientAIList()
{
    m_pCursorNode = 0;
    m_nCount = 0;
}

CClientAIList::~CClientAIList()
{
    CExoLinkedListNode* pNode = m_list.GetHeadPos();
    while (pNode != 0) {
        CGameObject* pGameObject = g_pAppManager->m_pClientExoApp->m_pInternal->GetGameObject(
            reinterpret_cast<unsigned long long>(pNode->m_pData));
        // AddObject only ever stores ids taken from a live object, so the downcast is unchecked.
        RemoveObject(static_cast<CSWCObject*>(pGameObject->AsSWCObject()));
        pNode = m_list.GetHeadPos();
    }
}

int CClientAIList::AddObject(CSWCObject* pObject)
{
    CExoLinkedListNode* pNode = m_list.m_pInternal->m_pHead;
    for (;;) {
        if (pNode == 0) {
            m_nCount++;
            pNode = m_list.AddTail(reinterpret_cast<unsigned long long*>(pObject->m_objectId));
            pObject->m_pAINode = pNode;
            if (m_pCursorNode == 0) {
                m_pCursorNode = pNode;
            }
            return 1;
        }
        if (reinterpret_cast<unsigned long long>(m_list.GetAtPos(pNode)) == pObject->m_objectId) {
            return 0;
        }
        m_list.GetNext(pNode);
    }
}

int CClientAIList::RemoveObject(CSWCObject* pObject)
{
    CExoLinkedListNode* pNode = pObject->m_pAINode;
    if (pNode == 0) {
        return 1;
    }
    if (m_pCursorNode == pNode) {
        m_list.GetNext(m_pCursorNode);
        if (m_pCursorNode == 0) {
            m_pCursorNode = (m_list.m_pInternal->m_pHead != pNode) ? m_list.m_pInternal->m_pHead : 0;
        }
    }
    m_nCount--;
    m_list.Remove(pNode);
    pObject->m_pAINode = 0;
    return 0;
}

CSWCObject* CClientAIList::GetNextObject()
{
    if (m_list.m_pInternal->m_pHead == 0) {
        return 0;
    }
    m_list.GetNext(m_pCursorNode);
    if (m_pCursorNode == 0) {
        m_pCursorNode = m_list.m_pInternal->m_pHead;
    }
    unsigned long long id = reinterpret_cast<unsigned long long>(m_list.GetAtPos(m_pCursorNode));
    CGameObject* pGameObject = g_pAppManager->m_pClientExoApp->m_pInternal->GetGameObject(id);
    if (pGameObject == 0) {
        return 0;
    }
    return static_cast<CSWCObject*>(pGameObject->AsSWCObject());
}

CClientAIMaster::CClientAIMaster()
{
}

CClientAIMaster::~CClientAIMaster()
{
}

void CClientAIMaster::SetExoAppInternal(CClientExoAppInternal* pApp)
{
    m_pExoAppInternal = pApp;
}

// Runs one AI tick: gives the round-robin roster m_list.m_nCount turns, calling each object's
// AIUpdate before advancing the cursor, then advances the loaded module by the pause timer's
// snapshot delta. The loop's entry guard tests both the cursor and the count, but the back edge
// tests the count alone, so a null object only skips its AIUpdate and does not end the walk.
void CClientAIMaster::UpdateState()
{
    CSWCObject* pObject = m_list.GetNextObject();
    int nCount = m_list.m_nCount;
    g_nAITimerSnapshot = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    if (pObject != 0 && nCount >= 1) {
        do {
            if (pObject != 0)
                pObject->AIUpdate();
            nCount--;
            pObject = m_list.GetNextObject();
        } while (nCount > 0);
    }
    g_AIObjectTime = static_cast<float>(static_cast<int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() -
                                  g_nAITimerSnapshot)) * 0.001f;
    g_nAITimerSnapshot = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    pModule->Update(static_cast<float>(g_pAppManager->m_pClientExoApp->m_pInternal->m_pPauseTimer->
                        GetSnapshotTimeDifference()) / 1000000.0f);
    g_AIModuleTime2 = static_cast<float>(static_cast<int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() -
                                   g_nAITimerSnapshot)) * 0.001f;
}

// Tests the segment (fX1,fY1)-(fX2,fY2) against every roster creature: 1 when clear, 0 when a
// creature's circle reaches it. The walk is round-robin from the list cursor and stops when it comes
// back to the object it started from.
int CClientAIMaster::NoCreaturesOnLine(float fX1, float fY1, float fX2, float fY2,
                                       unsigned long long nIgnoreId, float fExtraRadius)
{
    // A degenerate segment carries no direction, so it can never be crossed.
    if (fX2 == fX1 && fY2 == fY1)
        return 1;

    float fMaxX = (fX2 > fX1) ? fX2 : fX1;
    float fMinX = (fX2 < fX1) ? fX2 : fX1;
    float fMaxY = (fY2 > fY1) ? fY2 : fY1;
    float fMinY = (fY2 < fY1) ? fY2 : fY1;
    float fDX = fX2 - fX1;
    // The slope/intercept/normaliser triple the sloped form needs; a vertical segment leaves the
    // normaliser unset, and only the sloped form (fDX != 0) reads it.
    float fSlope;
    float fIntercept;
    float fInvLen K2_UNSET;
    if (fDX == 0.0f) {
        fSlope = 0.0f;
        fIntercept = 0.0f;
    } else {
        fSlope = (fY2 - fY1) / fDX;
        fIntercept = fY1 - fSlope * fX1;
        fInvLen = __builtin_sqrtf(1.0f / (fSlope * fSlope + 1.0f));
    }

    CSWCObject* pObject = m_list.GetNextObject();
    if (pObject == 0)
        return 1;
    CSWCObject* pFirstObject = pObject;

    do {
        if (pObject != 0
            && static_cast<unsigned long long>(pObject->m_objectId) != nIgnoreId
            && pObject->m_objectType == GAME_OBJECT_TYPE_CREATURE) {
            CSWCCreature* pCreature = pObject->AsSWCCreature();
            float fRadius = pCreature->m_pCreatureAppearance->m_fPersonalSpace + fExtraRadius;
            float fRadius2 = fRadius * fRadius;
            float fX = pObject->m_position.x;
            float fY = pObject->m_position.y;

            if (fDX == 0.0f) {
                if (fY <= fMinY) {
                    fX -= fMinX;
                    fY -= fMinY;
                    if (fX * fX + fY * fY + 0.0f <= fRadius2)
                        return 0;
                } else if (fY >= fMaxY) {
                    fX -= fMaxX;
                    fY -= fMaxY;
                    if (fX * fX + fY * fY + 0.0f <= fRadius2)
                        return 0;
                } else if (fX >= fMaxX - fRadius && fX <= fMaxX + fRadius) {
                    return 0;
                }
            } else {
                float fLineY = fIntercept + fSlope * fX;
                float fPerp = __builtin_fabsf(fY - fLineY) * fInvLen;
                if (fPerp < fRadius) {
                    // The projection of the creature onto the line, walked back out to x.
                    float fSign = (fY < fLineY) ? -1.0f : 1.0f;
                    float fProjX = fX + fSlope * (fSign * fPerp) * fInvLen;
                    if (fProjX >= fMinX && fProjX <= fMaxX)
                        return 0;
                }
                fX -= fX2;
                fY -= fY2;
                if (fX * fX + fY * fY + 0.0f <= fRadius2)
                    return 0;
            }
        }
        pObject = m_list.GetNextObject();
    } while (pObject != pFirstObject);

    return 1;
}

int CClientAIMaster::AddObject(CSWCObject* pObject)
{
    return m_list.AddObject(pObject);
}

int CClientAIMaster::RemoveObject(CSWCObject* pObject)
{
    return m_list.RemoveObject(pObject);
}
