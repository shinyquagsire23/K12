#include "k2stdint.h"
#include "port/aslglapi.h"
#if KOTOR1
#define K1_AURORA_LIST
#endif
#include "auremitter.h"
#include "aurmaterial.h"
#include "AURORA/aurmodel.h"
#include "aurcamera.h"
#include "aurscene.h"
#include "Matrix.h"
#include "Vector.h"

float emitterfadeheight = 5.0f;
int EmitterBounceType = 1;

int totalparticles;

#if !KOTOR1
CAurCamera* g_pWeatherCamera;
#endif
int totalemitters;
float birth;
int renderemitterspheres;
int useoldlightninginterp;

void Detonate(CAurObject* pObject, char* pszName, void* pContext);

void SetEmitterTarget(CAurObject* pObject, char* pszName, void* pContext);

#include <string.h>
#include "port/libcmath.h"
#include "port/crtapi.h"

extern Camera* CurrentCamera;

#include <algorithm>

extern int totalemitters;

extern float birth;



PartEmitter::PartEmitter(MdlNode* pNode) : Part(pNode)
{
    m_fAlphaEnd = 0.0f;
    m_fAlphaStart = 0.0f;

    // Clear the emitter's parameter block.
    // idiom-exempt: requires_token_comment
    memset(&m_fBirthRate, 0, 0x5c);  // idiom-exempt: the emitter's parameter block  // idiom-exempt: the orig's own +0x70 store-run construct
    m_fBlurLength = 1.0f;

    m_fLightningDelay = 0.0f;
    m_fLightningRadius = 0.0f;
    m_fLightningScale = 0.0f;
    m_fLightningSubDiv = 0.0f;
    m_fLightningZigzag = 0.0f;
    m_fRadius = 1.0f;
    m_fGravityAccel = 0.0f;
    m_fAlphaMid = 0.0f;
    // Default white tint.
    m_fPercentStart = 255.0f;
    m_fPercentMid = 255.0f;
    m_fPercentEnd = 255.0f;
    m_fSizeMid = 0.0f;
    m_fSizeMidY = 0.0f;
    m_fRandomBirthRate = 0.0f;
    m_fDelayTimer = 10000.0f;
    m_fControlPtTimer = 10000.0f;
    m_fTargetSize = 0.0f;
    m_fControlPtRadius = 0;
    m_fControlPtDelay = 0.0f;
    m_fTangentSpread = 0;
    m_fTangentLength = 0.0f;

    m_qInheritOrientation = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);

#if !KOTOR1  // K1 omits this member
    m_spTarget = 0;
#endif
    m_spWindManager = 0;
    m_pParticleMaterial = 0;
    m_pTileData = 0;
    m_bDetonate = false;

    m_pHitInfo = new CHitInfo(1, CHitInfo::kMaterialAllButTrigger, 0);
    totalemitters++;

#if !KOTOR1  // K1 has no weather emitters
    m_bWeather = false;
#endif
}

// Sets an emitter up from its parsed node: the render-mode and blend names select the draw and
// blending modes, a point-to-point emitter registers its target callback, and the orientation
// basis is seeded from the part's world frame.
void PartEmitter::Initialize()
{
    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode);

    if (!strcmp(pNode->m_szRender, "Normal"))
        m_nRenderMode = 1;
    else if (!strcmp(pNode->m_szRender, "Billboard_to_World_Z"))
        m_nRenderMode = 3;
    else if (!strcmp(pNode->m_szRender, "Billboard_to_Local_Z"))
        m_nRenderMode = 2;
    else if (!strcmp(pNode->m_szRender, "Linked"))
        m_nRenderMode = 4;
    else if (!strcmp(pNode->m_szRender, "Aligned_to_World_Z"))
        m_nRenderMode = 5;
    else if (!strcmp(pNode->m_szRender, "Aligned_to_Particle_Dir"))
        m_nRenderMode = 8;
    else if (!strcmp(pNode->m_szRender, "Motion_Blur"))
        m_nRenderMode = 9;

    m_bStarted = false;
    m_nGradientMode = 7;

    // A point-to-point emitter learns its target through this callback. The lifetime argument is
    // the callback's own, not the emitter's range. A node whose target is already selected keeps
    // its own range instead.
    if (pNode->m_flags.m_bPointToPoint) {
        m_pTargetCallBack = m_pGob->RegisterCallBack(SetEmitterTarget, const_cast<char*>("_EmitterTarget"),
                                              this, 9999.0f, false);
        if (!pNode->m_flags.m_bPointToPointSel)
            m_fGravityAccel = m_fGrav;
    }

    if (!_stricmp(pNode->m_szBlend, "Normal"))
        m_nBlendMode = 0;
    else if (!_stricmp(pNode->m_szBlend, "PunchThrough"))
        m_nBlendMode = 1;
    else
        m_nBlendMode = _stricmp(pNode->m_szBlend, "Punch-Through") ? 2 : 1;

    m_pParticleMaterial = new Material();
    m_pParticleMaterial->LoadTextures(this);

    m_fDelayTimer = AsLightningEmitter() ? 10000.0f : 0.0f;
    m_fControlPtTimer = 10000.0f;

    m_fBirthTimer = 0.0f;
    m_avVectors[4] = Vector(0.0f, 0.0f, 0.0f);
    m_avVectors[7] = m_avVectors[6];
    m_qPrevWorldOrientation = m_qWorldOrientation;

    // The emitter's orientation basis, the three axes new particles are spread along.
    Matrix mat = m_qWorldOrientation.getmatrix();
    m_avVectors[1] = Vector(mat.m_f[0], mat.m_f[1], mat.m_f[2]);
    m_avVectors[2] = Vector(mat.m_f[3], mat.m_f[4], mat.m_f[5]);
    m_avVectors[3] = Vector(mat.m_f[6], mat.m_f[7], mat.m_f[8]);
}

void PartEmitter::AnimateParticles(float fDeltaT)
{
    // Skip a fading-out emitter until its tile is fully opaque again.
    if (m_pGob != NULL && m_pGob->m_pTileData != NULL && m_pGob->m_pTileData->m_fFade < 1.0f)
        return;

    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode->AsMdlNodeEmitter());

    // This frame's travel offset. An already-animated inheriting-local emitter drifts by its Gob's
    // movement since the last frame.
    Vector vTravel(0.0f, 0.0f, 0.0f);
    if ((pNode->m_nEmitterFlags & MdlNodeEmitter::kEmitterInheritingLocal) && m_pGob != NULL && m_bStarted)
        vTravel = m_pGob->m_vPosition - m_avVectors[5];

    if (!m_bStarted)
    {
        // On the first frame only, seed the local frame from the part hierarchy, then let the
        // node's inherit flags replace it with the Gob's own world placement.
        GetWorld(m_avVectors[6], m_qWorldOrientation);
        if (m_pGob != NULL && (pNode->m_flags.m_bInheritingLocal || pNode->m_flags.m_bInheritingPart))
        {
            m_avVectors[5] = m_pGob->m_vPosition;
            m_qInheritOrientation = m_pGob->m_qOrientation;
        }
        if (pNode->m_flags.m_bInheritingPart)
        {
            m_avVectors[5] = m_avVectors[6];
            m_qInheritOrientation = m_qWorldOrientation;
        }

        if (m_spTarget.m_pObject != 0)
        {
            Vector vTarget = m_spTarget.m_pObject->GetPosition();
            Vector vHere(0.0f, 0.0f, 0.0f);
            Quaternion qHere;
            GetWorld(vHere, qHere);
            m_fRadius = magnitude(m_spTarget.m_pObject->GetPosition() - vHere);

            LightningEmitter* pBolt = AsLightningEmitter();
            if (pBolt != NULL)
            {
                // The bolt's segment count and birth rate, both scaled by the gap to the target.
                int nSeg = static_cast<int>(m_fRadius * m_fNumControlPts + 0.5f);
                m_fBirthRate = m_fRadius * m_fLightningSubDiv + 2.0f;

                // The bolt's control-point offsets run along the emitter's own local Z axis.
                Matrix matLocal = m_qLocalOrientation.getmatrix();
                Vector vZ = Vector(matLocal.m_f[6], matLocal.m_f[7], matLocal.m_f[8]);

                pBolt->m_lControlPoints.SetSize(nSeg + 2);
                pBolt->m_lPrevPoints.SetSize(nSeg + 2);
                pBolt->m_lNextPoints.SetSize(nSeg + 2);
                pBolt->m_lControlTangents.SetSize(nSeg + 2);
                pBolt->m_lPrevTangents.SetSize(nSeg + 2);
                pBolt->m_lNextTangents.SetSize(nSeg + 2);

                Vector vDirection(0.0f, 0.0f, 0.0f);
                pBolt->m_lNextPoints.m_pData[0] = vHere;
                pBolt->m_lControlPoints.m_pData[0] = vHere;
                pBolt->m_lNextPoints.m_pData[nSeg + 1] = vTarget;
                pBolt->m_lControlPoints.m_pData[nSeg + 1] = pBolt->m_lNextPoints.m_pData[nSeg + 1];

                pBolt->m_lNextTangents.m_pData[0] = vZ * m_fTangentLength;
                pBolt->m_lControlTangents.m_pData[0] = pBolt->m_lNextTangents.m_pData[0];
                pBolt->m_lNextTangents.m_pData[nSeg + 1] =
                    normalize(m_spTarget.m_pObject->GetPosition() - vHere) * m_fTangentLength;
                pBolt->m_lControlTangents.m_pData[nSeg + 1] = pBolt->m_lNextTangents.m_pData[nSeg + 1];

                // The bolt body is a cubic Bezier between the two ends, plus the curve's tangent in
                // the parallel direction list.
                for (int i = 1; i <= nSeg; i++)
                {
                    float t = static_cast<float>(i) / static_cast<float>(nSeg + 1);
                    if (t > 1.0f)
                        t = 1.0f;
                    float u = 1.0f - t;

                    Vector vP1 = vHere + vZ * m_fTangentLength;

                    Matrix matP2 = m_spTarget.m_pObject->GetOrientation().getmatrix();
                    Vector vZP2 = Vector(matP2.m_f[6], matP2.m_f[7], matP2.m_f[8]);
                    Vector vP2 = vTarget + vZP2 * m_fTangentLength;

                    Vector vPos = vHere * (u * u * u)
                                + vP1 * (3.0f * t * u * u)
                                + vP2 * (3.0f * t * t * u)
                                + vTarget * (t * t * t);
                    pBolt->m_lNextPoints.m_pData[i] = vPos;
                    pBolt->m_lControlPoints.m_pData[i] = pBolt->m_lNextPoints.m_pData[i];

                    Matrix matDir = m_spTarget.m_pObject->GetOrientation().getmatrix();
                    Vector vZDir = Vector(matDir.m_f[6], matDir.m_f[7], matDir.m_f[8]);
                    Vector vDiv = vTarget + vZDir * m_fTangentLength;

                    vDirection = vHere * (-3.0f * u * u)
                                      + vP1 * (3.0f * u * (1.0f - 3.0f * t))
                                      + vDiv * (3.0f * t * (2.0f - 3.0f * t))
                                      + vTarget * (3.0f * t * t);
                    vDirection.Normalize();
                    pBolt->m_lNextTangents.m_pData[i] = vDirection * m_fTangentLength;
                    pBolt->m_lControlTangents.m_pData[i] = pBolt->m_lNextTangents.m_pData[i];
                }

                Vector vSpan = m_spTarget.m_pObject->GetPosition() - vHere;
                if (m_fRadius != 0.0f)
                    vSpan = vSpan * (1.0f / m_fRadius);
                pBolt->m_fJitterAxisX = -vSpan.y;
                pBolt->m_fJitterAxisY = vSpan.z;
                pBolt->m_fJitterAxisZ = -vSpan.x;

                pBolt->m_vBoltSpan = m_spTarget.m_pObject->GetPosition() - vHere;
                pBolt->m_vBoltOrigin = vHere;
            }
        }

        m_bStarted = true;
    }

    if (!(pNode->m_nEmitterFlags & MdlNodeEmitter::kEmitterInheritingLocal))
    {
        m_avVectors[7] = m_avVectors[6];
        m_qPrevWorldOrientation = m_qWorldOrientation;
    }
    if ((pNode->m_nEmitterFlags & MdlNodeEmitter::kEmitterInheritingLocal) || pNode->m_flags.m_bInheritingLocal)
    {
        m_avVectors[7] = m_avVectors[6] + vTravel;
        m_qPrevWorldOrientation = m_qWorldOrientation;
    }

    GetWorld(m_avVectors[6], m_qWorldOrientation);

    Matrix matWorld = m_qWorldOrientation.getmatrix();
    m_avVectors[1] = Vector(matWorld.m_f[0], matWorld.m_f[1], matWorld.m_f[2]);
    m_avVectors[2] = Vector(matWorld.m_f[3], matWorld.m_f[4], matWorld.m_f[5]);
    m_avVectors[3] = Vector(matWorld.m_f[6], matWorld.m_f[7], matWorld.m_f[8]);

    Update(fDeltaT);
}

// Quaternion rotation helpers.

extern int quaternionidentityoptimization;
extern int useoldlightninginterp;

// Rotates a vector by a quaternion.
static inline Vector RotateVector(const Vector& v, const Quaternion& q)
{
    float w = q.w;
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
        float x = q.x, y = q.y, z = q.z;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * w;
        float yz = y * z, wy = y * w;
        float wz = z * w;
        Vector r;
        float m00 = 1.0f - ((yy + zz) + (yy + zz));
        float m01 = (xy - wz) + (xy - wz);
        float m02 = (xz + wy) + (xz + wy);
        float m10 = (xy + wz) + (xy + wz);
        float m11 = 1.0f - ((xx + zz) + (xx + zz));
        float m12 = (yz - wx) + (yz - wx);
        float m20 = (xz - wy) + (xz - wy);
        float m21 = (yz + wx) + (yz + wx);
        float m22 = 1.0f - ((xx + yy) + (xx + yy));
        r.x = v.x * m00 + v.y * m01 + v.z * m02;
        r.y = v.x * m10 + v.y * m11 + v.z * m12;
        r.z = v.x * m20 + v.y * m21 + v.z * m22;
        return r;
    }
    return v;
}

// Inverse rotation, spelled as the engine stores it: (-w, x, y, z).
static inline Quaternion InverseOf(const Quaternion& q)
{
#if KOTOR1  // K1 uses the out-of-line operator-
    return -q;
#else
    return Quaternion(-q.w, q.x, q.y, q.z);
#endif
}

void PartEmitter::moveParticle(Particle* pParticle, float fDeltaT, int nFlag)
{
    if (pParticle->m_bLanded)
        return;

    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode);

    Vector v;
    if (nFlag != 0)
    {
        // Rotate the particle's offset out of the previous frame and back into the current one,
        // both about the cached origin.
#if KOTOR1
        Quaternion qInv = -m_qPrevWorldOrientation;
#else
        Quaternion qInv(-m_qPrevWorldOrientation.w, m_qPrevWorldOrientation.x, m_qPrevWorldOrientation.y, m_qPrevWorldOrientation.z);
#endif
        Quaternion qRel = m_qWorldOrientation * qInv;
        v = RotateVector(pParticle->m_vPosition - m_avVectors[7], qRel) + m_avVectors[6];
    }
    else
    {
        v = pParticle->m_vPosition;
    }

    pParticle->m_vPosition = v + pParticle->m_vSpreadVelocity * fDeltaT;
    pParticle->m_vSpreadVelocity.z += m_fMass * -9.81f * fDeltaT;

    m_pHitInfo->ClearOutput();

    // A bouncing emitter reflects the particle's velocity about the hit normal and damps it.
    if (pNode->m_flags.m_bBouncing
        && currentscene->HitCheck(Vector(), pParticle->m_vPosition, &pParticle->m_vPosition,
                                  m_pHitInfo))
    {
        const Vector& vNormal = m_pHitInfo->m_vHitNormal;
#if KOTOR1
        float fDot = vNormal.x * pParticle->m_vSpreadVelocity.x
                   + vNormal.y * pParticle->m_vSpreadVelocity.y
                   + vNormal.z * pParticle->m_vSpreadVelocity.z;
#else
        float fDot = pParticle->m_vSpreadVelocity.x * vNormal.x
                   + pParticle->m_vSpreadVelocity.y * vNormal.y
                   + pParticle->m_vSpreadVelocity.z * vNormal.z;
#endif
#if KOTOR1
        pParticle->m_vSpreadVelocity = (pParticle->m_vSpreadVelocity + vNormal * -2.0f * fDot) * m_fBounceCoeff;
#else
        pParticle->m_vSpreadVelocity = (pParticle->m_vSpreadVelocity - vNormal * (2.0f * fDot)) * m_fBounceCoeff;
#endif
    }
}

extern int EmitterBounceType;
extern CAurCamera* g_pWeatherCamera;

// The weather wrap box is 10 units on a side, centred on the camera.
#if !KOTOR1
static inline float WeatherGridSize() { return 10.0f; }
#endif

void PartEmitter::moveParticleStandard(float fDeltaT)
{
    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode);

    if (m_lParticles.m_count >= 1)
    {
        // The frame-rate clamps are a fade step of 0.033 s of frame time held between 0.5 and 1,
        // and the frame-time-scaled speed ceiling the bounce test uses.
        float fAlpha = 0.033f / fDeltaT;
        if (fAlpha > 1.0f)
            fAlpha = 1.0f;
        float fAlphaMax = fAlpha;
        if (fAlphaMax < 0.5f)
            fAlphaMax = 0.5f;
        float fOneMinusDt = 1.0f - fDeltaT;
        float fDt26 = fDeltaT * 26.0f;

        // The two node toggles the loop needs, read once.
        int nInheriting = pNode->m_nEmitterFlags & MdlNodeEmitter::kEmitterInheriting;
        int nBouncing = pNode->m_nEmitterFlags & MdlNodeEmitter::kEmitterBouncing;

        // The wind sample, refreshed once per group of six particles.
        Vector vWind;

        for (int i = 0; i < m_lParticles.m_count; i++)
        {
            Particle* pParticle = m_lParticles.m_pData[i];
            if (pParticle->m_bLanded)
                continue;

            Vector vPos = pParticle->m_vPosition;
            Vector v;
            if (nInheriting)
            {
#if KOTOR1  // K1 recomputes the relative rotation for each of the two rotates
                Quaternion qRel = m_qWorldOrientation * -m_qPrevWorldOrientation;
                v = RotateVector(pParticle->m_vPosition - m_avVectors[7], qRel) + m_avVectors[6];
#else
                Quaternion qInv(-m_qPrevWorldOrientation.w, m_qPrevWorldOrientation.x, m_qPrevWorldOrientation.y, m_qPrevWorldOrientation.z);
                Quaternion qRel = m_qWorldOrientation * qInv;
                v = RotateVector(vPos - m_avVectors[7], qRel) + m_avVectors[6];
#endif
                // The spread velocity turns with the emitter, so its particles follow the new
                // heading.
#if KOTOR1
                qRel = m_qWorldOrientation * -m_qPrevWorldOrientation;
                pParticle->m_vSpreadVelocity = RotateVector(pParticle->m_vSpreadVelocity, qRel);
#else
                pParticle->m_vSpreadVelocity = RotateVector(pParticle->m_vSpreadVelocity, qRel);
#endif
            }
            else
            {
                v = vPos;
            }

            // For local inherit, add the Gob's movement since the cached origin so the particle
            // keeps its offset from the emitter.
            if ((pNode->m_nEmitterFlags & (MdlNodeEmitter::kEmitterInheriting | MdlNodeEmitter::kEmitterInheritingLocal)) == MdlNodeEmitter::kEmitterInheritingLocal && m_pGob != NULL)
            {
                Vector vDelta = m_pGob->m_vPosition - m_avVectors[5];
                v = v + vDelta;
                pParticle->m_vTrailPosition = pParticle->m_vTrailPosition + vDelta;
            }
            // Part inherit applies the same drift, off the emitter's own previous frame origin.
            if (pNode->m_nEmitterFlags & MdlNodeEmitter::kEmitterInheritingPart)
            {
                Vector vDelta = m_avVectors[6] - m_avVectors[5];
                v = v + vDelta;
                pParticle->m_vTrailPosition = pParticle->m_vTrailPosition + vDelta;
            }

            // The Euler step adds the offset direction, the particle's own spread velocity, and the
            // emitter's inherited velocity scaled by the particle's speed.
            pParticle->m_vPosition = v + pParticle->m_vSpreadVelocity * fDeltaT
                                       + pParticle->m_vVelocity * pParticle->m_fSpeed * fDeltaT;

            // Re-sample the scene's wind once per group of six particles, at the pre-step
            // position, then reuse it for the whole group.
            if (pNode->m_flags.m_bAffectedByWind)
            {
                if (m_lParticles.m_count < 50 || (i % 6) == 0)
                    vWind = currentscene->GetWindVector(v, fDeltaT);
                pParticle->m_vPosition = pParticle->m_vPosition + vWind;
            }

            // Gravity on the vertical component alone, then the frame-rate-independent speed decay.
            pParticle->m_vSpreadVelocity.z = pParticle->m_vSpreadVelocity.z + m_fMass * -9.81f * fDeltaT;
            float fSpeed = pParticle->m_fSpeed * fOneMinusDt;
            pParticle->m_fSpeed = fSpeed < 0.0f ? -fSpeed : fSpeed;

            if (nBouncing)
            {
                m_pHitInfo->ClearOutput();
                // A chunky emitter must exclude its own particles' meshes from the probe, or the
                // particle would collide with itself.
                if (pNode->m_szChunkName[0] != 0)
                {
                    m_pHitInfo->m_ignoreList.SetSize(0);
                    CAurObject* pChunk = static_cast<ChunkyParticle*>(pParticle)->m_pAurObject;
                    if (pChunk != NULL)
                        m_pHitInfo->m_ignoreList.Add(pChunk);
                }

                if (currentscene->HitCheck(vPos, pParticle->m_vPosition,
                                           &pParticle->m_vPosition, m_pHitInfo) != 0)
                {
                    const Vector& vNormal = m_pHitInfo->m_vHitNormal;
#ifdef K2_FIX_ENGINE_BUGS
                    float fMagnitude = 0.0f;
#else
                    float fMagnitude;
#endif

                    // Bounce mode 2 reflects at the damping factor and re-derives the speed from
                    // the result; mode 1 below also scales the velocity down.
                    if (EmitterBounceType == 2)
                    {
                        float fDot = pParticle->m_vSpreadVelocity.x * vNormal.x
                                   + pParticle->m_vSpreadVelocity.y * vNormal.y
                                   + pParticle->m_vSpreadVelocity.z * vNormal.z;
                        pParticle->m_vSpreadVelocity =
                            (pParticle->m_vSpreadVelocity + vNormal * -2.0f * fDot) * m_fBounceCoeff;
                        fMagnitude = magnitude(pParticle->m_vSpreadVelocity);
                    }

                    if (EmitterBounceType == 1)
                    {
                        if (m_fBounceCoeff == 0.0f)
                        {
                            // A fully-damped emitter just kills the particle's velocity, and a
                            // zero magnitude always re-orients below.
                            pParticle->m_vSpreadVelocity = Vector();
                            fMagnitude = 0.0f;
                        }
                        else
                        {
                            float fDot = pParticle->m_vSpreadVelocity.x * vNormal.x
                                       + pParticle->m_vSpreadVelocity.y * vNormal.y
                                       + pParticle->m_vSpreadVelocity.z * vNormal.z;
                            pParticle->m_vSpreadVelocity =
                                (pParticle->m_vSpreadVelocity + vNormal * -2.0f * fDot) * 0.75f;
                            pParticle->m_vSpreadVelocity.z = pParticle->m_vSpreadVelocity.z * m_fBounceCoeff * fAlphaMax;
                            fMagnitude = pParticle->m_vSpreadVelocity.x * pParticle->m_vSpreadVelocity.x
                                       + pParticle->m_vSpreadVelocity.y * pParticle->m_vSpreadVelocity.y
                                       + pParticle->m_vSpreadVelocity.z * pParticle->m_vSpreadVelocity.z;
                        }
                        if (fMagnitude < 0.5f)
                            goto reorient;
                    }
                    else if (acosf(vNormal.x < 0 ? -vNormal.x : vNormal.x) > 0.7853982f
                             && acosf(vNormal.y < 0 ? -vNormal.y : vNormal.y) > 0.7853982f
                             && fMagnitude < fDt26)
                    {
                    reorient:
                        pParticle->m_bLanded = true;
                        pParticle->m_qOrientation =
                            RotationArc(Vector(0.0f, 0.0f, 1.0f), vNormal);
                        pParticle->m_vPosition = pParticle->m_vPosition + vNormal * 0.02f;
                        pParticle->m_vDirection = vNormal;
                        if (pNode->m_szChunkName[0] != 0)
                            pParticle->m_vPosition =
                                pParticle->m_vPosition + vNormal * m_fSizeStart;
                    }
                }
            }

#if !KOTOR1  // K1 has no weather emitters
            // A weather emitter re-seats its particles into a wrapping grid around the current
            // weather camera, so the shower follows the camera across the map.
            if (m_bWeather)
            {
                if (g_pWeatherCamera != NULL)
                {
                    Vector vCamPos = g_pWeatherCamera->GetPosition();
                    static float width = WeatherGridSize();
                    static float height = WeatherGridSize();

                    float fX = pParticle->m_vPosition.x - vCamPos.x + 5.0f;
                    float fY = pParticle->m_vPosition.y - vCamPos.y + 5.0f;
                    float fZ = pParticle->m_vPosition.z - vCamPos.z;
                    // The re-centre is -5 for a non-negative operand and +5 otherwise, which
                    // lands the wrap in [-5, 5) around the camera.
                    fX = fmodf(fX, width) + (fX >= 0.0f ? -5.0f : 5.0f);
                    fY = fmodf(fY, height) + (fY >= 0.0f ? -5.0f : 5.0f);
                    pParticle->m_vPosition =
                        Vector(vCamPos.x + fX, vCamPos.y + fY, vCamPos.z + fZ);
                }
            }
#endif
        }
    }

    // Cache the reference pose the inherit arms read next frame.
    if (m_pGob != NULL && (pNode->m_nEmitterFlags & MdlNodeEmitter::kEmitterInheritingLocal))
    {
        m_avVectors[5] = m_pGob->m_vPosition;
        m_qInheritOrientation = m_pGob->m_qOrientation;
    }
    else if ((pNode->m_nEmitterFlags & MdlNodeEmitter::kEmitterInheritingPart))
    {
        m_avVectors[5] = m_avVectors[6];
        m_qInheritOrientation = m_qWorldOrientation;
    }
}

