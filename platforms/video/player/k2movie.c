#include <stdint.h>
#include <inttypes.h>
/*
 * k2movie -- see k2movie.h. SPDX-License-Identifier: MIT OR Apache-2.0
 */
#define GL_SILENCE_DEPRECATION
#include "k2movie.h"

#include <SDL.h>
#if defined(__IPHONEOS__)
/* iOS has no desktop GL and SDL_opengl.h is empty there: the embedding renders through gl4es,
   whose headers are the desktop ones. */
#include <GL/gl.h>
#include <GL/glext.h>
#else
#include <SDL_opengl.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- GL entry points, all through SDL_GL_GetProcAddress ---------------------------------------- */

/* The Android build defines K2M_GL_GETPROC as gl4es's lookup, so the player's GL calls go through the
   same GL layer, and state, as the engine's. */
#ifdef K2M_GL_GETPROC
extern void* K2M_GL_GETPROC(const char* name);
#else
#define K2M_GL_GETPROC SDL_GL_GetProcAddress
#endif
/* Or the embedding's own swap, when it needs one: iOS binds the view's colour renderbuffer around
   SDL's swap. */
#ifdef K2M_GL_SWAP
extern void K2M_GL_SWAP(SDL_Window* win);
#else
#define K2M_GL_SWAP SDL_GL_SwapWindow
#endif

#define K2M_GL(X)                                                                                  \
    X(void, ActiveTexture, (GLenum))                                                               \
    X(void, AttachShader, (GLuint, GLuint))                                                        \
    X(void, BindAttribLocation, (GLuint, GLuint, const GLchar*))                                   \
    X(void, BindBuffer, (GLenum, GLuint))                                                          \
    X(void, BindTexture, (GLenum, GLuint))                                                         \
    X(void, BlendFunc, (GLenum, GLenum))                                                           \
    X(void, BlendFuncSeparate, (GLenum, GLenum, GLenum, GLenum))                                   \
    X(void, BufferData, (GLenum, GLsizeiptr, const void*, GLenum))                                 \
    X(void, Clear, (GLbitfield))                                                                   \
    X(void, ClearColor, (GLfloat, GLfloat, GLfloat, GLfloat))                                      \
    X(void, ColorMask, (GLboolean, GLboolean, GLboolean, GLboolean))                               \
    X(void, CompileShader, (GLuint))                                                               \
    X(GLuint, CreateProgram, (void))                                                               \
    X(GLuint, CreateShader, (GLenum))                                                              \
    X(void, DeleteBuffers, (GLsizei, const GLuint*))                                               \
    X(void, DeleteProgram, (GLuint))                                                               \
    X(void, DeleteShader, (GLuint))                                                                \
    X(void, DeleteTextures, (GLsizei, const GLuint*))                                              \
    X(void, Disable, (GLenum))                                                                     \
    X(void, DisableVertexAttribArray, (GLuint))                                                    \
    X(void, DrawArrays, (GLenum, GLint, GLsizei))                                                  \
    X(void, DrawBuffer, (GLenum))                                                                  \
    X(void, Enable, (GLenum))                                                                      \
    X(void, EnableVertexAttribArray, (GLuint))                                                     \
    X(void, GenBuffers, (GLsizei, GLuint*))                                                        \
    X(void, GenTextures, (GLsizei, GLuint*))                                                       \
    X(GLenum, GetError, (void))                                                                    \
    X(void, GetFloatv, (GLenum, GLfloat*))                                                         \
    X(void, GetIntegerv, (GLenum, GLint*))                                                         \
    X(void, GetBooleanv, (GLenum, GLboolean*))                                                     \
    X(void, GetProgramInfoLog, (GLuint, GLsizei, GLsizei*, GLchar*))                               \
    X(void, GetProgramiv, (GLuint, GLenum, GLint*))                                                \
    X(void, GetShaderInfoLog, (GLuint, GLsizei, GLsizei*, GLchar*))                                \
    X(void, GetShaderiv, (GLuint, GLenum, GLint*))                                                 \
    X(GLint, GetUniformLocation, (GLuint, const GLchar*))                                          \
    X(void, GetVertexAttribiv, (GLuint, GLenum, GLint*))                                           \
    X(void, GetVertexAttribPointerv, (GLuint, GLenum, void**))                                     \
    X(GLboolean, IsEnabled, (GLenum))                                                              \
    X(void, LinkProgram, (GLuint))                                                                 \
    X(void, PixelStorei, (GLenum, GLint))                                                          \
    X(void, ShaderSource, (GLuint, GLsizei, const GLchar* const*, const GLint*))                   \
    X(void, TexImage2D, (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*)) \
    X(void, TexParameteri, (GLenum, GLenum, GLint))                                                \
    X(void, TexSubImage2D, (GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void*)) \
    X(void, Uniform1f, (GLint, GLfloat))                                                           \
    X(void, Uniform1i, (GLint, GLint))                                                             \
    X(void, Uniform4f, (GLint, GLfloat, GLfloat, GLfloat, GLfloat))                                \
    X(void, Uniform4fv, (GLint, GLsizei, const GLfloat*))                                          \
    X(void, UseProgram, (GLuint))                                                                  \
    X(void, VertexAttribPointer, (GLuint, GLint, GLenum, GLboolean, GLsizei, const void*))         \
    X(void, Viewport, (GLint, GLint, GLsizei, GLsizei))

