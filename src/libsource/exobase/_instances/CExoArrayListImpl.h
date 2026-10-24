#pragma once
#include "k2stdint.h"

#include "CExoArrayListDtors.h"

// Out-of-line bodies for CExoArrayList<T>. Only the instantiation TUs include this header; ordinary
// consumers include CExoArrayListDtors.h, which declares these without bodies, so their call sites
// stay out of line.

// Grow the backing store to capacity n, preserving the live elements.
template <class T>
void CExoArrayList<T>::Allocate(int n)
{
    T* pOld = m_pArray;
    m_capacity = n;
    m_pArray = new T[n];
#ifdef K2_FIX_ENGINE_BUGS
    // OOB fix: a shrink used to copy m_count elements into the smaller buffer.
    int nKeep = m_count < n ? m_count : n;
    for (int i = 0; i < nKeep; i++)
#else
    for (int i = 0; i < m_count; i++)
#endif
        m_pArray[i] = pOld[i];
    delete[] pOld;
}

template <class T>
CExoArrayList<T>::CExoArrayList(const CExoArrayList<T>& other)
    : m_pArray(0), m_count(0), m_capacity(0)
{
    for (int64_t i = 0; i < other.m_count; i++)
        Add(other.m_pArray[i]);
}

// Keeps the existing buffer, so capacity survives a reassignment.
template <class T>
CExoArrayList<T>& CExoArrayList<T>::operator=(const CExoArrayList<T>& other)
{
    m_count = 0;
    for (int64_t i = 0; i < other.m_count; i++)
        Add(other.m_pArray[i]);
    return *this;
}

template <class T>
void CExoArrayList<T>::Insert(T v, int index)
{
#if KOTOR1  // K1 shifts with a plain descending loop, K2 with an explicit guarded do-while
    int i = m_count;
    Add(v);
    for (; i > index; i--)
        m_pArray[i] = m_pArray[i - 1];
#else
    int64_t i = m_count;
    Add(v);
    if (static_cast<int>(i) > index) {
        do {
            m_pArray[i] = m_pArray[i - 1];
            i--;
        } while (i > index);
    }
#endif
    m_pArray[index] = v;
}

// AddUnique is omitted from this header when CEXO_ARRAYLIST_KEEP_ADDUNIQUE_OUT_OF_LINE is defined,
// leaving the member out of line for the TUs that instantiate it.
#ifndef CEXO_ARRAYLIST_KEEP_ADDUNIQUE_OUT_OF_LINE
template <class T>
void CExoArrayList<T>::AddUnique(T v)
{
    if (Contains(v) == 0)
        Add(v);
}
#endif
