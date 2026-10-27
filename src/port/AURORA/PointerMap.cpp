// Aurora's 32-bit model-file format to 64-bit runtime relocation layer. Each Create* walks a
// mirror struct into a runtime object carved from the active memory block and caches the result by
// mirror pointer.

#include "k2stdint.h"
#define AABB_DTOR_HEADER_BODY
#define MAXFACE_DTOR_DECLARED_ONLY
#include "PointerMap.h"
#include "AURORA/aurmodel.h"
#include "aurtexman.h"
#include "auremitter.h"

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/crtapi.h"

static int s_nNextBlockId;
static int s_nMaxTreeCount;
static int s_nMdlNodeCount;
static int s_nAABBCount;

PointerMap PointerMap::s_pointerMap;

// The 32-bit image's base-tree converter, which the Model and Animation converters start from.
inline MaxTree::MaxTree(const MaxTree32& p)
{
    m_pfnDestructor = reinterpret_cast<void (*)(void*)>(static_cast<int64_t>(p.m_pfnDestructor));
    m_pfnParseField = reinterpret_cast<void (*)(void*, char*)>(static_cast<int64_t>(p.m_pfnParseField));
    __strcpy_chk(m_szName, p.m_szName, 32);
    m_pMdlNode = reinterpret_cast<MdlNode*>(static_cast<int64_t>(p.m_pMdlNode));
    m_nNodeCount = p.m_nNodeCount;
    m_freeList.m_pData = reinterpret_cast<MdlNode**>(static_cast<int64_t>(p.m_pFreeList));
    __builtin_memcpy(&m_freeList.m_count, &p.m_nFreeListCount, sizeof(int));
    __builtin_memcpy(&m_freeList.m_capacity, &p.m_nFreeListCap, sizeof(int));
    m_freeTokens.m_pData = reinterpret_cast<unsigned int*>(static_cast<int64_t>(p.m_pFreeTokens));
    __builtin_memcpy(&m_freeTokens.m_count, &p.m_nFreeTokenCount, sizeof(int));
    __builtin_memcpy(&m_freeTokens.m_capacity, &p.m_nFreeTokenCap, sizeof(int));
    m_pMdlResource = reinterpret_cast<void*>(static_cast<int64_t>(p.m_pMdlResource));
    m_cType = p.m_cType;
}


int PointerMap::AllocateMemoryBlock(int nNumBlocks)
{
    int nId = ++s_nNextBlockId;

    MemoryBlock64* pBlock = new MemoryBlock64(nNumBlocks);

    m_blocks[nId] = pBlock;
    return s_nNextBlockId;
}

void PointerMap::DestroyMemoryBlock(const void* p)
{
    for (BlockMap::iterator it = m_blocks.begin(); it != m_blocks.end(); ++it) {
        if (it->second->m_pBase == p) {
            if (it->second->m_pBase != 0)
                operator delete[](const_cast<void*>(p));
            delete it->second;
            m_blocks.erase(it);
            return;
        }
    }
}

void PointerMap::PushActiveBlock(int nId)
{
    // Active blocks are a stack.
    ActiveBlock blk;
    blk.m_nId = nId;
    blk.m_it = m_blocks.find(nId);
    m_activeBlocks.push_front(blk);
}

void PointerMap::PopActiveBlock()
{
    m_activeBlocks.pop_front();
}

#if defined(K2_FIX_ENGINE_BUGS) && !KOTOR1  // K1 counts the carve in bytes already
// A block cursor moved on by nBytes, rounded up to 16 so the next carve (an object, or an array of
// shorts) starts aligned, as operator new's blocks do.
static PointerMapNode64* AdvanceBlockCursor(PointerMapNode64* pCursor, size_t nBytes)
{
    return reinterpret_cast<PointerMapNode64*>(reinterpret_cast<char*>(pCursor) + ((nBytes + 15) & ~static_cast<size_t>(15)));
}
#endif

