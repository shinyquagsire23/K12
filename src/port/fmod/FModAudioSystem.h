#ifndef FMODAUDIOSYSTEM_H
#define FMODAUDIOSYSTEM_H
#include "k2stdint.h"

// The audio-library wrapper behind the sound backend holds a library system object, the
// sound/stream/channel id tables and the channel-property cache that lets a recycled channel be
// rebuilt.

#include "FModAPI.h"
#include "FModAudioSystemInfo.h" 

class CExoSound;

class FModAudioSystem
{
public:
    FModAudioSystem();
    virtual ~FModAudioSystem();

    void InitSystem(int nVoiceHandles);
    int  InitChannels();
    void ShutdownSystem();

    // Called from the message pump once per frame; re-runs the background check.
    void UpdateSystem();
    void CheckAppBackgroundStatus();

    static int  CheckFModResult(FMOD_RESULT result);

    // File-system callbacks InitSystem registers with System::setFileSystem. All static: they reach
    // the object through the g_pExoSound global.
    static FMOD_RESULT SystemOpenCallback(const char* pszName, unsigned int* pnSize,
                                          void** ppHandle, void* pUserData);
    static FMOD_RESULT SystemCloseCallback(void* pHandle, void* pUserData);
    static FMOD_RESULT SystemReadCallback(void* pHandle, void* pBuffer, unsigned int nSize,
                                          unsigned int* pnRead, void* pUserData);
    static FMOD_RESULT SystemSeekCallback(void* pHandle, unsigned int nPos, void* pUserData);

    // Per-channel END callback, armed on every channel this class hands out; resolves the
    // ChannelInfo from the library's user data and forwards to HandleChannelEnd.
    static FMOD_RESULT ChannelCallback(FMOD_CHANNELCONTROL* pControl,
                                       FMOD_CHANNELCONTROL_TYPE nType,
                                       FMOD_CHANNELCONTROL_CALLBACK_TYPE nCallbackType,
                                       void* pCommandA, void* pCommandB);

    // Id-table lookups; all three return 0 when the key is absent.
    FModAudioSystemSoundInfo*   GetSoundFromMap(int nSoundId);
    FModAudioSystemStreamInfo*  GetStreamInfoFromMap(unsigned int nStreamId);
    FModAudioSystemChannelInfo* GetChannelInfoByUID(unsigned int nChannel);

    // The first channel whose m_bInUse is still 0, or 0 when every channel is busy.
    FModAudioSystemChannelInfo* GetFreeChannel();
    void ResetChannelInfo(FModAudioSystemChannelInfo* pInfo);

    // The 3D listener; both return non-zero on success, which every caller discards.
    int  Set3DListenerPosition(float x, float y, float z);
    int  Set3DListenerOrientation(float dx, float dy, float dz,
                                  float ux, float uy, float uz);

    // CreateSound takes an already-buffered WAV and keys it by an engine-side id.
    // ReleaseSound(int) is a refcount drop; ReleaseSound(sound pointer) is the real teardown, which
    // also stops every channel still pointing at that sound.
    int  CreateSound(char* pszName, int nSoundId, void* pData, unsigned int nLength, int b3D, int bFlags);
    int  ReleaseSound(int nSoundId);
    int  ReleaseSound(FMOD::Sound* pSound);
#if KOTOR1  // K1 has no Windows A-suffix rename and 64-bit channel ids
    uint64_t PlaySound(int nSoundId);
    uint64_t PlaySoundA(int nSoundId) { return PlaySound(nSoundId); }
#else
    int  PlaySoundA(int nSoundId);
#endif

