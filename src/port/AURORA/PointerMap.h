#ifndef KOTOR_POINTERMAP_H
#define KOTOR_POINTERMAP_H
#include "k2stdint.h"

#include <map>
#include <list>
#include <unordered_map>

#include "Vector.h"
#include "AURORA/aurmodel.h"

// K2_OFFSETOF (k2stdint.h): offsetof over the part and node classes, which are not standard-layout.

class MaxTree;
class MdlNode;
class MdlNodeReference;
class MdlNodeLight;
class MdlNodeTriMesh;
class MdlNodeSkin;
class AABB;
struct MdlNode32;

struct AABB32 {
    Vector min;
    Vector max;
    int    pChild0;
    int    pChild1;
    int    m_nIndex;
    int    m_nDir;
};
static_assert(sizeof(AABB32) == 0x28, "sizeof pinned by CreateAABB's field copy (ends at +0x28)");

struct MdlNodeReference32 : MdlNode32 {
    char m_szRefModel[32];
    int  m_bReattachable;
};
static_assert(sizeof(MdlNodeReference32) == 0x74, "pinned by the converter's last read (+0x70)");

struct MdlNodeLight32 : MdlNode32 {
    float m_fFlareRange;
    int   m_pFlareTextures;
    int   m_nFlareTextureCount;
    int   m_nFlareTextureCap;
    int   m_pFlareSizes;
    int   m_nFlareSizeCount;
    int   m_nFlareSizeCap;
    int   m_pFlarePositions;
    int   m_nFlarePositionCount;
    int   m_nFlarePositionCap;
    int   m_pFlareColourShifts;
    int   m_nColourShiftCount;
    int   m_nColourShiftCap;
    int   m_pFlareNames;
    int   m_nFlareNameCount;
    int   m_nFlareNameCap;
    int   m_nPriority;
    int   m_bAmbientOnly;
    int   m_bIsDynamic;
    int   m_bAffectDynamic;
    int   m_bShadow;
    int   m_bGenerateFlare;
    int   m_nFadingEnabled;
};
static_assert(sizeof(MdlNodeLight32) == 0xac, "pinned by the converter's last read (+0xa8)");

struct MdlNodeTriMesh32 : MdlNode32 {
    int  m_pfnGenVertices;
    int  m_pfnRemoveTemporaryArrays;
    int  m_pFaces;
    int  m_nFaceCount;
    int  m_nFaceCap;
    Vector m_vMin;
    Vector m_vMax;
    float m_fRadius;
    Vector m_vCentroid;
    Vector m_vDiffuse;
    Vector m_vAmbient;
    int  m_nTransparencyHint;
    char m_szTexture[32];
    char m_szLightmap[32];
    int  m_pVertexIndices;
    int  m_nVertexIndexCount;
    int  m_nVertexIndexCap;
    int  m_pLeftOverFaces;
    int  m_nLeftOverFaceCount;
    int  m_nLeftOverFaceCap;
    int  m_pBoneVertexCounts;
    int  m_nBoneVertexCount;
    int  m_nBoneVertexCap;
    int  m_pIndexPoolOffsets;
    int  m_nIndexPoolOffsetCount;
    int  m_nIndexPoolOffsetCap;
    int  m_pBonePools;
    int  m_nBonePoolCount;
    int  m_nBonePoolCap;
    int  m_nLeftOverPoolOffset;
    int  m_nLeftOverPool;
    int  m_nLeftOverIndexCount;
    int  m_nPrimitiveType;
    int  m_pPostProcess;
    int  m_nAnimateUV;
    float m_fUVDirectionX;
    float m_fUVDirectionY;
    float m_fUVJitter;
    float m_fUVJitterSpeed;
    int  m_nVertexStride;
    unsigned int m_uVertexFlags;
    int  m_nVertexOfs;
    int  m_nNormalOfs;
    int  m_nColorOfs;
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
#if !KOTOR1  // K1 has no dirt or hologram block
    bool m_bDirtEnabled;
    char _pad18b[1];
    unsigned short m_nDirtTexture;
    unsigned short m_nDirtWorldspace;
    bool m_bHologramDoNotDraw;
    char _pad191[3];
#endif
    int  m_fSurfaceArea;
    int  m_nReserved4;
    int  m_nGeomBase;
    int  m_paVertices;
};
#if KOTOR1
static_assert(K2_OFFSETOF(MdlNodeTriMesh32, m_nGeomBase) == 0x194, "K1 C2 @0x207e04");
static_assert(sizeof(MdlNodeTriMesh32) == 0x19c, "K1: the converter's last read is +0x198");
#else
static_assert(sizeof(MdlNodeTriMesh32) == 0x1a4, "pinned by the converter's last read (+0x1a0)");
#endif

