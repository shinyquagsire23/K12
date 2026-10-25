// AURORA scene-graph nodes, model parts and the .mdl loaders.
#include "k2stdint.h"
#include "port/aslglapi.h"
#include "aurres.h"
#define AURPARSE_HELPERS_OUT_OF_LINE  // the helpers are defined in aurmodelparse.cpp
#ifdef K2_HOST_BUILD
#define AABB_DTOR_HEADER_BODY   // as aurmodelparse.cpp's face destructors
#else
#define AABB_DTOR_DECLARED_ONLY noexcept
#endif
#define MAXTREE_EMIT_OUT_OF_LINE

#if KOTOR1
#define K1_AURORA_LIST  // K1's List layout
#endif
#include "AURORA/aurmodel.h"
#include "nullname.h"
#include "aurscene.h"
#include "auranim.h"
#include "auremitter.h"
#include "include/aurbehavior.h"
#include "AURORA/aurbehavior.h"
#include "List.h"
#include "aurtexman.h"
#include "aurmaterial.h"


#include "aurglcore.h"
#include "aurtrigger.h"
#include "PointerMap.h"
#include "aurparse.h"
#include "aurmodelrender.h"
#include "aurprimitive.h"
#include <cmath>
#include "aurio.h"
#include "Vector.h"
#include <cstddef>
#include <map>
#if KOTOR1  // K1 memoises minimum spheres in this map
#include <unordered_map>
#endif

int countmaxfaces;
int alwaysdominspheres;

const char MaxTree::MAXTREE_BASE_ID = 1;
const char MaxTree::MAXTREE_MODEL_ID = 2;
const char MaxTree::MAXTREE_ANIMATION_ID = 5;
const int TileData::FADE_NOT_CHECKED = 4;
int countpart;
int counttrimesh;
char* hitpartname;

int PartTriMesh::m_nNextPartID = 1;
bool PartTriMesh::m_bMaterialHasChanged = true;
unsigned short PartTriMesh::m_VertexIndices[4096];

void GetAllNamedTextures(Part* pPart, List<CAurTexture*>& out, char* pszName);
int  AurTextureSetAttributes(CAurTexture* pTexture, char* pszAttributes);  // returns 1 on success

void PartSetFog(Part* pPart, bool bFog);
int  PartSetCanDownSample(Part* pPart, char c);
Part* FindPart(Part* pRoot, char* pszName);

Animation* FindAnimation(Model* pModel, char* pszName);
Model* FindModel(char* pszName);
#if KOTOR1  // K1 streams models through SDL_RWops
#include "sdlrwops.h"
Model* LoadModel(SDL_RWops* pFile, SDL_RWops* pAux);
#else
Model* LoadModel(FILE* pFile, FILE* pAux);
#endif

Part* CreateInstanceTreeR(MdlNode* pNode, Gob* pGob);
void  PartInitMaterial(Gob* pGob, Part* pPart, char* pszName);
void  CreateReferenceObjects(Gob* pGob, Part* pPart);

#if KOTOR1  // K1 streams models through SDL_RWops
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif

#include "port/crtapi.h"

void AnimateHierarchy(Part* pPart, MdlNode* pNode, float a, float b, float c, bool d);
extern float position_scale_factor;

extern int removedelref;

// A second definition of MaxTree's tree-delete hook, kept here so the callers in this file can see
// its body.

#if KOTOR1  // K1 defines the port's operator delete; K2 defines foobar
void MaxTree::operator delete(void* pTree)
#else
void MaxTree::foobar(void* pTree)
#endif
{
    if (pTree == 0)
        return;
    MaxTree* p = static_cast<MaxTree*>(pTree);
    if (p->m_cType < 0) {
        p->FreeTheFreeList();
        Model* pModel = static_cast<Model*>(p);
        if (pModel->m_pSuperModel != 0) {
            if (removedelref == 0) {
                if (--pModel->m_pSuperModel->m_nRefCount == 0)
                    p->foobar(pModel->m_pSuperModel);
            }
            pModel->m_pSuperModel = 0;
        }
        p->m_pfnDestructor(p);
        Model* pAsModel = (p->m_cType & kTreeTypeMask) == kTreeModel ? pModel : 0;
#ifdef K2_FIX_ENGINE_BUGS
        if (pAsModel != 0 && pAsModel->m_pMdxResource != 0)
#else
        if (pAsModel->m_pMdxResource != 0)
#endif
            AurResFree(pAsModel->m_pMdxResource, 0);
        PointerMap::s_pointerMap.DestroyMemoryBlock(p);
        return;
    }
    if (p->m_cType == 0)
        return;
    p->m_pfnDestructor(p);
    MdlNode* pNode = p->m_pMdlNode;
    if (pNode != 0) {
        unsigned short kind = pNode->m_nFlags;
        if ((kind & NODE_LIGHT) == NODE_LIGHT)
            delete static_cast<MdlNodeLight*>(pNode);
        else if ((kind & NODE_CAMERA) == NODE_CAMERA)
            delete pNode;
        else if ((kind & NODE_DANGLYMESH) == NODE_DANGLYMESH)
            delete static_cast<MdlNodeDanglyMesh*>(pNode);
        else if ((kind & NODE_ANIMMESH) == NODE_ANIMMESH)
            delete static_cast<MdlNodeAnimMesh*>(pNode);
        else if ((kind & NODE_EMITTER) == NODE_EMITTER)
            delete pNode;
        else if ((kind & NODE_REFERENCE) == NODE_REFERENCE)
            delete pNode;
        else if ((kind & NODE_AABB) == NODE_AABB)
            delete static_cast<MdlNodeAABB*>(pNode);
        else if ((kind & NODE_SKIN) == NODE_SKIN)
            delete static_cast<MdlNodeSkin*>(pNode);
        else if ((kind & NODE_TRIGGER) == NODE_TRIGGER)
            delete pNode;
        else if ((kind & NODE_SABER) == NODE_SABER)
            delete static_cast<MdlNodeLightsaber*>(pNode);
        else if ((kind & NODE_TRIMESH) == NODE_TRIMESH)
            delete static_cast<MdlNodeTriMesh*>(pNode);
        else
            delete pNode;
    }
    if ((p->m_cType & kTreeTypeMask) == kTreeModel)
        delete static_cast<Model*>(p);
    else if ((p->m_cType & kTreeTypeMask) == kTreeAnimation)
        delete static_cast<Animation*>(p);
}

#include "port/libcbase.h"
#include "port/libcstring.h"

Gob* FindGob(char* pszName);

char* GetToken(char** ppsz);

char* skipspace(char* psz);

void exportparts(Gob* pGob, Part* pPart);

FILE* spewpltfp;

extern int checkevents;

float gobspherescaling = 1.35000002f;

#if KOTOR1  // K1: collision-radius tunable the minigame sets per track
float IosCollisionRadiusMultiplier = 1.0f;
#endif




bool TreeHasSingleTexture(Part* pPart);
bool TreeHasGeometry(Part* pPart);
bool TreeSetGeometryFlags(Part* pPart);
bool TreeHasTransparency(Part* pPart);

extern int enabletransform;

extern int systemmemoryshadows;

extern int quaternionidentityoptimization;

extern float mindooralpha;

int SphereIntersect(const Vector& vFrom, const Vector& vTo, const Vector& vCenter, float fRadius);

extern int counttrans;

void SetBeaming();

extern int rendermaxdepth;
Vector currentmat __attribute__((visibility("hidden")));

CAurObject* VoidToCAurObject(void* p) { return static_cast<CAurObject*>(p); }

inline CAurCamera* Gob::AsCAurCamera() { return 0; }

inline void* Gob::AsCamera() { return 0; }

extern List<Gob*> gobs;
int g_nSpawnedGobs;
int g_nCountOfGobs;
Scene* FindScene(char* pszName);


static void snprintff(char* buf, uint64_t n, const char* fmt, ...);

static int GobFindInsertIdx(char* pszName)
{
    int nLow = 0;
    int nHigh = gobs.m_count - 1;
    while (nLow <= nHigh)
    {
        int nMid = (nHigh + nLow) >> 1;
        int nCmp = _stricmp(gobs.m_pData[nMid]->m_szName, pszName);
        if (nCmp > 0)
            nHigh = nMid - 1;
        else if (nCmp < 0)
            nLow = nMid + 1;
        else
        {
            nLow = nMid;
            nHigh = nMid - 1;
        }
    }
    return nLow;
}

#if !KOTOR1
static void GobInsertAt(int nIdx, Gob* pGob)
{
    // inlined List<Gob*>::Insert(pGob, nIdx)
    if (gobs.m_count == gobs.m_capacity)
    {
        int nNew = gobs.m_capacity ? (gobs.m_capacity << 1) : 16;
        Gob** pOld = gobs.m_pData;
        gobs.m_capacity = nNew;
        gobs.m_pData = new Gob*[nNew];
        K2_ASSUME(gobs.m_count < nNew);   // the list was full at the old capacity, half of nNew
        for (int64_t i = 0; i < gobs.m_count; i++)
            gobs.m_pData[i] = pOld[i];
        delete[] pOld;
    }
    // nIdx is GobFindInsertIndex's answer, 0..m_count, and the list has room for one more.
    K2_ASSUME(nIdx >= 0 && nIdx <= gobs.m_count && gobs.m_count < gobs.m_capacity);
    for (int64_t i = gobs.m_count; i > nIdx; i--)
        gobs.m_pData[i] = gobs.m_pData[i - 1];
    gobs.m_count++;
    gobs.m_pData[nIdx] = pGob;
}
#endif

Gob::Gob(char* pszName)
{
    m_bForceAnimated = false;
    m_bInViewVolume = true;
    m_bAlwaysRendering = false;
    m_bInCutscene = false;
    m_cRenderBumpedOut = 0;
    m_bAnimatedOnce = false;
    m_cForcePose = 1;
    m_nVisible = 1;
    m_fVisibilityCheckTimer = 0.0f;
    m_szName[0] = 0;
    g_nSpawnedGobs++;
    g_nCountOfGobs++;
    if (pszName != 0)
    {
        char szName[64];
        snprintff(szName, 64, "%s", pszName);
        int n = 0;
        do
        {
            if (FindScene(szName) == 0 && FindGob(szName) == 0)
                break;
            n++;
            snprintff(szName, 64, "%s%d", pszName, n);
        } while (1);
        __strcpy_chk(m_szName, szName, 32);
    }

#if !KOTOR1  // the force-glow / hologram / illumination / rendering block
    m_nForcePoints = 0;
    m_nAlignmentValue = 50;
    m_bNonForceBlending = false;
    m_bForceGlowEnabled = false;
    m_fForceGlowR = 0.0f;
    m_fForceGlowG = 0.0f;
    m_fForceGlowB = 0.0f;
    m_bHologramEnabled = false;
    m_bIlluminationEnabled = false;
    m_fIlluminationR = 0.5f;
    m_fIlluminationG = 0.5f;
    m_fIlluminationB = 0.5f;
    m_bRenderingEnabled = true;
#endif

    m_vPosition = Vector(0.0f, 0.0f, 0.0f);
    m_qOrientation = Quaternion();
    m_vPreviousPosition = Vector(0.0f, 0.0f, 0.0f);
    m_qPreviousOrientation = Quaternion();

    m_nDynamicLights = 0;
    m_cCanDownSample = 1;
    m_nShadows = 1;
    m_nColorShifting = 0;
    m_nIllumination = 0;
    m_nRenderGobBBox = 0;
    m_vColorShift = Vector(1.0f, 1.0f, 1.0f);
    m_fColorShiftAmount = 1.0f;
    m_vIllumination = Vector(0.0f, 0.0f, 0.0f);
    m_fPersonalRadius = 0.0f;
    m_fCreatureRadius = 0.0f;

    m_vBoundingBoxMin = Vector(0.0f, 0.0f, 0.0f);
    m_vBoundingBoxMax = Vector(1.0f, 1.0f, 1.0f);
    m_fMinSphereRadius = 1.0f;
    m_vMinSphereCenter = Vector(0.0f, 0.0f, 0.0f);
    m_vGobBBox = Vector(0.0f, 0.0f, 1.0f);
    m_bRenderPersonalSpace = false;
    m_bMotionBlurred = false;
    m_bWind = false;
    m_fWindRadius = 0.0f;
    m_fWindStrength = 0.0f;
    m_fWindLife = 0.0f;
    m_bHasSingleTexture = false;
    m_bReserved1 = 0;
    m_bHasGeometry = false;
    m_bProcessFlag = false;
    m_pSingleMaterial = 0;
    m_pSingleLightPart = 0;
    m_pTileData = 0;
    m_pLookAt = 0;
    m_fMaxBlurLength = 1.5f;
    m_bDistortion = false;
    m_bIgnoreHitCheck = false;
    m_bAnimating = false;
    m_bHasTransparency = false;
    m_fObjectScale = 1.0f;
    m_fObjectScaleBase = 1.0f;

    m_pModel = 0;
    m_pLodModelX = 0;
    m_pLodModelZ = 0;
    m_pAddInModel = 0;
    m_pScene = 0;
    m_pPart = 0;
    m_pLodPartX = 0;
    m_pLodPartZ = 0;

    m_fCheckEventTime = 0.0f;
    m_nImposterHandle = 0;
    m_nImposters = 0;
    m_fFadeAlpha = 1.0f;
    m_bAnimateUV = false;
    m_pBehavior = 0;
    m_fViewDistance = 0.0f;
    m_fBumpOutDistance = 0.0f;
    m_nLastAnimateTick = -1;
    m_nReserved1 = 1;
#if !KOTOR1  // K2-only fields
    m_nPoseDisabled = 0;
    m_nReserved2 = 0;
#endif
    m_pExternalData = 0;
    m_nExternalDataType = 0;

    m_pLightContainer = new CAurObjectLightContainer();

#if KOTOR1  // K1 uses the out-of-line List<Gob*>::Insert
    gobs.Insert(this, GobFindInsertIdx(m_szName));
#else
    GobInsertAt(GobFindInsertIdx(m_szName), this);
#endif
    m_cHasSkin = 0;
}

void AurTextureRelease(CAurTexture* pTexture);
extern CAurTexture* AurTextureGetReference(char* sName, char* sExtension);

char* firstword(char* pszLine);
void Parse(char* pszLine, char* pszValue);

Gob::~Gob()
{
    g_nCountOfGobs--;

    DoEvent(const_cast<char*>("Dying"), 0);
    PlayAnimation(0, 1.0f, 0, 0.0f);
    SetScene(0);
    SetBehavior(0);

    if (m_pPart != 0)
    {
        delete m_pPart;
        m_pPart = 0;
    }

    for (int i = 0; i < m_listModels.m_count; i++)
    {
        Model* pModel = m_listModels.m_pData[i];
        if (removedelref == 0)
        {
            if (--pModel->m_nRefCount == 0)
                pModel->foobar(pModel);
        }
    }

    m_listModels.m_count = 0;
    m_listParts.m_count = 0;
    if (m_pModel != 0)
    {
        if (removedelref == 0)
        {
            if (--m_pModel->m_nRefCount == 0)
                m_pModel->foobar(m_pModel);
        }
    }
    if (m_pLodModelX != 0)
    {
        if (removedelref == 0)
        {
            if (--m_pLodModelX->m_nRefCount == 0)
                m_pLodModelX->foobar(m_pLodModelX);
        }
    }
    if (m_pLodModelZ != 0)
    {
        if (removedelref == 0)
        {
            if (--m_pLodModelZ->m_nRefCount == 0)
                m_pLodModelZ->foobar(m_pLodModelZ);
        }
    }

    for (int i = 0; i < m_callbacks.m_count; i++)
    {
        if (m_callbacks.m_pData[i])
            delete m_callbacks.m_pData[i];
        m_callbacks.m_pData[i] = 0;
    }
    m_callbacks.m_count = 0;

    {
        SafePointer<CAurTexture>& watch = m_texWatch;
        CAurTexture* p = watch.m_pObject;
        if (p != 0)
            p->Remove(&m_texWatch);
        watch.m_pObject = 0;
        AurTextureRelease(p);
    }

    gobs.Remove(this);
    Scene::RemoveFromSceneManager(this);

    if (m_pLookAt != 0)
    {
        delete static_cast<LookAtTarget*>(m_pLookAt);
        m_pLookAt = 0;
    }
    if (m_pLightContainer != 0)
    {
        delete m_pLightContainer;
        m_pLightContainer = 0;
    }
}

inline StringGob* Gob::AsString() { return 0; }

inline char* Gob::GetName() { return m_szName; }


inline void* Gob::GetExternalData() { return m_pExternalData; }

inline void Gob::SetExternalData(void* a) { m_pExternalData = a; }

inline int Gob::GetExternalDataType() { return m_nExternalDataType; }

inline void Gob::SetExternalDataType(int a) { m_nExternalDataType = a; }

void Gob::InheritExternal(CAurObject* a)
{
    Gob* pSource = static_cast<Gob*>(a);
    m_pExternalData = pSource->m_pExternalData;
    m_nExternalDataType = pSource->m_nExternalDataType;
}


Vector Gob::SetPosition(Vector a) { m_vPosition = a; return a; }

inline Vector Gob::GetPosition() { return m_vPosition; }

void Gob::SetOrientation(Quaternion a) { m_qOrientation = a; }

inline Quaternion Gob::GetOrientation() { return m_qOrientation; }

inline Vector Gob::GetPreviousPosition() { return m_vPreviousPosition; }


void Gob::EnableForceAnimations() { m_bForceAnimated = true; }

void Gob::DisableForceAnimations() { m_bForceAnimated = false; }

bool Gob::IsForceAnimated() { return m_bForceAnimated; }

void Gob::EnableInViewVolume()
{
    m_bInViewVolume = true;
    DoEvent(const_cast<char*>("propagateviewvolume"), 0);
}

inline void Gob::DisableInViewVolume() { m_bInViewVolume = false; }

inline bool Gob::IsInViewVolume() { return m_bInViewVolume; }

void Gob::EnableAlwaysRender() { m_bAlwaysRendering = true; }

void Gob::DisableAlwaysRender() { m_bAlwaysRendering = false; }

bool Gob::IsAlwaysRendering() { return m_bAlwaysRendering; }

void Gob::EnableInCutscene() { m_bInCutscene = true; }

void Gob::DisableInCutscene() { m_bInCutscene = false; }

bool Gob::IsInCutscene() { return m_bInCutscene; }

void Gob::SetProcessFlag() { m_bProcessFlag = true; }


#if !KOTOR1  // K2-only (no K1 counterpart)
inline void Gob::SetForcePoints(short a) { m_nForcePoints = a; }
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
inline unsigned short Gob::GetForcePoints() { return m_nForcePoints; }
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
inline void Gob::SetAlignmentValue(short a) { m_nAlignmentValue = a; }
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
inline unsigned short Gob::GetAlignmentValue() { return m_nAlignmentValue; }
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
inline void Gob::SetForceGlowColor(float a, float b, float c, float d)
{
    m_fForceGlowR = a;
    m_fForceGlowG = b;
    m_fForceGlowB = c;
    m_fForceGlowA = d;
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
inline void Gob::GetForceGlowColor(float& a, float& b, float& c, float& d)
{
    a = m_fForceGlowR;
    b = m_fForceGlowG;
    c = m_fForceGlowB;
    d = m_fForceGlowA;
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
inline void Gob::EnableForceGlow(bool a) { m_bForceGlowEnabled = a; }
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
inline bool Gob::IsForceGlowEnabled() { return m_bForceGlowEnabled; }
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
inline void Gob::SetNonForceBlending(bool a) { m_bNonForceBlending = a; }
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
inline bool Gob::IsNonForceBlending() { return m_bNonForceBlending; }
#endif // !KOTOR1


#if !KOTOR1  // K2-only (no K1 counterpart)
inline void Gob::SetEmitterAsWeather(bool a) { SetAllEmitterAsWeather(m_pPart, a); }
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
inline void Gob::EnableHologram(bool a) { EnableHologramRendering(m_pPart, a); }
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
inline bool Gob::IsHologramEnabled() { return m_bHologramEnabled; }
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
inline void Gob::EnableIllumination(bool a, float b, float c, float d)
{
    m_fIlluminationR = b;
    m_fIlluminationG = c;
    m_fIlluminationB = d;
    EnablePerObjectIllumination(m_pPart, a);
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
inline bool Gob::IsIlluminationEnable() { return m_bIlluminationEnabled; }
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
inline void Gob::GetIlluminationColor(float& r, float& g, float& b)
{
    r = m_fIlluminationR;
    g = m_fIlluminationG;
    b = m_fIlluminationB;
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
inline void Gob::EnableRendering(bool a) { m_bRenderingEnabled = a; }
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
inline bool Gob::IsRenderingEnabled() { return m_bRenderingEnabled; }
#endif // !KOTOR1


inline void Gob::TurnOffShadows() { m_nShadows = 0; }

inline void Gob::TurnOnShadows() { m_nShadows = 1; }

inline void Gob::EnableDistortion() { m_bDistortion = true; }

inline void Gob::DisableDistortion() { m_bDistortion = false; }

inline void Gob::SetIgnoreHitCheck(bool a) { m_bIgnoreHitCheck = a; }

inline bool Gob::IsAnimating() { return m_bAnimating; }

void Gob::HitCheckHack(Vector a, Vector b, Vector* c) { HitCheck(a, b, c, 0); }


inline void Gob::SetRenderPersonalSpace(bool a) { m_bRenderPersonalSpace = a; }

inline void Gob::SetMotionBlurred(bool a) { m_bMotionBlurred = a; }

inline bool Gob::GetMotionBlurred() { return m_bMotionBlurred; }

inline void Gob::SetMaxBlurLength(float a) { m_fMaxBlurLength = a; }

inline float Gob::GetMaxBlurLength() { return m_fMaxBlurLength; }

void Gob::SetWind(bool a, float b, float c, float d)
{
    m_fWindStrength = b;
    m_fWindRadius = c;
    m_bWind = a;
    m_fWindLife = d;
}


void Gob::GetBoundingBox(Vector& a, Vector& b)
{
    a = m_vBoundingBoxMin;
    b = m_vBoundingBoxMax;
}

void Gob::EnableRenderGobBBox(const Vector& a)
{
    m_nRenderGobBBox = 1;
    m_vGobBBox = a;
}

void Gob::DisableRenderGobBBox() { m_nRenderGobBBox = 0; }


inline CAurScene* Gob::GetScene() { return m_pScene; }

void Gob::EnumParts(Part* pPart, int (*cb)(CAurPart*), List<CAurPart*>* parts)
{
    if (pPart) {
        if (cb) {
            if (cb(static_cast<CAurPart*>(pPart)))
                parts->Add(static_cast<CAurPart*>(pPart));
        } else {
            parts->Add(static_cast<CAurPart*>(pPart));
        }
        if (pPart->m_nChildren >= 1)
            for (int i = 0; i < pPart->m_nChildren; i++)
                EnumParts(pPart->m_paChildren[i], cb, parts);
    }
}

void Gob::EnumParts(int (*cb)(CAurPart*), List<CAurPart*>* parts) { EnumParts(m_pPart, cb, parts); }

void Gob::DisableFog() { PartSetFog(m_pPart, false); }

void Gob::EnableFog() { PartSetFog(m_pPart, true); }

int Gob::SetCanDownSample(char a)
{
    if (m_cCanDownSample == a)
        return 0;
    m_cCanDownSample = a;
    return PartSetCanDownSample(m_pPart, a);
}

void Gob::SetBehavior(CAurBehavior* a)
{
    CAurBehavior* pOld = m_pBehavior;
    m_pBehavior = a;
    delete pOld;
}

void* Gob::AttachToObject(CAurObject* pObject, char* name, int nType)
{
    Part* pPart = 0;
    if (pObject)
        pPart = pObject->GetPart(name);
    CAurBehavior* pBehavior = 0;
    if (pPart) {
        pPart->GetWorld(m_vPosition, m_qOrientation);
        switch (nType) {
        case 0:
            pBehavior = new CAurBehaviorAttach(static_cast<CAurObject*>(this), pObject, pPart);  // idiom-exempt: opener 21 debt (Gob:CAurObject not yet derived)
            break;
        case 1:
            pBehavior = new CAurBehaviorAttachTranslate(static_cast<CAurObject*>(this), pObject, pPart);  // idiom-exempt: opener 21 debt (Gob:CAurObject not yet derived)
            break;
        case 2:
            pBehavior = new CAurBehaviorAttachRotate(static_cast<CAurObject*>(this), pObject, pPart);  // idiom-exempt: opener 21 debt (Gob:CAurObject not yet derived)
            break;
        case 3:
            pBehavior = new CAurBehaviorAttachNoOrientation(static_cast<CAurObject*>(this), pObject, pPart);  // idiom-exempt: opener 21 debt (Gob:CAurObject not yet derived)
            break;
        }
        SetBehavior(pBehavior);
        SetScene(static_cast<Gob*>(pObject)->m_pScene);
    } else {
        SetBehavior(0);
    }
    return pBehavior;
}

void* Gob::AttachToPart(CAurObject* pObject, CAurPart* pPart, int nType)
{
#ifdef K2_FIX_ENGINE_BUGS
    CAurBehavior* pBehavior = 0;
#else
    CAurBehavior* pBehavior;  // an nType above 3 leaves it unset
#endif
    pPart->GetWorld(m_vPosition, m_qOrientation);
    switch (nType) {
    case 0:
        pBehavior = new CAurBehaviorAttach(static_cast<CAurObject*>(this), pObject, pPart);  // idiom-exempt: opener 21 debt (Gob:CAurObject not yet derived)
        break;
    case 1:
        pBehavior = new CAurBehaviorAttachTranslate(static_cast<CAurObject*>(this), pObject, pPart);  // idiom-exempt: opener 21 debt (Gob:CAurObject not yet derived)
        break;
    case 2:
        pBehavior = new CAurBehaviorAttachRotate(static_cast<CAurObject*>(this), pObject, pPart);  // idiom-exempt: opener 21 debt (Gob:CAurObject not yet derived)
        break;
    case 3:
        pBehavior = new CAurBehaviorAttachNoOrientation(static_cast<CAurObject*>(this), pObject, pPart);  // idiom-exempt: opener 21 debt (Gob:CAurObject not yet derived)
        break;
    }
    SetBehavior(pBehavior);
    SetScene(static_cast<Gob*>(pObject)->m_pScene);
    return pBehavior;
}

void Gob::BehaviorMessage(char* a)
{
    if (m_pBehavior)
        m_pBehavior->Message(a);
}


void Gob::SetColorShifting(Vector a, float b, int c)
{
    m_vColorShift = a;
    m_nColorShifting = 1;
    m_fColorShiftAmount = b;
    if (c)
        DoEvent(const_cast<char*>("propagatecolorshift"), 0);
}

void Gob::SetIllumination(Vector a, int b)
{
    m_nIllumination = 1;
    m_vIllumination = a;
    if (b)
        DoEvent(const_cast<char*>("propagateillumination"), 0);
}

void Gob::SetUpImposters(int a, float* b)
{
    m_nImposters = a;
    m_nImposterHandle = GLRender::SetUpImposters(a, b);
}

// Transforms world to local for the shadow-volume walkers. Subtracts the frame origin, then
// rotates by the conjugate of the frame orientation.
static inline Vector WorldToLocal(const Vector& v, const Quaternion& q, const Vector& vOrigin)
{
#if KOTOR1  // K1 rotates through the out-of-line inverse operator
    Quaternion nq = -q;
    float w = nq.w, x = nq.x, y = nq.y, z = nq.z;
    Vector p = { v.x - vOrigin.x, v.y - vOrigin.y, v.z - vOrigin.z };
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * w;
        float yz = y * z, wy = y * w;
        float wz = z * w;
        Vector r;
        r.x = p.x * (1.0f - ((yy + zz) + (yy + zz))) + p.y * ((xy - wz) + (xy - wz)) + p.z * ((xz + wy) + (xz + wy));
        r.y = p.x * ((xy + wz) + (xy + wz)) + p.y * (1.0f - ((xx + zz) + (xx + zz))) + p.z * ((yz - wx) + (yz - wx));
        r.z = p.x * ((xz - wy) + (xz - wy)) + p.y * ((yz + wx) + (yz + wx)) + p.z * (1.0f - ((xx + yy) + (xx + yy)));
        p = r;
    }
    return p;
#else
    float w = q.w, x = q.x, y = q.y, z = q.z;
    float nw = -w;
    Vector p = { v.x - vOrigin.x, v.y - vOrigin.y, v.z - vOrigin.z };
    if (quaternionidentityoptimization == 0 || (w != -1.0f && w != 1.0f)) {
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * nw;
        float yz = y * z, wy = y * nw;
        float wz = z * nw;
        Vector r;
        r.x = p.x * (1.0f - ((yy + zz) + (yy + zz))) + p.y * ((xy - wz) + (xy - wz)) + p.z * ((xz + wy) + (xz + wy));
        r.y = p.x * ((xy + wz) + (xy + wz)) + p.y * (1.0f - ((xx + zz) + (xx + zz))) + p.z * ((yz - wx) + (yz - wx));
        r.z = p.x * ((xz - wy) + (xz - wy)) + p.y * ((yz + wx) + (yz + wx)) + p.z * (1.0f - ((xx + yy) + (xx + yy)));
        p = r;
    }
    return p;
#endif
}

// The inverse of WorldToLocal rotates by the frame orientation (not its conjugate), then adds
// the frame origin. LocalToWorldDir skips the add, which is a real operation under strict
// floating point.
static inline Vector LocalToWorldDir(const Vector& v, const Quaternion& q)
{
    float w = q.w, x = q.x, y = q.y, z = q.z;
    Vector r;
    if (quaternionidentityoptimization == 0 || (w != -1.0f && w != 1.0f)) {
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = x * w;
        float yz = y * z, wy = y * w;
        float wz = z * w;
        r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) + v.z * ((xz + wy) + (xz + wy));
        r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) + v.z * ((yz - wx) + (yz - wx));
        r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) + v.z * (1.0f - ((xx + yy) + (xx + yy)));
    } else {
        r = v;
    }
    return r;
}

