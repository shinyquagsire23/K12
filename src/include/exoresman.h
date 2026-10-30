#pragma once

#ifndef CEXORESMAN_H
#define CEXORESMAN_H

#include "k2stdint.h"
#include "exostring.h"

// The base-lib resource manager, the engine's virtual filesystem over every resource source
// (base-game BIF/KEY tables, override-folder loose files, ERF/RIM archives and resource-image
// packs), plus the CRes cache and its background loading thread.
//
// This header carries only the class itself, its by-value helper and forward declarations of
// everything else, so it can be included without pulling in the heavier internal models.

class CRes;
class CResRef;
class CKeyTableEntry;
class CExoKeyTable;
class CExoStringList;
class CExoLocString;

// A key table's source, the list it goes on (the sourceType of CExoResMan's Add/Remove/
// UpdateKeyTable and Exists, CExoKeyTable::m_nSourceType): a fixed KEY file's BIFs, a resource
// directory, an encapsulated (ERF/RIM/MOD) file, a resource image. NONE is a table not yet built.
enum {
    KEYTABLE_SOURCE_NONE         = 0,
    KEYTABLE_SOURCE_FIXED        = 1,
    KEYTABLE_SOURCE_DIRECTORY    = 2,
    KEYTABLE_SOURCE_ENCAPSULATED = 3,
    KEYTABLE_SOURCE_IMAGE        = 4,
};

#include "exocritsect.h"
// The six lists below are by-value CExoLinkedList<T> members, so the complete type is needed here.
#include "CExoLinkedList.h"

class CExoResMan {
public:
    CExoResMan();
    ~CExoResMan();

    int  GetTotalPhysicalMemory();
    int  SetTotalResourceMemory(int nBytes);

    // Looks a resource up through every source in the well-known Aurora precedence order: override
    // directories, then ERF/RIM archives, then resource-image packs, then base BIF/KEY last.
    int GetKeyEntry(const CResRef& resRef, unsigned short resType,
                      CExoKeyTable** ppTable, CKeyTableEntry** ppEntry);
    // Mints an unused ResRef derived from `resRef` by peeling off its trailing decimal run and
    // counting upwards until GetKeyEntry no longer finds a match.
    int GetNewResRef(const CResRef& resRef, unsigned short resType, CResRef& resRefNew);

    // Takes the resource-memory census behind the engine's memory report, with eight running totals
    // out and the grand byte total returned.
    int GetResourcesInMemory(int* pnResourceCount, int* pnKeyEntryCount, int* pnResourceBytes,
                             int* pnPackFileBytes, int* pnModelBytes, int* pnModelExtBytes,
                             int* pnTextureBytes, int* pnSoundBytes);
    int  GetIsStaticType(unsigned short resType);
    // The async loader thread's per-wakeup body, run under m_critSection.
    void AsyncLoad();
    void ResumeServicing();
    void SuspendServicing();

    // Scans m_freeResources reclaiming memory until 1MB is recovered or the list is exhausted,
    // returning whether anything was actually freed. Busy entries are skipped over, demanded ones
    // just lose their pending-free flag, and everything else is freed and removed from the list.
    int  FreeChunk();

    // Searches m_freeResources for pRes; on a hit, frees its file data (with the same offset
    // adjustment Malloc applies) and clears the demand-related flags, unless the resource now
    // reports itself busy. Returns whether pRes was found in the free list.
    bool Free(CRes* pRes);

    int  RemoveKeyTable(const CExoString& path, unsigned int sourceType);
    int  AddKeyTable(const CExoString& path, unsigned int sourceType, unsigned int arg);

    // The same by-name list scan as RemoveKeyTable, but on a match it rebuilds the table instead of
    // destroying it.
    bool UpdateKeyTable(const CExoString& path, unsigned int sourceType);

    unsigned short GetResTypeFromFile(const CExoString& path);
    int  RemoveFile(const CExoString& path, unsigned short resType);

    // Extracts the ResRef (basename, no extension, at most 16 characters) from a filename; on a
    // name with no dot, resRef is set empty rather than left untouched.
    void GetResRefFromFile(CResRef& resRef, const CExoString& path);

    int CreateDirectory2(const CExoString& path);

    // Same alias-resolve and empty-check shape as CreateDirectory2, but queries free disk space
    // instead of creating a directory.
    int GetFreeDiskSpace(const CExoString& path, uint64_ll_t* pFreeBytes);

    // Whole-file copy through two CExoFile objects and a heap staging buffer sized to the source
    // file. A zero dstResType means "same as source".
    bool FileCopy(const CExoString& srcPath, const CExoString& dstPath,
                  unsigned short srcResType, unsigned short dstResType);

