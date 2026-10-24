// dlopen / dlsym / dladdr on Windows.
//
// The shim overrides libc and SDL entry points by defining them (fopen, opendir, SDL_PollEvent ...)
// and reaches the real one with dlsym(RTLD_NEXT, name). Here that is:
//   * the CRT calls: the implementations below, on the CRT's wide-character functions. The shim's
//     paths are UTF-8 (SDL's, the engine's ASCII), so a name outside the ANSI code page still opens;
//     fopen is binary unless the mode says 't' (POSIX has no text mode, and the engine reads binary
//     data with "r"); rename replaces an existing file, as POSIX's does;
//   * SDL's: the static SDL2's own definition of that name, k2sdl_real_<name> (windows/sdl_interposed.h);
//   * anything else: the UCRT's export of that name.
#include <windows.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <wchar.h>

#include <string>

#include "dlfcn.h"

// SDL2's definitions of the names the shim defines too: only their addresses are taken, so one
// declaration shape does for all.
extern "C" {
#define K2SDL_INTERPOSED(name) void k2sdl_real_##name();
#include "sdl_interposed.h"
#undef K2SDL_INTERPOSED
}

namespace {

std::wstring Wide(const char* psz)
{
    if (!psz) return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, psz, -1, 0, 0);
    UINT cp = CP_UTF8;
    if (n <= 0) {   // not UTF-8: the ANSI code page's bytes
        cp = CP_ACP;
        n = MultiByteToWideChar(cp, 0, psz, -1, 0, 0);
    }
    std::wstring w(n > 0 ? n - 1 : 0, L'\0');
    if (n > 1) MultiByteToWideChar(cp, cp == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0, psz, -1, &w[0], n);
    return w;
}

std::string Narrow(const wchar_t* pwsz)
{
    int n = WideCharToMultiByte(CP_UTF8, 0, pwsz, -1, 0, 0, 0, 0);
    std::string s(n > 0 ? n - 1 : 0, '\0');
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, pwsz, -1, &s[0], n, 0, 0);
    return s;
}

// Opened with FILE_SHARE_DELETE, as every POSIX file is: the engine deletes and renames files it
// (or another of its streams) still has open -- a save overwrite removes the slot's savenfo.res
// while a reader of the save list holds it -- which _wfopen's handles refuse ("Device or resource
// busy").
FILE* RealFopen(const char* pszPath, const char* pszMode)
{
    std::string mode(pszMode ? pszMode : "r");
    if (mode.find('t') == std::string::npos && mode.find('b') == std::string::npos) mode += 'b';
    bool bPlus = mode.find('+') != std::string::npos;
    DWORD nAccess = 0, nCreate = 0;
    int nFlags = mode.find('t') != std::string::npos ? _O_TEXT : _O_BINARY;
    switch (mode[0]) {
    case 'r':
        nAccess = GENERIC_READ | (bPlus ? GENERIC_WRITE : 0);
        nCreate = OPEN_EXISTING;
        if (!bPlus) nFlags |= _O_RDONLY;
        break;
    case 'w':
        nAccess = GENERIC_WRITE | (bPlus ? GENERIC_READ : 0);
        nCreate = mode.find('x') != std::string::npos ? CREATE_NEW : CREATE_ALWAYS;
        break;
    case 'a':
        nAccess = GENERIC_WRITE | (bPlus ? GENERIC_READ : 0);
        nCreate = OPEN_ALWAYS;
        nFlags |= _O_APPEND;
        break;
    default:
        errno = EINVAL;
        return 0;
    }
    HANDLE h = CreateFileW(Wide(pszPath).c_str(), nAccess, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           0, nCreate, FILE_ATTRIBUTE_NORMAL, 0);
    if (h == INVALID_HANDLE_VALUE) {
        DWORD e = GetLastError();
        errno = e == ERROR_FILE_NOT_FOUND || e == ERROR_PATH_NOT_FOUND || e == ERROR_INVALID_NAME ? ENOENT
              : e == ERROR_FILE_EXISTS || e == ERROR_ALREADY_EXISTS ? EEXIST
              : e == ERROR_SHARING_VIOLATION || e == ERROR_LOCK_VIOLATION ? EBUSY : EACCES;
        return 0;
    }
    int fd = _open_osfhandle(reinterpret_cast<intptr_t>(h), nFlags);
    if (fd == -1) {
        CloseHandle(h);
        return 0;
    }
    std::string fdmode;
    for (char c : mode)
        if (c != 'x') fdmode += c;
    FILE* f = _fdopen(fd, fdmode.c_str());
    if (!f) _close(fd);
    return f;
}

