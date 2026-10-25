// Swoop-race and turret minigame controller (client side).

#include "appmanager.h"
#include "port/Mac/MacMain.h"
#include "clientexoapp.h"
#include "serverexoapp.h"
#include "globalvartables.h"
#include "swmgobject.h"
#include "swmgtrackfollower.h"
#include "AURORA/aurmodel.h"
#include "exosound.h"
#if KOTOR1
#endif
#include "aurcamera.h"
#include "Quaternion.h"
#include "CExoArrayListImpl.h"
#include "swmgbehavior.h"
#include "AURORA/aurbehavior.h"
#include "port/libcbase.h"
#if KOTOR1  // the model streams are SDL_RWops
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif

#include "nwcarea.h"
#include "partytables.h"
#include "twodimarray.h"
#include "swminigame.h"
#include "resare.h"
#include "aurscene.h"
#include "swcroom.h"
#include "nwcmodule.h"
#include "serveraimaster.h"
#include "nwsscriptevent.h"
#include "nwscreature.h"
#include "nwccreature.h"
#include "nwsarea.h"
#include "nwsmodule.h"
#include "gameobjectarray.h"
#include "nwcplaceable.h"
#include "resgff.h"

// Walkmesh-first line probe, defined with the creature code.
int SceneHitCheck(CAurScene* pScene, Vector vFrom, Vector vTo, Vector* pHit, CHitInfo* pInfo);

// The bullet list's AddPointer, instantiated here so the whole tree shares one copy.
extern template void SafePointerList<CAurObject>::AddPointer(SafePointer<CAurObject>*);

#if !KOTOR1 && defined(K2_FIX_ENGINE_BUGS)
// The track follower's distortion wake latch stays on while any minigame exists. RestartMiniGame
// builds the new game before deleting the old one, so this has to be a count, not a flag.
extern unsigned char g_adjustWake;
static int s_nLiveMiniGames;
#endif

// Drops the vehicle, the enemies, the prototype tracks, the obstacles and the sound source.
CSWMiniGame::~CSWMiniGame()
{
    SetPlayer(0);
    ClearEnemies();
    ClearTracks();
    ClearObstacles();
    delete m_pSound;
#if !KOTOR1 && defined(K2_FIX_ENGINE_BUGS)
    if (--s_nLiveMiniGames == 0)
        g_adjustWake = 0;
#endif
}

// Starts the race: clears the swoop forfeit flag, sends the vehicle and every surviving enemy
// off, and starts the forfeit sound.
void CSWMiniGame::Go()
{
#if KOTOR1  // no swoop-forfeit global, no party-table probes
    CSWMiniGameObjectArray* pArray = g_pAppManager->m_pClientExoApp->GetMGOArray();
    if (m_pPlayer != 0)
        m_pPlayer->Go();
#else
    if (m_type == MINIGAME_TYPE_SWOOP) {
        g_pAppManager->m_pServerExoApp->GetGlobalVariableTable()->SetValueBoolean(
            CExoString(const_cast<char*>("FORFEIT_SWOOPRACE")), 0);
    }
    CSWMiniGameObjectArray* pArray = g_pAppManager->m_pClientExoApp->GetMGOArray();
    if (m_pPlayer != 0) {
        g_pAppManager->m_pServerExoApp->GetPartyTable();
        g_pAppManager->m_pServerExoApp->GetPartyTable();
        g_pAppManager->m_pServerExoApp->GetPartyTable();
        m_pPlayer->Go();
    }
#endif
    for (int i = m_enemyIds.m_count; i-- > 0; ) {
        CSWMiniEnemy* pEnemy =
            static_cast<CSWMiniEnemy*>(pArray->GetMiniGameObject(m_enemyIds.m_pArray[i], CSWMiniGameObjectArray::PROBE_ENEMY));
        if (pEnemy != 0)
            static_cast<CSWTrackFollower*>(pEnemy)->Go();
        else
            m_enemyIds.DelIndex(i);
    }
    if (m_pSound != 0)
        m_pSound->Play(m_resForfeitSound, 8, 0, 1, 0);
}

void CSWMiniGame::SetMovementPerSecond(float mps)
{
    m_movementPerSecond = mps;
}

int CSWMiniGame::SetMouseAxisX(int axis)
{
    if (static_cast<unsigned int>(axis) >= 4)
        axis = m_mouseAxisX;
    else
        m_mouseAxisX = axis;
    return axis;
}

int CSWMiniGame::SetMouseAxisY(int axis)
{
    if (static_cast<unsigned int>(axis) >= 4)
        axis = m_mouseAxisY;
    else
        m_mouseAxisY = axis;
    return axis;
}

int CSWMiniGame::SetBumpPlane(int plane)
{
    if (static_cast<unsigned int>(plane) >= 4)
        plane = m_bumpPlane;
    else
        m_bumpPlane = plane;
    return plane;
}

int CSWMiniGame::SetDOF(int dof)
{
    if (static_cast<unsigned int>(dof) >= 8)
        dof = m_dof;
    else
        m_dof = dof;
    return dof;
}

int CSWMiniGame::SetType(int type)
{
    if (static_cast<unsigned int>(type - 1) >= 2)
        type = m_type;
    else
        m_type = type;
    return type;
}

int CSWMiniGame::SetPlane(int& plane, int value)
{
    if (static_cast<unsigned int>(value) >= 4)
        return plane;
    plane = value;
    return value;
}

int CSWMiniGame::GetAttachType()
{
    if (m_type == MINIGAME_TYPE_SWOOP)
        return 1;
    if (m_type == MINIGAME_TYPE_TURRET)
        return 2;
    return 0;
}

// Replaces the player's vehicle, loading every model named in resModels with the attach flag at
// the same index in flags, and parking it on the track named by resTrack.
CSWMiniPlayer* CSWMiniGame::SetPlayer(CExoArrayList<CResRef>& resModels,
                                     CExoArrayList<unsigned char>& flags,
                                     const CResRef& resTrack)
{
    int nModels = resModels.m_count;
    if (m_pPlayer != 0)
        delete m_pPlayer;
    m_pPlayer = static_cast<CSWTrackFollower*>(new CSWMiniPlayer(this));  // idiom-exempt: FLAT CSWMiniPlayer
    char szName[20];
    for (int i = 0; i < nModels; i++) {
        resModels.m_pArray[i].CopyToString(szName);
        CAurObject* pModel = NewCAurObject(szName, const_cast<char*>(""), 0, 0);
        if (pModel != 0)
            m_pPlayer->AddModel(pModel, flags.m_pArray[i]);
    }
    resTrack.CopyToString(szName);
    CAurObject* pTrack = NewCAurObject(szName, const_cast<char*>(""), 0, 0);
    if (pTrack != 0) {
        SWMGTrackInfo* pProtoTrack = GetProtoTrack(resTrack);
        if (pProtoTrack != 0)
            pTrack->SetPosition(pProtoTrack->m_vPosition);
        pTrack->AttachToScene(m_pArea->m_pScene);
        m_pPlayer->SetTrack(pTrack, GetAttachType());
    }
    SetCamera(m_pCamera);
    return static_cast<CSWMiniPlayer*>(m_pPlayer);
}

// As above, plus the spare camerahook model that a turret's camera rides on; bCameraHook makes
// the hook add itself to the game's attach type.
CSWMiniPlayer* CSWMiniGame::SetPlayer(CExoArrayList<CResRef>& resModels,
                                     CExoArrayList<unsigned char>& flags,
                                     const CResRef& resTrack,
                                     const CResRef& resCameraHook,
                                     unsigned char bCameraHook)
{
    int nModels = resModels.m_count;
    if (m_pPlayer != 0)
        delete m_pPlayer;
    m_pPlayer = static_cast<CSWTrackFollower*>(new CSWMiniPlayer(this));  // idiom-exempt: FLAT CSWMiniPlayer
    char szName[20];
    for (int i = 0; i < nModels; i++) {
        resModels.m_pArray[i].CopyToString(szName);
        CAurObject* pModel = NewCAurObject(szName, const_cast<char*>(""), 0, 0);
        if (pModel != 0)
            m_pPlayer->AddModel(pModel, flags.m_pArray[i]);
    }
    resTrack.CopyToString(szName);
    CAurObject* pTrack = NewCAurObject(szName, const_cast<char*>(""), 0, 0);
    if (pTrack != 0) {
        SWMGTrackInfo* pProtoTrack = GetProtoTrack(resTrack);
        if (pProtoTrack != 0)
            pTrack->SetPosition(pProtoTrack->m_vPosition);
        pTrack->AttachToScene(m_pArea->m_pScene);
        m_pPlayer->SetTrack(pTrack, GetAttachType());
    }
    resCameraHook.CopyToString(szName);
    CAurObject* pCameraHook = NewCAurObject(szName, const_cast<char*>(""), 0, 0);
    if (pCameraHook != 0) {
        pCameraHook->AttachToScene(m_pArea->m_pScene);
        static_cast<CSWMiniPlayer*>(m_pPlayer)->SetCameraHook(pCameraHook, bCameraHook);
    }
    SetCamera(m_pCamera);
    return static_cast<CSWMiniPlayer*>(m_pPlayer);
}

