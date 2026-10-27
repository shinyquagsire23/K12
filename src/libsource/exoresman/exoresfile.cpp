// Accessors of the packed-file base and of the resource-image container. The container models
// themselves live in CExoPackedFile.h and CExoKeyTableFormats.h.

#include "k2stdint.h"
#include "exoresfile.h"
#include <stddef.h>
#include "CExoArrayListImpl.h"
#include "exobase.h"
#include "restype.h"
#include "crc32poly.h"

// The four-character tags at the head of each container file, read as little-endian words.
enum {
    FOURCC_BIFF = 0x46464942,  // "BIFF"
    FOURCC_KEY = 0x2059454b,   // "KEY "
    FOURCC_ERF = 0x20465245,   // "ERF "
    FOURCC_HAK = 0x204b4148,   // "HAK "
    FOURCC_MOD = 0x20444f4d,   // "MOD "
    FOURCC_V1 = 0x20203156,    // "V1  "
    FOURCC_V1_0 = 0x302e3156,  // "V1.0"
};

#if KOTOR1  // K1 allocates without the file and line tags the K2 macros carry
#include <stdio.h>
#include <string.h>
#include "resgff.h"
extern "C" int MacDecompress(void* pDst, uint64_t nDstSize, void* pSrc, uint64_t nSrcSize);
void* ExoMalloc(uint64_t nSize);
void ExoFree(void* p);
#endif

CExoPackedFile::CExoPackedFile()
{
    m_bFileOpened = 0;
    m_bHeaderLoaded = 0;
}

CExoPackedFile::~CExoPackedFile()
{
}

void* CExoPackedFile::GetFile()
{
    return m_pFile;
}

void* CExoPackedFile::GetAsyncFile()
{
    return m_pAsyncFile;
}

CResourceImageHeader* CExoResourceImageFile::GetHeader()
{
    return m_pHeader;
}

ResourceImageEntry* CExoResourceImageFile::GetKeyList()
{
    return reinterpret_cast<ResourceImageEntry*>(reinterpret_cast<unsigned char*>(m_pHeader) + m_pHeader->m_nEntriesOffset);  // raw-offset: ERF/RIM image: offsets from the file header
}

ResourceImageEntry CExoResourceImageFile::GetKeyListEntry(unsigned int index)
{
#if KOTOR1  // K1 byte-swaps the numeric fields of the returned copy
    ResourceImageEntry entry =
        reinterpret_cast<ResourceImageEntry*>(reinterpret_cast<unsigned char*>(m_pHeader) + m_pHeader->m_nEntriesOffset)[index];  // raw-offset: ERF/RIM image: offsets from the file header
    ByteSwap(&entry.m_resIDRaw);
    ByteSwap(&entry.m_nDataOffset);
    ByteSwap(&entry.m_nResourceSize);
    ByteSwap(&entry.m_resType);
    return entry;
#else
    return reinterpret_cast<ResourceImageEntry*>(reinterpret_cast<unsigned char*>(m_pHeader) + m_pHeader->m_nEntriesOffset)[index];  // raw-offset: ERF/RIM image: offsets from the file header
#endif
}

unsigned int CExoResourceImageFile::GetResourceSize(unsigned int resId)
{
#if KOTOR1  // K1 reads a byte-swapped copy of the entry
    unsigned char* pKeyList = reinterpret_cast<unsigned char*>(m_pHeader) + m_pHeader->m_nEntriesOffset;  // raw-offset: ERF/RIM image: offsets from the file header
    ResourceImageEntry entry =
        *reinterpret_cast<ResourceImageEntry*>(pKeyList + (resId & RESID_INDEX_MASK) * sizeof(ResourceImageEntry));
    ByteSwap(&entry.m_resIDRaw);
    ByteSwap(&entry.m_nDataOffset);
    ByteSwap(&entry.m_nResourceSize);
    ByteSwap(&entry.m_resType);
    return entry.m_nResourceSize;
#else
    // The entry index lives in the resource id's low 14 bits, the packed layout
    // AddResourceImageContents produces.
    unsigned char* pKeyList = reinterpret_cast<unsigned char*>(m_pHeader) + m_pHeader->m_nEntriesOffset;  // raw-offset: ERF/RIM image: offsets from the file header
    ResourceImageEntry* pEntry =
        reinterpret_cast<ResourceImageEntry*>(pKeyList + (resId & RESID_INDEX_MASK) * sizeof(ResourceImageEntry));
    return pEntry->m_nResourceSize;
#endif
}

void* CExoResourceImageFile::GetResource(unsigned int resId)
{
#if KOTOR1  // K1 reads a byte-swapped copy of the entry
    unsigned char* pHeader = reinterpret_cast<unsigned char*>(m_pHeader);
    unsigned char* pKeyList = pHeader + reinterpret_cast<CResourceImageHeader*>(pHeader)->m_nEntriesOffset;  // raw-offset: ERF/RIM image: offsets from the file header
    ResourceImageEntry entry =
        *reinterpret_cast<ResourceImageEntry*>(pKeyList + (resId & RESID_INDEX_MASK) * sizeof(ResourceImageEntry));
    ByteSwap(&entry.m_resIDRaw);
    ByteSwap(&entry.m_nDataOffset);
    ByteSwap(&entry.m_nResourceSize);
    ByteSwap(&entry.m_resType);
    return pHeader + entry.m_nDataOffset;  // raw-offset: ERF/RIM image: offsets from the file header
#else
    unsigned char* pKeyList = reinterpret_cast<unsigned char*>(m_pHeader) + m_pHeader->m_nEntriesOffset;  // raw-offset: ERF/RIM image: offsets from the file header
    ResourceImageEntry* pEntry =
        reinterpret_cast<ResourceImageEntry*>(pKeyList + (resId & RESID_INDEX_MASK) * sizeof(ResourceImageEntry));
    return reinterpret_cast<unsigned char*>(m_pHeader) + pEntry->m_nDataOffset;  // raw-offset: ERF/RIM image: offsets from the file header
#endif
}

void CExoResourceImageFile::DeleteRefCount()
{
    m_nRefCount = (m_nRefCount < 2) ? 0 : (m_nRefCount - 1);
}

int CExoResourceImageFile::Initialize()
{
    m_pFile = NULL;
    m_nDrives = 0;
    m_nFileSize = 0;
    m_nRefCount = 0;
    m_bFileOpened = 0;
    m_pHeader = NULL;
    return 1;
}

unsigned int CExoEncapsulatedFile::GetResourceSize(unsigned int resId)
{
    return m_paEntries[resId & RESID_INDEX_MASK].m_nResourceSize;
}

int CExoEncapsulatedFile::Initialize()
{
    m_nDrives = 0;
    // m_nFileSize through m_bAsyncFileOpened are cleared as one run, ints and pointers together.
    __builtin_memset(&m_nFileSize, 0,
                     static_cast<size_t>(reinterpret_cast<char*>(&m_bAsyncFileOpened + 1)
                     - reinterpret_cast<char*>(&m_nFileSize)));
    m_pHeader = NULL;
    m_paEntries = NULL;
    m_nFormatTag = 0;
    return 1;
}

int CExoResFile::CloseFile()
{
    if (m_bFileOpened)
    {
        m_sFileName.CStr();  // sic: the result is computed and discarded
        if (m_pFile)
        {
            delete m_pFile;
            m_pFile = NULL;
        }
        m_bFileOpened = 0;
    }
    return 1;
}

CExoResFile::~CExoResFile()
{
    CloseFile();
}

int CExoEncapsulatedFile::CloseFile()
{
    if (m_bFileOpened)
    {
        if (m_pFile)
        {
            delete m_pFile;
            m_pFile = NULL;
        }
        m_bFileOpened = 0;
    }
    return 1;
}

CExoEncapsulatedFile::~CExoEncapsulatedFile()
{
    CloseFile();
}

int CExoResourceImageFile::CloseFile()
{
    if (m_bFileOpened)
    {
        // The header pointer owns the whole raw archive-header buffer.
        if (m_pHeader)
        {
            delete[] m_pHeader;
            m_pHeader = NULL;
        }
        m_bFileOpened = 0;
    }
    return 1;
}

CExoResourceImageFile::~CExoResourceImageFile()
{
    CloseFile();
}


CExoResFile::CExoResFile()
{
    m_nRefCount = 0;
    Initialize();
}

void CExoResFile::AddRefCount()
{
    if (m_nRefCount <= 0)
    {
        m_nRefCount = 0;
        OpenFile();
    }
    m_nRefCount++;
}