struct MdlNodeSkin32 : MdlNodeTriMesh32 {
    int m_pWeights;
    int m_nSkinCount;
    int m_nSkinCap;
    int m_nBoneWeightsOfs;
    int m_nBoneIndicesOfs;
    int m_paBoneMap;
    int m_nBoneCount;
    int m_pQBoneRefInv;
    int m_nBoneQuatCount;
    int m_nBoneQuatCap;
    int m_pTBoneRefInv;
    int m_nBonePosCount;
    int m_nBonePosCap;
    int m_pBoneConstantIndices;
    int m_nBoneSegCount;
    int m_nBoneSegCap;
    short m_aBoneNodeIndices[17];
};
#if KOTOR1
static_assert(sizeof(MdlNodeSkin32) == 0x200, "K1: MdlNodeTriMesh32 is 8 B shorter");
#else
static_assert(sizeof(MdlNodeSkin32) == 0x208, "0x1e4 + 0x22 rounds to 0x208 at align 4 (the row's last read is +0x204)");
#endif


struct MdlNodeDanglyMesh32 : MdlNodeTriMesh32 {
    int   m_pConstraints;
    int   m_nConstraintCount;
    int   m_nConstraintCap;
    float m_fDisplacement;
    float m_fTightness;
    float m_fPeriod;
    int   m_pVertexSource;
};
#if KOTOR1
static_assert(sizeof(MdlNodeDanglyMesh32) == 0x1b8, "K1: MdlNodeTriMesh32 is 8 B shorter");
#else
static_assert(sizeof(MdlNodeDanglyMesh32) == 0x1c0, "0x1a4 + 7*4 (last read +0x1bc)");
#endif

struct MdlNodeAABB32 : MdlNodeTriMesh32 {
    int m_pAABB;
};
#if KOTOR1
static_assert(sizeof(MdlNodeAABB32) == 0x1a0, "K1: MdlNodeTriMesh32 is 8 B shorter");
#else
static_assert(sizeof(MdlNodeAABB32) == 0x1a8, "0x1a4 + 4 (last read +0x1a4)");
#endif

struct MdlNodeLightsaber32 : MdlNodeTriMesh32 {
    int m_paPositions;
    int m_aTexCoords;
    int m_aNormals;
    unsigned int m_anBladePools[2];
};
#if KOTOR1
static_assert(sizeof(MdlNodeLightsaber32) == 0x1b0, "K1: MdlNodeTriMesh32 is 8 B shorter");
#else
static_assert(sizeof(MdlNodeLightsaber32) == 0x1b8, "0x1a4 + 0x10 + 4 (last read +0x1b0)");
#endif

struct MdlNodeEmitter32 : MdlNode32 {
    unsigned char m_abEmitterParams[224];
};
static_assert(sizeof(MdlNodeEmitter32) == 0x130, "0x50 + 0xe0 (the arm's last read is +0x12f)");

