#include "k2stdint.h"
#include "exofileinternal.h"
#include "exobase.h"

#include "port/libcbase.h"
#include "port/libcstring.h"
extern "C" {
    int  ASL_feof(FILE* f);
}

CExoFile::CExoFile(const CExoString& path, unsigned short resType, const CExoString& mode)
{
    m_pInternal = new CExoFileInternal(path, resType, mode);
}

// EFFECTIVE: scheduler tie-break on the destructor's frame-setup slot, not source-fixable.
CExoFile::~CExoFile()
{
    CExoFileInternal* p = m_pInternal;
    if (p) {
        p->~CExoFileInternal();
        operator delete(p);
        m_pInternal = 0;
    }
}

int CExoFile::FileOpened()
{
    return m_pInternal->m_pFile != 0;
}

int CExoFile::Eof()
{
#if KOTOR1  // K1: at end when the stream position equals its size
    CExoFileInternal* p = m_pInternal;
    if (!p->m_pFile) return 0;
    return SDL_RWtell(p->m_pFile) == SDL_RWsize(p->m_pFile);
#else
    FILE* f = m_pInternal->m_pFile;
    if (f) return ASL_feof(f);
    return static_cast<int>(reinterpret_cast<intptr_t>(f));
#endif
}

bool CExoFile::Flush()
{
#if KOTOR1
    return m_pInternal->Flush();
#else
    bool bResult = false;
    if (m_pInternal->m_pFile) {
        bResult = fflush(m_pInternal->m_pFile) == 0;
    }
    return bResult;
#endif
}

int CExoFile::Seek(int offset, int whence)
{
#if KOTOR1
    SDL_RWops* f = m_pInternal->m_pFile;
    if (!f) return 0;
    return SDL_RWseek(f, offset, whence) != -1;
#else
    int nResult = 0;
    if (m_pInternal->m_pFile) {
        nResult = fseek(m_pInternal->m_pFile, offset, whence) == 0;
    }
    return nResult;
#endif
}

bool CExoFile::SeekEnd()
{
#if KOTOR1
    SDL_RWops* f = m_pInternal->m_pFile;
    if (!f) return 0;
    return SDL_RWseek(f, 0, RW_SEEK_END) != -1;
#else
    bool bResult = false;
    if (m_pInternal->m_pFile) {
        bResult = fseek(m_pInternal->m_pFile, 0, 2) == 0;
    }
    return bResult;
#endif
}

bool CExoFile::SeekBeginning()
{
#if KOTOR1
    SDL_RWops* f = m_pInternal->m_pFile;
    if (!f) return 0;
    return SDL_RWseek(f, 0, RW_SEEK_SET) != -1;
#else
    bool bResult = false;
    if (m_pInternal->m_pFile) {
        bResult = fseek(m_pInternal->m_pFile, 0, 0) == 0;
    }
    return bResult;
#endif
}

unsigned int CExoFile::Write(const void* pBuffer, unsigned int elemSize, unsigned int count)
{
#if KOTOR1
    return m_pInternal->Write(pBuffer, elemSize, count);
#else
    unsigned int nWritten = 0;
    if (m_pInternal->m_pFile) {
        nWritten = static_cast<unsigned int>(fwrite(pBuffer, elemSize, count, m_pInternal->m_pFile));
        nWritten = nWritten * elemSize;
    }
    return nWritten;
#endif
}

unsigned int CExoFile::Write(const CExoString& str)
{
#if KOTOR1
    return m_pInternal->Write(str.CStr(), 1, str.GetLength());
#else
    unsigned int nWritten = 0;
    CExoFileInternal* p = m_pInternal;
    if (p->m_pFile) {
        const char* pStr = str.CStr();
        uint64_t len;
        if (str.m_sString == 0) {
            len = 0;
        } else {
            len = strlen(str.m_sString);
            len = static_cast<unsigned int>(len);
        }
        nWritten = static_cast<unsigned int>(fwrite(pStr, 1, len, p->m_pFile));
    }
    return nWritten;
#endif
}

unsigned int CExoFile::GetOffset()
{
    return m_pInternal->GetOffset();
}

int CExoFile::GetSize()
{
    return m_pInternal->GetSize();
}

unsigned int CExoFile::Read(void* pBuffer, unsigned int elemSize, unsigned int count)
{
    return m_pInternal->Read(pBuffer, elemSize, count);
}

void CExoFile::Read(CExoString& out, unsigned int count)
{
    m_pInternal->Read(&out, count);
}

unsigned int CExoFile::Write(const char* str)
{
#if KOTOR1
    return m_pInternal->Write(str, 1, static_cast<unsigned int>(strlen(str)));
#else
    return m_pInternal->Write(str);
#endif
}

#if KOTOR1
int CExoFile::IsCompressed()
{
    return m_pInternal->m_bCompressed;
}
#endif

void CExoFile::ReadAsync(void* pBuffer, unsigned int elemSize, unsigned int count)
{
    m_pInternal->ReadAsync(pBuffer, elemSize, count);
}

unsigned int CExoFile::ReadAsyncComplete()
{
    return m_pInternal->ReadAsyncComplete();
}

unsigned int CExoFile::ReadAsyncBytesRead()
{
    return m_pInternal->ReadAsyncBytesRead();
}

// Public facade over CExoAliasListInternal, forwarding every call through the pimpl.
CExoAliasList::CExoAliasList()
{
    m_pInternal = new CExoAliasListInternal();
}

CExoAliasList::~CExoAliasList()
{
    if (m_pInternal)
    {
        delete m_pInternal;
        m_pInternal = 0;
    }
}

void CExoAliasList::Add(const CExoString& alias, const CExoString& path)
{
    m_pInternal->Add(alias, path);
}

void CExoAliasList::ChangePath(const CExoString& alias, const CExoString& path)
{
    m_pInternal->ChangePath(alias, path);
}

void CExoAliasList::Clear()
{
    m_pInternal->Clear();
}

void CExoAliasList::Delete(const CExoString& alias)
{
    m_pInternal->Delete(alias);
}

const CExoString& CExoAliasList::GetAliasPath(const CExoString& alias, int flag) const
{
    return m_pInternal->GetAliasPath(alias, flag);
}

CExoString CExoAliasList::ResolveFileName(const CExoString& path, unsigned short resType) const
{
    return m_pInternal->ResolveFileName(path, resType);
}
