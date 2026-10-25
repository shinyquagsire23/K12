#include "k2stdint.h"
#include "port/sdlapi.h"
#include "exobaseinternal.h"

#include "exobase.h"
#include "CExoArrayListImpl.h"
#include "exoiniinternal.h"
#if KOTOR1  // K1 lists the OBB tree or a POSIX directory through std::string paths
#include <string>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "port/k1obb.h"
#endif

#include "port/libcstring.h"
#include "port/libcstdlib.h"

#include "Win32Find.h"
#include "restype.h"
#include "exostringinline.h"


enum { DRIVE_CDROM = 5 };

struct _STARTUPINFOA {
    unsigned int  cb;
    char*         lpReserved;
    char*         lpDesktop;
    char*         lpTitle;
    unsigned int  dwX;
    unsigned int  dwY;
    unsigned int  dwXSize;
    unsigned int  dwYSize;
    unsigned int  dwXCountChars;
    unsigned int  dwYCountChars;
    unsigned int  dwFillAttribute;
    unsigned int  dwFlags;
    unsigned short wShowWindow;
    unsigned short cbReserved2;
    unsigned char* lpReserved2;
    void*         hStdInput;
    void*         hStdOutput;
    void*         hStdError;
};
typedef _STARTUPINFOA STARTUPINFOA;

struct _PROCESS_INFORMATION {
    void*        hProcess;
    void*        hThread;
    unsigned int dwProcessId;
    unsigned int dwThreadId;
};
typedef _PROCESS_INFORMATION PROCESS_INFORMATION;

enum {
    RES_EXTENSION_FALLBACK = 0x57,
};

// The character a wide string's control codes and unconvertible characters become.
enum {
    UNPRINTABLE_CHAR = 0x7f,
};

extern CExoString g_sBuildNumber;
extern CExoString g_sVersionNumber;
extern CExoString g_sGameName;
extern CExoString g_sCopyrightInfo;
int GetInstallVersion(CExoString& sInstallVersion)
{
    K2_UNUSED(sInstallVersion);
    return 0;
}

void GetGameVersionInfo()
{
    g_sBuildNumber   = "514078";
    g_sVersionNumber = "1, 0, 3, 0";
#if KOTOR1  // K1 uses the first game's name and Latin-1 copyright signs
    g_sGameName      = "Star Wars: Knights of the Old Republic";
    g_sCopyrightInfo = "\xa9 2003 Lucasfilm Entertainment Company Ltd. or Lucasfilm Ltd. All rights"
                       " reserved. BioWare Odyssey Engine \xa9 2001 - 2003 BioWare Corp. All rights"
                       " reserved.";
#else
    g_sGameName      = "Star Wars: Knights of the Old Republic II";
    g_sCopyrightInfo = " 2003 Lucasfilm Entertainment Company Ltd. or Lucasfilm Ltd. All rights"
                       " reserved. BioWare Odyssey Engine  2001 - 2003 BioWare Corp. All rights"
                       " reserved.";
#endif
}

CExoBaseInternal::CExoBaseInternal()
{
    CreateResourceExtensionTable();
}