// Carves nNumNodes pool nodes off the active block and returns where they start (the block's old
// cursor). Every caller passes a byte count (an object's sizeof, or an array's), which K2 counts in
// 32-byte pool nodes (sic): each carve is followed by 31 times its own size of untouched block.
void* PointerMap::TakeBlockMemory(int nNumNodes)
{
    MemoryBlock64* pBlock = m_blocks.find(m_activeBlocks.front().m_nId)->second;
    PointerMapNode64* pMemory = pBlock->m_pCurrent;
#if defined(K2_FIX_ENGINE_BUGS) && !KOTOR1
    pBlock->m_pCurrent = AdvanceBlockCursor(pMemory, static_cast<size_t>(nNumNodes));
#else
    pBlock->m_pCurrent = pMemory + nNumNodes;
#endif
    return pMemory;
}

// Dispatches on the mirror's own type byte; each arm carves a node from the active block and
// converts into it.
MaxTree* PointerMap::CreateMaxTreeNode(MaxTree32* p)
{
    MemoryBlock64* pBlock = m_activeBlocks.front().m_it->second;

    std::unordered_map<void*, MaxTree*>::iterator it = pBlock->m_maxTrees.find(p);
    if (it != pBlock->m_maxTrees.end())
        return it->second;

    MaxTree* pNode = 0;
    if (p->m_cType == MaxTree::MAXTREE_MODEL_ID) {
        MemoryBlock64* pPool = m_blocks.find(m_activeBlocks.front().m_nId)->second;
        Model* pModel = reinterpret_cast<Model*>(pPool->m_pCurrent);
#if defined(K2_FIX_ENGINE_BUGS) && !KOTOR1
        pPool->m_pCurrent = AdvanceBlockCursor(pPool->m_pCurrent, sizeof(Model));
#else
        pPool->m_pCurrent += sizeof(Model);
#endif
        new (pModel) Model(*static_cast<Model32*>(p));
        pNode = static_cast<MaxTree*>(pModel);
    } else if (p->m_cType == MaxTree::MAXTREE_ANIMATION_ID) {
        MemoryBlock64* pPool = m_blocks.find(m_activeBlocks.front().m_nId)->second;
        Animation* pAnim = reinterpret_cast<Animation*>(pPool->m_pCurrent);
#if defined(K2_FIX_ENGINE_BUGS) && !KOTOR1
        pPool->m_pCurrent = AdvanceBlockCursor(pPool->m_pCurrent, sizeof(Animation));
#else
        pPool->m_pCurrent += sizeof(Animation);
#endif
        new (pAnim) Animation(*static_cast<Animation32*>(p));
        pNode = static_cast<MaxTree*>(pAnim);
    } else if (p->m_cType == MaxTree::MAXTREE_BASE_ID) {
        MemoryBlock64* pPool = m_blocks.find(m_activeBlocks.front().m_nId)->second;
        MaxTree* pTree = reinterpret_cast<MaxTree*>(pPool->m_pCurrent);
#if defined(K2_FIX_ENGINE_BUGS) && !KOTOR1
        pPool->m_pCurrent = AdvanceBlockCursor(pPool->m_pCurrent, sizeof(MaxTree));
#else
        pPool->m_pCurrent += sizeof(MaxTree);
#endif
        new (pTree) MaxTree(*p);
        pNode = pTree;
    }

    if (pNode != 0) {
        ++s_nMaxTreeCount;
        pBlock->m_maxTrees[p] = pNode;
    }
    return pNode;
}

// The 32-bit image's Model converter.
Model::Model(Model32& p) : MaxTree(p)
{
    m_nFlags = p.m_nFlags;
    m_bReserved1 = p.m_bReserved1;
    m_cAffectedByFog = p.m_cAffectedByFog;
    m_nRefCount = p.m_nRefCount;
    m_animations.m_pData = reinterpret_cast<Animation**>(static_cast<int64_t>(p.m_pAnimations));
    __builtin_memcpy(&m_animations.m_count, &p.m_nAnimationCount, sizeof(int));
    __builtin_memcpy(&m_animations.m_capacity, &p.m_nAnimationCap, sizeof(int));
    m_pSuperModel = 0;
    m_vBoundsMin = p.m_vBoundsMin;
    m_vBoundsMax = p.m_vBoundsMax;
    m_fRadius = p.m_fRadius;
    m_fAnimScale = p.m_fAnimScale;
    __strcpy_chk(m_szSuperModelName, p.m_szSuperModelName, 32);
    m_pAnimationRoot = reinterpret_cast<MdlNode*>(static_cast<int64_t>(p.m_pAnimationRoot));
    m_nPool = p.m_nPool;
    m_nPoolSize = p.m_nPoolSize;
    m_pMdxResource = 0;
    m_listStrings.m_pData = reinterpret_cast<char**>(static_cast<int64_t>(p.m_pStrings));
    __builtin_memcpy(&m_listStrings.m_count, &p.m_nStringCount, sizeof(int));
    __builtin_memcpy(&m_listStrings.m_capacity, &p.m_nStringCap, sizeof(int));
}

