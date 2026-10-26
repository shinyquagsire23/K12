// Entry points the engine calls by name that the macOS and Linux hosts have and Windows does not.
//
//   __sprintf_chk / __vsprintf_chk   bionic's (and glibc's, Darwin's) fortified sprintf, which the
//                                    engine's FORTIFY build lowered its calls to; mingw-w64 lacks
//                                    them. Semantics as compat/bionic_compat.c: abort on overflow.
//   glBindFramebuffer & co.          GL 3.0 framebuffer / renderbuffer calls the renderers make directly
//                                    (KOTOR II's aurglcore.cpp, KOTOR 1's bare GLES imports, k1gl.h);
//                                    opengl32.dll exports only GL 1.1, so each is looked up in the
//                                    current context, through the shim's lookup (the window's
//                                    framebuffer is the shim's own, shim/shim_glpresent.cpp).
#include <SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void* k2shim_gl_proc_c(const char* pszName);   // shim/shim_glpresent.cpp

int __vsprintf_chk(char* pszDst, int nFlag, size_t nDstLen, const char* pszFormat, va_list ap)
{
    (void)nFlag;
    int n = vsnprintf(pszDst, nDstLen, pszFormat, ap);
    if (n < 0 || (size_t)n >= nDstLen) {
        fprintf(stderr, "k2: FORTIFY: vsprintf: buffer overflow detected\n");
        abort();
    }
    return n;
}

int __sprintf_chk(char* pszDst, int nFlag, size_t nDstLen, const char* pszFormat, ...)
{
    va_list ap;
    va_start(ap, pszFormat);
    int n = __vsprintf_chk(pszDst, nFlag, nDstLen, pszFormat, ap);
    va_end(ap);
    return n;
}

#define K2_GL_PROC(ret, name, params)                                           \
    static ret (__stdcall* s_p) params;                                         \
    if (!s_p) *(void**)&s_p = k2shim_gl_proc_c(#name)

#define K2_GL_FORWARD(name, params, args)                                       \
    void name params                                                            \
    {                                                                           \
        K2_GL_PROC(void, name, params);                                         \
        s_p args;                                                               \
    }

#define K2_GL_FORWARD_RET(ret, name, params, args)                              \
    ret name params                                                             \
    {                                                                           \
        K2_GL_PROC(ret, name, params);                                          \
        return s_p args;                                                        \
    }

K2_GL_FORWARD(glBindFramebuffer, (unsigned int eTarget, unsigned int nFramebuffer), (eTarget, nFramebuffer))
K2_GL_FORWARD(glDeleteFramebuffers, (int nCount, const unsigned int* pnFramebuffers), (nCount, pnFramebuffers))
K2_GL_FORWARD(glGenFramebuffers, (int nCount, unsigned int* pnFramebuffers), (nCount, pnFramebuffers))
K2_GL_FORWARD(glFramebufferTexture2D,
              (unsigned int eTarget, unsigned int eAttachment, unsigned int eTexTarget, unsigned int nTexture, int nLevel),
              (eTarget, eAttachment, eTexTarget, nTexture, nLevel))
K2_GL_FORWARD(glGenRenderbuffers, (int nCount, unsigned int* pnRenderbuffers), (nCount, pnRenderbuffers))
K2_GL_FORWARD(glDeleteRenderbuffers, (int nCount, const unsigned int* pnRenderbuffers), (nCount, pnRenderbuffers))
K2_GL_FORWARD(glBindRenderbuffer, (unsigned int eTarget, unsigned int nRenderbuffer), (eTarget, nRenderbuffer))
K2_GL_FORWARD(glRenderbufferStorage, (unsigned int eTarget, unsigned int eFormat, int nWidth, int nHeight),
              (eTarget, eFormat, nWidth, nHeight))
K2_GL_FORWARD(glFramebufferRenderbuffer,
              (unsigned int eTarget, unsigned int eAttachment, unsigned int eRbTarget, unsigned int nRenderbuffer),
              (eTarget, eAttachment, eRbTarget, nRenderbuffer))
K2_GL_FORWARD_RET(unsigned int, glCheckFramebufferStatus, (unsigned int eTarget), (eTarget))