void PartEmitter::moveParticleP2PBezier(float fDeltaT)
{
    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode);
    int nInheriting = pNode->m_nEmitterFlags & MdlNodeEmitter::kEmitterInheriting;

    // The far end of the curve is the target's live position, jittered on each axis within the
    // emitter's wander range.
    Vector vTarget = m_spTarget.m_pObject->GetPosition();
    if (m_fTargetSize != 0.0f)
    {
        int n0 = rand_wincompatible();
        int n1 = n0 % static_cast<int>(m_fTargetSize * 50.0f);
        vTarget.x += static_cast<float>((n0 & 1) ? n1 : -n1) / 100.0f;

        int n2 = rand_wincompatible();
        int n3 = n2 % static_cast<int>(m_fTargetSize * 50.0f);
        vTarget.y += static_cast<float>((n2 & 1) ? n3 : -n3) / 100.0f;

        int n4 = rand_wincompatible();
        int n5 = n4 % static_cast<int>(m_fTargetSize * 50.0f);
        vTarget.z += static_cast<float>((n4 & 1) ? n5 : -n5) / 100.0f;
    }

    if (m_lParticles.m_count >= 1)
    {
        int nBouncing = pNode->m_nEmitterFlags & MdlNodeEmitter::kEmitterBouncing;
        float fOneMinusDt = 1.0f - fDeltaT;

        for (int i = 0; i < m_lParticles.m_count; i++)
        {
            Particle* pParticle = m_lParticles.m_pData[i];
            Vector vPos = pParticle->m_vPosition;
            Vector v;

            if (nInheriting)
            {
#if KOTOR1  // K1 recomputes the relative rotation for each of the two rotates
                Quaternion qRel = m_qWorldOrientation * -m_qPrevWorldOrientation;
                v = RotateVector(pParticle->m_vPosition - m_avVectors[7], qRel) + m_avVectors[6];
#else
                Quaternion qInv(-m_qPrevWorldOrientation.w, m_qPrevWorldOrientation.x, m_qPrevWorldOrientation.y, m_qPrevWorldOrientation.z);
                Quaternion qRel = m_qWorldOrientation * qInv;
                v = RotateVector(vPos - m_avVectors[7], qRel) + m_avVectors[6];
#endif
#if KOTOR1
                qRel = m_qWorldOrientation * -m_qPrevWorldOrientation;
                pParticle->m_vSpreadVelocity = RotateVector(pParticle->m_vSpreadVelocity, qRel);
#else
                pParticle->m_vSpreadVelocity = RotateVector(pParticle->m_vSpreadVelocity, qRel);
#endif
            }
            else
            {
                v = vPos;
            }

            // In the particle's own travel this frame, the spread velocity is added whole, with no
            // gravity term yet.
            Vector vStep = v + pParticle->m_vSpreadVelocity * fDeltaT;

            if (pNode->m_flags.m_bAffectedByWind)
                vStep = vStep + currentscene->GetWindVector(v, fDeltaT);

            // The curve parameter is the particle's age as a fraction of the emitter's lifetime,
            // clamped to 1.
            float fT = pParticle->m_fAge / m_fLifeExp;
            if (fT > 1.0f)
                fT = 1.0f;
            float fT2 = fT * fT;
            float fT3 = fT2 * fT;
            float fU = 1.0f - fT;
            float fU2 = fU * fU;
            float fU3 = fU2 * fU;
            float f3T = fT * 3.0f;
            float fA = fU3;
            float fB = f3T * fU * fU;
            float fC = fT * f3T * fU;
            float fD = fT3;

            // The two inner control points ride the watched object's forward axis and the
            // emitter's own third axis, each scaled by a handle length.
            Matrix mat = m_spTarget.m_pObject->GetOrientation().getmatrix();
            Vector vCtrl1 = m_avVectors[6] + m_avVectors[3] * m_fP2PBezier2;
            Vector vCtrl2 = vTarget + Vector(mat.m_f[6], mat.m_f[7], mat.m_f[8]) * m_fP2PBezier3;
            Vector vBez = m_avVectors[6] * fA + vCtrl1 * fB + vCtrl2 * fC + vTarget * fD;

            // The curve is only one term of the blend. It is damped against the particle's own
            // travel, and the inherited velocity is added outright.
            float fPow = powf(fT3, m_fCombineTime);
            pParticle->m_vPosition = vBez * fPow + vStep * (1.0f - fPow)
                                   + pParticle->m_vVelocity * pParticle->m_fSpeed * fDeltaT;

            // Gravity on the vertical velocity, then the frame-rate-independent speed decay.
            pParticle->m_vSpreadVelocity.z = pParticle->m_vSpreadVelocity.z + m_fMass * -9.81f * fDeltaT;
            float fSpeed = pParticle->m_fSpeed * fOneMinusDt;
            pParticle->m_fSpeed = fSpeed < 0.0f ? -fSpeed : fSpeed;

            // Cleared every frame, not only when bouncing.
            m_pHitInfo->ClearOutput();

            if (nBouncing)
            {
                // A chunky emitter must exclude its own particles' meshes from the probe, and it
                // extends the probe ray past the particle by the emitter's handle length.
                Vector vA = pParticle->m_vPosition;
                if (pNode->m_szChunkName[0] != 0)
                {
                    m_pHitInfo->m_ignoreList.SetSize(0);
                    CAurObject* pChunk = static_cast<ChunkyParticle*>(pParticle)->m_pAurObject;
                    if (pChunk != NULL)
                        m_pHitInfo->m_ignoreList.Add(pChunk);
                    vA = pParticle->m_vPosition + (pParticle->m_vPosition - vPos) * m_fSizeStart;
                }

                if (currentscene->HitCheck(vPos, vA, &pParticle->m_vPosition,
                                           m_pHitInfo) != 0)
                {
                    const Vector& vNormal = m_pHitInfo->m_vHitNormal;
                    float fDot = pParticle->m_vSpreadVelocity.x * vNormal.x
                               + pParticle->m_vSpreadVelocity.y * vNormal.y
                               + pParticle->m_vSpreadVelocity.z * vNormal.z;
                    pParticle->m_vSpreadVelocity =
                        (pParticle->m_vSpreadVelocity + vNormal * (-2.0f * fDot)) * m_fBounceCoeff;

                    // A nudge off the surface, then the reorient, which lays the particle flat on
                    // the hit normal and has it remember the normal, with a second, longer nudge
                    // for a chunky one.
                    pParticle->m_vPosition = pParticle->m_vPosition + vNormal * 0.005f;

                    float fMagSq = pParticle->m_vSpreadVelocity.x * pParticle->m_vSpreadVelocity.x
                                 + pParticle->m_vSpreadVelocity.y * pParticle->m_vSpreadVelocity.y
                                 + pParticle->m_vSpreadVelocity.z * pParticle->m_vSpreadVelocity.z;
                    if (fMagSq < 0.5f)
                    {
                        pParticle->m_bLanded = true;
                        pParticle->m_qOrientation =
                            RotationArc(Vector(0.0f, 0.0f, 1.0f), vNormal);
                        pParticle->m_vPosition = pParticle->m_vPosition + vNormal * 0.02f;
                        pParticle->m_vDirection = vNormal;
                    }
                    if (pNode->m_szChunkName[0] != 0)
                        pParticle->m_vPosition =
                            pParticle->m_vPosition + vNormal * m_fSizeStart;
                }
            }
        }
    }
}

void PartEmitter::moveParticleP2PGrav(float fDeltaT)
{
    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode);
    int nInheriting = pNode->m_nEmitterFlags & MdlNodeEmitter::kEmitterInheriting;

    Vector v;
    Vector vStep;
    Vector vTarget;
    Vector vToTarget;

    // The far end of the curve is the target's live position, jittered on each axis within the
    // emitter's wander range.
    vTarget = m_spTarget.m_pObject->GetPosition();
    if (m_fTargetSize != 0.0f)
    {
        int n0 = rand_wincompatible();
        int n1 = n0 % static_cast<int>(m_fTargetSize * 50.0f);
        float fOffX = static_cast<float>((n0 & 1) ? n1 : -n1) / 100.0f;

        int n2 = rand_wincompatible();
        int n3 = n2 % static_cast<int>(m_fTargetSize * 50.0f);
        float fOffY = static_cast<float>((n2 & 1) ? n3 : -n3) / 100.0f;

        int n4 = rand_wincompatible();
        int n5 = n4 % static_cast<int>(m_fTargetSize * 50.0f);
        float fOffZ = static_cast<float>((n4 & 1) ? n5 : -n5) / 100.0f;

        vTarget.x = fOffX + vTarget.x;
        vTarget.y = fOffY + vTarget.y;
        vTarget.z = fOffZ + vTarget.z;
    }

    if (m_lParticles.m_count >= 1)
    {
        int nBouncing = pNode->m_nEmitterFlags & MdlNodeEmitter::kEmitterBouncing;
        float fOneMinusDt = 1.0f - fDeltaT;

        for (int i = 0; i < m_lParticles.m_count; i++)
        {
            Particle* pParticle = m_lParticles.m_pData[i];
            Vector vPos = pParticle->m_vPosition;

            if (nInheriting)
            {
#if KOTOR1  // K1 recomputes the relative rotation for each of the two rotates
                Quaternion qRel = m_qWorldOrientation * -m_qPrevWorldOrientation;
                v = RotateVector(pParticle->m_vPosition - m_avVectors[7], qRel) + m_avVectors[6];
#else
                Quaternion qInv(-m_qPrevWorldOrientation.w, m_qPrevWorldOrientation.x, m_qPrevWorldOrientation.y, m_qPrevWorldOrientation.z);
                Quaternion qRel = m_qWorldOrientation * qInv;
                v = RotateVector(vPos - m_avVectors[7], qRel) + m_avVectors[6];
#endif
#if KOTOR1
                qRel = m_qWorldOrientation * -m_qPrevWorldOrientation;
                pParticle->m_vSpreadVelocity = RotateVector(pParticle->m_vSpreadVelocity, qRel);
#else
                pParticle->m_vSpreadVelocity = RotateVector(pParticle->m_vSpreadVelocity, qRel);
#endif
            }
            else
            {
                v = vPos;
            }

            // For Gob inherit, re-rotate the particle's offset between the cached and current Gob
            // orientations, then re-base it on the Gob's live position.
            if ((pNode->m_nEmitterFlags & (MdlNodeEmitter::kEmitterInheriting | MdlNodeEmitter::kEmitterInheritingLocal)) == MdlNodeEmitter::kEmitterInheritingLocal && m_pGob != NULL)
            {
                Quaternion& qGob = m_pGob->m_qOrientation;
#if KOTOR1  // K1 inverts the Gob orientation again for the velocity rotate
                v = RotateVector(v - m_avVectors[5], InverseOf(m_qInheritOrientation));
#else
                Quaternion qInv = InverseOf(m_qInheritOrientation);
                v = RotateVector(v - m_avVectors[5], qInv);
#endif
                v = RotateVector(v, qGob);

                Vector vDelta = m_pGob->m_vPosition - m_avVectors[5];
                pParticle->m_vTrailPosition = pParticle->m_vTrailPosition + vDelta;
                v = v + vDelta;
                v = v + m_avVectors[5];

#if KOTOR1
                pParticle->m_vSpreadVelocity =
                    RotateVector(RotateVector(pParticle->m_vSpreadVelocity, InverseOf(m_qInheritOrientation)), qGob);
#else
                pParticle->m_vSpreadVelocity = RotateVector(RotateVector(pParticle->m_vSpreadVelocity, qInv), qGob);
#endif
            }

            // Part inherit does the same double rotation, taken off the emitter's own frame
            // origins, with the trail position re-expressed around the Gob.
            if ((pNode->m_nEmitterFlags & MdlNodeEmitter::kEmitterInheritingPart) && m_pGob != NULL)
            {
                Quaternion& qGob = m_pGob->m_qOrientation;
                Vector vDelta = m_avVectors[6] - m_avVectors[5];

                v = RotateVector(v - m_avVectors[5], InverseOf(m_qInheritOrientation));
                v = RotateVector(v, qGob);
                v = v + vDelta;

                pParticle->m_vTrailPosition = pParticle->m_vTrailPosition + vDelta;
                pParticle->m_vTrailPosition = pParticle->m_vTrailPosition - m_pGob->m_vPosition;
                pParticle->m_vTrailPosition =
                    RotateVector(RotateVector(pParticle->m_vTrailPosition, InverseOf(m_qInheritOrientation)), qGob);
                pParticle->m_vTrailPosition = pParticle->m_vTrailPosition + m_pGob->m_vPosition;

                v = v + m_avVectors[5];
                pParticle->m_vSpreadVelocity =
                    RotateVector(RotateVector(pParticle->m_vSpreadVelocity, InverseOf(m_qInheritOrientation)), qGob);
            }

            // The Euler step, then the wind sample, which is passed the pre-step position.
            vStep = v + pParticle->m_vSpreadVelocity * fDeltaT;
            if (pNode->m_flags.m_bAffectedByWind)
                vStep = vStep + currentscene->GetWindVector(v, fDeltaT);

            // The homing pull. A particle already inside the arrival radius ends here; its age is
            // set to the emitter lifetime and the call returns.
            vToTarget = vTarget - v;
            float fDistSq = vToTarget.x * vToTarget.x + vToTarget.y * vToTarget.y
                          + vToTarget.z * vToTarget.z;
            float fArriveSq = m_fThreshold * m_fThreshold;
            if (fDistSq <= fArriveSq)
            {
                pParticle->m_fAge = m_fLifeExp;
                return;
            }

            // Damp the spread velocity, then accelerate along the unit line to the target: the
            // same `vPush` is added to the velocity outright and to the position by one more dt.
            float fPow = powf(m_fDrag, fDeltaT);
            pParticle->m_vSpreadVelocity = pParticle->m_vSpreadVelocity * fPow;
            Vector vPush = normalize(vToTarget) * m_fGrav * fDeltaT;
            vStep = vStep + vPush * fDeltaT;
            pParticle->m_vSpreadVelocity = pParticle->m_vSpreadVelocity + vPush;

            // The travel direction is this frame's actual displacement, not the aim vector.
            Vector vTravel = vStep - v;
            Vector vZero;
            if (!equal(vTravel, vZero))
                pParticle->m_vDirection = normalize(vTravel);

            // Arrival is re-tested against the segment the particle swept, and a near miss only
            // resets the age (it does not return).
            vToTarget = vTarget - segmentproject(v, vStep, vTarget);
            float fNearSq = vToTarget.x * vToTarget.x + vToTarget.y * vToTarget.y
                          + vToTarget.z * vToTarget.z;
            if (fNearSq <= fArriveSq)
                pParticle->m_fAge = m_fLifeExp;

            pParticle->m_vPosition = vStep
                                   + pParticle->m_vVelocity * pParticle->m_fSpeed * fDeltaT;

            // Gravity on the vertical velocity, then the frame-rate-independent speed decay.
            pParticle->m_vSpreadVelocity.z = pParticle->m_vSpreadVelocity.z + m_fMass * -9.81f * fDeltaT;
            float fSpeed = pParticle->m_fSpeed * fOneMinusDt;
            pParticle->m_fSpeed = fSpeed < 0.0f ? -fSpeed : fSpeed;

            m_pHitInfo->ClearOutput();

            if (nBouncing)
            {
                Vector vA = pParticle->m_vPosition;
                if (pNode->m_szChunkName[0] != 0)
                {
                    m_pHitInfo->m_ignoreList.SetSize(0);
                    CAurObject* pChunk = static_cast<ChunkyParticle*>(pParticle)->m_pAurObject;
                    if (pChunk != NULL)
                        m_pHitInfo->m_ignoreList.Add(pChunk);
                    vA = pParticle->m_vPosition + (pParticle->m_vPosition - vPos) * m_fSizeStart;
                }

                if (currentscene->HitCheck(vPos, vA, &pParticle->m_vPosition,
                                           m_pHitInfo) != 0)
                {
                    const Vector& vNormal = m_pHitInfo->m_vHitNormal;
                    float fDot = pParticle->m_vSpreadVelocity.x * vNormal.x
                               + pParticle->m_vSpreadVelocity.y * vNormal.y
                               + pParticle->m_vSpreadVelocity.z * vNormal.z;
                    pParticle->m_vSpreadVelocity =
                        (pParticle->m_vSpreadVelocity + vNormal * (-2.0f * fDot)) * m_fBounceCoeff;
                    pParticle->m_vPosition = pParticle->m_vPosition + vNormal * 0.005f;

                    float fMagSq = pParticle->m_vSpreadVelocity.x * pParticle->m_vSpreadVelocity.x
                                 + pParticle->m_vSpreadVelocity.y * pParticle->m_vSpreadVelocity.y
                                 + pParticle->m_vSpreadVelocity.z * pParticle->m_vSpreadVelocity.z;
                    if (fMagSq < 0.5f)
                    {
                        pParticle->m_bLanded = true;
                        pParticle->m_qOrientation =
                            RotationArc(Vector(0.0f, 0.0f, 1.0f), vNormal);
                        pParticle->m_vPosition = pParticle->m_vPosition + vNormal * 0.02f;
                        pParticle->m_vDirection = vNormal;
                    }
                    if (pNode->m_szChunkName[0] != 0)
                        pParticle->m_vPosition =
                            pParticle->m_vPosition + vNormal * m_fSizeStart;
                }
            }
        }
    }

    // Cache the Gob pose the inherit arms read next frame.
    if (m_pGob != NULL && (pNode->m_nEmitterFlags & MdlNodeEmitter::kEmitterInheritingLocal))
    {
        m_avVectors[5] = m_pGob->m_vPosition;
        m_qInheritOrientation = m_pGob->m_qOrientation;
    }
    else if (pNode->m_nEmitterFlags & MdlNodeEmitter::kEmitterInheritingPart)
    {
        m_avVectors[5] = m_avVectors[6];
#ifdef K2_FIX_ENGINE_BUGS
        if (m_pGob != NULL)
#endif
            m_qInheritOrientation = m_pGob->m_qOrientation;
    }
}

#if !KOTOR1  // K1 has no memory report
int PartEmitter::printMemory()
{
    printf("Part: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
           8, 16, 28, 44, 48, 52, 53, 54, 56, 72, 80, 88, 96);
    return printf(
        " -> PartEmitter: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, "
        "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, "
        "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, "
        "%d, %d, %d, %d, %d, %d, %d, %d, %d\n",
        100, 104, 108, 112, 116, 120, 124,
        128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188,
        192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252,
        256, 260, 264, 268, 272, 276, 280, 284, 288, 292, 296, 300, 304, 308,
        320, 332, 344, 356, 368, 380, 392, 404, 416, 428,
        444, 460, 480, 488, 504, 520, 528, 536, 544, 552,
        560, 562, 563, 564);
}
#endif // !KOTOR1

PartEmitter* PartEmitter::AsPartEmitter() { return this; }

LightningEmitter* PartEmitter::AsLightningEmitter() { return 0; }

PartEmitter::~PartEmitter()
{
    // Raw pointer member, so the hit record needs an explicit delete.
    delete m_pHitInfo;

    // Free every particle; the lists' own destructors free their arrays.
    for (int i = m_lParticles.m_count - 1; i >= 0; i--)
    {
        Particle* pParticle = m_lParticles.m_pData[i];
        m_lParticles.DelIndex(i);
        delete pParticle;
    }

    for (int i = m_lFreeParticles.m_count - 1; i >= 0; i--)
    {
        Particle* pParticle = m_lFreeParticles.m_pData[i];
        m_lFreeParticles.DelIndex(i);
        delete pParticle;
    }

    // Leave the scene's wind manager before the part goes away.
    if (m_spWindManager.m_pObject)
        m_spWindManager.m_pObject->DeregisterWindyEmitter(this);

    // The scene removes the part itself, not its Gob.
    if (m_pGob)
        Scene::RemoveFromSceneManager(this);

    delete m_pParticleMaterial;

    totalemitters--;
}

Part* MdlNodeEmitter::InternalCreateInstance(Gob* pGob)
{
    // The node's kind name picks the part class. Only "Explosion" does anything with the Gob,
    // registering the "Detonate" callback before the part is initialised.
    PartEmitter* pPart;
    if (strcmp(m_szUpdate, "Fountain") == 0)
        pPart = new FountainEmitter(this);
    else if (strcmp(m_szUpdate, "Explosion") == 0)
    {
        pPart = new ExplosionEmitter(this);
        if (pGob)
            pGob->RegisterCallBack(Detonate, const_cast<char*>("Detonate"), pPart, 9999.0f, false);
    }
    else if (strcmp(m_szUpdate, "Single") == 0)
        pPart = new SingleEmitter(this);
    else
        pPart = new LightningEmitter(this);

    pPart->m_pGob = pGob;
    pPart->Initialize();
    return pPart;
}

FountainEmitter::FountainEmitter(MdlNode* pNode) : PartEmitter(pNode) {}

ExplosionEmitter::ExplosionEmitter(MdlNode* pNode) : PartEmitter(pNode) {}

SingleEmitter::SingleEmitter(MdlNode* pNode) : PartEmitter(pNode) {}

LightningEmitter::LightningEmitter(MdlNode* pNode) : PartEmitter(pNode)
{
    m_fLightningScale = 0.0f;
    // Clear the bolt state.
#if KOTOR1  // K1 zeroes the scalars one by one (the Vectors/Lists by their own ctors)
    m_fJitterAxisX = 0.0f;
    m_fJitterAxisY = 0.0f;
    m_fJitterAxisZ = 0.0f;
    m_nActiveBranches = 0;
    m_bIsBranch = false;
#else
    memset(&m_vBoltSpan, 0, 0x2f1 - 0x238);  // idiom-exempt: the aggregate member block  // idiom-exempt: the orig's own 0x238..0x2f1 store-run
#endif
}

LightningEmitter::~LightningEmitter()
{
    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode->AsMdlNodeEmitter());
    if (m_lBranches.m_count != 0 && pNode->m_nNumBranches > 0)
    {
        for (int64_t i = 0; i < pNode->m_nNumBranches; i++)
        {
            if (m_lBranches.m_pData[i]->m_spTarget.m_pObject != NULL)
                delete m_lBranches.m_pData[i]->m_spTarget.m_pObject;
            delete m_lBranches.m_pData[i];
        }
    }
}

void SetEmitterTarget(CAurObject* pObject, char* pszEvent, void* pData);

void LightningEmitter::Initialize()
{
    PartEmitter::Initialize();
    m_fLifeExp = 1.0f;
    m_pTargetCallBack = m_pGob->RegisterCallBack(SetEmitterTarget, const_cast<char*>("_EmitterTarget"), this,
                                           9999.0f, false);
    m_nGradientMode = 6;

    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode->AsMdlNodeEmitter());
    m_lBranches.SetSize(pNode->m_nNumBranches);
    m_lBranchStartIndices.SetSize(pNode->m_nNumBranches);

    for (int i = 0; i < pNode->m_nNumBranches; i++)
    {
        m_lBranches.m_pData[i] = new LightningEmitter(m_pNode);
        m_lBranches.m_pData[i]->InitializeBranch();
    }
}

void LightningEmitter::InitializeBranch()
{
    PartEmitter::Initialize();
    m_pNode->ControlDispatch(this, -1.0f, 0.0f, 1.0f);

    m_lControlPoints.SetSize(2);
    m_lPrevPoints.SetSize(2);
    m_lNextPoints.SetSize(2);
    m_lControlTangents.SetSize(2);
    m_lPrevTangents.SetSize(2);
    m_lNextTangents.SetSize(2);

    m_lBranches.SetSize(0);
    m_lBranchStartIndices.SetSize(0);

    m_bIsBranch = true;

    m_spTarget = static_cast<CAurObject*>(new Gob(const_cast<char*>("_AUR_lightningbranch")));  // idiom-exempt: Gob.h does not model the CAurObject base (Gob.cpp/Camera.h precedent)

    m_fXSize = 0.0f;
    m_fYSize = 0.0f;
    m_fTargetSize = 0.0f;
    m_fDelayTimer = m_fLightningDelay;
    m_fControlPtTimer = m_fControlPtDelay;
}

void LightningEmitter::UpdateBranches()
{
    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode->AsMdlNodeEmitter());
    if (pNode->m_nNumBranches == 0)
        return;

    m_nActiveBranches = rand_wincompatible() % (pNode->m_nNumBranches + 1);
    if (m_nActiveBranches < 1)
        return;

    for (int i = 0; i < m_nActiveBranches; i++)
    {
        float fScale = static_cast<float>(rand_wincompatible() % 80) / 100.0f + 0.1f;

        m_lBranches.m_pData[i]->m_fSizeStart = m_fSizeStart * fScale;
        m_lBranches.m_pData[i]->m_fSizeMid = m_fSizeMid * fScale;
        m_lBranches.m_pData[i]->m_fSizeEnd = m_fSizeEnd * fScale;
        m_lBranches.m_pData[i]->m_fSizeStartY = m_fSizeStartY * fScale;
        m_lBranches.m_pData[i]->m_fSizeMidY = m_fSizeMidY * fScale;
        m_lBranches.m_pData[i]->m_fSizeEndY = m_fSizeEndY * fScale;

        m_lBranchStartIndices.m_pData[i] = static_cast<int>(static_cast<float>(m_lParticles.m_count) * fScale);

        m_lBranches.m_pData[i]->m_avVectors[6] = m_lBranches.m_pData[i]->m_lControlPoints.m_pData[0] =
            m_lParticles.m_pData[m_lBranchStartIndices.m_pData[i]]->m_vPosition;

        float fYaw = static_cast<float>(-(rand_wincompatible() % 45));
        float fRoll = static_cast<float>(rand_wincompatible() % 360);
        Quaternion qBranch = YawPitchRoll(fYaw, 0.0f, fRoll);

        m_lBranches.m_pData[i]->m_lControlTangents.m_pData[0] =
            RotateVector(Vector(m_fJitterAxisX, m_fJitterAxisY, m_fJitterAxisZ) * m_fTangentLength, qBranch);

        if (fScale > 0.5f)
        {
            m_lBranches.m_pData[i]->m_spTarget.m_pObject->SetPosition(
                m_spTarget.m_pObject->GetPosition());
            m_lBranches.m_pData[i]->m_lControlPoints.m_pData[1] = m_spTarget.m_pObject->GetPosition();
            m_lBranches.m_pData[i]->m_fTargetSize = m_fTargetSize;
            m_lBranches.m_pData[i]->m_fBirthRate =
                m_fLightningSubDiv * magnitude(m_spTarget.m_pObject->GetPosition() - m_lParticles.m_pData[m_lBranchStartIndices.m_pData[i]]->m_vPosition) + 2.0f;
            m_lBranches.m_pData[i]->m_lControlTangents.m_pData[1] =
                m_lControlTangents.m_pData[m_lControlTangents.m_count - 1];
        }
        else
        {
            Vector vSpan = m_lParticles.m_pData[m_lParticles.m_count - 1]->m_vPosition -
                           m_lParticles.m_pData[0]->m_vPosition;

            float fSeg = magnitude(vSpan) * 0.5f *
                         (static_cast<float>(rand_wincompatible() % 75) / 100.0f + 0.25f);
            if (fSeg != 0.0f)
            {
                Quaternion qSeg = YawPitchRoll(0.0f, static_cast<float>(rand_wincompatible() % 360), 0.0f);
                Vector vAdj =
                    RotateVector(Vector(m_fJitterAxisX, m_fJitterAxisY, m_fJitterAxisZ) * fSeg, qSeg);
                float fPush = static_cast<float>(rand_wincompatible() % 25) / 100.0f + 0.25f;
                Vector vNew = m_lBranches.m_pData[i]->m_lControlPoints.m_pData[0] + vSpan * fPush + vAdj;

                m_lBranches.m_pData[i]->m_spTarget.m_pObject->SetPosition(vNew);
                m_lBranches.m_pData[i]->m_fBirthRate =
                    m_fLightningSubDiv * magnitude(vNew - m_lParticles.m_pData[m_lBranchStartIndices.m_pData[i]]->m_vPosition) + 2.0f;
                m_lBranches.m_pData[i]->m_lControlTangents.m_pData[1] =
                    m_lBranches.m_pData[i]->m_lControlTangents.m_pData[0];
            }
            else
            {
                m_lBranches.m_pData[i]->m_fBirthRate = 0.0f;
                m_lBranches.m_pData[i]->m_spTarget.m_pObject->SetPosition(m_lParticles.m_pData[m_lBranchStartIndices.m_pData[i]]->m_vPosition);
            }

            m_lBranches.m_pData[i]->m_fTargetSize = 0.0f;
            m_lBranches.m_pData[i]->m_lControlPoints.m_pData[1] =
                m_lBranches.m_pData[i]->m_spTarget.m_pObject->GetPosition();

        // Both operands are the same element, so the expression self-subtracts.
            Vector vAxis = normalize(m_lBranches.m_pData[i]->m_lControlPoints.m_pData[1] -
                                     m_lBranches.m_pData[i]->m_lControlPoints.m_pData[1]);
            m_lBranches.m_pData[i]->m_fJitterAxisX = -vAxis.y;
            m_lBranches.m_pData[i]->m_fJitterAxisY = vAxis.z;
            m_lBranches.m_pData[i]->m_fJitterAxisZ = -vAxis.x;
        }

        m_lBranches.m_pData[i]->m_vBoltSpan =
            m_lBranches.m_pData[i]->m_spTarget.m_pObject->GetPosition() -
            m_lBranches.m_pData[i]->m_avVectors[6];
        m_lBranches.m_pData[i]->m_vBoltOrigin = m_lBranches.m_pData[i]->m_avVectors[6];
    }
}

