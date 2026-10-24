#include "k2stdint.h"
#include "aurres.h"
#include "aurio.h"
#include "AURORA/aurmodel.h"
#include "aurglcore.h"
#include "PointerMap.h"
#include "auremitter.h"
#include "aurtexman.h"
#include "resgff.h"
#define CEXO_ARRAYLIST_KEEP_ADDUNIQUE_OUT_OF_LINE 1
#include "CExoArrayListImpl.h"
#include <map>
#include <stdio.h>
#include <string.h>

extern int usenewalpha;
extern float materialalpha;

char* FuncInterp(char* pszCommand);
char* dofile(char* pszName);
int   g_nModelsRead;
extern Model* currentmodel;

Model* FindModel(char* pszName);
void  InsertModel(Model* pModel);
void  UpdateAnimFootprint(MdlNode* pNode, int nDelta);
void  ModelDestructor(void* pModel);
void  ModelParseField(void* pModel, char* pszLine);
CAurTexture* AurTextureGetReference(char* sName, char* sExtension);
extern int animationfootprint;
extern bool gHasRevComps_Mac;
extern int RyanTotalLevelVertices;
extern int RyanTotalLevelVertSize;
namespace ASL { unsigned int GetPrefAsUInt32(const char* pszKey, unsigned int nDefault); }
void  ByteSwap(uint64_t* p);
void  MdlNodeTriMeshGenVertices(void* pv);
void  MdlNodeTriMeshRemoveTemporaryArrays(void* pv);
void  MdlNodeAnimMeshGenVertices(void* pv);
void  MdlNodeAnimMeshRemoveTemporaryArrays(void* pv);
void  MdlNodeDanglyMeshGenVertices(void* pv);
void  MdlNodeDanglyMeshRemoveTemporaryArrays(void* pv);
void  MdlNodeSkinGenVertices(void* pv);
void  MdlNodeSkinRemoveTemporaryArrays(void* pv);

// The async-spawn queue's element, only ever held by pointer.
class AsyncEntry {
public:
    char m_abPad00[24];
    void* m_pHandle;
};

// The binary .mdl importer, reached when a resource's first word is 0. An anonymous-namespace class
// of this file, so every one of its methods is file-local.
//
// The two leading fields are the .mdl header's two sizes, which Read stores as it consumes them.
// The four Reset* helpers take no `this`. The node or node slot comes first and the image base
// second, and ResetMdlNode and ResetAABBTree take the pointer slot because the body re-reads it
// after every call.
namespace {
class InputBinary {
public:
    uint64_t m_nMdlSize;
    unsigned int  m_nMdxSize;
    char m_abPad0c[0x18 - 0x0c];
    void* m_pHandle;
    MaxTree* Read(void* pRes, void* pMdx);
    static void ResetMdlNode(MdlNode*& pNode, unsigned char* pBase);
    static void ResetAABBTree(AABB*& pAABB, unsigned char* pBase);
    static void ResetTriMeshParts(MdlNodeTriMesh* pMesh, unsigned char* pBase);
    static void ResetMdlNodeParts(MdlNode* pNode, unsigned char* pBase);
};
}

// The override path is a separate global from the IODispatcher singleton and its guard byte.
static char s_szOverridePath[64];

// The fortified sprintf wrapper, file-local. Its signature is (dst, length, fmt, ...) rather than
// the builtin's (dst, fmt, ...) because the size passed here is not a literal.
#include "port/libcbase.h"
#include "port/crtapi.h"
static int sprintf_s(char* buf, uint64_t n, const char* fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    int nResult = __vsprintf_chk(buf, 0, n, fmt, ap);
    __builtin_va_end(ap);
    return nResult;
}

IODispatcher::IODispatcher() {
}

IODispatcher::~IODispatcher() {
}

IODispatcher* IODispatcher::GetRef() {
    static IODispatcher s_instance;
    return &s_instance;
}

// Writes the .mdl header bytes to a fixed d:\override\ path. The result is deliberately ignored,
// so every caller reads failure.
bool IODispatcher::Write(MaxTree* pTree, char* pszName) {
    if (pszName)
        sprintf_s(s_szOverridePath, sizeof(s_szOverridePath), "d:\\override\\%s", pszName);
    else
        sprintf_s(s_szOverridePath, sizeof(s_szOverridePath), "d:\\override\\%s.mdl", pTree->m_szName);
#if KOTOR1  // K1: the port's logging `_fopen`
    fclose(_fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/libsource/AURORA/aurio.cpp",
                  "Write", 498, s_szOverridePath, "wb"));
#else
    fclose(fopen(s_szOverridePath, "wb"));
#endif
    return false;
}

// The .mdl/.mdx loader. The first byte of AurResGetDataBytes(4, pRes) selects the format: non-zero
// is the old text form, where the path is rewritten to .mdl and dofile's line interpreter builds the
// tree; zero is the binary form, handed to the InputBinary importer.
MaxTree* IODispatcher::ReadSync(char* pszName) {
    char szPath[128];
    InputBinary sBinary;
    sBinary.m_pHandle = 0;

    g_nModelsRead = g_nModelsRead + 1;
    void* pMdl = AurResGet(pszName, const_cast<char*>(".mdl"), 0, true);
    void* pMdx = AurResGet(pszName, const_cast<char*>(".mdx"), 0, true);
    if (pMdl == 0)
        return 0;

    unsigned char* pData = AurResGetDataBytes(4, pMdl);
    if (pData == 0)
        return 0;

    if (*pData != 0) {
        if (pMdx != 0)
            AurResFree(pMdx, 0);
        AurResFreeDataBytes(pMdl, pData);
        AurResFree(pMdl, 0);

        sprintf_s(szPath, sizeof(szPath), "%s%s", pszName, ".mdl");
        Model* pSaved = currentmodel;
        dofile(szPath);
        Model* pResult = currentmodel;
        currentmodel = pSaved;
        return pResult;
    }

    AurResFreeDataBytes(pMdl, pData);
    return sBinary.Read(pMdl, pMdx)->AsModel();
}

