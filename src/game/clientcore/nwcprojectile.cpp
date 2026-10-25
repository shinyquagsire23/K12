#include "nwcprojectile.h"
#include "nwscriptconst.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "worldtimer.h"
#include "gameobjectarray.h"
#include "clientaimaster.h"
#include "nwsrules.h"
#include "nwspell.h"
#include "AURORA/aurmodel.h"
#include "nwcanimbase.h"
#include "nwcanimbaseprojectile.h"
#include "nwcplaceable.h"
#include "nwcdoor.h"
#include "nwcmodule.h"
#include "nwcarea.h"
#include "aurscene.h"
#include "aurcamera.h"
#include "nwcitem.h"
#include "nwitem.h"
#include "nwccreature.h"
#include "nwscreature.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "exosound.h"
#include "swguiexamine.h"
#include <cstdio>
#include "serveranimation.h"
#include "soundprioritygroup.h"
#include "exostringinline.h"

#if KOTOR1  // K1: model files are SDL_RWops streams
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif


const int CSWCProjectile::NUM_IMPACT_SOUNDS = 2;
const int CSWCProjectile::NUM_SHOT_SOUNDS = 2;


// Play3DOneShotSound's volume byte at full volume (0 keeps the sound's own).
enum {
    ONE_SHOT_VOLUME_FULL = 0xff,
};

// How close to arrival (in milliseconds) a parried or deflected bolt makes its target flinch.
enum {
    FLINCH_LEAD_MS = 111,
};

CSWCProjectile::CSWCProjectile()
    : CSWCObject(CGameObject::OBJECT_TYPE_PROJECTILE)
{
    unsigned int nDay, nTod;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);
    m_nLastAIUpdateDay = nDay;
    m_nLastAIUpdateTod = nTod;

    m_position = Vector(0.0f, 0.0f, 0.0f);
    m_desiredPosition = Vector(0.0f, 0.0f, 0.0f);
    m_bUpdateEnabled = 1;
    m_bFaceCameraOnArrival = 0;
    m_nProjectileType = 0;
    m_bFlightSoundStarted = 0;
    m_bArrived = 0;
    m_vAcceleration = Vector(0.0f, 0.0f, 0.0f);
    m_vVelocity = Vector(0.0f, 0.0f, 0.0f);
    m_bStickToTarget = 1;
    m_nImpactLingerMs = 1;
    m_pSoundSource = 0;
    m_desiredPosTimer = 0;
    m_desiredPosTimer2 = 0;

    g_pAppManager->m_pClientExoApp->GetGameObjectArray()->AddInternalObject(m_objectId, this, 0);
    g_pAppManager->m_pClientExoApp->GetClientAIMaster()->AddObject(this);

    m_nProjectileSpellId = 0;
    m_bSpawnCombatVisualOnImpact = 0;
    m_spawnCombatVisualId = 0;
    m_nImpactMode = 0;
    m_vPreviousPosition = Vector(0.0f, 0.0f, 0.0f);
    m_spawnCombatVisualParam = 0;
    m_bSpiralCounterClockwise = 0;
    m_nProjOrientation = 0;
    m_pCurrentBehavior = 0;
    m_pRootReference = 0;
    m_sTargetPart = "impact";
    m_bTargetFlinched = 0;
    m_nReserved1 = 0;
    m_pMuzzleFlash = 0;
    m_nDeflectFlightTime = 0;
    m_fShieldRadius = 0.0f;
    m_oidTarget = OBJECT_INVALID;
    m_sourceObjectId = OBJECT_INVALID;
}

CSWCProjectile::~CSWCProjectile()
{
    g_pAppManager->m_pClientExoApp->GetGameObjectArray()->Delete(m_objectId);
    ClearBehaviorList();
    UnloadModel();
}

void CSWCProjectile::ClearBehaviorList()
{
    while (m_behaviorList.m_pInternal->m_pHead != 0) {
        CProjectileBehavior* pBehavior = static_cast<CProjectileBehavior*>(m_behaviorList.m_pInternal->RemoveHead());
        if (pBehavior != 0)
            delete pBehavior;
    }
}

int CSWCProjectile::SetAnimation(unsigned short nAnim)
{
    if (m_pModel == 0)
        return 0;
    if (m_pModel->SetAnimation(nAnim, 1.0f, 0, 0.0f) == 0)
        return 0;
    m_currentAnimation = nAnim;
    return 1;
}

void CSWCProjectile::SetDesiredPositionTimer(int timer)
{
    m_desiredPosTimer = timer;
    m_desiredPosTimer2 = timer;
}

void CSWCProjectile::SetSpawnCombatVisualOnImpact(unsigned char bSpawn, int visualId, short param)
{
    m_bSpawnCombatVisualOnImpact = bSpawn;
    m_spawnCombatVisualId = visualId;
    m_spawnCombatVisualParam = param;
}

CSWCProjectile* CSWCProjectile::AsSWCProjectile() { return this; }


#if KOTOR1  // K1's SetProjectileType inlines this builder; K2's calls it
__attribute__((always_inline))
#else
__attribute__((noinline))
#endif
void CSWCProjectile::SetBehaviorListHoming()
{
    Vector vAim;
    if (m_oidTarget != CGameObjectArray::INVALIDOBJID &&
        g_pAppManager->m_pClientExoApp->GetGameObject(m_oidTarget) != 0)
        vAim = Vector(0.0f, 0.0f, 0.0f);
    else
        vAim = m_desiredPosition;

    CProjectileBehavior* pBehavior = new CProjectileBehavior;
    pBehavior->m_nKind = CProjectileBehavior::KIND_HOMING;
    pBehavior->m_vStart = vAim;
    pBehavior->m_vEnd = vAim;
    pBehavior->m_nSpawnCount = 0;
    pBehavior->m_nTimer = m_desiredPosTimer2;
    pBehavior->m_nHandoffTimer = 0;
    pBehavior->m_oidTarget = m_oidTarget;
    pBehavior->m_nSubProjectileType = 0;
    pBehavior->m_fRate = -1.0f;
    m_behaviorList.m_pInternal->AddTail(pBehavior);
}

#if KOTOR1  // K1's SetProjectileType inlines this builder; K2's calls it
__attribute__((always_inline))
#else
__attribute__((noinline))
#endif
void CSWCProjectile::SetBehaviorListBallistic()
{
    Vector vAim;
    if (m_oidTarget != CGameObjectArray::INVALIDOBJID &&
        g_pAppManager->m_pClientExoApp->GetGameObject(m_oidTarget) != 0)
        vAim = Vector(0.0f, 0.0f, 0.0f);
    else
        vAim = m_desiredPosition;

    CProjectileBehavior* pBehavior = new CProjectileBehavior;
    pBehavior->m_nKind = CProjectileBehavior::KIND_BALLISTIC;
    pBehavior->m_vStart = vAim;
    pBehavior->m_vEnd = vAim;
    pBehavior->m_nSpawnCount = 0;
    pBehavior->m_nTimer = m_desiredPosTimer2;
    pBehavior->m_nHandoffTimer = 0;
    pBehavior->m_oidTarget = m_oidTarget;
    pBehavior->m_nSubProjectileType = 0;
    pBehavior->m_fRate = -1.0f;
    m_behaviorList.m_pInternal->AddTail(pBehavior);
}

#if KOTOR1  // K1's SetProjectileType inlines this builder; K2's calls it
__attribute__((always_inline))
#else
__attribute__((noinline))
#endif
void CSWCProjectile::SetBehaviorListBurstUp()
{
    Vector vAim;
    if (m_oidTarget != CGameObjectArray::INVALIDOBJID &&
        g_pAppManager->m_pClientExoApp->GetGameObject(m_oidTarget) != 0)
        vAim = Vector(0.0f, 0.0f, 0.0f);
    else
        vAim = m_desiredPosition;

    CProjectileBehavior* pBehavior = new CProjectileBehavior;
    pBehavior->m_nKind = CProjectileBehavior::KIND_BURST_UP;
    pBehavior->m_vStart = vAim;
    pBehavior->m_vEnd = vAim;
    pBehavior->m_nSpawnCount = 0;
    pBehavior->m_nTimer = m_desiredPosTimer2;
    pBehavior->m_nHandoffTimer = 0;
    pBehavior->m_oidTarget = m_oidTarget;
    pBehavior->m_nSubProjectileType = 0;
    pBehavior->m_fRate = -1.0f;
    m_behaviorList.m_pInternal->AddTail(pBehavior);
}

#if KOTOR1  // K1's SetProjectileType inlines this builder; K2's calls it
__attribute__((always_inline))
#else
__attribute__((noinline))
#endif
void CSWCProjectile::SetBehaviorListBurst()
{
    Vector vAim;
    if (m_oidTarget != CGameObjectArray::INVALIDOBJID &&
        g_pAppManager->m_pClientExoApp->GetGameObject(m_oidTarget) != 0)
        vAim = Vector(0.0f, 0.0f, 0.0f);
    else
        vAim = m_desiredPosition;

    CProjectileBehavior* pBehavior = new CProjectileBehavior;
    pBehavior->m_nKind = CProjectileBehavior::KIND_BURST;
    pBehavior->m_vStart = vAim;
    pBehavior->m_vEnd = vAim;
    pBehavior->m_nSpawnCount = 0;
    pBehavior->m_nTimer = m_desiredPosTimer2;
    pBehavior->m_nHandoffTimer = 0;
    pBehavior->m_oidTarget = m_oidTarget;
    pBehavior->m_nSubProjectileType = 0;
    pBehavior->m_fRate = -1.0f;
    m_behaviorList.m_pInternal->AddTail(pBehavior);
}

