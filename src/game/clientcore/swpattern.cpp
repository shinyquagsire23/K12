// Controller rumble sample storage and interpolation.

#include "k2stdint.h"
#include "swpattern.h"
#include "twodimarray.h"
#include <string.h>

CSWRumbleMotorData::CSWRumbleMotorData()
    : m_paMagnitudes(NULL), m_paSamplePoints(NULL), m_nNumSamplePoints(0), m_nCurrentIndex(0)
{
}

#if !KOTOR1  // K2 only; K1 has no equivalent
CSWRumbleMotorData::~CSWRumbleMotorData()
{
    // Plain delete, not delete[], although the arrays come from new[].
    if (m_paMagnitudes != NULL)
#ifdef K2_FIX_ENGINE_BUGS
        delete[] m_paMagnitudes;
#else
        delete m_paMagnitudes;
#endif
    if (m_paSamplePoints != NULL)
#ifdef K2_FIX_ENGINE_BUGS
        delete[] m_paSamplePoints;
#else
        delete m_paSamplePoints;
#endif
}
#endif // !KOTOR1

// Reallocates both arrays; the previous ones are not freed.
void CSWRumbleMotorData::SetNumSamplePoints(int nCount)
{
    if (nCount <= 1) {
        m_nNumSamplePoints = 0;
        return;
    }

    m_nNumSamplePoints = nCount;

    m_paMagnitudes = new float[nCount];
    m_paSamplePoints = new float[nCount];

    for (int i = 0; i < nCount; i++) {
        m_paMagnitudes[i] = 0.0f;
        m_paSamplePoints[i] = 0.0f;
    }
}

void CSWRumbleMotorData::SetSamplePoint(int nIndex, float fMagnitude, float fTime)
{
    if (nIndex >= 0 && nIndex < m_nNumSamplePoints && m_paSamplePoints != NULL && m_paMagnitudes != NULL) {
        m_paMagnitudes[nIndex] = fMagnitude;
        m_paSamplePoints[nIndex] = fTime;
    }
}

float CSWRumbleMotorData::GetMagnitude(float fSamplePoint, int bLoop)
{
    if (m_paSamplePoints == NULL || m_paMagnitudes == NULL)
        return 0.0f;

    int64_t nLast = static_cast<int64_t>(m_nNumSamplePoints) - 1;
    float fLast = m_paSamplePoints[nLast];

    if (bLoop == 0) {
        if (fLast < fSamplePoint)
            return 0.0f;
    } else {
        while (fSamplePoint > fLast)
            fSamplePoint = fSamplePoint - fLast;
    }

    // Scans forward from the cached cursor, then wraps from 0 back up to it; the wrap scan's bound
    // is the cursor captured before the first scan.
    int64_t nCurrent = static_cast<int64_t>(m_nCurrentIndex);
    int i;

    for (i = static_cast<int>(nCurrent); i < nLast; i++) {
        if (fSamplePoint >= m_paSamplePoints[i] && fSamplePoint < m_paSamplePoints[i + 1])
            goto found;
    }

    for (i = 0; i < nCurrent; i++) {
        if (fSamplePoint >= m_paSamplePoints[i] && fSamplePoint < m_paSamplePoints[i + 1])
            goto found;
    }

    return 0.0f;

found:
    m_nCurrentIndex = i;
    return m_paMagnitudes[i] + (m_paMagnitudes[i + 1] - m_paMagnitudes[i]) *
           ((fSamplePoint - m_paSamplePoints[i]) /
            (m_paSamplePoints[i + 1] - m_paSamplePoints[i]));
}

CSWRumblePattern::CSWRumblePattern()
{
    __builtin_memset(static_cast<void*>(this), 0, offsetof(CSWRumblePattern, m_nLooping) + sizeof(m_nLooping));
}

