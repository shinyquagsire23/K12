// The game's scene root is Scene, the concrete CAurScene the engine instantiates (CAurScene carries
// the data, Scene overrides the interface). Alongside it are CAurRoom and the tile and room spawn
// helpers.

#include "k2stdint.h"
#include "port/aslglapi.h"
#include "aurres.h"
#if KOTOR1
#define K1_AURORA_LIST  // K1's AURORA library uses the larger List header
#endif
#include "aurscene.h"
#include "nullname.h"
#include "AURORA/aurmodel.h"
#include "aurscenemanager.h"
#include "include/aurbehavior.h"
#include "aurlightmanager.h"
#include "auremitter.h"
#include "aurcamera.h"
#include "aurgrass.h"
#include "aurprimitive.h"
#include "aurtrigger.h"
#include "aurtexman.h"
#include "Quaternion.h"
#include "Vector.h"
#include "Matrix.h"
#include <map>

int enablegetnearestlights;

int enableshadows = 1;
int enablegrass = 1;
int enablesoftshadows = 1;
int enableemitters = 1;
int shadowalpha = 1056964608;
int countdisplaycreates;
int g_bInScreenshotMode;
int enablerendertriggers;

const int Scene::FADING_OFF = 1;
const int Scene::FADING_ON = 2;
const int CAurScene::ENABLE_FADE = 1;
const int CAurScene::DISABLE_FADE = 2;
const int CAurScene::ALWAYS_FADE = 4;
int Scene::geomFadeEnabled = 1;
float Scene::cellsize = 10.0f;
float Scene::binsize = 2.5999999f;

extern float g_DesatureFactor;

Scene* currentscene;

int interleaveanimations = 1;
bool g_bEnableInterleavedAnimations;

#include "port/crtapi.h"
#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcstdlib.h"
#include "port/libcmath.h"

#include "aurwindmanager.h"

void SceneBSPRemoveTiledata(Scene* pScene, const Vector& v);
int AurTextureReplaceTileReferences(CAurScene* pScene, const Vector& v,
                                    char* pszFrom, char* pszTo);
TileData* SceneBSPGetTileData(Scene* pScene, const Vector& v);
void SceneBSPRemovePart(Scene* pScene, Part* pPart);
int SceneBSPAddPart(Scene* pScene, Part* pPart, Model* pModel);
void SceneBSPAddGob(Scene* pScene, Gob* pGob);
void SceneBSPRemoveGob(Scene* pScene, Gob* pGob);

void UpdateStaticLighting(Scene* pScene, PartLight* pLight);

int HitPartOnBSPNode(BSPNode* pNode, Vector vStart, Vector vEnd, Vector* pvHit);
int BSPHitCheck(BSPNode* pNode, Vector vStart, Vector vEnd, Vector* pvHit,
                int (*pfnTest)(BSPNode*, Vector, Vector, Vector*));

unsigned int BoxIntersect(const Vector& vStart, const Vector& vEnd,
                 const Vector& vBoxMin, const Vector& vBoxMax);

extern CHitInfo* CurrentHitInfo;

int equal(const Vector& a, const Vector& b);



// Bounded sprintf into a caller-provided buffer.
static void ScenePrintf(char* pszBuf, uint64_t nSize, const char* pszFmt, ...)
{
    __builtin_va_list ap;
    __builtin_va_start(ap, pszFmt);
    __vsprintf_chk(pszBuf, 0, nSize, pszFmt, ap);
    __builtin_va_end(ap);
}

// The port's GL layer. Every capability, blend factor, compare function, stencil op, face
// and material property goes through a per-kind translation table indexed by the port's
// own ordinal, so calls read `glEnable(enableTranslationTable[kCapBlend])` rather than
// `glEnable(GL_BLEND)`. The ordinals are named below.


void Transform(const Vector& v, const Quaternion& q, int n);

void MakeMatrix4x4(float* pMatrix, const Vector& v, const Quaternion& q);

static inline Vector RotateVector(const Vector& v, const Quaternion& q);




enum {
    kCapBlend       = 0,  // GL_BLEND
    kCapAlphaTest   = 1,  // GL_ALPHA_TEST
    kCapFog         = 2,  // GL_FOG
    kCapCullFace    = 3,  // GL_CULL_FACE
    kCapTexture2D   = 4,  // GL_TEXTURE_2D
    kCapLighting    = 5,  // GL_LIGHTING
    kCapStencilTest = 10,  // GL_STENCIL_TEST
    kCapDepthTest   = 11,  // GL_DEPTH_TEST
    kCapFogStart    = 15,  // GL_FOG_START
    kCapFogEnd      = 16,  // GL_FOG_END
    kCapFogDensity  = 17,  // GL_FOG_DENSITY
    kCapFogColor    = 18,  // GL_FOG_COLOR
};

enum {
    kArrayTexCoord = 0,  // GL_TEXTURE_COORD_ARRAY
    kArrayNormal   = 1,  // GL_NORMAL_ARRAY
    kArrayColor    = 2,  // GL_COLOR_ARRAY
    kArrayVertex   = 3,  // GL_VERTEX_ARRAY
};

enum { kCmpLEqual = 3, kCmpGreater = 4, kCmpAlways = 7 };  // GL_LEQUAL / GL_GREATER / GL_ALWAYS
enum { kStencilKeep = 0, kStencilReplace = 2,  // GL_KEEP / GL_REPLACE
       kStencilIncr = 3, kStencilDecr = 4 };  // GL_INCR / GL_DECR — the shadow-volume pair
enum { kFaceCW = 0, kFaceCCW = 1 };  // GL_CW / GL_CCW (cullTranslationTable)
enum { kFaceFront = 0 };  // GL_FRONT
enum { kMaterialAmbient = 1, kMaterialDiffuse = 2, kMaterialEmission = 3 };
enum { kColorTrackingEmission = 0 };  // colorTrackingTranslationTable[0] = GL_EMISSION

int  AurGetAvailableStencilBits();
bool AurNV2xLevelShadersAvailable();
int  AurSoftShadowsAvailable();
int  AurATISoftShadowsAvailable();

Part* FindPart(Part* pPart, char* pszName);
float magnitude(const Vector& v);
Vector planelineintersection(const Vector& vPlaneNormal, float fPlaneD, const Vector& vOrigin,
                             const Vector& vDir);
void ProjectDynamicShadow(Gob* pGob, PartLight* pLight);

void SetBeaming();
int  AurGetStencilClearValue();
void AurVertexProgramMakeCurrent();
void IosSetDistortBuffer(bool bEnable);
#if !KOTOR1
void SetDistortBuffer(bool bEnable);
#endif

extern List<Part*> staticbeambucket;
extern List<Part*> staticshadowbucket;
extern List<PartTrigger*> triggerbucket;
extern List<Gob*>  gobbucket;
extern List<Gob*>  blurbucket;
extern List<Gob*>  g_listForcePushBucket;
extern List<Gob*>  g_listLightsaberBucket;
extern List<PartEmitter*> emitterbucket;
extern List<CAurTriangleBin*> binbucket;
extern List<PartTriMesh*> meshshadowbucket;

// Per-texture-id bucket of the meshes whose material has an alpha-cap pass.
#if KOTOR1
extern Bucket capbuckets[5000];
#else
extern std::map<unsigned int, Bucket> capbuckets;
#endif

// Texture priority order; every caller copies the returned list.
List<int>* AurTextureGetOrdering();
extern List<Gob*>  g_listHologramBucket;

extern List<PartTriMesh*> g_listBackgroundBucket;

// Per-texture-id bucket of meshes, the sibling of capbuckets.
#if KOTOR1
extern Bucket meshbuckets[5000];
#else
extern std::map<unsigned int, Bucket> meshbuckets;
#endif

// Per-texture-id bucket of the parts drawn through the fade pass.
#if KOTOR1
extern Bucket fadebuckets[5000];
#else
extern std::map<unsigned int, Bucket> fadebuckets;
#endif

int GobOrderCmp(const void* a, const void* b);

int EmitterOrderCmp(const void* a, const void* b);

int enablebeams = 1;
extern int  enableshadows;
extern int  enablesoftshadows;
#ifdef K2_QOL_SOFT_SHADOWS
extern bool g_bQolSoftShadowPass;   // aurglcore.cpp: SetUpSoftShadows set up the shadow mask
#endif
int enablefocusshadowing = 1;
float maxshadowdist = 35.0f;
float shadowlength = 5.0f;
extern int  enablerender;
int forcefogtest;
int renderpersonalspace;
int renderpaths;
int renderlensflares = 1;
int hitcheckflare = 1;
int reverseflarehit = 1;
float flarespeed = 3.0f;
float lensflaresize = 0.100000001f;
extern unsigned char g_ForceSight;
int newsorting = 1;
int numgobs;
#if KOTOR1  // K1 has no distortion output: every guarded depth-mask write is unconditional
static const bool g_useDistortionOutput = false;
#else
extern unsigned char g_useDistortionOutput;
#endif
extern int  enableemitters;
extern int  enablegrass;
float alphatest;
float grassalphatest = 0.400000006f;

extern List<CAurGrassInfo> g_GrassInfo;

extern Camera* CurrentCamera;

int  AurMultiTextureAvailable();
bool AurGrassMaterialInUse(int nMaterial);

extern float DeltaT;

int countvisibilityculls;
int rendertransparentcharacters;
int showlightpriorityinfo;
int dogobrender = 1;

int enablevisibilitytest;
float visibilitycutoffratio = 0.0399999991f;
float visibilitycheckdelay = 0.330000013f;

static void AurDefaultRenderCallback() {}
void (*AurRenderCallback)() = AurDefaultRenderCallback;

__attribute__((visibility("hidden"))) char g_bDistortBufferUsed;

extern int usearbvertexprograms;
int enablerenderstatic = 1;
extern int enablerendertriggers;
int enablerenderdynamic = 1;

float* Homogenize(const Vector& v, const float& w);

// The port's framebuffer-effect bracket; not present in the desktop build.
void FrameBufferModificationsStartIos(bool bEnable);
void FrameBufferModificationsEndIos(bool bEnable);

void SetShadowStencilBuffer();

void RenderPersonalSpace(Scene* pScene);
void RenderPaths(Scene* pScene);


Vector Scene::m_vLastCameraPosition;
List<Scene*> Scenes;
CHitInfo DefaultHitInfo(0, CHitInfo::kMaterialAllButTrigger, 0);
Scene* FindScene(char* pszName);
Gob*   FindGob(char* pszName);

int g_FocusModelCount;

CAurTexture* AurTextureGetReference(char* pszName, char* pszAltName);
CAurTexture* AurTextureGetNormalizationCubeMap();
int  AurATIFragmentShaderAvailable();
#if KOTOR1
int  GetMiniGamePlayingType();
#endif

#include "aurvertexprog.h"


extern template SafePointer<CAurTexture>& SafePointer<CAurTexture>::operator=(CAurTexture*);

Scene::Scene(char* pszName)
    : m_focusGob(const_cast<char*>("c_FocusGob"))
{
    m_szName[0] = '\0';

    if (pszName != 0) {
        char szName[64];
        ScenePrintf(szName, sizeof(szName), "%s", pszName);

        int nSerial = 0;
        while (FindScene(szName) != 0 || FindGob(szName) != 0) {
            nSerial++;
            ScenePrintf(szName, sizeof(szName), "%s%d", pszName, nSerial);
        }

        __strcpy_chk(m_szName, szName, sizeof(m_szName));
    }

    char szModel[32];
    ScenePrintf(szModel, sizeof(szModel), "c_FocusGobDummyModel%d", g_FocusModelCount);
    g_FocusModelCount++;

    Model* pModel = new Model(szModel);
    m_focusGob.m_pModel = pModel;
    pModel->AddRef(&m_focusGob);

    m_ambient.x = 0.0f;
    m_ambient.y = 0.0f;
    m_ambient.z = 0.0f;

    m_bFogEnabled = 0;
    m_nFogMode = 0;
    m_fFogStart = 5.0f;
    m_fFogEnd = 40.0f;
    m_fFogDensity = 0.1f;
    m_vFogColor.x = 1.0f;
    m_vFogColor.y = 1.0f;
    m_vFogColor.z = 1.0f;
#if !KOTOR1
    m_vFogColorGrey.x = m_vFogColor.x * 0.3f * g_DesatureFactor;
    m_vFogColorGrey.y = m_vFogColor.y * 0.59f * g_DesatureFactor;
    m_vFogColorGrey.z = m_vFogColor.z * 0.11f * g_DesatureFactor;
#endif

    m_nAnimateTick = 0;

    m_pLightManager = new LightManager();
    m_pWindManager = new WindManager();
    m_pNormalizationCubeMap = AurTextureGetNormalizationCubeMap();

    m_fHighlightPhase = 0.0f;
    m_nIgnoreVisibilityGraph = 1;

    Scenes.Add(this);

    m_pBSPRoot = 0;
    m_bAnimating = true;
    m_bHasBins = false;
    m_pReserved1 = 0;
    m_fReserved1 = 1.0f;
    m_fogOfWarRooms.m_count = 0;

    m_focusGob.SetScene(this);
    m_pCurrentRoom = 0;

#if KOTOR1  // K1 uses two textures and records the mini-game type
    m_pFilmNoiseTexture   = AurTextureGetReference(const_cast<char*>("filmnoisetex"), 0);
    m_pDistortionTexture  = AurTextureGetReference(const_cast<char*>("distortiontex"), 0);

    if (AurATIFragmentShaderAvailable())
        VertexProgramManager::ResetVertexPrograms();
    m_nMiniGameType = GetMiniGamePlayingType();
#else
    m_pDirtyTexture       = AurTextureGetReference(const_cast<char*>("dirtytex"), 0);
    m_pDirtyTexture2      = AurTextureGetReference(const_cast<char*>("dirtytex2"), 0);
    m_pDirtyTexture3      = AurTextureGetReference(const_cast<char*>("dirtytex3"), 0);
    m_pFilmNoiseTexture   = AurTextureGetReference(const_cast<char*>("filmnoisetex"), 0);
    m_pDistortionTexture  = AurTextureGetReference(const_cast<char*>("distortiontex"), 0);
    m_pLensFlareTexture   = AurTextureGetReference(const_cast<char*>("fx_Clair01"), 0);
    m_pHologramTexture    = AurTextureGetReference(const_cast<char*>("Holotex"), 0);

    if (AurATIFragmentShaderAvailable())
        VertexProgramManager::ResetVertexPrograms();
#endif
}

void Scene::SetFog(int bEnable)
{
    m_bFogEnabled = bEnable;
}

void Scene::SetFogRange(float fStart, float fEnd)
{
    m_fFogStart = fStart;
    m_fFogEnd = fEnd;
}

void Scene::GetFogRange(float& fStart, float& fEnd)
{
    fStart = m_fFogStart;
    fEnd = m_fFogEnd;
}

void Scene::SetFogColor(const Vector& v)
{
    m_vFogColor = v;

#if !KOTOR1  // K1 only stores it
    float fGrey = (m_vFogColor.x * 0.3f + m_vFogColor.y * 0.59f
                   + m_vFogColor.z * 0.11f) * g_DesatureFactor;
    m_vFogColorGrey.x = m_vFogColorGrey.y = m_vFogColorGrey.z = fGrey;
#endif
}

// These Scene overrides of the CAurScene interface deliberately do nothing.

void Scene::FinishSceneGeometry()
{
}

void Scene::PostProcessLS()
{
}

void Scene::InitBSP(int a, int b, int c, bool d, float e)
{
    K2_UNUSED(a); K2_UNUSED(b); K2_UNUSED(c); K2_UNUSED(d); K2_UNUSED(e);
}

void Scene::ListSpringPartsR(Part* pPart)
{
    K2_UNUSED(pPart);
}


void Scene::DeleteTile(const Vector& v)
{
    SceneBSPRemoveTiledata(this, v);
}

void Scene::ReplaceTileTexture(const Vector& v, char* pszFrom, char* pszTo)
{
    AurTextureReplaceTileReferences(this, v, pszFrom, pszTo);
}

void Scene::ChildAdd(Gob* pGob)
{
    if (pGob->m_pModel != 0)
    {
        SceneBSPAddGob(this, pGob);
        ListSpecialPartsR(pGob->m_pPart);
    }

    Objects().Add(pGob);
    if (pGob->AsCamera() != 0)
        m_cameras.Add(pGob);
}

void Scene::ChildRemove(Gob* pGob)
{
    if (pGob->m_pModel != 0)
    {
        SceneBSPRemoveGob(this, pGob);
        UnlistSpecialParts(pGob->m_pPart);
    }

    Objects().Remove(pGob);
    if (pGob->AsCamera() != 0)
        m_cameras.Remove(pGob);
}

int systemmemoryshadows;

void ProjectDynamicShadow(Gob* pGob, PartLight* pLight)
{
    float fRadius = pLight->m_fLightRadius;

    if (systemmemoryshadows)
    {
        pGob->PartVolumeProject(pGob->m_pPart,
                                pLight->m_vWorldPosition + Vector(0.0f, 0.0f, pLight->m_fVerticalDisplacement),
                                pLight->m_fShadowRadius > 0.0f ? pLight->m_fShadowRadius : fRadius,
                                Plane());
        return;
    }

    Plane* pPlane;
    if (fRadius > 1000.0f)
    {
        pPlane = new Plane(0.0f, 0.0f, -1.0f, -0.1f);
    }
    else
    {
        const Plane* paVolumes = CurrentCamera->m_viewState.m_aViewVolumes.m_pData;
        pPlane = new Plane(paVolumes[1].a, paVolumes[1].b, paVolumes[1].c, paVolumes[1].d + 0.05f);
    }

    pGob->PartVolumeProject(pGob->m_pPart,
                            pLight->m_vWorldPosition + Vector(0.0f, 0.0f, pLight->m_fVerticalDisplacement),
                            pLight->m_fShadowRadius > 0.0f ? pLight->m_fShadowRadius : fRadius,
                            *pPlane);
    delete pPlane;
}

void Scene::ListSpecialParts(Part* pPart)
{
    ListSpecialPartsR(pPart);
}

void* Scene::HitCheckGob(Vector vStart, Vector vEnd, Vector* pvHit, CHitInfo* pInfo)
{
    void* pHit = 0;

    for (int64_t i = 0; i < Objects().m_count; i++)
    {
        Gob* pGob = Objects().m_pData[i];

        if (pGob->m_pModel)
        {
            if ((pGob->m_pModel->m_nFlags & 1) && !pInfo->m_bIncludeEffects)
                continue;
            if (pGob->IsForcePushDistortion())
                continue;
            if (pGob->m_pModel->m_nFlags & Model::kModelHeatDistortion)
                continue;
        }

        if (pGob->m_pModel && pInfo->m_bSkipCharactersAndItems)
        {
            if (pGob->m_pModel->m_nFlags & (Model::kModelCharacter | Model::kModelLightsaber))
                continue;
            if ((pGob->m_pModel->m_nFlags & (Model::kModelPlaceable | Model::kModelItem)) == Model::kModelItem)
                continue;
        }

        if (pInfo->m_ignoreList.Contains(static_cast<CAurObject*>(pGob)))
            continue;

        void* pGobHit = pGob->HitCheck(vStart, vEnd, &vEnd, pInfo);
        if (pGobHit)
            pHit = pGobHit;
    }

    if (pvHit && pHit)
        *pvHit = vEnd;

    return pHit;
}



CHitInfo::CHitInfo(int a, int b, int c)
{
    m_bSkipObjects = a;
    m_nMaterialMask = b;
    *static_cast<volatile int*>(&m_ignoreList.m_count) = 0;
    m_bHitBoundingBoxes = c;
    m_bIgnoreStartInside = 0;
    m_bSkipCharactersAndItems = 0;
    m_bSkipFaded = 1;
    m_bHonorIgnoreHitCheck = 0;
    m_bSkipRooms = 0;
    m_bIncludeEffects = 0;
    ClearOutput();
}

void CHitInfo::Reset(int a, int b, int c)
{
    m_bSkipObjects = a;
    m_nMaterialMask = b;
    m_ignoreList.m_count = 0;
    m_bHitBoundingBoxes = c;
    m_bIgnoreStartInside = 0;
    m_bSkipCharactersAndItems = 0;
    m_bSkipFaded = 1;
    m_bHonorIgnoreHitCheck = 0;
    m_bSkipRooms = 0;
    m_bIncludeEffects = 0;
    ClearOutput();
}

void CHitInfo::ClearOutput()
{
    m_nHitType = 0;
    m_hitObject = 0;  // SafePointer::operator= unregisters and nulls
    *static_cast<volatile int*>(&m_nHitMaterialBit) = 0;
    __builtin_memset(&m_vHitNormal, 0, sizeof(m_vHitNormal) + sizeof(m_pszHitPartName));
    __builtin_memset(&m_hitPosition, 0, sizeof(m_hitPosition) + sizeof(m_vLocalHitPosition));
    m_nHitFaceIndex = -1;
    __builtin_memset(&m_pHitTriggerOwner, 0, sizeof(m_pHitTriggerOwner) + sizeof(m_pHitRoom));
}

int Scene::HitCheck(Vector vStart, Vector vEnd, Vector* pvHit, CHitInfo* pInfo)
{
    if (!pInfo)
    {
        pInfo = &DefaultHitInfo;
        pInfo->ClearOutput();
    }

    CurrentHitInfo = pInfo;

    if (!pInfo->m_bSkipRooms)
    {
        if (!m_rooms.m_count)
        {
            if (BSPHitCheck(m_pBSPRoot, vStart, vEnd, &vEnd, HitPartOnBSPNode))
            {
                pInfo->m_nHitType = 1;
                pInfo->m_hitPosition = vEnd;
                if (pvHit)
                    *pvHit = vEnd;
            }
        }

        for (int64_t i = 0; i < m_rooms.m_count; i++)
        {
            CAurRoom* pRoom = m_rooms.m_pData[i];
            if (BoxIntersect(vStart, vEnd, pRoom->m_bboxMin, pRoom->m_bboxMax)
                && HitPartOnBSPNode(static_cast<BSPNode*>(pRoom), vStart, vEnd, &vEnd))
            {
                pInfo->m_nHitType = 1;
                pInfo->m_hitPosition = vEnd;
                if (pvHit)
                    *pvHit = vEnd;
                pInfo->m_pHitRoom = pRoom;
            }
        }
    }

    CurrentHitInfo = 0;

    CAurObject* pHit = 0;
    if (!pInfo->m_bSkipObjects)
    {
        pHit = static_cast<CAurObject*>(HitCheckGob(vStart, vEnd, &vEnd, pInfo));
        if (pHit)
        {
            pInfo->m_nHitType = 2;
            pInfo->m_hitPosition = vEnd;
            if (pvHit)
                *pvHit = vEnd;
        }
    }

    pInfo->m_hitObject = pHit;
    return pInfo->m_nHitType;
}