static inline Vector LocalToWorld(const Vector& v, const Quaternion& q, const Vector& vOrigin)
{
    Vector r = LocalToWorldDir(v, q);
    r.x += vOrigin.x;
    r.y += vOrigin.y;
    r.z += vOrigin.z;
    return r;
}

// HitCheckPart and HitCheck share this body. It takes both endpoints into the gob's local frame,
// reject a ray that misses the bounding sphere, let the root part decide, then carry the
// part's local hit point and side vector back out to world.
Gob* Gob::HitCheckPart(CAurPart* a, Vector b, Vector c, Vector* d, CHitInfo* e)
{
    if (m_pModel == 0)
        return 0;
    if (m_pModel->m_nFlags & Model::kModelRoom)
        return 0;

    if (e == 0)
        e = &DefaultHitInfo;

    if (e->m_bHonorIgnoreHitCheck && m_bIgnoreHitCheck)
        return 0;
    if (e->m_bSkipFaded) {
        if (m_pModel->m_nFlags & 8) {
            if (m_fFadeAlpha <= mindooralpha)
                return 0;
        } else {
            if (m_fColorShiftAmount <= mindooralpha)
                return 0;
        }
    }

#if KOTOR1  // K1 shrinks the ray about the bounding sphere centre by the tunable
    if (IosCollisionRadiusMultiplier != 1.0f) {
        float fInv = 1.0f / IosCollisionRadiusMultiplier;
        b.x = m_vMinSphereCenter.x + fInv * (b.x - m_vMinSphereCenter.x);
        b.y = m_vMinSphereCenter.y + fInv * (b.y - m_vMinSphereCenter.y);
        b.z = m_vMinSphereCenter.z + fInv * (b.z - m_vMinSphereCenter.z);
        c.x = m_vMinSphereCenter.x + fInv * (c.x - m_vMinSphereCenter.x);
        c.y = m_vMinSphereCenter.y + fInv * (c.y - m_vMinSphereCenter.y);
        c.z = m_vMinSphereCenter.z + fInv * (c.z - m_vMinSphereCenter.z);
    }
#endif
    Part* pPart = static_cast<Part*>(a);
    Vector v1Local = WorldToLocal(b, m_qOrientation, m_vPosition);
    Vector v2Local = WorldToLocal(c, m_qOrientation, m_vPosition);

    if (!SphereIntersect(v1Local, v2Local, m_vMinSphereCenter, m_fMinSphereRadius))
        return 0;
    if (pPart == 0)
        return 0;
    if (pPart->HitCheck(v1Local, v2Local, &v2Local, e, 1.0f) == 0)
        return 0;

    Vector vHitWorld = LocalToWorld(v2Local, m_qOrientation, m_vPosition);
    e->m_hitPosition = vHitWorld;
    e->m_vHitNormal = LocalToWorldDir(e->m_vHitNormal, m_qOrientation);
    if (d != 0)
        *d = vHitWorld;
    return this;
}

void* Gob::HitCheck(Vector a, Vector b, Vector* c, CHitInfo* d)
{
    if (m_pModel == 0)
        return 0;
    if (m_pModel->m_nFlags & Model::kModelRoom)
        return 0;

    if (d == 0)
        d = &DefaultHitInfo;

    if (d->m_bHonorIgnoreHitCheck && m_bIgnoreHitCheck)
        return 0;
    if (d->m_bSkipFaded) {
        if (m_pModel->m_nFlags & 8) {
            if (m_fFadeAlpha <= mindooralpha)
                return 0;
        } else {
            if (m_fColorShiftAmount <= mindooralpha)
                return 0;
        }
    }

    Part* pPart = m_pPart;
    Vector v1Local = WorldToLocal(a, m_qOrientation, m_vPosition);
    Vector v2Local = WorldToLocal(b, m_qOrientation, m_vPosition);

    if (!SphereIntersect(v1Local, v2Local, m_vMinSphereCenter, m_fMinSphereRadius))
        return 0;
    if (pPart == 0)
        return 0;
    if (pPart->HitCheck(v1Local, v2Local, &v2Local, d, m_fObjectScale) == 0)
        return 0;

    Vector vHitWorld = LocalToWorld(v2Local, m_qOrientation, m_vPosition);
    d->m_hitPosition = vHitWorld;
    d->m_vHitNormal = LocalToWorldDir(d->m_vHitNormal, m_qOrientation);
    if (c != 0)
        *c = vHitWorld;
    return this;
}

Part* Gob::GetPart(char* name)
{
    if (_stricmp("root", name) == 0)
        return m_pPart;
    return FindPart(m_pPart, name);
}

int Gob::GetPartLocalPosition(char* a, Vector* b, Quaternion* c)
{
    Part* pPart = GetPart(a);
    if (pPart == 0)
        return 0;
    if (b != 0)
        *b = pPart->m_vLocalPosition;
    if (c != 0)
        *c = pPart->m_qLocalOrientation;
    return 1;
}

int Gob::SetPartLocalPosition(char* a, Vector b, Quaternion c)
{
    Part* pPart = GetPart(a);
    if (pPart == 0)
        return 0;
    pPart->m_vLocalPosition = b;
    pPart->m_qLocalOrientation = c;
    pPart->m_nLocalPositionSet = 1;
    return 1;
}

int Gob::SetPartPosition(char* a, Vector b, Quaternion c)
{
    Part* pPart = GetPart(a);
    if (pPart == 0)
        return 0;

    pPart->m_vLocalPosition.x = 0.0f;
    pPart->m_vLocalPosition.y = 0.0f;
    pPart->m_vLocalPosition.z = 0.0f;
    pPart->m_qLocalOrientation.w = 1.0f;
    pPart->m_qLocalOrientation.x = 0.0f;
    pPart->m_qLocalOrientation.y = 0.0f;
    pPart->m_qLocalOrientation.z = 0.0f;

    Vector vWorld = { 0.0f, 0.0f, 0.0f };
    Quaternion qWorld = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
    pPart->GetWorld(vWorld, qWorld);

    pPart->m_vLocalPosition = WorldToLocal(b, qWorld, vWorld);
#if KOTOR1  // K1 rotates through the out-of-line Quaternion inverse operator
    pPart->m_qLocalOrientation = normalize(-qWorld * c);
#else
    Quaternion qInverse = { -qWorld.w, qWorld.x, qWorld.y, qWorld.z };
    pPart->m_qLocalOrientation = normalize(qInverse * c);
#endif
    pPart->m_nLocalPositionSet = 1;
    return 1;
}

void Gob::GetMinimumBoundingBox()
{
    m_vBoundingBoxMin.x = 10000.0f;
    m_vBoundingBoxMin.y = 10000.0f;
    m_vBoundingBoxMin.z = 10000.0f;
    m_vBoundingBoxMax.x = -10000.0f;
    m_vBoundingBoxMax.y = -10000.0f;
    m_vBoundingBoxMax.z = -10000.0f;
    m_pPart->GetMinimumBoundingBox(m_vBoundingBoxMin, m_vBoundingBoxMax, m_fObjectScale);
}

void Gob::GetMinimumBoundingBox(Vector& a, Vector& b)
{
    GetMinimumBoundingBox();
    a = m_vBoundingBoxMin;
    b = m_vBoundingBoxMax;
}

void Gob::GetMinimumBoundingSphere(Vector& a, float& b)
{
    GetMinimumSphere();
    a = m_vMinSphereCenter;
    b = m_fMinSphereRadius;
}

void Gob::GetMinimumSphere()
{
    Part* pPart = m_pPart;
    if (pPart == 0)
        return;
    if (m_fMinSphereRadius != 1.0f)
        return;
    pPart->GetMinimumSphere(m_vMinSphereCenter, m_fMinSphereRadius);
    Vector v = m_vMinSphereCenter;
    Vector vLocal = m_pPart->m_vLocalPosition;
    float fScale = m_fObjectScale;
    v.x = (v.x + vLocal.x) * fScale;
    v.y = (v.y + vLocal.y) * fScale;
    v.z = fScale * (v.z + vLocal.z);
    m_vMinSphereCenter = v;
    m_fMinSphereRadius = fScale * m_fMinSphereRadius;
    m_fMinSphereRadius = m_fMinSphereRadius * gobspherescaling;
}


#if !KOTOR1  // K2-only (no K1 counterpart)
void Gob::EnableHologramRendering(Part* pPart, bool b)
{
    m_bHologramEnabled = b;
    pPart->m_bHologramRendering = b;
    for (int i = 0; i < pPart->m_nChildren; i++)
        EnableHologramRendering(pPart->m_paChildren[i], b);
}
#endif // !KOTOR1

#if !KOTOR1  // K2-only (no K1 counterpart)
void Gob::EnablePerObjectIllumination(Part* pPart, bool b)
{
    pPart->m_bPerObjectIllumination = b;
    m_bIlluminationEnabled = b;
    for (int i = 0; i < pPart->m_nChildren; i++)
        EnablePerObjectIllumination(pPart->m_paChildren[i], b);
}
#endif // !KOTOR1

void Gob::AddAttachment(Gob* pGob)
{
    if (m_attachments.Contains(pGob) == 0)
        m_attachments.Add(pGob);
}

void Gob::RemoveAttachment(Gob* pGob)
{
    if (m_attachments.Contains(pGob) != 0)
        m_attachments.Remove(pGob);
}

bool Gob::Append(char* pszModelName, char* pszPartName)
{
    Part* pParent = FindPart(m_pPart, pszPartName);
    if (pParent == 0)
        return false;

    Model* pModel = FindModel(pszModelName);
    if (pModel == 0)
        return false;

    pModel->m_nRefCount++;
    Part* pNode = CreateInstanceTreeR(pModel->m_pMdlNode, this);
    pNode->PostProcess(pNode);
    pNode->m_vLocalPosition = Vector(0.0f, 0.0f, 0.0f);
    pNode->m_qLocalOrientation = Quaternion();

    m_listModels.Add(pModel);
    m_listParts.Add(pNode);
// The instanced part becomes a child of the part found above; the child container overlays
// the modelled array as a List<Part*>.
    List<Part*>& rChildren = *reinterpret_cast<List<Part*>*>(&pParent->m_paChildren);  // raw-offset: Part's child array is a List<Part*> the destructor frees by hand
    rChildren.Add(pNode);
#if KOTOR1  // K1: the parent sphere memo is stale
    pParent->GetMinimumSphere_cache_GetData()->bValid = false;
#endif
    pNode->m_pParent = pParent;

    PartInitMaterial(this, pNode, const_cast<char*>(""));
    m_bProcessFlag = true;
    CreateReferenceObjects(this, pNode);
    DoEvent(const_cast<char*>("_EmitterTarget"), 0);
    return true;
}

bool Gob::Amputate(char* pszName)
{
    Part* pRoot = FindPart(m_pPart, pszName);
    if (pRoot == 0)
        return false;

    List<Part*> toProcess;
    toProcess.Add(pRoot);
    while (toProcess.m_count != 0)
    {
        Part* pNode = toProcess.m_pData[--toProcess.m_count];
        for (int i = 0; i < pNode->m_nChildren; i++)
            toProcess.Add(pNode->m_paChildren[i]);

        if (m_listParts.Contains(pNode) != 0)
        {
            int nIdx = 0;
            while (m_listParts.m_pData[nIdx] != pNode)
            {
                if (++nIdx == m_listParts.m_count)
                {
                    nIdx = -1;
                    break;
                }
            }

            if (!removedelref)
            {
                Model* pModel = m_listModels.m_pData[nIdx];
                if (--pModel->m_nRefCount == 0)
                    pModel->foobar(pModel);
            }

            m_listParts.DelIndex(nIdx);
            m_listModels.DelIndex(nIdx);
        }
    }

    List<Part*>& rChildren = *reinterpret_cast<List<Part*>*>(&pRoot->m_pParent->m_paChildren);  // raw-offset: Part's child array is a List<Part*> the destructor frees by hand
    rChildren.Remove(pRoot);
#if KOTOR1  // K1: the parent sphere memo is stale
    pRoot->m_pParent->GetMinimumSphere_cache_GetData()->bValid = false;
#endif
    pRoot->m_pParent = 0;
    delete pRoot;
    m_bProcessFlag = true;
    return true;
}

bool Gob::LoadAddInAnimations(char* pszName)
{
#if KOTOR1  // K1 has no loose-.mdl fallback; a miss dereferences null
    m_pAddInModel = FindModel(pszName);
#ifdef K2_FIX_ENGINE_BUGS
    if (m_pAddInModel == 0)
        return false;
#endif
    m_pAddInModel->m_nRefCount++;
    return true;
#else
    m_pAddInModel = FindModel(pszName);
    if (m_pAddInModel == 0)
    {
        char szFile[64];
        __strcpy_chk(szFile, pszName, sizeof(szFile));
        __builtin___strcat_chk(szFile, ".mdl", sizeof(szFile));
        FILE* pFile = fopen(szFile, "r");
        if (pFile == 0)
            return false;
        m_pAddInModel = LoadModel(pFile, 0);
        m_pAddInModel->SynchronizeTree(m_pModel);
        for (int i = 0; i < m_pModel->m_animations.m_count; i++)
            m_pModel->m_animations.m_pData[i]->SynchronizeTree(m_pModel);
    }
    m_pAddInModel->m_nRefCount++;
    return true;
#endif
}

void Gob::RemoveAddInAnimations()
{
    Model* pModel = m_pAddInModel;
    if (pModel != 0)
    {
        if (removedelref == 0)
        {
            if (--pModel->m_nRefCount == 0)
                pModel->foobar(pModel);
        }
    }
    m_pAddInModel = 0;
}

void Gob::SetScene(Scene* pScene)
{
    Scene* pOld = m_pScene;
    if (pOld == pScene)
        return;
    if (pOld != 0)
        pOld->ChildRemove(this);
    m_pScene = pScene;
    if (pScene != 0)
        pScene->ChildAdd(this);
    DoEvent(const_cast<char*>("UpdateScenes"), 0);
}

void Gob::ProcessGob()
{
    if (!m_bProcessFlag)
        return;
    m_pSingleMaterial = 0;
    m_bHasSingleTexture = TreeHasSingleTexture(m_pPart);
    if (!m_bHasSingleTexture)
    {
        m_pSingleMaterial = 0;
        m_bHasSingleTexture = false;
    }
    else if (m_pSingleMaterial == 0)
    {
        m_bHasSingleTexture = false;
    }
    m_bHasGeometry = TreeHasGeometry(m_pPart);
    TreeSetGeometryFlags(m_pPart);
    m_bHasTransparency = TreeHasTransparency(m_pPart);
    GetMinimumSphere();
    m_bProcessFlag = false;
}

void Gob::RenderBoundingBox()
{
    GLRender::Disable(Aurora::kCapCullFace);
    ASLgl::glPushMatrix();
    const Vector& v = m_vPosition;
    const Quaternion& q = m_qOrientation;
    if (enabletransform)
        GLRender::Transform(v, q, 1);
    if (m_fObjectScale != 1.0f)
        ASLgl::glScalef(m_fObjectScale, m_fObjectScale, m_fObjectScale);
    Vector vMax = m_vBoundingBoxMax;
    if (m_vBoundingBoxMax.z <= 0.1f)
        vMax.z = 0.1f;
    GLRender::DrawAxisAlignedFrame(m_vBoundingBoxMin, vMax, m_vGobBBox);
    ASLgl::glPopMatrix();
    GLRender::Enable(Aurora::kCapCullFace);
}

Vector Gob::GetCutsceneDummyPosition()
{
    Part* pPart = FindPart(m_pPart, const_cast<char*>("cutscenedummy"));
    Vector v(0.0f, 0.0f, 0.0f);
    if (pPart != 0)
    {
        Quaternion q(1.0f, 0.0f, 0.0f, 0.0f);
        pPart->GetWorld(v, q);
    }
    return v;
}

CAurBehavior* Gob::GetBehavior(int a)
{
    if (a != -1)
    {
        CAurBehavior* p = m_pBehavior;
        if (p == 0)
            return 0;
        if (p->GetType() != a)
            return 0;
    }
    return m_pBehavior;
}

void Gob::SetAllEmitterAsWeather(Part* pPart, bool b)
{
#if !KOTOR1  // K1 has no weather emitters (no PartEmitter::m_bWeather, no exported walker)
    PartEmitter* pEmitter = pPart->AsPartEmitter();
    if (pEmitter != 0)
        pEmitter->m_bWeather = b;
#endif
    for (int i = 0; i < pPart->m_nChildren; i++)
        SetAllEmitterAsWeather(pPart->m_paChildren[i], b);
}

char* Gob::GetModelName()
{
    Model* pModel = m_pModel;
    if (pModel == 0)
        return 0;
    return pModel->m_szName;
}

inline void Gob::SetAsCharacter()
{
    Model* pModel = m_pModel;
    if (pModel != 0)
        pModel->m_nFlags |= 4;
}

bool Gob::ShouldWeCheckVisibilityHACK()
{
    if (m_bInCutscene)
        return false;
    Model* pModel = m_pModel;
    if (pModel == 0)
        return true;
#if KOTOR1  // K1: other flag bits decide
    if (pModel->m_nFlags & Model::kModelItem)
        return false;
    return (pModel->m_nFlags & Model::kModelRoom) == 0;
#else
    if (pModel->m_nFlags & (Model::kModelItem | Model::kModelRoom))
        return false;
    return (pModel->m_nFlags & Model::kModelEffect) == 0;
#endif
}

int Gob::GetPartPosition(char* a, Vector* b, Quaternion* c)
{
    Part* pPart = GetPart(a);
    if (pPart == 0)
        return 0;
    Vector v = pPart->m_vLocalPosition;
    Quaternion q = pPart->m_qLocalOrientation;
    pPart->GetWorld(v, q);
    if (b != 0)
        *b = v;
    if (c != 0)
        *c = q;
    return 1;
}

bool Gob::SetObjectScale(float a, bool b)
{
    if (m_pModel == 0)
        return 0;
    float fFactor = m_fObjectScaleBase;
    if (b)
    {
        float fCurrent = m_fObjectScale;
        m_fObjectScaleBase = a;
        fFactor = fCurrent / fFactor;
    }
    m_fObjectScale = fFactor * a;
    for (int i = 0; i < m_attachments.m_count; i++)
        m_attachments.m_pData[i]->SetObjectScale(m_fObjectScale, false);
    return 1;
}

int Gob::SetTextureAttributes(char* a, char* b)
{
    List<CAurTexture*> textures;
    GetAllNamedTextures(m_pPart, textures, a);
    CAurTexture** pData = textures.m_pData;
    int n = textures.m_count;
    if (n < 1)
        return 0;
    for (int i = 0; i < n; i++)
        AurTextureSetAttributes(pData[i], b);
    return 1;
}

const char* spewpltnames(char* pszArgs)
{
    K2_UNUSED(pszArgs);
#if KOTOR1  // K1 opens through the port's debug-file shim
    spewpltfp = _fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/libsource/AURORA/aurmodel.cpp",
                       "spewpltnames", 5020, "D:\\override\\filenamesofplts", "wb");
#else
    spewpltfp = fopen("D:\\override\\filenamesofplts", "wb");
#endif
    return "SPEWING";
}

const char* quitspewing(char* pszArgs)
{
    K2_UNUSED(pszArgs);
    fclose(spewpltfp);
    return "CLOSESD";
}

void GobRenderLast(CAurObject* pObject)
{
    Gob* pGob = static_cast<Gob*>(pObject);
    if (pGob->m_pScene == 0)
        return;
    pGob->m_pScene->ChildRemove(pGob);
    pGob->m_pScene->ChildAdd(pGob);
}

const char* behaviormessage(char* pszArgs)
{
    char szGobName[64];
    int nConsumed;

    int nRead = sscanf(pszArgs, "%s%n", szGobName, &nConsumed);

    char* pszMessage = pszArgs + nConsumed;
    if (*pszMessage != 0)
        pszMessage++;

    if (nRead < 1 || szGobName[0] == 0 || *pszMessage == 0)
        return "usage behaviormesage gob message";

    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return "no object by that name";

    pGob->BehaviorMessage(pszMessage);
    return "OK";
}

const char* setmirrorimage(char* pszArgs)
{
    char* pszCursor = pszArgs;
    char* pszName = GetToken(&pszCursor);

    Gob* pGob = FindGob(pszName);
    if (pGob == 0)
        return "FAILURE";

    char* pszCount = GetToken(&pszCursor);
    int nImages = atoi(pszCount);
    float* paOffsets = new float[nImages * 3];

    int nSteps = nImages ? nImages : 1;

    for (int i = 0; i < nSteps; i++)
    {
        float fStep = 6.2831855f / static_cast<float>(nSteps);
        paOffsets[i * 3 + 0] = sinf(fStep) * 3.0f;
        paOffsets[i * 3 + 1] = cosf(fStep) * 3.0f;
        paOffsets[i * 3 + 2] = 0.0f;
    }

    pGob->SetUpImposters(atoi(pszCount), paOffsets);
    return "SUCCESS";
}

const char* message(char* pszArgs)
{
    char szGobName[64];
    int nConsumed;

    szGobName[0] = 0;
    nConsumed = 0;
    static_cast<void>(sscanf(pszArgs, "%s%n", szGobName, &nConsumed));

    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return "no object by that name";

    char* pszMessage = skipspace(pszArgs + nConsumed);
    if (pGob->m_pBehavior == 0)
        return "object doesn't even have a behavior associated with it";

    pGob->m_pBehavior->Message(pszMessage);
    return "OK";
}

