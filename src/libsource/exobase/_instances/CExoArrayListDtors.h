#pragma once
#include "k2stdint.h"

template <class T>
class CExoArrayList {  // idiom-exempt: canonical (home is exobase, alongside CExoString/CExoBase)
public:
    T*  m_pArray;
    int m_count;
    int m_capacity;

    CExoArrayList() { __builtin_memset(&m_pArray, 0, sizeof(*this)); }

    CExoArrayList(const CExoArrayList& other);

    ~CExoArrayList() { delete[] m_pArray; m_pArray = 0; }

    void Clear()
    {
        if (m_pArray)
        {
            delete[] m_pArray;
            m_pArray = 0;
            m_capacity = 0;
        }
        m_count = 0;
    }

    void Allocate(int n);
    CExoArrayList& operator=(const CExoArrayList& other);

    T& operator[](int i) { return m_pArray[i]; }

    void Insert(T v, int index);

    void Remove(T v) {
        int i = 0;
        while (i < m_count && m_pArray[i] != v)
            i++;
#if KOTOR1  // K1 has no found-check: removing an absent value drops the last element
        DelIndex(i);
#else
        if (i != m_count)
            DelIndex(i);
#endif
    }

    // Grows by doubling, starting at 16 elements.
    void Add(T v) {
        if (m_count == m_capacity) {
            Allocate(m_count != 0 ? m_count * 2 : 0x10);
        }
#if KOTOR1
        m_pArray[m_count++] = v;
#else
        int idx = m_count;
        m_count = idx + 1;
        m_pArray[idx] = v;
#endif
    }

    // Returns the number of matching elements, not a bool.
    int Contains(T v) const
    {
        int count = 0;
        for (int64_t i = 0; i < m_count; i++)
            if (m_pArray[i] == v)
                count++;
        return count;
    }

    // Like Contains, but compares the pointees and stops at the first match.
    template <class U>
    int DerefContains(U v) const
    {
        for (int64_t i = 0; i < m_count; i++)
            if (*m_pArray[i] == *v)
                return 1;
        return 0;
    }

    void AddUnique(T v);

    void DelIndex(int index) {
        m_count--;
        for (int i = index; i < m_count; i++)
            m_pArray[i] = m_pArray[i + 1];
    }

    void SetSize(int n) {
        if (n == 0) {
            if (m_pArray) {
                delete[] m_pArray;
                m_pArray = 0;
                m_capacity = 0;
            }
        } else {
            Allocate(n);
        }
        m_count = n;
    }
};
