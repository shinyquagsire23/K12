#ifndef LIBCSTAT_H
#define LIBCSTAT_H
#include "k2stdint.h"

// The engine's <sys/stat.h>, with bionic's aarch64 struct stat and the three calls the engine
// makes, declared once (port/libcbase.h explains the scheme). stat and chmod are nounwind where
// bionic's are not, so this header comes after every system header in a TU.

#if defined(K2_HOST_BUILD)
// The host C library fills its own, larger layout, so this model would overflow the stack there.
// Only st_mode and the modification time are read, and every layout carries them.
#include <sys/stat.h>
#else  // bionic's declarations, without its headers

#if !defined(_SYS_STAT_H_)
struct stat {
    uint64_t      st_dev;
    uint64_t      st_ino;
    unsigned int  st_mode;
    unsigned int  st_nlink;
    unsigned int  st_uid;
    unsigned int  st_gid;
    uint64_t      st_rdev;
    uint64_t      _pad1;
    int64_t       st_size;
    int           st_blksize;
    int           _pad2;
    int64_t       st_blocks;
    int64_t       st_atime_sec, st_atime_nsec;
    int64_t       st_mtime_sec, st_mtime_nsec;
    int64_t       st_ctime_sec, st_ctime_nsec;
    unsigned int  _unused4, _unused5;
};

extern "C" {
int stat(const char* pszPath, struct stat* pBuf) throw();
int chmod(const char* pszPath, unsigned int nMode) throw();
int fstat(int fd, struct stat* pBuf);
}
#endif

#endif  // K2_HOST_BUILD

#endif // LIBCSTAT_H