void LightningEmitter::UpdateControlPoints()
{
    float fYawRange = m_fControlPtRadius;
    float fPitchRange = m_fTangentSpread;
    Vector vTarget = m_spTarget.m_pObject->GetPosition();

    // The segment count is the target distance at the emitter's own point density, rounded.
    int nSeg = static_cast<int>(magnitude(vTarget - m_avVectors[6]) * m_fNumControlPts + 0.5f);
    int nCount = nSeg + 2;

    // Only a changed point count rebuilds the ladder; otherwise the previous frame's geometry
    // stands and only the jitter and particle placement below are redone.
    if (nCount != m_lControlPoints.m_count)
    {
        if (nCount > m_lControlPoints.m_count)
        {
            Vector vLast = m_lControlPoints.m_pData[m_lControlPoints.m_count - 1];
            m_lControlPoints.SetSize(nCount);
            m_lControlTangents.SetSize(nCount);
            m_lControlPoints.m_pData[nSeg + 1] = vLast;
            m_lPrevPoints.SetSize(nCount);
            m_lNextPoints.SetSize(nCount);
            m_lPrevTangents.SetSize(nCount);
            m_lNextTangents.SetSize(nCount);
        }
        else
        {
            m_lControlPoints.m_pData[nSeg + 1] = m_lControlPoints.m_pData[m_lControlPoints.m_count - 1];

            // Shrinking truncates the six lists in place; SetSize would reallocate a smaller
            // buffer.
            List<Vector>* apLists[6] = { &m_lControlPoints, &m_lPrevPoints, &m_lNextPoints,
                                         &m_lControlTangents, &m_lPrevTangents, &m_lNextTangents };
            while (m_lControlPoints.m_count > nCount)
                for (int k = 0; k < 6; k++)
                    apLists[k]->m_count--;
        }

        // The two end directions are the emitter's world up-axis at the head and the direction to
        // the target at the tail. Both are scaled by the emitter's bolt radius.
        Matrix matLocal = m_qLocalOrientation.getmatrix();
        Vector vUp(matLocal.m_f[6], matLocal.m_f[7], matLocal.m_f[8]);

        m_lControlTangents.m_pData[0] = vUp * m_fTangentLength;
        m_lControlTangents.m_pData[nSeg + 1] = normalize(vTarget - m_avVectors[6]) * m_fTangentLength;

        // The interior points and their tangents.
        Vector vDir;
        int nSteps = m_lControlPoints.m_count - 1;
        if (nSteps >= 2)
        {
            for (int i = 1; i < nSteps; i++)
            {
                float fT = static_cast<float>(i) / static_cast<float>(nSteps);
                if (fT > 1.0f)
                    fT = 1.0f;
                float fU = 1.0f - fT;

                Vector vHead = m_lControlPoints.m_pData[0];

                Matrix matHead = m_spTarget.m_pObject->GetOrientation().getmatrix();
                Vector vUpHead(matHead.m_f[6], matHead.m_f[7], matHead.m_f[8]);

                m_lControlPoints.m_pData[i] =
                    vHead * (fU * fU * fU)
                  + (vHead + vUp * m_fTangentLength) * (3.0f * fT * fU * fU)
                  + (m_lControlPoints.m_pData[m_lControlPoints.m_count - 1] + vUpHead * m_fTangentLength)
                    * (3.0f * fT * fT * fU)
                  + m_lControlPoints.m_pData[m_lControlPoints.m_count - 1] * (fT * fT * fT);

                // The tangent is the curve's own derivative.
                Matrix matHead2 = m_spTarget.m_pObject->GetOrientation().getmatrix();
                Vector vUpHead2(matHead2.m_f[6], matHead2.m_f[7], matHead2.m_f[8]);

                vDir =
                    vHead * (-3.0f * fU * fU)
                  + (vHead + vUp * m_fTangentLength) * (3.0f * fU * (1.0f - 3.0f * fT))
                  + (m_lControlPoints.m_pData[m_lControlPoints.m_count - 1] + vUpHead2 * m_fTangentLength)
                    * (3.0f * fT * (2.0f - 3.0f * fT))
                  + m_lControlPoints.m_pData[m_lControlPoints.m_count - 1] * (3.0f * fT * fT);
                vDir.Normalize();
                m_lControlTangents.m_pData[i] = vDir * m_fTangentLength;
            }
        }
    }

    // In the per-point jitter pair, both lists keep the previous frame's jittered points before
    // this frame's are rolled.
    if (m_lControlPoints.m_count >= 1)
    {
        int nYawRange = static_cast<int>(fYawRange * 100.0f);
        int nPitchRange = static_cast<int>(fPitchRange * 100.0f);

        for (int i = 0; i < m_lControlPoints.m_count; i++)
        {
            m_lPrevPoints.m_pData[i] = m_lNextPoints.m_pData[i];
            m_lPrevTangents.m_pData[i] = m_lNextTangents.m_pData[i];

            if (i == 0)
            {
                m_lNextPoints.m_pData[0] = m_lControlPoints.m_pData[0];
                Matrix matStart = m_qWorldOrientation.getmatrix();
                m_lNextTangents.m_pData[0] = Vector(matStart.m_f[6], matStart.m_f[7], matStart.m_f[8])
                                       * m_fTangentLength;
            }
            else if (i == m_lControlPoints.m_count - 1)
            {
                m_lNextPoints.m_pData[i] = m_lControlPoints.m_pData[i];
                m_lNextTangents.m_pData[i] = normalize(m_spTarget.m_pObject->GetPosition() -
                                                   m_avVectors[6]) * m_fTangentLength;
            }
            else
            {
                if (nYawRange != 0)
                {
                    int nYaw = rand_wincompatible() % nYawRange;
                    float fYaw = (rand_wincompatible() & 1) ? static_cast<float>(nYaw) * -0.01f
                                                            : static_cast<float>(nYaw) * 0.01f;
                    Quaternion qYaw = YawPitchRoll(0.0f, static_cast<float>(rand_wincompatible() % 360), 0.0f);
                    m_lNextPoints.m_pData[i] =
                        m_lControlPoints.m_pData[i] +
                        RotateVector(Vector(m_fJitterAxisX, m_fJitterAxisY, m_fJitterAxisZ) * fYaw, qYaw);
                }
                else
                {
                    m_lNextPoints.m_pData[i] = m_lControlPoints.m_pData[i];
                }

                if (nPitchRange != 0)
                {
                    int nYaw2 = rand_wincompatible() % nPitchRange;
                    float fYaw2 = (rand_wincompatible() & 1) ? static_cast<float>(nYaw2) * -0.01f
                                                             : static_cast<float>(nYaw2) * 0.01f;
                    int nPitch = rand_wincompatible() % nPitchRange;
                    float fPitch = (rand_wincompatible() & 1) ? static_cast<float>(nPitch) * -0.01f
                                                              : static_cast<float>(nPitch) * 0.01f;
                    Quaternion qPitch = YawPitchRoll(fYaw2, fPitch, 0.0f);
                    m_lNextTangents.m_pData[i] = RotateVector(m_lControlTangents.m_pData[i], qPitch);
                }
                else
                {
                    m_lNextTangents.m_pData[i] = m_lControlTangents.m_pData[i];
                }
            }

            if (m_bIsBranch)
                m_lPrevPoints.m_pData[i] = m_lNextPoints.m_pData[i];
        }
    }

    // Place every particle along the polyline.
    if (m_lParticles.m_count > 1)
    {
        int j = 1;
        float fLo = 0.0f;
        float fHi = 1.0f / static_cast<float>(m_lPrevPoints.m_count - 1);

        for (int i = 1; i < m_lParticles.m_count; i++)
        {
            float fT = static_cast<float>(i) / static_cast<float>(m_lParticles.m_count - 1);

            while (fT > fHi)
            {
                float fStep = 1.0f / static_cast<float>(m_lPrevPoints.m_count - 1);
                fLo = fHi;
                fHi += fStep;
                if (j + 1 >= m_lPrevPoints.m_count)
                {
                    fLo -= fStep;
                    fHi -= fStep;
                    break;
                }
                j++;
            }

            float fLocal = (fT - fLo) / (fHi - fLo);
            if (fLocal > 1.0f)
                fLocal = 1.0f;
            float fInv = 1.0f - fLocal;

            Vector vA = m_lPrevPoints.m_pData[j - 1];
            Vector vB = m_lPrevTangents.m_pData[j - 1];
            Vector vC = m_lPrevPoints.m_pData[j];
            Vector vD = m_lPrevTangents.m_pData[j];

            m_lParticles.m_pData[i]->m_vPosition =
                vA * (fInv * fInv * fInv)
              + (vA + vB) * (3.0f * fLocal * fInv * fInv)
              + (vC - vD) * (3.0f * fLocal * fLocal * fInv)
              + vC * (fLocal * fLocal * fLocal);
        }
    }
}

#if defined(K2_MOBILE_OPT) && !KOTOR1
static float& Max(float& fA, float& fB);
#endif
#if KOTOR1
extern volatile float s_fLightningLimit __attribute__((visibility("hidden")));
#else
extern float s_fLightningLimit __attribute__((visibility("hidden")));
#endif

void ConstrainBirthrateLightning(PartEmitter* pEmitter);
void LightningEmitter::Update(float fDeltaT)
{
    // Clamp the birthrate once for the whole frame.
    ConstrainBirthrateLightning(this);

    Vector vDelta;
    // The particle scale is the owning Gob's object scale, or 1.0f when the part has no Gob yet.
    float fScale = (m_pGob != NULL) ? m_pGob->m_fObjectScale : 1.0f;
    int nBirth = static_cast<int>(m_fBirthRate);

    int nCount = m_lParticles.m_count;
    m_fDelayTimer += fDeltaT;

    // With no birth this frame, or no watched target, retire every particle onto the free list.
    if (nBirth == 0 || m_spTarget.m_pObject == NULL)
    {
        for (int i = m_lParticles.m_count; i > 0; i--)
        {
            Particle* pParticle = m_lParticles.m_pData[i - 1];
            m_lParticles.DelIndex(i - 1);
            m_lFreeParticles.Add(pParticle);
        }
        return;
    }

    // Grow to the birth count, reusing the free list, or trim down to it.
    if (nCount < nBirth)
    {
        for (int i = nCount; i < nBirth; i++)
        {
            Particle* pParticle;
            if (m_lFreeParticles.m_count > 0)
            {
                pParticle = m_lFreeParticles.m_pData[m_lFreeParticles.m_count - 1];
                m_lFreeParticles.DelIndex(m_lFreeParticles.m_count - 1);
            }
            else
            {
                pParticle = new Particle(this, fDeltaT);
            }

            chkRandom(pParticle);
            m_lParticles.Add(pParticle);
        }
    }
    else if (nBirth < nCount)
    {
        // The trim runs down to index nBirth inclusive.
        for (int i = m_lParticles.m_count - 1; i >= nBirth; i--)
        {
            Particle* pParticle = m_lParticles.m_pData[i];
            m_lParticles.DelIndex(i);
            m_lFreeParticles.Add(pParticle);
        }
    }

    // The fragment buffer holds one entry per live particle.
    if (m_lParticleOffsets.m_count < m_lParticles.m_count)
        m_lParticleOffsets.SetSize(m_lParticles.m_count);

    // The per-particle age stepper, and the mode-9 position blend.
    for (int i = 0; i < m_lParticles.m_count; i++)
    {
        Particle* pParticle = m_lParticles.m_pData[i];

        // A particle whose age has reached its frame anchor restarts, clearing both age fields.
        if (pParticle->m_fAge == pParticle->m_fLastFrameAge)
        {
            pParticle->m_fAge = 0.0f;
            pParticle->m_fLastFrameAge = 0.0f;
            pParticle->m_fFrameTime = 0.0f;
        }
        pParticle->m_fAge += fDeltaT;

        if (m_nRenderMode == 9)
        {
            // The trail blend factor is this frame's time capped by the emitter's own trail rate.
            float fBlend = (m_fBlurLength < fDeltaT ? m_fBlurLength : fDeltaT) / m_fBlurLength;
            pParticle->m_vTrailPosition.x = pParticle->m_vPosition.x * fBlend
                                    + pParticle->m_vTrailPosition.x * (1.0f - fBlend);
            pParticle->m_vTrailPosition.y = pParticle->m_vPosition.y * fBlend
                                    + pParticle->m_vTrailPosition.y * (1.0f - fBlend);
            pParticle->m_vTrailPosition.z = pParticle->m_vPosition.z * fBlend
                                    + pParticle->m_vTrailPosition.z * (1.0f - fBlend);
        }
    }

    // On the outer clock, rebuild the bolt or blend onto the ladder.
    m_fControlPtTimer += fDeltaT;
    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode->AsMdlNodeEmitter());
    bool bBranches;

    if (m_fControlPtTimer > m_fControlPtDelay)
    {
        m_fControlPtTimer = 0.0f;

        // The bolt starts at the emitter's own anchor and ends on the watched target's live
        // position; UpdateControlPoints then rebuilds the whole ladder between them.
        m_lControlPoints.m_pData[0] = m_avVectors[6];
        m_lControlPoints.m_pData[m_lControlPoints.m_count - 1] = m_spTarget.m_pObject->GetPosition();
        UpdateControlPoints();

        if (pNode->m_nNumBranches < 1 || m_bIsBranch)
        {
            bBranches = false;
        }
        else
        {
            UpdateBranches();
            // Every live branch emitter has its bolt clock reset to ours, so the children rebuild
            // on the same frame the parent did.
            for (int i = 0; i < m_nActiveBranches; i++)
                m_lBranches.m_pData[i]->m_fControlPtTimer = m_fControlPtDelay;
            bBranches = true;
        }

        // Arm the fade clock one frame out from the emitter's own rate.
        m_fDelayTimer = m_fLightningDelay + 0.01f;
    }
    else if (static_cast<MdlNodeEmitter*>(m_pNode->AsMdlNodeEmitter())->m_bControlPtSmoothing != 0
             && !m_bIsBranch)
    {
        // The ladder blend walks every particle onto the polyline.
        float fFade = (m_fControlPtDelay == 0.0f) ? 0.0f : m_fControlPtTimer / m_fControlPtDelay;

        if (m_lParticles.m_count - 1 >= 2)
        {
            int    nLast   = m_lParticles.m_count - 1;
            int    nSegMax = m_lNextPoints.m_count - 1;
            float  fFadeInv = 1.0f - fFade;
            float  fSegStep = 1.0f / static_cast<float>(nSegMax);
            float  fSegLo = 0.0f;
            int    nSeg = 1;

            for (int i = 1; i < nLast; i++)
            {
                float fT = static_cast<float>(i) / static_cast<float>(nLast);
                if (fSegStep < fT)
                {
                    do
                    {
                        fSegLo = fSegStep;
                        fSegStep = fSegLo + 1.0f / static_cast<float>(nSegMax);
                        nSeg++;
                    } while (fSegStep < fT);
                }
                fT = (fT - fSegLo) / (fSegStep - fSegLo);
                if (1.0f < fT)
                    fT = 1.0f;

                // The segment's control points and control directions, from the four parallel
                // lists.
                const Vector& pA = m_lPrevPoints.m_pData[nSeg - 1];
                const Vector& pB = m_lNextPoints.m_pData[nSeg - 1];
                const Vector& dB = m_lPrevTangents.m_pData[nSeg - 1];
                const Vector& dC = m_lNextTangents.m_pData[nSeg - 1];
                const Vector& pC = m_lPrevPoints.m_pData[nSeg];
                const Vector& pD = m_lNextPoints.m_pData[nSeg];
                const Vector& dD = m_lPrevTangents.m_pData[nSeg];
                const Vector& dE = m_lNextTangents.m_pData[nSeg];

                float fT2 = fT * fT;
                float fInv = 1.0f - fT;
                float fB0 = fInv * fInv * fInv;
                float fB1 = 3.0f * fT * fInv * fInv;
                float fB2 = 3.0f * fT * fT * fInv;
                float fB3 = fT2 * fT;

                // The bezier's four control points are the fade-lerped pairs.
                float ax = fFadeInv * pA.x + fFade * pB.x;
                float ay = fFadeInv * pA.y + fFade * pB.y;
                float az = fFadeInv * pA.z + fFade * pB.z;
                float bx = fFadeInv * pC.x + fFade * pD.x;
                float by = fFadeInv * pC.y + fFade * pD.y;
                float bz = fFadeInv * pC.z + fFade * pD.z;
                float cx = ax + fFadeInv * dB.x + fFade * dC.x;
                float cy = ay + fFadeInv * dB.y + fFade * dC.y;
                float cz = az + fFadeInv * dB.z + fFade * dC.z;
                float dx = bx - (fFadeInv * dD.x + fFade * dE.x);
                float dy = by - (fFadeInv * dD.y + fFade * dE.y);
                float dz = bz - (fFadeInv * dD.z + fFade * dE.z);

                Particle* pParticle = m_lParticles.m_pData[i];
                pParticle->m_vPosition.x = ax * fB0 + cx * fB1 + dx * fB2 + bx * fB3;
                pParticle->m_vPosition.y = ay * fB0 + cy * fB1 + dy * fB2 + by * fB3;
                pParticle->m_vPosition.z = az * fB0 + cz * fB1 + dz * fB2 + bz * fB3;
            }
        }

        bBranches = false;
    }
    else
    {
        bBranches = false;
    }

    // On the inner clock, fade the first particle in or rotate the bolt.
    if (m_fDelayTimer > m_fLightningDelay)
    {
        // To fade in, the first particle is re-seated, then jittered within the emitter's radius.
        Particle* pFirst = m_lParticles.m_pData[0];
        bool bJittered;

        if (m_fXSize == 0.0f && m_fYSize == 0.0f)
        {
            pFirst->m_vPosition = m_avVectors[6];
            bJittered = false;
        }
        else
        {
            pFirst->randomPosition(static_cast<int>(fScale * (m_fXSize * 100.0f)),
                                  static_cast<int>(fScale * (m_fYSize * 100.0f)));
            bJittered = true;
        }

        Particle* pLast = m_lParticles.m_pData[m_lParticles.m_count - 1];
        pLast->m_vPosition = m_spTarget.m_pObject->GetPosition();

        // A non-zero jitter radius gives three independent offsets, one per component.
        bool bTailJittered = false;
        if (m_fTargetSize != 0.0f)
        {
            int n0 = rand_wincompatible();
            int v0 = n0 % static_cast<int>(m_fTargetSize * 50.0f);
            pLast->m_vPosition.x += static_cast<float>((n0 & 1) ? v0 : -v0) / 100.0f;
            int n1 = rand_wincompatible();
            int v1 = n1 % static_cast<int>(m_fTargetSize * 50.0f);
            pLast->m_vPosition.y += static_cast<float>((n1 & 1) ? v1 : -v1) / 100.0f;
            int n2 = rand_wincompatible();
            int v2 = n2 % static_cast<int>(m_fTargetSize * 50.0f);
            pLast->m_vPosition.z += static_cast<float>((n2 & 1) ? v2 : -v2) / 100.0f;
            bTailJittered = true;
        }

        // The ladder hand-off, taken only when an end moved.
        if (!bJittered && !bTailJittered)
        {
            // Neither end moved, so the ladder keeps this frame's geometry.
        }
        else if (m_lControlPoints.m_count == 2)
        {
            if (bJittered)
                m_lControlPoints.m_pData[0] = m_lParticles.m_pData[0]->m_vPosition;
            else
                m_lControlPoints.m_pData[1] = pLast->m_vPosition;
            UpdateControlPoints();
        }
        else if (bJittered)
        {
            m_lNextPoints.m_pData[0] = m_lParticles.m_pData[0]->m_vPosition;
            m_lControlPoints.m_pData[0] = m_lParticles.m_pData[0]->m_vPosition;

            float fFade = (m_fControlPtDelay == 0.0f) ? 0.0f : m_fControlPtTimer / m_fControlPtDelay;
            if (m_lParticles.m_count - 1 >= 2)
            {
                int   nLast    = m_lParticles.m_count - 1;
                int   nSegMax  = m_lNextPoints.m_count - 1;
                float fFadeInv = 1.0f - fFade;
                float fSegStep = 1.0f / static_cast<float>(nSegMax);
                float fSegLo   = 0.0f;
                int   nSeg     = 1;

                for (int i = 1; i < nLast; i++)
                {
                    float fT = static_cast<float>(i) / static_cast<float>(nLast);
                    if (fSegStep < fT)
                    {
                        do
                        {
                            fSegLo = fSegStep;
                            fSegStep = fSegLo + 1.0f / static_cast<float>(nSegMax);
                            nSeg++;
                        } while (fSegStep < fT);
                    }
                    fT = (fT - fSegLo) / (fSegStep - fSegLo);
                    if (1.0f < fT)
                        fT = 1.0f;

                    const Vector& pA = m_lPrevPoints.m_pData[nSeg - 1];
                    const Vector& pB = m_lNextPoints.m_pData[nSeg - 1];
                    const Vector& dB = m_lPrevTangents.m_pData[nSeg - 1];
                    const Vector& dC = m_lNextTangents.m_pData[nSeg - 1];
                    const Vector& pC = m_lPrevPoints.m_pData[nSeg];
                    const Vector& pD = m_lNextPoints.m_pData[nSeg];
                    const Vector& dD = m_lPrevTangents.m_pData[nSeg];
                    const Vector& dE = m_lNextTangents.m_pData[nSeg];

                    float fT2   = fT * fT;
                    float fInv  = 1.0f - fT;
                    float fB0   = fInv * fInv * fInv;
                    float fB1   = 3.0f * fT * fInv * fInv;
                    float fB2   = 3.0f * fT * fT * fInv;
                    float fB3   = fT2 * fT;

                    float ax = fFadeInv * pA.x + fFade * pB.x;
                    float ay = fFadeInv * pA.y + fFade * pB.y;
                    float az = fFadeInv * pA.z + fFade * pB.z;
                    float bx = fFadeInv * pC.x + fFade * pD.x;
                    float by = fFadeInv * pC.y + fFade * pD.y;
                    float bz = fFadeInv * pC.z + fFade * pD.z;
                    float cx = ax + fFadeInv * dB.x + fFade * dC.x;
                    float cy = ay + fFadeInv * dB.y + fFade * dC.y;
                    float cz = az + fFadeInv * dB.z + fFade * dC.z;
                    float dx = bx - (fFadeInv * dD.x + fFade * dE.x);
                    float dy = by - (fFadeInv * dD.y + fFade * dE.y);
                    float dz = bz - (fFadeInv * dD.z + fFade * dE.z);

                    Particle* pParticle = m_lParticles.m_pData[i];
                    pParticle->m_vPosition.x = ax * fB0 + cx * fB1 + dx * fB2 + bx * fB3;
                    pParticle->m_vPosition.y = ay * fB0 + cy * fB1 + dy * fB2 + by * fB3;
                    pParticle->m_vPosition.z = az * fB0 + cz * fB1 + dz * fB2 + bz * fB3;

                    // Only the head moved, so only the first segment is re-blended: the walk stops
                    // after the first particle past it.
                    if (nSeg > 1)
                        break;
                }
            }
        }
        else
        {
            Particle* pFirst2 = m_lParticles.m_pData[0];
            Particle* pLast2  = m_lParticles.m_pData[m_lParticles.m_count - 1];
            Vector vEnd = pLast2->m_vPosition;
            int nSegCount = m_lNextPoints.m_count;
            m_lNextPoints.m_pData[nSegCount - 1] = vEnd;
            m_lControlPoints.m_pData[nSegCount - 1] = vEnd;
            (void)pFirst2;

            float fFade = (m_fControlPtDelay == 0.0f) ? 0.0f : m_fControlPtTimer / m_fControlPtDelay;
            int nBase = m_lNextPoints.m_count - 2;
            float fSpan = static_cast<float>(nBase) / static_cast<float>(m_lNextPoints.m_count - 1);
            int nStart = static_cast<int>(fSpan * static_cast<float>(m_lParticles.m_count)) + 1;

            if (nStart < m_lParticles.m_count - 1)
            {
                float fFadeInv = 1.0f - fFade;
                for (int i = nStart; i < m_lParticles.m_count - 1; i++)
                {
                    float fT = (static_cast<float>(i) / static_cast<float>(m_lParticles.m_count - 1) - fSpan)
                             / (1.0f - fSpan);
                    if (1.0f < fT)
                        fT = 1.0f;
                    float fInv = 1.0f - fT;

                    const Vector& pA = m_lPrevPoints.m_pData[nBase];
                    const Vector& pB = m_lNextPoints.m_pData[nBase];
                    const Vector& dB = m_lPrevTangents.m_pData[nBase];
                    const Vector& dC = m_lNextTangents.m_pData[nBase];
                    const Vector& pC = m_lPrevPoints.m_pData[nSegCount - 1];
                    const Vector& pD = m_lNextPoints.m_pData[nSegCount - 1];
                    const Vector& dD = m_lPrevTangents.m_pData[nSegCount - 1];
                    const Vector& dE = m_lNextTangents.m_pData[nSegCount - 1];

                    float fB0 = fInv * fInv * fInv;
                    float fB1 = 3.0f * fT * fInv * fInv;
                    float fB2 = 3.0f * fT * fT * fInv;
                    float fB3 = fT * fT * fT;

                    float ax = fFadeInv * pA.x + fFade * pB.x;
                    float ay = fFadeInv * pA.y + fFade * pB.y;
                    float az = fFadeInv * pA.z + fFade * pB.z;
                    float bx = fFadeInv * pC.x + fFade * pD.x;
                    float by = fFadeInv * pC.y + fFade * pD.y;
                    float bz = fFadeInv * pC.z + fFade * pD.z;
                    float cx = ax + fFadeInv * dB.x + fFade * dC.x;
                    float cy = ay + fFadeInv * dB.y + fFade * dC.y;
                    float cz = az + fFadeInv * dB.z + fFade * dC.z;
                    float dx = bx - (fFadeInv * dD.x + fFade * dE.x);
                    float dy = by - (fFadeInv * dD.y + fFade * dE.y);
                    float dz = bz - (fFadeInv * dD.z + fFade * dE.z);

                    Particle* pParticle = m_lParticles.m_pData[i];
                    pParticle->m_vPosition.x = ax * fB0 + cx * fB1 + dx * fB2 + bx * fB3;
                    pParticle->m_vPosition.y = ay * fB0 + cy * fB1 + dy * fB2 + by * fB3;
                    pParticle->m_vPosition.z = az * fB0 + cz * fB1 + dz * fB2 + bz * fB3;
                }
            }
        }

        vDelta = m_lParticles.m_pData[0]->m_vPosition - pLast->m_vPosition;
        if (!equal(vDelta, Vector()))
        {
            m_fDelayTimer = 0.0f;

            if (useoldlightninginterp == 0)
            {
                // Jitter the bolt along its normalized first-to-last axis. The yaw accumulator
                // spirals the jitter direction down the bolt.
                Vector vDir = normalize(vDelta);
                // The yaw axis runs from the emitter's own anchor to the watched target.
                Vector vAxis = m_spTarget.m_pObject->GetPosition() - m_avVectors[6];
                int  nParticles = m_lParticles.m_count;
                float fLen = sqrtf(vDelta.x * vDelta.x + vDelta.y * vDelta.y
                                 + vDelta.z * vDelta.z);
                float fRate = m_fLightningScale * m_scale.m_fScale * ((fLen / static_cast<float>(nParticles)) * 0.5f);
                Vector vSpread(m_fJitterAxisX, m_fJitterAxisY, m_fJitterAxisZ);
                int  nSteps = static_cast<int>(m_fLightningZigzag);

                if (nParticles > 2)
                {
                    vDir = vDir * fRate;

                    int nRoll = 0;
                    for (int i = 1; i < nParticles - 1; i++)
                    {
                        if (nSteps != 0)
                        {
                            int nYaw = rand_wincompatible() % nSteps;
                            if (rand_wincompatible() & 1)
                                nYaw = 360 - nYaw;
                            nRoll += nYaw;
                            Quaternion qYaw(vAxis, static_cast<float>(nRoll) * 0.017453292f);
                            vSpread = RotateVector(vSpread, qYaw);
                        }

                        float fLocal = static_cast<float>(i) / static_cast<float>(nParticles);
                        if (fLocal > 0.5f)
                            fLocal = 1.0f - fLocal;
                        float fAmp = m_fLightningRadius * (fLocal + fLocal);

                        float fAmpJitter = fAmp * static_cast<float>(rand_wincompatible() % 100) * 0.01f;
                        Vector vJitter = vDir * (static_cast<float>(rand_wincompatible() % 100) * 0.01f);
                        if (rand_wincompatible() & 1)
                            vJitter = -vJitter;

                        m_lParticleOffsets.m_pData[i] = vSpread * fAmpJitter + vJitter;
                    }
                }

                // Re-anchor each live branch emitter on its particle.
                if (pNode->m_nNumBranches > 0 && m_lBranches.m_count != 0 && m_nActiveBranches > 0)
                {
                    for (int i = 0; i < m_nActiveBranches; i++)
                    {
                        int nIdx = m_lBranchStartIndices.m_pData[i];
                        LightningEmitter* pChild = m_lBranches.m_pData[i];
                        Vector v = m_lParticleOffsets.m_pData[nIdx]
                                 + m_lParticles.m_pData[nIdx]->m_vPosition;
                        pChild->m_lNextPoints.m_pData[0] = v;
                        pChild->m_lPrevPoints.m_pData[0] = v;
                        pChild->m_avVectors[6] = v;
                        pChild->m_vBoltSpan = pChild->m_spTarget.m_pObject->GetPosition()
                                            - pChild->m_avVectors[6];
                        pChild->m_vBoltOrigin = pChild->m_avVectors[6];
                    }
                }
            }
            else
            {
                // The legacy jitter path, a random cloud spread along the bolt.
                float fLen = m_fLightningSubDiv * magnitude(vDelta) * 0.5f;
                int nSeg = static_cast<int>(fLen);
                Vector vDir = normalize(vDelta);

                // The second axis is vDir x (-y, z, -x).
                RecursiveFractal(nSeg, nSeg, 1,
                                 Vector(-vDir.y, vDir.z, -vDir.x),
                                 Vector(vDir.y * -vDir.x - vDir.z * vDir.z,
                                        vDir.x * vDir.x - vDir.z * vDir.y,
                                        vDir.z * vDir.x + vDir.y * vDir.y),
                                 fScale);

                float fSpread = (sqrtf(vDelta.x * vDelta.x + vDelta.y * vDelta.y
                                      + vDelta.z * vDelta.z) / static_cast<float>(m_lParticles.m_count)) * 0.5f;
                if (fSpread >= 0.01 && m_lParticles.m_count > 2)
                {
                    int nRange = static_cast<int>(fSpread * 100.0f);
                    for (int i = 1; i < m_lParticles.m_count - 1; i++)
                    {
                        float fSx = vDir.x;
                        float fSy = vDir.y;
                        float fSz = vDir.z;
                        // An even roll negates the offset.
                        if (!(rand_wincompatible() & 1))
                        {
                            fSx = -fSx;
                            fSy = -fSy;
                            fSz = -fSz;
                        }
                        float fR = static_cast<float>((rand_wincompatible() % nRange) * 0.01);
                        Particle* pParticle = m_lParticles.m_pData[i];
                        float fJx = m_scale.m_fScale * (m_fLightningScale * (fSx * fR));
                        float fJy = m_scale.m_fScale * (m_fLightningScale * (fSy * fR));
                        float fJz = m_scale.m_fScale * (m_fLightningScale * (fSz * fR));
                        pParticle->m_vPosition.x += fJx;
                        pParticle->m_vPosition.y += fJy;
                        pParticle->m_vPosition.z += fJz;
                    }
                }
            }
        }
    }
    else
    {
        // Rotate the particles and the ladder by the arc from the cached target direction onto
        // the current one, then carry the child branches along.
        Vector vTarget = m_spTarget.m_pObject->GetPosition();
        Quaternion qArc = RotationArc(m_vBoltSpan, vTarget - m_avVectors[6]);
        bool bIdentity = (quaternionidentityoptimization != 0)
                      && (qArc.w == 1.0f || qArc.w == -1.0f);

        for (int i = 0; i < m_lParticles.m_count; i++)
        {
            Particle* pParticle = m_lParticles.m_pData[i];
            Vector v = pParticle->m_vPosition - m_vBoltOrigin;
            if (!bIdentity)
                v = RotateVector(v, qArc);
            pParticle->m_vPosition = v + m_avVectors[6];
        }

        for (int i = 0; i < m_lControlPoints.m_count; i++)
        {
            Vector v = m_lControlPoints.m_pData[i] - m_vBoltOrigin;
            if (!bIdentity)
                v = RotateVector(v, qArc);
            m_lControlPoints.m_pData[i] = v + m_avVectors[6];

            Vector d = m_lControlTangents.m_pData[i];
            if (!bIdentity)
                d = RotateVector(d, qArc);
            m_lControlTangents.m_pData[i] = d;
        }

        if (pNode->m_nNumBranches > 0 && m_lBranches.m_count != 0 && m_nActiveBranches > 0)
        {
            for (int i = 0; i < m_nActiveBranches; i++)
            {
                int nIdx = m_lBranchStartIndices.m_pData[i];
                LightningEmitter* pChild = m_lBranches.m_pData[i];
                Vector v = m_lParticleOffsets.m_pData[nIdx] + m_lParticles.m_pData[nIdx]->m_vPosition;
                pChild->m_lNextPoints.m_pData[0] = v;
                pChild->m_lPrevPoints.m_pData[0] = v;
                pChild->m_avVectors[6] = v;

                Vector vHere = pChild->m_spTarget.m_pObject->GetPosition() - m_vBoltOrigin;
                if (!bIdentity)
                    vHere = RotateVector(vHere, qArc);
                pChild->m_spTarget.m_pObject->SetPosition(vHere + m_avVectors[6]);
            }
        }

        m_vBoltSpan = vTarget - m_avVectors[6];
        m_vBoltOrigin = m_avVectors[6];
    }

    // On the branch emitters' own clocks, a rebuilt parent drags every live child onto the emitter
    // rate; otherwise each child advances by this frame's delta.
    if (pNode->m_nNumBranches > 0 && !m_bIsBranch && m_nActiveBranches > 0)
    {
        for (int i = 0; i < m_nActiveBranches; i++)
        {
            if (bBranches)
                m_lBranches.m_pData[i]->Update(m_fLightningDelay + 0.01f);
            else
                m_lBranches.m_pData[i]->Update(fDeltaT);
        }
    }
}

