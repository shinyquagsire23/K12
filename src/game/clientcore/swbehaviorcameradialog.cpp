#define K1_AURORA_LIST  // K1: the hit-check ignore lists grow from 8
#include "serverexoappinternal.h"
#include "aurscene.h"
#include "nwcobject.h"
#include "nwccreature.h"
#include "nwcplaceable.h"
#include "nwcdoor.h"
#include "nwcanimbase.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "Vector.h"
#include "Quaternion.h"
#include "include/aurbehavior.h"

// The renderer's "skip the matrix build when the rotation is a no-op" switch, and the three in-game
// shot tweaks, which are defined elsewhere.
extern int quaternionidentityoptimization;
extern float AspyrHackCameraFocus;
extern float AspyrHackCameraDistance;
extern float AspyrHackCameraYaw;

// Rotates a vector by a quaternion, skipping the work when the rotation is the identity.
static inline Vector RotateByQuaternion(const Vector& v, const Quaternion& q)
{
    float w = q.w;
    if (quaternionidentityoptimization != 0 && (w == 1.0f || w == -1.0f))
        return v;

    float x = q.x, y = q.y, z = q.z;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = x * w, wy = y * w, wz = z * w;
    Vector r;
    r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) +
          v.z * ((xz + wy) + (xz + wy));
    r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) +
          v.z * ((yz - wx) + (yz - wx));
    r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) +
          v.z * (1.0f - ((xx + yy) + (xx + yy)));
    return r;
}

// This behavior ignores every scripted message.
void CSWBehaviorCameraDialog::Message(char* pszParam)
{
    K2_UNUSED(pszParam);
}

// The behavior-type id the module looks up.
int CSWBehaviorCameraDialog::GetType()
{
    return BEHAVIOR_CAMERA_DIALOG;
}

CSWBehaviorCameraDialog::~CSWBehaviorCameraDialog()
{
}

CSWBehaviorCameraDialog::CSWBehaviorCameraDialog(CAurObject* pTarget, int nParam)
    : CAurBehavior(pTarget)
    , m_fHookHeadHeight(0.0f), m_fTargetHeadHeight(0.0f)
    , m_fCamHeightOffset(0), m_fTarHeightOffset(0), m_fPullBackRange(0.0f)
    , m_fShakeYaw(0.0f), m_fShakePitch(0.0f)
    , m_bShotObstructed(0), m_nParam(nParam), m_bRunBaseControl(1)
    , m_vShotPos(0.0f, 0.0f, 0.0f)
    , m_qShotRot(1.0f, 0.0f, 0.0f, 0.0f)
    , m_pShotHook(), m_pShotTarget()
{
    for (int i = 0; i < 4; i++) {
        m_slots[i].m_a = 0xffffffff;
        m_slots[i].m_b = 0xffffffff;
        m_slots[i].m_side = 0;
    }
}

// Records the line of action between two participants. The four slots are a small cache: the first
// free one (side zero) is filled, an already-recorded pair in either order short-circuits into
// GetLineOfActionSide, and when all four are taken slot 0 is overwritten.
int CSWBehaviorCameraDialog::AddCalculatedLineOfAction(unsigned long long a, unsigned long long b)
{
    int i = 0;
    for (int j = 0; j < 4; j++) {
        if (m_slots[j].m_side == 0) {
            i = j;
            break;
        }
        if ((m_slots[j].m_b == a && m_slots[j].m_a == b) ||
            (m_slots[j].m_b == b && m_slots[j].m_a == a)) {
            if (m_nParam == 0)
                return GetLineOfActionSide(a, b);
            return m_slots[j].m_side;
        }
    }
    m_slots[i].m_side = GetSideUnobstructed();
    m_slots[i].m_a = b;
    m_slots[i].m_b = a;
    return m_slots[i].m_side;
}

