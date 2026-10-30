#pragma once
#include "k2stdint.h"

#ifndef CERFFILE_H
#define CERFFILE_H

// The ERF/BIF-style archive writer the server-side save path flushes modules and areas through
// (CSWSModule::SaveStatic, CServerExoAppInternal's save paths, CSWGuiSaveLoad). Non-polymorphic:
// no vptr, non-virtual destructor. The header image is written wholesale, and a running copy of the
// three table offsets mirrors it.

#include "CExoLinkedList.h"
#include "exostring.h"

class CExoFile;
class CERFKey;
class CERFRes;
class CERFString;
class CRes;

// The ERF header image. The single memset in the constructor covers exactly this struct.
struct CERFHeader {
    // Signature and version share one 8-byte word so the version is copied in one store; keep the
    // struct 8-byte aligned.
    uint64_ll_t  m_nSignature;
    unsigned int m_nStringCount;
    unsigned int m_nStringTableSize;
    unsigned int m_nKeyCount;
    unsigned int m_nStringTableOffset;
    unsigned int m_nKeyTableOffset;
    unsigned int m_nResTableOffset;
    int          m_nBuildYear;
    int          m_nBuildDay;
    unsigned int m_nDescriptionStrRef;
    unsigned char m_abReserved[0xa0 - 0x2c];
};

class CERFFile {
public:
    CERFFile();
    // Non-virtual: the save path tears the archive down with an explicit destructor call and
    // operator delete, not with `delete p;`.
    ~CERFFile();

    int  Reset();
    void SetVersion(char* version);

    int  AddResource(CExoString& filePath);
    int  AddResource(char* sName, unsigned short nType, CRes* pRes);
    int  RemoveResource(CExoString& name, unsigned short nType);
    int  RemoveResource(CERFRes* pRes);
    int  AddString(CERFString* pString);
    int  ReadData(const CExoString& path);
    int  Create(CExoString& path);
    int  WriteHeader();
    int  Finish();
    int  Write(CExoString& path);
    void RecalculateOffsets();

    int  SetNumEntries(unsigned int nEntries);
    int  WriteStringTable();
    // Writes one resource, a raw GFF blob, into the archive. The resource is not deleted here; the
    // caller tears it down after the write.
    int  WriteResource(const CExoString& filePath, void* pExtra);
    int  WriteResource(char* sName, unsigned short nType, CRes* pRes, int nBufferSize, void* pExtra);
    void ImportFiles(const CExoString& aliasPrefix, void* fileTypeMask,
                      int (*progressCallback)(int, int, int, int),
                      int progressMin, int progressMax);
    int  Read(const CExoString& path);
    int  ReadHeaderVariance(const CExoString& path);

    // Reads a module .ifo out of an archive and formats its description string.
    static int ReadModuleDescription(const CExoString& path, unsigned short nType,
                                     uint64_t* pSize, CExoString* pName);

    void ExportFilesFromERF(const CExoString& path, const CExoString& aliasPrefix, void* fileTypeMask,
                             int (*progressCallback)(int, int, int, int),
                             int progressMin, int progressMax);
    void ExportFiles(const CExoString& dir, void* fileTypeMask,
                      int (*progressCallback)(int, int, int, int),
                      int progressMin, int progressMax);

    CERFHeader m_header;
    CExoLinkedList<CERFString> m_lstStrings;
    CExoLinkedList<CERFKey>    m_lstKeys;
    CExoLinkedList<CERFRes>    m_lstRes;
    CExoFile*                  m_pFile;
    unsigned int               m_nStringTableOffset;
    unsigned int               m_nKeyTableOffset;
    unsigned int               m_nResTableOffset;
    unsigned int               m_nEntryCount;
    unsigned int               m_nNextEntry;
    unsigned int               m_nFileSize;
    CExoString                 m_sDataPath;
};

#endif // CERFFILE_H

#ifndef CERFKEY_H
#define CERFKEY_H

// One entry of the archive's key table: the resource name to offset/type record. Non-polymorphic.

class CExoFile;

class CERFKey {
public:
    CERFKey();
    ~CERFKey();
    int  Reset();
    void AddToSig(void* pSig);
    int  Read(CExoFile& file);
    int  Write(CExoFile& file);
#if KOTOR1
    void SetName(CExoString& name);  // K1 defines this in the class
#else
    __attribute__((noinline)) void SetName(CExoString& name);
#endif

    char           m_sName[16];
    unsigned int   m_nID;
    unsigned short m_nType;
    unsigned short m_nReserved1;
};

#endif // CERFKEY_H

#ifndef CERFRES_H
#define CERFRES_H

// One resource payload inside an archive: the on-disk offset and size plus the lazily loaded bytes.
// Non-polymorphic. The payload is allocated with new[] but released with scalar delete.

class CExoFile;

class CERFRes {
public:
    CERFRes();
    ~CERFRes();
    int  Reset();
    void AddToSig(void* pSig);
    int  Read(CExoFile& file);
    int  ReadData(CExoFile& file);
    int  Write(CExoFile& file, unsigned int nHeaderOffset);
    __attribute__((always_inline))
    int  CopyToFile(CExoFile* pSource, CExoFile* pDest, void* pBuffer);

    unsigned int   m_nOffset;
    unsigned int   m_nSize;
    unsigned char* m_pData;
};

#endif // CERFRES_H

#ifndef CERFSTRING_H
#define CERFSTRING_H

// One entry of an archive's string table: an id, a length and the NUL-terminated bytes.
// Non-polymorphic. The length is the strlen of the buffer, not the CExoString buffer length, and it
// must stay unsigned: a signed member would be sign-extended at every use and would wrap the
// allocation in a negative-size clamp.

class CExoFile;

class CERFString {
public:
    CERFString();
    ~CERFString();
    int  Reset();
    void AddToSig(void* pSig);
    CExoString GetText();
    void SetText(CExoString& text);
    int  Read(CExoFile& file);
    int  Write(CExoFile& file);

    unsigned int m_nLanguageId;
    unsigned int m_nLength;
    char*        m_pString;
};

#endif // CERFSTRING_H
