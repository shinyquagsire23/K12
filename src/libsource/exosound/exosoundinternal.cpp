// The sound system's implementation, with voice pools, the priority-group table, the volume and
// mute state and the per-frame mixer.

#include "k2stdint.h"
#include "exosoundinternal.h"
#include "exosoundsourceinternal.h"
#include "exostreamingsoundsourceinternal.h"
#include "exosound.h"
#include "twodimarray.h"
#include "exoresman.h"
#include "soundprioritygroup.h"

int g_bSoundPaused;

#include "port/libcstring.h"
#include "port/libcstdlib.h"

// The voice pools: each holds at most 64, and at least 16 3D and 24 2D voices.
enum {
    MAX_VOICES = 64,
    MIN_3D_VOICES = 16,
    MIN_2D_VOICES = 24,
};

// A priority group's defaults before soundprioritygroups.2da is read: no limit on how many play,
// at full volume.
enum {
    PRIORITYGROUP_UNLIMITED = 255,
    SOUND_FULL_VOLUME = 127,
};

// GetSpeakerType's answer: the port never knows the speaker layout.
enum {
    SPEAKER_TYPE_UNKNOWN = 255,
};


int g_bDisableSound;

extern int g_totalStreamingHandlesAvailable;
#if !KOTOR1
int g_totalStreamingHandlesAvailable;
#endif
extern int g_generalStreamingHandlesAvailable;
#if !KOTOR1
int g_generalStreamingHandlesAvailable;
#endif

CExoSoundInternal::CExoSoundInternal()
{
    m_nReserved1 = 0;
    m_nReserved2 = 0;
    m_bInitialized = 0;
    m_paGroups = 0;
    m_maxGroup = 0;
    m_nNextStreamingVoiceId = 0;
#if !KOTOR1
    m_fDuckVolume = 1.0f;
#endif
    m_pa2DVoices = 0;
    m_pa3DVoices = 0;

    LoadPriorityGroups();

    m_fSoundEffectVolumeRaw = 1.0f;
    m_fDialogVolumeRaw = 1.0f;
    m_fMusicVolume = 1.0f;
    m_fSoundEffectVolume = 1.0f;
    m_bStreamingEnvironmentEffects = 1;
    m_bNonStreamingEnvironmentEffects = 1;
    m_fAmbientScale = 1.0f;
    m_fAmbientScaleRampStart = 1.0f;
    m_f2D3DBias = 1.0f;
    m_fDialogVolume = 1.0f;
    m_fEnvironmentEffectLevel = 0.6f;
    m_fMusicVolumeRaw = 1.0f;
    m_nUnMuteDuration = 0;
    m_nMuteTimerStart = 0;
    m_nMuteDuration = 0;
    m_bMuted = 0;
    m_nNumber2DVoices = 0;
    m_nNumber3DVoices = 0;
    m_nBestEAXAvailable = 0;
    m_fLastDesiredAmbientScale = 1.0f;
    m_nDesiredEnvironment = 0;
    m_fDesiredAmbientScale = 1.0f;

    m_nLastRenderTime = static_cast<int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
    m_nOcclusionCursorList = 1;
    m_pOcclusionCursor = 0;
    m_nOcclusionTimeUsedMax = 0;
    m_nObstructionTestsMax = 0;
    m_nOcclusionIdleFrames = 0;
    m_nOcclusionIdleFramesMax = 0;
    m_nOcclusionMaxTimeHigh = 0;
    m_nOcclusionTimeBudget = 3;
    m_nOcclusionTimeUsed = 0;
    m_nObstructionTests = 0;
    m_bDisplayOcclusionInfo = 0;
    g_bSoundPaused = 0;
    m_aSoundModes[0] = CExoSound::CEXOSOUND_MODE_GENERIC;
    m_nSoundModeIndex = 0;
    m_bLostFocusPaused = 0;
    m_pStreamingSoundSource = 0;
}

// Reads soundprioritygroups.2da: seeds every row, then reads one column at a time. Rows 2 and 9 get
// their volume scaled, and the playback variance is clamped to 0..1.
void CExoSoundInternal::LoadPriorityGroups()
{
    C2DA* p2DA = new C2DA(CResRef("PriorityGroups"), 0);

    if (p2DA->Load2DArray() == 0)
    {
        delete p2DA;
        return;
    }

    unsigned char nRows = static_cast<unsigned char>(p2DA->m_nRows);
    m_maxGroup = nRows;

    unsigned long long* pCounted =
        static_cast<unsigned long long*>(operator new[](sizeof(CPriorityGroup) * nRows + 8));
    *pCounted = nRows;
    CPriorityGroup* pGroups = reinterpret_cast<CPriorityGroup*>(pCounted + 1);

    CPriorityGroup* pEnd = pGroups + nRows;
    for (CPriorityGroup* pGroup = pGroups; pGroup != pEnd; pGroup++)
    {
        pGroup->m_nInterrupt = 1;
        pGroup->m_nMaxPlaying = PRIORITYGROUP_UNLIMITED;
        pGroup->m_nPlayingCount = 0;
        pGroup->m_nPriority = 0;
        pGroup->m_nVolume = SOUND_FULL_VOLUME;
        pGroup->m_fMaxDistance = 20.0f;
        pGroup->m_fMinDistance = 10.0f;
        pGroup->m_fPlaybackVariance = 0.0f;
        pGroup->m_nFadeTime = 0;
    }

    m_paGroups = pGroups;

    for (unsigned int i = 0; i < m_maxGroup; i++)
    {
        int nValue;
        float fMaxVolumeDistance;
        float fMinVolumeDistance;
        float fPlaybackVariance;

        if (p2DA->GetINTEntry(i, CExoString("Priority"), &nValue))
            m_paGroups[i].m_nPriority = static_cast<unsigned char>(nValue);

#if KOTOR1  // K1 reads the "Volume" column and has no boost on rows 2 and 9
        if (p2DA->GetINTEntry(i, CExoString("Volume"), &nValue))
            m_paGroups[i].m_nVolume = static_cast<unsigned char>(nValue);
#else
        if (p2DA->GetINTEntry(i, CExoString("Volume_PC"), &nValue))
        {
            if (i == 2 || i == 9)
                nValue = static_cast<int>(static_cast<float>(nValue) * 1.4f);
            m_paGroups[i].m_nVolume = static_cast<unsigned char>(nValue);
        }
#endif

        if (p2DA->GetINTEntry(i, CExoString("MaxPlaying"), &nValue))
            m_paGroups[i].m_nMaxPlaying = static_cast<unsigned char>(nValue);

        if (p2DA->GetINTEntry(i, CExoString("Interrupt"), &nValue))
            m_paGroups[i].m_nInterrupt = nValue;

        if (p2DA->GetINTEntry(i, CExoString("FadeTime"), &nValue))
            m_paGroups[i].m_nFadeTime = static_cast<unsigned short>(nValue);

        {
#if KOTOR1  // K1's distance columns have no _PC suffix
            CExoString sMaxVolumeColumn("MaxVolumeDist");
#else
            CExoString sMaxVolumeColumn("MaxVolumeDist_PC");
#endif
            if (p2DA->GetFLOATEntry(i, sMaxVolumeColumn, &fMaxVolumeDistance))
            {
#if KOTOR1
                CExoString sMinVolumeColumn("MinVolumeDist");
#else
                CExoString sMinVolumeColumn("MinVolumeDist_PC");
#endif
                // The columns are named for the volume; MinVolumeDist is where the sound fades out
                // (the maximum distance), MaxVolumeDist the radius it stays full inside.
                if (p2DA->GetFLOATEntry(i, sMinVolumeColumn, &fMinVolumeDistance))
                {
                    m_paGroups[i].m_fMaxDistance = fMinVolumeDistance;
                    m_paGroups[i].m_fMinDistance = fMaxVolumeDistance;
                }
            }
        }

        if (p2DA->GetFLOATEntry(i, CExoString("PlaybackVariance"), &fPlaybackVariance))
        {
            fPlaybackVariance = fPlaybackVariance < 0.0f ? 0.0f : fPlaybackVariance;
            fPlaybackVariance = fPlaybackVariance > 1.0f ? 1.0f : fPlaybackVariance;
            m_paGroups[i].m_fPlaybackVariance = fPlaybackVariance;
        }
    }

    delete p2DA;
}

CExoSoundInternal::~CExoSoundInternal()
{
    ShutDown();

    if (m_paGroups)
    {
        operator delete[](reinterpret_cast<char*>(m_paGroups) - 8);  // TODO: idiom (the array cookie LoadPriorityGroups writes; new[] would call the out-of-line ctor the original inlines)  // raw-offset: new[] array cookie
        m_paGroups = 0;
    }

    m_aProviders.m_count = 0;
    m_aProviders.Clear();
}

// Shuts down in four passes: stop the streaming sources, reap the playing list, hand both voice
// tables back to the backend, then clear the live count.
void CExoSoundInternal::ShutDown()
{
    CExoLinkedListNode* pNode = m_lStreaming.m_pInternal->m_pHead;
    CExoStreamingSoundSourceInternal* pStream = 0;
    if (m_lStreaming.m_pInternal->m_pHead != 0)
        pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->m_pHead->m_pData);

    while (pNode != 0)
    {
        m_lStreaming.m_pInternal->GetNext(pNode);
        pStream->Stop(0);
        if (pNode != 0)
            pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->GetAtPos(pNode));
    }

    while (m_lPlaying.m_pInternal->m_pHead != 0)
    {
        CExoSoundSourceInternal* pSource =
            static_cast<CExoSoundSourceInternal*>(m_lPlaying.m_pInternal->m_pHead->m_pData);

        if (!pSource->m_bOneShot)
        {
            CExoString sMsg;
            pSource->m_resRef.CopyToString(sMsg);
            sMsg.Format(const_cast<char*>("CExoSoundSource %s not freed"), sMsg.CStr());
        }

        if (pSource)
            delete pSource;
    }

    if (!g_bDisableSound)
    {
        if (m_pa3DVoices)
        {
            for (uint64_t i = 0; i < m_nNumber3DVoices; i++)
            {
                if (m_pa3DVoices[i].m_nChannel != EXOSOUND_NO_CHANNEL)
                {
#if !KOTOR1  // K1 does not stop the channel first
                    m_audioSystem.StopChannel(m_pa3DVoices[i].m_nChannel);
#endif
                    m_audioSystem.ReleaseSound(m_pa3DVoices[i].m_pSource->m_pRes->GetID());
                    m_pa3DVoices[i].m_nChannel = EXOSOUND_NO_CHANNEL;
                }
            }
            delete[] m_pa3DVoices;
            m_pa3DVoices = 0;
        }

        if (m_pa2DVoices)
        {
            for (uint64_t i = 0; i < m_nNumber2DVoices; i++)
            {
                if (m_pa2DVoices[i].m_nChannel != EXOSOUND_NO_CHANNEL)
                {
#if KOTOR1  // K1 stops nothing and does not null-test the wave
                    m_audioSystem.ReleaseSound(m_pa2DVoices[i].m_pSource->m_pRes->GetID());
#else
                    m_audioSystem.StopChannel(m_pa2DVoices[i].m_nChannel);
                    if (m_pa2DVoices[i].m_pSource->m_pRes)
                        m_audioSystem.ReleaseSound(m_pa2DVoices[i].m_pSource->m_pRes->GetID());
#endif
                    m_pa2DVoices[i].m_nChannel = EXOSOUND_NO_CHANNEL;
                }
            }
            delete[] m_pa2DVoices;
            m_pa2DVoices = 0;
        }
    }

    m_bInitialized = 0;
}

