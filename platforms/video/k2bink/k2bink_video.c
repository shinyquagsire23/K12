/*
 * k2bink -- Bink 1 video decoding (revisions d..i).
 *
 * A C port of unbikit's src/bik-video-decoder.ts and the 8x8 IDCT from src/transforms.ts
 * (https://github.com/blennies/unbikit). Copyright 2025 David Hart.
 * SPDX-License-Identifier: MIT OR Apache-2.0
 *
 * The bundle refill, the explicit-order tree read, the coefficient work-queue, the colour high-nibble
 * context and the IDCT follow the format's public description
 * (https://wiki.multimedia.cx/index.php/Bink_Video).
 *
 * A frame is up to four planes (alpha, Y, then the two chroma planes). Each plane is a grid of 8x8
 * blocks, the chroma planes in 16-pixel units of the luma size, walked row by row. Every block row
 * starts by topping up nine "bundles" -- per-row streams of block types, colours, motion offsets, DC
 * values -- and each block then pulls the values it needs from them.
 */
#include "k2bink_internal.h"
#include "k2bink_bits.h"
#include "k2bink_tables.h"

#include <string.h>

enum { BLK_SKIP, BLK_SCALED, BLK_MOTION, BLK_RUN, BLK_RESIDUE, BLK_INTRA, BLK_FILL, BLK_INTER,
       BLK_PATTERN, BLK_RAW };

/* bundle ids, in bitstream order */
enum { B_TYPE, B_SUBTYPE, B_COLOR, B_PATTERN, B_XOFF, B_YOFF, B_INTRA_DC, B_INTER_DC, B_RUN, B_COUNT };

/* ---- Huffman ----------------------------------------------------------------------------------
 * Bink uses 16 fixed prefix codes of 16 symbols each; the stream only picks one and permutes its
 * symbols. A code set is decoded with one table lookup of `max` peeked bits. */
typedef struct huff_set {
    uint8_t max;
    uint8_t lut[256];         /* peeked bits -> symbol << 4 | length */
} huff_set;

typedef struct tree {
    const huff_set* set;
    uint8_t sym[16];          /* code-set symbol -> value */
} tree;

static huff_set g_huff[16];
static int g_huff_ready;

static void huff_init(void)
{
    if (g_huff_ready) return;
    for (int s = 0; s < 16; ++s) {
        const uint8_t* codes = bink_tree_codes + s * 16;
        const uint8_t* lens = bink_tree_lens + s * 16;
        huff_set* h = &g_huff[s];
        h->max = lens[15];
        for (int sym = 0; sym < 16; ++sym)
            for (unsigned hi = 0; hi < (1u << (h->max - lens[sym])); ++hi)
                h->lut[hi << lens[sym] | codes[sym]] = (uint8_t)(sym << 4 | lens[sym]);
    }
    g_huff_ready = 1;
}

static inline unsigned huff_get(k2b_bits* br, const tree* t)
{
    unsigned e = t->set->lut[k2b_peek(br, t->set->max)];
    k2b_skip(br, e & 15);
    return t->sym[e >> 4];
}

/* ---- bundles ---------------------------------------------------------------------------------- */
typedef struct bundle {
    tree     t;
    unsigned count_bits;      /* width of the per-row "values in this chunk" field */
    int      wide;            /* int16 storage (motion offsets, DCs) instead of uint8 */
    uint8_t* u8;
    int16_t* s16;
    size_t   cap;
    size_t   fill;            /* values decoded so far in this plane */
    size_t   take;            /* values consumed so far */
    int      finished;        /* a zero-length chunk ends the bundle for the rest of the plane */
} bundle;

typedef struct plane {
    uint8_t* cur;             /* being decoded */
    uint8_t* prev;            /* previous frame, for motion compensation */
    int      stride;
    int      rows;
    unsigned bw, bh;          /* size in 8x8 blocks */
} plane;

typedef struct vdec {
    int      rev;             /* 'd'..'i' */
    int      alpha;
    int      uv_swapped;
    bundle   b[B_COUNT];
    tree     hi_tree[16];     /* colour high nibble: one tree per previous high nibble */
    unsigned hi_ctx;
    plane    pl[4];           /* Y, U, V, A */
    uint8_t* mem[2];          /* the two frame buffers (all planes each) */
    size_t   mem_size;
    int32_t  coef[64];
    uint8_t  scaled[64];
    k2b_bits br;
} vdec;

