#if KOTOR1
#define K1_AURORA_LIST  // this TU is K1's AURORA library (List.h grows from 8)
#endif
#include "aurcamera.h"
#include "port/aslglapi.h"

#include "include/aurbehavior.h"
#include "AURORA/aurbehavior.h"
#include "Plane.h"
#include "aurscene.h"

int g_nOriginalScreenWidth = 800;
int g_nOriginalScreenHeight = 600;
int g_nScaledNativeWidth = 800;
int g_nScaledNativeHeight = 600;
#if !KOTOR1
float limitFPS = 60.0f;
#endif

extern int g_nDesiredFullScreenRefreshRate;
extern unsigned char g_nBitsPerPixel;
extern int g_bDo3DGUIs;

#include "port/libcbase.h"
#include "port/libcmath.h"
#include <stddef.h>

extern Gob* FindGob(char* pszName);
Camera* CurrentCamera;


extern "C" void gluPerspective(double fovy, double aspect, double nzNear, double nzFar);
float DegToRad(float fAngle);
Vector NormalOf(const Vector& a, const Vector& b, const Vector& c);
bool  IosAntiAliasOn();
void  IosAntiAliasScaleSize(int& w, int& h);
#if KOTOR1
#include "port/k1gui.h"
bool  SaturationThisFrameIos();
#endif
Vector planelineintersection(const Vector& v, float f, const Vector& a, const Vector& b);
Vector normalize(const Vector& v);
int equal(const Vector& a, const Vector& b);

int    g_nScreenWidth = 800;
int    g_nScreenHeight = 600;
char   s_frameBufferModificationsStarted;
int    overridecamera;
Vector cameraposition;
Quaternion cameraorientation;
#if KOTOR1
float  IOS_FOG_CLIPDIST = 10000.0f;
#endif
int    frustumpick;
Vector frustumn;
float  frustumd;
extern int quaternionidentityoptimization;

#include "aurglcore.h"

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
        float m00 = 1.0f - ((yy + zz) + (yy + zz));
        float m01 = (xy - wz) + (xy - wz);
        float m02 = (xz + wy) + (xz + wy);
        float m10 = (xy + wz) + (xy + wz);
        float m11 = 1.0f - ((xx + zz) + (xx + zz));
        float m12 = (yz - wx) + (yz - wx);
        float m20 = (xz - wy) + (xz - wy);
        float m21 = (yz + wx) + (yz + wx);
        float m22 = 1.0f - ((xx + yy) + (xx + yy));
        r.x = v.x * m00 + v.y * m01 + v.z * m02;
        r.y = v.x * m10 + v.y * m11 + v.z * m12;
        r.z = v.x * m20 + v.y * m21 + v.z * m22;
        return r;
    }
    return v;
}



struct CameraViewAnim {
    float fLength;
    float fElapsed;
    float fOldAngle;
    float fNewAngle;
    int   nType;
};

Camera::Camera(char* pszName)
    : CAurCamera(), Gob(pszName)
{
    __builtin_memset(static_cast<void*>(&m_viewState), 0, sizeof(m_viewState));
    m_fViewAngle = 45.0f;
    m_fClipFar = 100.0f;
    m_pViewAnimState = 0;
    m_fViewAnimDefault = 1.0f;
    m_fClipNear = 0.1f;
    m_nViewX = 0;
    m_nViewY = 0;
    m_nViewW = 0;
    m_nViewH = 0;
    m_viewState.m_aViewVolumes.SetSize(6);
}

Camera::~Camera()
{
}

int Camera::PlayAnimation(const char* a, float b, int c, float d)
{
    K2_UNUSED(a); K2_UNUSED(b); K2_UNUSED(c); K2_UNUSED(d);
    return 0;
}

int Camera::AttachToScene(CAurScene* pScene)
{
    SetScene(static_cast<Scene*>(pScene));
    return 1;
}

Vector Camera::SetPosition(Vector vPos)
{
    m_vPosition = vPos;
    return vPos;
}

void Camera::SetOrientation(Quaternion qOri)
{
    m_qOrientation = qOri;
}

void Camera::SetClipDist(float fMin, float fMax)
{
    m_fClipNear = fMin;
    m_fClipFar = fMax;
}

void Camera::SetViewAngle(float fAngle)
{
    m_fViewAngle = fAngle;
}

void Camera::SetViewPort(int nX, int nY, int nW, int nH)
{
    m_nViewX = nX;
    m_nViewY = nY;
    m_nViewW = (nW > 1) ? nW : 1;
    m_nViewH = (nH > 1) ? nH : 1;
}

