#pragma once
#include "k2stdint.h"

#include "Quaternion.h"
#include "Vector.h"
#include "Vector.h"
#include "Quaternion.h"
#include "Plane.h"
#include "List.h"

class Gob;
class Material;
class MdlNode;
class Part;

// A node of a model's part tree: the local transform, the child list and the owning object.
class CAurPart {
public:

    CAurPart() { __builtin_memset(&m_nNodeID, 0, sizeof(m_nNodeID) + 3 * sizeof(m_bDrawEnabled)); }

    virtual ~CAurPart() {}

    virtual char* Name() = 0;
    virtual void GetLocal(Vector& outPos, Quaternion& outOrient) = 0;
    virtual void GetWorld(Vector& outPos, Quaternion& outOrient) = 0;
    virtual List<Part*>* GetChildren() = 0;
    virtual Part* GetParent() = 0;
    virtual void* GetAurObject() = 0;
    MdlNode* m_pNode;
    Vector m_vLocalPosition;
    Quaternion m_qLocalOrientation;

    // The bits of 1.0f, the scale's initial value: the image stores it as an integer.
    enum
    {
        kScaleOneBits = 0x3f800000,
    };
    union {
        int   m_nScaleBits = kScaleOneBits;
        float m_fScale;
    } m_scale;
    int  m_nNodeID = 0;
    bool m_bDrawEnabled = false;
    bool m_bHologramRendering = false;
    bool m_bPerObjectIllumination = false;
    Part** m_paChildren = 0;
    int  m_nChildren = 0;
    int  m_nCapacity = 0;
    Part* m_pParent = 0;
    Gob* m_pGob = 0;
    Material* m_pMaterial = 0;
    int  m_nLocalPositionSet = 0;
};

#ifndef PART_H
#define PART_H

#include "aurmaterial.h"

extern int countpart;

struct CAurShadowData;
class PartEmitter;
class PartLight;
class PartProjection;
class PartReference;
class PartSkin;
class PartTriMesh;
class PartAnimMesh;
class PartDanglyMesh;
class CHitInfo;
struct Plane;

// Base of the part hierarchy: owns the child parts and the material, and carries the virtual
// interface the renderer and the geometry code dispatch through.
class Part : public CAurPart {
public:

    Part(MdlNode* p);
    Part() = default;
    char* Name();
    List<Part*>* GetChildren();
    Part* GetParent();
    void* GetAurObject();
    void GetLocal(Vector& outPos, Quaternion& outOrient);
    void GetWorld(Vector& outPos, Quaternion& outOrient);
    static int GetDefaultOffset64(int nIndex);

#if defined(PART_DTOR_DECLARED_ONLY) || KOTOR1  // K1 defines ~Part out of line
    virtual ~Part();
#else
    virtual ~Part()
    {
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

    virtual void* HitCheck(Vector, Vector, Vector*, CHitInfo*, float);

#if !KOTOR1  // K1 has no printMemory slot
    virtual int printMemory();

#endif
    virtual int HitCheckGeom(Vector const&, Vector&, Vector*, CHitInfo*, float);

    virtual PartLight* AsPartLight() { return 0; }

    virtual void* AsPartCamera() { return 0; }

    virtual PartTriMesh* AsPartTriMesh() { return 0; }

    virtual PartAnimMesh* AsPartAnimMesh() { return 0; }

    virtual PartEmitter* AsPartEmitter() { return 0; }

    virtual PartReference* AsPartReference() { return 0; }

    virtual PartProjection* AsPartProjection() { return 0; }

    virtual void* AsPartShadow() { return 0; }

    virtual void* AsPartBeam() { return 0; }

    virtual void* AsPartAABB() { return 0; }

    virtual PartDanglyMesh* AsPartDanglyMesh() { return 0; }

    virtual PartSkin* AsPartSkin() { return 0; }

    virtual void* AsPartTrigger() { return 0; }

    virtual void* AsPartDummy() { return this; }

    virtual void Project(Vector const&, float, Plane const&, CAurShadowData*);
    virtual void Mat();
    virtual void Draw(bool);
    virtual void Animate(float);
    virtual void PostProcess(Part*);
    virtual Vector SetPartPosition(Vector);
    virtual void GetMinimumBoundingBox(Vector&, Vector&, float);
    virtual void GetMinimumSphere(Vector&, float&);

#if KOTOR1  // K1-only GetMinimumSphere cache

    struct GetMinimumSphere_cache_data {
        float  fLocalRadius;
        bool   bDrawEnabled;
        int    nAlwaysDomin;
        bool   bValid;
        Vector vCentre;
        float  fRadius;
        List<GetMinimumSphere_cache_data*> lChildren;
    };
    GetMinimumSphere_cache_data* GetMinimumSphere_cache_GetData();
    bool GetMinimumSphere_cache(GetMinimumSphere_cache_data* pData);
    float GetMinimumSphere_localradius();
#endif
};

#if KOTOR1  // K1-only cache size check
K2_LAYOUT_ASSERT(sizeof(Part::GetMinimumSphere_cache_data) == 0x30, "K1 operator new(0x30) @0x5b627c");
#endif

#endif

#ifndef PARTTRIMESH_H
#define PARTTRIMESH_H

struct CAurShadowData;
class CHitInfo;
struct Plane;
class TileData;
struct Vector;
class VertexPrimitive;

// A part with a vertex primitive and an index buffer.
class PartTriMesh : public Part {
public:
    // The render paths GetRenderPath picks and Draw dispatches on, named after the Render*
    // function each one calls (NOISE_MAPPED is KOTOR II's).
    enum {
        RENDER_PATH_FLAT                      = 0,
        RENDER_PATH_FLAT_VP                   = 1,
        RENDER_PATH_EMBM                      = 2,
        RENDER_PATH_LIGHTMAPPED_BUMP_SPECULAR = 3,
        RENDER_PATH_LIGHTMAPPED_BUMP_DIFFUSE  = 4,
        RENDER_PATH_BUMPMAPPED                = 5,
        RENDER_PATH_BUMPMAP_FALLBACK          = 6,
        RENDER_PATH_EMLM                      = 7,
        RENDER_PATH_EMLM_VP                   = 8,
        RENDER_PATH_NV1X_EMLM                 = 9,
        RENDER_PATH_ENVIRONMENT_MAPPED        = 10,
        RENDER_PATH_ENVIRONMENT_MAPPED_VP     = 11,
        RENDER_PATH_LIGHTMAPPED               = 12,
        RENDER_PATH_LIGHTMAPPED_VP            = 13,
        RENDER_PATH_LIGHTMAPPED_NO_SHADERS    = 14,
        RENDER_PATH_NORMAL_MAPPED             = 15,
        RENDER_PATH_NOISE_MAPPED              = 16,
    };

    PartTriMesh(MdlNode* node);
    int GetRenderPath();
    static bool m_bMaterialHasChanged;
    static int m_nNextPartID;
    static unsigned short m_VertexIndices[];
    virtual ~PartTriMesh();

#if !KOTOR1  // K1 has no printMemory slot
    virtual int printMemory();

#endif
    virtual int HitCheckGeom(Vector const&, Vector&, Vector*, CHitInfo*, float);
    virtual PartTriMesh* AsPartTriMesh() { return this; }

    virtual void* AsPartDummy() { return 0; }

    virtual void Project(Vector const&, float, Plane const&, CAurShadowData*);
    virtual void Mat();
    virtual void Draw(bool);
    virtual void GetMinimumSphere(Vector&, float&);
    virtual void RotateTexture(float);
    virtual int  WithinRange(Vector, float);

    virtual float GetTextureAngle() { return m_fTextureAngle; }