// The GL state bracket for the emitter render layer sets up positions, colours and texcoords with
// no normals, additive blending, and the depth mask handed to the backend when distortion output is
// on.

void SetDistortBuffer(bool bFlag);

int auremitterrendertangents;
int auremitterrenderbezier;
int auremitterrenderbranchbezier;
int auremitterrenderlightning = 1;

extern unsigned char g_ForceSight;

extern unsigned int enableTranslationTable[];
extern unsigned int clientStateTranslationTable[];
extern unsigned int blendTranslationTable[];
#if KOTOR1
extern unsigned int stencilOpTranslationTable[];
extern const unsigned int cmpTranslationTable[];
#endif
extern const int facingTranslationTable[];
extern const int fillTranslationTable[];
enum {
    kCapBlend     = 0,    // GL_BLEND
    kCapAlphaTest = 1,    // GL_ALPHA_TEST
    kCapFog       = 2,    // GL_FOG
    kCapCullFace  = 3,    // GL_CULL_FACE
    kCapDepthTest = 5,    // GL_DEPTH_TEST
    kCapMultisample = 19, // GL_MULTISAMPLE
};
enum {
    kClientStateTexCoord = 0,  // GL_TEXTURE_COORD_ARRAY
    kClientStateNormal   = 1,  // GL_NORMAL_ARRAY
    kClientStateColor    = 2,  // GL_COLOR_ARRAY
    kClientStateVertex   = 3,  // GL_VERTEX_ARRAY
};

// While distortion output is on the GL backend owns the depth mask and the emitter layer must
// leave it alone.
#if KOTOR1  // K1 has no distortion output
static const bool g_useDistortionOutput = false;
#else
extern unsigned char g_useDistortionOutput;
#endif

void PartEmitter::StartEmitterLayer()
{
    if (!g_useDistortionOutput)
        ASLgl::glDepthMask(0);
    ASLgl::glDisable(enableTranslationTable[kCapDepthTest]);
    ASLgl::glEnable(enableTranslationTable[kCapBlend]);
    ASLgl::glDisable(enableTranslationTable[kCapAlphaTest]);
    ASLgl::glEnableClientState(clientStateTranslationTable[kClientStateVertex]);
    ASLgl::glEnableClientState(clientStateTranslationTable[kClientStateColor]);
    ASLgl::glEnableClientState(clientStateTranslationTable[kClientStateTexCoord]);
    ASLgl::glDisableClientState(clientStateTranslationTable[kClientStateNormal]);
}

void PartEmitter::StopEmitterLayer()
{
    if (!g_useDistortionOutput)
        ASLgl::glDepthMask(1);
    ASLgl::glEnable(enableTranslationTable[kCapDepthTest]);
    ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
    ASLgl::glEnableClientState(clientStateTranslationTable[kClientStateNormal]);
}

void PartEmitter::Animate(float fDeltaT) { Part::Animate(fDeltaT); }

// Per-kind birthrate clamps. Above its kind's limit the rate halves but never drops below it. Only
// the mobile build spawns at the clamped rate.
#if KOTOR1  // K1 reads every limit as a volatile: the clamp's store re-loads it
extern volatile float s_fExplosionThreshold __attribute__((visibility("hidden")));
extern volatile float s_fExplosionLow       __attribute__((visibility("hidden")));
extern volatile float s_fExplosionHigh      __attribute__((visibility("hidden")));
extern volatile float s_fFountainDefault    __attribute__((visibility("hidden")));
extern volatile float s_fFountainPacked     __attribute__((visibility("hidden")));
extern volatile float s_fSingleLimit        __attribute__((visibility("hidden")));
extern volatile float s_fLightningLimit     __attribute__((visibility("hidden")));
volatile float s_fSingleLimit        = 17.0f;
volatile float s_fLightningLimit     = 20.0f;
volatile float s_fExplosionThreshold = 0.3f;
volatile float s_fExplosionLow       = 3.0f;
volatile float s_fExplosionHigh      = 7.0f;
volatile float s_fFountainPacked     = 3.0f;
volatile float s_fFountainDefault    = 15.0f;
#else
extern float s_fExplosionThreshold __attribute__((visibility("hidden")));
extern float s_fExplosionLow       __attribute__((visibility("hidden")));
extern float s_fExplosionHigh      __attribute__((visibility("hidden")));
extern float s_fFountainDefault    __attribute__((visibility("hidden")));
extern float s_fFountainPacked     __attribute__((visibility("hidden")));
extern float s_fSingleLimit        __attribute__((visibility("hidden")));
extern float s_fLightningLimit     __attribute__((visibility("hidden")));
#endif

#if defined(K2_MOBILE_OPT) && !KOTOR1
static float& Max(float& fA, float& fB) { return fA < fB ? fB : fA; }
#endif

void ConstrainBirthrateSingle(PartEmitter* pEmitter)
{
#ifdef K2_MOBILE_OPT
#if !KOTOR1
    float fHalf;
#endif
#if KOTOR1  // K1 re-reads the limit word for the store
    volatile float& fLimit = s_fSingleLimit;
#else
    float fLimit = s_fSingleLimit;
#endif
#if KOTOR1  // K1 clamps the rate to the limit instead of halving it
    if (pEmitter->m_fBirthRate > fLimit)
        pEmitter->m_fBirthRate = fLimit;
#else
    if (pEmitter->m_fBirthRate > fLimit)
    {
        fHalf = pEmitter->m_fBirthRate * 0.5f;
        pEmitter->m_fBirthRate = Max(fLimit, fHalf);
    }
#endif
#else
    K2_UNUSED(pEmitter);
#endif
}

void ConstrainBirthrateLightning(PartEmitter* pEmitter)
{
#ifdef K2_MOBILE_OPT
#if !KOTOR1
    float fHalf;
#endif
#if KOTOR1  // K1 re-reads the limit word for the store
    volatile float& fLimit = s_fLightningLimit;
#else
    float fLimit = s_fLightningLimit;
#endif
#if KOTOR1  // K1 clamps the rate to the limit instead of halving it
    if (pEmitter->m_fBirthRate > fLimit)
        pEmitter->m_fBirthRate = fLimit;
#else
    if (pEmitter->m_fBirthRate > fLimit)
    {
        fHalf = pEmitter->m_fBirthRate * 0.5f;
        pEmitter->m_fBirthRate = Max(fLimit, fHalf);
    }
#endif
#else
    K2_UNUSED(pEmitter);
#endif
}

void ConstrainBirthrateFountain(PartEmitter* pEmitter)
{
#ifdef K2_MOBILE_OPT
#if !KOTOR1
    float fHalf;
#endif
#if KOTOR1  // K1 clamps the rate to the limit (re-read) instead of halving it
    if (pEmitter->m_fLifeExp >= 3.0f)
    {
        if (pEmitter->m_fBirthRate > s_fFountainPacked)
            pEmitter->m_fBirthRate = s_fFountainPacked;
    }
    else if (pEmitter->m_fBirthRate > s_fFountainDefault)
        pEmitter->m_fBirthRate = s_fFountainDefault;
#else
    float fLimit = pEmitter->m_fLifeExp >= 3.0f ? s_fFountainPacked : s_fFountainDefault;

    if (pEmitter->m_fBirthRate > fLimit)
    {
        fHalf = pEmitter->m_fBirthRate * 0.5f;
        pEmitter->m_fBirthRate = Max(fLimit, fHalf);
    }
#endif
#else
    K2_UNUSED(pEmitter);
#endif
}
void ConstrainBirthrateExplosion(PartEmitter* pEmitter)
{
#ifdef K2_MOBILE_OPT
    float fMax = pEmitter->m_fSizeStart < pEmitter->m_fSizeEnd ? pEmitter->m_fSizeEnd : pEmitter->m_fSizeStart;
#if KOTOR1
    if (fMax < pEmitter->m_fSizeMid)
#else
    if (pEmitter->m_fSizeMid > fMax)
#endif
        fMax = pEmitter->m_fSizeMid;
#if KOTOR1  // K1 clamps the rate to the limit (re-read) instead of halving it
    if (fMax >= s_fExplosionThreshold)
    {
        if (pEmitter->m_fBirthRate > s_fExplosionLow)
            pEmitter->m_fBirthRate = s_fExplosionLow;
    }
    else if (pEmitter->m_fBirthRate > s_fExplosionHigh)
        pEmitter->m_fBirthRate = s_fExplosionHigh;
#else
    float& fLimitRef = !(s_fExplosionThreshold <= fMax) ? s_fExplosionHigh : s_fExplosionLow;
    float fHalf;
    float fLimit = fLimitRef;
    if (pEmitter->m_fBirthRate > fLimit)
    {
        fHalf = pEmitter->m_fBirthRate * 0.5f;
        pEmitter->m_fBirthRate = Max(fLimit, fHalf);
    }
#endif
#else
    K2_UNUSED(pEmitter);
#endif
}


void SingleEmitter::Update(float fDeltaT)
{
    // Halve the birth rate and floor it at its kind's limit.
    ConstrainBirthrateSingle(this);

    // A birth rate that has decayed below one emission per frame retires the live particle.
    if (static_cast<int>(m_fBirthRate) == 0)
    {
        if (m_lParticles.m_count != 0)
        {
            Particle* pParticle = m_lParticles.m_pData[0];
            m_lParticles.DelIndex(0);
            delete pParticle;
        }
        return;
    }

    if (m_lParticles.m_count == 0)
    {
        MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode);
        // On the first frame, build the particle the node's chunk name selects.
        if (pNode->m_szChunkName[0] != 0)
        {
            ChunkyParticle* pChunky = new ChunkyParticle(this, fDeltaT);
            m_lParticles.Add(pChunky);
        }
        else
        {
            Particle* pFresh = new Particle(this, fDeltaT);
            m_lParticles.Add(pFresh);
        }
        chkRandom(m_lParticles.m_pData[0]);
        return;
    }

    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode);
    Particle* pParticle = m_lParticles.m_pData[0];

    // Age the particle; a finite lifetime either wraps (the node's `loop`) or keeps counting, and
    // an infinite one (lifetime <= 0) is only pulled back if the clock went negative.
    pParticle->m_fAge += fDeltaT;
    if (m_fLifeExp > 0.0f)
    {
        if (pParticle->m_fAge > m_fLifeExp && pNode->m_nLoop != 0)
            pParticle->m_fAge = 0.0f;
    }
    else if (pParticle->m_fAge < 0.0f)
    {
        pParticle->m_fAge = fDeltaT;
    }

    // The orientation accumulator is spun by the emitter's own rate and wrapped into +-2*pi. It
    // is a wrap rather than a clamp, so the overshoot carries.
    pParticle->m_fRotation += m_fParticleRot * fDeltaT;
    if (pParticle->m_fRotation > 6.2831855f)
        pParticle->m_fRotation -= 6.2831855f;
    else if (pParticle->m_fRotation < -6.2831855f)
        pParticle->m_fRotation += 6.2831855f;

    // The motion-blur mode trails a ghost position behind the live one, catching it up at the
    // emitter's own rate capped by the frame time.
    if (m_nRenderMode == 9)
    {
        float t = m_fBlurLength < fDeltaT ? m_fBlurLength : fDeltaT;
        t = t / m_fBlurLength;
        pParticle->m_vTrailPosition =
            pParticle->m_vPosition * t + pParticle->m_vTrailPosition * (1.0f - t);
    }

    // Pick the movement integrator from the node's point-to-point flags.
    MdlNodeEmitter* pDispatch = static_cast<MdlNodeEmitter*>(m_pNode->AsMdlNodeEmitter());
    if (!(pDispatch->m_nEmitterFlags & 1))
        return moveParticleStandard(fDeltaT);
    if (!m_spTarget.m_pObject)
        return;
    if (pDispatch->m_nEmitterFlags & 2)
        return moveParticleP2PBezier(fDeltaT);
    return moveParticleP2PGrav(fDeltaT);
}

void FountainEmitter::Update(float fDeltaT)
{
#ifdef K2_MOBILE_OPT
#if !KOTOR1
    float fHalf;
#endif
#if KOTOR1  // K1 clamps (re-read) instead of halving
    if (m_fLifeExp >= 3.0f)
    {
        if (m_fBirthRate > s_fFountainPacked)
            m_fBirthRate = s_fFountainPacked;
    }
    else if (m_fBirthRate > s_fFountainDefault)
        m_fBirthRate = s_fFountainDefault;
#else
    float fLimit = m_fLifeExp >= 3.0f ? s_fFountainPacked : s_fFountainDefault;
    if (m_fBirthRate > fLimit)
    {
        fHalf = m_fBirthRate * 0.5f;
        m_fBirthRate = Max(fLimit, fHalf);
    }
#endif
#endif  // K2_MOBILE_OPT
    float fBirthrate = m_fBirthRate;

    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode);
    // Every spawn inherits the owning gob's object scale; with no gob the particles stay unit-sized.
    float fObjectScale = m_pGob ? m_pGob->m_fObjectScale : 1.0f;

    chkParticleLife(fDeltaT);

    // The motion-blur mode trails a ghost position behind every live particle, catching it up at
    // the emitter's own rate capped by the frame time. Deliberately left uninitialised: both of
    // its uses sit behind their own motion-blur test.
    float fTrail K2_UNSET;
    if (m_nRenderMode == 9)
    {
        float t = m_fBlurLength < fDeltaT ? m_fBlurLength : fDeltaT;
        fTrail = t / m_fBlurLength;
    }

    // For every live particle, update the trail, then the orientation accumulator spun by the
    // emitter's own rate and wrapped into +-2pi. It is a plain carry, not a clamp, so the overshoot
    // is kept.
    for (int i = m_lParticles.m_count - 1; i >= 0; i--)
    {
        Particle* pParticle = m_lParticles.m_pData[i];
        if (m_nRenderMode == 9)
            pParticle->m_vTrailPosition = pParticle->m_vPosition * fTrail
                                  + pParticle->m_vTrailPosition * (1.0f - fTrail);
        pParticle->m_fRotation += m_fParticleRot * fDeltaT;
        if (pParticle->m_fRotation > 6.2831855f)
            pParticle->m_fRotation -= 6.2831855f;
        else if (pParticle->m_fRotation < -6.2831855f)
            pParticle->m_fRotation += 6.2831855f;
    }

    MoveFunc(fDeltaT);

    // A birth rate that has decayed below one emission per frame spawns nothing at all.
    if (static_cast<int>(fBirthrate) < 1)
        return;

    float fInvBirthrate = 1.0f / fBirthrate;

    if (pNode->m_nSpawnType != 0)
    {
        // The spread spawn type lays the spawns out along the emitter's own travel segment. The
        // direction is this frame's travel plus the accumulator from the previous frame, and the
        // tail writes the unconsumed remainder back, so the fraction carries over.
        Vector vDir = m_avVectors[6] - m_avVectors[7] + m_avVectors[4];
        float fLength = __builtin_sqrtf(vDir.x * vDir.x
                                      + vDir.y * vDir.y
                                      + vDir.z * vDir.z);
        // 1/|v|, and 0 for a degenerate direction.
        Vector vStep = vDir * (fLength > 0.0f ? 1.0f / fLength : 0.0f);
        int nCount = static_cast<int>(fBirthrate * fLength);

        for (int i = 0; i < nCount; i++)
        {
            Particle* pParticle;
            int nFree = m_lFreeParticles.m_count - 1;
            if (nFree >= 0)
            {
                pParticle = m_lFreeParticles.m_pData[nFree];
                m_lFreeParticles.m_count = nFree;
            }
            else if (pNode->m_szChunkName[0] != 0)
            {
                pParticle = new ChunkyParticle(this, fDeltaT);
            }
            else
            {
                pParticle = new Particle(this, fDeltaT);
            }
            pParticle->initialize(fDeltaT, fObjectScale);
            m_lParticles.Add(pParticle);
            chkRandom(m_lParticles.m_pData[m_lParticles.m_count - 1]);

            // The emitter only supplies a position when it has no spread of its own, the head of
            // the segment on the first spawn, then one birth-period's worth of travel per spawn.
            if (m_fXSize == 0.0f && m_fYSize == 0.0f)
            {
                if (m_lParticles.m_count < 2)
                {
                    m_lParticles.m_pData[0]->m_vPosition =
                        m_avVectors[6] - vStep * fLength;
                }
                else
                {
                    m_lParticles.m_pData[m_lParticles.m_count - 1]->m_vPosition =
                        m_avVectors[7] - m_avVectors[4]
                        + vStep * (static_cast<float>(i + 1) * fInvBirthrate);
                }
            }
        }
        m_avVectors[4] = vDir - vStep * (static_cast<float>(nCount) * fInvBirthrate);
        return;
    }

    // The accumulator spawn type.
    m_fBirthTimer += fDeltaT;
    if (!(m_fBirthTimer >= fInvBirthrate))
        return;

    // A random emitter jitters the birth rate, signed by a second draw and floored at zero.
    int nJitter = static_cast<int>(m_fRandomBirthRate);
    if (nJitter >= 1)
    {
        int nSign = rand_wincompatible();
        float fJitter = static_cast<float>(rand_wincompatible() % nJitter);
        if (nSign & 1)
            fBirthrate += fJitter;
        else
            fBirthrate -= fJitter;
        if (fBirthrate < 0.0f)
            fBirthrate = 0.0f;
    }

    int nMax = static_cast<int>(fBirthrate) + 1;
    if (static_cast<int>(fBirthrate * m_fBirthTimer) % nMax < 1)
    {
        m_fBirthTimer = 0.0f;
        return;
    }
    for (int i = 0; i < static_cast<int>(fBirthrate * m_fBirthTimer) % nMax; i++)
    {
        Particle* pParticle;
        int nFree = m_lFreeParticles.m_count - 1;
        if (nFree >= 0)
        {
            pParticle = m_lFreeParticles.m_pData[nFree];
            m_lFreeParticles.m_count = nFree;
        }
        else if (pNode->m_szChunkName[0] != 0)
        {
            pParticle = new ChunkyParticle(this, fDeltaT);
        }
        else
        {
            pParticle = new Particle(this, fDeltaT);
        }
        pParticle->initialize(fDeltaT, fObjectScale);
        m_lParticles.Add(pParticle);
        chkRandom(m_lParticles.m_pData[m_lParticles.m_count - 1]);
    }
