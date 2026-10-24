// A silent FMOD. The engine's FMOD backend (src/libsource/exosound/FModAudioSystem) talks to the
// FMOD 5 low-level C++ API, which the Android build links as libfmod.so. This file implements
// exactly the 29 entry points the engine calls, against the engine's own model of the API
// (FModAPI.h), so a signature drift in the model shows up here as a compile error.
//
// Everything succeeds and nothing is heard, but channel lifetimes follow FMOD: a channel plays for
// its sound's duration (from the WAV header for in-memory sounds, estimated from the byte count for
// streams), isPlaying reports it until then, and System::update() delivers the END callback, as
// real FMOD does from update(), also after stop(). A looping sound never ends on its own. The
// engine's channel bookkeeping (FModAudioSystemChannelInfo::m_bDone) is set only by that callback,
// so without it every voice-over line counts as still playing and a conversation waiting on one
// never advances.
#include "FModAPI.h"
#include <stdint.h>
#include <string.h>
#include <time.h>

namespace {
struct NullSound   { void* user; bool loop; double seconds; };
struct NullChannel {
    void* user; bool paused; unsigned int pos;
    bool active; bool loop; double remaining;
    FMOD::FMOD_CHANNELCONTROL_CALLBACK callback;
};
FMOD::System g_system;
NullChannel g_channels[256];
int g_nextChannel;
double g_lastUpdate = -1.0;

double Now()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

uint32_t Le32(const unsigned char* p) { return p[0] | p[1] << 8 | p[2] << 16 | static_cast<uint32_t>(p[3]) << 24; }

// Duration of an in-memory RIFF/WAVE image: the data chunk over the fmt chunk's byte rate. The
// RIFF tag may sit behind a junk prefix (the game's MP3-in-WAV voice files), so look for it.
double WavSeconds(const unsigned char* p, unsigned int n)
{
    unsigned int base = 0;
    while (base + 12 <= n && base < 1024 && memcmp(p + base, "RIFF", 4) != 0) base++;
    if (base + 12 > n || memcmp(p + base + 8, "WAVE", 4) != 0) return -1.0;
    uint32_t byteRate = 0;
    for (unsigned int off = base + 12; off + 8 <= n;) {
        uint32_t size = Le32(p + off + 4);
        if (memcmp(p + off, "fmt ", 4) == 0 && off + 20 <= n) byteRate = Le32(p + off + 16);
        if (memcmp(p + off, "data", 4) == 0)
            return byteRate ? static_cast<double>(size < n - off - 8 ? size : n - off - 8) / byteRate : -1.0;
        off += 8 + size + (size & 1);
    }
    return -1.0;
}

void Finish(NullChannel* c)
{
    c->active = false;
    if (c->callback)
        c->callback(reinterpret_cast<FMOD_CHANNELCONTROL*>(c), FMOD_CHANNELCONTROL_CHANNEL,
                    FMOD_CHANNELCONTROL_CALLBACK_END, 0, 0);
}
}

extern "C" void k2audio_set_suspended(int) {}

extern "C" FMOD_RESULT FMOD_System_Create(FMOD::System** system)
{
    *system = &g_system;
    return FMOD_OK;
}