    virtual void TransformTextureMatrix();
    virtual void RestoreTextureMatrix();
    virtual void TransformModelviewMatrix();
    virtual void RestoreModelviewMatrix();
    Vector m_vBBoxMin;
    Vector m_vBBoxMax;
    Vector m_vSelfIllumColor;
    VertexPrimitive* m_pVertexPrimitive = 0;
    int m_nColorPool = -1;
    int m_nUVPool = -1;
    bool m_bHasColorPool = 0;
    bool m_bFog = 1;
    TileData* m_pTileData = 0;
    float m_fOpacity = 1.0f;
    char _padac[0xb0 - 0xac];
    int  m_nPartID;
    float* m_pafBaseUVs;  // (u, v) per vertex, the first UV set

#if KOTOR1  // K1 caches GetRenderPath() here
    int   m_nRenderPath = -1;
#endif
    float m_fTextureAngle = 0.0f;
};

#endif

#ifndef PARTAABB_H
#define PARTAABB_H

class CHitInfo;
struct Vector;

// Walkmesh part: collision geometry rather than something drawn.
class PartAABB : public PartTriMesh {
public:

    PartAABB(MdlNode* node) : PartTriMesh(node) {}

    virtual int HitCheckGeom(Vector const&, Vector&, Vector*, CHitInfo*, float);
    virtual void* AsPartAABB();
    virtual void* AsPartDummy();
    virtual void Draw(bool);
};

#endif

#ifndef PARTANIMMESH_H
#define PARTANIMMESH_H

class CHitInfo;
class MdlNode;
struct Vector;

// A tri-mesh whose vertices also come from the node's animation track.
class PartAnimMesh : public PartTriMesh {
public:

    PartAnimMesh(MdlNode* pNode) : PartTriMesh(pNode) {}

    virtual int HitCheckGeom(Vector const&, Vector&, Vector*, CHitInfo*, float);

    virtual PartAnimMesh* AsPartAnimMesh() { return this; }
    virtual void* AsPartDummy() { return 0; }
    virtual void Draw(bool);
};

#endif

#ifndef PARTCAMERA_H
#define PARTCAMERA_H

// Camera part; only the AsPartCamera probe distinguishes it.
class PartCamera : public Part {
public:

    virtual void* AsPartCamera() { return this; }

    virtual void* AsPartDummy();
};

#endif

#ifndef PARTDANGLYMESH_H
#define PARTDANGLYMESH_H


class MdlNode;
struct Vector;

// Cloth-like mesh: the vertices are simulated from the dangly constraints.
class PartDanglyMesh : public PartTriMesh {
public:

    PartDanglyMesh(MdlNode* node);
    virtual ~PartDanglyMesh();

#if !KOTOR1  // K1 has no printMemory slot
    virtual int printMemory();
#endif

    virtual PartDanglyMesh* AsPartDanglyMesh() { return this; }
    virtual void* AsPartDummy() { return 0; }
    virtual void Draw(bool);
    virtual void Animate(float);
    virtual Vector SetPartPosition(Vector);
    Vector m_vLastPosition;
    Quaternion m_qLastOrientation;
    Vector* m_paVertices;
    int    m_nVertexPool;
};

#endif

#ifndef PARTLIGHT_H
#define PARTLIGHT_H

// A light embedded in the model; the owner collects these when the object is added to a scene.
class PartLight : public Part {
public:

    PartLight(MdlNode* p);
    virtual ~PartLight();

#if !KOTOR1  // K1 has no printMemory slot
    virtual int printMemory();
#endif
    virtual PartLight* AsPartLight();
    virtual void* AsPartDummy();
    virtual void Mat();
    virtual void Draw(bool);
    Vector m_vLightColour;
    float m_fLightRadius = 0.0f;
    int  m_nCastsShadows = 0;
    float m_fShadowRadius = -1.0f;
    float m_fVerticalDisplacement = 0.0f;
    int  m_nAmbientOnly = 0;
    int  m_nAffectDynamic = 0;
    int  m_nIsDynamic = 0;
    int  m_nLightPriority = 0;
    float m_fEmitterScale = 1.0f;
    float m_fFadeWeightAlt = 0.0f;
    float m_fDistanceRatio = 0.0f;
    float m_fCullDistance = 0.0f;
    float m_fFlareFade = 0.0f;
    float m_fLightScale = 1.0f;
    float m_fShadowFade = 0.0f;
    Vector m_vWorldPosition;
};

#endif

#ifndef PARTLIGHTSABER_H
#define PARTLIGHTSABER_H

// Lightsaber blade: a trail of segments the animation ages out.
class PartLightsaber : public PartTriMesh {
public:

    PartLightsaber(MdlNode* node);
    virtual ~PartLightsaber();
    virtual void* AsPartDummy();
    virtual void Draw(bool);
    virtual void Animate(float);
    float m_afSegmentLife[25];
    Vector m_avSegmentPos[25];
    Quaternion m_aqSegmentOrient[25];
    short m_nOrientIndex;
    short m_nLifeIndex;
    short m_nSegmentCount;
    short _pad3ea;
    Vector m_vLastSegmentPos;
};

#if KOTOR1  // K1 part is 8 bytes larger
K2_LAYOUT_ASSERT(sizeof(PartLightsaber) == 0x400, "K1 PartLightsaber size");
#else
K2_LAYOUT_ASSERT(sizeof(PartLightsaber) == 0x3f8, "pinned by the factory's operator new(0x3f8)");
#endif

#endif

#ifndef PARTREFERENCE_H
#define PARTREFERENCE_H

class CAurObject;

// Reference part: an instance of another model.
class PartReference : public Part {
public:

    virtual PartReference* AsPartReference();
    virtual void* AsPartDummy();
    CAurObject* m_pObject = 0;

#ifdef _MSC_VER
    virtual ~PartReference();   // cl binds the delete where it parses it
#else
    virtual ~PartReference() { DeleteObject(m_pObject); }
    static void DeleteObject(CAurObject* pObject);
#endif

    char _pad70[0x78 - 0x70];
};

#endif

#ifndef PARTSKIN_H
#define PARTSKIN_H

struct CAurShadowData;
struct Plane;
struct Vector;

// Skinned mesh: the vertices are deformed by the node's bone weights.
class PartSkin : public PartTriMesh {
public:

    PartSkin(MdlNode* node) : PartTriMesh(node) {}

    virtual ~PartSkin();

    virtual PartSkin* AsPartSkin() { return this; }
    virtual void* AsPartDummy() { return 0; }
    virtual void Project(Vector const&, float, Plane const&, CAurShadowData*);
    virtual void Draw(bool);
    virtual void PostProcess(Part*);
    void DrawBumpedOutShell();
    void DrawBumpedOutShellVP();
};

#endif

#include "List.h"
#include "SafePointer.h"

struct AnimRun;
class CAurBehavior;
class CAurCamera;
class CAurPart;
class CAurScene;
class CHitInfo;
class CallBack;
class Gob;
class Part;
class Scene;

// Base of everything the renderer can place in a scene: a named, animated object with its own part
// tree, lights and callbacks. Gob is the game-side subclass.
class CAurObject : public SafePointerList<CAurObject> {  // idiom-exempt: canonical (home is libsource/aurora, alongside Gob/CAurScene)
public:

