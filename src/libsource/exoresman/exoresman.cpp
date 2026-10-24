#include "k2stdint.h"
#include "port/sdlapi.h"
#include "exoresmaninternal.h"
#include <stddef.h>
#include <stdio.h>
#include "exoresman.h"
#include "exostring.h"

#if KOTOR1  // K1's RemoveFile builds its path with std::string, so it needs the real libc headers
#include <string>
#include <algorithm>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#endif
#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcstdlib.h"
#include "port/crtapi.h"
#include "restype.h"
#include "exoresfile.h"
#include "port/win32const.h"

// The smallest resource-memory budget; a device with more than twice that gets half its memory.
enum { MIN_RESOURCE_MEMORY = 16 * 1024 * 1024 };

// st_mode's file-type field and its directory value (<sys/stat.h>'s S_IFMT and S_IFDIR, which the
// matching build's port/libcstat.h does not declare).
enum {
    MODE_TYPE_MASK = 0xf000,
    MODE_DIRECTORY = 0x4000,
};

// What K1's GetFreeDiskSpace always reports.
enum : unsigned int {
    K1_REPORTED_FREE_DISK_SPACE = 0xffffffff,
};

// The resource types GetIsStaticType reports static (the scripts, dialogs, journals, area and
// blueprint types), as bits over the type's distance from RESTYPE_NSS.
enum : unsigned long long {
    STATIC_RESTYPE_BITS = (1ULL << (RESTYPE_NSS - RESTYPE_NSS)) |
                          (1ULL << (RESTYPE_NCS - RESTYPE_NSS)) |
                          (1ULL << (RESTYPE_ARE - RESTYPE_NSS)) |
                          (1ULL << (RESTYPE_UTI - RESTYPE_NSS)) |
                          (1ULL << (RESTYPE_UTC - RESTYPE_NSS)) |
                          (1ULL << (RESTYPE_DLG - RESTYPE_NSS)) |
                          (1ULL << (RESTYPE_ITP - RESTYPE_NSS)) |
                          (1ULL << (RESTYPE_UTT - RESTYPE_NSS)) |
                          (1ULL << (RESTYPE_UTS - RESTYPE_NSS)) |
                          (1ULL << (RESTYPE_UTE - RESTYPE_NSS)) |
                          (1ULL << (RESTYPE_UTD - RESTYPE_NSS)) |
                          (1ULL << (RESTYPE_UTP - RESTYPE_NSS)) |
                          (1ULL << (RESTYPE_GIC - RESTYPE_NSS)) |
                          (1ULL << (RESTYPE_UTM - RESTYPE_NSS)) |
                          (1ULL << (RESTYPE_JRL - RESTYPE_NSS)) |
                          (1ULL << (RESTYPE_UTW - RESTYPE_NSS)),
};

// m_nModuleResourceFlags: what AsyncLoad mounted for the module.
enum {
    MODULERES_SAVED_RIM = 0x1,        // the current game's copy of the module's RIM
    MODULERES_MODULE_ARCHIVE = 0x2,   // the module's .mod
    MODULERES_SAVED_ARCHIVE = 0x4,    // the current game's copy of the module archive
    MODULERES_S_RIM = 0x8,            // the module's _s RIM
    MODULERES_A_RIM = 0x10,           // the module's _a RIM
    MODULERES_ADX_RIM = 0x20,         // the module's _adx RIM
};

extern CExoResMan* g_pExoResMan;

// The resID overload stops after m_nSize and leaves the holder and slack fields uninitialised; the
// default constructor sets every field.
CRes::CRes()
{
    m_nDemandCount = 0;
    m_nRequestCount = 0;
    m_dwResID = RESID_INVALID;
    m_dwFlags = 0;
    m_pFileData = NULL;
    m_pKeyEntry = NULL;
    m_nSize = 0;
    m_nHolderCount = 0;
    m_bAllocHeaderData = 0;
    m_bAllocTrailingData = 0;
}

CRes::CRes(unsigned int dwResID)
{
    m_nDemandCount = 0;
    m_nRequestCount = 0;
    m_dwResID = dwResID;
    m_dwFlags = 0;
    m_pFileData = NULL;
    m_pKeyEntry = NULL;
    m_nSize = 0;
}

CRes::~CRes()
{
    Dump(0);
}

int CRes::GetFixedResourceSize() { return -1; }
int CRes::GetFixedResourceDataOffset() { return 0; }
int CRes::OnResourceFreed() { return 1; }
int CRes::OnResourceServiced() { return 1; }

void CRes::Dump(int)
{
    const void* pSelf = this;
    if (pSelf == 0) {
        return;
    }
    if (m_nDemandCount != 0) {
        m_dwFlags |= CRes::RESFLAG_FREE_PENDING;
        return;
    }
    if ((m_dwFlags & CRes::RESFLAG_HAS_DATA) != 0 && m_pFileData != 0) {
        g_pExoResMan->Free(this);
    }
}

int CRes::Request()
{
    return g_pExoResMan->Request(this);
}

int CRes::CancelRequest()
{
    return g_pExoResMan->CancelRequest(this);
}

void* CRes::Demand()
{
    return g_pExoResMan->Demand(this);
}

int CRes::Release()
{
    return g_pExoResMan->Release(this);
}

int CRes::ReadRaw(int nSize, char* pBuffer)
{
    return g_pExoResMan->ReadRaw(this, nSize, pBuffer);
}

void CRes::GetResRef(CResRef& resRef, unsigned short& resType)
{
    resRef = m_pKeyEntry->m_resRef;
    resType = m_pKeyEntry->m_resType;
}

void CRes::SetID(unsigned int dwResID)
{
    Dump(0);
    m_dwResID = dwResID;
}

// Services the queue under the critical section, then parks until ResumeThread(). The shutdown flag
// is re-tested at the top of the loop, which is what lets the destructor's
// `m_bShuttingDown = 1; ResumeThread(...)` pair end the thread.
#include "port/win32api.h"

uint64_t AsyncRimLoadThreadProc(void* pParam)
{
    K2_UNUSED(pParam);
    while (g_pExoResMan != 0 && !g_pExoResMan->m_bShuttingDown) {
        g_pExoResMan->m_critSection.EnterCriticalSection();
        g_pExoResMan->AsyncLoad();
        g_pExoResMan->m_critSection.LeaveCriticalSection();
        SuspendThread(reinterpret_cast<void*>(g_pExoResMan->m_hAsyncThread));
    }
#if KOTOR1  // K1 exits the thread explicitly
    ExitThread(1);
#endif
    return 1;
}

CExoResMan::CExoResMan()
{
    CResGFFFieldIDHash::Initialize();

    m_pDemandQueueHeadCache = 0;
    m_bSuspended = 0;
    bChargenImageFileLoaded = 0;
    bMainMenuImageFileLoaded = 0;
    m_nReserved1 = 0;

#if KOTOR1  // K1 has no GlobalMemoryStatus; the budget comes from the member nothing ever writes
    int nTotalPhys = m_nTotalPhysicalMemory;
#else
    MEMORYSTATUS ms;
    GlobalMemoryStatus(&ms);
    int nTotalPhys = static_cast<int>(ms.dwTotalPhys);
#endif
    int nBudget = (nTotalPhys > 2 * MIN_RESOURCE_MEMORY + 1) ? (nTotalPhys >> 1) : MIN_RESOURCE_MEMORY;

    m_nAsyncLoadState = 0;
    m_nModuleResourceFlags = 0;
    m_bShuttingDown = 0;

#if !KOTOR1
    m_nTotalPhysicalMemory = nTotalPhys;
#endif
    m_nTotalResourceMemory = nBudget;
    m_nAvailableResourceMemory = nBudget;

    unsigned int dwThreadStartFlag = 0;
    m_hAsyncThread = reinterpret_cast<unsigned long long>(CreateThread(0, 0, AsyncRimLoadThreadProc,
                                                        &dwThreadStartFlag, 4,
                                                        &m_dwAsyncThreadId));
}

CExoResMan::~CExoResMan()
{
    m_bShuttingDown = 1;
    ResumeThread(reinterpret_cast<void*>(m_hAsyncThread));
    if (static_cast<long long>(m_hAsyncThread) != -1) {
        WaitForSingleObject(reinterpret_cast<void*>(m_hAsyncThread), INFINITE);
        CloseHandle(reinterpret_cast<void*>(m_hAsyncThread));
    }

    // The rest of the teardown is the implicit member destruction, in reverse declaration order.
}

int CExoResMan::GetTotalPhysicalMemory()
{
    return m_nTotalPhysicalMemory;
}

void CExoResMan::ResumeServicing()
{
    m_bSuspended = 0;
}

void CExoResMan::SuspendServicing()
{
    m_bSuspended = 1;
}

int CExoResMan::SetTotalResourceMemory(int nBytes)
{
    if (__builtin_expect(m_nTotalPhysicalMemory < nBytes, 1)) {
        return 0;
    }
    int nAvail;
    if (m_nTotalResourceMemory > nBytes) {
        int nDiff = m_nTotalResourceMemory - nBytes;
        if (m_nAvailableResourceMemory < nDiff) {
            return 0;
        }
        nAvail = m_nAvailableResourceMemory - nDiff;
    } else {
        nAvail = (nBytes - m_nTotalResourceMemory) + m_nAvailableResourceMemory;
    }
    m_nTotalResourceMemory = nBytes;
    m_nAvailableResourceMemory = nAvail;
    return 1;
}

int CExoResMan::GetIsStaticType(unsigned short resType)
{
    unsigned int n = static_cast<unsigned int>(resType) - RESTYPE_NSS;
    bool bStatic = true;
    if (n > RESTYPE_UTW - RESTYPE_NSS || ((1ULL << (n & 63)) & STATIC_RESTYPE_BITS) == 0) {
        goto checkSpecial;
    }
    return bStatic;
checkSpecial:
    if (resType == RESTYPE_PTH) {
        return bStatic;
    }
    return false;
}

void CExoResMan::FreeResourceData(CRes* pRes)
{
    if (pRes == 0 || pRes->m_pFileData == 0) {
        return;
    }
    if ((pRes->m_dwResID >> RESID_SOURCE_SHIFT) != RESSOURCE_IMAGE) {
        void* p = pRes->m_pFileData;
        if (pRes->m_bAllocHeaderData) {
            p = static_cast<char*>(p) - sizeof(CResHelperInfo);
        }
        operator delete[](p);
        unsigned int n = pRes->m_nSize;
        m_nAvailableResourceMemory = m_nAvailableResourceMemory + n;
    }
    pRes->m_nSize = 0;
    pRes->m_pFileData = 0;
}

bool CExoResMan::Free(CRes* pRes)
{
    if (pRes == 0 || pRes->m_pFileData == 0)
        return false;

    CExoLinkedListNode* pNode = m_freeResources.m_pInternal->m_pHead;
    if (m_freeResources.m_pInternal->m_pHead == 0)
        return false;

    void* pData = m_freeResources.m_pInternal->m_pHead->m_pData;
    if (pData == 0)
        return false;

    while (pData != pRes) {
        pData = m_freeResources.GetNext(pNode);
        if (pData == 0)
            return false;
    }

    pRes->OnResourceFreed();
    if (pRes->m_dwFlags & CRes::RESFLAG_LOADING)
        return false;

    m_freeResources.Remove(pNode);
    FreeResourceData(pRes);
    pRes->m_dwFlags &= ~(CRes::RESFLAG_HAS_DATA | CRes::RESFLAG_FREE_PENDING);
    return true;
}

int CExoResMan::FreeChunk()
{
    unsigned int totalFreed;
    CRes* pRes;

    CExoLinkedListNode* pNode = m_freeResources.GetHeadPos();
    pRes = m_freeResources.GetHead();
    if (pRes == 0) {
        totalFreed = 0;
        goto ret;
    }

    totalFreed = 0;

check:
    if (pRes->m_dwFlags & CRes::RESFLAG_LOADING) {
        pRes = static_cast<CRes*>(m_freeResources.GetNext(pNode));
        goto afterAdvance;
    }

    if (pRes->m_nDemandCount != 0) {
        pRes->m_dwFlags &= ~CRes::RESFLAG_FREE_PENDING;
        CExoLinkedListNode* pOldNode = pNode;
        pRes = static_cast<CRes*>(m_freeResources.GetNext(pNode));
        m_freeResources.Remove(pOldNode);
    } else {
        pRes->OnResourceFreed();
        totalFreed -= m_nAvailableResourceMemory;
        FreeResourceData(pRes);
        totalFreed += m_nAvailableResourceMemory;
        pRes->m_dwFlags &= ~(CRes::RESFLAG_HAS_DATA | CRes::RESFLAG_FREE_PENDING);
        CExoLinkedListNode* pOldNode = pNode;
        pRes = static_cast<CRes*>(m_freeResources.GetNext(pNode));
        m_freeResources.Remove(pOldNode);
    }

afterAdvance:
    if (pRes == 0)
        goto ret;
    if ((totalFreed >> 20) != 0)
        goto ret;
    if (m_freeResources.m_pInternal->m_pHead == 0)
        goto ret;
    goto check;

ret:
    return totalFreed != 0;
}

bool CExoResMan::Malloc(CRes* pRes)
{
    while (static_cast<int>(pRes->m_nSize) > m_nAvailableResourceMemory) {
        if (!FreeChunk()) {
            break;
        }
    }
    unsigned int nSize = pRes->m_nSize;
    m_nAvailableResourceMemory = m_nAvailableResourceMemory - nSize;

    // The count is added in 64 bits so the slack cannot wrap a 32-bit size.
    if (pRes->m_bAllocHeaderData) {
        pRes->m_pFileData = new char[static_cast<uint64_t>(nSize) + sizeof(CResHelperInfo)];
        pRes->m_pFileData = static_cast<char*>(pRes->m_pFileData) + sizeof(CResHelperInfo);
    } else if (pRes->m_bAllocTrailingData) {
        // The trailing block goes after a four-byte gap.
        pRes->m_pFileData = new char[static_cast<uint64_t>(nSize) + 4 + sizeof(CResHelperInfo)];
    } else {
        pRes->m_pFileData = new char[nSize];
    }
    return true;
}

