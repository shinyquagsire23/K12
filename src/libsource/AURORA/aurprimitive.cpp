// VertexPrimitive hierarchy, the abstract base and the four concrete vertex-array flavours (flat,
// animated, dangly and skinned), plus the NodeVertex record family.

#if KOTOR1
#define K1_AURORA_LIST
#endif
#include "aurprimitive.h"
#include "port/aslglapi.h"
#include "AURORA/aurmodel.h"

int usevertexprograms = 1;

#ifndef _MSC_VER  // MSVC: aurmodel.cpp's definition only (an inline copy beside it is LNK2005)
inline MdlNodeTriMesh* MdlNode::AsMdlNodeTriMesh()
{
    if ((m_nFlags & NODE_TRIMESH) != NODE_TRIMESH)
        return 0;
    return static_cast<MdlNodeTriMesh*>(this);
}
#endif
#include "aurmaterial.h"
#include "aurglcore.h"
#include "Vector.h"

// The ASL GL client-array state table.

extern unsigned int clientStateTranslationTable[];

#include "aurlightmanager.h"

// NodeVertex::m_nTexCoordFlags: which texture-coordinate sets the vertex carries.
enum {
    kNodeVertexTex0 = 1,
    kNodeVertexTex1 = 2,
    kNodeVertexTex2 = 4,
    kNodeVertexTex3 = 8,
};

extern int usearbvertexprograms;

int AurMultiTextureAvailable();
int AurVertexProgramsAvailable();
int AurBumpMapAvailable();

// =============================================================================================
//  VertexPrimitive - the abstract base
// =============================================================================================

VertexPrimitive::VertexPrimitive(PartTriMesh* part)
{
    m_pPart = part;
    m_nArrayCount = 0;
    m_bStaticArrays = 1;
    m_bRebuildPending = 0;
}

VertexPrimitive::~VertexPrimitive()
{
}

// The deleting destructor is emitted as a trap: the class is abstract, so a pointer to this base
// can never be deleted through it.

VertexPrimitiveSkinned* VertexPrimitive::AsVertexPrimitiveSkinned()
{
    return 0;
}

VertexPrimitiveAnim* VertexPrimitive::AsVertexPrimitiveAnim()
{
    return 0;
}

VertexPrimitiveDangly* VertexPrimitive::AsVertexPrimitiveDangly()
{
    return 0;
}

// Raises the rebuild-pending flag; SetupVertexArraysStandard lowers it again.
void VertexPrimitive::Rebuild()
{
    m_bRebuildPending = 1;
}

// Re-runs the subtype's array setup and clears the rebuild-pending flag. The colour client array is
// toggled first, on whether another gob is already lit.
//
// The two disable calls stay separate branches rather than one `if (a && !b)`: folding them
// changes the emitted control flow.
void VertexPrimitive::SetupVertexArraysStandard()
{
    PartTriMesh* pTriMesh = m_pPart->AsPartTriMesh();

    if (pTriMesh->m_bHasColorPool) {
        if (LightManager::m_pOnlyGob)
            ASLgl::glDisableClientState(clientStateTranslationTable[2]);
        else
            ASLgl::glEnableClientState(clientStateTranslationTable[2]);
    } else {
        ASLgl::glDisableClientState(clientStateTranslationTable[2]);
    }

    SetupArrays();
    m_bRebuildPending = 0;
}

// =============================================================================================
//  VertexPrimitiveFlat
// =============================================================================================

