// POSIX's <dirent.h> as the engine reads it (Windows builds only), in place of mingw-w64's, whose
// struct dirent has no d_type (KOTOR 1's CExoBaseInternal::GetDirectoryList tells directories from
// files by it). The three calls are the shim's: shim/shim_path.cpp's merged listings over
// windows/win_dlfcn.cpp's FindFirstFileW one.
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#define DT_UNKNOWN 0
#define DT_DIR 4
#define DT_REG 8
struct dirent {
    long long d_ino;
    unsigned short d_reclen;
    unsigned short d_namlen;
    unsigned char d_type;
    char d_name[260 * 3];   /* a MAX_PATH name, as UTF-8 */
};
typedef struct K2Dir DIR;
DIR* opendir(const char* pszPath);
struct dirent* readdir(DIR* pDir);
int closedir(DIR* pDir);
#ifdef __cplusplus
}
#endif
