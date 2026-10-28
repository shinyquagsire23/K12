#include "k2stdint.h"
#include "port/aslglapi.h"
#if KOTOR1
#define K1_AURORA_LIST  // K1's AURORA library (List.h grows from 8)
#endif
#include "aurscene.h"
#include "AURORA/aurmodel.h"
#include "aurscenemanager.h"
#include "bsp.h"
#include "List.h"
#include "aurgrass.h"
#include "auremitter.h"
#include "aurmaterial.h"
#include "aurmodelrender.h"
#include "aurtrigger.h"
#include "math3d.h"
#include "Quaternion.h"
#include "Plane.h"
#include "aurtexcontrol.h"
#include "aurcamera.h"
#include "aurlightmanager.h"
#include "include/aurbehavior.h"
#include "AURORA/aurbehavior.h"
#include <map>
#include "aurprimitive.h"
#include "aurglcore.h"

int hitcheck_static_bsp_parts = 1;

int countculledpart;
int countculledshadows;
int countculledbeams;
int countbackshadows;
int countculledgobs;
int countlights;
CHitInfo* CurrentHitInfo;
int hitcheck_static_trimesh_parts;
int hack_nobsp_hit_ground_zero;
int enablenonbspgroundhit;
int counthitcheckscenebspnode;

class PartLight;

Scene* FindScene(char* pszName);

Model* FindModel(char* pszName);
#include "port/libcstring.h"
#include "port/libcstdlib.h"
#include "port/libcmath.h"

int SceneBSPAddPart(Scene* pScene, Part* pPart, Model* pModel);
void SceneBSPRemovePart(Scene* pScene, Part* pPart);

void LightNodeTile(BSPNode* pNode, void* pData);
void LightNodeMeshes(BSPNode* pNode, void* pData);

void LightPartTriMesh(PartTriMesh* pPart, List<PartLight*> lights, Scene* pScene);

extern CHitInfo* CurrentHitInfo;  // the hit test in flight
extern int counthitcheckscenebspnode;  // counts the leaf tests this frame
extern int hack_nobsp_hit_ground_zero;  // set when the no-BSP ground fallback fired
extern int enablenonbspgroundhit;  // gate for that fallback
extern int hitcheck_static_trimesh_parts;  // gate for the mesh half of the sweep

static int NodeContainsGob(BSPNode* pNode, void* pData)
{
    if (pNode == 0)
        return 0;
    return pNode->m_pNodeData->m_gobs.Contains(static_cast<Gob*>(pData));
}

void LightBins(BSPNode* pNode, void* pData);

List<BSPNode*> staticlightbucket;


List<Gob*>  gobbucket;
List<Gob*>  blurbucket;
List<Gob*>  g_listForcePushBucket;
List<Gob*>  g_listLightsaberBucket;
List<Gob*>  g_listHologramBucket;
List<PartTriMesh*> g_listBackgroundBucket;
List<Gob*>  fadinginbucket;
List<Gob*>  fadingoutbucket;
List<Part*> staticshadowbucket;
List<Part*> staticbeambucket;
List<PartEmitter*> emitterbucket;
List<PartTriMesh*> meshshadowbucket;
List<PartTrigger*> triggerbucket;
List<CAurTriangleBin*> binbucket;

List<CAurTriangleBin*> g_pActiveBins;
List<CAurTriangleBin*> g_pOldActiveBins;

#if KOTOR1  // K1 indexes fixed per-texture arrays
Bucket meshbuckets[5000];
Bucket fadebuckets[5000];
Bucket capbuckets[5000];
#ifdef K2_FIX_ENGINE_BUGS
// A texture name past the arrays goes in bucket 0, whose parts each bind their own texture.
static inline int K1BucketTID(int nTID)
{
    return static_cast<unsigned>(nTID) < 5000u ? nTID : 0;
}
#else
#define K1BucketTID(n) (n)
#endif
#else
std::map<unsigned int, Bucket> meshbuckets;
std::map<unsigned int, Bucket> fadebuckets;
std::map<unsigned int, Bucket> capbuckets;
#endif

int countbinbucket;
int countactivebins;
int countmodels;


int buildstaticshadows = 1;
float minshadowdim = 1.0f;
float minshadowarea = 1.0f;
int shadowcull;

extern Camera* CurrentCamera;
float cameraepsilon = 0.300000012f;

int enablevisibilitygraph = 1;

extern List<Model*> Models;

extern float DeltaT;

__attribute__((visibility("hidden"))) Vector vOutsideRef;

__attribute__((visibility("hidden"))) float fOutsideClampRadius = 40.0f;
__attribute__((visibility("hidden"))) float fOutsideDistThreshold = 15.0f;
__attribute__((visibility("hidden"))) float fOutsideDistRatio = 0.05f;

__attribute__((visibility("hidden"))) float fTriMeshDistThreshold = 15.0f;
__attribute__((visibility("hidden"))) float fTriMeshMinigameOffRatio = 0.0045f;
__attribute__((visibility("hidden"))) float fTriMeshMinigameOnRatio = 0.0115f;

__attribute__((visibility("hidden"))) float fGobOutRadiusLimit = 2.5f;
__attribute__((visibility("hidden"))) float fGobOutSmallDistThreshold = 15.0f;
__attribute__((visibility("hidden"))) float fGobOutSmallDistRatio = 0.035f;
#if KOTOR1
__attribute__((visibility("hidden"))) float fGobOutNoBBDistThreshold = 15.0f;
#else
__attribute__((visibility("hidden"))) float fGobOutNoBBDistThreshold = 80.0f;
#endif
__attribute__((visibility("hidden"))) float fGobOutNoBBOnRatio = 0.015f;
__attribute__((visibility("hidden"))) float fGobOutBigDistThreshold = 15.0f;
__attribute__((visibility("hidden"))) float fGobOutNoBBOffRatio;

int enablebboxtest;

#if defined(K2_MOBILE_OPT) && defined(K2_QOL_IMPROVEMENTS)
// The cull floor; nothing is culled nearer than this while the fog still hides it.
static float s_fFogCullFloor = 0.0f;
#endif

extern int quaternionidentityoptimization;

int crap;
int GetMiniGamePlayingType();

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

#if KOTOR1
void nodedata::ClearMovingObjects()
{
    m_movingObjects.m_count = 0;
}

void nodedata::ClearMovingLights()
{
    m_movingLights.m_count = 0;
}

void nodedata::ClearMovingEmitters()
{
    m_movingEmitters.m_count = 0;
}

void nodedata::AddMovingObject(Gob* pGob)
{
    m_movingObjects.Add(pGob);
}

void nodedata::AddMovingLight(PartLight* pLight)
{
    m_movingLights.Add(pLight);
}

void nodedata::AddMovingEmitter(PartEmitter* pEmitter)
{
    m_movingEmitters.Add(pEmitter);
}

List<Gob*>* nodedata::GetMovingObjects()
{
    return &m_movingObjects;
}

List<PartLight*>* nodedata::GetMovingLights()
{
    return &m_movingLights;
}

List<PartEmitter*>* nodedata::GetMovingEmitters()
{
    return &m_movingEmitters;
}
#endif

nodedata::nodedata()
{
    __builtin_memset(static_cast<void*>(this), 0, sizeof(nodedata));
}

// m_pTileData is the only owned pointer; the lists free themselves.
nodedata::~nodedata()
{
    delete m_pTileData;
}

// BSP-traversal callback that allocates a leaf's payload.
void InitNodeData(BSPNode* pNode, void* pData)
{
    K2_UNUSED(pData);
    pNode->m_pNodeData = new nodedata();
}

// The leaf's tile record for a point, or null when the scene has no BSP.
TileData* SceneBSPGetTileData(Scene* pScene, const Vector& vTile)
{
    BSPNode* pNode = pScene->m_pBSPRoot;
    if (pNode == 0)
        return 0;
    BSPNode* pLeaf = BSPFitSphere(pNode, vTile, 0.0f);
    return pLeaf->m_pNodeData->m_pTileData;
}

void UpdateStaticLighting(Scene* pScene, PartLight* pLight);
// A BSPTraverse callback that forwards the static light to its owning scene.
void AnimateStaticLight(CAurObject* pObject, char* pszName, void* pLight)
{
    K2_UNUSED(pszName);
    UpdateStaticLighting(static_cast<Gob*>(pObject)->m_pScene, static_cast<PartLight*>(pLight));
}

// File a tile's parts in the leaf its origin falls in and add them to the BSP.
void SceneBSPAddTileData(Scene* pScene, TileData* pTileData, Vector vTile)
{
    if (pScene->m_pBSPRoot == 0)
        return;

    nodedata* pNodeData = BSPFitSphere(pScene->m_pBSPRoot, vTile, 0.0f)->m_pNodeData;
    pNodeData->m_pTileData = pTileData;

    if (pTileData->m_aParts.m_count > 0) {
        Part* pPart = pTileData->m_aParts.m_pData[0];
        SceneBSPAddPart(pScene, pPart, pPart->m_pNode->m_pTree->AsModel());
    }

    for (int64_t i = 1; i < pTileData->m_aParts.m_count; i++) {
        Part* pPart = pNodeData->m_pTileData->m_aParts.m_pData[i];
        SceneBSPAddPart(pScene, pPart, pPart->m_pNode->m_pTree->AsModel());
    }
}

// Unregister a tile's parts and drop its Gob.
void SceneBSPRemoveTiledata(Scene* pScene, const Vector& vTile)
{
    if (pScene->m_pBSPRoot == 0)
        return;

    nodedata* pNodeData = BSPFitSphere(pScene->m_pBSPRoot, vTile, 0.0f)->m_pNodeData;
    if (pNodeData->m_pTileData == 0)
        return;

    for (int64_t i = pNodeData->m_pTileData->m_aParts.m_count - 1; i != -1; i--) {
        Part* pPart = pNodeData->m_pTileData->m_aParts.m_pData[i];
        SceneBSPRemovePart(pScene, pPart);
        Model* pModel = pPart->m_pNode->m_pTree->AsModel();
        if (pModel != 0)
            pModel->DelRef(pModel);
        if (pPart != 0)
            delete pPart;
        pNodeData->m_pTileData->m_aParts.m_count--;
    }

    if (pNodeData->m_pTileData->m_pGob != 0) {
        delete pNodeData->m_pTileData->m_pGob;
        pNodeData->m_pTileData->m_pGob = 0;
    }
    delete pNodeData->m_pTileData;
    pNodeData->m_pTileData = 0;
}

static int NodeContainsLight(BSPNode* pNode, void* pData)
{
    if (pNode == 0)
        return 0;
    return pNode->m_pNodeData->m_lights.Contains(static_cast<Part*>(pData));
}

static int NodeContainsEmitter(BSPNode* pNode, void* pData)
{
    if (pNode == 0)
        return 0;
    return pNode->m_pNodeData->m_emitters.Contains(static_cast<PartEmitter*>(pData));
}

static int NodeContainsShadow(BSPNode* pNode, void* pData)
{
    if (pNode == 0)
        return 0;
    return pNode->m_pNodeData->m_shadows.Contains(static_cast<Part*>(pData));
}

static int NodeContainsAABB(BSPNode* pNode, void* pData)
{
    if (pNode == 0)
        return 0;
    return pNode->m_pNodeData->m_pPartAABB == static_cast<PartAABB*>(pData);
}

