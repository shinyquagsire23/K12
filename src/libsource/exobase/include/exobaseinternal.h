#pragma once
#include "k2stdint.h"

// No CRT headers here: several translation units include this from inside local extern "C" prototype
// blocks, and pulling in <stdint.h> would redeclare atoi.

#ifndef CEXOBASEINTERNAL_H
#define CEXOBASEINTERNAL_H

#include "exostring.h"

template<class T> class CExoArrayList;
class CFileInfo;

class CExoBaseInternal {
public:
    CExoBaseInternal();
    ~CExoBaseInternal();

    // Registers one entry of the alias table.
    static void AddAlias(const CExoString& sRoot, const CExoString& sBase,
                         const CExoString& sName, const CExoString& sDir);

    // Fills the resource-type to file-extension table.
    void CreateResourceExtensionTable();

    unsigned int GetDirectorySize(const CExoString& path);
    int CheckForCD();
    // Returns the number of entries added; subdirectories are skipped and not counted.
    int GetAugmentedDirectoryList(CExoArrayList<CFileInfo>* pOut, const CExoString& dir, unsigned short types, int flag);
    void SpawnExternalApplication(const CExoString& path, const CExoString& args, int flag);
    unsigned short GetResTypeFromExtension(const CExoString& ext);
    const CExoString& GetResourceExtension(unsigned short resType);
    // Returns 1 when the directory was opened, 0 when the host could not open it.
#if KOTOR1  // K1 takes a sixth argument: a nonzero c looks the directory up in the patch or main OBB first
    int GetDirectoryList(CExoArrayList<CExoString>* pOut, const CExoString& dir, unsigned short types, int a, int b, int c);
#else
    int GetDirectoryList(CExoArrayList<CExoString>* pOut, const CExoString& dir, unsigned short types, int a, int b);
#endif
    void ShutDown();
    // Builds the whole alias table: the install-relative base path plus the built-in aliases (HD0,
    // CD0, the override directory, ..., the full-path entry).
    void LoadAliases(const CExoString& path);

    int             m_nResTypeCount;
    unsigned short* m_pResTypes;
    CExoString*     m_aResourceExtensions;

    // Always reports success.
    int WcsToMbs(char* dst, const K2WChar* src, uint64_t n);
};

#endif // CEXOBASEINTERNAL_H

// idiom-exempt: canonical
// One entry of a directory listing, produced by CExoBaseInternal::GetAugmentedDirectoryList.
#include "exostring.h"

// A packed file timestamp: a Win32 system time without its day of week.
struct CExoFileTime {
    unsigned short m_nYear;
    unsigned short m_nMonth;
    unsigned short m_nDay;
    unsigned short m_nHour;
    unsigned short m_nMinute;
    unsigned short m_nSecond;
};

class CFileInfo {
public:
    CExoString   m_name;
    CExoFileTime m_time;
};