    // Finds the CExoKeyTable owning pRes, routing on the source-type tag in the top bits of the
    // resource id and matching the low-bit probe slot against each candidate's routing tag, and
    // returns that table's entry count for the resource type, or -1 when no table owns it.
    int GetTableCount(CRes* pRes, int resType);

    // The same list-select and probe-slot scan as GetTableCount, but gives up rather than retrying
    // when the matched table is still busy.
    void* GetResOfType(unsigned short resType, CRes* pRes);

    // The same list-select and probe-slot scan as GetTableCount, returning the matched table.
    CExoKeyTable* GetTable(CRes* pRes);

    // The shared by-list key-table scan. A zero filterTag matches any non-busy table holding the
    // key; a nonzero one additionally requires the table's internal subtype to match (the
    // encapsulated-files subtype split).
    int GetKeyEntryFromTable(CExoLinkedList<CExoKeyTable>& list, const CResRef& resRef,
                               unsigned short resType, CExoKeyTable** ppTable,
                               CKeyTableEntry** ppEntry, unsigned int filterTag);

    // The same by-list walk as GetKeyEntryFromTable, collecting every non-busy table's keys of the
    // requested type and merging them into one string list; the first non-null result is kept as
    // the accumulator and every later one is merged into it and released.
    CExoStringList* GetResOfTypeFromTable(CExoLinkedList<CExoKeyTable>& list, unsigned short resType);

    // Merges GetResOfTypeFromTable across all four source lists, with the same
    // accumulate-and-release idiom, unless bEncapsulatedOnly is set: then it returns after the
    // encapsulated-files list alone.
    CExoStringList* GetResOfType(unsigned short resType, int bEncapsulatedOnly);

    // On the failed-open path the engine leaks the encapsulated file object it just allocated.
    CExoLocString GetEncapsulatedFileDescription(const CExoString& path);

    // Recursively deletes the loose files and subdirectories under `path` (a nonzero `a` also
    // deletes files whose extension cannot be determined) and optionally the directory itself
    // (nonzero `b`). c and d select between two recursion shapes for descending into
    // subdirectories.
    int  WipeDirectory(const CExoString& path, int a, int b, int c, int d);

    // One forwarder per source type, for the fixed key tables, the override directories, the
    // encapsulated archives and the resource-image packs.
    void RemoveFixedKeyTableFile(const CExoString& path);
    void RemoveResourceDirectory(const CExoString& path);
    int  RemoveEncapsulatedResourceFile(const CExoString& path);
    void RemoveResourceImageFile(const CExoString& path);
    void AddFixedKeyTableFile(const CExoString& path);
    void AddResourceDirectory(const CExoString& path);
    int  AddEncapsulatedResourceFile(const CExoString& path, unsigned int arg);
    void AddResourceImageFile(const CExoString& path);

    // Queues a resource-image-pack path for the background loader: under the critical section it
    // stashes the path and its two arguments, records that work is queued and resumes the thread.
    void AddResourceImageFilesAsync(const CExoString& path, int a, int b);

    // Independent per-field copies of UpdateKeyTable's scan, one per source list.
    bool UpdateFixedKeyTableFile(const CExoString& path);
    bool UpdateResourceDirectory(const CExoString& path);
    bool UpdateEncapsulatedResourceFile(const CExoString& path);
    bool UpdateResourceImageFile(const CExoString& path);

    // The two WipeDirectory forwarders, whose constant argument slots are genuinely ordered
    // differently from each other.
    void NukeDirectory(const CExoString& path, int a, int b);
    int  CleanDirectory(const CExoString& path, int a, int b);

    int  SpaceUsedBySaveGames();
    int  FindModuleDownloadedPkg(const CExoString& name);
    bool ValidateSignedResource(const CExoString& path, unsigned short resType,
                                 unsigned int a, unsigned int b);
    bool CreateSignatureFile(const CExoString& path, unsigned short resType,
                              unsigned int a, unsigned int b);

    // The four per-source-type servicing workers behind Demand(). Each returns nonzero on success;
    // ServiceFromImage never reads its second parameter.
    int ServiceFromResFile(CRes* pRes, int nOffset);
    int ServiceFromEncapsulated(CRes* pRes, int nOffset);
    int ServiceFromDirectory(CRes* pRes, int nOffset);
    int ServiceFromImage(CRes* pRes, int nOffset);

    // Finishes the in-flight async read parked at m_pAsyncReadFile, handing its buffer to the CRes it was
    // started for.
    int ServiceCurrentAsyncRes();