#if KOTOR1
// K1: the voice tables are freed and re-allocated at the clamped counts; no StopChannel and no
// handle pools.
bool CExoSoundInternal::Initialize(unsigned char n2DVoices, unsigned char n3DVoices, int a, int b)
{
    K2_UNUSED(a); K2_UNUSED(b);
    if (g_bDisableSound == 1)
        return false;

    CExoLinkedList<CExoSoundSourceInternal> lReaped;

    if (m_audioSystem.m_bInitialized)
    {
        CExoLinkedListNode* pNode = m_lStreaming.m_pInternal->m_pHead;
        CExoStreamingSoundSourceInternal* pStream = 0;
        if (m_lStreaming.m_pInternal->m_pHead != 0)
            pStream =
                static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->m_pHead->m_pData);

        while (pNode != 0)
        {
            if (pStream->IsPlaying())
                pStream->Pause();

            if (pStream->m_nChannel != 0)
            {
                m_audioSystem.CloseStream(pStream->m_nStreamHandle);
                pStream->m_nChannel = static_cast<uint64_t>(-1);
                pStream->m_nStreamHandle = 0;
            }

            pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.GetNext(pNode));
        }

        if (m_lPlaying.m_pInternal->m_pHead != 0)
        {
            CExoSoundSourceInternal* pSource =
                static_cast<CExoSoundSourceInternal*>(m_lPlaying.m_pInternal->m_pHead->m_pData);

            for (;;)
            {
                CExoLinkedListNode* pNext = RemoveFromPlayingList(pSource);

                if (pSource->IsPlaying())
                {
                    pSource->StoreRestartSoundPosition();
                    pSource->ShutDownSource();
                    lReaped.AddHead(pSource);
                }
                else if (pSource->m_bOneShot)
                {
                    if (!m_lStoppingOneShots.m_pInternal->Contains(pSource))
                        delete pSource;
                }
                else
                {
                    pSource->ShutDownSource();
                }

                if (pNext == 0)
                    break;

                pSource = static_cast<CExoSoundSourceInternal*>(m_lPlaying.GetAtPos(pNext));
            }
        }

        if (m_pa3DVoices)
        {
            for (uint64_t i = 0; i < m_nNumber3DVoices; i++)
            {
                if (m_pa3DVoices[i].m_nChannel != static_cast<uint64_t>(-1))
                {
                    m_audioSystem.ReleaseSound(m_pa3DVoices[i].m_pSource->m_pRes->GetID());
                    m_pa3DVoices[i].m_nChannel = static_cast<uint64_t>(-1);
                }
            }
            delete[] m_pa3DVoices;
            m_pa3DVoices = 0;
        }

        if (m_pa2DVoices)
        {
            for (uint64_t i = 0; i < m_nNumber2DVoices; i++)
            {
                if (m_pa2DVoices[i].m_nChannel != static_cast<uint64_t>(-1))
                {
                    m_audioSystem.ReleaseSound(m_pa2DVoices[i].m_pSource->m_pRes->GetID());
                    m_pa2DVoices[i].m_nChannel = static_cast<uint64_t>(-1);
                }
            }
            delete[] m_pa2DVoices;
            m_pa2DVoices = 0;
        }

        m_audioSystem.ShutdownSystem();
    }
    else
    {
        int n3D = n3DVoices;
        if (n3D > MAX_VOICES)
            n3D = MAX_VOICES;
        if (n3D < MIN_3D_VOICES)
            n3D = MIN_3D_VOICES;

        int n2D = n2DVoices;
        if (n2D > MAX_VOICES)
            n2D = MAX_VOICES;
        if (n2D < MIN_2D_VOICES)
            n2D = MIN_2D_VOICES;

        m_audioSystem.InitSystem(n3D + n2D);
    }

    m_bInitialized = 1;

    if (!m_audioSystem.m_bInitialized)
    {
        m_audioSystem.ShutdownSystem();
        m_bInitialized = 0;
        return false;
    }

    unsigned char n3D = n3DVoices;
    if (n3D > MAX_VOICES)
        n3D = MAX_VOICES;
    if (n3D < MIN_3D_VOICES)
        n3D = MIN_3D_VOICES;
    m_nNumber3DVoices = n3D;

    C3DVoice* pa3DVoices = new C3DVoice[n3D];
    for (uint64_t i = 0; i < n3D; i++)
    {
        pa3DVoices[i].m_pSource = 0;
        pa3DVoices[i].m_nChannel = 0;
        pa3DVoices[i].m_bInUse = 0;
        pa3DVoices[i].m_nReserved1 = 0;
    }
    m_pa3DVoices = pa3DVoices;

    unsigned char n2D = n2DVoices;
    if (n2D > MAX_VOICES)
        n2D = MAX_VOICES;
    if (n2D < MIN_2D_VOICES)
        n2D = MIN_2D_VOICES;
    m_nNumber2DVoices = n2D;

    C2DVoice* pa2DVoices = new C2DVoice[n2D];
    for (uint64_t i = 0; i < n2D; i++)
    {
        pa2DVoices[i].m_pSource = 0;
        pa2DVoices[i].m_nChannel = 0;
        pa2DVoices[i].m_bInUse = 0;
        pa2DVoices[i].m_nReserved1 = 0;
    }
    m_pa2DVoices = pa2DVoices;

    unsigned char nMax = m_nNumber3DVoices > m_nNumber2DVoices ? m_nNumber3DVoices
                                                              : m_nNumber2DVoices;
    for (uint64_t i = 0; i < nMax; i++)
    {
        if (i < m_nNumber3DVoices)
        {
            m_pa3DVoices[i].m_pSource = 0;
            m_pa3DVoices[i].m_nReserved1 = 0;
            m_pa3DVoices[i].m_nChannel = static_cast<uint64_t>(-1);
            m_pa3DVoices[i].m_bInUse = 0;
        }
        if (i < m_nNumber2DVoices)
        {
            m_pa2DVoices[i].m_pSource = 0;
            m_pa2DVoices[i].m_nReserved1 = 0;
            m_pa2DVoices[i].m_nChannel = static_cast<uint64_t>(-1);
            m_pa2DVoices[i].m_bInUse = 0;
        }
    }

    m_environment = 0;

    while (lReaped.m_pInternal->m_nCount != 0)
    {
        CExoSoundSourceInternal* pReaped =
            static_cast<CExoSoundSourceInternal*>(lReaped.RemoveHead());
        pReaped->Play(1, 1);
    }

    CExoLinkedListNode* pNode = m_lStreaming.m_pInternal->m_pHead;
    CExoStreamingSoundSourceInternal* pStream = 0;
    if (m_lStreaming.m_pInternal->m_pHead != 0)
        pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->m_pHead->m_pData);

    while (pNode != 0)
    {
        pStream->InitializeSource();
        pStream->Resume();
        pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.GetNext(pNode));
    }

    return true;
}
#else
// Tears the backend down or starts it, then rebuilds both voice tables and re-queues the streaming
// sources that were playing. Returns false when sound is disabled or the backend will not start.
bool CExoSoundInternal::Initialize(unsigned char n2DVoices, unsigned char n3DVoices, int a, int b)
{
    K2_UNUSED(a); K2_UNUSED(b);
    if (g_bDisableSound == 1)
        return false;

    // The reaped playing sources park here while the backend restarts.
    CExoLinkedList<CExoSoundSourceInternal> lReaped;

    if (m_audioSystem.m_bInitialized)
    {
        CExoLinkedListNode* pNode = m_lStreaming.m_pInternal->m_pHead;
        CExoStreamingSoundSourceInternal* pStream = 0;
        if (m_lStreaming.m_pInternal->m_pHead != 0)
            pStream =
                static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->m_pHead->m_pData);

        while (pNode != 0)
        {
            if (pStream->IsPlaying())
                pStream->Pause();

            if (pStream->m_nChannel != 0)
            {
                m_audioSystem.StopChannel(pStream->m_nChannel);
                m_audioSystem.CloseStream(pStream->m_nStreamHandle);
                pStream->m_nChannel = -1;
                pStream->m_nStreamHandle = 0;
            }

            pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.GetNext(pNode));
        }

        if (m_lPlaying.m_pInternal->m_pHead != 0)
        {
            CExoSoundSourceInternal* pSource =
                static_cast<CExoSoundSourceInternal*>(m_lPlaying.m_pInternal->m_pHead->m_pData);

            for (;;)
            {
                CExoLinkedListNode* pNext = RemoveFromPlayingList(pSource);

                if (pSource->IsPlaying())
                {
                    pSource->StoreRestartSoundPosition();
                    pSource->ShutDownSource(0);
                    lReaped.AddHead(pSource);
                }
                else if (pSource->m_bOneShot)
                {
                    if (!m_lStoppingOneShots.m_pInternal->Contains(pSource))
                        delete pSource;
                }
                else
                {
                    pSource->ShutDownSource(0);
                }

                if (pNext == 0)
                    break;

                pSource = static_cast<CExoSoundSourceInternal*>(m_lPlaying.GetAtPos(pNext));
            }
        }

        if (m_pa3DVoices && m_nNumber3DVoices)
        {
            for (uint64_t i = 0; i < m_nNumber3DVoices; i++)
            {
                if (m_pa3DVoices[i].m_nChannel != EXOSOUND_NO_CHANNEL)
                {
                    m_audioSystem.StopChannel(m_pa3DVoices[i].m_nChannel);
                    m_audioSystem.ReleaseSound(m_pa3DVoices[i].m_pSource->m_pRes->GetID());
                    m_pa3DVoices[i].m_nChannel = EXOSOUND_NO_CHANNEL;
                }
            }
        }

        if (m_pa2DVoices && m_nNumber2DVoices)
        {
            for (uint64_t i = 0; i < m_nNumber2DVoices; i++)
            {
                if (m_pa2DVoices[i].m_nChannel != EXOSOUND_NO_CHANNEL)
                {
                    m_audioSystem.StopChannel(m_pa2DVoices[i].m_nChannel);
                    m_audioSystem.ReleaseSound(m_pa2DVoices[i].m_pSource->m_pRes->GetID());
                    m_pa2DVoices[i].m_nChannel = EXOSOUND_NO_CHANNEL;
                }
            }
        }

        m_audioSystem.ShutdownSystem();
    }
    else
    {
        int n3D = n3DVoices;
        if (n3D > MAX_VOICES)
            n3D = MAX_VOICES;
        if (n3D < MIN_3D_VOICES)
            n3D = MIN_3D_VOICES;

        int n2D = n2DVoices;
        if (n2D > MAX_VOICES)
            n2D = MAX_VOICES;
        if (n2D < MIN_2D_VOICES)
            n2D = MIN_2D_VOICES;

        m_audioSystem.InitSystem(n3D + n2D);
    }

    m_bInitialized = 1;

    if (!m_audioSystem.m_bInitialized)
    {
        m_audioSystem.ShutdownSystem();
        m_bInitialized = 0;
        return false;
    }

    unsigned char n3D = n3DVoices;
    if (n3D > MAX_VOICES)
        n3D = MAX_VOICES;
    if (n3D < MIN_3D_VOICES)
        n3D = MIN_3D_VOICES;
    m_nNumber3DVoices = n3D;

    if (m_pa3DVoices == 0)
    {
        // Allocated uninitialised and filled field by field; value-initialising the array would zero
        // it in one call instead.
        C3DVoice* paVoices = new C3DVoice[MAX_VOICES];
        for (uint64_t i = 0; i < MAX_VOICES; i++)
        {
            paVoices[i].m_pSource = 0;
            paVoices[i].m_nChannel = 0;
            paVoices[i].m_bInUse = 0;
            paVoices[i].m_nReserved1 = 0;
        }
        m_pa3DVoices = paVoices;
    }

    unsigned char n2D = n2DVoices;
    if (n2D > MAX_VOICES)
        n2D = MAX_VOICES;
    if (n2D < MIN_2D_VOICES)
        n2D = MIN_2D_VOICES;
    m_nNumber2DVoices = n2D;

    if (m_pa2DVoices == 0)
    {
        C2DVoice* paVoices = new C2DVoice[MAX_VOICES];
        for (uint64_t i = 0; i < MAX_VOICES; i++)
        {
            paVoices[i].m_pSource = 0;
            paVoices[i].m_nChannel = 0;
            paVoices[i].m_bInUse = 0;
            paVoices[i].m_nReserved1 = 0;
        }
        m_pa2DVoices = paVoices;
    }

    // One walk over the longer of the two counts resets both tables' live prefix.
    unsigned char nMax = m_nNumber3DVoices > m_nNumber2DVoices ? m_nNumber3DVoices
                                                              : m_nNumber2DVoices;
    for (uint64_t i = 0; i < nMax; i++)
    {
        if (i < m_nNumber3DVoices)
        {
            m_pa3DVoices[i].m_pSource = 0;
            m_pa3DVoices[i].m_nReserved1 = 0;
            m_pa3DVoices[i].m_nChannel = EXOSOUND_NO_CHANNEL;
            m_pa3DVoices[i].m_bInUse = 0;
        }
        if (i < m_nNumber2DVoices)
        {
            m_pa2DVoices[i].m_pSource = 0;
            m_pa2DVoices[i].m_nReserved1 = 0;
            m_pa2DVoices[i].m_nChannel = EXOSOUND_NO_CHANNEL;
            m_pa2DVoices[i].m_bInUse = 0;
        }
    }

    g_totalStreamingHandlesAvailable = m_nNumber2DVoices >> 1;
    g_generalStreamingHandlesAvailable = (m_nNumber2DVoices >> 1) - 8;
    m_environment = 0;

    while (lReaped.m_pInternal->m_nCount != 0)
    {
        CExoSoundSourceInternal* pReaped =
            static_cast<CExoSoundSourceInternal*>(lReaped.RemoveHead());
        pReaped->Play(1, 1);
    }

    CExoLinkedListNode* pNode = m_lStreaming.m_pInternal->m_pHead;
    CExoStreamingSoundSourceInternal* pStream = 0;
    if (m_lStreaming.m_pInternal->m_pHead != 0)
        pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->m_pHead->m_pData);

    while (pNode != 0)
    {
        pStream->InitializeSource();
        pStream->Resume();
        pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.GetNext(pNode));
    }

    return true;
}
#endif