#if KOTOR1
MaxTree* IODispatcher::ReadSync(SDL_RWops* pMdlFile, SDL_RWops* pMdxFile) {
#else
// The stdio flavour runs the same first-byte format test, but the text branch interprets the lines
// itself rather than rewriting a path. The line scan is the same switch dofile carries, with a
// leading blank or tab skipped and the trailing blanks trimmed before interpretation.
MaxTree* IODispatcher::ReadSync(FILE* pMdlFile, FILE* pMdxFile) {
#endif
    InputBinary sBinary;
    sBinary.m_pHandle = 0;

    void* pMdl = AurResGet(pMdlFile);
    if (pMdl == 0)
        return 0;

    unsigned char* pData = AurResGetDataBytes(4, pMdl);
    if (pData == 0)
        return 0;

    unsigned char nFirst = *pData;
    AurResFreeDataBytes(pMdl, pData);

    if (nFirst == 0)
        return sBinary.Read(pMdl, AurResGet(pMdxFile))->AsModel();

    Model* pSaved = currentmodel;
    char* pLine = AurResGetNextLine();
    while (pLine) {
        for (;;) {
            switch (static_cast<unsigned char>(*pLine)) {
            case '\t':
            case ' ':
                pLine++;
                continue;
            case 0:
            case '#':
                break;
            default: {
                uint64_t nLen = strlen(pLine);
                char* p = pLine + nLen - 1;
                while (p >= pLine) {
                    if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
                        *p-- = 0;
                    else
                        break;
                }
                FuncInterp(pLine);
                break;
            }
            }
            break;
        }
        pLine = AurResGetNextLine();
    }

    AurResFree(pMdl, 0);
    Model* pResult = currentmodel;
    currentmodel = pSaved;
    return pResult;
}

// A stub that allocates a queue entry, nulls one field and queues it. The name is never read and
// nothing is ever spawned; RetreiveAsync is its matching half.
AsyncEntry* IODispatcher::PreSpawnAsync(char* pszName) {
    K2_UNUSED(pszName);
    AsyncEntry* pEntry = new AsyncEntry;
    pEntry->m_pHandle = 0;
    m_asyncList.Add(pEntry);
    return pEntry;
}

// Counts how many queued entries equal the handle, frees the handle when it was found, and always
// reports 0.
MaxTree* IODispatcher::RetreiveAsync(void* pHandle) {
    int nFound = m_asyncList.Contains(pHandle);
    if (pHandle != 0 && nFound != 0)
        operator delete(pHandle);
    return 0;
}

// ================================================================================================
// The binary .mdl/.mdx importer (InputBinary, the inverse of the image writer below)
// ================================================================================================
//
// The .mdl image is the 32-bit relocatable form, where every pointer slot holds a 32-bit byte
// offset into the image. PointerMap converts each record (the *32 mirror to the runtime class,
// sign-extending the offsets into the 8-byte pointer slots), and the importer then walks the
// runtime tree re-pointing every slot: sub-records go back through PointerMap::Create*, flat arrays
// are copied into the active memory block with TakeBlockMemory, and the vertex and index payloads
// are uploaded into GLRender pools out of the .mdx image. ByteSwap is a no-op on this little-endian
// target.
//
// TakeBlockMemory counts 32-byte units, not bytes, and every caller passes a byte count, so the
// importer over-reserves 32-fold; the pool is sized to match, 1.5x the image in units. A
// K2_FIX_ENGINE_BUGS build packs the carves instead (PointerMap.cpp).

// Image addresses are formed by integer addition with the offset as the first operand. How the
// offset is widened (signed, unsigned, or not at all) is each call site's own cast, so the template
// parameter carries it.
static inline void* ImageAddress(int64_t nOffset, unsigned char* pBase)
{
    return reinterpret_cast<void*>(nOffset + reinterpret_cast<uintptr_t>(pBase));
}

// For a List<T> whose m_pData still holds an image offset, copies its m_count elements into block
// memory, or nulls it when empty. The count is re-read after TakeBlockMemory because the list is
// reached by reference.
//
// TOffset is how the slot's offset is read. The base node's and the derived kinds' lists read the
// full 8-byte slot, but ResetTriMeshParts reads its face list signed and its four index and bone
// lists as a 32-bit unsigned word.
template <class TOffset = uint64_t, class T>
static inline void RelocateList(List<T>& list, unsigned char* pBase)
{
    if (list.m_count == 0) {
        list.m_pData = 0;
    } else {
        void* pSource = ImageAddress(static_cast<TOffset>(reinterpret_cast<uintptr_t>(list.m_pData)), pBase);
        list.m_pData = static_cast<T*>(PointerMap::s_pointerMap.TakeBlockMemory(list.m_count * sizeof(T)));
        memcpy(static_cast<void*>(list.m_pData), pSource, list.m_count * sizeof(T));  // idiom-exempt: a bulk image copy
    }
}

// The same for a raw array whose element count lives in a separate field. The count is not re-read
// after the allocation: the byte size is computed once and feeds both calls.
template <class T>
static inline void RelocateArray(T*& pArray, int nCount, uint64_t nElementSize, unsigned char* pBase)
{
    if (nCount == 0) {
        pArray = 0;
    } else {
        void* pSource = ImageAddress(reinterpret_cast<uintptr_t>(pArray), pBase);
        uint64_t nSize = nCount * nElementSize;
        pArray = static_cast<T*>(PointerMap::s_pointerMap.TakeBlockMemory(static_cast<int>(nSize)));
        memcpy(pArray, pSource, nSize);  // idiom-exempt: a bulk image copy
    }
}

// Consumes the .mdl header (the two sizes after the zero word ReadSync already tested), copies the
// image and the .mdx payload out of the resource cache, opens a PointerMap block sized 1.5x the
// image, and relocates the Model: its string table, its animations (each with its event table and
// node tree), the animation root and the root node. The vertex payload goes into a fresh GLRender
// pool whose handle replaces the .mdx offset m_nPool carried in the image.
//
// The .mdx bytes are freed through the .mdl resource.
MaxTree* InputBinary::Read(void* pRes, void* pMdx)
{
    unsigned char* pData = AurResGetDataBytes(4, pRes);
    if (pData == 0)
        return 0;
    m_nMdlSize = *reinterpret_cast<unsigned int*>(pData);
    AurResFreeDataBytes(pRes, pData);
    pData = AurResGetDataBytes(4, pRes);
    m_nMdxSize = *reinterpret_cast<unsigned int*>(pData);
    AurResFreeDataBytes(pRes, pData);

    pData = AurResGetDataBytes(static_cast<unsigned int>(m_nMdlSize), pRes);
    unsigned char* pMdl = new unsigned char[m_nMdlSize];
    memcpy(pMdl, pData, m_nMdlSize);  // idiom-exempt: the raw .mdl image
    PointerMap::s_pointerMap.PushActiveBlock(
        PointerMap::s_pointerMap.AllocateMemoryBlock(static_cast<int>(m_nMdlSize * 1.5)));
    AurResFreeDataBytes(pRes, pData);

    pData = m_nMdxSize ? AurResGetDataBytes(m_nMdxSize, pMdx) : 0;
    unsigned char* pMdxData = 0;
    if (m_nMdxSize) {
        pMdxData = new unsigned char[m_nMdxSize];
        memcpy(pMdxData, pData, m_nMdxSize);  // idiom-exempt: the raw .mdx payload
    }
    AurResFreeDataBytes(pRes, pData);

    MaxTree* pTree = PointerMap::s_pointerMap.CreateMaxTreeNode(reinterpret_cast<MaxTree32*>(pMdl));
    Model* pModel = static_cast<Model*>(pTree);
    ByteSwap(reinterpret_cast<uint64_t*>(&pModel->m_pMdlNode));
    ByteSwap(&pModel->m_nNodeCount);
    ByteSwap(&pModel->m_nFlags);
    ByteSwap(&pModel->m_nRefCount);
    ByteSwap(reinterpret_cast<uint64_t*>(&pModel->m_animations.m_pData));
    ByteSwap(&pModel->m_animations.m_count);
    ByteSwap(&pModel->m_animations.m_capacity);
    ByteSwap(&pModel->m_vBoundsMin.x);
    ByteSwap(&pModel->m_vBoundsMin.y);
    ByteSwap(&pModel->m_vBoundsMin.z);
    ByteSwap(&pModel->m_vBoundsMax.x);
    ByteSwap(&pModel->m_vBoundsMax.y);
    ByteSwap(&pModel->m_vBoundsMax.z);
    ByteSwap(&pModel->m_fRadius);
    ByteSwap(&pModel->m_fAnimScale);
    ByteSwap(reinterpret_cast<uint64_t*>(&pModel->m_pAnimationRoot));
    ByteSwap(&pModel->m_nPool);
    ByteSwap(reinterpret_cast<unsigned int*>(&pModel->m_nPoolSize));
    ByteSwap(reinterpret_cast<uint64_t*>(&pModel->m_listStrings.m_pData));
    ByteSwap(&pModel->m_listStrings.m_count);
    ByteSwap(&pModel->m_listStrings.m_capacity);

    pModel->m_pMdlResource = pRes;
    pModel->m_cType = static_cast<char>(pTree->GetType() | MaxTree::kTreeCompiled);
    pModel->m_pSuperModel = FindModel(pModel->m_szSuperModelName);
    if (pModel->m_pSuperModel)
        pModel->m_pSuperModel->AddRef(pModel);
    pTree->ResetFreeLists();

    // The image's `m_nPool` is the payload's offset into the .mdx; the pool handle replaces it.
    unsigned int nMdxOffset = pModel->m_nPool;
    GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(0), pModel->m_nPoolSize, 2, 0, &pModel->m_nPool);
    memcpy(GLRender::LockPool(pModel->m_nPool, 0, 0, 1), ImageAddress(nMdxOffset, pMdxData),
           static_cast<unsigned int>(pModel->m_nPoolSize));
    GLRender::UnlockPool(pModel->m_nPool);
    pTree->AddToFreeTokenList(pModel->m_nPool);

    // The name table is an array of 32-bit string offsets, each string re-homed in block memory.
    // The table pointer is read before the count is cleared and narrowed to its 32-bit offset only
    // inside the if.
    int nStrings = pModel->m_listStrings.m_count;
    char** pStringTable = pModel->m_listStrings.m_pData;   // still an image offset
    pModel->m_listStrings.m_count = 0;
    if (nStrings > 0) {
        int* pStringOffsets = reinterpret_cast<int*>(pMdl + static_cast<int>(reinterpret_cast<intptr_t>(pStringTable)));
        pModel->m_listStrings.m_pData =
            static_cast<char**>(PointerMap::s_pointerMap.TakeBlockMemory(nStrings * sizeof(char*)));
        for (int i = 0; i < nStrings; i++) {
            char* pszSource = static_cast<char*>(ImageAddress(pStringOffsets[i], pMdl));
            char* pszName = static_cast<char*>(PointerMap::s_pointerMap.TakeBlockMemory(static_cast<int>(strlen(pszSource) + 1)));
            strcpy(pszName, pszSource);
            pModel->m_listStrings.Add(pszName);
        }
    }

    int nAnimations = pModel->m_animations.m_count;
    Animation** pAnimationTable = pModel->m_animations.m_pData;   // still an image offset
    pModel->m_animations.m_count = 0;
    if (nAnimations > 0) {
        int* pAnimationOffsets = reinterpret_cast<int*>(pMdl + static_cast<int>(reinterpret_cast<intptr_t>(pAnimationTable)));
        pModel->m_animations.m_pData =
            static_cast<Animation**>(PointerMap::s_pointerMap.TakeBlockMemory(nAnimations * sizeof(Animation*)));
        for (int i = 0; i < nAnimations; i++) {
            pModel->m_animations.Add(static_cast<Animation*>(PointerMap::s_pointerMap.CreateMaxTreeNode(
                static_cast<MaxTree32*>(ImageAddress(pAnimationOffsets[i], pMdl)))));
            // A reference to the slot, not a cached pointer, since the animation is re-loaded out
            // of the list after every call.
            Animation*& pAnimation = pModel->m_animations.m_pData[i];
            ByteSwap(reinterpret_cast<uint64_t*>(&pAnimation->m_pMdlNode));
            ByteSwap(&pAnimation->m_nNodeCount);
            ByteSwap(&pAnimation->m_fLength);
            ByteSwap(&pAnimation->m_fTransTime);
            ByteSwap(reinterpret_cast<uint64_t*>(&pAnimation->m_events.m_pData));
            ByteSwap(&pAnimation->m_events.m_count);
            ByteSwap(&pAnimation->m_events.m_capacity);
            // The writer zeroes an animation's owner slot, so offset 0, the model itself, is the
            // whole argument; the PointerMap already holds it.
            pAnimation->m_pModel =
                static_cast<Model*>(PointerMap::s_pointerMap.CreateMaxTreeNode(reinterpret_cast<MaxTree32*>(pMdl)));
            RelocateList(pAnimation->m_events, pMdl);
            pAnimation->m_pMdlNode = PointerMap::s_pointerMap.CreateMdlNode(
                static_cast<MdlNode32*>(ImageAddress(static_cast<int>(reinterpret_cast<intptr_t>(pAnimation->m_pMdlNode)), pMdl)));
            ResetMdlNode(pAnimation->m_pMdlNode, pMdl);
            UpdateAnimFootprint(pAnimation->m_pMdlNode, 1);
            animationfootprint = animationfootprint
                               + pAnimation->m_events.m_count * static_cast<int>(sizeof(AnimationEvent))
                               + static_cast<int>(sizeof(Animation) + sizeof(Animation*));
            pAnimation->m_pfnDestructor = MaxTreeDestructor;
            pAnimation->m_pfnParseField = MaxTreeParseField;
        }
    }

    if (pModel->m_pAnimationRoot)
        pModel->m_pAnimationRoot = PointerMap::s_pointerMap.CreateMdlNode(
            static_cast<MdlNode32*>(ImageAddress(static_cast<int>(reinterpret_cast<intptr_t>(pModel->m_pAnimationRoot)), pMdl)));
    pModel->m_pMdlNode = PointerMap::s_pointerMap.CreateMdlNode(
        static_cast<MdlNode32*>(ImageAddress(static_cast<int>(reinterpret_cast<intptr_t>(pModel->m_pMdlNode)), pMdl)));
    ResetMdlNode(pModel->m_pMdlNode, pMdl);
    pModel->m_pfnDestructor = ModelDestructor;
    pModel->m_pfnParseField = ModelParseField;
    InsertModel(pModel);
    pTree->AsModel()->m_pMdxResource = pMdx;
    pTree->AsModel()->m_pMdlResource = pRes;
    AurResFree(pRes, 0);
    AurResRemove(pMdx);
    if (pMdxData)
        delete[] pMdxData;
    PointerMap::s_pointerMap.PopActiveBlock();
    delete[] pMdl;
    return pTree;
}