void Scene::RenderShadows(int, int, int bStencilShadows, int bClearStencil)
{
    int nStencilBits = AurGetAvailableStencilBits();
    if (!nStencilBits)
        return;

    GLRender::Disable(Aurora::kCapTexture2D);
    GLRender::Disable(Aurora::kCapAlphaTest);
    GLRender::Disable(Aurora::kCapLighting);
    ASLgl::glStencilMask(0xff);
    GLRender::Enable(Aurora::kCapStencilTest);

    if (bClearStencil)
        ASLgl::glClear(GL_STENCIL_BUFFER_BIT);

    if (bStencilShadows)
    {
        PartLight* pLight = m_focusGob.m_pLightContainer->m_pSelectedLight;
        if (pLight && pLight->m_vWorldPosition.z > m_vSceneFocus.z + 1.75f)
        {
            if (enablesoftshadows && AurNV2xLevelShadersAvailable()
                && (AurSoftShadowsAvailable() || AurATISoftShadowsAvailable())
                && GLRender::cm_bSetUpImageSpaceShadows)
                GLRender::SetUpSoftShadows();

            ASLgl::glStencilMask(0xff);
            GLRender::Enable(Aurora::kCapStencilTest);
            RenderInterface::SetDepthMask(0);
            ASLgl::glColorMask(0, 0, 0, 0);
            GLRender::SetStencilFunc(Aurora::kCmpAlways, 0xf, 0xf);

            unsigned int nNormalArray = clientStateTranslationTable[kArrayNormal];
            ASLgl::glDisableClientState(nNormalArray);
            unsigned int nTexCoordArray = clientStateTranslationTable[kArrayTexCoord];
            ASLgl::glDisableClientState(nTexCoordArray);
            unsigned int nColorArray = clientStateTranslationTable[kArrayColor];
            ASLgl::glDisableClientState(nColorArray);

            DoGobShadowsForThisLight(*pLight);

            ASLgl::glEnableClientState(nNormalArray);
            ASLgl::glEnableClientState(nTexCoordArray);
            ASLgl::glEnableClientState(nColorArray);
            ASLgl::glColorMask(1, 1, 1, 1);
            ASLgl::glStencilMask(0);

            RenderStencilShadowsForThisLight(*pLight,
                                             m_focusGob.m_pLightContainer->m_fSelectedWeight,
                                             nStencilBits);

            if (enablesoftshadows && AurNV2xLevelShadersAvailable()
                && (AurSoftShadowsAvailable() || AurATISoftShadowsAvailable())
                && GLRender::cm_bSetUpImageSpaceShadows)
                GLRender::FinishSoftShadows();

            ASLgl::glStencilMask(0xff);
        }
    }

    GLRender::EnableClientState(Aurora::kClientStateNormal);
    GLRender::EnableClientState(Aurora::kClientStateTexCoord);
    if (LightManager::m_pOnlyGob)
        GLRender::EnableClientState(Aurora::kClientStateColor);
    RenderInterface::SetDepthMask(1);

    GLRender::Enable(Aurora::kCapTexture2D);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapLighting);
    GLRender::Disable(Aurora::kCapStencilTest);
}

void Scene::DoGobShadows()
{
    List<PartLight*> shadowLights;
    shadowLights = m_pLightManager->GetShadowLights(0);
    if (shadowLights.m_count == 0)
        return;

    for (int i = gobbucket.m_count; i != 0; i--)
    {
        if (interleaveanimations && g_bEnableInterleavedAnimations)
        {
            if (i <= gobbucket.m_count)
            {
                if (gobbucket.m_pData[i - 1]->m_pBehavior)
                    gobbucket.m_pData[i - 1]->Animate(DeltaT);
            }
            if (i > gobbucket.m_count)
                continue;
        }

        if (!gobbucket.m_pData[i - 1]->m_nShadows)
            continue;
        if (!gobbucket.m_pData[i - 1]->m_bHasGeometry)
            continue;

        if (enablefocusshadowing)
        {
            if (magnitude(currentscene->m_vSceneFocus - gobbucket.m_pData[i - 1]->m_vPosition)
                > maxshadowdist)
                continue;
        }

        if (gobbucket.m_pData[i - 1]->m_pModel
            && !(gobbucket.m_pData[i - 1]->m_pModel->m_nFlags & Model::kModelCharacter))
        {
            if (!gobbucket.m_pData[i - 1]->m_pBehavior)
                continue;
            if (!gobbucket.m_pData[i - 1]->m_pBehavior->AsBehaviorAttach())
                continue;
        }

        for (int j = 0; j < shadowLights.m_count; j++)
        {
            PartLight* pLight = shadowLights.m_pData[j];
            float fRadius = pLight->m_fLightRadius;
            float fFalloff = pLight->m_fShadowRadius;
            if (magnitude(pLight->m_vWorldPosition - gobbucket.m_pData[i - 1]->m_vPosition)
                < (fFalloff > 0.0f ? fFalloff : fRadius))
                ProjectDynamicShadow(gobbucket.m_pData[i - 1], shadowLights.m_pData[j]);
        }
    }
}

void Scene::DoGobShadowsForThisLight(PartLight& light)
{
    float fRadius = light.m_fLightRadius;
    float fFalloff = light.m_fShadowRadius;
    float fRange = fFalloff > 0.0f ? fFalloff : fRadius;
#if !KOTOR1
    Vector vFocus = m_focusGob.m_vPosition;
#endif

    for (int i = gobbucket.m_count; i--; )
    {
        if (interleaveanimations && g_bEnableInterleavedAnimations)
        {
            if (i < gobbucket.m_count)
            {
                if (gobbucket.m_pData[i]->m_pBehavior)
                    gobbucket.m_pData[i]->Animate(DeltaT);
            }
            if (i >= gobbucket.m_count)
                continue;
        }

        Gob* pGob = gobbucket.m_pData[i];
        if (!pGob->m_nShadows)
            continue;
        if (!pGob->m_bHasGeometry)
            continue;
        if (!(pGob->m_pModel->m_nFlags & (Model::kModelCharacter | Model::kModelItem)))
            continue;

        Vector vPos = pGob->m_vPosition;
#if !KOTOR1  // K1 has no focus-distance cull on gob shadows
        float dx = vPos.x - vFocus.x;
        float dy = vPos.y - vFocus.y;
        float dz = vPos.z - vFocus.z;
        if (dx * dx + dy * dy + dz * dz > 1100.0f)
            continue;
#endif

        if (pGob->IsInCutscene())
        {
            Part* pDummy = FindPart(pGob->m_pPart, const_cast<char*>("cutscenedummy"));
            Quaternion qOrient;
            if (pDummy)
                pDummy->GetWorld(vPos, qOrient);
        }

        if (enablefocusshadowing)
        {
            if (magnitude(currentscene->m_vSceneFocus - vPos) > maxshadowdist)
                continue;
        }

        if (fRange > magnitude(light.m_vWorldPosition - vPos))
            ProjectDynamicShadow(pGob, &light);
    }
}

void Scene::RenderStencilShadowsForThisLight(PartLight& light, float fWeight, int nStencilBits)
{
    float fFalloff = light.m_fShadowRadius;
    float fLightRadius = light.m_fLightRadius;
    float fRange = fFalloff > 0.0f ? fFalloff : fLightRadius;
    if (fRange == 0.0f)
        return;

    static RenderVertexVC* pVertexArray = new RenderVertexVC[11];

    float fRadius;
    if (fRange > 50.0f)
    {
        pVertexArray[0].pos[0] = m_vSceneFocus.x;
        pVertexArray[0].pos[1] = m_vSceneFocus.y;
        pVertexArray[0].pos[2] = m_vSceneFocus.z;
        fRadius = 50.0f;
    }
    else
    {
        float dz = __builtin_fabsf(light.m_vWorldPosition.z - m_vSceneFocus.z);
        float d = fRange * fRange - dz * dz;
        if (d < 0.0f)
            d = 0.0f;
        fRadius = __builtin_sqrtf(d);
        pVertexArray[0].pos[0] = light.m_vWorldPosition.x;
        pVertexArray[0].pos[1] = light.m_vWorldPosition.y;
        pVertexArray[0].pos[2] = m_vSceneFocus.z;
    }

    GLRender::SetStencilFunc(Aurora::kCmpLEqual, 1 << (nStencilBits - 1), 0xffffffff);
    GLRender::SetStencilOp(Aurora::kStencilKeep, Aurora::kStencilKeep, Aurora::kStencilKeep);
    GLRender::Disable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapBlend);

#ifdef K2_QOL_SOFT_SHADOWS
    // The mask only when SetUpSoftShadows set one up (aurglcore.cpp, QolSoftShadowsSetUp).
    if (g_bQolSoftShadowPass)
#else
    if (enablesoftshadows && AurNV2xLevelShadersAvailable()
        && (AurSoftShadowsAvailable() || AurATISoftShadowsAvailable())
        && GLRender::cm_bSetUpImageSpaceShadows)
#endif
        GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendZero);
    else
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);

    pVertexArray[0].color = static_cast<unsigned int>(fWeight * 0.5f * 255.0f) << 24;

    for (int i = 0; i < 10; i++)
    {
        float fAngle = i * (40.0f * 3.14159265f / 180.0f);
        pVertexArray[i + 1].pos[0] = pVertexArray[0].pos[0] + fRadius * static_cast<float>(__builtin_cos(fAngle));
        pVertexArray[i + 1].pos[1] = pVertexArray[0].pos[1] + fRadius * static_cast<float>(__builtin_sin(fAngle));
        pVertexArray[i + 1].pos[2] = m_vSceneFocus.z;
        pVertexArray[i + 1].color = 0;
    }

    GLRender::RenderObjectsVCUP(static_cast<Aurora::AuroraPrimitiveTypes>(5), pVertexArray, 11);
    GLRender::Enable(Aurora::kCapDepthTest);
}

void Scene::ListSpecialPartsR(Part* pPart)
{
    if (!pPart)
        return;

    PartLight* pLight = pPart->AsPartLight();
    if (pLight)
    {
        m_pLightManager->AddLight(pLight);
        SceneBSPAddPart(this, pLight, 0);
    }

    PartEmitter* pEmitter = pPart->AsPartEmitter();
    if (pEmitter)
    {
        SceneBSPAddPart(this, pEmitter, 0);
        m_emitters.AddUnique(pEmitter);
    }

    for (int64_t i = 0; i < pPart->m_nChildren; i++)
        ListSpecialPartsR(pPart->m_paChildren[i]);
}

Vector Scene::GetWindVector(Vector v, float f)
{
    return m_pWindManager->GetGlobalWind(v, f);
}

void Scene::AddObject(CAurObject* pObject)
{
    ChildAdd(static_cast<Gob*>(pObject));
}

bool Scene::SetObjectScale(CAurObject* pObject, float fScale)
{
    return static_cast<Gob*>(pObject)->SetObjectScale(fScale, true);
}

float Scene::GetObjectScale(CAurObject* pObject)
{
    return static_cast<Gob*>(pObject)->m_fObjectScale;
}

void Scene::SetBlindLights(CAurObject* pObject)
{
    LightManager::m_pOnlyGob = static_cast<Gob*>(pObject);
}


void Scene::EnableAnimations()
{
    m_bAnimating = true;
}

void Scene::DisableAnimations()
{
    m_bAnimating = false;
}

bool Scene::IsAnimating()
{
    return m_bAnimating;
}


void Scene::SetSceneFocus(Vector v)
{
    m_vSceneFocus = v;
}

int Scene::GetObjectCount()
{
    return Objects().m_count;
}

int Scene::GetNumRooms()
{
    return m_rooms.m_count;
}

CAurRoom* Scene::GetRoom(int i)
{
    if (i < 0 || i >= m_rooms.m_count)
        return 0;
    return m_rooms.m_pData[i];
}

Scene* Scene::SetCurrentRoom(CAurRoom* pRoom)
{
    m_pCurrentRoom = pRoom;
    return this;
}

void Scene::SetCurrentRoom(int i)
{
    if (i < 0 || i >= m_rooms.m_count)
        m_pCurrentRoom = 0;
    else
        m_pCurrentRoom = m_rooms.m_pData[i];
}

int Scene::EnableVisibilityGraph()
{
    CAurRoom* pRoom = m_pCurrentRoom;
    m_nIgnoreVisibilityGraph = 0;
    return pRoom != 0;
}

void Scene::DisableVisibilityGraph()
{
    m_nIgnoreVisibilityGraph = 1;
}

int Scene::GetIgnoreVisibilityGraph()
{
    return m_nIgnoreVisibilityGraph;
}

int Scene::IsGeomFadeEnabled()
{
    return geomFadeEnabled == 1;
}

bool Scene::SetGeomFadeProperties(int nMode)
{
    switch (nMode)
    {
    case 1:  // ENABLE_FADE
    case 2:  // DISABLE_FADE
    case 4:  // ALWAYS_FADE
        geomFadeEnabled = nMode;
        return 1;
    }
    return 0;
}


#if KOTOR1
// K1 stops the scan at the match and reads the slot again for the result.
Gob* Scene::GetObject(char* pszName)
{
    int i;
    for (i = 0; i < Objects().m_count; i++)
    {
        if (_stricmp(pszName, Objects().m_pData[i]->m_szName) == 0)
            break;
    }
    if (i < Objects().m_count)
        return Objects().m_pData[i];
    return 0;
}
#else
Gob* Scene::GetObjectA(char* pszName)
{
    for (int i = 0; i < Objects().m_count; i++)
    {
        if (_stricmp(pszName, Objects().m_pData[i]->m_szName) == 0)
            return Objects().m_pData[i];
    }
    return 0;
}
#endif

CAurRoom* Scene::FindRoom(char* pszName)
{
    for (int i = 0; i < m_rooms.m_count; i++)
    {
        if (_stricmp(pszName, m_rooms.m_pData[i]->m_szName) == 0)
            return m_rooms.m_pData[i];
    }
    return 0;
}

void Scene::DoStaticMeshShadows()
{
    PartLight* pElectedLight = m_pLightManager->m_pShadowSun;
    List<PartLight*> shadowLights = m_pLightManager->GetShadowLights(1);
    if (shadowLights.m_count == 1 && pElectedLight != shadowLights.m_pData[0])
        return;

    for (int i = 0; i < staticshadowbucket.m_count; i++)
        staticshadowbucket.m_pData[i]->Draw(false);
}

void Scene::DoStaticMeshBeams()
{
    if (!enablebeams)
        return;

    SetBeaming();
    for (int i = 0; i < staticbeambucket.m_count; i++)
        staticbeambucket.m_pData[i]->Draw(false);
}

void Scene::DoDynamicMeshProjections()
{
// The far frustum plane, in world space, read once for the whole pass.
    const Plane& qFar = CurrentCamera->m_viewState.m_aViewVolumes[1];
    Plane planeFar(qFar.a, qFar.b, qFar.c, qFar.d);

    List<PartLight*> shadowLights;
    shadowLights = m_pLightManager->GetShadowLights(0);
    if (shadowLights.m_count < 1)
        return;

    planeFar.d += 0.05f;

    for (int i = 0; i < shadowLights.m_count; i++)
    {
        PartLight* pLight = shadowLights.m_pData[i];

        float fOverride = pLight->m_fShadowRadius;
        float fRadius = pLight->m_fLightRadius;
        float fRange = fOverride > 0.0f ? fOverride : fRadius;
        if (fRange < shadowlength)
            fRange = shadowlength;

        for (int j = 0; j < meshshadowbucket.m_count; j++)
        {
            if (!meshshadowbucket.m_pData[j]->WithinRange(pLight->m_vWorldPosition, fRange))
                continue;

// The mesh's own pose becomes the modelview for the projection it draws.
            ASLgl::glPushMatrix();
            float aMatrix[16];
            MakeMatrix4x4(aMatrix, meshshadowbucket.m_pData[j]->m_vLocalPosition,
                          meshshadowbucket.m_pData[j]->m_qLocalOrientation);
            ASLgl::glMultMatrixf(aMatrix);

// Both the clip plane and the light position follow it down into that frame.
            Plane planeLocal = planeFar;
            planeLocal.Transform(meshshadowbucket.m_pData[j]->m_vLocalPosition,
                                 meshshadowbucket.m_pData[j]->m_qLocalOrientation);

            const Quaternion& q = meshshadowbucket.m_pData[j]->m_qLocalOrientation;
#if KOTOR1
            Vector vLight = RotateVector(pLight->m_vWorldPosition
                                             - meshshadowbucket.m_pData[j]->m_vLocalPosition, -q);
#else
            Vector vLight = RotateVector(pLight->m_vWorldPosition
                                             - meshshadowbucket.m_pData[j]->m_vLocalPosition,
                                         Quaternion(-q.w, q.x, q.y, q.z));
#endif

            meshshadowbucket.m_pData[j]->Project(vLight, fRange, planeLocal, 0);
            ASLgl::glPopMatrix();
        }
    }
}

#if KOTOR1
extern int g_RenderSkip;
#endif
void Scene::RenderDynamicGeometry()
{
#if KOTOR1  // K1: a render-skip frame sets no stencil state
    if (!g_RenderSkip)
    {
#endif
    if (enableshadows)
    {
        int nClearValue = AurGetStencilClearValue();
        GLRender::Enable(Aurora::kCapStencilTest);
        ASLgl::glStencilMask(0xff);
        GLRender::SetStencilFunc(Aurora::kCmpAlways, nClearValue, 0xffffffff);
        GLRender::SetStencilOp(Aurora::kStencilKeep, Aurora::kStencilKeep, Aurora::kStencilReplace);
    }

    AurVertexProgramMakeCurrent();
#if KOTOR1
    }
#endif
    DoGobBuckets();
}

void ClearDistortBufferIfNeeded();

#if defined(K2_QOL_IMPROVEMENTS) && !KOTOR1
// Additive static meshes (light shafts, glows) write no depth, so they are drawn after
// the gobs.
static List<PartTriMesh*> s_listLateAdditiveMeshes;

static bool K2QolIsAdditiveMaterial(Material* pMaterial)
{
    return pMaterial->m_lstSrcBlend.m_count > 0 && pMaterial->m_lstDstBlend.m_count > 0 &&
           pMaterial->m_lstSrcBlend.m_pData[0] == Aurora::kBlendSrcAlpha &&
           pMaterial->m_lstDstBlend.m_pData[0] == Aurora::kBlendOne;
}
#endif

