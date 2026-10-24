#ifndef CRIMFILE_H
#define CRIMFILE_H

#include "CExoLinkedList.h"
#include "exostring.h"

class CExoFile;
class CRes;

// The on-disk header, embedded at the start of CRIMFile.
struct CRIMHeader {
    char          m_szVersion[8];
    int           m_nReserved1;
    int           m_nNumEntries;
    int           m_nDataOffset;
    unsigned char m_bBlockAlign;
    char          m_aReserved2[99];
};

// One entry of a RIM archive, with its resource payload.
class CRIMRes {
public:
    CRIMRes();
    ~CRIMRes();

    int  Reset();
#if KOTOR1  // K1 defines SetName in the class
    void SetName(CExoString& sName);
#else
    __attribute__((noinline)) void SetName(CExoString& sName);
#endif
    int  Read(CExoFile& file);
    // bBlockAlign selects the payload's block alignment: 4 bytes, or a large block when non-zero.
    int  Write(CExoFile& file, unsigned int nEntryOffset, unsigned char bBlockAlign);

    char           m_szName[16];
    unsigned short m_nType;
    int            m_nID;
    int            m_nOffset;
    unsigned int   m_nSize;
    void*          m_pData;
};

// A RIM ("resource image") archive: a header, one entry per resource, then the resource payloads.
class CRIMFile {
public:
    CRIMFile();
    ~CRIMFile();

    __attribute__((noinline)) int Reset();
    void SetVersion(char* pszVersion);
    int  Read(const CExoString& sFileName);
    // Returns the file-open status.
    int  Create(CExoString& sFileName);
    int  WriteHeader();
    int  SetNumEntries(unsigned int nNumEntries);
    int  WriteResource(const CExoString& sResRef);
    // Declared CRes*, but treated as a CResGFF when bGFF is set.
    int  WriteResource(char* pszName, unsigned short nType, CRes* pRes, int bGFF);
    int  Finish();
    int  Write(CExoString& sFileName);
    void RecalculateOffsets();
    void ExportFiles(const CExoString& sPath);

    CRIMHeader     m_header;
    CExoLinkedList<CRIMRes> m_lResources;
    CExoFile*      m_pFile;
    int            m_nEntriesOffset;
    unsigned int   m_nRezCount;
    unsigned int   m_nWriteIndex;
};

#endif
