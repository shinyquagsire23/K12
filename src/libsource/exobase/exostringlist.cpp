#include "k2stdint.h"
#include "exostringlist.h"

#include "port/libcstring.h"
#include "port/libcstdlib.h"
#include "port/win32api.h"

CExoStringList::CExoStringList()
{
    m_paStrings = 0;
    m_nCount = 0;
    m_bDuplicate = 0;
    m_bSorted = 0;
    m_nCapacity = 0;
}

CExoStringList::CExoStringList(int bSorted, int bDuplicate, int nCapacity)
{
    m_paStrings = static_cast<CExoString**>(calloc(nCapacity, sizeof(CExoString*)));
    K2_ASSUME(m_paStrings != 0);   // an allocation failure is not handled (the engine-wide policy)
    m_nCount = 0;
    m_bDuplicate = bDuplicate;
    m_bSorted = bSorted;
    m_nCapacity = nCapacity;
}

CExoStringList::CExoStringList(CExoStringList* pOther)
{
    if (pOther == 0)
    {
        m_paStrings = 0;
        m_nCount = 0;
        m_bDuplicate = 0;
        m_bSorted = 0;
        m_nCapacity = 0;
        return;
    }

    int count = pOther->m_nCount;
    m_bDuplicate = pOther->m_bDuplicate;
    m_bSorted = pOther->m_bSorted;
    int nCapacity = pOther->m_nCapacity;
    m_nCapacity = nCapacity;
    m_paStrings = static_cast<CExoString**>(calloc(nCapacity, sizeof(CExoString*)));
    K2_ASSUME(m_paStrings != 0);   // an allocation failure is not handled (the engine-wide policy)
    m_nCount = count;

    for (int i = 0; i < count; i++)
    {
        CExoString* pNew = new CExoString();
        *pNew = *pOther->m_paStrings[i];
        m_paStrings[i] = pNew;
    }
}

CExoStringList::~CExoStringList()
{
    if (m_paStrings == 0)
        return;

    for (int i = 0; i < m_nCount; i++)
    {
        CExoString* p = m_paStrings[i];
        if (p != 0)
        {
            p->~CExoString();
            operator delete(p);
        }
    }
    free(m_paStrings);
}

int CExoStringList::GetCount()
{
    return m_nCount;
}

int CExoStringList::GetDuplicate()
{
    return m_bDuplicate;
}

int CExoStringList::GetSorted()
{
    return m_bSorted;
}

int CExoStringList::GetSize()
{
    return m_nCapacity;
}

CExoString* CExoStringList::operator[](int index)
{
    return m_paStrings[index];
}

CExoString* CExoStringList::GetString(int index)
{
    return m_paStrings[index];
}

void CExoStringList::Clear()
{
    for (int i = 0; i < m_nCapacity; i++)
    {
        CExoString* p = m_paStrings[i];
        if (p != 0)
        {
            p->~CExoString();
            operator delete(p);
            m_paStrings[i] = 0;
        }
    }
    m_nCount = 0;
}

void CExoStringList::Remove(int index)
{
    CExoString* p = m_paStrings[index];
    if (p != 0)
    {
        p->~CExoString();
        operator delete(p);
    }

    for (int i = index; i < m_nCount - 1; i++)
        m_paStrings[i] = m_paStrings[i + 1];

    m_nCount = m_nCount - 1;
}

void CExoStringList::Delete(CExoString* pString)
{
    int capacity = m_nCapacity;
    int count = m_nCount;
    CExoString** paNew = static_cast<CExoString**>(calloc(capacity, sizeof(CExoString*)));
    K2_ASSUME(paNew != 0);   // an allocation failure is not handled (the engine-wide policy)
    if (paNew != 0)
    {
        if (count >= 1)
        {
            int i = 0;
            do
            {
                CExoString* p = m_paStrings[i];
                if (p != 0 && strcmp(pString->CStr(), p->CStr()) == 0)
                {
                    p->~CExoString();
                    operator delete(p);
                    count--;
                    m_paStrings[i] = 0;
                }
                i++;
            } while (i < m_nCount);
        }

        capacity = m_nCapacity;
        if (capacity >= 1)
        {
            int writeIdx = 0;
            for (int readIdx = 0; readIdx < capacity; readIdx++)
            {
                CExoString* p = m_paStrings[readIdx];
                if (p != 0)
                    paNew[writeIdx++] = p;
            }
        }
    }

    free(m_paStrings);
    m_nCount = count;
    m_paStrings = paNew;
}

void CExoStringList::Introduce(CExoString* pString, int index)
{
    int nOldCapacity = m_nCapacity;
    int nNewCapacity = nOldCapacity;
    if (m_nCount >= nOldCapacity)
        nNewCapacity = nOldCapacity + 5;

    if (nNewCapacity > nOldCapacity)
    {
        CExoString** paNew = static_cast<CExoString**>(calloc(nNewCapacity, sizeof(CExoString*)));
        for (int i = 0; i < nOldCapacity; i++)
            paNew[i] = m_paStrings[i];
        free(m_paStrings);
        m_paStrings = paNew;
    }

    for (int i = m_nCount; i > index; i--)
        m_paStrings[i] = m_paStrings[i - 1];

    m_paStrings[index] = pString;
    m_nCapacity = nNewCapacity;
    m_nCount++;
}

