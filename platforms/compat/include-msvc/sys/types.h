// The UCRT's <sys/types.h> plus the POSIX types it lacks (mingw-w64 has them).
#include_next <sys/types.h>
#ifndef K2_MSVC_SYS_TYPES_H
#define K2_MSVC_SYS_TYPES_H
typedef unsigned short mode_t;
typedef long long ssize_t;
typedef int pid_t;
#define off_t long long   /* POSIX's 64-bit offset (fseeko); the UCRT's _off_t is 32 bits */
typedef unsigned int useconds_t;
#endif
