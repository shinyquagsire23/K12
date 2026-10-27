#include "k2stdint.h"
#include "rimfile.h"

#include "exobase.h"
#include "exofile.h"
#include "exolinkedlistinternal.h"
#include "resgff.h"
#include "exoresman.h"

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "restype.h"

#define CRIM_MODE_READ  "rb"
#define CRIM_MODE_WRITE "wb"
#define CRIM_RES_TYPE 0xbc1
#define CRIM_FILLER 0xbaadf00d
#define CRIM_ENTRY_SIZE 0x20
#define CRIM_HEADER_SIZE 0x78

// A block-aligned archive starts each payload on this boundary (an unaligned one on 4 bytes).
enum {
    RIM_BLOCK_ALIGN = 128,
};

CRIMRes::CRIMRes()
{
    m_nID = 0;
    m_nOffset = 0;
    m_nSize = 0;
    m_pData = 0;
    __builtin_memset(m_szName, 0, sizeof(m_szName));
    m_nType = RESTYPE_INVALID;
}

CRIMRes::~CRIMRes()
{
    if (m_pData != 0)
    {
        // Allocated with `new unsigned char[]` in Read and freed with scalar `operator delete`.
        delete static_cast<unsigned char*>(m_pData);
        m_pData = 0;
    }
    m_nType = RESTYPE_INVALID;
    m_nID = 0;
    m_nOffset = 0;
    m_nSize = 0;
    __builtin_memset(m_szName, 0, sizeof(m_szName));
}

int CRIMRes::Reset()
{
    if (m_pData != 0)
    {
        // Scalar `operator delete`, as in the destructor.
        delete static_cast<unsigned char*>(m_pData);
        m_pData = 0;
    }
    m_nType = RESTYPE_INVALID;
    m_nID = 0;
    m_nOffset = 0;
    m_nSize = 0;
    __builtin_memset(m_szName, 0, sizeof(m_szName));
    return 1;
}

void CRIMRes::SetName(CExoString& sName)
{
    __builtin_memset(m_szName, 0, sizeof(m_szName));
    char* pszName = sName.CStr();
#if KOTOR1  // K1: CExoString::GetLength is out of line, and the memcpy is fortified
    int nLength = sName.GetLength();
#ifdef K2_FIX_ENGINE_BUGS
    if (nLength > static_cast<int>(sizeof(m_szName)))
        nLength = sizeof(m_szName);
    memcpy(m_szName, pszName, nLength);  // idiom-exempt: runtime length (clamped to the slot)
#else
    if (__builtin_object_size(m_szName, 0) != static_cast<uint64_t>(-1))
        __memcpy_chk(m_szName, pszName, nLength, __builtin_object_size(m_szName, 0));
    else
        memcpy(m_szName, pszName, nLength);  // idiom-exempt: runtime length
#endif
#else
    int nLength = sName.m_sString ? static_cast<int>(strlen(sName.m_sString)) : 0;
#ifdef K2_FIX_ENGINE_BUGS
    if (nLength > static_cast<int>(sizeof(m_szName)))
        nLength = sizeof(m_szName);
#endif
    memcpy(m_szName, pszName, nLength);  // idiom-exempt: runtime length (source string's; m_szName is the fixed 16-byte slot)
#endif
}

int CRIMRes::Read(CExoFile& file)
{
    unsigned short nTypePad;
    file.Read(m_szName, sizeof(m_szName), 1);
    file.Read(&m_nType, 2, 1);
    file.Read(&nTypePad, 2, 1);   // the on-disk type field is 4 bytes; the high half is discarded
    file.Read(&m_nID, 4, 1);
    file.Read(&m_nOffset, 4, 1);
    file.Read(&m_nSize, 4, 1);
    m_pData = new unsigned char[m_nSize];
    int nPos = file.GetOffset();
    file.Seek(m_nOffset, 0);
    file.Read(m_pData, m_nSize, 1);
    file.Seek(nPos, 0);
    return 1;
}

