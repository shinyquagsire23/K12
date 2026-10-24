// An unhandled exception (an access violation, an illegal instruction ...) prints its code, the
// faulting address and the call stack to stderr before the process ends, as the macOS crash
// reporter would: each frame as module+offset, which
//   llvm-symbolizer --obj=k2.exe 0x<0x140000000 + offset>
// turns into a function and line (the executable carries DWARF; 0x140000000 is its link address).
// Compiled into the executable itself, not k2_platform: nothing references it, so an archive member
// would be dropped. K2_CRASH_HANDLER=0 leaves the exception to the system (or a debugger).
#include "build_id.h"
#include <windows.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void k2shim_glarr_dump(uintptr_t fault);   // shim/shim_glarrays.cpp: the last draws, the enabled arrays

extern "C" void k2host_log_flush(void);   // win_console.cpp

namespace {

void PrintFrame(int n, DWORD64 pc)   // n < 0: the rest of a line already begun
{
    HMODULE h = 0;
    char szModule[MAX_PATH] = "?";
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(pc), &h)) {
        GetModuleFileNameA(h, szModule, sizeof szModule);
        const char* pszBase = strrchr(szModule, '\\');
        if (n >= 0) fprintf(stderr, "k2:   #%-2d", n);
        fprintf(stderr, " %s+0x%llx\n", pszBase ? pszBase + 1 : szModule, static_cast<unsigned long long>(pc - reinterpret_cast<DWORD64>(h)));
    } else {
        if (n >= 0) fprintf(stderr, "k2:   #%-2d", n);
        fprintf(stderr, " 0x%llx\n", static_cast<unsigned long long>(pc));
    }
}

// Whether pc follows a call instruction: a stack word that does is a return address, one that does
// not is a spilled pointer or a stale value (most of what a raw stack scan finds).
bool AfterCall(DWORD64 pc)
{
    const unsigned char* p = reinterpret_cast<const unsigned char*>(pc);
    if (pc < 0x10000 || IsBadReadPtr(p - 7, 7)) return false;
    if (p[-5] == 0xE8) return true;                                       // call rel32
    if (p[-2] == 0xFF && (p[-1] & 0xF8) == 0xD0) return true;             // call reg
    if (p[-3] == 0xFF && (p[-2] & 0xF8) == 0x50) return true;             // call [reg+disp8]
    if (p[-3] == 0x41 && p[-2] == 0xFF && (p[-1] & 0xF8) == 0xD0) return true;   // call r8-r15
    if (p[-6] == 0xFF && (p[-5] == 0x15 || (p[-5] & 0xF8) == 0x90)) return true; // call [rip/reg+disp32]
    if (p[-4] == 0xFF && p[-3] == 0x54) return true;                      // call [reg+reg+disp8]
    if (p[-7] == 0xFF && p[-6] == 0x94) return true;                      // call [reg+reg+disp32]
    if (p[-2] == 0xFF && (p[-1] & 0xF8) == 0x10) return true;             // call [reg]
    return false;
}

bool InModule(DWORD64 pc)
{
    HMODULE h = 0;
    return GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              reinterpret_cast<LPCSTR>(pc), &h) != 0;
}

void Unwind(CONTEXT ctx, int n)
{
    for (; n < 64 && ctx.Rip; ++n) {
        PrintFrame(n, ctx.Rip);
        DWORD64 nImageBase = 0;
        PRUNTIME_FUNCTION pFn = RtlLookupFunctionEntry(ctx.Rip, &nImageBase, 0);
        if (!pFn) {   // a leaf function: the return address is on top of the stack
            if (IsBadReadPtr(reinterpret_cast<void*>(ctx.Rsp), 8)) break;
            ctx.Rip = *reinterpret_cast<DWORD64*>(ctx.Rsp);
            ctx.Rsp += 8;
            continue;
        }
        void* pHandlerData = 0;
        DWORD64 nFrame = 0;
        RtlVirtualUnwind(UNW_FLAG_NHANDLER, nImageBase, ctx.Rip, pFn, &ctx, &pHandlerData, &nFrame, 0);
    }
}

