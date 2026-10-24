#include "k2stdint.h"
#if KOTOR1
#define K1_AURORA_LIST  // K1's AURORA library, where List.h grows from 8
#endif
#include "aurwindmanager.h"
#include "auremitter.h"

#include "port/crtapi.h"
#include "port/libcstring.h"
#include "port/libcstdlib.h"
#include "port/libcmath.h"
float cosineLUT(float f);
float sineLUT(float f);

PointSourceWind::PointSourceWind()
    : m_vPosition(0.0f, 0.0f, 0.0f), m_fRadius(0.0f), m_fLife(0.0f), m_fStrength(0.0f)
{
}

PointSourceWind::PointSourceWind(Vector vPosition, float fRadius, float fLife, float fStrength)
    : m_vPosition(vPosition), m_fRadius(fRadius), m_fLife(fLife), m_fStrength(fStrength)
{
}

PointSourceWind::~PointSourceWind()
{
}

WindManager::WindManager()
{
    m_vWind = m_vFrameWind;
    m_fMaxDeviationX = 0.0f;
    m_fMaxDeviationY = 0.0f;
    m_fNoiseTimer = 0.0f;
    m_fNoisePeriod = 2.0f;
    fillNoiseField(m_afNoiseTile1);
    fillNoiseField(m_afNoiseTile2);
    m_pafNoiseCurrent = m_afNoiseTile1;
    m_pafNoiseNext = m_afNoiseTile2;
}

WindManager::WindManager(Vector vWind, float fA, float fB, float fC)
{
    m_fMaxDeviationX = fA;
    m_fMaxDeviationY = fB;
    m_fNoiseTimer = 0.0f;
    m_fNoisePeriod = fC;
    m_vWind = vWind;
    fillNoiseField(m_afNoiseTile1);
    fillNoiseField(m_afNoiseTile2);
    m_pafNoiseCurrent = m_afNoiseTile1;
    m_pafNoiseNext = m_afNoiseTile2;
}

WindManager::~WindManager()
{
}

// Fills a value-noise grid. Each octave overlays square tiles of a random value, halving the tile
// edge down to 1, so the field is white noise summed over the 16, 8, 4, 2 and 1-step tile passes.
void WindManager::fillNoiseField(float (*pafField)[16])
{
    memset(pafField, 0, 16 * 16 * sizeof(float));

    int nStep = 16;
    while (nStep >= 1) {
        for (int i = 0; i + nStep <= 16; i += nStep) {
            for (int j = 0; j + nStep <= 16; j += nStep) {
                float fValue = static_cast<float>(rand_wincompatible() % (500 / (16 / nStep))) / 1000.0f;
                for (int y = i; y < i + nStep; y++) {
                    for (int x = j; x < j + nStep; x++)
                        pafField[y][x] += fValue;
                }
            }
        }
        nStep /= 2;
    }
}

// Sums every point source that covers `v`. The distance is the Chebyshev distance, the largest of
// the three absolute component differences.
Vector WindManager::pointSourceWindVector(Vector v)
{
    Vector vResult(0.0f, 0.0f, 0.0f);
    Vector vDelta;
    for (int i = 0; i < m_lPointSourceWinds.m_count; i++) {
        PointSourceWind& w = m_lPointSourceWinds.m_pData[i];

        vDelta = v - w.m_vPosition;
        float fDist = vDelta.x;
        if (fDist < 0.0f)
            fDist = -fDist;
        float fDistY = vDelta.y;
        if (fDistY < 0.0f)
            fDistY = -fDistY;
        if (fDistY > fDist)
            fDist = fDistY;
        if (fabsf(vDelta.z) > fDist)
            fDist = vDelta.z < 0.0f ? -vDelta.z : vDelta.z;

        if (fDist < w.m_fRadius) {
            float fScale = (w.m_fRadius - fDist) * w.m_fStrength / w.m_fRadius;
            Vector vDir = normalize(vDelta);
            vResult = vResult + vDir * fScale;
        }
    }
    return vResult;
}

void WindManager::AddPointSourceWind(Vector v, float fRadius, float fLife, float fStrength)
{
    m_lPointSourceWinds.Add(PointSourceWind(v, fRadius, fLife, fStrength));
}

static inline Vector RotateVector(const Vector& v, const Quaternion& q)
{
    float w = q.w;
    float x = q.x, y = q.y, z = q.z;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, wx = x * w;
    float yz = y * z, wy = y * w;
    float wz = z * w;
    Vector r;
    r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) + v.z * ((xz + wy) + (xz + wy));
    r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) + v.z * ((yz - wx) + (yz - wx));
    r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) + v.z * (1.0f - ((xx + yy) + (xx + yy)));
    return r;
}

// Folds a world coordinate into the 16-entry noise tile.
static inline int NoiseIndex(float f)
{
    return abs(static_cast<int>(f) * 4) % 16;
}

// Rz(fAngleX) * Rx(fAngleY), built from the two cosines and sines.
static inline Quaternion MakeWindRotation(float cA, float sA, float cB, float sB)
{
    Quaternion q;
    q.x = cA * sB;
    q.y = sA * sB;
    q.z = sA * cB;
    q.w = cA * cB;
    return q;
}