int RealStat(const char* pszPath, struct stat* pSt)
{
    std::wstring w = Wide(pszPath);
    // _wstat rejects a trailing separator on a directory ("saves/"), which POSIX accepts.
    while (w.size() > 1 && (w.back() == L'/' || w.back() == L'\\') && w[w.size() - 2] != L':') w.pop_back();
    return _wstat64i32(w.c_str(), reinterpret_cast<struct _stat64i32*>(pSt));
}

int RealAccess(const char* pszPath, int nMode) { return _waccess(Wide(pszPath).c_str(), nMode & 6); }
// Another process holding the file without FILE_SHARE_DELETE (a virus scanner or indexer reading
// what the game just wrote): a moment later it is free. Still held, errno is EBUSY.
bool Retry(int& nTries)
{
    DWORD e = GetLastError();
    if (e != ERROR_SHARING_VIOLATION && e != ERROR_LOCK_VIOLATION) return false;
    if (++nTries > 20) {
        errno = EBUSY;
        return false;
    }
    Sleep(50);
    return true;
}

// A read-only file is deleted, as POSIX deletes one from a writable directory.
int RealRemove(const char* pszPath)
{
    std::wstring w = Wide(pszPath);
    DWORD nDirAttr = GetFileAttributesW(w.c_str());
    if (nDirAttr != INVALID_FILE_ATTRIBUTES && (nDirAttr & FILE_ATTRIBUTE_DIRECTORY))
        return _wrmdir(w.c_str());
    for (int nTries = 0;;) {
        if (_wremove(w.c_str()) == 0) return 0;
        if (!Retry(nTries)) break;
    }
    DWORD nAttr = GetFileAttributesW(w.c_str());
    if (errno != EACCES || nAttr == INVALID_FILE_ATTRIBUTES || !(nAttr & FILE_ATTRIBUTE_READONLY) ||
        (nAttr & FILE_ATTRIBUTE_DIRECTORY))
        return -1;
    SetFileAttributesW(w.c_str(), nAttr & ~FILE_ATTRIBUTE_READONLY);
    return _wremove(w.c_str());
}
// POSIX: remove() deletes an empty directory too; unlink() refuses one (EISDIR, not logged).
int RealUnlink(const char* pszPath)
{
    DWORD nAttr = GetFileAttributesW(Wide(pszPath).c_str());
    if (nAttr != INVALID_FILE_ATTRIBUTES && (nAttr & FILE_ATTRIBUTE_DIRECTORY)) {
        errno = EISDIR;
        return -1;
    }
    return RealRemove(pszPath);
}
int RealMkdir(const char* pszPath, mode_t) { return _wmkdir(Wide(pszPath).c_str()); }
int RealRmdir(const char* pszPath) { return _wrmdir(Wide(pszPath).c_str()); }
int RealChdir(const char* pszPath) { return _wchdir(Wide(pszPath).c_str()); }
int RealChmod(const char* pszPath, int nMode) { return _wchmod(Wide(pszPath).c_str(), nMode & (_S_IREAD | _S_IWRITE)); }

int RealRename(const char* pszOld, const char* pszNew)
{
    std::wstring from = Wide(pszOld), to = Wide(pszNew);
    for (int nTries = 0;;) {
        if (MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) return 0;
        DWORD e = GetLastError();
        errno = e == ERROR_FILE_NOT_FOUND || e == ERROR_PATH_NOT_FOUND ? ENOENT : EACCES;
        if (!Retry(nTries)) return -1;
    }
}

// A directory listing on FindFirstFileW, handed out as the CRT's DIR (only these three see inside).
struct WinDir {
    HANDLE hFind;
    WIN32_FIND_DATAW data;
    bool bHave;
    struct dirent entry;
};

DIR* RealOpendir(const char* pszPath)
{
    std::wstring pattern = Wide(pszPath && *pszPath ? pszPath : ".");
    if (pattern.back() != L'/' && pattern.back() != L'\\') pattern += L'/';
    pattern += L'*';
    WinDir* d = new WinDir();
    d->hFind = FindFirstFileW(pattern.c_str(), &d->data);
    if (d->hFind == INVALID_HANDLE_VALUE) {
        delete d;
        errno = ENOENT;
        return 0;
    }
    d->bHave = true;
    return reinterpret_cast<DIR*>(d);
}

