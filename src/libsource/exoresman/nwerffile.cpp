#include "k2stdint.h"
#include "nwerffile.h"

CERFFile::CERFFile()
{
    __builtin_memset(&m_header, 0, sizeof(m_header));
    m_pFile = 0;
    m_nEntryCount = 0;
    m_nNextEntry = 0;
}


CERFFile::~CERFFile()
{
    Reset();
}

#include "appmanager.h"
#include "exobase.h"
#include "CExoArrayListDtors.h"
#include "exofile.h"
#include "exoresman.h"
#include "resgff.h"
#include "restype.h"

// The on-disk sizes: the header, and a key-table entry (a 16-byte name, the id, the type and two
// unused bytes).
enum {
    ERF_HEADER_SIZE = 160,
    ERF_KEY_ENTRY_SIZE = 24,
};
K2_LAYOUT_ASSERT(sizeof(CERFHeader) == ERF_HEADER_SIZE, "the ERF header image");

// The copy loops' chunk, the size of the app manager's scratch buffer (CopyToFile's; AddResource
// copies one byte less per pass).
enum {
    ERF_COPY_CHUNK = CAppManager::LARGE_TEMP_BUFFER_SIZE,
};

// The words a resource-table entry holds until its resource is written: Write's and
// SetNumEntries'.
enum : unsigned int {
    ERF_RES_ENTRY_PLACEHOLDER = 0xabcdabcd,
    ERF_RES_ENTRY_CANARY = 0xbaadf00d,
};

#include "port/libcbase.h"
#include "strref.h"
#include "port/libcstring.h"
#include "port/libcmath.h"
#include "port/libctime.h"
#include "exostringinline.h"

int CERFFile::Reset()
{
    __builtin_memset(&m_header, 0, sizeof(m_header));

    // Each list owns its element records, so every node's payload is freed here.
    while (m_lstStrings.m_pInternal->m_pHead) {
        CERFString* pString = static_cast<CERFString*>(m_lstStrings.RemoveHead());
        if (pString) {
            delete pString;
        }
    }
    while (m_lstKeys.m_pInternal->m_pHead) {
        CERFKey* pKey = static_cast<CERFKey*>(m_lstKeys.RemoveHead());
        if (pKey) {
            delete pKey;
        }
    }
    while (m_lstRes.m_pInternal->m_pHead) {
        CERFRes* pRes = static_cast<CERFRes*>(m_lstRes.RemoveHead());
        if (pRes) {
            delete pRes;
        }
    }
    return 1;
}

void CERFFile::SetVersion(char* version)
{
    __builtin_memcpy(&m_header.m_nSignature, version, sizeof(m_header.m_nSignature));
}

// The path-taking overload splits the path into a stem and an extension, opens the file read-only,
// reads its whole body into a fresh CERFRes and appends the pair to the two lists. A file that will
// not open returns 0 and leaves the three allocations behind.
int CERFFile::AddResource(CExoString& filePath)
{
    CExoString sExt;
    CExoString sName;
    // Declared first and assigned below so the three allocations share one cleanup scope.
    CERFKey* pKey;
    CERFRes* pRes;
    CExoFile* pFile;
    int nColon;
    int nDot;
    unsigned short nResType;
    pKey = new CERFKey();
    pRes = new CERFRes();
    pFile = new CExoFile(filePath, RESTYPE_INVALID, CExoString("rb"));

    if (pFile->FileOpened() == 0) {
        return 0;
    }

    nColon = filePath.Find(CExoString(":"), 0);
    sName = filePath.Right(
        filePath.GetLength() - nColon - 1);
    nDot = sName.Find(CExoString("."), 0);
    sExt = sName.Right(
        sName.GetLength() - nDot - 1);
    sName = sName.Left(nDot);

    nResType = g_pExoBase->GetResTypeFromExtension(sExt);
#ifdef K2_FIX_ENGINE_BUGS
    pKey->SetName(sName);
#elif KOTOR1  // K1: CExoString::GetLength is out of line
    __memcpy_chk(pKey->m_sName, sName.CStr(), sName.GetLength(),
                 __builtin_object_size(pKey->m_sName, 0));  // idiom-exempt: a runtime byte count, not a sizeof
#else
    __memcpy_chk(pKey->m_sName, sName.CStr(), sName.m_sString ? static_cast<int>(strlen(sName.m_sString)) : 0,
                 __builtin_object_size(pKey->m_sName, 0));  // idiom-exempt: a runtime byte count, not a sizeof
#endif
    pKey->m_nID = m_header.m_nKeyCount;
    pKey->m_nType = nResType;

    pRes->m_nSize = pFile->GetSize();
    pRes->m_pData = new unsigned char[pRes->m_nSize];
    pFile->Read(pRes->m_pData, 1, pRes->m_nSize);

    m_lstKeys.AddTail(pKey);
    m_lstRes.AddTail(pRes);
    m_header.m_nKeyCount++;

    delete pFile;
    return 1;
}

