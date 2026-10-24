#include "k2stdint.h"
#include <stdio.h>
#if KOTOR1
#include <string>
#include <algorithm>
#endif
#include "exofileinternal.h"
#include "exobase.h"
#include "port/win32const.h"

CExoFileThread* g_ExoFileThread;

#include "port/libcstring.h"

// Length of a CExoString without going through GetLength(), so a call's result is not evaluated
// twice.
static inline int sLen(const CExoString& s)
{
#if KOTOR1  // K1: CExoString::GetLength is out of line
    return s.GetLength();
#else
    return s.m_sString ? static_cast<int>(strlen(s.m_sString)) : 0;
#endif
}

CExoAliasListInternal::CExoAliasListInternal()
    : m_pList(0)
{
    m_sNotFound = "";
}

CExoAliasListInternal::~CExoAliasListInternal()
{
    for (CExoAliasListEntry* p = m_pList; p != 0; )
    {
        CExoAliasListEntry* pNext = p->m_pNext;
        delete p;
        p = pNext;
    }
}

// Inserts in sorted order. The alias is upper-cased before it becomes the sort key, and the stored
// path always ends in a separator.
void CExoAliasListInternal::Add(const CExoString& alias, const CExoString& path)
{
    CExoString sAlias(alias);
    CExoString sPath(path);
    if (sPath == "")
        return;

    sAlias = sAlias.UpperCase();

    if (sPath.CStr()[sLen(sPath) - 1] != '\\')
        sPath = sPath + "\\";

    CExoAliasListEntry* pPrev = 0;
    CExoAliasListEntry* pCur = m_pList;
    for (; pCur != 0; pCur = pCur->m_pNext)
    {
        if (pCur->m_sAlias > sAlias)
            break;
        pPrev = pCur;
    }

    CExoAliasListEntry* pNew = new CExoAliasListEntry;
    pNew->m_sAlias = sAlias;
    pNew->m_sPath = sPath;

    if (m_pList == pCur)
        m_pList = pNew;
    pNew->m_pNext = pCur;
    pNew->m_pPrev = pPrev;
    if (pPrev != 0)
        pPrev->m_pNext = pNew;
    if (pCur != 0)
        pCur->m_pPrev = pNew;
}

// Re-points an existing entry's path rather than inserting a second one for the same alias.
void CExoAliasListInternal::ChangePath(const CExoString& alias, const CExoString& path)
{
    CExoString sAlias(alias);
    CExoString sPath(path);
    if (sPath == "")
        return;

    sAlias = sAlias.UpperCase();

    char* pPathStr = static_cast<char*>(sPath.CStr());
    if (pPathStr[sLen(sPath) - 1] != '\\')
        sPath = sPath + "\\";

    for (CExoAliasListEntry* pCur = m_pList; pCur != 0; pCur = pCur->m_pNext)
    {
        if (pCur->m_sAlias == sAlias)
        {
            pCur->m_sPath = sPath;
            return;
        }
    }
    Add(sAlias, sPath);
}

void CExoAliasListInternal::Clear()
{
    CExoAliasListEntry* pEntry = m_pList;
    if (pEntry == 0)
        return;

    do
    {
        CExoAliasListEntry* pNext = pEntry->m_pNext;
        delete pEntry;
        pEntry = pNext;
        m_pList = 0;
    } while (pEntry != 0);
}

// CompareNoCase returns non-zero when the strings match.
void CExoAliasListInternal::Delete(const CExoString& alias)
{
    CExoString sAlias(alias);
    if (sAlias == "")
        return;

    CExoAliasListEntry* pPrev = 0;
    CExoAliasListEntry* pCur = m_pList;
    while (pCur != 0)
    {
        CExoAliasListEntry* pNext = pCur->m_pNext;
        if (pCur->m_sAlias.CompareNoCase(sAlias) != 0)
        {
            if (m_pList == pCur)
                m_pList = pCur->m_pNext;
            if (pPrev != 0)
                pPrev->m_pNext = pNext;
            if (pNext != 0)
                pNext->m_pPrev = pPrev;
            break;
        }
        pPrev = pCur;
        pCur = pCur->m_pNext;
    }
    delete pCur;
}