#if KOTOR1
extern unsigned int pressedGamepadButtons;
extern unsigned int pressedGamepadButtonsThisFrame;
extern int preferPerfToVisual;
// Two debug bits of pressedGamepadButtons, above the pad's own GAMEPAD_* bits (MacMain.cpp), which
// no button sets in the port: held, static geometry is hidden; pressed, the perf mode toggles.
enum
{
    kPadDebugHideStatic = 0x8000,
    kPadDebugTogglePerf = 0x10000,
};
bool IosShadowsEnabled();
#ifdef K2_QOL_K1_ANTIALIAS
bool IosAntiAliasOn();
#endif
// K1: debug keys hide static geometry and toggle the perf mode; no hologram or dream textures.
void Scene::RenderSinglePass()
{
    enablerenderstatic = (pressedGamepadButtons & kPadDebugHideStatic) == 0;
    if (pressedGamepadButtonsThisFrame & kPadDebugTogglePerf)
        preferPerfToVisual = preferPerfToVisual == 0;

    if (!g_RenderSkip)
    {
#ifdef K2_QOL_K1_ANTIALIAS
        // Anti-aliasing renders the scene into the pbuffer whatever the frame-buffer switch says;
        // the camera has already enlarged the viewport for it.
        if (GLRender::m_bSaturationEnabled || !preferPerfToVisual || IosAntiAliasOn())
#else
        if (GLRender::m_bSaturationEnabled || !preferPerfToVisual)
#endif
            FrameBufferModificationsStartIos(m_pCurrentRoom != 0);

        if (!preferPerfToVisual && AurNV2xLevelShadersAvailable())
        {
            Matrix mat = CurrentCamera->m_qOrientation.getmatrix();
            Vector vCam = CurrentCamera->m_vPosition;
            GLRender::SetVertexShaderConstant(92, mat.m_f[0], mat.m_f[3], mat.m_f[6], vCam.x);
            GLRender::SetVertexShaderConstant(91, mat.m_f[1], mat.m_f[4], mat.m_f[7], vCam.y);
            GLRender::SetVertexShaderConstant(90, mat.m_f[2], mat.m_f[5], mat.m_f[8], vCam.z);
            GLRender::SetVertexShaderConstant(89, 0.0f, 0.0f, 0.0f, 1.0f);
        }

        DoFog();

        GLRender::Enable(Aurora::kCapBlend);
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
        GLRender::Disable(Aurora::kCapStencilTest);

        unsigned int nColorArray = clientStateTranslationTable[kArrayColor];
        ASLgl::glDisableClientState(nColorArray);
        GLRender::DisableColorMaterial();

        if (!preferPerfToVisual)
        {
            Vector vZero;
            GLRender::SetLightModel(Homogenize(vZero, 1.0f));
        }

        ASLgl::glDepthMask(1);

        if (enablerenderstatic)
        {
            RenderStaticGeometry();
            GLRender::pfuncEnablePixelShaders(GLRender::kPSFixedFunction, 0, 0, 0, 0);
        }

        if (!preferPerfToVisual)
            GLRender::SetLightModel(Homogenize(m_ambient, 1.0f));
        ASLgl::glDisableClientState(nColorArray);
        GLRender::DisableColorMaterial();

        if (!preferPerfToVisual && enablerendertriggers)
        {
            SetShadowStencilBuffer();
            DoTriggerBucket();
            DoHighlightTriggers(false);
        }

        if (!preferPerfToVisual)
        {
            SetShadowStencilBuffer();
            if (m_pCurrentRoom && enableshadows && IosShadowsEnabled())
                RenderShadows(0, 1, 1, 0);
        }
    }

    if (enablerenderdynamic)
    {
        RenderDynamicGeometry();
        if (!g_RenderSkip)
            GLRender::pfuncEnablePixelShaders(GLRender::kPSFixedFunction, 0, 0, 0, 0);
    }

    if (!(preferPerfToVisual | g_RenderSkip))
    {
        RenderPersonalSpace(this);
        RenderPaths(this);

        if (enablerendertriggers)
        {
            SetShadowStencilBuffer();
            DoHighlightTriggers(true);
        }
    }

    DoEmitterBucket();
    if (g_RenderSkip)
        return;

    if (!preferPerfToVisual)
        GLRender::pfuncEnablePixelShaders(GLRender::kPSFixedFunction, 0, 0, 0, 0);
    DoLightSaberBucket();
    if (!preferPerfToVisual)
    {
        GLRender::pfuncEnablePixelShaders(GLRender::kPSFixedFunction, 0, 0, 0, 0);
        GLRender::RenderDrawableSquares();
        RenderMotionBlur();
        DoDistortionBuckets();
        GLRender::pfuncEnablePixelShaders(GLRender::kPSFixedFunction, 0, 0, 0, 0);
    }

    if (GLRender::m_bSaturationEnabled || !preferPerfToVisual)
    {
        GLRender::SetFilmNoiseTexture(m_pFilmNoiseTexture);
        GLRender::cm_pForceDistortionTexture = m_pDistortionTexture;
        if (!preferPerfToVisual)
        {
            if (m_pCurrentRoom)
                GLRender::FrameBufferModifications(g_listForcePushBucket.m_count > 0 ? GLRender::kFBPassAll
                                                                      : GLRender::kFBPassAllButForceDistortion);
            RenderLensFlares();
            if (m_pCurrentRoom)
            {
                GLRender::Disable(Aurora::kCapDepthTest);
                GLRender::Disable(Aurora::kCapLighting);
                GLRender::FrameBufferModifications2(GLRender::kFBPassAllButForceDistortion);
            }
        }
    }

#ifdef K2_QOL_K1_ANTIALIAS
    if (GLRender::m_bSaturationEnabled || !preferPerfToVisual || IosAntiAliasOn())
    {
        GLRender::SetFilmNoiseTexture(m_pFilmNoiseTexture);   // the present quad binds it
        FrameBufferModificationsEndIos(m_pCurrentRoom != 0);
    }
#else
    if (GLRender::m_bSaturationEnabled || !preferPerfToVisual)
        FrameBufferModificationsEndIos(m_pCurrentRoom != 0);
#endif

    if (preferPerfToVisual)
    {
        GLRender::pfuncEnablePixelShaders(GLRender::kPSFixedFunction, 0, 0, 0, 0);
        ASLgl::glDepthMask(1);
        ASLgl::glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }

    GLRender::Disable(Aurora::kCapStencilTest);
    GLRender::Disable(Aurora::kCapLighting);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapBlend);
    GLRender::Disable(Aurora::kCapFog);

    GLRender::DisableClientState(Aurora::kClientStateNormal);
    GLRender::DisableClientState(Aurora::kClientStateTexCoord);
    GLRender::DisableClientState(Aurora::kClientStateVertex);
    GLRender::DisableClientState(Aurora::kClientStateColor);
    GLRender::DisableColorMaterial();
}
#else
void Scene::RenderSinglePass()
{
    FrameBufferModificationsStartIos(m_pCurrentRoom != 0);

    GLRender::SetDirtyTexture(0, m_pDirtyTexture);
    GLRender::SetDirtyTexture(1, m_pDirtyTexture2);
    GLRender::SetDirtyTexture(2, m_pDirtyTexture3);
    GLRender::m_pHologramFilterTexture = m_pHologramTexture;

    if (AurNV2xLevelShadersAvailable())
    {
        Matrix mat = CurrentCamera->m_qOrientation.getmatrix();
        Vector vCam = CurrentCamera->m_vPosition;
        GLRender::SetVertexShaderConstant(92, mat.m_f[0], mat.m_f[3], mat.m_f[6], vCam.x);
        GLRender::SetVertexShaderConstant(91, mat.m_f[1], mat.m_f[4], mat.m_f[7], vCam.y);
        GLRender::SetVertexShaderConstant(90, mat.m_f[2], mat.m_f[5], mat.m_f[8], vCam.z);
        GLRender::SetVertexShaderConstant(89, 0.0f, 0.0f, 0.0f, 1.0f);
    }

    DoFog();

    GLRender::Enable(Aurora::kCapBlend);
    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
    GLRender::Disable(Aurora::kCapStencilTest);

    GLRender::DisableClientState(Aurora::kClientStateColor);
    GLRender::DisableColorMaterial();

    GLRender::SetLightModel(Homogenize(Vector(), 1.0f));

    RenderInterface::SetDepthMask(1);

    if (enablerenderstatic)
    {
        RenderStaticGeometry();
        GLRender::pfuncEnablePixelShaders(GLRender::kPSFixedFunction, 0, 0, 0, 0);
    }

    GLRender::SetLightModel(Homogenize(m_ambient, 1.0f));
    GLRender::DisableClientState(Aurora::kClientStateColor);
    GLRender::DisableColorMaterial();

    if (enablerendertriggers)
    {
        SetShadowStencilBuffer();
        DoTriggerBucket();
        DoHighlightTriggers(false);
    }

    SetShadowStencilBuffer();

    if (m_pCurrentRoom && enableshadows)
    {
        GLRender::pfuncEnablePixelShaders(GLRender::kPSDefault, 0, 0, 0, 0);
        RenderShadows(0, 1, 1, 0);
        GLRender::pfuncEnablePixelShaders(GLRender::kPSFixedFunction, 0, 0, 0, 0);
    }

    if (enablerenderdynamic)
    {
        RenderDynamicGeometry();
        DoHologramGobBuckets();
    }

#if defined(K2_QOL_IMPROVEMENTS) && !KOTOR1
    if (s_listLateAdditiveMeshes.m_count != 0)
    {
        float vWhite[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        float vBlack[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        unsigned int nFace = facingTranslationTable[kFaceFront];

        AurVertexProgramMakeCurrent();
        m_pLightManager->EnableLightsDynamicOnly();
        ASLgl::glMaterialfv(nFace, materialTranslationTable[kMaterialAmbient], vWhite);
        ASLgl::glMaterialfv(nFace, materialTranslationTable[kMaterialDiffuse], vWhite);
        ASLgl::glMaterialfv(nFace, materialTranslationTable[kMaterialEmission], vBlack);
        GLRender::SetLightModel(Homogenize(Vector(), 1.0f));
        GLRender::DisableClientState(Aurora::kClientStateColor);
        GLRender::DisableColorMaterial();
        GLRender::Enable(Aurora::kCapBlend);
        PartTriMesh::m_bMaterialHasChanged = true;

        for (int i = 0; i < s_listLateAdditiveMeshes.m_count; i++)
        {
            PartTriMesh* pPart = s_listLateAdditiveMeshes.m_pData[i];
            pPart->m_pMaterial->BindTexture0();
            pPart->m_pMaterial->SetBlendingMode(0);
            pPart->Mat();
            pPart->Draw(false);
        }
        s_listLateAdditiveMeshes.m_count = 0;

        GLRender::pfuncEnablePixelShaders(GLRender::kPSFixedFunction, 0, 0, 0, 0);
        GLRender::SetLightModel(Homogenize(m_ambient, 1.0f));
        RenderInterface::SetDepthMask(1);
        PartTriMesh::m_bMaterialHasChanged = true;
    }
#endif

    ClearDistortBufferIfNeeded();

    DoDistortionBuckets();

    RenderPersonalSpace(this);
    RenderPaths(this);

    if (enablerendertriggers)
    {
        SetShadowStencilBuffer();
        DoHighlightTriggers(true);
    }

    GLRender::pfuncEnablePixelShaders(GLRender::kPSFixedFunction, 0, 0, 0, 0);
    DoEmitterBucket();
    GLRender::pfuncEnablePixelShaders(GLRender::kPSFixedFunction, 0, 0, 0, 0);
    DoLightSaberBucket();
    RenderMotionBlur();
    GLRender::pfuncEnablePixelShaders(GLRender::kPSFixedFunction, 0, 0, 0, 0);

    GLRender::SetFilmNoiseTexture(m_pFilmNoiseTexture);
    GLRender::cm_pForceDistortionTexture = m_pDistortionTexture;
    GLRender::m_pDreamDistortTexture = m_pLensFlareTexture;
    GLRender::SetAlphaFunc(Aurora::kCmpGreater, 0.0f);

    RenderLensFlares();

    if (m_pCurrentRoom)
    {
        GLRender::Disable(Aurora::kCapDepthTest);
        GLRender::Disable(Aurora::kCapLighting);
        GLRender::FrameBufferModifications2(GLRender::kFBPassAllButForceDistortion);
    }

    FrameBufferModificationsEndIos(m_pCurrentRoom != 0);

    GLRender::Disable(Aurora::kCapStencilTest);
    GLRender::Disable(Aurora::kCapLighting);
    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapBlend);
    GLRender::Disable(Aurora::kCapFog);

    GLRender::DisableClientState(Aurora::kClientStateNormal);
    GLRender::DisableClientState(Aurora::kClientStateTexCoord);
    GLRender::DisableClientState(Aurora::kClientStateVertex);
    GLRender::DisableClientState(Aurora::kClientStateColor);
    GLRender::DisableColorMaterial();
}
#endif

void Scene::RenderStaticGeometry()
{
    AurVertexProgramMakeCurrent();
    m_pLightManager->EnableLightsDynamicOnly();

    float vWhite[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float vBlack[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

#if KOTOR1  // K1 sends the materials to GL_FRONT_AND_BACK
    unsigned int nFace = GL_FRONT_AND_BACK;
#else
    unsigned int nFace = facingTranslationTable[kFaceFront];
#endif

    ASLgl::glMaterialfv(nFace, materialTranslationTable[kMaterialAmbient], vWhite);
    ASLgl::glMaterialfv(nFace, materialTranslationTable[kMaterialDiffuse], vWhite);
    ASLgl::glMaterialfv(nFace, materialTranslationTable[kMaterialEmission], vBlack);

    DoMeshBuckets();
    DoBinBucket();
}

void Scene::RenderOnlyGobs()
{
    m_pLightManager->EnableLights();
    GLRender::Disable(Aurora::kCapAlphaTest);
    AurVertexProgramMakeCurrent();

    for (int i = 0; i < Objects().m_count; i++)
    {
        m_pLightManager->GetNearestLights(this, Objects().m_pData[i]->m_vPosition, 2.0f, 0);
        Objects().m_pData[i]->Render(false);
    }
}

void Scene::RenderMotionBlur()
{
    GLRender::EnableClientState(Aurora::kClientStateTexCoord);
    GLRender::EnableClientState(Aurora::kClientStateVertex);
    GLRender::EnableClientState(Aurora::kClientStateNormal);
    GLRender::EnableClientState(Aurora::kClientStateColor);

    GLRender::Enable(Aurora::kCapBlend);
    GLRender::Enable(Aurora::kCapDepthTest);

    RenderInterface::SetDepthMask(0);

    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
    GLRender::Enable(Aurora::kCapLighting);

    DoBlurBuckets();

    RenderInterface::SetDepthMask(1);
}

#if !KOTOR1  // K2 only
void Scene::DoHologramGobBuckets()
{
    if (g_listHologramBucket.m_count == 0)
        return;

    countvisibilityculls = 0;
    Vector vPos;

    qsort(g_listHologramBucket.m_pData, g_listHologramBucket.m_count, sizeof(Gob*), GobOrderCmp);

    numgobs = g_listHologramBucket.m_count;
    m_pLightManager->m_lastUploadedLights.m_count = 0;
    m_pLightManager->m_lastUploadedWeights.m_count = 0;

    for (int i = g_listHologramBucket.m_count; i--; )
    {
        AurRenderCallback();

        Gob* pGob = g_listHologramBucket.m_pData[i];

        if (interleaveanimations && g_bEnableInterleavedAnimations)
        {
            if (i >= g_listHologramBucket.m_count)
                continue;
            pGob->Animate(DeltaT);
            if (i >= g_listHologramBucket.m_count)
                continue;
        }

        if (!pGob->VisibilityCheck())
            continue;

        if (rendertransparentcharacters && pGob->m_pModel && (pGob->m_pModel->m_nFlags & Model::kModelCharacter))
            GLRender::Disable(Aurora::kCapDepthTest);

        vPos = pGob->m_vPosition;

        Part* pDummy = 0;
        if (pGob->IsInCutscene())
            pDummy = FindPart(pGob->m_pPart, const_cast<char*>("cutscenedummy"));

        if (pDummy)
        {
            static Vector worldPos;
            static Quaternion worldOrient;
            pDummy->GetWorld(worldPos, worldOrient);
            m_pLightManager->SetupLightsForThisGob(worldPos, pGob->m_fMinSphereRadius);
            vPos = worldPos;
        }
        else
        {
            m_pLightManager->SetupLightsForThisGob(pGob->m_vPosition, pGob->m_fMinSphereRadius);
        }

        m_pLightManager->PrioritizeLights(vPos, false);

        if (pGob->m_pLightContainer)
            m_pLightManager->EnableHighestPriorityLightsFade(3, pGob->m_pLightContainer);
        else
            m_pLightManager->EnableHighestPriorityLights(3);

        if (showlightpriorityinfo && !_strnicmp(pGob->m_szName, "c_focusgob", 10))
            m_pLightManager->DebugPrintLightInfo(3, pGob->m_vPosition);

        GLRender::Enable(Aurora::kCapBlend);
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);

        if (dogobrender)
            pGob->Render(false);

        if (rendertransparentcharacters && (pGob->m_pModel->m_nFlags & Model::kModelCharacter))
            GLRender::Enable(Aurora::kCapDepthTest);
    }
}
#endif  // !KOTOR1

void Scene::DoBlurBuckets()
{
    numgobs = blurbucket.m_count;

    for (int i = 0; i < blurbucket.m_count; i++)
    {
        AurRenderCallback();
        m_pLightManager->SetupLightsForThisGob(blurbucket.m_pData[i]->m_vPosition,
                                               blurbucket.m_pData[i]->m_fMinSphereRadius);
        m_pLightManager->PrioritizeLights(blurbucket.m_pData[i]->m_vPosition, false);
        m_pLightManager->EnableHighestPriorityLights(3);
        blurbucket.m_pData[i]->RenderBlur();
    }
}

void Scene::DoLightSaberBucket()
{
    GLRender::Enable(Aurora::kCapBlend);

    for (int i = 0; i < g_listLightsaberBucket.m_count; i++)
    {
        Gob* pGob = g_listLightsaberBucket.m_pData[i];

        m_pLightManager->SetupLightsForThisGob(pGob->m_vPosition, pGob->m_fMinSphereRadius);
        m_pLightManager->PrioritizeLights(pGob->m_vPosition, false);

        if (pGob->m_pLightContainer)
            m_pLightManager->EnableHighestPriorityLightsFade(3, pGob->m_pLightContainer);
        else
            m_pLightManager->EnableHighestPriorityLights(3);

        pGob->Render(false);
    }
}

bool Gob::VisibilityCheck()
{
    if (!enablevisibilitytest)
        return true;
    if (m_pModel && (m_pModel->m_nFlags & Model::kModelAmbient))
        return true;
    if (IsAlwaysRendering())
        return true;

    float fDistance = magnitude(CurrentCamera->m_vPosition - m_vPosition);
    if (m_fMinSphereRadius * 2.0f / fDistance > visibilitycutoffratio)
    {
        m_fVisibilityCheckTimer = 0.0f;
        m_nVisible = 1;
        return true;
    }

    m_fVisibilityCheckTimer += DeltaT;
    if (m_fVisibilityCheckTimer > visibilitycheckdelay)
    {
        m_fVisibilityCheckTimer = 0.0f;

        CHitInfo hitInfo(0, CHitInfo::kMaterialAllButTrigger, 0);
        hitInfo.m_bSkipObjects = 1;
        Vector vHit;

        Vector vTarget = RotateVector(m_vMinSphereCenter, m_qOrientation);
        vTarget.x += m_vPosition.x;
        vTarget.y += m_vPosition.y;
        vTarget.z += m_vPosition.z;

        m_nVisible = m_pScene->HitCheck(CurrentCamera->m_vPosition, vTarget, &vHit, &hitInfo) == 0;
    }

    if (!m_nVisible)
        countvisibilityculls++;

    return m_nVisible != 0;
}

void Scene::DoGobBuckets()
{
#if KOTOR1  // K1: one pass, no force sight; g_RenderSkip gates the light reset and the draw
    countvisibilityculls = 0;
    Vector vPos;

    qsort(gobbucket.m_pData, gobbucket.m_count, sizeof(Gob*), GobOrderCmp);

    numgobs = gobbucket.m_count;
    if (!g_RenderSkip)
    {
        m_pLightManager->m_lastUploadedLights.m_count = 0;
        m_pLightManager->m_lastUploadedWeights.m_count = 0;
    }

    for (int i = gobbucket.m_count; i != 0; i--)
    {
        AurRenderCallback();

        Gob* pGob = gobbucket.m_pData[i - 1];

        if (interleaveanimations && g_bEnableInterleavedAnimations)
        {
            if (i > gobbucket.m_count)
                continue;
            pGob->Animate(DeltaT);
            if (i > gobbucket.m_count)
                continue;
        }

        if (g_RenderSkip)
            continue;

        if (!pGob->VisibilityCheck())
            continue;

        if (rendertransparentcharacters && pGob->m_pModel && (pGob->m_pModel->m_nFlags & Model::kModelCharacter))
            GLRender::Disable(Aurora::kCapDepthTest);

        vPos = pGob->m_vPosition;

        Part* pDummy = 0;
        if (pGob->IsInCutscene())
            pDummy = FindPart(pGob->m_pPart, const_cast<char*>("cutscenedummy"));

        if (pDummy)
        {
            static Vector worldPos;
            static Quaternion worldOrient;
            pDummy->GetWorld(worldPos, worldOrient);
            m_pLightManager->SetupLightsForThisGob(worldPos, pGob->m_fMinSphereRadius);
            vPos = worldPos;
        }
        else
        {
            m_pLightManager->SetupLightsForThisGob(pGob->m_vPosition,
                                                   pGob->m_fMinSphereRadius);
        }

        m_pLightManager->PrioritizeLights(vPos, false);

        if (pGob->m_pLightContainer)
            m_pLightManager->EnableHighestPriorityLightsFade(3, pGob->m_pLightContainer);
        else
            m_pLightManager->EnableHighestPriorityLights(3);

        if (showlightpriorityinfo && !_strnicmp(pGob->m_szName, "c_focusgob", 10))
            m_pLightManager->DebugPrintLightInfo(3, pGob->m_vPosition);

        GLRender::Enable(Aurora::kCapBlend);
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);

        if (dogobrender)
            pGob->Render(false);

        if (rendertransparentcharacters && (pGob->m_pModel->m_nFlags & Model::kModelCharacter))
            GLRender::Enable(Aurora::kCapDepthTest);
    }

    if (interleaveanimations && g_bEnableInterleavedAnimations)
    {
        for (int i = Objects().m_count; i != 0; i--)
        {
            if (i > Objects().m_count)
                continue;
            Objects().m_pData[i - 1]->Animate(DeltaT);
        }
    }
#else
    countvisibilityculls = 0;
    Vector vPos;

    qsort(gobbucket.m_pData, gobbucket.m_count, sizeof(Gob*), GobOrderCmp);

    numgobs = gobbucket.m_count;
    m_pLightManager->m_lastUploadedLights.m_count = 0;
    m_pLightManager->m_lastUploadedWeights.m_count = 0;

    int nPasses = g_ForceSight ? 2 : 1;

    for (int nPass = 0; nPass < nPasses; nPass++)
    {
        for (int i = gobbucket.m_count; i != 0; i--)
        {
            if (interleaveanimations && g_bEnableInterleavedAnimations)
            {
                if (i > gobbucket.m_count)
                    continue;
                gobbucket.m_pData[i - 1]->Animate(DeltaT);
            }
        }

        for (int i = gobbucket.m_count; i != 0; i--)
        {
            AurRenderCallback();

            Gob* pGob = gobbucket.m_pData[i - 1];

            if (g_ForceSight)
            {
                int nSight = pGob->m_pModel->m_nFlags & (Model::kModelCharacter | Model::kModelDoor);
                if (nPass == 0)
                {
                    if (nSight)
                        continue;
                }
                else if (!nSight)
                {
                    continue;
                }
            }

            if (!pGob->VisibilityCheck())
                continue;

            if (rendertransparentcharacters && pGob->m_pModel && (pGob->m_pModel->m_nFlags & Model::kModelCharacter))
                GLRender::Disable(Aurora::kCapDepthTest);

            vPos = pGob->m_vPosition;

            Part* pDummy = 0;
            if (pGob->IsInCutscene())
                pDummy = FindPart(pGob->m_pPart, const_cast<char*>("cutscenedummy"));

            if (pDummy)
            {
                static Vector worldPos;
                static Quaternion worldOrient;
                pDummy->GetWorld(worldPos, worldOrient);
                m_pLightManager->SetupLightsForThisGob(worldPos, pGob->m_fMinSphereRadius);
                vPos = worldPos;
            }
            else
            {
                m_pLightManager->SetupLightsForThisGob(pGob->m_vPosition,
                                                       pGob->m_fMinSphereRadius);
            }

            m_pLightManager->PrioritizeLights(vPos, false);

            if (pGob->m_pLightContainer)
                m_pLightManager->EnableHighestPriorityLightsFade(3, pGob->m_pLightContainer);
            else
                m_pLightManager->EnableHighestPriorityLights(3);

            if (showlightpriorityinfo && !_strnicmp(pGob->m_szName, "c_focusgob", 10))
                m_pLightManager->DebugPrintLightInfo(3, pGob->m_vPosition);

            GLRender::Enable(Aurora::kCapBlend);
            GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);

            if (dogobrender && pGob->IsRenderingEnabled())
                pGob->Render(false);

            if (rendertransparentcharacters && (pGob->m_pModel->m_nFlags & Model::kModelCharacter))
                GLRender::Enable(Aurora::kCapDepthTest);
        }
    }

    if (interleaveanimations && g_bEnableInterleavedAnimations)
    {
        for (int i = Objects().m_count; i != 0; i--)
        {
            if (i > Objects().m_count)
                continue;
            Objects().m_pData[i - 1]->Animate(DeltaT);
        }
    }
#endif
}

#if KOTOR1
#ifdef K2_QOL_K1_DISTORTION
extern bool g_bK1ForceDistortion;   // aurglcore.cpp: FrameBufferModificationsEndIos reads the stamp
#endif
// K1: no distortion buffer; force-push gobs only stamp stencil bit 1, colour writes off.
void Scene::DoDistortionBuckets()
{
#ifdef K2_QOL_K1_DISTORTION
    g_bK1ForceDistortion = g_listForcePushBucket.m_count > 0;
#endif
    GLRender::Enable(Aurora::kCapStencilTest);
    ASLgl::glColorMask(0, 0, 0, 0);
    GLRender::SetStencilOp(Aurora::kStencilKeep, Aurora::kStencilKeep, Aurora::kStencilReplace);
    ASLgl::glStencilMask(1);
    GLRender::SetStencilFunc(Aurora::kCmpAlways, 1, 0xff);

    for (int i = 0; i < g_listForcePushBucket.m_count; i++)
        g_listForcePushBucket.m_pData[i]->Render(false);

    ASLgl::glColorMask(1, 1, 1, 1);
    ASLgl::glStencilMask(0xff);
    GLRender::Disable(Aurora::kCapStencilTest);
}
#else
void Scene::DoDistortionBuckets()
{
    if (g_listForcePushBucket.m_count == 0)
        return;

    SetDistortBuffer(true);

    RenderInterface::SetDepthMask(0);
    g_useDistortionOutput = true;

    for (int i = 0; i < g_listForcePushBucket.m_count; i++)
        g_listForcePushBucket.m_pData[i]->Render(false);

    g_useDistortionOutput = false;
    RenderInterface::SetDepthMask(1);
    IosSetDistortBuffer(false);
}
#endif

void Scene::DoTriggerBucket()
{
    if (!enablerender)
        return;

    GLRender::Disable(Aurora::kCapBlend);
    GLRender::Disable(Aurora::kCapTexture2D);
    GLRender::Disable(Aurora::kCapLighting);

    for (int i = 0; i < triggerbucket.m_count; i++)
    {
        if (!triggerbucket.m_pData[i]->m_bHidden)
            triggerbucket.m_pData[i]->Draw(false);
    }

    GLRender::Enable(Aurora::kCapBlend);
    GLRender::Enable(Aurora::kCapTexture2D);
    GLRender::Enable(Aurora::kCapLighting);
}

static Vector ViewVolumeNormal(int nIndex)
{
    const Plane& q = CurrentCamera->m_viewState.m_aViewVolumes[nIndex];
    Vector v = { q.a, q.b, q.c };
    return v;
}