    virtual ~CAurObject() {}
    virtual CAurCamera* AsCAurCamera() = 0;
    virtual char* GetName() = 0;
    virtual char* GetModelName() = 0;
    virtual void  Render(bool bParam) = 0;
    virtual void  Animate(float fParam) = 0;
    virtual int   PlayAnimation(char* pszName, float fParam, int nParam, float fParam2) = 0;
    virtual AnimRun* GetAnimation(char* pszName, float* pfParam1, float* pfParam2) = 0;
    virtual bool  LoadAddInAnimations(char* pszName) = 0;
    virtual void  RemoveAddInAnimations() = 0;
    virtual CallBack* RegisterCallBack(void (*cb)(CAurObject*, char*, void*),
                                       char* a, void* b, float c, bool d) = 0;
    virtual void  DeregisterCallback(CallBack* a) = 0;
    virtual int   ReplaceTexture(char* a, char* b) = 0;
    virtual int   ReplaceTexture(char* a, char* b, int c,
                                 unsigned short* d, int e) = 0;
    virtual int   ReplaceTextureSubtree(char* a, char* b, int c,
                                        unsigned short* d) = 0;
    virtual int   RestoreTexture() = 0;
    virtual int   SetCanDownSample(char a) = 0;
    virtual int   SetTextureAttributes(char* a, char* b) = 0;
    virtual int   SetEnvironmentMap(char* a) = 0;
    virtual int   AttachToScene(CAurScene* a) = 0;
    virtual void* AttachToObject(CAurObject* a, char* b, int c) = 0;
    virtual void* AttachToPart(CAurObject* a, CAurPart* b, int c) = 0;
    virtual void  SetBehavior(CAurBehavior* a) = 0;
    virtual Vector SetPosition(Vector a) = 0;   // on Win64 a Vector return is a hidden first argument
    virtual void  SetOrientation(Quaternion a) = 0;
    virtual Vector GetPosition() = 0;
    virtual Quaternion GetOrientation() = 0;
    virtual Vector GetPreviousPosition() = 0;
    virtual void* GetExternalData() = 0;
    virtual void  SetExternalData(void*) = 0;
    virtual int   GetExternalDataType() = 0;
    virtual void  SetExternalDataType(int) = 0;
    virtual void  InheritExternal(CAurObject*) = 0;
    virtual void  HitCheckHack(Vector, Vector, Vector*) = 0;
    virtual Gob*  HitCheckPart(CAurPart*, Vector, Vector, Vector*, CHitInfo*) = 0;
    virtual void* HitCheck(Vector, Vector, Vector*, CHitInfo*) = 0;
    virtual void  SetIgnoreHitCheck(bool) = 0;
    virtual int   SetPartPosition(char*, Vector, Quaternion) = 0;
    virtual int   GetPartPosition(char*, Vector*, Quaternion*) = 0;
    virtual int   SetPartLocalPosition(char*, Vector, Quaternion) = 0;
    virtual int   GetPartLocalPosition(char*, Vector*, Quaternion*) = 0;
    virtual void  ReattachReference(CAurObject*, char*) = 0;
    virtual void  BehaviorMessage(char*) = 0;
    virtual bool  Append(char*, char*) = 0;
    virtual bool  Amputate(char*) = 0;
    virtual void  SetColorShifting(Vector, float, int) = 0;
    virtual void  SetIllumination(Vector, int) = 0;
    virtual void  TurnOffShadows() = 0;
    virtual void  TurnOnShadows() = 0;
    virtual void  SetLightPriority(int) = 0;
    virtual float GetMaximumLightRadius() = 0;
    virtual bool  SetObjectScale(float, bool) = 0;
    virtual void  EnableRenderGobBBox(const Vector&) = 0;
    virtual void  DisableRenderGobBBox() = 0;
    virtual void  SetWind(bool, float, float, float) = 0;
    virtual void  EnableForceAnimations() = 0;
    virtual void  DisableForceAnimations() = 0;
    virtual void  DisableFog() = 0;
    virtual void  EnableFog() = 0;
    virtual void  GetBoundingBox(Vector&, Vector&) = 0;
    virtual void  GetMinimumBoundingBox(Vector&, Vector&) = 0;
    virtual void  GetMinimumBoundingSphere(Vector&, float&) = 0;
    virtual void  EnableDistortion() = 0;
    virtual void  DisableDistortion() = 0;
    virtual void  SetAsCharacter() = 0;
    virtual CAurScene* GetScene() = 0;
    virtual CAurBehavior* GetBehavior(int nBehaviorId) = 0;
    virtual Part* GetPart(char*) = 0;
    virtual void  EnumParts(int (*cb)(CAurPart*), List<CAurPart*>* parts) = 0;
    virtual void  SetRenderPersonalSpace(bool) = 0;
    virtual void  BeginLookAtAnimate(CAurObject*, char*, char*, float, float, float, float) = 0;
    virtual void  EndLookAtAnimate() = 0;
    virtual void  PlayOutOfOrderAnimation(char*, float*, float*, int, float) = 0;
    virtual void  StopOutOfOrderAnimation(char*) = 0;
    virtual void  AddAnimationFlag(int) = 0;
    virtual void  RemoveAnimationFlag(int) = 0;
    virtual void  EnableAlwaysRender() = 0;
    virtual void  DisableAlwaysRender() = 0;
    virtual void  EnableInCutscene() = 0;
    virtual void  DisableInCutscene() = 0;
    virtual Vector GetCutsceneDummyPosition() = 0;
    virtual void  SetMotionBlurred(bool) = 0;
    virtual bool  GetMotionBlurred() = 0;
    virtual void  RenderBlur() = 0;
    virtual void  EnableRenderBumpedOut(char*, float) = 0;
    virtual void  DisableRenderBumpedOut() = 0;
    void DieWithoutParent();
    void AnimateExternal(float fSpeed);
};

#ifdef _MSC_VER
inline PartReference::~PartReference() { delete m_pObject; }
#else
inline void PartReference::DeleteObject(CAurObject* pObject) { delete pObject; }
#endif

#ifndef CAUROBJECTLIGHTCONTAINER_H
#define CAUROBJECTLIGHTCONTAINER_H

class PartLight;

// Collects the lights under one object and the weight behind the selected one.
class CAurObjectLightContainer {
public:

    CAurObjectLightContainer() : m_pSelectedLight(0), m_fSelectedWeight(0.0f) {}
    virtual ~CAurObjectLightContainer() {}

    List<PartLight*> m_lights;
    List<float>      m_lightWeights;
    PartLight* m_pSelectedLight;
    float      m_fSelectedWeight;
};

#endif

#include "aurora.h"

class Animation;
class CExoString;
class CAurCamera;
struct CAurRoom;
class CAurScene;
class CAurObject;
class CAurObjectLightContainer;
class CAurPart;
class CAurBehavior;
class CAurTexture;
class CHitInfo;
class CallBack;
class Material;
class Model;
class Part;
class PartLight;
class PartTriMesh;
class Scene;
class StringGob;
class TileData;

class LookAtTarget {
public:
    virtual ~LookAtTarget();
};

// One animation playing on a Gob: the current time, blend state and playback scale.
struct AnimRun {
    // The PlayAnimation flags a run keeps in m_nFlags.
    enum {
        kAnimOnce = 0x1,            // plays to its end and blends out; without it the run loops
        kAnimNoBlendIn = 0x2,       // starts at full weight
        kAnimLoopEvents = 0x4,      // fires "loop" each time it wraps
        kAnimOverlay = 0x8,         // goes in above every run that is not out-of-order, and keeps
                                    // the runs below it once blended in
        kAnimSyncTime = 0x10,       // starts at the time of the run below it
        kAnimQueue = 0x20,          // waits below the current run; "start" fires when it starts
        kAnimNoReuse = 0x40,        // never takes over a running run of the same name
        kAnimRemove = 0x80,         // drops the named run
        kAnimTrace = 0x100,         // reports itself through the trace statics
        kAnimOutOfOrder = 0x200,    // an OOAnimRun, keyed by its own arrays rather than a tree
        kAnimStop = 0x400,          // blends out now
    };

    AnimRun(Animation* pAnimation);
    static int   m_AnimFlags;
    static AnimRun* m_pTraceRun;
    static float m_AnimLength;
    Animation* m_pAnimation;
    float m_fSpeed;
    int  m_nFlags;
    float m_fPrevTime;
    float m_fTime;
    float m_fBlendAlpha;
    float m_fBlendDuration;
    float m_fScale;
    int   m_nFinished;
};

// An AnimRun driven by an explicit key list instead of a single track.
struct OOAnimRun : AnimRun {
    OOAnimRun(Animation* pAnimation, float* pfTimes, float* pfValues, int nKeys, float fDuration);
    ~OOAnimRun();
    float  m_fDuration;
    float* m_pfValues;
    float* m_pfTimes;
    int    m_nKeys;
};

// A game object: a model instance in a scene, with its animation runs, attachments and visibility
// state.
class Gob : public CAurObject {
public:

    // Whether this Gob draws into the force-push distortion buffer: flagged itself, or its model is.
    bool IsForcePushDistortion();

    Gob(char* pszName);
    virtual ~Gob();
    virtual CAurCamera* AsCAurCamera();
    virtual char* GetName();
    virtual char* GetModelName();
    virtual void Render(bool a);
    virtual void Animate(float a);
    virtual int PlayAnimation(char* a, float b, int c, float d);
    virtual AnimRun* GetAnimation(char* pszName, float* pfTime, float* pfLength);
    virtual bool LoadAddInAnimations(char* pszName);
    virtual void RemoveAddInAnimations();
    virtual CallBack* RegisterCallBack(void (*cb)(CAurObject*, char*, void*),
                                  char* a, void* b, float c, bool d);
    virtual void DeregisterCallback(CallBack* a);
    virtual int  ReplaceTexture(char* a, char* b);

