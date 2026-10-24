// binkgl -- exercises the engine's movie entry point (ASL::PlayBinkMovieGL -> k2movie) in a GL 2.1
// window set up the way the k2 executable's main() asks for one.
//
//   binkgl [--hidden] [--size WxH] [--volume V] [--track N] [--skip-at MS] FILE.bik...
//       play each file through ASL::PlayBinkMovieGL; report opened/skipped and wall time against
//       the movie's own duration; --skip-at posts a synthetic key release MS ms after each start
//   binkgl --verify N [--hidden] FILE.bik...
//       draw the first N frames of each file into an offscreen framebuffer at the movie's size and
//       compare every pixel with a CPU model of the shader (bilinear 4:2:0, the game's constants)
//
// Both modes plant sentinel GL state before each movie and check that it survives.
// Headless: SDL_AUDIODRIVER=dummy binkgl --hidden ... (the GL context still needs a display).
#define GL_SILENCE_DEPRECATION
#include "k2movie.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

namespace ASL {
int PlayBinkMovieGL(const char* pszMovie, float fVolume, unsigned int nSoundTrack,
                    unsigned int nInputDelayMs, bool* pbSkipped, bool* pbQuit);
}

#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif

namespace {

struct Gl {
    void (APIENTRY* GenFramebuffers)(GLsizei, GLuint*);
    void (APIENTRY* BindFramebuffer)(GLenum, GLuint);
    void (APIENTRY* FramebufferTexture2D)(GLenum, GLenum, GLenum, GLuint, GLint);
    GLenum (APIENTRY* CheckFramebufferStatus)(GLenum);
    void (APIENTRY* DeleteFramebuffers)(GLsizei, const GLuint*);
    void (APIENTRY* ActiveTexture)(GLenum);
    void (APIENTRY* GenTextures)(GLsizei, GLuint*);
    void (APIENTRY* DeleteTextures)(GLsizei, const GLuint*);
    void (APIENTRY* BindTexture)(GLenum, GLuint);
    void (APIENTRY* TexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
    void (APIENTRY* ReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*);
    void (APIENTRY* PixelStorei)(GLenum, GLint);
    void (APIENTRY* GetIntegerv)(GLenum, GLint*);
    void (APIENTRY* GetFloatv)(GLenum, GLfloat*);
    GLboolean (APIENTRY* IsEnabled)(GLenum);
    void (APIENTRY* Enable)(GLenum);
    void (APIENTRY* Disable)(GLenum);
    void (APIENTRY* Viewport)(GLint, GLint, GLsizei, GLsizei);
    void (APIENTRY* ClearColor)(GLfloat, GLfloat, GLfloat, GLfloat);
    GLenum (APIENTRY* GetError)(void);
    const GLubyte* (APIENTRY* GetString)(GLenum);
};
Gl gl;

bool LoadGl()
{
    bool ok = true;
#define L(name)                                                                                    \
    *reinterpret_cast<void**>(&gl.name) = SDL_GL_GetProcAddress("gl" #name);                        \
    if (!gl.name) { fprintf(stderr, "missing gl" #name "\n"); ok = false; }
    L(GenFramebuffers) L(BindFramebuffer) L(FramebufferTexture2D) L(CheckFramebufferStatus)
    L(DeleteFramebuffers) L(ActiveTexture) L(GenTextures) L(DeleteTextures) L(BindTexture)
    L(TexImage2D) L(ReadPixels) L(PixelStorei) L(GetIntegerv) L(GetFloatv) L(IsEnabled) L(Enable)
    L(Disable) L(Viewport) L(ClearColor) L(GetError) L(GetString)
#undef L
    return ok;
}

// sentinel state: set something distinctive, check it is still there afterwards.

GLuint g_sentinelTex;

void PlantSentinels(GLint vpW, GLint vpH)
{
    if (!g_sentinelTex) gl.GenTextures(1, &g_sentinelTex);
    gl.ActiveTexture(GL_TEXTURE0);
    gl.BindTexture(GL_TEXTURE_2D, g_sentinelTex);
    gl.ActiveTexture(GL_TEXTURE2);
    gl.Viewport(3, 5, vpW - 7, vpH - 11);
    gl.ClearColor(0.25f, 0.5f, 0.75f, 1.0f);
    gl.Enable(GL_DEPTH_TEST);
    gl.Enable(GL_BLEND);
    gl.PixelStorei(GL_UNPACK_ALIGNMENT, 8);
}

bool CheckSentinels(GLint vpW, GLint vpH)
{
    GLint active = 0, tex = 0, vp[4], align = 0, program = -1;
    GLfloat cc[4];
    gl.GetIntegerv(GL_ACTIVE_TEXTURE, &active);
    gl.ActiveTexture(GL_TEXTURE0);
    gl.GetIntegerv(GL_TEXTURE_BINDING_2D, &tex);
    gl.ActiveTexture(static_cast<GLenum>(active));
    gl.GetIntegerv(GL_VIEWPORT, vp);
    gl.GetFloatv(GL_COLOR_CLEAR_VALUE, cc);
    gl.GetIntegerv(GL_UNPACK_ALIGNMENT, &align);
    gl.GetIntegerv(GL_CURRENT_PROGRAM, &program);
    bool ok = active == GL_TEXTURE2 && tex == static_cast<GLint>(g_sentinelTex) && vp[0] == 3 && vp[1] == 5 &&
              vp[2] == vpW - 7 && vp[3] == vpH - 11 && cc[0] == 0.25f && cc[1] == 0.5f &&
              cc[2] == 0.75f && gl.IsEnabled(GL_DEPTH_TEST) && gl.IsEnabled(GL_BLEND) &&
              align == 8 && program == 0 && gl.GetError() == GL_NO_ERROR;
    if (!ok)
        fprintf(stderr, "  GL state NOT restored: active=%#x tex=%d vp=%d,%d,%d,%d cc=%g,%g,%g "
                        "depth=%d blend=%d align=%d program=%d\n",
                active, tex, vp[0], vp[1], vp[2], vp[3], cc[0], cc[1], cc[2],
                gl.IsEnabled(GL_DEPTH_TEST), gl.IsEnabled(GL_BLEND), align, program);
    return ok;
}

// --verify: a CPU model of the shader.

const float kCrc[3] = { 1.595794677734375f, -0.8134765625f, 0.0f };
const float kCbc[3] = { 0.0f, -0.391448974609375f, 2.017822265625f };
const float kAdj[3] = { -0.8740693926811218f, 0.531782329082489f, -1.085910677909851f };
const float kYs = 1.16412353515625f;

// GL_LINEAR sampling of an 8-bit plane at normalized (s, t), clamp-to-edge.
float Sample(const uint8_t* p, int stride, int w, int h, double s, double t)
{
    double x = s * w - 0.5, y = t * h - 0.5;
    int x0 = static_cast<int>(floor(x)), y0 = static_cast<int>(floor(y));
    double fx = x - x0, fy = y - y0;
    auto at = [&](int xi, int yi) {
        xi = xi < 0 ? 0 : xi >= w ? w - 1 : xi;
        yi = yi < 0 ? 0 : yi >= h ? h - 1 : yi;
        return p[yi * stride + xi] / 255.0;
    };
    double top = at(x0, y0) * (1 - fx) + at(x0 + 1, y0) * fx;
    double bot = at(x0, y0 + 1) * (1 - fx) + at(x0 + 1, y0 + 1) * fx;
    return static_cast<float>(top * (1 - fy) + bot * fy);
}

struct Verify {
    int frames = 0, limit = 0;
    int worst = 0;
    long long over2 = 0, pixels = 0;
    std::vector<uint8_t> rgba;
};

int VerifyHook(void* user, const k2movie_frame* f)
{
    Verify* v = static_cast<Verify*>(user);
    const k2bink_planes* pl = f->planes;
    int w = static_cast<int>(pl->width), h = static_cast<int>(pl->height), cw = (w + 1) / 2, ch = (h + 1) / 2;
    if (f->vp_w != w || f->vp_h != h) {
        fprintf(stderr, "  verify: picture landed at %dx%d, expected %dx%d\n", f->vp_w, f->vp_h, w, h);
        v->worst = 255;
        return 1;
    }
    v->rgba.resize(static_cast<size_t>(w) * h * 4);
    gl.PixelStorei(GL_PACK_ALIGNMENT, 1);
    gl.ReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, v->rgba.data());
    for (int gy = 0; gy < h; ++gy) {
        int y = h - 1 - gy;   // GL rows are bottom-up; the picture's row 0 is at the top
        double t = (y + 0.5) / h;
        for (int x = 0; x < w; ++x) {
            double s = (x + 0.5) / w;
            float Y = pl->y[y * pl->y_stride + x] / 255.0f;
            float V = Sample(pl->v, pl->uv_stride, cw, ch, s, t);
            float U = Sample(pl->u, pl->uv_stride, cw, ch, s, t);
            const uint8_t* got = &v->rgba[(static_cast<size_t>(gy) * w + x) * 4];
            for (int c = 0; c < 3; ++c) {
                float r = kYs * Y + kCrc[c] * V + kCbc[c] * U + kAdj[c];
                r = r < 0 ? 0 : r > 1 ? 1 : r;
                int want = static_cast<int>(lrintf(r * 255.0f));
                int d = abs(want - got[c]);
                if (d > v->worst) v->worst = d;
                if (d > 2) v->over2++;
            }
        }
    }
    v->pixels += static_cast<long long>(w) * h;
    return ++v->frames >= v->limit;
}

bool RunVerify(const char* path, int limit)
{
    int err;
    k2bink* b = k2bink_open_file(path, &err);
    if (!b) { printf("%s: %s\n", path, k2bink_strerror(err)); return false; }
    k2bink_info_t info;
    k2bink_info(b, &info);
    k2bink_close(b);

    GLuint tex, fbo;
    gl.GenTextures(1, &tex);
    gl.ActiveTexture(GL_TEXTURE0);
    gl.BindTexture(GL_TEXTURE_2D, tex);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(info.width), static_cast<GLsizei>(info.height), 0, GL_RGBA,
                  GL_UNSIGNED_BYTE, NULL);
    gl.GenFramebuffers(1, &fbo);
    gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    if (gl.CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "%s: offscreen framebuffer incomplete\n", path);
        return false;
    }
    PlantSentinels(static_cast<GLint>(info.width), static_cast<GLint>(info.height));

    Verify v;
    v.limit = limit;
    k2movie_opts o;
    k2movie_default_opts(&o);
    o.audio_track = -1;   // silent: the wall clock paces it
    o.target_w = static_cast<int>(info.width);
    o.target_h = static_cast<int>(info.height);
    o.after_draw = VerifyHook;
    o.user = &v;
    k2movie_result r;
    k2movie_play(path, &o, &r);

    bool state = CheckSentinels(static_cast<GLint>(info.width), static_cast<GLint>(info.height));
    gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
    gl.DeleteFramebuffers(1, &fbo);
    gl.DeleteTextures(1, &tex);
    bool pass = r.opened && v.frames == limit && v.worst <= 2 && state;
    printf("%s: %s  %d frame(s), %lld px, worst channel error %d, %lld channel(s) > 2, GL state %s\n",
           path, pass ? "PASS" : "FAIL", v.frames, v.pixels, v.worst, v.over2,
           state ? "restored" : "CLOBBERED");
    return pass;
}

// default mode: the engine entry point.

Uint32 SDLCALL PostKeyUp(Uint32, void*)
{
    SDL_Event e;
    memset(&e, 0, sizeof e);
    e.type = SDL_KEYUP;
    e.key.keysym.sym = SDLK_ESCAPE;
    e.key.keysym.scancode = SDL_SCANCODE_ESCAPE;
    SDL_PushEvent(&e);
    return 0;
}

bool RunPlay(const char* path, SDL_Window* win, float volume, int track, int skipAt)
{
    int err;
    k2bink* b = k2bink_open_file(path, &err);
    double dur = 0;
    if (b) {
        k2bink_info_t info;
        k2bink_info(b, &info);
        dur = static_cast<double>(info.frames) * info.fps_den / info.fps_num;
        k2bink_close(b);
    }
    int dw, dh;
    SDL_GL_GetDrawableSize(win, &dw, &dh);
    PlantSentinels(dw, dh);

    SDL_TimerID timer = skipAt >= 0 ? SDL_AddTimer(static_cast<Uint32>(skipAt), PostKeyUp, NULL) : 0;
    bool bSkipped = false, bQuit = false;
    Uint64 t0 = SDL_GetPerformanceCounter();
    int opened = ASL::PlayBinkMovieGL(path, volume, static_cast<unsigned>(track), 250, &bSkipped, &bQuit);
    double wall = static_cast<double>(SDL_GetPerformanceCounter() - t0) / SDL_GetPerformanceFrequency();
    if (timer) SDL_RemoveTimer(timer);

    bool state = CheckSentinels(dw, dh);
    printf("%s: opened=%d skipped=%d quit=%d  %.2f s wall for a %.2f s movie, GL state %s\n", path,
           opened, bSkipped, bQuit, wall, dur, state ? "restored" : "CLOBBERED");
    return opened && state;
}

} // namespace

int main(int argc, char** argv)
{
    int verify = 0, hidden = 0, track = 0, skipAt = -1, w = 1280, h = 720;
    float volume = 1.0f;
    std::vector<const char*> files;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--verify") && i + 1 < argc) verify = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--hidden")) hidden = 1;
        else if (!strcmp(argv[i], "--track") && i + 1 < argc) track = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--volume") && i + 1 < argc) volume = static_cast<float>(atof(argv[++i]));
        else if (!strcmp(argv[i], "--skip-at") && i + 1 < argc) skipAt = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--size") && i + 1 < argc) sscanf(argv[++i], "%dx%d", &w, &h);
        else files.push_back(argv[i]);
    }
    if (files.empty()) {
        fprintf(stderr, "usage: binkgl [--verify N] [--hidden] [--size WxH] [--volume V] "
                        "[--track N] [--skip-at MS] FILE.bik...\n");
        return 2;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    // Request the context the k2 executable's main() does, desktop GL 2.1 with the legacy profile.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_Window* win = SDL_CreateWindow("binkgl", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h,
                                       SDL_WINDOW_OPENGL | (hidden ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN));
    SDL_GLContext ctx = win ? SDL_GL_CreateContext(win) : NULL;
    if (!ctx) {
        fprintf(stderr, "GL 2.1 window: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_SetSwapInterval(1);
    if (!LoadGl()) return 1;
    printf("GL %s (%s)\n", reinterpret_cast<const char*>(gl.GetString(GL_VERSION)), reinterpret_cast<const char*>(gl.GetString(GL_RENDERER)));

    int failed = 0;
    for (const char* f : files)
        failed += verify ? !RunVerify(f, verify) : !RunPlay(f, win, volume, track, skipAt);

    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    printf("%zu file(s), %d failed\n", files.size(), failed);
    return failed != 0;
}