void Scene::DoHighlightTriggers(bool bHighlightGroup)
{
    List<PartTrigger*> highlights;
    PartTrigger* pTrigger;

    int nStencilBits = AurGetAvailableStencilBits();
    if (nStencilBits == 0)
        return;

    m_fHighlightPhase += (DeltaT + DeltaT) * 3.14159274f;

    if (!enablerender)
        return;

    GLRender::Disable(Aurora::kCapBlend);
    GLRender::Disable(Aurora::kCapTexture2D);
    GLRender::Disable(Aurora::kCapLighting);
    GLRender::Disable(Aurora::kCapAlphaTest);

    for (int i = 0; i < triggerbucket.m_count; i++)
    {
        pTrigger = triggerbucket.m_pData[i];
        if (!pTrigger->m_bHidden || (pTrigger->m_cHighlightGroup != 0) != bHighlightGroup)
            continue;

        highlights.Add(pTrigger);
    }

    for (int i = 0; i < highlights.m_count; i++)
    {
        SetShadowStencilBuffer();
        ASLgl::glStencilMask(0xff);
        GLRender::Enable(Aurora::kCapStencilTest);

        RenderInterface::SetDepthMask(0);

        ASLgl::glColorMask(0, 0, 0, 0);
        GLRender::SetStencilFunc(Aurora::kCmpAlways, 0xf, 0xf);
        GLRender::SetStencilOp(Aurora::kStencilKeep, Aurora::kStencilIncr, Aurora::kStencilKeep);
        GLRender::SetFrontFace(Aurora::kCullCW);
        highlights.m_pData[i]->DrawVolume();

        GLRender::SetStencilOp(Aurora::kStencilKeep, Aurora::kStencilDecr, Aurora::kStencilKeep);
        GLRender::SetFrontFace(Aurora::kCullCCW);
        highlights.m_pData[i]->DrawVolume();

        GLRender::SetFrontFace(Aurora::kCullCCW);
        ASLgl::glColorMask(1, 1, 1, 1);

        List<Vector> corners;
        corners.Add(normalize(cross(ViewVolumeNormal(5), ViewVolumeNormal(2))));
        corners.Add(normalize(cross(ViewVolumeNormal(3), ViewVolumeNormal(5))));
        corners.Add(normalize(cross(ViewVolumeNormal(4), ViewVolumeNormal(3))));
        corners.Add(normalize(cross(ViewVolumeNormal(2), ViewVolumeNormal(4))));

        ASLgl::glPushMatrix();
        ASLgl::glTranslatef(CurrentCamera->m_vPosition.x,
                            CurrentCamera->m_vPosition.y,
                            CurrentCamera->m_vPosition.z);
        GLRender::Enable(Aurora::kCapBlend);
        GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
        GLRender::SetStencilFunc(Aurora::kCmpLEqual, 1 << (nStencilBits - 1), 0xffffffff);
        GLRender::SetStencilOp(Aurora::kStencilKeep, Aurora::kStencilKeep, Aurora::kStencilKeep);

        pTrigger = highlights.m_pData[i];
        if (pTrigger->m_bPulse)
        {
            float fAlpha = pTrigger->m_color[3] * 0.5f
                           * (sinf(pTrigger->m_fPulseRate * m_fHighlightPhase) + 1.0f);
            ASLgl::glColor4f(pTrigger->m_color[0], pTrigger->m_color[1], pTrigger->m_color[2],
                             fAlpha);
        }
        else
        {
            ASLgl::glColor4fv(pTrigger->m_color);
        }

        GLRender::DrawFromVerts(Aurora::kPrimQuads, corners);

        ASLgl::glPopMatrix();
        GLRender::Disable(Aurora::kCapStencilTest);

        RenderInterface::SetDepthMask(1);
    }

    GLRender::Enable(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapBlend);
    GLRender::Enable(Aurora::kCapTexture2D);
    GLRender::Enable(Aurora::kCapLighting);
}

template <class T> static inline const T& min(const T& a, const T& b) { return b < a ? b : a; }
template <class T> static inline const T& max(const T& a, const T& b) { return a < b ? b : a; }

// A flare vertex colour is packed 0xAABBGGRR; the alpha byte is always opaque.
enum : unsigned int
{
    kFlareAlphaOpaque = 0xff000000,
};

static int FlareColourByte(float f)
{
    if (f < 0.0f)
        return 0;
    if (f > 1.0f)
        f = 1.0f;
    return static_cast<int>(f * 255.0f);
}

#if KOTOR1
bool IosBloomEnabled();
// K1: the bloom cap on a flare's fade-in.
__attribute__((visibility("hidden"))) float g_fBloomFlareFadeMax = 0.5f;
#endif
void Scene::RenderLensFlares()
{
    if (renderlensflares == 0)
        return;

    List<PartLight*> flareLights = m_pLightManager->GetLensFlareLights();

    static unsigned int renderListToken = GLRender::RequestPool(
        static_cast<Aurora::AuroraBufferRequestType>(0), 4 * sizeof(RenderVertexVTC), 0x42, 0x1011,
        &renderListToken);

    GLRender::Disable(Aurora::kCapLighting);
    GLRender::Disable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapBlend);
    GLRender::SetBlendFunc(Aurora::kBlendOne, Aurora::kBlendOne);
    GLRender::Enable(Aurora::kCapTexture2D);

    if (m_bFogEnabled)
        GLRender::Disable(Aurora::kCapFog);

    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();

    GLRender::SetMatrixMode(Aurora::kMatrixModelView2);
    ASLgl::glPushMatrix();
    ASLgl::glLoadIdentity();
    ASLgl::glTranslatef(0.0f, 0.0f, -2.0f);

#if !KOTOR1  // K1 has no force-sight tint
    if (g_ForceSight)
    {
        GLRender::pfuncEnablePixelShaders(GLRender::kPSDesat, 0, 0, 0, 0);
        float aTint[4] = { 0.0f, 0.0f, 0.0f, 0.75f };
        ASLgl::glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 1, aTint);
    }
#endif

    for (int i = 0; i < flareLights.m_count; i++)
    {
        PartLight* pLight = flareLights.m_pData[i];

        if (magnitude(pLight->m_vWorldPosition - CurrentCamera->m_vPosition)
                > static_cast<MdlNodeLight*>(pLight->m_pNode)->m_fFlareRange)
            continue;

        if (hitcheckflare)
        {
            static Vector impact;

            CHitInfo* pHitInfo = new CHitInfo(0, CHitInfo::kMaterialAllButTrigger, 0);
            pHitInfo->m_bHitBoundingBoxes = 0;
            pHitInfo->m_bIgnoreStartInside = 1;

            int nBlocked;
            if (reverseflarehit)
                nBlocked = HitCheck(CurrentCamera->m_vPosition, pLight->m_vWorldPosition,
                                    &impact, pHitInfo);
            else
                nBlocked = HitCheck(pLight->m_vWorldPosition, CurrentCamera->m_vPosition,
                                    &impact, pHitInfo);

            delete pHitInfo;

            if (nBlocked)
            {
                pLight->m_fFlareFade = max(pLight->m_fFlareFade - DeltaT * flarespeed, 0.0f);

                if (pLight->m_fFlareFade == 0.0f)
                    continue;
            }
#if KOTOR1  // K1: with bloom on, a flare fades in only to half strength
            else if (IosBloomEnabled())
            {
                pLight->m_fFlareFade = min(pLight->m_fFlareFade + DeltaT * flarespeed,
                                           g_fBloomFlareFadeMax);
            }
#endif
            else
            {
                pLight->m_fFlareFade = min(pLight->m_fFlareFade + DeltaT * flarespeed, 1.0f);
            }
        }

        MdlNodeLight* pNode = static_cast<MdlNodeLight*>(pLight->m_pNode);

        const Quaternion& qCamera = CurrentCamera->m_qOrientation;
#if KOTOR1
        Vector vDir = RotateVector(pLight->m_vWorldPosition - CurrentCamera->m_vPosition, -qCamera);
#else
        Vector vDir = RotateVector(pLight->m_vWorldPosition - CurrentCamera->m_vPosition,
                                   Quaternion(-qCamera.w, qCamera.x, qCamera.y, qCamera.z));
#endif

        float fDist = magnitude(vDir);
        if (fDist != 0.0f)
            vDir = vDir * (1.0f / fDist);

        Vector vPlaneNormal = { 0.0f, 0.0f, -1.0f };
        Vector vEye = { 0.0f, 0.0f, 0.0f };

        float fFacing = vDir.x * vPlaneNormal.x + vDir.y * vPlaneNormal.y + vDir.z * vPlaneNormal.z;
        if (fFacing < 0.0f)
            fFacing = 0.0f;
        fFacing = fFacing * fFacing;

        vDir = -planelineintersection(vPlaneNormal, 2.0f, vEye, vDir);

        for (int j = 0; j < pNode->FlareCount(); j++)
        {
            pNode->FlareTextures()[j]->Bind();

            float fRange = pNode->m_fFlareRange;
            Vector vColour = (pLight->m_vLightColour + pNode->FlareColourShifts()[j])
                             * pLight->m_fFlareFade * fFacing * (fRange - fDist);
            if (fRange != 0.0f)
                vColour = vColour * (1.0f / fRange);

            float fSize = (fRange - fDist) * pNode->FlareSizes()[j] / fRange * lensflaresize;

            ASLgl::glPushMatrix();
            ASLgl::glTranslatef(pNode->FlarePositions()[j] * vDir.x,
                                pNode->FlarePositions()[j] * vDir.y, 0.0f);

            static unsigned int renderListToken = GLRender::RequestPool(
                static_cast<Aurora::AuroraBufferRequestType>(0), 4 * sizeof(RenderVertexVTC), 0x42, 0x1011,
                &renderListToken);

            RenderVertexVTC* paVerts = static_cast<RenderVertexVTC*>(GLRender::LockPool(renderListToken, 0, 0, 1));

            unsigned int nColour = kFlareAlphaOpaque | FlareColourByte(vColour.x)
                                   | (FlareColourByte(vColour.y) << 8)
                                   | (FlareColourByte(vColour.z) << 16);

            for (int k = 0; k < 4; k++)
            {
                paVerts[k].color = nColour;
                paVerts[k].u = (k == 0 || k == 3) ? 0.0f : 1.0f;
                paVerts[k].v = (k < 2) ? 0.0f : 1.0f;
                paVerts[k].pos[0] = (k == 0 || k == 3) ? -fSize : fSize;
                paVerts[k].pos[1] = (k < 2) ? -fSize : fSize;
                paVerts[k].pos[2] = 0.0f;
            }

            GLRender::UnlockPool(renderListToken);
            GLRender::RenderObjectsVTCQuad(renderListToken, 4);
            ASLgl::glPopMatrix();
        }
    }

#if !KOTOR1
    if (g_ForceSight)
        GLRender::pfuncEnablePixelShaders(GLRender::kPSDesatOff, 0, 0, 0, 0);
#endif

    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);
    GLRender::Enable(Aurora::kCapDepthTest);
    GLRender::Enable(Aurora::kCapLighting);

    if (m_bFogEnabled)
        GLRender::Enable(Aurora::kCapFog);

    GLRender::SetMatrixMode(Aurora::kMatrixModelView2);
    ASLgl::glPopMatrix();
    GLRender::SetMatrixMode(Aurora::kMatrixModelView);
    ASLgl::glPopMatrix();
}

void Scene::RenderFogOfWarBoxes()
{
    if (m_fogOfWarRooms.m_count)
    {
        GLRender::Disable(Aurora::kCapLighting);
        GLRender::Disable(Aurora::kCapTexture2D);

        for (int i = 0; i < m_fogOfWarRooms.m_count; i++)
            GLRender::RenderBox(m_fogOfWarRooms.m_pData[i]->m_bboxMin,
                                m_fogOfWarRooms.m_pData[i]->m_bboxMax);
    }

    if (m_fogOfWarRooms.m_count)
    {
        GLRender::Enable(Aurora::kCapLighting);
        GLRender::Enable(Aurora::kCapTexture2D);
    }
}

#if KOTOR1
bool IosAntiAliasOn();
extern float IOS_FOG_CLIPDIST;
#endif
void Scene::DoFog()
{
    if (forcefogtest)
    {
        m_bFogEnabled = 1;
        GLRender::Enable(Aurora::kCapFog);
        GLRender::SetFogMode(m_nFogMode == 0 ? Aurora::kFogLinear
                                 : (m_nFogMode == 1 ? Aurora::kFogExp : Aurora::kFogExp2));

        Vector vRed;
        vRed.x = 1.0f;
        vRed.y = 0.0f;
        vRed.z = 0.0f;
        GLRender::SetFogModev(Aurora::kCapFogColor, vRed);
        m_vFogColor.x = 1.0f;
        m_vFogColor.y = 0.0f;
        m_vFogColor.z = 0.0f;

        GLRender::SetFogModef(Aurora::kCapFogStart, 0.1f);
        m_fFogStart = 0.0f;
        m_fFogEnd = 20.0f;
        m_fFogDensity = 0.1f;
        GLRender::SetFogModef(Aurora::kCapFogEnd, 20.0f);
        GLRender::SetFogModef(Aurora::kCapFogDensity, 0.1f);
        return;
    }

#if KOTOR1  // K1: a port table of module fog colours that switch fog off, plus the far clip it publishes
    // The fog colour channels the tables below test, as the bits of the float n / 255 (the area
    // stores its fog colour as bytes): kFogByteN is channel value N.
    enum
    {
        kFogByte5 = 0x3ca0a0a1,
        kFogByte9 = 0x3d109091,
        kFogByte17 = 0x3d888889,
        kFogByte21 = 0x3da8a8a9,
        kFogByte22 = 0x3db0b0b1,
        kFogByte23 = 0x3db8b8b9,
        kFogByte26 = 0x3dd0d0d1,
        kFogByte29 = 0x3de8e8e9,
        kFogByte30 = 0x3df0f0f1,
        kFogByte31 = 0x3df8f8f9,
        kFogByte50 = 0x3e48c8c9,
        kFogByte52 = 0x3e50d0d1,
        kFogByte58 = 0x3e68e8e9,
        kFogByte73 = 0x3e929293,
        kFogByte75 = 0x3e969697,
        kFogByte103 = 0x3ecececf,
        kFogByte112 = 0x3ee0e0e1,
        kFogByte140 = 0x3f0c8c8d,
        kFogByte142 = 0x3f0e8e8f,
        kFogByte143 = 0x3f0f8f90,
        kFogByte146 = 0x3f129293,
        kFogByte147 = 0x3f139394,
        kFogByte150 = 0x3f169697,
        kFogByte160 = 0x3f20a0a1,
        kFogByte169 = 0x3f29a9aa,
        kFogByte171 = 0x3f2babac,
        kFogByte177 = 0x3f31b1b2,
        kFogByte180 = 0x3f34b4b5,
        kFogByte190 = 0x3f3ebebf,
        kFogByte191 = 0x3f3fbfc0,
        kFogByte197 = 0x3f45c5c6,
        kFogByte214 = 0x3f56d6d7,
    };
#ifdef K2_QOL_CANTINA_FOG
    // The Upper City cantina and its dueling ring (tar_m02ae, the only area fogged this dark):
    // 10 / 25 hides the far side of the ring, so its fog starts and ends further out.
    if (FloatBits(m_vFogColor.x) == kFogByte23 && FloatBits(m_vFogColor.y) == kFogByte23 && FloatBits(m_vFogColor.z) == kFogByte21 &&
        m_fFogEnd < 45.0f) {
        m_fFogStart = 20.0f;
        m_fFogEnd = 45.0f;
    }
#endif
#ifdef K2_MOBILE_OPT   // the mobile port's: a fog-colour table that turns fog off, and the far clip at the fog end
    bool bClipFar = FloatBits(m_vFogColor.x) == kFogByte112 && FloatBits(m_vFogColor.y) == kFogByte143 && FloatBits(m_vFogColor.z) == kFogByte142;
    bool bAntiAlias = IosAntiAliasOn();
    if (!bClipFar && !bAntiAlias)
    {
        bool bNoFog = false;
        switch (FloatBits(m_vFogColor.x))
        {
        case kFogByte9: bNoFog = FloatBits(m_vFogColor.y) == kFogByte26 && FloatBits(m_vFogColor.z) == kFogByte30; break;
        case kFogByte26: bNoFog = FloatBits(m_vFogColor.y) == kFogByte22 && FloatBits(m_vFogColor.z) == kFogByte17; break;
        case kFogByte29:
            if (FloatBits(m_vFogColor.y) == kFogByte50 && FloatBits(m_vFogColor.z) == kFogByte75 && m_fFogEnd < 60.0f)
                m_fFogEnd = 60.0f;
            break;
        case kFogByte31: bNoFog = FloatBits(m_vFogColor.y) == kFogByte26 && FloatBits(m_vFogColor.z) == kFogByte5; break;
        case kFogByte73: bNoFog = FloatBits(m_vFogColor.y) == kFogByte52 && FloatBits(m_vFogColor.z) == kFogByte58; break;
        case kFogByte140: bNoFog = FloatBits(m_vFogColor.y) == kFogByte147 && FloatBits(m_vFogColor.z) == kFogByte143; break;
        case kFogByte150: bNoFog = FloatBits(m_vFogColor.y) == kFogByte160 && FloatBits(m_vFogColor.z) == kFogByte190; break;
        case kFogByte177: bNoFog = FloatBits(m_vFogColor.y) == kFogByte146 && FloatBits(m_vFogColor.z) == kFogByte103; break;
        case kFogByte191: bNoFog = FloatBits(m_vFogColor.y) == kFogByte197 && FloatBits(m_vFogColor.z) == kFogByte180; break;
        case kFogByte214: bNoFog = FloatBits(m_vFogColor.y) == kFogByte171 && FloatBits(m_vFogColor.z) == kFogByte169; break;
        }
        if (bNoFog)
            m_bFogEnabled = 0;
        IOS_FOG_CLIPDIST = m_bFogEnabled ? m_fFogEnd : 9500.0f;
    }
    if (bClipFar)
        IOS_FOG_CLIPDIST = 20000.0f;
#endif
#endif

    if (!m_bFogEnabled)
    {
        GLRender::Disable(Aurora::kCapFog);
        return;
    }

    GLRender::Enable(Aurora::kCapFog);
    GLRender::SetFogMode(m_nFogMode == 0 ? Aurora::kFogLinear
                             : (m_nFogMode == 1 ? Aurora::kFogExp : Aurora::kFogExp2));

    if (LightManager::m_pOnlyGob)
    {
// The blind-lights pass fogs everything to black rather than to the scene colour.
        Vector vBlack = { 0.0f, 0.0f, 0.0f };
        GLRender::SetFogModev(Aurora::kCapFogColor, vBlack);
    }
#if !KOTOR1
    else if (g_ForceSight)
        GLRender::SetFogModev(Aurora::kCapFogColor, m_vFogColorGrey);
#endif
    else
        GLRender::SetFogModev(Aurora::kCapFogColor, m_vFogColor);

    float fStart = m_fFogStart;
    GLRender::SetFogModef(Aurora::kCapFogStart, fStart);
    float fEnd = m_fFogEnd;
    GLRender::SetFogModef(Aurora::kCapFogEnd, fEnd);
    float fDensity = m_fFogDensity;
    GLRender::SetFogModef(Aurora::kCapFogDensity, fDensity);
}

void Scene::UnlistSpecialParts(Part* pPart)
{
    if (!pPart)
        return;

    PartLight* pLight = pPart->AsPartLight();
    if (pLight)
    {
        m_pLightManager->RemoveLight(pLight, false);
        SceneBSPRemovePart(this, pLight);
    }

    PartEmitter* pEmitter = pPart->AsPartEmitter();
    if (pEmitter)
    {
        m_emitters.Remove(pEmitter);
        SceneBSPRemovePart(this, pEmitter);
    }

    for (int i = 0; i < pPart->m_nChildren; i++)
        UnlistSpecialParts(pPart->m_paChildren[i]);
}

void Scene::RenderGobLast(CAurObject* pObject)
{
    Objects().Remove(static_cast<Gob*>(pObject));
    Objects().Add(static_cast<Gob*>(pObject));
}

#if KOTOR1
extern template void List<Gob*>::Insert(Gob*, int);
#endif
void Scene::RenderGobFirst(CAurObject* pObject)
{
    Objects().Remove(static_cast<Gob*>(pObject));
    Objects().Insert(static_cast<Gob*>(pObject), 0);
}

void Scene::RenderGobAfter(CAurObject* pObject, CAurObject* pAnchor)
{
    int nIndex = Objects().IndexOf(static_cast<Gob*>(pAnchor)) + 1;

    Objects().Remove(static_cast<Gob*>(pObject));
    Objects().Insert(static_cast<Gob*>(pObject), nIndex);
}

void Scene::RenderGobBefore(CAurObject* pObject, CAurObject* pAnchor)
{
    int nIndex = Objects().IndexOf(static_cast<Gob*>(pAnchor));

    Objects().Remove(static_cast<Gob*>(pObject));
    Objects().Insert(static_cast<Gob*>(pObject), nIndex);
}

void Scene::SetTileLights(const Vector& v, const Vector& vLightA, const Vector& vLightB)
{
    TileData* pTileData = SceneBSPGetTileData(this, v);

    if (pTileData->m_paLights[0] && pTileData->m_paLights[0]->m_vLightColour != vLightA)
    {
        pTileData->m_paLights[0]->m_vLightColour = vLightA;
        UpdateStaticLighting(this, pTileData->m_paLights[0]);
        pTileData->m_paLights[0]->m_nCastsShadows = 1;
        pTileData->m_paLights[0]->m_fShadowRadius = 12.0f;
        pTileData->m_paLights[0]->m_nAffectDynamic = 1;
        pTileData->m_paLights[0]->m_nLightPriority = 5;
    }

    if (pTileData->m_paLights[1] && pTileData->m_paLights[1]->m_vLightColour != vLightB)
    {
        pTileData->m_paLights[1]->m_vLightColour = vLightB;
        UpdateStaticLighting(this, pTileData->m_paLights[1]);
        pTileData->m_paLights[1]->m_nCastsShadows = 1;
        pTileData->m_paLights[1]->m_fShadowRadius = 8.0f;
        pTileData->m_paLights[1]->m_nAffectDynamic = 1;
        pTileData->m_paLights[1]->m_nLightPriority = 5;
    }
}

void Scene::SetTileFlames(const Vector& v, CAurObject* pObjectA, CAurObject* pObjectB)
{
    TileData* pTileData = SceneBSPGetTileData(this, v);

    SetSingleTileFlame(pObjectA, &pTileData->m_paFlameGobs[0], pTileData->m_paFlameParts[0]);
    if (pObjectA)
        static_cast<Gob*>(pObjectA)->m_pTileData = pTileData;

    SetSingleTileFlame(pObjectB, &pTileData->m_paFlameGobs[1], pTileData->m_paFlameParts[1]);
    if (pObjectB)
        static_cast<Gob*>(pObjectB)->m_pTileData = pTileData;
}

void Scene::SetSingleTileFlame(CAurObject* pObject, Gob** ppFlameGob, Part* pPart)
{
    List<PartLight*> lights;
    Gob* pGob = static_cast<Gob*>(pObject);

    if (*ppFlameGob == pGob)
        return;

    if (*ppFlameGob)
    {
// The outgoing flame was contributing static light to its surroundings; re-light
// everything it touched before it goes away.
        (*ppFlameGob)->CollectLights(lights);
        for (int i = 0; i < lights.m_count; i++)
            UpdateStaticLighting(this, lights.m_pData[i]);

        delete *ppFlameGob;
        *ppFlameGob = 0;
    }

    if (pGob && pPart)
    {
        Gob* pNewGob = pGob;
        pNewGob->m_vPosition = pPart->m_vLocalPosition;
        pNewGob->m_qOrientation = pPart->m_qLocalOrientation;
        pNewGob->SetScene(this);

        pNewGob->CollectLights(lights);
        for (int i = 0; i < lights.m_count; i++)
            UpdateStaticLighting(this, lights.m_pData[i]);

        *ppFlameGob = pNewGob;
    }
}