    // A stream's name is its id, so the file-system callbacks parse the name with strtoul.
    // CreateStream returns the new stream id; PlayStream returns the channel it landed on, or -1.
#if KOTOR1  // K1 streams are SDL_RWops and ids are 64-bit
    uint64_t CreateStream(char* pszName, SDL_RWops* pFile, int nOffset, int nSize,
                               int bSkip, int b3D, int bLoop);
    uint64_t PlayStream(uint64_t nStreamId, int bLoop);
    int  CloseStream(uint64_t nStreamId);
    int  GetStreamLength(uint64_t nStreamId);
    int  PauseChannel(uint64_t nChannel);
    int  ResumeChannel(uint64_t nChannel);
    int  StopChannel(uint64_t nChannel);
    void PauseAllActiveChannels();
    void ResumeAllActiveChannels();
    int  SetChannelVolume(uint64_t nChannel, float fVolume);
    int  SetChannelFrequency(uint64_t nChannel, float fFrequency);
    int  SetChannelPan(uint64_t nChannel, float fPan);
    int  SetChannelLoopCount(uint64_t nChannel, int nLoopCount);
    int  SetChannelPriority(uint64_t nChannel, int nPriority);
    int  SetChannelPlaybackPosition(uint64_t nChannel, int nPosition);
    int  SetChannelDistances(uint64_t nChannel, float fMaxDistance, float fMinDistance);
    int  SetChannelOcclusion(uint64_t nChannel, float fDirectOcclusion, float fReverbOcclusion);
    int  SetChannel3DPosition(uint64_t nChannel, float x, float y, float z);
    int  SetChannel3DPosition(FModAudioSystemChannelInfo* pInfo, FMOD_VECTOR* pPosition);
    int  GetIsChannelPlaying(uint64_t nChannel);
    int  GetIsChannelStolen(uint64_t nChannel);
    int  GetChannelPlaybackPosition(uint64_t nChannel);
#else
    int  CreateStream(char* pszName, __sFILE* pFile, int nOffset, int nSize,
                      int bSkip, int b3D, int bLoop);
    int  PlayStream(unsigned int nStreamId, int bLoop);
    int  CloseStream(unsigned int nStreamId);
    int  GetStreamLength(unsigned int nStreamId);

    // Every Set* gates on an invalid channel id, then on the value not being the float maximum where
    // the property is a float, and stores the library's result.
    int  PauseChannel(unsigned int nChannel);
    int  ResumeChannel(unsigned int nChannel);
    int  StopChannel(unsigned int nChannel);
    void PauseAllActiveChannels();
    void ResumeAllActiveChannels();
    int  SetChannelVolume(unsigned int nChannel, float fVolume);
    int  SetChannelFrequency(unsigned int nChannel, float fFrequency);
    int  SetChannelPan(unsigned int nChannel, float fPan);
    int  SetChannelLoopCount(unsigned int nChannel, int nLoopCount);
    int  SetChannelPriority(unsigned int nChannel, int nPriority);
    int  SetChannelPlaybackPosition(unsigned int nChannel, int nPosition);
    int  SetChannelDistances(unsigned int nChannel, float fMaxDistance, float fMinDistance);
    int  SetChannelOcclusion(unsigned int nChannel, float fDirectOcclusion, float fReverbOcclusion);
    int  SetChannel3DPosition(unsigned int nChannel, float x, float y, float z);
    int  SetChannel3DPosition(FModAudioSystemChannelInfo* pInfo, FMOD_VECTOR* pPosition);

    int  GetIsChannelPlaying(unsigned int nChannel);
    int  GetIsChannelStolen(unsigned int nChannel);
    int  GetIsChannelDone(unsigned int nChannel);
    int  GetChannelPlaybackPosition(unsigned int nChannel);
#endif

    // Wave metrics for a loaded sound; both take the resource id.
    int  GetSoundLength(int nSoundId);
    int  GetSoundSampleRate(int nSoundId);

    // Rebuilds a recycled channel: replays the sound, re-arms the callback and re-applies every
    // property that is not still FLT_MAX.
    void HandleChannelEnd(FModAudioSystemChannelInfo* pInfo);
    int  RebootStream(FModAudioSystemChannelInfo* pInfo);

public:
    FMOD::System* m_pSystem;
    FMOD_RESULT   m_nLastResult;
    // The three id tables are std::map objects spelled void* so that <map> stays out of this
    // header; the .cpp re-types them through the SOUND_MAP/STREAM_MAP/CHANNEL_MAP macros.
    void*         m_paSounds;
    void*         m_paStreams;
    void*         m_paChannels;
    int           m_nNumChannels;
#if KOTOR1  // K1: 64-bit stream ids
    uint64_t m_nNextStreamId;
    uint64_t _reserved40;
    int           m_bInitialized;
    int           _pad4c;
    int           m_bBackgroundPaused;
#else
    unsigned int  m_nNextStreamId;
    int           _reserved38;
    int           m_bInitialized;
    int           m_bBackgroundPaused;
#endif
};

#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(FModAudioSystem) == 0x58, "K1 FModAudioSystem layout changed");
#else
K2_LAYOUT_ASSERT(sizeof(FModAudioSystem) == 0x48, "FModAudioSystem layout changed");
#endif

#endif // FMODAUDIOSYSTEM_H