bool TreeHasSingleLightProperties(Part* pPart)
{
    PartTriMesh* pMesh = pPart->AsPartTriMesh();
    if (pMesh != 0)
    {
        PartTriMesh* pFirst = pPart->m_pGob->m_pSingleLightPart;
        if (pFirst == 0)
        {
            pMesh->m_pGob->m_pSingleLightPart = pMesh;
        }
        else
        {
            MdlNodeTriMesh* pNode = pMesh->m_pNode->AsMdlNodeTriMesh();
            MdlNodeTriMesh* pFirstNode = pFirst->m_pNode->AsMdlNodeTriMesh();
// Reads the diffuse colour first, then the ambient; the two field names are swapped relative to
// the data they carry.
            if (!equal(pNode->m_vDiffuse, pFirstNode->m_vDiffuse) ||
                !equal(pNode->m_vAmbient, pFirstNode->m_vAmbient))
                return false;
        }
    }

    for (int i = 0; i < pPart->m_nChildren; i++)
    {
        if (!TreeHasSingleLightProperties(pPart->m_paChildren[i]))
            return false;
    }
    return true;
}

bool TreeHasGeometry(Part* pPart)
{
    PartTriMesh* pMesh = pPart->AsPartTriMesh();
    if (pPart->m_pGob != 0)
    {
        if (pMesh != 0)
            return true;
        for (int i = 0; i < pPart->m_nChildren; i++)
        {
            if (TreeHasGeometry(pPart->m_paChildren[i]))
                return true;
        }
    }
    return false;
}

bool TreeHasTransparency(Part* pPart)
{
    PartTriMesh* pMesh = pPart->AsPartTriMesh();
    if (pPart->m_pGob != 0)
    {
        if (pMesh != 0 && pMesh->m_fOpacity < 1.0f)
            return true;
        for (int i = 0; i < pPart->m_nChildren; i++)
        {
            if (TreeHasTransparency(pPart->m_paChildren[i]))
                return true;
        }
    }
    return false;
}

bool TreeHasSingleTexture(Part* pPart)
{
    PartTriMesh* pMesh = pPart->AsPartTriMesh();
    if (pPart->m_pGob == 0)
        return false;
    if (pMesh != 0)
    {
        Material* pCached = pPart->m_pGob->m_pSingleMaterial;
        if (pCached == 0)
            pPart->m_pGob->m_pSingleMaterial = pMesh->m_pMaterial;
        else if (pMesh->m_pMaterial->Textures(0).m_pObject == 0)
        {
            if (pCached->Textures(0).m_pObject != 0)
                return false;
        }
        else if (pCached->Textures(0).m_pObject != pMesh->m_pMaterial->Textures(0).m_pObject)
            return false;
    }
    for (int i = 0; i < pPart->m_nChildren; i++)
    {
        if (!TreeHasSingleTexture(pPart->m_paChildren[i]))
            return false;
    }
    return true;
}

bool TreeSetGeometryFlags(Part* pPart)
{
    pPart->m_bDrawEnabled = false;
    for (int i = 0; i < pPart->m_nChildren; i++)
    {
        if (TreeSetGeometryFlags(pPart->m_paChildren[i]))
            pPart->m_bDrawEnabled = true;
    }
    if (pPart->AsPartTriMesh() != 0)
    {
        pPart->m_bDrawEnabled = true;
        return true;
    }
    return pPart->m_bDrawEnabled;
}

void GetAllNamedTextures(Part* pPart, List<CAurTexture*>& out, char* pszName)
{
    PartTriMesh* pMesh = pPart->AsPartTriMesh();
    if (pMesh != 0)
    {
        CAurTexture* pTexture = pMesh->m_pMaterial->Textures(0).m_pObject->GetReferencedTexture();
        if (pTexture->AsCAurTextureBasic() != 0 &&
            _stricmp(pTexture->GetName(), pszName) == 0 &&
            out.Contains(pTexture) == 0)
            out.Add(pTexture);
    }

    for (int i = 0; i < pPart->m_nChildren; i++)
        GetAllNamedTextures(pPart->m_paChildren[i], out, pszName);
}

void CreateReferenceObjects(Gob* pGob, Part* pPart)
{
    PartReference* pRef = pPart->AsPartReference();
    if (pRef != 0)
    {
        MdlNodeReference* pNode = pRef->m_pNode->AsMdlNodeReference();
        pRef->m_pObject = NewCAurObject(pNode->m_szRefModel, 0, 0, 0);
        if (pRef->m_pObject != 0)
        {
            pRef->m_pObject->AttachToObject(static_cast<CAurObject*>(pGob), pRef->Name(), 0);
            float fSpeed = ((rand_wincompatible() % 100) - 50.0f) / 1000.0f + 1.0f;
            float fPhase = (rand_wincompatible() % 100) / 100.0f;
            pRef->m_pObject->PlayAnimation(const_cast<char*>("default"), fSpeed, 0, fPhase);
        }
    }

    for (int i = 0; i < pPart->m_nChildren; i++)
        CreateReferenceObjects(pGob, pPart->m_paChildren[i]);
}

void Gob::ReattachReference(CAurObject* a, char* b)
{
    List<Part*> parts;
    parts.Add(m_pPart);

    while (parts.m_count > 0)
    {
        Part* part = parts.m_pData[0];

        PartReference* ref = part->AsPartReference();
        if (ref != 0 && ref->m_pNode != 0 && (ref->m_pNode->m_nFlags & NODE_REFERENCE) == NODE_REFERENCE &&
            static_cast<MdlNodeReference*>(ref->m_pNode)->m_bReattachable && ref->m_pObject != 0)
        {
            CAurObject* obj = ref->m_pObject;
            if (a != 0)
                obj->AttachToObject(a, b, 0);
            else
                obj->AttachToObject(static_cast<CAurObject*>(this), ref->Name(), 0);  // idiom-exempt: opener 21 debt (Gob:CAurObject not yet derived)

            Quaternion q = m_qOrientation;
            Vector v = ref->m_pObject->GetPosition();
            // inlined Quaternion::zdir()
            Vector vDir = Vector{
                2.0f * (q.x * q.z + q.w * q.y),
                2.0f * (q.z * q.y - q.x * q.w),
                1.0f - 2.0f * (q.x * q.x + q.y * q.y)
            };
            m_qOrientation = RotationArc(vDir, v - m_vPosition) * q;
        }

        parts.DelIndex(0);

        for (int i = 0; i < part->m_nChildren; i++)
            parts.Add(part->m_paChildren[i]);
    }
}

void PartSetFog(Part* pPart, bool bFog)
{
    PartTriMesh* pMesh = pPart->AsPartTriMesh();
    if (pMesh != 0)
        pMesh->m_bFog = bFog;

    for (int i = 0; i < pPart->m_nChildren; i++)
        PartSetFog(pPart->m_paChildren[i], bFog);
}

int PartSetCanDownSample(Part* pPart, char c)
{
    int nSet = 0;
    MdlNodeTriMesh* pNode = static_cast<MdlNodeTriMesh*>(pPart->m_pNode);
#ifdef K2_FIX_ENGINE_BUGS
    if (pNode != 0 && (pNode->m_nFlags & NODE_TRIMESH) == NODE_TRIMESH
#else
    if ((pNode != 0) & ((pNode->m_nFlags & NODE_TRIMESH) == NODE_TRIMESH)
#endif
        && pNode->m_szTexture[0] != 0 && strcmp(NULL_NAME, pNode->m_szTexture) != 0)
    {
        pPart->m_pMaterial->SetDownSampling(c);
        nSet = 1;
    }

    for (int i = 0; i < pPart->m_nChildren; i++)
        nSet += PartSetCanDownSample(pPart->m_paChildren[i], c);
    return nSet;
}

// The global gob registry, kept sorted by name.
List<Gob*> gobs;

Gob* FindGob(char* pszName)
{
    int nLow = 0;
    int nHigh = gobs.m_count - 1;
    while (nLow <= nHigh)
    {
        int nMid = (nHigh + nLow) >> 1;
        int nCompare = _stricmp(gobs.m_pData[nMid]->m_szName, pszName);
        if (nCompare > 0)
            nHigh = nMid - 1;
        else if (nCompare < 0)
            nLow = nMid + 1;
        else
        {
            nLow = nMid;
            nHigh = nMid - 1;
        }
    }
    if (nLow >= gobs.m_count)
        return 0;
    if (_stricmp(gobs.m_pData[nLow]->m_szName, pszName) != 0)
        return 0;
    return gobs.m_pData[nLow];
}

const char* gobdisablefog(char* pszArgs)
{
    Gob* pGob = FindGob(pszArgs);
    if (pGob == 0)
        return "Cannot find gob.";
    pGob->DisableFog();
    return "Success.";
}

const char* gobenablefog(char* pszArgs)
{
    Gob* pGob = FindGob(pszArgs);
    if (pGob == 0)
        return "Cannot find gob.";
    pGob->EnableFog();
    return "Success.";
}

const char* loadaddin(char* pszArgs)
{
    char szGobName[64];
    char szAddIn[64];

    if (sscanf(pszArgs, "%s %s", szGobName, szAddIn) != 2)
        return "Incorrect number of params";

    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return "Couldn't find Gob";

    pGob->LoadAddInAnimations(szAddIn);
    pGob->PlayAnimation(const_cast<char*>("ca1slashl"), 1.0f, 0, 0.0f);
    return "Playing ca1slashl";
}

const char* removeaddin(char* pszArgs)
{
    char szGobName[64];

    if (sscanf(pszArgs, "%s", szGobName) != 1)
        return "Incorrect number of params";

    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return "Couldn't find Gob";

    pGob->PlayAnimation(const_cast<char*>(NULL_NAME), 1.0f, 0, 0.0f);
    pGob->RemoveAddInAnimations();
    pGob->PlayAnimation(const_cast<char*>("ca1slashl"), 1.0f, 0, 0.0f);
    return "Removed add-in anims";
}

const char* rt(char* pszArgs)
{
    char szGobName[32];
    char szTexture2[32];
    char szTexture1[32];

    static_cast<void>(sscanf(pszArgs, "%s %s %s", szGobName, szTexture2, szTexture1));

    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return "No gob by that name!";
    pGob->ReplaceTexture(szTexture1, szTexture2);
    return "Replaced the texture";
}

const char* setgobstuff(char* pszArgs)
{
    char szGobName[64];
    char szTexture[64];

    szTexture[0] = 0;
    szGobName[0] = 0;
    if (sscanf(pszArgs, "%s %s", szGobName, szTexture) != 2)
        return "needs 2 params";

    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return "Can't find gob";
    pGob->SetTextureAttributes(szTexture, const_cast<char*>("bad texture"));
    return "Bad Texture";
}

const char* amputate(char* pszArgs)
{
    char szGobName[64];
    char szPartName[64];

    szPartName[0] = 0;
    szGobName[0] = 0;
    static_cast<void>(sscanf(pszArgs, "%s%s", szGobName, szPartName));
    if (szGobName[0] == 0 || szPartName[0] == 0)
        return "Usage:  ampuatate <gobname> <partname>";

    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return "Error:  No gob by that name";
    if (FindPart(pGob->m_pPart, szPartName) == 0)
        return "Error:  Object does not have part with that name";
    pGob->Amputate(szPartName);
    return "Amputation Sucessful";
}

#include "aurconsole.h"

// Must stay external. With internal linkage the optimizer specializes it for the fixed-size
// call sites and drops the length argument.
static void snprintff(char* buf, uint64_t n, const char* fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    __vsprintf_chk(buf, 0, n, fmt, ap);
    __builtin_va_end(ap);
}

// A part's display name lives in its model's string table, indexed by the node's name index;
// for an animation tree the table belongs to the animation's model.
static char* GetPartNodeName(Part* pPart)
{
    MdlNode* pNode = pPart->m_pNode;
    MaxTree* pTree = pNode->m_pTree;
    int nKind = pTree->m_cType & MaxTree::kTreeTypeMask;
    Animation* pAnim = nKind == MaxTree::kTreeAnimation ? static_cast<Animation*>(pTree) : static_cast<Animation*>(0);
    K2_ASSUME(pTree != 0 && (nKind == MaxTree::kTreeModel || pAnim != 0));   // a node's tree is the model or animation that loaded it
    List<char*>* pStrings;
    if (pTree != 0 && nKind == MaxTree::kTreeModel)
        pStrings = &static_cast<Model*>(pTree)->m_listStrings;
    else
        pStrings = &pAnim->m_pModel->m_listStrings;
    int nNameIndex = pNode->m_nNameIndex;
    return pStrings->m_pData[nNameIndex];
}

static void findalltrimeshparts(Part* pPart, List<PartTriMesh*>& out)
{
    PartTriMesh* pMesh = pPart->AsPartTriMesh();
    if (pMesh != 0)
        out.Add(pMesh);
    for (int i = 0; i < pPart->m_nChildren; i++)
        findalltrimeshparts(pPart->m_paChildren[i], out);
}

static void findallmaterials(Part* pPart, List<Material*>& out);

int Gob::ReplaceTexture(char* pszPublicName, char* pszTexName)
{
    if (pszPublicName[0] != 0)
    {
        List<Material*> materials;
        findallmaterials(m_pPart, materials);
        for (int i = 0; i < materials.m_count; i++)
        {
            materials.m_pData[i]->ReplaceTexture(pszPublicName, pszTexName);
            if (m_texWatch.m_pObject != 0)
                materials.m_pData[i]->Textures(0).m_pObject->SetIsEnvironmentMapped(true);
        }
        m_bProcessFlag = 1;
        return 1;
    }
    List<Material*> materials;
    findallmaterials(m_pPart, materials);
    for (int i = 0; i < materials.m_count; i++)
        materials.m_pData[i]->ReplaceTexture(0, pszTexName);
    return 0;
}

static void findallmaterials(Part* pPart, List<Material*>& out)
{
    Material* pMaterial = pPart->m_pMaterial;
    if (pMaterial != 0)
    {
        if (out.Contains(pMaterial) == 0)
            out.Add(pMaterial);
    }
    for (int i = 0; i < pPart->m_nChildren; i++)
        findallmaterials(pPart->m_paChildren[i], out);
}

int Gob::SetEnvironmentMap(char* pszName)
{
    char szName[64];
    if (pszName && pszName[0] != 0)
        __builtin___strcpy_chk(szName, pszName, sizeof(szName));
    else
        strcpy(szName, NULL_NAME);
    CAurTexture* pTexture = AurTextureGetReference(szName, 0);
    if (pTexture->IsInvalid())
    {
        AurTextureRelease(pTexture);
        return 0;
    }
    CAurTexture* pOld = m_texWatch.m_pObject;
    m_texWatch = pTexture;
    AurTextureRelease(pOld);
    if (m_texWatch.m_pObject != 0)
    {
        List<Material*> materials;
        findallmaterials(m_pPart, materials);
        for (int i = 0; i < materials.m_count; i++)
            materials.m_pData[i]->Textures(0).m_pObject->SetIsEnvironmentMapped(true);
    }
    return 1;
}

int Gob::RestoreTexture()
{
    List<PartTriMesh*> parts;
    findalltrimeshparts(m_pPart, parts);
    for (int i = 0; i < parts.m_count; i++)
    {
        PartTriMesh* pMesh = parts.m_pData[i];
        MdlNodeTriMesh* pNodeTriMesh = pMesh->m_pNode->AsMdlNodeTriMesh();
        pMesh->m_pMaterial->ReplaceTexture(0, pNodeTriMesh->m_szTexture);
    }
    return 1;
}

int Gob::ReplaceTexture(char* pszPublicName, char* pszTexName, int n, unsigned short* pa, int nUnused)
{
    K2_UNUSED(nUnused);
    if (spewpltfp)
    {
        fprintf(spewpltfp, "%s\n", pszTexName);
        for (int i = 0; i < n; i++)
            fprintf(spewpltfp, "%i ", static_cast<int>(pa[i]));
        fputc('\n', spewpltfp);
    }
#ifdef K2_FIX_ENGINE_BUGS
    if (n > 256)
        n = 256;
#endif
    int paSizes[256];
    for (int i = 0; i < n; i++)
        paSizes[i] = pa[i];
    char* pPublicName = (pszPublicName && pszPublicName[0] != 0) ? pszPublicName : 0;
    List<PartTriMesh*> parts;
    findalltrimeshparts(this->m_pPart, parts);
    for (int i = 0; i < parts.m_count; i++)
    {
        PartTriMesh* pMesh = parts.m_pData[i];
        MdlNode* pNode = pMesh->m_pNode;
        MaxTree* pTree = pNode->m_pTree;
        int nKind = pTree->m_cType & MaxTree::kTreeTypeMask;
        Animation* pAnim = nKind == MaxTree::kTreeAnimation ? static_cast<Animation*>(pTree) : static_cast<Animation*>(0);
        K2_ASSUME(pTree != 0 && (nKind == MaxTree::kTreeModel || pAnim != 0));   // a node's tree is the model or animation that loaded it
        List<char*>* pStrings;
        if (pTree != 0 && nKind == MaxTree::kTreeModel)
            pStrings = &static_cast<Model*>(pTree)->m_listStrings;
        else
            pStrings = &pAnim->m_pModel->m_listStrings;
        int nNameIndex = pNode->m_nNameIndex;
        pMesh->m_pMaterial->ReplaceTexture(pPublicName, pszTexName,
            pStrings->m_pData[nNameIndex], paSizes, n);
        if (this->m_texWatch.m_pObject != 0)
            parts.m_pData[i]->m_pMaterial->Textures(0).m_pObject->SetIsEnvironmentMapped(true);
    }
    this->m_bProcessFlag = 1;
    return 1;
}

int Gob::ReplaceTextureSubtree(char* pszTexturePath, char* pszTextureName, int n, unsigned short* pa)
{
    if (spewpltfp)
    {
        fprintf(spewpltfp, "%s\n", pszTextureName);
        for (int i = 0; i < n; i++)
            fprintf(spewpltfp, "%i ", static_cast<int>(pa[i]));
        fputc('\n', spewpltfp);
    }
#ifdef K2_FIX_ENGINE_BUGS
    if (n > 256)
        n = 256;
#endif
    int paSizes[256];
    for (int i = 0; i < n; i++)
        paSizes[i] = pa[i];
    List<Material*> materials;
    int bResult = 0;
    Part* pPart = FindPart(m_pPart, pszTexturePath);
    if (pPart)
    {
        findallmaterials(pPart, materials);
        for (int i = 0; i < materials.m_count; i++)
        {
            materials.m_pData[i]->ReplaceTexture(0, pszTextureName, pszTexturePath, paSizes, n);
            if (m_texWatch.m_pObject != 0)
                materials.m_pData[i]->Textures(0).m_pObject->SetIsEnvironmentMapped(true);
        }
        m_bProcessFlag = 1;
        bResult = 1;
    }
    return bResult;
}

void exportparts(Gob* pGob, Part* pPart)
{
    char szBuf[256];
    if (pGob == 0 || pPart == 0)
        return;
    snprintff(szBuf, 256, "%s.%s.position", pGob->m_szName, GetPartNodeName(pPart));
    new ConsoleFunc(szBuf, &pPart->m_vLocalPosition);
    snprintff(szBuf, 256, "%s.%s.orientation", pGob->m_szName, GetPartNodeName(pPart));
    new ConsoleFunc(szBuf, &pPart->m_qLocalOrientation);
    snprintff(szBuf, 256, "%s.%s.controlmode", pGob->m_szName, GetPartNodeName(pPart));
    new ConsoleFunc(szBuf, static_cast<int*>(&pPart->m_nLocalPositionSet));
    for (int64_t i = 0; i < pPart->m_nChildren; i++)
        exportparts(pGob, pPart->m_paChildren[i]);
}

const char* exposeparts(char* pszArgs)
{
    Gob* pGob = FindGob(pszArgs);
    if (pGob == 0)
        return "no gob of that name";
    exportparts(pGob, pGob->m_pPart);
    return "OK";
}

const char* append(char* pszArgs)
{
    char szGobName[64];
    char szModelName[64];
    char szPartName[64];

    szPartName[0] = 0;
    szModelName[0] = 0;
    szGobName[0] = 0;

    static_cast<void>(sscanf(pszArgs, "%s%s%s", szGobName, szPartName, szModelName));

    if (szGobName[0] == 0 || szPartName[0] == 0 || szModelName[0] == 0)
        return "Usage:  append <gobname> <partname> <modelname>";

    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return "Error:  No gob by that name";

    if (FindPart(pGob->m_pPart, szPartName) == 0)
        return "Error:  Object does not have part with that name";

    pGob->Append(szModelName, szPartName);
    return "OK";
}

// The explicit instantiation adds a non-discardable use of SafePointer<CAurTexture>::
// operator=, which keeps that call out of line in SetEnvironmentMap.
template class SafePointer<CAurTexture>;

inline CAurObject* Gob::AsCAurObject() { return static_cast<CAurObject*>(this); }  // idiom-exempt: Gob.h does not model the CAurObject base (Camera.h precedent)


extern bool g_bKeepHistory;
extern int  cullwithLOD;
extern List<Model*> Models;
void AurTextureMakeCurrent();

char* purgemodels(char* pszArgs)
{
    K2_UNUSED(pszArgs);
    static char szBuf[256];

    int nDeleted = 0;
    for (int i = Models.m_count; i != 0; )
    {
        i--;
        if (i >= Models.m_count)
            continue;

        Model* pModel = Models.m_pData[i];
        if (pModel->m_nRefCount == 0)
        {
            if (pModel)
                delete pModel;
            nDeleted++;
        }
    }

    snprintff(szBuf, 256, "deleted %d models", nDeleted);
    return szBuf;
}

char* spinmodels(char* pszArgs)
{
    K2_UNUSED(pszArgs);
    for (int i = 0; i < 1000; i++)
    {
        CAurObject* pRat1 = NewCAurObject(const_cast<char*>("c_wererat"), const_cast<char*>(""), 0, 0);
        CAurObject* pSun  = NewCAurObject(const_cast<char*>("gidy_sun"), const_cast<char*>(""), 0, 0);
        CAurObject* pRat2 = NewCAurObject(const_cast<char*>("c_wererat"), const_cast<char*>(""), 0, 0);
        CAurObject* pRat3 = NewCAurObject(const_cast<char*>("c_wererat"), const_cast<char*>(""), 0, 0);
        CAurObject* pRat4 = NewCAurObject(const_cast<char*>("c_wererat"), const_cast<char*>(""), 0, 0);
        CAurObject* pRat5 = NewCAurObject(const_cast<char*>("c_wererat"), const_cast<char*>(""), 0, 0);
        CAurObject* pRat6 = NewCAurObject(const_cast<char*>("c_wererat"), const_cast<char*>(""), 0, 0);

        pSun->AttachToObject(pRat1, const_cast<char*>("wrat_handl"), 0);
        pSun->AttachToObject(pRat1, const_cast<char*>("wrat_handr"), 0);
        pSun->AttachToObject(pRat1, const_cast<char*>("wrat_t4"), 0);

        if (pRat1) delete pRat1;
        if (pSun)  delete pSun;
        if (pRat2) delete pRat2;
        if (pRat3) delete pRat3;
        if (pRat4) delete pRat4;
        if (pRat5) delete pRat5;
        if (pRat6) delete pRat6;
    }

    return const_cast<char*>("SUCK-CESS");
}

#if KOTOR1  // K1 streams models through SDL_RWops
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux)
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux)
#endif
{
    bool bWasKeepingHistory = g_bKeepHistory;
    g_bKeepHistory = false;

    Model* pSuper1 = 0;
    Model* pSuper2 = 0;
    Model* pModel;
    if (pFile == 0)
    {
        pModel = FindModel(pszResRef);
        if (cullwithLOD)
        {
            char szName[64];
// The fortify form is spelled out because this file declares strcpy itself, which suppresses the
// builtin lowering.
            __builtin___strcpy_chk(szName, pszResRef, sizeof(szName));
            int nLen = static_cast<int>(strlen(pszResRef));
            szName[nLen] = '_';
            szName[nLen + 1] = 'x';
            szName[nLen + 2] = 0;
            pSuper1 = FindModel(szName);

            __builtin___strcpy_chk(szName, pszResRef, sizeof(szName));
            nLen = static_cast<int>(strlen(pszResRef));
            szName[nLen] = '_';
            szName[nLen + 1] = 'z';
            szName[nLen + 2] = 0;
            pSuper2 = FindModel(szName);
        }
    }
    else
    {
        pModel = LoadModel(pFile, pAux);
    }

    if (pModel == 0)
    {
        g_bKeepHistory = bWasKeepingHistory;
        return 0;
    }

    if (pszParam2 == 0 || *pszParam2 == 0)
        pszParam2 = pModel->m_szName;

    Gob* pGob = new Gob(pszParam2);
    pGob->m_pModel = pModel;
    pModel->m_nRefCount++;
    pGob->m_pLodModelX = pSuper1;
    if (pSuper1 != 0)
        pSuper1->m_nRefCount++;
    pGob->m_pLodModelZ = pSuper2;
    if (pSuper2 != 0)
        pSuper2->m_nRefCount++;

    Part* pRoot = CreateInstanceTreeR(pModel->m_pMdlNode, pGob);
    pRoot->PostProcess(pRoot);
    pGob->m_pPart = pRoot;
    pRoot->m_pNode->ProcessSkinSeams();
    pGob->m_pPart->m_pGob = pGob;

    if (pSuper1 != 0)
    {
        Part* pPart1 = CreateInstanceTreeR(pSuper1->m_pMdlNode, pGob);
        pPart1->PostProcess(pPart1);
        pGob->m_pLodPartX = pPart1;
        pPart1->m_pGob = pGob;
    }
    if (pSuper2 != 0)
    {
        Part* pPart2 = CreateInstanceTreeR(pSuper2->m_pMdlNode, pGob);
        pPart2->PostProcess(pPart2);
        pGob->m_pLodPartZ = pPart2;
        pPart2->m_pGob = pGob;
    }

    CreateReferenceObjects(pGob, pGob->m_pPart);
    if (pGob->m_pLodPartX != 0)
        CreateReferenceObjects(pGob, pGob->m_pLodPartX);
    if (pGob->m_pLodPartZ != 0)
        CreateReferenceObjects(pGob, pGob->m_pLodPartZ);

    pGob->DoEvent(const_cast<char*>("_EmitterTarget"), 0);
    PartInitMaterial(pGob, pGob->m_pPart, const_cast<char*>(""));
    if (pGob->m_pLodPartX != 0)
        PartInitMaterial(pGob, pGob->m_pLodPartX, const_cast<char*>(""));
    if (pGob->m_pLodPartZ != 0)
        PartInitMaterial(pGob, pGob->m_pLodPartZ, const_cast<char*>(""));

    g_bKeepHistory = bWasKeepingHistory;

    pGob->GetMinimumBoundingBox();
    pGob->m_bProcessFlag = 1;
    pGob->Animate(0.0f);

    return static_cast<CAurObject*>(pGob);  // idiom-exempt: Gob.h models Gob flat, so Gob*/CAurObject* are unrelated types here (CreateReferenceObjects precedent)
}