#if KOTOR1  // K1's SetProjectileType inlines this builder; K2's calls it
__attribute__((always_inline))
#else
__attribute__((noinline))
#endif
void CSWCProjectile::SetBehaviorListAccelerating()
{
    Vector vAim;
    if (m_oidTarget != CGameObjectArray::INVALIDOBJID &&
        g_pAppManager->m_pClientExoApp->GetGameObject(m_oidTarget) != 0)
        vAim = Vector(0.0f, 0.0f, 0.0f);
    else
        vAim = m_desiredPosition;

    CProjectileBehavior* pBehavior = new CProjectileBehavior;
    pBehavior->m_nKind = CProjectileBehavior::KIND_ACCELERATING;
    pBehavior->m_vStart = vAim;
    pBehavior->m_vEnd = vAim;
    pBehavior->m_nSpawnCount = 0;
    pBehavior->m_nTimer = m_desiredPosTimer2;
    pBehavior->m_nHandoffTimer = 0;
    pBehavior->m_oidTarget = m_oidTarget;
    pBehavior->m_nSubProjectileType = 0;
    pBehavior->m_fRate = -1.0f;
    m_behaviorList.m_pInternal->AddTail(pBehavior);
}

#if KOTOR1  // K1's SetProjectileType inlines this builder; K2's calls it
__attribute__((always_inline))
#else
__attribute__((noinline))
#endif
void CSWCProjectile::SetBehaviorListLinkedBurstUp()
{
    Vector vAim;
    if (m_oidTarget != CGameObjectArray::INVALIDOBJID &&
        g_pAppManager->m_pClientExoApp->GetGameObject(m_oidTarget) != 0)
        vAim = Vector(0.0f, 0.0f, 0.0f);
    else
        vAim = m_desiredPosition;

    CProjectileBehavior* pBehavior = new CProjectileBehavior;
    pBehavior->m_nKind = CProjectileBehavior::KIND_LINKED_BURST;
    pBehavior->m_vStart = vAim;
    pBehavior->m_vEnd = vAim;
    pBehavior->m_nSpawnCount = 0;
    pBehavior->m_nTimer = m_desiredPosTimer2;
    pBehavior->m_nHandoffTimer = 0;
    pBehavior->m_oidTarget = m_oidTarget;
    pBehavior->m_nSubProjectileType = 0;
    pBehavior->m_fRate = -1.0f;
    m_behaviorList.m_pInternal->AddTail(pBehavior);
}

#if KOTOR1  // K1's SetProjectileType inlines this builder; K2's calls it
__attribute__((always_inline))
#else
__attribute__((noinline))
#endif
void CSWCProjectile::SetBehaviorListLinked()
{
    Vector vAim;
    if (m_oidTarget != CGameObjectArray::INVALIDOBJID &&
        g_pAppManager->m_pClientExoApp->GetGameObject(m_oidTarget) != 0)
        vAim = Vector(0.0f, 0.0f, 0.0f);
    else
        vAim = m_desiredPosition;

    CProjectileBehavior* pBehavior = new CProjectileBehavior;
    pBehavior->m_nKind = CProjectileBehavior::KIND_LINKED;
    pBehavior->m_vStart = vAim;
    pBehavior->m_vEnd = Vector(0.0f, 0.0f, 0.0f);
    pBehavior->m_nSpawnCount = 0;
    pBehavior->m_nTimer = m_desiredPosTimer2;
    pBehavior->m_nHandoffTimer = 0;
    pBehavior->m_oidTarget = m_oidTarget;
    pBehavior->m_nSubProjectileType = 0;
    pBehavior->m_fRate = -1.0f;
    m_behaviorList.m_pInternal->AddTail(pBehavior);
}

void CSWCProjectile::SetBehaviorListBounce()
{
    CHitInfo hitInfo(0, ~CHitInfo::kMaterialTrigger, 0);
    Vector vHit;
    Vector avPoints[3];
    unsigned int anTimers[3];

    const float fDeltaX = m_desiredPosition.x - m_position.x;
    const float fDeltaY = m_desiredPosition.y - m_position.y;
    const float fDeltaZ = m_desiredPosition.z - m_position.z;

    hitInfo.m_bSkipObjects = 1;
    CAurScene* pScene = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pScene;

    unsigned char nCount = 1;
    float fT1 = 0.25f;
    float fT2 = 0.0f;
    float fZ1 = 0.0f;
    float fZ2 = 0.0f;
    float fSpan = 0.0f;
    float fToGoal = 0.0f;
    float fTSelected = 0.0f;

    avPoints[2] = m_desiredPosition;

    do {
        fT1 += -0.05f;
        avPoints[1] = Vector(m_desiredPosition.x - fDeltaX * fT1,
                             m_desiredPosition.y - fDeltaY * fT1,
                             m_desiredPosition.z - fDeltaZ * fT1);
        fZ1 = pScene->HitCheck(Vector(avPoints[1].x, avPoints[1].y, 1000.0f),
                               Vector(avPoints[1].x, avPoints[1].y, -1000.0f), &vHit,
                               &hitInfo) != 0
                  ? vHit.z
                  : (m_desiredPosition.z + m_position.z) * 0.5f;
        avPoints[1].z = fZ1;
        if (!(fT1 > 1e-5f)) {
            // With no surface anywhere along the line, aim straight at the goal from where we are.
            avPoints[0] = m_desiredPosition;
            anTimers[0] = m_desiredPosTimer2;
            anTimers[1] = 0;
            nCount = 1;
            goto buildNodes;
        }
    } while (m_desiredPosition.z - fZ1 >= 1.0f);

    anTimers[1] = static_cast<unsigned int>((1.0f - fT1) * static_cast<float>(m_desiredPosTimer2));

    fT2 = 0.55f;
    do {
        fT2 += -0.05f;
        avPoints[0] = Vector(m_desiredPosition.x - fDeltaX * fT2,
                             m_desiredPosition.y - fDeltaY * fT2,
                             m_desiredPosition.z - fDeltaZ * fT2);
        fZ2 = pScene->HitCheck(Vector(avPoints[0].x, avPoints[0].y, 1000.0f),
                               Vector(avPoints[0].x, avPoints[0].y, -1000.0f), &vHit,
                               &hitInfo) != 0
                  ? vHit.z
                  : (m_desiredPosition.z + m_position.z) * 0.5f;
        avPoints[0].z = fZ2;
    } while (fT1 < fT2 && fZ1 - fZ2 >= 0.5f);

    anTimers[0] = static_cast<unsigned int>((1.0f - fT2) * static_cast<float>(m_desiredPosTimer2));

    fSpan = (avPoints[0].x - avPoints[1].x) * (avPoints[0].x - avPoints[1].x) +
            (avPoints[0].y - avPoints[1].y) * (avPoints[0].y - avPoints[1].y) +
            (avPoints[0].z - avPoints[1].z) * (avPoints[0].z - avPoints[1].z);
    fToGoal = (avPoints[1].x - m_desiredPosition.x) * (avPoints[1].x - m_desiredPosition.x) +
              (avPoints[1].y - m_desiredPosition.y) * (avPoints[1].y - m_desiredPosition.y) +
              (avPoints[1].z - m_desiredPosition.z) * (avPoints[1].z - m_desiredPosition.z);
    fTSelected = (fSpan < fToGoal) ? fT1 : fT2;

    if (fTSelected <= fT1) {
        avPoints[0] = avPoints[1];
        avPoints[1] = m_desiredPosition;
        anTimers[1] = 3 * m_desiredPosTimer2 / 10;
        anTimers[0] = m_desiredPosTimer2 - anTimers[1];
        nCount = 2;
    } else {
        anTimers[2] = 2 * m_desiredPosTimer2 / 10;
        anTimers[1] = 3 * m_desiredPosTimer2 / 10;
        anTimers[0] = m_desiredPosTimer2 - anTimers[1] - anTimers[2];
        nCount = 3;
    }

buildNodes:
    for (unsigned char i = 0; i < nCount; i++) {
        CProjectileBehavior* pBehavior = new CProjectileBehavior(
            CProjectileBehavior::KIND_BALLISTIC, avPoints[i], m_desiredPosition,
            CGameObjectArray::INVALIDOBJID, anTimers[i], 0, -1.0f, 0);
        pBehavior->m_nSubProjectileType = 0;
        m_behaviorList.AddTail(pBehavior);
    }
}

void CSWCProjectile::SetBehaviorListHighBallistic()
{
    Vector vAim;
    if (m_oidTarget != CGameObjectArray::INVALIDOBJID &&
        g_pAppManager->m_pClientExoApp->GetGameObject(m_oidTarget) != 0)
        vAim = Vector(0.0f, 0.0f, 0.0f);
    else
        vAim = m_desiredPosition;

    CProjectileBehavior* pBehavior = new CProjectileBehavior;
    pBehavior->m_nKind = CProjectileBehavior::KIND_HOMING;
    pBehavior->m_vStart = m_position;
    pBehavior->m_vStart.z += 30.0f;
    pBehavior->m_vEnd = vAim;
    pBehavior->m_nSpawnCount = 0;
    pBehavior->m_nTimer = m_desiredPosTimer2 >> 1;
    pBehavior->m_nHandoffTimer = 0;
    pBehavior->m_oidTarget = CGameObjectArray::INVALIDOBJID;
    pBehavior->m_nSubProjectileType = 0;
    pBehavior->m_fRate = -1.0f;
    m_behaviorList.m_pInternal->AddTail(pBehavior);

    pBehavior = new CProjectileBehavior;
    pBehavior->m_nKind = CProjectileBehavior::KIND_HIGH_ARC;
    pBehavior->m_vStart = vAim;
    pBehavior->m_vStart.z += 30.0f;
    pBehavior->m_vEnd = vAim;
    pBehavior->m_nSpawnCount = 0;
    pBehavior->m_nTimer = 0;
    pBehavior->m_nHandoffTimer = 0;
    pBehavior->m_oidTarget = m_oidTarget;
    pBehavior->m_nSubProjectileType = 0;
    pBehavior->m_fRate = -1.0f;
    m_behaviorList.m_pInternal->AddTail(pBehavior);

    pBehavior = new CProjectileBehavior;
    pBehavior->m_nKind = CProjectileBehavior::KIND_HOMING;
    pBehavior->m_vStart = vAim;
    pBehavior->m_vEnd = vAim;
    pBehavior->m_nSpawnCount = 0;
    pBehavior->m_nTimer = m_desiredPosTimer2 - (m_desiredPosTimer2 >> 1);
    pBehavior->m_nHandoffTimer = 0;
    pBehavior->m_oidTarget = m_oidTarget;
    pBehavior->m_nSubProjectileType = 0;
    pBehavior->m_fRate = -1.0f;
    m_behaviorList.m_pInternal->AddTail(pBehavior);
}

