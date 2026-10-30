// CAurScene and Scene, the Aurora render scene, plus the room and hitcheck blocks the scene owns.
#pragma once

#include "Vector.h"
#include "List.h"
#include "SafePointer.h"
#include "aurtexman.h"

class CHitInfo;
class CAurObject;
struct BSPNode;
struct CAurRoom;
class Gob;
class LightManager;
class PartEmitter;
class WindManager;
class Scene;

// The abstract render-scene interface; Scene below is the concrete implementation. The derived
// class owns the object list, the fog block, the light and wind managers and the ambient colour;
// the base keeps only the SafePointerList sub-object, a name buffer and one counter.
class CAurScene : public SafePointerList<CAurScene> {
public:
    static const int ENABLE_FADE;
    static const int DISABLE_FADE;
    static const int ALWAYS_FADE;

    virtual ~CAurScene() {}
    virtual void SetFog(int a) = 0;
    virtual void SetFogColor(const Vector& color) = 0;
#if KOTOR1  // K1 spells this method GetObject
    virtual Gob* GetObject(char* a) = 0;
    Gob* GetObjectA(char* a) { return GetObject(a); }
#else
    virtual Gob* GetObjectA(char* a) = 0;
#endif
    virtual bool SetObjectScale(CAurObject* p, float f) = 0;
    virtual float GetObjectScale(CAurObject* p) = 0;
    virtual void Render() = 0;
    virtual void Animate(float a) = 0;
    virtual void InitBSP(int w, int h, int a, bool b, float c) = 0;
    virtual Gob* SpawnTile(char* a, const Vector& b, int c, int d) = 0;
    virtual void SetTileLights(const Vector& a, const Vector& b, const Vector& c) = 0;
    virtual void SetTileFlames(const Vector& a, CAurObject* b, CAurObject* c) = 0;
    virtual void ReplaceTileTexture(const Vector& a, char* b, char* c) = 0;
    virtual void DeleteTile(const Vector& a) = 0;
    virtual int HitCheck(Vector from, Vector to, Vector* pHit, CHitInfo* pInfo) = 0;
    virtual void ComputeStaticLighting();
    virtual void GetFogRange(float& a, float& b) = 0;
    virtual void SetFogRange(float fNear, float fFar) = 0;
    virtual void SetSceneFocus(Vector v) = 0;
    virtual bool SetGeomFadeProperties(int a) = 0;
    virtual int IsGeomFadeEnabled() = 0;
    virtual int GetObjectCount() = 0;
    virtual void FinishSceneGeometry() = 0;
    virtual int GetNumRooms() = 0;
    virtual void EnableAnimations() = 0;
    virtual void DisableAnimations() = 0;
    virtual void SetBlindLights(CAurObject* p) = 0;
    virtual int SpawnRooms(char* a) = 0;
    virtual Gob* SpawnRoom(char* a, const Vector& b, const Quaternion& c) = 0;
    virtual CAurRoom* FindRoom(char* a) = 0;
    virtual CAurRoom* GetRoom(int a) = 0;
    virtual Scene* SetCurrentRoom(CAurRoom* p) = 0;   // returns this
    virtual void SetCurrentRoom(int a) = 0;
    virtual int GetIgnoreVisibilityGraph() = 0;
    virtual int EnableVisibilityGraph() = 0;
    virtual void DisableVisibilityGraph() = 0;
    virtual int SaveVisibility(char* a, char* b) = 0;
    virtual int LoadVisibility(char* a) = 0;

    char   m_szName[64];
    int    m_nAnimateTick;

    // Ambient colour of the current area. Scene owns the storage, so the method reaches it through
    // a derived-class cast.
    void SetAmbient(Vector color);
};

extern int hitcheck_static_bsp_parts;

// Scene is the concrete render scene. It derives from CAurScene, so the base's slots are declared
// first and Scene's own follow.
#ifndef SCENE_H
#define SCENE_H

#include "AURORA/aurmodel.h"

class CAurObject;
struct CAurRoom;
class CAurTriangleBin;
class CHitInfo;
class Gob;
class Part;
class PartAABB;
class PartLight;
struct Quaternion;
struct Vector;