SWMGTrackInfo* CSWMiniGame::GetProtoTrack(int index)
{
    if (index < m_protoTracks.m_count)
        return m_protoTracks.m_pArray[index];
    return 0;
}

void CSWMiniGame::GetClipDist(float* pNear, float* pFar)
{
    if (pNear != 0)
        *pNear = m_clipNear;
    if (pFar != 0)
        *pFar = m_clipFar;
}

void CSWMiniGame::SetClipDist(float fNear, float fFar)
{
    float fMin = fNear > fFar ? fFar : fNear;
    float fMax = fNear > fFar ? fNear : fFar;
    m_clipNear = fMin;
    m_clipFar = fMax;
    if (m_pCamera != 0)
        m_pCamera->SetClipDist(fMin, fMax);
}

void CSWMiniGame::SetViewAngle(float fAngle)
{
    m_viewAngle = fAngle;
    if (m_pCamera != 0)
        m_pCamera->SetViewAngle(fAngle);
}

// Snaps the vehicle onto its track and clears the motion state. The track's position and
// modelhook lookups are made for their side effects only, and only the position and velocity
// components written below are cleared, not the whole vectors.
void CSWMiniGame::SetCurPos(Vector v)
{
    K2_UNUSED(v);
    m_pPlayer->m_pTrack->GetPosition();
    m_pPlayer->m_pTrack->GetPart(const_cast<char*>("modelhook"));
    m_pPlayer->m_pTrack->PlayAnimation(0, 0.0f, 2, 0.0f);
    m_pPlayer->m_pTrack->PlayAnimation(const_cast<char*>("track"), 0.0f, 2, 0.0f);
    m_curPhysicsState.m_vVelocity.x = 0.0f;
    m_prevPhysicsState.m_vVelocity.y = 0.0f;
    m_prevPhysicsState.m_vPosition.x = 0.0f;
    m_prevPhysicsState.m_vPosition.y = 0.0f;
    m_curPhysicsState.m_vPosition.x = 0.0f;
    m_curPhysicsState.m_vPosition.y = 0.0f;
    m_vSteerAxis.x = 0.0f;
    m_vSteerAxis.y = 0.0f;
    m_lateralAccelPerSec = 0.0f;
    m_prevPhysicsState.m_vVelocity.x = 0.0f;
    Vector vZero;
    static_cast<CSWMiniPlayer*>(m_pPlayer)->SetOffset(vZero);
    SetCamera(m_pCamera);
}

// Advances the vehicle one Runge-Kutta step. The stick axis accelerates it and the velocity damps
// in proportion to itself; the axis is an input rather than a constant over the frame, so the
// step samples the previous frame's axis at t, the average of the two at t+h/2, and this frame's
// axis at t+h. The result lands in m_curPhysicsState and the axis is remembered for the next call.
// Turret games then put the z velocity under straight gravity and add two lateral clamps: one for
// a centred stick, one for a hard brake when a fast slide is opposed by the stick.
void CSWMiniGame::AxisAcceleration(const Vector& vAxis, float fDelta)
{
    Vector vAxisPrev = m_vSteerAxis;
    Vector vVel = m_prevPhysicsState.m_vVelocity;
    Vector vPos = m_prevPhysicsState.m_vPosition;
    float  fLateral = m_lateralAccelPerSec;
    float  fDamp = fLateral / m_movementPerSecond;
    Vector vAxisMid = (vAxisPrev + vAxis) * 0.5f;

    Vector k1 = (vAxisPrev * fLateral - vVel * fDamp) * fDelta;
    Vector v1 = vVel + k1 * 0.5f;
    Vector k2 = (vAxisMid * fLateral - v1 * fDamp) * fDelta;
    Vector v2 = vVel + k2 * 0.5f;
    Vector k3 = (vAxisMid * fLateral - v2 * fDamp) * fDelta;
    Vector v3 = vVel + k3;
    Vector k4 = (vAxis * fLateral - v3 * fDamp) * fDelta;

    m_curPhysicsState.m_vVelocity = vVel + k1 * (1.0f / 6.0f) + k2 * (2.0f / 6.0f)
                                  + k3 * (2.0f / 6.0f) + k4 * (1.0f / 6.0f);
    m_curPhysicsState.m_vPosition = vPos + vVel * fDelta * (1.0f / 6.0f) + v1 * fDelta * (2.0f / 6.0f)
                                  + v2 * fDelta * (2.0f / 6.0f) + v3 * fDelta * (1.0f / 6.0f);

#if !KOTOR1  // K1 has the Runge-Kutta step only, for every minigame type
    if (m_type == MINIGAME_TYPE_SWOOP) {
        m_curPhysicsState.m_vVelocity.z = m_prevPhysicsState.m_vVelocity.z + fDelta * -9.8f;
        if (m_curPhysicsState.m_vPosition.z <= 0.0f)
            m_curPhysicsState.m_vVelocity.z = 0.0f;
        if (vAxis.x == 0.0f) {
            m_curPhysicsState.m_vVelocity.x += fDelta * -2.0f * m_curPhysicsState.m_vVelocity.x;
        } else if (__builtin_fabsf(m_curPhysicsState.m_vVelocity.x) > 20.0f) {
            if ((vAxis.x < 0.0f) != (m_curPhysicsState.m_vVelocity.x < 0.0f))
                m_curPhysicsState.m_vVelocity.x += fDelta * -5.0f * m_curPhysicsState.m_vVelocity.x;
        }
    }
#endif

    m_vSteerAxis = vAxis;
}

// Applies the stick axis in the form the minigame type expects (swoop: y negated, no z; turret:
// all three axes flipped) and hands it to whichever physics model m_nPhysicsModel selects: accelerating
// toward the stick, or moving the vehicle with it directly.
void CSWMiniGame::UpdateAxis(Vector& vAxis, float fDelta)
{
    if (m_type == MINIGAME_TYPE_SWOOP) {
        float fY = vAxis.y;
        float fZ = vAxis.z;
        vAxis.y = -fY;
#if KOTOR1  // K1's swoop negates z too
        vAxis.z = -fZ;
#else
        vAxis.z = fZ * 0.0f;
#endif
    } else if (m_type == MINIGAME_TYPE_TURRET) {
        float fX = vAxis.x;
        float fY = vAxis.y;
        float fZ = vAxis.z;
        vAxis.x = -fZ;
        vAxis.y = -fY;
        vAxis.z = -fX;
    } else {
        return;   // no other type steers
    }
    if (m_nPhysicsModel == 1)
        AxisAcceleration(vAxis, fDelta);
    else if (m_nPhysicsModel == 0)
        m_curPhysicsState.m_vPosition = m_prevPhysicsState.m_vPosition + vAxis * m_movementPerSecond * fDelta;
}

// Feeds a two-axis stick through the same type remap and physics dispatch as UpdateAxis, with the
// second input as z.
void CSWMiniGame::UpdateJoystick(float fX, float fY, float fDelta)
{
    Vector v(fX, 0.0f, fY);
    if (m_type == MINIGAME_TYPE_SWOOP) {
        v.y = -v.y;
#if KOTOR1  // K1's swoop negates z too
        v.z = -v.z;
#else
        v.z = v.z * 0.0f;
#endif
    } else if (m_type == MINIGAME_TYPE_TURRET) {
        v = Vector(-v.z, -v.y, -v.x);
    } else {
        return;   // no other type steers
    }
    if (m_nPhysicsModel == 1)
        AxisAcceleration(v, fDelta);
    else if (m_nPhysicsModel == 0)
        m_curPhysicsState.m_vPosition = m_prevPhysicsState.m_vPosition + v * m_movementPerSecond * fDelta;
}