    virtual int  ReplaceTexture(char* a, char* b, int c,
                                unsigned short* d, int e);

    virtual int  ReplaceTextureSubtree(char* a, char* b, int c,
                                       unsigned short* d);
    virtual int  RestoreTexture();
    virtual int  SetCanDownSample(char a);
    virtual int  SetTextureAttributes(char* a, char* b);
    virtual int  SetEnvironmentMap(char* a);
    virtual int  AttachToScene(CAurScene* a);
    virtual void* AttachToObject(CAurObject* a, char* b, int c);
    virtual void* AttachToPart(CAurObject* a, CAurPart* b, int c);
    virtual void SetBehavior(CAurBehavior* a);
    virtual Vector SetPosition(Vector a);
    virtual void SetOrientation(Quaternion a);
    virtual Vector GetPosition();
    virtual Quaternion GetOrientation();
    virtual Vector GetPreviousPosition();
    virtual void* GetExternalData();
    virtual void SetExternalData(void* a);
    virtual int  GetExternalDataType();
    virtual void SetExternalDataType(int a);
    virtual void InheritExternal(CAurObject* a);
    virtual void HitCheckHack(Vector a, Vector b, Vector* c);
    virtual Gob* HitCheckPart(CAurPart* a, Vector b, Vector c,
                              Vector* d, CHitInfo* e);
    virtual void* HitCheck(Vector a, Vector b, Vector* c, CHitInfo* d);
    virtual void SetIgnoreHitCheck(bool a);
    virtual int  SetPartPosition(char* a, Vector b, Quaternion c);
    virtual int  GetPartPosition(char* a, Vector* b, Quaternion* c);
    virtual int  SetPartLocalPosition(char* a, Vector b, Quaternion c);
    virtual int  GetPartLocalPosition(char* a, Vector* b, Quaternion* c);
    virtual void ReattachReference(CAurObject* a, char* b);
    virtual void BehaviorMessage(char* a);
    virtual bool Append(char* a, char* b);
    virtual bool Amputate(char* a);
    virtual void SetColorShifting(Vector a, float b, int c);
    virtual void SetIllumination(Vector a, int b);
    virtual void TurnOffShadows();
    virtual void TurnOnShadows();
    virtual void SetLightPriority(int a);
    virtual float GetMaximumLightRadius();
    virtual bool SetObjectScale(float a, bool b);
    virtual void EnableRenderGobBBox(const Vector& a);
    virtual void DisableRenderGobBBox();
    virtual void SetWind(bool a, float b, float c, float d);
    virtual void EnableForceAnimations();
    virtual void DisableForceAnimations();
    virtual void DisableFog();
    virtual void EnableFog();
    virtual void GetBoundingBox(Vector& a, Vector& b);
    virtual void GetMinimumBoundingBox(Vector& a, Vector& b);
    virtual void GetMinimumBoundingSphere(Vector& a, float& b);
    virtual void EnableDistortion();
    virtual void DisableDistortion();
    virtual void SetAsCharacter();
    virtual CAurScene* GetScene();
    virtual CAurBehavior* GetBehavior(int a);
    virtual Part* GetPart(char* name);
    virtual void EnumParts(int (*cb)(CAurPart*), List<CAurPart*>* parts);
    virtual void SetRenderPersonalSpace(bool a);
    virtual void BeginLookAtAnimate(CAurObject* target, char* bone, char* headBone,
                                    float arcH, float arcV, float f3, float f4);
    virtual void EndLookAtAnimate();

    virtual void PlayOutOfOrderAnimation(char* a, float* b, float* c,
                                         int d, float e);
    virtual void StopOutOfOrderAnimation(char* a);
    virtual void AddAnimationFlag(int a);
    virtual void RemoveAnimationFlag(int a);
    virtual void EnableAlwaysRender();
    virtual void DisableAlwaysRender();
    virtual void EnableInCutscene();
    virtual void DisableInCutscene();
    virtual Vector GetCutsceneDummyPosition();
    virtual void SetMotionBlurred(bool a);
    virtual bool GetMotionBlurred();
    virtual void RenderBlur();
    virtual void EnableRenderBumpedOut(char* a, float b);
    virtual void DisableRenderBumpedOut();
    virtual void SetMaxBlurLength(float a);
    virtual float GetMaxBlurLength();

#if !KOTOR1  // force points, alignment and hologram slots are K2-only
    virtual void SetForcePoints(short a);
    virtual unsigned short GetForcePoints();
    virtual void SetAlignmentValue(short a);
    virtual unsigned short GetAlignmentValue();
    virtual void SetForceGlowColor(float a, float b, float c, float d);
    virtual void GetForceGlowColor(float& a, float& b, float& c, float& d);
    virtual void EnableForceGlow(bool a);
    virtual bool IsForceGlowEnabled();
    virtual void SetNonForceBlending(bool a);
    virtual bool IsNonForceBlending();
    virtual void SetEmitterAsWeather(bool a);
    virtual void EnableHologram(bool a);
    virtual bool IsHologramEnabled();
    virtual void EnableIllumination(bool a, float b, float c, float d);
    virtual bool IsIlluminationEnable();
    virtual void GetIlluminationColor(float& r, float& g, float& b);
    virtual void EnableRendering(bool a);
    virtual bool IsRenderingEnabled();
#endif
    virtual void* AsCamera();
    virtual CAurObject* AsCAurObject();
    virtual StringGob* AsString();
    virtual void DoEvent(char* a, void* b);
    virtual bool IsForceAnimated();
    virtual bool IsAnimating();
    virtual void EnableInViewVolume();
    virtual void DisableInViewVolume();
    virtual bool IsInViewVolume();
    virtual void GetMinimumBoundingBox();
    virtual void GetMinimumSphere();
    virtual void SetUpImposters(int a, float* b);
    virtual bool IsAlwaysRendering();
    bool IsInCutscene();
    bool VisibilityCheck();
    bool CheckEvent(float fTime, bool bCheckViewVolume);
    bool ShouldWeCheckVisibilityHACK();
    void SetProcessFlag();
    void SetScene(Scene* pScene);
    void SetAllEmitterAsWeather(Part* pPart, bool b);
    void EnableHologramRendering(Part* pPart, bool b);
    void EnablePerObjectIllumination(Part* pPart, bool b);
    void EnumParts(Part* pPart, int (*cb)(CAurPart*), List<CAurPart*>* pa);
    void ProcessGob();
    void ProxyPartDraw(Part* pPart, bool b);
    void PartDraw(Part* pPart, bool b);
    void PartVolumeProject(Part* pPart, Vector v, float f, Plane p);
    void CollectLights(List<PartLight*>& lights);
    void  RenderBoundingBox();
    void  AddAttachment(Gob* pGob);
    void  RemoveAttachment(Gob* pGob);
    void  Reset();
    float SetAnimationTime(char* pszName, float fTime);
    float SetAnimationPercentage(char* pszName, float fPercentage);
    bool  m_bForceAnimated;
    bool  m_bInViewVolume;
    bool  m_bAlwaysRendering;
    bool  m_bBonesDirty;
    bool  m_bInCutscene;
    char  m_cRenderBumpedOut;
    bool  m_bAnimatedOnce;
    char  m_cForcePose;
    char  m_nVisible;
    char  _pad21[0x24 - 0x21];
    float m_fVisibilityCheckTimer;

#if !KOTOR1  // force points, alignment and hologram are K2-only
    unsigned short m_nForcePoints;
    unsigned short m_nAlignmentValue;
    bool  m_bNonForceBlending;
    bool  m_bForceGlowEnabled;
    float m_fForceGlowR;
    float m_fForceGlowG;
    float m_fForceGlowB;
    float m_fForceGlowA;
    bool  m_bHologramEnabled;
    bool  m_bIlluminationEnabled;
    char  _pad42[0x44 - 0x42];
    float m_fIlluminationR;
    float m_fIlluminationG;
    float m_fIlluminationB;
    bool  m_bRenderingEnabled;
#endif

