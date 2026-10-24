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
    int capacity = m_nCapacity;
    int count = m_nCount;
    int newCapacity = (count < capacity) ? capacity : capacity + 5;

    if (newCapacity > capacity)
    {
        CExoString** paNew = static_cast<CExoString**>(calloc(newCapacity, sizeof(CExoString*)));
        K2_ASSUME(paNew != 0);   // an allocation failure is not handled (the engine-wide policy)
        if (capacity < 1)
        {
            K2_ASSUME(m_nCount <= index);   // the count never passes the capacity: nothing to shift
            free(m_paStrings);
        }
        else
        {
            CExoString** paOld = m_paStrings;
            for (int i = 0; i < capacity; i++)
                paNew[i] = paOld[i];
            free(paOld);
        }
        count = m_nCount;
        m_paStrings = paNew;
    }

    if (index < count)
    {
        long long i = count;
        do
        {
            m_paStrings[i] = m_paStrings[i - 1];
            i--;
        } while (index < i);
    }

    m_paStrings[index] = pString;
    m_nCapacity = newCapacity;
    m_nCount = count + 1;
}

void CExoStringList::Insert(CExoString* pString, int index)
{
    if (m_bSorted == 0)
    {
        MessageBoxA(0, "Cannot call Insert directly on a sorted list.  Use Add instead.",
                    "Error: ExoStringList", 1);
        return;
    }

    int capacity = m_nCapacity;
    int count = m_nCount;
    int newCapacity = (count < capacity) ? capacity : capacity + 5;

    if (newCapacity > capacity)
    {
        CExoString** paNew = static_cast<CExoString**>(calloc(newCapacity, sizeof(CExoString*)));
        K2_ASSUME(paNew != 0);   // an allocation failure is not handled (the engine-wide policy)
        if (capacity < 1)
        {
            K2_ASSUME(m_nCount <= index);   // the count never passes the capacity: nothing to shift
            free(m_paStrings);
        }
        else
        {
            CExoString** paOld = m_paStrings;
            for (int i = 0; i < capacity; i++)
                paNew[i] = paOld[i];
            free(paOld);
        }
        count = m_nCount;
        m_paStrings = paNew;
    }

    if (index < count)
    {
        long long i = count;
        do
        {
            m_paStrings[i] = m_paStrings[i - 1];
            i--;
        } while (index < i);
    }

    m_paStrings[index] = pString;
    m_nCapacity = newCapacity;
    m_nCount = count + 1;
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

    int count = m_nCount;
    int capacity = m_nCapacity;
    int newCapacity = (count < capacity) ? capacity : capacity + 5;

    if (newCapacity > capacity)
    {
        CExoString** paNew = static_cast<CExoString**>(calloc(newCapacity, sizeof(CExoString*)));
        K2_ASSUME(paNew != 0);   // an allocation failure is not handled (the engine-wide policy)
        if (capacity < 1)
        {
            free(m_paStrings);
        }
        else
        {
            CExoString** paOld = m_paStrings;
            for (int i = 0; i < capacity; i++)
                paNew[i] = paOld[i];
            free(paOld);
        }
        count = m_nCount;
        m_paStrings = paNew;
    }

    for (int i = count; i > insertPos; i--)
        m_paStrings[i] = m_paStrings[i - 1];

    m_paStrings[insertPos] = pString;
    m_nCapacity = newCapacity;
    m_nCount = count + 1;
}