// flag is a prefix length; 0 means the whole alias. The empty alias never matches, and the empty
// string is also the miss value.
const CExoString& CExoAliasListInternal::GetAliasPath(const CExoString& alias, int flag) const
{
    if (alias == "")
        return m_sNotFound;

    if (flag == 0)
        flag = sLen(alias);

    for (CExoAliasListEntry* pCur = m_pList; pCur != 0; pCur = pCur->m_pNext)
    {
        if (pCur->m_sAlias.ComparePrefixNoCase(alias, flag) == 1)
            return pCur->m_sPath;
    }
    return m_sNotFound;
}

// "alias:sub/dir/file" + a resource type -> alias path + "/sub/dir/file.ext". The alias is everything
// up to the first ':'; a bare path or a leading ':' yields "". A doubled separator in the tail
// ("alias:\\sub") is skipped, and the extension is appended only when the resource type has one.
CExoString CExoAliasListInternal::ResolveFileName(const CExoString& path, unsigned short resType) const
{
    CExoString sResult;

    int nPos = path.Find(':', 0);
    if (nPos <= 0)
        return CExoString("");
#if KOTOR1  // K1 returns "" for an alias that resolves to an empty path
    if (g_pExoBase->m_pAliasList->GetAliasPath(path, nPos).GetLength() == 0)
        return CExoString("");
#endif

    int nSkip = nPos;
    if (path.CStr()[nPos + 1] == '\\')
        nSkip = nPos + 1;

    if (sLen(g_pExoBase->GetResourceExtension(resType)) != 0)
    {
        sResult.Format(const_cast<char*>("%s%s.%s"), g_pExoBase->m_pAliasList->GetAliasPath(path, nPos).CStr(),
                       path.CStr() + nSkip + 1, g_pExoBase->GetResourceExtension(resType).CStr());
    }
    else
    {
        sResult.Format(const_cast<char*>("%s%s"), g_pExoBase->m_pAliasList->GetAliasPath(path, nPos).CStr(),
                       path.CStr() + nSkip + 1);
    }
    return sResult;
}

extern "C" {
    int   ASL_ferror(FILE* f);
    int   ASL_feof(FILE* f);
}
#if KOTOR1
#include "port/k1obb.h"
bool HasTouchScreen();
#include "port/win32api.h"

CExoString g_sErrorString;
#endif

CExoFileInternal::CExoFileInternal(const CExoString& path, unsigned short resType, const CExoString& mode)
{
#if KOTOR1  // K1 tries the "?z?" compressed twin for 9997/9998, a "tv_" name off-touch, then the OBBs
    m_pFile = NULL;
    m_mode = mode;
    m_path = g_pExoBase->m_pAliasList->ResolveFileName(path, resType);
    std::string sPath(m_path.CStr());
    std::replace(sPath.begin(), sPath.end(), '\\', '/');
    m_bCompressed = 0;
    std::string sCompressed(sPath);
    std::string sTv(sPath);
    bool bCompressed = false;
    bool bTv = false;
    if (static_cast<unsigned short>(resType - 9997) < 2) {
        sCompressed.replace(sCompressed.size() - 2, 1, "z", 1);
        m_pFile = SDL_RWFromFile(sCompressed.c_str(), mode.CStr());
        bCompressed = true;
        if (m_pFile) {
            m_bCompressed = 1;
            return;
        }
    } else {
        static bool s_bHasTouchScreen = HasTouchScreen();
        if (!s_bHasTouchScreen && mode.CompareNoCase("rb")) {
            size_t nSlash = sTv.rfind('/');
            if (nSlash == std::string::npos)
                sTv.insert(0, "tv_", 3);
            else
                sTv.insert(nSlash + 1, "tv_", 3);
            m_pFile = SDL_RWFromFile(sTv.c_str(), mode.CStr());
            if (m_pFile)
                return;
            bTv = true;
        }
    }
    m_pFile = SDL_RWFromFile(sPath.c_str(), mode.CStr());
    if (m_pFile)
        return;
    if (g_patchObbMounted) {
        if (bCompressed) {
            m_pFile = g_patchObb->RWFromFile(sCompressed.c_str());
            if (m_pFile) {
                m_bCompressed = 1;
                return;
            }
        } else if (bTv) {
            m_pFile = g_patchObb->RWFromFile(sTv.c_str());
            if (m_pFile)
                return;
        }
        m_pFile = g_patchObb->RWFromFile(sPath.c_str());
        if (m_pFile)
            return;
    }
    if (g_obbMounted) {
        if (bCompressed) {
            m_pFile = g_mainObb->RWFromFile(sCompressed.c_str());
            if (m_pFile) {
                m_bCompressed = 1;
                return;
            }
        } else if (bTv) {
            m_pFile = g_mainObb->RWFromFile(sTv.c_str());
            if (m_pFile)
                return;
        }
        m_pFile = g_mainObb->RWFromFile(sPath.c_str());
    }
#else
    CExoString resolvedPath;
    m_pFile = NULL;
    resolvedPath = g_pExoBase->m_pAliasList->ResolveFileName(path, resType);
    m_pFile = fopen(resolvedPath.CStr(), mode.CStr());
    m_path = resolvedPath;
#endif
}

