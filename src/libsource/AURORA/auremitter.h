#pragma once

#ifndef PARTICLE_H
#define PARTICLE_H

#include "Vector.h"
#include "Quaternion.h"

class PartEmitter;
class ChunkyParticle;

class Particle {
public:
    Particle(PartEmitter* pEmitter, float fDeltaT);

    virtual ~Particle();
    virtual ChunkyParticle* AsChunkyParticle();
    virtual void initialize(float, float);

    void randomPosition(int nA, int nB);
    void inheritVelocity(float fDeltaT);
    void randomDirection(int nA, int nB);

    PartEmitter* m_pEmitter;
    Vector m_vPosition;
    float  m_fRotation;
    float  m_fSpeed;
    Vector m_vVelocity;
    char   _pad30[0x34 - 0x30];
    Quaternion m_qOrientation;
    int    m_nFrame;
    float  m_fLastFrameAge;
    float  m_fFrameTime;
    Vector m_vDirection;
    Vector m_vSpreadVelocity;
    Vector m_vTrailPosition;
    bool   m_bLanded;
    char   _pad75[0x78 - 0x75];
    float  m_fAge;
};

#endif

#ifndef CHUNKYPARTICLE_H
#define CHUNKYPARTICLE_H


class PartEmitter;
class CAurObject;
class Gob;

class ChunkyParticle : public Particle {
public:
    ChunkyParticle(PartEmitter* pEmitter, float fDeltaT);

    virtual ~ChunkyParticle();
    virtual ChunkyParticle* AsChunkyParticle();
    virtual void initialize(float, float);

    CAurObject* m_pAurObject;
};

#endif

#ifndef PARTEMITTER_H
#define PARTEMITTER_H

#include "AURORA/aurmodel.h"
#include "SafePointer.h"
#include "aurwindmanager.h"

class CAurObject;
class CHitInfo;
class TileData;
class Particle;
class Material;
class LightningEmitter;
class CallBack;

class PartEmitter : public Part {
public:

    PartEmitter(MdlNode* pNode);

    virtual ~PartEmitter();
#if !KOTOR1  // K1 has no printMemory slot
    virtual int printMemory();
#endif
    virtual PartEmitter* AsPartEmitter();
    virtual void Animate(float);
    virtual void Initialize();
    virtual void Update(float) = 0;
    virtual LightningEmitter* AsLightningEmitter();

    void AnimateParticles(float fDeltaT);
    void RenderFunc();
    void RenderStandard();
    void RenderLinked();
    void RenderMotionBlurred();
    void RenderNothing();
    void updateParticleFrame(float fDeltaT);
    void moveParticle(Particle* pParticle, float fDeltaT, int nFlag);
    void RenderBoundingSphere();
    void RecursiveFractal(int nIndex, int nCount, int nDepth, Vector vA, Vector vB, float fScale);
    void CalculateRadius();
    static void StartEmitterLayer();
    static void StopEmitterLayer();

    float GetGobAlpha();
    float GetTileAlpha();

    void MoveFunc(float fDeltaT);
    void moveParticleStandard(float fDeltaT);
    void moveParticleP2PGrav(float fDeltaT);
    void moveParticleP2PBezier(float fDeltaT);

    void chkRandom(Particle* pParticle);
    void chkParticleLife(float fDeltaT);
    int GetNextParticleFrame(int nFrame);
    int GetPreviousParticleFrame(int nFrame);