class Scene : public CAurScene {
public:
    // Static state, defined in Scene.cpp.
    static int geomFadeEnabled;
    static float binsize;
    static float cellsize;
    // Fade-state constants stored into m_eFadeState by ManageSceneBSP.
    static const int FADING_OFF;
    static const int FADING_ON;
    // The camera position captured the last time CameraMoved answered true.
    static Vector m_vLastCameraPosition;

    // Removes a gob from the scene's render buckets; static, acting on the global bucket lists.
    static void RemoveFromSceneManager(Gob* pGob);

    // The part flavour of the same un-bucketing.
    static void RemoveFromSceneManager(Part* pPart);

    // Part-tree walks; none of them is in the vtable.
    void ListSpecialParts(Part* pPart);
    void ListSpecialPartsR(Part* pPart);
    void ListSpringPartsR(Part* pPart);
    void UnlistSpecialParts(Part* pPart);

    // Visibility-graph queries, driven by the renderer's room culling.
    void ClearVisibility();
    int  QueryVisibility(CAurRoom* pFrom, CAurRoom* pRoom);
    // Only edits the two rooms' lists; the receiver is unused.
    void SetVisibility(CAurRoom* pFrom, CAurRoom* pRoom, int bVisible);

    // The gob half of the scene hitcheck; HitCheck does the BSP half and then calls this.
    void* HitCheckGob(Vector vStart, Vector vEnd, Vector* pvHit, CHitInfo* pInfo);

    // Draws a box for each fog-of-war room.
    void RenderFogOfWarBoxes();

    // Warms up the scene's particle emitters.
    void UpdateEmitters();

    // Defined out of line.
    Scene(char* pszName);

    virtual ~Scene();
    virtual void SetFog(int);
    virtual void SetFogColor(Vector const&);
#if KOTOR1  // K1 spells this method GetObject
    virtual Gob* GetObject(char*);
#else
    virtual Gob* GetObjectA(char*);
#endif
    virtual bool SetObjectScale(CAurObject*, float);
    virtual float GetObjectScale(CAurObject*);
    virtual void Render();
    virtual void Animate(float);
    virtual void InitBSP(int, int, int, bool, float);
    virtual Gob* SpawnTile(char*, Vector const&, int, int);
    virtual void SetTileLights(Vector const&, Vector const&, Vector const&);
    virtual void SetTileFlames(Vector const&, CAurObject*, CAurObject*);
    virtual void ReplaceTileTexture(Vector const&, char*, char*);
    virtual void DeleteTile(Vector const&);
    virtual int HitCheck(Vector, Vector, Vector*, CHitInfo*);
    virtual void GetFogRange(float&, float&);
    virtual void SetFogRange(float, float);
    virtual void SetSceneFocus(Vector);
    virtual bool SetGeomFadeProperties(int);
    virtual int IsGeomFadeEnabled();
    virtual int GetObjectCount();
    virtual void FinishSceneGeometry();
    virtual int GetNumRooms();
    virtual void EnableAnimations();
    virtual void DisableAnimations();
    virtual void SetBlindLights(CAurObject*);
    virtual int SpawnRooms(char*);
    virtual Gob* SpawnRoom(char*, Vector const&, Quaternion const&);
    virtual CAurRoom* FindRoom(char*);
    virtual CAurRoom* GetRoom(int);
    virtual Scene* SetCurrentRoom(CAurRoom*);   // returns this
    virtual void SetCurrentRoom(int);
    virtual int GetIgnoreVisibilityGraph();
    virtual int EnableVisibilityGraph();
    virtual void DisableVisibilityGraph();
    virtual int SaveVisibility(char*, char*);
    virtual int LoadVisibility(char*);
    virtual void RenderGobFirst(CAurObject*);
    virtual void RenderGobLast(CAurObject*);
    virtual void RenderGobAfter(CAurObject*, CAurObject*);
    virtual void RenderGobBefore(CAurObject*, CAurObject*);
    virtual bool IsAnimating();
    virtual void AddObject(CAurObject*);
    virtual void ChildAdd(Gob*);
    virtual void ChildRemove(Gob*);
    virtual void RenderSinglePass();
    virtual void RenderShadows(int, int, int, int);
    virtual void RenderDynamicGeometry();
    virtual void RenderStaticGeometry();
    virtual void RenderLensFlares();
    virtual void RenderMotionBlur();
    virtual Vector GetWindVector(Vector, float);
    virtual void PostProcessLS();
    virtual void DoMeshBuckets();
    virtual void DoBlurBuckets();
    virtual void DoGobBuckets();
#if !KOTOR1  // K1 has no hologram bucket pass
    virtual void DoHologramGobBuckets();
#endif
    virtual void DoDistortionBuckets();
    virtual void DoLightSaberBucket();
    virtual void DoCapBucket();
    virtual void DoBinBucket();
    virtual void DoFadeBuckets();
    virtual void DoGobShadows();
    virtual void DoGobShadowsForThisLight(PartLight&);
    virtual void RenderStencilShadowsForThisLight(PartLight&, float, int);
    virtual void DoStaticMeshShadows();
    virtual void DoStaticMeshBeams();
    virtual void DoDynamicMeshProjections();
    virtual void DoEmitterBucket();
    virtual void DoTriggerBucket();
    virtual void DoHighlightTriggers(bool);
    virtual void SetSingleTileFlame(CAurObject*, Gob**, Part*);
    virtual void RenderOnlyGobs();
    virtual void DoFog();
    virtual void AddFacesToBins(PartAABB*, Vector const&, List<List<CAurTriangleBin*>>&);