static inline unsigned align(unsigned v, unsigned a) { return (v + a - 1) / a * a; }

static unsigned ilog2(unsigned v) { unsigned r = 0; while (v >>= 1) ++r; return r; }

static inline int bundle_next(bundle* b)
{
    if (b->take >= b->cap) return 0;
    return b->wide ? b->s16[b->take++] : b->u8[b->take++];
}

static inline void bundle_put(bundle* b, int v)
{
    if (b->wide) b->s16[b->fill++] = (int16_t)v;
    else b->u8[b->fill++] = (uint8_t)v;
}

/* Returns how many values the next chunk of this bundle holds, or 0 when no chunk is due: the
 * bundle already ended, or values from its previous chunk are still waiting to be consumed. A chunk
 * that would overflow the bundle sets *err. */
static unsigned chunk_len(vdec* d, bundle* b, int* err)
{
    if (b->finished || b->fill > b->take) return 0;
    unsigned n = k2b_get(&d->br, b->count_bits);
    if (!n) { b->finished = 1; return 0; }
    if (b->fill + n + 32 > b->cap) { *err = 1; return 0; }   /* +32: block-type runs overshoot */
    return n;
}

/* ---- trees ------------------------------------------------------------------------------------ */
static void read_tree(vdec* d, tree* t)
{
    k2b_bits* br = &d->br;
    unsigned set = k2b_get(br, 4);
    t->set = &g_huff[set];
    if (set == 0) {
        for (int i = 0; i < 16; ++i) t->sym[i] = (uint8_t)i;
        return;
    }
    if (k2b_get1(br)) {
        /* explicit order: the first n+1 symbols are listed, the rest follow in ascending order */
        unsigned listed = k2b_get(br, 3) + 1, used = 0, n = 0;
        while (n < listed) {
            unsigned s = k2b_get(br, 4);
            t->sym[n++] = (uint8_t)s;
            used |= 1u << s;
        }
        for (unsigned s = 0; s < 16 && n < 16; ++s)   /* n < 16: duplicates in a corrupt list */
            if (!(used & (1u << s))) t->sym[n++] = (uint8_t)s;
        return;
    }
    /* merge-sort shuffle: pairs are swapped by one bit each, then runs of 2, 4, 8 are merged, each
     * step picking its source run with one bit */
    unsigned passes = k2b_get(br, 2);
    uint8_t a[16], b[16];
    uint8_t *src = a, *dst = b;
    for (int i = 0; i < 16; i += 2) {
        unsigned swap = k2b_get1(br);
        src[i] = (uint8_t)(i + swap);
        src[i + 1] = (uint8_t)(i + !swap);
    }
    for (unsigned p = 1; p <= passes; ++p) {
        unsigned run = 1u << p;
        for (unsigned base = 0; base < 16; base += run * 2) {
            unsigned l = base, r = base + run, lend = base + run, rend = base + 2 * run, o = base;
            while (l < lend && r < rend) dst[o++] = k2b_get1(br) ? src[r++] : src[l++];
            while (l < lend) dst[o++] = src[l++];
            while (r < rend) dst[o++] = src[r++];
        }
        uint8_t* t2 = src; src = dst; dst = t2;
    }
    memcpy(t->sym, src, 16);
}

/* Starts a plane: sizes every bundle's count field for this plane's width, then reads the trees */
static void plane_begin(vdec* d, unsigned bw)
{
    static const unsigned mul[B_COUNT] = { 1, 0, 64, 8, 1, 1, 1, 1, 48 };
    for (int i = 0; i < B_COUNT; ++i) {
        bundle* b = &d->b[i];
        unsigned span = (i == B_SUBTYPE ? bw / 2 : bw * mul[i]) + 511;
        b->count_bits = ilog2(span) + 1;
        if (i == B_COLOR) {
            for (int h = 0; h < 16; ++h) read_tree(d, &d->hi_tree[h]);
            d->hi_ctx = 0;
        }
        if (i != B_INTRA_DC && i != B_INTER_DC) read_tree(d, &b->t);
        b->fill = b->take = 0;
        b->finished = 0;
    }
}