// Both element records are heap-allocated here and handed to the two lists; the key's id is the
// current key count, i.e. the index it will occupy, which is why the count is bumped last. The
// name copy is the archive's fixed 16-byte field.
int CERFFile::AddResource(char* sName, unsigned short nType, CRes* pRes)
{
    CERFKey* pKey = new CERFKey();
    CERFRes* pNew = new CERFRes();

    memcpy(pKey->m_sName, sName, 16);  // idiom-exempt: the on-disk name field is a fixed 16 bytes
    pKey->m_nID = m_header.m_nKeyCount;
    pKey->m_nType = nType;

    unsigned int nSize = pRes->GetSize();
    pNew->m_nSize = nSize;
    pNew->m_pData = new unsigned char[nSize];
    memcpy(pNew->m_pData, pRes->GetData(), nSize);  // idiom-exempt: a runtime byte count, not a sizeof

    m_lstKeys.m_pInternal->AddTail(pKey);
    m_lstRes.m_pInternal->AddTail(pNew);
    m_header.m_nKeyCount++;
    return 1;
}

// Walks the key list for the first entry with the wanted type (checked first and &&-short-circuited)
// and a matching name, then tears down both records: the resource node sits at the same index in
// the res list, hence the two Remove calls on one node.
int CERFFile::RemoveResource(CExoString& name, unsigned short nType)
{
    CExoLinkedListNode* pNode = m_lstKeys.GetHeadPos();
    while (pNode != 0) {
        CERFKey* pKey = m_lstKeys.GetAtPos(pNode);
        if (pKey->m_nType == nType && CExoString(pKey->m_sName) == name) {
            CERFRes* pRes = m_lstRes.GetAtPos(pNode);
            delete pKey;
            delete pRes;
            m_lstKeys.Remove(pNode);
            m_lstRes.Remove(pNode);
            m_header.m_nKeyCount--;
            return 1;
        }
        m_lstRes.GetNext(pNode);
    }
    return 0;
}

int CERFFile::RemoveResource(CERFRes* pRes)
{
    K2_UNUSED(pRes);
    return 0;
}

int CERFFile::AddString(CERFString* pString)
{
    CERFString* pNew = new CERFString();
    pNew->m_nLanguageId = pString->m_nLanguageId;
    CExoString sText(pString->m_pString, pString->m_nLength);
    pNew->SetText(sText);
    m_lstStrings.m_pInternal->AddTail(pNew);
    return 1;
}

// Opens a second handle on the archive and reads each CERFRes payload in. Both bail-outs (the file
// did not open, the res list is empty) fall into the same tail, which returns 1 regardless.
int CERFFile::ReadData(const CExoString& path)
{
    m_sDataPath = path;

    CExoFile file(path, RESTYPE_SAV, CExoString("rb"));
    if (file.FileOpened() != 0) {
        CExoLinkedListNode* pNode = m_lstRes.m_pInternal->m_pHead;
        while (pNode != 0) {
            CERFRes* pRes = static_cast<CERFRes*>(m_lstRes.m_pInternal->GetAtPos(pNode));
            pRes->m_pData = new unsigned char[pRes->m_nSize];
            file.Seek(pRes->m_nOffset, 0);
            file.Read(pRes->m_pData, pRes->m_nSize, 1);
            m_lstRes.m_pInternal->GetNext(pNode);
        }
    }
    return 1;
}

int CERFFile::Read(const CExoString& path)
{
    CExoFile file(path, RESTYPE_SAV, CExoString("rb"));
    if (file.FileOpened() != 0) {
        file.Seek(0, 0);
        m_nFileSize = file.GetSize();
        file.Read(&m_header.m_nSignature, 8, 1);
        file.Read(&m_header.m_nStringCount, 4, 1);
        file.Read(&m_header.m_nStringTableSize, 4, 1);
        file.Read(&m_header.m_nKeyCount, 4, 1);
        file.Read(&m_header.m_nStringTableOffset, 4, 1);
        file.Read(&m_header.m_nKeyTableOffset, 4, 1);
        file.Read(&m_header.m_nResTableOffset, 4, 1);
        file.Read(&m_header.m_nBuildYear, 4, 1);
        file.Read(&m_header.m_nBuildDay, 4, 1);
        file.Read(&m_header.m_nDescriptionStrRef, 4, 1);
        file.Read(&m_header.m_abReserved, sizeof(m_header.m_abReserved), 1);
    }
    return 1;
}

