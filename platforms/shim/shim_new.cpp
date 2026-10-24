// Global operator new/new[] replacement: the engine gets zeroed memory, as the Android heap hands
// out in practice. Its constructors leave fields unwritten and the code reads them anyway, and the
// macOS allocator recycles dirty blocks, so those reads decide what gets saved and how a run plays
// out. The zeroing is unconditional: the engine depends on it.
#include <new>
#include <stdlib.h>
#ifdef __APPLE__
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

bool k2shim_glarr_watching(const void* p);
void k2shim_glarr_freed(const void* p, size_t n);

#ifdef _WIN32
#include <windows.h>
// K2_GUARD_HEAP=1 (diagnosis): every block ends against a no-access page, so an overrun faults where
// it happens. Memory-hungry; delete only decommits.
static bool GuardHeap()
{
    static int s = -1;
    if (s < 0) { const char* e = getenv("K2_GUARD_HEAP"); s = e && *e == '1'; }
    return s == 1;
}
static void* GuardAlloc(size_t n)
{
    size_t body = (n + 15) & ~static_cast<size_t>(15), pages = (body + 0xfff) / 0x1000;
    char* base = static_cast<char*>(VirtualAlloc(0, (pages + 1) * 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    if (!base) abort();
    DWORD old;
    VirtualProtect(base + pages * 0x1000, 0x1000, PAGE_NOACCESS, &old);
    return base + pages * 0x1000 - body;
}
#endif

static void* Alloc(size_t n)
{
    if (n == 0) n = 1;
#ifdef _WIN32
    if (GuardHeap()) return GuardAlloc(n);
#endif
    void* p = calloc(1, n);
    if (!p) abort();
    return p;
}

// Returns the block's size for the client-array shadow (shim_glarrays.cpp), which disables an
// enabled GL array still pointing into it before the next draw.
static size_t BlockSize(void* p)
{
#ifdef _WIN32
    if (GuardHeap()) {
        MEMORY_BASIC_INFORMATION mbi;
        VirtualQuery(p, &mbi, sizeof mbi);
        return static_cast<size_t>(static_cast<char*>(mbi.BaseAddress) + mbi.RegionSize - static_cast<char*>(p));  // raw-offset: address arithmetic over a memory region
    }
    return _msize(p);
#elif defined(__APPLE__)
    return malloc_size(p);
#else
    return malloc_usable_size(p);
#endif
}

static void Free(void* p)
{
    if (p && k2shim_glarr_watching(p))
        k2shim_glarr_freed(p, BlockSize(p));
#ifdef _WIN32
    if (GuardHeap()) {   // decommit the block's pages (an access after delete faults too)
        if (p) {
            MEMORY_BASIC_INFORMATION mbi;
            VirtualQuery(p, &mbi, sizeof mbi);
            VirtualFree(mbi.AllocationBase, 0, MEM_RELEASE);
        }
        return;
    }
#endif
    free(p);
}

void* operator new(size_t n) { return Alloc(n); }
void* operator new[](size_t n) { return Alloc(n); }
void* operator new(size_t n, const std::nothrow_t&) noexcept { return Alloc(n); }
void* operator new[](size_t n, const std::nothrow_t&) noexcept { return Alloc(n); }
void operator delete(void* p) noexcept { Free(p); }
void operator delete[](void* p) noexcept { Free(p); }
void operator delete(void* p, size_t) noexcept { Free(p); }
void operator delete[](void* p, size_t) noexcept { Free(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { Free(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { Free(p); }