char* doganmodels(char* pszArgs)
{
    K2_UNUSED(pszArgs);
    unsigned short paSizes[10] = {2, 260, 512, 512, 1024, 1280, 1280, 1572, 1577, 1577};

    for (int i = 0; i < 500; i++)
    {
        CAurObject* pGob = NewCAurObject(const_cast<char*>("pmh0"), const_cast<char*>(""), 0, 0);

        pGob->Append(const_cast<char*>("PMH0_FOOTR005"), const_cast<char*>("rfoot_g"));
        pGob->Append(const_cast<char*>("PMH0_FOOTL005"), const_cast<char*>("lfoot_g"));
        pGob->Append(const_cast<char*>("PMH0_SHINR009"), const_cast<char*>("rshin_g"));
        pGob->Append(const_cast<char*>("PMH0_SHINL009"), const_cast<char*>("lshin_g"));
        pGob->Append(const_cast<char*>("PMH0_LEGR003"), const_cast<char*>("rthigh_g"));
        pGob->Append(const_cast<char*>("PMH0_LEGL003"), const_cast<char*>("lthigh_g"));
        pGob->Append(const_cast<char*>("PMH0_PELVIS007"), const_cast<char*>("pelvis_g"));
        pGob->Append(const_cast<char*>("PMH0_CHEST012"), const_cast<char*>("torso_g"));
        pGob->Append(const_cast<char*>("PMH0_NECK001"), const_cast<char*>("neck_g"));
        pGob->Append(const_cast<char*>("PMH0_FORER013"), const_cast<char*>("rforearm_g"));
        pGob->Append(const_cast<char*>("PMH0_FOREL013"), const_cast<char*>("lforearm_g"));
        pGob->Append(const_cast<char*>("PMH0_BICEPR010"), const_cast<char*>("rbicep_g"));
        pGob->Append(const_cast<char*>("PMH0_BICEPL010"), const_cast<char*>("lbicep_g"));
        pGob->Append(const_cast<char*>("PMH0_HANDR004"), const_cast<char*>("rhand_g"));
        pGob->Append(const_cast<char*>("PMH0_HANDL004"), const_cast<char*>("lhand_g"));

        pGob->ReplaceTextureSubtree(const_cast<char*>("rfoot_g"), const_cast<char*>("pmh0_footr001"), 10, paSizes);
        pGob->ReplaceTextureSubtree(const_cast<char*>("lfoot_g"), const_cast<char*>("pmh0_footl001"), 10, paSizes);
        pGob->ReplaceTextureSubtree(const_cast<char*>("rshin_g"), const_cast<char*>("pmh0_shinr001"), 10, paSizes);
        pGob->ReplaceTextureSubtree(const_cast<char*>("lshin_g"), const_cast<char*>("pmh0_shinl001"), 10, paSizes);
        pGob->ReplaceTextureSubtree(const_cast<char*>("rthigh_g"), const_cast<char*>("pmh0_legr001"), 10, paSizes);
        pGob->ReplaceTextureSubtree(const_cast<char*>("lthigh_g"), const_cast<char*>("pmh0_legl001"), 10, paSizes);
        pGob->ReplaceTextureSubtree(const_cast<char*>("pelvis_g"), const_cast<char*>("pmh0_pelvis001"), 10, paSizes);
        pGob->ReplaceTextureSubtree(const_cast<char*>("torso_g"), const_cast<char*>("pmh0_chest001"), 10, paSizes);
        pGob->ReplaceTextureSubtree(const_cast<char*>("neck_g"), const_cast<char*>("pmh0_neck001"), 10, paSizes);
        pGob->ReplaceTextureSubtree(const_cast<char*>("rforearm_g"), const_cast<char*>("pmh0_forer002"), 10, paSizes);
        pGob->ReplaceTextureSubtree(const_cast<char*>("lforearm_g"), const_cast<char*>("pmh0_forel002"), 10, paSizes);
        pGob->ReplaceTextureSubtree(const_cast<char*>("rbicep_g"), const_cast<char*>("pmh0_bicepr001"), 10, paSizes);
        pGob->ReplaceTextureSubtree(const_cast<char*>("lbicep_g"), const_cast<char*>("pmh0_bicepl001"), 10, paSizes);
        pGob->ReplaceTextureSubtree(const_cast<char*>("rhand_g"), const_cast<char*>("pmh0_handr001"), 10, paSizes);
        pGob->ReplaceTextureSubtree(const_cast<char*>("lhand_g"), const_cast<char*>("pmh0_handl001"), 10, paSizes);

        AurTextureMakeCurrent();
        delete pGob;
    }

    return const_cast<char*>("SUCKS");
}

MdlNodeTriMesh_PostProcess::~MdlNodeTriMesh_PostProcess() noexcept(false)
{
    m_verts.SetSize(0);
    m_normals.SetSize(0);
    m_tverts.SetSize(0);
    m_tverts1.SetSize(0);
    m_tverts2.SetSize(0);
    m_tverts3.SetSize(0);
    m_colors.SetSize(0);
    m_faces.SetSize(0);
    for (int64_t i = 0; i < m_vertices.m_count; i++) {
        if (m_vertices.m_pData[i] != 0)
            delete m_vertices.m_pData[i];
    }
    m_vertices.SetSize(0);
}

MdlNodeTriMesh::~MdlNodeTriMesh()
{
    if (m_nFlags != 0)
        DestructorDispatch();
    m_nFlags = 0;
    MdlNodeTriMesh_PostProcess* pStrategy = static_cast<MdlNodeTriMesh_PostProcess*>(m_pPostProcess);
    if (pStrategy != 0) {
        pStrategy->~MdlNodeTriMesh_PostProcess();
        operator delete(pStrategy);
    }
    if (m_nLeftOverPool != static_cast<unsigned int>(-1)) {
        GLRender::DeletePool(m_nLeftOverPool);
        m_nLeftOverPool = static_cast<unsigned int>(-1);
    }
    for (int64_t i = 0; i < m_lIndexPools.m_count; i++) {
        GLRender::DeletePool(m_lIndexPools.m_pData[i]);
        m_lIndexPools.m_pData[i] = static_cast<unsigned int>(-1);
    }
    for (int64_t i = 0; i < m_lVertexIndices.m_count; i++) {
        if (m_lVertexIndices.m_pData[i] != 0)
            delete[] m_lVertexIndices.m_pData[i];
    }
    m_lVertexIndices.m_count = 0;
    delete[] m_paVertices;
}

MdlNodeDanglyMesh::~MdlNodeDanglyMesh()
{
    if (m_nFlags != 0)
        DestructorDispatch();
    m_nFlags = 0;
    delete[] static_cast<Vector*>(m_pVertexSource);
}

struct Bone {
    Part* m_pPart;
    int   m_nValue;

    void SetData(Part* pPart, int nValue);
};

void Bone::SetData(Part* pPart, int nValue)
{
    m_pPart = pPart;
    m_nValue = nValue;
}

PartSkin::~PartSkin()
{
}


MdlNodeTriMesh_PostProcess::MdlNodeTriMesh_PostProcess()
{
}

void Gob::CollectLights(List<PartLight*>& lights)
{
    lights.m_count = 0;
    List<Part*> stackParts;
    stackParts.Add(m_pPart);
    for (int i = 0; i < stackParts.m_count; i++)
    {
        Part* pPart = stackParts[i];
        PartLight* pLight = pPart->AsPartLight();
        if (pLight != 0)
            lights.Add(pLight);
        for (int j = 0; j < pPart->m_nChildren; j++)
            stackParts.Add(pPart->m_paChildren[j]);
    }
}

int PartHasMaterial(CAurPart* pPart)
{
    return pPart->m_pMaterial != 0;
}

void Gob::EnableRenderBumpedOut(char* pszTexture, float fBumpDist)
{
    m_cRenderBumpedOut = 1;
    List<CAurPart*> listParts;
    EnumParts(&PartHasMaterial, &listParts);

    int nSize = 0;
    char szBlending[32];
    szBlending[0] = 0;
    int nBlendMode50 = 0;
    int nBlendMode60 = 1;

    CAurTexture* pTexture = AurTextureGetReference(pszTexture, 0);
    if (pTexture->IsTxiLoaded())
    {
        unsigned char* pTxi = pTexture->GetTxiData();
        int nLength = pTexture->GetTxiSize();
        if (pTxi != 0 && nLength >= 1)
        {
            char szLine[8192];
            int nRemaining = nLength;
            do {
                int64_t i = 0;
                unsigned char c;
                do {
                    c = pTxi[i];
                    szLine[i++] = c;
                } while (i < nRemaining && c != '\n');
                nRemaining -= static_cast<int>(i);
                pTxi += i;
                if (nRemaining < 0)
                    break;
                szLine[i] = 0;
                char* pszLine = szLine;
                while (*pszLine == ' ' || *pszLine == '\t')
                    pszLine++;
                if (_stricmp("blending", firstword(pszLine)) == 0)
                    Parse(pszLine + __strlen_chk("blending", sizeof("blending")), szBlending);
                if (szBlending[0] != 0) {
                    if (memcmp(szBlending, "additive", sizeof("additive")) == 0) {
                        nBlendMode50 = 0;
                        nBlendMode60 = 2;
                    } else if (memcmp(szBlending, "punchthrough", sizeof("punchthrough")) == 0) {
                        nBlendMode50 = 2;
                        nBlendMode60 = 3;
                    }
                }
            } while (nRemaining > 0);
        }
    }
    else
    {
        void* pRes = AurResGet(pszTexture, const_cast<char*>(".txi"), &nSize, true);
        if (pRes == 0)
        {
            pTexture->SetTxiLoaded(true);
            return;
        }
        int nCount = (nSize < 0) ? ~nSize : nSize;
        if (nCount > 0)
        {
            pTexture->SetTxiData(AurResGetDataBytes(0, pRes), nCount);
            char* pszLine;
            while ((pszLine = AurResGetNextLine()) != 0)
            {
                while (*pszLine == ' ' || *pszLine == '\t')
                    pszLine++;
                if (_stricmp("blending", firstword(pszLine)) == 0)
                    Parse(pszLine + __strlen_chk("blending", sizeof("blending")), szBlending);
                if (szBlending[0] != 0) {
                    if (memcmp(szBlending, "additive", sizeof("additive")) == 0) {
                        nBlendMode50 = 0;
                        nBlendMode60 = 2;
                    } else if (memcmp(szBlending, "punchthrough", sizeof("punchthrough")) == 0) {
                        nBlendMode50 = 2;
                        nBlendMode60 = 3;
                    }
                }
            }
        }
        AurResFree(pRes, nSize);
        pTexture->SetTxiLoaded(true);
    }

    for (int i = 0; i < listParts.m_count; i++)
    {
        CAurTexture* pOld = listParts[i]->m_pMaterial->m_spr3;
        if (pOld != 0)
        {
            listParts[i]->m_pMaterial->m_spr3 = 0;
            AurTextureRelease(pOld);
        }
        listParts[i]->m_pMaterial->m_spr3 = pTexture;
        nSize = 0;
        listParts[i]->m_pMaterial->m_lstSrcBlend.SetSize(2);
        listParts[i]->m_pMaterial->m_lstDstBlend.SetSize(2);
        listParts[i]->m_pMaterial->m_lstSrcBlend.m_pData[1] = Aurora::AuroraBlendModes(nBlendMode50);
        listParts[i]->m_pMaterial->m_lstDstBlend.m_pData[1] = Aurora::AuroraBlendModes(nBlendMode60);
    }

    m_fBumpOutDistance = fBumpDist;
}

void Gob::DisableRenderBumpedOut()
{
    if (m_cRenderBumpedOut == 0)
        return;
    m_cRenderBumpedOut = 0;
    List<CAurPart*> listParts;
    EnumParts(&PartHasMaterial, &listParts);
    for (int i = 0; i < listParts.m_count; i++)
    {
        Material* pMaterial = listParts[i]->m_pMaterial;
        CAurTexture* pTexture = pMaterial->m_spr3;
        pMaterial->m_spr3 = 0;
        AurTextureRelease(pTexture);
    }
    m_fBumpOutDistance = 0.0f;
}

float Gob::GetMaximumLightRadius()
{
    List<PartLight*> listLights;
    CollectLights(listLights);
    float fMax = 0.0f;
    for (int i = 0; i < listLights.m_count; i++)
    {
        float fRadius = listLights[i]->m_fLightRadius;
        fMax = fRadius > fMax ? fRadius : fMax;
    }
    return fMax;
}

static void CollectPartLights(Part* pPart, List<PartLight*>& lights);

void Gob::SetLightPriority(int nPriority)
{
    List<PartLight*> listLights;
    CollectPartLights(m_pPart, listLights);
    for (int i = 0; i < listLights.m_count; i++)
        listLights[i]->m_nLightPriority = nPriority;
}

static void CollectPartLights(Part* pPart, List<PartLight*>& lights)
{
    PartLight* pLight = pPart->AsPartLight();
    if (pLight != 0)
        lights.Add(pLight);
    for (int i = 0; i < pPart->m_nChildren; i++)
        CollectPartLights(pPart->m_paChildren[i], lights);
}

char* enablebumpout(char* pszArgs)
{
    static char szBuf[256];
    char szGobName[64];
    char szTexture[64];
    float fBumpDist = 0.0f;
    int nFields = sscanf(pszArgs, "%s%f%s", szGobName, &fBumpDist, szTexture);
    if ((nFields | 1) != 3)
        return const_cast<char*>("incorrect parameters  usage: enablebumpout gob bumpdist texture");
    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
    {
        snprintff(szBuf, 128, "Object '%s' not found.", szGobName);
        return szBuf;
    }
    pGob->EnableRenderBumpedOut(szTexture, fBumpDist);
    return const_cast<char*>("success");
}

char* disablebumpout(char* pszArgs)
{
    static char szBuf[256];
    char szGobName[64];
    if (sscanf(pszArgs, "%s", szGobName) != 1)
        return const_cast<char*>("Incorrect parameters  Usage: disablebumpout gobName");
    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
    {
        snprintff(szBuf, 128, "Object '%s' not found.", szGobName);
        return szBuf;
    }
    pGob->DisableRenderBumpedOut();
    return const_cast<char*>("success");
}

char* getmaximumlightradius(char* pszArgs)
{
    static char szBuf[256];
    char szGobName[64];
    if (sscanf(pszArgs, "%s", szGobName) != 1)
        return const_cast<char*>("Failure: getmaximumlightradius requires a gob name");
    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return const_cast<char*>("That gob cannot be found!");
    snprintff(szBuf, 256, "Maximum light radius for %s is %.2f", szGobName,
              pGob->GetMaximumLightRadius());
    return szBuf;
}


char lastspawnedobject[64];
char* FuncInterp(char* pszExpr);
char* bload(char* pszName);
char* controlpart(char* pszArgs)
{
    char szGobName[64];
    char szPartName[64];
    int nOffset;
    szPartName[0] = '\0';
    szGobName[0] = '\0';
    if (sscanf(pszArgs, "%s%s%n", szGobName, szPartName, &nOffset) < 2 || szGobName[0] == '\0' ||
        szPartName[0] == '\0')
        return const_cast<char*>("usage:  setcolor objectname partname r g b");
    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return const_cast<char*>("no object by that name");
    Part* pPart = pGob->GetPart(szPartName);
    if (pPart == 0)
        return const_cast<char*>("no part by that name");
    MdlNode* pNode = pPart->m_pNode->ConJoinDispatch();
    pNode->ParseFieldDispatch(skipspace(pszArgs + nOffset));
    pNode->ControlDispatch(pPart, 0.0f, 0.0f, 1.0f);
    delete pNode;
    return const_cast<char*>("OK");
}

int AurControlPart(char* pszArgs)
{
    char szGobName[64];
    char szPartName[64];
    int nOffset;
    szPartName[0] = '\0';
    szGobName[0] = '\0';
    if (sscanf(pszArgs, "%s%s%n", szGobName, szPartName, &nOffset) < 2 || szGobName[0] == '\0' ||
        szPartName[0] == '\0')
        return 0;
    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return 0;
    Part* pPart = pGob->GetPart(szPartName);
    if (pPart == 0)
        return 0;
    MdlNode* pNode = pPart->m_pNode->ConJoinDispatch();
    pNode->ParseFieldDispatch(skipspace(pszArgs + nOffset));
    pNode->ControlDispatch(pPart, 0.0f, 0.0f, 1.0f);
    delete pNode;
    return 1;
}

char* attach(char* pszArgs)
{
    static char szBuf[128];
    char szGobName[64];
    char szThingName[64] K2_UNSET;
    char szPartName[64];
    szPartName[0] = '\0';
#ifdef K2_FIX_ENGINE_BUGS
    if ((sscanf(pszArgs, "%63s%63s%63s", szGobName, szThingName, szPartName) | 1) != 3)
#else
    if ((sscanf(pszArgs, "%s%s%s", szGobName, szThingName, szPartName) | 1) != 3)
#endif
        return const_cast<char*>("incorrect parameters  usage: attach thing object [part]");
    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
    {
        snprintff(szBuf, 128, "Object '%s' not found.", szGobName);
        return szBuf;
    }
    if (_stricmp(szThingName, "null") == 0)
    {
        pGob->SetBehavior(0);
        Scene* pOldScene = pGob->m_pScene;
        if (pOldScene != 0)
        {
            pOldScene->ChildRemove(pGob);
            pGob->m_pScene = 0;
            pGob->DoEvent(const_cast<char*>("UpdateScenes"), 0);
        }
        return const_cast<char*>("unattached object from everything");
    }
    Scene* pScene = FindScene(szThingName);
    if (pScene != 0)
    {
        if (pGob->m_pScene != pScene)
        {
            if (pGob->m_pScene != 0)
                pGob->m_pScene->ChildRemove(pGob);
            pGob->m_pScene = pScene;
            pScene->ChildAdd(pGob);
            pGob->DoEvent(const_cast<char*>("UpdateScenes"), 0);
        }
        return const_cast<char*>("attached to scene");
    }
    Gob* pTarget = FindGob(szThingName);
    if (pTarget == 0)
    {
        snprintff(szBuf, 128, "Object '%s' not found.", szThingName);
        return szBuf;
    }
    if (pGob->AttachToObject(static_cast<CAurObject*>(pTarget), szPartName, 0) == 0)
        snprintff(szBuf, 128, "Invalid target for attachment: '%s %s'.", szThingName, szPartName);
    else
        return const_cast<char*>("success");
    return szBuf;
}

char* lastspawned(char* pszArgs)
{
    K2_UNUSED(pszArgs);
    return lastspawnedobject;
}

char* spawn(char* pszArgs)
{
    char szResRef[128];
    char szParam[128];
    szResRef[0] = '\0';
    szParam[0] = '\0';
    static_cast<void>(sscanf(pszArgs, "%s %s", szResRef, szParam));
    CAurObject* pObject = NewCAurObject(szResRef, szParam, 0, 0);
    __strcpy_chk(lastspawnedobject, pObject != 0 ? static_cast<char*>(pObject->GetName()) : const_cast<char*>(NULL_NAME), 64);
    FuncInterp(const_cast<char*>("attach [lastspawned] scene"));
    return pObject != 0 ? const_cast<char*>("success") : const_cast<char*>("failure");
}

char* spawnbload(char* pszArgs)
{
    char szResRef[128];
    char szParam[128];
    szResRef[0] = '\0';
    szParam[0] = '\0';
    static_cast<void>(sscanf(pszArgs, "%s %s", szResRef, szParam));
    char* pszLoaded = bload(szResRef);
    if (_stricmp(pszLoaded, "FAILURE") == 0)
        return const_cast<char*>("FAILURE");
    CAurObject* pObject = NewCAurObject(szResRef, szParam, 0, 0);
    __strcpy_chk(lastspawnedobject, pObject != 0 ? static_cast<char*>(pObject->GetName()) : const_cast<char*>(NULL_NAME), 64);
    FuncInterp(const_cast<char*>("attach [lastspawned] scene"));
    return pObject != 0 ? const_cast<char*>("success") : const_cast<char*>("failure");
}

char* setgobscheme(char* pszArgs)
{
    char szGobName[64];
    char szTexture[64];
    char szPltName[64];
    if (sscanf(pszArgs, "%s %s %s", szGobName, szTexture, szPltName) != 3)
        return const_cast<char*>("Failure: setgobscheme requires name, texture, and pltname!");
    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return const_cast<char*>("That gob cannot be found!");
    unsigned short aPalette[256];
    for (int i = 0; i < 256; i++)
        aPalette[i] = static_cast<unsigned short>(rand_wincompatible() % 10);
    if (pGob->ReplaceTexture(szTexture, szPltName, 256, aPalette, 0) != 0)
        return const_cast<char*>("Success");
    return const_cast<char*>("Failure");
}

char* setprimarygob(char* pszArgs)
{
    char szGobName[64];
    int nPriority = 0;
    if (sscanf(pszArgs, "%s%d", szGobName, &nPriority) != 2)
        return const_cast<char*>("Failure: setprimarygob requires a gob name and a light priority");
    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return const_cast<char*>("That gob cannot be found!");
    pGob->SetLightPriority(nPriority);
    return const_cast<char*>("Success, set the primary gob");
}

char* setbboxongob(char* pszArgs)
{
    char szGobName[64];
    int nOnOff = 0;
    Vector vColor;
    int nFields = sscanf(pszArgs, "%s %d %f %f %f", szGobName, &nOnOff, &vColor.x, &vColor.y,
                         &vColor.z);
    if (nFields != 5 && nFields != 2)
        return const_cast<char*>("USAGE: setbboxongob <gobname> <1/0 (on/off)> <color>");
    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return const_cast<char*>("That gob cannot be found!");
    if (nOnOff == 0)
        pGob->DisableRenderGobBBox();
    else
        pGob->EnableRenderGobBBox(vColor);
    return const_cast<char*>("Success.");
}

char* setgobwind(char* pszArgs)
{
    char szGobName[64];
    float fIntensity;
    if (sscanf(pszArgs, "%s %f", szGobName, &fIntensity) != 2)
        return const_cast<char*>("Usage: setgobwind gobname intensity");
    Gob* pGob = FindGob(szGobName);
    if (pGob == 0)
        return const_cast<char*>("That gob cannot be found!");
    pGob->SetWind(true, fIntensity, 0.0f, 0.0f);
    return const_cast<char*>("Success.");
}




extern CAurTexture* AurTextureGetReference(char* sName, char* sExtension);
void AurTextureRelease(CAurTexture* pTexture);

int countnode = 0;
int countmodelnodes = 0;

// The node-kind words. They are external-linkage class statics declared without in-class
// initialisers, so they cannot be used as case labels: they are not integral constant expressions.
const unsigned short MdlNode::MDLNODE_BASE_ID = NODE_DUMMY;
const unsigned short MdlNode::MDLNODE_LIGHT_ID = NODE_LIGHT;
const unsigned short MdlNode::MDLNODE_AABB_ID = NODE_AABB;
const unsigned short MdlNode::MDLNODE_ANIM_ID = NODE_ANIMMESH;
const unsigned short MdlNode::MDLNODE_CAMERA_ID = NODE_CAMERA;
const unsigned short MdlNode::MDLNODE_DANGLY_ID = NODE_DANGLYMESH;
const unsigned short MdlNode::MDLNODE_EMITTER_ID = NODE_EMITTER;
const unsigned short MdlNode::MDLNODE_REFERENCE_ID = NODE_REFERENCE;
const unsigned short MdlNode::MDLNODE_SKIN_ID = NODE_SKIN;
const unsigned short MdlNode::MDLNODE_TRIGGER_ID = NODE_TRIGGER;
const unsigned short MdlNode::MDLNODE_TRIMESH_ID = NODE_TRIMESH;
const unsigned short MdlNode::MDLNODE_LIGHTSABER_ID = NODE_SABER;


void MdlNodeParseField(void* pv, char* pszLine)
{
    static_cast<MdlNode*>(pv)->InternalParseField(pszLine);
}

void MdlNodePostProcess(void* pv)
{
    static_cast<MdlNode*>(pv)->InternalPostProcess();
}

void MdlNodeControl(void* pv, Part* pPart, float fA, float fB, float fC)
{
    static_cast<MdlNode*>(pv)->InternalControl(pPart, fA, fB, fC);
}


void MdlNodeLightDestructor(void* pv)
{
    static_cast<MdlNodeLight*>(pv)->InternalDestructor();
}

void MdlNodeLightParseField(void* pv, char* pszLine)
{
    static_cast<MdlNodeLight*>(pv)->InternalParseField(pszLine);
}

Part* MdlNodeLightCreateInstance(void* pv, Gob* pGob)
{
    return static_cast<MdlNodeLight*>(pv)->InternalCreateInstance(pGob);
}

void MdlNodeReferenceParseField(void* pv, char* pszLine)
{
    static_cast<MdlNodeReference*>(pv)->InternalParseField(pszLine);
}

void MdlNodeLightsaberParseField(void* pv, char* pszLine)
{
    static_cast<MdlNodeLightsaber*>(pv)->InternalParseField(pszLine);
}

