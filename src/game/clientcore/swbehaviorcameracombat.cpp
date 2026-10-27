#include "appmanager.h"
#include "clientexoapp.h"
#include "clientoptions.h"
#include "include/aurbehavior.h"
#include "AURORA/aurmodel.h"
#include "aurcamera.h"
#include "aurscene.h"
#include "Matrix.h"
#include "Quaternion.h"
#include "Vector.h"
#include "swguicontrol.h"
#include "serverexoappinternal.h"
#include "nwccreatureappearance.h"
#include "nwsrules.h"
#include "twodimarrays.h"
#include "twodimarray.h"
#include "ClientExoAppInternal.h"

// The combat camera behavior, the object CSWCModule::SetCombatCamera builds.

extern Vector zaxis;
extern float CameraPersonalSpace;
extern int RenderQA;
extern int g_bMouseCameraRotateToggle;
extern int g_bMouseCameraRotateToggleIos;

#include "port/libcmath.h"

CSWBehaviorCameraCombat::CSWBehaviorCameraCombat(CAurObject* pObject)
    : CAurBehavior(pObject)
    , m_nReserved1(0)
    , m_nReserved2(0)
{
    m_fShakeYaw = 0.0f;
    m_fShakePitch = 0.0f;
    m_fMouseTurn = 0.0f;
    m_fTurn = 0.0f;

    // The camera's tunables come from row 8 of the CameraStyle table.
    C2DA* pCameraStyle = g_pRules->m_p2DArrays->m_pCameraStyle;
    pCameraStyle->GetFLOATEntry(8, CExoString("DISTANCE"), &m_fDistance);
    pCameraStyle->GetFLOATEntry(8, CExoString("HEIGHT"), &m_fHeight);
    pCameraStyle->GetFLOATEntry(8, CExoString("PITCH"), &m_fPitch);
    pCameraStyle->GetFLOATEntry(8, CExoString("ROTATION"), &m_fRotation);
    pCameraStyle->GetFLOATEntry(8, CExoString("SPEED"), &m_fSpeed);
    // The view angle has no member of its own; it seeds the live camera's field of view.
    float fViewAngle;
    pCameraStyle->GetFLOATEntry(8, CExoString("VIEWANGLE"), &fViewAngle);

    static_cast<CAurCamera*>(m_pOwner->AsCAurCamera())->SetViewAngle(fViewAngle);

    m_acceleration.m_fMaxSpeed =
        g_pAppManager->m_pClientExoApp->GetClientOptions()->GetCameraKeyboardDPS();
    m_acceleration.m_fAcceleration =
        g_pAppManager->m_pClientExoApp->GetClientOptions()->GetCameraKeyboardAcceleration();
    m_acceleration.m_fDeceleration =
        g_pAppManager->m_pClientExoApp->GetClientOptions()->GetCameraKeyboardDeceleration();
    m_bMouseRotating = g_bMouseCameraRotateToggle;
}

// Computes the camera's desired world position and returns the composed yaw. The target parameter
// is unused.
Vector CSWBehaviorCameraCombat::CalculateDesiredLocation(CSWCCreature* pCreature,
                                                         CSWCObject* pTarget,
                                                         float fParam, float& fYaw)
{
    K2_UNUSED(pTarget);
    CAurCamera* pCamera = g_pAppManager->m_pClientExoApp->GetModuleCamera();

    Vector vDir;

    float fTurn = m_fMouseTurn;
    Vector vCreaturePos(pCreature->m_position);
    if (m_fMouseTurn == 0.0f) {
        // The mouse is not driving, so the yaw rate is integrated through the acceleration state for
        // this frame.
        Vector vVelocity(0.0f, m_fTurn, 0.0f);
        CSWPhysicsState state = m_acceleration.Update(vVelocity, fParam, 1, 0, 0, 0);
        fTurn = state.m_vDisplacement.y;
    } else {
        m_acceleration.SetVelocity(m_fMouseTurn);
        m_fTurn = 0.0f;
    }

    Quaternion q = YawPitchRoll(fTurn, 0.0f, 0.0f) * pCamera->GetOrientation();
    fYaw = Yaw(q);

    // The stand-off slides along the orientation's local +Y projected to XY.
    vDir = q.ydir();
    vDir.z = 0.0f;
    vDir.Normalize();

    // The horizontal lanes are pulled back along that axis by the style's distance, while z is the
    // creature's head height plus the style's height, where the axis plays no part.
    Vector vRet = vCreaturePos + vDir * -m_fDistance;
    vRet.z = pCreature->m_position.z + pCreature->m_pCreatureAppearance->GetHeadHeightOffset() + m_fHeight;
    return vRet;
}