// Swaps the base record, then dispatches on the kind word. The tests compare against the
// MdlNode::MDLNODE_*_ID constants rather than literals, so this is an if-chain rather than a switch.
// The trimesh-family arms re-home their own arrays and then run ResetTriMeshParts, which does the
// base parts too; every other arm runs ResetMdlNodeParts directly.
void InputBinary::ResetMdlNode(MdlNode*& pNode, unsigned char* pBase)
{
    ByteSwap(reinterpret_cast<short*>(&pNode->m_nFlags));
    unsigned short nType = pNode->m_nFlags;
    ByteSwap(&pNode->m_nNodeID);
    ByteSwap(&pNode->m_nNameIndex);
    ByteSwap(reinterpret_cast<uint64_t*>(&pNode->m_pTree));
    ByteSwap(reinterpret_cast<uint64_t*>(&pNode->m_pParent));
    ByteSwap(&pNode->m_vPosition.x);
    ByteSwap(&pNode->m_vPosition.y);
    ByteSwap(&pNode->m_vPosition.z);
    ByteSwap(&pNode->m_qOrientation.w);
    ByteSwap(&pNode->m_qOrientation.x);
    ByteSwap(&pNode->m_qOrientation.y);
    ByteSwap(&pNode->m_qOrientation.z);
    ByteSwap(reinterpret_cast<uint64_t*>(&pNode->m_listChildren.m_pData));
    ByteSwap(&pNode->m_listChildren.m_count);
    ByteSwap(&pNode->m_listChildren.m_capacity);
    ByteSwap(reinterpret_cast<uint64_t*>(&pNode->m_listControllers.m_pData));
    ByteSwap(&pNode->m_listControllers.m_count);
    ByteSwap(&pNode->m_listControllers.m_capacity);
    ByteSwap(reinterpret_cast<uint64_t*>(&pNode->m_listFloats.m_pData));
    ByteSwap(&pNode->m_listFloats.m_count);
    ByteSwap(&pNode->m_listFloats.m_capacity);

    if (nType == MdlNode::MDLNODE_AABB_ID) {
        MdlNodeAABB* pAABB = static_cast<MdlNodeAABB*>(pNode);
        ByteSwap(reinterpret_cast<uint64_t*>(&pAABB->m_pAABB));
        pAABB->m_pAABB = PointerMap::s_pointerMap.CreateAABB(
            static_cast<AABB32*>(ImageAddress(static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pAABB->m_pAABB)), pBase)));
        ResetAABBTree(pAABB->m_pAABB, pBase);
        pAABB->m_pfnGenVertices = MdlNodeTriMeshGenVertices;
        pAABB->m_pfnRemoveTemporaryArrays = MdlNodeTriMeshRemoveTemporaryArrays;
        ResetTriMeshParts(pAABB, pBase);
    } else if (nType == MdlNode::MDLNODE_ANIM_ID) {
        MdlNodeAnimMesh* pAnim = static_cast<MdlNodeAnimMesh*>(pNode);
        RelocateList(pAnim->m_lAnimVerts, pBase);
        RelocateList(pAnim->m_lAnimTVerts, pBase);
        RelocateList(pAnim->m_lAnimNormals, pBase);
        // Four bytes per frame here, not the arrays' 12/8-byte stride.
        RelocateArray(pAnim->m_paVertexFrames, pAnim->m_nVertexFrameCount, sizeof(float), pBase);
        RelocateArray(pAnim->m_paTexCoordFrames, pAnim->m_nTexCoordFrameCount, sizeof(float), pBase);
        pAnim->m_pfnGenVertices = MdlNodeAnimMeshGenVertices;
        pAnim->m_pfnRemoveTemporaryArrays = MdlNodeAnimMeshRemoveTemporaryArrays;
        ResetTriMeshParts(pAnim, pBase);
    } else if (nType == MdlNode::MDLNODE_BASE_ID || nType == MdlNode::MDLNODE_CAMERA_ID) {
        ResetMdlNodeParts(pNode, pBase);
    } else if (nType == MdlNode::MDLNODE_DANGLY_ID) {
        MdlNodeDanglyMesh* pDangly = static_cast<MdlNodeDanglyMesh*>(pNode);
        ResetTriMeshParts(pDangly, pBase);
        ByteSwap(reinterpret_cast<uint64_t*>(&pDangly->m_lConstraints.m_pData));
        ByteSwap(&pDangly->m_lConstraints.m_count);
        ByteSwap(&pDangly->m_lConstraints.m_capacity);
        ByteSwap(&pDangly->m_fDisplacement);
        ByteSwap(&pDangly->m_fTightness);
        ByteSwap(&pDangly->m_fPeriod);
        ByteSwap(reinterpret_cast<uint64_t*>(&pDangly->m_pVertexSource));
        pDangly->m_pfnGenVertices = MdlNodeDanglyMeshGenVertices;
        pDangly->m_pfnRemoveTemporaryArrays = MdlNodeDanglyMeshRemoveTemporaryArrays;
        RelocateList(pDangly->m_lConstraints, pBase);
        // The vertex count is signed here, as in ResetTriMeshParts.
        int64_t nVertices = static_cast<short>(pDangly->m_nVertexCount);
        if (nVertices > 0) {
            void* pSource = ImageAddress(reinterpret_cast<uintptr_t>(pDangly->m_pVertexSource), pBase);
            uint64_t nSize = nVertices * sizeof(Vector3);
            pDangly->m_pVertexSource = PointerMap::s_pointerMap.TakeBlockMemory(static_cast<int>(nSize));
            memcpy(pDangly->m_pVertexSource, pSource, nSize);  // idiom-exempt: a bulk image copy
        }
    } else if (nType == MdlNode::MDLNODE_EMITTER_ID) {
        MdlNodeEmitter* pEmitter = static_cast<MdlNodeEmitter*>(pNode);
        ResetMdlNodeParts(pEmitter, pBase);
        ByteSwap(&pEmitter->m_fDeadSpace);
        ByteSwap(&pEmitter->m_fBlastRadius);
        ByteSwap(&pEmitter->m_fBlastLength);
        ByteSwap(&pEmitter->m_nNumBranches);
        ByteSwap(&pEmitter->m_bControlPtSmoothing);
        ByteSwap(&pEmitter->m_nXGrid);
        ByteSwap(&pEmitter->m_nYGrid);
        ByteSwap(&pEmitter->m_nSpawnType);
        ByteSwap(&pEmitter->m_nTwoSidedTex);
        ByteSwap(&pEmitter->m_nLoop);
        ByteSwap(&pEmitter->m_nRenderOrder);
        ByteSwap(reinterpret_cast<int*>(&pEmitter->m_nEmitterFlags));
    } else if (nType == MdlNode::MDLNODE_LIGHT_ID) {
        MdlNodeLight* pLight = static_cast<MdlNodeLight*>(pNode);
        ResetMdlNodeParts(pLight, pBase);
        ByteSwap(&pLight->m_fFlareRange);
        ByteSwap(reinterpret_cast<uint64_t*>(&pLight->m_lFlareTextures.m_pData));
        ByteSwap(&pLight->m_lFlareTextures.m_count);
        ByteSwap(&pLight->m_lFlareTextures.m_capacity);
        ByteSwap(reinterpret_cast<uint64_t*>(&pLight->m_lFlareSizes.m_pData));
        ByteSwap(&pLight->m_lFlareSizes.m_count);
        ByteSwap(&pLight->m_lFlareSizes.m_capacity);
        ByteSwap(reinterpret_cast<uint64_t*>(&pLight->m_lFlarePositions.m_pData));
        ByteSwap(&pLight->m_lFlarePositions.m_count);
        ByteSwap(&pLight->m_lFlarePositions.m_capacity);
        ByteSwap(reinterpret_cast<uint64_t*>(&pLight->m_lFlareColourShifts.m_pData));
        ByteSwap(&pLight->m_lFlareColourShifts.m_count);
        ByteSwap(&pLight->m_lFlareColourShifts.m_capacity);
        ByteSwap(reinterpret_cast<uint64_t*>(&pLight->m_lFlareNames.m_pData));
        ByteSwap(&pLight->m_lFlareNames.m_count);
        ByteSwap(&pLight->m_lFlareNames.m_capacity);
        ByteSwap(&pLight->m_nPriority);
        ByteSwap(&pLight->m_bAmbientOnly);
        ByteSwap(&pLight->m_bIsDynamic);
        ByteSwap(&pLight->m_bAffectDynamic);
        ByteSwap(&pLight->m_bShadow);
        ByteSwap(&pLight->m_bGenerateFlare);
        ByteSwap(&pLight->m_nFadingEnabled);

        // The flare names are an offset table of strings, each re-homed in block memory.
        int nNames = pLight->m_lFlareNames.m_count;
        int* pNameOffsets = static_cast<int*>(ImageAddress(static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pLight->m_lFlareNames.m_pData)), pBase));
        pLight->m_lFlareNames.m_count = 0;
        if (nNames > 0) {
            pLight->m_lFlareNames.m_pData =
                static_cast<char**>(PointerMap::s_pointerMap.TakeBlockMemory(nNames * sizeof(char*)));
            for (int i = 0; i < nNames; i++) {
                char* pszSource = static_cast<char*>(ImageAddress(pNameOffsets[i], pBase));
                char* pszName =
                    static_cast<char*>(PointerMap::s_pointerMap.TakeBlockMemory(static_cast<int>(strlen(pszSource) + 1)));
                strcpy(pszName, pszSource);
                pLight->m_lFlareNames.Add(pszName);
            }
        }
        RelocateList(pLight->m_lFlareSizes, pBase);
        RelocateList(pLight->m_lFlarePositions, pBase);
        RelocateList(pLight->m_lFlareColourShifts, pBase);
        // The texture list is rebuilt from the names; the image's texture slots are meaningless.
        pLight->m_lFlareTextures.SetSize(pLight->m_lFlareNames.m_count);
        for (int i = pLight->m_lFlareNames.m_count; i-- != 0;)
            pLight->m_lFlareTextures.m_pData[i] =
                AurTextureGetReference(pLight->m_lFlareNames.m_pData[i], 0);
        pLight->AddToFreeList(pLight);
    } else if (nType == MdlNode::MDLNODE_REFERENCE_ID) {
        MdlNodeReference* pReference = static_cast<MdlNodeReference*>(pNode);
        ByteSwap(&pReference->m_bReattachable);
        ResetMdlNodeParts(pReference, pBase);
    } else if (nType == MdlNode::MDLNODE_SKIN_ID) {
        MdlNodeSkin* pSkin = static_cast<MdlNodeSkin*>(pNode);
        ResetTriMeshParts(pSkin, pBase);
        ByteSwap(reinterpret_cast<uint64_t*>(&pSkin->m_lWeights.m_pData));
        ByteSwap(&pSkin->m_lWeights.m_count);
        ByteSwap(&pSkin->m_lWeights.m_capacity);
        ByteSwap(static_cast<unsigned int*>(&pSkin->m_nBoneWeightsOfs));
        ByteSwap(static_cast<unsigned int*>(&pSkin->m_nBoneIndicesOfs));
        ByteSwap(&pSkin->m_nBoneCount);
        ByteSwap(reinterpret_cast<uint64_t*>(&pSkin->m_paBoneMap));
        ByteSwap(reinterpret_cast<uint64_t*>(&pSkin->m_lQBoneRefInv.m_pData));
        ByteSwap(&pSkin->m_lQBoneRefInv.m_count);
        ByteSwap(&pSkin->m_lQBoneRefInv.m_capacity);
        ByteSwap(reinterpret_cast<uint64_t*>(&pSkin->m_lTBoneRefInv.m_pData));
        ByteSwap(&pSkin->m_lTBoneRefInv.m_count);
        ByteSwap(&pSkin->m_lTBoneRefInv.m_capacity);
        ByteSwap(reinterpret_cast<uint64_t*>(&pSkin->m_lBoneConstantIndices.m_pData));
        ByteSwap(&pSkin->m_lBoneConstantIndices.m_count);
        ByteSwap(&pSkin->m_lBoneConstantIndices.m_capacity);
        // The skin's bone map holds 17 entries.
        for (int i = 0; i < static_cast<int>(sizeof(static_cast<MdlNodeSkin32*>(0)->m_aBoneNodeIndices) / sizeof(short)); i++)
            ByteSwap(&pSkin->m_aBoneNodeIndices[i]);
        pSkin->m_pfnGenVertices = MdlNodeSkinGenVertices;
        pSkin->m_pfnRemoveTemporaryArrays = MdlNodeSkinRemoveTemporaryArrays;
        RelocateArray(pSkin->m_paBoneMap, pSkin->m_nBoneCount, sizeof(float), pBase);
        RelocateList(pSkin->m_lQBoneRefInv, pBase);
        RelocateList(pSkin->m_lTBoneRefInv, pBase);
        RelocateList(pSkin->m_lBoneConstantIndices, pBase);
    } else if (nType == MdlNode::MDLNODE_TRIGGER_ID) {
        ResetMdlNodeParts(pNode, pBase);
    } else if (nType == MdlNode::MDLNODE_LIGHTSABER_ID) {
        MdlNodeLightsaber* pSaber = static_cast<MdlNodeLightsaber*>(pNode);
        ResetTriMeshParts(pSaber, pBase);
        ByteSwap(reinterpret_cast<uint64_t*>(&pSaber->m_paPositions));
        ByteSwap(reinterpret_cast<uint64_t*>(&pSaber->m_aTexCoords));
        ByteSwap(reinterpret_cast<uint64_t*>(&pSaber->m_aNormals));
        pSaber->m_pfnGenVertices = MdlNodeTriMeshGenVertices;
        pSaber->m_pfnRemoveTemporaryArrays = MdlNodeTriMeshRemoveTemporaryArrays;
        // The two blade pools take 32 bytes per piece vertex, and a 20-byte (three position floats
        // and two texture floats) vertex per segment vertex per back segment. NUM_SEGMENT_VERTS is
        // read signed here although its declared type is unsigned short.
        GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(0),
                              MdlNodeLightsaber::NUM_PIECE_VERTS * 32, 2, 0x1012,
                              &pSaber->m_anBladePools[0]);
        GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(0),
                              5 * sizeof(float) * static_cast<short>(MdlNodeLightsaber::NUM_SEGMENT_VERTS)
                                  * MdlNodeLightsaber::NUM_BACK_SEGMENTS,
                              2, 0x1010, &pSaber->m_anBladePools[1]);
        void* pSource = ImageAddress(reinterpret_cast<uintptr_t>(pSaber->m_paPositions), pBase);
        uint64_t nSize = static_cast<short>(pSaber->m_nVertexCount) * sizeof(Vector3);
        pSaber->m_paPositions = static_cast<Vector3*>(PointerMap::s_pointerMap.TakeBlockMemory(static_cast<int>(nSize)));
        memcpy(pSaber->m_paPositions, pSource, nSize);  // idiom-exempt: a bulk image copy
        pSource = ImageAddress(reinterpret_cast<uintptr_t>(pSaber->m_aTexCoords), pBase);
        nSize = static_cast<short>(pSaber->m_nVertexCount) * sizeof(Vector2);
        pSaber->m_aTexCoords = static_cast<Vector2*>(PointerMap::s_pointerMap.TakeBlockMemory(static_cast<int>(nSize)));
        memcpy(pSaber->m_aTexCoords, pSource, nSize);  // idiom-exempt: a bulk image copy
        pSource = ImageAddress(reinterpret_cast<uintptr_t>(pSaber->m_aNormals), pBase);
        nSize = static_cast<short>(pSaber->m_nVertexCount) * sizeof(Vector3);
        pSaber->m_aNormals = static_cast<Vector3*>(PointerMap::s_pointerMap.TakeBlockMemory(static_cast<int>(nSize)));
        memcpy(pSaber->m_aNormals, pSource, nSize);  // idiom-exempt: a bulk image copy
        for (int64_t i = 0; i < static_cast<short>(pSaber->m_nVertexCount); i++) {
            ByteSwap(&pSaber->m_paPositions[i].x);
            ByteSwap(&pSaber->m_paPositions[i].y);
            ByteSwap(&pSaber->m_paPositions[i].z);
            ByteSwap(&pSaber->m_aTexCoords[i].u);
            ByteSwap(&pSaber->m_aTexCoords[i].v);
            ByteSwap(&pSaber->m_aNormals[i].x);
            ByteSwap(&pSaber->m_aNormals[i].y);
            ByteSwap(&pSaber->m_aNormals[i].z);
        }
    } else if (nType == MdlNode::MDLNODE_TRIMESH_ID) {
        MdlNodeTriMesh* pMesh = static_cast<MdlNodeTriMesh*>(pNode);
        pMesh->m_pfnGenVertices = MdlNodeTriMeshGenVertices;
        pMesh->m_pfnRemoveTemporaryArrays = MdlNodeTriMeshRemoveTemporaryArrays;
        ResetTriMeshParts(pMesh, pBase);
    }
}

