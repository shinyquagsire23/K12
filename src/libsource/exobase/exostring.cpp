#include "k2stdint.h"
#include "exostring.h"

// The step Format grows its shared buffer by until the text fits.
enum {
    FORMAT_BUFFER_STEP = 1024,
};

#include <stddef.h>
#include <stdio.h>

#include "port/crtapi.h"
#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcstdlib.h"
#include "exostringinline.h"
extern "C" {
}

CExoString::CExoString()
{
    m_sString = NULL;
    m_nBufferLength = 0;
}

CExoString::CExoString(const char* str)
{
    if (str != NULL && strlen(str) != 0) {
        m_nBufferLength = static_cast<unsigned int>(strlen(str) + 1);
        AllocString();
        strcpy(m_sString, str);
    } else {
        m_sString = NULL;
        m_nBufferLength = 0;
    }
}

CExoString::CExoString(const char* str, int length)
{
    if (length >= 1) {
        m_nBufferLength = length + 1;
        AllocString();
        strncpy(m_sString, str, length);
        m_sString[length] = '\0';
    } else {
        m_sString = NULL;
        m_nBufferLength = 0;
    }
}

int CExoString::operator==(const CExoString& other) const
{
    if (m_sString == NULL) {
        if (other.m_sString == NULL)
            return 1;
        return *other.m_sString == '\0';
    }
    if (other.m_sString == NULL)
        return *m_sString == '\0';
    return strcmp(m_sString, other.m_sString) == 0;
}

int CExoString::operator!=(const CExoString& other) const
{
    if (m_sString == NULL) {
        if (other.m_sString == NULL)
            return 0;
        return *other.m_sString != '\0';
    }
    if (other.m_sString == NULL)
        return *m_sString != '\0';
    return strcmp(m_sString, other.m_sString) != 0;
}

int CExoString::operator<(const CExoString& other) const
{
    if (m_sString == NULL) {
        if (other.m_sString == NULL)
            return 0;
        return *other.m_sString != '\0';
    }
    if (other.m_sString == NULL)
        return 0;
    return strcmp(m_sString, other.m_sString) < 0;
}

int CExoString::operator>(const CExoString& other) const
{
    if (m_sString == NULL)
        return 0;
    if (other.m_sString == NULL)
        return *m_sString != '\0';
    return strcmp(m_sString, other.m_sString) > 0;
}

int CExoString::operator<=(const CExoString& other) const
{
    if (m_sString != 0 && other.m_sString != 0)
        return strcmp(m_sString, other.m_sString) <= 0;
    if (m_sString == 0)
    {
        if (other.m_sString == 0)
            return 1;
    }
    else if (*m_sString == 0)
        return 1;
    return other.m_sString != 0;
}

int CExoString::operator>=(const CExoString& other) const
{
    if (m_sString != 0 && other.m_sString != 0)
        return strcmp(m_sString, other.m_sString) >= 0;
    if (m_sString == 0)
        if (other.m_sString == 0 || *other.m_sString == 0)
            return 1;
    return m_sString != 0;
}

int CExoString::operator==(const char* str) const
{
    if (m_sString != 0 && str != 0)
        return strcmp(m_sString, str) == 0;
    if (m_sString == 0 && str == 0)
        return 1;
    if (m_sString == 0)
    {
        if (str == 0)
            return 0;
        return *str == 0;
    }
    if (*m_sString == 0)
        return 1;
    if (str != 0)
        return *str == 0;
    return 0;
}

int CExoString::operator!=(const char* str) const
{
    if (m_sString != 0 && str != 0)
        return strcmp(m_sString, str) != 0;
    if (m_sString == 0)
    {
        if (str == 0)
            return 0;
        return *str != 0;
    }
    if (*m_sString != 0)
        return 1;
    if (str != 0)
        return *str != 0;
    return 0;
}

int CExoString::operator<(const char* str) const
{
    if (m_sString != 0 && str != 0)
        return strcmp(m_sString, str) < 0;
    if (str != 0)
        return *str != '\0';
    return 0;
}

int CExoString::operator>(const char* str) const
{
    if (str != NULL && m_sString != NULL)
        return strcmp(m_sString, str) > 0;
    if (m_sString == NULL)
        return 0;
    return *m_sString != '\0';
}

