// mingw-w64's <sys/stat.h> plus what POSIX's has and it lacks (Windows builds only): the two-argument
// mkdir (the old one-argument one is renamed away, windows/mingw_crt.py) and lstat, which on a file
// system without symlinks is stat itself. Both are the shim's: shim/shim_stdio.cpp's mkdir, and
// windows/win_posix.cpp's lstat, which reaches the file system past the shim's own stat. `_stat` is
// a plain function, as on the engine's own CRT (the shim's, shim/shim_stdio.cpp), not mingw-w64's
// macro for _stat64i32.
#include_next <sys/stat.h>
#ifndef K2_MINGW_SYS_STAT_H
#define K2_MINGW_SYS_STAT_H
#ifdef __cplusplus
extern "C" {
#endif
int mkdir(const char* pszPath, mode_t nMode);
int lstat(const char* pszPath, struct stat* pSt);
#ifdef __cplusplus
}
#endif
#define S_ISLNK(m) 0
#undef _stat
#endif
