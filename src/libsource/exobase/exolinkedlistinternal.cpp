#include "exolinkedlistinternal.h"
#include "exobase.h"

#if KOTOR1  // Source path the debug warnings below report
#define EXOLL_FILE const_cast<char*>("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/libsource/exobase/exolinkedlistinternal.cpp")
#else
#define EXOLL_FILE const_cast<char*>("C:/Users/Vu/Documents/AndroidStudioProjects/KOTOR1-2/KOTOR2_Android/app/src/main/cpp/kotor2/CODE/libsource/exobase/exolinkedlistinternal.cpp")
#endif

CExoLinkedListInternal::CExoLinkedListInternal()
{
    m_pHead = 0;
    m_pTail = 0;
    m_nCount = 0;
}

CExoLinkedListInternal::~CExoLinkedListInternal()
{
    CExoLinkedListNode* p = m_pHead;
    while (p != 0)
    {
        CExoLinkedListNode* pNext = p->m_pNext;
        operator delete(p);
        p = pNext;
    }
}

CExoLinkedListNode* CExoLinkedListInternal::AddHead(void* data)
{
    CExoLinkedListNode* pNode = static_cast<CExoLinkedListNode*>(::operator new(sizeof(CExoLinkedListNode)));
    CExoLinkedListNode* pOldHead = m_pHead;
    pNode->m_pPrev = 0;
    pNode->m_pData = 0;
    pNode->m_pNext = pOldHead;
    if (pOldHead != 0)
        pOldHead->m_pPrev = pNode;
    m_pHead = pNode;
    pNode->m_pData = data;
    if (m_pTail == 0)
        m_pTail = pNode;
    m_nCount++;
    return pNode;
}

CExoLinkedListNode* CExoLinkedListInternal::AddTail(void* data)
{
    CExoLinkedListNode* pNode = static_cast<CExoLinkedListNode*>(::operator new(sizeof(CExoLinkedListNode)));
    pNode->m_pNext = 0;
    pNode->m_pData = 0;
    CExoLinkedListNode* pOldTail = m_pTail;
    pNode->m_pPrev = pOldTail;
    if (pOldTail != 0)
        pOldTail->m_pNext = pNode;
    m_pTail = pNode;
    pNode->m_pData = data;
    if (m_pHead == 0)
        m_pHead = pNode;
    m_nCount++;
    return pNode;
}

CExoLinkedListNode* CExoLinkedListInternal::AddAfter(void* data, CExoLinkedListNode* pos)
{
    if (pos == 0)
    {
        g_pExoBase->m_pDebug->Warning(147, EXOLL_FILE, "CExoLinkedListInternal::AddAfter() Position parameter is NULL");
        return 0;
    }
    CExoLinkedListNode* pNode = static_cast<CExoLinkedListNode*>(::operator new(sizeof(CExoLinkedListNode)));
    pNode->m_pNext = 0;
    pNode->m_pData = 0;
    pNode->m_pPrev = 0;
    pNode->m_pNext = pos->m_pNext;
    if (pNode->m_pNext != 0)
        pNode->m_pNext->m_pPrev = pNode;
    pos->m_pNext = pNode;
    pNode->m_pPrev = pos;
    pNode->m_pData = data;
    if (m_pTail == pos)
        m_pTail = pNode;
    if (m_pHead == 0)
        m_pHead = pNode;
    m_nCount++;
    return pNode;
}

CExoLinkedListNode* CExoLinkedListInternal::AddBefore(void* data, CExoLinkedListNode* pos)
{
    if (pos == 0)
    {
        g_pExoBase->m_pDebug->Warning(191, EXOLL_FILE, "CExoLinkedListInternal::AddBefore() Position parameter is NULL");
        return 0;
    }
    CExoLinkedListNode* pNode = static_cast<CExoLinkedListNode*>(::operator new(sizeof(CExoLinkedListNode)));
    pNode->m_pNext = pos;
    pNode->m_pData = 0;
    CExoLinkedListNode* pOldPrev = pos->m_pPrev;
    pNode->m_pPrev = pOldPrev;
    if (pOldPrev != 0)
        pOldPrev->m_pNext = pNode;
    pos->m_pPrev = pNode;
    pNode->m_pData = data;
    if (m_pTail == 0)
        m_pTail = pNode;
    if (m_pHead == pos)
        m_pHead = pNode;
    m_nCount++;
    return pNode;
}