void MdlNodeDanglyMeshParseField(void* pv, char* pszLine)
{
    static_cast<MdlNodeDanglyMesh*>(pv)->InternalParseField(pszLine);
}

void MdlNodeDanglyMeshPostProcess(void* pv)
{
    static_cast<MdlNodeDanglyMesh*>(pv)->InternalPostProcess();
}

void MdlNodeTriMeshParseField(void* pv, char* pszLine)
{
    static_cast<MdlNodeTriMesh*>(pv)->InternalParseField(pszLine);
}

void MdlNodeAABBParseField(void* pv, char* pszLine)
{
    static_cast<MdlNodeAABB*>(pv)->InternalParseField(pszLine);
}

MdlNode* MdlNodeConJoinTwinMysLexia(void* pv)
{
    return new MdlNode(static_cast<char*>(pv));
}

MdlNodeLight* MdlNodeLightConJoinTwinMysLexia(void* pv)
{
    return new MdlNodeLight(static_cast<char*>(pv));
}

MdlNodeReference* MdlNodeReferenceConJoinTwinMysLexia(void* pv)
{
    return new MdlNodeReference(static_cast<char*>(pv));
}

MdlNodeSkin* MdlNodeSkinConJoinTwinMysLexia(void* pv)
{
    return new MdlNodeSkin(static_cast<char*>(pv));
}

MdlNodeAnimMesh* MdlNodeAnimMeshConJoinTwinMysLexia(void* pv)
{
    return new MdlNodeAnimMesh(static_cast<char*>(pv));
}

MdlNodeTriMesh* MdlNodeTriMeshConJoinTwinMysLexia(void* pv)
{
    return new MdlNodeTriMesh(static_cast<char*>(pv));
}

Part* MdlNodeCreateInstance(void* pv, Gob* pGob)
{
    K2_UNUSED(pGob);
    Part* pPart = new Part;
    pPart->m_pNode = static_cast<MdlNode*>(pv);
    countpart++;
    return pPart;
}

Part* MdlNode::InternalCreateInstance(Gob* pGob)
{
    K2_UNUSED(pGob);
    Part* pPart = new Part;
    pPart->m_pNode = this;
    countpart++;
    return pPart;
}

MdlNode* MdlNode::InternalConJoin()
{
    return new MdlNode(0);
}

MdlNode::MdlNode(char*)
    : m_listChildren(), m_listControllers(), m_listFloats()
{
    m_pParent = 0;
    m_nNodeID = 0;
    m_nFlags = 1;
    countnode++;
    m_vPosition = Vector(0.0f, 0.0f, 0.0f);
    m_qOrientation = Quaternion();
    {
        // raw-offset: the three Lists cleared again as six words, in the original's order (typed
        // stores schedule differently)
        void** pq = reinterpret_cast<void**>(&m_listChildren);
        pq[5] = 0; pq[4] = 0; pq[3] = 0; pq[2] = 0; pq[1] = 0; pq[0] = 0;  // raw-offset: see above
    }
    countmodelnodes++;
}

MdlNode::~MdlNode()
{
    if (m_nFlags != 0)
        DestructorDispatch();

    int n = m_listChildren.m_count;
    m_nFlags = 0;
    if (n) {
        for (int64_t i = n - 1; i != -1; i--) {
            MdlNode* pChild = m_listChildren.m_pData[i];
            int nFlags = pChild->m_nFlags;

            if ((nFlags & NODE_LIGHT) == NODE_LIGHT && pChild) {
                static_cast<MdlNodeLight*>(pChild)->~MdlNodeLight();
                operator delete(pChild);
            } else if ((nFlags & NODE_CAMERA) == NODE_CAMERA && pChild) {
                pChild->~MdlNode();
                operator delete(pChild);
            } else if ((nFlags & NODE_DANGLYMESH) == NODE_DANGLYMESH && pChild) {
                static_cast<MdlNodeDanglyMesh*>(pChild)->~MdlNodeDanglyMesh();
                operator delete(pChild);
            } else if ((nFlags & NODE_ANIMMESH) == NODE_ANIMMESH && pChild) {
                static_cast<MdlNodeAnimMesh*>(pChild)->~MdlNodeAnimMesh();
                operator delete(pChild);
            } else if ((nFlags & NODE_EMITTER) == NODE_EMITTER && pChild) {
                pChild->~MdlNode();
                operator delete(pChild);
            } else if ((nFlags & NODE_REFERENCE) == NODE_REFERENCE && pChild) {
                pChild->~MdlNode();
                operator delete(pChild);
            } else if ((nFlags & NODE_AABB) == NODE_AABB && pChild) {
                static_cast<MdlNodeAABB*>(pChild)->~MdlNodeAABB();
                operator delete(pChild);
            } else if ((nFlags & NODE_SKIN) == NODE_SKIN && pChild) {
                static_cast<MdlNodeSkin*>(pChild)->~MdlNodeSkin();
                operator delete(pChild);
            } else if ((nFlags & NODE_TRIGGER) == NODE_TRIGGER && pChild) {
                pChild->~MdlNode();
                operator delete(pChild);
            } else if ((nFlags & NODE_SABER) == NODE_SABER && pChild) {
                static_cast<MdlNodeLightsaber*>(pChild)->~MdlNodeLightsaber();
                operator delete(pChild);
            } else if ((nFlags & NODE_TRIMESH) == NODE_TRIMESH && pChild) {
                static_cast<MdlNodeTriMesh*>(pChild)->~MdlNodeTriMesh();
                operator delete(pChild);
            } else if (pChild) {
                pChild->~MdlNode();
                operator delete(pChild);
            }
        }
    }

    countmodelnodes--;

}

void MdlNode::AddToFreeList(MdlNode* pNode)
{
    m_pTree->m_freeList.Add(pNode);
}

void MdlNode::AddToFreeTokenList(unsigned int nToken)
{
    m_pTree->m_freeTokens.Add(nToken);
}

void MdlNode::InternalDestructor()
{
    for (int64_t i = m_listChildren.m_count - 1; i != -1; i--)
        m_listChildren.m_pData[i]->DestructorDispatch();
    countnode--;
}

void MdlNodeDestructor(void* pv)
{
    static_cast<MdlNode*>(pv)->InternalDestructor();
}

void MdlNodeLight::InternalDestructor()
{
    int n = FlareCount();
    if (n) {
        int i = n;
        int64_t l = i;
        do {
            l--;
            CAurTexture** pSlot = &FlareTextures()[l];
            CAurTexture* pTexture = *pSlot;
            if (pTexture) {
                pTexture->Remove(reinterpret_cast<SafePointer<CAurTexture>*>(pSlot));
            }
            *pSlot = 0;
            AurTextureRelease(pTexture);
            FlareCount()--;
        } while (l);
    }
    for (int64_t i = m_listChildren.m_count - 1; i != -1; i--)
        m_listChildren.m_pData[i]->DestructorDispatch();
    countnode--;
}

#if KOTOR1  // K1: every tri-mesh node teardown first tells the port layer it is going away
void IOS_MdlNodeTriMeshDestroyed(MdlNodeTriMesh* pNode);
#define MDLNODE_PLAIN_INTERNAL_DTOR(classname)                                            \
    void classname::InternalDestructor()                                                   \
    {                                                                                      \
        IOS_MdlNodeTriMeshDestroyed(this);                                                 \
        for (int64_t i = m_listChildren.m_count - 1; i != -1; i--)                             \
            m_listChildren.m_pData[i]->DestructorDispatch();                               \
        countnode--;                                                                        \
    }
#else
#define MDLNODE_PLAIN_INTERNAL_DTOR(classname)                                            \
    /* see MdlNode::InternalDestructor for the shared body */                              \
    void classname::InternalDestructor()                                                   \
    {                                                                                      \
        for (int64_t i = m_listChildren.m_count - 1; i != -1; i--)                             \
            m_listChildren.m_pData[i]->DestructorDispatch();                               \
        countnode--;                                                                        \
    }
#endif

MDLNODE_PLAIN_INTERNAL_DTOR(MdlNodeTriMesh)

MDLNODE_PLAIN_INTERNAL_DTOR(MdlNodeAnimMesh)

MDLNODE_PLAIN_INTERNAL_DTOR(MdlNodeDanglyMesh)

MDLNODE_PLAIN_INTERNAL_DTOR(MdlNodeSkin)

MDLNODE_PLAIN_INTERNAL_DTOR(MdlNodeAABB)

void MdlNodeAnimMeshDestructor(void* pv)
{
    static_cast<MdlNodeAnimMesh*>(pv)->InternalDestructor();
}

void MdlNodeSkinDestructor(void* pv)
{
    static_cast<MdlNodeSkin*>(pv)->InternalDestructor();
}

void MdlNodeLightsaber::InternalDestructor()
{
    GLRender::DeletePool(m_anBladePools[0]);
    int n = -1;
    m_anBladePools[0] = n;
    GLRender::DeletePool(m_anBladePools[1]);
    m_anBladePools[1] = n;
#if KOTOR1  // K1 tells the port layer the mesh is going away
    IOS_MdlNodeTriMeshDestroyed(this);
#endif
    for (int64_t i = m_listChildren.m_count - 1; i != -1; i--)
        m_listChildren.m_pData[i]->DestructorDispatch();
    countnode--;
}


MdlNodeCamera* MdlNode::AsMdlNodeCamera()
{
    if ((m_nFlags & NODE_CAMERA) != NODE_CAMERA)
        return 0;
    return static_cast<MdlNodeCamera*>(this);
}

MdlNodeDanglyMesh* MdlNode::AsMdlNodeDanglyMesh()
{
    if ((m_nFlags & NODE_DANGLYMESH) != NODE_DANGLYMESH)
        return 0;
    return static_cast<MdlNodeDanglyMesh*>(this);
}

MdlNodeAnimMesh* MdlNode::AsMdlNodeAnimMesh()
{
    if ((m_nFlags & NODE_ANIMMESH) != NODE_ANIMMESH)
        return 0;
    return static_cast<MdlNodeAnimMesh*>(this);
}

MdlNodeEmitter* MdlNode::AsMdlNodeEmitter()
{
    if ((m_nFlags & NODE_EMITTER) != NODE_EMITTER)
        return 0;
    return static_cast<MdlNodeEmitter*>(this);
}


MdlNodeReference* MdlNode::AsMdlNodeReference()
{
    if ((m_nFlags & NODE_REFERENCE) != NODE_REFERENCE)
        return 0;
    return static_cast<MdlNodeReference*>(this);
}

MdlNodeAABB* MdlNode::AsMdlNodeAABB()
{
    if ((m_nFlags & NODE_AABB) != NODE_AABB)
        return 0;
    return static_cast<MdlNodeAABB*>(this);
}

MdlNodeSkin* MdlNode::AsMdlNodeSkin()
{
    if ((m_nFlags & NODE_SKIN) != NODE_SKIN)
        return 0;
    return static_cast<MdlNodeSkin*>(this);
}

MdlNodeTrigger* MdlNode::AsMdlNodeTrigger()
{
    if ((m_nFlags & NODE_TRIGGER) != NODE_TRIGGER)
        return 0;
    return static_cast<MdlNodeTrigger*>(this);
}

MdlNodeLightsaber* MdlNode::AsMdlNodeLightsaber()
{
    if ((m_nFlags & NODE_SABER) != NODE_SABER)
        return 0;
    return static_cast<MdlNodeLightsaber*>(this);
}

MdlNodeTriMesh* MdlNode::AsMdlNodeTriMesh()
{
    if ((m_nFlags & NODE_TRIMESH) != NODE_TRIMESH)
        return 0;
    return static_cast<MdlNodeTriMesh*>(this);
}

Animation* MaxTree::AsAnimation()
{
    if ((m_cType & kTreeTypeMask) != kTreeAnimation)
        return 0;
    return static_cast<Animation*>(this);
}


bool MdlNode::IsMdlNodeAABB()
{
    return (m_nFlags & NODE_AABB) == NODE_AABB;
}

bool MdlNode::IsMdlNodeAnimMesh()
{
    return (m_nFlags & NODE_ANIMMESH) == NODE_ANIMMESH;
}

bool MdlNode::IsMdlNodeCamera()
{
    return (m_nFlags & NODE_CAMERA) == NODE_CAMERA;
}

bool MdlNode::IsMdlNodeDanglyMesh()
{
    return (m_nFlags & NODE_DANGLYMESH) == NODE_DANGLYMESH;
}

bool MdlNode::IsMdlNodeEmitter()
{
    return (m_nFlags & NODE_EMITTER) == NODE_EMITTER;
}

bool MdlNode::IsMdlNodeLight()
{
    return (m_nFlags & NODE_LIGHT) == NODE_LIGHT;
}

bool MdlNode::IsMdlNodeReference()
{
    return (m_nFlags & NODE_REFERENCE) == NODE_REFERENCE;
}

bool MdlNode::IsMdlNodeSkin()
{
    return (m_nFlags & NODE_SKIN) == NODE_SKIN;
}

bool MdlNode::IsMdlNodeTrigger()
{
    return (m_nFlags & NODE_TRIGGER) == NODE_TRIGGER;
}

bool MdlNode::IsMdlNodeTriMesh()
{
    return (m_nFlags & NODE_TRIMESH) == NODE_TRIMESH;
}

__attribute__((flatten)) void MdlNode::DestructorDispatch()
{
    switch (static_cast<short>(m_nFlags)) {
    case NODE_DUMMY:
        InternalDestructor();
        break;
    case NODE_LIGHT:
        AsMdlNodeLight()->InternalDestructor();
        break;
    case NODE_EMITTER:
        AsMdlNodeEmitter()->InternalDestructor();
        break;
    case NODE_CAMERA:
        AsMdlNodeCamera()->InternalDestructor();
        break;
    case NODE_REFERENCE:
        AsMdlNodeReference()->InternalDestructor();
        break;
    case NODE_TRIMESH:
        AsMdlNodeTriMesh()->InternalDestructor();
        break;
    case NODE_SKIN:
        AsMdlNodeSkin()->InternalDestructor();
        break;
    case NODE_ANIMMESH:
        AsMdlNodeAnimMesh()->InternalDestructor();
        break;
    case NODE_DANGLYMESH: {
        MdlNodeDanglyMesh* pDangly = (m_nFlags & NODE_DANGLYMESH) == NODE_DANGLYMESH ? static_cast<MdlNodeDanglyMesh*>(this) : static_cast<MdlNodeDanglyMesh*>(0);
        pDangly->InternalDestructor();
        break;
    }
    case NODE_AABB:
        AsMdlNodeAABB()->InternalDestructor();
        break;
    case NODE_TRIGGER:
        AsMdlNodeTrigger()->InternalDestructor();
        break;
    case NODE_SABER:
        AsMdlNodeLightsaber()->InternalDestructor();
        break;
    }
}

void MdlNode::ControlDispatch(Part* pPart, float fA, float fB, float fC)
{
    switch (static_cast<short>(m_nFlags)) {
    case NODE_DUMMY:
        InternalControl(pPart, fA, fB, fC);
        break;
    case NODE_LIGHT:
        AsMdlNodeLight()->InternalControl(pPart, fA, fB, fC);
        break;
    case NODE_EMITTER:
        AsMdlNodeEmitter()->InternalControl(pPart, fA, fB, fC);
        break;
    case NODE_CAMERA:
        AsMdlNodeCamera()->InternalControl(pPart, fA, fB, fC);
        break;
    case NODE_REFERENCE:
        AsMdlNodeReference()->InternalControl(pPart, fA, fB, fC);
        break;
    case NODE_TRIMESH:
        AsMdlNodeTriMesh()->InternalControl(pPart, fA, fB, fC);
        break;
    case NODE_SKIN:
        AsMdlNodeSkin()->InternalControl(pPart, fA, fB, fC);
        break;
    case NODE_ANIMMESH:
        AsMdlNodeAnimMesh()->InternalControl(pPart, fA, fB, fC);
        break;
    case NODE_DANGLYMESH:
        AsMdlNodeDanglyMesh()->InternalControl(pPart, fA, fB, fC);
        break;
    case NODE_AABB:
        AsMdlNodeAABB()->InternalControl(pPart, fA, fB, fC);
        break;
    case NODE_TRIGGER:
        AsMdlNodeTrigger()->InternalControl(pPart, fA, fB, fC);
        break;
    case NODE_SABER:
        AsMdlNodeLightsaber()->InternalControl(pPart, fA, fB, fC);
        break;
    }
}

void MdlNode::ParseFieldDispatch(char* pszLine)
{
    switch (static_cast<short>(m_nFlags)) {
    case NODE_DUMMY:
        InternalParseField(pszLine);
        break;
    case NODE_LIGHT:
        AsMdlNodeLight()->InternalParseField(pszLine);
        break;
    case NODE_EMITTER:
        AsMdlNodeEmitter()->InternalParseField(pszLine);
        break;
    case NODE_CAMERA:
        AsMdlNodeCamera()->InternalParseField(pszLine);
        break;
    case NODE_REFERENCE:
        AsMdlNodeReference()->InternalParseField(pszLine);
        break;
    case NODE_TRIMESH:
        AsMdlNodeTriMesh()->InternalParseField(pszLine);
        break;
    case NODE_SKIN:
        AsMdlNodeSkin()->InternalParseField(pszLine);
        break;
    case NODE_ANIMMESH:
        AsMdlNodeAnimMesh()->InternalParseField(pszLine);
        break;
    case NODE_DANGLYMESH: {
        MdlNodeDanglyMesh* pDangly = (m_nFlags & NODE_DANGLYMESH) == NODE_DANGLYMESH ? static_cast<MdlNodeDanglyMesh*>(this) : static_cast<MdlNodeDanglyMesh*>(0);
        pDangly->InternalParseField(pszLine);
        break;
    }
    case NODE_AABB:
        AsMdlNodeAABB()->InternalParseField(pszLine);
        break;
    case NODE_TRIGGER:
        AsMdlNodeTrigger()->InternalParseField(pszLine);
        break;
    case NODE_SABER:
        AsMdlNodeLightsaber()->InternalParseField(pszLine);
        break;
    }
}

void MdlNode::PostProcessDispatch()
{
    switch (static_cast<short>(m_nFlags)) {
    case NODE_DUMMY:
        InternalPostProcess();
        break;
    case NODE_LIGHT:
        AsMdlNodeLight()->InternalPostProcess();
        break;
    case NODE_EMITTER:
        AsMdlNodeEmitter()->InternalPostProcess();
        break;
    case NODE_CAMERA:
        AsMdlNodeCamera()->InternalPostProcess();
        break;
    case NODE_REFERENCE:
        AsMdlNodeReference()->InternalPostProcess();
        break;
    case NODE_TRIMESH:
        AsMdlNodeTriMesh()->InternalPostProcess();
        break;
    case NODE_SKIN:
        AsMdlNodeSkin()->InternalPostProcess();
        break;
    case NODE_ANIMMESH:
        AsMdlNodeAnimMesh()->InternalPostProcess();
        break;
    case NODE_DANGLYMESH:
        AsMdlNodeDanglyMesh()->InternalPostProcess();
        break;
    case NODE_AABB:
        AsMdlNodeAABB()->InternalPostProcess();
        break;
    case NODE_TRIGGER:
        AsMdlNodeTrigger()->InternalPostProcess();
        break;
    case NODE_SABER:
        AsMdlNodeLightsaber()->InternalPostProcess();
        break;
    }
}

Part* MdlNode::CreateInstanceDispatch(Gob* pGob)
{
    switch (static_cast<short>(m_nFlags)) {
    case NODE_DUMMY:
        return InternalCreateInstance(pGob);
    case NODE_LIGHT:
        return AsMdlNodeLight()->InternalCreateInstance(pGob);
    case NODE_EMITTER:
        return AsMdlNodeEmitter()->InternalCreateInstance(pGob);
    case NODE_CAMERA:
        return AsMdlNodeCamera()->InternalCreateInstance(pGob);
    case NODE_REFERENCE:
        return AsMdlNodeReference()->InternalCreateInstance(pGob);
    case NODE_TRIMESH:
        return AsMdlNodeTriMesh()->InternalCreateInstance(pGob);
    case NODE_SKIN:
        return AsMdlNodeSkin()->InternalCreateInstance(pGob);
    case NODE_ANIMMESH:
        return AsMdlNodeAnimMesh()->InternalCreateInstance(pGob);
    case NODE_DANGLYMESH:
        return AsMdlNodeDanglyMesh()->InternalCreateInstance(pGob);
    case NODE_AABB:
        return AsMdlNodeAABB()->InternalCreateInstance(pGob);
    case NODE_TRIGGER:
        return AsMdlNodeTrigger()->InternalCreateInstance(pGob);
    case NODE_SABER:
        return AsMdlNodeLightsaber()->InternalCreateInstance(pGob);
    }
    return 0;
}

MdlNode* MdlNode::ConJoinDispatch()
{
    switch (static_cast<short>(m_nFlags)) {
    case NODE_DUMMY:
        return InternalConJoin();
    case NODE_LIGHT:
        return AsMdlNodeLight()->InternalConJoin();
    case NODE_EMITTER:
        return AsMdlNodeEmitter()->InternalConJoin();
    case NODE_CAMERA:
        return AsMdlNodeCamera()->InternalConJoin();
    case NODE_REFERENCE:
        return AsMdlNodeReference()->InternalConJoin();
    case NODE_TRIMESH:
        return AsMdlNodeTriMesh()->InternalConJoin();
    case NODE_SKIN:
        return AsMdlNodeSkin()->InternalConJoin();
    case NODE_ANIMMESH:
        return AsMdlNodeAnimMesh()->InternalConJoin();
    case NODE_DANGLYMESH:
        return AsMdlNodeDanglyMesh()->InternalConJoin();
    case NODE_AABB:
        return AsMdlNodeAABB()->InternalConJoin();
    case NODE_TRIGGER:
        return AsMdlNodeTrigger()->InternalConJoin();
    case NODE_SABER:
        return AsMdlNodeLightsaber()->InternalConJoin();
    }
    return 0;
}

MdlNodeReference::MdlNodeReference(char* pszName) : MdlNode(pszName)
{
    m_nFlags = NODE_REFERENCE;
}

MdlNodeReference* MdlNodeReference::InternalConJoin()
{
    return new MdlNodeReference(nullptr);
}

MdlNodeCamera::MdlNodeCamera(char* pszName) : MdlNode(pszName)
{
    m_nFlags = 9;
}

MdlNodeCamera* MdlNodeCamera::InternalConJoin()
{
    return new MdlNodeCamera(nullptr);
}

MdlNode* MdlNodeCameraConJoinTwinMysLexia(void* pv)
{
    return new MdlNodeCamera(static_cast<char*>(pv));
}

MdlNodeLight::MdlNodeLight(char* pszName) : MdlNode(pszName)
{
    m_nFlags = MDLNODE_LIGHT_ID;
    m_nFadingEnabled = 1;
    m_bGenerateFlare = 0;
    m_bShadow = 1;
    m_bAffectDynamic = 1;
    m_bIsDynamic = 1;
    m_bAmbientOnly = 0;
    m_fFlareRange = 0.0f;
    m_nPriority = 5;
}

MdlNodeLight* MdlNodeLight::InternalConJoin()
{
    return new MdlNodeLight(nullptr);
}

Vector defaultdiffuse(0.8f, 0.8f, 0.8f);  // the engine default material colours
Vector defaultambient(0.2f, 0.2f, 0.2f);

void MdlNodeSkinGenVertices(void* pv);
void MdlNodeSkinRemoveTemporaryArrays(void* pv);
void MdlNodeAnimMeshGenVertices(void* pv);
void MdlNodeAnimMeshRemoveTemporaryArrays(void* pv);
void MdlNodeDanglyMeshGenVertices(void* pv);
void MdlNodeDanglyMeshRemoveTemporaryArrays(void* pv);


void MdlNodeDanglyMeshGenVertices(void* pv)
{
    static_cast<MdlNodeDanglyMesh*>(pv)->InternalGenVertices();
}

void MdlNodeDanglyMeshRemoveTemporaryArrays(void* pv)
{
    static_cast<MdlNodeDanglyMesh*>(pv)->InternalRemoveTemporaryArrays();
}

void MdlNodeTriMeshGenVertices(void* pv)
{
    static_cast<MdlNodeTriMesh*>(pv)->InternalGenVertices();
}
void MdlNodeTriMeshRemoveTemporaryArrays(void* pv)
{
    static_cast<MdlNodeTriMesh*>(pv)->InternalRemoveTemporaryArrays();
}

MdlNodeTriMesh::MdlNodeTriMesh(char* pszName) : MdlNode(pszName)
{
    __builtin_memset(static_cast<void*>(&m_faces), 0,
                     sizeof(m_faces) + sizeof(m_vMin) + sizeof(float));
    __builtin_memset(&m_vMax.y, 0, sizeof(m_vMax) - sizeof(float));
    m_fRadius = 0.0f;
    __builtin_memset(&m_vCentroid, 0,
                     sizeof(float) + sizeof(float));
    m_vCentroid.z = 0.0f;
    m_szTexture[0] = 0;
    m_szLightmap[0] = 0;
    __builtin_memset(static_cast<void*>(&m_lVertexIndices), 0, 5 * (sizeof(void*) + 2 * sizeof(int)));
    m_nLeftOverPool = static_cast<unsigned int>(-1);
    m_nLeftOverIndexCount = 0;
    m_pPostProcess = 0;
    m_nAnimateUV = 0;
    m_nVertexStride = -1;
    m_uVertexFlags = 0;
// Every unbound element offset is disabled, i.e. -1, so the fill is all ones rather than zero.
    __builtin_memset(&m_nVertexOfs, -1,
                     sizeof(m_nVertexOfs) + sizeof(m_nNormalOfs) + sizeof(int)
                     + 4 * sizeof(m_nTex0Ofs) + sizeof(m_nTangentOfs)
                     + 3 * sizeof(m_nReserved1));
    m_nTransparencyHint = 0;
    m_bHasLightmap = 0;
    m_bRotateTexture = 0;
    m_bIsBackgroundGeometry = 0;
    m_bShadow = 1;
    m_fSurfaceArea = 0;
    m_bBeaming = 0;
    m_nReserved4 = 0;
    m_vDiffuse = defaultdiffuse;
    m_vAmbient = defaultambient;
    m_bRender = 1;
#if !KOTOR1  // K2-only fields
    m_bDirtEnabled = 0;
#endif
    m_nVertexCount = 0;
    m_nTexCoordSets = 0;
#if !KOTOR1  // K2-only fields
    m_nDirtTexture = 1;
    m_nDirtWorldspace = 1;
    m_bHologramDoNotDraw = 0;
#endif
    m_nGeomBase = 0;
    m_paVertices = 0;
    m_nFlags = MDLNODE_TRIMESH_ID;
    m_pfnGenVertices = MdlNodeTriMeshGenVertices;
    m_pfnRemoveTemporaryArrays = MdlNodeTriMeshRemoveTemporaryArrays;
}

