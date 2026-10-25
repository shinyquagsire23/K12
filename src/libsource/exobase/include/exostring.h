#pragma once

class CExoString {
public:
    char*        m_sString;
    unsigned int m_nBufferLength;

    CExoString();
    CExoString(const char* str);
    CExoString(const CExoString& other);
    CExoString(const char* str, int length);
    CExoString(int value);
    ~CExoString();

    CExoString& operator=(const CExoString& other);
    CExoString& operator=(const char* str);

    int operator==(const CExoString& other) const;
    int operator!=(const CExoString& other) const;
    int operator<(const CExoString& other) const;
    int operator>(const CExoString& other) const;
    int operator<=(const CExoString& other) const;
    int operator>=(const CExoString& other) const;

    int operator==(const char* str) const;
    int operator!=(const char* str) const;
    int operator<(const char* str) const;
    int operator>(const char* str) const;
    int operator<=(const char* str) const;
    int operator>=(const char* str) const;

    char operator[](int nPos) const;

    CExoString operator+(const CExoString& other) const;
    void Insert(const CExoString& other, int nPos);

    int  CompareNoCase(const CExoString& other) const;
    int  CompareNoCase(const char* str) const;
    int  ComparePrefixNoCase(const CExoString& other, int nCount) const;

    char* CStr() const;
    // K2: inline, exostringinline.h; K1: out of line.
    int   GetLength() const;
    int   AsINT() const;
    float AsFLOAT() const;
    int   IsEmpty() const;
    int  Find(const CExoString& other, int nStart) const;
    int  Find(char ch, int nStart) const;
    int  FindNot(char ch, int nStart) const;
    CExoString Left(int nCount) const;
    CExoString Right(int nCount) const;
    CExoString SubString(int nStart, int nCount) const;
    CExoString LowerCase() const;
    CExoString UpperCase() const;
    int  StripNonAlphaNumeric(int bLower, int bAllowAt, int bAllowPunct);

    void Format(char* fmt, ...);

    // Buffer management: m_nBufferLength is set first, then the buffer allocated to it.
    void AllocString() { m_sString = new char[m_nBufferLength]; }
    void DeallocString()
    {
        delete[] m_sString;
        m_sString = 0;
    }
};
