// mingw-w64's <time.h> with POSIX's thread-safe calls (localtime_r, gmtime_r) and timegm (Windows
// builds only).
#ifndef _POSIX_THREAD_SAFE_FUNCTIONS
#define _POSIX_THREAD_SAFE_FUNCTIONS 200112L
#endif
#include_next <time.h>
#ifndef K2_MINGW_TIME_H
#define K2_MINGW_TIME_H
static inline time_t timegm(struct tm* pTm) { return _mkgmtime(pTm); }
#endif
