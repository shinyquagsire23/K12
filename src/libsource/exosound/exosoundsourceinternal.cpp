// CExoSoundSourceInternal implementation.

#include "k2stdint.h"
#include "exosoundsourceinternal.h"
#include "exobase.h"
#include "FModAudioSystem.h"
#include "exosoundinternal.h"

#include "port/crtapi.h"
#include "soundprioritygroup.h"

#ifdef K2_QOL_SOUND_STACK
// A multi-target power starts its impact sound once per target in the same instant, and every
// copy is a full-volume voice. Past QOL_STACK_MAX identical one-shots begun within
// QOL_STACK_WINDOW_MS the extras are not started.
static const int QOL_STACK_MAX = 3;
static const unsigned int QOL_STACK_WINDOW_MS = 150;
static const int QOL_STACK_SLOTS = 32;
static CResRef s_aQolStackRes[QOL_STACK_SLOTS];
static unsigned int s_aQolStackTime[QOL_STACK_SLOTS];
static int s_nQolStackNext;

static unsigned int QolStackNow()
{
    return static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
}

static int QolStackIsFull(const CResRef& res, unsigned int nNow)
{
    int nCount = 0;
    for (int i = 0; i < QOL_STACK_SLOTS; i++)
        if (nNow - s_aQolStackTime[i] < QOL_STACK_WINDOW_MS && s_aQolStackRes[i] == res)
            nCount++;
    return nCount >= QOL_STACK_MAX;
}

static void QolStackAdd(const CResRef& res, unsigned int nNow)
{
    s_aQolStackRes[s_nQolStackNext] = res;
    s_aQolStackTime[s_nQolStackNext] = nNow;
    s_nQolStackNext = (s_nQolStackNext + 1) % QOL_STACK_SLOTS;
}
#endif

CExoSoundSourceInternal::CExoSoundSourceInternal(CExoSoundInternal* pSound)
    : m_vPosition(), m_vOcclusionPosition()
{
    m_b3D = 0;
#if !KOTOR1
    m_fFixedVariance = 0.0f;
#endif
    m_bLooping = 0;
    m_bOneShot = 0;
    m_p2DVoice = 0;
    m_p3DVoice = 0;
#if !KOTOR1
    m_pReserved1 = 0;
    m_pReserved2 = 0;
#endif
    m_pListNode = 0;
    m_pSound = pSound;
    m_pAudioSystem = &pSound->m_audioSystem;
    m_nSoundLength = 0;
    m_nOneShotDelay = 0;
    m_nOneShotDelayTimer = 0;
    m_nVoiceId = pSound->m_nNextStreamingVoiceId++;
#if KOTOR1  // K1: the four floats are one 16-byte constant store
    m_fFixedVariance = 0.0f;
#endif
    m_fMaxDistance = 20.0f;
    m_fMinDistance = 10.0f;
    m_fPitchVariance = 0.0f;
    m_bLoaded = 0;
    m_nPlaybackPosition = 0;
    m_nListState = 0;
    m_nPriorityGroup = PRIORITY_GROUP_DEFAULT;

    CPriorityGroup* pGroup = pSound->GetPriorityGroup(PRIORITY_GROUP_DEFAULT);
    m_nGroupVolume = pGroup ? pGroup->m_nVolume : 127;
    m_nVolume = CExoSound::CMAX_VOLUME;

    float fZero = 0.0f;
    m_vOcclusionPosition.x = fZero;
    m_vOcclusionPosition.y = fZero;
    m_vOcclusionPosition.z = fZero;
    m_nOcclusionTime = 0;
    m_fOcclusionFactor = fZero;

    m_resRef.GetResRefStr();
}