    // -- data ------------------------------------------------------------------------------------
    List<Gob*> m_objects;
    int    m_bFogEnabled;
    float  m_fFogStart;
    float  m_fFogEnd;
    int    m_nFogMode;
    float  m_fFogDensity;
    Vector m_vFogColor;
#if !KOTOR1  // K1 has no force-sight grey fog colour
    Vector m_vFogColorGrey;
#endif
    LightManager* m_pLightManager;
    WindManager* m_pWindManager;
    Vector m_ambient;

    List<Gob*>& Objects() { return m_objects; }

    // Every emitter part in the scene, kept flat so the renderer can tick them without walking the
    // gob trees.
    List<PartEmitter*> m_emitters;
    // The scene's static BSP tree.
    BSPNode* m_pBSPRoot;
    // The shared normalization cube map, held through a SafePointer so a texture flush nulls it.
    SafePointer<CAurTexture> m_pNormalizationCubeMap;
    Vector m_vSceneFocus;
    // The trigger-highlight pulse phase, in radians.
    float  m_fHighlightPhase;
    // The frame's scene-fade state.
    int    m_eFadeState;
    // The rooms the fog-of-war overlay draws a box for, a per-frame subset of m_rooms.
    List<BSPNode*> m_fogOfWarRooms;
    // The area's whole room table.
    List<CAurRoom*> m_rooms;
    CAurRoom* m_pCurrentRoom;
    short  m_nIgnoreVisibilityGraph;
#if KOTOR1
    int    m_nMiniGameType;
#endif
    bool   m_bAnimating;
    // Set when a face is appended to a grass bin.
    bool   m_bHasBins;
    void*  m_pReserved1;
    float  m_fReserved1;
    // Post-process textures, acquired by name in the constructor.
    CAurTexture* m_pFilmNoiseTexture;
    CAurTexture* m_pDistortionTexture;
#if !KOTOR1  // K1 acquires only the film-noise and distortion textures
    CAurTexture* m_pDirtyTexture;
    CAurTexture* m_pDirtyTexture2;
    CAurTexture* m_pDirtyTexture3;
    CAurTexture* m_pLensFlareTexture;
    CAurTexture* m_pHologramTexture;
#endif
    // The camera gobs, kept flat so they can be animated before the rest.
    List<Gob*> m_cameras;

    // The focus gob the camera and the lighting rig hang off.
    Gob m_focusGob;
};

// The scene being animated or rendered; null outside those passes (aurscene.cpp).
extern Scene* currentscene;

