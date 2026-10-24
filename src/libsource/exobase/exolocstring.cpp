#include "k2stdint.h"
#include "exolocstringinternal.h"
#include "strref.h"

const float CExoLocString::TIME_PER_CHARACTER_SPOKEN = 0.11f;

#include "port/libcstring.h"

CExoLocString::CExoLocString()
{
    m_pInternal = new CExoLocStringInternal();
    m_nStrRef = STRREF_NONE;
}

CExoLocString::CExoLocString(const CExoLocString& other)
{
    m_pInternal = new CExoLocStringInternal();
    if (this != &other)
    {
        m_pInternal->Assign(other.m_pInternal);
        m_nStrRef = other.m_nStrRef;
    }
}

// EFFECTIVE: scheduler tie-break on the frame-setup slot, not source-fixable.
CExoLocString::~CExoLocString()
{
    CExoLocStringInternal* pInternal = m_pInternal;
    if (pInternal != 0)
    {
        pInternal->~CExoLocStringInternal();
        operator delete(pInternal);
    }
}

CExoLocString& CExoLocString::operator=(const CExoLocString& other)
{
    if (this != &other)
    {
        m_pInternal->Assign(other.m_pInternal);
        m_nStrRef = other.m_nStrRef;
    }
    return *this;
}

// EFFECTIVE (K1): K1 tail-duplicates the two `return 0` blocks.
int CExoLocString::operator==(const CExoLocString& other) const
{
    CExoLocStringInternal* pThis = m_pInternal;
    CExoLocStringInternal* pOther = other.m_pInternal;
    if (pThis->m_bTlkFetched == 0)
    {
        if (pOther->m_bTlkFetched != 0)
        {
            if (pThis->m_nCount != 0)
                return 0;
        }
        else if (!pThis->Compare(other))
        {
            return 0;
        }
    }
    else if (pOther->m_bTlkFetched == 0)
    {
        if (pOther->m_nCount != 0)
            return 0;
    }
    return m_nStrRef == other.m_nStrRef;
}

// EFFECTIVE (K1): K1 tail-duplicates the two `return 0` blocks.
int CExoLocString::Compare(const CExoLocString& other) const
{
    CExoLocStringInternal* pThis = m_pInternal;
    CExoLocStringInternal* pOther = other.m_pInternal;
    if (pThis->m_bTlkFetched == 0)
    {
        if (pOther->m_bTlkFetched != 0)
        {
            if (pThis->m_nCount != 0)
                return 0;
        }
        else if (!pThis->Compare(other))
        {
            return 0;
        }
    }
    else if (pOther->m_bTlkFetched == 0)
    {
        if (pOther->m_nCount != 0)
            return 0;
    }
    return m_nStrRef == other.m_nStrRef;
}

int CExoLocString::operator!=(const CExoLocString& other) const
{
    CExoLocStringInternal* pThis = m_pInternal;
    CExoLocStringInternal* pOther = other.m_pInternal;
    int eq;
    if (pThis->m_bTlkFetched == 0)
    {
        if (pOther->m_bTlkFetched != 0)
        {
            if (pThis->m_nCount != 0)
                return 1;
            eq = m_nStrRef == other.m_nStrRef;
        }
        else if (!pThis->Compare(other))
        {
            return 1;
        }
        else
        {
            eq = m_nStrRef == other.m_nStrRef;
        }
    }
    else if (pOther->m_bTlkFetched == 0)
    {
        if (pOther->m_nCount != 0)
            return 1;
        eq = m_nStrRef == other.m_nStrRef;
    }
    else
    {
        eq = m_nStrRef == other.m_nStrRef;
    }
    return !eq;
}

int CExoLocString::AddString(int nLanguage, const CExoString& str, unsigned char gender)
{
    unsigned char g1 = (nLanguage == 0) ? 0 : gender;
    unsigned int g2 = (nLanguage == 0) ? 0 : static_cast<unsigned int>(g1 & 0xff);

    m_pInternal->RemoveString(g2 + nLanguage * 2);

    CExoString cur;
    GetString(nLanguage, &cur, g1);
    if (cur == str)
        return 1;

    return m_pInternal->AddString(nLanguage * 2 + (g1 & 0xff), str);
}

void CExoLocString::RemoveString(int nLanguage, unsigned char gender)
{
    unsigned int g = (nLanguage != 0) ? gender : 0;
    m_pInternal->RemoveString((g & 0xff) + nLanguage * 2);
}