void CSWCProjectile::SetBehaviorListTest()
{
    Vector vAim;
    if (m_oidTarget != CGameObjectArray::INVALIDOBJID &&
        g_pAppManager->m_pClientExoApp->GetGameObject(m_oidTarget) != 0)
        vAim = Vector(0.0f, 0.0f, 0.0f);
    else
        vAim = m_desiredPosition;

    CProjectileBehavior* pBehavior = new CProjectileBehavior;
    pBehavior->m_nKind = CProjectileBehavior::KIND_BALLISTIC;
    pBehavior->m_vStart = vAim;
    pBehavior->m_vEnd = m_desiredPosition;
    pBehavior->m_nSpawnCount = 0;
    pBehavior->m_nTimer = m_desiredPosTimer2;
    pBehavior->m_nHandoffTimer = m_desiredPosTimer2 * 6 / 10;
    pBehavior->m_oidTarget = m_oidTarget;
    pBehavior->m_nSubProjectileType = 0;
    pBehavior->m_fRate = -1.0f;
    m_behaviorList.m_pInternal->AddTail(pBehavior);

    pBehavior = new CProjectileBehavior;
    pBehavior->m_nKind = CProjectileBehavior::KIND_BURST_UP;
    pBehavior->m_vStart = vAim;
    pBehavior->m_vEnd = m_desiredPosition;
    pBehavior->m_nSpawnCount = 3;
    pBehavior->m_nTimer = m_desiredPosTimer2 * 6 / 10;
    pBehavior->m_nHandoffTimer = 0;
    pBehavior->m_oidTarget = m_oidTarget;
    pBehavior->m_nSubProjectileType = 9;
    pBehavior->m_fRate = -1.0f;
    m_behaviorList.m_pInternal->AddTail(pBehavior);
}

#include "port/crtapi.h"
#include "port/libcbase.h"
#include "port/libcmath.h"
void CSWCProjectile::SetBehaviorListDoubleBallistic()
{
    Vector vHit;
    CHitInfo hitInfo(0, ~CHitInfo::kMaterialTrigger, 0);

    Vector vAim(0.0f, 0.0f, 0.0f);
    if (m_oidTarget == CGameObjectArray::INVALIDOBJID ||
        g_pAppManager->m_pClientExoApp->GetGameObject(m_oidTarget) == 0)
        vAim = m_desiredPosition;

    CAurScene* pScene = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pScene;

    CProjectileBehavior* pBehavior = new CProjectileBehavior(CProjectileBehavior::KIND_BALLISTIC,
        vAim, vAim, m_oidTarget, m_desiredPosTimer2, 0, -1.0f, 0);
    pBehavior->m_nSubProjectileType = 0;
    m_behaviorList.AddTail(pBehavior);

    hitInfo.m_bSkipObjects = 1;

    // The facing is read once, before rand; the sign test below reuses it.
    float fFaceX = m_orientation.x, fFaceY = m_orientation.y;
    float fAngle = atanf(fFaceY / fFaceX) +
                   static_cast<float>(rand_wincompatible() % 1570) / 1000.0f + 0.7853982f;
    fAngle = fAngle > 2.3561945f ? 2.3561945f : fAngle;

    float fSin, fCos;
    sincosf(fAngle, &fSin, &fCos);

    Vector vDir(fCos, fSin, 0.0f);
    if (fFaceX > 0.0f)
        vDir = Vector(-fCos, -fSin, 0.0f);

    Vector vScatter(m_desiredPosition.x + vDir.x * 3.0f, m_desiredPosition.y + vDir.y * 3.0f, 0.0f);
    if (pScene != 0 && pScene->HitCheck(Vector(vScatter.x, vScatter.y, 1000.0f),
                                        Vector(vScatter.x, vScatter.y, -1000.0f), &vHit, &hitInfo))
        vScatter.z = vHit.z;

    float fDistToGoal = magnitude(m_position - m_desiredPosition);
    unsigned int nTimer = m_desiredPosTimer2;
    float fDistToScatter = magnitude(m_desiredPosition - vScatter);
    pBehavior = new CProjectileBehavior(CProjectileBehavior::KIND_BALLISTIC, vScatter, vScatter,
        OBJECT_INVALID, static_cast<int>(fDistToScatter / (fDistToGoal / static_cast<float>(nTimer) / 2.5f)), 0, -1.0f, 0);
    pBehavior->m_nSubProjectileType = 0;
    m_behaviorList.AddTail(pBehavior);
}

void CSWCProjectile::SetBehaviorListTripleBallistic(int nCount)
{
    K2_UNUSED(nCount);
    CAurScene* pScene = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pScene;

    Vector vHit;
    CHitInfo hitInfo(0, ~CHitInfo::kMaterialTrigger, 0);

    Vector vAim(0.0f, 0.0f, 0.0f);
    if (m_oidTarget == CGameObjectArray::INVALIDOBJID ||
        g_pAppManager->m_pClientExoApp->GetGameObject(m_oidTarget) == 0)
        vAim = m_desiredPosition;

    CProjectileBehavior* pBehavior;

    if (pScene != 0)
    {
        Vector vStart = m_position;
        unsigned int nTimer = m_desiredPosTimer2;
        Vector vDelta(vAim.x - vStart.x, vAim.y - vStart.y, 0.0f);

        Vector vLayer1(vStart.x + vDelta.x * 0.8f, vStart.y + vDelta.y * 0.8f, 0.0f);
        hitInfo.m_bSkipObjects = 1;
        bool b1 = pScene->HitCheck(Vector(vLayer1.x, vLayer1.y, 1000.0f),
                                   Vector(vLayer1.x, vLayer1.y, -1000.0f), &vHit, &hitInfo);
        float fZ1 = b1 ? vHit.z : 0.0f;
        int nId1 = b1 ? hitInfo.m_nHitMaterialBit : -1;

        Vector vLayer2(vStart.x + vDelta.x * 0.9f, vStart.y + vDelta.y * 0.9f, 0.0f);
        bool b2 = pScene->HitCheck(Vector(vLayer2.x, vLayer2.y, 1000.0f),
                                   Vector(vLayer2.x, vLayer2.y, -1000.0f), &vHit, &hitInfo);
        float fZ2 = b2 ? vHit.z : 0.0f;
        int nId2 = b2 ? hitInfo.m_nHitMaterialBit : -1;

        Vector vLayer3(vStart.x + vDelta.x * 0.95f, vStart.y + vDelta.y * 0.95f, 0.0f);
        bool b3 = pScene->HitCheck(Vector(vLayer3.x, vLayer3.y, 1000.0f),
                                   Vector(vLayer3.x, vLayer3.y, -1000.0f), &vHit, &hitInfo);
        float fZ3 = b3 ? vHit.z : 0.0f;
        int nId3 = b3 ? hitInfo.m_nHitMaterialBit : -1;

        bool b4 = pScene->HitCheck(Vector(vAim.x, vAim.y, 1000.0f),
                                   Vector(vAim.x, vAim.y, -1000.0f), &vHit, &hitInfo);

        if (b1 && b2 && b3 && b4)
        {
            vAim.z = vHit.z;
            int nId4 = hitInfo.m_nHitMaterialBit;

            Vector vPoint1(vLayer1.x, vLayer1.y, fZ1);
            Vector vPoint2(vLayer2.x, vLayer2.y, fZ2);
            Vector vPoint3(vLayer3.x, vLayer3.y, fZ3);

            pBehavior = new CProjectileBehavior(CProjectileBehavior::KIND_BALLISTIC, vPoint1, vPoint1,
                m_oidTarget, static_cast<int>(0.6f * static_cast<float>(nTimer)), 0, -1.0f, 0);
            pBehavior->m_nSubProjectileType = 0;
            pBehavior->m_nGrenadeSoundRow = nId1;
            m_behaviorList.AddTail(pBehavior);

            magnitude(m_position - m_desiredPosition);
            magnitude(vPoint1 - vPoint2);

            pBehavior = new CProjectileBehavior(CProjectileBehavior::KIND_BALLISTIC, vPoint2, vPoint2,
                OBJECT_INVALID, static_cast<int>(0.2f * static_cast<float>(nTimer)), 0, -1.0f, 0);
            pBehavior->m_nSubProjectileType = 0;
            pBehavior->m_nGrenadeSoundRow = nId2;
            m_behaviorList.AddTail(pBehavior);

            magnitude(m_position - m_desiredPosition);
            magnitude(vPoint2 - vPoint3);

            pBehavior = new CProjectileBehavior(CProjectileBehavior::KIND_BALLISTIC, vPoint3, vPoint3,
                OBJECT_INVALID, static_cast<int>(0.1f * static_cast<float>(nTimer)), 0, -1.0f, 0);
            pBehavior->m_nSubProjectileType = 0;
            pBehavior->m_nGrenadeSoundRow = nId3;
            m_behaviorList.AddTail(pBehavior);

            magnitude(m_position - m_desiredPosition);
            magnitude(vPoint2 - vPoint3);

            pBehavior = new CProjectileBehavior(CProjectileBehavior::KIND_BALLISTIC, vAim, vAim,
                OBJECT_INVALID, static_cast<int>(0.1f * static_cast<float>(nTimer)), 0, -1.0f, 0);
            pBehavior->m_nSubProjectileType = 0;
            pBehavior->m_nGrenadeSoundRow = nId4;
            m_behaviorList.AddTail(pBehavior);
            return;
        }
    }

    magnitude(m_position - m_desiredPosition);
    magnitude(m_position - vAim);

    pBehavior = new CProjectileBehavior(CProjectileBehavior::KIND_BALLISTIC, vAim, vAim,
        OBJECT_INVALID, m_desiredPosTimer2, 0, -1.0f, 0);
    pBehavior->m_nSubProjectileType = 0;
    m_behaviorList.AddTail(pBehavior);
}

