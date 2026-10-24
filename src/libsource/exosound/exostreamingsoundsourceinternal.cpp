// CExoStreamingSoundSourceInternal's constructors, destructor and leaf state accessors.

#include "k2stdint.h"
#include "exostreamingsoundsourceinternal.h"
#include "exosoundinternal.h"
#include "exosound.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "restype.h"
#include "soundprioritygroup.h"
#if KOTOR1
#include "port/k1obb.h"
#endif
#include "reswave.h"
#include <string>
#include <algorithm>

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcmath.h"

#include "port/crtapi.h"

#if !KOTOR1
// The candidate-path buffers FormatName writes.
enum {
    PATH_BUFFER_SIZE = 256,
};

// Fortified path formatter for the candidate paths; its size argument is unread, so call sites pass
// 0.
static int FormatName(char* pszBuf, uint64_t nSize, const char* pszFmt, ...)
{
    K2_UNUSED(nSize);
    __builtin_va_list ap;
    __builtin_va_start(ap, pszFmt);
    int nResult = __vsprintf_chk(pszBuf, 0, PATH_BUFFER_SIZE, pszFmt, ap);
    __builtin_va_end(ap);
    return nResult;
}
#endif

const char* GetOverridePath(int nIndex, bool bFlag);

void DOS2MacPath(const char* sDosPath, char* sMacPath);

// Set when the engine's sound resources were modified on disk, true only for the remastered assets.
// InitializeSource streams the file as it lies while this is 0.
int g_bModifiedSound = 1;

extern CExoSound* g_pExoSound;

extern int g_totalStreamingHandlesAvailable;
extern int g_generalStreamingHandlesAvailable;

#if KOTOR1
// K1 has no stream type, one path string, and reads the group-2 volume straight from the table.
CExoStreamingSoundSourceInternal::CExoStreamingSoundSourceInternal(CExoSoundInternal* pSound)
    : m_nStreamHandle(0), m_vPosition(), m_vOcclusionPosition()
{
    m_nChannel = static_cast<uint64_t>(-1);
    m_bLooping = 0;
    m_nPriorityGroup = PRIORITY_GROUP_STREAMS_MUSIC;
    m_pListNode = 0;
    m_pSound = pSound;
    m_pAudioSystem = &pSound->m_audioSystem;
    m_nFadeDuration = 0;
    m_nStreamingVoiceId = pSound->m_nNextStreamingVoiceId++;
    m_nFadeStartTime = 0;
    m_pFile = 0;
    m_nStreamOffset = 0;
    m_nStreamLength = 0;
    m_nPlaybackPosition = 0;
    m_nVolume = CExoSound::CMAX_VOLUME;
    m_fMinDistance = 0.0f;
    m_fMaxDistance = 0.0f;
    m_fVolume = -1.0f;
    m_b3D = 0;
    m_vPosition.x = 0.0f;
    m_vPosition.y = 0.0f;
    m_vPosition.z = 0.0f;
    m_bReverbEnabled = 1;
    m_bDirty = 1;

    m_nGroupVolume = pSound->GetPriorityGroup(PRIORITY_GROUP_STREAMS_MUSIC)->m_nVolume;

    m_vOcclusionPosition.x = 0.0f;
    m_vOcclusionPosition.y = 0.0f;
    m_vOcclusionPosition.z = 0.0f;
    m_nOcclusionTime = 0;
    m_fOcclusionFactor = 0.0f;
}
#else
// m_nStreamHandle and the two Vectors are member initializers so their zeroing precedes the body's first
// scalar store; the occlusion vector is then zeroed again by the trailing block.
//
// m_nStreamingVoiceId takes the owner's counter value before the increment.
CExoStreamingSoundSourceInternal::CExoStreamingSoundSourceInternal(CExoSoundInternal* pSound,
                                                                   int nType)
    : m_nStreamHandle(0), m_vPosition(), m_vOcclusionPosition()
{
    m_nChannel = -1;
    m_nType = nType;
    m_bLooping = 0;
    m_nPriorityGroup = PRIORITY_GROUP_STREAMS_MUSIC;
    m_pListNode = 0;
    m_pSound = pSound;
    m_pAudioSystem = &pSound->m_audioSystem;
    m_nFadeDuration = 0;
    m_nStreamingVoiceId = pSound->m_nNextStreamingVoiceId++;
    m_nFadeStartTime = 0;
    m_pFile = 0;
    m_nStreamOffset = 0;
    m_nStreamLength = 0;
    m_nPlaybackPosition = 0;
    m_nVolume = CExoSound::CMAX_VOLUME;
    m_fMinDistance = 0.0f;
    m_fMaxDistance = 0.0f;
    m_fVolume = -1.0f;
    m_b3D = 0;
    m_vPosition.x = 0.0f;
    m_vPosition.y = 0.0f;
    m_vPosition.z = 0.0f;
    m_bReverbEnabled = 1;
    m_bDirty = 1;

    CPriorityGroup* pGroup = pSound->GetPriorityGroup(PRIORITY_GROUP_STREAMS_MUSIC);
    m_nGroupVolume = pGroup ? pGroup->m_nVolume : 127;

    // Zeroed again on purpose, though the member initializer above already did it.
    m_vOcclusionPosition.x = 0.0f;
    m_vOcclusionPosition.y = 0.0f;
    m_vOcclusionPosition.z = 0.0f;
    m_nOcclusionTime = 0;
    m_fOcclusionFactor = 0.0f;
    m_bStreamingMusicForMovie = 0;
}
#endif

// The owner's sound-system-live flag and the channel sentinel are tested before the backend call.
int CExoStreamingSoundSourceInternal::IsPlaying()
{
    if (m_pSound->m_bInitialized && m_nChannel != static_cast<decltype(m_nChannel)>(-1))
        return m_pAudioSystem->GetIsChannelPlaying(m_nChannel);
    return 0;
}

#if !KOTOR1  // K1 has no IsDone (and no FModAudioSystem::GetIsChannelDone)
int CExoStreamingSoundSourceInternal::IsDone()
{
    if (m_pSound->m_bInitialized && m_nChannel != -1)
        return m_pAudioSystem->GetIsChannelDone(m_nChannel);
    return 0;
}
#endif