    List<CAurRoom*> m_rooms;
    float m_fCheckEventTime;
    int   m_nImposterHandle;
    int   m_nImposters;
    char  m_szName[32];
    Model* m_pModel;
    Model* m_pLodModelX;
    Model* m_pLodModelZ;
    Model* m_pAddInModel;
    Scene* m_pScene;
    Part* m_pPart;
    Part* m_pLodPartX;
    Part* m_pLodPartZ;
    Vector m_vPosition;
    Quaternion m_qOrientation;
    Vector m_vPreviousPosition;
    Quaternion m_qPreviousOrientation;
    List<AnimRun*> m_paAnimations;
    int   m_nDynamicLights;
    char  m_cCanDownSample;
    char  _pad125[0x126 - 0x125];
    short m_nShadows;
    short m_nColorShifting;
    short m_nIllumination;
    short m_nRenderGobBBox;
    Vector m_vColorShift;
    float m_fColorShiftAmount;
    float m_fFadeAlpha;
    Vector m_vIllumination;
    List<Model*> m_listModels;
    List<Part*>  m_listParts;
    float m_fViewDistance;
    float m_fPersonalRadius;
    float m_fCreatureRadius;
    CAurPathInfo m_pathInfo;
    float m_fBumpOutDistance;
    Vector m_vBoundingBoxMin;
    Vector m_vBoundingBoxMax;
    float m_fMinSphereRadius;
    Vector m_vMinSphereCenter;
    Vector m_vGobBBox;
    bool  m_bRenderPersonalSpace;
    bool  m_bMotionBlurred;
    bool  m_bWind;
    char  _pad1d3[0x1d4 - 0x1d3];
    float m_fWindRadius;
    float m_fWindStrength;
    float m_fWindLife;
    bool  m_bHasSingleTexture;
    char  m_bReserved1;
    bool  m_bHasGeometry;
    bool  m_bProcessFlag;
    Material* m_pSingleMaterial;
    PartTriMesh* m_pSingleLightPart;
    TileData* m_pTileData;
    void* m_pLookAt;
    CAurObjectLightContainer* m_pLightContainer;
    bool  m_bDistortion;
    bool  m_bIgnoreHitCheck;
    bool  m_bAnimating;
    bool  m_bHasTransparency;
    float m_fMaxBlurLength;
    bool  m_bAnimateUV;
    CAurBehavior* m_pBehavior;
    int   m_nLastAnimateTick;
    List<CallBack*> m_callbacks;
    List<PartLight*>    m_partLights;
    SafePointer<CAurTexture> m_texWatch;
    void* m_pExternalData;
    int   m_nExternalDataType;
    float m_fObjectScale;
    float m_fObjectScaleBase;
    List<Gob*> m_attachments;
    int   m_nReserved1;

#if !KOTOR1  // K1 has no pose-gate words