CExoFileInternal::~CExoFileInternal()
{
#if KOTOR1
    if (m_pFile) {
        SDL_RWclose(m_pFile);
        m_pFile = NULL;
    }
#else
    if (m_pFile) {
        fclose(m_pFile);
        m_pFile = NULL;
    }
#endif
}

#if KOTOR1
bool CExoFileInternal::Flush()
{
    if (m_pFile && m_pFile->type == SDL_RWOPS_STDFILE)
        return fflush(static_cast<FILE*>(m_pFile->hidden.stdio.fp)) == 0;
    return false;
}
#endif

unsigned int CExoFileInternal::GetOffset()
{
#if KOTOR1
    if (m_pFile) return static_cast<unsigned int>(SDL_RWtell(m_pFile));
    return 0;
#else
    if (m_pFile) return static_cast<unsigned int>(ftell(m_pFile));
    return 0;
#endif
}

int CExoFileInternal::GetSize()
{
#if KOTOR1
    if (m_pFile) return static_cast<int>(SDL_RWsize(m_pFile));
    return -1;
#else
    if (!m_pFile) return static_cast<unsigned int>(-1);
    fflush(m_pFile);
    int pos = static_cast<int>(ftell(m_pFile));
    if (pos < 0) return static_cast<unsigned int>(-1);
    fseek(m_pFile, 0, 2);
    int size = static_cast<int>(ftell(m_pFile));
    fseek(m_pFile, pos, 0);
    return static_cast<unsigned int>(size);
#endif
}

unsigned int CExoFileInternal::Read(void* pBuffer, unsigned int elemSize, unsigned int count)
{
#if KOTOR1  // K1 reopens and retries a short read, logging to g_sErrorString
    if (!m_pFile)
        return 0;
    int nPos = static_cast<int>(SDL_RWtell(m_pFile));
    unsigned int nRead = static_cast<unsigned int>(SDL_RWread(m_pFile, pBuffer, elemSize, count) * elemSize);
    unsigned int nWant = count * elemSize;
    if (nRead == nWant)
        return nRead;
    for (int nAttempt = 0; nAttempt < 10; nAttempt++) {
        g_sErrorString.Format(const_cast<char*>("%sFAILED READ (%s) - ATTEMPT %d - Asked for %d bytes got %d bytes\n"),
                              m_path.CStr(), g_sErrorString.CStr(), nAttempt, nWant, nRead);
        Sleep(500);
        if (m_pFile) {
            SDL_RWclose(m_pFile);
            m_pFile = NULL;
        } else {
            g_sErrorString.Format(const_cast<char*>("%sFile handle was null\n"), g_sErrorString.CStr(), nAttempt);
        }
        m_pFile = SDL_RWFromFile(m_path.CStr(), m_mode.CStr());
        if (m_pFile) {
            SDL_RWseek(m_pFile, nPos, RW_SEEK_SET);
            nRead = static_cast<unsigned int>(SDL_RWread(m_pFile, pBuffer, elemSize, count) * elemSize);
        } else {
            g_sErrorString.Format(const_cast<char*>("%sFile handle failed to reopen\n"), g_sErrorString.CStr(), nAttempt);
        }
        if (nRead == nWant)
            break;
    }
    g_sErrorString.Format(const_cast<char*>(nRead == nWant ? "%sFile write recovered.\n" : "%sFile write failed.  Giving up\n"),
                          g_sErrorString.CStr());
    return nRead;
#else
    if (m_pFile) {
        uint64_t nRead = fread(pBuffer, elemSize, count, m_pFile);
        if (nRead == count || (ASL_ferror(m_pFile) == 0 && ASL_feof(m_pFile) == 0)) {
            return static_cast<unsigned int>(nRead) * elemSize;
        }
        // CStr() is called for its side effect only.
        m_path.CStr();
        clearerr(m_pFile);
    }
    return 0;
#endif
}