// Builds the resource-type/extension table, 88 rows filled by a straight-line run of explicit
// pairs.
void CExoBaseInternal::CreateResourceExtensionTable()
{
    const int nCount = 88;
    m_nResTypeCount = nCount;
    m_pResTypes = new unsigned short[nCount];
    m_aResourceExtensions = new CExoString[nCount];

    m_pResTypes[0] = 0;
    m_aResourceExtensions[0] = "res";
    m_pResTypes[1] = 1;
    m_aResourceExtensions[1] = "bmp";
    m_pResTypes[2] = 2;
    m_aResourceExtensions[2] = "mve";
    m_pResTypes[3] = 3;
    m_aResourceExtensions[3] = "tga";
    m_pResTypes[4] = 4;
    m_aResourceExtensions[4] = "wav";
    m_pResTypes[5] = 7;
    m_aResourceExtensions[5] = "ini";
    m_pResTypes[6] = 10;
    m_aResourceExtensions[6] = "txt";
    m_pResTypes[7] = 2022;
    m_aResourceExtensions[7] = "txi";
    m_pResTypes[8] = 9999;
    m_aResourceExtensions[8] = "key";
    m_pResTypes[9] = 9998;
    m_aResourceExtensions[9] = "bif";
    m_pResTypes[10] = 9997;
    m_aResourceExtensions[10] = "erf";
    m_pResTypes[11] = 2000;
    m_aResourceExtensions[11] = "plh";
    m_pResTypes[12] = 2001;
    m_aResourceExtensions[12] = "tex";
    m_pResTypes[13] = 2002;
    m_aResourceExtensions[13] = "mdl";
    m_pResTypes[14] = 2007;
    m_aResourceExtensions[14] = "lua";
    m_pResTypes[15] = 2003;
    m_aResourceExtensions[15] = "thg";
    m_pResTypes[16] = 2008;
    m_aResourceExtensions[16] = "slt";
    m_pResTypes[17] = 2009;
    m_aResourceExtensions[17] = "nss";
    m_pResTypes[18] = 2010;
    m_aResourceExtensions[18] = "ncs";
    m_pResTypes[19] = 2011;
    m_aResourceExtensions[19] = "mod";
    m_pResTypes[20] = 2012;
    m_aResourceExtensions[20] = "are";
    m_pResTypes[21] = 2013;
    m_aResourceExtensions[21] = "set";
    m_pResTypes[22] = 2014;
    m_aResourceExtensions[22] = "ifo";
    m_pResTypes[23] = 2015;
    m_aResourceExtensions[23] = "bic";
    m_pResTypes[24] = 2016;
    m_aResourceExtensions[24] = "wok";
    m_pResTypes[25] = 6;
    m_aResourceExtensions[25] = "plt";
    m_pResTypes[26] = 2005;
    m_aResourceExtensions[26] = "fnt";
    m_pResTypes[27] = 2017;
    m_aResourceExtensions[27] = "2da";
    m_pResTypes[28] = 2018;
    m_aResourceExtensions[28] = "tlk";
    m_pResTypes[29] = 8;
    m_aResourceExtensions[29] = "mp3";
    m_pResTypes[30] = 9;
    m_aResourceExtensions[30] = "mpg";
    m_pResTypes[31] = 2024;
    m_aResourceExtensions[31] = "bti";
    m_pResTypes[32] = 2025;
    m_aResourceExtensions[32] = "uti";
    m_pResTypes[33] = 2026;
    m_aResourceExtensions[33] = "btc";
    m_pResTypes[34] = 2027;
    m_aResourceExtensions[34] = "utc";
    m_pResTypes[35] = 2031;
    m_aResourceExtensions[35] = "btt";
    m_pResTypes[36] = 2032;
    m_aResourceExtensions[36] = "utt";
    m_pResTypes[37] = 2023;
    m_aResourceExtensions[37] = "git";
    m_pResTypes[38] = 2029;
    m_aResourceExtensions[38] = "dlg";
    m_pResTypes[39] = 2030;
    m_aResourceExtensions[39] = "itp";
    m_pResTypes[40] = 2033;
    m_aResourceExtensions[40] = "dds";
    m_pResTypes[41] = 2034;
    m_aResourceExtensions[41] = "bts";
    m_pResTypes[42] = 2035;
    m_aResourceExtensions[42] = "uts";
    m_pResTypes[43] = 2036;
    m_aResourceExtensions[43] = "ltr";
    m_pResTypes[44] = 2037;
    m_aResourceExtensions[44] = "gff";
    m_pResTypes[45] = 2038;
    m_aResourceExtensions[45] = "fac";
    m_pResTypes[46] = 2039;
    m_aResourceExtensions[46] = "bte";
    m_pResTypes[47] = 2040;
    m_aResourceExtensions[47] = "ute";
    m_pResTypes[48] = 2041;
    m_aResourceExtensions[48] = "btd";
    m_pResTypes[49] = 2042;
    m_aResourceExtensions[49] = "utd";
    m_pResTypes[50] = 2043;
    m_aResourceExtensions[50] = "btp";
    m_pResTypes[51] = 2044;
    m_aResourceExtensions[51] = "utp";
    m_pResTypes[52] = 2045;
    m_aResourceExtensions[52] = "dft";
    m_pResTypes[53] = 2046;
    m_aResourceExtensions[53] = "gic";
    m_pResTypes[54] = 2047;
    m_aResourceExtensions[54] = "gui";
    m_pResTypes[55] = 2048;
    m_aResourceExtensions[55] = "css";
    m_pResTypes[56] = 2049;
    m_aResourceExtensions[56] = "ccs";
    m_pResTypes[57] = 2050;
    m_aResourceExtensions[57] = "btm";
    m_pResTypes[58] = 2051;
    m_aResourceExtensions[58] = "utm";
    m_pResTypes[59] = 2052;
    m_aResourceExtensions[59] = "dwk";
    m_pResTypes[60] = 2053;
    m_aResourceExtensions[60] = "pwk";
    m_pResTypes[61] = 2054;
    m_aResourceExtensions[61] = "btg";
    m_pResTypes[62] = 2055;
    m_aResourceExtensions[62] = "utg";
    m_pResTypes[63] = 2056;
    m_aResourceExtensions[63] = "jrl";
    m_pResTypes[64] = 2057;
    m_aResourceExtensions[64] = "sav";
    m_pResTypes[65] = 2058;
    m_aResourceExtensions[65] = "utw";
    m_pResTypes[66] = 2059;
    m_aResourceExtensions[66] = "4pc";
    m_pResTypes[67] = 2060;
    m_aResourceExtensions[67] = "ssf";
    m_pResTypes[68] = 2061;
    m_aResourceExtensions[68] = "hak";
    m_pResTypes[69] = 3000;
    m_aResourceExtensions[69] = "lyt";
    m_pResTypes[70] = 3001;
    m_aResourceExtensions[70] = "vis";
    m_pResTypes[71] = 3002;
    m_aResourceExtensions[71] = "rim";
    m_pResTypes[72] = 11;
    m_aResourceExtensions[72] = "wma";
    m_pResTypes[73] = 3003;
    m_aResourceExtensions[73] = "pth";
    m_pResTypes[74] = 3005;
    m_aResourceExtensions[74] = "bwm";
    m_pResTypes[75] = 12;
    m_aResourceExtensions[75] = "wmv";
    m_pResTypes[76] = 3004;
    m_aResourceExtensions[76] = "lip";
    m_pResTypes[77] = 2062;
    m_aResourceExtensions[77] = "nwm";
    m_pResTypes[78] = 2063;
    m_aResourceExtensions[78] = "bik";
    m_pResTypes[79] = 3006;
    m_aResourceExtensions[79] = "txb";
    m_pResTypes[80] = 3007;
    m_aResourceExtensions[80] = "tpc";
    m_pResTypes[81] = 3008;
    m_aResourceExtensions[81] = "mdx";
    m_pResTypes[82] = 3009;
    m_aResourceExtensions[82] = "rsv";
    m_pResTypes[83] = 3010;
    m_aResourceExtensions[83] = "sig";
    m_pResTypes[84] = 13;
    m_aResourceExtensions[84] = "xmv";
    m_pResTypes[85] = 3011;
    m_aResourceExtensions[85] = "xbx";
#if KOTOR1  // K1 also registers the .log type at index 86; K2 leaves it unset
    m_pResTypes[86] = 14;
    m_aResourceExtensions[86] = "log";
#endif
    m_pResTypes[87] = 65535;
    m_aResourceExtensions[87] = "";
}