static int NodeContainsMesh(BSPNode* pNode, void* pData)
{
    if (pNode == 0)
        return 0;
    return pNode->m_pNodeData->m_meshes.Contains(static_cast<PartTriMesh*>(pData));
}

static int NodeContainsTrigger(BSPNode* pNode, void* pData)
{
    if (pNode == 0)
        return 0;
    return pNode->m_pNodeData->m_parts.Contains(static_cast<PartTrigger*>(pData));
}

static int NodeContainsOtherPart(BSPNode* pNode, void* pData)
{
    if (pNode == 0)
        return 0;
    return pNode->m_pNodeData->m_otherParts.Contains(static_cast<Part*>(pData));
}

// File a part in the leaf it occupies and in the side list its role belongs to.
int SceneBSPAddPart(Scene* pScene, Part* pPart, Model* pModel)
{
    if (pScene->m_pBSPRoot == 0)
        return 0;

    Vector vWorldPosition;
    Quaternion qWorldOrientation;
    pPart->GetWorld(vWorldPosition, qWorldOrientation);

    if (PartLight* pLight = pPart->AsPartLight()) {
        nodedata* pNodeData = BSPFitSphere(pScene->m_pBSPRoot, vWorldPosition, pLight->m_fLightRadius)->m_pNodeData;
        pNodeData->m_lights.AddUnique(pLight);
        pScene->m_pLightManager->AddLight(pLight);
    } else if (PartAABB* pAABB = static_cast<PartAABB*>(pPart->AsPartAABB())) {
        nodedata* pNodeData = BSPFitSphere(pScene->m_pBSPRoot, vWorldPosition, 0.0f)->m_pNodeData;
        pNodeData->m_pPartAABB = pAABB;
    } else if (PartTriMesh* pMesh = pPart->AsPartTriMesh()) {
        Vector vMin = pMesh->m_vBBoxMin;
        Vector vMax = pMesh->m_vBBoxMax;
        nodedata* pNodeData = BSPFitBox(pScene->m_pBSPRoot, vMin, vMax)->m_pNodeData;
        pNodeData->m_meshes.Add(pMesh);
    } else if (PartProjection* pProjection = pPart->AsPartProjection()) {
        Vector vMin = pProjection->m_vBBoxMin;
        Vector vMax = pProjection->m_vBBoxMax;
        nodedata* pNodeData = BSPFitBox(pScene->m_pBSPRoot, vMin, vMax)->m_pNodeData;
        if (pProjection->AsPartShadow())
            pNodeData->m_shadows.Add(static_cast<Part*>(pProjection->AsPartShadow()));
        else
            pNodeData->m_beams.Add(static_cast<Part*>(pProjection->AsPartBeam()));
    } else if (PartEmitter* pEmitter = pPart->AsPartEmitter()) {
        nodedata* pNodeData = BSPFitSphere(pScene->m_pBSPRoot, vWorldPosition, pEmitter->m_fRadius)->m_pNodeData;
        pNodeData->m_emitters.AddUnique(pEmitter);
        if (pEmitter->m_pNode->AsMdlNodeEmitter()->IsAffectedByWind()) {
            if (pScene->m_pWindManager != 0) {
                pEmitter->m_spWindManager = pScene->m_pWindManager;
                pScene->m_pWindManager->RegisterWindyEmitter(pEmitter);
            }
        }
    } else if (PartTrigger* pTrigger = static_cast<PartTrigger*>(pPart->AsPartTrigger())) {
        Vector vMin = pTrigger->m_vBBoxMin;
        Vector vMax = pTrigger->m_vBBoxMax;
        nodedata* pNodeData = BSPFitBox(pScene->m_pBSPRoot, vMin, vMax)->m_pNodeData;
        pNodeData->m_parts.AddUnique(pTrigger);
    } else {
        nodedata* pNodeData = BSPFitSphere(pScene->m_pBSPRoot, vWorldPosition, 0.0f)->m_pNodeData;
        pNodeData->m_otherParts.AddUnique(pPart);
    }

    if (pModel != 0)
        pModel->AddRef(pModel);
    return 1;
}

// Un-file a part, finding its leaf through its role.
void SceneBSPRemovePart(Scene* pScene, Part* pPart)
{
    if (pScene->m_pBSPRoot == 0)
        return;

    Vector vWorldPosition;
    Quaternion qWorldOrientation;
    pPart->GetWorld(vWorldPosition, qWorldOrientation);

    if (PartLight* pLight = pPart->AsPartLight()) {
        BSPNode* pNode = BSPSearch(pScene->m_pBSPRoot, vWorldPosition, NodeContainsLight, pLight);
        nodedata* pNodeData = pNode->m_pNodeData;
        if (pNodeData->m_lights.Contains(pLight))
            pNodeData->m_lights.Remove(pLight);
        // Only the light arm reports to the LightManager.
        pScene->m_pLightManager->RemoveLight(pLight, false);
    } else if (PartEmitter* pEmitter = pPart->AsPartEmitter()) {
        BSPNode* pNode = BSPSearch(pScene->m_pBSPRoot, vWorldPosition, NodeContainsEmitter, pEmitter);
        nodedata* pNodeData = pNode->m_pNodeData;
        if (pNodeData->m_emitters.Contains(pEmitter))
            pNodeData->m_emitters.Remove(pEmitter);
    } else if (PartProjection* pProjection = pPart->AsPartProjection()) {
        BSPNode* pNode = BSPSearch(pScene->m_pBSPRoot, vWorldPosition, NodeContainsShadow, pProjection);
        nodedata* pNodeData = pNode->m_pNodeData;
        // A projection is either a shadow or a beam; one search covers both shared lists.
        if (void* pShadow = pProjection->AsPartShadow()) {
            if (pNodeData->m_shadows.Contains(static_cast<Part*>(pShadow)))
                pNodeData->m_shadows.Remove(static_cast<Part*>(pShadow));
        } else if (void* pBeam = pProjection->AsPartBeam()) {
            if (pNodeData->m_beams.Contains(static_cast<Part*>(pBeam)))
                pNodeData->m_beams.Remove(static_cast<Part*>(pBeam));
        }
    } else if (PartAABB* pAABB = static_cast<PartAABB*>(pPart->AsPartAABB())) {
        BSPNode* pNode = BSPSearch(pScene->m_pBSPRoot, vWorldPosition, NodeContainsAABB, pAABB);
        nodedata* pNodeData = pNode->m_pNodeData;
        if (pNodeData->m_pPartAABB == pAABB)
            pNodeData->m_pPartAABB = 0;
    } else if (PartTriMesh* pMesh = pPart->AsPartTriMesh()) {
        BSPNode* pNode = BSPSearch(pScene->m_pBSPRoot, vWorldPosition, NodeContainsMesh, pMesh);
        nodedata* pNodeData = pNode->m_pNodeData;
        if (pNodeData->m_meshes.Contains(pMesh))
            pNodeData->m_meshes.Remove(pMesh);
    } else if (void* pTrigger = pPart->AsPartTrigger()) {
        BSPNode* pNode = BSPSearch(pScene->m_pBSPRoot, vWorldPosition, NodeContainsTrigger, pTrigger);
        nodedata* pNodeData = pNode->m_pNodeData;
        if (pNodeData->m_parts.Contains(static_cast<PartTrigger*>(pTrigger)))
            pNodeData->m_parts.Remove(static_cast<PartTrigger*>(pTrigger));
    } else {
        BSPNode* pNode = BSPSearch(pScene->m_pBSPRoot, vWorldPosition, NodeContainsOtherPart, pPart);
        nodedata* pNodeData = pNode->m_pNodeData;
        if (pNodeData->m_otherParts.Contains(pPart))
            pNodeData->m_otherParts.Remove(pPart);
    }
}

// File a Gob's bounding sphere in its leaf.
void SceneBSPAddGob(Scene* pScene, Gob* pGob)
{
    if (pScene->m_pBSPRoot == 0)
        return;

    nodedata* pNodeData = BSPFitSphere(pScene->m_pBSPRoot, pGob->m_vPosition, pGob->m_pModel->m_fRadius)->m_pNodeData;
    if (pNodeData->m_gobs.Contains(pGob) == 0)
        pNodeData->m_gobs.Add(pGob);
}

// Un-file a Gob, searching for the leaf that holds it.
void SceneBSPRemoveGob(Scene* pScene, Gob* pGob)
{
    BSPNode* pRoot = pScene->m_pBSPRoot;
    if (pRoot == 0)
        return;

    BSPNode* pNode = BSPSearch(pRoot, pGob->m_vPosition, NodeContainsGob, pGob);
    if (pNode == 0)
        return;

    nodedata* pNodeData = pNode->m_pNodeData;
    if (pNodeData->m_gobs.Contains(pGob))
        pNodeData->m_gobs.Remove(pGob);
}

// Collect the textures a part and its child subtree draw with.
void SceneBSPGetObjectTextures(Part* pPart, List<CAurTexture*>& out)
{
    if (pPart == 0)
        return;

    PartTriMesh* pMesh = pPart->AsPartTriMesh();
    if (pMesh != 0)
        out.Add(pMesh->m_pMaterial->m_spr0.m_pObject);

    for (int64_t i = 0; i < pPart->m_nChildren; i++)
        SceneBSPGetObjectTextures(pPart->m_paChildren[i], out);
}

// The tile-level half of the texture gather.
void SceneBSPGetTileTextures(Scene* pScene, const Vector& vTile, List<CAurTexture*>& out)
{
    BSPNode* pRoot = pScene->m_pBSPRoot;
    if (pRoot == 0) {
        out.m_count = 0;
        return;
    }

    TileData* pTileData = BSPFitSphere(pRoot, vTile, 0.0f)->m_pNodeData->m_pTileData;
    out.m_count = 0;
    if (pTileData == 0)
        return;

    for (int64_t i = 0; i < pTileData->m_aParts.m_count; i++) {
        PartTriMesh* pMesh = pTileData->m_aParts.m_pData[i]->AsPartTriMesh();
        if (pMesh != 0)
            out.Add(pMesh->m_pMaterial->m_spr0.m_pObject);
    }

    if (pTileData->m_pGob != 0)
        SceneBSPGetObjectTextures(pTileData->m_pGob->m_pPart, out);
}

// File a finished triangle bin in its leaf; false when the scene has no BSP.
bool SceneBSPAddBin(Scene* pScene, CAurTriangleBin* pBin)
{
    if (pScene->m_pBSPRoot == 0)
        return false;

    pBin->FinishGeometry();
    nodedata* pNodeData = BSPFitSphere(pScene->m_pBSPRoot, pBin->m_vCenter, pBin->m_fRadius)->m_pNodeData;
    pNodeData->m_bins.Add(pBin);
    return true;
}

