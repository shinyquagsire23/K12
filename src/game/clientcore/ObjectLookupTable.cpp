#include "k2stdint.h"
#include "ObjectLookupTable.h"
#include "gameobjectarray.h"

#include "port/libcstring.h"

CObjectLookupTable::CObjectLookupTable()
{
    m_paIDs = new unsigned long long[RING_SIZE];
    for (int i = 0; i < RING_SIZE; i++)
        m_paIDs[i] = OBJECT_INVALID;
    m_nReserved1 = 0;
    m_nCursorA = static_cast<unsigned int>(-1);
    m_nCursorB = 0;
}

CObjectLookupTable::~CObjectLookupTable()
{
    delete[] m_paIDs;
}

int CObjectLookupTable::AddID(unsigned long long nID)
{
    unsigned int nSlot = m_nCursorB;

    if (m_nCursorA == static_cast<unsigned int>(-1) || m_nCursorA == nSlot) {
        m_nCursorA++;
        if (m_nCursorA == RING_SIZE)
            m_nCursorA = 0;
    }

    m_paIDs[nSlot] = nID;
    m_nCursorB++;
    if (m_nCursorB == RING_SIZE)
        m_nCursorB = 0;
    return nSlot;
}

unsigned long long CObjectLookupTable::GetIDFromIndex(unsigned int nIndex)
{
    return m_paIDs[nIndex];
}

// Scans from the read cursor to the write cursor, wrapping at the end of the ring; a ring holding a
// single id still compares it. Returns -1 when the id is absent.
int CObjectLookupTable::GetIndexFromID(unsigned long long nID)
{
    unsigned int nIndex = m_nCursorA;
    if (nIndex == static_cast<unsigned int>(-1))
        return -1;

    do {
        if (m_paIDs[nIndex] == nID)
            return nIndex;
        if (++nIndex == RING_SIZE)
            nIndex = 0;
    } while (nIndex != m_nCursorB);
    return -1;
}

CObjectTableManager::CObjectTableManager()
    : m_slots()
    , m_nCount(0)
{
}

CObjectTableManager::~CObjectTableManager()
{
    ClearAll();
}

void CObjectTableManager::ClearAll()
{
    for (uint64_t i = 0; i < m_nCount; i++)
    {
        CObjectLookupTable* pTable = m_slots.m_pSlots[i];
        if (pTable)
        {
            delete pTable;
            m_slots.m_pSlots[i] = 0;
        }
    }
}

bool CObjectTableManager::RemovePlayer(unsigned int playerId)
{
    if (playerId > CObjectTableSlots::MAX_PLAYERS - 1)
        return false;

    CObjectLookupTable* pTable = m_slots.m_pSlots[playerId];
    if (pTable == 0)
        return false;

    delete pTable;
    m_slots.m_pSlots[playerId] = 0;
    return true;
}

int CObjectTableManager::CreateNewPlayer(unsigned int playerId)
{
    if (playerId > CObjectTableSlots::MAX_PLAYERS - 1)
        return 0;
    if (m_slots.m_pSlots[playerId] != 0)
        return 0;

    CObjectLookupTable* pTable = new CObjectLookupTable;
    m_slots.m_pSlots[playerId] = pTable;

    if (m_nCount <= playerId)
        m_nCount = playerId + 1;
    return 1;
}

bool CObjectTableManager::Touch(unsigned int nIndex, unsigned int playerId)
{
    if (playerId > CObjectTableSlots::MAX_PLAYERS - 1)
        return false;

    CObjectLookupTable* pTable = m_slots.m_pSlots[playerId];
    if (pTable == 0)
        return false;

    pTable->Touch(nIndex);
    return true;
}

// *pIndex is set before the bounds check.
bool CObjectTableManager::AddID(unsigned long long nID, unsigned int* pIndex, unsigned int playerId)
{
    *pIndex = INDEX_NONE;

    if (playerId > CObjectTableSlots::MAX_PLAYERS - 1)
        return false;

    CObjectLookupTable* pTable = m_slots.m_pSlots[playerId];
    if (pTable == 0) {
        if (!CreateNewPlayer(playerId))
            return false;
        pTable = m_slots.m_pSlots[playerId];
    }

    *pIndex = pTable->AddID(nID);
    return true;
}

unsigned long long CObjectTableManager::GetIDFromIndex(unsigned int nIndex, unsigned int playerId)
{
    if (playerId == static_cast<unsigned int>(-1))
        return OBJECT_INVALID;

#if KOTOR1
    // K1 calls CreateNewPlayer unconditionally (its own null check decides).
    CreateNewPlayer(playerId);
    return m_slots.m_pSlots[playerId]->GetIDFromIndex(nIndex);
#else
    CObjectLookupTable* pTable = m_slots.m_pSlots[playerId];
    if (pTable == 0) {
        CreateNewPlayer(playerId);
        pTable = m_slots.m_pSlots[playerId];
    }
    return pTable->GetIDFromIndex(nIndex);
#endif
}

int CObjectTableManager::GetIndexFromID(unsigned long long nID, unsigned int playerId)
{
    if (playerId == static_cast<unsigned int>(-1))
        return -1;

#if KOTOR1
    // K1 calls CreateNewPlayer unconditionally (its own null check decides).
    CreateNewPlayer(playerId);
    return m_slots.m_pSlots[playerId]->GetIndexFromID(nID);
#else
    CObjectLookupTable* pTable = m_slots.m_pSlots[playerId];
    if (pTable == 0) {
        CreateNewPlayer(playerId);
        pTable = m_slots.m_pSlots[playerId];
    }
    return pTable->GetIndexFromID(nID);
#endif
}