int CExoResMan::GetKeyEntry(const CResRef& resRef, unsigned short resType,
                              CExoKeyTable** ppTable, CKeyTableEntry** ppEntry)
{
#if defined(K2_UI_PC) || defined(K1_UI_XBOX)
    // A GUI layout comes from the archives before the override directories: the port's override/
    // copies re-lay the PC screens out for touch, so they must not win over the packed originals.
    if (resType == RESTYPE_GUI &&
        (GetKeyEntryFromTable(m_encapsulatedFiles, resRef, resType, ppTable, ppEntry, 1) ||
         GetKeyEntryFromTable(m_resourceImageFiles, resRef, resType, ppTable, ppEntry, 0) ||
         GetKeyEntryFromTable(m_encapsulatedFiles, resRef, resType, ppTable, ppEntry, 2) ||
         GetKeyEntryFromTable(m_fixedKeyTables, resRef, resType, ppTable, ppEntry, 0)))
        return true;
#endif

    if (GetKeyEntryFromTable(m_resourceDirectories, resRef, resType, ppTable, ppEntry, 0) ||
        GetKeyEntryFromTable(m_encapsulatedFiles, resRef, resType, ppTable, ppEntry, 1) ||
        GetKeyEntryFromTable(m_resourceImageFiles, resRef, resType, ppTable, ppEntry, 0) ||
        GetKeyEntryFromTable(m_encapsulatedFiles, resRef, resType, ppTable, ppEntry, 2) ||
        GetKeyEntryFromTable(m_fixedKeyTables, resRef, resType, ppTable, ppEntry, 0))
        return true;
    *ppTable = 0;
    *ppEntry = 0;
    return false;
}

bool CExoResMan::Request(CRes* pRes)
{
    if (pRes == 0 || (pRes->m_dwFlags & CRes::RESFLAG_RESIDENT) != 0) {
        return false;
    }
    if (pRes->m_nRequestCount == 0) {
        pRes->m_nRequestCount = 0;
        m_demandQueue.m_pInternal->AddTail(pRes);
    }
    pRes->m_nRequestCount = pRes->m_nRequestCount + 1;
    return true;
}

int CExoResMan::ReleaseResObject(CRes* pRes)
{
    if (pRes == 0) {
        return 0;
    }
    if (pRes->m_nHolderCount >= 2) {
        pRes->m_nHolderCount = pRes->m_nHolderCount - 1;
        return pRes->m_nHolderCount;
    }
    pRes->m_nHolderCount = 0;
    if (pRes->m_nDemandCount != 0) {
        return 0;
    }
    if (pRes->m_pKeyEntry != 0) {
        pRes->m_pKeyEntry->m_pRes = 0;
    }
    unsigned int dwFlags = pRes->m_dwFlags;
    if ((dwFlags & CRes::RESFLAG_HAS_DATA) != 0 && pRes->m_pFileData != 0) {
        g_pExoResMan->Free(pRes);
        dwFlags = pRes->m_dwFlags;
    }
    if ((dwFlags >> 3 & 1) != 0) {
        pRes->m_dwFlags = dwFlags | CRes::RESFLAG_ORPHANED;
    }
    return 0;
}

int CExoResMan::SpaceUsedBySaveGames()
{
    return 0;
}

int CExoResMan::FindModuleDownloadedPkg(const CExoString& name)
{
    K2_UNUSED(name);
    return 0;
}

bool CExoResMan::ValidateSignedResource(const CExoString& path, unsigned short resType, unsigned int a, unsigned int b)
{
    K2_UNUSED(path); K2_UNUSED(resType); K2_UNUSED(a); K2_UNUSED(b);
    return true;
}

bool CExoResMan::CreateSignatureFile(const CExoString& path, unsigned short resType, unsigned int a, unsigned int b)
{
    K2_UNUSED(path); K2_UNUSED(resType); K2_UNUSED(a); K2_UNUSED(b);
    return true;
}

int CExoResMan::RemoveKeyTable(const CExoString& path, unsigned int sourceType)
{
    CExoLinkedList<CExoKeyTable>* pList;
    switch (sourceType) {
    case KEYTABLE_SOURCE_FIXED: pList = &m_fixedKeyTables; break;
    case KEYTABLE_SOURCE_DIRECTORY: pList = &m_resourceDirectories; break;
    case KEYTABLE_SOURCE_ENCAPSULATED: pList = &m_encapsulatedFiles; break;
    case KEYTABLE_SOURCE_IMAGE: pList = &m_resourceImageFiles; break;
    default: return false;
    }

    CExoLinkedListNode* pNode = pList->GetHeadPos();
    if (pNode == 0) {
        return false;
    }
    CExoKeyTable* pTable = pList->GetAtPos(pNode);
    while (pTable == 0 || !(pTable->m_sFileName == path)) {
        pTable = pList->GetNext(pNode);
        if (pNode == 0) return false;
    }

    pTable->DestroyTable();
    pList->Remove(pNode);
    pTable->~CExoKeyTable();
    operator delete(pTable);
    return true;
}

// Mirrors RemoveKeyTable's by-name scan, but a miss builds a new CExoKeyTable instead of destroying
// one. m_nRoutingTag packs a per-category flag in its top bits with a probe slot id in bits 0-5,
// unique among the tables of the same list. This function scans the list once to build a 64-bit
// used-slot bitmap from every other table's tag, then picks the lowest free slot in [1,63], wrapping
// to 0 when all 64 are taken.
int CExoResMan::AddKeyTable(const CExoString& path, unsigned int sourceType, unsigned int arg)
{
    CExoLinkedList<CExoKeyTable>* pList;
    unsigned int nCategoryTag;
    switch (sourceType) {
    case KEYTABLE_SOURCE_FIXED: pList = &m_fixedKeyTables;      nCategoryTag = 0;          break;
    case KEYTABLE_SOURCE_DIRECTORY: pList = &m_resourceDirectories; nCategoryTag = ROUTINGTAG_DIRECTORY; break;
    case KEYTABLE_SOURCE_ENCAPSULATED: pList = &m_encapsulatedFiles;   nCategoryTag = ROUTINGTAG_ENCAPSULATED; break;
    case KEYTABLE_SOURCE_IMAGE: pList = &m_resourceImageFiles;  nCategoryTag = ROUTINGTAG_IMAGE; break;
    default: return false;
    }

    unsigned int aUsedProbeSlots[2] = {0, 0};
    CExoLinkedListNode* pNode = pList->GetHeadPos();
    CExoKeyTable* pTable;
    if (pNode == 0) goto notFound;

    pTable = pList->GetAtPos(pNode);
    while (1) {
        if (pTable != 0) {
            if (pTable->m_sFileName == path) goto found;
            {
                unsigned int nWord = (pTable->m_nRoutingTag >> 5) & (ROUTINGTAG_SLOT_MASK >> 5);
                K2_ASSUME(nWord < 2);   // every tag is a slot below 64 | a category in bits 29-31 (see notFound)
                aUsedProbeSlots[nWord] = (1u << (pTable->m_nRoutingTag & 31)) | aUsedProbeSlots[nWord];
            }
        }
        pTable = pList->GetNext(pNode);
        if (pNode == 0) goto notFound;
    }

found:
    {
        CExoString sMsg;
        if (sourceType == 4) {
            sMsg.Format(const_cast<char*>("Table being rebuilt, this RIM is being leaked: %s"), path.CStr());
            // sic: formatted but never logged or used
        }
        pTable->RebuildTable();
    }
    return true;

notFound:
    {
        unsigned int nProbeSlot = 1;
        while ((aUsedProbeSlots[nProbeSlot >> 5] & static_cast<uint32_t>(static_cast<uint64_t>(1) << (nProbeSlot & 31))) != 0) {
            if (++nProbeSlot == 64) {
                nProbeSlot = 0;
                break;
            }
        }

        unsigned int nRoutingTag = nProbeSlot | nCategoryTag;
        CExoKeyTable* pNew = new CExoKeyTable();
        CExoLinkedListNode* pAdded = pList->AddHead(pNew);
        int bOk = pNew->BuildNewTable(sourceType, path, nRoutingTag, 1);
        pNew->m_nInternalSubtype = arg;
        if (bOk != 0) {
            return bOk;
        }
        pList->Remove(pAdded);
        pNew->~CExoKeyTable();
        operator delete(pNew);
        return false;
    }
}

bool CExoResMan::UpdateKeyTable(const CExoString& path, unsigned int sourceType)
{
    CExoLinkedList<CExoKeyTable>* pList;
    switch (sourceType) {
    case KEYTABLE_SOURCE_FIXED: pList = &m_fixedKeyTables; break;
    case KEYTABLE_SOURCE_DIRECTORY: pList = &m_resourceDirectories; break;
    case KEYTABLE_SOURCE_ENCAPSULATED: pList = &m_encapsulatedFiles; break;
    case KEYTABLE_SOURCE_IMAGE: pList = &m_resourceImageFiles; break;
    default: return false;
    }

    CExoLinkedListNode* pNode = pList->GetHeadPos();
    if (pNode == 0) {
        return false;
    }
    CExoKeyTable* pTable = pList->GetAtPos(pNode);
    while (pTable == 0 || !path.CompareNoCase(pTable->m_sFileName)) {
        pTable = pList->GetNext(pNode);
        if (pNode == 0) return false;
    }

    pTable->RebuildTable();
    return true;
}

bool CExoResMan::UpdateFixedKeyTableFile(const CExoString& path)
{
    CExoLinkedListNode* pNode = m_fixedKeyTables.m_pInternal->m_pHead;
    if (pNode == 0) {
        return false;
    }
    CExoKeyTable* pTable = static_cast<CExoKeyTable*>(m_fixedKeyTables.m_pInternal->GetAtPos(pNode));
    while (pTable == 0 || !path.CompareNoCase(pTable->m_sFileName)) {
        pTable = static_cast<CExoKeyTable*>(m_fixedKeyTables.m_pInternal->GetNext(pNode));
        if (pNode == 0) return false;
    }

    pTable->RebuildTable();
    return true;
}

bool CExoResMan::UpdateResourceDirectory(const CExoString& path)
{
    CExoLinkedListNode* pNode = m_resourceDirectories.m_pInternal->m_pHead;
    if (pNode == 0) {
        return false;
    }
    CExoKeyTable* pTable = static_cast<CExoKeyTable*>(m_resourceDirectories.m_pInternal->GetAtPos(pNode));
    while (pTable == 0 || !path.CompareNoCase(pTable->m_sFileName)) {
        pTable = static_cast<CExoKeyTable*>(m_resourceDirectories.m_pInternal->GetNext(pNode));
        if (pNode == 0) return false;
    }

    pTable->RebuildTable();
    return true;
}

bool CExoResMan::UpdateEncapsulatedResourceFile(const CExoString& path)
{
    CExoLinkedListNode* pNode = m_encapsulatedFiles.m_pInternal->m_pHead;
    if (pNode == 0) {
        return false;
    }
    CExoKeyTable* pTable = static_cast<CExoKeyTable*>(m_encapsulatedFiles.m_pInternal->GetAtPos(pNode));
    while (pTable == 0 || !path.CompareNoCase(pTable->m_sFileName)) {
        pTable = static_cast<CExoKeyTable*>(m_encapsulatedFiles.m_pInternal->GetNext(pNode));
        if (pNode == 0) return false;
    }

    pTable->RebuildTable();
    return true;
}

bool CExoResMan::UpdateResourceImageFile(const CExoString& path)
{
    CExoLinkedListNode* pNode = m_resourceImageFiles.m_pInternal->m_pHead;
    if (pNode == 0) {
        return false;
    }
    CExoKeyTable* pTable = static_cast<CExoKeyTable*>(m_resourceImageFiles.m_pInternal->GetAtPos(pNode));
    while (pTable == 0 || !path.CompareNoCase(pTable->m_sFileName)) {
        pTable = static_cast<CExoKeyTable*>(m_resourceImageFiles.m_pInternal->GetNext(pNode));
        if (pNode == 0) return false;
    }

    pTable->RebuildTable();
    return true;
}

void CExoResMan::RemoveFixedKeyTableFile(const CExoString& path)
{
    RemoveKeyTable(path, KEYTABLE_SOURCE_FIXED);
}

void CExoResMan::RemoveResourceDirectory(const CExoString& path)
{
    RemoveKeyTable(path, KEYTABLE_SOURCE_DIRECTORY);
}

int CExoResMan::RemoveEncapsulatedResourceFile(const CExoString& path)
{
    return RemoveKeyTable(path, KEYTABLE_SOURCE_ENCAPSULATED);
}

void CExoResMan::RemoveResourceImageFile(const CExoString& path)
{
    RemoveKeyTable(path, KEYTABLE_SOURCE_IMAGE);
}

void CExoResMan::AddFixedKeyTableFile(const CExoString& path)
{
    AddKeyTable(path, KEYTABLE_SOURCE_FIXED, 0);
}

void CExoResMan::AddResourceDirectory(const CExoString& path)
{
    AddKeyTable(path, KEYTABLE_SOURCE_DIRECTORY, 0);
}

int CExoResMan::AddEncapsulatedResourceFile(const CExoString& path, unsigned int arg)
{
    return AddKeyTable(path, KEYTABLE_SOURCE_ENCAPSULATED, arg);
}

void CExoResMan::AddResourceImageFile(const CExoString& path)
{
    AddKeyTable(path, KEYTABLE_SOURCE_IMAGE, 0);
}

void CExoResMan::AddResourceImageFilesAsync(const CExoString& path, int a, int b)
{
    if (m_nAsyncLoadState == 1) {
        // sic: spins forever, nothing ever reloads the flag
        for (;;) {}
    }
    m_critSection.EnterCriticalSection();
    m_sAsyncModuleName = path;
    m_bAsyncLoadModule = a;
    m_bAsyncIsSaveGame = b;
    m_nAsyncLoadState = 1;
    m_nModuleResourceFlags = 0;
    ResumeThread(reinterpret_cast<void*>(m_hAsyncThread));
    m_critSection.LeaveCriticalSection();
}

unsigned short CExoResMan::GetResTypeFromFile(const CExoString& path)
{
    CExoString ext;
    unsigned short resType;
    int dotPos = path.Find('.', 0);
    if (dotPos < 0) {
        resType = RESTYPE_INVALID;
    } else {
        {
            CExoString sub = path.SubString(dotPos + 1, 3);
            ext = sub;
        }
        resType = g_pExoBase->GetResTypeFromExtension(ext);
    }
    return resType;
}

void CExoResMan::GetResRefFromFile(CResRef& resRef, const CExoString& path)
{
    int dotPos = path.Find('.', 0);
    if (dotPos <= 0) {
        resRef = const_cast<char*>("");
    } else {
        int len = (dotPos < 16) ? dotPos : 16;
        CExoString sub = path.SubString(0, len);
        resRef = sub;
    }
}

int CExoResMan::CreateDirectory2(const CExoString& path)
{
    CExoString resolved;
    resolved = g_pExoBase->m_pAliasList->ResolveFileName(path, RESTYPE_INVALID);
#if KOTOR1  // K1: CExoString::GetLength is out of line
    if (resolved.GetLength() == 0)
#else
    if (resolved.m_sString == NULL || static_cast<int>(strlen(resolved.m_sString)) == 0)
#endif
        return false;
#if KOTOR1
    return CreateDirectory(resolved.CStr(), NULL);
#else
    return CreateDirectoryA(resolved.CStr(), NULL);
#endif
}