void* CExoLinkedListInternal::GetAtPos(CExoLinkedListNode* pos) const
{
    if (pos == 0)
    {
        g_pExoBase->m_pDebug->Warning(236, EXOLL_FILE, "CExoLinkedListInternal::GetAtPos() Position paramter is NULL");
        return 0;
    }
    return pos->m_pData;
}

void* CExoLinkedListInternal::GetNext(CExoLinkedListNode*& pos) const
{
    CExoLinkedListNode* pCur = pos;
    if (pCur == 0)
    {
        g_pExoBase->m_pDebug->Warning(255, EXOLL_FILE, "CExoLinkedListInternal::GetNext() Position paramter is NULL");
        return 0;
    }
    CExoLinkedListNode* pNext = pCur->m_pNext;
    if (pNext == 0)
    {
        pos = 0;
        return 0;
    }
    pos = pNext;
    return pNext->m_pData;
}

void* CExoLinkedListInternal::GetPrev(CExoLinkedListNode*& pos) const
{
    CExoLinkedListNode* pCur = pos;
    if (pCur == 0)
    {
        g_pExoBase->m_pDebug->Warning(281, EXOLL_FILE, "CExoLinkedListInternal::GetPreviousg() Position paramter is NULL");
        return 0;
    }
    CExoLinkedListNode* pPrev = pCur->m_pPrev;
    if (pPrev == 0)
    {
        pos = 0;
        return 0;
    }
    pos = pPrev;
    return pPrev->m_pData;
}

void* CExoLinkedListInternal::Remove(CExoLinkedListNode* pos)
{
    if (pos == 0)
    {
        g_pExoBase->m_pDebug->Warning(307, EXOLL_FILE, "CExoLinkedListInternal::Remove() Position paramter is NULL");
        return 0;
    }
    void* pData = pos->m_pData;
    CExoLinkedListNode* pNext = pos->m_pNext;
    CExoLinkedListNode* pPrev = pos->m_pPrev;
    if (pPrev != 0)
        pPrev->m_pNext = pNext;
    if (pNext != 0)
        pNext->m_pPrev = pPrev;
    if (m_pTail == pos)
        m_pTail = pos->m_pPrev;
    if (m_pHead == pos)
        m_pHead = pos->m_pNext;
    operator delete(pos);
    m_nCount--;
    return pData;
}

void* CExoLinkedListInternal::RemoveHead()
{
    CExoLinkedListNode* pHead = m_pHead;
    if (pHead == 0)
    {
        g_pExoBase->m_pDebug->Warning(357, EXOLL_FILE, "CExoLinkedListInternal::RemoveHead() Removing from empty List");
        return 0;
    }
    CExoLinkedListNode* pNext = pHead->m_pNext;
    void* pData = pHead->m_pData;
    m_pHead = pNext;
    if (pNext == 0)
        m_pTail = 0;
    else
        pNext->m_pPrev = 0;
    operator delete(pHead);
    m_nCount--;
    return pData;
}

void* CExoLinkedListInternal::RemoveTail()
{
    CExoLinkedListNode* pTail = m_pTail;
    if (pTail == 0)
    {
        g_pExoBase->m_pDebug->Warning(395, EXOLL_FILE, "CExoLinkedListInternal::RemoveTail() Removing from empty List");
        return 0;
    }
    CExoLinkedListNode* pPrev = pTail->m_pPrev;
    void* pData = pTail->m_pData;
    m_pTail = pPrev;
    if (pPrev == 0)
        m_pHead = 0;
    else
        pPrev->m_pNext = 0;
    operator delete(pTail);
    m_nCount--;
    return pData;
}

int CExoLinkedListInternal::Contains(void* data) const
{
    CExoLinkedListNode* p = m_pHead;
    if (p == 0)
        return 0;
    do
    {
        if (p->m_pData == data)
            return 1;
        p = p->m_pNext;
    } while (p != 0);
    return 0;
}