// Flushes the string table: every CERFString writes its own {id, length, bytes} record back to
// back, then the table's own header is patched by seeking back to the start of the file. The size
// word it leaves there is the last entry's length plus 8, not the table total: the real total is
// computed by RecalculateOffsets, which runs after this pass. The two member mirrors are what let
// SetNumEntries and WriteResource find the tables again later.
int CERFFile::WriteStringTable()
{
    if (m_pFile == 0) {
        return 0;
    }
    unsigned int nCount = m_lstStrings.m_pInternal->m_nCount;
    unsigned int nLastSize = 0;
    unsigned int nStartOffset = m_pFile->GetOffset();
    unsigned int nEndOffset;
    CExoLinkedListNode* pNode = m_lstStrings.m_pInternal->m_pHead;

    while (pNode != 0) {
        CERFString* pString = static_cast<CERFString*>(m_lstStrings.m_pInternal->GetAtPos(pNode));
        nLastSize = pString->m_nLength + 8;
        pString->Write(*m_pFile);
        m_lstStrings.m_pInternal->GetNext(pNode);
    }

    nEndOffset = m_pFile->GetOffset();
    m_pFile->Seek(8, 0);
    m_pFile->Write(&nCount, 4, 1);
    m_pFile->Write(&nLastSize, 4, 1);
    unsigned int nPos = m_pFile->GetOffset();
    m_pFile->Seek(nPos + 4, 0);
    m_nStringTableOffset = nStartOffset;
    m_pFile->Write(&nStartOffset, 4, 1);
    m_nKeyTableOffset = nEndOffset;
    m_pFile->Write(&nEndOffset, 4, 1);
    m_pFile->SeekEnd();
    return 1;
}

// The pre-Create reader opens the archive, skips the header, then rebuilds the three lists straight
// off the counts in the header, which is why it has to run after Read(). The res loop is bounded by
// m_nKeyCount rather than by anything in the res table, since each key owns exactly one res record.
int CERFFile::ReadHeaderVariance(const CExoString& path)
{
    CExoFile file(path, RESTYPE_SAV, CExoString("rb"));
    if (file.FileOpened() != 0) {
        file.Seek(ERF_HEADER_SIZE, 0);

        for (unsigned int i = 0; i < m_header.m_nStringCount; i++) {
            CERFString* pString = new CERFString();
            pString->Read(file);
            m_lstStrings.m_pInternal->AddTail(pString);
        }
        for (unsigned int i = 0; i < m_header.m_nKeyCount; i++) {
            CERFKey* pKey = new CERFKey();
            pKey->Read(file);
            m_lstKeys.m_pInternal->AddTail(pKey);
        }
        for (unsigned int i = 0; i < m_header.m_nKeyCount; i++) {
            CERFRes* pRes = new CERFRes();
            pRes->Read(file);
            m_lstRes.m_pInternal->AddTail(pRes);
        }
    }
    return 1;
}

// The low-level sibling of the path overload. The key record is built from an explicit name and
// type instead of a parsed path, and the payload comes from one of two shapes. With a non-zero
// nBufferSize, pRes is really a CResGFF, which is packed into the archive directly; with a zero
// nBufferSize the CRes's raw bytes are copied through a buffer and written verbatim.
int CERFFile::WriteResource(char* sName, unsigned short nType, CRes* pRes, int nBufferSize, void* pExtra)
{
    K2_UNUSED(pExtra);
    CERFKey key;
    CERFRes res;

    if (m_nNextEntry >= m_nEntryCount) {
        return 0;
    }
    if (m_pFile == 0) {
        return 0;
    }

    CExoString sKeyName(sName);
    key.SetName(sKeyName);
    key.m_nID = m_nNextEntry;
    key.m_nType = nType;

    if (nBufferSize != 0) {
        CResGFF* pGff = static_cast<CResGFF*>(pRes);
        unsigned int nGffSize;
        int nGffOffset = m_pFile->GetOffset();
        pGff->Pack(0, 0);
        pGff->GetTotalSize();
        pGff->WriteGFFData(*m_pFile, nGffSize);
        m_pFile->Seek(m_nResTableOffset + m_nNextEntry * 8, 0);
        m_pFile->Write(&nGffOffset, 4, 1);
        m_pFile->Write(&nGffSize, 4, 1);
    } else {
        res.m_nSize = pRes->GetSize();
        unsigned char* pData = new unsigned char[pRes->GetSize()];
        memcpy(pData, pRes->GetData(), pRes->GetSize());  // idiom-exempt: a runtime byte count, not a sizeof
        res.m_pData = pData;
        res.Write(*m_pFile, m_nResTableOffset + m_nNextEntry * 8);
    }

    m_pFile->Seek(m_nKeyTableOffset + m_nNextEntry * ERF_KEY_ENTRY_SIZE, 0);
    key.Write(*m_pFile);
    m_pFile->SeekEnd();

    m_nNextEntry++;
    return 1;
}

