#include "k2stdint.h"
#include "exolocstringinternal.h"

#include "port/libcstring.h"

CExoLocStringInternal::CExoLocStringInternal()
{
    m_nCount = 0;
    m_bTlkFetched = 0;
}

CExoLocStringInternal::~CExoLocStringInternal()
{
    ClearLocString();
}

void CExoLocStringInternal::ClearLocString()
{
    CExoLinkedListInternal* pList = m_List.m_pInternal;
    while (pList->m_pHead != 0)
    {
        void* pData = pList->RemoveHead();
        if (pData != 0)
        {
            static_cast<ExoLocString_st*>(pData)->m_sString.~CExoString();
            operator delete(pData);
        }
        pList = m_List.m_pInternal;
    }
    m_nCount = 0;
}

int CExoLocStringInternal::AddString(int nID, const CExoString& str)
{
    CExoLinkedListNode* pos = m_List.m_pInternal->m_pHead;
    void* pData = m_List.m_pInternal->m_pHead ? m_List.m_pInternal->m_pHead->m_pData : 0;
    if (pData != 0)
    {
        ExoLocString_st* pNode = static_cast<ExoLocString_st*>(pData);
        while (true)
        {
            if (pNode->m_nID == nID)
            {
                pNode->m_sString = str;
                return 1;
            }
            pData = m_List.m_pInternal->GetNext(pos);
            if (pData == 0)
                break;
            pNode = static_cast<ExoLocString_st*>(pData);
        }
    }

    ExoLocString_st* pNew = new ExoLocString_st;
    pNew->m_nID = nID;
    pNew->m_sString = str;
    m_List.m_pInternal->AddTail(pNew);
    m_nCount++;
    return 1;
}

int CExoLocStringInternal::Assign(CExoLocStringInternal* pOther)
{
    CExoLinkedListInternal* pList = m_List.m_pInternal;
    while (pList->m_pHead != 0)
    {
        void* pData = pList->RemoveHead();
        if (pData != 0)
        {
            static_cast<ExoLocString_st*>(pData)->m_sString.~CExoString();
            operator delete(pData);
        }
        pList = m_List.m_pInternal;
    }
    m_nCount = 0;

    CExoLinkedListNode* otherPos = pOther->m_List.m_pInternal->m_pHead;
    void* pOtherData = pOther->m_List.m_pInternal->m_pHead ? pOther->m_List.m_pInternal->m_pHead->m_pData : 0;
    while (pOtherData != 0)
    {
        ExoLocString_st* pSrc = static_cast<ExoLocString_st*>(pOtherData);
        ExoLocString_st* pNew = new ExoLocString_st;
        pNew->m_nID = pSrc->m_nID;
        pNew->m_sString = pSrc->m_sString;
        m_nCount++;
        m_List.m_pInternal->AddTail(pNew);
        pOtherData = pOther->m_List.m_pInternal->GetNext(otherPos);
    }
    m_bTlkFetched = pOther->m_bTlkFetched;
    return 1;
}

int CExoLocStringInternal::Compare(const CExoLocString& other) const
{
    if (m_nCount != other.GetStringCount())
        return 0;

    CExoString* pA = new CExoString();
    CExoString* pB = new CExoString();

    int result = 1;
    void* pData = m_List.m_pInternal->m_pHead ? m_List.m_pInternal->m_pHead->m_pData : 0;
    CExoLinkedListNode* pos = m_List.m_pInternal->m_pHead;
    if (pData != 0)
    {
        ExoLocString_st* pNode = static_cast<ExoLocString_st*>(pData);
        while (true)
        {
            int nID = pNode->m_nID;
            *pA = pNode->m_sString;
            other.GetString(nID, pB, 0);
            if (*pA != *pB)
            {
                result = 0;
                break;
            }
            pData = m_List.m_pInternal->GetNext(pos);
            if (pData == 0)
                break;
            pNode = static_cast<ExoLocString_st*>(pData);
        }
    }

    pA->~CExoString();
    operator delete(pA);
    pB->~CExoString();
    operator delete(pB);
    return result;
}

int CExoLocStringInternal::GetString(unsigned int nIndex, int* pID, CExoString* pStr) const
{
    if (nIndex >= static_cast<unsigned int>(m_nCount))
        return 0;

    CExoLinkedListNode* pos = m_List.GetHeadPos();
    ExoLocString_st* pNode = m_List.GetHead();
    unsigned int i = 0;
    while (pNode != 0)
    {
        if (i == nIndex)
        {
            *pID = pNode->m_nID;
            *pStr = pNode->m_sString;
            return 1;
        }
        i++;
        pNode = m_List.GetNext(pos);
    }
    return 0;
}

int CExoLocStringInternal::GetString(int nID, CExoString* pStr) const
{
    CExoLinkedListNode* pos = m_List.m_pInternal->m_pHead;
    void* pData = m_List.m_pInternal->m_pHead ? m_List.m_pInternal->m_pHead->m_pData : 0;
    if (pData != 0)
    {
        ExoLocString_st* pNode = static_cast<ExoLocString_st*>(pData);
        while (true)
        {
            if (pNode->m_nID == nID)
            {
                *pStr = pNode->m_sString;
                return 1;
            }
            pData = m_List.m_pInternal->GetNext(pos);
            if (pData == 0)
                break;
            pNode = static_cast<ExoLocString_st*>(pData);
        }
    }
    return 0;
}

unsigned int CExoLocStringInternal::GetStringLength(unsigned int nIndex) const
{
    if (nIndex >= static_cast<unsigned int>(m_nCount))
        return 0;

    CExoLinkedListNode* pos = m_List.GetHeadPos();
    ExoLocString_st* pNode = m_List.GetHead();
    unsigned int i = 0;
    while (pNode != 0)
    {
        if (i == nIndex)
        {
#if KOTOR1  // K1 reads the length through CExoString::GetLength
            return pNode->m_sString.GetLength();
#else
            if (pNode->m_sString.m_sString == 0)
                return 0;
            return static_cast<unsigned int>(strlen(pNode->m_sString.m_sString));
#endif
        }
        i++;
        pNode = m_List.GetNext(pos);
    }
    return 0;
}

void CExoLocStringInternal::RemoveString(int nID)
{
    CExoLinkedListNode* pos = m_List.GetHeadPos();
    ExoLocString_st* pNode = m_List.GetHead();
    while (pNode != 0)
    {
        if (pNode->m_nID == nID)
        {
            m_List.Remove(pos);
            delete pNode;
            m_nCount--;
            return;
        }
        pNode = m_List.GetNext(pos);
    }
}