void CSWRumblePattern::LoadPattern(int nRow, C2DA* p2DA)
{
    if (nRow < p2DA->m_nRows) {
        CExoString sColumn;

        p2DA->GetINTEntry(nRow, CExoString("Looping"), &m_nLooping);

        int nSamples = 0;

        p2DA->GetINTEntry(nRow, CExoString("LSamples"), &nSamples);
        int nCount = nSamples;
        if (nCount <= 1) {
            m_left.m_nNumSamplePoints = 0;
        } else {
            m_left.m_nNumSamplePoints = nCount;
            m_left.m_paMagnitudes = new float[nCount];
            m_left.m_paSamplePoints = new float[nCount];
            memset(m_left.m_paMagnitudes, 0, nCount * sizeof(float));
            memset(m_left.m_paSamplePoints, 0, nCount * sizeof(float));

            for (int i = 0; i < nCount; i++) {
                float fMagnitude;
                float fTime;

                sColumn.Format(const_cast<char*>("LMagnitude%d"), i + 1);
                p2DA->GetFLOATEntry(nRow, sColumn, &fMagnitude);
                sColumn.Format(const_cast<char*>("LTime%d"), i + 1);
                p2DA->GetFLOATEntry(nRow, sColumn, &fTime);

                if (i < m_left.m_nNumSamplePoints && m_left.m_paSamplePoints != NULL && m_left.m_paMagnitudes != NULL) {
                    m_left.m_paMagnitudes[i] = fMagnitude;
                    m_left.m_paSamplePoints[i] = fTime;
                }
            }
        }

        p2DA->GetINTEntry(nRow, CExoString("RSamples"), &nSamples);
        nCount = nSamples;
        if (nCount <= 1) {
            m_right.m_nNumSamplePoints = 0;
        } else {
            m_right.m_nNumSamplePoints = nCount;
            m_right.m_paMagnitudes = new float[nCount];
            m_right.m_paSamplePoints = new float[nCount];
            memset(m_right.m_paMagnitudes, 0, nCount * sizeof(float));
            memset(m_right.m_paSamplePoints, 0, nCount * sizeof(float));

            for (int i = 0; i < nCount; i++) {
                float fMagnitude;
                float fTime;

                sColumn.Format(const_cast<char*>("RMagnitude%d"), i + 1);
                p2DA->GetFLOATEntry(nRow, sColumn, &fMagnitude);
                sColumn.Format(const_cast<char*>("RTime%d"), i + 1);
                p2DA->GetFLOATEntry(nRow, sColumn, &fTime);

                if (i < m_right.m_nNumSamplePoints && m_right.m_paSamplePoints != NULL && m_right.m_paMagnitudes != NULL) {
                    m_right.m_paMagnitudes[i] = fMagnitude;
                    m_right.m_paSamplePoints[i] = fTime;
                }
            }
        }
    }
}

// Interpolated magnitude for one channel; the same computation as CSWRumbleMotorData::GetMagnitude.
static inline __attribute__((always_inline)) float GetChannelMagnitude(
    float* pSamplePoints, float* pMagnitudes, int nCount, int& nCurrentIndex,
    float fSamplePoint, int bLoop)
{
    if (pSamplePoints == NULL || pMagnitudes == NULL)
        return 0.0f;

    int64_t nLast = static_cast<int64_t>(nCount) - 1;
    float fLast = pSamplePoints[nLast];

    if (bLoop == 0) {
        if (fLast < fSamplePoint)
            return 0.0f;
    } else {
        while (fSamplePoint > fLast)
            fSamplePoint = fSamplePoint - fLast;
    }

    int64_t nCurrent = static_cast<int64_t>(nCurrentIndex);
    int i;

    for (i = static_cast<int>(nCurrent); i < nLast; i++) {
        if (fSamplePoint >= pSamplePoints[i] && fSamplePoint < pSamplePoints[i + 1])
            goto found;
    }

    for (i = 0; i < nCurrent; i++) {
        if (fSamplePoint >= pSamplePoints[i] && fSamplePoint < pSamplePoints[i + 1])
            goto found;
    }

    return 0.0f;

found:
    nCurrentIndex = i;
    return pMagnitudes[i] + (pMagnitudes[i + 1] - pMagnitudes[i]) *
           ((fSamplePoint - pSamplePoints[i]) / (pSamplePoints[i + 1] - pSamplePoints[i]));
}

int CSWRumblePattern::GetMagnitudes(float fSamplePoint, float& fMagLeft, float& fMagRight)
{
    fMagLeft = GetChannelMagnitude(m_left.m_paSamplePoints, m_left.m_paMagnitudes, m_left.m_nNumSamplePoints,
                                   m_left.m_nCurrentIndex, fSamplePoint, m_nLooping);
    fMagRight = GetChannelMagnitude(m_right.m_paSamplePoints, m_right.m_paMagnitudes, m_right.m_nNumSamplePoints,
                                    m_right.m_nCurrentIndex, fSamplePoint, m_nLooping);

    // A looping pattern stays alive; otherwise it ends past the last sample of either channel.
    if (m_nLooping != 0)
        return 1;

    float fLeftEnd = 0.0f;
    int nLeftLast = m_left.m_nNumSamplePoints - 1;
    if (nLeftLast >= 0)
        fLeftEnd = m_left.m_paSamplePoints[nLeftLast];
    if (fLeftEnd > fSamplePoint)
        return 1;

    float fRightEnd = 0.0f;
    int nRightLast = m_right.m_nNumSamplePoints - 1;
    if (nRightLast >= 0)
        fRightEnd = m_right.m_paSamplePoints[nRightLast];
    return fRightEnd > fSamplePoint;
}