void CExoResFile::AddAsyncRefCount()
{
    if (m_nAsyncRefCount <= 0)
    {
        m_nAsyncRefCount = 0;
        OpenAsyncFile();
    }
    m_nAsyncRefCount++;
}

int CExoResFile::CloseAsyncFile()
{
    if (m_bAsyncFileOpened)
    {
        if (m_pAsyncFile)
        {
            delete m_pAsyncFile;
            m_pAsyncFile = NULL;
        }
        m_bAsyncFileOpened = 0;
    }
    return 1;
}

int CExoResFile::UnloadHeader()
{
    if (m_bHeaderLoaded)
    {
        if (m_paEntries)
        {
            delete[] m_paEntries;
            m_paEntries = NULL;
        }
        if (m_pHeader)
        {
            delete m_pHeader;
            m_pHeader = NULL;
        }
        m_bHeaderLoaded = 0;
        DeleteRefCount();
    }
    return 1;
}

void CExoResFile::DeleteRefCount()
{
    int nRemaining = m_nRefCount - 1;
    if (nRemaining <= 0)
    {
        m_nRefCount = 0;
        CloseFile();
    }
    else
    {
        m_nRefCount = nRemaining;
    }
}

void CExoResFile::DeleteAsyncRefCount()
{
    int nRemaining = m_nAsyncRefCount - 1;
    if (nRemaining <= 0)
    {
        m_nAsyncRefCount = 0;
        CloseAsyncFile();
    }
    else
    {
        m_nAsyncRefCount = nRemaining;
    }
}

int CExoResFile::Initialize()
{
    // sic: another discarded CStr(), exactly like CloseFile's.
    m_sFileName.CStr();
    m_nDrives = 0;
    __builtin_memset(&m_nFileSize, 0,
                     static_cast<size_t>(reinterpret_cast<char*>(&m_bAsyncFileOpened + 1)
                     - reinterpret_cast<char*>(&m_nFileSize)));
    m_pHeader = NULL;
    m_paEntries = NULL;
    return 1;
}

int CExoResFile::OpenFile()
{
    if (m_bFileOpened)
        return 1;

    m_pFile = new CExoFile(m_sFileName, RESTYPE_BIF, "rb");
    if (!m_pFile)
        return 0;

    if (m_pFile->FileOpened())
    {
        m_sFileName.CStr();  // sic: discarded, as everywhere else in this class
        m_bFileOpened = 1;
        return 1;
    }

    if (m_pFile)
    {
        delete m_pFile;
    }
    m_pFile = NULL;
    return 0;
}

int CExoResFile::OpenAsyncFile()
{
    if (m_bAsyncFileOpened)
        return 1;

    m_pAsyncFile = new CExoFile(m_sFileName, RESTYPE_BIF, "rb");
    if (!m_pAsyncFile)
        return 0;

    if (m_pAsyncFile->FileOpened())
    {
        m_bAsyncFileOpened = 1;
        return 1;
    }

    if (m_pAsyncFile)
    {
        delete m_pAsyncFile;
    }
    m_pAsyncFile = NULL;
    return 0;
}

int CExoResFile::LoadHeader(unsigned char a)
{
    K2_UNUSED(a);
    if (m_bHeaderLoaded)
        return 1;

    AddRefCount();
    if (!m_pFile)
    {
        m_nRefCount--;
        return 0;
    }

    m_sFileName.CStr();  // sic: discarded
    m_nFileSize = m_pFile->GetSize();

#if KOTOR1  // K1 does not check the read result; the header and every entry are byte-swapped
    m_pHeader = new CExoResFileHeader;
    m_pFile->Read(m_pHeader, sizeof(CExoResFileHeader), 1);
    ByteSwap(&m_pHeader->m_fileType);
    ByteSwap(&m_pHeader->m_fileVersion);
    ByteSwap(&m_pHeader->m_nVariableResourceCount);
    ByteSwap(&m_pHeader->m_nFixedResourceCount);
    ByteSwap(reinterpret_cast<unsigned int*>(&m_pHeader->m_nOffsetToVariableResourceTable));
    if (m_pHeader->m_fileType == FOURCC_BIFF &&
        m_pHeader->m_fileVersion == FOURCC_V1)
    {
        m_paEntries = NULL;
        if (m_pHeader->m_nVariableResourceCount)
        {
            m_paEntries = new CExoResFileEntry[m_pHeader->m_nVariableResourceCount];
            m_pFile->Seek(m_pHeader->m_nOffsetToVariableResourceTable, 0);
            m_pFile->Read(m_paEntries, sizeof(CExoResFileEntry),
                          m_pHeader->m_nVariableResourceCount);
            for (unsigned int i = 0; i < m_pHeader->m_nVariableResourceCount; i++) {
                ByteSwap(&m_paEntries[i].m_nResourceId);
                ByteSwap(&m_paEntries[i].m_nDataOffset);
                ByteSwap(&m_paEntries[i].m_nResourceSize);
                ByteSwap(&m_paEntries[i].m_nResourceType);
            }
            m_bHeaderLoaded = 1;
            return 1;
        }
    }
#else
    m_pHeader = new CExoResFileHeader;
    if (m_pFile->Read(m_pHeader, sizeof(CExoResFileHeader), 1) == sizeof(CExoResFileHeader) &&
        m_pHeader->m_fileType == FOURCC_BIFF &&
        m_pHeader->m_fileVersion == FOURCC_V1)
    {
        m_paEntries = NULL;
        if (m_pHeader->m_nVariableResourceCount)
        {
            m_paEntries = new CExoResFileEntry[m_pHeader->m_nVariableResourceCount];
            m_pFile->Seek(m_pHeader->m_nOffsetToVariableResourceTable, 0);
            m_pFile->Read(m_paEntries, sizeof(CExoResFileEntry),
                          m_pHeader->m_nVariableResourceCount);
            m_bHeaderLoaded = 1;
            return 1;
        }
    }
#endif

    delete m_pHeader;
    m_pHeader = NULL;
    DeleteRefCount();
    return 0;
}

int CExoResFile::ReadResource(unsigned int resId, void* pBuffer, unsigned int nSize, unsigned int nOffset)
{
    K2_UNUSED(nOffset);
    if (!pBuffer || !m_bFileOpened)
        return 0;

    unsigned int index = resId & RESID_INDEX_MASK;
    if (index >= m_pHeader->m_nVariableResourceCount)
        return 0;

    m_pFile->Seek(m_paEntries[index].m_nDataOffset, 0);
#if KOTOR1  // K1 reads a compressed BIF entry whole and decompresses it
    if (m_pFile->IsCompressed()) {
        unsigned int nUncompressedSize = m_paEntries[index].m_nResourceSize;
        int nEnd;
        if (index + 1 < m_pHeader->m_nVariableResourceCount)
            nEnd = m_paEntries[index + 1].m_nDataOffset;
        else
            nEnd = m_pFile->GetSize();
        uint64_t nCompressedSize = nEnd - static_cast<int>(m_paEntries[index].m_nDataOffset);
        char* pCompressed = new char[nCompressedSize];
        char* pUncompressed = new char[nUncompressedSize];
        unsigned int nRead = m_pFile->Read(pCompressed, static_cast<unsigned int>(nCompressedSize), 1);
        MacDecompress(pUncompressed, nUncompressedSize, pCompressed, nRead);
        unsigned int nCopied = (m_paEntries[index].m_nResourceSize > nSize)
                                   ? m_paEntries[index].m_nResourceSize : nSize;
        memcpy(pBuffer, pUncompressed, nCopied);  // idiom-exempt: runtime length (the larger of entry and buffer, as the original)
        delete[] pCompressed;
        delete[] pUncompressed;
        return nCopied;
    }
#endif

    // Two calls rather than one clamped argument.
    if (m_paEntries[index].m_nResourceSize > nSize)
        return m_pFile->Read(pBuffer, nSize, 1);
    return m_pFile->Read(pBuffer, m_paEntries[index].m_nResourceSize, 1);
}

#if KOTOR1
void CExoResFile::ReadResourceAsync(unsigned int resId, void* pBuffer, unsigned int nSize,
                                    unsigned int nOffset)
#else
void CExoResFile::ReadResourceAsync(unsigned int resId, void* pBuffer, unsigned int nSize,
                                    unsigned int)
