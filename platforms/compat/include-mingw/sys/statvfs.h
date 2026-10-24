// POSIX's statvfs, which the shim's GetDiskFreeSpace answers from (Windows builds: windows/win_posix.cpp,
// on GetDiskFreeSpaceExW).
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
struct statvfs {
    unsigned long long f_bsize;
    unsigned long long f_frsize;
    unsigned long long f_blocks;
    unsigned long long f_bfree;
    unsigned long long f_bavail;
};
int statvfs(const char* pszPath, struct statvfs* pBuf);
#ifdef __cplusplus
}
#endif