void CSWCProjectile::SetBehaviorListParried()
{
    Vector vAim;
    if (m_oidTarget == CGameObjectArray::INVALIDOBJID ||
        g_pAppManager->m_pClientExoApp->GetGameObject(m_oidTarget) == 0)
        vAim = m_desiredPosition;

    CProjectileBehavior* pBehavior = new CProjectileBehavior();
    pBehavior->m_vStart = vAim;
    pBehavior->m_vEnd = vAim;
    pBehavior->m_nSpawnCount = 0;
    pBehavior->m_oidTarget = m_oidTarget;
    pBehavior->m_nSubProjectileType = 1;
    pBehavior->m_nTimer = m_desiredPosTimer2;
    pBehavior->m_nHandoffTimer = 0;
    pBehavior->m_fRate = 0.0f;
    pBehavior->m_nKind = CProjectileBehavior::KIND_HOMING;
    m_behaviorList.AddTail(pBehavior);

    pBehavior = new CProjectileBehavior();
    pBehavior->m_vStart = m_vImpactPosition;
    pBehavior->m_vEnd = m_vImpactPosition;
    pBehavior->m_nSpawnCount = 0;
    pBehavior->m_oidTarget = OBJECT_INVALID;
    pBehavior->m_nSubProjectileType = 1;
    pBehavior->m_nTimer = m_nDeflectFlightTime;
    pBehavior->m_nHandoffTimer = 0;
    pBehavior->m_fRate = 0.0f;
    pBehavior->m_nKind = CProjectileBehavior::KIND_HOMING;
    m_behaviorList.AddTail(pBehavior);
}

void CSWCProjectile::SetBehaviorListDeflected()
{
    Vector vAim;
    if (m_oidTarget == CGameObjectArray::INVALIDOBJID ||
        g_pAppManager->m_pClientExoApp->GetGameObject(m_oidTarget) == 0)
        vAim = m_desiredPosition;

    CProjectileBehavior* pBehavior = new CProjectileBehavior();
    pBehavior->m_vStart = vAim;
    pBehavior->m_vEnd = vAim;
    pBehavior->m_nSpawnCount = 0;
    pBehavior->m_oidTarget = m_oidTarget;
    pBehavior->m_nSubProjectileType = 1;
    pBehavior->m_nTimer = m_desiredPosTimer2;
    pBehavior->m_nHandoffTimer = 0;
    pBehavior->m_fRate = 0.0f;
    pBehavior->m_nKind = CProjectileBehavior::KIND_HOMING;
    m_behaviorList.AddTail(pBehavior);

    pBehavior = new CProjectileBehavior();
    pBehavior->m_vStart = vAim;
    pBehavior->m_vEnd = vAim;
    pBehavior->m_nSpawnCount = 0;
    pBehavior->m_oidTarget = m_sourceObjectId;
    pBehavior->m_nSubProjectileType = 1;
    pBehavior->m_nTimer = m_desiredPosTimer2;
    pBehavior->m_nHandoffTimer = 0;
    pBehavior->m_fRate = 0.0f;
    pBehavior->m_nKind = CProjectileBehavior::KIND_HOMING;
    m_behaviorList.AddTail(pBehavior);
}

void CSWCProjectile::SetBehaviorListReflected()
{
    Vector vAim;
    if (m_oidTarget == CGameObjectArray::INVALIDOBJID ||
        g_pAppManager->m_pClientExoApp->GetGameObject(m_oidTarget) == 0)
        vAim = m_desiredPosition;

    CProjectileBehavior* pBehavior = new CProjectileBehavior();
    pBehavior->m_vStart = vAim;
    pBehavior->m_vEnd = vAim;
    pBehavior->m_nSpawnCount = 0;
    pBehavior->m_oidTarget = m_oidTarget;
    pBehavior->m_nSubProjectileType = 1;
    pBehavior->m_nTimer = m_desiredPosTimer2;
    pBehavior->m_nHandoffTimer = 0;
    pBehavior->m_fRate = m_fShieldRadius;
    pBehavior->m_nKind = CProjectileBehavior::KIND_HOMING;
    m_behaviorList.AddTail(pBehavior);

    pBehavior = new CProjectileBehavior();
    pBehavior->m_vStart = m_vImpactPosition;
    pBehavior->m_vEnd = m_vImpactPosition;
    pBehavior->m_nSpawnCount = 0;
    pBehavior->m_oidTarget = OBJECT_INVALID;
    pBehavior->m_nSubProjectileType = 1;
    pBehavior->m_nTimer = m_nDeflectFlightTime;
    pBehavior->m_nHandoffTimer = 0;
    pBehavior->m_fRate = 0.0f;
    pBehavior->m_nKind = CProjectileBehavior::KIND_HOMING;
    m_behaviorList.AddTail(pBehavior);
}

#include "port/crtapi.h"
void CSWCProjectile::SetBehaviorListSpiral()
{
    Vector vPitch;
    float fDeltaX = m_desiredPosition.x - m_position.x;
    float fDeltaY = m_desiredPosition.y - m_position.y;
    m_bSpiralCounterClockwise = static_cast<unsigned char>(rand_wincompatible() % 2);
    if (m_bSpiralCounterClockwise != 0)
        fDeltaY = -fDeltaY;
    else
        fDeltaX = -fDeltaX;
    vPitch.x = fDeltaY;
    vPitch.y = fDeltaX;

    if (vPitch.x * vPitch.x + vPitch.y * vPitch.y <= 1.0e-5f)
        vPitch = Vector(0.0f, 1.0f, 0.0f);

    Vector vUnit = normalize(vPitch);
    vPitch.x = vUnit.x * 2.0f;
    vPitch.y = vUnit.y * 2.0f;
    vPitch.z = vUnit.z * 2.0f;

    if (m_oidTarget == CGameObjectArray::INVALIDOBJID) {
        vPitch.x += m_desiredPosition.x;
        vPitch.y += m_desiredPosition.y;
        vPitch.z = m_position.z;
    }

    Vector vGoal;
    if (m_oidTarget == CGameObjectArray::INVALIDOBJID)
        vGoal = m_desiredPosition;

    int nLeadTime = m_desiredPosTimer2 - 2500;
    if (nLeadTime != 0) {
        CProjectileBehavior* pBehavior = new CProjectileBehavior(CProjectileBehavior::KIND_SPIRAL, vPitch, vGoal, m_oidTarget, nLeadTime, 0, 2.0f,
        0);
    pBehavior->m_nSubProjectileType = 0;
        m_behaviorList.AddTail(pBehavior);
    } else if (m_oidTarget == CGameObjectArray::INVALIDOBJID) {
        SetPosition(vPitch);
    } else {
        CGameObject* pTarget;
        if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->GetGameObject(
                m_oidTarget, &pTarget) == CGameObjectArray::SUCCESS) {
            CSWCObject* pTargetObject = static_cast<CSWCObject*>(pTarget);
            Vector vPos(pTargetObject->m_position.x + vPitch.x,
                        pTargetObject->m_position.y + vPitch.y, m_position.z);
            SetPosition(vPos);
        }
    }

    CProjectileBehavior* pTail = new CProjectileBehavior(CProjectileBehavior::KIND_SPIRAL_TAIL, vGoal, vGoal, m_oidTarget, 2500, 0, -1.0f,
        0);
    pTail->m_nSubProjectileType = 0;
    m_behaviorList.AddTail(pTail);
}

void CSWCProjectile::UnloadModel()
{
    if (m_pRootReference != 0) {
        m_pRootReference->AttachToObject(0, 0, 0);
        m_pRootReference->AttachToScene(0);
        delete m_pRootReference;
        m_pRootReference = 0;
    }

    if (m_pModel != 0) {
        if (m_pMuzzleFlash != 0) {
            delete m_pMuzzleFlash;
            m_pMuzzleFlash = 0;
        }
        m_pModel->UnloadModel();
        delete m_pModel;
        m_pModel = 0;
    }
}

void CSWCProjectile::UnloadModel(unsigned char which)
{
    if (m_pRootReference != 0) {
        m_pRootReference->AttachToObject(0, 0, 0);
        m_pRootReference->AttachToScene(0);
        delete m_pRootReference;
        m_pRootReference = 0;
    }

    if (m_pModel != 0)
        m_pModel->UnloadModel(which);
}

void CSWCProjectile::SetProjectileSpellId(unsigned int spellId)
{
    CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(spellId);
    if (spellId == SPELL_ID_COW || pSpell != 0) {
        m_nProjectileSpellId = spellId;
        if (spellId == SPELL_ID_COW) {
            m_impactSound = const_cast<char*>("c_cow_dead");
        } else {
            m_soundResRef = pSpell->m_projSoundResRef;
            m_impactSound = pSpell->m_projSoundResRef;
            m_bFaceCameraOnArrival = 0;
            m_nProjOrientation = pSpell->m_nProjOrientation;
        }
    }
}