unsigned int CExoFileInternal::Read(CExoString* out, unsigned int count)
{
#if KOTOR1
    SDL_RWops* f = m_pFile;
    if (!f)
        return 0;
    char* pBuffer = new char[count + 1];
    unsigned int nRead = static_cast<unsigned int>(SDL_RWread(f, pBuffer, 1, count));
    pBuffer[nRead] = '\0';
    *out = pBuffer;
    delete[] pBuffer;
    return nRead;
#else
    FILE* f = m_pFile;
    if (f) {
        char* pBuffer = new char[count + 1];
        unsigned int nRead = static_cast<unsigned int>(fread(pBuffer, 1, count, f));
        if (nRead == count || (ASL_ferror(m_pFile) == 0 && ASL_feof(m_pFile) == 0)) {
            pBuffer[nRead] = '\0';
            *out = pBuffer;
            operator delete[](pBuffer);
            return nRead;
        }
        // See Read(void*, ...) above.
        m_path.CStr();
        clearerr(m_pFile);
    }
    return 0;
#endif
}

#if KOTOR1
unsigned int CExoFileInternal::Write(const void* pBuffer, unsigned int elemSize, unsigned int count)
{
    if (!m_pFile)
        return 0;
    int nPos = static_cast<int>(SDL_RWtell(m_pFile));
    unsigned int nWritten = static_cast<unsigned int>(SDL_RWwrite(m_pFile, pBuffer, elemSize, count) * elemSize);
    unsigned int nWant = count * elemSize;
    if (nWritten == nWant)
        return nWritten;
    for (int nAttempt = 0; nAttempt < 10; nAttempt++) {
        g_sErrorString.Format(const_cast<char*>("%sFAILED WRITE (%s) - ATTEMPT %d - Tried to write %d bytes at position %d, "
                                                "actually wrote %d bytes\n"),
                              m_path.CStr(), g_sErrorString.CStr(), nAttempt, nWant, nPos, nWritten);
        Sleep(500);
        if (m_pFile) {
            SDL_RWclose(m_pFile);
            m_pFile = NULL;
        } else {
            g_sErrorString.Format(const_cast<char*>("%sFile handle was null\n"), g_sErrorString.CStr());
        }
        m_pFile = SDL_RWFromFile(m_path.CStr(), m_mode.CStr());
        if (m_pFile) {
            SDL_RWseek(m_pFile, nPos, RW_SEEK_SET);
            nWritten = static_cast<unsigned int>(SDL_RWwrite(m_pFile, pBuffer, elemSize, count) * elemSize);
        } else {
            g_sErrorString.Format(const_cast<char*>("%sFile handle failed to reopen\n"), g_sErrorString.CStr());
        }
        if (nWritten == nWant)
            break;
    }
    g_sErrorString.Format(const_cast<char*>(nWritten == nWant ? "%sFile write recovered.\n" : "%sFile write failed.  Giving up\n"),
                          g_sErrorString.CStr());
    return nWritten;
}
#else
unsigned int CExoFileInternal::Write(const char* str)
{
    FILE* f = m_pFile;
    if (!f) return 0;
    uint64_t len = strlen(str);
    return static_cast<unsigned int>(fwrite(str, 1, len, f));
}
#endif

void CExoFileInternal::ReadAsync(void* pBuffer, unsigned int elemSize, unsigned int count)
{
#if KOTOR1
    CExoFileThread* pReq = g_ExoFileThread;
    SDL_RWops* f = m_pFile;
    pReq->m_lock.EnterCriticalSection();
    void* hThread = pReq->m_hThread;
    pReq->m_bComplete = 0;
    pReq->m_pBuffer = pBuffer;
    pReq->m_pFile = f;
    pReq->m_elemSize = elemSize;
    pReq->m_count = count;
    ResumeThread(hThread);
    pReq->m_lock.LeaveCriticalSection();
#else
    CExoFileThread* pReq = g_ExoFileThread;
    FILE* f = m_pFile;
    pReq->m_lock.EnterCriticalSection();
    void* hThread = pReq->m_hThread;
    pReq->m_bComplete = 0;
    pReq->m_pBuffer = pBuffer;
    pReq->m_pFile = f;
    pReq->m_elemSize = elemSize;
    pReq->m_count = count;
    ResumeThread(hThread);
    pReq->m_lock.LeaveCriticalSection();
#endif
}