// Re-points the bounding-box tree through PointerMap::CreateAABB, depth-first. The second child's
// recursion is a tail call, which becomes a loop back to the top.
void InputBinary::ResetAABBTree(AABB*& pAABB, unsigned char* pBase)
{
    ByteSwap(&pAABB->min.x);
    ByteSwap(&pAABB->min.y);
    ByteSwap(&pAABB->min.z);
    ByteSwap(&pAABB->max.x);
    ByteSwap(&pAABB->max.y);
    ByteSwap(&pAABB->max.z);
    ByteSwap(reinterpret_cast<uint64_t*>(&pAABB->pChild0));
    ByteSwap(reinterpret_cast<uint64_t*>(&pAABB->pChild1));
    ByteSwap(&pAABB->m_nIndex);
    ByteSwap(&pAABB->m_nDir);
    if (pAABB->pChild0) {
        pAABB->pChild0 = PointerMap::s_pointerMap.CreateAABB(
            static_cast<AABB32*>(ImageAddress(static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pAABB->pChild0)), pBase)));
        ResetAABBTree(pAABB->pChild0, pBase);
    }
    if (pAABB->pChild1) {
        pAABB->pChild1 = PointerMap::s_pointerMap.CreateAABB(
            static_cast<AABB32*>(ImageAddress(static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pAABB->pChild1)), pBase)));
        ResetAABBTree(pAABB->pChild1, pBase);
    }
}

// Resets the trimesh body shared by every mesh kind, covering the base parts, the header swap, the
// vertex, face and index arrays, then the index payloads, one GLRender pool per bone segment plus
// the optional second pool, and finally the in-place swap of the model's shared vertex pool rows.
// The index payloads are 16-bit unless the Mac's gHasRevComps_Mac path is on and the count exceeds
// the ATIMagic preference, in which case they are swapped as 32-bit words, half as many.
#if KOTOR1 && defined(K1_UI_XBOX)
// The swoop HUD's last gear segment (m17mg_camera's Plane14) frames the wide keycap its "abutton"
// texture is in the PC and phone art. The console build draws the pad's round A glyph there
// (aurtexman.cpp), which the keycap's rectangle cut off at both sides, so the quad's texture window
// is widened, about its centre and keeping its proportions, until its longer side spans the whole
// texture.
static void K1ConsoleFitGlyphUVs(unsigned char* pRows, unsigned int nStride, int nTexOfs, int nVertices)
{
    float fMinU = 1e9f, fMaxU = -1e9f, fMinV = 1e9f, fMaxV = -1e9f;
    for (int i = 0; i < nVertices; i++) {
        const float* pUV = reinterpret_cast<const float*>(pRows + nStride * static_cast<unsigned int>(i) + nTexOfs);
        fMinU = pUV[0] < fMinU ? pUV[0] : fMinU;
        fMaxU = pUV[0] > fMaxU ? pUV[0] : fMaxU;
        fMinV = pUV[1] < fMinV ? pUV[1] : fMinV;
        fMaxV = pUV[1] > fMaxV ? pUV[1] : fMaxV;
    }
    const float fSpan = fMaxU - fMinU > fMaxV - fMinV ? fMaxU - fMinU : fMaxV - fMinV;
    if (!(fSpan > 0.0f))
        return;
    const float fMidU = (fMinU + fMaxU) * 0.5f;
    const float fMidV = (fMinV + fMaxV) * 0.5f;
    for (int i = 0; i < nVertices; i++) {
        float* pUV = reinterpret_cast<float*>(pRows + nStride * static_cast<unsigned int>(i) + nTexOfs);
        pUV[0] = 0.5f + (pUV[0] - fMidU) / fSpan;
        pUV[1] = 0.5f + (pUV[1] - fMidV) / fSpan;
    }
}
#endif

void InputBinary::ResetTriMeshParts(MdlNodeTriMesh* pMesh, unsigned char* pBase)
{
    ResetMdlNodeParts(pMesh, pBase);
    ByteSwap(reinterpret_cast<uint64_t*>(&pMesh->m_faces.m_pData));
    ByteSwap(&pMesh->m_faces.m_count);
    ByteSwap(&pMesh->m_faces.m_capacity);
    ByteSwap(&pMesh->m_vMin.x);
    ByteSwap(&pMesh->m_vMin.y);
    ByteSwap(&pMesh->m_vMin.z);
    ByteSwap(&pMesh->m_vMax.x);
    ByteSwap(&pMesh->m_vMax.y);
    ByteSwap(&pMesh->m_vMax.z);
    ByteSwap(&pMesh->m_fRadius);
    ByteSwap(&pMesh->m_vCentroid.x);
    ByteSwap(&pMesh->m_vCentroid.y);
    ByteSwap(&pMesh->m_vCentroid.z);
    ByteSwap(&pMesh->m_vDiffuse.x);
    ByteSwap(&pMesh->m_vDiffuse.y);
    ByteSwap(&pMesh->m_vDiffuse.z);
    ByteSwap(&pMesh->m_vAmbient.x);
    ByteSwap(&pMesh->m_vAmbient.y);
    ByteSwap(&pMesh->m_vAmbient.z);
    ByteSwap(&pMesh->m_nTransparencyHint);
    ByteSwap(reinterpret_cast<uint64_t*>(&pMesh->m_lVertexIndices.m_pData));
    ByteSwap(&pMesh->m_lVertexIndices.m_count);
    ByteSwap(&pMesh->m_lVertexIndices.m_capacity);
    ByteSwap(reinterpret_cast<uint64_t*>(&pMesh->m_lLeftOverFaces.m_pData));
    ByteSwap(&pMesh->m_lLeftOverFaces.m_count);
    ByteSwap(&pMesh->m_lLeftOverFaces.m_capacity);
    ByteSwap(reinterpret_cast<uint64_t*>(&pMesh->m_lVertexIndicesCount.m_pData));
    ByteSwap(&pMesh->m_lVertexIndicesCount.m_count);
    ByteSwap(&pMesh->m_lVertexIndicesCount.m_capacity);
    ByteSwap(reinterpret_cast<uint64_t*>(&pMesh->m_lIndexPoolOffsets.m_pData));
    ByteSwap(&pMesh->m_lIndexPoolOffsets.m_count);
    ByteSwap(&pMesh->m_lIndexPoolOffsets.m_capacity);
    ByteSwap(reinterpret_cast<uint64_t*>(&pMesh->m_lIndexPools.m_pData));
    ByteSwap(&pMesh->m_lIndexPools.m_count);
    ByteSwap(&pMesh->m_lIndexPools.m_capacity);
    ByteSwap(reinterpret_cast<unsigned int*>(&pMesh->m_nLeftOverPoolOffset));
    ByteSwap(&pMesh->m_nLeftOverPool);
    ByteSwap(&pMesh->m_nLeftOverIndexCount);
    ByteSwap(reinterpret_cast<uint64_t*>(&pMesh->m_nPrimitiveType));   // raw-offset: an 8-byte swap of a model file field read as 64-bit
    ByteSwap(reinterpret_cast<uint64_t*>(&pMesh->m_pPostProcess));
    ByteSwap(&pMesh->m_nAnimateUV);
    ByteSwap(&pMesh->m_fUVDirectionX);
    ByteSwap(&pMesh->m_fUVDirectionY);
    ByteSwap(&pMesh->m_fUVJitter);
    ByteSwap(&pMesh->m_fUVJitterSpeed);
    ByteSwap(&pMesh->m_nVertexStride);
    ByteSwap(&pMesh->m_uVertexFlags);
    ByteSwap(&pMesh->m_nVertexOfs);
    ByteSwap(&pMesh->m_nNormalOfs);
    ByteSwap(&pMesh->m_nColorOfs);
    ByteSwap(&pMesh->m_nTex0Ofs);
    ByteSwap(&pMesh->m_nTex1Ofs);
    ByteSwap(&pMesh->m_nTex2Ofs);
    ByteSwap(&pMesh->m_nTex3Ofs);
    ByteSwap(&pMesh->m_nTangentOfs);
    ByteSwap(&pMesh->m_nReserved1);
    ByteSwap(&pMesh->m_nReserved2);
    ByteSwap(&pMesh->m_nReserved3);
    ByteSwap(reinterpret_cast<short*>(&pMesh->m_nVertexCount));
    ByteSwap(reinterpret_cast<short*>(&pMesh->m_nTexCoordSets));
    ByteSwap(reinterpret_cast<float*>(&pMesh->m_fSurfaceArea));
    ByteSwap(reinterpret_cast<float*>(&pMesh->m_nReserved4));
    ByteSwap(&pMesh->m_nGeomBase);
    ByteSwap(reinterpret_cast<uint64_t*>(&pMesh->m_paVertices));

    unsigned int nATIMagic = ASL::GetPrefAsUInt32("ATIMagic", 32);

    // Every read of the vertex count here is signed.
    int64_t nVertices = static_cast<short>(pMesh->m_nVertexCount);
    if (nVertices > 0) {
        void* pSource = ImageAddress(static_cast<int>(reinterpret_cast<intptr_t>(pMesh->m_paVertices)), pBase);
        uint64_t nSize = nVertices * sizeof(Vector3);
        pMesh->m_paVertices = static_cast<float*>(PointerMap::s_pointerMap.TakeBlockMemory(static_cast<int>(nSize)));
        memcpy(pMesh->m_paVertices, pSource, nSize);  // idiom-exempt: a bulk image copy
    } else {
        pMesh->m_paVertices = 0;
    }
    RelocateList<int>(pMesh->m_faces, pBase);

    // The vertex-index lists stay inside the image, and only the pointers are rebased.
    pMesh->m_lVertexIndices.m_pData = pMesh->m_lVertexIndices.m_count
        ? static_cast<unsigned int**>(ImageAddress(reinterpret_cast<uintptr_t>(pMesh->m_lVertexIndices.m_pData), pBase)) : 0;
    RelocateList<unsigned int>(pMesh->m_lVertexIndicesCount, pBase);
    for (int64_t i = 0; i < pMesh->m_lVertexIndices.m_count; i++)
        pMesh->m_lVertexIndices.m_pData[i] =
            static_cast<unsigned int*>(ImageAddress(reinterpret_cast<uintptr_t>(pMesh->m_lVertexIndices.m_pData[i]), pBase));
    RelocateList<unsigned int>(pMesh->m_lLeftOverFaces, pBase);
    RelocateList<unsigned int>(pMesh->m_lIndexPools, pBase);
    RelocateList<unsigned int>(pMesh->m_lIndexPoolOffsets, pBase);

    // One index pool per bone segment, with the segment's short count at image offset from its own
    // pointer list. An odd count is rounded up and written back so the pool holds whole 32-bit
    // words.
    for (int64_t i = 0; i < pMesh->m_lIndexPools.m_count; i++) {
        int nIndices = pMesh->m_lVertexIndicesCount.m_pData[i];
        unsigned int nOffset = pMesh->m_lIndexPoolOffsets.m_pData[i];
        if (nIndices & 1)
            pMesh->m_lVertexIndicesCount.m_pData[i] = ++nIndices;
        GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(1), nIndices * 2, 2, 0,
                              &pMesh->m_lIndexPools.m_pData[i]);
        unsigned short* pIndices = static_cast<unsigned short*>(GLRender::LockPool(
            pMesh->m_lIndexPools.m_pData[i], 0, 0, 1));
        memcpy(pIndices, ImageAddress(nOffset, pBase),
               pMesh->m_lVertexIndicesCount.m_pData[i] * sizeof(unsigned short));
        if (gHasRevComps_Mac
            && static_cast<unsigned int>(pMesh->m_lVertexIndicesCount.m_pData[i]) > nATIMagic) {
            unsigned int* pWords = reinterpret_cast<unsigned int*>(pIndices);
            for (int64_t j = 0; j < pMesh->m_lVertexIndicesCount.m_pData[i] / 2; j++, pWords++)
                ByteSwap(pWords);
        } else {
            for (int64_t j = 0; j < pMesh->m_lVertexIndicesCount.m_pData[i]; j++, pIndices++)
                ByteSwap(pIndices);
        }
        GLRender::UnlockPool(pMesh->m_lIndexPools.m_pData[i]);
        pMesh->AddToFreeTokenList(pMesh->m_lIndexPools.m_pData[i]);
    }

    // The optional second index pool. Both counts are unsigned here, unlike the vertex counts.
    if (pMesh->m_nLeftOverPool != static_cast<unsigned int>(-1)) {
        unsigned int nIndices = static_cast<unsigned int>(pMesh->m_nLeftOverIndexCount);
        unsigned int nOffset = static_cast<unsigned int>(pMesh->m_nLeftOverPoolOffset);
        if (nIndices & 1)
            pMesh->m_nLeftOverIndexCount = ++nIndices;
        GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(1), nIndices * 2, 2, 0,
                              &pMesh->m_nLeftOverPool);
        unsigned short* pIndices =
            static_cast<unsigned short*>(GLRender::LockPool(pMesh->m_nLeftOverPool, 0, 0, 1));
        memcpy(pIndices, ImageAddress(nOffset, pBase),
               static_cast<unsigned int>(pMesh->m_nLeftOverIndexCount) * sizeof(unsigned short));
        if (gHasRevComps_Mac && static_cast<unsigned int>(pMesh->m_nLeftOverIndexCount) > nATIMagic) {
            unsigned int* pWords = reinterpret_cast<unsigned int*>(pIndices);
            for (uint64_t j = 0; j < static_cast<unsigned int>(pMesh->m_nLeftOverIndexCount) / 2; j++, pWords++)
                ByteSwap(pWords);
        } else {
            for (uint64_t j = 0; j < static_cast<unsigned int>(pMesh->m_nLeftOverIndexCount); j++, pIndices++)
                ByteSwap(pIndices);
        }
        GLRender::UnlockPool(pMesh->m_nLeftOverPool);
        pMesh->AddToFreeTokenList(pMesh->m_nLeftOverPool);
    }

    // In the model's shared vertex pool (uploaded by Read out of the .mdx), swap this mesh's rows
    // in place, word by word, plus the colour word unless the Mac path owns it.
    unsigned char* pPool = static_cast<unsigned char*>(GLRender::LockPool(pMesh->m_pTree->AsModel()->m_nPool, 0, 0, 1));
    for (int i = 0; i < static_cast<short>(pMesh->m_nVertexCount); i++) {
        // The row offset is a 32-bit unsigned product narrowed to int, so the stride arithmetic is
        // unsigned.
        unsigned int* pWords = reinterpret_cast<unsigned int*>(pPool + (static_cast<int64_t>(pMesh->m_nGeomBase)
                                               + static_cast<int>(static_cast<unsigned int>(pMesh->m_nVertexStride) * i)));
        for (uint64_t j = 0; j < pMesh->m_nVertexStride / sizeof(unsigned int); j++, pWords++)
            ByteSwap(pWords);
        if (pMesh->m_nColorOfs != -1 && !gHasRevComps_Mac)
            ByteSwap(reinterpret_cast<unsigned int*>(pPool + static_cast<int>(static_cast<unsigned int>(pMesh->m_nVertexStride) * i) + pMesh->m_nGeomBase
                                     + pMesh->m_nColorOfs));
    }
