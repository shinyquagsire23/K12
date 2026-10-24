// Force-included (-include) into every engine TU of the host build, never into the hybrid build.
//
// Deliberately includes no C library header: engine TUs take theirs from src/port/libc*.h, whose
// groups step aside for a C library header seen first.
#pragma once
#include "k2_compiler.h"
#include <stdint.h>
#include <stddef.h>
#if defined(__linux__) && !defined(__ANDROID__)
// The engine spells the C file handle `struct __sFILE` (bionic's and Darwin's tag, and the binary's
// mangling); glibc's is `struct _IO_FILE`. compat/include-glibc/stdio.h has the other half.
#define __sFILE _IO_FILE
#endif
#if defined(_WIN32)
// The same for mingw-w64's UCRT headers, whose handle is `struct _iobuf` (compat/include-mingw/stdio.h).
#define __sFILE _iobuf
#endif
#ifdef __cplusplus
extern "C" {
#endif
// bionic-only fortify entry points the engine calls by name; defined in compat/bionic_compat.c.
size_t __strlen_chk(const char* s, size_t s_len);
#ifdef __cplusplus
}
#endif