CExoBaseInternal::~CExoBaseInternal()
{
    delete[] m_pResTypes;
    delete[] m_aResourceExtensions;
}

// Registers one alias: reads "<sIniFile>/[Alias]/<sAlias>", falls back to sDefault when the ini has
// no such entry, then hands the result to the alias list. sRoot is unused.
void CExoBaseInternal::AddAlias(const CExoString& sRoot, const CExoString& sIniFile,
                                const CExoString& sAlias, const CExoString& sDefault)
{
    K2_UNUSED(sRoot);
    CExoString sPath;
    if (!g_pExoBase->m_pIni->ReadIniEntry(sPath, sIniFile, "Alias", sAlias))
        sPath = sDefault;
    g_pExoBase->m_pAliasList->Add(sAlias, sPath);
}

#if KOTOR1
// K1 lists the directory's entries from the patch OBB, else the main OBB (only when c == 0), else the
// "<external storage>/<dir>" directory if it exists, else the bare path. Entries keep their case.
int CExoBaseInternal::GetDirectoryList(CExoArrayList<CExoString>* pOut, const CExoString& sDir,
                                       unsigned short nResType, int a, int b, int c)
{
    CExoString sPath;
    CExoString sFileName;
    CExoString sExtension;

    sExtension = g_pExoBase->GetResourceExtension(nResType);
    if (sExtension == "" || a)
        sExtension = "*";

    sPath = g_pExoBase->m_pAliasList->ResolveFileName(sDir, RESTYPE_INVALID);
    std::string sDirPath(sPath.CStr());
    std::replace(sDirPath.begin(), sDirPath.end(), '\\', '/');
    std::string sFullPath;

    bool bObb = false;
    bool bExists = false;
    ObbFile* pObb = NULL;
    int nIndex = 0;
    if (c == 0) {
        if (g_patchObbMounted) {
            const char* pName = sDirPath.c_str();
            while (*pName == '.' || *pName == '/')
                pName++;
            nIndex = mz_zip_reader_locate_file(g_patchObb->GetZip(), pName, NULL, 0);
            if (nIndex >= 0) {
                pObb = g_patchObb;
                bObb = true;
            }
        }
        if (!bObb && g_obbMounted) {
            const char* pName = sDirPath.c_str();
            while (*pName == '.' || *pName == '/')
                pName++;
            nIndex = mz_zip_reader_locate_file(g_mainObb->GetZip(), pName, NULL, 0);
            if (nIndex >= 0) {
                pObb = g_mainObb;
                bObb = true;
            }
        }
    }

    DIR* pDir = NULL;
    std::vector<ObbEntry*>* pChildren = NULL;
    int nChildren = 0;
#ifdef K2_FIX_ENGINE_BUGS
    std::vector<ObbEntry*> aMerged;
#endif
    if (bObb) {
        ObbEntry* pDirEntry = pObb->m_ppEntries[nIndex];
        pChildren = &pDirEntry->m_aChildren;
#ifdef K2_FIX_ENGINE_BUGS
        // The patch OBB's directory hides the main OBB's of the same name, so list both. The
        // patch's entry wins a name they share.
        if (pObb == g_patchObb && g_obbMounted) {
            const char* pName = sDirPath.c_str();
            while (*pName == '.' || *pName == '/')
                pName++;
            int nMainIndex = mz_zip_reader_locate_file(g_mainObb->GetZip(), pName, NULL, 0);
            if (nMainIndex >= 0) {
                aMerged = *pChildren;
                for (ObbEntry* pMainEntry : g_mainObb->m_ppEntries[nMainIndex]->m_aChildren) {
                    bool bShadowed = false;
                    for (ObbEntry* pPatchEntry : *pChildren)
                        if (pPatchEntry->m_sName == pMainEntry->m_sName)
                            bShadowed = true;
                    if (!bShadowed)
                        aMerged.push_back(pMainEntry);
                }
                pChildren = &aMerged;
            }
        }
#endif
        nChildren = static_cast<int>(pChildren->size());
    } else {
        sFullPath = std::string(SDL_AndroidGetExternalStoragePath()) + "/" + sDirPath;
        bExists = access(sFullPath.c_str(), F_OK) == 0;
        pDir = opendir(bExists ? sFullPath.c_str() : sDirPath.c_str());
        if (!pDir)
            return 0;
    }

    CExoArrayList<unsigned long long> aFileTimes;
    int i;
    int nChild = 0;
    for (;;) {
        std::string sName;
        ObbEntry* pEntry K2_UNSET;
        dirent* pEnt K2_UNSET;
        if (bObb) {
            if (nChild >= nChildren)
                break;
            pEntry = (*pChildren)[nChild++];
            sName = pEntry->m_sName;
        } else {
            pEnt = readdir(pDir);
            if (!pEnt) {
                closedir(pDir);
                break;
            }
#ifdef K2_FIX_ENGINE_BUGS
            sName = pEnt->d_name;
#else
            sName = std::string(pEnt->d_name, sizeof(pEnt->d_name));
#endif
        }

        if (sExtension != "*") {
            std::string::size_type nDot = sName.rfind('.');
            if (nDot == std::string::npos && !a)
                continue;
            if (!sExtension.CompareNoCase(sName.substr(nDot + 1).c_str()))
                continue;
        }

        sFileName = sName.c_str();
        CExoString sLowerName = sFileName.LowerCase();
        if (a) {
            if ((bObb ? pEntry->m_bDirectory != 0 : pEnt->d_type == DT_DIR)
                && strcmp(sFileName.CStr(), ".") != 0
                && strcmp(sFileName.CStr(), "..") != 0)
            {
                if (b == 0) {
                    pOut->AddUnique(sFileName);
                } else {
                    for (i = 0; i < pOut->m_count; i++) {
                        if (pOut->m_pArray[i].LowerCase() >= sLowerName)
                            break;
                    }
                    pOut->Insert(sFileName, i);
                }
            }
        } else if (!(bObb ? pEntry->m_bDirectory != 0 : pEnt->d_type == DT_DIR)) {
            if (b == 2) {
                if (bObb) {
#ifdef K2_FIX_ENGINE_BUGS
                    for (i = 0; i < pOut->m_count; i++) {
                        if (pEntry->m_nTime > aFileTimes.m_pArray[i])
                            break;
                    }
#endif
                    aFileTimes.Insert(pEntry->m_nTime, i);
                    pOut->Insert(sFileName, i);
                } else {
                    struct stat st;
                    int nStat;
                    if (bExists)
                        nStat = stat((sFullPath + "/" + sName).c_str(), &st);
                    else
                        nStat = stat((sDirPath + "/" + sName).c_str(), &st);
#ifdef K2_FIX_ENGINE_BUGS
                    if (nStat == 0) {
#else
                    if (nStat != 0) {
#endif
                        unsigned long long nTime = st.st_mtime;
                        for (i = 0; i < pOut->m_count; i++) {
                            if (nTime > aFileTimes.m_pArray[i])
                                break;
                        }
                        aFileTimes.Insert(nTime, i);
                        pOut->Insert(sFileName, i);
                    }
                }
            } else if (b == 1) {
                int nLower = 0;
                int nUpper = pOut->m_count - 1;
                int nMid = nUpper / 2;
                while (nLower < nUpper) {
                    if (pOut->m_pArray[nMid].LowerCase() < sLowerName)
                        nLower = nMid + 1;
                    else
                        nUpper = nMid;
                    nMid = (nLower + nUpper) / 2;
                }
                if (nUpper >= 0 && pOut->m_pArray[nMid].LowerCase() < sLowerName)
                    pOut->Insert(sFileName, nMid + 1);
                else
                    pOut->Insert(sFileName, nMid);
            } else {
                pOut->AddUnique(sFileName);
            }
        }
    }
    return 1;
}
#else
// Lists "<resolved dir>[*.<ext>|\\*.<ext>]" into pOut. An empty extension becomes "*", the all-files
// wildcard, and so does any nonzero `a`, which doubles as the "list directories" mode:
//   a != 0  directories only, skipping "." and ".."; b == 0 appends, else inserts sorted by
//           lowercased name.
//   a == 0  files only; b == 1 binary-searches a lower_bound on the lowercased name, b == 2 inserts
//           by last-write time, anything else appends.
// Duplicates are detected on the lowercased name.
int CExoBaseInternal::GetDirectoryList(CExoArrayList<CExoString>* pOut, const CExoString& sDir,
                                       unsigned short nResType, int a, int b)
{
    CExoString sPath;
    CExoString sFileName;
    CExoString sExtension;
    WIN32_FIND_DATAA fd;

    sExtension = g_pExoBase->GetResourceExtension(nResType);
    if (sExtension == "" || a)
        sExtension = "*";

    sPath = g_pExoBase->m_pAliasList->ResolveFileName(sDir, RESTYPE_INVALID);
    if (sPath[(sPath.m_sString ? static_cast<int>(strlen(sPath.m_sString)) : 0) - 1] == '\\')
        sPath.Format(const_cast<char*>("%s*.%s"), sPath.CStr(), sExtension.CStr());
    else
        sPath.Format(const_cast<char*>("%s\\*.%s"), sPath.CStr(), sExtension.CStr());

    void* hFind = FindFirstFileA(sPath.CStr(), &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return 0;

    CExoArrayList<FILETIME> aFileTimes;
    unsigned int dwErr = 0;
    while (dwErr != ERROR_NO_MORE_FILES)
    {
        sFileName = fd.cFileName;
        sFileName = sFileName.LowerCase();
        if (!pOut->Contains(sFileName))
        {
            CExoString sLowerName = sFileName.LowerCase();

            if (a)
            {
                if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    && strcmp(sFileName.CStr(), ".") != 0
                    && strcmp(sFileName.CStr(), "..") != 0)
                {
                    if (b == 0)
                    {
                        pOut->Add(sFileName);
                    }
                    else
                    {
                        int i = 0;
                        for (; i < pOut->m_count; i++)
                        {
                            if (pOut->m_pArray[i].LowerCase() >= sLowerName)
                                break;
                        }
                        pOut->Insert(sFileName, i);
                    }
                }
            }
            else if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                if (b == 1)
                {
                    int nLower = 0;
                    int nUpper = pOut->m_count - 1;
                    int nMid = nUpper / 2;
                    while (nLower < nUpper)
                    {
                        if (pOut->m_pArray[nMid].LowerCase() < sLowerName)
                            nLower = nMid + 1;
                        else
                            nUpper = nMid;
                        nMid = (nLower + nUpper) / 2;
                    }
                    if (nUpper >= 0 && pOut->m_pArray[nMid].LowerCase() < sLowerName)
                        pOut->Insert(sFileName, nMid + 1);
                    else
                        pOut->Insert(sFileName, nMid);
                }
                else if (b == 2)
                {
                    int i = 0;
                    for (; i < pOut->m_count; i++)
                    {
                        if (CompareFileTime(&fd.ftLastWriteTime, &aFileTimes.m_pArray[i]) == 1)
                            break;
                    }
                    aFileTimes.Insert(fd.ftLastWriteTime, i);
                    pOut->Insert(sFileName, i);
                }
                else
                {
                    pOut->Add(sFileName);
                }
            }
        }

        if (!FindNextFileA(hFind, &fd))
            dwErr = GetLastError();
    }
    FindClose(hFind);
    return 1;
}
#endif