int CExoResMan::GetFreeDiskSpace(const CExoString& path, unsigned long long* pFreeBytes)
{
    CExoString resolved;
    resolved = g_pExoBase->m_pAliasList->ResolveFileName(path, RESTYPE_INVALID);
#if KOTOR1  // K1: CExoString::GetLength is out of line
    if (resolved.GetLength() == 0)
#else
    if (resolved.m_sString == NULL || static_cast<int>(strlen(resolved.m_sString)) == 0)
#endif
        return false;

#if KOTOR1  // K1 does not ask the file system: it always reports 4 GB free
    *pFreeBytes = K1_REPORTED_FREE_DISK_SPACE;
    return true;
#else
    unsigned long long freeBytesAvailable, totalBytes, totalFreeBytes;
    GetDiskFreeSpaceExA(resolved.CStr(), &freeBytesAvailable, &totalBytes, &totalFreeBytes);
    *pFreeBytes = freeBytesAvailable;
    return true;
#endif
}

bool CExoResMan::FileCopy(const CExoString& srcPath, const CExoString& dstPath,
                           unsigned short srcResType, unsigned short dstResType)
{
    if (srcPath == dstPath)
        return false;

    if (dstResType == RESTYPE_INVALID)
        dstResType = srcResType;

    CExoFile srcFile(srcPath, srcResType, CExoString("rb"));
    CExoFile dstFile(dstPath, dstResType, CExoString("wb"));

    if (!srcFile.FileOpened() || !dstFile.FileOpened())
        return false;

    // Kept at the top so both locals share one cleanup scope.
    long long nSize;
    char* pBuffer;
    nSize = srcFile.GetSize();
    pBuffer = new char[nSize >= 0 ? nSize : -1];
    srcFile.Read(pBuffer, srcFile.GetSize(), 1);
    dstFile.Write(pBuffer, srcFile.GetSize(), 1);
    dstFile.Flush();
    operator delete[](pBuffer);
    return true;
}

int CExoResMan::GetTableCount(CRes* pRes, int resType)
{
    CExoLinkedList<CExoKeyTable>* pList = 0;
    switch (pRes->m_dwResID >> RESID_SOURCE_SHIFT) {
    case RESSOURCE_BIF: pList = &m_fixedKeyTables; break;
    case RESSOURCE_IMAGE: pList = &m_resourceImageFiles; break;
    case RESSOURCE_ENCAPSULATED: pList = &m_encapsulatedFiles; break;
    case RESSOURCE_DIRECTORY: pList = &m_resourceDirectories; break;
    }
    K2_ASSUME(pList != 0);   // m_dwResID >> 30 is 0..3, and every value has a case

    CExoLinkedListNode* pNode = pList->GetHeadPos();
    if (pNode == 0) {
        return -1;
    }
    CExoKeyTable* pTable = pList->GetAtPos(pNode);
    if (pTable != 0) goto check;
next:
    pTable = pList->GetNext(pNode);
    if (pNode == 0) return -1;
    if (pTable == 0) goto next;
check:
    if ((pTable->m_nRoutingTag & ROUTINGTAG_SLOT_MASK) != ((pRes->m_dwResID >> RESID_SLOT_SHIFT) & RESID_SLOT_MASK)) goto next;

    return pTable->GetEntryCount(resType);
}

void* CExoResMan::GetResOfType(unsigned short resType, CRes* pRes)
{
    CExoLinkedList<CExoKeyTable>* pList = 0;
    switch (pRes->m_dwResID >> RESID_SOURCE_SHIFT) {
    case RESSOURCE_BIF: pList = &m_fixedKeyTables; break;
    case RESSOURCE_IMAGE: pList = &m_resourceImageFiles; break;
    case RESSOURCE_ENCAPSULATED: pList = &m_encapsulatedFiles; break;
    case RESSOURCE_DIRECTORY: pList = &m_resourceDirectories; break;
    }
    K2_ASSUME(pList != 0);   // m_dwResID >> 30 is 0..3, and every value has a case

    CExoLinkedListNode* pNode = pList->GetHeadPos();
    if (pNode == 0) {
        return 0;
    }
    CExoKeyTable* pTable = pList->GetAtPos(pNode);
    if (pTable != 0) goto check;
next:
    pTable = pList->GetNext(pNode);
    if (pNode == 0) return 0;
    if (pTable == 0) goto next;
check:
    if ((pTable->m_nRoutingTag & ROUTINGTAG_SLOT_MASK) != ((pRes->m_dwResID >> RESID_SLOT_SHIFT) & RESID_SLOT_MASK)) goto next;
    if (pTable->m_bLookupDisabled != 0) {
        return 0;
    }
    return pTable->GetKeysOfType(resType);
}

int CExoResMan::GetKeyEntryFromTable(CExoLinkedList<CExoKeyTable>& list, const CResRef& resRef,
                                       unsigned short resType, CExoKeyTable** ppTable,
                                       CKeyTableEntry** ppEntry, unsigned int filterTag)
{
    CExoLinkedListNode* pNode = list.GetHeadPos();
    if (pNode == 0) {
        return false;
    }
    CExoKeyTable* pTable = list.GetAtPos(pNode);
    while (pNode) {
        if (filterTag != 0 ? (pTable && pTable->m_nInternalSubtype == filterTag) : pTable != 0) {
            if (pTable->m_bLookupDisabled == 0) {
                CKeyTableEntry* pEntry = pTable->FindKey(resRef, resType);
                if (pEntry) {
                    *ppTable = pTable;
                    *ppEntry = pEntry;
                    return true;
                }
            }
        }
        pTable = list.GetNext(pNode);
    }
    return false;
}

CExoStringList* CExoResMan::GetResOfTypeFromTable(CExoLinkedList<CExoKeyTable>& list, unsigned short resType)
{
    CExoLinkedListNode* pNode = list.m_pInternal->m_pHead;
    if (pNode == 0)
        return 0;

    CExoKeyTable* pTable = static_cast<CExoKeyTable*>(list.m_pInternal->GetAtPos(pNode));
    CExoStringList* pResult = 0;
    do {
        CExoStringList* pAccum = pResult;
        if (pTable != 0 && pTable->m_bLookupDisabled == 0) {
            CExoStringList* pNew = pTable->GetKeysOfType(resType);
            pResult = pNew;
            if (pAccum != 0) {
                pResult = pAccum->Merge(pNew, 0, 1);
                pAccum->~CExoStringList();
                operator delete(pAccum);
                if (pNew != 0) {
                    pNew->~CExoStringList();
                    operator delete(pNew);
                }
            }
        }
        pTable = static_cast<CExoKeyTable*>(list.m_pInternal->GetNext(pNode));
    } while (pNode != 0);

    return pResult;
}

CExoStringList* CExoResMan::GetResOfType(unsigned short resType, int bEncapsulatedOnly)
{
    CExoStringList* pAccum = GetResOfTypeFromTable(m_encapsulatedFiles, resType);
    if (bEncapsulatedOnly)
        return pAccum;

    CExoStringList* pNew = GetResOfTypeFromTable(m_resourceImageFiles, resType);
    CExoStringList* pResult = pNew;
    if (pAccum != 0) {
        pResult = pAccum->Merge(pNew, 0, 1);
        pAccum->~CExoStringList();
        operator delete(pAccum);
        if (pNew != 0) {
            pNew->~CExoStringList();
            operator delete(pNew);
        }
    }

    pAccum = pResult;
    pNew = GetResOfTypeFromTable(m_resourceDirectories, resType);
    pResult = pNew;
    if (pAccum != 0) {
        pResult = pAccum->Merge(pNew, 0, 1);
        pAccum->~CExoStringList();
        operator delete(pAccum);
        if (pNew != 0) {
            pNew->~CExoStringList();
            operator delete(pNew);
        }
    }

    pAccum = pResult;
    pNew = GetResOfTypeFromTable(m_fixedKeyTables, resType);
    pResult = pNew;
    if (pAccum != 0) {
        pResult = pAccum->Merge(pNew, 0, 1);
        pAccum->~CExoStringList();
        operator delete(pAccum);
        if (pNew != 0) {
            pNew->~CExoStringList();
            operator delete(pNew);
        }
    }

    return pResult;
}

CExoLocString CExoResMan::GetEncapsulatedFileDescription(const CExoString& path)
{
    CExoLocString sDescription;
    CExoEncapsulatedFile* pFile = new CExoEncapsulatedFile();
    pFile->m_sFileName = path;

    if (pFile->OpenFile() != 0) {
        sDescription = pFile->m_sDescription;

        pFile->CloseFile();
        delete pFile;
        return sDescription;
    }
    // sic: pFile is never closed or deleted on this path, a real leak
    return CExoLocString();
}

int CExoResMan::RemoveFile(const CExoString& path, unsigned short resType)
{
    CExoString resolved;
    resolved = g_pExoBase->m_pAliasList->ResolveFileName(path, resType);
#if KOTOR1  // K1 unlinks under the external-storage root when the file is there, else the bare path
    if (resolved.GetLength() == 0)
        return 0;
    std::string sPath(resolved.CStr());
    std::replace(sPath.begin(), sPath.end(), '\\', '/');
    std::string sFull;
    sFull = std::string(SDL_AndroidGetExternalStoragePath()) + "/" + sPath;
    if (access(sFull.c_str(), F_OK) == 0) {
        if (unlink(sFull.c_str()) != 0)
            return 0;
    } else if (unlink(sPath.c_str()) != 0) {
        return 0;
    }
    return 1;
#else
    if (resolved.m_sString == NULL || static_cast<int>(strlen(resolved.m_sString)) == 0)
        return 0;
    struct stat st;
    if (stat(resolved.CStr(), &st) == -1)
        return 0;
    if ((st.st_mode & MODE_TYPE_MASK) == MODE_DIRECTORY)
        return 0;
    static_cast<void>(chmod(resolved.CStr(), 0200));
    return remove(resolved.CStr()) != -1;
#endif
}

