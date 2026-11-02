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
#if !defined(__ANDROID_API__) || __ANDROID_API__ >= 21
int    rand();
void   srand(unsigned int nSeed);
#endif
int    system(const char* pszCommand);
void   qsort(void* pBase, size_t nCount, size_t nSize, int (*pfnCompare)(const void*, const void*));
}
#endif

// Below API 21 bionic's <stdlib.h> defines rand, srand, atof and their neighbours inline over
// lrand48, srand48 and strtod (android/legacy_stdlib_inlines.h): KOTOR 1's armv7 library is API 16
// and imports those three, never rand or atof. This is that header's API 16 content under its own
// guard, with its declarations, so whichever of the two a TU reaches first is the one that defines
// them.
#if defined(__ANDROID_API__) && __ANDROID_API__ < 21 && !defined(_ANDROID_LEGACY_STDLIB_INLINES_H_)
#define _ANDROID_LEGACY_STDLIB_INLINES_H_
struct __locale_t;
typedef struct __locale_t* locale_t;
extern "C" {
int*   __errno() __attribute__((const));
long   strtol(const char* s, char** ppEnd, int nBase);
double strtod(const char* s, char** ppEnd);
long   lrand48();
void   srand48(long nSeed);
#if __ANDROID_API__ < 19
static inline int abs(int n) { return (n < 0) ? -n : n; }
static inline long labs(long n) { return (n < 0L) ? -n : n; }
static inline long long llabs(long long n) { return (n < 0LL) ? -n : n; }
#endif
static inline float strtof(const char* s, char** ppEnd) {
    double d = strtod(s, ppEnd);
    if (d > __FLT_MAX__) {
        *__errno() = 34;  // ERANGE
        return __builtin_huge_valf();
    } else if (d < -__FLT_MAX__) {
        *__errno() = 34;
        return -__builtin_huge_valf();
    }
    return static_cast<float>(d);
}
static inline double atof(const char* s) { return strtod(s, 0); }
static inline int rand() { return static_cast<int>(lrand48()); }
static inline void srand(unsigned int nSeed) { srand48(nSeed); }
static inline long random() { return lrand48(); }
static inline void srandom(unsigned int nSeed) { srand48(nSeed); }
static inline int grantpt(int) { return 0; }
static inline double strtod_l(const char* s, char** ppEnd, locale_t) { return strtod(s, ppEnd); }
static inline float strtof_l(const char* s, char** ppEnd, locale_t) { return strtof(s, ppEnd); }
static inline long strtol_l(const char* s, char** ppEnd, int nBase, locale_t) { return strtol(s, ppEnd, nBase); }
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

#if !defined(K2_HOST_BUILD) && !defined(_FORTIFY_SOURCE)
// A build without _FORTIFY_SOURCE (K1's armv7 libKOTOR.so imports no *_chk function) makes the plain
// call wherever the fortified build calls a checked entry point; the engine spells those by name.
#define K2_NO_FORTIFY 1
#include "port/libcstring.h"
extern "C" int sprintf(char* pszDst, const char* pszFormat, ...);
#define __strlen_chk(s, nObjSize) strlen(s)
#define __strchr_chk(s, c, nObjSize) strchr(s, c)
#define __strcpy_chk(dst, src, nDstSize) strcpy(dst, src)
#define __strcat_chk(dst, src, nDstSize) strcat(dst, src)
#define __strncpy_chk(dst, src, n, nDstSize) strncpy(dst, src, n)
#define __strncpy_chk2(dst, src, n, nDstSize, nSrcSize) strncpy(dst, src, n)
#define __memcpy_chk(dst, src, n, nDstSize) memcpy(dst, src, n)
#define __vsprintf_chk(pszDst, nFlag, nDstSize, pszFormat, ap) vsprintf(pszDst, pszFormat, ap)
#define __builtin___strcpy_chk(dst, src, nDstSize) strcpy(dst, src)
#define __builtin___strcat_chk(dst, src, nDstSize) strcat(dst, src)
#define __builtin___strncpy_chk(dst, src, n, nDstSize) strncpy(dst, src, n)
#define __builtin___memcpy_chk(dst, src, n, nDstSize) memcpy(dst, src, n)
#define __builtin___memset_chk(dst, c, n, nDstSize) memset(dst, c, n)
#define __builtin___sprintf_chk(pszDst, nFlag, nDstSize, ...) sprintf(pszDst, __VA_ARGS__)
#define __builtin___vsprintf_chk(pszDst, nFlag, nDstSize, pszFormat, ap) vsprintf(pszDst, pszFormat, ap)
// The TUs' own sprintf_s shims stand for the fortified sprintf.
#define sprintf_s(pszDst, nDstSize, ...) sprintf(pszDst, __VA_ARGS__)
#endif

#endif // LIBCBASE_H
