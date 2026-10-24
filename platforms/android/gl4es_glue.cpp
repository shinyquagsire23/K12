// The engine's GL 1.x/ARB surface on Android and iOS is gl4es over the GLES 2 context SDL creates.
//
// The shim is written against desktop GL: immediate mode, the matrix stacks, client arrays, GLSL 1.20
// with the fixed-function built-ins. gl4es provides all of that, built static with NOEGL/NO_LOADER,
// so the shim drives it by hand:
//   * every GL entry point the shim, the movie player and the engine look up comes from
//     gl4es_GetProcAddress through the shim's proc table, never from the GLES driver directly --
//     gl4es keeps its own copy of the state and must see every call;
//   * gl4es reaches the driver through SDL_GL_GetProcAddress, and is initialized once the first
//     context is current;
//   * gl4es_pre_swap() runs before each SDL_GL_SwapWindow (it flushes batched draws).
#include "k2shim.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gl4esinit.h>
#ifdef K2P_IOS
#include <SDL_syswm.h>
#include <dlfcn.h>
#endif

extern "C" void gl4es_pre_swap(void);

namespace {
#ifdef K2P_IOS
// iOS: SDL_GL_GetProcAddress is dlsym(RTLD_DEFAULT), which finds the executable's own glGetString,
// glBindTexture ... first -- the shim's and gl4es's exported entry points, not the driver's. gl4es
// must reach OpenGLES.framework itself.
//
// And EAGL has no default framebuffer: SDL's view draws into a framebuffer object of its own, so the
// driver's framebuffer 0 is nothing on screen. gl4es binds 0 for "the window" (its main-FBO mode
// off), so its glBindFramebuffer reaches the driver through BindFramebuffer, which binds SDL's.
unsigned int s_nViewFramebuffer, s_nViewColorbuffer;
void (*s_pBindFramebuffer)(unsigned int, unsigned int);

void BindFramebuffer(unsigned int target, unsigned int framebuffer)
{
    s_pBindFramebuffer(target, framebuffer ? framebuffer : s_nViewFramebuffer);
}

// K2_GL_TRACE: during the traced frame (k2shim_gl_tracing) the driver calls gl4es makes around a draw
// are checked against the DRIVER's error state, which gl4es does not report, and a failing one is
// logged with its arguments.
unsigned int (*s_pDriverGetError)(void);
void DriverCheck(const char* pszCall)
{
    if (!k2shim_gl_tracing || !s_pDriverGetError) return;
    unsigned int e = s_pDriverGetError();
    static const bool s_bAll = getenv("K2_GL_TRACE_DRIVER") != 0;   // every wrapped call, not just failures
    if (e) fprintf(stderr, "k2: trace:   DRIVER 0x%x after %s\n", e, pszCall);
    else if (s_bAll) fprintf(stderr, "k2: trace:   driver %s\n", pszCall);
}
#define K2_DRV(ret, name, params, args, fmt, ...)                                       \
    ret (*s_p_##name) params;                                                          \
    ret W_##name params                                                                \
    {                                                                                  \
        if (k2shim_gl_tracing && s_pDriverGetError) s_pDriverGetError();               \
        s_p_##name args;                                                               \
        if (k2shim_gl_tracing && s_pDriverGetError) {                                  \
            char sz[160];                                                              \
            snprintf(sz, sizeof sz, #name "(" fmt ")", __VA_ARGS__);                   \
            DriverCheck(sz);                                                           \
        }                                                                              \
    }
K2_DRV(void, glUseProgram, (unsigned int a), (a), "%u", a)
K2_DRV(void, glUniform1i, (int l, int v), (l, v), "%d, %d", l, v)
K2_DRV(void, glUniform1f, (int l, float v), (l, v), "%d, %g", l, static_cast<double>(v))
K2_DRV(void, glUniform1iv, (int l, int n, const int* v), (l, n, v), "%d, %d", l, n)
K2_DRV(void, glUniform1fv, (int l, int n, const float* v), (l, n, v), "%d, %d", l, n)
K2_DRV(void, glUniform2fv, (int l, int n, const float* v), (l, n, v), "%d, %d", l, n)
K2_DRV(void, glUniform3fv, (int l, int n, const float* v), (l, n, v), "%d, %d", l, n)
K2_DRV(void, glUniform4fv, (int l, int n, const float* v), (l, n, v), "%d, %d", l, n)
K2_DRV(void, glUniformMatrix4fv, (int l, int n, unsigned char t, const float* v), (l, n, t, v), "%d, %d, %d", l, n, t)
K2_DRV(void, glUniformMatrix3fv, (int l, int n, unsigned char t, const float* v), (l, n, t, v), "%d, %d, %d", l, n, t)
K2_DRV(void, glDrawElements, (unsigned int m, int n, unsigned int t, const void* i), (m, n, t, i), "%u, %d, %x, %p", m, n, t, i)
K2_DRV(void, glDrawArrays, (unsigned int m, int f, int n), (m, f, n), "%u, %d, %d", m, f, n)
K2_DRV(void, glVertexAttribPointer, (unsigned int i, int s, unsigned int t, unsigned char n, int st, const void* p),
       (i, s, t, n, st, p), "%u, %d, %x, %d, %d, %p", i, s, t, n, st, p)
K2_DRV(void, glEnableVertexAttribArray, (unsigned int i), (i), "%u", i)
K2_DRV(void, glDisableVertexAttribArray, (unsigned int i), (i), "%u", i)
K2_DRV(void, glBindBuffer, (unsigned int t, unsigned int b), (t, b), "%x, %u", t, b)
K2_DRV(void, glBufferData, (unsigned int t, long n, const void* d, unsigned int u), (t, n, d, u), "%x, %ld", t, n)
K2_DRV(void, glBufferSubData, (unsigned int t, long o, long n, const void* d), (t, o, n, d), "%x, %ld, %ld", t, o, n)
K2_DRV(void, glBindTexture, (unsigned int t, unsigned int x), (t, x), "%x, %u", t, x)
K2_DRV(void, glActiveTexture, (unsigned int t), (t), "%x", t)
K2_DRV(void, glVertexAttrib4f, (unsigned int i, float a, float b, float c, float d), (i, a, b, c, d), "%u", i)
#undef K2_DRV

void* DriverProc(const char* pszName)
{
    static void* s_pLib = dlopen("/System/Library/Frameworks/OpenGLES.framework/OpenGLES", RTLD_LAZY | RTLD_LOCAL);
    void* p = s_pLib ? dlsym(s_pLib, pszName) : 0;
    static const bool s_bTrace = getenv("K2_GL_TRACE") != 0;
    if (s_bTrace && p) {
        if (!s_pDriverGetError) s_pDriverGetError = reinterpret_cast<unsigned int (*)(void)>(dlsym(s_pLib, "glGetError"));
#define K2_W(name) if (strcmp(pszName, #name) == 0) { s_p_##name = reinterpret_cast<decltype(s_p_##name)>(p); return reinterpret_cast<void*>(W_##name); }
        K2_W(glUseProgram) K2_W(glUniform1i) K2_W(glUniform1f) K2_W(glUniform1iv) K2_W(glUniform1fv)
        K2_W(glUniform2fv) K2_W(glUniform3fv) K2_W(glUniform4fv) K2_W(glUniformMatrix4fv)
        K2_W(glUniformMatrix3fv) K2_W(glDrawElements) K2_W(glDrawArrays) K2_W(glVertexAttribPointer)
        K2_W(glEnableVertexAttribArray) K2_W(glDisableVertexAttribArray) K2_W(glBindBuffer)
        K2_W(glBufferData) K2_W(glBufferSubData) K2_W(glBindTexture) K2_W(glActiveTexture)
        K2_W(glVertexAttrib4f)
#undef K2_W
    }
    if (p && strcmp(pszName, "glBindFramebuffer") == 0) {
        s_pBindFramebuffer = reinterpret_cast<void (*)(unsigned int, unsigned int)>(p);
        return reinterpret_cast<void*>(BindFramebuffer);
    }
    return p;
}

void ReadViewFramebuffer()
{
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    SDL_Window* w = SDL_GL_GetCurrentWindow();
    if (w && SDL_GetWindowWMInfo(w, &info) && info.subsystem == SDL_SYSWM_UIKIT) {
        s_nViewFramebuffer = info.info.uikit.framebuffer;
        s_nViewColorbuffer = info.info.uikit.colorbuffer;
    }
    fprintf(stderr, "k2: iOS view framebuffer %u, colour renderbuffer %u\n", s_nViewFramebuffer,
            s_nViewColorbuffer);
}

// SDL's swap presents whatever renderbuffer the driver has bound (anything that binds another one is
// expected to rebind viewRenderbuffer), and gl4es leaves the engine's own bound -- a depth buffer,
// presented as a black screen. Bind the view's for the swap, then give gl4es back its binding, which
// it caches.
int s_nSavedRenderbuffer;

void BindViewColorbuffer()
{
    void (*getIntegerv)(unsigned int, int*) = reinterpret_cast<void (*)(unsigned int, int*)>(DriverProc("glGetIntegerv"));
    void (*bindRenderbuffer)(unsigned int, unsigned int) =
        reinterpret_cast<void (*)(unsigned int, unsigned int)>(DriverProc("glBindRenderbuffer"));
    getIntegerv(0x8CA7, &s_nSavedRenderbuffer);   // GL_RENDERBUFFER_BINDING
    bindRenderbuffer(0x8D41, s_nViewColorbuffer);  // GL_RENDERBUFFER
}

void RestoreRenderbuffer()
{
    (reinterpret_cast<void (*)(unsigned int, unsigned int)>(DriverProc("glBindRenderbuffer")))(0x8D41, static_cast<unsigned int>(s_nSavedRenderbuffer));
}
#endif

void MainFbSize(int* pnWidth, int* pnHeight)
{
    SDL_Window* w = SDL_GL_GetCurrentWindow();
    if (w) SDL_GL_GetDrawableSize(w, pnWidth, pnHeight);
}
} // namespace

void k2android_gl_context_created()
{
    static bool s_bDone;
    if (s_bDone) return;
    s_bDone = true;
#ifdef K2P_IOS
    // Apple's GLES 2 has limited NPOT only: a non-power-of-two texture with mipmap filtering is
    // incomplete and samples black (the touch UI's tutorial and help overlays, the chargen head's
    // render target). gl4es's forced-NPOT mode drops the mipmaps of NPOT textures instead.
    setenv("LIBGL_FORCENPOT", "1", 0);
    ReadViewFramebuffer();
    set_getprocaddress(DriverProc);
#else
    set_getprocaddress(static_cast<void* (*)(const char*)>(SDL_GL_GetProcAddress));
#endif
    set_getmainfbsize(MainFbSize);
    initialize_gl4es();
}

void k2android_gl_pre_swap()
{
    gl4es_pre_swap();
#ifdef K2P_IOS
    BindViewColorbuffer();
#endif
}

void k2android_gl_post_swap()
{
#ifdef K2P_IOS
    RestoreRenderbuffer();
#endif
}

// The movie player's swap, which iOS routes through here.
extern "C" void k2ios_gl_swap(SDL_Window* pWindow)
{
    k2android_gl_pre_swap();
    SDL_GL_SwapWindow(pWindow);
    k2android_gl_post_swap();
}

void* k2android_gl_proc(const char* pszName) { return gl4es_GetProcAddress(pszName); }