typedef struct k2m_gl {
#define X(ret, name, args) ret (APIENTRY* name) args;
    K2M_GL(X)
#undef X
    void (APIENTRY* BindFramebuffer)(GLenum, GLuint);   /* optional: GL 3.0 / ARB_framebuffer_object */
} k2m_gl;

static int k2m_gl_load(k2m_gl* gl)
{
    int ok = 1;
#define X(ret, name, args)                                                                         \
    *(void**)&gl->name = K2M_GL_GETPROC("gl" #name);                                        \
    if (!gl->name) { fprintf(stderr, "k2movie: missing gl" #name "\n"); ok = 0; }
    K2M_GL(X)
#undef X
    *(void**)&gl->BindFramebuffer = K2M_GL_GETPROC("glBindFramebuffer");
    if (!gl->BindFramebuffer)
        *(void**)&gl->BindFramebuffer = K2M_GL_GETPROC("glBindFramebufferEXT");
    return ok;
}

#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#endif
#ifndef GL_PIXEL_UNPACK_BUFFER
#define GL_PIXEL_UNPACK_BUFFER 0x88EC
#endif
#ifndef GL_PIXEL_UNPACK_BUFFER_BINDING
#define GL_PIXEL_UNPACK_BUFFER_BINDING 0x88EF
#endif

/* ---- the shader ------------------------------------------------------------------------------- */

/* Aspyr's Bink 1 constants: BT.601 limited range. cR is the V plane, cB the U plane. */
static const GLfloat k_crc[4]    = { 1.595794677734375f, -0.8134765625f, 0.0f, 0.0f };
static const GLfloat k_cbc[4]    = { 0.0f, -0.391448974609375f, 2.017822265625f, 0.0f };
static const GLfloat k_adj[4]    = { -0.8740693926811218f, 0.531782329082489f, -1.085910677909851f, 0.0f };
static const GLfloat k_yscale[4] = { 1.16412353515625f, 1.16412353515625f, 1.16412353515625f, 1.0f };

static const char k_vs[] =
    "#version 120\n"
    "attribute vec2 a_position;\n"
    "varying vec2 v_texcoords;\n"
    "uniform vec4 coord_xy;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = vec4(a_position * coord_xy.xy + coord_xy.zw, 0.0, 1.0);\n"
    "    v_texcoords = a_position;\n"
    "}\n";

static const char k_fs[] =
    "#version 120\n"
    "uniform sampler2D YTex;\n"
    "uniform sampler2D cRTex;\n"
    "uniform sampler2D cBTex;\n"
    "uniform sampler2D ATex;\n"
    "uniform vec4 crc;\n"
    "uniform vec4 cbc;\n"
    "uniform vec4 adj;\n"
    "uniform vec4 yscale;\n"
    "uniform float has_alpha;\n"
    "varying vec2 v_texcoords;\n"
    "void main()\n"
    "{\n"
    "    vec4 p = yscale * texture2D(YTex, v_texcoords).x;\n"
    "    p += crc * texture2D(cRTex, v_texcoords).x;\n"
    "    p += cbc * texture2D(cBTex, v_texcoords).x;\n"
    "    p += adj;\n"
    "    p.w = has_alpha > 0.5 ? texture2D(ATex, v_texcoords).x : 1.0;\n"
    "    gl_FragColor = p;\n"
    "}\n";

/* The unit quad as a triangle strip; the vertex shader maps it into the letterbox. */
static const GLfloat k_quad[8] = { 0, 0, 1, 0, 0, 1, 1, 1 };

static GLuint k2m_shader(const k2m_gl* gl, GLenum type, const char* src)
{
    GLuint s = gl->CreateShader(type);
    GLint ok = 0;
    gl->ShaderSource(s, 1, &src, NULL);
    gl->CompileShader(s);
    gl->GetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        gl->GetShaderInfoLog(s, sizeof log, NULL, log);
        fprintf(stderr, "k2movie: shader compile failed: %s\n", log);
        gl->DeleteShader(s);
        return 0;
    }
    return s;
}

/* ---- GL state the player touches -------------------------------------------------------------- */

typedef struct k2m_glstate {
    GLint     fbo, draw_buffer, program, active_texture, tex[4], array_buffer, unpack_buffer;
    GLint     viewport[4], unpack_align, unpack_row_length, unpack_skip_rows, unpack_skip_pixels;
    GLfloat   clear_color[4];
    GLboolean color_mask[4];
    GLboolean blend, cull, depth, scissor, stencil, alpha_test;
    GLint     blend_src_rgb, blend_dst_rgb, blend_src_a, blend_dst_a;
    GLint     a0_enabled, a0_size, a0_type, a0_normalized, a0_stride, a0_buffer;
    void*     a0_pointer;
} k2m_glstate;

static void k2m_save(const k2m_gl* gl, k2m_glstate* s)
{
    s->fbo = 0;
    if (gl->BindFramebuffer) gl->GetIntegerv(GL_FRAMEBUFFER_BINDING, &s->fbo);
    gl->GetIntegerv(GL_DRAW_BUFFER, &s->draw_buffer);
    gl->GetIntegerv(GL_CURRENT_PROGRAM, &s->program);
    gl->GetIntegerv(GL_ACTIVE_TEXTURE, &s->active_texture);
    for (int i = 0; i < 4; ++i) {
        gl->ActiveTexture(GL_TEXTURE0 + i);
        gl->GetIntegerv(GL_TEXTURE_BINDING_2D, &s->tex[i]);
    }
    gl->GetIntegerv(GL_ARRAY_BUFFER_BINDING, &s->array_buffer);
    gl->GetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &s->unpack_buffer);
    gl->GetIntegerv(GL_VIEWPORT, s->viewport);
    gl->GetIntegerv(GL_UNPACK_ALIGNMENT, &s->unpack_align);
    gl->GetIntegerv(GL_UNPACK_ROW_LENGTH, &s->unpack_row_length);
    gl->GetIntegerv(GL_UNPACK_SKIP_ROWS, &s->unpack_skip_rows);
    gl->GetIntegerv(GL_UNPACK_SKIP_PIXELS, &s->unpack_skip_pixels);
    gl->GetFloatv(GL_COLOR_CLEAR_VALUE, s->clear_color);
    gl->GetBooleanv(GL_COLOR_WRITEMASK, s->color_mask);
    s->blend = gl->IsEnabled(GL_BLEND);
    s->cull = gl->IsEnabled(GL_CULL_FACE);
    s->depth = gl->IsEnabled(GL_DEPTH_TEST);
    s->scissor = gl->IsEnabled(GL_SCISSOR_TEST);
    s->stencil = gl->IsEnabled(GL_STENCIL_TEST);
    s->alpha_test = gl->IsEnabled(GL_ALPHA_TEST);
    gl->GetIntegerv(GL_BLEND_SRC_RGB, &s->blend_src_rgb);
    gl->GetIntegerv(GL_BLEND_DST_RGB, &s->blend_dst_rgb);
    gl->GetIntegerv(GL_BLEND_SRC_ALPHA, &s->blend_src_a);
    gl->GetIntegerv(GL_BLEND_DST_ALPHA, &s->blend_dst_a);
    /* Generic attribute 0 aliases the fixed-function vertex array in a compatibility context. */
    gl->GetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &s->a0_enabled);
    gl->GetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_SIZE, &s->a0_size);
    gl->GetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_TYPE, &s->a0_type);
    gl->GetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &s->a0_normalized);
    gl->GetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &s->a0_stride);
    gl->GetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &s->a0_buffer);
    gl->GetVertexAttribPointerv(0, GL_VERTEX_ATTRIB_ARRAY_POINTER, &s->a0_pointer);
}

