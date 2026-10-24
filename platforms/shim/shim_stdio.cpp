// Aspyr's libc overrides. libkotor2.so defines its own fopen/stat/rename/remove/access/mkdir/
// rmdir/unlink/chdir (the Android build reads the game through its OBB VFS), so the engine hands them
// DOS paths straight from the alias resolver (".\\CHITIN.key"). Each one here translates the path --
// separators, "./", case-blind lookup -- and calls the real libc function. The definitions interpose
// for every object linked into the executable; dylibs such as SDL keep libc's. Translation is
// idempotent, so host paths from our own code pass through unchanged.
//
// With a user directory, reads look there first and every write -- fopen's write modes, mkdir, rename,
// remove, unlink, rmdir -- goes there: the install is never written.
//
// When the native file is not there, fopen (read modes), stat and access fall back to the mounted
// OBBs, as the Android build's overrides fall back to its ASL::FsApi VFS. fopen's last resort is the
// files compiled into the binary: the port's GLSL shaders, which a PC data set does not have.
#include "k2shim.h"

#include <SDL.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <strings.h>
#include <unistd.h>

#include <algorithm>
#include <vector>

namespace {

template <class Fn>
Fn Real(const char* pszName)
{
    Fn fn = reinterpret_cast<Fn>(dlsym(RTLD_NEXT, pszName));
    if (!fn) {
        fprintf(stderr, "k2shim: libc %s not found\n", pszName);
        abort();
    }
    return fn;
}

struct HostPath {
    char buf[1024];
    explicit HostPath(const char* p) { k2shim_host_path(p, buf, sizeof buf); }
    operator const char*() const { return buf; }
};

// Path a write lands on, the user directory's copy, its directories made when bMake.
struct WritePath {
    char buf[1024];
    explicit WritePath(const char* p, bool bMake = false) { k2shim_host_path_write(p, buf, sizeof buf, bMake); }
    operator const char*() const { return buf; }
};

// A delete or rename that failed for a file that is there -- a save overwrite that silently kept the
// old files -- says which file and why. errno survives the log line.
int Logged(const char* pszOp, const char* pszPath, int r)
{
    if (r != 0 && errno != ENOENT && errno != EISDIR) {
        int e = errno;
        fprintf(stderr, "k2shim: %s %s failed: %s\n", pszOp, pszPath, strerror(e));
        errno = e;
    }
    return r;
}

bool IsWrite(const char* pszMode) { return pszMode && strpbrk(pszMode, "wa+"); }

// Opens an .ini written on Windows, whose lines end in CR LF, as a stream without the CRs: the
// engine's reader splits on LF alone, so every value kept its CR and [Alias] HD0=.\<CR> sent every
// lookup to "\r/chitin.key". Anything but an .ini, and an .ini with no CR in it, comes back as it is.
FILE* WithoutCRs(FILE* f, const char* pszPath)
{
    size_t n = strlen(pszPath);
    if (!f || n < 4 || strcasecmp(pszPath + n - 4, ".ini") != 0) return f;
    std::vector<char> text;
    char buf[4096];
    for (size_t got; (got = fread(buf, 1, sizeof buf, f)) > 0;) text.insert(text.end(), buf, buf + got);
    if (std::find(text.begin(), text.end(), '\r') == text.end()) {
        rewind(f);
        return f;
    }
    FILE* copy = tmpfile();
    if (!copy) {
        rewind(f);
        return f;
    }
    fclose(f);
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    fwrite(text.data(), 1, text.size(), copy);
    rewind(copy);
    return copy;
}

} // namespace