/* ---- per-row bundle refills ------------------------------------------------------------------- */
static int fill_types(vdec* d, bundle* b)
{
    static const unsigned run_len[4] = { 4, 8, 12, 32 };
    int err = 0;
    unsigned n = chunk_len(d, b, &err);
    if (!n) return err;
    if (k2b_get1(&d->br)) {
        unsigned v = k2b_get(&d->br, 4);
        while (n--) bundle_put(b, (int)v);
        return 0;
    }
    unsigned last = 0;
    for (unsigned done = 0; done < n;) {
        unsigned v = huff_get(&d->br, &b->t);
        if (v < 12) { last = v; bundle_put(b, (int)v); ++done; continue; }
        for (unsigned r = run_len[v - 12]; r; --r) bundle_put(b, (int)last);
        done += run_len[v - 12];
    }
    return 0;
}

static int fill_colors(vdec* d, bundle* b)
{
    int err = 0;
    unsigned n = chunk_len(d, b, &err);
    if (!n) return err;
    int one_value = k2b_get1(&d->br);
    for (unsigned i = 0; i < (one_value ? 1 : n); ++i) {
        /* the high nibble's code set depends on the previous colour's high nibble */
        unsigned hi = huff_get(&d->br, &d->hi_tree[d->hi_ctx]);
        unsigned v = hi << 4 | huff_get(&d->br, &b->t);
        d->hi_ctx = hi;
        if (d->rev < 'i') v = v >= 128 ? 256 - v : v + 128;   /* older revisions: sign-magnitude */
        if (one_value) { while (n--) bundle_put(b, (int)v); }
        else bundle_put(b, (int)v);
    }
    return 0;
}

static int fill_patterns(vdec* d, bundle* b)
{
    int err = 0;
    unsigned n = chunk_len(d, b, &err);
    while (n--) {
        unsigned lo = huff_get(&d->br, &b->t);
        bundle_put(b, (int)(lo | huff_get(&d->br, &b->t) << 4));
    }
    return err;
}

static int fill_motion(vdec* d, bundle* b)
{
    int err = 0;
    unsigned n = chunk_len(d, b, &err);
    if (!n) return err;
    if (k2b_get1(&d->br)) {
        int v = (int)k2b_get(&d->br, 4);
        if (v) v = k2b_signed(&d->br, v);
        while (n--) bundle_put(b, v);
        return 0;
    }
    while (n--) {
        int v = (int)huff_get(&d->br, &b->t);
        bundle_put(b, v ? k2b_signed(&d->br, v) : 0);
    }
    return 0;
}

/* DC values: a start value, then groups of up to 8 deltas sharing one bit width */
static int fill_dc(vdec* d, bundle* b, int is_signed)
{
    int err = 0;
    unsigned n = chunk_len(d, b, &err);
    if (!n) return err;
    int32_t v = (int32_t)k2b_get(&d->br, is_signed ? 10 : 11);
    if (v && is_signed) v = k2b_signed(&d->br, v);
    bundle_put(b, v);
    for (unsigned i = 1; i < n;) {
        unsigned group = n - i < 8 ? n - i : 8, width = k2b_get(&d->br, 4);
        for (unsigned j = 0; j < group; ++j) {
            if (width) {
                int32_t delta = (int32_t)k2b_get(&d->br, width);
                if (delta) v += k2b_signed(&d->br, delta);
            }
            bundle_put(b, v);
        }
        i += group;
    }
    return 0;
}

static int fill_runs(vdec* d, bundle* b)
{
    int err = 0;
    unsigned n = chunk_len(d, b, &err);
    if (!n) return err;
    if (k2b_get1(&d->br)) {
        unsigned v = k2b_get(&d->br, 4);
        while (n--) bundle_put(b, (int)v);
        return 0;
    }
    while (n--) bundle_put(b, (int)huff_get(&d->br, &b->t));
    return 0;
}

/* ---- coefficients ------------------------------------------------------------------------------
 * DCT coefficients and residues share one bit-plane coder. Coefficient positions (in zigzag order)
 * sit on a work queue; each queue entry is either a group of four positions, a group of sixteen
 * that splits into four groups of four, or a single position. A set bit on an entry means "has a
 * significant coefficient at this bit plane"; a group then reads one bit per position -- clear:
 * the coefficient is emitted now, set: the position is queued as a single for a later pass.
 * Singles are queued at the head, splits at the tail, and a pass never revisits head entries
 * queued during that pass. */
