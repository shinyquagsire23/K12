// The CRT's <string.h> plus BSD's strlcpy / strlcat and strcasestr, which Darwin, glibc and bionic
// have (Windows builds only: windows/win_posix.cpp). Reaches libc++'s wrapper first, then
// mingw_crt.py's copy.
#include_next <string.h>
#ifndef K2_MINGW_STRING_H
#define K2_MINGW_STRING_H
#ifdef __cplusplus
extern "C" {
#endif
size_t strlcpy(char* pszDst, const char* pszSrc, size_t nSize);
size_t strlcat(char* pszDst, const char* pszSrc, size_t nSize);
char* strcasestr(const char* pszHaystack, const char* pszNeedle);
#ifdef __cplusplus
}
#endif
#endif