extern "C" {

FILE* fopen(const char* __restrict pszPath, const char* __restrict pszMode)
{
    static auto real = Real<FILE* (*)(const char*, const char*)>("fopen");
    if (!pszPath) return 0;
    if (IsWrite(pszMode)) return real(WritePath(pszPath, true), pszMode);
    HostPath host(pszPath);
    FILE* f = WithoutCRs(real(host, pszMode), host);
    if (!f) f = k2shim_obb_fopen(host, pszMode);
    if (!f && !strpbrk(pszMode, "wa+")) {
        size_t nSize;
        if (const unsigned char* pData = k2embedded_find(host, &nSize))
            f = fmemopen(const_cast<unsigned char*>(pData), nSize, "r");
    }
    return f;
}

int stat(const char* __restrict pszPath, struct stat* __restrict pSt)
{
    static auto real = Real<int (*)(const char*, struct stat*)>("stat");
    HostPath host(pszPath);
    int r = real(host, pSt);
    return r == 0 || !k2shim_obb_stat(host, pSt) ? r : 0;
}

int access(const char* pszPath, int nMode)
{
    static auto real = Real<int (*)(const char*, int)>("access");
    HostPath host(pszPath);
    int r = real(host, nMode);
    struct stat st;
    return r == 0 || (nMode & W_OK) || !k2shim_obb_stat(host, &st) ? r : 0;
}

// The engine's GetFileAttributes_core calls the MSVC CRT's `_stat`, and its sibling
// GetFileAttributesEx_core calls bionic's stat64: on LP64 both name the same record, so both take the
// stat path above. Windows has mingw-w64's own stat64, whose record is its own.
int _stat(const char* pszPath, struct stat* pSt) { return stat(pszPath, pSt); }
#if defined(__APPLE__)
int stat64(const char* pszPath, struct stat* pSt) { return stat(pszPath, pSt); }
#elif !defined(_WIN32)
int stat64(const char* pszPath, struct stat64* pSt) { return stat(pszPath, reinterpret_cast<struct stat*>(pSt)); }
#endif

int remove(const char* pszPath)
{
    static auto real = Real<int (*)(const char*)>("remove");
    WritePath path(pszPath);
    return Logged("remove", path, real(path));
}

int rename(const char* pszOld, const char* pszNew)
{
    static auto real = Real<int (*)(const char*, const char*)>("rename");
    WritePath from(pszOld);
    return Logged("rename", from, real(from, WritePath(pszNew, true)));
}

int unlink(const char* pszPath)
{
    static auto real = Real<int (*)(const char*)>("unlink");
    WritePath path(pszPath);
    return Logged("unlink", path, real(path));
}

// The engine clears a file's read-only bit (chmod 0200) before deleting one (CExoResMan::RemoveFile,
// overwriting a save), so this has to reach the file remove will, or Windows refuses the delete.
#ifdef _WIN32
int chmod(const char* pszPath, int nMode)   // mingw-w64's <io.h> spelling
#else
int chmod(const char* pszPath, mode_t nMode)
#endif
{
    static auto real = Real<int (*)(const char*, decltype(nMode))>("chmod");
    return real(WritePath(pszPath), nMode);
}

int mkdir(const char* pszPath, mode_t nMode)
{
    static auto real = Real<int (*)(const char*, mode_t)>("mkdir");
    return real(WritePath(pszPath, true), nMode);
}

int rmdir(const char* pszPath)
{
    static auto real = Real<int (*)(const char*)>("rmdir");
    WritePath path(pszPath);
    return Logged("rmdir", path, real(path));
}

int chdir(const char* pszPath)
{
    static auto real = Real<int (*)(const char*)>("chdir");
    return real(HostPath(pszPath));
}

#ifndef K2P_MOBILE_HOST
// Routes the engine's SDL streams (all of KOTOR 1's CExoFile, KOTOR II's newest-save scan) through
// fopen above, so they see the same case-blind lookup, user directory and OBBs. SDL's own calls still
// reach libc directly on macOS, where only the executable's calls bind to these overrides. The mobile
// hosts have their own.
SDL_RWops* SDL_RWFromFile(const char* pszFile, const char* pszMode)
{
    static auto real = Real<SDL_RWops* (*)(const char*, const char*)>("SDL_RWFromFile");
    if (pszFile && pszMode)
        if (FILE* fp = fopen(pszFile, pszMode)) return SDL_RWFromFP(fp, SDL_TRUE);
    return real(pszFile, pszMode);
}
#endif

} // extern "C"