#endif
{
    if (!pBuffer || !m_bAsyncFileOpened)
        return;

    unsigned int index = resId & RESID_INDEX_MASK;
    if (index >= m_pHeader->m_nVariableResourceCount)
        return;

    m_pAsyncFile->Seek(m_paEntries[index].m_nDataOffset, 0);

    if (m_paEntries[index].m_nResourceSize > nSize)
        m_pAsyncFile->ReadAsync(pBuffer, nSize, 1);
    else
        m_pAsyncFile->ReadAsync(pBuffer, m_paEntries[index].m_nResourceSize, 1);
#if KOTOR1  // K1: a compressed file waits for the async read, then re-reads synchronously
    if (m_pFile->IsCompressed()) {
        while (!m_pAsyncFile->ReadAsyncComplete()) {
        }
        ReadResource(resId, pBuffer, nSize, nOffset);
    }
#endif
}


CExoEncapsulatedFile::CExoEncapsulatedFile()
{
    Initialize();
}

void CExoEncapsulatedFile::AddRefCount()
{
    if (m_nRefCount <= 0)
    {
        m_nRefCount = 0;
        OpenFile();
    }
    m_nRefCount++;
}

void CExoEncapsulatedFile::AddAsyncRefCount()
{
    if (m_nAsyncRefCount <= 0)
    {
        m_nAsyncRefCount = 0;
        OpenAsyncFile();
    }
    m_nAsyncRefCount++;
}

int CExoEncapsulatedFile::CloseAsyncFile()
{
    if (m_bAsyncFileOpened)
    {
        if (m_pAsyncFile)
        {
            delete m_pAsyncFile;
            m_pAsyncFile = NULL;
        }
        m_bAsyncFileOpened = 0;
    }
    return 1;
}

int CExoEncapsulatedFile::UnloadHeader()
{
    if (m_bHeaderLoaded)
    {
        if (m_paEntries)
        {
            delete[] m_paEntries;
            m_paEntries = NULL;
        }
        m_sDescription.ClearLocString();
        if (m_pHeader)
        {
            delete m_pHeader;
            m_pHeader = NULL;
        }
        m_bHeaderLoaded = 0;
    }
    return 1;
}

void CExoEncapsulatedFile::DeleteRefCount()
{
    int nRemaining = m_nRefCount - 1;
    if (nRemaining <= 0)
    {
        m_nRefCount = 0;
        CloseFile();
    }
    else
    {
        m_nRefCount = nRemaining;
    }
}

void CExoEncapsulatedFile::DeleteAsyncRefCount()
{
    int nRemaining = m_nAsyncRefCount - 1;
    if (nRemaining <= 0)
    {
        m_nAsyncRefCount = 0;
        CloseAsyncFile();
    }
    else
    {
        m_nAsyncRefCount = nRemaining;
    }
}

int CExoEncapsulatedFile::OpenFile()
{
    if (m_bFileOpened)
        return 1;

    m_pFile = NULL;
    switch (m_nFormatTag)
    {
        case ENCAPSULATED_FORMAT_MOD: m_pFile = new CExoFile(m_sFileName, RESTYPE_MOD, "rb"); break;
        case ENCAPSULATED_FORMAT_SAV: m_pFile = new CExoFile(m_sFileName, RESTYPE_SAV, "rb"); break;
        case ENCAPSULATED_FORMAT_ERF: m_pFile = new CExoFile(m_sFileName, RESTYPE_ERF, "rb"); break;
        case ENCAPSULATED_FORMAT_HAK: m_pFile = new CExoFile(m_sFileName, RESTYPE_HAK, "rb"); break;
        case ENCAPSULATED_FORMAT_NWM: m_pFile = new CExoFile(m_sFileName, RESTYPE_NWM, "rb"); break;
    }

    int bOpened = 0;
    if (m_pFile)
    {
        if (m_pFile->FileOpened())
        {
            bOpened = 1;
        }
        else
        {
            delete m_pFile;
            m_pFile = NULL;
        }
    }
    m_bFileOpened = bOpened;
    return bOpened;
}

int CExoEncapsulatedFile::OpenAsyncFile()
{
    if (m_bAsyncFileOpened)
        return 1;

    m_pAsyncFile = NULL;
    switch (m_nFormatTag)
    {
        case ENCAPSULATED_FORMAT_MOD: m_pAsyncFile = new CExoFile(m_sFileName, RESTYPE_MOD, "rb"); break;
        case ENCAPSULATED_FORMAT_SAV: m_pAsyncFile = new CExoFile(m_sFileName, RESTYPE_SAV, "rb"); break;
        case ENCAPSULATED_FORMAT_ERF: m_pAsyncFile = new CExoFile(m_sFileName, RESTYPE_ERF, "rb"); break;
        case ENCAPSULATED_FORMAT_HAK: m_pAsyncFile = new CExoFile(m_sFileName, RESTYPE_HAK, "rb"); break;
        case ENCAPSULATED_FORMAT_NWM: m_pAsyncFile = new CExoFile(m_sFileName, RESTYPE_NWM, "rb"); break;
    }

    int bOpened = 0;
    if (m_pAsyncFile)
    {
        if (m_pAsyncFile->FileOpened())
        {
            bOpened = 1;
        }
        else
        {
            delete m_pAsyncFile;
            m_pAsyncFile = NULL;
        }
    }
    m_bAsyncFileOpened = bOpened;
    return bOpened;
}

int CExoEncapsulatedFile::ReadResource(unsigned int resId, void* pBuffer, unsigned int nSize,
                                       unsigned int nOffset)
{
    K2_UNUSED(nOffset);
    if (!pBuffer || !m_bFileOpened)
        return 0;

    unsigned int index = resId & RESID_INDEX_MASK;
    if (index >= m_pHeader->m_nEntryCount)
        return 0;

    m_pFile->Seek(m_paEntries[index].m_nDataOffset, 0);
#if KOTOR1  // K1 reads a compressed ERF entry whole and decompresses it
    if (m_pFile->IsCompressed()) {
        unsigned int nUncompressedSize = m_paEntries[index].m_nResourceSize;
        int nEnd;
        if (index + 1 < m_pHeader->m_nEntryCount)
            nEnd = m_paEntries[index + 1].m_nDataOffset;
        else
            nEnd = m_pFile->GetSize();
        uint64_t nCompressedSize = nEnd - static_cast<int>(m_paEntries[index].m_nDataOffset);
        char* pCompressed = new char[nCompressedSize];
        char* pUncompressed = new char[nUncompressedSize];
        unsigned int nRead = m_pFile->Read(pCompressed, static_cast<unsigned int>(nCompressedSize), 1);
        MacDecompress(pUncompressed, nUncompressedSize, pCompressed, nRead);
        unsigned int nCopied = (m_paEntries[index].m_nResourceSize > nSize)
                                   ? m_paEntries[index].m_nResourceSize : nSize;
        memcpy(pBuffer, pUncompressed, nCopied);  // idiom-exempt: runtime length (the larger of entry and buffer, as the original)
#ifdef K2_FIX_ENGINE_BUGS
        delete[] pCompressed;
        delete[] pUncompressed;
#else
        delete pCompressed;
        delete pUncompressed;
#endif
        return nCopied;
    }
    if (strstr(m_sFileName.CStr(), "swpc_tex_tpb"))
        puts("UNCOMPRESSED TEXTURE FILE USED FOR SOME REASON");
#endif
    if (m_paEntries[index].m_nResourceSize > nSize)
        return m_pFile->Read(pBuffer, nSize, 1);
    return m_pFile->Read(pBuffer, m_paEntries[index].m_nResourceSize, 1);
}

#if KOTOR1
void CExoEncapsulatedFile::ReadResourceAsync(unsigned int resId, void* pBuffer, unsigned int nSize,
                                             unsigned int nOffset)
#else
void CExoEncapsulatedFile::ReadResourceAsync(unsigned int resId, void* pBuffer, unsigned int nSize,
                                             unsigned int)
#endif
{
    if (!pBuffer || !m_bAsyncFileOpened)
        return;

    unsigned int index = resId & RESID_INDEX_MASK;
    if (index >= m_pHeader->m_nEntryCount)
        return;

    m_pAsyncFile->Seek(m_paEntries[index].m_nDataOffset, 0);
    if (m_paEntries[index].m_nResourceSize > nSize)
        m_pAsyncFile->ReadAsync(pBuffer, nSize, 1);
    else
        m_pAsyncFile->ReadAsync(pBuffer, m_paEntries[index].m_nResourceSize, 1);
#if KOTOR1  // K1: a compressed file waits for the async read, then re-reads synchronously
    if (m_pFile->IsCompressed()) {
        while (!m_pAsyncFile->ReadAsyncComplete()) {
        }
        ReadResource(resId, pBuffer, nSize, nOffset);
    }
#endif
}


