// The POSIX threads the engine's thread layer (src/port/ASL2/Win/WinThreadsMP) and the shim use, for
// the MSVC UCRT, which has none (mingw-w64 has winpthreads): a mutex, a condition variable, a thread,
// once and a thread name, over Win32's SRW locks and condition variables. Bodies:
// windows/win_msvc_posix.cpp. The records are opaque here (the engine never sees <windows.h>); the
// .cpp pins them against the Win32 types.
#ifndef K2_MSVC_PTHREAD_H
#define K2_MSVC_PTHREAD_H
#include <stddef.h>
#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef unsigned long long pthread_t;
typedef struct { void* srw; unsigned long owner; unsigned int count; int type; } pthread_mutex_t;
typedef struct { int type; } pthread_mutexattr_t;
typedef struct { void* cv; } pthread_cond_t;
typedef struct { int unused; } pthread_condattr_t;
typedef struct { size_t stack; int detach; } pthread_attr_t;
typedef struct { void* once; } pthread_once_t;
#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_MUTEX_RECURSIVE 1
#define PTHREAD_MUTEX_DEFAULT PTHREAD_MUTEX_NORMAL
#define PTHREAD_MUTEX_INITIALIZER { 0, 0, 0, PTHREAD_MUTEX_NORMAL }
#define PTHREAD_COND_INITIALIZER { 0 }
#define PTHREAD_ONCE_INIT { 0 }
#define PTHREAD_CREATE_JOINABLE 0
#define PTHREAD_CREATE_DETACHED 1
int pthread_mutexattr_init(pthread_mutexattr_t* pAttr);
int pthread_mutexattr_destroy(pthread_mutexattr_t* pAttr);
int pthread_mutexattr_settype(pthread_mutexattr_t* pAttr, int nType);
int pthread_mutex_init(pthread_mutex_t* pMutex, const pthread_mutexattr_t* pAttr);
int pthread_mutex_destroy(pthread_mutex_t* pMutex);
int pthread_mutex_lock(pthread_mutex_t* pMutex);
int pthread_mutex_trylock(pthread_mutex_t* pMutex);
int pthread_mutex_unlock(pthread_mutex_t* pMutex);
int pthread_cond_init(pthread_cond_t* pCond, const pthread_condattr_t* pAttr);
int pthread_cond_destroy(pthread_cond_t* pCond);
int pthread_cond_wait(pthread_cond_t* pCond, pthread_mutex_t* pMutex);
int pthread_cond_timedwait(pthread_cond_t* pCond, pthread_mutex_t* pMutex, const struct timespec* pAbsTime);
int pthread_cond_signal(pthread_cond_t* pCond);
int pthread_cond_broadcast(pthread_cond_t* pCond);
int pthread_attr_init(pthread_attr_t* pAttr);
int pthread_attr_destroy(pthread_attr_t* pAttr);
int pthread_attr_setstacksize(pthread_attr_t* pAttr, size_t nSize);
int pthread_attr_setdetachstate(pthread_attr_t* pAttr, int nState);
int pthread_create(pthread_t* pThread, const pthread_attr_t* pAttr, void* (*pfnStart)(void*), void* pArg);
int pthread_detach(pthread_t thread);
void pthread_exit(void* pValue);
pthread_t pthread_self(void);
int pthread_equal(pthread_t a, pthread_t b);
int pthread_once(pthread_once_t* pOnce, void (*pfnInit)(void));
int pthread_setname_np(pthread_t thread, const char* pszName);
#ifdef __cplusplus
}
#endif
#endif
