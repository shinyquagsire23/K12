#include "k2stdint.h"
#include "WinThreadsMP.h"

#include "port/win32const.h"
#include <atomic>
#include <errno.h>
#include <new>
#ifdef K2_HOST_BUILD
#include <sys/time.h>
#endif

// The smallest stack a thread is created with.
enum {
    MIN_THREAD_STACK_SIZE = 512 * 1024,
};


static std::atomic<int> s_nNextThreadId;
__attribute__((visibility("hidden"))) unsigned int s_nTlsIdSlot;
__attribute__((visibility("hidden"))) unsigned int s_nTlsHandleSlot;

namespace ASL {
    void OnCreateThread(const char* pszName);
}
void  ASLInitAutoTLSStorageForCurrentThread();
void* ASLAutoTLSGet(unsigned int nSlot);


ASLCriticalSection::ASLCriticalSection()
{
    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr) == 0) {
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m_mutex, &attr);
        pthread_mutexattr_destroy(&attr);
    }
    m_nSpinCount = 150;
}

ASLCriticalSection::~ASLCriticalSection()
{
    pthread_mutex_destroy(&m_mutex);
}

ASLThreadRegistry::ASLThreadRegistry()
{
}

ASLThreadRegistry& GetThreadRegistry()
{
    static ASLThreadRegistry* s_pRegistry = new ASLThreadRegistry();
    return *s_pRegistry;
}


// bTwo both starts the thread suspended and gates the mutex/cond pair; bOne is stored and never
// read.
ThreadHANDLE::ThreadHANDLE(unsigned int (*pfn)(void*), void* pArg, unsigned int nStackSize,
                           bool bOne, bool bTwo)
{
    m_nId = ++s_nNextThreadId;
    m_hThread = 0;
    m_bUseEndThread = bOne;
    m_pFunc = pfn;
    m_pArg = pArg;
    m_nExitCode = STILL_ACTIVE;
    m_pPendingIOTail = 0;
    m_hPendingIOSem = reinterpret_cast<void*>(-1);
    m_bRunnable = !bTwo;
    m_bCreateSuspended = bTwo;

    if (pfn != 0) {
        if (bTwo) {
            pthread_mutexattr_t attr;
            if (pthread_mutexattr_init(&attr) == 0) {
                if (pthread_mutex_init(&m_mutex, &attr) == 0) {
                    if (pthread_cond_init(&m_cond, 0) == 0)
                        pthread_mutexattr_destroy(&attr);
                }
            }
        }

        pthread_t h = 0;
        pthread_attr_t attr;
        if (pthread_attr_init(&attr) == 0) {
            if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) == 0) {
                pthread_attr_setstacksize(&attr, nStackSize > MIN_THREAD_STACK_SIZE ? static_cast<size_t>(nStackSize) : MIN_THREAD_STACK_SIZE);
                pthread_create(&h, &attr, reinterpret_cast<void* (*)(void*)>(reinterpret_cast<void (*)()>(ThreadProc)), this);  // pthread's exit value is never read
                m_hThread = h;
                pthread_attr_destroy(&attr);
            }
        }
    } else {
        m_hThread = pthread_self();
    }

    GetThreadRegistry().m_lock.Enter();
    GetThreadRegistry().m_threadsByHandle[(int64_t)m_hThread] = this;  // idiom-exempt: pthread_t is an integer on Android and a pointer on macOS; no one named cast converts both
    GetThreadRegistry().m_threadsById[m_nId] = this;
    GetThreadRegistry().m_lock.Leave();
}