// Refreshes the option-driven accelerator gains, refreshes the rotate latch and mouse cursor, then
// runs the desired location through DetermineSafePosition and aims the camera at it.
int CSWBehaviorCameraCombat::Control(float fParam)
{
    // The early exit leaves the return value at the base's zero; the on-a-stick sibling returns 1
    // here instead.
    if (CAurBehavior::Control(fParam) == 0)
        return 0;

    m_acceleration.m_fMaxSpeed =
        g_pAppManager->m_pClientExoApp->GetClientOptions()->GetCameraKeyboardDPS();
    m_acceleration.m_fAcceleration =
        g_pAppManager->m_pClientExoApp->GetClientOptions()->GetCameraKeyboardAcceleration();
    m_acceleration.m_fDeceleration =
        g_pAppManager->m_pClientExoApp->GetClientOptions()->GetCameraKeyboardDeceleration();

    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    CSWCObject* pTarget =
        static_cast<CSWCObject*>(g_pAppManager->m_pClientExoApp->GetGameObject(
            g_pAppManager->m_pClientExoApp->GetLastTarget()));

    // For the QA overlay distance probe, the positions are fetched and dropped.
    if (RenderQA != 0 && m_pOwner != 0 && pCreature != 0) {
        (void)m_pOwner->GetPosition();
        (void)m_pOwner->GetPosition();
        (void)m_pOwner->GetPosition();
        magnitude(m_pOwner->GetPosition() - pCreature->m_position);
    }

    // The gameplay option bit inverts the latch.
    int nToggle = g_bMouseCameraRotateToggle;
    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGameplay1)
        nToggle = (nToggle == 0);
    if (nToggle != 0) {
        if (m_bMouseRotating == 0) {
            g_pGuiMan->StoreCurrentMousePosition();
            m_bMouseRotating = nToggle;
            g_pAppManager->m_pClientExoApp->HideMouse(CClientExoAppInternal::MOUSEHIDE_CAMERA_ROTATE);
        }
    } else if (m_bMouseRotating != 0) {
        g_pGuiMan->RestoreOldMousePosition();
        m_bMouseRotating = 0;
        g_pAppManager->m_pClientExoApp->ShowMouse(CClientExoAppInternal::MOUSEHIDE_CAMERA_ROTATE);
    }

    // A cutscene or takeover input class always releases the cursor and clears the latch without
    // touching the toggle.
    if (g_pAppManager->m_pClientExoApp->GetInputClass() == 2) {
        m_bMouseRotating = 0;
        g_pAppManager->m_pClientExoApp->ShowMouse(CClientExoAppInternal::MOUSEHIDE_CAMERA_ROTATE);
    }

    // Falls back to the player creature when there is no separate target, then requires the target's
    // client object to be real.
    if (pTarget == 0)
        pTarget = pCreature;
    if (pCreature == 0)
        return 1;
    if (pTarget == 0)
        return 1;
    if (pTarget->AsSWCObject() == 0)
        return 1;

    // The creature's height is folded into its position here, and the same vector goes to
    // DetermineSafePosition, which does not add it again.
    Vector vPos(pCreature->m_position);
    vPos.z += pCreature->m_pCreatureAppearance->GetHeight();

    float fYaw;
    Vector vDesired =
        CalculateDesiredLocation(pCreature, pTarget->AsSWCObject(), fParam, fYaw);
    Vector vSafe = DetermineSafePosition(pCreature, vDesired, vPos);

    // The yaw helper is the one used here; the position query below is a leftover call whose result
    // is discarded.
    float fUnused;
    float fNextYaw = GetNextYaw(vDesired, fYaw, fParam, fUnused);

    // The shake pair offsets the composed yaw and pitch; the position is the safe one, unmodified.
    (void)m_pOwner->GetPosition();
    Quaternion qNext = YawPitchRoll(fNextYaw + m_fShakeYaw, m_fPitch + m_fShakePitch, 0.0f);
    m_pOwner->SetOrientation(qNext);
    m_pOwner->SetPosition(vSafe);
    return 1;
}