// Places the owner at the shot's cached position and aims it, rebuilding the shot's framing first
// when it is not a cached shot yet. Returns 1 on every path, including the early outs.
int CSWBehaviorCameraDialog::Control(float fParam)
{
    if (m_bRunBaseControl == 1 && CAurBehavior::Control(fParam) == 0)
        return 1;

    int nType = m_nShotType;
    int nSide K2_UNSET;
    if (nType == 6) {
    } else {
        if (m_pShotHook.m_pObject == 0 || m_pShotTarget.m_pObject == 0)
            return 1;
        nSide = GetLineOfActionSide(m_nLineOfActionA, m_nLineOfActionB);
        if (nSide == 0)
            return 1;
    }

    Vector vPos = m_vShotPos;
    Quaternion qRot = m_qShotRot;

    if (m_nParam != 0) {
        // For the two-participant shot, the objects' own origins frame it.
        if (nType != 6)
            GetCameraParameters(nType, nSide, m_pShotHook.m_pObject->GetPosition(),
                                m_pShotTarget.m_pObject->GetPosition(), vPos, qRot);
    } else if (nType != 6) {
        Vector vOut;
        Quaternion qOut;
        Vector vHook = m_pShotHook.m_pObject->GetPosition();
        Vector vTarget = m_pShotTarget.m_pObject->GetPosition();
        vHook.z = vHook.z + 0.1f + m_fHookHeadHeight;
        vTarget.z = vTarget.z + 0.1f + m_fTargetHeadHeight;
        if (m_pShotHook.m_pObject->GetPartPosition(const_cast<char*>("CAMERAHOOK"), &vOut, &qOut) != 0)
            vHook = vOut;
        if (m_pShotTarget.m_pObject->GetPartPosition(const_cast<char*>("CAMERAHOOK"), &vOut, &qOut) != 0)
            vTarget = vOut;
        GetCameraParameters(m_nShotType, nSide, vHook, vTarget, vPos, qRot);
    }

    m_pOwner->SetPosition(vPos);
    qRot = YawPitchRoll(Yaw(qRot) + m_fShakeYaw, Pitch(qRot) + m_fShakePitch, Roll(qRot));
    m_pOwner->SetOrientation(qRot);
    return 1;
}

// Which side of the cached line of action a and b are on: 0 when the pair is not in the cache (the
// first slot whose side is unset ends the search, since AddCalculatedLineOfAction fills from the
// first free slot), otherwise the record's own side when the pair is queried in the stored order and
// the flipped side when it is queried reversed.
int CSWBehaviorCameraDialog::GetLineOfActionSide(unsigned long long a, unsigned long long b)
{
    for (int i = 0; i < 4; i++) {
        if (m_slots[i].m_side == 0)
            return 0;
        if (m_slots[i].m_b == a && m_slots[i].m_a == b)
            return m_slots[i].m_side;
        if (m_slots[i].m_b == b && m_slots[i].m_a == a)
            return m_slots[i].m_side == 2 ? 1 : 2;
    }
    return 0;
}

