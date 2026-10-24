/*
 * k2bink internals shared by the container, video and audio parts.
 * SPDX-License-Identifier: MIT OR Apache-2.0
 */
#ifndef K2BINK_INTERNAL_H
#define K2BINK_INTERNAL_H

#include "k2bink.h"

#define K2BINK_PAD 8   /* zero bytes after every packet so the bit reader may over-read */

typedef struct k2bink_track {
    k2bink_track_info info;
    unsigned id;
    unsigned flags;
    void*    dec;       /* k2bink_audio.c state */
    float*   q;         /* decoded, interleaved, not yet drained */
    size_t   q_len, q_pos, q_cap;   /* in samples */
} k2bink_track;

struct k2bink {
    k2bink_io io;
    void*     user;
    k2bink_info_t info;
    unsigned  video_flags;
    unsigned  largest_frame;
    uint32_t* index;    /* frames+1 entries; bit 0 = keyframe */
    uint8_t*  pkt;      /* largest_frame + K2BINK_PAD */
    unsigned  next;     /* next frame to decode */
    k2bink_track track[K2BINK_MAX_TRACKS];
    void*     vdec;     /* k2bink_video.c state */
    void*     own_file; /* FILE* from k2bink_open_file, closed by k2bink_close */
};

void* k2b_alloc(k2bink* b, size_t n);        /* zeroed */
void  k2b_free(k2bink* b, void* p);

/* k2bink_video.c */
int  k2b_video_init(k2bink* b);
int  k2b_video_decode(k2bink* b, const uint8_t* data, size_t size, k2bink_planes* out);
void k2b_video_free(k2bink* b);

/* k2bink_audio.c: decode one packet and append to t->q */
int  k2b_audio_init(k2bink* b, k2bink_track* t);
int  k2b_audio_decode(k2bink* b, k2bink_track* t, const uint8_t* data, size_t size);
void k2b_audio_reset(k2bink* b, k2bink_track* t);
void k2b_audio_free(k2bink* b, k2bink_track* t);
float* k2b_audio_reserve(k2bink* b, k2bink_track* t, size_t n);   /* k2bink.c */

#endif
