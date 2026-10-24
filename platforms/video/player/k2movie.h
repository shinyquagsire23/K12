/*
 * k2movie -- the blocking full-screen movie player behind ASL::PlayBinkMovieGL.
 *
 * k2bink decodes; this draws the planes with a YUV->RGB fragment shader into the current SDL GL
 * window, plays one audio track on an SDL audio device, slaves presentation to that device's clock,
 * and takes skip input through a temporary SDL event filter. It returns when the movie ends or is
 * skipped. Every piece of GL state it touches is saved first and restored on the way out, so the
 * engine's renderer sees the context exactly as it left it.
 *
 * The player combines the two behaviours the game shipped with: on Android the skip rule (not before
 * one second of video), subtitles at frame/fps and pausing while the window has lost focus; on macOS
 * the event filter (input ignored for the first N ms, every event swallowed while the movie runs),
 * the shader and its Bink 1 colour constants, aspect-fit and GL state save/restore.
 *
 * Needs: SDL2 with video initialised and a GL 2.1 (GLSL 1.20) context current. GL entry points are
 * loaded with SDL_GL_GetProcAddress, so nothing links libGL.
 *
 * SPDX-License-Identifier: MIT OR Apache-2.0
 */
#ifndef K2MOVIE_H
#define K2MOVIE_H

#include "k2bink.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Called after the video quad is drawn and before the buffer swap. Return nonzero to stop. */
typedef struct k2movie_frame {
    const k2bink_planes*  planes;      /* valid for this call only */
    const k2bink_info_t*  info;
    double                seconds;     /* frame_index * fps_den / fps_num */
    int vp_x, vp_y, vp_w, vp_h;        /* where the picture landed, in GL window coordinates */
} k2movie_frame;
typedef int (*k2movie_draw_hook)(void* user, const k2movie_frame* f);

typedef struct k2movie_opts {
    float    volume;                   /* linear gain, 1 = unchanged */
    int      audio_track;              /* <0 or out of range: silent, paced by the wall clock */
    unsigned input_delay_ms;           /* input before this is dropped; the engine passes 250 */
    int      skip_min_frames;          /* a skip lands only once this frame is on screen;
                                          K2MOVIE_SKIP_AFTER_1S = fps_num/fps_den (Android) */
    int      pause_on_focus_loss;      /* Android: stop the clock and the draw while unfocused */
    /* 0 = draw to the default framebuffer, sized to the window's drawable (what the engine wants).
       Nonzero = draw to whatever framebuffer is bound, at this size (test harnesses). */
    int      target_w, target_h;
    k2movie_draw_hook after_draw;
    void*    user;
} k2movie_opts;

#define K2MOVIE_SKIP_AFTER_1S (-1)

typedef struct k2movie_result {
    int      opened;                   /* the file decoded far enough to show a frame */
    int      skipped;                  /* ended by user input (or a quit request) */
    int      quit;                     /* an SDL_QUIT arrived; it is re-posted for the caller */
    unsigned frames_shown;
    int      error;                    /* K2BINK_* from open/decode, or K2BINK_ERR_ARG when no GL
                                          window/context is current or the shader failed */
} k2movie_result;

void k2movie_default_opts(k2movie_opts* o);

/* Play `path` to the end. Returns result->error. */
int k2movie_play(const char* path, const k2movie_opts* opts, k2movie_result* result);

/* The same, reading the file through `io` (an OBB entry, say); `io_user` must outlive the call.
   `name` is for log lines only. */
int k2movie_play_io(const k2bink_io* io, void* io_user, const char* name, const k2movie_opts* opts,
                    k2movie_result* result);

#ifdef __cplusplus
}
#endif
#endif
