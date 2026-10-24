#include "exotlktable.h"
#ifndef CEXOLOCSTRINGINTERNAL_H
#define CEXOLOCSTRINGINTERNAL_H
#include "exostring.h"
#include "exolinkedlistinternal.h"
#include "CExoLinkedList.h"
#include "exolocstring.h"

// The pimpl half of CExoLocString, plus the talk-table records its GetString fallback needs.
//
// An entry is keyed by a combined id, language * 2 + gender, which every public wrapper packs before
// forwarding here.

// One heap-allocated entry in the list: a combined language/gender id and its text.
struct ExoLocString_st {
    int        m_nID;
    CExoString m_sString;
};

class CExoLocStringInternal {
public:
    CExoLinkedList<ExoLocString_st> m_List;
    int                     m_nCount;
    int                     m_bTlkFetched;

    CExoLocStringInternal();
    ~CExoLocStringInternal();

    void ClearLocString();
    int  AddString(int nID, const CExoString& str);
    int  Assign(CExoLocStringInternal* pOther);
    int  Compare(const CExoLocString& other) const;
    int  GetString(unsigned int nIndex, int* pID, CExoString* pStr) const;
    int  GetString(int nID, CExoString* pStr) const;
    unsigned int GetStringLength(unsigned int nIndex) const;
    void RemoveString(int nID);
};

#include "exoresman.h"

// A talk-table fetch record: the text, the sound resource and the sound's length.
struct STR_RES {
    CExoString text;
    CResRef    soundResRef;   // written only when the entry has a sound
    float      fSoundLength;  // written only when the entry has a sound length, else 0.0f
};

extern CTlkTable* g_pTlkTable;

#endif