static void k2m_set(const k2m_gl* gl, GLenum cap, GLboolean on)
{
    if (on) gl->Enable(cap); else gl->Disable(cap);
}

static void k2m_restore(const k2m_gl* gl, const k2m_glstate* s)
{
    gl->BindBuffer(GL_ARRAY_BUFFER, (GLuint)s->a0_buffer);
    gl->VertexAttribPointer(0, s->a0_size, (GLenum)s->a0_type, (GLboolean)s->a0_normalized,
                            s->a0_stride, s->a0_pointer);
    if (s->a0_enabled) gl->EnableVertexAttribArray(0); else gl->DisableVertexAttribArray(0);
    gl->BindBuffer(GL_ARRAY_BUFFER, (GLuint)s->array_buffer);
    gl->BindBuffer(GL_PIXEL_UNPACK_BUFFER, (GLuint)s->unpack_buffer);
    gl->UseProgram((GLuint)s->program);
    for (int i = 0; i < 4; ++i) {
        gl->ActiveTexture(GL_TEXTURE0 + i);
        gl->BindTexture(GL_TEXTURE_2D, (GLuint)s->tex[i]);
    }
    gl->ActiveTexture((GLenum)s->active_texture);
    gl->PixelStorei(GL_UNPACK_ALIGNMENT, s->unpack_align);
    gl->PixelStorei(GL_UNPACK_ROW_LENGTH, s->unpack_row_length);
    gl->PixelStorei(GL_UNPACK_SKIP_ROWS, s->unpack_skip_rows);
    gl->PixelStorei(GL_UNPACK_SKIP_PIXELS, s->unpack_skip_pixels);
    gl->Viewport(s->viewport[0], s->viewport[1], s->viewport[2], s->viewport[3]);
    gl->ClearColor(s->clear_color[0], s->clear_color[1], s->clear_color[2], s->clear_color[3]);
    gl->ColorMask(s->color_mask[0], s->color_mask[1], s->color_mask[2], s->color_mask[3]);
    k2m_set(gl, GL_BLEND, s->blend);
    k2m_set(gl, GL_CULL_FACE, s->cull);
    k2m_set(gl, GL_DEPTH_TEST, s->depth);
    k2m_set(gl, GL_SCISSOR_TEST, s->scissor);
    k2m_set(gl, GL_STENCIL_TEST, s->stencil);
    k2m_set(gl, GL_ALPHA_TEST, s->alpha_test);
    gl->BlendFuncSeparate((GLenum)s->blend_src_rgb, (GLenum)s->blend_dst_rgb,
                          (GLenum)s->blend_src_a, (GLenum)s->blend_dst_a);
    if (gl->BindFramebuffer) gl->BindFramebuffer(GL_FRAMEBUFFER, (GLuint)s->fbo);
    gl->DrawBuffer((GLenum)s->draw_buffer);
}

