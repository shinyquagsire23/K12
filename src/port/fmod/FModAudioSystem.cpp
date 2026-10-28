
#include "k2stdint.h"
#include "FModAudioSystem.h"
#include <list>
#include <map>
#include <inttypes.h>
#include "exosoundinternal.h"
#include "exosound.h"

#if !KOTOR1 || defined(K2_HOST_BUILD)  // K1's FMOD backend is libandroid_port.so, not libKOTOR.so

extern CExoSound* g_pExoSound;
extern bool       g_bIsInBackground;

#include "port/libcbase.h"
#include "port/libcstdlib.h"

// The decode and stream buffer every stream gets.
enum {
    STREAM_BUFFER_SIZE = 1280 * 1024,
};

#if KOTOR1  // K1 (libandroid_port.so): stream and channel ids are 64-bit
typedef uint64_t FModId;
#else
typedef unsigned int FModId;
#endif

static const FModId kInvalidChannel = static_cast<FModId>(-1);

// The engine's "this property is not set" sentinel for every float channel property. Spelled here
// because <float.h> cannot be included from this TU: its bionic headers declare atoi(), which
// main.cpp also declares with a different exception specification.
static const float kFltMax = __FLT_MAX__;


// The three id tables are stored as void* in the header; these re-type them so a use reads as a
// plain member access.
typedef std::map<int, FModAudioSystemSoundInfo*>            SoundMap;
typedef std::map<unsigned int, FModAudioSystemStreamInfo*>  StreamMap;
typedef std::map<unsigned int, FModAudioSystemChannelInfo*> ChannelMap;

#define SOUND_MAP   (*reinterpret_cast<SoundMap*>(m_paSounds))
#define STREAM_MAP  (*reinterpret_cast<StreamMap*>(m_paStreams))
#define CHANNEL_MAP (*reinterpret_cast<ChannelMap*>(m_paChannels))


FModAudioSystemSoundInfo* FModAudioSystem::GetSoundFromMap(int nSoundId)
{
    std::map<int, FModAudioSystemSoundInfo*>::iterator it = SOUND_MAP.find(nSoundId);
    if (it == SOUND_MAP.end())
        return 0;
    return it->second;
}

FModAudioSystemStreamInfo* FModAudioSystem::GetStreamInfoFromMap(unsigned int nStreamId)
{
    std::map<unsigned int, FModAudioSystemStreamInfo*>::iterator it = STREAM_MAP.find(nStreamId);
    if (it == STREAM_MAP.end())
        return 0;
    return it->second;
}

FModAudioSystemChannelInfo* FModAudioSystem::GetChannelInfoByUID(unsigned int nChannel)
{
    std::map<unsigned int, FModAudioSystemChannelInfo*>::iterator it = CHANNEL_MAP.find(nChannel);
    if (it == CHANNEL_MAP.end())
        return 0;
    return it->second;
}

FModAudioSystemChannelInfo* FModAudioSystem::GetFreeChannel()
{
    for (std::map<unsigned int, FModAudioSystemChannelInfo*>::iterator it = CHANNEL_MAP.begin();
         it != CHANNEL_MAP.end(); ++it)
    {
        if (it->second->m_bInUse == 0)
            return it->second;
    }
    return 0;
}

void FModAudioSystem::ResetChannelInfo(FModAudioSystemChannelInfo* pInfo)
{
    pInfo->Reset();
}


FModAudioSystem::FModAudioSystem()
{
    m_nNumChannels = 0;
    m_nNextStreamId = 0;
#if KOTOR1
    _reserved40 = 0;
#else
    _reserved38 = 0;
#endif
    m_bInitialized = 0;
    m_bBackgroundPaused = 0;
    m_paSounds = new std::map<int, FModAudioSystemSoundInfo*>();
    m_paStreams = new std::map<unsigned int, FModAudioSystemStreamInfo*>();
    m_paChannels = new std::map<unsigned int, FModAudioSystemChannelInfo*>();
}

FModAudioSystem::~FModAudioSystem()
{
    for (std::map<int, FModAudioSystemSoundInfo*>::iterator it = SOUND_MAP.begin();
         it != SOUND_MAP.end();)
    {
        delete it->second;
#ifdef K2_FIX_ENGINE_BUGS
        SOUND_MAP.erase(it++);
#else
        SOUND_MAP.erase(it);
        ++it;   // sic: steps from the node erase() just freed
#endif
    }
    SOUND_MAP.clear();
    delete static_cast<SoundMap*>(m_paSounds);

    for (std::map<unsigned int, FModAudioSystemStreamInfo*>::iterator it = STREAM_MAP.begin();
         it != STREAM_MAP.end();)
    {
        delete it->second;
#ifdef K2_FIX_ENGINE_BUGS
        STREAM_MAP.erase(it++);
#else
        STREAM_MAP.erase(it);
        ++it;   // sic: steps from the node erase() just freed
#endif
    }
    STREAM_MAP.clear();
    delete static_cast<StreamMap*>(m_paStreams);

    for (std::map<unsigned int, FModAudioSystemChannelInfo*>::iterator it = CHANNEL_MAP.begin();
         it != CHANNEL_MAP.end();)
    {
        delete it->second;
#ifdef K2_FIX_ENGINE_BUGS
        CHANNEL_MAP.erase(it++);
#else
        CHANNEL_MAP.erase(it);
        ++it;   // sic: steps from the node erase() just freed
#endif
    }
    CHANNEL_MAP.clear();
    delete static_cast<ChannelMap*>(m_paChannels);
}

int FModAudioSystem::CheckFModResult(FMOD_RESULT result)
{
    return result == FMOD_OK;
}

FMOD_RESULT FModAudioSystem::SystemCloseCallback(void* pHandle, void* pUserData)
{
    K2_UNUSED(pHandle); K2_UNUSED(pUserData);
    return FMOD_OK;
}

