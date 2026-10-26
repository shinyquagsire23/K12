#include "k2stdint.h"
#include "port/aslglapi.h"
#define MAXTREE_KEEP_DTOR_OUT_OF_LINE

#if KOTOR1
#define K1_AURORA_LIST  // K1: List.h grows from 8
#endif
#include "AURORA/aurmodel.h"
#include "nullname.h"
#include "auranim.h"
#include "auremitter.h"
#include "aurscene.h"
#include "include/aurbehavior.h"
#include "List.h"
#include "aurtexman.h"
#include "math3d.h"
#include "aurmaterial.h"
#include "aurwindmanager.h"
#include "aurprimitive.h"

int maxdepth = 1000;
float animdepthdist1 = 12.0f;
float animdepthdist2 = 30.0f;
int animdepth2 = 8;
int animdepth1 = 12;
int checkdepth = 1;

int depth;

int realmaxdepth;

int AnimRun::m_AnimFlags;
AnimRun* AnimRun::m_pTraceRun;
float AnimRun::m_AnimLength;

float lightsaberdelta = 0.00999999978f;
float lightsabersegmentlife = 0.0120000001f;
int lightsaberSegments;


extern int quaternionidentityoptimization;

void GetAllNamedTextures(Part* pPart, List<CAurTexture*>& out, char* pszName);
int  AurTextureSetAttributes(CAurTexture* pTexture, char* pszAttributes);

void PartSetFog(Part* pPart, bool b);
int  PartSetCanDownSample(Part* pPart, char c);
Part* FindPart(Part* pRoot, char* pszName);

// Finds the named animation in a model's tree.
Animation* FindAnimation(Model* pModel, char* pszName);
Model* FindModel(char* pszName);
#if KOTOR1  // K1: the model streams are SDL_RWops
struct SDL_RWops;
Model* LoadModel(SDL_RWops* pFile, SDL_RWops* pAux);
#else
Model* LoadModel(FILE* pFile, FILE* pAux);
#endif

// Applies a model node's pose to a part subtree; Gob::Reset uses it to snap a tree back to rest.
void AnimateHierarchy(Part* pPart, MdlNode* pNode, float a, float b, float c, bool d);

float position_scale_factor = 1.0f;

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcmath.h"

#include "port/crtapi.h"


int enablegobanimations = 1;
int disablegobdist;
int testanimationshortcircuit = 1;
int rebuildbboxes;
int rebuildspheres = 1;
float talkalpha;

void ProceduralAnimateHierarchy(Part* pPart);

// A bounded formatter; the size argument is the destination buffer length.
static void snprintff(char* buf, uint64_t n, const char* fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    __vsprintf_chk(buf, 0, n, fmt, ap);
    __builtin_va_end(ap);
}

Gob* FindGob(char* pszName);

AnimRun* CurrentAnimation(Gob* pGob);

void exportparts(Gob* pGob, Part* pPart);

extern FILE* spewpltfp;

// Clamp helpers. They return by reference, so binding a constant bound materialises a temporary.
template <class T> static inline const T& min(const T& a, const T& b) { return b < a ? b : a; }
template <class T> static inline const T& max(const T& a, const T& b) { return a < b ? b : a; }

int checkevents = 1;

// Kept out of line, so the SafePointer construction sites branch to it rather than expanding it.
extern template void SafePointerList<CAurObject>::AddPointer(SafePointer<CAurObject>*);

extern float gobspherescaling;

#include "aurglcore.h"
#include "Quaternion.h"
#include "Vector.h"

extern int quaternionidentityoptimization;

