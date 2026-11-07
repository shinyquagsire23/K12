#pragma once
#include "k2stdint.h"

#include "exostring.h"

// The alias-path resolver behind CExoAliasList. Both special members are declared so they stay
// out of line.
class CExoAliasListInternal {
public:
    CExoAliasListInternal();
    ~CExoAliasListInternal();

    void Add(const CExoString& alias, const CExoString& path);
    void ChangePath(const CExoString& alias, const CExoString& path);
    void Clear();
    void Delete(const CExoString& alias);
    const CExoString& GetAliasPath(const CExoString& alias, int flag) const;
    CExoString ResolveFileName(const CExoString& path, unsigned short resType) const;

private:
    // One alias table entry. The list is doubly linked: Add/Delete splice through both links, while
    // the destructor and Clear only follow m_pNext.
    struct CExoAlias {
        CExoString m_sAlias;
        CExoString m_sPath;
        CExoAlias* m_pNext;
        CExoAlias* m_pPrev;
    };

    CExoString m_sNotFound;  // the miss value
    CExoAlias* m_pList;      // the chain head
};

#include "exocritsect.h"

#include "port/libcbase.h"
#if KOTOR1
#include "sdlrwops.h"
#endif

// The base-lib asynchronous file-read worker slot. One global instance backs every async read in the
// engine: a reader fills the slot and resumes the thread, the worker blocks on m_lock, reads into
// m_pBuffer and reports through m_bytesRead/m_bComplete. Only one request is outstanding at a time.
class CExoFileThread {
public:
    CExoFileThread();
    ~CExoFileThread();

    void*               m_hThread;
    unsigned int        m_nThreadId;
    CExoCriticalSection m_lock;
    unsigned int        m_bComplete;  // the worker sets 1 when a read finishes
    unsigned int        m_bShutDown;  // raised by the destructor, re-tested by the worker
#if KOTOR1  // K1: an SDL_RWops request; the two counts are read back sign-extended
    SDL_RWops*          m_pFile;
    int                 m_elemSize;
    int                 m_count;
#else
    FILE*               m_pFile;
    unsigned int        m_elemSize;
    unsigned int        m_count;
#endif
    unsigned int        m_bytesRead;  // elemSize * the bytes read
    void*               m_pBuffer;

    // Reads and AsyncRead run the whole request body themselves under m_lock, synchronously.
#if KOTOR1
    void Read(void* pBuffer, unsigned int nElemSize, unsigned int nCount, SDL_RWops* pFile);
#else
    void Read(void* pBuffer, unsigned int nElemSize, unsigned int nCount, FILE* pFile);
#endif
    void AsyncRead();
};

// The worker, C++ linkage.
uint64_t AsyncReadThreadProc(void* pArg);

// The one global worker slot, reachable from every translation unit.
extern CExoFileThread* g_ExoFileThread;

#include "exofile.h"
#include "exobase.h"
#include <stddef.h>

// The public CExoFile plus the implementation types its bodies need, split out of CExoFile.h so that
// consumer TUs can include the public class alone.

// The implementation behind CExoFile's public surface, a file-handle wrapper.
class CExoFileInternal {
public:
    CExoFileInternal(const CExoString& path, unsigned short resType, const CExoString& mode);
    ~CExoFileInternal();

    unsigned int GetOffset();
    int          GetSize();  // signed: CExoResMan::FileCopy sign-extends it
    unsigned int Read(void* pBuffer, unsigned int elemSize, unsigned int count);
    unsigned int Read(CExoString* out, unsigned int count);
#if KOTOR1
    unsigned int Write(const void* pBuffer, unsigned int elemSize, unsigned int count);
    bool         Flush();
#else
    unsigned int Write(const char* str);
    // K2: header inlines (weak in the armv7 image, which keeps them out of line behind CExoFile).
    int          Eof();
    bool         Flush();
    int          Seek(int offset, int whence);
    bool         SeekBeginning();
    bool         SeekEnd();
    unsigned int Write(const void* pBuffer, unsigned int elemSize, unsigned int count);
    unsigned int Write(const CExoString& str);
#endif

    void ReadAsync(void* pBuffer, unsigned int elemSize, unsigned int count);
    unsigned int ReadAsyncComplete();
    unsigned int ReadAsyncBytesRead();

#if KOTOR1  // K1: an SDL_RWops stream that may be a compressed archive; the open mode is kept for reopening
    unsigned char m_bCompressed;
    SDL_RWops*    m_pFile;
    CExoString    m_path;
    CExoString    m_mode;
#else
    FILE*      m_pFile;
    CExoString m_path;
#endif
};
#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CExoFileInternal) == 0x30, "K1 CExoFile's operator new(0x30)");
#endif

#if !KOTOR1
extern "C" {
    int  ASL_feof(FILE* f);
}

inline int CExoFileInternal::Eof()
{
    if (m_pFile)
        return ASL_feof(m_pFile);
    return 0;
}

inline bool CExoFileInternal::Flush()
{
    bool bResult = false;
    if (m_pFile)
        bResult = fflush(m_pFile) == 0;
    return bResult;
}

inline int CExoFileInternal::Seek(int offset, int whence)
{
    int nResult = 0;
    if (m_pFile)
        nResult = fseek(m_pFile, offset, whence) == 0;
    return nResult;
}

inline bool CExoFileInternal::SeekBeginning()
{
    bool bResult = false;
    if (m_pFile)
        bResult = fseek(m_pFile, 0, 0) == 0;
    return bResult;
}

inline bool CExoFileInternal::SeekEnd()
{
    bool bResult = false;
    if (m_pFile)
        bResult = fseek(m_pFile, 0, 2) == 0;
    return bResult;
}

inline unsigned int CExoFileInternal::Write(const void* pBuffer, unsigned int elemSize, unsigned int count)
{
    unsigned int nWritten = 0;
    if (m_pFile)
        nWritten = static_cast<unsigned int>(fwrite(pBuffer, elemSize, count, m_pFile)) * elemSize;
    return nWritten;
}

inline unsigned int CExoFileInternal::Write(const CExoString& str)
{
    if (!m_pFile)
        return 0;
    return static_cast<unsigned int>(fwrite(str.CStr(), 1, static_cast<unsigned int>(str.GetLength()), m_pFile));
}
#endif

#include "port/win32api.h"
