// shim_glpresent.cpp -- Windows: the window's frame is drawn into a framebuffer object of the
// shim's and copied to the window's back buffer just before each swap.
//
// The engines draw a frame in several passes straight into the window's back buffer (clear, the
// scene, its present quad, the GUI), and K1 draws its GUI once more after a swap that a load screen
// makes in the middle of a frame, on the buffer as the swap left it. NVIDIA's driver presents a GL
// window through a DXGI flip-model swap chain by default, where the back buffer after a swap is
// another buffer of the chain: it can show a buffer the next frame is being drawn into, so the
// display gets a just-cleared (black) buffer or the scene before the GUI pass. Drawing into a buffer
// of our own that persists across swaps, and touching the window's back buffer only with one full
// copy right before the swap, leaves the window nothing to show but whole frames, and gives the
// after-swap draw the contents it was written against.
//
// Every bind of framebuffer 0 the engines and the movie player make (glBindFramebuffer[EXT] by name
// through k2shim_gl_proc, the engine's own forwarder in windows/win_engine_imports.c) binds ours
// instead, and the queries that would see it answer as for the window (binding 0, GL_BACK).
// K2_GL_OFFSCREEN=0 turns it off.
#include "k2shim.h"

#include <SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
namespace {

typedef void (K2_GLAPI* PfnBind)(unsigned int, unsigned int);
typedef void (K2_GLAPI* PfnGen)(int, unsigned int*);
typedef void (K2_GLAPI* PfnGetIntegerv)(unsigned int, int*);
typedef void (K2_GLAPI* PfnEnum)(unsigned int);
typedef unsigned char (K2_GLAPI* PfnIsEnabled)(unsigned int);
typedef void (K2_GLAPI* PfnStorage)(unsigned int, unsigned int, int, int);
typedef void (K2_GLAPI* PfnAttach)(unsigned int, unsigned int, unsigned int, unsigned int);
typedef unsigned int (K2_GLAPI* PfnStatus)(unsigned int);
typedef void (K2_GLAPI* PfnBlit)(int, int, int, int, int, int, int, int, unsigned int, unsigned int);
typedef void (K2_GLAPI* PfnClearColor)(float, float, float, float);
typedef void (K2_GLAPI* PfnClearDepth)(double);
typedef void (K2_GLAPI* PfnClearStencil)(int);
typedef void (K2_GLAPI* PfnColorMask)(unsigned char, unsigned char, unsigned char, unsigned char);
typedef void (K2_GLAPI* PfnDepthMask)(unsigned char);
typedef void (K2_GLAPI* PfnStencilMask)(unsigned int);
typedef void (K2_GLAPI* PfnGetFloatv)(unsigned int, float*);
typedef void (K2_GLAPI* PfnGetBooleanv)(unsigned int, unsigned char*);

enum : unsigned int {
    kFramebuffer = 0x8D40, kReadFramebuffer = 0x8CA8, kDrawFramebuffer = 0x8CA9,
    kDrawFramebufferBinding = 0x8CA6, kReadFramebufferBinding = 0x8CAA,
    kRenderbuffer = 0x8D41, kRenderbufferBinding = 0x8CA7,
    kColorAttachment0 = 0x8CE0, kDepthAttachment = 0x8D00, kStencilAttachment = 0x8D20,
    kComplete = 0x8CD5, kDrawBuffer = 0x0C01, kReadBuffer = 0x0C02, kBack = 0x0405,
    kScissorTest = 0x0C11, kAlphaBits = 0x0D55, kDepthBits = 0x0D56, kStencilBits = 0x0D57,
};

// The driver's entry points, never the wrappers below.
struct Gl {
    PfnBind BindFramebuffer, BindFramebufferEXT, BindRenderbuffer;
    PfnGen GenFramebuffers, GenRenderbuffers;
    PfnGetIntegerv GetIntegerv;
    PfnEnum Enable, Disable, Clear, DrawBuffer, ReadBuffer;
    PfnIsEnabled IsEnabled;
    PfnStorage RenderbufferStorage;
    PfnAttach FramebufferRenderbuffer;
    PfnStatus CheckFramebufferStatus;
    PfnBlit BlitFramebuffer;
    PfnClearColor ClearColor;
    PfnClearDepth ClearDepth;
    PfnClearStencil ClearStencil;
    PfnColorMask ColorMask;
    PfnDepthMask DepthMask;
    PfnStencilMask StencilMask;
    PfnGetFloatv GetFloatv;
    PfnGetBooleanv GetBooleanv;
};
Gl g;

template <typename T> void Load(T& p, const char* pszName)
{
    if (!p) p = reinterpret_cast<T>(SDL_GL_GetProcAddress(pszName));
}

bool LoadAll()
{
    Load(g.BindFramebuffer, "glBindFramebuffer");
    Load(g.BindRenderbuffer, "glBindRenderbuffer");
    Load(g.GenFramebuffers, "glGenFramebuffers");
    Load(g.GenRenderbuffers, "glGenRenderbuffers");
    Load(g.GetIntegerv, "glGetIntegerv");
    Load(g.Enable, "glEnable");
    Load(g.Disable, "glDisable");
    Load(g.Clear, "glClear");
    Load(g.IsEnabled, "glIsEnabled");
    Load(g.RenderbufferStorage, "glRenderbufferStorage");
    Load(g.FramebufferRenderbuffer, "glFramebufferRenderbuffer");
    Load(g.CheckFramebufferStatus, "glCheckFramebufferStatus");
    Load(g.BlitFramebuffer, "glBlitFramebuffer");
    Load(g.ClearColor, "glClearColor");
    Load(g.ClearDepth, "glClearDepth");
    Load(g.ClearStencil, "glClearStencil");
    Load(g.ColorMask, "glColorMask");
    Load(g.DepthMask, "glDepthMask");
    Load(g.StencilMask, "glStencilMask");
    Load(g.GetFloatv, "glGetFloatv");
    Load(g.GetBooleanv, "glGetBooleanv");
    return g.BindFramebuffer && g.BindRenderbuffer && g.GenFramebuffers && g.GenRenderbuffers &&
           g.GetIntegerv && g.Enable && g.Disable && g.Clear && g.IsEnabled && g.RenderbufferStorage &&
           g.FramebufferRenderbuffer && g.CheckFramebufferStatus && g.BlitFramebuffer && g.ClearColor &&
           g.ClearDepth && g.ClearStencil && g.ColorMask && g.DepthMask && g.StencilMask &&
           g.GetFloatv && g.GetBooleanv;
}

// The framebuffer standing in for the window's, in the context it was made in (the shim keeps one
// context for the game's life; a deleted one is parked for the next wglCreateContext).
struct Offscreen {
    SDL_GLContext ctx;
    unsigned int fbo, color, depth;
    unsigned int eColorFormat, eDepthFormat;
    bool bStencil;
    int w, h;
};
Offscreen s;
bool s_bFailed;      // no framebuffer objects, or an incomplete one: straight to the window
bool s_bPresenting;  // inside k2shim_gl_present (K1's movie swaps through K1's SDL_GL_SwapWindow)

bool Enabled()
{
    static int s_n = -1;
    if (s_n < 0) {
        const char* psz = getenv("K2_GL_OFFSCREEN");
        s_n = !(psz && strcmp(psz, "0") == 0);
    }
    return s_n && !s_bFailed;
}

bool Active()
{
    return s.fbo && s.ctx == SDL_GL_GetCurrentContext();
}

// Clears the whole buffer (bound to the draw target) to black and puts the engine's clear state back.
void ClearAll()
{
    float afColor[4];
    double fDepth = 1.0;
    int nStencil = 0;
    unsigned char abColorMask[4], bDepthMask;
    int nStencilMask = 0;
    g.GetFloatv(0x0C22, afColor);              // GL_COLOR_CLEAR_VALUE
    float fDepthF = 1.0f;
    g.GetFloatv(0x0B73, &fDepthF);             // GL_DEPTH_CLEAR_VALUE
    fDepth = fDepthF;
    g.GetIntegerv(0x0B91, &nStencil);          // GL_STENCIL_CLEAR_VALUE
    g.GetBooleanv(0x0C23, abColorMask);        // GL_COLOR_WRITEMASK
    g.GetBooleanv(0x0B72, &bDepthMask);        // GL_DEPTH_WRITEMASK
    g.GetIntegerv(0x0B98, &nStencilMask);      // GL_STENCIL_WRITEMASK
    bool bScissor = g.IsEnabled(kScissorTest) != 0;
    if (bScissor) g.Disable(kScissorTest);
    g.ClearColor(0, 0, 0, 0);
    g.ClearDepth(1.0);
    g.ClearStencil(0);
    g.ColorMask(1, 1, 1, 1);
    g.DepthMask(1);
    g.StencilMask(0xffffffffu);
    g.Clear(0x4000 | 0x0100 | 0x0400);
    g.ClearColor(afColor[0], afColor[1], afColor[2], afColor[3]);
    g.ClearDepth(fDepth);
    g.ClearStencil(nStencil);
    g.ColorMask(abColorMask[0], abColorMask[1], abColorMask[2], abColorMask[3]);
    g.DepthMask(bDepthMask);
    g.StencilMask(static_cast<unsigned int>(nStencilMask));
    if (bScissor) g.Enable(kScissorTest);
}

void Storage(int w, int h)
{
    int nRenderbuffer = 0;
    g.GetIntegerv(kRenderbufferBinding, &nRenderbuffer);
    g.BindRenderbuffer(kRenderbuffer, s.color);
    g.RenderbufferStorage(kRenderbuffer, s.eColorFormat, w, h);
    g.BindRenderbuffer(kRenderbuffer, s.depth);
    g.RenderbufferStorage(kRenderbuffer, s.eDepthFormat, w, h);
    g.BindRenderbuffer(kRenderbuffer, static_cast<unsigned int>(nRenderbuffer));
    s.w = w;
    s.h = h;
}

// Makes the stand-in for the current context, matching the window's buffer (alpha, depth and
// stencil bits: the engine blends with the destination alpha), and binds it where the window's
// framebuffer was bound. The window's framebuffer must be the draw binding.
void Create(int w, int h)
{
    int nAlpha = 0, nDepth = 0, nStencil = 0;
    g.GetIntegerv(kAlphaBits, &nAlpha);
    g.GetIntegerv(kDepthBits, &nDepth);
    g.GetIntegerv(kStencilBits, &nStencil);
    s = Offscreen();
    s.ctx = SDL_GL_GetCurrentContext();
    s.eColorFormat = nAlpha > 0 ? 0x8058 : 0x8051;          // GL_RGBA8 : GL_RGB8
    s.bStencil = nStencil > 0;
    s.eDepthFormat = s.bStencil ? 0x88F0                     // GL_DEPTH24_STENCIL8
                   : nDepth > 24 ? 0x81A7 : 0x81A6;          // GL_DEPTH_COMPONENT32 / 24
    g.GenFramebuffers(1, &s.fbo);
    g.GenRenderbuffers(1, &s.color);
    g.GenRenderbuffers(1, &s.depth);
    Storage(w, h);
    g.BindFramebuffer(kFramebuffer, s.fbo);
    g.FramebufferRenderbuffer(kFramebuffer, kColorAttachment0, kRenderbuffer, s.color);
    g.FramebufferRenderbuffer(kFramebuffer, kDepthAttachment, kRenderbuffer, s.depth);
    if (s.bStencil)
        g.FramebufferRenderbuffer(kFramebuffer, kStencilAttachment, kRenderbuffer, s.depth);
    unsigned int eStatus = g.CheckFramebufferStatus(kFramebuffer);
    if (eStatus != kComplete) {
        fprintf(stderr, "k2shim: offscreen frame: framebuffer incomplete (0x%x); drawing to the window\n",
                eStatus);
        g.BindFramebuffer(kFramebuffer, 0);
        s = Offscreen();
        s_bFailed = true;
        return;
    }
    ClearAll();
    fprintf(stderr, "k2shim: offscreen frame %dx%d (alpha %d, depth %d, stencil %d)\n", w, h, nAlpha,
            nDepth, nStencil);
}

PfnBind s_pBindFramebuffer, s_pBindFramebufferEXT;
PfnGetIntegerv s_pGetIntegerv;
PfnEnum s_pDrawBuffer, s_pReadBuffer;

unsigned int Window(unsigned int nFramebuffer)
{
    return nFramebuffer == 0 && Active() ? s.fbo : nFramebuffer;
}
void K2_GLAPI BindFramebuffer(unsigned int eTarget, unsigned int nFramebuffer)
{
    s_pBindFramebuffer(eTarget, Window(nFramebuffer));
}
void K2_GLAPI BindFramebufferEXT(unsigned int eTarget, unsigned int nFramebuffer)
{
    s_pBindFramebufferEXT(eTarget, Window(nFramebuffer));
}

bool Bound(unsigned int eBinding)
{
    if (!Active()) return false;
    int n = 0;
    s_pGetIntegerv(eBinding, &n);
    return static_cast<unsigned int>(n) == s.fbo;
}
void K2_GLAPI GetIntegerv(unsigned int ePname, int* pn)
{
    s_pGetIntegerv(ePname, pn);
    if (!pn || !s.fbo) return;
    if ((ePname == kDrawFramebufferBinding || ePname == kReadFramebufferBinding) &&
        static_cast<unsigned int>(*pn) == s.fbo && Active())
        *pn = 0;
    else if (ePname == kDrawBuffer && *pn == static_cast<int>(kColorAttachment0) && Bound(kDrawFramebufferBinding))
        *pn = kBack;
    else if (ePname == kReadBuffer && *pn == static_cast<int>(kColorAttachment0) && Bound(kReadFramebufferBinding))
        *pn = kBack;
}
// GL_FRONT / GL_BACK and the other window buffers name the one colour buffer ours has.
bool WindowBuffer(unsigned int eMode)
{
    return eMode >= 0x0400 && eMode <= 0x0408;   // GL_FRONT_LEFT .. GL_FRONT_AND_BACK
}
void K2_GLAPI DrawBuffer(unsigned int eMode)
{
    s_pDrawBuffer(WindowBuffer(eMode) && Bound(kDrawFramebufferBinding) ? kColorAttachment0 : eMode);
}
void K2_GLAPI ReadBuffer(unsigned int eMode)
{
    s_pReadBuffer(WindowBuffer(eMode) && Bound(kReadFramebufferBinding) ? kColorAttachment0 : eMode);
}

} // namespace

