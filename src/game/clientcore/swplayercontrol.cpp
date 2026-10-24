#include <stdio.h>
#ifdef K2_HOST_BUILD
#include "swparty.h"
extern "C" int k2host_move_keys_held(void);
#endif
#ifdef K2_QOL_CONTROLLER
#include "port/k2qolapi.h"
#endif
#include <string.h>
#include "swplayercontrol.h"
#include "Vector.h"
#include "Quaternion.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "nwccreature.h"
#include "nwcmodule.h"
#include "nwcarea.h"
#include "twodimarrays.h"
#include "nwsrules.h"
#include "twodimarray.h"
#include "exostring.h"
#include "clientoptions.h"
#include "nwccreatureappearance.h"
#include "nwccreaturestats.h"
#include "nwscreature.h"
#include "nwcanimbase.h"
#include "serverexoapp.h"
#include "aurcamera.h"
#include "nwscriptconst.h"

float fCombatTurnSpeed = 900.0f;
float turbospeed = 15.0f;
int turbo;

void CSWPlayerControl::Initialize()
{
    m_objectId = OBJECT_INVALID;
    m_pCamera = 0;
    m_bEnabled = 0;
}

CSWPlayerControl::CSWPlayerControl()
{
    Initialize();
}

CSWPlayerControl::CSWPlayerControl(unsigned long long id, CAurCamera* pAurCamera)
{
    m_objectId = id;
    m_bEnabled = 0;
    m_pCamera = pAurCamera;
}

// The first of the base's two concrete policies. Both constructors are the base constructor plus
// Init().
void CSWPlayerControlDriving::Init()
{
    m_nMoveMask = 0;
    m_fAcceleration = 10.0f;
    m_fMaxSpeed = 6.0f;
    m_fDeceleration = 15.0f;
    m_nSpeedPair = 0;
    m_nPrevSpeedPair = 0;
    m_nReserved1 = 0x3f800000;
}

CSWPlayerControlDriving::CSWPlayerControlDriving(unsigned long long id, CAurCamera* pAurCamera)
    : CSWPlayerControl(id, pAurCamera)
{
    Init();
}

CSWPlayerControlDriving::CSWPlayerControlDriving()
    : CSWPlayerControl()
{
    Init();
}

// Initializes the policy's tunables, which are the base turn rate, the camera-turn rates from the
// camera-style 2DA row the area selected, and the keyboard drive settings from the client options.
void CSWPlayerControlCamRelative::Init()
{
    m_fTurnRate = 480.0f;

    int nStyle = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_cameraStyle;
    C2DA* pCameraStyle = g_pRules->m_p2DArrays->m_pCameraStyle;

    pCameraStyle->GetFLOATEntry(nStyle, CExoString("MaxTurnRate"), &m_fMaxTurnRate);
    pCameraStyle->GetFLOATEntry(nStyle, CExoString("MinTurnRate"), &m_fMinTurnRate);

    m_vTurn = Vector();

    m_acceleration.m_fMaxSpeed =
        g_pAppManager->m_pClientExoApp->GetClientOptions()->GetCameraKeyboardDPS();
    m_acceleration.m_fAcceleration =
        g_pAppManager->m_pClientExoApp->GetClientOptions()->GetCameraKeyboardAcceleration();
    m_acceleration.m_fDeceleration =
        g_pAppManager->m_pClientExoApp->GetClientOptions()->GetCameraKeyboardDeceleration();
}

void CSWPlayerControlDriving::UpDown(float f)
{
    if (f < 0.0f)
        m_nMoveMask |= MOVE_FORWARD;
    else if (f > 0.0f)
        m_nMoveMask |= MOVE_BACK;
}

void CSWPlayerControlDriving::LeftRight(float f)
{
    if (f > 0.0f)
        m_nMoveMask |= MOVE_STRAFE_RIGHT;
    else if (f < 0.0f)
        m_nMoveMask |= MOVE_STRAFE_LEFT;
}

extern int quaternionidentityoptimization;