// Demands the wave and registers it with the backend as a sound.
int CExoSoundSourceInternal::Demand()
{
    if (m_pRes == 0)
        return 0;

    if (m_bLoaded != 0)
    {
        if (m_pRes->GetDemands() != 0)
            return 1;
    }

    void* pData = m_pRes->Demand();
    m_bLoaded = pData != 0;
    if (pData == 0)
        return 0;

    {
        CExoString sName;
        m_resRef.CopyToString(sName);

        m_pAudioSystem->CreateSound(sName.CStr(), static_cast<int>(m_pRes->GetID()), m_pRes->GetData(),
                                    GetSize(), m_b3D, m_bLooping);
    }

    return m_bLoaded;
}

void CExoSoundSourceInternal::CalculateFixedVarianceFrequency()
{
    m_frequency = static_cast<unsigned int>(m_fFixedVariance * static_cast<float>(m_baseFrequency));
}

// Takes a random pitch step with a random sign. The low bit of rand() picks both the magnitude
// range and the direction, and each arm re-draws rand() for the delta.
void CExoSoundSourceInternal::CalculatePitchVarianceFrequency()
{
    int nRand = rand_wincompatible();
    float fBase = static_cast<float>(m_baseFrequency);
    if (nRand & 1)
    {
        unsigned int nRange = static_cast<unsigned int>(m_fPitchVariance * fBase);
        if (nRange != 0)
            m_nPitchVarianceFrequency += rand_wincompatible() % nRange;
    }
    else
    {
        unsigned int nRange = static_cast<unsigned int>(fBase * 0.5f * m_fPitchVariance);
        if (nRange != 0)
            m_nPitchVarianceFrequency -= rand_wincompatible() % nRange;
    }
}

int CExoSoundSourceInternal::IsScalable()
{
    if (m_nPriorityGroup == PRIORITY_GROUP_LOOPING_AREA_WIDE_AMBIENTS)
        return 1;
    if (m_nPriorityGroup == PRIORITY_GROUP_SINGLE_SHOT_GLOBAL && m_p2DVoice != 0)
        return 1;
    return 0;
}

void CExoSoundSourceInternal::SetLooping(int b)
{
    if (m_nListState != 0)
        return;
    m_bLooping = b;
}

// A source is playing when its bound 3D voice is live, otherwise when its 2D voice is.
int CExoSoundSourceInternal::IsPlaying()
{
    if (m_pSound->m_bInitialized == 0)
        return 0;

    ExoSoundChannel nChannel = EXOSOUND_NO_CHANNEL;
    if (m_p3DVoice != 0)
        nChannel = m_p3DVoice->m_nChannel;

    if (nChannel == EXOSOUND_NO_CHANNEL)
    {
        if (m_p2DVoice == 0)
            return 0;
        nChannel = m_p2DVoice->m_nChannel;
        if (nChannel == EXOSOUND_NO_CHANNEL)
            return 0;
    }

    return m_pAudioSystem->GetIsChannelPlaying(nChannel);
}

#if KOTOR1  // K1: replays the source on its live 3D (else 2D) voice, then rewinds the saved position
void CExoSoundSourceInternal::Resume()
{
    if (m_pSound->m_bInitialized == 0)
        return;
#ifdef K2_FIX_ENGINE_BUGS
    if (IsPlaying())
        return;
#endif

    if (m_p3DVoice != 0 && m_p3DVoice->m_nChannel != EXOSOUND_NO_CHANNEL)
    {
#ifdef K2_FIX_ENGINE_BUGS
        m_pAudioSystem->StopChannel(m_p3DVoice->m_nChannel);
#endif
        m_pSound->PlaySourceOn3DVoice(this, m_p3DVoice);
    }
    else if (m_p2DVoice != 0 && m_p2DVoice->m_nChannel != EXOSOUND_NO_CHANNEL)
    {
#ifdef K2_FIX_ENGINE_BUGS
        m_pAudioSystem->StopChannel(m_p2DVoice->m_nChannel);
#endif
        m_pSound->PlaySourceOn2DVoice(this, m_p2DVoice);
    }
    m_nPlaybackPosition = 0;
}
#else
void CExoSoundSourceInternal::Resume() {}
#endif

int CExoSoundSourceInternal::IsPaused() { return 0; }