#ifdef K2_FIX_ENGINE_BUGS
    // Only the spawned particles' share of the accumulated time is used up: dropping the
    // remainder thinned an emitter whose rate lies between the frame rate and twice it.
    {
        float fSpent = static_cast<float>(static_cast<int>(fBirthrate * m_fBirthTimer) % nMax) / fBirthrate;
        m_fBirthTimer = m_fBirthTimer > fSpent ? m_fBirthTimer - fSpent : 0.0f;
        if (m_fBirthTimer >= fInvBirthrate)
            m_fBirthTimer = 0.0f;
    }
#else
    m_fBirthTimer = 0.0f;
#endif
}


void ExplosionEmitter::Update(float fDeltaT)
{
    ConstrainBirthrateExplosion(this);
    float fBirthrate = m_fBirthRate;

    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode);
    float fObjectScale = m_pGob ? m_pGob->m_fObjectScale : 1.0f;

    chkParticleLife(fDeltaT);

    // The motion-blur mode trails a ghost position behind every live particle, catching it up at
    // the emitter's own rate capped by the frame time. Deliberately left uninitialised: both uses
    // sit behind their own motion-blur test.
    float fTrail K2_UNSET;
    if (m_nRenderMode == 9)
    {
        float t = m_fBlurLength < fDeltaT ? m_fBlurLength : fDeltaT;
        fTrail = t / m_fBlurLength;
    }

    // For every live particle, update the trail, then the orientation accumulator spun by the
    // emitter's own rate and wrapped into +-2pi. It is a plain carry, not a clamp.
    for (int i = m_lParticles.m_count - 1; i >= 0; i--)
    {
        if (m_nRenderMode == 9)
            m_lParticles.m_pData[i]->m_vTrailPosition =
                m_lParticles.m_pData[i]->m_vPosition * fTrail
                + m_lParticles.m_pData[i]->m_vTrailPosition * (1.0f - fTrail);
        m_lParticles.m_pData[i]->m_fRotation += m_fParticleRot * fDeltaT;
        if (m_lParticles.m_pData[i]->m_fRotation > 6.2831855f)
            m_lParticles.m_pData[i]->m_fRotation -= 6.2831855f;
        else if (m_lParticles.m_pData[i]->m_fRotation < -6.2831855f)
            m_lParticles.m_pData[i]->m_fRotation += 6.2831855f;
    }

    // For the burst count, a rate below one emission per frame spawns nothing at all.
    int nBirthrate = static_cast<int>(fBirthrate);

    // Pick the movement integrator; the Bezier variant only runs while the watched object is
    // alive.
    MdlNodeEmitter* pDispatch = static_cast<MdlNodeEmitter*>(m_pNode->AsMdlNodeEmitter());
    if (!(pDispatch->m_nEmitterFlags & 1))
    {
        moveParticleStandard(fDeltaT);
    }
    else if (m_spTarget.m_pObject)
    {
        if (pDispatch->m_nEmitterFlags & 2)
            moveParticleP2PBezier(fDeltaT);
        else
            moveParticleP2PGrav(fDeltaT);
    }

    // The burst fires once. The detonation callback sets the trigger, and the latch records that it
    // has fired. The wind source is registered at the emitter's world frame, sized by the node's
    // blast radius and length.
    if (nBirthrate != 0 && m_bDetonate && m_fDelayTimer < 1.0f)
    {
        if (pNode->m_fBlastRadius != 0.0f)
        {
            Vector vPos;
            Quaternion qOrient;
            GetWorld(vPos, qOrient);
            currentscene->m_pWindManager->AddPointSourceWind(
                vPos, pNode->m_fBlastRadius, pNode->m_fBlastLength, 1.0f);
        }

        // The spawn loop is inside the one-shot gate; outside it, an Explosion emitter re-bursts
        // every frame.
        for (int i = 0; i < nBirthrate; i++)
        {
            // The free list is used as a stack, its count the top index.
            Particle* pParticle;
            int nFree = m_lFreeParticles.m_count - 1;
            if (nFree >= 0)
            {
                pParticle = m_lFreeParticles.m_pData[nFree];
                m_lFreeParticles.m_count = nFree;
                pParticle->initialize(fDeltaT, 1.0f);
            }
            else
            {
                if (pNode->m_szChunkName[0] != 0)
                    pParticle = new ChunkyParticle(this, fDeltaT);
                else
                    pParticle = new Particle(this, fDeltaT);
                pParticle->initialize(fDeltaT, fObjectScale);
            }
            m_lParticles.Add(pParticle);
            chkRandom(m_lParticles.m_pData[m_lParticles.m_count - 1]);
        }
    }

    m_fDelayTimer = m_bDetonate ? 1.0f : 0.0f;
    m_bDetonate = false;
}

extern float emitterfadeheight;

extern int renderemitterspheres;

extern int g_nAliasingMode;

extern unsigned char g_adjustWake;

#include "aurglcore.h"

void PartEmitter::RenderFunc()
{
    if (m_pGob != 0 && m_pGob->m_pModel != 0 &&
        (m_pGob->m_bDistortion || (m_pGob->m_pModel->m_nFlags & 2) != 0))
    {
#if KOTOR1  // K1 only stamps stencil bit 1 (no distortion buffer)
        ASLgl::glEnable(enableTranslationTable[10]);
        ASLgl::glColorMask(0, 0, 0, 0);
        ASLgl::glStencilOp(stencilOpTranslationTable[0], stencilOpTranslationTable[0],
                           stencilOpTranslationTable[2]);
        ASLgl::glStencilMask(1);
        ASLgl::glStencilFunc(cmpTranslationTable[7], 1, 0xff);
#else
        SetDistortBuffer(true);
        if (!g_useDistortionOutput)
            ASLgl::glDepthMask(0);
        g_useDistortionOutput = true;
        GLRender::pfuncEnablePixelShaders(GLRender::kPSDefault, m_pParticleMaterial->Textures(0).m_pObject, 0, 0, 0);
#endif
    }
    else
    {
        ASLgl::glColorMask(1, 1, 1, 1);
        ASLgl::glStencilMask(0);
        ASLgl::glDisable(enableTranslationTable[10]);
    }

    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode->AsMdlNodeEmitter());

    // A point-to-point emitter with no live watched target has nothing to draw at all.
    if (pNode->m_flags.m_bPointToPoint && m_spTarget.m_pObject == 0)
    {
        if (m_pGob != 0 && m_pGob->m_pModel != 0 &&
            (m_pGob->m_bDistortion || (m_pGob->m_pModel->m_nFlags & 2) != 0))
        {
#if KOTOR1  // K1: the early-outs restore the stencil state like the normal exit
            ASLgl::glColorMask(1, 1, 1, 1);
            ASLgl::glStencilMask(0);
            ASLgl::glDisable(enableTranslationTable[10]);
#else
            SetDistortBuffer(true);
            if (!g_useDistortionOutput)
                ASLgl::glDepthMask(0);
            g_useDistortionOutput = true;
            GLRender::pfuncEnablePixelShaders(GLRender::kPSDefault, m_pParticleMaterial->Textures(0).m_pObject, 0, 0, 0);
#endif
        }
        return;
    }

    // A chunky emitter (its node carries a chunk resref) only syncs its particles' aurora objects.
    if (pNode->m_szChunkName[0] != 0)
    {
        RenderNothing();
        if (m_pGob != 0 && m_pGob->m_pModel != 0 &&
            (m_pGob->m_bDistortion || (m_pGob->m_pModel->m_nFlags & 2) != 0))
        {
#if KOTOR1  // K1: the early-outs restore the stencil state like the normal exit
            ASLgl::glColorMask(1, 1, 1, 1);
            ASLgl::glStencilMask(0);
            ASLgl::glDisable(enableTranslationTable[10]);
#else
            SetDistortBuffer(true);
            if (!g_useDistortionOutput)
                ASLgl::glDepthMask(0);
            g_useDistortionOutput = true;
            GLRender::pfuncEnablePixelShaders(GLRender::kPSDefault, m_pParticleMaterial->Textures(0).m_pObject, 0, 0, 0);
#endif
        }
        return;
    }

#if !KOTOR1  // K1: no force sight
    if (g_ForceSight != 0)
    {
        GLRender::pfuncEnablePixelShaders(GLRender::kPSDesat, 0, 0, 0, 0);
        const float afEnvParams[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, afEnvParams);
    }
#endif

    if (m_nRenderMode == 4)
    {
        LightningEmitter* pLightning = AsLightningEmitter();
        if (pLightning == 0)
        {
            RenderLinked();
        }
        else
        {
            // The bolt's tangent and handle plot.
            if (auremitterrendertangents)
            {
                ASLgl::glDisable(GL_LIGHTING);
                ASLgl::glDisable(GL_TEXTURE_2D);
                ASLgl::glColor3f(0.0f, 1.0f, 0.0f);
                ASLgl::glLineWidth(2.5f);

                float fFrac = 0.0f;
                if (pLightning->m_fControlPtDelay != 0.0f)
                    fFrac = pLightning->m_fControlPtTimer / pLightning->m_fControlPtDelay;

                ASLgl::glBegin(GL_LINES);

                float fInv = 1.0f - fFrac;

                ASLgl::glVertex3f(pLightning->m_lPrevPoints.m_pData[0].x * fInv +
                                      pLightning->m_lNextPoints.m_pData[0].x * fFrac,
                                  pLightning->m_lPrevPoints.m_pData[0].y * fInv +
                                      pLightning->m_lNextPoints.m_pData[0].y * fFrac,
                                  pLightning->m_lPrevPoints.m_pData[0].z * fInv +
                                      pLightning->m_lNextPoints.m_pData[0].z * fFrac);
                ASLgl::glVertex3f(pLightning->m_lPrevPoints.m_pData[0].x * fInv +
                                      pLightning->m_lNextPoints.m_pData[0].x * fFrac +
                                      pLightning->m_lPrevTangents.m_pData[0].x * fInv +
                                      pLightning->m_lNextTangents.m_pData[0].x * fFrac,
                                  pLightning->m_lPrevPoints.m_pData[0].y * fInv +
                                      pLightning->m_lNextPoints.m_pData[0].y * fFrac +
                                      pLightning->m_lPrevTangents.m_pData[0].y * fInv +
                                      pLightning->m_lNextTangents.m_pData[0].y * fFrac,
                                  pLightning->m_lPrevPoints.m_pData[0].z * fInv +
                                      pLightning->m_lNextPoints.m_pData[0].z * fFrac +
                                      pLightning->m_lPrevTangents.m_pData[0].z * fInv +
                                      pLightning->m_lNextTangents.m_pData[0].z * fFrac);
                for (int i = 1; i < pLightning->m_lPrevPoints.m_count - 1; i++)
                {
                    ASLgl::glVertex3f(pLightning->m_lPrevPoints.m_pData[i].x * fInv +
                                          pLightning->m_lNextPoints.m_pData[i].x * fFrac -
                                          (pLightning->m_lPrevTangents.m_pData[i].x * fInv +
                                           pLightning->m_lNextTangents.m_pData[i].x * fFrac),
                                      pLightning->m_lPrevPoints.m_pData[i].y * fInv +
                                          pLightning->m_lNextPoints.m_pData[i].y * fFrac -
                                          (pLightning->m_lPrevTangents.m_pData[i].y * fInv +
                                           pLightning->m_lNextTangents.m_pData[i].y * fFrac),
                                      pLightning->m_lPrevPoints.m_pData[i].z * fInv +
                                          pLightning->m_lNextPoints.m_pData[i].z * fFrac -
                                          (pLightning->m_lPrevTangents.m_pData[i].z * fInv +
                                           pLightning->m_lNextTangents.m_pData[i].z * fFrac));
                    ASLgl::glVertex3f(pLightning->m_lPrevPoints.m_pData[i].x * fInv +
                                          pLightning->m_lNextPoints.m_pData[i].x * fFrac +
                                          pLightning->m_lPrevTangents.m_pData[i].x * fInv +
                                          pLightning->m_lNextTangents.m_pData[i].x * fFrac,
                                      pLightning->m_lPrevPoints.m_pData[i].y * fInv +
                                          pLightning->m_lNextPoints.m_pData[i].y * fFrac +
                                          pLightning->m_lPrevTangents.m_pData[i].y * fInv +
                                          pLightning->m_lNextTangents.m_pData[i].y * fFrac,
                                      pLightning->m_lPrevPoints.m_pData[i].z * fInv +
                                          pLightning->m_lNextPoints.m_pData[i].z * fFrac +
                                          pLightning->m_lPrevTangents.m_pData[i].z * fInv +
                                          pLightning->m_lNextTangents.m_pData[i].z * fFrac);
                }

                int nLast = pLightning->m_lPrevPoints.m_count - 1;

                ASLgl::glVertex3f(pLightning->m_lPrevPoints.m_pData[nLast].x * fInv +
                                      pLightning->m_lNextPoints.m_pData[nLast].x * fFrac -
                                      (pLightning->m_lPrevTangents.m_pData[nLast].x * fInv +
                                       pLightning->m_lNextTangents.m_pData[nLast].x * fFrac),
                                  pLightning->m_lPrevPoints.m_pData[nLast].y * fInv +
                                      pLightning->m_lNextPoints.m_pData[nLast].y * fFrac -
                                      (pLightning->m_lPrevTangents.m_pData[nLast].y * fInv +
                                       pLightning->m_lNextTangents.m_pData[nLast].y * fFrac),
                                  pLightning->m_lPrevPoints.m_pData[nLast].z * fInv +
                                      pLightning->m_lNextPoints.m_pData[nLast].z * fFrac -
                                      (pLightning->m_lPrevTangents.m_pData[nLast].z * fInv +
                                       pLightning->m_lNextTangents.m_pData[nLast].z * fFrac));
                ASLgl::glVertex3f(pLightning->m_lPrevPoints.m_pData[nLast].x * fInv +
                                      pLightning->m_lNextPoints.m_pData[nLast].x * fFrac,
                                  pLightning->m_lPrevPoints.m_pData[nLast].y * fInv +
                                      pLightning->m_lNextPoints.m_pData[nLast].y * fFrac,
                                  pLightning->m_lPrevPoints.m_pData[nLast].z * fInv +
                                      pLightning->m_lNextPoints.m_pData[nLast].z * fFrac);

                ASLgl::glEnd();
                ASLgl::glEnable(GL_TEXTURE_2D);
                ASLgl::glEnable(GL_LIGHTING);
            }

            // The spline draws the two control sets, then the sampled curve.
            if (auremitterrenderbezier)
            {
                float fFrac = 0.0f;
                if (pLightning->m_fControlPtDelay != 0.0f)
                    fFrac = pLightning->m_fControlPtTimer / pLightning->m_fControlPtDelay;
                ASLgl::glDisable(GL_LIGHTING);
                ASLgl::glDisable(GL_TEXTURE_2D);
                ASLgl::glColor3f(1.0f, 0.0f, 0.0f);
                ASLgl::glPointSize(10.0f);
                ASLgl::glBegin(GL_POINTS);
                float fInv = 1.0f - fFrac;
                for (int i = 0; i < pLightning->m_lPrevPoints.m_count; i++)
                {
                    ASLgl::glVertex3f(pLightning->m_lPrevPoints.m_pData[i].x * fInv +
                                          pLightning->m_lNextPoints.m_pData[i].x * fFrac,
                                      pLightning->m_lPrevPoints.m_pData[i].y * fInv +
                                          pLightning->m_lNextPoints.m_pData[i].y * fFrac,
                                      pLightning->m_lPrevPoints.m_pData[i].z * fInv +
                                          pLightning->m_lNextPoints.m_pData[i].z * fFrac);
                }
                ASLgl::glEnd();

                ASLgl::glColor3f(0.0f, 1.0f, 1.0f);
                ASLgl::glLineWidth(1.0f);
                ASLgl::glBegin(GL_LINE_STRIP);
                ASLgl::glVertex3f(pLightning->m_lPrevPoints.m_pData[0].x * fInv +
                                      pLightning->m_lNextPoints.m_pData[0].x * fFrac,
                                  pLightning->m_lPrevPoints.m_pData[0].y * fInv +
                                      pLightning->m_lNextPoints.m_pData[0].y * fFrac,
                                  pLightning->m_lPrevPoints.m_pData[0].z * fInv +
                                      pLightning->m_lNextPoints.m_pData[0].z * fFrac);

                // The result is discarded.
                pLightning->m_spTarget.m_pObject->GetPosition();

                for (int i = 1; i < pLightning->m_lPrevPoints.m_count; i++)
                {
                    for (float fT = 0.0f; fT <= 1.0f; fT += 0.05f)
                    {
                        if (fT > 1.0f)
                            fT = 1.0f;

                        float fU = 1.0f - fT;
                        float fW0 = fU * fU * fU;
                        float fW1 = 3.0f * fT * fU * fU;
                        float fW2 = 3.0f * fT * fT * fU;
                        float fW3 = fT * fT * fT;

                        const Vector& vA0 = pLightning->m_lPrevPoints.m_pData[i - 1];
                        const Vector& vB0 = pLightning->m_lNextPoints.m_pData[i - 1];
                        const Vector& vC0 = pLightning->m_lPrevTangents.m_pData[i - 1];
                        const Vector& vD0 = pLightning->m_lNextTangents.m_pData[i - 1];
                        const Vector& vA1 = pLightning->m_lPrevPoints.m_pData[i];
                        const Vector& vB1 = pLightning->m_lNextPoints.m_pData[i];
                        const Vector& vC1 = pLightning->m_lPrevTangents.m_pData[i];
                        const Vector& vD1 = pLightning->m_lNextTangents.m_pData[i];

                        float fP0x = vA0.x * fInv + vB0.x * fFrac;
                        float fP0y = vA0.y * fInv + vB0.y * fFrac;
                        float fP0z = vA0.z * fInv + vB0.z * fFrac;
                        float fP1x = vA1.x * fInv + vB1.x * fFrac;
                        float fP1y = vA1.y * fInv + vB1.y * fFrac;
                        float fP1z = vA1.z * fInv + vB1.z * fFrac;
                        float fH1x = vC1.x * fInv + vD1.x * fFrac;
                        float fH1y = vC1.y * fInv + vD1.y * fFrac;
                        float fH1z = vC1.z * fInv + vD1.z * fFrac;

                        ASLgl::glVertex3f(fP0x * fW0 + (fP0x + vC0.x * fInv + vD0.x * fFrac) * fW1 +
                                              (fP1x - fH1x) * fW2 + fP1x * fW3,
                                          fP0y * fW0 + (fP0y + vC0.y * fInv + vD0.y * fFrac) * fW1 +
                                              (fP1y - fH1y) * fW2 + fP1y * fW3,
                                          fP0z * fW0 + (fP0z + vC0.z * fInv + vD0.z * fFrac) * fW1 +
                                              (fP1z - fH1z) * fW2 + fP1z * fW3);
                    }
                }

                ASLgl::glVertex3f(
                    pLightning->m_lPrevPoints.m_pData[pLightning->m_lPrevPoints.m_count - 1].x * fInv +
                        pLightning->m_lNextPoints.m_pData[pLightning->m_lNextPoints.m_count - 1].x * fFrac,
                    pLightning->m_lPrevPoints.m_pData[pLightning->m_lPrevPoints.m_count - 1].y * fInv +
                        pLightning->m_lNextPoints.m_pData[pLightning->m_lNextPoints.m_count - 1].y * fFrac,
                    pLightning->m_lPrevPoints.m_pData[pLightning->m_lPrevPoints.m_count - 1].z * fInv +
                        pLightning->m_lNextPoints.m_pData[pLightning->m_lNextPoints.m_count - 1].z * fFrac);
                ASLgl::glEnd();
                ASLgl::glEnable(GL_TEXTURE_2D);
                ASLgl::glEnable(GL_LIGHTING);
            }

            // The same two passes, replayed per child branch emitter.
            if (auremitterrenderbranchbezier && pLightning->m_nActiveBranches > 0)
            {
                float fFrac = 0.0f;
                if (pLightning->m_fControlPtDelay != 0.0f)
                    fFrac = pLightning->m_fControlPtTimer / pLightning->m_fControlPtDelay;
                ASLgl::glDisable(GL_LIGHTING);
                ASLgl::glDisable(GL_TEXTURE_2D);

                for (int i = 0; i < pLightning->m_nActiveBranches; i++)
                {
                    float fInv = 1.0f - fFrac;

                    ASLgl::glColor3f(1.0f, 0.0f, 0.0f);
                    ASLgl::glPointSize(10.0f);
                    ASLgl::glBegin(GL_POINTS);
                    for (int j = 0; j < pLightning->m_lBranches.m_pData[i]->m_lPrevPoints.m_count; j++)
                    {
                        ASLgl::glVertex3f(
                            pLightning->m_lBranches.m_pData[i]->m_lPrevPoints.m_pData[j].x * fInv +
                                pLightning->m_lBranches.m_pData[i]->m_lNextPoints.m_pData[j].x * fFrac,
                            pLightning->m_lBranches.m_pData[i]->m_lPrevPoints.m_pData[j].y * fInv +
                                pLightning->m_lBranches.m_pData[i]->m_lNextPoints.m_pData[j].y * fFrac,
                            pLightning->m_lBranches.m_pData[i]->m_lPrevPoints.m_pData[j].z * fInv +
                                pLightning->m_lBranches.m_pData[i]->m_lNextPoints.m_pData[j].z * fFrac);
                    }
                    ASLgl::glEnd();

                    ASLgl::glColor3f(0.0f, 1.0f, 1.0f);
                    ASLgl::glLineWidth(1.0f);
                    ASLgl::glBegin(GL_LINE_STRIP);
                    ASLgl::glVertex3f(
                        pLightning->m_lBranches.m_pData[i]->m_lPrevPoints.m_pData[0].x * fInv +
                            pLightning->m_lBranches.m_pData[i]->m_lNextPoints.m_pData[0].x * fFrac,
                        pLightning->m_lBranches.m_pData[i]->m_lPrevPoints.m_pData[0].y * fInv +
                            pLightning->m_lBranches.m_pData[i]->m_lNextPoints.m_pData[0].y * fFrac,
                        pLightning->m_lBranches.m_pData[i]->m_lPrevPoints.m_pData[0].z * fInv +
                            pLightning->m_lBranches.m_pData[i]->m_lNextPoints.m_pData[0].z * fFrac);

                    // Discarded result, as in the parent's arm.
                    pLightning->m_lBranches.m_pData[i]->m_spTarget.m_pObject->GetPosition();

                    for (int j = 1; j < pLightning->m_lBranches.m_pData[i]->m_lPrevPoints.m_count; j++)
                    {
                        for (float fT = 0.0f; fT <= 1.0f; fT += 0.05f)
                        {
                            if (fT > 1.0f)
                                fT = 1.0f;

                            float fU = 1.0f - fT;
                            float fW0 = fU * fU * fU;
                            float fW1 = 3.0f * fT * fU * fU;
                            float fW2 = 3.0f * fT * fT * fU;
                            float fW3 = fT * fT * fT;

                            const Vector& vA0 = pLightning->m_lBranches.m_pData[i]->m_lPrevPoints.m_pData[j - 1];
                            const Vector& vB0 = pLightning->m_lBranches.m_pData[i]->m_lNextPoints.m_pData[j - 1];
                            const Vector& vC0 = pLightning->m_lBranches.m_pData[i]->m_lPrevTangents.m_pData[j - 1];
                            const Vector& vD0 = pLightning->m_lBranches.m_pData[i]->m_lNextTangents.m_pData[j - 1];
                            const Vector& vA1 = pLightning->m_lBranches.m_pData[i]->m_lPrevPoints.m_pData[j];
                            const Vector& vB1 = pLightning->m_lBranches.m_pData[i]->m_lNextPoints.m_pData[j];
                            const Vector& vC1 = pLightning->m_lBranches.m_pData[i]->m_lPrevTangents.m_pData[j];
                            const Vector& vD1 = pLightning->m_lBranches.m_pData[i]->m_lNextTangents.m_pData[j];

                            float fP0x = vA0.x * fInv + vB0.x * fFrac;
                            float fP0y = vA0.y * fInv + vB0.y * fFrac;
                            float fP0z = vA0.z * fInv + vB0.z * fFrac;
                            float fP1x = vA1.x * fInv + vB1.x * fFrac;
                            float fP1y = vA1.y * fInv + vB1.y * fFrac;
                            float fP1z = vA1.z * fInv + vB1.z * fFrac;
                            float fH1x = vC1.x * fInv + vD1.x * fFrac;
                            float fH1y = vC1.y * fInv + vD1.y * fFrac;
                            float fH1z = vC1.z * fInv + vD1.z * fFrac;

                            ASLgl::glVertex3f(fP0x * fW0 + (fP0x + vC0.x * fInv + vD0.x * fFrac) * fW1 +
                                                  (fP1x - fH1x) * fW2 + fP1x * fW3,
                                              fP0y * fW0 + (fP0y + vC0.y * fInv + vD0.y * fFrac) * fW1 +
                                                  (fP1y - fH1y) * fW2 + fP1y * fW3,
                                              fP0z * fW0 + (fP0z + vC0.z * fInv + vD0.z * fFrac) * fW1 +
                                                  (fP1z - fH1z) * fW2 + fP1z * fW3);
                        }
                    }

                    ASLgl::glVertex3f(
                        pLightning->m_lBranches.m_pData[i]->m_lPrevPoints
                                .m_pData[pLightning->m_lBranches.m_pData[i]->m_lPrevPoints.m_count - 1].x * fInv +
                            pLightning->m_lBranches.m_pData[i]->m_lNextPoints
                                .m_pData[pLightning->m_lBranches.m_pData[i]->m_lPrevPoints.m_count - 1].x * fFrac,
                        pLightning->m_lBranches.m_pData[i]->m_lPrevPoints
                                .m_pData[pLightning->m_lBranches.m_pData[i]->m_lPrevPoints.m_count - 1].y * fInv +
                            pLightning->m_lBranches.m_pData[i]->m_lNextPoints
                                .m_pData[pLightning->m_lBranches.m_pData[i]->m_lPrevPoints.m_count - 1].y * fFrac,
                        pLightning->m_lBranches.m_pData[i]->m_lPrevPoints
                                .m_pData[pLightning->m_lBranches.m_pData[i]->m_lPrevPoints.m_count - 1].z * fInv +
                            pLightning->m_lBranches.m_pData[i]->m_lNextPoints
                                .m_pData[pLightning->m_lBranches.m_pData[i]->m_lPrevPoints.m_count - 1].z * fFrac);
                    ASLgl::glEnd();
                }

                ASLgl::glEnable(GL_TEXTURE_2D);
                ASLgl::glEnable(GL_LIGHTING);
            }

            if (auremitterrenderlightning)
            {
                for (int i = 0; i < pLightning->m_nActiveBranches; i++)
                    pLightning->m_lBranches.m_pData[i]->RenderLinked();
            }
            if (auremitterrenderlightning)
                RenderLinked();
        }
    }
    else if (m_nRenderMode == 9)
    {
        RenderMotionBlurred();
    }
    else
    {
        RenderStandard();
    }