// Turns a pair of camera points (the hook and the target) into the camera's position and aim (the
// framing pass), for one shot kind and one side of the line of action. The shot-kind tables are per
// type.
void CSWBehaviorCameraDialog::GetCameraParameters(unsigned int nType, unsigned int nSide,
                                                  Vector vA, Vector vB,
                                                  Vector& vOut, Quaternion& qOut)
{
    Vector vDir(0.0f, 0.0f, 0.0f);

    if (equal(vA, vB)) {
        vB = Vector(vA.x + 1.5f, vA.y + 1.5f, vA.z + 0.1f);
        m_bShotObstructed = 1;
    }

    float fFocusDef;
    float fDistDef;
    float fYawDef = 30.0f;
    float fHookZMag;
    float fTargetZMag;
    float fExtraZMag;
    float fOutZMag;
    if (nType == 3) {
        fTargetZMag = 0.0f;
        fFocusDef = 0.5f;
        fDistDef = 1.5f;
        fExtraZMag = -0.2f;
        fOutZMag = 0.3f;
        fYawDef = 90.0f;
        fHookZMag = 0.0f;
    } else if (nType == 2) {
        fHookZMag = -0.1f;
        fTargetZMag = -0.05f;
        fFocusDef = 0.3f;
        fDistDef = 0.8f;
        fOutZMag = 0.0f;
        fExtraZMag = 0.0f;
    } else {
        fFocusDef = 1.0f;
        fOutZMag = 0.0f;
        fDistDef = 0.5f;
        fExtraZMag = 0.0f;
        fTargetZMag = 0.0f;
        fHookZMag = 0.0f;
    }

#if KOTOR1  // no Aspyr shot overrides in K1
    float fFocus = fFocusDef;
    float fDist = fDistDef;
    float fYaw = fYawDef;
#else
    float fFocus = (AspyrHackCameraFocus != 0.0f) ? AspyrHackCameraFocus : fFocusDef;
    float fDist = (AspyrHackCameraDistance != 0.0f) ? AspyrHackCameraDistance : fDistDef;
    float fYaw = (AspyrHackCameraYaw != 0.0f) ? AspyrHackCameraYaw : fYawDef;
#endif
    float fYawSign = (nSide == 2) ? -fYaw : fYaw;

    Vector vBase;
    float fBaseZ = 0.0f;

    if (m_nParam == 0) {
        if (nType == 1 || m_bShotObstructed == 1) {
            float fAngle = (nSide == 2) ? -30.0f : 30.0f;
            vB.z = vA.z;
            Vector vN = normalize(Vector(vA.x - vB.x, vA.y - vB.y, vA.z - vB.z));
            vBase.x = vA.x + vN.x * -0.2f;
            vBase.y = vA.y + vN.y * -0.2f;
            fBaseZ = (vA.z + vN.z * -0.2f) - 0.04f + *reinterpret_cast<float*>(&m_fTarHeightOffset);
            Quaternion q = YawPitchRoll(fAngle, 0.0f, 0.0f);
            Vector vR = RotateByQuaternion(Vector(vA.x - vB.x, vA.y - vB.y, vA.z - vB.z), q);
            vDir = normalize(vR);
            float fZOff = 0.0f;
            if (m_bShotObstructed == 0)
                fZOff = m_fPullBackRange;
            float fZAdj = fZOff * 0.2f;
            fBaseZ = fBaseZ - fZAdj;
            vDir = normalize(vDir);
            float fScale = fZOff + 0.5f;
            vOut.x = vBase.x - vDir.x * fScale;
            vOut.y = vBase.y - vDir.y * fScale;
            vOut.z = (fBaseZ - vDir.z * fScale) + 0.0f + *reinterpret_cast<float*>(&m_fCamHeightOffset);
            vOut.z = fZAdj + vOut.z;
        } else {
            Vector vDelta(vB.x - vA.x, vB.y - vA.y, vB.z - vA.z);
            float fMag = magnitude(vDelta);
            float fHookM = fHookZMag * fMag;
            float fTargetM = fTargetZMag * fMag;
            float fExtraM = fExtraZMag * fMag;
            float fOutM = fOutZMag * fMag;
            float fCamHeightOffset = *reinterpret_cast<float*>(&m_fCamHeightOffset);
            float fTarHeightOffset = *reinterpret_cast<float*>(&m_fTarHeightOffset);
            float fExtraZ = fExtraM + fTarHeightOffset;
            float fVbZ = fTargetM + vB.z;
            vA.z = fHookM + vA.z;
            vB.z = fVbZ;
            float fBaseX = vB.x + fFocus * (vA.x - vB.x);
            float fBaseY = vB.y + fFocus * (vA.y - vB.y);
            fBaseZ = fVbZ + fFocus * (vA.z - vB.z);
            fBaseZ = fExtraZ + fBaseZ;
            Quaternion q = YawPitchRoll(fYawSign, 0.0f, 0.0f);
            Vector vR = RotateByQuaternion(Vector(vA.x - vB.x, vA.y - vB.y, vA.z - vB.z), q);
            vDir = vR;
            vBase.x = fBaseX;
            vBase.y = fBaseY;
            float fAccum = fOutM + fCamHeightOffset;
            if (nType == 2) {
                Vector vN2 = normalize(vR);
                float fSx = fDist * vR.x;
                float fSy = fDist * vR.y;
                float fSz = fDist * vR.z;
                float f0 = fSx + vN2.x * 0.15f;
                float f1 = fSy + vN2.y * 0.15f;
                float f2 = fSz + vN2.z * 0.15f;
                vOut.x = vBase.x - f0;
                vOut.y = vBase.y - f1;
                vOut.z = fBaseZ - f2;
            } else if (nType == 3) {
                float fSx = fDist * vR.x;
                float fSy = fDist * vR.y;
                float fSz = fDist * vR.z;
                vOut.x = vBase.x - fSx;
                vOut.y = vBase.y - fSy;
                vOut.z = fBaseZ - fSz;
            }
            vOut.z = fAccum + vOut.z;
        }
    } else {
        float fHookZ = m_fHookHeadHeight;
        if (nType == 1) {
            float fAngle = (nSide == 2) ? -30.0f : 30.0f;
            vA.z = fHookZ + vA.z;
            vB.z = fHookZ + vB.z;
            Vector vN = normalize(Vector(vA.x - vB.x, vA.y - vB.y, vA.z - vB.z));
            vBase.x = vA.x + vN.x * -0.2f;
            vBase.y = vA.y + vN.y * -0.2f;
            fBaseZ = (vA.z + vN.z * -0.2f) - 0.04f + *reinterpret_cast<float*>(&m_fTarHeightOffset);
            Quaternion q = YawPitchRoll(fAngle, 0.0f, 0.0f);
            Vector vR = RotateByQuaternion(Vector(vA.x - vB.x, vA.y - vB.y, vA.z - vB.z), q);
            vDir = normalize(vR);
            vDir = normalize(vDir);
            vOut.x = vBase.x + vDir.x * -0.5f;
            vOut.y = vBase.y + vDir.y * -0.5f;
            vOut.z = (fBaseZ + vDir.z * -0.5f) + 0.0f + *reinterpret_cast<float*>(&m_fCamHeightOffset);
        } else {
            Vector vDelta(vB.x - vA.x, vB.y - vA.y, vB.z - vA.z);
            float fMag = magnitude(vDelta);
            float fHookM = fHookZMag * fMag;
            float fTargetM = fTargetZMag * fMag;
            float fExtraM = fExtraZMag * fMag;
            float fOutM = fOutZMag * fMag;
            float fCamHeightOffset = *reinterpret_cast<float*>(&m_fCamHeightOffset);
            float fTarHeightOffset = *reinterpret_cast<float*>(&m_fTarHeightOffset);
            float fExtraZ = fExtraM + fTarHeightOffset;
            float fVbZ = fTargetM + (m_fTargetHeadHeight + vB.z);
            vA.z = fHookM + (fHookZ + vA.z);
            vB.z = fVbZ;
            float fBaseX = vB.x + fFocus * (vA.x - vB.x);
            float fBaseY = vB.y + fFocus * (vA.y - vB.y);
            fBaseZ = fVbZ + fFocus * (vA.z - vB.z);
            fBaseZ = fExtraZ + fBaseZ;
            Quaternion q = YawPitchRoll(fYawSign, 0.0f, 0.0f);
            Vector vR = RotateByQuaternion(Vector(vA.x - vB.x, vA.y - vB.y, vA.z - vB.z), q);
            vDir = vR;
            vBase.x = fBaseX;
            vBase.y = fBaseY;
            float fAccum = fOutM + fCamHeightOffset;
            if (nType == 2) {
                Vector vN2 = normalize(vR);
                float fSx = fDist * vR.x;
                float fSy = fDist * vR.y;
                float fSz = fDist * vR.z;
                float f0 = fSx + vN2.x * 0.15f;
                float f1 = fSy + vN2.y * 0.15f;
                float f2 = fSz + vN2.z * 0.15f;
                vOut.x = vBase.x - f0;
                vOut.y = vBase.y - f1;
                vOut.z = fBaseZ - f2;
            } else if (nType == 3) {
                float fSx = fDist * vR.x;
                float fSy = fDist * vR.y;
                float fSz = fDist * vR.z;
                vOut.x = vBase.x - fSx;
                vOut.y = vBase.y - fSy;
                vOut.z = fBaseZ - fSz;
            }
            vOut.z = fAccum + vOut.z;
        }
        if (m_bShotObstructed != 0) {
            Vector vHitPoint(0.0f, 0.0f, 0.0f);
            CHitInfo hit(0, ~CHitInfo::kMaterialTrigger, 0);
            hit.m_bSkipObjects = 0;
            hit.m_ignoreList.Add(m_pShotHook.m_pObject);
            hit.m_ignoreList.Add(m_pShotTarget.m_pObject);
            CSWCObject* pObj =
                static_cast<CSWCObject*>(g_pAppManager->m_pClientExoApp->GetGameObject(m_nLineOfActionA));
            if (pObj != 0 && pObj->AsSWCObject() != 0 &&
                static_cast<CSWCObject*>(pObj->AsSWCObject())->GetAnimationBase() != 0) {
                Gob* pModel =
                    static_cast<CSWCObject*>(pObj->AsSWCObject())->GetAnimationBase()
                        ->GetModel(CSWCObject::ANIM_MODEL_HEAD);
                if (pModel != 0)
                    hit.m_ignoreList.Add(static_cast<CAurObject*>(pModel));
            }
            pObj = static_cast<CSWCObject*>(g_pAppManager->m_pClientExoApp->GetGameObject(m_nLineOfActionB));
            if (pObj != 0 && pObj->AsSWCObject() != 0 &&
                static_cast<CSWCObject*>(pObj->AsSWCObject())->GetAnimationBase() != 0) {
                Gob* pModel =
                    static_cast<CSWCObject*>(pObj->AsSWCObject())->GetAnimationBase()
                        ->GetModel(CSWCObject::ANIM_MODEL_HEAD);
                if (pModel != 0)
                    hit.m_ignoreList.Add(static_cast<CAurObject*>(pModel));
            }
            if (m_pOwner->GetScene()->HitCheck(Vector(vBase.x, vBase.y, fBaseZ), vOut, &vHitPoint,
                                              &hit) != 0) {
                Vector vAim(vHitPoint.x - vBase.x, vHitPoint.y - vBase.y, vHitPoint.z - fBaseZ);
                vAim = normalize(vAim);
                vOut.x = vHitPoint.x + vAim.x * -0.1f;
                vOut.y = vHitPoint.y + vAim.y * -0.1f;
                vOut.z = vHitPoint.z + vAim.z * -0.1f;
            }
        }
    }

    Vector vDirN = normalize(vDir);
    Vector vAim(vBase.x - vOut.x, vBase.y - vOut.y, fBaseZ - vOut.z);
    qOut = YawPitchRoll(Yaw(vDirN), Pitch(normalize(vAim)) + 90.0f, 0.0f);
}

