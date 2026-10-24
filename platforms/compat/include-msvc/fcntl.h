// The UCRT's <fcntl.h> plus the POSIX open flags (compat/include-mingw/fcntl.h adds O_CLOEXEC).
#include_next <fcntl.h>
#ifndef K2_MSVC_FCNTL_H
#define K2_MSVC_FCNTL_H
#ifndef O_RDONLY
#define O_RDONLY _O_RDONLY
#define O_WRONLY _O_WRONLY
#define O_RDWR _O_RDWR
#define O_CREAT _O_CREAT
#define O_TRUNC _O_TRUNC
#define O_EXCL _O_EXCL
#define O_APPEND _O_APPEND
#define O_BINARY _O_BINARY
#endif
#endif
