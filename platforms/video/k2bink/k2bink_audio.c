/*
 * k2bink -- Bink audio decoding (DCT and RDFT variants).
 *
 * A C port of unbikit's src/bik-audio-decoder.ts and the audio transforms in src/transforms.ts
 * (https://github.com/blennies/unbikit). Copyright 2025 David Hart.
 * SPDX-License-Identifier: MIT OR Apache-2.0
 *
 * A packet holds one or more blocks. Each block, per channel: two raw 29-bit floats (the first two
 * coefficients), one 8-bit quantizer index per frequency band, then the remaining coefficients as
 * runs sharing a bit width. An inverse DCT, or inverse real DFT, gives frame_len samples, whose last
 * 1/16 overlap-crossfades into the next block.
 */
#include "k2bink_internal.h"
#include "k2bink_bits.h"
#include "k2bink_tables.h"

#include <math.h>
#include <string.h>

#define MAX_BANDS 26
#define MAX_CH    2

typedef struct adec {
    int      dct;
    unsigned out_ch;          /* channels the caller sees */
    unsigned nch;             /* channels coded separately: out_ch for DCT, 1 for interleaved RDFT */
    unsigned bits, len;       /* frame length = 1 << bits */
    unsigned overlap;         /* len / 16 */
    unsigned n_bands;
    unsigned bands[MAX_BANDS + 1];
    float    quant[96];
    float    base;
    int      first;
    float*   coef[MAX_CH];    /* len each */
    float*   tail[MAX_CH];    /* previous block's last `overlap` samples */
    /* DCT: reciprocal cosines for every sub-size 2^0..2^bits, concatenated. The DCT runs in double
     * because Lee's factorization multiplies by 0.5/cos(~pi/2) -- about 650 at 2048 -- near the middle
     * output, which magnifies float rounding past 1 LSB there. */
    double*  rcos;
    double*  dx; double* dtmp; /* len each */
    /* RDFT: quarter-length twiddles and the half-size complex FFT's tables */
    float*   cos_q; float* sin_q;
    float*   tw;              /* interleaved cos/sin, fft_n entries each */
    uint16_t* rev;
    unsigned fft_bits;
} adec;

/* ---- transforms ------------------------------------------------------------------------------- */
/* DCT-III by Lee's recursive factorization. `data` and `tmp` swap roles at each level. */
static void idct_rec(const adec* a, double* data, unsigned off, unsigned n, unsigned nbits, double* tmp)
{
    if (n < 2) return;
    const double* rc = a->rcos + ((1u << nbits) - 1);   /* table for size 2^nbits */
    unsigned half = n >> 1;
    tmp[off] = data[off];
    tmp[off + half] = data[off + 1];
    for (unsigned i = 1; i < half; ++i) {
        tmp[off + i] = data[off + 2 * i];
        tmp[off + i + half] = data[off + 2 * i - 1] + data[off + 2 * i + 1];
    }
    idct_rec(a, tmp, off, half, nbits - 1, data);
    idct_rec(a, tmp, off + half, half, nbits - 1, data);
    for (unsigned i = 0; i < half; ++i) {
        double x = tmp[off + i], y = tmp[off + i + half] * rc[i];
        data[off + i] = x + y;
        data[off + n - 1 - i] = x - y;
    }
}

/* radix-2 decimation-in-time complex FFT, in place, interleaved re/im */
static void fft(const adec* a, float* d)
{
    unsigned n = 1u << a->fft_bits;
    for (unsigned i = 0; i < n; ++i) {
        unsigned j = a->rev[i];
        if (j > i) {
            float tr = d[2 * i], ti = d[2 * i + 1];
            d[2 * i] = d[2 * j]; d[2 * i + 1] = d[2 * j + 1];
            d[2 * j] = tr; d[2 * j + 1] = ti;
        }
    }
    for (unsigned size = 2; size <= n; size <<= 1) {
        unsigned half = size >> 1, step = n / size;
        for (unsigned i = 0; i < n; i += size)
            for (unsigned j = 0, t = 0; j < half; ++j, t += step) {
                float wr = a->tw[2 * t], wi = a->tw[2 * t + 1];
                float* e = d + 2 * (i + j);
                float* o = d + 2 * (i + j + half);
                float tr = wr * o[0] - wi * o[1], ti = wr * o[1] + wi * o[0];
                o[0] = e[0] - tr; o[1] = e[1] - ti;
                e[0] += tr; e[1] += ti;
            }
    }
}