int CExoResMan::WipeDirectory(const CExoString& path, int a, int b, int c, int d)
{
    CExoString resolved;

    if (a != 0) {
        CExoArrayList<CExoString> fileList;
        CExoString name;
#if KOTOR1
        g_pExoBase->GetDirectoryList(&fileList, path, RESTYPE_INVALID, 0, 0, 0);
#else
        g_pExoBase->GetDirectoryList(&fileList, path, RESTYPE_INVALID, 0, 0);
#endif
        for (int i = 0; i < static_cast<int>(fileList.m_count); i++) {
            name = fileList.m_pArray[i];
            unsigned short resType = GetResTypeFromFile(name);
#if KOTOR1  // K1 removes only files whose resource type is known; no dot-file case
            if (resType != RESTYPE_INVALID) {
                int len = name.Find('.', 0);
#else
            char c0 = name[0];
            if (resType != RESTYPE_INVALID || c0 == '.') {
                int len = name.Find('.', 0);
                if (len == 0 && name.m_sString != NULL) {
                    len = static_cast<int>(strlen(name.m_sString));
                }
#endif
                if (len < 0)
                    return false;
#if KOTOR1  // K1 (Android port): "/" separators
                CExoString sep("/");
#else
                CExoString sep("\\");
#endif
                CExoString dirWithSep = path + sep;
                CExoString baseNoExt = name.SubString(0, len);
                CExoString fullPath = dirWithSep + baseNoExt;
                if (!RemoveFile(fullPath, resType))
                    return false;
            }
        }
    }

    if (d == 0) {
        if (c != 0) {
            CExoArrayList<CExoString> dirList;
            CExoString name;
#if KOTOR1
            g_pExoBase->GetDirectoryList(&dirList, path, RESTYPE_INVALID, 1, 0, 0);
#else
            g_pExoBase->GetDirectoryList(&dirList, path, RESTYPE_INVALID, 1, 0);
#endif
            for (int i = 0; i < static_cast<int>(dirList.m_count); i++) {
                name = dirList.m_pArray[i];
#if KOTOR1  // K1 (Android port): "/" separators
                CExoString sep("/");
#else
                CExoString sep("\\");
#endif
                CExoString dirWithSep = path + sep;
                CExoString subPath = dirWithSep + name;
                if (!WipeDirectory(subPath, a, 0, 1, 0))
                    return false;
            }
        }
    } else {
        CExoArrayList<CExoString> dirList;
        CExoString name;
#if KOTOR1
        g_pExoBase->GetDirectoryList(&dirList, path, RESTYPE_INVALID, 1, 0, 0);
#else
        g_pExoBase->GetDirectoryList(&dirList, path, RESTYPE_INVALID, 1, 0);
#endif
        for (int i = 0; i < static_cast<int>(dirList.m_count); i++) {
            name = dirList.m_pArray[i];
#if KOTOR1  // K1 (Android port): "/" separators
            CExoString sep("/");
#else
            CExoString sep("\\");
#endif
            CExoString dirWithSep = path + sep;
            CExoString subPath = dirWithSep + name;
            if (!WipeDirectory(subPath, a, 1, 1, 1))
                return false;
        }
    }

    if (b == 0)
        return true;

    resolved = g_pExoBase->m_pAliasList->ResolveFileName(path, RESTYPE_INVALID);
#if KOTOR1  // K1: CExoString::GetLength is out of line
    if (resolved.GetLength() == 0)
#else
    if (resolved.m_sString == NULL || static_cast<int>(strlen(resolved.m_sString)) == 0)
#endif
        return false;
#if KOTOR1
    return RemoveDirectory(resolved.CStr());
#else
    return RemoveDirectoryA(resolved.CStr());
#endif
}

void CExoResMan::NukeDirectory(const CExoString& path, int a, int b)
{
    WipeDirectory(path, a, 1, 0, b);
}

int CExoResMan::CleanDirectory(const CExoString& path, int a, int b)
{
    return WipeDirectory(path, 1, 0, b, a);
}

void* CExoResMan::Demand(CRes* pRes)
{
    if (pRes == 0) {
        return 0;
    }
    if (pRes->m_dwResID == RESID_INVALID) {
        return 0;
    }
    unsigned int dwFlags = pRes->m_dwFlags;
    if ((dwFlags & CRes::RESFLAG_RESIDENT) != 0 && pRes->m_pFileData != 0) {
        // Resident. If it was parked for freeing and nobody else holds it, take it back off the
        // free list. Walks the list directly rather than through GetAtPos/GetNext.
        if ((dwFlags & CRes::RESFLAG_FREE_PENDING) != 0 && pRes->m_nDemandCount == 0) {
            CExoLinkedListNode* pNode = m_freeResources.m_pInternal->m_pHead;
            CRes* pFree = 0;
            if (m_freeResources.m_pInternal->m_pHead != 0) {
                pFree = static_cast<CRes*>(m_freeResources.m_pInternal->m_pHead->m_pData);
            }
            while (pFree != 0) {
                if (pFree == pRes) {
                    m_freeResources.Remove(pNode);
                    pRes->m_dwFlags &= ~CRes::RESFLAG_FREE_PENDING;
                    break;
                }
                pFree = static_cast<CRes*>(m_freeResources.GetNext(pNode));
            }
        }
    } else if ((dwFlags & CRes::RESFLAG_LOADING) != 0) {
        if (m_pAsyncReadFile == 0) {
            // Returns without bumping the demand count, unlike every other exit.
            return pRes->m_pFileData;
        }
        while (static_cast<CExoFile*>(m_pAsyncReadFile)->ReadAsyncComplete() == 0) {
#if !KOTOR1  // K1 spins without yielding
            Sleep(1);
#endif
        }
        if (ServiceCurrentAsyncRes() == 0) {
            return 0;
        }
    } else {
        int bServiced = 0;
        switch (pRes->m_dwResID >> RESID_SOURCE_SHIFT) {
        case RESSOURCE_BIF: bServiced = ServiceFromResFile(pRes, 0);      break;
        case RESSOURCE_IMAGE: bServiced = ServiceFromImage(pRes, 0);        break;
        case RESSOURCE_ENCAPSULATED: bServiced = ServiceFromEncapsulated(pRes, 0); break;
        case RESSOURCE_DIRECTORY: bServiced = ServiceFromDirectory(pRes, 0);    break;
        }
        if (bServiced == 0) {
            return 0;
        }
        CExoLinkedListNode* pNode = m_demandQueue.m_pInternal->m_pHead;
        CRes* pQueued = 0;
        if (m_demandQueue.m_pInternal->m_pHead != 0) {
            pQueued = static_cast<CRes*>(m_demandQueue.m_pInternal->m_pHead->m_pData);
        }
        if (pNode != 0) {
            while (pQueued != pRes) {
                pQueued = static_cast<CRes*>(m_demandQueue.GetNext(pNode));
                if (pNode == 0) goto bump;
            }
            m_demandQueue.Remove(pNode);
        }
    }
bump:
    pRes->m_nDemandCount = pRes->m_nDemandCount + 1;
    return pRes->m_pFileData;
}

int CExoResMan::Release(CRes* pRes)
{
    if (pRes == 0) {
        return -1;
    }
    // Resource-image resources drop a container ref count on every release; the table lookup is
    // dereferenced without a null check.
    if ((pRes->m_dwResID >> RESID_SOURCE_SHIFT) == RESSOURCE_IMAGE) {
        static_cast<CExoPackedFile*>(GetTable(pRes)->m_paDependentFiles[0])->DeleteRefCount();
    }
    unsigned short nDemand = pRes->m_nDemandCount;
    if (nDemand >= 2) {
        pRes->m_nDemandCount = nDemand - 1;
        return nDemand;
    }
    if (nDemand == 0) {
        return -1;
    }
    pRes->m_nDemandCount = 0;
    if ((pRes->m_dwFlags & CRes::RESFLAG_FREE_PENDING) == 0) {
        m_freeResources.m_pInternal->AddHead(pRes);
        pRes->m_dwFlags |= CRes::RESFLAG_FREE_PENDING;
        return 0;
    }
    Dump(pRes, 0);
    if (pRes->m_pFileData != 0) {
        m_freeResources.m_pInternal->AddHead(pRes);
        Dump(pRes, 0);
    }
    pRes->m_dwFlags &= ~CRes::RESFLAG_FREE_PENDING;
    return 0;
}

int CExoResMan::CancelRequest(CRes* pRes)
{
    if (pRes == 0) {
        return 0;
    }
    unsigned int dwFlags = pRes->m_dwFlags;
    if ((dwFlags & CRes::RESFLAG_RESIDENT) != 0) {
        return 0;
    }
    unsigned short nRequests = pRes->m_nRequestCount;
    if (nRequests == 0) {
        return 0;
    }
    nRequests = nRequests - 1;
    pRes->m_nRequestCount = nRequests;

    if ((dwFlags & CRes::RESFLAG_LOADING) != 0 && nRequests == 0) {
        CExoKeyTable* pTable = GetTable(pRes);
        if (pTable == 0) {
            return 0;
        }
        unsigned int dwResID = pRes->m_dwResID;
        unsigned int nSourceTag = dwResID >> RESID_SOURCE_SHIFT;
        if (nSourceTag != 1 && nSourceTag != 2) {
            if (nSourceTag == 0) {
                static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[(dwResID >> RESID_FILE_SHIFT) & RESID_FILE_MASK])->DeleteRefCount();
            } else if (m_pAsyncReadFile != 0 && pRes->m_nRequestCount == 0) {
                delete static_cast<CExoFile*>(m_pAsyncReadFile);
            }
        } else {
            static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[0])->DeleteRefCount();
        }
        m_pDemandQueueHeadCache = 0;
        m_pAsyncReadFile = 0;
    }

    CExoLinkedListNode* pNode = m_demandQueue.m_pInternal->m_pHead;
    CRes* pQueued = 0;
    if (m_demandQueue.m_pInternal->m_pHead != 0) {
        pQueued = static_cast<CRes*>(m_demandQueue.m_pInternal->m_pHead->m_pData);
    }
    if (pNode != 0) {
        while (pQueued != pRes) {
            pQueued = static_cast<CRes*>(m_demandQueue.m_pInternal->GetNext(pNode));
            if (pNode == 0) goto done;
        }
        if (pRes->m_nRequestCount == 0) {
            m_demandQueue.m_pInternal->Remove(pNode);
        }
    }
done:
    return pRes->m_nRequestCount;
}

int CExoResMan::ServiceCurrentAsyncRes()
{
    CRes* pRes = static_cast<CRes*>(m_pDemandQueueHeadCache);
    CExoLinkedList<CExoKeyTable>* pList = 0;
    switch (pRes->m_dwResID >> RESID_SOURCE_SHIFT) {
    case RESSOURCE_BIF: pList = &m_fixedKeyTables; break;
    case RESSOURCE_IMAGE: pList = &m_resourceImageFiles; break;
    case RESSOURCE_ENCAPSULATED: pList = &m_encapsulatedFiles; break;
    case RESSOURCE_DIRECTORY: pList = &m_resourceDirectories; break;
    }
    K2_ASSUME(pList != 0);   // m_dwResID >> 30 is 0..3, and every value has a case

    CExoLinkedListNode* pNode = pList->GetHeadPos();
    if (pNode == 0) {
        return 0;
    }
    CExoKeyTable* pTable = pList->GetAtPos(pNode);
    if (pTable != 0) goto check;
next:
    pTable = pList->GetNext(pNode);
    if (pNode == 0) return 0;
    if (pTable == 0) goto next;
check:
    if ((pTable->m_nRoutingTag & ROUTINGTAG_SLOT_MASK) != ((pRes->m_dwResID >> RESID_SLOT_SHIFT) & RESID_SLOT_MASK)) goto next;

    {
        unsigned int dwResID = static_cast<CRes*>(m_pDemandQueueHeadCache)->m_dwResID;
        unsigned int nSourceTag = dwResID >> RESID_SOURCE_SHIFT;
        if (nSourceTag != 1 && nSourceTag != 2) {
            if (nSourceTag == 0) {
                static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[(dwResID >> RESID_FILE_SHIFT) & RESID_FILE_MASK])->DeleteAsyncRefCount();
            } else if (m_pAsyncReadFile != 0) {
                delete static_cast<CExoFile*>(m_pAsyncReadFile);
                m_pAsyncReadFile = 0;
            }
        } else {
            static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[0])->DeleteAsyncRefCount();
        }
    }

    CRes* pCur = static_cast<CRes*>(m_pDemandQueueHeadCache);
    pCur->m_dwFlags = (pCur->m_dwFlags & ~CRes::RESFLAG_LOADING) | CRes::RESFLAG_RESIDENT;
    if (pCur->m_pFileData == 0) {
        return 0;
    }
    pCur->m_nRequestCount = 0;
    int bServiced = pCur->OnResourceServiced();
    m_demandQueue.m_pInternal->RemoveHead();
    m_pDemandQueueHeadCache = 0;
    m_pAsyncReadFile = 0;
    return bServiced;
}

// Services one loose override-directory resource: build "<dir>[\<subdir>]\<resref>", open it,
// size it, allocate through the same Malloc path the container variants use, then either hand the
// read to CExoFile::ReadAsync (leaving the file object parked in m_pAsyncReadFile for
// ServiceCurrentAsyncRes to reap) or read it synchronously and report through OnResourceServiced.
int CExoResMan::ServiceFromDirectory(CRes* pRes, int bAsync)
{
    CExoString sPath;
    CExoString sFileName;
#if !KOTOR1
    const CExoString* pEntryDir;
#endif
    CExoFile* pFile;
    int nSize;

    if (pRes == 0) {
        return 0;
    }
    if ((pRes->m_dwFlags & CRes::RESFLAG_RESIDENT) != 0) {
        return 0;
    }
    if (pRes->m_pKeyEntry == 0) {
        return 0;
    }
    CExoLinkedListNode* pNode = m_resourceDirectories.GetHeadPos();
    if (pNode == 0) {
        return 0;
    }
    CExoKeyTable* pTable = m_resourceDirectories.GetAtPos(pNode);
    while (pNode != 0) {
        if (pTable != 0 && (pTable->m_nRoutingTag & ROUTINGTAG_SLOT_MASK) == ((pRes->m_dwResID >> RESID_SLOT_SHIFT) & RESID_SLOT_MASK))
            goto found;
        pTable = m_resourceDirectories.GetNext(pNode);
    }
    return 0;

found:
    pRes->m_pKeyEntry->m_resRef.CopyToString(sFileName);
    // The key entry's own m_sFileName is the resource's subdirectory under the override root, and is
    // only interposed when non-empty, tested as a raw NUL check rather than through the string
    // helpers.
#if KOTOR1  // K1 has no per-entry subdirectory
    sPath = pTable->m_sFileName + CExoString("/") + sFileName;
#else
    pEntryDir = &pRes->m_pKeyEntry->m_sFileName;
    if (pEntryDir->m_sString != NULL && pEntryDir->m_sString[0] != 0) {
        CExoString sSubDir = *pEntryDir;
        sPath = pTable->m_sFileName + CExoString("\\") + sSubDir + CExoString("\\") + sFileName;
    } else {
        sPath = pTable->m_sFileName + CExoString("\\") + sFileName;
    }
#endif
    // sic: the result goes nowhere, a debug branch compiled away in NDEBUG
    static_cast<void>(pTable->m_sFileName == "REBOOTDATA:");

    pFile = new CExoFile(sPath, pRes->m_pKeyEntry->m_resType, CExoString("rb"));
    if (pFile->FileOpened() == 0) {
        delete pFile;
        return 0;
    }
    nSize = pFile->GetSize();
    pRes->m_nSize = nSize;
    if (nSize == 0) {
        delete pFile;
        return 0;
    }
    Malloc(pRes);
    if (bAsync != 0) {
        m_pAsyncReadFile = pFile;
        pFile->ReadAsync(pRes->m_pFileData, nSize, 1);
        return 1;
    }
    pFile->Read(pRes->m_pFileData, nSize, 1);
    delete pFile;
    int bServiced = pRes->OnResourceServiced();
    pRes->m_dwFlags = (pRes->m_dwFlags & ~4u) | (static_cast<unsigned int>(bServiced != 0) << 2);
    return bServiced;
}

// The return-buffer ring's two static members are defined here; nothing outside this TU reaches
// them.
int  CResRef::s_nCurBufferIndex;
char CResRef::RETURN_BUFFERS[4][17];

char* CResRef::GetResRefStr() const
{
    s_nCurBufferIndex = (s_nCurBufferIndex + 1) % 4;
    char* pBuf = RETURN_BUFFERS[s_nCurBufferIndex];
    __builtin_memcpy(pBuf, m_sResRef, sizeof(m_sResRef));
    pBuf[16] = 0;
    return pBuf;
}

int CExoResMan::ServiceFromResFile(CRes* pRes, int bAsync)
{
    if (pRes == 0) {
        return 0;
    }
    if ((pRes->m_dwFlags & CRes::RESFLAG_RESIDENT) != 0) {
        return 0;
    }
    unsigned int nFileIdx = (pRes->m_dwResID >> RESID_FILE_SHIFT) & RESID_FILE_MASK;

    CExoLinkedListNode* pNode = m_fixedKeyTables.GetHeadPos();
    if (pNode == 0) {
        return 0;
    }
    CExoKeyTable* pTable = m_fixedKeyTables.GetAtPos(pNode);
    while (pNode) {
        if (pTable && (pTable->m_nRoutingTag & ROUTINGTAG_SLOT_MASK) == ((pRes->m_dwResID >> RESID_SLOT_SHIFT) & RESID_SLOT_MASK)) {
            if (bAsync != 0) {
                static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[nFileIdx])->AddAsyncRefCount();
                int nSize = static_cast<int>(static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[nFileIdx])->GetResourceSize(pRes->m_dwResID & RESID_INDEX_MASK));
                pRes->m_nSize = nSize;
                Malloc(pRes);
                m_pAsyncReadFile = static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[nFileIdx])->GetAsyncFile();
                static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[nFileIdx])->ReadResourceAsync(pRes->m_dwResID, pRes->m_pFileData, nSize, 0);
                return 1;
            }

            static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[nFileIdx])->AddRefCount();
            int nSize = static_cast<int>(static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[nFileIdx])->GetResourceSize(pRes->m_dwResID & RESID_INDEX_MASK));
            pRes->m_nSize = nSize;
            Malloc(pRes);
            int nRead = static_cast<int>(static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[nFileIdx])->ReadResource(pRes->m_dwResID, pRes->m_pFileData, nSize, 0));
            static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[nFileIdx])->DeleteRefCount();
            if (nRead != nSize) {
                return 0;
            }
            int bServiced = pRes->OnResourceServiced();
            if (bServiced != 0) {
                // sic: all four results are unused; the debug log that consumed them is compiled out
                pRes->m_pKeyEntry->m_resRef.GetResRefStr();
                g_pExoBase->GetResourceExtension(pRes->m_pKeyEntry->m_resType).CStr();
                static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[nFileIdx])->m_sFileName.CStr();
                pRes->m_pKeyEntry->m_resRef.GetResRefStr();
            }
            pRes->m_dwFlags = (pRes->m_dwFlags & ~4u) | (static_cast<unsigned int>(bServiced != 0) << 2);
            return bServiced;
        }
        pTable = m_fixedKeyTables.GetNext(pNode);
    }
    return 0;
}