/* ---- input: a temporary SDL event filter ------------------------------------------------------ */

typedef struct k2m_input {
    Uint64 deadline;                    /* performance counter before which input is dropped */
    volatile int skip, quit, unfocused;
} k2m_input;

/* While a movie runs every input event is swallowed: a release -- key, mouse button, joystick button,
   controller button, finger -- requests a skip once the input delay has passed, and SDL_QUIT requests
   a skip and a quit. Window events pass through so the engine still sees resizes and focus changes. */
static int SDLCALL k2m_filter(void* user, SDL_Event* e)
{
    k2m_input* in = (k2m_input*)user;
    switch (e->type) {
    case SDL_WINDOWEVENT:
        if (e->window.event == SDL_WINDOWEVENT_FOCUS_LOST) in->unfocused = 1;
        else if (e->window.event == SDL_WINDOWEVENT_FOCUS_GAINED) in->unfocused = 0;
        return 1;
    case SDL_QUIT:
        in->skip = in->quit = 1;
        return 0;
    case SDL_KEYUP:
    case SDL_MOUSEBUTTONUP:
    case SDL_JOYBUTTONUP:
    case SDL_CONTROLLERBUTTONUP:
    case SDL_FINGERUP:
        if (SDL_GetPerformanceCounter() >= in->deadline) in->skip = 1;
        return 0;
    default:
        return 0;
    }
}

/* ---- the media clock -------------------------------------------------------------------------- */

/* Seconds of movie time. With audio, the device's consumed-sample count is the master; while the
   device is starved, or there is no audio, the clock runs on from the last reading on the wall clock,
   so a short audio track or a slow decode never stalls or races the picture. */
typedef struct k2m_clock {
    SDL_AudioDeviceID dev;
    unsigned rate, channels;
    Uint64   queued;                    /* sample frames ever queued */
    double   anchor_media, freq;
    Uint64   anchor_wall;
} k2m_clock;

static double k2m_now(k2m_clock* c)
{
    Uint64 wall = SDL_GetPerformanceCounter();
    if (c->dev && c->queued) {
        Uint64 pending = SDL_GetQueuedAudioSize(c->dev) / (c->channels * sizeof(float));
        if (pending > 0) {
            c->anchor_media = (double)(c->queued - pending) / c->rate;
            c->anchor_wall = wall;
            return c->anchor_media;
        }
    }
    return c->anchor_media + (double)(wall - c->anchor_wall) / c->freq;
}