// The 32-bit image's Animation converter. The animation's root name is a fixed 32-byte array in the
// file, so it copies whole; the base's name is a terminated string and goes through the checked
// copy.
Animation::Animation(Animation32& p) : MaxTree(p)
{
    m_fLength = p.m_fLength;
    m_fTransTime = p.m_fTransTime;
    __builtin_memcpy(m_szAnimRoot, p.m_szAnimRoot, sizeof(p.m_szAnimRoot));
    m_events.m_pData = reinterpret_cast<AnimationEvent*>(static_cast<int64_t>(p.m_pEvents));
    __builtin_memcpy(&m_events.m_count, &p.m_nEventCount, sizeof(int));
    __builtin_memcpy(&m_events.m_capacity, &p.m_nEventCap, sizeof(int));
    m_pModel = reinterpret_cast<Model*>(static_cast<int64_t>(p.m_pModel));
}

// The 32-bit image's MdlNodeReference converter. The base conversion is MdlNode's own converter (in
// MdlNode.cpp); only the reference's own fields are copied here.
MdlNodeReference::MdlNodeReference(MdlNodeReference32& p) : MdlNode(p)
{
    __builtin_memcpy(m_szRefModel, p.m_szRefModel, sizeof(p.m_szRefModel));
    m_bReattachable = p.m_bReattachable;
}

// Converts a 32-bit image's MdlNodeLight, copying the base's fields plus the light's five lists.
MdlNodeLight::MdlNodeLight(MdlNodeLight32& p) : MdlNode(p)
{
    m_fFlareRange = p.m_fFlareRange;
    m_lFlareTextures.m_pData = reinterpret_cast<SafePointer<CAurTexture>*>(static_cast<int64_t>(p.m_pFlareTextures));
    __builtin_memcpy(&m_lFlareTextures.m_count, &p.m_nFlareTextureCount, sizeof(int));
    __builtin_memcpy(&m_lFlareTextures.m_capacity, &p.m_nFlareTextureCap, sizeof(int));
    m_lFlareSizes.m_pData = reinterpret_cast<float*>(static_cast<int64_t>(p.m_pFlareSizes));
    __builtin_memcpy(&m_lFlareSizes.m_count, &p.m_nFlareSizeCount, sizeof(int));
    __builtin_memcpy(&m_lFlareSizes.m_capacity, &p.m_nFlareSizeCap, sizeof(int));
    m_lFlarePositions.m_pData = reinterpret_cast<float*>(static_cast<int64_t>(p.m_pFlarePositions));
    __builtin_memcpy(&m_lFlarePositions.m_count, &p.m_nFlarePositionCount, sizeof(int));
    __builtin_memcpy(&m_lFlarePositions.m_capacity, &p.m_nFlarePositionCap, sizeof(int));
    m_lFlareColourShifts.m_pData = reinterpret_cast<Vector*>(static_cast<int64_t>(p.m_pFlareColourShifts));
    __builtin_memcpy(&m_lFlareColourShifts.m_count, &p.m_nColourShiftCount, sizeof(int));
    __builtin_memcpy(&m_lFlareColourShifts.m_capacity, &p.m_nColourShiftCap, sizeof(int));
    m_lFlareNames.m_pData = reinterpret_cast<char**>(static_cast<int64_t>(p.m_pFlareNames));
    __builtin_memcpy(&m_lFlareNames.m_count, &p.m_nFlareNameCount, sizeof(int));
    m_lFlareNames.m_capacity = p.m_nFlareNameCap;
    m_nPriority = p.m_nPriority;
    m_bAmbientOnly = p.m_bAmbientOnly;
    m_bIsDynamic = p.m_bIsDynamic;
    m_bAffectDynamic = p.m_bAffectDynamic;
    m_bShadow = p.m_bShadow;
    m_bGenerateFlare = p.m_bGenerateFlare;
    m_nFadingEnabled = p.m_nFadingEnabled;
}

