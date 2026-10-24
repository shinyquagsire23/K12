// mingw-w64's <unistd.h> plus pread (Windows builds only: windows/win_posix.cpp, a positioned
// ReadFile that leaves the descriptor's offset alone, as POSIX's does) and sysconf.
#include_next <unistd.h>
#ifndef K2_MINGW_UNISTD_H
#define K2_MINGW_UNISTD_H
#include <sys/types.h>
#ifdef __cplusplus
extern "C" {
#endif
long long pread(int nFd, void* pBuf, size_t nCount, long long nOffset);
// sysconf for the one name the shim asks (the processor count).
#define _SC_NPROCESSORS_ONLN 84
long sysconf(int nName);
#ifdef __cplusplus
}
#endif
#endif
