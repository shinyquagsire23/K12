#include <stddef.h>
#include "AURORA/aurmodel.h"
#include "exosound.h"
#include "swmgbehavior.h"
#include "swmgtrackfollower.h"
#include "Quaternion.h"
#include "Vector.h"
#include "soundprioritygroup.h"

extern CExoSound* g_pExoSound;

CSWMGBehaviorFire::CSWMGBehaviorFire(CAurObject* pObject, CAurObject* pAttachTo, CAurPart* pPart)
    : CAurBehaviorAttach(pObject, pAttachTo, pPart), m_resFireSound()
{
    // m_fRateOfFire is written by the gun bank's SetRateOfFire after construction.
    m_fTimeToNextShot = 0.0f;
}

int CSWMGBehaviorFire::Control(float fParam)
{
    if (!CAurBehaviorAttach::Control(fParam))
        return 0;

    if (m_fTimeToNextShot > 0.0f) {
        float fTimeToNextShot = m_fTimeToNextShot - fParam;
        if (fTimeToNextShot < 0.0f)
            fTimeToNextShot = 0.0f;
        m_fTimeToNextShot = fTimeToNextShot;
    }
    return 1;
}

int CSWMGBehaviorFire::Fire()
{
    if (m_fTimeToNextShot == 0.0f) {
        m_fTimeToNextShot = m_fRateOfFire;
        m_pOwner->PlayAnimation(const_cast<char*>("fire"), 1.0f, 1, 0.0f);
        m_pOwner->PlayAnimation(const_cast<char*>("ready"), 1.0f, AnimRun::kAnimQueue, 0.0f);

        if (m_resFireSound.IsValid()) {
            g_pExoSound->PlayOneShotSound(CResRef(m_resFireSound.GetResRef()), PRIORITY_GROUP_DEFAULT, 0, 0,
                                          0.0f, 0.0f);
        }
        return 1;
    }
    return 0;
}

void CSWMGBehaviorFire::SetRateOfFire(float rate)
{
    // Re-base a mid-cycle countdown by the rate delta so a rate change neither skips nor doubles
    // the next shot; an idle or already-elapsed timer is left alone.
    if (m_fTimeToNextShot != 0.0f && m_fTimeToNextShot < m_fRateOfFire)
        m_fTimeToNextShot -= m_fRateOfFire - rate;
    m_fRateOfFire = rate;
}

int CSWMGBehaviorFire::GetType() { return BEHAVIOR_TYPE_MGFIRE; }





extern CExoSound* g_pExoSound;

CSWMGBehaviorTrackAndFire::CSWMGBehaviorTrackAndFire(CAurObject* pObject, CAurObject* pAttachTo,
                                                     CAurPart* pPart, CAurObject* pTarget)
    : CSWMGBehaviorTrack(pObject, pAttachTo, pPart, pTarget), m_resFireSound()
{
    // m_fRateOfFire is written by the gun bank's SetRateOfFire after construction.
    m_fTimeToNextShot = 0.0f;
}

int CSWMGBehaviorTrackAndFire::Control(float fParam)
{
    if (!CSWMGBehaviorTrack::Control(fParam))
        return 0;

    if (m_fTimeToNextShot > 0.0f) {
        float fTimeToNextShot = m_fTimeToNextShot - fParam;
        if (fTimeToNextShot < 0.0f)
            fTimeToNextShot = 0.0f;
        m_fTimeToNextShot = fTimeToNextShot;
    }

    // Fire only while the target is inside the aim cone and the cadence timer has run out.
    if (m_bTargetInCone != 0 && m_fTimeToNextShot == 0.0f) {
        m_fTimeToNextShot = m_fRateOfFire;
        m_pOwner->PlayAnimation(const_cast<char*>("fire"), 1.0f, 1, 0.0f);
        m_pOwner->PlayAnimation(const_cast<char*>("ready"), 1.0f, AnimRun::kAnimQueue, 0.0f);

#if KOTOR1  // K1 is silent while the sound system is paused
        if (m_resFireSound.IsValid() &&
            g_pExoSound->GetSoundMode() != CExoSound::CEXOSOUND_MODE_PAUSE &&
            g_pExoSound->GetSoundMode() != CExoSound::CEXOSOUND_MODE_PAUSE_ALL &&
            g_pExoSound->GetSoundMode() != CExoSound::CEXOSOUND_MODE_PAUSE_ALL_BUT_MUSIC) {
#else
        if (m_resFireSound.IsValid()) {
#endif
            g_pExoSound->Play3DOneShotSound(CResRef(m_resFireSound.GetResRef()),
                                            m_pOwner->GetPosition(), 0.0f, PRIORITY_GROUP_MINIGAME_ENEMY, 0, 0, 0.0f, 0.0f);
        }
    }
    return 1;
}

void CSWMGBehaviorTrackAndFire::SetRateOfFire(float rate)
{
    // Same re-basing as CSWMGBehaviorFire::SetRateOfFire.
    if (m_fTimeToNextShot != 0.0f && m_fTimeToNextShot < m_fRateOfFire)
        m_fTimeToNextShot -= m_fRateOfFire - rate;
    m_fRateOfFire = rate;
}

int CSWMGBehaviorTrackAndFire::GetType() { return BEHAVIOR_TYPE_MGTRACKANDFIRE; }





CSWMGBehaviorBullet::CSWMGBehaviorBullet(CAurObject* pObject, CSWTrackFollower* pFollower,
                                         const CSWMGBullet& bullet)
    : CAurBehavior(pObject)
{
    m_bullet = bullet;
    m_pTrackFollower = pFollower;
}

int CSWMGBehaviorBullet::Control(float fParam)
{
    if (!CAurBehavior::Control(fParam))
        return 0;

    m_bullet.m_fLifeSpan -= fParam;
    CAurObject* pOwner = m_pOwner;
    if (m_bullet.m_fLifeSpan <= 0.0f) {
        if (pOwner != NULL)
            delete pOwner;
        return 0;
    }

    Quaternion q = pOwner->GetOrientation();

    // Column 1 of the rotation matrix is the model's local Y axis.
    Vector vDir;
    vDir.x = (q.x * q.y - q.w * q.z) + (q.x * q.y - q.w * q.z);
    vDir.y = 1.0f - ((q.x * q.x + q.z * q.z) + (q.x * q.x + q.z * q.z));
    vDir.z = (q.y * q.z + q.x * q.w) + (q.y * q.z + q.x * q.w);

    Vector vOffset = vDir * (m_bullet.m_fSpeed * fParam);
    pOwner->SetPosition(vOffset + m_pOwner->GetPosition());
    return 1;
}

int CSWMGBehaviorBullet::GetType() { return BEHAVIOR_TYPE_MGBULLET; }






extern int quaternionidentityoptimization;

float Pitch(const Vector& orientation);
float Yaw(const Vector& orientation);

// Rotates a vector by a quaternion, and by its inverse.
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
    r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) + v.z * ((xz + wy) + (xz + wy));
    r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) + v.z * ((yz - wx) + (yz - wx));
    r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) + v.z * (1.0f - ((xx + yy) + (xx + yy)));
    return r;
}

