// Runtime coverage recorder for the host builds (`K2P_COV=ON`, the root CMakeLists.txt).
//
// The engine library is built with `-finstrument-functions`, so
// `__cyg_profile_func_enter(this_fn, call_site)` runs on every entry. this_fn is the entry point's
// address, so no guard numbering, no per-module init callback and no section-symbol arithmetic is
// involved: a plain address hash.
//
// The counters record what the game actually executes.
#include <dlfcn.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" void k2cov_reset(void);
extern "C" void k2cov_dump(const char* szPath);
extern "C" void __cyg_profile_func_enter(void* pFn, void* pCaller);
extern "C" void __cyg_profile_func_exit(void* pFn, void* pCaller);

namespace {

const uint32_t kSlots = 1u << 19;      // 524,288 slots: the engine is ~14k functions
const uint32_t kProbes = 8;            // linear probes before a key is counted as a miss

struct Entry {
    uintptr_t nAddr;                   // function address (runtime), 0 = empty
    uint32_t uCount;
};

Entry g_aTable[kSlots];
uint32_t g_nFuncs;
uint32_t g_nMisses;
uintptr_t g_nImageBase;

uintptr_t ImageBase(void)
{
    if (!g_nImageBase) {
        Dl_info info;
        if (dladdr(reinterpret_cast<void*>(&__cyg_profile_func_enter), &info) && info.dli_fbase)
            g_nImageBase = reinterpret_cast<uintptr_t>(info.dli_fbase);
    }
    return g_nImageBase;
}

void AtExit(void)
{
    const char* szPath = getenv("K2_COV");
    if (szPath && *szPath)
        k2cov_dump(szPath);
}

struct AtExitReg {
    AtExitReg(void) { atexit(AtExit); }
} g_atExit;

}  // namespace

extern "C" void __cyg_profile_func_enter(void* pFn, void* pCaller)
{
    (void)pCaller;
    uintptr_t nKey = reinterpret_cast<uintptr_t>(pFn);
    if (!nKey)
        return;
    uint32_t uSlot = static_cast<uint32_t>((nKey >> 2) * 2654435761u) & (kSlots - 1);
    for (uint32_t i = 0; i < kProbes; i++) {
        Entry& e = g_aTable[uSlot];
        if (e.nAddr == nKey) {
            e.uCount++;
            return;
        }
        if (!e.nAddr) {
            e.nAddr = nKey;
            e.uCount = 1;
            g_nFuncs++;
            return;
        }
        uSlot = (uSlot + 1) & (kSlots - 1);
    }
    g_nMisses++;
}

// An exit call is emitted too; only the entry counts are recorded.
extern "C" void __cyg_profile_func_exit(void* pFn, void* pCaller)
{
    (void)pFn;
    (void)pCaller;
}

// The dump is ASLR-independent. The `probe` line carries this symbol's own runtime address, so a
// recorded address turns back into a link-time one (runtime minus what `nm` reports for the probe
// symbol) without any platform-specific slide API.
extern "C" void k2cov_dump(const char* szPath)
{
    FILE* f = fopen(szPath, "w");
    if (!f) {
        fprintf(stderr, "k2cov: cannot write %s\n", szPath);
        return;
    }
    fprintf(f, "k2cov 1\n");
    fprintf(f, "image %#" PRIx64 "\n", static_cast<uint64_t>(ImageBase()));
    fprintf(f, "probe %#" PRIx64 "\n", static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&k2cov_reset)));
    fprintf(f, "funcs %u\n", g_nFuncs);
    if (g_nMisses)
        fprintf(f, "misses %u\n", g_nMisses);
    for (uint32_t uSlot = 0; uSlot < kSlots; uSlot++) {
        if (!g_aTable[uSlot].nAddr)
            continue;
        fprintf(f, "%#" PRIx64 " %u\n", static_cast<uint64_t>(g_aTable[uSlot].nAddr), g_aTable[uSlot].uCount);
    }
    fclose(f);
    fprintf(stderr, "k2cov: %u functions hot -> %s\n", g_nFuncs, szPath);
}

extern "C" void k2cov_reset(void)
{
    for (uint32_t uSlot = 0; uSlot < kSlots; uSlot++)
        g_aTable[uSlot].uCount = 0;
}