/* Starts, or resumes, at the clock's current reading; a pause freezes it where it stands. */
static void k2m_pause(k2m_clock* c, int paused)
{
    if (paused) c->anchor_media = k2m_now(c);
    else c->anchor_wall = SDL_GetPerformanceCounter();
    if (c->dev) SDL_PauseAudioDevice(c->dev, paused);
}

/* ---- the player ------------------------------------------------------------------------------- */

void k2movie_default_opts(k2movie_opts* o)
{
    memset(o, 0, sizeof *o);
    o->volume = 1.0f;
    o->input_delay_ms = 250;
    o->skip_min_frames = K2MOVIE_SKIP_AFTER_1S;
    o->pause_on_focus_loss = 1;
}

typedef struct k2m_player {
    k2m_gl         gl;
    k2bink*        bink;
    k2bink_info_t  info;
    k2bink_planes  planes;
    GLuint         tex[4], program, vbo;
    GLint          u_coord;
    int            vp[4];
    float*         abuf;
    int            track;
    float          volume;
    k2m_clock      clock;
} k2m_player;

enum { K2M_ABUF = 8192 };

static void k2m_texture(k2m_player* p, GLuint* t, unsigned w, unsigned h)
{
    const k2m_gl* gl = &p->gl;
    gl->GenTextures(1, t);
    gl->BindTexture(GL_TEXTURE_2D, *t);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->TexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, (GLsizei)w, (GLsizei)h, 0, GL_LUMINANCE,
                   GL_UNSIGNED_BYTE, NULL);
}

static void k2m_upload_plane(k2m_player* p, int unit, const uint8_t* src, int stride, unsigned w, unsigned h)
{
    const k2m_gl* gl = &p->gl;
    gl->ActiveTexture(GL_TEXTURE0 + unit);
    gl->BindTexture(GL_TEXTURE_2D, p->tex[unit]);
    gl->PixelStorei(GL_UNPACK_ROW_LENGTH, stride);
    gl->TexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)w, (GLsizei)h, GL_LUMINANCE, GL_UNSIGNED_BYTE, src);
}

/* Unit 0 = Y, 1 = cR (V), 2 = cB (U), 3 = alpha: the macOS player's sampler layout. */
static void k2m_upload(k2m_player* p)
{
    const k2bink_planes* f = &p->planes;
    unsigned cw = (f->width + 1) / 2, ch = (f->height + 1) / 2;
    k2m_upload_plane(p, 0, f->y, f->y_stride, f->width, f->height);
    k2m_upload_plane(p, 1, f->v, f->uv_stride, cw, ch);
    k2m_upload_plane(p, 2, f->u, f->uv_stride, cw, ch);
    if (f->a) k2m_upload_plane(p, 3, f->a, f->a_stride, f->width, f->height);
}

/* Letterboxes the movie into a target of tw x th, keeping its aspect. */
static void k2m_fit(k2m_player* p, int tw, int th)
{
    double va = (double)p->info.width / p->info.height, ta = (double)tw / th;
    double sx = ta > va ? va / ta : 1.0, sy = ta > va ? 1.0 : ta / va;
    p->vp[2] = (int)(tw * sx + 0.5);
    p->vp[3] = (int)(th * sy + 0.5);
    p->vp[0] = (tw - p->vp[2]) / 2;
    p->vp[1] = (th - p->vp[3]) / 2;
    p->gl.Viewport(0, 0, tw, th);
    /* texcoord (0,0) is the top-left of the picture */
    p->gl.Uniform4f(p->u_coord, (GLfloat)(2 * sx), (GLfloat)(-2 * sy), (GLfloat)-sx, (GLfloat)sy);
}

static int k2m_draw(k2m_player* p, SDL_Window* win, const k2movie_opts* o)
{
    const k2m_gl* gl = &p->gl;
    int tw = o->target_w, th = o->target_h;
    if (!tw || !th) SDL_GL_GetDrawableSize(win, &tw, &th);
    if (tw <= 0 || th <= 0) return 0;
    k2m_fit(p, tw, th);
    gl->Clear(GL_COLOR_BUFFER_BIT);
    gl->DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    int stop = 0;
    if (o->after_draw) {
        k2movie_frame fr;
        fr.planes = &p->planes;
        fr.info = &p->info;
        fr.seconds = (double)p->planes.frame_index * p->info.fps_den / p->info.fps_num;
        fr.vp_x = p->vp[0]; fr.vp_y = p->vp[1]; fr.vp_w = p->vp[2]; fr.vp_h = p->vp[3];
        /* the hook gets a fixed-function context (the engine's text renderer draws subtitles) */
        gl->UseProgram(0);
        gl->DisableVertexAttribArray(0);
        gl->BindBuffer(GL_ARRAY_BUFFER, 0);
        stop = o->after_draw(o->user, &fr);
        /* the hook may have drawn with its own state (subtitles); put ours back */
        gl->UseProgram(p->program);
        gl->BindBuffer(GL_ARRAY_BUFFER, p->vbo);
        gl->VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        gl->EnableVertexAttribArray(0);
        for (int i = 0; i < 4; ++i) {
            gl->ActiveTexture(GL_TEXTURE0 + i);
            gl->BindTexture(GL_TEXTURE_2D, p->tex[i]);
        }
    }
    if (!o->target_w) K2M_GL_SWAP(win);
    return stop;
}

