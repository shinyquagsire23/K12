// An audible FMOD on miniaudio. It has the same 29 entry points as the silent backend, compiled
// against the engine's own model of the FMOD 5 low-level API (src/port/fmod/FModAPI.h), so a drift
// in that model is a compile error here. Selected with -DK2P_AUDIO=miniaudio.
//
// What the engine needs from an FMOD backend:
//
//  * `setFileSystem` hands over the engine's open/close/read/seek callbacks, and the "file name" a
//    stream is created with is a decimal stream id rather than a path: SystemOpenCallback strtouls
//    it back and answers with the FILE* the engine's own stream table holds. Those callbacks are
//    the only route to a stream's bytes.
//  * The engine's seek callback is a no-op: it expects FMOD to hand back the StreamInfo as the
//    per-file userdata, FMOD passes 0, and the guard swallows the fseek (see FModAudioSystem::
//    SystemSeekCallback). A forward-only reader is all it can service, so each stream is read
//    whole, once, at createSound time rather than through a ma_vfs that would need to seek. The
//    file is already positioned at the stream's offset when CreateStream is called. The largest
//    shipped stream is 14.5 MB (streamsounds/mus_main.wav) and only a handful are open at a time.
//  * A one-shot arrives as FMOD_OPENMEMORY: the "name" argument is the audio image and
//    exinfo->length its size.
//  * The end callback drives the whole engine. FModAudioSystemChannelInfo::m_bDone is set nowhere
//    else, so a conversation waiting on a VO line never advances without it. A sound that plays
//    out reports its end from System::update(), on the game thread; miniaudio's own end callback
//    runs on the audio thread and only sets a flag. A stopped channel reports nothing: the
//    engine's only stop is StopChannel, which resets the channel record immediately and lets
//    PlaySoundA hand it to the next sound in the same frame, so a late end would be taken for the
//    new sound's.
//  * isPlaying's result code matters where its bool does not: GetIsChannelPlaying treats
//    FMOD_ERR_CHANNEL_STOLEN and FMOD_ERR_INVALID_HANDLE as "not playing" and ignores the bool.
//  * A Channel* is a handle, not a pointer: FMOD encodes the voice and a generation, and a call
//    through a handle whose channel has since stopped, played out or been reused fails with
//    FMOD_ERR_INVALID_HANDLE. The engine relies on that: its channel records keep the handle of a
//    finished sound and go on pausing, resuming and testing it (a GUI click's record, then
//    StopChannel / IsPlaying / ResumeAllSounds). A bare voice pointer would reach whatever that
//    voice plays next -- RebootStream parks a finished stream paused on a free voice, and the
//    button's stale handle resumed it, so every button played the player's last grunt.
//  * The engine does its own voice management (GetFreeChannel over m_nNumChannels channel
//    records, PlaySoundA returns -1 when they are all busy), so a pool the size of System::init()'s
//    maxchannels never has to steal. The stealing path below is a backstop.
//
// Deliberate gaps:
//  * set3DOcclusion applies the direct term as gain only; the one-pole lowpass that should go with
//    it is left to the Steam Audio stage, because driving a ma_lpf_node's cutoff from the game
//    thread every frame while the audio thread reads it is not a safe trade for a tone change.
//  * The reverb term is accepted and ignored. There is no reverb bus yet.
//
// 3D panning is ours, not miniaudio's. miniaudio's spatializer scales each speaker by
// (dot(dir, speaker) + 1) / 2, so a sound dead ahead plays at 0.5 in both channels -- 6 dB under
// the same sound played 2D, which is how every 2D sound (music, UI, ambience) reaches the mix.
// FMOD has no such dip, and the engine's levels assume it doesn't: every 3D voice (soundset barks,
// conversation VO, creature chatter) came out audibly quiet. So a spatial voice keeps miniaudio's
// distance rolloff with its directional term switched off, and update() pans it from its
// listener-space azimuth with unity gain at the centre. The engine's own setPan is ignored on a 3D
// voice, as FMOD's default 3D level of 1 ignores 2D panning.
//
// Environment knobs:
//   K2_AUDIO_TRACE=1        log every sound/stream/voice event to stderr
//   K2_AUDIO_WAV=<path>     render to that WAV instead of a device, pumped from System::update(),
//                           so an unattended run can be listened to, or measured, afterwards
#include <stdint.h>
#include "FModAPI.h"
#include "k2ma.h"
#include "k2wavsniff.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

namespace {

// ------------------------------------------------------------------------------------------
// state
// ------------------------------------------------------------------------------------------

// What FMOD::Sound* points at: the encoded bytes, held for as long as the engine keeps the sound,
// plus the sniffed payload window and whatever has been probed out of it.
struct SoundImpl
{
    unsigned char*     data;
    size_t             size;
    size_t             payloadOff;
    size_t             payloadLen;
    ma_encoding_format encoding;
    bool               loop;
    bool               spatial;
    bool               stream;
    bool               alive;
    void*              user;

