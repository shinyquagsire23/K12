#ifndef CRTAPI_H
#define CRTAPI_H
#include "k2stdint.h"
#include "port/libcbase.h"

// The C runtime functions the port layer answers (platforms/shim/shim_crt.cpp) that no system header
// declares the way the engine calls them, declared once for every engine TU.
//
// KOTOR II's library calls Aspyr's rand_wincompatible / srand_wincompatible, the MSVC generator
// (a 15-bit result) its saves and scripts expect; KOTOR 1's calls the C library's rand directly.
#if KOTOR1
inline int rand_wincompatible()
{
    return rand();
}
inline void srand_wincompatible(unsigned int nSeed)
{
    srand(nSeed);
}
#else
extern "C" int rand_wincompatible();
extern "C" void srand_wincompatible(unsigned int nSeed);
#endif

// The MSVC case-insensitive compare. KOTOR 1's library has no _stricmp; it calls the C library's
// strcasecmp, which bionic declares pure. Under cl.exe the CRT's <string.h> declares _stricmp and
// _strdup, with its annotations.
#if defined(_MSC_VER)
#include <string.h>
#endif
#if KOTOR1
#define _stricmp strcasecmp
#elif !defined(_MSC_VER)
extern "C" int _stricmp(const char* pszA, const char* pszB) throw();
#endif

// The length-limited compare; the port layer reads the count 32 bits wide. KOTOR 1 calls the C
// library's strncasecmp.
#if KOTOR1
#define _strnicmp strncasecmp
typedef size_t StrnicmpCount;
#else
extern "C" int _strnicmp(const char* pszA, const char* pszB, unsigned int n) throw();
typedef unsigned int StrnicmpCount;
#endif

// In-place case conversion and integer formatting. KOTOR 1's port declared its own with C++ linkage
// (platforms/k1/k1crt.cpp).
#if KOTOR1
char* _strlwr(char* s) throw();
char* _strupr(char* s) throw();
char* _itoa(int nValue, char* pszBuffer, int nRadix);
#else
extern "C" char* _strlwr(char* s) throw();
extern "C" char* _strupr(char* s) throw();
extern "C" char* _itoa(int nValue, char* pszBuffer, int nRadix);
#endif

#if !defined(_MSC_VER)
extern "C" char* _strdup(const char* pszText) throw();
#endif
extern "C" int _finite(double x) throw();
// MSVC's version answers -1 on truncation, and then writes no terminator.
extern "C" int _vsnprintf(char* pszBuffer, size_t nSize, const char* pszFormat, __builtin_va_list ap);

// The rename the save screens call, separators mapped (KOTOR 1: platforms/k1/k1port.cpp, C linkage;
// KOTOR II: the Android port's AndroidJNI.cpp, C++ linkage, under the external-storage directory).
#if KOTOR1
extern "C" int _rename(const char* pszOld, const char* pszNew);
#else
int _rename(const char* pszOld, const char* pszNew);
#endif

#if KOTOR1
// From KOTOR 1's port, the debug fopen, with the calling source file, function and line first, and
// _strdate with C++ linkage (k1port.cpp; it writes nothing).
extern "C" FILE* _fopen(const char* pszSrcFile, const char* pszFunction, int nLine, const char* pszPath, const char* pszMode);
void _strdate(char* pszDate);
#endif

#endif // CRTAPI_H
