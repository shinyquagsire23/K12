#ifndef CEXOFILE_H
#define CEXOFILE_H
// idiom-exempt: canonical

#include "exostring.h"

// A thin public pimpl wrapper over CExoFileInternal. The constructor heap-allocates the internal
// object and almost every method is a one-line forwarder.
//
// This header carries the CExoFile class alone: its only member is a CExoFileInternal pointer and
// every CExoString is taken by reference, so consumers can include it without the stdio
// file-pointer and Win32-shim declarations.
class CExoFileInternal;


class CExoFile {
public:
    CExoFile(const CExoString& path, unsigned short resType, const CExoString& mode);
    ~CExoFile();

    // These six have no CExoFileInternal counterpart: they read the internal stdio handle directly
    // and call the libc function themselves, each with its own null guard.
    int  FileOpened();
    int  Eof();
    bool Flush();
    // int rather than bool: the two CTlkTable call sites test the whole result.
    int  Seek(int offset, int whence);
    bool SeekBeginning();
    bool SeekEnd();
    // Also has no CExoFileInternal counterpart: calls fwrite directly and returns bytes written.
    unsigned int Write(const void* pBuffer, unsigned int elemSize, unsigned int count);

    // Tail-call thunks into CExoFileInternal.
    unsigned int GetOffset();
    int          GetSize();
    unsigned int Read(void* pBuffer, unsigned int elemSize, unsigned int count);
    void Read(CExoString& out, unsigned int count);
    unsigned int Write(const char* str);
    void ReadAsync(void* pBuffer, unsigned int elemSize, unsigned int count);
    unsigned int ReadAsyncComplete();
    unsigned int ReadAsyncBytesRead();
#if KOTOR1
    int IsCompressed();
#endif

    unsigned int Write(const CExoString& str);

private:
    CExoFileInternal* m_pInternal;
};
#endif