int CERFFile::Create(CExoString& path)
{
    m_pFile = new CExoFile(path, RESTYPE_SAV, CExoString("wb"));
    return m_pFile->FileOpened() != 0;
}

int CERFFile::WriteHeader()
{
    if (m_pFile == 0) {
        return 0;
    }
    time_t tNow = time(0);
    struct tm* pTime = localtime(&tNow);
    if (pTime != 0) {
        m_header.m_nBuildYear = pTime->tm_year;
        m_header.m_nBuildDay = pTime->tm_yday;
    }
    m_pFile->Write(this, ERF_HEADER_SIZE, 1);
    return 1;
}

int CERFFile::WriteResource(const CExoString& filePath, void* pExtra)
{
    K2_UNUSED(pExtra);

    if (m_nNextEntry >= m_nEntryCount) {
        return 0;
    }
    if (m_pFile == 0) {
        return 0;
    }

    CERFKey key;
    CExoFile* pFile = new CExoFile(filePath, RESTYPE_INVALID, CExoString("rb"));

    int nColon = filePath.Find(CExoString(":"), 0);
    CExoString sName = filePath.Right(
        filePath.GetLength() - nColon - 1);
    int nDot = sName.Find(CExoString("."), 0);
    CExoString sExt = sName.Right(
        sName.GetLength() - nDot - 1);
    sName = sName.Left(nDot);

    unsigned short nResType = g_pExoBase->GetResTypeFromExtension(sExt);
    key.SetName(sName);
    key.m_nID = m_nNextEntry;
    key.m_nType = nResType;

    int nSize = pFile->GetSize();

#if KOTOR1  // K1 copies the whole resource through one heap buffer, not the large temp buffer
    char* pBuffer = new char[nSize + 1];
    int nHeaderOffset = m_pFile->GetOffset();
    pFile->Read(pBuffer, nSize, 1);
    m_pFile->Write(pBuffer, nSize, 1);
    delete[] pBuffer;
#else
    void* pBuffer = g_pAppManager->GetLargeTempBuffer();

    // The chunk cap is one byte under the one CopyToFile uses. The copy loop counts down a separate
    // remaining-bytes variable: nSize itself is what the resource-table patch below records.
    int nRemaining = nSize;
    int nHeaderOffset = m_pFile->GetOffset();
    while (nRemaining > 0) {
        int nChunk = nRemaining < ERF_COPY_CHUNK - 1 ? nRemaining : ERF_COPY_CHUNK - 1;
        pFile->Read(pBuffer, 1, nChunk);
        m_pFile->Write(pBuffer, nChunk, 1);
        nRemaining -= nChunk;
    }
#endif

    // Patch this entry's record in the resource table (offset, then size).
    m_pFile->Seek(m_nResTableOffset + m_nNextEntry * 8, 0);
    m_pFile->Write(&nHeaderOffset, 4, 1);
    m_pFile->Write(&nSize, 4, 1);
    m_pFile->SeekEnd();

    delete pFile;

    // ...then its key record, in the slot SetNumEntries blanked out.
    m_pFile->Seek(m_nKeyTableOffset + m_nNextEntry * ERF_KEY_ENTRY_SIZE, 0);
    key.Write(*m_pFile);
    m_pFile->SeekEnd();

    m_nNextEntry++;
    return 1;
}

int CERFFile::Finish()
{
    m_pFile->Flush();
    if (m_pFile != 0) {
        delete m_pFile;
    }
    m_pFile = 0;
    return 1;
}

