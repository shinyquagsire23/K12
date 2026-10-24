#pragma once

#include <stddef.h>

// One rumble motor's sampled magnitude envelope: two parallel heap float arrays.
class CSWRumbleMotorData {
public:
    float* m_paMagnitudes;
    float* m_paSamplePoints;
    int    m_nNumSamplePoints;
    int    m_nCurrentIndex;

    CSWRumbleMotorData();
#if !KOTOR1  // K1 has no destructor; the sample arrays are never freed
    ~CSWRumbleMotorData();
#endif

    void SetNumSamplePoints(int nCount);
    void SetSamplePoint(int nIndex, float fMagnitude, float fTime);
    float GetMagnitude(float fSamplePoint, int bLoop);
};

#include <stddef.h>

class C2DA;

// One rumble pattern: the left and right motors' envelopes and the looping flag.
class CSWRumblePattern {
public:
    CSWRumbleMotorData m_left;
    CSWRumbleMotorData m_right;
    int                m_nLooping;

    CSWRumblePattern();
    void LoadPattern(int nRow, C2DA* p2DA);
    int  GetMagnitudes(float fSamplePoint, float& fMagLeft, float& fMagRight);
};