#if KOTOR1  // K1 lays the same members out at different offsets
K2_LAYOUT_ASSERT(__builtin_offsetof(Scene, m_pLightManager) == 0x90, "K1 Scene");
K2_LAYOUT_ASSERT(__builtin_offsetof(Scene, m_emitters) == 0xb0, "K1 Scene");
K2_LAYOUT_ASSERT(__builtin_offsetof(Scene, m_rooms) == 0xf8, "K1 Scene");
K2_LAYOUT_ASSERT(__builtin_offsetof(Scene, m_nIgnoreVisibilityGraph) == 0x110, "K1 Scene");
K2_LAYOUT_ASSERT(__builtin_offsetof(Scene, m_nMiniGameType) == 0x114, "K1 Scene");
K2_LAYOUT_ASSERT(__builtin_offsetof(Scene, m_bAnimating) == 0x118, "K1 Scene");
K2_LAYOUT_ASSERT(__builtin_offsetof(Scene, m_pReserved1) == 0x120, "K1 Scene");
K2_LAYOUT_ASSERT(__builtin_offsetof(Scene, m_fReserved1) == 0x128, "K1 Scene");
K2_LAYOUT_ASSERT(__builtin_offsetof(Scene, m_pFilmNoiseTexture) == 0x130, "K1 Scene");
K2_LAYOUT_ASSERT(__builtin_offsetof(Scene, m_cameras) == 0x140, "K1 Scene");
K2_LAYOUT_ASSERT(__builtin_offsetof(Scene, m_focusGob) == 0x150, "K1 Scene");
K2_LAYOUT_ASSERT(sizeof(Scene) == 0x3a8, "K1 Scene");
#endif

#endif // SCENE_H

// CAurRoom is a renderer room (a visibility cell of an area), derived from BSPNode.
#include "bsp.h"

class CAurObject;

struct CAurRoom : BSPNode {

    // The rooms visible from this one.
    List<CAurRoom*> m_visibleRooms;

    // The objects this room owns; deleted on teardown.
    List<CAurObject*> m_objects;

    // The single object SpawnRoom stores for this room.
    CAurObject* m_pObject;

    // Name buffer.
    char   m_szName[64];

#if !KOTOR1  // K1's room has no background-room flag
    char   m_bHasBackgroundGeometry;
#endif

    CAurRoom();
    CAurRoom(char* pszName);
    ~CAurRoom();
};

#if KOTOR1
K2_LAYOUT_ASSERT(sizeof(CAurRoom) == 0xe0, "K1 CAurRoom (new 0xe0 in Scene::SpawnRoom)");
#endif

void CAurRoomGetBBox(CAurRoom* pRoom, Vector* pMin, Vector* pMax);

#ifndef CHITINFO_H
#define CHITINFO_H

// The scene hitcheck's in/out parameter block, filled by Scene::HitCheck.

class CAurObject;

class CHitInfo {
public:
    int m_bSkipObjects;
    int m_nMaterialMask;
    List<CAurObject*> m_ignoreList;
    int m_bHitBoundingBoxes;
    int m_nHitType;
    Vector m_hitPosition;
    Vector m_vLocalHitPosition;
    int m_nHitMaterialBit;
    SafePointer<CAurObject> m_hitObject;
    int m_nHitFaceIndex;
    Vector m_vHitNormal;
    char* m_pszHitPartName;
    void* m_pHitTriggerOwner;
    void* m_pHitRoom;
    int m_bIgnoreStartInside;
    unsigned char m_bSkipCharactersAndItems;
    unsigned char m_bSkipFaded;
    unsigned char m_bHonorIgnoreHitCheck;
    unsigned char m_bSkipRooms;
    unsigned char m_bIncludeEffects;

    // m_nMaterialMask and m_nHitMaterialBit hold one bit per walkmesh surface material; material
    // 30 is the trigger surface.
    enum {
        kMaterialTrigger = 0x40000000,
    };
    enum : unsigned int {
        kMaterialAllButTrigger = 0xBFFFFFFF,
    };

    CHitInfo(int a, int b, int c);
    void Reset(int a, int b, int c);
    void ClearOutput();
    ~CHitInfo() __attribute__((used)) {}
};

extern CHitInfo DefaultHitInfo;

#endif // CHITINFO_H
