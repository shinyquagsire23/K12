#include "k2wavsniff.h"

#include <string.h>

// MPEG frame-header fields, or 0 for "this is not a frame header".
//
// A lone sync word is not evidence: 16-bit PCM silence is full of 0xff bytes, and `ff ff 02 00` (two
// samples of -1 and 2) satisfies every single-header test there is -- which is exactly what the
// 470-byte prefix of streamsounds like `p_t3m3_bat6.wav` is made of. So k2wav_is_mpeg below walks a
// chain of frames instead: a header is only believed when the header its own length points at is
// also a header, of the same version, layer and sample rate, three times over.
static const unsigned short k2wav_bitrate[2][3][16] = {
    {   // MPEG 1: layer III, II, I
        {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0},
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0},
        {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0}
    },
    {   // MPEG 2 / 2.5: layer III, II, I
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0}
    }
};
static const unsigned short k2wav_samplerate[4][3] = {
    {11025, 12000,  8000},   // MPEG 2.5
    {    0,     0,     0},   // reserved
    {22050, 24000, 16000},   // MPEG 2
    {44100, 48000, 32000}    // MPEG 1
};

// Returns the frame length in bytes, and fills *id with the version|layer|rate triple that every
// frame of one stream shares. 0 means "not a frame header".
static size_t k2wav_frame_len(const unsigned char* p, size_t n, unsigned* id)
{
    unsigned version, layer, bitrateIdx, rateIdx, padding, bitrate, rate;

    if (n < 4)                 return 0;
    if (p[0] != 0xff)          return 0;
    if ((p[1] & 0xe0) != 0xe0) return 0;

    version    = (p[1] >> 3) & 0x03;      // 0 = 2.5, 1 = reserved, 2 = 2, 3 = 1
    layer      = (p[1] >> 1) & 0x03;      // 0 = reserved, 1 = III, 2 = II, 3 = I
    bitrateIdx = (p[2] >> 4) & 0x0f;
    rateIdx    = (p[2] >> 2) & 0x03;
    padding    = (p[2] >> 1) & 0x01;
    if (version == 1 || layer == 0 || rateIdx == 3) return 0;
    // Index 0 is "free format" (the encoder sets its own rate out of band) and 15 is reserved.
    // Neither appears in any shipped asset, and both would leave the frame length unknown.
    if (bitrateIdx == 0 || bitrateIdx == 15)        return 0;

    bitrate = k2wav_bitrate[version == 3 ? 0 : 1][layer - 1][bitrateIdx] * 1000u;
    rate    = k2wav_samplerate[version][rateIdx];
    if (bitrate == 0 || rate == 0) return 0;

    if (id) *id = (version << 4) | (layer << 2) | rateIdx;
    if (layer == 3)                             // Layer I: 384 samples, 4-byte slots
        return (size_t)((12u * bitrate / rate + padding) * 4u);
    if (version == 3 || layer == 2)             // MPEG 1, or Layer II at any version: 1152 samples
        return (size_t)(144u * bitrate / rate + padding);
    return (size_t)(72u * bitrate / rate + padding);   // MPEG 2/2.5 Layer III: 576 samples
}

static int k2wav_is_mpeg(const unsigned char* p, size_t n)
{
    unsigned id0 = 0;
    size_t   off = 0;
    int      i;

    for (i = 0; i < 3; i++)
    {
        unsigned id  = 0;
        size_t   len = k2wav_frame_len(p + off, n - off, &id);
        if (len == 0 || (i > 0 && id != id0))
            return 0;
        if (i == 0)
            id0 = id;
        off += len;
        if (off + 4 > n)
            return i > 0;    // a stream too short for three frames still had the ones it had
    }
    return 1;
}

static int k2wav_is_id3(const unsigned char* p, size_t n)
{
    return n >= 3 && p[0] == 'I' && p[1] == 'D' && p[2] == '3';
}

static unsigned k2wav_le32(const unsigned char* p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
}

void k2wav_sniff(const void* data, size_t size, k2wav_payload* out)
{
    const unsigned char* p = (const unsigned char*)data;
    size_t base = 0;
    size_t off;

    out->kind   = K2WAV_UNKNOWN;
    out->offset = 0;
    out->length = size;
    if (p == 0 || size < 4)
        return;

    // With no RIFF at the start the engine still probes for one behind the fake header's prefix, so
    // a buffer that carries that prefix can reach here.
    if (memcmp(p, "RIFF", 4) != 0 && size > 0x1d6 + 12 && memcmp(p + 0x1d6, "RIFF", 4) == 0)
        base = 0x1d6;

    if (base + 12 <= size && memcmp(p + base, "RIFF", 4) == 0 && memcmp(p + base + 8, "WAVE", 4) == 0)
    {
        for (off = base + 12; off + 8 <= size;)
        {
            unsigned chunk = k2wav_le32(p + off + 4);
            if (memcmp(p + off, "data", 4) == 0)
            {
                size_t payload = off + 8;
                size_t avail   = size - payload;
                // The fake header's `data` size is 0 and the real one can overrun the buffer (the
                // engine hands us a BIF slice, not the whole file), so clamp both ways.
                size_t len = (chunk == 0 || (size_t)chunk > avail) ? avail : (size_t)chunk;
                if (len != 0 && (k2wav_is_mpeg(p + payload, len) || k2wav_is_id3(p + payload, len)))
                {
                    out->kind   = K2WAV_MP3;
                    out->offset = payload;
                    out->length = len;
                    return;
                }
                out->kind   = K2WAV_RIFF;       // honest container: the decoder wants all of it
                out->offset = base;
                out->length = size - base;
                return;
            }
            off += 8 + chunk + (chunk & 1);
            if (chunk > size)                   // a corrupt size would wrap the cursor
                break;
        }
        out->kind   = K2WAV_RIFF;               // RIFF/WAVE with no data chunk in reach
        out->offset = base;
        out->length = size - base;
        return;
    }

    if (k2wav_is_mpeg(p, size) || k2wav_is_id3(p, size))
    {
        out->kind   = K2WAV_MP3;
        out->offset = 0;
        out->length = size;
    }
}