// Returns its first float unchanged. The mouse-sensitivity calls and the vector argument are kept
// only because nothing here consumes their results.
float CSWBehaviorCameraCombat::GetNextYaw(const Vector& v, float fA, float fB, float& fOut)
{
    K2_UNUSED(v); K2_UNUSED(fB);
    float fYaw = Yaw(m_pOwner->GetOrientation());
    if (fYaw != fA)
        fYaw = g_pAppManager->m_pClientExoApp->GetClientOptions()->GetMouseSenSetting();
    fYaw = g_pAppManager->m_pClientExoApp->GetClientOptions()->GetMouseSenSetting();
    fOut = 0.0f;
    return fA;
}

// Advances v toward the owner's current position by fB units, but only when the frame step is above
// 0.001 and the squared gap above 0.01.
Vector CSWBehaviorCameraCombat::GetNextPosition(const Vector& v, float fA, float fB)
{
    Vector vPos = m_pOwner->GetPosition();
    Vector vDiff(v.x - vPos.x, v.y - vPos.y, v.z - vPos.z);
    if (fA > 0.001f) {
        float fMagSq = vDiff.x * vDiff.x + vDiff.y * vDiff.y + vDiff.z * vDiff.z;
        if (fMagSq > 0.01f) {
            float fStep = sqrtf(fMagSq) / fA * fB;
            Vector vStep = normalize(vDiff) * fStep;
            Vector vOwner = m_pOwner->GetPosition();
            return Vector(vOwner.x + vStep.x, vOwner.y + vStep.y, vOwner.z + vStep.z);
        }
    }
    // The skip path returns v itself, not the difference vector.
    return v;
}

// Aims at whatever the last target resolved to and parks the owner behind it, then runs the parked
// point through DetermineSafePosition.
void CSWBehaviorCameraCombat::PutAtDesiredLocation()
{
    CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetSWParty()->GetPlayerCharacter();
    CSWCObject* pTarget =
        static_cast<CSWCObject*>(g_pAppManager->m_pClientExoApp->GetGameObject(
            g_pAppManager->m_pClientExoApp->GetLastTarget()));
    g_pAppManager->m_pClientExoApp->GetModuleCamera();

    if (pCreature == 0)
        return;
    if (pTarget == 0)
        return;
    if (pTarget->AsSWCObject() == 0)
        return;

    Vector vPos(pCreature->m_position);
    // The heading is the direction to the target.
    Vector vTargetPos = pTarget->AsSWCObject()->m_position;
    Vector vDiff(vTargetPos.x - vPos.x, vTargetPos.y - vPos.y, vTargetPos.z - vPos.z);
    float fYaw = Yaw(vDiff);
    vDiff.Normalize();

    Vector vSafe = DetermineSafePosition(
        pCreature,
        Vector(vPos.x + vDiff.x * -m_fDistance, vPos.y + vDiff.y * -m_fDistance,
               vPos.z + vDiff.z * -m_fDistance),
        vPos);

    Quaternion qNew = YawPitchRoll(fYaw, m_fPitch, 0.0f);
    m_pOwner->SetOrientation(qNew);
    m_pOwner->SetPosition(vSafe);
}

