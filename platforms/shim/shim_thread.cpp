// Win32 threads, handles and critical sections. Threads are the engine's own ThreadHANDLE
// (src/game/WinThreadsMP), built the way Aspyr's CreateThread builds them; this file supplies the
// out-of-scope pieces around it: HANDLE_Mac (the refcounted base), the auto-TLS block, and the
// Win32 entry points that dispatch to the handle's virtuals.
#include <stdint.h>
#include "k2shim.h"

#include "WinThreadsMP.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mutex>

HANDLE_Mac::HANDLE_Mac() : m_nRefCount(1) {}
HANDLE_Mac::~HANDLE_Mac() {}
unsigned int HANDLE_Mac::WaitForSingleObject(unsigned int) { return 0; }
unsigned int HANDLE_Mac::WaitForSingleObjectEx(unsigned int) { return 0; }
unsigned int HANDLE_Mac::Signal() { return 0; }
unsigned int HANDLE_Mac::AddRef() { return static_cast<unsigned int>(++m_nRefCount); }

unsigned int HANDLE_Mac::CloseHandle()
{
    if (--m_nRefCount == 0) delete this;
    return 1;
}

// ASL auto-TLS: per-thread storage addressed by byte offset, where ASLAutoTLSGet(n) is
// `TlsGetValue(idx) + n`. The two offsets ThreadProc publishes are never written anywhere in the
// engine, so both are 0 and the handle pointer overwrites the id, as the engine itself does.

namespace {
struct AutoTls { unsigned char bytes[256]; };
thread_local AutoTls* t_pAutoTls;
}

void ASLInitAutoTLSStorageForCurrentThread()
{
    if (!t_pAutoTls) t_pAutoTls = static_cast<AutoTls*>(calloc(1, sizeof(AutoTls)));
}

void* ASLAutoTLSGet(unsigned int nSlot)
{
    ASLInitAutoTLSStorageForCurrentThread();
    return nSlot < sizeof(AutoTls) ? t_pAutoTls->bytes + nSlot : 0;
}

namespace ASL {

// Null, deliberately: the Windows message path reaches the thread's message queue at a fixed offset
// from the thread handle, and the host ThreadHANDLE is laid out differently. The shim's own queue
// replaces that path.
struct ASLThread;   // MSG_Mac.cpp's view of the thread object
ASLThread* GetCurrentThreadHANDLE() { return 0; }

void OnCreateThread(const char* pszName)
{
#if defined(__APPLE__)
    pthread_setname_np(pszName);
#elif defined(__linux__)
    pthread_setname_np(pthread_self(), pszName);
#endif
}

} // namespace ASL

// Win32 entry points.

namespace {

bool IsHandle(void* h) { return h != 0 && h != reinterpret_cast<void*>(-1); }

// ThreadHANDLE::WaitForSingleObject spins until m_hThread is 0; the thread's own exit clears it.
[[noreturn]] void EndCurrentThread(unsigned int nCode)
{
    // ThreadProc stored the handle in the auto-TLS block (offset 0; see above)
    if (ThreadHANDLE* p = t_pAutoTls ? *reinterpret_cast<ThreadHANDLE**>(t_pAutoTls->bytes) : 0) p->m_hThread = 0;  // raw-offset: the auto-TLS block holds the handle at offset 0 by contract
    pthread_exit(reinterpret_cast<void*>(static_cast<uintptr_t>(nCode)));
    __builtin_unreachable();   // winpthreads and compat/include-msvc declare pthread_exit without noreturn
}

} // namespace

K2_WIN32_API_BEGIN

void* CreateThread(void* /*pSecAttr*/, uint64_t nStackSize, uint64_t (*pfnStart)(void*), void* pParam,
                   uint64_t dwFlags, unsigned int* pThreadId)
{
    // CREATE_SUSPENDED (4) is ThreadHANDLE's runnable gate. The engine's thread procedures return a
    // 64-bit DWORD_PTR and ThreadHANDLE keeps the low 32 bits.
    ThreadHANDLE* p = new ThreadHANDLE(reinterpret_cast<unsigned int (*)(void*)>(pfnStart), pParam,
                                       static_cast<unsigned int>(nStackSize), false, (dwFlags & 4) != 0);
    if (pThreadId) *pThreadId = p->m_hThread ? p->m_nId : 0;
    return p->m_hThread ? p : 0;
}

// Aspyr's suspend is cooperative: the engine only ever suspends the calling thread (the file
// worker parks itself), and resume releases it.
uint64_t SuspendThread(void* hThread)
{
    if (!IsHandle(hThread)) return static_cast<uint64_t>(-1);
    static_cast<ThreadHANDLE*>(hThread)->DisableRunnableState();
    return 0;
}

uint64_t ResumeThread(void* hThread)
{
    if (!IsHandle(hThread)) return static_cast<uint64_t>(-1);
    ThreadHANDLE* p = static_cast<ThreadHANDLE*>(hThread);
    if (p->m_bCreateSuspended) p->EnableRunnableState();   // only suspend-capable threads have the pair
    return 1;
}

int SetThreadPriority(void*, int) { return 1; }

void _endthreadex(unsigned int nReturnCode) throw() { EndCurrentThread(nReturnCode); }

unsigned int WaitForSingleObject(void* hHandle, unsigned int dwMilliseconds)
{
    if (!IsHandle(hHandle)) return 0xffffffffu;   // WAIT_FAILED
    return static_cast<HANDLE_Mac*>(hHandle)->WaitForSingleObject(dwMilliseconds);
}

int CloseHandle(void* hObject)
{
    if (!IsHandle(hObject)) return 0;
    return static_cast<int>(static_cast<HANDLE_Mac*>(hObject)->CloseHandle());
}

void* CreateSemaphoreA(void* /*pSec*/, int nInitial, int nMax, void* /*pName*/) throw()
{
    return new SemaphoreHANDLE(nInitial, nMax, 0);
}

// CRITICAL_SECTION: the engine gives us 40 bytes (CExoCriticalSectionInternal), too small for a
// host pthread_mutex_t, so the first word holds a heap mutex.
void InitializeCriticalSection(void* pCs)
{
    memset(pCs, 0, 0x28);
    *static_cast<std::recursive_mutex**>(pCs) = new std::recursive_mutex;
}
void DeleteCriticalSection(void* pCs)
{
    delete *static_cast<std::recursive_mutex**>(pCs);
    *static_cast<std::recursive_mutex**>(pCs) = 0;
}
void EnterCriticalSection(void* pCs) { (*static_cast<std::recursive_mutex**>(pCs))->lock(); }
void LeaveCriticalSection(void* pCs) { (*static_cast<std::recursive_mutex**>(pCs))->unlock(); }

int SetPriorityClass(void*, unsigned int) { return 1; }
void* GetCurrentProcess_Win32() { return reinterpret_cast<void*>(-1); }

void ExitThread(unsigned int nExitCode) { EndCurrentThread(nExitCode); }
void _endthread() throw() { EndCurrentThread(0); }

// The SList pair is a no-op stub in the engine too.
int InitializeSListHead(void*) throw() { return 0; }
void* InterlockedPopEntrySList(void*) throw() { return 0; }

K2_WIN32_API_END