enum { Q_GROUP4, Q_GROUP16, Q_GROUP4_ONCE, Q_SINGLE, Q_DEAD };

typedef struct coef_queue {
    uint8_t pos[128];
    uint8_t kind[128];
    unsigned head, tail;
} coef_queue;

static void queue_init(coef_queue* q, int residue)
{
    static const uint8_t groups[3] = { 4, 24, 44 };
    q->head = q->tail = 64;
    for (int i = 0; i < 3; ++i) { q->pos[q->tail] = groups[i]; q->kind[q->tail++] = Q_GROUP4; }
    if (residue) { q->pos[q->tail] = 0; q->kind[q->tail++] = Q_GROUP4_ONCE; }
    else for (uint8_t p = 1; p <= 3; ++p) { q->pos[q->tail] = p; q->kind[q->tail++] = Q_SINGLE; }
}

typedef struct coef_sink {
    int      residue;
    int32_t* blk;
    unsigned bits;            /* DCT: magnitude bit count; residue: current bit-plane value */
    int      budget;          /* residue: remaining coefficient/refinement bits (-1 = exhausted) */
    uint8_t  nz[64];          /* block positions set so far */
    unsigned n_nz;
} coef_sink;

/* Emits the coefficient at zigzag position zz; nonzero when the residue budget runs out */
static int emit(k2b_bits* br, coef_sink* s, unsigned zz)
{
    unsigned at = bink_scan[zz];
    if (s->residue) {
        s->nz[s->n_nz++] = (uint8_t)at;
        s->blk[at] = k2b_signed(br, (int32_t)s->bits);
        return --s->budget < 0;
    }
    int32_t v = s->bits ? k2b_signed(br, (int32_t)(k2b_get(br, s->bits) | 1u << s->bits))
                        : (k2b_get1(br) ? -1 : 1);
    s->blk[at] = v;
    s->nz[s->n_nz++] = (uint8_t)at;
    return 0;
}

/* Makes one pass over the queue at the current bit plane; nonzero stops decoding */
static int queue_pass(k2b_bits* br, coef_queue* q, coef_sink* s)
{
    unsigned i = q->head;
    while (i < q->tail) {
        unsigned p = q->pos[i], k = q->kind[i];
        if (k == Q_DEAD || !k2b_get1(br)) { ++i; continue; }
        switch (k) {
        case Q_GROUP16:
            /* becomes a one-shot group of four; the other three quarters join the tail */
            q->kind[i] = Q_GROUP4_ONCE;
            for (unsigned o = 4; o < 16; o += 4) { q->pos[q->tail] = (uint8_t)(p + o); q->kind[q->tail++] = Q_GROUP4_ONCE; }
            continue;                           /* re-examine this entry in the same pass */
        case Q_SINGLE:
            q->kind[i++] = Q_DEAD;
            if (emit(br, s, p)) return 1;
            continue;
        default: /* Q_GROUP4, Q_GROUP4_ONCE */
            if (k == Q_GROUP4) { q->pos[i] = (uint8_t)(p + 4); q->kind[i] = Q_GROUP16; }   /* stays */
            else q->kind[i++] = Q_DEAD;
            for (unsigned o = 0; o < 4; ++o) {
                if (k2b_get1(br)) { --q->head; q->pos[q->head] = (uint8_t)(p + o); q->kind[q->head] = Q_SINGLE; }
                else if (emit(br, s, p + o)) return 1;
            }
            continue;
        }
    }
    return 0;
}