// Tests a segment against the static geometry of one leaf: triggers, meshes, then the box.
int HitPartOnBSPNode(BSPNode* pNode, Vector vStart, Vector vEnd, Vector* pvHit)
{
    counthitcheckscenebspnode++;

    nodedata* pNodeData = pNode->m_pNodeData;
    if (pNodeData == 0)
        return 0;

    int bHit = 0;

    for (int64_t i = 0; i < pNodeData->m_parts.m_count; i++) {
        PartTrigger* pTrigger = pNodeData->m_parts.m_pData[i];
        if (BoxIntersect(vStart, vEnd, pTrigger->m_vBBoxMin, pTrigger->m_vBBoxMax)
            && pTrigger->HitCheck(vStart, vEnd, &vEnd, CurrentHitInfo, 1.0f) != 0) {
            bHit = 1;
            if (pvHit != 0)
                *pvHit = vEnd;
        }
    }

    if (hitcheck_static_trimesh_parts != 0) {
        for (int64_t i = 0; i < pNodeData->m_meshes.m_count; i++) {
            if (BoxIntersect(vStart, vEnd, pNodeData->m_meshes.m_pData[i]->m_vBBoxMin,
                             pNodeData->m_meshes.m_pData[i]->m_vBBoxMax)
                && pNodeData->m_meshes.m_pData[i]->HitCheck(vStart, vEnd, &vEnd, CurrentHitInfo, 1.0f) != 0) {
                bHit = 1;
                if (pvHit != 0)
                    *pvHit = vEnd;
            }
        }
    }

    hack_nobsp_hit_ground_zero = 0;
    if (hitcheck_static_bsp_parts != 0) {
        PartAABB* pAABB = pNodeData->m_pPartAABB;
        if (pAABB != 0)
            bHit |= pAABB->HitCheck(vStart, vEnd, pvHit, CurrentHitInfo, 1.0f) != 0;
        else if (enablenonbspgroundhit != 0 && pNode->m_pFront == 0 && pNode->m_pBack == 0
                 && vEnd.z < 0.0f && vStart.z > 0.0f) {
            if (pvHit != 0) {
                Vector vNormal(0.0f, 0.0f, 1.0f);
                *pvHit = planelineintersection(vNormal, 0.0f, vStart, vEnd);
            }
            bHit = 1;
            hack_nobsp_hit_ground_zero = 1;
        }
    }

    return bHit;
}

// Drop both grass-pass bin lists.
void AurClearActiveBins()
{
    g_pActiveBins.m_count = 0;
    g_pOldActiveBins.m_count = 0;
}

// True when every x/y corner of the box lies above the plane.
bool TwoDBoxAbovePlane(const Vector& vCorner0, const Vector& vCorner1, const Plane& plane)
{
    return !(vCorner0.x * plane.a + vCorner0.y * plane.b + plane.d <= 0.0f)
        && !(vCorner1.x * plane.a + vCorner1.y * plane.b + plane.d <= 0.0f)
        && !(vCorner0.x * plane.a + vCorner1.y * plane.b + plane.d <= 0.0f)
        && !(vCorner1.x * plane.a + vCorner0.y * plane.b + plane.d <= 0.0f);
}