    int m_nBlendMode;
    float m_fAlphaEnd;
    float m_fAlphaStart;
    float m_fBirthRate;
    float m_fBounceCoeff;
    float m_fCombineTime;
    float m_fDrag;
    float m_fFPS;
    float m_fFrameEnd;
    float m_fFrameStart;
    float m_fGrav;
    float m_fLifeExp;
    float m_fMass;
    float m_fP2PBezier2;
    float m_fP2PBezier3;
    float m_fParticleRot;
    float m_fRandVel;
    float m_fSizeStart;
    float m_fSizeEnd;
    float m_fSizeStartY;
    float m_fSizeEndY;
    float m_fSpread;
    float m_fThreshold;
    float m_fVelocity;
    float m_fXSize;
    float m_fYSize;
    float m_fBlurLength;
    float m_fLightningDelay;
    float m_fLightningRadius;
    float m_fLightningScale;
    float m_fLightningSubDiv;
    float m_fLightningZigzag;
    float m_fBirthTimer;
    float m_fRadius;
    float m_fGravityAccel;
    float m_fAlphaMid;
    float m_fPercentStart;
    float m_fPercentMid;
    float m_fPercentEnd;
    float m_fSizeMid;
    float m_fSizeMidY;
    float m_fRandomBirthRate;
    float m_fDelayTimer;
    float m_fControlPtTimer;
    float m_fTargetSize;
    float m_fNumControlPts;
    float m_fControlPtRadius;
    float m_fControlPtDelay;
    float m_fTangentSpread;
    float m_fTangentLength;
    int   m_nRenderMode;
    char  _pad130[0x134 - 0x130];
    Vector m_avVectors[10];
    Quaternion m_qInheritOrientation;
    Quaternion m_qWorldOrientation;
    Quaternion m_qPrevWorldOrientation;
    char _pad1dc[0x1e0 - 0x1dc];
    SafePointer<WindManager> m_spWindManager;
    List<Particle*> m_lParticles;
    List<Particle*> m_lFreeParticles;
    Material* m_pParticleMaterial;
    SafePointer<CAurObject> m_spTarget;
    CHitInfo* m_pHitInfo;
    TileData* m_pTileData;
    CallBack* m_pTargetCallBack;
    short m_nGradientMode;
    bool m_bDetonate;
    bool m_bStarted;
#if !KOTOR1  // K1 has no weather emitters
    bool m_bWeather;
    // Keeps the derived emitter classes' first members at their expected offsets.
    char _pad235[0x238 - 0x235];
#endif
};

#endif

#ifndef EXPLOSIONEMITTER_H
#define EXPLOSIONEMITTER_H


class ExplosionEmitter : public PartEmitter {
public:

    ExplosionEmitter(MdlNode* pNode);

    virtual void Update(float);
};

#endif

#ifndef FOUNTAINEMITTER_H
#define FOUNTAINEMITTER_H


class FountainEmitter : public PartEmitter {
public:

    FountainEmitter(MdlNode* pNode);

    virtual void Update(float);
};

#endif

#ifndef LIGHTNINGEMITTER_H
#define LIGHTNINGEMITTER_H

#include "List.h"

class LightningEmitter : public PartEmitter {
public:

    LightningEmitter(MdlNode* pNode);

    virtual ~LightningEmitter();
    virtual void Initialize();
    virtual void Update(float);
    virtual LightningEmitter* AsLightningEmitter();
    virtual void UpdateControlPoints();
    virtual void UpdateBranches();

    void InitializeBranch();

    Vector m_vBoltSpan;
    Vector m_vBoltOrigin;
    List<Vector> m_lControlPoints;
    List<Vector> m_lPrevPoints;
    List<Vector> m_lNextPoints;
    List<Vector> m_lControlTangents;
    List<Vector> m_lPrevTangents;
    List<Vector> m_lNextTangents;
    List<LightningEmitter*> m_lBranches;
    List<int> m_lBranchStartIndices;
    List<Vector> m_lParticleOffsets;
    float m_fJitterAxisX;
    float m_fJitterAxisY;
    float m_fJitterAxisZ;
    int   m_nActiveBranches;
    bool  m_bIsBranch;
};

#endif

#ifndef SINGLEEMITTER_H
#define SINGLEEMITTER_H


class SingleEmitter : public PartEmitter {
public:

    SingleEmitter(MdlNode* pNode);

    virtual void Update(float);
};