MdlNodeTriMesh* MdlNodeTriMesh::InternalConJoin()
{
    return new MdlNodeTriMesh(nullptr);
}

MdlNodeSkin::MdlNodeSkin(char* pszName) : MdlNodeTriMesh(pszName)
{
// A real seed, since PartSkin::PostProcess fills a slot only while it still reads -1.
    m_nBoneWeightsOfs = 0xffffffff;
    m_nBoneIndicesOfs = 0xffffffff;
    __builtin_memset(static_cast<void*>(&m_lQBoneRefInv), 0,
                     sizeof(m_lQBoneRefInv) + sizeof(m_lTBoneRefInv) + sizeof(m_lBoneConstantIndices));
    m_paBoneMap = 0;
    m_pfnGenVertices = MdlNodeSkinGenVertices;
    m_pfnRemoveTemporaryArrays = MdlNodeSkinRemoveTemporaryArrays;
    m_nFlags = MDLNODE_SKIN_ID;
}

MdlNodeSkin* MdlNodeSkin::InternalConJoin()
{
    return new MdlNodeSkin(nullptr);
}

MdlNodeAnimMesh::MdlNodeAnimMesh(char* pszName) : MdlNodeTriMesh(pszName)
{
    m_fSamplePeriod = 0.0f;
    m_paVertexFrames = 0;
    m_paTexCoordFrames = 0;
    m_nVertexFrameCount = 0;
    m_nTexCoordFrameCount = 0;
    m_pfnGenVertices = MdlNodeAnimMeshGenVertices;
    m_pfnRemoveTemporaryArrays = MdlNodeAnimMeshRemoveTemporaryArrays;
    m_nFlags = MDLNODE_ANIM_ID;
}

MdlNodeAnimMesh* MdlNodeAnimMesh::InternalConJoin()
{
    return new MdlNodeAnimMesh(nullptr);
}

MdlNodeDanglyMesh::MdlNodeDanglyMesh(char* pszName) : MdlNodeTriMesh(pszName)
{
    __builtin_memset(static_cast<void*>(&m_lConstraints), 0, sizeof(m_lConstraints));
    m_nFlags = MDLNODE_DANGLY_ID;
    m_pfnGenVertices = MdlNodeDanglyMeshGenVertices;
    m_pfnRemoveTemporaryArrays = MdlNodeDanglyMeshRemoveTemporaryArrays;
}

MdlNodeAABB::MdlNodeAABB(char* pszName) : MdlNodeTriMesh(pszName)
{
    m_nFlags = MDLNODE_AABB_ID;
    m_pAABB = 0;
    m_bRender = false;
    m_bShadow = false;
}

MdlNodeLightsaber::MdlNodeLightsaber(char* pszName) : MdlNodeTriMesh(pszName)
{
    m_nFlags = MDLNODE_LIGHTSABER_ID;
    GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(0), 256, 2, 0x1012,
                          &m_anBladePools[0]);
    GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(0), 1600, 2, 0x1010,
                          &m_anBladePools[1]);
}

const short MdlNodeLightsaber::NUM_FRONT_SEGMENTS = 20;
const short MdlNodeLightsaber::NUM_BACK_SEGMENTS = 20;
const short MdlNodeLightsaber::NUM_PIECE_VERTS = 8;
const unsigned short MdlNodeLightsaber::NUM_SEGMENT_VERTS = 4;

#if !KOTOR1  // K1 inlines this ctor into InternalConJoin
__attribute__((noinline))
#endif
MdlNodeEmitter::MdlNodeEmitter(char* pszName) throw() : MdlNode(pszName)
{
    m_nFlags = MDLNODE_EMITTER_ID;
    m_nEmitterFlags = 0;
    m_szRender[0] = 0;
    m_szBlend[0] = 0;
    m_szTexture[0] = 0;
    m_szUpdate[0] = 0;
    __builtin_memset(&m_fDeadSpace, 0,
                     sizeof(m_fDeadSpace) + sizeof(m_fBlastRadius) + sizeof(m_fBlastLength)
                     + sizeof(m_nNumBranches) + sizeof(m_bControlPtSmoothing) + sizeof(m_nXGrid)
                     + sizeof(m_nYGrid) + sizeof(m_nSpawnType));
    m_anChunkName[0] = 0;
    m_anChunkName[1] = 0;
    m_anChunkName[2] = 0;
    m_anChunkName[3] = 0;
    m_nTwoSidedTex = 0;
    m_nLoop = 0;
    m_nRenderOrder = 0;
    m_bFrameBlending = 0;
    __builtin_memset(m_szDepthTextureName, 0, sizeof(m_szDepthTextureName));
}

MdlNodeEmitter* MdlNodeEmitter::InternalConJoin()
{
    return new MdlNodeEmitter(nullptr);
}

#if KOTOR1  // K1's ConJoin twin factory
MdlNodeEmitter* MdlNodeEmitterConJoinTwinMysLexia(void* pv)
{
    return new MdlNodeEmitter(static_cast<char*>(pv));
}
#endif

Part* MdlNodeCameraCreateInstance(void* pv, Gob* pGob)
{
    K2_UNUSED(pGob);
    PartCamera* pPart = new PartCamera;
    pPart->m_pNode = static_cast<MdlNode*>(pv);
    countpart++;
    return pPart;
}

PartCamera* MdlNodeCamera::InternalCreateInstance(Gob* pGob)
{
    K2_UNUSED(pGob);
    PartCamera* pPart = new PartCamera;
    pPart->m_pNode = this;
    countpart++;
    return pPart;
}

PartReference* MdlNodeReference::InternalCreateInstance(Gob* pGob)
{
    K2_UNUSED(pGob);
    PartReference* pPart = new PartReference;
    pPart->m_pNode = this;
    countpart++;
    return pPart;
}

PartTriMesh* MdlNodeTriMesh::InternalCreateInstance(Gob* pGob)
{
    K2_UNUSED(pGob);
    return new PartTriMesh(this);
}

PartAnimMesh* MdlNodeAnimMesh::InternalCreateInstance(Gob* pGob)
{
    K2_UNUSED(pGob);
    return new PartAnimMesh(this);
}

PartDanglyMesh* MdlNodeDanglyMesh::InternalCreateInstance(Gob* pGob)
{
    K2_UNUSED(pGob);
    return new PartDanglyMesh(this);
}

PartAABB* MdlNodeAABB::InternalCreateInstance(Gob* pGob)
{
    K2_UNUSED(pGob);
    return new PartAABB(this);
}

PartLightsaber* MdlNodeLightsaber::InternalCreateInstance(Gob* pGob)
{
    K2_UNUSED(pGob);
    return new PartLightsaber(this);
}

bool MdlNodeTriMesh::HasTangentBases()
{
    return m_nTangentOfs != -1;
}

// Each parser-table trampoline re-types the receiver and must stay a tail call, so
// the callees stay out of line.

void MdlNodeCameraParseField(void* pv, char* pszLine)
{
    static_cast<MdlNodeCamera*>(pv)->InternalParseField(pszLine);
}

void MdlNodeCameraControl(void* pv, Part* pPart, float fA, float fB, float fC)
{
    static_cast<MdlNodeCamera*>(pv)->InternalControl(pPart, fA, fB, fC);
}


void MdlNodeLightPostProcess(void* pv)
{
    static_cast<MdlNodeLight*>(pv)->InternalPostProcess();
}

void MdlNodeLightControl(void* pv, Part* pPart, float fA, float fB, float fC)
{
    static_cast<MdlNodeLight*>(pv)->InternalControl(pPart, fA, fB, fC);
}

void MdlNodeReferenceControl(void* pv, Part* pPart, float fA, float fB, float fC)
{
    static_cast<MdlNodeReference*>(pv)->InternalControl(pPart, fA, fB, fC);
}

void MdlNodeTriMeshPostProcess(void* pv)
{
    static_cast<MdlNodeTriMesh*>(pv)->InternalPostProcess();
}

void MdlNodeTriMeshControl(void* pv, Part* pPart, float fA, float fB, float fC)
{
    static_cast<MdlNodeTriMesh*>(pv)->InternalControl(pPart, fA, fB, fC);
}


void MdlNodeDanglyMeshDestructor(void* pv)
{
    static_cast<MdlNodeDanglyMesh*>(pv)->InternalDestructor();
}

Part* MdlNodeDanglyMeshCreateInstance(void* pv, Gob* pGob)
{
    K2_UNUSED(pGob);
    return new PartDanglyMesh(static_cast<MdlNode*>(pv));
}

void MdlNodeTriMeshDestructor(void* pv)
{
    static_cast<MdlNodeTriMesh*>(pv)->InternalDestructor();
}

Part* MdlNodeTriMeshCreateInstance(void* pv, Gob* pGob)
{
    K2_UNUSED(pGob);
    return new PartTriMesh(static_cast<MdlNode*>(pv));
}

void MdlNodeAABBDestructor(void* pv)
{
    static_cast<MdlNodeAABB*>(pv)->InternalDestructor();
}

Part* MdlNodeAABBCreateInstance(void* pv, Gob* pGob)
{
    K2_UNUSED(pGob);
    return new PartAABB(static_cast<MdlNode*>(pv));
}

void MdlNodeLightsaberDestructor(void* pv)
{
    static_cast<MdlNodeLightsaber*>(pv)->InternalDestructor();
}

Part* MdlNodeLightsaberCreateInstance(void* pv, Gob* pGob)
{
    K2_UNUSED(pGob);
    return new PartLightsaber(static_cast<MdlNode*>(pv));
}

Part* MdlNodeReferenceCreateInstance(void* pv, Gob* pGob)
{
    K2_UNUSED(pGob);
    PartReference* pPart = new PartReference;
    pPart->m_pNode = static_cast<MdlNode*>(pv);
    countpart++;
    return pPart;
}


extern int alwaysdominspheres;
extern int quaternionidentityoptimization;

extern float position_scale_factor;

extern char* hitpartname;

extern int countpart;

extern List<Gob*> gobs;

// Hit-tests a segment against a polygon. Builds the polygon's plane, clips the segment against it,
// then walks the edges to confirm the intersection lies inside. Either output may be null.
unsigned int polyhit(const Vector* verts, int vertCount, const Vector& segStart,
                     const Vector& segEnd, Vector* outPoint, Vector* outNormal);

// Segment-versus-box test; nonzero when [a, b] intersects the box.
unsigned int BoxIntersect(const Vector& a, const Vector& b,
                          const Vector& boxMin, const Vector& boxMax);

int aabb_testopposite;

Vector VectorMin(Vector a, Vector b);
Vector VectorMax(Vector a, Vector b);

// Quaternion-to-vector rotation, TU-local like the other copies of it.
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