// The member is re-read for the call's argument because the call may change it, and the backend's
// loop count is inverted relative to the flag.
void CExoStreamingSoundSourceInternal::SetLooping(int bLooping)
{
    m_bLooping = bLooping;

    if (m_pSound->m_bInitialized && m_nChannel != static_cast<decltype(m_nChannel)>(-1)
        && m_pAudioSystem->GetIsChannelPlaying(m_nChannel))
        m_pAudioSystem->SetChannelLoopCount(m_nChannel, m_bLooping == 0);
}

// A group the table does not know falls back to the current one, which is only replaced once the
// fallback resolves. The group's two float distances are stored in reverse order: the minimum comes
// from the group's second float and the maximum from its first.
void CExoStreamingSoundSourceInternal::SetPriorityGroup(unsigned char nGroup)
{
    CPriorityGroup* pGroup = m_pSound->GetPriorityGroup(nGroup);

    if (pGroup)
    {
        m_nPriorityGroup = nGroup;
    }
    else
    {
        pGroup = m_pSound->GetPriorityGroup(m_nPriorityGroup);
#if !KOTOR1  // K1 does not null-test the fallback group
        if (!pGroup)
            return;
#endif
    }

    m_pAudioSystem->SetChannelPriority(m_nChannel, pGroup->m_nPriority);
    m_nGroupVolume = pGroup->m_nVolume;
    SetVolume(m_nVolume, 0, 1.0f);
    m_fMinDistance = pGroup->m_fMinDistance;
    m_fMaxDistance = pGroup->m_fMaxDistance;
}

// Tests range, answering 1 while the source is not 3D, otherwise whether the listener is within the
// maximum distance.
int CExoStreamingSoundSourceInternal::GetWithinRangeOfListener()
{
    if (!m_b3D)
        return 1;

    Vector vDiff = m_pSound->m_vListenerPosition - m_vPosition;
    return magnitude(vDiff) <= m_fMaxDistance;
}

// Only a live 3D channel fades. The fade is recorded as a timestamp and a duration: the current
// low-resolution timer and the requested millisecond count.
void CExoStreamingSoundSourceInternal::FadeAndStop(unsigned int nMilliseconds)
{
    if (m_pSound->m_bInitialized && m_nChannel != static_cast<decltype(m_nChannel)>(-1)
        && m_pAudioSystem->GetIsChannelPlaying(m_nChannel))
    {
        m_nFadeStartTime = g_pExoBase->m_pTimers->GetLowResolutionTimer();
        m_nFadeDuration = nMilliseconds;
    }
}

// Resumes behind the inverse guard of Pause, running only when the channel is not already playing,
// then restoring and clearing the saved playback position.
void CExoStreamingSoundSourceInternal::Resume()
{
    if (!m_pSound->m_bInitialized || m_nChannel == static_cast<decltype(m_nChannel)>(-1))
        return;
    if (m_pAudioSystem->GetIsChannelPlaying(m_nChannel))
        return;

    SetVolume(m_nVolume, 0, 1.0f);
    m_pAudioSystem->SetChannelLoopCount(m_nChannel, m_bLooping == 0);
    m_pAudioSystem->SetChannelPlaybackPosition(m_nChannel, m_nPlaybackPosition);
    m_pAudioSystem->ResumeChannel(m_nChannel);
    m_nPlaybackPosition = 0;
}

// Every stream is paused except "evil_ending", which is paused only while the credit sequence runs.
// CompareNoCase returns nonzero when the strings are equal, so the equality test on its result means
// "is not evil_ending".
void CExoStreamingSoundSourceInternal::Pause()
{
    CExoString sResRefName;
    m_resRef.CopyToString(sResRefName);

    if (sResRefName.CompareNoCase("evil_ending") == 0
        || g_pAppManager->m_pClientExoApp->GetCreditSequenceInProgress())
    {
        if (m_pSound->m_bInitialized && m_nChannel != static_cast<decltype(m_nChannel)>(-1)
            && m_pAudioSystem->GetIsChannelPlaying(m_nChannel))
        {
            m_pAudioSystem->PauseChannel(m_nChannel);
            m_nPlaybackPosition = m_pAudioSystem->GetChannelPlaybackPosition(m_nChannel);
        }
    }
}

// Clears the fade triple and unlinks the source from whichever list owns it; the fade flag then
// closes the stream, releases the channel and credits the handle pool back. The channel test in that
// second half is against zero, not the -1 sentinel every other guard here uses: zero means the source
// holds no sound slot.
#if KOTOR1
// K1's argumentless form: pause, clear the fade triple, unlink.
void CExoStreamingSoundSourceInternal::Stop()
{
    if (m_nChannel != static_cast<uint64_t>(-1))
        m_pAudioSystem->PauseChannel(m_nChannel);

    m_nFadeDuration = 0;
    m_nFadeStartTime = 0;
    m_nPlaybackPosition = 0;

    if (m_pListNode)
        m_pSound->RemoveFromStreamingList(this);
    else
        m_pSound->RemoveFromNonPlayingStreamingLoopingList(this);
}
#else
void CExoStreamingSoundSourceInternal::Stop(int bFade)
{
    if (m_nChannel != -1)
        m_pAudioSystem->PauseChannel(m_nChannel);

    m_nFadeDuration = 0;
    m_nFadeStartTime = 0;
    m_nPlaybackPosition = 0;

    if (m_pListNode)
        m_pSound->RemoveFromStreamingList(this);
    else
        m_pSound->RemoveFromNonPlayingStreamingLoopingList(this);

    if (bFade && m_nChannel)
    {
        m_pAudioSystem->CloseStream(m_nStreamHandle);
        m_nChannel = -1;
        g_totalStreamingHandlesAvailable++;
        if (m_nType == 0)
            g_generalStreamingHandlesAvailable++;
    }
}
#endif