void CExoSoundSourceInternal::Stop()
{
    ShutDownSource(false);
}

void CExoSoundSourceInternal::Set3D(int b3D)
{
    if (m_nListState == 1)
        return;
    if (m_p3DVoice != 0)
        return;
    if (m_p2DVoice != 0)
        return;

    m_b3D = b3D;
}

void CExoSoundSourceInternal::SetOneShotDelay(unsigned int nDelay)
{
    if (m_pSound->m_bInitialized == 0)
        return;

    m_nOneShotDelay = nDelay;
    m_nOneShotDelayTimer = g_pExoBase->m_pTimers->GetLowResolutionTimer();
    m_pSound->AddToDelayedOneShotList(this);
}

void CExoSoundSourceInternal::Pause()
{
    if (m_pSound->m_bInitialized == 0)
        return;

    ExoSoundChannel nChannel = EXOSOUND_NO_CHANNEL;
    if (m_p3DVoice != 0)
        nChannel = m_p3DVoice->m_nChannel;
    if (nChannel == EXOSOUND_NO_CHANNEL)
    {
        if (m_p2DVoice == 0)
            return;
        nChannel = m_p2DVoice->m_nChannel;
        if (nChannel == EXOSOUND_NO_CHANNEL)
            return;
    }

    if (m_pAudioSystem->GetIsChannelPlaying(nChannel) == 0)
        return;

    if (m_p3DVoice != 0 && m_p3DVoice->m_nChannel != EXOSOUND_NO_CHANNEL)
    {
        m_nPlaybackPosition = m_pAudioSystem->GetChannelPlaybackPosition(m_p3DVoice->m_nChannel);
        m_pAudioSystem->PauseChannel(m_p3DVoice->m_nChannel);
    }
    else if (m_p2DVoice != 0 && m_p2DVoice->m_nChannel != EXOSOUND_NO_CHANNEL)
    {
        m_nPlaybackPosition = m_pAudioSystem->GetChannelPlaybackPosition(m_p2DVoice->m_nChannel);
        m_pAudioSystem->PauseChannel(m_p2DVoice->m_nChannel);
    }
}

void CExoSoundSourceInternal::SetPosition(const Vector& vPosition)
{
    m_vPosition = vPosition;

    if (m_b3D == 0)
        return;

    if (m_pSound->m_bInitialized == 0)
        return;

    ExoSoundChannel nChannel = EXOSOUND_NO_CHANNEL;
    if (m_p3DVoice != 0)
        nChannel = m_p3DVoice->m_nChannel;
    if (nChannel == EXOSOUND_NO_CHANNEL)
    {
        if (m_p2DVoice == 0)
            return;
        nChannel = m_p2DVoice->m_nChannel;
        if (nChannel == EXOSOUND_NO_CHANNEL)
            return;
    }

    if (m_pAudioSystem->GetIsChannelPlaying(nChannel) == 0)
        return;

#ifdef K2_FIX_ENGINE_BUGS
    if (m_p3DVoice == 0)
        return;
#endif
    m_pAudioSystem->SetChannel3DPosition(m_p3DVoice->m_nChannel,
                                         vPosition.x, vPosition.y, -vPosition.z);
}

// Re-applies volume, pitch variance and the min/max distance pair from the group's record.
void CExoSoundSourceInternal::SetPriorityGroup(unsigned char nGroup)
{
    if (m_pSound->m_bInitialized == 0)
        return;

    m_nPriorityGroup = nGroup;

    CPriorityGroup* pGroup = m_pSound->GetPriorityGroup(nGroup);
#if !KOTOR1  // K1: no null test on the group
    if (pGroup == 0)
        return;
#endif

    if (m_b3D)
    {
        if (m_p3DVoice != 0 && m_p3DVoice->m_nChannel != EXOSOUND_NO_CHANNEL)
            m_pAudioSystem->SetChannelPriority(m_p3DVoice->m_nChannel, pGroup->m_nPriority);
    }
    else
    {
        if (m_p2DVoice != 0 && m_p2DVoice->m_nChannel != EXOSOUND_NO_CHANNEL)
            m_pAudioSystem->SetChannelPriority(m_p2DVoice->m_nChannel, pGroup->m_nPriority);
    }

    m_nGroupVolume = pGroup->m_nVolume;
    SetVolume(m_nVolume, 0, 1.0f);
    SetPitchVariance(pGroup->m_fPlaybackVariance);
    m_fMaxDistance = pGroup->m_fMaxDistance;
    m_fMinDistance = pGroup->m_fMinDistance;
}