// Rotates a vector by a quaternion, skipping the work when the rotation is the identity.
static inline Vector RotateByQuaternion(const Vector& v, const Quaternion& q)
{
    float w = q.w;
    if (quaternionidentityoptimization != 0 && (w == 1.0f || w == -1.0f))
        return v;

    float x = q.x, y = q.y, z = q.z;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, wx = x * w;
    float yz = y * z, wy = y * w, wz = z * w;
    Vector r;
    r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) +
          v.z * ((xz + wy) + (xz + wy));
    r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) +
          v.z * ((yz - wx) + (yz - wx));
    r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) +
          v.z * (1.0f - ((xx + yy) + (xx + yy)));
    return r;
}

// Applies mouse-look yaw. Rotates the creature's own orientation about Z by the given angle,
// applies it, and stores the normalized result in m_vStationaryOrientation.
void CSWPlayerControlDriving::MouseLeftRight(float f)
{
    if (m_objectId == OBJECT_INVALID)
        return;
    if (m_pCamera == 0)
        return;
    if (m_bEnabled == 0)
        return;

    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId);
    if (pCreature == 0)
        return;

    Vector v = pCreature->m_orientation;
    Quaternion q(Vector(0.0f, 0.0f, 1.0f), -f);
    Vector r = RotateByQuaternion(v, q);

    pCreature->SetOrientation(r);
    pCreature->m_vStationaryOrientation = normalize(r);
}

// The per-frame drive integrator. Each axis accelerates while its key is held, is clamped to
// +/-m_fMaxSpeed, and decays back toward zero on a frame where it did not move. Diagonal input is
// scaled by 1/sqrt(2), and the travel is committed along the creature's normalized orientation.
void CSWPlayerControlDriving::Drive(float f)
{
    Vector vMove;

    if (m_objectId == OBJECT_INVALID)
        return;
    if (m_pCamera == 0)
        return;
    if (m_bEnabled == 0)
        return;

    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId);
    if (pCreature == 0)
        return;

    int nMask = m_nMoveMask;
    float fScale = ((nMask & (MOVE_FORWARD | MOVE_BACK)) && (nMask & (MOVE_STRAFE_LEFT | MOVE_STRAFE_RIGHT)))
                       ? 0.70710677f : 1.0f;

    m_prev.m_fPrevForwardSpeed = m_axis.m_fForwardSpeed;
    m_prev.m_fPrevStrafeSpeed = m_axis.m_fStrafeSpeed;

    if (nMask & MOVE_FORWARD)
    {
        m_axis.m_fForwardSpeed += m_fAcceleration * f;
        if (m_axis.m_fForwardSpeed > m_fMaxSpeed)
            m_axis.m_fForwardSpeed = m_fMaxSpeed;
    }
    else if (nMask & MOVE_BACK)
    {
        m_axis.m_fForwardSpeed -= m_fAcceleration * f;
        if (m_axis.m_fForwardSpeed < -m_fMaxSpeed)
            m_axis.m_fForwardSpeed = -m_fMaxSpeed;
    }

    if (nMask & MOVE_STRAFE_LEFT)
    {
        m_axis.m_fStrafeSpeed += m_fAcceleration * f;
        if (m_axis.m_fStrafeSpeed > m_fMaxSpeed)
            m_axis.m_fStrafeSpeed = m_fMaxSpeed;
    }
    else if (nMask & MOVE_STRAFE_RIGHT)
    {
        m_axis.m_fStrafeSpeed -= m_fAcceleration * f;
        if (m_axis.m_fStrafeSpeed < -m_fMaxSpeed)
            m_axis.m_fStrafeSpeed = -m_fMaxSpeed;
    }

    if (m_axis.m_fForwardSpeed == m_prev.m_fPrevForwardSpeed)
    {
        if (m_axis.m_fForwardSpeed > 0.0f)
        {
            m_axis.m_fForwardSpeed -= m_fDeceleration * f;
            if (m_axis.m_fForwardSpeed < 0.0f)
                m_axis.m_fForwardSpeed = 0.0f;
        }
        else if (m_axis.m_fForwardSpeed < 0.0f)
        {
            m_axis.m_fForwardSpeed += m_fDeceleration * f;
            if (m_axis.m_fForwardSpeed > 0.0f)
                m_axis.m_fForwardSpeed = 0.0f;
        }
    }
    if (m_axis.m_fStrafeSpeed == m_prev.m_fPrevStrafeSpeed)
    {
        if (m_axis.m_fStrafeSpeed > 0.0f)
        {
            m_axis.m_fStrafeSpeed -= m_fDeceleration * f;
            if (m_axis.m_fStrafeSpeed < 0.0f)
                m_axis.m_fStrafeSpeed = 0.0f;
        }
        else if (m_axis.m_fStrafeSpeed < 0.0f)
        {
            m_axis.m_fStrafeSpeed += m_fDeceleration * f;
            if (m_axis.m_fStrafeSpeed > 0.0f)
                m_axis.m_fStrafeSpeed = 0.0f;
        }
    }

    vMove = normalize(pCreature->m_orientation) * m_axis.m_fForwardSpeed * f * fScale +
            normalize(cross(Vector(0.0f, 0.0f, 1.0f), pCreature->m_orientation)) * m_axis.m_fStrafeSpeed * f * fScale +
            pCreature->m_position;

    pCreature->SetSWAnimationSpeed(m_axis.m_fForwardSpeed, m_axis.m_fStrafeSpeed);
    if (pCreature->SetPrecalculatedPosition(vMove) == 0)
    {
        m_axis.m_fForwardSpeed = 0.0f;
        m_axis.m_fStrafeSpeed = 0.0f;
    }
    m_nMoveMask = 0;
}

