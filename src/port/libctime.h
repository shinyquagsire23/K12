#ifndef LIBCTIME_H
#define LIBCTIME_H
#include "k2stdint.h"

// The engine's <time.h>, the time functions it calls, declared once (port/libcbase.h explains the
// scheme). The functions are declared as bionic declares them (none is nounwind: CERFFile::Write
// keeps a landing pad on time and localtime); the types are bionic's own and would collide with its
// <time.h>, so this header comes after every system header in a TU.

#if defined(K2_HOST_BUILD)
#include <time.h>
#else  // bionic's declarations, without its headers

#if !defined(_TIME_H_)
typedef int64_t time_t;
typedef int64_t clock_t;

// bionic's layout.
struct tm {
    int         tm_sec;
    int         tm_min;
    int         tm_hour;
    int         tm_mday;
    int         tm_mon;
    int         tm_year;
    int         tm_wday;
    int         tm_yday;
    int         tm_isdst;
    long        tm_gmtoff;
    const char* tm_zone;
};

extern "C" {
time_t     time(time_t* pTimer);
struct tm* localtime(const time_t* pTimer);
clock_t    clock();
}
#endif

#endif  // K2_HOST_BUILD

#endif // LIBCTIME_H