unsigned int ThreadHANDLE::ThreadProc(void* pArg)
{
    ThreadHANDLE* pThis = static_cast<ThreadHANDLE*>(pArg);

    ASLInitAutoTLSStorageForCurrentThread();
    // Each published value is followed by a read of the same slot whose result is discarded.
    *static_cast<unsigned int*>(ASLAutoTLSGet(s_nTlsIdSlot)) = pThis->m_nId;
    (void)ASLAutoTLSGet(s_nTlsIdSlot);
    *static_cast<ThreadHANDLE**>(ASLAutoTLSGet(s_nTlsHandleSlot)) = pThis;
    (void)ASLAutoTLSGet(s_nTlsHandleSlot);

    ASL::OnCreateThread(pThis->m_nId > 1 ? "Win" : "WinMain");

    if (pThis->m_bCreateSuspended) {
        pthread_mutex_lock(&pThis->m_mutex);
        while (!pThis->m_bRunnable)
            pthread_cond_wait(&pThis->m_cond, &pThis->m_mutex);
        pthread_mutex_unlock(&pThis->m_mutex);
    }

    unsigned int nResult = pThis->m_pFunc(pThis->m_pArg);
    pThis->m_nExitCode = nResult;

    if (pThis->m_nId >= 2) {
        GetThreadRegistry().m_lock.Enter();
        std::map<int64_t, ThreadHANDLE*>::iterator it = GetThreadRegistry().m_threadsByHandle.find((int64_t)pThis->m_hThread);  // idiom-exempt: as above (pthread_t)
        std::map<int64_t, ThreadHANDLE*>::iterator itEnd = GetThreadRegistry().m_threadsByHandle.end();
        if (it != itEnd)
            GetThreadRegistry().m_threadsByHandle.erase(it);

        std::map<unsigned int, ThreadHANDLE*>::iterator it2 = GetThreadRegistry().m_threadsById.find(pThis->m_nId);
        std::map<unsigned int, ThreadHANDLE*>::iterator itEnd2 = GetThreadRegistry().m_threadsById.end();
        if (it2 != itEnd2)
            GetThreadRegistry().m_threadsById.erase(it2);
        GetThreadRegistry().m_lock.Leave();
    }

    if (pThis->m_bUseEndThread) {
        _endthread();
        return nResult;
    }
    _endthreadex(nResult);
    return nResult;
}

// The pthread mutex and condition variable are deliberately not destroyed; only the pending-IO
// pair and the embedded CWin32MSGQueue are torn down.
ThreadHANDLE::~ThreadHANDLE()
{
    if (m_hPendingIOSem != reinterpret_cast<void*>(-1)) {
        ::CloseHandle(m_hPendingIOSem);
        m_hPendingIOSem = reinterpret_cast<void*>(-1);
    }
    if (m_pPendingIOTail != 0) {
        operator delete(m_pPendingIOTail);
        m_pPendingIOTail = 0;
    }
}


unsigned int ThreadHANDLE::WaitForSingleObject(unsigned int dwMilliseconds)
{
    if (dwMilliseconds != INFINITE) {
        unsigned int nDeadline = GetTickCount() + dwMilliseconds;
        while (m_hThread != 0 && GetTickCount() < nDeadline)
            SleepEx(0, 0);
        return m_hThread == 0 ? 0 : WAIT_TIMEOUT;
    }
    while (m_hThread != 0)
        SleepEx(0, 0);
    return 0;
}

// The semaphore and the free-list head are created once, on the first pending IO.
void ThreadHANDLE::AddPendingIO(MacPendingIO* pPendingIO)
{
    pPendingIO->m_pThread = this;
    if (m_pPendingIOTail != 0)
        return;
    m_hPendingIOSem = CreateSemaphoreA(0, 0, 100, 0);
    m_pPendingIOTail = operator new(16);
    InitializeSListHead(m_pPendingIOTail);
}

void ThreadHANDLE::SignalCompletedIO(MacPendingIO* pPendingIO)
{
    K2_UNUSED(pPendingIO);
}

unsigned int ThreadHANDLE::WaitForPendingIO(unsigned int dwMilliseconds)
{
    unsigned int nResult = ::WaitForSingleObject(m_hPendingIOSem, dwMilliseconds);
    if (nResult == 0) {
        MacPendingIO* pHead = static_cast<MacPendingIO*>(InterlockedPopEntrySList(m_pPendingIOTail));
        pHead->m_pTail->m_pInternalHigh = 0;
        if (pHead != 0) {
            pHead->~MacPendingIO();
            operator delete(pHead);
        }
    }
    return nResult;
}