// Sums nFileSizeLow over every entry matching "<resolved dir>[*.*|\\*.*]". A path that already ends
// in a backslash must not get a second one.
unsigned int CExoBaseInternal::GetDirectorySize(const CExoString& sDir)
{
    WIN32_FIND_DATAA fd;
    CExoString sPath;
    CExoString sName;
    CExoString sSearch;
    void* hFind;

    sPath = g_pExoBase->m_pAliasList->ResolveFileName(sDir, RESTYPE_INVALID);

    if (sPath[sPath.GetLength() - 1] == '\\')
        sPath.Format(const_cast<char*>("%s*.*"), sPath.CStr());
    else
        sPath.Format(const_cast<char*>("%s\\*.*"), sPath.CStr());

    hFind = FindFirstFileA(sPath.CStr(), &fd);
#if KOTOR1  // K1's FindFirstFileA fails with null
    if (hFind == 0)
#else
    if (hFind == INVALID_HANDLE_VALUE)
#endif
        return 0;

    unsigned int nTotal = 0;
    unsigned int dwErr = 0;
    while (dwErr != ERROR_NO_MORE_FILES)
    {
        sName = fd.cFileName;
        unsigned int nSize = fd.nFileSizeLow;
        if (!FindNextFileA(hFind, &fd))
            dwErr = GetLastError();
        nTotal += nSize;
    }
    FindClose(hFind);
    return nTotal;
}