/* DCT block: blk[0] holds the DC on entry; quant is the 16-level table set (intra or inter) */
static void read_dct(vdec* d, int32_t* blk, const int32_t* quant)
{
    k2b_bits* br = &d->br;
    coef_queue q;
    coef_sink s = { 0, blk, 0, 0, { 0 }, 0 };
    queue_init(&q, 0);
    for (int bits = (int)k2b_get(br, 4) - 1; bits >= 0; --bits) {
        s.bits = (unsigned)bits;
        queue_pass(br, &q, &s);
    }
    const int32_t* qm = quant + (k2b_get(br, 4) << 6);   /* natural order */
    /* products may exceed 32 bits on corrupt input: wrap like the reference instead of UB */
    blk[0] = (int32_t)((uint32_t)blk[0] * (uint32_t)qm[0]) >> 11;
    for (unsigned i = 0; i < s.n_nz; ++i) {
        unsigned at = s.nz[i];
        blk[at] = (int32_t)((uint32_t)blk[at] * (uint32_t)qm[at]) >> 11;
    }
}

/* residue block: bit planes from 2^n down to 1, refining already-set coefficients first */
static void read_residue(vdec* d, int32_t* blk)
{
    k2b_bits* br = &d->br;
    coef_queue q;
    coef_sink s = { 1, blk, 0, 0, { 0 }, 0 };
    queue_init(&q, 1);
    s.budget = (int)k2b_get(br, 7);
    for (s.bits = 1u << k2b_get(br, 3); s.bits; s.bits >>= 1) {
        for (unsigned i = 0; i < s.n_nz; ++i) {
            if (!k2b_get1(br)) continue;
            int32_t* c = &blk[s.nz[i]];
            *c += *c < 0 ? -(int32_t)s.bits : (int32_t)s.bits;
            if (--s.budget < 0) return;
        }
        if (queue_pass(br, &q, &s)) return;
    }
}

/* ---- 8x8 IDCT ----------------------------------------------------------------------------------
 * The fixed-point 1-D transform, applied down the columns and then along the rows. Constants are
 * the cosines scaled by 2^11. Row outputs are rounded (+0x7f) and scaled down by 2^8. */
#define K_R2    2896    /* sqrt(2) */
#define K_A     2217    /* sqrt(2+sqrt2) - sqrt(2-sqrt2) */
#define K_B     3784    /* sqrt(2+sqrt2) */
#define K_C   (-5352)   /* -(sqrt(2+sqrt2) + sqrt(2-sqrt2)) */
#define FMUL(k, x) ((int32_t)((uint32_t)(k) * (uint32_t)(x)) >> 11)

static inline void idct1d(const int32_t* in, int in_step, int32_t* out, int out_step, int32_t bias, int shift)
{
    int32_t x0 = in[0] + bias, x1 = in[in_step], x2 = in[2 * in_step], x3 = in[3 * in_step];
    int32_t x4 = in[4 * in_step], x5 = in[5 * in_step], x6 = in[6 * in_step], x7 = in[7 * in_step];

    /* even half */
    int32_t s04 = x0 + x4, d04 = x0 - x4;
    int32_t s26 = x2 + x6, d26 = FMUL(K_R2, x2 - x6);
    int32_t ev0 = s04 + s26, ev3 = s04 - s26;
    int32_t ev1 = d04 + d26 - s26, ev2 = d04 - d26 + s26;

    /* odd half */
    int32_t s17 = x1 + x7, d17 = x1 - x7, s53 = x5 + x3, d53 = x5 - x3;
    int32_t rot = FMUL(K_B, d53 + d17);
    int32_t od0 = s17 + s53;
    int32_t od1 = FMUL(K_C, d53) - od0 + rot;
    int32_t od2 = FMUL(K_R2, s17 - s53) - od1;
    int32_t od3 = FMUL(K_A, d17) + od2 - rot;

    out[0]            = (ev0 + od0) >> shift;
    out[out_step]     = (ev1 + od1) >> shift;
    out[2 * out_step] = (ev2 + od2) >> shift;
    out[3 * out_step] = (ev3 - od3) >> shift;
    out[4 * out_step] = (ev3 + od3) >> shift;
    out[5 * out_step] = (ev2 - od2) >> shift;
    out[6 * out_step] = (ev1 - od1) >> shift;
    out[7 * out_step] = (ev0 - od0) >> shift;
}

/* Transforms blk in place: columns, then rows, the row outputs rounded and >> 8 */
static void idct8x8(int32_t* blk)
{
    int32_t tmp[8];
    for (int c = 0; c < 8; ++c) {
        idct1d(blk + c, 8, tmp, 1, 0, 0);
        for (int r = 0; r < 8; ++r) blk[c + 8 * r] = tmp[r];
    }
    for (int r = 0; r < 8; ++r) {
        idct1d(blk + 8 * r, 1, tmp, 1, 0x7f, 8);
        memcpy(blk + 8 * r, tmp, sizeof tmp);
    }
}