#if KOTOR1 && defined(K1_UI_XBOX)
    if (pMesh->m_nTex0Ofs != -1 && strcasecmp(pMesh->m_szTexture, "abutton") == 0)
        K1ConsoleFitGlyphUVs(pPool + pMesh->m_nGeomBase, pMesh->m_nVertexStride, pMesh->m_nTex0Ofs,
                             static_cast<short>(pMesh->m_nVertexCount));
#endif
    GLRender::UnlockPool(pMesh->m_pTree->AsModel()->m_nPool);
    RyanTotalLevelVertices += static_cast<short>(pMesh->m_nVertexCount);
    RyanTotalLevelVertSize += pMesh->m_nVertexStride * static_cast<short>(pMesh->m_nVertexCount);
}

// The base-node relocation every kind runs, covering the owning tree, the parent link, the children
// (each created and then recursively reset through ResetMdlNode), the controller table (whose
// target offsets are re-based to the 64-bit Part layout through Part::GetDefaultOffset64) and the
// controller key floats.
void InputBinary::ResetMdlNodeParts(MdlNode* pNode, unsigned char* pBase)
{
    pNode->m_pTree = PointerMap::s_pointerMap.CreateMaxTreeNode(
        static_cast<MaxTree32*>(ImageAddress(static_cast<int>(reinterpret_cast<intptr_t>(pNode->m_pTree)), pBase)));
    pNode->m_pParent = pNode->m_pParent
        ? reinterpret_cast<unsigned long long>(PointerMap::s_pointerMap.CreateMdlNode(
              static_cast<MdlNode32*>(ImageAddress(static_cast<unsigned int>(pNode->m_pParent), pBase))))
        : 0;

    int nChildren = pNode->m_listChildren.m_count;
    if (nChildren == 0) {
        pNode->m_listChildren.m_pData = 0;
    } else {
        int* pChildOffsets = static_cast<int*>(ImageAddress(static_cast<unsigned int>(reinterpret_cast<uintptr_t>(pNode->m_listChildren.m_pData)), pBase));
        pNode->m_listChildren.m_count = 0;
        if (nChildren > 0) {
            pNode->m_listChildren.m_pData =
                static_cast<MdlNode**>(PointerMap::s_pointerMap.TakeBlockMemory(nChildren * sizeof(MdlNode*)));
            for (int i = 0; i < nChildren; i++) {
                pNode->m_listChildren.Add(
                    PointerMap::s_pointerMap.CreateMdlNode(static_cast<MdlNode32*>(ImageAddress(pChildOffsets[i], pBase))));
                ResetMdlNode(pNode->m_listChildren.m_pData[i], pBase);
            }
        }
    }

    RelocateList(pNode->m_listControllers, pBase);
    // The flag and the argument go through one element address, since the store after the call
    // re-indexes; NewController's fields sit in an anonymous union, so the address has to be held or
    // m_pData is reloaded after the or-assign.
    for (int64_t i = 0; i < pNode->m_listControllers.m_count; i++) {
        NewController* pController = &pNode->m_listControllers.m_pData[i];
        pController->m_target.m_nOffset |= NewController::kTargetUnconverted;
        pNode->m_listControllers.m_pData[i].m_target.m_nDefaultOffset = static_cast<short>(Part::GetDefaultOffset64(pController->m_target.m_nDefaultOffset));
    }
    RelocateList(pNode->m_listFloats, pBase);
}

// Default controller offset per node format, from the controller's own table. A negative index
// returns the index itself, and an index with no entry returns -1.
//
// The attributes on this definition are deliberate, since the body has to live outside the class
// for the local static and its map to keep one shared instance.
inline int Part::GetDefaultOffset64(int nIndex) {
    int nOffset = nIndex;
    if (nIndex < 0)
        return nOffset;
    static std::map<int, int> offsetMapPart = { { 16, 24 }, { 28, 36 } };
    if (offsetMapPart.find(nIndex) == offsetMapPart.end())
        return -1;
    return offsetMapPart[nIndex];
}

// The .mdl/.mdx image writer, the inverse of ReadSync.
//
// The output is a relocatable image of the runtime Model tree, where every pointer field is
// rewritten with the byte offset of the object it pointed at, and the objects are appended in a
// fixed order. Two tree walkers do the heavy lifting, MDLWriteCollect (the sizes) and MDLWriteNode
// (the image).

// One entry of the pointer-to-offset table.
struct MDLNodeLink {
    void*         m_pObject;   // a Model*, an Animation* or an MdlNode*
    uint64_t m_nOffset;   // where that object landed in the .mdl image
};

// The three fields share one object. The two tree walkers take it by address and read its
// m_nMdlSize as the running .mdl size, so &ctx is &m_links.
struct MDLWriteContext {
    CExoArrayList<MDLNodeLink*> m_links;
    unsigned int m_nMdlSize;
    unsigned int m_nMdxSize;

    // Owns the links, so the writer's normal exit and its landing pad both free every entry.
    ~MDLWriteContext() {
        for (int i = 0; i < m_links.m_count; i++)
            delete m_links.m_pArray[i];
    }
};

// The image's two headers. The writer blits each runtime object's own bytes into the image and then
// replaces its pointer slots with byte offsets. These two structs are that image: identical offsets
// and widths, with the pointer slots typed as 8-byte integers because they hold offsets.
struct MDLModelImage {
    uint64_t m_pfnDestructor;
    uint64_t m_pfnParseField;
    char m_szName[32];
    uint64_t m_nRootNode;       // Model::m_pMdlNode, as an offset
    char _raw38[0x40 - 0x38];
    uint64_t m_nFreeList;       // List<MdlNode*>::m_pData
    int  m_nFreeListCount;
    int  m_nFreeListCap;
    uint64_t m_nFreeTokens;     // List<unsigned int>::m_pData
    int  m_nFreeTokenCount;
    int  m_nFreeTokenCap;
    char _raw60[0x6c - 0x60];
    unsigned short m_nFlags;
    char _raw6e[2];
    int  m_nRefCount;                // zeroed in the image
    char _raw74[0x78 - 0x74];
    uint64_t m_nAnimTable;      // Model::m_animations.m_pData, as an offset
    int  m_nAnimCount;
    int  m_nAnimCap;                 // set to the count
    uint64_t m_pSuperModel;
    Vector m_vBoundsMin;
    Vector m_vBoundsMax;
    char _rawa8[0xac - 0xa8];        // float 7.0f copied verbatim
    float m_fAnimScale;
    char _rawb0[0xd0 - 0xb0];
    uint64_t m_nAnimRootNode;   // Model::m_pAnimationRoot, as an offset
    int  m_nPool;                    // zeroed in the image
    char _rawdc[0xe0 - 0xdc];        // the .mdx source byte count, copied verbatim
    char _rawe0[0xe8 - 0xe0];
    uint64_t m_nStringTable;    // Model::m_listStrings.m_pData, as an offset
    int  m_nStringCount;
    int  m_nStringCap;               // set to the count
};
#ifndef _MSC_VER   // the MSVC ABI lays Model / Animation out otherwise: no writer there
static_assert(sizeof(MDLModelImage) == sizeof(Model), "the header blit is sizeof(Model)");
#endif

struct MDLAnimationImage {
    uint64_t m_pfnDestructor;
    uint64_t m_pfnParseField;
    char m_szName[32];
    uint64_t m_nRootNode;       // Animation::m_pMdlNode, as an offset
    char _raw38[0x40 - 0x38];
    uint64_t m_nFreeList;
    int  m_nFreeListCount;
    int  m_nFreeListCap;
    uint64_t m_nFreeTokens;
    int  m_nFreeTokenCount;
    int  m_nFreeTokenCap;
    char _raw60[0x6c - 0x60];
    float m_fLength;
    char _raw70[0x74 - 0x70];        // float 0.25f copied verbatim
    char m_szAnimRoot[36];
    uint64_t m_nEventTable;     // Animation::m_events.m_pData, as an offset
    int  m_nEventCount;
    int  m_nEventCap;                // set to the count
    uint64_t m_nModel;          // Animation::m_pModel, zeroed in the image
};
#ifndef _MSC_VER
static_assert(sizeof(MDLAnimationImage) == sizeof(Animation), "the blit is sizeof(Animation)");
#endif