void Scene::ClearVisibility()
{
    for (int i = 0; i < m_rooms.m_count; i++)
        m_rooms.m_pData[i]->m_visibleRooms.m_count = 0;
}

int Scene::QueryVisibility(CAurRoom* pFrom, CAurRoom* pRoom)
{
    if (pFrom->m_visibleRooms.m_count <= 0)
        return 0;

    return pFrom->m_visibleRooms.Contains(pRoom);
}

void Scene::SetVisibility(CAurRoom* pFrom, CAurRoom* pRoom, int bVisible)
{
    int nFound = pFrom->m_visibleRooms.Contains(pRoom);

    if (bVisible)
    {
        if (nFound == 0)
        {
            pFrom->m_visibleRooms.Add(pRoom);
            pRoom->m_visibleRooms.Add(pFrom);
        }
    }
    else if (nFound != 0)
    {
        pFrom->m_visibleRooms.Remove(pRoom);
        pRoom->m_visibleRooms.Remove(pFrom);
    }
}

int Scene::LoadVisibility(char* pszName)
{
    char szRoom[64];
    char szVisible[64];
    int nCount;

    ClearVisibility();

    void* pRes = AurResGet(pszName, const_cast<char*>(".vis"), 0, true);
    if (!pRes)
        return 0;

    char* pszLine;
    while ((pszLine = AurResGetNextLine()) != 0)
    {
        szRoom[0] = '\0';
        nCount = 0;
        static_cast<void>(sscanf(pszLine, "%s%d", szRoom, &nCount));

        CAurRoom* pFrom = FindRoom(szRoom);
        if (nCount < 1)
            continue;
        if (!pFrom)
            continue;

        for (int i = 0; i < nCount; i++)
        {
            szVisible[0] = '\0';
            pszLine = AurResGetNextLine();
            static_cast<void>(sscanf(pszLine, "%s", szVisible));

            CAurRoom* pRoom = FindRoom(szVisible);
            if (pRoom)
                SetVisibility(pFrom, pRoom, 1);
        }
    }

    AurResFree(pRes, 0);
    return 1;
}

#if KOTOR1
__attribute__((visibility("hidden"))) int g_nMeshSkipTexMin;
__attribute__((visibility("hidden"))) int g_nMeshSkipTexMax;
__attribute__((visibility("hidden"))) int g_nMeshSkipTexMin2;
__attribute__((visibility("hidden"))) int g_nMeshSkipTexMax2;
__attribute__((visibility("hidden"))) int g_nMeshSkipPartMin = -1;
__attribute__((visibility("hidden"))) int g_nMeshSkipPartMax = -1;
#endif
void Scene::DoMeshBuckets()
{
    List<TileData*> tileList;
#if defined(K2_QOL_IMPROVEMENTS) && !KOTOR1
    s_listLateAdditiveMeshes.m_count = 0;
#endif

    for (int i = 0; i < g_listBackgroundBucket.m_count; i++)
    {
        PartTriMesh* pPart = g_listBackgroundBucket.m_pData[i];
        Material* pMaterial = pPart->m_pMaterial;
        pMaterial->BindTexture0();
        pMaterial->SetBlendingMode(0);
        pPart->Mat();
        pPart->Draw(false);
    }

    List<int> caps(*AurTextureGetOrdering());

    for (int i = 0; i < caps.m_count; i++)
    {
        int nTextureId = caps.m_pData[i];
        if (meshbuckets[nTextureId].m_nCount == 0)
            continue;

        Material* pMaterial = meshbuckets[nTextureId].m_paParts[0]->m_pMaterial;
#if defined(K2_QOL_IMPROVEMENTS) && !KOTOR1
        if (K2QolIsAdditiveMaterial(pMaterial))
        {
            for (int j = 0; j < meshbuckets[nTextureId].m_nCount; j++)
            {
                PartTriMesh* pPart = meshbuckets[nTextureId].m_paParts[j];
                s_listLateAdditiveMeshes.Add(pPart);
                TileData* pTile = pPart->m_pTileData;
                if (pTile && !tileList.Contains(pTile))
                    tileList.Add(pTile);
            }
            continue;
        }
#endif
#if !KOTOR1  // K1 binds the texture in Mat()
        pMaterial->BindTexture0();
#endif
        pMaterial->SetBlendingMode(0);

        for (int j = 0; j < meshbuckets[nTextureId].m_nCount; j++)
        {
            PartTriMesh* pPart = meshbuckets[nTextureId].m_paParts[j];
#if KOTOR1  // K1: debug skip ranges (two texture-id ranges, one part-index range)
            if ((nTextureId >= g_nMeshSkipTexMin && nTextureId <= g_nMeshSkipTexMax)
                || (nTextureId >= g_nMeshSkipTexMin2 && nTextureId <= g_nMeshSkipTexMax2)
                || (j >= g_nMeshSkipPartMin && j <= g_nMeshSkipPartMax))
                continue;
#endif
            pPart->Mat();
            pPart->Draw(false);

            if (pPart->m_pTileData && !tileList.Contains(pPart->m_pTileData))
                tileList.Add(pPart->m_pTileData);
        }
    }

    for (int i = 0; i < meshbuckets[0].m_nCount; i++)
    {
        PartTriMesh* pPart = meshbuckets[0].m_paParts[i];
        if (i == 0)
            pPart->m_pMaterial->BindTexture0();
        pPart->Mat();
        pPart->Draw(false);

        if (!tileList.Contains(pPart->m_pTileData))
            tileList.Add(pPart->m_pTileData);
    }

    GLRender::EnableClientState(Aurora::kClientStateTexCoord);

    for (int i = 0; i < tileList.m_count; i++)
    {
        TileData* pTile = tileList.m_pData[i];
        if (pTile)
        {
            pTile->m_nFadeState = TileData::FADE_NOT_CHECKED;
            pTile->m_fOpacity = pTile->m_fBaseOpacity;
        }
    }
}

void Scene::DoCapBucket()
{
    List<int> caps(*AurTextureGetOrdering());

    for (int i = 0; i < caps.m_count; i++)
    {
        int nTextureId = caps.m_pData[i];
        if (capbuckets[nTextureId].m_nCount == 0)
            continue;

        Material* pMaterial = capbuckets[nTextureId].m_paParts[0]->m_pMaterial;
        pMaterial->BindTexture0();
        pMaterial->SetBlendingMode(0);

        for (int j = 0; j < capbuckets[nTextureId].m_nCount; j++)
        {
            PartTriMesh* pPart = capbuckets[nTextureId].m_paParts[j];
            pPart->Mat();
            pPart->Draw(false);
        }
    }

    for (int i = 0; i < capbuckets[0].m_nCount; i++)
    {
        PartTriMesh* pPart = capbuckets[0].m_paParts[i];
        if (i == 0)
            pPart->m_pMaterial->BindTexture0();
        pPart->Mat();
        pPart->Draw(false);
    }
}

void Scene::DoBinBucket()
{
    if (!enablegrass)
        return;

#if !KOTOR1  // K1 blends only
    GLRender::IsEnabled(Aurora::kCapAlphaTest);
    GLRender::Enable(Aurora::kCapAlphaTest);
#endif
    GLRender::Enable(Aurora::kCapBlend);
    GLRender::SetBlendFunc(Aurora::kBlendSrcAlpha, Aurora::kBlendInvSrcAlpha);

#if !KOTOR1
    RenderInterface::SetDepthMask(1);
#endif

    GLRender::Disable(Aurora::kCapCullFace);

    GLRender::ActivateTextureStage(0);

    if (LightManager::m_pOnlyGob)
        GLRender::EnableClientState(Aurora::kClientStateColor);
    GLRender::DisableClientState(Aurora::kClientStateColor);

    ASLgl::glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

    GLRender::SetAlphaFunc(Aurora::kCmpGreater, grassalphatest);

    for (int i = 0; i < g_GrassInfo.m_count; i++)
    {
        if (!AurGrassMaterialInUse(i))
            continue;

        for (int j = 0; j < binbucket.m_count; j++)
            binbucket.m_pData[j]->RenderGrassPolys(i, CurrentCamera->m_vPosition,
                                                   m_pWindManager);
    }

    GLRender::SetAlphaFunc(Aurora::kCmpGreater, alphatest);
    PartTriMesh::m_bMaterialHasChanged = true;
    GLRender::Enable(Aurora::kCapCullFace);
}