    // The workers behind the CRes demand, release and cancel methods, which each forward to
    // the matching one of these.
    void* Demand(CRes* pRes);
    int   Release(CRes* pRes);
    int   CancelRequest(CRes* pRes);

    int  ServiceFromResFileRaw(CRes* pRes, int a, char* pBuf);
    int  ServiceFromEncapsulatedRaw(CRes* pRes, int a, char* pBuf);
    int  ServiceFromDirectoryRaw(CRes* pRes, int a, char* pBuf);
    int  ServiceFromImageRaw(CRes* pRes, int a, char* pBuf);
    // Dispatches on the source-type tag in the top bits of the resource id: fixed key table,
    // resource image, encapsulated archive or resource directory.
    int  ReadRaw(CRes* pRes, int a, char* pBuf);

    void Dump(CRes* pRes, int a);
    // The per-frame demand-queue pump, servicing one queued resource per call off a
    // lazily-refreshed head cache. The time-slice parameter is never read.
    void Update(unsigned int nTimeSlice);
    unsigned int GetResID(const CResRef& resRef, unsigned short resType);
    int  Exists(const CResRef& resRef, unsigned short resType, unsigned int* pSourceType = 0);

    // This overload asks whether the resource exists specifically within the one list named by
    // sourceType, rather than in any list. Returns int rather than bool, so a caller's
    // whole-word test stays whole-word.
    int  Exists(const CResRef& resRef, unsigned short resType, unsigned int sourceType);

    // This overload out-params the owning table's filename instead of its numeric source type, and
    // sets the name empty when no table holds the resource.
    int Exists(const CResRef& resRef, unsigned short resType, CExoString& sFileName);

    // Attaches a resource to the key-table entry matching the given reference; a no-op when the
    // resource is null, no entry matches, or the entry already owns a resource.
    void SetResObject(const CResRef& resRef, unsigned short resType, CRes* pRes);

    // Finds the resource for a reference, logging the source on the first reference only. A
    // resource-image source skips logging outright, and the override-directory case re-scans the
    // image packs first so that an image-pack copy suppresses the log.
    CRes* GetResObject(const CResRef& resRef, unsigned short resType);

    // Applies CRes::Dump's logic to every resource cached in a table: demanded resources are just
    // flagged for later cleanup, everything else with loaded data is freed. The parameter is the
    // real CExoLinkedList<CExoKeyTable>&, which callers reinterpret_cast their internal list to.
    void DumpTable(CExoLinkedList<CExoKeyTable>& list);
    void DumpAll();

    void FreeResourceData(CRes* pRes);
    bool Malloc(CRes* pRes);
    bool Request(CRes* pRes);

    // Returns 0 for a null or unowned resource, otherwise the decremented holder count.
    int  ReleaseResObject(CRes* pRes);

    // Both are stubs that return an empty string and ignore every argument, including the
    // out-parameter of the second overload.
    CExoString CacheResource(const CExoString& path, unsigned short resType);
    CExoString CacheResource(const CExoString& path, unsigned short resType, int& out);

    int  m_nTotalPhysicalMemory;        // total physical memory, recorded once by the constructor
    int  m_nTotalResourceMemory;        // adjustable resource-cache budget
    int  m_nAvailableResourceMemory;    // remaining budget, adjusted on every allocation and free
    void* m_pReserved1;

    // Six by-value owning lists. Each eight-byte CExoLinkedList<T> wrapper allocates its backing
    // internal list on construction and deletes it on destruction.
    CExoLinkedList<CExoKeyTable> m_fixedKeyTables;      // base-game BIF/KEY tables
    CExoLinkedList<CExoKeyTable> m_resourceDirectories; // override loose-file directories
    CExoLinkedList<CExoKeyTable> m_encapsulatedFiles;   // ERF/RIM archives
    CExoLinkedList<CExoKeyTable> m_resourceImageFiles;  // resource-image packs
    CExoLinkedList<CRes>         m_freeResources;       // released-but-cached resources
    CExoLinkedList<CRes>         m_demandQueue;         // pending async loads

    void* m_pDemandQueueHeadCache;   // cached head of the demand queue
    void* m_pAsyncReadFile;                  // an active-async-read marker, unmodeled

    int  m_bSuspended;
    // Two "this menu's resource-image pack is currently mounted" latches, cleared with the matching
    // RemoveResourceImageFile call when the scene goes away. The remaining six bits are untouched.
    unsigned char bChargenImageFileLoaded : 1;
    unsigned char bMainMenuImageFileLoaded : 1;
    unsigned char m_nReserved1 : 6;