FMOD_RESULT FModAudioSystem::SystemReadCallback(void* pHandle, void* pBuffer, unsigned int nSize,
                                                unsigned int* pnRead, void* pUserData)
{
    K2_UNUSED(pUserData);
#if KOTOR1
    unsigned int nRead = static_cast<unsigned int>(SDL_RWread(static_cast<SDL_RWops*>(pHandle), pBuffer, 1, nSize));
#else
    unsigned int nRead = static_cast<unsigned int>(fread(pBuffer, 1, nSize, static_cast<__sFILE*>(pHandle)));
#endif
    *pnRead = nRead;
    return (nRead < nSize) ? FMOD_ERR_FILE_EOF : FMOD_OK;
}

// The third parameter is typed void* but the body reads an int member out of it, i.e. the author
// expected the library to hand back the channel info whose offset field lives there. The userdata
// out-parameter is never stored through, so in practice the library passes null and this guard is
// what keeps the seek from happening.
FMOD_RESULT FModAudioSystem::SystemSeekCallback(void* pHandle, unsigned int nPos, void* pUserData)
{
    if (pUserData != 0)
#if KOTOR1
        SDL_RWseek(static_cast<SDL_RWops*>(pHandle), static_cast<int64_t>(nPos + (static_cast<FModAudioSystemStreamInfo*>(pUserData))->m_nOffset),
                   RW_SEEK_SET);
#else
        fseek(static_cast<__sFILE*>(pHandle), static_cast<long>(nPos + static_cast<FModAudioSystemStreamInfo*>(pUserData)->m_nOffset), 0);
#endif
    return FMOD_OK;
}

// The stream name is the stream id, so it parses back with strtoul and hands the library the open
// stream pointer the table holds. The size out-parameter is never written.
FMOD_RESULT FModAudioSystem::SystemOpenCallback(const char* pszName, unsigned int* pnSize,
                                                void** ppHandle, void* pUserData)
{
    K2_UNUSED(pnSize); K2_UNUSED(pUserData);
    unsigned int nStreamId = static_cast<unsigned int>(strtoul(pszName, 0, 0));
    FModAudioSystemStreamInfo* pInfo = g_pExoSound->m_pInternal->m_audioSystem.GetStreamInfoFromMap(nStreamId);
    if (pInfo == 0)
        return FMOD_ERR_FILE_NOTFOUND;
    *ppHandle = pInfo->m_pFile;
    return FMOD_OK;
}

// The callback receives the channel control, so this recovers the owning channel info from the user
// data armed at play time and forwards only the end notification.
FMOD_RESULT FModAudioSystem::ChannelCallback(FMOD_CHANNELCONTROL* pControl,
                                             FMOD_CHANNELCONTROL_TYPE nType,
                                             FMOD_CHANNELCONTROL_CALLBACK_TYPE nCallbackType,
                                             void* pCommandA, void* pCommandB)
{
    K2_UNUSED(nType); K2_UNUSED(pCommandA); K2_UNUSED(pCommandB);
    if (pControl != 0)
    {
        void* pUserData = 0;
        pControl->getUserData(&pUserData);
        if (nCallbackType == FMOD_CHANNELCONTROL_CALLBACK_END && pUserData != 0)
            g_pExoSound->m_pInternal->m_audioSystem.HandleChannelEnd(static_cast<FModAudioSystemChannelInfo*>(pUserData));
    }
    return FMOD_OK;
}


// Every call result is stored into m_nLastResult here and none of them is tested: a failed system
// create still falls through to the driver query. The voice-handle count is padded by 5 and the
// padded value is what m_nNumChannels records.
void FModAudioSystem::InitSystem(int nVoiceHandles)
{
    m_nLastResult = FMOD_System_Create(&m_pSystem);

    int nNumDrivers;
    m_nLastResult = m_pSystem->getNumDrivers(&nNumDrivers);
    m_nLastResult = m_pSystem->setOutput(nNumDrivers == 0 ? FMOD_OUTPUTTYPE_NOSOUND : FMOD_OUTPUTTYPE_AUDIOTRACK);
    m_nLastResult = m_pSystem->setFileSystem(SystemOpenCallback, SystemCloseCallback,
                                             SystemReadCallback, SystemSeekCallback, 0, 0, -1);

    nVoiceHandles += 5;
    m_nLastResult = m_pSystem->init(nVoiceHandles, 0, 0);
    if (m_nLastResult == FMOD_OK)
    {
        m_nNumChannels = nVoiceHandles;
        m_bInitialized = InitChannels();
    }
}

int FModAudioSystem::InitChannels()
{
    for (int i = 0; i < m_nNumChannels; i++)
    {
        FModAudioSystemChannelInfo* pInfo = new FModAudioSystemChannelInfo();
        FMOD::Channel* pChannel = 0;
        m_nLastResult = m_pSystem->getChannel(i, &pChannel);
        if (m_nLastResult != FMOD_OK)
        {
            pInfo->~FModAudioSystemChannelInfo();
            operator delete(pInfo);
            return 0;
        }
        pInfo->m_nUID = i;
        pInfo->_reserved08 = i;
        pInfo->m_pChannel = pChannel;
        if (!CHANNEL_MAP.insert(std::make_pair(static_cast<unsigned int>(i), pInfo)).second)
        {
            pInfo->~FModAudioSystemChannelInfo();
            operator delete(pInfo);
        }
    }
    return 1;
}

void FModAudioSystem::ShutdownSystem()
{
    m_bInitialized = 0;
}

void FModAudioSystem::UpdateSystem()
{
    if (m_bInitialized != 0)
    {
        m_pSystem->update();
        CheckAppBackgroundStatus();
    }
}

void FModAudioSystem::CheckAppBackgroundStatus()
{
    if (g_bIsInBackground)
    {
        if (m_bBackgroundPaused != 0)
            return;
        for (std::map<unsigned int, FModAudioSystemChannelInfo*>::iterator it = CHANNEL_MAP.begin();
             it != CHANNEL_MAP.end(); ++it)
        {
            it->second->m_pChannel->setPaused(true);
        }
        m_bBackgroundPaused = 1;
    }
    else
    {
        if (m_bBackgroundPaused == 0)
            return;
        for (std::map<unsigned int, FModAudioSystemChannelInfo*>::iterator it = CHANNEL_MAP.begin();
             it != CHANNEL_MAP.end(); ++it)
        {
            FModAudioSystemChannelInfo* pInfo = it->second;
            if (pInfo->m_bInUse != 0 && pInfo->m_bPaused == 0 && pInfo->m_bDone == 0)
                pInfo->m_pChannel->setPaused(false);
        }
        m_bBackgroundPaused = 0;
    }
}


