#ifndef LIBCSTDLIB_H
#define LIBCSTDLIB_H
#include "k2stdint.h"

// The engine's <stdlib.h>, with the few <ctype.h> and <locale.h> functions it calls, declared once.
// Nounwind (throw()) where bionic's declarations are not, so this header comes after every system
// header in a TU (the exception specifications differ, an error in either order); port/libcbase.h
// explains the scheme.

#if defined(K2_HOST_BUILD)
// A host takes its own C library's declarations (Darwin's <ctype.h> defines tolower and its
// neighbours inline, with C++ linkage).
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>

// wctomb over K2WChar (k2stdint.h), the same in every host build: a code point up to 0xff is its own
// byte, anything above fails (-1). That is the "C" locale of Darwin (Aspyr's macOS build) and of the
// Windows CRT; glibc's "C" fails above 0x7f, and the engine's single-byte strings (cp1252 TLK text,
// the fonts) want the Latin-1 byte.
inline int wctomb(char* pDst, char32_t c)
{
    if (!pDst) return 0;
    if (c > 0xff) return -1;
    *pDst = static_cast<char>(c);
    return 1;
}
#else  // bionic's declarations, without its headers

#if !defined(_STDLIB_H)
extern "C" {
double atof(const char* s) throw();
unsigned long strtoul(const char* s, char** ppEnd, int nBase) throw();
void*  malloc(size_t n) throw();
void*  calloc(size_t nCount, size_t nSize) throw();
void*  realloc(void* p, size_t n) throw();
void   free(void* p) throw();
int    abs(int n) throw() __attribute__((const));
char*  getenv(const char* pszName) throw();
int    wctomb(char* pDst, wchar_t c) throw();
void   exit(int nCode) throw() __attribute__((noreturn));
void   abort() throw() __attribute__((noreturn));
}
#endif

// bionic's <ctype.h> has no guard macro; _CTYPE_U is its.
#if !defined(_CTYPE_U)
extern "C" {
int tolower(int c) throw();
int toupper(int c) throw();
int isalpha(int c) throw();
int isgraph(int c) throw();
}
#endif

#if !defined(_LOCALE_H_)
extern "C" char* setlocale(int nCategory, const char* pszLocale) throw();
#endif

#endif  // K2_HOST_BUILD

#endif // LIBCSTDLIB_H
