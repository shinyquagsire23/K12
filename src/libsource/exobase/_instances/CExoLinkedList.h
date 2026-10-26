#ifndef CEXOLINKEDLIST_H
#define CEXOLINKEDLIST_H

#include "exolinkedlistinternal.h"

// The typed wrapper over CExoLinkedListInternal, a one-member handle that owns its backing list.
template <class T>
class CExoLinkedList {
public:
    CExoLinkedList() : m_pInternal(new CExoLinkedListInternal()) {}
    ~CExoLinkedList();

    CExoLinkedListNode* GetHeadPos() const { return m_pInternal->m_pHead; }
    CExoLinkedListNode* GetTailPos() const { return m_pInternal->m_pTail; }
    T* GetHead() const { return m_pInternal->m_pHead ? static_cast<T*>(m_pInternal->m_pHead->m_pData) : 0; }
    T* GetAtPos(CExoLinkedListNode* pos) const { return static_cast<T*>(m_pInternal->GetAtPos(pos)); }
    T* GetNext(CExoLinkedListNode*& pos) const { return static_cast<T*>(m_pInternal->GetNext(pos)); }
    T* GetPrev(CExoLinkedListNode*& pos) const { return static_cast<T*>(m_pInternal->GetPrev(pos)); }
    CExoLinkedListNode* AddHead(T* pData) { return m_pInternal->AddHead(pData); }
    CExoLinkedListNode* AddTail(T* pData) { return m_pInternal->AddTail(pData); }
    CExoLinkedListNode* AddAfter(T* pData, CExoLinkedListNode* pos) { return m_pInternal->AddAfter(pData, pos); }
    CExoLinkedListNode* AddBefore(T* pData, CExoLinkedListNode* pos) { return m_pInternal->AddBefore(pData, pos); }
    T* Remove(CExoLinkedListNode* pos) { return static_cast<T*>(m_pInternal->Remove(pos)); }
    T* RemoveHead() { return static_cast<T*>(m_pInternal->RemoveHead()); }
    T* RemoveTail() { return static_cast<T*>(m_pInternal->RemoveTail()); }
    int Contains(T* pData) const { return m_pInternal->Contains(pData); }
    unsigned int Count() const { return m_pInternal->m_nCount; }

    CExoLinkedListInternal* m_pInternal;
};

template <class T>
CExoLinkedList<T>::~CExoLinkedList()
{
    // Cache the pointer: the destructor call may clobber the member.
    CExoLinkedListInternal* pInternal = m_pInternal;
    if (pInternal) {
        pInternal->~CExoLinkedListInternal();
        operator delete(pInternal);
    }
}

#endif