// The 32-bit image's trimesh converter.
MdlNodeTriMesh::MdlNodeTriMesh(MdlNodeTriMesh32& p) : MdlNode(p)
{
    m_pfnGenVertices = reinterpret_cast<void (*)(void*)>(static_cast<int64_t>(p.m_pfnGenVertices));
    m_pfnRemoveTemporaryArrays = reinterpret_cast<void (*)(void*)>(static_cast<int64_t>(p.m_pfnRemoveTemporaryArrays));
    m_faces.m_pData = reinterpret_cast<MaxFace*>(static_cast<int64_t>(p.m_pFaces));
    __builtin_memcpy(&m_faces.m_count, &p.m_nFaceCount, sizeof(int));
    __builtin_memcpy(&m_faces.m_capacity, &p.m_nFaceCap, sizeof(int));
    m_vMin = p.m_vMin;
    m_vMax = p.m_vMax;
    m_fRadius = p.m_fRadius;
    m_vCentroid = p.m_vCentroid;
    m_vDiffuse = p.m_vDiffuse;
    m_vAmbient = p.m_vAmbient;
    m_nTransparencyHint = p.m_nTransparencyHint;
    __strcpy_chk(m_szTexture, p.m_szTexture, 32);
    __strcpy_chk(m_szLightmap, p.m_szLightmap, 32);
#if !KOTOR1  // K1 has no MAL_Rk01AA texture-name fixup
    if (_stricmp(m_szTexture, "MAL_Rk01AA") == 0)
        strcpy(m_szTexture, "MAL_Rk01");
#endif
    m_lVertexIndices.m_pData = reinterpret_cast<unsigned int**>(static_cast<int64_t>(p.m_pVertexIndices));
    __builtin_memcpy(&m_lVertexIndices.m_count, &p.m_nVertexIndexCount, sizeof(int));
    __builtin_memcpy(&m_lVertexIndices.m_capacity, &p.m_nVertexIndexCap, sizeof(int));
    m_lLeftOverFaces.m_pData = reinterpret_cast<unsigned int*>(static_cast<int64_t>(p.m_pLeftOverFaces));
    __builtin_memcpy(&m_lLeftOverFaces.m_count, &p.m_nLeftOverFaceCount, sizeof(int));
    __builtin_memcpy(&m_lLeftOverFaces.m_capacity, &p.m_nLeftOverFaceCap, sizeof(int));
    m_lVertexIndicesCount.m_pData = reinterpret_cast<int*>(static_cast<int64_t>(p.m_pBoneVertexCounts));
    __builtin_memcpy(&m_lVertexIndicesCount.m_count, &p.m_nBoneVertexCount, sizeof(int));
    __builtin_memcpy(&m_lVertexIndicesCount.m_capacity, &p.m_nBoneVertexCap, sizeof(int));
    m_lIndexPoolOffsets.m_pData = reinterpret_cast<unsigned int*>(static_cast<int64_t>(p.m_pIndexPoolOffsets));
    __builtin_memcpy(&m_lIndexPoolOffsets.m_count, &p.m_nIndexPoolOffsetCount, sizeof(int));
    __builtin_memcpy(&m_lIndexPoolOffsets.m_capacity, &p.m_nIndexPoolOffsetCap, sizeof(int));
    m_lIndexPools.m_pData = reinterpret_cast<unsigned int*>(static_cast<int64_t>(p.m_pBonePools));
    __builtin_memcpy(&m_lIndexPools.m_count, &p.m_nBonePoolCount, sizeof(int));
    m_lIndexPools.m_capacity = p.m_nBonePoolCap;
    m_nLeftOverPoolOffset = p.m_nLeftOverPoolOffset;
    m_nLeftOverPool = static_cast<unsigned int>(p.m_nLeftOverPool);
    m_nLeftOverIndexCount = p.m_nLeftOverIndexCount;
    m_nPrimitiveType = p.m_nPrimitiveType;
    m_pPostProcess = reinterpret_cast<void*>(static_cast<int64_t>(p.m_pPostProcess));
    __builtin_memcpy(&m_nAnimateUV, &p.m_nAnimateUV, sizeof(int));
    m_fUVDirectionX = p.m_fUVDirectionX;
    m_fUVDirectionY = p.m_fUVDirectionY;
    m_fUVJitter = p.m_fUVJitter;
    m_fUVJitterSpeed = p.m_fUVJitterSpeed;
    m_nVertexStride = p.m_nVertexStride;
    m_uVertexFlags = p.m_uVertexFlags;
    m_nVertexOfs = p.m_nVertexOfs;
    m_nNormalOfs = p.m_nNormalOfs;
    m_nColorOfs = p.m_nColorOfs;
    m_nTex0Ofs = p.m_nTex0Ofs;
    m_nTex1Ofs = p.m_nTex1Ofs;
    m_nTex2Ofs = p.m_nTex2Ofs;
    m_nTex3Ofs = p.m_nTex3Ofs;
    m_nTangentOfs = p.m_nTangentOfs;
    m_nReserved1 = p.m_nReserved1;
    m_nReserved2 = p.m_nReserved2;
    m_nReserved3 = p.m_nReserved3;
    m_nVertexCount = p.m_nVertexCount;
    m_nTexCoordSets = p.m_nTexCoordSets;
    m_bHasLightmap = p.m_bHasLightmap;
    m_bRotateTexture = p.m_bRotateTexture;
    m_bIsBackgroundGeometry = p.m_bIsBackgroundGeometry;
    m_bShadow = p.m_bShadow;
    m_bBeaming = p.m_bBeaming;
    m_bRender = p.m_bRender;
#if !KOTOR1  // K1's trimesh has no dirt or hologram fields
    m_bDirtEnabled = p.m_bDirtEnabled;
    m_nDirtTexture = p.m_nDirtTexture;
    m_nDirtWorldspace = p.m_nDirtWorldspace;
    m_bHologramDoNotDraw = p.m_bHologramDoNotDraw;
#endif
    m_fSurfaceArea = p.m_fSurfaceArea;
    m_nReserved4 = p.m_nReserved4;
    m_nGeomBase = p.m_nGeomBase;
    m_paVertices = reinterpret_cast<float*>(static_cast<int64_t>(p.m_paVertices));
}