void ThreadHANDLE::EnableRunnableState()
{
    pthread_mutex_lock(&m_mutex);
    m_bRunnable = true;
    pthread_mutex_unlock(&m_mutex);
    pthread_cond_signal(&m_cond);
}

void ThreadHANDLE::DisableRunnableState()
{
    pthread_mutex_lock(&m_mutex);
    m_bRunnable = false;
    while (!m_bRunnable)
        pthread_cond_wait(&m_cond, &m_mutex);
    pthread_mutex_unlock(&m_mutex);
}

EventHANDLE::EventHANDLE(_SECURITY_ATTRIBUTES* pSec, int bManualReset, int bInitialState,
                         K2WChar const* pszName)
{
    K2_UNUSED(pSec);
    m_szName[0] = 0;
    m_nType = 0;

    GetThreadRegistry().m_syncLock.Enter();
    GetThreadRegistry().m_syncPrimitives.insert(this);
    if (pszName != 0)
        wcscpy(m_szName, pszName);
    else
        m_szName[0] = 0;
    m_nType = 1;
    GetThreadRegistry().m_syncLock.Leave();

    m_bSignaled = false;
    m_nSignalCount = 0;
    m_nWaiterCount = 0;

    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr) == 0) {
        if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) == 0) {
            if (pthread_mutex_init(&m_mutex, &attr) == 0) {
                if (pthread_cond_init(&m_cond, 0) == 0)
                    pthread_mutexattr_destroy(&attr);
            }
        }
    }

    m_bManualReset = (bManualReset != 0);

    if (bInitialState != 0)
        SetEvent();
}

// Removes itself from the sync set, then tears down the mutex, the condition variable and the
// waiter list.
EventHANDLE::~EventHANDLE()
{
    GetThreadRegistry().m_syncLock.Enter();
    std::set<SyncPrimitive*>::iterator itEnd = GetThreadRegistry().m_syncPrimitives.end();
    std::set<SyncPrimitive*>::iterator it = GetThreadRegistry().m_syncPrimitives.find(this);
    if (it != itEnd)
        GetThreadRegistry().m_syncPrimitives.erase(it);
    GetThreadRegistry().m_syncLock.Leave();

    // A local pointer so these three calls form their own scope.
    pthread_mutex_t* pMutex = &m_mutex;
    pthread_mutex_lock(pMutex);
    pthread_mutex_unlock(pMutex);
    pthread_mutex_destroy(pMutex);
    pthread_cond_destroy(&m_cond);
}


// A zero timeout tries the lock and reports a timeout without unlocking; otherwise the wait uses
// an absolute deadline. m_nWaiterCount counts the waiters inside the wait, and m_nSignalCount is
// sampled once per attempt set to decide the verdict afterwards.
unsigned int EventHANDLE::WaitForSingleObject(unsigned int dwMilliseconds)
{
    unsigned int nResult;
    if (dwMilliseconds == 0) {
        if (pthread_mutex_trylock(&m_mutex) != 0)
            return WAIT_TIMEOUT;
    } else {
        pthread_mutex_lock(&m_mutex);
    }

    while (true) {
        if (m_bSignaled) {
            if (!m_bManualReset)
                m_bSignaled = false;
            nResult = 0;
            break;
        }
        if (dwMilliseconds < 2) {
            nResult = WAIT_TIMEOUT;
            break;
        }

        int nOldCount = m_nSignalCount;
        int nSec = static_cast<int>(dwMilliseconds / 1000);
        int nUsec = static_cast<int>(dwMilliseconds % 1000) * 1000000;
        int nRet;
        bool bWoke = false;
        do {
            m_nWaiterCount++;
            if (dwMilliseconds == INFINITE) {
                nRet = pthread_cond_wait(&m_cond, &m_mutex);
            } else {
                timeval tv;
                gettimeofday(&tv, 0);
                timespec ts;
                ts.tv_sec = tv.tv_sec + nSec;
                ts.tv_nsec = tv.tv_usec * 1000 + nUsec;
                if (ts.tv_nsec >= 1000000000) {
                    ts.tv_sec++;
                    ts.tv_nsec -= 1000000000;
                }
                nRet = pthread_cond_timedwait(&m_cond, &m_mutex, &ts);
            }
            m_nWaiterCount--;
            if (m_bSignaled) {
                bWoke = true;
                break;
            }
        } while (dwMilliseconds >= 2 && nRet != ETIMEDOUT && nOldCount == m_nSignalCount);
        if (bWoke)
            continue;
        nResult = (nOldCount == m_nSignalCount) ? WAIT_TIMEOUT : 0;
        break;
    }

    pthread_mutex_unlock(&m_mutex);
    return nResult;
}

