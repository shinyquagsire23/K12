#pragma once

// A client-side projectile, spawned by CSWCMessage's SafeProjectile* handlers and by
// CSWCVisualEffectOnObject::ApplyLightsaberThrow for a thrown saber.
//
// The behavior list is a chain of steering programs; the SetBehaviorList* builders arm the
// projectile with one or more of them.

#include "nwcobject.h"
#include "Vector.h"
#include "Vector.h"
#include "CExoLinkedList.h"
#include "exostring.h"
#include "exoresman.h"

class CAurObject;
class CProjectileBehavior;

class CSWCProjectile : public CSWCObject {
public:
    // SetProjectileSpellId's ids with no spells.2da row in either game: the MIRV effect's projectile,
    // and the cheat effect's cow, which plays c_cow_dead on impact.
    enum {
        SPELL_ID_MIRV = 340,
        SPELL_ID_COW  = 11000,
    };

    // SetProjectileType's types, each the SetBehaviorList* program it builds: spells.2da's ProjType
    // values (1-9, 11, 50) and the blaster-bolt outcomes (parried, deflected, reflected).
    enum {
        PROJECTILE_TYPE_HOMING            = 1,
        PROJECTILE_TYPE_BALLISTIC         = 2,
        PROJECTILE_TYPE_HIGH_BALLISTIC    = 3,
        PROJECTILE_TYPE_BURST_UP          = 4,
        PROJECTILE_TYPE_ACCELERATING      = 5,
        PROJECTILE_TYPE_SPIRAL            = 6,
        PROJECTILE_TYPE_LINKED            = 7,
        PROJECTILE_TYPE_BOUNCE            = 8,
        PROJECTILE_TYPE_BURST             = 9,
        PROJECTILE_TYPE_LINKED_BURST_UP   = 10,
        PROJECTILE_TYPE_GRENADE           = 11,
        PROJECTILE_TYPE_TRIPLE_BALLISTIC  = 12,
        PROJECTILE_TYPE_DOUBLE_BALLISTIC  = 13,
        PROJECTILE_TYPE_PARRIED           = 14,
        PROJECTILE_TYPE_DEFLECTED         = 15,
        PROJECTILE_TYPE_REFLECTED         = 16,
        PROJECTILE_TYPE_TEST              = 50,
    };

    CSWCProjectile();
    virtual ~CSWCProjectile();

    // <VTABLE-GEN: the FIVE slots CSWCProjectile really overrides (_ZTV14CSWCProjectile); the
    virtual CSWCProjectile* AsSWCProjectile();
    virtual void AIUpdate();
    virtual int  LoadModel(const CResRef& r, unsigned char a, unsigned char b);
    virtual void UnloadModel();
    virtual void UnloadModel(unsigned char a);
    // </VTABLE-GEN>

    // The ammunition table's sound-column counts; declared but never defined.
    static const int NUM_IMPACT_SOUNDS;
    static const int NUM_SHOT_SOUNDS;

    // Wire entry points driven by CSWCMessage's SafeProjectile* handlers.
    void SetDesiredPositionTimer(int timer);
    void SetSpawnCombatVisualOnImpact(unsigned char bSpawn, int visualId, short param);
    void SetProjectileSpellId(unsigned int spellId);
    void SetProjectileType(unsigned int nType);
    void SetProjectileImpactSound(const CResRef& res);
    void SetDesiredTargetObject(unsigned long long targetId, const CExoString& sPart);
    void SetProjectileVelAndAccel(const Vector& v);

    // A separate, non-virtual overload of the base's three-argument virtual; returns 0 or 1.
    int  SetAnimation(unsigned short nAnim);
    using CSWCObject::SetAnimation;

    // Behavior builders; each pushes one or more newly allocated CProjectileBehavior nodes onto
    // m_behaviorList.
    void ClearBehaviorList();
    void SetBehaviorListHoming();
    void SetBehaviorListBallistic();
    void SetBehaviorListHighBallistic();
    void SetBehaviorListBurstUp();
    void SetBehaviorListBurst();
    void SetBehaviorListAccelerating();
    void SetBehaviorListSpiral();
    void SetBehaviorListLinked();
    void SetBehaviorListBounce();
    void SetBehaviorListLinkedBurstUp();
    void SetBehaviorListTest();
    void SetBehaviorListTripleBallistic(int nCount);
    void SetBehaviorListDoubleBallistic();
    void SetBehaviorListParried();
    void SetBehaviorListDeflected();
    void SetBehaviorListReflected();
    // Returns int rather than bool, which would narrow the result.
    int  UpdateCurrentBehavior();
    void MoveProjectileToPosition();
    void MoveProjectileAccelerating();
    void MoveProjectileSpiral();
    void MoveProjectileToFinalVelocity();