CExoResourceImageFile::CExoResourceImageFile()
{
    Initialize();
}

void CExoResourceImageFile::AddRefCount()
{
    if (m_nRefCount <= 0)
    {
        m_nRefCount = 0;
        OpenFile();
    }
    m_nRefCount++;
}

int CExoResourceImageFile::UnloadHeader()
{
    return 1;
}


// EFFECTIVE: two conditional branches have the opposite polarity; same targets.
int CExoEncapsulatedFile::LoadHeader(unsigned char nFormatTag)
{
    CExoString sText;
    char* pDescription;

    if (m_bHeaderLoaded)
        return 1;

    m_pFile = NULL;
    switch (nFormatTag)
    {
        case ENCAPSULATED_FORMAT_MOD: m_pFile = new CExoFile(m_sFileName, RESTYPE_MOD, "rb"); break;
        case ENCAPSULATED_FORMAT_SAV: m_pFile = new CExoFile(m_sFileName, RESTYPE_SAV, "rb"); break;
        case ENCAPSULATED_FORMAT_ERF: m_pFile = new CExoFile(m_sFileName, RESTYPE_ERF, "rb"); break;
        case ENCAPSULATED_FORMAT_HAK: m_pFile = new CExoFile(m_sFileName, RESTYPE_HAK, "rb"); break;
        case ENCAPSULATED_FORMAT_NWM: m_pFile = new CExoFile(m_sFileName, RESTYPE_NWM, "rb"); break;
    }

    if (!m_pFile)
        return 0;

    if (!m_pFile->FileOpened())
    {
        delete m_pFile;
        m_pFile = NULL;
        return 0;
    }

    m_nFormatTag = nFormatTag;
    m_nFileSize = m_pFile->GetSize();

    m_pHeader = new CEncapsulatedFileHeader;
    m_pFile->Read(m_pHeader, sizeof(CEncapsulatedFileHeader), 1);
#if KOTOR1  // K1 byte-swaps the eight header words
    ByteSwap(&m_pHeader->m_fileType);
    ByteSwap(&m_pHeader->m_version);
    ByteSwap(&m_pHeader->m_nLanguageCount);
    ByteSwap(&m_pHeader->m_nLocalizedStringSize);
    ByteSwap(&m_pHeader->m_nEntryCount);
    ByteSwap(reinterpret_cast<unsigned int*>(&m_pHeader->m_nOffsetToLocalizedString));
    ByteSwap(reinterpret_cast<unsigned int*>(&m_pHeader->m_nOffsetToKeyList));
    ByteSwap(reinterpret_cast<unsigned int*>(&m_pHeader->m_nOffsetToResourceList));
#endif

    // The per-tag magic chain; every arm shares the version check and the teardown.
    if (nFormatTag == ENCAPSULATED_FORMAT_ERF)
    {
        if (m_pHeader->m_fileType != FOURCC_ERF)
            goto fail;
    }
    else if (nFormatTag == ENCAPSULATED_FORMAT_HAK)
    {
        if (m_pHeader->m_fileType != FOURCC_HAK)
            goto fail;
    }
    else if (m_pHeader->m_fileType != FOURCC_MOD)
    {
        goto fail;
    }

    if (m_pHeader->m_version != FOURCC_V1_0)
        goto fail;

    m_pFile->Seek(m_pHeader->m_nOffsetToLocalizedString, 0);
    if (m_pHeader->m_nLocalizedStringSize)
    {
        pDescription = new char[m_pHeader->m_nLocalizedStringSize];
        m_pFile->Read(pDescription, 1, m_pHeader->m_nLocalizedStringSize);

        if (m_pHeader->m_nLanguageCount)
        {
            unsigned int i = 0;
            char* pRecord = pDescription;
            do
            {
                // The length is unsigned.
#if KOTOR1
                // K1 swaps a copy of each (the language in the lower slot)
                const CEncapsulatedLocalizedString* pEntry = reinterpret_cast<const CEncapsulatedLocalizedString*>(pRecord);
                unsigned int nLength;
                int nLanguage = pEntry->m_nLanguage;
                nLength = pEntry->m_nLength;
                ByteSwap(&nLanguage);
                ByteSwap(&nLength);
                pRecord += sizeof(CEncapsulatedLocalizedString);
                sText = CExoString(pRecord, nLength);
                pRecord += nLength;
                m_sDescription.AddString(nLanguage, sText, 0);
                i++;
#else
                const CEncapsulatedLocalizedString* pEntry = reinterpret_cast<const CEncapsulatedLocalizedString*>(pRecord);
                int nLanguage = pEntry->m_nLanguage;
                unsigned int nLength = pEntry->m_nLength;
                char* pText = pRecord + sizeof(CEncapsulatedLocalizedString);
                sText = CExoString(pText, nLength);
                m_sDescription.AddString(nLanguage, sText, 0);
                i++;
                pRecord = pText + nLength;
#endif
            } while (i < m_pHeader->m_nLanguageCount);
        }
        delete[] pDescription;
    }

    m_paEntries = new EncapsulatedFileEntry[m_pHeader->m_nEntryCount];
    m_pFile->Seek(m_pHeader->m_nOffsetToResourceList, 0);
    m_pFile->Read(m_paEntries, sizeof(EncapsulatedFileEntry), m_pHeader->m_nEntryCount);
#if KOTOR1
    for (unsigned int i = 0; i < m_pHeader->m_nEntryCount; i++) {
        ByteSwap(&m_paEntries[i].m_nDataOffset);
        ByteSwap(&m_paEntries[i].m_nResourceSize);
    }
#endif
    m_bHeaderLoaded = 1;
    delete m_pFile;  // sic: not nulled
    return 1;

fail:
    delete m_pHeader;
    m_pHeader = NULL;
    delete m_pFile;
    m_pFile = NULL;
    return 0;
}


#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcstdlib.h"

int CExoResourceImageFile::OpenFile()
{
    if (m_bFileOpened)
        return 1;

    m_pFile = new CExoFile(m_sFileName, RESTYPE_RSV, "rb");
    if (!m_pFile)
        return 0;

    if (!m_pFile->FileOpened())
    {
        delete m_pFile;
        m_pFile = NULL;

        m_pFile = new CExoFile(m_sFileName, RESTYPE_RIM, "rb");
        if (!m_pFile)
            return 0;
        // sic: this arm leaks the second file object.
        if (!m_pFile->FileOpened())
            return 0;
    }

    // Held unsigned although GetSize returns int.
    unsigned int nFileSize = m_pFile->GetSize();
    m_nFileSize = nFileSize;
    m_pHeader = reinterpret_cast<CResourceImageHeader*>(new char[nFileSize]);
    if (m_pFile->Read(m_pHeader, nFileSize, 1) == nFileSize)
    {
        m_bFileOpened = 1;
        delete m_pFile;
        m_pFile = NULL;
        return 1;
    }

    delete m_pFile;
    m_pFile = NULL;
    return 0;
}

int CExoResourceImageFile::ReadResource(unsigned int resId, void* pBuffer, unsigned int nSize,
                                        unsigned int nOffset)
{
    K2_UNUSED(nOffset);
    unsigned int nCopied = 0;
    if (!pBuffer || !m_bFileOpened)
        return nCopied;

    unsigned int index = resId & RESID_INDEX_MASK;
    if (index >= m_pHeader->m_nEntryCount)
        return 0;

    ResourceImageEntry** paKeyList =
        reinterpret_cast<ResourceImageEntry**>(reinterpret_cast<unsigned char*>(m_pHeader) + m_pHeader->m_nEntriesOffset);  // raw-offset: ERF/RIM image: offsets from the file header
    ResourceImageEntry* pEntry = paKeyList[index];

    nCopied = pEntry->m_nResourceSize;
    if (pEntry->m_nResourceSize > nSize)
        nCopied = nSize;
    unsigned char* pData = reinterpret_cast<unsigned char*>(m_pHeader) + pEntry->m_nDataOffset;  // raw-offset: ERF/RIM image: offsets from the file header
    memcpy(pBuffer, pData, nCopied);  // idiom-exempt: a runtime resource byte count, not a sizeof
    return nCopied;
}

void CExoResourceImageFile::ReadResourceAsync(unsigned int resId, void* pBuffer, unsigned int nSize,
                                              unsigned int nOffset)
{
    ReadResource(resId, pBuffer, nSize, nOffset);
}

