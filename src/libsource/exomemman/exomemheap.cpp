#include "exomemheap.h"
#include <stddef.h>
#include <stdio.h>

CExoMemHeap* CExoMemHeap::m_lHeapsStart;

void CExoMemHeap::SetHeapName(const char* pszName)
{
    sprintf(m_szHeapName, "%s-%s", pszName, m_szNameSuffix);
}

void CExoMemHeap::RemoveHeap(CExoMemHeap* pHeap)
{
    if (m_lHeapsStart == pHeap)
    {
        m_lHeapsStart = pHeap->m_pNext;
        return;
    }

    CExoMemHeap* pPrev = m_lHeapsStart;
    while (pPrev->m_pNext != pHeap)
    {
        pPrev = pPrev->m_pNext;
    }
    pPrev->m_pNext = pHeap->m_pNext;
}

void CExoMemHeap::AddHeap(CExoMemHeap* pHeap)
{
    pHeap->m_pNext = NULL;

    if (m_lHeapsStart == NULL)
    {
        m_lHeapsStart = pHeap;
        return;
    }

    CExoMemHeap* pLast = m_lHeapsStart;
    while (pLast->m_pNext != NULL)
    {
        pLast = pLast->m_pNext;
    }
    pLast->m_pNext = pHeap;
}