// Unlike every other channel row this one does not test the lookup result: the lower-bound node's
// value is read with neither an end() test nor a key-equality test (sic; ResumeChannel likewise).
int FModAudioSystem::PauseChannel(FModId nChannel)
{
    if (nChannel == kInvalidChannel)
        return 0;
#ifdef K2_FIX_ENGINE_BUGS
    FModAudioSystemChannelInfo* pInfo = GetChannelInfoByUID(static_cast<unsigned int>(nChannel));
    if (pInfo == 0)
        return 0;
#else
    FModAudioSystemChannelInfo* pInfo = CHANNEL_MAP.lower_bound(static_cast<unsigned int>(nChannel))->second;
#endif
    pInfo->m_bPaused = 1;
    m_nLastResult = pInfo->m_pChannel->setPaused(true);
    return m_nLastResult == FMOD_OK;
}

// Resuming clears m_bPaused and m_bDone together, while PauseChannel writes m_bPaused alone. The
// asymmetry is deliberate: a channel resumed after its end callback must not stay marked done.
int FModAudioSystem::ResumeChannel(FModId nChannel)
{
    if (nChannel == kInvalidChannel)
        return 0;
#ifdef K2_FIX_ENGINE_BUGS
    FModAudioSystemChannelInfo* pInfo = GetChannelInfoByUID(static_cast<unsigned int>(nChannel));
    if (pInfo == 0)
        return 0;
#else
    FModAudioSystemChannelInfo* pInfo = CHANNEL_MAP.lower_bound(static_cast<unsigned int>(nChannel))->second;
#endif
    pInfo->m_bPaused = 0;
    pInfo->m_bDone = 0;
    m_nLastResult = pInfo->m_pChannel->setPaused(false);
#if KOTOR1  // K1 restarts a channel that has already played out
    if (m_nLastResult != FMOD_OK && RebootStream(pInfo))
        m_nLastResult = pInfo->m_pChannel->setPaused(false);
#endif
    return m_nLastResult == FMOD_OK;
}

int FModAudioSystem::SetChannelVolume(FModId nChannel, float fVolume)
{
    if (nChannel == kInvalidChannel)
        return 0;
    if (fVolume == kFltMax)
        return 0;
    FModAudioSystemChannelInfo* pInfo = GetChannelInfoByUID(static_cast<unsigned int>(nChannel));
    if (pInfo == 0)
        return 0;
    m_nLastResult = pInfo->m_pChannel->setVolume(fVolume);
    if (m_nLastResult != FMOD_OK)
        return 0;
    pInfo->m_fVolume = fVolume;
    return 1;
}

int FModAudioSystem::SetChannelFrequency(FModId nChannel, float fFrequency)
{
    if (nChannel == kInvalidChannel)
        return 0;
    if (fFrequency == kFltMax)
        return 0;
    FModAudioSystemChannelInfo* pInfo = GetChannelInfoByUID(static_cast<unsigned int>(nChannel));
    if (pInfo == 0)
        return 0;
    m_nLastResult = pInfo->m_pChannel->setFrequency(fFrequency);
    if (m_nLastResult != FMOD_OK)
        return 0;
    pInfo->m_fFrequency = fFrequency;
    return 1;
}

int FModAudioSystem::SetChannelPan(FModId nChannel, float fPan)
{
    if (nChannel == kInvalidChannel)
        return 0;
    if (fPan == kFltMax)
        return 0;
    FModAudioSystemChannelInfo* pInfo = GetChannelInfoByUID(static_cast<unsigned int>(nChannel));
    if (pInfo == 0)
        return 0;
    m_nLastResult = pInfo->m_pChannel->setPan(fPan);
    if (m_nLastResult != FMOD_OK)
        return 0;
    pInfo->m_fPan = fPan;
    return 1;
}

// The two floats are the (min, max) pair in reverse of the parameter order, so parameter 2 goes to
// the minimum and parameter 1 to the maximum.
int FModAudioSystem::SetChannelDistances(FModId nChannel, float fMaxDistance, float fMinDistance)
{
    if (nChannel == kInvalidChannel)
        return 0;
    if (fMaxDistance == kFltMax)
        return 0;
    if (fMinDistance == kFltMax)
        return 0;
    FModAudioSystemChannelInfo* pInfo = GetChannelInfoByUID(static_cast<unsigned int>(nChannel));
    if (pInfo == 0)
        return 0;
    m_nLastResult = pInfo->m_pChannel->set3DMinMaxDistance(fMinDistance, fMaxDistance);
    if (m_nLastResult != FMOD_OK)
        return 0;
    pInfo->m_fMinDistance = fMinDistance;
    pInfo->m_fMaxDistance = fMaxDistance;
    return 1;
}

int FModAudioSystem::SetChannelOcclusion(FModId nChannel, float fDirectOcclusion, float fReverbOcclusion)
{
    if (nChannel == kInvalidChannel)
        return 0;
    if (fDirectOcclusion == kFltMax)
        return 0;
    if (fReverbOcclusion == kFltMax)
        return 0;
    FModAudioSystemChannelInfo* pInfo = GetChannelInfoByUID(static_cast<unsigned int>(nChannel));
    if (pInfo == 0)
        return 0;
    m_nLastResult = pInfo->m_pChannel->set3DOcclusion(fDirectOcclusion, fReverbOcclusion);
    if (m_nLastResult != FMOD_OK)
        return 0;
    pInfo->m_fDirectOcclusion = fDirectOcclusion;
    pInfo->m_fReverbOcclusion = fReverbOcclusion;
    return 1;
}