void CExoSoundInternal::AddToDelayedOneShotList(CExoSoundSourceInternal* pSource)
{
    if (!m_lDelayedOneShots.m_pInternal->Contains(pSource))
    {
        pSource->m_nListState = 3;
        pSource->m_pListNode = m_lDelayedOneShots.m_pInternal->AddHead(pSource);
    }
}

CExoLinkedListNode* CExoSoundInternal::RemoveFromDelayedOneShotList(CExoSoundSourceInternal* pSource)
{
    CExoLinkedListNode* pNode = pSource->m_pListNode;
    m_lDelayedOneShots.GetNext(pNode);
    m_lDelayedOneShots.Remove(pSource->m_pListNode);
    pSource->m_nListState = 0;
    pSource->m_pListNode = 0;
    return pNode;
}

CExoLinkedListNode* CExoSoundInternal::RemoveFromNonPlayingLoopingList(CExoSoundSourceInternal* pSource)
{
    CExoLinkedListNode* pNode = pSource->m_pListNode;
    m_lNonPlayingLooping.GetNext(pNode);
    m_lNonPlayingLooping.Remove(pSource->m_pListNode);
    pSource->m_nListState = 0;
    pSource->m_pListNode = 0;
    return pNode;
}

void CExoSoundInternal::AddToNonPlayingLoopingList(CExoSoundSourceInternal* pSource)
{
    if (!m_lNonPlayingLooping.m_pInternal->Contains(pSource))
    {
        pSource->m_nListState = 2;
        pSource->m_pListNode = m_lNonPlayingLooping.m_pInternal->AddHead(pSource);
    }
}

// Sets the environment effect level and re-applies the volumes. A -1.0f argument passes through
// unclamped and leaves the level at -1.0f.
void CExoSoundInternal::SetEnvironmentEffectLevel(float f)
{
#if KOTOR1  // K1 returns early on the -1.0f pass-through
    if (f == -1.0f) {
        m_fEnvironmentEffectLevel = f;
        ApplyNewVolume(0);
        return;
    }
    f = f > 0.0f ? f : 0.0f;
    f = f < 1.0f ? f : 1.0f;
#else
    if (f != -1.0f)
    {
        f = f > 0.0f ? f : 0.0f;
        f = f < 1.0f ? f : 1.0f;
    }
#endif

    m_fEnvironmentEffectLevel = f;
    ApplyNewVolume(0);
}

// Mutes for 500 ms: the mute timer starts, the un-mute duration is cleared.
void CExoSoundInternal::MuteSound()
{
    if (m_bMuted)
        return;

    m_nUnMuteDuration = 0;
    m_bMuted = 1;
    m_nMuteDuration = 500;
    m_nMuteTimerStart = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer()) / 1000;
}

// Mirrors MuteSound: the un-mute duration starts and the mute duration is cleared.
void CExoSoundInternal::UnMuteSound()
{
    if (!m_bMuted)
        return;

    m_nMuteDuration = 0;
    m_bMuted = 0;
    m_nUnMuteDuration = 500;
    m_nMuteTimerStart = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer()) / 1000;
}

// Parks the stopped streaming source for a later re-start.
void CExoSoundInternal::AddToNonPlayingStreamingLoopingList(CExoStreamingSoundSourceInternal* pSource)
{
    m_lNonPlayingStreamingLooping.m_pInternal->AddHead(pSource);
}

void CExoSoundInternal::SetEnvironment(unsigned char env)
{
    m_environment = env;
}

