#define K1_AURORA_LIST  // K1: the line-of-sight hit lists grow from 8
#include "k2stdint.h"
#include "nwccreature.h"
#include "nullname.h"
#include "CExoArrayListImpl.h"
#include "AURORA/aurmodel.h"
#include "nwccreatureappearance.h"
#include "nwcarea.h"
#include "nwccreaturestats.h"
#include "nwcprojectile.h"
#include "gameobjectarray.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "ClientExoAppInternal.h"
#include "nwcobject.h"
#include "nwcplaceable.h"
#include "nwcmodule.h"
#include "nwcdoor.h"
#include "nwctrigger.h"
#include "nwsobject.h"
#include "nwsplaceable.h"
#include "nwsitem.h"
#include "nwscreature.h"
#include "nwscombatround.h"
#include "nwsplayer.h"
#include "nwsarea.h"
#include "nwarea.h"
#include "SWCollisionMesh.h"
#include "nwrace.h"
#include "nwseffectlist.h"
#include "nwseffect.h"
#include "CExoArrayListDtors.h"
#include "serverexoapp.h"
#include "serverexoappinternal.h"
#include "globalvartables.h"
#include "nwsrules.h"
#include "nwcitem.h"
#include "nwspell.h"
#include "nwsitemproperty.h"
#include "guiingame.h"
#include "swguiexamine.h"
#include "swgui.h"
#include "swguimaininterfacepc2.h"
#include "nwchandlemessage.h"
#include "exostring.h"
#include "exofile.h"
#include "worldtimer.h"
#include "nwscript.h"
#include "nwclass.h"
#include "nwskill.h"
#include "nwitem.h"
#include "resgff.h"
#include "exolocstring.h"
#include "exosound.h"
#include "soundset.h"
#include "exoresman.h"
#include "exolocstringinternal.h"
#include "exoiniinternal.h"
#include "lip.h"
#include "nwcvisualeffect.h"
#include "aurscene.h"
#include "serveranimation.h"
#include "itemtype.h"
#include "soundprioritygroup.h"
#include "tutorial.h"
#include "enginevfx.h"

extern int g_bLogConversations;

extern int RenderPosition;

const int CSWCCreature::MODE_DEFAULT = 0;
const int CSWCCreature::MODE_SWDRIVE = 1;

#include "port/crtapi.h"
#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcstdlib.h"
#include "port/libcmath.h"
extern int hitcheck_static_trimesh_parts;
extern float fCombatTurnSpeed;
extern int quaternionidentityoptimization;

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
        r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) + v.z * ((xz + wy) + (xz + wy));
        r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) + v.z * ((yz - wx) + (yz - wx));
        r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) + v.z * (1.0f - ((xx + yy) + (xx + yy)));
        return r;
    }
    return v;
}
extern int g_bShowIndicator;
int RenderClosestPathPoint;
int RenderTriangles;
int renderwalktry;
int renderintersects;
#if KOTOR1
float IosDisableInitiateDialogTime;   // a script's lockout on the player starting a conversation
#endif
extern int enablemodelrequest;
extern Vector zaxis;
extern float creaturewindintensity;
#if KOTOR1
uint64_t AurAddDrawableSquare(float* pPoints, uint64_t nColor);
#else
uint64_t AurAddDrawableSquare(float* pPoints, uint64_t nColor, const char* pFile, int nLine);
#endif
void AurRemoveDrawableSquare(uint64_t nId);
float Yaw(const Vector& orientation);
static void snprintff(char* pBuf, uint64_t nSize, const char* pFmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, pFmt);
    __vsprintf_chk(pBuf, 0, nSize, pFmt, ap);
    __builtin_va_end(ap);
}
static unsigned int s_walkTrySquareIds[8] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
                                             0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
void SwitchWeaponEvent(CAurObject* pObject, char* pEvent, void* pData);
void FootstepEvent(CAurObject*, char*, void*);
void HitEvent(CAurObject*, char*, void*);
void HitGroundEvent(CAurObject*, char*, void*);
void SwingShortEvent(CAurObject*, char*, void*);
void SwingLongEvent(CAurObject*, char*, void*);
void SwingTwirlEvent(CAurObject*, char*, void*);
void HitClashEvent(CAurObject*, char*, void*);
void HitContactEvent(CAurObject*, char*, void*);
void HitParryEvent(CAurObject*, char*, void*);
void Blur(CAurObject*, char*, void*);
void Unblur(CAurObject*, char*, void*);
void GetPersonalRadius(CAurObject*, char*, void*);
void GetCreatureRadius(CAurObject*, char*, void*);
void GetPath(CAurObject*, char*, void*);
extern CExoSound* g_pExoSound;
int g_nCombatAnimation;
extern unsigned char g_initiatedByController;
float DriveWalkThreshold = 0.600000024f;
int g_nHackAlignment = -1;
bool AurCheckFrameBufferModificationsAvailable();
#if KOTOR1
extern int preferPerfToVisual;
#endif
#include "twodimarrays.h"
#include "clientoptions.h"
#include "swparty.h"
#include "partytables.h"
#include "exolinkedlistinternal.h"
#include "nwcspellvisual.h"
#include "nwcanimbase.h"
#include "nwcanimbasetw.h"
#include "nwcanimbasehead.h"
#include "nwcanimbasewield.h"
#include "nwcanimbaseheadwield.h"
#include "aurorainterface.h"
#include "nwsdoor.h"
#include "nwsmodule.h"
#include "swsroom.h"
#include "swroomsurfacemesh.h"
#include "swguicontrol.h"
#include "AURORA/aurbehavior.h"
#include "clientaimaster.h"
#include "nwscriptconst.h"
#include "nwmessagetypes.h"
#include "nwportrait.h"
#include "strref.h"
#include "exostringinline.h"

#if KOTOR1
// KOTOR 2's spells.2da rows the shared code names; KOTOR 1's nwscript.nss has no such powers.
enum {
    FORCE_POWER_DROID_TRICK     = 201,
    FORCE_POWER_DROID_CONFUSION = 269,
    FORCE_POWER_WOOKIEE_RAGE_I  = 271,
};
#endif

// CSWGuiInterfaceAction::m_nFlags: bit 0 enables the entry, bits 1-4 say why it is disabled (the
// main interface's hint strings: force depleted, restricted by armor, missing item, target too close,
// full health, party leader unavailable).
enum {
    ACTION_FLAG_ENABLED                       = 0x01,
    ACTION_FLAG_REASON_MASK                   = 0x1e,
    ACTION_FLAG_REASON_FORCE_DEPLETED         = 0x02,
    ACTION_FLAG_REASON_RESTRICTED_BY_ARMOR    = 0x04,
    ACTION_FLAG_REASON_MISSING_ITEM           = 0x06,
    ACTION_FLAG_REASON_TARGET_TOO_CLOSE       = 0x08,
    ACTION_FLAG_REASON_FULL_HEALTH            = 0x0a,
    ACTION_FLAG_REASON_PARTY_LEADER_UNAVAILABLE = 0x0c,
    ACTION_FLAG_REASON_NOT_ENOUGH_HEALTH      = 0x0e,
};

// GetTargetInterfaceTargetType's results: the radial menu row a target selects.
enum {
    TARGET_INTERFACE_OTHER    = -1,
    TARGET_INTERFACE_OPENABLE = 0,      // a door or a placeable with an inventory
    TARGET_INTERFACE_TRIGGER  = 1,
    TARGET_INTERFACE_USABLE   = 2,      // a non-hostile creature or any other placeable
    TARGET_INTERFACE_HOSTILE  = 3,
    TARGET_INTERFACE_NONE     = 0xff,
};

// The AISTATE column of aiscripts.2da (CSWSCreatureStats::m_aiState), the party behaviours.
enum {
    AI_STATE_AGGRESSIVE     = 9,
    AI_STATE_DEFENSIVE      = 10,
    AI_STATE_RANGED_SUPPORT = 11,
    AI_STATE_STATIONARY     = 12,
    AI_STATE_SUPPORT        = 13,   // jedi support, or grenadier for a non-jedi
};

// The m_nAIStateAction values (attacking, 3 and casting) that hold the ready stance in place of
// the idle pause.
enum {
    AI_STATE_ACTIONS_READY_STANCE = (1 << CSWSCreature::AI_STATE_ACTION_ATTACKING) | (1 << 3) |
                                    (1 << CSWSCreature::AI_STATE_ACTION_CASTING),
};

// The attack results that landed, which spawn the target's hit visuals.
enum {
    ATTACK_RESULTS_LANDED = (1 << ATTACK_RESULT_HIT_SUCCESSFUL) | (1 << ATTACK_RESULT_CRITICAL_HIT) |
                            (1 << ATTACK_RESULT_AUTOMATIC_HIT) | (1 << ATTACK_RESULT_ATTACK_RESISTED),
};

// The rows of surfacemat.2da (CSWCArea::GetSurfaceMaterial) this file tests.
enum {
    SURFACE_MATERIAL_DIRT    = 1,
    SURFACE_MATERIAL_GRASS   = 3,
    SURFACE_MATERIAL_STONE   = 4,
    SURFACE_MATERIAL_WOOD    = 5,
    SURFACE_MATERIAL_WATER   = 6,
    SURFACE_MATERIAL_CARPET  = 9,
    SURFACE_MATERIAL_METAL   = 10,
    SURFACE_MATERIAL_PUDDLES = 11,
    SURFACE_MATERIAL_SWAMP   = 12,
    SURFACE_MATERIAL_MUD     = 13,
    SURFACE_MATERIAL_LEAVES  = 14,
    SURFACE_MATERIAL_LAVA    = 15,
};

// PlaySoundSetSound ids (the sound-set entry plus one) CSWSCreature does not name.
enum {
    VOICE_CHAT_PAIN_1        = 13,
    VOICE_CHAT_PAIN_2        = 14,
    VOICE_CHAT_BEGIN_STEALTH = 21,
};


CSWCCreature::CSWCCreature()
    : CSWCObject(CGameObject::OBJECT_TYPE_CREATURE)
{
    m_nStateFlags = (m_nStateFlags & 0xfc) | 2;
    m_bInParty = 0;
#if !KOTOR1
    m_bIsPuppet = 0;
#endif
    m_pSoundSet = 0;
    m_nSoundSetFile = -1;
#if KOTOR1
    m_pStats = new CSWCLevelUpStats();
#else
    m_pStats = new CSWCLevelUpStats(this);
#endif
    m_wayPointCount = 0;
    m_pWayPoints = 0;
    m_pCurrentWayPoint = 0;
    m_currentAnimation = SERVER_ANIMATION_PAUSE;
    m_bConjureActive = 0;
    m_bSuppressWeaponDraw = 0;
    m_nReserved2 = 0;
    m_bMovementSpeedLimited = 0;
    m_bOverrideLightsaberState = 0;
    m_wieldedItemIdRight = OBJECT_INVALID;
    m_wieldedItemIdLeft = OBJECT_INVALID;
    m_desiredPositionTimer = 0;
    m_pCurrentAttackData = new CSWCCombatAttackData();
    m_pCurrentCastData = new CSWCCombatAttackData();
#if KOTOR1
    for (int i = 0; i < 18; i++) {
#else
    for (int i = 0; i < 20; i++) {
#endif
        m_aEquippedItemIds[i] = CGameObjectArray::INVALIDOBJID;
        m_aLastInstant[i] = 0;
    }
    unsigned int nWorldTimeDay;
    unsigned int nWorldTimeMs;
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetWorldTime(&nWorldTimeDay, &nWorldTimeMs);
    m_nLastAIUpdateDay = nWorldTimeDay;
    m_nLastAIUpdateTod = nWorldTimeMs;
    m_pCreatureAppearance = new CSWCCreatureAppearance(this);
    m_commandTag = 0;
    m_appliedCommandTag = 0;
    m_nClientPathId = 0;
    m_bServerWayPoints = 1;
    m_bHasWayPointSpeed = 0;
    m_orientation.x = 1.0f;
    m_orientation.y = 0.0f;
    m_orientation.z = 0.0f;
    m_vMovingOrientation.x = 1.0f;
    m_vMovingOrientation.y = 0.0f;
    m_vMovingOrientation.z = 0.0f;
    m_vStationaryOrientation.x = 1.0f;
    m_vStationaryOrientation.y = 0.0f;
    m_vStationaryOrientation.z = 0.0f;
    m_loopingAnimation = SERVER_ANIMATION_PAUSE;
    m_blurWeapon = 0;
    m_pLoopingCreatureSound = 0;
    m_bLoopingSoundStarted = 0;
    m_nReserved3 = 0;
    m_bPlot = 0;
    m_nReserved4 = 1;
    m_pSpellCastVisual = new CSWCSpellCastVisual();
    m_lockOrientationObjectId = CGameObjectArray::INVALIDOBJID;
    m_nAIStateFlags = 0xffff;
    m_nReserved1 = 0;
    m_bLastFootstepVisual = 0;
    m_fFootstepGroundZ = 0.0f;
    for (int i = 0; i < 12; i++)
        m_aHitVisualGobs[i] = 0;
    m_pNastyVisualGob = 0;
    m_pReserved1 = 0;
    m_pReserved2 = 0;
    m_bInCombat = 0;
    m_pReserved3 = 0;
    for (int i = 0; i < 4; i++)
        m_aSpecialAttackGobs[i] = 0;
    m_pMonkVisuals = 0;
    m_bMonkEyesLoaded = 0;
    m_bMonkEyes01Loaded = 0;
    m_bDetectMode = 0;
    m_nStealthState = 0;
    m_bParryMode = 0;
    m_bPowerAttackMode = 0;
    m_bImprovedPowerAttackMode = 0;
    m_nReserved5 = 0;
    m_bRapidShotMode = 0;
    m_bFlurryMode = 0;
    m_bVisibilityEffectApplied = 0;
    m_nReputation = 2;
    m_nPlayerInfoExtra = 0;
    m_nReserved6 = 200000;
    m_pPartyStatus = 0;
    m_nGold = 0;
#if !KOTOR1
    m_nItemComponent = 0;
    m_nChemicals = 0;
#endif
    m_conjureTargetId = CGameObjectArray::INVALIDOBJID;
    m_nRoundTimer = 6000;
    m_nWeaponOverlayTimer = 2000;
    m_pNewAnimBase = 0;
    m_fPendingMoveSpeedScale = 0;
    m_nAppearanceRequestTimer = 0;
    m_bCameraReattachPending = 0;
    m_bCameraFindClearView = 0;
    m_bDisguised = 0;
    m_bCommandable = 0;
    m_bExcited = 0;
    for (int i = 0; i < 4; i++)
        m_aLookAtTargetIds[i] = CGameObjectArray::INVALIDOBJID;
    m_nAmbientPauseElapsed = 0;
    m_bAmbientPausePlaying = 0;
    m_nAmbientPauseDelay = rand_wincompatible() % 4 * 30000 + 60000;
    m_fWayPointSpeed = 1.0f;
    m_areaObjectId = CGameObjectArray::INVALIDOBJID;
    m_pSpellLikeAbilityList = 0;
    m_switchMode = 0;
    m_nReserved7 = 0;
    m_swAnimationSpeed = 0.0f;
    m_fSWStrafeSpeed = 0.0f;
    m_fTurnAngle = 0;
    m_nSWAnimation = SERVER_ANIMATION_NONE;
    m_attackIndex = -1;
    m_lastAttackIndex = -1;
    m_nForcedAttackAnimation = 0;
    m_engagedState = 0;
    m_bDisableLookAt = 0;
    m_thrownLightsaber = OBJECT_INVALID;
    m_bEquippedItemsHidden = 0;
    m_bHeadItemHidden = 0;
    m_oidBodyBag = OBJECT_INVALID;
    m_bRunning = 0;
    m_lookAtTargetId = OBJECT_INVALID;
    for (int i = 0; i < 17; i++)
        m_aAnimationCallbacks[i] = 0;
    m_pRollingFootstepSound = 0;
    m_rollingFootstepVariance = 0.0f;
    m_bRollingFootstepPlaying = 0;
    m_rollingFootstepType = 0;
    m_pendingDrawAnimation = 0xffff;
    m_fFreeLookTurnVel = 0.0f;
#if !KOTOR1
    m_nSwitchWeaponsCooldown = -1;
    m_nLastAIUpdateTime = 0;
#endif
}

CSWCCreaturePartyStatus::CSWCCreaturePartyStatus()
{
    m_bAIStateFlag = 1;
    m_bSingletonParty = 1;
    m_bFactionLeader = 0;
    m_objectId = OBJECT_INVALID;
    m_sName = "";
}

CSWCCreaturePartyStatus::~CSWCCreaturePartyStatus()
{
}

CSWCCreature::~CSWCCreature()
{
    delete m_pRollingFootstepSound;
    delete m_pSpellLikeAbilityList;
    if (m_pSoundSet) {
        delete m_pSoundSet;
        m_pSoundSet = 0;
    }
    if (m_pStats) {
        delete m_pStats;
        m_pStats = 0;
    }
    if (m_pReserved3) {
        delete m_pReserved3;
        m_pReserved3 = 0;
    }
    if (m_pLoopingCreatureSound) {
        delete m_pLoopingCreatureSound;
        m_pLoopingCreatureSound = 0;
    }
    delete m_pSpellCastVisual;
    delete m_pCreatureAppearance;
    delete[] m_pWayPoints;
    UnloadModel();
    delete m_pCurrentAttackData;
    delete m_pCurrentCastData;
    for (int i = 0; i < 12; i++) {
        delete m_aHitVisualGobs[i];
        m_aHitVisualGobs[i] = 0;
    }
    delete m_pNastyVisualGob;
    for (int i = 0; i < 4; i++) {
        if (m_aSpecialAttackGobs[i]) {
            m_aSpecialAttackGobs[i]->AttachToScene(0);
            delete m_aSpecialAttackGobs[i];
            m_aSpecialAttackGobs[i] = 0;
        }
    }
    Gob* pMonkVisuals = m_pMonkVisuals;
    if (pMonkVisuals) {
        pMonkVisuals->AttachToScene(0);
        delete m_pMonkVisuals;
    }
    delete m_pReserved1;
    delete m_pReserved2;
    if (m_pPartyStatus) {
        delete m_pPartyStatus;
        m_pPartyStatus = 0;
    }
    m_thrownLightsaber = CGameObjectArray::INVALIDOBJID;
    ClearVisibilityList();
    g_pAppManager->m_pClientExoApp->ClearCachedCreatures(this);
}

void* CSWCCreature::GetCurrentWayPoint() {
    return m_pCurrentWayPoint;
}

void CSWCCreature::SetEngagedState(int engagedState) {
    m_engagedState = engagedState;
}

void CSWCCreature::SetThrownLightsaber(unsigned long long id) {
    m_thrownLightsaber = id;
}

void CSWCCreature::ResetDesiredPositionTimer() {
    m_desiredPositionTimer = 512;
}

// Syncs to a server-sent position: caches it, re-grounds the model, retimes the walk or run
// animation from the appearance distances and the remaining path, repositions the looping
// sounds and rotates toward the cached orientation.
void CSWCCreature::ClientServerSyncHandler_Position(const Vector& v, int anim, float f) {
    m_desiredPosition = v;
    MoveCreatureToPosition(v);
    SetGroundNormal(Vector(0.0f, 0.0f, v.z));
    if (AnimationStationary(static_cast<unsigned short>(anim)))
        return;
    float fCol = 0.0f;
#ifdef K2_FIX_ENGINE_BUGS
    float fScale = m_fWalkRate;   // neither walking nor running: the walk rate
#else
    float fScale;
#endif
    const int* pCol;
    if (AnimationWalking(static_cast<unsigned short>(anim))) {
        fScale = m_fWalkRate;
        pCol = &CTwoDimArrays::COL2DA_APPEARANCETABLE_WALKDIST;
    } else {
        if (AnimationRunning(static_cast<unsigned short>(anim)) == 0)
            goto noColumn;
        fScale = m_fRunRate;
        float fRate = m_pCreatureAppearance->GetRunLooksAwfulRate();
        GetAnimationBase()->m_runLooksAwful =
            (fScale < fRate);
        pCol = &CTwoDimArrays::COL2DA_APPEARANCETABLE_RUNDIST;
    }
    g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
        m_pCreatureAppearance->m_nAppearanceId, *pCol, &fCol);
    fCol *= 1000.0f;
noColumn:;
    float fSpeed = 0.0f;
    if (m_pCurrentWayPoint) {
        int nClass = m_bServerWayPoints;
        float fDist = 0.0f;
        Vector prev = {m_desiredPosition.x, m_desiredPosition.y, 0.0f};
        Vector* p = static_cast<Vector*>(m_pCurrentWayPoint);
        do {
            Vector cur = {p->x, p->y, 0.0f};
            p++;
            Vector d = {cur.x - prev.x, cur.y - prev.y, 0.0f};
            fDist += magnitude(d);
            prev = cur;
        } while (p < m_pWayPoints + m_wayPointCount);
        float fThresh = (nClass == 1) ? 500.0f : 1000.0f;
        float fRef = (fScale * 1000.0f) / 2000000.0f;
        float fNew = 1.0f;
        if (fDist <= fRef) {
            fNew = fDist / fRef;
            fNew = (fNew >= 0.01f) ? __builtin_sqrtf(fNew) : 0.1f;
            fNew = fNew + static_cast<float>(m_nAIUpdateDeltaMs) / -1000.0f;
        }
        float fOld = m_fMoveSpeedScale;
        if (fOld <= 1.0f) {
            float fCap = fOld + static_cast<float>(m_nAIUpdateDeltaMs) / fThresh;
            if (fNew >= fCap)
                fNew = fCap;
        }
        fSpeed = fNew <= 0.1f ? 0.1f : fNew;
    }
    m_fMoveSpeedScale = fSpeed;
    float fLen0 = GetAnimationLength(GetCurrentAnimation());
    float fRate11 = 0.0f;
    if (fLen0 != 0.0f && fCol != 0.0f)
        fRate11 = (fScale / 1000.0f) / (fCol / fLen0);
    if (anim == SERVER_ANIMATION_RUN && f < 0.5f) {
        float fLenE6 = m_pModel->GetAnimationLength(m_nLowerGaitAnimation);
        g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
            m_pCreatureAppearance->m_nAppearanceId,
            CTwoDimArrays::COL2DA_APPEARANCETABLE_WALKDIST, &fCol);
        fCol *= 1000.0f;
        fRate11 = 0.0f;
        if (fLenE6 != 0.0f && fCol != 0.0f)
            fRate11 = (fScale / 1000.0f) / (fCol / fLenE6);
        float f16 = m_pModel->GetAnimationTime(GetCurrentAnimation());
        if (f16 > 0.0f) {
            float fCur = m_pModel->GetAnimationLength(GetCurrentAnimation());
            m_pModel->SetAnimation(m_nLowerGaitAnimation, fSpeed * fRate11, 0, fLenE6 * (f16 / fCur) / 1000.0f);
        } else {
            m_pModel->SetAnimation(m_nLowerGaitAnimation, fSpeed * fRate11, 0, 0.0f);
        }
    } else {
        if (m_pModel->m_runLooksAwful == 1) {
            float fLenE6 = m_pModel->GetAnimationLength(m_nLowerGaitAnimation);
            g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
                m_pCreatureAppearance->m_nAppearanceId,
                CTwoDimArrays::COL2DA_APPEARANCETABLE_WALKDIST, &fCol);
            fCol *= 1000.0f;
            fRate11 = 0.0f;
            if (fLenE6 != 0.0f && fCol != 0.0f)
                fRate11 = (fScale / 1000.0f) / (fCol / fLenE6);
        }
        float f16 = m_pModel->GetAnimationTime(m_nLowerGaitAnimation);
        if (f16 > 0.0f) {
            float fLenE6 = m_pModel->GetAnimationLength(m_nLowerGaitAnimation);
            float fCur = m_pModel->GetAnimationLength(GetCurrentAnimation());
            m_pModel->SetAnimation(GetCurrentAnimation(), fSpeed * fRate11, 0,
                                   (f16 / fLenE6) * fCur / 1000.0f);
        } else {
            m_pModel->SetAnimation(GetCurrentAnimation(), fSpeed * fRate11, 0, 0.0f);
        }
    }
    if (m_pLoopingCreatureSound)
        m_pLoopingCreatureSound->SetPosition(v, 1.5f);
    if (m_pStreamingVocalization)
        m_pStreamingVocalization->SetPosition(v);
    RotateCreatureToOrientation(AnimationStationary(static_cast<unsigned short>(anim)) ? static_cast<const Vector&>(m_vStationaryOrientation)
                                                          : static_cast<const Vector&>(m_vMovingOrientation));
}

void CSWCCreature::StopAmbientPauseAnim() {
    m_nAmbientPauseElapsed = 0;
    m_bAmbientPausePlaying = 0;
}

void CSWCCreature::SetSWAnimationSpeed(float speed, float speed2) {
    m_fSWStrafeSpeed = speed2;
    m_swAnimationSpeed = speed;
}

void CSWCCreature::SwitchMode(int mode) {
    if (static_cast<unsigned>(mode) <= 2)
        m_switchMode = mode;
}

int CSWCCreature::IsHostile() {
    return m_nReputation == 2;
}

void CSWCCreature::HideEquippedHeadItem() {
    if (m_pModel) {
        m_pModel->HideHeadItem();
        m_bHeadItemHidden = 1;
    }
}

void CSWCCreature::ShowEquippedHeadItem() {
    if (m_pModel) {
        m_pModel->ShowHeadItem();
        m_bHeadItemHidden = 0;
    }
}

// Not implemented.
void CSWCCreature::HideEquippedItemsForCasting(const CResRef& r1, const CResRef& r2) {
    K2_UNUSED(r1); K2_UNUSED(r2);
}

// Not implemented.
void CSWCCreature::HandleItemProperties(CSWCItem* pItem, unsigned int n) {
    K2_UNUSED(pItem); K2_UNUSED(n);
}

// The unimplemented counterpart of IsHostileUsableItem; always returns 0.
int CSWCCreature::IsFriendlyUsableItem(CSWSItem* pItem) {
    K2_UNUSED(pItem);
    return 0;
}

// Base item type 6 always qualifies; a launcher qualifies when its ammo type is equipped in
// either weapon slot. Nothing qualifies in menu-disabled areas.
int CSWCCreature::IsHostileUsableItem(CSWSItem* pItem) {
#if KOTOR1
// K1: base type 6 only (no launcher and ammo pair).
    if (pItem != 0 && m_pArea->GetServerArea()->m_nPlayerRestrictMode == 0 &&
        pItem->GetBaseItem()->m_nItemType == ITEM_TYPE_GRENADE)
        return 1;
    return 0;
#else
    if (pItem == 0 || m_pArea->GetServerArea()->m_nPlayerRestrictMode != 0)
        return 0;
    if (pItem->GetBaseItem()->m_nItemType == ITEM_TYPE_GRENADE)
        return 1;
    if (pItem->GetBaseItem()->m_nItemType != ITEM_TYPE_ROCKET)
        return 0;
    GetServerObject();
    CSWSItem* pEquipped = static_cast<CSWSItem*>(
        GetServerObject()->AsSWSCreature()->m_pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTARM));
    if (pEquipped != 0 && pEquipped->GetBaseItem()->m_nItemType == ITEM_TYPE_WRIST_LAUNCHER)
        return 1;
    GetServerObject();
    pEquipped = static_cast<CSWSItem*>(
        GetServerObject()->AsSWSCreature()->m_pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTARM));
    return pEquipped != 0 && pEquipped->GetBaseItem()->m_nItemType == ITEM_TYPE_WRIST_LAUNCHER;
#endif
}

// Action-menu usability gate for a leader. A not-for-sale item is gated by its properties:
// grenades go through the Demolitions skill check, other items need a usable property, a
// carried copy for the race-locked base types, or a usable spell. A for-sale item instead
// needs the party to have the right race for the locked base types.
int CSWCCreature::IsLeaderUsableItem(CSWSItem* pItem, int unused, unsigned char nType) {
    K2_UNUSED(unused);
    if (pItem == 0)
        return 0;
    CSWSCreature* pLeader = 0;
    if (GetServerObject())
        pLeader = GetServerObject()->AsSWSCreature();
    CSWSArea* pArea = m_pArea->GetServerArea();
    if (pLeader == 0)
        return 0;
    CSWItemProperty* pProperty;
    CSWBaseItem* pBaseItem;
    unsigned char nBaseType;
    if (pItem->m_bPlot != 0) {
        if ((nType & 2) == 0 || pItem->GetBaseItem()->m_nItemType != ITEM_TYPE_TRAP_KIT) {
            if (pItem->GetPropertyByType(&pProperty, ITEM_PROPERTY_ACTIVATE_ITEM, 0) == 0)
                return 0;
            pBaseItem = pItem->GetBaseItem();
            if (pBaseItem != 0) {
                nBaseType = pBaseItem->m_nItemType;
                if (nBaseType <= ITEM_TYPE_FOREARM_BANDS) {
bit_test:
                    if (((1 << nBaseType) & ((1 << ITEM_TYPE_DROID_UTILITY_DEVICE) | (1 << ITEM_TYPE_DROID_SHIELD) |
                                             (1 << ITEM_TYPE_FOREARM_BANDS))) != 0) {
                        int bCarried = pLeader->m_pInventory->GetItemInInventory(pItem);
                        if ((nType & 4) == 0 || bCarried == 0)
                            return 0;
                        goto spell_check;
                    }
                }
                if ((nType & 4) == 0)
                    return 0;
            }
spell_check:
            CSWSpell* pSpell = g_pRules->GetSpell(pProperty->m_subType);
            if (pSpell == 0)
                return 0;
            if (pSpell->m_nItemTargeting != 1)
                return 0;
        } else {
skill_check:
            if (pItem->GetPropertyByType(&pProperty, ITEM_PROPERTY_TRAP, 0) != 0) {
                if (pProperty->m_bUseableByte == 0)
                    return 0;
                if (pLeader->m_pStats->GetCanUseSkill(SKILL_DEMOLITIONS) == 0)
                    return 0;
            }
        }
        return pLeader->CanUseItem(pItem, 1, 1);
    }
    if (pArea->m_nPlayerRestrictMode != 0)
        return 0;
#if KOTOR1
// K1: the race-locked base types are gated by CanUseItem alone.
    if (((nType & 1) != 0 &&
         (pItem->GetBaseItem()->m_nItemType == ITEM_TYPE_MEDICAL_EQUIPMENT ||
          pItem->GetBaseItem()->m_nItemType == ITEM_TYPE_SQUAD_RECOVERY_KIT ||
          pItem->GetBaseItem()->m_nItemType == ITEM_TYPE_DROID_REPAIR_EQUIPMENT)) ||
        ((nType & 4) != 0 && pItem->GetBaseItem()->m_nItemType == ITEM_TYPE_STIMULANT)) {
        if (pItem->GetPropertyByType(&pProperty, ITEM_PROPERTY_ACTIVATE_ITEM, 0) != 0 &&
            pProperty->m_bUseableByte == 0)
            return 0;
        return pLeader->CanUseItem(pItem, 1, 1);
    }
    if ((nType & 2) != 0 && pItem->GetBaseItem()->m_nItemType == ITEM_TYPE_TRAP_KIT)
        goto skill_check;
    if (pItem->GetPropertyByType(&pProperty, ITEM_PROPERTY_ACTIVATE_ITEM, 0) == 0)
        return 0;
    if (pProperty->m_bUseableByte == 0)
        return 0;
    pBaseItem = pItem->GetBaseItem();
    if (pBaseItem == 0)
        goto spell_check;
    nBaseType = pBaseItem->m_nItemType;
    if (nBaseType <= ITEM_TYPE_FOREARM_BANDS)
        goto bit_test;
    if ((nType & 4) == 0)
        return 0;
    goto spell_check;
#else
    if ((nType & 2) != 0 && pItem->GetBaseItem()->m_nItemType == ITEM_TYPE_TRAP_KIT)
        goto skill_check;
    if ((nType & 1) == 0 ||
        (pItem->GetBaseItem()->m_nItemType != ITEM_TYPE_STIMULANT &&
         pItem->GetBaseItem()->m_nItemType != ITEM_TYPE_MEDICAL_EQUIPMENT &&
         pItem->GetBaseItem()->m_nItemType != ITEM_TYPE_SQUAD_RECOVERY_KIT &&
         pItem->GetBaseItem()->m_nItemType != ITEM_TYPE_DROID_REPAIR_EQUIPMENT)) {
        if (pItem->GetPropertyByType(&pProperty, ITEM_PROPERTY_ACTIVATE_ITEM, 0) == 0)
            return 0;
        if (pProperty->m_bUseableByte == 0)
            return 0;
        pBaseItem = pItem->GetBaseItem();
        if (pBaseItem == 0)
            goto spell_check;
        nBaseType = pBaseItem->m_nItemType;
        if (nBaseType <= ITEM_TYPE_FOREARM_BANDS)
            goto bit_test;
        if ((nType & 4) == 0)
            return 0;
        goto spell_check;
    }
    if (pItem->GetPropertyByType(&pProperty, ITEM_PROPERTY_ACTIVATE_ITEM, 0) != 0 &&
        pProperty->m_bUseableByte == 0)
        return 0;
    nBaseType = pItem->GetBaseItem()->m_nItemType;
    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    const unsigned short* pRace = &CSWRace::CRACE_HUMAN;
    if (nBaseType == ITEM_TYPE_DROID_REPAIR_EQUIPMENT)
        pRace = &CSWRace::CRACE_DROID;
    return pParty->PartyHasRace(*pRace) != 0;
#endif
}
// Replaces the soundset with one loaded from the given row.

void CSWCCreature::SetSoundSet(unsigned short nSoundSet) {
    if (static_cast<unsigned short>(m_nSoundSetFile) == nSoundSet)
        return;
    m_nSoundSetFile = nSoundSet;
    if (m_pSoundSet != 0) {
        delete m_pSoundSet;
        m_pSoundSet = 0;
    }
    if (static_cast<unsigned short>(m_nSoundSetFile) == 0xffff)
        return;
    CSoundSetList* pSoundSetList = g_pAppManager->m_pClientExoApp->GetSoundSetList();
    if (pSoundSetList == 0)
        return;
    CResRef resRef = pSoundSetList->GetSoundSet(m_nSoundSetFile);
    if (resRef == const_cast<char*>(""))
        return;
    m_pSoundSet = new CSoundSet(resRef);
}

// Replaces the way-point array: an unchanged count copies in place, a different count drops
// the old array and value-initializes a fresh one.
__attribute__((noinline))
void CSWCCreature::SetWayPoints(unsigned short nCount, Vector* pPoints) {
    if (m_wayPointCount == nCount) {
        memcpy(m_pWayPoints, pPoints, nCount * sizeof(Vector));
        m_pCurrentWayPoint = m_pWayPoints;
    } else {
        m_wayPointCount = nCount;
        if (m_pWayPoints != 0)
            delete[] m_pWayPoints;
        if (nCount != 0) {
            Vector* pNewPoints = new Vector[nCount]();
            m_pWayPoints = pNewPoints;
            m_pCurrentWayPoint = pNewPoints;
            memcpy(pNewPoints, pPoints, nCount * sizeof(Vector));
        } else {
            m_pWayPoints = 0;
            m_pCurrentWayPoint = 0;
        }
    }
}

static const int RANGED_ATTACK_TABLE_A[5] = {362, 363, 364, 0, 365};
static const int RANGED_ATTACK_TABLE_C[5] = {218, 232, 240, 0, 353};
static const int RANGED_ATTACK_TABLE_B[5] = {219, 233, 241, 0, 354};
static const int RANGED_ATTACK_TABLE_D[5] = {217, 231, 239, 0, 352};

int CSWCCreature::UpdateRangedAttackData(int a, int b) {
    int bCreature = IsCreature();
    switch (a) {
    case FEAT_IMPROVED_POWER_BLAST: case FEAT_POWER_BLAST: case FEAT_MASTER_POWER_BLAST:
        if (bCreature)
            return CSWCAnimBase::CLIENT_ANIMATION_DROID_POWER_BLAST;
        {
            unsigned int n = b - 5u;
            if (n < 5)
                return RANGED_ATTACK_TABLE_A[static_cast<int>(n)];
        }
        return 0;
    case FEAT_IMPROVED_SNIPER_SHOT: case FEAT_SNIPER_SHOT: case FEAT_MASTER_SNIPER_SHOT:
        if (bCreature)
            return CSWCAnimBase::CLIENT_ANIMATION_DROID_SNIPER_SHOT;
        {
            unsigned int n = b - 5u;
            if (n < 5)
                return RANGED_ATTACK_TABLE_B[static_cast<int>(n)];
        }
        return 0;
    case FEAT_MULTI_SHOT: case FEAT_RAPID_SHOT: case FEAT_IMPROVED_RAPID_SHOT:
        if (bCreature)
            return CSWCAnimBase::CLIENT_ANIMATION_DROID_BLASTER_ATTACK_2;
        {
            unsigned int n = b - 5u;
            if (n < 5)
                return RANGED_ATTACK_TABLE_C[static_cast<int>(n)];
        }
        return 0;
    default:
        if (bCreature)
            return CSWCAnimBase::CLIENT_ANIMATION_DROID_BLASTER_ATTACK_1;
        {
            unsigned int n = b - 5u;
            if (n < 5)
                return RANGED_ATTACK_TABLE_D[static_cast<int>(n)];
        }
        return 0;
    }
}

#if !KOTOR1  // K1 has no rand-picked melee variants
static const short MELEE_ATTACK_TABLE_A2[3] = {113, 392, 393};
static const short MELEE_ATTACK_TABLE_A3[3] = {154, 413, 414};
static const short MELEE_ATTACK_TABLE_A4[3] = {195, 434, 435};
static const short MELEE_ATTACK_TABLE_A8[3] = {494, 495, 496};
static const short MELEE_ATTACK_TABLE_B2[3] = {114, 394, 395};
static const short MELEE_ATTACK_TABLE_B3[3] = {155, 415, 416};
static const short MELEE_ATTACK_TABLE_B4[3] = {196, 436, 437};
static const short MELEE_ATTACK_TABLE_B8[3] = {497, 498, 499};
static const short MELEE_ATTACK_TABLE_C2[3] = {115, 396, 397};
static const short MELEE_ATTACK_TABLE_C3[3] = {156, 417, 418};
static const short MELEE_ATTACK_TABLE_C4[3] = {197, 438, 439};
static const short MELEE_ATTACK_TABLE_C8[3] = {500, 501, 502};
static const short MELEE_ATTACK_TABLE_D2[5] = {122, 123, 410, 411, 412};
static const short MELEE_ATTACK_TABLE_D3[5] = {163, 164, 431, 432, 433};
static const short MELEE_ATTACK_TABLE_D4[5] = {204, 205, 452, 453, 454};
static const short MELEE_ATTACK_TABLE_D8[5] = {521, 522, 523, 524, 525};
#endif

int CSWCCreature::UpdateMeleeAttackData(unsigned short a, int b, int c, int d) {
#if KOTOR1
// K1: no rand-picked attack variants and no flurry arms; every id is relative to
// m_attackIndex or a fixed per-weapon-class constant.
    if (b == 0 || IsCreature()) {
        int nIdx = m_attackIndex;
        if (a == SERVER_ANIMATION_ATTACK_2) {
            if (c == 0) {
                switch (d) {
                case CSWCAnimBase::WEAPON_CLASS_STUN_BATON:
                    return nIdx % 2 + CSWCAnimBase::CLIENT_ANIMATION_STUN_BATON_ATTACK_1;
                case CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER:
                    return nIdx + CSWCAnimBase::CLIENT_ANIMATION_SINGLE_SABER_ATTACK_1;
                case CSWCAnimBase::WEAPON_CLASS_TWO_HANDED_SABER:
                    return nIdx + CSWCAnimBase::CLIENT_ANIMATION_TWO_HANDED_SABER_ATTACK_1;
                case CSWCAnimBase::WEAPON_CLASS_DUAL_SABER:
                    return nIdx + CSWCAnimBase::CLIENT_ANIMATION_DUAL_SABER_ATTACK_1;
                case CSWCAnimBase::WEAPON_CLASS_NATURAL:
                    return nIdx % 2 + CSWCAnimBase::CLIENT_ANIMATION_NATURAL_ATTACK_1;
                default: return 0;
                }
            }
            if (IsCreature()) {
                if (GetAnimationBase()->AnimationExists(CSWCAnimBase::CLIENT_ANIMATION_MONSTER_ATTACK_1))
                    return nIdx + CSWCAnimBase::CLIENT_ANIMATION_MONSTER_ATTACK_1;
            } else {
                switch (d) {
                case CSWCAnimBase::WEAPON_CLASS_STUN_BATON:
                    return nIdx + CSWCAnimBase::CLIENT_ANIMATION_STUN_BATON_ATTACK_1;
                case CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER:
                    return nIdx + CSWCAnimBase::CLIENT_ANIMATION_SINGLE_SABER_ATTACK_MONSTER_1;
                case CSWCAnimBase::WEAPON_CLASS_TWO_HANDED_SABER:
                    return nIdx + CSWCAnimBase::CLIENT_ANIMATION_TWO_HANDED_SABER_ATTACK_MONSTER_1;
                case CSWCAnimBase::WEAPON_CLASS_DUAL_SABER:
                    return nIdx + CSWCAnimBase::CLIENT_ANIMATION_DUAL_SABER_ATTACK_MONSTER_1;
                case CSWCAnimBase::WEAPON_CLASS_NATURAL:
                    return nIdx + CSWCAnimBase::CLIENT_ANIMATION_NATURAL_ATTACK_1;
                default: return 0;
                }
            }
        } else if (!IsCreature()) {
            switch (d) {
            case CSWCAnimBase::WEAPON_CLASS_STUN_BATON:
                return nIdx + CSWCAnimBase::CLIENT_ANIMATION_STUN_BATON_ATTACK_1;
            case CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER:
                return nIdx + CSWCAnimBase::CLIENT_ANIMATION_SINGLE_SABER_GENERIC_ATTACK_1;
            case CSWCAnimBase::WEAPON_CLASS_TWO_HANDED_SABER:
                return nIdx + CSWCAnimBase::CLIENT_ANIMATION_TWO_HANDED_SABER_GENERIC_ATTACK_1;
            case CSWCAnimBase::WEAPON_CLASS_DUAL_SABER:
                return nIdx + CSWCAnimBase::CLIENT_ANIMATION_DUAL_SABER_GENERIC_ATTACK_1;
            case CSWCAnimBase::WEAPON_CLASS_NATURAL:
                return nIdx + CSWCAnimBase::CLIENT_ANIMATION_NATURAL_ATTACK_1;
            default: return 0;
            }
        }
        return nIdx + CSWCAnimBase::CLIENT_ANIMATION_MONSTER_GENERIC_ATTACK_1;
    }
    switch (b) {
    case FEAT_CRITICAL_STRIKE: case FEAT_IMPROVED_CRITICAL_STRIKE: case FEAT_MASTER_CRITICAL_STRIKE:
        switch (d) {
        case CSWCAnimBase::WEAPON_CLASS_STUN_BATON: return CSWCAnimBase::CLIENT_ANIMATION_STUN_BATON_ATTACK_1;
        case CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER:
            return CSWCAnimBase::CLIENT_ANIMATION_SINGLE_SABER_CRITICAL_STRIKE_1;
        case CSWCAnimBase::WEAPON_CLASS_TWO_HANDED_SABER:
            return CSWCAnimBase::CLIENT_ANIMATION_TWO_HANDED_SABER_CRITICAL_STRIKE_1;
        case CSWCAnimBase::WEAPON_CLASS_DUAL_SABER:
            return CSWCAnimBase::CLIENT_ANIMATION_DUAL_SABER_CRITICAL_STRIKE_1;
        default: return 0;
        }
    case FEAT_FLURRY: case FEAT_WHIRLWIND_ATTACK: case FEAT_IMPROVED_FLURRY:
        switch (d) {
        case CSWCAnimBase::WEAPON_CLASS_STUN_BATON: return CSWCAnimBase::CLIENT_ANIMATION_STUN_BATON_ATTACK_2;
        case CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER:
            return CSWCAnimBase::CLIENT_ANIMATION_SINGLE_SABER_FLURRY_1;
        case CSWCAnimBase::WEAPON_CLASS_TWO_HANDED_SABER:
            return CSWCAnimBase::CLIENT_ANIMATION_TWO_HANDED_SABER_FLURRY_1;
        case CSWCAnimBase::WEAPON_CLASS_DUAL_SABER: return CSWCAnimBase::CLIENT_ANIMATION_DUAL_SABER_FLURRY_1;
        default: return 0;
        }
    case FEAT_IMPROVED_POWER_ATTACK: case FEAT_POWER_ATTACK: case FEAT_MASTER_POWER_ATTACK:
        switch (d) {
        case CSWCAnimBase::WEAPON_CLASS_STUN_BATON: return CSWCAnimBase::CLIENT_ANIMATION_STUN_BATON_ATTACK_1;
        case CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER:
            return CSWCAnimBase::CLIENT_ANIMATION_SINGLE_SABER_POWER_ATTACK_1;
        case CSWCAnimBase::WEAPON_CLASS_TWO_HANDED_SABER:
            return CSWCAnimBase::CLIENT_ANIMATION_TWO_HANDED_SABER_POWER_ATTACK_1;
        case CSWCAnimBase::WEAPON_CLASS_DUAL_SABER:
            return CSWCAnimBase::CLIENT_ANIMATION_DUAL_SABER_POWER_ATTACK_1;
        default: return 0;
        }
    case CSWSCreatureStats::FEAT_FORCE_JUMP:
    case CSWSCreatureStats::FEAT_FORCE_JUMP_ADVANCED:
    case CSWSCreatureStats::FEAT_FORCE_JUMP_MASTERY:
        if (static_cast<unsigned int>(d - 2) < 3)
            return d + (CSWCAnimBase::CLIENT_ANIMATION_SINGLE_SABER_FORCE_JUMP -
                        CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER);
        break;
    }
    return 0;
#else
    if (b == 0 || IsCreature()) {
        int nIdx = m_attackIndex;
        if (a == SERVER_ANIMATION_ATTACK_2) {
            if (c == 0) {
                switch (d) {
                case CSWCAnimBase::WEAPON_CLASS_STUN_BATON:
                    return nIdx % 2 + CSWCAnimBase::CLIENT_ANIMATION_STUN_BATON_ATTACK_1;
                case CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER:
                    return nIdx + CSWCAnimBase::CLIENT_ANIMATION_SINGLE_SABER_ATTACK_1;
                case CSWCAnimBase::WEAPON_CLASS_TWO_HANDED_SABER:
                    return nIdx + CSWCAnimBase::CLIENT_ANIMATION_TWO_HANDED_SABER_ATTACK_1;
                case CSWCAnimBase::WEAPON_CLASS_DUAL_SABER:
                    return nIdx + CSWCAnimBase::CLIENT_ANIMATION_DUAL_SABER_ATTACK_1;
                case CSWCAnimBase::WEAPON_CLASS_NATURAL:
                    GetServerObject();
                    if (GetServerObject()->AsSWSCreature()->m_pStats->HasFeat(
                            CSWSCreatureStats::FEAT_COMPLEX_UNARMED_ANIMS)) {
                        int n;
                        if (m_pStats->GetLevel() <= 9) {
                            if (m_pStats->GetLevel() <= 6)
                                n = m_pStats->GetLevel() > 3 ? 3 : 2;
                            else
                                n = 4;
                        } else {
                            n = 5;
                        }
                        return nIdx % n + CSWCAnimBase::CLIENT_ANIMATION_UNARMED_ATTACK_1;
                    }
                    return nIdx % 2 + CSWCAnimBase::CLIENT_ANIMATION_NATURAL_ATTACK_1;
                default:
                    return 0;
                }
            }
            if (IsCreature()) {
                if (GetAnimationBase()->AnimationExists(CSWCAnimBase::CLIENT_ANIMATION_MONSTER_ATTACK_1))
                    return nIdx + CSWCAnimBase::CLIENT_ANIMATION_MONSTER_ATTACK_1;
            } else {
                switch (d) {
                case CSWCAnimBase::WEAPON_CLASS_STUN_BATON:
                    return nIdx + CSWCAnimBase::CLIENT_ANIMATION_STUN_BATON_ATTACK_1;
                case CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER:
                    return nIdx + CSWCAnimBase::CLIENT_ANIMATION_SINGLE_SABER_ATTACK_MONSTER_1;
                case CSWCAnimBase::WEAPON_CLASS_TWO_HANDED_SABER:
                    return nIdx + CSWCAnimBase::CLIENT_ANIMATION_TWO_HANDED_SABER_ATTACK_MONSTER_1;
                case CSWCAnimBase::WEAPON_CLASS_DUAL_SABER:
                    return nIdx + CSWCAnimBase::CLIENT_ANIMATION_DUAL_SABER_ATTACK_MONSTER_1;
                case CSWCAnimBase::WEAPON_CLASS_NATURAL:
                    GetServerObject();
                    if (GetServerObject()->AsSWSCreature()->m_pStats->HasFeat(
                            CSWSCreatureStats::FEAT_COMPLEX_UNARMED_ANIMS))
                        return nIdx + CSWCAnimBase::CLIENT_ANIMATION_UNARMED_ATTACK_MONSTER_1;
                    return nIdx + CSWCAnimBase::CLIENT_ANIMATION_NATURAL_ATTACK_1;
                default:
                    return 0;
                }
            }
        } else if (!IsCreature()) {
            int n = rand_wincompatible() % 5;
            switch (d) {
            case CSWCAnimBase::WEAPON_CLASS_STUN_BATON:
                return nIdx + CSWCAnimBase::CLIENT_ANIMATION_STUN_BATON_ATTACK_1;
            case CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER: return MELEE_ATTACK_TABLE_D2[n];
            case CSWCAnimBase::WEAPON_CLASS_TWO_HANDED_SABER: return MELEE_ATTACK_TABLE_D3[n];
            case CSWCAnimBase::WEAPON_CLASS_DUAL_SABER: return MELEE_ATTACK_TABLE_D4[n];
            case CSWCAnimBase::WEAPON_CLASS_NATURAL:
                GetServerObject();
                if (GetServerObject()->AsSWSCreature()->m_pStats->HasFeat(
                        CSWSCreatureStats::FEAT_COMPLEX_UNARMED_ANIMS))
                    return MELEE_ATTACK_TABLE_D8[n];
                return nIdx + CSWCAnimBase::CLIENT_ANIMATION_NATURAL_ATTACK_1;
            default:
                return 0;
            }
        }
        return nIdx + CSWCAnimBase::CLIENT_ANIMATION_MONSTER_GENERIC_ATTACK_1;
    }
    __asm__ __volatile__("" : "+r"(b));  // idiom-exempt: keeps b opaque to the optimizer, which would otherwise prove b != 0 and delete one arm.
    switch (b) {
    case FEAT_CRITICAL_STRIKE: case FEAT_IMPROVED_CRITICAL_STRIKE: case FEAT_MASTER_CRITICAL_STRIKE: {
        int n = rand_wincompatible() % 3;
        switch (d) {
        case CSWCAnimBase::WEAPON_CLASS_STUN_BATON: return CSWCAnimBase::CLIENT_ANIMATION_STUN_BATON_ATTACK_1;
        case CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER: return MELEE_ATTACK_TABLE_A2[n];
        case CSWCAnimBase::WEAPON_CLASS_TWO_HANDED_SABER: return MELEE_ATTACK_TABLE_A3[n];
        case CSWCAnimBase::WEAPON_CLASS_DUAL_SABER: return MELEE_ATTACK_TABLE_A4[n];
        case CSWCAnimBase::WEAPON_CLASS_NATURAL: return MELEE_ATTACK_TABLE_A8[n];
        default: return 0;
        }
    }
    case FEAT_FLURRY: case FEAT_WHIRLWIND_ATTACK: case FEAT_IMPROVED_FLURRY: {
        int n = rand_wincompatible() % 3;
        switch (d) {
        case CSWCAnimBase::WEAPON_CLASS_STUN_BATON: return CSWCAnimBase::CLIENT_ANIMATION_STUN_BATON_ATTACK_2;
        case CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER: return MELEE_ATTACK_TABLE_B2[n];
        case CSWCAnimBase::WEAPON_CLASS_TWO_HANDED_SABER: return MELEE_ATTACK_TABLE_B3[n];
        case CSWCAnimBase::WEAPON_CLASS_DUAL_SABER: return MELEE_ATTACK_TABLE_B4[n];
        case CSWCAnimBase::WEAPON_CLASS_NATURAL: return MELEE_ATTACK_TABLE_B8[n];
        default: return 0;
        }
    }
    case FEAT_IMPROVED_POWER_ATTACK: case FEAT_POWER_ATTACK: case FEAT_MASTER_POWER_ATTACK: {
        int n = rand_wincompatible() % 3;
        switch (d) {
        case CSWCAnimBase::WEAPON_CLASS_STUN_BATON: return CSWCAnimBase::CLIENT_ANIMATION_STUN_BATON_ATTACK_1;
        case CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER: return MELEE_ATTACK_TABLE_C2[n];
        case CSWCAnimBase::WEAPON_CLASS_TWO_HANDED_SABER: return MELEE_ATTACK_TABLE_C3[n];
        case CSWCAnimBase::WEAPON_CLASS_DUAL_SABER: return MELEE_ATTACK_TABLE_C4[n];
        case CSWCAnimBase::WEAPON_CLASS_NATURAL: return MELEE_ATTACK_TABLE_C8[n];
        default: return 0;
        }
    }
    case CSWSCreatureStats::FEAT_FORCE_JUMP:
    case CSWSCreatureStats::FEAT_FORCE_JUMP_ADVANCED:
    case CSWSCreatureStats::FEAT_FORCE_JUMP_MASTERY:
        if (static_cast<unsigned int>(d - 2) < 3)
            return d + (CSWCAnimBase::CLIENT_ANIMATION_SINGLE_SABER_FORCE_JUMP -
                        CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER);
        break;
    case -1:
        return CSWCAnimBase::CLIENT_ANIMATION_POWER_PUNCH;
    // Every other feat id up to the force-jump feats falls out with 0.
    case 0: case FEAT_ADVANCED_JEDI_DEFENSE: case FEAT_ADVANCED_GUARD_STANCE:
    case FEAT_AMBIDEXTERITY: case FEAT_ARMOUR_PROF_HEAVY: case FEAT_ARMOUR_PROF_LIGHT:
    case FEAT_ARMOUR_PROF_MEDIUM: case FEAT_CAUTIOUS: case FEAT_DOUBLE_WEAPON_FIGHTING:
    case FEAT_EMPATHY: case FEAT_GEAR_HEAD: case FEAT_GREAT_FORTITUDE: case FEAT_IMPLANT_LEVEL_1:
    case FEAT_IMPLANT_LEVEL_2: case FEAT_IMPLANT_LEVEL_3: case FEAT_IMPROVED_POWER_BLAST:
    case FEAT_IMPROVED_SNIPER_SHOT: case FEAT_IRON_WILL: case FEAT_LIGHTNING_REFLEXES: case 23:
    case FEAT_MASTER_JEDI_DEFENSE: case FEAT_MASTER_GUARD_STANCE: case FEAT_MULTI_SHOT:
    case FEAT_PERCEPTIVE: case FEAT_POWER_BLAST: case FEAT_RAPID_SHOT: case FEAT_SNIPER_SHOT:
    case FEAT_WEAPON_FOCUS_BLASTER: case FEAT_WEAPON_FOCUS_BLASTER_RIFLE:
    case FEAT_WEAPON_FOCUS_GRENADE: case FEAT_WEAPON_FOCUS_HEAVY_WEAPONS:
    case FEAT_WEAPON_FOCUS_LIGHTSABER: case FEAT_WEAPON_FOCUS_MELEE_WEAPONS:
    case FEAT_WEAPON_FOCUS_SIMPLE_WEAPONS: case FEAT_WEAPON_PROFICIENCY_BLASTER:
    case FEAT_WEAPON_PROFICIENCY_BLASTER_RIFLE: case FEAT_WEAPON_PROFICIENCY_GRENADE:
    case FEAT_WEAPON_PROFICIENCY_HEAVY_WEAPONS: case FEAT_WEAPON_PROFICIENCY_LIGHTSABER:
    case FEAT_WEAPON_PROFICIENCY_MELEE_WEAPONS: case FEAT_WEAPON_PROFICIENCY_SIMPLE_WEAPONS:
    case FEAT_WEAPON_SPECIALIZATION_BLASTER: case FEAT_WEAPON_SPECIALIZATION_BLASTER_RIFLE:
    case FEAT_WEAPON_SPECIALIZATION_GRENADE: case FEAT_WEAPON_SPECIALIZATION_HEAVY_WEAPONS:
    case FEAT_WEAPON_SPECIALIZATION_LIGHTSABER: case FEAT_WEAPON_SPECIALIZATION_MELEE_WEAPONS:
    case FEAT_WEAPON_SPECIALIZATION_SIMPLE_WEAPONS: case FEAT_GUARD_STANCE: case FEAT_JEDI_DEFENSE:
    case FEAT_UNCANNY_DODGE_1: case FEAT_UNCANNY_DODGE_2: case FEAT_SKILL_FOCUS_COMPUTER_USE:
    case 59: case FEAT_SNEAK_ATTACK_1D6: case FEAT_SNEAK_ATTACK_2D6: case FEAT_SNEAK_ATTACK_3D6:
    case FEAT_SNEAK_ATTACK_4D6: case FEAT_SNEAK_ATTACK_5D6: case FEAT_SNEAK_ATTACK_6D6:
    case FEAT_SNEAK_ATTACK_7D6: case FEAT_SNEAK_ATTACK_8D6: case FEAT_SNEAK_ATTACK_9D6:
    case FEAT_SNEAK_ATTACK_10D6: case FEAT_SKILL_FOCUS_DEMOLITIONS: case FEAT_SKILL_FOCUS_STEALTH:
    case FEAT_SKILL_FOCUS_AWARENESS: case FEAT_SKILL_FOCUS_PERSUADE: case FEAT_SKILL_FOCUS_REPAIR:
    case FEAT_SKILL_FOCUS_SECURITY: case FEAT_SKILL_FOCUS_TREAT_INJUURY:
    case FEAT_MASTER_SNIPER_SHOT: case FEAT_DROID_UPGRADE_1: case FEAT_DROID_UPGRADE_2:
    case FEAT_DROID_UPGRADE_3: case FEAT_MASTER_POWER_BLAST: case FEAT_TOUGHNESS:
    case FEAT_ADVANCED_DOUBLE_WEAPON_FIGHTING: case FEAT_FORCE_FOCUS_ALTER:
    case FEAT_FORCE_FOCUS_CONTROL: case FEAT_FORCE_FOCUS_SENSE: case FEAT_FORCE_FOCUS_ADVANCED:
    case FEAT_FORCE_FOCUS_MASTERY: case FEAT_IMPROVED_RAPID_SHOT: case FEAT_PROFICIENCY_ALL:
    case FEAT_BATTLE_MEDITATION: case CSWSCreatureStats::FEAT_WOOKIEE_TOUGHNESS:
    case CSWSCreatureStats::FEAT_BLASTER_INTEGRATION: case CSWSCreatureStats::FEAT_FORCE_CAMOFLAGE:
    case CSWSCreatureStats::FEAT_FORCE_IMMUNITY_FEAR:
    case CSWSCreatureStats::FEAT_FORCE_IMMUNITY_STUN:
    case CSWSCreatureStats::FEAT_FORCE_IMMUNITY_PARALYSIS:
        break;
    default:
        if (b != CSWSCreatureStats::FEAT_SHIELD_BREAKER)
            return 0;
        return CSWCAnimBase::CLIENT_ANIMATION_POWER_PUNCH;
    }
    return 0;
#endif
}

// Maps an equip-slot bit to its last-instant-equip flag (0 for bits with no slot).
int CSWCCreature::GetLastInstant(unsigned int nInstant) {
    switch (nInstant) {
    case MSG_SLOT_HEAD:              return m_aLastInstant[0];
    case MSG_SLOT_BODY:              return m_aLastInstant[1];
    case MSG_SLOT_BOOTS:             return m_aLastInstant[2];
    case MSG_SLOT_HANDS:             return m_aLastInstant[3];
    case MSG_SLOT_RIGHT_WEAPON:      return m_aLastInstant[4];
    case MSG_SLOT_LEFT_WEAPON:       return m_aLastInstant[5];
    case MSG_SLOT_CLOAK:             return m_aLastInstant[6];
    case MSG_SLOT_LEFT_ARM:          return m_aLastInstant[7];
    case MSG_SLOT_RIGHT_ARM:         return m_aLastInstant[8];
    case MSG_SLOT_IMPLANT:           return m_aLastInstant[9];
    case MSG_SLOT_BELT:              return m_aLastInstant[10];
    case MSG_SLOT_CREATURE_WEAPON_L: return m_aLastInstant[11];
    case MSG_SLOT_CREATURE_WEAPON_R: return m_aLastInstant[12];
    case MSG_SLOT_CREATURE_WEAPON_B: return m_aLastInstant[13];
    case MSG_SLOT_CREATURE_ARMOR:    return m_aLastInstant[14];
    }
    return 0;
}

void CSWCCreature::SetLastInstant(unsigned int nInstant, int value) {
    switch (nInstant) {
    case MSG_SLOT_HEAD:              m_aLastInstant[0] = value;  break;
    case MSG_SLOT_BODY:              m_aLastInstant[1] = value;  break;
    case MSG_SLOT_BOOTS:             m_aLastInstant[2] = value;  break;
    case MSG_SLOT_HANDS:             m_aLastInstant[3] = value;  break;
    case MSG_SLOT_RIGHT_WEAPON:      m_aLastInstant[4] = value;  break;
    case MSG_SLOT_LEFT_WEAPON:       m_aLastInstant[5] = value;  break;
    case MSG_SLOT_CLOAK:             m_aLastInstant[6] = value;  break;
    case MSG_SLOT_LEFT_ARM:          m_aLastInstant[7] = value;  break;
    case MSG_SLOT_RIGHT_ARM:         m_aLastInstant[8] = value;  break;
    case MSG_SLOT_IMPLANT:           m_aLastInstant[9] = value;  break;
    case MSG_SLOT_BELT:              m_aLastInstant[10] = value; break;
    case MSG_SLOT_CREATURE_WEAPON_L: m_aLastInstant[11] = value; break;
    case MSG_SLOT_CREATURE_WEAPON_R: m_aLastInstant[12] = value; break;
    case MSG_SLOT_CREATURE_WEAPON_B: m_aLastInstant[13] = value; break;
    case MSG_SLOT_CREATURE_ARMOR:    m_aLastInstant[14] = value; break;
#if !KOTOR1  // K1 has no creature-weapon slots
    case MSG_SLOT_RIGHT_WEAPON_2: m_aLastInstant[15] = value; break;
    case MSG_SLOT_LEFT_WEAPON_2:  m_aLastInstant[16] = value; break;
#endif
    }
}

// Classifies a radial-menu target as no target, hostile creature (or any creature while the reticle
// is changing), non-hostile creature or non-inventory placeable, door or inventory placeable,
// trigger, or anything else.
int CSWCCreature::GetTargetInterfaceTargetType(CSWCObject* pTarget) {
    if (pTarget == 0)
        return TARGET_INTERFACE_NONE;
    if (pTarget->AsSWCCreature() != 0) {
        if (pTarget->AsSWCCreature()->IsHostile() != 0)
            return TARGET_INTERFACE_HOSTILE;
        return g_pAppManager->m_pClientExoApp->GetTargetChanging() != 0 ? TARGET_INTERFACE_HOSTILE
                                                                         : TARGET_INTERFACE_USABLE;
    }
    if (pTarget->AsSWCDoor() != 0)
        return TARGET_INTERFACE_OPENABLE;
    if (pTarget->AsSWCPlaceable() != 0) {
        CSWCPlaceable* pPlaceable = pTarget->AsSWCPlaceable();
        if (pPlaceable != 0) {
            CSWSPlaceable* pServerPlaceable = pPlaceable->GetServerPlaceable();
            if (pServerPlaceable != 0 && pServerPlaceable->m_bHasInventory != 0)
                return TARGET_INTERFACE_OPENABLE;
        }
        return TARGET_INTERFACE_USABLE;
    }
    return pTarget->AsSWCTrigger() != 0 ? TARGET_INTERFACE_TRIGGER : TARGET_INTERFACE_OTHER;
}

// Returns d when a matches b, c or d, otherwise 0.
int CSWCCreature::CheckFeat(int a, int b, int c, int d) {
    return (a == d || a == c || a == b) ? d : 0;
}

// The first of a, b or c the creature has, else -1. Prefers the server creature's stats.
int CSWCCreature::CheckFeat(int a, int b, int c) {
    CSWSCreature* pServerCreature = 0;
    if (GetServerObject())
        pServerCreature = GetServerObject()->AsSWSCreature();
    if (pServerCreature != 0 && pServerCreature->m_pStats != 0) {
        if (pServerCreature->m_pStats->HasFeat(static_cast<unsigned short>(a)))
            return a;
        if (pServerCreature->m_pStats->HasFeat(static_cast<unsigned short>(b)))
            return b;
        return pServerCreature->m_pStats->HasFeat(static_cast<unsigned short>(c)) ? c : -1;
    }
    if (m_pStats->HasFeat(static_cast<unsigned short>(a)))
        return a;
    if (m_pStats->HasFeat(static_cast<unsigned short>(b)))
        return b;
    if (!m_pStats->HasFeat(static_cast<unsigned short>(c)))
        return -1;
    return c;
}

// Appends one feat entry to a radial-menu list: the feat name (suffixed when the menu targets
// the creature itself), the callback, the action id and the feat icon.
void CSWCCreature::EnableFeatForMenu(int nFeat, CExoArrayList<CSWGuiInterfaceAction>& list,
                                     void (CSWCObject::*pfnAction)(unsigned int, CSWCCreature*),
                                     CSWCCreature* pCreature) {
    if (nFeat == -1)
        return;
    CSWFeat* pFeat = g_pRules->GetFeat(static_cast<unsigned short>(nFeat));
    int nOldCount = list.m_count;
    list.SetSize(list.m_count + 1);
    CExoString sName = g_pAppManager->m_pClientExoApp->GetGUIString(pFeat->m_nNameStrRef);
    if (pCreature == this)
        sName = sName + " (" + g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SELF) + ")";
    list.m_pArray[nOldCount].m_sName = sName;
    list.m_pArray[nOldCount].m_pfnAction =
        static_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(pfnAction);
    list.m_pArray[nOldCount].m_nObjectId = pCreature->m_objectId;
    list.m_pArray[nOldCount].m_nActionId = nFeat | CSWGuiInterfaceAction::ACTION_ID_FEAT;
    list.m_pArray[nOldCount].m_resRef = pFeat->m_icon;
}

void CSWCCreature::ResetUpdateTimes(unsigned int a, unsigned int b) {
    m_nLastAIUpdateDay = a;
    m_nLastAIUpdateTod = b;
}

CExoString CSWCCreature::GetName() {
#if KOTOR1
    return m_pStats->GetFullName();  // K1 builds the name on demand
#else
    return m_pStats->m_name;
#endif
}

int CSWCCreature::GetArmourClass() {
    if (m_pCreatureAppearance)
        return m_pCreatureAppearance->GetArmourClass();
    return 0;
}

// Loads a .lip viseme track and hands it to the model. Viseme times normalize to [0,1] over
// the animation length; a track that starts at 0 and ends near 1 gets the ini lip delay added
// and its last time clamped to 1. Returns 0 on load failure, no visemes or an off-range track.
int CSWCCreature::LipSync(const CResRef& r) {
#if !KOTOR1  // K1 has no "Lip Delay" setting
    static unsigned char s_bLipDelayRead;
    static float s_fLipDelay;
    if (!s_bLipDelayRead) {
        s_bLipDelayRead = 1;
        CExoIni ini;
        CExoString sValue;
        if (ini.ReadIniEntry(sValue, ".\\swkotor2.ini", "Sound Options", "Lip Delay")) {
            s_fLipDelay = sValue.AsFLOAT();
        }
    }
#endif
    CExoString sName(r.GetResRefStr());
    CLIP* pLip = new CLIP();
    if (pLip->LoadLip(sName) == 0) {
        delete pLip;
        return 0;
    }
    int nVisemes = pLip->GetNumVisemes();
    float fAnimLength = pLip->GetAnimLength();
    float* pTimes = new float[static_cast<unsigned int>(nVisemes)];
    float* pVisemes = new float[static_cast<unsigned int>(nVisemes)];
    int bSuccess = 0;
    if (nVisemes >= 1) {
        for (unsigned int i = 0; i < static_cast<unsigned int>(nVisemes); i++) {
            pTimes[i] = pLip->GetTimeIndex(i) / fAnimLength;
            pVisemes[i] = (pLip->GetVisemeIndex(i) + 1.0f) * 0.0625f;
        }
        if (pTimes[0] == 0.0f && !(pTimes[nVisemes - 1] >= 1.00001f) &&
            !(pTimes[nVisemes - 1] <= 0.99999f)) {
#if !KOTOR1
            for (unsigned int i = 0; i < static_cast<unsigned int>(nVisemes); i++)
                pTimes[i] = (pLip->GetTimeIndex(i) + s_fLipDelay) / fAnimLength;
#endif
            pTimes[nVisemes - 1] = 1.0f;
            m_pModel->SetAnimationVisemes(0, 0, 0, 0.0f);
            bSuccess = m_pModel->SetAnimationVisemes(pTimes, pVisemes, nVisemes, fAnimLength);
        }
    }
    pLip->UnloadLip();
    delete pLip;
    delete[] pTimes;
    delete[] pVisemes;
    return bSuccess;
}

void CSWCCreature::StopLipSync() {
    m_pModel->SetAnimationVisemes(0, 0, 0, 0.0f);
}

void CSWCCreature::RemoveShadowBlob() {
    static_cast<CSWCObject*>(this)->CSWCObject::RemoveShadowBlob();  // idiom-exempt: the header models these classes flat.
}

// Ignores both arguments: puts the client app into combat mode 1.
void CSWCCreature::ActionInitiateCombatMode(unsigned int n, CSWCCreature* p) {
    K2_UNUSED(n); K2_UNUSED(p);
    g_pAppManager->m_pClientExoApp->SetCombatMode(1);
}

void CSWCCreature::UnloadModel(unsigned char a) {
    if (m_pModel)
        m_pModel->UnloadModel(a);
}

void CSWCCreature::UnloadModel() {
    static_cast<CSWCObject*>(this)->CSWCObject::HidePopUpBubble();  // idiom-exempt: the header models these classes flat.
    if (m_pModel) {
        m_pModel->UnloadModel();
        delete m_pModel;
        m_pModel = 0;
    }
}

void CSWCCreature::SetArea(CSWCArea* pArea) {
    static_cast<CSWCObject*>(this)->CSWCObject::SetArea(pArea);  // idiom-exempt: the header models these classes flat.
    m_areaObjectId = m_pArea ? m_pArea->m_objectId : CGameObjectArray::INVALIDOBJID;
}

void CSWCCreature::UpdateAreaId() {
    m_areaObjectId = m_pArea ? m_pArea->m_objectId : CGameObjectArray::INVALIDOBJID;
}

int CSWCCreature::TestAIStateActionGroup(int group) {
    static const int s_group2Table[7] = { 1, 1, 1, 1, 0, 1, 1 };
    switch (group) {
    case 0:
        if ((m_nAIStateAction & 0xfe) == 2) return 1;
        break;
    case 2:
        if (m_nAIStateAction >= CSWSCreature::AI_STATE_ACTION_PICKING_UP_ITEM &&
            m_nAIStateAction <= CSWSCreature::AI_STATE_ACTION_CLOSING_DOOR)
            return s_group2Table[static_cast<signed char>(m_nAIStateAction - CSWSCreature::AI_STATE_ACTION_PICKING_UP_ITEM)];
        break;
    case 3:
        if (m_nAIStateAction == CSWSCreature::AI_STATE_ACTION_CASTING) return 1;
        break;
    case 1:
        if (m_nAIStateAction < 2) return 1;
        break;
    }
    return 0;
}

// Picks this attack's animation index: the attack animation with a == 0 rerolls 0..4 until it
// differs from the excluded and the last-used index, anything else is a coin flip. The result
// is stamped into both m_attackIndex and m_lastAttackIndex.
void CSWCCreature::ResolveAttackIndex(unsigned short anim, int a, int exclude) {
    int idx;
    if (anim == SERVER_ANIMATION_ATTACK_2 && a == 0) {
        do {
            idx = rand_wincompatible() % 5;
        } while (idx == exclude || idx == m_lastAttackIndex);
    } else {
        idx = rand_wincompatible() % 2;
    }
    m_attackIndex = idx;
    m_lastAttackIndex = idx;
}

// Drains the visibility list back-to-front.
void CSWCCreature::ClearVisibilityList() {
    for (int i = m_visibilityList.m_count - 1; i >= 0; i--) {
        CSWVisibilityNode* pNode = m_visibilityList.m_pArray[i];
        m_visibilityList.DelIndex(i);
        delete pNode;
    }
}

// Powers both equipped weapon slots (4 right, 5 left) on or off.
void CSWCCreature::SetOverrideLightsaberState(int on, int a, int b) {
    if (on) {
        CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
            m_aEquippedItemIds[4]);
        if (pItem)
            pItem->PowerItem(a, 1, b, 0);
        pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[5]);
        if (pItem)
            pItem->PowerItem(a, 1, b, 0);
    }
#if KOTOR1  // K1 records the override flag
    m_bOverrideLightsaberState = on;
#endif
}

void CSWCCreature::PerformCastSpell(unsigned int spellId, unsigned long long objId,
                                    const Vector& pos, unsigned char a, unsigned char b,
                                    unsigned char c, int d) {
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg)
        pMsg->SendPlayerToServerInput_CastSpell(spellId, a, b, c, d, pos, objId);
}

void CSWCCreature::PerformSkill(unsigned char skill, unsigned char subType, unsigned long long objId) {
    if (objId != CGameObjectArray::INVALIDOBJID) {
        CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
        if (pMsg) {
            Vector pos = {};
            pMsg->SendPlayerToServerInput_UseSkill(skill, subType, objId, pos);
        }
    }
}

void CSWCCreature::PerformCombatFeat(unsigned short nFeat, CSWCCreature* pTarget) {
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg)
        pMsg->SendPlayerToServerInput_Attack(m_objectId, nFeat);
    if (pTarget) {
        pTarget->m_nStateFlags |= 1;
        if (pTarget->m_objectId == g_pAppManager->m_pClientExoApp->GetPlayerCreatureId())
            g_pAppManager->m_pClientExoApp->SetCombatMode(pTarget->m_nStateFlags & 1);
    }
}

#if !KOTOR1
// Sends the weapon-switch request, throttled by the weapon-switch cooldown and suppressed in
// menu-disabled areas. a == -1 swaps to the other weapon set.
void CSWCCreature::ActionMenuSwitchWeapons(unsigned int a, CSWCCreature* pTarget) {
    K2_UNUSED(pTarget);
    if (m_nSwitchWeaponsCooldown <= 0) {
        CSWSCreature* pServer;
        if (GetServerObject() == 0 || (pServer = GetServerObject()->AsSWSCreature()) == 0 ||
            pServer->GetArea() == 0 ||
            static_cast<CSWSArea*>(pServer->GetArea())->m_nPlayerRestrictMode == 0) {
            int bSwap = (a == 0xffffffff);
            g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_SwitchWeapons(
                m_objectId, bSwap);
            m_nSwitchWeaponsCooldown = 1000;
        }
    }
}
#endif

void CSWCCreature::SetItemUnEquippedByMe(unsigned long long itemId) {
    if (itemId != CGameObjectArray::INVALIDOBJID) {
        CGameObject* pObject;
        if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->GetGameObject(itemId, &pObject)
                == CGameObjectArray::SUCCESS) {
            CSWCItem* pItem = static_cast<CSWCItem*>(pObject->AsSWCItem());
            pItem->SetEquippedBy(static_cast<CSWCCreature*>(0));
        }
    }
}

int CSWCCreature::ShowLightSaberContactVisual(CSWCCombatAttackData* pData) {
    K2_UNUSED(pData);
    Vector pos = {};
    Quaternion rot;  // Identity quaternion.
    if (GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)->GetPartPosition(const_cast<char*>("LightSaberHook"), &pos, &rot)) {
        m_pArea->ApplyFireAndForgetVisualEffect(VFX_COM_SPARKS_PARRY_METAL, pos);
        return 1;
    }
    return 0;
}

// The weapon's WeaponSize column minus the wielder's size-category column in the appearance table.
int CSWCCreature::GetRelativeWeaponSize(CSWCItem* pItem) {
    int nSizeCategory;
    g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
        m_pCreatureAppearance->m_nAppearanceId,
        CTwoDimArrays::COL2DA_APPEARANCETABLE_SIZECATEGORY, &nSizeCategory);
    CSWBaseItem* pBaseItem = g_pRules->GetBaseItem(pItem->m_baseItem);
    return pBaseItem->m_nWeaponSize - nSizeCategory;
}

// Fires the impact animation on the slot's special-attack models.
void CSWCCreature::PlaySpecialAttacks(unsigned char slot) {
    switch (slot) {
    case 0:
    case 1:
        if (m_aSpecialAttackGobs[0])
            m_aSpecialAttackGobs[0]->PlayAnimation(const_cast<char*>("impact"), 1.0f, 1, 0.0f);
        if (m_aSpecialAttackGobs[1])
            m_aSpecialAttackGobs[1]->PlayAnimation(const_cast<char*>("impact"), 1.0f, 1, 0.0f);
        break;
    case 2:
        if (m_aSpecialAttackGobs[2])
            m_aSpecialAttackGobs[2]->PlayAnimation(const_cast<char*>("impact"), 1.0f, 1, 0.0f);
        break;
    case 3:
        if (m_aSpecialAttackGobs[3])
            m_aSpecialAttackGobs[3]->PlayAnimation(const_cast<char*>("impact"), 1.0f, 1, 0.0f);
        break;
    }
}

// Same dispatch as PlaySpecialAttacks, with the cessation animation.
void CSWCCreature::StopSpecialAttacks(unsigned char slot) {
    switch (slot) {
    case 0:
    case 1:
        if (m_aSpecialAttackGobs[0])
            m_aSpecialAttackGobs[0]->PlayAnimation(const_cast<char*>("cessation"), 1.0f, 1, 0.0f);
        if (m_aSpecialAttackGobs[1])
            m_aSpecialAttackGobs[1]->PlayAnimation(const_cast<char*>("cessation"), 1.0f, 1, 0.0f);
        break;
    case 2:
        if (m_aSpecialAttackGobs[2])
            m_aSpecialAttackGobs[2]->PlayAnimation(const_cast<char*>("cessation"), 1.0f, 1, 0.0f);
        break;
    case 3:
        if (m_aSpecialAttackGobs[3])
            m_aSpecialAttackGobs[3]->PlayAnimation(const_cast<char*>("cessation"), 1.0f, 1, 0.0f);
        break;
    }
}

int CSWCCreature::GetForceShield() {
    CSWSCreature* pServer;
    if (GetServerObject() != 0 &&
        (pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature())) != 0) {
        for (int64_t i = 0; i < pServer->m_effects.m_count; i++) {
            CGameEffect* pEffect = pServer->m_effects.m_pArray[i];
            if (CSWSEffectListHandler::EFFECT_FORCESHIELD == pEffect->m_subType)
                return pEffect->GetInteger(0);
            if (CSWSEffectListHandler::EFFECT_FORCESHIELD < pEffect->m_subType)
                return 0;
        }
    }
    return 0;
}

void CSWCCreature::ComputeZValue(CSWCArea* pArea, Vector* pPos, float f, Vector* pVec) {
    K2_UNUSED(f); K2_UNUSED(pVec);
    CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
    if (pServer) {
        pPos->z = 0.0f;
        pPos->z = pServer->ComputeHeight(pArea->m_areaResRef, *pPos, 1);
    }
}

void CSWCCreature::DoFootstepRumble(int a) {
    g_pAppManager->m_pClientExoApp->LookUpAndPerformRumbleWithCutOff(
        a, CClientExoApp::RUMBLE_TABLE_FOOTSTEP_SOUNDS, m_position);
}

void CSWCCreature::SetRunLooksAwful(int a) {
    GetAnimationBase()->m_runLooksAwful = a;
}

void CSWCCreature::SetSpecialWalkAnimation(unsigned char a) {
    GetAnimationBase()->m_specialWalkAnimation = a;
}

// Tests whether a weapon type is ranged: types 5 to 9, with 8 excluded.
int CSWCCreature::IsRangedWeapon(unsigned char type) {
    static const int s_rangedWeapon[5] = {1, 1, 1, 0, 1};
    char idx = static_cast<char>(type - 5);
    if (static_cast<unsigned char>(idx) <= 4)
        return s_rangedWeapon[static_cast<int>(idx)];
    return 0;
}

CSWSCreature* CSWCCreature::GetServerCreature() {
    if (GetServerObject())
        return GetServerObject()->AsSWSCreature();
    return 0;
}

// On disable, clears any look-at target and latches the flag.
void CSWCCreature::SetDisableLookAt(int disable) {
    if (disable) {
        LookAt(OBJECT_INVALID, 0.0f);  // INVALIDOBJID
        m_bDisableLookAt = 1;
    } else {
        m_bDisableLookAt = 0;
    }
}

// Spawns a special attack's impact visual on the equipped weapons: the attack id picks the
// visualeffects.2da row, the model node names the visual and c selects the attach hook.
void CSWCCreature::ResolveSpecialAttackVisuals(int a, int b, int c) {
    static const int rows11[3] = {4027, 4012, 4027};
    static const int rows13[3] = {4025, 4014, 4025};
    static const int rows35[3] = {4031, 4017, 4031};
    static const int rows51[3] = {4026, 4018, 4026};
    static const int rows53[3] = {4028, 4019, 4028};
    static const int rows5b[3] = {4030, 4021, 4030};
    int nRow = VFX_COM_SNIPER_SHOT;
    const int* pRows = 0;
    switch (a) {
    case FEAT_IMPROVED_POWER_ATTACK:
        if (static_cast<unsigned>(b - CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER) >= 3)
            return;
        pRows = rows11;
        break;
    case FEAT_IMPROVED_POWER_BLAST:
        nRow = VFX_COM_POWER_BLAST_IMPROVED;
        break;
    case FEAT_IMPROVED_CRITICAL_STRIKE:
        if (static_cast<unsigned>(b - CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER) >= 3)
            return;
        pRows = rows13;
        break;
    case FEAT_IMPROVED_SNIPER_SHOT:
        nRow = VFX_COM_SNIPER_SHOT_IMPROVED;
        break;
    case FEAT_MULTI_SHOT:
        nRow = VFX_COM_MULTI_SHOT;
        break;
    case FEAT_SNIPER_SHOT:
        break;
    case FEAT_WHIRLWIND_ATTACK:
        if (static_cast<unsigned>(b - CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER) >= 3)
            return;
        pRows = rows35;
        break;
    case FEAT_MASTER_SNIPER_SHOT:
        nRow = VFX_COM_SNIPER_SHOT_MASTERY;
        break;
    case FEAT_MASTER_CRITICAL_STRIKE:
        if (static_cast<unsigned>(b - CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER) >= 3)
            return;
        pRows = rows51;
        break;
    case FEAT_MASTER_POWER_BLAST:
        nRow = VFX_COM_POWER_BLAST_MASTERY;
        break;
    case FEAT_MASTER_POWER_ATTACK:
        if (static_cast<unsigned>(b - CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER) >= 3)
            return;
        pRows = rows53;
        break;
    case FEAT_IMPROVED_FLURRY:
        if (static_cast<unsigned>(b - CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER) >= 3)
            return;
        pRows = rows5b;
        break;
    case FEAT_IMPROVED_RAPID_SHOT:
        nRow = VFX_COM_RAPID_SHOT_IMPROVED;
        break;
    default:
        return;
    }
    if (pRows != 0)
        nRow = pRows[b - CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER];
    CExoString sRow(nRow);
    CExoString sVisual;
    g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
        sRow, CExoString("Imp_Impact_Node"), &sVisual);
    if (!sVisual.IsEmpty()) {  // K1 only
        CExoString sHook;
        if (c == 0)
            sHook = "root";
        else
            sHook = "bullethook";
        CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
            m_aEquippedItemIds[INVENTORY_SLOT_RIGHTWEAPON]);
        if (pItem != 0)
            pItem->SpawnVisualEffect(sVisual, sHook);
        if ((b | 2) == 6) {
            CSWCItem* pLeftItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
                m_aEquippedItemIds[INVENTORY_SLOT_LEFTWEAPON]);
            if (pLeftItem != 0)
                pLeftItem->SpawnVisualEffect(sVisual, sHook);
        }
    }
}

int CSWCCreature::ResolveSpecialAttackVisualsMelee(int rank, int v2, int v3, int v4) {
    int visual;
    if (rank == 4) visual = v4;
    else if (rank == 3) visual = v3;
    else if (rank == 2) visual = v2;
    else return 0;
    return visual;
}

// Stops the rolling-footstep loop and clears its state.
void CSWCCreature::StopRollingFootstepSound() {
    if (m_pRollingFootstepSound && m_bRollingFootstepPlaying) {
        m_pRollingFootstepSound->Stop();
        m_bRollingFootstepPlaying = 0;
        m_rollingFootstepType = 0;
    }
}

void CSWCCreature::PlayRollingFootstepSound(int type) {
    if (!m_pRollingFootstepSound)
        return;
    m_pRollingFootstepSound->SetPosition(m_position, 0.0f);
    if (m_bRollingFootstepPlaying && m_rollingFootstepType == type)
        return;
    if (type != 0)
        m_pRollingFootstepSound->SetFixedVariance(m_rollingFootstepVariance + 1.0f);
    else
        m_pRollingFootstepSound->SetFixedVariance(1.0f);
    m_pRollingFootstepSound->Play();
    m_bRollingFootstepPlaying = 1;
    m_rollingFootstepType = type;
}

void CSWCCreature::DeleteLoopingCreatureSound() {
    m_pLoopingCreatureSound->Stop();
    if (m_pLoopingCreatureSound)
        delete m_pLoopingCreatureSound;
    m_pLoopingCreatureSound = 0;
}

// Advances the way-point cursor; 0 when there are no way-points or it is already on the last.
int CSWCCreature::MoveToNextWayPoint() {
    if (m_wayPointCount == 0)
        return 0;
    if (m_pCurrentWayPoint == &m_pWayPoints[m_wayPointCount - 1])
        return 0;
    m_pCurrentWayPoint = &static_cast<Vector*>(m_pCurrentWayPoint)[1];
    return 1;
}

// Copies out the way-point array and returns the cursor index, 0 when it was not found.
void CSWCCreature::GetWayPoints(unsigned short* pCount, Vector** ppPoints,
                                unsigned short* pCurrent) {
    *pCount = m_wayPointCount;
    *ppPoints = m_pWayPoints;
    unsigned short i = 0;
    if (m_wayPointCount != 0) {
        while (&m_pWayPoints[i] != static_cast<Vector*>(m_pCurrentWayPoint) && i < m_wayPointCount)
            i++;
        if (i >= m_wayPointCount)
            i = 0;
    }
    *pCurrent = i;}

int CSWCCreature::ResetAnimationState() {
    if (static_cast<CSWCObject*>(this)->CSWCObject::GetLoopingAnimation() == SERVER_ANIMATION_READY)  // idiom-exempt: the header models these classes flat.
        SetAnimation(SERVER_ANIMATION_READY, 0, 0);
    return 1;
}

void CSWCCreature::PerformExamine(unsigned long long objId) {
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg)
        pMsg->SendPlayerToServerInput_Examine(objId);
}

void CSWCCreature::PerformQuickChat(int nChat) {
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg)
        pMsg->SendPlayerToServerQuickChat(static_cast<unsigned short>(nChat));
}

const unsigned short CSWCCreature::CCREATURE_EMOTE_ANIMATION[13] = {
    10044, 10034, 10041, 10029, 10029, 10035, 10028, 10056, 10058, 10055, 10039, 10057, 10030
};
const unsigned short CSWCCreature::CCREATURE_EMOTE_SOUNDID[13] = {
    0xffff, 0xffff, 0xffff, 0xffff, 28, 0xffff, 0xffff, 0, 0, 0, 0, 0, 0
};

void CSWCCreature::PerformEmote(int emote) {
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg) {
        pMsg->SendPlayerToServerInput_PlayAnimation(m_objectId, m_position,
                                                    CCREATURE_EMOTE_ANIMATION[emote]);
        if ((0x1f90LL >> emote) & 1)
            pMsg->SendPlayerToServerQuickChat(CCREATURE_EMOTE_SOUNDID[emote]);
    }
}

void CSWCCreature::PerformDialog(unsigned long long objId) {
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg)
        pMsg->SendPlayerToServerInput_Dialog(objId);
}

void CSWCCreature::PerformCancelDisguise() {
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg)
        pMsg->SendPlayerToServerInput_CancelDisguise(m_objectId);
}

void CSWCCreature::PerformAttack(unsigned long long objId) {
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg)
        pMsg->SendPlayerToServerInput_Attack(objId, 0);
}

void CSWCCreature::PerformToggleMode(int mode, unsigned long long objId) {
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg)
        pMsg->SendPlayerToServerInput_ToggleMode(static_cast<unsigned char>(mode), objId);
}

int CSWCCreature::IsInjured() {
#if KOTOR1
    CSWSCreature* pCreature = 0;
    if (GetServerObject())
        pCreature = GetServerObject()->AsSWSCreature();
    return pCreature->IsInjured();
#else
    if (GetServerObject()) {
        CSWSCreature* pCreature = GetServerObject()->AsSWSCreature();
        if (pCreature)
            return pCreature->IsInjured();
    }
    return 0;
#endif
}

// Re-evaluates the way-point list against a candidate final point and commits the winner.
// Walkability comes from the area's TestDirectLine, tried in order:
//
// 1. keep the existing list if the first way-point is reachable
// 2. prepend the candidate
// 3. drop every way-point before the first reachable one
// 4. rebuild from the current way-point
// 5. snap straight to the candidate
void CSWCCreature::EvaluateNewWayPoints(int nCount, Vector* pPoints, int nId, const Vector& vNew) {
    Vector vGroundNormal = {0.0f, 0.0f, 0.0f};
    if (nId != 0 && m_bHasWayPointSpeed != 0 && m_appliedCommandTag != nId)
        return;
    int bAllocated = 0;
    if (nCount >= 1 && m_pArea != 0) {
        float fHit = 0.015625f;
        Vector vFrom = m_position;
        Vector vTo = pPoints[0];
        if (m_pArea->TestDirectLine(m_objectId, vFrom, vTo, fHit, 0.015625f, 0, 0) != 1) {
            vFrom = m_position;
            vTo = vNew;
            if (m_pArea->TestDirectLine(m_objectId, vFrom, vTo, fHit, 0.015625f, 0, 0) == 1) {
                int nNewCount = nCount + 1;
                Vector* pNewPoints = new Vector[nNewCount]();
                pNewPoints[0] = vNew;
                memcpy(&pNewPoints[1], pPoints, nCount * sizeof(Vector));
                bAllocated = 1;
                nCount = nNewCount;
                pPoints = pNewPoints;
            } else {
                int bFound = 0;
                if (nCount >= 2) {
                    int i = 0;
                    Vector* pTest = pPoints + 1;
                    do {
                        vFrom = m_position;
                        vTo = *pTest;
                        if (m_pArea->TestDirectLine(m_objectId, vFrom, vTo, fHit, 0.015625f,
                                                    0, 0) == 1) {
                            int nNewCount = nCount - i;
                            Vector* pNewPoints = new Vector[nNewCount]();
                            // Drops the way-points before the first reachable one.
                            memcpy(pNewPoints, pTest, nNewCount * sizeof(Vector));
                            bAllocated = 1;
                            nCount = nNewCount;
                            pPoints = pNewPoints;
                            bFound = 1;
                            break;
                        }
                        i++;
                        pTest++;
                    } while (i + 1 < nCount);
                }
                if (!bFound) {
                    int bRebuilt = 0;
                    if (GetEndWayPoint() != 0) {
                        Vector vEnd, vLast;
                        Vector* pEnd = GetEndWayPoint();
                        if (pEnd != 0) {
                            vEnd = *pEnd;
                            vLast = pPoints[nCount - 1];
                        } else {
                            vEnd.x = 0.0f; vEnd.y = 0.0f; vEnd.z = 0.0f;
                            vLast.x = 0.0f; vLast.y = 0.0f; vLast.z = 0.0f;
                        }
                        vFrom = vEnd;
                        vTo = vLast;
                        if (m_pArea->TestDirectLine(m_objectId, vFrom, vTo, fHit, 0.015625f,
                                                    0, 0) == 1) {
                            unsigned short nWayPoints = m_wayPointCount;
                            Vector* pWayPoints = m_pWayPoints;
                            int nCurrent = 0;
                            for (uint64_t k = 0; k < nWayPoints; k++) {
                                if (pWayPoints + k == m_pCurrentWayPoint) {
                                    nCurrent = static_cast<int>(k);
                                    break;
                                }
                            }
                            int nRemain = nWayPoints - static_cast<unsigned short>(nCurrent);
                            int nNewCount = nRemain + 1;
                            Vector* pNewPoints = new Vector[nNewCount]();
                            memcpy(pNewPoints, pWayPoints + static_cast<unsigned short>(nCurrent),
                                   nRemain * sizeof(Vector));
                            pNewPoints[nRemain] = vTo;
                            bAllocated = 1;
                            nCount = nNewCount;
                            pPoints = pNewPoints;
                            bRebuilt = 1;
                        }
                    }
                    if (!bRebuilt) {
                        Vector vG = pPoints[0];
                        if (g_pAppManager->m_pServerExoApp != 0) {
                            vG.z = 0.0f;
                            vG.z = g_pAppManager->m_pServerExoApp->ComputeHeight(
                                m_pArea->m_areaResRef, vG, 1);
                        }
                        SetPosition(vNew);
                        m_desiredPosition = vNew;
                        SetGroundNormal(vGroundNormal);
                    }
                }
            }
        }
    }
    if (g_pAppManager->m_pClientExoApp->GetCaptureEvents() == 1) {
        CExoString s;
        unsigned long long nTime =
            g_pAppManager->m_pClientExoApp->GetWorldTimer()->GetSnapshotTime();
        s.Format(const_cast<char*>("%d %c %x %f %f %f\n"), static_cast<int>(nTime / 1000), 'Y', m_objectId,
                 static_cast<double>(m_position.x), static_cast<double>(m_position.y), static_cast<double>(m_position.z));
        static_cast<CExoFile*>(g_pAppManager->m_pClientExoApp->GetCaptureEventFile())->Write(s);
    }
    SetWayPoints(static_cast<unsigned short>(nCount), pPoints);
    m_bServerWayPoints = 1;
    if (bAllocated)
        delete[] pPoints;
}

// Submits a walk command: decides what the creature may actually walk to and hands it to the
// server. The direct line wins; otherwise a bisection search along it, and with b == 2 a
// side-step fan search runs before the commit.
int CSWCCreature::ClientSideWalkCommand(Vector& pos, unsigned char a, unsigned char b) {
    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    CSWCArea* pArea = pModule->m_pArea;
    if (GetCurrentAnimation() == SERVER_ANIMATION_DEAD || GetCurrentAnimation() == SERVER_ANIMATION_DEAD_2 ||
        GetCurrentAnimation() == SERVER_ANIMATION_DIE || GetCurrentAnimation() == SERVER_ANIMATION_DIE_2 ||
        GetCurrentAnimation() == CSWCAnimBase::CLIENT_ANIMATION_DIE_3 ||
        GetCurrentAnimation() == SERVER_ANIMATION_DEAD_PRONE ||
        GetCurrentAnimation() == SERVER_ANIMATION_CAST_1 ||
        GetCurrentAnimation() == SERVER_ANIMATION_CAST_2 ||
        GetCurrentAnimation() == SERVER_ANIMATION_CAST_3 ||
        GetCurrentAnimation() == SERVER_ANIMATION_CAST_4 ||
        GetCurrentAnimation() == SERVER_ANIMATION_THROW_SABER_LOOP ||
        GetCurrentAnimation() == SERVER_ANIMATION_CAST_1_OVERLAY ||
        GetCurrentAnimation() == SERVER_ANIMATION_CAST_2_OVERLAY ||
        GetCurrentAnimation() == SERVER_ANIMATION_CAST_3_OVERLAY)
        return 0;
    if (GetCurrentAnimation() == SERVER_ANIMATION_CAST_4_OVERLAY ||
        GetCurrentAnimation() == SERVER_ANIMATION_THROW_SABER_LOOP_OVERLAY ||
        GetCurrentAnimation() == SERVER_ANIMATION_POWER_ATTACK ||
        GetCurrentAnimation() == SERVER_ANIMATION_INJECT ||
        GetCurrentAnimation() == SERVER_ANIMATION_ACTIVATE_ITEM || GetCurrentAnimation() == 10071)
        return 0;
    if (pModule == 0 || m_bCommandable == 0 || pArea == 0)
        return 0;

    float fHit = m_pCreatureAppearance->m_fPersonalSpace;
    float fHeight = m_pCreatureAppearance->GetHeight();
    Vector vResult = {0.0f, 0.0f, 0.0f};
    float fSpeed = 1.0f;

    if (pArea->TestDirectLine(m_objectId, m_desiredPosition, pos, fHit, fHeight, 1, 0) == 1) {
        vResult = pos;
    } else if (b == 1 || b == 2) {
        Vector vDelta;
        vDelta.x = pos.x - m_position.x;
        vDelta.y = pos.y - m_position.y;
        vDelta.z = 0.0f;
        if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z == 0.0f)
            vDelta = Vector(1.0f, 0.0f, 0.0f);
        if (b == 1) {
            if (AnimationStationary(GetCurrentAnimation()) == 0) {
                Vector vN = normalize(vDelta);
                if (m_vMovingOrientation.x * vN.x + m_vMovingOrientation.y * vN.y + m_vMovingOrientation.z * vN.z < 0.5f)
                    return 0;
            }
        }
        float fStep = 0.5f;
        float fDec = 0.25f;
        int bFound = 0;
        do {
            Vector vTest;
            vTest.x = m_desiredPosition.x + fStep * (pos.x - m_desiredPosition.x);
            vTest.y = m_desiredPosition.y + fStep * (pos.y - m_desiredPosition.y);
            vTest.z = m_desiredPosition.z + fStep * (pos.z - m_desiredPosition.z);
            if (pArea->TestDirectLine(m_objectId, m_desiredPosition, vTest, fHit, fHeight, 1, 0) ==
                1) {
                vResult = vTest;
                bFound = 1;
                fStep += fDec;
            } else {
                fStep -= fDec;
            }
            fDec *= 0.5f;
        } while (fDec > 0.005f);
        if (bFound) {
            g_pAppManager->m_pClientExoApp->m_pInternal->ResetDrivingTimers(fStep);
        } else if (b == 2) {
            Vector vFrom = m_desiredPosition;
            Vector vFromDelta;
            vFromDelta.x = pos.x - vFrom.x;
            vFromDelta.y = pos.y - vFrom.y;
            vFromDelta.z = pos.z - vFrom.z;
            Vector vDir = normalize(vFromDelta);
            float fMag = magnitude(vFromDelta);
            Vector vPerp = normalize(Vector(vDir.y, -vDir.x, 0.0f));
            Vector vPerpNeg = normalize(Vector(-vPerp.x, -vPerp.y, -vPerp.z));
            Vector vDirA = vDir;
            Vector vPerpA = vPerpNeg;
            Vector vDirB = vDir;
            Vector vPerpB = vPerp;
            int bA = 1;
            int bB = 1;
            int nIter = 15;
            int nStatusA = 0;
            int nStatusB = 0;
            do {
                Vector vA = normalize(Vector(vDirA.x + vPerpA.x, vDirA.y + vPerpA.y,
                                             vDirA.z + vPerpA.z));
                Vector vB = normalize(Vector(vDirB.x + vPerpB.x, vDirB.y + vPerpB.y,
                                             vDirB.z + vPerpB.z));
                Vector vTest;
                if (bA) {
                    vTest.x = m_position.x + fMag * vA.x;
                    vTest.y = m_position.y + fMag * vA.y;
                    vTest.z = m_position.z + fMag * vA.z;
                    nStatusA = pArea->TestDirectLine(m_objectId, vFrom, vTest, fHit, fHeight, 1, 0);
                }
                if (bB) {
                    vTest.x = m_position.x + fMag * vB.x;
                    vTest.y = m_position.y + fMag * vB.y;
                    vTest.z = m_position.z + fMag * vB.z;
                    nStatusB = pArea->TestDirectLine(m_objectId, vFrom, vTest, fHit, fHeight, 1, 0);
                }
                if (bA) {
                    if (nStatusA != 0) {
                        vPerpA = vA;
                    } else {
                        vDirA = vA;
                        bA = (nStatusB == 0);
                    }
                }
                if (bB) {
                    if (nStatusB != 0) {
                        vPerpB = vB;
                    } else {
                        vDirB = vB;
                        bB = (nStatusA == 0);
                    }
                }
                nIter--;
            } while (nIter != 0 && (bA != 0 || bB != 0));
            if (bA == bB) {
                float fDotA = vDir.x * vPerpA.x + vDir.y * vPerpA.y + vDir.z * vPerpA.z;
                float fDotB = vDir.x * vPerpB.x + vDir.y * vPerpB.y + vDir.z * vPerpB.z;
                if (!(fDotA > fDotB))
                    vPerpA = vPerpB;
            } else if (!bA) {
                if (bB)
                    vPerpA = vPerpB;
                else
                    vPerpA = Vector(0.0f, 0.0f, 0.0f);
            }
            float fProj = vDir.x * vPerpA.x + vDir.y * vPerpA.y + vDir.z * vPerpA.z;
            fProj = fmaxf(fProj, 0.25f);
            float fScale = fMag * fProj;
            vResult.x = m_position.x + vPerpA.x * fScale;
            vResult.y = m_position.y + vPerpA.y * fScale;
            vResult.z = m_position.z + vPerpA.z * fScale;
            if (pArea->TestDirectLine(m_objectId, vFrom, vResult, fHit, fHeight, 1, 0) != 1)
                return 0;
            fSpeed = fProj;
        } else {
            return 0;
        }
    } else {
        return 0;
    }

    if (b == 2) {
        m_fWayPointSpeed = fSpeed;
        m_bHasWayPointSpeed = 1;
    } else {
        m_bHasWayPointSpeed = 0;
        unsigned short nAnim;
        if (a == 1 && m_bMovementSpeedLimited == 0)
            nAnim = SERVER_ANIMATION_RUN;
        else
            nAnim = SERVER_ANIMATION_WALK;
        SetAnimation(nAnim, 0, 0);
    }
    Vector* pWayPoints = new Vector[2];
    pWayPoints[0] = m_desiredPosition;
    pWayPoints[1] = vResult;
#if KOTOR1
// K1: the way-point copy is written out here rather than called.
    if (m_wayPointCount == 2) {
        memcpy(m_pWayPoints, pWayPoints, 2 * sizeof(Vector));
        m_pCurrentWayPoint = m_pWayPoints;
    } else {
        m_wayPointCount = 2;
        if (m_pWayPoints != 0)
            delete[] m_pWayPoints;
        Vector* pNewPoints = new Vector[2]();
        m_pWayPoints = pNewPoints;
        m_pCurrentWayPoint = pNewPoints;
        memcpy(pNewPoints, pWayPoints, 2 * sizeof(Vector));
    }
#else
    SetWayPoints(2, pWayPoints);
#endif
    m_bServerWayPoints = 0;
    delete[] pWayPoints;
    pos = vResult;
    return 1;
}

// Slides a candidate destination around whatever the walkmesh blocks, grounds it, applies it
// and feeds the party follow sampler. Returns 1 when a position was applied or the move was
// preempted, 0 when the slide iterations ran out.
#ifdef K2_HOST_BUILD
// Logs why a moving player creature stays put, once per change of reason.
static void K2LogMoveBlock(CSWCCreature* pCreature, CSWSCreature* pServer, bool bPending)
{
    static char s_szLast[512] = "";
    char szWhy[512];
    int n = 0;
    if (pServer != 0 && pServer->IsDebilitated(0) != 0)
        n += snprintf(szWhy + n, sizeof szWhy - n, "debilitated ");
    if (pServer != 0 && (pServer->m_nAIState & 2) == 0)
        n += snprintf(szWhy + n, sizeof szWhy - n, "0x16e0 %#x ", static_cast<unsigned>(pServer->m_nAIState));
    if (bPending && pServer != 0) {
        n += snprintf(szWhy + n, sizeof szWhy - n, "actions [");
        int i = 0;
        for (CExoLinkedListNode* pNode = pServer->m_actionList.m_pInternal->m_pHead;
             pNode != 0 && i < 6 && n < 400; pNode = pNode->m_pNext, i++) {
            CSWSObjectActionNode* pAction = static_cast<CSWSObjectActionNode*>(pNode->m_pData);
            n += snprintf(szWhy + n, sizeof szWhy - n, "%s%d(%#llx)", i ? " " : "",
                          pAction ? pAction->m_nActionId : -1,
                          pAction ? pAction->m_params[0].m_n : 0ULL);
        }
        n += snprintf(szWhy + n, sizeof szWhy - n, "] ");
    }
    if (pCreature->m_engagedState != 0)
        n += snprintf(szWhy + n, sizeof szWhy - n, "engaged %d ", pCreature->m_engagedState);
    if (pCreature->AnimationDialog(pCreature->m_currentAnimation) != 0)
        n += snprintf(szWhy + n, sizeof szWhy - n, "dialog anim %d ", static_cast<int>(pCreature->m_currentAnimation));
    if (n == 0)
        snprintf(szWhy, sizeof szWhy, "free");
    if (strcmp(szWhy, s_szLast) != 0) {
        fprintf(stderr, "k2: move %s\n", szWhy);
        strcpy(s_szLast, szWhy);
    }
}
#endif

int CSWCCreature::SetPrecalculatedPosition(const Vector& vPrecalc) {
    if (RenderClosestPathPoint) {
        CSWSArea* pArea = g_pAppManager->m_pServerExoApp->GetModule()->GetArea();
        GetServerObject();  // Result discarded.
        pArea->m_pActivePathfindInfo = GetServerObject()->AsSWSCreature()->m_pPathfindInfo;
        unsigned int nPoint;
        if (g_pAppManager->m_pServerExoApp->GetModule()->GetArea()->ClosestPathPoint(
                m_position, nPoint)) {
            CSWSArea* pArea2 = g_pAppManager->m_pServerExoApp->GetModule()->GetArea();
            Vector vPoint;
            vPoint.x = pArea2->m_paPathPoints[static_cast<int>(nPoint)].m_point.x;
            vPoint.y = pArea2->m_paPathPoints[static_cast<int>(nPoint)].m_point.y;
            vPoint.z = 0.0f;
            CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
            pServer->ComputeHeight(static_cast<CSWSArea*>(pServer->GetModule()->GetArea())->m_resRef, vPoint, 0);
        }
    }
    if (RenderTriangles) {
        CSWSCreature* pServerCreature = 0;
        if (GetServerObject() != 0)
            pServerCreature = GetServerObject()->AsSWSCreature();
        int nRoom = static_cast<CSWSArea*>(pServerCreature->GetArea())->GetRoomIndex(vPrecalc, 0);
        pServerCreature = 0;
        if (GetServerObject() != 0)
            pServerCreature = GetServerObject()->AsSWSCreature();
        CSWSRoom* pRoom = static_cast<CSWSArea*>(pServerCreature->GetArea())->GetRoom(nRoom);
        int nTriA, nTriB, nTriC;
        Vector vA = {0.0f, 0.0f, 0.0f};
        Vector vB = {0.0f, 0.0f, 0.0f};
        Vector vC = {0.0f, 0.0f, 0.0f};
        if (pRoom != 0) {
            int nTri = pRoom->m_pSurfaceMesh->FindTriangle(vPrecalc, 1, Vector{1.0f, 1.0f, 0.0f});
            pRoom->m_pSurfaceMesh->GetTriangleVertices(0, nTri, &nTriA, &nTriB, &nTriC);
            pRoom->m_pSurfaceMesh->GetVertex(nTriA, &vA);
            pRoom->m_pSurfaceMesh->GetVertex(nTriB, &vB);
            pRoom->m_pSurfaceMesh->GetVertex(nTriC, &vC);
        }
        pServerCreature = 0;
        if (GetServerObject() != 0)
            pServerCreature = GetServerObject()->AsSWSCreature();
        Vector vEye;
        vEye.x = pServerCreature->m_position.x;
        vEye.y = pServerCreature->m_position.y;
        vEye.z = pServerCreature->m_position.z + 1.5f;
        pServerCreature = 0;
        if (GetServerObject() != 0)
            pServerCreature = GetServerObject()->AsSWSCreature();
        Vector vHead;
        vHead.x = pServerCreature->m_orientation.x + pServerCreature->m_orientation.x + vEye.x;
        vHead.y = pServerCreature->m_orientation.y + pServerCreature->m_orientation.y + vEye.y;
        vHead.z = pServerCreature->m_orientation.z + pServerCreature->m_orientation.z + vEye.z;
        Vector vOut = {0.0f, 0.0f, 0.0f};
        pServerCreature = 0;
        if (GetServerObject() != 0)
            pServerCreature = GetServerObject()->AsSWSCreature();
        static_cast<CSWSArea*>(pServerCreature->GetArea())->ClearLineOfSight(
            vEye, vHead, &vOut, 0, OBJECT_INVALID, OBJECT_INVALID, 0, 0);
        if (pRoom != 0) {
            CSWRoomSurfaceMeshHitInfo hitInfo2(0xffffffff);
            pRoom->m_pSurfaceMesh->CheckAABBAll(vEye, vHead, hitInfo2);
            if (hitInfo2.m_triangleIndex != -1) {
                pRoom->m_pSurfaceMesh->GetTriangleVertices(0, hitInfo2.m_triangleIndex,
                                                           &nTriA, &nTriB, &nTriC);
                pRoom->m_pSurfaceMesh->GetVertex(nTriA, &vA);
                pRoom->m_pSurfaceMesh->GetVertex(nTriB, &vB);
                pRoom->m_pSurfaceMesh->GetVertex(nTriC, &vC);
            }
        }
    }
    CSWSCreature* pServerCreature;
    if (GetServerObject() == 0) {
        pServerCreature = 0;
    } else {
        pServerCreature = GetServerObject()->AsSWSCreature();
        if (pServerCreature != 0 &&
            (pServerCreature->IsDebilitated(0) != 0 ||
             (pServerCreature->m_nAIState & 2) == 0)) {
            return 1;
        }
    }
    unsigned long long serverId =
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(m_objectId);
#ifdef K2_HOST_BUILD
    K2LogMoveBlock(this, pServerCreature,
                   g_pAppManager->m_pServerExoApp->IsActionsPending(serverId) != 0);
#endif
    if (g_pAppManager->m_pServerExoApp->IsActionsPending(serverId) |
        m_engagedState |
        AnimationDialog(m_currentAnimation)) {
        return 1;
    }
#ifdef K2_FIX_ENGINE_BUGS
    if (pServerCreature == 0)
        return 1;
#endif
#if KOTOR1
    GetServerObject();  // K1 re-fetches the server creature here.
    int flagValue = GetServerObject()->AsSWSCreature()->m_pPathfindInfo->m_nPersonalSpaceBits;
#else
    int flagValue = pServerCreature->m_pPathfindInfo->m_nPersonalSpaceBits;
#endif
    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    Vector vDelta;
    vDelta.x = vPrecalc.x - m_position.x;
    vDelta.y = vPrecalc.y - m_position.y;
    vDelta.z = vPrecalc.z - m_position.z;
    Vector vDirTo = normalize(vDelta);
    if (pModule == 0 || m_switchMode != 1)
        return 1;
    if (pModule->m_pArea == 0)
        return 1;
    CWalkHitInfo hitInfo = {};
    hitInfo.m_hitObjectId = OBJECT_INVALID;
    if (renderwalktry) {
        for (int j = 0; j < 8; j++) {
            if (s_walkTrySquareIds[j] != 0xffffffff) {
                AurRemoveDrawableSquare(s_walkTrySquareIds[j]);
                s_walkTrySquareIds[j] = 0xffffffff;
            }
        }
    }
    Vector vDelta2;
    vDelta2.x = m_position.x - vPrecalc.x;
    vDelta2.y = m_position.y - vPrecalc.y;
    vDelta2.z = m_position.z - vPrecalc.z;
    Vector vDirFrom = normalize(vDelta2);
    CPathfindInformation* pPathInfo =
        g_pAppManager->m_pServerExoApp->GetCreaturePathfindInformation(m_objectId);
    if (pPathInfo != 0)
        pPathInfo->m_oidGoalObject = OBJECT_INVALID;
#ifdef K2_FIX_ENGINE_BUGS
    else
        return 1;
#endif
    Vector vTarget = vPrecalc;
    Vector vPrevNormal = {0.0f, 0.0f, 0.0f};
    unsigned int nColor = 0x1de20000;
    int nSubType;
    int i = 1;
    bool bSecond = false;
    do {
        if (renderwalktry) {
            Vector quad[4];
            quad[0] = m_position;
            quad[1] = m_position;
            quad[2] = vTarget;
            quad[3] = vTarget;
#if KOTOR1
            s_walkTrySquareIds[i] = static_cast<unsigned int>(AurAddDrawableSquare(&quad[0].x, (nColor & 0xfffe0000) | 0x1000000));
#else
            s_walkTrySquareIds[i] = static_cast<unsigned int>(AurAddDrawableSquare(
                &quad[0].x, (nColor & 0xfffe0000) | 0x1000000,
                "C:/Users/Vu/Documents/AndroidStudioProjects/KOTOR1-2/KOTOR2_Android/app/src/main/cpp/kotor2/CODE/game/clientcore/nwccreature.cpp",
                11932));
#endif
        }
        int status = pModule->m_pArea->TestDirectLine(m_objectId, m_position, vTarget,
                                                      *reinterpret_cast<const float*>(&flagValue), pPathInfo->m_fHeightAlias,
                                                      1, &hitInfo);
        if (status == -3) {
            CSWSCreature* pOther =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(hitInfo.m_hitObjectId);
            if (pOther != 0) {
                pServerCreature->BumpFriends(pOther, m_position, vTarget);
                status = pModule->m_pArea->TestDirectLine(m_objectId, m_position, vTarget,
                                                          *reinterpret_cast<const float*>(&flagValue), pPathInfo->m_fHeightAlias,
                                                          1, &hitInfo);
                if (status == -3) {
#if KOTOR1
// K1: the server creature is re-fetched per use and the safe-location radius is +1.0.
                    CSWSCreature* pSafe = 0;
                    if (GetServerObject() != 0)
                        pSafe = GetServerObject()->AsSWSCreature();
                    if (static_cast<CSWSArea*>(pSafe->GetArea())->TestSafeLocationPoint(m_position,
                                                                             pPathInfo) != 0) {
                        status = -3;
                    } else {
                        pSafe = 0;
                        if (GetServerObject() != 0)
                            pSafe = GetServerObject()->AsSWSCreature();
                        static_cast<CSWSArea*>(pSafe->GetArea())->ComputeSafeLocation(
                            m_position, 1.0f, pPathInfo, 1, &m_position);
#else
                    if (static_cast<CSWSArea*>(pServerCreature->GetArea())->TestSafeLocationPoint(
                            m_position, pPathInfo) != 0) {
                        status = -3;
                    } else {
                        static_cast<CSWSArea*>(pServerCreature->GetArea())->ComputeSafeLocation(
                            m_position, -1.01f, pPathInfo, 1, &m_position);
#endif
                        status = pModule->m_pArea->TestDirectLine(
                            m_objectId, m_position, vTarget, *reinterpret_cast<const float*>(&flagValue),
                            pPathInfo->m_fHeightAlias, 1, &hitInfo);
                    }
                }
            } else {
                status = -3;
            }
        }
        if (status == 1) {
            CServerExoApp* pServer = g_pAppManager->m_pServerExoApp;
            if (pServer != 0) {
                vTarget.z = 0.0f;
                vTarget.z = pServer->ComputeHeight(pModule->m_pArea->m_areaResRef, vTarget, 1);
                pServer = g_pAppManager->m_pServerExoApp;
            }
            pServer->UpdateSubareasOnMoveTo(m_objectId, m_position, vTarget);
            SetPosition(vTarget);
            m_desiredPosition = vTarget;
            g_pAppManager->m_pServerExoApp->UpDatePlayerPosition(m_objectId, vTarget,
                                                                 m_orientation);
            CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
            pParty->m_followData.ProcessPoint(vTarget, Yaw(m_orientation), *reinterpret_cast<float*>(&flagValue));
            return 1;
        }
        if (status != -1 && status != -2 && status != -3)
            return 0;
        if (hitInfo.m_hitType == 1) {
            bool bFlip = false;
            if (bSecond) {
                float fDot1 =
                    vDirFrom.x * (hitInfo.m_hitNormal.z * zaxis.y -
                                  zaxis.z * hitInfo.m_hitNormal.y) +
                    vDirFrom.y * (zaxis.z * hitInfo.m_hitNormal.x -
                                  hitInfo.m_hitNormal.z * zaxis.x) +
                    vDirFrom.z * (zaxis.x * hitInfo.m_hitNormal.y -
                                  zaxis.y * hitInfo.m_hitNormal.x);
                float fDot2 =
                    vDirFrom.x * (vPrevNormal.z * zaxis.y - zaxis.z * vPrevNormal.y) +
                    vDirFrom.y * (zaxis.z * vPrevNormal.x - vPrevNormal.z * zaxis.x) +
                    vDirFrom.z * (zaxis.x * vPrevNormal.y - zaxis.y * vPrevNormal.x);
                bFlip = (fDot1 > 0.0f && fDot2 < 0.0f) || (fDot1 < 0.0f && fDot2 > 0.0f);
            }
            if (bFlip) {
                if (hitInfo.m_hitSubType != 2)
                    return 0;
                float cx = zaxis.y * vDirTo.z - zaxis.z * vDirTo.y;
                float cy = zaxis.z * vDirTo.x - zaxis.x * vDirTo.z;
                float cz = zaxis.x * vDirTo.y - zaxis.y * vDirTo.x;
                if (!(vDirTo.z * 0.0f +
                          (vDirTo.y * hitInfo.m_triNormalLocal.x -
                           vDirTo.x * hitInfo.m_triNormalLocal.y) >
                      0.0f)) {
                    vTarget.x = m_position.x - 0.03f * cx;
                    vTarget.y = m_position.y - 0.03f * cy;
                    vTarget.z = m_position.z - 0.03f * cz;
                } else {
                    vTarget.x = 0.03f * cx + m_position.x;
                    vTarget.y = 0.03f * cy + m_position.y;
                    vTarget.z = 0.03f * cz + m_position.z;
                }
                nSubType = 2;
            } else if (hitInfo.m_hitSubType == 2) {
                float cx = zaxis.y * vDirTo.z - zaxis.z * vDirTo.y;
                float cy = zaxis.z * vDirTo.x - zaxis.x * vDirTo.z;
                float cz = zaxis.x * vDirTo.y - zaxis.y * vDirTo.x;
                if (!(vDirTo.z * 0.0f +
                          (vDirTo.y * hitInfo.m_hitNormal.x -
                           vDirTo.x * hitInfo.m_hitNormal.y) >
                      0.0f)) {
                    vTarget.x = 0.06f * cx + m_position.x;
                    vTarget.y = 0.06f * cy + m_position.y;
                    vTarget.z = 0.06f * cz + m_position.z;
                } else {
                    vTarget.x = m_position.x - 0.06f * cx;
                    vTarget.y = m_position.y - 0.06f * cy;
                    vTarget.z = m_position.z - 0.06f * cz;
                }
                nSubType = 2;
            } else {
                float dhx = hitInfo.m_hitPosition.x - m_position.x;
                float dhy = hitInfo.m_hitPosition.y - m_position.y;
                float dhz = hitInfo.m_hitPosition.z - m_position.z;
                float dtx = vTarget.x - m_position.x;
                float dty = vTarget.y - m_position.y;
                float dtz = vTarget.z - m_position.z;
                float dHit = dhx * dhx + dhy * dhy + dhz * dhz;
                float dTgt = dtx * dtx + dty * dty + dtz * dtz;
                float dx, dy, dz;
                if (dHit > dTgt) {
                    dx = hitInfo.m_hitPosition.x - vTarget.x;
                    dy = hitInfo.m_hitPosition.y - vTarget.y;
                    dz = hitInfo.m_hitPosition.z - vTarget.z;
                } else {
                    dx = vTarget.x - hitInfo.m_hitPosition.x;
                    dy = vTarget.y - hitInfo.m_hitPosition.y;
                    dz = vTarget.z - hitInfo.m_hitPosition.z;
                }
                float dot = hitInfo.m_hitNormal.x * dx + hitInfo.m_hitNormal.y * dy +
                            hitInfo.m_hitNormal.z * dz;
                vTarget.x -= hitInfo.m_hitNormal.x * dot;
                vTarget.y -= hitInfo.m_hitNormal.y * dot;
                vTarget.z -= hitInfo.m_hitNormal.z * dot;
                nSubType = hitInfo.m_hitSubType;
            }
            vPrevNormal = hitInfo.m_hitNormal;
            if (renderintersects == 1) {
                char buf[256];
                snprintff(buf, sizeof(buf) - 1,
                                   "V1:%3.2f,%3.2f  V2:%3.2f,%3.2f IT:%d FR:%d PF:%d WT:%d",
                                   hitInfo.m_hitPointA.x, hitInfo.m_hitPointA.y,
                                   hitInfo.m_hitPointB.x, hitInfo.m_hitPointB.y,
                                   nSubType, 1, status, i);
            }
        }
        bSecond = true;
        nColor += 0x1de20000;
    } while (i++ < 6);
    return 0;
}

void QueuedCallbackFunction(CAurObject* pObj, char* pName, void* pData);

// Registers the queued-animation callback on the model, once.
void CSWCCreature::SetQuededAnimationCallback() {
    if (m_pQuededAnimationCallback == 0) {
        Gob* pGob = GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
        m_pQuededAnimationCallback =
            pGob->RegisterCallBack(QueuedCallbackFunction, const_cast<char*>("*"), AsSWCObject(), 9999.0f, 0);
    }
}

// Deregisters the queued-animation callback; 1 on success, 0 when none was registered.
int CSWCCreature::ClearQuededAnimationCallback() {
    int ret = 0;
    Gob* pGob = GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
    if (pGob && m_pQuededAnimationCallback) {
        pGob->DeregisterCallback(m_pQuededAnimationCallback);
        m_pQuededAnimationCallback = 0;
        ret = 1;
    }
    return ret;
}

// Dialog-relevant creatures attach to no scene while a minigame is up.
void CSWCCreature::AttachToScene(CAurScene* pScene) {
    if (m_bInParty && g_pAppManager->m_pClientExoApp->GetMiniGame())
        pScene = 0;
    static_cast<CSWCObject*>(this)->CSWCObject::AttachToScene(pScene);  // idiom-exempt: the header models these classes flat.
}

// Loads the monk eye-glow visual for appearances 4 and 6, picking the model by alignment
// band. Any existing visual is deleted first.
void CSWCCreature::LoadMonkVisuals() {
    if ((m_pCreatureAppearance->m_nAppearanceId | 2) != 6)
        return;
    if (m_pStats->m_alignmentGoodEvil <= 40) {
        if (m_bMonkEyesLoaded != 0)
            return;
    } else {
        if (m_bMonkEyes01Loaded != 0)
            return;
    }
    if (m_pMonkVisuals != 0) {
        delete m_pMonkVisuals;
        m_pMonkVisuals = 0;
    }
    unsigned short nAlignment = static_cast<unsigned short>(m_pStats->m_alignmentGoodEvil);
    Gob* pMonk = static_cast<Gob*>(NewCAurObject(
        const_cast<char*>(nAlignment == 3 ? "vfx_monkeyes" : "vfx_monkeyes01"),
        const_cast<char*>("monkeyes"), 0, 0));
    m_pMonkVisuals = pMonk;
    m_bMonkEyesLoaded = (nAlignment == 3);
    m_bMonkEyes01Loaded = (nAlignment != 3);
    if (pMonk != 0) {
        CAurObject* pModel = static_cast<CAurObject*>(GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY));
        pMonk->AttachToObject(pModel, const_cast<char*>("HEADHOOK"), 0);
        m_pMonkVisuals->PlayAnimation(const_cast<char*>("impact"), 1.0f, 0, 0.0f);
    }
}

void CSWCCreature::UnloadMonkVisuals() {
    if (m_pMonkVisuals) {
        m_pMonkVisuals->AttachToScene(0);
        delete m_pMonkVisuals;
        m_pMonkVisuals = 0;
        m_bMonkEyesLoaded = 0;
    m_bMonkEyes01Loaded = 0;
    }
}

// Loads the special-attack visuals for the four monk power attacks and hooks the selected
// one onto the model.
void CSWCCreature::LoadSpecialAttacks(unsigned char nType) {
    switch (nType) {
    case 0:
    case 1:
        if (m_aSpecialAttackGobs[0] == 0) {
            m_aSpecialAttackGobs[0] = static_cast<Gob*>(NewCAurObject(
                const_cast<char*>("vcm_monkki"), const_cast<char*>("kistrike"), 0, 0));
        }
        if (m_aSpecialAttackGobs[1] == 0) {
            m_aSpecialAttackGobs[1] = static_cast<Gob*>(NewCAurObject(
                const_cast<char*>("vcm_monkki"), const_cast<char*>("kistrike"), 0, 0));
        }
        break;
    case 2:
        if (m_aSpecialAttackGobs[2] == 0) {
            m_aSpecialAttackGobs[2] = static_cast<Gob*>(NewCAurObject(
                const_cast<char*>("vcm_monkstun"), const_cast<char*>("stunfist"), 0, 0));
        }
        break;
    case 3:
        if (m_aSpecialAttackGobs[3] == 0) {
            m_aSpecialAttackGobs[3] = static_cast<Gob*>(NewCAurObject(
                const_cast<char*>("vcm_monkquiv"), const_cast<char*>("quivpalm"), 0, 0));
        }
        break;
    }
    if (GetCurrentAnimation() == 10203 || GetCurrentAnimation() == 10202 ||
        GetAnimationBase()->GetOverlayAnimationPlaying(10254) == 1) {
        m_aSpecialAttackGobs[nType]->AttachToObject(
            static_cast<CAurObject*>(GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)), const_cast<char*>("rhand"), 0);
    } else if (GetCurrentAnimation() == 10204 || GetCurrentAnimation() == 10263) {
        m_aSpecialAttackGobs[nType]->AttachToObject(
            static_cast<CAurObject*>(GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)), const_cast<char*>("rfoot_g"), 0);
    } else if (GetCurrentAnimation() == 10264 || GetCurrentAnimation() == 10265 ||
               GetCurrentAnimation() == 10205) {
        m_aSpecialAttackGobs[nType]->AttachToObject(
            static_cast<CAurObject*>(GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)), const_cast<char*>("lfoot_g"), 0);
    }
}

void CSWCCreature::HideEquippedItems(const CResRef& r1, const CResRef& r2) {
    if (m_pModel) {
        static_cast<CSWCObject*>(this)->GetAnimationBase();  // idiom-exempt: the header models these classes flat.
        m_pModel->HideWieldedItems(r1, r2);
        m_bEquippedItemsHidden = 1;
    }
}

float CSWCCreature::GetLightSaberThrowRate() {
    if (GetServerObject()) {
        if (GetServerObject()->AsSWSCreature()) {
            GetServerObject();  // Result discarded.
            return GetServerObject()->AsSWSCreature()->m_lightSaberThrowRate;
        }
    }
    return 37.0f;
}

void CSWCCreature::SetLightSaberThrowRate(float rate) {
    if (GetServerObject()) {
        if (GetServerObject()->AsSWSCreature()) {
            GetServerObject();  // Result discarded.
            GetServerObject()->AsSWSCreature()->m_lightSaberThrowRate = rate;
        }
    }
}

void CSWCCreature::PerformFeat(int nFeat, unsigned long long objId) {
    if (objId != CGameObjectArray::INVALIDOBJID) {
        CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
        if (pMsg)
            pMsg->SendPlayerToServerInput_UseFeat(static_cast<unsigned short>(nFeat), nFeat >> 16, objId);
    }
}

void CSWCCreature::PerformPickupItem(unsigned long long objId) {
    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(objId);
    if (pItem) {
        pItem->m_bInventoryRequestPending = 1;
        CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
        if (pMsg)
            pMsg->SendPlayerToServerInventory_Pickup(objId, CGameObjectArray::INVALIDOBJID);
    }
}

void CSWCCreature::PerformCastSpellLikeAbility(unsigned int spellId, unsigned char subType,
                                               unsigned long long objId, const Vector& pos) {
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg)
        pMsg->SendPlayerToServerInput_CastSpellLikeAbility(spellId, subType, pos, objId);
}

void CSWCCreature::PerformUseItem(unsigned long long itemId, int a, unsigned long long objId,
                                  const Vector& pos) {
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg)
        pMsg->SendPlayerToServerInput_UseItem(itemId, static_cast<unsigned char>(a), static_cast<unsigned char>(a >> 16), objId, pos);
}

void CSWCCreature::PerformGiveItem(unsigned long long itemId, unsigned long long objId,
                                   int count) {
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg)
        pMsg->SendPlayerToServerInput_GiveItem(m_objectId, itemId, objId, count);
}

void CSWCCreature::CreateNewSpellLikeAbilityList() {
    m_pSpellLikeAbilityList = new CExoArrayList<CSWCStats_SpellLikeAbility>();
}

void CSWCCreature::DeleteSpellLikeAbilityList() {
    if (m_pSpellLikeAbilityList) {
        m_pSpellLikeAbilityList->SetSize(0);
        delete m_pSpellLikeAbilityList;
        m_pSpellLikeAbilityList = 0;
    }
}

// Creatures use the appearance's L and S model types, not the full-body F.
int CSWCCreature::IsCreature() {
    CExoString str;
    CExoString column("MODELTYPE");
    g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
        m_pCreatureAppearance->m_nAppearanceId, column, &str);
    unsigned char c = str[0];
    int creature = 0;
    switch (c) {
    case 'L':
    case 'S':
    case 'l':
    case 's':
        creature = 1;
    }
    return creature;
}

int CSWCCreature::InCreatureCombat(CSWCObject* pObj) {
    if (pObj == 0)
        return 0;
    if (IsCreature())
        return 1;
    if (pObj->AsSWCCreature() == 0)
        return 0;
    if (pObj->AsSWCCreature()->IsCreature())
        return 1;
    return 0;
}

int CSWCCreature::InCreatureCombat(unsigned long long objId) {
    CSWCObject* pObj =
        static_cast<CSWCObject*>(g_pAppManager->m_pClientExoApp->GetGameObject(objId)->AsSWCObject());
    if (pObj == 0)
        return 0;
    if (IsCreature())
        return 1;
    if (pObj->AsSWCCreature() == 0)
        return 0;
    if (pObj->AsSWCCreature()->IsCreature())
        return 1;
    return 0;
}

int CSWCCreature::BarkString(unsigned long long objId, const CExoString& str,
                             const CResRef& resRef, int a) {
    LipSync(resRef);
    return BarkBubble(objId, str, resRef, a);
}

unsigned long long CSWCCreature::GetAttackingWeapon(int nAttack) {
    switch (nAttack) {
    case CSWSCombatRound::WEAPON_ATTACK_TYPE_ONHAND:
    case CSWSCombatRound::WEAPON_ATTACK_TYPE_EXTRA: return m_aEquippedItemIds[4];
    case CSWSCombatRound::WEAPON_ATTACK_TYPE_OFFHAND: return m_aEquippedItemIds[5];
    case CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON1: return m_aEquippedItemIds[11];
    case CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON2: return m_aEquippedItemIds[12];
    case CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON3: return m_aEquippedItemIds[13];
    default: return OBJECT_INVALID;
    }
}

// Updates the pop-up bubble that tracks the module's current area.
void CSWCCreature::UpdatePopUpBubble() {
    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (pModule && pModule->m_pArea) {
        if (m_areaObjectId == pModule->m_pArea->m_objectId)
            static_cast<CSWCObject*>(this)->CSWCObject::UpdatePopUpBubble();  // idiom-exempt: the header models these classes flat.
        else
            static_cast<CSWCObject*>(this)->CSWCObject::HidePopUpBubble();  // idiom-exempt: the header models these classes flat.
    }
}

// Blocked while the server reports pending actions or a dialog animation plays.
int CSWCCreature::IsDriveAnimationBlocked() {
    int engaged = m_engagedState;
    unsigned long long serverId =
        g_pAppManager->m_pServerExoApp->ClientToServerObjectId(m_objectId);
    int pending = g_pAppManager->m_pServerExoApp->IsActionsPending(serverId);
    int animDialog = static_cast<CSWCObject*>(this)->CSWCObject::AnimationDialog(m_currentAnimation);  // idiom-exempt: the header models these classes flat.
    return ((animDialog | pending) == 0) ? engaged : 1;
}

// Wires the animation-event callbacks onto the model. Every event registers with the same
// fire distance except the footstep, and every one passes the creature itself as userdata
// except the hit event.
void CSWCCreature::RegisterCallbacks() {
    Gob* pGob = m_pModel->GetModel(CSWCAnimBase::ANIM_MODEL_BODY);
    if (pGob) {
        m_aAnimationCallbacks[0] =
            pGob->RegisterCallBack(FootstepEvent, const_cast<char*>("snd_Footstep"), this, 50.0f, 0);
        m_aAnimationCallbacks[1] =
            pGob->RegisterCallBack(HitEvent, const_cast<char*>("hit"), m_pCurrentAttackData, 9999.0f, 0);
        m_aAnimationCallbacks[3] =
            pGob->RegisterCallBack(HitGroundEvent, const_cast<char*>("snd_hitground"), this, 9999.0f, 0);
        m_aAnimationCallbacks[4] =
            pGob->RegisterCallBack(SwingShortEvent, const_cast<char*>("SwingShort"), this, 9999.0f, 0);
        m_aAnimationCallbacks[5] =
            pGob->RegisterCallBack(SwingLongEvent, const_cast<char*>("SwingLong"), this, 9999.0f, 0);
        m_aAnimationCallbacks[6] =
            pGob->RegisterCallBack(SwingTwirlEvent, const_cast<char*>("SwingTwirl"), this, 9999.0f, 0);
        m_aAnimationCallbacks[7] =
            pGob->RegisterCallBack(HitClashEvent, const_cast<char*>("Clash"), this, 9999.0f, 0);
        m_aAnimationCallbacks[8] =
            pGob->RegisterCallBack(HitContactEvent, const_cast<char*>("Contact"), this, 9999.0f, 0);
        m_aAnimationCallbacks[9] =
            pGob->RegisterCallBack(HitParryEvent, const_cast<char*>("HitParry"), this, 9999.0f, 0);
        m_aAnimationCallbacks[10] =
            pGob->RegisterCallBack(Blur, const_cast<char*>("blur_start"), this, 9999.0f, 0);
        m_aAnimationCallbacks[11] =
            pGob->RegisterCallBack(Unblur, const_cast<char*>("blur_end"), this, 9999.0f, 0);
        m_aAnimationCallbacks[12] =
            pGob->RegisterCallBack(Unblur, const_cast<char*>("doneattack01"), this, 9999.0f, 0);
        m_aAnimationCallbacks[13] =
            pGob->RegisterCallBack(Unblur, const_cast<char*>("doneattack02"), this, 9999.0f, 0);
        m_aAnimationCallbacks[14] =
            pGob->RegisterCallBack(GetPersonalRadius, const_cast<char*>("GetPersonalRadius"), this, 9999.0f, 0);
        m_aAnimationCallbacks[15] =
            pGob->RegisterCallBack(GetCreatureRadius, const_cast<char*>("GetCreatureRadius"), this, 9999.0f, 0);
        m_aAnimationCallbacks[16] =
            pGob->RegisterCallBack(GetPath, const_cast<char*>("GetPath"), this, 9999.0f, 0);
    }
}

void CSWCCreature::UnRegisterCallbacks() {
    Gob* pGob = m_pModel->GetModel(CSWCAnimBase::ANIM_MODEL_BODY);
    if (pGob) {
        for (int i = 0; i < 17; i++) {
            if (m_aAnimationCallbacks[i])
                pGob->DeregisterCallback(m_aAnimationCallbacks[i]);
        }
    }
}

// Reports the appearance's personal-space radius through the event payload.
void GetPersonalRadius(CAurObject* pObject, char* pEvent, void* pData) {
    K2_UNUSED(pEvent);
    float fRadius = 0.0f;
    CSWCCreature* pCreature = static_cast<CSWCCreature*>(static_cast<Gob*>(pObject)->GetExternalData());
    if (pCreature != 0) {
        g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
            pCreature->m_pCreatureAppearance->m_nAppearanceId,
            CTwoDimArrays::COL2DA_APPEARANCETABLE_PERSPACE, &fRadius);
        *static_cast<float*>(pData) = fRadius;
    }
}

// GetPersonalRadius over the creature personal-space column.
void GetCreatureRadius(CAurObject* pObject, char* pEvent, void* pData) {
    K2_UNUSED(pEvent);
    float fRadius = 0.0f;
    CSWCCreature* pCreature = static_cast<CSWCCreature*>(static_cast<Gob*>(pObject)->GetExternalData());
    if (pCreature != 0) {
        g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
            pCreature->m_pCreatureAppearance->m_nAppearanceId,
            CTwoDimArrays::COL2DA_APPEARANCETABLE_CREPERSPACE, &fRadius);
        *static_cast<float*>(pData) = fRadius;
    }
}

struct CGetPathData {
    char  _pad00[8];
    int   m_nPathFloats;
    char  _pad0c[4];
    void* m_pPathFloats;
};

// Clears the path-result slot.
void GetPath(CAurObject* pObject, char* pEvent, void* pData) {
    K2_UNUSED(pObject); K2_UNUSED(pEvent);
    CGetPathData* pPathData = static_cast<CGetPathData*>(pData);
    pPathData->m_nPathFloats = 0;
    pPathData->m_pPathFloats = 0;
}

// Blurs the userdata creature's weapons.
void Blur(CAurObject* pObject, char* pEvent, void* pData) {
    K2_UNUSED(pObject); K2_UNUSED(pEvent);
    static_cast<CSWCCreature*>(pData)->BlurWeapon(1);
}

void Unblur(CAurObject* pObject, char* pEvent, void* pData) {
    K2_UNUSED(pObject); K2_UNUSED(pEvent);
    static_cast<CSWCCreature*>(pData)->BlurWeapon(0);
}

// Forwards to the userdata creature's footstep.
void FootstepEvent(CAurObject* pObject, char* pEvent, void* pData) {
    K2_UNUSED(pObject); K2_UNUSED(pEvent);
    static_cast<CSWCCreature*>(pData)->PlayFootstep();
}

// Plays a single footfall: picks the footstepsounds.2da row from the appearance's footstep
// type (a rolling type plays its loop instead), falls back to row 0 when stealthed and stays
// silent while the server reports silent movement. The ground surface material names the
// column, and the sound only plays within the priority group's audible distance.
void CSWCCreature::PlayFootstep() {
    CExoString sSurface;
    CExoString sSound;
    int nFootstepType;
    float fPitch = 0.0f;
    CSWCArea* pArea = m_pArea;
    if (pArea == 0)
        return;
    g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
        m_pCreatureAppearance->m_nAppearanceId,
        CTwoDimArrays::COL2DA_APPEARANCETABLE_FOOTSTEPTYPE, &nFootstepType);
    if (g_pRules->m_p2DArrays->m_pFootstepSounds->GetCExoStringEntry(
            nFootstepType, CTwoDimArrays::COL2DA_FOOTSTEPSOUNDS_ROLLING, &sSound) != 0)
        return;
    int bSilent = 0;
    if (m_nStealthState == 1) {
        nFootstepType = 0;
#if !KOTOR1  // K1: a stealthed footfall still plays
        CSWSCreature* pServerCreature;
        if (GetServerObject() != 0 &&
            (pServerCreature = GetServerObject()->AsSWSCreature()) != 0)
            bSilent = pServerCreature->m_silentMoveCount != 0;
#endif
    }
    unsigned int nSurface = pArea->GetSurfaceMaterial(m_position);
    g_pRules->m_p2DArrays->m_pSurfaceMaterial->GetCExoStringEntry(
        nSurface, CTwoDimArrays::COL2DA_SURFACEMATERIAL_NAME, &sSurface);
    if (sSurface != "")
        sSurface = sSurface + CExoString(rand_wincompatible() % 3);
    g_pRules->m_p2DArrays->m_pFootstepSounds->GetCExoStringEntry(nFootstepType, sSurface,
                                                                 &sSound);
    g_pRules->m_p2DArrays->m_pFootstepSounds->GetFLOATEntry(
        nFootstepType, CTwoDimArrays::COL2DA_FOOTSTEPSOUNDS_PITCHOFFSET, &fPitch);
    int nGroup;
    if (m_objectId == g_pAppManager->m_pClientExoApp->GetPlayerCreatureId()) {
        nGroup = PRIORITY_GROUP_PLAYER_FOOTSTEPS;
    } else {
        int nSize;
        g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
            m_pCreatureAppearance->m_nAppearanceId,
            CTwoDimArrays::COL2DA_APPEARANCETABLE_SIZECATEGORY, &nSize);
        nGroup = static_cast<unsigned>(nSize - 1) < 3 ? PRIORITY_GROUP_SMALL_CREATURE_FOOTSTEPS
                                                      : PRIORITY_GROUP_LARGE_CREATURE_FOOTSTEPS;
    }
    Vector vListener = {};
    float fUnused;
    float fDistance;
    g_pExoSound->GetPriorityGroupDistance(nGroup, &fUnused, &fDistance);
    g_pExoSound->GetListenerPosition(&vListener);
    if (magnitude(m_position - vListener) > fDistance + 2.0f)
        return;
    if (!bSilent)
        g_pExoSound->Play3DOneShotSound(CResRef(static_cast<char*>(sSound.CStr())), m_position, 0.0f,
                                        static_cast<unsigned char>(nGroup), 0, 0, 0.0f, fPitch);
    if ((nFootstepType | 1) != 7) {
        switch (nSurface) {
        case SURFACE_MATERIAL_DIRT:
            DoFootstepVisual(1, 0);
            m_bLastFootstepVisual = 0;
            break;
        case SURFACE_MATERIAL_WOOD:
            DoFootstepAudio(0);
            break;
        case SURFACE_MATERIAL_WATER:
        case SURFACE_MATERIAL_PUDDLES:
        case SURFACE_MATERIAL_SWAMP:
        case SURFACE_MATERIAL_MUD:
            DoFootstepVisual(0, m_bLastFootstepVisual == 0);
            m_bLastFootstepVisual = 1;
            break;
        default:
            m_bLastFootstepVisual = 0;
            break;
        }
    }
    g_pAppManager->m_pClientExoApp->LookUpAndPerformRumbleWithCutOff(
        nFootstepType, CClientExoApp::RUMBLE_TABLE_FOOTSTEP_SOUNDS, m_position);
}

// Shared body of the Swing*Event callbacks.
void Swing(const CExoString& sName, CSWCCombatAttackData* pData, int nCount) {
    if (pData != 0) {
        // The Swing* callbacks register the creature itself as their user data, though Swing's
        // signature types it CSWCCombatAttackData*: the id read here is the creature's own.
        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            reinterpret_cast<CSWCCreature*>(pData)->m_objectId);
        if (pCreature != 0)
            pCreature->PlaySwingSound(sName, pData, nCount);
    }
}

// Probes a line walkmesh first. It pulls the segment's far end back to the closest walkmesh hit,
// then re-runs the probe through the renderer's own scene hit check with static BSP geometry
// disabled. The scene's result wins when it is non-zero.
int SceneHitCheck(CAurScene* pScene, Vector vFrom, Vector vTo, Vector* pHit, CHitInfo* pInfo) {
    K2_UNUSED(pHit);
    CSWRoomSurfaceMeshHitInfo hitInfo(0xffffffff);
    int nHit = 0;
    CServerExoApp* pServerApp = g_pAppManager->m_pServerExoApp;
    if (pServerApp != 0) {
        CSWSModule* pModule = pServerApp->GetModule();
        if (pModule != 0) {
            CSWSArea* pArea = pModule->GetArea();
            if (pArea != 0) {
                for (int i = 0; pArea->GetRoom(i) != 0; i++) {
                    hitInfo.ClearOutput();
                    if (pArea->GetRoom(i)->CheckAABBAll(vFrom, vTo, hitInfo) != 0) {
                        vTo = hitInfo.m_point;
                        nHit = 1;
                    }
                }
            }
        }
    }
    int nSavedBspParts = hitcheck_static_bsp_parts;
    hitcheck_static_bsp_parts = 0;
    int nSceneHit = pScene->HitCheck(vFrom, vTo, 0, pInfo);
    hitcheck_static_bsp_parts = nSavedBspParts;
    if (nSceneHit != 0)
        nHit = nSceneHit;
    return nHit;
}

static CExoString g_sHeadTurnBoneOverride;

// A debug hook that installs a head-turn bone name and hands back the confirmation text.
const char* overrideheadturnbone(char* pBone) {
    static CExoString sMessage;
    g_sHeadTurnBoneOverride = pBone;
    sMessage.Format(const_cast<char*>("Overriding head turn bone with '%s'"), pBone);
    return sMessage.CStr();
}

// Looks up a client creature by object id; null when the id is not one.
CSWCCreature* GetCreature(unsigned long long id) {
    CGameObject* pObject;
    if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->GetGameObject(id, &pObject)
            == CGameObjectArray::SUCCESS)
        return pObject->AsSWCCreature();
    return 0;
}

// Plays the named swing sound on the creature the userdata names.
void SwingShortEvent(CAurObject* pObject, char* pEvent, void* pData) {
    K2_UNUSED(pObject); K2_UNUSED(pEvent);
    CExoString sName("SwingShort");
    if (pData != 0) {
        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            static_cast<CSWCCreature*>(pData)->m_objectId);
        if (pCreature != 0)
            pCreature->PlaySwingSound(sName, static_cast<CSWCCombatAttackData*>(pData), 3);
    }
}

// SwingShortEvent over "SwingLong".
void SwingLongEvent(CAurObject* pObject, char* pEvent, void* pData) {
    K2_UNUSED(pObject); K2_UNUSED(pEvent);
    CExoString sName("SwingLong");
    if (pData != 0) {
        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            static_cast<CSWCCreature*>(pData)->m_objectId);
        if (pCreature != 0)
            pCreature->PlaySwingSound(sName, static_cast<CSWCCombatAttackData*>(pData), 3);
    }
}

// SwingShortEvent over "SwingTwirl".
void SwingTwirlEvent(CAurObject* pObject, char* pEvent, void* pData) {
    K2_UNUSED(pObject); K2_UNUSED(pEvent);
    CExoString sName("SwingTwirl");
    if (pData != 0) {
        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            static_cast<CSWCCreature*>(pData)->m_objectId);
        if (pCreature != 0)
            pCreature->PlaySwingSound(sName, static_cast<CSWCCombatAttackData*>(pData), 3);
    }
}

// SwingShortEvent over "Clash" with a 2-sound count.
void HitClashEvent(CAurObject* pObject, char* pEvent, void* pData) {
    K2_UNUSED(pObject); K2_UNUSED(pEvent);
    CExoString sName("Clash");
    if (pData != 0) {
        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            static_cast<CSWCCreature*>(pData)->m_objectId);
        if (pCreature != 0)
            pCreature->PlaySwingSound(sName, static_cast<CSWCCombatAttackData*>(pData), 2);
    }
}

// SwingShortEvent over "Parry" with a 2-sound count.
void HitParryEvent(CAurObject* pObject, char* pEvent, void* pData) {
    K2_UNUSED(pObject); K2_UNUSED(pEvent);
    CExoString sName("Parry");
    if (pData != 0) {
        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            static_cast<CSWCCreature*>(pData)->m_objectId);
        if (pCreature != 0)
            pCreature->PlaySwingSound(sName, static_cast<CSWCCombatAttackData*>(pData), 2);
    }
}

// Fires the saber-contact visual on the creature the userdata names.
void HitContactEvent(CAurObject* pObject, char* pEvent, void* pData) {
    K2_UNUSED(pObject); K2_UNUSED(pEvent);
    if (pData != 0) {
        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(
            static_cast<CSWCCreature*>(pData)->m_objectId);
        if (pCreature != 0)
            pCreature->ShowLightSaberContactVisual(static_cast<CSWCCombatAttackData*>(pData));
    }
}

// Plays the foot-fall thud. The appearance's sound-set row indexes the appearance sound set for a
// "Fall*" column picked off the ground surface under the creature.
void HitGroundEvent(CAurObject* pObject, char* pEvent, void* pData) {
    K2_UNUSED(pObject); K2_UNUSED(pEvent);
    CExoString sSound;
    if (pData != 0) {
        CSWCCreature* pCreature = static_cast<CSWCCreature*>(pData);
        int nSoundAppType;
        g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
            pCreature->m_pCreatureAppearance->m_nAppearanceId,
            CTwoDimArrays::COL2DA_APPEARANCETABLE_SOUNDAPPTYPE, &nSoundAppType);
        if (pCreature->m_pArea != 0) {
            int nSurface = pCreature->m_pArea->GetSurfaceMaterial(pCreature->m_position);
            switch (nSurface) {
            case SURFACE_MATERIAL_DIRT:
            case SURFACE_MATERIAL_GRASS:
            case SURFACE_MATERIAL_LEAVES:
                g_pRules->m_p2DArrays->m_pAppearanceSounds->GetCExoStringEntry(
                    nSoundAppType, CExoString("FallDirt"), &sSound);
                break;
            case SURFACE_MATERIAL_STONE:
            case SURFACE_MATERIAL_WOOD:
            case SURFACE_MATERIAL_CARPET:
            case SURFACE_MATERIAL_LAVA:
                g_pRules->m_p2DArrays->m_pAppearanceSounds->GetCExoStringEntry(
                    nSoundAppType, CExoString("FallHard"), &sSound);
                break;
            case SURFACE_MATERIAL_WATER:
            case SURFACE_MATERIAL_PUDDLES:
            case SURFACE_MATERIAL_SWAMP:
            case SURFACE_MATERIAL_MUD:
                g_pRules->m_p2DArrays->m_pAppearanceSounds->GetCExoStringEntry(
                    nSoundAppType, CExoString("FallWater"), &sSound);
                break;
            case SURFACE_MATERIAL_METAL:
                g_pRules->m_p2DArrays->m_pAppearanceSounds->GetCExoStringEntry(
                    nSoundAppType, CExoString("FallMetal"), &sSound);
                break;
            }
        }
        g_pExoSound->Play3DOneShotSound(CResRef(static_cast<char*>(sSound.CStr())), pCreature->m_position,
                                        0.0f, 0, 0, 0, 0.0f, 0.0f);
    }
}

// Hit callback, registered with a live attack record as userdata. Plays the hit sound, the
// target's reaction and the pain sound, or spawns the impact visuals, by attack result.
void HitEvent(CAurObject* pObject, char* pEvent, void* pData) {
    K2_UNUSED(pObject); K2_UNUSED(pEvent);
    CSWCCombatAttackData* pAttackData = static_cast<CSWCCombatAttackData*>(pData);
    CSWCObject* pTarget;
    CSWCCreature* pAttacker;
    if (pData == 0) {
        pTarget = 0;
        pAttacker = 0;
    } else {
        pTarget = static_cast<CSWCObject*>(g_pAppManager->m_pClientExoApp->GetGameObject(
            pAttackData->m_weaponObjectId));
        pAttacker = static_cast<CSWCCreature*>(g_pAppManager->m_pClientExoApp->GetGameObject(
            pAttackData->m_attackerObjectId));
        if (pTarget == 0 || pAttacker == 0)
            return;
    }
    if (pAttacker->AsSWCCreature() == 0)
        return;
    pAttacker = pAttacker->AsSWCCreature();
    if (pAttackData->m_rangedAttackGroup.m_bRangedAttack != 0)
        return;
    unsigned char nResult = pAttackData->m_nAttackResult;
    if (nResult >= 1 && nResult <= 3) {
        goto hit;
    } else if (nResult == 8) {
        if (pTarget->AnimationParry(pTarget->GetCurrentAnimation()) != 0)
            goto parry;
        nResult = pAttackData->m_nAttackResult;
        goto check4;
    } else if (nResult == 5) {
        goto hit;
    }
check4:
    if (nResult == 4 && pTarget->AnimationParry(pTarget->GetCurrentAnimation()) != 0)
        goto parry;
    goto tail;
parry:
    pAttacker->PlayHitSound(pAttackData, 0);
    {
        int nAbortOnParry;
        g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
            pAttacker->m_pCreatureAppearance->m_nAppearanceId, CExoString("ABORTONPARRY"),
            &nAbortOnParry);
        if (nAbortOnParry != 0) {
            if (pAttacker->m_blurWeapon != 0) {
                CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
                    pAttacker->m_aEquippedItemIds[4]);
                if (pItem != 0)
                    g_pRules->GetBaseItem(pItem->m_baseItem);
                if (pItem == 0 || pItem->m_bVisualEffectLoaded != 1)
                    pAttacker->m_blurWeapon = 0;
            }
            pAttacker->AbortFireForgetAnimation(0);
        }
        if (pAttackData->m_pNext != 0)
            pTarget->AbortFireForgetAnimation(0);
    }
    goto tail;
hit:
    pAttacker->PlayHitSound(pAttackData, 0);
    if (pTarget->AsSWCCreature() != 0) {
        CSWCCreature* pTargetCreature = pTarget->AsSWCCreature();
        if (pTargetCreature->IsCreature() != 0) {
            if (pTargetCreature->GetServerObject() != 0) {
                CSWSCreature* pServerCreature =
                    pTargetCreature->GetServerObject()->AsSWSCreature();
                if (pServerCreature != 0 && pServerCreature->IsDebilitated(1) != 0)
                    goto pain;
            }
            if (pAttackData->m_reaxnGroup.m_nFeatId != 0 ||
                pTargetCreature->m_creatureType != CSWCAnimBase::CREATURE_TYPE_FULL)
                pTarget->GetAnimationBase()->SetOverlayAnimation(SERVER_ANIMATION_DAMAGE_GENERIC, 1);
        }
    }
pain:
    if (pTarget->AsSWCCreature() != 0 &&
        pTarget->AsSWCCreature()->m_pStats->m_nCurrentHitPoints > 0 &&
        pAttackData->m_killingBlowGroup.m_bKillingBlow == 0) {
        CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
        CSWCCreature* pPlayer = pParty->GetPlayerCharacter();
        if ((pPlayer != 0 && pTarget->m_objectId == pPlayer->m_objectId) ||
            rand_wincompatible() % 5 == 0) {
            unsigned short nSound = VOICE_CHAT_PAIN_1;
            if ((rand_wincompatible() & 1) == 0)
                nSound = VOICE_CHAT_PAIN_2;
            pTarget->AsSWCCreature()->PlaySoundSetSound(nSound, 1);
        }
    }
tail:
    if (pAttackData->m_nAttackResult < 6 && ((1 << pAttackData->m_nAttackResult) & ATTACK_RESULTS_LANDED) != 0) {
        if (pTarget->AsSWCCreature() != 0) {
            Vector vOffset;
            vOffset.x = pAttacker->m_position.x - pTarget->m_position.x;
            vOffset.y = pAttacker->m_position.y - pTarget->m_position.y;
            vOffset.z = pAttacker->m_position.z - pTarget->m_position.z;
            pTarget->AsSWCCreature()->SpawnHitVisuals(1, pAttackData, vOffset,
                                                      CExoString("impact"));
        } else if (pTarget->AsSWCDoor() != 0 && pTarget->m_pArea != 0) {
            Vector pos = {};
            Quaternion rot;  // Identity quaternion.
            pTarget->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY)
                ->GetPartPosition(const_cast<char*>("lookathook"), &pos, &rot);
            pTarget->m_pArea->ApplyFireAndForgetVisualEffect(VFX_COM_SPARKS_LARGE, pos);
            pTarget->m_pArea->ApplyFireAndForgetVisualEffect(VFX_COM_SPARKS_PARRY_METAL, pos);
        }
    }
}

// Plays one entry of the creature's soundset as a fresh one-shot stream, gated on the server
// twin, so dead or dying creatures only get the death sound. A failed Play deletes the new
// source.
void CSWCCreature::PlaySoundSetSound(unsigned short nSound, int b3D) {
    STR_RES strres;
    CSWSCreature* pServerCreature;
    unsigned char nPriority;
    int nSizeCategory;
    CExoStreamingSoundSource* pOld;
    if (GetSoundSetStrres(nSound, strres) == 0)
        return;
    if (GetServerObject()) {
        if (GetServerObject()->AsSWSCreature()) {
            GetServerObject();  // Result discarded.
            if (GetServerObject()->AsSWSCreature()->GetDead() != 0) {
                if (nSound != CSWSCreature::VOICE_CHAT_DEATH)
                    return;
            } else {
                pServerCreature = 0;
                if (GetServerObject())
                    pServerCreature = GetServerObject()->AsSWSCreature();
                K2_ASSUME(pServerCreature != 0);   // the same twin the two tests above found
                if (pServerCreature->GetDeadTemp() != 0 && nSound != CSWSCreature::VOICE_CHAT_DEATH)
                    return;
            }
        }
    }
    if (g_pAppManager->m_pClientExoApp->GetPlayerCreature() == this) {
        nPriority = PRIORITY_GROUP_PLAYER_CHAT;
    } else if (g_pAppManager->m_pClientExoApp->GetPlayerByGameObjectID(m_objectId) != 0) {
        nPriority = PRIORITY_GROUP_NON_PLAYER_CHAT;
    } else {
        g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
            m_pCreatureAppearance->m_nAppearanceId,
            CTwoDimArrays::COL2DA_APPEARANCETABLE_SIZECATEGORY, &nSizeCategory);
        nPriority = nSizeCategory < 4 ? PRIORITY_GROUP_SMALL_CREATURE_VOCALIZATIONS
                                      : PRIORITY_GROUP_LARGE_CREATURE_VOCALIZATIONS;
    }
    CExoStreamingSoundSource* pSource = m_pStreamingVocalization;
    m_vocalizationType = 0;
    if (pSource != 0) {
        pSource->~CExoStreamingSoundSource();
        operator delete(pSource);
        m_pStreamingVocalization = 0;
    }
    pSource = new CExoStreamingSoundSource(0);
    m_pStreamingVocalization = pSource;
    pSource->Set3D(b3D);
    m_pStreamingVocalization->SetPriorityGroup(nPriority);
    if (b3D != 0) {
        m_pStreamingVocalization->SetPosition(m_position);
    }
#ifdef K2_FIX_ENGINE_BUGS
    if (strres.soundResRef == const_cast<char*>("p_atton_slct1"))
        strres.soundResRef = const_cast<char*>("p_atton_sclt1");
#endif
    if (m_pStreamingVocalization->Play(strres.soundResRef, 4, 0, 0, 0) == 0) {
        pOld = m_pStreamingVocalization;
        if (pOld != 0) {
            pOld->~CExoStreamingSoundSource();
            operator delete(pOld);
        }
        m_pStreamingVocalization = 0;
    }
}

// Reads a string and sound pair out of the soundset (0 when no soundset is loaded).
int CSWCCreature::GetSoundSetStrres(unsigned short nIdx, STR_RES& out) {
    if (m_nSoundSetFile == -1)
        return 0;
    if (m_pSoundSet == 0)
        return 0;
    if (m_pSoundSet->m_pRes->Demand() == 0)
        return 0;
    m_pSoundSet->GetStrres(nIdx, out);
    m_pSoundSet->m_pRes->Release();
    return 1;
}

// Caches the server-sent orientation as a normalized vector, then rotates to it.
void CSWCCreature::ClientServerSyncHandler_Orientation(const Vector& v) {
    SetDesiredStationaryOrientation(v);
    SetDesiredWalkOrientation(v);
    CSWCObject* pBase = static_cast<CSWCObject*>(this);  // idiom-exempt: cast-this-to-base — flat header (no
    if (pBase->CSWCObject::AnimationStationary(pBase->CSWCObject::GetCurrentAnimation()))
        RotateCreatureToOrientation(m_vStationaryOrientation);
    else
        RotateCreatureToOrientation(m_vMovingOrientation);
}

void CSWCCreature::PartyMenuAwareness(unsigned int n, CSWCCreature* p) {
    K2_UNUSED(n); K2_UNUSED(p);
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg) {
        pMsg->SendPlayerToServerInput_UseSkill(3, 0, m_objectId, {0.0f, 0.0f, 0.0f});
    }
}

// Tracks the weapon blur state, except while the on-hand weapon carries the no-blur flag.
void CSWCCreature::BlurWeapon(int blur) {
    if (m_blurWeapon != blur) {
        CSWCItem* pItem =
            g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[4]);
        if (pItem) {
            int nBaseItem = pItem->m_baseItem;
            g_pRules->GetBaseItem(nBaseItem);
            if (pItem->m_bVisualEffectLoaded == 1)
                return;
        }
        m_blurWeapon = blur;
    }
}

// appearance.2da HEAD_ARC_H for our appearance; 40.0 when the cell is empty.
float CSWCCreature::GetHeadTurnHorizontal() {
    float value;
    if (!g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
            m_pCreatureAppearance->m_nAppearanceId,
            CTwoDimArrays::COL2DA_APPEARANCETABLE_HEAD_ARC_H, &value))
        return 40.0f;
    return value;
}

// appearance.2da HEAD_ARC_V for our appearance; 30.0 when the cell is empty.
float CSWCCreature::GetHeadTurnVertical() {
    float value;
    if (!g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
            m_pCreatureAppearance->m_nAppearanceId,
            CTwoDimArrays::COL2DA_APPEARANCETABLE_HEAD_ARC_V, &value))
        value = 30.0f;
    return value;
}

void CSWCCreature::ForceAppearanceUpdate() {
    m_nAppearanceRequestTimer = 0;
    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    m_pCreatureAppearance->m_nAppearanceId = m_nAppearanceId;
    SetAppearance(&m_appearanceInfo, 0xef, 1, 0);
    if (pModule && pModule->m_pArea)
        AttachToScene(pModule->m_pArea->m_pScene);
}

int CSWCCreature::RequestAppearance(CCAppearanceInfo* pInfo, unsigned char flags) {
    if (flags & 1)
        m_pCreatureAppearance->RequestBodyAppearance(pInfo);
    if (flags & 2)
        m_pCreatureAppearance->RequestHeadAppearance(pInfo);
    if (flags & 4)
        m_pCreatureAppearance->RequestTailAppearance(pInfo);
    if (flags & 8)
        m_pCreatureAppearance->RequestWingAppearance(pInfo);
    m_appearanceInfo.core = pInfo->core;
    return 1;
}

void CSWCCreature::ReAttachCamera() {
    CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
    if (m_nAppearanceRequestTimer == 0) {
        if (g_pAppManager->m_pClientExoApp->GetPlayerCreature() == this) {
            if ((pModule != 0) & (m_bCameraReattachPending != 0) && pModule->m_pCamera != 0 &&
                pModule->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_DIALOG) == 0 &&
                pModule->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_FREE_LOOK) == 0) {
                pModule->SetCameraFollow(m_objectId, m_bCameraFindClearView);
                m_bCameraReattachPending = 0;
                m_bCameraFindClearView = 0;
            }
        }
    }
}

// Latches the orientation-lock target: a non-zero flag stores unconditionally, a zero flag
// only while the server creature is alive and either out of combat or at full health.
void CSWCCreature::SetLockOrientationToObject(unsigned long long objId, int flag) {
    if (g_pAppManager->m_pServerExoApp != 0) {
        unsigned long long serverId =
            g_pAppManager->m_pServerExoApp->ClientToServerObjectId(m_objectId);
        CSWSCreature* pServerCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(serverId);
        if (pServerCreature != 0) {
            if (flag != 0) {
                m_lockOrientationObjectId = objId;
            } else if (pServerCreature->GetDead() == 0) {
                if (pServerCreature->GetDeadTemp() == 0)
                    m_lockOrientationObjectId = objId;
            }
        }
    }
}

// No-op for a non-leader party member. In camera mode 5 re-arms the leader indicator,
// restores the module camera, takes input back and drops the on-a-stick look-at target.
void CSWCCreature::ResetCameraToFollow() {
    if (g_pAppManager->m_pClientExoApp->GetSWParty()->GetIndex(m_objectId) != 0)
        return;
    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK) {
        g_bShowIndicator = 1;
        g_pAppManager->m_pClientExoApp->GetModule()->RestoreCamera();
        g_pAppManager->m_pClientExoApp->SetInputClass(0, 1);
    }
    CSWCameraOnAStick* pOnAStick = static_cast<CSWCameraOnAStick*>(
        g_pAppManager->m_pClientExoApp->GetModule()->m_pCamera->GetBehavior(BEHAVIOR_CAMERA_ON_A_STICK));
    if (pOnAStick != 0)
        pOnAStick->ClearLookAtTarget();
}

// Caches the server-sent animation rate, skips the no-sync animation set and the
// already-current animation, then routes fire-and-forget animations to
// AddFireForgetAnimation and the rest to SetAnimation.
void CSWCCreature::ClientServerSyncHandler_Animation(int anim, float f) {
    m_animationSpeed = f;
    switch (anim) {
    case SERVER_ANIMATION_DEAD: case SERVER_ANIMATION_DEAD_2: case SERVER_ANIMATION_ATTACK:
    case SERVER_ANIMATION_DODGE: case SERVER_ANIMATION_PARRY: case SERVER_ANIMATION_DAMAGE:
    case SERVER_ANIMATION_ATTACK_2: case SERVER_ANIMATION_DEAD_PRONE:
        return;
    }
    if (anim == GetCurrentAnimation())
        return;
    if (AnimationFireAndForget(static_cast<unsigned short>(anim)))
        AddFireForgetAnimation(static_cast<unsigned short>(anim), 0);
    else
        SetAnimation(static_cast<unsigned short>(anim), 0, 0);
}

// Fire-and-forget animation entry point. The animation id picks the shape: the stance set
// plays at the creature's own cached rate, no payload plays a fixed duration, and a
// combat-attack payload's kind byte selects one of four arms (resolved attack, fixed-duration
// visual, attack plus queued reaction, plain framed animation). Kinds 2 and 3 do nothing.
void CSWCCreature::AddFireForgetAnimation(unsigned short a, void* p)
{
    if ((a >= SERVER_ANIMATION_HEAD_TURN_LEFT && a <= SERVER_ANIMATION_PAUSE_BORED) ||
        a == SERVER_ANIMATION_PAUSE_FIDGET)
    {
        unsigned int nDuration;
        if (m_animationSpeed != 0.0f)
            nDuration = static_cast<unsigned int>(GetAnimationLength(a) / __builtin_fabsf(m_animationSpeed));
        else
            nDuration = static_cast<unsigned int>(GetAnimationLength(a));
        InsertFireForgetAnimation(a, nDuration, m_animationSpeed, p, 0);
// The ambient-pause animation only survives a fire-forget that carries a payload.
        if (p == 0 && m_bAmbientPausePlaying != 0)
            StopAmbientPauseAnim();
        return;
    }

    if (m_bAmbientPausePlaying != 0)
        StopAmbientPauseAnim();

    if (p == 0)
    {
        unsigned int nDuration;
        if (m_animationSpeed != 0.0f)
            nDuration = static_cast<unsigned int>(GetAnimationLength(a) / __builtin_fabsf(m_animationSpeed));
        else
            nDuration = static_cast<unsigned int>(GetAnimationLength(a));
        if (a == 6)
            nDuration = 50;
        InsertFireForgetAnimation(a, nDuration, m_animationSpeed, 0, 0);
        return;
    }

    CSWCCombatAttackData* pData = static_cast<CSWCCombatAttackData*>(p);
    switch (pData->m_nDataType)
    {
    case CSWCCombatAttackData::ATTACK_DATA_ATTACK:
    {
        GetAnimationBase();
        m_engagedState = (a == SERVER_ANIMATION_ATTACK_2);
        unsigned short nAnimation = a;
        if (UpdateAttackData(&nAnimation, pData) == 0)
        {
            operator delete(p);
            return;
        }
        float fLength = GetAnimationLength(nAnimation);
        unsigned short nFrames = pData->m_nAnimationLength;
        float fSpeed = nFrames != 0 ? fLength / static_cast<float>(nFrames) : 0.0f;
        AbortFireForgetAnimation(0);
        if (GetAnimationBase() != 0 && GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY) != 0)
            GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY)->PlayAnimation(const_cast<char*>(NULL_NAME), 1.0f, 0, 0.0f);
        InsertFireForgetAnimation(nAnimation, nFrames, fSpeed, p, 0);

// Mirror the reaction onto whoever the attack names.
        CSWCCreature* pTarget =
            g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(pData->m_weaponObjectId);
        if (pTarget != 0)
        {
            if (a == SERVER_ANIMATION_ATTACK_2)
            {
                unsigned short nTargetFrames = pData->m_nAnimationLength;
                float fTargetSpeed = nTargetFrames != 0 ? fLength / static_cast<float>(nTargetFrames) : 0.0f;
                if (pData->m_rangedAttackGroup.m_bRangedAttack != 0)
                {
                    if (AnimationLooping(pTarget->GetCurrentAnimation()) != 0 &&
                        pData->m_reaxnGroup.m_nReaxnAnimation != SERVER_ANIMATION_READY)
                        pTarget->InsertFireForgetAnimation(pData->m_reaxnGroup.m_nReaxnAnimation,
                                                           nTargetFrames, fTargetSpeed, 0, 0);
                }
                else
                {
                    // Window the reaction off the attack's own frame count.
                    float fMeleeSpeed = nTargetFrames != 0 ? fLength / static_cast<float>(nTargetFrames) : 0.0f;
                    if (pData->m_reaxnGroup.m_nReaxnAnimation != SERVER_ANIMATION_READY)
                    {
                        pTarget->AbortFireForgetAnimation(0);
                        if (pTarget->GetAnimationBase() != 0 &&
                            pTarget->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY) != 0)
                            pTarget->GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY)
                                ->PlayAnimation(const_cast<char*>(NULL_NAME), 1.0f, 0, 0.0f);
                        pTarget->InsertFireForgetAnimation(pData->m_reaxnGroup.m_nReaxnAnimation,
                                                           nTargetFrames, fMeleeSpeed, 0, 0);
                    }
                }
            }
            else if (AnimationLooping(pTarget->GetCurrentAnimation()) != 0 &&
                     pData->m_reaxnGroup.m_nReaxnAnimation != SERVER_ANIMATION_READY)
            {
                unsigned short nTargetFrames = pData->m_nAnimationLength;
                float fTargetSpeed = nTargetFrames != 0 ? fLength / static_cast<float>(nTargetFrames) : 0.0f;
                pTarget->InsertFireForgetAnimation(pData->m_reaxnGroup.m_nReaxnAnimation, nTargetFrames,
                                                   fTargetSpeed, 0, 0);
            }
            else
            {
                pTarget->GetCurrentAnimation();
            }
        }

        PlayHitSound(pData, 1);
        if (pData->m_rangedAttackGroup.m_bRangedAttack == 0)
        {
// The player always grunts; everyone else one hit in five.
            CSWCCreature* pPC = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
            if ((pPC == 0 || m_objectId != pPC->m_objectId) &&
                rand_wincompatible() % 5 != 0)
                return;
            int nRoll = rand_wincompatible() % 3;
            unsigned short nSound = nRoll == 0 ? 10 : (nRoll == 1 ? 11 : 12);
            PlaySoundSetSound(nSound, 1);
        }
        return;
    }
    case CSWCCombatAttackData::ATTACK_DATA_CAST:
    {
// The long fixed duration is for the force-power casts; everything else gets the short one.
        unsigned int nDuration = 500;
        switch (a)
        {
        case SERVER_ANIMATION_CAST_1:
        case SERVER_ANIMATION_CAST_2:
        case SERVER_ANIMATION_CAST_3:
        case SERVER_ANIMATION_CAST_4:
        case SERVER_ANIMATION_THROW_SABER_LOOP:
        case SERVER_ANIMATION_POWER_ATTACK:
            nDuration = 1330;
            break;
        default:
            break;
        }
        float fLength = GetAnimationLength(a);
        float fSpeed = 0.0f;
        if (nDuration != 0)
            fSpeed = fLength / static_cast<float>(nDuration);
        InsertFireForgetAnimation(a, nDuration, fSpeed, p, 0);
        return;
    }
    case CSWCCombatAttackData::ATTACK_DATA_REACTION:
    {
        // Fetched unconditionally, then discarded by the override below.
        unsigned int nBaseLength = static_cast<unsigned int>(GetAnimationLength(a));
        // The four fall and die transitions run a fixed second.
        unsigned int nDuration =
            static_cast<unsigned short>(a - SERVER_ANIMATION_FALL_FROM_BEHIND) < 4 ? 1000 : nBaseLength;
        float fAttackLength = GetAnimationLength(a);
        float fSpeed = 0.0f;
        if (nDuration != 0)
            fSpeed = fAttackLength / static_cast<float>(nDuration);
        InsertFireForgetAnimation(a, nDuration, fSpeed, 0, 0);

// The reaction is queued behind the attack, so its window is the attack's length plus the
// payload's own lead-in.
        unsigned int nAttackFrames = static_cast<unsigned int>(GetAnimationLength(a));
        float fReactionLength = GetAnimationLength(pData->m_reaxnGroup.m_nReaxnAnimation);
        float fTotal = static_cast<float>(nAttackFrames) + static_cast<float>(pData->m_reaxnGroup.m_nReaxnDelay);
        float fReactionSpeed = fTotal != 0.0f ? fReactionLength / fTotal : 0.0f;
        InsertFireForgetAnimation(pData->m_reaxnGroup.m_nReaxnAnimation, static_cast<unsigned int>(fTotal),
                                  fReactionSpeed, 0, 0);
        operator delete(p);
        return;
    }
    case CSWCCombatAttackData::ATTACK_DATA_FIXED_LENGTH:
    {
        unsigned short nFrames = pData->m_nAnimationLength;
        float fLength = GetAnimationLength(a);
        float fSpeed;
        if (nFrames != 0)
            fSpeed = fLength / static_cast<float>(nFrames);
        else
            fSpeed = 0.0f;
        InsertFireForgetAnimation(a, nFrames, fSpeed, 0, 0);
        operator delete(p);
        return;
    }
    }
}

// Replaces the looping creature-sound source with a fresh one, priority group 6 for the
// player and 7 for everyone else.
void CSWCCreature::SetNewLoopingCreatureSound(const CExoString& s) {
    m_pLoopingCreatureSound = new CExoSoundSource(CResRef(s));
    int nPriority = g_pAppManager->m_pClientExoApp->GetPlayerCreature() == this ? PRIORITY_GROUP_LOOPING_PLAYER
                                                                               : PRIORITY_GROUP_LOOPING_NON_PLAYER;
    m_pLoopingCreatureSound->SetPriorityGroup(static_cast<unsigned char>(nPriority));
    m_pLoopingCreatureSound->SetLooping(1);
}

// Defers the draw animation while combat is paused; with player movement input live, replays
// it as the overlay.
void CSWCCreature::PlayDrawAnimation(unsigned short anim) {
    if (g_pAppManager->m_pClientExoApp->GetPausedByCombat()) {
        m_pendingDrawAnimation = anim;
        return;
    }
#if KOTOR1
    InsertFireForgetAnimation(anim, 1500, 1.0f, 0, 0);
    m_pendingDrawAnimation = 0xffff;
#else
    if (g_pAppManager->m_pClientExoApp->GetPlayerMovementInput()) {
        CSWCAnimBase* pAnimBase = GetAnimationBase();
        pAnimBase->SetOverlayAnimation(
            GetAnimationBase()->ServerToClientAnimation(SERVER_ANIMATION_WIELD_SWAP), 1);
    } else {
        InsertFireForgetAnimation(anim, 1500, 1.0f, 0, 0);
        m_pendingDrawAnimation = 0xffff;
    }
#endif
}

void CSWCCreature::AnimateFireAndForget() {
    if (AnimationFireAndForget(m_currentAnimation) == 1) {
        m_nFireForgetTimeLeftMs -= m_nAIUpdateDeltaMs;
        if (m_nFireForgetTimeLeftMs < 0) {
            if (AnimationOverlay(m_currentAnimation))
                m_pModel->SetOverlayAnimation(m_currentAnimation, 0);
            m_nFireForgetTimeLeftMs = 0;
            SetAnimation(GetLoopingAnimation(), 0, 0);
        }
    }
    if (AnimationLooping(m_currentAnimation) == 1) {
        unsigned short anim;
        float speed;
        unsigned int duration;
        void* pData;
        if (GetTopFireForgetAnimation(&anim, &speed, &duration, &pData) == 1) {
            CSWCCombatAttackData* pCopied1 = 0;
            if (AnimationAttack(anim) || anim == SERVER_ANIMATION_READY) {
                pCopied1 = static_cast<CSWCCombatAttackData*>(pData);
                if (pCopied1)
                    *m_pCurrentAttackData = *pCopied1;
            }
            if ((anim >> 2) == (SERVER_ANIMATION_COMBAT_STEP_FORWARD >> 2))  // the four combat steps
                PlayFootstep();
            CSWCCombatAttackData* pCopied2 = 0;
            switch (anim) {
            case SERVER_ANIMATION_CAST_1:
            case SERVER_ANIMATION_CAST_2:
            case SERVER_ANIMATION_CAST_3:
            case SERVER_ANIMATION_CAST_4:
            case SERVER_ANIMATION_THROW_SABER_LOOP:
            case SERVER_ANIMATION_POWER_ATTACK:
                pCopied2 = static_cast<CSWCCombatAttackData*>(pData);
                if (pCopied2)
                    *m_pCurrentCastData = *pCopied2;
                break;
            }
            TestAndReEquipweapons(m_currentAnimation, anim);
            m_currentAnimation = anim;
            if (AnimationHideEquippedItems(anim)) {
                CResRef r1;
                CResRef r2;
                if (m_pModel) {
                    GetAnimationBase();
                    m_pModel->HideWieldedItems(r1, r2);
                    m_bEquippedItemsHidden = 1;
                }
            }
            if (m_bModelSyncDisabled) {
                m_bModelSyncDisabled = 0;
                Vector pos = m_position;
                Vector ori = m_orientation;
                m_pModel->SetPosition(pos);
                m_pModel->SetOrientation(ori);
                m_pModel->SetAlwaysRender(0);
                m_pModel->SetInCutscene(0);
                AbortFireForgetAnimation(0);
            }
            int bOutOfPlace;
            if (AnimationPlayOutOfPlace(anim)) {
                m_bModelSyncDisabled = 1;
                Quaternion q(1.0f, 0.0f, 0.0f, 0.0f);
                Vector v = {0.0f, 0.0f, 0.0f};
                m_pModel->SetPosition(v);
                m_pModel->SetOrientation(q);
                m_pModel->SetAlwaysRender(1);
                m_pModel->SetInCutscene(1);
                m_bPlayingCutsceneAnimation = 1;
                bOutOfPlace = 1;
            } else {
                bOutOfPlace = 0;
            }
            if (AnimationOverlay(anim)) {
                m_pModel->SetOverlayAnimation(anim, 1);
            } else {
                CSWCAnimBase* pModel = m_pModel;
                float fSpeed = speed;
                pModel->SetAnimation(anim, fSpeed, bOutOfPlace ? 3 : 1, 0.0f);
            }
            m_nFireForgetTimeLeftMs = duration;
            if (pData) {
                CGameObject* pObj = g_pAppManager->m_pClientExoApp->GetGameObject(
                    static_cast<CSWCCombatAttackData*>(pData)->m_weaponObjectId);
                if (pObj && pObj->AsSWCObject()) {
                    pObj->AsSWCObject();
                    { CExoString unused; }
                }
            }
            RemoveTopFireForgetAnimation();
            delete pCopied1;
            delete pCopied2;
        }
    }
}

// appearance.2da FreeLookEffect for our appearance; -1 when the appearance helper is gone.
int CSWCCreature::GetFreeLookVideoEffect() {
    int nEffect = -1;
    CSWCCreatureAppearance* pAppearance = m_pCreatureAppearance;
    if (pAppearance != 0) {
        int bFound;
        {
            C2DA* pAppearanceTable = g_pRules->m_p2DArrays->m_pAppearance;
            unsigned short nAppearanceId = pAppearance->m_nAppearanceId;
            CExoString sColumn("FreeLookEffect");
            bFound = pAppearanceTable->GetINTEntry(nAppearanceId, sColumn, &nEffect);
        }
        if (bFound == 0)
            nEffect = -1;
    }
    return nEffect;
}

// Queues a party-menu item use. Self-targets send a zeroed position, other targets ours. The
// item id's inventory-slot bit is masked off.
#if !KOTOR1
__attribute__((noinline))
#endif
void CSWCCreature::PartyMenuUseItem(unsigned int itemId, CSWCCreature* pTarget) {
#if KOTOR1
// K1 names no target object when the item is used on someone else.
    itemId &= CSWGuiInterfaceAction::ACTION_ID_ITEM_CLEAR;
    unsigned long long id = m_objectId;
    if (pTarget->m_objectId == id) {
        Vector pos = {};
        CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
        if (pMsg)
            pMsg->SendPlayerToServerInput_UseItem(itemId, 0, 0, id, pos);
    } else {
        CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
        if (pMsg)
            pMsg->SendPlayerToServerInput_UseItem(itemId, 0, 0, OBJECT_INVALID, m_position);
    }
#else
    itemId &= CSWGuiInterfaceAction::ACTION_ID_ITEM_CLEAR;
    unsigned long long id = m_objectId;
    if (pTarget->m_objectId == id) {
        Vector pos = {};
        CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
        if (pMsg)
            pMsg->SendPlayerToServerInput_UseItem(itemId, 0, 0, id, pos);
    } else {
        CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
        if (pMsg)
            pMsg->SendPlayerToServerInput_UseItem(itemId, 0, 0, id, m_position);
    }
#endif
}

// Arms the target's combat-mode bit, mirrors it into the client app when the target is the
// player, then queues the item use.
void CSWCCreature::PerformUseHostileItem(unsigned long long itemId, CSWCCreature* pTarget) {
    if (pTarget) {
        pTarget->m_nStateFlags |= 1;
        if (pTarget->m_objectId == g_pAppManager->m_pClientExoApp->GetPlayerCreatureId())
            g_pAppManager->m_pClientExoApp->SetCombatMode(pTarget->m_nStateFlags & 1);
    }
    PartyMenuUseItem(static_cast<unsigned int>(itemId), pTarget);
}

void CSWCCreature::PerformUnEquipItem(unsigned long long itemId) {
    CAppManager** ppMgr = &g_pAppManager;
    CClientExoApp* pClient = (*ppMgr)->m_pClientExoApp;
    if (m_bDisguised != 0) {
        CExoString sMsg("");
        pClient->AddFeedbackMessage(FEEDBACK_CANNOT_USE_ITEM, 0, sMsg);
    } else {
        CSWCItem* pItem = pClient->GetItemByGameObjectID(itemId);
        if (pItem != 0) {
            pItem->m_bInventoryRequestPending = 1;
            CSWCMessage* pMsg = (*ppMgr)->m_pClientExoApp->GetSWCMessage();
            if (pMsg != 0)
                pMsg->SendPlayerToServerInventory_Unequip(itemId, CGameObjectArray::INVALIDOBJID);
        }
    }
}

void CSWCCreature::PartyMenuStealth(unsigned int n, CSWCCreature* pTarget) {
    K2_UNUSED(n); K2_UNUSED(pTarget);
    if (g_pAppManager->m_pServerExoApp->GetPartyTable()->m_soloMode != 0 ||
        g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount == 1) {
        unsigned long long id = m_objectId;
        if (CGameObjectArray::INVALIDOBJID != id) {
            CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
            if (pMsg != 0) {
                Vector pos = {};
                pMsg->SendPlayerToServerInput_UseSkill(2, 0, id, pos);
            }
        }
    } else {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->ShowSoloModeQuery(1);
    }
}

#if KOTOR1
__attribute__((noinline))  // K1: called out of line by the per-frame drivers
#endif
void CSWCCreature::MoveCreatureToPosition(const Vector& vPos) {
    unsigned int nTimer = m_desiredPositionTimer;
    unsigned int nElapsed = nTimer - m_nAIUpdateDeltaMs;
    if (nTimer > m_nAIUpdateDeltaMs) {
        float fDx = m_desiredPosition.x - m_position.x;
        float fDy = m_desiredPosition.y - m_position.y;
        float fDz = m_desiredPosition.z - m_position.z;
        float fDist2 = fDx * fDx + fDy * fDy + fDz * fDz;
        m_desiredPositionTimer = nElapsed;
        if (!(fDist2 < 1e-5f) && !(fDx * fDx + fDy * fDy >= 36.0f)) {
            float t = static_cast<float>(nElapsed) / static_cast<float>(nTimer);
            Vector vNew;
            vNew.x = vPos.x + (m_position.x - vPos.x) * t;
            vNew.y = vPos.y + (m_position.y - vPos.y) * t;
            vNew.z = vPos.z + (m_position.z - vPos.z) * t;
            SetPosition(vNew);
            return;
        }
    }
    m_desiredPositionTimer = 0;
    SetPosition(vPos);
}

// Uses a hostile item from the party menu. The tutorial window gets first refusal for the two
// directly-usable base types, otherwise the target's combat-mode bit is armed and the use is
// queued.
void CSWCCreature::PartyMenuUseHostileItem(unsigned int itemId, CSWCCreature* pTarget) {
#if KOTOR1
// K1: only base type 6 asks the tutorial window, and the arming plus use happens in
// PerformUseHostileItem.
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(itemId);
    if (pItem != 0 && pItem->GetBaseItem()->m_nItemType == ITEM_TYPE_GRENADE &&
        g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_USE_GRENADE, m_objectId, pTarget->m_objectId, itemId))
        return;
    PerformUseHostileItem(itemId, pTarget);
#else
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(itemId);
    if (pItem != 0 &&
        (pItem->GetBaseItem()->m_nItemType == ITEM_TYPE_GRENADE ||
        pItem->GetBaseItem()->m_nItemType == ITEM_TYPE_ROCKET)) {
        if (g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_USE_GRENADE, m_objectId,
                                                             pTarget->m_objectId, itemId))
            return;
    }
    if (pTarget != 0) {
        pTarget->m_nStateFlags |= 1;
        if (pTarget->m_objectId == g_pAppManager->m_pClientExoApp->GetPlayerCreatureId())
            g_pAppManager->m_pClientExoApp->SetCombatMode(pTarget->m_nStateFlags & 1);
    }
    PartyMenuUseItem(itemId, pTarget);
#endif
}

// Blocked while the server reports the creature's item actions as disabled; only the feedback
// message is shown. Otherwise the item is flagged and either equipped into the requested slot
// or picked up first.
void CSWCCreature::PerformEquipItem(unsigned long long itemId, unsigned int nSlot) {
    CAppManager** ppMgr = &g_pAppManager;
    CClientExoApp* pClient = (*ppMgr)->m_pClientExoApp;
    if (m_bDisguised != 0) {
        CExoString sMsg("");
        pClient->AddFeedbackMessage(FEEDBACK_CANNOT_USE_ITEM, 0, sMsg);
    } else {
        CSWCItem* pItem = pClient->GetItemByGameObjectID(itemId);
        if (pItem != 0) {
            CSWCMessage* pMsg = (*ppMgr)->m_pClientExoApp->GetSWCMessage();
            if (pMsg != 0) {
                pItem->m_bInventoryRequestPending = 1;
                if (pItem->m_pArea != 0)
                    pItem->m_nPendingEquipSlot = nSlot;
                if (pItem->m_nPendingEquipSlot == -1)
                    pMsg->SendPlayerToServerInventory_Equip(itemId, nSlot);
                else
                    pMsg->SendPlayerToServerInventory_Pickup(itemId,
                                                             CGameObjectArray::INVALIDOBJID);
            }
        }
    }
}

// Counts the spell-like-ability entries matching a spell id and class that still have uses.
int CSWCCreature::GetSpellLikeAbilityUsesLeft(unsigned int spellId, unsigned char nClass) {
    int nUses = 0;
    if (m_pSpellLikeAbilityList != 0) {
        for (int i = 0; i < m_pSpellLikeAbilityList->m_count; i++) {
            CSWCStats_SpellLikeAbility* pEntry = &m_pSpellLikeAbilityList->m_pArray[i];
            if (pEntry->m_nSpellId == spellId && pEntry->m_nCasterLevel == nClass &&
                pEntry->m_nReady != 0)
                nUses++;
        }
    }
    return nUses;
}

// With b set and the whole-creature part requested while an appearance refresh is pending and
// the part is absent, the refresh runs first.
Gob* CSWCCreature::GetModel(unsigned char a, int b) {
    if (m_pModel != 0) {
        if (b != 0) {
            if ((a == CSWCAnimBase::ANIM_MODEL_BODY) & (m_nAppearanceRequestTimer != 0) &&
                m_pModel->GetModel(CSWCAnimBase::ANIM_MODEL_BODY) == 0) {
                m_nAppearanceRequestTimer = 0;
                CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
                m_pCreatureAppearance->m_nAppearanceId = m_nAppearanceId;
                SetAppearance(&m_appearanceInfo, 0xef, 1, 0);
                if (pModule != 0 && pModule->m_pArea != 0)
                    AttachToScene(pModule->m_pArea->m_pScene);
                ReAttachCamera();
            }
        }
        return m_pModel->GetModel(a);
    }
    return 0;
}

// Valid feat ids go through the tutorial window first, otherwise the attack-feat use is
// queued. Either way the target's combat-mode bit is then armed.
void CSWCCreature::ActionMenuUseAttackFeat(unsigned int nFeat, CSWCCreature* pTarget) {
    if (g_pRules->GetFeat(static_cast<unsigned short>(nFeat)) == 0)
        return;
    if (g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_USE_COMBAT_FEAT, m_objectId, OBJECT_INVALID,
                                                           nFeat & 0xffff))
        return;
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg != 0)
        pMsg->SendPlayerToServerInput_Attack(m_objectId, static_cast<unsigned short>(nFeat));
    if (pTarget != 0) {
        pTarget->m_nStateFlags |= 1;
        if (pTarget->m_objectId == g_pAppManager->m_pClientExoApp->GetPlayerCreatureId())
            g_pAppManager->m_pClientExoApp->SetCombatMode(pTarget->m_nStateFlags & 1);
    }
}

// Blocked while the server reports the creature's item actions as disabled; only the feedback
// message is shown. Otherwise both items are flagged and the equip toggle is queued.
void CSWCCreature::PerformToggleEquip(unsigned long long itemId1, unsigned long long itemId2) {
    CAppManager** ppMgr = &g_pAppManager;
    CClientExoApp* pClient = (*ppMgr)->m_pClientExoApp;
    if (m_bDisguised != 0) {
        CExoString sMsg("");
        pClient->AddFeedbackMessage(FEEDBACK_CANNOT_USE_ITEM, 0, sMsg);
    } else {
        CSWCItem* pItem = pClient->GetItemByGameObjectID(itemId1);
        if (pItem != 0) {
            pItem->m_bInventoryRequestPending = 1;
            CSWCItem* pItem2 = (*ppMgr)->m_pClientExoApp->GetItemByGameObjectID(itemId2);
            if (pItem2 != 0)
                pItem2->m_bInventoryRequestPending = 1;
            CSWCMessage* pMsg = (*ppMgr)->m_pClientExoApp->GetSWCMessage();
            if (pMsg != 0)
                pMsg->SendPlayerToServerInventory_EquipToggle(itemId1, itemId2);
        }
    }
}

// Recomputes the locked-orientation facing toward the lock target, falling back to our own
// orientation.
void CSWCCreature::SetDesiredOrientationForLock() {
    CGameObjectArray* pObjects = g_pAppManager->m_pClientExoApp->GetGameObjectArray();
    if (pObjects != 0) {
        CGameObject* pObject;
        if (pObjects->GetGameObject(m_lockOrientationObjectId, &pObject) ==
            CGameObjectArray::SUCCESS) {
            CSWCObject* pTarget = static_cast<CSWCObject*>(pObject);
            if (pTarget != 0) {
                Vector vDelta;
                vDelta.x = pTarget->m_position.x - m_position.x;
                vDelta.y = pTarget->m_position.y - m_position.y;
                vDelta.z = pTarget->m_position.z - m_position.z;
                if (vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z > 1e-5f) {
                    vDelta.z = 0.0f;
                    SetDesiredStationaryOrientation(normalize(vDelta));
                }
            }
        } else {
            SetOrientation(m_orientation);
        }
    }
}

// Sets a mine from the party menu. When the area forbids mines a message box is shown instead
// of queueing the item use.
void CSWCCreature::ActionMenuSetMine(unsigned int itemId, CSWCCreature* pTarget) {
    K2_UNUSED(pTarget);
    CSWSCreature* pServerCreature = g_pAppManager->m_pServerExoApp->GetPlayerCreature();
    if (pServerCreature == 0 || pServerCreature->GetArea() == 0 ||
        static_cast<CSWSArea*>(pServerCreature->GetArea())->GetPlayerCanSetMines() != 0) {
        itemId &= CSWGuiInterfaceAction::ACTION_ID_ITEM_CLEAR;
        if (g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_SET_MINE, m_objectId, itemId, 0))
            return;
        CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
        if (pMsg != 0)
            pMsg->SendPlayerToServerInput_UseItem(itemId, 0, 0, OBJECT_INVALID, m_position);
    } else {
        CSWGuiMessageBox* pBox =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pSoloModeQueryBox;
        pBox->SetAllowCancel(0);
        pBox->SetMessage(STRREF_MINE_LIMIT_REACHED);
        pBox->SetCallback(0, 0, 0);
        g_pAppManager->m_pClientExoApp->GetSWGuiManager()->AddPanel(pBox, 1, 1);
    }
}

// Starts dialog with our server object: clears its action queue, points it at the party
// leader, queues the dialog input and switches the GUI to the dialog state.
void CSWCCreature::ActionInitiateDialog(unsigned int n, CSWCCreature* pTarget) {
    K2_UNUSED(n); K2_UNUSED(pTarget);
#if KOTOR1
// K1: while the post-dialog cooldown runs, only the party leader may start a talk.
    if (IosDisableInitiateDialogTime > 0.0f) {
        int i;
        for (i = 0; i < 4; i++)
            if (g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0) == this)
                break;
        if (i == 4)
            return;
    }
#endif
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->GetCanClick() != 0 &&
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState == 0 &&
        m_bInCombat == 0 &&
        GetServerObject() != 0) {
        GetServerObject()->ClearAllActions(1);
        CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
        if (pLeader != 0 && GetServerObject()->m_bOrientOnClick != 0 &&
            GetServerObject()->m_bCommandable != 0) {
            Vector vDelta;
            vDelta.x = pLeader->m_position.x - m_position.x;
            vDelta.y = pLeader->m_position.y - m_position.y;
            vDelta.z = pLeader->m_position.z - m_position.z;
            if (GetServerObject() != 0)
                GetServerObject()->SetOrientation(vDelta);
        }
        g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_Dialog(
            m_objectId);
        g_pAppManager->m_pClientExoApp->GetInGameGui()->SetGlobalDialogState(1);
    }
}

// Shoves this creature one body-length away from the party leader: faces back at the leader,
// picks a walkable destination on the push line, installs a two-point push path on the
// server's pathfind scratch and kicks the server quick-walk.
void CSWCCreature::ActionPushAway(unsigned int n, CSWCCreature* pTarget) {
    K2_UNUSED(n); K2_UNUSED(pTarget);
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->GetCanClick() != 0 &&
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState == 0 &&
        m_bInCombat == 0 &&
        GetServerObject() != 0) {
        GetServerObject()->ClearAllActions(1);
        CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0);
        if (pLeader != 0 && GetServerObject()->m_bOrientOnClick != 0 &&
            GetServerObject()->m_bCommandable != 0) {
            Vector vUs = m_position;
            Vector vLeader = pLeader->m_position;
            pLeader->ClearAllActions();
            Vector vDir;
            vDir.x = vUs.x - vLeader.x;
            vDir.y = vUs.y - vLeader.y;
            vDir.z = vUs.z - vLeader.z;
            vDir = normalize(vDir);
            Vector vDest;
            vDest.x = m_position.x + vDir.x;
            vDest.y = m_position.y + vDir.y;
            vDest.z = m_position.z + vDir.z;
            {
                Vector vFacing;
                vFacing.x = -vDir.x;
                vFacing.y = -vDir.y;
                vFacing.z = -vDir.z;
                SetOrientation(vFacing);
            }
            CWalkHitInfo hitInfo = {};
            hitInfo.m_hitObjectId = OBJECT_INVALID;
            const CResRef& areaResRef = static_cast<CSWSArea*>(GetServerObject()->GetArea())->m_resRef;
            GetServerObject();
            int flagValue = GetServerObject()->AsSWSCreature()->m_pPathfindInfo->m_nPersonalSpaceBits;
            GetServerObject();
            int status = g_pAppManager->m_pServerExoApp->TestDirectLine(
                m_objectId, areaResRef, m_position, vDest,
                *reinterpret_cast<const float*>(&flagValue),
                GetServerObject()->AsSWSCreature()->m_pPathfindInfo->m_fHeightAlias, 1, &hitInfo);
            if (status == 0)
                vDest = m_position;
            else if (status != 1)
                vDest = hitInfo.m_hitPosition;
            float* pPath = new float[4];
            pPath[0] = m_position.x;
            pPath[1] = m_position.y;
            pPath[2] = vDest.x;
            pPath[3] = vDest.y;
            GetServerObject();
            if (GetServerObject()->AsSWSCreature()->m_pPathfindInfo->m_nWayPointFloats != 0) {
                GetServerObject();
                float* pOld = GetServerObject()->AsSWSCreature()->m_pPathfindInfo->m_paWayPoints;
                GetServerObject();
                GetServerObject()->AsSWSCreature()->m_pPathfindInfo->m_paWayPoints = 0;
                if (pOld != 0)
                    delete[] pOld;
            }
            GetServerObject();
            GetServerObject()->AsSWSCreature()->m_pPathfindInfo->m_nWayPointFloats = 4;
            GetServerObject();
            GetServerObject()->AsSWSCreature()->m_pPathfindInfo->m_paWayPoints = pPath;
            GetServerObject();
            GetServerObject()->AsSWSCreature()->m_pPathfindInfo->m_nWayPointCursor = 0;
            GetServerObject();
            GetServerObject()->AsSWSCreature()->m_nMoveState = 4;
            (GetServerObject() ? GetServerObject()->AsSWSCreature() : static_cast<CSWSCreature*>(0))
                ->WalkUpdateLocation_QuickWalk(1);
        }
    }
}

#if !KOTOR1
// Applies a behavior pick to the target's server stats, raises the matching feedback window
// and arms the main interface's feedback timer.
void CSWCCreature::ActionMenuSelectBehavior(unsigned int nBehavior, CSWCCreature* pTarget) {
    CSWSCreature* pServerCreature = 0;
    if (pTarget->GetServerObject())
        pServerCreature = pTarget->GetServerObject()->AsSWSCreature();
    CSWSCreatureStats* pStats = pServerCreature != 0 ? pServerCreature->m_pStats : 0;
    if (pStats != 0) {
        int nStrRef = -1;
        int nWindow = TUTORIAL_NONE;
        switch (nBehavior) {
        case CSWGuiInterfaceAction::ACTION_ID_BEHAVIOR_AGGRESSIVE:
            nStrRef = STRREF_BEHAVIOR_AGGRESSIVE_HELP;
            nWindow = TUTORIAL_BEHAVIOR_AGGRESSIVE;
            pStats->m_aiState = AI_STATE_AGGRESSIVE;
            break;
        case CSWGuiInterfaceAction::ACTION_ID_BEHAVIOR_DEFENSIVE:
            break;
        case CSWGuiInterfaceAction::ACTION_ID_BEHAVIOR_RANGED_SUPPORT:
            nStrRef = STRREF_BEHAVIOR_RANGED_HELP;
            nWindow = TUTORIAL_BEHAVIOR_RANGED;
            pStats->m_aiState = AI_STATE_RANGED_SUPPORT;
            break;
        case CSWGuiInterfaceAction::ACTION_ID_BEHAVIOR_STATIONARY:
            nStrRef = STRREF_BEHAVIOR_STATIONARY_HELP;
            nWindow = TUTORIAL_BEHAVIOR_STATIONARY;
            pStats->m_aiState = AI_STATE_STATIONARY;
            break;
        case CSWGuiInterfaceAction::ACTION_ID_BEHAVIOR_SUPPORT:
            if (CSWClass::IsJedi(m_pStats->GetClass(m_pStats->m_nNumClasses - 1) & 0xff)) {
                nStrRef = STRREF_BEHAVIOR_JEDI_SUPPORT_HELP;
                nWindow = TUTORIAL_BEHAVIOR_JEDI;
            } else {
                nStrRef = STRREF_BEHAVIOR_GRENADIER_HELP;
                nWindow = TUTORIAL_BEHAVIOR_GRENADIER;
            }
            pStats->m_aiState = AI_STATE_SUPPORT;
            break;
        }
        g_pAppManager->m_pClientExoApp->ShowTutorialWindow(static_cast<unsigned char>(nWindow), OBJECT_INVALID, OBJECT_INVALID, 0);
        CSWGuiMainInterface* pMainInterface =
            g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface;
        pMainInterface->m_feedbackTimer = 4.0f;
        pMainInterface->m_feedbackStrRef = nStrRef;
    }
}
#endif

// Toggles the stance from the party menu. One id sends stance 1, another arms the target's
// combat-mode bit, when its hostility flag allows for every other party member too, and sends
// stance 0.
void CSWCCreature::ActionMenuStance(unsigned int nStance, CSWCCreature* pTarget) {
    int nMode;
    if (nStance == CSWGuiInterfaceAction::ACTION_ID_STANCE_1) {
        nMode = 1;
    } else if (nStance != CSWGuiInterfaceAction::ACTION_ID_STANCE_0) {
        return;
    } else {
        if (pTarget != 0) {
            pTarget->m_nStateFlags |= 1;
            if (pTarget->m_objectId == g_pAppManager->m_pClientExoApp->GetPlayerCreatureId())
                g_pAppManager->m_pClientExoApp->SetCombatMode(pTarget->m_nStateFlags & 1);
            if (pTarget->m_bInParty != 0) {
                int nCount = g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount;
                if (nCount >= 2) {
                    int i = 1;
                    do {
                        CSWCCreature* pMember =
                            g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(i);
                        pMember->m_nStateFlags |= 1;
                        if (pMember->m_objectId ==
                            g_pAppManager->m_pClientExoApp->GetPlayerCreatureId())
                            g_pAppManager->m_pClientExoApp->SetCombatMode(pMember->m_nStateFlags & 1);
                        i++;
                    } while (i != nCount);
                }
            }
        }
        nMode = 0;
    }
    g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_Stance(
        static_cast<char>(nMode), m_objectId);
}

// Drives the rolling-footstep loop from the current animation: the walk and run animations
// play it at base variance, the two heavy animations at variance + 1, anything else stops it.
void CSWCCreature::UpdateRollingFootstepSound() {
    if (m_pRollingFootstepSound == 0)
        return;
    int nAnim = GetCurrentAnimation();
    switch (nAnim) {
    case SERVER_ANIMATION_WALK: case SERVER_ANIMATION_WALK_BACKWARD: case SERVER_ANIMATION_STEALTH:
    case SERVER_ANIMATION_WALK_INJURED:
        if (m_pRollingFootstepSound != 0) {
            m_pRollingFootstepSound->SetPosition(m_position, 0.0f);
            if (m_bRollingFootstepPlaying != 0 && m_rollingFootstepType == 0)
                return;
            m_pRollingFootstepSound->SetFixedVariance(1.0f);
            m_pRollingFootstepSound->Play();
            m_bRollingFootstepPlaying = 1;
            m_rollingFootstepType = 0;
        }
        break;
    case SERVER_ANIMATION_RUN: case SERVER_ANIMATION_RUN_INJURED:
        if (m_pRollingFootstepSound != 0) {
            m_pRollingFootstepSound->SetPosition(m_position, 0.0f);
            if (m_bRollingFootstepPlaying != 0 && m_rollingFootstepType == 1)
                return;
            m_pRollingFootstepSound->SetFixedVariance(m_rollingFootstepVariance + 1.0f);
            m_pRollingFootstepSound->Play();
            m_bRollingFootstepPlaying = 1;
            m_rollingFootstepType = 1;
        }
        break;
    default:
        if (m_pRollingFootstepSound != 0 && m_bRollingFootstepPlaying != 0) {
            m_pRollingFootstepSound->Stop();
            m_bRollingFootstepPlaying = 0;
            m_rollingFootstepType = 0;
        }
        break;
    }
}

// Recomputes the animation base's weapon-class bytes from the equipped right- and left-hand
// items: an empty right hand is fully unarmed, an empty left hand maps the right item's wield
// byte through the switch, and the lightsaber latch ANDs across both items.
void CSWCCreature::UpdateWeaponClass() {
    unsigned long long idRight = m_aEquippedItemIds[4];
    unsigned long long idLeft = m_aEquippedItemIds[5];
    unsigned char nWeaponClass;
#if !KOTOR1
    int bLightsaber;
#endif
    if (idRight != OBJECT_INVALID) {
        CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(idRight);
        if (pItem == 0) {
            nWeaponClass = 0;
#if !KOTOR1
            bLightsaber = 0;
#endif
        } else {
            CSWBaseItem* pBaseItem = g_pRules->GetBaseItem(pItem->m_baseItem);
#if !KOTOR1
            CSWSItem* pServerItem = pItem->GetServerItem();
            if (pServerItem == 0) {
                bLightsaber = 0;
            } else {
                bLightsaber = pServerItem->IsLightsaber() != 0;
            }
#endif
            if (idLeft == OBJECT_INVALID) {
                switch (pBaseItem->m_weaponWieldType) {
                case CSWBaseItem::WEAPON_WIELD_STUN_BATON: nWeaponClass = CSWCAnimBase::WEAPON_CLASS_STUN_BATON; break;
                case CSWBaseItem::WEAPON_WIELD_ONE_HANDED: nWeaponClass = CSWCAnimBase::WEAPON_CLASS_SINGLE_SABER; break;
                case CSWBaseItem::WEAPON_WIELD_TWO_HANDED: nWeaponClass = CSWCAnimBase::WEAPON_CLASS_TWO_HANDED_SABER; break;
                case CSWBaseItem::WEAPON_WIELD_PISTOL:     nWeaponClass = CSWCAnimBase::WEAPON_CLASS_SINGLE_BLASTER; break;
                case CSWBaseItem::WEAPON_WIELD_RIFLE:      nWeaponClass = CSWCAnimBase::WEAPON_CLASS_RIFLE; break;
                case CSWBaseItem::WEAPON_WIELD_HEAVY:      nWeaponClass = CSWCAnimBase::WEAPON_CLASS_HEAVY_CARBINE; break;
                default: nWeaponClass = 0; break;
                }
                CSWCAnimBase* pAnimBase = GetAnimationBase();
                pAnimBase->m_nWeaponType = pBaseItem->m_nWeaponType;
            } else {
                CSWCItem* pOffHand = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(idLeft);
                if (pOffHand == 0) {
                    nWeaponClass = 0;
                } else {
                    CSWBaseItem* pOffBase = g_pRules->GetBaseItem(pOffHand->m_baseItem);
#if !KOTOR1
                    if (bLightsaber) {
                        CSWSItem* pOffServer = pOffHand->GetServerItem();
                        if (pOffServer == 0) {
                            bLightsaber = 1;
                        } else {
                            bLightsaber = pOffServer->IsLightsaber() ? bLightsaber : 0;
                        }
                    }
#endif
                    nWeaponClass = pOffBase->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_ONE_HANDED ? CSWCAnimBase::WEAPON_CLASS_DUAL_SABER :
                                   pOffBase->m_weaponWieldType == CSWBaseItem::WEAPON_WIELD_PISTOL ? CSWCAnimBase::WEAPON_CLASS_DUAL_BLASTER : 0;
                    CSWCAnimBase* pAnimBase = GetAnimationBase();
                    pAnimBase->m_nWeaponType = pOffBase->m_nWeaponType;
                }
            }
        }
    } else {
        if (idLeft == OBJECT_INVALID) {
            GetAnimationBase()->m_nWeaponType = 2;
#if !KOTOR1
            bLightsaber = 0;
#endif
            nWeaponClass = CSWCAnimBase::WEAPON_CLASS_NATURAL;
        } else {
            g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(idLeft);
            nWeaponClass = 0;
#if !KOTOR1
            bLightsaber = 0;
#endif
        }
    }
    GetAnimationBase()->m_nWeaponClass = nWeaponClass;
#if !KOTOR1  // K1 has no lightsaber flag or feat latch
    GetAnimationBase()->m_bLightsaber = static_cast<unsigned char>(bLightsaber);
    if (GetServerObject()) {
        if (GetServerObject()->AsSWSCreature()) {
            GetServerObject();  // Result discarded.
            if (GetServerObject()->AsSWSCreature()->m_pStats) {
                GetServerObject();  // Result discarded.
                int bHasFeat = GetServerObject()->AsSWSCreature()->m_pStats->HasFeat(CSWSCreatureStats::FEAT_COMPLEX_UNARMED_ANIMS);
                CSWCAnimBase* pAnimBase = GetAnimationBase();
                if (bHasFeat) {
                    pAnimBase->m_bHasUnarmedFeat = 1;
                } else {
                    pAnimBase->m_bHasUnarmedFeat = 0;
                }
            }
        }
    }
#endif
}

#if KOTOR1
// K1: no caller-side gate argument; only the creature's own flag holds the re-power back.
void CSWCCreature::UpdateVisibleWeapons(unsigned int nSlot, CSWCItem* pItem, int a) {
    CSWCItem* pPartItem = 0;
    unsigned char nPart;
    switch (nSlot) {
    case MSG_SLOT_RIGHT_WEAPON:
        nPart = CSWCAnimBase::ANIM_MODEL_RIGHT_ITEM_BODY;
        break;
    case MSG_SLOT_LEFT_WEAPON:
        nPart = CSWCAnimBase::ANIM_MODEL_LEFT_ITEM_BODY;
        break;
    default:
        goto send;
    }
    if (a != 0)
        goto send;
    if (GetAnimationBase()->AnimationExists(SERVER_ANIMATION_DRAW_WEAPON) == 1) {
        int nGate = m_bSuppressWeaponDraw;
        Gob* pPart = GetModel(nPart, 1);
        if (pPart != 0 && pPart->GetExternalDataType() == GAME_OBJECT_TYPE_ITEM)
            pPartItem = static_cast<CSWCItem*>(pPart->GetExternalData());
        if (nGate != 0)
            goto send;
        if (pPartItem == pItem)
            goto send;
        GetAnimationBase();
        m_nAmbientPauseElapsed = 0;
        m_bAmbientPausePlaying = 0;
        if (m_fadeState != 0)
            PlayDrawAnimation(SERVER_ANIMATION_DRAW_WEAPON);
        if (pPartItem != 0)
            pPartItem->PowerItem(0, 0, 1, 0);
    }
send:
    SwitchWeaponEvent(0, 0, this);
}
#endif

// Re-powers the visible weapon part after an equip change and re-fires the draw animation
// when its flag allows.
void CSWCCreature::UpdateVisibleWeapons(unsigned int nSlot, CSWCItem* pItem, int a, int b) {
    CSWCItem* pPartItem = 0;
    unsigned char nPart;
    switch (nSlot) {
    case MSG_SLOT_RIGHT_WEAPON:
        nPart = CSWCAnimBase::ANIM_MODEL_RIGHT_ITEM_BODY;
        break;
    case MSG_SLOT_LEFT_WEAPON:
        nPart = CSWCAnimBase::ANIM_MODEL_LEFT_ITEM_BODY;
        break;
    default:
        goto send;
    }
    if (a != 0)
        goto send;
    if (GetAnimationBase()->AnimationExists(SERVER_ANIMATION_DRAW_WEAPON) == 1) {
        int nGate = m_bSuppressWeaponDraw;
        Gob* pPart = GetModel(nPart, 1);
        if (pPart != 0 && pPart->GetExternalDataType() == GAME_OBJECT_TYPE_ITEM)
            pPartItem = static_cast<CSWCItem*>(pPart->GetExternalData());
        if ((nGate | b) != 0)
            goto send;
        if (pPartItem == pItem)
            goto send;
        GetAnimationBase();
        m_nAmbientPauseElapsed = 0;
        m_bAmbientPausePlaying = 0;
        if (m_fadeState != 0)
            PlayDrawAnimation(SERVER_ANIMATION_DRAW_WEAPON);
        if (pPartItem != 0)
            pPartItem->PowerItem(0, 0, 1, 0);
    }
send:
    SwitchWeaponEvent(0, 0, this);
}

// Weapon-switch event worker, registered by RegisterCallbacks. Gates on the event name, then
// per hand: when the equipped id differs from the last-wielded id or the force flag is set,
// re-wields the item, stamping its possessor, light priority and distortion, or un-powers and
// un-wields the absent one.
void SwitchWeaponEvent(CAurObject* pObject, char* pEvent, void* pData) {
    K2_UNUSED(pObject);
    CSWCCreature* pCreature = static_cast<CSWCCreature*>(pData);
    CSWCItem* pOldItem = 0;  // Initialized where the engine reads it stale.
    int bForceUpdate = 0;
    int bLoading = 0;
    int bUpdate = 1;
    if (pEvent != 0) {
        if (strcmp(pEvent, "AppearanceForceUpdate") == 0) {
            bForceUpdate = 1;
            bUpdate = (pCreature != 0) ? (pCreature->m_bInCombat != 0) : 1;
        }
        bLoading = (strcmp(pEvent, "Loading") == 0);
        if (bLoading != 0)
            bUpdate = 0;
    }
    if (pCreature == 0)
        return;
    unsigned long long idRight = pCreature->m_aEquippedItemIds[4];
    CSWCItem* pItemRight = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(idRight);
    unsigned long long idLeft = pCreature->m_aEquippedItemIds[5];
    CSWCItem* pItemLeft = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(idLeft);
    float fScale;
    if (g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
            pCreature->m_pCreatureAppearance->m_nAppearanceId, CExoString("WEAPONSCALE"),
            &fScale) == 0)
        fScale = 1.0f;
    if ((bForceUpdate != 0) | (pCreature->m_wieldedItemIdRight != idRight)) {
        if (pItemRight != 0) {
            Gob* pGob = pItemRight->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
            pItemRight->SetEquippedBy(pCreature);
            pItemRight->m_possessorObjectId = pCreature->m_objectId;
            if (pGob != 0) {
                pGob->SetLightPriority(
                    (pCreature == g_pAppManager->m_pClientExoApp->GetPlayerCreature()) ? 2 : 4);
                if (pCreature->HasVisualEffectApplied(VFX_DUR_DISTORTION) != 0) {
                    CSWCItem* pOld = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
                        pCreature->m_wieldedItemIdRight);
                    if (pOld != 0)
                        pOld->DisableDistortion();
                    pItemRight->EnableDistortion();
                }
                pCreature->GetAnimationBase()->Wield(pItemRight->m_objectId,
                                                     CExoString("rhand"), 1, fScale);
                if (pCreature->AnimationLooping(pCreature->GetCurrentAnimation()) != 0)
                    pCreature->SetAnimation(pCreature->GetCurrentAnimation(), 0, 0);
                pItemRight->PowerItem(bUpdate, 0, pCreature->m_aLastInstant[4] == 0,
                                      bLoading | bForceUpdate);
            }
        } else {
            Gob* pGob = pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_RIGHT_ITEM_BODY, 1);
            if (pGob != 0) {
                pOldItem = static_cast<CSWCItem*>(pGob->GetExternalData());
                if (pOldItem != 0) {
                    pOldItem->m_possessorObjectId = OBJECT_INVALID;
                    pOldItem->PowerItem(0, 0, pCreature->m_aLastInstant[4] == 0,
                                        bLoading | bForceUpdate);
                }
            }
            pCreature->GetAnimationBase()->Wield(OBJECT_INVALID, CExoString(""), 1, 1.0f);
        }
        pCreature->m_wieldedItemIdRight = idRight;
    }
    if ((bForceUpdate != 0) | (pCreature->m_wieldedItemIdLeft != idLeft)) {
        if (pItemLeft != 0) {
            Gob* pGob = pItemLeft->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
            pItemLeft->SetEquippedBy(pCreature);
            pItemLeft->m_possessorObjectId = pCreature->m_objectId;
            if (pGob != 0) {
                pGob->SetLightPriority(
                    (pCreature == g_pAppManager->m_pClientExoApp->GetPlayerCreature()) ? 2 : 4);
                if (pCreature->HasVisualEffectApplied(VFX_DUR_DISTORTION) != 0) {
                    CSWCItem* pOld = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
                        pCreature->m_wieldedItemIdLeft);
                    if (pOld != 0)
                        pOld->DisableDistortion();
                    pItemLeft->EnableDistortion();
                }
                pCreature->GetAnimationBase()->Wield(pItemLeft->m_objectId,
                                                     CExoString("lhand"), 0, fScale);
                if (pCreature->AnimationLooping(pCreature->GetCurrentAnimation()) != 0)
                    pCreature->SetAnimation(pCreature->GetCurrentAnimation(), 0, 0);
                pItemLeft->PowerItem(bUpdate, 0, pCreature->m_aLastInstant[5] == 0, 0);
            }
        } else {
            Gob* pGob = pCreature->GetModel(CSWCAnimBase::ANIM_MODEL_LEFT_ITEM_BODY, 1);
            if (pGob != 0) {
                pOldItem = static_cast<CSWCItem*>(pGob->GetExternalData());
                if (pOldItem != 0) {
                    pOldItem->m_possessorObjectId = OBJECT_INVALID;
                    goto powerLeft;
                }
            }
            if (pOldItem == 0)
                goto wieldLeft;
powerLeft:
            pOldItem->PowerItem(0, 0, pCreature->m_aLastInstant[4] == 0, 0);
wieldLeft:
            pCreature->GetAnimationBase()->Wield(OBJECT_INVALID, CExoString(""), 0, 1.0f);
        }
        pCreature->m_wieldedItemIdLeft = idLeft;
    }
}

// Stamps one usable-item radial entry, gated on IsLeaderUsableItem: the localized item name
// with its stack or equipped decoration, the item's object id as the action id, the
// party-menu callback and the item icon.
void CSWCCreature::CreateUsableItemEntry(CExoArrayList<CSWGuiInterfaceAction>& list,
                                         CSWSItem* pItem, int nEquipped, unsigned char nType) {
    int nOldCount = list.m_count;
    if (IsLeaderUsableItem(pItem, static_cast<int>(reinterpret_cast<unsigned long long>(pItem)), nType) == 0)
        return;
    CExoString sName;
    pItem->m_localizedName.GetString(g_pAppManager->m_pClientExoApp->GetClientLanguage(),
                                     &sName, 0);
#if KOTOR1
// K1: one caption, no token parsing, always PartyMenuUseItem. Medical items are greyed out
// when nobody they would heal needs it.
    sName.Format(const_cast<char*>("%s (%s)"), sName.CStr(),
                 g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SELF).CStr());
    list.SetSize(nOldCount + 1);
    list.m_pArray[nOldCount].m_sName = sName;
    list.m_pArray[nOldCount].m_nActionId = static_cast<unsigned int>(pItem->m_objectId) | CSWGuiInterfaceAction::ACTION_ID_ITEM;
    list.m_pArray[nOldCount].m_pfnAction = &CSWCCreature::PartyMenuUseItem;
    list.m_pArray[nOldCount].m_nObjectId = m_objectId;
    list.m_pArray[nOldCount].m_resRef = pItem->GetIcon();
    list.m_pArray[nOldCount].m_nCount = pItem->m_stackCount;
    switch (pItem->GetBaseItem()->m_nItemType) {
    case ITEM_TYPE_DROID_REPAIR_EQUIPMENT:
    case ITEM_TYPE_MEDICAL_EQUIPMENT:
        if (m_pStats->m_nCurrentHitPoints < m_pStats->GetMaxHitPoints())
            break;
        if (GetServerObject() != 0) {
            CSWSCreature* pServer = GetServerObject()->AsSWSCreature();
            if (pServer != 0 && pServer->m_bPoisoned != 0)
                return;
        }
        list.m_pArray[nOldCount].m_nFlags &= ~1;
        list.m_pArray[nOldCount].m_nFlags =
            (list.m_pArray[nOldCount].m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_FULL_HEALTH;
        return;
    case ITEM_TYPE_SQUAD_RECOVERY_KIT: {
        int bHurt = m_pStats->m_nCurrentHitPoints < m_pStats->GetMaxHitPoints();
        for (int i = 1; i <= 2; i++) {
            CSWCCreature* pMember = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(i);
            if (pMember != 0 && pMember->GetServerObject() != 0 &&
                pMember->GetServerObject()->AsSWSCreature() != 0 &&
                pMember->m_pStats->m_nCurrentHitPoints < pMember->m_pStats->GetMaxHitPoints())
                bHurt = 1;
        }
        if (GetServerObject() != 0 && GetServerObject()->AsSWSCreature() != 0 && bHurt)
            return;
        list.m_pArray[nOldCount].m_nFlags &= ~1;
        list.m_pArray[nOldCount].m_nFlags =
            (list.m_pArray[nOldCount].m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_FULL_HEALTH;
        return;
    }
    }
    if (nEquipped != 0) {
        CSWItemProperty* pProperty;
        pItem->GetPropertyByType(&pProperty, ITEM_PROPERTY_ACTIVATE_ITEM, 0);
        list.m_pArray[nOldCount].m_nFlags =
            (list.m_pArray[nOldCount].m_nFlags & ~1) | (pProperty->m_bUseable & 1);
    }
#else
    if (pItem->m_stackCount >= 2) {
        if (nEquipped != 0) {
            sName.Format(const_cast<char*>("%s (%s) (%d)"), sName.CStr(),
                         g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SELF).CStr(),
                         pItem->m_stackCount);
        } else {
            sName.Format(const_cast<char*>("%s (%d)"), sName.CStr(), pItem->m_stackCount);
        }
    } else {
        if (nEquipped != 0) {
            sName.Format(const_cast<char*>("%s (%s)"), sName.CStr(),
                         g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_SELF).CStr());
        } else {
            sName.Format(const_cast<char*>("%s "), sName.CStr());
        }
    }
    int nLt = sName.Find('<', 0);
    int nLb = sName.Find('{', 0);
    if ((nLt & nLb) != -1) {
        CSWCCreature* pPC = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
        g_pSWTlkTable->ParseStr(sName, pPC, 1, 1);
    }
    list.SetSize(list.m_count + 1);
    list.m_pArray[nOldCount].m_sName = sName;
    list.m_pArray[nOldCount].m_nActionId =
        static_cast<unsigned int>(pItem->m_objectId) | CSWGuiInterfaceAction::ACTION_ID_ITEM;
    list.m_pArray[nOldCount].m_pfnAction =
        (nType == 1) ? &CSWCCreature::PartyMenuUseItemSelect
                     : &CSWCCreature::PartyMenuUseItem;
    list.m_pArray[nOldCount].m_nObjectId = m_objectId;
    list.m_pArray[nOldCount].m_resRef = pItem->GetIcon();
    CSWBaseItem* pBaseItem = pItem->GetBaseItem();
    K2_UNUSED(pBaseItem);
    if (nEquipped != 0 && pItem->GetBaseItem()->m_nItemType != ITEM_TYPE_WRIST_LAUNCHER) {
        CSWItemProperty* pProperty;
        pItem->GetPropertyByType(&pProperty, ITEM_PROPERTY_ACTIVATE_ITEM, 0);
        list.m_pArray[nOldCount].m_nFlags =
            (pProperty->m_bUseable & 1) | (list.m_pArray[nOldCount].m_nFlags & ~1);
    }
#endif
}

// Unequips the seven weapon slots and clears both wield slots on the model.
void CSWCCreature::StripEquippedItems() {
    for (int i = 0; i != 20; i++) {
        unsigned long long itemId;
        switch (i - 1) {
        case 0:  itemId = m_aEquippedItemIds[0]; break;
        case 1:  itemId = m_aEquippedItemIds[1]; break;
        case 3:  itemId = m_aEquippedItemIds[2]; break;
        case 7:  itemId = m_aEquippedItemIds[3]; break;
        case 15: itemId = m_aEquippedItemIds[4]; break;
        case 31: itemId = m_aEquippedItemIds[5]; break;
        case 63: itemId = m_aEquippedItemIds[6]; break;
        default: itemId = CGameObjectArray::INVALIDOBJID; break;
        }
        if (itemId == CGameObjectArray::INVALIDOBJID)
            continue;
        CGameObject* pObj;
        if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->GetGameObject(itemId, &pObj) !=
            CGameObjectArray::SUCCESS)
            continue;
        static_cast<CSWCItem*>(pObj->AsSWCItem())->SetEquippedBy(0);
    }
    if (m_pModel != 0) {
        m_pModel->Wield(CGameObjectArray::INVALIDOBJID, CExoString(""), 0, 1.0f);
        m_pModel->Wield(CGameObjectArray::INVALIDOBJID, CExoString(""), 1, 1.0f);
    }
}

// Plays a swing whoosh: the sound variant comes from the on-hand weapon's weaponsounds.2da
// row, overridden by the off-hand weapon when that is a lightsaber. The pitch offset rides
// along from the table.
int CSWCCreature::PlaySwingSound(const CExoString& sBase, CSWCCombatAttackData* pData, int nCount) {
    K2_UNUSED(pData);
    unsigned char nSoundType = 0;
    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[4]);
    if (pItem != 0 && pItem->GetBaseItem() != 0)
        nSoundType = pItem->GetBaseItem()->m_bIsPC;
    CSWCItem* pOffHand = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[5]);
    if (pOffHand != 0 && pOffHand->GetBaseItem() != 0) {
        CSWSItem* pServerItem = pOffHand->GetServerItem();
        if (pServerItem != 0 && pServerItem->IsLightsaber() == 1)
            nSoundType = pOffHand->GetBaseItem()->m_bIsPC;
    }
    nCount = rand_wincompatible() % nCount;
    CExoString sSound;
    CExoString sNum(nCount);
    sNum = sBase + sNum;
    g_pRules->m_p2DArrays->m_pWeaponSounds->GetCExoStringEntry(nSoundType, sNum, &sSound);
    float fPitch = 0.0f;
    g_pRules->m_p2DArrays->m_pWeaponSounds->GetFLOATEntry(
        nSoundType, CTwoDimArrays::COL2DA_WEAPONSOUNDS_PITCHOFFSET, &fPitch);
    return g_pExoSound->Play3DOneShotSound(CResRef(sSound), m_position, 1.5f, PRIORITY_GROUP_COMBAT, 0, 0, 0.0f,
                                           fPitch);
}

// Plays the impact sound for a landed attack: the material is resolved off the hit target's
// armor, or off a door or placeable, unless the attack was parried, in which case the sound is
// simply "Parry". Material plus a random suffix names the weaponsounds.2da column.
int CSWCCreature::PlayHitSound(CSWCCombatAttackData* pData, int a) {
    if (pData == 0)
        return 0;
    CSWCObject* pObject = static_cast<CSWCObject*>(g_pAppManager->m_pClientExoApp->GetGameObject(
        pData->m_weaponObjectId));
    if (pObject == 0)
        return 0;
    if (pObject->AsSWCObject() == 0)
        return 0;
    CSWCObject* pTarget = pObject->AsSWCObject();
#if KOTOR1
    int nSoundAppType = 0;
    int nSoundType = 0;
#else
    int nSoundType = 0;
    int nSoundAppType = 0;
#endif
    CExoString sSound;
    float fPitch = 0.0f;
    if (a == 0 && pData->m_rangedAttackGroup.m_bRangedAttack == 0) {
        g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
            m_pCreatureAppearance->m_nAppearanceId,
            CTwoDimArrays::COL2DA_APPEARANCETABLE_SOUNDAPPTYPE, &nSoundAppType);
        g_pRules->m_p2DArrays->m_pAppearanceSounds->GetINTEntry(
            nSoundAppType, CTwoDimArrays::COL2DA_APPEARANCESOUNDS_WEAPON, &nSoundType);
        if (nSoundType == 0) {
            unsigned long long nItemId;
            if (pData->m_nWeaponAttackType == 1 || pData->m_nWeaponAttackType == 6 ||
                GetAnimationBase()->m_nWeaponClass == CSWCAnimBase::WEAPON_CLASS_TWO_HANDED_SABER)
                nItemId = m_aEquippedItemIds[4];
            else
                nItemId = pData->m_nWeaponAttackType == 2 ? m_aEquippedItemIds[5]
                                                : CGameObjectArray::INVALIDOBJID;
            CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(nItemId);
            if (pItem != 0)
                nSoundType = g_pRules->GetBaseItem(pItem->m_baseItem)->m_bIsPC;
        }
        if ((pData->m_nAttackResult == 8 || pData->m_nAttackResult == 4) &&
            pTarget->AnimationParry(pTarget->GetCurrentAnimation()) != 0) {
            CExoString sKey = CExoString("Parry") + CExoString(rand_wincompatible() % 2);
            g_pRules->m_p2DArrays->m_pWeaponSounds->GetCExoStringEntry(nSoundType, sKey, &sSound);
            g_pRules->m_p2DArrays->m_pWeaponSounds->GetFLOATEntry(
                nSoundType, CTwoDimArrays::COL2DA_WEAPONSOUNDS_PITCHOFFSET, &fPitch);
        } else {
            CExoString sMaterial;
            if (pTarget->m_objectType == CGameObject::OBJECT_TYPE_CREATURE) {
                CSWCCreature* pCreature = pTarget->AsSWCCreature();
                if (pCreature->GetForceShield() != 0)
                    sMaterial = "forcefield";
                else if (pCreature->m_bStoneTexture != 0 || pCreature->m_bShadeTexture != 0)
                    sMaterial = "stone";
                else if (pCreature->m_bBarkTexture != 0)
                    sMaterial = "wood";
                else {
                    g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
                        pCreature->m_pCreatureAppearance->m_nAppearanceId,
                        CTwoDimArrays::COL2DA_APPEARANCETABLE_SOUNDAPPTYPE, &nSoundAppType);
                    g_pRules->m_p2DArrays->m_pAppearanceSounds->GetCExoStringEntry(
                        nSoundAppType, CTwoDimArrays::COL2DA_APPEARANCESOUNDS_ARMORTYPE,
                        &sMaterial);
                    if (sMaterial.IsEmpty()) {
                        CSWCItem* pArmor =
                            g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(
                                pCreature->m_aEquippedItemIds[1]);
                        if (pArmor != 0)
                            sMaterial = pArmor->GetBaseItem()->m_sArmorType;
                        if (sMaterial.IsEmpty())
                            sMaterial = "leather";
                    }
                }
            } else if (pTarget->m_objectType == CGameObject::OBJECT_TYPE_DOOR) {
                CSWCDoor* pDoor = pTarget->AsSWCDoor();
                if (pDoor->m_nAppearance == 0)
                    g_pRules->m_p2DArrays->m_pGenericDoors->GetINTEntry(
                        pDoor->m_nGenericType, CTwoDimArrays::COL2DA_GENERICDOORS_SOUNDAPPTYPE,
                        &nSoundAppType);
                else
                    g_pRules->m_p2DArrays->m_pDoorTypes->GetINTEntry(
                        pDoor->m_nAppearance, CTwoDimArrays::COL2DA_DOORTYPES_SOUNDAPPTYPE,
                        &nSoundAppType);
                g_pRules->m_p2DArrays->m_pPlaceableSounds->GetCExoStringEntry(
                    nSoundAppType, CTwoDimArrays::COL2DA_PLACEABLESOUNDS_ARMORTYPE, &sMaterial);
            } else if (pTarget->m_objectType == CGameObject::OBJECT_TYPE_PLACEABLE) {
                CSWCPlaceable* pPlaceable = pTarget->AsSWCPlaceable();
                g_pRules->m_p2DArrays->m_pPlaceables->GetINTEntry(
                    pPlaceable->m_nAppearanceType, CTwoDimArrays::COL2DA_PLACEABLES_SOUNDAPPTYPE,
                    &nSoundAppType);
                g_pRules->m_p2DArrays->m_pPlaceableSounds->GetCExoStringEntry(
                    nSoundAppType, CTwoDimArrays::COL2DA_PLACEABLESOUNDS_ARMORTYPE, &sMaterial);
            }
            sMaterial = sMaterial + CExoString(rand_wincompatible() % 2);
            g_pRules->m_p2DArrays->m_pWeaponSounds->GetCExoStringEntry(nSoundType, sMaterial,
                                                                       &sSound);
            g_pRules->m_p2DArrays->m_pWeaponSounds->GetFLOATEntry(
                nSoundType, CTwoDimArrays::COL2DA_WEAPONSOUNDS_PITCHOFFSET, &fPitch);
        }
        sSound.CStr();
        g_pExoSound->Play3DOneShotSound(CResRef(static_cast<char*>(sSound.CStr())), m_position, 1.5f, PRIORITY_GROUP_COMBAT, 0,
                                        0, 0.0f, fPitch);
    }
    return 1;
}

// Spawns or recycles one hit visual in the creature's 12-slot pool: the attack picks the
// visualeffects.2da row, the row's impact-node column names the model node, and the visual is
// oriented toward the attack and attached to the equipped weapon's hook.
int CSWCCreature::SpawnHitVisuals(unsigned char a, CSWCCombatAttackData* pData, const Vector& v,
                                  const CExoString& s) {
    CExoString sImpact(s);
    if (a != 1)
        return 0;
    CExoString sRow;
    if (pData->m_rangedAttackGroup.m_bRangedAttack != 0)
        sRow = CExoString(VFX_COM_BLASTER_IMPACT);
    else if (pData->m_nWeaponAttackType == 7)
        sRow = CExoString(VFX_COM_SPARKS_LARGE);
    else
        sRow = CExoString(VFX_COM_SPARKS_LIGHTSABER);
    CExoString sNode;
    g_pRules->m_p2DArrays->m_pVisualEffect->GetCExoStringEntry(
        sRow, CExoString("imp_impact_node"), &sNode);
    for (int i = 0; i < 12; i++) {
        Gob* pGob = m_aHitVisualGobs[i];
        float fDot;
        float fAngle;
        unsigned long long nItemId;
        CSWCItem* pItem;
        if (pGob != 0) {
            float fTime = 0.0f;
            pGob->GetAnimation(const_cast<char*>("impact"), &fTime, 0);
            if (fTime != -1.0f)
                continue;
            delete pGob;
            m_aHitVisualGobs[i] = 0;
        }
        pGob = static_cast<Gob*>(NewCAurObject(static_cast<char*>(sNode.CStr()), const_cast<char*>("blood"), 0, 0));
        if (pGob == 0)
            return 0;
        if (sImpact == "")
            sImpact = "impact";
        pGob->AttachToObject(static_cast<CAurObject*>(GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)), static_cast<char*>(sImpact.CStr()), 3);
        fDot = dot(normalize(v), m_orientation);
        if (fDot > 1.0f)
            fDot = 1.0f;
        fAngle = acosf(fDot);
        switch (pData->m_nWeaponAttackType) {
        case CSWSCombatRound::WEAPON_ATTACK_TYPE_ONHAND:
        case CSWSCombatRound::WEAPON_ATTACK_TYPE_EXTRA:
            nItemId = m_aEquippedItemIds[4];
            break;
        case CSWSCombatRound::WEAPON_ATTACK_TYPE_OFFHAND:
            nItemId = m_aEquippedItemIds[5];
            break;
        case CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON1:
            nItemId = m_aEquippedItemIds[11];
            break;
        case CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON2:
            nItemId = m_aEquippedItemIds[12];
            break;
        case CSWSCombatRound::WEAPON_ATTACK_TYPE_CWEAPON3:
            nItemId = m_aEquippedItemIds[13];
            break;
        default:
            nItemId = OBJECT_INVALID;
            break;
        }
        pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(nItemId);
        if (pItem != 0)
            g_pRules->GetBaseItem(pItem->m_baseItem);
        if (cross(v, normalize(m_orientation)).z >= 0.0f)
            pGob->SetOrientation(Quaternion(Vector{0.0f, 0.0f, 1.0f}, -fAngle) *
                                 Quaternion(Vector{1.0f, 0.0f, 0.0f}, 1.0f));
        else
            pGob->SetOrientation(Quaternion(Vector{0.0f, 0.0f, 1.0f}, fAngle) *
                                 Quaternion(Vector{1.0f, 0.0f, 0.0f}, 1.0f));
        pGob->PlayAnimation(const_cast<char*>("impact"), 1.0f, 1, 0.0f);
        m_aHitVisualGobs[i] = pGob;
        break;
    }
    return 1;
}

// Builds the radial-menu usable-item list: the three equipped slots first, then every item in
// the server inventory.
void CSWCCreature::GetUsableItems(CExoArrayList<CSWGuiInterfaceAction>& list, unsigned char nType) {
    CSWSCreature* pServerCreature = 0;
    if (GetServerObject())
        pServerCreature = GetServerObject()->AsSWSCreature();
    if (pServerCreature != 0) {
        if (static_cast<CSWSArea*>(pServerCreature->GetArea())->m_nPlayerRestrictMode == 0) {
            CAppManager** ppMgr = &g_pAppManager;
            CSWCItem* pItem =
                (*ppMgr)->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[7]);
            if (pItem != 0)
                CreateUsableItemEntry(list, pItem->GetServerItem(), 1, nType);
            pItem = (*ppMgr)->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[8]);
            if (pItem != 0)
                CreateUsableItemEntry(list, pItem->GetServerItem(), 1, nType);
            pItem = (*ppMgr)->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[10]);
            if (pItem != 0)
                CreateUsableItemEntry(list, pItem->GetServerItem(), 1, nType);
        }
        CItemRepository* pRepo = pServerCreature->GetItemRepository(1);
        if (pRepo->ItemListGetCount() != 0) {
            for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++)
                CreateUsableItemEntry(list, pRepo->ItemListGetItem(i), 0, nType);
        }
    }
}

void CSWCCreature::SetName(const CExoString& first, const CExoString& last) {
    CExoString sFull;
    CSWCCreatureStats* pStats = m_pStats;
    pStats->m_name = first;
    pStats->m_sLastName = last;
    sFull = "[";
#if KOTOR1  // K1 uses the full name
    sFull = sFull + m_pStats->GetFullName() + "]";
#else
    sFull = sFull + m_pStats->m_name + "]";
#endif
    SetPopUpBubbleDefaultText(sFull);
    SetPopUpBubbleDefaultCreatureID(m_objectId);
}

// Attacks from the action menu. The attack-mash tutorials get first refusal, then the target's
// combat-mode bit is armed, party-wide when its hostility flag allows, the mash timer re-arms and
// the attack input is queued.
void CSWCCreature::ActionMenuAttack(unsigned int n, CSWCCreature* pTarget) {
    K2_UNUSED(n);
    if (g_pAppManager->m_pClientExoApp->GetAttackMashTutorialTimer() != 0 &&
        g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_ATTACK_BUTTON_MASH, pTarget->m_objectId,
                                                           m_objectId, 0))
        return;
    if (g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_ATTACK, pTarget->m_objectId,
                                                           m_objectId, 0))
        return;
    pTarget->m_nStateFlags |= 1;
    if (pTarget->m_objectId == g_pAppManager->m_pClientExoApp->GetPlayerCreatureId())
        g_pAppManager->m_pClientExoApp->SetCombatMode(pTarget->m_nStateFlags & 1);
#if !KOTOR1  // K1 arms only the target
    if (pTarget->m_bInParty != 0) {
        int nCount = g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount;
        if (nCount >= 2) {
            int i = 1;
            do {
                CSWCCreature* pMember =
                    g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(i);
                pMember->m_nStateFlags |= 1;
                if (pMember->m_objectId ==
                    g_pAppManager->m_pClientExoApp->GetPlayerCreatureId())
                    g_pAppManager->m_pClientExoApp->SetCombatMode(pMember->m_nStateFlags & 1);
                i++;
            } while (i != nCount);
        }
    }
#endif
    g_pAppManager->m_pClientExoApp->SetAttackMashTutorialTimer(3000);
    g_pAppManager->m_pClientExoApp->GetSWCMessage()->SendPlayerToServerInput_Attack(
        m_objectId, 0);
}

// Sets the combat state. On a change this re-powers both equipped weapons while the server
// creature is alive and able, then locks or clears the look-at suppression.
void CSWCCreature::SetCombatState(int bCombat) {
    int nOldState = m_bInCombat;
    m_bInCombat = bCombat;
    if (bCombat != 0)
        GetAnimationBase();
    if (nOldState != bCombat) {
        CSWSCreature* pServerCreature = 0;
        if (GetServerObject())
            pServerCreature = GetServerObject()->AsSWSCreature();
        if (pServerCreature != 0 && pServerCreature->GetDead() == 0 &&
            pServerCreature->GetDeadTemp() == 0) {
            if (m_aEquippedItemIds[4] != OBJECT_INVALID) {
                CGameObject* pObj =
                    g_pAppManager->m_pClientExoApp->GetGameObject(m_aEquippedItemIds[4]);
                if (pObj != 0 && pObj->AsSWCItem() != 0)
                    static_cast<CSWCItem*>(pObj->AsSWCItem())->PowerItem(bCombat, 0, 1, 0);
            }
            if (m_aEquippedItemIds[5] != OBJECT_INVALID) {
                CGameObject* pObj =
                    g_pAppManager->m_pClientExoApp->GetGameObject(m_aEquippedItemIds[5]);
                if (pObj != 0 && pObj->AsSWCItem() != 0)
                    static_cast<CSWCItem*>(pObj->AsSWCItem())->PowerItem(bCombat, 0, 1, 0);
            }
        }
    }
    if (bCombat != 0) {
        LookAt(OBJECT_INVALID, 0.0f);
        m_bDisableLookAt = 1;
    } else {
        m_bDisableLookAt = 0;
    }
}

// Probes for a usable item. For ourselves: the three equipped slots, then the inventory. For
// another creature: inventory items pass only while we are hostile and IsHostileUsableItem
// agrees.
int CSWCCreature::HasUsableItems(CSWCCreature* pOther) {
    CSWSCreature* pServerCreature = 0;
    if (GetServerObject())
        pServerCreature = GetServerObject()->AsSWSCreature();
    if (pServerCreature == 0)
        return 0;
    int bMenuDisabled = static_cast<CSWSArea*>(pServerCreature->GetArea())->m_nPlayerRestrictMode;
    CSWSCreature* pOtherServer = 0;
    if (pOther->GetServerObject())
        pOtherServer = pOther->GetServerObject()->AsSWSCreature();
    CItemRepository* pRepo = pOtherServer->GetItemRepository(1);
    if (pOther == this && bMenuDisabled == 0) {
        CSWCItem* pItem =
            g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[7]);
        if (pItem != 0 && IsLeaderUsableItem(pItem->GetServerItem(), 0, 0xff))
            return 1;
        pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[8]);
        if (pItem != 0 && IsLeaderUsableItem(pItem->GetServerItem(), 0, 0xff))
            return 1;
        pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[10]);
        if (pItem != 0 && IsLeaderUsableItem(pItem->GetServerItem(), 0, 0xff))
            return 1;
    }
    if (pRepo->ItemListGetCount() != 0) {
        for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++) {
            CSWSItem* pItem = pRepo->ItemListGetItem(i);
            if (pOther == this) {
                if (IsLeaderUsableItem(pItem, 0, 0xff))
                    return 1;
            } else {
                if (IsHostile() != 0 && IsHostileUsableItem(pItem) != 0)
                    return 1;
            }
        }
    }
    return 0;
}

// Spawns the foot-fall visual for a surface at the creature's position, nudged along its
// orientation and snapped to the ground with a scene hit check. The snap is skipped unless the
// creature's height changed since the last one. An invalid type leaks the effect.
void CSWCCreature::DoFootstepVisual(unsigned char nType, int bForce) {
    CSWCVisualEffect* pEffect = new CSWCVisualEffect(1);
    CExoString sEffect;
    switch (nType) {
    case 0:
        sEffect = "fx_step_splash";
        break;
    case 1:
        sEffect = "fx_step_dust";
        break;
    case 2:
        sEffect = "fx_step_leaf";
        break;
    default:
        return;
    }
    if (pEffect->LoadModel(CResRef(sEffect), CSWCAnimBase::ANIM_MODEL_BODY, 8) == 0)
        return;
    if (m_pArea == 0)
        return;
    Vector vPos = {0.0f, 0.0f, 0.0f};
    Gob* pModel;
    switch (m_currentAnimation) {
    case SERVER_ANIMATION_WALK:
    case SERVER_ANIMATION_WALK_BACKWARD:
    case SERVER_ANIMATION_RUN:
    case SERVER_ANIMATION_STRAFE_LEFT:
    case SERVER_ANIMATION_STRAFE_RIGHT:
    case SERVER_ANIMATION_WALK_STRAFE_LEFT:
    case SERVER_ANIMATION_WALK_STRAFE_RIGHT:
    case SERVER_ANIMATION_RUN_STRAFE_LEFT:
    case SERVER_ANIMATION_RUN_STRAFE_RIGHT: {
        pModel = GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
        if (pModel != 0) {
            if (nType == 0) {
                if (m_currentAnimation == SERVER_ANIMATION_WALK_BACKWARD)
                    vPos = m_position - m_orientation * 0.5f;
                else
                    vPos = m_orientation * 0.5f + m_position;
            } else {
                if (m_currentAnimation == SERVER_ANIMATION_WALK_BACKWARD)
                    vPos = m_position - m_orientation * 0.1f;
                else
                    vPos = m_orientation * 0.1f + m_position;
            }
            if ((bForce != 0 || __builtin_fabsf(m_position.z - m_fFootstepGroundZ) > 1.0f) &&
                m_pArea->m_pScene != 0) {
                Vector vStart = m_position;
                vStart.z += 1.0f;
                CHitInfo hitInfo(0, CHitInfo::kMaterialAllButTrigger, 0);
                Vector vHit = {0.0f, 0.0f, 0.0f};
                int nOldTrimeshParts;
                int nHit;
                hitInfo.m_bSkipObjects = 1;
                g_pAppManager->m_pClientExoApp->AddObjectModelDirectlyToIgnoreList(
                    m_objectId, &hitInfo.m_ignoreList);
                nOldTrimeshParts = hitcheck_static_trimesh_parts;
                hitcheck_static_trimesh_parts = 1;
                nHit = m_pArea->m_pScene->HitCheck(
                    vStart, m_position,
                    &vHit, &hitInfo);
                if (nHit != 0)
                    m_fFootstepGroundZ = vHit.z + 0.01f;
                else
                    m_fFootstepGroundZ = m_position.z + 0.35f;
                hitcheck_static_trimesh_parts = nOldTrimeshParts;
            }
            if (nType == 0) {
                Vector vUp = {0.0f, 0.0f, 1.0f};
                Vector vDir = cross(vUp, m_orientation);
                if (vDir.x * vDir.x + vDir.y * vDir.y + vDir.z * vDir.z > 1e-05f) {
                    vDir = normalize(vDir);
                    if ((rand_wincompatible() & 1) != 0) {
                        float fScatter = static_cast<float>(rand_wincompatible() % 20) / 100.0f;
                        vPos = vDir * fScatter + vPos;
                    } else {
                        float fScatter = static_cast<float>(rand_wincompatible() % 20) / 100.0f;
                        vPos = vPos - vDir * fScatter;
                    }
                }
                vPos.z = m_fFootstepGroundZ;
            }
        }
        break;
    }
    default:
        vPos = m_position;
        break;
    }
    pEffect->AttachToScene(m_pArea->m_pScene);
    pEffect->m_bImpactOnly = 1;
    pEffect->m_bDeleteWhenFinished = 1;
    pEffect->SetPosition(vPos);
    pEffect->SetOrientation(m_orientation);
    pEffect->SetAnimation(1);
}

// Plays player-only footstep foley, a 5% chance per call of a random creak variant.
void CSWCCreature::DoFootstepAudio(unsigned char flag) {
    if (flag == 0 && g_pAppManager->m_pClientExoApp->GetPlayerCreatureId() == m_objectId &&
        rand_wincompatible() % 100 <= 4) {
        CExoString sSound;
        sSound = "as_cv_florcreak";
        sSound = sSound + CExoString(rand_wincompatible() % 3 + 1);
        g_pExoSound->Play3DOneShotSound(CResRef(sSound), m_position, 0.0f, PRIORITY_GROUP_PLAYER_FOOTSTEPS, 0, 0, 0.0f,
                                        0.0f);
    }
}

// Recomputes the weapon overlays. It bails on an active overlay animation, the busy flag, an empty
// model or combat state 1, and otherwise only does a pair of item lookups whose results it
// discards.
int CSWCCreature::ComputeWeaponOverlays(int a, int b) {
    K2_UNUSED(b);
    if (GetAnimationBase() == 0)
        return 0;
    if (a != 1)
        return 1;
    if (GetAnimationBase()->GetOverlayAnimationPlaying(SERVER_ANIMATION_LISTEN) == 1)
        return 1;
    if (GetAnimationBase()->GetOverlayAnimationPlaying(SERVER_ANIMATION_LISTEN_INJURED) == 1)
        return 1;
    if (GetAnimationBase()->GetOverlayAnimationPlaying(SERVER_ANIMATION_TALK_NORMAL) == 1)
        return 1;
    if (GetAnimationBase()->GetOverlayAnimationPlaying(SERVER_ANIMATION_TALK_INJURED) == 1)
        return 1;
    if (GetAnimationBase()->GetOverlayAnimationPlaying(SERVER_ANIMATION_TALK_PLEADING) == 1)
        return 1;
    if (GetAnimationBase()->GetOverlayAnimationPlaying(SERVER_ANIMATION_TALK_FORCEFUL) == 1)
        return 1;
    if (GetAnimationBase()->GetOverlayAnimationPlaying(SERVER_ANIMATION_TALK_LAUGHING) == 1)
        return 1;
    if (GetAnimationBase()->GetOverlayAnimationPlaying(SERVER_ANIMATION_TALK_SAD) == 1)
        return 1;
    if (m_bAmbientPausePlaying != 0)
        return 1;
    if (m_nAIStateFlags == 0)
        return 1;
    g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[5]);
    if (m_bInCombat == 1)
        return 1;
    GetAnimationBase();
    g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[4]);
    return 1;
}

// Computes the walk-speed factor over the remaining way-points: the ground-projected path length,
// curved by a square root above 0.01, blended against the frame offset and clamped to the
// walk-speed ceiling and the 0.1 floor. 0 when there is no current way-point.
float CSWCCreature::ComputeSpeedFactor(float f) {
    if (m_pCurrentWayPoint == 0)
        return 0.0f;
    float fTotal = 0.0f;
    Vector vPrev = m_desiredPosition;
    Vector* p = static_cast<Vector*>(m_pCurrentWayPoint);
    Vector* pEnd = m_pWayPoints + m_wayPointCount;
    do {
        Vector vDelta;
        vDelta.x = p->x - vPrev.x;
        vDelta.y = p->y - vPrev.y;
        vDelta.z = 0.0f;
        fTotal += magnitude(vDelta);
        vPrev = *p;
        p++;
    } while (p < pEnd);
    float fSpeed = (m_bServerWayPoints == 1) ? 500.0f : 1000.0f;
    float fNorm = f * 1000.0f / 2000000.0f;
    float fResult = 1.0f;
    if (fTotal <= fNorm) {
        float fRatio = fTotal / fNorm;
        float fCurve = fRatio >= 0.01f ? sqrtf(fRatio) : 0.1f;
        fResult = fCurve + static_cast<float>(m_nAIUpdateDeltaMs) / -1000.0f;
    }
    if (m_fMoveSpeedScale <= 1.0f) {
        float fWalk = m_fMoveSpeedScale + static_cast<float>(m_nAIUpdateDeltaMs) / fSpeed;
        if (fResult >= fWalk)
            fResult = fWalk;
    }
    // A float max that sends NaN to the floor, as a floating-point minimum would.
    return fResult < 0.1f ? 0.1f : fResult;
}

// Projects a point above the creature's head into camera space and scales the screen point by
// the GUI manager's factors. Returns 2 when the point lies within the camera's plane depth and
// 1 otherwise.
int CSWCCreature::GetMenuPoint(Vector& vOut) {
    CAurCamera* pCamera = g_pAppManager->m_pClientExoApp->GetModuleCamera();
    Vector vDir = -pCamera->GetOrientation().zdir();
    vOut = m_position;
    CSWSCreature* pServerCreature = 0;
    if (GetServerObject())
        pServerCreature = GetServerObject()->AsSWSCreature();
    if (pServerCreature != 0 &&
        (pServerCreature->m_sTag == "zaalbar" || pServerCreature->m_sTag == "hk47"))
        vOut.z += 2.0f;
    else
        vOut.z += 1.75f;
    float fPoint = dot(vOut, vDir);
    Vector vCamPos = pCamera->GetPosition();
    float fCam = dot(vCamPos, vDir);
    vOut = pCamera->MapToScreen(vOut, true);
    CAppManager** ppMgr = &g_pAppManager;
    vOut.x *= (*ppMgr)->m_pClientExoApp->GetSWGuiManager()->m_nScreenWidth;
    vOut.y *= (*ppMgr)->m_pClientExoApp->GetSWGuiManager()->m_nScreenHeight;
    return !(fPoint > fCam) ? 2 : 1;
}

// Projects the head bone into camera space, falling back to the whole model and then the root
// dummy, and scales by the GUI manager's factors. 0 when no usable part exists.
char CSWCCreature::GetReticlePoint(Vector& v) {
    Gob* pGob = m_pModel->GetModel(CSWCAnimBase::ANIM_MODEL_BODY);
    if (pGob == 0)
        return 0;
    Part* pPart = pGob->GetPart(const_cast<char*>("head_g"));
    if (pPart == 0) {
        Gob* pGob2 = m_pModel->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD);
        if (pGob2 != 0)
            pPart = pGob2->GetPart(const_cast<char*>("head_g"));
        if (pPart == 0) {
            pPart = pGob->GetPart(const_cast<char*>("rootdummy"));
            if (pPart == 0)
                return 0;
        }
    }
    Quaternion qIdentity;  // Identity quaternion.
    pPart->GetWorld(v, qIdentity);
    CAurCamera* pCamera = g_pAppManager->m_pClientExoApp->GetModuleCamera();
    Vector vDir = -pCamera->GetOrientation().zdir();
    float fPoint = dot(v, vDir);
    Vector vCamPos = pCamera->GetPosition();
    float fCam = dot(vDir, vCamPos);
    char nRet = !(fPoint > fCam) ? 2 : 1;
    v = pCamera->MapToScreen(v, true);
    short nWidth = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenWidth;
    v.x = v.x * nWidth;
    short nHeight = g_pAppManager->m_pClientExoApp->GetSWGuiManager()->m_nScreenHeight;
    v.y = v.y * nHeight;
    return nRet;
}

// Drives the animation each frame. It re-locks the orientation if locked, rotates, moves, falls
// back to the looping animation when stationary, drives fire-and-forget animations and applies the
// idle and injured corrections.
void CSWCCreature::AnimationUpdate() {
    if (AnimationStationary(m_currentAnimation) == 0) {
        CSWSCreature* pServerCreature = 0;
        if (GetServerObject())
            pServerCreature = GetServerObject()->AsSWSCreature();
        if (pServerCreature != 0) {
            GetServerObject();
            if (GetServerObject()->AsSWSCreature()->m_nMoveState == 0)
                return;
        }
    }
    if (m_lockOrientationObjectId != CGameObjectArray::INVALIDOBJID)
        SetDesiredOrientationForLock();
    RotateCreatureToOrientation(AnimationStationary(m_currentAnimation) == 0 ? m_vMovingOrientation
                                                                             : m_vStationaryOrientation);
    MoveCreatureToPosition(m_desiredPosition);
    if (AnimationStationary(m_currentAnimation) == 0)
        SetAnimation(GetLoopingAnimation(), 0, 0);
    AnimateFireAndForget();
    unsigned short nAnim = m_currentAnimation;
    if (nAnim == SERVER_ANIMATION_READY &&
        (m_nAIStateAction > CSWSCreature::AI_STATE_ACTION_CASTING ||
        ((1 << m_nAIStateAction) & AI_STATE_ACTIONS_READY_STANCE) == 0) && m_bInCombat != 1) {
        SetAnimation(SERVER_ANIMATION_PAUSE, 0, 0);
        nAnim = m_currentAnimation;
    }
    if (AnimationPause(nAnim) != 0 &&
        m_nAIStateAction <= CSWSCreature::AI_STATE_ACTION_CASTING && ((1 << m_nAIStateAction) & AI_STATE_ACTIONS_READY_STANCE) != 0)
        SetAnimation(SERVER_ANIMATION_READY, 0, 0);
    if (AnimationPause(m_currentAnimation) != 0 && m_currentAnimation != SERVER_ANIMATION_CHOKE &&
        g_pAppManager->m_pServerExoApp->IsInjured(m_objectId) != 0) {
        if (!(g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK &&
              g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0) == this))
            SetAnimation(SERVER_ANIMATION_PAUSE_INJURED, 0, 0);
    }
}

// Probes whether this creature can stealth at all: the area must allow it, the stealth skill
// must be usable, and either a stealth belt is equipped, the feat is held, or one of the three
// stealth powers is known.
int CSWCCreature::IsStealthCapable() {
    CSWSCreature* pServerCreature = 0;
    if (GetServerObject())
        pServerCreature = GetServerObject()->AsSWSCreature();
    if (pServerCreature == 0)
        return 0;
    CSWSCreature* pServerCreature2 = 0;
    if (GetServerObject())
        pServerCreature2 = GetServerObject()->AsSWSCreature();
    if (static_cast<CSWSArea*>(pServerCreature2->GetArea())->m_nPlayerRestrictMode != 0)
        return 0;
    if (m_pStats->GetCanUseSkill(SKILL_STEALTH) == 0)
        return 0;
    int bHasBelt = 0;
    CSWCItem* pItem =
        g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[10]);
    if (pItem != 0 && pItem->GetBaseItem() != 0 &&
        pItem->GetBaseItem()->m_nItemType == ITEM_TYPE_STEALTH_UNIT)
        bHasBelt = 1;
    pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[14]);
    if (pItem != 0 && pItem->GetBaseItem() != 0 &&
        pItem->GetBaseItem()->m_nItemType == ITEM_TYPE_STEALTH_UNIT)
        bHasBelt |= 1;
#if KOTOR1
    return bHasBelt;  // K1: the stealth belt is the only source
#else
    GetServerObject();
    CSWSCreatureStats* pStats = GetServerObject()->AsSWSCreature()->m_pStats;
    int bHasFeat = pStats->HasFeat(FEAT_PERSONAL_CLOAKING_SHIELD) != 0;
    GetServerObject();
    pStats = GetServerObject()->AsSWSCreature()->m_pStats;
    if (pStats->HasSpell(0, FORCE_POWER_FORCE_CAMOUFLAGE, 0) != 0)
        return 1;
    GetServerObject();
    pStats = GetServerObject()->AsSWSCreature()->m_pStats;
    if (pStats->HasSpell(0, FORCE_POWER_IMPROVED_FORCE_CAMOUFLAGE, 0) != 0)
        return 1;
    GetServerObject();
    pStats = GetServerObject()->AsSWSCreature()->m_pStats;
    return (bHasBelt | bHasFeat | (pStats->HasSpell(0, FORCE_POWER_MASTER_FORCE_CAMOUFLAGE, 0) != 0));
#endif
}

// Picks a random scout point near the creature: a random distance along one of the four
// cardinal directions of its orientation.
Vector CSWCCreature::GetRandomPoint(float f) {
    if (f <= 0.0f)
        f = static_cast<float>(rand_wincompatible() % 5);
    int nCase = rand_wincompatible() % 4;
    Vector vOrient = m_orientation;
    switch (nCase) {
    case 0:
        return m_position + normalize(Vector(-vOrient.y, vOrient.x, 0.0f)) * f;
    case 1:
        return m_position + normalize(Vector(vOrient.y, -vOrient.x, 0.0f)) * f;
    case 2:
        return vOrient * f + m_position;
    case 3:
        return m_position - vOrient * f;
    }
    return Vector(0.0f, 0.0f, 0.0f);
}

// Refreshes the shadow blob for the shadow detail. Detail 2 removes the blob, detail 1 removes and
// re-applies it for every creature but the player, and any other detail removes and re-applies it.
void CSWCCreature::UpdateShadowBlob() {
    unsigned char nDetail = g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nShadowDetail;
    switch (nDetail) {
    case 2:
        RemoveShadowBlob();
        break;
    case 1: {
        unsigned long long nObjectId = m_objectId;
        unsigned long long nPlayerId = g_pAppManager->m_pClientExoApp->GetPlayerCreatureId();
        RemoveShadowBlob();
        if (nObjectId == nPlayerId) {
            if (g_pAppManager->m_pClientExoApp->GetModule() != 0 &&
                g_pAppManager->m_pClientExoApp->GetModule()->m_pArea != 0) {
                g_pAppManager->m_pClientExoApp->GetModule();
            }
        } else {
            ApplyShadowBlob();
        }
        break;
    }
    default:
        RemoveShadowBlob();
        ApplyShadowBlob();
        break;
    }
    if (m_pModel != 0) {
        m_pModel->SetShadowBlobOrientation(RotationArc(Vector{0.0f, 0.0f, 1.0f}, m_groundNormal));
    }
}

// Builds the shadow blob for our appearance size, orients it to the shadow normal and hides it
// while the display flag is zero. Returns the model's ApplyShadowBlob result.
int CSWCCreature::ApplyShadowBlob() {
    CExoString sModel;
    int nResult = 0;
    if (m_pModel != 0) {
        int nSize;
        if (!g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
                m_pCreatureAppearance->m_nAppearanceId,
                CTwoDimArrays::COL2DA_APPEARANCETABLE_SIZECATEGORY, &nSize))
            nSize = CREATURE_SIZE_MEDIUM;
        switch (nSize) {
        case CREATURE_SIZE_TINY:
        case CREATURE_SIZE_SMALL:
            sModel = "Fx_Shadow_S";
            break;
        case CREATURE_SIZE_LARGE:
        case CREATURE_SIZE_HUGE:
            sModel = "Fx_Shadow_L";
            break;
        case CREATURE_SIZE_MEDIUM:
            sModel = "Fx_Shadow_M";
            break;
        default:
            sModel = "Fx_Shadow_M";
            break;
        }
        nResult = m_pModel->ApplyShadowBlob(sModel);
        m_pModel->SetShadowBlobOrientation(RotationArc(Vector{0.0f, 0.0f, 1.0f}, m_groundNormal));
        if (m_bVisible == 0)
            HideShadowBlob();
    }
    return nResult;
}

// Sets the "no attack" defaults. Both object ids are OBJECT_INVALID, the reaction field is -1 and
// everything else is zero.
CSWCCombatAttackData::CSWCCombatAttackData()
{
    m_nDataType = ATTACK_DATA_NONE;
    m_nCastTargetType = 0;
    m_attackerObjectId = OBJECT_INVALID;  // OBJECT_INVALID
    m_weaponObjectId = OBJECT_INVALID;  // OBJECT_INVALID
    m_nAnimationLength = 0;
    m_nTotalDamage = 0;
    m_nAttackResult = 0;
    m_nReserved1 = 0;
    m_castTargetGroup.m_fCastTargetX = 0;
    m_castTargetGroup.m_fCastTargetY = 0;
    m_fCastTargetZ = 0;
    m_reaxnGroup.m_nReaxnDelay = 0;
    m_reaxnGroup.m_nReaxnAnimation = 0;
    m_reaxnGroup.m_nReaxnAnimLength = 0;
    m_reaxnGroup.m_nFeatId = 0;
    m_spellGroup.m_nSpellId = 0xffffffff;
    m_rangedAttackGroup.m_bRangedAttack = 0;
    m_killingBlowGroup.m_bKillingBlow = 0;
    m_nWeaponAttackType = 0;
    m_spellGroup.m_nCastLevel = 0;
    m_pNext = 0;
    m_fAnimationSpeed = 0;
}

CSWCCombatAttackData::~CSWCCombatAttackData()
{
}

// The same reset as the constructor, plus releasing the chained next node.
// EFFECTIVE: the zero and 0xffffffff stores merge at different widths (a q store here, d and x in the orig).
void CSWCCombatAttackData::ClearAttackData()
{
    m_nDataType = ATTACK_DATA_NONE;
    m_nCastTargetType = 0;
    m_attackerObjectId = OBJECT_INVALID;  // OBJECT_INVALID
    m_weaponObjectId = OBJECT_INVALID;  // OBJECT_INVALID
    m_nAnimationLength = 0;
    m_nTotalDamage = 0;
    m_nAttackResult = 0;
    m_nReserved1 = 0;
    m_castTargetGroup.m_fCastTargetX = 0;
    m_castTargetGroup.m_fCastTargetY = 0;
    m_fCastTargetZ = 0;
    m_reaxnGroup.m_nReaxnDelay = 0;
    m_reaxnGroup.m_nReaxnAnimation = 0;
    m_reaxnGroup.m_nReaxnAnimLength = 0;
    m_reaxnGroup.m_nFeatId = 0;
    m_spellGroup.m_nSpellId = 0xffffffff;
    m_rangedAttackGroup.m_bRangedAttack = 0;
    m_killingBlowGroup.m_bKillingBlow = 0;
    m_nWeaponAttackType = 0;
    m_spellGroup.m_nCastLevel = 0;
    m_fAnimationSpeed = 0;
    if (m_pNext != 0) {
        delete m_pNext;
        m_pNext = 0;
    }
}

void CSWCCombatAttackData::Copy(CSWCCombatAttackData* pSource)
{
    *this = *pSource;
}

// Resolves one attack's animation pair: the weapon's attack-type byte drives the ranged, melee
// or other split, then the reaction animation and the special-attack visuals.
int CSWCCreature::UpdateAttackData(unsigned short* pOutAnim, CSWCCombatAttackData* pData) {
    CGameObject* pWeaponObj =
        g_pAppManager->m_pClientExoApp->GetGameObject(pData->m_weaponObjectId);
// The no-weapon path returns the null lookup's own result, a failure rather than a success.
    if (pWeaponObj == 0) {
        *pOutAnim = 0xffff;
        pData->m_reaxnGroup.m_nReaxnAnimation = 0xffff;
        return 0;
    }
    unsigned char nWeaponType = GetAnimationBase()->m_nWeaponClass;
    CSWCObject* pWeaponObj2 = static_cast<CSWCObject*>(pWeaponObj->AsSWCObject());
    int nAnim;
    if (m_nForcedAttackAnimation != 0) {
        nAnim = m_nForcedAttackAnimation;
        m_nForcedAttackAnimation = 0;
    } else {
        nAnim = g_nCombatAnimation;
        if (nAnim == 0 || m_objectId != 0xffffffff) {
            if (pData->m_rangedAttackGroup.m_bRangedAttack != 0) {
                nAnim = UpdateRangedAttackData(pData->m_reaxnGroup.m_nFeatId, nWeaponType);
            } else {
                int bRanged;
                if (pWeaponObj2 == 0) {
                    bRanged = 0;
                } else if (IsCreature() != 0) {
                    bRanged = 1;
                } else if (pWeaponObj2->AsSWCCreature() != 0 &&
                           static_cast<CSWCCreature*>(pWeaponObj2->AsSWCCreature())->IsCreature() != 0) {
                    bRanged = 1;
                } else {
                    bRanged = 0;
                }
                nAnim = UpdateMeleeAttackData(*pOutAnim, pData->m_reaxnGroup.m_nFeatId, bRanged, nWeaponType);
            }
        }
    }
    unsigned char nWeaponType2 = pWeaponObj2->GetAnimationBase()->m_nWeaponClass;
    if (pWeaponObj2 != 0 && pWeaponObj2->AsSWCCreature() != 0 &&
        static_cast<CSWCCreature*>(pWeaponObj2->AsSWCCreature())->IsCreature() != 0)
        nWeaponType2 = 0;
    unsigned short nReaction =
        ResolveReactionAnimation(static_cast<unsigned short>(nAnim), pData->m_reaxnGroup.m_nReaxnAnimation, nWeaponType2);
    ResolveSpecialAttackVisuals(pData->m_reaxnGroup.m_nFeatId, nWeaponType, pData->m_rangedAttackGroup.m_bRangedAttack);
    *pOutAnim = static_cast<unsigned short>(nAnim);
    pData->m_reaxnGroup.m_nReaxnAnimation = nReaction;
    return 1;
}
// Resolves the reaction animation for an incoming attack: the attack animation adds the
// reaction column to the row in CombatAnimations.2da.

unsigned short CSWCCreature::ResolveReactionAnimation(unsigned short a, unsigned short b, int c) {
#if !KOTOR1  // K1 has no feat-selected reaction column
    if (c == 8) {
        GetServerObject();
        c = GetServerObject()->AsSWSCreature()->m_pStats->HasFeat(CSWSCreatureStats::FEAT_COMPLEX_UNARMED_ANIMS) ? 10 : 8;
    }
#endif
    CExoString sRow(a);
    CExoString sCol(c);
    int val = 0;
    switch (b) {
    case SERVER_ANIMATION_READY:
        val = SERVER_ANIMATION_READY;
        break;
    case SERVER_ANIMATION_DODGE:
        sCol = CExoString("Dodge") + sCol;
        break;
    case SERVER_ANIMATION_PARRY:
        sCol = CExoString("Parry") + sCol;
        break;
    case SERVER_ANIMATION_DAMAGE:
        sCol = CExoString("Damage") + sCol;
        break;
    }
    if (b != SERVER_ANIMATION_READY)
        g_pRules->m_p2DArrays->m_pCombatAnimations->GetINTEntry(sRow, sCol, &val);
    return static_cast<unsigned short>(val);
}

#if KOTOR1
// K1: after a body or head rebuild the current animation is replayed, or the looping one when
// the current is a stationary one-shot, and the alignment tint is reapplied.
int CSWCCreature::SetAppearance(CCAppearanceInfo* pInfo, unsigned char flags, int b) {
    DetachVisualEffects();
    unsigned short nLooping = 0;
    unsigned short nCurrent = 0;
    if (flags & 3) {
        nLooping = GetLoopingAnimation();
        nCurrent = GetCurrentAnimation();
    }
    if (flags & 1) {
        Vector vPos = m_position;
        Vector vOrient = m_orientation;
        Vector vDesired = m_desiredPosition;
        Vector vMovingOrientation = m_vMovingOrientation;
        Vector vStationaryOrientation = m_vStationaryOrientation;
        if (m_pCreatureAppearance->CreateBodyAppearance(pInfo) == 0)
            return 0;
        SetPosition(vPos);
        m_desiredPosition = vDesired;
        SetOrientation(vOrient);
        m_vMovingOrientation = normalize(vMovingOrientation);
        SetDesiredStationaryOrientation(vStationaryOrientation);
    }
    if (flags & 2)
        m_pCreatureAppearance->CreateHeadAppearance(pInfo);
    if (flags & 4)
        m_pCreatureAppearance->CreateTailAppearance(pInfo);
    if (flags & 8)
        m_pCreatureAppearance->CreateWingAppearance(pInfo);
    if (flags & 3) {
        SetLoopingAnimation(nLooping);
        AbortFireForgetAnimation(0);
        if (AnimationLooping(nCurrent) == 1 || AnimationStationary(nCurrent) == 0)
            SetAnimation(nCurrent, 0, 0);
        else
            SetAnimation(nLooping, 0, 0);
    }
    UpdateShadowBlob();
    if (b != 0)
        ReAttachVisualEffects();
    LoadRollingFootstepSound();
    m_pCreatureAppearance->SetAppearanceAlignment(m_pStats->m_alignmentGoodEvil);
    m_pCreatureAppearance->ForceAnimate();
    return 1;
}
#else
// Rebuilds the appearance by flag bits: bit 0 body, 1 head, 2 tail, 3 wings. Bits 0 and 1 also
// snapshot and restore the looping animation; the other flags re-attach the visual effects and
// skip the alignment refresh.
int CSWCCreature::SetAppearance(CCAppearanceInfo* pInfo, unsigned char flags, int b, int c) {
    DetachVisualEffects();
    unsigned short nAnim = 0;
    if (flags & 3) {
        nAnim = GetLoopingAnimation();
        GetCurrentAnimation();  // Result discarded.
    }
    if (flags & 1) {
        Vector vPos = m_position;
        Vector vOrient = m_orientation;
        Vector vDesired = m_desiredPosition;
        Vector vMovingOrientation = m_vMovingOrientation;
        Vector vStationaryOrientation = m_vStationaryOrientation;
        if (m_pCreatureAppearance->CreateBodyAppearance(pInfo) == 0)
            return 0;
        SetPosition(vPos);
        m_desiredPosition = vDesired;
        SetOrientation(vOrient);
        SetDesiredWalkOrientation(vMovingOrientation);
        SetDesiredStationaryOrientation(vStationaryOrientation);
    }
    if (flags & 2)
        m_pCreatureAppearance->CreateHeadAppearance(pInfo);
    if (flags & 4)
        m_pCreatureAppearance->CreateTailAppearance(pInfo);
    if (flags & 8)
        m_pCreatureAppearance->CreateWingAppearance(pInfo);
    if (flags & 3) {
        SetLoopingAnimation(nAnim);
        AbortFireForgetAnimation(0);
    }
    UpdateShadowBlob();
    if (b != 0)
        ReAttachVisualEffects();
    LoadRollingFootstepSound();
    if (c == 0)
        m_pCreatureAppearance->SetAppearanceAlignment(m_pStats->m_alignmentGoodEvil);
    m_pCreatureAppearance->ForceAnimate();
    return 1;
}
#endif

// Loads the rolling-footstep loop from footstepsounds.2da, picking the resref and its pitch
// variance from the appearance's footstep-type row. Returns whether a source is live.
int CSWCCreature::LoadRollingFootstepSound() {
    CExoString sSound;
    if (m_pRollingFootstepSound != 0) {
        delete m_pRollingFootstepSound;
        m_pRollingFootstepSound = 0;
    }
    int nFootstepType;
    if (g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
            m_pCreatureAppearance->m_nAppearanceId,
            CTwoDimArrays::COL2DA_APPEARANCETABLE_FOOTSTEPTYPE, &nFootstepType) == 0)
        return 0;
    if (g_pRules->m_p2DArrays->m_pFootstepSounds->GetCExoStringEntry(
            nFootstepType, CTwoDimArrays::COL2DA_FOOTSTEPSOUNDS_ROLLING, &sSound) == 0)
        return 0;
    m_pRollingFootstepSound = new CExoSoundSource(CResRef(sSound));
    m_pRollingFootstepSound->Set3D(1);
    int nGroup;
    if (m_objectId == g_pAppManager->m_pClientExoApp->GetPlayerCreatureId()) {
        nGroup = PRIORITY_GROUP_PLAYER_FOOTSTEPS;
    } else {
        int nSize;
        g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
            m_pCreatureAppearance->m_nAppearanceId,
            CTwoDimArrays::COL2DA_APPEARANCETABLE_SIZECATEGORY, &nSize);
        nGroup = static_cast<unsigned>(nSize - 1) < 3 ? PRIORITY_GROUP_SMALL_CREATURE_FOOTSTEPS
                                                      : PRIORITY_GROUP_LARGE_CREATURE_FOOTSTEPS;
    }
    m_pRollingFootstepSound->SetPriorityGroup(static_cast<unsigned char>(nGroup));
    m_pRollingFootstepSound->SetLooping(1);
    m_pRollingFootstepSound->SetPosition(m_position, 1.5f);
    g_pRules->m_p2DArrays->m_pFootstepSounds->GetFLOATEntry(
        nFootstepType, CTwoDimArrays::COL2DA_FOOTSTEPSOUNDS_PITCHOFFSET,
        &m_rollingFootstepVariance);
    m_bRollingFootstepPlaying = 0;
    m_rollingFootstepType = 0;
    return m_pRollingFootstepSound != 0;
}

// Builds the save-file base name for this creature: first plus last name, capped at 16
// characters and stripped to alphanumerics, then a decimal suffix appended until no .utc
// resource of that name exists.
CExoString CSWCCreature::GetSaveCreatureFilename(const CExoString& s1, const CExoString& s2) {
    K2_UNUSED(s1); K2_UNUSED(s2);
    CExoString sName;
    CExoString sResult;
    CExoString sSuffix;
#if KOTOR1  // K1 concatenates the two name fields in place
    sName = m_pStats->m_name + m_pStats->m_sLastName;
#else
    sName = m_pStats->GetFirstName() + m_pStats->GetLastName();
#endif
    sName = sName.SubString(0, 16);  // a resref's length
    sName.StripNonAlphaNumeric(1, 0, 0);
    if (sName[0] == 0)
        sName = "0";
    sResult = sName;
    int nCount = 1;
    while (true) {
        if (!g_pExoResMan->Exists(CResRef(static_cast<char*>(sResult.CStr())), RESTYPE_BIC, static_cast<unsigned int*>(0)))
            break;
        sSuffix = CExoString(nCount);
#if KOTOR1  // K1 calls GetLength out of line
        int nLen = sSuffix.GetLength();
#else
        int nLen = sSuffix.m_sString != 0 ? static_cast<int>(strlen(sSuffix.m_sString)) : 0;
#endif
        sName = sName.SubString(0, 16 - nLen);
        sResult = sName + sSuffix;
        nCount++;
    }
    return sResult;
}

// Does the per-frame conjuring bookkeeping. The conjure-start animations fetch the pending spell,
// the in-progress ones reset the cast-visual timer, and on completion the spell's visuals load or
// unload according to the appearance's gender and race flags.
void CSWCCreature::HandleConjuringCreature() {
    CSWSpell* pSpell K2_UNSET;
    int bGotSpell = 0;
    switch (m_currentAnimation) {
    case SERVER_ANIMATION_CONJURE_1: case SERVER_ANIMATION_CONJURE_2: case SERVER_ANIMATION_CAST_1:
    case SERVER_ANIMATION_CAST_2: case SERVER_ANIMATION_CAST_3: case SERVER_ANIMATION_CAST_4:
    case SERVER_ANIMATION_THROW_SABER_LOOP:
    case SERVER_ANIMATION_CAST_1_OVERLAY: case SERVER_ANIMATION_CAST_2_OVERLAY:
    case SERVER_ANIMATION_CAST_3_OVERLAY: case SERVER_ANIMATION_CAST_4_OVERLAY:
    case SERVER_ANIMATION_THROW_SABER_LOOP_OVERLAY: case SERVER_ANIMATION_POWER_ATTACK:
    case SERVER_ANIMATION_CONJURE_3:
        pSpell = g_pRules->GetSpell(m_conjureSpellId);
        bGotSpell = 1;
        break;
    }
    int nConjured;
    switch (m_currentAnimation) {
    case SERVER_ANIMATION_CAST_1: case SERVER_ANIMATION_CAST_2: case SERVER_ANIMATION_CAST_3:
    case SERVER_ANIMATION_CAST_4: case SERVER_ANIMATION_THROW_SABER_LOOP:
    case SERVER_ANIMATION_CAST_1_OVERLAY: case SERVER_ANIMATION_CAST_2_OVERLAY:
    case SERVER_ANIMATION_CAST_3_OVERLAY: case SERVER_ANIMATION_CAST_4_OVERLAY:
    case SERVER_ANIMATION_THROW_SABER_LOOP_OVERLAY: case SERVER_ANIMATION_POWER_ATTACK:
        nConjured = m_bConjureActive;
        if (nConjured == 0) {
            m_pSpellCastVisual->m_aUnloadDelay[0] = 5000;
            m_pSpellCastVisual->m_aUnloadDelay[1] = 5000;
            m_pSpellCastVisual->m_bCastStarted = 0;
            nConjured = 1;
            m_bConjureActive = 1;
        }
        break;
    default:
        nConjured = m_bConjureActive;
        break;
    }
    if (bGotSpell == 0) {
        if (nConjured == 1) {
            if (m_pSpellCastVisual->m_bCastStarted == 0) {
                m_pSpellCastVisual->UnloadConjureVisual();
                m_pSpellCastVisual->UnloadCastVisual();
            }
            m_conjureSpellId = -1;
        }
    } else if (pSpell != 0 && nConjured == 0) {
        CResRef sGendered;
        sGendered = const_cast<char*>("");
        unsigned short nRace = m_pStats->m_nRace;
        if (g_pRules->m_paRaces[nRace].m_bPlayerRace != 0)
            sGendered = m_pCreatureAppearance->m_nGender == GENDER_FEMALE ? pSpell->m_conjSoundFemale
                                                                  : pSpell->m_conjSoundMale;
        CResRef sResRef6;
        sResRef6 = pSpell->m_conjSoundVFX;
        m_pSpellCastVisual->UnloadConjureVisual();
        m_pSpellCastVisual->UnloadCastVisual();
        m_pSpellCastVisual->LoadConjureVisual(pSpell->m_conjHeadVisual, pSpell->m_conjHandVisual,
                                              pSpell->m_conjGrndVisual, sGendered, sResRef6, this);
        m_pSpellCastVisual->m_aUnloadDelay[0] = 5000;
        m_pSpellCastVisual->m_aUnloadDelay[1] = 5000;
        m_pSpellCastVisual->m_bCastStarted = 0;
    }
    m_pSpellCastVisual->UpdateTimer(m_nAIUpdateDeltaMs);
    m_bConjureActive = bGotSpell;
}

// Stealth on/off transition. On a state change this probes framebuffer-effect support and
// swaps the stealth visual pair on the creature itself, then mirrors the swap onto the
// puppeted creature when there is one.
void CSWCCreature::SetStealthState(int nState, int b) {
#if KOTOR1
    if (nState != 0) {
        if (nState == 1 && m_nStealthState == 0) {
            PlaySoundSetSound(VOICE_CHAT_BEGIN_STEALTH, 1);
            g_pAppManager->m_pClientExoApp->GetClientOptions();
            if (preferPerfToVisual != 0 || (AurCheckFrameBufferModificationsAvailable() & 1) == 0) {
                Vector vZero = {};
                AddVisualEffect(VFX_DUR_STEALTH_FIELD, 0, OBJECT_INVALID, OBJECT_INVALID, 0, 0, vZero);
            } else {
                Vector vZero = {};
                AddVisualEffect(VFX_DUR_DISTORTION, 0, OBJECT_INVALID, OBJECT_INVALID, 0, 0, vZero);
            }
            if (b == 0)
                AddFireForgetAnimation(SERVER_ANIMATION_ACTIVATE_ITEM, 0);
        }
    } else if (m_nStealthState == 1) {
        g_pAppManager->m_pClientExoApp->GetClientOptions();
        if (preferPerfToVisual != 0 || (AurCheckFrameBufferModificationsAvailable() & 1) == 0)
            DeleteVisualEffect(VFX_DUR_STEALTH_FIELD);
        else
            DeleteVisualEffect(VFX_DUR_DISTORTION);
        Vector vZero = {};
        AddVisualEffect(VFX_DUR_CLOAK_REMOVAL, 1, OBJECT_INVALID, OBJECT_INVALID, 0, 0, vZero);
        if (b == 0)
            AddFireForgetAnimation(SERVER_ANIMATION_ACTIVATE_ITEM, 0);
    }
    m_nStealthState = nState;
#else
    K2_UNUSED(b);
    int bChanged = 0;
    if (nState != 0) {
        if (nState == 1 && m_nStealthState == 0) {
            PlaySoundSetSound(VOICE_CHAT_BEGIN_STEALTH, 1);
            if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nFrameBuffer != 0)
                AurCheckFrameBufferModificationsAvailable();
            Vector vZero = {};
            AddVisualEffect(VFX_DUR_DISTORTION, 0, OBJECT_INVALID, OBJECT_INVALID, 0, 0, vZero);
            bChanged = 1;
        }
    } else if (m_nStealthState == 1) {
        if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nFrameBuffer != 0)
            AurCheckFrameBufferModificationsAvailable();
        DeleteVisualEffect(VFX_DUR_DISTORTION);
        Vector vZero = {};
        AddVisualEffect(VFX_DUR_CLOAK_REMOVAL, 1, OBJECT_INVALID, OBJECT_INVALID, 0, 0, vZero);
        bChanged = 1;
    }
    m_nStealthState = nState;
    if (GetServerObject() == 0)
        return;
    CSWSCreature* pServer = GetServerObject()->AsSWSCreature();
    if (bChanged == 0 || pServer == 0)
        return;
    int nPUP = pServer->m_nAssignedPup;
    if (nPUP == -1)
        return;
    unsigned long long pupId =
        g_pAppManager->m_pServerExoApp->GetPartyTable()->GetPUPObject(nPUP, 0, 1);
    CSWSCreature* pPUP = static_cast<CSWSCreature*>(g_pAppManager->m_pServerExoApp->GetGameObject(pupId));
    if (pPUP == 0)
        return;
    CSWCCreature* pPUPCreature = pPUP->GetClientCreature();
    if (pPUPCreature == 0)
        return;
    // Frame-buffer effects are only live in the QoL build. The puppeted creature keeps the
    // stock shell either way, so the arm that removes it still finds it.
    if (nState != 0) {
        PlaySoundSetSound(VOICE_CHAT_BEGIN_STEALTH, 1);
#ifdef K2_QOL_FRAME_BUFFER_EFFECTS
        if (true) {
#else
        if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nFrameBuffer == 0 ||
            (AurCheckFrameBufferModificationsAvailable() & 1) == 0) {
#endif
            Vector vZero = {};
            pPUPCreature->AddVisualEffect(VFX_DUR_STEALTH_FIELD, 0, OBJECT_INVALID, OBJECT_INVALID, 0, 0, vZero);
        } else {
            Vector vZero = {};
            pPUPCreature->AddVisualEffect(VFX_DUR_DISTORTION, 0, OBJECT_INVALID, OBJECT_INVALID, 0, 0, vZero);
        }
    } else {
#ifdef K2_QOL_FRAME_BUFFER_EFFECTS
        if (true)
#else
        if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nFrameBuffer == 0 ||
            (AurCheckFrameBufferModificationsAvailable() & 1) == 0)
#endif
            pPUPCreature->DeleteVisualEffect(VFX_DUR_STEALTH_FIELD);
        else
            pPUPCreature->DeleteVisualEffect(VFX_DUR_DISTORTION);
        Vector vZero = {};
        pPUPCreature->AddVisualEffect(VFX_DUR_CLOAK_REMOVAL, 1, OBJECT_INVALID, OBJECT_INVALID, 0, 0, vZero);
    }
#endif
}

// Applies one visibility-list node: adds or removes the visibility effect on the target
// creature according to the node's mask and whether the effect is already there.
__attribute__((always_inline))
void CSWCCreature::UpdateRelativePerception(CSWVisibilityNode* pNode) {
    CSWCCreature* pTarget =
        g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(pNode->m_objectId);
    if (pTarget == 0)
        return;
    unsigned char nFlags = pNode->m_flags;
    if (m_pArea->GetObjectsAtPositionVisible(pTarget->m_position) != 1)
        return;
    if ((nFlags & 0x10) != 0 || (nFlags & 3) == 2) {
        if (pTarget->m_bVisibilityEffectApplied == 0) {
            Vector vZero = {};
            pTarget->AddVisualEffect(VFX_DUR_INVISIBILITY, 0, OBJECT_INVALID, OBJECT_INVALID, 0, 0, vZero);
            pTarget->m_bVisibilityEffectApplied = 1;
            return;
        }
    }
    if ((nFlags & 0x11111111) != 1)
        return;
    if ((nFlags & 3) == 2)
        return;
    if (pTarget->m_bVisibilityEffectApplied != 1)
        return;
    pTarget->DeleteVisualEffect(VFX_DUR_INVISIBILITY);
    pTarget->m_bVisibilityEffectApplied = 0;
}

void CSWCCreature::UpdateRelativePerception(unsigned long long objId) {
    for (int i = 0; i < m_visibilityList.m_count; i++) {
        CSWVisibilityNode* pNode = m_visibilityList.m_pArray[i];
        if (pNode->m_objectId == objId) {
            UpdateRelativePerception(pNode);
            return;
        }
    }
}

// Finds the attached creature-light effect in the visual-effects list and returns its light
// object, gated on the effect's own flags.
void* CSWCCreature::GetCreatureLight(int a) {
    CExoLinkedListNode* pNode = m_visualEffects.m_pInternal->m_pHead;
    CSWCVisualEffectOnObject* pEffect;
    if (m_visualEffects.m_pInternal->m_pHead == 0)
        pEffect = 0;
    else
        pEffect = static_cast<CSWCVisualEffectOnObject*>(m_visualEffects.m_pInternal->m_pHead->m_pData);
    while (pNode != 0) {
        if (pEffect != 0 && static_cast<unsigned char>(pEffect->GetHardCodedEffectClass()) == 4 &&
            pEffect->m_bFinished == 0 && pEffect->m_bAttached != 0 &&
            (a == 0 || pEffect->m_bCastShadows != 0))
            return pEffect->m_pLight;
        pEffect = static_cast<CSWCVisualEffectOnObject*>(m_visualEffects.m_pInternal->GetNext(pNode));
    }
    return 0;
}

void CSWCCreature::PerformDropItem(unsigned long long objId) {
    Vector pos = m_position;
    CSWCItem* pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(objId);
    if (pItem) {
        pItem->m_bInventoryRequestPending = 1;
        CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
        if (pMsg)
            pMsg->SendPlayerToServerInventory_Drop(objId, pos.x, pos.y, pos.z);
    }
}

// The combat idle when the combat-state flag is set, the injured idle when injured and not the
// leading party member, otherwise the pause-ready idle.
int CSWCCreature::GetPauseReadyAnimation() {
    if (m_bInCombat != 0)
        return SERVER_ANIMATION_READY;
    if (g_pAppManager->m_pServerExoApp == 0)
        return SERVER_ANIMATION_PAUSE;
    if (!g_pAppManager->m_pServerExoApp->IsInjured(m_objectId))
        return SERVER_ANIMATION_PAUSE;
    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK &&
        g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0) == this)
        return SERVER_ANIMATION_PAUSE;
    return SERVER_ANIMATION_PAUSE_INJURED;
}

void CSWCCreature::GetPersonalActions(int nType, CExoArrayList<CSWGuiInterfaceAction>& list) {
    switch (nType) {
    case 0: GetSelfForcePowers(list); break;
    case 1: GetUsableItems(list, 1); break;
    case 2: GetUsableItems(list, 4); break;
    case 3: GetUsableItems(list, 2); break;
#if !KOTOR1
    case 4: GetBehaviors(list); break;
    case 5: GetStances(list, nType); break;
#endif
    }
}

int CSWCCreature::BarkBubble(unsigned long long objId, const CExoString& str,
                             const CResRef& resRef, int a) {
    CGuiInGame* pGui = g_pAppManager->m_pClientExoApp->GetInGameGui();
    pGui->ShowBarkBubble(objId, str, resRef, a);
    return 1;
}

void CSWCCreature::ActionMenuUsePassiveFeat(unsigned int nFeat, CSWCCreature* p) {
    K2_UNUSED(p);
    if (g_pRules->GetFeat(static_cast<unsigned short>(nFeat))) {
        CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
        if (pMsg)
            pMsg->SendPlayerToServerInput_UseFeat(static_cast<unsigned short>(nFeat), 0, m_objectId);
    }
}

extern int RenderPosition;
void CSWCCreature::SetPosition(const Vector& v) {
    m_position = v;
    if (m_pArea)
        m_pArea->GetObjectsAtPositionVisible(v);
    if (RenderPosition)
        AsSWCCreature();  // Result discarded.
    if (m_pModel && m_bModelSyncDisabled == 0)
        m_pModel->SetPosition(v);
    UpdateWalkMesh();
}

// Party-membership toggle; on removal a leading player creature is demoted first.
int CSWCCreature::SetInParty(int partyFlag) {
    if (m_bInParty == partyFlag)
        return 1;
    m_bInParty = partyFlag;
    if (partyFlag) {
        return g_pAppManager->m_pClientExoApp->GetSWParty()->AddCharacter(m_objectId);
    } else {
        if (g_pAppManager->m_pClientExoApp->IsPlayerCreature(this))
            g_pAppManager->m_pClientExoApp->GetSWParty()->SetLeader(-3);
        return g_pAppManager->m_pClientExoApp->GetSWParty()->RemoveCharacter(m_objectId);
    }
}

void CSWCCreature::SetCreatureType(unsigned char type) {
    m_creatureType = type;
    m_pModel->m_creatureType = type;
}

void CSWCCreature::Hilite(const Vector& v, unsigned char a, float f, int b) {
    K2_UNUSED(f);
    if (m_pModel)
        m_pModel->Hilite(v, a, b);
}

void CSWCCreature::UnHilite(unsigned char a) {
    if (m_pModel)
        m_pModel->UnHilite(a);
}

CSWCCreature* CSWCCreature::AsSWCCreature() {
    return this;
}

void CSWCCreature::HandleSwitchWeapon() {
}

void CSWCCreature::UpdateMonkVisuals() {
}

void CSWCCreature::ShowFeedbackBubbleText() {
}

void CSWCCreature::SetPopUpBubbleDecayTime(float f) {
    K2_UNUSED(f);
}

int CSWCCreature::CanSwapWeapons() {
    return 1;
}

// Counts equipped items matching the server item's base item and comparison key, walking the
// 20 slot bits and skipping the empty ones.
int CSWCCreature::GetMatchingEquippedItemCount(CSWSItem* pItem) {
    int nCount = 0;
    for (int i = 0; i != 20; i++) {
        if (i - INVENTORY_SLOT_CWEAPON_L >= 0 && i - INVENTORY_SLOT_CWEAPON_L < 4)  // the creature slots
            continue;
        CClientExoApp* pApp = g_pAppManager->m_pClientExoApp;
        unsigned long long* pItemId;
        switch (1 << i) {
        case MSG_SLOT_HEAD:              pItemId = &m_aEquippedItemIds[0];  break;
        case MSG_SLOT_BODY:              pItemId = &m_aEquippedItemIds[1];  break;
        case MSG_SLOT_BOOTS:             pItemId = &m_aEquippedItemIds[2];  break;
        case MSG_SLOT_HANDS:             pItemId = &m_aEquippedItemIds[3];  break;
        case MSG_SLOT_RIGHT_WEAPON:      pItemId = &m_aEquippedItemIds[4];  break;
        case MSG_SLOT_LEFT_WEAPON:       pItemId = &m_aEquippedItemIds[5];  break;
        case MSG_SLOT_CLOAK:             pItemId = &m_aEquippedItemIds[6];  break;
        case MSG_SLOT_LEFT_ARM:          pItemId = &m_aEquippedItemIds[7];  break;
        case MSG_SLOT_RIGHT_ARM:         pItemId = &m_aEquippedItemIds[8];  break;
        case MSG_SLOT_IMPLANT:           pItemId = &m_aEquippedItemIds[9];  break;
        case MSG_SLOT_BELT:              pItemId = &m_aEquippedItemIds[10]; break;
        case MSG_SLOT_CREATURE_WEAPON_L: pItemId = &m_aEquippedItemIds[11]; break;
        case MSG_SLOT_CREATURE_WEAPON_R: pItemId = &m_aEquippedItemIds[12]; break;
        case MSG_SLOT_CREATURE_WEAPON_B: pItemId = &m_aEquippedItemIds[13]; break;
        case MSG_SLOT_CREATURE_ARMOR:    pItemId = &m_aEquippedItemIds[14]; break;
        case MSG_SLOT_RIGHT_WEAPON_2:    pItemId = &m_aEquippedItemIds[15]; break;
        case MSG_SLOT_LEFT_WEAPON_2:     pItemId = &m_aEquippedItemIds[16]; break;
        default:      pItemId = const_cast<unsigned long long*>(&CGameObjectArray::INVALIDOBJID); break;
        }
        CSWCItem* pEquipped = pApp->GetItemByGameObjectID(*pItemId);
        if (pEquipped != 0) {
            CSWSItem* pServerItem = pEquipped->GetServerItem();
            if (pServerItem != 0 && pServerItem->m_baseItem == pItem->m_baseItem &&
                pServerItem->m_nModelVariation == pItem->m_nModelVariation &&
                pItem->CompareItem(pServerItem) != 0) {
                nCount++;
            }
        }
    }
    return nCount;
}

int CSWCCreature::CanUseMonkVisuals(int a) {
    K2_UNUSED(a);
    return 0;
}

// The first way-point in the array; null when the array is empty.
Vector* CSWCCreature::GetStartWayPoint() {
    if (m_wayPointCount)
        return m_pWayPoints;
    return 0;
}

// The last way-point in the array; null when the array is empty.
Vector* CSWCCreature::GetEndWayPoint() {
    if (m_wayPointCount)
        return &m_pWayPoints[m_wayPointCount - 1];
    return 0;
}

int CSWCCreature::Moving() {
    float speed = m_switchMode == 1 ? m_swAnimationSpeed : m_fMoveSpeedScale;
    return speed != 0.0f;
}

// The stats block's portrait row, or the sentinel when there is no stats block.
unsigned short CSWCCreature::GetPortraitId() {
    if (m_pStats)
        return m_pStats->m_portraitId;
    return CSWPortrait::PORTRAIT_ID_NONE;
}

#if !KOTOR1
void CSWCCreature::SetBaseCNPCAlignment(short a) {
    if (m_pStats) {
        int v = a;
        if (v < 0)
            v = -1;
        if (v >= 100)
            v = 100;
        m_pStats->m_baseCNPCAlignment = static_cast<short>(v);
    }
}
#endif

// Ambient-pause animation driver for creature-type-1 models. When forced this picks one of
// two pause animations and plays it on both model parts; otherwise it accumulates time to a
// threshold, then picks a pause animation plus a second one from the six-way switch, flags both
// parts and raises the threshold by that animation's length.
void CSWCCreature::CGPauseCycle(float f, int b) {
    if (m_pModel == 0 || m_pModel->m_creatureType != CSWCAnimBase::CREATURE_TYPE_FULL)
        return;
    CExoString sAnim1;
    CExoString sAnim2;
    float fUnused = 0.0f;
    float fAnimLen = 0.0f;
    Gob* pPart;
    Gob* pPart2;
    if (b != 0) {
        int r = rand_wincompatible() % 2;
        if (r == 1)
            sAnim1 = "pause2";
        else if (r == 0)
            sAnim1 = "pause1";
        m_nAmbientPauseDelay = 10000 + (rand_wincompatible() % 201) * 100;
        float fRand = static_cast<float>(rand_wincompatible() % 100);
        pPart = m_pModel->GetModel(CSWCAnimBase::ANIM_MODEL_BODY);
        if (pPart) {
            pPart->GetAnimation(static_cast<char*>(sAnim1.CStr()), &fUnused, &fAnimLen);
            pPart->PlayAnimation(static_cast<char*>(sAnim1.CStr()), 1.0f, 2, fAnimLen * 0.01f * fRand);
        }
        pPart2 = m_pModel->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD);
        if (pPart2)
            pPart2->PlayAnimation(static_cast<char*>(sAnim1.CStr()), 1.0f, 2, fAnimLen * 0.01f * fRand);
        return;
    }
    m_nAmbientPauseElapsed += static_cast<int>(f * 1000.0f);
    if (m_nAmbientPauseElapsed < m_nAmbientPauseDelay)
        return;
    int r = rand_wincompatible() % 2;
    if (r == 1)
        sAnim1 = "pause2";
    else if (r == 0)
        sAnim1 = "pause1";
    switch (rand_wincompatible() % 6) {
    case 0: sAnim2 = "greeting"; break;
    case 1: sAnim2 = "hturnl"; break;
    case 2: sAnim2 = "hturnr"; break;
    case 3: sAnim2 = "pausebrd"; break;
    case 4: sAnim2 = "pausesh"; break;
    case 5: sAnim2 = "pause3"; break;
    }
    pPart = m_pModel->GetModel(CSWCAnimBase::ANIM_MODEL_BODY);
    if (pPart) {
        pPart->GetAnimation(static_cast<char*>(sAnim2.CStr()), &fUnused, &fAnimLen);
        pPart->AddAnimationFlag(1);
        pPart->PlayAnimation(static_cast<char*>(sAnim2.CStr()), 1.0f, AnimRun::kAnimOnce | AnimRun::kAnimQueue, 0.0f);
        m_nAmbientPauseDelay += static_cast<int>(fAnimLen * 1000.0f);
        pPart->PlayAnimation(static_cast<char*>(sAnim1.CStr()), 1.0f, AnimRun::kAnimQueue | AnimRun::kAnimNoReuse, 0.0f);
    }
    pPart2 = m_pModel->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD);
    if (pPart2) {
        pPart2->AddAnimationFlag(1);
        pPart2->PlayAnimation(static_cast<char*>(sAnim2.CStr()), 1.0f, AnimRun::kAnimOnce | AnimRun::kAnimQueue, 0.0f);
        pPart2->PlayAnimation(static_cast<char*>(sAnim1.CStr()), 1.0f, AnimRun::kAnimQueue | AnimRun::kAnimNoReuse, 0.0f);
    }
    m_nAmbientPauseDelay = 10000 + (rand_wincompatible() % 201) * 100;
    m_nAmbientPauseElapsed = 0;
}

// Clamps the alignment into the stats block, then forwards the raw value to the appearance
// helper.
void CSWCCreature::SetAlignmentGoodEvil(short a) {
    if (m_pStats) {
        int v = a;
        if (v < 0)
            v = 0;
        if (v >= 100)
            v = 100;
        m_pStats->m_alignmentGoodEvil = static_cast<short>(v);
    }
    if (m_pCreatureAppearance)
        m_pCreatureAppearance->SetAppearanceAlignment(a);
}

// When the new portrait differs from the current one, caches it in the stats block and
// refreshes the pop-up bubble portrait.
void CSWCCreature::SetPortrait(const CResRef& r) {
    if (GetPortrait(0) != r) {
        m_pStats->m_portrait.SetPortrait(r);
        SetPopUpBubblePortrait(GetPortrait(1));
    }
}

// Stores the portrait row on the stats block and refreshes the pop-up bubble portrait.
void CSWCCreature::SetPortraitId(unsigned short a) {
    if (m_pStats) {
        m_pStats->SetPortraitId(a);
        SetPopUpBubblePortrait(GetPortrait(1));
    }
}

// The portrait wrapper's resref for our state, or an empty resref without a stats block.
CResRef CSWCCreature::GetPortrait(unsigned char a) {
    if (m_pStats)
        return m_pStats->m_portrait.GetPortrait(a);
    return CResRef("");
}

// The stats block's alignment-varied portrait resref, or an empty resref without a stats
// block.
CResRef CSWCCreature::GetPortrait(int a) {
    if (m_pStats)
        return m_pStats->GetPortrait(a);
    return CResRef("");
}

// Forwards to the appearance helper.

void* CSWCCreature::GetCameraFollowPart() {
    return m_pCreatureAppearance->GetCameraFollowPart();
}

int CSWCCreature::GetFollowingBody() {
    return m_pCreatureAppearance->GetFollowingBody();
}

float CSWCCreature::GetHeadHeightOffset() {
    return m_pCreatureAppearance->GetHeadHeightOffset();
}

float CSWCCreature::GetCameraOffset() {
    return m_pCreatureAppearance->GetCameraOffset();
}

// Re-shows the wielded weapons on the model, then detaches any thrown lightsaber's projectile
// from the scene.
void CSWCCreature::ShowEquippedItems()
{
    if (m_pModel) {
        ComputeWeaponOverlays(1, 0);
        m_pModel->ShowWieldedItems();
        unsigned long long id = m_thrownLightsaber;
        m_bEquippedItemsHidden = 0;
        if (id != CGameObjectArray::INVALIDOBJID) {
            CSWCProjectile* pProjectile =
                g_pAppManager->m_pClientExoApp->GetProjectileByGameObjectID(id);
            if (pProjectile)
                pProjectile->AttachToScene(0);
            m_thrownLightsaber = CGameObjectArray::INVALIDOBJID;
        }
    }
}

// nSlot is one inventory-slot bit; only 17 of the 20 slots have a backing array entry, and
// every other value returns the sentinel.
unsigned long long CSWCCreature::GetEquippedItemID(unsigned int nSlot) {
    switch (nSlot) {
    case MSG_SLOT_HEAD:              return m_aEquippedItemIds[0];
    case MSG_SLOT_BODY:              return m_aEquippedItemIds[1];
    case MSG_SLOT_BOOTS:             return m_aEquippedItemIds[2];
    case MSG_SLOT_HANDS:             return m_aEquippedItemIds[3];
    case MSG_SLOT_RIGHT_WEAPON:      return m_aEquippedItemIds[4];
    case MSG_SLOT_LEFT_WEAPON:       return m_aEquippedItemIds[5];
    case MSG_SLOT_CLOAK:             return m_aEquippedItemIds[6];
    case MSG_SLOT_LEFT_ARM:          return m_aEquippedItemIds[7];
    case MSG_SLOT_RIGHT_ARM:         return m_aEquippedItemIds[8];
    case MSG_SLOT_IMPLANT:           return m_aEquippedItemIds[9];
    case MSG_SLOT_BELT:              return m_aEquippedItemIds[10];
    case MSG_SLOT_CREATURE_WEAPON_L: return m_aEquippedItemIds[11];
    case MSG_SLOT_CREATURE_WEAPON_R: return m_aEquippedItemIds[12];
    case MSG_SLOT_CREATURE_WEAPON_B: return m_aEquippedItemIds[13];
    case MSG_SLOT_CREATURE_ARMOR:    return m_aEquippedItemIds[14];
#if !KOTOR1  // K1 has no creature-weapon slots
    case MSG_SLOT_RIGHT_WEAPON_2: return m_aEquippedItemIds[15];
    case MSG_SLOT_LEFT_WEAPON_2:  return m_aEquippedItemIds[16];
#endif
    default:      return CGameObjectArray::INVALIDOBJID;
    }
}

#if !KOTOR1
static void UnequipItemById(unsigned long long itemId) {
    CGameObject* pObj;
    if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->GetGameObject(itemId, &pObj) ==
        CGameObjectArray::SUCCESS) {
        static_cast<CSWCItem*>(pObj->AsSWCItem())->SetEquippedBy(0);
    }
}
#endif

#if KOTOR1
// K1's form: no creature-weapon slots, so only the two hands are paired, and the replaced
// item's model is always torn down. A hand slot keeps its old item when the incoming id already
// sits in the other hand.
void CSWCCreature::EquipToInventorySlot(unsigned int nSlot, CSWCItem* pItem) {
    Gob* pModel = 0;
    unsigned long long itemId = OBJECT_INVALID;
    if (pItem != 0) {
        pModel = pItem->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
        itemId = pItem->m_objectId;
        pItem->SetEquippedBy(this);
    }

    CSWCItem* pOld = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(GetEquippedItemID(nSlot));

    if (nSlot == MSG_SLOT_BODY) {
        if (pModel != 0) pModel->AttachToScene(0);
        if (m_aEquippedItemIds[1] != itemId) SetItemUnEquippedByMe(m_aEquippedItemIds[1]);
        m_aEquippedItemIds[1] = itemId;
    } else if (nSlot == MSG_SLOT_RIGHT_WEAPON || nSlot == MSG_SLOT_LEFT_WEAPON) {
        if (GetEquippedItemID(nSlot) != itemId) {
            if (nSlot == MSG_SLOT_RIGHT_WEAPON) {
                if (GetEquippedItemID(MSG_SLOT_LEFT_WEAPON) != itemId ||
                    GetEquippedItemID(MSG_SLOT_LEFT_WEAPON) == CGameObjectArray::INVALIDOBJID)
                    SetItemUnEquippedByMe(GetEquippedItemID(nSlot));
            } else {
                if (GetEquippedItemID(MSG_SLOT_RIGHT_WEAPON) != itemId ||
                    GetEquippedItemID(MSG_SLOT_RIGHT_WEAPON) == CGameObjectArray::INVALIDOBJID)
                    SetItemUnEquippedByMe(GetEquippedItemID(nSlot));
            }
        }
        SetEquippedItemID(nSlot, itemId);
        UpdateWeaponClass();
        UpdateVisibleWeapons(nSlot, pItem, GetLastInstant(nSlot));
        ResetAnimationState();
    } else if (nSlot == MSG_SLOT_HEAD) {
        if (GetEquippedItemID(nSlot) != itemId) SetItemUnEquippedByMe(m_aEquippedItemIds[0]);
        m_aEquippedItemIds[0] = itemId;
        if (pItem != 0) pItem->m_possessorObjectId = m_objectId;
    } else {
        if (GetEquippedItemID(nSlot) != itemId) SetItemUnEquippedByMe(GetEquippedItemID(nSlot));
        SetEquippedItemID(nSlot, itemId);
    }

    if (pOld != 0 && pOld != pItem &&
        pOld->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1) != 0) pOld->UnloadModel();
    if (pItem != 0) pItem->SetArea(0);
}
#endif

#if !KOTOR1
// Drops whatever the slot currently holds and records the item's object id in it. A null item
// is a plain unequip. The four wielded slots also clear their pair before stamping the new id,
// and the two hand slots then run the weapon-refresh tail. The head slot re-parents the item to
// this creature instead.
void CSWCCreature::EquipToInventorySlot(unsigned int nSlot, CSWCItem* pItem, int bFlag) {
    Gob* pModel = 0;
    unsigned long long itemId = OBJECT_INVALID;
    if (pItem != 0) {
        pModel = pItem->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
        itemId = pItem->m_objectId;
        pItem->SetEquippedBy(this);
    }

    const unsigned long long* pSlotId = &CGameObjectArray::INVALIDOBJID;
    switch (nSlot) {
    case MSG_SLOT_HEAD:              pSlotId = &m_aEquippedItemIds[0];  break;
    case MSG_SLOT_BODY:              pSlotId = &m_aEquippedItemIds[1];  break;
    case MSG_SLOT_BOOTS:             pSlotId = &m_aEquippedItemIds[2];  break;
    case MSG_SLOT_HANDS:             pSlotId = &m_aEquippedItemIds[3];  break;
    case MSG_SLOT_RIGHT_WEAPON:      pSlotId = &m_aEquippedItemIds[4];  break;
    case MSG_SLOT_LEFT_WEAPON:       pSlotId = &m_aEquippedItemIds[5];  break;
    case MSG_SLOT_CLOAK:             pSlotId = &m_aEquippedItemIds[6];  break;
    case MSG_SLOT_LEFT_ARM:          pSlotId = &m_aEquippedItemIds[7];  break;
    case MSG_SLOT_RIGHT_ARM:         pSlotId = &m_aEquippedItemIds[8];  break;
    case MSG_SLOT_IMPLANT:           pSlotId = &m_aEquippedItemIds[9];  break;
    case MSG_SLOT_BELT:              pSlotId = &m_aEquippedItemIds[10]; break;
    case MSG_SLOT_CREATURE_WEAPON_L: pSlotId = &m_aEquippedItemIds[11]; break;
    case MSG_SLOT_CREATURE_WEAPON_R: pSlotId = &m_aEquippedItemIds[12]; break;
    case MSG_SLOT_CREATURE_WEAPON_B: pSlotId = &m_aEquippedItemIds[13]; break;
    case MSG_SLOT_CREATURE_ARMOR:    pSlotId = &m_aEquippedItemIds[14]; break;
    case MSG_SLOT_RIGHT_WEAPON_2:    pSlotId = &m_aEquippedItemIds[15]; break;
    case MSG_SLOT_LEFT_WEAPON_2:     pSlotId = &m_aEquippedItemIds[16]; break;
    }

    unsigned long long slotId = *pSlotId;
    CSWCItem* pOld = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(slotId);

    pSlotId = &m_aEquippedItemIds[5];

    bool bRaw = false;  // The incoming id sits in another wielded slot.
    bool bEarly = false;  // Right-hand arm stamps before the pair check.
    bool bAccum = false;  // The accumulated flag.
    bool bUnload = false;  // Tear the replaced item's model down.
    bool bOther = false;  // The raw incoming-id case.
    const unsigned long long* pPair = 0;
    unsigned long long nOtherId = 0;  // The pair slot's id, shared by the two creature-weapon arms.

    if (m_aEquippedItemIds[5] == slotId || m_aEquippedItemIds[4] == slotId) {
        if (nSlot == MSG_SLOT_RIGHT_WEAPON) {
            if (m_aEquippedItemIds[5] != slotId) goto L1e8;
            goto L200;
        }
        if (nSlot == MSG_SLOT_LEFT_WEAPON) goto L220;
        bAccum = false;
        if (nSlot == MSG_SLOT_LEFT_WEAPON_2) goto L1ac;
        goto L22c;
    }
    if (nSlot != MSG_SLOT_RIGHT_WEAPON) goto L220;

L1e8:
    if (m_aEquippedItemIds[15] == slotId) goto L200;
    if (m_aEquippedItemIds[16] != slotId) goto L3b4;
L200:
    bRaw = true;
L204:
    pPair = &m_aEquippedItemIds[4];
    bUnload = true;
    bEarly = true;
    bOther = bRaw && slotId != CGameObjectArray::INVALIDOBJID;
    goto L3ec;
L3b4:
    bRaw = false;
    goto L204;

L1ac:
    if (m_aEquippedItemIds[5] == slotId) goto L1cc;
    if (m_aEquippedItemIds[4] == slotId) goto L1cc;
    if (m_aEquippedItemIds[15] != slotId) goto L3d0;
L1cc:
    bRaw = true;
    goto L3d4;
L3d0:
    bRaw = false;
L3d4:
    pPair = &m_aEquippedItemIds[16];
    bEarly = false;
    goto L3e0;

L220:
    bAccum = true;
    if (nSlot == MSG_SLOT_LEFT_WEAPON_2) goto L1ac;
L22c:
    if (nSlot == MSG_SLOT_RIGHT_WEAPON_2) goto L274;
    if (nSlot != MSG_SLOT_LEFT_WEAPON) goto L29c;
    if (m_aEquippedItemIds[4] == slotId) goto L260;
    if (m_aEquippedItemIds[15] == slotId) goto L260;
    if (m_aEquippedItemIds[16] != slotId) goto L614;
L260:
    bRaw = true;
L264:
    pPair = pSlotId;
    bEarly = false;
    goto L3e0;
L614:
    bRaw = false;
    goto L264;

L274:
    if (m_aEquippedItemIds[5] == slotId) goto L294;
    if (m_aEquippedItemIds[4] == slotId) goto L294;
    if (m_aEquippedItemIds[16] != slotId) goto L3bc;
L294:
    bRaw = true;
    goto L3c0;
L3bc:
    bRaw = false;
L3c0:
    pPair = &m_aEquippedItemIds[15];
    bEarly = false;
    goto L3e0;

L29c:
    if (nSlot - 2 != 0) goto L36c;
    bUnload = bAccum;
    if (pModel != 0) pModel->AttachToScene(0);
    if (m_aEquippedItemIds[1] != itemId &&
        m_aEquippedItemIds[1] != CGameObjectArray::INVALIDOBJID) {
        UnequipItemById(m_aEquippedItemIds[1]);
    }
    m_aEquippedItemIds[1] = itemId;
    goto Finish;

L3e0:
    bOther = bRaw && slotId != CGameObjectArray::INVALIDOBJID;
    bUnload = bAccum;
L3ec:
    if (*pPair != itemId) goto L47c;
L3f8:
    SetEquippedItemID(nSlot, itemId);
    if (nSlot != MSG_SLOT_LEFT_WEAPON && nSlot != MSG_SLOT_RIGHT_WEAPON) goto Finish;
L418:
    UpdateWeaponClass();
    {
        int nInstant = GetLastInstant(nSlot);
        UpdateVisibleWeapons(nSlot, pItem, nInstant, bFlag);
    }
    ResetAnimationState();
    goto Finish;

L47c:
    if (!bEarly) goto L4f0;
    if (m_aEquippedItemIds[5] == CGameObjectArray::INVALIDOBJID || m_aEquippedItemIds[5] != itemId) {
        if (!bOther && m_aEquippedItemIds[4] != CGameObjectArray::INVALIDOBJID)
            UnequipItemById(m_aEquippedItemIds[4]);
    }
    goto L3f8;

L4f0:
    switch (nSlot) {
    case MSG_SLOT_LEFT_WEAPON_2:  goto L51c;
    case MSG_SLOT_RIGHT_WEAPON_2: goto L530;
    case MSG_SLOT_LEFT_WEAPON:    break;
    default:      goto L3f8;
    }
    if (m_aEquippedItemIds[4] != CGameObjectArray::INVALIDOBJID) goto L59c;
    if (bOther) goto L600;
    goto L5a8;
L51c:
    if (m_aEquippedItemIds[15] != CGameObjectArray::INVALIDOBJID) goto L56c;
    if (bOther) goto L8b4;
    goto L578;
L56c:
    if (bOther) goto L8b4;
    if (m_aEquippedItemIds[15] == itemId) goto L8b4;
    goto L578;
L530:
    if (m_aEquippedItemIds[16] != CGameObjectArray::INVALIDOBJID) goto L580;
    if (bOther) goto L8b4;
    goto L58c;
L580:
    if (bOther) goto L8b4;
    if (m_aEquippedItemIds[16] == itemId) goto L8b4;
    goto L58c;
L578:
    nOtherId = m_aEquippedItemIds[16];
    goto L590;
L58c:
    nOtherId = m_aEquippedItemIds[15];
L590:
    if (nOtherId != CGameObjectArray::INVALIDOBJID) UnequipItemById(nOtherId);
    goto L8b4;
L59c:
    if (bOther) goto L600;
    if (m_aEquippedItemIds[4] == itemId) goto L600;
L5a8:
    if (m_aEquippedItemIds[5] != CGameObjectArray::INVALIDOBJID) UnequipItemById(m_aEquippedItemIds[5]);
L600:
    SetEquippedItemID(nSlot, itemId);
    goto L418;

L36c:
    pPair = &CGameObjectArray::INVALIDOBJID;
    bUnload = bAccum;
    if (nSlot >= MSG_SLOT_IMPLANT) goto L544;
    if (nSlot >= MSG_SLOT_CLOAK) goto L61c;
    if (nSlot - 1 > MSG_SLOT_LEFT_WEAPON - 1) goto L6b4;
    switch (nSlot) {
    case MSG_SLOT_HEAD:        pPair = &m_aEquippedItemIds[0]; break;
    case MSG_SLOT_BOOTS:       pPair = &m_aEquippedItemIds[2]; break;
    case MSG_SLOT_HANDS:       pPair = &m_aEquippedItemIds[3]; break;
    case MSG_SLOT_LEFT_WEAPON: pPair = pSlotId; break;
    }
    goto L6b4;
L61c:
    if (nSlot == MSG_SLOT_CLOAK) { pPair = &m_aEquippedItemIds[6];  goto L6b4; }
    if (nSlot == MSG_SLOT_LEFT_ARM) { pPair = &m_aEquippedItemIds[7];  goto L6b4; }
    if (nSlot == MSG_SLOT_RIGHT_ARM) { pPair = &m_aEquippedItemIds[8];  goto L6b4; }
    goto L6b4;
L544:
    if (nSlot < MSG_SLOT_CREATURE_WEAPON_R) goto L63c;
    if (nSlot >= MSG_SLOT_CREATURE_ARMOR) goto L65c;
    if (nSlot == MSG_SLOT_CREATURE_WEAPON_R) { pPair = &m_aEquippedItemIds[12]; goto L6b4; }
    if (nSlot == MSG_SLOT_CREATURE_WEAPON_B) { pPair = &m_aEquippedItemIds[13]; goto L6b4; }
    goto L6b4;
L63c:
    if (nSlot == MSG_SLOT_IMPLANT) { pPair = &m_aEquippedItemIds[9];  goto L6b4; }
    if (nSlot == MSG_SLOT_BELT) { pPair = &m_aEquippedItemIds[10]; goto L6b4; }
    if (nSlot == MSG_SLOT_CREATURE_WEAPON_L) { pPair = &m_aEquippedItemIds[11]; goto L6b4; }
    goto L6b4;
L65c:
    if (nSlot == MSG_SLOT_CREATURE_ARMOR) { pPair = &m_aEquippedItemIds[14]; goto L6b4; }
    if (nSlot == MSG_SLOT_RIGHT_WEAPON_2) { pPair = &m_aEquippedItemIds[15]; goto L6b4; }
L6b4:
    if (nSlot == MSG_SLOT_HEAD) {
        if (*pPair != itemId && m_aEquippedItemIds[0] != CGameObjectArray::INVALIDOBJID)
            UnequipItemById(m_aEquippedItemIds[0]);
        m_aEquippedItemIds[0] = itemId;
        if (pItem != 0) pItem->m_possessorObjectId = m_objectId;
        goto Finish;
    }
    if (*pPair == itemId) goto L8b4;
    pPair = &CGameObjectArray::INVALIDOBJID;
    switch (nSlot) {
    case MSG_SLOT_BODY:              pPair = &m_aEquippedItemIds[1]; break;
    case MSG_SLOT_BOOTS:             pPair = &m_aEquippedItemIds[2]; break;
    case MSG_SLOT_HANDS:             pPair = &m_aEquippedItemIds[3]; break;
    case MSG_SLOT_RIGHT_WEAPON:      pPair = &m_aEquippedItemIds[4]; break;
    case MSG_SLOT_LEFT_WEAPON:       pPair = pSlotId; break;
    case MSG_SLOT_CLOAK:             pPair = &m_aEquippedItemIds[6];  break;
    case MSG_SLOT_LEFT_ARM:          pPair = &m_aEquippedItemIds[7];  break;
    case MSG_SLOT_RIGHT_ARM:         pPair = &m_aEquippedItemIds[8];  break;
    case MSG_SLOT_IMPLANT:           pPair = &m_aEquippedItemIds[9];  break;
    case MSG_SLOT_BELT:              pPair = &m_aEquippedItemIds[10]; break;
    case MSG_SLOT_CREATURE_WEAPON_L: pPair = &m_aEquippedItemIds[11]; break;
    case MSG_SLOT_CREATURE_WEAPON_R: pPair = &m_aEquippedItemIds[12]; break;
    case MSG_SLOT_CREATURE_WEAPON_B: pPair = &m_aEquippedItemIds[13]; break;
    case MSG_SLOT_CREATURE_ARMOR:    pPair = &m_aEquippedItemIds[14]; break;
    case MSG_SLOT_RIGHT_WEAPON_2:    pPair = &m_aEquippedItemIds[15]; break;
    case MSG_SLOT_LEFT_WEAPON_2:     pPair = &m_aEquippedItemIds[16]; break;
    }
    nOtherId = *pPair;
    if (nOtherId != CGameObjectArray::INVALIDOBJID) UnequipItemById(nOtherId);
L8b4:
    SetEquippedItemID(nSlot, itemId);

Finish:
    if (pOld != 0 && pOld != pItem) {
        Gob* pOldModel = pOld->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1);
        if (bUnload && pOldModel != 0) pOld->UnloadModel();
    }
    if (pItem != 0) pItem->SetArea(0);
}
#endif

// Stores the id in the slot's array entry; a no-op for unslotted values.
void CSWCCreature::SetEquippedItemID(unsigned int nSlot, unsigned long long itemId) {
    switch (nSlot) {
    case MSG_SLOT_HEAD:              m_aEquippedItemIds[0] = itemId;  break;
    case MSG_SLOT_BODY:              m_aEquippedItemIds[1] = itemId;  break;
    case MSG_SLOT_BOOTS:             m_aEquippedItemIds[2] = itemId;  break;
    case MSG_SLOT_HANDS:             m_aEquippedItemIds[3] = itemId;  break;
    case MSG_SLOT_RIGHT_WEAPON:      m_aEquippedItemIds[4] = itemId;  break;
    case MSG_SLOT_LEFT_WEAPON:       m_aEquippedItemIds[5] = itemId;  break;
    case MSG_SLOT_CLOAK:             m_aEquippedItemIds[6] = itemId;  break;
    case MSG_SLOT_LEFT_ARM:          m_aEquippedItemIds[7] = itemId;  break;
    case MSG_SLOT_RIGHT_ARM:         m_aEquippedItemIds[8] = itemId;  break;
    case MSG_SLOT_IMPLANT:           m_aEquippedItemIds[9] = itemId;  break;
    case MSG_SLOT_BELT:              m_aEquippedItemIds[10] = itemId; break;
    case MSG_SLOT_CREATURE_WEAPON_L: m_aEquippedItemIds[11] = itemId; break;
    case MSG_SLOT_CREATURE_WEAPON_R: m_aEquippedItemIds[12] = itemId; break;
    case MSG_SLOT_CREATURE_WEAPON_B: m_aEquippedItemIds[13] = itemId; break;
    case MSG_SLOT_CREATURE_ARMOR:    m_aEquippedItemIds[14] = itemId; break;
#if !KOTOR1
    case MSG_SLOT_RIGHT_WEAPON_2: m_aEquippedItemIds[15] = itemId; break;
    case MSG_SLOT_LEFT_WEAPON_2:  m_aEquippedItemIds[16] = itemId; break;
#endif
    }
}

int CSWCCreature::CanDualWield(CSWCItem* pItem) {
    CSWBaseItem* pBaseItem = g_pRules->GetBaseItem(pItem->m_baseItem);
    if (pBaseItem == 0)
        return 0;
    if (pItem->GetBaseItem()->m_nItemType == ITEM_TYPE_LIGHT_SOURCE)
        return 1;
    if (pBaseItem->m_nWeaponType == 0)
        return 0;
    int nRelSize = GetRelativeWeaponSize(pItem);
    if (pBaseItem->m_bRangedWeapon != 0)
        return 2;
    return static_cast<unsigned>(nRelSize + 2) < 3;
}

#if !KOTOR1
// Builds the creature's radial-menu stance entries. nType is never read, and the string local
// is unused. Each new entry takes the form dispatch callback, this creature's object id and
// the stance flags.
void CSWCCreature::GetStances(CExoArrayList<CSWGuiInterfaceAction>& list, int nType)
{
    K2_UNUSED(nType);
    int nOldCount = list.m_count;
    CExoString sUnused;  // Unused.
    m_pStats->GetMenuInfo(list, 3, 0, 0, 0, 0.0001f, 0, -1, 0, 1, 0, 0, 0);
    for (int i = nOldCount; i < list.m_count; i++) {
        CSWGuiInterfaceAction& entry = list.m_pArray[i];
        entry.m_nActionId &= CSWGuiInterfaceAction::ACTION_ID_SPELL_CLEAR;
        entry.m_pfnAction = &CSWCCreature::ActionMenuForm;
        entry.m_nObjectId = m_objectId;
        entry.m_bChangesState = 1;
    }
}
#endif

// Builds the creature's radial-menu self-targeted force powers. The implant-switching and
// wookiee-rage feats hand off to the implant and wookiee-rage builders; otherwise the server twin supplies the force item
// mask, the force-point pool, the room force rating and the Force Body level.
void CSWCCreature::GetSelfForcePowers(CExoArrayList<CSWGuiInterfaceAction>& list)
{
#if KOTOR1
// K1: no implant or rage feats and no room, Force Body, health, stance or level inputs.
    int nForcePoints = 0;
    unsigned int nForceItemMask = 0;
    if (GetServerObject() != 0) {
        CSWSCreature* pServer = GetServerObject()->AsSWSCreature();
        if (pServer != 0) {
            nForceItemMask = pServer->GetForceItemMask();
            if (pServer->m_bBodyFuel != 0)
                nForcePoints = static_cast<short>(pServer->GetCurrentHitPoints(0));
            else
                nForcePoints = static_cast<short>(pServer->m_pStats->m_nCurrentForcePoints + pServer->m_pStats->m_nTemporaryForcePoints);
        }
    }
    int nOldCount = list.m_count;
    m_pStats->GetMenuInfo(list, 3, nForcePoints, 0, nForceItemMask, 0.0001f, 0, -1, 0, 0, 0, 0, 0);
#else
    if (m_pStats->HasFeat(FEAT_IMPLANT_SWITCHING))
        GetSwappableImplants(list);
    if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_WOOKIEE_RAGE_I)
        || m_pStats->HasFeat(CSWSCreatureStats::FEAT_WOOKIEE_RAGE_II)
        || m_pStats->HasFeat(CSWSCreatureStats::FEAT_WOOKIEE_RAGE_III))
        GetWookieeRage(list);

    CSWSCreature* pServer;
    unsigned int nForceItemMask;
    short nForcePoints;
    int nHasServer;
    if (GetServerObject() == 0) {
        nForceItemMask = 0;
        pServer = 0;
        nHasServer = 0;
        nForcePoints = 0;
    } else {
        pServer = GetServerObject()->AsSWSCreature();
        if (pServer == 0) {
            nForceItemMask = 0;
            nHasServer = 0;
            nForcePoints = 0;
        } else {
            nForceItemMask = pServer->GetForceItemMask();
            if (pServer->m_bBodyFuel != 0)
                nForcePoints = static_cast<short>(pServer->GetCurrentHitPoints(0));
            else
                nForcePoints = static_cast<short>(pServer->m_pStats->m_nCurrentForcePoints + pServer->m_pStats->m_nTemporaryForcePoints);
            nHasServer = 1;
        }
    }

    int nOldCount = list.m_count;
    int nRoomForceRating = 0;
#ifdef K2_FIX_ENGINE_BUGS
    CSWSArea* pArea = nHasServer != 0 ? static_cast<CSWSArea*>(pServer->GetArea()) : 0;
#else
    CSWSArea* pArea = static_cast<CSWSArea*>((GetServerObject() ? GetServerObject()->AsSWSCreature()
                                                    : static_cast<CSWSCreature*>(0))->GetArea());
#endif
    if (pArea != 0) {
        CSWSCreature* pTwin = GetServerObject() ? GetServerObject()->AsSWSCreature() : static_cast<CSWSCreature*>(0);
        K2_ASSUME(pTwin != 0);   // an area was found only through the server twin
        nRoomForceRating = pArea->GetRoomForceRating(pTwin->m_position);
    }

    int nForceBodyLevel = -1;
    short nCurrentHP = 0;
    int nStanceId = 0;
    char cCHAMod = 0;
    if (nHasServer != 0) {
        for (int64_t i = 0; i < pServer->m_effects.m_count; i++) {
            if (pServer->m_effects.m_pArray[i]->m_subType == CSWSEffectListHandler::EFFECT_FORCE_BODY) {
                nForceBodyLevel = pServer->m_effects.m_pArray[i]->GetInteger(0);
                break;
            }
        }
        nCurrentHP = pServer->GetCurrentHitPoints(0);
        nStanceId = pServer->m_stanceId;
        cCHAMod = pServer->m_pStats->m_nCHAMod;
    }

    m_pStats->GetMenuInfo(list, 3, nForcePoints, 0, nForceItemMask, 0.0001f, nRoomForceRating,
                          nForceBodyLevel, nCurrentHP, 0, nStanceId, 0, cCHAMod);
#endif

    for (int i = nOldCount; i < list.m_count; i++) {
        CSWGuiInterfaceAction& entry = list.m_pArray[i];
        entry.m_pfnAction = &CSWCCreature::ForceMenuAction;
        entry.m_nObjectId = m_objectId;
    }
}

// Builds the creature's radial-menu hostile force powers against pTarget. The exclude mask
// classifies the target by race; a droid-capable caster also gets the confusion entries, and a
// droid caster gets only the equipment powers. Powers beyond the target's range get the
// out-of-range flag.
void CSWCCreature::GetHostileForcePowers(CSWCObject* pTarget, CExoArrayList<CSWGuiInterfaceAction>& list)
{
    if (GetServerObject() == 0)
        return;
    CSWSCreature* pServer = GetServerObject()->AsSWSCreature();
    if (pServer == 0)
        return;
    if (static_cast<CSWSArea*>(pServer->GetArea())->m_nPlayerRestrictMode != 0)
        return;

#if KOTOR1
// K1: no droid confusion or subrace flag, and no room, Force Body, health, stance or level
// inputs.
    unsigned short nExcludeMask;
    if (pTarget->AsSWCCreature() != 0) {
        if (pTarget->AsSWCCreature()->m_pStats->m_nRace == CSWRace::CRACE_DROID)
            nExcludeMask = CSWRace::CRACE_DROID_MASK;
        else if (pTarget->AsSWCCreature()->m_pStats->m_nRace == CSWRace::CRACE_HUMAN)
            nExcludeMask = CSWRace::CRACE_HUMAN_MASK;
        else
            nExcludeMask = 0;
    } else {
        nExcludeMask = CSWRace::CRACE_PLACEABLE_MASK;
    }

    unsigned int nForceItemMask = pServer->GetForceItemMask();
    short nForcePoints;
    if (pServer->m_bBodyFuel != 0)
        nForcePoints = pServer->GetCurrentHitPoints(0);
    else
        nForcePoints = pServer->m_pStats->m_nCurrentForcePoints + pServer->m_pStats->m_nTemporaryForcePoints;

    int nOldCount = list.m_count;
    if (m_pStats->m_nRace == CSWRace::CRACE_DROID) {
        GetEquipmentForcePowers(list, 1, nExcludeMask, nForceItemMask);
    } else {
        float fDx = pTarget->m_position.x - m_position.x;
        float fDy = pTarget->m_position.y - m_position.y;
        float fDz = pTarget->m_position.z - m_position.z;
        m_pStats->GetMenuInfo(list, 1, nForcePoints, nExcludeMask, nForceItemMask,
                              fDx * fDx + fDy * fDy + fDz * fDz, 0, -1, 0, 0, 0, 0, 0);
    }
#else
    unsigned short nExcludeMask;
    int nTargetIsDroid = 0;
    if (pTarget->AsSWCCreature() != 0) {
        if (pTarget->AsSWCCreature()->m_pStats->m_nRace == CSWRace::CRACE_DROID) {
            nExcludeMask = CSWRace::CRACE_DROID_MASK;
        } else if (pTarget->AsSWCCreature()->m_pStats->m_nRace == CSWRace::CRACE_HUMAN) {
            nExcludeMask = CSWRace::CRACE_HUMAN_MASK;
            nTargetIsDroid = pTarget->AsSWCCreature()->m_pStats->m_nSubraceIndex == 2;
        } else {
            nExcludeMask = 0;
        }
    } else {
        nExcludeMask = CSWRace::CRACE_PLACEABLE_MASK;
    }

    unsigned int nForceItemMask = pServer->GetForceItemMask();
    short nForcePoints;
    if (pServer->m_bBodyFuel != 0)
        nForcePoints = pServer->GetCurrentHitPoints(0);
    else
        nForcePoints = pServer->m_pStats->m_nCurrentForcePoints + pServer->m_pStats->m_nTemporaryForcePoints;

    if ((m_pStats->HasFeat(CSWSCreatureStats::FEAT_DROID_TRICK)
         || m_pStats->HasFeat(CSWSCreatureStats::FEAT_DROID_CONFUSION)) &&
        (CSWRace::CRACE_DROID_MASK & nExcludeMask) != 0)
        GetDroidConfusion(list, pTarget);

    int nOldCount = list.m_count;
    if (m_pStats->m_nRace == CSWRace::CRACE_DROID) {
        GetEquipmentForcePowers(list, 1, nExcludeMask, nForceItemMask);
    } else {
        float fDx = pTarget->m_position.x - m_position.x;
        float fDy = pTarget->m_position.y - m_position.y;
        float fDz = pTarget->m_position.z - m_position.z;
        CSWSArea* pArea = static_cast<CSWSArea*>((GetServerObject() ? GetServerObject()->AsSWSCreature()
                                                        : static_cast<CSWSCreature*>(0))->GetArea());
        int nRoomForceRating = 0;
        if (pArea != 0) {
            CSWSCreature* pTwin = GetServerObject() ? GetServerObject()->AsSWSCreature() : static_cast<CSWSCreature*>(0);
            K2_ASSUME(pTwin != 0);   // the server twin was tested non-null on entry
            nRoomForceRating = pArea->GetRoomForceRating(pTwin->m_position);
        }

        int nForceBodyLevel = -1;
        for (int64_t i = 0; i < pServer->m_effects.m_count; i++) {
            if (pServer->m_effects.m_pArray[i]->m_subType == CSWSEffectListHandler::EFFECT_FORCE_BODY) {
                nForceBodyLevel = pServer->m_effects.m_pArray[i]->GetInteger(0);
                break;
            }
        }
        m_pStats->GetMenuInfo(list, 1, nForcePoints, nExcludeMask, nForceItemMask,
                              fDx * fDx + fDy * fDy + fDz * fDz, nRoomForceRating, nForceBodyLevel,
                              pServer->GetCurrentHitPoints(0), 0, pServer->m_stanceId,
                              nTargetIsDroid, pServer->m_pStats->m_nCHAMod);
    }
#endif

    for (int i = nOldCount; i < list.m_count; i++) {
        CSWGuiInterfaceAction& entry = list.m_pArray[i];
        entry.m_pfnAction = &CSWCCreature::ForceMenuHostileAction;
        entry.m_nObjectId = pTarget->m_objectId;
    }
}

int CSWCCreature::GetInterfaceTargetType(CSWCObject* pTarget)
{
    if (pTarget == 0)
        return INTERFACE_TARGET_NONE;
    if (pTarget->AsSWCCreature() != 0) {
        if (pTarget->AsSWCCreature()->IsHostile() != 0)
            return INTERFACE_TARGET_HOSTILE;
        return g_pAppManager->m_pClientExoApp->GetTargetChanging() != 0 ? INTERFACE_TARGET_HOSTILE
                                                                        : INTERFACE_TARGET_NONHOSTILE;
    }
    if (pTarget->AsSWCDoor() != 0)
        return INTERFACE_TARGET_OPENABLE;
    if (pTarget->AsSWCPlaceable() != 0) {
        CSWCPlaceable* pPlaceable = pTarget->AsSWCPlaceable();
        CSWSPlaceable* pServerPlaceable = 0;
        if (pPlaceable != 0)
            pServerPlaceable = pPlaceable->GetServerPlaceable();
        if (pServerPlaceable != 0 && pServerPlaceable->m_bHasInventory != 0)
            return INTERFACE_TARGET_OPENABLE;
        return INTERFACE_TARGET_NONHOSTILE;
    }
    return pTarget->AsSWCTrigger() != 0 ? INTERFACE_TARGET_TRIGGER : INTERFACE_TARGET_NONE;
}

// Adds one radial entry per mine item in the server inventory, with the localized name and its
// stack suffix, the set-mine callback, the item's object id as the action id and the item icon.
void CSWCCreature::GetMines(CExoArrayList<CSWGuiInterfaceAction>& list)
{
    if (GetServerObject() == 0)
        return;
    CSWSCreature* pServerCreature = GetServerObject()->AsSWSCreature();
    if (pServerCreature == 0)
        return;
    if (static_cast<CSWSArea*>(pServerCreature->GetArea())->m_nPlayerRestrictMode != 0)
        return;
    if (m_pStats->GetCanUseSkill(SKILL_DEMOLITIONS) == 0)
        return;

    CClientExoApp* pExoApp = g_pAppManager->m_pClientExoApp;
    int nOldCount = list.m_count;
    int nLanguage = pExoApp->GetClientLanguage();
    CItemRepository* pRepo = pServerCreature->GetItemRepository(1);
    CExoString sName;
#if !KOTOR1  // K1 lists the mine without a stack count
    CExoString sCount;
#endif
    CExoString sSuffix;
    sSuffix.Format(const_cast<char*>(" (%s)"), pExoApp->GetGUIString(STRREF_GROUND).CStr());

    // Declared outside the loop so their lifetime spans both invokes.
    CSWSItem* pItem;
    CSWBaseItem* pBaseItem;
    for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++) {
        pItem = pRepo->ItemListGetItem(i);
        if (pItem->m_bPlot != 0)
            continue;
        pBaseItem = static_cast<CSWItem*>(pItem)->GetBaseItem();
        if (pBaseItem->m_nItemType != ITEM_TYPE_TRAP_KIT)
            continue;
        pItem->m_localizedName.GetString(nLanguage, &sName, 0);
#if KOTOR1
        sName = sName + sSuffix;
#else
        if (pItem->m_stackCount >= 2)
            sCount.Format(const_cast<char*>(" (%d)"), pItem->m_stackCount);
        else
            sCount = const_cast<char*>("");
        sName = sName + sSuffix + sCount;
#endif
        list.SetSize(nOldCount + 1);
        list.m_pArray[nOldCount].m_sName = sName;
        list.m_pArray[nOldCount].m_pfnAction = &CSWCCreature::ActionMenuSetMine;
        list.m_pArray[nOldCount].m_nObjectId = m_objectId;
        list.m_pArray[nOldCount].m_nActionId = static_cast<unsigned int>(pItem->m_objectId) | CSWGuiInterfaceAction::ACTION_ID_ITEM;
        list.m_pArray[nOldCount].m_resRef = pItem->GetIcon();
#if KOTOR1  // K1 puts the stack count in the entry, not the label
        list.m_pArray[nOldCount].m_nCount = pItem->m_stackCount;
#endif
        nOldCount++;
    }
}

// Stamps the one Talk radial entry when the server twin allows dialog, then disables it while
// the player creature is dying and this creature is not party-interactable.
void CSWCCreature::GetFriendlyInterfaceActions(CSWCObject* pTarget, int nType,
                                               CExoArrayList<CSWGuiInterfaceAction>& list)
{
    K2_UNUSED(nType);
    int nOldCount = list.m_count;
#if KOTOR1  // K1 stamps the Talk entry unconditionally
    {
        {
#else
    if (GetServerObject() != 0) {
        CSWSCreature* pServer = GetServerObject()->AsSWSCreature();
        if (pServer != 0 && pServer->m_bConfused == 0) {
#endif
            list.SetSize(nOldCount + 1);
            list.m_pArray[nOldCount].m_sName =
                g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_DIALOG);
            list.m_pArray[nOldCount].m_pfnAction = &CSWCCreature::ActionInitiateDialog;
            list.m_pArray[nOldCount].m_nObjectId = m_objectId;
            list.m_pArray[nOldCount].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_TALK;
            list.m_pArray[nOldCount].m_resRef = const_cast<char*>("i_dialog");
        }
    }

    unsigned long long nPlayerId = g_pAppManager->m_pServerExoApp->GetPlayerCreatureId();
    CSWSPlayer* pPlayer = g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(nPlayerId);
    unsigned long long nClientId =
        g_pAppManager->m_pClientExoApp->ServerToClientObjectId(pPlayer->m_gameObjectId);
    CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(nClientId);
    CSWCCreature* pPlayerCreature;
    CSWSCreature* pServerCreature;
#if KOTOR1  // K1 keeps the flag block inline
    if ((
#else
    if (__builtin_expect(
#endif
            pObject != 0 && pObject->AsSWCCreature() != 0 &&
                (pPlayerCreature = static_cast<CSWCCreature*>(pObject->AsSWCCreature()),
                 pTarget->m_objectId != nClientId) &&
                pPlayerCreature->m_bInParty != 0 &&
                pPlayerCreature->GetServerCreature() != 0 &&
#if KOTOR1  // K1: the dying test is GetDeadTemp()
                (pServerCreature = pPlayerCreature->GetServerObject() != 0
                                       ? pPlayerCreature->GetServerObject()->AsSWSCreature()
                                       : 0,
                 pServerCreature->GetDeadTemp() != 0) &&
#else
                (pPlayerCreature->GetServerObject(),
                 pServerCreature = pPlayerCreature->GetServerObject()->AsSWSCreature(),
                 pServerCreature->GetDeadTemp() != 0) &&
#endif
                GetServerObject() != 0 &&
                GetServerObject()->AsSWSCreature() != 0 &&
#if KOTOR1
                (GetServerObject(), GetServerObject()->AsSWSCreature()->m_bPartyInteract == 0))) {
#else
                (GetServerObject(), GetServerObject()->AsSWSCreature()->m_bPartyInteract == 0),
            0)) {
#endif
        unsigned char& cFlags = list.m_pArray[nOldCount].m_nFlags;
        cFlags &= ~ACTION_FLAG_ENABLED;
        list.m_pArray[nOldCount].m_nFlags =
            (list.m_pArray[nOldCount].m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_PARTY_LEADER_UNAVAILABLE;
    }
}

void CSWCCreature::GetHostileInterfaceActions(CSWCObject* pTarget, int nType,
                                              CExoArrayList<CSWGuiInterfaceAction>& list)
{
    K2_UNUSED(pTarget); K2_UNUSED(nType);
    if (GetServerObject() == 0)
        return;
    CSWSCreature* pServer = GetServerObject()->AsSWSCreature();
    if (pServer == 0)
        return;
    if (static_cast<CSWSArea*>(pServer->GetArea())->m_nPlayerRestrictMode != 0)
        return;

    int nOldCount = list.m_count;
    list.SetSize(list.m_count + 1);
    list.m_pArray[nOldCount].m_sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ATTACK);
    list.m_pArray[nOldCount].m_pfnAction = &CSWCCreature::ActionMenuAttack;
    list.m_pArray[nOldCount].m_nObjectId = m_objectId;
    list.m_pArray[nOldCount].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_ATTACK;
    list.m_pArray[nOldCount].m_resRef = const_cast<char*>("i_attack");
}

// One hostile-item entry per item in our server inventory, gated on IsHostileUsableItem: the
// localized name, the target's object id, the item's object id as the action id, the item icon
// and the stack count.
void CSWCCreature::GetHostileItems(CExoArrayList<CSWGuiInterfaceAction>& list,
                                   CSWCCreature* pCreature) {
    CSWSCreature* pServerCreature = 0;
    if (GetServerObject())
        pServerCreature = GetServerObject()->AsSWSCreature();
    CItemRepository* pRepo = pServerCreature->GetItemRepository(1);
    if (pRepo->ItemListGetCount() != 0) {
        for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++)
            CreateHostileItemEntry(list, pRepo->ItemListGetItem(i), pCreature);
    }
}

void CSWCCreature::GetHostileItems(CExoArrayList<CSWGuiInterfaceAction>& list,
                                   CSWCPlaceable* pPlaceable) {
    CSWSCreature* pServerCreature = 0;
    if (GetServerObject())
        pServerCreature = GetServerObject()->AsSWSCreature();
    CItemRepository* pRepo = pServerCreature->GetItemRepository(1);
    if (pRepo->ItemListGetCount() != 0) {
        for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++)
            CreateHostileItemEntry(list, pRepo->ItemListGetItem(i), pPlaceable);
    }
}

void CSWCCreature::CreateHostileItemEntry(CExoArrayList<CSWGuiInterfaceAction>& list,
                                          CSWSItem* pItem, CSWCCreature* pCreature)
{
    int nOldCount = list.m_count;
    if (IsHostileUsableItem(pItem) != 0) {
        CExoString sName;
        int nLanguage = g_pAppManager->m_pClientExoApp->GetClientLanguage();
        pItem->m_localizedName.GetString(nLanguage, &sName, 0);
        list.SetSize(list.m_count + 1);
        list.m_pArray[nOldCount].m_sName = sName;
        list.m_pArray[nOldCount].m_nActionId =
            static_cast<unsigned int>(pItem->m_objectId) | CSWGuiInterfaceAction::ACTION_ID_ITEM;
        list.m_pArray[nOldCount].m_pfnAction =
            static_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCCreature::PartyMenuUseHostileItem);
        list.m_pArray[nOldCount].m_nObjectId = pCreature->m_objectId;
        list.m_pArray[nOldCount].m_resRef = pItem->GetIcon();
        list.m_pArray[nOldCount].m_nCount = pItem->m_stackCount;
    }
}

void CSWCCreature::CreateHostileItemEntry(CExoArrayList<CSWGuiInterfaceAction>& list,
                                          CSWSItem* pItem, CSWCPlaceable* pPlaceable)
{
    int nOldCount = list.m_count;
    if (IsHostileUsableItem(pItem) != 0) {
        CExoString sName;
        int nLanguage = g_pAppManager->m_pClientExoApp->GetClientLanguage();
        pItem->m_localizedName.GetString(nLanguage, &sName, 0);
        if (pItem->m_stackCount >= 2)
            sName.Format(const_cast<char*>("%s (%d)"), sName.CStr(), pItem->m_stackCount);
        list.SetSize(list.m_count + 1);
        list.m_pArray[nOldCount].m_sName = sName;
        list.m_pArray[nOldCount].m_nActionId =
            static_cast<unsigned int>(pItem->m_objectId) | CSWGuiInterfaceAction::ACTION_ID_ITEM;
        list.m_pArray[nOldCount].m_pfnAction =
            static_cast<void (CSWCCreature::*)(unsigned int, CSWCCreature*)>(&CSWCCreature::PartyMenuUseHostileItem);
        list.m_pArray[nOldCount].m_nObjectId = pPlaceable->m_objectId;
        list.m_pArray[nOldCount].m_resRef = pItem->GetIcon();
        list.m_pArray[nOldCount].m_nCount = pItem->m_stackCount;
    }
}

// Builds the target creature's radial-menu combat feats and skills. The equipped on-hand weapon
// picks the set: a ranged weapon gives three armed feat entries, otherwise four unarmed ones.
// Every entry takes the attack-feat dispatch callback and the target.
void CSWCCreature::GetCombatFeatsSkills(CExoArrayList<CSWGuiInterfaceAction>& list,
                                        CSWCCreature* pCreature)
{
    if (GetServerObject() == 0)
        return;
    CSWSCreature* pServer = GetServerObject()->AsSWSCreature();
    if (pServer == 0)
        return;
    if (static_cast<CSWSArea*>(pServer->GetArea())->m_nPlayerRestrictMode != 0)
        return;

    if (m_aEquippedItemIds[4] != OBJECT_INVALID) {
        CGameObject* pObject =
            g_pAppManager->m_pClientExoApp->GetGameObject(m_aEquippedItemIds[4]);
        if (pObject != 0 && pObject->AsSWCItem() != 0) {
            CSWBaseItem* pBaseItem =
                g_pRules->GetBaseItem(static_cast<CSWCItem*>(pObject->AsSWCItem())->m_baseItem);
            if (pBaseItem != 0 && pBaseItem->m_bRangedWeapon != 0) {
                EnableFeatForMenu(CheckFeat(FEAT_MASTER_POWER_BLAST, FEAT_IMPROVED_POWER_BLAST, FEAT_POWER_BLAST), list,
                                  reinterpret_cast<void (CSWCObject::*)(unsigned int, CSWCCreature*)>(
                                      &CSWCCreature::ActionMenuUseAttackFeat),
                                  pCreature);
                EnableFeatForMenu(CheckFeat(FEAT_MULTI_SHOT, FEAT_IMPROVED_RAPID_SHOT, FEAT_RAPID_SHOT), list,
                                  reinterpret_cast<void (CSWCObject::*)(unsigned int, CSWCCreature*)>(
                                      &CSWCCreature::ActionMenuUseAttackFeat),
                                  pCreature);
                EnableFeatForMenu(CheckFeat(FEAT_MASTER_SNIPER_SHOT, FEAT_IMPROVED_SNIPER_SHOT, FEAT_SNIPER_SHOT), list,
                                  reinterpret_cast<void (CSWCObject::*)(unsigned int, CSWCCreature*)>(
                                      &CSWCCreature::ActionMenuUseAttackFeat),
                                  pCreature);
                return;
            }
        }
    }
    EnableFeatForMenu(CheckFeat(FEAT_MASTER_CRITICAL_STRIKE, FEAT_IMPROVED_CRITICAL_STRIKE, FEAT_CRITICAL_STRIKE), list,
                      reinterpret_cast<void (CSWCObject::*)(unsigned int, CSWCCreature*)>(
                          &CSWCCreature::ActionMenuUseAttackFeat),
                      pCreature);
    EnableFeatForMenu(CheckFeat(FEAT_WHIRLWIND_ATTACK, FEAT_IMPROVED_FLURRY, FEAT_FLURRY), list,
                      reinterpret_cast<void (CSWCObject::*)(unsigned int, CSWCCreature*)>(
                          &CSWCCreature::ActionMenuUseAttackFeat),
                      pCreature);
    EnableFeatForMenu(CheckFeat(FEAT_MASTER_POWER_ATTACK, FEAT_IMPROVED_POWER_ATTACK, FEAT_POWER_ATTACK), list,
                      reinterpret_cast<void (CSWCObject::*)(unsigned int, CSWCCreature*)>(
                          &CSWCCreature::ActionMenuUseAttackFeat),
                      pCreature);
#if !KOTOR1  // K1 has no extra melee attack entry
    EnableFeatForMenu(CheckFeat(CSWSCreatureStats::FEAT_SHIELD_BREAKER, -1, -1), list,
                      reinterpret_cast<void (CSWCObject::*)(unsigned int, CSWCCreature*)>(
                          &CSWCCreature::ActionMenuUseAttackFeat),
                      pCreature);
#endif
}

// Builds the creature's single non-combat radial entry: stealth. Gated on the area allowing
// the menu and on a usable stealth skill, then on actually being able to stealth through an
// equipped stealth belt, the feat, or one of the three camouflage powers.
void CSWCCreature::GetNonCombatFeatsSkills(CExoArrayList<CSWGuiInterfaceAction>& list)
{
    if (GetServerObject() == 0)
        return;
    CSWSCreature* pServer = GetServerObject()->AsSWSCreature();
    if (pServer == 0)
        return;
    int nOldCount = list.m_count;
    CExoString sName;
    if (static_cast<CSWSArea*>(pServer->GetArea())->m_nPlayerRestrictMode == 0 &&
        m_pStats->GetCanUseSkill(SKILL_STEALTH) != 0) {
        CClientExoApp* pExoApp = g_pAppManager->m_pClientExoApp;
        bool bArmed;
        CSWCItem* pItemR = pExoApp->GetItemByGameObjectID(m_aEquippedItemIds[10]);
        if (pItemR == 0 || pItemR->GetBaseItem() == 0)
            bArmed = false;
        else
            bArmed = pItemR->GetBaseItem()->m_nItemType == ITEM_TYPE_STEALTH_UNIT;
        CSWCItem* pItemL = pExoApp->GetItemByGameObjectID(m_aEquippedItemIds[14]);
        if (pItemL != 0 && pItemL->GetBaseItem() != 0)
            bArmed |= pItemL->GetBaseItem()->m_nItemType == ITEM_TYPE_STEALTH_UNIT;
#if KOTOR1
        if (bArmed) {  // K1: the stealth belt alone
#else
        CSWSCreatureStats* pServerStats = pServer->m_pStats;
        int nHasFeat = pServerStats->HasFeat(FEAT_PERSONAL_CLOAKING_SHIELD);
        if (pServerStats->HasSpell(0, FORCE_POWER_FORCE_CAMOUFLAGE, 0) != 0 ||
            pServerStats->HasSpell(0, FORCE_POWER_IMPROVED_FORCE_CAMOUFLAGE, 0) != 0 ||
            (nHasFeat | static_cast<int>(bArmed) | pServerStats->HasSpell(0, FORCE_POWER_MASTER_FORCE_CAMOUFLAGE, 0)) != 0) {
#endif
            sName = pExoApp->GetGUIString(STRREF_STEALTH) + " (" + pExoApp->GetGUIString(STRREF_SELF) + ")";
            list.SetSize(nOldCount + 1);
            list.m_pArray[nOldCount].m_sName = sName;
            list.m_pArray[nOldCount].m_pfnAction = &CSWCCreature::PartyMenuStealth;
            list.m_pArray[nOldCount].m_nObjectId = m_objectId;
            list.m_pArray[nOldCount].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_STEALTH;
            list.m_pArray[nOldCount].m_resRef = g_pRules->m_paSkills[2].m_icon;
        }
    }
}

void CSWCCreature::GetTargetActions(CSWCObject* pTarget, int nType,
                                    CExoArrayList<CSWGuiInterfaceAction>& list)
{
    switch (GetInterfaceTargetType(pTarget)) {
    case INTERFACE_TARGET_OPENABLE:
        if (pTarget->AsSWCPlaceable() != 0) {
            pTarget->AsSWCPlaceable()->GetTargetActions(this, nType, list);
            return;
        }
        if (pTarget->AsSWCDoor() != 0) {
            pTarget->AsSWCDoor()->GetTargetActions(this, nType, list);
            return;
        }
        break;
    case INTERFACE_TARGET_TRIGGER:
        if (pTarget->AsSWCTrigger() != 0) {
            pTarget->AsSWCTrigger()->GetTargetActions(this, nType, list);
            return;
        }
        break;
    case INTERFACE_TARGET_NONHOSTILE:
#if KOTOR1  // K1: only a hostile placeable's force-power menu
        if (pTarget->AsSWCPlaceable() != 0) {
            if (nType == 1 && pTarget->AsSWCPlaceable()->IsHostile() != 0 &&
                pTarget->AsSWCPlaceable()->GetServerPlaceable()->GetReputation(
                    GetServerObject()->m_objectId, 0) <= 10) {
                GetHostileForcePowers(pTarget, list);
                return;
            }
            pTarget->AsSWCPlaceable()->GetTargetActions(this, nType, list);
            return;
        }
#else
        if (pTarget->AsSWCPlaceable() != 0 &&
            pTarget->AsSWCPlaceable()->GetServerPlaceable() != 0) {
            pTarget->AsSWCPlaceable()->IsHostile();
            pTarget->AsSWCPlaceable()->GetServerPlaceable()->GetReputation(
                GetServerObject()->m_objectId, 0);
            if (nType == 1 || nType == 2) {
                if (pTarget->AsSWCPlaceable()->IsHostile() != 0 &&
                    pTarget->AsSWCPlaceable()->GetServerPlaceable()->GetReputation(
                        GetServerObject()->m_objectId, 0) <= 10) {
                    if (nType != 1) {
                        if (nType != 2)
                            return;
                        CSWCPlaceable* pPlaceable = pTarget->AsSWCPlaceable();
                        CSWSCreature* pServerCreature = 0;
                        if (GetServerObject())
                            pServerCreature = GetServerObject()->AsSWSCreature();
                        CItemRepository* pRepo = pServerCreature->GetItemRepository(1);
                        if (pRepo->ItemListGetCount() != 0) {
                            for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++)
                                CreateHostileItemEntry(list, pRepo->ItemListGetItem(i), pPlaceable);
                        }
                        return;
                    }
                    GetHostileForcePowers(pTarget, list);
                    return;
                }
            }
            pTarget->AsSWCPlaceable()->GetTargetActions(this, nType, list);
            return;
        }
#endif
        break;
    case INTERFACE_TARGET_HOSTILE:
        if (nType != 2) {
            if (nType != 1) {
                if (nType != 0)
                    return;
                GetCombatFeatsSkills(list, pTarget->AsSWCCreature());
                pTarget->AsSWCCreature()->GetHostileInterfaceActions(static_cast<CSWCObject*>(this), nType, list);  // idiom-exempt: the header models these classes flat.
                return;
            }
            GetHostileForcePowers(pTarget, list);
            return;
        }
        {
            CSWCCreature* pCreature = pTarget->AsSWCCreature();
            CSWSCreature* pServerCreature = 0;
            if (GetServerObject())
                pServerCreature = GetServerObject()->AsSWSCreature();
            CItemRepository* pRepo = pServerCreature->GetItemRepository(1);
            if (pRepo->ItemListGetCount() != 0) {
                for (unsigned int i = 0; i < pRepo->ItemListGetCount(); i++)
                    CreateHostileItemEntry(list, pRepo->ItemListGetItem(i), pCreature);
            }
            return;
        }
    }
}

// Builds the creature's radial-menu behavior entries from the ai scripts 2DA: one entry per row
// whose ai state is not 10, labelled by that row's name strref. The persisted ai state is
// clamped to the valid set first, and the fixed action-id and resref pairs come from it.
#if !KOTOR1
void CSWCCreature::GetBehaviors(CExoArrayList<CSWGuiInterfaceAction>& list)
{
    C2DA* pAIScripts = g_pRules->m_p2DArrays->m_pAIScripts;
    GetServerObject();
    unsigned short nAIState = GetServerObject()->AsSWSCreature()->m_pStats->m_aiState;
    if (__builtin_expect((nAIState > AI_STATE_SUPPORT ||
                          ((1 << nAIState) & ((1 << AI_STATE_AGGRESSIVE) | (1 << AI_STATE_RANGED_SUPPORT) |
                                              (1 << AI_STATE_STATIONARY) | (1 << AI_STATE_SUPPORT))) == 0) != 0, 0)) {
        GetServerObject();
        GetServerObject()->AsSWSCreature()->m_pStats->m_aiState = 9;
    }

    int nIdx = list.m_count;
    int nNameStrRef = -1;
    int nRowAIState = -1;
    for (int i = 0; i < pAIScripts->m_nRows; i++) {
        pAIScripts->GetINTEntry(i, CExoString("NAME_STRREF"), &nNameStrRef);
        pAIScripts->GetINTEntry(i, CExoString("AISTATE"), &nRowAIState);
        if (nRowAIState == AI_STATE_DEFENSIVE)
            continue;
        if (i == 4) {
            if (__builtin_expect(CSWClass::IsJedi(m_pStats->GetClass(m_pStats->m_nNumClasses - 1)) == 0, 0))
                nNameStrRef = STRREF_GRENADIER;
        }
        list.SetSize(nIdx + 1);
        list.m_pArray[nIdx].m_sName =
            g_pAppManager->m_pClientExoApp->GetGUIString(nNameStrRef);
        list.m_pArray[nIdx].m_pfnAction = &CSWCCreature::ActionMenuSelectBehavior;
        list.m_pArray[nIdx].m_bChangesState = 1;
        list.m_pArray[nIdx].m_nObjectId = m_objectId;
        switch (nRowAIState) {
        case AI_STATE_AGGRESSIVE:
            list.m_pArray[nIdx].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_BEHAVIOR_AGGRESSIVE;
            list.m_pArray[nIdx].m_resRef = const_cast<char*>("ib_aggressive");
            break;
        case AI_STATE_STATIONARY:
            list.m_pArray[nIdx].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_BEHAVIOR_STATIONARY;
            list.m_pArray[nIdx].m_resRef = const_cast<char*>("ib_stationary");
            break;
        case AI_STATE_RANGED_SUPPORT:
            list.m_pArray[nIdx].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_BEHAVIOR_RANGED_SUPPORT;
            list.m_pArray[nIdx].m_resRef = const_cast<char*>("ib_ranged");
            break;
        case AI_STATE_SUPPORT:
            list.m_pArray[nIdx].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_BEHAVIOR_SUPPORT;
            if (CSWClass::IsJedi(m_pStats->GetClass(m_pStats->m_nNumClasses - 1)) != 0)
                list.m_pArray[nIdx].m_resRef = const_cast<char*>("ib_jedisupport");
            else
                list.m_pArray[nIdx].m_resRef = const_cast<char*>("ib_grenadier");
            break;
        }
        nIdx++;
    }
}
#endif

// Builds the droid caster's radial-menu equipment force powers. Scans the low equipment slots
// for items that grant a spell, skips excluded classes and fans each spell's sub-list out into
// key, uses and item triples, which are insertion-sorted and appended as menu entries with the
// spell name, the item's action id, its icon and the availability flags.
void CSWCCreature::GetEquipmentForcePowers(CExoArrayList<CSWGuiInterfaceAction>& list,
                                           unsigned int a, int nExcludeMask,
                                           unsigned int nForceItemMask)
{
    CExoArrayList<short> keys;
    CExoArrayList<short> uses;
    CExoArrayList<CSWCItem*> items;

    for (int64_t i = 0; i < 15; i++) {
        CSWCItem* pItem =
            g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[i]);
        if (pItem == 0)
            continue;
        CSWSpell* pSpell = g_pRules->GetSpell(pItem->GetActivePropertySpellID(0));
        if (pSpell == 0)
            continue;
        if ((pSpell->m_nExclusionMask & nExcludeMask) != 0)
            continue;
        int nSubCount = pSpell->m_nSubRadialCount;
        if (nSubCount == 0)
            continue;

        for (int j = 0; j < nSubCount; j++) {
            short nSubId = static_cast<short>(pSpell->GetSubRadialSpell(static_cast<unsigned char>(j)));
            int nUses = nSubId % 10;
            int nKey = nSubId / 10 - (nSubId / 1000) * 100;
            if (nSubId / 1000 != static_cast<int>(a))
                continue;

            int64_t pos;
            for (pos = 0; pos < items.m_count; pos++) {
                if (nKey == keys.m_pArray[pos]) {
                    if (nUses > uses.m_pArray[pos]) {
                        uses.m_pArray[pos] = static_cast<short>(nUses);
                        items.m_pArray[pos] = pItem;
                    }
                    break;
                }
                if (nKey < keys.m_pArray[pos]) {
                    uses.Insert(static_cast<short>(nUses), static_cast<int>(pos));
                    keys.Insert(static_cast<short>(nKey), static_cast<int>(pos));
                    items.Insert(pItem, static_cast<int>(pos));
                    break;
                }
            }
            if (pos == items.m_count) {
                items.Add(pItem);
                uses.Add(static_cast<short>(nUses));
                keys.Add(static_cast<short>(nKey));
            }
        }
    }

    int64_t nIdx = list.m_count;
    for (int64_t i = 0; i < items.m_count; i++) {
        CSWCItem* pItem = items.m_pArray[i];
        CSWSpell* pSpell = g_pRules->GetSpell(pItem->GetActivePropertySpellID(0));
        list.SetSize(static_cast<int>(nIdx + 1));
        list.m_pArray[nIdx].m_sName = pSpell->GetSpellNameText();
        list.m_pArray[nIdx].m_nActionId = static_cast<unsigned int>(pItem->m_objectId | CSWGuiInterfaceAction::ACTION_ID_SPELL);
        if (pSpell->m_iconResRef == const_cast<char*>(""))
            list.m_pArray[nIdx].m_resRef = pItem->m_iconResRef;
        else
            list.m_pArray[nIdx].m_resRef = pSpell->m_iconResRef;
        list.m_pArray[nIdx].m_nFlags &= ~ACTION_FLAG_REASON_MASK;
        int nEnabled = 1;
        CSWSItem* pServerItem = pItem->GetServerItem();
        if (pServerItem != 0) {
            CSWItemProperty* pProp = pServerItem->GetActiveProperty(0);
            if (pProp != 0 && pServerItem->GetUsedActivePropertyUsesLeft(0) == 0) {
                if (pProp->m_costTableValue != IPRP_CHARGECOST_0_CHARGES_PER_USE
                    && pProp->m_costTableValue != IPRP_CHARGECOST_UNLIMITED_USE) {
                    list.m_pArray[nIdx].m_nFlags =
                        (list.m_pArray[nIdx].m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_FORCE_DEPLETED;
                    nEnabled = 0;
                }
            }
        }
        if ((pSpell->m_nForbidItemMask & nForceItemMask) != 0) {
            list.m_pArray[nIdx].m_nFlags =
                (list.m_pArray[nIdx].m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_RESTRICTED_BY_ARMOR;
            nEnabled = 0;
        }
        if ((pSpell->m_nRequireItemMask & ~nForceItemMask) != 0) {
            list.m_pArray[nIdx].m_nFlags =
                (list.m_pArray[nIdx].m_nFlags & ~ACTION_FLAG_REASON_MASK) | ACTION_FLAG_REASON_MISSING_ITEM;
            nEnabled = 0;
        }
        list.m_pArray[nIdx].m_nFlags = static_cast<unsigned char>((list.m_pArray[nIdx].m_nFlags & ~1) | nEnabled);
        nIdx++;
    }
}


// The force-power callback. Stances forward to the form dispatch; otherwise a Jedi target goes
// through the tutorial window and falls back to casting the spell, a droid target gets an item use
// and anything else is dropped.
void CSWCCreature::ForceMenuAction(unsigned int n, CSWCCreature* p)
{
#if KOTOR1
// K1: no stances and no direct-cast ids; the Jedi classes are 3, 4 and 5.
    unsigned int nId = n & CSWGuiInterfaceAction::ACTION_ID_SPELL_CLEAR;
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg == 0)
        return;
    CSWCCreatureStats* pStats = p->m_pStats;
    unsigned char nClassSlot;
    if (pStats->GetClass(0) == CLASS_TYPE_JEDIGUARDIAN || pStats->GetClass(0) == CLASS_TYPE_JEDICONSULAR ||
        pStats->GetClass(0) == CLASS_TYPE_JEDISENTINEL) {
        nClassSlot = 0;
    } else if (pStats->GetClass(1) == CLASS_TYPE_JEDIGUARDIAN || pStats->GetClass(1) == CLASS_TYPE_JEDICONSULAR ||
               pStats->GetClass(1) == CLASS_TYPE_JEDISENTINEL) {
        nClassSlot = 1;
    } else {
        if (pStats->m_nRace == CSWRace::CRACE_DROID)
            pMsg->SendPlayerToServerInput_UseItem(nId, 0, 0, m_objectId, m_position);
        return;
    }
    int nOut;
    if (g_pRules->m_p2DArrays->m_pSpells->GetINTEntry(nId, CExoString("FORCEHOSTILE"), &nOut) == 0) {
        if (g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_USE_FRIENDLY_FORCE_POWER, m_objectId, nClassSlot, nId) != 0)
            return;
    } else if (g_pAppManager->m_pClientExoApp->ShowTutorialWindow(TUTORIAL_USE_HOSTILE_FORCE_POWER, m_objectId, nClassSlot, nId) != 0) {
        return;
    }
    unsigned long long nObjId = m_objectId;
    Vector vPos = {0.0f, 0.0f, 0.0f};
    CSWCMessage* pMsg2 = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg2 != 0)
        pMsg2->SendPlayerToServerInput_CastSpell(nId, nClassSlot, 0, 0, 0, vPos, nObjId);
#else
    unsigned int nId = n & CSWGuiInterfaceAction::ACTION_ID_SPELL_CLEAR;
    if (nId - FORM_SABER_I_SHII_CHO <= 10) {
        ActionMenuForm(nId, p);
        return;
    }
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg == 0)
        return;

    CSWCCreatureStats* pStats = p->m_pStats;
    int bJediClass;
    unsigned char nClassSlot;
    if (pStats->GetClass(0) == CLASS_TYPE_JEDIGUARDIAN || pStats->GetClass(0) == CLASS_TYPE_JEDICONSULAR ||
        pStats->GetClass(0) == CLASS_TYPE_JEDISENTINEL || pStats->GetClass(0) == CLASS_TYPE_JEDIWEAPONMASTER ||
        pStats->GetClass(0) == CLASS_TYPE_JEDIMASTER || pStats->GetClass(0) == CLASS_TYPE_JEDIWATCHMAN ||
        pStats->GetClass(0) == CLASS_TYPE_SITHASSASSIN || pStats->GetClass(0) == CLASS_TYPE_SITHMARAUDER ||
        pStats->GetClass(0) == CLASS_TYPE_SITHLORD) {
        nClassSlot = 0;
        bJediClass = 1;
    } else if (pStats->GetClass(1) == CLASS_TYPE_JEDIGUARDIAN || pStats->GetClass(1) == CLASS_TYPE_JEDICONSULAR ||
               pStats->GetClass(1) == CLASS_TYPE_JEDISENTINEL || pStats->GetClass(1) == CLASS_TYPE_JEDIWEAPONMASTER ||
               pStats->GetClass(1) == CLASS_TYPE_JEDIMASTER || pStats->GetClass(1) == CLASS_TYPE_JEDIWATCHMAN ||
               pStats->GetClass(1) == CLASS_TYPE_SITHASSASSIN || pStats->GetClass(1) == CLASS_TYPE_SITHMARAUDER ||
               pStats->GetClass(1) == CLASS_TYPE_SITHLORD) {
        nClassSlot = 1;
        bJediClass = 1;
    } else {
        nClassSlot = 2;
        bJediClass = 0;
    }

    if (nId - FORCE_POWER_WOOKIEE_RAGE_I < 3 || n == FORCE_POWER_DROID_TRICK ||
        n == FORCE_POWER_DROID_CONFUSION || n == (FORCE_POWER_DROID_TRICK | CSWGuiInterfaceAction::ACTION_ID_SPELL) ||
        n == (FORCE_POWER_DROID_CONFUSION | CSWGuiInterfaceAction::ACTION_ID_SPELL)) {
        unsigned long long nObjId = m_objectId;
        Vector vPos = {0.0f, 0.0f, 0.0f};
        CSWCMessage* pMsg2 = g_pAppManager->m_pClientExoApp->GetSWCMessage();
        if (pMsg2 == 0)
            return;
        pMsg2->SendPlayerToServerInput_CastSpell(nId, 0, 0, 0, 0, vPos, nObjId);
        return;
    }

    if (bJediClass == 0) {
        if (pStats->m_nRace == CSWRace::CRACE_DROID)
            pMsg->SendPlayerToServerInput_UseItem(nId, 0, 0, m_objectId, m_position);
        return;
    }

    int nOut;
    int bForceHostile = g_pRules->m_p2DArrays->m_pSpells->GetINTEntry(
        nId, CExoString("FORCEHOSTILE"), &nOut);
    unsigned char nWindow = TUTORIAL_USE_FRIENDLY_FORCE_POWER;
    if (bForceHostile != 0)
        nWindow = TUTORIAL_USE_HOSTILE_FORCE_POWER;
    if (g_pAppManager->m_pClientExoApp->ShowTutorialWindow(nWindow, m_objectId,
                                                           nClassSlot, nId) == 0) {
        unsigned long long nObjId = m_objectId;
        Vector vPos = {0.0f, 0.0f, 0.0f};
        CSWCMessage* pMsg2 = g_pAppManager->m_pClientExoApp->GetSWCMessage();
        if (pMsg2 != 0)
            pMsg2->SendPlayerToServerInput_CastSpell(nId, nClassSlot, 0, 0, 0, vPos, nObjId);
    }
#endif
}

#if !KOTOR1
__attribute__((noinline))
void CSWCCreature::ActionMenuForm(unsigned int n, CSWCCreature* p)
{
    K2_UNUSED(p);
    if (GetServerObject() == 0)
        return;
    CSWSCreature* pServer = GetServerObject()->AsSWSCreature();
    if (pServer == 0)
        return;
    pServer->m_stanceId = n;
    GetAnimationBase()->m_stanceId = n;
    if (n - FORM_SABER_I_SHII_CHO > FORM_FORCE_IV_MASTERY - FORM_SABER_I_SHII_CHO)
        return;
    n += STRREF_FORM_SHII_CHO_INITIATED - FORM_SABER_I_SHII_CHO;
    CSWGuiMainInterface* pMain = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface;
    pMain->m_feedbackTimer = 4.0f;
    pMain->m_feedbackStrRef = n;
}
#endif

// Sets the combat-mode bit from a, then mirrors it into the client app when this creature is
// the player creature.
void CSWCCreature::SetCombatMode(int a)
{
    m_nStateFlags = (m_nStateFlags & ~1) | (a & 1);
    if (m_objectId == g_pAppManager->m_pClientExoApp->GetPlayerCreatureId())
        g_pAppManager->m_pClientExoApp->SetCombatMode(m_nStateFlags & 1);
}

// The hostile force-power callback forces the target into combat mode, then runs the normal
// force-menu dispatch.
void CSWCCreature::ForceMenuHostileAction(unsigned int n, CSWCCreature* p)
{
    if (p != 0)
        p->SetCombatMode(1);
    ForceMenuAction(n, p);
}

// Builds the implant-swap radial entries: reads the current-implant global, clamps it, and
// stamps one entry per implant with a GUI strref name, a swap callback, a spell-like action id
// and an item icon.
#if !KOTOR1
void CSWCCreature::GetSwappableImplants(CExoArrayList<CSWGuiInterfaceAction>& list)
{
    CExoString sImplantVar("CANDEROUS_CUR_IMPLANT");
    char cCurImplant = 0;
    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->GetValueNumber(sImplantVar,
                                                                             cCurImplant);
    if (cCurImplant < 1 || cCurImplant > 4) {
        cCurImplant = 1;
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(sImplantVar,
                                                                                 cCurImplant);
    }

    int nOldCount = list.m_count;

    list.SetSize(nOldCount + 1);
    list.m_pArray[nOldCount].m_sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_REGENERATION_IMPLANT);
    list.m_pArray[nOldCount].m_pfnAction = &CSWCCreature::ActionMenuSwapImplantRegen;
    list.m_pArray[nOldCount].m_nObjectId = m_objectId;
    list.m_pArray[nOldCount].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_IMPLANT_REGENERATION;
    list.m_pArray[nOldCount].m_resRef = const_cast<char*>("ii_implant1_001");
    list.m_pArray[nOldCount].m_bChangesState = 1;

    list.SetSize(nOldCount + 2);
    list.m_pArray[nOldCount + 1].m_sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_STRENGTH_IMPLANT);
    list.m_pArray[nOldCount + 1].m_pfnAction = &CSWCCreature::ActionMenuSwapImplantStr;
    list.m_pArray[nOldCount + 1].m_nObjectId = m_objectId;
    list.m_pArray[nOldCount + 1].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_IMPLANT_STRENGTH;
    list.m_pArray[nOldCount + 1].m_resRef = const_cast<char*>("ii_implant3_003");
    list.m_pArray[nOldCount + 1].m_bChangesState = 1;

    list.SetSize(nOldCount + 3);
    list.m_pArray[nOldCount + 2].m_sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ALACRITY_IMPLANT);
    list.m_pArray[nOldCount + 2].m_pfnAction = &CSWCCreature::ActionMenuSwapImplantAgi;
    list.m_pArray[nOldCount + 2].m_nObjectId = m_objectId;
    list.m_pArray[nOldCount + 2].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_IMPLANT_ALACRITY;
    list.m_pArray[nOldCount + 2].m_resRef = const_cast<char*>("ii_implant2_002");
    list.m_pArray[nOldCount + 2].m_bChangesState = 1;

    list.SetSize(nOldCount + 4);
    list.m_pArray[nOldCount + 3].m_sName = g_pAppManager->m_pClientExoApp->GetGUIString(STRREF_ENDURANCE_IMPLANT);
    list.m_pArray[nOldCount + 3].m_pfnAction = &CSWCCreature::ActionMenuSwapImplantEnd;
    list.m_pArray[nOldCount + 3].m_nObjectId = m_objectId;
    list.m_pArray[nOldCount + 3].m_nActionId = CSWGuiInterfaceAction::ACTION_ID_IMPLANT_ENDURANCE;
    list.m_pArray[nOldCount + 3].m_resRef = const_cast<char*>("ii_implant3_004");
    list.m_pArray[nOldCount + 3].m_bChangesState = 1;
}
#endif

// Builds the wookiee-rage radial entry: the highest owned rage tier, its feat name, the
// force-menu dispatch callback, a rage spell id and the feat icon.
void CSWCCreature::GetWookieeRage(CExoArrayList<CSWGuiInterfaceAction>& list)
{
    int nOldCount = list.m_count;
    unsigned int nSpellId = CSWGuiInterfaceAction::ACTION_ID_SPELL | FORCE_POWER_WOOKIEE_RAGE_I;
    unsigned short nFeatId;
    if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_WOOKIEE_RAGE_III)) {
        nFeatId = CSWSCreatureStats::FEAT_WOOKIEE_RAGE_III;
        nSpellId += 2;
    } else if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_WOOKIEE_RAGE_II)) {
        nFeatId = CSWSCreatureStats::FEAT_WOOKIEE_RAGE_II;
        nSpellId += 1;
    } else if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_WOOKIEE_RAGE_I)) {
        nFeatId = CSWSCreatureStats::FEAT_WOOKIEE_RAGE_I;
    } else {
        return;
    }
    CSWFeat* pFeat = g_pRules->GetFeat(nFeatId);
    if (pFeat == 0)
        return;
    list.SetSize(nOldCount + 1);
    list.m_pArray[nOldCount].m_sName = pFeat->GetNameText();
    list.m_pArray[nOldCount].m_pfnAction = &CSWCCreature::ForceMenuAction;
    list.m_pArray[nOldCount].m_nObjectId = m_objectId;
    list.m_pArray[nOldCount].m_nActionId = nSpellId;
    list.m_pArray[nOldCount].m_resRef = pFeat->m_icon;
    unsigned char& cFlags = list.m_pArray[nOldCount].m_nFlags;
    cFlags &= ~ACTION_FLAG_REASON_MASK;
    list.m_pArray[nOldCount].m_nFlags |= 1;
}

#if !KOTOR1  // K2-only (no K1 counterpart)
// The implant-swap callbacks each hand ActionSwapImplant a hardcoded implant id.
void CSWCCreature::ActionMenuSwapImplantRegen(unsigned int n, CSWCCreature* p)
{
    K2_UNUSED(n); K2_UNUSED(p);
    ActionSwapImplant(1032);
}

void CSWCCreature::ActionMenuSwapImplantStr(unsigned int n, CSWCCreature* p)
{
    K2_UNUSED(n); K2_UNUSED(p);
    ActionSwapImplant(1033);
}

void CSWCCreature::ActionMenuSwapImplantAgi(unsigned int n, CSWCCreature* p)
{
    K2_UNUSED(n); K2_UNUSED(p);
    ActionSwapImplant(1035);
}

void CSWCCreature::ActionMenuSwapImplantEnd(unsigned int n, CSWCCreature* p)
{
    K2_UNUSED(n); K2_UNUSED(p);
    ActionSwapImplant(1034);
}
#endif // !KOTOR1

// Builds the droid-confusion radial entry: the master feat wins over the lesser one unless the
// global-variable lookup fails, and the winning feat picks the spell id with the spell-id bit
// set.
void CSWCCreature::GetDroidConfusion(CExoArrayList<CSWGuiInterfaceAction>& list, CSWCObject* pTarget)
{
    int nOldCount = list.m_count;
    char nDroidConfActive;
    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->GetValueNumber(CExoString("000_Droid_Conf_Active"), nDroidConfActive);

    unsigned int nSpellId = FORCE_POWER_DROID_TRICK | CSWGuiInterfaceAction::ACTION_ID_SPELL;
    unsigned short nFeat;
    if (m_pStats->HasFeat(CSWSCreatureStats::FEAT_DROID_CONFUSION) && !nDroidConfActive) {
        nFeat = CSWSCreatureStats::FEAT_DROID_CONFUSION;
        nSpellId = FORCE_POWER_DROID_CONFUSION | CSWGuiInterfaceAction::ACTION_ID_SPELL;
    } else {
        if (!m_pStats->HasFeat(CSWSCreatureStats::FEAT_DROID_TRICK))
            return;
        nFeat = CSWSCreatureStats::FEAT_DROID_TRICK;
    }

    CSWFeat* pFeat = g_pRules->GetFeat(nFeat);
    if (pFeat == 0)
        return;

    list.SetSize(nOldCount + 1);
    list.m_pArray[nOldCount].m_sName = pFeat->GetNameText();
    list.m_pArray[nOldCount].m_pfnAction = &CSWCCreature::ForceMenuAction;
    list.m_pArray[nOldCount].m_nObjectId = pTarget->m_objectId;
    list.m_pArray[nOldCount].m_nActionId = nSpellId;
    list.m_pArray[nOldCount].m_resRef = pFeat->m_icon;
    unsigned char& cFlags = list.m_pArray[nOldCount].m_nFlags;
    cFlags &= ~ACTION_FLAG_REASON_MASK;
    list.m_pArray[nOldCount].m_nFlags |= 1;
}

// Moves the current-implant global to the previous one, maps the id to the new implant and its
// feedback strref, writes the global back, fires the swap script and arms the main interface's
// feedback.
#if !KOTOR1
void CSWCCreature::ActionSwapImplant(int nImplantId)
{
    CExoString sCurImplant("CANDEROUS_CUR_IMPLANT");
    CExoString sPrevImplant("CANDEROUS_P_IMPLANT");
    char cCurImplant = 0;
    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->GetValueNumber(sCurImplant,
                                                                             cCurImplant);
    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(sPrevImplant,
                                                                             cCurImplant);

    char cNewImplant = 1;
    unsigned int nStrRef = STRREF_REGENERATION_IMPLANT_ACTIVATED;
    switch (nImplantId) {
    case CSWGuiInterfaceAction::ACTION_ID_IMPLANT_REGENERATION:
        break;
    case CSWGuiInterfaceAction::ACTION_ID_IMPLANT_STRENGTH:
        cNewImplant = 2;
        nStrRef = STRREF_STRENGTH_IMPLANT_ACTIVATED;
        break;
    case CSWGuiInterfaceAction::ACTION_ID_IMPLANT_ENDURANCE:
        cNewImplant = 3;
        nStrRef = STRREF_ENDURANCE_IMPLANT_ACTIVATED;
        break;
    case CSWGuiInterfaceAction::ACTION_ID_IMPLANT_ALACRITY:
        cNewImplant = 4;
        nStrRef = STRREF_ALACRITY_IMPLANT_ACTIVATED;
        break;
    default:
        return;
    }

    g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueNumber(sCurImplant,
                                                                             cNewImplant);
    CExoString sScript("a_swapimplant");
    CVirtualMachine* pVM = g_pVirtualMachine;
    CSWGuiMainInterface* pMain;
    static_cast<CSWCObject*>(this)->GetServerObject();  // idiom-exempt: the header models these classes flat.
    pVM->RunScript(&sScript,
                   static_cast<CSWCObject*>(this)->GetServerObject()->AsSWSCreature()->m_objectId,  // idiom-exempt: the header models these classes flat.
                   1);
    pMain = g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface;
    pMain->m_feedbackTimer = 4.0f;
    pMain->m_feedbackStrRef = nStrRef;
}
#endif

// Writes the character-generation GFF for a player-created creature: the name split into first
// and last at the last space, gold and components, the stats block's identity and appearance
// fields, the class and level lists with their sub-lists, the inventory and the default
// henchman scripts.
void CSWCCreature::SaveCharGenCreature()
{
    CExoLocString locStr;
    CResGFF* pGFF = new CResGFF;
    CResStruct topStruct;
    CResList classList;
    CResList skillList;
    CResList featList;
    CResList lvlStatList;
    CResStruct classElem;
    CResStruct skillElem;
    CResStruct featElem;
    CResStruct lvlStatElem;
    pGFF->CreateGFFFile(&topStruct, "BIC ", "V2.0");

#if KOTOR1
    locStr.AddString(0, m_pStats->m_name, 0);
    pGFF->WriteFieldCExoLocString(&topStruct, locStr, const_cast<char*>("FirstName"));
    locStr.AddString(0, m_pStats->m_sLastName, 0);
    pGFF->WriteFieldCExoLocString(&topStruct, locStr, const_cast<char*>("LastName"));
#else
    CExoString sName = m_pStats->GetFirstName() + " " + m_pStats->GetLastName();
    while (true) {
        if (sName.IsEmpty())
            break;
        if (sName[sName.GetLength() - 1] != ' ')
            break;
        sName = sName.SubString(0, sName.GetLength() - 1);
    }
    locStr.AddString(0, sName, 0);
    pGFF->WriteFieldCExoLocString(&topStruct, locStr, const_cast<char*>("FirstName"));
    sName = "";
    locStr.AddString(0, sName, 0);
    pGFF->WriteFieldCExoLocString(&topStruct, locStr, const_cast<char*>("LastName"));
#endif
    locStr.AddString(0, m_pStats->m_sDescription, 0);
    pGFF->WriteFieldCExoLocString(&topStruct, locStr, const_cast<char*>("Description"));

    pGFF->WriteFieldDWORD(&topStruct, m_nGold, const_cast<char*>("Gold"));
#if !KOTOR1
    pGFF->WriteFieldINT(&topStruct, 1, const_cast<char*>("PlayerCreated"));
    pGFF->WriteFieldDWORD(&topStruct, m_nItemComponent, const_cast<char*>("ItemComponent"));
    pGFF->WriteFieldDWORD(&topStruct, m_nChemicals, const_cast<char*>("Chemicals"));
#endif
    CResRef conversation = m_pStats->m_conversation;
    pGFF->WriteFieldCResRef(&topStruct, conversation, const_cast<char*>("Conversation"));
    pGFF->WriteFieldINT(&topStruct, m_pStats->m_nAge, const_cast<char*>("Age"));
    pGFF->WriteFieldBYTE(&topStruct, m_pStats->m_nGender, const_cast<char*>("Gender"));
    pGFF->WriteFieldBYTE(&topStruct, static_cast<unsigned char>(m_pStats->m_nRace), const_cast<char*>("Race"));
    pGFF->WriteFieldCExoString(&topStruct, m_pStats->m_raceString, const_cast<char*>("Subrace"));
    pGFF->WriteFieldBYTE(&topStruct, m_pStats->m_nSubraceIndex, const_cast<char*>("SubraceIndex"));
    pGFF->WriteFieldBYTE(&topStruct, m_pStats->m_nStartingPackage, const_cast<char*>("StartingPackage"));
    pGFF->WriteFieldCExoString(&topStruct, m_pStats->m_sDeity, const_cast<char*>("Deity"));
    pGFF->WriteFieldSHORT(&topStruct, static_cast<short>(m_pStats->GetArmorClass(1)), const_cast<char*>("ArmorClass"));
    pGFF->WriteFieldCHAR(&topStruct, m_pStats->m_nReflexSavingThrow, const_cast<char*>("RefSaveThrow"));
    pGFF->WriteFieldCHAR(&topStruct, m_pStats->m_nWillSavingThrow, const_cast<char*>("WillSaveThrow"));
    pGFF->WriteFieldCHAR(&topStruct, m_pStats->m_nFortSavingThrow, const_cast<char*>("FortSaveThrow"));
    pGFF->WriteFieldBYTE(&topStruct, m_pStats->m_nNumClasses - 1, const_cast<char*>("MClassLevUpIn"));
    pGFF->WriteFieldWORD(&topStruct, m_nSoundSetFile, const_cast<char*>("SoundSetFile"));

    {
        pGFF->AddList(&classList, &topStruct, const_cast<char*>("ClassList"));
        pGFF->AddListElement(&classElem, &classList, 0);
        pGFF->WriteFieldSHORT(&classElem, m_pStats->GetClassLevel(0), const_cast<char*>("ClassLevel"));
        pGFF->WriteFieldINT(&classElem, m_pStats->GetClass(0), const_cast<char*>("Class"));
        CResList knownList;
        if (m_pStats->GetNumberKnownSpells(0, 0) != 0) {
            CExoString label;
            label.Format(const_cast<char*>("%s%d"), const_cast<char*>("KnownList"), 0);
            pGFF->AddList(&knownList, &classElem, static_cast<char*>(label.CStr()));
        }
        for (unsigned char nSpell = 0; nSpell < m_pStats->GetNumberKnownSpells(0, 0); nSpell++) {
            CResStruct spellElem;
            pGFF->AddListElement(&spellElem, &knownList, 0);
            pGFF->WriteFieldWORD(&spellElem, static_cast<unsigned short>(m_pStats->GetKnownSpell(0, 0, nSpell)), const_cast<char*>("Spell"));
        }
    }

    pGFF->WriteFieldWORD(&topStruct, m_pStats->m_nSkillPointsRemaining, const_cast<char*>("SkillPoints"));
    {
        pGFF->AddList(&skillList, &topStruct, const_cast<char*>("SkillList"));
        if (g_pRules->m_nNumSkills != 0) {
            unsigned short i = 0;
            do {
                pGFF->AddListElement(&skillElem, &skillList, 0);
                unsigned char nRank = i < g_pRules->m_nNumSkills ? m_pStats->m_pSkillRanksBase[i] : 0;
                pGFF->WriteFieldBYTE(&skillElem, nRank, const_cast<char*>("Rank"));
                i++;
            } while (i < g_pRules->m_nNumSkills);
        }
    }

    {
        pGFF->AddList(&featList, &topStruct, const_cast<char*>("FeatList"));
        if (m_pStats->m_lstFeats.m_count != 0) {
            unsigned short i = 0;
            do {
                unsigned short nFeat = m_pStats->GetFeat(i);
                pGFF->AddListElement(&featElem, &featList, 0);
                pGFF->WriteFieldWORD(&featElem, nFeat, const_cast<char*>("Feat"));
                i++;
            } while (i < static_cast<unsigned short>(m_pStats->m_lstFeats.m_count));
        }
    }

    CSWRace* pRace = &g_pRules->m_paRaces[m_pStats->m_nRace];
    pGFF->WriteFieldBYTE(&topStruct, static_cast<unsigned char>(m_pStats->m_abScores[0] - pRace->m_anAbilityAdjust[0]), const_cast<char*>("Str"));
    pGFF->WriteFieldBYTE(&topStruct, static_cast<unsigned char>(m_pStats->m_abScores[1] - pRace->m_anAbilityAdjust[1]), const_cast<char*>("Dex"));
    pGFF->WriteFieldBYTE(&topStruct, static_cast<unsigned char>(m_pStats->m_abScores[2] - pRace->m_anAbilityAdjust[5]), const_cast<char*>("Con"));
    pGFF->WriteFieldBYTE(&topStruct, static_cast<unsigned char>(m_pStats->m_abScores[3] - pRace->m_anAbilityAdjust[2]), const_cast<char*>("Int"));
    pGFF->WriteFieldBYTE(&topStruct, static_cast<unsigned char>(m_pStats->m_abScores[4] - pRace->m_anAbilityAdjust[4]), const_cast<char*>("Wis"));
    pGFF->WriteFieldBYTE(&topStruct, static_cast<unsigned char>(m_pStats->m_abScores[5] - pRace->m_anAbilityAdjust[3]), const_cast<char*>("Cha"));

    pGFF->WriteFieldSHORT(&topStruct, static_cast<short>((m_pStats->m_nHitPointsBase - m_pStats->GetMaxHitPoints()) + m_pStats->m_nCurrentHitPoints),
                          const_cast<char*>("CurrentHitPoints"));
    pGFF->WriteFieldSHORT(&topStruct, m_pStats->m_nHitPointsBase, const_cast<char*>("HitPoints"));
    pGFF->WriteFieldSHORT(&topStruct, static_cast<short>(m_pStats->GetMaxHitPoints()), const_cast<char*>("MaxHitPoints"));
    pGFF->WriteFieldSHORT(&topStruct, m_pStats->m_nCurrentHitPoints, const_cast<char*>("PregameCurrent"));
    pGFF->WriteFieldSHORT(&topStruct, (m_pStats->m_nForcePoints - m_pStats->m_nMaxForcePoints) + m_pStats->m_nCurrentForcePoints + m_pStats->m_nTemporaryForcePoints,
                          const_cast<char*>("CurrentForce"));
    pGFF->WriteFieldSHORT(&topStruct, static_cast<short>(m_pStats->m_nForcePoints), const_cast<char*>("ForcePoints"));
    pGFF->WriteFieldSHORT(&topStruct, static_cast<short>(m_pStats->m_nMaxForcePoints), const_cast<char*>("MaxForcePoints"));

    pGFF->AddList(&lvlStatList, &topStruct, const_cast<char*>("LvlStatList"));
    pGFF->AddListElement(&lvlStatElem, &lvlStatList, 0);
    pGFF->WriteFieldBYTE(&lvlStatElem, g_pRules->m_paClasses[m_pStats->GetClass(0)].m_nHitDie, const_cast<char*>("LvlStatHitDie"));
    pGFF->WriteFieldBYTE(&lvlStatElem, m_pStats->GetClass(0), const_cast<char*>("LvlStatClass"));
    {
        CResList lvlKnownList;
        if (m_pStats->GetNumberKnownSpells(0, 0) != 0) {
            CExoString label;
            label.Format(const_cast<char*>("%s%d"), const_cast<char*>("KnownList"), 0);
            pGFF->AddList(&lvlKnownList, &lvlStatElem, static_cast<char*>(label.CStr()));
        }
        for (unsigned char nSpell = 0; nSpell < m_pStats->GetNumberKnownSpells(0, 0); nSpell++) {
            CResStruct spellElem;
            pGFF->AddListElement(&spellElem, &lvlKnownList, 0);
            pGFF->WriteFieldWORD(&spellElem, static_cast<unsigned short>(m_pStats->GetKnownSpell(0, 0, nSpell)), const_cast<char*>("Spell"));
        }
    }

    pGFF->WriteFieldWORD(&lvlStatElem, m_pStats->m_nSkillPointsRemaining, const_cast<char*>("SkillPoints"));
    {
        pGFF->AddList(&skillList, &lvlStatElem, const_cast<char*>("SkillList"));
        if (g_pRules->m_nNumSkills != 0) {
            unsigned short i = 0;
            do {
                pGFF->AddListElement(&skillElem, &skillList, 0);
                unsigned char nRank = i < g_pRules->m_nNumSkills ? m_pStats->m_pSkillRanksBase[i] : 0;
                pGFF->WriteFieldBYTE(&skillElem, nRank, const_cast<char*>("Rank"));
                i++;
            } while (i < g_pRules->m_nNumSkills);
        }
    }

    {
        pGFF->AddList(&featList, &lvlStatElem, const_cast<char*>("FeatList"));
        if (m_pStats->m_lstFeats.m_count != 0) {
            unsigned short i = 0;
            do {
                unsigned short nFeat = m_pStats->GetFeat(i);
                pGFF->AddListElement(&featElem, &featList, 0);
                pGFF->WriteFieldWORD(&featElem, nFeat, const_cast<char*>("Feat"));
                i++;
            } while (i < static_cast<unsigned short>(m_pStats->m_lstFeats.m_count));
        }
    }

    pGFF->WriteFieldDWORD(&topStruct, m_pStats->m_nExperience, const_cast<char*>("Experience"));
    pGFF->WriteFieldWORD(&topStruct, m_pStats->m_portraitId, const_cast<char*>("PortraitId"));
    CResRef portrait = m_pStats->GetPortrait(0);
    pGFF->WriteFieldCResRef(&topStruct, portrait, const_cast<char*>("Portrait"));
    pGFF->WriteFieldBYTE(&topStruct, static_cast<unsigned char>(m_pStats->m_alignmentGoodEvil), const_cast<char*>("GoodEvil"));
#if !KOTOR1
    pGFF->WriteFieldCHAR(&topStruct, static_cast<char>(m_pStats->m_baseCNPCAlignment), const_cast<char*>("BaseCNPCAlignment"));
#endif
    pGFF->WriteFieldBYTE(&topStruct, m_pStats->m_nColorSkin, const_cast<char*>("Color_Skin"));
    pGFF->WriteFieldBYTE(&topStruct, m_pStats->m_nColorHair, const_cast<char*>("Color_Hair"));
    pGFF->WriteFieldBYTE(&topStruct, m_pStats->m_nColorTattoo1, const_cast<char*>("Color_Tattoo1"));
    pGFF->WriteFieldBYTE(&topStruct, m_pStats->m_nColorTattoo2, const_cast<char*>("Color_Tattoo2"));
    pGFF->WriteFieldINT(&topStruct, m_pStats->m_nPhenotype, const_cast<char*>("Phenotype"));
    pGFF->WriteFieldWORD(&topStruct, m_pStats->m_nAppearanceType, const_cast<char*>("Appearance_Type"));
    pGFF->WriteFieldBYTE(&topStruct, m_pStats->m_nAppearanceHead, const_cast<char*>("Appearance_Head"));
    pGFF->WriteFieldBYTE(&topStruct, m_pStats->m_bDuplicatingHead, const_cast<char*>("DuplicatingHead"));
    pGFF->WriteFieldBYTE(&topStruct, static_cast<unsigned char>(m_pStats->m_nUseBackupHead), const_cast<char*>("UseBackupHead"));
    pGFF->WriteFieldBYTE(&topStruct, m_pStats->m_bTail, const_cast<char*>("Tail"));
    pGFF->WriteFieldBYTE(&topStruct, m_pStats->m_bWings, const_cast<char*>("Wings"));

    // Allocates less than sizeof(CResList), as the engine does.
    CResList* pItemList = new CResList();
    pGFF->AddList(pItemList, &topStruct, const_cast<char*>("ItemList"));
    // A player-created creature's stats block is really a CSWCLevelUpStats.
    CSWCLevelUpStats* pLevelUpStats = static_cast<CSWCLevelUpStats*>(m_pStats);
    if (static_cast<unsigned char>(pLevelUpStats->m_lstPackageResRefs.m_count) != 0) {
        uint64_t i = 0;
        do {
            CResStruct itemElem;
            pGFF->AddListElement(&itemElem, pItemList, 0);
            pGFF->WriteFieldCResRef(&itemElem, pLevelUpStats->m_lstPackageResRefs.m_pArray[i],
                                    const_cast<char*>("InventoryRes"));
            i++;
        } while (i < static_cast<unsigned char>(pLevelUpStats->m_lstPackageResRefs.m_count));
    }
    delete pItemList;
    CExoString sScript;
    sScript = "k_hen_heartbt01";
    pGFF->WriteFieldCResRef(&topStruct, CResRef(sScript), const_cast<char*>("ScriptHeartbeat"));
    sScript = "k_hen_percept01";
    pGFF->WriteFieldCResRef(&topStruct, CResRef(sScript), const_cast<char*>("ScriptOnNotice"));
    sScript = "k_def_spellat01";
    pGFF->WriteFieldCResRef(&topStruct, CResRef(sScript), const_cast<char*>("ScriptSpellAt"));
    sScript = "k_hen_attacked01";
    pGFF->WriteFieldCResRef(&topStruct, CResRef(sScript), const_cast<char*>("ScriptAttacked"));
    sScript = "k_def_damage01";
    pGFF->WriteFieldCResRef(&topStruct, CResRef(sScript), const_cast<char*>("ScriptDamaged"));
    sScript = "";
    pGFF->WriteFieldCResRef(&topStruct, CResRef(sScript), const_cast<char*>("ScriptDisturbed"));
    sScript = "k_hen_combend01";
    pGFF->WriteFieldCResRef(&topStruct, CResRef(sScript), const_cast<char*>("ScriptEndRound"));
    sScript = "k_hen_dialogue01";
    pGFF->WriteFieldCResRef(&topStruct, CResRef(sScript), const_cast<char*>("ScriptDialogue"));
    sScript = "k_hen_spawn01";
    pGFF->WriteFieldCResRef(&topStruct, CResRef(sScript), const_cast<char*>("ScriptSpawn"));
    sScript = "";
    pGFF->WriteFieldCResRef(&topStruct, CResRef(sScript), const_cast<char*>("ScriptRested"));
    sScript = "";
    pGFF->WriteFieldCResRef(&topStruct, CResRef(sScript), const_cast<char*>("ScriptDeath"));
    sScript = "k_def_userdef01";
    pGFF->WriteFieldCResRef(&topStruct, CResRef(sScript), const_cast<char*>("ScriptUserDefine"));
    sScript = "k_def_blocked01";
    pGFF->WriteFieldCResRef(&topStruct, CResRef(sScript), const_cast<char*>("ScriptOnBlocked"));
    sScript = "";
    pGFF->WriteFieldCResRef(&topStruct, CResRef(sScript), const_cast<char*>("ScriptEndDialogue"));

    CExoString sFileName;
    CExoString sFile;
    sFile = "temp";
    sFileName = sFile;
    sFile = CExoString("TEMP:") + sFile;
    pGFF->WriteGFFFile(CExoString(sFile.CStr()), RESTYPE_BIC);
    delete pGFF;
}

static CExoString s_headTurnBoneOverride;

// Starts or stops head tracking of a target: both model parts get the begin or end animation,
// and a failed bone lookup or a zero arc clears the track. The bone name comes from the
// head-turn-bone override when set, otherwise appearance.2da.
int CSWCCreature::LookAt(unsigned long long objId, float fRange) {
    CExoString sBone;
    CExoString sHeadBone;
    int nResult = 0;
    Gob* pHeadModel;
    Gob* pBodyModel;
    float fArc;
    int nHeadTrack;
    CSWCObject* pTarget;
    float fArcV;

    if (m_bDisableLookAt == 0 && m_lookAtTargetId != objId && m_bPlayingCutsceneAnimation == 0 &&
        m_pCreatureAppearance != 0) {
        pHeadModel = GetModel(CSWCAnimBase::ANIM_MODEL_HEAD, 0);
        pBodyModel = GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 0);
        if (pHeadModel != 0 || pBodyModel != 0) {
            if (g_pRules->m_p2DArrays->m_pAppearance->GetINTEntry(
                    m_pCreatureAppearance->m_nAppearanceId,
                    CTwoDimArrays::COL2DA_APPEARANCETABLE_HEADTRACK, &nHeadTrack) == 0)
                nHeadTrack = 0;
            if (objId != OBJECT_INVALID && nHeadTrack != 0) {
                pTarget =
                    static_cast<CSWCObject*>(g_pAppManager->m_pClientExoApp->GetGameObject(objId));
                if (pTarget != 0)
                    pTarget = static_cast<CSWCObject*>(pTarget->AsSWCObject());
                if (pTarget != 0 && pTarget->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 0) != 0) {
                    float dx = pTarget->m_position.x - m_position.x;
                    float dy = pTarget->m_position.y - m_position.y;
                    float dz = pTarget->m_position.z - m_position.z;
                    if (dx * dx + dy * dy + dz * dz > fRange * fRange) {
// Out of range: a plain 0, with the tracking state untouched.
                    } else {
                        if (s_headTurnBoneOverride != "") {
                            sHeadBone = s_headTurnBoneOverride;
                        } else if (g_pRules->m_p2DArrays->m_pAppearance->GetCExoStringEntry(
                                       m_pCreatureAppearance->m_nAppearanceId,
                                       CTwoDimArrays::COL2DA_APPEARANCETABLE_HEADBONE,
                                       &sHeadBone) == 0) {
                            sHeadBone = "hturn_g";
                        }
                        float fArcH = 40.0f;
                        if (g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
                                m_pCreatureAppearance->m_nAppearanceId,
                                CTwoDimArrays::COL2DA_APPEARANCETABLE_HEAD_ARC_H, &fArc) != 0)
                            fArcH = fArc;
                        if (g_pRules->m_p2DArrays->m_pAppearance->GetFLOATEntry(
                                m_pCreatureAppearance->m_nAppearanceId,
                                CTwoDimArrays::COL2DA_APPEARANCETABLE_HEAD_ARC_V, &fArc) == 0)
                            fArc = 30.0f;
                        fArcV = fArc;
                        if (pTarget->m_objectType == GAME_OBJECT_TYPE_CREATURE) {
                            sBone = "CAMERAHOOK";
                        } else {
                            sBone = pTarget->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)->GetModelName();
                        }
                        m_lookAtTargetId = objId;
                        if (pHeadModel != 0)
                            pHeadModel->BeginLookAtAnimate(
                                static_cast<CAurObject*>(pTarget->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)), static_cast<char*>(sBone.CStr()),
                                static_cast<char*>(sHeadBone.CStr()), fArcH, fArcV, 0.25f, fRange);
                        if (pBodyModel != 0)
                            pBodyModel->BeginLookAtAnimate(
                                static_cast<CAurObject*>(pTarget->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)), static_cast<char*>(sBone.CStr()),
                                static_cast<char*>(sHeadBone.CStr()), fArcH, fArcV, 0.25f, fRange);
                        nResult = 1;
                    }
                }
            } else {
                m_lookAtTargetId = OBJECT_INVALID;  // INVALIDOBJID
                if (pHeadModel != 0)
                    pHeadModel->EndLookAtAnimate();
                if (pBodyModel != 0)
                    pBodyModel->EndLookAtAnimate();
            }
        }
    }
    return nResult;
}

// Picks this frame's head-track target and re-issues the look-at. Only runs while the look-at
// flags allow it and no non-stationary animation is playing. Four candidate slots are scanned,
// and a target wins only by sitting inside the slot's range and within a 110-degree cone of our
// orientation.
void CSWCCreature::UpdateLookAt() {
    if ((~m_nAIStateFlags & 3) == 0 &&
        (AnimationStationary(GetCurrentAnimation()) != 1 ||
        GetCurrentAnimation() == SERVER_ANIMATION_PAUSE)) {
        if (m_nAIStateFlags & 2)
            m_aLookAtTargetIds[0] = m_oidCombatTarget;
        else
            m_aLookAtTargetIds[0] = CGameObjectArray::INVALIDOBJID;

        if (m_oidLookAtOverride != CGameObjectArray::INVALIDOBJID) {
            m_aLookAtTargetIds[2] = m_oidLookAtOverride;
        } else if (CSWCArea* pArea = m_pArea) {
            CSWCObject* pNearest = 0;
            float fNearest = 8.0f;
            for (int64_t i = 0; i < pArea->m_objectIdList.m_count; i++) {
                CSWCObject* pObj = static_cast<CSWCObject*>(g_pAppManager->m_pClientExoApp->GetGameObject(
                    pArea->m_objectIdList.m_pArray[i]));
                if (pObj != 0)
                    pObj = static_cast<CSWCObject*>(pObj->AsSWCObject());
                if (pObj == 0)
                    continue;
                if (pObj->m_bSpokeChat == 0 && pObj->m_bAttractsLookAt == 0)
                    continue;
                float dx = pObj->m_position.x - m_position.x;
                float dy = pObj->m_position.y - m_position.y;
                float dz = pObj->m_position.z - m_position.z;
                float fDist2 = dx * dx + dy * dy + dz * dz;
                if (fDist2 < fNearest * fNearest) {
                    fNearest = __builtin_sqrtf(fDist2);  // Inline square root.
                    pNearest = pObj;
                }
            }
            if (pNearest != 0 && pNearest->m_objectId != m_objectId)
                m_aLookAtTargetIds[2] = pNearest->m_objectId;
            else
                m_aLookAtTargetIds[2] = CGameObjectArray::INVALIDOBJID;
        }

        if (g_pAppManager->m_pClientExoApp->GetPlayerByGameObjectID(m_objectId) == 0) {
            CExoLinkedListInternal* pPlayers =
                g_pAppManager->m_pClientExoApp->GetPlayerList()->m_pInternal;
            CExoLinkedListNode* pNode = pPlayers->m_pHead;
            CSWCPlayer* pPlayer = pNode ? static_cast<CSWCPlayer*>(pNode->m_pData) : 0;
            CSWCObject* pNearest = 0;
            float fNearest = 5.0f;
            while (pNode != 0) {
                if (pPlayer != 0) {
                    CSWCObject* pObj =
                        static_cast<CSWCObject*>(g_pAppManager->m_pClientExoApp->GetGameObject(
                            pPlayer->m_gameObjectId));
                    if (pObj != 0)
                        pObj = static_cast<CSWCObject*>(pObj->AsSWCObject());
                    if (pObj != 0 && pObj->m_pArea == m_pArea) {
                        CSWCCreature* pCreature = pObj->AsSWCCreature();
                        int visible = 1;
                        if (pCreature != 0) {
                            if (m_visibilityList.m_count > 0) {
                                int i = m_visibilityList.m_count;
                                CSWVisibilityNode** ppNode = m_visibilityList.m_pArray;
                                do {
                                    CSWVisibilityNode* pVis = *ppNode;
                                    if (pVis->m_objectId == pCreature->m_objectId &&
                                        (pVis->m_flags & 1) == 0)
                                        visible &= static_cast<int>(pVis->m_flags << 30) >> 31;
                                    ppNode++;
                                } while (--i != 0);
                            } else {
                                visible = 0;
                            }
                        }
                        if (visible != 0) {
                            float dx = pObj->m_position.x - m_position.x;
                            float dy = pObj->m_position.y - m_position.y;
                            float dz = pObj->m_position.z - m_position.z;
                            float fDist2 = dx * dx + dy * dy + dz * dz;
                            if (fDist2 < fNearest) {
                                fNearest = __builtin_sqrtf(fDist2);  // Inline square root.
                                pNearest = pObj;
                            }
                        }
                    }
                }
                pPlayer = static_cast<CSWCPlayer*>(pPlayers->GetNext(pNode));
            }
            if (pNearest != 0)
                m_aLookAtTargetIds[3] = pNearest->m_objectId;
            else
                m_aLookAtTargetIds[3] = CGameObjectArray::INVALIDOBJID;
        }

        Vector vDelta = {0.0f, 0.0f, 0.0f};
        for (int i = 0; i < 4; i++) {
            unsigned long long id = m_aLookAtTargetIds[i];
            if (id == m_objectId) {
                m_aLookAtTargetIds[i] = CGameObjectArray::INVALIDOBJID;
                continue;
            }
            CSWCObject* pObj =
                static_cast<CSWCObject*>(g_pAppManager->m_pClientExoApp->GetGameObject(id));
            if (pObj != 0)
                pObj = static_cast<CSWCObject*>(pObj->AsSWCObject());
            if (pObj == 0)
                continue;
            vDelta.x = pObj->m_position.x - m_position.x;
            vDelta.y = pObj->m_position.y - m_position.y;
            vDelta.z = pObj->m_position.z - m_position.z;
            float fDist2 =
                vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z;
            if (!(fDist2 < 1e-05f)) {
                vDelta.z = 0.0f;
                vDelta = normalize(vDelta);
                float fDot = vDelta.x * m_orientation.x + vDelta.y * m_orientation.y +
                             vDelta.z * m_orientation.z;
                if (!(acosf(fDot) * 57.295776f <= 110.0f))
                    continue;
            }
            LookAt(pObj->m_objectId, (i == 3) ? 5.0f : 8.0f);
            return;
        }
    }
    LookAt(CGameObjectArray::INVALIDOBJID, 0.0f);
}

#if !KOTOR1
// Party-menu use-item entry point. Controller input validates against the target's server
// creature and forwards to PartyMenuUseItem. Mouse input is a small state machine: a solo party
// or a self-targeted item goes straight to the character, otherwise the first click arms the
// pending use and a second one resolves the party member.
void CSWCCreature::PartyMenuUseItemSelect(unsigned int itemId, CSWCCreature* pTarget) {
    if (g_initiatedByController != 0) {
        itemId &= CSWGuiInterfaceAction::ACTION_ID_ITEM_CLEAR;
        int serverId =
            static_cast<int>(g_pAppManager->m_pServerExoApp->ClientToServerObjectId(itemId));
        CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(serverId);
        if (pItem == 0)
            return;
        pItem->GetBaseItem();  // Result discarded.
        if (pTarget->GetServerObject() == 0)
            return;
        CSWSCreature* pServer = static_cast<CSWSCreature*>(pTarget->GetServerObject()->AsSWSCreature());
        if (pServer == 0)
            return;
        if (pServer->CanUseItem(pItem, 1, 1) == 0)
            return;
        PartyMenuUseItem(itemId, pTarget);
        return;
    }

    int nPartyMember = g_pAppManager->m_pClientExoApp->GetInGameGui()
                           ->m_pMainInterface->m_nPartyItemUseTarget;
    itemId &= CSWGuiInterfaceAction::ACTION_ID_ITEM_CLEAR;
    int serverId = static_cast<int>(g_pAppManager->m_pServerExoApp->ClientToServerObjectId(itemId));
    if (g_pAppManager->m_pClientExoApp->GetSWParty()->m_nCharacterCount == 1)
        goto solo;
    if (g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(serverId)
            ->GetBaseItem()
            ->m_nItemType == ITEM_TYPE_SQUAD_RECOVERY_KIT)
        goto solo;

    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface->m_bPartyItemUsePending != 0) {
        if (nPartyMember == -1)
            return;
        CSWCCreature* pChar =
            g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(nPartyMember);
        unsigned long long targetId = pChar->m_objectId;
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface->m_bPartyItemUsePending = 0;
        pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(nPartyMember);
        CSWSCreature* pServer = 0;
        if (pChar->GetServerObject() != 0)
            pServer = static_cast<CSWSCreature*>(pChar->GetServerObject()->AsSWSCreature());
        CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(serverId);
        if (pServer->CanUseItem(pItem, 0, 1) == 0)
            goto end;
        pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(nPartyMember);
        CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
        if (pMsg == 0)
            goto end;
        pMsg->SendPlayerToServerInput_UseItem(itemId, 0, 0, targetId, pChar->m_position);
        goto end;
    }

    if (g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(serverId)
            ->GetBaseItem()
            ->m_nItemType == ITEM_TYPE_FOREARM_BANDS) {
        unsigned long long oid = m_objectId;
        CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
        if (pMsg != 0)
            pMsg->SendPlayerToServerInput_UseItem(itemId, 0, 0, oid, m_position);
    } else {
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface->m_oidPartyItemUse = itemId;
        g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface->m_bPartyItemUsePending = 1;
    }
    goto end;

solo: {
    CSWCCreature* pChar = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(
        nPartyMember == -1 ? 0 : nPartyMember);
    CSWSCreature* pServer = 0;
    if (pChar->GetServerObject() != 0)
        pServer = static_cast<CSWSCreature*>(pChar->GetServerObject()->AsSWSCreature());
    CSWSItem* pItem = g_pAppManager->m_pServerExoApp->GetItemByGameObjectID(serverId);
    if (pServer->CanUseItem(pItem, 0, 1) == 0)
        return;
    unsigned long long oid = m_objectId;
    CSWCMessage* pMsg = g_pAppManager->m_pClientExoApp->GetSWCMessage();
    if (pMsg == 0)
        return;
    pMsg->SendPlayerToServerInput_UseItem(itemId, 0, 0, oid, m_position);
    return;
}

end:
    g_pAppManager->m_pClientExoApp->GetInGameGui()->m_pMainInterface->m_nPartyItemUseTarget = -1;
}
#endif

// Runs the client creature's per-frame tick. It refreshes the wall-clock delta, decays the weapon,
// feedback and overlay timers, replays a combat-paused draw animation, fires the ambient idle, pops
// the bubble, runs one of the dialog, drive or follow animation drivers, tilts the model to the
// server's surface normal, powers the conjured weapon pair and repositions the looping sound. The
// tail either removes the creature or runs the floaty-text, wind and rolling-footstep updates.
void CSWCCreature::AIUpdate() {
#if !KOTOR1
    unsigned int nTime = g_pExoBase->m_pTimers->GetLowResolutionTimer();
    if (m_nSwitchWeaponsCooldown != -1) {
        m_nSwitchWeaponsCooldown += m_nLastAIUpdateTime - static_cast<int>(nTime);
        if (m_nSwitchWeaponsCooldown < 0)
            m_nSwitchWeaponsCooldown = 0;
    }
    m_nLastAIUpdateTime = static_cast<int>(nTime);
#endif

    unsigned int nDay, nTimeOfDay, nOutA, nOutB;
    g_pAppManager->m_pClientExoApp->GetActiveTimer(m_objectId)->GetWorldTime(&nDay, &nTimeOfDay);
    g_pAppManager->m_pClientExoApp->GetWorldTimer()->SubtractWorldTimes(
        nDay, nTimeOfDay, m_nLastAIUpdateDay, m_nLastAIUpdateTod, &nOutA, &nOutB);
    m_nAIUpdateDeltaMs = nOutB;
    if (m_feedbackTimer != 0)
        m_feedbackTimer = static_cast<unsigned int>(m_feedbackTimer) > m_nAIUpdateDeltaMs ? m_feedbackTimer - m_nAIUpdateDeltaMs : 0;

    if (static_cast<short>(m_pendingDrawAnimation) != -1 &&
        g_pAppManager->m_pClientExoApp->GetPausedByCombat() == 0)
        PlayDrawAnimation(m_pendingDrawAnimation);

    if (m_pModel != 0) {
        if (m_currentAnimation == SERVER_ANIMATION_PAUSE) {
            m_nAmbientPauseElapsed += m_nAIUpdateDeltaMs;
            if (m_nAmbientPauseElapsed >= m_nAmbientPauseDelay &&
                g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState == 0) {
                unsigned short anim;
                if (m_pModel->m_creatureType == CSWCAnimBase::CREATURE_TYPE_FULL) {
                    int nRand = rand_wincompatible() % 7;
                    if (nRand < 4)
                        anim = static_cast<unsigned short>(SERVER_ANIMATION_HEAD_TURN_LEFT + nRand);
                    else
                        anim = SERVER_ANIMATION_PAUSE_FIDGET;
                } else {
                    int nRand = rand_wincompatible() % 5;
                    anim = (nRand == 0) ? SERVER_ANIMATION_HEAD_TURN_LEFT
                                        : (nRand == 1 ? SERVER_ANIMATION_HEAD_TURN_RIGHT : SERVER_ANIMATION_PAUSE_FIDGET);
                }
#if KOTOR1 && !defined(K2_FIX_ENGINE_BUGS)
                AddFireForgetAnimation(anim, &anim);
#else
// K1 queues a stack local as the payload, which the fire-and-forget path later reads as an
// attack record.
                static CSWCCombatAttackData s_nullData;
                static bool s_initNullData = false;
                if (!s_initNullData) {
                    memset(static_cast<void*>(&s_nullData), 0, sizeof(s_nullData));
                    s_initNullData = true;
                }
                AddFireForgetAnimation(anim, &s_nullData);
#endif
                m_nAmbientPauseElapsed = 0;
                m_nAmbientPauseDelay = (rand_wincompatible() % 4) * 5000 + 15000;
            }
        } else {
            m_nAmbientPauseElapsed = 0;
        }
    }

    UpdatePopUpBubble();

#if KOTOR1
    if (m_switchMode == 1 && IsDriveAnimationBlocked() == 0) {
#else
    if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bDialogCameraSet == 0 && m_switchMode == 1 &&
        IsDriveAnimationBlocked() == 0) {
#endif
        if (g_pAppManager->m_pClientExoApp->GetInGameGui()->IsInDialog(g_pAppManager->m_pClientExoApp->GetPlayerCreatureId()))
            AnimationUpdate();
        else {
            AnimateFireAndForget();
            DriveAnimationUpdate();
        }
    } else {
        if (m_bInParty != 0) {
            CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
            int nIndex = pParty->GetIndex(m_objectId);
            if (static_cast<unsigned>(nIndex - 1) <= 1)
                pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
            if (m_bInParty != 0) {
                CSWSCreature* pServer = GetServerCreature();
                if (pServer != 0 && pServer->GetCurrentlyFollowingLeader() != 0) {
                    AnimateFireAndForget();
                    FollowAnimationUpdate();
                    MoveCreatureToPosition(m_desiredPosition);
                    goto updateDone;
                }
            }
        }
        AnimationUpdate();
    }
updateDone:

    if (m_objectId != OBJECT_INVALID && m_pModel != 0 &&
        m_objectId == g_pAppManager->m_pClientExoApp->GetPlayerCreatureId())
        m_pModel->Lean();

    if (m_bModelSyncDisabled == 0) {
        if (m_pArea != 0 && m_pArea->GetIsObjectCorpse(m_objectId) == 1)
            goto groundTiltRange;
        if (GetServerCreature() != 0) {
            CSWSCreature* pServer;
            GetServerObject();  // Result discarded.
            pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature());
            if (pServer->GetDead() == 1) {
            groundTiltRange:
                Vector vNormal = g_pAppManager->m_pServerExoApp->GetNormalOfPoint(m_position);
                float fPitch = Pitch(vNormal);
                GetAnimationBase()->SetGroundTilt(
                    (fPitch <= 80.0f && fPitch >= 65.0f) ? vNormal : zaxis, m_orientation);
                goto groundTiltDone;
            } else {
#if KOTOR1
                if (GetServerCreature()->GetDeadTemp() == 1)
                    goto groundTiltRange;
#else
                GetServerObject();  // Result discarded.
                pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature());
                if (pServer->GetDeadTemp() != 0)
                    goto groundTiltRange;
#endif
            }
        }
        if (m_pCreatureAppearance->m_bGroundTilt != 0) {
            Vector vNormal = g_pAppManager->m_pServerExoApp->GetNormalOfPoint(m_position);
            float fPitch = Pitch(vNormal);
            GetAnimationBase()->SetGroundTilt(fPitch > 40.0f ? vNormal : zaxis, m_orientation);
        } else {
            GetAnimationBase()->SetGroundTilt(zaxis, m_orientation);
        }
    groundTiltDone:;
    }

    HandleConjuringCreature();
    UpdateVisualEffectsList();

#if !KOTOR1
    if (m_bInCombat == 1) {
        CSWSCreature* pServer = GetServerCreature();
        if (pServer != 0 && static_cast<short>(pServer->GetCurrentHitPoints(0)) >= 1) {
            CSWInventory* pInventory = pServer->m_pInventory;
            if (pInventory != 0) {
                CSWSItem* pRight = static_cast<CSWSItem*>(pInventory->GetItemInSlot(CSWInventory::SLOT_RIGHTWEAPON));
                CSWSItem* pLeft = static_cast<CSWSItem*>(pInventory->GetItemInSlot(CSWInventory::SLOT_LEFTWEAPON));
                if (pRight != 0)
                    pRight->PowerItem(1, 0, 1);
                if (pLeft != 0)
                    pLeft->PowerItem(1, 0, 1);
            }
        }
    }
#endif

    unsigned int nDay2, nTimeOfDay2;
    g_pAppManager->m_pClientExoApp->GetActiveTimer(m_objectId)->GetWorldTime(&nDay2, &nTimeOfDay2);
    m_nLastAIUpdateDay = nDay2;
    m_nLastAIUpdateTod = nTimeOfDay2;
    if (m_pLoopingCreatureSound != 0 && m_bLoopingSoundStarted == 0) {
        m_pLoopingCreatureSound->SetPosition(m_position, 1.5f);
        m_pLoopingCreatureSound->Set3D(1);
        m_pLoopingCreatureSound->Play();
        m_bLoopingSoundStarted = 1;
    }

    m_nRoundTimer -= m_nAIUpdateDeltaMs;
    if (m_nRoundTimer < 1)
        m_nRoundTimer = 6000;
    m_nWeaponOverlayTimer -= m_nAIUpdateDeltaMs;
    if (m_nWeaponOverlayTimer <= 0) {
        m_nWeaponOverlayTimer = 2000;
        ComputeWeaponOverlays(1, 0);
    }

    if (enablemodelrequest) {
        g_pAppManager->m_pClientExoApp->GetModule();  // Result discarded.
        if (m_nAppearanceRequestTimer != 0) {
            if (m_nAppearanceRequestTimer == 1) {
                m_nAppearanceRequestTimer = 0;
            } else if (static_cast<unsigned>(m_nAppearanceRequestTimer) > m_nAIUpdateDeltaMs) {
                m_nAppearanceRequestTimer -= m_nAIUpdateDeltaMs;
                goto appearanceDone;
            }
            ForceAppearanceUpdate();
        }
appearanceDone:
        ReAttachCamera();
    }

    if (FadeObject()) {
        g_pAppManager->m_pClientExoApp->GetClientAIMaster()->m_list.RemoveObject(this);
        CGameObject* pRemoved;
        if (g_pAppManager->m_pClientExoApp->GetGameObjectArray()->Delete(m_objectId, &pRemoved) ==
                CGameObjectArray::SUCCESS &&
            pRemoved != 0) {
            delete this;
            return;
        }
    }

    UpdateFloatyText();
    if ((m_fadeState == 1 && g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCreatureWindMode == 2) ||
        (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCreatureWindMode == 1 &&
         g_pAppManager->m_pClientExoApp->GetPlayerCreatureId() == m_objectId))
        SetWind(1, creaturewindintensity);
    else
        SetWind(0, 0.0f);
    if (m_pModel != 0)
        m_pModel->UpdateSound(m_objectId == g_pAppManager->m_pClientExoApp->GetPlayerCreatureId());
    UpdateRollingFootstepSound();
}

// Per-frame facing. Two mutually exclusive sources: a live attacking server twin turns toward
// its own orientation at the combat turn speed, otherwise the creature turns toward the passed
// direction and fires one of the two turn animations.
void CSWCCreature::RotateCreatureToOrientation(const Vector& vDir) {
    if (GetServerObject() != 0 && GetServerObject()->AsSWSCreature() != 0) {
        CSWSCreature* pServer = 0;
        if (GetServerObject() != 0)
            pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature());
        if (pServer->IsDebilitated(0) != 0)
            return;
        GetServerObject();  // Result discarded.
        pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature());
        if (pServer->GetDead() != 0)
            return;
        pServer = 0;
        if (GetServerObject() != 0)
            pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature());
        if (pServer->GetDeadTemp() != 0)
            return;
    }

    if (GetServerObject() == 0 || GetServerObject()->AsSWSCreature() == 0)
        return;

    CSWSCreature* pServer = 0;
    if (GetServerObject() != 0)
        pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature());
    if (pServer != 0) {
        GetServerObject();  // Result discarded.
        pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature());
        if (pServer->m_bCombatState != 0) {
            pServer = 0;
            if (GetServerObject() != 0)
                pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature());
            if (pServer->HasAction(1) == 0) {
                GetServerObject();  // Result discarded.
                if (GetServerObject()->AsSWSCreature()->m_attemptedAttackTargetId !=
                    OBJECT_INVALID) {  // INVALIDOBJID
                    pServer = 0;
                    if (GetServerObject() != 0)
                        pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature());
                    GetServerObject();  // Result discarded.
                    if (pServer->GetReputation(GetServerObject()->AsSWSCreature()
                                                   ->m_attemptedAttackTargetId, 0) < 11)
                        goto faceCurrentTarget;
                }
                GetServerObject();  // Result discarded.
                if (GetServerObject()->AsSWSCreature()->m_oidAttemptedSpellTarget ==
                    OBJECT_INVALID)  // INVALIDOBJID
                    goto rotateToDir;
                pServer = 0;
                if (GetServerObject() != 0)
                    pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature());
                GetServerObject();  // Result discarded.
                if (pServer->GetReputation(GetServerObject()->AsSWSCreature()->m_oidAttemptedSpellTarget, 0) > 10)
                    goto rotateToDir;
faceCurrentTarget: {
                    pServer = 0;
                    if (GetServerObject() != 0)
                        pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature());
                    Vector vFrom = pServer->m_orientation;
                    float fTurn = Yaw(vDir) - Yaw(vFrom);
                    if (fTurn > 180.0f)
                        fTurn -= 360.0f;
                    if (fTurn < -180.0f)
                        fTurn += 360.0f;
                    float fMax = fCombatTurnSpeed * static_cast<float>(m_nAIUpdateDeltaMs);
                    if (fTurn < 0.0f) {
                        fMax = -fMax;
                        if (fTurn < fMax)
                            fTurn = fMax;
                    } else if (fTurn > fMax) {
                        fTurn = fMax;
                    }
                    Quaternion q = YawPitchRoll(fTurn, 0.0f, 0.0f);
                    Vector vOut = RotateVector(vFrom, q);
                    pServer->SetOrientation(vOut);
                    SetOrientation(vOut);
                    return;
                }
            }
        }
    }

rotateToDir: {
        Vector vNorm(vDir.x, vDir.y, 0.0f);
        vNorm = normalize(vNorm);
        Vector vOut;
        float fCross = vNorm.y * m_orientation.x - vNorm.x * m_orientation.y;
        float fSign = (fCross >= 0.0f) ? 1.0f : -1.0f;
        float fDot = vNorm.x * m_orientation.x + vNorm.y * m_orientation.y +
                     vNorm.z * m_orientation.z;
        float fYawSelf = Yaw(m_orientation);
        float fYawDir = Yaw(vNorm);
        if (fDot < 1.0f && fabsf(fYawSelf - fYawDir) > 15.0f) {
            if (m_currentAnimation == SERVER_ANIMATION_PAUSE ||
                m_currentAnimation == SERVER_ANIMATION_READY ||
                m_currentAnimation == SERVER_ANIMATION_LISTEN ||
                m_currentAnimation == SERVER_ANIMATION_TALK_NORMAL ||
                m_currentAnimation == SERVER_ANIMATION_TALK_PLEADING ||
                m_currentAnimation == SERVER_ANIMATION_TALK_FORCEFUL ||
                m_currentAnimation == SERVER_ANIMATION_TALK_LAUGHING ||
                m_currentAnimation == SERVER_ANIMATION_TALK_SAD ||
                m_currentAnimation == SERVER_ANIMATION_HEAD_TURN_LEFT ||
                m_currentAnimation == SERVER_ANIMATION_HEAD_TURN_RIGHT ||
                m_currentAnimation == SERVER_ANIMATION_TALK_INJURED ||
                m_currentAnimation == SERVER_ANIMATION_LISTEN_INJURED) {
                if (m_currentAnimation == SERVER_ANIMATION_HEAD_TURN_LEFT ||
                    m_currentAnimation == SERVER_ANIMATION_HEAD_TURN_RIGHT)
                    AbortFireForgetAnimation(0);
                if (fSign < 0.0f) {
                    if (m_currentAnimation == SERVER_ANIMATION_PAUSE ||
                        m_currentAnimation == SERVER_ANIMATION_LISTEN ||
                        m_currentAnimation == SERVER_ANIMATION_TALK_NORMAL ||
                        m_currentAnimation == SERVER_ANIMATION_TALK_PLEADING ||
                        m_currentAnimation == SERVER_ANIMATION_TALK_FORCEFUL ||
                        m_currentAnimation == SERVER_ANIMATION_TALK_LAUGHING ||
                        m_currentAnimation == SERVER_ANIMATION_TALK_SAD ||
                        m_currentAnimation == SERVER_ANIMATION_TALK_INJURED ||
                        m_currentAnimation == SERVER_ANIMATION_LISTEN_INJURED) {
                        if (GetAnimationBase() != 0 &&
                            GetAnimationBase()->m_creatureType == CSWCAnimBase::CREATURE_TYPE_FULL &&
                            static_cast<unsigned short>(m_currentAnimation - CSWCAnimBase::CLIENT_ANIMATION_TURN_LEFT_2) >= 2)
                            AddFireForgetAnimation(CSWCAnimBase::CLIENT_ANIMATION_TURN_RIGHT_2, 0);
                    } else if (m_currentAnimation == SERVER_ANIMATION_READY) {
                        AddFireForgetAnimation(CSWCAnimBase::CLIENT_ANIMATION_TURN_RIGHT_2, 0);
                    }
                } else {
                    if (m_currentAnimation == SERVER_ANIMATION_PAUSE ||
                        m_currentAnimation == SERVER_ANIMATION_LISTEN ||
                        m_currentAnimation == SERVER_ANIMATION_TALK_NORMAL ||
                        m_currentAnimation == SERVER_ANIMATION_TALK_PLEADING ||
                        m_currentAnimation == SERVER_ANIMATION_TALK_FORCEFUL ||
                        m_currentAnimation == SERVER_ANIMATION_TALK_LAUGHING ||
                        m_currentAnimation == SERVER_ANIMATION_TALK_SAD ||
                        m_currentAnimation == SERVER_ANIMATION_TALK_INJURED ||
                        m_currentAnimation == SERVER_ANIMATION_LISTEN_INJURED) {
                        if (GetAnimationBase() != 0 &&
                            GetAnimationBase()->m_creatureType == CSWCAnimBase::CREATURE_TYPE_FULL &&
                            static_cast<unsigned short>(m_currentAnimation - CSWCAnimBase::CLIENT_ANIMATION_TURN_LEFT_2) >= 2)
                            AddFireForgetAnimation(CSWCAnimBase::CLIENT_ANIMATION_TURN_LEFT_2, 0);
                    } else if (m_currentAnimation == SERVER_ANIMATION_READY) {
                        AddFireForgetAnimation(CSWCAnimBase::CLIENT_ANIMATION_TURN_LEFT_2, 0);
                    }
                }
            }
        }
        float fBlend = (1.0f - fDot) * 3.0f;
        if (m_currentAnimation == SERVER_ANIMATION_RUN)
            fBlend *= 2.0f;
        float fStep = fBlend;
        if (fStep <= 0.55f)
            fStep = 0.55f;
        else if (fStep >= 1.0f)
            fStep = 1.0f;
        float fAngle = fStep * static_cast<float>(m_nAIUpdateDeltaMs) * 4000.0f / 1000000.0f;
        if (cosf(fAngle) <= fDot) {
            vOut = vDir;
        } else {
            float fRot = fSign * fAngle;
            vOut.x = m_orientation.x * cosf(fRot) - m_orientation.y * sinf(fRot);
            vOut.y = m_orientation.x * sinf(fRot) + m_orientation.y * cosf(fRot);
        }
        vOut.z = 0.0f;
        vOut = normalize(vOut);
        SetOrientation(vOut);
    }
}

// Per-frame follow-animation driver for the two companion party slots. Early-outs on a dead or
// debilitated server twin; the distance to the follow point picks walk or run, injured creatures
// get the limp variants and the animation's own length normalizes the playback speed. Always
// ends facing the follow orientation.
void CSWCCreature::FollowAnimationUpdate() {
    if (GetServerObject() != 0 && GetServerObject()->AsSWSCreature() != 0) {
        CSWSCreature* pServer = 0;
        if (GetServerObject() != 0)
            pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature());
        K2_ASSUME(pServer != 0);   // the same twin the test above found
        if (pServer->GetDeadTemp() != 0)
            return;
        GetServerObject();  // Result discarded.
        pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature());
        if (pServer->GetDead() != 0)
            return;
        pServer = 0;
        if (GetServerObject() != 0)
            pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature());
        if (pServer->IsDebilitated(0) != 0)
            return;
    }

    int nMember = g_pAppManager->m_pClientExoApp->GetSWParty()->GetIndex(m_objectId);
    if (static_cast<unsigned>(nMember - 1) > 1)
        return;
    CSWParty* pParty = g_pAppManager->m_pClientExoApp->GetSWParty();
    float fDist = pParty->m_members[nMember].m_followDistance;
    unsigned short anim;
    if (fDist <= 0.0001f) {
        if (g_pAppManager->m_pClientExoApp->GetInGameGui()->m_nGlobalDialogState != 0 &&
            IsDoingDialogAnimation() != 0) {
            m_animationSpeed = 0.0f;
            goto rotate;
        }
        fDist = 1.0f;
        anim = static_cast<unsigned short>(GetPauseReadyAnimation());
    } else {
        float fSpeed;
        if (fDist >= DriveWalkThreshold * m_fRunRate) {
            fSpeed = m_pCreatureAppearance->m_runSpeed * 1000.0f;
            if (m_bInParty != 0 && GetServerObject() != 0) {
                CSWSCreature* pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature());
                if (pServer != 0 && pServer->IsInjured() != 0) {
                    if (!(g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK &&
                          g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0) == this)) {
                        anim = SERVER_ANIMATION_RUN_INJURED;
                        goto apply;
                    }
                }
            }
#if KOTOR1  // K1: no feat exemption
            if ((m_nActivityFlags & 1) != 0)
#else
            if ((m_nActivityFlags & 1) != 0 && m_pStats->HasFeat(FEAT_STEALTH_RUN) == 0)
#endif
                anim = SERVER_ANIMATION_STEALTH;
            else
                anim = SERVER_ANIMATION_RUN;
        } else {
            fSpeed = m_pCreatureAppearance->m_walkSpeed * 1000.0f;
            if (m_bInParty != 0 && GetServerObject() != 0) {
                CSWSCreature* pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature());
                if (pServer != 0 && pServer->IsInjured() != 0) {
                    if (!(g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK &&
                          g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0) == this)) {
                        anim = SERVER_ANIMATION_WALK_INJURED;
                        goto apply;
                    }
                }
            }
#if KOTOR1  // K1: no feat exemption
            if ((m_nActivityFlags & 1) != 0)
#else
            if ((m_nActivityFlags & 1) != 0 && m_pStats->HasFeat(FEAT_STEALTH_RUN) == 0)
#endif
                anim = SERVER_ANIMATION_STEALTH;
            else
                anim = SERVER_ANIMATION_WALK;
        }
apply:
        fDist = fDist / (fSpeed / (GetAnimationLength(anim) * 0.001f));
    }
    m_animationSpeed = fDist;
    m_nSWAnimation = anim;
    SetAnimation(anim, 0, 0);
rotate:
    RotateCreatureToOrientation(pParty->m_members[nMember].m_followOrientation);
}

// Returns the drive acceleration this creature animates at, a flat 15.0 for a creature with the
// drive flag but without the feat, otherwise the value from the appearance row.
float CSWCCreature::GetDriveAcceleration() {
#if KOTOR1  // K1: no feat exemption
    if ((m_nActivityFlags & 1) != 0)
#else
    if ((m_nActivityFlags & 1) != 0 && m_pStats->HasFeat(FEAT_STEALTH_RUN) == 0)
#endif
        return 15.0f;
    return m_pCreatureAppearance->m_driveAccel;
}

// Pushes a hard-shadow enable down to the three equipped items that carry their own models,
// then to this creature's own model.
void CSWCCreature::TurnOnShadows() {
    CSWCItem* pItem =
        g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[4]);
    if (pItem != 0)
        pItem->TurnOnShadows();
    pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[5]);
    if (pItem != 0)
        pItem->TurnOnShadows();
    pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[0]);
    if (pItem != 0)
        pItem->TurnOnShadows();
    if (m_pModel != 0)
        m_pModel->TurnOnShadows();
}

// TurnOnShadows over the same slots.
void CSWCCreature::TurnOffShadows() {
    CSWCItem* pItem =
        g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[4]);
    if (pItem != 0)
        pItem->TurnOffShadows();
    pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[5]);
    if (pItem != 0)
        pItem->TurnOffShadows();
    pItem = g_pAppManager->m_pClientExoApp->GetItemByGameObjectID(m_aEquippedItemIds[0]);
    if (pItem != 0)
        pItem->TurnOffShadows();
    if (m_pModel != 0)
        m_pModel->TurnOffShadows();
}

// Drives the drive animation each frame. It early-outs on a dialog animation or a dead, debilitated
// or disabled server twin. A non-stationary current animation preempts to zero lean while actions
// are pending or combat is engaged. The animation's speed and the lean sign pick the drive, idle or
// walk/run animation, and its own length normalizes the playback speed.
void CSWCCreature::DriveAnimationUpdate() {
    if (AnimationDialog(m_currentAnimation) != 0)
        return;
    if (g_pAppManager->m_pServerExoApp != 0 && GetServerObject() != 0) {
        CSWSCreature* pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature());
        if (pServer != 0) {
            if (pServer->GetDead() != 0)
                return;
#if KOTOR1  // K1: the dying tests are GetIsPCDying and GetDeadTemp
            if (pServer->GetIsPCDying() != 0 || pServer->GetDeadTemp() != 0)
#else
            if (pServer->GetDeadTemp() != 0)
#endif
                return;
            if (pServer->IsDebilitated(0) != 0)
                return;
            if ((pServer->m_nAIState >> 1 & 1) == 0)
                return;
        }
    }
    if (AnimationStationary(m_currentAnimation) == 0) {
        unsigned long long serverId =
            g_pAppManager->m_pServerExoApp->ClientToServerObjectId(m_objectId);
        if (g_pAppManager->m_pServerExoApp->IsActionsPending(serverId) |
            m_engagedState |
            AnimationDialog(m_currentAnimation)) {
            m_pModel->SetLean(0.0f);
            return;
        }
    }
    float fScale = m_pCreatureAppearance->m_fDriveMaxSpeed;
    g_pAppManager->m_pClientExoApp->GetSWParty();  // Result discarded.
#if KOTOR1  // K1: no feat exemption
    if ((m_nActivityFlags & 1) != 0 && m_swAnimationSpeed > m_pCreatureAppearance->m_walkSpeed) {
#else
    if ((m_nActivityFlags & 1) != 0 && m_swAnimationSpeed > m_pCreatureAppearance->m_walkSpeed &&
        m_pStats->HasFeat(FEAT_STEALTH_RUN) == 0) {
#endif
        m_swAnimationSpeed = m_pCreatureAppearance->m_walkSpeed;
    }
    unsigned short anim;
    if (m_swAnimationSpeed > 0.0f) {
        if (m_fSWStrafeSpeed > 0.0f) {
            m_animationSpeed = m_fSWStrafeSpeed / fScale;
            m_nSWAnimation = 10102;
            SetAnimation(10102, 0, 0);
            return;
        }
        if (m_fSWStrafeSpeed < 0.0f) {
            m_animationSpeed = -m_fSWStrafeSpeed / fScale;
            m_nSWAnimation = 10103;
            SetAnimation(10103, 0, 0);
            return;
        }
        if (m_fSWStrafeSpeed == 0.0f) {
            GetPauseReadyAnimation();  // Result discarded.
            goto drive;
        }
    }
    if (m_swAnimationSpeed == 0.0f) {
        if (m_fSWStrafeSpeed < 0.0f) {
            m_animationSpeed = -m_fSWStrafeSpeed / fScale;
            m_nSWAnimation = 10105;
            SetAnimation(10105, 0, 0);
            return;
        }
        if (m_fSWStrafeSpeed > 0.0f) {
            m_animationSpeed = m_fSWStrafeSpeed / fScale;
            m_nSWAnimation = 10104;
            SetAnimation(10104, 0, 0);
            return;
        }
    }
    if (m_swAnimationSpeed < 0.0f) {
        if (m_fSWStrafeSpeed > 0.0f) {
            m_animationSpeed = -m_fSWStrafeSpeed / fScale;
            m_nSWAnimation = 10103;
            SetAnimation(10103, 0, 0);
            return;
        }
        if (m_fSWStrafeSpeed < 0.0f) {
            m_animationSpeed = m_fSWStrafeSpeed / fScale;
            m_nSWAnimation = 10102;
            SetAnimation(10102, 0, 0);
            return;
        }
        if (m_fSWStrafeSpeed == 0.0f) {
            m_animationSpeed = m_swAnimationSpeed / fScale;
            m_nSWAnimation = SERVER_ANIMATION_RUN;
            SetAnimation(SERVER_ANIMATION_RUN, 0, 0);
            return;
        }
    }
    m_animationSpeed = 1.0f;
    anim = static_cast<unsigned short>(GetPauseReadyAnimation());
    if (anim == SERVER_ANIMATION_PAUSE &&
        g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode != CClientOptions::CAMERA_MODE_FREE_LOOK &&
        AnimationPause(m_currentAnimation) != 0)
        return;
    m_nSWAnimation = anim;
    SetAnimation(anim, 0, 0);
    return;

drive:
    {
        float fSpeed;
        if (m_swAnimationSpeed > fScale * DriveWalkThreshold) {
            if (m_bInParty != 0 && GetServerObject() != 0) {
                CSWSCreature* pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature());
                if (pServer != 0 && pServer->IsInjured() != 0 &&
                    !(g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK &&
                      g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0) == this)) {
                    anim = SERVER_ANIMATION_RUN_INJURED;
                    goto run;
                }
            }
#if KOTOR1  // K1: no feat exemption
            if ((m_nActivityFlags & 1) != 0)
#else
            if ((m_nActivityFlags & 1) != 0 && m_pStats->HasFeat(FEAT_STEALTH_RUN) == 0)
#endif
                anim = SERVER_ANIMATION_STEALTH;
            else
                anim = SERVER_ANIMATION_RUN;
run:
            m_bRunning = 1;
            fSpeed = m_pCreatureAppearance->m_runSpeed;
        } else {
            if (m_bInParty != 0 && GetServerObject() != 0) {
                CSWSCreature* pServer = static_cast<CSWSCreature*>(GetServerObject()->AsSWSCreature());
                if (pServer != 0 && pServer->IsInjured() != 0 &&
                    !(g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK &&
                      g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0) == this)) {
                    anim = SERVER_ANIMATION_WALK_INJURED;
                    goto walk;
                }
            }
#if KOTOR1  // K1: no feat exemption
            if ((m_nActivityFlags & 1) != 0)
#else
            if ((m_nActivityFlags & 1) != 0 && m_pStats->HasFeat(FEAT_STEALTH_RUN) == 0)
#endif
                anim = SERVER_ANIMATION_STEALTH;
            else
                anim = SERVER_ANIMATION_WALK;
walk:
            m_bRunning = 0;
            fSpeed = m_pCreatureAppearance->m_walkSpeed;
        }
        m_animationSpeed = m_swAnimationSpeed / (fSpeed / (GetAnimationLength(anim) * 0.001f));
        int bResume = m_nSWAnimation != 10095;
        m_nSWAnimation = anim;
        SetAnimation(anim, bResume, 0);
        return;
    }
}

// Dresses a GUI 3D-scene widget with this creature: picks the idle animation from its
// alignment, plays it on the creature's own models and on the widget's scene copy, then attaches
// the widget camera to the right camera hook and nudges it up to head height. The optional
// stand-in answers the identity questions in place of this creature.
void CSWCCreature::Setup3DScene(CSWGui3DSceneView& view, CSWCCreature* pCreature,
                                CSWSCreature* pServer, int bSetAnimation)
{
    int nAlignment = (pCreature != 0 ? pCreature : this)->m_pStats->m_alignmentGoodEvil;
    if (g_nHackAlignment != -1)
        nAlignment = g_nHackAlignment;

    CExoString sCreatureAnim;
    CExoString sSceneAnim;

    if (nAlignment == 0)
        sSceneAnim = "evil";
    else if (nAlignment == 100)
        sSceneAnim = "good";
    else
        sSceneAnim.Format(const_cast<char*>("align%d"), nAlignment / 5 + 1);

    if (pServer == 0 && g_pAppManager->m_pServerExoApp != 0)
    {
        if (pCreature != 0)
            pServer = pCreature->GetServerCreature();
        else
            pServer = GetServerCreature();
    }

#if KOTOR1  // K1 has no HK-50 tag test
    if (pServer != 0 && (pServer->m_sTag == "t3m4" || pServer->m_sTag == "hk47"))
#else
    if (pServer != 0 && (pServer->m_sTag == "t3m4" || pServer->m_sTag == "hk47" ||
                        pServer->m_sTag == "3cfd"))
#endif
    {
        sCreatureAnim = "pause1";
    }
    else if (nAlignment <= 39)
        sCreatureAnim = "evil";
    else if (nAlignment >= 60)
        sCreatureAnim = "good";
    else
        sCreatureAnim = "neutral";

    Gob* pPiece = GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY);
    if (bSetAnimation && pPiece != 0)
        pPiece->PlayAnimation(static_cast<char*>(sCreatureAnim.CStr()), 1.0f, 2, 0.0f);

    pPiece = GetAnimationBase()->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD);
    if (bSetAnimation && pPiece != 0)
        pPiece->PlayAnimation(static_cast<char*>(sCreatureAnim.CStr()), 1.0f, 2, 0.0f);

    pPiece = static_cast<Gob*>(view.m_scene.GetModel(0));
    if (pPiece == 0)
        return;

    float fAnimLength;
    pPiece->GetAnimation(static_cast<char*>(sSceneAnim.CStr()), 0, &fAnimLength);
#if KOTOR1  // K1 uses the creature's own animation
    if (fAnimLength != -1.0f)
        pPiece->PlayAnimation(static_cast<char*>(sSceneAnim.CStr()), 1.0f, 2, 0.0f);
    else
        pPiece->PlayAnimation(static_cast<char*>(sCreatureAnim.CStr()), 1.0f, 2, 0.0f);
#else
    if (fAnimLength != -1.0f)
    {
        pPiece->PlayAnimation(static_cast<char*>(sSceneAnim.CStr()), 1.0f, 2, 0.0f);
    }
    else if (sCreatureAnim.CompareNoCase("pause1") == 0)
    {
        pPiece->PlayAnimation(static_cast<char*>(sCreatureAnim.CStr()), 1.0f, 2, 0.0f);
    }
    else
    {
        if (nAlignment <= 39)
            sSceneAnim = "evil";
        else if (nAlignment >= 60)
            sSceneAnim = "good";
        else
            sSceneAnim = "neutral";
        pPiece->PlayAnimation(static_cast<char*>(sSceneAnim.CStr()), 1.0f, 2, 0.0f);
    }
#endif

    CExoString sCameraHook;
    if (pServer != 0)
    {
#if KOTOR1
        if (pServer->m_sTag == "t3m4")
#else
        if (pServer->m_sTag == "t3m4" || pServer->m_sTag == "3cfd")
#endif
            sCameraHook = "camerahookt";
        else if (pServer->m_sTag == "hk47")
            sCameraHook = "camerahookh";
        else if (pServer->m_sTag == "zaalbar")
            sCameraHook = "camerahookz";
        else
            sCameraHook = "camerahook";
    }
    else
    {
        sCameraHook = "camerahook";
    }

    if (pPiece->GetPart(static_cast<char*>(sCameraHook.CStr())) == 0)
        sCameraHook = "camerahook";

    view.m_scene.m_pCamera->AttachToObject(static_cast<CAurObject*>(pPiece), static_cast<char*>(sCameraHook.CStr()), 1);
    view.m_scene.m_pCamera->SetViewAngle(22.726f);

    CAurBehaviorAttachTranslate* pBehavior =
        static_cast<CAurBehaviorAttachTranslate*>(view.m_scene.m_pCamera->GetBehavior(kBehaviorAttachTranslate));
    if (pBehavior != 0)
    {
        pBehavior->m_bUnrotatedTranslation = 1;
        float fHeadHeight = GetHeadHeightOffset();
        pBehavior->m_vTranslation.x = 0.0f;
        pBehavior->m_vTranslation.y = 0.0f;
        pBehavior->m_vTranslation.z = fHeadHeight;
    }
}

// Swaps the creature's animation base for one of the requested shape and makes sure the named
// model resource is resident. Each shape allocates its own class size, and the head, wielder and
// head-wielder shapes reach their shared sub-object through the virtual base offset.
int CSWCCreature::RequestModel(const CResRef& r1, const CResRef& r2, unsigned char a)
{
    if (m_pModel == 0 || m_pModel->m_nAnimBaseType != a) {
        CSWCAnimBase* pNew;
        switch (a) {
        case CSWCAnimBase::ANIM_BASE_TYPE_PLAIN:
            pNew = new CSWCAnimBase();
            break;
        case CSWCAnimBase::ANIM_BASE_TYPE_HEAD:
            pNew = new CSWCAnimBaseHead();
            break;
        case CSWCAnimBase::ANIM_BASE_TYPE_WIELD:
            pNew = new CSWCAnimBaseWield();
            break;
        case CSWCAnimBase::ANIM_BASE_TYPE_HEAD_WIELD:
            pNew = new CSWCAnimBaseHeadWield();
            break;
        case CSWCAnimBase::ANIM_BASE_TYPE_TAIL_WINGS:
            pNew = new CSWCAnimBaseTW();
            break;
        default:
            return 0;
        }
        m_pNewAnimBase = pNew;
        if (pNew == 0)
            return 0;
        m_pNewAnimBase->Set(0, 1850.0f);
        m_pNewAnimBase->Set(1, 6600.0f);
        m_pNewAnimBase->Set(2, 0.925f);
        m_pNewAnimBase->Set(3, 3.3f);
        m_fPendingMoveSpeedScale = 0;
        if (m_pModel == 0) {
            m_pModel = m_pNewAnimBase;
            m_fMoveSpeedScale = 0.0f;
            m_pNewAnimBase = 0;
        }
    }
    CExoString s;
    r1.CopyToString(s);
    if (!CAuroraInterface::IsResident(static_cast<char*>(s.CStr()))) {
        CAuroraInterface::RequestModel(static_cast<char*>(s.CStr()));
        r2.CopyToString(s);
#if KOTOR1  // K1: the length result is unused
        s.GetLength();
#endif
    }
    return 1;
}

// Loads or swaps the creature's model and animation base. A pending animation base is promoted
// into the model first; the requested shape is built when the live base's kind byte differs. On
// the higher thresholds the fresh model gets the creature as external data, the head-height
// offset, the hologram and rendering flags and the alignment-driven force-glow colour.
int CSWCCreature::LoadModel(const CResRef& r, unsigned char a, unsigned char b)
{
    if (m_pNewAnimBase != 0) {
        if (m_pModel != 0) {
            delete m_pModel;
            m_pModel = 0;
        }
        m_pModel = m_pNewAnimBase;
        m_pNewAnimBase = 0;
        m_fMoveSpeedScale = m_fPendingMoveSpeedScale;
        m_fPendingMoveSpeedScale = 0.0f;
    }
    if (m_pModel != 0 && m_pModel->m_nAnimBaseType != b) {
        delete m_pModel;
        m_pModel = 0;
    }
    if (m_pModel == 0) {
        CSWCAnimBase* pNew;
        switch (b) {
        case CSWCAnimBase::ANIM_BASE_TYPE_PLAIN:
            pNew = new CSWCAnimBase();
            break;
        case CSWCAnimBase::ANIM_BASE_TYPE_HEAD:
            pNew = new CSWCAnimBaseHead();
            break;
        case CSWCAnimBase::ANIM_BASE_TYPE_WIELD:
            pNew = new CSWCAnimBaseWield();
            break;
        case CSWCAnimBase::ANIM_BASE_TYPE_HEAD_WIELD:
            pNew = new CSWCAnimBaseHeadWield();
            break;
        case CSWCAnimBase::ANIM_BASE_TYPE_TAIL_WINGS:
            pNew = new CSWCAnimBaseTW();
            break;
        default:
            return 0;
        }
        m_pModel = pNew;
        if (pNew == 0)
            return 0;
        m_pModel->Set(0, 1850.0f);
        m_pModel->Set(1, 6600.0f);
        m_pModel->Set(2, 0.925f);
        m_pModel->Set(3, 3.3f);
        m_fMoveSpeedScale = 0.0f;
    }
    if (!m_pModel->LoadModel(r, a)) {
        char buf[256];
        snprintff(buf, sizeof(buf), "CSWCCreature::LoadModel(): Failed to load creature model '%s'.",
                  r.GetResRefStr());
        return 0;
    }
    if (a >= CSWCAnimBase::ANIM_MODEL_WINGS) {
        Gob* pModelObj = m_pModel->GetModel(a);
        pModelObj->SetExternalData(this);
        pModelObj->SetExternalDataType(CGameObject::OBJECT_TYPE_CREATURE);
        if (a == CSWCAnimBase::ANIM_MODEL_BODY) {
            Vector v = {0.0f, 0.0f, 0.0f};
            Quaternion q;
            RegisterCallbacks();
            if (pModelObj->GetPartLocalPosition(const_cast<char*>("headconjure"), &v, &q)) {
                m_fHeadHeight = v.z + v.z * -0.3f / 2.4f;
            } else {
                m_fHeadHeight = 2.1f;
            }
        }
#if !KOTOR1  // K1 has no hologram, will-not-render or force-glow flags
        CSWSCreature* pServer = GetServerCreature();
        if (pServer != 0) {
            if (pServer->m_bHologram != 0)
                pModelObj->EnableHologram(1);
            if (pServer->m_bWillNotRender != 0)
                pModelObj->EnableRendering(0);
        }
        if (pServer == 0 || pServer->m_bWillNotRender == 0)
            pModelObj->EnableRendering(1);
        if (pServer != 0 && pServer->m_bWillNotRender != 0)
            pModelObj->EnableRendering(0);
        short nAlign = m_pStats->m_alignmentGoodEvil;
        pModelObj->SetAlignmentValue(nAlign);
        float fR, fG, fB, fA;
        if (nAlign >= 61) {
            float t = (nAlign - 60) / 40.0f;
            fR = t * 0.01f + (1.0f - t) * 0.65f;
            fG = t * 0.05f + (1.0f - t) * 0.65f;
            fB = 1.0f;
            fA = t;
        } else if (nAlign >= 41) {
            if (nAlign >= 50) {
                float t = (50 - nAlign) / 10.0f;
                float fV = t + (1.0f - t) * 0.65f;
                fR = fV;
                fG = fV;
                fB = 0.9f;
                fA = t;
            } else {
                float t = (nAlign - 40) / 10.0f;
                float fV = t + (1.0f - t) * 0.65f;
                fR = 1.0f;
                fG = fV;
                fB = fV;
                fA = t;
            }
        } else {
            float t = (40 - nAlign) / 40.0f;
            fR = 1.0f;
            fG = t * 0.01f + (1.0f - t) * 0.65f;
            fB = t * 0.05f + (1.0f - t) * 0.65f;
            fA = t;
        }
        pModelObj->SetForceGlowColor(fR, fG, fB, fA);
        pModelObj->EnableForceGlow(m_pStats->m_nRace != CSWRace::CRACE_DROID);
        pModelObj->SetNonForceBlending(1);
        CSWSCreature* pServer2 = GetServerCreature();
        if (pServer2 != 0)
            pModelObj->SetForcePoints(pServer2->GetMaxForcePoints());
#endif
    }
    return 1;
}

// Probes line of sight with an eye-to-eye walkmesh and scene hit check, with both creatures' body
// and head models on the ignore list. A hit that resolves to a door or a placeable is added to the
// ignore list and the probe re-runs; anything else ends the walk. Returns whether sight is blocked.
int CSWCCreature::CanSee(CSWCObject* pTarget)
{
    if (pTarget == 0)
        return 0;
    Vector vFrom = m_position;
    Vector vTo = pTarget->m_position;
    float fHeight = pTarget->GetCanSeeHeight();
    DefaultHitInfo.ClearOutput();
    DefaultHitInfo.m_bSkipCharactersAndItems = 1;
    DefaultHitInfo.m_ignoreList.m_count = 0;
    DefaultHitInfo.m_ignoreList.Add(static_cast<CAurObject*>(GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)));
    Gob* pModel = GetModel(CSWCAnimBase::ANIM_MODEL_HEAD, 1);
    if (pModel != 0)
        DefaultHitInfo.m_ignoreList.Add(static_cast<CAurObject*>(pModel));
    DefaultHitInfo.m_ignoreList.Add(static_cast<CAurObject*>(pTarget->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)));
    vFrom.z += 1.5f;
    vTo.z += fHeight;
    pModel = pTarget->GetModel(CSWCAnimBase::ANIM_MODEL_HEAD, 1);
    if (pModel != 0)
        DefaultHitInfo.m_ignoreList.Add(static_cast<CAurObject*>(pModel));
    DefaultHitInfo.ClearOutput();
    int nResult = SceneHitCheck(m_pArea->m_pScene, vFrom, vTo, 0, &DefaultHitInfo);
    while (nResult == 2) {
        CSWCObject* pHitObj =
            static_cast<CSWCObject*>(static_cast<Gob*>(DefaultHitInfo.m_hitObject.m_pObject)->GetExternalData());
        nResult = 1;
        if (pHitObj == 0)
            break;
        if (pHitObj->AsSWCDoor() != 0) {
            CSWSDoor* pDoor = pHitObj->AsSWCDoor()->GetServerDoor();
            if (pDoor != 0 && pDoor->m_curOpenState == 0)
                break;
            DefaultHitInfo.m_ignoreList.Add(static_cast<CAurObject*>(pHitObj->AsSWCDoor()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)));
        } else {
            if (pHitObj->AsSWCPlaceable() == 0) {
                nResult = 2;
                break;
            }
            DefaultHitInfo.m_ignoreList.Add(
                static_cast<CAurObject*>(pHitObj->AsSWCPlaceable()->GetModel(CSWCAnimBase::ANIM_MODEL_BODY, 1)));
        }
        DefaultHitInfo.ClearOutput();
        nResult = SceneHitCheck(m_pArea->m_pScene, vFrom, vTo, 0, &DefaultHitInfo);
    }
    return nResult != 1;
}
