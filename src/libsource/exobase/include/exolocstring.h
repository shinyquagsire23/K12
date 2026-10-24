#ifndef CEXOLOCSTRING_H
#define CEXOLOCSTRING_H

// A localized string: either a talk-table row or its own per-language text. Language 0 has no
// gender variant; the gender bit is forced to 0 before the language and gender are combined.
class CExoString;
class CExoLocStringInternal;

class CExoLocString {
public:
    CExoLocStringInternal* m_pInternal;

    // The talk-table row, or the all-ones sentinel when the string carries its own text.
    unsigned long long      m_nStrRef;

    static const float TIME_PER_CHARACTER_SPOKEN;

    CExoLocString();
    CExoLocString(const CExoLocString& other);
    ~CExoLocString();

    CExoLocString& operator=(const CExoLocString& other);
    int operator==(const CExoLocString& other) const;
    int Compare(const CExoLocString& other) const;
    int operator!=(const CExoLocString& other) const;

    int  AddString(int nLanguage, const CExoString& str, unsigned char gender);
    void RemoveString(int nLanguage, unsigned char gender);
    int  GetString(int nLanguage, CExoString* pStr, unsigned char gender);
    void ClearLocString();
    int  GetString(unsigned int nIndex, int* pLanguage, CExoString* pStr, unsigned char* pGender) const;
    // Kept out of line on purpose: GetSpokenTimeEstimate calls it from this same translation unit.
    __attribute__((noinline))
    int  GetString(int nLanguage, CExoString* pStr, unsigned char gender) const;
    void GetStringInternal(int nID, CExoString* pStr) const;
    // Returns int: callers such as HasValidString test the result.
    int  GetStringLoc(int nLanguage, CExoString* pStr, unsigned char gender) const;
    int  GetStringCount() const;
    unsigned int GetStringLength(unsigned int nIndex) const;
    void RemoveString(int nID);
    float GetSpokenTimeEstimate(int nLanguage, unsigned char gender) const;
};

#endif