void CExoResourceImageFile::AddAsyncRefCount()
{
}

int CExoResourceImageFile::CloseAsyncFile()
{
    return 1;
}

void CExoResourceImageFile::DeleteAsyncRefCount()
{
}

int CExoResourceImageFile::OpenAsyncFile()
{
    return 1;
}

int CExoResourceImageFile::LoadHeader(unsigned char a)
{
    K2_UNUSED(a);
    return 0;
}

// DestroyTable's sprintf call goes through the __builtin___sprintf_chk builtin directly: no
// <stdio.h> may be included here, because CExoFile.h forward-declares its own opaque file type,
// which conflicts with the real typedef.

// Placement new, declared by hand for the same reason (<new> pulls in <cstdio>).
inline void* operator new(size_t, void* p) throw() { return p; }


unsigned int CExoKeyTable::m_nCRC32[256];

CExoKeyTable::CExoKeyTable()
{
    m_bValid = 0;
    m_bLookupDisabled = 0;
    m_nInternalSubtype = 0;
    m_nTableSize = 0;
    m_nAddKeyProbes = 0;
    m_paEntries = NULL;
    m_nReserved1 = 0xffffffff;
    m_nSourceType = KEYTABLE_SOURCE_NONE;
    m_sFileName = "";
    m_nDependentFileCount = 0;
    m_paDependentFiles = NULL;

    for (int i = 0; i < 256; i++) {
        unsigned int crc = static_cast<unsigned int>(i);
        for (int k = 0; k < 8; k++)
            crc = (crc & 1) ? (CRC32_POLYNOMIAL ^ (crc >> 1)) : (crc >> 1);
        m_nCRC32[i] = crc;
    }
}

CExoKeyTable::~CExoKeyTable()
{
    void** paFiles = m_paDependentFiles;
    if (paFiles != NULL) {
        if (m_nDependentFileCount != 0) {
            for (unsigned int i = 0; i < m_nDependentFileCount; i++) {
                CExoPackedFile* pFile = static_cast<CExoPackedFile*>(paFiles[i]);
                if (pFile != NULL) {
                    pFile->UnloadHeader();
                    paFiles = m_paDependentFiles;
                    pFile = static_cast<CExoPackedFile*>(paFiles[i]);
                    if (pFile != NULL) {
                        delete pFile;
                        paFiles = m_paDependentFiles;
                    }
                }
            }
        }
        if (paFiles != NULL)
            operator delete[](paFiles);
    }

    delete[] m_paEntries;

    m_bValid = 0;
    m_bLookupDisabled = 1;
    m_nTableSize = 0;
    m_paEntries = NULL;
    m_nAddKeyProbes = 0;
    m_nReserved1 = 0xffffffff;
    m_nSourceType = KEYTABLE_SOURCE_NONE;
    m_sFileName = "";
    m_nDependentFileCount = 0;
    m_paDependentFiles = NULL;
}

int CExoKeyTable::Hash(const CResRef& resRef, unsigned short resType)
{
    unsigned int crc = 0;
    for (int i = 0; i < 16; i++) {
        unsigned char c = static_cast<unsigned char>(resRef.m_sResRef[i]);
        if (c == 0)
            break;
        int lc = tolower(c);
        crc = m_nCRC32[static_cast<unsigned char>((lc + resType) ^ crc)] ^ (crc >> 8);
    }
    return crc % m_nTableSize;
}

CKeyTableEntry* CExoKeyTable::FindKey(const CResRef& resRef, unsigned short resType)
{
    CKeyTableEntry* pResult = NULL;
    if (!(resRef == const_cast<char*>("")) && m_nTableSize != 0) {
        int bStop = 0;
        int bWrapped = 0;
        unsigned int startIndex = Hash(resRef, resType);
        unsigned int index = startIndex;
        do {
            if (m_paEntries[index].m_resRef == resRef && m_paEntries[index].m_resType == resType) {
                pResult = &m_paEntries[index];
                bStop = 1;
                bWrapped = 0;
            } else if (m_paEntries[index].m_resRef == const_cast<char*>("")) {
                bStop = 1;
                bWrapped = 0;
            } else {
                bStop = 0;
                index = (index + 1 < m_nTableSize) ? index + 1 : 0;
                if (index == startIndex)
                    bWrapped = 1;
            }
        } while (!(bStop | bWrapped));
    }
    return pResult;
}


CKeyTableEntry* CExoKeyTable::FindKey(CRes* pRes)
{
    if (pRes != NULL) {
        CKeyTableEntry* pEntry = pRes->m_pKeyEntry;
        if (pEntry != NULL)
            return FindKey(pEntry->m_resRef, pEntry->m_resType);
    }
    return NULL;
}

void* CExoKeyTable::GetRes(const CResRef& resRef, unsigned short resType)
{
    CKeyTableEntry* pEntry = FindKey(resRef, resType);
    return pEntry ? pEntry->m_pRes : NULL;
}

unsigned int CExoKeyTable::GetResID(const CResRef& resRef, unsigned short resType)
{
    CKeyTableEntry* pEntry = FindKey(resRef, resType);
    if (pEntry != NULL)
        return pEntry->m_dwResID;
    return RESID_INVALID;
}

bool CExoKeyTable::GetTableEntry(unsigned int index, CResRef& outResRef, unsigned short& outResType)
{
    if (m_paEntries == NULL || m_nTableSize <= index)
        return false;
    CKeyTableEntry* pEntry = &m_paEntries[index];
    outResRef = pEntry->m_resRef;
    outResType = pEntry->m_resType;
    return true;
}

bool CExoKeyTable::GetTableIndex(unsigned int& outIndex, const CResRef& resRef, unsigned short resType)
{
    if (m_paEntries == NULL || m_nTableSize == 0)
        return false;
    for (unsigned int i = 0; i < m_nTableSize; i++) {
        CKeyTableEntry* pEntry = &m_paEntries[i];
        if (pEntry->m_resType == resType) {
            if (pEntry->m_resRef == resRef) {
                outIndex = i;
                return true;
            }
        }
    }
    return false;
}

int CExoKeyTable::GetEntryCount(int bIncludeStatic)
{
    if (m_nTableSize == 0)
        return 0;
    int count = 0;
    for (unsigned int i = 0; i < m_nTableSize; i++) {
        CKeyTableEntry* pEntry = &m_paEntries[i];
        if (pEntry == NULL)
            continue;
        unsigned short resType = pEntry->m_resType;
        if (resType != RESTYPE_INVALID) {
            if (bIncludeStatic || !g_pExoResMan->GetIsStaticType(resType))
                count++;
        }
    }
    return count;
}

CExoStringList* CExoKeyTable::GetKeysOfType(unsigned short resType)
{
    if (resType == RESTYPE_INVALID || m_nTableSize == 0)
        return NULL;

    CExoStringList* pList = new CExoStringList(0, 1, 5);
    for (unsigned int i = 0; i < m_nTableSize; i++) {
        CKeyTableEntry* pEntry = &m_paEntries[i];
        if (pEntry->m_resType == resType) {
            char buf[20];
            pEntry->m_resRef.CopyToString(buf);
            CExoString* pStr = new CExoString(buf);
            pList->Add(pStr);
        }
    }
    if (pList->GetCount() > 0)
        return pList;
    pList->~CExoStringList();
    operator delete(pList);
    return NULL;
}

#if KOTOR1  // K1 keeps no source file name per entry
CKeyTableEntry* CExoKeyTable::AddKey(const CResRef& resRef, unsigned short resType,
                                     unsigned int resID, int a)
#else
CKeyTableEntry* CExoKeyTable::AddKey(const CResRef& resRef, unsigned short resType,
                                     unsigned int resID, int a, CExoString sourceFile)
