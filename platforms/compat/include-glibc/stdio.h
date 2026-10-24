// glibc's <stdio.h> plus the guard bionic and Darwin define (Linux host builds only).
//
// The engine's headers keep the C file handle opaque and forward-declare `struct FILE` only while
// `_STDIO_H_` (bionic's and Darwin's include guard) is undefined; glibc's guard is `_STDIO_H`. The
// handle's own spelling, `struct __sFILE`, is mapped to glibc's `_IO_FILE` by k2host_prelude.h.
#include_next <stdio.h>
#ifndef _STDIO_H_
#define _STDIO_H_ 1
#endif
