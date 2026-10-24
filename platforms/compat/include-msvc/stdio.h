// The UCRT's <stdio.h> plus POSIX's 64-bit seek names and popen (mingw-w64 has them).
#include_next <stdio.h>
#ifndef K2_MSVC_STDIO_H
#define K2_MSVC_STDIO_H
#include <sys/types.h>
#define fseeko _fseeki64
#define ftello _ftelli64
#define popen _popen
#define pclose _pclose
#define fileno _fileno
#endif