unsigned int EventHANDLE::WaitForSingleObjectEx(unsigned int dwMilliseconds)
{
    K2_UNUSED(dwMilliseconds);
    return WAIT_FAILED;
}

unsigned int EventHANDLE::Signal()
{
    pthread_mutex_lock(&m_mutex);
    m_bSignaled = true;
    m_nSignalCount++;
    pthread_cond_broadcast(&m_cond);
    pthread_mutex_unlock(&m_mutex);
    SetLastError(0);
    return 1;
}

unsigned int EventHANDLE::ResetEvent()
{
    pthread_mutex_lock(&m_mutex);
    m_bSignaled = false;
    pthread_mutex_unlock(&m_mutex);
    return 0;
}

unsigned int EventHANDLE::PulseEvent()
{
    pthread_mutex_lock(&m_mutex);
    m_nSignalCount++;
    if (m_nWaiterCount != 0) {
        if (m_bManualReset)
            pthread_cond_broadcast(&m_cond);
        else
            pthread_cond_signal(&m_cond);
    }
    m_bSignaled = false;
    pthread_mutex_unlock(&m_mutex);
    return 0;
}

unsigned int EventHANDLE::SetEvent()
{
    pthread_mutex_lock(&m_mutex);
    m_bSignaled = true;
    m_nSignalCount++;
    pthread_cond_broadcast(&m_cond);
    pthread_mutex_unlock(&m_mutex);
    return 0;
}

SemaphoreHANDLE::SemaphoreHANDLE(int nInitial, int nMax, K2WChar const* pszName)
{
    K2_UNUSED(nInitial); K2_UNUSED(nMax);
    m_szName[0] = 0;
    m_nType = 0;

    GetThreadRegistry().m_syncLock.Enter();
    GetThreadRegistry().m_syncPrimitives.insert(this);
    if (pszName != 0)
        wcscpy(m_szName, pszName);
    else
        m_szName[0] = 0;
    m_nType = 2;
    GetThreadRegistry().m_syncLock.Leave();
}

SemaphoreHANDLE::~SemaphoreHANDLE()
{
    GetThreadRegistry().m_syncLock.Enter();
    std::set<SyncPrimitive*>::iterator itEnd = GetThreadRegistry().m_syncPrimitives.end();
    std::set<SyncPrimitive*>::iterator it = GetThreadRegistry().m_syncPrimitives.find(this);
    if (it != itEnd)
        GetThreadRegistry().m_syncPrimitives.erase(it);
    GetThreadRegistry().m_syncLock.Leave();
}


unsigned int SemaphoreHANDLE::WaitForSingleObject(unsigned int dwMilliseconds)
{
    K2_UNUSED(dwMilliseconds);
    return 0;
}

unsigned int SemaphoreHANDLE::WaitForSingleObjectEx(unsigned int dwMilliseconds)
{
    K2_UNUSED(dwMilliseconds);
    return WAIT_FAILED;
}

unsigned int SemaphoreHANDLE::Signal()
{
    SetLastError(0);
    return 1;
}

unsigned int SemaphoreHANDLE::Release()
{
    return 0;
}