LONG WINAPI OnUnhandled(EXCEPTION_POINTERS* pInfo)
{
    const EXCEPTION_RECORD* r = pInfo->ExceptionRecord;
    fprintf(stderr, "k2: build %s: unhandled exception 0x%08lx at %p", K2P_BUILD_ID, r->ExceptionCode,
            r->ExceptionAddress);
    uintptr_t nFault = 0;
    if (r->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && r->NumberParameters >= 2) {
        nFault = static_cast<uintptr_t>(r->ExceptionInformation[1]);
        fprintf(stderr, " (%s 0x%llx)", r->ExceptionInformation[0] == 8 ? "execute" : r->ExceptionInformation[0] ? "write" : "read",
                static_cast<unsigned long long>(nFault));
    }
    fprintf(stderr, "\n");
    CONTEXT ctx = *pInfo->ContextRecord;
    fprintf(stderr, "k2:   rip %llx rsp %llx rbp %llx rax %llx rcx %llx rdx %llx r8 %llx r9 %llx\n",
            ctx.Rip, ctx.Rsp, ctx.Rbp, ctx.Rax, ctx.Rcx, ctx.Rdx, ctx.R8, ctx.R9);
    fprintf(stderr, "k2:   rbx %llx rsi %llx rdi %llx r10 %llx r11 %llx r12 %llx r13 %llx r14 %llx r15 %llx\n",
            ctx.Rbx, ctx.Rsi, ctx.Rdi, ctx.R10, ctx.R11, ctx.R12, ctx.R13, ctx.R14, ctx.R15);
    bool bInModule = InModule(ctx.Rip);
    if (!bInModule)
        fprintf(stderr, "k2:   the pc is in no module: a driver's generated code, or a call through a stale pointer\n");
    // Return addresses (stack words that follow a call, in any module) up to the stack's base: when
    // the pc has no unwind data (generated code, a stale pointer), these name the callers.
    NT_TIB* pTib = reinterpret_cast<NT_TIB*>(NtCurrentTeb());
    DWORD64* pEnd = static_cast<DWORD64*>(pTib->StackBase);
    int iFirst = -1;
    DWORD64 nFirst = 0;
    for (int i = 0, nShown = 0; nShown < 24; ++i) {
        DWORD64* p = reinterpret_cast<DWORD64*>(ctx.Rsp) + i;
        if (p >= pEnd || IsBadReadPtr(p, 8)) break;
        if (InModule(*p) && AfterCall(*p)) {
            fprintf(stderr, "k2:   stack[%d]", i);
            PrintFrame(-1, *p);
            if (iFirst < 0) { iFirst = i; nFirst = *p; }
            ++nShown;
        }
    }
    k2shim_glarr_dump(nFault);
    if (bInModule) {
        Unwind(ctx, 0);
    } else if (iFirst >= 0) {   // unwind from the first return address the scan found
        fprintf(stderr, "k2:   unwound from stack[%d]:\n", iFirst);
        ctx.Rip = nFirst;
        ctx.Rsp += 8 * static_cast<DWORD64>(iFirst + 1);
        Unwind(ctx, 1);
    }
    fflush(stderr);
    k2host_log_flush();
    return EXCEPTION_CONTINUE_SEARCH;
}

// A stack overflow, or a fast-fail, as it happens: the first leaves too little stack for the
// unhandled filter, the second never reaches it.
LONG WINAPI OnFirstChance(EXCEPTION_POINTERS* pInfo)
{
    DWORD c = pInfo->ExceptionRecord->ExceptionCode;
    if (c != EXCEPTION_STACK_OVERFLOW && c != 0xC0000409 /* STATUS_STACK_BUFFER_OVERRUN */)
        return EXCEPTION_CONTINUE_SEARCH;
    fprintf(stderr, "k2: exception 0x%08lx at %p\n", c, pInfo->ExceptionRecord->ExceptionAddress);
    PrintFrame(0, pInfo->ContextRecord->Rip);
    fflush(stderr);
    k2host_log_flush();
    return EXCEPTION_CONTINUE_SEARCH;
}

struct Install {
    Install()
    {
        const char* e = getenv("K2_CRASH_HANDLER");
        if (e && *e == '0') return;
        SetUnhandledExceptionFilter(OnUnhandled);
        AddVectoredExceptionHandler(1, OnFirstChance);
    }
} s_install;

} // namespace

// A failed stack canary (a build with -fstack-protector-strong in CMAKE_C(XX)_FLAGS): dumps the call
// stack, whose #1 is the frame that got overwritten. mingw-w64's prints a line and exits.
extern "C" __declspec(noreturn) void __stack_chk_fail(void)
{
    void* apFrames[48];
    USHORT n = RtlCaptureStackBackTrace(0, 48, apFrames, 0);
    fprintf(stderr, "k2: stack smashing detected\n");
    for (USHORT i = 0; i < n; ++i) PrintFrame(i, reinterpret_cast<DWORD64>(apFrames[i]));
    fflush(stderr);
    k2host_log_flush();
    abort();
}
