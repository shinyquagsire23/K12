#pragma once
#include "k2stdint.h"

#include "Vector.h"
#include "exobase.h"
#include "CExoArrayListDtors.h"
#include "CExoLinkedList.h"
#include "exosound.h"
#include "FModAudioSystem.h"


#include "prioritygroup.h"

class CExoStreamingSoundSourceInternal;
class CExoStreamingSoundSource;
class CExoSoundSourceInternal;
class CAllowedSoundProvider;
class CResRef;

#if KOTOR1  // K1: the FMOD channel handle is 64-bit
typedef uint64_t ExoSoundChannel;
#else
typedef int ExoSoundChannel;
#endif
static const ExoSoundChannel EXOSOUND_NO_CHANNEL = static_cast<ExoSoundChannel>(-1);

struct C2DVoice {
    CExoSoundSourceInternal* m_pSource;
#if KOTOR1  // K1: the FMOD channel handle is 64-bit
    uint64_t m_nChannel;
    int  m_bInUse;
    int  m_nReserved1;
#else
    int  m_nChannel;
    int  m_bInUse;
    int  m_nReserved1;
    char _pad14[0x18 - 0x14];
#endif
};
struct C3DVoice {
    CExoSoundSourceInternal* m_pSource;
#if KOTOR1
    uint64_t m_nChannel;
    int  m_bInUse;
    int  m_nReserved1;
#else
    int  m_nChannel;
    int  m_bInUse;
    int  m_nReserved1;
    char _pad14[0x18 - 0x14];
#endif
};


class CExoSoundInternal {
public:
    CExoSoundInternal();
    ~CExoSoundInternal();

    void SetEnvironment(unsigned char env);
#if KOTOR1  // K1 has no out-of-line GetPriorityGroup and no range fallback
    CPriorityGroup* GetPriorityGroup(unsigned char group) { return &m_paGroups[group]; }
#else
    CPriorityGroup* GetPriorityGroup(unsigned char group);
#endif
    void AddToNonPlayingStreamingLoopingList(CExoStreamingSoundSourceInternal* pSource);

    void AddToPlayingList(CExoSoundSourceInternal* pSource);
    int  AddToStreamingList(CExoStreamingSoundSourceInternal* pSource);
    void PlaySourceOn2DVoice(CExoSoundSourceInternal* pSource, C2DVoice* pVoice);
    void PlaySourceOn3DVoice(CExoSoundSourceInternal* pSource, C3DVoice* pVoice);
    void InterruptPlayingStream(unsigned char nGroup);
    void InterruptPlayingSound(unsigned char nGroup);
    int  ResumeAllSounds();
    int  PauseAllSounds(int bPause);
    void PauseAllButStreamingMusicForMovie();
    int  GetSliderType(CExoStreamingSoundSourceInternal* pSource);
    int  GetSliderType(CExoSoundSourceInternal* pSource);
    float GetSliderVolume(int nSliderType, int nRaw);
    int  GetNumber2DSamplesPlaying();
    int  GetNumber3DSamplesPlaying();
    int  GetPauseable(CExoStreamingSoundSourceInternal* pSource);
    int  GetPauseable(CExoSoundSourceInternal* pSource);

    int  GetClosestRecentSound(const Vector& vSourcePos, Vector& vClosestPos,
                               unsigned int& nTime, float& fFactor);

    void LoadPriorityGroups();
    void ShutDown();

    void MuteSound();
    void UnMuteSound();
    void AddToDelayedOneShotList(CExoSoundSourceInternal* pSource);
    void AddToNonPlayingLoopingList(CExoSoundSourceInternal* pSource);
    CExoLinkedListNode* RemoveFromDelayedOneShotList(CExoSoundSourceInternal* pSource);
    CExoLinkedListNode* RemoveFromNonPlayingLoopingList(CExoSoundSourceInternal* pSource);
    CExoLinkedListNode* RemoveFromPlayingList(CExoSoundSourceInternal* pSource);
    CExoLinkedListNode* RemoveFromStreamingList(CExoStreamingSoundSourceInternal* pSource);
    CExoLinkedListNode* RemoveFromNonPlayingStreamingLoopingList(
        CExoStreamingSoundSourceInternal* pSource);

    C3DVoice* GetFree3DVoice(unsigned char nGroup);
    C2DVoice* GetFree2DVoice(unsigned char nGroup);