// Returns 0 for an out-of-range variance, 1 otherwise.
int CExoSoundSourceInternal::SetFixedVariance(float fVariance)
{
    if (!(fVariance <= 2.0f && fVariance >= 0.5f))
        return 0;

    m_fFixedVariance = fVariance;
    m_fPitchVariance = 0.0f;

    if (m_pSound->m_bInitialized == 0)
        return 1;

    Pause();

#if KOTOR1  // K1: re-tunes a live channel, then always Resume()s
    uint64_t nChannel = EXOSOUND_NO_CHANNEL;
    if (m_p3DVoice != 0)
        nChannel = m_p3DVoice->m_nChannel;
    if (nChannel == EXOSOUND_NO_CHANNEL && m_p2DVoice != 0)
        nChannel = m_p2DVoice->m_nChannel;
    if (nChannel != EXOSOUND_NO_CHANNEL)
    {
        CalculateFixedVarianceFrequency();
        m_pAudioSystem->SetChannelFrequency(nChannel, static_cast<float>(m_frequency));
    }
    Resume();
    return 1;
#else
    int nChannel = EXOSOUND_NO_CHANNEL;
    if (m_p3DVoice != 0)
        nChannel = m_p3DVoice->m_nChannel;
    if (nChannel == EXOSOUND_NO_CHANNEL)
    {
        if (m_p2DVoice == 0)
            return 1;
        nChannel = m_p2DVoice->m_nChannel;
        if (nChannel == EXOSOUND_NO_CHANNEL)
            return 1;
    }

    CalculateFixedVarianceFrequency();
    m_pAudioSystem->SetChannelFrequency(nChannel, static_cast<float>(m_frequency));
    return 1;
#endif
}

// Only a bound 3D slot has an occlusion to push.
void CExoSoundSourceInternal::DoObstructionWork()
{
    if (m_p3DVoice == 0)
        return;

    m_pAudioSystem->SetChannelOcclusion(m_p3DVoice->m_nChannel, m_fOcclusionFactor, 0.0f);
}

int CExoSoundSourceInternal::Release()
{
    if (m_bLoaded == 0)
        return 0;

#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)  // K1: the live channel id is dropped without a StopChannel
    if (m_p2DVoice != 0 && m_p2DVoice->m_nChannel != EXOSOUND_NO_CHANNEL)
        m_p2DVoice->m_nChannel = EXOSOUND_NO_CHANNEL;
    else if (m_p3DVoice != 0 && m_p3DVoice->m_nChannel != EXOSOUND_NO_CHANNEL)
        m_p3DVoice->m_nChannel = EXOSOUND_NO_CHANNEL;
#else
    if (m_p2DVoice != 0 && m_p2DVoice->m_nChannel != EXOSOUND_NO_CHANNEL)
    {
        m_pAudioSystem->StopChannel(m_p2DVoice->m_nChannel);
        m_p2DVoice->m_nChannel = EXOSOUND_NO_CHANNEL;
    }
    else if (m_p3DVoice != 0 && m_p3DVoice->m_nChannel != EXOSOUND_NO_CHANNEL)
    {
        m_pAudioSystem->StopChannel(m_p3DVoice->m_nChannel);
        m_p3DVoice->m_nChannel = EXOSOUND_NO_CHANNEL;
    }
#endif

    m_pAudioSystem->ReleaseSound(m_pRes->GetID());
    m_bLoaded = 0;
    return m_pRes->Release() != 0;
}

