// The UCRT's <sys/stat.h> plus the POSIX mode macros it lacks; compat/include-mingw's adds mkdir /
// lstat on top (this one is searched first and includes it).
#include <sys/types.h>
#include_next <sys/stat.h>
#ifndef K2_MSVC_SYS_STAT_H
#define K2_MSVC_SYS_STAT_H
#include <io.h>
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#define S_IWUSR _S_IWRITE
#define S_IXUSR _S_IEXEC
#define S_IRWXU (_S_IREAD | _S_IWRITE | _S_IEXEC)
#define S_IRGRP 0
#define S_IWGRP 0
#define S_IXGRP 0
#define S_IRWXG 0
#define S_IROTH 0
#define S_IWOTH 0
#define S_IXOTH 0
#define S_IRWXO 0
#endif
#endif
// The 64-bit record and call mingw-w64 names stat64 (MacFiles.cpp's GetFileAttributesEx_core).
#ifndef stat64
#define stat64 _stat64
#endif