void CExoStringList::Insert(CExoString* pString, int index)
{
    if (m_bSorted == 0)
    {
        MessageBoxA(0, "Cannot call Insert directly on a sorted list.  Use Add instead.",
                    "Error: ExoStringList", 1);
        return;
    }

    Introduce(pString, index);
}

CExoStringList* CExoStringList::Merge(CExoStringList* pOther, int bSorted, int bDuplicate)
{
    CExoStringList* pMerged = new CExoStringList(bSorted, bDuplicate, m_nCapacity);

    for (int i = 0; i < m_nCount; i++)
        pMerged->Add(new CExoString(m_paStrings[i]->CStr()));

    if (pOther != 0)
    {
        for (int i = 0; i < pOther->m_nCount; i++)
            pMerged->Add(new CExoString(pOther->m_paStrings[i]->CStr()));
    }

    return pMerged;
}

CExoStringList& CExoStringList::operator=(CExoStringList& other)
{
    const void* pSelf = this;
    if (pSelf != 0)
    {
        for (int i = 0; i < m_nCount; i++)
        {
            CExoString* p = m_paStrings[i];
            if (p != 0)
            {
                p->~CExoString();
                operator delete(p);
            }
        }
        free(m_paStrings);

        int count = other.m_nCount;
        m_nCount = count;
        m_bDuplicate = other.m_bDuplicate;
        m_bSorted = other.m_bSorted;
        int capacity = other.m_nCapacity;
        m_nCapacity = capacity;
        m_paStrings = static_cast<CExoString**>(calloc(capacity, sizeof(CExoString*)));
        K2_ASSUME(m_paStrings != 0);   // an allocation failure is not handled (the engine-wide policy)

        if (count >= 1)
        {
            int i = 0;
            do
            {
                CExoString* pSrc = other.m_paStrings[i];
                m_paStrings[i] = new CExoString(pSrc->CStr());
                i++;
            } while (i < m_nCount);
        }
    }

    return *this;
}

// The flag is inverted. With m_bSorted clear, Add forwards to AddSorted; with it set, Add does a
// plain unsorted append.
void CExoStringList::Add(CExoString* pString)
{
    if (m_bSorted == 0)
    {
        AddSorted(pString);
        return;
    }

    if (m_bDuplicate == 2)
    {
        MessageBoxA(0, "Duplicate string found.", "Error: ExoStringList", 1);
        return;
    }

    int insertPos = m_nCount;
    if (m_bDuplicate == 1)
    {
        int bAdd = 1;
        for (int i = 0; i < m_nCount; i++)
        {
            if (strcmp(pString->CStr(), m_paStrings[i]->CStr()) == 0)
            {
                bAdd = 0;
                i = m_nCount;
            }
        }
        if (!bAdd)
            return;
    }

    Introduce(pString, insertPos);
}

// Inserts sorted, with front and end fast paths, else a binary search, and growth by 5 when full.
// The duplicate policy is m_bDuplicate: 2 asserts, 1 rejects silently, 0 inserts alongside the
// existing duplicate.
void CExoStringList::AddSorted(CExoString* pString)
{
    int bDone = 0;
    int nCmp = 0;
    int nHi = 0;

    if (m_nCount >= 1)
    {
        nCmp = strcmp(pString->CStr(), m_paStrings[0]->CStr());
        if (nCmp < 0)
        {
            Introduce(pString, 0);
            bDone = 1;
        }
        else if (nCmp == 0)
        {
            if (m_bDuplicate == 2)
                MessageBoxA(0, "Duplicate string detected.", "Error: ExoStringList", 1);
            else if (m_bDuplicate == 0)
                Introduce(pString, 0);
            bDone = 1;
        }
        else if (m_nCount >= 1)
        {
            nHi = m_nCount - 1;
            nCmp = strcmp(pString->CStr(), m_paStrings[nHi]->CStr());
            if (nCmp > 0)
            {
                Introduce(pString, m_nCount);
                bDone = 1;
            }
            else if (nCmp == 0)
            {
                if (m_bDuplicate == 2)
                    MessageBoxA(0, "Duplicate string detected.", "Error: ExoStringList", 1);
                else if (m_bDuplicate == 0)
                    Introduce(pString, m_nCount);
                bDone = 1;
            }
        }
    }

    // Binary search between the two ends: the probe moves by half the remaining span, towards the
    // side the comparison picked.
    int nLo = 0;
    int nMid = m_nCount / 2;
    int nStep = nMid;
    while (nHi - nLo > 1 && !bDone)
    {
        nCmp = strcmp(pString->CStr(), m_paStrings[nMid]->CStr());
        if (nCmp < 0)
        {
            nStep = -1;
            nHi = nMid;
        }
        else if (nCmp > 0)
        {
            nStep = 1;
            nLo = nMid;
        }
        else
        {
            if (m_bDuplicate == 2)
                MessageBoxA(0, "Duplicate string detected.", "Error: ExoStringList", 1);
            else if (m_bDuplicate == 0)
                Introduce(pString, nMid);
            bDone = 1;
        }
        nStep *= (nHi - nLo) / 2;
        nMid += nStep;
    }

    if (bDone)
        return;
    if (nCmp > 0)
        nMid++;
    Introduce(pString, nMid);
}