// Distance pre-reject, then a box sweep of the planes from a persistent cursor.
int PartOutside(PartTriMesh* pPart, List<Plane>& planes)
{
#ifdef K2_MOBILE_OPT   // the mobile port's size-over-distance cull; the PC lineage has only the planes
    MdlNodeTriMesh* pNode = pPart->m_pNode->AsMdlNodeTriMesh();

    Vector vWorldPosition = pPart->m_vLocalPosition
        + RotateVector(pNode->m_vCentroid, pPart->m_qLocalOrientation);
    float fDist = magnitude(vOutsideRef - vWorldPosition) - pNode->m_fRadius;

#ifdef K2_QOL_IMPROVEMENTS
    if (fDist > fTriMeshDistThreshold && fDist > s_fFogCullFloor) {
#else
    if (fDist > fTriMeshDistThreshold) {
#endif
        float fRatio = pNode->m_fRadius / fDist;
        float fLimit = GetMiniGamePlayingType() != 0 ? fTriMeshMinigameOffRatio
                                                     : fTriMeshMinigameOnRatio;
        if (fRatio < fLimit)
            return true;
    }
#endif

    static int lastClipped = 0;

    for (int i = 0; i < planes.m_count; i++) {
        int nPlane = (lastClipped + i) % planes.m_count;
        if (BoxAbovePlane(pPart->m_vBBoxMin, pPart->m_vBBoxMax, planes.m_pData[nPlane]) > 0.0f) {
            lastClipped = (lastClipped + i) % planes.m_count;
            return true;
        }
    }

    return false;
}

// Sphere sweep of the planes; caches the largest per-plane distance on the light.
int PartOutside(PartLight* pLight, List<Plane>& planes)
{
    float fMaxDist = -10000.0f;

    for (int64_t i = 0; i < planes.m_count; i++) {
        float fDist = SphereAbovePlane(pLight->m_vWorldPosition, pLight->m_fLightRadius,
                                       planes.m_pData[i]);
        if (fMaxDist < fDist)
            fMaxDist = fDist;
        if (fDist > 0.0f)
            return true;
    }

    pLight->m_fCullDistance = fMaxDist + pLight->m_fLightRadius;
    return false;
}

// Distance pre-reject, then a sphere sweep of the planes.
int PartOutside(PartEmitter* pPart, List<Plane>& planes)
{
    Vector vWorldPosition;
    Quaternion qWorldOrientation;
    pPart->GetWorld(vWorldPosition, qWorldOrientation);

#ifdef K2_MOBILE_OPT   // the mobile port's size-over-distance cull; the PC lineage has only the planes
    float fMin = __builtin_fminf(pPart->m_fRadius, fOutsideClampRadius);
    float fDist = magnitude(vOutsideRef - vWorldPosition);
#ifdef K2_QOL_IMPROVEMENTS
    if (fDist - fMin > fOutsideDistThreshold && fDist - fMin > s_fFogCullFloor) {
#else
    if (fDist - fMin > fOutsideDistThreshold) {
#endif
        if (fMin / (fDist - fMin) < fOutsideDistRatio)
            return true;
    }
#endif

    for (int64_t i = 0; i < planes.m_count; i++) {
        if (SphereAbovePlane(vWorldPosition, pPart->m_fRadius, planes.m_pData[i]) > 0.0f)
            return true;
    }

    return false;
}

// Box test against each plane, from a persistent cursor.
int PartOutside(PartProjection* pPart, List<Plane>& planes)
{
    static int lastClipped = 0;

    int nRet = 0;
    for (int i = 0; i < planes.m_count; i++) {
        int nPlane = (lastClipped + i) % planes.m_count;
        if (BoxAbovePlane(pPart->m_vBBoxMin, pPart->m_vBBoxMax, planes.m_pData[nPlane]) > 0.0f) {
            lastClipped = i;
            nRet = 1;
            break;
        }
    }
    return nRet;
}

// The projection's box test over the trigger's own box.
int PartOutside(PartTrigger* pPart, List<Plane>& planes)
{
    static int lastClipped = 0;

    int nRet = 0;
    for (int i = 0; i < planes.m_count; i++) {
        int nPlane = (lastClipped + i) % planes.m_count;
        if (BoxAbovePlane(pPart->m_vBBoxMin, pPart->m_vBBoxMax, planes.m_pData[nPlane]) > 0.0f) {
            lastClipped = i;
            nRet = 1;
            break;
        }
    }
    return nRet;
}

// Space-delimited tokenizer for the debug console; returns the token start, or null.
char* GetToken(char** ppszCursor)
{
    char* pszToken = *ppszCursor;

    while (**ppszCursor == ' ')
        (*ppszCursor)++;

    char c = **ppszCursor;
    if (c == 0)
        return 0;

    while ((c == ' ') != (c != 0)) {
        (*ppszCursor)++;
        c = **ppszCursor;
    }

    if (c == ' ') {
        *(*ppszCursor) = 0;
        (*ppszCursor)++;
    } else {
        (*ppszCursor)++;
    }

    return pszToken;
}

// Console handler for "setbsphere <model> <radius>".
const char* setbsphere(char* pszArgs)
{
    char* pszCursor = pszArgs;
    char c;

    while ((c = *pszCursor) == ' ')
        pszCursor++;
    if (c == 0)
        return "0";

    pszCursor++;
    do
        c = *pszCursor++;
    while ((c == ' ') != (c != 0));
    if (c == ' ')
        pszCursor[-1] = 0;

    if (pszArgs != 0) {
        Model* pModel = FindModel(pszArgs);

        char* pszValue = pszCursor;
        while ((c = *pszValue) == ' ')
            pszValue++;
        if (c != 0) {
            pszValue++;
            do
                c = *pszValue++;
            while ((c == ' ') != (c != 0));
            if (c == ' ')
                pszValue[-1] = 0;

            pModel->m_fRadius = static_cast<float>(atof(pszCursor));
            return "1";
        }
    }

    return "0";
}

// The scene-graph node's outside test, over its own bounding sphere.
int GobOutside(Gob* pGob, List<Plane>& planes)
{
    if (pGob->IsInViewVolume())
        return 1;

#ifdef K2_MOBILE_OPT   // the mobile port's size-over-distance culls; the PC lineage has only the planes
    Vector vGobPos = pGob->m_vPosition;
    Vector vRef = vOutsideRef;
    float fDist = magnitude(vRef - (vGobPos
                       + RotateVector(pGob->m_vMinSphereCenter, pGob->m_qOrientation)))
                - pGob->m_fMinSphereRadius;

#ifdef K2_QOL_IMPROVEMENTS
    if (fDist <= s_fFogCullFloor) {
        // Still inside the fog, so only the frustum test may drop it.
    } else
#endif
    if (pGob->m_fMinSphereRadius < fGobOutRadiusLimit) {
        if (pGob->m_cHasSkin == 0) {
#if KOTOR1  // K1 uses one ratio and a 15.0 threshold, with no mini-game case
            if (fDist > fGobOutNoBBDistThreshold
                && pGob->m_fMinSphereRadius / fDist < fGobOutNoBBOnRatio)
                return 1;
#else
            if (fDist > fGobOutNoBBDistThreshold) {
                float fRatio = pGob->m_fMinSphereRadius / fDist;
                float fLimit = GetMiniGamePlayingType() != 0 ? fGobOutNoBBOffRatio
                                                            : fGobOutNoBBOnRatio;
                if (fRatio < fLimit)
                    return 1;
            }
#endif
        } else {
            if (fDist > fGobOutSmallDistThreshold
                && pGob->m_fMinSphereRadius / fDist < fGobOutSmallDistRatio)
                return 1;
        }
    } else {
        if (fDist > fGobOutBigDistThreshold) {
            float fRatio = pGob->m_fMinSphereRadius / fDist;
            float fLimit = GetMiniGamePlayingType() != 0 ? fTriMeshMinigameOffRatio
                                                        : fTriMeshMinigameOnRatio;
            if (fRatio < fLimit)
                return 1;
        }
    }
#endif

    for (int64_t i = 0; i < planes.m_count; i++) {
        if (enablebboxtest)
            continue;

        Vector vSphere = pGob->m_vPosition
            + RotateVector(pGob->m_vMinSphereCenter, pGob->m_qOrientation);
        if (SphereAbovePlane(vSphere, pGob->m_fMinSphereRadius, planes.m_pData[i]) > 0.0f)
            return 1;
    }

    return 0;
}

// Sphere sweep for a triangle bin.
bool BinOutside(CAurTriangleBin* pBin, List<Plane>& planes)
{
    for (int64_t i = 0; i < planes.m_count; i++) {
        if (SphereAbovePlane(pBin->m_vCenter, pBin->m_fRadius, planes.m_pData[i]) > 0.0f)
            return true;
    }

    return false;
}

// Sort a finished mesh into this frame's draw buckets.
void AddPartToMeshBuckets(PartTriMesh* pPart)
{
    PartDanglyMesh* pDanglyMesh = pPart->AsPartDanglyMesh();
    if (pDanglyMesh != 0)
        pDanglyMesh->Animate(DeltaT);

    MdlNodeTriMesh* pTriMesh = pPart->m_pNode->AsMdlNodeTriMesh();
    if (pTriMesh->m_bIsBackgroundGeometry) {
        g_listBackgroundBucket.Add(pPart);
        return;
    }

#if KOTOR1
    meshbuckets[K1BucketTID(pPart->m_pMaterial->GetTextureTID())].Add(pPart);
#else
    meshbuckets[pPart->m_pMaterial->GetTextureTID()].Add(pPart);
#endif
    meshshadowbucket.Add(pPart);
}

// File a mesh under its material's texture id in the cap-bucket map.
void AddPartToCapBuckets(PartTriMesh* pPart)
{
#if KOTOR1
    capbuckets[K1BucketTID(pPart->m_pMaterial->GetTextureTID())].Add(pPart);
#else
    capbuckets[pPart->m_pMaterial->GetTextureTID()].Add(pPart);
#endif
}

// Reset the per-frame render buckets and counters.
void ClearBuckets(Scene* pScene)
{
    K2_UNUSED(pScene);
#if KOTOR1  // K1 clears the per-texture array slots up to the highest texture id, with no hologram bucket
    int nMaxTexID = AurTextureGetMaxTexID();
#else
    AurTextureGetMaxTexID();
#endif

    gobbucket.m_count = 0;
    g_listForcePushBucket.m_count = 0;
    g_listLightsaberBucket.m_count = 0;
    blurbucket.m_count = 0;
    emitterbucket.m_count = 0;
#if KOTOR1
    for (int i = 0; i <= nMaxTexID; i++)
        capbuckets[i].m_nCount = 0;
    for (int i = 0; i <= nMaxTexID; i++) {
        meshbuckets[i].m_nCount = 0;
        fadebuckets[i].m_nCount = 0;
    }
#else
    g_listHologramBucket.m_count = 0;

    meshbuckets.clear();
    fadebuckets.clear();
    capbuckets.clear();
#endif

    g_listBackgroundBucket.m_count = 0;
    staticshadowbucket.m_count = 0;
    staticbeambucket.m_count = 0;
    meshshadowbucket.m_count = 0;
    triggerbucket.m_count = 0;
    staticlightbucket.m_count = 0;
    binbucket.m_count = 0;
    fadingoutbucket.m_count = 0;
    fadinginbucket.m_count = 0;

    g_pOldActiveBins = g_pActiveBins;
    g_pActiveBins.m_count = 0;

    countbinbucket = 0;
    countactivebins = 0;
    countmodels = Models.m_count;
}

// Queue every leaf the light's sphere overlaps for a static-light refresh.
void UpdateStaticLighting(Scene* pScene, PartLight* pLight)
{
    if (pLight->m_nIsDynamic != 0)
        return;

    List<BSPNode*> nodes;
    if (pScene->m_pBSPRoot != 0) {
        BSPSphereCheck(pScene->m_pBSPRoot, pLight->m_vWorldPosition, pLight->m_fLightRadius, nodes);
        for (int64_t i = 0; i < nodes.m_count; i++)
            staticlightbucket.AddUnique(nodes.m_pData[i]);
    }
}

// Re-light every queued leaf, then sweep the tree for lights that joined.
void DoStaticLightBuckets(Scene* pScene)
{
    for (int64_t i = 0; i < staticlightbucket.m_count; i++)
        LightNodeTile(staticlightbucket.m_pData[i], pScene);

    if (staticlightbucket.m_count != 0 && pScene->m_pBSPRoot != 0)
        BSPTraverse(pScene->m_pBSPRoot, LightBins, pScene);
}

// A console handler that enables geometry fade for the main scene.
const char* setfadetoalways(char* pszArgs)
{
    K2_UNUSED(pszArgs);
    FindScene(const_cast<char*>("mainscene"));
    Scene::geomFadeEnabled = CAurScene::ALWAYS_FADE;
    return "SUCCESS";
}

// A BSPTraverse callback that refreshes the colour of every triangle bin in a leaf.
void LightBins(BSPNode* pNode, void* pData)
{
    K2_UNUSED(pData);
    nodedata* pNodeData = pNode->m_pNodeData;
    for (int64_t i = 0; i < pNodeData->m_bins.m_count; i++)
        pNodeData->m_bins.m_pData[i]->ReloadColor();
}

// LightNodeTile over the leaf's mesh list plus its bounding-box part.
void LightNodeMeshes(BSPNode* pNode, void* pData)
{
    nodedata* pNodeData = pNode->m_pNodeData;

    List<PartLight*> lights;
    lights = static_cast<Scene*>(pData)->m_pLightManager->m_staticLights;

    List<Part*> meshes;
    for (int64_t i = 0; i < pNodeData->m_meshes.m_count; i++)
        meshes.Add(pNodeData->m_meshes.m_pData[i]);
    if (pNodeData->m_pPartAABB != 0)
        meshes.Add(pNodeData->m_pPartAABB);

    List<PartLight*> bucket;
    for (int64_t m = 0; m < meshes.m_count; m++) {
        PartTriMesh* pMesh = static_cast<PartTriMesh*>(meshes.m_pData[m]);
        MdlNodeTriMesh* pTriMesh = pMesh->m_pNode->AsMdlNodeTriMesh();
        Vector v = RotateVector(pTriMesh->m_vCentroid, pMesh->m_qLocalOrientation)
                   + pMesh->m_vLocalPosition;

        bucket.m_count = 0;
        for (int64_t j = 0; j < lights.m_count; j++) {
            PartLight* pLight = lights.m_pData[j];
            if (pLight->m_nIsDynamic != 0)
                continue;
            if (equal(pLight->m_vLightColour, Vector(0.0f, 0.0f, 0.0f)))
                continue;

            if (magnitude(pLight->m_vWorldPosition - v) < pLight->m_fLightRadius + pTriMesh->m_fRadius)
                bucket.Add(pLight);
        }

        LightPartTriMesh(pMesh, bucket, static_cast<Scene*>(pData));
    }
}

// Runs the per-leaf static-light pass, filtering the scene's lights per mesh, then colouring it.
void LightNodeTile(BSPNode* pNode, void* pData)
{
    nodedata* pNodeData = pNode->m_pNodeData;
    if (pNodeData->m_pTileData == 0)
        return;

    List<PartLight*> lights;
    lights = static_cast<Scene*>(pData)->m_pLightManager->m_staticLights;

    List<PartLight*> bucket;
    for (int64_t i = 0; i < pNodeData->m_pTileData->m_aParts.m_count; i++) {
        MdlNodeTriMesh* pTriMesh;
        PartTriMesh* pMesh = pNodeData->m_pTileData->m_aParts.m_pData[i]->AsPartTriMesh();
        if (pMesh == 0)
            continue;

        pTriMesh = pMesh->m_pNode->AsMdlNodeTriMesh();
        Vector v = RotateVector(pTriMesh->m_vCentroid, pMesh->m_qLocalOrientation)
                   + pMesh->m_vLocalPosition;

        bucket.m_count = 0;
        for (int64_t j = 0; j < lights.m_count; j++) {
            PartLight* pLight = lights.m_pData[j];
            if (pLight->m_nIsDynamic != 0)
                continue;
            if (equal(pLight->m_vLightColour, Vector(0.0f, 0.0f, 0.0f)))
                continue;

            if (magnitude(pLight->m_vWorldPosition - v) < pLight->m_fLightRadius + pTriMesh->m_fRadius)
                bucket.Add(pLight);
        }

        LightPartTriMesh(pMesh, bucket, static_cast<Scene*>(pData));
    }
}

// Delete every entry in the leaf's shadow list.
void RemoveShadows(BSPNode* pNode, void* pData)
{
    K2_UNUSED(pData);
    nodedata* pNodeData = pNode->m_pNodeData;
    for (int64_t i = pNodeData->m_shadows.m_count - 1; i != -1; i--) {
        if (pNodeData->m_shadows.m_pData[i] != 0)
            delete pNodeData->m_shadows.m_pData[i];
        pNodeData->m_shadows.m_count--;
    }
}

// RemoveShadows over the beam list.
void RemoveBeams(BSPNode* pNode, void* pData)
{
    K2_UNUSED(pData);
    nodedata* pNodeData = pNode->m_pNodeData;
    for (int64_t i = pNodeData->m_beams.m_count - 1; i != -1; i--) {
        if (pNodeData->m_beams.m_pData[i] != 0)
            delete pNodeData->m_beams.m_pData[i];
        pNodeData->m_beams.m_count--;
    }
}

// Build a beam or shadow part for every shadow-casting mesh and light pair.
void BSPCreateStaticProjections(Scene* pScene)
{
    if (buildstaticshadows == 0)
        return;

    GLRender::EnableClientState(Aurora::kClientStateVertex);
    GLRender::DisableClientState(Aurora::kClientStateColor);
    GLRender::DisableClientState(Aurora::kClientStateTexCoord);
    GLRender::DisableClientState(Aurora::kClientStateNormal);

    List<PartLight*> shadowLights;
    shadowLights = pScene->m_pLightManager->GetShadowLights(1);

    List<BSPNode*> nodeList;
    nodeList.Add(pScene->m_pBSPRoot);
    while (nodeList.m_count > 0) {
        BSPNode* pNode = nodeList.m_pData[nodeList.m_count - 1];
        nodeList.m_count--;
        if (pNode->m_pFront != 0)
            nodeList.Add(pNode->m_pFront);
        if (pNode->m_pBack != 0)
            nodeList.Add(pNode->m_pBack);

        nodedata* pNodeData = pNode->m_pNodeData;
        if ((pNodeData->m_pTileData != 0 && pNodeData->m_pTileData->m_nMode != 0) ||
            shadowLights.m_count < 1)
            continue;

        for (int64_t i = 0; i < shadowLights.m_count; i++) {
            PartLight* pLight = shadowLights.m_pData[i];
            if (pLight->m_fLightRadius < 1000.0f)
                continue;
            if (pNodeData->m_meshes.m_count < 1)
                continue;

            for (int64_t j = 0; j < pNodeData->m_meshes.m_count; j++) {
                PartTriMesh* pMesh = pNodeData->m_meshes.m_pData[j];
                MdlNodeTriMesh* pMdlNode = static_cast<MdlNodeTriMesh*>(pMesh->m_pNode);
                if (!pMdlNode->m_bShadow && !pMdlNode->m_bBeaming)
                    continue;

                if (shadowcull == 1) {
                    Vector vSize = pMdlNode->m_vMax - pMdlNode->m_vMin;
                    float fMax = vSize.y;
                    if (fMax < vSize.z)
                        fMax = vSize.z;
                    if (!(vSize.x < fMax))
                        fMax = vSize.x;
                    if (fMax < minshadowdim)
                        continue;
                } else if (shadowcull == 2) {
                    Vector vSize = pMdlNode->m_vMax - pMdlNode->m_vMin;
                    float fA = vSize.x, fB = vSize.y, fC = vSize.z, fT;
                    if (fB < fA) { fT = fA; fA = fB; fB = fT; }
                    if (fC < fB) { fT = fB; fB = fC; fC = fT; }
                    if (fB < fA) { fT = fA; fA = fB; fB = fT; }
                    if (fC * fB < minshadowarea)
                        continue;
                }

                if (pMesh->m_pTileData != 0 &&
                    pMesh->m_vBBoxMax.z <= pMesh->m_pTileData->m_vPosition.z)
                    continue;

                if (!pMesh->WithinRange(pLight->m_vWorldPosition, pLight->m_fLightRadius))
                    continue;

                if (pMdlNode->m_bBeaming) {
                    PartBeam* pBeam = new PartBeam(pMesh, pLight);
                    pBeam->Draw(false);
                    if (pBeam->m_nQuadCount == 0)
                        delete pBeam;
                    else
                        SceneBSPAddPart(pScene, pBeam, 0);
                } else {
                    PartShadow* pShadow = new PartShadow(pMesh, pLight);
                    pShadow->Draw(false);
                    if (pShadow->m_nQuadCount == 0)
                        delete pShadow;
                    else
                        SceneBSPAddPart(pScene, pShadow, 0);
                }
            }
        }
    }
}

// Destroy a leaf's payload and the parts left in its lists.
void DeleteNonGobParts(BSPNode* pNode, void* pData)
{
    K2_UNUSED(pData);
    nodedata* pNodeData = pNode->m_pNodeData;

    for (int64_t i = pNodeData->m_shadows.m_count - 1; i != -1; i--) {
        if (pNodeData->m_shadows.m_pData[i]->m_pGob == 0 && pNodeData->m_shadows.m_pData[i] != 0)
            delete pNodeData->m_shadows.m_pData[i];
        pNodeData->m_shadows.m_count--;
    }

    for (int64_t i = pNodeData->m_beams.m_count - 1; i != -1; i--) {
        if (pNodeData->m_beams.m_pData[i]->m_pGob == 0)
            delete pNodeData->m_beams.m_pData[i];
        pNodeData->m_beams.m_count--;
    }

    for (int64_t i = pNodeData->m_lights.m_count - 1; i != -1; i--) {
        if (pNodeData->m_lights.m_pData[i]->m_pGob == 0) {
            Model* pModel = pNodeData->m_lights.m_pData[i]->m_pNode->m_pTree->AsModel();
            if (pModel != 0)
                pModel->DelRef(pModel);
            delete pNodeData->m_lights.m_pData[i];
        }
        pNodeData->m_lights.m_count--;
    }

    for (int64_t i = pNodeData->m_meshes.m_count - 1; i != -1; i--) {
        if (pNodeData->m_meshes.m_pData[i]->m_pGob == 0) {
            Model* pModel = pNodeData->m_meshes.m_pData[i]->m_pNode->m_pTree->AsModel();
            if (pModel != 0)
                pModel->DelRef(pModel);
            delete pNodeData->m_meshes.m_pData[i];
        }
        pNodeData->m_meshes.m_count--;
    }

    for (int64_t i = pNodeData->m_emitters.m_count - 1; i != -1; i--) {
        if (pNodeData->m_emitters.m_pData[i]->m_pGob == 0) {
            Model* pModel = pNodeData->m_emitters.m_pData[i]->m_pNode->m_pTree->AsModel();
            if (pModel != 0)
                pModel->DelRef(pModel);
            delete pNodeData->m_emitters.m_pData[i];
        }
        pNodeData->m_emitters.m_count--;
    }

    for (int64_t i = pNodeData->m_otherParts.m_count - 1; i != -1; i--) {
        if (pNodeData->m_otherParts.m_pData[i]->m_pGob == 0) {
            Model* pModel = pNodeData->m_otherParts.m_pData[i]->m_pNode->m_pTree->AsModel();
            if (pModel != 0)
                pModel->DelRef(pModel);
            delete pNodeData->m_otherParts.m_pData[i];
        }
        pNodeData->m_otherParts.m_count--;
    }

    for (int64_t i = pNodeData->m_bins.m_count - 1; i != -1; i--) {
        if (pNodeData->m_bins.m_pData[i] != 0)
            delete pNodeData->m_bins.m_pData[i];
        pNodeData->m_bins.m_count--;
    }

    delete pNodeData;
}

// Append a leaf's bins whose clamped distance is under the given squared threshold.
void CollectBins(BSPNode* pNode, const Vector& vPos, float fMinDistSq)
{
    nodedata* pNodeData = pNode->m_pNodeData;

    int nBins = pNodeData->m_bins.m_count;
    for (int i = 0; i < nBins; i++) {
        CAurTriangleBin* pBin = pNodeData->m_bins.m_pData[i];
        float fGap = magnitude(vPos - pBin->m_vCenter) - pBin->m_fRadius;
        if (fGap < 0.0f)
            fGap = 0.0f;
        float fGapSq = fGap * fGap;
        if (fGapSq < fMinDistSq) {
            countactivebins++;
            g_pActiveBins.Add(pBin);
            pBin->SetMinDistanceSquared(fGapSq);
        }
    }
}

// Build this frame's grass polys, then tear down the ones that dropped out.
#if KOTOR1
bool IosGrassEnabled();  // K1 port option
#endif
void AurProcessActiveBins()
{
#if KOTOR1  // K1 builds and tears down nothing while the grass option is off
    if (!IosGrassEnabled())
        return;
#endif
    for (int64_t i = 0; i < g_pActiveBins.m_count; i++)
        g_pActiveBins.m_pData[i]->BuildGrassPolys();

    for (int64_t j = 0; j < g_pOldActiveBins.m_count; j++) {
        CAurTriangleBin* pOldBin = g_pOldActiveBins.m_pData[j];

        bool bFound = false;
        for (int64_t k = 0; k < g_pActiveBins.m_count; k++) {
            if (g_pActiveBins.m_pData[k] == pOldBin) {
                bFound = true;
                break;
            }
        }

        if (!bFound)
            pOldBin->DestroyGrassPolys();
    }
}

// True when the camera drifted past cameraepsilon; re-arms the saved position.
bool CameraMoved(Scene* pScene)
{
    K2_UNUSED(pScene);
    if (magnitude(CurrentCamera->m_vPosition - Scene::m_vLastCameraPosition) > cameraepsilon) {
        Scene::m_vLastCameraPosition = CurrentCamera->m_vPosition;
        return true;
    }
    return false;
}

// Drop the three per-frame lists every room keeps.
void ClearSceneRooms(Scene* pScene)
{
    for (int64_t i = 0; i < pScene->m_rooms.m_count; i++) {
        nodedata* pNodeData = pScene->m_rooms.m_pData[i]->m_pNodeData;
        pNodeData->m_movingLights.m_count = 0;
        pNodeData->m_movingObjects.m_count = 0;
        pNodeData->m_movingEmitters.m_count = 0;
    }
}

// File a light in every room whose x/y box contains it, or in all of them.
void AddLightToRooms(Scene* pScene, PartLight* pLight)
{
    int nAdded = 0;

    for (int64_t i = 0; i < pScene->m_rooms.m_count; i++) {
        CAurRoom* pRoom = pScene->m_rooms.m_pData[i];
        if (pLight->m_vWorldPosition.x >= pRoom->m_bboxMin.x
            && pLight->m_vWorldPosition.y >= pRoom->m_bboxMin.y
            && pLight->m_vWorldPosition.x <= pRoom->m_bboxMax.x
            && pLight->m_vWorldPosition.y <= pRoom->m_bboxMax.y) {
            pRoom->m_pNodeData->m_movingLights.Add(pLight);
            nAdded++;
        }
    }

    if (nAdded == 0) {
        for (int64_t i = 0; i < pScene->m_rooms.m_count; i++)
            pScene->m_rooms.m_pData[i]->m_pNodeData->m_movingLights.Add(pLight);
    }
}

// The emitter half of the room registration; a stationary emitter reuses its Gob's room.
void AddEmitterToRooms(Scene* pScene, PartEmitter* pEmitter)
{
    Gob* pGob = pEmitter->m_pGob;
    if (pGob == 0)
        return;

    if (equal(pGob->m_vPosition, pGob->m_vPreviousPosition) && pGob->m_rooms.m_count > 0) {
        static_cast<nodedata*>(static_cast<void*>(pGob->m_rooms.m_pData[0]))->m_movingEmitters.Add(pEmitter);
    } else {
        int nAdded = 0;

        for (int64_t i = 0; i < pScene->m_rooms.m_count; i++) {
            CAurRoom* pRoom = pScene->m_rooms.m_pData[i];
            if (pGob->m_vPosition.x >= pRoom->m_bboxMin.x
                && pGob->m_vPosition.y >= pRoom->m_bboxMin.y
                && pGob->m_vPosition.x <= pRoom->m_bboxMax.x
                && pGob->m_vPosition.y <= pRoom->m_bboxMax.y) {
                pRoom->m_pNodeData->m_movingEmitters.Add(pEmitter);
                nAdded++;
            }
        }

        if (nAdded == 0) {
            for (int64_t i = 0; i < pScene->m_rooms.m_count; i++)
                pScene->m_rooms.m_pData[i]->m_pNodeData->m_movingEmitters.Add(pEmitter);
        }
    }
}

// File a Gob in the room containing its bounding-box centre.
void AddObjectToRooms(Scene* pScene, Gob* pGob)
{
    Vector vWorldPos;
    if (pGob->m_pModel != 0 && (pGob->m_pModel->m_nFlags & 4) != 0) {
        vWorldPos = pGob->m_vPosition;
    } else {
        Vector vCenter = (pGob->m_vBoundingBoxMin + pGob->m_vBoundingBoxMax) * 0.5f;
        vWorldPos = pGob->m_vPosition + RotateVector(vCenter, pGob->m_qOrientation);
    }

    int nAdded = 0;
    if (pGob->m_pModel == 0 || (pGob->m_pModel->m_nFlags & Model::kModelRoom) == 0) {
        for (int64_t i = 0; i < pScene->m_rooms.m_count; i++) {
            CAurRoom* pRoom = pScene->m_rooms.m_pData[i];
            if (vWorldPos.x >= pRoom->m_bboxMin.x
                && vWorldPos.y >= pRoom->m_bboxMin.y
                && vWorldPos.x <= pRoom->m_bboxMax.x
                && vWorldPos.y <= pRoom->m_bboxMax.y) {
                pRoom->m_pNodeData->m_movingObjects.Add(pGob);
                crap = static_cast<int>(i);
                nAdded++;
            }
        }
    }

    if (nAdded == 0) {
        for (int64_t i = 0; i < pScene->m_rooms.m_count; i++)
            pScene->m_rooms.m_pData[i]->m_pNodeData->m_movingObjects.Add(pGob);
        crap = 999;
    }
}

// Publishes a room's light set, setup lights by colour and flare lights by colour shifts.
void LightManager::CollectLightsForGivenRoom(CAurRoom* pRoom)
{
    m_setupLights.m_count = 0;
    m_lensFlareLights.m_count = 0;

    List<CAurRoom*> rooms;
    if (pRoom != 0) {
        rooms = pRoom->m_visibleRooms;
        rooms.Add(pRoom);
    } else {
        rooms = currentscene->m_rooms;
    }

    for (int r = 0; r < rooms.m_count; r++) {
        nodedata* pNodeData = rooms.m_pData[r]->m_pNodeData;

        for (int i = 0; i < pNodeData->m_lights.m_count; i++) {
            PartLight* pLight = static_cast<PartLight*>(pNodeData->m_lights.m_pData[i]);
            if (static_cast<MdlNodeLight*>(pLight->m_pNode)->m_lFlareColourShifts.m_count >= 1)
                m_lensFlareLights.AddUnique(pLight);
            if ((pLight->m_vLightColour.x != 0.0f) | (pLight->m_vLightColour.y != 0.0f) ||
                pLight->m_vLightColour.z != 0.0f)
                m_setupLights.Add(pLight);
        }

        for (int i = 0; i < pNodeData->m_movingLights.m_count; i++) {
            PartLight* pLight = pNodeData->m_movingLights.m_pData[i];
            if (static_cast<MdlNodeLight*>(pLight->m_pNode)->m_lFlareColourShifts.m_count >= 1)
                m_lensFlareLights.AddUnique(pLight);
            if (pLight->m_vLightColour.x != 0.0f || pLight->m_vLightColour.y != 0.0f ||
                pLight->m_vLightColour.z != 0.0f)
                m_setupLights.AddUnique(pLight);
        }
    }
}

// Runs the per-mesh dynamic-light pass, writing one packed colour word per vertex.
void LightPartTriMesh(PartTriMesh* pPart, List<PartLight*> lights, Scene* pScene)
{
    K2_UNUSED(pScene);
    if (pPart == 0)
        return;
    if (pPart->m_pTileData != 0 && pPart->m_pTileData->m_nMode != 0)
        return;

    Vector vWorldPos;
    Quaternion q;

    MdlNodeTriMesh* pNode = static_cast<MdlNodeTriMesh*>(pPart->m_pVertexPrimitive->m_pNode);
    if (pNode->m_nVertexCount == 0)
        return;

    if (lights.m_count == 0) {
        Vector vZero;
        if (equal(pPart->m_vSelfIllumColor, vZero)) {
            if (pPart->m_bHasColorPool) {
                GLRender::DeletePool(pPart->m_nColorPool);
                pPart->m_nColorPool = -1;
                pPart->m_bHasColorPool = false;
                pPart->m_pVertexPrimitive->Rebuild();
            }
            return;
        }
        if (!pPart->m_bHasColorPool && pNode->m_nVertexCount == 0)
            return;
    }

    if (!pPart->m_bHasColorPool) {
        GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(0),
                              static_cast<short>(pNode->m_nVertexCount) * 4 + 4, 0x42, 0,
                              reinterpret_cast<unsigned int*>(&pPart->m_nColorPool));
        pPart->m_bHasColorPool = true;
        pPart->m_pVertexPrimitive->Rebuild();
    }
    pPart->GetWorld(vWorldPos, q);

    unsigned int* pColors = static_cast<unsigned int*>(GLRender::LockPool(pPart->m_nColorPool, 0, 0, 1));
    unsigned char* pVerts = static_cast<unsigned char*>(GLRender::LockPool(pNode->m_pTree->AsModel()->m_nPool,
                                                               0, 0, 16));

    static Vector accumulator;

    for (int i = 0; i < static_cast<short>(pNode->m_nVertexCount); i++) {
        unsigned char* pVertex = pVerts + pNode->m_nGeomBase + pNode->m_nVertexStride * i;
        Vector r1 = RotateVector(*reinterpret_cast<Vector*>(pVertex + pNode->m_nVertexOfs), q) + vWorldPos;
        Vector r2 = RotateVector(*reinterpret_cast<Vector*>(pVertex + pNode->m_nNormalOfs), q);

        accumulator = Vector(0.0f, 0.0f, 0.0f);
        if (pPart->m_pMaterial->m_bDecal)
            accumulator = Vector(1.0f, 1.0f, 1.0f);
        else
            accumulator = pPart->m_vSelfIllumColor;

        for (int j = 0; j < lights.m_count; j++) {
            PartLight* pLight = lights.m_pData[j];
            if (pLight->m_fLightRadius == 0.0f)
                continue;

            Vector vDelta = pLight->m_vWorldPosition - r1;
            float fDist = magnitude(vDelta);
            if (fDist == 0.0f)
                fDist = 1.0e-6f;
            float fAtten = 1.0f - fDist / pLight->m_fLightRadius;
            if (fAtten < 0.0f)
                continue;

            const Vector* pNormal;
            float fLambert;
            if (static_cast<MdlNodeLight*>(pLight->m_pNode)->m_bAmbientOnly) {
                pNormal = &pNode->m_vAmbient;
                fLambert = 1.0f;
            } else {
                Vector vDir = vDelta * (1.0f / fDist);
                fLambert = r2.x * vDir.x + r2.y * vDir.y + r2.z * vDir.z;
                if (fLambert < 0.0f)
                    continue;
                pNormal = &pNode->m_vDiffuse;
            }

            float fScale = fAtten * fLambert;
            accumulator.x += pNormal->x * (pLight->m_vLightColour.x * fScale);
            accumulator.y += pNormal->y * (pLight->m_vLightColour.y * fScale);
            accumulator.z += pNormal->z * (pLight->m_vLightColour.z * fScale);
        }

        if (pNode->m_nColorOfs != -1) {
            unsigned char* pVertexColor = pVertex + pNode->m_nColorOfs;
            accumulator.x *= static_cast<float>(pVertexColor[0]) / 255.0f;
            accumulator.y *= static_cast<float>(pVertexColor[1]) / 255.0f;
            accumulator.z *= static_cast<float>(pVertexColor[2]) / 255.0f;
        }

        if (accumulator.x < 0.0f) accumulator.x = 0.0f;
        if (accumulator.x > 1.0f) accumulator.x = 1.0f;
        if (accumulator.y < 0.0f) accumulator.y = 0.0f;
        if (accumulator.y > 1.0f) accumulator.y = 1.0f;
        if (accumulator.z < 0.0f) accumulator.z = 0.0f;
        if (accumulator.z > 1.0f) accumulator.z = 1.0f;

        unsigned char nR = static_cast<unsigned char>(accumulator.x * 255.0f + 0.5f);
        unsigned char nG = static_cast<unsigned char>(accumulator.y * 255.0f + 0.5f);
        unsigned char nB = static_cast<unsigned char>(accumulator.z * 255.0f + 0.5f);
        pColors[i] = static_cast<unsigned int>(nR | (nG << 8) | (nB << 16) | 0xff000000);
    }

    GLRender::UnlockPool(pPart->m_nColorPool);
    GLRender::UnlockPool(pNode->m_pTree->AsModel()->m_nPool);
}

// The room list to draw this frame.
#if KOTOR1
// K1: no view-volume culling; the current room plus the rooms visible from it, else every room.
void CollectActiveRooms(Scene* pScene, List<CAurRoom*>& outRooms)
{
    CAurRoom* pCurrentRoom = pScene->m_pCurrentRoom;
    if (pCurrentRoom == 0) {
        outRooms = pScene->m_rooms;
        return;
    }

    outRooms.m_count = 0;
    if (pScene->m_nIgnoreVisibilityGraph != 0 || enablevisibilitygraph == 0) {
        outRooms = pScene->m_rooms;
        return;
    }

    outRooms.Add(pCurrentRoom);
    for (int64_t i = 0; i < pScene->m_pCurrentRoom->m_visibleRooms.m_count; i++)
        outRooms.Add(pScene->m_pCurrentRoom->m_visibleRooms.m_pData[i]);
}
#else
void CollectActiveRooms(Scene* pScene, List<CAurRoom*>& outRooms)
{
    CAurRoom* pCurrentRoom = pScene->m_pCurrentRoom;
    outRooms.m_count = 0;

    if (pCurrentRoom == 0) {
        for (int64_t i = 0; i < pScene->m_rooms.m_count; i++)
            outRooms.Add(pScene->m_rooms.m_pData[i]);
        return;
    }

    if (pScene->m_nIgnoreVisibilityGraph != 0 || enablevisibilitygraph == 0) {
        outRooms.m_count = 0;
        for (int64_t i = 0; i < pScene->m_rooms.m_count; i++)
            outRooms.Add(pScene->m_rooms.m_pData[i]);
        return;
    }

    outRooms.Add(pScene->m_pCurrentRoom);

    Camera* pCamera = CurrentCamera;
    for (int64_t i = 0; i < pScene->m_pCurrentRoom->m_visibleRooms.m_count; i++) {
        CAurRoom* pRoom = pScene->m_pCurrentRoom->m_visibleRooms.m_pData[i];

        bool bActive = true;
        if (pRoom->m_bHasBackgroundGeometry == 0) {
            for (int64_t j = 0; j < pCamera->m_viewState.m_aViewVolumes.m_count; j++) {
                if (BoxAbovePlane(pRoom->m_bboxMin, pRoom->m_bboxMax,
                                  pCamera->m_viewState.m_aViewVolumes[static_cast<int>(j)]) > 0.0f) {
                    bActive = false;
                    break;
                }
            }
        }

        if (bActive)
            outRooms.Add(pRoom);
    }
}
#endif

// File a trigger part in every room containing its box centre.
void SceneAddPartToRooms(Scene* pScene, Part* pPart)
{
    PartTrigger* pTrigger = static_cast<PartTrigger*>(pPart->AsPartTrigger());

    for (int64_t i = 0; i < pScene->m_rooms.m_count; i++) {
        if (pTrigger == 0)
            continue;

        CAurRoom* pRoom = pScene->m_rooms.m_pData[i];
        if ((pTrigger->m_vBBoxMin.x + pTrigger->m_vBBoxMax.x) * 0.5f >= pRoom->m_bboxMin.x
            && (pTrigger->m_vBBoxMin.y + pTrigger->m_vBBoxMax.y) * 0.5f >= pRoom->m_bboxMin.y
            && (pTrigger->m_vBBoxMin.x + pTrigger->m_vBBoxMax.x) * 0.5f <= pRoom->m_bboxMax.x
            && (pTrigger->m_vBBoxMin.y + pTrigger->m_vBBoxMax.y) * 0.5f <= pRoom->m_bboxMax.y)
            pRoom->m_pNodeData->m_parts.Add(pTrigger);
    }
}

// Remove a trigger part from every room that holds it.
void SceneRemovePartFromRooms(Scene* pScene, Part* pPart)
{
    PartTrigger* pTrigger = static_cast<PartTrigger*>(pPart->AsPartTrigger());

    for (int64_t i = 0; i < pScene->m_rooms.m_count; i++) {
        if (pTrigger == 0)
            continue;

        CAurRoom* pRoom = pScene->m_rooms.m_pData[i];
        if (pRoom->m_pNodeData->m_parts.Contains(pTrigger) == 0)
            continue;

        pRoom->m_pNodeData->m_parts.Remove(pTrigger);
    }
}

int enablescenemanagement = 1;
int currentroomindex = -1;
float fadeWidth = 1.0f;
float wedgeCloseFactor = 0.333299994f;
float perpCloseFactor;

__attribute__((visibility("hidden"))) float fTriMeshMinigameAltRatio = 0.1f;
__attribute__((visibility("hidden"))) float fBladeDistThreshold = 15.0f;
__attribute__((visibility("hidden"))) float fBladeRadiusMinRatio = 1.0f;

extern int countculledpart;
extern int countculledshadows;
extern int countculledbeams;
extern int countbackshadows;
extern int countculledgobs;
extern int countlights;

bool IsPlayerControllingT3M4Tutorial();


inline List<Plane>& FadePlaneList(Camera* pCamera)
{
    return pCamera->m_viewState.m_aViewVolumes;
}

static void ProcessParts(BSPNode* pNode, void* pData, List<Plane>& planes);

static void ConstructCameraFadePlaneDef(Scene* pScene, const Vector& vTarget, const Vector& vCamPos,
                                        Vector vDelta, const Vector& vDir, float fMag);

static void ConstructCameraFadePlaneDef(Scene* pScene, const Vector& vTarget, const Vector& vCamPos,
                                        Vector vDelta, const Vector& vDir, float fMag)
{
    static Vector vertZ = Vector(0.0f, 0.0f, 1.0f);

    vDelta.z = 0.0f;
    // The wedge's half-width offset, perpendicular to the flat view direction.
    Vector vPerp = Vector(-vDir.y, vDir.x, 0.0f) * fadeWidth;

    Vector vNear = normalize(cross(vertZ, vDelta * (1.0f - wedgeCloseFactor) - vPerp));
    Vector vNearPoint = vCamPos + vPerp;
    Plane planeNear;
    planeNear.a = vNear.x;
    planeNear.b = vNear.y;
    planeNear.c = vNear.z;
    planeNear.d = -(vNear.x * vNearPoint.x + vNear.y * vNearPoint.y + vNear.z * vNearPoint.z);

    Vector vFar = normalize(cross(vDelta * (1.0f - wedgeCloseFactor) + vPerp, vertZ));
    Vector vFarPoint = vCamPos - vPerp;
    Plane planeFar;
    planeFar.a = vFar.x;
    planeFar.b = vFar.y;
    planeFar.c = vFar.z;
    planeFar.d = -(vFar.x * vFarPoint.x + vFar.y * vFarPoint.y + vFar.z * vFarPoint.z);

    pScene->m_eFadeState = Scene::FADING_ON;

    Camera* pCam = CurrentCamera;
    Vector vNormal = Vector(vDir.x, vDir.y, 0.0f) * (1.0f / fMag);
    Vector vPoint = vTarget - vDelta * perpCloseFactor;
    pCam->m_viewState.m_aFadePlane[0] = vNormal.x;
    pCam->m_viewState.m_aFadePlane[1] = vNormal.y;
    pCam->m_viewState.m_aFadePlane[2] = vNormal.z;
    pCam->m_viewState.m_aFadePlane[3] =
        -(vNormal.x * vPoint.x + vNormal.y * vPoint.y + vNormal.z * vPoint.z);

    pCam->SetFadePlanes(planeNear, planeFar);
}

// Runs the frame's scene pass: visibility, room registration, static lighting, buckets.
void ManageSceneBSP(Scene* pScene)
{
    LightManager::cm_bLightManagerUpdated = 1;

    if (enablescenemanagement == 0)
        return;

    if (currentroomindex >= 0) {
        pScene->m_pCurrentRoom = pScene->m_rooms.m_pData[currentroomindex];
        pScene->m_nIgnoreVisibilityGraph = 0;
        enablevisibilitygraph = 1;
    }

    if (pScene->m_pBSPRoot == 0 && pScene->m_rooms.m_count == 0)
        return;

    for (int64_t i = 0; i < pScene->Objects().m_count; i++)
        pScene->Objects().m_pData[i]->DisableInViewVolume();

    LightManager* pLightManager = pScene->m_pLightManager;
    static Vector lastcamerapos;
    static Quaternion lastcameraorientation;

    // Does the view need a fade wedge?
    CAurBehaviorCameraFollow* pFollow = 0;
    if (CurrentCamera != 0 && CurrentCamera->m_pBehavior != 0)
        pFollow = CurrentCamera->m_pBehavior->AsBehaviorCameraFollow();
    if (pFollow != 0) {
        Vector vTarget = pFollow->GetTargetWorldPosition();
        Vector vCamPos = CurrentCamera->GetPosition();
        Vector vDelta = vTarget - vCamPos;
        Vector vDir = normalize(vDelta);
        float fMag = sqrtf(vDir.x * vDir.x + vDir.y * vDir.y);
        if (fMag < 0.4)
            pScene->m_eFadeState = Scene::FADING_OFF;
        else
            ConstructCameraFadePlaneDef(pScene, vTarget, vCamPos, vDelta, vDir, fMag);
    } else {
        pScene->m_eFadeState = Scene::FADING_OFF;
    }

    DoStaticLightBuckets(pScene);
    ClearBuckets(pScene);
    ClearSceneRooms(pScene);

    pScene->m_pLightManager->Reset();
    pScene->m_pLightManager->UpdateLights();

    // Re-file every enabled light into the rooms it reaches, and re-add it to the BSP when the
    // frame culled the room table away.
    for (int64_t i = 0; i < pLightManager->m_dynamicGobLights.m_count; i++) {
        PartLight* pLight = pLightManager->m_dynamicGobLights.m_pData[i];
        AddLightToRooms(pScene, pLight);
        if (pScene->m_rooms.m_count == 0) {
            SceneBSPRemovePart(pScene, pLight);
            SceneBSPAddPart(pScene, pLight, 0);
        }
    }

    if (pScene->m_nIgnoreVisibilityGraph == 0 && enablevisibilitygraph != 0)
        pScene->m_pLightManager->CollectLightsForGivenRoom(pScene->m_pCurrentRoom);
    else if (pScene->m_rooms.m_count != 0)
        pScene->m_pLightManager->CollectLightsForGivenRoom(0);

    K2_ASSUME(CurrentCamera != 0);   // Scene::Render runs only inside Camera::RenderScene, which sets it
    if (pScene->m_rooms.m_count != 0)
        pScene->m_pLightManager->SetupLightsForThisRoom(pScene->m_vSceneFocus, CurrentCamera->m_fClipFar);
    else
        pScene->m_pLightManager->SetupLightsForThisPositionAndRadius(pScene->m_vSceneFocus, CurrentCamera->m_fClipFar);

    for (int64_t i = 0; i < pScene->m_emitters.m_count; i++) {
        PartEmitter* pEmitter = pScene->m_emitters.m_pData[i];
        AddEmitterToRooms(pScene, pEmitter);
        if (pScene->m_rooms.m_count == 0 && pEmitter->m_pGob != 0) {
            SceneBSPRemovePart(pScene, pEmitter);
            SceneBSPAddPart(pScene, pEmitter, 0);
        }
    }

    for (int64_t i = 0; i < pScene->Objects().m_count; i++) {
        Gob* pGob = pScene->Objects().m_pData[i];
        if (pGob->m_pModel == 0)
            continue;

        AddObjectToRooms(pScene, pGob);
        if (pScene->m_rooms.m_count == 0
            && strcmp(pGob->m_szName, "c_FocusGob") != 0) {
            SceneBSPRemoveGob(pScene, pGob);
            SceneBSPAddGob(pScene, pScene->Objects().m_pData[i]);
        }
    }

    countculledpart = 0;
    countculledshadows = 0;
    countculledbeams = 0;
    countbackshadows = 0;
    countculledgobs = 0;

    List<CAurRoom*> rooms;
    CollectActiveRooms(pScene, rooms);

    if (pScene->m_rooms.m_count != 0) {
        if (pScene->m_bHasBins) {
            for (int64_t i = 0; i < rooms.m_count; i++)
                BSPTraverse(rooms.m_pData[i], CurrentCamera->m_vPosition,
                            CAurTriangleBin::m_fFarRadiusSquared, CollectBins);

            AurProcessActiveBins();
        }

#if defined(K2_MOBILE_OPT) && defined(K2_QOL_IMPROVEMENTS)
        s_fFogCullFloor = pScene->m_bFogEnabled != 0 ? pScene->m_fFogEnd * 1.5f : 0.0f;
#endif
        // Per-area fog and ratio overrides.
        for (int64_t i = 0; i < rooms.m_count; i++) {
            vOutsideRef = CurrentCamera->m_vPosition;
#if KOTOR1  // K1 has no per-module minigame culling overrides
            BSPTraverseVolumeApprox(rooms.m_pData[i], CurrentCamera->m_vPosition,
                                    FadePlaneList(CurrentCamera), ProcessParts, pScene);
#else
            bool bTutorial = IsPlayerControllingT3M4Tutorial();

            CAurRoom* pRoom = rooms.m_pData[i];
            char cZone = pRoom->m_szName[0];
            bool bChanged = false;

            float fOldNoBBOnRatio K2_UNSET;
            float fOldTriMeshRatio K2_UNSET;
            float fOldSmallDistRatio K2_UNSET;
            float fOldOutsideRatio K2_UNSET;

            if (cZone == '5') {
                if (strstr(pRoom->m_szName, "504ond") != 0) {
                    fOldNoBBOnRatio = fGobOutNoBBOnRatio;
                    fOldTriMeshRatio = fTriMeshMinigameOnRatio;
                    fOldSmallDistRatio = fGobOutSmallDistRatio;
                    fOldOutsideRatio = fOutsideDistRatio;
                    fTriMeshMinigameOnRatio = 0.05f;
                    fGobOutNoBBOnRatio = 0.1f;
                    fGobOutSmallDistRatio = 0.06f;
                    bChanged = true;
                }
                if (strstr(pRoom->m_szName, "503ond") != 0) {
                    fOldNoBBOnRatio = fGobOutNoBBOnRatio;
                    fOldTriMeshRatio = fTriMeshMinigameOnRatio;
                    fOldSmallDistRatio = fGobOutSmallDistRatio;
                    fOldOutsideRatio = fOutsideDistRatio;
                    fTriMeshMinigameOnRatio = 0.05f;
                    fGobOutNoBBOnRatio = 0.1f;
                    fGobOutSmallDistRatio = 0.06f;
                    bChanged = true;
                    goto traverse;  // skip the 103per/902/102 legs
                }
            } else if (cZone == '2') {
                if (strstr(pRoom->m_szName, "221tel") != 0) {
                    fOldNoBBOnRatio = fGobOutNoBBOnRatio;
                    fOldTriMeshRatio = fTriMeshMinigameOnRatio;
                    fOldSmallDistRatio = fGobOutSmallDistRatio;
                    fOldOutsideRatio = fOutsideDistRatio;
                    fTriMeshMinigameOnRatio = 0.014f;
                    fGobOutNoBBOnRatio = 0.1f;
                    fGobOutSmallDistRatio = 0.06f;
                    bChanged = true;
                }
            } else if (cZone == '0' && bTutorial) {
                if (strstr(pRoom->m_szName, "001ebo") != 0) {
                    fOldNoBBOnRatio = fGobOutNoBBOnRatio;
                    fOldTriMeshRatio = fTriMeshMinigameOnRatio;
                    fOldSmallDistRatio = fGobOutSmallDistRatio;
                    fOldOutsideRatio = fOutsideDistRatio;
                    fTriMeshMinigameOnRatio = fTriMeshMinigameAltRatio;
                    bChanged = true;
                    goto traverse;  // same
                }
            }

            // This leg tests the first active room, not the current one, and only runs when
            // the 503ond and 001ebo arms above did not jump straight to the traversal.
            if (rooms.m_pData[0]->m_szName[0] == '1'
                && strstr(rooms.m_pData[0]->m_szName, "103per") != 0) {
                fOldNoBBOnRatio = fGobOutNoBBOnRatio;
                fOldTriMeshRatio = fTriMeshMinigameOnRatio;
                fOldSmallDistRatio = fGobOutSmallDistRatio;
                fOldOutsideRatio = fOutsideDistRatio;
                fTriMeshMinigameOnRatio = 0.005f;
                fGobOutNoBBOnRatio = 0.02f;
                fGobOutSmallDistRatio = 0.05f;
                fOutsideDistRatio = 0.07f;
                bChanged = true;
            } else if (cZone == '9') {
                if (strstr(pRoom->m_szName, "902mala") != 0) {
                    fOldNoBBOnRatio = fGobOutNoBBOnRatio;
                    fOldTriMeshRatio = fTriMeshMinigameOnRatio;
                    fOldSmallDistRatio = fGobOutSmallDistRatio;
                    fOldOutsideRatio = fOutsideDistRatio;
                    fTriMeshMinigameOnRatio = 0.0115f;
                    fGobOutNoBBOnRatio = 0.005f;
                    fGobOutSmallDistRatio = 0.005f;
                    bChanged = true;
                }
            } else if (cZone == '1') {
                if (strstr(pRoom->m_szName, "102per") != 0) {
                    fOldNoBBOnRatio = fGobOutNoBBOnRatio;
                    fOldTriMeshRatio = fTriMeshMinigameOnRatio;
                    fOldSmallDistRatio = fGobOutSmallDistRatio;
                    fOldOutsideRatio = fOutsideDistRatio;
                    fTriMeshMinigameOnRatio = 0.06f;
                    fGobOutNoBBOnRatio = 0.03f;
                    fGobOutSmallDistRatio = 0.05f;
                    fOutsideDistRatio = 0.07f;
                    bChanged = true;
                }
            }

        traverse:
            BSPTraverseVolumeApprox(pRoom, CurrentCamera->m_vPosition,
                                    FadePlaneList(CurrentCamera), ProcessParts, pScene);

            if (bChanged) {
                fGobOutNoBBOnRatio = fOldNoBBOnRatio;
                fTriMeshMinigameOnRatio = fOldTriMeshRatio;
                fGobOutSmallDistRatio = fOldSmallDistRatio;
                fOutsideDistRatio = fOldOutsideRatio;
            }
#endif
        }
    } else {
        BSPTraverseVolumeApprox(pScene->m_pBSPRoot, CurrentCamera->m_vPosition,
                                FadePlaneList(CurrentCamera), ProcessParts, pScene);
    }

    pLightManager->PrioritizeLights(pScene->m_vSceneFocus, 1);

    countlights = 0;
    for (int64_t i = 0; i < pScene->m_pLightManager->m_enabledLights.m_count; i++) {
        PartLight* pLight = pScene->m_pLightManager->m_enabledLights.m_pData[i];
        Vector vColour = pLight->m_vLightColour;
        if (!vColour.IsZero())
            countlights++;
    }

    for (int64_t i = 0; i < pScene->Objects().m_count; i++) {
        Gob* pGob = pScene->Objects().m_pData[i];
        if (pGob->m_pModel == 0)
            continue;

        if (pScene->m_rooms.m_count == 0) {
            gobbucket.Add(pGob);
            continue;
        }

        if (pGob->IsInViewVolume() || (pGob->m_pModel->m_nFlags & Model::kModelAmbient) != 0 || pGob->IsAlwaysRendering()) {
#if !KOTOR1  // K1 has no hologram bucket
            if (pGob->IsHologramEnabled()) {
                g_listHologramBucket.Add(pGob);
                continue;
            }
#endif

            if (pGob->IsForcePushDistortion()) {
                g_listForcePushBucket.Add(pGob);
            } else if (pGob->m_pModel != 0 && (pGob->m_pModel->m_nFlags & Model::kModelLightsaber) != 0) {
                // A null model falls to gobbucket, not to this test.
#ifdef K2_MOBILE_OPT   // the mobile port drops a blade past 15 m; the PC lineage draws every one
                Vector vCenter = RotateVector(pGob->m_vMinSphereCenter, pGob->m_qOrientation)
                                 + pGob->m_vPosition;
                float fDist = magnitude(vOutsideRef - vCenter) - pGob->m_fMinSphereRadius;
                if (!(fDist > fBladeDistThreshold
                      && pGob->m_fMinSphereRadius / fDist < fBladeRadiusMinRatio))
#endif
                    g_listLightsaberBucket.Add(pGob);
            } else {
                gobbucket.Add(pGob);
            }
        } else {
            pGob->m_nVisible = 1;
        }
    }

    // Close the frame.
    CurrentCamera->EnableInViewVolume();
    lastcamerapos = CurrentCamera->m_vPosition;
    lastcameraorientation = CurrentCamera->m_qOrientation;
}

int renderaabb;
int cullfoggeometry = 1;

float fadeToZeroTime = 0.25f;

__attribute__((visibility("hidden"))) float fMeshDistThreshold = 15.0f;
__attribute__((visibility("hidden"))) float fMeshMinigameOnRatio = 0.0045f;
#if KOTOR1
__attribute__((visibility("hidden"))) float fMeshMinigameOffRatio = 0.0115f;
#else
__attribute__((visibility("hidden"))) float fMeshMinigameOffRatio = 0.004f;
#endif

#include "port/crtapi.h"

static inline __attribute__((always_inline)) void UpdateGobViewDistance(Gob* pGob)
{
    Vector vCenter = RotateVector(pGob->m_vMinSphereCenter, pGob->m_qOrientation)
                     + pGob->m_vPosition;
    pGob->m_fViewDistance = magnitude(CurrentCamera->m_vPosition - vCenter);
}

// Cull a leaf's parts against the fade planes and bucket the survivors.
static void ProcessParts(BSPNode* pNode, void* pData, List<Plane>& planes)
{
    K2_UNUSED(pData);
    if (pNode == 0)
        return;

    nodedata* pNodeData = pNode->m_pNodeData;
    if (pNodeData == 0)
        return;

    if (renderaabb && pNodeData->m_pPartAABB != 0)
        AddPartToMeshBuckets(static_cast<PartTriMesh*>(pNodeData->m_pPartAABB));

    // A tile that must not fade stops the whole leaf.
    if (cullfoggeometry == 0 && pNodeData->m_pTileData != 0
        && pNodeData->m_pTileData->m_bFogged != 0)
        return;

    // The leaf's own gobs.
    for (int64_t i = 0; i < pNodeData->m_gobs.m_count; i++) {
        Gob* pGob = pNodeData->m_gobs.m_pData[i];
        if (GobOutside(pGob, planes)) {
            countculledgobs++;
            continue;
        }

        pGob->EnableInViewVolume();
        UpdateGobViewDistance(pGob);
    }

    // The gobs a room registered here this frame, copied before the walk.
    List<Gob*> gobList = pNodeData->m_movingObjects;
    for (int64_t i = 0; i < gobList.m_count; i++) {
        Gob* pGob = gobList.m_pData[i];

        if (pGob->IsInViewVolume())
            UpdateGobViewDistance(pGob);

        if (GobOutside(pGob, planes)) {
            countculledgobs++;
            continue;
        }

        pGob->EnableInViewVolume();
        UpdateGobViewDistance(pGob);
    }

    // The leaf's own emitters.
    for (int64_t i = 0; i < pNodeData->m_emitters.m_count; i++) {
        PartEmitter* pEmitter = pNodeData->m_emitters.m_pData[i];
        if (!PartOutside(pEmitter, planes))
            emitterbucket.Add(pEmitter);
    }

    // The emitters a room registered here this frame. One that is outside the volume is
    // still drawn when its model name matches the one hard-coded exception.
    List<PartEmitter*> emitterList = pNodeData->m_movingEmitters;
    for (int64_t i = 0; i < emitterList.m_count; i++) {
        PartEmitter* pEmitter = emitterList.m_pData[i];
        if (!PartOutside(pEmitter, planes)
            || strstr(_strupr(pEmitter->m_pNode->m_pTree->m_szName), "BAODUR") != 0)
            emitterbucket.AddUnique(pEmitter);
    }

    // Advance this leaf's fade-in.
    if (pNodeData->m_pTileData != 0 && pNodeData->m_pTileData->m_fFade < 1.0f
        && pNodeData->m_pTileData->m_bFogged == 0) {
        pNodeData->m_pTileData->m_fFade += DeltaT / fadeToZeroTime;
        if (pNodeData->m_pTileData->m_fFade > 1.0f)
            pNodeData->m_pTileData->m_fFade = 1.0f;
    }

    // Meshes get the distance and ratio cull, then the plane sweep.
    if (renderaabb == 0) {
        for (int64_t i = 0; i < pNodeData->m_meshes.m_count; i++) {
            PartTriMesh* pMesh = pNodeData->m_meshes.m_pData[i];
#ifdef K2_MOBILE_OPT   // the mobile port's size-over-distance cull; the PC lineage has only the planes
            MdlNodeTriMesh* pMdlNode = static_cast<MdlNodeTriMesh*>(pMesh->m_pNode);
            float fRadius = pMdlNode->m_fRadius;
            float fDist = magnitude(vOutsideRef - pMesh->m_vLocalPosition) - fRadius;

            if (fDist > fMeshDistThreshold) {
                float fLimit = GetMiniGamePlayingType() != 0 ? fMeshMinigameOnRatio
                                                             : fMeshMinigameOffRatio;
                if (fRadius / fDist < fLimit)
                    continue;
            }
#endif

            if (PartOutside(pMesh, planes))
                continue;

            AddPartToMeshBuckets(pMesh);
        }
    }

    // Shadows, triggers and beams.
    for (int64_t i = 0; i < pNodeData->m_shadows.m_count; i++) {
        Part* pShadow = pNodeData->m_shadows.m_pData[i];
        if (PartOutside(static_cast<PartProjection*>(pShadow), planes)) {
            countculledshadows++;
            continue;
        }

        staticshadowbucket.Add(pShadow);
    }

    for (int64_t i = 0; i < pNodeData->m_parts.m_count; i++) {
        PartTrigger* pTrigger = pNodeData->m_parts.m_pData[i];
        if (PartOutside(pTrigger, planes))
            continue;

        triggerbucket.AddUnique(pTrigger);
    }

    for (int64_t i = 0; i < pNodeData->m_beams.m_count; i++) {
        Part* pBeam = pNodeData->m_beams.m_pData[i];
        if (PartOutside(static_cast<PartProjection*>(pBeam), planes)) {
            countculledbeams++;
            continue;
        }

        staticbeambucket.Add(pBeam);
    }

    // Bins.
    for (int64_t i = 0; i < pNodeData->m_bins.m_count; i++) {
        CAurTriangleBin* pBin = pNodeData->m_bins.m_pData[i];
        if (BinOutside(pBin, planes))
            continue;

        binbucket.Add(pBin);
        countbinbucket++;
    }
}