// BuildVertexArrays is deliberately called unqualified: the base constructor dispatches the
// derived override, which for the flat case is the empty one.
VertexPrimitiveFlat::VertexPrimitiveFlat(PartTriMesh* part)
    : VertexPrimitive(part)
{
    m_pNode = part->m_pNode;

    MdlNodeTriMesh* node = static_cast<MdlNodeTriMesh*>(m_pNode);
    if (static_cast<short>(node->m_nVertexCount) == 0) {
        m_nArrayCount = 1;
        m_nPoolHandle = -1;
        return;
    }

    if (node->m_nAnimateUV) {
        GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(0),
                              static_cast<short>(node->m_nVertexCount) * 8, 2, 0,
                              reinterpret_cast<unsigned int*>(&part->m_nUVPool));
        m_nArrayCount += 2;
    }

    BuildVertexArrays();

    m_nArrayCount += 2 * static_cast<short>(static_cast<MdlNodeTriMesh*>(m_pNode)->m_nTexCoordSets) + 6;

    if (part->m_pMaterial->HasBumpMap() &&
        static_cast<MdlNodeTriMesh*>(m_pNode)->HasTangentBases() &&
        AurBumpMapAvailable()) {
        m_nPoolHandle = GLRender::GetFogArraysInfoRef();
        m_nArrayCount += 9;
    } else {
        m_nPoolHandle = -1;
    }
}

// Releases the fog-arrays pool if one was acquired.
VertexPrimitiveFlat::~VertexPrimitiveFlat()
{
    if (m_nPoolHandle != -1) {
        GLRender::DeletePool(m_nPoolHandle);
        m_nPoolHandle = -1;
    }
}

VertexPrimitiveFlat* VertexPrimitiveFlat::AsVertexPrimitiveFlat()
{
    return this;
}

unsigned char* VertexPrimitiveFlat::SetupArraysVAR(unsigned char*)
{
    return 0;
}

void VertexPrimitiveFlat::UpdateLightVectors(Vector const&)
{
}

void VertexPrimitiveFlat::UpdateHalfAngleVectors(Vector const&, Vector const&)
{
}

void VertexPrimitiveFlat::UpdateFogValues(Vector const&, float)
{
}

// Copies two colour words per vertex out of the interleaved rows into the part's colour pool.
// The node offsets are read into locals before the loop because the pool stores may alias the node.
void VertexPrimitiveFlat::BuildVertexArrays()
{
    MdlNodeTriMesh* node = static_cast<MdlNodeTriMesh*>(m_pNode);
    if (node->m_nAnimateUV == 0)
        return;

    PartTriMesh* pTriMesh = m_pPart->AsPartTriMesh();
    unsigned int* paColors = static_cast<unsigned int*>(GLRender::LockPool(pTriMesh->m_nUVPool, 0, 0, 1));
    char* paRows = static_cast<char*>(GLRender::LockPool(m_pNode->m_pTree->AsModel()->m_nPool, 0, 0, 8));

    MdlNodeTriMesh* pNode = static_cast<MdlNodeTriMesh*>(m_pNode);
    char* paSrc = paRows + pNode->m_nTex0Ofs + pNode->m_nGeomBase;
    int nStride = pNode->m_nVertexStride;
    int nCount = static_cast<short>(pNode->m_nVertexCount);

    int nOffset = 0;
    for (int i = 0; i < nCount; i++) {
        unsigned int* pSrc = reinterpret_cast<unsigned int*>(paSrc + nOffset);
        paColors[0] = pSrc[0];
        paColors[1] = pSrc[1];
        paColors += 2;
        nOffset += nStride;
    }

    GLRender::UnlockPool(pTriMesh->m_nUVPool);
    GLRender::UnlockPool(m_pNode->m_pTree->AsModel()->m_nPool);
}

