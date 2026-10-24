#pragma once

#ifndef CSWMINIGAMEOBJECT_H
#define CSWMINIGAMEOBJECT_H

#include "exoresman.h"
#include "exostring.h"
#include "Vector.h"
#include "AURORA/aurmodel.h"
#include "SafePointer.h"
#include "swmgguns.h"

class CResGFF;
struct CResStruct;
class CSWTrackFollower;
class CSWMiniPlayer;
class CSWMiniEnemy;
class CSWMGObstacle;

// Base class of everything a CSWMiniGame tracks: scripted track followers (CSWTrackFollower, and
// through it CSWMiniPlayer / CSWMiniEnemy) and static obstacles (CSWMGObstacle). The destructor is
// not virtual; the four As* probes are stubs here and are overridden by each derived type.
class CSWMiniGameObject {
public:
    // GetScriptName / SetScriptName slots, in the order LoadScripts reads the GFF fields: the
    // object's own five, then the track follower's eight.
    enum {
        SCRIPT_ON_CREATE       = 0,
        SCRIPT_ON_HIT_BULLET   = 1,
        SCRIPT_ON_HIT_FOLLOWER = 2,
        SCRIPT_ON_ANIM_EVENT   = 3,
        SCRIPT_ON_HEARTBEAT    = 4,
        SCRIPT_ON_DAMAGE       = 5,
        SCRIPT_ON_DEATH        = 6,
        SCRIPT_ON_FIRE         = 7,
        SCRIPT_ON_HIT_OBSTACLE = 8,
        SCRIPT_ON_TRACK_LOOP   = 9,
        SCRIPT_ON_ACCELERATE   = 10,
        SCRIPT_ON_BRAKE        = 11,
        SCRIPT_ON_HIT_WORLD    = 12,
    };

#if KOTOR1  // K1's destructor is virtual
    virtual ~CSWMiniGameObject();
#endif
    virtual void PlayAnimation(char*, float, int) = 0;
    virtual CExoString GetName() = 0;
    virtual void LoadScripts(CResGFF*, CResStruct*, CResStruct&);
    virtual CResRef GetScriptName(int) = 0;
    virtual void SetScriptName(int, const CResRef&) = 0;
    virtual CSWMiniGameObject* AsFollower() { return 0; }
    virtual CSWMiniGameObject* AsPlayer() { return 0; }
    virtual CSWMiniGameObject* AsEnemy() { return 0; }
    virtual CSWMiniGameObject* AsObstacle() { return 0; }
    virtual void Update(float);

    CSWMiniGameObject(unsigned int);
#if !KOTOR1
    ~CSWMiniGameObject();
#endif

    CSWTrackFollower* GetLastBulletHitShooter();
    CSWTrackFollower* GetLastFollowerHit();
    void OnHitBullet(int nShooterId);
    void OnHitFollower(int nFollowerId);
    void OnCreate();

    unsigned int m_nObjectType;
    unsigned int m_nObjectId;
    unsigned char m_bPendingDelete;
    char _pad11[0x18 - 0x11];
    CExoString m_sName;
    SafePointer<CAurObject> m_spObject;
    CSWMGBullet m_bullet;
    CExoString m_sLastBulletHitPart;
    unsigned int m_nLastBulletHitShooter;
    unsigned int m_nLastFollowerHit;
    Vector m_vPosition;
};

#endif

#ifndef CSWMINIGAMEOBJECTARRAY_H
#define CSWMINIGAMEOBJECTARRAY_H

class CSWMiniGameObject;
class CExoString;

// App-wide id -> object table, handed out by CClientExoApp as void*. The 255 slots are indexed by
// CSWMiniGameObject::m_nObjectId, which wraps at 255; that value is also the "none" sentinel.
class CSWMiniGameObjectArray {
public:
    // The id table's size; the top id doubles as "no object" (m_nLastBulletHitShooter,
    // m_nLastFollowerHit, a full table).
    enum {
        OBJECT_ID_NONE = 0xff,
    };

    // GetMiniGameObject's nType probes.
    enum {
        PROBE_ANY           = 0,
        PROBE_PLAYER        = 1,
        PROBE_ENEMY         = 2,
        PROBE_OBSTACLE      = 3,
        PROBE_FOLLOWER      = 4,
        PROBE_TRIGGER_ENEMY = 5,
    };

    CSWMiniGameObjectArray();
    // nType selects the probe: 0 = any object, 1 = AsPlayer, 2 = AsEnemy, 3 = AsObstacle,
    // 4 = AsFollower, 5 = enemy with its m_bTrigger flag set.
    CSWMiniGameObject* GetMiniGameObject(unsigned int, unsigned char);
    CSWMiniGameObject* GetMiniGameObjectByName(const CExoString&);
    unsigned int GetNextID();

    int m_nNextId;
    CSWMiniGameObject* m_apObjects[255];
};

#endif
