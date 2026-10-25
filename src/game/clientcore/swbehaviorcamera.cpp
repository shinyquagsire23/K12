#define K1_AURORA_LIST  // K1's AURORA List layout
#include "appmanager.h"
#include "clientexoapp.h"
#include "clientoptions.h"
#include "include/aurbehavior.h"
#include "AURORA/aurmodel.h"
#include "aurcamera.h"
#include "Matrix.h"
#include "Quaternion.h"
#include "Vector.h"
#include "exostring.h"
#include "twodimarray.h"
#include "twodimarrays.h"
#include "nwsrules.h"
#include "nwcmodule.h"
#include "nwcarea.h"
#include "nwcobject.h"
#include "swgui.h"
#include "guiingame.h"
#include "SWAccelerationControlers.h"
#include "serverexoappinternal.h"
#include "ClientExoAppInternal.h"

extern template void SafePointerList<CAurObject>::AddPointer(SafePointer<CAurObject>*);

char* firstword(char* pszLine);
void  Parse(char* pszLine, float& fValue);

#include "port/crtapi.h"

bool IsJoystickConnected();

#include <string.h>
#include "nwccreature.h"
#include "aurscene.h"
#include "swbehaviorcamera.h"
#include <math.h>

extern int g_bMouseCameraRotateToggle;

extern int RenderQA;

#include "port/libcmath.h"

float CameraLookUp   = 30.0f;
float CameraLookDown = 30.0f;
float CameraRotate   = 90.0f;
float CameraInterpAmt1 = 2.5f;

int CameraStyle = -1;
int CameraFreeStyle;
int CameraFreeStyleMode;

extern CSWGuiManager* g_pGuiMan;
extern int   g_bMouseCameraRotateToggleIos;
int bCameraSlideBack;
extern int   CameraHorzAxisFlip;
float CameraInterpAmt2 = 7.5f;
float DesiredOrientation = -1.0f;

extern int quaternionidentityoptimization;

extern Vector zaxis;

float CameraInterpDistAmt = 0.5f;

// Rotates v by q, skipping the matrix multiply for an identity rotation.
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

CSWCameraOnAStick::CSWCameraOnAStick(CAurObject* pCameraObject, CAurObject* pFollowPart)
    : CAurBehavior(pCameraObject)
{
    m_pCameraObject = pCameraObject;
    AddDependency(pFollowPart);
    m_pTargetObject = pFollowPart;
    m_pCombatCamera = 0;
    m_nStyle = -1;

    SetCameraStyle(g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_cameraStyle);

    m_nReserved5 = 0;
    m_vPlayerFacing.x = 0.0f;
    m_vPlayerFacing.y = 0.0f;
    m_vPlayerFacing.z = 0.0f;
#if KOTOR1
    // K1 keeps the style's tilt speed and has no pitch integrator.
    m_fTurn = 0.0f;
    m_fTiltAccel = 0.0f;
    m_fTurnAccel = 0.0f;
    m_fReserved4 = 100.0f;
    m_fTilt = m_fPresetPitch;
#else
    m_fTurn = 0.0f;
    m_fTiltAccel = 0.0f;
    m_fTurnAccel = 0.0f;
    m_fReserved4 = 100.0f;
    m_fTilt = m_fPresetPitch;
    m_fTiltAccelInput = 0.0f;
#endif

    // The start orientation is the target's local +Z in the XZ plane, the style's pitch, and no
    // roll.
    Quaternion qTarget = m_pTargetObject.m_pObject->GetOrientation();
    Vector vDir = normalize(qTarget.zdir());
    m_qDesiredOrientation = YawPitchRoll(Yaw(vDir), m_fPresetPitch, 0.0f);
    m_pOwner->SetOrientation(m_qDesiredOrientation);

    Vector vTarget = m_pTargetObject.m_pObject->GetPosition();
    vTarget.z += 1.5f;
    Vector vNew = vTarget - qTarget.ydir();
    m_pOwner->SetPosition(vNew);

    m_vLastCameraPosition = vNew;
    m_vLastTargetPosition = vNew;
    m_vLastPosition = vNew;

    m_bIsDialog = 1;
    m_bKeepCurrentYaw = 0;
    m_vLastTargetPosition.x = 3.4028235e+38f;  // FLT_MAX: the behavior has never been placed
    m_bPreserveFraming = 0;
    m_fShakeYaw = 0.0f;
    m_fShakePitch = 0.0f;
    m_lookAtTargetId = OBJECT_INVALID;
    m_nReserved2 = 0;
    m_fReserved1 = -1.0f;
    m_bLookAtActive = 0;
#if !KOTOR1  // K1 leaves m_fInterpAmount uninitialised
    m_fInterpAmount = 0.0f;
    m_fTiltSpeed = 0.0f;
    m_fSavedTiltSpeed = 0.0f;
#endif
    m_bMouseRotating = g_bMouseCameraRotateToggle;
}

CSWCameraOnAStick::CSWCameraOnAStick(CCameraSettings& settings)
    : CAurBehavior(settings.m_pCameraObject)
{
    m_pTargetObject = settings.m_pTarget;
    m_pCameraObject = settings.m_pCameraObject;
    m_nStyle = -1;
    m_nFollowingBody = settings.m_nFollowingBody;
    m_fCameraOffset = settings.m_fTargetHeight;

    SetCameraStyle(settings.m_nStyle);

#if !KOTOR1
    m_acceleration.m_fMaxSpeed = 200.0f;
    m_acceleration.m_fAcceleration = 500.0f;
    m_fTiltAccelInput = 0.0f;
#endif
    AddDependency(m_pTargetObject);

    m_nReserved5 = 0;
    m_vPlayerFacing.x = 0.0f;
    m_vPlayerFacing.y = 0.0f;
    m_vPlayerFacing.z = 0.0f;
    m_fReserved4 = 100.0f;
    m_fTurn = 0.0f;
    m_fTiltAccel = 0.0f;
    m_fTurnAccel = 0.0f;
#if KOTOR1
    m_fTilt = m_fPresetPitch;  // K1 saves no pitch in CCameraSettings
#else
    m_fTilt = settings.m_fTilt;
#endif

    Quaternion qTarget = m_pTargetObject.m_pObject->GetOrientation();
    Vector vDir = normalize(qTarget.zdir());
    m_qDesiredOrientation = YawPitchRoll(Yaw(vDir), m_fPresetPitch, 0.0f);
    // The camera starts at the target's own orientation, not at m_qDesiredOrientation.
    m_pOwner->SetOrientation(qTarget);

    Vector vTarget = m_pTargetObject.m_pObject->GetPosition();
    vTarget.z += 1.5f;
    Vector vNew = vTarget - qTarget.ydir();

    m_lookAtTargetId = OBJECT_INVALID;
    m_fShakeYaw = 0.0f;
    m_fShakePitch = 0.0f;
    m_nReserved2 = 0;
    m_bLookAtActive = 0;
    m_fReserved1 = -1.0f;
#if !KOTOR1  // K1 leaves m_fInterpAmount uninitialised
    m_fInterpAmount = 0.0f;
#endif
    m_bIsDialog = 1;
    m_bKeepCurrentYaw = 1;

    m_vLastPosition = vNew;
    m_vLastTargetPosition = vNew;
    m_vLastCameraPosition = vNew;

    m_bMouseRotating = g_bMouseCameraRotateToggle;
}

CSWCameraOnAStick::~CSWCameraOnAStick()
{
}

void CSWCameraOnAStick::UpdateCameraStyle()
{
    SetCameraStyle(g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_cameraStyle);
}