struct dirent* RealReaddir(DIR* p)
{
    WinDir* d = reinterpret_cast<WinDir*>(p);
    if (!d || !d->bHave) return 0;
    std::string name = Narrow(d->data.cFileName);
    snprintf(d->entry.d_name, sizeof d->entry.d_name, "%s", name.c_str());
    d->entry.d_namlen = static_cast<unsigned short>(strlen(d->entry.d_name));
    d->entry.d_reclen = sizeof d->entry;
    d->entry.d_ino = 0;
    d->entry.d_type = (d->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DT_DIR : DT_REG;
    d->bHave = FindNextFileW(d->hFind, &d->data) != 0;
    return &d->entry;
}

int RealClosedir(DIR* p)
{
    WinDir* d = reinterpret_cast<WinDir*>(p);
    if (!d) return -1;
    FindClose(d->hFind);
    delete d;
    return 0;
}

struct Entry {
    const char* pszName;
    void* pFn;
};
const Entry s_aReal[] = {
    { "fopen", reinterpret_cast<void*>(RealFopen) },     { "stat", reinterpret_cast<void*>(RealStat) },       { "access", reinterpret_cast<void*>(RealAccess) },
    { "remove", reinterpret_cast<void*>(RealRemove) },   { "rename", reinterpret_cast<void*>(RealRename) },   { "unlink", reinterpret_cast<void*>(RealUnlink) },
    { "mkdir", reinterpret_cast<void*>(RealMkdir) },     { "rmdir", reinterpret_cast<void*>(RealRmdir) },     { "chdir", reinterpret_cast<void*>(RealChdir) },
    { "chmod", reinterpret_cast<void*>(RealChmod) },
    { "opendir", reinterpret_cast<void*>(RealOpendir) }, { "readdir", reinterpret_cast<void*>(RealReaddir) }, { "closedir", reinterpret_cast<void*>(RealClosedir) },
#define K2SDL_INTERPOSED(name) { #name, reinterpret_cast<void*>(k2sdl_real_##name) },
#include "sdl_interposed.h"
#undef K2SDL_INTERPOSED
};

char s_szError[256];

} // namespace

extern "C" {

void* dlopen(const char* pszFile, int)
{
    HMODULE h = pszFile ? LoadLibraryW(Wide(pszFile).c_str()) : GetModuleHandleW(0);
    if (!h) snprintf(s_szError, sizeof s_szError, "dlopen %s: error %lu", pszFile, GetLastError());
    return static_cast<void*>(h);
}

void* dlsym(void* pHandle, const char* pszName)
{
    if (pHandle == RTLD_NEXT) {
        for (const Entry& e : s_aReal)
            if (strcmp(e.pszName, pszName) == 0) return e.pFn;
        if (HMODULE h = GetModuleHandleW(L"ucrtbase.dll"))
            if (FARPROC p = GetProcAddress(h, pszName)) return reinterpret_cast<void*>(p);
        snprintf(s_szError, sizeof s_szError, "dlsym RTLD_NEXT %s: not found", pszName);
        return 0;
    }
    HMODULE h = pHandle == RTLD_DEFAULT ? GetModuleHandleW(0) : static_cast<HMODULE>(pHandle);
    FARPROC p = GetProcAddress(h, pszName);
    if (!p) snprintf(s_szError, sizeof s_szError, "dlsym %s: not found", pszName);
    return reinterpret_cast<void*>(p);
}

int dlclose(void* pHandle) { return pHandle && FreeLibrary(static_cast<HMODULE>(pHandle)) ? 0 : -1; }

const char* dlerror(void) { return s_szError[0] ? s_szError : 0; }

int dladdr(const void* pAddr, Dl_info* pInfo)
{
    HMODULE h = 0;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            static_cast<LPCWSTR>(pAddr), &h))
        return 0;
    static char s_szModule[MAX_PATH];
    GetModuleFileNameA(h, s_szModule, sizeof s_szModule);
    pInfo->dli_fname = s_szModule;
    pInfo->dli_fbase = static_cast<void*>(h);
    pInfo->dli_sname = 0;
    pInfo->dli_saddr = 0;
    return 1;
}

int lstat(const char* pszPath, struct stat* pSt) { return RealStat(pszPath, pSt); }

} // extern "C"