/* inverse real DFT via a half-size complex FFT. In: [Re0, ReN/2, Re1, Im1, ...] */
static void irdft(const adec* a, float* d)
{
    unsigned n = a->len, q = n >> 2;
    float dc = d[0], ny = d[1];
    d[0] = 0.5f * (dc + ny);
    d[1] = 0.5f * (dc - ny);
    for (unsigned i = 1; i < q; ++i) {
        unsigned i1 = 2 * i, i2 = n - i1;
        float er = 0.5f * (d[i1] + d[i2]), oi = 0.5f * (d[i1] - d[i2]);
        float ei = 0.5f * (d[i1 + 1] - d[i2 + 1]), or_ = -0.5f * (d[i1 + 1] + d[i2 + 1]);
        float c = a->cos_q[i], s = a->sin_q[i];
        d[i1] = er + or_ * c - oi * s;
        d[i1 + 1] = ei + oi * c + or_ * s;
        d[i2] = er - or_ * c + oi * s;
        d[i2 + 1] = -ei + oi * c + or_ * s;
    }
    fft(a, d);
}

/* ---- bitstream -------------------------------------------------------------------------------- */
/* 5-bit exponent, 23-bit mantissa, sign */
static float read_float29(k2b_bits* br)
{
    int e = (int)k2b_get(br, 5);
    double v = ldexp((double)k2b_get(br, 23), e - 23);
    return (float)(k2b_get1(br) ? -v : v);
}

static void read_channel(adec* a, k2b_bits* br, float* c)
{
    c[0] = read_float29(br) * a->base;
    c[1] = read_float29(br) * a->base;
    float q[MAX_BANDS];
    for (unsigned b = 0; b < a->n_bands; ++b) {
        unsigned v = k2b_get(br, 8);
        q[b] = a->quant[v < 95 ? v : 95];
    }
    /* coefficients from 2 up come in runs (8, or a coded length) sharing one magnitude width;
     * a coefficient's quantizer is that of the band containing it */
    unsigned band = 0;
    for (unsigned i = 2; i < a->len;) {
        unsigned end = i + (k2b_get1(br) ? bink_audio_rle_lengths[k2b_get(br, 4)] : 8u);
        if (end > a->len) end = a->len;
        unsigned width = k2b_get(br, 4);
        if (!width) {
            memset(c + i, 0, (end - i) * sizeof(float));
            i = end;
            continue;
        }
        for (; i < end; ++i) {
            while (a->bands[band + 1] <= i) ++band;
            unsigned m = k2b_get(br, width);
            c[i] = m ? (k2b_get1(br) ? -q[band] : q[band]) * (float)m : 0.0f;
        }
    }
}

/* ---- lifecycle -------------------------------------------------------------------------------- */
int k2b_audio_init(k2bink* b, k2bink_track* t)
{
    adec* a = k2b_alloc(b, sizeof *a);
    if (!a) return K2BINK_ERR_NOMEM;
    t->dec = a;
    unsigned rate = t->info.sample_rate;
    a->dct = t->info.uses_dct;
    a->out_ch = t->info.channels;
    a->bits = rate < 22050 ? 9 : rate < 44100 ? 10 : 11;
    a->nch = a->out_ch;
    if (!a->dct) {
        /* RDFT audio interleaves channels into one longer frame */
        unsigned c = a->out_ch, lg = 0;
        while ((1u << lg) < c) ++lg;          /* ceil(log2) */
        rate *= c;
        a->bits += lg & 3;
        a->nch = 1;
    }
    a->len = 1u << a->bits;
    a->overlap = a->len >> 4;
    unsigned half_rate = (rate + 1) / 2;
    a->base = (float)((a->dct ? (double)a->len : 2.0) / (sqrt((double)a->len) * 32768.0));
    for (int i = 0; i < 96; ++i)
        a->quant[i] = (float)(exp(i * (0.0664 / log10(M_E))) * a->base);

    /* bands: edges at the critical frequencies below Nyquist, even-aligned */
    unsigned nb = 0;
    while (nb < 24 && bink_audio_band_freqs[nb] < half_rate) ++nb;
    a->n_bands = nb + 1;
    a->bands[0] = 2;
    for (unsigned i = 1; i < a->n_bands; ++i)
        a->bands[i] = (unsigned)((uint64_t)bink_audio_band_freqs[i - 1] * a->len / half_rate) & ~1u;
    a->bands[a->n_bands] = a->len;

    for (unsigned c = 0; c < a->nch; ++c) {
        a->coef[c] = k2b_alloc(b, a->len * sizeof(float));
        a->tail[c] = k2b_alloc(b, a->overlap * sizeof(float));
        if (!a->coef[c] || !a->tail[c]) return K2BINK_ERR_NOMEM;
    }

    if (a->dct) {
        a->rcos = k2b_alloc(b, ((size_t)2 << a->bits) * sizeof(double));
        a->dx = k2b_alloc(b, a->len * sizeof(double));
        a->dtmp = k2b_alloc(b, a->len * sizeof(double));
        if (!a->rcos || !a->dx || !a->dtmp) return K2BINK_ERR_NOMEM;
        for (unsigned lvl = 0; lvl <= a->bits; ++lvl) {
            unsigned n = 1u << lvl;
            for (unsigned i = 0; i < n; ++i)
                a->rcos[n - 1 + i] = 0.5 / cos((i + 0.5) * M_PI / n);
        }
    } else {
        unsigned q = a->len >> 2, fn;
        double theta = 2 * M_PI / a->len;
        a->cos_q = k2b_alloc(b, q * sizeof(float));
        a->sin_q = k2b_alloc(b, q * sizeof(float));
        a->fft_bits = a->bits - 1;
        fn = 1u << a->fft_bits;
        a->tw = k2b_alloc(b, fn * sizeof(float));
        a->rev = k2b_alloc(b, fn * sizeof(uint16_t));
        if (!a->cos_q || !a->sin_q || !a->tw || !a->rev) return K2BINK_ERR_NOMEM;
        for (unsigned i = 0; i < q; ++i) { a->cos_q[i] = (float)cos(i * theta); a->sin_q[i] = (float)sin(i * theta); }
        for (unsigned i = 0; i < fn; ++i) {
            unsigned r = 0;
            for (unsigned k = 0, v = i; k < a->fft_bits; ++k, v >>= 1) r = r << 1 | (v & 1);
            a->rev[i] = (uint16_t)r;
        }
        for (unsigned i = 0; i < fn / 2; ++i) {
            double ang = -2 * M_PI * i / fn;
            a->tw[2 * i] = (float)cos(ang);
            a->tw[2 * i + 1] = (float)sin(ang);
        }
    }
    a->first = 1;
    return K2BINK_OK;
}

