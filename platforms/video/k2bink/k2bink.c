/*
 * k2bink -- container parsing and the public API.
 *
 * Written from the public format description (https://wiki.multimedia.cx/index.php/Bink_Container).
 * SPDX-License-Identifier: MIT OR Apache-2.0
 */
#include "k2bink_internal.h"

#include <stdlib.h>
#include <string.h>
#ifndef K2BINK_NO_STDIO
#include <stdio.h>
#endif

/* video flags */
#define BIK_VF_ALPHA     0x00100000u
#define BIK_VF_GRAYSCALE 0x00020000u
/* audio track flags */
#define BIK_AF_DCT       0x1000u
#define BIK_AF_STEREO    0x2000u

void* k2b_alloc(k2bink* b, size_t n)
{
    void* p = b->io.alloc ? b->io.alloc(b->user, n) : malloc(n);
    if (p) memset(p, 0, n);
    return p;
}

void k2b_free(k2bink* b, void* p)
{
    if (!p) return;
    if (b->io.free) b->io.free(b->user, p); else free(p);
}

static uint32_t rd32(const uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
static uint16_t rd16(const uint8_t* p) { return (uint16_t)(p[0] | p[1] << 8); }

static int read_exact(k2bink* b, void* dst, size_t n)
{
    return b->io.read(b->user, dst, n) == n ? K2BINK_OK : K2BINK_ERR_IO;
}

static int parse_header(k2bink* b)
{
    uint8_t h[44];
    int r = read_exact(b, h, sizeof h);
    if (r) return r;
    if (h[0] == 'K' && h[1] == 'B' && h[2] == '2') return K2BINK_ERR_UNSUPP;   /* Bink 2 */
    if (h[0] != 'B' || h[1] != 'I' || h[2] != 'K') return K2BINK_ERR_FORMAT;
    /* revisions 'd' to 'i' except 'e'; 'b' packs a different bitstream, 'c' and 'e' are untested */
    if (h[3] < 'd' || h[3] > 'i' || h[3] == 'e') return K2BINK_ERR_UNSUPP;

    k2bink_info_t* in = &b->info;
    in->revision = (char)h[3];
    in->frames = rd32(h + 8);
    b->largest_frame = rd32(h + 12);
    in->width = rd32(h + 20);
    in->height = rd32(h + 24);
    in->fps_num = rd32(h + 28);
    in->fps_den = rd32(h + 32);
    b->video_flags = rd32(h + 36);
    in->audio_tracks = rd32(h + 40);
    in->has_alpha = !!(b->video_flags & BIK_VF_ALPHA);
    in->grayscale = !!(b->video_flags & BIK_VF_GRAYSCALE);

    if (!in->frames || in->frames > 1000000 || !in->width || in->width > 7680 || !in->height ||
        in->height > 4800 || !in->fps_num || !in->fps_den || b->largest_frame > (64u << 20))
        return K2BINK_ERR_FORMAT;
    if (in->audio_tracks > K2BINK_MAX_TRACKS) return K2BINK_ERR_UNSUPP;

    /* per track: a u32 (max decoded packet size; unused here), then u16 rate + u16 flags, then u32 id */
    uint8_t t[K2BINK_MAX_TRACKS * 12];
    unsigned nt = in->audio_tracks;
    if (nt && (r = read_exact(b, t, nt * 12))) return r;
    for (unsigned i = 0; i < nt; ++i) {
        k2bink_track* tr = &b->track[i];
        tr->info.sample_rate = rd16(t + nt * 4 + i * 4);
        tr->flags = rd16(t + nt * 4 + i * 4 + 2);
        tr->id = rd32(t + nt * 8 + i * 4);
        tr->info.uses_dct = !!(tr->flags & BIK_AF_DCT);
        tr->info.channels = (tr->flags & BIK_AF_STEREO) ? 2 : 1;
        in->track[i] = tr->info;
    }

    /* frame index: frames+1 little-endian offsets; bit 0 flags a keyframe */
    size_t n = (size_t)in->frames + 1;
    b->index = k2b_alloc(b, n * 4);
    if (!b->index) return K2BINK_ERR_NOMEM;
    if ((r = read_exact(b, b->index, n * 4))) return r;
    for (size_t i = 0; i < n; ++i) b->index[i] = rd32((const uint8_t*)&b->index[i]);
    for (size_t i = 0; i < in->frames; ++i)
        if ((b->index[i + 1] & ~1u) <= (b->index[i] & ~1u) ||
            (b->index[i + 1] & ~1u) - (b->index[i] & ~1u) > b->largest_frame)
            return K2BINK_ERR_FORMAT;

    b->pkt = k2b_alloc(b, (size_t)b->largest_frame + K2BINK_PAD);
    return b->pkt ? K2BINK_OK : K2BINK_ERR_NOMEM;
}

k2bink* k2bink_open(const k2bink_io* io, void* user, int* err)
{
    int r = K2BINK_ERR_ARG;
    k2bink* b = NULL;
    if (!io || !io->read || !io->seek) goto out;
    b = io->alloc ? io->alloc(user, sizeof *b) : malloc(sizeof *b);
    r = K2BINK_ERR_NOMEM;
    if (!b) goto out;
    memset(b, 0, sizeof *b);
    b->io = *io;
    b->user = user;
    if ((r = parse_header(b))) goto fail;
    if ((r = k2b_video_init(b))) goto fail;
    for (unsigned i = 0; i < b->info.audio_tracks; ++i)
        if ((r = k2b_audio_init(b, &b->track[i]))) goto fail;
    r = K2BINK_OK;
    goto out;
fail:
    k2bink_close(b);
    b = NULL;
out:
    if (err) *err = r;
    return b;
}

int k2bink_info(const k2bink* b, k2bink_info_t* out)
{
    if (!b || !out) return K2BINK_ERR_ARG;
    *out = b->info;
    return K2BINK_OK;
}

static int queue_push(k2bink* b, k2bink_track* t, size_t need)
{
    if (t->q_pos && t->q_pos == t->q_len) t->q_pos = t->q_len = 0;
    if (t->q_len + need <= t->q_cap) return K2BINK_OK;
    size_t live = t->q_len - t->q_pos, cap = t->q_cap ? t->q_cap : 4096;
    while (cap < live + need) cap *= 2;
    float* q = k2b_alloc(b, cap * sizeof(float));
    if (!q) return K2BINK_ERR_NOMEM;
    if (live) memcpy(q, t->q + t->q_pos, live * sizeof(float));
    k2b_free(b, t->q);
    t->q = q; t->q_cap = cap; t->q_pos = 0; t->q_len = live;
    return K2BINK_OK;
}

/* called by k2bink_audio.c: reserve room for n more samples, returns where to write them */
float* k2b_audio_reserve(k2bink* b, k2bink_track* t, size_t n)
{
    if (queue_push(b, t, n)) return NULL;
    float* p = t->q + t->q_len;
    t->q_len += n;
    return p;
}

int k2bink_decode_video(k2bink* b, k2bink_planes* out)
{
    if (!b || !out) return K2BINK_ERR_ARG;
    if (b->next >= b->info.frames) return K2BINK_EOF;
    uint32_t off = b->index[b->next] & ~1u, end = b->index[b->next + 1] & ~1u;
    size_t size = end - off;
    int r;
    if (b->io.seek(b->user, off)) return K2BINK_ERR_IO;
    if ((r = read_exact(b, b->pkt, size))) return r;
    memset(b->pkt + size, 0, K2BINK_PAD);

    /* per track: u32 packet size; a non-empty packet starts with a u32 decoded-sample count */
    const uint8_t* p = b->pkt;
    size_t left = size;
    for (unsigned i = 0; i < b->info.audio_tracks; ++i) {
        if (left < 4) return K2BINK_ERR_FORMAT;
        uint32_t n = rd32(p);
        p += 4; left -= 4;
        if (n > left) return K2BINK_ERR_FORMAT;
        if (n >= 4 && (r = k2b_audio_decode(b, &b->track[i], p + 4, n - 4))) return r;
        p += n; left -= n;
    }
    if ((r = k2b_video_decode(b, p, left, out))) return r;
    out->frame_index = b->next;
    out->keyframe = b->index[b->next] & 1;
    ++b->next;
    return K2BINK_OK;
}

int k2bink_decode_audio(k2bink* b, int track, float* out, int max_frames)
{
    if (!b || track < 0 || (unsigned)track >= b->info.audio_tracks || !out || max_frames < 0) return K2BINK_ERR_ARG;
    k2bink_track* t = &b->track[track];
    size_t ch = t->info.channels, avail = (t->q_len - t->q_pos) / ch;
    size_t n = avail < (size_t)max_frames ? avail : (size_t)max_frames;
    memcpy(out, t->q + t->q_pos, n * ch * sizeof(float));
    t->q_pos += n * ch;
    return (int)n;
}

int k2bink_seek_keyframe(k2bink* b, unsigned frame)
{
    if (!b) return K2BINK_ERR_ARG;
    if (frame >= b->info.frames) frame = b->info.frames - 1;
    while (frame > 0 && !(b->index[frame] & 1)) --frame;
    b->next = frame;
    for (unsigned i = 0; i < b->info.audio_tracks; ++i) {
        b->track[i].q_pos = b->track[i].q_len = 0;
        k2b_audio_reset(b, &b->track[i]);
    }
    return (int)frame;
}

void k2bink_close(k2bink* b)
{
    if (!b) return;
    k2b_video_free(b);
    for (unsigned i = 0; i < K2BINK_MAX_TRACKS; ++i) {
        k2b_audio_free(b, &b->track[i]);
        k2b_free(b, b->track[i].q);
    }
    k2b_free(b, b->index);
    k2b_free(b, b->pkt);
    k2bink_io io = b->io;
    void* user = b->user;
#ifndef K2BINK_NO_STDIO
    if (b->own_file) fclose((FILE*)b->own_file);
#endif
    if (io.free) io.free(user, b); else free(b);
}

const char* k2bink_strerror(int err)
{
    switch (err) {
    case K2BINK_OK: return "ok";
    case K2BINK_EOF: return "end of stream";
    case K2BINK_ERR_IO: return "I/O error";
    case K2BINK_ERR_FORMAT: return "not a Bink file, or corrupt";
    case K2BINK_ERR_UNSUPP: return "unsupported Bink version";
    case K2BINK_ERR_NOMEM: return "out of memory";
    case K2BINK_ERR_ARG: return "bad argument";
    case K2BINK_ERR_UNIMPL: return "not implemented yet";
    default: return "unknown error";
    }
}

#ifndef K2BINK_NO_STDIO
static size_t f_read(void* u, void* d, size_t n) { return fread(d, 1, n, (FILE*)u); }
static int f_seek(void* u, uint64_t o) { return fseeko((FILE*)u, (off_t)o, SEEK_SET); }

k2bink* k2bink_open_file(const char* path, int* err)
{
    FILE* f = fopen(path, "rb");
    if (!f) { if (err) *err = K2BINK_ERR_IO; return NULL; }
    k2bink_io io = { f_read, f_seek, NULL, NULL };
    k2bink* b = k2bink_open(&io, f, err);
    if (!b) fclose(f);
    else b->own_file = f;
    return b;
}
#endif
