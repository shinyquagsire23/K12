// bionic-only libc entry points the engine calls by name (its FORTIFY build lowered string calls
// to them). macOS libc has the __*_chk family but not these three. Semantics follow bionic's
// libc/bionic/fortify.cpp: abort on overflow, otherwise behave like the plain call.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void k2_fortify_fail(const char* what)
{
    fprintf(stderr, "k2: FORTIFY: %s: buffer overflow detected\n", what);
    abort();
}

size_t __strlen_chk(const char* s, size_t s_len)
{
    size_t n = strlen(s);
    if (n >= s_len)
        k2_fortify_fail("strlen");
    return n;
}

char* __strchr_chk(const char* s, int c, size_t s_len)
{
    for (size_t i = 0; i < s_len; ++i) {
        if (s[i] == (char)c)
            return (char*)(s + i);
        if (s[i] == 0)
            return NULL;
    }
    k2_fortify_fail("strchr");
    return NULL;
}

char* __strncpy_chk2(char* dst, const char* src, size_t n, size_t dst_len, size_t src_len)
{
    if (n > dst_len)
        k2_fortify_fail("strncpy");
    // bionic also checks the SOURCE extent: it may not read past src_len before finding a NUL
    size_t i = 0;
    for (; i < n; ++i) {
        if (i >= src_len)
            k2_fortify_fail("strncpy (source)");
        dst[i] = src[i];
        if (src[i] == 0)
            break;
    }
    for (; i < n; ++i)
        dst[i] = 0;
    return dst;
}
