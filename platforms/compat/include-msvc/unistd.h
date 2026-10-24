// POSIX <unistd.h> for the MSVC UCRT, which has none: the descriptor calls under their POSIX names
// (the UCRT's are underscored, <io.h> / <direct.h> / <process.h>), plus what compat/include-mingw's
// unistd.h adds (pread, sysconf). Bodies: windows/win_msvc_posix.cpp, windows/win_posix.cpp.
#ifndef K2_MSVC_UNISTD_H
#define K2_MSVC_UNISTD_H
#include <io.h>
#include <direct.h>
#include <process.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef __cplusplus
extern "C" {
#endif
#ifndef R_OK
#define F_OK 0
#define W_OK 2
#define R_OK 4
#define X_OK 0
#endif
#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif
long long pread(int nFd, void* pBuf, size_t nCount, long long nOffset);
#define _SC_NPROCESSORS_ONLN 84
long sysconf(int nName);
int usleep(unsigned int nMicroseconds);
unsigned int sleep(unsigned int nSeconds);
int ftruncate(int nFd, long long nLength);
#ifdef __cplusplus
}
#endif
#endif