int CRIMRes::Write(CExoFile& file, unsigned int nEntryOffset, unsigned char bBlockAlign)
{
    // The payload is bracketed by six leading and ten trailing zero bytes.
    unsigned char cZero = 0;
    unsigned short nZero = 0;

    for (int i = 0; i < 6; i++)
        file.Write(&cZero, 1, 1);

    unsigned int nOffset = file.GetOffset();
    int nAlign = (bBlockAlign != 0) ? RIM_BLOCK_ALIGN : 4;
    unsigned int nRemainder = nOffset & static_cast<unsigned int>(nAlign - 1);
    int nPad = nAlign - static_cast<int>(nRemainder);
    if (static_cast<int>(nRemainder) > 0 && nPad > 0)
    {
        for (int i = 0; i < nPad; i++)
            file.Write(&cZero, 1, 1);
    }

    // The payload's real offset, stamped back into the record.
    m_nOffset = file.GetOffset();
    file.Write(m_pData, m_nSize, 1);

    for (int i = 0; i < 10; i++)
        file.Write(&cZero, 1, 1);

    // Now seek back to the entry slot and stamp the record.
    file.Seek(nEntryOffset, 0);
    file.Write(m_szName, sizeof(m_szName), 1);
    file.Write(&m_nType, 2, 1);
    file.Write(&nZero, 2, 1);   // the type field's high half is always zero on disk
    file.Write(&m_nID, 4, 1);
    file.Write(&m_nOffset, 4, 1);
    file.Write(&m_nSize, 4, 1);
    file.SeekEnd();
    return 1;
}

CRIMFile::CRIMFile()
{
    __builtin_memset(&m_header, 0, sizeof(m_header));
    m_pFile = 0;
    m_nRezCount = 0;
    m_nWriteIndex = 0;
}

CRIMFile::~CRIMFile()
{
    Reset();
}

int CRIMFile::Reset()
{
    __builtin_memset(&m_header, 0, sizeof(m_header));

    // Drain the entry list, freeing each record's payload first. The name is left alone here,
    // unlike in CRIMRes::Reset, because the record is about to be deleted.
    if (m_lResources.m_pInternal->m_pHead != 0)
    {
        CRIMRes* pRes = static_cast<CRIMRes*>(m_lResources.m_pInternal->RemoveHead());
        while (true)
        {
            if (pRes != 0)
            {
                if (pRes->m_pData != 0)
                {
                    // Freed with scalar `operator delete`, matching the `new[]` in CRIMRes::Read.
                    delete static_cast<unsigned char*>(pRes->m_pData);
                    pRes->m_pData = 0;
                }
                pRes->m_nType = RESTYPE_INVALID;
                pRes->m_nID = 0;
                pRes->m_nOffset = 0;
                pRes->m_nSize = 0;
                // The record is released without running ~CRIMRes.
                operator delete(pRes);
            }
            if (m_lResources.m_pInternal->m_pHead == 0)
                break;
            pRes = static_cast<CRIMRes*>(m_lResources.m_pInternal->RemoveHead());
        }
    }
    return 1;
}

void CRIMFile::SetVersion(char* pszVersion)
{
    __builtin_memcpy(m_header.m_szVersion, pszVersion, sizeof(m_header.m_szVersion));
}

int CRIMFile::Read(const CExoString& sFileName)
{
    CExoFile file(sFileName, CRIM_RES_TYPE, CExoString(CRIM_MODE_READ));
    if (file.FileOpened() != 0)
    {
        file.Read(m_header.m_szVersion, sizeof(m_header.m_szVersion), 1);
        file.Read(&m_header.m_nReserved1, sizeof(m_header.m_nReserved1), 1);
        file.Read(&m_header.m_nNumEntries, sizeof(m_header.m_nNumEntries), 1);
        file.Read(&m_header.m_nDataOffset, sizeof(m_header.m_nDataOffset), 1);
        file.Read(&m_header.m_bBlockAlign, sizeof(m_header.m_bBlockAlign), 1);
        file.Read(m_header.m_aReserved2, sizeof(m_header.m_aReserved2), 1);

        if (static_cast<unsigned int>(m_header.m_nNumEntries) != 0)
        {
            unsigned int i = 0;
            do
            {
                CRIMRes* pRes = new CRIMRes();
                pRes->Read(file);
                m_lResources.AddTail(pRes);
                i++;
            } while (i < static_cast<unsigned int>(m_header.m_nNumEntries));
        }
    }
    return 1;
}

