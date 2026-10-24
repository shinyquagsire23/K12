#ifndef CEXOSTRINGLIST_H
#define CEXOSTRINGLIST_H

#include "exostring.h"

// A growable array of heap-allocated CExoString*, optionally kept sorted and optionally rejecting
// duplicates.
class CExoStringList {
public:
    CExoStringList();
    CExoStringList(int bSorted, int bDuplicate, int nCapacity);
    CExoStringList(CExoStringList* pOther);
    ~CExoStringList();

    int GetCount();
    int GetDuplicate();
    int GetSorted();
    int GetSize();
    CExoString* operator[](int index);

    CExoString* GetString(int index);
    void Clear();
    void Remove(int index);
    void Delete(CExoString* pString);

    void Add(CExoString* pString);
    void AddSorted(CExoString* pString);
    void Introduce(CExoString* pString, int index);
    void Insert(CExoString* pString, int index);
    CExoStringList* Merge(CExoStringList* pOther, int bSorted, int bDuplicate);
    CExoStringList& operator=(CExoStringList& other);

    CExoString** m_paStrings;
    int m_nCount;
    int m_bDuplicate;
    int m_bSorted;
    int m_nCapacity;
};

#endif