// Converts a 32-bit image's skin node. The trimesh base conversion is a separate call, so only the
// skin's own fields are copied here.
MdlNodeSkin::MdlNodeSkin(MdlNodeSkin32& p) : MdlNodeTriMesh(p)
{
    m_lWeights.m_pData = reinterpret_cast<SkinVertexWeight*>(static_cast<int64_t>(p.m_pWeights));
    __builtin_memcpy(&m_lWeights.m_count, &p.m_nSkinCount, sizeof(int));
    m_lWeights.m_capacity = p.m_nSkinCap;
    m_nBoneWeightsOfs = p.m_nBoneWeightsOfs;
    m_nBoneIndicesOfs = p.m_nBoneIndicesOfs;
    m_paBoneMap = reinterpret_cast<float*>(static_cast<int64_t>(p.m_paBoneMap));
    m_nBoneCount = p.m_nBoneCount;
    m_lQBoneRefInv.m_pData = reinterpret_cast<Quaternion*>(static_cast<int64_t>(p.m_pQBoneRefInv));
    __builtin_memcpy(&m_lQBoneRefInv.m_count, &p.m_nBoneQuatCount, sizeof(int));
    __builtin_memcpy(&m_lQBoneRefInv.m_capacity, &p.m_nBoneQuatCap, sizeof(int));
    m_lTBoneRefInv.m_pData = reinterpret_cast<Vector*>(static_cast<int64_t>(p.m_pTBoneRefInv));
    __builtin_memcpy(&m_lTBoneRefInv.m_count, &p.m_nBonePosCount, sizeof(int));
    __builtin_memcpy(&m_lTBoneRefInv.m_capacity, &p.m_nBonePosCap, sizeof(int));
    m_lBoneConstantIndices.m_pData = reinterpret_cast<int*>(static_cast<int64_t>(p.m_pBoneConstantIndices));
    __builtin_memcpy(&m_lBoneConstantIndices.m_count, &p.m_nBoneSegCount, sizeof(int));
    __builtin_memcpy(&m_lBoneConstantIndices.m_capacity, &p.m_nBoneSegCap, sizeof(int));
    __builtin_memcpy(&m_aBoneNodeIndices, p.m_aBoneNodeIndices, sizeof(p.m_aBoneNodeIndices));
}