int CExoResMan::ServiceFromEncapsulated(CRes* pRes, int bAsync)
{
    if (pRes == 0) {
        return 0;
    }
    if ((pRes->m_dwFlags & CRes::RESFLAG_RESIDENT) != 0) {
        return 0;
    }
    CExoLinkedListNode* pNode = m_encapsulatedFiles.GetHeadPos();
    if (pNode == 0) {
        return 0;
    }
    CExoKeyTable* pTable = m_encapsulatedFiles.GetAtPos(pNode);
    while (pNode) {
        if (pTable && (pTable->m_nRoutingTag & ROUTINGTAG_SLOT_MASK) == ((pRes->m_dwResID >> RESID_SLOT_SHIFT) & RESID_SLOT_MASK)) {
            if (bAsync != 0) {
                static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[0])->AddAsyncRefCount();
                int nSize = static_cast<int>(static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[0])->GetResourceSize(pRes->m_dwResID & RESID_INDEX_MASK));
                pRes->m_nSize = nSize;
                Malloc(pRes);
                m_pAsyncReadFile = static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[0])->GetAsyncFile();
                static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[0])->ReadResourceAsync(pRes->m_dwResID, pRes->m_pFileData, nSize, 0);
                return 1;
            }

            static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[0])->AddRefCount();
            int nSize = static_cast<int>(static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[0])->GetResourceSize(pRes->m_dwResID & RESID_INDEX_MASK));
            pRes->m_nSize = nSize;
            Malloc(pRes);
            int nRead = static_cast<int>(static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[0])->ReadResource(pRes->m_dwResID, pRes->m_pFileData, nSize, 0));
            static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[0])->DeleteRefCount();
            if (nRead != nSize) {
                return 0;
            }
            int bServiced = pRes->OnResourceServiced();
            // The async and sync flags differ only in bit 2.
            pRes->m_dwFlags = (pRes->m_dwFlags & ~4u) | (static_cast<unsigned int>(bServiced != 0) << 2);
            return bServiced;
        }
        pTable = m_encapsulatedFiles.GetNext(pNode);
    }
    return 0;
}

// The pack is already mapped, so this asks the image file for the size and for a pointer into its
// own mapping: no allocation and no copy, which is why the flags get bit 2 (borrowed data) instead
// of taking the allocate path.
int CExoResMan::ServiceFromImage(CRes* pRes, int nOffset)
{
    K2_UNUSED(nOffset);
    if (pRes == 0) {
        return 0;
    }
    if ((pRes->m_dwFlags & CRes::RESFLAG_RESIDENT) != 0) {
        return 0;
    }
    CExoLinkedListNode* pNode = m_resourceImageFiles.GetHeadPos();
    if (pNode == 0) {
        return 0;
    }
    CExoKeyTable* pTable = m_resourceImageFiles.GetAtPos(pNode);
    while (pNode) {
        if (pTable && (pTable->m_nRoutingTag & ROUTINGTAG_SLOT_MASK) == ((pRes->m_dwResID >> RESID_SLOT_SHIFT) & RESID_SLOT_MASK)) {
            unsigned int nResId = pRes->m_dwResID & RESID_INDEX_MASK;
            pRes->m_nSize = static_cast<CExoResourceImageFile*>(pTable->m_paDependentFiles[0])->GetResourceSize(nResId);
            pRes->m_pFileData = static_cast<CExoResourceImageFile*>(pTable->m_paDependentFiles[0])->GetResource(nResId);
            pRes->m_dwFlags |= CRes::RESFLAG_RESIDENT;
            return pRes->OnResourceServiced();
        }
        pTable = m_resourceImageFiles.GetNext(pNode);
    }
    return 0;
}

void CExoResMan::Update(unsigned int nTimeSlice)
{
    K2_UNUSED(nTimeSlice);
    if (m_bSuspended != 0) {
        return;
    }
    CExoLinkedListNode* pNode = m_demandQueue.m_pInternal->m_pHead;
    if (pNode == 0) {
        return;
    }
    if (m_pDemandQueueHeadCache == 0) {
        m_pDemandQueueHeadCache = pNode->m_pData;
    }
    CRes* pRes = static_cast<CRes*>(m_pDemandQueueHeadCache);
    if (pRes->m_dwResID == RESID_INVALID || (pRes->m_dwFlags & CRes::RESFLAG_RESIDENT) != 0) {
        m_demandQueue.m_pInternal->RemoveHead();
        m_pDemandQueueHeadCache = 0;
        m_pAsyncReadFile = 0;
        return;
    }
    if ((pRes->m_dwFlags & CRes::RESFLAG_LOADING) != 0) {
        if (m_pAsyncReadFile == 0) {
            return;
        }
        if (static_cast<CExoFile*>(m_pAsyncReadFile)->ReadAsyncComplete() == 0) {
            return;
        }
        ServiceCurrentAsyncRes();
        return;
    }

    int bServiced = 0;
    switch (pRes->m_dwResID >> RESID_SOURCE_SHIFT) {
    case RESSOURCE_BIF: bServiced = ServiceFromResFile(pRes, 1);      break;
    case RESSOURCE_IMAGE: bServiced = ServiceFromImage(pRes, 1);        break;
    case RESSOURCE_ENCAPSULATED: bServiced = ServiceFromEncapsulated(pRes, 1); break;
    case RESSOURCE_DIRECTORY: bServiced = ServiceFromDirectory(pRes, 1);    break;
    }
    if (bServiced == 0) {
        return;
    }
    CRes* pServiced = static_cast<CRes*>(m_pDemandQueueHeadCache);
    if ((pServiced->m_dwFlags & CRes::RESFLAG_RESIDENT) != 0) {
        return;
    }
    pServiced->m_dwFlags |= CRes::RESFLAG_LOADING;
}

// The four Raw workers behind ReadRaw read a resource's bytes straight into a caller-supplied buffer,
// without going through the CRes cache. All four open with the same by-routing-tag table scan, each
// against one hardcoded source list instead of a switch. The four bodies are near-duplicates but not
// interchangeable: each carries its own asymmetries.

int CExoResMan::ServiceFromResFileRaw(CRes* pRes, int nSize, char* pBuf)
{
    if (pRes == 0) {
        return 0;
    }
    // The BIF/KEY format is the only multi-file one: bits [20:29] of the resID pick which of the
    // table's dependent BIFs owns this resource.
    unsigned int nFileIdx = (pRes->m_dwResID >> RESID_FILE_SHIFT) & RESID_FILE_MASK;

    CExoLinkedListNode* pNode = m_fixedKeyTables.GetHeadPos();
    if (pNode == 0) {
        return 0;
    }
    CExoKeyTable* pTable = m_fixedKeyTables.GetAtPos(pNode);
    while (pNode) {
        if (pTable && (pTable->m_nRoutingTag & ROUTINGTAG_SLOT_MASK) == ((pRes->m_dwResID >> RESID_SLOT_SHIFT) & RESID_SLOT_MASK)) {
            static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[nFileIdx])->AddRefCount();
            // The only one of the four that clamps the request to the resource's real size.
            int nResSize = static_cast<int>(static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[nFileIdx])->GetResourceSize(pRes->m_dwResID & RESID_INDEX_MASK));
            if (nResSize < nSize) {
                nSize = nResSize;
            }
            pRes->m_nSize = nSize;
            if (static_cast<int>(static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[nFileIdx])->ReadResource(pRes->m_dwResID, pBuf, nSize, 0)) != nSize) {
                return 0;
            }
            static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[nFileIdx])->DeleteRefCount();
            return 1;
        }
        pTable = m_fixedKeyTables.GetNext(pNode);
    }
    return 0;
}

// Two asymmetries of its own: it never asks the container for the resource's real size, so a
// caller-supplied nSize larger than the resource overruns, and unlike every other variant it calls
// Free(pRes) on the way out.
int CExoResMan::ServiceFromEncapsulatedRaw(CRes* pRes, int nSize, char* pBuf)
{
    if (pRes == 0) {
        return 0;
    }
    CExoLinkedListNode* pNode = m_encapsulatedFiles.GetHeadPos();
    if (pNode == 0) {
        return 0;
    }
    CExoKeyTable* pTable = m_encapsulatedFiles.GetAtPos(pNode);
    while (pNode) {
        if (pTable && (pTable->m_nRoutingTag & ROUTINGTAG_SLOT_MASK) == ((pRes->m_dwResID >> RESID_SLOT_SHIFT) & RESID_SLOT_MASK)) {
            static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[0])->AddRefCount();
            pRes->m_nSize = nSize;
            if (static_cast<int>(static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[0])->ReadResource(pRes->m_dwResID, pBuf, nSize, 0)) != nSize) {
                return 0;
            }
            Free(pRes);
            static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[0])->DeleteRefCount();
            return 1;
        }
        pTable = m_encapsulatedFiles.GetNext(pNode);
    }
    return 0;
}

int CExoResMan::ServiceFromDirectoryRaw(CRes* pRes, int nSize, char* pBuf)
{
    CExoString sPath;
    CExoString sFileName;
    CExoFile* pFile;

    if (pRes == 0 || pRes->m_pKeyEntry == 0) {
        return 0;
    }
    CExoLinkedListNode* pNode = m_resourceDirectories.GetHeadPos();
    if (pNode == 0) {
        return 0;
    }
    CExoKeyTable* pTable = m_resourceDirectories.GetAtPos(pNode);
    while (pNode != 0) {
        if (pTable != 0 && (pTable->m_nRoutingTag & ROUTINGTAG_SLOT_MASK) == ((pRes->m_dwResID >> RESID_SLOT_SHIFT) & RESID_SLOT_MASK))
            goto found;
        pTable = m_resourceDirectories.GetNext(pNode);
    }
    return 0;

found:
    pRes->m_pKeyEntry->m_resRef.CopyToString(sFileName);
#if KOTOR1  // K1 (Android port): "/" separators
    sPath = pTable->m_sFileName + CExoString("/") + sFileName;
#else
    sPath = pTable->m_sFileName + CExoString("\\") + sFileName;
#endif
    pFile = new CExoFile(sPath, pRes->m_pKeyEntry->m_resType, CExoString("rb"));
    if (pFile->FileOpened() == 0) {
        delete pFile;
        return 0;
    }
    pRes->m_nSize = nSize;
    pFile->Read(pBuf, nSize, 1);
    delete pFile;
    return 1;
}

// Two asymmetries of its own: it asks for the resource size and then discards the answer, and it is
// the one Raw variant that allocates the resource's data, which the supplied buffer then makes
// redundant. Neither ref count is touched here.
int CExoResMan::ServiceFromImageRaw(CRes* pRes, int nSize, char* pBuf)
{
    if (pRes == 0) {
        return 0;
    }
    CExoLinkedListNode* pNode = m_resourceImageFiles.GetHeadPos();
    if (pNode == 0) {
        return 0;
    }
    CExoKeyTable* pTable = m_resourceImageFiles.GetAtPos(pNode);
    while (pNode) {
        if (pTable && (pTable->m_nRoutingTag & ROUTINGTAG_SLOT_MASK) == ((pRes->m_dwResID >> RESID_SLOT_SHIFT) & RESID_SLOT_MASK)) {
            static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[0])->GetResourceSize(pRes->m_dwResID & RESID_INDEX_MASK);
            pRes->m_nSize = nSize;
            Malloc(pRes);
            return static_cast<int>(static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[0])->ReadResource(pRes->m_dwResID, pBuf, nSize, 0)) == nSize;
        }
        pTable = m_resourceImageFiles.GetNext(pNode);
    }
    return 0;
}

int CExoResMan::ReadRaw(CRes* pRes, int a, char* pBuf)
{
    switch (pRes->m_dwResID >> RESID_SOURCE_SHIFT) {
    case RESSOURCE_BIF: return ServiceFromResFileRaw(pRes, a, pBuf);
    case RESSOURCE_IMAGE: return ServiceFromImageRaw(pRes, a, pBuf);
    case RESSOURCE_ENCAPSULATED: return ServiceFromEncapsulatedRaw(pRes, a, pBuf);
    case RESSOURCE_DIRECTORY: return ServiceFromDirectoryRaw(pRes, a, pBuf);
    }
    return 0;
}

void CExoResMan::Dump(CRes* pRes, int a)
{
    K2_UNUSED(a);
    if (pRes == 0) {
        return;
    }
    if (pRes->m_nDemandCount != 0) {
        pRes->m_dwFlags |= CRes::RESFLAG_FREE_PENDING;
        return;
    }
    if ((pRes->m_dwFlags & CRes::RESFLAG_HAS_DATA) != 0 && pRes->m_pFileData != 0) {
        Free(pRes);
    }
}

unsigned int CExoResMan::GetResID(const CResRef& resRef, unsigned short resType)
{
    CExoKeyTable* pTable = 0;
    CKeyTableEntry* pEntry = 0;
    int found = GetKeyEntry(resRef, resType, &pTable, &pEntry);
    if (found == 0) {
        return RESID_INVALID;
    }
    return pEntry->m_dwResID;
}

// Mint an unused variant of `resRef`: peel the trailing decimal run to get a base stem and a
// starting number, then count upwards, re-testing each candidate through GetKeyEntry until one is
// absent from every table.
//
// The digit scan's exits are order-sensitive: the non-digit test must come before the decrement, or
// the stem length comes out one short. szResRef has a known 20-byte bound, so the length and copy
// below are spelled as the fortified checked forms; the copy with a non-constant offset stays a
// plain call.
int CExoResMan::GetNewResRef(const CResRef& resRef, unsigned short resType, CResRef& resRefNew)
{
    resRefNew = resRef;

    char szResRef[20];
    __builtin_memcpy(szResRef, &resRefNew, sizeof(CResRef));
    szResRef[16] = '\0';

    int nLength = static_cast<int>(__strlen_chk(szResRef, sizeof(szResRef)));

    // Walk backwards over at most 8 trailing decimal digits; i ends on the last stem character.
    int i = nLength - 1;
    int nDigits = 0;
    char c = szResRef[i];
    while (c <= '9' && nDigits < 8) {
        if (c < '0')
            break;
        i--;
        if (i < 0)
            break;
        c = szResRef[i];
        nDigits++;
    }

    int nBaseLength = i > 12 ? 13 : i + 1;

    char* szBase = NULL;
    if (nBaseLength != 0) {
        szBase = static_cast<char*>(calloc(nBaseLength + 1, 1));
        __strncpy_chk2(szBase, szResRef, nBaseLength, static_cast<uint64_t>(-1), sizeof(szResRef));
    }

    char* szNumber;
    int nNumber = 0;
#ifdef K2_FIX_ENGINE_BUGS
    // Room for any int's "%d" and its NUL; the digit run is at most 8 long.
    szNumber = static_cast<char*>(calloc(12, 1));
    if (szNumber == NULL) {
        free(szBase);
        return 0;
    }
    if (i <= 12 && nLength != nBaseLength) {
        memcpy(szNumber, &szResRef[nBaseLength], static_cast<size_t>(nLength - nBaseLength));  // idiom-exempt: runtime length (the digit run, at most 8)
        nNumber = atoi(szNumber);
    }
#else
    if (i <= 12 && nLength != nBaseLength) {
        int nNumberLength = nLength - nBaseLength;
        szNumber = static_cast<char*>(calloc(nNumberLength, 1));
        strncpy(szNumber, &szResRef[nBaseLength], nNumberLength);
        nNumber = atoi(szNumber);
    } else {
        szNumber = static_cast<char*>(calloc(nBaseLength > 0 ? nBaseLength : 3, 1));
    }
#endif

    sprintf(szNumber, "%d", nNumber);

    int bResult = 0;
    int nCount = nNumber + 1;
    while (static_cast<int>(strlen(szNumber)) < 4 && nNumber >= 0) {
        if (szBase != NULL) {
            sprintf(szResRef, "%s%03d", szBase, nCount % 1000);
        } else {
            sprintf(szResRef, "%03d", nCount);
        }
        resRefNew = szResRef;

        CExoKeyTable* pTable = NULL;
        CKeyTableEntry* pEntry = NULL;
        int bExists = GetKeyEntry(resRefNew, resType, &pTable, &pEntry);
        sprintf(szNumber, "%d", nCount);
        nCount++;
        if (bExists == 0) {
            bResult = 1;
            break;
        }
    }

    if (szBase != NULL) {
        free(szBase);
    }
    if (szNumber != NULL) {
        free(szNumber);
    }
    return bResult;
}