int CRIMFile::Create(CExoString& sFileName)
{
    m_pFile = new CExoFile(sFileName, CRIM_RES_TYPE, CExoString(CRIM_MODE_WRITE));
    return m_pFile->FileOpened() != 0;
}

int CRIMFile::WriteHeader()
{
    if (m_pFile == 0)
        return 0;
    m_header.m_bBlockAlign = 0;
    m_header.m_nReserved1 = 1;
    m_pFile->Write(&m_header, sizeof(m_header), 1);
    return 1;
}

int CRIMFile::SetNumEntries(unsigned int nNumEntries)
{
    if (m_pFile == 0)
        return 0;

    m_nRezCount = nNumEntries;
    m_nWriteIndex = 0;

    // Patch the entry count in the already-written header, then stamp the entry table's offset: the
    // table follows the header, and the cursor sits 16 bytes in, just past the count.
    m_pFile->Seek(__builtin_offsetof(CRIMHeader, m_nNumEntries), 0);
    m_pFile->Write(&nNumEntries, 4, 1);
    int nResult = 1;
    m_nEntriesOffset = m_pFile->GetOffset() + (CRIM_HEADER_SIZE - 16);
    m_pFile->Write(&m_nEntriesOffset, 4, 1);
    m_pFile->SeekEnd();

    // Reserve the entry slots with the filler. The 16-byte name slot written here is never
    // initialised.
    unsigned int nFiller = CRIM_FILLER;
    char aBlank[16];
    for (unsigned int i = 0; i < nNumEntries; i++)
    {
        m_pFile->Write(aBlank, sizeof(aBlank), 1);
        m_pFile->Write(&nFiller, 4, 1);
        m_pFile->Write(&nFiller, 4, 1);
        m_pFile->Write(&nFiller, 4, 1);
        m_pFile->Write(&nFiller, 4, 1);
    }
    return nResult;
}

int CRIMFile::WriteResource(const CExoString& sResRef)
{
    if (m_nWriteIndex >= m_nRezCount)
        return 0;
    if (m_pFile == 0)
        return 0;

    CRIMRes res;

    // The resref is opened as a raw file so it can be measured and slurped.
    CExoFile* pFile = new CExoFile(sResRef, RESTYPE_INVALID, CExoString(CRIM_MODE_READ));

    // Split "prefix:name.ext" into a base name and an extension.
    int nColon = sResRef.Find(CExoString(":"), 0);
#if KOTOR1  // K1: CExoString::GetLength is out of line
    CExoString sName = sResRef.Right(sResRef.GetLength() - nColon - 1);
#else
    int nNameLen;
#ifdef K2_FIX_ENGINE_BUGS
    nNameLen = 0;
#endif
    if (sResRef.m_sString != 0)
        nNameLen = static_cast<int>(strlen(sResRef.m_sString));
    CExoString sName = sResRef.Right(nNameLen - nColon - 1);
#endif

    int nDot = sName.Find(CExoString("."), 0);
#if KOTOR1  // K1: CExoString::GetLength is out of line
    CExoString sExt = sName.Right(sName.GetLength() - nDot - 1);
#else
    int nExtLen;
#ifdef K2_FIX_ENGINE_BUGS
    nExtLen = 0;
#endif
    if (sName.m_sString != 0)
        nExtLen = static_cast<int>(strlen(sName.m_sString));
    CExoString sExt = sName.Right(nExtLen - nDot - 1);
#endif
    sName = sName.Left(nDot);

    unsigned short nType = g_pExoBase->GetResTypeFromExtension(sExt);

    res.SetName(sName);
    res.m_nID = m_nWriteIndex;
    res.m_nType = nType;

    unsigned int nSize = pFile->GetSize();
    unsigned char* pData = new unsigned char[nSize];
    pFile->Read(pData, 1, nSize);
    res.m_nSize = nSize;
    res.m_pData = pData;
    pFile->~CExoFile();
    operator delete(pFile);

    res.Write(*m_pFile, m_nEntriesOffset + m_nWriteIndex * CRIM_ENTRY_SIZE, m_header.m_bBlockAlign);
    m_pFile->SeekEnd();
    m_nWriteIndex++;
    return 1;
}