/* pixels wrap modulo 256 (no clamping), matching the reference decoders */
static void put_block(const int32_t* blk, uint8_t* dst, int stride)
{
    for (int r = 0; r < 8; ++r, dst += stride)
        for (int c = 0; c < 8; ++c) dst[c] = (uint8_t)blk[8 * r + c];
}

static void add_block(const int32_t* blk, uint8_t* dst, int stride)
{
    for (int r = 0; r < 8; ++r, dst += stride)
        for (int c = 0; c < 8; ++c) dst[c] = (uint8_t)(dst[c] + blk[8 * r + c]);
}

/* ---- blocks ----------------------------------------------------------------------------------- */
/* Copies the 8x8 block of the previous frame at `ref`, an offset into the plane, to `at` */
static int copy_prev(const plane* p, long at, long ref)
{
    if (ref < 0 || ref + 7L * p->stride + 8 > (long)p->stride * p->rows) return 1;
    if (ref == at) return 0;                 /* cur starts as a copy of prev */
    for (int r = 0; r < 8; ++r) memcpy(p->cur + at + r * p->stride, p->prev + ref + r * p->stride, 8);
    return 0;
}

static int motion_ref(vdec* d, const plane* p, long at, long* ref)
{
    int dx = bundle_next(&d->b[B_XOFF]);
    int dy = bundle_next(&d->b[B_YOFF]);
    *ref = at + dx + (long)dy * p->stride;
    return copy_prev(p, at, *ref);
}

static void blk_fill(vdec* d, uint8_t* dst, int stride, int size)
{
    uint8_t v = (uint8_t)bundle_next(&d->b[B_COLOR]);
    for (int r = 0; r < size; ++r) memset(dst + r * stride, v, (size_t)size);
}

static void blk_pattern(vdec* d, uint8_t* dst, int stride)
{
    uint8_t col[2];
    col[0] = (uint8_t)bundle_next(&d->b[B_COLOR]);
    col[1] = (uint8_t)bundle_next(&d->b[B_COLOR]);
    for (int r = 0; r < 8; ++r) {
        unsigned bits = (unsigned)bundle_next(&d->b[B_PATTERN]);
        for (int c = 0; c < 8; ++c, bits >>= 1) dst[r * stride + c] = col[bits & 1];
    }
}

static void blk_raw(vdec* d, uint8_t* dst, int stride)
{
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c) dst[r * stride + c] = (uint8_t)bundle_next(&d->b[B_COLOR]);
}

/* Runs of one colour, or of literal colours, along one of 16 space-filling scan patterns */
static int blk_run(vdec* d, uint8_t* dst, int stride)
{
    const uint8_t* scan = bink_patterns + (k2b_get(&d->br, 4) << 6);
    unsigned done = 0;
    while (done < 63) {
        unsigned run = (unsigned)bundle_next(&d->b[B_RUN]) + 1;
        if (done + run > 64) return 1;
        int same = k2b_get1(&d->br);
        uint8_t v = same ? (uint8_t)bundle_next(&d->b[B_COLOR]) : 0;
        for (unsigned i = 0; i < run; ++i, ++scan) {
            if (!same) v = (uint8_t)bundle_next(&d->b[B_COLOR]);
            dst[(*scan >> 3) * stride + (*scan & 7)] = v;
        }
        done += run;
    }
    if (done == 63) dst[(*scan >> 3) * stride + (*scan & 7)] = (uint8_t)bundle_next(&d->b[B_COLOR]);
    return 0;
}

static void blk_intra(vdec* d, uint8_t* dst, int stride)
{
    memset(d->coef, 0, sizeof d->coef);
    d->coef[0] = bundle_next(&d->b[B_INTRA_DC]);
    read_dct(d, d->coef, bink_quant);
    idct8x8(d->coef);
    put_block(d->coef, dst, stride);
}