// Runs the full mixer path. Scales the volume by the slider, the group volume and, for groups 4 and
// 5, a ducking factor, pushes it to the channel and then refreshes the 3D stream.
void CExoStreamingSoundSourceInternal::SetVolume(unsigned char nVolume, int a, float f)
{
    int nSliderType = m_pSound->GetSliderType(this);
    float fScale = 1.0f;
    if (m_nPriorityGroup == PRIORITY_GROUP_SINGLE_SHOT_GLOBAL || m_nPriorityGroup == PRIORITY_GROUP_LOOPING_AREA_WIDE_AMBIENTS)
        fScale = f;

    if (!a)
        m_nVolume = nVolume;

    if (!m_pSound->m_bInitialized || m_nChannel == static_cast<decltype(m_nChannel)>(-1))
        return;

    int nVolumeScaled = static_cast<int>(fScale * static_cast<float>(nVolume));

    if (!(m_pSound->GetMutable(this) | a))
        a = (m_pSound->GetSliderVolume(nSliderType, 1)
             != m_pSound->GetSliderVolume(nSliderType, 0));

    float fSlider = m_pSound->GetSliderVolume(nSliderType, a);
    fSlider *= static_cast<float>(m_nGroupVolume) / 127.0f;

#if !KOTOR1  // K1 has no ducking and no 0.6 cut for groups 4/5
    if (nSliderType == 3 || (m_nPriorityGroup & 0xfe) == 4)
        fSlider *= m_pSound->m_fDuckVolume;
    if ((m_nPriorityGroup & 0xfe) == 4)
        fSlider *= 0.6f;
#endif

    float fFinal = fSlider * static_cast<float>(nVolumeScaled) / 127.0f;
    m_pAudioSystem->SetChannelVolume(m_nChannel, fFinal);
    m_fVolume = fFinal;
    Update3DStream();
}

