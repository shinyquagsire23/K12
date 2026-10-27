#if KOTOR1
#define K1_AURORA_LIST  // K1's AURORA library spells List.h differently
#endif
#include "include/aurbehavior.h"

#include "AURORA/aurbehavior.h"
#include "exosound.h"
#include "serverexoappinternal.h"
#include "appmanager.h"
#include "clientexoapp.h"
#include "clientoptions.h"

extern int g_bMouseCameraRotateToggleIos;
#include "aurcamera.h"
#include "AURORA/aurmodel.h"
#include "aurscene.h"
#include "Matrix.h"
#include "Quaternion.h"

// Header-free equivalents of std::min/std::max: <algorithm> drags in <math.h>, which conflicts
// with the throw()-specified CRT declarations below.
template <class T> static inline const T& min(const T& a, const T& b) { return b < a ? b : a; }
template <class T> static inline const T& max(const T& a, const T& b) { return a < b ? b : a; }

// <string.h> is needed for the strlen calls on the command literals in the Message handlers.
#include <string.h>

// `_stricmp` is not in bionic's <string.h> (it spells it strcasecmp), so it is declared here.
#include "port/crtapi.h"

// The current-scene global the per-frame Control hooks read.

// The debug console's gob/scene lookups.
Gob* FindGob(char* pszName);
Scene* FindScene(char* pszName);

#include "port/libcbase.h"
#include "port/libcmath.h"

// In the aurparse cursor helpers, `firstword` returns a pointer into the line, `Parse` consumes the
// rest of the cursor and a reference to the field being set.
char* firstword(char* pszLine);
void Parse(char* pszLine, int& nValue);
void Parse(char* pszLine, Vector& vValue);
void Parse(char* pszLine, float& fValue);

float DegToRad(float fDegrees);

// A local variadic formatter that prints " %f  %f " into the buffer it is handed. Nothing reads
// the result; it exists so Control's call to it stays direct.
#include <stdarg.h>
static void FormatDebugFloats(char* pszBuffer, ...) {
    va_list args;
    va_start(args, pszBuffer);
    vsprintf(pszBuffer, " %f  %f ", args);
    va_end(args);
}

// All five of the CAurBehavior base's own leaf bodies are stubs.
void CAurBehavior::SetScene() {
}

void* CAurBehavior::AsBehaviorAttach() {
    return 0;
}

void* CAurBehavior::AsBehaviorNavigate() {
    return 0;
}

CAurBehaviorCameraFollow* CAurBehavior::AsBehaviorCameraFollow() {
    return 0;
}

int CAurBehavior::GetType() {
    return 0;
}

CAurBehavior::CAurBehavior(CAurObject* pOwner) {
    m_pOwner = pOwner;
}

char* nullscene(char* pszGobName) {
    // Resolves a gob by name and detaches it from whatever scene it is in.
    Gob* pGob = FindGob(pszGobName);
    if (pGob == 0)
        return const_cast<char*>("hey, no Gob by that name");
    pGob->SetScene(0);
    return const_cast<char*>("OK");
}

char* setscene(char* pszParam) {
    // The companion console command. Both answer with a string the console prints. Two char[64]
    // buffers, so the sscanf return value doubles as the arity check.
    char szGobName[64];
    char szSceneName[64];
    if (sscanf(pszParam, "%s %s", szGobName, szSceneName) != 2)
        return const_cast<char*>("Must specify gobname and scenename!");
    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return const_cast<char*>("hey, no Gob by that name");
    Scene* pScene = FindScene(szSceneName);
    if (pScene == 0)
        return const_cast<char*>("hey, no Scene by that name");
    pGob->SetScene(pScene);
    return const_cast<char*>("OK");
}

// The three CAurBehaviorAttach subclasses.

CAurBehaviorAttachTranslate::CAurBehaviorAttachTranslate(CAurObject* pObj, CAurObject* pAttachTo,
                                                         CAurPart* pPart)
    : CAurBehaviorAttach(pObj, pAttachTo, pPart) {
    // m_bUnrotatedTranslation is deliberately left uninitialized.
}