// Points the fixed-function buffers at this primitive's arrays.
void VertexPrimitiveFlat::SetupArrays()
{
    // The result is unused, but the call is kept.
    VertexPrimitiveAnim* pAnim = AsVertexPrimitiveAnim();
    VertexPrimitiveDangly* pDangly = AsVertexPrimitiveDangly();
    (void)pDangly;

    PartTriMesh* pTriMesh = m_pPart->AsPartTriMesh();
    MdlNodeTriMesh* node = static_cast<MdlNodeTriMesh*>(m_pNode);

    if (pTriMesh->m_bHasColorPool) {
        GLRender::SetColorBuffer(4, 0, pTriMesh->m_nColorPool, static_cast<short>(node->m_nVertexCount) + 1);
    }

    GLRender::SetInterleavedBuffer(static_cast<MdlNodeTriMesh*>(m_pNode)->m_nVertexStride,
                                   static_cast<MdlNodeTriMesh*>(m_pNode)->m_uVertexFlags,
                                   m_pNode->m_pTree->AsModel()->m_nPool,
                                   static_cast<short>(node->m_nVertexCount) + 1,
                                   node, node->m_nGeomBase);

    if (pAnim) {
        GLRender::SetTexCoordBuffer(8, 0, pAnim->m_nTexCoordPool, static_cast<short>(node->m_nVertexCount));
        GLRender::SetVertexBuffer(12, 0, pAnim->m_nVertexPool,
                                  static_cast<short>(static_cast<MdlNodeTriMesh*>(m_pNode)->m_nVertexCount));
    }
}

// Re-points every client array into a caller-supplied vertex-array-range block and returns the
// pointer advanced past the rows this primitive consumed.
unsigned char* VertexPrimitiveFlat::ReconnectArraysVAR(unsigned char* paData)
{
    int nCount = static_cast<short>(static_cast<MdlNodeTriMesh*>(m_pNode)->m_nVertexCount);

    PartTriMesh* pTriMesh = m_pPart->AsPartTriMesh();
    MdlNodeTriMesh* node = pTriMesh->m_pNode->AsMdlNodeTriMesh();

    if (pTriMesh->m_bHasColorPool) {
        ASLgl::glColorPointer(4, GL_UNSIGNED_BYTE, 0, paData);
        paData = reinterpret_cast<unsigned char*>(reinterpret_cast<unsigned int*>(paData) + nCount);
        if (LightManager::m_pOnlyGob)
            ASLgl::glDisableClientState(clientStateTranslationTable[2]);
        else
            ASLgl::glEnableClientState(clientStateTranslationTable[2]);
    } else {
        ASLgl::glDisableClientState(clientStateTranslationTable[2]);
    }

    // Fall-through switch: every stage also runs the stages below it.
    switch (static_cast<short>(static_cast<MdlNodeTriMesh*>(m_pNode)->m_nTexCoordSets)) {
    case 4:
        if (AurMultiTextureAvailable())
            ASLgl::glClientActiveTextureARB(GL_TEXTURE3);
        GLRender::cm_uiCurrentStage = 3;
        ASLgl::glTexCoordPointer(2, GL_FLOAT, node->m_nVertexStride, paData + node->m_nTex3Ofs);
    case 3:
        if (AurMultiTextureAvailable())
            ASLgl::glClientActiveTextureARB(GL_TEXTURE2);
        GLRender::cm_uiCurrentStage = 2;
        ASLgl::glTexCoordPointer(2, GL_FLOAT, node->m_nVertexStride, paData + node->m_nTex2Ofs);
    case 2:
        if (AurMultiTextureAvailable())
            ASLgl::glClientActiveTextureARB(GL_TEXTURE1);
        GLRender::cm_uiCurrentStage = 1;
        ASLgl::glTexCoordPointer(2, GL_FLOAT, node->m_nVertexStride, paData + node->m_nTex1Ofs);
    case 1:
        if (AurMultiTextureAvailable())
            ASLgl::glClientActiveTextureARB(GL_TEXTURE0);
        GLRender::cm_uiCurrentStage = 0;
        ASLgl::glTexCoordPointer(2, GL_FLOAT, node->m_nVertexStride, paData + node->m_nTex0Ofs);
    }

    if (static_cast<short>(static_cast<MdlNodeTriMesh*>(m_pNode)->m_nTexCoordSets) > 0)
        ASLgl::glEnableClientState(clientStateTranslationTable[0]);
    else
        ASLgl::glDisableClientState(clientStateTranslationTable[0]);

    ASLgl::glNormalPointer(GL_FLOAT, node->m_nVertexStride, paData + node->m_nNormalOfs);
    ASLgl::glVertexPointer(3, GL_FLOAT, node->m_nVertexStride, paData + node->m_nVertexOfs);

    if (pTriMesh->m_pMaterial->HasBumpMap() &&
        static_cast<MdlNodeTriMesh*>(m_pNode)->HasTangentBases() &&
        AurBumpMapAvailable()) {
        if (usearbvertexprograms)
            ASLgl::glVertexAttribPointerARB(12, 3, GL_FLOAT, 0, node->m_nVertexStride,
                                            paData + node->m_nTangentOfs);
        if (usearbvertexprograms)
            ASLgl::glVertexAttribPointerARB(13, 3, GL_FLOAT, 0, node->m_nVertexStride,
                                            paData + node->m_nTangentOfs + 12);
        if (usearbvertexprograms)
            ASLgl::glVertexAttribPointerARB(14, 3, GL_FLOAT, 0, node->m_nVertexStride,
                                            paData + node->m_nTangentOfs + 24);
        if (AurMultiTextureAvailable())
            ASLgl::glClientActiveTextureARB(GL_TEXTURE0);
        GLRender::cm_uiCurrentStage = 0;
    }

    return paData + node->m_nVertexStride * nCount;
}