// Lists "<resolved dir>\\*.<ext>" into pOut. An empty extension becomes "*", the all-files wildcard.
// Subdirectories are skipped, and the number of entries added is returned.
int CExoBaseInternal::GetAugmentedDirectoryList(CExoArrayList<CFileInfo>* pOut,
                                                const CExoString& sDir,
                                                unsigned short nResType, int bSorted)
{
    WIN32_FIND_DATAA fd;
    CExoFileTime time;
    CExoString sFileName;
    CExoString sSearch;
    CExoString sUnused;
    CExoString sExt;

    sExt = g_pExoBase->GetResourceExtension(nResType);
    if (sExt == "")
        sExt = "*";

    sSearch = g_pExoBase->m_pAliasList->ResolveFileName(sDir, RESTYPE_INVALID);
    sSearch.Format(const_cast<char*>("%s\\*.%s"), sSearch.CStr(), sExt.CStr());

    void* hFind = FindFirstFileA(sSearch.CStr(), &fd);
#if KOTOR1  // K1's port FindFirstFileA fails with NULL
    if (hFind == 0)
#else
    if (hFind == INVALID_HANDLE_VALUE)
#endif
        return 0;

    int nAdded = 0;
    unsigned int dwErr = 0;
    do
    {
        sFileName = fd.cFileName;

        FILETIME ftLocal;
        SYSTEMTIME st;
        FileTimeToLocalFileTime(&fd.ftLastWriteTime, &ftLocal);
        FileTimeToSystemTime(&ftLocal, &st);

        time.m_nYear = st.wYear;
        time.m_nMonth = st.wMonth;
        time.m_nDay = st.wDay;
        time.m_nHour = st.wHour;
        time.m_nMinute = st.wMinute;
        time.m_nSecond = st.wSecond;

        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            if (bSorted)
            {
                int i = 0;
                for (i = 0; i < pOut->m_count; i++)
                {
                    if (pOut->m_pArray[i].m_name >= sFileName)
                        break;
                }
                pOut->Insert(CFileInfo{ sFileName, time }, i);
            }
            else
            {
                pOut->Add(CFileInfo{ sFileName, time });
            }
            nAdded++;
        }

        if (!FindNextFileA(hFind, &fd))
            dwErr = GetLastError();
    } while (dwErr != ERROR_NO_MORE_FILES);

    FindClose(hFind);
    return nAdded;
}