int CSWPlayerControlDriving::Control(float f)
{
    Drive(f);
    return 1;
}

// The second policy, the only one with its own drive-state integrator and a by-value acceleration
// subobject. Both constructors are the base constructor plus a tail call to Init().

CSWPlayerControlCamRelative::CSWPlayerControlCamRelative(unsigned long long id, CAurCamera* pAurCamera)
    : CSWPlayerControl(id, pAurCamera),
      m_vTurn(), m_nReserved1(0), m_nReserved2(0), m_nReserved3(0),
      m_vDriveA(), m_vDriveB(), m_vDriveVel(), m_vDrivePos(), m_vDriveTarget(), m_acceleration()
{
    Init();
}

CSWPlayerControlCamRelative::CSWPlayerControlCamRelative()
    : CSWPlayerControl(),
      m_vTurn(), m_nReserved1(0), m_nReserved2(0), m_nReserved3(0),
      m_vDriveA(), m_vDriveB(), m_vDriveVel(), m_vDrivePos(), m_vDriveTarget(), m_acceleration()
{
    Init();
}

void CSWPlayerControl::SetCamera(CAurCamera* pAurCamera)
{
    m_pCamera = pAurCamera;
}

// The base policy's no-op and zero-return slots, kept out of line so the header stays bodyless.

void CSWPlayerControl::UpDown(float f) { K2_UNUSED(f); }

float CSWPlayerControl::GetUpDown() { return 0.0f; }

void CSWPlayerControl::LeftRight(float f) { K2_UNUSED(f); }

float CSWPlayerControl::GetLeftRight() { return 0.0f; }

void CSWPlayerControl::SetPlayerWalking(int b) { K2_UNUSED(b); }

int CSWPlayerControl::GetPlayerWalking() { return 0; }

void CSWPlayerControl::RunKey() {}

float CSWPlayerControl::GetCurSpeed() { return 0.0f; }

void CSWPlayerControl::MouseLeftRight(float f) { K2_UNUSED(f); }

void CSWPlayerControl::MouseUpDown(float f) { K2_UNUSED(f); }

void CSWPlayerControl::ResetDriveAcceleration() {}

int CSWPlayerControl::Control(float f) { K2_UNUSED(f); return 0; }

// Enables or disables the control, switching the creature between the steering mode and the default
// one.
void CSWPlayerControl::SetEnabled(int bEnabled)
{
    m_bEnabled = bEnabled;

    if (m_objectId == OBJECT_INVALID)
        return;

    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId);
    if (pCreature == 0)
        return;

    if (m_bEnabled)
        pCreature->SwitchMode(CSWCCreature::MODE_SWDRIVE);
    else
        pCreature->SwitchMode(CSWCCreature::MODE_DEFAULT);
}