int CSWCProjectile::LoadModel(const CResRef& r, unsigned char which, unsigned char b)
{
    K2_UNUSED(b);
    if (m_pModel == 0)
        m_pModel = new CSWCAnimBaseProjectile();

    if (!m_pModel->LoadModel(r, which))
        return 0;

    Gob* pModelObject = m_pModel->GetModel(which);
    pModelObject->SetExternalData(this);
    pModelObject->SetExternalDataType(CGameObject::OBJECT_TYPE_PROJECTILE);
    m_pModel->TurnOffShadows();
    return 1;
}

void CSWCProjectile::CreateMuzzleFlash(const CExoString& sModel, CAurObject* pAttachTo,
                                      const CExoString& sPart)
{
    if (sModel.IsEmpty() || pAttachTo == 0)
        return;

    m_pMuzzleFlash = NewCAurObject(sModel.CStr(), const_cast<char*>("muzzleflash"), 0, 0);
    m_pMuzzleFlash->AttachToObject(pAttachTo, sPart.CStr(), 3);
    m_pMuzzleFlash->PlayAnimation(const_cast<char*>("impact"), 1.0f, 1, 0.0f);
}

void CSWCProjectile::SetProjectileType(unsigned int nType)
{
    m_nProjectileType = nType;

    if (m_oidTarget != CGameObjectArray::INVALIDOBJID) {
        CGameObject* pTarget;
        if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->GetGameObject(
                m_oidTarget, &pTarget) == CGameObjectArray::SUCCESS) {
            CSWCObject* pTargetObject = static_cast<CSWCObject*>(pTarget);
            m_desiredPosition = pTargetObject->m_position;
        }
    }

    while (m_behaviorList.m_pInternal->m_pHead != 0) {
        CProjectileBehavior* pBehavior = static_cast<CProjectileBehavior*>(m_behaviorList.m_pInternal->RemoveHead());
        if (pBehavior != 0)
            delete pBehavior;
    }

    switch (nType) {
    case PROJECTILE_TYPE_HOMING:           SetBehaviorListHoming(); break;
    case PROJECTILE_TYPE_BALLISTIC:        SetBehaviorListBallistic(); break;
    case PROJECTILE_TYPE_HIGH_BALLISTIC:   SetBehaviorListHighBallistic(); break;
    case PROJECTILE_TYPE_BURST_UP:         SetBehaviorListBurstUp(); break;
    case PROJECTILE_TYPE_ACCELERATING:     SetBehaviorListAccelerating(); break;
    case PROJECTILE_TYPE_SPIRAL:           SetBehaviorListSpiral(); break;
    case PROJECTILE_TYPE_LINKED:           SetBehaviorListLinked(); break;
    case PROJECTILE_TYPE_BOUNCE:           SetBehaviorListBounce(); break;
    case PROJECTILE_TYPE_BURST:            SetBehaviorListBurst(); break;
    case PROJECTILE_TYPE_LINKED_BURST_UP:  SetBehaviorListLinkedBurstUp(); break;
    case PROJECTILE_TYPE_GRENADE:
    case PROJECTILE_TYPE_TRIPLE_BALLISTIC: SetBehaviorListTripleBallistic(nType); break;
    case PROJECTILE_TYPE_DOUBLE_BALLISTIC: SetBehaviorListDoubleBallistic(); break;
    case PROJECTILE_TYPE_PARRIED:          SetBehaviorListParried(); break;
    case PROJECTILE_TYPE_DEFLECTED:        SetBehaviorListDeflected(); break;
    case PROJECTILE_TYPE_REFLECTED:        SetBehaviorListReflected(); break;
    case PROJECTILE_TYPE_TEST:             SetBehaviorListTest(); break;
    default: return;
    }

    m_pCurrentBehavior = static_cast<CProjectileBehavior*>(m_behaviorList.m_pInternal->RemoveHead());
    m_desiredPosTimer = m_pCurrentBehavior->m_nTimer;
    m_desiredPosTimer2 = m_pCurrentBehavior->m_nTimer;
    m_desiredPosition = m_pCurrentBehavior->m_vStart;
    SetProjectileVelAndAccel(m_orientation);
}

void CSWCProjectile::SetDesiredTargetObject(unsigned long long targetId, const CExoString& sPart)
{
    CGameObject* pTarget = g_pAppManager->m_pClientExoApp->GetGameObject(targetId);
    if (pTarget != 0) {
        CSWCPlaceable* pPlaceable = pTarget->AsSWCPlaceable();
        CSWCDoor* pDoor = pTarget->AsSWCDoor();
        if (pPlaceable != 0) {
            m_sTargetPart = pPlaceable->m_sModelName + CExoString("_impact");
        } else if (pDoor != 0) {
            m_sTargetPart = pDoor->m_sModelName + CExoString("impc");
        } else {
            m_oidTarget = CGameObjectArray::INVALIDOBJID;
            if (sPart == "")
                m_sTargetPart = const_cast<char*>("impact");
            else
                m_sTargetPart = sPart;
        }
    } else {
        m_oidTarget = CGameObjectArray::INVALIDOBJID;
        if (sPart == "")
            m_sTargetPart = const_cast<char*>("impact");
        else
            m_sTargetPart = sPart;
    }
    m_oidTarget = targetId;
}