// res-type to extension string, by linear scan of m_pResTypes.
const CExoString& CExoBaseInternal::GetResourceExtension(unsigned short nResType)
{
    int64_t i;
    for (i = 0; i < m_nResTypeCount; i++)
    {
        if (m_pResTypes[i] == nResType)
            break;
    }
    if (i >= m_nResTypeCount)
        i = RES_EXTENSION_FALLBACK;
    return m_aResourceExtensions[i];
}

// The inverse scan, by case-insensitive name.
unsigned short CExoBaseInternal::GetResTypeFromExtension(const CExoString& sExt)
{
    int64_t i;
    for (i = 0; i < m_nResTypeCount; i++)
    {
        if (m_aResourceExtensions[i].CompareNoCase(sExt) != 0)
            goto found;
    }
    i = RES_EXTENSION_FALLBACK;
found:
    return m_pResTypes[i];
}

// Builds the alias table: the install-relative base path once, then every built-in alias.
void CExoBaseInternal::LoadAliases(const CExoString& sPath)
{
    CExoString sBase;
    sBase.Format(const_cast<char*>(".\\%s"), sPath.CStr());

    {
        const CExoString sName("HD0");
        const CExoString sDir(".\\");
        AddAlias(CExoString("d:\\"), sBase, sName, sDir);
    }
    {
        const CExoString sName("CD0");
        const CExoString sDir("d:\\");
        AddAlias(CExoString("d:\\"), sBase, sName, sDir);
    }
    {
        const CExoString sName("OVERRIDE");
        const CExoString sDir(".\\override");
        AddAlias(CExoString("d:\\override"), sBase, sName, sDir);
    }
    {
        const CExoString sName("ERRORTEX");
        const CExoString sDir(".\\errortex");
        AddAlias(CExoString("d:\\errortex"), sBase, sName, sDir);
    }
    {
        const CExoString sName("TEMP");
        const CExoString sDir(".\\temp");
        AddAlias(CExoString("d:\\temp"), sBase, sName, sDir);
    }
    {
        const CExoString sName("MODULES");
        const CExoString sDir(".\\modules");
        AddAlias(CExoString("d:\\modules"), sBase, sName, sDir);
    }
    {
        const CExoString sName("NWMFILES");
        const CExoString sDir(".\\nwm");
        AddAlias(CExoString("d:\\nwm"), sBase, sName, sDir);
    }
    {
        const CExoString sName("LOGS");
        const CExoString sDir(".\\logs");
        AddAlias(CExoString("d:\\logs"), sBase, sName, sDir);
    }
    {
        const CExoString sName("LOCALVAULT");
        const CExoString sDir(".\\localvault");
        AddAlias(CExoString("d:\\localvault"), sBase, sName, sDir);
    }
    {
        const CExoString sName("DMVAULT");
        const CExoString sDir(".\\dmvault");
        AddAlias(CExoString("d:\\dmvault"), sBase, sName, sDir);
    }
    {
        const CExoString sName("SERVERVAULT");
        const CExoString sDir(".\\servervault");
        AddAlias(CExoString("d:\\servervault"), sBase, sName, sDir);
    }
    {
        const CExoString sName("SAVES");
        const CExoString sDir(".\\saves");
        AddAlias(CExoString("u:\\"), sBase, sName, sDir);
    }
    {
        const CExoString sName("MUSIC");
        const CExoString sDir(".\\music");
        AddAlias(CExoString("d:\\music"), sBase, sName, sDir);
    }
    {
        const CExoString sName("STREAMMUSIC");
        const CExoString sDir(".\\streammusic");
        AddAlias(CExoString("d:\\streammusic"), sBase, sName, sDir);
    }
    {
        const CExoString sName("MOVIES");
        const CExoString sDir(".\\movies");
        AddAlias(CExoString("d:\\movies"), sBase, sName, sDir);
    }
    {
        const CExoString sName("TEMPCLIENT");
        const CExoString sDir(".\\tempclient");
        AddAlias(CExoString("d:\\tempclient"), sBase, sName, sDir);
    }
#if KOTOR1  // K1 also registers CURRENTGAME here
    {
        const CExoString sName("CURRENTGAME");
        const CExoString sDir(".\\currentgame");
        AddAlias(CExoString("d:\\currentgame"), sBase, sName, sDir);
    }
#endif
    {
        const CExoString sName("HAK");
        const CExoString sDir(".\\hak");
        AddAlias(CExoString("d:\\hak"), sBase, sName, sDir);
    }
    {
        const CExoString sName("TEXTUREPACKS");
        const CExoString sDir(".\\texturepacks");
        AddAlias(CExoString("d:\\texturepacks"), sBase, sName, sDir);
    }
#if KOTOR1  // K1: streamed waves; no streamed voice or supermodels folders
    {
        const CExoString sName("STREAMWAVES");
        const CExoString sDir(".\\streamwaves");
        AddAlias(CExoString("d:\\streamwaves"), sBase, sName, sDir);
    }
#else
    {
        const CExoString sName("STREAMVOICE");
        const CExoString sDir(".\\streamvoice");
        AddAlias(CExoString("d:\\streamvoice"), sBase, sName, sDir);
    }
    {
        const CExoString sName("SUPERMODELS");
        const CExoString sDir(".\\supermodels");
        AddAlias(CExoString("d:\\supermodels"), sBase, sName, sDir);
    }
#endif
    {
        const CExoString sName("DOWNLOADS");
        const CExoString sDir(".\\downloads");
        AddAlias(CExoString("t:\\"), sBase, sName, sDir);
    }
    {
        const CExoString sName("OPTIONS");
        const CExoString sDir(".\\");
        AddAlias(CExoString("t:\\"), sBase, sName, sDir);
    }
    {
        const CExoString sName("AMBIENT");
        const CExoString sDir(".\\ambient");
        AddAlias(CExoString("d:\\ambient"), sBase, sName, sDir);
    }
    {
        const CExoString sName("PATCH");
        const CExoString sDir(".\\patch");
        AddAlias(CExoString("d:\\patch"), sBase, sName, sDir);
    }
    {
        const CExoString sName("PORTRAITS");
        const CExoString sDir(".\\portraits");
        AddAlias(CExoString("d:\\portraits"), sBase, sName, sDir);
    }
    {
        const CExoString sName("MUSIC");
        const CExoString sDir(".\\music");
        AddAlias(CExoString("d:\\music"), sBase, sName, sDir);
    }
    {
        const CExoString sName("CURRENTGAME");
        const CExoString sDir(".\\currentgame");
        AddAlias(CExoString("d:\\currentgame"), sBase, sName, sDir);
    }
    {
        const CExoString sName("GAMEINPROGRESS");
        const CExoString sDir(".\\gameinprogress");
        AddAlias(CExoString("z:\\gameinprogress"), sBase, sName, sDir);
    }
    {
        const CExoString sName("FUTUREGAME");
        const CExoString sDir(".\\futuregame");
        AddAlias(CExoString("z:\\futuregame"), sBase, sName, sDir);
    }
    {
        const CExoString sName("RIMS");
        const CExoString sDir(".\\rims");
        AddAlias(CExoString("d:\\rims"), sBase, sName, sDir);
    }
    {
        const CExoString sName("RIMSXBOX");
        const CExoString sDir(".\\rimsxbox");
        AddAlias(CExoString("d:\\rimsxbox"), sBase, sName, sDir);
    }
    {
        const CExoString sName("REBOOTDATA");
        const CExoString sDir(".\\rebootdata");
        AddAlias(CExoString("d:\\rebootdata"), sBase, sName, sDir);
    }
#if !KOTOR1  // K1 has no CACHE alias
    {
        const CExoString sName("CACHE");
        const CExoString sDir(".\\");
        AddAlias(CExoString("z:\\cache"), sBase, sName, sDir);
    }
#endif
    {
        const CExoString sName("LIPS");
        const CExoString sDir(".\\lips");
        AddAlias(CExoString("d:\\lips"), sBase, sName, sDir);
    }
#if !KOTOR1  // K1 has no DOCUMENTS or FULLPATH alias
    {
        const CExoString sName("DOCUMENTS");
        const CExoString sDir(".");
        AddAlias(CExoString("f:\\"), sBase, sName, sDir);
    }
    {
        const CExoString sName("FULLPATH");
        const CExoString sDir("");
        AddAlias(CExoString(""), sBase, sName, sDir);
    }
#endif
}