// Inserts sorted, with front and end fast paths, else a binary search, and growth by 5 when full.
// The duplicate policy is m_bDuplicate: 2 asserts, 1 rejects silently, 0 inserts alongside the
// existing duplicate.
void CExoStringList::AddSorted(CExoString* pString)
{
    int count = m_nCount;

    if (count >= 1)
    {
        CExoString* pFirst = m_paStrings[0];
        int cmp = strcmp(pString->CStr(), pFirst->CStr());

        if (cmp == 0)
        {
            if (m_bDuplicate == 2)
            {
                MessageBoxA(0, "Duplicate string detected.", "Error: ExoStringList", 1);
                return;
            }
            if (m_bDuplicate != 0)
                return;
            // fall through: insert anyway at position 0
        }

        if (cmp <= 0)
        {
            // insert at the front
            int capacity = m_nCapacity;
            int newCapacity = (count < capacity) ? capacity : capacity + 5;

            if (newCapacity > capacity)
            {
                CExoString** paNew = static_cast<CExoString**>(calloc(newCapacity, sizeof(CExoString*)));
                K2_ASSUME(paNew != 0);   // an allocation failure is not handled (the engine-wide policy)
                if (capacity < 1)
                {
                    K2_ASSUME(m_nCount == 0);   // the count never passes the capacity: an empty list
                    free(m_paStrings);
                }
                else
                {
                    CExoString** paOld = m_paStrings;
                    for (int i = 0; i < capacity; i++)
                        paNew[i] = paOld[i];
                    free(paOld);
                }
                count = m_nCount;
                m_paStrings = paNew;
            }

            if (count > 0)
            {
                long long i = count;
                do
                {
                    m_paStrings[i] = m_paStrings[i - 1];
                    i--;
                } while (i > 0);
            }

            m_paStrings[0] = pString;
            m_nCapacity = newCapacity;
            m_nCount = count + 1;
            return;
        }

        // cmp > 0: pString sorts after element 0
        int lastIdx = m_nCount - 1;
        if (m_nCount >= 1)
        {
            CExoString* pLast = m_paStrings[lastIdx];
            int cmpLast = strcmp(pString->CStr(), pLast->CStr());

            if (cmpLast == 0 && m_bDuplicate == 2)
            {
                MessageBoxA(0, "Duplicate string detected.", "Error: ExoStringList", 1);
                return;
            }
            if (cmpLast == 0 && m_bDuplicate != 0)
                return;

            if (cmpLast >= 0)
            {
                // append at the end (either pString sorts after the last element, or it equals
                // the last element and m_bDuplicate == 0 so it's inserted anyway)
                int capacity = m_nCapacity;
                count = m_nCount;
                int newCapacity = (count < capacity) ? capacity : capacity + 5;

                if (newCapacity > capacity)
                {
                    CExoString** paNew = static_cast<CExoString**>(calloc(newCapacity, sizeof(CExoString*)));
                    K2_ASSUME(paNew != 0);   // an allocation failure is not handled (the engine-wide policy)
                    if (capacity < 1)
                    {
                        K2_ASSUME(m_nCount == 0);   // the count never passes the capacity: an empty list
                        free(m_paStrings);
                    }
                    else
                    {
                        CExoString** paOld = m_paStrings;
                        for (int i = 0; i < capacity; i++)
                            paNew[i] = paOld[i];
                        free(paOld);
                    }
                    count = m_nCount;
                    m_paStrings = paNew;
                }

                m_paStrings[count] = pString;
                m_nCapacity = newCapacity;
                m_nCount = count + 1;
                return;
            }

            // strictly between element 0 and element lastIdx: binary search
            int lo = 0;
            int hi = lastIdx;
            while (hi - lo > 1)
            {
                int mid = (lo + hi) >> 1;
                CExoString* pMid = m_paStrings[mid];
                int cmpMid = strcmp(pString->CStr(), pMid->CStr());

                if (cmpMid < 0)
                {
                    hi = mid;
                }
                else if (cmpMid == 0)
                {
                    if (m_bDuplicate == 2)
                    {
                        MessageBoxA(0, "Duplicate string detected.", "Error: ExoStringList", 1);
                        return;
                    }
                    if (m_bDuplicate != 0)
                        return;

                    int capacity = m_nCapacity;
                    count = m_nCount;
                    int newCapacity = (count < capacity) ? capacity : capacity + 5;

                    if (newCapacity > capacity)
                    {
                        CExoString** paNew = static_cast<CExoString**>(calloc(newCapacity, sizeof(CExoString*)));
                        K2_ASSUME(paNew != 0);   // an allocation failure is not handled (the engine-wide policy)
                        if (capacity < 1)
                        {
                            K2_ASSUME(m_nCount == 0);   // the count never passes the capacity: an empty list
                            free(m_paStrings);
                        }
                        else
                        {
                            CExoString** paOld = m_paStrings;
                            for (int i = 0; i < capacity; i++)
                                paNew[i] = paOld[i];
                            free(paOld);
                        }
                        count = m_nCount;
                        m_paStrings = paNew;
                    }

                    if (mid < count)
                    {
                        long long i = count;
                        do
                        {
                            m_paStrings[i] = m_paStrings[i - 1];
                            i--;
                        } while (mid < i);
                    }

                    m_paStrings[mid] = pString;
                    m_nCapacity = newCapacity;
                    m_nCount = count + 1;
                    return;
                }
                else
                {
                    lo = mid;
                }
            }

            int insertPos = hi;
            int capacity = m_nCapacity;
            count = m_nCount;
            int newCapacity = (count < capacity) ? capacity : capacity + 5;

            if (newCapacity > capacity)
            {
                CExoString** paNew = static_cast<CExoString**>(calloc(newCapacity, sizeof(CExoString*)));
                K2_ASSUME(paNew != 0);   // an allocation failure is not handled (the engine-wide policy)
                if (capacity < 1)
                {
                    K2_ASSUME(m_nCount == 0);   // the count never passes the capacity: an empty list
                    free(m_paStrings);
                }
                else
                {
                    CExoString** paOld = m_paStrings;
                    for (int i = 0; i < capacity; i++)
                        paNew[i] = paOld[i];
                    free(paOld);
                }
                count = m_nCount;
                m_paStrings = paNew;
            }

            if (insertPos < count)
            {
                long long i = count;
                do
                {
                    m_paStrings[i] = m_paStrings[i - 1];
                    i--;
                } while (insertPos < i);
            }

            m_paStrings[insertPos] = pString;
            m_nCapacity = newCapacity;
            m_nCount = count + 1;
            return;
        }
    }

    // empty list (or the m_nCount<1 re-check above): insert at position 0
    int insertPos = m_nCount;
    if (insertPos < 0)
        insertPos = insertPos + 1;
    insertPos = insertPos >> 1;

    int capacity = m_nCapacity;
    int newCapacity = (count < capacity) ? capacity : capacity + 5;

    if (newCapacity > capacity)
    {
        CExoString** paNew = static_cast<CExoString**>(calloc(newCapacity, sizeof(CExoString*)));
        K2_ASSUME(paNew != 0);   // an allocation failure is not handled (the engine-wide policy)
        if (capacity < 1)
        {
            K2_ASSUME(m_nCount == 0 && insertPos == 0);   // the count never passes the capacity: an empty list
            free(m_paStrings);
        }
        else
        {
            CExoString** paOld = m_paStrings;
            for (int i = 0; i < capacity; i++)
                paNew[i] = paOld[i];
            free(paOld);
        }
        count = m_nCount;
        m_paStrings = paNew;
    }

    if (insertPos < count)
    {
        long long i = count;
        do
        {
            m_paStrings[i] = m_paStrings[i - 1];
            i--;
        } while (insertPos < i);
    }

    m_paStrings[insertPos] = pString;
    m_nCapacity = newCapacity;
    m_nCount = count + 1;
}