// Hands the control to a new object id: the outgoing creature returns to MODE_DEFAULT and the
// incoming one goes to MODE_SWDRIVE, both only while the control is enabled.
void CSWPlayerControl::SetPlayer(unsigned long long id)
{
    if (m_objectId != OBJECT_INVALID) {
        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId);
        if (pCreature)
            pCreature->SwitchMode(CSWCCreature::MODE_DEFAULT);
    }

    m_objectId = id;

    if (id != OBJECT_INVALID && m_bEnabled) {
        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(id);
        if (pCreature)
            pCreature->SwitchMode(CSWCCreature::MODE_SWDRIVE);
    }
}


extern int turbo;
extern float turbospeed;

extern int turbo;
extern float turbospeed;

extern float fCombatTurnSpeed;
#include "port/crtapi.h"

// The +Z axis rotated by q, factored so that the (0,0,1) source axis drops out.
static inline Vector ForwardVector(const Quaternion& q)
{
    float w = q.w, x = q.x, y = q.y, z = q.z;
    Vector r;
    r.x = 2.0f * (x * z + y * w);
    r.y = 2.0f * (y * z - x * w);
    r.z = 1.0f - 2.0f * (x * x + y * y);
    return r;
}

// The camera-relative per-frame driver:
// - gates on the object id, the camera, the server app and both creature halves, then on the
//   server-side party and hit-point test;
// - in third-person camera mode, steps the free-look turn velocity and writes the resulting yaw to
//   both the client and the server creature;
// - builds the input axis in the camera's own heading;
// - turns the creature toward that heading, at a rate interpolated between the two turn rates by how
//   fast it is already moving;
// - integrates the drive state with a four-stage Runge-Kutta step carrying Simpson weights, commits
//   the travel along the creature's facing, then updates the animation speed and the lean.
int CSWPlayerControlCamRelative::Control(float fDelta)
{
#ifdef K2_HOST_BUILD
    if (k2host_move_keys_held()) {
        static char s_szLast[256];
        char szWhy[256];
        CSWCCreature* pC = m_objectId == OBJECT_INVALID ? 0
                         : g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId);
        CSWSCreature* pS = pC ? pC->GetServerCreature() : 0;
        unsigned long long nPartyId = OBJECT_INVALID;
        if (CSWCCreature* pLeader = g_pAppManager->m_pClientExoApp->GetSWParty()->GetCharacter(0))
            nPartyId = pLeader->m_objectId;
        snprintf(szWhy, sizeof szWhy,
                 "object %#llx (leader %#llx), camera %d, creature %d, server %d, dead %d, dying %d, "
                 "dead temp %d, up/down %.2f, left/right %.2f",
                 static_cast<unsigned long long>(m_objectId), nPartyId, static_cast<int>(m_pCamera != 0),
                 static_cast<int>(pC != 0), static_cast<int>(pS != 0),
                 pS ? pS->GetDead() : -1,
#if KOTOR1
                 pS ? pS->GetIsPCDying() : -1, pS ? pS->GetDeadTemp() : -1,
#else
                 -1, -1,
#endif
                 m_upDown, m_leftRight);
        if (strcmp(szWhy, s_szLast) != 0) {
            fprintf(stderr, "k2: move control: %s\n", szWhy);
            strcpy(s_szLast, szWhy);
        }
    }
#endif
    if (m_objectId == OBJECT_INVALID || m_pCamera == 0 || g_pAppManager->m_pServerExoApp == 0)
        return 0;

    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId);
// Dereferences pCreature before the null test below, so the test only covers pServer.
    CSWSCreature* pServer = pCreature->GetServerCreature();
    if (pCreature == 0 || pServer == 0)
        return 0;

    if (pServer->GetDead() != 0)
        return 0;
#if KOTOR1  // K1: the dying tests are GetIsPCDying() / GetDeadTemp()
    if (pServer->GetIsPCDying() != 0 || pServer->GetDeadTemp() != 0)
#else
    if (pServer->m_bInParty != 0 && pServer->GetCurrentHitPoints(0) < 1)
