// The fixed-width integers, for every build of this tree.
//
// A width is spelled here by its width: int64_t and uint64_t are `long` and `unsigned long` on LP64
// targets and stay 64 bits on Win64, where `long` is only 32.
//
// The engine cannot include the platform <stdint.h>, because its <sys/cdefs.h> brings CRT
// prototypes along whose exception specifications clash with the ones engine translation units
// declare themselves. The compiler's own type macros are the same types, so the two may meet in one
// TU.
#ifndef K2STDINT_H
#define K2STDINT_H

#include "k2layout.h"

// K2_UNUSED(param): a parameter the body, or this #if arm of it, never reads. The signature keeps
// it; the macro is the tree's one spelling of "ignored", so `grep K2_UNUSED` lists them. A
// discarded read of a named object folds away at any optimisation level.
#define K2_UNUSED(x) static_cast<void>(x)

// Suppression and portability annotations. None of them changes the engine's code.
//
// K2_UNSET: `int n K2_UNSET;` -- a local the engine leaves uninitialised on paths that never read it,
// because the analysers cannot correlate a guard with the flag, mask or switch that set it. The
// engine build declares it bare; a host build value-initialises it.
//
// K2_ASSUME(cond): an invariant the static analyser cannot see, such as a singleton that is never
// null. Only the analyser reads it; it must not become __builtin_assume, which would change the
// code.
//
// K2_LEAF: `class T K2_LEAF : public B` -- a class nothing derives from, which would otherwise draw
// clang -Wdelete-non-abstract-non-virtual-dtor. `final` in a host build, empty otherwise, since
// `final` would turn the engine's virtual calls through T* into direct ones.
//
// K2_OFFSETOF(C, m): offsetof over a class that is not standard-layout, such as one with a vtable or
// with members in both a base and a derived class. Every compiler that builds this tree gives the
// member's offset in this build's layout, which is what the field tables and layout pins want; the
// macro silences -Winvalid-offsetof for these only.
#if defined(__clang__)
#define K2_OFFSETOF(C, m)                                                                                   \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Winvalid-offsetof\"")             \
    __builtin_offsetof(C, m) _Pragma("clang diagnostic pop")
#else
#define K2_OFFSETOF(C, m) offsetof(C, m)
#endif
#if defined(K2_HOST_BUILD)
#define K2_UNSET = {}
#define K2_LEAF final
#else
#define K2_UNSET
#define K2_LEAF
#endif
#if defined(_MSC_VER) && defined(_PREFAST_)
#include <sal.h>
#define K2_ASSUME(cond) _Analysis_assume_(cond)
#else
#define K2_ASSUME(cond) static_cast<void>(sizeof(!(cond)))   // type-checked, never evaluated
#endif

#if defined(K2_HOST_BUILD)
#include <stddef.h>
#include <stdint.h>
#else
typedef __SIZE_TYPE__ size_t;
typedef __INT8_TYPE__ int8_t;
typedef __INT16_TYPE__ int16_t;
typedef __INT32_TYPE__ int32_t;
typedef __INT64_TYPE__ int64_t;
typedef __UINT8_TYPE__ uint8_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;
typedef __INTPTR_TYPE__ intptr_t;
typedef __UINTPTR_TYPE__ uintptr_t;
#endif

// int64_t is `long` on the LP64 targets and `long long` on Darwin and Win64. Where the engine
// overloads on both (ByteSwap), the int64_t overload is a function of its own only when this is
// defined.
#if __SIZEOF_LONG__ == 8 && !defined(__APPLE__)
#define K2_INT64_IS_LONG 1
#endif

// The engine's wide character is a 32-bit code unit. Win64's wchar_t is a 16-bit UTF-16 unit, so a
// host spells it char32_t, which is 32 bits under every compiler; port/libcstring.h and
// port/libcstdlib.h give it the wcs* functions the engine calls. The engine build keeps wchar_t. A
// Windows API that takes WCHAR stays WCHAR.
#if defined(__cplusplus)
#if defined(K2_HOST_BUILD)
typedef char32_t K2WChar;
#else
typedef wchar_t K2WChar;
#endif
static_assert(sizeof(K2WChar) == 4, "the engine's wide character is a 32-bit code unit");
#endif

#endif