int k2b_audio_decode(k2bink* b, k2bink_track* t, const uint8_t* data, size_t size)
{
    adec* a = t->dec;
    k2b_bits br;
    k2b_bits_init(&br, data, size);
    unsigned keep = a->len - a->overlap;      /* new samples per block, per coded channel */

    while (k2b_left(&br) > 0) {
        if (a->dct) k2b_skip(&br, 2);
        for (unsigned c = 0; c < a->nch; ++c) {
            float* x = a->coef[c];
            read_channel(a, &br, x);
            if (a->dct) {
                for (unsigned i = 0; i < a->len; ++i) a->dx[i] = x[i];
                idct_rec(a, a->dx, 0, a->len, a->bits, a->dtmp);
                /* The inverse DCT's normalization is 2/N, which matches the reference at 32, 44.1 and
                 * 48 kHz alike. The 4*base unbikit uses instead is 4*sqrt(2) too loud at N=2048 and
                 * 2*sqrt(2) too quiet relative to that at 1024, which is what KOTOR's 32 kHz movies
                 * use. */
                double g = 2.0 / a->len;
                for (unsigned i = 0; i < a->len; ++i) x[i] = (float)(a->dx[i] * g);
            } else {
                irdft(a, x);
            }
        }
        /* crossfade the head of this block with the previous block's tail; with interleaved
         * channels the ramp advances per interleaved sample */
        for (unsigned c = 0; c < a->nch; ++c) {
            float* x = a->coef[c];
            if (!a->first) {
                float span = (float)(a->overlap * a->nch);
                for (unsigned i = 0; i < a->overlap; ++i) {
                    float w = (float)(i * a->nch + c) / span;
                    x[i] = a->tail[c][i] + (x[i] - a->tail[c][i]) * w;
                }
            }
            memcpy(a->tail[c], x + keep, a->overlap * sizeof(float));
        }
        a->first = 0;

        /* append `keep` sample frames (DCT) or `keep` interleaved samples (RDFT) */
        size_t n = (size_t)keep * a->nch;
        float* out = k2b_audio_reserve(b, t, n);
        if (!out) return K2BINK_ERR_NOMEM;
        if (a->nch == 1) memcpy(out, a->coef[0], n * sizeof(float));
        else
            for (unsigned i = 0; i < keep; ++i)
                for (unsigned c = 0; c < a->nch; ++c) *out++ = a->coef[c][i];
        k2b_align32(&br);
    }
    return K2BINK_OK;
}

void k2b_audio_reset(k2bink* b, k2bink_track* t)
{
    (void)b;
    if (t->dec) ((adec*)t->dec)->first = 1;
}

void k2b_audio_free(k2bink* b, k2bink_track* t)
{
    adec* a = t->dec;
    if (!a) return;
    for (unsigned c = 0; c < MAX_CH; ++c) { k2b_free(b, a->coef[c]); k2b_free(b, a->tail[c]); }
    k2b_free(b, a->rcos); k2b_free(b, a->dx); k2b_free(b, a->dtmp);
    k2b_free(b, a->cos_q); k2b_free(b, a->sin_q); k2b_free(b, a->tw); k2b_free(b, a->rev);
    k2b_free(b, a);
    t->dec = NULL;
}