    int   m_nPoseDisabled;
    int   m_nReserved2;
#endif
    char  m_cHasSkin;
};

#if KOTOR1  // K1 Gob layout mirrors
K2_LAYOUT_ASSERT(__builtin_offsetof(Gob, m_rooms) == 0x28, "K1 Gob layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(Gob, m_szName) == 0x44, "K1 Gob layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(Gob, m_bRenderPersonalSpace) == 0x1a0, "K1 Gob layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(Gob, m_pExternalData) == 0x228, "K1 Gob layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(Gob, m_nReserved1) == 0x250, "K1 Gob layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(Gob, m_cHasSkin) == 0x254, "K1 Gob layout");
K2_LAYOUT_ASSERT(sizeof(Gob) == 0x258, "K1 Gob size");
#endif

extern int countmaxfaces;

// A triangle from the 3ds Max mesh importer, before it is split into render bins.
class MaxFace {
public:
    MaxFace();

#ifdef MAXFACE_DTOR_DECLARED_ONLY

    ~MaxFace();
#else
    __attribute__((used)) ~MaxFace() { countmaxfaces--; }
#endif

    Vector& Normal() { return *reinterpret_cast<Vector*>(this); }

    union {
        float m_fNormal[3];
        int   m_nNormal[3];
    };
    float m_fPlaneDist;
    union {
        int m_nSurfaceMaterial;
        int m_nMaterial;
    };
    short m_anNeighbors[3];

    union {
        unsigned short m_anVertIndices[3];
        short m_nVIndex[3];
    };
};

// Per-face MeshSmooth data carried alongside MaxFace.
class MaxFace_PostProcess {
public:
    MaxFace_PostProcess();

#ifdef MAXFACE_PP_DTOR_DECLARED_ONLY
    ~MaxFace_PostProcess();
#else
    __attribute__((used)) ~MaxFace_PostProcess()
    {
        delete[] m_paVertIndices;
        delete[] m_paTVertIndices;
        delete[] m_paNormalIndices;
        m_nSmoothGroup = 0;
        m_nMaterial = 0;
    }
#endif
    int* m_paVertIndices;
    int* m_paTVertIndices;
    int  m_nSmoothGroup;
    int  m_nMaterial;
    int* m_paNormalIndices;
    static List<Vector> texindices1;
    static List<Vector> texindices2;
    static List<Vector> texindices3;
};

#include "auranim.h"

class MdlNodeReference;
class MdlNodeTriMesh;
class MdlNodeLight;
class MdlNodeAABB;
class MdlNodeDanglyMesh;
class MdlNodeSkin;
class MdlNodeAnimMesh;
class MdlNodeLightsaber;
class MdlNodeTrigger;
class MdlNodeEmitter;
class MdlNodeCamera;
class MaxTree;
class Part;
class Gob;

// MdlNode::m_nFlags: the node's parts as the public MDL format names them, one bit each, and the
// node types they add up to (every node has a header; a skin is a mesh with skin data, and so on).
enum MdlNodeFlag {
    NODE_HAS_HEADER = 0x1,
    NODE_HAS_LIGHT = 0x2,
    NODE_HAS_EMITTER = 0x4,
    NODE_HAS_CAMERA = 0x8,
    NODE_HAS_REFERENCE = 0x10,
    NODE_HAS_MESH = 0x20,
    NODE_HAS_SKIN = 0x40,
    NODE_HAS_ANIM = 0x80,
    NODE_HAS_DANGLY = 0x100,
    NODE_HAS_AABB = 0x200,
    NODE_HAS_TRIGGER = 0x400,
    NODE_HAS_SABER = 0x800,

    NODE_DUMMY = NODE_HAS_HEADER,
    NODE_LIGHT = NODE_HAS_HEADER | NODE_HAS_LIGHT,
    NODE_EMITTER = NODE_HAS_HEADER | NODE_HAS_EMITTER,
    NODE_CAMERA = NODE_HAS_HEADER | NODE_HAS_CAMERA,
    NODE_REFERENCE = NODE_HAS_HEADER | NODE_HAS_REFERENCE,
    NODE_TRIMESH = NODE_HAS_HEADER | NODE_HAS_MESH,
    NODE_SKIN = NODE_TRIMESH | NODE_HAS_SKIN,
    NODE_ANIMMESH = NODE_TRIMESH | NODE_HAS_ANIM,
    NODE_DANGLYMESH = NODE_TRIMESH | NODE_HAS_DANGLY,
    NODE_AABB = NODE_TRIMESH | NODE_HAS_AABB,
    NODE_TRIGGER = NODE_HAS_HEADER | NODE_HAS_TRIGGER,
    NODE_SABER = NODE_TRIMESH | NODE_HAS_SABER,
};

// MdlNodeTriMesh::m_uVertexFlags: the per-vertex arrays a mesh's vertex record interleaves, as the
// public MDL format names the MDX data bits.
enum MdxDataFlag {
    MDX_VERTICES = 0x1,
    MDX_TEX0_VERTICES = 0x2,
    MDX_TEX1_VERTICES = 0x4,
    MDX_TEX2_VERTICES = 0x8,
    MDX_TEX3_VERTICES = 0x10,
    MDX_VERTEX_NORMALS = 0x20,
    MDX_VERTEX_COLORS = 0x40,
    MDX_TANGENT_SPACE = 0x80,
};

struct MdlNode32 {
    unsigned short m_nFlags;
    short          m_nNodeID;
    int            m_nNameIndex;
    int            m_pTree;
    int            m_pParent;
    Vector         m_vPosition;
    Quaternion     m_qOrientation;
    int            m_pChildren;
    int            m_nChildCount;
    int            m_nChildCap;
    int            m_pControllers;
    int            m_nControllerCount;
    int            m_nControllerCap;
    int            m_pFloats;
    int            m_nFloatCount;
    int            m_nFloatCap;
};
static_assert(sizeof(MdlNode32) == 0x50, "pinned by every derived mirror starting at +0x50");

// Base node of a .mdl file's node tree; the subclasses hold the geometry, lights and cameras the
// nodes describe.
class MdlNode {
public:

    static const unsigned short MDLNODE_BASE_ID;
    static const unsigned short MDLNODE_LIGHT_ID;
    static const unsigned short MDLNODE_AABB_ID;
    static const unsigned short MDLNODE_ANIM_ID;
    static const unsigned short MDLNODE_CAMERA_ID;
    static const unsigned short MDLNODE_DANGLY_ID;
    static const unsigned short MDLNODE_EMITTER_ID;
    static const unsigned short MDLNODE_REFERENCE_ID;
    static const unsigned short MDLNODE_SKIN_ID;
    static const unsigned short MDLNODE_TRIGGER_ID;
    static const unsigned short MDLNODE_TRIMESH_ID;
    static const unsigned short MDLNODE_LIGHTSABER_ID;
    ~MdlNode();

    MdlNode(const MdlNode32& p)
    {
        m_nFlags = p.m_nFlags;
        m_nNodeID = p.m_nNodeID;
        m_nNameIndex = p.m_nNameIndex;
        m_pTree = reinterpret_cast<MaxTree*>(static_cast<intptr_t>(p.m_pTree));
        m_pParent = static_cast<unsigned long long>(static_cast<long long>(p.m_pParent));
        m_vPosition = p.m_vPosition;
        m_qOrientation = p.m_qOrientation;
        m_listChildren.m_pData = reinterpret_cast<MdlNode**>(static_cast<intptr_t>(p.m_pChildren));
        __builtin_memcpy(&m_listChildren.m_count, &p.m_nChildCount, sizeof(int));
        __builtin_memcpy(&m_listChildren.m_capacity, &p.m_nChildCap, sizeof(int));
        m_listControllers.m_pData = reinterpret_cast<NewController*>(static_cast<intptr_t>(p.m_pControllers));
        __builtin_memcpy(&m_listControllers.m_count, &p.m_nControllerCount, sizeof(int));
        __builtin_memcpy(&m_listControllers.m_capacity, &p.m_nControllerCap, sizeof(int));
        m_listFloats.m_pData = reinterpret_cast<float*>(static_cast<intptr_t>(p.m_pFloats));
        __builtin_memcpy(&m_listFloats.m_count, &p.m_nFloatCount, sizeof(int));
        __builtin_memcpy(&m_listFloats.m_capacity, &p.m_nFloatCap, sizeof(int));
    }

    MdlNode(char*);
    void DestructorDispatch();
    void ReorderChildren();
    void ParseFieldDispatch(char* pszLine);
    void PostProcessDispatch();
    Part* CreateInstanceDispatch(Gob* pGob);
    MdlNode* ConJoinDispatch();
    Part* InternalCreateInstance(Gob* pGob);
    void AddToFreeList(MdlNode* pNode);
    void AddToFreeTokenList(unsigned int nToken);
    void InternalDestructor();
    MdlNode* InternalConJoin();
    void InternalParseField(char* pszLine);
    void InternalPostProcess();
    void InternalControl(Part* pPart, float fA, float fB, float fC);
    void ControlDispatch(Part* pPart, float fA, float fB, float fC);
    void ProcessSkinSeams();
    int CreateSkinNodeList(List<MdlNodeTriMesh*>& result);
    MdlNodeTriMesh* AsMdlNodeTriMesh();
    MdlNodeReference* AsMdlNodeReference();
    MdlNodeLight* AsMdlNodeLight();
    MdlNodeAABB* AsMdlNodeAABB();
    MdlNodeDanglyMesh* AsMdlNodeDanglyMesh();
    MdlNodeEmitter* AsMdlNodeEmitter();
    MdlNodeCamera* AsMdlNodeCamera();
    MdlNodeTrigger* AsMdlNodeTrigger();
    MdlNodeSkin* AsMdlNodeSkin();
    MdlNodeAnimMesh* AsMdlNodeAnimMesh();
    MdlNodeLightsaber* AsMdlNodeLightsaber();
    bool IsMdlNodeAABB();
    bool IsMdlNodeAnimMesh();
    bool IsMdlNodeCamera();
    bool IsMdlNodeDanglyMesh();
    bool IsMdlNodeEmitter();
    bool IsMdlNodeLight();
    bool IsMdlNodeReference();
    bool IsMdlNodeSkin();
    bool IsMdlNodeTrigger();
    bool IsMdlNodeTriMesh();
    char* TranslateNodeName();
    unsigned short m_nFlags;
    short m_nNodeID;
    int  m_nNameIndex;
    MaxTree* m_pTree;
    unsigned long long m_pParent;
    Vector     m_vPosition;
    Quaternion m_qOrientation;
    List<MdlNode*>     m_listChildren;
    List<NewController> m_listControllers;
    List<float>        m_listFloats;
};

class Model;
class Animation;
#include "port/libcbase.h"
void MaxTreeDestructor(void* pTree);
void MaxTreeParseField(void* pTree, char* pszLine);

#if defined(MAXTREE_OUT_OF_LINE) || defined(MAXTREE_KEEP_DTOR_OUT_OF_LINE)
#define MAXTREE_KEEP_OUT_OF_LINE __attribute__((noinline))
#elif defined(MAXTREE_EMIT_OUT_OF_LINE)
#define MAXTREE_KEEP_OUT_OF_LINE __attribute__((used))
#else
#define MAXTREE_KEEP_OUT_OF_LINE
#endif

struct MaxTree32;

// A parsed model tree: a node hierarchy with its name table and shared data arrays.
class MaxTree {
public:

    // m_cType: the tree's kind in the low seven bits, and the bit a tree loaded from a compiled
    // model resource carries (which makes the char negative).
    enum {
        kTreeBase = 1,
        kTreeModel = 2,
        kTreeAnimation = 5,
        kTreeTypeMask = 0x7f,
        kTreeCompiled = 0x80,
    };

    static const char MAXTREE_BASE_ID;
    static const char MAXTREE_MODEL_ID;
    static const char MAXTREE_ANIMATION_ID;

    MAXTREE_KEEP_OUT_OF_LINE MaxTree(char* pszName) {
        __strcpy_chk(m_szName, pszName, 32);
        m_pMdlNode = 0;
        m_cType = kTreeBase;
        m_pMdlResource = 0;
        m_nNodeCount = 0;
        m_freeTokens.SetSize(0);
        m_freeList.SetSize(0);
        m_pfnDestructor = MaxTreeDestructor;
        m_pfnParseField = MaxTreeParseField;
    }

    MaxTree() {}
    MaxTree(const MaxTree32& p);

    MAXTREE_KEEP_OUT_OF_LINE ~MaxTree() {

#if !KOTOR1  // K1 never frees the root node here
        if (m_cType && m_pMdlNode)
            delete m_pMdlNode;
#endif
    }

    void InternalDestructor();
    void InternalParseField(char* pszLine);
    void SynchronizeTree(MaxTree* pOther);
    void AddToFreeList(MdlNode* pNode);
    void AddToFreeTokenList(unsigned int nToken);
    void ResetFreeLists();

#if KOTOR1  // K1 names this teardown operator delete
    static void operator delete(void* pTree);
    void foobar(void* pTree) { MaxTree::operator delete(pTree); }
#else
    void foobar(void* pTree);
#endif

    void FreeTheFreeList();
    Model* AsModel();
    Animation* AsAnimation();
    int GetType();
    void (*m_pfnDestructor)(void*);
    void (*m_pfnParseField)(void*, char*);
    char m_szName[32];
    MdlNode* m_pMdlNode;
    int  m_nNodeCount;
    List<MdlNode*> m_freeList;
    List<unsigned int> m_freeTokens;
    void* m_pMdlResource;
    char m_cType;
    char _pad69[0x6c - 0x69];
};

class MaxFace;
class MdlNodeTriMesh_PostProcess;
struct MdlNodeTriMesh32;
class MdlNodeTriMesh;
class AABB;
class PartTriMesh;

struct Vector2 {
    float u, v;
};

struct Vector3 {
    float x, y, z;
};

// Node holding a vertex buffer and an index buffer.
class MdlNodeTriMesh : public MdlNode {
public:

    ~MdlNodeTriMesh();
    void InternalDestructor();
    void InternalRemoveTemporaryArrays();
    void InternalGenVertices();
    void ProcessVerts();
    void InternalParseField(char* pszLine);
    PartTriMesh* InternalCreateInstance(Gob* pGob);
    void InternalPostProcess();
    void ComputeLocalSurfaceArea(List<MaxFace>& faces, float& fArea);
    void InternalControl(Part* pPart, float fA, float fB, float fC);
    bool HasTangentBases();
    void BuildVertexIndices();
    void BuildNeighborLists();
    void CalculateTangentSpaceBasis();
    MdlNodeTriMesh* InternalConJoin();
    MdlNodeTriMesh(char* pszName);
    MdlNodeTriMesh(MdlNodeTriMesh32& p32);
    void (*m_pfnGenVertices)(void*);
    void (*m_pfnRemoveTemporaryArrays)(void*);
    List<MaxFace> m_faces;

    MdlNodeTriMesh_PostProcess* PostProcess() { return static_cast<MdlNodeTriMesh_PostProcess*>(m_pPostProcess); }
    MaxFace* Faces() { return m_faces.m_pData; }
    short* Indices() { return reinterpret_cast<short*>(m_faces.m_pData); }
    int& FrameCount() { return m_faces.m_count; }

    Vector m_vMin;
    Vector m_vMax;
    float m_fRadius;
    Vector m_vCentroid;
    Vector m_vDiffuse;
    Vector m_vAmbient;
    int m_nTransparencyHint;
    char m_szTexture[0xec - 0xcc];
    char m_szLightmap[0x10c - 0xec];
    List<unsigned int*> m_lVertexIndices;
    List<unsigned int> m_lLeftOverFaces;
    List<int> m_lVertexIndicesCount;
    List<unsigned int> m_lIndexPoolOffsets;
    List<unsigned int> m_lIndexPools;
    int m_nLeftOverPoolOffset;
    unsigned int m_nLeftOverPool;
    int m_nLeftOverIndexCount;
    int m_nPrimitiveType;
    void* m_pPostProcess;
    int  m_nAnimateUV;
    float m_fUVDirectionX;
    float m_fUVDirectionY;
    float m_fUVJitter;
    float m_fUVJitterSpeed;
    int m_nVertexStride;
    unsigned int m_uVertexFlags;
    int m_nVertexOfs;
    int m_nNormalOfs;
    int m_nColorOfs;
    int  m_nTex0Ofs;
    int  m_nTex1Ofs;
    int  m_nTex2Ofs;
    int  m_nTex3Ofs;
    int  m_nTangentOfs;
    int  m_nReserved1;
    int  m_nReserved2;
    int  m_nReserved3;
    unsigned short m_nVertexCount;
    unsigned short m_nTexCoordSets;
    bool m_bHasLightmap;
    bool m_bRotateTexture;
    bool m_bIsBackgroundGeometry;
    bool m_bShadow;
    bool m_bBeaming;
    bool m_bRender;

#if !KOTOR1  // the dirt and hologram block is K2-only
    bool m_bDirtEnabled;
    char _pad1cb[0x1cc - 0x1cb];
    unsigned short m_nDirtTexture;
    unsigned short m_nDirtWorldspace;
    bool m_bHologramDoNotDraw;
    char _pad1d1[0x1d4 - 0x1d1];
#endif
    int m_fSurfaceArea;
    int m_nReserved4;
    int m_nGeomBase;
    float* m_paVertices;
};

class AABB;
class MaxFace;
struct RenderVertexC;
struct RenderVertexV;
class PartAABB;
struct MdlNodeAABB32;

// Walkmesh node: the axis-aligned bounding-box tree used for collision.
class MdlNodeAABB : public MdlNodeTriMesh {
public:

    MdlNodeAABB(MdlNodeAABB32& p32);
    MdlNodeAABB(char* pszName);
    AABB* m_pAABB;
    ~MdlNodeAABB();
    void InternalDestructor();
    void InternalParseField(char* pszLine);
    PartAABB* InternalCreateInstance(Gob* pGob);

    List<MaxFace>& Faces() { return m_faces; }
    RenderVertexV* Vertices() { return reinterpret_cast<RenderVertexV*>(m_paVertices); }
};

class PartAnimMesh;

// Tri-mesh node with per-vertex animation tracks.
class MdlNodeAnimMesh : public MdlNodeTriMesh {
public:

    ~MdlNodeAnimMesh();
    void InternalDestructor();
    void InternalPostProcess();
    void InternalParseField(char* pszLine);
    void InternalControl(Part* pPart, float fA, float fB, float fC);
    void InternalGenVertices();
    void InternalRemoveTemporaryArrays();
    PartAnimMesh* InternalCreateInstance(Gob* pGob);
    MdlNodeAnimMesh* InternalConJoin();
    MdlNodeAnimMesh(char* pszName);

    struct Face {
        char _pad1a[0x1a];
        short m_Index0;
        short m_Index1;
        short m_Index2;
    };

    Face* Faces() { return reinterpret_cast<Face*>(m_faces.m_pData); }

    float m_fSamplePeriod;
    List<Vector> m_lAnimVerts;
    List<Vector> m_lAnimTVerts;
    List<Vector> m_lAnimNormals;
    Vector3* m_paVertexFrames;
    Vector2* m_paTexCoordFrames;
    int m_nVertexFrameCount;
    int m_nTexCoordFrameCount;
};

class PartCamera;

// Node describing a camera.
class MdlNodeCamera : public MdlNode {
public:
    void InternalParseField(char* pszLine);
    PartCamera* InternalCreateInstance(Gob* pGob);
    void InternalControl(Part* pPart, float fA, float fB, float fC);
    MdlNodeCamera* InternalConJoin();
    MdlNodeCamera(char* pszName);
};

class PartDanglyMesh;
struct MdlNodeDanglyMesh32;

// Node with the dangly-mesh constraint parameters.
class MdlNodeDanglyMesh : public MdlNodeTriMesh {
public:

    MdlNodeDanglyMesh(MdlNodeDanglyMesh32& p32);
    MdlNodeDanglyMesh(char* pszName);
    List<float> m_lConstraints;
    float m_fDisplacement;
    float m_fTightness;
    float m_fPeriod;
    void* m_pVertexSource;
    ~MdlNodeDanglyMesh();
    void InternalDestructor();
    void InternalParseField(char* pszLine);
    void InternalPostProcess();
    PartDanglyMesh* InternalCreateInstance(Gob* pGob);
    void InternalRemoveTemporaryArrays();
    void InternalGenVertices();
};

class CAurTexture;
class PartLight;
struct Vector;
struct MdlNodeLight32;

// Node describing a light.
class MdlNodeLight : public MdlNode {
public:

    MdlNodeLight(MdlNodeLight32& p32);
    ~MdlNodeLight();
    MdlNodeLight* InternalConJoin();
    MdlNodeLight(char* pszName);
    void InternalParseField(char* pszLine);
    PartLight* InternalCreateInstance(Gob* pGob);
    void InternalPostProcess();
    void InternalControl(Part* pPart, float fA, float fB, float fC);
    void BinaryInternalDestructor();
    void InternalDestructor();
    float m_fFlareRange;
    List<SafePointer<CAurTexture>> m_lFlareTextures;
    List<float> m_lFlareSizes;
    List<float> m_lFlarePositions;
    List<Vector> m_lFlareColourShifts;
    List<char*> m_lFlareNames;

    CAurTexture** FlareTextures() { return reinterpret_cast<CAurTexture**>(m_lFlareTextures.m_pData); }
    int& FlareCount() { return m_lFlareTextures.m_count; }
    float* FlareSizes() { return m_lFlareSizes.m_pData; }
    float* FlarePositions() { return m_lFlarePositions.m_pData; }
    Vector* FlareColourShifts() { return m_lFlareColourShifts.m_pData; }
    int m_nPriority;
    int m_bAmbientOnly;
    int m_bIsDynamic;
    int m_bAffectDynamic;
    int m_bShadow;
    int m_bGenerateFlare;
    int m_nFadingEnabled;
};

#ifndef MDLNODELIGHTSABER_H
#define MDLNODELIGHTSABER_H

class PartLightsaber;
struct MdlNodeLightsaber32;

// Node describing a lightsaber blade.
class MdlNodeLightsaber : public MdlNodeTriMesh {
public:

    MdlNodeLightsaber(MdlNodeLightsaber32& p32);
    MdlNodeLightsaber(char* pszName);
    Vector3* m_paPositions;
    Vector2* m_aTexCoords;
    Vector3* m_aNormals;
    unsigned int m_anBladePools[2];
    ~MdlNodeLightsaber();
    void InternalDestructor();
    void InternalParseField(char* pszLine);
    void InternalPostProcess();
    PartLightsaber* InternalCreateInstance(Gob* pGob);
    static const short NUM_FRONT_SEGMENTS;
    static const short NUM_BACK_SEGMENTS;
    static const short NUM_PIECE_VERTS;
    static const unsigned short NUM_SEGMENT_VERTS;
};

#if KOTOR1  // K1 node is 8 bytes smaller
K2_LAYOUT_ASSERT(sizeof(MdlNodeLightsaber) == 0x200, "K1 MdlNodeLightsaber size");
#else
K2_LAYOUT_ASSERT(sizeof(MdlNodeLightsaber) == 0x208,
              "pinned by the factory's operator new(0x208) @0x7698a4");
#endif

#endif

class PartReference;
struct MdlNodeReference32;

// Node pointing at another model file.
class MdlNodeReference : public MdlNode {
public:

#if KOTOR1  // K1 inlines the converter
    MdlNodeReference(MdlNodeReference32& p32);
#else
    __attribute__((noinline)) MdlNodeReference(MdlNodeReference32& p32);
#endif

    void InternalControl(Part* pPart, float fA, float fB, float fC);
    void InternalParseField(char* pszLine);
    PartReference* InternalCreateInstance(Gob* pGob);
    MdlNodeReference* InternalConJoin();
    MdlNodeReference(char* pszName);
    char m_szRefModel[0x88 - 0x68];
    int  m_bReattachable;
};

#ifndef MDLNODESKIN_H
#define MDLNODESKIN_H

#include "skinvertexweight.h"

class AABB;
class Part;
struct MdlNodeSkin32;

// Node with the bone weights and bind-pose data for a skinned mesh.
class MdlNodeSkin : public MdlNodeTriMesh {
public:

#if KOTOR1  // K1 inlines the converter
    MdlNodeSkin(MdlNodeSkin32& p32);
#else
    __attribute__((noinline)) MdlNodeSkin(MdlNodeSkin32& p32);
#endif

    ~MdlNodeSkin();
    void InternalDestructor();
    void InternalRemoveTemporaryArrays();
    void InternalParseField(char* pszLine);
    void InternalPostProcess();
    Part* InternalCreateInstance(Gob* pGob);
    void InternalGenVertices();
    MdlNodeSkin* InternalConJoin();
    MdlNodeSkin(char* pszName);
    List<SkinVertexWeight> m_lWeights;
    unsigned int m_nBoneWeightsOfs;
    unsigned int m_nBoneIndicesOfs;
    float* m_paBoneMap;
    int m_nBoneCount;
    List<Quaternion> m_lQBoneRefInv;
    List<Vector> m_lTBoneRefInv;
    List<int> m_lBoneConstantIndices;
    short m_aBoneNodeIndices[17];
};

#endif

// MeshSmooth post-process parameters baked next to a tri-mesh node.
class MdlNodeTriMesh_PostProcess {
public:

    MdlNodeTriMesh_PostProcess();
    ~MdlNodeTriMesh_PostProcess() noexcept(false);
    List<Vector> m_verts;
    List<Vector> m_tverts;
    List<Vector> m_tverts1;
    List<Vector> m_tverts2;
    List<Vector> m_tverts3;
    List<Vector> m_colors;
    List<MaxFace_PostProcess> m_faces;
    List<Vector> m_normals;
    List<unsigned int*> m_vertices;
};

class Gob;
class Animation;
struct Model32;

// Root of a model file: the shared data arrays plus the top-level node.
class Model : public MaxTree {
public:
    // m_nFlags: the classification keywords of the model's ASCII header, and the room bit
    // Scene::SpawnRoom sets.
    enum {
        kModelEffect = 0x1,
        kModelForceDistortion = 0x2,
        kModelCharacter = 0x4,
        kModelDoor = 0x8,
        kModelLightsaber = 0x10,
        kModelItem = 0x20,
        kModelAmbient = 0x40,
        kModelHeatDistortion = 0x80,
        kModelRoom = 0x100,
        kModelSensitive = 0x200,
        kModelPlaceable = 0x400,    // set with kModelItem
    };

    Model(char* pszName);
    Model(Model32& p32);
    ~Model();
    int AddRef(Gob* pGob);
    int DelRef(Gob* pGob);
    int AddRef(Model* pModel);
    int DelRef(Model* pModel);
    void InternalDestructor();
    void InternalParseField(char* pszLine);
    unsigned short m_nFlags;
    char m_bReserved1;
    char m_cAffectedByFog;
    int  m_nRefCount;
    List<Animation*> m_animations;
    Model* m_pSuperModel;
    Vector m_vBoundsMin;
    Vector m_vBoundsMax;
    float m_fRadius;
    float m_fAnimScale;
    char m_szSuperModelName[0xd0 - 0xb0];
    MdlNode* m_pAnimationRoot;
    unsigned int m_nPool;
    int  m_nPoolSize;
    void* m_pMdxResource;
    List<char*> m_listStrings;
};

inline bool Gob::IsForcePushDistortion()
{
    return m_pModel != 0 && (m_bDistortion || (m_pModel->m_nFlags & Model::kModelForceDistortion) != 0);
}

class Gob;
class Part;
class PartLight;
struct Quaternion;

// One walkmesh tile: its corners, neighbours and the parts drawn on it.
class TileData {
public:

    static const int FADE;
    static const int DO_NOT_FADE;
    static const int FADE_NOT_CHECKED;
    TileData();
    ~TileData();
    PartLight* m_paLights[2];
    Part* m_paFlameParts[2];
    Gob*  m_paFlameGobs[2];
    List<Part*> m_aParts;
    Gob*  m_pGob;
    Vector m_vPosition;
    int    m_nMode;
    Vector m_vBBoxMin;
    Vector m_vBBoxMax;
    float  m_fOpacity;
    float  m_fBaseOpacity;
    int   m_nReserved1;
    List<TileData*>    m_aNeighbors;
    List<void*>        m_aReserved1;
    List<PartTriMesh*> m_aNeighborMeshes;
    List<PartTriMesh*> m_aOwnMeshes;
    int   m_nFadeState;
    char  m_bFogged;
    float m_fFade;
};

struct Animation32;

struct AnimationEvent {
    float m_fTime = 0.0f;
    char  m_szName[32];
};

// Root of an animation file: the node hierarchy that overrides a model's.
class Animation : public MaxTree {
public:

    Animation(char* pszName);
    __attribute__((noinline)) Animation(Animation32& p32);
    void PostProcess();
    void Prune(MdlNode* pNode);
    void InternalParseField(char* pszLine);
    ~Animation();
    float m_fLength;
    float m_fTransTime;
    char m_szAnimRoot[32];
    List<AnimationEvent> m_events;
    Model*   m_pModel;
};

// The node's name, from the string table of the model that owns its tree (an animation's tree
// borrows its model's).
inline char* MdlNode::TranslateNodeName()
{
    return (m_pTree->AsModel() ? m_pTree->AsModel() : m_pTree->AsAnimation()->m_pModel)->m_listStrings[m_nNameIndex];
}