// Writes a whole ERF through a stack CExoFile: header, string table, key table, res canaries and
// res payloads. This path never calls Create.
int CERFFile::Write(CExoString& filePath)
{
    time_t tNow;
    CExoFile file(filePath, RESTYPE_SAV, CExoString("wb"));
    if (file.FileOpened() == 0) {
        return 0;
    }

    m_header.m_nKeyCount = m_lstKeys.m_pInternal->m_nCount;
    m_header.m_nStringCount = m_lstStrings.m_pInternal->m_nCount;

    unsigned int nStringTableSize = 0;
    CExoLinkedListNode* pNode = m_lstStrings.m_pInternal->m_pHead;
    while (pNode != 0) {
        nStringTableSize += static_cast<CERFString*>(m_lstStrings.GetAtPos(pNode))->m_nLength + 8;
        m_lstStrings.GetNext(pNode);
    }
    m_header.m_nStringTableSize = nStringTableSize;
    m_header.m_nStringTableOffset = ERF_HEADER_SIZE;
    m_header.m_nKeyTableOffset = m_header.m_nStringTableOffset + nStringTableSize;
    m_header.m_nResTableOffset = m_header.m_nKeyTableOffset + m_lstKeys.m_pInternal->m_nCount * ERF_KEY_ENTRY_SIZE;

    tNow = time(0);
    struct tm* pTime = localtime(&tNow);
    if (pTime != 0) {
        m_header.m_nBuildYear = pTime->tm_year;
        m_header.m_nBuildDay = pTime->tm_yday;
    }
    file.Write(&m_header.m_nSignature, 8, 1);
    file.Write(&m_header.m_nStringCount, 4, 1);
    file.Write(&m_header.m_nStringTableSize, 4, 1);
    file.Write(&m_header.m_nKeyCount, 4, 1);
    file.Write(&m_header.m_nStringTableOffset, 4, 1);
    file.Write(&m_header.m_nKeyTableOffset, 4, 1);
    file.Write(&m_header.m_nResTableOffset, 4, 1);
    file.Write(&m_header.m_nBuildYear, 4, 1);
    file.Write(&m_header.m_nBuildDay, 4, 1);
    file.Write(&m_header.m_nDescriptionStrRef, 4, 1);
    file.Write(&m_header.m_abReserved, sizeof(m_header.m_abReserved), 1);

    pNode = m_lstStrings.m_pInternal->m_pHead;
    while (pNode != 0) {
        CERFString* pString = static_cast<CERFString*>(m_lstStrings.GetAtPos(pNode));
        pString->Write(file);
        m_lstStrings.GetNext(pNode);
    }

    pNode = m_lstKeys.m_pInternal->m_pHead;
    while (pNode != 0) {
        CERFKey* pKey = static_cast<CERFKey*>(m_lstKeys.GetAtPos(pNode));
        pKey->Write(file);
        m_lstKeys.GetNext(pNode);
    }

    unsigned int nPad = ERF_RES_ENTRY_PLACEHOLDER;
    for (unsigned int i = 0; i < m_lstRes.m_pInternal->m_nCount; i++) {
        file.Write(&nPad, 4, 1);
        file.Write(&nPad, 4, 1);
    }

    pNode = m_lstRes.m_pInternal->m_pHead;
    nPad = 0;
    while (pNode != 0) {
        CERFRes* pRes = static_cast<CERFRes*>(m_lstRes.GetAtPos(pNode));
        pRes->Write(file, m_header.m_nResTableOffset + nPad);
        m_lstRes.GetNext(pNode);
        nPad += 8;
    }
    return 1;
}

// Patches the archive's key count in place, then reserves space: nEntries blank CERFKeys where the
// key table goes (m_nKeyTableOffset, filled in by WriteStringTable) and a canary word per entry
// where the resource table will follow. The blank keys and canary words are what the per-resource
// WriteResource pass later overwrites. Returns 0 when there is no open file.
int CERFFile::SetNumEntries(unsigned int nEntries)
{
    CERFKey key;
    if (m_pFile == 0) {
        return 0;
    }
    m_nEntryCount = nEntries;
    m_nNextEntry = 0;

    m_pFile->Seek(__builtin_offsetof(CERFHeader, m_nKeyCount), 0);
    m_pFile->Write(&nEntries, 4, 1);
    m_pFile->Seek(m_nKeyTableOffset, 0);
    for (unsigned int i = 0; i < nEntries; i++) {
        m_pFile->Write(&key, ERF_KEY_ENTRY_SIZE, 1);
    }

    m_nResTableOffset = m_pFile->GetOffset();
    m_pFile->Seek(__builtin_offsetof(CERFHeader, m_nResTableOffset), 0);
    m_pFile->Write(&m_nResTableOffset, 4, 1);
    m_pFile->SeekEnd();

    unsigned int nPad = ERF_RES_ENTRY_CANARY;
    for (unsigned int i = 0; i < nEntries; i++) {
        m_pFile->Write(&nPad, 4, 1);
        m_pFile->Write(&nPad, 4, 1);
    }
    return 1;
}