// Rotates by the transpose, undoing the orientation and bringing a point already in the gob's world
// frame back into its local frame.
static inline Vector RotateByTransposeModel(const Vector& v, const Quaternion& q)
{
    float w = q.w;
    if (quaternionidentityoptimization == 0 || (w != -1.0f && w != 1.0f)) {
        float nw = -w;
        float x = q.x, y = q.y, z = q.z;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, yz = y * z;
        float wx = nw * x, wy = nw * y, wz = nw * z;
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



// Capability ordinals for the port's GL translation table; this is its own numbering, not the GL

// The converted targets of the base-node controllers: where position, orientation and scale live in
// this build's Part.
enum {
    kTargetPosition = K2_OFFSETOF(Part, m_vLocalPosition),
    kTargetOrientation = K2_OFFSETOF(Part, m_qLocalOrientation),
    kTargetScale = K2_OFFSETOF(Part, m_scale),
};

bool TreeHasSingleTexture(Part* pPart);
bool TreeHasGeometry(Part* pPart);
bool TreeSetGeometryFlags(Part* pPart);
bool TreeHasTransparency(Part* pPart);

extern int enabletransform;

extern int systemmemoryshadows;

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


extern int counttrans;

void SetBeaming();

extern int rendermaxdepth;

extern Vector currentmat __attribute__((visibility("hidden")));

void Gob::AddAnimationFlag(int a)
{
    int i = m_paAnimations.m_count - 1;
    if (i < 0)
        return;
    m_paAnimations.m_pData[i]->m_nFlags |= a;
}

// Removes an animation flag. The removal subtracts rather than clearing bits and is skipped when the
// flags word is already zero.
void Gob::RemoveAnimationFlag(int a)
{
    int i = m_paAnimations.m_count - 1;
    if (i < 0)
        return;
    AnimRun* pAnim = m_paAnimations.m_pData[i];
    if (pAnim->m_nFlags == 0)
        return;
    if (a == 0)
        return;
    pAnim->m_nFlags -= a;
}

// Reports whether the run has an event to fire at the given time. The global checkevents kill-switch
// short-circuits to true; otherwise the stored time decides, and the view-volume test is reached only
// when that time is not past.
bool Gob::CheckEvent(float fTime, bool bCheckViewVolume)
{
    if (!checkevents)
        return true;
    if (m_fCheckEventTime < fTime)
        return true;
    if (!bCheckViewVolume)
        return false;
    return IsInViewVolume();
}

// Stops the out-of-order animation by playing the named one again with the stop bit set, which is how a
// stop is spelled for these runs.
void Gob::StopOutOfOrderAnimation(char* a)
{
    for (int i = 0; i < m_paAnimations.m_count; i++)
    {
        if (m_paAnimations.m_pData[i]->m_nFlags & AnimRun::kAnimOutOfOrder)
        {
            PlayAnimation(a, 1.0f, AnimRun::kAnimStop, 0.0f);
            return;
        }
    }
}

// Recursive animation lookup by name. On a miss it retries on the model's supermodel with the same
// name, and if that whole chain misses, once more with the literal "default".
#if KOTOR1
// K1: the comparison lowercases the first characters before calling strcasecmp.
static inline unsigned char K1AsciiLower(unsigned char c)
{
    return static_cast<unsigned int>(c - 'A') < 26 ? c + ('a' - 'A') : c;
}
static inline bool K1NameEq(const char* a, const char* b)
{
    return K1AsciiLower(a[0]) == K1AsciiLower(b[0]) && strcasecmp(a, b) == 0;
}
#endif
Animation* FindAnimation(Model* pModel, char* pszName)
{
    if (pszName == 0)
        return 0;

    for (int i = 0; i < pModel->m_animations.m_count; i++)
    {
#if KOTOR1
        if (K1NameEq(pModel->m_animations.m_pData[i]->m_szName, pszName))
#else
        if (_stricmp(pModel->m_animations.m_pData[i]->m_szName, pszName) == 0)
#endif
            return pModel->m_animations.m_pData[i];
    }
    if (pModel->m_pSuperModel != 0)
        return FindAnimation(pModel->m_pSuperModel, pszName);

    char szDefault[8] = "default";
    for (int i = 0; i < pModel->m_animations.m_count; i++)
    {
#if KOTOR1
        if (K1NameEq(pModel->m_animations.m_pData[i]->m_szName, szDefault))
#else
        if (_stricmp(pModel->m_animations.m_pData[i]->m_szName, szDefault) == 0)
#endif
            return pModel->m_animations.m_pData[i];
    }
    if (pModel->m_pSuperModel != 0)
        return FindAnimation(pModel->m_pSuperModel, szDefault);
    return 0;
}

// Builds a keyframe-driven run from the given time and value arrays and pushes it to the front of the
// play list. Unlike the other playback entry points, the add-in model is not consulted. The run is
// marked out-of-order and carries the play-order pair in its flag word.
void Gob::PlayOutOfOrderAnimation(char* pszName, float* pfTimes, float* pfValues, int nKeys,
                                  float fDuration)
{
    Animation* pAnimation = FindAnimation(m_pModel, pszName);
    if (pAnimation == 0)
        return;

    OOAnimRun* pRun = new OOAnimRun(pAnimation, pfTimes, pfValues, nKeys, fDuration);
    pRun->m_fTime = 0.0f;
    pRun->m_fBlendAlpha = 0.0f;
    pRun->m_fBlendDuration = pAnimation->m_fTransTime;
    pRun->m_nFlags = AnimRun::kAnimOutOfOrder | AnimRun::kAnimOverlay | AnimRun::kAnimOnce;
    m_paAnimations.Insert(pRun, 0);
}

// The forward-scanning counterpart of SetAnimationTime. It answers with the first run playing the
// name, and with the parsed animation's length when nothing is. The add-in model is searched first,
// this gob's model second, and the returned time is -1 when nothing is playing.
AnimRun* Gob::GetAnimation(char* pszName, float* pfTime, float* pfLength)
{
    AnimRun* pAnimation = 0;
    for (int i = 0; i < m_paAnimations.m_count; i++)
    {
        if (_stricmp(m_paAnimations.m_pData[i]->m_pAnimation->m_szName, pszName) == 0)
        {
            pAnimation = m_paAnimations.m_pData[i];
            break;
        }
    }
    if (pAnimation == 0)
    {
        Animation* pTree = 0;
        if (m_pAddInModel != 0)
            pTree = FindAnimation(m_pAddInModel, pszName);
        if (pTree == 0)
            pTree = FindAnimation(m_pModel, pszName);
        if (pfTime != 0)
            *pfTime = -1.0f;
        if (pfLength != 0)
        {
            if (pTree != 0)
                *pfLength = pTree->m_fLength;
            else
                *pfLength = -1.0f;
        }
    }
    else
    {
        if (pfTime != 0)
            *pfTime = pAnimation->m_fTime;
        if (pfLength != 0)
            *pfLength = pAnimation->m_pAnimation->m_fLength;
    }
    return pAnimation;
}

// Walks the supermodel chain to the model that owns the animation, multiplying each hop's animation
// scale on the way. There is no null guard, so a tree no model in the chain owns loops forever.
float FindAnimScale(Model* pModel, Animation* pAnimation)
{
    float fScale = 1.0f;
    for (;;)
    {
        if (pModel->m_animations.Contains(pAnimation) != 0)
            return fScale;
        fScale = fScale * pModel->m_fAnimScale;
        pModel = pModel->m_pSuperModel;
    }
}

// Re-derives every playing animation's scale against the current model, re-binding it by name after a
// model swap, then snaps the part tree back to its rest pose.
void Gob::Reset()
{
    for (int i = 0; i < m_paAnimations.m_count; i++)
    {
        AnimRun* pAnimation = m_paAnimations.m_pData[i];
        Animation* pTree = FindAnimation(m_pModel, pAnimation->m_pAnimation->m_szName);
        pAnimation->m_fScale = FindAnimScale(m_pModel, pTree);
    }

    position_scale_factor = 1.0f;
    AnimateHierarchy(m_pPart, m_pPart->m_pNode, 0.0f, 0.0f, 1.0f, false);
}

// Searches by name in reverse, walking the play list from the end, and clamps the time into the
// run's length. Returns the value actually stored, or -1 when the name is empty or no run matches.
float Gob::SetAnimationTime(char* pszName, float fTime)
{
    float fResult = -1.0f;
    if (pszName != 0 && *pszName != 0)
    {
#ifdef K2_FIX_ENGINE_BUGS
        AnimRun* pAnimation = 0;
#else
        AnimRun* pAnimation;
#endif
        int i;
        for (i = m_paAnimations.m_count - 1; i >= 0; i--)
        {
            if (_stricmp(m_paAnimations.m_pData[i]->m_pAnimation->m_szName, pszName) == 0)
            {
                pAnimation = m_paAnimations.m_pData[i];
                if (pAnimation == 0)
                    return fResult;
                break;
            }
        }
#ifdef K2_FIX_ENGINE_BUGS
        if (pAnimation == 0)
            return fResult;
#endif
        if (fTime < 0.0f)
        {
            pAnimation->m_fTime = 0.0f;
            fResult = 0.0f;
        }
        else
        {
            float fLength = pAnimation->m_pAnimation->m_fLength;
            if (fLength < fTime)
            {
                pAnimation->m_fTime = fLength;
                fResult = fLength;
            }
            else
            {
                pAnimation->m_fTime = fTime;
                fResult = fTime;
            }
        }
    }
    return fResult;
}

// As SetAnimationTime, but the argument is a fraction of the length. A fraction of 1 or more seeks to
// the very end rather than past it.
float Gob::SetAnimationPercentage(char* pszName, float fPercentage)
{
    float fResult = -1.0f;
    if (pszName != 0 && *pszName != 0)
    {
#ifdef K2_FIX_ENGINE_BUGS
        AnimRun* pAnimation = 0;
#else
        AnimRun* pAnimation;
#endif
        int i;
        for (i = m_paAnimations.m_count - 1; i >= 0; i--)
        {
            if (_stricmp(m_paAnimations.m_pData[i]->m_pAnimation->m_szName, pszName) == 0)
            {
                pAnimation = m_paAnimations.m_pData[i];
                if (pAnimation == 0)
                    return fResult;
                break;
            }
        }
#ifdef K2_FIX_ENGINE_BUGS
        if (pAnimation == 0)
            return fResult;
#endif
        if (fPercentage <= 0.0f)
        {
            pAnimation->m_fTime = 0.0f;
            fResult = 0.0f;
        }
        else if (fPercentage >= 1.0f)
        {
            fResult = pAnimation->m_pAnimation->m_fLength;
            pAnimation->m_fTime = fResult;
        }
        else
        {
            fResult = pAnimation->m_pAnimation->m_fLength * fPercentage;
            pAnimation->m_fTime = fResult;
        }
    }
    return fResult;
}

// Plays, queues or stops an animation. The flag word's bits select the behaviour: keep the run's
// existing blend fields, queued, adopt the next run's play time, queue priority, ignore an
// already-playing run of this name, stop this name, and only set the flags without restarting. A
// null or empty name, or the literal "nullptr", stops every animation instead. Runs with the same
// name are scanned from the end of the list, and runs are inserted so the oldest plays last.
int Gob::PlayAnimation(char* pszName, float fSpeed, int nFlags, float fStart)
{
    char szBuf[256];

    if (pszName == 0 || *pszName == 0 || _stricmp(NULL_NAME, pszName) == 0)
    {
        for (int i = m_paAnimations.m_count - 1; i >= 0; i--)
        {
            if (m_pLookAt != 0
                && static_cast<LookAtAnimation*>(m_pLookAt)->m_pRun == m_paAnimations.m_pData[i])
            {
                static_cast<LookAtAnimation*>(m_pLookAt)->m_pRun = 0;
                delete static_cast<CustomAnimation*>(m_pLookAt);
                m_pLookAt = 0;
            }
            delete m_paAnimations.m_pData[i];
        }
        m_paAnimations.m_count = 0;
        return 1;
    }

    AnimRun* pRun = 0;
    bool bReuse = false;
    if ((nFlags & AnimRun::kAnimNoReuse) == 0)
    {
        int i = m_paAnimations.m_count - 1;
        while (i >= 0)
        {
            if (_stricmp(m_paAnimations.m_pData[i]->m_pAnimation->m_szName, pszName) == 0)
                break;
            i--;
        }
        if (i >= 0)
        {
            pRun = m_paAnimations.m_pData[i];
            if ((nFlags & AnimRun::kAnimStop) != 0)
            {
                pRun->m_nFlags |= AnimRun::kAnimStop | AnimRun::kAnimOnce;
                return 1;
            }
        }
    }

    if (pRun == 0 && _stricmp(pszName, "current") == 0)
        pRun = CurrentAnimation(this);

    if (pRun != 0)
    {
        m_paAnimations.Remove(pRun);
        bReuse = true;
    }

    if ((nFlags & AnimRun::kAnimRemove) != 0)
    {
        if (!bReuse)
            return 0;
        snprintff(szBuf, 64, "done%s", pRun->m_pAnimation->m_szName);
        DoEvent(szBuf, 0);
        if (m_pLookAt != 0 && static_cast<LookAtAnimation*>(m_pLookAt)->m_pRun == pRun)
        {
            static_cast<LookAtAnimation*>(m_pLookAt)->m_pRun = 0;
            delete static_cast<CustomAnimation*>(m_pLookAt);
            m_pLookAt = 0;
        }
        delete pRun;
        return 1;
    }

    Model* pModel = 0;
    Animation* pAnimation = 0;
    if (m_pAddInModel != 0)
    {
        pAnimation = FindAnimation(m_pAddInModel, pszName);
        if (pAnimation != 0)
            pModel = m_pAddInModel;
    }
    if (pAnimation == 0)
    {
        pAnimation = FindAnimation(m_pModel, pszName);
        pModel = m_pModel;
    }

    if (fStart < 0.0f)
        fStart = 0.0f;
    if (pAnimation != 0)
    {
        while (fStart > pAnimation->m_fLength)
            fStart = fStart - pAnimation->m_fLength;

        if (pRun == 0)
        {
            pRun = new AnimRun(pAnimation);

            if (fSpeed == 0.0f)
                pRun->m_fTime = 0.0f;
            if (fSpeed < 0.0f)
                pRun->m_fTime = pAnimation->m_fLength + 1e-7f;
            if (fSpeed > 0.0f || fStart != 0.0f)
                pRun->m_fTime = (fStart != 0.0f) ? fStart : -1e-7f;

            if ((nFlags & AnimRun::kAnimNoBlendIn) == 0)
            {
                pRun->m_fBlendAlpha = 0.0f;
                pRun->m_fBlendDuration = pAnimation->m_fTransTime;
            }
        }
    }
    else if (pRun == 0)
    {
        return 0;
    }

    pRun->m_fScale = FindAnimScale(pModel, pAnimation);
    pRun->m_nFinished = 0;
    pRun->m_fSpeed = fSpeed;
    pRun->m_nFlags = nFlags;

    int nIndex = 0;
    if ((nFlags & AnimRun::kAnimOverlay) != 0)
    {
        for (; nIndex < m_paAnimations.m_count; nIndex++)
            if ((m_paAnimations.m_pData[nIndex]->m_nFlags & AnimRun::kAnimOutOfOrder) == 0)
                break;
    }
    else
    {
        for (; nIndex < m_paAnimations.m_count; nIndex++)
        {
            int n = m_paAnimations.m_pData[nIndex]->m_nFlags;
            if ((n & AnimRun::kAnimOverlay) != 0)
                continue;
            if ((nFlags & AnimRun::kAnimQueue) != 0)
                break;
            if ((n & AnimRun::kAnimQueue) != 0)
                continue;
            break;
        }
    }

    m_paAnimations.Insert(pRun, nIndex);

    if ((nFlags & AnimRun::kAnimSyncTime) != 0 && nIndex + 1 < m_paAnimations.m_count)
    {
        pRun->m_fTime = m_paAnimations.m_pData[nIndex + 1]->m_fTime;
        return 1;
    }
    if ((nFlags & (AnimRun::kAnimQueue | AnimRun::kAnimOverlay)) != 0)
        return 1;

    // Everything queued behind the new run is announced done and dropped.
    while (nIndex + 1 < m_paAnimations.m_count)
    {
        AnimRun* pNext = m_paAnimations.m_pData[nIndex + 1];
        snprintff(szBuf, 256, "done%s", pNext->m_pAnimation->m_szName);
        m_paAnimations.Remove(pNext);
        DoEvent(szBuf, 0);
        delete pNext;
    }

    snprintff(szBuf, 256, "start%s", pRun->m_pAnimation->m_szName);
    DoEvent(szBuf, 0);
    return 1;
}

CallBack::CallBack()
{
    m_szName[0] = 0;
    m_pfnCallback = 0;
    m_pUserData = 0;
    m_fTime = 10.0f;
    m_bFlag = 0;
}

    // The name copy is the fortified strcpy with the member's own array size.
CallBack::CallBack(void (*pfn)(CAurObject*, char*, void*), char* pszName, void* pUserData,
                   float fTime, bool b)
{
    m_pfnCallback = pfn;
    __strcpy_chk(m_szName, pszName != 0 ? pszName : "", 32);
    m_pUserData = pUserData;
    m_fTime = fTime;
    m_bFlag = b;
}

CallBack* Gob::RegisterCallBack(void (*cb)(CAurObject*, char*, void*), char* pszName,
                                void* pUserData, float fTime, bool bFlag)
{
    CallBack* pCallBack = new CallBack(cb, pszName, pUserData, fTime, bFlag);
    m_callbacks.Add(pCallBack);
    return pCallBack;
}

// Only a callback this gob actually registered is freed; removal shifts the list down in place.
void Gob::DeregisterCallback(CallBack* a)
{
    for (int i = 0; i < m_callbacks.m_count; i++)
    {
        if (m_callbacks.m_pData[i] == a)
        {
            m_callbacks.DelIndex(i);
            delete a;
            return;
        }
    }
}

// Walks the callback list back to front so a handler may deregister itself, and passes the event data,
// or the callback's own stored user data when that is null. The safe-pointer local is re-tested every
// iteration because a callback may destroy this gob.
void Gob::DoEvent(char* sEvent, void* pData)
{
    int n = m_callbacks.m_count;
    SafePointer<CAurObject> pThis(static_cast<CAurObject*>(this));  // idiom-exempt: opener 21 debt (Gob:CAurObject not yet derived)
    for (int i = n - 1; i >= 0 && pThis.m_pObject != 0; i--)
    {
        CallBack* pCallBack = m_callbacks.m_pData[i];
#if KOTOR1  // K1: tests the first character before strcasecmp
        if (pCallBack->m_szName[0] != 0 && pCallBack->m_szName[0] != '*' &&
            !K1NameEq(pCallBack->m_szName, sEvent))
            continue;
#else
        if (pCallBack->m_szName[0] != 0 && pCallBack->m_szName[0] != '*' &&
            _stricmp(pCallBack->m_szName, sEvent) != 0)
            continue;
#endif
        if (!CheckEvent(pCallBack->m_fTime, pCallBack->m_bFlag))
            continue;
        pCallBack->m_pfnCallback(static_cast<CAurObject*>(this), sEvent,  // idiom-exempt: opener 21 debt (Gob:CAurObject not yet derived)
                                 pData != 0 ? pData : pCallBack->m_pUserData);
    }
}

// Fires the animation events between the two times, wrapping past the end of the animation when the
// interval is inverted: the tail piece is handled by a recursive call and the head piece here. One flag
// bit selects the direction and another means "use the whole timeline"; the huge bounds are sentinels
// that leave the wrapped half unbounded.
void AnimateEvents(Gob* pGob, Animation* pAnim, float fStart, float fEnd, int nFlags)
{
    if (nFlags & 2)
    {
        fStart = -1000000000.0f;
        fEnd = 1000000000.0f;
        nFlags &= ~2;
    }

    if (nFlags & 1)
    {
        while (fStart < fEnd)
        {
            AnimateEvents(pGob, pAnim, fStart, -1000000000.0f, nFlags);
            fStart = 1000000000.0f;
        }

        for (int i = pAnim->m_events.m_count - 1; i >= 0; i--)
        {
            AnimationEvent* pEvent = &pAnim->m_events.m_pData[i];
            if (pEvent->m_fTime >= fEnd && pEvent->m_fTime < fStart)
                pGob->DoEvent(pEvent->m_szName, 0);
        }
    }
    else
    {
        while (fStart > fEnd)
        {
            AnimateEvents(pGob, pAnim, fStart, 1000000000.0f, nFlags);
            fStart = -1000000000.0f;
        }

        for (int i = 0; i < pAnim->m_events.m_count; i++)
        {
            AnimationEvent* pEvent = &pAnim->m_events.m_pData[i];
            if (pEvent->m_fTime > fStart && pEvent->m_fTime <= fEnd)
                pGob->DoEvent(pEvent->m_szName, 0);
        }
    }
}

// The first run that is neither queued nor paused, which is the one actually playing.
AnimRun* CurrentAnimation(Gob* pGob)
{
    for (int i = 0; i < pGob->m_paAnimations.m_count; i++)
    {
        AnimRun* pAnimation = pGob->m_paAnimations.m_pData[i];
        if ((pAnimation->m_nFlags & (AnimRun::kAnimQueue | AnimRun::kAnimOverlay)) == 0)
            return pAnimation;
    }
    return 0;
}

void Gob::EndLookAtAnimate()
{
    if (m_pLookAt != 0)
        static_cast<CustomAnimation*>(m_pLookAt)->Deactivate();
}

// The blend and scale defaults are zero previous time, zero time, alpha 1.0 and no blend duration, plus
// a speed of 1.0 and no flags.
AnimRun::AnimRun(Animation* pAnimation)
{
    m_pAnimation = pAnimation;
    m_fSpeed = 1.0f;
    m_nFlags = 0;
    m_fPrevTime = 0.0f;
    m_fTime = 0.0f;
    m_fBlendAlpha = 1.0f;
    m_fBlendDuration = 0.0f;
    m_fScale = 1.0f;
    m_nFinished = 0;
}

// The deactivated flag and the two pointers are this class's own members; the animation and run
// pointers stay null until the builders fill them in.
CustomAnimation::CustomAnimation()
    : m_bDeactivated(0), m_pAnimation(0), m_pRun(0)
{
}

// Deletes the tree as a plain model tree rather than as an animation, so the animation's own event list
// is deliberately not freed on this path.
CustomAnimation::~CustomAnimation()
{
    delete static_cast<MaxTree*>(m_pAnimation);
}

// The arc limits are clamped through this translation unit's reference-returning min helper, and the
// last statement is a virtual call to BuildAnimation.
LookAtAnimation::LookAtAnimation(CAurObject* pObject, Part* pPart, Gob* pGob, Part* pPart2,
                                 float fArcH, float fArcV, float f3, float f4)
{
    m_pObject.m_pObject = pObject;
    if (pObject != 0)
        pObject->AddPointer(&m_pObject);
    m_pTargetPart = pPart;
    m_pGob = pGob;
    m_pSourcePart = pPart2;
#if KOTOR1  // K1: the arc clamp is fminf rather than the reference-returning min
    m_fArcH = fminf(fArcH < 0.0f ? -fArcH : fArcH, 89.0f);
    m_fArcV = fminf(fArcV < 0.0f ? -fArcV : fArcV, 89.0f);
#else
    m_fArcH = min(fArcH < 0.0f ? -fArcH : fArcH, 89.0f);
    m_fArcV = min(fArcV < 0.0f ? -fArcV : fArcV, 89.0f);
#endif
    m_fTransTime = f3 < 0.0f ? -f3 : f3;
    m_fMaxDistance = f4 < 0.0f ? -f4 : f4;
    m_pGoalKey = 0;
    m_pSourceKey = 0;
    m_bReleaseStarted = 0;
    BuildAnimation();
}

// Drops every play-list entry whose tree is the look-at animation's, then runs the generated teardown.
LookAtAnimation::~LookAtAnimation()
{
    int i = m_pGob->m_paAnimations.m_count;
    while (i--)
        if (m_pGob->m_paAnimations.m_pData[i] == m_pRun)
            m_pGob->m_paAnimations.DelIndex(i);
}

// The two part names are looked up on different trees, the target name on the looked-at gob's tree
// and the source name on this gob's. The old look-at run is unlinked and nulled before its object
// is deleted, and the new run is pushed to the front of the play list.
void Gob::BeginLookAtAnimate(CAurObject* pObject, char* pszTargetPart, char* pszSourcePart,
                             float fArcH, float fArcV, float f3, float f4)
{
    Part* pTargetPart = FindPart(static_cast<Gob*>(pObject)->m_pPart, pszTargetPart);
    Part* pSourcePart = FindPart(m_pPart, pszSourcePart);

    if (pTargetPart == 0 || pSourcePart == 0)
    {
        EndLookAtAnimate();
        return;
    }

    if (m_pLookAt != 0)
    {
        AnimRun* pOldRun = static_cast<LookAtAnimation*>(m_pLookAt)->m_pRun;
        if (pOldRun != 0)
        {
            if (m_paAnimations.Contains(pOldRun))
                m_paAnimations.Remove(pOldRun);
            delete pOldRun;
            static_cast<LookAtAnimation*>(m_pLookAt)->m_pRun = 0;
        }
        delete static_cast<CustomAnimation*>(m_pLookAt);
        m_pLookAt = 0;
    }

    LookAtAnimation* pLookAt = new LookAtAnimation(pObject, pTargetPart, this, pSourcePart,
                                                  fArcH, fArcV, f3, f4);
    m_pLookAt = pLookAt;

    Animation* pAnimation = pLookAt->m_pAnimation;
    AnimRun* pRun = new AnimRun(pAnimation);
    pRun->m_fBlendAlpha = 0.0f;
    pRun->m_fBlendDuration = pAnimation->m_fTransTime;
    pRun->m_nFlags = AnimRun::kAnimOverlay;
    m_paAnimations.Insert(pRun, 0);
    static_cast<LookAtAnimation*>(m_pLookAt)->m_pRun = pRun;
}

// The console command. The two part names cross over at the call site, so the looked-at gob's part
// is the target and the looking gob's part the source.
const char* beginlookatanimate(char* pszArgs)
{
    char szLookingGob[32];
    char szLookingPart[32];
    char szLookAtGob[32];
    char szLookAtPart[32];

    if (sscanf(pszArgs, "%s %s %s %s", szLookingGob, szLookingPart, szLookAtGob, szLookAtPart) != 4)
        return "Usage: beginlookatanimate lookinggob lookingpart lookatgob lookatpart";

    Gob* pLookingGob = FindGob(szLookingGob);
    if (pLookingGob == 0)
        return "Cannot find looking gob.";

    Gob* pLookAtGob = FindGob(szLookAtGob);
    if (pLookAtGob == 0)
        return "Cannot find lookat gob.";

    pLookingGob->BeginLookAtAnimate(static_cast<CAurObject*>(pLookAtGob), szLookAtPart, szLookingPart,
                                    60.0f, 45.0f, 0.25f, 10.0f);
    return "success";
}

const char* endlookatanimate(char* pszArgs)
{
    Gob* pGob = FindGob(pszArgs);
    if (pGob == 0)
        return "Cannot find gob.";
    pGob->EndLookAtAnimate();
    return "success";
}

const char* addflag(char* pszArgs)
{
    char szName[64];
    int  nFlag;

    if (sscanf(pszArgs, "%s %d", szName, &nFlag) != 2)
        return "Usage: addflag <gobname> <flag>";

    Gob* pGob = FindGob(szName);
    if (pGob == 0)
        return "Failure - could not find gob with that name.";

    pGob->AddAnimationFlag(nFlag);
    return "Success";
}

// Advances the current run's play time. A looping run saturates at either end of its length; an
// unlooping one wraps once past the length and back up past zero.
const char* animadvance(char* pszArgs)
{
    char szName[64];
    float fAdvance = 0.0f;
    szName[0] = '\0';

    static_cast<void>(sscanf(pszArgs, "%s %f", szName, &fAdvance));

    Gob* pGob = FindGob(szName);
    if (pGob == 0)
        return "No Object with that name";

    AnimRun* pRun = CurrentAnimation(pGob);
    if (pRun == 0)
        return "No Current Animation";

    float fTime = fAdvance + pRun->m_fTime;
    pRun->m_fTime = fTime;
    float fLength = pRun->m_pAnimation->m_fLength;

    float fWrap;
    float fRepair;
    if ((pRun->m_nFlags & AnimRun::kAnimOnce) != 0)
    {
        fWrap = (fTime > fLength) ? fLength : fTime;
        fRepair = 0.0f;
    }
    else
    {
        fWrap = (fTime > fLength) ? fTime - fLength : fTime;
        fRepair = fLength + fWrap;
    }

    float fNew = (fWrap < 0.0f) ? fRepair : fWrap;
    if (fTime > fLength || fWrap < 0.0f)
        pRun->m_fTime = fNew;

    static char szResult[256];
    snprintff(szResult, 256, "%s %s %f", szName, pRun->m_pAnimation->m_szName, static_cast<double>(fNew));
    return szResult;
}

// The console command `play <object> <anim> [speed] [flags] [starttime]`. Two to five tokens are
// accepted, so the trailing arguments keep their defaults when the scan stops early.
const char* play(char* pszArgs)
{
    char szName[64];
    char szAnim[64];
    float fSpeed = 1.0f;
    int   nFlags = 0;
    float fStart = 0.0f;

    int nRet = sscanf(pszArgs, "%s %s %f %d %f", szName, szAnim, &fSpeed, &nFlags, &fStart);
    if (static_cast<unsigned int>(nRet - 2) <= 3)
    {
        Gob* pGob = FindGob(szName);
        if (pGob == 0)
            return "unable to find object by that name";

        if (pGob->PlayAnimation(szAnim, fSpeed, nFlags, fStart) == 0)
            return "didn't find animation by that name";
        return "success";
    }
    return "usage:  play object anim [speed] [flags] [starttime]";
}

const char* getanim(char* pszArgs)
{
    char szName[64];
    char szAnim[64];
    float fTime;
    float fLength;

    if (sscanf(pszArgs, "%s %s", szName, szAnim) != 2)
        return "usage:  getanim object anim";

    Gob* pGob = FindGob(szName);
    if (pGob == 0)
        return "unable to find object by that name";

    if (pGob->GetAnimation(szAnim, &fTime, &fLength) == 0)
        return "animation not found on that gob";

    static char szResult[256];
    snprintff(szResult, 256, "animation found: time=%.2f, length=%.2f",
              static_cast<double>(fTime), static_cast<double>(fLength));
    return szResult;
}

// The console command `resetgob <object>`.
const char* resetgob(char* pszArgs)
{
    char szName[64];
    szName[0] = '\0';

    static_cast<void>(sscanf(pszArgs, "%s", szName));

    Gob* pGob = FindGob(szName);
    if (pGob == 0)
        return "No Object with that name";

    pGob->Reset();
    return "Gob has been reset";
}

// The tree's parser entry point, used by the constructor below.
void AnimationParseField(void* pAnimation, char* pszLine);

// The tag is the tree base's own type tag, which distinguishes the plain tree, a model and an animation.
// The body clears the tag and drops the event list's storage; the count test is real, since the resize is
// gated on the count as well as on the data pointer.
Animation::~Animation()
{
    if (m_cType) {
        m_cType = 0;
        if (m_events.m_count)
            m_events.SetSize(0);
    }
}

Animation::Animation(char* pszName) : MaxTree(pszName), m_pModel(0)
{
    m_cType = MaxTree::MAXTREE_ANIMATION_ID;
    m_szAnimRoot[0] = 0;
    m_fLength = 1.0f;
    m_fTransTime = 0.25f;
    m_pfnParseField = AnimationParseField;
}

// The model parser's per-tree-type parse trampoline. It re-types the receiver and calls the real
// member.
void AnimationParseField(void* pAnimation, char* pszLine)
{
    static_cast<Animation*>(pAnimation)->InternalParseField(pszLine);
}

int animnoloop(char* pszArgs) { K2_UNUSED(pszArgs); return AnimRun::kAnimOnce; }

int animqueue(char* pszArgs) { K2_UNUSED(pszArgs); return AnimRun::kAnimQueue; }

int animremove(char* pszArgs) { K2_UNUSED(pszArgs); return AnimRun::kAnimRemove; }

// Drives the animation each tick. It advances every run, poses the part tree and fires the events
// each run covered.
//
// The whole body is skipped when the scene tick counter already matches the value latched on this gob,
// so a pose advances at most once per scene tick. Three events are announced: "start" when a queued run
// is unpaused, "loop" when a run wraps past its end, and "done" when a blend-out finishes. The blend
// alpha climbs while a run plays and, once the run has reached its end, falls twice as fast; at zero the
// non-looping runs queued behind it are dropped. A zero blend duration is special-cased on both sides so
// a zero-length blend completes in one tick instead of dividing by zero.
//
// The pose is suppressed for an already-posed gob that is neither in a cutscene, lit, always rendering,
// nor in the view volume.
void Gob::Animate(float fDelta)
{
    char szBuf[64];

    ProcessGob();

    if (enablegobanimations == 0)
        return;

    // Skip the tick when the scene's tick counter has not advanced since the last pose.
    if (m_pScene != 0 && m_pScene->m_nAnimateTick == m_nLastAnimateTick)
        return;
    if (m_pScene != 0)
        m_nLastAnimateTick = m_pScene->m_nAnimateTick;

    if (m_pScene != 0 && m_bWind && m_pScene->m_pWindManager != 0)
    {
        // The radius defaults to 1.2 and the life to how far this gob moved since the last tick, clamped
        // to 1.0, so a still gob makes no wind.
        float fRadius = m_fWindRadius == 0.0f ? 1.2f : m_fWindRadius;
        float fLife = m_fWindLife;
        if (fLife == 0.0f)
        {
            Vector vMove = m_vPosition - m_vPreviousPosition;
            fLife = magnitude(vMove);
            if (fLife > 1.0f)
                fLife = 1.0f;
        }
        m_pScene->m_pWindManager->AddPointSourceWind(m_vPosition, fRadius, fLife,
                                                     m_fWindStrength);
    }

    SafePointer<CAurObject> pThis(static_cast<CAurObject*>(this));  // idiom-exempt: opener 21 debt (Gob:CAurObject not yet derived)

    if (m_pBehavior != 0)
    {
        m_pBehavior->Control(fDelta);
        if (pThis.m_pObject == 0)
            return;
    }

    // A zero delta only re-poses gobs whose model asks to be forced through the animation.
    if (fDelta <= 0.0f && (m_pModel == 0 || (m_pModel->m_nFlags & Model::kModelLightsaber) == 0))
        return;

    // The event-distance metric is the larger horizontal axis of this gob's offset from the scene
    // focus.
    if (m_pScene != 0 && disablegobdist == 0)
    {
        m_fCheckEventTime =
            max(m_vPosition.x - m_pScene->m_vSceneFocus.x < 0.0f
                    ? -(m_vPosition.x - m_pScene->m_vSceneFocus.x)
                    : m_vPosition.x - m_pScene->m_vSceneFocus.x,
                m_vPosition.y - m_pScene->m_vSceneFocus.y < 0.0f
                    ? -(m_vPosition.y - m_pScene->m_vSceneFocus.y)
                    : m_vPosition.y - m_pScene->m_vSceneFocus.y);
    }
    else
        m_fCheckEventTime = 0.0f;

    // When the scene is not animating, only a force-animated gob is posed; a model carrying the force flag
    // is asked a second time, because the two calls may disagree.
    bool bAnimate = true;
    if (m_pScene != 0 && !m_pScene->IsAnimating())
    {
        bool bForce = IsForceAnimated();
        if (!bForce && (m_pModel == 0 || (m_pModel->m_nFlags & Model::kModelLightsaber) == 0))
            return;
        if (m_pModel != 0 && (m_pModel->m_nFlags & Model::kModelLightsaber) != 0)
            bAnimate = IsForceAnimated();
        else
            bAnimate = true;
    }

    if (m_pPart != 0 && m_bAnimateUV)
        ProceduralAnimateHierarchy(m_pPart);

    // Tick the look-at animation through its own virtuals; a report of inactive tears it down: every run
    // playing its tree leaves the play list, its own run is freed, then the object.
    if (m_pLookAt != 0)
    {
        if (static_cast<LookAtAnimation*>(m_pLookAt)->IsActive())
            static_cast<LookAtAnimation*>(m_pLookAt)->Control();
        else
        {
            Animation* pLookAtTree = static_cast<LookAtAnimation*>(m_pLookAt)->m_pAnimation;
            for (int j = m_paAnimations.m_count; j > 0; j--)
            {
                if (m_paAnimations.m_pData[j - 1]->m_pAnimation == pLookAtTree)
                    m_paAnimations.DelIndex(j - 1);
            }
            AnimRun* pDeadRun = static_cast<LookAtAnimation*>(m_pLookAt)->m_pRun;
            if (pDeadRun != 0)
            {
                delete pDeadRun;
                static_cast<LookAtAnimation*>(m_pLookAt)->m_pRun = 0;
            }
            delete static_cast<CustomAnimation*>(m_pLookAt);
            m_pLookAt = 0;
        }
    }

    // In the already-posed cull, an unlit, non-cutscene gob that is not always rendering and not in
    // the view volume skips the pose unless its model carries the force flag.
    if (m_pModel != 0)
    {
        if (!IsInCutscene() && m_cForcePose == 0 && m_nDynamicLights == 0
            && (m_pModel->m_nFlags & (Model::kModelEffect | Model::kModelAmbient)) == 0 && !IsAlwaysRendering()
            && (m_pModel->m_nFlags & Model::kModelDoor) == 0 && !IsInViewVolume()
            && (m_pModel->m_nFlags & Model::kModelLightsaber) == 0)
        {
            if (IsInViewVolume() || m_pModel == 0 || (m_pModel->m_nFlags & Model::kModelCharacter) == 0)
                return;
            bAnimate = false;
        }
    }

    int nCount = m_paAnimations.m_count;
#if KOTOR1  // K1: no pose gate word, the pose always applies
    bool bCanPose = true;
#else
    bool bCanPose = m_nPoseDisabled == 0;
#endif
    m_cForcePose = 0;
    m_bAnimating = nCount > 0;

    // A queued last run is the exception: the queue bit on the last entry means it was played as a queue
    // extender, so clearing it and announcing "start" lets it take part this tick.
    if (nCount > 0)
    {
        AnimRun* pLast = m_paAnimations.m_pData[nCount - 1];
        if (pLast->m_nFlags & AnimRun::kAnimQueue)
        {
            pLast->m_nFlags -= AnimRun::kAnimQueue;
            snprintff(szBuf, 64, "start%s", pLast->m_pAnimation->m_szName);
            DoEvent(szBuf, 0);
        }
    }
    bool bPose = bAnimate && bCanPose;

    // Runs are walked back to front. Three flags matter: the queue marker, which makes the entry a skip;
    // the blend-out bit, which makes the run blend out at its end instead of wrapping; and the
    // out-of-order bit, which means the run carries its own key arrays.
    for (int i = nCount;; i--)
    {
        if (pThis.m_pObject == 0)
            return;
        if (i == 0)
            break;
        AnimRun* pRun = m_paAnimations.m_pData[i - 1];
        if (pRun->m_nFlags & AnimRun::kAnimQueue)
            continue;

        pRun->m_fPrevTime = pRun->m_fTime;
        pRun->m_fTime = pRun->m_fTime + pRun->m_fSpeed * fDelta;

        bool bPastEnd = false;
        bool bAdvance = false;

        if (pRun->m_nFlags & AnimRun::kAnimOnce)
        {
            if (pRun->m_nFinished != 0)
            {
                // The blend-out finished last tick, so announce it, unlink it and free it.
                snprintff(szBuf, 64, "done%s", pRun->m_pAnimation->m_szName);
                m_paAnimations.Remove(pRun);
                DoEvent(szBuf, 0);
                if (pRun->m_nFlags & AnimRun::kAnimOutOfOrder)
                {
                    // The key arrays belong to the run subclass and are freed here; the flag is the only type
                    // tag there is.
                    OOAnimRun* pOORun = static_cast<OOAnimRun*>(pRun);
                    delete[] pOORun->m_pfValues;
                    delete[] pOORun->m_pfTimes;
                }
                delete pRun;
                continue;
            }

            if (pRun->m_fTime < 0.0f)
            {
                pRun->m_fTime = 0.0f;
                bAdvance = true;
            }
            else if (pRun->m_nFlags & AnimRun::kAnimOutOfOrder)
            {
                // The out-of-order run's own end is its duration, not the tree's length.
                if (pRun->m_fTime > static_cast<OOAnimRun*>(pRun)->m_fDuration)
                {
                    pRun->m_fBlendDuration = 0.5f;
                    pRun->m_fTime = static_cast<OOAnimRun*>(pRun)->m_fDuration;
                    bPastEnd = true;
                    bAdvance = true;
                }
            }
            else if (pRun->m_fTime > pRun->m_pAnimation->m_fLength)
            {
                pRun->m_fTime = pRun->m_pAnimation->m_fLength;
                bAdvance = true;
            }

            // Reaching a boundary, or being flagged as wanting the ramp, starts the blend-out and unpauses
            // the run queued below this one, since the list is ordered oldest last.
            if (bAdvance || (pRun->m_nFlags & AnimRun::kAnimStop))
            {
                if (i > 1)
                {
                    AnimRun* pQueued = m_paAnimations.m_pData[i - 2];
                    if (pQueued->m_nFlags & AnimRun::kAnimQueue)
                    {
                        pQueued->m_nFlags -= AnimRun::kAnimQueue;
                        snprintff(szBuf, 64, "start%s", pQueued->m_pAnimation->m_szName);
                        DoEvent(szBuf, 0);
                    }
                }

                float fBlend = pRun->m_fBlendDuration;
                float fStep = fBlend <= 0.0f ? 2.0f : 2.0f * fDelta / fBlend;
                pRun->m_fBlendAlpha = pRun->m_fBlendAlpha - fStep;
                if (pRun->m_fBlendAlpha <= 0.0f)
                {
                    pRun->m_fBlendAlpha = 0.0f;
                    pRun->m_nFinished = 1;
                    if (fBlend == 0.0f)
                        pRun->m_fBlendAlpha = 1.0f;
                }
            }
        }
        else
        {
            // A wrapping run gains the tree's length off the bottom and loses it off the top, and
            // both ends announce "loop".
            if (pRun->m_fSpeed <= 0.0f && pRun->m_fTime < 0.0f)
            {
                if (pRun->m_nFlags & AnimRun::kAnimLoopEvents)
                {
                    snprintff(szBuf, 64, "loop%s", pRun->m_pAnimation->m_szName);
                    DoEvent(szBuf, 0);
                }
                pRun->m_fTime = pRun->m_fTime + pRun->m_pAnimation->m_fLength;
                if (pRun->m_fTime < 0.0f)
                    pRun->m_fTime = 0.0f;
            }

            if (pRun->m_fTime > pRun->m_pAnimation->m_fLength && pRun->m_fSpeed >= 0.0f)
            {
                if (pRun->m_nFlags & AnimRun::kAnimLoopEvents)
                {
                    snprintff(szBuf, 64, "loop%s", pRun->m_pAnimation->m_szName);
                    DoEvent(szBuf, 0);
                }
                pRun->m_fTime = pRun->m_fTime - pRun->m_pAnimation->m_fLength;
                if (pRun->m_fTime > pRun->m_pAnimation->m_fLength)
                    pRun->m_fTime = pRun->m_pAnimation->m_fLength;
            }
        }

        if (pRun->m_nFinished != 0)
            goto Pose;
        if (!(pRun->m_fBlendAlpha < 1.0f))
            goto Pose;

        // Blending in: nothing left to do unless the ramp tops out, which retires the run and everything
        // queued behind it unless the run is flagged.
        {
            float fBlend = pRun->m_fBlendDuration;
            float fStep = fBlend <= 0.0f ? 1.0f : fDelta / fBlend;
            pRun->m_fBlendAlpha = pRun->m_fBlendAlpha + fStep;
            if (pRun->m_fBlendAlpha > 1.0f)
            {
                pRun->m_fBlendAlpha = 1.0f;
                if ((pRun->m_nFlags & AnimRun::kAnimOverlay) == 0)
                {
                    while (i < m_paAnimations.m_count)
                    {
                        AnimRun* pDrop = m_paAnimations.m_pData[i];
                        m_paAnimations.Remove(pDrop);
                        delete pDrop;
                    }
                }
            }
        }

    Pose:
        // The trace statics report what is being animated right now; Gob::Reset is their other end.
        position_scale_factor = pRun->m_fScale;
        int nReverse = pRun->m_fSpeed < 0.0f ? 1 : 0;
        if (pRun->m_nFlags & AnimRun::kAnimTrace)
        {
            AnimRun::m_AnimFlags = nReverse;
            AnimRun::m_pTraceRun = pRun;
            AnimRun::m_AnimLength = pRun->m_pAnimation->m_fLength;
        }

        if ((pRun->m_nFlags & AnimRun::kAnimOutOfOrder) == 0)
        {
            if (bPose)
            {
                AnimateHierarchy(m_pPart, pRun->m_pAnimation->m_pMdlNode,
                                 -100.0f - pRun->m_fPrevTime, pRun->m_fTime, pRun->m_fBlendAlpha,
                                 false);
                m_bAnimatedOnce = 1;
            }
        }
        else
        {
            // The out-of-order arm normalises the time by the run's duration, finds the key pair it
            // falls between and hands the node walk the two values plus the fraction between them.
            // A not-found index leaves the fraction at 0.
            OOAnimRun* pOORun = static_cast<OOAnimRun*>(pRun);
            float fT = pRun->m_fTime / pOORun->m_fDuration;
            int nKeys = pOORun->m_nKeys;
            int k = 0;
            if (pOORun->m_pfTimes[0] < fT && nKeys >= 1)
            {
                for (k = 1; k < nKeys; k++)
                {
                    if (!(pOORun->m_pfTimes[k] < fT))
                        break;
                }
            }
            float fAlpha = 0.0f;
            if (k != nKeys)
            {
                float fKeyPrev = pOORun->m_pfTimes[k - 1];
                fAlpha = (fT - fKeyPrev) / (pOORun->m_pfTimes[k] - fKeyPrev);
            }

            if (bPose)
            {
                if (bPastEnd)
                {
                    AnimateHierarchy(m_pPart, pRun->m_pAnimation->m_pMdlNode, -100.0f,
                                     pOORun->m_pfValues[k], pRun->m_fBlendAlpha, false);
                    talkalpha = pRun->m_fBlendAlpha;
                }
                else
                {
                    AnimateHierarchy(m_pPart, pRun->m_pAnimation->m_pMdlNode,
                                     pOORun->m_pfValues[k - 1], pOORun->m_pfValues[k], fAlpha,
                                     false);
                }
                m_bAnimatedOnce = 1;
            }
        }

        // The event pass rides the same tick, over the interval this run just covered.
        AnimateEvents(this, pRun->m_pAnimation, pRun->m_fPrevTime, pRun->m_fTime, nReverse);
        AnimRun::m_AnimFlags = 0;
        AnimRun::m_AnimLength = 0;
        AnimRun::m_pTraceRun = 0;
    }

    // Nothing left playing: give the part tree its own tick and refresh the bounds, unless a scene that is
    // animating, or a model asking for the pass, says otherwise.
    if (m_pPart == 0)
        return;
    if ((m_pScene != 0 && (m_bAnimating || testanimationshortcircuit == 0))
        || (m_pModel != 0 && (m_pModel->m_nFlags & Model::kModelLightsaber) != 0))
    {
        m_pPart->Animate(fDelta);
        if (rebuildbboxes)
            GetMinimumBoundingBox();
        if (rebuildspheres)
            GetMinimumSphere();
    }
}

// The look-at animation is active while its play-state record has not reached the finished latch.
bool LookAtAnimation::IsActive()
{
    return m_pRun->m_nFinished == 0;
}

// Sets the deactivated latch. EndLookAtAnimate's whole body is this call, and the constructor clears it.
void LookAtAnimation::Deactivate()
{
    m_bDeactivated = 1;
}

// Allocates the time and value key arrays; the values are scaled by the tree's length and the times
// copied verbatim.
OOAnimRun::OOAnimRun(Animation* pAnimation, float* pfTimes, float* pfValues, int nKeys,
                     float fDuration)
    : AnimRun(pAnimation)
{
    m_pfValues = new float[nKeys];
    m_pfTimes = new float[nKeys];
    for (int i = 0; i < nKeys; i++)
    {
        m_pfValues[i] = pfValues[i] * pAnimation->m_fLength;
        m_pfTimes[i] = pfTimes[i];
    }
    m_fDuration = fDuration;
    m_nKeys = nKeys;
}

static inline void CopyOrientation(Quaternion* pKey, Part* pPart)
{
    pKey->w = pPart->m_qLocalOrientation.x;
    pKey->x = pPart->m_qLocalOrientation.y;
    pKey->y = pPart->m_qLocalOrientation.z;
    pKey->z = pPart->m_qLocalOrientation.w;
}

// Value-returning clamps local to this file. The shared min and max return by reference, which the
// look-at constructor depends on, so Control uses these instead.
template <class T> static inline T clampMin(const T& a, const T& b) { return b < a ? b : a; }
template <class T> static inline T clampMax(const T& a, const T& b) { return a < b ? b : a; }

// The per-tick driver of the head and eye tracker, in two arms.
//
// The activation arm, latched by a flag, records the source part's local orientation as the from key and
// the identity as the to key, and restarts the run. The aiming arm derives a yaw and a pitch from the two
// parts' world transforms: the yaw is the turn that takes the source's rotated +Y axis onto the horizontal
// offset to the target, the pitch is the elevation of the normalised offset, and each is clamped to its
// arc limit before the two are composed.
//
// The two object tests must not be folded, because the Deactivate call between them forces the
// second to re-read the member.
void LookAtAnimation::Control()
{
    if (m_pObject.m_pObject == 0)
        Deactivate();
    if (m_pObject.m_pObject != 0
        && magnitude(m_pGob->m_vPosition - static_cast<Gob*>(m_pObject.m_pObject)->m_vPosition) > m_fMaxDistance)
        Deactivate();

    if (m_bDeactivated)
    {
        if (m_bReleaseStarted != 0)
            return;

        m_bReleaseStarted = 1;
        CopyOrientation(m_pSourceKey, m_pSourcePart);
#if KOTOR1  // K1: the goal is the current pose too, with no blend back and length 0
        CopyOrientation(m_pGoalKey, m_pSourcePart);
#else
        m_pGoalKey->w = 0.0f;
        m_pGoalKey->x = 0.0f;
        m_pGoalKey->y = 0.0f;
        m_pGoalKey->z = 1.0f;
#endif

        m_pRun->m_nFlags = AnimRun::kAnimOverlay | AnimRun::kAnimOnce;
        m_pRun->m_fPrevTime = 0.0f;
        m_pRun->m_fTime = 0.0f;
        m_pRun->m_fBlendAlpha = 1.0f;
#if KOTOR1  // K1: m_nFinished is left as it is
        m_pAnimation->m_fLength = 0.0f;
#else
        m_pRun->m_nFinished = 0;
        m_pAnimation->m_fLength = 0.25f;
#endif
        return;
    }

    CopyOrientation(m_pSourceKey, m_pSourcePart);

    Vector vSource;
    Quaternion qSource;
    m_pSourcePart->GetWorld(vSource, qSource);

    // The source's own +Y axis, rotated into world space and flattened to the XY plane.
    Vector vHeading = RotateVector(Vector(0.0f, 1.0f, 0.0f), qSource);

    Vector vTarget;
    Quaternion qTarget;
    m_pTargetPart->GetWorld(vTarget, qTarget);

    Vector vOffset = vTarget - vSource;
    Quaternion qTurn = RotationArc(Vector(vHeading.x, vHeading.y, 0.0f),
                                   Vector(vOffset.x, vOffset.y, 0.0f));
    float fYaw = Yaw(qTurn * m_pSourcePart->m_qLocalOrientation);

    vOffset = normalize(vOffset);
    float fPitch = Pitch(Quaternion(Vector(1.0f, 0.0f, 0.0f), asinf(vOffset.z)));

#if KOTOR1  // K1: fminf/fmaxf, so a NaN angle becomes the limit
    fYaw = fminf(fmaxf(fYaw, -m_fArcH), m_fArcH);
    fPitch = fminf(fmaxf(fPitch, -m_fArcV), m_fArcV);
#else
    fYaw = clampMax(fYaw, -m_fArcH);
    fYaw = clampMin(fYaw, m_fArcH);
    fPitch = clampMax(fPitch, -m_fArcV);
    fPitch = clampMin(fPitch, m_fArcV);
#endif

    Quaternion qKey = YawPitchRoll(fYaw, 0.0f, 0.0f) * YawPitchRoll(0.0f, fPitch, 0.0f);
    m_pGoalKey->w = qKey.x;
    m_pGoalKey->x = qKey.y;
    m_pGoalKey->y = qKey.z;
    m_pGoalKey->z = qKey.w;

    m_pRun->m_fPrevTime = 0.0f;
    m_pRun->m_fTime = 0.0f;
    m_pRun->m_fBlendAlpha = 1.0f;
    m_pRun->m_nFinished = 0;
}

// Builds the look-at animation by hand: one synthetic animation tree, one float table and one orientation
// controller.
//
// The tree mirrors the part chain. The first node is the source part's own part and carries the
// keys, and each ancestor part contributes one looking-parent node on top of it. Every node is
// linked both ways, and the topmost node becomes the tree's root, which is what AnimateHierarchy
// walks.
//
// The float table is the whole animation, holding the key's time and blend length, then two
// quaternions back to back, the from key, the source part's own orientation slot, and the to key.
// The two pointers the class keeps point into this table, which is why one orientation copy can
// write both ends.
//
// The controller record is hand-built to match what the parser produces for a four-value, axis plus angle,
// key. The animation's length and its blend-length twin are then re-seeded from the constructor's float
// argument, which the console command defaults to 0.25f.
void LookAtAnimation::BuildAnimation()
{
    m_pAnimation = new Animation(const_cast<char*>("LookAtAnimation"));

    MdlNode* pNode = new MdlNode(const_cast<char*>("looking"));
    pNode->m_nNodeID = static_cast<short>(m_pSourcePart->m_nNodeID);
    MdlNode* pTop = pNode;
    for (Part* pPart = m_pSourcePart->m_pParent; pPart != 0; pPart = pPart->m_pParent)
    {
        MdlNode* pParent = new MdlNode(const_cast<char*>("lookingParent"));
        pTop->m_pParent = reinterpret_cast<unsigned long long>(pParent);
        pParent->m_listChildren.Add(pTop);
        pParent->m_nNodeID = static_cast<short>(pPart->m_nNodeID);
        pTop = pParent;
    }
    pTop->m_pTree = m_pAnimation;
    pTop->m_pParent = 0;
    m_pAnimation->m_pMdlNode = pTop;

    NewController c;
    unsigned long long nTimeIndex = static_cast<unsigned long long>(static_cast<unsigned short>(pNode->m_listFloats.m_count));
    pNode->m_listFloats.Add(0.0f);
    pNode->m_listFloats.Add(0.25f);
    int nKey = pNode->m_listFloats.m_count;

    pNode->m_listFloats.Add(0.0f);
    pNode->m_listFloats.Add(0.0f);
    pNode->m_listFloats.Add(0.0f);
    pNode->m_listFloats.Add(1.0f);
    pNode->m_listFloats.Add(0.0f);
    pNode->m_listFloats.Add(0.0f);
    pNode->m_listFloats.Add(0.0f);
    pNode->m_listFloats.Add(1.0f);

    c.m_nKeysPacked = nTimeIndex | (4ULL << 32) | (static_cast<unsigned long long>(static_cast<unsigned short>(nKey)) << 16);
    c.m_nTargetPacked = kTargetOrientation | (2ULL << 48);
    pNode->m_listControllers.Add(c);

    m_pSourceKey = reinterpret_cast<Quaternion*>(&pNode->m_listFloats.m_pData[static_cast<unsigned short>(nKey)]);
    m_pGoalKey = reinterpret_cast<Quaternion*>(&pNode->m_listFloats.m_pData[static_cast<unsigned short>(nKey) + 4]);

    m_pAnimation->m_fLength = m_fTransTime;
    m_pAnimation->m_fTransTime = m_fTransTime;
}

// Frees the two key arrays and clears both pointers.
OOAnimRun::~OOAnimRun()
{
    if (m_pfValues != 0)
    {
        delete[] m_pfValues;
        m_pfValues = 0;
    }
    if (m_pfTimes != 0)
    {
        delete[] m_pfTimes;
        m_pfTimes = 0;
    }
}

// Cascades the tick to every child part. The bound is re-read through the loop because the virtual call
// may add or remove children.
void Part::Animate(float f)
{
    for (int64_t i = 0; i < m_nChildren; i++)
        m_paChildren[i]->Animate(f);
}

// Ages the blade's ring buffer one frame, then emits a new segment if the part has moved.
//
// The aging half walks the life array from the cursor for the live-segment count; each expired entry
// retires the tail of the ring but never below two live segments, so the blade always keeps a base and a
// tip. The walking cursor is a local copy of the member, since it has to visit every live slot, including
// ones retired this frame.
void PartLightsaber::Animate(float fDelta)
{
    float fStep = lightsaberdelta > fDelta ? lightsaberdelta : fDelta;
    short nLifeIndex = m_nLifeIndex;

    if (currentscene != 0 &&
        currentscene->IsAnimating())
    {
        short nCount = m_nSegmentCount;
        short i = 0;
        short nExpired = 0;

        if (nCount > 0)
        {
            for (; i < nCount; i++)
            {
                m_afSegmentLife[nLifeIndex] += fStep;
                if (m_afSegmentLife[nLifeIndex] > lightsabersegmentlife && nCount - nExpired >= 2)
                {
                    m_nLifeIndex++;
                    if (m_nLifeIndex == MdlNodeLightsaber::NUM_BACK_SEGMENTS)
                        m_nLifeIndex = 0;
                    nExpired++;
                }
                nLifeIndex++;
                if (nLifeIndex == MdlNodeLightsaber::NUM_BACK_SEGMENTS)
                    nLifeIndex = 0;
            }
        }
        m_nSegmentCount = nCount - nExpired;
    }

    // This part's world transform.
    Vector vPos;
    Quaternion qIdentity(1.0f, 0.0f, 0.0f, 0.0f);
    GetWorld(vPos, qIdentity);

    // Emit a segment unless the part has not moved and no slot has aged past 60 ms; the age test is what
    // keeps a stationary but shimmering blade emitting.
#if KOTOR1  // K1: emits only when the part moved, with no 60 ms age test
    if (vPos != m_vLastSegmentPos)
#else
    if (vPos != m_vLastSegmentPos || m_afSegmentLife[m_nLifeIndex] > 60.0f)
#endif
    {
        if (magnitude(vPos - m_vLastSegmentPos) > 1.0f)
        {
            m_nOrientIndex = 0;
            m_nLifeIndex = 0;
            m_nSegmentCount = 0;
        }

        m_vLastSegmentPos = vPos;
        m_afSegmentLife[m_nOrientIndex] = 0.0f;
        m_avSegmentPos[m_nOrientIndex] = vPos;
        m_aqSegmentOrient[m_nOrientIndex] = qIdentity;

        // The ring holds the documented segment count; the arrays are 25 long only because that is what the
        // constructor seeds.
        if (m_nSegmentCount != MdlNodeLightsaber::NUM_BACK_SEGMENTS)
        {
            m_nOrientIndex++;
            m_nSegmentCount++;
            if (m_nOrientIndex >= MdlNodeLightsaber::NUM_BACK_SEGMENTS)
                m_nOrientIndex = 0;
        }
    }

    lightsaberSegments = m_nSegmentCount;
}

// This virtual is a bare return in the shipped build: the pendant solver was cut from the port, in every
// shipped build, so it is one deliberate change rather than an inlining artefact.
//
// As a consequence, the shipped runtime renders dangly meshes forever in their rest pose. The
// constructor still copies the rest pose into the part's slice of the model pool, and the dangly
// vertex primitive still allocates a velocity array that is never read.
//
// The body below is the restored solver, compiled only when the port-cut flag is off, so the shipped build
// still emits the bare return.
#ifndef K2_MOBILE_OPT
extern int renderdangly;

void PartDanglyMesh::Animate(float fDelta)
{
    if (renderdangly == 0 || currentscene == 0)
        return;

    // The integrator's step is clamped to [1/80 s, 1/28.6 s] so a pendant neither freezes on a long frame
    // nor explodes on a short one. Everything downstream, the child cascade included, uses the clamped
    // step.
    float fStep = fDelta;
#ifdef K2_FIX_ENGINE_BUGS
    // The 1/80 s floor ran a pendant, and through the cascade its children, faster than real time above
    // 80 fps; the Euler step scales with the step, so only the ceiling is needed.
    if (fStep > 0.035f)
        fStep = 0.035f;
#else
    if (fStep > 0.035f)
        fStep = 0.035f;
    else if (0.0125f > fStep)
        fStep = 0.0125f;
#endif

    // Scratch vectors, re-seeded per vertex. They are function-local statics, so the solver is not
    // re-entrant. The acceleration vector is never read on any path.
    static Vector tempCoord;
    static Vector nodevertex;
    static Vector vertex;
    static Vector currentvelocity;
    static Vector faccel;
    (void)faccel;

    Vector     vPos;
    Quaternion qOrient;
    GetWorld(vPos, qOrient);

    MdlNodeDanglyMesh* pNode = m_pNode->AsMdlNodeDanglyMesh();
    // These guards exist because the vertex primitive is created lazily by PartInitMaterial, and a
    // node whose constraints field never parsed has no weight array.
    VertexPrimitiveDangly* pVP =
        m_pVertexPrimitive != 0 ? m_pVertexPrimitive->AsVertexPrimitiveDangly() : 0;
    if (pNode == 0 || pVP == 0 || pVP->m_paVertices == 0 ||
        pNode->m_lConstraints.m_pData == 0 || pNode->m_pVertexSource == 0 ||
        m_paVertices == 0)
        return;

    // The vertex count is read as a signed short off the unsigned field.
    const int nVerts = static_cast<short>(pNode->m_nVertexCount);
    Vector* paSource = static_cast<Vector*>(pNode->m_pVertexSource);

    if (magnitude(vPos - m_vLastPosition) > 5.0f)
    {
        // Guard against a teleport, where a load or re-parent moved the part further than any frame
        // of motion could. The simulation is dropped and the rest pose restored rather than
        // whipping the pendant across the world. The velocities are deliberately not cleared.
        for (int i = 0; i < nVerts; i++)
            m_paVertices[i] = paSource[i];
    }
    else
    {
        // The constructor leaves the stored orientation's x, y and z at zero, so the first tick adopts this
        // frame's transform and integrates from rest.
        if (m_qLastOrientation.x == 0.0f && m_qLastOrientation.y == 0.0f && m_qLastOrientation.z == 0.0f)
        {
            m_qLastOrientation = qOrient;
            m_vLastPosition = vPos;
        }

        const Quaternion qLast = m_qLastOrientation;
        const Vector     vLast = m_vLastPosition;

        // The scene wind at the part's position, brought into the part's own frame and scaled by 20.
        const Vector vWind = RotateByTransposeModel(
            currentscene->GetWindVector(vPos, fStep), qOrient) * 20.0f;

        // The frame-to-frame twist is the previous orientation conjugated, times the current one,
        // with the scalar part negated, which is the same rotation.
        Quaternion qDelta;
        qDelta.w = -(qOrient.w * qLast.w + qOrient.x * qLast.x
                   + qOrient.y * qLast.y + qOrient.z * qLast.z);
        qDelta.x = qOrient.x * qLast.w - qOrient.w * qLast.x
                 + qOrient.y * qLast.z - qOrient.z * qLast.y;
        qDelta.y = qOrient.y * qLast.w - qOrient.w * qLast.y
                 + qOrient.z * qLast.x - qOrient.x * qLast.z;
        qDelta.z = qOrient.z * qLast.w - qOrient.w * qLast.z
                 + qOrient.x * qLast.y - qOrient.y * qLast.x;

        // The frame's translation, expressed in the previous frame.
        const Vector vMove = RotateByTransposeModel(vLast - vPos, qLast);

        // The two model-authored coefficients are scaled into this step. Tightness is the spring
        // pulling a vertex back to its rest position, period the velocity damping.
        const float fSpring  = pNode->m_fTightness * 0.5f * fStep;
        const float fDamping = pNode->m_fPeriod * 1.5f * fStep;

        for (int i = 0; i < nVerts; i++)
        {
            // The model's constraint weight, 0..255. 0 means the vertex is pinned to the bone and is never
            // simulated at all.
            const float fConstraint = pNode->m_lConstraints.m_pData[i];
            if (fConstraint == 0.0f)
                continue;

            nodevertex      = paSource[i];
            vertex          = m_paVertices[i];
            currentvelocity = pVP->m_paVertices[i];

            // Carry the vertex and its velocity along with the part's own motion first...
            vertex          = RotateVector(vertex + vMove, qDelta);
            currentvelocity = RotateVector(currentvelocity, qDelta);

            // ...then one explicit Euler step of the spring-damper back toward the rest pose.
            currentvelocity = currentvelocity
                            + (nodevertex - vertex) * (fConstraint * fSpring)
                            - currentvelocity * fDamping;
            pVP->m_paVertices[i] = currentvelocity;

            tempCoord = vertex + currentvelocity * fStep;

            // In the vertex's allowance, a free vertex gets the whole authored displacement, a
            // tightly constrained one almost none.
            const float fLimit = (fConstraint / -255.0f + 1.0f) * pNode->m_fDisplacement;

            // The wind pushes by the same allowance.
            if (currentscene != 0)
                tempCoord = tempCoord + vWind * fLimit;

            // Bail out on divergence, since the integrator can blow up on a step the teleport guard
            // did not catch. The six one-sided compares are all false for a NaN, so a NaN survives
            // into the clamp below.
            const Vector vOff = tempCoord - nodevertex;
            if (-1.0e10f > vOff.x || -1.0e10f > vOff.y || -1.0e10f > vOff.z ||
                vOff.x > 1.0e10f || vOff.y > 1.0e10f || vOff.z > 1.0e10f)
            {
                m_paVertices[i] = nodevertex;
                continue;
            }

            // Clamp the excursion per axis, not by length.
            if (vOff.x > fLimit)
                tempCoord.x = nodevertex.x + fLimit;
            else if (-fLimit > vOff.x)
                tempCoord.x = nodevertex.x - fLimit;
            if (vOff.y > fLimit)
                tempCoord.y = nodevertex.y + fLimit;
            else if (-fLimit > vOff.y)
                tempCoord.y = nodevertex.y - fLimit;
            if (vOff.z > fLimit)
                tempCoord.z = nodevertex.z + fLimit;
            else if (-fLimit > vOff.z)
                tempCoord.z = nodevertex.z - fLimit;

            m_paVertices[i] = tempCoord;
        }
    }

    // Upload the block into this node's slice of the owning model's vertex pool; both arms below land
    // here, a teleport reset included.
    Model* pModel = m_pNode->m_pTree != 0 ? m_pNode->m_pTree->AsModel() : 0;
    if (pModel != 0)
    {
        const int nStride = pNode->m_nVertexStride;
        char* pPool = static_cast<char*>(GLRender::LockPool(pModel->m_nPool,
                                               pNode->m_nVertexOfs + pNode->m_nGeomBase,
                                               nVerts * nStride, 1));
        if (pPool != 0 && nStride >= static_cast<int>(sizeof(Vector)))
        {
            for (int i = 0; i < nVerts; i++)
                *reinterpret_cast<Vector*>(pPool + nStride * i) = m_paVertices[i];
        }
        GLRender::UnlockPoolDeferred(pModel->m_nPool);
    }

    // Latch this frame's transform for the next tick, then cascade to the children.
    m_vLastPosition = vPos;
    m_qLastOrientation = qOrient;

#ifdef K2_FIX_ENGINE_BUGS
    Part::Animate(fDelta);
#else
    Part::Animate(fStep);
#endif
}
#else
// In the shipped shape this vtable slot is a bare return.
void PartDanglyMesh::Animate(float)
{
}
#endif  // K2_MOBILE_OPT

// The model node hierarchy's per-frame control tick, one override per node kind.
//
// Every derived body is the shared body below with one per-kind accessor call in front, so the whole
// family is kept together.
//
// The shared body clamps the adjusted time below -95.0f, then for every controller calls Control and,
// while a traced run is active, ReportKeys with the adjusted time. The controller count is re-read every
// iteration because Control may resize the list.
//
// Each node kind's own tick opens with one virtual call on the part, its own part view, and discards the
// result.

void MdlNode::InternalControl(Part* pPart, float fA, float fB, float fC)
{
    pPart->AsPartTriMesh();

#if KOTOR1
    // K1: the loop is unswitched on the clamp, which makes the body too big to expand into the per-type
    // overrides.
    for (int64_t i = 0; i < m_listControllers.m_count; i++) {
        m_listControllers.m_pData[i].Control(pPart, this, fA, fB, fC);
        if (AnimRun::m_pTraceRun) {
            if (fA < -95.0f)
                m_listControllers.m_pData[i].ReportKeys(pPart, this, -100.0f - fA, fB);
            else
                m_listControllers.m_pData[i].ReportKeys(pPart, this, fA, fB);
        }
    }
#else
    for (int64_t i = 0; i < m_listControllers.m_count; i++) {
        // The adjusted time is loop-invariant but computed inside the body on purpose, because
        // hoisting it above the loop would move the computation before the loop guard.
        float fAdj = fA < -95.0f ? -100.0f - fA : fA;

        m_listControllers.m_pData[i].Control(pPart, this, fA, fB, fC);
        if (AnimRun::m_pTraceRun)
            m_listControllers.m_pData[i].ReportKeys(pPart, this, fAdj, fB);
    }
#endif
}

void MdlNodeLight::InternalControl(Part* pPart, float fA, float fB, float fC)
{
    pPart->AsPartLight();
    MdlNode::InternalControl(pPart, fA, fB, fC);
}

// The prologue's part view is handed to the emitter's radius calculation after the base tick, as a tail
// call, so it must be the last statement.
void MdlNodeEmitter::InternalControl(Part* pPart, float fA, float fB, float fC)
{
    PartEmitter* pEmitter = pPart->AsPartEmitter();
    MdlNode::InternalControl(pPart, fA, fB, fC);
    pEmitter->CalculateRadius();
}

void MdlNodeCamera::InternalControl(Part* pPart, float fA, float fB, float fC)
{
    pPart->AsPartCamera();
    MdlNode::InternalControl(pPart, fA, fB, fC);
}

void MdlNodeReference::InternalControl(Part* pPart, float fA, float fB, float fC)
{
    pPart->AsPartReference();
    MdlNode::InternalControl(pPart, fA, fB, fC);
}

// The trimesh accessor is the one the base's own body also opens with, so it is called twice.
void MdlNodeTriMesh::InternalControl(Part* pPart, float fA, float fB, float fC)
{
    pPart->AsPartTriMesh();
    MdlNode::InternalControl(pPart, fA, fB, fC);
}

extern int quaternionidentityoptimization;

// Spins the direction about +Z by a random angle drawn from the spread, quantised to hundredths of a
// turn: the count is the spread times 100 taken modulo, and the angle is that count over 100 times two
// pi.
Vector RandomDirection(Vector vDir, float fSpread)
{
    int n = rand_wincompatible() % static_cast<int>(fSpread * 100.0f);
    Quaternion q(Vector(0.0f, 0.0f, 1.0f), (n / 100.0f) * 2.0f * 3.14159274f);
    return RotateVector(vDir, q);
}


// The model hierarchy's procedural pass, ticked from Gob::Animate for a gob whose flag byte is set. It
// walks the part tree and, per mesh, scrolls the part's two-floats-per-frame offset stream by the node's
// UV direction, jitters a locked pool around it and copies that pool into the model's animated-vertex pool
// at the node's own stride and offset.
//
// The pool id is read twice, and the second read lands after the trimesh downcast because that call
// may change it. The texture test is written as two parts on purpose, so the first-character check
// is emitted before the substring search.
#if KOTOR1
// K1: two hidden tunables gate the whole walk, an enable byte and a model-size limit past which neither
// this part nor its children animate; no texture special case; the geometry pool is locked whole and the
// node's geometry base added by hand.
__attribute__((visibility("hidden"))) bool g_bProceduralAnimate = true;
__attribute__((visibility("hidden"))) unsigned int g_nProceduralAnimateLimit = 35000;
#endif
void ProceduralAnimateHierarchy(Part* pPart)
{
#if KOTOR1
    if (!g_bProceduralAnimate)
        return;
#endif
    PartTriMesh* pMesh = pPart->AsPartTriMesh();
    if (pMesh != 0 && pMesh->m_nUVPool != -1)
    {
        MdlNodeTriMesh* pNode = pMesh->m_pNode->AsMdlNodeTriMesh();
        if (pNode->m_nAnimateUV != 0 && pNode->m_nTex0Ofs != -1 && pMesh->m_nUVPool != -1)
        {
#if KOTOR1
            if (static_cast<unsigned int>(pNode->m_pTree->AsModel()->m_nPoolSize) > g_nProceduralAnimateLimit)
                return;
#endif
            float fJitter = pNode->m_fUVJitter;
            float* pfPool = static_cast<float*>(GLRender::LockPool(pMesh->m_nUVPool, 0, 0, 16));

            float fX = pMesh->m_pafBaseUVs[0];
            float fY = pMesh->m_pafBaseUVs[1];
            float fOffX = static_cast<float>(static_cast<int>(fX));
            float fOffY = static_cast<float>(static_cast<int>(fY));
#if !KOTOR1
            if (pNode->m_szTexture[0] == 'L' && strstr(pNode->m_szTexture, "LEH_hyper01") != 0)
            {
                fOffX = fX > 25.0f ? 25.0f : 0.0f;
                fOffY = fX > 25.0f ? 200.0f : 0.0f;
            }
#endif

            // The jitter is read once, before the loop, because a read of the member inside the body cannot
            // be hoisted past the calls that may write through the node. The half-jitter and the jitter count
            // are pure computations of that value and belong inside the body.
#if KOTOR1
            int nJitter = static_cast<int>(fJitter * 1000.0f);
            float fHalf = fJitter * 0.5f;
#endif
            for (int64_t i = 0; i < static_cast<short>(pNode->m_nVertexCount); i++)
            {
#if !KOTOR1
                float fHalf = fJitter * 0.5f;
                int nJitter = static_cast<int>(fJitter * 1000.0f);
#endif
                float* pfPos = &pMesh->m_pafBaseUVs[i * 2];

                pfPos[0] += pNode->m_fUVDirectionX;
                pfPos[1] += pNode->m_fUVDirectionY;
                if (nJitter != 0)
                {
                    pfPool[i * 2] += pNode->m_fUVJitterSpeed *
                                     (static_cast<float>(rand_wincompatible() % nJitter) * 0.001f - fHalf);
                    if (pfPool[i * 2] - pfPos[0] > fHalf)
                        pfPool[i * 2] = pfPos[0] + fHalf;
                    else if (pfPool[i * 2] - pfPos[0] < -fHalf)
                        pfPool[i * 2] = pfPos[0] - fHalf;

                    pfPool[i * 2 + 1] += pNode->m_fUVJitterSpeed *
                                         (static_cast<float>(rand_wincompatible() % nJitter) * 0.001f - fHalf);
                    if (pfPool[i * 2 + 1] - pfPos[1] > fHalf)
                        pfPool[i * 2 + 1] = pfPos[1] + fHalf;
                    else if (pfPool[i * 2 + 1] - pfPos[1] < -fHalf)
                        pfPool[i * 2 + 1] = pfPos[1] - fHalf;
                }
                pfPos[0] -= fOffX;
                pfPos[1] -= fOffY;
            }

#if KOTOR1
            char* pDst = static_cast<char*>(GLRender::LockPool(pNode->m_pTree->AsModel()->m_nPool, 0, 0, 1));
            for (int64_t i = 0; i < static_cast<short>(pNode->m_nVertexCount); i++)
                *reinterpret_cast<unsigned long long*>(pDst + pNode->m_nGeomBase + pNode->m_nVertexStride * static_cast<int>(i) +
                                       pNode->m_nTex0Ofs)
                    = reinterpret_cast<unsigned long long*>(pfPool)[i];
#else
            char* pDst = static_cast<char*>(GLRender::LockPool(pNode->m_pTree->AsModel()->m_nPool,
                                                   pNode->m_nGeomBase,
                                                   pNode->m_nVertexStride * static_cast<short>(pNode->m_nVertexCount), 1));
            for (int64_t i = 0; i < static_cast<short>(pNode->m_nVertexCount); i++)
                *reinterpret_cast<unsigned long long*>(pDst + pNode->m_nVertexStride * i + pNode->m_nTex0Ofs)
                    = reinterpret_cast<unsigned long long*>(pfPool)[i];
#endif

            GLRender::UnlockPool(pNode->m_pTree->AsModel()->m_nPool);
            GLRender::UnlockPool(pMesh->m_nUVPool);
        }
    }

    for (int64_t i = 0; i < pPart->m_nChildren; i++)
        ProceduralAnimateHierarchy(pPart->m_paChildren[i]);
}

extern int   depth;
extern int  realmaxdepth;
extern int  maxdepth;
extern int  checkdepth;
extern int  animdepth1;
extern int  animdepth2;
extern float animdepthdist1;
extern float animdepthdist2;

// Applies a model node's pose to a part and then descends: for every child part it looks for the model
// node whose id matches the part's and recurses. Gob::Reset uses the same walker to snap a tree back to
// its rest pose.
//
// The depth counters are the walk's own guards. The flag is both read and written. The
// node-is-the-model-root test ORs into the flag passed in and the updated flag is passed down, so
// the fold must be a bitwise or rather than a short-circuit one.
void AnimateHierarchy(Part* pPart, MdlNode* pNode, float fA, float fB, float fC, bool bD)
{
    if (depth++ >= realmaxdepth)
        realmaxdepth = depth;

    if (!bD && pPart != 0)
        bD = bD | (pPart->m_pNode == pPart->m_pNode->m_pTree->AsModel()->m_pAnimationRoot);

    if (pPart->m_nLocalPositionSet == 0 && bD)
        pNode->ControlDispatch(pPart, fA, fB, fC);

    int nMaxDepth = 100;
    Gob* pGob = pPart->m_pGob;
    if (pGob != 0 && checkdepth != 0 && !pGob->IsInCutscene() && pGob->m_bAnimatedOnce)
    {
        float fSize = pGob->m_fMinSphereRadius;
        if (fSize < 0.5f)
            fSize = 0.5f;
        float fRatio = fSize / 1.6f;
        if (pGob->m_fCheckEventTime > animdepthdist2 * fRatio)
            nMaxDepth = animdepth2;
        else if (pGob->m_fCheckEventTime > animdepthdist1 * fRatio)
            nMaxDepth = animdepth1;
    }

    if (depth < nMaxDepth && depth < maxdepth)
    {
        for (int i = 0; i < pPart->m_nChildren; i++)
        {
            Part* pChild = pPart->m_paChildren[i];
            if (pChild->m_nNodeID != -1)
            {
                for (int j = 0; j < pNode->m_listChildren.m_count; j++)
                {
                    MdlNode* pNodeChild = pNode->m_listChildren.m_pData[(i + j) % pNode->m_listChildren.m_count];
                    if (pChild->m_nNodeID == pNodeChild->m_nNodeID)
                    {
                        AnimateHierarchy(pChild, pNodeChild, fA, fB, fC, bD);
                        break;
                    }
                }
            }
        }
    }

    depth--;
}

// One animation controller attached to a model node, with the record layout and the key readers.
//
// The record is 16 bytes, holding a 32-bit target field offset in the part, a 16-bit second-offset
// field, all ones meaning none, a 16-bit key count, a 16-bit first key-time index in the node's
// float list, a 16-bit value index, an 8-bit kind whose low nibble is the value kind and whose high
// nibble marks a bezier controller, and an unused 16-bit field. The node's float list holds, per
// controller, the key times starting at the time index and then the key values starting at the
// value index: three floats per key for a plain vector, four for a quaternion, one for a float and
// nine for a bezier vector.
Quaternion slerp(Quaternion qA, const Quaternion& qB, float fT);
Vector InterpolateBezier(const Vector& a, const Vector& b, const Vector& c,
                        const Vector& d, float fT);
float InterpolateBezier(float a, float b, float c, float d, float fT);

// The model's 32-bit to 64-bit field-offset lookup.
int GetOffset64(int nOffset, int nType);


extern float position_scale_factor;

extern float DeltaT;

// The key search the readers share. The index ends as the number of leading key times below the
// requested one, 0 at or before the first key and the key count past the last, and the alpha as the
// position inside the span it lands in, 1.0 when there is no span to interpolate.
void NewController::GetKeyIndexAndAlpha(float fTime, MdlNode* pNode, int& nIndex, float& fAlpha)
{
    nIndex = 0;
    fAlpha = 1.0f;

    unsigned short nCount = m_target.m_nKeyCount;
    if (nCount != 0) {
        unsigned short nTimeIndex = m_keys.m_nTimeIndex;
        float* pafValues = pNode->m_listFloats.m_pData;

        uint64_t i = 0;
        while (i < nCount && pafValues[nTimeIndex + i] < fTime) {
            i++;
            nIndex = static_cast<int>(i);
        }
        if (i != 0 && i != nCount) {
            float fLow = pafValues[m_keys.m_nTimeIndex + static_cast<int>(i) - 1];
            float fHigh = pafValues[m_keys.m_nTimeIndex + static_cast<int>(i)];
            fAlpha = (fTime - fLow) / (fHigh - fLow);
        }
    }
}

// Decodes one key's orientation, in two on-disk forms:
//
// * kind 2: a packed 32-bit rotation, three fixed-point components in one unsigned int, each mapped from
//   its bit field by 2 * field / (2046 or 1022) - 1, with the fourth component recovered from unit length
// * kind 4: four plain floats, rotated: the file stores (x, y, z, w) while this engine's quaternion is
//   (w, x, y, z), so the stores land one slot round
void NewController::GetQuaternionFromIndexLocation(int nKey, MdlNode* pNode, Quaternion& qValue)
{
    switch (m_keys.m_nKind & kKeyLayoutMask) {
    case kKeyQuaternion: {
        if (nKey < 2)
            nKey = 1;
        float* pafKey = &pNode->m_listFloats.m_pData[m_keys.m_nValueIndex + static_cast<int64_t>(nKey * 4 - 4)];
        qValue.x = pafKey[0];
        qValue.y = pafKey[1];
        qValue.z = pafKey[2];
        qValue.w = pafKey[3];
        break;
    }
    case kKeyPackedQuaternion: {
        if (nKey < 2)
            nKey = 1;
        unsigned int nPacked = *reinterpret_cast<unsigned int*>(&pNode->m_listFloats.m_pData[m_keys.m_nValueIndex + nKey - 1]);

        // The packed components are signed ints, even though the mask makes them non-negative.
        int n0 = nPacked & 0x7ff;
        int n1 = (nPacked >> 11) & 0x7ff;
        int n2 = nPacked >> 22;
        float fX = static_cast<float>(n0) * 2.0f / 2046.0f - 1.0f;
        float fY = static_cast<float>(n1) * 2.0f / 2046.0f - 1.0f;
        float fZ = static_cast<float>(n2) * 2.0f / 1022.0f - 1.0f;
        qValue.x = fX;
        qValue.y = fY;
        qValue.z = fZ;

        float fSq = fX * fX + fY * fY + fZ * fZ;
        if (fSq <= 1.0f) {
            qValue.w = sqrtf(1.0f - fSq);
        } else {
#if KOTOR1  // K1: each component is divided by the length, with no reciprocal
            float fLen = sqrtf(fSq);
            qValue.x = fX / fLen;
            qValue.y = fY / fLen;
            qValue.z = fZ / fLen;
#else
            float fInv = 1.0f / sqrtf(fSq);
            qValue.x = fX * fInv;
            qValue.y = fY * fInv;
            qValue.z = fZ * fInv;
#endif
            qValue.w = 0.0f;
        }
        break;
    }
    }
}

// The orientation reader. Kinds 2 and 4 are the only ones carrying an orientation and both go through the
// index-location decoder; a time landing between two keys is a slerp of the two.
int NewController::GetQuaternionValue(float fTime, MdlNode* pNode, Quaternion& qValue)
{
    int nKind = m_keys.m_nKind & kKeyLayoutMask;
    if (nKind != 4 && nKind != 2)
        return false;

    int nIndex;
    float fAlpha;
    GetKeyIndexAndAlpha(fTime, pNode, nIndex, fAlpha);

    static Quaternion qFirst;
    static Quaternion qSecond;

    if (nIndex == 0) {
        GetQuaternionFromIndexLocation(0, pNode, qValue);
    } else if (nIndex != m_target.m_nKeyCount) {
        GetQuaternionFromIndexLocation(nIndex, pNode, qFirst);
        GetQuaternionFromIndexLocation(nIndex + 1, pNode, qSecond);
        qValue = Interpolate(qFirst, qSecond, fAlpha);
    } else {
        GetQuaternionFromIndexLocation(nIndex, pNode, qValue);
    }
    return true;
}

// Reads a scalar, kind 1. One float per key, times first, so the key at index i owns the time at
// the time index plus i and the value at the value index plus i. The bezier arm has a three-float
// stride, and its two control points are stored as deltas from the following key.
bool NewController::GetFloatValue(float fTime, MdlNode* pNode, float& fValue)
{
    if ((m_keys.m_nKind & kKeyLayoutMask) != kKeyFloat)
        return false;

    unsigned short nCount = m_target.m_nKeyCount;
    if (nCount == 0) {
        fValue = pNode->m_listFloats.m_pData[m_keys.m_nValueIndex];
        return true;
    }

    unsigned short nTimeIndex = m_keys.m_nTimeIndex;
    if (!(pNode->m_listFloats.m_pData[nTimeIndex] < fTime)) {
        fValue = pNode->m_listFloats.m_pData[m_keys.m_nValueIndex];
        return true;
    }

    int nIndex;
    float fAlpha;
    GetKeyIndexAndAlpha(fTime, pNode, nIndex, fAlpha);

    // Bezier controllers are marked by a bit in the kind's high nibble, as in the vector reader.
    if (nIndex == nCount) {
        if ((m_keys.m_nKind & kKeyFlagsMask) == kKeyBezier)
            fValue = pNode->m_listFloats.m_pData[m_keys.m_nValueIndex + 3 * (nCount - 1)];
        else
            fValue = pNode->m_listFloats.m_pData[m_keys.m_nValueIndex + nCount - 1];
        return true;
    }
    if ((m_keys.m_nKind & kKeyFlagsMask) == kKeyBezier) {
        fValue = InterpolateBezier(pNode->m_listFloats.m_pData[m_keys.m_nValueIndex + 3 * nIndex - 3],
                                  pNode->m_listFloats.m_pData[m_keys.m_nValueIndex + 3 * nIndex - 1],
                                  pNode->m_listFloats.m_pData[m_keys.m_nValueIndex + 3 * nIndex - 3] +
                                      pNode->m_listFloats.m_pData[m_keys.m_nValueIndex + 3 * nIndex],
                                  pNode->m_listFloats.m_pData[m_keys.m_nValueIndex + 3 * nIndex - 1] +
                                      pNode->m_listFloats.m_pData[m_keys.m_nValueIndex + 3 * nIndex + 1],
                                  fAlpha);
    } else {
        fValue = (1.0f - fAlpha) * pNode->m_listFloats.m_pData[m_keys.m_nValueIndex + nIndex - 1] +
                 fAlpha * pNode->m_listFloats.m_pData[m_keys.m_nValueIndex + nIndex];
    }
    return true;
}

// The vector reader, kind 3. Same key search as its siblings, then three destinations:
//
// * index 0: the first key's point, at the value index
// * index equal to the key count: the last key's point, stride 9 for a bezier controller and 3 otherwise
// * in between: for a bezier controller the four-point interpolation, where a bezier key is nine floats,
//   and otherwise a componentwise lerp
//
// Every result is scaled by the trace scale factor when the controller drives the part's position field;
// other fields use 1.0f.
int NewController::GetVectorValue(float fTime, MdlNode* pNode, Vector& vValue)
{
    if ((m_keys.m_nKind & kKeyLayoutMask) != kKeyVector)
        return false;

    int nIndex;
    float fAlpha;
    GetKeyIndexAndAlpha(fTime, pNode, nIndex, fAlpha);

    static Vector vFirst;
    static Vector vSecond;

    float fScale = (m_target.m_nOffset == kTargetPosition) ? position_scale_factor : 1.0f;

    if (nIndex == 0) {
        float* pafValues = pNode->m_listFloats.m_pData;
        vValue.x = pafValues[m_keys.m_nValueIndex];
        vValue.y = pafValues[m_keys.m_nValueIndex + 1];
        vValue.z = pafValues[m_keys.m_nValueIndex + 2];
    } else if ((m_keys.m_nKind & kKeyFlagsMask) == kKeyBezier) {
        if (nIndex == m_target.m_nKeyCount) {
            float* pafValues = pNode->m_listFloats.m_pData;
            vValue.x = pafValues[m_keys.m_nValueIndex + 9 * nIndex - 9];
            vValue.y = pafValues[m_keys.m_nValueIndex + 9 * nIndex - 8];
            vValue.z = pafValues[m_keys.m_nValueIndex + 9 * nIndex - 7];
        } else {
            float* pafValues = pNode->m_listFloats.m_pData;
            vFirst.x = pafValues[m_keys.m_nValueIndex + 9 * nIndex - 9];
            vFirst.y = pafValues[m_keys.m_nValueIndex + 9 * nIndex - 8];
            vFirst.z = pafValues[m_keys.m_nValueIndex + 9 * nIndex - 7];
            vSecond.x = pafValues[m_keys.m_nValueIndex + 9 * nIndex];
            vSecond.y = pafValues[m_keys.m_nValueIndex + 9 * nIndex + 1];
            vSecond.z = pafValues[m_keys.m_nValueIndex + 9 * nIndex + 2];
            vValue = InterpolateBezier(vFirst, vSecond,
                                       vFirst + Vector(pafValues[m_keys.m_nValueIndex + 9 * nIndex - 3],
                                                       pafValues[m_keys.m_nValueIndex + 9 * nIndex - 2],
                                                       pafValues[m_keys.m_nValueIndex + 9 * nIndex - 1]),
                                       vSecond + Vector(pafValues[m_keys.m_nValueIndex + 9 * nIndex + 3],
                                                        pafValues[m_keys.m_nValueIndex + 9 * nIndex + 4],
                                                        pafValues[m_keys.m_nValueIndex + 9 * nIndex + 5]),
                                       fAlpha);
            vValue = vValue * fScale;
            return true;
        }
    } else {
        if (nIndex == m_target.m_nKeyCount) {
            float* pafValues = pNode->m_listFloats.m_pData;
            vValue.x = pafValues[m_keys.m_nValueIndex + 3 * nIndex - 3];
            vValue.y = pafValues[m_keys.m_nValueIndex + 3 * nIndex - 2];
            vValue.z = pafValues[m_keys.m_nValueIndex + 3 * nIndex - 1];
        } else {
            float* pafValues = pNode->m_listFloats.m_pData;
            vFirst.x = pafValues[m_keys.m_nValueIndex + 3 * nIndex - 3];
            vFirst.y = pafValues[m_keys.m_nValueIndex + 3 * nIndex - 2];
            vFirst.z = pafValues[m_keys.m_nValueIndex + 3 * nIndex - 1];
            vSecond.x = pafValues[m_keys.m_nValueIndex + 3 * nIndex];
            vSecond.y = pafValues[m_keys.m_nValueIndex + 3 * nIndex + 1];
            vSecond.z = pafValues[m_keys.m_nValueIndex + 3 * nIndex + 2];
            vValue = Interpolate(vFirst, vSecond, fAlpha);
            vValue = vValue * fScale;
            return true;
        }
    }

    vValue = vValue * fScale;
    return true;
}

// Reports one traced key. It samples the controller's own value at the key's time, then walks the
// node's controller list for the complementary one, a vector controller for the quaternion at the
// node's orientation field and a quaternion controller for the vector at the position field, and
// hands both to the owning gob's event sink. The report is a stack payload whose quaternion flag
// says which of the two sampled values the event should use; when the complementary controller is
// absent the quaternion falls back to the part's own local orientation.
void NewController::ReportKey(Part* pPart, MdlNode* pNode, int nKey, float fAlpha)
{
    float fTime = pNode->m_listFloats.m_pData[m_keys.m_nTimeIndex + nKey];
    Gob* pGob = pPart->m_pGob;

    Quaternion qValue = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
    Vector vValue = Vector(0.0f, 0.0f, 0.0f);
    NewControllerKeyReport report;
    report.m_pVector = 0;
    report.m_pQuaternion = 0;
    report.m_bQuaternion = true;
    report.m_fAlpha = fAlpha;

    int nKind = m_keys.m_nKind & kKeyLayoutMask;
    if (nKind == kKeyPackedQuaternion || nKind == kKeyQuaternion) {
        GetQuaternionValue(fTime, pNode, qValue);
        report.m_pQuaternion = &qValue;
        report.m_bQuaternion = false;
        for (int i = 0; i < pNode->m_listControllers.m_count; i++) {
            if (&pNode->m_listControllers.m_pData[i] == this)
                continue;
            if (pNode->m_listControllers.m_pData[i].m_target.m_nOffset != kTargetPosition)
                continue;
            // The sibling samples itself, not this controller.
            if (pNode->m_listControllers.m_pData[i].GetVectorValue(fTime, pNode, vValue)) {
                report.m_pVector = &vValue;
                break;
            }
        }
    } else if (nKind == kKeyVector) {
        GetVectorValue(fTime, pNode, vValue);
        report.m_pVector = &vValue;
        for (int i = 0; i < pNode->m_listControllers.m_count; i++) {
            if (&pNode->m_listControllers.m_pData[i] == this)
                continue;
            if (pNode->m_listControllers.m_pData[i].m_target.m_nOffset != kTargetOrientation)
                continue;
            if (pNode->m_listControllers.m_pData[i].GetQuaternionValue(fTime, pNode, qValue)) {
                report.m_pQuaternion = &qValue;
                break;
            }
        }
        if (report.m_pQuaternion == 0) {
            qValue = pPart->m_qLocalOrientation;
            report.m_pQuaternion = &qValue;
        }
    } else {
        // Any other kind reports nothing.
        return;
    }

    pGob->DoEvent(const_cast<char*>("ReportKey"), &report);
}

// Runs one control tick, dispatched on the record's low-nibble kind. 2 and 4 carry an orientation,
// 1 a scalar and 3 a vector. All three arms share one shape:
//
// 1. resolve the offset; when its top nibble is set the field lives behind a 64-bit lookup keyed on the
//    node's kind word
// 2. hold the destination's current value as the blend's "from"
// 3. sample the controller at the current time and, unless the previous time is below the "no previous
//    frame" sentinel the gob passes, sample again there and use that as the "from"
// 4. blend by the alpha, slerp for the quaternion and componentwise lerp for the other two, and store
//    back through the resolved offset
//
// The second offset adds the node's own value at that offset, so a controller on a child node animates
// relative to its parent's contribution. That applies to the scalar and vector arms only; the quaternion
// arm rebuilds from the node's own rotation.
//
// The scalar arm's file statics are file-visible, which is why the sampled value is a global rather than a
// local: it has to survive the second-offset fix-up. Writing the scale field also notifies the mesh: for a
// trimesh whose model carries the scale-animated flag, the record's own render byte is set.
void NewController::Control(Part* pPart, MdlNode* pNode, float fPrevTime, float fTime, float fAlpha)
{
    switch (m_keys.m_nKind & kKeyLayoutMask) {
        case kKeyPackedQuaternion:
        case kKeyQuaternion: {
            Quaternion qPrev(1.0f, 0.0f, 0.0f, 0.0f);
            Quaternion qValue(1.0f, 0.0f, 0.0f, 0.0f);
            if (m_target.m_nOffset & kTargetUnconverted) {
                short nKind = pPart->m_pNode->m_nFlags;
                m_target.m_nOffset &= kTargetOffsetMask;
                m_target.m_nOffset = GetOffset64(m_target.m_nOffset, nKind);
            }
            Quaternion* pDest = reinterpret_cast<Quaternion*>(reinterpret_cast<char*>(pPart) + m_target.m_nOffset);  // raw-offset: the controller's target offset comes from the animation data
            // The destination's current value lives in its own local and the sampled previous value is copied
            // into it, never the reverse.
            Quaternion qFrom = *pDest;
            GetQuaternionValue(fTime, pNode, qValue);
            if (!(fPrevTime < -95.0f)) {
                GetQuaternionValue(fPrevTime, pNode, qPrev);
                qFrom = qPrev;
            }
            *pDest = Interpolate(qFrom, qValue, fAlpha);
            break;
        }
        case kKeyFloat: {
            // The three statics are what lets the sampled value survive the second-offset fix-up below.
            static float value_old;
            static float value0;
            static float value;
            if (m_target.m_nOffset & kTargetUnconverted) {
                short nKind = pPart->m_pNode->m_nFlags;
                m_target.m_nOffset &= kTargetOffsetMask;
                m_target.m_nOffset = GetOffset64(m_target.m_nOffset, nKind);
            }
            float* pDest = reinterpret_cast<float*>(reinterpret_cast<char*>(pPart) + m_target.m_nOffset);  // raw-offset: the controller's target offset comes from the animation data
            value_old = *pDest;
            GetFloatValue(fTime, pNode, value);
            if (m_target.m_nDefaultOffset != -1)
                value += *reinterpret_cast<float*>(reinterpret_cast<char*>(pPart->m_pNode) + m_target.m_nDefaultOffset);  // raw-offset: the controller's target offset comes from the animation data
            float* pFrom;
            if (fPrevTime < -95.0f) {
                pFrom = &value_old;
            } else {
                GetFloatValue(fPrevTime, pNode, value0);
                pFrom = &value0;
            }
            *pDest = (1.0f - fAlpha) * *pFrom + fAlpha * value;
#if !KOTOR1  // K1: no re-resolve and no mesh render-flag update after the store
            if (m_target.m_nOffset & kTargetUnconverted) {
                short nKind = pPart->m_pNode->m_nFlags;
                m_target.m_nOffset &= kTargetOffsetMask;
                m_target.m_nOffset = GetOffset64(m_target.m_nOffset, nKind);
            }
            if (m_target.m_nOffset == kTargetScale) {
                MdlNodeTriMesh* pTriMesh = pPart->m_pNode->AsMdlNodeTriMesh();
                if (pTriMesh != 0) {
                    Model* pModel = pTriMesh->m_pTree->AsModel();
                    if (pModel != 0 && (pModel->m_nFlags & Model::kModelLightsaber) != 0)
                        pTriMesh->m_bRender = true;
                }
            }
#endif
            break;
        }
        case kKeyVector: {
            Vector vPrev(0.0f, 0.0f, 0.0f);
            Vector vValue(0.0f, 0.0f, 0.0f);
            if (m_target.m_nOffset & kTargetUnconverted) {
                short nKind = pPart->m_pNode->m_nFlags;
                m_target.m_nOffset &= kTargetOffsetMask;
                m_target.m_nOffset = GetOffset64(m_target.m_nOffset, nKind);
            }
            Vector* pDest = reinterpret_cast<Vector*>(reinterpret_cast<char*>(pPart) + m_target.m_nOffset);  // raw-offset: the controller's target offset comes from the animation data
            Vector vOld = *pDest;
            GetVectorValue(fTime, pNode, vValue);
            Vector* pBase K2_UNSET;
            if (m_target.m_nDefaultOffset != -1) {
                pBase = reinterpret_cast<Vector*>(reinterpret_cast<char*>(pPart->m_pNode) + m_target.m_nDefaultOffset);  // raw-offset: the controller's target offset comes from the animation data
                vValue.x = pBase->x + vValue.x;
                vValue.y = pBase->y + vValue.y;
                vValue.z = pBase->z + vValue.z;
            }
            if (fPrevTime < -95.0f) {
                *pDest = vOld * (1.0f - fAlpha) + vValue * fAlpha;
            } else {
                GetVectorValue(fPrevTime, pNode, vPrev);
                if (m_target.m_nDefaultOffset != -1)
                {
                    vPrev.x = pBase->x + vPrev.x;
                    vPrev.y = pBase->y + vPrev.y;
                    vPrev.z = pBase->z + vPrev.z;
                }
                *pDest = vPrev * (1.0f - fAlpha) + vValue * fAlpha;
            }
            break;
        }
    }
}

// The tracer's entry point, the other half of the control pair. It does the same key walk, but
// every key is reported rather than applied to the part. The two float arguments are a time
// interval, and each key reported carries the duration of the span it owns; the payload's alpha
// ends as what is left of the frame.
//
// The start time is clamped up to 0 and the end time down to the run length. When the start is at or past
// the end the span wraps: the tail from the start to the last key, then the head from the run's start back
// up to the end, with the closing duration spanning the wrap.
void NewController::ReportKeys(Part* pPart, MdlNode* pNode, float fTimeA, float fTimeB)
{
    if (!pPart->m_pGob)
        return;

    unsigned short nTimeIndex = m_keys.m_nTimeIndex;
    // Gob::Animate publishes the traced run's tree length here.
    float fLen = AnimRun::m_AnimLength;
    float fStart = fTimeA;
    if (fStart < 0.0f)
        fStart = 0.0f;
    float fEnd = (fLen < fTimeB) ? fLen : fTimeB;

    NewControllerKeyReport report;
    report.m_pQuaternion = 0;
    report.m_pVector = 0;
    report.m_bQuaternion = true;

    unsigned short nKeyCount = m_target.m_nKeyCount;
    int nIndexA = 0;
    int nIndexB = 0;
    if (nKeyCount != 0) {
        float* pafTimes = &pNode->m_listFloats.m_pData[nTimeIndex];
        uint64_t i = 0;
        while (i < nKeyCount && pafTimes[i] < fStart)
            i++;
        nIndexA = static_cast<int>(i);
    }
    if (nKeyCount != 0) {
        float* pafTimes = &pNode->m_listFloats.m_pData[nTimeIndex];
        uint64_t i = 0;
        while (i < nKeyCount && pafTimes[i] < fEnd)
            i++;
        nIndexB = static_cast<int>(i);
    }

    // The run's direction flag, a 32-bit int read as its low byte.
    if (static_cast<unsigned char>(AnimRun::m_AnimFlags) & 1)
        return;

    float fTotal = 0.0f;

    if (fStart < fEnd) {
        // For a forward span, every key strictly inside the interval is reported as the duration of
        // the span it owns, the start to the first key and then key to key.
        if (nIndexA < nIndexB) {
            for (int i = 0; i != nIndexB - nIndexA; i++) {
                int nKey = nIndexA + i;
                float fCur = pNode->m_listFloats.m_pData[nTimeIndex + nKey];
                float fPrev = (i == 0)
                                  ? fStart
                                  : pNode->m_listFloats.m_pData[nTimeIndex + nKey - 1];
                float fDuration = fCur - fPrev;
                ReportKey(pPart, pNode, nKey, fDuration);
                fTotal += fDuration;
            }
        }
    } else {
        // For a wrapped span, the tail from the start to the end of the run, then the head back up
        // to the end. The closing duration spans the wrap, so the last key's time is carried across
        // it.
        float fLast = (nIndexA != nKeyCount)
                          ? pNode->m_listFloats.m_pData[nTimeIndex + nKeyCount - 1]
                          : fStart;
        for (int nKey = nIndexA; nKey < m_target.m_nKeyCount; nKey++) {
            float fCur;
            float fPrev;
            if (nKey == nIndexA) {
                fCur = pNode->m_listFloats.m_pData[nIndexA];
                fPrev = fStart;
            } else {
                fCur = pNode->m_listFloats.m_pData[nTimeIndex + nKey];
                fPrev = pNode->m_listFloats.m_pData[nTimeIndex + nKey - 1];
            }
            float fDuration = fCur - fPrev;
            ReportKey(pPart, pNode, nKey, fDuration);
            fTotal += fDuration;
        }
        if (nIndexB != 0) {
            for (int i = 0; i != nIndexB; i++) {
                float fDuration;
                if (i == 0)
                    fDuration = (fLen - fLast) + pNode->m_listFloats.m_pData[nTimeIndex];
                else
                    fDuration = pNode->m_listFloats.m_pData[nTimeIndex + i]
                              - pNode->m_listFloats.m_pData[nTimeIndex + i - 1];
                ReportKey(pPart, pNode, i, fDuration);
                fTotal += fDuration;
            }
        }
    }

    report.m_fAlpha = DeltaT - fTotal;
    report.m_pQuaternion = 0;
    report.m_pVector = 0;
    report.m_bQuaternion = (m_keys.m_nKind & kKeyLayoutMask) == kKeyVector;

    pPart->m_pGob->DoEvent(const_cast<char*>("ReportKey"), &report);
}