CExoSoundSourceInternal::~CExoSoundSourceInternal()
{
    ShutDownSource(false);
}

// m_nListState is the list-state discriminator: 1 playing, 2 non-playing looping, 3 delayed one-shot,
// and only the matching list is unlinked. The flag argument is unused.
#if KOTOR1
void CExoSoundSourceInternal::ShutDownSource()
#else
void CExoSoundSourceInternal::ShutDownSource(bool bShutDown)
#endif
{
#if !KOTOR1
    K2_UNUSED(bShutDown);
#endif
    switch (m_nListState)
    {
    case 1:
        m_pSound->RemoveFromPlayingList(this);
        break;
    case 2:
        m_pSound->RemoveFromNonPlayingLoopingList(this);
        break;
    case 3:
        m_pSound->RemoveFromDelayedOneShotList(this);
        break;
    }

    if (m_pSound->m_bInitialized != 0)
    {
#if KOTOR1  // K1: no StopChannel; both voices drop their owner only when it is this source
        if (m_p3DVoice != 0)
        {
            if (m_p3DVoice->m_nChannel != EXOSOUND_NO_CHANNEL)
            {
#ifdef K2_FIX_ENGINE_BUGS
                m_pAudioSystem->StopChannel(m_p3DVoice->m_nChannel);
#endif
                m_p3DVoice->m_nChannel = EXOSOUND_NO_CHANNEL;
            }
            if (m_p3DVoice->m_pSource == this)
                m_p3DVoice->m_pSource = 0;
            m_p3DVoice = 0;
        }
        else if (m_p2DVoice != 0)
        {
            if (m_p2DVoice->m_nChannel != EXOSOUND_NO_CHANNEL)
            {
#ifdef K2_FIX_ENGINE_BUGS
                m_pAudioSystem->StopChannel(m_p2DVoice->m_nChannel);
#endif
                m_p2DVoice->m_nChannel = EXOSOUND_NO_CHANNEL;
            }
            if (m_p2DVoice->m_pSource == this)
                m_p2DVoice->m_pSource = 0;
            m_p2DVoice = 0;
        }
#else
        if (m_p3DVoice != 0)
        {
            if (m_p3DVoice->m_nChannel != EXOSOUND_NO_CHANNEL)
            {
                m_pAudioSystem->StopChannel(m_p3DVoice->m_nChannel);
                m_p3DVoice->m_nChannel = EXOSOUND_NO_CHANNEL;
            }
            m_p3DVoice->m_pSource = 0;
            m_p3DVoice = 0;
        }
        else if (m_p2DVoice != 0)
        {
            if (m_p2DVoice->m_nChannel != EXOSOUND_NO_CHANNEL)
            {
                m_pAudioSystem->StopChannel(m_p2DVoice->m_nChannel);
                m_p2DVoice->m_nChannel = EXOSOUND_NO_CHANNEL;
            }
            if (m_p2DVoice->m_pSource == this)
                m_p2DVoice->m_pSource = 0;
            m_p2DVoice = 0;
        }
#endif
    }

    Release();
    m_nListState = 0;
    m_pListNode = 0;
}