// Which side of the line of action the shot is unobstructed from: 2 when side 2's probe from hook to
// target sees no more obstacles than side 1's, otherwise 1. The probe is a small family of rays (the
// two camera points, their midpoint, six framing passes and ten hit checks), and its answer is cached
// per participant pair by AddCalculatedLineOfAction.
//
// The two participant blocks are not looped, since each names its own class's camera follow part.
int CSWBehaviorCameraDialog::GetSideUnobstructed()
{
    int nA = 0;
    int nB = 0;
    // The GetPartPosition out-parameters; the quaternion doubles as the framing call's aim out-param.
    Vector vPart(0.0f, 0.0f, 0.0f);
    Quaternion qPart(1.0f, 0.0f, 0.0f, 0.0f);

    Vector vHook = m_pShotHook.m_pObject->GetPosition();
    Vector vTarget = m_pShotTarget.m_pObject->GetPosition();
    float fHookAdj = m_fHookHeadHeight;
    float fTargetAdj = m_fTargetHeadHeight;
    float fHookZ = vHook.z + 0.1f;
    float fTargetZ = vTarget.z + 0.1f;

    Vector vHookPos = vHook;
    Vector vTargetPos = vTarget;
    if (m_nParam != 0) {
        vHookPos.z = fHookZ;
        vTargetPos.z = fTargetZ;
    } else {
        if (m_pShotHook.m_pObject->GetPartPosition(const_cast<char*>("CAMERAHOOK"), &vPart, &qPart) != 0)
            vHookPos = vPart;
        else
            vHookPos.z = fHookZ + fHookAdj;
        if (m_pShotTarget.m_pObject->GetPartPosition(const_cast<char*>("CAMERAHOOK"), &vPart, &qPart) != 0)
            vTargetPos = vPart;
        else
            vTargetPos.z = fTargetZ + fTargetAdj;
    }

    Vector vMid;
    vMid.x = vHook.x + (vTarget.x - vHook.x) * 0.5f;
    vMid.y = vHook.y + (vTarget.y - vHook.y) * 0.5f;
    vMid.z = (fHookZ + (fTargetZ - fHookZ) * 0.5f) +
             (fHookAdj + (fHookAdj - fTargetAdj) * 0.5f);

    Vector vDir(0.0f, 0.0f, 0.0f); // GetCameraParameters' framing direction out-param
    Vector vOut(0.0f, 0.0f, 0.0f); // the probe's hit point out-param
    CHitInfo hit(0, ~CHitInfo::kMaterialTrigger, 0);
    hit.m_bSkipObjects = 0;
    hit.m_ignoreList.Add(m_pShotHook.m_pObject);
    hit.m_ignoreList.Add(m_pShotTarget.m_pObject);

    Gob* pModel;
    CSWCObject* pObj = static_cast<CSWCObject*>(g_pAppManager->m_pClientExoApp->GetGameObject(m_nLineOfActionA));
    if (pObj != 0 && pObj->AsSWCObject() != 0 &&
        static_cast<CSWCObject*>(pObj->AsSWCObject())->GetAnimationBase() != 0) {
        pModel = static_cast<CSWCObject*>(pObj->AsSWCObject())->GetAnimationBase()
                     ->GetModel(CSWCObject::ANIM_MODEL_HEAD);
        if (pModel != 0)
            hit.m_ignoreList.Add(static_cast<CAurObject*>(pModel));
    }
    pObj = static_cast<CSWCObject*>(g_pAppManager->m_pClientExoApp->GetGameObject(m_nLineOfActionB));
    if (pObj != 0 && pObj->AsSWCObject() != 0 &&
        static_cast<CSWCObject*>(pObj->AsSWCObject())->GetAnimationBase() != 0) {
        pModel = static_cast<CSWCObject*>(pObj->AsSWCObject())->GetAnimationBase()
                     ->GetModel(CSWCObject::ANIM_MODEL_HEAD);
        if (pModel != 0)
            hit.m_ignoreList.Add(static_cast<CAurObject*>(pModel));
    }

    // For side 1, extra sight lines only exist for a single-participant shot.
    GetCameraParameters(3, 2, vHookPos, vTargetPos, vDir, qPart);
    nA += m_pOwner->GetScene()->HitCheck(vMid, vDir, &vOut, &hit);
    hit.ClearOutput();
    if (m_nParam == 0) {
        nA += m_pOwner->GetScene()->HitCheck(vHookPos, vDir, &vOut, &hit);
        hit.ClearOutput();
        nA += m_pOwner->GetScene()->HitCheck(vTargetPos, vDir, &vOut, &hit);
        hit.ClearOutput();
    }
    GetCameraParameters(2, 2, vHookPos, vTargetPos, vDir, qPart);
    nA += m_pOwner->GetScene()->HitCheck(vHookPos, vDir, &vOut, &hit);
    hit.ClearOutput();
    GetCameraParameters(2, 1, vTargetPos, vHookPos, vDir, qPart);
    nA += m_pOwner->GetScene()->HitCheck(vTargetPos, vDir, &vOut, &hit);
    hit.ClearOutput();

    // Side 2 runs the same probe the other way round, with the a/b argument pairs transposed.
    GetCameraParameters(3, 1, vHookPos, vTargetPos, vDir, qPart);
    nB += m_pOwner->GetScene()->HitCheck(vMid, vDir, &vOut, &hit);
    hit.ClearOutput();
    if (m_nParam == 0) {
        nB += m_pOwner->GetScene()->HitCheck(vHookPos, vDir, &vOut, &hit);
        hit.ClearOutput();
        nB += m_pOwner->GetScene()->HitCheck(vTargetPos, vDir, &vOut, &hit);
        hit.ClearOutput();
    }
    GetCameraParameters(2, 1, vHookPos, vTargetPos, vDir, qPart);
    nB += m_pOwner->GetScene()->HitCheck(vHookPos, vDir, &vOut, &hit);
    hit.ClearOutput();
    GetCameraParameters(2, 2, vTargetPos, vHookPos, vDir, qPart);
    nB += m_pOwner->GetScene()->HitCheck(vTargetPos, vDir, &vOut, &hit);
    hit.ClearOutput();

    return (nA <= nB) ? 2 : 1;
}