#endif
        return 0;

// Clamped with a compare rather than fminf.
    float fStep = fDelta;
    if (fStep > 1.0f)
        fStep = 1.0f;

    // The camera-relative look (third-person camera mode).
    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_FREE_LOOK)
    {
        CSWPhysicsState state = m_acceleration.Update(
            Vector(0.0f, pCreature->m_fFreeLookTurnVel, 0.0f), fStep, 1, 0, 0, 0);
        float fLookYaw = state.m_vDisplacement.y;
        pCreature->m_fFreeLookTurnVel = 0.0f;

        Quaternion qCam = pointto(pCreature->m_orientation);
        Quaternion qLook = YawPitchRoll(fLookYaw + Yaw(qCam), Pitch(qCam), Roll(qCam));

        Vector vFwdA = ForwardVector(qLook);
        pCreature->SetOrientation(vFwdA);
        Vector vFwdB = ForwardVector(qLook);
        pServer->SetOrientation(vFwdB);
    }

    // The input axis, expressed in the camera's heading.
    Vector vAxis(-m_leftRight, m_upDown, 0.0f);
    if (m_leftRight != 0.0f && m_upDown != 0.0f)
        vAxis.Normalize();
    if (m_bPlayerWalking != 0)
        vAxis = vAxis * 0.5f;

    Vector vCamFwd = ForwardVector(m_pCamera->GetOrientation());
    vCamFwd.z = 0.0f;
    if (vCamFwd.x != 0.0f && vCamFwd.y != 0.0f)
        vCamFwd.Normalize();

    Quaternion qHeading = YawPitchRoll(Yaw(vCamFwd), 0.0f, 0.0f);

    pCreature->m_fTurnAngle = 0;

    // The turn direction and its magnitude.
    float fResult = 0.0f;
    bool bAxisZero = vAxis.x == 0.0f && vAxis.y == 0.0f && vAxis.z == 0.0f;
    if (!bAxisZero && m_bEnabled != 0)
    {
        Vector vTurn = RotateByQuaternion(vAxis, qHeading);
        pointto(vTurn);

        float fTurnYaw = Yaw(vTurn);
        float fCreatureYaw = Yaw(pCreature->m_orientation);
        if (fCreatureYaw < 0.0f)
            fCreatureYaw += 360.0f;
        if (fTurnYaw < 0.0f)
            fTurnYaw += 360.0f;

        float fDiff;
        float fSign;
        if (fTurnYaw > fCreatureYaw)
        {
            fDiff = fTurnYaw - fCreatureYaw;
            fSign = fDiff <= 180.0f ? 1.0f : -1.0f;
        }
        else
        {
            fDiff = fCreatureYaw - fTurnYaw;
            fSign = fDiff <= 180.0f ? -1.0f : 1.0f;
        }
        m_fTurnDirection = fSign;

        float fAngle = fDiff > 180.0f ? 360.0f - fDiff : fDiff;
// The field is declared int but holds the float's bits.
        *reinterpret_cast<float*>(&pCreature->m_fTurnAngle) = fSign * fAngle;

        if (__builtin_fabsf(fAngle) > 0.1f && __builtin_fabsf(pCreature->m_swAnimationSpeed) > 0.1f)
        {
            float fSpeedRatio = magnitude(DriveVel()) / GetMaxSpeed();
            float fTurnRate = m_fMinTurnRate + (1.0f - fSpeedRatio) * (m_fMaxTurnRate - m_fMinTurnRate);
            float fTurn = fStep * fTurnRate * m_fTurnDirection;
            if (fTurn > fAngle)
                fTurn = fAngle;
            else if (fTurn < -fAngle)
                fTurn = -fAngle;

            Quaternion qAim = YawPitchRoll(fTurn, 0.0f, 0.0f);
            Vector vAim = RotateByQuaternion(pCreature->m_orientation, qAim);
            Vector vNorm = normalize(vAim);
            if ((pServer->m_nAIState & 2) != 0)
            {
                pCreature->SetOrientation(vNorm);
                pCreature->m_vMovingOrientation = normalize(vNorm);
            }
            TurnVector() = vNorm;
        }
    }

    // The four-stage drive integrator.
    bool bStopped = bAxisZero &&
                    !(DriveVel().x == 0.0f && DriveVel().y == 0.0f) &&
                    __builtin_fabsf(DriveVel().x) < 0.25f && __builtin_fabsf(DriveVel().y) < 0.25f;

    if (bStopped)
    {
        DriveVel() = Vector(0.0f, 0.0f, 0.0f);
        DriveA() = DriveVel();
        DriveB() = DrivePos();
    }
    else
    {
        if (vAxis.x == 0.0f && vAxis.y != 0.0f)
            DriveVel().x = 0.0f;
        if (vAxis.x != 0.0f && vAxis.y == 0.0f)
            DriveVel().y = 0.0f;

// In stage 1, the derivative is target*accel - velocity*accel/maxSpeed, scaled by the step.
        Vector vMid = (vAxis + DriveTarget()) * 0.5f;
        float fAccel = GetAcceleration();
        float fMax = GetMaxSpeed();
        if (fMax <= 1.8f)
            fAccel = fAccel * (fMax / 1.8f);
        float fInvMax = fAccel / fMax;

        Vector k1;
        k1.x = (DriveTarget().x * fAccel - DriveVel().x * fInvMax) * fStep;
        k1.y = (DriveTarget().y * fAccel - DriveVel().y * fInvMax) * fStep;
        k1.z = (DriveTarget().z * fAccel - fInvMax * DriveVel().z) * fStep;
        Vector m1 = DriveVel() + k1 * 0.5f;

        Vector k2;
        fAccel = GetAcceleration();
        fMax = GetMaxSpeed();
        if (fMax <= 1.8f)
            fAccel = fAccel * (fMax / 1.8f);
        fInvMax = fAccel / fMax;
        k2.x = (vMid.x * fAccel - m1.x * fInvMax) * fStep;
        k2.y = (vMid.y * fAccel - m1.y * fInvMax) * fStep;
        k2.z = (vMid.z * fAccel - m1.z * fInvMax) * fStep;
        Vector m2 = DriveVel() + k2 * 0.5f;

        Vector k3;
        fAccel = GetAcceleration();
        fMax = GetMaxSpeed();
        if (fMax <= 1.8f)
            fAccel = fAccel * (fMax / 1.8f);
        fInvMax = fAccel / fMax;
        k3.x = (vMid.x * fAccel - m2.x * fInvMax) * fStep;
        k3.y = (vMid.y * fAccel - m2.y * fInvMax) * fStep;
        k3.z = (vMid.z * fAccel - m2.z * fInvMax) * fStep;

        Vector vEnd = DriveVel() + k3;

        fAccel = GetAcceleration();
        fMax = GetMaxSpeed();
        if (fMax <= 1.8f)
            fAccel = fAccel * (fMax / 1.8f);
        fInvMax = fAccel / fMax;

// The two accumulators, Simpson-weighted. k4 uses this frame's axis, not the midpoint.
        Vector k4;
        k4.x = (vAxis.x * fAccel - vEnd.x * fInvMax) * fStep;
        k4.y = (vAxis.y * fAccel - vEnd.y * fInvMax) * fStep;
        k4.z = (vAxis.z * fAccel - vEnd.z * fInvMax) * fStep;

// With Simpson weights, the velocity arm carries 1/6, 1/3, 1/3, 1/6, and the position arm scales
// every term by the step as well.
        const float fSixth = 1.0f / 6.0f;
        const float fThird = 1.0f / 3.0f;
        DriveA() = DriveVel() + k1 * fSixth + k2 * fThird + k3 * fThird + k4 * fSixth;
        DriveB() = DrivePos() + DriveVel() * fStep * fSixth + m1 * fStep * fThird
                 + m2 * fStep * fThird + vEnd * fStep * fSixth;

// This frame's axis becomes next frame's drive target.
        DriveTarget() = vAxis;

        // The frame's travel, and the threshold that decides "arrived" from "still moving".
        Vector vMove = DriveB() - DrivePos();
// Travel below the threshold counts as arrived, and so does a NaN travel.
        float fThreshold = GetMaxSpeed() < 5000.0f ? 0.0f : 0.001089f;
        if (!(vMove.x * vMove.x + vMove.y * vMove.y + vMove.z * vMove.z > fThreshold))
        {
            // Once arrived, only the combat camera mode turns to face the current target.
            if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nCameraMode == CClientOptions::CAMERA_MODE_COMBAT)
            {
                CSWSCreature* pTarget = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
                    g_pAppManager->m_pServerExoApp->ClientToServerObjectId(
                        g_pAppManager->m_pClientExoApp->GetLastTarget()));

                if (pTarget != 0 &&
                    pServer->GetReputation(pTarget->m_objectId, 0) <= 10 &&
                    pCreature->IsDriveAnimationBlocked() == 0)
                {
                    Vector vToTarget = pTarget->m_position - pServer->m_position;
                    float fYawDelta = Yaw(vToTarget) - Yaw(pServer->m_orientation);
                    if (fYawDelta > 180.0f)
                        fYawDelta -= 360.0f;
                    if (fYawDelta < -180.0f)
                        fYawDelta += 360.0f;

                    float fMaxTurn = fStep * fCombatTurnSpeed;
                    if (fYawDelta < 0.0f)
                    {
                        if (fYawDelta < -fMaxTurn)
                            fYawDelta = -fMaxTurn;
                    }
                    else if (fYawDelta > fMaxTurn)
                        fYawDelta = fMaxTurn;

                    Quaternion qFace = YawPitchRoll(fYawDelta, 0.0f, 0.0f);
                    Vector vFace = RotateByQuaternion(pServer->m_orientation, qFace);
                    pServer->SetOrientation(vFace);
                    pCreature->SetOrientation(vFace);
                    fResult = 0.0f;
                }
            }
        }
        else
        {
            // While moving, flatten the facing, step along it, and commit.
            Vector vFlat = normalize(
                Vector(pCreature->m_orientation.x, pCreature->m_orientation.y, 0.0f));
            pCreature->SetOrientation(vFlat);

            float fDistance = magnitude(vMove);
            Vector vNext = pCreature->m_position + pCreature->m_orientation * fDistance;
// The magnitude is computed and discarded; the call is kept.
            magnitude(pCreature->m_position - vNext);

            float fSpeed = magnitude(DriveA());
#if KOTOR1
            // K1 tests finiteness in single precision, and only after the 1.0 test.
            if (fSpeed > 1.0f && __builtin_isfinite(fSpeed))
#else
            int bFinite = _finite(static_cast<double>(fSpeed));
            if (fSpeed > 1.0f && bFinite != 0)
#endif
            {
                if (pCreature->SetPrecalculatedPosition(vNext) == 0)
                {
                    TurnVector() = Vector(0.0f, 0.0f, 0.0f);
                }
                pCreature->ResetCameraToFollow();
                fResult = fSpeed;
            }
        }

        DriveVel() = DriveA();
        DrivePos() = DriveB();
    }