#if !KOTOR1
    if (g_ForceSight != 0)
        GLRender::pfuncEnablePixelShaders(GLRender::kPSDesatOff, 0, 0, 0, 0);
#endif

    if (m_pGob != 0 && m_pGob->m_pModel != 0 &&
        (m_pGob->m_bDistortion || (m_pGob->m_pModel->m_nFlags & 2) != 0))
    {
#if KOTOR1
        ASLgl::glColorMask(1, 1, 1, 1);
        ASLgl::glStencilMask(0);
        ASLgl::glDisable(enableTranslationTable[10]);
#else
        SetDistortBuffer(false);
        g_useDistortionOutput = false;
        GLRender::pfuncEnablePixelShaders(GLRender::kPSFixedFunction, 0, 0, 0, 0);
        if (!g_useDistortionOutput)
            ASLgl::glDepthMask(1);
#endif
    }
}

int particlecount;
extern int countfaces;

static RenderVertexVTC* ResizeEmitterPool(RenderVertexVTC* pPool, unsigned int nOldSize,
                                          unsigned int nNewSize)
{
    RenderVertexVTC* pNew = new RenderVertexVTC[nNewSize];
    memcpy(pNew, pPool, nOldSize * sizeof(RenderVertexVTC));
#ifdef K2_HOST_BUILD
    // K1's RenderObjectsVTCQuadUP leaves the colour and texcoord arrays enabled on the pool, and a
    // driver that reads every enabled array (NVIDIA) faults on a freed one: the old pool stays.
    (void)pPool;
#else
    delete[] pPool;
#endif
    return pNew;
}

void PartEmitter::RenderLinked()
{
    static RenderVertexVTC* particlePool = new RenderVertexVTC[1000];
    static unsigned int poolSize = 1000;

    if (m_pGob != NULL && m_pGob->m_pTileData != NULL && m_pGob->m_pTileData->m_fFade < 1.0f)
        return;
#if KOTOR1  // K1 composes each particle's sprite-frame spin with a copy of the camera orientation
    Quaternion qCamera;
    Quaternion qFrame;
#endif
    if (m_lParticles.m_count == 0)
        return;

    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode);

    // The camera orientation, copied once here after the empty early-out. Only its matrix rows
    // are needed later, after the GL bracket.
    Quaternion qCam = CurrentCamera->m_qOrientation;

    LightningEmitter* pLightning = AsLightningEmitter();

    // The per-particle ribbon positions are the particle's own position, plus the bolt's fragment
    // vector at the same index when this emitter is a lightning bolt.
    List<Vector> lPositions;
    lPositions.SetSize(m_lParticles.m_count);
    for (int64_t i = 0; i < m_lParticles.m_count; i++)
    {
        lPositions.m_pData[i] = m_lParticles.m_pData[i]->m_vPosition;
        if (pLightning != NULL)
            lPositions.m_pData[i] = pLightning->m_lParticleOffsets.m_pData[i]
                                 + lPositions.m_pData[i];
    }

    if (renderemitterspheres)
        RenderBoundingSphere();

    // The shared pool is grown to `count * 8` and kept. Only the first `poolSize` vertices hold
    // valid data, so the copy stops at the old size.
    if (poolSize < static_cast<unsigned int>(m_lParticles.m_count * 4))
    {
        unsigned int nOldSize = poolSize;
        poolSize = static_cast<unsigned int>(m_lParticles.m_count * 8);
        particlePool = ResizeEmitterPool(particlePool, nOldSize, poolSize);
    }

    // A Gob colour-shift scale, applied to both ribbon widths when it is not exactly 1.0.
    float fScale = 1.0f;
    if (m_pGob != NULL && m_pGob->m_fObjectScale != 1.0f)
        fScale = m_pGob->m_fObjectScale;

    float fFade = m_fFPS > 0.0f ? 1.0f / m_fFPS : 0.0f;

    m_pParticleMaterial->BindTexture0();
    m_pParticleMaterial->SetBlendingMode(0);
#if KOTOR1
    qCamera = CurrentCamera->m_qOrientation;
#endif

    // The per-particle colour/frame step. A "position" emitter (mode 6) spreads the whole particle
    // set across one frame; every other kind advances by its own rate, or by the engine's 0.01
    // quantum when that rate is not positive.
    float fFrameStep;
    if (m_nGradientMode == 6)
        fFrameStep = 1.0f / static_cast<float>(m_lParticles.m_count);
    else
        fFrameStep = m_fLifeExp > 0.0f ? 1.0f / m_fLifeExp : 0.01f;

    // The texcoords walk (u,v) across the node's sprite-sheet grid.
    int nXGrid = pNode->m_nXGrid != 0 ? pNode->m_nXGrid : 1;
    int nYGrid = pNode->m_nYGrid != 0 ? pNode->m_nYGrid : 1;
    float fInvXGrid = 1.0f / static_cast<float>(nXGrid);
    float fInvYGrid = 1.0f / static_cast<float>(nYGrid);

    if (!g_useDistortionOutput)
        ASLgl::glDepthMask(0);
    ASLgl::glDisable(enableTranslationTable[kCapDepthTest]);
    ASLgl::glEnable(enableTranslationTable[kCapBlend]);
    ASLgl::glDisable(enableTranslationTable[kCapAlphaTest]);

    // The emitter's own texture-blend mode, picked by Initialize from the node's blend name. The
    // additive arm saves and restores the fog state, not the blend cap the first two arms touch:
    // additive particle drawing turns fog off and puts it back.
    unsigned char bBlendWasEnabled = 0;
    if (m_nBlendMode == 1)
    {
        ASLgl::glDisable(enableTranslationTable[kCapBlend]);
        ASLgl::glEnable(enableTranslationTable[kCapAlphaTest]);
    }
    else if (m_nBlendMode == 0)
    {
        ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
    }
    else
    {
        ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[2]);
        bBlendWasEnabled = ASLgl::glIsEnabled(enableTranslationTable[kCapFog]);
        ASLgl::glDisable(enableTranslationTable[kCapFog]);
    }

    updateParticleFrame(fFade);
    particlecount += m_lParticles.m_count;

    float fTileAlpha = GetTileAlpha();
    float fGobAlpha = GetGobAlpha();

    // The camera orientation's three rows as world-space axes; row 2 is the ribbon's frame axis.
    Matrix matCamera = qCam.getmatrix();
    Vector vAxisX(matCamera.m_f[0], matCamera.m_f[1], matCamera.m_f[2]);
    Vector vAxisY(matCamera.m_f[3], matCamera.m_f[4], matCamera.m_f[5]);
    Vector vAxisZ(matCamera.m_f[6], matCamera.m_f[7], matCamera.m_f[8]);

    int nVert = 0;
    for (int i = 0; i < m_lParticles.m_count; i++)
    {
        Particle* pParticle = m_lParticles.m_pData[i];

        // The particle's colour, its secondary alpha value and its two ribbon widths.
        Vector vColor;
        float fAlpha;
        float fWidthU;
        float fWidthV;
        if (!(m_fLifeExp >= 0.0f))
        {
            // A "position" emitter: one flat colour, no fading at all.
            vColor = m_avVectors[9];
            fAlpha = m_fAlphaStart;
            fWidthU = m_fSizeStart * 0.5f;
            fWidthV = (m_fSizeStartY != 0.0f || m_fSizeMidY != 0.0f || m_fSizeEndY != 0.0f)
                    ? m_fSizeStartY * 0.5f : fWidthU;
        }
        else
        {
            float fT = (m_nGradientMode == 6) ? static_cast<float>(i) * fFrameStep
                                          : pParticle->m_fAge * fFrameStep;
            if (m_fPercentStart == 255.0f)
            {
                vColor = m_avVectors[9] * (1.0f - fT) + m_avVectors[8] * fT;
                fAlpha = m_fAlphaStart * (1.0f - fT) + m_fAlphaEnd * fT;
                fWidthU = (m_fSizeStart * (1.0f - fT) + m_fSizeEnd * fT) * 0.5f;
                fWidthV = (m_fSizeStartY != 0.0f || m_fSizeEndY != 0.0f)
                        ? (m_fSizeStartY * (1.0f - fT) + m_fSizeEndY * fT) * 0.5f : fWidthU;
            }
            else if (fT >= m_fPercentEnd)
            {
                vColor = m_avVectors[8];
                fAlpha = m_fAlphaEnd;
                fWidthU = m_fSizeEnd;
                fWidthV = (m_fSizeEndY != 0.0f) ? m_fSizeEndY : fWidthU;
            }
            else if (fT >= m_fPercentMid)
            {
                float fF = (fT - m_fPercentMid) / (m_fPercentEnd - m_fPercentMid);
                vColor = m_avVectors[0] * (1.0f - fF) + m_avVectors[8] * fF;
                fAlpha = m_fAlphaMid * (1.0f - fF) + m_fAlphaEnd * fF;
                fWidthU = (m_fSizeMid * (1.0f - fF) + m_fSizeEnd * fF) * 0.5f;
                fWidthV = (m_fSizeMidY != 0.0f || m_fSizeEndY != 0.0f)
                        ? (m_fSizeMidY * (1.0f - fF) + m_fSizeEndY * fF) * 0.5f : fWidthU;
            }
            else if (fT >= m_fPercentStart)
            {
                float fF = (fT - m_fPercentStart) / (m_fPercentMid - m_fPercentStart);
                vColor = m_avVectors[9] * (1.0f - fF) + m_avVectors[0] * fF;
                fAlpha = m_fAlphaStart * (1.0f - fF) + m_fAlphaMid * fF;
                fWidthU = (m_fSizeStart * (1.0f - fF) + m_fSizeMid * fF) * 0.5f;
                fWidthV = (m_fSizeStartY != 0.0f || m_fSizeMidY != 0.0f)
                        ? (m_fSizeStartY * (1.0f - fF) + m_fSizeMidY * fF) * 0.5f : fWidthU;
            }
            else
            {
                vColor = m_avVectors[9];
                fAlpha = m_fAlphaStart;
                fWidthU = m_fSizeStart;
                fWidthV = (m_fSizeStartY != 0.0f) ? m_fSizeStartY : fWidthU;
            }
        }
        // Clamp the colour and alpha before the segment block, which indexes the next particle.
        vColor.x = vColor.x < 0.0f ? 0.0f : (vColor.x >= 1.0f ? 1.0f : vColor.x);
        vColor.y = vColor.y < 0.0f ? 0.0f : (vColor.y >= 1.0f ? 1.0f : vColor.y);
        vColor.z = vColor.z < 0.0f ? 0.0f : (vColor.z >= 1.0f ? 1.0f : vColor.z);
        fAlpha = fAlpha < 0.0f ? 0.0f : (fAlpha >= 1.0f ? 1.0f : fAlpha);

        // The per-particle sprite-frame orientation is the camera's forward axis rotated by the
        // particle's own spin angle. Its result is unused; the constructor call is the statement.
#if KOTOR1  // the product is unused too, but K1's operator* is out of line, so the call stays
        qFrame = Quaternion(vAxisZ, m_lParticles.m_pData[i]->m_fRotation);
        qFrame = qFrame * qCamera;
#else
        Quaternion qFrame(vAxisZ, m_lParticles.m_pData[i]->m_fRotation);
#endif

        if (fScale != 1.0f)
        {
            fWidthU *= fScale;
            fWidthV *= fScale;
        }

        // The segment block stays inside this guard, so the last particle's segment is never
        // computed, and the two seam vertices are shared with the neighbouring segments.
        if (i < m_lParticles.m_count - 1)
        {
            // The segment's screen-space direction.
            const Vector& vP0 = lPositions.m_pData[i];
            Vector vDir = lPositions.m_pData[i + 1] - vP0;

            float fScreenX = vDir.x * vAxisX.x + vDir.y * vAxisX.y + vDir.z * vAxisX.z;
            float fScreenY = vDir.x * vAxisY.x + vDir.y * vAxisY.y + vDir.z * vAxisY.z;
            float fLen = __builtin_sqrtf(fScreenX * fScreenX + fScreenY * fScreenY);

            int nFrame = static_cast<int>(fInvXGrid * static_cast<float>(pParticle->m_nFrame));
            int nRemainder = pParticle->m_nFrame - nXGrid * nFrame;

            // Rotate back out to world space, the direction and its perpendicular in the screen
            // plane. A degenerate segment leaves the two camera rows untouched.
            Vector vAlong = vAxisY;
            Vector vAcross = vAxisX;
            if (fLen > 0.00001)
            {
                fScreenX /= fLen;
                fScreenY /= fLen;
                vAlong = vAxisX * fScreenX + vAxisY * fScreenY;
                vAcross = vAxisX * fScreenY - vAxisY * fScreenX;
            }

            Vector vOffAcross = vAcross * (2.0f * fWidthU);
            Vector vOffAlong = vAlong * (fWidthV * 0.5f);

            unsigned int nColor = static_cast<unsigned int>(vColor.x * 255.0f)
                                | (static_cast<unsigned int>(vColor.y * 255.0f) << 8)
                                | (static_cast<unsigned int>(vColor.z * 255.0f) << 16)
                                | (static_cast<unsigned int>((fGobAlpha * (fTileAlpha * fAlpha)) * 255.0f) << 24);

            // In the frame's texcoords, u walks the sprite grid's columns within one frame and v
            // picks the frame row (bottom-up).
            float fU0 = static_cast<float>(nRemainder + 1) * fInvXGrid;
            float fU1 = static_cast<float>(nRemainder) * fInvXGrid;
            float fV0 = 1.0f - static_cast<float>(nFrame + 1) * fInvYGrid;
            float fV1 = 1.0f - static_cast<float>(nFrame) * fInvYGrid;
            particlePool[nVert + 0].color = nColor;
            particlePool[nVert + 1].color = nColor;
            particlePool[nVert + 2].color = nColor;
            particlePool[nVert + 3].color = nColor;

            // Vertex 0: the previous segment's own seam vertex, or, for the very first segment,
            // built from the particle position.
            Vector v0;
            if (nVert == 0)
                v0 = vP0 + vOffAcross - vOffAlong;
            else
                v0 = Vector(particlePool[nVert - 3].pos[0], particlePool[nVert - 3].pos[1],
                            particlePool[nVert - 3].pos[2]);
            particlePool[nVert + 0].pos[0] = v0.x;
            particlePool[nVert + 0].pos[1] = v0.y;
            particlePool[nVert + 0].pos[2] = v0.z;
            particlePool[nVert + 0].u = fU0;
            particlePool[nVert + 0].v = fV0;

            Vector v1 = lPositions.m_pData[i + 1] + vOffAcross + vOffAlong;
            particlePool[nVert + 1].pos[0] = v1.x;
            particlePool[nVert + 1].pos[1] = v1.y;
            particlePool[nVert + 1].pos[2] = v1.z;
            particlePool[nVert + 1].u = fU0;
            particlePool[nVert + 1].v = fV1;

            Vector v2 = lPositions.m_pData[i + 1] - vOffAcross + vOffAlong;
            particlePool[nVert + 2].pos[0] = v2.x;
            particlePool[nVert + 2].pos[1] = v2.y;
            particlePool[nVert + 2].pos[2] = v2.z;
            particlePool[nVert + 2].u = fU1;
            particlePool[nVert + 2].v = fV1;

            // Vertex 3: the mirror of vertex 0 (read back out of the pool), and the next segment's
            // vertex 0, the strip's shared seam.
            Vector v3;
            if (nVert == 0)
                v3 = vP0 - vOffAcross - vOffAlong;
            else
                v3 = Vector(particlePool[nVert - 2].pos[0], particlePool[nVert - 2].pos[1],
                            particlePool[nVert - 2].pos[2]);
            particlePool[nVert + 3].pos[0] = v3.x;
            particlePool[nVert + 3].pos[1] = v3.y;
            particlePool[nVert + 3].pos[2] = v3.z;
            particlePool[nVert + 3].u = fU1;
            particlePool[nVert + 3].v = fV0;
        }
        nVert += 4;
    }

#if !KOTOR1  // K1 keeps no draw counters
    countfaces += m_lParticles.m_count;
#endif
    GLRender::RenderObjectsVTCQuadUP(particlePool, m_lParticles.m_count * 4 - 4);

    if (!g_useDistortionOutput)
        ASLgl::glDepthMask(1);
    ASLgl::glEnable(enableTranslationTable[kCapDepthTest]);
    ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
    if (bBlendWasEnabled != 0)
        ASLgl::glEnable(enableTranslationTable[kCapFog]);
}

// The sprite-frame spin's two trig lookups.
float sineLUT(float f);
float cosineLUT(float f);

void PartEmitter::RenderMotionBlurred()
{
    static RenderVertexVTC* particlePool = new RenderVertexVTC[1000];
    static unsigned int poolSize = 1000;

    if (m_pGob != NULL && m_pGob->m_pTileData != NULL && m_pGob->m_pTileData->m_fFade < 1.0f)
        return;

    // The camera's forward axis, the sprite-frame spin axis.
    Vector vAxisZ;
#if KOTOR1  // K1 composes each particle's sprite-frame spin with a copy of the camera orientation
    Quaternion qCamera;
    Quaternion qFrame;
#endif

    int nCount = m_lParticles.m_count;
    if (nCount == 0)
        return;

    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode);
    Matrix matCamera = CurrentCamera->m_qOrientation.getmatrix();
    Vector vAxisX(matCamera.m_f[0], matCamera.m_f[1], matCamera.m_f[2]);
    Vector vAxisY(matCamera.m_f[3], matCamera.m_f[4], matCamera.m_f[5]);

    if (renderemitterspheres)
        RenderBoundingSphere();

    // The shared pool is grown to `count * 8` vertices and kept; only the first `poolSize` hold
    // valid data, so the copy stops at the old size.
    if (poolSize < static_cast<unsigned int>(nCount * 4))
    {
        unsigned int nOldSize = poolSize;
        poolSize = static_cast<unsigned int>(nCount * 8);
        particlePool = ResizeEmitterPool(particlePool, nOldSize, poolSize);
    }

    // A Gob colour-shift scale applied to both widths when it is not exactly 1.0, on every path
    // (the flat-colour arm included), as in RenderLinked.
    float fScale = 1.0f;
    if (m_pGob != NULL && m_pGob->m_fObjectScale != 1.0f)
        fScale = m_pGob->m_fObjectScale;

    float fFade = m_fFPS > 0.0f ? 1.0f / m_fFPS : 0.0f;

    m_pParticleMaterial->BindTexture0();
    m_pParticleMaterial->SetBlendingMode(0);

    Matrix matAxisZ = CurrentCamera->m_qOrientation.getmatrix();
    vAxisZ = Vector(matAxisZ.m_f[6], matAxisZ.m_f[7], matAxisZ.m_f[8]);
#if KOTOR1
    qCamera = CurrentCamera->m_qOrientation;
#endif

    // For the per-particle colour/frame step, a "position" emitter (mode 6) spreads the whole
    // particle set across one frame, and every other kind advances by its own rate or the 0.01f
    // quantum.
    float fFrameStep;
    if (m_nGradientMode == 6)
        fFrameStep = 1.0f / static_cast<float>(nCount);
    else
        fFrameStep = m_fLifeExp > 0.0f ? 1.0f / m_fLifeExp : 0.01f;

    // The sprite-sheet grid, with a zero grid treated as one.
    int nXGrid = pNode->m_nXGrid != 0 ? pNode->m_nXGrid : 1;
    int nYGrid = pNode->m_nYGrid;
    float fYGrid = static_cast<float>(nYGrid);
    float fInvXGrid = 1.0f / static_cast<float>(nXGrid);

    if (!g_useDistortionOutput)
        ASLgl::glDepthMask(0);
    ASLgl::glDisable(enableTranslationTable[kCapDepthTest]);
    ASLgl::glEnable(enableTranslationTable[kCapBlend]);
    ASLgl::glDisable(enableTranslationTable[kCapAlphaTest]);
    ASLgl::glDisable(enableTranslationTable[kCapCullFace]);

    unsigned char bBlendWasEnabled;
    if (m_nBlendMode == 0)
    {
        ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
        bBlendWasEnabled = 0;
    }
    else if (m_nBlendMode == 1)
    {
        ASLgl::glDisable(enableTranslationTable[kCapBlend]);
        ASLgl::glEnable(enableTranslationTable[kCapAlphaTest]);
        bBlendWasEnabled = 0;
    }
    else
    {
        ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[2]);
        bBlendWasEnabled = ASLgl::glIsEnabled(enableTranslationTable[kCapFog]);
        ASLgl::glDisable(enableTranslationTable[kCapFog]);
    }

    updateParticleFrame(fFade);
    particlecount += m_lParticles.m_count;
    float fInvYGrid = nYGrid != 0 ? 1.0f / fYGrid : 1.0f;

    float fTileAlpha = GetTileAlpha();
    float fGobAlpha = GetGobAlpha();

    for (int i = nCount - 1; i >= 0; i--)
    {
        Particle* pParticle = m_lParticles.m_pData[i];

        // The particle's color, its alpha and its two ribbon widths.
        Vector vColor;
        float fAlpha;
        float fWidthU;
        float fWidthV;
        if (!(m_fLifeExp >= 0.0f))
        {
            vColor = m_avVectors[9];
            fAlpha = m_fAlphaStart;
            fWidthU = m_fSizeStart * 0.5f;
            fWidthV = (m_fSizeStartY != 0.0f || m_fSizeEndY != 0.0f) ? m_fSizeStartY * 0.5f : fWidthU;
        }
        else
        {
            float fT = (m_nGradientMode == 6) ? static_cast<float>(i) * fFrameStep
                                          : pParticle->m_fAge * fFrameStep;
            if (m_fPercentStart == 255.0f)
            {
                vColor = m_avVectors[9] * (1.0f - fT) + m_avVectors[8] * fT;
                fAlpha = m_fAlphaStart * (1.0f - fT) + m_fAlphaEnd * fT;
                fWidthU = (m_fSizeStart * (1.0f - fT) + m_fSizeEnd * fT) * 0.5f;
                fWidthV = (m_fSizeStartY != 0.0f || m_fSizeEndY != 0.0f)
                        ? (m_fSizeStartY * (1.0f - fT) + m_fSizeEndY * fT) * 0.5f : fWidthU;
            }
            else if (fT >= m_fPercentEnd)
            {
                vColor = m_avVectors[8];
                fAlpha = m_fAlphaEnd;
                fWidthU = m_fSizeEnd;
                fWidthV = (m_fSizeEndY != 0.0f) ? m_fSizeEndY : fWidthU;
            }
            else if (fT >= m_fPercentMid)
            {
                float fF = (fT - m_fPercentMid) / (m_fPercentEnd - m_fPercentMid);
                vColor = m_avVectors[0] * (1.0f - fF) + m_avVectors[8] * fF;
                fAlpha = m_fAlphaMid * (1.0f - fF) + m_fAlphaEnd * fF;
                fWidthU = (m_fSizeMid * (1.0f - fF) + m_fSizeEnd * fF) * 0.5f;
                fWidthV = (m_fSizeMidY != 0.0f || m_fSizeEndY != 0.0f)
                        ? (m_fSizeMidY * (1.0f - fF) + m_fSizeEndY * fF) * 0.5f : fWidthU;
            }
            else if (fT >= m_fPercentStart)
            {
                float fF = (fT - m_fPercentStart) / (m_fPercentMid - m_fPercentStart);
                vColor = m_avVectors[9] * (1.0f - fF) + m_avVectors[0] * fF;
                fAlpha = m_fAlphaStart * (1.0f - fF) + m_fAlphaMid * fF;
                fWidthU = (m_fSizeStart * (1.0f - fF) + m_fSizeMid * fF) * 0.5f;
                fWidthV = (m_fSizeStartY != 0.0f || m_fSizeMidY != 0.0f)
                        ? (m_fSizeStartY * (1.0f - fF) + m_fSizeMidY * fF) * 0.5f : fWidthU;
            }
            else
            {
                vColor = m_avVectors[9];
                fAlpha = m_fAlphaStart;
                fWidthU = m_fSizeStart;
                fWidthV = (m_fSizeStartY != 0.0f) ? m_fSizeStartY : fWidthU;
            }
        }
        float fBlurWidth = fWidthU;
        if (fScale != 1.0f)
        {
            fWidthU *= fScale;
            fWidthV *= fScale;
        }

        vColor.x = vColor.x < 0.0f ? 0.0f : (vColor.x >= 1.0f ? 1.0f : vColor.x);
        vColor.y = vColor.y < 0.0f ? 0.0f : (vColor.y >= 1.0f ? 1.0f : vColor.y);
        vColor.z = vColor.z < 0.0f ? 0.0f : (vColor.z >= 1.0f ? 1.0f : vColor.z);
        fAlpha = fAlpha < 0.0f ? 0.0f : (fAlpha >= 1.0f ? 1.0f : fAlpha);

        // The per-particle sprite-frame orientation, from the camera's forward axis and the
        // particle's own spin angle. Its result is unused; the construction is the statement.
        Vector vAxis = normalize(vAxisZ);
        float fSin = sineLUT(pParticle->m_fRotation * 0.5f);
        float fCos = cosineLUT(pParticle->m_fRotation * 0.5f);
#if KOTOR1  // the product is unused too, but K1's operator* is out of line, so the call stays
        qFrame = Quaternion(fCos, vAxis.x * fSin, vAxis.y * fSin, vAxis.z * fSin);
        qFrame = qFrame * qCamera;
#else
        Quaternion qFrame(fCos, vAxis.x * fSin, vAxis.y * fSin, vAxis.z * fSin);
        (void)qFrame;
#endif

        // The segment's screen-space direction, from the previous position to this one.
        Vector vDir = pParticle->m_vPosition - pParticle->m_vTrailPosition;
        float fScreenX = vDir.x * vAxisX.x + vDir.y * vAxisX.y + vDir.z * vAxisX.z;
        float fScreenY = vDir.x * vAxisY.x + vDir.y * vAxisY.y + vDir.z * vAxisY.z;
        float fLen = __builtin_sqrtf(fScreenX * fScreenX + fScreenY * fScreenY);

        int nFrame = static_cast<int>(fInvXGrid * static_cast<float>(pParticle->m_nFrame));
        int nRemainder = pParticle->m_nFrame - nXGrid * nFrame;

        // The rotation back out to world space. A degenerate (or already-unit) segment leaves the
        // two camera rows untouched, and the zero-length case substitutes 1.0 for the divisor so
        // the quad's stretch below stays finite.
        float fDivisor = fLen != 0.0f ? fLen : 1.0f;
        Vector vAlong = vAxisY;
        Vector vAcross = vAxisX;
        if (fLen != 1.0f && fLen > 0.00001)
        {
            fScreenX /= fDivisor;
            fScreenY /= fDivisor;
            vAlong = vAxisX * fScreenX + vAxisY * fScreenY;
            vAcross = vAxisX * fScreenY - vAxisY * fScreenX;
        }

        Vector vOffAcross = vAcross * (2.0f * fWidthU);
        Vector vOffAlong = vAlong * (fWidthV * 0.5f);

        unsigned int nBaseColor = static_cast<unsigned int>(vColor.x * 255.0f)
                                | (static_cast<unsigned int>(vColor.y * 255.0f) << 8)
                                | (static_cast<unsigned int>(vColor.z * 255.0f) << 16);
        // The trail pair is faded by how far the particle travelled this frame.
        float fBlur = fBlurWidth / fDivisor * 2.0f;
        if (fBlur > 1.0f)
            fBlur = 1.0f;
        unsigned int nHeadColor = nBaseColor
                | (static_cast<unsigned int>(fGobAlpha * (fTileAlpha * fAlpha) * 255.0f) << 24);
        unsigned int nTailColor = nBaseColor
                | (static_cast<unsigned int>(fGobAlpha * (fTileAlpha * (fAlpha * fBlur)) * 255.0f) << 24);

        // In the frame's texcoords, u walks the sprite grid's columns within one frame and v picks
        // the frame row (bottom-up).
        float fU0 = static_cast<float>(nRemainder + 1) * fInvXGrid;
        float fU1 = static_cast<float>(nRemainder) * fInvXGrid;
        float fV0 = 1.0f - static_cast<float>(nFrame + 1) * fInvYGrid;
        float fV1 = 1.0f - static_cast<float>(nFrame) * fInvYGrid;

        int nVert = i * 4;
        particlePool[nVert + 0].color = nHeadColor;
        particlePool[nVert + 1].color = nHeadColor;
        particlePool[nVert + 2].color = nTailColor;
        particlePool[nVert + 3].color = nTailColor;

        Vector vPos = pParticle->m_vPosition;
        Vector vPrev = pParticle->m_vTrailPosition;

        Vector v0 = vPos + vOffAcross + vOffAlong;
        particlePool[nVert + 0].pos[0] = v0.x;
        particlePool[nVert + 0].pos[1] = v0.y;
        particlePool[nVert + 0].pos[2] = v0.z;
        particlePool[nVert + 0].u = fU0;
        particlePool[nVert + 0].v = fV1;

        Vector v1 = vPos - vOffAcross + vOffAlong;
        particlePool[nVert + 1].pos[0] = v1.x;
        particlePool[nVert + 1].pos[1] = v1.y;
        particlePool[nVert + 1].pos[2] = v1.z;
        particlePool[nVert + 1].u = fU1;
        particlePool[nVert + 1].v = fV1;

        Vector v2 = vPrev - vOffAcross - vOffAlong;
        particlePool[nVert + 2].pos[0] = v2.x;
        particlePool[nVert + 2].pos[1] = v2.y;
        particlePool[nVert + 2].pos[2] = v2.z;
        particlePool[nVert + 2].u = fU1;
        particlePool[nVert + 2].v = fV0;

        Vector v3 = vPrev + vOffAcross - vOffAlong;
        particlePool[nVert + 3].pos[0] = v3.x;
        particlePool[nVert + 3].pos[1] = v3.y;
        particlePool[nVert + 3].pos[2] = v3.z;
        particlePool[nVert + 3].u = fU0;
        particlePool[nVert + 3].v = fV0;
    }