MutexHANDLE::MutexHANDLE(bool bInitialOwner, K2WChar const* pszName)
{
    m_szName[0] = 0;
    m_nType = 0;

    GetThreadRegistry().m_syncLock.Enter();
    GetThreadRegistry().m_syncPrimitives.insert(this);
    if (pszName != 0) {
        wcsncpy(m_szName, pszName, 259);
        m_szName[259] = 0;
    } else {
        m_szName[0] = 0;
    }
    m_nType = 3;
    GetThreadRegistry().m_syncLock.Leave();

    m_hOwner = bInitialOwner ? pthread_self() : 0;
    m_nRecursionCount = 0;
}

MutexHANDLE::~MutexHANDLE()
{
    GetThreadRegistry().m_syncLock.Enter();
    std::set<SyncPrimitive*>::iterator itEnd = GetThreadRegistry().m_syncPrimitives.end();
    std::set<SyncPrimitive*>::iterator it = GetThreadRegistry().m_syncPrimitives.find(this);
    if (it != itEnd)
        GetThreadRegistry().m_syncPrimitives.erase(it);
    GetThreadRegistry().m_syncLock.Leave();
}


unsigned int MutexHANDLE::WaitForSingleObject(unsigned int dwMilliseconds)
{
    K2_UNUSED(dwMilliseconds);
    if (pthread_equal(pthread_self(), m_hOwner))
        m_nRecursionCount++;
    return 0;
}

unsigned int MutexHANDLE::Signal()
{
    int nErr = 0;
    if (pthread_equal(pthread_self(), m_hOwner)) {
        if (m_nRecursionCount != 0)
            m_nRecursionCount--;
        else
            m_hOwner = 0;
    } else {
        nErr = ERROR_NOT_OWNER;
    }
    SetLastError(nErr);
    return nErr == 0;
}

unsigned int MutexHANDLE::Release()
{
    if (!pthread_equal(pthread_self(), m_hOwner))
        return ERROR_NOT_OWNER;
    if (m_nRecursionCount == 0) {
        m_hOwner = 0;
        return 0;
    }
    m_nRecursionCount--;
    return 0;
}

WaitableTimerHANDLE::WaitableTimerHANDLE(_SECURITY_ATTRIBUTES* pSec, int bManualReset,
                                         K2WChar const* pszName)
{
    K2_UNUSED(pSec); K2_UNUSED(bManualReset);
    m_szName[0] = 0;
    m_nType = 0;

    GetThreadRegistry().m_syncLock.Enter();
    GetThreadRegistry().m_syncPrimitives.insert(this);
    if (pszName != 0)
        wcscpy(m_szName, pszName);
    else
        m_szName[0] = 0;
    m_nType = 4;
    GetThreadRegistry().m_syncLock.Leave();
}

WaitableTimerHANDLE::~WaitableTimerHANDLE()
{
    GetThreadRegistry().m_syncLock.Enter();
    std::set<SyncPrimitive*>::iterator itEnd = GetThreadRegistry().m_syncPrimitives.end();
    std::set<SyncPrimitive*>::iterator it = GetThreadRegistry().m_syncPrimitives.find(this);
    if (it != itEnd)
        GetThreadRegistry().m_syncPrimitives.erase(it);
    GetThreadRegistry().m_syncLock.Leave();
}


unsigned int WaitableTimerHANDLE::WaitForSingleObject(unsigned int dwMilliseconds)
{
    K2_UNUSED(dwMilliseconds);
    return WAIT_FAILED;
}

unsigned int WaitableTimerHANDLE::Set(const _LARGE_INTEGER* pDueTime, int nPeriod,
                                     void (*pfn)(void*, unsigned int, unsigned int),
                                     void* pArg, int bResume)
{
    K2_UNUSED(pArg);
    if (pfn == 0 && pDueTime->QuadPart <= 0 && nPeriod == 0) {
        if (bResume)
            SetLastError(ERROR_NOT_SUPPORTED);
        return 1;
    }
    return 0;
}