// Steers by mouse. Each configured axis names which position component it drives, and a negative
// axis id both picks that component and flips the direction, so one setting covers both ways.
void CSWMiniGame::UpdateMouse(float fX, float fY, float fDelta)
{
    float fScale = m_movementPerSecond * fDelta;
    float fAmount = fScale * fX;
    float fAmountY = fScale * fY;
    int nAxis = m_mouseAxisX;
    if (nAxis < 0) {
        fAmount = -fAmount;
        nAxis = -nAxis;
    }
    float* pfAxis = 0;
    switch (nAxis) {
    case MINIGAME_AXIS_X: pfAxis = &m_curPhysicsState.m_vPosition.x; break;
    case MINIGAME_AXIS_Y: pfAxis = &m_curPhysicsState.m_vPosition.y; break;
    case MINIGAME_AXIS_Z: pfAxis = &m_curPhysicsState.m_vPosition.z; break;
    }
    if (pfAxis != 0)
        *pfAxis -= fAmount;

    nAxis = m_mouseAxisY;
    if (nAxis < 0) {
        fAmountY = -fAmountY;
        nAxis = -nAxis;
    }
    pfAxis = 0;
    switch (nAxis) {
    case MINIGAME_AXIS_X: pfAxis = &m_curPhysicsState.m_vPosition.x; break;
    case MINIGAME_AXIS_Y: pfAxis = &m_curPhysicsState.m_vPosition.y; break;
    case MINIGAME_AXIS_Z: pfAxis = &m_curPhysicsState.m_vPosition.z; break;
    }
    if (pfAxis != 0)
        *pfAxis -= fAmountY;
}

// Per-frame tick: advances the vehicle's physics and rolls the new state over the previous one,
// then for a swoop turns the accumulated steering into a bank angle. With the stick off centre
// the bank steps up to a 0.25 limit; with the stick centred it snaps to the nearest of four
// detents and decays back to level. The bank is finally mapped onto a ten-step roll and handed to
// the vehicle's position adjustment.
void CSWMiniGame::Control(float fDelta)
{
    if (m_pPlayer != 0) {
        static_cast<CSWMiniPlayer*>(m_pPlayer)->Control(fDelta, m_curPhysicsState, m_prevPhysicsState);
        m_prevPhysicsState = m_curPhysicsState;
    }

    int nSign = (m_fBankAngle < 0.0f) ? -1 : 1;
    if (m_type != MINIGAME_TYPE_SWOOP)
        return;

    float fAbs = __builtin_fabsf(m_fBankAngle);
    if (m_vSteerAxis.x != 0.0f) {
        float fStep = m_vSteerAxis.x * fDelta;
        m_bBankDetentLatched = 0;
        if ((m_vSteerAxis.x < 0.0f && m_fBankAngle < 0.0f) ||
            (m_fBankAngle > 0.0f && m_vSteerAxis.x > 0.0f)) {
            if (fAbs < 0.25f) {
                float fNew = m_fBankAngle + fStep;
                m_fBankAngle = fNew;
                if (fNew >= 0.25f)
                    m_fBankAngle = 0.255f;
                else if (fNew <= -0.25f)
                    m_fBankAngle = -0.255f;
            }
        } else {
            m_fBankAngle = fStep;
        }
    } else if (m_fBankAngle != 0.0f) {
        float fSign = static_cast<float>(nSign);
        if (m_bBankDetentLatched == 0) {
            if (fAbs >= 0.23f && fAbs < 0.235f)
                m_fBankAngle = fSign * 0.235f;
            else if (fAbs >= 0.17f && fAbs < 0.175f)
                m_fBankAngle = fSign * 0.175f;
            else if (fAbs >= 0.1f && fAbs < 0.105f)
                m_fBankAngle = fSign * 0.105f;
            else if (fAbs >= 0.042f && fAbs < 0.047f)
                m_fBankAngle = fSign * 0.042f;
            m_bBankDetentLatched = 1;
        }
        float fStep = fSign * fDelta;
        m_fBankAngle = (__builtin_fabsf(fStep) > __builtin_fabsf(m_fBankAngle)) ? 0.0f : m_fBankAngle - fStep;
    }

    float fValue = m_fBankAngle;
    int nAmount;
    float fBank = __builtin_fabsf(fValue);
    if (fBank >= 0.25f)
        nAmount = nSign * 10;
    else if (fBank >= 0.24f)
        nAmount = nSign * 9;
    else if (fBank >= 0.23f)
        nAmount = nSign * 8;
    else if (fBank >= 0.2f)
        nAmount = nSign * 7;
    else if (fBank >= 0.17f)
        nAmount = nSign * 6;
    else if (fBank >= 0.135f)
        nAmount = nSign * 5;
    else if (fBank >= 0.1f)
        nAmount = nSign * 4;
    else if (fBank >= 0.071f)
        nAmount = nSign * 3;
    else if (fBank >= 0.042f)
        nAmount = nSign * 2;
    else if (fBank >= 0.021f)
        nAmount = nSign;
    else
        nAmount = 0;
    if (m_pPlayer != 0)
        static_cast<CSWMiniPlayer*>(m_pPlayer)->AdjustPosition(nAmount, 0);
}

SWMGTrackInfo* CSWMiniGame::GetProtoTrack(const CResRef& resTrack)
{
    int nNumTracks = m_protoTracks.m_count;
    for (int i = 0; i < nNumTracks; i++) {
        SWMGTrackInfo* pTrack = m_protoTracks.m_pArray[i];
        if (resTrack == pTrack->m_resTrack)
            return pTrack;
    }
    return 0;
}

CSWMGObstacle* CSWMiniGame::GetObstacle(int index)
{
    if (index < m_obstacleIds.m_count)
        return static_cast<CSWMGObstacle*>(g_pAppManager->m_pClientExoApp->GetMGOArray()
            ->GetMiniGameObject(m_obstacleIds.m_pArray[index], CSWMiniGameObjectArray::PROBE_OBSTACLE));
    return 0;
}

CSWMiniEnemy* CSWMiniGame::GetEnemy(int index)
{
    if (index < m_enemyIds.m_count)
        return static_cast<CSWMiniEnemy*>(g_pAppManager->m_pClientExoApp->GetMGOArray()
            ->GetMiniGameObject(m_enemyIds.m_pArray[index], CSWMiniGameObjectArray::PROBE_ENEMY));
    return 0;
}

void CSWMiniGame::SetupSwoopUpgrades()
{
    g_pAppManager->m_pServerExoApp->GetPartyTable();
    g_pAppManager->m_pServerExoApp->GetPartyTable();
    g_pAppManager->m_pServerExoApp->GetPartyTable();
}

CSWMiniPlayer* CSWMiniGame::SetPlayer(CSWMiniPlayer* pPlayer)
{
    if (m_pPlayer != 0)
        delete m_pPlayer;
    m_pPlayer = static_cast<CSWTrackFollower*>(pPlayer);  // CSWMiniPlayer is laid out flat over CSWTrackFollower
    SetCamera(m_pCamera);
    return static_cast<CSWMiniPlayer*>(m_pPlayer);
}

int CSWMiniGame::ClearTracks()
{
    int nNumTracks = m_protoTracks.m_count;
    for (int i = nNumTracks; i > 0; i--) {
        SWMGTrackInfo* pTrack = m_protoTracks.m_pArray[i - 1];
        if (pTrack != 0)
            delete pTrack;
    }
    m_protoTracks.Clear();
    return nNumTracks;
}

int CSWMiniGame::ClearEnemies()
{
    int nCount = m_enemyIds.m_count;
    CSWMiniGameObjectArray* pArray = g_pAppManager->m_pClientExoApp->GetMGOArray();
    for (int i = 0; i < nCount; i++) {
        CSWMiniEnemy* pEnemy = static_cast<CSWMiniEnemy*>(pArray->GetMiniGameObject(m_enemyIds.m_pArray[i], CSWMiniGameObjectArray::PROBE_ENEMY));
        if (pEnemy != 0)
            delete static_cast<CSWTrackFollower*>(pEnemy);
    }
    m_enemyIds.Clear();
    return nCount;
}

int CSWMiniGame::ClearObstacles()
{
    int nCount = m_obstacleIds.m_count;
    CSWMiniGameObjectArray* pArray = g_pAppManager->m_pClientExoApp->GetMGOArray();
    for (int i = 0; i < nCount; i++) {
        CSWMGObstacle* pObstacle =
            static_cast<CSWMGObstacle*>(pArray->GetMiniGameObject(m_obstacleIds.m_pArray[i], CSWMiniGameObjectArray::PROBE_OBSTACLE));
        if (pObstacle != 0)
            delete pObstacle;
    }
    m_obstacleIds.Clear();
    return nCount;
}