unsigned int CExoFileInternal::ReadAsyncComplete()
{
    return g_ExoFileThread->m_bComplete;
}

unsigned int CExoFileInternal::ReadAsyncBytesRead()
{
    return g_ExoFileThread->m_bytesRead;
}

#include "port/win32api.h"

CExoFileThread::CExoFileThread()
{
    m_bComplete = 0;
    m_bShutDown = 0;
    m_pFile = 0;
    m_elemSize = 0;
    m_count = 0;
    m_bytesRead = 0;
    m_pBuffer = 0;
    unsigned int nThreadId = 0;
    // The parameter and thread-id arguments are swapped; the thread id lands in m_nThreadId, and
    // the worker ignores the zeroed local it is handed.
    m_hThread = CreateThread(0, 0, AsyncReadThreadProc, &nThreadId, 4, reinterpret_cast<unsigned int*>(&m_nThreadId));
    SetThreadPriority(m_hThread, -2);
}

CExoFileThread::~CExoFileThread()
{
    m_bShutDown = 1;
    ResumeThread(m_hThread);
#if KOTOR1  // K1 uses NULL for an invalid handle
    if (m_hThread)
#else
    if (m_hThread != reinterpret_cast<void*>(-1))
#endif
    {
        WaitForSingleObject(m_hThread, INFINITE);
        CloseHandle(m_hThread);
    }
}

uint64_t AsyncReadThreadProc(void* pArg)
{
    K2_UNUSED(pArg);
    if (g_ExoFileThread->m_bShutDown)
        return 1;
    for (;;)
    {
        g_ExoFileThread->m_lock.EnterCriticalSection();
        CExoFileThread* p = g_ExoFileThread;
        p->m_lock.EnterCriticalSection();
        p->m_bytesRead = 0;
        if (p->m_pFile != 0)
        {
#if KOTOR1  // K1 reports the element count, without the elemSize scale
            p->m_bytesRead = static_cast<unsigned int>(SDL_RWread(p->m_pFile, p->m_pBuffer, p->m_elemSize, p->m_count));
#else
            p->m_bytesRead =
                p->m_elemSize *
                static_cast<unsigned int>(fread(p->m_pBuffer, p->m_elemSize, p->m_count, p->m_pFile));
#endif
        }
        p->m_bComplete = 1;
        p->m_pBuffer = 0;
        p->m_pFile = 0;
        p->m_elemSize = 0;
        p->m_count = 0;
        p->m_lock.LeaveCriticalSection();
        g_ExoFileThread->m_lock.LeaveCriticalSection();
        SuspendThread(g_ExoFileThread->m_hThread);
        if (g_ExoFileThread->m_bShutDown)
            return 1;
    }
}

void CExoFileThread::AsyncRead()
{
    m_lock.EnterCriticalSection();
    m_bytesRead = 0;
    if (m_pFile != 0)
    {
#if KOTOR1  // K1 reports the element count, without the elemSize scale
        m_bytesRead = static_cast<unsigned int>(SDL_RWread(m_pFile, m_pBuffer, m_elemSize, m_count));
#else
        m_bytesRead = m_elemSize *
                      static_cast<unsigned int>(fread(m_pBuffer, m_elemSize, m_count, m_pFile));
#endif
    }
    m_bComplete = 1;
    m_pBuffer = 0;
    m_pFile = 0;
    m_elemSize = 0;
    m_count = 0;
    m_lock.LeaveCriticalSection();
}

#if KOTOR1
void CExoFileThread::Read(void* pBuffer, unsigned int nElemSize, unsigned int nCount,
                          SDL_RWops* pFile)
#else
void CExoFileThread::Read(void* pBuffer, unsigned int nElemSize, unsigned int nCount,
                          FILE* pFile)
#endif
{
    m_lock.EnterCriticalSection();
    m_bComplete = 0;
    m_pBuffer = pBuffer;
    m_pFile = pFile;
    m_elemSize = nElemSize;
    m_count = nCount;
    ResumeThread(m_hThread);
    m_lock.LeaveCriticalSection();
}