// The flag argument means raw: when it is 0 the volume byte is re-derived by comparing the slider's
// raw and effective pair.
void CExoSoundSourceInternal::SetVolume(unsigned char nVolume, int a, float f)
{
    int nSliderType = m_pSound->GetSliderType(this);

    if (a == 0)
        m_nVolume = nVolume;

    if (!(m_pSound->GetMutable(this) | a))
        a = (m_pSound->GetSliderVolume(nSliderType, 1) != m_pSound->GetSliderVolume(nSliderType, 0));

    float fVol = m_pSound->GetSliderVolume(nSliderType, a);

    float fScale = 1.0f;
    if (m_nPriorityGroup == PRIORITY_GROUP_LOOPING_AREA_WIDE_AMBIENTS || (m_nPriorityGroup == PRIORITY_GROUP_SINGLE_SHOT_GLOBAL && m_p2DVoice != 0))
        fScale = f;

    if (m_pSound->m_bInitialized == 0)
        return;

    int nScaled = static_cast<int>(fScale * static_cast<float>(nVolume) * static_cast<float>(m_nGroupVolume) / 127.0f);

    if (m_b3D)
    {
        if (m_p3DVoice == 0)
            return;
        ExoSoundChannel nChannel = m_p3DVoice->m_nChannel;
        if (nChannel == EXOSOUND_NO_CHANNEL)
            return;

        float s = m_pSound->m_f2D3DBias;
        if (s < 1.0f)
            fVol = fVol * s;

        m_pAudioSystem->SetChannelVolume(nChannel, fVol * static_cast<float>(nScaled) / 127.0f);
    }
    else
    {
        if (m_p2DVoice == 0)
            return;
        ExoSoundChannel nChannel = m_p2DVoice->m_nChannel;
        if (nChannel == EXOSOUND_NO_CHANNEL)
            return;

        float s = m_pSound->m_f2D3DBias;
        if (s > 1.0f)
            fVol = fVol * (2.0f - s);

        m_pAudioSystem->SetChannelVolume(nChannel, fVol * static_cast<float>(nScaled) / 127.0f);
    }
}

// Clamps the variance to [0, 1] and folds a fresh random delta into m_nPitchVarianceFrequency,
// so the value accumulates.
void CExoSoundSourceInternal::SetPitchVariance(float fVariance)
{
    if (fVariance < 0.0f)
        fVariance = 0.0f;
    if (fVariance > 1.0f)
        fVariance = 1.0f;

    m_fPitchVariance = fVariance;
    m_fFixedVariance = 0.0f;

    if (m_pSound->m_bInitialized == 0)
        return;

    Pause();

    if (m_p3DVoice != 0 && m_p3DVoice->m_nChannel != EXOSOUND_NO_CHANNEL)
    {
        if (rand_wincompatible() & 1)
        {
            unsigned int nRange = static_cast<unsigned int>(static_cast<float>(m_baseFrequency) * m_fPitchVariance);
            if (nRange != 0)
                m_nPitchVarianceFrequency += static_cast<unsigned int>(rand_wincompatible()) % nRange;
        }
        else
        {
            unsigned int nRange = static_cast<unsigned int>(static_cast<float>(m_baseFrequency) * 0.5f * m_fPitchVariance);
            if (nRange != 0)
                m_nPitchVarianceFrequency -= static_cast<unsigned int>(rand_wincompatible()) % nRange;
        }

        m_pAudioSystem->SetChannelFrequency(m_p3DVoice->m_nChannel, static_cast<float>(m_nPitchVarianceFrequency));
    }
    else if (m_p2DVoice != 0 && m_p2DVoice->m_nChannel != EXOSOUND_NO_CHANNEL)
    {
        if (rand_wincompatible() & 1)
        {
            unsigned int nRange = static_cast<unsigned int>(static_cast<float>(m_baseFrequency) * m_fPitchVariance);
            if (nRange != 0)
                m_nPitchVarianceFrequency += static_cast<unsigned int>(rand_wincompatible()) % nRange;
        }
        else
        {
            unsigned int nRange = static_cast<unsigned int>(static_cast<float>(m_baseFrequency) * 0.5f * m_fPitchVariance);
            if (nRange != 0)
                m_nPitchVarianceFrequency -= static_cast<unsigned int>(rand_wincompatible()) % nRange;
        }

        m_pAudioSystem->SetChannelFrequency(m_p2DVoice->m_nChannel, static_cast<float>(m_nPitchVarianceFrequency));
    }
#if KOTOR1  // K1: the paused source is resumed
    Resume();
#endif
}