// Converts the 32-bit model format's dangly mesh into this build's. Only CreateMdlNode's 32-bit arm
// uses it.
MdlNodeDanglyMesh::MdlNodeDanglyMesh(MdlNodeDanglyMesh32& p) : MdlNodeTriMesh(p)
{
    m_lConstraints.m_pData = reinterpret_cast<float*>(static_cast<int64_t>(p.m_pConstraints));
    // Copied rather than assigned, so the count is a store of its own.
    __builtin_memcpy(&m_lConstraints.m_count, &p.m_nConstraintCount, sizeof(int));
    m_lConstraints.m_capacity = p.m_nConstraintCap;
    m_fDisplacement = p.m_fDisplacement;
    m_fTightness = p.m_fTightness;
    m_fPeriod = p.m_fPeriod;
    m_pVertexSource = reinterpret_cast<void*>(static_cast<int64_t>(p.m_pVertexSource));
}

// The 32-bit image's bounding-box node converter.
MdlNodeAABB::MdlNodeAABB(MdlNodeAABB32& p) : MdlNodeTriMesh(p)
{
    m_pAABB = reinterpret_cast<AABB*>(static_cast<int64_t>(p.m_pAABB));
}

// The 32-bit image's lightsaber-node converter. The two pool ids move as one 8-byte copy.
MdlNodeLightsaber::MdlNodeLightsaber(MdlNodeLightsaber32& p) : MdlNodeTriMesh(p)
{
    m_paPositions = reinterpret_cast<Vector3*>(static_cast<int64_t>(p.m_paPositions));
    m_aTexCoords = reinterpret_cast<Vector2*>(static_cast<int64_t>(p.m_aTexCoords));
    m_aNormals = reinterpret_cast<Vector3*>(static_cast<int64_t>(p.m_aNormals));
    __builtin_memcpy(m_anBladePools, p.m_anBladePools, sizeof(m_anBladePools));
}

// Converts a 32-bit image's emitter node: the base node's conversion, then the emitter's own field
// window as one contiguous block.
MdlNodeEmitter::MdlNodeEmitter(MdlNodeEmitter32& p) : MdlNode(p)
{
    __builtin_memcpy(&m_fDeadSpace, p.m_abEmitterParams, sizeof(p.m_abEmitterParams));
}

AABB* PointerMap::CreateAABB(AABB32* p)
{
    MemoryBlock64* pBlock = m_activeBlocks.front().m_it->second;

    std::unordered_map<void*, AABB*>::iterator it = pBlock->m_aabbs.find(p);
    if (it != pBlock->m_aabbs.end())
        return it->second;

    MemoryBlock64* pPool = m_blocks.find(m_activeBlocks.front().m_nId)->second;

    AABB* pAABB = reinterpret_cast<AABB*>(pPool->m_pCurrent);
#if defined(K2_FIX_ENGINE_BUGS) && !KOTOR1
    pPool->m_pCurrent = AdvanceBlockCursor(pPool->m_pCurrent, sizeof(AABB));
#else
    pPool->m_pCurrent += sizeof(AABB);
#endif

    // The bounds type is a plain aggregate, so value-initialising it clears all 12 bytes of max.
    __builtin_memset(&pAABB->max, 0, sizeof(Vector));

    pAABB->min = p->min;
    pAABB->max = p->max;
    pAABB->pChild0 = reinterpret_cast<AABB*>(static_cast<int64_t>(p->pChild0));
    pAABB->pChild1 = reinterpret_cast<AABB*>(static_cast<int64_t>(p->pChild1));
    pAABB->m_nIndex = p->m_nIndex;
    pAABB->m_nDir = p->m_nDir;

    if (pAABB != 0) {
        ++s_nAABBCount;
        pBlock->m_aabbs[p] = pAABB;
    }
    return pAABB;
}