/* Decode the next container frame and queue its audio. */
static int k2m_decode(k2m_player* p)
{
    int r = k2bink_decode_video(p->bink, &p->planes);
    if (r != K2BINK_OK) return r;
    for (unsigned t = 0; t < p->info.audio_tracks; ++t) {
        int n;
        while ((n = k2bink_decode_audio(p->bink, (int)t, p->abuf, K2M_ABUF)) > 0) {
            if (!p->clock.dev || (int)t != p->track) continue;   /* drain the other tracks */
            unsigned ch = p->clock.channels;
            for (int i = 0; i < n * (int)ch; ++i) p->abuf[i] *= p->volume;
            SDL_QueueAudio(p->clock.dev, p->abuf, (Uint32)(n * ch * sizeof(float)));
            p->clock.queued += (Uint64)n;
        }
    }
    return K2BINK_OK;
}

/* Pump input until movie time `due`. Returns 1 when the user ended the movie. */
static int k2m_wait(k2m_player* p, k2m_input* in, double due, int64_t shown_index, int64_t skip_from,
                    const k2movie_opts* o)
{
    int paused = 0;
    for (;;) {
        SDL_PumpEvents();
        int want_pause = o->pause_on_focus_loss && in->unfocused;
        if (want_pause != paused) k2m_pause(&p->clock, paused = want_pause);
        if (in->quit) return 1;
        /* Android: a skip is honoured only once frame `skip_from` is on screen; until then the
           request stays pending. */
        if (in->skip && shown_index >= skip_from) return 1;
        if (paused) { SDL_Delay(10); continue; }
        double now = k2m_now(&p->clock);
        if (now >= due) return 0;
        double wait = due - now;
        SDL_Delay(wait > 0.004 ? (Uint32)(wait * 1000 * 0.5) : 1);
    }
}

static int k2m_audio_open(k2m_player* p)
{
    if (p->track < 0 || (unsigned)p->track >= p->info.audio_tracks) return 0;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "k2movie: no audio: %s\n", SDL_GetError());
        return 0;
    }
    SDL_AudioSpec want, have;
    memset(&want, 0, sizeof want);
    want.freq = (int)p->info.track[p->track].sample_rate;
    want.format = AUDIO_F32SYS;
    want.channels = (Uint8)p->info.track[p->track].channels;
    want.samples = 1024;
    p->clock.dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);   /* SDL converts for us */
    if (!p->clock.dev) {
        fprintf(stderr, "k2movie: no audio device: %s\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return 0;
    }
    p->clock.rate = (unsigned)want.freq;
    p->clock.channels = want.channels;
    return 1;
}

static void k2m_audio_close(k2m_player* p)
{
    if (!p->clock.dev) return;
    SDL_CloseAudioDevice(p->clock.dev);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    p->clock.dev = 0;
}