void CSWCProjectile::SetProjectileVelAndAccel(const Vector& v)
{
    Vector vDir;
    Vector vSpread;

    if (m_pCurrentBehavior == 0)
        return;

    Vector vAim;
    if (m_oidTarget == OBJECT_INVALID) {
        vAim = m_pCurrentBehavior->m_vStart;
    } else {
        CGameObject* pObject;
        if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->GetGameObject(
                m_oidTarget, &pObject) == CGameObjectArray::SUCCESS &&
            pObject->m_objectType > CGameObject::OBJECT_TYPE_AREA) {
            CSWCObject* pTarget = static_cast<CSWCObject*>(pObject);
            Vector vPart;
            Quaternion q;
            if (pTarget->GetModel(ANIM_MODEL_BODY, 1) == 0) {
                vAim = m_desiredPosition;
            } else if (pTarget->GetModel(ANIM_MODEL_BODY, 1)->GetPartPosition(m_sTargetPart.CStr(), &vPart,
                                                                 &q) != 0) {
                vAim = vPart + m_pCurrentBehavior->m_vStart;
            } else {
                vAim = pTarget->m_position;
                vAim.z += 1.25f;
                vAim = vAim + m_pCurrentBehavior->m_vStart;
            }
        }
    }

    vDir = vAim - m_position;
    if (vDir.x * vDir.x + vDir.y * vDir.y + vDir.z * vDir.z <= 1.0e-5f) {
        m_vVelocity = Vector(0.0f, 0.0f, 0.0f);
        m_vAcceleration = Vector(0.0f, 0.0f, 0.0f);
        return;
    }

    float fTime = static_cast<float>(m_desiredPosTimer) / 1000.0f;
    Vector vOrient = v;
    static int bWentLeft;

    switch (m_pCurrentBehavior->m_nKind) {
    case CProjectileBehavior::KIND_HOMING:
    case CProjectileBehavior::KIND_SPIRAL_TAIL:
    case CProjectileBehavior::KIND_UNK0D:
        m_vAcceleration = Vector(0.0f, 0.0f, 0.0f);
        break;
    case CProjectileBehavior::KIND_BALLISTIC:
        m_vAcceleration = Vector(0.0f, 0.0f, -9.81f);
        break;
    case CProjectileBehavior::KIND_BOUNCE:
    case CProjectileBehavior::KIND_UNK08:
    case CProjectileBehavior::KIND_HIGH_ARC:
    default:
        m_vVelocity = Vector(0.0f, 0.0f, 0.0f);
        m_vAcceleration = Vector(0.0f, 0.0f, 0.0f);
        break;
    case CProjectileBehavior::KIND_BURST_UP: {
        vOrient = normalize(vOrient);
        float fAngle = acosf(vOrient.x);
        if (vOrient.y < 0.0f)
            fAngle = 6.2831855f - fAngle;

        float fJitter = static_cast<float>(rand_wincompatible() % 900) / 10.0f * 0.017453292f;
        if (bWentLeft == 0)
            fAngle += fJitter;
        else
            fAngle -= fJitter;

        vSpread.x = cosf(fAngle);
        vSpread.y = sinf(fAngle);
        vSpread.z = static_cast<float>(rand_wincompatible() % 100) / 100.0f;
        bWentLeft = !bWentLeft;

        Vector vNormal = normalize(vSpread);
        int nRand = rand_wincompatible();
        int nRange = static_cast<int>(5.0f * logf(magnitude(vDir)));
        if (nRange < 0)
            nRange = -nRange;
        float fSpeed = static_cast<float>(nRand % (nRange + 1)) + 3.0f;

        Vector vVelocity = vNormal * fSpeed;
        m_vVelocity = vVelocity;
        m_vAcceleration = (vDir - vVelocity * fTime) * 2.0f * (1.0f / (fTime * fTime));
        break;
    }
    case CProjectileBehavior::KIND_ACCELERATING:
        m_vVelocity = Vector(0.0f, 0.0f, 0.0f);
        m_vAcceleration = (vDir - m_vVelocity * fTime) * 2.0f * (1.0f / (fTime * fTime));
        break;
    case CProjectileBehavior::KIND_LINKED: {
        m_vVelocity = Vector(0.0f, 0.0f, 0.0f);
        m_vAcceleration = Vector(0.0f, 0.0f, 0.0f);

        Gob* pModel = GetModel(ANIM_MODEL_BODY, 1);
        if (pModel == 0)
            return;
        CGameObjectArray* pArray = g_pAppManager->m_pClientExoApp->GetGameObjectArray();
        if (pArray == 0)
            return;

        CGameObject* pObject;
        if (pArray->GetGameObject(m_oidTarget, &pObject) == CGameObjectArray::SUCCESS) {
            CSWCObject* pTarget = static_cast<CSWCObject*>(pObject);
            Vector vPart;
            Quaternion q;
            if (pTarget->GetModel(ANIM_MODEL_BODY, 1) == 0)
                return;
            int nFound =
                pTarget->GetModel(ANIM_MODEL_BODY, 1)->GetPartPosition(m_sTargetPart.CStr(), &vPart, &q);
            Gob* pPartModel = pTarget->GetModel(ANIM_MODEL_BODY, 1);
            if (nFound != 0)
                pModel->ReattachReference(static_cast<CAurObject*>(pPartModel), m_sTargetPart.CStr());
            else
                pModel->ReattachReference(static_cast<CAurObject*>(pPartModel), const_cast<char*>("root"));
            return;
        }

        m_pRootReference = NewCAurObject(const_cast<char*>("ref"), const_cast<char*>("dummytarget"), 0, 0);
        if (m_pRootReference == 0)
            return;
        m_pRootReference->SetPosition(m_pCurrentBehavior->m_vStart);
        pModel->ReattachReference(static_cast<CAurObject*>(m_pRootReference), const_cast<char*>("root"));
        break;
    }
    case CProjectileBehavior::KIND_BURST: {
        vOrient = normalize(vOrient);
        float fAngle = acosf(vOrient.x);
        if (vOrient.y < 0.0f)
            fAngle = 6.2831855f - fAngle;

        float fJitter = static_cast<float>(rand_wincompatible() % 900) / 10.0f * 0.017453292f;
        if (bWentLeft == 0)
            fAngle += fJitter;
        else
            fAngle -= fJitter;

        vSpread.x = cosf(fAngle);
        vSpread.y = sinf(fAngle);
        int nSide = rand_wincompatible();
        vSpread.z = static_cast<float>(rand_wincompatible() % (nSide % 2 == 0 ? 50 : 100)) /
                    (nSide % 2 == 0 ? -100.0f : 100.0f);
        bWentLeft = !bWentLeft;

        Vector vNormal = normalize(vSpread);
        int nRand = rand_wincompatible();
        int nRange = static_cast<int>(5.0f * logf(magnitude(vDir)));
        if (nRange < 0)
            nRange = -nRange;
        float fSpeed = static_cast<float>(nRand % (nRange + 1)) + 3.0f;

        Vector vVelocity = vNormal * fSpeed;
        m_vVelocity = vVelocity;
        m_vAcceleration = (vDir - vVelocity * fTime) * 2.0f * (1.0f / (fTime * fTime));
        break;
    }
    case CProjectileBehavior::KIND_SPIRAL: {
    vOrient.z = 0.0f;
    Vector vPerp = normalize(vOrient) * 7.5f;
    m_vAcceleration = vPerp;
    m_vVelocity = vDir * (2.0f / fTime) - vPerp;
    m_vAcceleration.x = (vPerp.x * vPerp.x - m_vVelocity.x * m_vVelocity.x) / (2.0f * vDir.x);
    m_vAcceleration.y = (vPerp.y * vPerp.y - m_vVelocity.y * m_vVelocity.y) / (2.0f * vDir.y);
    break;
    }

    case CProjectileBehavior::KIND_LINKED_BURST: {
        vOrient = normalize(vOrient);
        float fAngle = acosf(vOrient.x);
        if (vOrient.y < 0.0f)
            fAngle = 6.2831855f - fAngle;

        float fJitter = static_cast<float>(rand_wincompatible() % 900) / 10.0f * 0.017453292f;
        if (bWentLeft == 0)
            fAngle += fJitter;
        else
            fAngle -= fJitter;

        vSpread.x = cosf(fAngle);
        vSpread.y = sinf(fAngle);
        vSpread.z = static_cast<float>(rand_wincompatible() % 100) / 100.0f;
        bWentLeft = !bWentLeft;

        Vector vNormal = normalize(vSpread);
        int nRand = rand_wincompatible();
        int nRange = static_cast<int>(5.0f * logf(magnitude(vDir)));
        if (nRange < 0)
            nRange = -nRange;
        float fSpeed = static_cast<float>(nRand % (nRange + 1)) + 3.0f;

        Vector vVelocity = vNormal * fSpeed;
        m_vVelocity = vVelocity;
        m_vAcceleration = (vDir - vVelocity * fTime) * 2.0f * (1.0f / (fTime * fTime));

        m_pRootReference = NewCAurObject(const_cast<char*>("ref"), const_cast<char*>(""), 0, 0);
        if (m_pRootReference == 0)
            return;

        Gob* pModel = GetModel(ANIM_MODEL_BODY, 1);
        Vector vPos = pModel->GetPosition();
        m_pRootReference->SetPosition(vPos);
        vPos.z += 0.01f;
        pModel->SetPosition(vPos);
        m_pRootReference->SetOrientation(pModel->GetOrientation());
        m_pRootReference->AttachToScene(pModel->GetScene());
        pModel->ReattachReference(m_pRootReference, const_cast<char*>("root"));
        break;
    }
}
}

void CSWCProjectile::MoveProjectileToPosition()
{
    Vector vAim;

    if (m_pCurrentBehavior == 0) {
        m_bArrived = 1;
        return;
    }

    if (m_pCurrentBehavior->m_oidTarget == CGameObjectArray::INVALIDOBJID) {
        m_desiredPosition = m_pCurrentBehavior->m_vStart;
    } else {
        Quaternion q;
        CGameObject* pTargetObject;
        if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->GetGameObject(
                m_pCurrentBehavior->m_oidTarget, &pTargetObject) == CGameObjectArray::SUCCESS) {
            CSWCObject* pTarget = static_cast<CSWCObject*>(pTargetObject);
            if (pTarget->GetModel(ANIM_MODEL_BODY, 1) == 0) {
                vAim = m_desiredPosition;
            } else if (pTarget->GetModel(ANIM_MODEL_BODY, 1)->GetPartPosition(m_sTargetPart.CStr(), &vAim,
                                                                  &q) == 0) {
                unsigned char bImpactTarget = m_nImpactMode;
                vAim = pTarget->m_position;
                vAim.z += 1.25f;
                if ((bImpactTarget & ~1) == 2)
                    vAim = vAim + m_orientation * -0.5f;
            }
            m_pCurrentBehavior->m_vEnd = vAim;
            m_desiredPosition = vAim + m_pCurrentBehavior->m_vStart;
        } else {
            m_desiredPosition = m_pCurrentBehavior->m_vStart;
        }
    }

    if (m_nAIUpdateDeltaMs < m_desiredPosTimer) {
        unsigned int nDelta = m_desiredPosTimer - m_nAIUpdateDeltaMs;
        if (nDelta > static_cast<unsigned int>(m_pCurrentBehavior->m_nHandoffTimer)) {
            m_desiredPosTimer = nDelta;
            goto skipTick;
        }
    } else {
        m_nAIUpdateDeltaMs -= m_desiredPosTimer;
        m_desiredPosTimer = 0;
        SetPosition(m_desiredPosition);
    }

    if (!UpdateCurrentBehavior())
        return;
    m_desiredPosTimer -= m_nAIUpdateDeltaMs;

skipTick:
    if (m_pCurrentBehavior == 0)
        return;

    if (m_pCurrentBehavior->m_oidTarget == CGameObjectArray::INVALIDOBJID) {
        m_desiredPosition = m_pCurrentBehavior->m_vStart;
    } else {
        Quaternion q;
        CGameObject* pTargetObject;
        if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->GetGameObject(
                m_pCurrentBehavior->m_oidTarget, &pTargetObject) == CGameObjectArray::SUCCESS) {
            CSWCObject* pTarget = static_cast<CSWCObject*>(pTargetObject);
            if (pTarget->GetModel(ANIM_MODEL_BODY, 1) == 0) {
                vAim = m_desiredPosition;
            } else if (pTarget->GetModel(ANIM_MODEL_BODY, 1)->GetPartPosition(m_sTargetPart.CStr(), &vAim,
                                                                  &q) == 0) {
                unsigned char bImpactTarget = m_nImpactMode;
                vAim = pTarget->m_position;
                vAim.z += 1.25f;
                if ((bImpactTarget & ~1) == 2)
                    vAim = vAim + m_orientation * -0.5f;
            }
            m_pCurrentBehavior->m_vEnd = vAim;
            m_desiredPosition = vAim + m_pCurrentBehavior->m_vStart;
        }
        // Unlike the first aim block, a failed lookup leaves m_desiredPosition untouched.
    }

    float fGoalX = m_desiredPosition.x - m_position.x;
    float fGoalY = m_desiredPosition.y - m_position.y;
    float fGoalZ = m_desiredPosition.z - m_position.z;
    if (fGoalX * fGoalX + fGoalY * fGoalY + fGoalZ * fGoalZ <= 1.0e-5f) {
        SetPosition(m_desiredPosition);
        if (!(m_pCurrentBehavior->m_fRate >= 0.0f))
            return;
    } else {
        if (m_pCurrentBehavior->m_fRate >= 0.0f &&
            m_pCurrentBehavior->m_oidTarget != CGameObjectArray::INVALIDOBJID) {
            float fAimX = m_position.x - vAim.x;
            float fAimY = m_position.y - vAim.y;
            float fAimZ = m_position.z - vAim.z;
            float fRate = m_pCurrentBehavior->m_fRate;
            if (fAimX * fAimX + fAimY * fAimY + fAimZ * fAimZ <= fRate * fRate)
                goto tickBehavior;
        }

        m_vPreviousPosition = m_position;
        switch (m_pCurrentBehavior->m_nKind) {
        case CProjectileBehavior::KIND_HOMING:
            MoveProjectileAccelerating();
            break;
        case CProjectileBehavior::KIND_BALLISTIC:
            MoveProjectileAccelerating();
            break;
        case CProjectileBehavior::KIND_BURST_UP:
            MoveProjectileAccelerating();
            break;
        case CProjectileBehavior::KIND_ACCELERATING:
            MoveProjectileAccelerating();
            break;
        case CProjectileBehavior::KIND_BURST:
            MoveProjectileAccelerating();
            break;
        case CProjectileBehavior::KIND_SPIRAL:
            MoveProjectileAccelerating();
            break;
        case CProjectileBehavior::KIND_SPIRAL_TAIL:
            MoveProjectileSpiral();
            break;
        case CProjectileBehavior::KIND_LINKED:
            break;
        case CProjectileBehavior::KIND_LINKED_BURST:
            MoveProjectileAccelerating();
            break;
        case CProjectileBehavior::KIND_UNK0D:
            // The one kind that both accelerates and forces the model into THROW_BACK.
            MoveProjectileAccelerating();
            if (m_pModel != 0 && m_pModel->SetAnimation(CSWCAnimBaseProjectile::PROJECTILE_ANIMATION_THROW_BACK, 1.0f, 0, 0.0f) != 0)
                m_currentAnimation = CSWCAnimBaseProjectile::PROJECTILE_ANIMATION_THROW_BACK;
            break;
        // The four no-step ids take no step at all.
        case CProjectileBehavior::KIND_BOUNCE:
            break;
        case CProjectileBehavior::KIND_UNK08:
            break;
        case CProjectileBehavior::KIND_HIGH_ARC:
            break;
        default:
            break;
        }
        return;
    }