/* A 16x16 block coded as an 8x8 one and pixel-doubled */
static int blk_scaled(vdec* d, uint8_t* dst, int stride)
{
    uint8_t* s = d->scaled;
    switch (bundle_next(&d->b[B_SUBTYPE])) {
    case BLK_FILL:    blk_fill(d, dst, stride, 16); return 0;
    case BLK_RAW:     blk_raw(d, s, 8); break;
    case BLK_INTRA:   blk_intra(d, s, 8); break;
    case BLK_RUN:     if (blk_run(d, s, 8)) return 1; break;
    case BLK_PATTERN: blk_pattern(d, s, 8); break;
    default:          return 1;
    }
    for (int r = 0; r < 16; ++r)
        for (int c = 0; c < 16; ++c) dst[r * stride + c] = s[(r >> 1) * 8 + (c >> 1)];
    return 0;
}

static int decode_plane(vdec* d, plane* p)
{
    plane_begin(d, p->bw);
    for (unsigned by = 0; by < p->bh; ++by) {
        /* sequenced: every refill reads the bitstream */
        if (fill_types(d, &d->b[B_TYPE]) || fill_types(d, &d->b[B_SUBTYPE]) ||
            fill_colors(d, &d->b[B_COLOR]) || fill_patterns(d, &d->b[B_PATTERN]) ||
            fill_motion(d, &d->b[B_XOFF]) || fill_motion(d, &d->b[B_YOFF]) ||
            fill_dc(d, &d->b[B_INTRA_DC], 0) || fill_dc(d, &d->b[B_INTER_DC], 1) ||
            fill_runs(d, &d->b[B_RUN]))
            return K2BINK_ERR_FORMAT;

        long row = (long)by * 8 * p->stride;
        for (unsigned bx = 0; bx < p->bw; ++bx) {
            long at = row + bx * 8;
            uint8_t* dst = p->cur + at;
            long ref;
            switch (bundle_next(&d->b[B_TYPE])) {
            case BLK_SKIP: break;
            case BLK_SCALED:
                /* covers this block and the next, on this row and the next; the odd row's
                 * matching entries carry no data */
                if (!(by & 1) && blk_scaled(d, dst, p->stride)) return K2BINK_ERR_FORMAT;
                ++bx;
                break;
            case BLK_MOTION:
                if (motion_ref(d, p, at, &ref)) return K2BINK_ERR_FORMAT;
                break;
            case BLK_RUN:
                if (blk_run(d, dst, p->stride)) return K2BINK_ERR_FORMAT;
                break;
            case BLK_RESIDUE:
                if (motion_ref(d, p, at, &ref)) return K2BINK_ERR_FORMAT;
                memset(d->coef, 0, sizeof d->coef);
                read_residue(d, d->coef);
                add_block(d->coef, dst, p->stride);
                break;
            case BLK_INTRA:
                blk_intra(d, dst, p->stride);
                break;
            case BLK_FILL:
                blk_fill(d, dst, p->stride, 8);
                break;
            case BLK_INTER:
                if (motion_ref(d, p, at, &ref)) return K2BINK_ERR_FORMAT;
                memset(d->coef, 0, sizeof d->coef);
                d->coef[0] = bundle_next(&d->b[B_INTER_DC]);
                read_dct(d, d->coef, bink_quant + 16 * 64);
                idct8x8(d->coef);
                add_block(d->coef, dst, p->stride);
                break;
            case BLK_PATTERN:
                blk_pattern(d, dst, p->stride);
                break;
            case BLK_RAW:
                blk_raw(d, dst, p->stride);
                break;
            default:
                return K2BINK_ERR_FORMAT;
            }
        }
    }
    k2b_align32(&d->br);
    return K2BINK_OK;
}