namespace FMOD {
FMOD_RESULT System::release() { return FMOD_OK; }
FMOD_RESULT System::update()
{
    double now = Now(), dt = g_lastUpdate < 0 ? 0 : now - g_lastUpdate;
    g_lastUpdate = now;
    for (NullChannel& c : g_channels) {
        if (!c.active) continue;
        if (!c.paused) c.remaining -= dt;
        if (!c.loop && c.remaining <= 0) Finish(&c);
    }
    return FMOD_OK;
}
FMOD_RESULT System::init(int, unsigned int, void*) { return FMOD_OK; }
FMOD_RESULT System::setOutput(FMOD_OUTPUTTYPE) { return FMOD_OK; }
FMOD_RESULT System::getNumDrivers(int* n) { *n = 1; return FMOD_OK; }
FMOD_RESULT System::setFileSystem(FMOD_FILE_OPENCALLBACK, FMOD_FILE_CLOSECALLBACK, FMOD_FILE_READCALLBACK,
                                  FMOD_FILE_SEEKCALLBACK, FMOD_FILE_ASYNCREADCALLBACK,
                                  FMOD_FILE_ASYNCCANCELCALLBACK, int) { return FMOD_OK; }
FMOD_RESULT System::setStreamBufferSize(unsigned int, unsigned int) { return FMOD_OK; }
FMOD_RESULT System::set3DListenerAttributes(int, const FMOD_VECTOR*, const FMOD_VECTOR*,
                                            const FMOD_VECTOR*, const FMOD_VECTOR*) { return FMOD_OK; }
FMOD_RESULT System::getChannel(int id, Channel** ch)
{
    *ch = reinterpret_cast<Channel*>(&g_channels[id & 255]);
    return FMOD_OK;
}
FMOD_RESULT System::playSound(Sound* sound, ChannelGroup*, bool paused, Channel** ch)
{
    NullChannel* c = &g_channels[g_nextChannel++ & 255];
    const NullSound* s = reinterpret_cast<const NullSound*>(sound);
    *c = NullChannel();
    c->paused = paused;
    c->active = true;
    c->loop = s && s->loop;
    c->remaining = s ? s->seconds : 0.0;
    if (ch) *ch = reinterpret_cast<Channel*>(c);
    return FMOD_OK;
}
FMOD_RESULT System::createSound(const char* name, unsigned int mode, FMOD_CREATESOUNDEXINFO* exinfo, Sound** s)
{
    NullSound* sound = new NullSound();
    sound->loop = (mode & FMOD_LOOP_NORMAL) != 0;
    unsigned int n = exinfo ? exinfo->length : 0;
    double seconds = -1.0;
    if ((mode & FMOD_OPENMEMORY) && name) seconds = WavSeconds(reinterpret_cast<const unsigned char*>(name), n);
    if (seconds < 0)   // a stream (only its byte count is known): 128 kbit/s MP3, else 22 kHz 16-bit mono
        seconds = n / (exinfo && exinfo->suggestedsoundtype == FMOD_SOUND_TYPE_MPEG ? 16000.0 : 44100.0);
    sound->seconds = seconds;
    *s = reinterpret_cast<Sound*>(sound);
    return FMOD_OK;
}

FMOD_RESULT Sound::setUserData(void* u) { reinterpret_cast<NullSound*>(this)->user = u; return FMOD_OK; }
FMOD_RESULT Sound::release() { delete reinterpret_cast<NullSound*>(this); return FMOD_OK; }
// Units: 1 milliseconds, 2 PCM samples (taken as 22050 Hz), 4 PCM bytes (16-bit mono). Never 0 ms:
// GetSoundSampleRate divides by it.
FMOD_RESULT Sound::getLength(unsigned int* len, unsigned int unit)
{
    double seconds = reinterpret_cast<NullSound*>(this)->seconds;
    double rate = unit == 1 ? 1000.0 : unit == 2 ? 22050.0 : unit == 4 ? 44100.0 : 1000.0;
    *len = static_cast<unsigned int>(seconds * rate);
    if (*len == 0) *len = 1;
    return FMOD_OK;
}

// FMOD reports a stopped channel's end callback from the next update(), not from inside stop().
FMOD_RESULT ChannelControl::stop()
{
    NullChannel* c = reinterpret_cast<NullChannel*>(this);
    c->loop = false; c->remaining = 0;
    return FMOD_OK;
}
FMOD_RESULT ChannelControl::setPaused(bool p) { reinterpret_cast<NullChannel*>(this)->paused = p; return FMOD_OK; }
FMOD_RESULT ChannelControl::isPlaying(bool* p) { *p = reinterpret_cast<NullChannel*>(this)->active; return FMOD_OK; }
FMOD_RESULT ChannelControl::setVolume(float) { return FMOD_OK; }
FMOD_RESULT ChannelControl::setPan(float) { return FMOD_OK; }
FMOD_RESULT ChannelControl::set3DAttributes(const FMOD_VECTOR*, const FMOD_VECTOR*) { return FMOD_OK; }
FMOD_RESULT ChannelControl::set3DMinMaxDistance(float, float) { return FMOD_OK; }
FMOD_RESULT ChannelControl::set3DOcclusion(float, float) { return FMOD_OK; }
FMOD_RESULT ChannelControl::setUserData(void* u) { reinterpret_cast<NullChannel*>(this)->user = u; return FMOD_OK; }
FMOD_RESULT ChannelControl::getUserData(void** u) { *u = reinterpret_cast<NullChannel*>(this)->user; return FMOD_OK; }
FMOD_RESULT ChannelControl::setCallback(FMOD_CHANNELCONTROL_CALLBACK cb) { reinterpret_cast<NullChannel*>(this)->callback = cb; return FMOD_OK; }

FMOD_RESULT Channel::setFrequency(float) { return FMOD_OK; }
FMOD_RESULT Channel::setPriority(int) { return FMOD_OK; }
FMOD_RESULT Channel::setPosition(unsigned int p, unsigned int) { reinterpret_cast<NullChannel*>(this)->pos = p; return FMOD_OK; }
FMOD_RESULT Channel::getPosition(unsigned int* p, unsigned int) { *p = reinterpret_cast<NullChannel*>(this)->pos; return FMOD_OK; }
} // namespace FMOD