    unsigned long long m_hAsyncThread;
    unsigned int        m_dwAsyncThreadId;

    CExoCriticalSection m_critSection;
    char m_nAsyncLoadState;
    char _pad79[3];
    int  m_bShuttingDown;                 // set before the async thread is joined

    CExoString m_sAsyncModuleName;                  // image-pack path queued for the background loader
    int m_bAsyncLoadModule;                         // the queued request's second argument
    int m_bAsyncIsSaveGame;                         // the queued request's third argument
    char m_nModuleResourceFlags;                        // cleared when new work is queued
};
extern CExoResMan* g_pExoResMan;

#endif

#ifndef CRES_H
#define CRES_H

// The base-lib resource wrapper. It has no translation unit of its own, because its methods live in
// the resource manager's, which owns every resource's lifecycle; this header therefore stays
// dependency-light so any TU holding a CRes* (or deriving from it) can include it.
class CKeyTableEntry;
class CResRef;

// The res-helper-info block a CAurora* wrapper stamps into its resource's slack (CResHelper.h). It
// is unaligned by construction (two bytes, then an eight-byte pointer), hence packed (`#pragma pack`); the pointer
// field is still a single unaligned store.
#pragma pack(push, 1)
struct CResHelperInfo {
    unsigned short m_nResType;    // the wrapper's resource type
    void*          m_pResHelper;  // the wrapper's this
};
#pragma pack(pop)
static_assert(sizeof(CResHelperInfo) == 10, "the ten-byte info block");

#if KOTOR1
#include "alignedptr.h"
// K1's model, MDX, 4PC, DDS, PLT and font loaders stamp the block field by field through CAlignedPtr.
inline void SetResHelperInfo(CResHelperInfo* pInfo, unsigned short nResType, void* pResHelper)
{
    // A packed member's address may not be taken, so the two fields are addressed by offset.
    char* pBlock = reinterpret_cast<char*>(pInfo);
    // raw-offset: the packed block's first field
    CAlignedPtr<unsigned short>(reinterpret_cast<unsigned short*>(pBlock + __builtin_offsetof(CResHelperInfo, m_nResType)))
        .Set(nResType);
    // raw-offset: the packed block's unaligned pointer field
    CAlignedPtr<void*>(reinterpret_cast<void**>(pBlock + __builtin_offsetof(CResHelperInfo, m_pResHelper)))
        .Set(pResHelper);
}
#endif

class CRes {  // idiom-exempt: canonical
public:
    // m_dwFlags.
    enum : unsigned int {
        RESFLAG_RESIDENT = 0x4,        // serviced: m_pFileData holds the resource
        RESFLAG_LOADING = 0x10,        // an async read is filling m_pFileData
        RESFLAG_HAS_DATA = RESFLAG_RESIDENT | RESFLAG_LOADING,
        RESFLAG_FREE_PENDING = 0x100,  // on the manager's free list, or to be dumped once released
        RESFLAG_ORPHANED = 0x200,      // its key table went away while it was still demanded
    };

    CRes();
    // Leaves the holder count and the two allocation flags uninitialised, unlike the default
    // constructor.
    CRes(unsigned int dwResID);
    // <VTABLE-GEN: binary ground truth (_ZTV) — regenerated by tools/hoist_headers.py; do not edit inside markers */
    //   primary vtable, 6 slots
    virtual ~CRes();  // slots 0-1 (D2+D0)
    virtual int GetFixedResourceSize();
    virtual int GetFixedResourceDataOffset();
    virtual int OnResourceFreed();
    virtual int OnResourceServiced();
    // </VTABLE-GEN>
    // The two size hooks sit between the destructor and OnResourceFreed in the vtable, so both
    // must be declared here even as stubs, for OnResourceFreed to land on the slot the resource
    // manager's free path calls.

    // Non-virtual. If the resource is still demanded it is flagged for later cleanup; otherwise
    // its data is freed through the global resource manager. The parameter is unused.
    void Dump(int);

    // Demands the resource through the global manager and returns its file data, or null when it
    // cannot be serviced.
    void* Demand();
    // Releases the resource through the global manager. Returns int rather than void, because call
    // sites test the result.
    int Release();
    // Both return a value rather than void. Every path sets one, and the forwarders that call them
    // end their failure path with an explicit zero result. Nothing that calls them uses the value,
    // and its width (int or unsigned short) is not yet pinned, so int is the placeholder.
    int Request();       // bumps the request count and demands the resource
    int CancelRequest(); // the undo of Request

