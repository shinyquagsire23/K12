#include "k2stdint.h"
#include "ObjectLookupTable.h"

#include "port/libcstring.h"

void CObjectLookupTable::Touch(unsigned int nIndex)
{
    unsigned long long nSaved = m_paIDs[nIndex];

    if (m_nCursorA > nIndex || m_nCursorA != m_nCursorB) {
        // Shifts the run back over the touched slot and appends it at the write end.
        memmove(&m_paIDs[nIndex], &m_paIDs[nIndex + 1],
                (m_nCursorB - nIndex - 1) * sizeof(unsigned long long));
        m_paIDs[m_nCursorB - 1] = nSaved;
        return;
    }

    // Shifts the span up by one and puts the touched slot at the read end; when the touched slot is
    // the read end there is nothing to move.
    if (m_nCursorA < nIndex) {
        memmove(&m_paIDs[m_nCursorA + 1], &m_paIDs[m_nCursorA],
                (nIndex - m_nCursorA) * sizeof(unsigned long long));
        m_paIDs[m_nCursorA] = nSaved;
    }

    m_nCursorA++;
    m_nCursorB++;
    if (m_nCursorA == RING_SIZE) {
        m_nCursorA = 0;
        m_nCursorB = 0;
    }
}

void* CObjectLookupTable::GetUpdate(void* pUnused, unsigned int* pnSize)
{
    K2_UNUSED(pUnused);
    char* pBuffer;

    if (m_nCursorA == m_nCursorB) {
        *pnSize = sizeof(CObjectLookupBlockHeader) + RING_SIZE * static_cast<unsigned int>(sizeof(unsigned long long));
#ifdef K2_FIX_ENGINE_BUGS
        return nullptr;
#else
        return pBuffer;
#endif
    }

    unsigned int nSize = (m_nCursorB << 3) | static_cast<unsigned int>(sizeof(CObjectLookupBlockHeader));
    *pnSize = nSize;
    pBuffer = new char[nSize];
    *reinterpret_cast<unsigned int*>(pBuffer) = m_nCursorA;
    return memcpy(pBuffer + sizeof(CObjectLookupBlockHeader), m_paIDs,
                  m_nCursorB * sizeof(unsigned long long)); // idiom-exempt: runtime length
}

void CObjectLookupTable::Update(void* pBuffer, unsigned int nSize)
{
    unsigned int nReadCursor = *static_cast<unsigned int*>(pBuffer);
    unsigned int nDataBytes = nSize - 4;
    m_nCursorA = nReadCursor;
#if KOTOR1
    // K1: the full-ring case is copied separately, as one fixed-size block.
    if (nDataBytes == RING_SIZE * static_cast<unsigned int>(sizeof(*m_paIDs))) {
        m_nCursorB = nReadCursor;
        __builtin_memcpy(m_paIDs, static_cast<char*>(pBuffer) + sizeof(CObjectLookupBlockHeader), RING_SIZE * sizeof(*m_paIDs));
    } else {
        m_nCursorB = nDataBytes / 8;
        memcpy(m_paIDs, static_cast<char*>(pBuffer) + sizeof(CObjectLookupBlockHeader), nDataBytes); // idiom-exempt: runtime length
    }
#else
    m_nCursorB = (nDataBytes == RING_SIZE * static_cast<unsigned int>(sizeof(*m_paIDs))) ? nReadCursor : nDataBytes / 8;
    memcpy(m_paIDs, static_cast<char*>(pBuffer) + sizeof(CObjectLookupBlockHeader), nDataBytes); // idiom-exempt: runtime length
#endif
    operator delete[](pBuffer);
}