#ifdef K2_HOST_BUILD
    if (!bAxisZero) {
        static char s_szLast[160] = "";
        char szWhy[160];
        snprintf(szWhy, sizeof szWhy, "enabled %d, max speed %.2f, accel %.2f, %s", m_bEnabled, GetMaxSpeed(),
                 GetAcceleration(), fResult > 0.0f ? "moving" : "not moving");
        if (strcmp(szWhy, s_szLast) != 0) {
            fprintf(stderr, "k2: move step: %s\n", szWhy);
            strcpy(s_szLast, szWhy);
        }
    }
#endif
    // The animation and lean tail.
    pCreature->SetSWAnimationSpeed(fResult, 0.0f);
    pCreature->GetAnimationBase()->SetLean(0.0f);
    return 1;
}

void CSWPlayerControlCamRelative::UpDown(float f)
{
    m_upDown = f;
}

float CSWPlayerControlCamRelative::GetUpDown()
{
    return m_upDown;
}

void CSWPlayerControlCamRelative::LeftRight(float f)
{
    m_leftRight = f;
}

float CSWPlayerControlCamRelative::GetLeftRight()
{
    return m_leftRight;
}

void CSWPlayerControlCamRelative::SetPlayerWalking(int b)
{
    m_bPlayerWalking = b;
}