void CSWCameraOnAStick::SetCameraStyle(int nStyle)
{
    if (m_nStyle == nStyle)
        return;

    C2DA* pCameraStyle = g_pRules->m_p2DArrays->m_pCameraStyle;
    if (pCameraStyle == 0) {
        m_nStyle = -1;
        m_fDist = 6.25f;
        m_fSpeed = 20.0f;
        m_fPresetPitch = 80.0f;
        m_fHeight = 3.5f;
        return;
    }
    if (pCameraStyle->m_nRows < nStyle)
        return;

    m_nStyle = nStyle;
    pCameraStyle->GetFLOATEntry(m_nStyle, CExoString("DISTANCE"), &m_fDist);
    pCameraStyle->GetFLOATEntry(m_nStyle, CExoString("SPEED"), &m_fSpeed);
    pCameraStyle->GetFLOATEntry(m_nStyle, CExoString("PITCH"), &m_fPresetPitch);
#if KOTOR1
    pCameraStyle->GetFLOATEntry(m_nStyle, CExoString("HEIGHT"), &m_fHeight);
    pCameraStyle->GetFLOATEntry(m_nStyle, CExoString("TILTUP"), &CameraLookUp);
    pCameraStyle->GetFLOATEntry(m_nStyle, CExoString("TILTDOWN"), &CameraLookDown);
#else
    pCameraStyle->GetFLOATEntry(m_nStyle, CExoString("TILTUP"), &CameraLookUp);
    pCameraStyle->GetFLOATEntry(m_nStyle, CExoString("TILTDOWN"), &CameraLookDown);
    pCameraStyle->GetFLOATEntry(m_nStyle, CExoString("HEIGHT"), &m_fHeight);
#endif
    pCameraStyle->GetFLOATEntry(m_nStyle, CExoString("TILTSPEED"), &m_fTiltSpeed);
    pCameraStyle->GetFLOATEntry(m_nStyle, CExoString("ROTATION"), &CameraRotate);

    float fViewAngle;
    pCameraStyle->GetFLOATEntry(m_nStyle, CExoString("VIEWANGLE"), &fViewAngle);

    m_splitAcceleration.m_fMaxSpeed =
        g_pAppManager->m_pClientExoApp->GetClientOptions()->GetCameraKeyboardDPS();
    m_splitAcceleration.m_fAcceleration =
        g_pAppManager->m_pClientExoApp->GetClientOptions()->GetCameraKeyboardAcceleration();
    m_splitAcceleration.m_fDeceleration =
        g_pAppManager->m_pClientExoApp->GetClientOptions()->GetCameraKeyboardDeceleration();

    if (fViewAngle == 0.0f)
        fViewAngle = 55.0f;

    static_cast<CAurCamera*>(m_pCameraObject->AsCAurCamera())->SetViewAngle(fViewAngle);
    m_fStyleDistance = m_fDist;
    m_fStyleViewAngle = fViewAngle;
}