// A non-3D source is unconditionally in range; a 3D one is measured against the group's max
// distance.
int CExoSoundSourceInternal::GetWithinRangeOfListener()
{
    if (m_b3D == 0)
        return 1;

    Vector vDelta = m_pSound->m_vListenerPosition - m_vPosition;
    return magnitude(vDelta) <= m_fMaxDistance;
}

void CExoSoundSourceInternal::StoreRestartSoundPosition()
{
    if (m_pSound->m_bInitialized == 0)
        m_nPlaybackPosition = 0;
    else
        m_nPlaybackPosition = m_pAudioSystem->GetChannelPlaybackPosition(
            m_b3D ? m_p3DVoice->m_nChannel : m_p2DVoice->m_nChannel);
}

// Loads the wave if it is not loaded yet, then either restarts a channel this source already
// owns or picks a free voice through the priority group and files the source in the playing
// list. Returns the wave length.
int CExoSoundSourceInternal::Play(int a, int b)
{
    CExoString sName;

    m_bOneShotFailed = 0;
    m_nOneShotDelay = 0;
    m_nOneShotDelayTimer = 0;

    if (m_pSound->m_bInitialized != 0)
    {
        int bNotLooping = (m_bLooping == 0);

        if (b == 0)
        {
            m_nPlaybackPosition = 0;
            if (m_p3DVoice == 0)
                goto Try2D;
        }
        else if (m_p3DVoice == 0)
        {
            goto Try2D;
        }

        if (m_p3DVoice->m_nChannel != EXOSOUND_NO_CHANNEL)
        {
#if KOTOR1
            m_pAudioSystem->PauseChannel(m_p3DVoice->m_nChannel);
            m_pAudioSystem->SetChannelLoopCount(m_p3DVoice->m_nChannel, bNotLooping);
            m_pAudioSystem->SetChannelPlaybackPosition(m_p3DVoice->m_nChannel, 0);
            m_pAudioSystem->ResumeChannel(m_p3DVoice->m_nChannel);
#else
            m_pAudioSystem->PauseChannel(static_cast<unsigned int>(m_p3DVoice->m_nChannel));
            m_pAudioSystem->SetChannelLoopCount(static_cast<unsigned int>(m_p3DVoice->m_nChannel), bNotLooping);
            m_pAudioSystem->SetChannelPlaybackPosition(static_cast<unsigned int>(m_p3DVoice->m_nChannel), 0);
            m_pAudioSystem->ResumeChannel(static_cast<unsigned int>(m_p3DVoice->m_nChannel));
#endif
            return m_nSoundLength;
        }

Try2D:
        if (m_p2DVoice != 0 && m_p2DVoice->m_nChannel != EXOSOUND_NO_CHANNEL)
        {
#if KOTOR1  // K1: a live 2D voice is restarted like the 3D one (pause, loop count, rewind, resume)
            m_pAudioSystem->PauseChannel(m_p2DVoice->m_nChannel);
            m_pAudioSystem->SetChannelLoopCount(m_p2DVoice->m_nChannel, bNotLooping);
            m_pAudioSystem->SetChannelPlaybackPosition(m_p2DVoice->m_nChannel, 0);
            m_pAudioSystem->ResumeChannel(m_p2DVoice->m_nChannel);
#else
            m_pAudioSystem->SetChannelPlaybackPosition(static_cast<unsigned int>(m_p2DVoice->m_nChannel), 0);
#endif
            return m_nSoundLength;
        }

        if (m_nSoundLength == 0 && m_bLoaded == 0)
        {
            if (Demand() == 0)
                goto Fail;

            if (m_pRes != 0)
            {
                sName.CStr();
                m_pRes->GetDemands();
            }

            m_nSoundLength = m_pSound->m_audioSystem.GetSoundLength(static_cast<int>(m_pRes->GetID()));
            m_nPitchVarianceFrequency = m_baseFrequency =
                static_cast<unsigned int>(m_pSound->m_audioSystem.GetSoundSampleRate(static_cast<int>(m_pRes->GetID())));

            if (m_fPitchVariance != 0.0f)
                CalculatePitchVarianceFrequency();

            if (m_fFixedVariance != 0.0f)
                CalculateFixedVarianceFrequency();
        }

        if (m_b3D != 0)
        {
            if (!(magnitude(m_pSound->m_vListenerPosition - m_vPosition) <= m_fMaxDistance))
            {
                Release();
                if (m_bLooping == 0)
                    goto Fail;
                if (m_nListState != 0)
                    return m_nSoundLength;
                m_pSound->AddToNonPlayingLoopingList(this);
                return m_nSoundLength;
            }
        }

        CPriorityGroup* pGroup = m_pSound->GetPriorityGroup(m_nPriorityGroup);
#if KOTOR1  // K1: no null test on the group
        if (pGroup->m_nPlayingCount >= pGroup->m_nMaxPlaying)
#else
        if (pGroup != 0 && pGroup->m_nPlayingCount >= pGroup->m_nMaxPlaying)
#endif
        {
#if KOTOR1  // K1: an interrupting group stops one of its playing sounds and this one plays
            if (a == 0 && pGroup->m_nInterrupt != 0)
                m_pSound->InterruptPlayingSound(m_nPriorityGroup);
            else
            {
            Release();
            if (m_bLooping == 0)
                goto Fail;
            if (m_nListState != 0)
                return m_nSoundLength;
            m_pSound->AddToNonPlayingLoopingList(this);
            return m_nSoundLength;
            }
        }
#else
            if (a == 0)
            {
                if (pGroup->m_nInterrupt != 0)
                    goto Fail;
            }

            Release();
            if (m_bLooping == 0)
                goto Fail;
            if (m_nListState != 0)
                return m_nSoundLength;
            m_pSound->AddToNonPlayingLoopingList(this);
            return m_nSoundLength;
        }
#endif

#ifdef K2_QOL_SOUND_STACK
        unsigned int nQolNow = QolStackNow();
        if (m_bLooping == 0 && QolStackIsFull(m_resRef, nQolNow))
        {
            Release();
            goto Fail;
        }
#endif

        if (m_b3D != 0)
        {
            C3DVoice* pVoice = m_pSound->GetFree3DVoice(m_nPriorityGroup);
            if (pVoice == 0)
            {
                Release();
                if (m_bLooping == 0)
                    goto Fail;
                if (m_nListState != 0)
                    return m_nSoundLength;
                m_pSound->AddToNonPlayingLoopingList(this);
                return m_nSoundLength;
            }

            if (m_bLoaded == 0 && Demand() == 0)
                goto Fail;

            m_pSound->PlaySourceOn3DVoice(this, pVoice);
        }
        else
        {
            C2DVoice* pVoice = m_pSound->GetFree2DVoice(m_nPriorityGroup);
            if (pVoice == 0)
            {
                Release();
                if (m_bLooping == 0)
                    goto Fail;
                if (m_nListState != 0)
                    return m_nSoundLength;
                m_pSound->AddToNonPlayingLoopingList(this);
                return m_nSoundLength;
            }

            if (m_bLoaded == 0 && Demand() == 0)
                goto Fail;

            m_pSound->PlaySourceOn2DVoice(this, pVoice);
        }

        if (m_nListState == 2)
            m_pSound->RemoveFromNonPlayingLoopingList(this);

        m_pSound->AddToPlayingList(this);
#ifdef K2_QOL_SOUND_STACK
        if (m_bLooping == 0)
            QolStackAdd(m_resRef, nQolNow);
#endif

        if (m_pSound->m_bLostFocusPaused != 0
            || (g_bSoundPaused != 0 && m_pSound->GetPauseable(this) != 0))
        {
            Pause();
            return m_nSoundLength;
        }
    }
    else
    {
        if (m_bOneShot != 0)
            m_bOneShotFailed = 1;
        return m_nSoundLength;
    }

    return m_nSoundLength;

Fail:
    if (m_bOneShot != 0)
        m_bOneShotFailed = 1;
    return m_nSoundLength;
}
