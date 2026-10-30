#ifndef WINTHREADSMP_H
#define WINTHREADSMP_H
#include "k2stdint.h"

// The ASL2 port layer's Win32 thread and synchronisation primitives, a refcounted handle base, a
// thread handle, four waitable sync objects and the lazy registry that tracks them.

#include <map>
#include <set>
#include <list>
#include <atomic>
#include <pthread.h>

#include "port/win32api.h"
#if defined(K2_HOST_BUILD)
#include "port/libcstring.h"
#endif

class SyncPrimitive;
class ThreadHANDLE;

struct _SECURITY_ATTRIBUTES;
struct _OVERLAPPED;
struct _LARGE_INTEGER { long long QuadPart; };

// Defined and compiled in its own translation unit, so the thread handle's calls to it stay out of
// line.
#include "MSG_Mac.h"

// The refcounted handle base. The four virtuals are followed by the destructor, and that is the
// vtable slot order.
class HANDLE_Mac {
public:
    HANDLE_Mac();
    virtual unsigned int WaitForSingleObject(unsigned int dwMilliseconds);
    virtual unsigned int WaitForSingleObjectEx(unsigned int dwMilliseconds);
    virtual unsigned int Signal();
    virtual unsigned int CloseHandle();
    virtual ~HANDLE_Mac();

    // Returns the new count. Not virtual, so it does not affect the slot order.
    unsigned int AddRef();

    int m_nRefCount;
};

// A recursive critical section that spins on a try-lock, then takes a blocking lock. Enter is
// defined in the header because every constructor uses it.
class ASLCriticalSection {
public:
    ASLCriticalSection();
    ~ASLCriticalSection();

    void Enter() {
        int n = m_nSpinCount;
        while (n != 0 && pthread_mutex_trylock(&m_mutex) != 0)
            n--;
        if (n == 0)
            pthread_mutex_lock(&m_mutex);
    }
    void Leave() { pthread_mutex_unlock(&m_mutex); }

    pthread_mutex_t m_mutex;
    int             m_nSpinCount;
};

// Scoped enter/leave. The source spells the Enter and Leave pairs out at each site instead of using
// this type.
class ASLScopeLock {
    ASLCriticalSection* m_pLock;
public:
    explicit ASLScopeLock(ASLCriticalSection& lock) : m_pLock(&lock) { m_pLock->Enter(); }
    ~ASLScopeLock() { m_pLock->Leave(); }
};

// The process-wide thread and sync-object registry, created on first use.
class ASLThreadRegistry {
public:
    ASLThreadRegistry();

    std::map<int64_t, ThreadHANDLE*>           m_threadsByHandle;
    std::map<unsigned int, ThreadHANDLE*>   m_threadsById;
    ASLCriticalSection                      m_lock;
    std::set<SyncPrimitive*>                m_syncPrimitives;
    ASLCriticalSection                      m_syncLock;
};

ASLThreadRegistry& GetThreadRegistry();

// A pending overlapped I/O record; all three of its bodies are trivial in this build.
class MacPendingIO {
public:
    MacPendingIO(void* pv, _OVERLAPPED* pOverlapped, void (*pfn)(unsigned int, unsigned int, _OVERLAPPED*));
    ~MacPendingIO();
    static void OverlappedCompletionProc(void* pArg);

    void* m_pReserved1;
    void* m_pReserved2;
    void* m_pThread;
    void* m_pReserved3;
    struct CTail { void* m_pReserved1; void* m_pInternalHigh; }* m_pTail;
    void* m_pReserved4;
};

// A thread, which overrides only WaitForSingleObject and registers in the registry's two maps
// rather than in its sync-object set.
class ThreadHANDLE : public HANDLE_Mac {
public:
    ThreadHANDLE(unsigned int (*pfn)(void*), void* pArg, unsigned int nStackSize, bool bOne, bool bTwo);
    virtual ~ThreadHANDLE();

    virtual unsigned int WaitForSingleObject(unsigned int dwMilliseconds);

    static unsigned int ThreadProc(void* pArg);