// Probes four points around the desired location with the scene's ray check, along the side axis
// and straight up and down, each offset by the personal-space scale, and keeps the largest miss
// distance as the clearance in hand. It then shortens the desired location toward the creature by
// that clearance and slides it out along the creature's own local +Z by a fraction of the same.
// Returns the offered location unchanged when none of the four probes hits anything.
//
// EFFECTIVE: length-exact, 86 bytes of diff, with identical unwind ranges, pad for pad. The
// residual is operand commutation in the crossed product and matrix row, and the scratch slot
// picked for the normalized direction in the tail; all bit-exact.
Vector CSWBehaviorCameraCombat::DetermineSafePosition(CSWCCreature* pCreature,
                                                     const Vector& vDesired,
                                                     const Vector& vCreaturePos)
{
    Vector vOut;                        // the scene's hit point
    CHitInfo hit(0, ~CHitInfo::kMaterialTrigger, 0);
    hit.m_bSkipCharactersAndItems = 1;
    Vector vPos = vDesired;
    Vector vToDesired, vSide, vSideOff, vUpOff, vProbe1, vProbe2, vProbe3, vProbe4;
    float fBest;
    bool bFound;
    hit.ClearOutput();

    // Side axis, world up crossed with the creature-to-desired direction.
    vToDesired = Vector(vCreaturePos.x - vPos.x, vCreaturePos.y - vPos.y, vCreaturePos.z - vPos.z);
    vSide = normalize(cross(zaxis, vToDesired));

    fBest = 0.0f;
    bFound = false;

    // Probe 1 pushes the desired point along the side axis. magnitude() is out of line, so the
    // accumulate is a ternary over two calls rather than a maximum, and the second call runs when the
    // first distance does not win. Each pair of probes shares one offset.
    vSideOff = vSide * CameraPersonalSpace;
    vProbe1 = vPos + vSideOff;
    hit.ClearOutput();
    hit.m_bHonorIgnoreHitCheck = 1;
    if (m_pOwner->GetScene()->HitCheck(vCreaturePos, vProbe1, &vOut, &hit)) {
#if KOTOR1
        // K1 evaluates the distance once per probe.
        {
            float fDist1 = magnitude(vOut - vProbe1);
            fBest = fBest > fDist1 ? fBest : fDist1;
        }
#else
        fBest = fBest > magnitude(vOut - vProbe1) ? fBest : magnitude(vOut - vProbe1);
#endif
        bFound = true;
    }

    // Probe 2 takes the opposite side.
    vProbe2 = vPos - vSideOff;
    hit.ClearOutput();
    hit.m_bHonorIgnoreHitCheck = 1;
    if (m_pOwner->GetScene()->HitCheck(vCreaturePos, vProbe2, &vOut, &hit)) {
#if KOTOR1
        {
            float fDist2 = magnitude(vOut - vProbe2);
            fBest = fBest > fDist2 ? fBest : fDist2;
        }
#else
        fBest = fBest > magnitude(vOut - vProbe2) ? fBest : magnitude(vOut - vProbe2);
#endif
        bFound = true;
    }

    // Probe 3 is straight above the desired point.
    vUpOff = Vector(0.0f, 0.0f, 1.0f) * CameraPersonalSpace;
    vProbe3 = vPos + vUpOff;
    hit.ClearOutput();
    hit.m_bHonorIgnoreHitCheck = 1;
    if (m_pOwner->GetScene()->HitCheck(vCreaturePos, vProbe3, &vOut, &hit)) {
#if KOTOR1
        {
            float fDist3 = magnitude(vOut - vProbe3);
            fBest = fBest > fDist3 ? fBest : fDist3;
        }
#else
        fBest = fBest > magnitude(vOut - vProbe3) ? fBest : magnitude(vOut - vProbe3);
#endif
        bFound = true;
    }

    // Probe 4 is straight below.
    vProbe4 = vPos - vUpOff;
    hit.ClearOutput();
    hit.m_bHonorIgnoreHitCheck = 1;
    if (m_pOwner->GetScene()->HitCheck(vCreaturePos, vProbe4, &vOut, &hit)) {
#if KOTOR1
        {
            float fDist4 = magnitude(vOut - vProbe4);
            fBest = fBest > fDist4 ? fBest : fDist4;
        }
#else
        fBest = fBest > magnitude(vOut - vProbe4) ? fBest : magnitude(vOut - vProbe4);
#endif
        bFound = true;
    }

    // Nothing hit anywhere, so the offered location comes back untouched.
    if (!bFound)
        return vPos;

    // Shorten toward the creature, then lift along its own local +Z.
    Vector vAway = vDesired - vCreaturePos;
    float fDist = magnitude(vAway);
    Vector vNorm = normalize(vAway);

    // The creature model's local +Z.
    Vector vAxis = normalize(pCreature->GetModel(CSWCObject::ANIM_MODEL_BODY, 1)->GetOrientation().zdir());

    // The lift is proportional to how much of the stand-off distance is left over.
    vPos = vPos - vNorm * fBest + vAxis * ((fBest / fDist) * (CameraPersonalSpace + 0.15f));
    return vPos;
}

int CSWBehaviorCameraCombat::GetType()
{
    return BEHAVIOR_CAMERA_COMBAT;
}

// Latches a turn amount unless the iOS mouse-camera-rotate mode vetoes it; the gameplay option bit
// inverts the toggle.
void CSWBehaviorCameraCombat::SetTurn(float f)
{
#ifndef K2_UI_TOUCH
    // The PC camera-rotate latch, driven by the right mouse button.
    int nToggle = g_bMouseCameraRotateToggle;
#else
    int nToggle = g_bMouseCameraRotateToggleIos;
#endif
    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGameplay1)
        nToggle = (nToggle == 0);
    m_fMouseTurn = nToggle != 0 ? f : 0.0f;
}