// The position is kept as a heap FMOD_VECTOR owned by the channel info, which is what Reset frees,
// so this allocates even on the success path.
int FModAudioSystem::SetChannel3DPosition(FModId nChannel, float x, float y, float z)
{
    if (nChannel == kInvalidChannel)
        return 0;
    FModAudioSystemChannelInfo* pInfo = GetChannelInfoByUID(static_cast<unsigned int>(nChannel));
    if (pInfo == 0)
        return 0;
    FMOD_VECTOR* pPosition = new FMOD_VECTOR;
    pPosition->x = x;
    pPosition->y = y;
    pPosition->z = z;
    m_nLastResult = pInfo->m_pChannel->set3DAttributes(pPosition, 0);
    if (m_nLastResult == FMOD_OK)
    {
        pInfo->SetPosition(pPosition);
        return 1;
    }
    operator delete(pPosition);
    return 0;
}

int FModAudioSystem::SetChannel3DPosition(FModAudioSystemChannelInfo* pInfo, FMOD_VECTOR* pPosition)
{
    if (pInfo == 0)
        return 0;
    if (pPosition == 0)
        return 0;
    m_nLastResult = pInfo->m_pChannel->set3DAttributes(pPosition, 0);
    if (m_nLastResult != FMOD_OK)
        return 0;
    pInfo->SetPosition(pPosition);
    return 1;
}

// The stored flag is the inverse of the request. m_bNoLoop is set when no looping was asked for,
// and the reboot branch runs when it is clear.
int FModAudioSystem::SetChannelLoopCount(FModId nChannel, int nLoopCount)
{
    if (nChannel == kInvalidChannel)
        return 0;
    FModAudioSystemChannelInfo* pInfo = GetChannelInfoByUID(static_cast<unsigned int>(nChannel));
    if (pInfo == 0)
        return 0;
    if (nLoopCount == 0)
    {
        pInfo->m_bNoLoop = 1;
        return 1;
    }
    pInfo->m_bNoLoop = 0;
    return 1;
}

int FModAudioSystem::SetChannelPriority(FModId nChannel, int nPriority)
{
    if (nChannel == kInvalidChannel)
        return 0;
    if (nPriority == -1)
        return 0;
    FModAudioSystemChannelInfo* pInfo = GetChannelInfoByUID(static_cast<unsigned int>(nChannel));
    if (pInfo == 0)
        return 0;
    m_nLastResult = pInfo->m_pChannel->setPriority(nPriority);
    if (m_nLastResult != FMOD_OK)
        return 0;
    pInfo->m_nPriority = nPriority;
    return 1;
}

int FModAudioSystem::SetChannelPlaybackPosition(FModId nChannel, int nPosition)
{
    if (nChannel == kInvalidChannel)
        return 0;
    FModAudioSystemChannelInfo* pInfo = GetChannelInfoByUID(static_cast<unsigned int>(nChannel));
    if (pInfo == 0)
        return 0;
    m_nLastResult = pInfo->m_pChannel->setPosition(static_cast<unsigned int>(nPosition), 1);
    if (m_nLastResult != FMOD_OK)
        return 0;
    return 1;
}

int FModAudioSystem::GetChannelPlaybackPosition(FModId nChannel)
{
    if (nChannel == kInvalidChannel)
        return 0;
    FModAudioSystemChannelInfo* pInfo = GetChannelInfoByUID(static_cast<unsigned int>(nChannel));
    if (pInfo == 0)
        return 0;
    unsigned int nPosition;
    m_nLastResult = pInfo->m_pChannel->getPosition(&nPosition, 1);
    return (m_nLastResult == FMOD_OK) ? static_cast<int>(nPosition) : -1;
}

int FModAudioSystem::StopChannel(FModId nChannel)
{
    if (nChannel == kInvalidChannel)
        return 0;
    FModAudioSystemChannelInfo* pInfo = GetChannelInfoByUID(static_cast<unsigned int>(nChannel));
    if (pInfo == 0)
        return 0;
    pInfo->m_bStopped = 1;
    m_nLastResult = pInfo->m_pChannel->stop();
    pInfo->Reset();
    return m_nLastResult == FMOD_OK;
}

#if !KOTOR1  // not in K1's port library
int FModAudioSystem::GetIsChannelDone(FModId nChannel)
{
    if (nChannel == kInvalidChannel)
        return 0;
    FModAudioSystemChannelInfo* pInfo = GetChannelInfoByUID(nChannel);
    if (pInfo == 0)
        return 0;
    return pInfo->m_bDone;
}
#endif

// Two result codes mean "not ours any more", a stolen channel and an invalid handle; both are
// treated as not playing rather than as errors.
int FModAudioSystem::GetIsChannelPlaying(FModId nChannel)
{
    if (nChannel == kInvalidChannel)
        return 0;
    FModAudioSystemChannelInfo* pInfo = GetChannelInfoByUID(static_cast<unsigned int>(nChannel));
    if (pInfo == 0)
        return 0;
    bool bIsPlaying = false;
    m_nLastResult = pInfo->m_pChannel->isPlaying(&bIsPlaying);
    if (m_nLastResult == FMOD_ERR_CHANNEL_STOLEN || m_nLastResult == FMOD_ERR_INVALID_HANDLE)
        return 0;
    if (pInfo->m_bInUse == 0)
        return 0;
    if (pInfo->m_bPaused != 0)
        return 0;
    return pInfo->m_bDone == 0;
}

int FModAudioSystem::GetIsChannelStolen(FModId nChannel)
{
    FModAudioSystemChannelInfo* pInfo = GetChannelInfoByUID(static_cast<unsigned int>(nChannel));
    if (pInfo == 0)
        return 0;
    bool bIsPlaying = false;
    m_nLastResult = pInfo->m_pChannel->isPlaying(&bIsPlaying);
    return m_nLastResult == FMOD_ERR_CHANNEL_STOLEN;
}