// The two-participant shot, reached from CGuiInGame::UpdateShotCamera's non-placeable path. The two
// ids are the participants' game-object ids, the kind is the shot type, the two int references are
// the per-shot camera tweaks and the float is the dialog distance. Everything here is a side effect
// on `this`; nothing is returned.
//
// The class dials run creature, then placeable, then door, and the first non-null one is the object
// the hook hangs off. The distance takes a near-zero dead zone of plus or minus 0.001.
void CSWBehaviorCameraDialog::SetShot(unsigned long long a, unsigned long long b, const int& nType,
                                      const float& c, const float& d, float range)
{
    Vector vPart(0.0f, 0.0f, 0.0f);
    Quaternion qPart(1.0f, 0.0f, 0.0f, 0.0f);
    int nHit;

#ifdef K2_FIX_ENGINE_BUGS
    // A wide shot is kept only while it frames the same two participants; a new speaker (a party
    // interjection) would otherwise stay off screen for every later wide shot.
    if (m_nShotType == 3 && nType == 3 &&
        ((m_nLineOfActionA == a && m_nLineOfActionB == b) ||
         (m_nLineOfActionA == b && m_nLineOfActionB == a)))
        return;
#else
    if (m_nShotType == 3 && nType == 3)
        return;
#endif
    if (nType == 6)
        return;

    m_bShotObstructed = 0;
    if (m_pOwner->AsCAurCamera() != 0)
        static_cast<CAurCamera*>(m_pOwner->AsCAurCamera())->SetViewAngle(55.0f);

    CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(a);
    if (pObj == 0)
        return;
    CSWCCreature* pCreature = pObj->AsSWCCreature();
    CSWCPlaceable* pPlaceable = pObj->AsSWCPlaceable();
    CSWCDoor* pDoor = pObj->AsSWCDoor();
    // The anchor read and the height read appear in all three arms, so the pair is repeated per class
    // rather than assigned once to a chosen pointer.
    if (pCreature != 0) {
        m_pShotHook = static_cast<CAurObject*>(pCreature->GetCameraFollowPart());
        m_fHookHeadHeight = pCreature->GetHeadHeightOffset();
    } else if (pPlaceable != 0) {
        m_pShotHook = static_cast<CAurObject*>(pPlaceable->GetCameraFollowPart());
        m_fHookHeadHeight = pPlaceable->GetHeadHeightOffset();
    } else if (pDoor != 0) {
        m_pShotHook = static_cast<CAurObject*>(pDoor->GetCameraFollowPart());
        m_fHookHeadHeight = pDoor->GetHeadHeightOffset();
    } else {
        return;
    }
    AddDependency(m_pShotHook.m_pObject);

    pObj = g_pAppManager->m_pClientExoApp->GetGameObject(b);
    if (pObj == 0)
        return;
    pCreature = pObj->AsSWCCreature();
    pPlaceable = pObj->AsSWCPlaceable();
    pDoor = pObj->AsSWCDoor();
    if (pCreature != 0) {
        m_pShotTarget = static_cast<CAurObject*>(pCreature->GetCameraFollowPart());
        m_fTargetHeadHeight = pCreature->GetHeadHeightOffset();
    } else if (pPlaceable != 0) {
        m_pShotTarget = static_cast<CAurObject*>(pPlaceable->GetCameraFollowPart());
        m_fTargetHeadHeight = pPlaceable->GetHeadHeightOffset();
    } else if (pDoor != 0) {
        m_pShotTarget = static_cast<CAurObject*>(pDoor->GetCameraFollowPart());
        m_fTargetHeadHeight = pDoor->GetHeadHeightOffset();
    } else {
        return;
    }
    AddDependency(m_pShotTarget.m_pObject);

    m_nLineOfActionA = a;
    m_nLineOfActionB = b;
    m_fCamHeightOffset = *reinterpret_cast<const int*>(&c);
    m_fTarHeightOffset = *reinterpret_cast<const int*>(&d);
    m_fPullBackRange = (range < 0.001f && range > -0.001f) ? 0.0f : range;
    int nSide = AddCalculatedLineOfAction(a, b);

    Vector vDir(0.0f, 0.0f, 0.0f);
    Quaternion qRot(1.0f, 0.0f, 0.0f, 0.0f);
    Vector vOut(0.0f, 0.0f, 0.0f);
    CHitInfo hit(0, ~CHitInfo::kMaterialTrigger, 0);
    hit.m_bSkipObjects = 0;
    hit.m_ignoreList.Add(m_pShotHook.m_pObject);
    hit.m_ignoreList.Add(m_pShotTarget.m_pObject);

    Gob* pModel;
    CSWCObject* pParticipant =
        static_cast<CSWCObject*>(g_pAppManager->m_pClientExoApp->GetGameObject(m_nLineOfActionA));
    if (pParticipant != 0 && pParticipant->AsSWCObject() != 0 &&
        static_cast<CSWCObject*>(pParticipant->AsSWCObject())->GetAnimationBase() != 0) {
        pModel = static_cast<CSWCObject*>(pParticipant->AsSWCObject())
                      ->GetAnimationBase()
                     ->GetModel(CSWCObject::ANIM_MODEL_HEAD);
        if (pModel != 0)
            hit.m_ignoreList.Add(static_cast<CAurObject*>(pModel));
    }
    pParticipant = static_cast<CSWCObject*>(g_pAppManager->m_pClientExoApp->GetGameObject(m_nLineOfActionB));
    if (pParticipant != 0 && pParticipant->AsSWCObject() != 0 &&
        static_cast<CSWCObject*>(pParticipant->AsSWCObject())->GetAnimationBase() != 0) {
        pModel = static_cast<CSWCObject*>(pParticipant->AsSWCObject())
                      ->GetAnimationBase()
                     ->GetModel(CSWCObject::ANIM_MODEL_HEAD);
        if (pModel != 0)
            hit.m_ignoreList.Add(static_cast<CAurObject*>(pModel));
    }

    Vector vHookPos = m_pShotHook.m_pObject->GetPosition();
    Vector vTargetPos = m_pShotTarget.m_pObject->GetPosition();
    vHookPos.z += 0.1f;
    vTargetPos.z += 0.1f;
    Vector vMid;
    vMid.x = vHookPos.x + (vTargetPos.x - vHookPos.x) * 0.5f;
    vMid.y = vHookPos.y + (vTargetPos.y - vHookPos.y) * 0.5f;
    vMid.z = (vHookPos.z + (vTargetPos.z - vHookPos.z) * 0.5f) +
             (m_fHookHeadHeight + (m_fHookHeadHeight - m_fTargetHeadHeight) * 0.5f);

    if (m_nParam != 0) {
        GetCameraParameters(nType, nSide, m_pShotHook.m_pObject->GetPosition(),
                            m_pShotTarget.m_pObject->GetPosition(), vDir, qRot);
    } else {
        if (m_pShotHook.m_pObject->GetPartPosition(const_cast<char*>("CAMERAHOOK"), &vPart, &qPart) != 0)
            vHookPos = vPart;
        else
            vHookPos.z += m_fHookHeadHeight;
        if (m_pShotTarget.m_pObject->GetPartPosition(const_cast<char*>("CAMERAHOOK"), &vPart, &qPart) != 0)
            vTargetPos = vPart;
        else
            vTargetPos.z += m_fTargetHeadHeight;
        GetCameraParameters(nType, nSide, vHookPos, vTargetPos, vDir, qRot);
    }

    if (nType == 3) {
        nHit = m_pOwner->GetScene()->HitCheck(vMid, vDir, &vOut, &hit);
        hit.ClearOutput();
        if (m_nParam == 0) {
            nHit += m_pOwner->GetScene()->HitCheck(vHookPos, vDir, &vOut, &hit);
            hit.ClearOutput();
            nHit += m_pOwner->GetScene()->HitCheck(vTargetPos, vDir, &vOut, &hit);
            hit.ClearOutput();
        }
    } else {
        nHit = m_pOwner->GetScene()->HitCheck(vHookPos, vDir, &vOut, &hit);
        hit.ClearOutput();
    }

    if (nHit != 0)
        m_bShotObstructed = 1;
    m_nShotType = nType;
    m_bRunBaseControl = 0;
    Control(0.0f);
    m_bRunBaseControl = 1;
}

// Adopts a framed shot: caches the placement, marks the shot cached, and gives the owner's camera
// the shot's view angle. Only shot kind 6 is adopted; anything else returns at once.
void CSWBehaviorCameraDialog::SetShot(int shotType, Vector pos, Quaternion rot, float f)
{
    if (shotType != 6)
        return;

    m_bShotObstructed = 0;
    if (m_pOwner->AsCAurCamera() != 0)
        static_cast<CAurCamera*>(m_pOwner->AsCAurCamera())->SetViewAngle(f);

    m_nShotType = 6;
    m_vShotPos = pos;
    m_qShotRot = rot;
}

// Moves the owner to the shot's cached position and orientation.
void CSWBehaviorCameraDialog::ApplyCameraShot()
{
    m_pOwner->SetPosition(m_vShotPos);
    m_pOwner->SetOrientation(m_qShotRot);
}
