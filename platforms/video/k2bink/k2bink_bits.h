/*
 * k2bink -- LSB-first bit reader (Bink packs bits from the low end of each byte upward).
 * Reads past the end of the buffer return zero bits.
 * SPDX-License-Identifier: MIT OR Apache-2.0
 */
#ifndef K2BINK_BITS_H
#define K2BINK_BITS_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct k2b_bits {
    const uint8_t* buf;
    size_t size;        /* bytes */
    size_t pos;         /* bits */
} k2b_bits;

static inline void k2b_bits_init(k2b_bits* br, const uint8_t* buf, size_t size)
{
    br->buf = buf; br->size = size; br->pos = 0;
}

static inline uint64_t k2b__load64(const k2b_bits* br, size_t byte)
{
    uint64_t v = 0;
    if (byte + 8 <= br->size) {
        uint8_t t[8];
        memcpy(t, br->buf + byte, 8);
        for (int i = 7; i >= 0; --i) v = v << 8 | t[i];
    } else {
        for (int i = 7; i >= 0; --i) v = v << 8 | (byte + i < br->size ? br->buf[byte + i] : 0);
    }
    return v;
}

/* n in [0, 32] */
static inline uint32_t k2b_peek(const k2b_bits* br, unsigned n)
{
    if (!n) return 0;
    return (uint32_t)((k2b__load64(br, br->pos >> 3) >> (br->pos & 7)) & ((1ull << n) - 1));
}
static inline void     k2b_skip(k2b_bits* br, unsigned n) { br->pos += n; }
static inline uint32_t k2b_get(k2b_bits* br, unsigned n)
{
    uint32_t v = k2b_peek(br, n);
    br->pos += n;
    return v;
}
static inline unsigned k2b_get1(k2b_bits* br)
{
    size_t byte = br->pos >> 3;
    unsigned v = byte < br->size ? (br->buf[byte] >> (br->pos & 7)) & 1 : 0;
    ++br->pos;
    return v;
}
/* v, negated if the next bit is set */
static inline int32_t  k2b_signed(k2b_bits* br, int32_t v) { return k2b_get1(br) ? -v : v; }
static inline void     k2b_align32(k2b_bits* br) { br->pos = (br->pos + 31) & ~(size_t)31; }
static inline long     k2b_left(const k2b_bits* br) { return (long)(br->size * 8) - (long)br->pos; }

#endif