// The per-frame mixer update.
void CExoSoundInternal::Render()
{
    CExoLinkedListNode* pNode;
    // The frame clock, the environment byte and the ambient-scale ramp.
    int nNow = static_cast<int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
    unsigned int nElapsed = static_cast<unsigned int>(nNow - m_nLastRenderTime);
    m_nLastRenderTime = nNow;

    float fDesired = m_fDesiredAmbientScale;
    if (m_nDesiredEnvironment != m_environment)
        m_environment = static_cast<unsigned char>(m_nDesiredEnvironment);

    if (fDesired != m_fAmbientScale)
    {
        float fStep = static_cast<float>(nElapsed) / 1000.0f;
        fStep = fStep < 0.1f ? fStep : 0.1f;

        m_fAmbientScale += fStep * (fDesired - m_fAmbientScaleRampStart);

        if (fDesired < m_fAmbientScale && fDesired > m_fAmbientScaleRampStart)
            m_fAmbientScale = fDesired;
        if (fDesired < m_fAmbientScaleRampStart)
            m_fAmbientScale = fDesired > m_fAmbientScale ? fDesired : m_fAmbientScale;
    }
    else
    {
        m_fAmbientScaleRampStart = m_fAmbientScale;
    }

    if (m_fLastDesiredAmbientScale != fDesired)
        m_fAmbientScaleRampStart = m_fAmbientScale;
    m_fLastDesiredAmbientScale = fDesired;

    // Reap streaming sources that are done, paused out or no longer looping.
    {
        pNode = m_lStreaming.m_pInternal->m_pHead;
        CExoStreamingSoundSourceInternal* pStream = 0;
        if (m_lStreaming.m_pInternal->m_pHead != 0)
            pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->m_pHead->m_pData);

        while (pNode != 0)
        {
            if (!pStream->IsPlaying() && !g_bSoundPaused && !pStream->m_bLooping)
            {
                RemoveFromStreamingList(pStream);
                break;
            }
            pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.GetNext(pNode));
        }
    }

    // The streaming 3D update scan.
    {
        pNode = m_lStreaming.m_pInternal->m_pHead;
        CExoStreamingSoundSourceInternal* pStream = 0;
        if (m_lStreaming.m_pInternal->m_pHead != 0)
            pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->m_pHead->m_pData);

        while (pNode != 0)
        {
            if (pStream->m_b3D != 0)
            {
                if (pStream->m_bLooping == 0)
                {
                    pStream->Update3DStream();
                }
                else if (!pStream->GetWithinRangeOfListener())
                {
                    m_lStreaming.GetNext(pNode);
                    pStream->Stop(1);
                    AddToNonPlayingStreamingLoopingList(pStream);
                    if (pNode == 0)
                        break;
                    pStream = static_cast<CExoStreamingSoundSourceInternal*>(
                        m_lStreaming.GetAtPos(pNode));
                    continue;
                }
                else if (pStream->m_b3D != 0)
                {
                    pStream->Update3DStream();
                }
            }
            pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.GetNext(pNode));
        }
    }

    // Of the parked non-playing streaming sources, start the first playable one.
    {
        pNode = m_lNonPlayingStreamingLooping.m_pInternal->m_pHead;
        CExoStreamingSoundSourceInternal* pStream = 0;
        if (m_lNonPlayingStreamingLooping.m_pInternal->m_pHead != 0)
            pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lNonPlayingStreamingLooping.m_pInternal->m_pHead->m_pData);

        while (pNode != 0)
        {
            CPriorityGroup* pGroup = GetPriorityGroup(pStream->m_nPriorityGroup);

            if (m_lStreaming.m_pInternal->m_nCount <
                    static_cast<unsigned int>(m_nNumber2DVoices - m_nNumber2DVoices / 2) &&
                (pGroup == 0 || pGroup->m_nPlayingCount < pGroup->m_nMaxPlaying) &&
                pStream->m_bLooping != 0 &&
                pStream->GetWithinRangeOfListener())
            {
                pStream->Update3DStream();
                pStream->Play(pStream->m_resRef, pStream->m_nResType, 0, 0, 0);

                // Unlink the entry just started; the walk resumes at its successor.
                pNode = RemoveFromNonPlayingStreamingLoopingList(pStream);
                if (pNode == 0)
                    break;
                pStream = static_cast<CExoStreamingSoundSourceInternal*>(
                    m_lNonPlayingStreamingLooping.GetAtPos(pNode));
                continue;
            }

            pStream = static_cast<CExoStreamingSoundSourceInternal*>(
                m_lNonPlayingStreamingLooping.GetNext(pNode));
        }
    }

    // The mute-in ramp takes the dialog, music and effect volumes down toward zero.
    unsigned int nMuteNow = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer()) / 1000;
    if (m_nMuteDuration != 0)
    {
        unsigned int nDiff = nMuteNow - m_nMuteTimerStart;
        if (nDiff != 0)
        {
            float f = static_cast<float>(nDiff) / static_cast<float>(m_nMuteDuration);
            if (f > 1.0f)
                f = 1.0f;

            m_fDialogVolumeRaw = m_fDialogVolume - f * m_fDialogVolume;
            m_fMusicVolumeRaw = m_fMusicVolume - f * m_fMusicVolume;
            m_fSoundEffectVolumeRaw = m_fSoundEffectVolume - f * m_fSoundEffectVolume;
            ApplyNewVolume(1);

            if (f == 1.0f)
            {
                m_nMuteDuration = 0;
                m_nMuteTimerStart = 0;

                pNode = m_lStreaming.m_pInternal->m_pHead;
                CExoStreamingSoundSourceInternal* pStream = 0;
                if (m_lStreaming.m_pInternal->m_pHead != 0)
                    pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->m_pHead->m_pData);

                while (pNode != 0)
                {
                    if (pStream->m_nFadeDuration != 0)
                    {
                        pNode = RemoveFromStreamingList(pStream);
                        pStream->Stop(0);
                        if (pNode == 0)
                            break;
                        pStream = static_cast<CExoStreamingSoundSourceInternal*>(
                            m_lStreaming.GetAtPos(pNode));
                        continue;
                    }
                    pStream = static_cast<CExoStreamingSoundSourceInternal*>(
                        m_lStreaming.GetNext(pNode));
                }
            }
        }
    }

    // The un-mute ramp brings the same three volumes back up.
    if (m_nUnMuteDuration != 0)
    {
        unsigned int nDiff = nMuteNow - m_nMuteTimerStart;
        if (nDiff != 0)
        {
            float f = static_cast<float>(nDiff) / static_cast<float>(m_nUnMuteDuration);
            if (f > 1.0f)
                f = 1.0f;

            m_fDialogVolumeRaw = f * m_fDialogVolume;
            m_fMusicVolumeRaw = m_fMusicVolume * f;
            m_fSoundEffectVolumeRaw = m_fSoundEffectVolume * f;
            ApplyNewVolume(1);

            if (f == 1.0f)
            {
                m_nUnMuteDuration = 0;
                m_nMuteTimerStart = 0;
            }
        }
    }

    // The delayed one-shot countdown.
    unsigned int nLowRes = g_pExoBase->m_pTimers->GetLowResolutionTimer();
    {
        pNode = m_lDelayedOneShots.m_pInternal->m_pHead;
        CExoSoundSourceInternal* pSource = 0;
        if (m_lDelayedOneShots.m_pInternal->m_pHead != 0)
            pSource = static_cast<CExoSoundSourceInternal*>(m_lDelayedOneShots.m_pInternal->m_pHead->m_pData);

        while (pNode != 0)
        {
            unsigned int nDiff = pSource->m_nOneShotDelayTimer > nLowRes
                ? ~(pSource->m_nOneShotDelayTimer - nLowRes)
                : nLowRes - pSource->m_nOneShotDelayTimer;
            if (nDiff == 0 || nDiff < pSource->m_nOneShotDelay)
            {
                pSource = static_cast<CExoSoundSourceInternal*>(m_lDelayedOneShots.GetNext(pNode));
                continue;
            }

            pNode = RemoveFromDelayedOneShotList(pSource);
            pSource->Play(0, 0);
            if (pSource->m_bOneShotFailed != 0)
                delete pSource;
            if (pNode == 0)
                break;
            // The walk advances through m_lPlaying rather than the list just removed from.
            pSource = static_cast<CExoSoundSourceInternal*>(m_lPlaying.GetAtPos(pNode));
        }
    }

    // The streaming volume ramp and fade-out, only while not muted.
    if (m_bMuted == 0)
    {
        pNode = m_lStreaming.m_pInternal->m_pHead;
        CExoStreamingSoundSourceInternal* pStream = 0;
        if (m_lStreaming.m_pInternal->m_pHead != 0)
            pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->m_pHead->m_pData);

        while (pNode != 0)
        {
            if (m_fAmbientScale != m_fAmbientScaleRampStart)
                pStream->SetVolume(pStream->m_nVolume, 0, m_fAmbientScale);

            if (pStream->m_nFadeDuration != 0)
            {
                unsigned int nDiff = pStream->m_nFadeStartTime > nLowRes
                    ? ~(pStream->m_nFadeStartTime - nLowRes)
                    : nLowRes - pStream->m_nFadeStartTime;
                if (nDiff != 0)
                {
                    if (nDiff < pStream->m_nFadeDuration)
                    {
                        // The fade ratio is computed in double precision.
                        unsigned char nVolume = pStream->m_nVolume;
                        unsigned char nFaded = static_cast<unsigned char>(nVolume -
                            static_cast<int>(static_cast<double>(nDiff) / static_cast<double>(pStream->m_nFadeDuration) * static_cast<double>(nVolume)));
                        pStream->SetVolume(nFaded, 1, 1.0f);
                    }
                    else
                    {
                        pNode = RemoveFromStreamingList(pStream);
                        pStream->Stop(0);
                        if (pNode == 0)
                            break;
                        pStream = static_cast<CExoStreamingSoundSourceInternal*>(
                            m_lStreaming.GetAtPos(pNode));
                        continue;
                    }
                }
            }
            pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.GetNext(pNode));
        }
    }

    // The playing-list pass.
    {
        pNode = m_lPlaying.m_pInternal->m_pHead;
        CExoSoundSourceInternal* pSource = 0;
        if (m_lPlaying.m_pInternal->m_pHead != 0)
            pSource = static_cast<CExoSoundSourceInternal*>(m_lPlaying.m_pInternal->m_pHead->m_pData);

        while (pNode != 0)
        {
            if (pSource->IsPlaying())
            {
                if (pSource->GetWithinRangeOfListener())
                {
                    if (m_fAmbientScale != m_fAmbientScaleRampStart)
                        pSource->SetVolume(pSource->m_nVolume, 0, m_fAmbientScale);
                    pSource = static_cast<CExoSoundSourceInternal*>(m_lPlaying.GetNext(pNode));
                    continue;
                }

                // Still playing but out of range, so it leaves the playing list.
                pNode = RemoveFromPlayingList(pSource);
                if (pSource->m_bOneShot)
                {
                    bool bPlaying = pSource->IsPlaying() != 0;
                    bool bInList = m_lStoppingOneShots.m_pInternal->Contains(pSource) != 0;
                    if (bPlaying)
                    {
                        if (!bInList)
                            m_lStoppingOneShots.AddHead(pSource);
                    }
                    else if (!bInList)
                    {
                        delete pSource;
                    }
                }
                else
                {
                    pSource->ShutDownSource(false);
                    if (pSource->m_bLooping != 0)
                        AddToNonPlayingLoopingList(pSource);
                }
            }
            else
            {
                pNode = RemoveFromPlayingList(pSource);
                if (pSource->m_bOneShot)
                {
                    if (!m_lStoppingOneShots.m_pInternal->Contains(pSource))
                        delete pSource;
                }
                else if (!pSource->IsPaused())
                {
                    pSource->ShutDownSource(false);
                }
            }

            if (pNode == 0)
                break;
            pSource = static_cast<CExoSoundSourceInternal*>(m_lPlaying.GetAtPos(pNode));
        }
    }

    // Re-start the parked non-playing looping sources.
    {
        pNode = m_lNonPlayingLooping.m_pInternal->m_pHead;
        CExoSoundSourceInternal* pSource = 0;
        if (m_lNonPlayingLooping.m_pInternal->m_pHead != 0)
            pSource = static_cast<CExoSoundSourceInternal*>(m_lNonPlayingLooping.m_pInternal->m_pHead->m_pData);

        while (pNode != 0)
        {
            m_lNonPlayingLooping.GetNext(pNode);
            pSource->Play(1, 0);
            if (pNode == 0)
                break;
            pSource = static_cast<CExoSoundSourceInternal*>(m_lNonPlayingLooping.GetAtPos(pNode));
        }
    }

    // The listener state is zeroed and then re-applied, so the zeroing cannot be observed.
    {
        Vector vPos = m_vListenerPosition;
        Vector vDir = m_vListenerOrientation;
        Vector vUp = m_vListenerUp;
        m_vListenerPosition = m_vListenerOrientation = m_vListenerUp = Vector(0.0f, 0.0f, 0.0f);

        SetListenerPosition(vPos);
        SetListenerOrientation(vDir, vUp);
    }

    // Reap the stopped one-shots.
    {
        pNode = m_lStoppingOneShots.m_pInternal->m_pHead;
        while (pNode != 0)
        {
            CExoSoundSourceInternal* pSource = static_cast<CExoSoundSourceInternal*>(
                m_lStoppingOneShots.GetAtPos(pNode));
            if (!pSource->IsPlaying())
            {
                CExoLinkedListNode* pCur = pNode;
                m_lStoppingOneShots.GetNext(pNode);
                m_lStoppingOneShots.Remove(pCur);
                if (pSource != 0)
                    delete pSource;
            }
            else
            {
                m_lStoppingOneShots.GetNext(pNode);
            }
        }
    }

    // The time-budgeted occlusion walk, with this frame's delta.
    OcclusionRender(nElapsed);
}

#if !KOTOR1
CPriorityGroup* CExoSoundInternal::GetPriorityGroup(unsigned char group)
{
    unsigned int idx = m_maxGroup > group ? group : 23;
    return &m_paGroups[idx];
}
#endif

void CExoSoundInternal::SetRollOffFactor(float f) { K2_UNUSED(f); }

void CExoSoundInternal::Enumerate3DProviders() {}

void CExoSoundInternal::LoadProviderPreferences() {}

int CExoSoundInternal::GetSpeakerType() { return SPEAKER_TYPE_UNKNOWN; }

int CExoSoundInternal::GetMutable(CExoStreamingSoundSourceInternal* pSource) { K2_UNUSED(pSource); return 1; }

int CExoSoundInternal::GetMutable(CExoSoundSourceInternal* pSource) { K2_UNUSED(pSource); return 1; }

#if !KOTOR1  // K1 has no music/ambient ducking
void CExoSoundInternal::StartDuckingMusicAndAmbientSounds()
{
    m_fDuckVolume = 0.2f;
    ApplyNewVolume(0);
}
#endif

#if !KOTOR1  // K1 has no music/ambient ducking
void CExoSoundInternal::StopDuckingMusicAndAmbientSounds()
{
    m_fDuckVolume = 1.0f;
    ApplyNewVolume(0);
}
#endif


void CExoSoundInternal::SetDialogVolume(float fVolume, int a)
{
    if (a == 0)
        m_fDialogVolume = fVolume;
    m_fDialogVolumeRaw = fVolume;
}

void CExoSoundInternal::SetSoundEffectVolume(float fVolume, int a)
{
    if (a == 0)
        m_fSoundEffectVolume = fVolume;
    m_fSoundEffectVolumeRaw = fVolume;
}

