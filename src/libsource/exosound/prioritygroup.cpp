#include "prioritygroup.h"

CPriorityGroup::CPriorityGroup()
{
    m_nPriority = 0;
    m_nVolume = 127;
    m_nMaxPlaying = 255;
    m_nInterrupt = 1;
    m_fMinDistance = 10.0f;
    m_fMaxDistance = 20.0f;
    m_nFadeTime = 0;
    m_nPlayingCount = 0;
    m_fPlaybackVariance = 0.0f;
}

CPriorityGroup::~CPriorityGroup()
{
}
