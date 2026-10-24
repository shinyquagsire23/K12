#ifndef LIBCSTRING_H
#define LIBCSTRING_H
#include "k2stdint.h"

// The engine's <string.h>, declaring once the string and memory functions it calls.
// Nounwind (throw()) where bionic's declarations are not, so this header comes after every system
// header in a TU (the exception specifications differ, an error in either order); port/libcbase.h
// explains the scheme.

#if defined(K2_HOST_BUILD)
// A host takes its own C library's declarations; bionic's fortified strchr is the port's.
#include <string.h>
#include <wchar.h>
extern "C" char* __strchr_chk(const char* s, int c, size_t nObjSize);

// wcs* functions over K2WChar (k2stdint.h). The C library's take its own wchar_t, 16 bits on
// Win64, so a host's engine strings go through these overloads in every build.
inline size_t wcslen(const char32_t* s)
{
    const char32_t* p = s;
    while (*p) ++p;
    return static_cast<size_t>(p - s);
}

inline char32_t* wcscpy(char32_t* dst, const char32_t* src)
{
    char32_t* p = dst;
    while ((*p++ = *src++) != 0) {}
    return dst;
}

inline char32_t* wcsncpy(char32_t* dst, const char32_t* src, size_t n)
{
    size_t i = 0;
    for (; i < n && src[i]; ++i) dst[i] = src[i];
    for (; i < n; ++i) dst[i] = 0;
    return dst;
}
#else  // bionic's declarations, without its headers

#if !defined(_STRING_H)
extern "C" {
size_t strlen(const char* s) throw();
char*  strcpy(char* dst, const char* src) throw();
char*  strncpy(char* dst, const char* src, size_t n) throw();
char*  strcat(char* dst, const char* src) throw();
int    strcmp(const char* a, const char* b) throw();
int    strncmp(const char* a, const char* b, size_t n) throw();
char*  strchr(const char* s, int c) throw();
char*  strrchr(const char* s, int c) throw();
char*  strstr(const char* s, const char* sub) throw();
char*  strtok(char* s, const char* delim) throw();
size_t strcspn(const char* s, const char* reject) throw();
void*  memcpy(void* dst, const void* src, size_t n) throw();
void*  memmove(void* dst, const void* src, size_t n) throw();
void*  memset(void* dst, int c, size_t n) throw();
int    memcmp(const void* a, const void* b, size_t n) throw();
size_t wcslen(const wchar_t* s) throw();

// The _FORTIFY_SOURCE=2 entry points bionic's <string.h> declares; the engine calls them by name,
// with the object size last.
size_t __strlen_chk(const char* s, size_t nObjSize) throw();
char*  __strchr_chk(const char* s, int c, size_t nObjSize) throw();
}
#endif

#endif  // K2_HOST_BUILD

#endif // LIBCSTRING_H