// The writer's two tree walkers and their helpers, all file-local. The image they fill is the
// blitted runtime objects themselves, so a slot is addressed through the object's own type and an
// offset is stored where the pointer was.
static void MDLWriteCollect(MDLWriteContext* pCtx, MdlNode* pNode);
static void MDLWriteNode(MDLWriteContext* pCtx, MdlNode* pNode, unsigned char* pOut,
                         unsigned int* pnOffset);

// An image offset, stored where a blitted object's pointer was.
struct MDLImageOffset {
    uint64_t m_nOffset;
    template <class T> operator T*() const { return reinterpret_cast<T*>(m_nOffset); }
};

// A blitted object's image, at nBase in the image buffer.
template <class T>
static inline T* MDLImage(unsigned char* pOut, unsigned int nBase)
{
    return reinterpret_cast<T*>(pOut + nBase);
}

// Appends a list's elements at *pnOffset and points the image's copy of the list at them. The
// image's capacity is set to the count, so the reader sees an exactly sized list.
template <class T>
static inline void MDLWriteList(List<T>& image, List<T>& list, unsigned char* pOut,
                                unsigned int* pnOffset)
{
    image.m_pData = MDLImageOffset{*pnOffset};
    uint64_t nSize = list.m_count * sizeof(T);
    memcpy(pOut + *pnOffset, list.m_pData, nSize);  // idiom-exempt: a bulk image copy
    image.m_capacity = list.m_count;
    *pnOffset += static_cast<unsigned int>(nSize);
}

// The image offset an already written object landed at, or 0 when it has not been written.
static inline uint64_t MDLFindOffset(MDLWriteContext* pCtx, void* pObject)
{
    for (int64_t i = 0; i < pCtx->m_links.m_count; i++)
        if (pCtx->m_links.m_pArray[i]->m_pObject == pObject)
            return pCtx->m_links.m_pArray[i]->m_nOffset;
    return 0;
}

// Adds the base node's share of the .mdl size, its child, controller and float tables, then every
// child.
static inline void MDLWriteCollectParts(MDLWriteContext* pCtx, MdlNode* pNode)
{
    pCtx->m_nMdlSize += pNode->m_listControllers.m_count * sizeof(NewController);
    pCtx->m_nMdlSize += pNode->m_listFloats.m_count * sizeof(float);
    pCtx->m_nMdlSize += pNode->m_listChildren.m_count * sizeof(MdlNode*);
    for (int64_t i = 0; i < pNode->m_listChildren.m_count; i++)
        MDLWriteCollect(pCtx, pNode->m_listChildren.m_pData[i]);
}

// Adds a tri-mesh's share, its vertex, face and index tables, the bytes of every index pool it
// owns, and the base node's share.
static void MDLWriteCollectTriMesh(MDLWriteContext* pCtx, MdlNodeTriMesh* pMesh)
{
    pCtx->m_nMdlSize += static_cast<short>(pMesh->m_nVertexCount) * sizeof(Vector3);
    pCtx->m_nMdlSize += pMesh->m_faces.m_count * sizeof(MaxFace);
    pCtx->m_nMdlSize += pMesh->m_lVertexIndices.m_count * sizeof(unsigned int*);
    pCtx->m_nMdlSize += pMesh->m_lVertexIndicesCount.m_count * sizeof(int);
    pCtx->m_nMdlSize += pMesh->m_lLeftOverFaces.m_count * sizeof(unsigned int);
    pCtx->m_nMdlSize += pMesh->m_lIndexPools.m_count * sizeof(uint64_t);
    for (int64_t i = 0; i < pMesh->m_lIndexPools.m_count; i++)
        pCtx->m_nMdlSize += GLRender::GetPoolSize(pMesh->m_lIndexPools.m_pData[i]);
    if (pMesh->m_nLeftOverPool != static_cast<unsigned int>(-1))
        pCtx->m_nMdlSize += GLRender::GetPoolSize(pMesh->m_nLeftOverPool);
    MDLWriteCollectParts(pCtx, pMesh);
}

// Walks a node tree accumulating the .mdl size of every node and of everything it points at.
// EFFECTIVE: the box-tree count keeps one fewer pointer live, so x19/x22 swap from there on.
static void MDLWriteCollect(MDLWriteContext* pCtx, MdlNode* pNode)
{
    if (pNode->m_nFlags == MdlNode::MDLNODE_AABB_ID) {
        MdlNodeAABB* pAABB = static_cast<MdlNodeAABB*>(pNode);
        pCtx->m_nMdlSize += sizeof(MdlNodeAABB);

        // Count the box tree breadth first, one level per pass.
        List<AABB*> level;
        List<AABB*> next;
        List<AABB*>* pLevel = &level;
        List<AABB*>* pNext = &next;
        pLevel->Add(pAABB->m_pAABB);
        int nBoxes = 0;
        do {
            for (int64_t i = 0; i < pLevel->m_count; i++) {
                if (pLevel->m_pData[i]->pChild0)
                    pNext->Add(pLevel->m_pData[i]->pChild0);
                if (pLevel->m_pData[i]->pChild1)
                    pNext->Add(pLevel->m_pData[i]->pChild1);
                nBoxes++;
            }
            pLevel->m_count = 0;
            List<AABB*>* pSwap = pLevel;
            pLevel = pNext;
            pNext = pSwap;
        } while (pLevel->m_count > 0);
        pCtx->m_nMdlSize += nBoxes * sizeof(AABB);

        MDLWriteCollectTriMesh(pCtx, pAABB);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_ANIM_ID) {
        MdlNodeAnimMesh* pAnim = static_cast<MdlNodeAnimMesh*>(pNode);
        // Three floats per vertex per position frame, two per texture-coordinate frame.
        pCtx->m_nMdlSize += sizeof(MdlNodeAnimMesh)
                          + (pAnim->m_nVertexFrameCount * 3 + pAnim->m_nTexCoordFrameCount * 2)
                            * static_cast<short>(pAnim->m_nVertexCount) * sizeof(float);
        MDLWriteCollectTriMesh(pCtx, pAnim);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_BASE_ID) {
        pCtx->m_nMdlSize += sizeof(MdlNode);
        MDLWriteCollectParts(pCtx, pNode);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_CAMERA_ID) {
        pCtx->m_nMdlSize += sizeof(MdlNodeCamera);
        MDLWriteCollectParts(pCtx, pNode);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_DANGLY_ID) {
        MdlNodeDanglyMesh* pDangly = static_cast<MdlNodeDanglyMesh*>(pNode);
        pCtx->m_nMdlSize += sizeof(MdlNodeDanglyMesh)
                          + pDangly->m_lConstraints.m_count * sizeof(float)
                          + static_cast<short>(pDangly->m_nVertexCount) * sizeof(Vector3);
        MDLWriteCollectTriMesh(pCtx, pDangly);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_EMITTER_ID) {
        pCtx->m_nMdlSize += sizeof(MdlNodeEmitter);
        MDLWriteCollectParts(pCtx, pNode);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_LIGHT_ID) {
        MdlNodeLight* pLight = static_cast<MdlNodeLight*>(pNode);
        int nNames = pLight->m_lFlareNames.m_count;
        pCtx->m_nMdlSize += sizeof(MdlNodeLight) + nNames * sizeof(char*);
        for (int64_t i = 0; i < nNames; i++)
            pCtx->m_nMdlSize += static_cast<unsigned int>(strlen(pLight->m_lFlareNames.m_pData[i]) + 1);
        pCtx->m_nMdlSize += pLight->m_lFlareSizes.m_count * sizeof(float)
                          + pLight->m_lFlarePositions.m_count * sizeof(float)
                          + pLight->m_lFlareColourShifts.m_count * sizeof(Vector);
        MDLWriteCollectParts(pCtx, pNode);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_REFERENCE_ID) {
        pCtx->m_nMdlSize += sizeof(MdlNodeReference);
        MDLWriteCollectParts(pCtx, pNode);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_SKIN_ID) {
        MdlNodeSkin* pSkin = static_cast<MdlNodeSkin*>(pNode);
        pCtx->m_nMdlSize += sizeof(MdlNodeSkin)
                          + pSkin->m_nBoneCount * sizeof(float)
                          + pSkin->m_lQBoneRefInv.m_count * sizeof(Quaternion)
                          + pSkin->m_lTBoneRefInv.m_count * sizeof(Vector)
                          + pSkin->m_lBoneConstantIndices.m_count * sizeof(int);
        MDLWriteCollectTriMesh(pCtx, pSkin);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_TRIGGER_ID) {
        // A trigger is written as a bare base node.
        pCtx->m_nMdlSize += sizeof(MdlNode);
        MDLWriteCollectParts(pCtx, pNode);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_LIGHTSABER_ID) {
        MdlNodeLightsaber* pSaber = static_cast<MdlNodeLightsaber*>(pNode);
        pCtx->m_nMdlSize += sizeof(MdlNodeLightsaber)
                          + static_cast<short>(pSaber->m_nVertexCount) * (2 * sizeof(Vector3) + sizeof(Vector2));
        MDLWriteCollectTriMesh(pCtx, pSaber);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_TRIMESH_ID) {
        pCtx->m_nMdlSize += sizeof(MdlNodeTriMesh);
        MDLWriteCollectTriMesh(pCtx, static_cast<MdlNodeTriMesh*>(pNode));
    }
}

// Writes a box tree depth first, each box followed by its first subtree and then its second.
static void MDLWriteAABBTree(AABB* pBox, unsigned char* pOut, unsigned int* pnOffset)
{
    memcpy(pOut + *pnOffset, pBox, sizeof(AABB));
    unsigned int nBase = *pnOffset;
    *pnOffset += sizeof(AABB);
    if (pBox->pChild0) {
        reinterpret_cast<AABB*>(&pOut[nBase])->pChild0 = MDLImageOffset{*pnOffset};
        MDLWriteAABBTree(pBox->pChild0, pOut, pnOffset);
    }
    if (pBox->pChild1) {
        reinterpret_cast<AABB*>(&pOut[nBase])->pChild1 = MDLImageOffset{*pnOffset};
        MDLWriteAABBTree(pBox->pChild1, pOut, pnOffset);
    }
}

// Writes the base node's tables after the node at nBase. The owning tree and the parent become the
// offsets they were written at, then the child table and every child, the controllers and the
// floats are written.
static void MDLWriteNodeParts(MDLWriteContext* pCtx, MdlNode* pNode, unsigned char* pOut,
                              unsigned int* pnOffset, unsigned int nBase)
{
    uint64_t nTree = MDLFindOffset(pCtx, pNode->m_pTree);
    MdlNode* pImage = reinterpret_cast<MdlNode*>(pOut + nBase);
    pImage->m_pTree = MDLImageOffset{nTree};
    if (pNode->m_pParent)
        pImage->m_pParent = MDLFindOffset(pCtx, reinterpret_cast<void*>(pNode->m_pParent));

    MDLWriteList(pImage->m_listChildren, pNode->m_listChildren, pOut, pnOffset);
    uint64_t* pChildTable = reinterpret_cast<uint64_t*>(pOut + reinterpret_cast<uint64_t>(pImage->m_listChildren.m_pData));
    for (int64_t i = 0; i < pNode->m_listChildren.m_count; i++) {
        pChildTable[i] = *pnOffset;
        MDLWriteNode(pCtx, pNode->m_listChildren.m_pData[i], pOut, pnOffset);
    }

    if (pNode->m_listControllers.m_count > 0)
        MDLWriteList(pImage->m_listControllers, pNode->m_listControllers, pOut, pnOffset);
    if (pNode->m_listFloats.m_count > 0)
        MDLWriteList(pImage->m_listFloats, pNode->m_listFloats, pOut, pnOffset);
}