#if KOTOR1
// K1: one HD0 path per flavour, no override candidates, and no move to the non-playing list when the
// range test parks the source.
int CExoStreamingSoundSourceInternal::Play(const CResRef& resRef, unsigned short a, int b, int c,
                                           int d)
{
    K2_UNUSED(d);
    if (m_nChannel == static_cast<uint64_t>(-1) || resRef != m_resRef)
    {
        CExoString sResRefName;
        resRef.CopyToString(sResRefName);
        sResRefName.CStr();

        m_resRef = resRef;
        m_nResType = a;
        resRef.CopyToString(m_sStreamPath);

        if (c != 0)
        {
            m_sStreamPath.Format(const_cast<char*>("HD0:STREAMMUSIC\\%s"), m_sStreamPath.CStr());
        }
        else if (b != 0)
        {
            if ((m_sStreamPath[0] == 'n' || m_sStreamPath[0] == 'N') && m_sStreamPath.GetLength() == 16
                && m_sStreamPath[1] != '_')
            {
                char szArea[6];
                szArea[0] = m_sStreamPath[1];
                szArea[1] = m_sStreamPath[2];
                szArea[2] = m_sStreamPath[3];
                szArea[3] = m_sStreamPath[4];
                szArea[4] = m_sStreamPath[5];
                szArea[5] = 0;
                char szSpeaker[7];
                szSpeaker[0] = m_sStreamPath[6];
                szSpeaker[1] = m_sStreamPath[7];
                szSpeaker[2] = m_sStreamPath[8];
                szSpeaker[3] = m_sStreamPath[9];
                szSpeaker[4] = m_sStreamPath[10];
                szSpeaker[5] = m_sStreamPath[11];
                szSpeaker[6] = 0;
                m_sStreamPath.Format(const_cast<char*>("HD0:STREAMWAVES\\%s\\%s\\%s"), szArea, szSpeaker, m_sStreamPath.CStr());
            }
            else
            {
                m_sStreamPath.Format(const_cast<char*>("HD0:STREAMWAVES\\%s"), m_sStreamPath.CStr());
            }
        }
        else
        {
            m_sStreamPath.Format(const_cast<char*>("HD0:STREAMSOUNDS\\%s"), m_sStreamPath.CStr());
        }

        m_sStreamPath = g_pExoBase->m_pAliasList->ResolveFileName(m_sStreamPath, a);

        if (m_nChannel != static_cast<uint64_t>(-1))
            m_pAudioSystem->PauseChannel(m_nChannel);
        m_nFadeDuration = 0;
        m_nFadeStartTime = 0;
        m_nPlaybackPosition = 0;
        if (m_pListNode)
            m_pSound->RemoveFromStreamingList(this);
        else
            m_pSound->RemoveFromNonPlayingStreamingLoopingList(this);
        if (m_nChannel != static_cast<uint64_t>(-1))
        {
            m_pAudioSystem->CloseStream(m_nStreamHandle);
            m_nChannel = static_cast<uint64_t>(-1);
            m_nStreamHandle = 0;
        }
        InitializeSource();
    }
    else
    {
        if (m_nChannel != static_cast<uint64_t>(-1))
            m_pAudioSystem->PauseChannel(m_nChannel);
        m_nFadeDuration = 0;
        m_nFadeStartTime = 0;
        m_nPlaybackPosition = 0;
        if (m_pListNode)
            m_pSound->RemoveFromStreamingList(this);
        else
            m_pSound->RemoveFromNonPlayingStreamingLoopingList(this);
    }

    if (m_pSound->m_bInitialized && m_nChannel != static_cast<uint64_t>(-1))
    {
        SetVolume(m_nVolume, 1, m_pSound->m_fAmbientScale);

        bool bLoopCount = (m_bLooping == 0);
        Update3DStream();
        m_pAudioSystem->SetChannelLoopCount(m_nChannel, bLoopCount);

        m_nPlaybackPosition = 0;
        int nLength = m_pAudioSystem->GetStreamLength(m_nStreamHandle);

        CPriorityGroup* pGroup = m_pSound->GetPriorityGroup(m_nPriorityGroup);
        if (pGroup->m_nPlayingCount >= pGroup->m_nMaxPlaying)
        {
            if (pGroup->m_nInterrupt != 0)
            {
                m_pSound->InterruptPlayingStream(m_nPriorityGroup);
            }
            else if (m_bLooping != 0)
            {
                goto AddToNonPlayingLooping;
            }
            else
            {
                return 0;
            }
        }

        if (m_bLooping != 0 && m_b3D != 0)
        {
            Vector vDiff = m_pSound->m_vListenerPosition - m_vPosition;
            if (!(magnitude(vDiff) <= m_fMaxDistance))
                goto AddToNonPlayingLooping;
        }

        if (m_pSound->AddToStreamingList(this))
        {
            m_pAudioSystem->ResumeChannel(m_nChannel);
            if (m_pSound->m_bLostFocusPaused != 0
                || (g_bSoundPaused != 0 && m_pSound->GetPauseable(this) != 0))
                Pause();
            return nLength;
        }
        return 0;

    AddToNonPlayingLooping:
        m_pSound->AddToNonPlayingStreamingLoopingList(this);
        return nLength;
    }
    return 0;
}
#else
// Builds the candidate path and re-opens the stream when the resref changed or no channel is open
// yet; otherwise only the tear-down half runs. The arms, selected by c and b: music formats one HD0
// path, stream sounds walks the override candidates first and formats afterwards, and voice recovers
// a three-character area prefix plus a speaker run from the resref and picks one of three shapes.
// Everything then resolves through the alias list, tears the old stream down and re-opens it.
// Returns the stream length.
int CExoStreamingSoundSourceInternal::Play(const CResRef& resRef, unsigned short a, int b, int c,
                                           int d)
{
    K2_UNUSED(d);
    if (m_nChannel == -1 || resRef != m_resRef)
    {
        CExoString sResRefName;
        resRef.CopyToString(sResRefName);
        sResRefName.CStr();   // the return value is discarded, but the call stays

        m_resRef = resRef;
        m_nResType = a;
        resRef.CopyToString(m_sStreamPath);

        for (int i = 0; i < 16; i++)
            m_asOverridePaths[i] = "";

        const char* pszOverride;

        if (c == 0)
        {
            if (b == 0)
            {
                for (int i = 0; (pszOverride = GetOverridePath(i, true)) != 0; i++)
                {
                    char szPath[PATH_BUFFER_SIZE];
                    FormatName(szPath, sizeof(szPath), "%s\\streamsounds\\%s", pszOverride,
                               m_sStreamPath.CStr());
                    m_asOverridePaths[i] = szPath;
                }
                m_sStreamPath.Format(const_cast<char*>("HD0:STREAMSOUNDS\\%s"), m_sStreamPath.CStr());
            }
            else
            {
                char* pszName = m_sStreamPath.m_sString;
                if (pszName)
                {
                    int nLen = static_cast<int>(strlen(pszName));
                    if (nLen != 0)
                    {
                        char szFirst[4];
                        szFirst[0] = m_sStreamPath[0];
                        szFirst[1] = m_sStreamPath[1];
                        szFirst[2] = m_sStreamPath[2];
                        szFirst[3] = 0;

                        int nNum = atoi(szFirst);
                        if (_stricmp(szFirst, "gbl") == 0
                            || _stricmp(szFirst, "avo") == 0
                            || _stricmp(szFirst, "000") == 0
                            || nNum > 0)
                        {
                            char szMid[32];
                            uint64_t nMid = static_cast<uint64_t>(nLen) - 3;
                            // Copies the run after the three-character prefix, at least three bytes.
                            if (nMid >= 4)
                            {
                                int64_t i = 0;
                                do
                                {
                                    szMid[i] = m_sStreamPath[static_cast<int>(i) + 3];
                                    i++;
                                } while (static_cast<uint64_t>(i) != nMid - 3);
                            }
                            else
                            {
                                nMid = 3;
                            }
                            szMid[nMid - 3] = 0;

                            m_sStreamPath.Format(const_cast<char*>("HD0:STREAMVOICE\\%s\\%s\\%s"), szFirst, szMid,
                                            m_sStreamPath.CStr());
                            for (int i = 0; (pszOverride = GetOverridePath(i, true)) != 0; i++)
                            {
                                char szPath[PATH_BUFFER_SIZE];
                                FormatName(szPath, sizeof(szPath), "%s\\streamvoice\\%s\\%s\\%s",
                                           pszOverride, szFirst, szMid, sResRefName.CStr());
                                m_asOverridePaths[i] = szPath;
                            }
                        }
                        else
                        {
                            m_sStreamPath.Format(const_cast<char*>("HD0:STREAMVOICE\\%s"), m_sStreamPath.CStr());
                        }

                        goto ResolvePaths;
                    }
                }
                m_sStreamPath = "HD0:STREAMVOICE\\";
            }
        }
        else
        {
            m_sStreamPath.Format(const_cast<char*>("HD0:STREAMMUSIC\\%s"), m_sStreamPath.CStr());
            for (int i = 0; (pszOverride = GetOverridePath(i, true)) != 0; i++)
            {
                char szPath[PATH_BUFFER_SIZE];
                FormatName(szPath, sizeof(szPath), "%s\\streammusic\\%s", pszOverride,
                           sResRefName.CStr());
                m_asOverridePaths[i] = szPath;
            }
        }

    ResolvePaths:
        m_sStreamPath = g_pExoBase->m_pAliasList->ResolveFileName(m_sStreamPath, a);
        for (int i = 0; i < 16; i++)
        {
            if (m_asOverridePaths[i] != "")
                m_asOverridePaths[i] = g_pExoBase->m_pAliasList->ResolveFileName(m_asOverridePaths[i], a);
            else
                break;
        }

        if (m_nChannel != -1)
            m_pAudioSystem->PauseChannel(m_nChannel);
        m_nFadeDuration = 0;
        m_nFadeStartTime = 0;
        m_nPlaybackPosition = 0;
        if (m_pListNode)
            m_pSound->RemoveFromStreamingList(this);
        else
            m_pSound->RemoveFromNonPlayingStreamingLoopingList(this);
        if (m_nChannel != -1)
        {
            m_pAudioSystem->StopChannel(m_nChannel);
            m_pAudioSystem->CloseStream(m_nStreamHandle);
            m_nChannel = -1;
            m_nStreamHandle = 0;
        }
        InitializeSource();
    }
    else
    {
        if (m_nChannel != -1)
            m_pAudioSystem->PauseChannel(m_nChannel);
        m_nFadeDuration = 0;
        m_nFadeStartTime = 0;
        m_nPlaybackPosition = 0;
        if (m_pListNode)
            m_pSound->RemoveFromStreamingList(this);
        else
            m_pSound->RemoveFromNonPlayingStreamingLoopingList(this);
    }

    if (m_pSound->m_bInitialized && m_nChannel != -1)
    {
        SetVolume(m_nVolume, 1, m_pSound->m_fAmbientScale);

        bool bLoopCount = (m_bLooping == 0);
        Update3DStream();
        m_pAudioSystem->SetChannelLoopCount(m_nChannel, bLoopCount);

        m_nPlaybackPosition = 0;
        int nLength = m_pAudioSystem->GetStreamLength(m_nStreamHandle);

        CPriorityGroup* pGroup = m_pSound->GetPriorityGroup(m_nPriorityGroup);
        if (pGroup && pGroup->m_nPlayingCount >= pGroup->m_nMaxPlaying)
        {
            if (pGroup->m_nInterrupt != 0)
            {
                m_pSound->InterruptPlayingStream(m_nPriorityGroup);
            }
            else if (m_bLooping != 0)
            {
                goto AddToNonPlayingLooping;
            }
            else
            {
                return 0;
            }
        }

        if (m_bLooping != 0 && m_b3D != 0)
        {
            Vector vDiff = m_pSound->m_vListenerPosition - m_vPosition;
            // An ordered compare, since `>` would differ for a NaN distance.
            if (!(magnitude(vDiff) <= m_fMaxDistance))
            {
                Stop(1);
                goto AddToNonPlayingLooping;
            }
        }

        if (m_pSound->AddToStreamingList(this))
        {
            m_pAudioSystem->ResumeChannel(m_nChannel);
            if (m_pSound->m_bLostFocusPaused != 0
                || (g_bSoundPaused != 0 && m_pSound->GetPauseable(this) != 0))
                Pause();
            return nLength;
        }
        return 0;

    AddToNonPlayingLooping:
        m_pSound->AddToNonPlayingStreamingLoopingList(this);
        return nLength;
    }
    return 0;
}
#endif