// Walks all four key-table lists filling eight running totals and returns a ninth, the grand byte
// total, which is the only figure carrying the override corrections the middle two passes apply.
//
// One pass per list, and they are not the same shape:
//   - resource-image files are measured from the pack directory: every slot with a real resID
//     contributes the pack's resource size, and the pack's own file size goes to pnPackFileBytes,
//     the only out-param that pass touches.
//   - override directories and ERF/RIM archives are measured from resident resources. Each one
//     first subtracts whatever the same resref costs in the resource-image list, since an override
//     un-counts the packed copy it shadows, then adds its own size. The archives only subtract for
//     subtype 1, the first of the two filtered sweeps GetKeyEntry does.
//   - the fixed key tables are a plain resident pass with no override correction.
//
// The four per-type buckets follow the image's own CResHelper instantiations: 4 wave, 2002 model,
// 3007 TPC, 3008 MDX. 3006 has no helper but is bucketed with TPC, and TXB exists with no other
// candidate number, so it is the port's.
//
// The 3006 and 3007 arms of each switch must stay separate.
int CExoResMan::GetResourcesInMemory(int* pnResourceCount, int* pnKeyEntryCount,
                                     int* pnResourceBytes, int* pnPackFileBytes,
                                     int* pnModelBytes, int* pnModelExtBytes,
                                     int* pnTextureBytes, int* pnSoundBytes)
{
    CExoLinkedListNode* pNode;
    CExoKeyTable* pTable;
    CResRef resRef;

    *pnResourceCount = 0;
    *pnKeyEntryCount = 0;
    *pnResourceBytes = 0;
    *pnPackFileBytes = 0;
    *pnModelBytes = 0;
    *pnModelExtBytes = 0;
    *pnTextureBytes = 0;
    *pnSoundBytes = 0;

    int nTotalBytes = 0;

    // Resource-image packs are measured straight off the pack directory.
    pNode = m_resourceImageFiles.m_pInternal->m_pHead;
    if (pNode != 0) {
        pTable = static_cast<CExoKeyTable*>(m_resourceImageFiles.GetAtPos(pNode));
        while (1) {
            if (pTable != 0 && pTable->m_bLookupDisabled == 0) {
                *pnPackFileBytes += static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[0])->m_nFileSize;
                for (unsigned int j = 0; j < pTable->m_nTableSize; j++) {
                    CKeyTableEntry* pEntry = pTable->m_paEntries + j;
                    if (pEntry == 0) {
                        continue;
                    }
                    (*pnKeyEntryCount)++;
                    if (pEntry->m_dwResID == RESID_INVALID) {
                        continue;
                    }
                    int nSize = static_cast<CExoPackedFile*>(pTable->m_paDependentFiles[0])
                                    ->GetResourceSize(pEntry->m_dwResID);
                    nTotalBytes += nSize;
                    (*pnResourceCount)++;
                    switch (pEntry->m_resType) {
                    case RESTYPE_TXB: *pnTextureBytes += nSize; break;
                    case RESTYPE_TPC: *pnTextureBytes += nSize; break;
                    case RESTYPE_MDX: *pnModelExtBytes += nSize; break;
                    case RESTYPE_WAV: *pnSoundBytes += nSize; break;
                    case RESTYPE_MDL: *pnModelBytes += nSize; break;
                    }
                }
            }
            pTable = static_cast<CExoKeyTable*>(m_resourceImageFiles.GetNext(pNode));
            if (pNode == 0) {
                break;
            }
        }
    }

    // Override directories count their resident resources, minus the packed copies they shadow.
    pNode = m_resourceDirectories.m_pInternal->m_pHead;
    if (pNode != 0) {
        pTable = static_cast<CExoKeyTable*>(m_resourceDirectories.GetAtPos(pNode));
        while (pNode != 0) {
            if (pTable != 0 && pTable->m_bLookupDisabled == 0) {
                for (unsigned int j = 0; j < pTable->m_nTableSize; j++) {
                    CKeyTableEntry* pEntry = pTable->m_paEntries + j;
                    if (pEntry == 0) {
                        continue;
                    }
                    (*pnKeyEntryCount)++;
                    if (pEntry->m_pRes == 0 || (pEntry->m_pRes->m_dwFlags & CRes::RESFLAG_RESIDENT) == 0) {
                        continue;
                    }
                    resRef = pEntry->m_resRef;
                    unsigned short resType = pEntry->m_resType;

                    CExoLinkedListNode* pImageNode = m_resourceImageFiles.m_pInternal->m_pHead;
                    if (pImageNode != 0) {
                        CExoKeyTable* pImageTable =
                            static_cast<CExoKeyTable*>(m_resourceImageFiles.GetAtPos(pImageNode));
                        while (pImageNode != 0) {
                            CKeyTableEntry* pImageEntry;
                            if (pImageTable != 0 && pImageTable->m_bLookupDisabled == 0 &&
                                (pImageEntry = pImageTable->FindKey(resRef, resType)) != 0) {
                                int nPacked = static_cast<CExoPackedFile*>(pImageTable->m_paDependentFiles[0])
                                                  ->GetResourceSize(pImageEntry->m_dwResID);
                                nTotalBytes -= nPacked;
                                (*pnResourceCount)--;
                                switch (pImageEntry->m_resType) {
                                case RESTYPE_TXB: *pnTextureBytes -= nPacked; break;
                                case RESTYPE_TPC: *pnTextureBytes -= nPacked; break;
                                case RESTYPE_MDX: *pnModelExtBytes -= nPacked; break;
                                case RESTYPE_WAV: *pnSoundBytes -= nPacked; break;
                                case RESTYPE_MDL: *pnModelBytes -= nPacked; break;
                                }
                                break;
                            }
                            pImageTable =
                                static_cast<CExoKeyTable*>(m_resourceImageFiles.GetNext(pImageNode));
                        }
                    }

                    int nSize = pEntry->m_pRes->m_nSize;
                    *pnResourceBytes += nSize;
                    nTotalBytes += nSize;
                    (*pnResourceCount)++;
                    switch (pEntry->m_resType) {
                    case RESTYPE_TXB: *pnTextureBytes += nSize; break;
                    case RESTYPE_TPC: *pnTextureBytes += nSize; break;
                    case RESTYPE_MDX: *pnModelExtBytes += nSize; break;
                    case RESTYPE_WAV: *pnSoundBytes += nSize; break;
                    case RESTYPE_MDL: *pnModelBytes += nSize; break;
                    }
                }
            }
            pTable = static_cast<CExoKeyTable*>(m_resourceDirectories.GetNext(pNode));
        }
    }

    // ERF/RIM archives: the same, but only subtype 1 un-counts its packed shadow.
    pNode = m_encapsulatedFiles.m_pInternal->m_pHead;
    if (pNode != 0) {
        pTable = static_cast<CExoKeyTable*>(m_encapsulatedFiles.GetAtPos(pNode));
        while (pNode != 0) {
            if (pTable != 0 && pTable->m_bLookupDisabled == 0) {
                for (unsigned int j = 0; j < pTable->m_nTableSize; j++) {
                    CKeyTableEntry* pEntry = pTable->m_paEntries + j;
                    if (pEntry == 0) {
                        continue;
                    }
                    (*pnKeyEntryCount)++;
                    if (pEntry->m_pRes == 0 || (pEntry->m_pRes->m_dwFlags & CRes::RESFLAG_RESIDENT) == 0) {
                        continue;
                    }
                    if (pTable->m_nInternalSubtype == 1) {
                        resRef = pEntry->m_resRef;
                        unsigned short resType = pEntry->m_resType;

                        CExoLinkedListNode* pImageNode = m_resourceImageFiles.m_pInternal->m_pHead;
                        if (pImageNode != 0) {
                            CExoKeyTable* pImageTable =
                                static_cast<CExoKeyTable*>(m_resourceImageFiles.GetAtPos(pImageNode));
                            while (pImageNode != 0) {
                                CKeyTableEntry* pImageEntry;
                                if (pImageTable != 0 && pImageTable->m_bLookupDisabled == 0 &&
                                    (pImageEntry = pImageTable->FindKey(resRef, resType)) != 0) {
                                    int nPacked =
                                        static_cast<CExoPackedFile*>(pImageTable->m_paDependentFiles[0])
                                            ->GetResourceSize(pImageEntry->m_dwResID);
                                    nTotalBytes -= nPacked;
                                    (*pnResourceCount)--;
                                    switch (pImageEntry->m_resType) {
                                    case RESTYPE_TXB: *pnTextureBytes -= nPacked; break;
                                    case RESTYPE_TPC: *pnTextureBytes -= nPacked; break;
                                    case RESTYPE_MDX: *pnModelExtBytes -= nPacked; break;
                                    case RESTYPE_WAV: *pnSoundBytes -= nPacked; break;
                                    case RESTYPE_MDL: *pnModelBytes -= nPacked; break;
                                    }
                                    break;
                                }
                                pImageTable = static_cast<CExoKeyTable*>(m_resourceImageFiles.m_pInternal
                                                  ->GetNext(pImageNode));
                            }
                        }
                    }

                    int nSize = pEntry->m_pRes->m_nSize;
                    *pnResourceBytes += nSize;
                    nTotalBytes += nSize;
                    (*pnResourceCount)++;
                    switch (pEntry->m_resType) {
                    case RESTYPE_TXB: *pnTextureBytes += nSize; break;
                    case RESTYPE_TPC: *pnTextureBytes += nSize; break;
                    case RESTYPE_MDX: *pnModelExtBytes += nSize; break;
                    case RESTYPE_WAV: *pnSoundBytes += nSize; break;
                    case RESTYPE_MDL: *pnModelBytes += nSize; break;
                    }
                }
            }
            pTable = static_cast<CExoKeyTable*>(m_encapsulatedFiles.GetNext(pNode));
        }
    }

    // The base BIF/KEY tables get a plain resident pass.
    pNode = m_fixedKeyTables.m_pInternal->m_pHead;
    if (pNode != 0) {
        pTable = static_cast<CExoKeyTable*>(m_fixedKeyTables.GetAtPos(pNode));
        while (pNode != 0) {
            if (pTable != 0 && pTable->m_bLookupDisabled == 0) {
                for (unsigned int j = 0; j < pTable->m_nTableSize; j++) {
                    CKeyTableEntry* pEntry = pTable->m_paEntries + j;
                    if (pEntry == 0) {
                        continue;
                    }
                    (*pnKeyEntryCount)++;
                    if (pEntry->m_pRes == 0 || (pEntry->m_pRes->m_dwFlags & CRes::RESFLAG_RESIDENT) == 0) {
                        continue;
                    }
                    int nSize = pEntry->m_pRes->m_nSize;
                    *pnResourceBytes += nSize;
                    nTotalBytes += nSize;
                    (*pnResourceCount)++;
                    switch (pEntry->m_resType) {
                    case RESTYPE_TXB: *pnTextureBytes += nSize; break;
                    case RESTYPE_TPC: *pnTextureBytes += nSize; break;
                    case RESTYPE_MDX: *pnModelExtBytes += nSize; break;
                    case RESTYPE_WAV: *pnSoundBytes += nSize; break;
                    case RESTYPE_MDL: *pnModelBytes += nSize; break;
                    }
                }
            }
            pTable = static_cast<CExoKeyTable*>(m_fixedKeyTables.GetNext(pNode));
        }
    }

    return nTotalBytes;
}