void CExoSoundInternal::SetMusicVolume(float fVolume, int a, int b)
{
    if (a == 0)
        m_fMusicVolume = fVolume;
    m_fMusicVolumeRaw = fVolume;

    if (b == 1)
    {
        CPriorityGroup* pGroup = GetPriorityGroup(PRIORITY_GROUP_STREAMS_MUSIC);

        float fScaled = fVolume * 127.0f;
        if (fScaled < 0.0f)
            fScaled = 0.0f;
#if KOTOR1  // K1 caps the converted integer at 127
        int nScaled = static_cast<int>(fScaled);
        if (nScaled > 127)
            nScaled = 127;

        pGroup->m_nVolume = static_cast<unsigned char>(nScaled);
#else
        if (fScaled > 127.0f)
            fScaled = 127.0f;

        pGroup->m_nVolume = static_cast<unsigned char>(static_cast<int>(fScaled));
#endif
    }
}

void CExoSoundInternal::AddToPlayingList(CExoSoundSourceInternal* pSource)
{
    if (!m_lPlaying.m_pInternal->Contains(pSource))
    {
        pSource->m_nListState = 1;
        pSource->m_pListNode = m_lPlaying.m_pInternal->AddHead(pSource);

        CPriorityGroup* pGroup = GetPriorityGroup(pSource->m_nPriorityGroup);
#if KOTOR1  // K1 has no null test on the group
        pGroup->m_nPlayingCount++;
#else
        if (pGroup)
            pGroup->m_nPlayingCount++;
#endif
    }
}

int CExoSoundInternal::AddToStreamingList(CExoStreamingSoundSourceInternal* pSource)
{
    if (m_lStreaming.m_pInternal->m_nCount >=
        static_cast<unsigned int>(m_nNumber2DVoices - m_nNumber2DVoices / 2))
    {
        CPriorityGroup* pGroup = GetPriorityGroup(pSource->m_nPriorityGroup);
        if (pGroup == 0)
            return 0;

        CExoStreamingSoundSourceInternal* pBest = 0;
        unsigned char nBestScore = 0;

        CExoLinkedListNode* pNode = m_lStreaming.m_pInternal->m_pHead;
        CExoStreamingSoundSourceInternal* pObj = 0;
        if (m_lStreaming.m_pInternal->m_pHead != 0)
            pObj = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->m_pHead->m_pData);

        for (; pNode != 0;
             pObj = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.GetNext(pNode)))
        {
            if (pObj->m_bLooping)
            {
                CPriorityGroup* pSourceGroup = GetPriorityGroup(pObj->m_nPriorityGroup);
                if (pSourceGroup != 0)
                {
                    unsigned char nScore = pSourceGroup->m_nPriority;
                    if (nScore > pGroup->m_nPriority && nScore > nBestScore)
                    {
                        nBestScore = nScore;
                        pBest = pObj;
                    }
                }
            }
        }

        if (pBest == 0)
            return 0;

        pBest->Stop(0);
        if (pBest->m_bLooping)
            AddToNonPlayingStreamingLoopingList(pBest);
    }

    pSource->m_pListNode = m_lStreaming.AddHead(pSource);

    CPriorityGroup* pGroup = GetPriorityGroup(pSource->m_nPriorityGroup);
#if KOTOR1  // K1 has no null test on the group
    pGroup->m_nPlayingCount++;
#else
    if (pGroup)
        pGroup->m_nPlayingCount++;
#endif

    return 1;
}

int CExoSoundInternal::GetSliderType(CExoStreamingSoundSourceInternal* pSource)
{
    unsigned char nGroup = pSource->m_nPriorityGroup;

    if (nGroup == 8 || nGroup == 9 || nGroup == 16 || nGroup == 17 || nGroup == 26)
        return 1;
    if (nGroup == 1)
        return 3;
    return nGroup == 2 ? 3 : 2;
}

int CExoSoundInternal::GetSliderType(CExoSoundSourceInternal* pSource)
{
    unsigned char nGroup = pSource->m_nPriorityGroup;

    if (nGroup == 8 || nGroup == 9 || nGroup == 16 || nGroup == 17 || nGroup == 26)
        return 1;
    if (nGroup == 1)
        return 3;
    return nGroup == 2 ? 3 : 2;
}

float CExoSoundInternal::GetSliderVolume(int nSliderType, int nRaw)
{
    if (nSliderType == 1)
        return nRaw ? m_fDialogVolumeRaw : m_fDialogVolume;
    if (nSliderType == 2)
        return nRaw ? m_fSoundEffectVolumeRaw : m_fSoundEffectVolume;
    if (nSliderType == 3)
        return nRaw ? m_fMusicVolumeRaw : m_fMusicVolume;
    return 1.0f;
}

void CExoSoundInternal::SetListenerPosition(const Vector& vPos)
{
    if (equal(m_vListenerPosition, vPos))
        return;

    m_vListenerPosition = vPos;
    m_audioSystem.Set3DListenerPosition(vPos.x, vPos.y, -vPos.z);
}

void CExoSoundInternal::SetListenerOrientation(const Vector& vDir, const Vector& vUp)
{
    if (equal(m_vListenerOrientation, vDir) && equal(m_vListenerUp, vUp))
        return;

    m_vListenerOrientation = vDir;
    m_vListenerUp = vUp;
    m_audioSystem.Set3DListenerOrientation(vDir.x, vDir.y, -vDir.z, vUp.x, vUp.y, -vUp.z);
}

void CExoSoundInternal::OcclusionRender(unsigned int nMaxTime)
{
    CExoLinkedListNode* pStartNode = m_pOcclusionCursor;
    unsigned int nLastTime = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
    unsigned int nNow;

    m_nOcclusionIdleFrames++;

    if (m_pOcclusionCursor == 0)
    {
        m_pOcclusionCursor = m_lPlaying.GetHeadPos();
        m_nOcclusionCursorList = 1;
        if (m_pOcclusionCursor == 0)
            m_nOcclusionIdleFrames = 0;
    }

    if (m_nOcclusionTimeUsed > m_nOcclusionTimeUsedMax)
        m_nOcclusionTimeUsedMax = m_nOcclusionTimeUsed;

    bool bFirstPass = true;

    while (m_nOcclusionTimeUsed < m_nOcclusionTimeBudget)
    {
        if (m_nOcclusionCursorList == 1)
        {
            CExoLinkedListNode* pNode = m_pOcclusionCursor;

            while (pNode != 0)
            {
                if (m_nOcclusionTimeUsed >= m_nOcclusionTimeBudget)
                    break;

                m_nOcclusionIdleFrames = 0;

                CExoSoundSourceInternal* pSource = static_cast<CExoSoundSourceInternal*>(
                    m_lPlaying.GetAtPos(pNode));

                if (pSource != 0 && pSource->IsPlaying() && pSource->m_b3D &&
                    !pSource->m_bOneShot)
                {
                    Vector vSourcePos = pSource->m_vPosition;
                    Vector vClosest = m_vListenerPosition;
                    unsigned int nTime = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
                    float fFactor = 0.0f;

                    if (GetClosestRecentSound(vSourcePos, vClosest, nTime, fFactor))
                    {
                        if (m_nOcclusionMaxTimeHigh < nMaxTime)
                            m_nOcclusionMaxTimeHigh = nMaxTime;

                        pSource->m_vOcclusionPosition = vClosest;
                        pSource->m_fOcclusionFactor = fFactor;
                        pSource->m_nOcclusionTime = nTime;
                    }
                    else
                    {
                        fFactor = GetObstructionFactor(vSourcePos, vClosest);
                        pSource->m_vOcclusionPosition = vClosest;
                        pSource->m_fOcclusionFactor = fFactor;
                        pSource->m_nOcclusionTime = nTime;

                        if (++m_nObstructionTests > m_nObstructionTestsMax)
                            m_nObstructionTestsMax = m_nObstructionTests;
                    }

                    g_pExoBase->m_pTimers->GetHighResolutionTimer();
                    pSource->DoObstructionWork();
                }

                m_lPlaying.GetNext(pNode);

                nNow = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
                m_nOcclusionTimeUsed += static_cast<int>(nNow - nLastTime);
                nLastTime = nNow;

                if (pNode == pStartNode)
                {
                    m_pOcclusionCursor = pStartNode;
                    m_nObstructionTests = 0;
                    goto done;
                }
            }

            if (pNode == 0)
            {
                m_nOcclusionCursorList = 2;
                pNode = m_lStreaming.m_pInternal->m_pHead;
            }

            m_pOcclusionCursor = pNode;

            nNow = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
            m_nOcclusionTimeUsed += static_cast<int>(nNow - nLastTime);
            nLastTime = nNow;
        }

        if (!bFirstPass && m_pOcclusionCursor == pStartNode)
        {
            m_nObstructionTests = 0;
            goto done;
        }

        if (m_nOcclusionCursorList == 2)
        {
            CExoLinkedListNode* pNode = m_pOcclusionCursor;

            while (pNode != 0)
            {
                if (m_nOcclusionTimeUsed >= m_nOcclusionTimeBudget)
                    break;

                m_nOcclusionIdleFrames = 0;

                CExoStreamingSoundSourceInternal* pStream =
                    static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.GetAtPos(pNode));

                if (pStream != 0 && pStream->IsPlaying() && pStream->m_b3D)
                {
                    Vector vSourcePos = pStream->m_vPosition;
                    Vector vClosest = m_vListenerPosition;
                    unsigned int nTime = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
                    float fFactor = 0.0f;

                    if (GetClosestRecentSound(vSourcePos, vClosest, nTime, fFactor))
                    {
                        if (m_nOcclusionMaxTimeHigh < nMaxTime)
                            m_nOcclusionMaxTimeHigh = nMaxTime;

                        pStream->m_vOcclusionPosition = vClosest;
                        pStream->m_fOcclusionFactor = fFactor;
                        pStream->m_nOcclusionTime = nTime;
                    }
                    else
                    {
                        fFactor = GetObstructionFactor(vSourcePos, vClosest);
                        pStream->m_vOcclusionPosition = vClosest;
                        pStream->m_fOcclusionFactor = fFactor;
                        pStream->m_nOcclusionTime = nTime;

                        if (++m_nObstructionTests > m_nObstructionTestsMax)
                            m_nObstructionTestsMax = m_nObstructionTests;
                    }

                    g_pExoBase->m_pTimers->GetHighResolutionTimer();
                    pStream->DoObstructionWork();
                }

                m_lStreaming.GetNext(pNode);

                nNow = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
                m_nOcclusionTimeUsed += static_cast<int>(nNow - nLastTime);
                nLastTime = nNow;

                if (pNode == pStartNode)
                {
                    m_pOcclusionCursor = pStartNode;
                    m_nObstructionTests = 0;
                    goto done;
                }
            }

            if (pNode == 0)
            {
                m_nOcclusionCursorList = 1;
                pNode = m_lPlaying.m_pInternal->m_pHead;
            }

            m_pOcclusionCursor = pNode;

            nNow = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
            m_nOcclusionTimeUsed += static_cast<int>(nNow - nLastTime);
            nLastTime = nNow;
        }

        bFirstPass = false;
        m_nObstructionTests = 0;

        if (m_pOcclusionCursor == pStartNode)
            goto done;
    }