Vector WindManager::GetGlobalWindNoPoint(const Vector& v, float f)
{
    if (m_vWind.x == 0.0f && m_vWind.y == 0.0f && m_vWind.z == 0.0f)
        return m_vWind;

    int nX = NoiseIndex(v.x);
    int nY = NoiseIndex(v.y);

    static Quaternion q;

    float fValue = (1.0f - m_fNoiseBlend) * m_pafNoiseCurrent[nX][nY]
                 + m_fNoiseBlend * m_pafNoiseNext[nX][nY];

    float fAngleX = 0.0f;
    if (m_fMaxDeviationX != 0.0f)
        fAngleX = (fValue * 2.0f * m_fMaxDeviationX - m_fMaxDeviationX) * 0.5f;

    float fScale = fValue * f;
    Vector vScaled;
    vScaled.x = m_vWind.x * fScale;
    vScaled.y = m_vWind.y * fScale;
    vScaled.z = m_vWind.z * fScale;

    float fAngleY = 0.0f;
    if (m_fMaxDeviationY != 0.0f) {
        // The second axis samples the transposed tile.
        float fValueT = (1.0f - m_fNoiseBlend) * m_pafNoiseCurrent[nY][nX]
                      + m_fNoiseBlend * m_pafNoiseNext[nY][nX];
        fAngleY = (m_fMaxDeviationY * (fValueT * 2.0f) - m_fMaxDeviationY) * 0.5f;
    }

    float cA = cosineLUT(fAngleX);
    float sA = sineLUT(fAngleX);
    float cB = cosineLUT(fAngleY);
    float sB = sineLUT(fAngleY);
    q = MakeWindRotation(cA, sA, cB, sB);

    return RotateVector(vScaled, q);
}

// Returns the wind at `v`, the noise-scaled global wind plus the point-source field, rotated by the
// two-angle rotation.
Vector WindManager::GetGlobalWind(Vector v, float f)
{
    if (m_vWind.x == 0.0f && m_vWind.y == 0.0f && m_vWind.z == 0.0f) {
        Vector vStill = pointSourceWindVector(v);
        if (vStill.x == 0.0f && vStill.y == 0.0f && vStill.z == 0.0f)
            return m_vWind;
    }

    int nX = NoiseIndex(v.x);
    int nY = NoiseIndex(v.y);

    static Quaternion q;

    float fValue = (1.0f - m_fNoiseBlend) * m_pafNoiseCurrent[nX][nY]
                 + m_fNoiseBlend * m_pafNoiseNext[nX][nY];

    float fScale = fValue * f;
    Vector vScaled;
    vScaled.x = m_vWind.x * fScale;
    vScaled.y = m_vWind.y * fScale;
    vScaled.z = m_vWind.z * fScale;

    Vector vPoint = pointSourceWindVector(v);
    Vector vTotal = vScaled + vPoint;

    float fAngleX = 0.0f;
    if (m_fMaxDeviationX != 0.0f)
        fAngleX = (fValue * 2.0f * m_fMaxDeviationX - m_fMaxDeviationX) * 0.5f;

    float fAngleY = 0.0f;
    if (m_fMaxDeviationY != 0.0f) {
        float fValueT = (1.0f - m_fNoiseBlend) * m_pafNoiseCurrent[nY][nX]
                      + m_fNoiseBlend * m_pafNoiseNext[nY][nX];
        fAngleY = (m_fMaxDeviationY * (fValueT * 2.0f) - m_fMaxDeviationY) * 0.5f;
    }

    float cA = cosineLUT(fAngleX);
    float sA = sineLUT(fAngleX);
    float cB = cosineLUT(fAngleY);
    float sB = sineLUT(fAngleY);
    q = MakeWindRotation(cA, sA, cB, sB);

    return RotateVector(vTotal, q);
}

// The point-source field alone; the float argument is ignored.
Vector WindManager::GetGlobalPointWind(const Vector& v, float f)
{
    K2_UNUSED(f);
    return pointSourceWindVector(v);
}

// Ping-pongs the two noise tiles once the accumulated frame time passes the period, then drops the
// point sources whose life has run out.
void WindManager::updateTimer(float fDeltaT)
{
    if ((m_fNoiseTimer += fDeltaT) > m_fNoisePeriod) {
        float (*pafStandby)[16] = m_pafNoiseCurrent;
        m_pafNoiseCurrent = m_pafNoiseNext;
        m_pafNoiseNext = pafStandby;
        fillNoiseField(pafStandby);
        m_fNoiseTimer = 0.0f;
    }

    m_fNoiseBlend = m_fNoiseTimer / m_fNoisePeriod;
    m_vFrameWind = m_vWind * fDeltaT;

    List<PointSourceWind>& lWinds = m_lPointSourceWinds;
    for (int i = 0; i < lWinds.m_count; i++) {
        PointSourceWind& w = lWinds.m_pData[i];
        w.m_fLife -= fDeltaT;
        if (w.m_fLife < 0.0f) {
            lWinds.DelIndex(i);
            i--;
        }
    }
}

void WindManager::SetGlobalWind(Vector v)
{
    m_vWind = v;
    // The count is re-read every iteration: CalculateRadius is free to deregister the emitter.
    for (int i = 0; i < m_lWindyEmitters.m_count; i++)
        m_lWindyEmitters.m_pData[i]->CalculateRadius();
}

void WindManager::SetMaximumDeviation(float fA, float fB)
{
    m_fMaxDeviationX = fA;
    m_fMaxDeviationY = fB;
}

void WindManager::RegisterWindyEmitter(PartEmitter* pEmitter)
{
    m_lWindyEmitters.AddUnique(pEmitter);
}

// List::Remove hands DelIndex an index of -1 for an element that is not there, so check first.
void WindManager::DeregisterWindyEmitter(PartEmitter* pEmitter)
{
    if (m_lWindyEmitters.Contains(pEmitter))
        m_lWindyEmitters.Remove(pEmitter);
}