tickBehavior:
    // UpdateCurrentBehavior may pop the chain, so its timer debit is banked around it.
    int nTimer = m_desiredPosTimer;
    UpdateCurrentBehavior();
    m_desiredPosTimer += nTimer;
}

int CSWCProjectile::UpdateCurrentBehavior()
{
    if (m_pCurrentBehavior != 0) {
        delete m_pCurrentBehavior;
        m_pCurrentBehavior = 0;
    }

    while (m_behaviorList.m_pInternal->m_pHead != 0) {
        m_pCurrentBehavior = static_cast<CProjectileBehavior*>(m_behaviorList.RemoveHead());
        if (m_pCurrentBehavior == 0)
            return 0;

        if (m_pCurrentBehavior->m_nKind == CProjectileBehavior::KIND_BALLISTIC &&
            m_pCurrentBehavior->m_nGrenadeSoundRow != -1) {
            C2DA* pGrenadeSound = g_pRules->m_p2DArrays->m_pGrenadeSound;
            CExoString sSound;
            if (pGrenadeSound->GetCExoStringEntry(m_pCurrentBehavior->m_nGrenadeSoundRow, "SOUND",
                                                  &sSound) != 0) {
                g_pExoSound->Play3DOneShotSound(CResRef(sSound), m_position, 0.0f, PRIORITY_GROUP_COMBAT, 0, ONE_SHOT_VOLUME_FULL,
                                                0.0f, 0.0f);
            }
        }

        if (m_pCurrentBehavior->m_nKind == CProjectileBehavior::KIND_HIGH_ARC) {
            SetAnimation(CSWCAnimBaseProjectile::PROJECTILE_ANIMATION_DEFAULT);
            (GetModel(ANIM_MODEL_BODY, 1))->CAurObject::AnimateExternal(1.0f);

            if (m_pCurrentBehavior->m_oidTarget != CGameObjectArray::INVALIDOBJID) {
                CGameObject* pObject;
                if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->GetGameObject(
                        m_pCurrentBehavior->m_oidTarget, &pObject) == CGameObjectArray::SUCCESS) {
                    CSWCObject* pTargetObject = static_cast<CSWCObject*>(pObject);
                    Vector v = {pTargetObject->m_position.x + m_pCurrentBehavior->m_vStart.x,
                                pTargetObject->m_position.y + m_pCurrentBehavior->m_vStart.y,
                                pTargetObject->m_position.z + m_pCurrentBehavior->m_vStart.z};
                    SetPosition(v);
                }
            } else {
                SetPosition(m_pCurrentBehavior->m_vStart);
            }

            (GetModel(ANIM_MODEL_BODY, 1))->CAurObject::AnimateExternal(1.0f);
            SetAnimation(CSWCAnimBaseProjectile::PROJECTILE_ANIMATION_TRAVEL);
        }

        if (m_pCurrentBehavior->m_nSpawnCount != 0) {
            for (unsigned char i = 0; i < m_pCurrentBehavior->m_nSpawnCount; i++) {
                CSWSpell* pSpell = g_pRules->m_pSpellArray->GetSpell(m_nProjectileSpellId);
                if (pSpell == 0)
                    break;

                CSWCProjectile* pNew = new CSWCProjectile();
                pNew->SetProjectileSpellId(m_nProjectileSpellId);
                pNew->LoadModel(pSpell->m_projectileResRef, ANIM_MODEL_BODY, 0);

                CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
                if (pModule != 0 && pModule->m_pArea != 0)
                    pNew->AttachToScene(pModule->m_pArea->m_pScene);

                pNew->SetPosition(m_position);
                pNew->SetOrientation(m_orientation);
                pNew->m_desiredPosTimer = m_pCurrentBehavior->m_nTimer;
                pNew->m_desiredPosTimer2 = m_pCurrentBehavior->m_nTimer;
                pNew->SetAnimation(GetCurrentAnimation());
                pNew->m_desiredPosition = m_pCurrentBehavior->m_vStart;
                pNew->SetDesiredTargetObject(m_oidTarget, CExoString(""));
                pNew->SetProjectileType(m_pCurrentBehavior->m_nSubProjectileType);
            }
        }

        m_desiredPosTimer = m_pCurrentBehavior->m_nTimer;
        m_desiredPosTimer2 = m_pCurrentBehavior->m_nTimer;
        m_desiredPosition = m_pCurrentBehavior->m_vStart;
        SetProjectileVelAndAccel(m_orientation);

        if (m_nProjectileType == PROJECTILE_TYPE_DEFLECTED) {
            m_bTargetFlinched = 1;
            SetDesiredTargetObject(m_pCurrentBehavior->m_oidTarget, CExoString("impact"));
        } else if (m_nProjectileType == PROJECTILE_TYPE_REFLECTED) {
            if (m_pArea != 0)
                m_pArea->ApplyFireAndForgetVisualEffect(VFX_COM_SPARKS_PARRY_METAL, m_position);
        }

        if (m_pCurrentBehavior->m_nKind != CProjectileBehavior::KIND_HIGH_ARC &&
            m_nAIUpdateDeltaMs < m_desiredPosTimer)
            return 1;
    }

    return 0;
}

void CSWCProjectile::MoveProjectileAccelerating()
{
    Vector vDir = m_desiredPosition - m_position;
    float fTime = static_cast<float>(m_desiredPosTimer) / 1000.0f;

    Vector vVelocity = (vDir + vDir - m_vAcceleration * (fTime * fTime)) * (1.0f / (fTime + fTime));
    m_vVelocity = vVelocity;

    float fStep = static_cast<float>(m_nAIUpdateDeltaMs) / 1000.0f;
    Vector vNew = m_position + vVelocity * fStep;

    Vector vStep = m_position - vNew;
    if (vStep.x * vStep.x + vStep.y * vStep.y + vStep.z * vStep.z >
        vDir.x * vDir.x + vDir.y * vDir.y + vDir.z * vDir.z)
        vNew = m_desiredPosition;

    // The facing reuses vDir: other ProjOrientation values face along the desired-minus-position delta.
    if (m_nProjOrientation != 0) {
        if (m_nProjOrientation == 1)
            vDir = m_pCurrentBehavior->m_vEnd - vNew;
        else if (m_nProjOrientation == 2)
            vDir = vNew - m_position;

        if (vDir.x * vDir.x + vDir.y * vDir.y + vDir.z * vDir.z > 1.0e-5f)
            SetOrientation(normalize(vDir));
    }

    SetPosition(vNew);
}

void CSWCProjectile::MoveProjectileSpiral()
{
    Vector vNew;
    Vector vFace;
    Vector vDir = m_desiredPosition - m_position;
    float fTime = static_cast<float>(m_desiredPosTimer) / 1000.0f;

    if (m_bSpiralCounterClockwise != 0) {
        vFace.x = -vDir.y;
        vFace.y = vDir.x;
    } else {
        vFace.x = vDir.y;
        vFace.y = -vDir.x;
    }
    vFace.z = 0.0f;

    Vector vVelocity = normalize(vFace) * 7.5f + vDir * (1.0f / fTime);
    m_vVelocity = vVelocity;

    float fStep = static_cast<float>(m_nAIUpdateDeltaMs) / 1000.0f;
    vNew = m_position + vVelocity * fStep;

    Vector vStep = m_position - vNew;
    if (vStep.x * vStep.x + vStep.y * vStep.y + vStep.z * vStep.z >
        vDir.x * vDir.x + vDir.y * vDir.y + vDir.z * vDir.z)
        vNew = m_desiredPosition;

    // The facing reuses vDir: other ProjOrientation values face along the desired-minus-position delta.
    if (m_nProjOrientation != 0) {
        if (m_nProjOrientation == 1)
            vDir = m_pCurrentBehavior->m_vEnd - vNew;
        else if (m_nProjOrientation == 2)
            vDir = vNew - m_position;

        if (vDir.x * vDir.x + vDir.y * vDir.y + vDir.z * vDir.z > 1.0e-5f)
            SetOrientation(normalize(vDir));
    }

    SetPosition(vNew);
}