// Stops the vehicle, every enemy and the forfeit sound.
void CSWMiniGame::Stop()
{
    CSWMiniGameObjectArray* pArray = g_pAppManager->m_pClientExoApp->GetMGOArray();
    if (m_pPlayer != 0)
        m_pPlayer->Stop();
    for (int i = m_enemyIds.m_count; i-- > 0; ) {
        CSWMiniEnemy* pEnemy =
            static_cast<CSWMiniEnemy*>(pArray->GetMiniGameObject(m_enemyIds.m_pArray[i], CSWMiniGameObjectArray::PROBE_ENEMY));
        if (pEnemy != 0)
            static_cast<CSWTrackFollower*>(pEnemy)->Stop();
        else
            m_enemyIds.DelIndex(i);
    }
    if (m_pSound != 0)
        m_pSound->Pause();
}

// Resumes the vehicle, every enemy and the forfeit sound.
void CSWMiniGame::Resume()
{
    CSWMiniGameObjectArray* pArray = g_pAppManager->m_pClientExoApp->GetMGOArray();
    if (m_pPlayer != 0)
        m_pPlayer->Resume();
    for (int i = m_enemyIds.m_count; i-- > 0; ) {
        CSWMiniEnemy* pEnemy =
            static_cast<CSWMiniEnemy*>(pArray->GetMiniGameObject(m_enemyIds.m_pArray[i], CSWMiniGameObjectArray::PROBE_ENEMY));
        if (pEnemy != 0)
            static_cast<CSWTrackFollower*>(pEnemy)->Resume();
        else
            m_enemyIds.DelIndex(i);
    }
    if (m_pSound != 0)
        m_pSound->Resume();
}

// Index of an object id in an id list. A file-local helper rather than a list member, since the
// list template has no index-returning finder.
static int FindMiniGameObjectId(const CExoArrayList<unsigned int>& list, unsigned int nObjectId)
{
    for (int i = 0; i < list.m_count; i++)
        if (list.m_pArray[i] == nObjectId)
            return i;
    return -1;
}

CSWMiniEnemy* CSWMiniGame::RemoveEnemy(CSWMiniEnemy* pEnemy)
{
    unsigned int nObjectId = static_cast<CSWMiniGameObject*>(pEnemy)->m_nObjectId;
    int i = FindMiniGameObjectId(m_enemyIds, nObjectId);
    if (i != -1) {
        m_enemyIds.DelIndex(i);
        return pEnemy;
    }
    return 0;
}

// Unregisters the obstacle and detaches its model from the scene.
CSWMGObstacle* CSWMiniGame::RemoveObstacle(CSWMGObstacle* pObstacle)
{
    unsigned int nObjectId = static_cast<CSWMiniGameObject*>(pObstacle)->m_nObjectId;
    int i = FindMiniGameObjectId(m_obstacleIds, nObjectId);
    if (i != -1) {
        m_obstacleIds.DelIndex(i);
        pObstacle->m_pGob.m_pObject->AttachToScene(0);
        return pObstacle;
    }
    return 0;
}

SWMGTrackInfo* CSWMiniGame::AddTrack(const CResRef& resTrack, const Vector& vPosition)
{
    int nNumTracks = m_protoTracks.m_count;
    for (int i = 0; i < nNumTracks; i++) {
        SWMGTrackInfo* pTrack = m_protoTracks.m_pArray[i];
        if (pTrack->m_resTrack == resTrack)
            return pTrack;
    }
    SWMGTrackInfo* pTrack = new SWMGTrackInfo;
    pTrack->m_resTrack = resTrack;
    pTrack->m_vPosition = vPosition;
    m_protoTracks.Add(pTrack);
    return pTrack;
}

// The world axis the current bump plane measures against.
Vector CSWMiniGame::BumpPlane(const Quaternion& qRot)
{
    switch (m_bumpPlane) {
    case MINIGAME_AXIS_X:
        return Vector(1.0f - 2.0f * (qRot.y * qRot.y + qRot.z * qRot.z),
                      2.0f * (qRot.x * qRot.y + qRot.z * qRot.w),
                      2.0f * (qRot.z * qRot.x - qRot.y * qRot.w));
    case MINIGAME_AXIS_Y:
        return Vector(2.0f * (qRot.x * qRot.y - qRot.w * qRot.z),
                      1.0f - 2.0f * (qRot.x * qRot.x + qRot.z * qRot.z),
                      2.0f * (qRot.x * qRot.w + qRot.y * qRot.z));
    case MINIGAME_AXIS_Z:
        return Vector(2.0f * (qRot.x * qRot.z + qRot.w * qRot.y),
                      2.0f * (qRot.z * qRot.y - qRot.x * qRot.w),
                      1.0f - 2.0f * (qRot.x * qRot.x + qRot.y * qRot.y));
    default:
        return Vector(0.0f, 0.0f, 0.0f);
    }
}

// Bullets are held through SafePointers so that a bullet destroyed elsewhere self-nulls and drops
// out of the list on the next pass.
void CSWMiniGame::AddBullet(CAurObject* pBullet)
{
    m_bullets.Add(new SafePointer<CAurObject>(pBullet));
}

// Drops every list slot that still points at this bullet.
void CSWMiniGame::RemoveBullet(CAurObject* pBullet)
{
    for (int i = m_bullets.m_count; i-- > 0; ) {
        SafePointer<CAurObject>* pSafe = m_bullets.m_pArray[i];
        if (pSafe->m_pObject == pBullet) {
            delete pSafe;
            m_bullets.DelIndex(i);
        }
    }
}

void CSWMiniGame::UpdateBulletParent(CSWTrackFollower* pFollower)
{
    int nBullets = m_bullets.m_count;
    for (int i = 0; i < nBullets; i++) {
        CAurObject* pBullet = m_bullets.m_pArray[i]->m_pObject;
        CSWMGBehaviorBullet* pBehavior = static_cast<CSWMGBehaviorBullet*>(pBullet->GetBehavior(BEHAVIOR_TYPE_MGBULLET));
        if (pBehavior != 0 && pBehavior->m_pTrackFollower == pFollower)
            pBehavior->m_pTrackFollower = 0;
    }
}

// Attaches the camera to the vehicle. A swoop rides the camerahook part of the first model that
// has one, falling back to the track so a bike without a hook still gets a rail camera; a turret
// rides its separate camerahook model. The clip and view settings are re-pushed so a camera that
// was just moved keeps what SetClipDist and SetViewAngle stored.
CAurCamera* CSWMiniGame::SetCamera(CAurCamera* pCamera)
{
    m_pCamera = pCamera;
    if (pCamera != 0 && m_pPlayer != 0 && m_pPlayer->m_pTrack != 0)
    {
        int nModels = m_pPlayer->m_models.m_count;
        if (m_type == MINIGAME_TYPE_TURRET)
        {
            m_pCamera->AttachToObject(static_cast<CSWMiniPlayer*>(m_pPlayer)->m_pCameraHook,
                                      const_cast<char*>("camerahook"), 0);
        }
        else if (m_type == MINIGAME_TYPE_SWOOP)
        {
            CAurObject* pModel = 0;
            Part* pPart = 0;
            for (int i = 1; pPart == 0 && i <= nModels; i++)
            {
                pModel = m_pPlayer->GetModel(i - 1);
                pPart = 0;
                if (pModel != 0)
                    pPart = pModel->GetPart(const_cast<char*>("camerahook"));
            }
            if (pPart != 0)
            {
                CAurCamera* pCam = m_pCamera;
                pCam->SetBehavior(new CAurBehaviorAttach(pCam->AsCAurObject(), pModel, pPart));
            }
            else
            {
                pModel = m_pPlayer->m_pTrack;
                if (pModel != 0)
                {
                    pPart = pModel->GetPart(const_cast<char*>("camerahook"));
                    if (pPart != 0)
                    {
                        CAurCamera* pCam = m_pCamera;
                        pCam->SetBehavior(new CAurBehaviorAttach(pCam->AsCAurObject(), pModel, pPart));
                    }
                }
            }
        }
        m_pCamera->SetClipDist(m_clipNear, m_clipFar);
        m_pCamera->SetViewAngle(m_viewAngle);
    }
    return m_pCamera;
}