void FModAudioSystem::PauseAllActiveChannels()
{
    for (std::map<unsigned int, FModAudioSystemChannelInfo*>::iterator it = CHANNEL_MAP.begin();
         it != CHANNEL_MAP.end(); ++it)
    {
        it->second->m_pChannel->setPaused(true);
    }
}

void FModAudioSystem::ResumeAllActiveChannels()
{
    for (std::map<unsigned int, FModAudioSystemChannelInfo*>::iterator it = CHANNEL_MAP.begin();
         it != CHANNEL_MAP.end(); ++it)
    {
        FModAudioSystemChannelInfo* pInfo = it->second;
        if (pInfo->m_bInUse != 0 && pInfo->m_bPaused == 0 && pInfo->m_bDone == 0)
            pInfo->m_pChannel->setPaused(false);
    }
}

// Two getLength calls on the same sound, because the millisecond query has to succeed before the
// sample query runs; the sample rate is milliseconds * 1000 / samples.

int FModAudioSystem::GetSoundLength(int nSoundId)
{
    FModAudioSystemSoundInfo* pInfo = GetSoundFromMap(nSoundId);
    if (pInfo == 0)
        return 0;
    unsigned int nLength;
    m_nLastResult = pInfo->m_pSound->getLength(&nLength, 1);
    return (m_nLastResult == FMOD_OK) ? static_cast<int>(nLength) : 0;
}

// Two getLength calls on the same sound, because the millisecond query has to succeed before the
// sample query runs; the sample rate is milliseconds * 1000 / samples.
int FModAudioSystem::GetSoundSampleRate(int nSoundId)
{
    FModAudioSystemSoundInfo* pInfo = GetSoundFromMap(nSoundId);
    if (pInfo == 0)
        return 0;
    unsigned int nLengthMs;
    m_nLastResult = pInfo->m_pSound->getLength(&nLengthMs, 1);
    if (m_nLastResult != FMOD_OK)
        return 0;
    unsigned int nLengthSamples;
    m_nLastResult = pInfo->m_pSound->getLength(&nLengthSamples, 2);
    if (m_nLastResult != FMOD_OK)
        return 0;
    return static_cast<int>((nLengthSamples * 1000) / nLengthMs);
}

int FModAudioSystem::GetStreamLength(FModId nStreamId)
{
    FModAudioSystemStreamInfo* pInfo = GetStreamInfoFromMap(static_cast<unsigned int>(nStreamId));
    if (pInfo == 0)
        return 0;
    unsigned int nLength;
    m_nLastResult = pInfo->m_pSound->getLength(&nLength, 1);
    return (m_nLastResult == FMOD_OK) ? static_cast<int>(nLength) : 0;
}


int FModAudioSystem::Set3DListenerPosition(float x, float y, float z)
{
    if (m_bInitialized == 0)
        return 0;
    FMOD_VECTOR vPosition = { x, y, z };
    m_nLastResult = m_pSystem->set3DListenerAttributes(0, &vPosition, 0, 0, 0);
    return m_nLastResult == FMOD_OK;
}

int FModAudioSystem::Set3DListenerOrientation(float dx, float dy, float dz,
                                              float ux, float uy, float uz)
{
    if (m_bInitialized == 0)
        return 0;
    FMOD_VECTOR vForward = { dx, dy, dz };
    FMOD_VECTOR vUp = { ux, uy, uz };
    m_nLastResult = m_pSystem->set3DListenerAttributes(0, 0, 0, &vForward, &vUp);
    return m_nLastResult == FMOD_OK;
}


// Drops a reference. The erase uses the iterator the lookup produced, going straight from the found
// node into the tree removal with no second walk by key.
int FModAudioSystem::ReleaseSound(int nSoundId)
{
    std::map<int, FModAudioSystemSoundInfo*>::iterator it = SOUND_MAP.find(nSoundId);
    if (it == SOUND_MAP.end())
        return 0;
    FModAudioSystemSoundInfo* pInfo = it->second;
    if (pInfo->m_nRefCount >= 2)
    {
        pInfo->m_nRefCount--;
        return 0;
    }
    if (pInfo->m_nRefCount != 1)
        return 0;
    if (ReleaseSound(pInfo->m_pSound) == 0)
        return 0;
    SOUND_MAP.erase(it);
    delete pInfo;
    return 1;
}

// Drops a reference to a sound and, at zero, frees it. Every channel still pointing at it is
// stopped first, with the matching channel ids collected into a list before anything is stopped.
// The trailing check for -1 is real, because the list can legitimately hold it.
int FModAudioSystem::ReleaseSound(FMOD::Sound* pSound)
{
    std::list<unsigned int> aChannels;
    for (std::map<unsigned int, FModAudioSystemChannelInfo*>::iterator it = CHANNEL_MAP.begin();
         it != CHANNEL_MAP.end(); it++)
    {
        if (it->second->m_pSound == pSound)
            aChannels.push_back(it->first);
    }
    for (std::list<unsigned int>::iterator it = aChannels.begin(); it != aChannels.end(); it++)
    {
        unsigned int nChannel = *it;
        if (nChannel == static_cast<unsigned int>(kInvalidChannel))
            continue;
        StopChannel(nChannel);
    }
    m_nLastResult = pSound->release();
    return m_nLastResult == FMOD_OK;
}