    bool Initialize(unsigned char n2DVoices, unsigned char n3DVoices, int a, int b);
    void StartDuckingMusicAndAmbientSounds();
    void StopDuckingMusicAndAmbientSounds();
    void SetMusicVolume(float fVolume, int a, int nChannel);
    void SetSoundEffectVolume(float fVolume, int a);
    void SetDialogVolume(float fVolume, int a);
    void ApplyNewVolume(int a);
    int  Test2D3DBias();
    void SetRollOffFactor(float f);
    void Render();
    void OcclusionRender(unsigned int nMaxTime);
    void SetListenerOrientation(const Vector& vDir, const Vector& vUp);
    void SetListenerPosition(const Vector& vPos);
    int  PlayOneShotSound(const CResRef& r, unsigned char a, unsigned int b, unsigned char c,
                          float d, float e);
    int  Play3DOneShotSound(const CResRef& r, Vector vPos, unsigned char a, unsigned int b,
                            unsigned char c, float d, float e);
    void StopAllOneShots();
    int  GetSpeakerType();
    void SetEnvironmentEffectLevel(float f);
    int  RemoveAllSounds();
    void ResumeAllStreamingMusicForMovie();
    void SetSoundMode(int mode, int flag);
#if KOTOR1
    void SetSoundMode(int mode);
#endif

    void Enumerate3DProviders();
    void LoadProviderPreferences();
    int GetMutable(CExoStreamingSoundSourceInternal* pSource);
    int GetMutable(CExoSoundSourceInternal* pSource);

#if !KOTOR1  // K1 has no music or ambient ducking
    float         m_fDuckVolume;
#endif
    int           m_bStreamingEnvironmentEffects;
    int           m_bNonStreamingEnvironmentEffects;
    int           m_bInitialized;
    int           m_bMuted;
    unsigned char m_environment;
#if KOTOR1
    char          _pad15[3];
#else
    char          _pad15[0x18 - 0x15];
#endif
    float         m_fAmbientScale;
    float         m_fAmbientScaleRampStart;
    float         m_fLastDesiredAmbientScale;
    int           m_nDesiredEnvironment;
    float         m_fDesiredAmbientScale;
    unsigned char m_nNumber2DVoices;
    unsigned char m_nNumber3DVoices;
    unsigned char m_maxGroup;
#if KOTOR1  // K1: the tail here is padding
    char          _pad2f[5];
#else
    char          _pad2f[0x30 - 0x2f];
#endif
    C2DVoice* m_pa2DVoices;
    C3DVoice* m_pa3DVoices;
    CExoLinkedList<CExoSoundSourceInternal> m_lDelayedOneShots;
    CExoLinkedList<CExoSoundSourceInternal> m_lNonPlayingLooping;
    CExoLinkedList<CExoStreamingSoundSourceInternal> m_lNonPlayingStreamingLooping;
    CExoLinkedList<CExoSoundSourceInternal> m_lPlaying;
    CExoLinkedList<CExoStreamingSoundSourceInternal> m_lStreaming;
    CExoLinkedList<CExoSoundSourceInternal> m_lStoppingOneShots;
    CPriorityGroup* m_paGroups;
    unsigned int  m_nMuteTimerStart;
    unsigned int  m_nMuteDuration;
    int           m_nNextStreamingVoiceId;
    unsigned int  m_nUnMuteDuration;
    float         m_f2D3DBias;
    float         m_fDialogVolume;
    float         m_fEnvironmentEffectLevel;
    float         m_fMusicVolumeRaw;
    float         m_fSoundEffectVolumeRaw;
    float         m_fDialogVolumeRaw;
    float         m_fMusicVolume;
    float         m_fSoundEffectVolume;
    Vector        m_vListenerOrientation;
    Vector        m_vListenerUp;
    Vector        m_vListenerPosition;
    char          _padcc[0xd0 - 0xcc];
    CExoLinkedListNode* m_pOcclusionCursor;
    int           m_nOcclusionCursorList;
    unsigned int  m_nOcclusionIdleFrames;
    unsigned int  m_nOcclusionIdleFramesMax;
    unsigned int  m_nOcclusionTimeUsedMax;
    unsigned int  m_nObstructionTestsMax;
    unsigned int  m_nOcclusionMaxTimeHigh;
    unsigned int  m_nOcclusionTimeBudget;
    unsigned int  m_nOcclusionTimeUsed;
    unsigned int  m_nObstructionTests;
    int           m_bDisplayOcclusionInfo;
    int           m_nReserved1;
    int           m_nReserved2;
    int           m_bHardwareSound;
    int           m_nEAX;
    int           m_nBestEAXAvailable;
    char          _pad114[0x118 - 0x114];
    CExoArrayList<CAllowedSoundProvider*> m_aProviders;
    int           m_nLastRenderTime;
    int           m_aSoundModes[10];
    int           m_nSoundModeIndex;
    int           m_bLostFocusPaused;
    char          _pad15c[0x160 - 0x15c];
    CExoStreamingSoundSource* m_pStreamingSoundSource;
    FModAudioSystem m_audioSystem;
};
#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CExoSoundInternal) == 0x1c0, "K1 CExoSound ctor @0x67b74c: operator new(0x1c0)");
#endif

extern int g_bSoundPaused;

float GetObstructionFactor(Vector vFrom, Vector vTo);