// The arms are a real if/else on a live channel; the else arm is the no-channel path, which still
// has to close the file and the stream handle, so it is not a mirror of the other. The live arm
// credits the two handle pools, choosing by type, and clears the channel.
CExoStreamingSoundSourceInternal::~CExoStreamingSoundSourceInternal()
{
    if (m_pListNode)
        m_pSound->RemoveFromStreamingList(this);
    else
        m_pSound->RemoveFromNonPlayingStreamingLoopingList(this);

#if KOTOR1  // K1 closes the stream, then the SDL_RWops; no StopChannel and no handle pools
    if (m_pSound->m_bInitialized && m_nChannel != static_cast<uint64_t>(-1))
    {
        m_pAudioSystem->CloseStream(m_nStreamHandle);
        m_nChannel = static_cast<uint64_t>(-1);
        m_nStreamHandle = 0;
        if (m_pFile)
        {
            SDL_RWclose(m_pFile);
            m_nStreamHandle = 0;
            m_pFile = 0;
            m_nStreamOffset = 0;
            m_nStreamLength = 0;
        }
    }
#else
    if (m_pSound->m_bInitialized && m_nChannel != -1)
    {
        m_pAudioSystem->StopChannel(m_nChannel);
        m_pAudioSystem->CloseStream(m_nStreamHandle);
        m_pFile = 0;
        m_nChannel = 0;
        m_nStreamHandle = 0;

        g_totalStreamingHandlesAvailable++;
        if (m_nType == 0)
            g_generalStreamingHandlesAvailable++;
    }
    else if (m_pFile)
    {
        if (m_nStreamHandle)
        {
            m_pAudioSystem->CloseStream(m_nStreamHandle);
            m_nStreamHandle = 0;
        }
        fclose(m_pFile);
    }
#endif
}

// Whether the source's priority group scales with a caller-supplied factor.
int CExoStreamingSoundSourceInternal::IsScalable()
{
    return m_nPriorityGroup == PRIORITY_GROUP_LOOPING_AREA_WIDE_AMBIENTS || m_nPriorityGroup == PRIORITY_GROUP_SINGLE_SHOT_GLOBAL;
}

void CExoStreamingSoundSourceInternal::SetReverbEnabled(int b)
{
    if (m_bReverbEnabled != b)
    {
        m_bReverbEnabled = b;
        m_bDirty = 1;
    }
}

int CExoStreamingSoundSourceInternal::GetPlayFailed()
{
    return 0;
}