// =============================================================================================
//  VertexPrimitiveAnim
// =============================================================================================

// One pool for positions (three floats per vertex) and one for texture coordinates (two).
VertexPrimitiveAnim::VertexPrimitiveAnim(PartTriMesh* part)
    : VertexPrimitiveFlat(part)
{
    GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(0),
                          static_cast<short>(static_cast<MdlNodeTriMesh*>(m_pNode)->m_nVertexCount) * 3 * 4, 2, 0,
                          reinterpret_cast<unsigned int*>(&m_nVertexPool));
    GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(0),
                          static_cast<short>(static_cast<MdlNodeTriMesh*>(m_pNode)->m_nVertexCount) * 8, 2, 0,
                          reinterpret_cast<unsigned int*>(&m_nTexCoordPool));
    BuildVertexArrays();
    m_bStaticArrays = 0;
}

// Both pools are released unguarded, unlike the flat fog-pool handle the base destructor releases.
VertexPrimitiveAnim::~VertexPrimitiveAnim()
{
    GLRender::DeletePool(m_nVertexPool);
    m_nVertexPool = -1;
    GLRender::DeletePool(m_nTexCoordPool);
    m_nTexCoordPool = -1;
}

VertexPrimitiveAnim* VertexPrimitiveAnim::AsVertexPrimitiveAnim()
{
    return this;
}

// The animated mesh's arrays are filled by PartAnimMesh's own update, not from the node's static
// colour rows.
void VertexPrimitiveAnim::BuildVertexArrays()
{
}

unsigned char* VertexPrimitiveAnim::SetupArraysVAR(unsigned char*)
{
    return 0;
}

// =============================================================================================
//  VertexPrimitiveDangly
// =============================================================================================

// A heap Vector array, one per vertex, value-initialised.
VertexPrimitiveDangly::VertexPrimitiveDangly(PartTriMesh* part)
    : VertexPrimitiveFlat(part)
{
    m_paVertices = new Vector[static_cast<short>(static_cast<MdlNodeTriMesh*>(m_pNode)->m_nVertexCount)]();
    m_bStaticArrays = 0;
}

VertexPrimitiveDangly::~VertexPrimitiveDangly()
{
    delete[] m_paVertices;
}

VertexPrimitiveDangly* VertexPrimitiveDangly::AsVertexPrimitiveDangly()
{
    return this;
}

// The dangly simulation writes m_paVertices directly.
void VertexPrimitiveDangly::BuildVertexArrays()
{
}

unsigned char* VertexPrimitiveDangly::SetupArraysVAR(unsigned char*)
{
    return 0;
}

// =============================================================================================
//  VertexPrimitiveSkinned
// =============================================================================================

