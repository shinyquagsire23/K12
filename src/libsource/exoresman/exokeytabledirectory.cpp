#include "k2stdint.h"
#include "exoresfile.h"
#include <stddef.h>
#include "CExoArrayListImpl.h"
#include "exobase.h"

#include "port/libcstring.h"
#include "exostringinline.h"
#include "restype.h"

int CExoKeyTable::AddDirectoryContents(int)
{
#if KOTOR1  // K1: no override subdirectories; two listings of the directory into one list
    CExoString curFile;
    CExoArrayList<CExoString> files;
    CResRef resRef;
    m_sFileName.CStr();
    g_pExoBase->GetDirectoryList(&files, m_sFileName, RESTYPE_INVALID, 0, 0, 0);
    g_pExoBase->GetDirectoryList(&files, m_sFileName, RESTYPE_INVALID, 0, 0, 1);
    int nFiles = files.m_count;
    m_nTableSize = (nFiles * 2) | 1;
    m_paEntries = AllocateTable(m_nTableSize);
    for (int i = 0; i < nFiles; i++) {
        curFile = files.m_pArray[i];
        unsigned short resType = g_pExoResMan->GetResTypeFromFile(curFile);
        if (resType == RESTYPE_INVALID)
            continue;
        g_pExoResMan->GetResRefFromFile(resRef, curFile);
        if (resRef != const_cast<char*>("")) {
            AddKey(resRef, resType, (m_nRoutingTag << RESID_FILE_SHIFT) | RESID_DIRECTORY, 0);
            curFile.CStr();
            resRef.GetResRefStr();
        }
    }
#else
    CExoString curFile;
    CExoArrayList<CExoString> topLevelFiles;
    CExoArrayList<CExoString> subdirNames;
    CExoArrayList<CExoArrayList<CExoString>> subdirFileLists;
    CResRef resRef;
    int nTopLevel;
    unsigned int newTableSize;
    unsigned short resType;
    unsigned int resID;

    CExoString overrideLit("OVERRIDE");
    int numSubdirs;
    if (m_sFileName.ComparePrefixNoCase(overrideLit, overrideLit.GetLength()) == 0) {
        numSubdirs = 0;
    } else {
        g_pExoBase->GetDirectoryList(&subdirNames, m_sFileName, RESTYPE_INVALID, 1, 0);
        numSubdirs = subdirNames.m_count;
    }

    CExoString scratch;
    int totalSubFiles = 0;
    if (numSubdirs >= 1) {
        for (int i = 0; i < numSubdirs; i++) {
            scratch = CExoString("OVERRIDE:") + subdirNames.m_pArray[i];

            CExoArrayList<CExoString> filesInSubdir;
            g_pExoBase->GetDirectoryList(&filesInSubdir, scratch, RESTYPE_INVALID, 0, 0);
            subdirFileLists.Add(filesInSubdir);

            totalSubFiles += subdirFileLists.m_pArray[i].m_count;
        }
    }

    g_pExoBase->GetDirectoryList(&topLevelFiles, m_sFileName, RESTYPE_INVALID, 0, 0);
    nTopLevel = topLevelFiles.m_count;
    newTableSize = ((nTopLevel + static_cast<unsigned int>(totalSubFiles)) * 2) | 1;
    m_nTableSize = newTableSize;
    m_paEntries = AllocateTable(newTableSize);

    for (int i = 0; i < nTopLevel; i++) {
        curFile = topLevelFiles.m_pArray[i];
        resType = g_pExoResMan->GetResTypeFromFile(curFile);
        if (resType != RESTYPE_INVALID) {
            g_pExoResMan->GetResRefFromFile(resRef, curFile);
            if (resRef != const_cast<char*>("")) {
                resID = (m_nRoutingTag << RESID_FILE_SHIFT) | RESID_DIRECTORY;
                AddKey(resRef, resType, resID, 0, CExoString(""));
            }
        }
    }

    for (int i = 0; i < numSubdirs; i++) {
        scratch = subdirNames.m_pArray[i];
        for (int j = 0; j < subdirFileLists.m_pArray[i].m_count; j++) {
            curFile = subdirFileLists.m_pArray[i].m_pArray[j];
            resType = g_pExoResMan->GetResTypeFromFile(curFile);
            if (resType != RESTYPE_INVALID) {
                g_pExoResMan->GetResRefFromFile(resRef, curFile);
                if (resRef != const_cast<char*>("")) {
                    resID = (m_nRoutingTag << RESID_FILE_SHIFT) | RESID_DIRECTORY;
                    AddKey(resRef, resType, resID, 0, CExoString(scratch));
                }
            }
        }
    }

#endif
    return 1;
}
