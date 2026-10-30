#pragma once
#include "k2layout.h"

// One sound priority-group bucket; the fields are the soundprioritygroups.2da row.
class CPriorityGroup {
public:
    CPriorityGroup();
    ~CPriorityGroup();

    int           m_nInterrupt;
    unsigned char m_nMaxPlaying;
    unsigned char m_nPlayingCount;
    unsigned char m_nPriority;
    unsigned char m_nVolume;
    float m_fMaxDistance;
    float m_fMinDistance;
    float m_fPlaybackVariance;
    unsigned short m_nFadeTime;
};

K2_LAYOUT_ASSERT(sizeof(CPriorityGroup) == 0x18, "CPriorityGroup is the 0x18-byte soundprioritygroups.2da row");