// The skinned flavour registers six extra per-vertex rows on top of the flat layout.
VertexPrimitiveSkinned::VertexPrimitiveSkinned(PartSkin* part)
    : VertexPrimitiveFlat(static_cast<PartTriMesh*>(part))
{
    m_nArrayCount += 6;
}

// A skinned primitive adds no members, so this is the flat destructor body, and it has to stay in
// its header for that to hold.
VertexPrimitiveSkinned::~VertexPrimitiveSkinned()
{
}

// The skinned geometry is built by the render family; the slot exists so the flat constructor's
// virtual dispatch has a target.
void VertexPrimitiveSkinned::BuildVertexArrays()
{
}

// Wraps the flat re-link, then links the two skinned attributes when vertex programs are available.
// The vertex-program global is re-read between the two links, so the blocks stay separate.
unsigned char* VertexPrimitiveSkinned::ReconnectArraysVAR(unsigned char* paData)
{
    unsigned char* p = VertexPrimitiveFlat::ReconnectArraysVAR(paData);

    if (!AurVertexProgramsAvailable())
        return p;

    MdlNodeTriMesh* node = static_cast<MdlNodeTriMesh*>(m_pNode);
    int nCount = static_cast<short>(node->m_nVertexCount);

    if (usearbvertexprograms) {
        ASLgl::glVertexAttribPointerARB(1, 4, GL_FLOAT, 0, 0, p);
    }
    p += nCount * 16;
    if (usearbvertexprograms) {
        ASLgl::glVertexAttribPointerARB(4, 4, GL_SHORT, 0, 0, p);
    }
    p = reinterpret_cast<unsigned char*>(reinterpret_cast<unsigned int*>(p) + nCount);
    return p;
}

// NodeVertex -- the per-vertex record family. The records are not a virtual hierarchy: the kind is
// a byte, and every record zeroes its whole member span in one operation rather than per member.

// Running total of the vertex bytes allocated so far; every constructor adds 8.
int vertexsize = 0;

const unsigned char NodeVertex::NODEVERTEX_ANIM_ID;
const unsigned char NodeVertex::NODEVERTEX_DANGLY_ID;
const unsigned char NodeVertex::NODEVERTEX_SKINNED_ID;

NodeVertexAnim* NodeVertex::AsNodeVertexAnim()
{
    return m_nType == NODEVERTEX_ANIM_ID ? static_cast<NodeVertexAnim*>(this) : 0;
}

NodeVertexDangly* NodeVertex::AsNodeVertexDangly()
{
    return m_nType == NODEVERTEX_DANGLY_ID ? static_cast<NodeVertexDangly*>(this) : 0;
}

// Positions and the seventh vector are always supplied; the four texture coordinates are copied
// only when given, each setting its own flag bit. The white colour is a statement here rather than
// a member default, so the default constructor below leaves the colour zeroed.
NodeVertex::NodeVertex(Vector* pPos, Vector* pTex0, Vector* pTex1, Vector* pTex2, Vector* pTex3,
                       Vector* pNormal, Vector* pColor)
{
    __builtin_memset(this, 0,
                     sizeof(m_vPos) * 7 + sizeof(m_nTexCoordFlags) + sizeof(m_nType));
    m_vColor = Vector(1.0f, 1.0f, 1.0f);
    m_vPos = *pPos;
    m_vNormal = *pNormal;
    if (pTex0 != 0) {
        m_vTex0 = *pTex0;
        m_nTexCoordFlags |= kNodeVertexTex0;
    }
    if (pTex1 != 0) {
        m_vTex1 = *pTex1;
        m_nTexCoordFlags |= kNodeVertexTex1;
    }
    if (pTex2 != 0) {
        m_vTex2 = *pTex2;
        m_nTexCoordFlags |= kNodeVertexTex2;
    }
    if (pTex3 != 0) {
        m_vTex3 = *pTex3;
        m_nTexCoordFlags |= kNodeVertexTex3;
    }
    if (pColor != 0)
        m_vColor = *pColor;
    vertexsize += 8;
}