int CRIMFile::WriteResource(char* pszName, unsigned short nType, CRes* pRes, int bGFF)
{
    CRIMRes res;
    if (m_nWriteIndex >= m_nRezCount)
        return 0;
    if (m_pFile == 0)
        return 0;

    CExoString sName(pszName);
    int nSize;
    unsigned char* pData;
    res.SetName(sName);
    res.m_nID = m_nWriteIndex;
    res.m_nType = nType;

    if (bGFF == 0)
    {
        // For a plain payload, measure, copy and hand the record to CRIMRes::Write.
        res.m_nSize = pRes->GetSize();
        nSize = pRes->GetSize();
        pData = new unsigned char[nSize];
        memcpy(pData, pRes->GetData(), pRes->GetSize());  // idiom-exempt: runtime length (resource size)
        res.m_pData = pData;
        res.Write(*m_pFile, m_nEntriesOffset + m_nWriteIndex * CRIM_ENTRY_SIZE, m_header.m_bBlockAlign);
    }
    else
    {
        // The GFF arm writes the entry out of locals rather than through CRIMRes::Write.
        int nDataOffset = m_pFile->GetOffset();
        unsigned char cZero = 0;
        unsigned short nZero = 0;
        int nAlign;
        unsigned int nRemainder;
        unsigned int nGFFSize;

        CResGFF* pGFF = static_cast<CResGFF*>(pRes);
        pGFF->Pack(0, 0);
        pGFF->GetTotalSize();

        for (int i = 0; i < 6; i++)
            m_pFile->Write(&cZero, 1, 1);

        nAlign = (m_header.m_bBlockAlign != 0) ? RIM_BLOCK_ALIGN : 4;
        nRemainder = m_pFile->GetOffset() & static_cast<unsigned int>(nAlign - 1);
        nAlign -= static_cast<int>(nRemainder);
        if (nRemainder != 0 && nAlign != 0)
        {
            do
            {
                m_pFile->Write(&cZero, 1, 1);
            } while (--nAlign != 0);
        }

        nDataOffset = m_pFile->GetOffset();
        nGFFSize = 0;
        pGFF->WriteGFFData(*m_pFile, nGFFSize);

        for (int i = 0; i < 10; i++)
            m_pFile->Write(&cZero, 1, 1);

        // Then the entry itself, the name from the CExoString followed by the locals.
        char szEntryName[16];
        __builtin_strncpy(szEntryName, sName.CStr(), sizeof(szEntryName));
        m_pFile->Seek(m_nEntriesOffset + m_nWriteIndex * CRIM_ENTRY_SIZE, 0);
        m_pFile->Write(szEntryName, sizeof(szEntryName), 1);
        m_pFile->Write(&nType, 2, 1);
        m_pFile->Write(&nZero, 2, 1);
        m_pFile->Write(&m_nWriteIndex, 4, 1);
        m_pFile->Write(&nDataOffset, 4, 1);
        m_pFile->Write(&nGFFSize, 4, 1);
    }

    m_pFile->SeekEnd();
    m_nWriteIndex++;
    return 1;
}

int CRIMFile::Finish()
{
    m_pFile->Flush();
    if (m_pFile != 0)
        delete m_pFile;
    m_pFile = 0;
    return 1;
}

int CRIMFile::Write(CExoString& sFileName)
{
    K2_UNUSED(sFileName);
    return 0;
}

void CRIMFile::RecalculateOffsets()
{
}

void CRIMFile::ExportFiles(const CExoString& sPath)
{
    CExoString sFilePath;
    CExoLinkedListNode* pNode = m_lResources.GetHeadPos();
    CRIMRes* pRes;
    CExoFile* pFile;

    if (m_header.m_nNumEntries != 0)
    {
        for (unsigned int i = 1; ; i++)
        {
            pRes = m_lResources.GetAtPos(pNode);
            sFilePath = sPath + CExoString(pRes->m_szName);

            pFile = new CExoFile(sFilePath, pRes->m_nType, CExoString(CRIM_MODE_WRITE));
            pFile->Write(pRes->m_pData, pRes->m_nSize, 1);
            pFile->Flush();
            pFile->~CExoFile();
            operator delete(pFile);

            m_lResources.GetNext(pNode);
            if (i >= static_cast<unsigned int>(m_header.m_nNumEntries))
                break;
        }
    }
}