// The tri-mesh tables every mesh kind shares, then the base node's. A mesh with vertices also gets
// its vertex array and the contents of its index pools; the pool payload offsets go in m_lIndexPoolOffsets,
// which is resized on the live node to one entry per pool first.
static void MDLWriteTriMeshParts(MDLWriteContext* pCtx, MdlNodeTriMesh* pMesh, unsigned char* pOut,
                                 unsigned int* pnOffset, unsigned int nBase)
{
    MdlNodeTriMesh* pImage = reinterpret_cast<MdlNodeTriMesh*>(pOut + nBase);
    if (pMesh->m_faces.m_count > 0)
        MDLWriteList(pImage->m_faces, pMesh->m_faces, pOut, pnOffset);
    if (pMesh->m_lLeftOverFaces.m_count > 0)
        MDLWriteList(pImage->m_lLeftOverFaces, pMesh->m_lLeftOverFaces, pOut, pnOffset);
    MDLWriteList(pImage->m_lVertexIndicesCount, pMesh->m_lVertexIndicesCount, pOut, pnOffset);
    pImage->m_lVertexIndices.m_pData = 0;

    if (pImage->m_nVertexCount != 0) {
        pImage->m_paVertices = MDLImageOffset{*pnOffset};
        memcpy(pOut + *pnOffset, pMesh->m_paVertices,  // idiom-exempt: a bulk image copy
               static_cast<short>(pMesh->m_nVertexCount) * sizeof(Vector3));
        *pnOffset += static_cast<short>(pMesh->m_nVertexCount) * sizeof(Vector3);

        pMesh->m_lIndexPoolOffsets.SetSize(pMesh->m_lIndexPools.m_count);
        memcpy(static_cast<void*>(&pImage->m_lIndexPoolOffsets), &pMesh->m_lIndexPoolOffsets, sizeof(pImage->m_lIndexPoolOffsets));
        MDLWriteList(pImage->m_lIndexPoolOffsets, pMesh->m_lIndexPoolOffsets, pOut, pnOffset);
        MDLWriteList(pImage->m_lIndexPools, pMesh->m_lIndexPools, pOut, pnOffset);

        // Each pool's payload offset is stored as 8 bytes into the 4-byte table, so every entry
        // spills over the next one and the last spills past the table.
        for (int i = 0; i < pMesh->m_lIndexPools.m_count; i++) {
            void* pPool = GLRender::LockPool(pMesh->m_lIndexPools.m_pData[i], 0, 0, 8);
            *static_cast<uint64_t*>(ImageAddress(reinterpret_cast<uint64_t>(&pImage->m_lIndexPoolOffsets.m_pData[i]), pOut)) = *pnOffset;
            memcpy(pOut + *pnOffset, pPool,  // idiom-exempt: a pool's byte count
                   GLRender::GetPoolSize(pMesh->m_lIndexPools.m_pData[i]));
            *pnOffset += GLRender::GetPoolSize(pMesh->m_lIndexPools.m_pData[i]);
            GLRender::UnlockPool(pMesh->m_lIndexPools.m_pData[i]);
        }

        if (pMesh->m_nLeftOverPool != static_cast<unsigned int>(-1)) {
            void* pPool = GLRender::LockPool(pMesh->m_nLeftOverPool, 0, 0, 8);
            pImage->m_nLeftOverPoolOffset = *pnOffset;
            memcpy(pOut + *pnOffset, pPool,  // idiom-exempt: a pool's byte count
                   GLRender::GetPoolSize(pMesh->m_nLeftOverPool));
            *pnOffset += GLRender::GetPoolSize(pMesh->m_nLeftOverPool);
            GLRender::UnlockPool(pMesh->m_nLeftOverPool);
        } else {
            pImage->m_nLeftOverPoolOffset = -1;
            pImage->m_nLeftOverPool = static_cast<unsigned int>(-1);
        }
    }

    MDLWriteNodeParts(pCtx, pMesh, pOut, pnOffset, nBase);
}

// Serializes a node recursively. Records the node's offset, blits it at *pnOffset, appends the
// kind's own arrays and then the shared tri-mesh and base-node tables.
// EFFECTIVE: the guarded list writes order an independent zero-extend and add the other way round.
static void MDLWriteNode(MDLWriteContext* pCtx, MdlNode* pNode, unsigned char* pOut,
                         unsigned int* pnOffset)
{
    MDLNodeLink* pLink = new MDLNodeLink;
    pLink->m_pObject = pNode;
    pLink->m_nOffset = *pnOffset;
    pCtx->m_links.Add(pLink);

    if (pNode->m_nFlags == MdlNode::MDLNODE_AABB_ID) {
        MdlNodeAABB* pAABB = static_cast<MdlNodeAABB*>(pNode);
        memcpy(pOut + *pnOffset, pNode, sizeof(MdlNodeAABB));
        unsigned int nBase = *pnOffset;
        *pnOffset += sizeof(MdlNodeAABB);
        if (pAABB->m_pAABB) {
            MDLImage<MdlNodeAABB>(pOut, nBase)->m_pAABB = MDLImageOffset{*pnOffset};
            MDLWriteAABBTree(pAABB->m_pAABB, pOut, pnOffset);
        }
        MDLWriteTriMeshParts(pCtx, pAABB, pOut, pnOffset, nBase);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_ANIM_ID) {
        MdlNodeAnimMesh* pAnim = static_cast<MdlNodeAnimMesh*>(pNode);
        memcpy(pOut + *pnOffset, pNode, sizeof(MdlNodeAnimMesh));
        unsigned int nBase = *pnOffset;
        *pnOffset += sizeof(MdlNodeAnimMesh);
        if (pAnim->m_lAnimVerts.m_count > 0)
            MDLWriteList(MDLImage<MdlNodeAnimMesh>(pOut, nBase)->m_lAnimVerts, pAnim->m_lAnimVerts, pOut, pnOffset);
        if (pAnim->m_lAnimTVerts.m_count > 0)
            MDLWriteList(MDLImage<MdlNodeAnimMesh>(pOut, nBase)->m_lAnimTVerts, pAnim->m_lAnimTVerts, pOut, pnOffset);
        if (pAnim->m_lAnimNormals.m_count > 0)
            MDLWriteList(MDLImage<MdlNodeAnimMesh>(pOut, nBase)->m_lAnimNormals, pAnim->m_lAnimNormals, pOut, pnOffset);
        if (pAnim->m_paVertexFrames) {
            MDLImage<MdlNodeAnimMesh>(pOut, nBase)->m_paVertexFrames = MDLImageOffset{*pnOffset};
            memcpy(pOut + *pnOffset, pAnim->m_paVertexFrames,  // idiom-exempt: a bulk image copy
                   sizeof(Vector3) * pAnim->m_nVertexFrameCount * static_cast<short>(pAnim->m_nVertexCount));
            *pnOffset += pAnim->m_nVertexFrameCount * static_cast<short>(pAnim->m_nVertexCount) * sizeof(Vector3);
        }
        if (pAnim->m_paTexCoordFrames) {
            MDLImage<MdlNodeAnimMesh>(pOut, nBase)->m_paTexCoordFrames = MDLImageOffset{*pnOffset};
            memcpy(pOut + *pnOffset, pAnim->m_paTexCoordFrames,  // idiom-exempt: a bulk image copy
                   sizeof(Vector2) * pAnim->m_nTexCoordFrameCount * static_cast<short>(pAnim->m_nVertexCount));
            *pnOffset += pAnim->m_nTexCoordFrameCount * static_cast<short>(pAnim->m_nVertexCount) * sizeof(Vector2);
        }
        MDLWriteTriMeshParts(pCtx, pAnim, pOut, pnOffset, nBase);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_BASE_ID) {
        memcpy(pOut + *pnOffset, pNode, sizeof(MdlNode));
        unsigned int nBase = *pnOffset;
        *pnOffset += sizeof(MdlNode);
        MDLWriteNodeParts(pCtx, pNode, pOut, pnOffset, nBase);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_CAMERA_ID) {
        memcpy(pOut + *pnOffset, pNode, sizeof(MdlNodeCamera));
        unsigned int nBase = *pnOffset;
        *pnOffset += sizeof(MdlNodeCamera);
        MDLWriteNodeParts(pCtx, pNode, pOut, pnOffset, nBase);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_DANGLY_ID) {
        MdlNodeDanglyMesh* pDangly = static_cast<MdlNodeDanglyMesh*>(pNode);
        memcpy(pOut + *pnOffset, pNode, sizeof(MdlNodeDanglyMesh));
        unsigned int nBase = *pnOffset;
        *pnOffset += sizeof(MdlNodeDanglyMesh);
        MDLWriteList(MDLImage<MdlNodeDanglyMesh>(pOut, nBase)->m_lConstraints, pDangly->m_lConstraints, pOut, pnOffset);
        MDLImage<MdlNodeDanglyMesh>(pOut, nBase)->m_pVertexSource = MDLImageOffset{*pnOffset};
        memcpy(pOut + *pnOffset, pDangly->m_pVertexSource,  // idiom-exempt: a bulk image copy
               static_cast<short>(pDangly->m_nVertexCount) * sizeof(Vector3));
        *pnOffset += static_cast<short>(pDangly->m_nVertexCount) * sizeof(Vector3);
        MDLWriteTriMeshParts(pCtx, pDangly, pOut, pnOffset, nBase);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_EMITTER_ID) {
        memcpy(pOut + *pnOffset, pNode, sizeof(MdlNodeEmitter));
        unsigned int nBase = *pnOffset;
        *pnOffset += sizeof(MdlNodeEmitter);
        MDLWriteNodeParts(pCtx, pNode, pOut, pnOffset, nBase);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_LIGHT_ID) {
        MdlNodeLight* pLight = static_cast<MdlNodeLight*>(pNode);
        memcpy(pOut + *pnOffset, pNode, sizeof(MdlNodeLight));
        unsigned int nBase = *pnOffset;
        *pnOffset += sizeof(MdlNodeLight);
        if (pLight->m_lFlareNames.m_count > 0) {
            MDLWriteList(MDLImage<MdlNodeLight>(pOut, nBase)->m_lFlareNames, pLight->m_lFlareNames, pOut, pnOffset);
            char** pNameTable = static_cast<char**>(ImageAddress(
                reinterpret_cast<uint64_t>(MDLImage<MdlNodeLight>(pOut, nBase)->m_lFlareNames.m_pData), pOut));
            for (int64_t i = 0; i < pLight->m_lFlareNames.m_count; i++) {
                int nLength = static_cast<int>(strlen(pLight->m_lFlareNames.m_pData[i])) + 1;
                pNameTable[i] = MDLImageOffset{*pnOffset};
                memcpy(pOut + *pnOffset, pLight->m_lFlareNames.m_pData[i], nLength);  // idiom-exempt: a bulk image copy
                *pnOffset += nLength;
            }
        }
        // The flare textures are runtime references; the reader resolves them by name.
        if (pLight->m_lFlareTextures.m_count > 0) {
            MDLImage<MdlNodeLight>(pOut, nBase)->m_lFlareTextures.m_pData = 0;
            MDLImage<MdlNodeLight>(pOut, nBase)->m_lFlareTextures.m_count = 0;
            MDLImage<MdlNodeLight>(pOut, nBase)->m_lFlareTextures.m_capacity = 0;
        }
        if (pLight->m_lFlareSizes.m_count > 0)
            MDLWriteList(MDLImage<MdlNodeLight>(pOut, nBase)->m_lFlareSizes, pLight->m_lFlareSizes, pOut, pnOffset);
        if (pLight->m_lFlarePositions.m_count > 0)
            MDLWriteList(MDLImage<MdlNodeLight>(pOut, nBase)->m_lFlarePositions, pLight->m_lFlarePositions, pOut, pnOffset);
        if (pLight->m_lFlareColourShifts.m_count > 0)
            MDLWriteList(MDLImage<MdlNodeLight>(pOut, nBase)->m_lFlareColourShifts, pLight->m_lFlareColourShifts, pOut, pnOffset);
        MDLWriteNodeParts(pCtx, pNode, pOut, pnOffset, nBase);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_REFERENCE_ID) {
        memcpy(pOut + *pnOffset, pNode, sizeof(MdlNodeReference));
        unsigned int nBase = *pnOffset;
        *pnOffset += sizeof(MdlNodeReference);
        MDLWriteNodeParts(pCtx, pNode, pOut, pnOffset, nBase);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_SKIN_ID) {
        MdlNodeSkin* pSkin = static_cast<MdlNodeSkin*>(pNode);
        memcpy(pOut + *pnOffset, pNode, sizeof(MdlNodeSkin));
        unsigned int nBase = *pnOffset;
        *pnOffset += sizeof(MdlNodeSkin);
        MDLImage<MdlNodeSkin>(pOut, nBase)->m_paBoneMap = MDLImageOffset{*pnOffset};
        memcpy(pOut + *pnOffset, pSkin->m_paBoneMap,  // idiom-exempt: a bulk image copy
               pSkin->m_nBoneCount * sizeof(float));
        *pnOffset += pSkin->m_nBoneCount * sizeof(float);
        MDLWriteList(MDLImage<MdlNodeSkin>(pOut, nBase)->m_lQBoneRefInv, pSkin->m_lQBoneRefInv, pOut, pnOffset);
        MDLWriteList(MDLImage<MdlNodeSkin>(pOut, nBase)->m_lTBoneRefInv, pSkin->m_lTBoneRefInv, pOut, pnOffset);
        MDLWriteList(MDLImage<MdlNodeSkin>(pOut, nBase)->m_lBoneConstantIndices, pSkin->m_lBoneConstantIndices, pOut, pnOffset);
        MDLWriteTriMeshParts(pCtx, pSkin, pOut, pnOffset, nBase);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_TRIGGER_ID) {
        memcpy(pOut + *pnOffset, pNode, sizeof(MdlNode));
        unsigned int nBase = *pnOffset;
        *pnOffset += sizeof(MdlNode);
        MDLWriteNodeParts(pCtx, pNode, pOut, pnOffset, nBase);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_LIGHTSABER_ID) {
        MdlNodeLightsaber* pSaber = static_cast<MdlNodeLightsaber*>(pNode);
        memcpy(pOut + *pnOffset, pNode, sizeof(MdlNodeLightsaber));
        unsigned int nBase = *pnOffset;
        *pnOffset += sizeof(MdlNodeLightsaber);
        MDLImage<MdlNodeLightsaber>(pOut, nBase)->m_paPositions = MDLImageOffset{*pnOffset};
        memcpy(pOut + *pnOffset, pSaber->m_paPositions,  // idiom-exempt: a bulk image copy
               static_cast<short>(pSaber->m_nVertexCount) * sizeof(Vector3));
        *pnOffset += static_cast<short>(pSaber->m_nVertexCount) * sizeof(Vector3);
        MDLImage<MdlNodeLightsaber>(pOut, nBase)->m_aNormals = MDLImageOffset{*pnOffset};
        memcpy(pOut + *pnOffset, pSaber->m_aNormals,  // idiom-exempt: a bulk image copy
               static_cast<short>(pSaber->m_nVertexCount) * sizeof(Vector3));
        *pnOffset += static_cast<short>(pSaber->m_nVertexCount) * sizeof(Vector3);
        MDLImage<MdlNodeLightsaber>(pOut, nBase)->m_aTexCoords = MDLImageOffset{*pnOffset};
        memcpy(pOut + *pnOffset, pSaber->m_aTexCoords,  // idiom-exempt: a bulk image copy
               static_cast<short>(pSaber->m_nVertexCount) * sizeof(Vector2));
        *pnOffset += static_cast<short>(pSaber->m_nVertexCount) * sizeof(Vector2);
        MDLWriteTriMeshParts(pCtx, pSaber, pOut, pnOffset, nBase);
    } else if (pNode->m_nFlags == MdlNode::MDLNODE_TRIMESH_ID) {
        memcpy(pOut + *pnOffset, pNode, sizeof(MdlNodeTriMesh));
        unsigned int nBase = *pnOffset;
        *pnOffset += sizeof(MdlNodeTriMesh);
        MDLWriteTriMeshParts(pCtx, static_cast<MdlNodeTriMesh*>(pNode), pOut, pnOffset, nBase);
    }
}