// Builds the view transform and the six frustum planes, then renders the scene.
void Camera::RenderScene()
{
    if (m_pScene) {
        GLRender::SetPolygonRenderMode(Aurora::kFacingFrontAndBack, Aurora::kFillSolid);
        GLRender::SetMatrixMode(Aurora::kMatrixModelView2);
        ASLgl::glPushMatrix();
        GLRender::SetMatrixMode(Aurora::kMatrixProjection);
        ASLgl::glPushMatrix();

        int avwViewport[4];
        if (IosAntiAliasOn()) {
            GLRender::GetViewPort(avwViewport);
#if KOTOR1  // K1: full-screen test against the SDL mode only, no origin test, no latch
            if (avwViewport[2] == g_SDL_mode.w && avwViewport[3] == g_SDL_mode.h) {
                int nAAW = avwViewport[2];
                int nAAH = avwViewport[3];
#else
            if (static_cast<unsigned int>(avwViewport[0]) < 10u && static_cast<unsigned int>(avwViewport[1]) < 10u &&
                avwViewport[2] == g_nScreenWidth && avwViewport[3] == g_nScreenHeight) {
                int nAAW = avwViewport[2];
                s_frameBufferModificationsStarted = 1;
                int nAAH = avwViewport[3];
#endif
                IosAntiAliasScaleSize(nAAW, nAAH);
                ASLgl::glViewport(0, 0, nAAW, nAAH);
            }
        }
        GLRender::GetViewPort(avwViewport);

        int nViewW, nViewH;
        if (m_nViewW != 0 || m_nViewH != 0) {
            if (m_nViewX >= g_nScreenWidth || (m_nViewX + m_nViewW) < 0 ||
                m_nViewY >= g_nScreenHeight || (m_nViewY + m_nViewH) < 0) {
                m_pScene->m_nAnimateTick += 1;
                goto done;
            }
            ASLgl::glViewport(m_nViewX, m_nViewY, m_nViewW, m_nViewH);
            nViewW = m_nViewW;
            nViewH = m_nViewH;
        } else {
            nViewW = avwViewport[2];
            nViewH = avwViewport[3];
        }

        float fAspect = static_cast<float>(nViewW) / static_cast<float>(nViewH);
        m_fViewAnimDefault = static_cast<float>(nViewH) * 0.01f;

        GLRender::SetMatrixMode(Aurora::kMatrixProjection);
        ASLgl::glLoadIdentity();

#if KOTOR1 && defined(K2_MOBILE_OPT)  // K1's mobile port: the far clip follows the fog end, or 10000 on a saturation frame
        if (!IosAntiAliasOn()) {
            if (IOS_FOG_CLIPDIST > 0.0f)
                m_fClipFar = IOS_FOG_CLIPDIST;
            if (SaturationThisFrameIos())
                m_fClipFar = 10000.0f;
        }
#endif
        m_viewState.m_aViewVolumes[0] = Plane(0.0f, 0.0f, 1.0f, m_fClipNear);
        m_viewState.m_aViewVolumes[1] = Plane(0.0f, 0.0f, -1.0f, -m_fClipFar);

        if (m_fViewAngle > 0.0f) {
            gluPerspective(static_cast<double>(m_fViewAngle), static_cast<double>(fAspect), static_cast<double>(m_fClipNear),
                           static_cast<double>(m_fClipFar));

            float fLeft   = -fAspect * tanf(DegToRad(m_fViewAngle * 0.5f));
            float fRight  =  fAspect * tanf(DegToRad(m_fViewAngle * 0.5f));
            float fBottom = -tanf(DegToRad(m_fViewAngle * 0.5f));
            float fTop    =  tanf(DegToRad(m_fViewAngle * 0.5f));

            {
                Vector n = NormalOf(Vector(0.0f, 0.0f, 0.0f), Vector(fLeft, fTop, -1.0f),
                                    Vector(fLeft, fBottom, -1.0f));
                Plane& q = m_viewState.m_aViewVolumes[2];
                q.a = n.x; q.b = n.y; q.c = n.z; q.d = 0.0f;
            }
            {
                Vector n = NormalOf(Vector(0.0f, 0.0f, 0.0f), Vector(fRight, fBottom, -1.0f),
                                    Vector(fRight, fTop, -1.0f));
                Plane& q = m_viewState.m_aViewVolumes[3];
                q.a = n.x; q.b = n.y; q.c = n.z; q.d = 0.0f;
            }
            {
                Vector n = NormalOf(Vector(0.0f, 0.0f, 0.0f), Vector(fRight, fTop, -1.0f),
                                    Vector(fLeft, fTop, -1.0f));
                Plane& q = m_viewState.m_aViewVolumes[4];
                q.a = n.x; q.b = n.y; q.c = n.z; q.d = 0.0f;
            }
            {
                Vector n = NormalOf(Vector(0.0f, 0.0f, 0.0f), Vector(fLeft, fBottom, -1.0f),
                                    Vector(fRight, fBottom, -1.0f));
                Plane& q = m_viewState.m_aViewVolumes[5];
                q.a = n.x; q.b = n.y; q.c = n.z; q.d = 0.0f;
            }
        } else {
            ASLgl::glOrtho(0.0, static_cast<double>(fAspect * m_fViewAnimDefault), 0.0, static_cast<double>(m_fViewAnimDefault),
                           static_cast<double>(m_fClipNear), static_cast<double>(m_fClipFar));
            m_viewState.m_aViewVolumes[2] = Plane(-1.0f, 0.0f, 0.0f, 0.0f);
            m_viewState.m_aViewVolumes[3] = Plane(1.0f, 0.0f, 0.0f, -(fAspect * m_fViewAnimDefault));
            m_viewState.m_aViewVolumes[4] = Plane(0.0f, -1.0f, 0.0f, 0.0f);
            m_viewState.m_aViewVolumes[5] = Plane(0.0f, 1.0f, 0.0f, -m_fViewAnimDefault);
        }

        if (overridecamera)
            GLRender::SetViewTransform(cameraposition, cameraorientation);
        else
            GLRender::SetViewTransform(m_vPosition, m_qOrientation);

#if KOTOR1  // K1 keeps the plane loop rolled
#pragma clang loop unroll(disable)
#endif
        for (int iVolume = 0; iVolume < 6; iVolume++) {
            Plane& q = m_viewState.m_aViewVolumes[iVolume];
                        Vector r = RotateVector(Vector(q.a, q.b, q.c), m_qOrientation);
            q.a = r.x; q.b = r.y; q.c = r.z;
            q.d = q.d - dot(r, m_vPosition);
        }

        Plane& qPick = m_viewState.m_aViewVolumes[frustumpick];
        frustumn = Vector(qPick.a, qPick.b, qPick.c);
        frustumd = qPick.d;

        m_nViewVolumeSet = 1;
        CurrentCamera = this;
        GLRender::SetMatrixMode(Aurora::kMatrixModelView2);
        m_pScene->Render();
        GLRender::SetMatrixMode(Aurora::kMatrixProjection);
        ASLgl::glPopMatrix();
        GLRender::SetMatrixMode(Aurora::kMatrixModelView2);
        ASLgl::glPopMatrix();
        CurrentCamera = 0;

        if (m_nViewW != 0 || m_nViewH != 0)
            ASLgl::glViewport(avwViewport[0], avwViewport[1], avwViewport[2], avwViewport[3]);
    }
done:
    ;
}

// Direction of the mouse ray through the view rectangle.
Vector Camera::MouseDir(int nMouseX, int nMouseY)
{
    int nView[4];
    __builtin_memcpy(nView, &m_nViewX, sizeof(nView));
    if (nView[2] == 0 || nView[3] == 0)
        GLRender::GetViewPort(nView);

    int nDX = nMouseX - nView[0];
    int nDY = nMouseY - nView[1];
    float fTan = tanf(m_fViewAngle * 0.5f * 0.017453292f);
    float fW2 = static_cast<float>(nView[2]) * 0.5f;
    float fH2 = static_cast<float>(nView[3]) * 0.5f;
    Vector v = Vector(static_cast<float>(nDX), static_cast<float>(nDY), 0.0f);
    v.x -= fW2;
    v.y -= fH2;
    v.x *= fTan;
    v.y *= fTan;
    v.x /= fH2;
    v.y /= fH2;
    v.z = -1.0f;

    v = RotateVector(v, m_qOrientation);
    v.Normalize();
    return v;
}

// Projects a world-space point onto the view rectangle. With `b` set the result is normalised
// to the 0..1 range.

Vector Camera::MapToScreen(Vector v, bool b)
{
    int nView[4];
    __builtin_memcpy(nView, &m_nViewX, sizeof(nView));
    if (nView[2] == 0 || nView[3] == 0)
        GLRender::GetViewPort(nView);

    if (m_fViewAngle > 0.0f) {
        v = v - m_vPosition;

// Inverse rotation, spelled with a negated-w quaternion.
#if KOTOR1  // K1 calls the Quaternion inverse operator
        v = RotateVector(v, -m_qOrientation);
#else
        float w = m_qOrientation.w;
        v = RotateVector(v, Quaternion(-w, m_qOrientation.x, m_qOrientation.y, m_qOrientation.z));
#endif

        Vector ray = planelineintersection(Vector(0.0f, 0.0f, 1.0f), 1.0f, Vector(0.0f, 0.0f, 0.0f), v);
        float fRx = ray.x;
        float fRy = ray.y;
        float fTan = tanf(m_fViewAngle * 0.5f * 0.017453292f);
        float fH2 = static_cast<float>(nView[3]) * 0.5f;
        float fW2 = static_cast<float>(nView[2]) * 0.5f;
        float fXterm = fRx * fH2 / fTan;
        float fZterm = fRy * fH2 / fTan;
        float fX = static_cast<float>(nView[0]) + (fW2 + fXterm);
        float fY = static_cast<float>(nView[1]) + (fH2 + fZterm);
        if (b) {
            fX /= static_cast<float>(nView[2]);
            fY = 1.0f - fY / static_cast<float>(nView[3]);
        }
        return Vector(fX, fY, 0.0f);
    }
    return Vector(0.0f, 0.0f, 0.0f);
}

void Camera::BehaviorMessage(char* pszMsg)
{
    Gob::BehaviorMessage(pszMsg);
}

void* Camera::AttachToObject(CAurObject* pObj, char* psz, int n)
{
    return Gob::AttachToObject(pObj, psz, n);
}

void* Camera::AttachToPart(CAurObject* pObj, CAurPart* pPart, int n)
{
    return Gob::AttachToPart(pObj, pPart, n);
}

CAurBehavior* Camera::GetBehavior(int nBehaviorId)
{
    return Gob::GetBehavior(nBehaviorId);
}

void Camera::SetBehavior(CAurBehavior* pBehavior)
{
    Gob::SetBehavior(pBehavior);
}

Plane Camera::GetViewVolume(int nIndex)
{
    return m_viewState.m_aViewVolumes[nIndex];
}

void Camera::HitCheck(Vector a, Vector b, Vector* c)
{
    Gob::HitCheck(a, b, c, 0);
}

// Screen-space hit test: the perspective mode casts a mouse ray, the orthographic mode builds the
// segment from the clip planes.
int Camera::HitCheck(int nX, int nY, Vector* pResult, CHitInfo* pInfo)
{
    if (!m_pScene)
        return 0;
    if (m_fViewAngle > 0.0f) {
        Vector vDir = MouseDir(nX, nY);
        return m_pScene->HitCheck(m_vPosition + vDir * m_fClipNear,
                                  m_vPosition + vDir * m_fClipFar, pResult, pInfo);
    }
    int nView[4];
    __builtin_memcpy(nView, &m_nViewX, sizeof(nView));
    if (nView[2] == 0 || nView[3] == 0)
        GLRender::GetViewPort(nView);
    int nXRel = nX - nView[0];
    int nYRel = nY - nView[1];
    if (nXRel >= nView[2])
        return 0;
    if ((nXRel | nYRel) < 0)
        return 0;
    if (nYRel >= nView[3])
        return 0;
    float fScale = m_fViewAnimDefault;
// The fraction is built as a Vector so both lanes are computed together. 0.5 is the
// pixel-centre offset, not a rounding artefact.
    Vector vFrac = Vector(static_cast<float>(nXRel) + 0.5f, static_cast<float>(nYRel) + 0.5f, 0.0f);
    vFrac.x /= static_cast<float>(nView[3]);
    vFrac.y /= static_cast<float>(nView[3]);
    vFrac.x *= fScale;
    vFrac.y *= fScale;
    Vector vNear = m_vPosition + Vector(vFrac.x, vFrac.y, -m_fClipNear);
    Vector vFar  = m_vPosition + Vector(vFrac.x, vFrac.y, -m_fClipNear - m_fClipFar);
    return m_pScene->HitCheck(vNear, vFar, pResult, pInfo);
}

// Nearest plane-line intersection inside the frustum, else the far clip distance.
float Camera::HitCheckViewVolumeNearest(const Vector& vStart, const Vector& vEnd, Vector* pResult, int* pIndex)
{
    Vector res = Vector(0.0f, 0.0f, 0.0f);
    Vector vZero = Vector(0.0f, 0.0f, 0.0f);
    float fBest = m_fClipFar;
    if (pIndex)
        *pIndex = 0;
    for (int i = 2; i < 6; i++) {
        res = planelineintersection(*reinterpret_cast<const Vector*>(&m_viewState.m_aViewVolumes[i]),
                                    m_viewState.m_aViewVolumes[i].d, vStart, vEnd);
        if (equal(res, vZero))
            continue;
        bool bInside = true;
        for (int j = 0; j < 6; j++) {
            if (j != i &&
                res.x * m_viewState.m_aViewVolumes[j].a + res.y * m_viewState.m_aViewVolumes[j].b +
                    res.z * m_viewState.m_aViewVolumes[j].c + m_viewState.m_aViewVolumes[j].d > 0.0f) {
                bInside = false;
                break;
            }
        }
        if (!bInside)
            continue;
        res.x -= m_vPosition.x;
        res.y -= m_vPosition.y;
        res.z -= m_vPosition.z;
        float fDist = res.x * m_viewState.m_aViewVolumes[1].a + res.y * m_viewState.m_aViewVolumes[1].b +
                      res.z * m_viewState.m_aViewVolumes[1].c;
        if (fDist < fBest && fDist > 0.0f) {
            if (pResult)
                *pResult = res;
            if (pIndex)
                *pIndex = 1;
            fBest = fDist;
        }
    }
    return fBest;
}

// Casts the boundary between the ground plane and each frustum side plane, and reports the result
// furthest along cPlane. Returns false when no candidate is in front of cPlane.
bool Camera::SceneBoundaryCheck(const Vector& vStart, const Vector& vEnd, const Plane& cPlane, Vector* pResult)
{
    static const int knBoundaryPartner[4] = { 4, 5, 3, 2 };
    Plane cGround(0.0f, 0.0f, 1.0f, 0.0f);
    Vector aResults[4] = {};
    int nBest = 0;
    float fBest = 0.0f;
    for (int i = 0; i < 4; i++) {
        const Plane& qA = m_viewState.m_aViewVolumes[i + 2];
        const Plane& qB = m_viewState.m_aViewVolumes[knBoundaryPartner[i]];
        Vector vA = Vector(qA.a, qA.b, qA.c);
        Vector vB = Vector(qB.a, qB.b, qB.c);
        Vector vCross;
        vCross.x = vA.y * vB.z - vA.z * vB.y;
        vCross.y = vA.z * vB.x - vA.x * vB.z;
        vCross.z = vA.x * vB.y - vA.y * vB.x;
        Vector vRes = planelineintersection(*reinterpret_cast<const Vector*>(&cGround), cGround.d,
                                            m_vPosition, m_vPosition + vCross);
        aResults[i] = vRes;
        float fDist = vRes.x * cPlane.a + vRes.y * cPlane.b + vRes.z * cPlane.c + cPlane.d;
        if (fDist > fBest) {
            fBest = fDist;
            nBest = i;
        }
    }
    if (fBest == 0.0f)
        return false;

    Vector vRes = aResults[nBest];
    Vector vDiff;
    vDiff.x = m_vPosition.x - vRes.x;
    vDiff.y = m_vPosition.y - vRes.y;
    vDiff.z = m_vPosition.z - vRes.z;
    Vector vDelta;
    vDelta.x = vEnd.x - vStart.x;
    vDelta.y = vEnd.y - vStart.y;
    vDelta.z = vEnd.z - vStart.z;
    Vector vNorm = normalize(Vector(vDiff.y * vDelta.z - vDiff.z * vDelta.y,
                                    vDiff.z * vDelta.x - vDiff.x * vDelta.z,
                                    vDiff.x * vDelta.y - vDiff.y * vDelta.x));
    float fDot = dot(vNorm, vStart);
    Plane cPlane2(vNorm.x, vNorm.y, vNorm.z, -fDot);
    Vector vOut = planelineintersection(*reinterpret_cast<const Vector*>(&cPlane2), cPlane2.d,
                                        m_vPosition,
                                        m_vPosition + Vector(m_viewState.m_aViewVolumes[1].a,
                                                             m_viewState.m_aViewVolumes[1].b,
                                                             m_viewState.m_aViewVolumes[1].c));
    *pResult = vOut;
    return true;
}


void Camera::GetMinimumBoundingBox(Vector& a, Vector& b)
{
    K2_UNUSED(a); K2_UNUSED(b);
}


void Camera::Render(bool a)
{
    if (this != CurrentCamera && CurrentCamera)
        Gob::Render(a);
}

float Camera::GetYaw()
{
    if (m_pBehavior && m_pBehavior->AsBehaviorCameraFollow())
        return static_cast<CAurBehaviorCameraFollow*>(m_pBehavior)->m_fYaw;
    return 0.0f;
}

float Camera::GetPitch()
{
    if (m_pBehavior && m_pBehavior->AsBehaviorCameraFollow())
        return static_cast<CAurBehaviorCameraFollow*>(m_pBehavior)->m_fPitch;
    return 0.0f;
}

float Camera::GetDist()
{
    if (m_pBehavior && m_pBehavior->AsBehaviorCameraFollow())
        return static_cast<CAurBehaviorCameraFollow*>(m_pBehavior)->m_fDist;
    return 0.0f;
}

void Camera::SetFollow(CAurObject* pFollow)
{
    if (pFollow)
        SetBehavior(new CAurBehaviorCameraFollow(this, pFollow));
    else
        SetBehavior(0);
}

void Camera::BeginViewAngleAnimation(float fNewAngle, float fLength, int nType)
{
    if (m_pViewAnimState) {
        delete static_cast<CameraViewAnim*>(m_pViewAnimState);
        m_fViewAngle = 45.0f;
    }
    CameraViewAnim* p = new CameraViewAnim;
    m_pViewAnimState = p;
    p->fLength = fLength;
    p->fNewAngle = fNewAngle;
    p->fOldAngle = m_fViewAngle;
    p->nType = nType;
    p->fElapsed = 0.0f;
}

void Camera::EndViewAngleAnimation(bool bReset)
{
    if (m_pViewAnimState)
        delete static_cast<CameraViewAnim*>(m_pViewAnimState);
    m_pViewAnimState = 0;
    if (bReset)
        m_fViewAngle = 45.0f;
}

void Camera::Animate(float fDelta)
{
    if (m_pViewAnimState) {
        CameraViewAnim* p = static_cast<CameraViewAnim*>(m_pViewAnimState);
        p->fElapsed += fDelta;
        if (p->fElapsed > p->fLength) {
            delete p;
            m_pViewAnimState = 0;
        } else {
            float t = p->fElapsed / p->fLength;
            // case 3 falls through into case 0
            switch (p->nType) {
            case 1: {   // cosine smoothstep
                float c = cosf(t * 3.14159265f);
                float w = (c + 1.0f) * -0.5f + 1.0f;
                m_fViewAngle = (1.0f - w) * p->fOldAngle + w * p->fNewAngle;
                break;
            }
            case 2: {   // half-cosine; the 1-(1-c) is deliberate
                float c = cosf(t * 3.14159265f / 2.0f);
                m_fViewAngle = (1.0f - (1.0f - c)) * p->fOldAngle + (1.0f - c) * p->fNewAngle;
                break;
            }
            case 3: {   // half-sine: folds t and shares case 0's body
                t = sinf(t * 3.14159265f / 2.0f);
            }
            case 0:     // linear, and the shared tail for case 3
                m_fViewAngle = (1.0f - t) * p->fOldAngle + t * p->fNewAngle;
            }
        }
    }
    Gob::Animate(fDelta);
}

// debug-console handler: beginviewangleanim [angle] [length] [interpType]
char* beginviewangleanim(char* pszCmd)
{
    float fAngle;
    float fLength;
    int   nType;
    if (sscanf(pszCmd, "%f%f%d", &fAngle, &fLength, &nType) != 3)
        return const_cast<char*>("beginviewangleanim [angle] [length] [interpType]");
// Unchecked downcast: a missing camera is not handled.
    Camera* pCam = static_cast<Camera*>(FindGob(const_cast<char*>("camera")));
    pCam->BeginViewAngleAnimation(fAngle, fLength, nType);
    return const_cast<char*>("success");
}

// debug-console handler: endviewangleanim
char* endviewangleanim(char*)
{
// Unchecked downcast, as in beginviewangleanim.
    Camera* pCam = static_cast<Camera*>(FindGob(const_cast<char*>("camera")));
    pCam->EndViewAngleAnimation(1);
    return const_cast<char*>("success");
}

CAurCamera* NewCAurCamera(char* pszName)
{
    Camera* pCam = new Camera(pszName);
    return pCam;
}