int CSWPlayerControlCamRelative::GetPlayerWalking()
{
    return m_bPlayerWalking;
}

void CSWPlayerControlCamRelative::ResetDriveAcceleration()
{
    m_vTurn = Vector();
    m_vDriveA = Vector();
    m_vDriveB = Vector();
    m_vDriveVel = Vector();
    m_vDrivePos = Vector();
    m_vDriveTarget = Vector();
}

void CSWPlayerControlCamRelative::SetTurnVelocity(float f)
{
    m_acceleration.SetVelocity(f);
}

// Answers the drive max-speed ceiling, the debug turbo speed when either turbo switch is on,
// otherwise the appearance's maximum drive speed, or its walk-speed base when driving is disabled;
// 6.0f stands in when there is no creature.
#if KOTOR1
// K1 scales the chosen speed by its own drive-speed constant.
float g_fDriveSpeedScale __attribute__((visibility("hidden"))) = 1.3f;
#endif
float CSWPlayerControlCamRelative::GetMaxSpeed()
{
    if (turbo != 0 || g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bTurboSpeed != 0)
        return turbospeed;

    if (g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId) == 0)
        return 6.0f;

    g_pAppManager->m_pClientExoApp->GetSWParty();

#if KOTOR1
    unsigned short nFlags = g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId)->m_nActivityFlags;