#endif

class Gob;
struct MdlNodeEmitter32;

class MdlNodeEmitter : public MdlNode {
public:
    MdlNodeEmitter(MdlNodeEmitter32& p32);
    MdlNodeEmitter(char* pszName) throw();

    void InternalParseField(char* pszLine);
    void InternalPostProcess();
    void InternalControl(Part* pPart, float fA, float fB, float fC);
    MdlNodeEmitter* InternalConJoin();
    Part* InternalCreateInstance(Gob* pGob);

    bool IsPointToPoint();        // bit 0
    bool IsPointToPointSel();     // bit 1
    bool IsAffectedByWind();      // bit 2
    bool IsTinted();              // bit 3
    bool IsBouncing();            // bit 4
    bool IsRandom();              // bit 5
    bool IsRandomOrder();         // bit 12
    bool IsInheriting();          // bit 6
    bool IsInheritingVelocity();  // bit 7
    bool IsInheritingLocal();     // bit 8
    bool IsSplat();               // bit 9
    bool IsInheritingPart();      // bit 10
    bool UsesDepthTexture();

    void EnablePointToPoint();        // bit 0
    void EnablePointToPointSel();     // bit 1
    void EnableAffectedByWind();      // bit 2
    void EnableTinted();              // bit 3
    void EnableBouncing();            // bit 4
    void EnableRandom();              // bit 5
    void EnableRandomOrder();         // bit 12
    void EnableInheriting();          // bit 6
    void EnableInheritingVelocity();  // bit 7
    void EnableInheritingLocal();     // bit 8
    void EnableSplat();               // bit 9
    void EnableInheritingPart();      // bit 10
    void EnableDepthTexture();        // bit 11

    float m_fDeadSpace;
    float m_fBlastRadius;
    float m_fBlastLength;
    int   m_nNumBranches;
    int   m_bControlPtSmoothing;
    int   m_nXGrid;
    int   m_nYGrid;
    int   m_nSpawnType;
    char m_szUpdate[32];
    char m_szRender[32];
    char m_szBlend[32];
    char m_szTexture[32];
    union {
        char m_szChunkName[16];
        int  m_anChunkName[4];
    };
    int  m_nTwoSidedTex;
    int  m_nLoop;
    short m_nRenderOrder;
    bool m_bFrameBlending;
    char m_szDepthTextureName[32];
    char _pad143;
    // m_nEmitterFlags' bits, the m_flags fields seen as one word.
    enum {
        kEmitterPointToPoint = 0x1,
        kEmitterPointToPointSel = 0x2,
        kEmitterAffectedByWind = 0x4,
        kEmitterTinted = 0x8,
        kEmitterBouncing = 0x10,
        kEmitterRandom = 0x20,
        kEmitterInheriting = 0x40,
        kEmitterInheritingVelocity = 0x80,
        kEmitterInheritingLocal = 0x100,
        kEmitterSplat = 0x200,
        kEmitterInheritingPart = 0x400,
        kEmitterDepthTexture = 0x800,
        kEmitterRandomOrder = 0x1000,
    };

    // Whole-word view of the flag bits, for the call sites that need one word load.
    union {
        struct {
            unsigned int m_bPointToPoint : 1;
            unsigned int m_bPointToPointSel : 1;
            unsigned int m_bAffectedByWind : 1;
            unsigned int m_bTinted : 1;
            unsigned int m_bBouncing : 1;
            unsigned int m_bRandom : 1;
            unsigned int m_bInheriting : 1;
            unsigned int m_bInheritingVelocity : 1;
            unsigned int m_bInheritingLocal : 1;
            unsigned int m_bSplat : 1;
            unsigned int m_bInheritingPart : 1;
            unsigned int m_bDepthTexture : 1;
            unsigned int m_bRandomOrder : 1;
            unsigned int _unused : 19;
        } m_flags;
        unsigned int m_nEmitterFlags;
    };
};