int CExoLocString::GetString(int nLanguage, CExoString* pStr, unsigned char gender)
{
    unsigned char g = (nLanguage != 0) ? gender : 0;
    int combined = nLanguage * 2 + g;
    if (m_pInternal->GetString(combined, pStr))
        return 1;

    STR_RES res;
    if (g_pTlkTable == 0 || !g_pTlkTable->Fetch(static_cast<unsigned int>(m_nStrRef), res, g))
        return 0;

    *pStr = res.text;
    if (*pStr != "")
    {
        m_pInternal->AddString(combined, *pStr);
        m_pInternal->m_bTlkFetched = 1;
    }
    return 1;
}

void CExoLocString::ClearLocString()
{
    m_pInternal->ClearLocString();
}

int CExoLocString::GetString(unsigned int nIndex, int* pLanguage, CExoString* pStr, unsigned char* pGender) const
{
    int result = m_pInternal->GetString(nIndex, pLanguage, pStr);
    *pGender = static_cast<unsigned char>(*pLanguage % 2);
    *pLanguage = *pLanguage / 2;
    return result;
}

// EFFECTIVE: stack-slot tie-break on the STR_RES local, not source-fixable.
int CExoLocString::GetString(int nLanguage, CExoString* pStr, unsigned char gender) const
{
    unsigned char g = (nLanguage != 0) ? gender : 0;
    int combined = nLanguage * 2 + g;
    if (m_pInternal->GetString(combined, pStr))
        return 1;

    STR_RES res;
    if (g_pTlkTable == 0 || !g_pTlkTable->Fetch(static_cast<unsigned int>(m_nStrRef), res, g))
        return 0;

    *pStr = res.text;
    return 1;
}

void CExoLocString::GetStringInternal(int nID, CExoString* pStr) const
{
    m_pInternal->GetString(nID, pStr);
}

int CExoLocString::GetStringLoc(int nLanguage, CExoString* pStr, unsigned char gender) const
{
    unsigned int g = (nLanguage != 0) ? (gender & 0xff) : 0;
    return m_pInternal->GetString(g + nLanguage * 2, pStr);
}

int CExoLocString::GetStringCount() const
{
    return m_pInternal->m_nCount;
}

unsigned int CExoLocString::GetStringLength(unsigned int nIndex) const
{
    return m_pInternal->GetStringLength(nIndex);
}

void CExoLocString::RemoveString(int nID)
{
    m_pInternal->RemoveString(nID * 2);
    m_pInternal->RemoveString((nID != 0 ? 1 : 0) | (nID << 1));
}

// Estimated seconds to speak the string: markup tokens are parsed out, the ends trimmed, and the
// remaining length scaled by TIME_PER_CHARACTER_SPOKEN plus one second.
float CExoLocString::GetSpokenTimeEstimate(int nLanguage, unsigned char gender) const
{
    CExoString s;
    unsigned char g = (nLanguage != 0) ? (gender & 0xff) : 0;
    GetString(nLanguage * 2 + g, &s, 0);

#if KOTOR1  // K1 has no token pass and no blank trimming; the raw string's length is timed
    float estimate = 0.0f;
    if (s.GetLength() != 0)
        estimate = static_cast<float>(s.GetLength()) * TIME_PER_CHARACTER_SPOKEN + 1.0f;
    return estimate;
#else
    int nFoundLt = s.Find('<', 0);
    int nFoundBrace = s.Find('{', 0);
    if ((nFoundLt & nFoundBrace) != -1)
        g_pTlkTable->ParseStr(s);

    float estimate = 0.0f;
    while (s.m_sString != 0)
    {
        if (static_cast<int>(strlen(s.m_sString)) < 1)
            break;
        if (static_cast<unsigned char>(s[0]) != ' ')
            break;
        s = s.SubString(1, (s.m_sString ? static_cast<int>(strlen(s.m_sString)) : 0) - 1);
    }
    while (s.m_sString != 0 && static_cast<int>(strlen(s.m_sString)) >= 1 &&
           static_cast<unsigned char>(s[(s.m_sString ? static_cast<int>(strlen(s.m_sString)) : 0) - 1]) == ' ')
        s = s.SubString(0, (s.m_sString ? static_cast<int>(strlen(s.m_sString)) : 0) - 1);
    if (s.m_sString != 0)
    {
        if (static_cast<int>(strlen(s.m_sString)) != 0)
            estimate = static_cast<float>(s.m_sString ? static_cast<int>(strlen(s.m_sString)) : 0) *
                           TIME_PER_CHARACTER_SPOKEN + 1.0f;
    }
    return estimate;
#endif
}