#endif
{
    K2_UNUSED(a);
    unsigned int index;
    unsigned int startIndex;
    unsigned int maskedResID = resID & ~(RESID_SLOT_MASK << RESID_SLOT_SHIFT);

recompute:
    startIndex = Hash(resRef, resType);
    if (resRef == const_cast<char*>(""))
        return 0;
    index = startIndex;
    do {
        if (m_paEntries[index].m_resRef == const_cast<char*>(""))
            goto insert_or_dup;
        if (m_paEntries[index].m_resRef == resRef && m_paEntries[index].m_resType == resType)
            goto insert_or_dup;
        unsigned int nextIndex = (index + 1 < m_nTableSize) ? index + 1 : 0;
        m_nAddKeyProbes++;
        index = nextIndex;
    } while (index != startIndex);
    goto recompute;

insert_or_dup:
    if (m_paEntries[index].m_dwResID != RESID_INVALID && m_paEntries[index].m_resRef != const_cast<char*>("")) {
        char msg[100] = "CExoKeyTable::AddKey: Duplicate Resource ";
        char resRefStr[20];
        m_paEntries[index].m_resRef.CopyToString(resRefStr);
        __builtin___strcat_chk(msg, resRefStr, sizeof(msg));
        __builtin___strcat_chk(msg, " - Ignoring Duplicate", sizeof(msg));
        return 0;
    }

    m_paEntries[index].m_dwResID = maskedResID | (m_nRoutingTag << 14);
    m_paEntries[index].m_resRef = resRef;
    m_paEntries[index].m_resType = resType;
#if !KOTOR1
    m_paEntries[index].m_sFileName = sourceFile;
#endif
    m_nAddKeyProbes++;
    return &m_paEntries[index];
}

int CExoKeyTable::LocateBifFile(const CExoString& path)
{
    CExoFile* pFile = new CExoFile(path, RESTYPE_BIF, CExoString("rb"));
    bool bOpened = pFile->FileOpened();
    pFile->~CExoFile();
    operator delete(pFile);
    return bOpened;
}

CKeyTableEntry* CExoKeyTable::AllocateTable(unsigned int n)
{
    CKeyTableEntry* entries = new CKeyTableEntry[n];
    CKeyTableEntry* p = entries;
    for (uint64_t i = 0; i < n; i++, p++) {
        p->m_resRef = const_cast<char*>("");
        p->m_pRes = NULL;
        p->m_dwResID = RESID_INVALID;
        // The reset writes the padding between m_dwResID and m_resType too, so the three stores
        // coalesce into one. Nothing reads those bytes.
        p->_pad1c[0] = 0;  // idiom-exempt: the original writes this byte too
        p->_pad1c[1] = 0;  // idiom-exempt: the original writes this byte too
        p->m_resType = RESTYPE_INVALID;
#if !KOTOR1
        p->m_sFileName = "";
#endif
    }
    return entries;
}

int CExoKeyTable::AddResourceImageContents(int)
{
    ResourceImageEntry entryBuf;
    CResRef resRef;

    m_nDependentFileCount = 1;
    m_paDependentFiles = static_cast<void**>(operator new[](8));

    m_paDependentFiles[0] = new CExoResourceImageFile();

    if (m_paDependentFiles[0] == NULL) {
        if (m_paDependentFiles != NULL)
            operator delete[](m_paDependentFiles);
        m_paDependentFiles = NULL;
        return 0;
    }

    static_cast<CExoResourceImageFile*>(m_paDependentFiles[0])->m_sFileName = m_sFileName;

    int bOpened = static_cast<CExoPackedFile*>(m_paDependentFiles[0])->OpenFile();
    if (bOpened == 0)
        return 0;  // sic: leaks the file object and the entry array

    CResourceImageHeader* pHeader = static_cast<CExoResourceImageFile*>(m_paDependentFiles[0])->m_pHeader;
#if KOTOR1  // K1 byte-swaps the header's count and entry offset in place
    ByteSwap(&pHeader->m_nEntryCount);
    ByteSwap(&pHeader->m_nEntriesOffset);
#endif
    if (pHeader->m_nEntryCount != 0) {
        m_nTableSize = static_cast<unsigned int>(pHeader->m_nEntryCount * 1.2);
        m_paEntries = AllocateTable(m_nTableSize);
        if (pHeader->m_nEntryCount != 0) {
            unsigned int i = 0;
            do {
                entryBuf = static_cast<CExoResourceImageFile*>(m_paDependentFiles[0])->GetKeyListEntry(i);
                resRef = reinterpret_cast<unsigned char*>(&entryBuf);  // the raw 16 on-disk bytes, no NUL scan
                entryBuf.m_resIDRaw = (entryBuf.m_resIDRaw & RESID_LOCAL_MASK)
                                      | ((m_nRoutingTag & RESID_FILE_MASK) << RESID_FILE_SHIFT)
                                      | RESID_IMAGE;
#if KOTOR1
                AddKey(resRef, entryBuf.m_resType, entryBuf.m_resIDRaw, 0);
#else
                AddKey(resRef, entryBuf.m_resType, entryBuf.m_resIDRaw, 0, CExoString(""));
#endif
                i++;
            } while (i < pHeader->m_nEntryCount);
        }
        return 1;
    }

    m_bValid = 0;
    delete static_cast<CExoPackedFile*>(m_paDependentFiles[0]);
    if (m_paDependentFiles != NULL)
        operator delete[](m_paDependentFiles);
    m_paDependentFiles = NULL;
    return 0;
}

int CExoKeyTable::AddEncapsulatedContents(int)
{
    CEncapsulatedFileHeader header;
    CEncapsulatedKeyEntry entry;
    CExoString unused1;
    CExoString unused2;
    CResRef resRef;

    int formatTag;
    CExoFile* pFile = new CExoFile(m_sFileName, RESTYPE_NWM, CExoString("rb"));
    if (pFile->FileOpened()) {
        formatTag = ENCAPSULATED_FORMAT_NWM;
    } else {
        pFile->~CExoFile();
        operator delete(pFile);
        pFile = new CExoFile(m_sFileName, RESTYPE_MOD, CExoString("rb"));
        if (pFile->FileOpened()) {
            formatTag = ENCAPSULATED_FORMAT_MOD;
        } else {
            pFile->~CExoFile();
            operator delete(pFile);
            pFile = new CExoFile(m_sFileName, RESTYPE_SAV, CExoString("rb"));
            if (pFile->FileOpened()) {
                formatTag = ENCAPSULATED_FORMAT_SAV;
            } else {
                pFile->~CExoFile();
                operator delete(pFile);
                pFile = new CExoFile(m_sFileName, RESTYPE_ERF, CExoString("rb"));
                if (pFile->FileOpened()) {
                    formatTag = ENCAPSULATED_FORMAT_ERF;
                } else {
                    pFile->~CExoFile();
                    operator delete(pFile);
                    pFile = new CExoFile(m_sFileName, RESTYPE_HAK, CExoString("rb"));
                    if (pFile->FileOpened()) {
                        formatTag = ENCAPSULATED_FORMAT_HAK;
                    } else {
                        pFile->~CExoFile();
                        operator delete(pFile);
                        m_bValid = 0;
                        return 0;
                    }
                }
            }
        }
    }

#if KOTOR1
    m_sFileName.CStr();  // K1: a stripped debug print's argument
#endif
    pFile->Read(&header, sizeof(header), 1);
#if KOTOR1  // K1: the read header words go through ByteSwap
    ByteSwap(&header.m_fileType);
    ByteSwap(&header.m_version);
    ByteSwap(&header.m_nEntryCount);
    ByteSwap(reinterpret_cast<unsigned int*>(&header.m_nOffsetToKeyList));
    ByteSwap(reinterpret_cast<unsigned int*>(&header.m_nOffsetToResourceList));
#endif

    if (formatTag == ENCAPSULATED_FORMAT_ERF) {
        if (header.m_fileType != FOURCC_ERF) goto fail;
    } else if (formatTag == ENCAPSULATED_FORMAT_HAK) {
        if (header.m_fileType != FOURCC_HAK) goto fail;
    } else if (header.m_fileType != FOURCC_MOD) {
        goto fail;
    }

    if (header.m_version != FOURCC_V1_0) goto fail;
    if (header.m_nEntryCount == 0) goto fail;

    m_nTableSize = static_cast<unsigned int>(header.m_nEntryCount * 1.2);
    m_paEntries = AllocateTable(m_nTableSize);
    pFile->Seek(header.m_nOffsetToKeyList, 0);
    if (header.m_nEntryCount != 0) {
        unsigned int i = 0;
        do {
            pFile->Read(&entry, sizeof(entry), 1);
#if KOTOR1
            ByteSwap(&entry.m_resIDRaw);
            ByteSwap(&entry.m_resType);
#endif
#if KOTOR1  // K1 names the raw bytes in a loop-body local
            unsigned char* pRaw = reinterpret_cast<unsigned char*>(&entry);
            resRef = pRaw;  // the raw 16 on-disk bytes, no NUL scan
#else
            resRef = reinterpret_cast<unsigned char*>(&entry);  // the raw 16 on-disk bytes, no NUL scan
#endif
            entry.m_resIDRaw = (entry.m_resIDRaw & RESID_LOCAL_MASK)
                               | ((m_nRoutingTag & RESID_FILE_MASK) << RESID_FILE_SHIFT)
                               | RESID_ENCAPSULATED;
#if KOTOR1
            AddKey(resRef, entry.m_resType, entry.m_resIDRaw, 0);
#else
            AddKey(resRef, entry.m_resType, entry.m_resIDRaw, 0, CExoString(""));
#endif
            i++;
        } while (i < header.m_nEntryCount);
    }
    pFile->~CExoFile();
    operator delete(pFile);

    m_nDependentFileCount = 1;
    m_paDependentFiles = new void*[1];

    m_paDependentFiles[0] = new CExoEncapsulatedFile();

    if (m_paDependentFiles[0] == NULL) {
        delete[] m_paDependentFiles;
        m_paDependentFiles = NULL;
        return 0;
    }

    static_cast<CExoEncapsulatedFile*>(m_paDependentFiles[0])->m_sFileName = m_sFileName;

    {
        static_cast<CExoPackedFile*>(m_paDependentFiles[0])->LoadHeader(static_cast<unsigned char>(formatTag));
    }
    return 1;

fail:
    m_bValid = 0;
    pFile->~CExoFile();
    operator delete(pFile);
    return 0;
}