// The async loader thread's per-wakeup body, run under the critical section. Mounts everything a
// module needs (its `_a` / `_adx` / `_s` resource-image packs, its module archive, every override
// directory's copy of the same, the `_dlg` dialog RIM and finally the current-game save overlay)
// and records what mounted in m_nModuleResourceFlags's bits. m_nAsyncLoadState is the completion status the polling caller
// reads: 4 failed, 3 mounted, 2 mounted with the `_s` layer from an override.
//
// The `%s\modules\%s` sprintf loops walk GetOverridePath(i) until it returns null. AddKeyTable takes
// a const CExoString& yet two call sites below build a temporary copy of an already-named string;
// the copies are intended, not accidental.
#if KOTOR1
// K1 has no override-directory probes (no `%s\modules\%s` loops, no module override
// add/remove), the `_s` layer mounts straight from the module alias, and there is no `_dlg` RIM.
void CExoResMan::AsyncLoad()
{
    m_critSection.EnterCriticalSection();

    CExoString sPath;
    CExoString sUnused;

    int bResult;
    int bOverrideLayer;

    if (m_bAsyncLoadModule == 0) {
        sPath = CExoString("RIMS:") + m_sAsyncModuleName;
        bResult = AddKeyTable(sPath, KEYTABLE_SOURCE_IMAGE, 0);
        bOverrideLayer = 0;
        goto finish;
    }

    sPath = m_sAsyncModuleName + CExoString("_a");
    if (Exists(CResRef(sPath), RESTYPE_RIM) != 0) {
        sPath = CExoString("RIMS:") + sPath;
        bResult = AddKeyTable(sPath, KEYTABLE_SOURCE_IMAGE, 0);
        if (bResult == 0) {
            bOverrideLayer = 0;
            goto currentGameOnly;
        }
        m_nModuleResourceFlags |= MODULERES_A_RIM;
    } else {
        bResult = 1;
    }

    sPath = m_sAsyncModuleName + CExoString("_adx");
    if (Exists(CResRef(sPath), RESTYPE_RIM) != 0) {
        sPath = CExoString("RIMS:") + sPath;
        bResult = AddKeyTable(sPath, KEYTABLE_SOURCE_IMAGE, 0);
        if (bResult == 0) {
            bOverrideLayer = 0;
            goto currentGameOnly;
        }
        m_nModuleResourceFlags |= MODULERES_ADX_RIM;
    }

    AddKeyTable(CExoString("MODULES:"), KEYTABLE_SOURCE_DIRECTORY, 0);

    if (Exists(CResRef(m_sAsyncModuleName), RESTYPE_MOD) != 0) {
        sPath = CExoString("MODULES:") + m_sAsyncModuleName;
        bResult = AddKeyTable(sPath, KEYTABLE_SOURCE_ENCAPSULATED, 2);
        bOverrideLayer = 0;
        if (bResult != 0) {
            m_nModuleResourceFlags |= MODULERES_MODULE_ARCHIVE;
        }
    } else {
        sPath = m_sAsyncModuleName + CExoString("_s");
        bOverrideLayer = 0;
        if (Exists(CResRef(sPath), RESTYPE_RIM) != 0) {
            sPath = CExoString("MODULES:") + sPath;
            bResult = AddKeyTable(sPath, KEYTABLE_SOURCE_IMAGE, 0);
            if (bResult != 0) {
                m_nModuleResourceFlags |= MODULERES_S_RIM;
                bOverrideLayer = 1;
            }
        }
    }

    RemoveKeyTable(CExoString("MODULES:"), KEYTABLE_SOURCE_DIRECTORY);
    if (bResult == 0 || m_bAsyncIsSaveGame != 0) {
        goto currentGameOnly;
    }

    sPath = CExoString("CURRENTGAME:") + m_sAsyncModuleName;
    AddKeyTable(CExoString("CURRENTGAME:"), KEYTABLE_SOURCE_DIRECTORY, 0);

    if (Exists(CResRef(m_sAsyncModuleName), RESTYPE_RSV) != 0 ||
        Exists(CResRef(m_sAsyncModuleName), RESTYPE_RIM) != 0) {
        bResult = AddKeyTable(sPath, KEYTABLE_SOURCE_IMAGE, 0);
        if (bResult != 0) {
            m_nModuleResourceFlags |= MODULERES_SAVED_RIM;
        }
    }

    RemoveKeyTable(CExoString("CURRENTGAME:"), KEYTABLE_SOURCE_DIRECTORY);
    goto finish;

currentGameOnly:
    sPath = CExoString("CURRENTGAME:") + m_sAsyncModuleName;
    bResult = AddKeyTable(sPath, KEYTABLE_SOURCE_ENCAPSULATED, 2);
    if (bResult != 0) {
        m_nModuleResourceFlags |= MODULERES_SAVED_ARCHIVE;
    }

finish:
    if (bResult == 0) {
        m_nAsyncLoadState = 4;
    } else if (bOverrideLayer == 0) {
        m_nAsyncLoadState = 3;
    } else {
        m_nAsyncLoadState = 2;
    }
    m_critSection.LeaveCriticalSection();
}
#else
void CExoResMan::AsyncLoad()
{
    m_critSection.EnterCriticalSection();

    CExoString sPath;
    CExoString sUnused;  // sic: constructed and destroyed, never read (a dead debug local)

    int bResult;
    int bOverrideLayer;
    int i;
    const char* pOverridePath;

    if (m_bAsyncLoadModule == 0) {
        sPath = CExoString("RIMS:") + m_sAsyncModuleName;
        bResult = AddKeyTable(sPath, KEYTABLE_SOURCE_IMAGE, 0);
        bOverrideLayer = 0;
        goto finish;
    }

    sPath = m_sAsyncModuleName + CExoString("_a");
    if (Exists(CResRef(sPath), RESTYPE_RIM) != 0) {
        sPath = CExoString("RIMS:") + sPath;
        bResult = AddKeyTable(sPath, KEYTABLE_SOURCE_IMAGE, 0);
        if (bResult == 0) {
            bOverrideLayer = 0;
            goto currentGameOnly;
        }
        m_nModuleResourceFlags |= MODULERES_A_RIM;
    } else {
        bResult = 1;
    }

    sPath = m_sAsyncModuleName + CExoString("_adx");
    if (Exists(CResRef(sPath), RESTYPE_RIM) != 0) {
        sPath = CExoString("RIMS:") + sPath;
        bResult = AddKeyTable(sPath, KEYTABLE_SOURCE_IMAGE, 0);
        if (bResult == 0) {
            bOverrideLayer = 0;
            goto currentGameOnly;
        }
        m_nModuleResourceFlags |= MODULERES_ADX_RIM;
    }

    AddKeyTable(CExoString("MODULES:"), KEYTABLE_SOURCE_DIRECTORY, 0);
    AddModuleOverrideResourceDirectories();

    if (Exists(CResRef(m_sAsyncModuleName), RESTYPE_MOD) != 0) {
        sPath = CExoString("MODULES:") + m_sAsyncModuleName;
        bResult = AddKeyTable(sPath, KEYTABLE_SOURCE_ENCAPSULATED, 2);
        for (i = 0;; i++) {
            char szBuf[200];
            pOverridePath = GetOverridePath(i, false);
            if (pOverridePath == NULL) {
                break;
            }
            sprintf(szBuf, "%s\\modules\\%s", pOverridePath, m_sAsyncModuleName.CStr());
            bResult |= AddKeyTable(CExoString(szBuf), KEYTABLE_SOURCE_ENCAPSULATED, 2);
        }
        bOverrideLayer = 0;
        if (bResult != 0) {
            m_nModuleResourceFlags |= MODULERES_MODULE_ARCHIVE;
        }
        goto unmountModules;
    }

    sPath = m_sAsyncModuleName + CExoString("_s");
    if (Exists(CResRef(sPath), RESTYPE_RIM) != 0) {
        for (i = 0;; i++) {
            char szBuf[200];
            pOverridePath = GetOverridePath(i, false);
            if (pOverridePath == NULL) {
                sPath = CExoString("MODULES:") + sPath;
                bResult = AddKeyTable(sPath, KEYTABLE_SOURCE_IMAGE, 0);
                break;
            }
            sprintf(szBuf, "%s\\modules\\%s", pOverridePath, sPath.CStr());
            bResult = AddKeyTable(CExoString(szBuf), KEYTABLE_SOURCE_IMAGE, 0);
            if (bResult != 0) {
                break;
            }
        }
        if (bResult != 0) {
            m_nModuleResourceFlags |= MODULERES_S_RIM;
            bOverrideLayer = 1;
        } else {
            bOverrideLayer = 0;
        }
    } else {
        bOverrideLayer = 0;
    }

    sPath = CExoString("MODULES:") + m_sAsyncModuleName + CExoString("_dlg");
    AddKeyTable(CExoString(sPath), KEYTABLE_SOURCE_ENCAPSULATED, 2);
    for (i = 0;; i++) {
        char szBuf[200];
        pOverridePath = GetOverridePath(i, false);
        if (pOverridePath == NULL) {
            break;
        }
        sprintf(szBuf, "%s\\modules\\%s_dlg", pOverridePath, m_sAsyncModuleName.CStr());
        CExoString sDialogPath(szBuf);
        AddKeyTable(CExoString(sDialogPath), KEYTABLE_SOURCE_ENCAPSULATED, 2);
    }

unmountModules:
    RemoveModuleOverrideResourceDirectories();
    RemoveKeyTable(CExoString("MODULES:"), KEYTABLE_SOURCE_DIRECTORY);
    if (bResult == 0 || m_bAsyncIsSaveGame != 0) {
        goto currentGameOnly;
    }

    sPath = CExoString("CURRENTGAME:") + m_sAsyncModuleName;
    AddKeyTable(CExoString("CURRENTGAME:"), KEYTABLE_SOURCE_DIRECTORY, 0);

    if (Exists(CResRef(m_sAsyncModuleName), RESTYPE_RSV) != 0 ||
        Exists(CResRef(m_sAsyncModuleName), RESTYPE_RIM) != 0) {
        bResult = AddKeyTable(sPath, KEYTABLE_SOURCE_IMAGE, 0);
        if (bResult != 0) {
            m_nModuleResourceFlags |= MODULERES_SAVED_RIM;
        }
    }

    RemoveKeyTable(CExoString("CURRENTGAME:"), KEYTABLE_SOURCE_DIRECTORY);
    goto finish;

currentGameOnly:
    sPath = CExoString("CURRENTGAME:") + m_sAsyncModuleName;
    bResult = AddKeyTable(sPath, KEYTABLE_SOURCE_ENCAPSULATED, 2);
    if (bResult != 0) {
        m_nModuleResourceFlags |= MODULERES_SAVED_ARCHIVE;
    }

finish:
    if (bResult == 0) {
        m_nAsyncLoadState = 4;
    } else if (bOverrideLayer == 0) {
        m_nAsyncLoadState = 3;
    } else {
        m_nAsyncLoadState = 2;
    }
    m_critSection.LeaveCriticalSection();
}
#endif

int CExoResMan::Exists(const CResRef& resRef, unsigned short resType, unsigned int* pSourceType)
{
    CExoKeyTable* pTable = 0;
    CKeyTableEntry* pEntry = 0;
    int found = GetKeyEntry(resRef, resType, &pTable, &pEntry);
    if (pSourceType != 0 && found != 0) {
        *pSourceType = pTable->m_nSourceType;
    }
    return found;
}

int CExoResMan::Exists(const CResRef& resRef, unsigned short resType, CExoString& sFileName)
{
    CExoKeyTable* pTable = 0;
    CKeyTableEntry* pEntry = 0;
    int found = GetKeyEntry(resRef, resType, &pTable, &pEntry);
    if (found == 1 && pTable != 0) {
        sFileName = pTable->m_sFileName;
    } else {
        sFileName = "";
    }
    return found;
}

void CExoResMan::SetResObject(const CResRef& resRef, unsigned short resType, CRes* pRes)
{
    CExoKeyTable* pTable = 0;
    CKeyTableEntry* pEntry = 0;
    if (pRes != 0 && GetKeyEntry(resRef, resType, &pTable, &pEntry) != 0 && pEntry->m_pRes == 0) {
        pEntry->m_pRes = pRes;
        pRes->m_pKeyEntry = pEntry;
        pRes->m_nHolderCount = 1;
        pRes->m_dwResID = pEntry->m_dwResID;
    }
}

CExoKeyTable* CExoResMan::GetTable(CRes* pRes)
{
    CExoLinkedList<CExoKeyTable>* pList = 0;
    switch (pRes->m_dwResID >> RESID_SOURCE_SHIFT) {
    case RESSOURCE_BIF: pList = &m_fixedKeyTables; break;
    case RESSOURCE_IMAGE: pList = &m_resourceImageFiles; break;
    case RESSOURCE_ENCAPSULATED: pList = &m_encapsulatedFiles; break;
    case RESSOURCE_DIRECTORY: pList = &m_resourceDirectories; break;
    }
    K2_ASSUME(pList != 0);   // m_dwResID >> 30 is 0..3, and every value has a case

    CExoLinkedListNode* pNode = pList->GetHeadPos();
    if (pNode == 0) {
        return 0;
    }
    CExoKeyTable* pTable = pList->GetAtPos(pNode);
    if (pTable != 0) goto check;
next:
    pTable = pList->GetNext(pNode);
    if (pNode == 0) return 0;
    if (pTable == 0) goto next;
check:
    if ((pTable->m_nRoutingTag & ROUTINGTAG_SLOT_MASK) != ((pRes->m_dwResID >> RESID_SLOT_SHIFT) & RESID_SLOT_MASK)) goto next;

    return pTable;
}

CRes* CExoResMan::GetResObject(const CResRef& resRef, unsigned short resType)
{
    CExoKeyTable* pTable = 0;
    CKeyTableEntry* pEntry = 0;
    int found = GetKeyEntry(resRef, resType, &pTable, &pEntry);
    CRes* pRes;
    if (found == 0) {
        pRes = 0;
    } else {
        pRes = pEntry->m_pRes;
        if (pRes != 0) {
            pRes->m_nHolderCount = pRes->m_nHolderCount + 1;
        }
        if (pTable->m_nSourceType != KEYTABLE_SOURCE_IMAGE) {
            if (pRes == 0 || pRes->m_nHolderCount == 1) {
                CExoString sPath;
                CExoString sType;
                CExoString sExt;
                // The arms are in reverse of the natural comparison order.
                if (pTable->m_nSourceType == KEYTABLE_SOURCE_ENCAPSULATED) {
                    sType = "ERF";
                } else if (pTable->m_nSourceType == KEYTABLE_SOURCE_DIRECTORY) {
                    CExoKeyTable* pScan;
                    CExoLinkedListNode* pNode = m_resourceImageFiles.m_pInternal->m_pHead;
                    if (pNode != 0) {
                        pScan = static_cast<CExoKeyTable*>(m_resourceImageFiles.m_pInternal->GetAtPos(pNode));
                        while (1) {
                            if (pScan != 0 && pScan->m_bLookupDisabled == 0 && pScan->FindKey(resRef, resType) != 0)
                                goto done;
                            pScan = static_cast<CExoKeyTable*>(m_resourceImageFiles.m_pInternal->GetNext(pNode));
                            if (pNode == 0) break;
                        }
                    }
                    sType = "DIR";
                } else if (pTable->m_nSourceType == KEYTABLE_SOURCE_FIXED) {
                    sType = "BIF";
                    goto done;
                }
                sExt = g_pExoBase->GetResourceExtension(pEntry->m_resType);
                resRef.CopyToString(sPath);
                // sic: these CStr() results are unused; the debug log that consumed them compiles
                // away in this build.
                sPath.CStr();
                sExt.CStr();
                sType.CStr();
                pTable->m_sFileName.CStr();
            done:
                ;
            }
        }
        pRes = pEntry->m_pRes;
    }
    return pRes;
}
int CExoResMan::Exists(const CResRef& resRef, unsigned short resType, unsigned int sourceType)
{
    CExoLinkedList<CExoKeyTable>* pList;
    switch (sourceType) {
    case KEYTABLE_SOURCE_FIXED: pList = &m_fixedKeyTables; break;
    case KEYTABLE_SOURCE_DIRECTORY: pList = &m_resourceDirectories; break;
    case KEYTABLE_SOURCE_ENCAPSULATED: pList = &m_encapsulatedFiles; break;
    case KEYTABLE_SOURCE_IMAGE: pList = &m_resourceImageFiles; break;
    default: return 0;
    }

    CExoLinkedListNode* pNode = pList->GetHeadPos();
    if (pNode == 0) {
        return 0;
    }
    CExoKeyTable* pTable = pList->GetAtPos(pNode);
    while (1) {
        if (pTable != 0 && pTable->m_bLookupDisabled == 0 && pTable->FindKey(resRef, resType) != 0)
            return 1;
        pTable = pList->GetNext(pNode);
        if (pNode == 0) return 0;
    }
}
void CExoResMan::DumpTable(CExoLinkedList<CExoKeyTable>& list)
{
    CExoLinkedListNode* pNode = list.m_pInternal->m_pHead;
    if (pNode == 0)
        return;

    CExoKeyTable* pTable = static_cast<CExoKeyTable*>(list.m_pInternal->GetAtPos(pNode));
    while (1) {
        if (pTable != 0) {
            for (unsigned int i = 0; i < pTable->m_nTableSize; i++) {
                CKeyTableEntry* pEntry = &pTable->m_paEntries[i];
                if (pEntry == 0 || pEntry->m_pRes == 0)
                    continue;
                CRes* pRes = pEntry->m_pRes;
                if (pRes->m_nDemandCount != 0) {
                    pRes->m_dwFlags |= CRes::RESFLAG_FREE_PENDING;
                    continue;
                }
                if ((pRes->m_dwFlags & CRes::RESFLAG_HAS_DATA) == 0)
                    continue;
                if (pRes->m_pFileData == 0)
                    continue;
                Free(pRes);
            }
        }
        pTable = static_cast<CExoKeyTable*>(list.m_pInternal->GetNext(pNode));
        if (pNode == 0)
            return;
    }
}

