// The three FModAudioSystem bookkeeping records. They have a separate translation unit because
// each special member is reached through a call from FModAudioSystem.cpp, so it must not be visible
// there.

#include "FModAudioSystemInfo.h"

// The engine's "this property is not set" sentinel for every float channel property.
// <float.h> cannot be included from this TU: bionic's reaches a header that declares atoi(),
// which main.cpp declares again, so the value is spelled out.
static const float kFltMax = __FLT_MAX__;


FModAudioSystemSoundInfo::FModAudioSystemSoundInfo()
{
    m_nRefCount = 0;
    m_pSound = 0;
}

FModAudioSystemSoundInfo::~FModAudioSystemSoundInfo()
{
}

FModAudioSystemStreamInfo::FModAudioSystemStreamInfo()
{
    m_nStreamId = 0;
    m_pFile = 0;
    m_pSound = 0;
    m_nOffset = 0;
    m_nSize = 0;
}

FModAudioSystemStreamInfo::~FModAudioSystemStreamInfo()
{
}

FModAudioSystemChannelInfo::FModAudioSystemChannelInfo()
{
    m_nUID = -1;
    m_nPriority = -1;
    _reserved08 = -1;
    m_bIsStream = 0;
    m_bInUse = 0;
    m_bNoLoop = 0;
    m_bPaused = 0;
    m_bDone = 0;
    m_bStopped = 0;
    m_fVolume = kFltMax;
    m_fFrequency = kFltMax;
    m_fMinDistance = kFltMax;
    m_fMaxDistance = kFltMax;
    m_fDirectOcclusion = kFltMax;
    m_fReverbOcclusion = kFltMax;
    m_fPan = kFltMax;
    m_pVec3DPosition = 0;
    m_pChannel = 0;
    m_pSound = 0;
}

// The channel owns its 3D position vector.
FModAudioSystemChannelInfo::~FModAudioSystemChannelInfo()
{
    if (m_pVec3DPosition != 0)
        operator delete(m_pVec3DPosition);
}

// The uid survives a recycle. Every other field goes back to its initial value and the
// owned 3D position vector is released.
void FModAudioSystemChannelInfo::Reset()
{
    m_nPriority = -1;
    _reserved08 = -1;
    m_bIsStream = 0;
    m_bInUse = 0;
    m_bNoLoop = 0;
    m_bPaused = 0;
    m_bDone = 0;
    m_bStopped = 0;
    m_fVolume = kFltMax;
    m_fFrequency = kFltMax;
    m_fMinDistance = kFltMax;
    m_fMaxDistance = kFltMax;
    m_fDirectOcclusion = kFltMax;
    m_fReverbOcclusion = kFltMax;
    m_fPan = kFltMax;
    m_pChannel = 0;
    m_pSound = 0;
    if (m_pVec3DPosition != 0) {
        operator delete(m_pVec3DPosition);
        m_pVec3DPosition = 0;
    }
}
