#ifndef CEXOLINKEDLISTINTERNAL_H
#define CEXOLINKEDLISTINTERNAL_H

// Generic intrusive doubly-linked list backing CExoLinkedList<T>. The position-taking methods report
// a null node through CExoDebug::Warning, quoting this file and the caller's line number.

// Intrusive list node. POD: no ctor or dtor, allocated and freed by the list's own methods.
struct CExoLinkedListNode {  // idiom-exempt: canonical (home is the list class's own header)
    CExoLinkedListNode* m_pPrev;
    CExoLinkedListNode* m_pNext;
    void*                m_pData;
};

class CExoLinkedListInternal {
public:
    CExoLinkedListNode* m_pHead;
    CExoLinkedListNode* m_pTail;
    unsigned int         m_nCount;

    CExoLinkedListInternal();
    ~CExoLinkedListInternal();

    CExoLinkedListNode* AddHead(void* data);
    CExoLinkedListNode* AddTail(void* data);
    CExoLinkedListNode* AddAfter(void* data, CExoLinkedListNode* pos);
    CExoLinkedListNode* AddBefore(void* data, CExoLinkedListNode* pos);
    void*  GetAtPos(CExoLinkedListNode* pos) const;
    void*  GetNext(CExoLinkedListNode*& pos) const;
    void*  GetPrev(CExoLinkedListNode*& pos) const;
    void*  Remove(CExoLinkedListNode* pos);
    void*  RemoveHead();
    void*  RemoveTail();
    int    Contains(void* data) const;
};

#endif