// The inverse rotation (transpose), for bringing a world-frame point back into local space.
static inline Vector RotateByTranspose(const Vector& v, const Quaternion& q)
{
#if KOTOR1  // K1 rotates through the out-of-line inverse operator
    return RotateVector(v, -q);
#else
    float w = q.w;
    if (quaternionidentityoptimization == 0 || (w != 1.0f && w != -1.0f)) {
        float nw = -w;
        float x = q.x, y = q.y, z = q.z;
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, xz = x * z, wx = nw * x;
        float yz = y * z, wy = nw * y;
        float wz = nw * z;
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
#endif
}

// A second transpose helper, since this class's copy orders its cross-products differently from
// the one above.
static inline Vector RotateByTransposeModel(const Vector& v, const Quaternion& q)
{
#if KOTOR1  // K1 rotates through the out-of-line inverse operator
    return RotateVector(v, -q);
#else
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
#endif
}

Part::Part(MdlNode* p)
{
    m_pNode = p;
    countpart++;
}

#if KOTOR1  // K1 keeps the sphere cache in a map
extern std::unordered_map<const Part*, Part::GetMinimumSphere_cache_data*> GetMinimumSphere_cache_map;
Part::~Part()
{
    auto it = GetMinimumSphere_cache_map.find(this);
    if (it != GetMinimumSphere_cache_map.end()) {
        delete it->second;
        GetMinimumSphere_cache_map.erase(it);
    }
    for (int64_t i = m_nChildren - 1; i != -1; i--) {
        if (m_paChildren[i])
            delete m_paChildren[i];
        m_nChildren = static_cast<int>(i);
    }
    if (m_pMaterial)
        delete m_pMaterial;
    countpart--;
    delete[] m_paChildren;
}
#endif
Vector Part::SetPartPosition(Vector)
{
    return Vector(0.0f, 0.0f, 0.0f);
}

#if KOTOR1  // K1 keeps the sphere cache in a map
std::unordered_map<const Part*, Part::GetMinimumSphere_cache_data*> GetMinimumSphere_cache_map;
Part::GetMinimumSphere_cache_data* Part::GetMinimumSphere_cache_GetData()
{
    auto it = GetMinimumSphere_cache_map.find(this);
    if (it == GetMinimumSphere_cache_map.end())
        it = GetMinimumSphere_cache_map.emplace(this, new GetMinimumSphere_cache_data()).first;
    return it->second;
}

float Part::GetMinimumSphere_localradius()
{
    PartEmitter* pEmitter = AsPartEmitter();
    if (pEmitter && (m_pGob->m_pModel->m_nFlags & 1))
        return pEmitter->m_fRadius;
    return 0.0f;
}

bool Part::GetMinimumSphere_cache(GetMinimumSphere_cache_data* pData)
{
    if (pData == 0)
        pData = GetMinimumSphere_cache_GetData();
    if (!pData->bValid)
        return false;
    if (pData->bDrawEnabled != m_bDrawEnabled)
        return false;
    if (pData->nAlwaysDomin != alwaysdominspheres)
        return false;
    if (pData->fLocalRadius != GetMinimumSphere_localradius())
        return false;
    if (m_bDrawEnabled || alwaysdominspheres) {
        bool bOk = true;
        for (int64_t i = 0; bOk && i < m_nChildren; i++)
            bOk = m_paChildren[i]->GetMinimumSphere_cache(pData->lChildren.m_pData[i]);
        return bOk;
    }
    return true;
}

#endif
void Part::GetMinimumSphere(Vector& outCentre, float& outRadius)
{
#if KOTOR1  // K1 memoises through the cache, recomputed only when stale
    GetMinimumSphere_cache_data* pData = GetMinimumSphere_cache_GetData();
    if (GetMinimumSphere_cache(pData)) {
        outCentre = pData->vCentre;
        outRadius = pData->fRadius;
        return;
    }
    pData->lChildren.m_count = 0;

    Vector centre;
    float bestRadius = GetMinimumSphere_localradius();
    pData->fLocalRadius = bestRadius;

    if (m_bDrawEnabled || alwaysdominspheres) {
        for (int i = 0; i < m_nChildren; i++) {
            Vector v;
            float R = 0.0f;
            m_paChildren[i]->GetMinimumSphere(v, R);
            pData->lChildren.Add(m_paChildren[i]->GetMinimumSphere_cache_GetData());
            v = RotateVector(v, m_paChildren[i]->m_qLocalOrientation) + m_paChildren[i]->m_vLocalPosition;
            Vector dir = centre - v;
            bool bBigger = bestRadius < R;
            float hi, lo;
            if (bBigger) { hi = R; lo = bestRadius; } else { hi = bestRadius; lo = R; }
            float mag = magnitude(dir);
            if (mag == 0.0f || hi >= lo + mag) {
                centre = bBigger ? v : centre;
                bestRadius = hi;
                continue;
            }
            float t = (bestRadius / mag + (1.0f - R / mag)) * 0.5f;
            centre = v + (centre - v) * t;
            bestRadius = mag * (bestRadius / mag + 1.0f - t);
        }
    }

    outCentre = centre;
    outRadius = bestRadius;
    pData->nAlwaysDomin = alwaysdominspheres;
    pData->bValid = true;
    pData->bDrawEnabled = m_bDrawEnabled;
    pData->vCentre = centre;
    pData->fRadius = bestRadius;
#else
    Vector centre;  // {0,0,0}
    float bestRadius = 0.0f;

    PartEmitter* pEmitter = AsPartEmitter();
    if (pEmitter && (m_pGob->m_pModel->m_nFlags & 1)) {
        bestRadius = pEmitter->m_fRadius;
    }

    if (m_bDrawEnabled || alwaysdominspheres) {
        for (int i = 0; i < m_nChildren; i++) {
            Vector v;
            float R = 0.0f;
            m_paChildren[i]->GetMinimumSphere(v, R);
            v = RotateVector(v, m_paChildren[i]->m_qLocalOrientation) + m_paChildren[i]->m_vLocalPosition;
            Vector dir = centre - v;
// Ternary min/max, since the library calls would stay out of line.
            bool bBigger = bestRadius < R;
            float hi, lo;
            if (bBigger) { hi = R; lo = bestRadius; } else { hi = bestRadius; lo = R; }
            float mag = magnitude(dir);
            if (mag == 0.0f || hi >= lo + mag) {
                centre = bBigger ? v : centre;
                bestRadius = hi;
                continue;
            }
            float t = (bestRadius / mag + (1.0f - R / mag)) * 0.5f;
            centre = v + (centre - v) * t;
            bestRadius = mag * (bestRadius / mag + 1.0f - t);
        }
    }

    outCentre = centre;
    outRadius = bestRadius;
#endif
}

#if KOTOR1  // K1 inlines this downcast at its call sites
static inline Model* K1AsModelInline(MaxTree* pTree)
{
    if ((pTree->m_cType & MaxTree::kTreeTypeMask) != MaxTree::kTreeModel)
        return 0;
    return static_cast<Model*>(pTree);
}
#endif

void Part::GetMinimumBoundingBox(Vector& outMin, Vector& outMax, float scale)
{
    PartTriMesh* trimesh = AsPartTriMesh();
    MdlNode* node;
    MdlNodeTriMesh* mesh;
    if (trimesh != 0
        && ((node = trimesh->m_pNode) == 0 || (node->m_nFlags & NODE_SABER) != NODE_SABER)
        && (mesh = node->AsMdlNodeTriMesh())->m_nVertexCount) {
        Vector worldPos;
        Quaternion worldOrient(1.0f, 0.0f, 0.0f, 0.0f);
        trimesh->GetWorld(worldPos, worldOrient);
        scale *= m_scale.m_fScale;

        mesh = trimesh->m_pNode->AsMdlNodeTriMesh();
#if KOTOR1  // K1 inlines the downcast here
        Model* model = K1AsModelInline(node->m_pTree);
#else
        Model* model = node->m_pTree->AsModel();
#endif
        void* pool = GLRender::LockPool(model->m_nPool, 0, 0, 8);

        for (int frameId = 0; frameId < mesh->m_faces.m_count; frameId++) {
            for (int j = 0; j < 3; j++) {
                short sindex = reinterpret_cast<short*>(mesh->m_faces.m_pData)[frameId * 16 + 13 + j];  // raw-offset: the original's byte-offset induction over the faces' vertex indices
                float* vp = reinterpret_cast<float*>(static_cast<char*>(pool) + mesh->m_nVertexStride * sindex
                                     + mesh->m_nGeomBase);
                Vector v(vp[0] * scale, vp[1] * scale, vp[2] * scale);
                Vector r = RotateVector(v, worldOrient);
                Vector a = (r + worldPos) - trimesh->m_pGob->m_vPosition;
                a = RotateByTranspose(a, trimesh->m_pGob->m_qOrientation);
                outMin = VectorMin(outMin, a);
                outMax = VectorMax(outMax, a);
            }
        }
        GLRender::UnlockPool(model->m_nPool);
    } else {
        scale *= m_scale.m_fScale;
    }

    for (int i = 0; i < m_nChildren; i++)
        m_paChildren[i]->GetMinimumBoundingBox(outMin, outMax, scale);
}

int Part::HitCheckGeom(Vector const&, Vector&, Vector*, CHitInfo*, float)
{
    return 0;
}

void* Part::HitCheck(Vector a, Vector b, Vector* c, CHitInfo* d, float scale)
{
    static Vector vTemp;
    float s = m_scale.m_fScale * scale;
// The transformed endpoints are written back into the by-value parameters; b shares the
// block after the scale split.
    if (s == 1.0f) {
        a = RotateByTranspose(a - m_vLocalPosition, m_qLocalOrientation);
        vTemp = b - m_vLocalPosition;
    } else {
        Vector p = m_vLocalPosition * s;
        a = RotateByTranspose(a - p, m_qLocalOrientation);
        vTemp = b - p;
    }
    b = RotateByTranspose(vTemp, m_qLocalOrientation);

    int r = HitCheckGeom(a, b, &b, d, s);
    void* hit;
    if (r == 0) {
        hit = 0;
    } else if (r == -1 && d->m_bIgnoreStartInside != 0) {
        hit = 0;
    } else {
        hitpartname = Name();
        d->m_pszHitPartName = Name();
        hit = this;
    }
    for (int i = 0; i < m_nChildren; i++) {
        void* cr = m_paChildren[i]->HitCheck(a, b, &b, d, s);
        if (cr) hit = cr;
    }
    if (hit) {
        vTemp = RotateVector(b, m_qLocalOrientation)
                + ((s == 1.0f) ? m_vLocalPosition : (m_vLocalPosition * s));
        if (d) {
            d->m_vHitNormal = RotateVector(d->m_vHitNormal, m_qLocalOrientation);
            d->m_hitPosition = vTemp;
        }
        if (c) *c = vTemp;
    }
    return hit;
}

const char* lasthitpart(char*)
{
    return hitpartname ? hitpartname : NULL_NAME;
}

const char* setcolorshifting(char* pszParams)
{
    char szName[64];
    float fRed = 0.0f, fGreen = 0.0f, fBlue = 0.0f, fAlpha = 1.0f;
    int nPropagate = 1;
    szName[0] = 0;

    int nFields = sscanf(pszParams, "%s%f%f%f%f%d",
                         szName, &fRed, &fGreen, &fBlue, &fAlpha, &nPropagate);
    if (static_cast<unsigned>(nFields - 4) > 2)
        return "Usage:  setcolorshifting <gobname> <red> <green> <blue> <optional alpha> <optional propagate>";

    int nLow = 0;
    int nHigh = gobs.m_count - 1;
    while (nLow <= nHigh)
    {
        int nMid = (nHigh + nLow) >> 1;
        int nCmp = _stricmp(gobs.m_pData[nMid]->m_szName, szName);
        if (nCmp > 0)
            nHigh = nMid - 1;
        else if (nCmp < 0)
            nLow = nMid + 1;
        else
        {
            nLow = nMid;
            nHigh = nMid - 1;
        }
    }
    if (nLow < gobs.m_count)
    {
        if (_stricmp(gobs.m_pData[nLow]->m_szName, szName) == 0)
        {
            Gob* pGob = gobs.m_pData[nLow];
            if (pGob != 0)
            {
                pGob->SetColorShifting(Vector(fRed, fGreen, fBlue), fAlpha, nPropagate);
                return "Success";
            }
        }
    }
    return "Error:  No gob by that name";
}

const char* setillumination(char* pszParams)
{
    char szName[64];
    float fRed, fGreen, fBlue;
    fRed = fGreen = fBlue = 0.0f;
    szName[0] = 0;

    int nFields = sscanf(pszParams, "%s%f%f%f", szName, &fRed, &fGreen, &fBlue);
    if ((nFields | 1) != 5)
        return "Usage:  setillumination <gobname> <red> <green> <blue> <optional alpha>";

    int nHigh = gobs.m_count - 1;
    int nLow = 0;
    while (nLow <= nHigh)
    {
        int nMid = (nHigh + nLow) >> 1;
        int nCmp = _stricmp(gobs.m_pData[nMid]->m_szName, szName);
        if (nCmp > 0)
            nHigh = nMid - 1;
        else if (nCmp < 0)
            nLow = nMid + 1;
        else
        {
            nLow = nMid;
            nHigh = nMid - 1;
        }
    }
    if (nLow < gobs.m_count)
    {
        if (_stricmp(gobs.m_pData[nLow]->m_szName, szName) == 0)
        {
            Gob* pGob = gobs.m_pData[nLow];
            if (pGob != 0)
            {
                pGob->SetIllumination(Vector(fRed, fGreen, fBlue), 1);
                return "Success";
            }
        }
    }
    return "Error:  No gob by that name";
}

unsigned int AnimMeshHitCheck(PartAnimMesh* part, MdlNodeAnimMesh* node, Vector a, Vector b,
                              Vector* out, float scale)
{
    unsigned int result = 0;
    if (part == 0 || node == 0)
        return result;

    int poolId = part->m_pVertexPrimitive->AsVertexPrimitiveAnim()->m_nVertexPool;
    Vector* pool = static_cast<Vector*>(GLRender::LockPool(poolId, 0, 0, 8));

    if (scale == 1.0f) {
        for (int64_t i = 0; i < node->m_faces.m_count; i++) {
            MdlNodeAnimMesh::Face* face = &node->Faces()[i];
            Vector verts[3];
            verts[0] = pool[static_cast<int64_t>(face->m_Index0)];
            verts[1] = pool[static_cast<int64_t>(face->m_Index1)];
            verts[2] = pool[static_cast<int64_t>(face->m_Index2)];
            result |= polyhit(verts, 3, a, b, &b, 0);
        }
    } else {
        for (int64_t i = 0; i < node->m_faces.m_count; i++) {
            MdlNodeAnimMesh::Face* face = &node->Faces()[i];
            Vector verts[3];
            verts[0] = pool[static_cast<int64_t>(face->m_Index0)] * scale;
            verts[1] = pool[static_cast<int64_t>(face->m_Index1)] * scale;
            verts[2] = pool[static_cast<int64_t>(face->m_Index2)] * scale;
            result |= polyhit(verts, 3, a, b, &b, 0);
        }
    }

    poolId = part->m_pVertexPrimitive->AsVertexPrimitiveAnim()->m_nVertexPool;
    GLRender::UnlockPool(poolId);
    if (out != 0 && result)
        *out = b;
    return result;
}

void PartInitMaterial(Gob* gob, Part* part, char* name)
{
    PartTriMesh* trimesh = part->AsPartTriMesh();
    if (trimesh) {
        MdlNodeTriMesh* node = trimesh->m_pNode->AsMdlNodeTriMesh();
        if (node->m_nAnimateUV != 0 && node->m_nTex0Ofs != -1 && gob != 0)
            gob->m_bAnimateUV = true;

        if (trimesh->m_pMaterial != 0)
            trimesh->m_pMaterial->LoadTextures(part);

        if (trimesh->AsPartSkin()) {
            trimesh->m_pVertexPrimitive = new VertexPrimitiveSkinned(trimesh->AsPartSkin());
        } else if (trimesh->AsPartAnimMesh()) {
            trimesh->m_pVertexPrimitive = new VertexPrimitiveAnim(trimesh);
        } else if (trimesh->AsPartDanglyMesh()) {
            trimesh->m_pVertexPrimitive = new VertexPrimitiveDangly(trimesh);
        } else {
            trimesh->m_pVertexPrimitive = new VertexPrimitiveFlat(trimesh);
        }
    }

    for (int64_t i = 0; i < part->m_nChildren; i++)
        PartInitMaterial(gob, part->m_paChildren[i], name);
}

static inline char* ListStringAt(List<char*>* pList, int nIndex)
{
    return pList->m_pData[nIndex];
}

Part* FindPart(Part* pPart, char* pszName)
{
    if (!pPart || !pszName)
        return 0;

    MaxTree* pTree = pPart->m_pNode->m_pTree;
    int nTag = pTree->m_cType & MaxTree::kTreeTypeMask;
    K2_ASSUME(pTree != 0 && (nTag == MaxTree::kTreeModel || nTag == MaxTree::kTreeAnimation));   // a node's tree is the model or animation that loaded it
    Model* pModel = (pTree != 0 && nTag == MaxTree::kTreeModel) ? static_cast<Model*>(pTree)
                                             : (nTag == MaxTree::kTreeAnimation ? static_cast<Animation*>(pTree) : 0)->m_pModel;

    if (!_stricmp(ListStringAt(&pModel->m_listStrings, pPart->m_pNode->m_nNameIndex), pszName))
        return pPart;

    for (int64_t i = 0; i < pPart->m_nChildren; i++) {
        Part* pFound = FindPart(pPart->m_paChildren[i], pszName);
        if (pFound)
            return pFound;
    }
    return 0;
}

void Part::PostProcess(Part* part)
{
    for (int64_t i = 0; i < m_nChildren; i++)
        m_paChildren[i]->PostProcess(part);
}

Part* CreateInstanceTreeR(MdlNode* pNode, Gob* pGob)
{
    if (!pNode)
        return 0;

    Part* pPart = pNode->CreateInstanceDispatch(pGob);
    pPart->m_pGob = pGob;
    position_scale_factor = 1.0f;
    pNode->ControlDispatch(pPart, -1.0f, 0.0f, 1.0f);

    List<Part*>* pChildren = reinterpret_cast<List<Part*>*>(&pPart->m_paChildren);  // raw-offset: Part's child array is a List<Part*> the destructor frees by hand
    pChildren->SetSize(pNode->m_listChildren.m_count);

    for (int64_t i = 0; i < pNode->m_listChildren.m_count; i++) {
        pChildren->m_pData[i] = CreateInstanceTreeR(pNode->m_listChildren.m_pData[i], pGob);
        pChildren->m_pData[i]->m_pParent = pPart;
    }
#if KOTOR1  // K1: a fresh subtree starts with a stale sphere memo
    pPart->GetMinimumSphere_cache_GetData()->bValid = false;
#endif

    pPart->m_nNodeID = pNode->m_nNodeID;
    return pPart;
}

Part* CreateInstanceTree(MdlNode* pNode, Gob* pGob)
{
    Part* pPart = CreateInstanceTreeR(pNode, pGob);
    pPart->PostProcess(pPart);
    return pPart;
}

char* Part::Name()
{
#if KOTOR1  // K1 walks the same lookup in one expression
    return m_pNode->m_pTree->AsModel()->m_listStrings.m_pData[m_pNode->m_nNameIndex];
#else
    MaxTree* tree = m_pNode->m_pTree;
    int idx = m_pNode->m_nNameIndex;
    return tree->AsModel()->m_listStrings.m_pData[idx];
#endif
}

List<Part*>* Part::GetChildren()
{
    return reinterpret_cast<List<Part*>*>(&m_paChildren);  // raw-offset: Part's child array is a List<Part*> the destructor frees by hand
}

Part* Part::GetParent()
{
    return m_pParent;
}

void* Part::GetAurObject()
{
    return m_pGob;
}

void Part::GetWorld(Vector& outPos, Quaternion& outOrient)
{
    outPos = Vector(0.0f, 0.0f, 0.0f);
    outOrient = Quaternion();

    Gob* pGob = m_pGob;
    if (pGob == 0) {
        outPos = m_vLocalPosition;
        outOrient = m_qLocalOrientation;
        return;
    }

    Part* p = this;
    do {
        outPos = outPos * p->m_scale.m_fScale;
        outPos = RotateVector(outPos, p->m_qLocalOrientation);
        outPos = outPos + p->m_vLocalPosition;
        outOrient = p->m_qLocalOrientation * outOrient;
        p = p->m_pParent;
    } while (p != 0);

    if (pGob->m_fObjectScale != 1.0f)
        outPos = outPos * pGob->m_fObjectScale;
    outPos = RotateVector(outPos, pGob->m_qOrientation);
    outPos = outPos + pGob->m_vPosition;
    outOrient = pGob->m_qOrientation * outOrient;
}

void Part::GetLocal(Vector& outPos, Quaternion& outOrient)
{
    float fScale = 1.0f;

    if (m_pParent != 0) {
        m_pParent->GetLocal(outPos, outOrient);
        fScale = m_pParent->m_scale.m_fScale;
    } else if (m_pGob != 0) {
#if KOTOR1  // K1 rotates through the out-of-line Quaternion inverse operator
        outOrient = -m_pGob->m_qOrientation * outOrient;
#else
        outOrient = Quaternion(-m_pGob->m_qOrientation.w, m_pGob->m_qOrientation.x,
                               m_pGob->m_qOrientation.y, m_pGob->m_qOrientation.z)
                    * outOrient;
#endif
        outPos = RotateByTransposeModel(outPos - m_pGob->m_vPosition, m_pGob->m_qOrientation);
        fScale = m_pGob->m_fObjectScale;
    }

#if KOTOR1  // K1 rotates through the out-of-line Quaternion inverse operator
    outOrient = -m_qLocalOrientation * outOrient;
#else
    outOrient = Quaternion(-m_qLocalOrientation.w, m_qLocalOrientation.x, m_qLocalOrientation.y,
                           m_qLocalOrientation.z)
                * outOrient;
#endif
    outPos = RotateByTransposeModel(outPos - m_vLocalPosition, m_qLocalOrientation);
    outPos = outPos * (1.0f / fScale);
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int Part::printMemory()
{
    return printf("Part: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
                  8, 16, 28, 44, 48, 52, 53, 54, 56, 72, 80, 88, 96);
}
#endif // !KOTOR1


Gob* FindGob(char* pszName);

MdlNodeLight* MdlNode::AsMdlNodeLight()
{
    if ((m_nFlags & NODE_LIGHT) != NODE_LIGHT)
        return 0;
    return static_cast<MdlNodeLight*>(this);
}


void MaxTreeDestructor(void* pTree)
{
    MaxTree* p = static_cast<MaxTree*>(pTree);
    if (p->m_pMdlNode != 0)
        p->m_pMdlNode->DestructorDispatch();
}

void MaxTree::InternalDestructor()
{
    if (m_pMdlNode != 0)
        m_pMdlNode->DestructorDispatch();
}

void MaxTreeParseField(void* pTree, char* pszLine)
{
    static_cast<MaxTree*>(pTree)->InternalParseField(pszLine);
}

// An anchor for MaxTree's constructor and destructor. Both are defined in the class, so an
// unreferenced use here guarantees this unit defines them.
#if !defined(K2_HOST_BUILD)
__attribute__((used)) static void MaxTreeOutOfLineAnchor(char* pszName)
{
    MaxTree tree(pszName);
}
#endif


void MaxTree::AddToFreeList(MdlNode* pNode)
{
    m_freeList.Add(pNode);
}

void MaxTree::AddToFreeTokenList(unsigned int nToken)
{
    m_freeTokens.Add(nToken);
}

void MaxTree::ResetFreeLists()
{
    m_freeList.SetSize(0);
    m_freeTokens.SetSize(0);
}



extern int removedelref;
extern int animationfootprint;


void MaxTree::FreeTheFreeList()
{
    for (int64_t i = 0; i < m_freeList.m_count; i++)
        m_freeList.m_pData[i]->AsMdlNodeLight()->BinaryInternalDestructor();
    m_freeList.SetSize(0);
    for (int64_t i = 0; i < m_freeTokens.m_count; i++) {
        GLRender::DeletePool(m_freeTokens.m_pData[i]);
        m_freeTokens.m_pData[i] = 0xffffffff;
    }
    m_freeTokens.SetSize(0);
}

int Model::DelRef(Model* pModel)
{
    K2_UNUSED(pModel);
    int nCount = m_nRefCount;
    if (removedelref == 0) {
        nCount = --m_nRefCount;
        if (nCount == 0) {
            foobar(this);
            nCount = 0;
        }
    }
    return nCount;
}

#if KOTOR1  // interposable: no inlining, no readonly inference
__attribute__((weak))  // interposable: no inlining, no readonly inference
#endif
Model* MaxTree::AsModel()
{
    if ((m_cType & kTreeTypeMask) != kTreeModel)
        return 0;
    return static_cast<Model*>(this);
}

int GetMaxID(MdlNode* pNode, int nID);
void SynchronizeNodes(MdlNode* pDst, MdlNode* pSrc, int& rnMaxID);
void UpdateAnimFootprint(MdlNode* pNode, int nDelta);

int MaxTree::GetType()
{
    return static_cast<unsigned char>(m_cType) & kTreeTypeMask;
}


void ModelDestructor(void* pModel)
{
    static_cast<Model*>(pModel)->InternalDestructor();
}

void ModelParseField(void* pModel, char* pszLine)
{
    static_cast<Model*>(pModel)->InternalParseField(pszLine);
}

void InsertModel(Model* pModel);
void UpdateAnimFootprint(MdlNode* pNode, int nDelta);

List<Model*> Models;
int animationfootprint;
int removedelref;




void Model::InternalDestructor()
{
    if (Models.Contains(this))
        Models.Remove(this);
    if (m_pSuperModel != 0) {
        if (removedelref == 0) {
            if (--m_pSuperModel->m_nRefCount == 0)
                foobar(m_pSuperModel);
        }
        m_pSuperModel = 0;
    }
    animationfootprint -= m_animations.m_count * 8;
    animationfootprint -= m_animations.m_count * 176;
    if (m_animations.m_count != 0) {
        int64_t i = m_animations.m_count - 1;
        do {
            animationfootprint -= 36 * m_animations.m_pData[i]->m_events.m_count;
            UpdateAnimFootprint(m_animations.m_pData[i]->m_pMdlNode, -1);
            Animation* pAnim = m_animations.m_pData[i];
            pAnim->m_pfnDestructor(pAnim);
        } while (i-- != 0);
    }
    if (m_pMdlNode != 0)
        m_pMdlNode->DestructorDispatch();
}

Model::Model(char* pszName) : MaxTree(pszName)
{
    m_cType = kTreeModel;  // overrides the base ctor's kTreeBase
    m_pSuperModel = 0;
    m_nFlags = 0;
    m_bReserved1 = 0;
    m_cAffectedByFog = 1;
    m_fRadius = 7.0f;
    m_fAnimScale = 1.0f;
    m_nRefCount = 0;
    m_vBoundsMin.x = -5.0f;
    m_vBoundsMin.y = -5.0f;
    m_vBoundsMin.z = -1.0f;
    m_vBoundsMax.x = 5.0f;
    m_vBoundsMax.y = 5.0f;
    m_vBoundsMax.z = 10.0f;
    InsertModel(this);
    m_szSuperModelName[0] = 0;
    m_pfnDestructor = ModelDestructor;
    m_pfnParseField = ModelParseField;
    m_nPool = 0xffffffff;
    m_nPool = GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(0), 1, 2, 0, &m_nPool);
    m_nPoolSize = 0;
    m_pMdxResource = 0;
    m_listStrings.m_count = 0;
}

Model::~Model()
{
    if (m_cType) {
        m_cType = 0;
        if (m_animations.m_count != 0) {
            int64_t i = m_animations.m_count - 1;
            do {
                foobar(m_animations.m_pData[i]);
                m_animations.m_count = static_cast<int>(i);
                i--;
            } while (i != -1);
        }
        GLRender::DeletePool(m_nPool);
        m_nPool = 0xffffffff;
        for (int64_t i = 0; i < m_listStrings.m_count; i++)
            delete[] m_listStrings.m_pData[i];
    }
}


int Model::DelRef(Gob* pGob)
{
    K2_UNUSED(pGob);
    int nCount = m_nRefCount;
    if (removedelref == 0) {
        nCount = --m_nRefCount;
        if (nCount == 0) {
            foobar(this);
            nCount = 0;
        }
    }
    return nCount;
}

int Model::AddRef(Gob* pGob)
{
    K2_UNUSED(pGob);
    return ++m_nRefCount;
}

int Model::AddRef(Model* pModel)
{
    K2_UNUSED(pModel);
    return ++m_nRefCount;
}

extern Model* currentmodel;


int FormatName(char* pszBuf, uint64_t nSize, const char* pszFmt, ...);

Model* BinaryFindModel(char* pszName);

extern int counttrimesh;

Vector g_HitCheckBoxExpansion;

unsigned int BoxIntersect(const Vector& a, const Vector& b, const Vector& boxMin,
                          const Vector& boxMax);
unsigned int BoxIntersect(const Vector& a, const Vector& b, const Vector& boxMin,
                          const Vector& boxMax, Vector* out);
// math3d.cpp defines this overload as `bool`, but the callers here were compiled against
// `unsigned int` and read the whole result word, so it is kept wide on the target build.
#ifdef K2_HOST_BUILD
typedef bool PolyhitResult;
#else
typedef unsigned int PolyhitResult;
#endif
PolyhitResult polyhit(const Vector* verts, int vertCount, const Vector& node,
                      const float& f, const Vector& a, const Vector& b,
                      Vector* hit);

struct TriFace {
    Vector vNode;
    float  fScale;
    int    m_nMaterial;
    char   _pad14[0x1a - 0x14];
    short  m_nVIndex[3];
};

struct TriMeshPoolPair {
    unsigned int a;
    unsigned int b;
};

PartTriMesh::PartTriMesh(MdlNode* node)
{
    m_pNode = node;
    countpart++;

    m_pMaterial = new Material();

    m_nPartID = m_nNextPartID++;

    if (node->m_pTree != 0 && (node->m_pTree->m_cType & MaxTree::kTreeTypeMask) == MaxTree::kTreeModel)
        m_bFog = static_cast<Model*>(node->m_pTree)->m_cAffectedByFog;

    counttrimesh++;

    MdlNodeTriMesh* pMesh = node->AsMdlNodeTriMesh();
    if (pMesh->m_nAnimateUV != 0)
    {
        TriMeshPoolPair* pDst = new TriMeshPoolPair[static_cast<short>(pMesh->m_nVertexCount)];
        m_pafBaseUVs = reinterpret_cast<float*>(pDst);

#if KOTOR1  // K1 inlines the downcast here
        Model* pModel = K1AsModelInline(pMesh->m_pTree);
#else
        Model* pModel = pMesh->m_pTree->AsModel();
#endif
        unsigned int nPool = pModel->m_nPool;
        char* pPool = static_cast<char*>(GLRender::LockPool(nPool, 0, 0, 8));

        const int* pSrc = reinterpret_cast<const int*>(pPool + pMesh->m_nTex0Ofs + pMesh->m_nGeomBase);
        for (int64_t i = 0; i < static_cast<short>(pMesh->m_nVertexCount); i++)
        {
            pDst[i].a = static_cast<unsigned int>(pSrc[0]);
            pDst[i].b = static_cast<unsigned int>(pSrc[1]);
            pSrc = reinterpret_cast<const int*>(reinterpret_cast<const char*>(pSrc) + pMesh->m_nVertexStride);
        }

        GLRender::UnlockPool(nPool);
    }
    else
    {
        m_pafBaseUVs = 0;
    }
}

PartTriMesh::~PartTriMesh()
{
    if (m_pVertexPrimitive)
        delete m_pVertexPrimitive;

    if (m_bHasColorPool) {
        m_bHasColorPool = false;
        GLRender::DeletePool(m_nColorPool);
        m_nColorPool = -1;
    }
    if (m_nUVPool != -1) {
        GLRender::DeletePool(m_nUVPool);
        m_nUVPool = -1;
    }
    counttrimesh--;

    if (m_pafBaseUVs) {
        delete[] m_pafBaseUVs;
        m_pafBaseUVs = 0;
    }

}

void PartTriMesh::GetMinimumSphere(Vector& outCentre, float& outRadius)
{
    Part::GetMinimumSphere(outCentre, outRadius);

    MdlNodeTriMesh* mesh = m_pNode->AsMdlNodeTriMesh();
    float fRadius = mesh->m_fRadius;
    Vector vMesh = mesh->m_vCentroid;

    float fLarge = outRadius;
    float fSmall = fRadius;
    Vector vLarge = outCentre;
    if (!(fRadius < outRadius)) {
        fLarge = fRadius;
        fSmall = outRadius;
        vLarge = vMesh;
        outCentre = vLarge;
    }

    Vector dir = vMesh - vLarge;
    float mag = magnitude(dir);
    if (mag == 0.0f || fLarge >= fSmall + mag) {
        outRadius = fLarge;
        return;
    }

    float f = (fRadius + (mag - outRadius)) * 0.5f;
    outRadius = mag + outRadius - f;
    outCentre = outCentre + (vMesh - outCentre) * (f / mag);
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int PartTriMesh::printMemory()
{
    printf("Part: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
           8, 16, 28, 44, 48, 52, 53, 54, 56, 72, 80, 88, 96);
    return printf(" -> PartTriMesh: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
                  100, 112, 124, 136, 144, 148, 152, 153, 160, 168, 172, 176, 184, 192);
}
#endif // !KOTOR1

int PartTriMesh::HitCheckGeom(Vector const& a, Vector& b, Vector* c, CHitInfo* h, float f)
{
    if (AsPartSkin() != 0)
        return 0;

    MdlNodeTriMesh* node = static_cast<MdlNodeTriMesh*>(m_pVertexPrimitive->m_pNode);
    if (node != 0 && (node->m_nFlags & NODE_SABER) == NODE_SABER)
        return 0;
#ifdef K2_FIX_ENGINE_BUGS
    if (node == 0)
        return 0;
#endif
    if (node->m_nVertexCount == 0)
        return 0;

    if (h->m_bHitBoundingBoxes != 0) {
// On the box-expansion path, the probe runs against an expanded copy of the node's box.
        static Vector sb;
        sb = b;
        int r;
        if (f == 1.0f)
            r = static_cast<int>(BoxIntersect(a, b, node->m_vMin - g_HitCheckBoxExpansion,
                                  node->m_vMax + g_HitCheckBoxExpansion, c));
        else
            r = static_cast<int>(BoxIntersect(a, b, node->m_vMin * f - g_HitCheckBoxExpansion,
                                  node->m_vMax * f + g_HitCheckBoxExpansion, c));
        if (r == -1) {
            if (h->m_bIgnoreStartInside != 0)
                b = sb;
            else
                h->ClearOutput();
        } else if (r == 0) {
            return 0;
        } else {
            h->ClearOutput();
            float sign = ((r - 1) & 1) ? 1.0f : -1.0f;
            (&h->m_vHitNormal.x)[(r - 1) / 2] = sign;
        }
        h->m_hitPosition = *c;
        return r;
    }

    int result = 0;
    if (f == 1.0f) {
        if (!BoxIntersect(a, b, node->m_vMin, node->m_vMax) || node->m_faces.m_count < 1)
            return 0;
        float* pVertices = node->m_paVertices;
        for (int64_t i = 0; i < node->m_faces.m_count; i++) {
            TriFace& face = reinterpret_cast<TriFace*>(node->m_faces.m_pData)[i];
            Vector verts[3];
            for (int k = 0; k < 3; k++) {
                float* pV = &pVertices[face.m_nVIndex[k] * 3];
                verts[k].x = pV[0];
                verts[k].y = pV[1];
                verts[k].z = pV[2];
            }
            result |= static_cast<int>(polyhit(verts, 3, face.vNode, face.fScale, a, b, &b));
        }
    } else {
        if (!BoxIntersect(a, b, node->m_vMin * f, node->m_vMax * f) || node->m_faces.m_count < 1)
            return 0;
        float* pVertices = node->m_paVertices;
        for (int64_t i = 0; i < node->m_faces.m_count; i++) {
            TriFace& face = reinterpret_cast<TriFace*>(node->m_faces.m_pData)[i];
            Vector verts[3];
            for (int k = 0; k < 3; k++) {
                float* pV = &pVertices[face.m_nVIndex[k] * 3];
                verts[k].x = pV[0] * f;
                verts[k].y = pV[1] * f;
                verts[k].z = pV[2] * f;
            }
            result |= static_cast<int>(polyhit(verts, 3, face.vNode, face.fScale, a, b, &b));
        }
    }
    if (c != 0 && result != 0) {
        h->ClearOutput();
        h->m_vHitNormal = Vector(0.0f, 0.0f, 0.0f);
        h->m_hitPosition = b;
        h->m_vLocalHitPosition = b;
        *c = b;
    }
    return result;
}


// A trivial POD, because the array allocated here must not zero its elements.
struct DanglyVertex {
    float m_x, m_y, m_z;
};

extern int renderdangly;

PartDanglyMesh::PartDanglyMesh(MdlNode* node)
    : PartTriMesh(node)
{
    m_paVertices = 0;

    short n = static_cast<short>(node->AsMdlNodeTriMesh()->m_nVertexCount);
    m_paVertices = reinterpret_cast<Vector*>(new DanglyVertex[n]);
    memcpy(m_paVertices, node->AsMdlNodeDanglyMesh()->m_pVertexSource, n * sizeof(DanglyVertex));
    GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(0),
        static_cast<unsigned int>(static_cast<short>(node->AsMdlNodeTriMesh()->m_nVertexCount) * sizeof(DanglyVertex)),
        2, 0, reinterpret_cast<unsigned int*>(&m_nVertexPool));
    void* pLocked = GLRender::LockPool(m_nVertexPool, 0, 0, 1);
    memcpy(pLocked, m_paVertices, static_cast<short>(node->AsMdlNodeTriMesh()->m_nVertexCount) * sizeof(DanglyVertex));
    GLRender::UnlockPool(m_nVertexPool);
}

PartDanglyMesh::~PartDanglyMesh()
{
    if (m_paVertices)
        delete[] m_paVertices;
    GLRender::DeletePool(m_nVertexPool);
    m_nVertexPool = -1;
}


#if !KOTOR1  // K2-only (no K1 counterpart)
int PartDanglyMesh::printMemory()  // returns the second printf result
{
    printf("Part: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n", 8, 16, 28, 44,
           48, 52, 53, 54, 56, 72, 80, 88, 96);
    return printf(" -> PartDanglyMesh: %d, %d, %d, %d\n", 196, 208, 224, 232);
}
#endif // !KOTOR1

Vector PartDanglyMesh::SetPartPosition(Vector)
{
    return Vector(0.0f, 0.0f, 0.0f);
}

// PartLight's bodies. The base Part(MdlNode*) initialiser is expanded here rather than called.
extern int countpart;

extern List<Gob*> gobs;



PartLight::PartLight(MdlNode* p)
{
    m_pNode = p;
    countpart++;
}

PartLight* MdlNodeLight::InternalCreateInstance(Gob* pGob)
{
    PartLight* pPart = new PartLight(this);
    pPart->m_nCastsShadows = m_bShadow;
    pPart->m_nAffectDynamic = m_bAffectDynamic;
    pPart->m_nIsDynamic = m_bIsDynamic;
    pPart->m_nAmbientOnly = m_bAmbientOnly;
    pPart->m_nLocalPositionSet = 0;
    if (pGob != 0) {
        if (m_bIsDynamic) {
            pGob->m_nDynamicLights = 1;
        } else {
            if (m_bAffectDynamic)
                pGob->m_nDynamicLights = 1;
            pGob->m_partLights.Add(pPart);
        }
    }
    return pPart;
}

PartLight::~PartLight()
{
    for (int i = 0; i < gobs.m_count; i++) {
        CAurObjectLightContainer* pLight = gobs.m_pData[i]->m_pLightContainer;
        if (!pLight)
            continue;
        if (pLight->m_lights.Contains(this)) {
            int idx = pLight->m_lights.GetIndex(this);
            if (idx >= 0) {
                pLight->m_lights.DelIndex(idx);
                pLight->m_lightWeights.DelIndex(idx);
            }
        }
        if (pLight->m_pSelectedLight == this) {
            pLight->m_pSelectedLight = NULL;
            pLight->m_fSelectedWeight = 0;
        }
    }
}

#if !KOTOR1  // K2-only (no K1 counterpart)
int PartLight::printMemory()  // returns the second printf result
{
    printf("Part: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
           8, 16, 28, 44, 48, 52, 53, 54, 56, 72, 80, 88, 96);
    return printf(" -> PartLight: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n",
           100, 112, 116, 120, 124, 128, 132, 136, 140, 144,
           148, 152, 156, 160, 164, 168, 172);
}
#endif // !KOTOR1

PartLight* PartLight::AsPartLight()
{
    return this;
}

void* PartLight::AsPartDummy()
{
    return 0;
}


void MdlNodeAnimMeshParseField(void* pv, char* pszLine)
{
    static_cast<MdlNodeAnimMesh*>(pv)->InternalParseField(pszLine);
}

void MdlNodeAnimMeshPostProcess(void* pv)
{
    static_cast<MdlNodeAnimMesh*>(pv)->InternalPostProcess();
}

void MdlNodeAnimMeshControl(void* pv, Part* pPart, float fA, float fB, float fC)
{
    static_cast<MdlNodeAnimMesh*>(pv)->InternalControl(pPart, fA, fB, fC);
}


PartAnimMesh* MdlNodeAnimMeshCreateInstance(void* pv, Gob* pGob)
{
    K2_UNUSED(pGob);
    return new PartAnimMesh(static_cast<MdlNode*>(pv));
}

void MdlNodeAnimMeshGenVertices(void* pv)
{
    static_cast<MdlNodeAnimMesh*>(pv)->InternalGenVertices();
}

void MdlNodeAnimMeshRemoveTemporaryArrays(void* pv)
{
    static_cast<MdlNodeAnimMesh*>(pv)->InternalRemoveTemporaryArrays();
}

MdlNodeAnimMesh::~MdlNodeAnimMesh()
{
    if (m_nFlags != 0)
        DestructorDispatch();
    m_nFlags = 0;
    DestructorDispatch();
    delete[] m_paVertexFrames;
    delete[] m_paTexCoordFrames;
}


void MdlNodeSkinParseField(void* pv, char* pszLine)
{
    static_cast<MdlNodeSkin*>(pv)->InternalParseField(pszLine);
}

void MdlNodeSkinPostProcess(void* pv)
{
    static_cast<MdlNodeSkin*>(pv)->InternalPostProcess();
}

void MdlNodeSkinCreateInstance(void* pv, Gob* pGob)
{
    static_cast<MdlNodeSkin*>(pv)->InternalCreateInstance(pGob);
}

void MdlNodeSkinGenVertices(void* pv)
{
    static_cast<MdlNodeSkin*>(pv)->InternalGenVertices();
}

void MdlNodeSkinRemoveTemporaryArrays(void* pv)
{
    static_cast<MdlNodeSkin*>(pv)->InternalRemoveTemporaryArrays();
}

MdlNodeSkin::~MdlNodeSkin()
{
    if (m_nFlags != 0)
        DestructorDispatch();
    m_nFlags = 0;
    if (m_paBoneMap != 0) {
        delete[] m_paBoneMap;
        m_paBoneMap = 0;
    }
    m_lTBoneRefInv.SetSize(0);
    m_lQBoneRefInv.SetSize(0);
}

PartLightsaber::PartLightsaber(MdlNode* node)
    : PartTriMesh(node)
{
    for (int i = 0; i < 25; i++)
        m_aqSegmentOrient[i] = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);

    m_nOrientIndex = 0;
    m_nLifeIndex = 0;
    m_nSegmentCount = 0;
}