// Creates a sound from an in-memory buffer. The buffer is the third parameter and it carries the
// audio data, not a name: the mode includes FMOD_OPENMEMORY, the library's convention for "the name
// argument is really a memory buffer", so the first parameter is dead in this TU. An id already in
// the table is a refcount bump, not a second sound.
int FModAudioSystem::CreateSound(char* pszName, int nSoundId, void* pData, unsigned int nLength, int b3D, int bFlags)
{
    K2_UNUSED(pszName); K2_UNUSED(bFlags);
    SoundMap::iterator it = SOUND_MAP.find(nSoundId);
    if (it != SOUND_MAP.end())
    {
        it->second->m_nRefCount++;
        return 1;
    }

    FMOD::Sound* pSound = 0;
    FMOD_CREATESOUNDEXINFO exinfo;
    memset(&exinfo, 0, sizeof(exinfo));
    exinfo.cbsize = sizeof(exinfo);
    exinfo.length = nLength;
    exinfo.suggestedsoundtype = FMOD_SOUND_TYPE_WAV;

    m_nLastResult = m_pSystem->createSound(static_cast<const char*>(pData),
                                           FMOD_LOWMEM | FMOD_OPENMEMORY | FMOD_LOOP_OFF
                                               | (b3D != 0 ? FMOD_3D : FMOD_2D),
                                           &exinfo, &pSound);
    if (m_nLastResult != FMOD_OK)
        return 0;

    FModAudioSystemSoundInfo* pInfo = new FModAudioSystemSoundInfo();
    pInfo->m_nRefCount = 1;
    pInfo->m_pSound = pSound;
    if (!SOUND_MAP.insert(std::make_pair(nSoundId, pInfo)).second)
    {
        pInfo->~FModAudioSystemSoundInfo();
        operator delete(pInfo);
    }
    return 1;
}

// Returns the channel uid, or -1 when nothing could be played. The uid, not the index, is what
// comes back, and it is the same value armed as the user data.
#if KOTOR1
uint64_t FModAudioSystem::PlaySound(int nSoundId)
#else
int FModAudioSystem::PlaySoundA(int nSoundId)
#endif
{
    FModAudioSystemSoundInfo* pInfo = GetSoundFromMap(nSoundId);
    if (pInfo == 0)
        return kInvalidChannel;
    FMOD::Channel* pFmodChannel = 0;
    FModAudioSystemChannelInfo* pChannel = GetFreeChannel();
    if (pChannel == 0)
        return kInvalidChannel;

    m_nLastResult = m_pSystem->playSound(pInfo->m_pSound, 0, false, &pFmodChannel);
    if (m_nLastResult != FMOD_OK)
        return kInvalidChannel;

    pChannel->Reset();
    pChannel->m_pSound = pInfo->m_pSound;
    pChannel->m_pChannel = pFmodChannel;
    pChannel->m_bInUse = 1;
    pFmodChannel->setUserData(pChannel);
    pFmodChannel->setCallback(ChannelCallback);
    return pChannel->m_nUID;
}


static int FModFormatStreamName(char* pszBuf, uint64_t nDstSize, uint64_t nSize,
                                uint64_t nFlag, unsigned int nStreamId)
{
    K2_UNUSED(nDstSize); K2_UNUSED(nFlag);
    return snprintf(pszBuf, nSize, "%" PRIu64, static_cast<uint64_t>(nStreamId));
}

#if KOTOR1
uint64_t FModAudioSystem::CreateStream(char* pszName, SDL_RWops* pFile, int nOffset, int nSize,
                                  int bSkip, int b3D, int bLoop)
#else
int FModAudioSystem::CreateStream(char* pszName, __sFILE* pFile, int nOffset, int nSize,
                                  int bSkip, int b3D, int bLoop)