void CAurBehaviorAttachTranslate::Message(char* pszParam) {
    // Handles the same two commands as the base's Message.
    if (_stricmp("diewithoutparent", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("diewithoutparent"), m_nDieWithoutParent);
    if (_stricmp("m_vTranslation", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_vTranslation"), m_vTranslation);
}

// Rotates a vector by a quaternion.
extern int quaternionidentityoptimization;   // the rotation identity fast-out

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

int CAurBehaviorAttachTranslate::Control(float fParam) {
    // The base does the attach/teardown work; this class only adds a translation offset to the
    // owner's position once the base says the attachment is still live.
    int nResult = CAurBehaviorAttach::Control(fParam);
    if (nResult == 0)
        return 0;

    // The offset is either the raw m_vTranslation, when m_bUnrotatedTranslation says it is already
    // expressed in world space, or that vector rotated by the owner's own orientation.
    CAurObject* pOwner = m_pOwner;
    int nUnrotated = m_bUnrotatedTranslation;
    Vector vPosition = pOwner->GetPosition();
    if (nUnrotated)
        pOwner->SetPosition(vPosition + m_vTranslation);
    else
        pOwner->SetPosition(vPosition + RotateVector(m_vTranslation, m_pOwner->GetOrientation()));
    return nResult;
}

int CAurBehaviorAttachRotate::Control(float fParam) {
    // Rotation goes on top of the orientation the owner already has, composed z, then x, then y.
    int nResult = CAurBehaviorAttach::Control(fParam);
    if (nResult == 0)
        return 0;
    CAurObject* pOwner = m_pOwner;
    pOwner->SetOrientation(
        pOwner->GetOrientation() * YawPitchRoll(m_vRotation.z, m_vRotation.x, m_vRotation.y));
    return nResult;
}

int CAurBehaviorAttachNoOrientation::Control(float fParam) {
    // Whatever the base did, the owner is put back to the orientation it had before the call. The
    // orientation is captured before the base call, since the base may move it, and restored only
    // when the base reported the attachment still live.
    Quaternion qOrientation = m_pOwner->GetOrientation();
    int nResult = CAurBehaviorAttach::Control(fParam);
    if (nResult != 0)
        m_pOwner->SetOrientation(qOrientation);
    return nResult;
}

void CAurBehaviorAttachRotate::Message(char* pszParam) {
    // Identical to Translate's, except the second keyword is "m_vRotation" and it lands in this
    // class's own Vector.
    if (_stricmp("diewithoutparent", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("diewithoutparent"), m_nDieWithoutParent);
    if (_stricmp("m_vRotation", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_vRotation"), m_vRotation);
}

CAurBehaviorAttachRotate::CAurBehaviorAttachRotate(CAurObject* pObj, CAurObject* pAttachTo,
                                                   CAurPart* pPart)
    : CAurBehaviorAttach(pObj, pAttachTo, pPart) {
    // Same implicit Vector() zeroing of m_vRotation; nothing else is written.
}

CAurBehaviorAttachNoOrientation::CAurBehaviorAttachNoOrientation(CAurObject* pObj,
                                                                 CAurObject* pAttachTo,
                                                                 CAurPart* pPart)
    : CAurBehaviorAttach(pObj, pAttachTo, pPart) {
}


void CAurBehavior::Message(char* pszParam) {
    K2_UNUSED(pszParam);
    // The base class reacts to no message at all.
}

int CAurBehavior::Control(float fParam) {
    // The owner is watched for the duration of the call. A dependency's Animate can destroy it, and
    // this SafePointer keeps the check at the end honest and un-registers on every exit path.
    SafePointer<CAurObject> spOwner(m_pOwner);

    // Dependencies are walked backwards, so a dead one can be dropped in place (DelIndex compacts
    // the array the same direction the loop is walking). A live one gets the frame's Animate.
    for (int i = m_dependencies.m_count - 1; i >= 0; i--) {
        SafePointer<CAurObject>* pNode = m_dependencies.m_pData[i];
        if (pNode->m_pObject != 0)
            pNode->m_pObject->Animate(fParam);
        else {
            delete pNode;
            m_dependencies.DelIndex(i);
        }
    }

    // Only a behavior that is still the owner's active one (id -1 = "the current behavior")
    // counts as handled, re-read through the SafePointer after the Animate calls above.
    if (spOwner.m_pObject == 0)
        return 0;
    return spOwner.m_pObject->GetBehavior(-1) == this;
}

void CAurBehavior::AddDependency(CAurObject* pObject) {
    // The node registers itself on the pointee's SafePointerList from SafePointer's own constructor.
    SafePointer<CAurObject>* pNode = new SafePointer<CAurObject>(pObject);
    m_dependencies.Add(pNode);
}

void CAurBehavior::RemoveDependency(CAurObject* pObject) {
    for (int i = 0; i < m_dependencies.m_count; i++) {
        SafePointer<CAurObject>* pNode = m_dependencies.m_pData[i];
        if (*pNode == pObject) {
            delete pNode;                 // ~SafePointer un-registers it from the pointee
            m_dependencies.DelIndex(i);
            return;
        }
    }
}

CAurBehavior::~CAurBehavior() {
    int nCount = m_dependencies.m_count;
    for (int i = 0; i < nCount; i++)
        delete m_dependencies.m_pData[i];
    m_dependencies.SetSize(0);
}

// CAurBehaviorAttach shares this TU with CAurBehavior so that the base constructor's body is
// visible to every derived constructor.


char* firstword(char* pszLine);
void Parse(char* pszLine, int& nValue);

void CAurBehaviorAttach::SetScene() {
    // SetScene and the scene pointer are Gob's: both objects are reached through their Gob.
    if (m_pAttachedTo)
        static_cast<Gob*>(m_pOwner)->SetScene(static_cast<Gob*>(m_pAttachedTo.m_pObject)->m_pScene);
}

void CAurBehaviorAttach::UpdateGobPosition() {
    // Pull the attached part's world transform into the owner's own position and orientation, then
    // let the owner react. The owner is held in a local across the GetWorld call.
    Gob* pOwner = static_cast<Gob*>(m_pOwner);
    m_pAttachedPart->GetWorld(pOwner->m_vPosition, pOwner->m_qOrientation);
    pOwner->DoEvent(const_cast<char*>("UpdateDependentPosition"), 0);
}

void CAurBehaviorAttach::Message(char* pszParam) {
    // A one-command behavior, where only "diewithoutparent" is recognised.
    if (_stricmp("diewithoutparent", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("diewithoutparent"), m_nDieWithoutParent);
}

int CAurBehaviorAttach::Control(float fParam) {
    // Per-frame current-scene hook. Unlike CAurBehaviorCameraFollow::Control, which discards the
    // result and always calls the base, this class gates the base update on it.
    bool bAnimating = true;
    if (currentscene != 0)
        bAnimating = currentscene->IsAnimating();

    // A SafePointer local, not just the raw member, since it registers itself on the owner's
    // watcher list and un-registers on every exit path.
    CAurObject* pOwner = m_pOwner;
    SafePointer<CAurObject> spOwner(pOwner);
    Gob* pGobOwner = static_cast<Gob*>(pOwner);

    if (bAnimating) {
        if (CAurBehavior::Control(fParam) == 0)
            return 0;
    }

    if (spOwner.m_pObject->GetBehavior(-1) != this)
        return 1;

    if (m_pAttachedTo.m_pObject == 0) {
        if (m_nDieWithoutParent != 0) {
            // Re-read from the member, not from the cached local.
            if (m_pOwner != 0)
                delete m_pOwner;
        } else {
            pGobOwner->SetBehavior(0);
        }
        return 1;
    }

    m_pAttachedPart->GetWorld(pGobOwner->m_vPosition, pGobOwner->m_qOrientation);
    return 1;
}

CAurBehaviorAttach::~CAurBehaviorAttach() {
    // The attached object is cached in a local before the guard.
    CAurObject* pAttached = m_pAttachedTo;

    // Only when there is an attachment.
    if (pAttached != 0) {
        // Detach from the owner unless the owner's current behavior is still this attachment and
        // still aimed at the same object, in which case the owner tears it down itself.
        if (m_pOwner != 0) {
            CAurBehavior* pOwnerBehavior = static_cast<Gob*>(m_pOwner)->m_pBehavior;
            if (pOwnerBehavior == 0) {
                static_cast<Gob*>(pAttached)->RemoveAttachment(static_cast<Gob*>(m_pOwner));
            } else {
                CAurBehaviorAttach* pOwnerAttach = static_cast<CAurBehaviorAttach*>(pOwnerBehavior->AsBehaviorAttach());
                if (pOwnerAttach == 0 || pOwnerAttach->m_pAttachedTo.m_pObject != m_pAttachedTo.m_pObject)
                    static_cast<Gob*>(pAttached)->RemoveAttachment(static_cast<Gob*>(m_pOwner));
            }
        }
        pAttached->DeregisterCallback(m_pUpdateScenesCallBack);
        pAttached->DeregisterCallback(m_pUpdatePositionCallBack);
        pAttached->DeregisterCallback(m_pColorShiftCallBack);
        pAttached->DeregisterCallback(m_pIlluminationCallBack);
        pAttached->DeregisterCallback(m_pRenderCallBack);
        pAttached->DeregisterCallback(m_pViewVolumeCallBack);
    }
}

// The six per-part callbacks the constructor registers on the attached object. The callbacks
// share this TU with CAurBehaviorAttach. Every one of them spells its event test
// `_stricmp(pszEvent, "...")`, the opposite argument order to Message above. UpdateScenesAttach
// and UpdateDependentPosition never read their first parameter; the four Propagate* ones read the
// color/illumination state off the object that raised the event and push it onto the registered
// context, which for those four is the owner Gob.

void UpdateScenesAttach(CAurObject*, char* pszEvent, void* pData) {
    CAurBehaviorAttach* pAttach = static_cast<CAurBehaviorAttach*>(pData);
    if (_stricmp(pszEvent, "UpdateScenes") == 0) {
        pAttach->SetScene();
#ifdef K2_FIX_ENGINE_BUGS
    } else if (_stricmp(pszEvent, "Dying") == 0 && pAttach->m_nDieWithoutParent != 0) {
#else
    } else if (pAttach->m_nDieWithoutParent != 0 && _stricmp(pszEvent, "Dying") == 0) {
#endif
        // The delete is the last thing either arm does.
        if (pAttach->m_pOwner != 0)
            delete pAttach->m_pOwner;
    }
}

void UpdateDependentPosition(CAurObject*, char* pszEvent, void* pData) {
    // Re-enters the behavior through its virtual UpdateGobPosition slot.
    if (_stricmp(pszEvent, "UpdateDependentPosition") == 0)
        static_cast<CAurBehaviorAttach*>(pData)->UpdateGobPosition();
}

void PropagateColorShifting(CAurObject* pObject, char* pszEvent, void* pData) {
    Gob* pGobObject = static_cast<Gob*>(pObject);
    if (_stricmp(pszEvent, "propagatecolorshift") == 0)
        static_cast<Gob*>(pData)->SetColorShifting(pGobObject->m_vColorShift,
                                        pGobObject->m_fColorShiftAmount, 1);
}

void PropagateIllumination(CAurObject* pObject, char* pszEvent, void* pData) {
    Gob* pGobObject = static_cast<Gob*>(pObject);
    if (_stricmp(pszEvent, "propagateillumination") == 0)
        static_cast<Gob*>(pData)->SetIllumination(pGobObject->m_vIllumination, 1);
}

void PropagateRender(CAurObject*, char* pszEvent, void* pData) {
    Gob* pGob = static_cast<Gob*>(pData);
    if (_stricmp(pszEvent, "propagaterender") == 0) {
        // The live behavior gets a Control(0) pass, and then the owner renders, not the object
        // that raised the event; the first parameter is unused here.
        if (pGob->m_pBehavior != 0)
            pGob->m_pBehavior->Control(0.0f);
        pGob->Render(true);
    }
}

void PropagateViewVolume(CAurObject* pObject, char* pszEvent, void* pData) {
    if (_stricmp(pszEvent, "propagateviewvolume") == 0) {
        // Asymmetric on purpose, the test is made on the raising object and the call goes to the
        // owner. IsInViewVolume's bool return is what gates it.
        if (static_cast<Gob*>(pObject)->IsInViewVolume())
            static_cast<Gob*>(pData)->EnableInViewVolume();
    }
}

CAurBehaviorAttach::CAurBehaviorAttach(CAurObject* pObject, CAurObject* pAttachTo, CAurPart* pPart)
    : CAurBehavior(pObject)
    , m_pAttachedTo()
{
    // The member is zeroed, registered as a dependency, and only then assigned, so the pointee's
    // SafePointerList sees the registration before the assignment.
    AddDependency(pAttachTo);
    m_pAttachedTo = pAttachTo;
    m_pAttachedPart = pPart;

    m_nReserved1 = 0;
    m_nDieWithoutParent = 0;
    m_pUpdateScenesCallBack = 0;
    m_pUpdatePositionCallBack = 0;
    m_pColorShiftCallBack = 0;
    m_pIlluminationCallBack = 0;
    m_pRenderCallBack = 0;
    m_pViewVolumeCallBack = 0;

    // The owner is held in a local for this whole block, and the attached object in another.
    Gob* pOwner = static_cast<Gob*>(m_pOwner);
    pOwner->SetScene(0);
    Gob* pAttached = static_cast<Gob*>(m_pAttachedTo.m_pObject);
    pOwner->SetScene(pAttached->m_pScene);

    // Take the attached part's current world transform for the owner, then let the owner react to
    // both the re-parent and the resulting transform.
    CAurPart* pWorldPart = m_pAttachedPart;
    pWorldPart->GetWorld(pOwner->m_vPosition, pOwner->m_qOrientation);
    pOwner->DoEvent(const_cast<char*>("UpdateDependentPosition"), 0);
#if !KOTOR1  // K1 sends no `_EmitterTarget` event
    pOwner->DoEvent(const_cast<char*>("_EmitterTarget"), 0);
#endif

    pAttached->AddAttachment(pOwner);

    // Carry an existing non-trivial scale over from the attached object, expressed against the
    // owner's own base scale (which is what SetObjectScale measures against).
    if (pAttached->m_fObjectScale != pOwner->m_fObjectScale / pOwner->m_fObjectScaleBase)
        pOwner->SetObjectScale(pAttached->m_fObjectScale * pOwner->m_fObjectScaleBase, false);

    // The first two callbacks are handed `this`, since they drive the behavior's own state, and
    // the last four the owner Gob, since they push state onto it.
    m_pUpdateScenesCallBack = m_pAttachedTo.m_pObject->RegisterCallBack(
        UpdateScenesAttach, const_cast<char*>("*"), this, 9999.0f, false);
    m_pUpdatePositionCallBack = m_pAttachedTo.m_pObject->RegisterCallBack(
        UpdateDependentPosition, const_cast<char*>("UpdateDependentPosition"), this, 9999.0f, false);
    m_pColorShiftCallBack = m_pAttachedTo.m_pObject->RegisterCallBack(
        PropagateColorShifting, const_cast<char*>("propagatecolorshift"), m_pOwner, 9999.0f, false);
    m_pIlluminationCallBack = m_pAttachedTo.m_pObject->RegisterCallBack(
        PropagateIllumination, const_cast<char*>("propagateillumination"), m_pOwner, 9999.0f, false);
    m_pRenderCallBack = m_pAttachedTo.m_pObject->RegisterCallBack(
        PropagateRender, const_cast<char*>("propagaterender"), m_pOwner, 9999.0f, false);
    m_pViewVolumeCallBack = m_pAttachedTo.m_pObject->RegisterCallBack(
        PropagateViewVolume, const_cast<char*>("propagateviewvolume"), m_pOwner, 9999.0f, false);
}

// CAurBehaviorNavigate.

CAurBehaviorNavigate::CAurBehaviorNavigate(CAurObject* pOwner) : CAurBehavior(pOwner) {
    m_fTurn = 0.0f;
    m_fTilt = 0.0f;
    m_fSpeed = 0.0f;
    m_fRaise = 0.0f;
    m_fStrafe = 0.0f;
}

void CAurBehaviorNavigate::Message(char* pszParam) {
    // Five independent tests, each re-parsing `firstword(pszParam)`: the console hands over a whole
    // line and every command is matched against its own first word.
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
}

int CAurBehaviorNavigate::Control(float fParam) {
    // In the per-frame current-scene hook, the base only runs when there is no scene at all, or the
    // scene reports itself animating.
    if (currentscene == 0 || currentscene->IsAnimating()) {
        if (CAurBehavior::Control(fParam) == 0)
            return 0;
    }

    // The owner's heading and elevation, read off its orientation quaternion.
    float fYaw = Yaw(m_pOwner->GetOrientation());
    float fPitch = Pitch(m_pOwner->GetOrientation());

    // Integrate this frame's turn and tilt velocities, clamping the pitch so the camera never looks
    // below the horizon.
    fYaw += m_fTurn * fParam;
    fPitch = max(0.0f, fPitch + m_fTilt * fParam);
    m_fTurn = 0.0f;
    m_fTilt = 0.0f;

    // The heading direction is (-sin, cos, 0), "left" of the yaw, which is why the x lane lands
    // as a subtract against the owner's position.
    if (m_fSpeed != 0.0f) {
        Vector vForward(-sinf(DegToRad(fYaw)), cosf(DegToRad(fYaw)), 0.0f);
        CAurObject* pOwner = m_pOwner;
        pOwner->SetPosition(pOwner->GetPosition() + vForward * m_fSpeed * fParam);
    }
    m_fSpeed = 0.0f;

    // Strafe is the heading direction crossed with the world up vector. The z lane keeps its 0*x
    // products rather than collapsing to a literal 0, because a folded product would not propagate
    // NaN.
    if (m_fStrafe != 0.0f) {
        Vector vForward(-sinf(DegToRad(fYaw)), cosf(DegToRad(fYaw)), 0.0f);
        Vector vStrafe = cross(vForward, Vector(0.0f, 0.0f, 1.0f));
        CAurObject* pOwner = m_pOwner;
        pOwner->SetPosition(pOwner->GetPosition() + vStrafe * m_fStrafe * fParam);
    }
    m_fStrafe = 0.0f;

    // Raise is a plain world-space Z step, added on top of whatever the two moves above did, and
    // not rotated by the heading.
    Vector vPosition = m_pOwner->GetPosition();
    vPosition.z += m_fRaise * fParam;
    m_pOwner->SetPosition(vPosition);
    m_fRaise = 0.0f;

    // Re-orient on the owner's existing roll.
    CAurObject* pOwner = m_pOwner;
    pOwner->SetOrientation(YawPitchRoll(fYaw, fPitch, Roll(pOwner->GetOrientation())));
    return 1;
}

// CAurBehaviorCameraOnAStick.
CAurBehaviorCameraOnAStick::CAurBehaviorCameraOnAStick(CAurObject* pCamera, CAurObject* pTarget)
    : CAurBehavior(pCamera) {
    Quaternion q;
    AddDependency(pTarget);
    m_pTarget = pTarget;

    // m_fMaxDist holds the float 100.0f as its bit pattern.
    m_fDist = 2.5f;
    m_fSpeed = 20.0f;
    m_bOrientWithTarget = 0;
    m_vDir = Vector(0.0f, 0.0f, 0.0f);
    m_fPitch = 70.0f;
    m_fHeight = 4.0f;
    m_fMaxDist = 0x42C80000;

    q = YawPitchRoll(0.0f, 70.0f, 0.0f);
    m_fYaw = 0;
    m_pOwner->SetOrientation(q);

    // The camera parks m_fDist behind the target along the orientation's local +Z. The rotated Z
    // result is discarded: the camera's height is m_fHeight.
    Vector vTarget = m_pTarget.m_pObject->GetPosition();
    Vector vZ = q.zdir();
    Vector vNew = vTarget + vZ * m_fDist;
    vNew.z = m_fHeight;
    m_pOwner->SetPosition(vNew);
    m_vPos = vNew;
}

CAurBehaviorCameraOnAStick::~CAurBehaviorCameraOnAStick() {
    // The destructor only releases m_pTarget; everything else in the class is POD.
}

void CAurBehaviorCameraOnAStick::Message(char* pszParam) {
    // Like CAurBehaviorNavigate::Message, but five of the six parse a float; "m_bOrientWithTarget"
    // goes to the int overload of Parse, which is why the member is an int and not a bool.
    if (_stricmp("m_fDist", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_fDist"), m_fDist);
    if (_stricmp("m_fSpeed", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_fSpeed"), m_fSpeed);
    if (_stricmp("m_fPitch", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_fPitch"), m_fPitch);
    if (_stricmp("m_fHeight", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_fHeight"), m_fHeight);
    if (_stricmp("m_bOrientWithTarget", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_bOrientWithTarget"), m_bOrientWithTarget);
    if (_stricmp("m_vDir", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("m_vDir"), m_vDir);
}

int CAurBehaviorCameraOnAStick::Control(float fParam) {
    // The same per-frame current-scene hook Navigate::Control makes, but here the result is
    // discarded.
    if (currentscene != 0)
        currentscene->IsAnimating();

    // Unlike Navigate::Control, whose early exit returns the base's own result, this one returns 1
    // when the base reports failure.
    if (CAurBehavior::Control(fParam) == 0)
        return 1;

    if (m_pTarget.m_pObject == 0) {
        m_pOwner->SetBehavior(0);
        return 1;
    }

    Vector vCam = m_pOwner->GetPosition();
    Vector vTarget = m_pTarget.m_pObject->GetPosition();
    Vector vDiff(vTarget.x - vCam.x, vTarget.y - vCam.y, 0.0f);

    // The extra sqrtf is the engine's, not a slip, since `magnitude` already ends in a square root
    // and the result is rooted again. Only the first of the three magnitude calls here gets that
    // extra step; the two inside the orientation test do not.
    float fDist = sqrtf(magnitude(vDiff));
    float fStep = m_fSpeed * ((fDist - m_fDist) / m_fDist * fParam);

    // z is m_fHeight for both arms; only the orientation test can rewrite x/y.
    Vector vNew(vCam.x + vDiff.x / fDist * fStep, vCam.y + vDiff.y / fDist * fStep, m_fHeight);

    if (m_bOrientWithTarget != 0 && magnitude(m_vDir) > 0.0f) {
        // The look direction is recomputed from the updated position, and the camera is then pulled
        // back along m_vDir by the distance to the target it would have had.
        Vector vLook(vTarget.x - vNew.x, vTarget.y - vNew.y, 0.0f);
        float fMag = magnitude(vLook);
        vNew = Vector(vTarget.x - fMag * m_vDir.x, vTarget.y - fMag * m_vDir.y, m_fHeight);
        m_pOwner->SetOrientation(YawPitchRoll(Yaw(normalize(m_vDir)), m_fPitch, 0.0f));
    } else {
        m_pOwner->SetOrientation(YawPitchRoll(Yaw(normalize(vDiff)), m_fPitch, 0.0f));
    }
    m_pOwner->SetPosition(vNew);
    return 1;
}

CAurBehaviorCameraFollow::CAurBehaviorCameraFollow(CAurObject* pCam, CAurObject* pTarget)
    : CAurBehavior(pCam) {
    AddDependency(pTarget);
    m_pTarget = pTarget;
    m_vFollowPosition = pTarget->GetPosition();

    m_vOffset = Vector(0.0f, 0.0f, 1.0f);
    m_fDist = 8.0f;
    m_fZoomDeltaAccum = 0.0f;
    m_fFOV = 1.0f;
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

void CAurBehaviorCameraFollow::Message(char* pszParam) {
    // Seventeen independent `_stricmp(literal, firstword(param))` tests, nothing here is else-if.
    if (_stricmp("turn", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("turn"), m_fYawVel);
    if (_stricmp("spring", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("spring"), m_fFOV);
    if (_stricmp("dist", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("dist"), m_fDist);
    if (_stricmp("offset", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("offset"), m_vOffset);      // the only Vector arm
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
        Parse(pszParam + strlen("clamppitch"), m_fPitchClipGate);     // the pitch clip gate
    if (_stricmp("clampdist", firstword(pszParam)) == 0)
        Parse(pszParam + strlen("clampdist"), m_fDistClipGate);      // the distance clip gate
}

CAurBehaviorCameraFollow::~CAurBehaviorCameraFollow() {
    // The destructor only releases m_pTarget.
}

// CAurBehaviorCameraFollow, the last of the family in this file.

Vector CAurBehaviorCameraFollow::GetTargetWorldPosition() {
    return static_cast<Gob*>(m_pTarget.m_pObject)->m_vPosition;
}

int CAurBehaviorCameraFollow::Control(float fParam) {
    // The per-frame current-scene hook; its result is discarded.
    if (currentscene != 0)
        currentscene->IsAnimating();

    // Overwritten with the YawPitchRoll result below. Quaternion's members are (w, x, y, z).
    Quaternion qGoal;

    if (CAurBehavior::Control(fParam) == 0)
        return 1;

    CAurObject* pTarget = m_pTarget;
    if (pTarget == 0) {
        m_pOwner->SetBehavior(0);
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

    Vector vTargetPos = pTarget->GetPosition();
    Vector vPos = Vector{
        (vTargetPos.x + m_vOffset.x) * fBlend + m_vFollowPosition.x * (1.0f - fBlend),
        (vTargetPos.y + m_vOffset.y) * fBlend + m_vFollowPosition.y * (1.0f - fBlend),
        (vTargetPos.z + m_vOffset.z) * fBlend + m_vFollowPosition.z * (1.0f - fBlend)
    };

    qGoal = YawPitchRoll(m_fYaw, m_fPitch, 0.0f);
    // One owner read per call pair. A single owner value stays live across
    // GetOrientation->SetOrientation, and the member is re-read for GetOrientation->SetPosition.
    // Four direct reads of the member do not work either: each of the virtual calls may write it,
    // so the reads cannot be merged.
    CAurObject* pOwner = m_pOwner;
    Quaternion qCam = pOwner->GetOrientation();
    qCam = normalize(slerp(qCam, qGoal, fBlend));
    pOwner->SetOrientation(qCam);

    pOwner = m_pOwner;
    qCam = pOwner->GetOrientation();
    Vector vDir = qCam.zdir();

    pOwner->SetPosition(Vector{
        vPos.x + vDir.x * m_fDist,
        vPos.y + vDir.y * m_fDist,
        vPos.z + vDir.z * m_fDist
    });

    m_vFollowPosition = vPos;
    return 1;
}

char* navigate(char* pszGobName) {
    // The third member of the nullscene/setscene family of console commands puts a fresh
    // CAurBehaviorNavigate on a gob and re-sets the gob's own scene.
    Gob* pGob = FindGob(pszGobName);
    if (pGob == 0)
        return const_cast<char*>("hey, no Gob by that name");
    // Read before the new-expression.
    Scene* pScene = pGob->m_pScene;
    pGob->SetBehavior(new CAurBehaviorNavigate(static_cast<CAurObject*>(pGob)));
    // Gob::SetScene is not virtual, so this stays a direct call.
    pGob->SetScene(pScene);
    return const_cast<char*>("OK");
}

// CAurBehaviorCameraLooseFollow shares this TU with the classes above; its own file was folded in
// for the same reason CameraFollow's was.

CAurBehaviorCameraLooseFollow::CAurBehaviorCameraLooseFollow(CAurObject* pCamera, CAurObject* pTarget)
    : CAurBehavior(pCamera) {
    AddDependency(pTarget);
    m_pTarget = pTarget;
    m_pCamera = reinterpret_cast<CAurCamera*>(pCamera);
    m_vFollowPosition = pTarget->GetPosition();

    m_vOffset = Vector(0.0f, 0.0f, 1.0f);
    m_fDist = 8.0f;
    m_fZoomDeltaAccum = 0.0f;
    m_fSpring = 750.0f;
    m_fYaw = 0.0f;
    m_fYawVel = 0.0f;
    // This seed's assignment order matters, the same way it does in the camera siblings: the
    // 360.0f store has to land before the store that ends the GetPosition copy above.
    m_fYawClipMax = 360.0f;
    m_fPitch = 70.0f;
    m_fPitchVel = 0.0f;
    m_fYawClipGate = 0.0f;
    m_fYawClipMin = 0.0f;
    m_fPitchClipGate = 1.0f;
    m_fPitchClipMin = 45.0f;
    m_fPitchClipMax = 70.0f;
    m_fDistClipGate = 1.0f;
    m_fDistClipMin = 6.0f;
    m_fDistClipMax = 9.0f;
    m_fViewPlaneMargin = 1.5f;
    m_fReserved1 = 4.0f;
    m_fReserved2 = 1.0f;
    m_bRestartMove = 0;
    m_bCenterOnTarget = 1;
    m_bMovingToGoal = 0;
}

CAurBehaviorCameraLooseFollow::~CAurBehaviorCameraLooseFollow() {
    // The destructor only releases m_pTarget.
}

void CAurBehaviorCameraLooseFollow::Message(char* pszParam) {
    K2_UNUSED(pszParam);
    // This behavior takes no console commands at all, unlike its siblings.
}

int CAurBehaviorCameraLooseFollow::Control(float fParam) {
    if (currentscene != 0)
        currentscene->IsAnimating();

    // Overwritten with the YawPitchRoll result further down; the identity quaternion is
    // materialised before the base call.
    Quaternion qNew;

    if (CAurBehavior::Control(fParam) == 0)
        return 1;

    if (m_pTarget.m_pObject == 0) {
        m_pOwner->SetBehavior(0);
        return 1;
    }

    // The whole body is skipped (not early-returned) when the camera is absent or in a view-volume
    // set-up pass, or when a re-centering pass is already in flight.
    if (m_pCamera != 0 && m_pCamera->ViewVolumeSet() != 0 &&
        m_bCenterOnTarget == 0 && m_bMovingToGoal == 0) {
        Vector vTarget = m_pTarget.m_pObject->GetPosition();
        Vector vOwner = m_pOwner->GetPosition();
        Quaternion qOwner = m_pOwner->GetOrientation();

        // The owner orientation's local +Z axis, negated and projected to XY. It is the axis the
        // dead-zone correction below slides the owner along.
        Vector vDir = -qOwner.zdir();
        vDir.z = 0.0f;
        vDir.Normalize();

        Vector vDiff(vTarget.x - vOwner.x, vTarget.y - vOwner.y, 0.0f);
        Vector vDelta(vDiff);
        vDelta.Normalize();

        float fDot = vDelta.x * vDir.x + vDelta.y * vDir.y + vDelta.z * vDir.z;
        float fCross = vDir.x * vDelta.y - vDelta.x * vDir.y;
        // The projection is taken from the unnormalized difference, not from vDelta, and its z term
        // is a known zero.
        float fProj = vDiff.x * vDir.x + vDiff.y * vDir.y + vDir.z * vDiff.z;

        float fAngle = acosf(fDot);

        // A discarded debug format; see FormatDebugFloats. The divisor is a float PI constant
        // widened into the double expression.
        char szDebug[256];
        FormatDebugFloats(szDebug, static_cast<double>(fDot), fAngle * 180.0 / static_cast<double>(3.1415927f));

        float fDeg = fAngle * 180.0f / 3.1415927f;

        // In the dead zone, pull the owner toward the target when it lags, push it away when it
        // leads. The two arms are written separately and their identical tails merge into one
        // block, which is why the sign fix and the m_fYawVel store appear only once; a projection
        // inside the dead zone skips them and keeps the previous m_fYawVel.
        if (fProj > m_fDistClipMax) {
            Vector vPos = m_pOwner->GetPosition();
            m_pOwner->SetPosition(Vector(vPos.x + vDir.x * (fProj - m_fDistClipMax),
                                         vPos.y + vDir.y * (fProj - m_fDistClipMax),
                                         vPos.z + vDir.z * (fProj - m_fDistClipMax)));
            if (fCross < 0.0f)
                fDeg = -fDeg;
            m_fYawVel = fDeg;
        } else if (fProj < m_fDistClipMin) {
            Vector vPos = m_pOwner->GetPosition();
            m_pOwner->SetPosition(Vector(vPos.x - vDir.x * (m_fDistClipMin - fProj),
                                         vPos.y - vDir.y * (m_fDistClipMin - fProj),
                                         vPos.z - vDir.z * (m_fDistClipMin - fProj)));
            if (fCross < 0.0f)
                fDeg = -fDeg;
            m_fYawVel = fDeg;
        }

        // View-volume 3 / 2 rejection tests the target's position relative to the camera plane. The
        // 4-lane result is the engine's Plane (our CAurCamera.h types it Quaternion, same four
        // floats), so the lanes are (a,b,c,d) in the quaternion's (w,x,y,z) memory order.
        Plane qPlane3 = m_pCamera->GetViewVolume(3);
        Vector vTarget3 = m_pTarget.m_pObject->GetPosition();
        if (qPlane3.a * vTarget3.x + qPlane3.b * vTarget3.y +
            qPlane3.c * vTarget3.z + qPlane3.d + m_fViewPlaneMargin > 0.0f)
            m_fYawVel = -30.0f;

        Plane qPlane2 = m_pCamera->GetViewVolume(2);
        Vector vTarget2 = m_pTarget.m_pObject->GetPosition();
        if (qPlane2.a * vTarget2.x + qPlane2.b * vTarget2.y +
            qPlane2.c * vTarget2.z + qPlane2.d + m_fViewPlaneMargin > 0.0f)
            m_fYawVel = 30.0f;
    }

    // Per-frame integration of the accumulated turn rate and of the distance.
    m_fYaw += m_fYawVel * fParam;
    m_fYawVel = 0.0f;

    if (m_fYawClipGate != 0.0f)
#if KOTOR1  // K1 clamps with fminf/fmaxf, so a NaN argument takes the bound
        m_fYaw = fmaxf(m_fYawClipMin, fminf(m_fYaw, m_fYawClipMax));
#else
        m_fYaw = max(m_fYawClipMin, min(m_fYaw, m_fYawClipMax));
#endif

    m_fDist *= exp2f(m_fZoomDeltaAccum * fParam * -0.5f);
    m_fZoomDeltaAccum = 0.0f;

    if (m_fDistClipGate != 0.0f)
#if KOTOR1  // K1 clamps with fminf/fmaxf
        m_fDist = fmaxf(m_fDistClipMin, fminf(m_fDist, m_fDistClipMax));
#else
        m_fDist = max(m_fDistClipMin, min(m_fDist, m_fDistClipMax));
#endif

    float fBlend = 1.0f - exp2f(-(m_fSpring * fParam));

    qNew = YawPitchRoll(m_fYaw, m_fPitch, 0.0f);
    m_pOwner->SetOrientation(normalize(slerp(m_pOwner->GetOrientation(), qNew, fBlend)));

    if (m_bRestartMove != 0) {
        // Both results are dropped: this is a re-init pass that only flips the flags. The two
        // calls must stay, since they cannot be proven side-effect free.
        m_pOwner->GetPosition();
        m_pTarget.m_pObject->GetPosition();
        m_bMovingToGoal = 1;
        m_bRestartMove = 0;
    }

    if (m_bCenterOnTarget != 0) {
        // Snap the loose-follow offset onto the target's current position plus m_vOffset.
        Vector vTarget = m_pTarget.m_pObject->GetPosition();
        m_vGoalPosition = Vector(vTarget.x + m_vOffset.x, vTarget.y + m_vOffset.y,
                          vTarget.z + m_vOffset.z);
        m_bCenterOnTarget = 0;
        m_bMovingToGoal = 1;
    } else if (m_bMovingToGoal == 0) {
        return 1;
    }

    // Blend the previous frame's position toward the target's, then push it back out along the
    // owner's local +Z by the follow distance, as the on-a-stick ctor does.
    Vector vLerp = Vector(m_vFollowPosition.x * (1.0f - fBlend) + m_vGoalPosition.x * fBlend,
                          m_vFollowPosition.y * (1.0f - fBlend) + m_vGoalPosition.y * fBlend,
                          m_vFollowPosition.z * (1.0f - fBlend) + m_vGoalPosition.z * fBlend);

    Vector vZ = m_pOwner->GetOrientation().zdir();
    m_pOwner->SetPosition(Vector(vLerp.x + vZ.x * m_fDist,
                                 vLerp.y + vZ.y * m_fDist,
                                 vLerp.z + vZ.z * m_fDist));
    m_vFollowPosition = vLerp;

    Vector vZGoal = m_pOwner->GetOrientation().zdir();
    Vector vGoal = Vector(m_vGoalPosition.x + vZGoal.x * m_fDist,
                          m_vGoalPosition.y + vZGoal.y * m_fDist,
                          m_vGoalPosition.z + vZGoal.z * m_fDist);

    if (equal(vGoal, m_pOwner->GetPosition()) != 0) {
        m_bMovingToGoal = 0;
        m_fSpring = 5.0f;
    }
    return 1;
}

void CAurBehaviorCameraLooseFollow::CenterOnTarget() {
    // CenterOnTarget is triggered by the console command of the same name.
    m_bCenterOnTarget = 1;
}
