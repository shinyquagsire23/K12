// Force-included (/FI) into every own-code TU of the cl.exe build (the host presets on Windows),
// and included by k2host_prelude.h for the engine. Empty for every other compiler, clang-cl
// included: those accept the GNU spellings below as they are.
//
// cl.exe has no __attribute__ and few of clang's builtins. The attributes the engine and the shim
// carry are codegen requests (noinline, always_inline, visibility, used, pure, const, flatten,
// format, no_stack_protector, noreturn, pass_object_size) or layout requests whose cl spelling is in
// the source itself (packed: #pragma pack; aligned: alignas; weak: one definition per build;
// gcc_struct and the GNU vector types: _MSC_VER branches), so the attribute is dropped. The builtins
// are the CRT functions they stand for.
#pragma once
#if defined(_MSC_VER) && !defined(__clang__)

#include <float.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#define __attribute__(x)

// GCC's predefined type and limit macros, which k2stdint.h and List.h's placement new use.
#define __SIZE_TYPE__ unsigned __int64
#define __INTPTR_TYPE__ __int64
#define __UINTPTR_TYPE__ unsigned __int64
#define __INT8_TYPE__ signed char
#define __INT16_TYPE__ short
#define __INT32_TYPE__ int
#define __INT64_TYPE__ long long
#define __UINT8_TYPE__ unsigned char
#define __UINT16_TYPE__ unsigned short
#define __UINT32_TYPE__ unsigned int
#define __UINT64_TYPE__ unsigned long long
#define __SIZEOF_LONG__ 4
#define __FLT_MAX__ 3.40282347e+38F

// The empty compiler barriers (`__asm__ __volatile__("" ::: "memory")`): cl has no inline asm.
#define __asm__
#define __volatile__(...) _ReadWriteBarrier()

#ifdef __cplusplus
extern "C" {
#endif
void* _ReturnAddress(void);
void _ReadWriteBarrier(void);
void __debugbreak(void);
void* __cdecl _alloca(size_t n);
// The CRT functions the builtins below stand for; a builtin's caller need not have declared them.
void* __cdecl memset(void* pDst, int c, size_t n);
void* __cdecl memcpy(void* pDst, const void* pSrc, size_t n);
int __cdecl strcmp(const char* a, const char* b);
int __cdecl strncmp(const char* a, const char* b, size_t n);
size_t __cdecl strlen(const char* s);
char* __cdecl strcpy(char* pDst, const char* pSrc);
char* __cdecl strcat(char* pDst, const char* pSrc);
int __cdecl abs(int n);
float __cdecl fabsf(float f);
float __cdecl sqrtf(float f);
float __cdecl ceilf(float f);
float __cdecl floorf(float f);
float __cdecl fminf(float a, float b);
float __cdecl fmaxf(float a, float b);
float __cdecl sinf(float f);
float __cdecl cosf(float f);
double __cdecl sin(double d);
double __cdecl cos(double d);
double __cdecl pow(double a, double b);
#ifdef __cplusplus
}
#endif
#pragma intrinsic(_ReturnAddress, _ReadWriteBarrier, __debugbreak, _alloca)

// A function rather than a macro: some callers put #if inside its argument list, which a macro
// argument may not hold.
static __forceinline long long __builtin_expect(long long x, long long expected) { (void)expected; return x; }

#define __builtin_va_list va_list
// vadefs.h's own names: some engine TUs define va_start as __builtin_va_start themselves.
#define __builtin_va_start(ap, last) __crt_va_start(ap, last)
#define __builtin_va_end(ap) __crt_va_end(ap)
#define __builtin_return_address(level) _ReturnAddress()
#define __builtin_trap() __debugbreak()
#define __builtin_debugtrap() __debugbreak()
// "Unknown" (type 0 and 1), which reduces the fortified calls below to the plain ones.
#define __builtin_object_size(p, type) SIZE_MAX

#define __builtin_memset memset
#define __builtin_memcpy memcpy
#define __builtin_strcmp strcmp
#define __builtin_strncmp strncmp
#define __builtin_strlen strlen
#define __builtin_strncpy strncpy
#define __builtin_strchr strchr
#define __builtin_strstr strstr
#define __builtin_snprintf snprintf
#define __builtin_vsnprintf vsnprintf
#define __builtin_abs abs
#define __builtin_fabsf fabsf
#define __builtin_sqrtf sqrtf
#define __builtin_ceilf ceilf
#define __builtin_floorf floorf
#define __builtin_fminf fminf
#define __builtin_fmaxf fmaxf
#define __builtin_sinf sinf
#define __builtin_cosf cosf
#define __builtin_sin sin
#define __builtin_cos cos
#define __builtin_pow pow
#define __builtin_isfinite(x) _finite(x)

// The fortified builtins become the plain call (cl checks no object size).
#define __builtin___strcpy_chk(d, s, size) strcpy(d, s)
#define __builtin___strcat_chk(d, s, size) strcat(d, s)
#define __builtin___memset_chk(d, c, n, size) memset(d, c, n)
#define __builtin___sprintf_chk(d, flag, size, ...) sprintf(d, __VA_ARGS__)
#define __builtin___vsprintf_chk(d, flag, size, fmt, ap) vsprintf(d, fmt, ap)

#endif  // _MSC_VER && !__clang__