// Spawns an obstacle model at vPosition and registers it. A model that fails to load is not
// registered, so the id list only ever names objects that are in the scene.
CSWMGObstacle* CSWMiniGame::AddObstacle(const CResRef& resName, const Vector& vPosition)
{
    char szName[20];
    resName.CopyToString(szName);
    CAurObject* pObject = NewCAurObject(szName, const_cast<char*>(""), 0, 0);
#if KOTOR1  // K1 returns an uninitialised pointer when the model could not be created
    CSWMGObstacle* pObstacle;
#ifdef K2_FIX_ENGINE_BUGS
    pObstacle = 0;
#endif
    if (pObject != 0) {
        pObject->SetPosition(vPosition);
        pObject->AttachToScene(m_pArea->m_pScene);
        pObject->PlayAnimation(const_cast<char*>("ready"), 1.0f, 0, 0.0f);

        pObstacle = new CSWMGObstacle(this, pObject);
        m_obstacleIds.Add(pObstacle->m_nObjectId);
    }
    return pObstacle;
#else
    if (pObject == 0)
        return 0;
    pObject->SetPosition(vPosition);
    pObject->AttachToScene(m_pArea->m_pScene);
    pObject->PlayAnimation(const_cast<char*>("ready"), 1.0f, 0, 0.0f);

    CSWMGObstacle* pObstacle = new CSWMGObstacle(this, pObject);
    m_obstacleIds.Add(pObstacle->m_nObjectId);
    return pObstacle;
#endif
}

// Registers an enemy id; registering the same enemy twice is a no-op.
CSWMiniEnemy* CSWMiniGame::AddEnemy(CSWMiniEnemy* pEnemy)
{
    unsigned int nObjectId = pEnemy->m_nObjectId;
    if (m_enemyIds.Contains(nObjectId) == 0)
        m_enemyIds.Add(nObjectId);
    return pEnemy;
}

// Builds an enemy from resModels and resTrack and registers it. The attach flag is always 0.
CSWMiniEnemy* CSWMiniGame::AddEnemy(CExoArrayList<CResRef>& resModels,
                                    CExoArrayList<unsigned char>& flags,
                                    const CResRef& resTrack)
{
    int nModels = resModels.m_count;
    CSWMiniEnemy* pEnemy = new CSWMiniEnemy(this);
    char szName[20];
    for (int i = 0; i < nModels; i++) {
        resModels.m_pArray[i].CopyToString(szName);
        CAurObject* pModel = NewCAurObject(szName, const_cast<char*>(""), 0, 0);
        if (pModel != 0)
            pEnemy->AddModel(pModel, flags.m_pArray[i]);
    }
    resTrack.CopyToString(szName);
    CAurObject* pTrack = NewCAurObject(szName, const_cast<char*>(""), 0, 0);
    if (pTrack != 0) {
        SWMGTrackInfo* pProtoTrack = GetProtoTrack(resTrack);
        if (pProtoTrack != 0)
            pTrack->SetPosition(pProtoTrack->m_vPosition);
        pTrack->AttachToScene(m_pArea->m_pScene);
        pEnemy->SetTrack(pTrack, 0);
    }
    m_enemyIds.Add(pEnemy->m_nObjectId);
    return pEnemy;
}

// Builds the minigame over the given area: clears the motion state, creates the forfeit sound
// source, binds the wrapped resource to the area's own resref, and resolves each minigame type's
// row of the swoopupgrade table. The movement rates are left to Load.
CSWMiniGame::CSWMiniGame(CSWCArea* pArea)
{
#if !KOTOR1 && defined(K2_FIX_ENGINE_BUGS)
    s_nLiveMiniGames++;
#endif
    m_prevPhysicsState.m_vVelocity = Vector();
    m_prevPhysicsState.m_vPosition = Vector();
    m_curPhysicsState.m_vVelocity = Vector();
    m_curPhysicsState.m_vPosition = Vector();
    m_nPhysicsModel = 1;
    m_vSteerAxis = Vector();
#if KOTOR1
    Ios_minigamePaused = 0;
#endif
    m_pCamera = 0;
    m_pPlayer = 0;
    m_pArea = pArea;
    m_bumpPlane = 0;
    m_dof = 0;
    m_type = 0;
    m_mouseAxisX = 0;
    m_mouseAxisY = 0;
    m_fBankAngle = 0.0f;
    m_viewAngle = 65.0f;
    m_clipNear = 0.1f;
    m_clipFar = 100.0f;
    m_pPlayerRoom = 0;
    m_pSound = new CExoStreamingSoundSource(1);
    m_pSound->SetLooping(1);
#if !KOTOR1
    m_fReserved1 = 1.0f;
    m_fReserved2 = 1.0f;
    m_fReserved3 = 1.0f;
#endif
    SetResRef(pArea->m_areaResRef, 0);
#if !KOTOR1
    m_pSwoopUpgrades = new C2DA(CResRef("swoopupgrade"), 0);
    m_pSwoopUpgrades->Load2DArray();
    int nValue = 0;
    m_pSwoopUpgrades->GetINTEntry(g_pAppManager->m_pServerExoApp->GetPartyTable()->m_swoop[0],
                                  "level", &nValue);
    m_swoopUpgrades[0] = nValue;
    nValue = 0;
    m_pSwoopUpgrades->GetINTEntry(g_pAppManager->m_pServerExoApp->GetPartyTable()->m_swoop[1],
                                  "level", &nValue);
    m_swoopUpgrades[1] = nValue;
    nValue = 0;
    m_pSwoopUpgrades->GetINTEntry(g_pAppManager->m_pServerExoApp->GetPartyTable()->m_swoop[2],
                                  "level", &nValue);
    m_swoopUpgrades[2] = nValue;
    nValue = 0;
    m_pSwoopUpgrades->GetINTEntry(g_pAppManager->m_pServerExoApp->GetPartyTable()->m_swoop[3],
                                  "level", &nValue);
    m_swoopUpgrades[3] = nValue;
    nValue = 0;
    m_pSwoopUpgrades->GetINTEntry(g_pAppManager->m_pServerExoApp->GetPartyTable()->m_swoop[4],
                                  "level", &nValue);
    m_swoopUpgrades[4] = nValue;
    nValue = 0;
    m_pSwoopUpgrades->GetINTEntry(g_pAppManager->m_pServerExoApp->GetPartyTable()->m_swoop[5],
                                  "level", &nValue);
    m_swoopUpgrades[5] = nValue;
    m_jumpSpeed = 0.0f;
#endif
}

// Callback run once a bullet's explosion has finished playing. A bullet owns no context, so the
// context argument is ignored.
void BulletDie(CAurObject* pObject, char*, void*)
{
    delete pObject;
}

// Per-frame tick: retires dead minigame objects, then ticks the vehicle, the enemies, the
// obstacles and the in-flight explosions, dropping ids whose object has gone. The vehicle's hit
// check runs afterwards and the room it ends up in becomes the scene's current room. DoHitCheck
// can clear the vehicle, so it is re-tested before the room lookup.
void CSWMiniGame::Update(float fDeltaTime)
{
    CSWMiniGameObjectArray* pArray = g_pAppManager->m_pClientExoApp->GetMGOArray();

    for (int i = 0; i < CSWMiniGameObjectArray::OBJECT_ID_NONE; i++)
    {
        CSWMiniGameObject* pObject = pArray->GetMiniGameObject(i, CSWMiniGameObjectArray::PROBE_ANY);
#if !KOTOR1 && defined(K2_FIX_ENGINE_BUGS)
        // only a dead follower is flagged here (see CSWTrackFollower::OnDeath)
        if (pObject != 0 && pObject->m_bPendingDelete != 0 && pObject->AsFollower() != 0) {
            CSWTrackFollower* pFollower = static_cast<CSWTrackFollower*>(pObject->AsFollower());
            // deleting through the follower lets its gun banks finish a dying gun's animation
            pFollower->m_flags |= 2;
            delete pFollower;
        }
#else
#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(push)
#pragma warning(disable : 5205)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-abstract-non-virtual-dtor"
#endif
        if (pObject != 0 && pObject->m_bPendingDelete != 0)
            delete pObject;
#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif
#endif
    }

    if (m_pPlayer != 0)
        m_pPlayer->Update(fDeltaTime);

    for (int i = m_enemyIds.m_count - 1; i >= 0; i--)
    {
        CSWMiniGameObject* pEnemy = pArray->GetMiniGameObject(m_enemyIds.m_pArray[i], CSWMiniGameObjectArray::PROBE_ENEMY);
        if (pEnemy != 0)
            pEnemy->Update(fDeltaTime);
        else
            m_enemyIds.DelIndex(i);
    }

    for (int i = m_obstacleIds.m_count - 1; i >= 0; i--)
    {
        CSWMiniGameObject* pObstacle = pArray->GetMiniGameObject(m_obstacleIds.m_pArray[i], CSWMiniGameObjectArray::PROBE_OBSTACLE);
        if (pObstacle != 0)
            pObstacle->Update(fDeltaTime);
        else
            m_obstacleIds.DelIndex(i);
    }

    for (int i = m_explodeBullets.m_count - 1; i >= 0; i--)
    {
        SWMGExplodeBullet* pExplode = m_explodeBullets.m_pArray[i];
        CAurObject* pObject = pExplode->m_spObject;
        pObject->SetPosition(pExplode->m_vHitPosition);
        pObject->SetOrientation(pExplode->m_qOrientation);
        delete pExplode;
        m_explodeBullets.DelIndex(i);
    }

#if KOTOR1  // K1 reads the vehicle's flags without the outer null test
#ifdef K2_FIX_ENGINE_BUGS
    if (m_pPlayer != 0 && (m_pPlayer->m_flags & 1) != 0)
#else
    if ((m_pPlayer->m_flags & 1) != 0)
#endif
        DoHitCheck();

    if (m_pPlayer != 0 && m_pPlayer->m_models.m_count != 0)
    {
        CSWCRoom* pRoom = m_pArea->GetRoom(m_pPlayer->GetPosition());
        if (pRoom != 0)
            m_pPlayerRoom = pRoom;
    }
#else
    if (m_pPlayer != 0)
    {
        if ((m_pPlayer->m_flags & 1) != 0)
            DoHitCheck();

        if (m_pPlayer != 0 && m_pPlayer->m_models.m_count != 0)
        {
            CSWCRoom* pRoom = m_pArea->GetRoom(m_pPlayer->GetPosition());
            if (pRoom != 0)
                m_pPlayerRoom = pRoom;
        }
    }
#endif

    if (m_pArea != 0 && m_pArea->m_pScene != 0 && m_pPlayerRoom != 0)
        m_pArea->m_pScene->SetCurrentRoom(m_pPlayerRoom->m_nSceneRoomIndex);
}

