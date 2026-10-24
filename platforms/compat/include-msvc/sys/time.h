// POSIX <sys/time.h> for the MSVC UCRT: gettimeofday (windows/win_msvc_posix.cpp).
#ifndef K2_MSVC_SYS_TIME_H
#define K2_MSVC_SYS_TIME_H
#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif
// winsock.h (which <windows.h> includes) defines the same record unguarded: take it from there when
// it came first, else define it and keep winsock.h out (nothing here uses sockets).
#if !defined(_WINSOCKAPI_) && !defined(_WINSOCK2API_)
#define _WINSOCKAPI_
struct timeval { long tv_sec; long tv_usec; };
#endif
struct timezone;
int gettimeofday(struct timeval* pTv, struct timezone* pTz);
#ifdef __cplusplus
}
#endif
#endif