struct MaxTree32 {
    int  m_pfnDestructor;
    int  m_pfnParseField;
    char m_szName[32];
    int  m_pMdlNode;
    int  m_nNodeCount;
    int  m_pFreeList;
    int  m_nFreeListCount;
    int  m_nFreeListCap;
    int  m_pFreeTokens;
    int  m_nFreeTokenCount;
    int  m_nFreeTokenCap;
    int  m_pMdlResource;
    char m_cType;
};
static_assert(sizeof(MaxTree32) == 0x50, "pinned by Model32/Animation32 starting at +0x50");

struct Model32 : MaxTree32 {
    unsigned short m_nFlags;
    char m_bReserved1;
    char m_cAffectedByFog;
    int  m_nRefCount;
    int  m_pAnimations;
    int  m_nAnimationCount;
    int  m_nAnimationCap;
    char _pad64[4];
    Vector m_vBoundsMin;
    Vector m_vBoundsMax;
    float m_fRadius;
    float m_fAnimScale;
    char m_szSuperModelName[32];
    int  m_pAnimationRoot;
    int  m_nPool;
    int  m_nPoolSize;
    char _padb4[4];
    int  m_pStrings;
    int  m_nStringCount;
    int  m_nStringCap;
};
static_assert(sizeof(Model32) == 0xc4, "pinned by the Model converter's last read (+0xc0)");

struct Animation32 : MaxTree32 {
    float m_fLength;
    float m_fTransTime;
    char m_szAnimRoot[32];
    int  m_pEvents;
    int  m_nEventCount;
    int  m_nEventCap;
    int  m_pModel;
};
static_assert(sizeof(Animation32) == 0x88, "pinned by the Animation converter's last read (+0x84)");

#include "AABB.h"

#if KOTOR1  // K1 has no pool stride: new[n] bytes, end = base + n
struct PointerMapNode64 { unsigned char m_abStorage[1]; };
#else
struct PointerMapNode64 { unsigned char m_abStorage[32]; };
#endif

class MemoryBlock64 {
public:
    PointerMapNode64* m_pBase;
    PointerMapNode64* m_pCurrent;
    PointerMapNode64* m_pEnd;

    std::unordered_map<void*, MaxTree*> m_maxTrees;
    std::unordered_map<void*, MdlNode*> m_mdlNodes;
    std::unordered_map<void*, AABB*>    m_aabbs;

    MemoryBlock64(int nNumBlocks)
    {
        m_pBase = new PointerMapNode64[nNumBlocks];
        m_pCurrent = m_pBase;
#if KOTOR1
        m_pEnd = m_pBase + nNumBlocks;
#else
        m_pEnd = m_pBase + static_cast<int64_t>(nNumBlocks) * 32;
#endif
    }

};
K2_LAYOUT_ASSERT(sizeof(MemoryBlock64) == 0x90, "sizeof pinned by AllocateMemoryBlock's new(0x90)");

class PointerMap {
public:
    typedef std::map<const int, MemoryBlock64*> BlockMap;

    struct ActiveBlock {
        int m_nId;
        BlockMap::iterator m_it;
    };

    int  AllocateMemoryBlock(int nNumBlocks);
    void DestroyMemoryBlock(const void* p);
    void PushActiveBlock(int nId);
    void PopActiveBlock();
    void* TakeBlockMemory(int nNumNodes);
    AABB* CreateAABB(AABB32* p);

    MaxTree* CreateMaxTreeNode(MaxTree32* p);
    MdlNode* CreateMdlNode(MdlNode32* p);

    static PointerMap s_pointerMap;

private:
    BlockMap m_blocks;
    std::list<ActiveBlock> m_activeBlocks;
};
K2_LAYOUT_ASSERT(sizeof(PointerMap) == 0x30, "sizeof pinned by s_pointerMap's .bss size");
K2_LAYOUT_ASSERT(sizeof(PointerMap::ActiveBlock) == 0x10, "ActiveBlock is {int, node-pointer}");

#endif  // KOTOR_POINTERMAP_H