/* ---- lifecycle -------------------------------------------------------------------------------- */
int k2b_video_init(k2bink* b)
{
    huff_init();
    vdec* d = k2b_alloc(b, sizeof *d);
    if (!d) return K2BINK_ERR_NOMEM;
    b->vdec = d;
    const k2bink_info_t* in = &b->info;
    d->rev = in->revision;
    d->alpha = in->has_alpha;
    d->uv_swapped = in->revision >= 'h';

    /* Planes are padded so a block (or a 16x16 scaled block straddling the last column/row) never
     * leaves the buffer: one extra block of slack right and below. */
    size_t offs[4], total = 0;
    for (int i = 0; i < 4; ++i) {
        plane* p = &d->pl[i];
        int chroma = i == 1 || i == 2;
        if (i == 3 && !d->alpha) continue;
        unsigned unit = chroma ? 16 : 8;
        p->bw = (in->width + unit - 1) / unit;
        p->bh = (in->height + unit - 1) / unit;
        p->stride = (int)align(p->bw * 8 + 8, 16);
        p->rows = (int)(p->bh * 8 + 8);
        offs[i] = total;
        total += (size_t)p->stride * p->rows;
    }
    d->mem_size = total;
    d->mem[0] = k2b_alloc(b, total);
    d->mem[1] = k2b_alloc(b, total);
    if (!d->mem[0] || !d->mem[1]) return K2BINK_ERR_NOMEM;
    for (int i = 0; i < 4; ++i) {
        if (i == 3 && !d->alpha) continue;
        d->pl[i].cur = d->mem[0] + offs[i];
        d->pl[i].prev = d->mem[1] + offs[i];
    }

    /* bundle capacity: a whole luma plane's worth of values, plus one maximal chunk */
    size_t blocks = (size_t)d->pl[0].bw * d->pl[0].bh;
    static const unsigned per_block[B_COUNT] = { 1, 1, 64, 8, 1, 1, 1, 1, 64 };
    for (int i = 0; i < B_COUNT; ++i) {
        bundle* bd = &d->b[i];
        bd->wide = i >= B_XOFF && i <= B_INTER_DC;
        bd->cap = blocks * per_block[i] + 2 * ((size_t)d->pl[0].bw * per_block[i] + 512) + 64;
        if (bd->wide) bd->s16 = k2b_alloc(b, bd->cap * sizeof(int16_t));
        else bd->u8 = k2b_alloc(b, bd->cap);
        if (!bd->s16 && !bd->u8) return K2BINK_ERR_NOMEM;
    }
    return K2BINK_OK;
}

int k2b_video_decode(k2bink* b, const uint8_t* data, size_t size, k2bink_planes* out)
{
    vdec* d = b->vdec;
    /* each frame starts as a copy of the previous one; SKIP blocks leave it untouched */
    uint8_t* prev_base = d->mem[1];
    memcpy(d->mem[0], prev_base, d->mem_size);
    k2b_bits_init(&d->br, data, size);

    int r;
    if (d->alpha) {
        if (d->rev >= 'i') k2b_skip(&d->br, 32);
        if ((r = decode_plane(d, &d->pl[3]))) return r;
    }
    if (d->rev >= 'i') k2b_skip(&d->br, 32);
    static const int order[2][3] = { { 0, 1, 2 }, { 0, 2, 1 } };
    for (int i = 0; i < 3; ++i) {
        if ((r = decode_plane(d, &d->pl[order[d->uv_swapped][i]]))) return r;
        if (k2b_left(&d->br) < 1) break;     /* grayscale streams stop after Y */
    }

    /* the decoded frame becomes the reference: swap the buffer roles */
    uint8_t* t = d->mem[0]; d->mem[0] = d->mem[1]; d->mem[1] = t;
    for (int i = 0; i < 4; ++i) {
        if (i == 3 && !d->alpha) continue;
        uint8_t* decoded = d->pl[i].cur;
        d->pl[i].cur = d->pl[i].prev;
        d->pl[i].prev = decoded;
    }

    out->y = d->pl[0].prev; out->y_stride = d->pl[0].stride;
    out->u = d->pl[1].prev; out->v = d->pl[2].prev; out->uv_stride = d->pl[1].stride;
    out->a = d->alpha ? d->pl[3].prev : NULL; out->a_stride = d->alpha ? d->pl[3].stride : 0;
    out->width = b->info.width;
    out->height = b->info.height;
    return K2BINK_OK;
}

void k2b_video_free(k2bink* b)
{
    vdec* d = b->vdec;
    if (!d) return;
    k2b_free(b, d->mem[0]);
    k2b_free(b, d->mem[1]);
    for (int i = 0; i < B_COUNT; ++i) { k2b_free(b, d->b[i].u8); k2b_free(b, d->b[i].s16); }
    k2b_free(b, d);
    b->vdec = NULL;
}