done:
    if (m_nOcclusionIdleFrames > m_nOcclusionIdleFramesMax)
        m_nOcclusionIdleFramesMax = m_nOcclusionIdleFrames;

    m_nOcclusionTimeUsed = 0;
}

void CExoSoundInternal::PlaySourceOn2DVoice(CExoSoundSourceInternal* pSource, C2DVoice* pVoice)
{
    // Read before the two pointer stores below, which may alias this flag.
    int bLoop = !pSource->m_bLooping;

    pSource->m_p2DVoice = pVoice;
    pVoice->m_pSource = pSource;

    pVoice->m_nChannel = m_audioSystem.PlaySoundA(pSource->m_pRes->GetID());

    m_audioSystem.PauseChannel(pVoice->m_nChannel);
    m_audioSystem.SetChannelLoopCount(pVoice->m_nChannel, bLoop);
    m_audioSystem.SetChannelPlaybackPosition(pVoice->m_nChannel, pSource->m_nPlaybackPosition);

    if (pSource->m_fPitchVariance != 0.0f)
        m_audioSystem.SetChannelFrequency(pVoice->m_nChannel, static_cast<float>(pSource->m_nPitchVarianceFrequency));
    else if (pSource->m_fFixedVariance != 0.0f)
        m_audioSystem.SetChannelFrequency(pVoice->m_nChannel, static_cast<float>(pSource->m_frequency));
    else
        m_audioSystem.SetChannelFrequency(pVoice->m_nChannel, static_cast<float>(pSource->m_baseFrequency));

    pSource->SetVolume(pSource->m_nVolume, 0, m_fAmbientScale);
    m_audioSystem.ResumeChannel(pVoice->m_nChannel);
}


int CExoSoundInternal::GetClosestRecentSound(const Vector& vSourcePos, Vector& vClosestPos,
                                            unsigned int& nTime, float& fFactor)
{
    Vector vSource = vSourcePos;
    Vector vClosest = vClosestPos;
    unsigned int nStart = nTime;

    CExoLinkedListNode* pNode = m_lPlaying.m_pInternal->m_pHead;
    while (pNode)
    {
        CExoSoundSourceInternal* pSource =
            static_cast<CExoSoundSourceInternal*>(m_lPlaying.GetAtPos(pNode));

        if (pSource && pSource->m_b3D)
        {
            Vector vPosition = pSource->m_vPosition;
            Vector vOcclusion = pSource->m_vOcclusionPosition;
            unsigned int nOcclusionTime = pSource->m_nOcclusionTime;
            if (magnitude(vSource - vPosition) < 0.5f
                && magnitude(vClosest - vOcclusion) < 0.5f
                && nOcclusionTime - nStart <= 174)
            {
                vClosestPos = vOcclusion;
                nTime = nOcclusionTime;
                fFactor = pSource->m_fOcclusionFactor;
                return 1;
            }
        }

        m_lPlaying.GetNext(pNode);
    }

    pNode = m_lStreaming.m_pInternal->m_pHead;
    while (pNode)
    {
        CExoStreamingSoundSourceInternal* pSource =
            static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.GetAtPos(pNode));

        if (pSource && pSource->m_b3D)
        {
            Vector vPosition = pSource->m_vPosition;
            Vector vOcclusion = pSource->m_vOcclusionPosition;
            unsigned int nOcclusionTime = pSource->m_nOcclusionTime;
            if (magnitude(vSource - vPosition) < 0.5f
                && magnitude(vClosest - vOcclusion) < 0.5f
                && nOcclusionTime - nStart <= 174)
            {
                vClosestPos = vOcclusion;
                nTime = nOcclusionTime;
                fFactor = pSource->m_fOcclusionFactor;
                return 1;
            }
        }

        m_lStreaming.GetNext(pNode);
    }

    return 0;
}


void CExoSoundInternal::PlaySourceOn3DVoice(CExoSoundSourceInternal* pSource, C3DVoice* pVoice)
{
    // As in the 2D twin, the flag is read before the pointer stores, which may alias it.
    int bLoop = !pSource->m_bLooping;

    pVoice->m_pSource = pSource;
    Vector vPos = pSource->m_vPosition;
    float fMaxDistance = pSource->m_fMaxDistance;
    float fMinDistance = pSource->m_fMinDistance;
    pSource->m_p3DVoice = pVoice;

    pVoice->m_nChannel = m_audioSystem.PlaySoundA(pSource->m_pRes->GetID());

    m_audioSystem.PauseChannel(pVoice->m_nChannel);
    m_audioSystem.SetChannelLoopCount(pVoice->m_nChannel, bLoop);

    pSource->SetVolume(pSource->m_nVolume, 0, 1.0f);

    m_audioSystem.SetChannelDistances(pVoice->m_nChannel, fMaxDistance, fMinDistance);
    m_audioSystem.SetChannelPlaybackPosition(pVoice->m_nChannel, pSource->m_nPlaybackPosition);
    m_audioSystem.SetChannel3DPosition(pVoice->m_nChannel, vPos.x, vPos.y, -vPos.z);

    if (pSource->m_fPitchVariance != 0.0f)
        m_audioSystem.SetChannelFrequency(pVoice->m_nChannel, static_cast<float>(pSource->m_nPitchVarianceFrequency));
    else if (pSource->m_fFixedVariance != 0.0f)
        m_audioSystem.SetChannelFrequency(pVoice->m_nChannel, static_cast<float>(pSource->m_frequency));
    else
        m_audioSystem.SetChannelFrequency(pVoice->m_nChannel, static_cast<float>(pSource->m_baseFrequency));

    Vector vSourcePos = pSource->m_vPosition;
    Vector vClosest = m_vListenerPosition;
    unsigned int nTime = static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000);
    float fFactor = 0.0f;

    if (GetClosestRecentSound(vSourcePos, vClosest, nTime, fFactor))
    {
        pSource->m_vOcclusionPosition = vClosest;
        pSource->m_fOcclusionFactor = fFactor;
        pSource->m_nOcclusionTime = nTime;
    }
    else
    {
        fFactor = GetObstructionFactor(vSourcePos, vClosest);
        pSource->m_vOcclusionPosition = vClosest;
        pSource->m_fOcclusionFactor = fFactor;
        pSource->m_nOcclusionTime = nTime;

        if (++m_nObstructionTests > m_nObstructionTestsMax)
            m_nObstructionTestsMax = m_nObstructionTests;
    }

    pSource->DoObstructionWork();

    m_nOcclusionTimeUsed += static_cast<int>(static_cast<unsigned int>(g_pExoBase->m_pTimers->GetHighResolutionTimer() / 1000) - nTime);
    m_audioSystem.ResumeChannel(pVoice->m_nChannel);
}


int CExoSoundInternal::GetNumber2DSamplesPlaying()
{
    uint64_t i = 0;
    int nPlaying = 0;
    for (; i < m_nNumber2DVoices; i++)
    {
        CExoSoundSourceInternal* pSource = m_pa2DVoices[i].m_pSource;
        if (pSource && pSource->IsPlaying())
            nPlaying++;
    }
    return nPlaying;
}

int CExoSoundInternal::GetNumber3DSamplesPlaying()
{
    uint64_t i = 0;
    int nPlaying = 0;
    for (; i < m_nNumber3DVoices; i++)
    {
        CExoSoundSourceInternal* pSource = m_pa3DVoices[i].m_pSource;
        if (pSource && pSource->IsPlaying())
            nPlaying++;
    }
    return nPlaying;
}

int CExoSoundInternal::GetPauseable(CExoStreamingSoundSourceInternal* pSource)
{
    int nMode = m_aSoundModes[m_nSoundModeIndex];

    if (nMode == CExoSound::CEXOSOUND_MODE_PAUSE)
    {
        switch (pSource->m_nPriorityGroup)
        {
        case PRIORITY_GROUP_MUSIC_STINGERS:
        case PRIORITY_GROUP_STREAMS_MUSIC:
        case PRIORITY_GROUP_LOOPING_AREA_WIDE_AMBIENTS:
        case PRIORITY_GROUP_GUI:
            return 0;
        }
    }
    else if (nMode == CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC)
    {
        switch (pSource->m_nPriorityGroup)
        {
        case PRIORITY_GROUP_MUSIC_STINGERS:
        case PRIORITY_GROUP_STREAMS_MUSIC:
        case PRIORITY_GROUP_GUI:
            return 0;
        }
    }
    return 1;
}

int CExoSoundInternal::GetPauseable(CExoSoundSourceInternal* pSource)
{
    int nMode = m_aSoundModes[m_nSoundModeIndex];

    if (nMode == CExoSound::CEXOSOUND_MODE_PAUSE)
    {
        switch (pSource->m_nPriorityGroup)
        {
        case PRIORITY_GROUP_MUSIC_STINGERS:
        case PRIORITY_GROUP_STREAMS_MUSIC:
        case PRIORITY_GROUP_LOOPING_AREA_WIDE_AMBIENTS:
        case PRIORITY_GROUP_GUI:
            return 0;
        }
    }
    else if (nMode == CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC)
    {
        switch (pSource->m_nPriorityGroup)
        {
        case PRIORITY_GROUP_MUSIC_STINGERS:
        case PRIORITY_GROUP_STREAMS_MUSIC:
        case PRIORITY_GROUP_GUI:
            return 0;
        }
    }
    return 1;
}

int CExoSoundInternal::Test2D3DBias()
{
    PlayOneShotSound(CResRef("soundbias_test"), 11, 250, 0, 0.0f, 0.0f);
    return Play3DOneShotSound(CResRef("soundbias_test"), m_vListenerPosition, 11, 1000, 0,
                             0.0f, 0.0f);
}

// The list walks below load the head node and its payload separately, then stop on the node rather
// than on the payload.

#if !KOTOR1  // K1 has no streaming-music-for-movie flag
void CExoSoundInternal::ResumeAllStreamingMusicForMovie()
{
    g_bSoundPaused = 0;

    CExoLinkedListNode* pNode = m_lStreaming.m_pInternal->m_pHead;
    CExoStreamingSoundSourceInternal* pStream = 0;
    if (m_lStreaming.m_pInternal->m_pHead != 0)
        pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->m_pHead->m_pData);

    while (pNode != 0)
    {
        if (pStream->m_bStreamingMusicForMovie)
            pStream->Resume();
        pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->GetNext(pNode));
    }
}
#endif

void CExoSoundInternal::InterruptPlayingStream(unsigned char nGroup)
{
    CExoLinkedListNode* pNode = m_lStreaming.m_pInternal->m_pHead;
    CExoStreamingSoundSourceInternal* pStream = 0;
    if (m_lStreaming.m_pInternal->m_pHead != 0)
        pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->m_pHead->m_pData);

    while (pNode != 0)
    {
        if (pStream->m_nPriorityGroup == nGroup)
        {
            pStream->Stop(0);
            if (pStream->m_bLooping)
                AddToNonPlayingStreamingLoopingList(pStream);
            break;
        }
        pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->GetNext(pNode));
    }
}

