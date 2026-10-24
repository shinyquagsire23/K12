#include "gameobjectarray.h"
#include "k2stdint.h"

const unsigned char CGameObject::OBJECT_TYPE_MODULE = 3;
const unsigned char CGameObject::OBJECT_TYPE_AREA = 4;
const unsigned char CGameObject::OBJECT_TYPE_CREATURE = 5;
const unsigned char CGameObject::OBJECT_TYPE_ITEM = 6;
const unsigned char CGameObject::OBJECT_TYPE_TRIGGER = 7;
const unsigned char CGameObject::OBJECT_TYPE_PROJECTILE = 8;
const unsigned char CGameObject::OBJECT_TYPE_PLACEABLE = 9;
const unsigned char CGameObject::OBJECT_TYPE_DOOR = 10;
const unsigned char CGameObject::OBJECT_TYPE_AREAOFEFFECTOBJECT = 11;
const unsigned char CGameObject::OBJECT_TYPE_WAYPOINT = 12;
const unsigned char CGameObject::OBJECT_TYPE_ENCOUNTER = 13;
const unsigned char CGameObject::OBJECT_TYPE_STORE = 14;
const unsigned char CGameObject::OBJECT_TYPE_SOUND = 16;
const unsigned long long CGameObjectArray::INVALIDOBJID = OBJECT_INVALID;
const unsigned long long CGameObjectArray::MAXCHAROBJID = MAX_CHARACTER_ID;
const unsigned int CGameObjectArray::LISTTYPE_MASK = ID_LIST_BIT;
const unsigned char CGameObjectArray::SUCCESS = 0;

CGameObject::CGameObject(unsigned char objectType, unsigned long long objectId) {
    m_objectType = objectType;
    m_objectId = objectId;
}

void CGameObject::SetId(unsigned long long) {}

void CGameObject::ResetUpdateTimes(unsigned int, unsigned int) {}

CSWCObject* CGameObject::AsSWCObject() { return 0; }

CSWSObject* CGameObject::AsSWSObject() { return 0; }

CSWCDoor* CGameObject::AsSWCDoor() { return 0; }

CSWSDoor* CGameObject::AsSWSDoor() { return 0; }

CSWCModule* CGameObject::AsSWCModule() { return 0; }

CSWSModule* CGameObject::AsSWSModule() { return 0; }

CSWCArea* CGameObject::AsSWCArea() { return 0; }

CSWSArea* CGameObject::AsSWSArea() { return 0; }

CSWCCreature* CGameObject::AsSWCCreature() { return 0; }

CSWSCreature* CGameObject::AsSWSCreature() { return 0; }

CSWCItem* CGameObject::AsSWCItem() { return 0; }

CSWSItem* CGameObject::AsSWSItem() { return 0; }

CSWCTrigger* CGameObject::AsSWCTrigger() { return 0; }

CSWSTrigger* CGameObject::AsSWSTrigger() { return 0; }

CSWCProjectile* CGameObject::AsSWCProjectile() { return 0; }

CSWSPlaceable* CGameObject::AsSWSPlaceable() { return 0; }

CSWCPlaceable* CGameObject::AsSWCPlaceable() { return 0; }

CSWSAreaOfEffectObject* CGameObject::AsSWSAreaOfEffectObject() { return 0; }

CSWCAreaOfEffectObject* CGameObject::AsSWCAreaOfEffectObject() { return 0; }

CSWSWaypoint* CGameObject::AsSWSWaypoint() { return 0; }

CSWSEncounter* CGameObject::AsSWSEncounter() { return 0; }

CSWCStore* CGameObject::AsSWCStore() { return 0; }

CSWSStore* CGameObject::AsSWSStore() { return 0; }

CSWSSoundObject* CGameObject::AsSWSSoundObject() { return 0; }

CSWCSoundObject* CGameObject::AsSWCSoundObject() { return 0; }

void CGameObjectArray::Clean(unsigned short)
{
}

unsigned char CGameObjectArray::GetGameObject(unsigned long long id, CGameObject** ppObject)
{
    CGameObjectArrayNode* pNode = m_paBuckets[(id >> 19 & BUCKETS_PER_LIST) | (id & BUCKET_MASK)];
    if (pNode != 0)
    {
        do
        {
            unsigned int nId = static_cast<unsigned int>(id) & ID_VALUE_MASK;
            if (pNode->m_id == nId)
            {
                *ppObject = pNode->m_pObject;
                return 0;
            }
            pNode = pNode->m_pNext;
        } while (pNode != 0);
    }
    *ppObject = 0;
    return 1;
}