#if !KOTOR1  // K1 keeps no draw counters
    countfaces += nCount;
#endif
    GLRender::RenderObjectsVTCQuadUP(particlePool, nCount * 4);

    if (!g_useDistortionOutput)
        ASLgl::glDepthMask(1);
    ASLgl::glEnable(enableTranslationTable[kCapDepthTest]);
    ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
    ASLgl::glEnable(enableTranslationTable[kCapCullFace]);
    if (bBlendWasEnabled != 0)
        ASLgl::glEnable(enableTranslationTable[kCapFog]);
}

void PartEmitter::RenderStandard()
{
    static RenderVertexVTC* particlePool = new RenderVertexVTC[1000];
    static unsigned int poolSize = 1000;

    if (m_pGob != NULL && m_pGob->m_pTileData != NULL && m_pGob->m_pTileData->m_fFade < 1.0f)
        return;

    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode);

    // Frame blending draws the whole particle set once per sprite frame, five frames deep.
    int nPassCount = pNode->m_bFrameBlending ? 5 : 1;

    if (renderemitterspheres)
    {
        Vector vCenter;
        Quaternion qOrientation;
        GetWorld(vCenter, qOrientation);
        Vector vColor = { 1.0f, 0.5f, 1.0f };
        GLRender::DrawSphereBound(m_fRadius, vCenter, vColor, 1.0f, true);
    }

    // Returns on an empty particle list; the bounding sphere above still draws.
    int nCount = m_lParticles.m_count;
    if (nCount == 0)
        return;

    if (poolSize < static_cast<unsigned int>(nCount * 4))
    {
        unsigned int nOldSize = poolSize;
        poolSize = static_cast<unsigned int>(nCount * 8);
        particlePool = ResizeEmitterPool(particlePool, nOldSize, poolSize);
    }

    float fScale = 1.0f;
    if (m_pGob != NULL && m_pGob->m_fObjectScale != 1.0f)
        fScale = m_pGob->m_fObjectScale;

    float fFade = m_fFPS > 0.0f ? 1.0f / m_fFPS : 0.0f;

    m_pParticleMaterial->BindTexture0();
    m_pParticleMaterial->SetBlendingMode(0);

    // The emitter's orientation for this draw mode; only its Z axis is read.
    Matrix matOrient;
    Quaternion qOrient;
    float fSign = 1.0f;
    int nMode = m_nRenderMode;
    if (nMode == 1)
    {
        qOrient = CurrentCamera->m_qOrientation;
        matOrient = qOrient.getmatrix();
    }
    else if (nMode == 2)
    {
        qOrient = m_qWorldOrientation;
        matOrient = qOrient.getmatrix();
    }
    else if (nMode == 3)
    {
        qOrient = Quaternion(Vector(0.0f, 0.0f, 1.0f), 0.0f);
    }
    else
    {
        qOrient = CurrentCamera->m_qOrientation;
        matOrient = qOrient.getmatrix();
        // The world-forward dot of the matrix's Y row; a negative result flips the flat-billboard
        // arm's up axis.
        Vector vForward = { 0.0f, 0.0f, 1.0f };
        float fForwardY = matOrient.m_f[3] * vForward.x + matOrient.m_f[4] * vForward.y
                        + matOrient.m_f[5] * vForward.z;
        if (fForwardY < 0.0f)
            fSign = -1.0f;
    }
    Vector vAxisZ(matOrient.m_f[6], matOrient.m_f[7], matOrient.m_f[8]);

    // The sprite-sheet grid and the per-particle frame step.
    int nXGrid = pNode->m_nXGrid != 0 ? pNode->m_nXGrid : 1;
    int nYGrid = pNode->m_nYGrid;
    float fFrameStep = m_fLifeExp > 0.0f ? 1.0f / m_fLifeExp : 0.01f;

    if (!g_useDistortionOutput)
        ASLgl::glDepthMask(0);
    ASLgl::glDisable(enableTranslationTable[kCapDepthTest]);
    ASLgl::glEnable(enableTranslationTable[kCapBlend]);
    ASLgl::glDisable(enableTranslationTable[kCapAlphaTest]);
#if !KOTOR1  // K1 leaves the multisample cap alone
    if (g_nAliasingMode > 0)
        ASLgl::glDisable(enableTranslationTable[kCapMultisample]);
#endif

    // A two-sided-textured node is drawn with both facings filled in, and cull face off.
    if (pNode->m_nTwoSidedTex != 0)
    {
        ASLgl::glPolygonMode(facingTranslationTable[2], fillTranslationTable[2]);
        ASLgl::glDisable(enableTranslationTable[kCapCullFace]);
    }

    unsigned char bBlendWasEnabled = 0;
    if (m_nBlendMode == 1)
    {
        ASLgl::glDisable(enableTranslationTable[kCapBlend]);
        ASLgl::glEnable(enableTranslationTable[kCapAlphaTest]);
    }
    else if (m_nBlendMode == 0)
    {
        ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
    }
    else
    {
        ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[2]);
        bBlendWasEnabled = ASLgl::glIsEnabled(enableTranslationTable[kCapFog]);
        ASLgl::glDisable(enableTranslationTable[kCapFog]);
    }

#if !KOTOR1  // K1 has no MG_Distort wake special case
    // The MG_Distort wake billboard is drawn with a fixed negative ignition offset on the emitter
    // and on every particle.
    if (g_adjustWake && m_pGob != NULL)
    {
        if (strcmp(m_pGob->GetName(), "MG_Distort") == 0)
        {
            m_vLocalPosition.y = -1.0f;
            for (int64_t i = 0; i < m_lParticles.m_count; i++)
                m_lParticles.m_pData[i]->m_vSpreadVelocity.y = -1.3f;
        }
    }
#endif

    updateParticleFrame(fFade);
    particlecount += m_lParticles.m_count;
    ASLgl::glPushMatrix();

    float fTileAlpha = GetTileAlpha();
    float fGobAlpha = GetGobAlpha();

    float fInvXGrid = 1.0f / static_cast<float>(nXGrid);
    float fInvYGrid = nYGrid != 0 ? 1.0f / static_cast<float>(nYGrid) : 1.0f;

    // Of the two frame-blend walkers, passes 1/3 carry the first and passes 2/4 the second.
    int nFrameA = 0;
    int nFrameB = 0;

    for (int nPass = 0; nPass < nPassCount; nPass++)
    {
        int nVert = m_lParticles.m_count * 4 - 4;
        for (int i = 0; i < m_lParticles.m_count; i++)
        {
            Particle* pParticle = m_lParticles.m_pData[i];

            // The particle's colour, its alpha and its two widths.
            Vector vColor;
            float fAlpha;
            float fWidthU;
            float fWidthV;
            if (!(m_fLifeExp >= 0.0f))
            {
                vColor = m_avVectors[9];
                fAlpha = m_fAlphaStart;

                vColor.x = vColor.x < 0.0f ? 0.0f : (vColor.x >= 1.0f ? 1.0f : vColor.x);
                vColor.y = vColor.y < 0.0f ? 0.0f : (vColor.y >= 1.0f ? 1.0f : vColor.y);
                vColor.z = vColor.z < 0.0f ? 0.0f : (vColor.z >= 1.0f ? 1.0f : vColor.z);
                fAlpha = fAlpha < 0.0f ? 0.0f : (fAlpha >= 1.0f ? 1.0f : fAlpha);

                fWidthU = m_fSizeStart * 0.5f;
                fWidthV = (m_fSizeStartY != 0.0f || m_fSizeMidY != 0.0f || m_fSizeEndY != 0.0f)
                        ? m_fSizeStartY * 0.5f : fWidthU;
            }
            else
            {
                float fT = fFrameStep * pParticle->m_fAge;
                if (m_fPercentStart == 255.0f)
                {
                    vColor = m_avVectors[9] * (1.0f - fT) + m_avVectors[8] * fT;
                    fAlpha = m_fAlphaStart * (1.0f - fT) + m_fAlphaEnd * fT;
                    fWidthU = (m_fSizeStart * (1.0f - fT) + m_fSizeEnd * fT) * 0.5f;
                    fWidthV = (m_fSizeStartY != 0.0f || m_fSizeEndY != 0.0f)
                            ? (m_fSizeStartY * (1.0f - fT) + m_fSizeEndY * fT) * 0.5f : fWidthU;
                }
                else if (fT >= m_fPercentEnd)
                {
                    vColor = m_avVectors[8];
                    fAlpha = m_fAlphaEnd;
                    fWidthU = m_fSizeEnd * 0.5f;
                    fWidthV = (m_fSizeEndY != 0.0f) ? m_fSizeEndY * 0.5f : fWidthU;
                }
                else if (fT >= m_fPercentMid)
                {
                    float fF = (fT - m_fPercentMid) / (m_fPercentEnd - m_fPercentMid);
                    vColor = m_avVectors[0] * (1.0f - fF) + m_avVectors[8] * fF;
                    fAlpha = m_fAlphaMid * (1.0f - fF) + m_fAlphaEnd * fF;
                    fWidthU = (m_fSizeMid * (1.0f - fF) + m_fSizeEnd * fF) * 0.5f;
                    fWidthV = (m_fSizeMidY != 0.0f || m_fSizeEndY != 0.0f)
                            ? (m_fSizeMidY * (1.0f - fF) + m_fSizeEndY * fF) * 0.5f : fWidthU;
                }
                else if (fT >= m_fPercentStart)
                {
                    float fF = (fT - m_fPercentStart) / (m_fPercentMid - m_fPercentStart);
                    vColor = m_avVectors[9] * (1.0f - fF) + m_avVectors[0] * fF;
                    fAlpha = m_fAlphaStart * (1.0f - fF) + m_fAlphaMid * fF;
                    fWidthU = (m_fSizeStart * (1.0f - fF) + m_fSizeMid * fF) * 0.5f;
                    fWidthV = (m_fSizeStartY != 0.0f || m_fSizeMidY != 0.0f)
                            ? (m_fSizeStartY * (1.0f - fF) + m_fSizeMidY * fF) * 0.5f : fWidthU;
                }
                else
                {
                    vColor = m_avVectors[9];
                    fAlpha = m_fAlphaStart;
                    fWidthU = m_fSizeStart * 0.5f;
                    fWidthV = (m_fSizeStartY != 0.0f) ? m_fSizeStartY * 0.5f : fWidthU;
                }

                vColor.x = vColor.x < 0.0f ? 0.0f : (vColor.x >= 1.0f ? 1.0f : vColor.x);
                vColor.y = vColor.y < 0.0f ? 0.0f : (vColor.y >= 1.0f ? 1.0f : vColor.y);
                vColor.z = vColor.z < 0.0f ? 0.0f : (vColor.z >= 1.0f ? 1.0f : vColor.z);
                fAlpha = fAlpha < 0.0f ? 0.0f : (fAlpha >= 1.0f ? 1.0f : fAlpha);
            }

            // The particle's own frame is an override pair plus the spin axis. The oriented flag
            // replaces both widths and picks the particle's own m_vDirection as the spin axis. A
            // self-oriented particle spins its own m_qOrientation, every other one the emitter's
            // orientation built by the mode switch above.
            unsigned int nNodeFlags = pNode->m_nEmitterFlags;
            Quaternion qFrame;
            if (pParticle->m_bLanded)
            {
                if ((nNodeFlags & MdlNodeEmitter::kEmitterSplat) != 0)
                {
                    fWidthU = m_fSizeEnd;
                    fWidthV = (m_fSizeEndY != 0.0f) ? m_fSizeEndY : m_fSizeEnd;
                }
                qFrame = Quaternion(pParticle->m_vDirection, pParticle->m_fRotation)
                       * pParticle->m_qOrientation;
            }
            else
            {
                if ((nNodeFlags & MdlNodeEmitter::kEmitterSplat) != 0)
                {
                    fWidthU = m_fSizeStart;
                    fWidthV = (m_fSizeStartY != 0.0f) ? m_fSizeStartY : m_fSizeStart;
                }
                qFrame = Quaternion(vAxisZ, pParticle->m_fRotation) * qOrient;
            }

            // The Gob colour-shift scale, applied on every path.
            if (fScale != 1.0f)
            {
                fWidthU *= fScale;
                fWidthV *= fScale;
            }

            // Which sprite frame this pass draws, and how strongly. Passes 1 and 2 derive a
            // candidate frame; passes 3 and 4 step those candidates once more.
            int nFrame = 0;
            int nRemainder = 0;
            float fBlend = 0.0f;
            switch (nPass)
            {
            case 1:
                nFrameA = pParticle->m_nFrame;
                if (m_fFrameStart != m_fFrameEnd)
                {
                    if (!(m_fFrameEnd < m_fFrameStart))
                    {
                        nFrameA += 1;
                        if (!(m_fFrameEnd >= static_cast<float>(nFrameA)))
                            nFrameA = -1;
                    }
                    else
                    {
                        nFrameA -= 1;
                        if (!(m_fFrameEnd <= static_cast<float>(nFrameA)))
                            nFrameA = -1;
                    }
                    if (nFrameA >= 0)
                    {
                        fBlend = (pParticle->m_fAge - pParticle->m_fFrameTime) / fFade * 0.5f;
                        nFrame = static_cast<int>(fInvXGrid * static_cast<float>(nFrameA));
                        nRemainder = nFrameA - nXGrid * nFrame;
                    }
                }
                else
                {
                    nFrameA = -1;
                }
                break;
            case 2:
                nFrameB = pParticle->m_nFrame;
                if (m_fFrameStart != m_fFrameEnd)
                {
                    if (!(m_fFrameEnd < m_fFrameStart))
                    {
                        nFrameB -= 1;
                        if (!(m_fFrameStart <= static_cast<float>(nFrameB)))
                            nFrameB = -1;
                    }
                    else
                    {
                        nFrameB += 1;
                        if (!(m_fFrameStart >= static_cast<float>(nFrameB)))
                            nFrameB = -1;
                    }
                    if (nFrameB >= 0)
                    {
                        fBlend = 1.0f - (pParticle->m_fAge - pParticle->m_fFrameTime) / fFade * 0.5f;
                        nFrame = static_cast<int>(fInvXGrid * static_cast<float>(nFrameB));
                        nRemainder = nFrameB - nXGrid * nFrame;
                    }
                }
                else
                {
                    nFrameB = -1;
                }
                break;
            case 3:
                if (m_fFrameStart != m_fFrameEnd)
                {
                    if (!(m_fFrameEnd < m_fFrameStart))
                    {
                        nFrameA += 1;
                        if (!(m_fFrameEnd >= static_cast<float>(nFrameA)))
                            nFrameA = -1;
                    }
                    else
                    {
                        nFrameA -= 1;
                        if (!(m_fFrameEnd <= static_cast<float>(nFrameA)))
                            nFrameA = -1;
                    }
                    if (nFrameA >= 0)
                    {
                        fBlend = (pParticle->m_fAge - pParticle->m_fFrameTime) / fFade * 0.25f;
                        nFrame = static_cast<int>(fInvXGrid * static_cast<float>(nFrameA));
                        nRemainder = nFrameA - nXGrid * nFrame;
                    }
                }
                else
                {
                    nFrameA = -1;
                }
                break;
            case 4:
                if (m_fFrameStart != m_fFrameEnd)
                {
                    if (!(m_fFrameEnd < m_fFrameStart))
                    {
                        nFrameB -= 1;
                        if (!(m_fFrameStart <= static_cast<float>(nFrameB)))
                            nFrameB = -1;
                    }
                    else
                    {
                        nFrameB += 1;
                        if (!(m_fFrameStart >= static_cast<float>(nFrameB)))
                            nFrameB = -1;
                    }
                    if (nFrameB >= 0)
                    {
                        fBlend = (1.0f - (pParticle->m_fAge - pParticle->m_fFrameTime) / fFade)
                               * 0.5f * 0.5f;
                        nFrame = static_cast<int>(fInvXGrid * static_cast<float>(nFrameB));
                        nRemainder = nFrameB - nXGrid * nFrame;
                    }
                }
                else
                {
                    nFrameB = -1;
                }
                break;
            default:
                // The pass that just draws the particle's own frame, at full alpha (or half when
                // the emitter is frame-blending).
                nRemainder = pParticle->m_nFrame;
                nFrame = static_cast<int>(fInvXGrid * static_cast<float>(nRemainder));
                nRemainder -= nXGrid * nFrame;
                fBlend = pNode->m_bFrameBlending ? 0.5f : 1.0f;
                break;
            }

            // The quad's two half-axes.
            Matrix matBasis;
            Vector vRight;
            Vector vUp;
            if (m_nRenderMode == 8)
            {
                // For a self-orienting emitter, both axes come straight off the particle's own
                // quaternion and the spin product above is unused.
                matBasis = pParticle->m_qOrientation.getmatrix();
                vRight = Vector(matBasis.m_f[0], matBasis.m_f[1], matBasis.m_f[2]);
                vUp = Vector(matBasis.m_f[6], matBasis.m_f[7], matBasis.m_f[8]);
            }
            else
            {
                matBasis = qFrame.getmatrix();
                vRight = Vector(matBasis.m_f[0], matBasis.m_f[1], matBasis.m_f[2]);
                if (m_nRenderMode == 5)
                    vUp = Vector(0.0f, 0.0f, fSign);
                else if ((nNodeFlags & MdlNodeEmitter::kEmitterSplat) != 0)
                    vUp = cross(pParticle->m_vDirection, vRight);
                else
                    vUp = cross(vAxisZ, vRight);
            }

            int nColor = static_cast<int>(vColor.x * 255.0f)
                       | (static_cast<int>(vColor.y * 255.0f) << 8)
                       | (static_cast<int>(vColor.z * 255.0f) << 16)
                       | (static_cast<int>(fGobAlpha * (fTileAlpha * (fAlpha * fBlend)) * 255.0f) << 24);

            Vector vA = vRight * fWidthU;
            Vector vB = vUp * fWidthV;

            float fU0 = fInvXGrid * static_cast<float>(nRemainder);
            float fU1 = fInvXGrid * static_cast<float>(nRemainder + 1);
            float fV0 = 1.0f - fInvYGrid * static_cast<float>(nFrame);
            float fV1 = 1.0f - fInvYGrid * static_cast<float>(nFrame + 1);

            particlePool[nVert + 0].color = nColor;
            particlePool[nVert + 1].color = nColor;
            particlePool[nVert + 2].color = nColor;
            particlePool[nVert + 3].color = nColor;

            Vector vPos = pParticle->m_vPosition;

            Vector v0 = vPos - vA - vB;
            particlePool[nVert + 0].pos[0] = v0.x;
            particlePool[nVert + 0].pos[1] = v0.y;
            particlePool[nVert + 0].pos[2] = v0.z;
            particlePool[nVert + 0].u = fU0;
            particlePool[nVert + 0].v = fV1;

            Vector v1 = vPos + vA - vB;
            particlePool[nVert + 1].pos[0] = v1.x;
            particlePool[nVert + 1].pos[1] = v1.y;
            particlePool[nVert + 1].pos[2] = v1.z;
            particlePool[nVert + 1].u = fU1;
            particlePool[nVert + 1].v = fV1;

            Vector v2 = vPos + vA + vB;
            particlePool[nVert + 2].pos[0] = v2.x;
            particlePool[nVert + 2].pos[1] = v2.y;
            particlePool[nVert + 2].pos[2] = v2.z;
            particlePool[nVert + 2].u = fU1;
            particlePool[nVert + 2].v = fV0;

            Vector v3 = vPos - vA + vB;
            particlePool[nVert + 3].pos[0] = v3.x;
            particlePool[nVert + 3].pos[1] = v3.y;
            particlePool[nVert + 3].pos[2] = v3.z;
            particlePool[nVert + 3].u = fU0;
            particlePool[nVert + 3].v = fV0;

            nVert -= 4;
        }

        // Every pass after the first draws additively over the previous frame's sprite.
        if (nPass != 0)
        {
            ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[2]);
            bBlendWasEnabled = ASLgl::glIsEnabled(enableTranslationTable[kCapFog]);
            ASLgl::glDisable(enableTranslationTable[kCapFog]);
        }

#if !KOTOR1  // K1 keeps no draw counters
        countfaces += m_lParticles.m_count;
#endif
        GLRender::RenderObjectsVTCQuadUP(particlePool, m_lParticles.m_count * 4);
    }

    ASLgl::glPopMatrix();
    if (!g_useDistortionOutput)
        ASLgl::glDepthMask(1);
    ASLgl::glEnable(enableTranslationTable[kCapDepthTest]);
    ASLgl::glBlendFunc(blendTranslationTable[0], blendTranslationTable[1]);
#if !KOTOR1
    if (g_nAliasingMode > 0)
        ASLgl::glEnable(enableTranslationTable[kCapMultisample]);
#endif
    if (pNode->m_nTwoSidedTex != 0)
    {
        ASLgl::glPolygonMode(facingTranslationTable[0], fillTranslationTable[2]);
        ASLgl::glEnable(enableTranslationTable[kCapCullFace]);
    }
    if (bBlendWasEnabled != 0)
        ASLgl::glEnable(enableTranslationTable[kCapFog]);
}

void PartEmitter::RenderNothing()
{
    // A fading-out emitter draws nothing until it is opaque.
    if (m_pGob != NULL && m_pGob->m_pTileData != NULL && m_pGob->m_pTileData->m_fFade < 1.0f)
        return;

    if (renderemitterspheres)
    {
        Vector vCenter;
        Quaternion qOrientation;
        GetWorld(vCenter, qOrientation);
        Vector vColor = { 1.0f, 0.5f, 1.0f };
        GLRender::DrawSphereBound(m_fRadius, vCenter, vColor, 1.0f, true);
    }

    if (m_lParticles.m_count > 0)
    {
        for (int64_t i = 0; i < m_lParticles.m_count; i++)
        {
            ChunkyParticle* pParticle =
                static_cast<ChunkyParticle*>(m_lParticles.m_pData[i]);
            if (pParticle->m_pAurObject == NULL)
                continue;

            pParticle->m_pAurObject->SetPosition(pParticle->m_vPosition);

            // A non-spinning emitter never touches the orientation.
            if (m_fParticleRot == 0.0f || pParticle->m_bLanded)
                continue;

            pParticle->m_pAurObject->SetOrientation(
                Quaternion(pParticle->m_vDirection, pParticle->m_fRotation));
        }
    }
}

void PartEmitter::RenderBoundingSphere()
{
    // The sphere sits at the part's world origin; the orientation is only an out-parameter.
    Vector vCenter;
    Quaternion qOrientation;
    GetWorld(vCenter, qOrientation);
    Vector vColor = { 1.0f, 0.5f, 1.0f };
    GLRender::DrawSphereBound(m_fRadius, vCenter, vColor, 1.0f, true);
}

void PartEmitter::RecursiveFractal(int nIndex, int nCount, int nDepth, Vector vA, Vector vB,
                                   float fScale)
{
    for (;;)
    {
        // The scale quantises to hundredths; the halving below is a double.
        int nMax = static_cast<int>(fScale * 100.0f);
        int nRand = rand_wincompatible() % nMax;
        vB.x += m_fLightningRadius * (static_cast<float>(nRand) * 0.01f);

        // The bolt path is taken only by a LightningEmitter; every other emitter kind answers 0.
        LightningEmitter* pLightning = AsLightningEmitter();
        Vector v = vA * vB.x;
        if (pLightning)
            pLightning->m_lParticleOffsets.m_pData[nIndex] = v;
        else
            m_lParticles.m_pData[nIndex]->m_vPosition
                = m_lParticles.m_pData[nIndex]->m_vPosition + v;

        if (nCount == 0)
            return;
        nCount = static_cast<int>(nCount * 0.5);
        fScale *= 0.5f;
        RecursiveFractal(nIndex - nCount, nCount, nDepth, vA, vB, fScale);
        nIndex += nCount;
    }
}