int CExoSoundInternal::ResumeAllSounds()
{
    g_bSoundPaused = 0;

    CExoLinkedListNode* pNode = m_lStreaming.m_pInternal->m_pHead;
    CExoStreamingSoundSourceInternal* pStream = 0;
    if (m_lStreaming.m_pInternal->m_pHead != 0)
        pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->m_pHead->m_pData);

    while (pNode != 0)
    {
        pStream->Resume();
        pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->GetNext(pNode));
    }

    CExoSoundSourceInternal* pSource = 0;
    pNode = m_lPlaying.m_pInternal->m_pHead;
    if (m_lPlaying.m_pInternal->m_pHead != 0)
        pSource = static_cast<CExoSoundSourceInternal*>(m_lPlaying.m_pInternal->m_pHead->m_pData);

    while (pNode != 0)
    {
        pSource->Resume();
        pSource = static_cast<CExoSoundSourceInternal*>(m_lPlaying.m_pInternal->GetNext(pNode));
    }

    return 1;
}

void CExoSoundInternal::ApplyNewVolume(int a)
{
    CExoLinkedListNode* pNode = m_lStreaming.m_pInternal->m_pHead;
    CExoStreamingSoundSourceInternal* pStream = 0;
    if (m_lStreaming.m_pInternal->m_pHead != 0)
        pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->m_pHead->m_pData);

    while (pNode != 0)
    {
        if (pStream != 0)
            pStream->SetVolume(pStream->m_nVolume, a, 1.0f);
        pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->GetNext(pNode));
    }

    CExoSoundSourceInternal* pSource = 0;
    pNode = m_lPlaying.m_pInternal->m_pHead;
    if (m_lPlaying.m_pInternal->m_pHead != 0)
        pSource = static_cast<CExoSoundSourceInternal*>(m_lPlaying.m_pInternal->m_pHead->m_pData);

    while (pNode != 0)
    {
        if (pSource != 0)
            pSource->SetVolume(pSource->m_nVolume, a, 1.0f);
        pSource = static_cast<CExoSoundSourceInternal*>(m_lPlaying.m_pInternal->GetNext(pNode));
    }
}

int CExoSoundInternal::PlayOneShotSound(const CResRef& resRef, unsigned char nPriorityGroup,
                                       unsigned int nDelay, unsigned char nVolume,
                                       float fFixedVariance, float fPitchVariance)
{
    if (resRef == const_cast<char*>(""))
        return 0;

    CExoSoundSourceInternal* pSource = new CExoSoundSourceInternal(this);

    pSource->SetResRef(resRef, 0);
    pSource->m_bOneShot = 1;
    pSource->SetPriorityGroup(nPriorityGroup);

    if (nVolume)
        pSource->SetVolume(nVolume, 0, 1.0f);
    if (fFixedVariance != 0.0f)
        pSource->SetFixedVariance(fFixedVariance);
    if (fPitchVariance != 0.0f)
        pSource->SetPitchVariance(fPitchVariance);

    int nResult;
    if (nDelay != 0)
    {
        pSource->SetOneShotDelay(nDelay);
        nResult = 0;
    }
    else
    {
        nResult = pSource->Play(0, 0);
        if (pSource->m_bOneShotFailed)
            delete pSource;
    }

    return nResult;
}

int CExoSoundInternal::Play3DOneShotSound(const CResRef& resRef, Vector vPosition,
                                          unsigned char nPriorityGroup, unsigned int nDelay,
                                          unsigned char nVolume, float fFixedVariance,
                                          float fPitchVariance)
{
    if (resRef == const_cast<char*>(""))
        return 0;

    const char* sResRef = resRef.GetResRefStr();
    char cFirst = static_cast<char>(toupper(sResRef[0]));

    int nResult;
    if ((cFirst == 'C' || cFirst == 'P' || cFirst == 'N') && sResRef[1] == '_')
    {
        if (m_pStreamingSoundSource != 0)
        {
            m_pStreamingSoundSource->Stop(0);
            delete m_pStreamingSoundSource;
        }

        m_pStreamingSoundSource = new CExoStreamingSoundSource(0);
        m_pStreamingSoundSource->SetPriorityGroup(nPriorityGroup);
        m_pStreamingSoundSource->Set3D(1);
        m_pStreamingSoundSource->SetPosition(vPosition);
        if (nVolume)
            m_pStreamingSoundSource->SetVolume(nVolume);

        CPriorityGroup* pGroup = GetPriorityGroup(nPriorityGroup);
        float fMinDistance = pGroup->m_fMinDistance;
        float fMaxDistance = pGroup->m_fMaxDistance;
        m_pStreamingSoundSource->SetDistance(fMinDistance, fMaxDistance);

        nResult = m_pStreamingSoundSource->Play(resRef, 4, 0, 0, 0);
    }
    else
    {
        CExoSoundSourceInternal* pSource = new CExoSoundSourceInternal(this);

        pSource->SetResRef(resRef, 0);
        pSource->m_bOneShot = 1;
        pSource->Set3D(1);
        pSource->SetPriorityGroup(nPriorityGroup);
        pSource->SetPosition(vPosition);
        if (nVolume)
            pSource->SetVolume(nVolume, 0, 1.0f);
        if (fFixedVariance != 0.0f)
            pSource->SetFixedVariance(fFixedVariance);
        if (fPitchVariance != 0.0f)
            pSource->SetPitchVariance(fPitchVariance);

        CPriorityGroup* pGroup = GetPriorityGroup(nPriorityGroup);
        pSource->m_fMaxDistance = pGroup->m_fMaxDistance;
        pSource->m_fMinDistance = pGroup->m_fMinDistance;

        if (nDelay != 0)
        {
            pSource->SetOneShotDelay(nDelay);
            nResult = 0;
        }
        else
        {
            nResult = pSource->Play(0, 0);
            if (pSource->m_bOneShotFailed)
                delete pSource;
        }
    }

    return nResult;
}

#if !KOTOR1  // K1 has no streaming-music-for-movie flag
void CExoSoundInternal::PauseAllButStreamingMusicForMovie()
{
    CExoLinkedListNode* pNode = m_lStreaming.m_pInternal->m_pHead;
    CExoStreamingSoundSourceInternal* pStream = 0;
    if (m_lStreaming.m_pInternal->m_pHead != 0)
        pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->m_pHead->m_pData);

    while (pNode != 0)
    {
        if (GetPauseable(pStream) && !pStream->m_bStreamingMusicForMovie)
            pStream->Pause();
        pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->GetNext(pNode));
    }

    CExoSoundSourceInternal* pSource = 0;
    pNode = m_lPlaying.m_pInternal->m_pHead;
    if (m_lPlaying.m_pInternal->m_pHead != 0)
        pSource = static_cast<CExoSoundSourceInternal*>(m_lPlaying.m_pInternal->m_pHead->m_pData);

    while (pNode != 0)
    {
        if (!pSource->IsPaused() && GetPauseable(pSource))
            pSource->Pause();
        pSource = static_cast<CExoSoundSourceInternal*>(m_lPlaying.m_pInternal->GetNext(pNode));
    }
}
#endif

int CExoSoundInternal::PauseAllSounds(int bPause)
{
    g_bSoundPaused = 1;

    CExoLinkedListNode* pNode = m_lStreaming.m_pInternal->m_pHead;
    CExoStreamingSoundSourceInternal* pStream = 0;
    if (m_lStreaming.m_pInternal->m_pHead != 0)
        pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->m_pHead->m_pData);

    while (pNode != 0)
    {
        if (bPause)
            pStream->Pause();
        else if (GetPauseable(pStream))
            pStream->Pause();
        pStream = static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->GetNext(pNode));
    }

    CExoSoundSourceInternal* pSource = 0;
    pNode = m_lPlaying.m_pInternal->m_pHead;
    if (m_lPlaying.m_pInternal->m_pHead != 0)
        pSource = static_cast<CExoSoundSourceInternal*>(m_lPlaying.m_pInternal->m_pHead->m_pData);

    while (pNode != 0)
    {
        if (!pSource->IsPaused() && GetPauseable(pSource))
            pSource->Pause();
        pSource = static_cast<CExoSoundSourceInternal*>(m_lPlaying.m_pInternal->GetNext(pNode));
    }

    return 1;
}

C3DVoice* CExoSoundInternal::GetFree3DVoice(unsigned char nGroup)
{
    CPriorityGroup* pGroup = GetPriorityGroup(nGroup);
    if (pGroup == 0)
        return 0;

    uint64_t nCount = m_nNumber3DVoices;
    if (nCount == 0)
        return 0;

    uint64_t i = 0;
    unsigned char nBestScore = 0;
    int nBest K2_UNSET;
    for (; i < nCount; i++)
    {
        if (m_pa3DVoices[i].m_bInUse != 0)
            continue;

        // An empty slot answers with itself rather than null.
        C3DVoice* pVoice = &m_pa3DVoices[i];
        CExoSoundSourceInternal* pSource = pVoice->m_pSource;
        if (pSource == 0)
            return pVoice;

        CPriorityGroup* pSourceGroup = GetPriorityGroup(pSource->m_nPriorityGroup);
        if (pSourceGroup == 0)
            continue;

        unsigned char nScore = pSourceGroup->m_nPriority;
        if (nScore > pGroup->m_nPriority && nScore > nBestScore)
        {
            nBestScore = nScore;
            nBest = static_cast<int>(i);
        }
    }

    if (nBestScore == 0)
        return 0;

    CExoSoundSourceInternal* pSource = m_pa3DVoices[nBest].m_pSource;
    if (pSource->m_bOneShot)
    {
        if (pSource->IsPlaying())
        {
            if (!m_lStoppingOneShots.m_pInternal->Contains(pSource))
                m_lStoppingOneShots.AddHead(pSource);
        }
        else if (!m_lStoppingOneShots.m_pInternal->Contains(pSource))
        {
            delete pSource;
        }
    }
    else
    {
        pSource->ShutDownSource(0);
        if (pSource->m_bLooping)
            AddToNonPlayingLoopingList(pSource);
    }

    return &m_pa3DVoices[nBest];
}

C2DVoice* CExoSoundInternal::GetFree2DVoice(unsigned char nGroup)
{
    CPriorityGroup* pGroup = GetPriorityGroup(nGroup);
    if (pGroup == 0)
        return 0;

    if (m_nNumber2DVoices < 2)
        return 0;

    uint64_t i = 0;
    unsigned char nBestScore = 0;
    int nBest K2_UNSET;
    // Only the first half of the table is searched.
    for (; i < m_nNumber2DVoices / 2; i++)
    {
        if (m_pa2DVoices[i].m_bInUse != 0)
            continue;

        C2DVoice* pVoice = &m_pa2DVoices[i];
        CExoSoundSourceInternal* pSource = pVoice->m_pSource;
        if (pSource == 0)
            return pVoice;

        CPriorityGroup* pSourceGroup = GetPriorityGroup(pSource->m_nPriorityGroup);
        if (pSourceGroup == 0)
            continue;

        unsigned char nScore = pSourceGroup->m_nPriority;
        if (nScore > pGroup->m_nPriority && nScore > nBestScore)
        {
            nBestScore = nScore;
            nBest = static_cast<int>(i);
        }
    }

    if (nBestScore == 0)
        return 0;

    CExoSoundSourceInternal* pSource = m_pa2DVoices[nBest].m_pSource;
    if (pSource->m_bOneShot)
    {
        if (pSource->IsPlaying())
        {
            if (!m_lStoppingOneShots.m_pInternal->Contains(pSource))
                m_lStoppingOneShots.AddHead(pSource);
        }
        else if (!m_lStoppingOneShots.m_pInternal->Contains(pSource))
        {
            delete pSource;
        }
    }
    else
    {
        pSource->ShutDownSource(0);
        if (pSource->m_bLooping)
            AddToNonPlayingLoopingList(pSource);
    }

    return &m_pa2DVoices[nBest];
}