int CExoString::operator<=(const char* str) const
{
    if (m_sString != 0 && str != 0)
        return strcmp(m_sString, str) <= 0;
    if (m_sString == 0 && str == 0)
        return 1;
    if (m_sString != 0)
    {
        if (*m_sString == 0)
            return 1;
    }
    return str != 0;
}

int CExoString::operator>=(const char* str) const
{
    if (m_sString != 0 && str != 0)
        return strcmp(m_sString, str) >= 0;
    if (m_sString == 0 && str == 0)
        return 1;
    if (str != 0)
    {
        if (*str == 0)
            return 1;
    }
    if (m_sString == 0)
        return 0;
    return 1;
}

char CExoString::operator[](int nPos) const
{
    if (nPos < 0)
        return 0;
    if (m_sString == NULL)
        return 0;
    if (strlen(m_sString) <= static_cast<unsigned int>(nPos))
        return 0;
    return m_sString[nPos];
}

CExoString::CExoString(const CExoString& other)
{
    if (other.m_sString != NULL && strlen(other.m_sString) != 0) {
        m_nBufferLength = static_cast<unsigned int>(strlen(other.m_sString) + 1);
        AllocString();
        strcpy(m_sString, other.m_sString);
    } else {
        m_sString = NULL;
        m_nBufferLength = 0;
    }
}

CExoString::~CExoString()
{
    if (m_sString != NULL) {
        m_nBufferLength = 0;
#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // K1 frees the buffer with scalar delete
        delete m_sString;
        m_sString = NULL;
#else
        DeallocString();
#endif
    }
}

CExoString& CExoString::operator=(const CExoString& other)
{
    if (this != &other) {
        if (m_sString != NULL) {
            if (other.m_sString == NULL || strlen(other.m_sString) + 1 > m_nBufferLength) {
                m_nBufferLength = 0;
#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // K1 frees the buffer with scalar delete
                delete m_sString;
                m_sString = NULL;
#else
                DeallocString();
#endif
            }
        }
        int length;
        if (other.m_sString != NULL && (length = static_cast<int>(strlen(other.m_sString))) >= 1) {
            if (m_sString == NULL) {
                m_nBufferLength = length + 1;
                AllocString();
            }
            strcpy(m_sString, other.m_sString);
        } else if (m_sString != NULL) {
#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // K1 frees the buffer with scalar delete
            delete m_sString;
            m_sString = NULL;
#else
            DeallocString();
#endif
            m_nBufferLength = 0;
        }
    }
    return *this;
}

CExoString& CExoString::operator=(const char* str)
{
    if (m_sString != NULL) {
        if (str == NULL || strlen(str) + 1 > m_nBufferLength) {
            m_nBufferLength = 0;
#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // K1 frees the buffer with scalar delete
            delete m_sString;
            m_sString = NULL;
#else
            DeallocString();
#endif
        }
    }
    if (str != NULL && static_cast<int>(strlen(str)) >= 1) {
        if (m_sString == NULL) {
            m_nBufferLength = static_cast<unsigned int>(strlen(str) + 1);
            AllocString();
        }
        strcpy(m_sString, str);
    } else if (m_sString != NULL) {
#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // K1 frees the buffer with scalar delete
        delete m_sString;
        m_sString = NULL;
#else
        DeallocString();
#endif
        m_nBufferLength = 0;
    }
    return *this;
}

int CExoString::Find(char ch, int nStart) const
{
    if (nStart < 0)
        return -1;
    const char* p = m_sString;
    if (p == NULL)
        return -1;
    for (int i = nStart; i >= 1; i--) {
        if (*p == '\0')
            return -1;
        p++;
    }
    while (*p != '\0') {
        if (*p == ch)
            return nStart;
        p++;
        nStart++;
    }
    return -1;
}

int CExoString::FindNot(char ch, int nStart) const
{
    if (nStart < 0)
        return -1;
    const char* p = m_sString;
    if (p == NULL)
        return -1;
    for (int i = nStart; i >= 1; i--) {
        if (*p == '\0')
            return -1;
        p++;
    }
    while (*p != '\0') {
        if (*p != ch)
            return nStart;
        p++;
        nStart++;
    }
    return -1;
}