float PartEmitter::GetGobAlpha()
{
    // No Gob means no colour-shift record, so the emitter is opaque.
    Gob* pGob = m_pGob;
    if (pGob)
        return pGob->m_fColorShiftAmount;
    return 1.0f;
}

float PartEmitter::GetTileAlpha()
{
    TileData* pTile = m_pTileData;
    if (!pTile)
    {
        if (!m_pGob)
            return 1.0f;
        pTile = m_pGob->m_pTileData;
        if (!pTile)
            return 1.0f;
    }
    if (m_avVectors[6].z - pTile->m_vPosition.z > emitterfadeheight)
        return pTile->m_fOpacity;
    return 1.0f;
}

FILE* pAurLogFile;

char* aurstartlog(char* pszName)
{
    K2_UNUSED(pszName);
#if KOTOR1  // K1 uses the port's tracing fopen
    pAurLogFile = _fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/libsource/AURORA/auremitter.cpp",
                         "aurstartlog", 824, "AurLogFile.txt", "w");
#else
    pAurLogFile = fopen("AurLogFile.txt", "w");
#endif
    return pAurLogFile != NULL ? const_cast<char*>("success") : const_cast<char*>("failed");
}

void PartEmitter::CalculateRadius()
{
    if (m_pGob != 0 && m_pGob->m_pModel != 0 && (m_pGob->m_pModel->m_nFlags & 1) != 0)
    {
        m_fRadius = 10000.0f;
        return;
    }

    Vector vLocal(0.0f, 0.0f, 0.0f);
    Quaternion qLocal;
    GetWorld(vLocal, qLocal);

    if (m_spTarget.m_pObject != 0)
    {
        m_fRadius = magnitude(m_spTarget.m_pObject->GetPosition() - vLocal);
        return;
    }

    float fGravity = m_fMass * -9.81f;
    m_fGravityAccel = fGravity < 0.0f ? -fGravity : fGravity;

    // magnitude() takes its argument by address, so the wind vector needs a local.
    float fWind = 0.0f;
    if (m_spWindManager.m_pObject != 0)
    {
        Vector vWind = m_spWindManager.m_pObject->m_vWind;
        fWind = magnitude(vWind);
    }

    // The radius terms are speed and wind travel, the two half-extent maxima, then the position
    // spread.
    float fSpeed = m_fLifeExp;
    float fTerm = fSpeed * (m_fVelocity + m_fRandVel + fWind);
    float fVel = m_fGravityAccel * 0.5f * fSpeed * fSpeed;
    float fHalfA = 0.5f * std::max(std::max(m_fSizeStart, m_fSizeEnd), m_fSizeMid);
    float fHalfB = 0.5f * std::max(std::max(m_fSizeStartY, m_fSizeEndY), m_fSizeMidY);
    float fSpreadX = m_fXSize / 100.0f;
    float fSpreadZ = m_fYSize / 100.0f;
    float fDiag = __builtin_sqrtf(fSpreadX * fSpreadX + fSpreadZ * fSpreadZ);
    float fPick = fHalfB;
    if (fHalfB <= 0.0f)
        fPick = fHalfA;

    m_fRadius = 1.15f * (fDiag + (fTerm + fVel + __builtin_sqrtf(fHalfA * fHalfA + fPick * fPick)));

    // Grow the estimate so it reaches every live particle's own aurora object.
    for (int i = 0; i < m_lParticles.m_count; i++)
    {
        ChunkyParticle* pChunky = m_lParticles.m_pData[i]->AsChunkyParticle();
        if (pChunky == 0)
            break;

        Gob* pObject = static_cast<Gob*>(pChunky->m_pAurObject);
        if (pObject != 0)
        {
            float fReach = magnitude(pObject->m_vPosition - vLocal) + pObject->m_fMinSphereRadius;
            m_fRadius = std::max(m_fRadius, fReach);
        }
    }

    // The emitter's birthrate is handed to the shared counter the particle spawners read.
    birth = m_fBirthRate;
}

void PartEmitter::MoveFunc(float fDeltaT)
{
    // AsMdlNodeEmitter hands back a plain MdlNode*, so the emitter view needs an explicit cast.
    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode->AsMdlNodeEmitter());
    if (!pNode->m_flags.m_bPointToPoint)
    {
        moveParticleStandard(fDeltaT);
        return;
    }
    // A point-to-point emitter whose watched object has died stops stepping entirely.
    if (!m_spTarget.m_pObject)
        return;
    if (pNode->m_flags.m_bPointToPointSel)
        moveParticleP2PBezier(fDeltaT);
    else
        moveParticleP2PGrav(fDeltaT);
}

void PartEmitter::chkParticleLife(float fDeltaT)
{
    // Snapshot the chunk resref before the loop touches the particles.
    char cHasResRef = static_cast<MdlNodeEmitter*>(m_pNode)->m_szChunkName[0];
    for (int i = m_lParticles.m_count - 1; i >= 0; i--)
    {
        Particle* pParticle = m_lParticles.m_pData[i];
        if (m_fLifeExp >= 0.0f && pParticle->m_fAge >= m_fLifeExp)
        {
            // Expired, so move the particle off the live list and onto the free list.
            m_lParticles.DelIndex(i);
            m_lFreeParticles.Add(pParticle);
            if (cHasResRef)
            {
                // Only a chunky emitter's particles have aurora objects.
                CAurObject* pObject = static_cast<ChunkyParticle*>(pParticle)->m_pAurObject;
                if (pObject)
                    pObject->AttachToScene(0);
            }
        }
        else
        {
            pParticle->m_fAge += fDeltaT;
            if (pParticle->m_fAge < 0.0f)
            {
                pParticle->m_fAge = 0.0f;
                pParticle->m_fLastFrameAge = 0.0f;
            }
        }
    }
}

void PartEmitter::updateParticleFrame(float fDeltaT)
{
    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode);
    // The frame range is a span, not an interval, so it holds |first - last| + 1 frames.
    int nRange = static_cast<int>(m_fFrameEnd - m_fFrameStart);
    if (nRange < 0)
        nRange = -nRange;
    nRange += 1;
    bool bRandom = pNode->m_flags.m_bRandom;
    bool bRandomOrder = pNode->m_flags.m_bRandomOrder;

    for (int i = 0; i < m_lParticles.m_count; i++)
    {
        Particle* pParticle = m_lParticles.m_pData[i];

        // A random emitter meters its frames off one age field and everything else off the other;
        // both hold the age at which the frame last changed.
        float* pfLastFrame = bRandom ? &pParticle->m_fLastFrameAge : &pParticle->m_fFrameTime;
        float fElapsed = pParticle->m_fAge - *pfLastFrame;

        if (fDeltaT != 0.0f && fElapsed >= fDeltaT && m_fFrameStart != m_fFrameEnd)
        {
            if (bRandomOrder)
            {
                int nFrame = rand_wincompatible() % nRange;
                pParticle->m_nFrame = nFrame;
                if (!bRandom)
                    pParticle->m_fFrameTime = static_cast<float>(nFrame) * fDeltaT;
            }
            else
            {
                int nFirst = static_cast<int>(m_fFrameEnd);
                int nLast = static_cast<int>(m_fFrameStart);
                float fSteps = __builtin_floorf(fElapsed / fDeltaT);

                if (m_fFrameEnd < m_fFrameStart)
                {
                    // Descending, the frame steps down and an underrun wraps back to the high end.
                    int nFrame = pParticle->m_nFrame;
                    int nSteps = static_cast<int>(-fSteps);
                    nFrame += nSteps;
                    pParticle->m_nFrame = nFrame;
                    if (bRandom)
                    {
                        pParticle->m_fFrameTime += static_cast<float>(nSteps) * fDeltaT;
                        if (m_fFrameEnd > static_cast<float>(nFrame))
                        {
                            do {
                                nFrame += nLast - nFirst + 1;
                            } while (m_fFrameEnd > static_cast<float>(nFrame));
                            pParticle->m_nFrame = nFrame;
                        }
                    }
                    else
                    {
#if !KOTOR1  // K1 has no "loop" wrap: an overrun is only clamped
                        if (nFrame < nLast && pNode->m_nLoop)
                        {
                            nFrame = nFirst;
                            pParticle->m_nFrame = nFrame;
                        }
#endif
                        nFrame = nFrame > nLast ? nFrame : nLast;
                        pParticle->m_nFrame = nFrame;
                        pParticle->m_fFrameTime = static_cast<float>(nFrame) * fDeltaT;
                    }
                }
                else
                {
                    // Ascending, the frame steps up and an overrun wraps back down.
                    int nFrame = pParticle->m_nFrame;
                    int nSteps = static_cast<int>(fSteps);
                    nFrame += nSteps;
                    pParticle->m_nFrame = nFrame;
                    if (bRandom)
                    {
                        pParticle->m_fFrameTime += static_cast<float>(nSteps) * fDeltaT;
                        if (nFrame > nFirst)
                        {
                            do {
                                nFrame -= nFirst - nLast + 1;
                            } while (nFrame > nFirst);
                            pParticle->m_nFrame = nFrame;
                        }
                    }
                    else
                    {
#if !KOTOR1  // K1 has no "loop" wrap: an overrun is only clamped
                        // "loop" sends an overrun back to the far end instead of clamping.
                        if (nFrame > nFirst && pNode->m_nLoop)
                        {
                            nFrame = nLast;
                            pParticle->m_nFrame = nFrame;
                        }
#endif
                        nFrame = nFrame < nFirst ? nFrame : nFirst;
                        pParticle->m_nFrame = nFrame;
                        pParticle->m_fFrameTime = static_cast<float>(nFrame) * fDeltaT;
                    }
                }
            }

            // An emitter with an infinite lifetime keeps its particles' clocks at zero.
            if (m_fLifeExp >= 0.0f)
                pParticle->m_fLastFrameAge = pParticle->m_fAge;
            else
            {
                pParticle->m_fLastFrameAge = 0.0f;
                pParticle->m_fAge = 0.0f;
            }
        }
    }
}

void PartEmitter::chkRandom(Particle* pParticle)
{
    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pNode);
    if (!pNode->m_flags.m_bRandom)
        return;
    int nFrames = static_cast<int>(m_fFrameEnd - m_fFrameStart) + 1;
    if (nFrames < 0)
        nFrames = -nFrames;
    if (nFrames == 0)
        return;
    int nRand = rand_wincompatible() % nFrames;
    pParticle->m_nFrame = static_cast<int>(m_fFrameStart + static_cast<float>(nRand));
}

int PartEmitter::GetNextParticleFrame(int nFrame)
{
    // Returns -1 when there is no next frame in the current direction.
    if (m_fFrameStart != m_fFrameEnd)
    {
        if (m_fFrameEnd < m_fFrameStart)
        {
            int n = nFrame - 1;
            if (m_fFrameEnd <= static_cast<float>(n))
                return n;
        }
        else
        {
            int n = nFrame + 1;
            if (m_fFrameEnd >= static_cast<float>(n))
                return n;
        }
    }
    return -1;
}

int PartEmitter::GetPreviousParticleFrame(int nFrame)
{
    if (m_fFrameStart != m_fFrameEnd)
    {
        if (m_fFrameEnd < m_fFrameStart)
        {
            int n = nFrame + 1;
            if (m_fFrameStart >= static_cast<float>(n))
                return n;
        }
        else
        {
            int n = nFrame - 1;
            if (m_fFrameStart <= static_cast<float>(n))
                return n;
        }
    }
    return -1;
}

extern int totalparticles;

Particle::Particle(PartEmitter* pEmitter, float fDeltaT)
    : m_vPosition(), m_vVelocity(), m_qOrientation(),
      m_vDirection(), m_vSpreadVelocity(), m_vTrailPosition()
{
    m_pEmitter = pEmitter;
    initialize(fDeltaT, 1.0f);
    totalparticles++;
}

Particle::~Particle()
{
    totalparticles--;
}


void Particle::initialize(float fDeltaT, float fScale)
{
    // Re-seed the particle from its emitter.
    m_bLanded = false;
    m_fRotation = 0.0f;
    m_fAge = 0.0f;
    m_fLastFrameAge = 0.0f;
    m_fFrameTime = 0.0f;
    m_qOrientation = m_pEmitter->m_qWorldOrientation;
    m_fSpeed = 0.0f;
    m_vVelocity = Vector(0.0f, 0.0f, 0.0f);
    m_nFrame = static_cast<int>(m_pEmitter->m_fFrameStart);

    // The random speed spread takes one draw for the magnitude and one for the sign. The 0.01 test
    // is a double on purpose.
    float fSpread = m_pEmitter->m_fRandVel;
    if (fSpread > 0.01)
    {
        int n = rand_wincompatible() % static_cast<int>(fSpread * 100.0f);
        n = rand_wincompatible() % 2 > 0 ? -n : n;
        fSpread = static_cast<float>(n) * 0.01f;
    }
    else
        fSpread = 0.0f;

    int nX = static_cast<int>(m_pEmitter->m_fXSize * 100.0f * fScale);
    int nZ = static_cast<int>(m_pEmitter->m_fYSize * 100.0f * fScale);
    if (nX != 0 || nZ != 0)
        randomPosition(nX, nZ);
    else
        m_vPosition = m_pEmitter->m_avVectors[6];

#if KOTOR1  // K1 lifts every new particle 0.06 up
    static Vector s_vLift(0.0f, 0.0f, 0.06f);
    m_vPosition = m_vPosition + s_vLift;
#endif
    m_vTrailPosition = m_vPosition;

    MdlNodeEmitter* pNode = static_cast<MdlNodeEmitter*>(m_pEmitter->m_pNode);
    if (pNode->m_flags.m_bInheritingVelocity)
        inheritVelocity(fDeltaT);

    if (m_pEmitter->m_fSpread != 0.0f)
        randomDirection(nX, nZ);

    // The particle starts out facing along the emitter's Z axis.
    Matrix mat = m_qOrientation.getmatrix();
    m_vDirection = Vector(mat.m_f[6], mat.m_f[7], mat.m_f[8]);
    m_vSpreadVelocity = m_vDirection * (fSpread + m_pEmitter->m_fVelocity);
}

void Particle::randomPosition(int nA, int nB)
{
    // The two draws multiply into the magnitude, halved before the modulo.
    int nA2 = 0;
    if (nA >= 1)
        nA2 = (rand_wincompatible() * rand_wincompatible()) % static_cast<int>(static_cast<float>(nA) * 0.5f);
    int nB2 = 0;
    if (nB >= 1)
        nB2 = (rand_wincompatible() * rand_wincompatible()) % static_cast<int>(static_cast<float>(nB) * 0.5f);

    nA2 = rand_wincompatible() % 2 > 0 ? -nA2 : nA2;
    nB2 = rand_wincompatible() % 2 > 0 ? -nB2 : nB2;

    // 0.0001f is the engine's position-spread quantum.
    m_vPosition = m_pEmitter->m_avVectors[6]
                + m_pEmitter->m_avVectors[1] * (static_cast<float>(nA2) * 0.0001f)
                + m_pEmitter->m_avVectors[2] * (static_cast<float>(nB2) * 0.0001f);
}

void Particle::inheritVelocity(float fDeltaT)
{
    PartEmitter* pE = m_pEmitter;
    // 1/dT, but 0.01f when fDeltaT <= 0.
    float fScale = fDeltaT > 0.0f ? 1.0f / fDeltaT : 0.01f;

    Vector vA = pE->m_avVectors[7];
    Vector vB = pE->m_avVectors[6];
    Vector vD = pE->m_avVectors[3];
    Vector vDelta = vB - m_vPosition;
    float fDist = __builtin_sqrtf(vDelta.x * vDelta.x
                                + vDelta.y * vDelta.y
                                + vDelta.z * vDelta.z);

    // The emitter's drift is its axis-relative offset over the travel distance, plus its own frame
    // displacement, both per second.
    Matrix mat = pE->m_qPrevWorldOrientation.getmatrix();
    Vector vAxis(mat.m_f[6], mat.m_f[7], mat.m_f[8]);
    Vector v = (vD - vAxis) * fDist * fScale + (vB - vA) * fScale;

    m_vVelocity = v;
    float fMag = __builtin_sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    m_fSpeed = fMag;
    if (fMag != 0.0f)
        m_vVelocity = v * (1.0f / fMag);
    else
        m_vVelocity = Vector(0.0f, 0.0f, 0.0f);
}

void Particle::randomDirection(int nA, int nB)
{
    K2_UNUSED(nA); K2_UNUSED(nB);
    // Both int arguments are unused by the body; they stay on the signature only.

    // The direction from the camera to this particle.
    Vector vDir = normalize(m_vPosition - CurrentCamera->m_vPosition);

    // Half of the emitter's cone spread, in radians, converted to hundredths below.
    float fSpread = m_pEmitter->m_fSpread * 0.5f;

    // Two random spins, a Y-axis tilt of up to half the cone spread, then a roll over a full turn
    // (628 hundredths of a radian).
    float fTilt = static_cast<float>(rand_wincompatible() % (static_cast<int>(fSpread * 100.0f) + 1)) * 0.01f;
    Quaternion qTilt(m_pEmitter->m_avVectors[1], fTilt);
    m_qOrientation = qTilt * m_qOrientation;

    float fRoll = static_cast<float>(rand_wincompatible() % 628) * 0.01f;
    Quaternion qRoll(m_pEmitter->m_avVectors[3], fRoll);
    m_qOrientation = qRoll * m_qOrientation;

    // The node's dead space; zero means no aim pass at all.
    float fNode = static_cast<MdlNodeEmitter*>(m_pEmitter->m_pNode)->m_fDeadSpace;
    if (fNode == 0.0f)
        return;

    // The orientation's row 1 (the up axis) is the aim target. The loop spins the particle about
    // cross(vDir, up) by the angle still missing from the cone.
    Matrix mat = m_qOrientation.getmatrix();
    Vector vUp(mat.m_f[3], mat.m_f[4], mat.m_f[5]);
    float fDot = vDir.x * vUp.x + vDir.y * vUp.y + vDir.z * vUp.z;

    // The cosine here is evaluated in double precision; the angle arms below use float.
    Quaternion qResult = m_qOrientation;
    while ((fDot < 0.0f ? -fDot : fDot)
               < cos(1.57079632 - static_cast<MdlNodeEmitter*>(m_pEmitter->m_pNode)->m_fDeadSpace))
    {
        Matrix matUp = m_qOrientation.getmatrix();
        Vector vTurnUp(matUp.m_f[3], matUp.m_f[4], matUp.m_f[5]);
        float fAngle = fDot < 0.0f ? fNode + 1.5707964f - fDot
                                   : fDot - 1.5707964f + fNode;
        Quaternion qTurn(cross(vDir, vTurnUp), fAngle);
        qResult = qTurn * m_qOrientation;

        Matrix matNew = qResult.getmatrix();
        float fNewDot = vDir.x * matNew.m_f[3] + vDir.y * matNew.m_f[4] + vDir.z * matNew.m_f[5];
        fDot = fNewDot < 0.0f ? -fNewDot : fNewDot;
    }
    m_qOrientation = qResult;
}

#include "port/libcbase.h"

#if KOTOR1  // K1: the model streams are SDL_RWops
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif

#if KOTOR1 && !defined(_MSC_VER)  // K1 inlines this ctor into the three Update bodies; `used` keeps the exported copy
inline __attribute__((used))
#endif
ChunkyParticle::ChunkyParticle(PartEmitter* pEmitter, float fDeltaT) : Particle(pEmitter, fDeltaT)
{
    if (pEmitter)
    {
        m_pAurObject = NewCAurObject(
            static_cast<MdlNodeEmitter*>(pEmitter->m_pNode)->m_szChunkName, const_cast<char*>(""), 0, 0);
        if (m_pAurObject)
            m_pAurObject->AttachToScene(currentscene);
    }
}

ChunkyParticle::~ChunkyParticle()
{
    if (m_pAurObject)
    {
        delete m_pAurObject;
        m_pAurObject = 0;
    }
}


ChunkyParticle* Particle::AsChunkyParticle()
{
    return NULL;
}

ChunkyParticle* ChunkyParticle::AsChunkyParticle()
{
    return this;
}

void ChunkyParticle::initialize(float fA, float fB)
{
    // The base particle is re-seeded at unit scale; the scale argument goes to the aurora object.
    Particle::initialize(fA, 1.0f);
    if (m_pAurObject)
    {
        m_pAurObject->AttachToScene(currentscene);
        m_pAurObject->SetObjectScale(fB, true);
        // Returns PlayAnimation's int result.
        m_pAurObject->PlayAnimation(const_cast<char*>("impact"), 1.0f, 1, 0.0f);
    }
}

LightningEmitter* LightningEmitter::AsLightningEmitter() { return this; }

// MdlNodeEmitter is the particle-emitter flavor of a parsed .mdl node. This file hosts the node's
// parser-table trampolines and the one-line accessors over its flag word.

extern template SafePointer<CAurObject>& SafePointer<CAurObject>::operator=(CAurObject*);

void MdlNodeEmitterParseField(void* pv, char* pszLine)
{
    static_cast<MdlNodeEmitter*>(pv)->InternalParseField(pszLine);
}

void MdlNodeEmitterPostProcess(void* pv)
{
    static_cast<MdlNodeEmitter*>(pv)->InternalPostProcess();
}

void MdlNodeEmitterControl(void* pv, Part* pPart, float fA, float fB, float fC)
{
    static_cast<MdlNodeEmitter*>(pv)->InternalControl(pPart, fA, fB, fC);
}

Part* MdlNodeEmitterCreateInstance(void* pv, Gob* pGob)
{
    return static_cast<MdlNodeEmitter*>(pv)->InternalCreateInstance(pGob);
}

// The name parameter is never read; the node's name byte is cleared instead.

#if !KOTOR1
MdlNodeEmitter* MdlNodeEmitterConJoinTwinMysLexia(void* pv)
{
#ifdef K2_FIX_ENGINE_BUGS
    char* pszName = static_cast<char*>(pv);
#else
    K2_UNUSED(pv);
    char* pszName;
#endif
    return new MdlNodeEmitter(pszName);
}
#endif

void SetEmitterTarget(CAurObject* pObject, char* pszName, void* pContext)
{
    K2_UNUSED(pObject); K2_UNUSED(pszName);
    PartEmitter* pEmitter = static_cast<PartEmitter*>(pContext);

    for (int i = 0; i < pEmitter->m_nChildren; i++)
    {
        Part* pChild = pEmitter->m_paChildren[i];
        if (pChild->AsPartReference() != 0)
        {
            pEmitter->m_spTarget = pChild->AsPartReference()->m_pObject;

            Vector vWorld;
            Quaternion qWorld;
            pEmitter->GetWorld(vWorld, qWorld);

            if (pEmitter->m_spTarget.m_pObject != 0)
                pEmitter->m_fRadius =
                    magnitude(pEmitter->m_spTarget.m_pObject->GetPosition() - vWorld);
            else
                pEmitter->m_fRadius = 0.0f;
            return;
        }
    }

    pEmitter->m_pGob->DeregisterCallback(pEmitter->m_pTargetCallBack);
}


// Part lookup by name, shared with Gob.cpp and GobAnimation.cpp.
Part* FindPart(Part* pRoot, char* pszName);

CAurObject* GetEmitterTarget(CAurObject* pObject, char* pszName)
{
    if (pObject == 0)
        return 0;

    Part* pRoot = static_cast<Gob*>(pObject)->m_pPart;
    if (pRoot == 0)
        return 0;

    if (pszName != 0)
    {
        Part* pFound = FindPart(pRoot, pszName);
        if (pFound == 0)
            return 0;
        if (pFound->AsPartEmitter() != 0)
            return pFound->AsPartEmitter()->m_spTarget;
        return 0;
    }

    List<Part*> searchParts;
    searchParts.Add(pRoot);
    while (searchParts.m_count != 0)
    {
        Part* pSearch = searchParts.m_pData[--searchParts.m_count];
        if (pSearch->AsPartEmitter() != 0)
            return pSearch->AsPartEmitter()->m_spTarget;
        for (int i = 0; i < pSearch->m_nChildren; i++)
            searchParts.Add(pSearch->m_paChildren[i]);
    }
    return 0;
}

void Detonate(CAurObject* pObject, char* pszName, void* pContext)
{
    K2_UNUSED(pObject); K2_UNUSED(pszName);
    static_cast<PartEmitter*>(pContext)->m_bDetonate = true;
}


bool MdlNodeEmitter::IsPointToPoint() { return m_flags.m_bPointToPoint; }

bool MdlNodeEmitter::IsPointToPointSel() { return m_flags.m_bPointToPointSel; }

bool MdlNodeEmitter::IsAffectedByWind() { return m_flags.m_bAffectedByWind; }

bool MdlNodeEmitter::IsTinted() { return m_flags.m_bTinted; }

bool MdlNodeEmitter::IsBouncing() { return m_flags.m_bBouncing; }

bool MdlNodeEmitter::IsRandom() { return m_flags.m_bRandom; }

bool MdlNodeEmitter::IsRandomOrder() { return m_flags.m_bRandomOrder; }

bool MdlNodeEmitter::IsInheriting() { return m_flags.m_bInheriting; }

bool MdlNodeEmitter::IsInheritingVelocity() { return m_flags.m_bInheritingVelocity; }

bool MdlNodeEmitter::IsInheritingLocal() { return m_flags.m_bInheritingLocal; }

bool MdlNodeEmitter::IsSplat() { return m_flags.m_bSplat; }

bool MdlNodeEmitter::IsInheritingPart() { return m_flags.m_bInheritingPart; }

bool MdlNodeEmitter::UsesDepthTexture() { return false; }

void MdlNodeEmitter::EnablePointToPoint() { m_flags.m_bPointToPoint = true; }

void MdlNodeEmitter::EnablePointToPointSel() { m_flags.m_bPointToPointSel = true; }

void MdlNodeEmitter::EnableAffectedByWind() { m_flags.m_bAffectedByWind = true; }

void MdlNodeEmitter::EnableTinted() { m_flags.m_bTinted = true; }

void MdlNodeEmitter::EnableBouncing() { m_flags.m_bBouncing = true; }

void MdlNodeEmitter::EnableRandom() { m_flags.m_bRandom = true; }

void MdlNodeEmitter::EnableRandomOrder() { m_flags.m_bRandomOrder = true; }

void MdlNodeEmitter::EnableInheriting() { m_flags.m_bInheriting = true; }

void MdlNodeEmitter::EnableInheritingVelocity() { m_flags.m_bInheritingVelocity = true; }

void MdlNodeEmitter::EnableInheritingLocal() { m_flags.m_bInheritingLocal = true; }

void MdlNodeEmitter::EnableSplat() { m_flags.m_bSplat = true; }

void MdlNodeEmitter::EnableInheritingPart() { m_flags.m_bInheritingPart = true; }

void MdlNodeEmitter::EnableDepthTexture() { m_flags.m_bDepthTexture = true; }