void CExoStreamingSoundSourceInternal::DoObstructionWork()
{
}
// The per-frame 3D update computes pan, distance falloff, the behind-listener factor and the
// occlusion exponent, then pushes the volume and pan. The listener position is local so it survives
// the magnitude call and is reused for the direction, while the source position is re-read from the
// member.
#if KOTOR1
// K1: no 3D-position push or range cut-off; pan and volume are min/max clamps.
void CExoStreamingSoundSourceInternal::Update3DStream()
{
    if (m_vPosition.x == 0.0f && m_vPosition.y == 0.0f && m_vPosition.z == 0.0f)
        return;

    if (!(m_b3D && m_fVolume > 0.0f && m_fMaxDistance > 0.0f
          && m_fMinDistance < m_fMaxDistance))
        return;

    if (g_pExoSound->GetSoundMode() == CExoSound::CEXOSOUND_MODE_PAUSE_ALL
        || g_pExoSound->GetSoundMode() == CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC) {
        m_pAudioSystem->SetChannelVolume(m_nChannel, 0.0f);
        return;
    }

    float fVolume;
    float fPanScale = 0.5f;
    Vector vOrientation;
    Vector vDirection;
    vOrientation = m_pSound->m_vListenerOrientation;
    Vector vListenerPosition = m_pSound->m_vListenerPosition;
    fVolume = m_fVolume;
    float fDistance = magnitude(m_vPosition - vListenerPosition);

    if (fDistance > m_fMinDistance)
        fVolume = fVolume * ((m_fMaxDistance - fDistance) / (m_fMaxDistance - m_fMinDistance));

    vDirection = m_vPosition - vListenerPosition;
    vDirection.Normalize();
    vOrientation.Normalize();

    float fDot = vOrientation.x * vDirection.x + vOrientation.y * vDirection.y;

    if (vOrientation.y > 0.0f) {
        if (vDirection.y > 0.0f) {
            if (vOrientation.x < vDirection.x)
                fPanScale = 1.0f - __builtin_fabsf(fDot) * 0.5f;
            else
                fPanScale = __builtin_fabsf(fDot) * 0.5f;
        } else {
            if (vOrientation.x > -vDirection.x)
                fPanScale = 1.0f - __builtin_fabsf(fDot) * 0.5f;
            else
                fPanScale = __builtin_fabsf(fDot) * 0.5f;
        }
    } else {
        if (vDirection.y < 0.0f) {
            if (vOrientation.x > vDirection.x)
                fPanScale = 1.0f - __builtin_fabsf(fDot) * 0.5f;
            else
                fPanScale = __builtin_fabsf(fDot) * 0.5f;
        } else {
            if (vOrientation.x < -vDirection.x)
                fPanScale = 1.0f - __builtin_fabsf(fDot) * 0.5f;
            else
                fPanScale = __builtin_fabsf(fDot) * 0.5f;
        }
    }

    fPanScale = std::min(0.95f, std::max(0.05f, fPanScale));

    if (fDot < 0.0f)
        fVolume = fVolume * (fDot * 0.125f + 1.0f);

    fVolume = powf(fVolume * (1.0f - m_fOcclusionFactor), 0.6f);
    fVolume = std::min(1.0f, std::max(0.0f, fVolume));

    m_pAudioSystem->SetChannelVolume(m_nChannel, fVolume);
    m_pAudioSystem->SetChannelPan(m_nChannel, fPanScale * 2.0f - 1.0f);

    if (m_bDirty)
        m_bDirty = 0;
}
#else
void CExoStreamingSoundSourceInternal::Update3DStream()
{
    if (m_b3D)
        m_pAudioSystem->SetChannel3DPosition(m_nChannel, m_vPosition.x, m_vPosition.y,
                                             -m_vPosition.z);

    if (m_vPosition.x == 0.0f && m_vPosition.y == 0.0f && m_vPosition.z == 0.0f)
        return;

    if (!(m_b3D && m_fVolume > 0.0f && m_fMaxDistance > 0.0f
          && m_fMinDistance < m_fMaxDistance))
        return;

    if (g_pExoSound->GetSoundMode() == CExoSound::CEXOSOUND_MODE_PAUSE_ALL
        || g_pExoSound->GetSoundMode() == CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC) {
        m_pAudioSystem->SetChannelVolume(m_nChannel, 0.0f);
        return;
    }

    Vector vDirection;
    Vector vOrientation = m_pSound->m_vListenerOrientation;
    Vector vListenerPosition = m_pSound->m_vListenerPosition;
    float fVolume = m_fVolume;
    float fDistance = magnitude(m_vPosition - vListenerPosition);

    if (fDistance > m_fMinDistance && m_fMinDistance != m_fMaxDistance) {
        if (fDistance > m_fMaxDistance)
            fVolume = 0.0f;
        else
            fVolume = fVolume * ((m_fMaxDistance - fDistance)
                                   / (m_fMaxDistance - m_fMinDistance));
    }

    vDirection = m_vPosition - vListenerPosition;
    vDirection.Normalize();
    vOrientation.Normalize();

    float fDot = vOrientation.x * vDirection.x + vOrientation.y * vDirection.y;

    float fPanScale;
    if (vOrientation.y > 0.0f) {
        if (vDirection.y > 0.0f) {
            if (vOrientation.x < vDirection.x)
                fPanScale = 1.0f - __builtin_fabsf(fDot) * 0.5f;
            else
                fPanScale = __builtin_fabsf(fDot) * 0.5f;
        } else {
            if (vOrientation.x > -vDirection.x)
                fPanScale = 1.0f - __builtin_fabsf(fDot) * 0.5f;
            else
                fPanScale = __builtin_fabsf(fDot) * 0.5f;
        }
    } else {
        if (vDirection.y < 0.0f) {
            if (vOrientation.x > vDirection.x)
                fPanScale = 1.0f - __builtin_fabsf(fDot) * 0.5f;
            else
                fPanScale = __builtin_fabsf(fDot) * 0.5f;
        } else {
            if (vOrientation.x < -vDirection.x)
                fPanScale = 1.0f - __builtin_fabsf(fDot) * 0.5f;
            else
                fPanScale = __builtin_fabsf(fDot) * 0.5f;
        }
    }

    fPanScale = fPanScale <= 0.05f ? 0.05f : fPanScale;
    float fPan = fPanScale > 0.95f ? 0.9f : fPanScale * 2.0f - 1.0f;

    if (fDot < 0.0f)
        fVolume = fVolume * (fDot * 0.125f + 1.0f);

    fVolume = fVolume * (1.0f - m_fOcclusionFactor);
    if (fVolume < 0.0f) {
        fVolume = 0.25f;
    } else if (fVolume != 0.0f) {
        fVolume = powf(fVolume, 0.6f);
        if (fVolume < 0.0f)
            fVolume = 0.0f;
        fVolume = fVolume >= 1.0f ? 1.0f : fVolume;
    }

    m_pAudioSystem->SetChannelVolume(m_nChannel, fVolume);
    m_pAudioSystem->SetChannelPan(m_nChannel, fPan);

    if (m_bDirty)
        m_bDirty = 0;
}
#endif