static int k2m_gl_setup(k2m_player* p, const k2movie_opts* o)
{
    const k2m_gl* gl = &p->gl;
    GLuint vs = k2m_shader(gl, GL_VERTEX_SHADER, k_vs), fs = k2m_shader(gl, GL_FRAGMENT_SHADER, k_fs);
    GLint linked = 0;
    if (vs && fs) {
        p->program = gl->CreateProgram();
        gl->AttachShader(p->program, vs);
        gl->AttachShader(p->program, fs);
        gl->BindAttribLocation(p->program, 0, "a_position");
        gl->LinkProgram(p->program);
        gl->GetProgramiv(p->program, GL_LINK_STATUS, &linked);
        if (!linked) {
            char log[1024];
            gl->GetProgramInfoLog(p->program, sizeof log, NULL, log);
            fprintf(stderr, "k2movie: shader link failed: %s\n", log);
        }
    }
    if (vs) gl->DeleteShader(vs);
    if (fs) gl->DeleteShader(fs);
    if (!linked) {
        if (p->program) gl->DeleteProgram(p->program);
        p->program = 0;
        return 0;
    }

    if (!o->target_w) {
        if (gl->BindFramebuffer) gl->BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl->DrawBuffer(GL_BACK);
    }
    gl->UseProgram(p->program);
    gl->Uniform1i(gl->GetUniformLocation(p->program, "YTex"), 0);
    gl->Uniform1i(gl->GetUniformLocation(p->program, "cRTex"), 1);
    gl->Uniform1i(gl->GetUniformLocation(p->program, "cBTex"), 2);
    /* without alpha, point ATex at a live texture: an empty unit trips driver validation */
    gl->Uniform1i(gl->GetUniformLocation(p->program, "ATex"), p->info.has_alpha ? 3 : 0);
    gl->Uniform4fv(gl->GetUniformLocation(p->program, "crc"), 1, k_crc);
    gl->Uniform4fv(gl->GetUniformLocation(p->program, "cbc"), 1, k_cbc);
    gl->Uniform4fv(gl->GetUniformLocation(p->program, "adj"), 1, k_adj);
    gl->Uniform4fv(gl->GetUniformLocation(p->program, "yscale"), 1, k_yscale);
    gl->Uniform1f(gl->GetUniformLocation(p->program, "has_alpha"), p->info.has_alpha ? 1.0f : 0.0f);
    p->u_coord = gl->GetUniformLocation(p->program, "coord_xy");

    unsigned w = p->info.width, h = p->info.height;
    k2m_texture(p, &p->tex[0], w, h);
    k2m_texture(p, &p->tex[1], (w + 1) / 2, (h + 1) / 2);
    k2m_texture(p, &p->tex[2], (w + 1) / 2, (h + 1) / 2);
    if (p->info.has_alpha) k2m_texture(p, &p->tex[3], w, h);
    for (int i = 0; i < 4; ++i) {
        gl->ActiveTexture(GL_TEXTURE0 + i);
        gl->BindTexture(GL_TEXTURE_2D, p->tex[i]);
    }
    gl->BindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    gl->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gl->PixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    gl->PixelStorei(GL_UNPACK_SKIP_PIXELS, 0);

    gl->GenBuffers(1, &p->vbo);
    gl->BindBuffer(GL_ARRAY_BUFFER, p->vbo);
    gl->BufferData(GL_ARRAY_BUFFER, sizeof k_quad, k_quad, GL_STATIC_DRAW);
    gl->VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    gl->EnableVertexAttribArray(0);

    gl->Disable(GL_CULL_FACE);
    gl->Disable(GL_DEPTH_TEST);
    gl->Disable(GL_SCISSOR_TEST);
    gl->Disable(GL_STENCIL_TEST);
    gl->Disable(GL_ALPHA_TEST);
    gl->ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    if (p->info.has_alpha) {
        gl->Enable(GL_BLEND);
        gl->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        gl->Disable(GL_BLEND);
    }
    gl->ClearColor(0, 0, 0, 1);
    return 1;
}

static void k2m_gl_teardown(k2m_player* p, SDL_Window* win, const k2movie_opts* o)
{
    const k2m_gl* gl = &p->gl;
    /* leave the screen black rather than the last frame under the engine's next draw */
    if (!o->target_w) {
        gl->Clear(GL_COLOR_BUFFER_BIT);
        K2M_GL_SWAP(win);
    }
    gl->DisableVertexAttribArray(0);
    gl->DeleteBuffers(1, &p->vbo);
    gl->DeleteTextures(p->info.has_alpha ? 4 : 3, p->tex);
    gl->UseProgram(0);
    gl->DeleteProgram(p->program);
}