int CExoKeyTable::AddKeyTableContents(int)
{
    CExoFile* pFile;
    KeyFileHeader header;
    unsigned int tableBufSize;
    void* pTableBuf;
    int colonPos;
    BifTableEntry* pBifEntry;
    unsigned int i;
    int dotPos;
    KeyTableEntry entry;
    CExoString driveStr;       // this table's own path, up to its first ':' (e.g. "CD0")
    CExoString bifNameStr;     // one BIF's raw file-table basename (reused/reassigned per BIF)
    CExoString fallbackDrive;  // "CD0:" fallback prefix when LocateBifFile can't find a BIF
    CResRef keyResRef;         // reused/reassigned per key-table record

    pFile = new CExoFile(m_sFileName, RESTYPE_KEY, CExoString("rb"));
    if (!pFile->FileOpened()) {
        m_bValid = 0;
        goto fail;
    }

#if KOTOR1
    m_sFileName.CStr();  // K1: a stripped debug print's argument
#endif
    pFile->Read(&header, sizeof(header), 1);
#if KOTOR1  // K1: the six header words go through ByteSwap
    ByteSwap(&header.m_fileType);
    ByteSwap(&header.m_fileVersion);
    ByteSwap(&header.m_nBifCount);
    ByteSwap(&header.m_nKeyCount);
    ByteSwap(reinterpret_cast<unsigned int*>(&header.m_nOffsetToFileTable));
    ByteSwap(reinterpret_cast<unsigned int*>(&header.m_nOffsetToKeyTable));
#endif
    if (header.m_fileType != FOURCC_KEY) {
        m_bValid = 0;
        goto fail;
    }
    if (header.m_fileVersion != FOURCC_V1) {
        m_bValid = 0;
        goto fail;
    }
    if (header.m_nBifCount == 0 && header.m_nKeyCount == 0) {
        m_bValid = 0;
        goto fail;
    }

    m_nTableSize = static_cast<unsigned int>(static_cast<float>(header.m_nKeyCount) / 0.85) + 1023;
    m_paEntries = AllocateTable(m_nTableSize);

    {
        tableBufSize = header.m_nOffsetToKeyTable - header.m_nOffsetToFileTable;
#if KOTOR1
        pTableBuf = ExoMalloc(tableBufSize);
#else
        pTableBuf = EXOMALLOC(tableBufSize,
            "C:/Users/Vu/Documents/AndroidStudioProjects/KOTOR1-2/KOTOR2_Android/app/src/main/cpp/"
            "kotor2/CODE/libsource/exoresman/exoresfile.cpp", 991);
#endif
        if (pTableBuf == NULL) {
            m_bValid = 0;
            delete[] m_paEntries;
            m_paEntries = NULL;
            goto fail;
        }

        pFile->Seek(header.m_nOffsetToFileTable, 0);
        pFile->Read(pTableBuf, tableBufSize, 1);

        colonPos = m_sFileName.Find(':', 0);
        if (colonPos < 1)
            return 0;  // sic: leaks the file, the table buffer and the entry array
        driveStr = m_sFileName.SubString(0, colonPos);

        m_nDependentFileCount = header.m_nBifCount;
        m_paDependentFiles = new void*[header.m_nBifCount];

        if (header.m_nBifCount != 0) {
            pBifEntry = static_cast<BifTableEntry*>(pTableBuf);
            for (i = 0; i < header.m_nBifCount; i++, pBifEntry++) {
#if KOTOR1
                ByteSwap(&pBifEntry->m_nFileSize);
                ByteSwap(&pBifEntry->m_nFilenameOffset);
                ByteSwap(&pBifEntry->m_nFilenameSize);
                ByteSwap(&pBifEntry->m_nDrives);
#endif
                m_paDependentFiles[i] = new CExoResFile();

                if (m_paDependentFiles[i] == NULL) {
                    delete[] m_paDependentFiles;
                    m_paDependentFiles = NULL;
                    delete[] m_paEntries;
                    m_paEntries = NULL;
#if KOTOR1
                    ExoFree(pTableBuf);
#else
                    EXOFREE(pTableBuf,
                        "C:/Users/Vu/Documents/AndroidStudioProjects/KOTOR1-2/KOTOR2_Android/app/"
                        "src/main/cpp/kotor2/CODE/libsource/exoresman/exoresfile.cpp", 1081);
#endif
                    goto fail;
                }

                static_cast<CExoResFile*>(m_paDependentFiles[i])->m_nDrives = pBifEntry->m_nDrives;
                static_cast<CExoResFile*>(m_paDependentFiles[i])->m_nFileSize = pBifEntry->m_nFileSize;
                pBifEntry->m_nFilenameOffset -= header.m_nOffsetToFileTable;
                bifNameStr = static_cast<const char*>(pTableBuf) + pBifEntry->m_nFilenameOffset;
#if KOTOR1
                bifNameStr.CStr();  // K1: a stripped debug print's argument
#endif

                dotPos = bifNameStr.Find('.', 0);
                if (dotPos < 0) {
                    delete[] m_paDependentFiles;
                    m_paDependentFiles = NULL;
                    delete[] m_paEntries;
                    m_paEntries = NULL;
#if KOTOR1
                    ExoFree(pTableBuf);
#else
                    EXOFREE(pTableBuf,
                        "C:/Users/Vu/Documents/AndroidStudioProjects/KOTOR1-2/KOTOR2_Android/app/"
                        "src/main/cpp/kotor2/CODE/libsource/exoresman/exoresfile.cpp", 1119);
#endif
                    goto fail;
                }

                static_cast<CExoResFile*>(m_paDependentFiles[i])->m_sFileName =
                    driveStr + CExoString(":") + bifNameStr.SubString(0, dotPos);

                if (static_cast<CExoResFile*>(m_paDependentFiles[i])->m_nDrives != 1
                    && !LocateBifFile(static_cast<CExoResFile*>(m_paDependentFiles[i])->m_sFileName)) {
                    fallbackDrive = "CD0:";
                    static_cast<CExoResFile*>(m_paDependentFiles[i])->m_sFileName =
                        fallbackDrive + bifNameStr.SubString(0, dotPos);
                }

                static_cast<CExoPackedFile*>(m_paDependentFiles[i])->LoadHeader(0);
            }
        }

        pFile->Seek(header.m_nOffsetToKeyTable, 0);
        if (header.m_nKeyCount != 0) {
            for (i = 0; i < header.m_nKeyCount; i++) {
                pFile->Read(entry.m_resRef, sizeof(entry.m_resRef), 1);
                pFile->Read(&entry.m_resType, 2, 1);
                pFile->Read(&entry.m_resID, 4, 1);
#if KOTOR1
                ByteSwap(&entry.m_resType);
                ByteSwap(&entry.m_resID);
#endif
                keyResRef = entry.m_resRef;
                entry.m_resID &= ~RESID_SOURCE_BITS;
#if KOTOR1
                AddKey(keyResRef, entry.m_resType, entry.m_resID, 0);
#else
                AddKey(keyResRef, entry.m_resType, entry.m_resID, 0, CExoString(""));
#endif
            }
        }

        pFile->~CExoFile();
        operator delete(pFile);
#if KOTOR1
        ExoFree(pTableBuf);
#else
        EXOFREE(pTableBuf,
            "C:/Users/Vu/Documents/AndroidStudioProjects/KOTOR1-2/KOTOR2_Android/app/src/main/cpp/"
            "kotor2/CODE/libsource/exoresman/exoresfile.cpp", 1213);
#endif
        return 1;
    }

fail:
    pFile->~CExoFile();
    operator delete(pFile);
    return 0;
}