void Scene::DoFadeBuckets()
{
    List<TileData*> tileList;
    List<int> caps(*AurTextureGetOrdering());

    m_pLightManager->EnableLightsDynamicOnly();

#if KOTOR1  // K1 has no glColorMaterial; materials go to GL_FRONT_AND_BACK
    GLRender::Enable(Aurora::kCapBlend);
    GLRender::EnableColorMaterial();
#else
    GLRender::Enable(Aurora::kCapBlend);
    GLRender::SetColorMaterial(Aurora::kFacingFront, Aurora::kColorTrackingEmission);
    GLRender::EnableColorMaterial();
#endif

    if (LightManager::m_pOnlyGob)
        GLRender::EnableClientState(Aurora::kClientStateColor);

    float vWhite[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float vBlack[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

#if KOTOR1
    ASLgl::glMaterialfv(GL_FRONT_AND_BACK, materialTranslationTable[kMaterialAmbient], vWhite);
    ASLgl::glMaterialfv(GL_FRONT_AND_BACK, materialTranslationTable[kMaterialDiffuse], vWhite);
    ASLgl::glMaterialfv(GL_FRONT_AND_BACK, materialTranslationTable[kMaterialEmission], vBlack);
#else
    GLRender::SetMaterialv(Aurora::kFacingFront, Aurora::kMaterialAmbient, vWhite);
    GLRender::SetMaterialv(Aurora::kFacingFront, Aurora::kMaterialDiffuse, vWhite);
    GLRender::SetMaterialv(Aurora::kFacingFront, Aurora::kMaterialEmission, vBlack);
#endif

    for (int i = 0; i < caps.m_count; i++)
    {
        int nTextureId = caps.m_pData[i];
        if (fadebuckets[nTextureId].m_nCount == 0)
            continue;

        Material* pMaterial = fadebuckets[nTextureId].m_paParts[0]->m_pMaterial;
        pMaterial->BindTexture0();
        pMaterial->SetBlendingMode(0);

        for (int j = 0; j < fadebuckets[nTextureId].m_nCount; j++)
        {
            PartTriMesh* pPart = fadebuckets[nTextureId].m_paParts[j];

            float fSaved = pPart->m_fOpacity;
            pPart->m_fOpacity = fSaved * pPart->m_pTileData->m_fOpacity;
            if (pPart->m_fOpacity > 0.0f)
            {
                pPart->Mat();

                if (pPart->m_pTileData != 0 && pPart->m_pTileData->m_fFade < 1.0f)
                {
                    GLRender::Enable(Aurora::kCapFog);

                    float fFade = pPart->m_pTileData->m_fFade * pPart->m_pTileData->m_fFade;
                    Vector vSceneFog = m_vFogColor;
                    Vector vFogColor = { vSceneFog.x * fFade, vSceneFog.y * fFade,
                                         fFade * vSceneFog.z };
                    GLRender::SetFogModev(Aurora::kCapFogColor, vFogColor);
                    GLRender::SetFogModef(Aurora::kCapFogDensity, m_fFogDensity * 2.0f);
                    GLRender::SetFogModef(Aurora::kCapFogStart,
                                  pPart->m_pTileData->m_fFade * m_fFogStart);
                    GLRender::SetFogModef(Aurora::kCapFogEnd,
                                  pPart->m_pTileData->m_fFade * m_fFogEnd + 0.1f);
                    pPart->Draw(false);

                    if (!m_bFogEnabled)
                        GLRender::Disable(Aurora::kCapFog);
                    GLRender::SetFogModef(Aurora::kCapFogDensity, m_fFogDensity);
                    GLRender::SetFogModef(Aurora::kCapFogStart, m_fFogStart);
                    GLRender::SetFogModef(Aurora::kCapFogEnd, m_fFogEnd);
                    GLRender::SetFogModev(Aurora::kCapFogColor, m_vFogColor);
                }
                else
                {
                    pPart->Draw(false);
                }
            }

            pPart->m_fOpacity = fSaved;

            if (!tileList.Contains(pPart->m_pTileData))
                tileList.Add(pPart->m_pTileData);
        }
    }

    for (int i = 0; i < fadebuckets[0].m_nCount; i++)
    {
        PartTriMesh* pPart = fadebuckets[0].m_paParts[i];
        float fSaved;
        if (i == 0)
            pPart->m_pMaterial->BindTexture0();

        fSaved = pPart->m_fOpacity;
        pPart->m_fOpacity = fSaved * pPart->m_pTileData->m_fOpacity;
        if (pPart->m_fOpacity > 0.0f)
        {
            pPart->Mat();

            if (pPart->m_pTileData != 0 && pPart->m_pTileData->m_fFade < 1.0f)
            {
                GLRender::Enable(Aurora::kCapFog);

                float fFade = pPart->m_pTileData->m_fFade * pPart->m_pTileData->m_fFade;
                Vector vSceneFog = m_vFogColor;
                Vector vFogColor = { vSceneFog.x * fFade, vSceneFog.y * fFade,
                                     fFade * vSceneFog.z };
                GLRender::SetFogModev(Aurora::kCapFogColor, vFogColor);
                GLRender::SetFogModef(Aurora::kCapFogDensity, m_fFogDensity * 2.0f);
                GLRender::SetFogModef(Aurora::kCapFogStart,
                              pPart->m_pTileData->m_fFade * m_fFogStart);
                GLRender::SetFogModef(Aurora::kCapFogEnd,
                              pPart->m_pTileData->m_fFade * m_fFogEnd + 0.1f);
                pPart->Draw(false);

                if (!m_bFogEnabled)
                    GLRender::Disable(Aurora::kCapFog);
                GLRender::SetFogModef(Aurora::kCapFogDensity, m_fFogDensity);
                GLRender::SetFogModef(Aurora::kCapFogStart, m_fFogStart);
                GLRender::SetFogModef(Aurora::kCapFogEnd, m_fFogEnd);
                GLRender::SetFogModev(Aurora::kCapFogColor, m_vFogColor);
            }
            else
            {
                pPart->Draw(false);
            }
        }

        pPart->m_fOpacity = fSaved;

        if (!tileList.Contains(pPart->m_pTileData))
            tileList.Add(pPart->m_pTileData);
    }

    for (int i = 0; i < tileList.m_count; i++)
    {
        TileData* pTile = tileList.m_pData[i];
        pTile->m_fOpacity = pTile->m_fBaseOpacity;
        pTile->m_nFadeState = TileData::FADE_NOT_CHECKED;
    }


    GLRender::DisableClientState(Aurora::kClientStateColor);
    GLRender::DisableColorMaterial();
    GLRender::Disable(Aurora::kCapBlend);
}

void Scene::DoEmitterBucket()
{
#if KOTOR1  // K1: g_RenderSkip keeps particles animating but skips every GL call and draw
    if (!g_RenderSkip)
    {
        ASLgl::glClearStencil(0);
        ASLgl::glStencilMask(0xff);
        ASLgl::glClear(GL_STENCIL_BUFFER_BIT);
    }

    if (!enableemitters)
        return;

    if (!g_RenderSkip)
    {
        GLRender::Enable(Aurora::kCapStencilTest);
        GLRender::Disable(Aurora::kCapStencilTest);
        PartEmitter::StartEmitterLayer();
    }

    qsort(emitterbucket.m_pData, emitterbucket.m_count, sizeof(PartEmitter*), EmitterOrderCmp);

    bool bAnimating = IsAnimating();

    if (enablerender)
    {
        for (int i = 0; i < emitterbucket.m_count; i++)
        {
            PartEmitter* pEmitter = emitterbucket.m_pData[i];

            if (bAnimating || (pEmitter->m_pGob && pEmitter->m_pGob->IsForceAnimated()))
                emitterbucket.m_pData[i]->AnimateParticles(DeltaT > 0.1f ? 0.1f : DeltaT);

            if (!g_RenderSkip)
                emitterbucket.m_pData[i]->RenderFunc();
        }
    }

    if (!g_RenderSkip)
        PartEmitter::StopEmitterLayer();
#else
    ASLgl::glClearStencil(0);
    ASLgl::glStencilMask(0xff);
    ASLgl::glClear(GL_STENCIL_BUFFER_BIT);

    if (!enableemitters)
        return;

    GLRender::Enable(Aurora::kCapStencilTest);
    GLRender::Disable(Aurora::kCapStencilTest);

    PartEmitter::StartEmitterLayer();

    qsort(emitterbucket.m_pData, emitterbucket.m_count, sizeof(PartEmitter*), EmitterOrderCmp);

    bool bAnimating = IsAnimating();

    if (enablerender)
    {
        for (int i = 0; i < emitterbucket.m_count; i++)
        {
            PartEmitter* pEmitter = emitterbucket.m_pData[i];

            if (bAnimating || (pEmitter->m_pGob && pEmitter->m_pGob->IsForceAnimated()))
                emitterbucket.m_pData[i]->AnimateParticles(DeltaT > 0.1f ? 0.1f : DeltaT);

            emitterbucket.m_pData[i]->RenderFunc();
        }
    }

    PartEmitter::StopEmitterLayer();

    ASLgl::glPushAttrib(0);
    ASLgl::glPopAttrib();
#endif
}

void Scene::Animate(float fTime)
{
    currentscene = this;

    if (interleaveanimations && g_bEnableInterleavedAnimations)
    {
        for (int i = m_cameras.m_count; i--; )
        {
            if (i < m_cameras.m_count)
                m_cameras.m_pData[i]->Animate(fTime);
        }

        for (int i = Objects().m_count; i--; )
        {
            if (i < Objects().m_count)
            {
                if (!Objects().m_pData[i]->m_bAnimatedOnce)
                    Objects().m_pData[i]->Animate(fTime);
            }
        }
    }
    else
    {
        for (int i = Objects().m_count; i--; )
        {
            if (i < Objects().m_count)
                Objects().m_pData[i]->Animate(fTime);
        }
    }

    currentscene = 0;
}

Scene* g_pLastScene;
extern unsigned int g_SceneRender_CurrentTime_ms;
int g_nDanglyMeshAnimationByMod3Counter = 2;
int enablescenerender = 1;
int count_render;
int countvertices;
extern int currenttextime;
int modelloadtime;
int appendtime;
int maxtotaltime;
int maxmodelloadtime;
int maxappendtime;

#include "port/win32api.h"

bool AurATIValidate();
void AurTextureMakeCurrent();
void AurTextureForceControllerUpdate();
void ManageSceneBSP(Scene* pScene);
#if KOTOR1 && defined(K2_QOL_K1_ANTIALIAS)
void K1SceneViewportToScreen();
#endif

void Scene::Render()
{
    AurATIValidate();
    g_pLastScene = this;
#if !KOTOR1  // K1 has no frame timestamp
    g_SceneRender_CurrentTime_ms = timeGetTime();
#endif

    if (m_nAnimateTick == 0)
        UpdateEmitters();

    if ((interleaveanimations == 0) | (g_bEnableInterleavedAnimations == 0))
        m_nAnimateTick++;

    g_nDanglyMeshAnimationByMod3Counter = (g_nDanglyMeshAnimationByMod3Counter + 1) % 3;

    if (enablescenerender)
    {
        count_render = 0;
        countvertices = 0;

        m_focusGob.SetPosition(m_vSceneFocus);
        PartTriMesh::m_bMaterialHasChanged = true;
        AurTextureMakeCurrent();
        AurTextureForceControllerUpdate();

        currentscene = this;
        ManageSceneBSP(this);

        ASLgl::glPushAttrib(GL_FOG_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
        GLRender::SetDepthFunc(Aurora::kCmpLEqual);
#if KOTOR1  // K1 also resets depth test, blend, lighting and texturing
        GLRender::Enable(Aurora::kCapDepthTest);
        GLRender::Disable(Aurora::kCapBlend);
        GLRender::Enable(Aurora::kCapLighting);
        GLRender::Enable(Aurora::kCapTexture2D);
#endif
        GLRender::EnableClientState(Aurora::kClientStateNormal);
        GLRender::EnableClientState(Aurora::kClientStateTexCoord);
        GLRender::EnableClientState(Aurora::kClientStateVertex);
        GLRender::Enable(Aurora::kCapAlphaTest);
        GLRender::SetAlphaFunc(Aurora::kCmpGreater, alphatest);
        GLRender::Enable(Aurora::kCapLighting);

#if KOTOR1 && defined(K2_QOL_K1_ANTIALIAS)
        // Only a scene with a current room is drawn into the anti-aliasing pbuffer
        // (RenderSinglePass); any other (the title screen's) is drawn at the screen's size.
        if ((m_pBSPRoot == 0 && m_rooms.m_count == 0) || m_pCurrentRoom == 0)
            K1SceneViewportToScreen();
#endif
        if (m_pBSPRoot == 0 && m_rooms.m_count == 0)
        {
            RenderOnlyGobs();
            currentscene = 0;
            ASLgl::glPopAttrib();
        }
        else
        {
            m_pWindManager->updateTimer(DeltaT);
            RenderSinglePass();
            ASLgl::glPopAttrib();
            currentscene = 0;

            if (maxtotaltime < modelloadtime + currenttextime + appendtime)
                maxtotaltime = modelloadtime + currenttextime + appendtime;
            if (maxmodelloadtime < modelloadtime)
                maxmodelloadtime = modelloadtime;
            modelloadtime = 0;
            if (maxappendtime < appendtime)
                maxappendtime = appendtime;
            appendtime = 0;

            IsAnimating();
        }
    }

    if (interleaveanimations && g_bEnableInterleavedAnimations)
        m_nAnimateTick++;
}

void Scene::UpdateEmitters()
{
    currentscene = this;

    for (int i = 0; i < m_rooms.m_count; i++)
    {
        nodedata* pNodeData = m_rooms.m_pData[i]->m_pNodeData;

        for (int j = 0; j < pNodeData->m_emitters.m_count; j++)
        {
            PartEmitter* pEmitter = pNodeData->m_emitters.m_pData[j];

            for (int k = 0; k < 100; k++)
                pEmitter->AnimateParticles(0.1f);
        }
    }

    for (int i = 0; i < m_emitters.m_count; i++)
    {
        PartEmitter* pEmitter = m_emitters.m_pData[i];

        for (int k = 0; k < 100; k++)
            pEmitter->AnimateParticles(0.1f);
    }

    currentscene = 0;
}

int Scene::SaveVisibility(char* pszDir, char* pszName)
{
    char szPath[256];

    ScenePrintf(szPath, sizeof(szPath), "%s/%s.VIS",
                (pszDir && *pszDir) ? pszDir : ".", pszName);

#if KOTOR1  // K1's ported fopen traces
    FILE* pFile = _fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/libsource/AURORA/aurscene.cpp",
                         "SaveVisibility", 5116, szPath, "w");
#else
    FILE* pFile = fopen(szPath, "w");
#endif
    if (!pFile)
        return 0;

    for (int i = 0; i < m_rooms.m_count; i++)
    {
        fprintf(pFile, "%s %d\n", m_rooms.m_pData[i]->m_szName,
                m_rooms.m_pData[i]->m_visibleRooms.m_count);

        for (int j = 0; j < m_rooms.m_pData[i]->m_visibleRooms.m_count; j++)
            fprintf(pFile, "  %s\n", m_rooms.m_pData[i]->m_visibleRooms.m_pData[j]->m_szName);
    }

    fclose(pFile);
    return 1;
}

extern List<Scene*> Scenes;

void AurEnableSaturation();
void AurDisableSaturation();
int  AurGetAvailableStencilBits();

Gob* FindGob(char* pszName);

void SetShadows(Part* pPart, int nShadows)
{
    PartLight* pLight = pPart->AsPartLight();
    if (pLight != 0)
    {
        MdlNodeLight* pNode = static_cast<MdlNodeLight*>(pLight->m_pNode);
        if (pNode->m_bAffectDynamic != 0 || pNode->m_bIsDynamic != 0)
            pLight->m_nCastsShadows = nShadows;
    }

    for (int i = 0; i < pPart->m_nChildren; i++)
        SetShadows(pPart->m_paChildren[i], nShadows);
}

void RenderPersonalSpace(Scene* pScene)
{
#if !KOTOR1  // K1 has no early-out; the per-gob test alone gates the spheres
    if (renderpersonalspace == 0)
        return;
#endif

    GLRender::Disable(Aurora::kCapDepthTest);
    for (int i = 0; i < pScene->Objects().m_count; i++)
    {
        Gob* pGob = pScene->Objects().m_pData[i];
        if ((renderpersonalspace || pGob->m_bRenderPersonalSpace) &&
            pGob->m_fPersonalRadius > 0.0f)
        {
            ASLgl::glPushMatrix();
            Transform(pGob->m_vPosition, pGob->m_qOrientation, 1);

            Vector vCenter;
            Vector vColor = { 1.0f, 0.6f, 1.0f };
            GLRender::DrawSphereBound(0.001f, vCenter, vColor, 1.0f, false);
            GLRender::DrawSphereBound(pGob->m_fPersonalRadius, vCenter, vColor, 1.0f, false);
            vColor.y = 0.0f;
            GLRender::DrawSphereBound(pGob->m_fCreatureRadius, vCenter, vColor, 1.0f, false);

            ASLgl::glPopMatrix();
        }
    }
    GLRender::Enable(Aurora::kCapDepthTest);
}

void InitGrassBins(Vector vMin, Vector vMax, List<List<CAurTriangleBin*> >& bins)
{
    bins.m_count = 0;
    int nBinsX = static_cast<int>(__builtin_ceilf((vMax.x - vMin.x) / Scene::binsize));
    int nBinsY = static_cast<int>(__builtin_ceilf((vMax.y - vMin.y) / Scene::binsize));
    if (nBinsX < 0 || nBinsY < 0)
        return;

    bins.SetSize(nBinsX);
    for (int i = 0; i < nBinsX; i++)
    {
        bins.m_pData[i].m_count = 0;
        bins.m_pData[i].SetSize(nBinsY);
        for (int j = 0; j < nBinsY; j++)
            bins.m_pData[i].m_pData[j] = 0;
    }
}

// View of MdlNodeTriMesh's layout, which is not included here; keep field order and padding in
// step with that header.
struct TriMeshView {
    char     _pad00[0x08];
    MaxTree* m_pTree;
    char     _pad10[0x78 - 0x10];
    short*   m_pIndices;
    int      m_nFrameCount;
    char     _pad84[0x18c - 0x84];
    int      m_nVertexStride;
    char     _pad190[0x194 - 0x190];
    int      m_nVertexOfs;
    char     _pad198[0x1c0 - 0x198];
    unsigned short m_nVertexCount;
#if KOTOR1  // K1's MdlNodeTriMesh has no dirt/hologram block
    char     _pad1c2[0x1d4 - 0x1c2];
#else
    char     _pad1c2[0x1dc - 0x1c2];
#endif
    int      m_nGeomBase;
};

extern bool AurGrassMaterialInUse(int nMaterial);
extern float Round(float fValue, float fIncrement);
extern Vector VectorMin(Vector vA, Vector vB);
extern Vector VectorMax(Vector vA, Vector vB);
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

static inline Vector RotateVectorDirect(Vector v, const Quaternion& q)
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

void Scene::AddFacesToBins(PartAABB* pPartAABB, Vector const& vOrigin,
                           List<List<CAurTriangleBin*> >& bins)
{
    MdlNodeTriMesh* pTriMesh = pPartAABB->m_pNode->AsMdlNodeTriMesh();
    TriMeshView* pT = reinterpret_cast<TriMeshView*>(pTriMesh);

    Vector vLocal;
    Quaternion qLocal(1.0f, 0.0f, 0.0f, 0.0f);
    pPartAABB->GetWorld(vLocal, qLocal);

    for (int64_t i = 0; i < pT->m_nFrameCount; i++) {
        const MaxFace* pFace = &reinterpret_cast<const MaxFace*>(pT->m_pIndices)[i];
        if (!AurGrassMaterialInUse(pFace->m_nMaterial))
            continue;

        void* pool = GLRender::LockPool(pT->m_pTree->AsModel()->m_nPool, 0, 0, 8);

        int stride = pT->m_nVertexStride;
        short i0 = pFace->m_nVIndex[0];
        short i1 = pFace->m_nVIndex[1];
        short i2 = pFace->m_nVIndex[2];
        float* v0 = reinterpret_cast<float*>(static_cast<char*>(pool) + stride * i0
                             + pT->m_nVertexOfs + static_cast<unsigned int>(pT->m_nGeomBase));
        float* v1 = reinterpret_cast<float*>(static_cast<char*>(pool) + stride * i1
                             + pT->m_nVertexOfs + static_cast<unsigned int>(pT->m_nGeomBase));
        float* v2 = reinterpret_cast<float*>(static_cast<char*>(pool) + stride * i2
                             + pT->m_nVertexOfs + static_cast<unsigned int>(pT->m_nGeomBase));

        Vector vAvg((v0[0] + v1[0] + v2[0]) * (1.0f / 3.0f),
                    (v0[1] + v1[1] + v2[1]) * (1.0f / 3.0f),
                    (v0[2] + v1[2] + v2[2]) * (1.0f / 3.0f));
        Vector vWorld = RotateVector(vAvg, qLocal);
        vWorld.x += vLocal.x;
        vWorld.y += vLocal.y;
        vWorld.z += vLocal.z;
        vWorld.x = Round(vWorld.x, 0.001f);
        vWorld.y = Round(vWorld.y, 0.001f);
        vWorld.z = Round(vWorld.z, 0.001f);

        int cellx = static_cast<int>(__builtin_floorf((vWorld.x - vOrigin.x) / Scene::binsize));
        cellx = cellx < bins.m_count - 1 ? cellx : bins.m_count - 1;
        cellx = cellx < 0 ? 0 : cellx;

        int celly = static_cast<int>(__builtin_floorf((vWorld.y - vOrigin.y) / Scene::binsize));
        celly = celly < bins.m_pData[cellx].m_count - 1 ? celly : bins.m_pData[cellx].m_count - 1;
        celly = celly < 0 ? 0 : celly;

        if (bins.m_pData[cellx].m_pData[celly] == 0)
            bins.m_pData[cellx].m_pData[celly] = new CAurTriangleBin;
        GLRender::UnlockPool(pT->m_pTree->AsModel()->m_nPool);
        bins.m_pData[cellx].m_pData[celly]->AddTriangle(*pFace, pPartAABB, pTriMesh);
        m_bHasBins = true;
    }
}

void RenderPaths(Scene* pScene)
{
    if (renderpaths == 0)
        return;

    GLRender::Disable(Aurora::kCapDepthTest);
    GLRender::Disable(Aurora::kCapLighting);
    GLRender::Disable(Aurora::kCapTexture2D);
    ASLgl::glColor3f(0.6f, 1.0f, 1.0f);

    for (int i = 0; i < pScene->Objects().m_count; i++)
    {
        Gob* pGob = pScene->Objects().m_pData[i];
        ASLgl::glBegin(GL_LINE_STRIP);
        for (int j = 0; j < pGob->m_pathInfo.m_nPathFloats / 3; j++)
        {
            ASLgl::glVertex3f(pGob->m_pathInfo.m_pPathFloats[j * 3],
                              pGob->m_pathInfo.m_pPathFloats[j * 3 + 1],
                              pGob->m_pathInfo.m_pPathFloats[j * 3 + 2]);
        }
        ASLgl::glEnd();
    }

    GLRender::Enable(Aurora::kCapLighting);
    GLRender::Enable(Aurora::kCapTexture2D);
    GLRender::Enable(Aurora::kCapDepthTest);
}

void RenderDrawableSquares()
{
#if KOTOR1  // K1 forwards to the renderer
    GLRender::RenderDrawableSquares();
#endif
}

char* CAurRoomGetName(CAurRoom* pRoom)
{
    return pRoom->m_szName;
}

void AurEnableInterleavedAnimations()
{
    g_bEnableInterleavedAnimations = true;
}

void AurDisableInterleavedAnimations()
{
    g_bEnableInterleavedAnimations = false;
}

const char* enablesaturation(char* pszArgs)
{
    K2_UNUSED(pszArgs);
    AurEnableSaturation();
    return "SUCCESS";
}

const char* disablesaturation(char* pszArgs)
{
    K2_UNUSED(pszArgs);
    AurDisableSaturation();
    return "SUCCESS";
}

void SetShadowStencilBuffer()
{
    int nBits = AurGetAvailableStencilBits();
    if (nBits != 0)
        ASLgl::glClearStencil(~(-1 << (nBits - 1)));

    ASLgl::glStencilMask(0xff);
    ASLgl::glClear(GL_STENCIL_BUFFER_BIT);  // GL_STENCIL_BUFFER_BIT (glClear's mask is a bitfield, so it is passed raw)
}

Vector CubePoint(Vector& vMin, Vector& vMax, int nCorner)
{
    Vector v;
    v.x = (nCorner & 1) ? vMax.x : vMin.x;
    v.y = ((nCorner / 2) & 1) ? vMax.y : vMin.y;
    v.z = (nCorner / 4) ? vMax.z : vMin.z;
    return v;
}

Scene* FindScene(char* pszName)
{
    for (int i = 0; i < Scenes.m_count; i++) {
        if (_stricmp(pszName, Scenes.m_pData[i]->m_szName) == 0)
            return Scenes.m_pData[i];
    }
    return 0;
}

extern float maxdeltat;
extern int maxappendtime;
extern int maxmodelloadtime;
extern int maxtotaltime;
extern int texturegettime;
extern int maxdecodetime;
extern int maxgltime;
extern int maxtexloadtime;
extern int maxtextime;

const char* dogantest(char* pszArgs)
{
    K2_UNUSED(pszArgs);
    maxdeltat = 0;
    maxappendtime = 0;
    maxmodelloadtime = 0;
    maxtotaltime = 0;
    texturegettime = 0;
    maxdecodetime = 0;
    maxgltime = 0;
    maxtexloadtime = 0;
    maxtextime = 0;
    return "done.";
}

const char* SetShadowing(char* pszGobName, int bShadowing)
{
    Gob* pGob = FindGob(pszGobName);
    if (pGob == 0)
        return "no gob by that name";
    if (pGob->m_nDynamicLights == 0)
        return "no dynamic lights on that gob - nothing done";

    SetShadows(pGob->m_pPart, bShadowing);
    return "success";
}

CAurScene* NewCAurScene(char* pszName)
{
    return new Scene(pszName);
}

int Gob::AttachToScene(CAurScene* a)
{
    SetScene(static_cast<Scene*>(a));
    return 1;
}

// The "find the one scene" console commands.

char* GetToken(char** ppszCursor);


const char* setcurrentroom(char* pszArgs)
{
    int nRoom;
    static_cast<void>(sscanf(pszArgs, "%d", &nRoom));

    if (nRoom >= Scenes.m_pData[0]->GetNumRooms())
        return "eh? That room doesn't exist.";

    Scenes.m_pData[0]->SetCurrentRoom(Scenes.m_pData[0]->GetRoom(nRoom));
    return "Done.";
}

const char* setonlygob(char* pszArgs)
{
    Gob* pGob = FindGob(pszArgs);

    Scene* pScene = FindScene(const_cast<char*>("mainscene"));
    if (pScene == 0)
        pScene = FindScene(const_cast<char*>("scene"));
    if (pScene == 0)
        return "Failure - cannot find scene.";

    pScene->SetBlindLights(static_cast<CAurObject*>(pGob));
    return "success";
}

const char* usevisibilitygraph(char* pszArgs)
{
    int nOn;
    static_cast<void>(sscanf(pszArgs, "%d", &nOn));

    Scene* pScene = FindScene(const_cast<char*>("mainscene"));
    if (pScene == 0)
        pScene = FindScene(const_cast<char*>("scene"));
    if (pScene == 0)
        return "Could not find scene";

    if (nOn == 0)
        pScene->DisableVisibilityGraph();
    else if (nOn == 1)
        pScene->EnableVisibilityGraph();

    return "Ok.";
}

const char* setgeomfade(char* pszArgs)
{
    int nMode = atoi(GetToken(&pszArgs));

    Scene* pScene = FindScene(const_cast<char*>("mainscene"));
    if (pScene == 0)
        FindScene(const_cast<char*>("scene"));
    if (pScene == 0)
        return NULL_NAME;

    if (pScene->SetGeomFadeProperties(nMode))
        return "SUCCESS";
    return NULL_NAME;
}

void CAurRoomDisableFog(CAurRoom* pRoom)
{
    for (int i = 0; i < pRoom->m_pNodeData->m_meshes.m_count; i++)
        pRoom->m_pNodeData->m_meshes.m_pData[i]->m_bFog = false;
}

void CAurRoomEnableFog(CAurRoom* pRoom)
{
    for (int i = 0; i < pRoom->m_pNodeData->m_meshes.m_count; i++)
        pRoom->m_pNodeData->m_meshes.m_pData[i]->m_bFog = true;
}

const char* disablefog(char* pszArgs)
{
    CAurRoom* pRoom = Scenes.m_pData[0]->FindRoom(pszArgs);
    if (pRoom == 0)
        return "Cannot find room with that name";

    CAurRoomDisableFog(pRoom);
    return "Success.";
}

const char* setscale(char* pszArgs)
{
    Gob* pGob = FindGob(GetToken(&pszArgs));
    if (pGob == 0)
        return NULL_NAME;

    char* pszScale = GetToken(&pszArgs);
    if (pGob->m_pScene != 0 &&
        !pGob->m_pScene->SetObjectScale(static_cast<CAurObject*>(pGob), static_cast<float>(atof(pszScale))))
        return NULL_NAME;

    return "SUCCESS";
}

// Animation, transparency and visibility console commands.

const char* disableanimations(char* pszArgs)
{
    K2_UNUSED(pszArgs);
    Scene* pScene = FindScene(const_cast<char*>("scene"));
    if (pScene == 0)
        pScene = FindScene(const_cast<char*>("mainscene"));
    if (pScene == 0)
        return "Failed: couldn't find the main scene.";

    pScene->DisableAnimations();
    return "Success.";
}

const char* enableanimations(char* pszArgs)
{
    K2_UNUSED(pszArgs);
    Scene* pScene = FindScene(const_cast<char*>("scene"));
    if (pScene == 0)
        pScene = FindScene(const_cast<char*>("mainscene"));
    if (pScene == 0)
        return "Failed: couldn't find the main scene.";

    pScene->EnableAnimations();
    return "Success.";
}

// The transparent-character pair.

const char* enabletransparentcharacters(char* pszArgs)
{
    float fAlpha = 0.5f;
    static_cast<void>(sscanf(pszArgs, "%f", &fAlpha));

    for (int i = 0; i < gobbucket.m_count; i++)
    {
        Gob* pGob = gobbucket.m_pData[i];
        if (pGob->m_pModel->m_nFlags & 4)
            pGob->SetColorShifting(Vector(1.0f, 1.0f, 1.0f), fAlpha, 1);
    }

    rendertransparentcharacters = 1;
    return "Done.";
}

const char* disabletransparentcharacters(char* pszArgs)
{
    K2_UNUSED(pszArgs);
    for (int i = 0; i < gobbucket.m_count; i++)
    {
        Gob* pGob = gobbucket.m_pData[i];
        if (pGob->m_pModel->m_nFlags & 4)
            pGob->SetColorShifting(Vector(1.0f, 1.0f, 1.0f), 1.0f, 1);
    }

    rendertransparentcharacters = 0;
    return "Done.";
}

// The force-animation pair.

const char* enableforceanimatins(char* pszArgs)
{
    Gob* pGob = FindGob(pszArgs);
    if (pGob == 0)
        return "Cannot find gob.";

    pGob->EnableForceAnimations();
    return "Success.";
}

const char* disableforceanimatins(char* pszArgs)
{
    Gob* pGob = FindGob(pszArgs);
    if (pGob == 0)
        return "Cannot find gob.";

    pGob->DisableForceAnimations();
    return "Success.";
}

// The visibility console commands.

const char* visibilityset(char* pszArgs)
{
    char szFrom[64];
    char szTo[64];
    int nVisible = 1;

    szTo[0] = '\0';
    szFrom[0] = '\0';
    static_cast<void>(sscanf(pszArgs, "%s %s %d", szFrom, szTo, &nVisible));

    CAurRoom* pFrom = Scenes.m_pData[0]->FindRoom(szFrom);
    CAurRoom* pTo = Scenes.m_pData[0]->FindRoom(szTo);
    if (pFrom == 0 || pTo == 0)
        return " unable to find room by that name";

    Scenes.m_pData[0]->SetVisibility(pFrom, pTo, nVisible);
    return nVisible ? "ok set visibility" : "ok, cleared visibility";
}

const char* visibilitysave(char* pszArgs)
{
    char szName[16];
    static_cast<void>(sscanf(pszArgs, "%s", szName));

    if (Scenes.m_pData[0]->SaveVisibility(const_cast<char*>(""), szName))
        return "ok";
    return "nope";
}

const char* visibilityload(char* pszArgs)
{
    char szName[16];
    static_cast<void>(sscanf(pszArgs, "%s", szName));

    if (Scenes.m_pData[0]->LoadVisibility(szName))
        return "ok";
    return "nope";
}

#if !KOTOR1  // K1 has no distortion buffer
// The distortion-buffer bracket. The latch is set only when the buffer is turned on;
// the off path leaves it to ClearDistortBufferIfNeeded.

void SetDistortBuffer(bool bEnable)
{
    if (bEnable)
        g_bDistortBufferUsed = 1;

    IosSetDistortBuffer(bEnable);
}

#ifdef K2_QOL_IMPROVEMENTS
void AurGetClearColor(float* pfColor);
void AurSetClearColor(float fRed, float fGreen, float fBlue, float fAlpha);
#endif

void ClearDistortBufferIfNeeded()
{
    if (g_bDistortBufferUsed != 1)
        return;

#ifdef K2_QOL_IMPROVEMENTS
    float afClearColor[4];
    AurGetClearColor(afClearColor);
#endif
    IosSetDistortBuffer(true);
    ASLgl::glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ASLgl::glClear(GL_COLOR_BUFFER_BIT);
    IosSetDistortBuffer(false);
#ifdef K2_QOL_IMPROVEMENTS
    AurSetClearColor(afClearColor[0], afClearColor[1], afClearColor[2], afClearColor[3]);
#endif
    g_bDistortBufferUsed = 0;
}
#endif // !KOTOR1

Vector Round(const Vector& v, float fIncrement)
{
    return Vector(Round(v.x, fIncrement),
                  Round(v.y, fIncrement),
                  Round(v.z, fIncrement));
}

char* getgobpos(char* pszArgs)
{
    Gob* pGob = FindGob(pszArgs);
    if (pGob == 0)
        return const_cast<char*>(NULL_NAME);

    char* pszBuf = new char[64];
    ScenePrintf(pszBuf, 64, "x %f y %f z %f",
                pGob->m_vPosition.x, pGob->m_vPosition.y, pGob->m_vPosition.z);
    return pszBuf;
}

PartTrigger* parttrigger;
PartTrigger* AurPartTriggerCreate(CAurScene* pScene, Vector* paVertices, int nVertices, int bSkipGroundProbe,
                                  float* pafColor, void* pOwner);
void AurPartTriggerDelete(CAurScene* pScene, PartTrigger* pPartTrigger, bool bDelete);
void AurPartTriggerSetColor(PartTrigger* pPartTrigger, float* pfColor);
void AurPartTriggerSetHighlight(PartTrigger* pPartTrigger, bool bHighlight, float fHeight,
                                bool bAffectDynamic);
void AurPartTriggerSetHighlightParams(PartTrigger* pPartTrigger, const Vector& vColor,
                                      float fAlpha, bool bPulse, float fPulseRate,
                                      bool bAdditive);

void Scene::RemoveFromSceneManager(Part* pPart)
{
    PartEmitter* pEmitter = pPart->AsPartEmitter();
    PartTrigger* pTrigger = static_cast<PartTrigger*>(pPart->AsPartTrigger());

    if (pEmitter)
    {
        if (emitterbucket.Contains(pEmitter))
            emitterbucket.Remove(pEmitter);
    }
    if (pTrigger)
    {
        if (triggerbucket.Contains(pTrigger))
            triggerbucket.Remove(pTrigger);
    }
}

void Scene::RemoveFromSceneManager(Gob* pGob)
{
    if (gobbucket.Contains(pGob))
        gobbucket.Remove(pGob);
#if !KOTOR1  // K1 has no hologram bucket
    else if (g_listHologramBucket.Contains(pGob))
        g_listHologramBucket.Remove(pGob);
#endif
}

const char* replacetilereferences(char* pszArgs)
{
    Vector vPos;
    char szTo[128];
    char szFrom[128];
    if (sscanf(pszArgs, "%f%f%f %s %s", &vPos.x, &vPos.y, &vPos.z, szTo, szFrom) != 5)
        return "Usage: replacetilereferences <position> <oldtexname> <newtexname>";

    return AurTextureReplaceTileReferences(Scenes.m_pData[0], vPos, szFrom, szTo) ? "Success"
                                                                                  : "Failure";
}

const char* enableshadowing(char* pszArgs)
{
    Gob* pGob = FindGob(pszArgs);
    if (pGob == 0)
        return "no gob by that name";
    if (pGob->m_nDynamicLights == 0)
        return "no dynamic lights on that gob - nothing done";

    SetShadows(pGob->m_pPart, 1);
    return "success";
}

const char* disableshadowing(char* pszArgs)
{
    Gob* pGob = FindGob(pszArgs);
    if (pGob == 0)
        return "no gob by that name";
    if (pGob->m_nDynamicLights == 0)
        return "no dynamic lights on that gob - nothing done";

    SetShadows(pGob->m_pPart, 0);
    return "success";
}

const char* initscenebsp(char* pszArgs)
{
    int nWidth = 0;
    int nHeight = 0;
    int nBorder = 0;
    int nBins = 0;
    static_cast<void>(sscanf(pszArgs, "%d%d%d%d", &nWidth, &nHeight, &nBorder, &nBins));

    if (nWidth == 0 || nHeight == 0)
        return "USAGE: initscenebsp width height bordersize hasbins";

    Scenes.m_pData[0]->InitBSP(nWidth, nHeight, nBorder, nBins != 0, 10.0f);
    return "OK";
}

const char* spawnrooms(char* pszArgs)
{
    char szLayout[128];
    if (sscanf(pszArgs, "%s", szLayout) != 1)
        return "USAGE: spawnrooms <layout filename>";

    if (Scenes.m_pData[0]->SpawnRooms(szLayout) == 0)
        return "Could not open layout file...";
    return "Success, attempted to spawn the rooms";
}

const char* addtrigger(char* pszArgs)
{
    if (parttrigger != 0)
        return "Test trigger already exists";

    Scene* pScene = Scenes.m_pData[0];

    int nArg = 1;
    if (sscanf(pszArgs, "%d", &nArg) > 1)
        return "Usage: addtrigger [geometry is 3d]";

    float afColor[4] = {0.0f, 1.0f, 0.0f, 0.25f};

    List<Vector> list;
    list.Add(Vector(9.5f, 11.0f, 1.0f));
    list.Add(Vector(5.0f, 11.0f, 1.0f));
    list.Add(Vector(8.0f, 14.0f, 1.0f));
    list.Add(Vector(0.0f, 10.0f, 1.0f));
    list.Add(Vector(8.0f, 6.0f, 1.0f));
    list.Add(Vector(8.2f, 6.2f, 1.0f));
    list.Add(Vector(4.9f, 9.0f, 1.0f));
    list.Add(Vector(9.5f, 9.0f, 1.0f));

    parttrigger = AurPartTriggerCreate(pScene, list.m_pData, list.m_count, nArg, afColor, 0);

    list.m_count = 0;
    list.Add(Vector(0.0f, 0.0f, 1.0f));
    list.Add(Vector(1.0f, 0.0f, 4.0f));
    list.Add(Vector(6.0f, 7.0f, 5.0f));
    list.Add(Vector(0.0f, 1.0f, 10.0f));

    parttrigger = AurPartTriggerCreate(pScene, list.m_pData, list.m_count, nArg, afColor, 0);
    return "OK, added the trigger to the scene";
}

char* hittrigger(char* pszArgs)
{
    K2_UNUSED(pszArgs);
    Scene* pScene = Scenes.m_pData[0];

    CHitInfo* pHitInfo = new CHitInfo(0, CHitInfo::kMaterialTrigger, 0);
    Vector vEnd = Vector(0.5f, 0.5f, -10.0f);
    if (pScene->HitCheck(Vector(0.5f, 0.5f, 60.0f), vEnd, &vEnd, pHitInfo) == 0 ||
        (pHitInfo->m_nHitMaterialBit & CHitInfo::kMaterialTrigger) == 0)
        return const_cast<char*>("No hit on a trigger!");

    static char szResult[256];
    ScenePrintf(szResult, 256, "OK, hit the trigger at (%.2f, %.2f, %.2f)", vEnd.x, vEnd.y,
                vEnd.z);
    return szResult;
}

void lightstaticmesh()
{
    for (int i = 0; i < Scenes.m_count; i++)
        Scenes.m_pData[i]->ComputeStaticLighting();

    staticshadowbucket.m_count = 0;
}

const char* highlighttrigger(char* pszArgs)
{
    if (parttrigger == 0)
        return "Trigger does not exist";

    int nHighlight;
    int nAffectDynamic = 0;
    float fHeight = 10.0f;
    int nParsed = sscanf(pszArgs, "%d %f %d", &nHighlight, &fHeight, &nAffectDynamic);
    if (static_cast<unsigned>(nParsed - 1) <= 2)
    {
        AurPartTriggerSetHighlight(parttrigger, nHighlight != 0, fHeight, nAffectDynamic != 0);
        return "Success.";
    }
    return "Usage: highlighttrigger [0-1] [height] [affect dynamic]";
}

const char* modifytrigger(char* pszArgs)
{
    if (parttrigger == 0)
        return "Trigger does not exist";

    float afColor[4];
    if (sscanf(pszArgs, "%f %f %f %f", &afColor[0], &afColor[1], &afColor[2], &afColor[3]) != 4)
        return "Usage: modifytrigger r g b a";

    AurPartTriggerSetColor(parttrigger, afColor);
    return "OK, set the trigger's color";
}

const char* deletetrigger(char* pszArgs)
{
    K2_UNUSED(pszArgs);
    if (parttrigger == 0)
        return "Trigger does not exist";

    AurPartTriggerDelete(Scenes.m_pData[0], parttrigger, false);
    parttrigger = 0;
    return "OK, deleted the trigger from the scene";
}

const char* highlightparams(char* pszArgs)
{
    if (parttrigger == 0)
        return "Trigger does not exist";

    Vector vColor;
    float fPulse;
    float fFrequency;
    int nPulseOn;
    if (sscanf(pszArgs, "%f%f%f %f %d %f", &vColor.x, &vColor.y, &vColor.z, &fPulse, &nPulseOn,
               &fFrequency) != 6)
        return "Usage: highlightparams color alpha pulse frequency";

    AurPartTriggerSetHighlightParams(parttrigger, vColor, fPulse, nPulseOn != 0, fFrequency, false);
    return "Success.";
}

#if KOTOR1  // K1: the model streams are SDL_RWops
struct SDL_RWops;
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject* NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif

const char* finishscenegeometry(char* pszArgs)
{
    K2_UNUSED(pszArgs);
    Scene* pScene = FindScene(const_cast<char*>("mainscene"));
    if (pScene == 0)
        pScene = FindScene(const_cast<char*>("scene"));
    if (pScene == 0)
        return "Failure - could not find scene.";

    pScene->FinishSceneGeometry();
    return "Success.";
}

char* bload(char* pszName);
char* spawnbinarytile(char* pszArgs)
{
    char szModel[128];
    char szModelMdl[128];
    Vector vParsed;
    szModel[0] = '\0';
    int nAngle = 0;
    int nUnused = 0;
    static_cast<void>(sscanf(pszArgs, "%s %f%f%f %d %d", szModel, &vParsed.x, &vParsed.y, &vParsed.z, &nAngle,
           &nUnused));
    Vector vPos(vParsed);
    nUnused = 0;

    ScenePrintf(szModelMdl, 128, "%s.mdl", szModel);
    bload(szModelMdl);

#if KOTOR1  // K1: an unbounded scan for "mainscene"
    Scene** ppScene = Scenes.m_pData;
    Scene* pScene;
    do
        pScene = *ppScene++;
    while (_stricmp("mainscene", pScene->m_szName) != 0);
#else
    Scene* pScene = 0;
    int i = 0;
    do
    {
        if (_stricmp("mainscene", Scenes.m_pData[i]->m_szName) == 0)
        {
            pScene = Scenes.m_pData[i];
            break;
        }
        i++;
    } while (i < Scenes.m_count);
#ifdef K2_FIX_ENGINE_BUGS
    if (pScene == 0)
        return const_cast<char*>("no mainscene");
#else
    if (pScene == 0)
    {
        for (int j = 0; ; j++)
            (void)_stricmp("scene", Scenes.m_pData[j]->m_szName);
    }
#endif
#endif

    pScene->SpawnTile(szModel, vPos, nAngle, nUnused);
    return const_cast<char*>("OK");
}

const char* spawntile(char* pszArgs)
{
    char szModel[128];
    Vector vParsed;
    int nAngle = 0;
    int nUnused = 0;
    szModel[0] = '\0';
    static_cast<void>(sscanf(pszArgs, "%s %f%f%f %d %d", szModel, &vParsed.x, &vParsed.y, &vParsed.z, &nAngle,
           &nUnused));
    Vector vPos(vParsed);
    nUnused = 0;

    Scene* pScene = FindScene(const_cast<char*>("mainscene"));
    if (pScene == 0)
        pScene = FindScene(const_cast<char*>("scene"));

    pScene->SpawnTile(szModel, vPos, nAngle, nUnused);
    return "OK";
}

const char* spawnbordertile(char* pszArgs)
{
    char szModel[128];
    Vector vParsed;
    szModel[0] = '\0';
    int nType = 0;
    static_cast<void>(sscanf(pszArgs, "%s %f%f%f %d", szModel, &vParsed.x, &vParsed.y, &vParsed.z, &nType));
    Vector vPos(vParsed);

    Scenes.m_pData[0]->SpawnTile(szModel, vPos, nType, 1);
    return "OK";
}

const char* deletetile(char* pszArgs)
{
    Vector vPos;
    if (sscanf(pszArgs, "%f %f %f", &vPos.x, &vPos.y, &vPos.z) != 3)
        return "Usage: deletetile <tile position>";

    Scene* pScene;
    if (Scenes.m_count == 1)
    {
        pScene = Scenes.m_pData[0];
    }
    else
    {
        pScene = FindScene(const_cast<char*>("mainscene"));
        if (pScene == 0)
            pScene = FindScene(const_cast<char*>("scene"));
    }

    pScene->DeleteTile(vPos);
    return "OK, deleted any information at the given tile location";
}

const char* settilelights(char* pszArgs)
{
    Vector vPos;
    Vector vLightA;
    Vector vLightB;
    if (sscanf(pszArgs, "%f%f%f %f%f%f %f%f%f", &vPos.x, &vPos.y, &vPos.z, &vLightA.x, &vLightA.y,
               &vLightA.z, &vLightB.x, &vLightB.y, &vLightB.z) != 9)
        return "Usage: settilelights <tile position> <light#1 color> <light#2 color>";

    Scene* pScene;
    if (Scenes.m_count == 1)
    {
        pScene = Scenes.m_pData[0];
    }
    else
    {
        pScene = FindScene(const_cast<char*>("mainscene"));
        if (pScene == 0)
            pScene = FindScene(const_cast<char*>("scene"));
    }

    pScene->SetTileLights(vPos, vLightA, vLightB);
    return "OK, set tile lighting";
}

const char* settileflames(char* pszArgs)
{
    Vector vPos;
    char szFlameA[64];
    char szFlameB[64];
    szFlameA[0] = '\0';
    szFlameB[0] = '\0';
    int nParsed = sscanf(pszArgs, "%f%f%f %s %s", &vPos.x, &vPos.y, &vPos.z, szFlameA, szFlameB);
    if (static_cast<unsigned>(nParsed - 3) <= 2)
    {
        Scene* pScene;
        if (Scenes.m_count == 1)
        {
            pScene = Scenes.m_pData[0];
        }
        else
        {
            pScene = FindScene(const_cast<char*>("mainscene"));
            if (pScene == 0)
                pScene = FindScene(const_cast<char*>("scene"));
        }

        Gob* pObjectA = static_cast<Gob*>(NewCAurObject(szFlameA, const_cast<char*>(""), 0, 0));
        Gob* pObjectB = static_cast<Gob*>(NewCAurObject(szFlameB, const_cast<char*>(""), 0, 0));
        pScene->SetTileFlames(vPos, static_cast<CAurObject*>(pObjectA), static_cast<CAurObject*>(pObjectB));
        return "OK, set tile lighting";
    }
    return "Usage: settileflames <tile position> [flame1] [flame2]";
}


int AurSetGlobalWind(Vector v)
{
    Scene* pScene = FindScene(const_cast<char*>("mainscene"));
    if (pScene == 0)
        pScene = FindScene(const_cast<char*>("scene"));
    if (pScene == 0)
        return 0;

    pScene->m_pWindManager->SetGlobalWind(v);
    return 1;
}

int AurSetGlobalWindVariation(float fA, float fB)
{
    Scene* pScene = FindScene(const_cast<char*>("mainscene"));
    if (pScene == 0)
        pScene = FindScene(const_cast<char*>("scene"));
    if (pScene == 0)
        return 0;

    pScene->m_pWindManager->SetMaximumDeviation(fA * (3.14159265f / 180.0f),
                                                fB * (3.14159265f / 180.0f));
    return 1;
}

const char* setglobalwind(char* pszArgs)
{
    float fX;
    float fY;
    float fZ;
    if (sscanf(pszArgs, "%f %f %f", &fX, &fY, &fZ) != 3)
        return "eh? Need 3 floats as input....";

    Scene* pScene = FindScene(const_cast<char*>("mainscene"));
    if (pScene == 0)
        pScene = FindScene(const_cast<char*>("scene"));
    if (pScene == 0)
        return "eh? Couldn't find scene...";

    pScene->m_pWindManager->SetGlobalWind(Vector(fX, fY, fZ));
    return "Done.";
}

const char* setglobalwindvariation(char* pszArgs)
{
    float fA = 0.0f;
    float fB = 0.0f;
    int nParsed = sscanf(pszArgs, "%f %f", &fA, &fB);
    if (static_cast<unsigned>(nParsed - 1) > 1)
        return "eh? Need 1 or 2 floats as input...";

    Scene* pScene = FindScene(const_cast<char*>("mainscene"));
    if (pScene == 0)
        pScene = FindScene(const_cast<char*>("scene"));
    if (pScene == 0)
        return "eh? Couldn't find scene...";

    pScene->m_pWindManager->SetMaximumDeviation(fA * (3.14159265f / 180.0f),
                                                fB * (3.14159265f / 180.0f));
    return "Done.";
}

const char* setpointwind(char* pszArgs)
{
    float fRadius;
    float fLife;
    float fStrength;
    Vector vPos;
    int nParsed = sscanf(pszArgs, "%f %f %f %f %f %f", &vPos.x, &vPos.y, &vPos.z, &fRadius, &fLife,
                         &fStrength);
    if (static_cast<unsigned>(nParsed - 5) > 1)
        return "Usage: setpointwind position radius duration intensity";

    Scene* pScene = FindScene(const_cast<char*>("mainscene"));
    if (pScene == 0)
        pScene = FindScene(const_cast<char*>("scene"));
    if (pScene == 0)
        return "eh? Couldn't find scene...";

    pScene->m_pWindManager->AddPointSourceWind(vPos, fRadius, fLife, fStrength);
    return "Done.";
}

void AnimateStaticLight(CAurObject* pObject, char* pszName, void* pData);

void ProcessStaticObject(Gob* pGob, Scene* pScene, TileData* pTileData,
                         const Vector& vPosition, const Quaternion& qOrientation)
{
    pTileData->m_pGob = pGob;
    pGob->m_vPosition = vPosition;
    pGob->m_qOrientation = qOrientation;
    pGob->m_pTileData = pTileData;
    pGob->SetScene(pScene);

    pGob->RegisterCallBack(AnimateStaticLight, const_cast<char*>("AnimateStaticLight"), 0, 9999.0f, false);
}

CAurRoom::CAurRoom() : BSPNode(Vector(), 0.0f)
{
#if !KOTOR1
    m_bHasBackgroundGeometry = 0;
#endif
    m_pObject = 0;
}

CAurRoom::CAurRoom(char* pszName) : BSPNode(Vector(), 0.0f)
{
#if KOTOR1  // K1: the name is the trailing member, so the fortified copy is a plain strcpy
    __builtin___strcpy_chk(m_szName, pszName, __builtin_object_size(m_szName, 1));
    m_pObject = 0;
#else
    __builtin___strcpy_chk(m_szName, pszName, sizeof(m_szName));
    m_pObject = 0;
    m_bHasBackgroundGeometry = 0;
#endif
}

void AurTextureRelease(CAurTexture* pTexture);

void DeleteNonGobParts(BSPNode* pNode, void* pData);
void AurClearActiveBins();

Scene::~Scene()
{
// A drain loop, since SetScene(0) removes the gob from this scene, shrinking the list under the
// loop.
    while (m_objects.m_count - 1 >= 0)
        m_objects.m_pData[m_objects.m_count - 1]->SetScene(0);

    m_focusGob.SetScene(0);
    Scenes.Remove(this);

    if (LightManager* pLightManager = m_pLightManager) {
        pLightManager->LightManager::~LightManager();   // a non-virtual destructor; nothing derives from it
        operator delete(pLightManager);
    }
    delete m_pBSPRoot;
    AurClearActiveBins();

    while (m_rooms.m_count > 0) {
        BSPTraverse(m_rooms.m_pData[m_rooms.m_count - 1], DeleteNonGobParts, 0);
        delete m_rooms.m_pData[m_rooms.m_count - 1];
        m_rooms.m_count--;
    }

    delete m_pWindManager;

    if (m_pNormalizationCubeMap) {
        CAurTexture* pTexture = m_pNormalizationCubeMap;
        m_pNormalizationCubeMap = 0;
        AurTextureRelease(pTexture);
    }

    if (m_pFilmNoiseTexture) {
        CAurTexture* pTexture = m_pFilmNoiseTexture;
        m_pFilmNoiseTexture = 0;
        AurTextureRelease(pTexture);
    }

    if (m_pDistortionTexture) {
        CAurTexture* pTexture = m_pDistortionTexture;
        m_pDistortionTexture = 0;
        AurTextureRelease(pTexture);
    }

#if !KOTOR1  // K1 releases only the cube map, film noise and distortion textures
    if (m_pDirtyTexture) {
        CAurTexture* pTexture = m_pDirtyTexture;
        m_pDirtyTexture = 0;
        AurTextureRelease(pTexture);
    }

    if (m_pDirtyTexture2) {
        CAurTexture* pTexture = m_pDirtyTexture2;
        m_pDirtyTexture2 = 0;
        AurTextureRelease(pTexture);
    }

    if (m_pDirtyTexture3) {
        CAurTexture* pTexture = m_pDirtyTexture3;
        m_pDirtyTexture3 = 0;
        AurTextureRelease(pTexture);
    }

    if (m_pLensFlareTexture) {
        CAurTexture* pTexture = m_pLensFlareTexture;
        m_pLensFlareTexture = 0;
        AurTextureRelease(pTexture);
    }

    if (m_pHologramTexture) {
        CAurTexture* pTexture = m_pHologramTexture;
        m_pHologramTexture = 0;
        AurTextureRelease(pTexture);
    }

    meshbuckets.clear();
#else
// K1: the static bucket array is emptied (counts only, nothing freed).
    for (int i = 0; i < 5000; i++)
        meshbuckets[i].m_nCount = 0;
#endif
}

CAurRoom::~CAurRoom()
{
    delete m_pObject;

    for (int i = 0; i < m_objects.m_count; i++)
        delete m_objects.m_pData[i];
}

MdlNode* findnode(MdlNode* pNode, char* pszName)
{
    if (strcmp(pNode->TranslateNodeName(), pszName) == 0)
        return pNode;

    for (int i = 0; i < pNode->m_listChildren.m_count; i++) {
        MdlNode* pFound = findnode(pNode->m_listChildren.m_pData[i], pszName);
        if (pFound)
            return pFound;
    }

    return 0;
}

void SetPartTextureRotate(Part* pPart, int nAngle)
{
    PartTriMesh* pMesh = pPart->AsPartTriMesh();
    if (pMesh != 0 && static_cast<MdlNodeTriMesh*>(pMesh->m_pNode)->m_bRotateTexture != 0)
        pMesh->RotateTexture(static_cast<float>(nAngle) * 90.0f);

    for (int i = 0; i < pPart->m_nChildren; i++)
        SetPartTextureRotate(pPart->m_paChildren[i], nAngle);
}

void CAurScene::ComputeStaticLighting()
{
    CAurScene* pSelf = this;
    if (pSelf != 0)
        static_cast<Scene*>(this)->PostProcessLS();  // CAurScene viewed as its Scene implementation  // TODO: idiom (this-cast to the Scene interface)
}

Model* FindModel(char* pszName);
void   AurTextureSetModeStatic();
void   AurTextureSetModeDynamic();
Part*  CreateInstanceTree(MdlNode* pNode, Gob* pGob);
void   PartInitMaterial(Gob* pGob, Part* pPart, char* pszName);
#if KOTOR1  // K1: the model streams are SDL_RWops
struct SDL_RWops;
CAurObject*   NewCAurObject(char* pszResRef, char* pszParam2, SDL_RWops* pFile, SDL_RWops* pAux);
#else
CAurObject*   NewCAurObject(char* pszResRef, char* pszParam2, FILE* pFile, FILE* pAux);
#endif
void   ExpandReferenceParts(Part* pPart);
void   CollapsePartTree(Gob* pGob, Part* pPart, List<Part*>& parts, Vector vPos, Quaternion qOrient,
                        Scene* pScene, CAurRoom* pRoom);
void   ComputePartBBox(PartTriMesh* pPart);

Gob* Scene::SpawnRoom(char* pszName, const Vector& vPos, const Quaternion& qOrient)
{
    AurTextureSetModeStatic();

    Model* pModel = FindModel(pszName);
    if (pModel == 0)
        return 0;

    pModel->m_nFlags |= Model::kModelRoom;

    CAurRoom* pRoom = new CAurRoom(pszName);
    nodedata* pNodeData = new nodedata();

    Vector vMin(1000000.0f, 1000000.0f, 1000000.0f);
    Vector vMax(-1000000.0f, -1000000.0f, -1000000.0f);

    char szNode[64];
    __strcpy_chk(szNode, pModel->m_szName, sizeof(szNode));
    __strcat_chk(szNode, "a", sizeof(szNode));

    Gob* pGob = 0;
    Part* pPart = 0;
    if (findnode(pModel->m_pMdlNode, szNode) != 0)
    {
        pGob = static_cast<Gob*>(NewCAurObject(pszName, const_cast<char*>(""), 0, 0));
        pGob->m_vPosition = vPos;
        pGob->m_qOrientation = qOrient;
        pPart = pGob->m_pPart;
        pGob->SetScene(this);
        pRoom->m_pObject = static_cast<CAurObject*>(pGob);  // Gob derives from CAurObject in the engine, but not in this tree  // idiom-exempt: opener 21 debt (Gob:CAurObject not yet derived)
    }
    else
    {
        pPart = CreateInstanceTree(pModel->m_pMdlNode, 0);
        char szTag[32];
        ScenePrintf(szTag, sizeof(szTag), "%s", pszName);
        PartInitMaterial(0, pPart, szTag);
        pGob = 0;
    }

    List<Part*> parts;
    CollapsePartTree(pGob, pPart, parts, vPos, qOrient, this, pRoom);

    pRoom->m_bboxMin = Vector(100000.0f, 100000.0f, 100000.0f);
    pRoom->m_bboxMax = Vector(-100000.0f, -100000.0f, -100000.0f);

    List<PartAABB*> aabbs;
    bool bHasAABB = false;
    for (int64_t i = 0; i < parts.m_count; i++)
    {
        PartTriMesh* pMesh    = parts.m_pData[i]->AsPartTriMesh();
        PartLight*   pLight   = parts.m_pData[i]->AsPartLight();
        PartEmitter* pEmitter = parts.m_pData[i]->AsPartEmitter();
        PartAABB*    pBox     = static_cast<PartAABB*>(parts.m_pData[i]->AsPartAABB());

        if (pMesh != 0)
        {
            if (pMesh->AsPartAnimMesh() == 0)
            {
                ComputePartBBox(pMesh);
                MdlNodeTriMesh* pTriMeshNode = pMesh->m_pNode->AsMdlNodeTriMesh();
                if (pTriMeshNode->m_bIsBackgroundGeometry)
                {
#if !KOTOR1  // K1 keeps only background meshes out of the room box
                    pRoom->m_bHasBackgroundGeometry = 1;
#endif
                }
                else
                {
                    pRoom->m_bboxMax = VectorMax(pRoom->m_bboxMax, pMesh->m_vBBoxMax);
                    pRoom->m_bboxMin = VectorMin(pRoom->m_bboxMin, pMesh->m_vBBoxMin);
                }
            }

            pModel->AddRef(static_cast<Gob*>(0));
            pNodeData->m_meshes.Add(pMesh);
            if (pBox != 0)
            {
                pNodeData->m_pPartAABB = pBox;
                aabbs.Add(pBox);
                vMax = VectorMax(vMax, pBox->m_vBBoxMax);
                vMin = VectorMin(vMin, pBox->m_vBBoxMin);
                bHasAABB = true;
            }
        }
        else if (pLight != 0)
        {
            m_pLightManager->AddLight(pLight);
            pNodeData->m_lights.Add(static_cast<Part*>(pLight));
            if (pLight->m_nIsDynamic != 0)
                SceneBSPAddPart(this, pLight, 0);
            pModel->AddRef(static_cast<Gob*>(0));
        }
        else if (pEmitter != 0)
        {
            pNodeData->m_emitters.Add(pEmitter);
            pModel->AddRef(static_cast<Gob*>(0));
        }
        else
        {
            delete parts.m_pData[i];
        }
    }

    if (aabbs.m_count > 0)
    {
        List<List<CAurTriangleBin*> > bins;
        InitGrassBins(vMin, vMax, bins);
        for (int64_t i = 0; i < aabbs.m_count; i++)
            AddFacesToBins(aabbs.m_pData[i], vMin, bins);
        for (int64_t i = 0; i < bins.m_count; i++)
        {
            for (int64_t j = 0; j < bins.m_pData[i].m_count; j++)
            {
                CAurTriangleBin* pBin = bins.m_pData[i].m_pData[j];
                if (pBin == 0)
                    continue;
                pBin->FinishGeometry();
                pNodeData->m_bins.Add(pBin);
            }
        }
    }

    pRoom->m_pNodeData = pNodeData;
    m_rooms.Add(pRoom);
    AurTextureSetModeDynamic();

    if (!bHasAABB)
    {
        pRoom->m_bboxMin = Vector(100000.0f, 100000.0f, 100000.0f);
        pRoom->m_bboxMax = Vector(-100000.0f, -100000.0f, -100000.0f);
    }

    return pGob;
}

Gob* SpawnPartsForTile(char* pszName, const Vector& vPos, const Quaternion& qOrient,
                       int nTextureAngle, int nMode, List<Part*>& parts)
{
    Model* pModel = FindModel(pszName);
    if (pModel == 0)
        return 0;

    AurTextureSetModeStatic();

    Gob* pGob = 0;
    Part* pPart = 0;
    bool bHavePart = false;
    if (nMode == 0) {
        char szNode[64];
        __strcpy_chk(szNode, pModel->m_szName, sizeof(szNode));
        __strcat_chk(szNode, "a", sizeof(szNode));
        if (findnode(pModel->m_pMdlNode, szNode) != 0) {
            pGob = static_cast<Gob*>(NewCAurObject(pszName, const_cast<char*>(""), 0, 0));
            pPart = pGob->m_pPart;
            bHavePart = true;
        }
    }

    if (!bHavePart) {
        pPart = CreateInstanceTree(pModel->m_pMdlNode, 0);
        char szTag[32];
        ScenePrintf(szTag, sizeof(szTag), "(%d,%d)", static_cast<int>(__builtin_floorf(vPos.x)),
                    static_cast<int>(__builtin_floorf(vPos.y)));
        PartInitMaterial(0, pPart, szTag);
        pGob = 0;
    }

    SetPartTextureRotate(pPart, nTextureAngle);
    ExpandReferenceParts(pPart);
    CollapsePartTree(pGob, pPart, parts, vPos, qOrient, 0, 0);
    AurTextureSetModeDynamic();

    return pGob;
}

void CollapsePartTree(Gob* pGob, Part* pPart, List<Part*>& parts, Vector vPos, Quaternion qOrient,
                      Scene* pScene, CAurRoom* pRoom)
{
    char szName[64];
    Vector vSave;
    Quaternion qSave;

    if (pGob != 0)
    {
        __strcpy_chk(szName, pGob->m_pModel->m_szName, sizeof(szName));
        __strcat_chk(szName, "a", sizeof(szName));
        vSave = pPart->m_vLocalPosition;
        qSave = pPart->m_qLocalOrientation;
    }

    pPart->m_vLocalPosition = vPos + RotateVector(pPart->m_vLocalPosition, qOrient);
    pPart->m_qLocalOrientation = qOrient * pPart->m_qLocalOrientation;

    parts.Add(pPart);

    Part* pFound = 0;
    for (int64_t i = 0; i < parts.m_count; i++)
    {
        Part* pNode = parts.m_pData[i];

        PartReference* pRef = pNode->AsPartReference();
        if (pRef != 0 && pRoom != 0 && pScene != 0)
        {
            Gob* pObj = static_cast<Gob*>(NewCAurObject(static_cast<MdlNodeReference*>(pRef->m_pNode)->m_szRefModel, const_cast<char*>(""), 0, 0));
            if (pObj != 0)
            {
                pObj->SetPosition(pNode->m_vLocalPosition);
                pObj->SetOrientation(pNode->m_qLocalOrientation);
                float fSpeed = ((rand_wincompatible() % 100) - 50.0f) / 1000.0f + 1.0f;
                float fPhase = (rand_wincompatible() % 100) / 100.0f;
                pObj->PlayAnimation(const_cast<char*>("default"), fSpeed, 0, fPhase);
                pObj->SetScene(pScene);
                pRoom->m_objects.Add(static_cast<CAurObject*>(pObj));  // Gob derives from CAurObject in the engine, but not in this tree  // idiom-exempt: opener 21 debt (Gob:CAurObject not yet derived)
            }
        }

        for (int64_t j = 0; j < pNode->m_nChildren; j++)
        {
            Part* pChild = pNode->m_paChildren[j];
            if (pGob != 0 && _stricmp(pChild->m_pNode->TranslateNodeName(), szName) == 0)
            {
                pFound = pChild;
            }
            else
            {
                pChild->m_vLocalPosition = pNode->m_vLocalPosition +
                                           RotateVector(pChild->m_vLocalPosition, pNode->m_qLocalOrientation);
                pChild->m_qLocalOrientation = pNode->m_qLocalOrientation * pChild->m_qLocalOrientation;
                parts.Add(pChild);
            }
        }

        pNode->m_nChildren = 0;
        pNode->m_pParent = 0;
        if (pNode->m_pGob != 0 && pNode->m_pGob->m_pPart == pNode)
            pNode->m_pGob->m_pPart = 0;
        pNode->m_pGob = 0;
    }

    if (pGob != 0)
    {
        parts.Remove(pPart);
        List<Part*>& rChildren = *reinterpret_cast<List<Part*>*>(&pPart->m_paChildren);  // raw-offset: Part's child array is a List<Part*> the destructor frees by hand
        rChildren.Add(pFound);
#if KOTOR1  // K1: the collapsed root's sphere memo is stale
        pPart->GetMinimumSphere_cache_GetData()->bValid = false;
#endif
        pPart->m_pGob = pGob;
        pGob->m_pPart = pPart;
        pPart->m_vLocalPosition = vSave;
        pPart->m_qLocalOrientation = qSave;
// The result is discarded; the call exists for the lazy-bounds pass it runs.
        pGob->GetMinimumBoundingBox();
        pGob->SetProcessFlag();
    }
}

char* firstword(char* pszLine);
void  Parse(char* pszLine, int& nValue);

int Scene::SpawnRooms(char* pszName)
{
    void* pRes = AurResGet(pszName, const_cast<char*>(".lyt"), 0, true);
    if (!pRes)
        return 0;

    char szName[128];
    int nRoomCount = 0;
    int nTrackCount = 0;
    Vector vPos;

    char* pszLine;
    while ((pszLine = AurResGetNextLine()) != 0)
    {
        while (*pszLine == ' ' || *pszLine == '\t')
            pszLine++;

        if (nRoomCount > 0)
        {
            static_cast<void>(sscanf(pszLine, "%s %f%f%f", szName, &vPos.x, &vPos.y, &vPos.z));
            Quaternion qRot;
            SpawnRoom(szName, vPos, qRot);
            nRoomCount--;
        }

        if (_stricmp("roomcount", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("roomcount", sizeof("roomcount")), nRoomCount);

        if (nTrackCount > 0)
        {
            static_cast<void>(sscanf(pszLine, "%s %f%f%f", szName, &vPos.x, &vPos.y, &vPos.z));
            Gob* pObj = static_cast<Gob*>(NewCAurObject(szName, szName, 0, 0));
            if (pObj)
            {
                pObj->AttachToScene(this);
                pObj->SetPosition(vPos);
            }
            nTrackCount--;
        }

        if (_stricmp("trackcount", firstword(pszLine)) == 0)
            Parse(pszLine + __strlen_chk("trackcount", sizeof("trackcount")), nTrackCount);
    }

    AurResFree(pRes, 0);
    LoadVisibility(pszName);
    return m_rooms.m_count;
}

void ComputePartBBox(PartTriMesh* pPart)
{
    TriMeshView* pT = reinterpret_cast<TriMeshView*>(pPart->m_pVertexPrimitive->m_pNode);

    void* pool = GLRender::LockPool(pT->m_pTree->AsModel()->m_nPool, 0, 0, 8);

    char* pVertices = static_cast<char*>(pool) + static_cast<unsigned int>(pT->m_nGeomBase);

    Vector vMax(-100000.0f, -100000.0f, -100000.0f);
    Vector vMin(100000.0f, 100000.0f, 100000.0f);

    for (int i = 0; i < static_cast<short>(pT->m_nVertexCount); i++) {
        float* pVertex = reinterpret_cast<float*>(pVertices + pT->m_nVertexStride * i + pT->m_nVertexOfs);

        Vector vWorld = RotateVectorDirect(*reinterpret_cast<Vector*>(pVertex), pPart->m_qLocalOrientation);
        vWorld.x += pPart->m_vLocalPosition.x;
        vWorld.y += pPart->m_vLocalPosition.y;
        vWorld.z += pPart->m_vLocalPosition.z;
        vWorld.x = Round(vWorld.x, 0.001f);
        vWorld.y = Round(vWorld.y, 0.001f);
        vWorld.z = Round(vWorld.z, 0.001f);

        vMin = VectorMin(vMin, vWorld);
        vMax = VectorMax(vMax, vWorld);
    }

    GLRender::UnlockPool(pT->m_pTree->AsModel()->m_nPool);

    pPart->m_vBBoxMin = vMin;
    pPart->m_vBBoxMax = vMax;
}

int GobOrderCmp(void const* pA, void const* pB)
{
    Gob* pGobA = *static_cast<Gob* const*>(pA);
    Gob* pGobB = *static_cast<Gob* const*>(pB);

    if (pGobA->m_fColorShiftAmount < pGobB->m_fColorShiftAmount)
        return -1;
    if (pGobA->m_fColorShiftAmount > pGobB->m_fColorShiftAmount)
        return 1;

    unsigned short nFlagsA = pGobA->m_pModel->m_nFlags;
    unsigned short nFlagsB = pGobB->m_pModel->m_nFlags;

    if (!(nFlagsB & Model::kModelEffect) && (nFlagsA & Model::kModelEffect))
        return -1;
    if (!(nFlagsA & Model::kModelEffect) && (nFlagsB & Model::kModelEffect))
        return 1;

    if (newsorting != 0)
    {
        if (pGobA->m_bHasTransparency && !pGobB->m_bHasTransparency)
            return -1;
        if (!pGobA->m_bHasTransparency && pGobB->m_bHasTransparency)
            return 1;
#if defined(K2_QOL_IMPROVEMENTS) && !KOTOR1
// Two see-through gobs only blend correctly back-to-front: the flag keys draw every door
// before every placeable, whatever the distance.
        if (pGobA->m_bHasTransparency && pGobB->m_bHasTransparency && !g_ForceSight)
            return (pGobA->m_fViewDistance > pGobB->m_fViewDistance) ? 1 : -1;
#endif
    }

#if KOTOR1  // K1: no force-sight order; bit 2 sorts last, bits 4 and 3 sort first
    if ((nFlagsA & Model::kModelCharacter) && !(nFlagsB & Model::kModelCharacter))
        return 1;
    if (!(nFlagsA & Model::kModelCharacter) && (nFlagsB & Model::kModelCharacter))
        return -1;
    if ((nFlagsA & Model::kModelLightsaber) && !(nFlagsB & Model::kModelLightsaber))
        return -1;
    if (!(nFlagsA & Model::kModelLightsaber) && (nFlagsB & Model::kModelLightsaber))
        return 1;
    if ((nFlagsA & Model::kModelDoor) && !(nFlagsB & Model::kModelDoor))
        return -1;
    if (!(nFlagsA & Model::kModelDoor) && (nFlagsB & Model::kModelDoor))
        return 1;
#else
    if (g_ForceSight)
    {
        if ((nFlagsA & Model::kModelDoor) && !(nFlagsB & Model::kModelDoor))
            return -1;
        if (!(nFlagsA & Model::kModelDoor) && (nFlagsB & Model::kModelDoor))
            return 1;
        if ((nFlagsA & Model::kModelCharacter) && !(nFlagsB & Model::kModelCharacter))
            return 1;
        if (!(nFlagsA & Model::kModelCharacter) && (nFlagsB & Model::kModelCharacter))
            return -1;
    }
    else
    {
        if ((nFlagsA & Model::kModelDoor) && !(nFlagsB & Model::kModelDoor))
            return 1;
        if (!(nFlagsA & Model::kModelDoor) && (nFlagsB & Model::kModelDoor))
            return -1;
        if ((nFlagsA & Model::kModelCharacter) && !(nFlagsB & Model::kModelCharacter))
            return -1;
        if (!(nFlagsA & Model::kModelCharacter) && (nFlagsB & Model::kModelCharacter))
            return 1;
    }
#endif

    return (pGobA->m_fViewDistance > pGobB->m_fViewDistance) ? 1 : -1;
}

int EmitterOrderCmp(void const* pA, void const* pB)
{
    PartEmitter* pEmitterA = *static_cast<PartEmitter* const*>(pA);
    PartEmitter* pEmitterB = *static_cast<PartEmitter* const*>(pB);

    if (pEmitterA->m_pGob != 0 && pEmitterB->m_pGob != 0 && pEmitterA->m_pGob == pEmitterB->m_pGob)
    {
        MdlNodeEmitter* pNodeA = pEmitterA->m_pNode->AsMdlNodeEmitter();
        MdlNodeEmitter* pNodeB = pEmitterB->m_pNode->AsMdlNodeEmitter();

        return static_cast<int>(pNodeA->m_nRenderOrder) - static_cast<int>(pNodeB->m_nRenderOrder);
    }

    int nBlendA = pEmitterA->m_nBlendMode;
    int nBlendB = pEmitterB->m_nBlendMode;

    if (nBlendA == 2)
    {
        if (nBlendB != 2)
            return 1;
    }
    else if (nBlendB == 2)
    {
        return -1;
    }

    Vector vPosA;
    if (pEmitterA->m_pGob != 0)
        vPosA = pEmitterA->m_pGob->m_vPosition;
    else
        vPosA = pEmitterA->m_vLocalPosition;

    Vector vPosB;
    if (pEmitterB->m_pGob != 0)
        vPosB = pEmitterB->m_pGob->m_vPosition;
    else
        vPosB = pEmitterB->m_vLocalPosition;

    float fDistA = magnitude(CurrentCamera->m_vPosition - vPosA);
    float fDistB = magnitude(CurrentCamera->m_vPosition - vPosB);

    float fDeltaA = fDistA - fDistB;
    float fDeltaB = fDistB - fDistA;

    if ((fDeltaA < fDeltaB ? fDeltaB : fDeltaA) < 0.01f)
        return (pEmitterA <= pEmitterB) ? 1 : -1;

    return (fDistA > fDistB) ? -1 : 1;
}

void ExpandReferenceParts(Part* pPart)
{
    PartReference* pRef = pPart->AsPartReference();
    if (pRef != 0)
    {
        MdlNodeReference* pRefNode = static_cast<MdlNodeReference*>(pRef->m_pNode);
        Model* pModel = FindModel(pRefNode->m_szRefModel);

        List<Part*>& rChildren = *reinterpret_cast<List<Part*>*>(&pRef->m_paChildren);  // raw-offset: Part's child array is a List<Part*> the destructor frees by hand
        rChildren.Add(CreateInstanceTree(pModel->m_pMdlNode, 0));
#if KOTOR1  // K1: the reference part's sphere memo is stale
        pRef->GetMinimumSphere_cache_GetData()->bValid = false;
#endif

        char szTag[32];
        ScenePrintf(szTag, sizeof(szTag), "%s", pRefNode->TranslateNodeName());
        PartInitMaterial(0, pPart, szTag);
    }

    for (int64_t i = 0; i < pPart->m_nChildren; i++)
        ExpandReferenceParts(pPart->m_paChildren[i]);
}

bool ProcessTilePart(TileData* pTileData, Part* pPart, char* pszName)
{
    char szBuf[64];

    __strcpy_chk(szBuf, pszName, sizeof(szBuf));
    __strcat_chk(szBuf, "ml1", sizeof(szBuf));
    if (_stricmp(pPart->m_pNode->TranslateNodeName(), szBuf) == 0)
    {
        pTileData->m_paLights[0] = pPart->AsPartLight();
        return 1;
    }

    __strcpy_chk(szBuf, pszName, sizeof(szBuf));
    __strcat_chk(szBuf, "ml2", sizeof(szBuf));
    if (_stricmp(pPart->m_pNode->TranslateNodeName(), szBuf) == 0)
    {
        pTileData->m_paLights[1] = pPart->AsPartLight();
        return 1;
    }

    __strcpy_chk(szBuf, pszName, sizeof(szBuf));
    __strcat_chk(szBuf, "sl1", sizeof(szBuf));
    if (_stricmp(pPart->m_pNode->TranslateNodeName(), szBuf) == 0)
    {
        pTileData->m_paFlameParts[0] = pPart;
        return 1;
    }

    __strcpy_chk(szBuf, pszName, sizeof(szBuf));
    __strcat_chk(szBuf, "sl2", sizeof(szBuf));
    if (_stricmp(pPart->m_pNode->TranslateNodeName(), szBuf) == 0)
    {
        pTileData->m_paFlameParts[1] = pPart;
        return true;
    }

    return false;
}

void ProcessStaticParts(List<Part*>& rParts, Scene* pScene, TileData* pTileData, char* pszName, int nFlag)
{
    K2_UNUSED(pScene);
    if (rParts.m_count <= 0)
        return;

    List<Part*>& rTileParts = pTileData->m_aParts;

    for (int i = 0; i < rParts.m_count; i++)
    {
        Part* pPart = rParts.m_pData[i];

        PartTriMesh* pTriMesh = pPart->AsPartTriMesh();
        bool bProcessed = ProcessTilePart(pTileData, pPart, pszName);

        bool bKeep;

        if (pTriMesh != 0)
        {
            pTriMesh->m_pTileData = pTileData;

            if (pTriMesh->AsPartAnimMesh() == 0)
                ComputePartBBox(pTriMesh);

            bKeep = (nFlag == 0) || (pTriMesh->AsPartAABB() == 0);
        }
        else
        {
            PartEmitter* pEmitter = pPart->AsPartEmitter();

            if (nFlag != 0)
            {
                bKeep = false;
            }
            else
            {
                bKeep = bProcessed || (pEmitter != 0) || (pPart->AsPartLight() != 0);

                if (pEmitter != 0)
                    pEmitter->m_pTileData = pTileData;
            }
        }

        if (!bKeep)
        {
            delete pPart;
            continue;
        }

        rTileParts.Add(pPart);
    }
}


static inline List<TileData*>& TileNeighbors(TileData* pTileData)
{
    return pTileData->m_aNeighbors;
}
static inline List<PartTriMesh*>& TileNeighborMeshes(TileData* pTileData)
{
    return pTileData->m_aNeighborMeshes;
}
static inline List<PartTriMesh*>& TileOwnMeshes(TileData* pTileData)
{
    return pTileData->m_aOwnMeshes;
}

void SceneBSPAddTileData(Scene* pScene, TileData* pTileData, Vector vTile);

Gob* Scene::SpawnTile(char* pszName, const Vector& vPos, int nAngle, int nMode)
{
    Quaternion qOrient = YawPitchRoll(static_cast<float>(nAngle) * 90.0f, 0.0f, 0.0f);

    List<Part*> parts;
    Gob* pGob = SpawnPartsForTile(pszName, vPos, qOrient, nAngle, nMode, parts);
    int i;
    int j;

    TileData* pTileData = new TileData();
    pTileData->m_vPosition = vPos;
    pTileData->m_nMode = nMode;

    if (pGob != 0)
        ProcessStaticObject(pGob, this, pTileData, vPos, qOrient);

    ProcessStaticParts(parts, this, pTileData, pszName, nMode);

    if (pTileData->m_aParts.m_count > 0)
    {
        i = 0;
        while (i < pTileData->m_aParts.m_count && pTileData->m_aParts[i]->AsPartTriMesh() == 0)
            i++;

        if (pTileData->m_aParts[i]->AsPartTriMesh() != 0)
        {
            pTileData->m_vBBoxMin = pTileData->m_aParts[i]->AsPartTriMesh()->m_vBBoxMin;
            pTileData->m_vBBoxMax = pTileData->m_aParts[i]->AsPartTriMesh()->m_vBBoxMax;

            for (j = i + 1; j < pTileData->m_aParts.m_count; j++)
            {
                PartTriMesh* pMesh = pTileData->m_aParts[j]->AsPartTriMesh();
                if (pMesh == 0)
                    continue;

                pTileData->m_vBBoxMin = VectorMin(pTileData->m_vBBoxMin, pMesh->m_vBBoxMin);
                pTileData->m_vBBoxMax = VectorMax(pTileData->m_vBBoxMax, pMesh->m_vBBoxMax);
            }
        }
    }

    SceneBSPAddTileData(this, pTileData, vPos);

    for (i = -1; i <= 1; i += 2)
    {
        Vector vOffset(static_cast<float>(i) * Scene::cellsize, 0.0f, 0.0f);
        TileData* pNeighbor = SceneBSPGetTileData(this, vPos + vOffset);

        if (pNeighbor != 0 && pNeighbor != pTileData)
        {
            TileNeighbors(pTileData).Add(pNeighbor);
            TileNeighbors(pNeighbor).Add(pTileData);

            for (int64_t k = 0; k < TileOwnMeshes(pTileData).m_count; k++)
                TileNeighborMeshes(pNeighbor).Add(TileOwnMeshes(pTileData).m_pData[k]);

            for (int64_t k = 0; k < TileOwnMeshes(pNeighbor).m_count; k++)
                TileNeighborMeshes(pTileData).Add(TileOwnMeshes(pNeighbor).m_pData[k]);
        }
    }

    for (i = -1; i <= 1; i += 2)
    {
        Vector vOffset(0.0f, static_cast<float>(i) * Scene::cellsize, 0.0f);
        TileData* pNeighbor = SceneBSPGetTileData(this, vPos + vOffset);

        if (pNeighbor != 0 && pNeighbor != pTileData)
        {
            TileNeighbors(pTileData).Add(pNeighbor);
            TileNeighbors(pNeighbor).Add(pTileData);

            for (int64_t k = 0; k < TileOwnMeshes(pTileData).m_count; k++)
                TileNeighborMeshes(pNeighbor).Add(TileOwnMeshes(pTileData).m_pData[k]);

            for (int64_t k = 0; k < TileOwnMeshes(pNeighbor).m_count; k++)
                TileNeighborMeshes(pTileData).Add(TileOwnMeshes(pNeighbor).m_pData[k]);
        }
    }

    return pGob;
}

void CAurScene::SetAmbient(Vector color)
{
    static_cast<Scene*>(this)->m_ambient = color;  // CAurScene viewed as its Scene implementation  // TODO: idiom (this-cast to the Scene interface)
}

void CAurRoomGetBBox(CAurRoom* pRoom, Vector* pMin, Vector* pMax) {
    *pMin = pRoom->m_bboxMin;
    *pMax = pRoom->m_bboxMax;
}
