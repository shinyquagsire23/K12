#pragma once
#include "k2stdint.h"

// The pimpl behind a streaming (music, ambient, dialog) sound source. The class is not polymorphic:
// CExoStreamingSoundSource's destructor calls this one's directly and then deletes the object.

#include "Vector.h"
#include "exolinkedlistinternal.h"
#include "exostring.h"
#include "exoresman.h"

class CExoSoundInternal;
class CPriorityGroup;
class FModAudioSystem;

// The stdio surface is declared by hand, as the build does not use the builtin declarations.
#include "port/libcbase.h"

#if KOTOR1
struct SDL_RWops;
#endif

class CExoStreamingSoundSourceInternal {
public:
#if KOTOR1 // K1 layout: 64-bit channel/stream ids, no stream type, one path string, an SDL stream and no movie flag
    uint64_t m_nChannel;
    uint64_t m_nStreamHandle;
    int           m_bLooping;
    unsigned char m_nPriorityGroup;
    CResRef       m_resRef;
    unsigned char _gap25[0x28 - 0x25];
    CExoLinkedListNode* m_pListNode;
    CExoSoundInternal*  m_pSound;
    FModAudioSystem* m_pAudioSystem;
    CExoString    m_sStreamPath;
    unsigned int  m_nFadeDuration;
    int           m_nStreamingVoiceId;
    unsigned int  m_nFadeStartTime;
    SDL_RWops*    m_pFile;
    int           m_nStreamOffset;
    int           m_nStreamLength;
    unsigned char _gap70[0x74 - 0x70];
    int           m_nPlaybackPosition;
    unsigned short m_nResType;
    unsigned char m_nVolume;
    unsigned char _gap7b[1];
    float         m_fMinDistance;
    float         m_fMaxDistance;
    unsigned char m_nGroupVolume;
    unsigned char _gap85[0x88 - 0x85];
    float         m_fVolume;
    int           m_b3D;
    Vector        m_vPosition;
    int           m_bReverbEnabled;
    int           m_bDirty;
    Vector        m_vOcclusionPosition;
    unsigned int  m_nOcclusionTime;
    float         m_fOcclusionFactor;
    unsigned char _gapb8[0xc8 - 0xb8];
#else
    int           m_nChannel;
    int           m_nStreamHandle;
    int           m_nType;
    int           m_bLooping;
    unsigned char m_nPriorityGroup;
    CResRef       m_resRef;
    CExoLinkedListNode* m_pListNode;
    CExoSoundInternal*  m_pSound;
    CExoString    m_sStreamPath;
    FModAudioSystem* m_pAudioSystem;
    // 16 preallocated path strings, constructed and destructed as a block.
    CExoString    m_asOverridePaths[16];
    unsigned int  m_nFadeDuration;
    int           m_nStreamingVoiceId;
    unsigned int  m_nFadeStartTime;
    FILE*         m_pFile;
    int           m_nStreamOffset;
    int           m_nStreamLength;
    unsigned char _gap170[0x174 - 0x170];
    int           m_nPlaybackPosition;
    unsigned short m_nResType;
    unsigned char m_nVolume;
    unsigned char _gap17b[1];
    float         m_fMinDistance;
    float         m_fMaxDistance;
    unsigned char m_nGroupVolume;
    unsigned char _gap185[0x188 - 0x185];
    float         m_fVolume;
    int           m_b3D;
    Vector        m_vPosition;
    int           m_bReverbEnabled;
    int           m_bDirty;
    Vector        m_vOcclusionPosition;
    unsigned int  m_nOcclusionTime;
    float         m_fOcclusionFactor;
    int           m_bStreamingMusicForMovie;
#endif

    CExoStreamingSoundSourceInternal(CExoSoundInternal* pSound, int nType);
#if KOTOR1
    CExoStreamingSoundSourceInternal(CExoSoundInternal* pSound);
#endif
    ~CExoStreamingSoundSourceInternal();

    int  IsScalable();
    void SetReverbEnabled(int b);
    int  GetPlayFailed();
    void DoObstructionWork();

    // The public source class forwards to everything below.
    int  IsPlaying();
    int  IsDone();
    int  GetWithinRangeOfListener();
    int  Play(const CResRef& resRef, unsigned short a, int b, int c, int d);
    void Pause();
    void Resume();
    void InitializeSource();
    void SetLooping(int bLooping);
    void SetVolume(unsigned char nVolume, int a, float f);
    void Update3DStream();
#if KOTOR1
    void Stop();
    void Stop(int) { Stop(); }
#else
    void Stop(int bFade);
#endif
    void FadeAndStop(unsigned int nMilliseconds);
    void SetPriorityGroup(unsigned char nGroup);
};
#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CExoStreamingSoundSourceInternal) == 0xc8, "K1 CExoStreamingSoundSource ctor @0x67b4fc: operator new(0xc8)");
#endif