    void AddPendingIO(MacPendingIO* pPendingIO);
    void SignalCompletedIO(MacPendingIO* pPendingIO);
    unsigned int WaitForPendingIO(unsigned int dwMilliseconds);
    void EnableRunnableState();
    void DisableRunnableState();

    unsigned int m_nId;
    pthread_t   m_hThread;
    bool        m_bCreateSuspended;
    bool        m_bRunnable;
    pthread_cond_t  m_cond;
    pthread_mutex_t m_mutex;
    unsigned int (*m_pFunc)(void*);
    void*       m_pArg;
    unsigned int m_nExitCode;
    void*       m_pPendingIOTail;
    void*       m_hPendingIOSem;
    CWin32MSGQueue m_msgQueue;
    bool        m_bUseEndThread;
};

// The shared base of the four waitable objects. It declares no virtuals of its own.
class SyncPrimitive : public HANDLE_Mac {
public:
    SyncPrimitive()
    {
        m_szName[0] = 0;
        m_nType = 0;
    }

    K2WChar m_szName[260];
    int     m_nType;
};

// An event, which overrides all three wait and signal virtuals and owns the real condition-variable
// logic of this file.
class EventHANDLE : public SyncPrimitive {
public:
    EventHANDLE(_SECURITY_ATTRIBUTES* pSec, int bManualReset, int bInitialState, K2WChar const* pszName);
    virtual ~EventHANDLE();

    virtual unsigned int WaitForSingleObject(unsigned int dwMilliseconds);
    virtual unsigned int WaitForSingleObjectEx(unsigned int dwMilliseconds);
    virtual unsigned int Signal();

    unsigned int SetEvent();
    unsigned int ResetEvent();
    unsigned int PulseEvent();

    bool m_bSignaled;
    bool m_bManualReset;
    std::list<unsigned int> m_waiters;
    pthread_cond_t  m_cond;
    pthread_mutex_t m_mutex;
    int m_nSignalCount;
    volatile int m_nWaiterCount;
};

// A semaphore, which overrides the three wait and signal virtuals, all near-stubs in this build.
class SemaphoreHANDLE : public SyncPrimitive {
public:
    SemaphoreHANDLE(int nInitial, int nMax, K2WChar const* pszName);
    virtual ~SemaphoreHANDLE();

    virtual unsigned int WaitForSingleObject(unsigned int dwMilliseconds);
    virtual unsigned int WaitForSingleObjectEx(unsigned int dwMilliseconds);
    virtual unsigned int Signal();

    unsigned int Release();
};

// A mutex that tracks its owner by thread identity.
class MutexHANDLE : public SyncPrimitive {
public:
    MutexHANDLE(bool bInitialOwner, K2WChar const* pszName);
    virtual ~MutexHANDLE();

    virtual unsigned int WaitForSingleObject(unsigned int dwMilliseconds);
    virtual unsigned int Signal();

    unsigned int Release();

    void* m_pReserved1;
    pthread_t m_hOwner;
    int       m_nRecursionCount;
    std::list<unsigned int> m_waiters;
};

// A waitable timer, which overrides only WaitForSingleObject.
class WaitableTimerHANDLE : public SyncPrimitive {
public:
    WaitableTimerHANDLE(_SECURITY_ATTRIBUTES* pSec, int bManualReset, K2WChar const* pszName);
    virtual ~WaitableTimerHANDLE();

    virtual unsigned int WaitForSingleObject(unsigned int dwMilliseconds);

    unsigned int Set(const _LARGE_INTEGER* pDueTime, int nPeriod,
                     void (*pfn)(void*, unsigned int, unsigned int), void* pArg, int bResume);
};

#ifndef K2_HOST_BUILD   // bionic's sizes; a host libc has its own
static_assert(sizeof(pthread_mutex_t) == 0x28, "bionic aarch64 pthread_mutex_t is 0x28");
static_assert(sizeof(pthread_cond_t) == 0x30, "bionic aarch64 pthread_cond_t is 0x30");
#endif

#endif // WINTHREADSMP_H