// Creates a runtime node from a 32-bit mirror, as the entry point of the 32-bit node hierarchy:
// dispatches on the mirror's kind word and carves the matching runtime node from the active block.
MdlNode* PointerMap::CreateMdlNode(MdlNode32* p)
{
    MemoryBlock64* pBlock = m_activeBlocks.front().m_it->second;

    std::unordered_map<void*, MdlNode*>::iterator it = pBlock->m_mdlNodes.find(p);
    if (it != pBlock->m_mdlNodes.end())
        return it->second;

    MdlNode* pNode = 0;
    if (p->m_nFlags == MdlNode::MDLNODE_BASE_ID) {
        MdlNode* pNew = static_cast<MdlNode*>(TakeBlockMemory(sizeof(MdlNode)));
        new (pNew) MdlNode(*p);
        pNode = pNew;
    } else if (p->m_nFlags == MdlNode::MDLNODE_LIGHT_ID) {
        MdlNodeLight* pNew = static_cast<MdlNodeLight*>(TakeBlockMemory(sizeof(MdlNodeLight)));
        new (pNew) MdlNodeLight(*static_cast<MdlNodeLight32*>(p));
        pNode = static_cast<MdlNode*>(pNew);
    } else if (p->m_nFlags == MdlNode::MDLNODE_EMITTER_ID) {
        MdlNodeEmitter* pNew = static_cast<MdlNodeEmitter*>(TakeBlockMemory(sizeof(MdlNodeEmitter)));
        new (pNew) MdlNodeEmitter(*static_cast<MdlNodeEmitter32*>(p));
        pNode = static_cast<MdlNode*>(pNew);
    } else if (p->m_nFlags == MdlNode::MDLNODE_CAMERA_ID) {
        // no node is built for this kind
    } else if (p->m_nFlags == MdlNode::MDLNODE_REFERENCE_ID) {
        MdlNodeReference* pNew = static_cast<MdlNodeReference*>(TakeBlockMemory(sizeof(MdlNodeReference)));
        new (pNew) MdlNodeReference(*static_cast<MdlNodeReference32*>(p));
        pNode = static_cast<MdlNode*>(pNew);
    } else if (p->m_nFlags == MdlNode::MDLNODE_TRIMESH_ID) {
        MdlNodeTriMesh* pNew = static_cast<MdlNodeTriMesh*>(TakeBlockMemory(sizeof(MdlNodeTriMesh)));
        new (pNew) MdlNodeTriMesh(*static_cast<MdlNodeTriMesh32*>(p));
        pNode = static_cast<MdlNode*>(pNew);
    } else if (p->m_nFlags == MdlNode::MDLNODE_SKIN_ID) {
        MdlNodeSkin* pNew = static_cast<MdlNodeSkin*>(TakeBlockMemory(sizeof(MdlNodeSkin)));
        new (pNew) MdlNodeSkin(*static_cast<MdlNodeSkin32*>(p));
        pNode = static_cast<MdlNode*>(pNew);
    } else if (p->m_nFlags == MdlNode::MDLNODE_ANIM_ID) {
        // no node is built for this kind
    } else if (p->m_nFlags == MdlNode::MDLNODE_DANGLY_ID) {
        MdlNodeDanglyMesh* pNew = static_cast<MdlNodeDanglyMesh*>(TakeBlockMemory(sizeof(MdlNodeDanglyMesh)));
        new (pNew) MdlNodeDanglyMesh(*static_cast<MdlNodeDanglyMesh32*>(p));
        pNode = static_cast<MdlNode*>(pNew);
    } else if (p->m_nFlags == MdlNode::MDLNODE_AABB_ID) {
        MdlNodeAABB* pNew = static_cast<MdlNodeAABB*>(TakeBlockMemory(sizeof(MdlNodeAABB)));
        new (pNew) MdlNodeAABB(*static_cast<MdlNodeAABB32*>(p));
        pNode = static_cast<MdlNode*>(pNew);
    } else if (p->m_nFlags == MdlNode::MDLNODE_TRIGGER_ID) {
        // no node is built for this kind
    } else if (p->m_nFlags == MdlNode::MDLNODE_LIGHTSABER_ID) {
        MdlNodeLightsaber* pNew = static_cast<MdlNodeLightsaber*>(TakeBlockMemory(sizeof(MdlNodeLightsaber)));
        new (pNew) MdlNodeLightsaber(*static_cast<MdlNodeLightsaber32*>(p));
        pNode = static_cast<MdlNode*>(pNew);
    } else {
        // unknown kinds get a base node
        MdlNode* pNew = static_cast<MdlNode*>(TakeBlockMemory(sizeof(MdlNode)));
        new (pNew) MdlNode(*p);
        pNode = pNew;
    }

    if (pNode != 0) {
        ++s_nMdlNodeCount;
        pBlock->m_mdlNodes[p] = pNode;
    }
    return pNode;
}