void CSWCProjectile::MoveProjectileToFinalVelocity()
{
    Vector vDir = m_desiredPosition - m_position;
    float fTime = static_cast<float>(m_desiredPosTimer) / 1000.0f;

    Vector vVelocity = vDir * (2.0f / fTime) - m_vAcceleration;
    m_vVelocity = vVelocity;

    float fStep = static_cast<float>(m_nAIUpdateDeltaMs) / 1000.0f;
    Vector vNew = m_position + vVelocity * fStep;

    Vector vStep = m_position - vNew;
    if (vStep.x * vStep.x + vStep.y * vStep.y + vStep.z * vStep.z >
        vDir.x * vDir.x + vDir.y * vDir.y + vDir.z * vDir.z)
        vNew = m_desiredPosition;

    // The facing reuses vDir: other ProjOrientation values face along the desired-minus-position delta.
    if (m_nProjOrientation != 0) {
        if (m_nProjOrientation == 1)
            vDir = m_pCurrentBehavior->m_vEnd - vNew;
        else if (m_nProjOrientation == 2)
            vDir = vNew - m_position;

        if (vDir.x * vDir.x + vDir.y * vDir.y + vDir.z * vDir.z > 1.0e-5f)
            SetOrientation(normalize(vDir));
    }

    SetPosition(vNew);
}

void CSWCProjectile::AIUpdate()
{
    // Intentionally unused.
    CExoString sUnused;

    unsigned int nDay, nTod;
    unsigned int nElapsedDay, nElapsedTod;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nDay, &nTod);
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTod, m_nLastAIUpdateDay, m_nLastAIUpdateTod, &nElapsedDay, &nElapsedTod);
    m_nAIUpdateDeltaMs = nElapsedTod;

    // MoveProjectileToPosition consumes m_desiredPosTimer; on the arrived path bank and restore it.
    if (m_bArrived == 1) {
        unsigned int nTimer = m_desiredPosTimer;
        m_desiredPosTimer = 0;
        MoveProjectileToPosition();
        m_desiredPosTimer = nTimer;
    } else {
        MoveProjectileToPosition();
    }

    if (m_bUpdateEnabled == 1) {
        if (m_pSoundSource != 0)
            m_pSoundSource->SetPosition(m_position, 1.5f);

        if (m_bArrived == 1) {
            // Arrived.
            Quaternion qCamera =
                g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera->GetOrientation();

            if (m_bFaceCameraOnArrival != 0) {
                SetOrientation(Vector(cosf((Yaw(qCamera) - 90.0f) * 0.017453292f),
                                      sinf((Yaw(qCamera) - 90.0f) * 0.017453292f), 0.0f));
            }

            if (m_nAIUpdateDeltaMs < m_desiredPosTimer)
                m_desiredPosTimer -= m_nAIUpdateDeltaMs;
            else
                m_desiredPosTimer = 0;

            if (m_desiredPosTimer == 0) {
                AttachToScene(0);
                if (m_pSoundSource != 0)
                    delete m_pSoundSource;
                delete this;
                return;
            }
        } else {
            // In flight.
            if (GetCurrentAnimation() == CSWCAnimBaseProjectile::PROJECTILE_ANIMATION_TRAVEL ||
                GetCurrentAnimation() == CSWCAnimBaseProjectile::PROJECTILE_ANIMATION_THROW_OUT ||
                GetCurrentAnimation() == CSWCAnimBaseProjectile::PROJECTILE_ANIMATION_THROW_BACK ||
                GetCurrentAnimation() == SERVER_ANIMATION_PAUSE) {
                if (m_bFlightSoundStarted == 0) {
                    if (m_pSoundSource != 0) {
                        delete m_pSoundSource;
                        m_pSoundSource = 0;
                    }
                    if (m_soundResRef.GetResRef()[0] != 0) {
                        m_pSoundSource = new CExoSoundSource(m_soundResRef);
                        m_pSoundSource->SetPriorityGroup(PRIORITY_GROUP_PROJECTILE);
                        m_pSoundSource->Set3D(1);
                        m_pSoundSource->SetLooping(1);
                        m_pSoundSource->SetPosition(m_position, 1.5f);
                        m_pSoundSource->Play();
                    }
                }

                // The target's "got shot" flinch, for the parried and deflected bolts only, while the
                // target is mid-animation, alive and (if a party member) still conscious.
                if (m_desiredPosTimer < FLINCH_LEAD_MS && m_nProjectileType >= PROJECTILE_TYPE_PARRIED &&
                    m_bTargetFlinched == 0) {
                    CSWCCreature* pCreature =
                        g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_oidTarget);
                    if (pCreature != 0 &&
                        static_cast<CSWCObject*>(pCreature)
                                ->AnimationFireAndForget(
                                    static_cast<CSWCObject*>(pCreature)->GetCurrentAnimation()) == 0) {
                        CSWSCreature* pServer = pCreature->GetServerCreature();
                        if (pServer != 0 && pServer->GetDead() == 0 &&
#if KOTOR1  // K1 tests GetDeadTemp() instead of the party/consciousness check
                            pServer->GetDeadTemp() == 0 &&
#else
                            (pServer->m_bInParty == 0 || pServer->GetCurrentHitPoints(0) >= 1) &&
#endif
                            (m_nProjectileType & ~1u) == PROJECTILE_TYPE_PARRIED) {
                            int nRoll = rand_wincompatible();
                            static_cast<CSWCObject*>(pCreature)->GetAnimationBase()
                                ->SetOverlayAnimation((nRoll & 1) ^ SERVER_ANIMATION_DEFLECT_2, 1);
                            m_bTargetFlinched = 1;
                        }
                    }
                }

                m_bFlightSoundStarted = 1;

                if (m_desiredPosTimer == 0) {
                    // The arrival tick starts the impact animation, impact sound and hit visuals.
                    if (m_pModel != 0 && m_pModel->SetAnimation(CSWCAnimBaseProjectile::PROJECTILE_ANIMATION_IMPACT, 1.0f, 0, 0.0f) != 0)
                        m_currentAnimation = CSWCAnimBaseProjectile::PROJECTILE_ANIMATION_IMPACT;

                    m_bArrived = 1;

                    if (m_pSoundSource != 0) {
                        m_pSoundSource->Stop();
                        delete m_pSoundSource;
                        m_pSoundSource = 0;
                    }
                    if (m_impactSound.IsValid() == 1)
                        g_pExoSound->Play3DOneShotSound(m_impactSound, m_position, 0.0f, PRIORITY_GROUP_PROJECTILE, 0,
                                                        0, 0.0f, 0.0f);

                    if (m_bStickToTarget == 0) {
                        m_desiredPosTimer = 0;
                    } else {
                        CGameObject* pObject;
                        if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->GetGameObject(
                                m_oidTarget, &pObject) == CGameObjectArray::SUCCESS) {
                            CSWCObject* pTargetObject = static_cast<CSWCObject*>(pObject);

                            Vector vPart;
                            Quaternion qPart;

                            CAurObject* pOtherModel = 0;
                            if (m_nImpactMode == 2) {
                                CSWCItem* pItem =
                                    g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
                                        pTargetObject->AsSWCCreature()->GetEquippedItemID(CSWInventory::SLOT_LEFTWEAPON));
                                if (pItem != 0 &&
                                    g_pRules->m_pBaseItemArray
                                            ->GetBaseItem(pItem->m_baseItem)
                                            ->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_NONE) {
                                    if (pTargetObject->m_bNoProjectileAttach == 0)
                                        pOtherModel = static_cast<CAurObject*>(pItem->GetModel(ANIM_MODEL_BODY, 1));
                                    else
                                        m_nImpactLingerMs = 0;
                                }
                            } else if (m_nImpactMode == 0) {
                                if (pTargetObject->GetAnimationBase() != 0) {
                                    if (pTargetObject->m_bNoProjectileAttach == 0)
                                        pOtherModel =
                                            static_cast<CAurObject*>(pTargetObject->GetModel(ANIM_MODEL_BODY, 1));
                                    else
                                        m_nImpactLingerMs = 0;
                                }
                            }

                            Gob* pModel = GetModel(ANIM_MODEL_BODY, 1);
                            if (pOtherModel != 0 && pModel != 0) {
                                if (m_nImpactMode == 0) {
                                    pModel->AttachToObject(pOtherModel, m_sTargetPart.CStr(), 0);
                                } else if (m_nImpactMode == 2) {
                                    CExoString sShieldName;
                                    sShieldName.Format(const_cast<char*>("imp_shield_%d"),
                                                       rand_wincompatible() % 2 + 1);
                                    pModel->AttachToObject(pOtherModel, sShieldName.CStr(), 0);
                                } else {
                                    // Any other value skips the root re-orientation.
                                    goto skip_root;
                                }

                                pModel->GetPartPosition(const_cast<char*>("root"), &vPart, &qPart);
                                qPart = YawPitchRoll(Yaw(m_orientation), Pitch(m_orientation), 0.0f);
                                pModel->SetPartPosition(const_cast<char*>("root"), vPart, qPart);
                            }
                        skip_root:

                            if (pTargetObject->AsSWCCreature() != 0 &&
                                m_bSpawnCombatVisualOnImpact != 0) {
                                Vector vImpact = m_position - m_vPreviousPosition;
                                CSWCCombatAttackData* pAttack = new CSWCCombatAttackData();
                                pAttack->m_nAttackResult = (m_spawnCombatVisualId != 0) ? 2 : 1;
                                pAttack->m_nTotalDamage = static_cast<unsigned short>(m_spawnCombatVisualParam);
                                pAttack->m_rangedAttackGroup.m_bRangedAttack = 1;
                                pTargetObject->AsSWCCreature()->SpawnHitVisuals(
                                    m_bSpawnCombatVisualOnImpact, pAttack, vImpact, m_sTargetPart);
                                delete pAttack;
                                m_bSpawnCombatVisualOnImpact = 0;
                            }
                        }
                        m_desiredPosTimer = m_nImpactLingerMs;
                    }
                }
            }
        }
    }

    m_nLastAIUpdateDay = nDay;
    m_nLastAIUpdateTod = nTod;
}
