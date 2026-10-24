/*
 * k2bink -- a Bink 1 (revisions d, f..i) decoder in C99, no dependencies.
 *
 * A C port of unbikit (https://github.com/blennies/unbikit).
 * Copyright 2025 David Hart. C port copyright 2026 the KOTORDecomp contributors.
 * SPDX-License-Identifier: MIT OR Apache-2.0
 *
 * Usage:
 *   k2bink* b = k2bink_open(&io, user, &err);          // parses the container header and frame index
 *   k2bink_info_t info; k2bink_info(b, &info);
 *   while (k2bink_decode_video(b, &planes) == K2BINK_OK) {
 *       // planes.y/u/v(/a) stay valid until the next decode_video / seek / close
 *       for (t = 0; t < info.audio_tracks; ++t)
 *           while ((n = k2bink_decode_audio(b, t, buf, cap)) > 0) ...;
 *   }
 *   k2bink_close(b);
 *
 * Each call to k2bink_decode_video consumes one container frame: it decodes that frame's video and
 * queues the audio packets stored alongside it. k2bink_decode_audio drains a track's queue as
 * interleaved float samples in [-1, 1]. Audio for frame N is stored with frame N but is not
 * necessarily *played* at frame N's time -- the player clocks video to the audio device.
 */
#ifndef K2BINK_H
#define K2BINK_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct k2bink k2bink;

enum {
    K2BINK_OK          = 0,
    K2BINK_EOF         = 1,   /* no more frames (decode_video) */
    K2BINK_ERR_IO      = -1,
    K2BINK_ERR_FORMAT  = -2,  /* not a Bink file, or a corrupt one */
    K2BINK_ERR_UNSUPP  = -3,  /* Bink 2, or Bink 1 revision b, c or e */
    K2BINK_ERR_NOMEM   = -4,
    K2BINK_ERR_ARG     = -5,
    K2BINK_ERR_UNIMPL  = -6   /* a decoder stage with no implementation */
};

/* Byte source. read returns bytes read (short only at EOF); seek returns 0 on success. */
typedef struct k2bink_io {
    size_t (*read)(void* user, void* dst, size_t n);
    int    (*seek)(void* user, uint64_t offset);
    /* optional allocator; NULL => malloc/free */
    void*  (*alloc)(void* user, size_t n);
    void   (*free)(void* user, void* p);
} k2bink_io;

typedef struct k2bink_track_info {
    unsigned sample_rate;
    unsigned channels;          /* output channels after de-interleave (1 or 2) */
    int      uses_dct;          /* 1 = DCT audio, 0 = RDFT audio */
} k2bink_track_info;

#define K2BINK_MAX_TRACKS 8

typedef struct k2bink_info_t {
    char     revision;          /* 'd', 'f'..'i' */
    unsigned width, height;
    unsigned fps_num, fps_den;
    unsigned frames;
    int      has_alpha;
    int      grayscale;
    unsigned audio_tracks;
    k2bink_track_info track[K2BINK_MAX_TRACKS];
} k2bink_info_t;

/* 8-bit planar YUV 4:2:0 (BT.601, limited range). u/v are (w+1)/2 x (h+1)/2. a is NULL unless
 * has_alpha. Buffers are owned by the decoder. */
typedef struct k2bink_planes {
    const uint8_t* y; const uint8_t* u; const uint8_t* v; const uint8_t* a;
    int y_stride, uv_stride, a_stride;
    unsigned width, height;
    unsigned frame_index;       /* index of the frame just decoded */
    int      keyframe;
} k2bink_planes;

/* err (optional) receives K2BINK_OK or the reason open returned NULL */
k2bink* k2bink_open(const k2bink_io* io, void* user, int* err);
int     k2bink_info(const k2bink* b, k2bink_info_t* out);
int     k2bink_decode_video(k2bink* b, k2bink_planes* out);
/* max_frames is in sample frames (one sample per channel); returns frames written, 0 when the
 * track's queue is empty, <0 on error. */
int     k2bink_decode_audio(k2bink* b, int track, float* out, int max_frames);
/* seek to the last keyframe at or before `frame`; the next decode_video returns that keyframe.
 * Returns the frame index landed on, or <0. Clears the audio queues. */
int     k2bink_seek_keyframe(k2bink* b, unsigned frame);
void    k2bink_close(k2bink* b);

#ifndef K2BINK_NO_STDIO
/* convenience: a FILE*-backed source */
k2bink* k2bink_open_file(const char* path, int* err);
#endif

const char* k2bink_strerror(int err);

#ifdef __cplusplus
}
#endif
#endif