// Sweeps a bullet against the object it was fired at: the renderer checks the parts between the
// bullet's previous and current positions, and on a hit the bullet's parameters are published onto
// the target's hit record. In the modules where the minigame does not shoot, a hit only destroys
// the bullet; anywhere else the bullet explodes and an SWMGExplodeBullet is queued for the next
// Update.
int CSWMiniGame::HitCheckBullet(CAurObject* pBullet, CAurObject* pObject, CSWMiniGameObject* pHitInfo)
{
    CSWMGBehaviorBullet* pBehavior = static_cast<CSWMGBehaviorBullet*>(pBullet->GetBehavior(BEHAVIOR_TYPE_MGBULLET));
    if (pBehavior == 0)
        return 0;

#if KOTOR1  // K1 tests for a "hitbullet" part
    Part* pPart = pObject->GetPart(const_cast<char*>("hitbullet"));
#else
    Part* pPart = pObject->GetPart(const_cast<char*>("root"));
#endif
    if (pPart == 0)
        return 0;

    Vector vPrevious = pBullet->GetPreviousPosition();
    Vector vCurrent = pBullet->GetPosition();
    if (pObject->HitCheckPart(pPart, vPrevious, vCurrent, 0, &DefaultHitInfo) == 0)
        return 0;

    pHitInfo->m_bullet = pBehavior->m_bullet;
    pHitInfo->m_nLastBulletHitShooter = pBehavior->m_pTrackFollower->m_nObjectId;
    pHitInfo->m_sLastBulletHitPart = DefaultHitInfo.m_pszHitPartName;
    pHitInfo->m_vPosition = DefaultHitInfo.m_hitPosition;
    pHitInfo->OnHitBullet(0);

#if !KOTOR1  // K1 explodes the bullet whatever the module
    if (g_pAppManager->m_pClientExoApp->GetModule()->GetModuleDisplayName().CompareNoCase("505OND") ||
        g_pAppManager->m_pClientExoApp->GetModule()->GetModuleDisplayName().CompareNoCase("module000"))
    {
        delete pBullet;
        return 1;
    }
#endif

    if (pBullet->PlayAnimation(const_cast<char*>("explode"), 1.0f, 3, 0.0f) == 0)
    {
        delete pBullet;
        return 1;
    }

    SWMGExplodeBullet* pExplode = new SWMGExplodeBullet;
#if KOTOR1  // K1 binds the watcher in place
    pExplode->m_spObject.m_pObject = pBullet;
    pBullet->AddPointer(&pExplode->m_spObject);
#else
    pExplode->m_spObject = pBullet;
#endif
    // Quarter turn about x, applied before the bullet's own orientation.
    pExplode->m_qOrientation = pointto(DefaultHitInfo.m_vHitNormal);
    pExplode->m_qOrientation = Quaternion(Vector(1.0f, 0.0f, 0.0f), 1.5707964f) * pExplode->m_qOrientation;
    pExplode->m_vHitPosition = DefaultHitInfo.m_hitPosition;
    pBullet->SetBehavior(0);
    pBullet->RegisterCallBack(BulletDie, const_cast<char*>("doneexplode"), 0, 9999.0f, 0);
    m_explodeBullets.Add(pExplode);
    return 1;
}

// Per-frame bullet hit checks, run while the vehicle is alive. Every live bullet is tested
// against the vehicle, the enemies and the obstacles; a bullet that connects anywhere is retired.
// Survivors are detonated: a walkmesh probe catches a bullet that swept through level geometry,
// and swoop games also sweep the whole area server-side and hand the first object they touch a
// damage event.
#if KOTOR1
// K1 scales the collision radius by the difficulty setting, destroys the bullet on an obstacle hit
// instead of removing the obstacle, and has no walkmesh probe or area sweep.
extern float IosCollisionRadiusMultiplier;
__attribute__((visibility("hidden"))) float g_afIosBulletRadiusScale[3] = { 1.9f, 1.4f, 1.0f };

