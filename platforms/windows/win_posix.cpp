// The few POSIX / BSD libc functions the engine and the shim call that the UCRT does not have
// (declared in compat/include-mingw/).
#include <stdint.h>
#include <windows.h>

#include <ctype.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <unistd.h>


extern "C" {

char* strcasestr(const char* pszHaystack, const char* pszNeedle)
{
    for (; *pszHaystack; pszHaystack++) {
        const char* a = pszHaystack;
        const char* b = pszNeedle;
        while (*a && *b && tolower(static_cast<unsigned char>(*a)) == tolower(static_cast<unsigned char>(*b))) a++, b++;
        if (!*b) return const_cast<char*>(pszHaystack);
    }
    return *pszNeedle ? 0 : const_cast<char*>(pszHaystack);
}

long long pread(int nFd, void* pBuf, size_t nCount, long long nOffset)
{
    HANDLE h = reinterpret_cast<HANDLE>(_get_osfhandle(nFd));
    if (h == INVALID_HANDLE_VALUE) return -1;
    OVERLAPPED ov = {};
    ov.Offset = static_cast<DWORD>(nOffset);
    ov.OffsetHigh = static_cast<DWORD>(nOffset >> 32);
    DWORD nRead = 0;
    if (!ReadFile(h, pBuf, static_cast<DWORD>(nCount), &nRead, &ov))
        return GetLastError() == ERROR_HANDLE_EOF ? 0 : -1;
    return nRead;
}

size_t strlcpy(char* pszDst, const char* pszSrc, size_t nSize)
{
    size_t n = strlen(pszSrc);
    if (nSize) {
        size_t c = n < nSize - 1 ? n : nSize - 1;
        memcpy(pszDst, pszSrc, c);
        pszDst[c] = 0;
    }
    return n;
}

size_t strlcat(char* pszDst, const char* pszSrc, size_t nSize)
{
    size_t d = strnlen(pszDst, nSize);
    return d == nSize ? nSize + strlen(pszSrc) : d + strlcpy(pszDst + d, pszSrc, nSize - d);
}

int setenv(const char* pszName, const char* pszValue, int bOverwrite)
{
    if (!bOverwrite && getenv(pszName)) return 0;
    return _putenv_s(pszName, pszValue) == 0 ? 0 : -1;
}

int unsetenv(const char* pszName) { return _putenv_s(pszName, "") == 0 ? 0 : -1; }

long sysconf(int nName)
{
    if (nName != _SC_NPROCESSORS_ONLN) return -1;
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return static_cast<int64_t>(si.dwNumberOfProcessors);
}

int statvfs(const char* pszPath, struct statvfs* pBuf)
{
    ULARGE_INTEGER avail, total, free;
    if (!GetDiskFreeSpaceExA(pszPath && *pszPath ? pszPath : ".", &avail, &total, &free)) return -1;
    pBuf->f_bsize = pBuf->f_frsize = 4096;
    pBuf->f_blocks = total.QuadPart / 4096;
    pBuf->f_bfree = free.QuadPart / 4096;
    pBuf->f_bavail = avail.QuadPart / 4096;
    return 0;
}

FILE* fmemopen(void* pBuf, size_t nSize, const char*)
{
    FILE* f = tmpfile();
    if (!f) return 0;
    if (fwrite(pBuf, 1, nSize, f) != nSize) {
        fclose(f);
        return 0;
    }
    rewind(f);
    return f;
}

} // extern "C"
