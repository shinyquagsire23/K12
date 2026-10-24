// The POSIX calls mingw-w64 (its CRT glue and winpthreads) supplies and the MSVC UCRT does not
// (MSVC builds only; the declarations are compat/include-msvc/): threads, sleeps, the clocks,
// ftruncate.
#include <windows.h>

#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <io.h>
#include <process.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static_assert(sizeof(SRWLOCK) == sizeof(void*) && sizeof(CONDITION_VARIABLE) == sizeof(void*) &&
              sizeof(INIT_ONCE) == sizeof(void*), "pthread.h's opaque records hold one pointer each");

namespace {

SRWLOCK* Srw(pthread_mutex_t* p) { return reinterpret_cast<SRWLOCK*>(&p->srw); }
CONDITION_VARIABLE* Cv(pthread_cond_t* p) { return reinterpret_cast<CONDITION_VARIABLE*>(&p->cv); }

// A wait gives the lock up whole (a recursive owner's count included) and takes it back the same.
int Wait(pthread_cond_t* pCond, pthread_mutex_t* pMutex, DWORD nMs)
{
    unsigned int nCount = pMutex->count;
    pMutex->owner = 0;
    pMutex->count = 0;
    BOOL bOk = SleepConditionVariableSRW(Cv(pCond), Srw(pMutex), nMs, 0);
    pMutex->owner = GetCurrentThreadId();
    pMutex->count = nCount;
    return bOk ? 0 : (GetLastError() == ERROR_TIMEOUT ? ETIMEDOUT : EINVAL);
}

uint64_t RealtimeNs()
{
    FILETIME ft;
    GetSystemTimePreciseAsFileTime(&ft);
    uint64_t n = (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    return (n - 116444736000000000ull) * 100;   // 1601 -> 1970, 100 ns -> ns
}

uint64_t MonotonicNs()
{
    static LARGE_INTEGER s_freq;
    if (!s_freq.QuadPart) QueryPerformanceFrequency(&s_freq);
    LARGE_INTEGER n;
    QueryPerformanceCounter(&n);
    return static_cast<uint64_t>(n.QuadPart / s_freq.QuadPart) * 1000000000ull +
           static_cast<uint64_t>(n.QuadPart % s_freq.QuadPart) * 1000000000ull / static_cast<uint64_t>(s_freq.QuadPart);
}

void SleepNs(uint64_t nNs)
{
    // A high-resolution waitable timer; Sleep's 1 ms granularity is the fallback.
    HANDLE h = CreateWaitableTimerExW(0, 0, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    if (!h) {
        Sleep(static_cast<DWORD>((nNs + 999999) / 1000000));
        return;
    }
    LARGE_INTEGER due;
    due.QuadPart = -static_cast<LONGLONG>((nNs + 99) / 100);
    SetWaitableTimer(h, &due, 0, 0, 0, FALSE);
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
}

struct Start {
    void* (*pfn)(void*);
    void* pArg;
};

unsigned __stdcall ThreadMain(void* p)
{
    Start s = *static_cast<Start*>(p);
    delete static_cast<Start*>(p);
    s.pfn(s.pArg);
    return 0;
}

} // namespace

extern "C" {

int pthread_mutexattr_init(pthread_mutexattr_t* pAttr) { pAttr->type = PTHREAD_MUTEX_NORMAL; return 0; }
int pthread_mutexattr_destroy(pthread_mutexattr_t*) { return 0; }
int pthread_mutexattr_settype(pthread_mutexattr_t* pAttr, int nType) { pAttr->type = nType; return 0; }

int pthread_mutex_init(pthread_mutex_t* pMutex, const pthread_mutexattr_t* pAttr)
{
    InitializeSRWLock(Srw(pMutex));
    pMutex->owner = 0;
    pMutex->count = 0;
    pMutex->type = pAttr ? pAttr->type : PTHREAD_MUTEX_NORMAL;
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t*) { return 0; }

int pthread_mutex_lock(pthread_mutex_t* pMutex)
{
    DWORD nSelf = GetCurrentThreadId();
    if (pMutex->type == PTHREAD_MUTEX_RECURSIVE && pMutex->owner == nSelf) {
        ++pMutex->count;
        return 0;
    }
    AcquireSRWLockExclusive(Srw(pMutex));
    pMutex->owner = nSelf;
    pMutex->count = 1;
    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t* pMutex)
{
    DWORD nSelf = GetCurrentThreadId();
    if (pMutex->type == PTHREAD_MUTEX_RECURSIVE && pMutex->owner == nSelf) {
        ++pMutex->count;
        return 0;
    }
    if (!TryAcquireSRWLockExclusive(Srw(pMutex))) return EBUSY;
    pMutex->owner = nSelf;
    pMutex->count = 1;
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t* pMutex)
{
    if (--pMutex->count) return 0;
    pMutex->owner = 0;
    ReleaseSRWLockExclusive(Srw(pMutex));
    return 0;
}

int pthread_cond_init(pthread_cond_t* pCond, const pthread_condattr_t*) { InitializeConditionVariable(Cv(pCond)); return 0; }
int pthread_cond_destroy(pthread_cond_t*) { return 0; }
int pthread_cond_wait(pthread_cond_t* pCond, pthread_mutex_t* pMutex) { return Wait(pCond, pMutex, INFINITE); }

int pthread_cond_timedwait(pthread_cond_t* pCond, pthread_mutex_t* pMutex, const struct timespec* pAbsTime)
{
    uint64_t nDeadline = static_cast<uint64_t>(pAbsTime->tv_sec) * 1000000000ull + static_cast<uint64_t>(pAbsTime->tv_nsec);
    uint64_t nNow = RealtimeNs();
    DWORD nMs = nDeadline > nNow ? static_cast<DWORD>((nDeadline - nNow + 999999) / 1000000) : 0;
    return Wait(pCond, pMutex, nMs);
}

int pthread_cond_signal(pthread_cond_t* pCond) { WakeConditionVariable(Cv(pCond)); return 0; }
int pthread_cond_broadcast(pthread_cond_t* pCond) { WakeAllConditionVariable(Cv(pCond)); return 0; }

int pthread_attr_init(pthread_attr_t* pAttr) { pAttr->stack = 0; pAttr->detach = PTHREAD_CREATE_JOINABLE; return 0; }
int pthread_attr_destroy(pthread_attr_t*) { return 0; }
int pthread_attr_setstacksize(pthread_attr_t* pAttr, size_t nSize) { pAttr->stack = nSize; return 0; }
int pthread_attr_setdetachstate(pthread_attr_t* pAttr, int nState) { pAttr->detach = nState; return 0; }

// Nothing joins: the handle is closed at once, and the thread is named by its id.
int pthread_create(pthread_t* pThread, const pthread_attr_t* pAttr, void* (*pfnStart)(void*), void* pArg)
{
    unsigned int nId = 0;
    uintptr_t h = _beginthreadex(0, pAttr ? static_cast<unsigned int>(pAttr->stack) : 0, ThreadMain,
                                 new Start{ pfnStart, pArg }, 0, &nId);
    if (!h) return EAGAIN;
    CloseHandle(reinterpret_cast<HANDLE>(h));
    *pThread = nId;
    return 0;
}

int pthread_detach(pthread_t) { return 0; }
void pthread_exit(void*) { ExitThread(0); }
pthread_t pthread_self(void) { return GetCurrentThreadId(); }
int pthread_equal(pthread_t a, pthread_t b) { return a == b; }

int pthread_once(pthread_once_t* pOnce, void (*pfnInit)(void))
{
    struct Call {
        static BOOL CALLBACK Run(PINIT_ONCE, PVOID p, PVOID*) { reinterpret_cast<void (*)(void)>(p)(); return TRUE; }
    };
    InitOnceExecuteOnce(reinterpret_cast<PINIT_ONCE>(&pOnce->once), Call::Run, reinterpret_cast<PVOID>(pfnInit), 0);
    return 0;
}

int pthread_setname_np(pthread_t thread, const char* pszName)
{
    HANDLE h = OpenThread(THREAD_SET_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(thread));
    if (!h) return ESRCH;
    wchar_t sz[64];
    MultiByteToWideChar(CP_UTF8, 0, pszName, -1, sz, 64);
    sz[63] = 0;
    SetThreadDescription(h, sz);
    CloseHandle(h);
    return 0;
}

int clock_gettime(clockid_t nClock, struct timespec* pTs)
{
    uint64_t n = nClock == CLOCK_MONOTONIC ? MonotonicNs() : RealtimeNs();
    pTs->tv_sec = static_cast<time_t>(n / 1000000000ull);
    pTs->tv_nsec = static_cast<long>(n % 1000000000ull);
    return 0;
}

int nanosleep(const struct timespec* pReq, struct timespec*)
{
    SleepNs(static_cast<uint64_t>(pReq->tv_sec) * 1000000000ull + static_cast<uint64_t>(pReq->tv_nsec));
    return 0;
}

int clock_nanosleep(clockid_t nClock, int nFlags, const struct timespec* pReq, struct timespec*)
{
    uint64_t nReq = static_cast<uint64_t>(pReq->tv_sec) * 1000000000ull + static_cast<uint64_t>(pReq->tv_nsec);
    if (nFlags & TIMER_ABSTIME) {
        uint64_t nNow = nClock == CLOCK_MONOTONIC ? MonotonicNs() : RealtimeNs();
        if (nReq <= nNow) return 0;
        nReq -= nNow;
    }
    SleepNs(nReq);
    return 0;
}

int strcasecmp(const char* pszA, const char* pszB) { return _stricmp(pszA, pszB); }
int strncasecmp(const char* pszA, const char* pszB, size_t n) { return _strnicmp(pszA, pszB, n); }

int usleep(unsigned int nMicroseconds) { SleepNs(static_cast<uint64_t>(nMicroseconds) * 1000); return 0; }
unsigned int sleep(unsigned int nSeconds) { Sleep(nSeconds * 1000); return 0; }
int ftruncate(int nFd, long long nLength) { return _chsize_s(nFd, nLength) ? -1 : 0; }

int gettimeofday(struct timeval* pTv, struct timezone*)
{
    uint64_t n = RealtimeNs() / 1000;
    pTv->tv_sec = static_cast<long>(n / 1000000);
    pTv->tv_usec = static_cast<long>(n % 1000000);
    return 0;
}

// bionic's fortify entry points (the engine was built with _FORTIFY_SOURCE: __builtin___strcpy_chk
// and friends); mingw-w64's libssp has them. The object size is not checked again here.
void* __memcpy_chk(void* pDst, const void* pSrc, size_t n, size_t) { return memcpy(pDst, pSrc, n); }
void* __memset_chk(void* pDst, int c, size_t n, size_t) { return memset(pDst, c, n); }
char* __strcpy_chk(char* pDst, const char* pSrc, size_t) { return strcpy(pDst, pSrc); }
char* __strncpy_chk(char* pDst, const char* pSrc, size_t n, size_t) { return strncpy(pDst, pSrc, n); }
char* __strcat_chk(char* pDst, const char* pSrc, size_t) { return strcat(pDst, pSrc); }

// sincosf (glibc's and bionic's; clang folds sinf + cosf of one angle into it).
void sincosf(float f, float* pSin, float* pCos)
{
    *pSin = sinf(f);
    *pCos = cosf(f);
}

// The UCRT's _vsnprintf is an inline in <stdio.h>; the engine declares and calls an extern one.
int k2_vsnprintf(char* pszBuffer, size_t nSize, const char* pszFormat, va_list ap)
{
    return _vsnprintf_l(pszBuffer, nSize, pszFormat, 0, ap);
}
#pragma comment(linker, "/alternatename:_vsnprintf=k2_vsnprintf")

} // extern "C"