void CSWMiniGame::DoHitCheck()
{
    CSWMiniGameObjectArray* pArray = g_pAppManager->m_pClientExoApp->GetMGOArray();

    for (int i = m_bullets.m_count - 1; i >= 0; i--)
    {
        SafePointer<CAurObject>* pBulletRef = m_bullets.m_pArray[i];
        CAurObject* pBullet = *pBulletRef;
        if (pBullet == 0)
        {
            delete pBulletRef;
            m_bullets.DelIndex(i);
            continue;
        }

        if (m_pPlayer != 0 && m_pPlayer->DoBulletHitCheck(pBullet))
        {
            delete m_bullets.m_pArray[i];
            m_bullets.DelIndex(i);
            continue;
        }

        int bHit = 0;
        for (int j = m_enemyIds.m_count - 1; j >= 0; j--)
        {
            CSWMiniEnemy* pEnemy =
                static_cast<CSWMiniEnemy*>(pArray->GetMiniGameObject(m_enemyIds.m_pArray[j], CSWMiniGameObjectArray::PROBE_ENEMY));
            if (pEnemy == 0)
            {
                m_enemyIds.DelIndex(j);
                continue;
            }
            unsigned char nDifficulty =
                g_pAppManager->m_pClientExoApp->GetClientOptions()->m_difficultyLevel;
            IosCollisionRadiusMultiplier =
                g_afIosBulletRadiusScale[nDifficulty == 2 ? 2 : (nDifficulty == 1 ? 1 : 0)];
            int bEnemyHit = pEnemy->DoBulletHitCheck(pBullet);
            IosCollisionRadiusMultiplier = 1.0f;
            if (bEnemyHit)
            {
                delete m_bullets.m_pArray[i];
                m_bullets.DelIndex(i);
                bHit = 1;
                break;
            }
        }
        if (bHit != 0)
            continue;

        for (int j = m_obstacleIds.m_count - 1; j >= 0; j--)
        {
            CSWMGObstacle* pObstacle =
                static_cast<CSWMGObstacle*>(pArray->GetMiniGameObject(m_obstacleIds.m_pArray[j], CSWMiniGameObjectArray::PROBE_OBSTACLE));
            if (pObstacle == 0)
            {
                m_obstacleIds.DelIndex(j);
                continue;
            }
            if (HitCheckBullet(pBullet, pObstacle->m_pGob, pObstacle))
            {
                delete m_bullets.m_pArray[i];
                m_bullets.DelIndex(i);
                break;
            }
        }
    }

    for (int j = m_obstacleIds.m_count - 1; j >= 0; j--)
    {
        if (m_pPlayer != 0 && (m_pPlayer->m_flags & 2) == 0)
        {
            CSWMGObstacle* pObstacle =
                static_cast<CSWMGObstacle*>(pArray->GetMiniGameObject(m_obstacleIds.m_pArray[j], CSWMiniGameObjectArray::PROBE_OBSTACLE));
            if (pObstacle != 0)
                m_pPlayer->DoObstacleHitCheck(pObstacle);
            else
                m_obstacleIds.DelIndex(j);
        }
    }

    if (m_pPlayer != 0 && (m_pPlayer->m_flags & 2) == 0)
    {
        for (int i = 1; i <= 3; i++)
        {
            if (m_pPlayer == 0)
                continue;
            if (static_cast<CSWMiniPlayer*>(m_pPlayer)->DoFollowerHitCheck(m_enemyIds, i == 1) == 0)
                break;
        }
    }
}
#else
void CSWMiniGame::DoHitCheck()
{
    CSWMiniGameObjectArray* pArray = g_pAppManager->m_pClientExoApp->GetMGOArray();
    const unsigned int nEventSignal = CServerAIMaster::EVENT_SIGNAL_EVENT;

    for (int i = m_bullets.m_count - 1; i >= 0; i--)
    {
        SafePointer<CAurObject>* pBulletRef = m_bullets.m_pArray[i];
        CAurObject* pBullet = *pBulletRef;
        if (pBullet == 0)
        {
            delete pBulletRef;
            m_bullets.DelIndex(i);
            continue;
        }

        int bHit = 0;
        if (m_pPlayer != 0 && m_pPlayer->DoBulletHitCheck(pBullet))
        {
            delete pBulletRef;
            m_bullets.DelIndex(i);
            bHit = 1;
        }
        else
        {
            for (int j = m_enemyIds.m_count - 1; j >= 0; j--)
            {
                CSWMiniEnemy* pEnemy =
                    static_cast<CSWMiniEnemy*>(pArray->GetMiniGameObject(m_enemyIds.m_pArray[j], CSWMiniGameObjectArray::PROBE_ENEMY));
                if (pEnemy == 0)
                {
                    m_enemyIds.DelIndex(j);
                    continue;
                }
                if (pEnemy->DoBulletHitCheck(pBullet))
                {
                    delete pBulletRef;
                    m_bullets.DelIndex(i);
                    bHit = 1;
                    break;
                }
            }
        }

        if (!bHit)
        {
            for (int j = m_obstacleIds.m_count - 1; j >= 0; j--)
            {
                CSWMGObstacle* pObstacle =
                    static_cast<CSWMGObstacle*>(pArray->GetMiniGameObject(m_obstacleIds.m_pArray[j], CSWMiniGameObjectArray::PROBE_OBSTACLE));
                if (pObstacle == 0)
                {
                    m_obstacleIds.DelIndex(j);
                    continue;
                }
                if (HitCheckBullet(pBullet, pObstacle->m_pGob, pObstacle))
                {
                    pObstacle->OnHitBullet(0);
                    delete pBulletRef;
                    m_bullets.DelIndex(i);
                    pObstacle->m_pGob.m_pObject->DisableAlwaysRender();
                    delete pObstacle;
                    m_obstacleIds.DelIndex(j);
                    bHit = 1;
                    break;
                }
            }
        }

        // Module 107PER does not use explode bullets at all.
        int nModuleName =
            g_pAppManager->m_pClientExoApp->GetModule()->GetModuleDisplayName().CompareNoCase("107PER");
        if (bHit != 0)
            continue;
        if (nModuleName == 0)
            continue;

        if (m_type == MINIGAME_TYPE_TURRET)
        {
            Vector vCur = pBullet->GetPosition();
            Vector vPrev = pBullet->GetPreviousPosition();
            Vector vHitPoint(0.0f, 0.0f, 0.0f);
            CHitInfo hitInfo(0, ~CHitInfo::kMaterialTrigger, 0);

            // A bullet that did not move cannot have swept anything.
            if (vPrev.x != 0.0f || vPrev.y != 0.0f || vPrev.z != 0.0f)
            {
                hitInfo.m_bSkipObjects = 1;
                if (SceneHitCheck(g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_pScene,
                                  vCur, vPrev, &vHitPoint, &hitInfo) != 0)
                {
                    delete pBullet;
                    delete pBulletRef;
                    m_bullets.DelIndex(i);
                    bHit = 1;
                }
            }
        }

        if (bHit != 0)
            continue;

        if (m_type != MINIGAME_TYPE_TURRET)
            continue;

        // The object array lookup here is discarded; ids are resolved through GetGameObject below.
        g_pAppManager->m_pClientExoApp->GetGameObjectArray();

        CSWSArea* pServerArea = g_pAppManager->m_pServerExoApp->GetModule()->GetArea();
        unsigned long long oid;
        for (int fObject = pServerArea->GetFirstObjectInArea(oid);
             fObject != 0;
             fObject = pServerArea->GetNextObjectInArea(oid))
        {
            CSWMGBehaviorBullet* pBehavior = static_cast<CSWMGBehaviorBullet*>(pBullet->GetBehavior(BEHAVIOR_TYPE_MGBULLET));
            if (pBehavior == 0)
                break;

            unsigned int nClientId =
                static_cast<unsigned int>(g_pAppManager->m_pClientExoApp->ServerToClientObjectId(oid));
            CGameObject* pObject = g_pAppManager->m_pClientExoApp->GetGameObject(nClientId);
            if (pObject == 0)
                continue;
            // Only creatures and placeables have a model to hit; a party member is spared.
            if (pObject->AsSWCCreature() == 0 && pObject->AsSWCPlaceable() == 0)
                continue;
            Gob* pModel;
            if (pObject->AsSWCCreature() == 0)
            {
                pModel = pObject->AsSWCPlaceable()->GetModel(CSWCObject::ANIM_MODEL_BODY, 1);
            }
            else
            {
                CSWSCreature* pServerCreature = pObject->AsSWCCreature()->GetServerCreature();
                if (pServerCreature != 0 && pServerCreature->m_bInParty != 0)
                    continue;
                pModel = pObject->AsSWCCreature()->GetModel(CSWCObject::ANIM_MODEL_BODY, 1);
            }
            if (pModel == 0)
                continue;

            CHitInfo aiHit(0, ~CHitInfo::kMaterialTrigger, 0);
            aiHit.m_bHitBoundingBoxes = 1;
            if (pModel->HitCheck(pBullet->GetPreviousPosition(), pBullet->GetPosition(),
                                 0, &aiHit) == 0)
                continue;

            CScriptEvent* pEvent = new CScriptEvent();
            pEvent->m_eventType = CSWSSCRIPTEVENT_EVENTTYPE_ON_USER_DEFINED_EVENT;
            pEvent->SetInteger(0, pBehavior->m_bullet.m_nDamage);
            g_pAppManager->m_pServerExoApp->GetServerAIMaster()->AddEventDeltaTime(
                0, 0, oid, oid, nEventSignal, pEvent);

            delete pBullet;
            delete pBulletRef;
            m_bullets.DelIndex(i);
            bHit = 1;
            break;
        }
    }

    // Obstacles are swept against the vehicle and its followers get three passes at the enemy
    // list, the first of which applies the damage. A wrecked vehicle does neither.
    for (int j = m_obstacleIds.m_count - 1; j >= 0; j--)
    {
        if (m_pPlayer != 0 && (m_pPlayer->m_flags & 2) == 0)
        {
            CSWMGObstacle* pObstacle =
                static_cast<CSWMGObstacle*>(pArray->GetMiniGameObject(m_obstacleIds.m_pArray[j], CSWMiniGameObjectArray::PROBE_OBSTACLE));
            if (pObstacle != 0)
                m_pPlayer->DoObstacleHitCheck(pObstacle);
            else
                m_obstacleIds.DelIndex(j);
        }
    }

    if (m_pPlayer != 0 && (m_pPlayer->m_flags & 2) == 0)
    {
        for (int i = 1; i <= 3; i++)
        {
            if (m_pPlayer == 0)
                continue;
            if (static_cast<CSWMiniPlayer*>(m_pPlayer)->DoFollowerHitCheck(m_enemyIds, i == 1) == 0)
                break;
        }
    }
}