PartLightsaber::~PartLightsaber()
{
    m_nSegmentCount = 0;
    m_nOrientIndex = 0;
    m_nLifeIndex = 0;
}


void* PartLightsaber::AsPartDummy()
{
    return 0;
}

extern float lightsaberdelta;
extern float lightsabersegmentlife;
extern int lightsaberSegments;


extern int lightsaberrender;
extern int renderpieces;
extern int lightsaberwireframe;
extern int lightsaberlighting;
extern int enablelightsaberbluradd;
extern int lightsaberrenderfronthalf;
extern float lightsabersegmentinterval;
extern float lightsaberfudgefactor;
extern int lightsabertotalsegments;
extern unsigned char g_useDistortionOutput;


extern int quaternionidentityoptimization;

void AurTextureRelease(CAurTexture* pTexture);

CAurTexture* AurTextureGetReference(char* sName, char* sExtension);

void MdlNodeLight::BinaryInternalDestructor()
{
    int n = FlareCount();
    if (n) {
        int i = n;
        int64_t l = i;
        do {
            l--;
            CAurTexture** pSlot = &FlareTextures()[l];
            CAurTexture* pTexture = *pSlot;
            if (pTexture)
                pTexture->Remove(reinterpret_cast<SafePointer<CAurTexture>*>(pSlot));
            *pSlot = 0;
            AurTextureRelease(pTexture);
            FlareCount()--;
        } while (l);
    }
    m_lFlareTextures.SetSize(0);
}

MdlNodeLight::~MdlNodeLight()
{
    if (m_nFlags != 0)
        DestructorDispatch();
    m_nFlags = 0;
    for (int64_t i = static_cast<int64_t>(m_lFlareNames.m_count) - 1; i != -1; i--) {
        char* pszName = m_lFlareNames.m_pData[i];
        if (pszName != 0)
            delete[] pszName;
        m_lFlareNames.m_count--;
    }
}

TileData::TileData()
{
    m_nFadeState = 4;
    // raw-offset: the zeroed run is written as pointer pairs; typed stores do not merge into the same q stores
    void** pQuad = reinterpret_cast<void**>(&m_pGob);
    pQuad[0] = pQuad[1] = 0;
    void** pFlames = reinterpret_cast<void**>(&m_paFlameParts[0]);  // raw-offset: as above, the parts and gobs
    pFlames[0] = pFlames[1] = pFlames[2] = pFlames[3] = 0;
    void** pLights = reinterpret_cast<void**>(&m_paLights[0]);  // raw-offset: as above
    pLights[0] = pLights[1] = 0;
    m_vBBoxMin = Vector(1000000.0f, 1000000.0f, 1000000.0f);
    m_vBBoxMax = Vector(1000000.0f, 1000000.0f, 1000000.0f);
    m_fOpacity = 1.0f;
    m_fBaseOpacity = 1.0f;
    m_vPosition.z = 0.0f;
    m_nMode = 0;
    m_nReserved1 = 0;
    m_bFogged = 0;
    m_fFade = 1.0f;
}

TileData::~TileData()
{
    delete m_pGob;
}

class MaxFace;
struct RenderVertexC;
struct RenderVertexV;

extern int renderaabb;
extern int countfaces;
extern int enabletransform;


static MdlNodeTriMesh* aabb_mesh __attribute__((used));
static CHitInfo* aabb_hitinfo __attribute__((used));
static int aabb_direction_heuristic __attribute__((used));
extern int aabb_testopposite;
int countboxes;

unsigned int BoxIntersect(const Vector& a, const Vector& b,
                          const Vector& boxMin, const Vector& boxMax);
PolyhitResult polyhit(const Vector* verts, int vertCount, const Vector& node,
                      const float& f, const Vector& a, const Vector& b, Vector* hit);

int HitCheckAABBnode(AABB* aabb, const Vector& a, Vector& b, float f)
{
    countboxes++;
    int hit;
    if (f == 1.0f) {
        hit = BoxIntersect(a, b, aabb->min, aabb->max);
    } else {
        Vector iMin = aabb->min * f;
        Vector iMax = aabb->max * f;
        hit = BoxIntersect(a, b, iMin, iMax);
    }
    if (!hit)
        return 0;

    int index = aabb->m_nIndex;
    if (index != -1) {
        MaxFace* face = &aabb_mesh->m_faces.m_pData[index];
        if (aabb_hitinfo->m_nMaterialMask & (1 << face->m_nMaterial)) {
            Vector verts[3];
            Vector* pVerts = reinterpret_cast<Vector*>(aabb_mesh->m_paVertices);
            verts[0] = pVerts[face->m_nVIndex[0]];
            verts[1] = pVerts[face->m_nVIndex[1]];
            verts[2] = pVerts[face->m_nVIndex[2]];
            if (polyhit(verts, 3, face->Normal(), face->m_fPlaneDist, a, b, &b)) {
                aabb_hitinfo->ClearOutput();
                aabb_hitinfo->m_nHitFaceIndex = index;
                aabb_hitinfo->m_nHitMaterialBit = 1 << face->m_nMaterial;
                aabb_hitinfo->m_vHitNormal = face->Normal();
                aabb_hitinfo->m_hitPosition = b;
                aabb_hitinfo->m_vLocalHitPosition = b;
                return 1;
            }
        }
        return 0;
    }

    int m = aabb->m_nDir & aabb_direction_heuristic;
    int r;
    if (aabb_testopposite) {
        if (m != 0) {
            r = HitCheckAABBnode(aabb->pChild1, a, b, f);
            return r + HitCheckAABBnode(aabb->pChild0, a, b, f);
        }
    } else {
        if (m == 0) {
            r = HitCheckAABBnode(aabb->pChild1, a, b, f);
            return r + HitCheckAABBnode(aabb->pChild0, a, b, f);
        }
    }
    r = HitCheckAABBnode(aabb->pChild0, a, b, f);
    return r + HitCheckAABBnode(aabb->pChild1, a, b, f);
}

int PartAABB::HitCheckGeom(const Vector& p1, Vector& p2, Vector* pOut, CHitInfo* pInfo, float f)
{
    MdlNode* node = m_pNode;
    aabb_mesh = node->AsMdlNodeTriMesh();
    aabb_hitinfo = pInfo;

    int heur = 0;
    heur |= (p1.x < p2.x) ? AABB::kDirPosX : AABB::kDirNegX;
    heur |= (p1.y < p2.y) ? AABB::kDirPosY : AABB::kDirNegY;
    heur |= (p1.z < p2.z) ? AABB::kDirPosZ : AABB::kDirNegZ;
    aabb_direction_heuristic = heur;

    AABB* pRoot = node->AsMdlNodeAABB()->m_pAABB;
    int result = HitCheckAABBnode(pRoot, p1, p2, f);
    if (pOut && result)
        *pOut = p2;
    return result;
}

void* PartAABB::AsPartAABB() { return this; }

void* PartAABB::AsPartDummy() { return 0; }

MdlNodeAABB::~MdlNodeAABB()
{
    if (m_nFlags != 0)
        DestructorDispatch();
    m_nFlags = 0;
    delete m_pAABB;
}


MdlNodeLightsaber::~MdlNodeLightsaber()
{
    if (m_nFlags != 0)
        DestructorDispatch();
    m_nFlags = 0;
}

void MdlNodeLightsaberPostProcess(void* pv)
{
    static_cast<MdlNodeLightsaber*>(pv)->InternalPostProcess();
}

void RenderFlat(VertexPrimitiveFlat* vertices);

extern int enabletransform;

unsigned int AnimMeshHitCheck(PartAnimMesh* part, MdlNodeAnimMesh* node, Vector a, Vector b,
                              Vector* out, float scale);

int PartAnimMesh::HitCheckGeom(Vector const& a, Vector& b, Vector* c, CHitInfo*, float f)
{
    MdlNodeAnimMesh* node = static_cast<MdlNodeAnimMesh*>(m_pNode);
    if ((node->m_nFlags & NODE_ANIMMESH) != NODE_ANIMMESH)
        node = 0;
    return static_cast<int>(AnimMeshHitCheck(this, node, a, b, c, f));
}

void CAurObject::DieWithoutParent()
{
    Gob* pGob = static_cast<Gob*>(this);
    if (pGob->m_pBehavior)
        pGob->m_pBehavior->Message(const_cast<char*>("diewithoutparent 1"));
}

void CAurObject::AnimateExternal(float fSpeed)
{
    Gob* pGob = static_cast<Gob*>(this);
    if (pGob->m_pScene) {
        pGob->m_pScene->m_nAnimateTick++;
        Animate(fSpeed);
        pGob->m_pScene->m_nAnimateTick++;
    } else {
        Animate(fSpeed);
    }
}

// A controller's 32-bit (PC) target offset to the offset of the same member in this build's Part
// classes. Under the MSVC ABI a derived part's first member does not sit in Part's tail padding, so
// the offsets are spelled by member.
#define K2_PART_OFFSET(C, m) static_cast<int>(K2_OFFSETOF(C, m))
#if KOTOR1
#define K2_PART_PICK(k1, k2) (k1)
#else
#define K2_PART_PICK(k1, k2) (k2)
#endif
int GetOffset64(int nOffset, int nType) {
    static std::map<int, int> offsetMapPart = {
#if KOTOR1  // K1 has no 45/46, adds 72
        {4, K2_PART_OFFSET(Part, m_pNode)},
        {8, K2_PART_OFFSET(Part, m_vLocalPosition)}, {20, K2_PART_OFFSET(Part, m_qLocalOrientation)},
        {36, K2_PART_OFFSET(Part, m_scale)}, {40, K2_PART_OFFSET(Part, m_nNodeID)},
        {44, K2_PART_OFFSET(Part, m_bDrawEnabled)}, {48, K2_PART_OFFSET(Part, m_paChildren)},
        {60, K2_PART_OFFSET(Part, m_pParent)}, {64, K2_PART_OFFSET(Part, m_pGob)},
        {68, K2_PART_OFFSET(Part, m_pMaterial)}, {72, K2_PART_OFFSET(Part, m_nLocalPositionSet)}
#else
        {4, K2_PART_OFFSET(Part, m_pNode)}, {8, K2_PART_OFFSET(Part, m_vLocalPosition)},
        {20, K2_PART_OFFSET(Part, m_qLocalOrientation)}, {36, K2_PART_OFFSET(Part, m_scale)},
        {40, K2_PART_OFFSET(Part, m_nNodeID)}, {44, K2_PART_OFFSET(Part, m_bDrawEnabled)},
        {45, K2_PART_OFFSET(Part, m_bHologramRendering)},
        {46, K2_PART_OFFSET(Part, m_bPerObjectIllumination)}, {48, K2_PART_OFFSET(Part, m_paChildren)},
        {60, K2_PART_OFFSET(Part, m_pParent)}, {64, K2_PART_OFFSET(Part, m_pGob)},
        {68, K2_PART_OFFSET(Part, m_pMaterial)}
#endif
    };
    static std::map<int, int> offsetMapTriMesh = {
        {76, K2_PART_OFFSET(PartTriMesh, m_vBBoxMin)}, {88, K2_PART_OFFSET(PartTriMesh, m_vBBoxMax)},
        {100, K2_PART_OFFSET(PartTriMesh, m_vSelfIllumColor)},
        {112, K2_PART_OFFSET(PartTriMesh, m_pVertexPrimitive)},
        {116, K2_PART_OFFSET(PartTriMesh, m_nColorPool)}, {120, K2_PART_OFFSET(PartTriMesh, m_nUVPool)},
        {124, K2_PART_OFFSET(PartTriMesh, m_bHasColorPool)}, {125, K2_PART_OFFSET(PartTriMesh, m_bFog)},
        {128, K2_PART_OFFSET(PartTriMesh, m_pTileData)}, {132, K2_PART_OFFSET(PartTriMesh, m_fOpacity)},
        {136, K2_PART_OFFSET(PartTriMesh, _padac)}, {140, K2_PART_OFFSET(PartTriMesh, m_nPartID)},
        {144, K2_PART_OFFSET(PartTriMesh, m_pafBaseUVs)}
    };
    static std::map<int, int> offsetMapEmitter = {
        {76, K2_PART_OFFSET(PartEmitter, m_nBlendMode)}, {80, K2_PART_OFFSET(PartEmitter, m_fAlphaEnd)},
        {84, K2_PART_OFFSET(PartEmitter, m_fAlphaStart)}, {88, K2_PART_OFFSET(PartEmitter, m_fBirthRate)},
        {92, K2_PART_OFFSET(PartEmitter, m_fBounceCoeff)}, {96, K2_PART_OFFSET(PartEmitter, m_fCombineTime)},
        {100, K2_PART_OFFSET(PartEmitter, m_fDrag)}, {104, K2_PART_OFFSET(PartEmitter, m_fFPS)},
        {108, K2_PART_OFFSET(PartEmitter, m_fFrameEnd)}, {112, K2_PART_OFFSET(PartEmitter, m_fFrameStart)},
        {116, K2_PART_OFFSET(PartEmitter, m_fGrav)}, {120, K2_PART_OFFSET(PartEmitter, m_fLifeExp)},
        {124, K2_PART_OFFSET(PartEmitter, m_fMass)}, {128, K2_PART_OFFSET(PartEmitter, m_fP2PBezier2)},
        {132, K2_PART_OFFSET(PartEmitter, m_fP2PBezier3)}, {136, K2_PART_OFFSET(PartEmitter, m_fParticleRot)},
        {140, K2_PART_OFFSET(PartEmitter, m_fRandVel)}, {144, K2_PART_OFFSET(PartEmitter, m_fSizeStart)},
        {148, K2_PART_OFFSET(PartEmitter, m_fSizeEnd)}, {152, K2_PART_OFFSET(PartEmitter, m_fSizeStartY)},
        {156, K2_PART_OFFSET(PartEmitter, m_fSizeEndY)}, {160, K2_PART_OFFSET(PartEmitter, m_fSpread)},
        {164, K2_PART_OFFSET(PartEmitter, m_fThreshold)}, {168, K2_PART_OFFSET(PartEmitter, m_fVelocity)},
        {172, K2_PART_OFFSET(PartEmitter, m_fXSize)}, {176, K2_PART_OFFSET(PartEmitter, m_fYSize)},
        {180, K2_PART_OFFSET(PartEmitter, m_fBlurLength)}, {184, K2_PART_OFFSET(PartEmitter, m_fLightningDelay)},
        {188, K2_PART_OFFSET(PartEmitter, m_fLightningRadius)}, {192, K2_PART_OFFSET(PartEmitter, m_fLightningScale)},
        {196, K2_PART_OFFSET(PartEmitter, m_fLightningSubDiv)}, {200, K2_PART_OFFSET(PartEmitter, m_fLightningZigzag)},
        {204, K2_PART_OFFSET(PartEmitter, m_fBirthTimer)}, {208, K2_PART_OFFSET(PartEmitter, m_fRadius)},
        {212, K2_PART_OFFSET(PartEmitter, m_fGravityAccel)}, {216, K2_PART_OFFSET(PartEmitter, m_fAlphaMid)},
        {220, K2_PART_OFFSET(PartEmitter, m_fPercentStart)}, {224, K2_PART_OFFSET(PartEmitter, m_fPercentMid)},
        {228, K2_PART_OFFSET(PartEmitter, m_fPercentEnd)}, {232, K2_PART_OFFSET(PartEmitter, m_fSizeMid)},
        {236, K2_PART_OFFSET(PartEmitter, m_fSizeMidY)}, {240, K2_PART_OFFSET(PartEmitter, m_fRandomBirthRate)},
        {244, K2_PART_OFFSET(PartEmitter, m_fDelayTimer)}, {248, K2_PART_OFFSET(PartEmitter, m_fControlPtTimer)},
        {252, K2_PART_OFFSET(PartEmitter, m_fTargetSize)}, {256, K2_PART_OFFSET(PartEmitter, m_fNumControlPts)},
        {260, K2_PART_OFFSET(PartEmitter, m_fControlPtRadius)}, {264, K2_PART_OFFSET(PartEmitter, m_fControlPtDelay)},
        {268, K2_PART_OFFSET(PartEmitter, m_fTangentSpread)}, {272, K2_PART_OFFSET(PartEmitter, m_fTangentLength)},
        {276, K2_PART_OFFSET(PartEmitter, m_nRenderMode)}, {280, K2_PART_OFFSET(PartEmitter, _pad130)},
        {284, K2_PART_OFFSET(PartEmitter, m_avVectors)},
        {296, K2_PART_OFFSET(PartEmitter, m_avVectors) + 12},
        {308, K2_PART_OFFSET(PartEmitter, m_avVectors) + 24},
        {320, K2_PART_OFFSET(PartEmitter, m_avVectors) + 36},
        {332, K2_PART_OFFSET(PartEmitter, m_avVectors) + 48},
        {344, K2_PART_OFFSET(PartEmitter, m_avVectors) + 60},
        {356, K2_PART_OFFSET(PartEmitter, m_avVectors) + 72},
        {368, K2_PART_OFFSET(PartEmitter, m_avVectors) + 84},
        {380, K2_PART_OFFSET(PartEmitter, m_avVectors) + 96},
        {392, K2_PART_OFFSET(PartEmitter, m_avVectors) + 108},
        {404, K2_PART_OFFSET(PartEmitter, m_qInheritOrientation)}, {420, K2_PART_OFFSET(PartEmitter, m_qWorldOrientation)},
        {436, K2_PART_OFFSET(PartEmitter, m_qPrevWorldOrientation)}, {452, K2_PART_OFFSET(PartEmitter, m_spWindManager)},
        {456, K2_PART_OFFSET(PartEmitter, m_lParticles)},
        {468, K2_PART_OFFSET(PartEmitter, m_lFreeParticles)},
        {480, K2_PART_OFFSET(PartEmitter, m_pParticleMaterial)},
        {484, K2_PART_OFFSET(PartEmitter, m_spTarget)},
        {488, K2_PART_OFFSET(PartEmitter, m_pHitInfo)},
        {492, K2_PART_OFFSET(PartEmitter, m_pTileData)},
        {496, K2_PART_OFFSET(PartEmitter, m_pTargetCallBack)}, {500, K2_PART_OFFSET(PartEmitter, m_nGradientMode)},
        {502, K2_PART_OFFSET(PartEmitter, m_bDetonate)}, {503, K2_PART_OFFSET(PartEmitter, m_bStarted)},
        {504, K2_PART_PICK(K2_PART_OFFSET(PartEmitter, m_bStarted) + 1, K2_PART_OFFSET(PartEmitter, m_bWeather))}
    };
    static std::map<int, int> offsetMapLight = {
        {76, K2_PART_OFFSET(PartLight, m_vLightColour)}, {88, K2_PART_OFFSET(PartLight, m_fLightRadius)},
        {92, K2_PART_OFFSET(PartLight, m_nCastsShadows)}, {96, K2_PART_OFFSET(PartLight, m_fShadowRadius)},
        {100, K2_PART_OFFSET(PartLight, m_fVerticalDisplacement)}, {104, K2_PART_OFFSET(PartLight, m_nAmbientOnly)},
        {108, K2_PART_OFFSET(PartLight, m_nAffectDynamic)}, {112, K2_PART_OFFSET(PartLight, m_nIsDynamic)},
        {116, K2_PART_OFFSET(PartLight, m_nLightPriority)},
        {120, K2_PART_OFFSET(PartLight, m_fEmitterScale)},
        {124, K2_PART_OFFSET(PartLight, m_fFadeWeightAlt)}, {128, K2_PART_OFFSET(PartLight, m_fDistanceRatio)},
        {132, K2_PART_OFFSET(PartLight, m_fCullDistance)}, {136, K2_PART_OFFSET(PartLight, m_fFlareFade)},
        {140, K2_PART_OFFSET(PartLight, m_fLightScale)}, {144, K2_PART_OFFSET(PartLight, m_fShadowFade)}
    };

    std::map<int, int>* pOffsetMap = &offsetMapPart;
    if (nOffset >= 76 && nType != NODE_DUMMY) {
        switch (nType) {
        case NODE_LIGHT:   pOffsetMap = &offsetMapLight;   break;
        case NODE_EMITTER: pOffsetMap = &offsetMapEmitter; break;
        case NODE_TRIMESH:
        case NODE_SKIN:
        case NODE_DANGLYMESH:
        case NODE_AABB:
        case NODE_SABER:
            pOffsetMap = &offsetMapTriMesh;
            break;
        case 4:  case 6:
        case NODE_ANIMMESH:
        case NODE_TRIGGER:
            pOffsetMap = &offsetMapPart;
            break;
        default:
            pOffsetMap = &offsetMapPart;
            break;
        }
    }
    if (pOffsetMap->find(nOffset) == pOffsetMap->end())
        return 0;
    return (*pOffsetMap)[nOffset];
}


extern int countmaxfaces;

MaxFace::MaxFace()
{
    m_nNormal[0] = 0;
    m_nNormal[1] = 0;
    m_nNormal[2] = 0;
    countmaxfaces++;
    m_nSurfaceMaterial = 0;
}


MaxFace_PostProcess::MaxFace_PostProcess()
{
    __builtin_memset(static_cast<void*>(this), 0, sizeof(*this));
}

List<Vector> MaxFace_PostProcess::texindices1;
List<Vector> MaxFace_PostProcess::texindices2;
List<Vector> MaxFace_PostProcess::texindices3;

#define PART_DTOR_DECLARED_ONLY




PartReference* PartReference::AsPartReference()
{
    return this;
}

void* PartReference::AsPartDummy()
{
    return 0;
}


void* PartCamera::AsPartDummy()
{
    return 0;
}




int PartProjection::m_nNextProjectionID = 1;

extern int countpart;

PartProjection::PartProjection(PartTriMesh* pTriMesh, PartLight* pLight)
    : m_pTriMesh(pTriMesh), m_pLight(pLight)
{
    m_pNode = 0;
    countpart++;
    m_nProjectionId = m_nNextProjectionID++;
}

PartProjection::~PartProjection()
{
    GLRender::DeletePool(m_nVertexPool);
    m_nVertexPool = -1;
    GLRender::DeletePool(m_nIndexPool);
    m_nIndexPool = -1;
}

