// What is actually inside a KOTOR II "wav"?
//
// The container lies. Of the 2,265 `wav` resources in the shipped BIFs, 2,128 are honest
// RIFF/PCM, 127 are a RIFF/WAVE wrapper whose `data` chunk holds MP3 frames, and 10 are bare MP3
// with no container at all. `streammusic/*.wav` is the second shape with a 58-byte fake header
// whose `fmt ` chunk claims 8-bit 22,050 Hz PCM and whose `data` chunk claims a length of zero;
// `streamsounds/*.wav` is the third. A decoder that trusts the RIFF magic decodes silence (or
// noise) for 137 BIF resources and for every streamed line of dialogue in the game.
//
// So: sniff the payload, not the container. k2wav_sniff() reports the byte range to hand a decoder
// and which decoder to hand it to.
#ifndef K2WAVSNIFF_H
#define K2WAVSNIFF_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    K2WAV_UNKNOWN = 0,   // nothing recognised; let the decoder sniff for itself
    K2WAV_RIFF    = 1,   // a genuine RIFF/WAVE: hand the whole container to the wav decoder
    K2WAV_MP3     = 2    // MPEG audio frames (bare, or behind a fake RIFF header)
} k2wav_kind;

typedef struct
{
    k2wav_kind kind;
    size_t     offset;   // first byte to give the decoder
    size_t     length;   // how many bytes from `offset`
} k2wav_payload;

// Never fails: an unrecognised buffer comes back as K2WAV_UNKNOWN over the whole range.
void k2wav_sniff(const void* data, size_t size, k2wav_payload* out);

#ifdef __cplusplus
}
#endif

#endif // K2WAVSNIFF_H