// Empty in the shipped build.
void CExoBaseInternal::ShutDown()
{
}

// Launches a child process and, when both bQuit and the launch succeeded, asks the message loop to
// quit. The launch itself happens whatever bQuit says.
void CExoBaseInternal::SpawnExternalApplication(const CExoString& sPath,
                                                const CExoString& sArgs, int bQuit)
{
    STARTUPINFOA si = {};
    PROCESS_INFORMATION pi;


    int bLaunched = CreateProcessA(sPath.CStr(), sArgs.CStr(), 0, 0, 0, 0, 0,
                                   0, &si, &pi);
#ifdef K2_FIX_ENGINE_BUGS
    if (bLaunched) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
#endif
    if (bQuit && bLaunched)
        PostQuitMessage(0);
}

// Converts at most n wide chars into dst with wctomb, writing a substitute byte for a character that
// fails to convert and for every control char.
int CExoBaseInternal::WcsToMbs(char* dst, const K2WChar* src, uint64_t n)
{
    uint64_t nLen = wcslen(src);
    uint64_t nLimit = (nLen < n) ? nLen : n;
    if (nLimit == 0)
        return 1;

    int nOut = 0;
    for (uint64_t i = 0; i < nLimit; i++)
    {
        int nChar = wctomb(dst + nOut, src[i]);
        if (nChar == -1 || static_cast<uint64_t>(src[i] - 1) <= 30)
        {
            dst[nOut] = UNPRINTABLE_CHAR;
            nOut++;
        }
        else
        {
            nOut += nChar;
        }
    }
    return 1;
}