    // Builds the gun-barrel flash, hung off the given part of the given model.
    void CreateMuzzleFlash(const CExoString& sModel, CAurObject* pAttachTo,
                           const CExoString& sPart);

    unsigned char m_bSpawnCombatVisualOnImpact;
    char          _pad149[0x14c-0x149];
    Vector        m_vPreviousPosition;
    short         m_spawnCombatVisualParam;
    char          _pad15a[0x15c-0x15a];
    int           m_spawnCombatVisualId;
    Vector        m_desiredPosition;
    unsigned int  m_desiredPosTimer;
    unsigned long long m_oidTarget;
    CExoString    m_sTargetPart;
    CResRef       m_soundResRef;
    CResRef       m_impactSound;
    CExoSoundSource* m_pSoundSource;
    int           m_bStickToTarget;
    unsigned char m_nImpactMode;
    char          _pad1b5[0x1b8-0x1b5];
    int           m_bUpdateEnabled;
    int           m_bFlightSoundStarted;
    int           m_bArrived;
    int           m_bFaceCameraOnArrival;
    unsigned int  m_nProjectileType;
    int           m_nImpactLingerMs;
    int           m_nProjectileSpellId;
    Vector        m_vVelocity;
    Vector        m_vAcceleration;
    unsigned int  m_desiredPosTimer2;
    CExoLinkedList<CProjectileBehavior> m_behaviorList;
    CProjectileBehavior* m_pCurrentBehavior;
    CAurObject*   m_pRootReference;
    unsigned char m_bSpiralCounterClockwise;
    unsigned char m_nProjOrientation;
    char          _pad20a[0x20c-0x20a];
    int           m_bTargetFlinched;
    int           m_nReserved1;
    char          _pad214[0x218-0x214];
    CAurObject*   m_pMuzzleFlash;
    Vector        m_vImpactPosition;
    int           m_nDeflectFlightTime;
    float         m_fShieldRadius;
    char          _pad234[0x238-0x234];
    unsigned long long m_sourceObjectId;
};

K2_LAYOUT_ASSERT(sizeof(CSWCProjectile) == 0x240, "CSWCProjectile: sizeof pinned by the new sites");


// One steering program on a projectile's behavior chain.
//
// The kind ids name primitives, not builders. One builder can push several behaviors, which is why
// several arms share an id.
class CProjectileBehavior {
public:
    // m_nGrenadeSoundRow is deliberately left uninitialised: only the grenade arms write it, after
    // construction.
    CProjectileBehavior() = default;
    CProjectileBehavior(unsigned char nKind, const Vector& vStart, const Vector& vEnd,
                        unsigned long long oidTarget, int nTimer, int nHandoffTimer, float fRate,
                        unsigned char nSpawnCount)
        : m_vStart(vStart), m_vEnd(vEnd), m_oidTarget(oidTarget), m_nTimer(nTimer),
          m_nHandoffTimer(nHandoffTimer), m_fRate(fRate), m_nSpawnCount(nSpawnCount), m_nKind(nKind) {}

    Vector             m_vStart;
    Vector             m_vEnd;
    unsigned long long m_oidTarget;
    int                m_nSubProjectileType;
    int                m_nTimer;
    int                m_nHandoffTimer;
    float              m_fRate;
    unsigned char      m_nSpawnCount;
    unsigned char      m_nKind;
    unsigned char      _pad32[2];
    int                m_nGrenadeSoundRow;

    enum {
        KIND_HOMING       = 0x01,
        KIND_BALLISTIC    = 0x02,  // plays m_nGrenadeSoundRow
        KIND_BOUNCE       = 0x03,
        KIND_BURST_UP     = 0x04,
        KIND_ACCELERATING = 0x05,
        KIND_SPIRAL_TAIL  = 0x06,
        KIND_LINKED       = 0x07,
        KIND_UNK08        = 0x08,  // no step, like KIND_BOUNCE and KIND_HIGH_ARC
        KIND_BURST        = 0x09,
        KIND_SPIRAL       = 0x0a,
        KIND_HIGH_ARC     = 0x0b,  // the launch leg: slide to the target, then 30.0f up on z
        KIND_LINKED_BURST = 0x0c,
        KIND_UNK0D        = 0x0d,  // also calls SetAnimation(4)
        KIND_UNK0E        = 0x0e
    };
};

K2_LAYOUT_ASSERT(sizeof(CProjectileBehavior) == 0x38, "CProjectileBehavior: pinned by the new sites");
inline void CSWCProjectile::SetProjectileImpactSound(const CResRef& res)
{
    m_impactSound = res;
}
