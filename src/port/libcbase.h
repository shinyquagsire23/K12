#ifndef LIBCBASE_H
#define LIBCBASE_H
#include "k2stdint.h"

// The C library functions the engine calls, declared once. Under bionic the engine compiles without
// the C library's headers: -fno-builtin keeps every call opaque, and bionic's declarations would
// bring back the purity attributes (and the _FORTIFY_SOURCE inline wrappers) that let the compiler
// fold or merge the calls. A TU that does include a C library header, directly or through libc++,
// gets that header's declarations instead: each group steps aside when bionic's header of the same
// name has been seen. Host builds take their own C library's headers, and declare only what those
// lack.
//
// This header declares exactly as bionic does (no exception specification; bionic's purity), plus
// the functions no header declares, so it may come before or after any system header.
// port/libcstring.h, port/libcmath.h and port/libcstdlib.h declare the ones the engine calls as
// nounwind (throw()) where bionic's are not. Clang rejects the differing exception specification in
// either order, so a TU includes those after every system header. Their specifications follow
// glibc's __THROW: a function that can neither call back into user code nor be a thread
// cancellation point is throw().

#if defined(K2_HOST_BUILD)
// A host needs no such constraint, and glibc's declarations differ from bionic's (__THROW on rand,
// atoi, strcasecmp, snprintf ...; sscanf behind an asm label), so it takes its own C library's.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if __has_include(<strings.h>)
#include <strings.h>
#endif
extern "C" {
char* __strncpy_chk2(char* dst, const char* src, size_t n, size_t nDstSize, size_t nSrcSize);
#if defined(__GLIBC__)
void  sincosf(float x, float* pSin, float* pCos) throw();
#else
void  sincosf(float x, float* pSin, float* pCos);
#endif
}
#else  // bionic's declarations, without its headers

#if !defined(_STDIO_H_)
// bionic's handle; the tag name fixes the mangled form of FILE* parameters.
typedef struct __sFILE FILE;
extern "C" {
FILE*  fopen(const char* pszPath, const char* pszMode);
int    fclose(FILE* pFile);
size_t fread(void* pDst, size_t nSize, size_t nCount, FILE* pFile);
size_t fwrite(const void* pSrc, size_t nSize, size_t nCount, FILE* pFile);
int    fseek(FILE* pFile, long nOffset, int nWhence);
long   ftell(FILE* pFile);
int    fflush(FILE* pFile);
char*  fgets(char* pDst, int nMax, FILE* pFile);
int    fputs(const char* s, FILE* pFile);
int    fputc(int c, FILE* pFile);
int    fprintf(FILE* pFile, const char* pszFormat, ...);
int    printf(const char* pszFormat, ...);
int    puts(const char* s);
int    putchar(int c);
int    snprintf(char* pszDst, size_t nSize, const char* pszFormat, ...);
int    vsprintf(char* pszDst, const char* pszFormat, __builtin_va_list ap);
int    vsnprintf(char* pszDst, size_t nSize, const char* pszFormat, __builtin_va_list ap);
int    sscanf(const char* s, const char* pszFormat, ...);
int    rename(const char* pszOld, const char* pszNew);
int    fileno(FILE* pFile);
}
#endif

#if !defined(_STDLIB_H)
extern "C" {
// <sys/cdefs.h> declares atoi itself (bits/get_device_api_level_inlines.h) in every TU that sees any
// bionic header, pure.
int    atoi(const char* s);
int    rand();
void   srand(unsigned int nSeed);
int    system(const char* pszCommand);
void   qsort(void* pBase, size_t nCount, size_t nSize, int (*pfnCompare)(const void*, const void*));
}
#endif

#if !defined(_MATH_H_)
// A BSD extension. Bionic's own redeclaration of it is not allowed to drop a throw().
extern "C" void sincosf(float x, float* pSin, float* pCos);
#endif

#if !defined(_STRING_H)
// The fortified strncpy (bits/fortify/string.h), whose last two arguments are the destination's and
// the source's object sizes.
extern "C" char* __strncpy_chk2(char* dst, const char* src, size_t n, size_t nDstSize, size_t nSrcSize);
#endif

// <strings.h> (which <string.h> includes) has no guard macro of its own; bzero is its.
#if !defined(bzero)
// bionic's own declarations, purity included.
extern "C" {
int strcasecmp(const char* a, const char* b) __attribute__((pure));
int strncasecmp(const char* a, const char* b, size_t n) __attribute__((pure));
}
#endif

#endif  // K2_HOST_BUILD

// The fortified entry points bionic leaves to the compiler's __builtin___*_chk (no header declares
// them), which the engine calls by name: the destination's object size comes last, and
// __vsprintf_chk's nFlag is 0.
extern "C" {
char*  __strcpy_chk(char* dst, const char* src, size_t nDstSize) throw();
char*  __strcat_chk(char* dst, const char* src, size_t nDstSize) throw();
char*  __strncpy_chk(char* dst, const char* src, size_t n, size_t nDstSize) throw();
void*  __memcpy_chk(void* dst, const void* src, size_t n, size_t nDstSize) throw();
int    __vsprintf_chk(char* pszDst, int nFlag, size_t nDstSize, const char* pszFormat,
                      __builtin_va_list ap);
}

#endif // LIBCBASE_H