#if !KOTOR1
static inline Vector RotateByQuaternionInverse(const Vector& v, const Quaternion& q)
{
    float w = q.w;
    if (quaternionidentityoptimization != 0 && (w == -1.0f || w == 1.0f))
        return v;

    float x = q.x, y = q.y, z = q.z;
    float nw = -w;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = x * nw, wy = y * nw, wz = z * nw;
    Vector r;
    r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) + v.z * ((xz + wy) + (xz + wy));
    r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) + v.z * ((yz - wx) + (yz - wx));
    r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) + v.z * (1.0f - ((xx + yy) + (xx + yy)));
    return r;
}
#endif

#if KOTOR1  // K1: the inverse is the forward rotate by -q, quaternion first
static inline Vector RotateByQuaternionK1(const Quaternion& q, const Vector& v)
{
    return RotateByQuaternion(v, q);
}
#endif

CSWMGBehaviorTrack::CSWMGBehaviorTrack(CAurObject* pObject, CAurObject* pAttachTo,
                                       CAurPart* pPart, CAurObject* pTarget)
    : CAurBehaviorAttach(pObject, pAttachTo, pPart)
{
    // m_fSpreadRadius is written by the gun bank after construction; m_vTargetOffset zeroes
    // itself through Vector's default constructor.
    m_bTargetInCone = 0;
    m_pTarget = pTarget;
    AddDependency(pTarget);
}

int CSWMGBehaviorTrack::Control(float fParam)
{
    m_bTargetInCone = 0;
    if (!CAurBehaviorAttach::Control(fParam))
        return 0;

    if (m_pTarget.m_pObject == NULL) {
        m_pOwner->SetBehavior(NULL);
        return 0;
    }

#if KOTOR1  // K1: the inverse is formed from -q first
    Quaternion q = m_pOwner->GetOrientation();
    Vector vAim = RotateByQuaternionK1(-q, RotateByQuaternion(m_pOwner->GetPosition(), q) + m_vTargetOffset);
    Vector vLocal = RotateByQuaternionK1(-q, m_pTarget.m_pObject->GetPosition() - vAim);
#else
    Quaternion q = m_pOwner->GetOrientation();
    Vector vPos = m_pOwner->GetPosition();

    // Rotate the owner position into world space, add the muzzle offset, then rotate back.
    Vector vAim = RotateByQuaternionInverse(RotateByQuaternion(vPos, q) + m_vTargetOffset, q);

    // The target's direction from the muzzle, in the owner's local frame.
    Vector vLocal = RotateByQuaternionInverse(m_pTarget.m_pObject->GetPosition() - vAim, q);
#endif

    if (m_targetting.m_fSensingRadius * m_targetting.m_fSensingRadius <
        vLocal.x * vLocal.x + vLocal.y * vLocal.y + vLocal.z * vLocal.z)
        return 1;

    float fPitch = Pitch(vLocal);
    float fYaw = Yaw(vLocal);

    // In-cone flag: set optimistically, cleared by whichever clamp fires.
    m_bTargetInCone = 1;
    if (m_targetting.m_fHorizontalSpread < __builtin_fabsf(fYaw)) {
        m_bTargetInCone = 0;
        if (fYaw < 0.0f)
            fYaw = -m_targetting.m_fHorizontalSpread;
        else
            fYaw = m_targetting.m_fHorizontalSpread;
    }
    if (m_targetting.m_fVerticalSpread < __builtin_fabsf(fPitch)) {
        m_bTargetInCone = 0;
        if (fPitch < 0.0f)
            fPitch = -m_targetting.m_fVerticalSpread;
        else
            fPitch = m_targetting.m_fVerticalSpread;
    }

    float fRoll = Roll(q);
    Quaternion qAim = YawPitchRoll(fYaw, -fPitch, fRoll);
    m_pOwner->SetOrientation(qAim * q);
    return 1;
}

int CSWMGBehaviorTrack::GetType() { return BEHAVIOR_TYPE_MGTRACK; }