unsigned char CGameObjectArray::Delete(unsigned long long id)
{
    unsigned long long idx = (id >> 19 & BUCKETS_PER_LIST) | (id & BUCKET_MASK);
    CGameObjectArrayNode** ppSlot = &m_paBuckets[idx];
    CGameObjectArrayNode* pNode = *ppSlot;
    if (pNode == 0)
        return 1;
    unsigned int nId = static_cast<unsigned int>(id) & ID_VALUE_MASK;
    if (pNode->m_id != nId)
    {
        do
        {
            ppSlot = &pNode->m_pNext;
            pNode = *ppSlot;
            if (pNode == 0)
                return 1;
        } while (pNode->m_id != nId);
    }
    *ppSlot = pNode->m_pNext;
    operator delete(pNode);
    return 0;
}

unsigned char CGameObjectArray::Delete(unsigned long long id, CGameObject** ppObject)
{
    unsigned long long idx = (id >> 19 & BUCKETS_PER_LIST) | (id & BUCKET_MASK);
    CGameObjectArrayNode* pNode = m_paBuckets[idx];
    if (pNode == 0)
        return 1;
    unsigned int nId = static_cast<unsigned int>(id) & ID_VALUE_MASK;
    CGameObjectArrayNode* pPrev;
    if (pNode->m_id == nId)
    {
        pPrev = 0;
    }
    else
    {
        do
        {
            pPrev = pNode;
            pNode = pPrev->m_pNext;
            if (pNode == 0)
                return 1;
        } while (pNode->m_id != nId);
    }
    *ppObject = pNode->m_pObject;
    CGameObjectArrayNode* pNext = pNode->m_pNext;
    if (pPrev == 0)
        m_paBuckets[idx] = pNext;
    else
        pPrev->m_pNext = pNext;
    operator delete(pNode);
    return 0;
}

CGameObjectArray::CGameObjectArray(int bLarge)
{
    m_bLarge = bLarge;
    if (bLarge)
    {
        m_paBuckets = new CGameObjectArrayNode*[2 * BUCKETS_PER_LIST];
        for (int i = 0; i < 2 * BUCKETS_PER_LIST; i++)
            m_paBuckets[i] = 0;
    }
    else
    {
        m_paBuckets = new CGameObjectArrayNode*[BUCKETS_PER_LIST];
        for (int i = 0; i < BUCKETS_PER_LIST; i++)
            m_paBuckets[i] = 0;
    }
    for (int i = 0; i < 2; i++)
    {
        m_nNextObjectId[i] = 0;
        m_nNextCharacterObjectId[i] = MAX_CHARACTER_ID;
    }
}

CGameObjectArray::~CGameObjectArray() noexcept(false)
{
    unsigned long long nBuckets = (m_bLarge == 0) ? BUCKETS_PER_LIST : 2 * BUCKETS_PER_LIST;
    for (unsigned long long i = 0; i < nBuckets; i++)
    {
        while (m_paBuckets[i] != 0)
        {
            CGameObjectArrayNode* pNode = m_paBuckets[i];
            if (pNode->m_pObject != 0)
                delete pNode->m_pObject;
            if (m_paBuckets[i] == pNode)
            {
                m_paBuckets[i] = pNode->m_pNext;
                delete pNode;
            }
        }
    }
    delete[] m_paBuckets;
    m_paBuckets = 0;
}

