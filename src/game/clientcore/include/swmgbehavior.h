// swmgbehavior.h -- the minigame behaviours: a bullet in flight, a firing gun, and the two
// tracking variants that combine aiming with firing.

#pragma once

#ifndef CSWMGBEHAVIORBULLET_H
#define CSWMGBEHAVIORBULLET_H

#include "include/aurbehavior.h"
#include "swmgguns.h"

class CAurObject;
class CSWTrackFollower;

// The behaviour id CSWMGBehaviorBullet::GetType() returns, which callers hand
// CAurObject::GetBehavior to find a bullet's behaviour.
#define BEHAVIOR_TYPE_MGBULLET 0xaaab

class CSWMGBehaviorBullet : public CAurBehavior {
public:

    virtual int Control(float fParam);
    virtual int GetType();

    CSWMGBullet       m_bullet;
    CSWTrackFollower* m_pTrackFollower;

    CSWMGBehaviorBullet(CAurObject* pObject, CSWTrackFollower* pFollower,
                        const CSWMGBullet& bullet);
};

#endif // CSWMGBEHAVIORBULLET_H

#ifndef CSWMGBEHAVIORFIRE_H
#define CSWMGBEHAVIORFIRE_H

#include "AURORA/aurbehavior.h"
#include "exoresman.h"

// The behaviour id a caller hands CAurObject::GetBehavior to ask a gun model for its fire behaviour.
#define BEHAVIOR_TYPE_MGFIRE 0xaaad

class CSWMGBehaviorFire : public CAurBehaviorAttach {
public:

    virtual int Control(float fParam);
    virtual int GetType();
    // 1 when the shot went off, 0 when the gun is not ready yet.
    virtual int Fire();

    CResRef m_resFireSound;
    float   m_fRateOfFire;
    float   m_fTimeToNextShot;

    CSWMGBehaviorFire(CAurObject* pObject, CAurObject* pAttachTo, CAurPart* pPart);
    // Re-bases a pending countdown by the delta, so a rate change mid-cycle neither skips nor
    // doubles a shot.
    void SetRateOfFire(float rate);
    void SetFireSound(const CResRef& res);
};

inline void CSWMGBehaviorFire::SetFireSound(const CResRef& res)
{
    m_resFireSound = res;
}

#endif // CSWMGBEHAVIORFIRE_H

#ifndef CSWMGBEHAVIORTRACK_H
#define CSWMGBEHAVIORTRACK_H

#include "AURORA/aurbehavior.h"
#include "SafePointer.h"
#include "Vector.h"

class CAurObject;

// The behaviour id CSWMGBehaviorTrack::GetType() returns.
#define BEHAVIOR_TYPE_MGTRACK 0xaaac

class CSWMGBehaviorTrack : public CAurBehaviorAttach {
public:

    virtual int Control(float fParam);
    virtual int GetType();

    CSWMGTargettingParameters m_targetting;
    bool    m_bTargetInCone;
    char    _pad79[0x7c - 0x79];
    float   m_fSpreadRadius;
    SafePointer<CAurObject> m_pTarget;
    Vector  m_vTargetOffset;

    CSWMGBehaviorTrack(CAurObject* pObject, CAurObject* pAttachTo, CAurPart* pPart,
                       CAurObject* pTarget);
};

#endif // CSWMGBEHAVIORTRACK_H

#ifndef CSWMGBEHAVIORTRACKANDFIRE_H
#define CSWMGBEHAVIORTRACKANDFIRE_H

#include "exoresman.h"

// The behaviour id CSWMGBehaviorTrackAndFire::GetType() returns.
#define BEHAVIOR_TYPE_MGTRACKANDFIRE 0xaaaa

class CSWMGBehaviorTrackAndFire : public CSWMGBehaviorTrack {
public:

    virtual int Control(float fParam);
    virtual int GetType();

    CResRef m_resFireSound;
    float   m_fRateOfFire;
    float   m_fTimeToNextShot;

    CSWMGBehaviorTrackAndFire(CAurObject* pObject, CAurObject* pAttachTo, CAurPart* pPart,
                              CAurObject* pTarget);
    void SetRateOfFire(float rate);
    void SetFireSound(const CResRef& res);
};

inline void CSWMGBehaviorTrackAndFire::SetFireSound(const CResRef& res)
{
    m_resFireSound = res;
}

#endif // CSWMGBEHAVIORTRACKANDFIRE_H