// The flag byte is deliberately left alone: it is the one field neither default constructor touches.
NodeVertex::NodeVertex()
{
    m_nType = 0;
    __builtin_memset(&m_vPos, 0, sizeof(m_vPos) * 7);
}

// The three lists are default-constructed and then assigned, so they stay out of the
// member-initializer list.
NodeVertexAnim::NodeVertexAnim(Vector* pPos, Vector* pTex0, Vector* pTex1, Vector* pTex2,
                               Vector* pTex3, Vector* pNormal, Vector* pColor,
                               List<Vector>& l0, List<Vector>& l1, List<Vector>& l2)
    : NodeVertex(pPos, pTex0, pTex1, pTex2, pTex3, pNormal, pColor)
{
    m_nType = NODEVERTEX_ANIM_ID;
    m_lVertexFrames = l0;
    m_lTexCoordFrames = l1;
    m_lNormalFrames = l2;
}

NodeVertexAnim::NodeVertexAnim()
{
    m_nType = NODEVERTEX_ANIM_ID;
}

NodeVertexDangly::NodeVertexDangly(Vector* pPos, Vector* pTex0, Vector* pTex1, Vector* pTex2,
                                   Vector* pTex3, Vector* pNormal, Vector* pColor,
                                   float fConstraint)
    : NodeVertex(pPos, pTex0, pTex1, pTex2, pTex3, pNormal, pColor)
{
    m_nType = NODEVERTEX_DANGLY_ID;
    m_fConstraint = fConstraint;
}

NodeVertexDangly::NodeVertexDangly()
{
    m_nType = NODEVERTEX_DANGLY_ID;
    m_fConstraint = 0.0f;
}

// Seven componentwise comparisons over the base vectors, with the other vertex passed first.
// Written as an `if` chain returning true, not as a returned conjunction: the returned form folds
// the tail into a select and loses the final branch.
bool NodeVertex::operator==(const NodeVertex& rhs)
{
    if (equal(rhs.m_vPos, m_vPos)
        && equal(rhs.m_vTex0, m_vTex0)
        && equal(rhs.m_vTex1, m_vTex1)
        && equal(rhs.m_vTex2, m_vTex2)
        && equal(rhs.m_vTex3, m_vTex3)
        && equal(rhs.m_vNormal, m_vNormal)
        && equal(rhs.m_vColor, m_vColor))
        return true;
    return false;
}

// The same chain, repeated rather than factored out, and without the three list members this class
// adds.
bool NodeVertexAnim::operator==(const NodeVertexAnim& rhs)
{
    if (equal(rhs.m_vPos, m_vPos)
        && equal(rhs.m_vTex0, m_vTex0)
        && equal(rhs.m_vTex1, m_vTex1)
        && equal(rhs.m_vTex2, m_vTex2)
        && equal(rhs.m_vTex3, m_vTex3)
        && equal(rhs.m_vNormal, m_vNormal)
        && equal(rhs.m_vColor, m_vColor))
        return true;
    return false;
}

// The base chain plus the dangling scalar, compared with fcmp rather than equal(), and again with
// the other vertex first.
bool NodeVertexDangly::operator==(const NodeVertexDangly& rhs)
{
    if (equal(rhs.m_vPos, m_vPos)
        && equal(rhs.m_vTex0, m_vTex0)
        && equal(rhs.m_vTex1, m_vTex1)
        && equal(rhs.m_vTex2, m_vTex2)
        && equal(rhs.m_vTex3, m_vTex3)
        && equal(rhs.m_vNormal, m_vNormal)
        && equal(rhs.m_vColor, m_vColor)
        && rhs.m_fConstraint == m_fConstraint)
        return true;
    return false;
}

// Empties the three lists explicitly; without the SetSize(0) calls the trailing member destructors
// free the buffers a second time and the body is far too short.
NodeVertexAnim::~NodeVertexAnim()
{
    m_lVertexFrames.SetSize(0);
    m_lTexCoordFrames.SetSize(0);
    m_lNormalFrames.SetSize(0);
}