int CExoKeyTable::BuildNewTable(unsigned int routingType, const CExoString& name,
                                 unsigned int routingTag, int arg)
{
    if (name == "")
        return 0;

    m_sFileName = name;

    switch (routingType) {
    case KEYTABLE_SOURCE_FIXED:
        m_nRoutingTag = routingTag;
        m_nSourceType = KEYTABLE_SOURCE_FIXED;
        return AddKeyTableContents(arg);
    case KEYTABLE_SOURCE_DIRECTORY:
        m_nRoutingTag = routingTag;
        m_nSourceType = KEYTABLE_SOURCE_DIRECTORY;
        return AddDirectoryContents(arg);
    case KEYTABLE_SOURCE_ENCAPSULATED:
        m_nRoutingTag = routingTag;
        m_nSourceType = KEYTABLE_SOURCE_ENCAPSULATED;
        return AddEncapsulatedContents(arg);
    case KEYTABLE_SOURCE_IMAGE:
        m_nRoutingTag = routingTag;
        m_nSourceType = KEYTABLE_SOURCE_IMAGE;
        return AddResourceImageContents(arg);
    default:
        return 0;
    }
}

void CExoKeyTable::DestroyTable()
{
    m_bLookupDisabled = 1;
    if (m_nTableSize == 0) {
        return;
    }
    for (unsigned int i = 0; i < m_nTableSize; i++) {
        CKeyTableEntry* pEntry = &m_paEntries[i];
        if (pEntry->m_pRes == NULL) {
            continue;
        }

        CRes* pRes;
        if ((pEntry->m_dwResID >> RESID_SOURCE_SHIFT) == RESSOURCE_IMAGE) {
            pRes = pEntry->m_pRes;
        } else {
            CExoKeyTable* pFoundTable = NULL;
            CKeyTableEntry* pFoundEntry = NULL;
            g_pExoResMan->GetKeyEntry(pEntry->m_resRef, pEntry->m_resType, &pFoundTable, &pFoundEntry);
            pRes = pEntry->m_pRes;
            if (pFoundEntry != NULL) {
                pFoundEntry->m_pRes = pRes;
                pRes->m_dwResID = pFoundEntry->m_dwResID;
                pRes->m_pKeyEntry = pFoundEntry;
                pEntry->m_pRes = NULL;
                pRes = pFoundEntry->m_pRes;
                if (pRes->m_nDemandCount != 0) {
                    pRes->m_dwFlags |= CRes::RESFLAG_FREE_PENDING;
                } else {
                    pRes->Dump(0);
                }
                continue;
            }
        }

        if (pRes->m_nDemandCount == 0) {
            pRes->Dump(0);
            pEntry->m_pRes->m_pKeyEntry = NULL;
            pEntry->m_pRes->m_dwResID = RESID_INVALID;
            continue;
        }

        CExoString msg;
        pEntry->m_resRef.CopyToString(msg);
        char buf[256];
        __builtin___sprintf_chk(buf, 0, sizeof(buf),
            "CExoKeyTable::DestroyTable: Resource %s still in demand during table deletion",
            msg.CStr());
        pRes->m_dwFlags |= CRes::RESFLAG_ORPHANED;
    }
}
void CExoKeyTable::RebuildTable()
{
    CKeyTableEntry* pFoundEntry = NULL;
    CExoKeyTable* pFoundTable = NULL;

    CKeyTableEntry* oldEntries = m_paEntries;
    if (oldEntries == NULL) {
        if (m_nSourceType != KEYTABLE_SOURCE_NONE && m_sFileName != "") {
            BuildNewTable(m_nSourceType, m_sFileName, m_nRoutingTag, 1);
        }
        return;
    }

    unsigned int oldTableSize = m_nTableSize;

    if (m_nSourceType == KEYTABLE_SOURCE_IMAGE && m_paDependentFiles != NULL) {
        static_cast<CExoPackedFile*>(m_paDependentFiles[0])->CloseFile();
    }

#if !KOTOR1
#ifdef K2_FIX_ENGINE_BUGS
    printf("#### Rebuild Table: %p %s\n", static_cast<void*>(m_paEntries), m_sFileName.CStr());
#else
    printf("#### Rebuild Table: %x %s\n", m_paEntries, m_sFileName.CStr());
#endif
#endif

    unsigned int sourceType = m_nSourceType;
    unsigned int routingTag = m_nRoutingTag;
    m_nTableSize = 0;
    m_paEntries = NULL;
    BuildNewTable(sourceType, m_sFileName, routingTag, 0);

    m_bLookupDisabled = 1;

    if (oldTableSize != 0) {
        for (unsigned int i = 0; i < oldTableSize; i++) {
            CKeyTableEntry* pOld = &oldEntries[i];
            if (pOld->m_pRes == NULL) {
                continue;
            }

            unsigned short oldResType = pOld->m_resType;
            int bMoved = 0;

            if (m_paEntries != NULL && m_nTableSize != 0) {
                for (unsigned int j = 0; j < m_nTableSize; j++) {
                    if (m_paEntries[j].m_resType == oldResType &&
                        m_paEntries[j].m_resRef == pOld->m_resRef) {
                        m_paEntries[j].m_pRes = pOld->m_pRes;
                        m_paEntries[j].m_pRes->m_dwResID = m_paEntries[j].m_dwResID;
                        pOld->m_pRes = NULL;
                        m_paEntries[j].m_pRes->m_pKeyEntry = &m_paEntries[j];
                        bMoved = 1;
                        break;
                    }
                }
            }
            if (bMoved) {
                continue;
            }

            g_pExoResMan->GetKeyEntry(pOld->m_resRef, pOld->m_resType, &pFoundTable, &pFoundEntry);
            if (pFoundEntry != NULL) {
                pFoundEntry->m_pRes = pOld->m_pRes;
                pFoundEntry->m_pRes->m_dwResID = pFoundEntry->m_dwResID;
                pOld->m_pRes = NULL;
                pFoundEntry->m_pRes->m_pKeyEntry = pFoundEntry;
                if (pFoundEntry->m_pRes->m_nDemandCount != 0) {
                    pFoundEntry->m_pRes->m_dwFlags |= CRes::RESFLAG_FREE_PENDING;
                } else {
                    pFoundEntry->m_pRes->Dump(0);
                }
                continue;
            }

            if (pOld->m_pRes->m_nDemandCount == 0) {
                pOld->m_pRes->Dump(0);
                // The orphan is detached from the old entry, which is deleted below.
                pOld->m_pRes->m_pKeyEntry = NULL;
                pOld->m_pRes->m_dwResID = RESID_INVALID;
            } else {
                pOld->m_pRes->m_dwFlags |= CRes::RESFLAG_ORPHANED;
            }
        }
    }

    delete[] oldEntries;

    if (m_nTableSize != 0) {
        for (unsigned int j = 0; j < m_nTableSize; j++) {
            if (m_paEntries[j].m_pRes != NULL) {
                continue;
            }

            g_pExoResMan->GetKeyEntry(m_paEntries[j].m_resRef, m_paEntries[j].m_resType,
                                      &pFoundTable, &pFoundEntry);
            if (pFoundEntry == NULL || pFoundEntry->m_pRes == NULL) {
                continue;
            }
            if (pFoundTable->m_nRoutingTag >= m_nRoutingTag) {
                continue;
            }

            if (pFoundEntry->m_pRes->m_nDemandCount != 0) {
                pFoundEntry->m_pRes->m_dwFlags |= CRes::RESFLAG_FREE_PENDING;
            } else {
                pFoundEntry->m_pRes->Dump(0);
            }
            m_paEntries[j].m_pRes = pFoundEntry->m_pRes;
            m_paEntries[j].m_pRes->m_dwResID = m_paEntries[j].m_dwResID;
            pFoundEntry->m_pRes = NULL;
            m_paEntries[j].m_pRes->m_pKeyEntry = &m_paEntries[j];
        }
    }

    m_bLookupDisabled = 0;
}
unsigned int CExoResFile::GetResourceSize(unsigned int resId)
{
    return m_paEntries[resId & RESID_INDEX_MASK].m_nResourceSize;
}