void CERFFile::RecalculateOffsets()
{
    m_header.m_nKeyCount = m_lstKeys.m_pInternal->m_nCount;
    m_header.m_nStringCount = m_lstStrings.m_pInternal->m_nCount;

    unsigned int nStringTableSize = 0;
    CExoLinkedListNode* pNode = m_lstStrings.m_pInternal->m_pHead;
    while (pNode != 0) {
        nStringTableSize += static_cast<CERFString*>(m_lstStrings.m_pInternal->GetAtPos(pNode))->m_nLength + 8;
        m_lstStrings.m_pInternal->GetNext(pNode);
    }

    m_header.m_nStringTableSize = nStringTableSize;
    m_header.m_nStringTableOffset = ERF_HEADER_SIZE;
    m_header.m_nKeyTableOffset = m_header.m_nStringTableOffset + nStringTableSize;
    m_header.m_nResTableOffset = m_header.m_nKeyTableOffset + m_lstKeys.m_pInternal->m_nCount * ERF_KEY_ENTRY_SIZE;
}

// The bulk-import sibling of ExportFiles asks the base library for every file under an alias
// directory, sizes the ERF's entry table to that count, then writes each one. The declared
// fileTypeMask argument is unused.
//
// The archive name is built with Format("%s%s", ...) from the directory and the entry name.
void CERFFile::ImportFiles(const CExoString& dir, void* fileTypeMask,
                           int (*progressCallback)(int, int, int, int),
                           int progressMin, int progressMax)
{
    K2_UNUSED(fileTypeMask);  // dead in this build

    CExoArrayList<CExoString> list;
    CExoString sName;

#if KOTOR1  // K1's GetDirectoryList takes a third trailing int
    g_pExoBase->GetDirectoryList(&list, dir, RESTYPE_INVALID, 0, 0, 0);
#else
    g_pExoBase->GetDirectoryList(&list, dir, RESTYPE_INVALID, 0, 0);
#endif
    SetNumEntries(list.m_count);

    for (int i = 0; i < list.m_count; i++) {
        sName.Format(const_cast<char*>("%s%s"), dir.CStr(), list.m_pArray[i].CStr());
        WriteResource(sName, 0);
        if (progressCallback != 0) {
            progressCallback(i, list.m_count, progressMin, progressMax);
        }
    }
}

void CERFFile::ExportFiles(const CExoString& dir, void* fileTypeMask,
                           int (*progressCallback)(int, int, int, int),
                           int progressMin, int progressMax)
{
    K2_UNUSED(fileTypeMask);  // dead in this build

    CExoString sPath;
    CExoLinkedListNode* pKeyNode = m_lstKeys.GetHeadPos();
    CExoLinkedListNode* pResNode = m_lstRes.GetHeadPos();

    for (unsigned int i = 0; i < m_header.m_nKeyCount; i++) {
        // Declared first and assigned after so the pair shares one cleanup scope.
        CERFKey* pKey;
        CERFRes* pRes;
        CExoFile* pFile;
        pKey = m_lstKeys.GetAtPos(pKeyNode);
        pRes = m_lstRes.GetAtPos(pResNode);

        sPath = dir + CExoString(pKey->m_sName);
        pFile = new CExoFile(sPath, pKey->m_nType, CExoString("wb"));
        pFile->Write(pRes->m_pData, pRes->m_nSize, 1);
        pFile->Flush();
        if (progressCallback != 0) {
            progressCallback(i, m_header.m_nKeyCount, progressMin, progressMax);
        }
        delete pFile;

        m_lstKeys.GetNext(pKeyNode);
        m_lstRes.GetNext(pResNode);
    }
}

void CERFFile::ExportFilesFromERF(const CExoString& path, const CExoString& aliasPrefix,
                                  void* fileTypeMask,
                                  int (*progressCallback)(int, int, int, int),
                                  int progressMin, int progressMax)
{
    K2_UNUSED(fileTypeMask);  // dead in this build

    CExoFile file(path, RESTYPE_SAV, CExoString("rb"));
    if (file.FileOpened() != 0) {
        CExoString sOutPath;
        CExoLinkedListNode* pKeyNode = m_lstKeys.GetHeadPos();
        CExoLinkedListNode* pResNode = m_lstRes.GetHeadPos();

        unsigned int i;
        for (i = 0; i < m_header.m_nKeyCount; i++) {
            // Declared first and assigned after so the pair shares one cleanup scope.
            CERFKey* pKey;
            CERFRes* pRes;
            CExoFile* pOut;
            pKey = m_lstKeys.GetAtPos(pKeyNode);
            pRes = m_lstRes.GetAtPos(pResNode);

            sOutPath = aliasPrefix + CExoString(pKey->m_sName);
            pOut = new CExoFile(sOutPath, pKey->m_nType, CExoString("wb"));
            pRes->CopyToFile(&file, pOut, 0);
            pOut->Flush();
            if (progressCallback != 0) {
                progressCallback(i, m_header.m_nKeyCount, progressMin, progressMax);
            }
            delete pOut;

            m_lstKeys.GetNext(pKeyNode);
            m_lstRes.GetNext(pResNode);
        }

        if (progressCallback != 0) {
            progressCallback(m_header.m_nKeyCount, m_header.m_nKeyCount, progressMin, progressMax);
        }
    }
}