// Locates the install CD: reads the CD0 alias out of swkotor2.ini (taking only its first character,
// the drive letter), then probes drives C..Z for the first one that is a CD-ROM and carries every file
// of the retail disc's root set. A drive that has them all wins: CD0 is written into the ini if it was
// not already there, and the call returns 1.
int CExoBaseInternal::CheckForCD()
{
    CExoString sCdPath;
    CExoString sFileName;
    CExoString sDrivePath;
    CExoArrayList<CExoString> aCdFiles;
    WIN32_FIND_DATAA fd;

    int bIni = g_pExoBase->m_pIni->ReadIniEntry(sCdPath, EXO_GAME_INI, "Alias", "CD0");
    if (bIni)
        sCdPath = sCdPath.Left(1);

    aCdFiles.Add("data1.cab");
    aCdFiles.Add("data1.hdr");
    aCdFiles.Add("data2.cab");
    aCdFiles.Add("layout.bin");
    aCdFiles.Add("setup.exe");
    aCdFiles.Add("setup.ini");
    aCdFiles.Add("data");
    aCdFiles.Add("readme-kotor.rtf");

    int bRet = 0;
    int nDrive = 2;
    while (nDrive < 26)
    {
        if (bIni)
        {
            sCdPath.Format(const_cast<char*>("%s:\\*.*"), sCdPath.CStr());
            nDrive--;
        }
        else
        {
            sCdPath.Format(const_cast<char*>("%c:\\*.*"), nDrive + 'A');
        }

        sDrivePath = sCdPath.Left(3);
        if (GetDriveTypeA(sDrivePath.CStr()) == DRIVE_CDROM)
        {
            void* hFind = FindFirstFileA(sCdPath.CStr(), &fd);
#if KOTOR1  // K1's FindFirstFileA fails with null
            if (hFind != 0)
#else
            if (hFind != INVALID_HANDLE_VALUE)
#endif
            {
                int nFound = 0;
                unsigned int dwFindErr = 0;
                while (dwFindErr != ERROR_NO_MORE_FILES)
                {
                    sFileName = fd.cFileName;
                    sFileName = sFileName.LowerCase();
                    if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                        || (strcmp(sFileName.CStr(), ".") != 0
                            && strcmp(sFileName.CStr(), "..") != 0))
                    {
                        if (aCdFiles.Contains(sFileName))
                            nFound++;
                    }
                    if (!FindNextFileA(hFind, &fd))
                        dwFindErr = GetLastError();
                }
                FindClose(hFind);

                if (nFound == aCdFiles.m_count)
                {
                    if (!g_pExoBase->m_pIni->ReadIniEntry(sFileName, EXO_GAME_INI, "Alias", "CD0"))
                        g_pExoBase->m_pIni->WriteIniEntry(sCdPath.Left(3), EXO_GAME_INI, "Alias", "CD0");
                    bRet = 1;
                    break;
                }
            }
        }

        bRet = 0;
        bIni = 0;
        nDrive++;
    }
    return bRet;
}