    // Reads nSize raw bytes of the resource into pBuffer, returning nonzero on success. Used by
    // the texture wrappers to peek a header without demanding the whole resource.
    int            ReadRaw(int nSize, char* pBuffer);
    // Dumps any currently-held file data, then overwrites the resource id. The store is
    // unconditional, so this faults on a null resource pointer just as the dump inside it would.
    void           SetID(unsigned int dwResID);
    // Copies the owning key entry's reference and type back out. Faults on a resource that no
    // entry owns, which the key-entry pointer is dereferenced for twice with no guard.
    void           GetResRef(CResRef& resRef, unsigned short& resType);
    void*          GetData();
    // Returns int, not unsigned short. The field is an unsigned short either way, but every
    // caller compares the result as signed.
    int            GetDemands();
    // The same accessor's sign is not pinned by any caller yet, so it stays unsigned short.
    unsigned short GetRequests();
    unsigned int   GetID();
    int            GetSize();     // signed: every widening caller sign-extends it

    unsigned short  m_nDemandCount;    // demand and release nesting count
    unsigned short  m_nRequestCount;   // request and cancel nesting count
    unsigned int    m_dwResID;         // the owning key entry's resource id; its top bits carry the
                                       // source-list tag and dispatch the per-source-type read
    unsigned int    m_dwFlags;         // RESFLAG_* bits, only partly reproduced
    void*           m_pFileData;       // raw loaded-resource buffer
    CKeyTableEntry* m_pKeyEntry;       // owning key-table entry, set by SetResObject
    unsigned int    m_nSize;           // allocated size of m_pFileData
    unsigned int    m_nHolderCount;    // external holder count, maintained with the key entry
    int             m_bAllocHeaderData;  // the data starts a CResHelperInfo into its real
                                       // allocation, which holds the block
    int             m_bAllocTrailingData;   // the allocation is oversized by fourteen bytes: a
                                       // trailing block and its four-byte gap (the non-offset
                                       // path only)
};                                     // sizeof == 0x38

#endif

// The engine's resource key, a fixed 16-byte, NUL-padded, lowercased filename stem naming one
// resource in the key table, BIF files or override tree. It is a plain 16-byte POD with no heap
// storage and no vtable, trivially copyable and trivially destructible, which is why it is passed
// and returned in registers rather than by hidden pointer.
class CExoString;

class CResRef {  // idiom-exempt: canonical (home is resgff.h, alongside CResGFF/CResStruct/CResList)
public:
    char m_sResRef[16];

    // --- Construction ---
    // The copy constructor is the defaulted memberwise one and there is no user-declared destructor:
    // CResRef is trivially copyable for calls and trivially destructible.
    CResRef();
    CResRef(const CResRef& other) = default;
    CResRef(char* s);
    CResRef(const char* s);
    CResRef(unsigned char* s);
    CResRef(unsigned char* s, unsigned char n);
    CResRef(const CExoString& s);

    // --- Assignment ---
    // All of these return CResRef by value rather than a reference, matching how a trivially
    // copyable 16-byte class is passed back.
    CResRef operator=(char* s);                 // copies up to the first NUL
    CResRef operator=(unsigned char* s);        // raw on-disk bytes, no NUL scan
    CResRef operator=(const CExoString& s);
    CResRef operator=(const CResRef& other);    // per-byte lowercasing; not the memberwise copy
    CResRef operator+=(const CExoString& s);

    // --- Comparison ---
    // The reference and literal comparisons take a `char*` rather than a `const char*`, matching the
    // declarations, so literal call sites spell the cast to `char*`.
    int  operator==(char* s) const;
    bool operator==(const CExoString& s) const;
    int  operator==(const CResRef& other) const;
    int  operator!=(char* s) const;
    int  operator!=(const CExoString& s) const;
    int  operator!=(const CResRef& other) const;
    int  operator!();

    // --- Accessors ---
    char* GetResRef() const;                    // the buffer is the first member, so this returns it
    void  GetResRef(unsigned char* out) const;
    // Copies the 16 bytes plus a terminator into the next slot of a global ring buffer and returns
    // it: a printable temporary, valid only until the ring wraps.
    char* GetResRefStr() const;
    int   IsValid() const;                      // whether the first character is nonzero
    unsigned char GetLength() const;            // the length up to the first NUL, 0..16
    void  CopyToString(char* out) const;
    void  CopyToString(CExoString& out) const;

    // Four slots of seventeen bytes each, indexed by a rotating counter; the defining translation
    // unit is the resource manager's.
    static int  s_nCurBufferIndex;
    static char RETURN_BUFFERS[4][17];
};