unsigned char CGameObjectArray::AddInternalObject(unsigned long long& id, CGameObject* pObject, int bChar)
{
    if (pObject == 0)
        return 4;

    unsigned int* pCounter;
    unsigned int nId;
    unsigned int nNewCounter;
    if (bChar == 0)
    {
        pCounter = &m_nNextObjectId[0];
        nId = *pCounter;
        nNewCounter = nId + 1;
        id = nId;
    }
    else
    {
        pCounter = &m_nNextCharacterObjectId[0];
        nNewCounter = *pCounter;
        nId = nNewCounter;
        id = nId;
        nNewCounter = nNewCounter - 1;
    }
    *pCounter = nNewCounter;
    unsigned int bucketIdx = nId & BUCKET_MASK;

    CGameObjectArrayNode* pNode = static_cast<CGameObjectArrayNode*>(::operator new(sizeof(CGameObjectArrayNode)));
    pNode->m_id = nId;
    pNode->m_pObject = pObject;
    pNode->m_pNext = 0;

    CGameObjectArrayNode** ppSlot = &m_paBuckets[bucketIdx];
    CGameObjectArrayNode* pHead = *ppSlot;
    if (pHead != 0)
    {
        CGameObjectArrayNode* pCur = pHead;
        if (pHead->m_id > nId)
        {
            CGameObjectArrayNode* pPrev;
            do
            {
                pPrev = pCur;
                pCur = pPrev->m_pNext;
                if (pCur == 0)
                {
                    ppSlot = &pPrev->m_pNext;
                    goto INSERT;
                }
            } while (pCur->m_id > nId);
            if (pPrev != 0)
            {
                ppSlot = &pPrev->m_pNext;
                pNode->m_pNext = *ppSlot;
                goto INSERT;
            }
        }
        pNode->m_pNext = pHead;
    }
INSERT:
    *ppSlot = pNode;
    return 0;
}

unsigned char CGameObjectArray::AddCharacterObjectAtPos(unsigned long long id, CGameObject* pObject)
{
    unsigned char status = 1;
    unsigned int nId;
    unsigned long long list;
    unsigned long long idx;
    unsigned int* pCounter;
    CGameObjectArrayNode* pNode;
    CGameObjectArrayNode** ppBucket;
    CGameObjectArrayNode* pHead;
    CGameObjectArrayNode* pCur;
    CGameObjectArrayNode* pPrev;

    if (__builtin_expect(pObject == 0, 0))
        goto RETURN_NULL;
    if (__builtin_expect((static_cast<unsigned int>(id) & OBJECT_INVALID) != 0, 0))
        goto END;
    nId = static_cast<unsigned int>(id) & ID_VALUE_MASK;
    if (__builtin_expect(nId <= OBJECT_INVALID, 0))
        goto END;

    list = (id >> 31) & 1;
    idx = (list << 12) | (id & BUCKET_MASK);
    pCounter = &m_nNextCharacterObjectId[list];
    if (nId <= *pCounter)
        *pCounter = nId - 1;

    pNode = static_cast<CGameObjectArrayNode*>(::operator new(sizeof(CGameObjectArrayNode)));
    pNode->m_id = nId;
    pNode->m_pObject = pObject;
    pNode->m_pNext = 0;

    ppBucket = &m_paBuckets[idx];
    pHead = *ppBucket;
    status = 0;
    if (pHead != 0)
    {
        if (pHead->m_id > nId)
        {
            pCur = pHead;
            do
            {
                pPrev = pCur;
                pCur = pPrev->m_pNext;
                if (pCur == 0)
                    goto INSERT_AFTER_PREV;
            } while (pCur->m_id > nId);
            if (pPrev != 0)
            {
INSERT_AFTER_PREV:
                pNode->m_pNext = pPrev->m_pNext;
                pPrev->m_pNext = pNode;
                goto END;
            }
        }
        pNode->m_pNext = pHead;
    }
    *ppBucket = pNode;
    goto END;
RETURN_NULL:
    status = 4;
END:
    return status;
}