// Pulls the module .ifo out of an ERF and formats its description string into pName. Static, see
// the header: the header record's string reference doubles as the outward size result. The string
// table is read as a heap array of CERFString records ({id, length, bytes}), and the description is
// the record whose id matches nType, falling back to the record with id 0 and then to the first
// record. Every failure path stores its own sentinel into *pOutSize.
int CERFFile::ReadModuleDescription(const CExoString& path, unsigned short nType,
                                    uint64_t* pOutSize, CExoString* pOutName)
{
    CExoFile* pFile = new CExoFile(path, RESTYPE_MOD, CExoString("rb"));
    if (pFile->FileOpened() == 0) {
        *pOutSize = STRREF_MODULE_DESCRIPTION_NO_FILE;
        delete pFile;
        return 0;
    }

    CERFHeader header;
    pFile->Read(&header, sizeof(header), 1);

    if (header.m_nDescriptionStrRef + 1 >= 2) {
        *pOutSize = header.m_nDescriptionStrRef;
        delete pFile;
        return 1;
    }

    *pOutSize = STRREF_NONE;

    unsigned int nCount = header.m_nStringCount;
    if (nCount == 0) {
        *pOutSize = STRREF_MODULE_DESCRIPTION_NO_STRINGS;
        delete pFile;
        return 1;
    }

    CERFString* pStrings = new CERFString[nCount];
    unsigned int nID;
    unsigned int nLength;
    for (int i = 0; i < static_cast<int>(nCount); i++) {
        pFile->Read(&nID, 4, 1);
        pFile->Read(&nLength, 4, 1);
        char* pText = new char[nLength];
        pFile->Read(pText, 1, nLength);
        pStrings[i].m_pString = pText;
        pStrings[i].m_nLanguageId = nID;
        pStrings[i].m_nLength = nLength;
    }

    if (nCount == 1) {
        pOutName->Format(const_cast<char*>("%s"), pStrings[0].GetText().CStr());
        delete pFile;
        delete[] pStrings;
        return 1;
    }

    for (int i = 0; i < static_cast<int>(nCount); i++) {
        if (pStrings[i].m_nLanguageId == nType) {
            pOutName->Format(const_cast<char*>("%s"), pStrings[i].GetText().CStr());
            delete pFile;
            delete[] pStrings;
            return 1;
        }
    }
    for (int i = 0; i < static_cast<int>(nCount); i++) {
        if (pStrings[i].m_nLanguageId == 0) {
            pOutName->Format(const_cast<char*>("%s"), pStrings[i].GetText().CStr());
            delete pFile;
            delete[] pStrings;
            return 1;
        }
    }

    pOutName->Format(const_cast<char*>("%s"), pStrings[0].GetText().CStr());
    delete pFile;
    delete[] pStrings;
    return 1;
}

CERFKey::CERFKey()
{
    __builtin_memset(m_sName, 0, sizeof(m_sName));
    m_nID = 0;
    m_nType = RESTYPE_INVALID;
    m_nReserved1 = 0;
}

CERFKey::~CERFKey()
{
    Reset();
}

int CERFKey::Reset()
{
    m_nID = 0;
    m_nType = RESTYPE_INVALID;
    __builtin_memset(m_sName, 0, sizeof(m_sName));
    return 1;
}

void CERFKey::AddToSig(void* pSig)
{
    K2_UNUSED(pSig);
}

int CERFKey::Read(CExoFile& file)
{
    file.Read(m_sName, 16, 1);
    file.Read(&m_nID, 4, 1);
    file.Read(&m_nType, 2, 1);
    file.Read(&m_nReserved1, 2, 1);
    return 1;
}

int CERFKey::Write(CExoFile& file)
{
    file.Write(m_sName, 16, 1);
    file.Write(&m_nID, 4, 1);
    file.Write(&m_nType, 2, 1);
    file.Write(&m_nReserved1, 2, 1);
    return 1;
}