CExoLinkedListNode* CExoSoundInternal::RemoveFromPlayingList(CExoSoundSourceInternal* pSource)
{
    CExoLinkedListNode* pNode = m_lPlaying.m_pInternal->m_pHead;
    CExoSoundSourceInternal* pObj = 0;
    if (m_lPlaying.m_pInternal->m_pHead != 0)
        pObj = static_cast<CExoSoundSourceInternal*>(m_lPlaying.m_pInternal->m_pHead->m_pData);

    while (pNode != 0)
    {
        if (pObj == pSource)
        {
            if (pNode == m_pOcclusionCursor)
            {
                // The member cursor is advanced, so m_pOcclusionCursor moves off the removed node.
                m_lPlaying.GetNext(m_pOcclusionCursor);
                if (m_pOcclusionCursor == 0)
                {
                    m_pOcclusionCursor = m_lStreaming.GetHeadPos();
                    m_nOcclusionCursorList = 2;
                    if (m_pOcclusionCursor == 0)
                    {
                        m_nOcclusionCursorList = 1;
                        m_pOcclusionCursor = 0;
                    }
                }
            }
            break;
        }
        pObj = static_cast<CExoSoundSourceInternal*>(m_lPlaying.GetNext(pNode));
    }

    pNode = pSource->m_pListNode;
    m_lPlaying.GetNext(pNode);
    m_lPlaying.Remove(pSource->m_pListNode);
    pSource->m_nListState = 0;
    pSource->m_pListNode = 0;

#if KOTOR1  // K1: no range fallback on the group index
    CPriorityGroup* pGroup = GetPriorityGroup(pSource->m_nPriorityGroup);
#else
    CPriorityGroup* pGroup =
        &m_paGroups[m_maxGroup > pSource->m_nPriorityGroup ? pSource->m_nPriorityGroup : PRIORITY_GROUP_DEFAULT];
#endif
    pGroup->m_nPlayingCount--;

    return pNode;
}

CExoLinkedListNode* CExoSoundInternal::RemoveFromStreamingList(CExoStreamingSoundSourceInternal* pSource)
{
    CExoLinkedListNode* pNode = pSource->m_pListNode;
    if (pNode == 0)
        return 0;

    if (pNode == m_pOcclusionCursor)
    {
        m_lStreaming.GetNext(m_pOcclusionCursor);
        if (m_pOcclusionCursor == 0)
        {
            m_pOcclusionCursor = m_lPlaying.GetHeadPos();
            m_nOcclusionCursorList = 1;
            if (m_pOcclusionCursor == 0)
            {
                m_nOcclusionCursorList = 1;
                m_pOcclusionCursor = 0;
            }
        }
    }

    m_lStreaming.GetNext(pNode);
    m_lStreaming.Remove(pSource->m_pListNode);
    pSource->m_pListNode = 0;

#if KOTOR1  // K1: no range fallback on the group index
    CPriorityGroup* pGroup = GetPriorityGroup(pSource->m_nPriorityGroup);
#else
    CPriorityGroup* pGroup =
        &m_paGroups[m_maxGroup > pSource->m_nPriorityGroup ? pSource->m_nPriorityGroup : PRIORITY_GROUP_DEFAULT];
#endif
    pGroup->m_nPlayingCount--;

    return pNode;
}

CExoLinkedListNode* CExoSoundInternal::RemoveFromNonPlayingStreamingLoopingList(
    CExoStreamingSoundSourceInternal* pSource)
{
    CExoLinkedListNode* pNode = m_lNonPlayingStreamingLooping.m_pInternal->m_pHead;
    CExoStreamingSoundSourceInternal* pObj = 0;
    if (m_lNonPlayingStreamingLooping.m_pInternal->m_pHead != 0)
        pObj = static_cast<CExoStreamingSoundSourceInternal*>(
            m_lNonPlayingStreamingLooping.m_pInternal->m_pHead->m_pData);

    while (pNode != 0)
    {
        CExoLinkedListNode* pCur = pNode;
        CExoStreamingSoundSourceInternal* pNext = static_cast<CExoStreamingSoundSourceInternal*>(
            m_lNonPlayingStreamingLooping.GetNext(pNode));
        if (pObj == pSource)
        {
            m_lNonPlayingStreamingLooping.Remove(pCur);
            return pNode;
        }
        pObj = pNext;
    }

    return 0;
}

int CExoSoundInternal::RemoveAllSounds()
{
    g_bSoundPaused = 1;

    while (m_lStreaming.m_pInternal->m_pHead != 0)
    {
        CExoStreamingSoundSourceInternal* pStream =
            static_cast<CExoStreamingSoundSourceInternal*>(m_lStreaming.m_pInternal->m_pHead->m_pData);
        if (pStream != 0)
            delete pStream;
    }

    while (m_lPlaying.m_pInternal->m_pHead != 0)
    {
        CExoSoundSourceInternal* pSource =
            static_cast<CExoSoundSourceInternal*>(m_lPlaying.m_pInternal->m_pHead->m_pData);
        if (pSource->IsPlaying())
        {
            if (!m_lStoppingOneShots.m_pInternal->Contains(pSource))
                m_lStoppingOneShots.m_pInternal->AddHead(pSource);
        }
        else if (!m_lStoppingOneShots.m_pInternal->Contains(pSource))
        {
            delete pSource;
        }
    }

    return 1;
}

void CExoSoundInternal::StopAllOneShots()
{
    CExoLinkedListNode* pNode = m_lPlaying.m_pInternal->m_pHead;
    CExoSoundSourceInternal* pObj = 0;
    if (m_lPlaying.m_pInternal->m_pHead != 0)
        pObj = static_cast<CExoSoundSourceInternal*>(m_lPlaying.m_pInternal->m_pHead->m_pData);

    while (pNode != 0)
    {
        // An empty payload falls through to the loop's own node test.
        if (pObj == 0)
        {
        }
        else if (!pObj->m_bOneShot)
        {
            pObj = static_cast<CExoSoundSourceInternal*>(m_lPlaying.m_pInternal->GetNext(pNode));
        }
        else
        {
            pNode = RemoveFromPlayingList(pObj);
            if (pObj->IsPlaying())
            {
                if (!m_lStoppingOneShots.m_pInternal->Contains(pObj))
                    m_lStoppingOneShots.m_pInternal->AddHead(pObj);
            }
            else if (!m_lStoppingOneShots.m_pInternal->Contains(pObj))
            {
                delete pObj;
            }

            if (pNode != 0)
                pObj = static_cast<CExoSoundSourceInternal*>(m_lPlaying.m_pInternal->GetAtPos(pNode));
        }
    }
}

#if KOTOR1  // K1 has no movie-music flag, so every pause is PauseAllSounds
void CExoSoundInternal::SetSoundMode(int mode)
#else
void CExoSoundInternal::SetSoundMode(int mode, int flag)
#endif
{
    int nCurrent;

    if (mode == CExoSound::CEXOSOUND_MODE_LOSTFOCUS ||
        mode == CExoSound::CEXOSOUND_MODE_GAINFOCUS)
    {
        while (true)
        {
            if (mode == CExoSound::CEXOSOUND_MODE_LOSTFOCUS)
            {
#if KOTOR1
                PauseAllSounds(1);
#else
                if (flag)
                    PauseAllButStreamingMusicForMovie();
                else
                    PauseAllSounds(1);
#endif
                m_bLostFocusPaused = 1;
                return;
            }

            m_bLostFocusPaused = 0;
            ResumeAllSounds();

            if (m_nSoundModeIndex == 0)
                return;

            mode = m_aSoundModes[m_nSoundModeIndex];
            m_nSoundModeIndex--;

            if (mode == CExoSound::CEXOSOUND_MODE_LOSTFOCUS)
            {
#if !KOTOR1
                flag = 0;
#endif
                continue;
            }

            if (mode == CExoSound::CEXOSOUND_MODE_GAINFOCUS)
                continue;

#if !KOTOR1
            flag = 0;
#endif
            break;
        }
    }

    nCurrent = m_aSoundModes[m_nSoundModeIndex];

    if (nCurrent == mode && mode != CExoSound::CEXOSOUND_MODE_GENERIC)
        return;

    if (nCurrent == CExoSound::CEXOSOUND_MODE_MUTE)
        UnMuteSound();

    nCurrent = m_aSoundModes[m_nSoundModeIndex];

    if (nCurrent == CExoSound::CEXOSOUND_MODE_PAUSE ||
        nCurrent == CExoSound::CEXOSOUND_MODE_PAUSE_ALL ||
        nCurrent == CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC)
        ResumeAllSounds();

    if (mode == CExoSound::CEXOSOUND_MODE_GENERIC)
    {
        m_nSoundModeIndex--;
        if (m_nSoundModeIndex < 0)
            m_nSoundModeIndex = 0;
        nCurrent = m_aSoundModes[m_nSoundModeIndex];
    }
    else
    {
        m_nSoundModeIndex = m_nSoundModeIndex > 8 ? 9 : m_nSoundModeIndex + 1;
        m_aSoundModes[m_nSoundModeIndex] = mode;
        nCurrent = mode;
    }

    if (nCurrent == CExoSound::CEXOSOUND_MODE_MUTE)
        MuteSound();

    if (nCurrent == CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC ||
        nCurrent == CExoSound::CEXOSOUND_MODE_PAUSE ||
        nCurrent == CExoSound::CEXOSOUND_MODE_PAUSE_ALL)
    {
#if KOTOR1
        PauseAllSounds(0);
#else
        if (flag)
            PauseAllButStreamingMusicForMovie();
        else
            PauseAllSounds(0);
#endif
    }
}

void CExoSoundInternal::InterruptPlayingSound(unsigned char nGroup)
{
    CExoLinkedListNode* pNode = m_lPlaying.m_pInternal->m_pHead;
    CExoSoundSourceInternal* pSource = 0;
    if (m_lPlaying.m_pInternal->m_pHead != 0)
        pSource = static_cast<CExoSoundSourceInternal*>(m_lPlaying.m_pInternal->m_pHead->m_pData);

    while (pNode != 0)
    {
        if (pSource->m_nPriorityGroup == nGroup)
        {
            if (pSource->m_bOneShot)
            {
                if (pSource->IsPlaying())
                {
                    if (!m_lStoppingOneShots.m_pInternal->Contains(pSource))
                        m_lStoppingOneShots.m_pInternal->AddHead(pSource);
                }
                else if (!m_lStoppingOneShots.m_pInternal->Contains(pSource))
                {
                    delete pSource;
                }
            }
            else
            {
                pSource->ShutDownSource(0);
                if (pSource->m_bLooping)
                    AddToNonPlayingLoopingList(pSource);
            }

            break;
        }

        pSource = static_cast<CExoSoundSourceInternal*>(m_lPlaying.m_pInternal->GetNext(pNode));
    }
}
