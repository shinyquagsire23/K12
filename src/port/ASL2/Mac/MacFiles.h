#pragma once
#include "k2stdint.h"

// The Win32 stdio-API emulation layer of the Mac/Android port.

#include <string>

// The stdio stream type is kept as a forward declaration rather than <stdio.h>: -fno-builtin is a
// tree-wide flag and that header re-adds the purity attributes the flag exists to defeat, so the
// stdio entry points this TU calls are declared by hand in the .cpp.
#include "port/libcbase.h"

#include "WinThreadsMP.h"

// The port's path type. It is not the Carbon FSRef but a class of the same name that holds the
// path string.
struct FSRef {
    std::string m_sPath;
};

// A refcounted wrapper around a stdio stream.
class FileHANDLE : public HANDLE_Mac {
public:
    FileHANDLE(int nFD, const uint64_t* pFile, unsigned int nAccess, unsigned int nFlags);
    virtual ~FileHANDLE();

    bool GetFSRef(FSRef* pFSRef);

    int m_nFD;
    FILE* m_pFile;
    unsigned int m_nAccess;
    unsigned long long m_nFileCursor;
    // The mapping code reads byte 3 of this word, so the byte view has to be a real member.
    union {
        unsigned int m_nFlags;
        unsigned char m_abFlags[4];
    };
};

// A refcounted memory mapping.
class FileMappingHANDLE : public HANDLE_Mac {
public:
    FileMappingHANDLE(void* pUnk, unsigned int nProtect);
    virtual ~FileMappingHANDLE();

    FileHANDLE* m_pFileHandle;
    HANDLE_Mac* m_hFile;
    unsigned char m_bMapped;
    unsigned int m_nProtect;
    unsigned int m_nMappingSize;
    int m_nMapFD;
};
