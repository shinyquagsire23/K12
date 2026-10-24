// The single place the vendored miniaudio's build switches are chosen.
//
// Several MA_NO_* switches change miniaudio's struct layouts (MA_NO_RESOURCE_MANAGER removes a
// pointer from ma_engine_config, for one), so the implementation TU and every consumer must agree
// about them. Including this header instead of <miniaudio.h> is what guarantees that: k2ma.c is
// `#define MINIAUDIO_IMPLEMENTATION` + this file.
#ifndef K2MA_H
#define K2MA_H

// We never use the resource manager: platforms/audio/fmod_miniaudio.cpp owns the encoded bytes
// itself (the engine's "file name" is a stream-id handle, not a path) and drives one ma_decoder per
// voice.
#define MA_NO_RESOURCE_MANAGER
// No shipped KOTOR II asset is FLAC: 2,265 in-BIF `wav` resources are RIFF/PCM, fake-RIFF/MP3 or
// raw MP3, and streammusic/streamsounds are the same two MP3 shapes.
#define MA_NO_FLAC

#include "miniaudio.h"

#endif // K2MA_H