    bool               probed;        // the three below are filled on first getLength/setFrequency
    ma_uint32          sampleRate;
    ma_uint32          channels;
    ma_uint64          frames;
};

// What FMOD::Channel* points at. The pool is fixed and the pointers are stable, because
// InitChannels caches one per channel record at startup (System::getChannel) and keeps using it
// until the first playSound replaces it -- PauseAllActiveChannels calls setPaused on channels
// that have never played a thing.
struct Voice
{
    ma_sound    sound;
    ma_decoder  decoder;
    bool        soundInit;
    bool        decoderInit;
    bool        inUse;
    bool        paused;
    bool        rewound;         // setPosition since the last start: a start plays from there
    double      resumedAt;       // when it was last started, for LogQuickStreamEnd
    volatile ma_uint32 endPending; // set on the audio thread, drained by System::update()
    SoundImpl*  src;
    void*       user;
    FMOD::FMOD_CHANNELCONTROL_CALLBACK callback;
    float       volume;
    float       occlusion;
    int         priority;
    unsigned    order;           // play order, for the voice-stealing tie-break
    uintptr_t   gen;             // bumped by every playSound; part of the Channel handle
};

FMOD::System  g_system;
ma_context    g_context;
ma_engine     g_engine;
bool          g_contextInit;
bool          g_engineInit;
bool          g_noDevice;        // no output device: we pump the graph from update() instead
bool          g_trace;
Voice*        g_voices;
int           g_numVoices;
unsigned      g_playOrder;
double        g_lastPump = -1.0;
float*        g_pumpBuf;
size_t        g_pumpFrames;
ma_encoder    g_wav;             // K2_AUDIO_WAV: the whole mix, written from PumpSilent
bool          g_wavInit;

FMOD::FMOD_FILE_OPENCALLBACK  g_fileOpen;
FMOD::FMOD_FILE_CLOSECALLBACK g_fileClose;
FMOD::FMOD_FILE_READCALLBACK  g_fileRead;

void Trace(const char* fmt, ...)
{
    if (!g_trace) return;
    va_list ap;
    va_start(ap, fmt);
    fputs("[k2audio] ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}

double Now()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

ma_decoder_config DecoderConfig(const SoundImpl* s)
{
    ma_decoder_config cfg = ma_decoder_config_init(ma_format_f32, 0, 0);   // 0,0 = keep native
    if (s->encoding != ma_encoding_format_unknown)
        cfg.encodingFormat = s->encoding;                                   // never re-sniff
    return cfg;
}

// Length, rate and channel count, decoded once and cached. For MP3 this walks every frame header
// in the payload (dr_mp3 has no other way to count), which is why it is lazy: nothing pays for it
// unless the engine asks, and GetSoundLength/GetSoundSampleRate/GetStreamLength are the only
// askers.
void Probe(SoundImpl* s)
{
    if (s->probed) return;
    s->probed = true;
    ma_decoder        dec;
    ma_decoder_config cfg = DecoderConfig(s);
    if (ma_decoder_init_memory(s->data + s->payloadOff, s->payloadLen, &cfg, &dec) != MA_SUCCESS)
    {
        Trace("probe FAILED (%zu bytes, kind %d)", s->payloadLen, static_cast<int>(s->encoding));
        return;
    }
    ma_uint64 frames = 0;
    ma_decoder_get_length_in_pcm_frames(&dec, &frames);
    s->frames     = frames;
    s->sampleRate = dec.outputSampleRate;
    s->channels   = dec.outputChannels;
    ma_decoder_uninit(&dec);
    Trace("probe %llu frames @ %u Hz x%u", static_cast<unsigned long long>(s->frames), s->sampleRate, s->channels);
}

// Makes a Channel handle, the generation above the voice index + 1 (never null). Resolve answers 0
// for a handle whose voice has been handed to a newer playSound.
FMOD::Channel* MakeHandle(Voice* v)
{
    return reinterpret_cast<FMOD::Channel*>(v->gen << 16 | static_cast<uintptr_t>(v - g_voices + 1));
}

Voice* Resolve(const void* h)
{
    uintptr_t n = reinterpret_cast<uintptr_t>(h);
    uintptr_t i = (n & 0xffff) - 1;
    if (!g_voices || i >= static_cast<uintptr_t>(g_numVoices))
        return 0;
    if (g_voices[i].gen != n >> 16)
    {
        Trace("stale channel handle: voice %d generation %lu, now %lu", static_cast<int>(i),
              static_cast<uint64_t>(n >> 16), static_cast<uint64_t>(g_voices[i].gen));
        return 0;
    }
    return &g_voices[i];
}

void TeardownVoice(Voice* v)
{
    if (v->soundInit)   { ma_sound_uninit(&v->sound);     v->soundInit = false; }
    if (v->decoderInit) { ma_decoder_uninit(&v->decoder); v->decoderInit = false; }
    v->inUse      = false;
    v->src        = 0;
    v->endPending = 0;
}

// The engine never over-subscribes the pool, so the first free slot is the normal answer. When
// there is none, FMOD's own rule applies: 0 is the most important priority, so the victim is the
// highest priority value, oldest first.
Voice* AcquireVoice()
{
    Voice* victim = 0;
    for (int i = 0; i < g_numVoices; i++)
    {
        Voice* v = &g_voices[i];
        if (!v->inUse)
            return v;
        if (victim == 0 || v->priority > victim->priority
            || (v->priority == victim->priority && v->order < victim->order))
            victim = v;
    }
    static bool s_stoleOnce;
    if (victim && !s_stoleOnce)
    {
        s_stoleOnce = true;
        fprintf(stderr, "k2audio: voice pool full (%d): stealing voice %d (priority %d)\n",
                g_numVoices, static_cast<int>(victim - g_voices), victim->priority);
    }
    if (victim)
        Trace("stealing voice %d (priority %d)", static_cast<int>(victim - g_voices), victim->priority);
    return victim;
}

// The pool's high-water mark is logged regardless of K2_AUDIO_TRACE, each time it passes another
// eighth of the pool, with how many of the busy voices are paused. A pool that only ever fills is a
// leak of paused voices, and the engine's channel records -- one per voice -- run out with it.
int g_poolHigh;

void LogPoolHighWater(int busy, int parked)
{
    int step = g_numVoices / 8 > 0 ? g_numVoices / 8 : 1;
    if (busy < g_poolHigh + step && !(busy == g_numVoices && g_poolHigh < g_numVoices))
        return;
    g_poolHigh = busy;
    fprintf(stderr, "k2audio: %d of %d voices busy (%d paused)\n", busy, g_numVoices, parked);
}

// The two ways a stream (music, ambience, VO) can fall silent the moment it is resumed are logged
// once each, regardless of K2_AUDIO_TRACE -- positioned past its end, or ending within a second of
// a start. The engine's own reply to a stream that ends is to park it paused until something
// resumes it.
bool g_loggedSeekPastEnd;
bool g_loggedQuickEnd;

void LogOnce(bool* pDone, const char* fmt, ...)
{
    if (*pDone)
        return;
    *pDone = true;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

void LogQuickStreamEnd(const Voice* v)
{
    if (g_loggedQuickEnd || !v->src || !v->src->stream || g_noDevice)
        return;
    double played = Now() - v->resumedAt;
    if (played >= 1.0)
        return;
    Probe(v->src);
    if (v->src->frames >= 2 * static_cast<ma_uint64>(v->src->sampleRate))
        LogOnce(&g_loggedQuickEnd, "k2audio: a %llu-frame stream ended %.2f s after it was started\n",
                static_cast<unsigned long long>(v->src->frames), played);
}

void OnSoundEnd(void* pUserData, ma_sound* /*pSound*/)
{
    // Audio thread. The miniaudio docs forbid touching the sound from here; a flag is all we do.
    static_cast<Voice*>(pUserData)->endPending = 1;
}

void ApplyVolume(Voice* v)
{
    if (!v->soundInit) return;
    float occ = v->occlusion < 0.0f ? 0.0f : (v->occlusion > 1.0f ? 1.0f : v->occlusion);
    ma_sound_set_volume(&v->sound, v->volume * (1.0f - occ));
}

// A spatial voice's left/right pan from where it sits in listener space (+x is the listener's
// right). Balance-mode pan keeps the near ear at 1 and fades the far one, floored at 0.2 -- the
// same far-ear floor miniaudio's own spatializer uses -- so a centred voice plays at full level.
void ApplySpatialPan(Voice* v)
{
    if (!v->soundInit || !v->src || !v->src->spatial)
        return;
    ma_vec3f rel;
    ma_spatializer_get_relative_position_and_direction(&v->sound.engineNode.spatializer,
                                                       &g_engine.listeners[0], &rel, 0);
    float len = sqrtf(rel.x * rel.x + rel.y * rel.y + rel.z * rel.z);
    float pan = len > 0.001f ? rel.x / len : 0.0f;
    pan *= 0.8f;
    ma_sound_set_pan(&v->sound, pan);
}

// Pumps the node graph by wall-clock time. Only the no-device path needs this; with a device the
// audio thread does it. Keeps end callbacks, cursors and durations behaving the same way in both,
// which is what lets a machine with no output still finish a conversation.
void PumpSilent()
{
    double now = Now();
    double dt  = (g_lastPump < 0.0) ? 0.0 : now - g_lastPump;
    g_lastPump = now;
    if (dt <= 0.0) return;
    if (dt > 0.25) dt = 0.25;        // a long stall must not blast through a whole line of VO

    ma_uint32 rate     = ma_engine_get_sample_rate(&g_engine);
    ma_uint32 channels = ma_engine_get_channels(&g_engine);
    size_t    want     = static_cast<size_t>(dt * rate) + 1;
    if (want > g_pumpFrames)
    {
        float* grown = static_cast<float*>(realloc(g_pumpBuf, want * channels * sizeof(float)));
        if (!grown) return;
        g_pumpBuf    = grown;
        g_pumpFrames = want;
    }
    ma_uint64 read = 0;
    ma_engine_read_pcm_frames(&g_engine, g_pumpBuf, want, &read);
    if (g_wavInit && read)
        ma_encoder_write_pcm_frames(&g_wav, g_pumpBuf, read, 0);
}

void ShutdownAll()
{
    for (int i = 0; i < g_numVoices; i++)
        TeardownVoice(&g_voices[i]);
    if (g_engineInit) { ma_engine_uninit(&g_engine); g_engineInit = false; }
    if (g_wavInit)    { ma_encoder_uninit(&g_wav);   g_wavInit = false; }
    free(g_voices);  g_voices  = 0; g_numVoices  = 0;
    free(g_pumpBuf); g_pumpBuf = 0; g_pumpFrames = 0;
    g_lastPump = -1.0;
    g_poolHigh = 0;
}

}  // namespace

// The app has gone to the background (Android) or come back. miniaudio mixes on its own device
// thread, so the engine's own pause (WM_ACTIVATEAPP -> SetSoundMode LOSTFOCUS) is not enough: on
// Android SDL blocks the main thread inside the event pump before that message is dispatched.
// Stopping the device here is what SDL does for its own audio devices at the same point.
extern "C" void k2audio_set_suspended(int suspended)
{
    if (!g_engineInit || g_noDevice)
        return;
    ma_result r = suspended ? ma_engine_stop(&g_engine) : ma_engine_start(&g_engine);
    Trace("%s device: %d", suspended ? "suspend" : "resume", static_cast<int>(r));
}

// ------------------------------------------------------------------------------------------
// FMOD::System
// ------------------------------------------------------------------------------------------

// The engine creates a system more than once. CExoSoundInternal::Initialize on a live system tears
// the backend down (ShutdownSystem, which only drops a flag) and returns false; the next Initialize
// runs InitSystem again, i.e. FMOD_System_Create + System::init. Real FMOD answers that with a
// brand-new System object and leaks the old one, so a second Create here has to hand back a system
// that init() will accept, not the spent one.
extern "C" FMOD_RESULT FMOD_System_Create(FMOD::System** system)
{
    g_trace = getenv("K2_AUDIO_TRACE") != 0;
    if (g_engineInit)
    {
        Trace("re-create: tearing the previous system down");
        ShutdownAll();
    }
#ifdef K2P_IOS
    // Use the audio session SDL sets up for a movie's sound (Playback, mixing with and ducking
    // other apps, in SDL_coreaudio.m). With miniaudio's default (PlayAndRecord) SDL changes the
    // category when the movie opens its device, which deactivates the session and silences the game
    // until the app is next brought back from the background.
    ma_context_config ctxCfg = ma_context_config_init();
    ctxCfg.coreaudio.sessionCategory = ma_ios_session_category_playback;
    ctxCfg.coreaudio.sessionCategoryOptions =
        ma_ios_session_category_option_mix_with_others | ma_ios_session_category_option_duck_others;
    if (!g_contextInit && ma_context_init(0, 0, &ctxCfg, &g_context) == MA_SUCCESS)
        g_contextInit = true;
#else
    if (!g_contextInit && ma_context_init(0, 0, 0, &g_context) == MA_SUCCESS)
        g_contextInit = true;
#endif
    *system = &g_system;
    return FMOD_OK;
}

namespace FMOD {

// InitSystem picks NOSOUND when this answers 0, so answer it for real.
FMOD_RESULT System::getNumDrivers(int* n)
{
    *n = 0;
    if (g_contextInit)
    {
        ma_device_info* playback = 0;
        ma_uint32       count    = 0;
        if (ma_context_get_devices(&g_context, &playback, &count, 0, 0) == MA_SUCCESS)
            *n = static_cast<int>(count);
    }
    Trace("getNumDrivers -> %d", *n);
    return FMOD_OK;
}

FMOD_RESULT System::setOutput(FMOD_OUTPUTTYPE output)
{
    g_noDevice = (output == FMOD_OUTPUTTYPE_NOSOUND || output == FMOD_OUTPUTTYPE_NOSOUND_NRT);
    return FMOD_OK;
}

FMOD_RESULT System::init(int maxchannels, unsigned int, void*)
{
    if (g_engineInit)
        return FMOD_ERR_INITIALIZED;

    const char* wavPath = getenv("K2_AUDIO_WAV");
    if (wavPath)
        g_noDevice = true;

    g_numVoices = maxchannels > 0 ? maxchannels : 64;
    g_voices    = static_cast<Voice*>(calloc(static_cast<size_t>(g_numVoices), sizeof(Voice)));
    if (!g_voices)
        return FMOD_ERR_INITIALIZATION;
    for (int i = 0; i < g_numVoices; i++)
    {
        g_voices[i].volume   = 1.0f;
        g_voices[i].priority = 128;
    }

    for (int attempt = 0; attempt < 2 && !g_engineInit; attempt++)
    {
        ma_engine_config cfg = ma_engine_config_init();
        cfg.listenerCount    = 1;
        if (g_noDevice || attempt == 1)
        {
            cfg.noDevice   = MA_TRUE;
            cfg.channels   = 2;
            cfg.sampleRate = 44100;
            g_noDevice     = true;
        }
        else
        {
            cfg.pContext = g_contextInit ? &g_context : 0;
        }
        if (ma_engine_init(&cfg, &g_engine) == MA_SUCCESS)
            g_engineInit = true;
        else
            Trace("ma_engine_init failed (attempt %d)", attempt);
    }
    if (!g_engineInit)
        return FMOD_ERR_INITIALIZATION;

    // KOTOR's world is Z-up, and FMOD's 3D space is left-handed: miniaudio defaults to Y-up and
    // right-handed, which would mirror every pan. Both are listener properties.
    ma_engine_listener_set_world_up(&g_engine, 0, 0.0f, 0.0f, 1.0f);
    g_engine.listeners[0].config.handedness = ma_handedness_left;

    if (wavPath)
    {
        ma_encoder_config enc = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32,
                                                       ma_engine_get_channels(&g_engine),
                                                       ma_engine_get_sample_rate(&g_engine));
        g_wavInit = ma_encoder_init_file(wavPath, &enc, &g_wav) == MA_SUCCESS;
        Trace("K2_AUDIO_WAV=%s: %s", wavPath, g_wavInit ? "recording" : "COULD NOT OPEN");
    }

    Trace("init: %d voices, %u Hz x%u%s", g_numVoices, ma_engine_get_sample_rate(&g_engine),
          ma_engine_get_channels(&g_engine), g_noDevice ? " (no device)" : "");
    return FMOD_OK;
}

FMOD_RESULT System::release()
{
    ShutdownAll();
    if (g_contextInit) { ma_context_uninit(&g_context); g_contextInit = false; }
    return FMOD_OK;
}

// The end callbacks land here, on the game thread, as real FMOD's do. The callback re-enters this
// file (HandleChannelEnd -> playSound for a loop or a stream reboot), so everything the voice owns is
// read out before the call and nothing is touched after it.
FMOD_RESULT System::update()
{
    if (!g_engineInit)
        return FMOD_OK;
    if (g_noDevice)
        PumpSilent();

    int busy = 0, parked = 0;
    for (int i = 0; i < g_numVoices; i++)
    {
        Voice* v = &g_voices[i];
        if (v->inUse)
        {
            busy++;
            parked += v->paused;
        }
    }
    LogPoolHighWater(busy, parked);

    for (int i = 0; i < g_numVoices; i++)
    {
        Voice* v = &g_voices[i];
        if (v->inUse)
            ApplySpatialPan(v);
        if (!v->endPending)
            continue;
        v->endPending = 0;
        if (!v->inUse)
            continue;          // stopped or torn down since the audio thread raised it
        v->inUse = false;
        LogQuickStreamEnd(v);
        FMOD_CHANNELCONTROL_CALLBACK cb = v->callback;
        uintptr_t gen = v->gen;
        Trace("end voice %d", i);
        if (cb)
            cb(reinterpret_cast<FMOD_CHANNELCONTROL*>(MakeHandle(v)), FMOD_CHANNELCONTROL_CHANNEL,
               FMOD_CHANNELCONTROL_CALLBACK_END, 0, 0);
        // A channel that has played out is gone, as FMOD's is: every later call through its handle
        // fails. (Unless the callback's own playSound already took this voice for the next sound.)
        if (v->gen == gen)
            v->gen++;
    }
    return FMOD_OK;
}

FMOD_RESULT System::setFileSystem(FMOD_FILE_OPENCALLBACK useropen, FMOD_FILE_CLOSECALLBACK userclose,
                                  FMOD_FILE_READCALLBACK userread, FMOD_FILE_SEEKCALLBACK /*userseek*/,
                                  FMOD_FILE_ASYNCREADCALLBACK, FMOD_FILE_ASYNCCANCELCALLBACK, int)
{
    // userseek is deliberately dropped: the engine's is a no-op (FModAudioSystem.cpp), so we read
    // each stream forward, whole, and seek inside our own copy.
    g_fileOpen  = useropen;
    g_fileClose = userclose;
    g_fileRead  = userread;
    return FMOD_OK;
}

FMOD_RESULT System::setStreamBufferSize(unsigned int, unsigned int) { return FMOD_OK; }

FMOD_RESULT System::set3DListenerAttributes(int listener, const FMOD_VECTOR* pos,
                                            const FMOD_VECTOR* vel, const FMOD_VECTOR* forward,
                                            const FMOD_VECTOR* up)
{
    // Set3DListenerPosition and Set3DListenerOrientation each pass NULL for the other's fields, so a
    // null here means "leave alone", never "zero".
    if (!g_engineInit || listener != 0)
        return FMOD_OK;
    if (pos)     ma_engine_listener_set_position(&g_engine, 0, pos->x, pos->y, pos->z);
    if (vel)     ma_engine_listener_set_velocity(&g_engine, 0, vel->x, vel->y, vel->z);
    if (forward) ma_engine_listener_set_direction(&g_engine, 0, forward->x, forward->y, forward->z);
    if (up)      ma_engine_listener_set_world_up(&g_engine, 0, up->x, up->y, up->z);
    return FMOD_OK;
}

FMOD_RESULT System::getChannel(int channelid, Channel** channel)
{
    if (!g_voices || channelid < 0 || channelid >= g_numVoices)
        return FMOD_ERR_INVALID_PARAM;
    *channel = MakeHandle(&g_voices[channelid]);
    return FMOD_OK;
}

FMOD_RESULT System::playSound(Sound* sound, ChannelGroup*, bool paused, Channel** channel)
{
    SoundImpl* s = reinterpret_cast<SoundImpl*>(sound);
    if (!g_engineInit || !s || !s->alive)
        return FMOD_ERR_INVALID_HANDLE;

    Voice* v = AcquireVoice();
    if (!v)
        return FMOD_ERR_CHANNEL_ALLOC;
    TeardownVoice(v);
    v->gen++;   // every handle to this voice's previous sound is now stale

    v->user      = 0;
    v->callback  = 0;
    v->volume    = 1.0f;
    v->occlusion = 0.0f;
    v->priority  = 128;

    ma_decoder_config cfg = DecoderConfig(s);
    if (ma_decoder_init_memory(s->data + s->payloadOff, s->payloadLen, &cfg, &v->decoder) != MA_SUCCESS)
    {
        Trace("decoder init FAILED (%zu bytes)", s->payloadLen);
        return FMOD_ERR_FORMAT;
    }
    v->decoderInit = true;

    // Looping has to go through the flag, not ma_data_source_set_looping: the last thing
    // ma_sound_init_from_data_source_internal does is overwrite the data source's looping with
    // whatever the flag says, so setting it beforehand is silently undone (and FMOD_LOOP_NORMAL
    // music then ends after one pass and comes back only through the engine's reboot path).
    ma_uint32 flags = s->spatial ? 0u : static_cast<ma_uint32>(MA_SOUND_FLAG_NO_SPATIALIZATION);
    if (s->loop)
        flags |= static_cast<ma_uint32>(MA_SOUND_FLAG_LOOPING);
    if (ma_sound_init_from_data_source(&g_engine, &v->decoder, flags, 0, &v->sound) != MA_SUCCESS)
    {
        ma_decoder_uninit(&v->decoder);
        v->decoderInit = false;
        return FMOD_ERR_INTERNAL;
    }
    v->soundInit = true;
    ma_sound_set_end_callback(&v->sound, OnSoundEnd, v);
    // Distance rolloff only; the directions belong to ApplySpatialPan (see the top of the file).
    if (s->spatial)
        ma_sound_set_directional_attenuation_factor(&v->sound, 0.0f);

    v->src       = s;
    v->inUse     = true;
    v->paused    = paused;
    v->rewound   = false;
    v->resumedAt = Now();
    v->order     = ++g_playOrder;
    ApplySpatialPan(v);
    if (!paused)
        ma_sound_start(&v->sound);

    Trace("play voice %d: sound %p (%zu bytes)%s%s%s", static_cast<int>(v - g_voices), static_cast<void*>(s), s->size,
          s->stream ? " stream" : "", s->loop ? " loop" : "", paused ? " paused" : "");
    if (channel)
        *channel = MakeHandle(v);
    return FMOD_OK;
}

// Two shapes, and the "name" argument means something different in each:
//   FMOD_OPENMEMORY -- `name` is the audio image, exinfo->length its size (CreateSound);
//   FMOD_CREATESTREAM -- `name` is the decimal stream id to feed back through the engine's own
//                        open/read callbacks (CreateStream).
FMOD_RESULT System::createSound(const char* name_or_data, unsigned int mode,
                                FMOD_CREATESOUNDEXINFO* exinfo, Sound** sound)
{
    unsigned int   length = exinfo ? exinfo->length : 0;
    unsigned char* data   = 0;
    size_t         size   = 0;

    if (mode & FMOD_OPENMEMORY)
    {
        if (!name_or_data || length == 0)
            return FMOD_ERR_INVALID_PARAM;
        // FMOD_OPENMEMORY (as opposed to _POINT) copies, and the engine frees its buffer.
        data = static_cast<unsigned char*>(malloc(length));
        if (!data)
            return FMOD_ERR_INVALID_PARAM;
        memcpy(data, name_or_data, length);
        size = length;
    }
    else if (mode & FMOD_CREATESTREAM)
    {
        if (!g_fileOpen || !g_fileRead || !name_or_data)
            return FMOD_ERR_FILE_NOTFOUND;
        unsigned int filesize = 0;
        void*        handle   = 0;
        FMOD_RESULT  r        = g_fileOpen(name_or_data, &filesize, &handle, 0);
        if (r != FMOD_OK)
            return r;
        // The engine never writes the size out-parameter, so exinfo->length (CreateStream's own
        // nSize) is the only length there is. Read a little past it anyway and stop at EOF.
        size_t cap = length ? length : (1u << 20);
        data = static_cast<unsigned char*>(malloc(cap));
        while (data && size < cap)
        {
            unsigned int got   = 0;
            unsigned int chunk = static_cast<unsigned int>(cap - size);
            if (chunk > (1u << 16)) chunk = 1u << 16;
            FMOD_RESULT rr = g_fileRead(handle, data + size, chunk, &got, 0);
            size += got;
            if (rr != FMOD_OK || got == 0)
                break;
        }
        if (g_fileClose)
            g_fileClose(handle, 0);
        if (!data || size == 0)
        {
            free(data);
            return FMOD_ERR_FILE_BAD;
        }
        Trace("stream \"%s\": %zu bytes", name_or_data, size);
    }
    else
    {
        return FMOD_ERR_FILE_NOTFOUND;   // the engine never creates a sound from a real path
    }

    k2wav_payload payload;
    k2wav_sniff(data, size, &payload);

    SoundImpl* s = static_cast<SoundImpl*>(calloc(1, sizeof(SoundImpl)));
    if (!s)
    {
        free(data);
        return FMOD_ERR_INVALID_PARAM;
    }
    s->data        = data;
    s->size        = size;
    s->payloadOff  = payload.offset;
    s->payloadLen  = payload.length;
    s->encoding    = payload.kind == K2WAV_MP3  ? ma_encoding_format_mp3
                   : payload.kind == K2WAV_RIFF ? ma_encoding_format_wav
                                                : ma_encoding_format_unknown;
    s->loop        = (mode & FMOD_LOOP_NORMAL) != 0;
    s->spatial     = (mode & FMOD_3D) != 0;
    s->stream      = (mode & FMOD_CREATESTREAM) != 0;
    s->alive       = true;
    *sound         = reinterpret_cast<Sound*>(s);

    Trace("createSound %p: %zu bytes -> %s payload at +%zu (%zu bytes)%s", static_cast<void*>(s), size,
          payload.kind == K2WAV_MP3 ? "mp3" : payload.kind == K2WAV_RIFF ? "riff" : "unknown",
          payload.offset, payload.length, s->spatial ? " 3d" : "");
    return FMOD_OK;
}

// ------------------------------------------------------------------------------------------
// FMOD::Sound
// ------------------------------------------------------------------------------------------

FMOD_RESULT Sound::setUserData(void* userdata)
{
    // CreateStream passes a stack buffer here: store it, never dereference it.
    reinterpret_cast<SoundImpl*>(this)->user = userdata;
    return FMOD_OK;
}

FMOD_RESULT Sound::release()
{
    SoundImpl* s = reinterpret_cast<SoundImpl*>(this);
    // Every live voice decodes straight out of s->data, so they have to go first -- the engine
    // stops them before releasing, but only the ones its channel table still knows about.
    for (int i = 0; i < g_numVoices; i++)
    {
        if (g_voices[i].src == s)
            TeardownVoice(&g_voices[i]);
    }
    s->alive = false;
    free(s->data);
    free(s);
    return FMOD_OK;
}

// Units: 1 ms, 2 PCM frames, 4 PCM bytes. Never answer 0 ms: GetSoundSampleRate divides by it.
FMOD_RESULT Sound::getLength(unsigned int* length, unsigned int lengthtype)
{
    SoundImpl* s = reinterpret_cast<SoundImpl*>(this);
    Probe(s);
    if (s->sampleRate == 0)
        return FMOD_ERR_FORMAT;
    switch (lengthtype)
    {
    case FMOD_TIMEUNIT_PCM:      *length = static_cast<unsigned int>(s->frames); break;
    case FMOD_TIMEUNIT_PCMBYTES: *length = static_cast<unsigned int>(s->frames * s->channels * 2); break;
    default:                     *length = static_cast<unsigned int>(s->frames * 1000 / s->sampleRate); break;
    }
    if (*length == 0)
        *length = 1;
    return FMOD_OK;
}

// ------------------------------------------------------------------------------------------
// FMOD::ChannelControl
// ------------------------------------------------------------------------------------------

// Every row here can land on a voice that has never played anything: InitChannels caches one
// System::getChannel handle per channel record and PauseAllActiveChannels walks all of them, hence
// the soundInit guard on each.
FMOD_RESULT ChannelControl::stop()
{
    Voice* v = Resolve(this);
    if (!v)
        return FMOD_ERR_INVALID_HANDLE;
    if (v->soundInit)
    {
        Trace("stop voice %d", static_cast<int>(v - g_voices));
        ma_sound_stop(&v->sound);
    }
    // The voice is free at once and the handle dead; no end follows (see the top of the file).
    v->inUse      = false;
    v->paused     = false;
    v->endPending = 0;
    v->gen++;
    return FMOD_OK;
}

FMOD_RESULT ChannelControl::setPaused(bool paused)
{
    Voice* v  = Resolve(this);
    if (!v)
        return FMOD_ERR_INVALID_HANDLE;
    if (v->soundInit && v->paused != paused)
        Trace("%s voice %d", paused ? "pause" : "resume", static_cast<int>(v - g_voices));
    v->paused = paused;
    if (!v->soundInit)
        return FMOD_OK;
    if (paused)
        ma_sound_stop(&v->sound);          // miniaudio's stop keeps the cursor: it is a pause
    else if (v->inUse && ma_sound_at_end(&v->sound) && !v->rewound)
        v->endPending = 1;                 // paused between its last frame and its end: finish it,
                                           // where ma_sound_start would play it again from the top
    else if (v->inUse)
    {
        v->resumedAt = Now();
        v->rewound   = false;
        ma_sound_start(&v->sound);
    }
    return FMOD_OK;
}

FMOD_RESULT ChannelControl::isPlaying(bool* isplaying)
{
    Voice* v   = Resolve(this);
    if (!v)
    {
        *isplaying = false;
        return FMOD_ERR_INVALID_HANDLE;
    }
    *isplaying = v->inUse && !v->paused && v->soundInit && ma_sound_is_playing(&v->sound);
    return FMOD_OK;
}

FMOD_RESULT ChannelControl::setVolume(float volume)
{
    Voice* v  = Resolve(this);
    if (!v)
        return FMOD_ERR_INVALID_HANDLE;
    v->volume = volume;
    ApplyVolume(v);
    return FMOD_OK;
}

FMOD_RESULT ChannelControl::setPan(float pan)
{
    Voice* v = Resolve(this);
    if (!v)
        return FMOD_ERR_INVALID_HANDLE;
    if (v->soundInit && !(v->src && v->src->spatial))
        ma_sound_set_pan(&v->sound, pan);
    return FMOD_OK;
}

FMOD_RESULT ChannelControl::set3DAttributes(const FMOD_VECTOR* position, const FMOD_VECTOR* velocity)
{
    Voice* v = Resolve(this);
    if (!v)
        return FMOD_ERR_INVALID_HANDLE;
    if (!v->soundInit)
        return FMOD_OK;
    if (position) ma_sound_set_position(&v->sound, position->x, position->y, position->z);
    if (velocity) ma_sound_set_velocity(&v->sound, velocity->x, velocity->y, velocity->z);
    ApplySpatialPan(v);
    return FMOD_OK;
}

FMOD_RESULT ChannelControl::set3DMinMaxDistance(float mindistance, float maxdistance)
{
    Voice* v = Resolve(this);
    if (!v)
        return FMOD_ERR_INVALID_HANDLE;
    if (!v->soundInit)
        return FMOD_OK;
    ma_sound_set_min_distance(&v->sound, mindistance);
    ma_sound_set_max_distance(&v->sound, maxdistance);
    return FMOD_OK;
}

// Direct term as gain only; the lowpass belongs to the later stage (see the deliberate gaps above).
FMOD_RESULT ChannelControl::set3DOcclusion(float directocclusion, float /*reverbocclusion*/)
{
    Voice* v     = Resolve(this);
    if (!v)
        return FMOD_ERR_INVALID_HANDLE;
    v->occlusion = directocclusion;
    ApplyVolume(v);
    return FMOD_OK;
}

FMOD_RESULT ChannelControl::setUserData(void* userdata)
{
    Voice* v = Resolve(this);
    if (!v)
        return FMOD_ERR_INVALID_HANDLE;
    v->user = userdata;
    return FMOD_OK;
}

FMOD_RESULT ChannelControl::getUserData(void** userdata)
{
    Voice* v = Resolve(this);
    *userdata = v ? v->user : 0;
    if (!v)
        return FMOD_ERR_INVALID_HANDLE;
    return FMOD_OK;
}

FMOD_RESULT ChannelControl::setCallback(FMOD_RESULT (*callback)(FMOD_CHANNELCONTROL*,
                                                                FMOD_CHANNELCONTROL_TYPE,
                                                                FMOD_CHANNELCONTROL_CALLBACK_TYPE,
                                                                void*, void*))
{
    Voice* v = Resolve(this);
    if (!v)
        return FMOD_ERR_INVALID_HANDLE;
    v->callback = callback;
    return FMOD_OK;
}

// ------------------------------------------------------------------------------------------
// FMOD::Channel
// ------------------------------------------------------------------------------------------

// FMOD's frequency is absolute Hz, miniaudio's pitch is a ratio against the source's own rate.
FMOD_RESULT Channel::setFrequency(float frequency)
{
    Voice* v = Resolve(this);
    if (!v)
        return FMOD_ERR_INVALID_HANDLE;
    if (!v->soundInit || !v->src)
        return FMOD_OK;
    Probe(v->src);
    if (v->src->sampleRate == 0 || frequency <= 0.0f)
        return FMOD_OK;
    ma_sound_set_pitch(&v->sound, frequency / static_cast<float>(v->src->sampleRate));
    return FMOD_OK;
}

FMOD_RESULT Channel::setPriority(int priority)
{
    Voice* v = Resolve(this);
    if (!v)
        return FMOD_ERR_INVALID_HANDLE;
    v->priority = priority;
    return FMOD_OK;
}

// postype is always FMOD_TIMEUNIT_MS from this engine (SetChannelPlaybackPosition passes 1).
FMOD_RESULT Channel::setPosition(unsigned int position, unsigned int postype)
{
    Voice* v = Resolve(this);
    if (!v)
        return FMOD_ERR_INVALID_HANDLE;
    if (!v->soundInit || !v->src)
        return FMOD_OK;
    Probe(v->src);
    ma_uint64 frame = position;
    if (postype != FMOD_TIMEUNIT_PCM)
        frame = static_cast<ma_uint64>(position) * v->src->sampleRate / 1000;
    if (v->src->stream && v->src->frames && frame >= v->src->frames)
        LogOnce(&g_loggedSeekPastEnd, "k2audio: a stream was positioned at %u ms, past its end "
                "(%llu frames): it ends the moment it resumes\n", position,
                static_cast<unsigned long long>(v->src->frames));
    ma_sound_seek_to_pcm_frame(&v->sound, frame);
    v->rewound = true;
    return FMOD_OK;
}

FMOD_RESULT Channel::getPosition(unsigned int* position, unsigned int postype)
{
    Voice* v  = Resolve(this);
    *position = 0;
    if (!v)
        return FMOD_ERR_INVALID_HANDLE;
    if (!v->soundInit || !v->src)
        return FMOD_ERR_INVALID_HANDLE;
    Probe(v->src);
    ma_uint64 cursor = 0;
    if (ma_sound_get_cursor_in_pcm_frames(&v->sound, &cursor) != MA_SUCCESS)
        return FMOD_ERR_INVALID_HANDLE;
    *position = (postype == FMOD_TIMEUNIT_PCM || v->src->sampleRate == 0)
                    ? static_cast<unsigned int>(cursor)
                    : static_cast<unsigned int>(cursor * 1000 / v->src->sampleRate);
    return FMOD_OK;
}

}  // namespace FMOD
