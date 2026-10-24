// The UCRT's <time.h> plus POSIX's localtime_r / gmtime_r / timegm / clock_gettime / nanosleep
// (mingw-w64 and winpthreads have them; bodies: windows/win_msvc_posix.cpp).
#include_next <time.h>
#ifndef K2_MSVC_TIME_H
#define K2_MSVC_TIME_H
#ifdef __cplusplus
extern "C" {
#endif
static __inline struct tm* localtime_r(const time_t* pT, struct tm* pTm) { return localtime_s(pTm, pT) ? 0 : pTm; }
static __inline struct tm* gmtime_r(const time_t* pT, struct tm* pTm) { return gmtime_s(pTm, pT) ? 0 : pTm; }
typedef int clockid_t;
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
int clock_gettime(clockid_t nClock, struct timespec* pTs);
int nanosleep(const struct timespec* pReq, struct timespec* pRem);
#define TIMER_ABSTIME 1
int clock_nanosleep(clockid_t nClock, int nFlags, const struct timespec* pReq, struct timespec* pRem);
#ifdef __cplusplus
}
#endif
#endif
