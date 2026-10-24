// mingw-w64's <stdio.h> plus the guard bionic and Darwin define (Windows builds only).
//
// The engine's headers keep the C file handle opaque and forward-declare `struct FILE` only while
// `_STDIO_H_` (bionic's and Darwin's include guard) is undefined; mingw-w64's is `_INC_STDIO`. The
// handle's own spelling, `struct __sFILE`, is mapped to `struct _iobuf` by k2host_prelude.h.
#include_next <stdio.h>
#ifndef _STDIO_H_
#define _STDIO_H_ 1
#endif
#ifndef K2_MINGW_STDIO_H
#define K2_MINGW_STDIO_H
// POSIX's memory stream, which the UCRT does not have: a temporary file holding a copy of the bytes
// (windows/win_posix.cpp). Read-only use only (the shim's embedded shaders).
#ifdef __cplusplus
extern "C"
#endif
FILE* fmemopen(void* pBuf, size_t nSize, const char* pszMode);
#endif