// printf-style reformat, through a static scratch buffer that grows until the result fits.
void CExoString::Format(char* pszFormat, ...)
{
    static char* s_pszBuffer = NULL;
    static int   s_nBufferSize = 0;

    bool bInit = (s_pszBuffer == NULL && s_nBufferSize == 0);

    __builtin_va_list ap;
    __builtin_va_start(ap, pszFormat);

    if (bInit) {
        s_nBufferSize = FORMAT_BUFFER_STEP;
        s_pszBuffer = new char[FORMAT_BUFFER_STEP];
    }

#if KOTOR1  // K1 formats with the builtin, which binds libc
    int nLength = __builtin_vsnprintf(s_pszBuffer, s_nBufferSize, pszFormat, ap);
#else
    int nLength = _vsnprintf(s_pszBuffer, s_nBufferSize, pszFormat, ap);
#endif
    if (nLength < -1)
        return;
#if KOTOR1 && defined(K2_FIX_ENGINE_BUGS)
    // C99 vsnprintf reports a truncated result by its full length, not by -1.
    while (nLength < 0 || nLength >= s_nBufferSize) {
#else
    while (nLength < 0) {
#endif
        s_nBufferSize += FORMAT_BUFFER_STEP;
        delete[] s_pszBuffer;
        s_pszBuffer = new char[s_nBufferSize];
#if KOTOR1
        nLength = __builtin_vsnprintf(s_pszBuffer, s_nBufferSize, pszFormat, ap);
#else
        nLength = _vsnprintf(s_pszBuffer, s_nBufferSize, pszFormat, ap);
#endif
    }

    if (m_sString == NULL || static_cast<unsigned int>(nLength) >= m_nBufferLength) {
        if (m_sString != NULL) {
            m_nBufferLength = 0;
#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // K1 frees the buffer with scalar delete
            delete m_sString;
            m_sString = NULL;
#else
            DeallocString();
#endif
        }
        m_nBufferLength = nLength + 1;
        AllocString();
    }
    K2_ASSUME(s_pszBuffer != NULL);   // allocated on the first call (bInit), only ever replaced since
    strncpy(m_sString, s_pszBuffer, nLength);
    m_sString[nLength] = 0;
}

int CExoString::Find(const CExoString& other, int nStart) const
{
    const char* h = m_sString;
    if (h == NULL)
        return -1;
    if (nStart < 0)
        return -1;
    const char* n = other.m_sString;
    if (n == NULL)
        return -1;
    for (int k = nStart; k != 0; k--) {
        if (*h == '\0')
            return -1;
        h++;
    }
    int j = 0;
    while (h[j] != '\0') {
        if (n[j] == '\0')
            return static_cast<int>(h - m_sString);
        if (h[j] == n[j])
            j++;
        else {
            h++;
            j = 0;
        }
    }
    if (n[j] == '\0')
        return static_cast<int>(h - m_sString);
    return -1;
}

CExoString CExoString::Left(int nCount) const
{
    CExoString result;
    if (m_sString != NULL) {
        int length = static_cast<int>(strlen(m_sString));
        if (nCount >= 0 && length != 0) {
            if (static_cast<unsigned>(length) < static_cast<unsigned>(nCount))
                nCount = length;
            if (result.m_sString != NULL)
                result.DeallocString();
            result.m_nBufferLength = nCount + 1;
            result.AllocString();
            strncpy(result.m_sString, m_sString, nCount)[nCount] = '\0';
        }
    }
    return result;
}

CExoString CExoString::Right(int nCount) const
{
    CExoString result;
    if (m_sString != NULL) {
        int length = static_cast<int>(strlen(m_sString));
        if (nCount >= 0 && length != 0) {
            if (static_cast<unsigned>(length) < static_cast<unsigned>(nCount))
                nCount = length;
            if (result.m_sString != NULL)
                result.DeallocString();
            result.m_nBufferLength = nCount + 1;
            result.AllocString();
            strncpy(result.m_sString, m_sString + static_cast<unsigned>(length - nCount), nCount)[nCount] = '\0';
        }
    }
    return result;
}

CExoString CExoString::SubString(int nStart, int nCount) const
{
    CExoString result;
    if (m_sString != NULL) {
        int length = static_cast<int>(strlen(m_sString));
        if (nStart >= 0 && nCount != 0 && static_cast<unsigned>(length) > static_cast<unsigned>(nStart)) {
            if (nCount < 0)
                nCount = length - nStart;
            if (result.m_sString != NULL)
                result.DeallocString();
            result.m_nBufferLength = nCount + 1;
            result.AllocString();
            strncpy(result.m_sString, m_sString + nStart, nCount)[nCount] = '\0';
        }
    }
    return result;
}

CExoString CExoString::LowerCase() const
{
    CExoString result;
    if (m_sString != NULL) {
        result.m_nBufferLength = GetLength() + 1;
        result.AllocString();
        int i;
        for (i = 0; m_sString[i] != '\0'; i++) {
            char c = m_sString[i];
            result.m_sString[i] = static_cast<unsigned char>(c - 'A') < 26 ? static_cast<char>(c + ('a' - 'A')) : c;
        }
        result.m_sString[i] = '\0';
    }
    return result;
}

CExoString CExoString::UpperCase() const
{
    CExoString result;
    if (m_sString != NULL) {
        result.m_nBufferLength = GetLength() + 1;
        result.AllocString();
        int i;
        for (i = 0; m_sString[i] != '\0'; i++) {
            char c = m_sString[i];
            result.m_sString[i] = static_cast<unsigned char>(c - 'a') < 26 ? static_cast<char>(c - ('a' - 'A')) : c;
        }
        result.m_sString[i] = '\0';
    }
    return result;
}

CExoString CExoString::operator+(const CExoString& other) const
{
    CExoString result;
    int lenOther = (other.m_sString != NULL) ? static_cast<int>(strlen(other.m_sString)) : 0;
    int lenThis  = (m_sString != NULL) ? static_cast<int>(strlen(m_sString)) : 0;
    if (lenThis != 0 || lenOther != 0) {
        if (result.m_sString != NULL) {
            result.DeallocString();
            result.m_nBufferLength = 0;
        }
        if (lenOther == 0) {
            result.m_nBufferLength = lenThis + 1;
            result.AllocString();
            strcpy(result.m_sString, m_sString);
        } else if (lenThis == 0) {
            result.m_nBufferLength = lenOther + 1;
            result.AllocString();
            strcpy(result.m_sString, other.m_sString);
        } else {
            result.m_nBufferLength = lenOther + 1 + lenThis;
            result.AllocString();
            strcpy(result.m_sString, m_sString);
            strcat(result.m_sString, other.m_sString);
        }
    }
    return result;
}

// Inserts in the middle of the string. The buffer length is left stale afterwards.
void CExoString::Insert(const CExoString& other, int nPos)
{
    if (other.m_sString == NULL)
        return;
    int lenOther = static_cast<int>(strlen(other.m_sString));
    int lenThis  = (m_sString != NULL) ? static_cast<int>(strlen(m_sString)) : 0;
    if (lenOther != 0 && nPos >= 0 && static_cast<unsigned>(nPos) < static_cast<unsigned>(lenThis)) {
        char* pNew = new char[static_cast<unsigned>(lenOther + lenThis + 1)];
        pNew[0] = '\0';
        char* pOld = m_sString;
        m_sString = pNew;
        if (pOld != NULL)
            strncpy(pNew, pOld, nPos);
        m_sString[nPos] = '\0';
        strcat(m_sString, other.m_sString);
        strcat(m_sString, pOld + nPos);
        delete[] pOld;
    }
}

int CExoString::CompareNoCase(const CExoString& other) const
{
    const char* p1 = other.m_sString;
    if (p1 == NULL)
        return m_sString == NULL;
    int lenOther = other.GetLength();
    const char* p2 = m_sString;
    if (p2 == NULL)
        return 0;
    if (lenOther != GetLength())
        return 0;
    for (int i = 0; i < lenOther; i++) {
        signed char c1 = p1[i];
        signed char c2 = p2[i];
        if (c1 != c2) {
            if (static_cast<unsigned>(c1 - 'A') <= 25) {
                if (c1 + ('a' - 'A') != c2)
                    return 0;
            } else if (static_cast<unsigned>(c2 - 'A') <= 25) {
                if (c2 + ('a' - 'A') != c1)
                    return 0;
            } else {
                return 0;
            }
        }
    }
    return 1;
}

int CExoString::CompareNoCase(const char* str) const
{
    if (m_sString == NULL && str == NULL)
        return 1;
    if (str == NULL || m_sString == NULL)
        return 0;
#if KOTOR1  // K1 compares with libc strcasecmp
    return strcasecmp(str, m_sString) == 0;
#else
    return _stricmp(str, m_sString) == 0;
#endif
}

int CExoString::ComparePrefixNoCase(const CExoString& other, int nCount) const
{
    if (m_sString == NULL)
        return 0;
    const char* p = (other.m_sString != NULL) ? other.m_sString : "";
#if KOTOR1  // K1 compares with libc strncasecmp
    return strncasecmp(p, m_sString, nCount) == 0;
#else
    return _strnicmp(p, m_sString, nCount) == 0;
#endif
}

char* CExoString::CStr() const
{
    return m_sString != NULL ? m_sString : const_cast<char*>("");
}

#if KOTOR1  // K1 has these four accessors, K2 does not
int CExoString::AsINT() const
{
    return m_sString != NULL ? atoi(m_sString) : 0;
}

float CExoString::AsFLOAT() const
{
    return m_sString != NULL ? static_cast<float>(atof(m_sString)) : 0.0f;
}

int CExoString::GetLength() const
{
    return m_sString != NULL ? static_cast<int>(strlen(m_sString)) : 0;
}

int CExoString::IsEmpty() const
{
    return m_sString == NULL || m_sString[0] == '\0';
}
#endif

// Removes every character outside the allowed set in place and returns whether anything changed.
// bLower selects the lowercase-only set, lowercasing A-Z as it goes.
int CExoString::StripNonAlphaNumeric(int bLower, int bAllowAt, int bAllowPunct)
{
    if (m_sString == NULL)
        return 0;
    int len = static_cast<int>(strlen(m_sString));
    if (len == 0)
        return 0;

    char szAllowed[128];
#if KOTOR1  // K1's sets carry the Latin-1 accented letters themselves
    if (bLower) {
        __builtin_memcpy(szAllowed,
            "abcdefghijklmnopqrstuvwxyz0123456789'"
            "\xdf\xe9\xc9\xe8\xc8\xc7\xe7\xf9\xd9\xf1\xd1\xe0\xc0\xc4\xe4\xdc\xfc\xd6\xf6"
            "_-", 59);
    } else {
        __builtin_memcpy(szAllowed,
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", 63);
    }
    if (bAllowAt)
        __builtin___strcat_chk(szAllowed, "@.", sizeof(szAllowed));
    if (bAllowPunct)
        __builtin___strcat_chk(szAllowed,
            ".',"
            "\xdf\xe9\xc9\xe8\xc8\xc7\xe7\xf9\xd9\xf1\xd1\xe0\xc0\xc4\xe4\xdc\xfc\xd6\xf6"
            "_- ", sizeof(szAllowed));
#else
    if (bLower) {
        __builtin_memcpy(szAllowed,
            "abcdefghijklmnopqrstuvwxyz0123456789'"
            "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd"
            "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd"
            "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd"
            "_-", 96);
        szAllowed[96] = '\0';
    } else {
        __builtin_memcpy(szAllowed,
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", 63);
    }
    if (bAllowAt)
        __builtin___strcat_chk(szAllowed, "@.", sizeof(szAllowed));
    if (bAllowPunct)
        __builtin___strcat_chk(szAllowed,
            ".',"
            "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd"
            "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd"
            "\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd\xef\xbf\xbd"
            "_- ", sizeof(szAllowed));
#endif

    unsigned i = 0;
    int bModified = 0;
    for (; i < static_cast<unsigned>(len); i++) {
        if (bLower) {
            char c = m_sString[i];
            if (c >= 'A' && c <= 'Z')
                m_sString[i] = c + ('a' - 'A');
        }
        if (__strchr_chk(szAllowed, static_cast<signed char>(m_sString[i]), sizeof(szAllowed)) == NULL) {
            for (unsigned j = i; j < static_cast<unsigned>(len); j++)
                m_sString[j] = m_sString[j + 1];
            i--;
            len--;
            bModified = 1;
        }
    }
    return bModified;
}

CExoString::CExoString(int value)
{
    char buf[33];
    sprintf(buf, "%i", value);
    m_nBufferLength = static_cast<unsigned int>(__strlen_chk(buf, sizeof(buf)) + 1);
    AllocString();
    strcpy(m_sString, buf);
}