void CERFKey::SetName(CExoString& name)
{
    __builtin_memset(m_sName, 0, sizeof(m_sName));
#if KOTOR1  // K1 calls GetLength() out of line, so the copy keeps a runtime length
    char* pszName = name.CStr();
    int nLength = name.GetLength();
#ifdef K2_FIX_ENGINE_BUGS
    if (nLength > static_cast<int>(sizeof(m_sName)))
        nLength = sizeof(m_sName);
    memcpy(m_sName, pszName, nLength);  // idiom-exempt: a runtime byte count (clamped to the field)
#else
    if (__builtin_object_size(m_sName, 0) != static_cast<uint64_t>(-1))
        __memcpy_chk(m_sName, pszName, nLength, __builtin_object_size(m_sName, 0));
    else
        memcpy(m_sName, pszName, nLength);  // idiom-exempt: a runtime byte count, not a sizeof
#endif
#elif defined(K2_FIX_ENGINE_BUGS)
    char* pszName = name.CStr();
    int nLength = name.m_sString ? static_cast<int>(strlen(name.m_sString)) : 0;
    if (nLength > static_cast<int>(sizeof(m_sName)))
        nLength = sizeof(m_sName);
    memcpy(m_sName, pszName, nLength);  // idiom-exempt: a runtime byte count, not a sizeof
#else
    memcpy(m_sName, name.CStr(), name.m_sString ? static_cast<int>(strlen(name.m_sString)) : 0);  // idiom-exempt: a runtime byte count, not a sizeof
#endif
}

CERFRes::CERFRes()
{
    m_nOffset = 0;
    m_nSize = 0;
    m_pData = 0;
}

CERFRes::~CERFRes()
{
    Reset();
}

int CERFRes::Reset()
{
    if (m_pData != 0) {
#ifdef K2_FIX_ENGINE_BUGS
        delete[] m_pData;
#else
        delete m_pData;
#endif
        m_pData = 0;
    }
    m_nOffset = 0;
    m_nSize = 0;
    return 1;
}

void CERFRes::AddToSig(void* pSig)
{
    K2_UNUSED(pSig);
}

int CERFRes::Read(CExoFile& file)
{
    file.Read(&m_nOffset, 4, 1);
    file.Read(&m_nSize, 4, 1);
    return 1;
}

int CERFRes::ReadData(CExoFile& file)
{
    m_pData = new unsigned char[m_nSize];
    file.Seek(m_nOffset, 0);
    file.Read(m_pData, m_nSize, 1);
    return 1;
}

int CERFRes::Write(CExoFile& file, unsigned int nHeaderOffset)
{
    m_nOffset = file.GetOffset();
    file.Write(m_pData, m_nSize, 1);
    file.Seek(nHeaderOffset, 0);
    file.Write(&m_nOffset, 4, 1);
    file.Write(&m_nSize, 4, 1);
    file.SeekEnd();
    return 1;
}

int CERFRes::CopyToFile(CExoFile* pSource, CExoFile* pDest, void* pBuffer)
{
    pSource->Seek(m_nOffset, 0);

    // The incoming buffer is unused. The body re-points it at CAppManager's shared scratch buffer,
    // which is what both the read and the write below use.
    pBuffer = g_pAppManager->GetLargeTempBuffer();

    int nRemaining = m_nSize;
    while (nRemaining > 0) {
        unsigned int nChunk = (nRemaining < ERF_COPY_CHUNK) ? static_cast<unsigned int>(nRemaining) : ERF_COPY_CHUNK;
        pSource->Read(pBuffer, nChunk, 1);
        pDest->Write(pBuffer, nChunk, 1);
        nRemaining -= nChunk;
    }
    return 1;
}

CERFString::CERFString()
{
    m_nLanguageId = 0;
    m_nLength = 0;
    m_pString = 0;
}

CERFString::~CERFString()
{
    Reset();
}

int CERFString::Reset()
{
    if (m_pString != 0) {
        delete[] m_pString;
        m_pString = 0;
    }
    m_nLanguageId = 0;
    m_nLength = 0;
    return 1;
}

void CERFString::AddToSig(void* pSig)
{
    K2_UNUSED(pSig);
}

CExoString CERFString::GetText()
{
    return CExoString(m_pString, m_nLength);
}

void CERFString::SetText(CExoString& text)
{
    if (m_pString != 0) {
        delete[] m_pString;
    }
    m_nLength = text.GetLength();
    m_pString = new char[m_nLength + 1];
    memcpy(m_pString, text.CStr(), m_nLength);  // idiom-exempt: a runtime byte count, not a sizeof
    m_pString[m_nLength] = 0;
}

int CERFString::Read(CExoFile& file)
{
    CExoString sText;
    file.Read(&m_nLanguageId, 4, 1);
    file.Read(&m_nLength, 4, 1);
    file.Read(sText, m_nLength);
    SetText(sText);
    return 1;
}

int CERFString::Write(CExoFile& file)
{
    file.Write(&m_nLanguageId, 4, 1);
    file.Write(&m_nLength, 4, 1);
    file.Write(m_pString, m_nLength, 1);
    return 1;
}