void CSWCameraOnAStick::Message(char* pszParam)
{
    if (_stricmp("m_fDist", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_fDist"), m_fDist);
    if (_stricmp("m_fSpeed", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_fSpeed"), m_fSpeed);
    if (_stricmp("m_fPresetPitch", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_fPresetPitch"), m_fPresetPitch);
    if (_stricmp("m_fHeight", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_fHeight"), m_fHeight);
}

void CSWCameraOnAStick::SetCameraForScreenShot()
{
    Vector vTarget = m_pTargetObject.m_pObject->GetPosition();
    Vector vOwner = m_pOwner->GetPosition();
    Vector vDir(vTarget.x - vOwner.x, vTarget.y - vOwner.y, vTarget.z - vOwner.z);

    Pitch(vDir);
    vDir.z = 0.0f;
    if (vDir.x * vDir.x + vDir.y * vDir.y + vDir.z * vDir.z > 0.0f) {
        m_pOwner->SetOrientation(YawPitchRoll(Yaw(normalize(vDir)), m_fPresetPitch, 0.0f));
    }
}

void CSWCameraOnAStick::Control_CheckAndUpdateCameraStyle()
{
    m_splitAcceleration.m_fMaxSpeed =
        g_pAppManager->m_pClientExoApp->GetClientOptions()->GetCameraKeyboardDPS();
    m_splitAcceleration.m_fAcceleration =
        g_pAppManager->m_pClientExoApp->GetClientOptions()->GetCameraKeyboardAcceleration();
    m_splitAcceleration.m_fDeceleration =
        g_pAppManager->m_pClientExoApp->GetClientOptions()->GetCameraKeyboardDeceleration();

    if (CameraStyle != -1) {
        SetCameraStyle(CameraStyle);
        CameraStyle = -1;
    }
}

// Returns the desired position, the argument minus an m_fDist offset along the behavior's rotated
// +Y.
Vector CSWCameraOnAStick::Control_ComputeDesiredPosition(Vector vTarget)
{
    Vector vOffset = Vector(0.0f, 1.0f, 0.0f) * m_fDist;
    Vector vRot = RotateVector(vOffset, m_qDesiredOrientation);
    return Vector(vTarget.x - vRot.x, vTarget.y - vRot.y, vTarget.z - vRot.z);
}

// Springs the interpolation amount, accelerating while the two positions are far apart and
// decelerating while they are close, then clamps it into [fB, fC].
float CSWCameraOnAStick::Control_AccelerateInterp(float fA, float fB, float fC, Vector vA,
                                                 Vector vB, float fD)
{
    float fDistSq = (vB.x - vA.x) * (vB.x - vA.x) + (vB.y - vA.y) * (vB.y - vA.y)
                  + (vB.z - vA.z) * (vB.z - vA.z);
    if (fDistSq > 1.0f)
        fA += 3.0f * fD;
    if (fDistSq < 1.0f)
        fA -= 5.0f * fD;
    fA = (fA > fB) ? fA : fB;
    fA = (fA < fC) ? fA : fC;
    return fA;
}

// Builds the owner's orientation for the frame: the yaw facing the followed object from the XZ-plane
// direction, the accumulated pitch offset by 90 degrees, and both shake offsets. The tilt knobs are
// re-seeded from the 2DA globals every frame, and the pitch accumulator is driven either by the raw
// tilt input, which also flushes the spring, or by the spring's integrated second-vector y lane. A
// joystick switches the tilt rate from 100 to 50 and forces the mouse-rotate latch on.
#if KOTOR1
// K1 has no pitch spring: the tilt input drives the pitch directly, clamped to the style's pitch
// plus the look limits.
Quaternion CSWCameraOnAStick::Control_SetCameraOrientation(float fDeltaTime)
{
    Vector vTarget = m_pTargetObject.m_pObject->GetPosition();
    Vector vOwner = m_pOwner->GetPosition();
    Vector vDir(vTarget.x - vOwner.x, vTarget.y - vOwner.y, 0.0f);

    float fPitch = Pitch(m_pOwner->GetOrientation());

    float fTiltAccel = m_fTiltAccel;
    if (fabsf(fTiltAccel) > 1.0f) {
        fTiltAccel /= 75.0f;
        m_fTiltAccel = fTiltAccel;
    }
    if (fTiltAccel > 0.0f) {
        float fTilt = m_fTilt + fTiltAccel * m_fTiltSpeed * fDeltaTime;
        float fMax = m_fPresetPitch + CameraLookUp;
        m_fTilt = (fMax < fTilt) ? fMax : fTilt;
    }
    if (fTiltAccel < 0.0f) {
        float fTilt = m_fTilt + fTiltAccel * m_fTiltSpeed * fDeltaTime;
        float fMin = m_fPresetPitch - CameraLookDown;
        m_fTilt = (fMin > fTilt) ? fMin : fTilt;
    }
    float fEase = (m_fTilt - fPitch) * -0.5f * fDeltaTime;
    m_fTiltAccel = 0.0f;

    return YawPitchRoll(Yaw(normalize(vDir)) + m_fShakeYaw,
                        m_fTilt + fEase + m_fShakePitch, 0.0f);
}
#else
Quaternion CSWCameraOnAStick::Control_SetCameraOrientation(float fDeltaTime)
{
    Vector vTarget = m_pTargetObject.m_pObject->GetPosition();
    Vector vOwner = m_pOwner->GetPosition();
    Vector vDir(vTarget.x - vOwner.x, vTarget.y - vOwner.y, 0.0f);

    Pitch(m_pOwner->GetOrientation());

    m_fTiltUpLimit = CameraLookUp;
    m_fTiltDownLimit = CameraLookDown;
    m_fTiltRate = 100.0f;

    int bRotate = g_bMouseCameraRotateToggle;
    CClientOptions* pOptions = g_pAppManager->m_pClientExoApp->GetClientOptions();
    if (pOptions->m_bGameplay1)
        bRotate = (bRotate == 0);
    if (IsJoystickConnected()) {
        m_fTiltRate = 50.0f;
        bRotate = 1;
    }

    if (bRotate && m_fTiltAccel != 0.0f) {
        m_fTilt += m_fTiltRate * fDeltaTime * m_fTiltAccel;
        // Clear both integrator states.
        __builtin_memset(&m_acceleration.m_prev, 0, 2 * sizeof(CSWPhysicsState));
    } else {
        CSWPhysicsState state = m_acceleration.Update(Vector(0.0f, m_fTiltAccelInput, 0.0f),
                                                     fDeltaTime, 1, 0, 0, 0);
        m_fTilt += state.m_vDisplacement.y;
        m_fTiltAccelInput = 0.0f;
    }

    // Clamp the accumulated pitch into the look range.
    float fTilt = m_fTilt;
    if (fTilt < -m_fTiltUpLimit)
        fTilt = -m_fTiltUpLimit;
    if (fTilt > m_fTiltDownLimit)
        fTilt = m_fTiltDownLimit;
    m_fTilt = fTilt;

    return YawPitchRoll(Yaw(normalize(vDir)) + m_fShakeYaw,
                        m_fTilt + 90.0f + m_fShakePitch, 0.0f);
}
#endif

// Rebuilds the orientation and position from the target for the first frame and seeds the three
// history vectors.
void CSWCameraOnAStick::Control_SetCameraInintalState()
{
    Quaternion qTarget = m_pTargetObject.m_pObject->GetOrientation();

    Vector vDir = normalize(qTarget.zdir());
    m_qDesiredOrientation = YawPitchRoll(Yaw(vDir) + m_fShakeYaw, m_fShakeYaw + m_fPresetPitch, 0.0f);
    m_pOwner->SetOrientation(m_qDesiredOrientation);

    Vector vPos = m_pTargetObject.m_pObject->GetPosition();
    vPos.z += 1.5f;
    Vector vNew = vPos - qTarget.ydir();
    m_pOwner->SetPosition(vNew);
    m_vLastCameraPosition = vNew;
    m_vLastTargetPosition = vNew;
    m_vLastPosition = vNew;
}

// Snaps toward a look-at target, then applies the mouse-rotate latch and the turn integrator.
Quaternion CSWCameraOnAStick::Control_UpdateCameraDesiredOrientation(Quaternion qDesired, float fDeltaTime)
{
    m_fInterpClip = 0.0f;
    m_bKeepCurrentYaw = 1;
    (void)m_pTargetObject.m_pObject->GetPosition();

    float fMouseSen = g_pAppManager->m_pClientExoApp->GetClientOptions()->GetMouseSenSetting();

    // Any pending turn cancels the look-at target.
    if (m_fTurn == 0.0f && m_fTurnAccel == 0.0f) {
        if (m_lookAtTargetId != OBJECT_INVALID) {
            CSWCObject* pLookAt =
                static_cast<CSWCObject*>(g_pAppManager->m_pClientExoApp->GetGameObject(m_lookAtTargetId));
            if (pLookAt != 0) {
                Vector vTargetPos = m_pTargetObject.m_pObject->GetPosition();

                Vector vCross = cross(zaxis, qDesired.ydir());

                Vector vDir(pLookAt->m_position.x - vTargetPos.x,
                            pLookAt->m_position.y - vTargetPos.y,
                            pLookAt->m_position.z - vTargetPos.z);
                float fDot = vDir.x * vCross.x + vDir.y * vCross.y + vDir.z * vCross.z;

                // Aim 15 degrees off to whichever side the target is on.
                float fAngle = (fDot < 0.0f) ? -15.0f : 15.0f;
                // Drop the target once it leaves a ~30 degree cone.
                if (__builtin_fabsf(fDot) < 0.866f)
                    m_lookAtTargetId = OBJECT_INVALID;

                qDesired = YawPitchRoll(Yaw(vDir) - fAngle, 0.0f, 0.0f);
                m_bKeepCurrentYaw = 0;
                m_fInterpClip = CameraInterpAmt2;
                m_bLookAtActive = 1;
            }
        }
    } else {
        m_lookAtTargetId = OBJECT_INVALID;
    }

    g_pAppManager->m_pClientExoApp->GetSWGuiManager();

    // The gameplay option inverts the mouse-rotate latch; changing it moves the cursor in or out.
    int bRotate = g_bMouseCameraRotateToggle;
    if (g_pAppManager->m_pClientExoApp->GetClientOptions()->m_bGameplay1)
        bRotate = (bRotate == 0);
    if (bRotate != 0) {
        if (m_bMouseRotating == 0) {
            g_pGuiMan->StoreCurrentMousePosition();
            m_bMouseRotating = bRotate;
            g_pAppManager->m_pClientExoApp->HideMouse(CClientExoAppInternal::MOUSEHIDE_CAMERA_ROTATE);
        }
    } else if (m_bMouseRotating != 0) {
        g_pGuiMan->RestoreOldMousePosition();
        m_bMouseRotating = 0;
        g_pAppManager->m_pClientExoApp->ShowMouse(CClientExoAppInternal::MOUSEHIDE_CAMERA_ROTATE);
    }

    // A cutscene/takeover input class always releases the cursor and clears the latch.
    if (g_pAppManager->m_pClientExoApp->GetInputClass() == 2) {
        m_bMouseRotating = 0;
        g_pAppManager->m_pClientExoApp->ShowMouse(CClientExoAppInternal::MOUSEHIDE_CAMERA_ROTATE);
    }

    // With the keyboard rotate off, or a save-game stall in progress, the cursor is shown. Only when
    // no turn input is pending does the acceleration state integrate.
#ifndef K2_UI_TOUCH
    int nIos = bRotate;   // the PC's own latch (right button / Lookabout)
#else
    int nIos = g_bMouseCameraRotateToggleIos;
#endif
    int bNeg;
    int bPos;
    if (nIos == 0 || g_pAppManager->m_pClientExoApp->GetInGameGui()->m_bInGameMenuShown != 0)
        g_pAppManager->m_pClientExoApp->ShowMouse(CClientExoAppInternal::MOUSEHIDE_CAMERA_ROTATE);
    else
        g_pAppManager->m_pClientExoApp->HideMouse(CClientExoAppInternal::MOUSEHIDE_CAMERA_ROTATE);
    float fTurn = m_fTurn;  // read after the cursor call

    if (nIos == 0 && m_fTurn == 0.0f) {
        CSWPhysicsState state = m_splitAcceleration.Update(
            Vector(0.0f, m_fTurnAccel, 0.0f), fDeltaTime, 1, 0, 0, 0);
        fTurn = state.m_vDisplacement.y;
        bNeg = fTurn < 0.0f;
        bPos = fTurn > 0.0f;
    } else {
        m_splitAcceleration.Update(Vector(0.0f, fTurn / fMouseSen, 0.0f), fDeltaTime, 1, 0, 0, 0);
        m_fTurnAccel = 0.0f;
        bNeg = nIos;
        bPos = nIos;
    }

    // Two mirrored yaw steps, where free-slide takes the whole negated turn and otherwise the turn
    // is half-wave rectified by CameraHorzAxisFlip, in opposite directions for the two sides.
    if (bNeg) {
        float fYaw;
        if (nIos == 0 && bCameraSlideBack)
            fYaw = fTurn;
        else if (CameraHorzAxisFlip)
            fYaw = (fTurn < 0.0f) ? 0.0f : fTurn;
        else
            fYaw = (fTurn > 0.0f) ? 0.0f : fTurn;
        qDesired = YawPitchRoll(-fYaw, 0.0f, 0.0f) * qDesired;
    }
    if (bPos) {
        float fYaw;
        if (nIos == 0 && bCameraSlideBack)
            fYaw = fTurn;
        else if (CameraHorzAxisFlip)
            fYaw = (fTurn > 0.0f) ? 0.0f : fTurn;
        else
            fYaw = (fTurn < 0.0f) ? 0.0f : fTurn;
        qDesired = YawPitchRoll(-fYaw, 0.0f, 0.0f) * qDesired;
    }

    m_fTurn = 0.0f;
    m_vLastTargetPosition = m_pTargetObject.m_pObject->GetPosition();

    // A pending orientation request wins outright and resets to -1.0f.
    if (DesiredOrientation >= 0.0f) {
        qDesired = YawPitchRoll(DesiredOrientation, 0.0f, 0.0f);
        DesiredOrientation = -1.0f;
    }

    return qDesired;
}
// Drives the free-style camera in one of three modes selected by CameraFreeStyleMode (slide in
// world axes, accumulated yaw and pitch, slide along Z).
int CSWCameraOnAStick::CameraFreeStyleControl(float fDeltaTime)
{
    if (CameraFreeStyleMode == 0) {
        if (m_fTurn != 0.0f) {
            CAurObject* pOwner = m_pOwner;
            Vector vPos = pOwner->GetPosition();
            pOwner->SetPosition(vPos + Vector(1.0f, 0.0f, 0.0f) * fDeltaTime * m_fTurn);
        }
        if (m_fTiltAccel < 0.0f) {
            CAurObject* pOwner = m_pOwner;
            Vector vPos = pOwner->GetPosition();
            pOwner->SetPosition(vPos + Vector(0.0f, -1.0f, 0.0f) * fDeltaTime);
        }
        if (m_fTiltAccel > 0.0f) {
            CAurObject* pOwner = m_pOwner;
            Vector vPos = pOwner->GetPosition();
            pOwner->SetPosition(vPos + Vector(0.0f, 1.0f, 0.0f) * fDeltaTime);
        }
    }
    if (CameraFreeStyleMode == 1) {
        static float TYaw = 0.0f;
        static float TPitch = 0.0f;

        if (m_fTurn != 0.0f)
            TYaw += m_fTurn;
        if (m_fTiltAccel < 0.0f)
            TPitch += 0.1f;
        if (m_fTiltAccel > 0.0f)
            TPitch -= 0.1f;

        CAurObject* pOwner = m_pOwner;
        pOwner->SetOrientation(YawPitchRoll(TYaw, TPitch, 0.0f));
    }
    if (CameraFreeStyleMode == 2) {
        if (m_fTiltAccel < 0.0f) {
            CAurObject* pOwner = m_pOwner;
            Vector vPos = pOwner->GetPosition();
            pOwner->SetPosition(vPos + Vector(0.0f, 0.0f, 1.0f) * fDeltaTime);
        }
        if (m_fTiltAccel > 0.0f) {
            CAurObject* pOwner = m_pOwner;
            Vector vPos = pOwner->GetPosition();
            pOwner->SetPosition(vPos + Vector(0.0f, 0.0f, -1.0f) * fDeltaTime);
        }
    }

    m_fTurn = 0.0f;
    m_fTiltAccel = 0.0f;
    return 0;
}

// Per-frame driver.
int CSWCameraOnAStick::Control(float fParam)
{
    if (m_bPreserveFraming != 0) {
        float fRad = 0.017453292f;  // degrees to radians
        float fNumerator = m_fStyleDistance * tanf(m_fStyleViewAngle * fRad * 0.5f);
        float fViewAngle = static_cast<CAurCamera*>(m_pCameraObject->AsCAurCamera())->GetViewAngle();
        m_fDist = fNumerator / tanf(fViewAngle * fRad * 0.5f);
    }

    Control_CheckAndUpdateCameraStyle();

    if (!CAurBehavior::Control(fParam))
        return 1;

    if (CameraFreeStyle) {
        CameraFreeStyleControl(fParam);
        return 0;
    }

    if (m_pTargetObject.m_pObject == 0) {
        m_pOwner->SetBehavior(0);
        return 1;
    }

    // FLT_MAX means the behavior has not been placed yet.
    if (m_vLastTargetPosition.x >= 3.4028235e+38f)
        Control_SetCameraInintalState();

    m_qDesiredOrientation = Control_UpdateCameraDesiredOrientation(m_qDesiredOrientation, fParam);

    Vector vOwnerPos = m_pOwner->GetPosition();
    Vector vTarget = GetTargetHeadPosition();
    Vector vDesired = Control_ComputeDesiredPosition(vTarget);

    m_fInterpAmount = Control_AccelerateInterp(m_fInterpAmount, CameraInterpAmt1, m_fInterpClip,
                                               m_pOwner->GetPosition(), vDesired, fParam);

    Vector vNew = Control_ComputeNewCameraPosition(vDesired, vOwnerPos, vTarget, fParam);
    Vector vHit = Control_HitCheckCamera(vNew);

    if (RenderQA != 0 && m_pOwner != 0) {
        m_pOwner->GetPosition();
        m_pOwner->GetPosition();
        m_pOwner->GetPosition();
        magnitude(m_pOwner->GetPosition() - vTarget);
    }

    m_pOwner->SetPosition(vHit);
    m_pOwner->SetOrientation(Control_SetCameraOrientation(fParam));

    if (m_bIsDialog)
        m_bIsDialog = 0;

    return 1;
}

void CSWCameraOnAStick::SetCombatCamera(CAurObject* pTarget)
{
    m_pCombatCamera = pTarget;
}

void CSWCameraOnAStick::ClearCombatCamera()
{
    m_pCombatCamera = 0;
}

void CSWCameraOnAStick::ResetLastPosition(Vector vPosition, Quaternion qOrientation)
{
    m_vLastPosition = vPosition;
    m_vLastCameraPosition = vPosition;
    m_qDesiredOrientation = qOrientation;
    m_pOwner->SetPosition(vPosition);
    m_pOwner->SetOrientation(qOrientation);
}

void CSWCameraOnAStick::SaveCameraSettings(CCameraSettings& settings)
{
    settings.m_bValid = 1;
    settings.m_nCameraType = CCameraSettings::CAMERA_ON_STICK;
    settings.m_pCameraObject = m_pCameraObject;
    settings.m_pTarget = m_pTargetObject;
    settings.m_nStyle = m_nStyle;
    settings.m_nFollowingBody = m_nFollowingBody;
    settings.m_fTargetHeight = m_fCameraOffset;
    settings.m_nVideoEffect = -1;
#if !KOTOR1
    settings.m_fTilt = m_fTilt;
#endif
}

// Points the camera at a new target object.
void CSWCameraOnAStick::ChangeTarget(CAurObject* pTarget, int bFindClearView)
{
    K2_UNUSED(bFindClearView);
    m_fTurn = 0.0f;
    if (m_pTargetObject != pTarget) {
        RemoveDependency(m_pTargetObject);
        AddDependency(pTarget);
    }
    m_pTargetObject = pTarget;

    // The look direction in the XZ plane is the negated +X row of the target's matrix.
    Quaternion qTarget;
    Quaternion qOwner;
    Vector vDir;
    qTarget = m_pTargetObject.m_pObject->GetOrientation();
    qOwner = m_pOwner->GetOrientation();
    m_pOwner->SetOrientation(YawPitchRoll(Yaw(qTarget), Pitch(qOwner), Roll(qOwner)));

    vDir = -qTarget.ydir();
    vDir.z = 0.0f;
    vDir.Normalize();

    Vector vPos = m_pTargetObject.m_pObject->GetPosition();
    Vector vNew(vPos.x + m_fDist * vDir.x, vPos.y + m_fDist * vDir.y, m_fHeight);
    m_vLastTargetPosition = vNew;
    m_vLastCameraPosition = vNew;
}

// Points the camera at a world object: offset from the followed target toward it, plus the height.
void CSWCameraOnAStick::SetLookAtTarget(CSWCObject* pTarget)
{
    if (pTarget == 0)
        return;
    if (m_pTargetObject.m_pObject == 0)
        return;

    m_lookAtTargetId = pTarget->m_objectId;
    Vector vDelta = m_pTargetObject.m_pObject->GetPosition() - pTarget->m_position;
    m_vLookAtPosition = normalize(vDelta) * m_fDist;

    Vector vPos = m_pTargetObject.m_pObject->GetPosition();
    m_vLookAtPosition = Vector(vPos.x + m_vLookAtPosition.x, vPos.y + m_vLookAtPosition.y,
                      vPos.z + m_vLookAtPosition.z + (m_fCameraOffset + m_fHeight));
}

void CSWCameraOnAStick::ClearLookAtTarget()
{
    m_lookAtTargetId = OBJECT_INVALID;
}

int CSWCameraOnAStick::GetType() { return BEHAVIOR_CAMERA_ON_A_STICK; }

extern template void SafePointerList<CAurObject>::AddPointer(SafePointer<CAurObject>*);

CSWCameraFreeLook::CSWCameraFreeLook(CAurObject* pCameraObject, CAurObject* pFollowPart,
                                     int nVideoEffect)
    : CAurBehavior(pCameraObject)
{
    m_pCameraObject = pCameraObject;
    AddDependency(pFollowPart);
    m_pFollowPart = pFollowPart;
    m_fTiltSpeed = 0.0f;
    m_fPitchAngle = 0.0f;
    m_fTiltAccel = 0.0f;
    m_fShakeYaw = 0.0f;
    m_fShakePitch = 0.0f;
    UpdateCamraStyle();
    m_nVideoEffect = nVideoEffect;
    g_pAppManager->m_pClientExoApp->EnableVideoEffect(nVideoEffect);
    // Heavier damping and stiffness than the integrator's own default.
    m_acceleration.m_fMaxSpeed = 200.0f;
    m_acceleration.m_fAcceleration = 500.0f;
}

CSWCameraFreeLook::CSWCameraFreeLook(CCameraSettings& settings)
    : CAurBehavior(settings.m_pCameraObject)
{
    m_pFollowPart = settings.m_pTarget;
    m_pCameraObject = settings.m_pCameraObject;
    m_fTiltSpeed = 0.0f;
    m_fPitchAngle = 0.0f;
    m_fTiltAccel = 0.0f;
    UpdateCamraStyle();
    m_nVideoEffect = settings.m_nVideoEffect;
    g_pAppManager->m_pClientExoApp->EnableVideoEffect(m_nVideoEffect);
    m_acceleration.m_fMaxSpeed = 200.0f;
    m_acceleration.m_fAcceleration = 500.0f;
}

// Re-reads the free-look CameraStyle columns for the current area's style row.
void CSWCameraFreeLook::UpdateCamraStyle()
{
    C2DA* pCameraStyle = g_pRules->m_p2DArrays->m_pCameraStyle;
    if (pCameraStyle == 0) {
        m_fTurnSpeed = 60.0f;
        m_fTiltSpeedScale = 60.0f;
    }

    int nRow = g_pAppManager->m_pClientExoApp->GetModule()->m_pArea->m_cameraStyle;

#if KOTOR1
    pCameraStyle->GetFLOATEntry(nRow, CExoString("FL_TiltSpeed"), &m_fTiltSpeedScale);
#else
    pCameraStyle->GetFLOATEntry(nRow, CExoString("FL_PC_TiltSpeed"), &m_fTiltSpeedScale);
#endif
#if KOTOR1
    pCameraStyle->GetFLOATEntry(nRow, CExoString("FL_RotateSpeed"), &m_fTurnSpeed);
#else
    pCameraStyle->GetFLOATEntry(nRow, CExoString("FL_PC_RotateSpeed"), &m_fTurnSpeed);
#endif
    pCameraStyle->GetFLOATEntry(nRow, CExoString("FL_LOOKUP"), &m_fLookUp);
    pCameraStyle->GetFLOATEntry(nRow, CExoString("FL_LOOKDOWN"), &m_fLookDown);

    // The damping tracks the wider of the two look limits.
    m_acceleration.m_fMaxSpeed = m_fLookUp > m_fLookDown ? m_fLookUp : m_fLookDown;
}

CSWCameraFreeLook::~CSWCameraFreeLook()
{
    g_pAppManager->m_pClientExoApp->DisableVideoEffect();
}

void CSWCameraFreeLook::Message(char* pszParam)
{
    K2_UNUSED(pszParam);
}

// Returns 1 unconditionally; the base result only gates the body.
int CSWCameraFreeLook::Control(float fDeltaTime)
{
    // Both speeds are re-derived from the mouse-sensitivity slider every frame, so the 2DA columns
    // UpdateCamraStyle reads are only defaults. K1 has no slider re-derivation.
#if !KOTOR1
    float fSensitivity =
        static_cast<float>(g_pAppManager->m_pClientExoApp->GetClientOptions()->m_nMouseSensitivity) / 100.0f;
    m_fTurnSpeed = 15.0f * fSensitivity + 30.0f;
    m_fTiltSpeedScale = 25.0f * fSensitivity + 40.0f;
#endif

    if (CAurBehavior::Control(fDeltaTime)) {
        g_pAppManager->m_pClientExoApp->HideMouse(CClientExoAppInternal::MOUSEHIDE_CAMERA_ROTATE);

        // GetPartPosition only writes its out params on success, so the values below are fallbacks.
        Vector vHook;
        Quaternion qHook;
        if (!m_pFollowPart.m_pObject->GetPartPosition(const_cast<char*>("FreeLookHook"), &vHook, &qHook)) {
            if (!m_pFollowPart.m_pObject->GetPartPosition(const_cast<char*>("CameraHook"), &vHook, &qHook)) {
                // The third probe's result is ignored: the rise applies whenever the first two fail.
                m_pFollowPart.m_pObject->GetPartPosition(const_cast<char*>("root"), &vHook, &qHook);
                vHook.z += 2.0f;
            }
        }
        m_pOwner->SetPosition(vHook);

        // A non-zero tilt input integrates directly and clears the spring; otherwise the spring's
        // second state vector holds the increment.
        if (m_fTiltSpeed != 0.0f) {
#if KOTOR1
            // K1 clears both integrator states at once.
            m_fPitchAngle += m_fTiltSpeedScale * fDeltaTime * m_fTiltSpeed;
            __builtin_memset(&m_acceleration.m_prev, 0, 2 * sizeof(CSWPhysicsState));
#else
            m_acceleration.m_prev = CSWPhysicsState();
            m_fPitchAngle += m_fTiltSpeedScale * fDeltaTime * m_fTiltSpeed;
            m_acceleration.m_cur = CSWPhysicsState();
#endif
        } else {
            CSWPhysicsState state =
                m_acceleration.Update(Vector(0.0f, m_fTiltAccel, 0.0f), fDeltaTime, 1, 0, 0, 0);
#if KOTOR1
            float fPitchAngle = state.m_vDisplacement.y + m_fPitchAngle;
            m_fTiltAccel = 0.0f;
            m_fPitchAngle = fPitchAngle;
#else
            m_fTiltAccel = 0.0f;
            m_fPitchAngle += state.m_vDisplacement.y;
#endif
        }

        // Wrap into (-180, 180] and clamp to the 2DA's look limits.
        float fPitch = m_fPitchAngle;
        fPitch += fPitch < -180.0f ? 360.0f : 0.0f;
        fPitch -= fPitch > 180.0f ? 360.0f : 0.0f;
        if (fPitch < -m_fLookDown)
            fPitch = -m_fLookDown;
        if (fPitch > m_fLookUp)
            fPitch = m_fLookUp;
        m_fPitchAngle = fPitch;

        m_pOwner->SetOrientation(YawPitchRoll(Yaw(qHook) + m_fShakeYaw,
                                              m_fPitchAngle + 90.0f + m_fShakePitch, 0.0f));
    }
    return 1;
}

void CSWCameraFreeLook::ChangeTarget(CAurObject* pFollowPart, int bFindClearView)
{
    // Retarget only when it really changes; the assignment below always runs.
    if (m_pFollowPart != pFollowPart) {
        RemoveDependency(m_pFollowPart);
        AddDependency(pFollowPart);
    }
    m_pFollowPart = pFollowPart;
    if (bFindClearView == 1) {
        // Snap the camera onto the new target.
        Vector v = m_pFollowPart.m_pObject->GetPosition();
        m_pOwner->SetPosition(v);
    }
}

void CSWCameraFreeLook::SaveCameraSettings(CCameraSettings& settings)
{
    settings.m_bValid = 1;
    settings.m_nCameraType = CCameraSettings::CAMERA_FREE_LOOK;
    settings.m_pCameraObject = m_pCameraObject;
    settings.m_pTarget = m_pFollowPart;
    settings.m_nVideoEffect = m_nVideoEffect;
}

int CSWCameraFreeLook::GetType() { return BEHAVIOR_CAMERA_FREE_LOOK; }

char* firstword(char* pszLine);
void  Parse(char* pszLine, float& fValue);


extern template void SafePointerList<CAurObject>::AddPointer(SafePointer<CAurObject>*);

CSWCameraDeath::CSWCameraDeath(CAurObject* pOwner, CAurObject* pTarget)
    : CAurBehavior(pOwner)
{
    m_fYaw = 0.0f;
    m_fSpeedYaw = 30.0f;
    m_fDesiredPitch = 90.0f;
    m_fDesiredDistance = 3.0f;
    m_fLogYaw = 0.5f;
    m_fLogPitch = 0.01f;
    m_fLogDistance = 0.5f;
    m_fHeight = 0.75f;
    m_fDesiredHeight = 0.0f;
    m_fLogHeight = 0.01f;
    SetTarget(pTarget);
}

// Retargets the death camera, unwinding the old registration and seeding the yaw from the target
// delta.
void CSWCameraDeath::SetTarget(CAurObject* pTarget)
{
    if (m_spTarget)
        RemoveDependency(m_spTarget);

    if (pTarget) {
        AddDependency(pTarget);
        m_fYaw = Yaw(normalize(pTarget->GetPosition() - m_pOwner->GetPosition()));
        m_fHeight = 0.75f;
    }

    m_spTarget = pTarget;
}

CSWCameraDeath::~CSWCameraDeath()
{
}

// Runs the death camera's per-frame orbit, holding the target, blending the aim toward it and
// pulling back.
int CSWCameraDeath::Control(float fDelta)
{
    // Pin the owner for the duration of the tick.
    SafePointer<CAurObject> spOwner(m_pOwner);

    if (CAurBehavior::Control(fDelta) == 0)
        return 0;

    if (spOwner.m_pObject->GetBehavior(-1) == this && m_spTarget != 0) {
        // Smooth the orbit offset, then advance and wrap the yaw.
        m_fHeight = m_fHeight + (m_fDesiredHeight - m_fHeight) * m_fLogHeight;

        m_fYaw = m_fYaw + m_fSpeedYaw * fDelta;
        if (m_fYaw < 0.0f)
            m_fYaw = m_fYaw + 360.0f;
        else if (m_fYaw >= 360.0f)
            m_fYaw = m_fYaw - 360.0f;

        // Both targets are unwrapped onto the same turn as the current yaw: the owner's actual
        // orientation is walked up in 360-degree steps until it meets the desired angle.
        float fYawTarget = Yaw(m_pOwner->GetOrientation());
        while (fYawTarget + 360.0f < m_fYaw)
            fYawTarget = fYawTarget + 360.0f;

        float fPitchTarget = 90.0f - m_fDesiredPitch;
        if (fPitchTarget < 0.0f)
            fPitchTarget = fPitchTarget + 360.0f;

        float fPitch = Pitch(m_pOwner->GetOrientation());
        while (fPitch < fPitchTarget)
            fPitch = fPitch + 360.0f;

        // The blended aim, as a rotation.
        Quaternion q = YawPitchRoll(fYawTarget + (m_fYaw - fYawTarget) * m_fLogYaw,
                                    fPitch + (fPitchTarget - fPitch) * m_fLogPitch, 0.0f);

        // Pull back by the current distance to the target, relaxed toward m_fDesiredDistance.
        float fDist = magnitude(m_spTarget.m_pObject->GetPosition() - m_pOwner->GetPosition());
        float fApproach = (m_fDesiredDistance - fDist) * m_fLogDistance;

        // The camera rests at the target raised by the smoothed offset.
        Vector vPos = m_spTarget.m_pObject->GetPosition() + Vector(0.0f, 0.0f, m_fHeight);

        Vector vHitPos(0.0f, 0.0f, 0.0f);
        CHitInfo hitInfo(0, ~CHitInfo::kMaterialTrigger, 0);
        hitInfo.m_bSkipCharactersAndItems = 1;
        hitInfo.m_ignoreList.Add(m_spTarget);
        hitInfo.ClearOutput();

        // Pull back along the aim's own forward axis, the rotation's third column, by the relaxed
        // distance.
        Vector vEnd = vPos + q.zdir() * (fDist + fApproach);

        // The wall probe runs from the target out past the camera by 25 percent of that reach; on a
        // hit the camera stops 25 percent into the surface the scene reported.
        Vector vDir = normalize(vEnd - vPos);
        hitInfo.m_bHonorIgnoreHitCheck = 1;
        Vector vTo = vEnd + vDir * 0.25f;
        CAurScene* pScene = m_pOwner->GetScene();
        if (pScene->HitCheck(vPos, vTo, &vHitPos, &hitInfo) != 0)
            vEnd = vHitPos + hitInfo.m_vHitNormal * 0.25f;

        m_pOwner->SetPosition(vEnd);
        m_pOwner->SetOrientation(q);
    }
    return 1;
}

void CSWCameraDeath::Message(char* pszParam)
{
    if (_stricmp("m_yaw", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_yaw"), m_fYaw);
    if (_stricmp("m_speedYaw", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_speedYaw"), m_fSpeedYaw);
    if (_stricmp("m_desiredPitch", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_desiredPitch"), m_fDesiredPitch);
    if (_stricmp("m_desiredDistance", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_desiredDistance"), m_fDesiredDistance);
    if (_stricmp("m_logYaw", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_logYaw"), m_fLogYaw);
    if (_stricmp("m_logPitch", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_logPitch"), m_fLogPitch);
    if (_stricmp("m_logDistance", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_logDistance"), m_fLogDistance);
}

int CSWCameraDeath::GetType() { return BEHAVIOR_CAMERA_DEATH; }

// Console command that attaches a death camera to the player's camera.
char* DeathCamera(char* pszParam)
{
    K2_UNUSED(pszParam);
    if (g_pAppManager->m_pClientExoApp != 0) {
        CSWCModule* pModule = g_pAppManager->m_pClientExoApp->GetModule();
        CSWCCreature* pCreature = g_pAppManager->m_pClientExoApp->GetPlayerCreature();

        if (pModule != 0 && pCreature != 0 && pModule->m_pCamera != 0) {
            CAurCamera* pCamera = pModule->m_pCamera;
            pCamera->SetBehavior(new CSWCameraDeath(static_cast<CAurObject*>(pCamera->AsCAurObject()),
                                                    static_cast<CAurObject*>(pCreature->GetCameraFollowPart())));
        }
    }
    return const_cast<char*>("Success");
}

char* firstword(char* pszLine);
void  Parse(char* pszLine, float& fValue);


CSWCameraNavigate::CSWCameraNavigate(CAurObject* pTarget)
    : CAurBehavior(pTarget)
{
    m_fTurn = 0.0f;
    m_fTilt = 0.0f;
    m_fSpeed = 0.0f;
    m_fRaise = 0.0f;
    m_fStrafe = 0.0f;
    m_fRoll = 0.0f;
    m_fSmoothedRoll = 0.0f;
}

void CSWCameraNavigate::Message(char* pszParam)
{
    if (_stricmp("turn", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("turn"), m_fTurn);
    if (_stricmp("tilt", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("tilt"), m_fTilt);
    if (_stricmp("speed", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("speed"), m_fSpeed);
    if (_stricmp("raise", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("raise"), m_fRaise);
    if (_stricmp("strafe", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("strafe"), m_fStrafe);
    if (_stricmp("roll", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("roll"), m_fRoll);
}

extern int g_nHSMode;

extern int RenderQA;
extern int quaternionidentityoptimization;

// Local min/max; <algorithm> is not usable in this TU.
template <class T> static inline T min(T a, T b) { return a < b ? a : b; }
template <class T> static inline T max(T a, T b) { return a > b ? a : b; }

// Drives the camera each frame, with a high-speed swoop arm on g_nHSMode and the ordinary navigate
// arm.
int CSWCameraNavigate::Control(float fParam)
{
    if (CAurBehavior::Control(fParam) == 0)
        return 0;

    if (g_nHSMode != 0) {
        m_fSpeed *= 5.0f;
        m_fStrafe *= 3.0f;
        m_fTurn *= 2.67f;
        m_fTilt *= 3.33f;

        // The composition order is asymmetric: only the first product has the new rotation on the left.
        Quaternion qOwner = m_pOwner->GetOrientation();
        Quaternion q = YawPitchRoll(m_fTurn * fParam, 0.0f, 0.0f) * qOwner;
        m_fTurn = 0.0f;
        q = q * YawPitchRoll(0.0f, -90.0f, 0.0f);

        // Smooth the roll with a lerp whose two weights sum to 1, seeding both members with the
        // result.
        float fRate = fParam + fParam;
        float fBlend = m_fRoll * (fRate + 0.033f) + m_fSmoothedRoll * (0.967f - fRate);
        m_fRoll = fBlend;
        m_fSmoothedRoll = fBlend;
        q = q * YawPitchRoll(0.0f, 0.0f, fBlend - Roll(q));

        // Probe the elevation by tilting the aim down by this frame's tilt, keeping the result only
        // while it stays inside the +-90 degree gimbal window.
        float fPitch = Pitch(q) - m_fTilt * fParam;
        if (fPitch > -90.0f && fPitch < 90.0f)
            q = q * YawPitchRoll(0.0f, -m_fTilt * fParam, 0.0f);

        // Keep the pre-rotation aim: the position move below uses it, not `q`.
        Quaternion qAim = q;
        m_fTilt = 0.0f;
        q = q * YawPitchRoll(0.0f, 90.0f, 0.0f);
        q = normalize(q);
        m_pOwner->SetOrientation(q);

        // Both members are read and cleared before the guard, so the stores are unconditional.
        float fStrafe = m_fStrafe;
        float fSpeed = m_fSpeed;
        m_fStrafe = 0.0f;
        m_fSpeed = 0.0f;
        if (fStrafe != 0.0f || fSpeed != 0.0f) {
            // Identity fast-out, using the raw local vector when the rotation is a no-op.
            Vector vWorld(fStrafe, -fSpeed, 0.0f);
            if (quaternionidentityoptimization == 0 || (qAim.w != 1.0f && qAim.w != -1.0f)) {
                Matrix mat = qAim.getmatrix();
                vWorld = Vector(mat.m_f[0] * vWorld.x + mat.m_f[3] * vWorld.y + mat.m_f[6] * vWorld.z,
                                mat.m_f[1] * vWorld.x + mat.m_f[4] * vWorld.y + mat.m_f[7] * vWorld.z,
                                mat.m_f[2] * vWorld.x + mat.m_f[5] * vWorld.y + mat.m_f[8] * vWorld.z);
            }
            CAurObject* pOwner = m_pOwner;
            pOwner->SetPosition(pOwner->GetPosition() + vWorld * fParam);
        }
    } else {
        float fYaw = Yaw(m_pOwner->GetOrientation());
        float fPitch = Pitch(m_pOwner->GetOrientation());

        // The tilt and forward speed flip once the camera's up axis has rolled past the horizon. The
        // matrix comes from the owner's orientation, not the aimed rotation below.
        Vector vUp = m_pOwner->GetOrientation().zdir();
        Vector vWorldUp(0.0f, 0.0f, 1.0f);
        float fUp = dot(vUp, vWorldUp);
        float fSign;
        if (fUp >= 0.0f)
            fSign = -1.0f;
        else
            fSign = 1.0f;

        fYaw += m_fTurn * fParam;
        fPitch += m_fTilt * fSign * fParam;
        // A NaN pitch stays NaN.
        fPitch = fPitch < 0.0f ? 0.0f : fPitch;
        m_fTurn = 0.0f;
        m_fTilt = 0.0f;

        // The aim, held 90 degrees off the navigate plane.
        Quaternion q = YawPitchRoll(fYaw, fPitch - 90.0f,
                                    Roll(m_pOwner->GetOrientation()));

        if (m_fStrafe != 0.0f || fSign * m_fSpeed != 0.0f) {
            // The local-frame offset; an exactly identity aim skips the rotation.
            Vector vWorld(m_fStrafe, fSign * m_fSpeed, 0.0f);
            if (quaternionidentityoptimization == 0 || (q.w != 1.0f && q.w != -1.0f)) {
                Matrix mat = q.getmatrix();
                vWorld = Vector(mat.m_f[0] * vWorld.x + mat.m_f[3] * vWorld.y + mat.m_f[6] * vWorld.z,
                                mat.m_f[1] * vWorld.x + mat.m_f[4] * vWorld.y + mat.m_f[7] * vWorld.z,
                                mat.m_f[2] * vWorld.x + mat.m_f[5] * vWorld.y + mat.m_f[8] * vWorld.z);
            }
            CAurObject* pOwner = m_pOwner;
            pOwner->SetPosition(pOwner->GetPosition() + vWorld * fParam);
        }

        m_pOwner->SetOrientation(YawPitchRoll(fYaw, fPitch, Roll(m_pOwner->GetOrientation())));
    }

    // The QA overlay probe fetches the positions and discards them.
#if !KOTOR1  // K1 has no such probe
    if (RenderQA != 0 && m_pOwner != 0) {
        m_pOwner->GetPosition();
        m_pOwner->GetPosition();
        m_pOwner->GetPosition();
    }
#endif
    return 1;
}

int CSWCameraNavigate::GetType() { return BEHAVIOR_CAMERA_NAVIGATE; }

// Where the camera wants to be this frame: vTarget pulled back along the owner's Y axis by the
// interpolated distance, with the target's height added on z.
Vector CSWCameraOnAStick::Control_ComputeNewCameraPosition(Vector vDesired, Vector vOwnerPos,
                                                           Vector vTarget, float fDeltaTime)
{
    K2_UNUSED(vDesired);
    Vector vDelta(vOwnerPos.x - vTarget.x, vOwnerPos.y - vTarget.y, 0.0f);

    Quaternion qDir(zaxis, Yaw(-vDelta) * 0.017453292f);
    float fDirYaw = Yaw(qDir);
    if (fDirYaw < 0.0f)
        fDirYaw += 360.0f;

    float fOwnerYaw = Yaw(m_qDesiredOrientation);
    if (fOwnerYaw < 0.0f)
        fOwnerYaw += 360.0f;

    // The signed yaw step toward the target, wrapped into [-180, 180). The two outer arms push the
    // naive difference back across the 0/360 seam.
    float fDiff;
    if (fDirYaw >= 270.0f && fOwnerYaw <= 90.0f)
        fDiff = (360.0f - fDirYaw) + fOwnerYaw;
    else if (fDirYaw <= 90.0f && fOwnerYaw >= 270.0f)
        fDiff = -(fDirYaw + (360.0f - fOwnerYaw));
    else
        fDiff = fOwnerYaw - fDirYaw;

    float fAngle;
    if ((fDiff > 179.9999f && fDiff < 180.0001f)
        || (fDiff < -179.9999f && fDiff > -180.0001f)) {
        fAngle = 180.0f;
    } else {
        if (fDiff > 180.0f)
            fDiff -= 360.0f;
        else if (fDiff < -180.0f)
            fDiff += 360.0f;
        fAngle = fDiff;
    }

    if (m_bIsDialog) {
        // The dialog and shot camera snap to the target's own orientation on the first frame.
        m_bIsDialog = 0;
        m_qDesiredOrientation = m_pTargetObject.m_pObject->GetOrientation();
        fAngle = Yaw(m_qDesiredOrientation);
    } else {
        fAngle = fAngle * m_fInterpAmount * fDeltaTime;
    }

    if (fAngle > 360.0f)
        fAngle -= 360.0f;

    // Set by the settings-restore constructor: this frame keeps the current yaw.
    float fDestYaw;
    if (m_bKeepCurrentYaw) {
        m_bKeepCurrentYaw = 0;
        fDestYaw = fOwnerYaw;
    } else {
        if (fAngle < 0.0f)
            fAngle += 360.0f;
        fDestYaw = fDirYaw + fAngle;
    }

    // The new pull-back axis is the owner's +Y rotated into the new yaw, scaled by the distance
    // lerped halfway toward the measured target distance.
    Quaternion qNew(zaxis, fDestYaw * 0.017453292f);
    float fDeltaDist = magnitude(vDelta) - m_fDist;
    float fDist = m_fDist + (fDeltaDist - fDeltaDist * CameraInterpDistAmt);

    Vector vOffset = RotateVector(Vector(0.0f, 1.0f, 0.0f) * fDist, qNew);
    return Vector(vTarget.x - vOffset.x, vTarget.y - vOffset.y, vTarget.z + m_fHeight);
}

float CameraPersonalSpace = 0.349999994f;

// Casts the line from the followed object to the wanted position each frame, lets the server
// shorten it past any creature in the way, then probes four points around that endpoint and keeps
// the farthest hit.
Vector CSWCameraOnAStick::Control_HitCheckCamera(Vector vNewPosition)
{
    // The offered endpoint and the position returned at the end; the shrink can move the first.
    Vector vEnd = vNewPosition;
    Vector vPos = vNewPosition;

    // The four probes' hit points, zeroed once up front.
    Vector vOut1;
    Vector vOut2;
    Vector vOut3;
    Vector vOut4;

    CHitInfo hitInfo(0, ~CHitInfo::kMaterialTrigger, 0);
    hitInfo.m_bSkipCharactersAndItems = 1;
    hitInfo.m_ignoreList.Add(m_pTargetObject.m_pObject);

    // One pass over the probes.
    for (int i = 1; i > 0; i--) {
        hitInfo.ClearOutput();

        // The followed object's own position, lifted by the camera's height offset.
        Vector vStart = GetTargetHeadPosition();

        // Let the server shorten the line past creatures; a wall between the two points does not move
        // it. The shortened endpoint carries forward.
        Vector vShrunk;
        if (g_pAppManager->m_pServerExoApp->ShrinkLineByCreatures(vStart, vEnd, vShrunk)) {
            vEnd = vShrunk;
            vPos = vEnd;
        }

        // The side axis is world-up crossed with the endpoint-to-start direction.
        Vector vToStart(vStart.x - vEnd.x, vStart.y - vEnd.y, vStart.z - vEnd.z);
        Vector vSide = normalize(cross(zaxis, vToStart));

        // Each probe pair shares one offset.
        Vector vSideOff = vSide * CameraPersonalSpace;
        Vector vProbe1(vEnd + vSideOff);
        hitInfo.ClearOutput();
        hitInfo.m_bHonorIgnoreHitCheck = 1;
        int nHit1 = m_pOwner->GetScene()->HitCheck(vStart, vProbe1, &vOut1, &hitInfo);

        Vector vProbe2(vEnd - vSideOff);
        hitInfo.ClearOutput();
        hitInfo.m_bHonorIgnoreHitCheck = 1;
        int nHit2 = m_pOwner->GetScene()->HitCheck(vStart, vProbe2, &vOut2, &hitInfo);

        Vector vUpOff = Vector(0.0f, 0.0f, 1.0f) * CameraPersonalSpace;
        Vector vProbe3(vEnd + vUpOff);
        hitInfo.ClearOutput();
        hitInfo.m_bHonorIgnoreHitCheck = 1;
        int nHit3 = m_pOwner->GetScene()->HitCheck(vStart, vProbe3, &vOut3, &hitInfo);

        Vector vProbe4(vEnd - vUpOff);
        hitInfo.ClearOutput();
        hitInfo.m_bHonorIgnoreHitCheck = 1;
        int nHit4 = m_pOwner->GetScene()->HitCheck(vStart, vProbe4, &vOut4, &hitInfo);

        // With nothing in the way anywhere, hand the offered position straight back.
        if (!nHit1 && !nHit2 && !nHit3 && !nHit4)
            break;

        Vector vAway(vEnd.x - vStart.x, vEnd.y - vStart.y, vEnd.z - vStart.z);
        float fBest = magnitude(vAway);

        // The farthest hit of the four wins, so magnitude() is called twice per probe.
        float fDist = 0.0f;
#if KOTOR1
        // K1 measures each probe once.
        if (nHit1) {
            float fHit = magnitude(vOut1 - vProbe1);
            fDist = (fHit < 0.0f) ? 0.0f : fHit;
        }
        if (nHit2) {
            float fHit = magnitude(vOut2 - vProbe2);
            fDist = (fDist > fHit) ? fDist : fHit;
        }
        if (nHit3) {
            float fHit = magnitude(vOut3 - vProbe3);
            fDist = (fDist > fHit) ? fDist : fHit;
        }
        if (nHit4) {
            float fHit = magnitude(vOut4 - vProbe4);
            fDist = (fDist > fHit) ? fDist : fHit;
        }
#else
        if (nHit1) {
            fDist = (magnitude(vOut1 - vProbe1) < 0.0f) ? 0.0f : magnitude(vOut1 - vProbe1);
        }
        if (nHit2) {
            fDist = (fDist > magnitude(vOut2 - vProbe2)) ? fDist : magnitude(vOut2 - vProbe2);
        }
        if (nHit3) {
            fDist = (fDist > magnitude(vOut3 - vProbe3)) ? fDist : magnitude(vOut3 - vProbe3);
        }
        if (nHit4) {
            fDist = (fDist > magnitude(vOut4 - vProbe4)) ? fDist : magnitude(vOut4 - vProbe4);
        }
#endif

        Vector vLine = normalize(vAway);

        // The target's own local +Z.
        Vector vAxis = normalize(m_pTargetObject.m_pObject->GetOrientation().zdir());

        // The lift grows with the fraction of the line the hit consumed.
        float fLift = (fDist / fBest) * (CameraPersonalSpace + 0.15f);
        vPos = Vector(vPos.x - fDist * vLine.x + vAxis.x * fLift,
                      vPos.y - fDist * vLine.y + vAxis.y * fLift,
                      vPos.z - fDist * vLine.z + vAxis.z * fLift);
    }
    return vPos;
}

extern template void SafePointerList<CAurObject>::AddPointer(SafePointer<CAurObject>*);

char* firstword(char* pszLine);
void  Parse(char* pszLine, float& fValue);
void  Parse(char* pszLine, Vector& vValue);


void CSWCameraStaticTracker::SetOffset(Vector offset)
{
    m_offset = offset;
}

void CSWCameraStaticTracker::Init()
{
    m_fTrackSpeed = 10.0f;
    m_fMinPitch = 45.0f;
    m_offset = Vector(0.0f, 0.0f, 1.0f);
    m_bZoomByDistance = 0;
}

// Aims the tracker and its owner's camera at the watched object. fParam == 0 snaps; otherwise the
// turn is exponential-smoothed.
void CSWCameraStaticTracker::PointToTarget(float fParam)
{
    Vector vOwner = m_pOwner->GetPosition();
    Vector vDelta = m_spTarget.m_pObject->GetPosition() + m_offset - vOwner;
    float fDist = magnitude(vDelta);
    if (m_bZoomByDistance != 0) {
        // 1/8 of the distance, so 8 units of range is one unit of scale; the 45-degree base is the
        // tracker's nominal half-FOV.
        float fAngle = 45.0f / (fDist * 0.125f);
        if (fAngle > 90.0f)
            fAngle = 90.0f;
        static_cast<CAurCamera*>(m_pOwner->AsCAurCamera())->SetViewAngle(fAngle);
    }
    float fPitch = Pitch(vDelta) + 90.0f;
    if (fPitch < m_fMinPitch)
        fPitch = m_fMinPitch;
    Vector vNorm = normalize(vDelta);
    Quaternion q = YawPitchRoll(Yaw(vNorm), fPitch, 0.0f);
    m_orientation = q;
    // fParam == 0 means snap: the freshly built aim is used as-is. Otherwise it is exponentially
    // smoothed against the camera's current orientation, 1 - 2^(-speed*fParam).
    if (fParam != 0.0f) {
        float fT = 1.0f - exp2f(-m_fTrackSpeed * fParam);
        m_pOwner->SetOrientation(normalize(slerp(m_pOwner->GetOrientation(), m_orientation, fT)));
    } else {
        m_pOwner->SetOrientation(normalize(m_orientation));
    }
}

void CSWCameraStaticTracker::Message(char* pszParam)
{
    if (_stricmp("m_fTrackSpeed", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_fTrackSpeed"), m_fTrackSpeed);
    if (_stricmp("m_fMinPitch", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_fMinPitch"), m_fMinPitch);
    if (_stricmp("m_vOffset", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_vOffset"), m_offset);
}

int CSWCameraStaticTracker::GetType() { return BEHAVIOR_CAMERA_STATIC_TRACKER; }

void CSWCameraStaticTracker::SetPosition(Vector v)
{
    m_pOwner->SetPosition(v);
}

CSWCameraStaticTracker::CSWCameraStaticTracker(CAurObject* pOwner, CAurObject* pTarget)
    : CAurBehavior(pOwner)
{
    AddDependency(pTarget);
    m_spTarget = pTarget;
    Init();
    PointToTarget(0.0f);
}

CSWCameraStaticTracker::CSWCameraStaticTracker(CAurObject* pOwner, CAurObject* pTarget, Vector vOffset)
    : CAurBehavior(pOwner)
{
    AddDependency(pTarget);
    m_spTarget = pTarget;
    Init();
    m_pOwner->SetPosition(vOffset);
    PointToTarget(0.0f);
}

// The base call must be base-qualified, or a plain Control() would recurse.
int CSWCameraStaticTracker::Control(float fParam)
{
    if (CAurBehavior::Control(fParam))
        PointToTarget(fParam);
    return 1;
}

extern template void SafePointerList<CAurObject>::AddPointer(SafePointer<CAurObject>*);

char* firstword(char* pszLine);
void  Parse(char* pszLine, float& fValue);
void  Parse(char* pszLine, Vector& vValue);


CSWBehaviorCameraFollow::CSWBehaviorCameraFollow(CAurObject* pCam, CAurObject* pTarget)
    : CAurBehavior(pCam)
{
    AddDependency(pTarget);
    m_pTarget = pTarget;
    m_vFollowPosition = pTarget->GetPosition();

    m_vOffset = Vector(0.0f, 0.0f, 1.0f);
    m_fDist = 8.0f;
    m_fZoomDeltaAccum = 0.0f;
    m_fFOV = 750.0f;
    m_fYaw = 0.0f;
    m_fYawVel = 0.0f;
    m_fPitch = 45.0f;
    m_fPitchVel = 0.0f;
    m_fYawClipGate = 0.0f;
    m_fYawClipMax = 360.0f;
    m_fYawClipMin = 0.0f;
    m_fDistClipMin = 0.0f;
    m_fDistClipMax = 10.0f;
    m_fPitchClipGate = 0.0f;
    m_fPitchClipMin = 0.0f;
    m_fPitchClipMax = 360.0f;
    m_fDistClipGate = 0.0f;
}

CSWBehaviorCameraFollow::~CSWBehaviorCameraFollow() {
}

void CSWBehaviorCameraFollow::Message(char* pszParam) {
    if (_stricmp("turn", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("turn"), m_fYawVel);
    if (_stricmp("spring", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("spring"), m_fFOV);
    if (_stricmp("dist", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("dist"), m_fDist);
    if (_stricmp("offset", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("offset"), m_vOffset);
    if (_stricmp("yaw", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("yaw"), m_fYaw);
    if (_stricmp("pitch", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("pitch"), m_fPitch);
    if (_stricmp("tilt", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("tilt"), m_fPitchVel);
    if (_stricmp("speed", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("speed"), m_fZoomDeltaAccum);
    if (_stricmp("minyaw", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("minyaw"), m_fYawClipMin);
    if (_stricmp("maxyaw", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("maxyaw"), m_fYawClipMax);
    if (_stricmp("minpitch", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("minpitch"), m_fPitchClipMin);
    if (_stricmp("maxpitch", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("maxpitch"), m_fPitchClipMax);
    if (_stricmp("mindist", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("mindist"), m_fDistClipMin);
    if (_stricmp("maxdist", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("maxdist"), m_fDistClipMax);
    if (_stricmp("clampyaw", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("clampyaw"), m_fYawClipGate);
    if (_stricmp("clamppitch", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("clamppitch"), m_fPitchClipGate);
    if (_stricmp("clampdist", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("clampdist"), m_fDistClipGate);
}

// Ticks the follow camera each frame, integrating yaw and pitch, decaying the distance, blending
// the position toward the target, slerping the orientation and placing the camera along its own
// forward axis.
int CSWBehaviorCameraFollow::Control(float fParam) {
    // Identity orientation, overwritten below.
    Quaternion qGoal;

    if (CAurBehavior::Control(fParam) == 0)
        return 1;

    CAurObject* pTarget = m_pTarget;
    if (pTarget == 0) {
        m_pOwner->SetBehavior(0);   // drop the behavior
        return 1;
    }

    m_fYaw += m_fYawVel * fParam;
    m_fYawVel = 0;
    if (m_fYawClipGate != 0.0f)
        m_fYaw = max(m_fYawClipMin, min(m_fYaw, m_fYawClipMax));

    m_fPitch += m_fPitchVel * fParam;
    m_fPitchVel = 0;
    if (m_fPitchClipGate != 0.0f)
        m_fPitch = max(m_fPitchClipMin, min(m_fPitch, m_fPitchClipMax));

    float fDist = m_fDist;
    m_fDist = fDist * exp2f(m_fZoomDeltaAccum * fParam * -0.5f);
    m_fZoomDeltaAccum = 0;
    if (m_fDistClipGate != 0.0f)
        m_fDist = max(m_fDistClipMin, min(m_fDist, m_fDistClipMax));

    float fBlend = 1.0f - exp2f(-m_fFOV * fParam);

    Vector vPos = (pTarget->GetPosition() + m_vOffset) * fBlend + m_vFollowPosition * (1.0f - fBlend);

    qGoal = YawPitchRoll(m_fYaw, m_fPitch, 0.0f);
    // The owner is re-read for each call pair rather than hoisted into one local.
    CAurObject* pOwner = m_pOwner;
    Quaternion qCam = pOwner->GetOrientation();
    qCam = normalize(slerp(qCam, qGoal, fBlend));
    pOwner->SetOrientation(qCam);

    pOwner = m_pOwner;
    pOwner->SetPosition(vPos + pOwner->GetOrientation().zdir() * m_fDist);

    m_vFollowPosition = vPos;
    return 1;
}

inline int CSWBehaviorCameraFollow::GetType() { return BEHAVIOR_CAMERA_FOLLOW; }