/* Runs the decode and present loop. Frame 0 is already decoded, and its audio queued, on entry. */
static void k2m_run(k2m_player* p, SDL_Window* win, const k2movie_opts* o, k2movie_result* res,
                    const char* path)
{
    k2m_input in;
    memset(&in, 0, sizeof in);
    in.deadline = SDL_GetPerformanceCounter() +
                  (Uint64)o->input_delay_ms * (SDL_GetPerformanceFrequency() / 1000);
    SDL_EventFilter prev_filter = NULL;
    void* prev_user = NULL;
    SDL_GetEventFilter(&prev_filter, &prev_user);
    SDL_SetEventFilter(k2m_filter, &in);   /* also drops the queued events our filter rejects */
    SDL_bool saver = SDL_IsScreenSaverEnabled();
    SDL_DisableScreenSaver();

    int64_t skip_from = o->skip_min_frames == K2MOVIE_SKIP_AFTER_1S
                         ? (int64_t)(p->info.fps_num / p->info.fps_den)
                         : o->skip_min_frames;
    double spf = (double)p->info.fps_den / p->info.fps_num;
    int64_t shown = -1;
    int r = K2BINK_OK;

    p->clock.freq = (double)SDL_GetPerformanceFrequency();
    p->clock.anchor_media = 0;
    k2m_pause(&p->clock, 0);   /* start the device and the clock at movie time 0 */
    for (;;) {
        if (k2m_wait(p, &in, p->planes.frame_index * spf, shown, skip_from, o)) { res->skipped = 1; break; }
        k2m_upload(p);
        shown = (int64_t)p->planes.frame_index;
        res->frames_shown++;
        if (k2m_draw(p, win, o)) break;
        /* decode ahead of the frame's due time, so its audio is queued before the device needs it */
        if ((r = k2m_decode(p)) != K2BINK_OK) break;
    }
    if (r == K2BINK_EOF) {
        /* hold the last frame for its duration */
        if (k2m_wait(p, &in, (shown + 1) * spf, shown, skip_from, o)) res->skipped = 1;
    } else if (r != K2BINK_OK) {
        res->error = r;
        fprintf(stderr, "k2movie: %s: decode error after frame %" PRId64 ": %s\n", path, shown, k2bink_strerror(r));
    }
    res->quit = in.quit;

    if (saver) SDL_EnableScreenSaver();
    SDL_SetEventFilter(prev_filter, prev_user);
    if (in.quit) {   /* the engine still gets to see the quit request */
        SDL_Event q;
        memset(&q, 0, sizeof q);
        q.type = SDL_QUIT;
        SDL_PushEvent(&q);
    }
}

/* The file comes from `path` when `io` is NULL, else from `io`/`io_user`; `path` names it in logs. */
static int k2m_play(const char* path, const k2bink_io* io, void* io_user, const k2movie_opts* opts,
                    k2movie_result* result)
{
    k2movie_opts defaults;
    k2movie_result res;
    k2m_player* p = NULL;
    memset(&res, 0, sizeof res);
    if (!opts) { k2movie_default_opts(&defaults); opts = &defaults; }

    SDL_Window* win = SDL_GL_GetCurrentWindow();
    if (!path || !win || !SDL_GL_GetCurrentContext()) { res.error = K2BINK_ERR_ARG; goto out; }
    if (!(p = (k2m_player*)calloc(1, sizeof *p))) { res.error = K2BINK_ERR_NOMEM; goto out; }
    if (!k2m_gl_load(&p->gl)) { res.error = K2BINK_ERR_ARG; goto out; }

    if (!(p->bink = io ? k2bink_open(io, io_user, &res.error) : k2bink_open_file(path, &res.error))) goto out;
    k2bink_info(p->bink, &p->info);
    if (!p->info.fps_num || !p->info.fps_den || !p->info.frames) { res.error = K2BINK_ERR_FORMAT; goto out; }
    if (!(p->abuf = (float*)malloc(sizeof(float) * K2M_ABUF * 2))) { res.error = K2BINK_ERR_NOMEM; goto out; }
    p->volume = opts->volume;
    p->track = opts->audio_track;

    k2m_audio_open(p);
    /* Frame 0 before any GL work: a file that cannot show a picture never touches the context. */
    if ((res.error = k2m_decode(p)) == K2BINK_OK) {
        k2m_glstate saved;
        k2m_save(&p->gl, &saved);
        if (k2m_gl_setup(p, opts)) {
            res.opened = 1;
            k2m_run(p, win, opts, &res, path);
            k2m_gl_teardown(p, win, opts);
        } else {
            res.error = K2BINK_ERR_ARG;
        }
        k2m_restore(&p->gl, &saved);
    }
    k2m_audio_close(p);

out:
    if (p) {
        if (p->bink) k2bink_close(p->bink);
        free(p->abuf);
        free(p);
    }
    if (result) *result = res;
    return res.error;
}

int k2movie_play(const char* path, const k2movie_opts* opts, k2movie_result* result)
{
    return k2m_play(path, NULL, NULL, opts, result);
}

int k2movie_play_io(const k2bink_io* io, void* io_user, const char* name, const k2movie_opts* opts,
                    k2movie_result* result)
{
    if (!io) {
        k2movie_result res;
        memset(&res, 0, sizeof res);
        res.error = K2BINK_ERR_ARG;
        if (result) *result = res;
        return res.error;
    }
    return k2m_play(name ? name : "(stream)", io, io_user, opts, result);
}