#endif
// Reads the minigame out of the wrapped area's GFF: the "MiniGame" struct's scalars, then the
// player, enemy and obstacle blocks. Only the player block is required; the mouse, enemy and
// obstacle blocks are optional, so a minigame with no obstacles is legal.
int CSWMiniGame::Load()
{
    CResRef resTemp;
    CExoArrayList<CResRef> models;
    CExoArrayList<unsigned char> flags;
    CExoString sName;

    if (m_pRes->Demand() == 0)
        return 0;

    CResStruct sTop;
    m_pRes->GetTopLevelStruct(&sTop);
    CResStruct sMiniGame;
    m_pRes->GetStructFromStruct(&sMiniGame, &sTop, const_cast<char*>("MiniGame"));

    int bSuccess;
    unsigned int nValue = m_pRes->ReadFieldDWORD(&sMiniGame, const_cast<char*>("Type"), bSuccess, 0);
    if (nValue - 1 < 2)
        m_type = nValue;

    // The movement defaults depend on the type already stored, not on the Type field just read.
    m_movementPerSecond = m_pRes->ReadFieldFLOAT(&sMiniGame, const_cast<char*>("MovementPerSec"), bSuccess,
        m_type == MINIGAME_TYPE_TURRET ? 90.0f : (m_type == MINIGAME_TYPE_SWOOP ? 6.0f : 0.0f));
    m_lateralAccelPerSec = m_pRes->ReadFieldFLOAT(&sMiniGame, const_cast<char*>("LateralAccel"), bSuccess, 60.0f);

    nValue = m_pRes->ReadFieldDWORD(&sMiniGame, const_cast<char*>("Bump_Plane"), bSuccess, 0);
    if (nValue <= 3)
        m_bumpPlane = nValue;
    m_bDoBumping = m_pRes->ReadFieldBYTE(&sMiniGame, const_cast<char*>("DoBumping"), bSuccess, 0);
    m_bUseInertia = m_pRes->ReadFieldBYTE(&sMiniGame, const_cast<char*>("UseInertia"), bSuccess, 0);

    nValue = m_pRes->ReadFieldDWORD(&sMiniGame, const_cast<char*>("DOF"), bSuccess, 0);
    if (nValue <= 7)
        m_dof = nValue;

    SetMusic(m_pRes->ReadFieldCResRef(&sMiniGame, const_cast<char*>("Music"), bSuccess, CResRef("")));

    float fNearClip = m_pRes->ReadFieldFLOAT(&sMiniGame, const_cast<char*>("Near_Clip"), bSuccess, 0.1f);
    float fFarClip = m_pRes->ReadFieldFLOAT(&sMiniGame, const_cast<char*>("Far_Clip"), bSuccess, 100.0f);
    SetClipDist(fNearClip, fFarClip);
    SetViewAngle(m_pRes->ReadFieldFLOAT(&sMiniGame, const_cast<char*>("CameraViewAngle"), bSuccess, 65.0f));

    CResStruct sPlayer;
    bSuccess = m_pRes->GetStructFromStruct(&sPlayer, &sMiniGame, const_cast<char*>("Player"));
    if (bSuccess == 0)
        return 0;
    CResList list;
    bSuccess = m_pRes->GetList(&list, &sPlayer, const_cast<char*>("Models"));
    if (bSuccess == 0)
        return 0;
    int nModels = m_pRes->GetListCount(&list);
    for (int i = 0; i < nModels; i++)
    {
        CResStruct sModel;
        m_pRes->GetListElement(&sModel, &list, i);
        resTemp = m_pRes->ReadFieldCResRef(&sModel, const_cast<char*>("Model"), bSuccess, CResRef(""));
        if (resTemp.IsValid() == 0)
            continue;
        models.Add(resTemp);
        flags.Add(m_pRes->ReadFieldBYTE(&sModel, const_cast<char*>("RotatingModel"), bSuccess, 1));
    }
    if (models.m_count == 0)
        return 0;

    resTemp = m_pRes->ReadFieldCResRef(&sPlayer, const_cast<char*>("Track"), bSuccess, CResRef(""));
    if (resTemp.IsValid() == 0)
        return 0;

    CSWMiniPlayer* pPlayer;
    if (m_type == MINIGAME_TYPE_TURRET)
    {
        // The camerahook needs a resref of its own; resTemp still holds the track.
        CResRef resCameraHook;
        resCameraHook = m_pRes->ReadFieldCResRef(&sPlayer, const_cast<char*>("Camera"), bSuccess, CResRef(""));
        if (resCameraHook.IsValid() == 0)
            return 0;
        pPlayer = SetPlayer(models, flags, resTemp, resCameraHook,
            m_pRes->ReadFieldBYTE(&sPlayer, const_cast<char*>("CameraRotate"), bSuccess, 0));
    }
    else
    {
        pPlayer = SetPlayer(models, flags, resTemp);
    }
    models.SetSize(0);
    flags.SetSize(0);
    if (pPlayer == 0)
        return 0;
    pPlayer->Load(m_pRes, &sPlayer);

    // Everything from here on is optional: a failed lookup just skips its block.
    CResStruct sMouse;
    bSuccess = m_pRes->GetStructFromStruct(&sMouse, &sMiniGame, const_cast<char*>("Mouse"));
    if (bSuccess != 0)
    {
        nValue = m_pRes->ReadFieldDWORD(&sMouse, const_cast<char*>("AxisX"), bSuccess, 0);
        if (nValue <= 3)
            m_mouseAxisX = nValue;
        nValue = m_pRes->ReadFieldDWORD(&sMouse, const_cast<char*>("AxisY"), bSuccess, 0);
        if (nValue <= 3)
            m_mouseAxisY = nValue;
        if (m_pRes->ReadFieldBYTE(&sMouse, const_cast<char*>("FlipAxisX"), bSuccess, 0) != 0)
            m_mouseAxisX = -m_mouseAxisX;
        if (m_pRes->ReadFieldBYTE(&sMouse, const_cast<char*>("FlipAxisY"), bSuccess, 0) != 0)
            m_mouseAxisY = -m_mouseAxisY;
    }

    bSuccess = m_pRes->GetList(&list, &sMiniGame, const_cast<char*>("Enemies"));
    if (bSuccess != 0)
    {
        int nEnemies = m_pRes->GetListCount(&list);
        for (int i = 0; i < nEnemies; i++)
        {
            CResStruct sEnemy;
            bSuccess = m_pRes->GetListElement(&sEnemy, &list, i);
            if (bSuccess == 0)
                continue;
            CResList enemyModels;
            bSuccess = m_pRes->GetList(&enemyModels, &sEnemy, const_cast<char*>("Models"));
            if (bSuccess == 0)
                continue;
            int nEnemyModels = m_pRes->GetListCount(&enemyModels);
            for (int j = 0; j < nEnemyModels; j++)
            {
                CResStruct sModel;
                m_pRes->GetListElement(&sModel, &enemyModels, j);
                resTemp = m_pRes->ReadFieldCResRef(&sModel, const_cast<char*>("Model"), bSuccess, CResRef(""));
                if (resTemp.IsValid() == 0)
                    continue;
                models.Add(resTemp);
                flags.Add(m_pRes->ReadFieldBYTE(&sModel, const_cast<char*>("RotatingModel"), bSuccess, 1));
            }
            if (models.m_count == 0)
                continue;
            resTemp = m_pRes->ReadFieldCResRef(&sEnemy, const_cast<char*>("Track"), bSuccess, CResRef(""));
            if (resTemp.IsValid() == 0)
            {
                models.SetSize(0);
                flags.SetSize(0);
                continue;
            }
            CSWMiniEnemy* pEnemy = AddEnemy(models, flags, resTemp);
            models.SetSize(0);
            flags.SetSize(0);
            pEnemy->Load(m_pRes, &sEnemy);
        }
    }

    bSuccess = m_pRes->GetList(&list, &sMiniGame, const_cast<char*>("Obstacles"));
    if (bSuccess != 0)
    {
        int nObstacles = m_pRes->GetListCount(&list);
        for (int i = 0; i < nObstacles; i++)
        {
            CResStruct sObstacle;
            bSuccess = m_pRes->GetListElement(&sObstacle, &list, i);
            if (bSuccess == 0)
                continue;
            resTemp = m_pRes->ReadFieldCResRef(&sObstacle, const_cast<char*>("Name"), bSuccess, CResRef(""));
            if (resTemp.IsValid() == 0)
                continue;
            resTemp.CopyToString(sName);
            // Obstacles are matched by name to the object that already has a model, then the
            // obstacle loads its own GFF half on top.
            CSWMiniGameObject* pObject =
                g_pAppManager->m_pClientExoApp->GetMGOArray()->GetMiniGameObjectByName(sName);
            if (pObject != 0)
                static_cast<CSWMGObstacle*>(pObject)->Load(m_pRes, &sObstacle);
        }
    }

    m_pRes->Release();
    return 1;
}