void CExoResMan::DumpAll()
{
    DumpTable(m_encapsulatedFiles);
    DumpTable(m_resourceImageFiles);
    DumpTable(m_resourceDirectories);
    DumpTable(m_fixedKeyTables);
}

// Both overloads hand the path straight back. The call target matters: returning a default
// CExoString instead is the same code, but leaves dialog.tlk unopened.
CExoString CExoResMan::CacheResource(const CExoString& path, unsigned short resType)
{
    K2_UNUSED(resType);
    return path;
}

CExoString CExoResMan::CacheResource(const CExoString& path, unsigned short resType, int& out)
{
    K2_UNUSED(resType); K2_UNUSED(out);
    return path;
}

// The engine-wide in-place byte-swap family, called from every on-disk format reader (CResGFF,
// CRes4PC, CResDDS, CResPLT, CAuroraCompressedTexture, ...). The engine also shipped on big-endian
// consoles, so every header field is round-tripped through one of these; on a little-endian target
// each body is empty. The pointer overloads declared alongside were never defined. The order is the
// integer widths, then float and double, then unsigned long.

void ByteSwap(unsigned short* p) { K2_UNUSED(p); }
void ByteSwap(short* p) { K2_UNUSED(p); }
void ByteSwap(unsigned int* p) { K2_UNUSED(p); }
void ByteSwap(int* p) { K2_UNUSED(p); }
void ByteSwap(unsigned long long* p) { K2_UNUSED(p); }
#if KOTOR1
void ByteSwap(int64_t* p) { K2_UNUSED(p); }
#else
void ByteSwap(long long* p) { K2_UNUSED(p); }
#endif
void ByteSwap(float* p) { K2_UNUSED(p); }
void ByteSwap(double* p) { K2_UNUSED(p); }
#if K2_INT64_IS_LONG
void ByteSwap(uint64_t* p) { K2_UNUSED(p); }
#endif

// CResRef leaf accessors, declared in CResRef.h.

FILE* g_fpNRL;

// The raw 16-byte key as a copyable aggregate, so every whole-key move goes through a struct
// assignment, since a memcpy or memset spelling would become a real call.
struct ResRefBytes { char b[16]; };

// The two RIM leak-tracking console helpers. Both are dead code in this build: nothing branches to
// them.
char* RIMStartTracking(char* pszArgs)
{
#if KOTOR1
    // K1 goes through the port's file/line-tagged _fopen
    g_fpNRL = _fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/libsource/exoresman/exoresman.cpp",
                     "RIMStartTracking", 126, pszArgs, "w");
#else
    g_fpNRL = fopen(pszArgs, "w");
#endif
    return g_fpNRL != 0 ? const_cast<char*>("tracking begun") : const_cast<char*>("could not open file");
}

// Closes the handle when one is open. The global is not cleared, and the argument is ignored.
char* RIMStopTracking(char* pszArgs)
{
    K2_UNUSED(pszArgs);
    if (g_fpNRL != 0)
    {
        fclose(g_fpNRL);
        return const_cast<char*>("tracking stopped");
    }
    return const_cast<char*>("wasn't tracking");
}

CResRef::CResRef()
{
    *reinterpret_cast<ResRefBytes*>(m_sResRef) = ResRefBytes();  // raw-offset: a resref is 16 name bytes, compared as words
}

void CResRef::GetResRef(unsigned char* out) const
{
    *reinterpret_cast<ResRefBytes*>(out) = *reinterpret_cast<const ResRefBytes*>(m_sResRef);  // raw-offset: a resref is 16 name bytes, compared as words
}

void CResRef::CopyToString(char* out) const
{
    *reinterpret_cast<ResRefBytes*>(out) = *reinterpret_cast<const ResRefBytes*>(m_sResRef);  // raw-offset: a resref is 16 name bytes, compared as words
    out[16] = 0;
}

// Each of these is a forwarder whose only statement is the matching assignment.
CResRef::CResRef(char* s)
{
    *this = s;
}

CResRef::CResRef(const char* s)
{
    *this = const_cast<char*>(s);
}

// Kept out of line so every forwarding constructor shares this one copy routine.
CResRef::CResRef(const CExoString& s)
{
    *this = s;
}

// Builds the key from a CExoString by zeroing the whole key, taking the source length, clamping it
// to 16, copying that many bytes and lower-casing them. A null source takes the all-zero spelling
// and returns early, so the copy only ever sees a live pointer. The ternary in nLength's
// initialiser is what puts one zero-store on each arm of the length test.
__attribute__((noinline))
CResRef CResRef::operator=(const CExoString& s)
{
#if KOTOR1  // K1: CExoString::GetLength is out of line
    unsigned int nLength = s.GetLength();
#else
    unsigned int nLength = (s.m_sString ? static_cast<unsigned int>(strlen(s.m_sString)) : 0);
#endif
    *reinterpret_cast<ResRefBytes*>(m_sResRef) = ResRefBytes();  // raw-offset: a resref is 16 name bytes, compared as words
    if (nLength != 0)
    {
        unsigned int nCopy = (nLength < 16u) ? nLength : 16u;
        memcpy(m_sResRef, s.CStr(), nCopy);  // idiom-exempt: runtime length (clamped source length)
        for (unsigned int i = 0; i < nCopy; i++)
            m_sResRef[i] = static_cast<char>(tolower(static_cast<unsigned char>(m_sResRef[i])));
    }
    return *this;
}

CResRef::CResRef(unsigned char* s)
{
    *this = s;
}

// Builds the key from a length plus raw bytes. At most 16 bytes come in, and whatever is left of
// the key is zeroed. A null source takes the same all-zero spelling as the default constructor and
// returns early. The pad count is kept as the byte-typed local that drives both the fill and the
// copy, so both lengths are computed from it.
CResRef::CResRef(unsigned char* s, unsigned char n)
{
    if (s == 0)
    {
        *reinterpret_cast<ResRefBytes*>(m_sResRef) = ResRefBytes();  // raw-offset: a resref is 16 name bytes, compared as words
        return;
    }

    unsigned char nPad = 16 - ((n < 16) ? n : 16);
    memset(m_sResRef + 16 - nPad, 0, nPad);  // idiom-exempt: a runtime byte count, not a sizeof
    memcpy(m_sResRef, s, 16 - nPad);         // idiom-exempt: a runtime byte count, not a sizeof
}

// Repacks the key into a 17-byte NUL-terminated buffer and hands that buffer to the string class.
void CResRef::CopyToString(CExoString& out) const
{
    char sBuf[17];
    CopyToString(sBuf);
    out = sBuf;
}

int CResRef::IsValid() const
{
    return m_sResRef[0] != 0;
}

int CResRef::operator!()
{
    return m_sResRef[0] == 0;
}

char* CResRef::GetResRef() const
{
    return const_cast<char*>(m_sResRef);
}

unsigned char CResRef::GetLength() const
{
    for (int i = 0; i < 16; i++)
        if (m_sResRef[i] == 0)
            return static_cast<unsigned char>(i);
    return 16;
}

// Keys are already lower-cased on assignment, so no case folding is needed here.
int CResRef::operator==(const CResRef& other) const
{
    return memcmp(other.m_sResRef, m_sResRef, 16) == 0;
}

int CResRef::operator!=(const CResRef& other) const
{
    return memcmp(other.m_sResRef, m_sResRef, 16) != 0;
}

// Case-insensitive comparison against a C string, bounded by the 16-byte key: both sides are folded
// with tolower() and the walk stops at the first difference or at a shared NUL. The argument's
// bytes are signed, so the cast is needed on the stored side only.
int CResRef::operator==(char* s) const
{
    for (int i = 0; i < 16; i++) {
        int c = tolower(s[i]);
        if (c != tolower(static_cast<unsigned char>(m_sResRef[i])))
            return 0;
        if (c == 0)
            return 1;
    }
    return 1;
}

int CResRef::operator!=(char* s) const
{
    for (int i = 0; i < 16; i++) {
        int c = tolower(s[i]);
        if (c != tolower(static_cast<unsigned char>(m_sResRef[i])))
            return 1;
        if (c == 0)
            return 0;
    }
    return 0;
}

// Compares the string's bytes, including its NUL, against the key, clamped to the buffer; a null
// or empty string therefore compares just the one NUL byte. The length is open-coded here rather
// than taken from the string class.
bool CResRef::operator==(const CExoString& s) const
{
#if KOTOR1  // K1: an IsEmpty() call whose result is unused
    s.IsEmpty();
#endif
    char* pStr = s.CStr();
#if KOTOR1  // K1: CExoString::GetLength is out of line
    unsigned int nLength = s.GetLength() + 1;
#else
    size_t nLength = static_cast<unsigned int>((s.m_sString ? static_cast<int>(strlen(s.m_sString)) : 0) + 1);
#endif
    if (nLength > 15)
        nLength = 16;
    return memcmp(pStr, m_sResRef, nLength) == 0;
}

// int rather than bool: the engine's only call site normalizes the result.
int CResRef::operator!=(const CExoString& s) const
{
#if KOTOR1  // K1: an IsEmpty() call whose result is unused
    s.IsEmpty();
#endif
    char* pStr = s.CStr();
#if KOTOR1  // K1: CExoString::GetLength is out of line
    unsigned int nLength = s.GetLength() + 1;
#else
    size_t nLength = static_cast<unsigned int>((s.m_sString ? static_cast<int>(strlen(s.m_sString)) : 0) + 1);
#endif
    if (nLength > 15)
        nLength = 16;
    return memcmp(pStr, m_sResRef, nLength) != 0;
}

// Unlike the trivial memberwise copy, this lower-cases every byte on the way in, which keeps the
// key canonical however the source was built. Returns the new value by value.
CResRef CResRef::operator=(const CResRef& other)
{
    for (int i = 0; i < 16; i++)
        m_sResRef[i] = static_cast<char>(tolower(static_cast<unsigned char>(other.m_sResRef[i])));
    return *this;
}

// Assigns the key from a C string, zeroing the whole key first, then copying and lower-casing byte
// by byte, stopping at the source's NUL or at 16. The bytes are read as signed char, so no cast is
// needed on the way in.
CResRef CResRef::operator=(char* s)
{
    *reinterpret_cast<ResRefBytes*>(m_sResRef) = ResRefBytes();  // raw-offset: a resref is 16 name bytes, compared as words

    if (s == 0)
        return *this;

    for (int i = 0; i < 16; i++)
    {
        int c = s[i];
        if (c == 0)
            break;
        m_sResRef[i] = static_cast<char>(tolower(c));
    }

    return *this;
}

// Assigns from raw on-disk resref bytes. The 16 bytes are copied whole, with no NUL scan and no
// pre-zeroing, then the byte-at-a-time lower-casing runs over the destination until it hits a NUL.
// The fill starts one past the NUL, so a run that fills all 16 bytes returns without touching the
// buffer.
CResRef CResRef::operator=(unsigned char* s)
{
    *reinterpret_cast<ResRefBytes*>(m_sResRef) = *reinterpret_cast<const ResRefBytes*>(s);  // raw-offset: a resref is 16 name bytes, compared as words

    for (int i = 0; i < 16; i++)
    {
        if (m_sResRef[i] == 0)
        {
            memset(m_sResRef + i + 1, 0, 15 - i);  // idiom-exempt: a runtime byte count, not a sizeof
            break;
        }
        m_sResRef[i] = static_cast<char>(tolower(static_cast<unsigned char>(m_sResRef[i])));
    }

    return *this;
}

// Round-trips the key through a CExoString so the concatenation and the lower-casing are the string
// class's, then truncates the result back into the fixed 16-byte buffer. The key is zeroed first,
// so an append that overflows leaves exactly the first 16 characters and no stale tail.
CResRef CResRef::operator+=(const CExoString& s)
{
    CExoString sCombined;
    CopyToString(sCombined);
    sCombined = sCombined + s;
    sCombined = sCombined.LowerCase();

    *reinterpret_cast<ResRefBytes*>(m_sResRef) = ResRefBytes();  // raw-offset: a resref is 16 name bytes, compared as words

#if KOTOR1  // K1: CExoString::GetLength is out of line (called once for the clamp)
    if (sCombined.GetLength() > 0) {
        const char* pStr = sCombined.CStr();
        unsigned int nLen = sCombined.GetLength();
        memcpy(m_sResRef, pStr, nLen < 16 ? nLen : 16);  // idiom-exempt: runtime length
    }
#else
    if (static_cast<int>(sCombined.m_sString ? strlen(sCombined.m_sString) : 0) > 0)
        memcpy(m_sResRef, sCombined.CStr(),
               (sCombined.m_sString ? strlen(sCombined.m_sString) : 0) > 16
                   ? 16
                   : (sCombined.m_sString ? strlen(sCombined.m_sString) : 0));
#endif
    return *this;
}

// The resource base-class leaf accessors, laid out in CRes.h.


void* CRes::GetData()
{
    return m_pFileData;
}

int CRes::GetDemands()
{
    return m_nDemandCount;
}

unsigned short CRes::GetRequests()
{
    return m_nRequestCount;
}

unsigned int CRes::GetID()
{
    return m_dwResID;
}

int CRes::GetSize()
{
    return m_nSize;
}