#if KOTOR1
bool IODispatcher::Write(MaxTree* pTree, SDL_RWops* pFile, SDL_RWops* pOut) {
#else
bool IODispatcher::Write(MaxTree* pTree, FILE* pFile, FILE* pOut) {
#endif
    MDLWriteContext ctx;

#ifdef _MSC_VER
    K2_UNUSED(pTree); K2_UNUSED(pFile); K2_UNUSED(pOut);
    return false;
#else
    if (pFile == 0)
        return false;

    uint64_t nZero = 0;
#if KOTOR1
    SDL_RWwrite(pFile, &nZero, 8, 1);
#else
    fwrite(&nZero, 8, 1, pFile);
#endif

    Model* pModel = pTree->AsModel();

    ctx.m_nMdlSize = 0;
    ctx.m_nMdxSize = 0;

    ctx.m_nMdxSize = ctx.m_nMdxSize + pModel->m_nPoolSize;
    ctx.m_nMdlSize = ctx.m_nMdlSize + pModel->m_animations.m_count * 8 + 248;

    for (int64_t i = 0; i < pModel->m_animations.m_count; i++) {
        Animation* pAnim = pModel->m_animations.m_pData[i];
        ctx.m_nMdlSize = ctx.m_nMdlSize + pAnim->m_events.m_count * 36 + 176;
        MDLWriteCollect(&ctx, pAnim->m_pMdlNode);
    }

    ctx.m_nMdlSize = ctx.m_nMdlSize + pModel->m_listStrings.m_count * 8;
    for (int64_t i = 0; i < pModel->m_listStrings.m_count; i++)
        ctx.m_nMdlSize = ctx.m_nMdlSize
                       + static_cast<unsigned int>(strlen(pModel->m_listStrings.m_pData[i])) + 1;

    MDLWriteCollect(&ctx, pModel->m_pMdlNode);

#if KOTOR1  // K1: the two sizes are written byte-swapped
    unsigned int nSize = ctx.m_nMdlSize;
    ByteSwap(&nSize);
    SDL_RWwrite(pFile, &nSize, 4, 1);
    nSize = ctx.m_nMdxSize;
    ByteSwap(&nSize);
    SDL_RWwrite(pFile, &nSize, 4, 1);
#else
    fwrite(&ctx.m_nMdlSize, 4, 1, pFile);
    fwrite(&ctx.m_nMdxSize, 4, 1, pFile);
#endif

    unsigned char* pMdlBuf = new unsigned char[ctx.m_nMdlSize];
    memset(pMdlBuf, 0, ctx.m_nMdlSize);  // idiom-exempt: the image is a raw byte buffer

    unsigned char* pMdxBuf = 0;
    if (ctx.m_nMdxSize != 0)
        pMdxBuf = new unsigned char[ctx.m_nMdxSize];

    unsigned int nOffset = 0;

    MDLNodeLink* pModelLink = new MDLNodeLink;
    pModelLink->m_pObject = pModel;
    pModelLink->m_nOffset = 0;
    ctx.m_links.Add(pModelLink);

    memcpy(pMdlBuf, pModel, sizeof(Model));
    nOffset = sizeof(Model);

    MDLModelImage* pImage = reinterpret_cast<MDLModelImage*>(pMdlBuf);

    unsigned char* pPool = static_cast<unsigned char*>(GLRender::LockPool(pModel->m_nPool, 0, 0, 8));
    pImage->m_nPool = 0;
    memcpy(pMdxBuf, pPool, static_cast<unsigned int>(pModel->m_nPoolSize));  // idiom-exempt: a runtime byte count
    GLRender::UnlockPool(pModel->m_nPool);

    pImage->m_nStringTable = nOffset;
    memcpy(pMdlBuf + nOffset, pModel->m_listStrings.m_pData,
           static_cast<uint64_t>(pModel->m_listStrings.m_count) * 8);
    nOffset = nOffset + static_cast<unsigned int>(pModel->m_listStrings.m_count) * 8;
    pImage->m_nStringCap = pModel->m_listStrings.m_count;
    for (int64_t i = 0; i < pModel->m_listStrings.m_count; i++) {
        const char* pszName = pModel->m_listStrings.m_pData[i];
        reinterpret_cast<uint64_t*>(pMdlBuf + pImage->m_nStringTable)[i] = nOffset;
        strcpy(reinterpret_cast<char*>(pMdlBuf + nOffset), pszName);
        nOffset = nOffset + static_cast<unsigned int>(strlen(pszName)) + 1;
    }

    pImage->m_nAnimTable = nOffset;
    memcpy(pMdlBuf + nOffset, pModel->m_animations.m_pData,
           static_cast<uint64_t>(pModel->m_animations.m_count) * 8);
    pImage->m_nAnimCap = pModel->m_animations.m_count;
    nOffset = nOffset + static_cast<unsigned int>(pModel->m_animations.m_count) * 8;

    for (int64_t i = 0; i < pModel->m_animations.m_count; i++) {
        unsigned int nObjOffset = nOffset;
        MDLAnimationImage* pAnimImage = reinterpret_cast<MDLAnimationImage*>(pMdlBuf + nObjOffset);
        reinterpret_cast<uint64_t*>(pMdlBuf + pImage->m_nAnimTable)[i] = nObjOffset;
        Animation* pAnim = pModel->m_animations.m_pData[i];

        MDLNodeLink* pLink = new MDLNodeLink;
        pLink->m_pObject = pAnim;
        pLink->m_nOffset = nObjOffset;
        ctx.m_links.Add(pLink);

        memcpy(pAnimImage, pAnim, sizeof(Animation));
        nOffset = nOffset + sizeof(Animation);

        uint64_t nEventSize = sizeof(AnimationEvent) * static_cast<uint64_t>(pAnim->m_events.m_count);
        if (pAnim->m_events.m_count != 0) {
            pAnimImage->m_nEventTable = nOffset;
            memcpy(pMdlBuf + nOffset, pAnim->m_events.m_pData,  // idiom-exempt: the size is a count
                   nEventSize);
            nOffset = nOffset + static_cast<unsigned int>(nEventSize);
            pAnimImage->m_nEventCap = pAnim->m_events.m_count;
        }

        if (pAnim->m_pMdlNode != 0) {
            pAnimImage->m_nRootNode = nOffset;
            MDLWriteNode(&ctx, pAnim->m_pMdlNode, pMdlBuf, &nOffset);
        }

        pAnimImage->m_nModel = 0;
    }

    pImage->m_nRefCount = 0;

    if (pModel->m_pMdlNode != 0) {
        pImage->m_nRootNode = nOffset;
        MDLWriteNode(&ctx, pModel->m_pMdlNode, pMdlBuf, &nOffset);
    }

    if (pModel->m_pAnimationRoot != 0) {
        uint64_t nFound = 0;
        for (int64_t i = 0; i < ctx.m_links.m_count; i++) {
            if (ctx.m_links.m_pArray[i]->m_pObject == pModel->m_pAnimationRoot) {
                nFound = ctx.m_links.m_pArray[i]->m_nOffset;
                break;
            }
        }
        pImage->m_nAnimRootNode = nFound;
    }

#if KOTOR1  // K1: both images are byte-swapped word by word before the write
    for (unsigned int i = 0; i < ctx.m_nMdlSize; i += 4)
        ByteSwap(reinterpret_cast<unsigned int*>(pMdlBuf + i));
    for (unsigned int i = 0; i < ctx.m_nMdxSize; i += 4)
        ByteSwap(reinterpret_cast<unsigned int*>(pMdxBuf + i));
    SDL_RWwrite(pFile, pMdlBuf, 1, ctx.m_nMdlSize);
    SDL_RWwrite(pOut, pMdxBuf, 1, ctx.m_nMdxSize);
#else
    fwrite(pMdlBuf, 1, static_cast<uint64_t>(ctx.m_nMdlSize), pFile);
    fwrite(pMdxBuf, 1, static_cast<uint64_t>(ctx.m_nMdxSize), pOut);
#endif

    delete[] pMdxBuf;
    delete[] pMdlBuf;

    return true;
#endif
}