#endif
{
    K2_UNUSED(pszName);
#if KOTOR1
    K2_UNUSED(b3D);
#endif
    FMOD_CREATESOUNDEXINFO exinfo;
    memset(&exinfo, 0, sizeof(exinfo));
    exinfo.cbsize = sizeof(exinfo);
    exinfo.length = nSize;
    exinfo.decodebuffersize = STREAM_BUFFER_SIZE;
    exinfo.suggestedsoundtype = (bSkip == 0) ? FMOD_SOUND_TYPE_WAV : FMOD_SOUND_TYPE_MPEG;
    exinfo.filebuffersize = nSize;
    FMOD::Sound* pSound = 0;

    FModAudioSystemStreamInfo* pInfo = new FModAudioSystemStreamInfo();

    unsigned int nStreamId = static_cast<unsigned int>(++m_nNextStreamId);
    if (nStreamId == 0)
    {
        nStreamId = 1;
        m_nNextStreamId = 1;
    }
    pInfo->m_nStreamId = nStreamId;
    pInfo->m_pFile = pFile;
    pInfo->m_nOffset = nOffset;
    pInfo->m_nSize = nSize;
    if (!STREAM_MAP.insert(std::make_pair(nStreamId, pInfo)).second)
    {
        pInfo->~FModAudioSystemStreamInfo();
        operator delete(pInfo);
        return 0;
    }

    int nLen = FModFormatStreamName(0, static_cast<uint64_t>(-1), 0, 0, static_cast<unsigned int>(m_nNextStreamId));
#ifdef K2_HOST_BUILD
    char szName[21];  // standard C++ has no VLA; a uint64_t is at most 20 digits
#else
    char szName[nLen + 1];
#endif
    FModFormatStreamName(szName, static_cast<uint64_t>(-1), nLen + 1, 0, static_cast<unsigned int>(m_nNextStreamId));

    m_pSystem->setStreamBufferSize(STREAM_BUFFER_SIZE, FMOD_TIMEUNIT_RAWBYTES);

    unsigned int nMode = FMOD_CREATESTREAM | (bLoop != 0 ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
#if KOTOR1 && defined(K2_FIX_ENGINE_BUGS)
    // K1 places its streams itself and never positions one.
    nMode |= FMOD_LOWMEM | FMOD_2D;
#else
    nMode |= FMOD_LOWMEM | (b3D == 0 ? FMOD_2D : FMOD_3D);
#endif

    m_nLastResult = m_pSystem->createSound(szName, nMode, &exinfo, &pSound);
    if (m_nLastResult != FMOD_OK)
        return 0;

    pInfo->m_pSound = pSound;
    pSound->setUserData(szName);
    GetStreamInfoFromMap(static_cast<unsigned int>(m_nNextStreamId))->m_pSound = pSound;
    return static_cast<int>(m_nNextStreamId);
}

// The loop flag is passed as the paused argument, not as the looping flag.
#if KOTOR1
uint64_t FModAudioSystem::PlayStream(FModId nStreamId, int bLoop)
#else
int FModAudioSystem::PlayStream(FModId nStreamId, int bLoop)
#endif
{
    FModAudioSystemStreamInfo* pStream = GetStreamInfoFromMap(static_cast<unsigned int>(nStreamId));
    if (pStream == 0)
        return kInvalidChannel;
    FModAudioSystemChannelInfo* pChannel = GetFreeChannel();
    if (pChannel == 0)
        return kInvalidChannel;

    FMOD::Channel* pFmodChannel = 0;
    m_nLastResult = m_pSystem->playSound(pStream->m_pSound, 0, bLoop != 0, &pFmodChannel);
    if (m_nLastResult != FMOD_OK)
        return kInvalidChannel;

    pChannel->Reset();
    pChannel->m_bIsStream = 1;
    pChannel->m_bInUse = 1;
    pChannel->m_pChannel = pFmodChannel;
    pChannel->m_pSound = pStream->m_pSound;
    pFmodChannel->setUserData(pChannel);
    pFmodChannel->setCallback(ChannelCallback);
    return pChannel->m_nUID;
}

int FModAudioSystem::CloseStream(FModId nStreamId)
{
    std::map<unsigned int, FModAudioSystemStreamInfo*>::iterator it = STREAM_MAP.find(static_cast<unsigned int>(nStreamId));
    if (it == STREAM_MAP.end())
        return 0;
    FModAudioSystemStreamInfo* pStream = it->second;
    if (pStream != 0)
    {
        // The loop stops the first channel playing this stream and then leaves; StopChannel's own
        // result is discarded.
        for (std::map<unsigned int, FModAudioSystemChannelInfo*>::iterator itCh = CHANNEL_MAP.begin();
             itCh != CHANNEL_MAP.end(); ++itCh)
        {
            if (itCh->second->m_pSound != pStream->m_pSound)
                continue;
            StopChannel(itCh->first);
            break;
        }
        m_nLastResult = pStream->m_pSound->release();
        pStream->m_pFile = 0;
        pStream->m_pSound = 0;
    }
    STREAM_MAP.erase(it);
    delete pStream;
    return 1;
}


// Called through ChannelCallback when a channel finishes. A looping channel is rebuilt in place; a
// one-shot channel is replayed only when it was neither explicitly stopped nor a stream, and is
// otherwise just marked done. The property re-application looks the channel up again, once per
// property, on purpose.
void FModAudioSystem::HandleChannelEnd(FModAudioSystemChannelInfo* pInfo)
{
    if (pInfo->m_bNoLoop == 0)
    {
        if (pInfo->m_bStopped == 0 && pInfo->m_bIsStream != 0)
            RebootStream(pInfo);
        pInfo->m_bDone = 1;
        return;
    }

    FMOD::Channel* pFmodChannel = 0;
    m_nLastResult = m_pSystem->playSound(pInfo->m_pSound, 0, true, &pFmodChannel);
    if (m_nLastResult != FMOD_OK)
        return;

    pInfo->m_pChannel = pFmodChannel;
    pFmodChannel->setUserData(pInfo);
    pFmodChannel->setCallback(ChannelCallback);

    if (static_cast<FModId>(pInfo->m_nUID) != kInvalidChannel)
    {
        int nPriority = pInfo->m_nPriority;
        if (nPriority != -1)
        {
            FModAudioSystemChannelInfo* pTarget = GetChannelInfoByUID(pInfo->m_nUID);
            if (pTarget != 0)
            {
                m_nLastResult = pTarget->m_pChannel->setPriority(nPriority);
                if (m_nLastResult == FMOD_OK)
                    pTarget->m_nPriority = nPriority;
            }
        }
    }

    if (static_cast<FModId>(pInfo->m_nUID) != kInvalidChannel && pInfo->m_fFrequency != kFltMax)
    {
        float fFrequency = pInfo->m_fFrequency;
        FModAudioSystemChannelInfo* pTarget = GetChannelInfoByUID(pInfo->m_nUID);
        if (pTarget != 0)
        {
            m_nLastResult = pTarget->m_pChannel->setFrequency(fFrequency);
            if (m_nLastResult == FMOD_OK)
                pTarget->m_fFrequency = fFrequency;
        }
    }

    if (static_cast<FModId>(pInfo->m_nUID) != kInvalidChannel && pInfo->m_fVolume != kFltMax)
    {
        float fVolume = pInfo->m_fVolume;
        FModAudioSystemChannelInfo* pTarget = GetChannelInfoByUID(pInfo->m_nUID);
        if (pTarget != 0)
        {
            m_nLastResult = pTarget->m_pChannel->setVolume(fVolume);
            if (m_nLastResult == FMOD_OK)
                pTarget->m_fVolume = fVolume;
        }
    }

    if (static_cast<FModId>(pInfo->m_nUID) != kInvalidChannel && pInfo->m_fMaxDistance != kFltMax
        && pInfo->m_fMinDistance != kFltMax)
    {
        float fMaxDistance = pInfo->m_fMaxDistance;
        float fMinDistance = pInfo->m_fMinDistance;
        FModAudioSystemChannelInfo* pTarget = GetChannelInfoByUID(pInfo->m_nUID);
        if (pTarget != 0)
        {
            m_nLastResult = pTarget->m_pChannel->set3DMinMaxDistance(fMinDistance,
                                                                     fMaxDistance);
            if (m_nLastResult == FMOD_OK)
            {
                pTarget->m_fMinDistance = fMinDistance;
                pTarget->m_fMaxDistance = fMaxDistance;
            }
        }
    }

    if (static_cast<FModId>(pInfo->m_nUID) != kInvalidChannel && pInfo->m_fDirectOcclusion != kFltMax
        && pInfo->m_fReverbOcclusion != kFltMax)
    {
        float fDirectOcclusion = pInfo->m_fDirectOcclusion;
        float fReverbOcclusion = pInfo->m_fReverbOcclusion;
        FModAudioSystemChannelInfo* pTarget = GetChannelInfoByUID(pInfo->m_nUID);
        if (pTarget != 0)
        {
            m_nLastResult = pTarget->m_pChannel->set3DOcclusion(fDirectOcclusion,
                                                                fReverbOcclusion);
            if (m_nLastResult == FMOD_OK)
            {
                pTarget->m_fDirectOcclusion = fDirectOcclusion;
                pTarget->m_fReverbOcclusion = fReverbOcclusion;
            }
        }
    }

    if (static_cast<FModId>(pInfo->m_nUID) != kInvalidChannel && pInfo->m_fPan != kFltMax)
    {
        float fPan = pInfo->m_fPan;
        FModAudioSystemChannelInfo* pTarget = GetChannelInfoByUID(pInfo->m_nUID);
        if (pTarget != 0)
        {
            m_nLastResult = pTarget->m_pChannel->setPan(fPan);
            if (m_nLastResult == FMOD_OK)
                pTarget->m_fPan = fPan;
        }
    }

    if (pInfo != 0 && pInfo->m_pVec3DPosition != 0)
    {
        FMOD_VECTOR* pPosition = pInfo->m_pVec3DPosition;
        m_nLastResult = pInfo->m_pChannel->set3DAttributes(pPosition, 0);
        if (m_nLastResult == FMOD_OK)
        {
            pInfo->SetPosition(pPosition);
        }
    }

    K2_ASSUME(pInfo != 0);   // ChannelCallback, the only caller, always passes a channel
    ResumeChannel(pInfo->m_nUID);
}

int FModAudioSystem::RebootStream(FModAudioSystemChannelInfo* pInfo)
{
    FMOD::Channel* pFmodChannel = 0;
    m_nLastResult = m_pSystem->playSound(pInfo->m_pSound, 0, true, &pFmodChannel);
    if (m_nLastResult != FMOD_OK)
        return 0;

    pInfo->m_pChannel = pFmodChannel;
    pFmodChannel->setUserData(pInfo);
    pFmodChannel->setCallback(ChannelCallback);

    if (static_cast<FModId>(pInfo->m_nUID) != kInvalidChannel)
    {
        int nPriority = pInfo->m_nPriority;
        if (nPriority != -1)
        {
            FModAudioSystemChannelInfo* pTarget = GetChannelInfoByUID(pInfo->m_nUID);
            if (pTarget != 0)
            {
                m_nLastResult = pTarget->m_pChannel->setPriority(nPriority);
                if (m_nLastResult == FMOD_OK)
                    pTarget->m_nPriority = nPriority;
            }
        }
    }

    if (static_cast<FModId>(pInfo->m_nUID) != kInvalidChannel && pInfo->m_fFrequency != kFltMax)
    {
        float fFrequency = pInfo->m_fFrequency;
        FModAudioSystemChannelInfo* pTarget = GetChannelInfoByUID(pInfo->m_nUID);
        if (pTarget != 0)
        {
            m_nLastResult = pTarget->m_pChannel->setFrequency(fFrequency);
            if (m_nLastResult == FMOD_OK)
                pTarget->m_fFrequency = fFrequency;
        }
    }

    if (static_cast<FModId>(pInfo->m_nUID) != kInvalidChannel && pInfo->m_fVolume != kFltMax)
    {
        float fVolume = pInfo->m_fVolume;
        FModAudioSystemChannelInfo* pTarget = GetChannelInfoByUID(pInfo->m_nUID);
        if (pTarget != 0)
        {
            m_nLastResult = pTarget->m_pChannel->setVolume(fVolume);
            if (m_nLastResult == FMOD_OK)
                pTarget->m_fVolume = fVolume;
        }
    }

    if (static_cast<FModId>(pInfo->m_nUID) != kInvalidChannel && pInfo->m_fMaxDistance != kFltMax
        && pInfo->m_fMinDistance != kFltMax)
    {
        float fMaxDistance = pInfo->m_fMaxDistance;
        float fMinDistance = pInfo->m_fMinDistance;
        FModAudioSystemChannelInfo* pTarget = GetChannelInfoByUID(pInfo->m_nUID);
        if (pTarget != 0)
        {
            m_nLastResult = pTarget->m_pChannel->set3DMinMaxDistance(fMinDistance,
                                                                     fMaxDistance);
            if (m_nLastResult == FMOD_OK)
            {
                pTarget->m_fMinDistance = fMinDistance;
                pTarget->m_fMaxDistance = fMaxDistance;
            }
        }
    }

    if (static_cast<FModId>(pInfo->m_nUID) != kInvalidChannel && pInfo->m_fDirectOcclusion != kFltMax
        && pInfo->m_fReverbOcclusion != kFltMax)
    {
        float fDirectOcclusion = pInfo->m_fDirectOcclusion;
        float fReverbOcclusion = pInfo->m_fReverbOcclusion;
        FModAudioSystemChannelInfo* pTarget = GetChannelInfoByUID(pInfo->m_nUID);
        if (pTarget != 0)
        {
            m_nLastResult = pTarget->m_pChannel->set3DOcclusion(fDirectOcclusion,
                                                                fReverbOcclusion);
            if (m_nLastResult == FMOD_OK)
            {
                pTarget->m_fDirectOcclusion = fDirectOcclusion;
                pTarget->m_fReverbOcclusion = fReverbOcclusion;
            }
        }
    }

    if (static_cast<FModId>(pInfo->m_nUID) != kInvalidChannel && pInfo->m_fPan != kFltMax)
    {
        float fPan = pInfo->m_fPan;
        FModAudioSystemChannelInfo* pTarget = GetChannelInfoByUID(pInfo->m_nUID);
        if (pTarget != 0)
        {
            m_nLastResult = pTarget->m_pChannel->setPan(fPan);
            if (m_nLastResult == FMOD_OK)
                pTarget->m_fPan = fPan;
        }
    }

    if (pInfo != 0 && pInfo->m_pVec3DPosition != 0)
    {
        FMOD_VECTOR* pPosition = pInfo->m_pVec3DPosition;
        m_nLastResult = pInfo->m_pChannel->set3DAttributes(pPosition, 0);
        if (m_nLastResult == FMOD_OK)
        {
            pInfo->SetPosition(pPosition);
        }
    }
    return 1;
}
#endif