#ifdef K2_QOL_CONTROLLER
    float fScale = k2qol_faster_movement() ? g_fDriveSpeedScale : 1.0f;
#else
    float fScale = g_fDriveSpeedScale;
#endif
    CSWCCreatureAppearance* pAppearance =
        g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId)->m_pCreatureAppearance;
    return fScale * ((nFlags & 1) ? pAppearance->m_walkSpeed : pAppearance->m_fDriveMaxSpeed);
#elif defined(K2_QOL_CONTROLLER)
    // The Faster Movement option, KOTOR 1's mobile drive-speed scale.
    const float fScale = k2qol_faster_movement() ? 1.3f : 1.0f;
    if ((g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId)->m_nActivityFlags & 1) != 0 &&
        g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId)->m_pStats->HasFeat(FEAT_STEALTH_RUN) == 0)
    {
        return fScale * g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId)->m_pCreatureAppearance->m_walkSpeed;
    }
    return fScale * g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId)->m_pCreatureAppearance->m_fDriveMaxSpeed;
#else
    if ((g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId)->m_nActivityFlags & 1) != 0 &&
        g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId)->m_pStats->HasFeat(FEAT_STEALTH_RUN) == 0)
    {
        return g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId)->m_pCreatureAppearance->m_walkSpeed;
    }
    return g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId)->m_pCreatureAppearance->m_fDriveMaxSpeed;
#endif
}

// Nothing stores the value: the body is a null test and a second lookup.
void CSWPlayerControlCamRelative::SetMaxSpeed(float fMaxSpeed)
{
    K2_UNUSED(fMaxSpeed);
    if (g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId) == 0)
        return;
    g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId);
}

// 25.0f, the acceleration default, when there is no creature; otherwise the creature's own value.
float CSWPlayerControlCamRelative::GetAcceleration()
{
    if (g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId) == 0)
        return 25.0f;
    return g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId)->GetDriveAcceleration();
}

// Writes the creature appearance's drive acceleration directly; there is no setter for it.
void CSWPlayerControlCamRelative::SetAcceleration(float fAcceleration)
{
    if (g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId) == 0)
        return;
    g_pAppManager->m_pClientExoApp->GetCreatureByGameObjectID(m_objectId)->m_pCreatureAppearance->m_driveAccel = fAcceleration;
}

// The larger of the two velocity components.
float CSWPlayerControlCamRelative::GetCurSpeed()
{
    float fX = __builtin_fabsf(m_vDriveVel.x);
    float fY = __builtin_fabsf(m_vDriveVel.y);
    return fX > fY ? fX : fY;
}