void* k2shim_glpresent_proc(const char* pszName, void* real)
{
    if (!real || !Enabled()) return real;
    struct { const char* name; void** slot; void* wrapper; } const k[] = {
        { "glBindFramebuffer", reinterpret_cast<void**>(&s_pBindFramebuffer), reinterpret_cast<void*>(BindFramebuffer) },
        { "glBindFramebufferEXT", reinterpret_cast<void**>(&s_pBindFramebufferEXT), reinterpret_cast<void*>(BindFramebufferEXT) },
        { "glGetIntegerv", reinterpret_cast<void**>(&s_pGetIntegerv), reinterpret_cast<void*>(GetIntegerv) },
        { "glDrawBuffer", reinterpret_cast<void**>(&s_pDrawBuffer), reinterpret_cast<void*>(DrawBuffer) },
        { "glReadBuffer", reinterpret_cast<void**>(&s_pReadBuffer), reinterpret_cast<void*>(ReadBuffer) },
    };
    for (const auto& e : k)
        if (strcmp(pszName, e.name) == 0) {
            *e.slot = real;
            return e.wrapper;
        }
    return real;
}

void k2shim_gl_present(SDL_Window* pWindow, void (*pfnSwap)(SDL_Window*))
{
    if (s_bPresenting || !Enabled() || !SDL_GL_GetCurrentContext() || !LoadAll()) {
        pfnSwap(pWindow);
        return;
    }
    s_bPresenting = true;
    int w = 0, h = 0;
    SDL_GL_GetDrawableSize(pWindow, &w, &h);
    if (Active()) {
        int nDraw = 0, nRead = 0;
        g.GetIntegerv(kDrawFramebufferBinding, &nDraw);
        g.GetIntegerv(kReadFramebufferBinding, &nRead);
        bool bScissor = g.IsEnabled(kScissorTest) != 0;
        if (bScissor) g.Disable(kScissorTest);
        g.BindFramebuffer(kReadFramebuffer, s.fbo);
        g.BindFramebuffer(kDrawFramebuffer, 0);
        int cw = s.w < w ? s.w : w, ch = s.h < h ? s.h : h;
        g.BlitFramebuffer(0, 0, cw, ch, 0, 0, cw, ch, 0x4000, 0x2600);   // COLOR, NEAREST
        pfnSwap(pWindow);
        if ((w != s.w || h != s.h) && w > 0 && h > 0) {
            Storage(w, h);
            g.BindFramebuffer(kDrawFramebuffer, s.fbo);
            ClearAll();
        }
        g.BindFramebuffer(kDrawFramebuffer, nDraw ? static_cast<unsigned int>(nDraw) : s.fbo);
        g.BindFramebuffer(kReadFramebuffer, nRead ? static_cast<unsigned int>(nRead) : s.fbo);
        if (bScissor) g.Enable(kScissorTest);
    } else {
        pfnSwap(pWindow);
        int nDraw = 0;
        g.GetIntegerv(kDrawFramebufferBinding, &nDraw);
        if (w > 0 && h > 0 && nDraw == 0) {
            if (s.fbo)   // another context's: its names mean nothing here
                fprintf(stderr, "k2shim: offscreen frame: a new GL context\n");
            Create(w, h);
        }
    }
    s_bPresenting = false;
}
#else
void* k2shim_glpresent_proc(const char*, void* real) { return real; }
void k2shim_gl_present(SDL_Window* pWindow, void (*pfnSwap)(SDL_Window*)) { pfnSwap(pWindow); }
#endif

// For C callers: the movie player's GL lookup and swap (K2M_GL_GETPROC / K2M_GL_SWAP on Windows),
// and the engine's GL 3.0 forwarders.
extern "C" void* k2shim_gl_proc_c(const char* pszName) { return k2shim_gl_proc(pszName); }
extern "C" void k2shim_gl_swap_c(SDL_Window* pWindow) { k2shim_gl_present(pWindow, SDL_GL_SwapWindow); }