#if KOTOR1
// K1: streams open through the main OBB as an SDL_RWops, and the .wav arm only tries the resref's
// own path.
void CExoStreamingSoundSourceInternal::InitializeSource()
{
    const int RIFF = 0x46464952;      // "RIFF", little-endian

    if (g_bModifiedSound == 0)
    {
        if (m_pSound->m_bInitialized == 0)
            return;

        std::string sPath(m_sStreamPath.CStr());
        std::replace(sPath.begin(), sPath.end(), '\\', '/');

        m_pFile = g_mainObb->RWFromFile(sPath.c_str());
        SDL_RWseek(m_pFile, 0, RW_SEEK_END);
        m_nStreamLength = static_cast<int>(SDL_RWtell(m_pFile));
        SDL_RWseek(m_pFile, 0, RW_SEEK_SET);
        m_nStreamOffset = 0;
        m_nStreamHandle = m_pAudioSystem->CreateStream(m_sStreamPath.CStr(), m_pFile, m_nStreamOffset,
                                                m_nStreamLength, 0, m_b3D, m_bLooping);
        m_nChannel = m_pAudioSystem->PlayStream(m_nStreamHandle, 1);
        return;
    }

    if (m_pSound->m_bInitialized == 0)
        return;

    if (m_pFile)
    {
        m_pAudioSystem->CloseStream(m_nStreamHandle);
        SDL_RWclose(m_pFile);
        m_nStreamHandle = 0;
        m_pFile = 0;
        m_nStreamOffset = 0;
        m_nStreamLength = 0;
    }
    m_pFile = 0;

    if (m_nResType == RESTYPE_MP3)
    {
        CExoString sBase = m_sStreamPath.Left(m_sStreamPath.GetLength() - 3) + CExoString("wav");

        std::string sWav(sBase.CStr());
        std::replace(sWav.begin(), sWav.end(), '\\', '/');
        m_pFile = g_mainObb->RWFromFile(sWav.c_str());

        int nSkip = 1;
        if (m_pFile)
        {
            int nMagic;
            SDL_RWread(m_pFile, &nMagic, 1, 4);
            if (nMagic == RIFF)
            {
                int nChunk;
                SDL_RWread(m_pFile, &nChunk, 4, 1);
                SDL_RWseek(m_pFile, 0, RW_SEEK_END);
                int nFileSize = static_cast<int>(SDL_RWtell(m_pFile));
                if (nChunk + 8 != nFileSize)
                {
                    SDL_RWseek(m_pFile, nChunk + 8, RW_SEEK_SET);
                    m_nStreamOffset = nChunk + 8;
                    m_nStreamLength = nFileSize - (nChunk + 8);
                    goto DoCreate;
                }
                m_pAudioSystem->CloseStream(m_nStreamHandle);
            }
            else
            {
                m_pAudioSystem->CloseStream(m_nStreamHandle);
            }
            SDL_RWclose(m_pFile);
            m_nStreamHandle = 0;
            m_pFile = 0;
            m_nStreamOffset = 0;
            m_nStreamLength = 0;
        }

        {
            std::string sWavPath(sBase.CStr());
            std::replace(sWavPath.begin(), sWavPath.end(), '\\', '/');
            m_pFile = g_mainObb->RWFromFile(sWavPath.c_str());
            if (m_pFile)
            {
                SDL_RWseek(m_pFile, 0, RW_SEEK_END);
                m_nStreamLength = static_cast<int>(SDL_RWtell(m_pFile));
                SDL_RWseek(m_pFile, 0, RW_SEEK_SET);
                nSkip = 0;
            }
        }

        if (nSkip == 0)
        {
DoCreate:
            m_nStreamHandle = m_pAudioSystem->CreateStream(m_sStreamPath.CStr(), m_pFile, m_nStreamOffset,
                                                    m_nStreamLength, nSkip, m_b3D, m_bLooping);
            m_nChannel = m_pAudioSystem->PlayStream(m_nStreamHandle, 1);
        }
        return;
    }

    std::string sPath(m_sStreamPath.CStr());
    std::replace(sPath.begin(), sPath.end(), '\\', '/');

    m_pFile = g_mainObb->RWFromFile(sPath.c_str());
    if (m_pFile == 0)
        return;

    int nMagic;
    int nOffset = 0;
    int nSize;
    SDL_RWread(m_pFile, &nMagic, 1, 4);
    if (nMagic != RIFF)
    {
        nOffset = WAV_CUSTOM_HEADER_SIZE;
        SDL_RWseek(m_pFile, nOffset, RW_SEEK_SET);
        SDL_RWread(m_pFile, &nMagic, 1, 4);
        if (nMagic != RIFF)
        {
            m_pAudioSystem->CloseStream(m_nStreamHandle);
            SDL_RWclose(m_pFile);
            m_nStreamHandle = 0;
            m_pFile = 0;
            m_nStreamOffset = 0;
            m_nStreamLength = 0;
            return;
        }
        SDL_RWseek(m_pFile, 0, RW_SEEK_END);
        nSize = static_cast<int>(SDL_RWtell(m_pFile)) - nOffset;
        m_nStreamOffset = nOffset;
    }
    else
    {
        SDL_RWseek(m_pFile, 0, RW_SEEK_END);
        nSize = static_cast<int>(SDL_RWtell(m_pFile));
    }
    m_nStreamLength = nSize;
    SDL_RWseek(m_pFile, nOffset, RW_SEEK_SET);
    m_nStreamHandle = m_pAudioSystem->CreateStream(m_sStreamPath.CStr(), m_pFile, m_nStreamOffset,
                                            m_nStreamLength, 0, m_b3D, m_bLooping);
    m_nChannel = m_pAudioSystem->PlayStream(m_nStreamHandle, 1);
}
#else
// The streaming-open state machine. Three entry shapes, in this order:
// (a) unmodified sound resources: normalise the resref's path separators and stream the file from
//     offset 0;
// (b) stream type other than 8: probe for the wave-file magic at offset 0, else after a 470-byte
//     prefix, and stream either the whole file or the tail after that prefix;
// (c) stream type 8: build ".wav" candidate names from the resref and the candidate ring, try each
//     one plus a DOS-to-Mac-path variant, and for a candidate that really is a wave chunk skip the
//     chunk, so the stream starts at the chunk size plus 8 with the remaining length.
//
// The skip flag does two jobs. It is the stream-creation argument (1 starts the stream after a wave
// chunk) and the guard for that call, since the all-candidates-failed path leaves it at 1.
void CExoStreamingSoundSourceInternal::InitializeSource()
{
    const int RIFF = 0x46464952;      // "RIFF", little-endian

    if (m_nChannel == 0)
    {
        if (m_nType == 0 && g_generalStreamingHandlesAvailable < 1)
            return;
    }

    if (g_bModifiedSound == 0)
    {
        if (m_pSound->m_bInitialized == 0)
            return;

        std::string sPath(m_sStreamPath.CStr());
        std::replace(sPath.begin(), sPath.end(), '\\', '/');

        m_pFile = fopen(sPath.c_str(), "rb");
        fseek(m_pFile, 0, 2 /*SEEK_END*/);
        m_nStreamLength = static_cast<int>(ftell(m_pFile));
        fseek(m_pFile, 0, 0 /*SEEK_SET*/);
        m_nStreamOffset = 0;
        m_nStreamHandle = m_pAudioSystem->CreateStream(m_sStreamPath.CStr(), m_pFile, m_nStreamOffset,
                                                m_nStreamLength, 0, m_b3D, m_bLooping);
        m_nChannel = m_pAudioSystem->PlayStream(m_nStreamHandle, 1);
        return;
    }

    if (m_pSound->m_bInitialized == 0)
        return;

    if (m_pFile)
    {
        m_pAudioSystem->CloseStream(m_nStreamHandle);
        fclose(m_pFile);
        m_nStreamHandle = 0;
        m_pFile = 0;
        m_nStreamOffset = 0;
        m_nStreamLength = 0;
    }
    m_pFile = 0;

    if (m_nResType == RESTYPE_MP3)
    {
        char* pszName = m_sStreamPath.m_sString;
        int nNameLen = pszName ? static_cast<int>(strlen(pszName)) : 0;
        CExoString sBase = m_sStreamPath.Left(nNameLen - 3) + CExoString("wav");

        CExoString asNames[16];
        for (int i = 0; i < 16; i++)
        {
            if (m_asOverridePaths[i][0] == '\0')
                break;
            char* pszSlot = m_asOverridePaths[i].m_sString;
            int nSlotLen = pszSlot ? static_cast<int>(strlen(pszSlot)) : 0;
            asNames[i] = m_asOverridePaths[i].Left(nSlotLen - 3) + CExoString("wav");
        }

        // Normalised but never read, since the conversion below takes sBase. Kept because the
        // string's allocation and release are side effects.
        std::string sWav(sBase.CStr());
        std::replace(sWav.begin(), sWav.end(), '\\', '/');

        char szPath[1024];   // the DOS2MacPath destination
        DOS2MacPath(sBase.CStr(), szPath);

        m_pFile = fopen(asNames[0].CStr(), "rb");
        if (!m_pFile) m_pFile = fopen(asNames[1].CStr(), "rb");
        if (!m_pFile) m_pFile = fopen(asNames[2].CStr(), "rb");
        if (!m_pFile) m_pFile = fopen(asNames[3].CStr(), "rb");
        if (!m_pFile) m_pFile = fopen(asNames[4].CStr(), "rb");
        if (!m_pFile) m_pFile = fopen(asNames[5].CStr(), "rb");
        if (!m_pFile) m_pFile = fopen(asNames[6].CStr(), "rb");
        if (!m_pFile) m_pFile = fopen(asNames[7].CStr(), "rb");
        if (!m_pFile) m_pFile = fopen(asNames[8].CStr(), "rb");
        if (!m_pFile) m_pFile = fopen(asNames[9].CStr(), "rb");
        if (!m_pFile) m_pFile = fopen(asNames[10].CStr(), "rb");
        if (!m_pFile) m_pFile = fopen(asNames[11].CStr(), "rb");
        if (!m_pFile) m_pFile = fopen(asNames[12].CStr(), "rb");
        if (!m_pFile) m_pFile = fopen(asNames[13].CStr(), "rb");
        if (!m_pFile) m_pFile = fopen(asNames[14].CStr(), "rb");
        if (!m_pFile) m_pFile = fopen(asNames[15].CStr(), "rb");
        if (!m_pFile) m_pFile = fopen(szPath, "rb");

        // 1 when the candidate list fails and 1 again for the chunked stream; 0 once a candidate
        // opened cleanly.
        int nSkip = 1;
        if (m_pFile)
        {
            int nMagic;
            fread(&nMagic, 1, 4, m_pFile);
            if (nMagic == RIFF)
            {
                int nChunk;
                fread(&nChunk, 4, 1, m_pFile);
                fseek(m_pFile, 0, 2 /*SEEK_END*/);
                int nFileSize = static_cast<int>(ftell(m_pFile));
                if (nChunk + 8 != nFileSize)
                {
                    fseek(m_pFile, nChunk + 8, 0 /*SEEK_SET*/);
                    m_nStreamOffset = nChunk + 8;
                    m_nStreamLength = nFileSize - (nChunk + 8);
                    goto DoCreate;
                }
                m_pAudioSystem->CloseStream(m_nStreamHandle);
            }
            else
            {
                m_pAudioSystem->CloseStream(m_nStreamHandle);
            }
            fclose(m_pFile);
            m_nStreamHandle = 0;
            m_pFile = 0;
            m_nStreamOffset = 0;
            m_nStreamLength = 0;
        }

        {
            std::string sWavPath(sBase.CStr());
            std::replace(sWavPath.begin(), sWavPath.end(), '\\', '/');
            m_pFile = fopen(sWavPath.c_str(), "rb");
            if (m_pFile == 0)
            {
                for (int i = 0; i < 16; i++)
                {
                    if (m_asOverridePaths[i][0] == '\0')
                        break;
                    m_pFile = fopen(m_asOverridePaths[i].CStr(), "rb");
                    if (m_pFile)
                        break;
                }
            }
            if (m_pFile)
            {
                fseek(m_pFile, 0, 2 /*SEEK_END*/);
                m_nStreamLength = static_cast<int>(ftell(m_pFile));
                fseek(m_pFile, 0, 0 /*SEEK_SET*/);
                nSkip = 0;
            }
        }

        if (nSkip == 0)
        {
DoCreate:
            m_nStreamHandle = m_pAudioSystem->CreateStream(m_sStreamPath.CStr(), m_pFile, m_nStreamOffset,
                                                    m_nStreamLength, nSkip, m_b3D, m_bLooping);
            m_nChannel = m_pAudioSystem->PlayStream(m_nStreamHandle, 1);
        }
        return;
    }

    std::string sPath(m_sStreamPath.CStr());
    std::replace(sPath.begin(), sPath.end(), '\\', '/');

#if defined(K2_UI_XBOX) || defined(K2_FIX_ENGINE_BUGS)
    // The console builds (and every UI here) open the override candidates before the resref's own
    // path, as the type-8 arm above does, so a mod's copy of a streamed sound is the one played.
    for (int i = 0; i < 16 && m_pFile == 0; i++)
    {
        if (m_asOverridePaths[i][0] == '\0')
            break;
        m_pFile = fopen(m_asOverridePaths[i].CStr(), "rb");
    }
    if (m_pFile == 0)
#endif
    m_pFile = fopen(sPath.c_str(), "rb");
    if (m_pFile == 0)
        return;

    int nMagic;
    int nOffset = 0;
    int nSize;
    fread(&nMagic, 1, 4, m_pFile);
    if (nMagic != RIFF)
    {
        nOffset = WAV_CUSTOM_HEADER_SIZE;
        fseek(m_pFile, nOffset, 0 /*SEEK_SET*/);
        fread(&nMagic, 1, 4, m_pFile);
        if (nMagic != RIFF)
        {
            m_pAudioSystem->CloseStream(m_nStreamHandle);
            fclose(m_pFile);
            m_nStreamHandle = 0;
            m_pFile = 0;
            m_nStreamOffset = 0;
            m_nStreamLength = 0;
            return;
        }
        fseek(m_pFile, 0, 2 /*SEEK_END*/);
        nSize = static_cast<int>(ftell(m_pFile)) - nOffset;
        m_nStreamOffset = nOffset;
    }
    else
    {
        fseek(m_pFile, 0, 2 /*SEEK_END*/);
        nSize = static_cast<int>(ftell(m_pFile));
    }
    m_nStreamLength = nSize;
    fseek(m_pFile, nOffset, 0 /*SEEK_SET*/);
    m_nStreamHandle = m_pAudioSystem->CreateStream(m_sStreamPath.CStr(), m_pFile, m_nStreamOffset,
                                            m_nStreamLength, 0, m_b3D, m_bLooping);
    m_nChannel = m_pAudioSystem->PlayStream(m_nStreamHandle, 1);
}
#endif