unsigned char CGameObjectArray::AddExternalObject(unsigned long long& id, CGameObject* pObject, int bChar)
{
    K2_UNUSED(bChar);
    unsigned char status = 1;
    unsigned long long origId;
    unsigned int nId;
    CGameObjectArrayNode* pNode;
    CGameObjectArrayNode** ppBucket;
    CGameObjectArrayNode* pHead;
    CGameObjectArrayNode* pCur;
    CGameObjectArrayNode* pPrev;

    if (__builtin_expect(pObject == 0, 0))
        goto RETURN_NULL;

    origId = id;
    nId = static_cast<unsigned int>(origId) & ID_VALUE_MASK;
    if ((static_cast<unsigned int>(origId) & OBJECT_INVALID) == 0)
    {
        if (__builtin_expect(nId > OBJECT_INVALID - 1, 0))
            goto END;
        if (m_nNextObjectId[1] <= nId)
            m_nNextObjectId[1] = nId + 1;
    }
    else
    {
        if (__builtin_expect(nId < OBJECT_INVALID + 1, 0))
            goto END;
        if (m_nNextCharacterObjectId[1] >= nId)
            m_nNextCharacterObjectId[1] = nId - 1;
    }

    pNode = static_cast<CGameObjectArrayNode*>(::operator new(sizeof(CGameObjectArrayNode)));
    pNode->m_id = nId;
    pNode->m_pObject = pObject;
    pNode->m_pNext = 0;

    ppBucket = &m_paBuckets[BUCKETS_PER_LIST | (origId & BUCKET_MASK)];
    pHead = *ppBucket;
    status = 0;
    if (pHead != 0)
    {
        if (pHead->m_id > nId)
        {
            pCur = pHead;
            do
            {
                pPrev = pCur;
                pCur = pPrev->m_pNext;
                if (pCur == 0)
                    goto INSERT_AFTER_PREV;
            } while (pCur->m_id > nId);
            if (pPrev != 0)
            {
INSERT_AFTER_PREV:
                pNode->m_pNext = pPrev->m_pNext;
                pPrev->m_pNext = pNode;
                goto WRITEBACK;
            }
        }
        pNode->m_pNext = pHead;
    }
    *ppBucket = pNode;
WRITEBACK:
    id = origId | ID_LIST_BIT;
    goto END;
RETURN_NULL:
    status = 4;
END:
    return status;
}

// On an id collision the occupant is moved to a fresh id and the insert is retried.
unsigned char CGameObjectArray::AddObjectAtPos(unsigned long long id, CGameObject* pObject)
{
    unsigned char status;
#ifdef K2_FIX_ENGINE_BUGS
    status = 0;
#endif
    unsigned long long list;
    unsigned int nId;
    unsigned int nCounter;
    unsigned long long idx;
    CGameObjectArrayNode* pNode;
    CGameObjectArrayNode** ppBucket;
    CGameObjectArrayNode* pHead;
    unsigned int nCurId;
    CGameObjectArrayNode* pCur;
    CGameObjectArrayNode* pPrev;

    while (true)
    {
        if (__builtin_expect(pObject == 0, 0))
        {
            status = 4;
            goto END;
        }

        list = (id >> 31) & 1;
        nId = static_cast<unsigned int>(id) & ID_VALUE_MASK;
        if ((static_cast<unsigned int>(id) & OBJECT_INVALID) == 0)
        {
            if (__builtin_expect(nId > OBJECT_INVALID - 1, 0))
            {
                status = 1;
                goto END;
            }
            nCounter = m_nNextObjectId[list];
            if (nCounter <= nId)
                m_nNextObjectId[list] = nCounter = nId + 1;
        }
        else
        {
            if (__builtin_expect(nId < OBJECT_INVALID + 1, 0))
            {
                status = 1;
                goto END;
            }
            nCounter = m_nNextCharacterObjectId[list];
            if (nCounter >= nId)
                m_nNextCharacterObjectId[list] = nCounter = nId - 1;
        }

        idx = (list << 12) | (id & BUCKET_MASK);
        pNode = static_cast<CGameObjectArrayNode*>(::operator new(sizeof(CGameObjectArrayNode)));
        pNode->m_id = nId;
        pNode->m_pObject = pObject;
        pNode->m_pNext = 0;

        ppBucket = &m_paBuckets[idx];
        pHead = *ppBucket;
        if (pHead == 0)
        {
            *ppBucket = pNode;
            status = 0;
            goto END;
        }

        nCurId = pHead->m_id;
        pCur = pHead;
        if (nCurId > nId)
        {
            do
            {
                pPrev = pCur;
                pCur = pPrev->m_pNext;
                if (pCur == 0)
                    goto INSERT_AFTER_PREV;
                nCurId = pCur->m_id;
            } while (nCurId > nId);
        }
        else
        {
            pPrev = 0;
        }
        if (nCurId != nId)
        {
            status = 0;
            if (pPrev == 0)
            {
                pNode->m_pNext = pHead;
                *ppBucket = pNode;
                goto END;
            }
            pNode->m_pNext = pPrev->m_pNext;
INSERT_AFTER_PREV:
            pPrev->m_pNext = pNode;
            goto END;
        }

        pObject = pCur->m_pObject;
        operator delete(pNode);
        id = nCounter | (id & ID_LIST_BIT);
    }
END:
    return status;
}
