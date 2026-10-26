#ifndef FMODAUDIOSYSTEMINFO_H
#define FMODAUDIOSYSTEMINFO_H

// The three bookkeeping records FModAudioSystem's maps own. Their definitions stay in
// FModAudioSystemInfo.cpp so that FModAudioSystem.cpp keeps calling them out of line.

#include "FModAPI.h"
#if KOTOR1
#include "sdlrwops.h"
#endif

class FModAudioSystemSoundInfo
{
public:
    FModAudioSystemSoundInfo();
    ~FModAudioSystemSoundInfo();

    int           m_nRefCount;
    int           _reserved04;
    FMOD::Sound*  m_pSound;
};

class FModAudioSystemStreamInfo
{
public:
    FModAudioSystemStreamInfo();
    ~FModAudioSystemStreamInfo();

    int           m_nStreamId;
    int           _reserved04;
#if KOTOR1  // K1: the stream handle is an SDL_RWops
    SDL_RWops*    m_pFile;
#else
    __sFILE*      m_pFile;
#endif
    FMOD::Sound*  m_pSound;
    int           m_nOffset;
    int           m_nSize;
};

// One channel. The floats are seeded FLT_MAX, the sentinel for "this property is not set": the
// SetChannel* calls only push a value to the mixer when it differs from that, and a recycled channel is
// re-applied with exactly the properties that are not still FLT_MAX.
class FModAudioSystemChannelInfo
{
public:
    FModAudioSystemChannelInfo();
    ~FModAudioSystemChannelInfo();
    void Reset();
    // Takes ownership of the 3D position, freeing the one it replaces.
    void SetPosition(FMOD_VECTOR* pPosition)
    {
        if (m_pVec3DPosition != 0 && m_pVec3DPosition != pPosition)
            delete m_pVec3DPosition;
        m_pVec3DPosition = pPosition;
    }

    int            m_nUID;
    int            m_nPriority;
    int            _reserved08;
    int            m_bIsStream;
    int            m_bInUse;
    int            m_bNoLoop;
    int            m_bPaused;
    int            m_bDone;
    int            m_bStopped;
    float          m_fVolume;
    float          m_fFrequency;
    float          m_fMinDistance;
    float          m_fMaxDistance;
    float          m_fDirectOcclusion;
    float          m_fReverbOcclusion;
    float          m_fPan;
    FMOD_VECTOR*   m_pVec3DPosition;
    FMOD::Channel* m_pChannel;
    FMOD::Sound*   m_pSound;
};

#endif // FMODAUDIOSYSTEMINFO_H
