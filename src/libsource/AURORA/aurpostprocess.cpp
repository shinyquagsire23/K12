
#include "k2stdint.h"
#include "aurres.h"
#include "aurpostprocess.h"
#include "nullname.h"
#include "AURORA/aurmodel.h"
#include "aurglcore.h"
#include "aurtexman.h"
#include "aurprimitive.h"
#include "auremitter.h"
#include "Matrix.h"
#include "aurmaterial.h"
#include "aurparse.h"
#if KOTOR1
#include "alignedptr.h"
#endif

// NodeVertex::m_nTexCoordFlags: which texture-coordinate sets the vertex carries.
enum {
    kNodeVertexTex0 = 0x01,
    kNodeVertexTex1 = 0x02,
    kNodeVertexTex2 = 0x04,
    kNodeVertexTex3 = 0x08,
};

int minstripsize = 5;

extern int countnode;
extern int countmodelnodes;

#define MAXTREE_OUT_OF_LINE

extern int animationfootprint;

int GetMaxID(MdlNode* pNode, int nID);
void SynchronizeNodes(MdlNode* pDst, MdlNode* pSrc, int& rnMaxID);
void UpdateAnimFootprint(MdlNode* pNode, int nDelta);

CAurTexture* AurTextureGetReference(char* sName, char* sExtension);

extern int quaternionidentityoptimization;

// Rotates a vector by a quaternion, shared with the other engine TUs.
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

#if !KOTOR1
// Inverse rotation: brings a world-frame vector back into the mesh's local frame.
static inline Vector RotateByTranspose(const Vector& v, const Quaternion& q)
{
    float w = q.w;
    if (quaternionidentityoptimization == 0 || (w != -1.0f && w != 1.0f)) {
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
}
#endif

#include "port/libcbase.h"
#include "port/libcstring.h"

#include "port/crtapi.h"

int GetMaxID(MdlNode* pNode, int nMax)
{
    if (pNode->m_nNodeID > nMax)
        nMax = pNode->m_nNodeID;

    for (int64_t i = 0; i < pNode->m_listChildren.m_count; i++)
        nMax = GetMaxID(pNode->m_listChildren.m_pData[i], nMax);

    return nMax;
}

// Re-numbers this subtree to match pOther: a child whose name matches takes the matching
// child's id, everything else takes the next id off the shared counter.
void SynchronizeNodes(MdlNode* pNode, MdlNode* pOther, int& nIndex)
{
    pNode->m_nNodeID = pOther->m_nNodeID;

    if (pNode->m_listChildren.m_count < 1)
        return;

    for (int64_t i = 0; i < pNode->m_listChildren.m_count; i++) {
        MdlNode* pChild = pNode->m_listChildren.m_pData[i];

        for (int64_t j = 0; j < pOther->m_listChildren.m_count; j++) {
            MdlNode* pOtherChild = pOther->m_listChildren.m_pData[j];
            if (!_stricmp(pChild->TranslateNodeName(), pOtherChild->TranslateNodeName())) {
                SynchronizeNodes(pChild, pOtherChild, nIndex);
                goto next;
            }
        }

        pChild->m_nNodeID = static_cast<short>(nIndex);
        nIndex++;
    next:;
    }
}

// qsort comparator: nodes that are not tri-meshes sort last, then the zero transparency
// hint, then by signed hint.
int SiblingOrderCmp(void const* pA, void const* pB)
{
    MdlNodeTriMesh* p1 = (*static_cast<MdlNode* const*>(pA))->AsMdlNodeTriMesh();
    MdlNodeTriMesh* p2 = (*static_cast<MdlNode* const*>(pB))->AsMdlNodeTriMesh();

    int nResult = p1 != 0 ? 1 : (p2 != 0 ? -1 : 0);
    if (p1 == 0 || p2 == 0)
        return nResult;

    int n1 = p1->m_nTransparencyHint;
    int n2 = p2->m_nTransparencyHint;
    if (n1 == n2)
        return 0;
    if (n1 == 0)
        return 1;
    if (n2 == 0)
        return -1;
    return n1 <= n2 ? -1 : 1;
}

// Sorts the child list when any tri-mesh child asks for a sibling order, then recurses.
void MdlNode::ReorderChildren()
{
    for (int64_t i = 0; i < m_listChildren.m_count; i++) {
        if (m_listChildren.m_pData[i]->AsMdlNodeTriMesh() == 0)
            continue;
        if (m_listChildren.m_pData[i]->AsMdlNodeTriMesh()->m_nTransparencyHint > 0) {
            qsort(m_listChildren.m_pData, m_listChildren.m_count, sizeof(MdlNode*),
                  SiblingOrderCmp);
            break;
        }
    }

    for (int64_t i = 0; i < m_listChildren.m_count; i++)
        m_listChildren.m_pData[i]->ReorderChildren();
}

extern int animationfootprint;

// Bills (nDelta = +1) or recredits (nDelta = -1) animationfootprint for every byte the subtree
// owns: two container counts at their element strides, one per-kind size term, and 8 bytes
// per child.
void UpdateAnimFootprint(MdlNode* pNode, int nDelta)
{
    animationfootprint += nDelta * pNode->m_listFloats.m_count * 4;
    animationfootprint += nDelta * pNode->m_listControllers.m_count * 16;

#if KOTOR1  // K1's mesh kinds are 8 bytes smaller
    if (pNode->AsMdlNodeAABB())
        animationfootprint += nDelta * 488;
    else if (pNode->AsMdlNodeAnimMesh())
        animationfootprint += nDelta * 560;
    else if (pNode->AsMdlNodeCamera())
        animationfootprint += nDelta * 104;
    else if (pNode->AsMdlNodeDanglyMesh())
        animationfootprint += nDelta * 520;
    else if (pNode->AsMdlNodeEmitter())
        animationfootprint += nDelta * 328;
    else if (pNode->AsMdlNodeLight())
        animationfootprint += nDelta * 224;
    else if (pNode->AsMdlNodeLightsaber())
        animationfootprint += nDelta * 512;
    else if (pNode->AsMdlNodeReference())
        animationfootprint += nDelta * 144;
    else if (pNode->AsMdlNodeSkin())
        animationfootprint += nDelta * 608;
    else if (pNode->AsMdlNodeTriMesh())
        animationfootprint += nDelta * 480;
#else
    if (pNode->AsMdlNodeAABB())
        animationfootprint += nDelta * 496;
    else if (pNode->AsMdlNodeAnimMesh())
        animationfootprint += nDelta * 568;
    else if (pNode->AsMdlNodeCamera())
        animationfootprint += nDelta * 104;
    else if (pNode->AsMdlNodeDanglyMesh())
        animationfootprint += nDelta * 528;
    else if (pNode->AsMdlNodeEmitter())
        animationfootprint += nDelta * 328;
    else if (pNode->AsMdlNodeLight())
        animationfootprint += nDelta * 224;
    else if (pNode->AsMdlNodeLightsaber())
        animationfootprint += nDelta * 520;
    else if (pNode->AsMdlNodeReference())
        animationfootprint += nDelta * 144;
    else if (pNode->AsMdlNodeSkin())
        animationfootprint += nDelta * 616;
    else if (pNode->AsMdlNodeTriMesh())
        animationfootprint += nDelta * 488;
#endif

    animationfootprint += nDelta * pNode->m_listChildren.m_count * 8;

    for (int64_t i = 0; i < pNode->m_listChildren.m_count; i++)
        UpdateAnimFootprint(pNode->m_listChildren.m_pData[i], nDelta);
}

// Fills result with every renderable tri-mesh in this subtree and returns the number of
// renderable skin nodes found.
int MdlNode::CreateSkinNodeList(List<MdlNodeTriMesh*>& result)
{
    int nCount = 0;

    for (int64_t i = 0; i < m_listChildren.m_count; i++) {
        if (m_listChildren.m_pData[i]->AsMdlNodeLightsaber())
            continue;

        if (m_listChildren.m_pData[i]->AsMdlNodeTriMesh() != 0
            && m_listChildren.m_pData[i]->AsMdlNodeTriMesh()->m_bRender) {
            result.Add(m_listChildren.m_pData[i]->AsMdlNodeTriMesh());

            if (m_listChildren.m_pData[i]->AsMdlNodeSkin())
                nCount++;
        }

        nCount += m_listChildren.m_pData[i]->CreateSkinNodeList(result);
    }

    return nCount;
}

// Re-numbers this tree's nodes against pOther and re-bills the animation footprint.
__attribute__((no_stack_protector))
void MaxTree::SynchronizeTree(MaxTree* pOther)
{
    if (pOther == 0)
        return;
    MdlNode* pOtherNode = pOther->m_pMdlNode;
    int nMaxID = GetMaxID(pOtherNode, -1) + 1;
    SynchronizeNodes(m_pMdlNode, pOtherNode, nMaxID);
    if (AsAnimation() != 0) {
        animationfootprint += 184;
        animationfootprint += AsAnimation()->m_events.m_count * 36;
        UpdateAnimFootprint(m_pMdlNode, 1);
    }
}

void MdlNode::InternalPostProcess() {}

// noinline keeps this body out of the file-scope parser trampoline below, which must stay a
// tail call.
__attribute__((noinline))
void MdlNodeSkin::InternalPostProcess()
{
    if (static_cast<short>(m_nVertexCount) > 0)
        return;

    ProcessVerts();
    MaxFace_PostProcess::texindices1.m_count = 0;
    MaxFace_PostProcess::texindices2.m_count = 0;
    MaxFace_PostProcess::texindices3.m_count = 0;
}

// Builds one animated vertex per face corner, gathering the corner's per-frame stream data,
// and dedups it against the scratchpad's vertex array.
void MdlNodeAnimMesh::InternalGenVertices()
{
    m_nVertexFrameCount = m_lAnimVerts.m_count / PostProcess()->m_verts.m_count;
    m_nTexCoordFrameCount = m_lAnimTVerts.m_count / PostProcess()->m_tverts.m_count;

    for (int64_t i = 0; i < static_cast<int64_t>(m_faces.m_count); i++) {
        for (int j = 0; j < 3; j++) {
            List<Vector> l0, l1, l2;
            int nIdx0 = PostProcess()->m_faces.m_pData[i].m_paVertIndices[j];
            int nIdx1 = PostProcess()->m_tverts.m_count ? PostProcess()->m_faces.m_pData[i].m_paTVertIndices[j] : 0;
            int nIdx2 = PostProcess()->m_faces.m_pData[i].m_paNormalIndices[j];

            for (int k = 0; k < m_nVertexFrameCount; k++) {
                l0.Add(m_lAnimVerts.m_pData[k * PostProcess()->m_verts.m_count + nIdx0]);
                l2.Add(m_lAnimNormals.m_pData[k * PostProcess()->m_normals.m_count + nIdx2]);
            }
            for (int k = 0; k < m_nTexCoordFrameCount; k++)
                l1.Add(m_lAnimTVerts.m_pData[k * PostProcess()->m_tverts.m_count + nIdx1]);

            Vector* pVert = &PostProcess()->m_verts.m_pData[nIdx0];
            Vector* pTex0 = PostProcess()->m_tverts.m_count ? &PostProcess()->m_tverts.m_pData[nIdx1] : 0;
            Vector* pTex1 = PostProcess()->m_tverts1.m_count
                            ? &PostProcess()->m_tverts1.m_pData[PostProcess()->m_faces.m_pData[i].m_paTVertIndices[j + 3]] : 0;
            Vector* pTex2 = PostProcess()->m_tverts2.m_count
                            ? &PostProcess()->m_tverts2.m_pData[PostProcess()->m_faces.m_pData[i].m_paTVertIndices[j + 6]] : 0;
            Vector* pTex3 = PostProcess()->m_tverts3.m_count
                            ? &PostProcess()->m_tverts3.m_pData[PostProcess()->m_faces.m_pData[i].m_paTVertIndices[j + 9]] : 0;
            Vector* pNormal = &PostProcess()->m_normals.m_pData[nIdx2];
            Vector* pColor = PostProcess()->m_colors.m_count >= 1
                             ? &PostProcess()->m_colors.m_pData[PostProcess()->m_faces.m_pData[i].m_paVertIndices[j]] : 0;

            int64_t index;
            NodeVertexAnim* pVertex = new NodeVertexAnim(pVert, pTex0, pTex1, pTex2, pTex3, pNormal, pColor,
                                                         l0, l1, l2);

            for (index = 0; index < static_cast<int64_t>(PostProcess()->m_vertices.m_count); index++) {
                NodeVertex* pOld = reinterpret_cast<NodeVertex*>(PostProcess()->m_vertices.m_pData[index]);
                if (*pOld->AsNodeVertexAnim() == *pVertex) {
                    delete pVertex;
                    pVertex = 0;
                    m_faces.m_pData[i].m_anVertIndices[j] = static_cast<unsigned short>(index);
                    break;
                }
            }
            if (pVertex != 0) {
                m_faces.m_pData[i].m_anVertIndices[j] = static_cast<unsigned short>(PostProcess()->m_vertices.m_count);
                PostProcess()->m_vertices.Add(reinterpret_cast<unsigned int*>(pVertex));
            }
        }
    }
}

void MdlNodeAnimMesh::InternalPostProcess() {}

// Builds the blade's four vertex arrays: a cap piece at each end and a ribbon of segments
// between them.
void MdlNodeLightsaber::InternalPostProcess()
{
    if (static_cast<short>(m_nVertexCount) > 0)
        return;

    ProcessVerts();
    (*m_pfnGenVertices)(this);

    // aurmodel.cpp defines the four shape constants (20, 20, 8, 4).
    K2_ASSUME(NUM_FRONT_SEGMENTS == 20 && NUM_BACK_SEGMENTS == 20 && NUM_PIECE_VERTS == 8 && NUM_SEGMENT_VERTS == 4);
    short nTotal = (NUM_FRONT_SEGMENTS + NUM_BACK_SEGMENTS) * NUM_SEGMENT_VERTS
                 + NUM_PIECE_VERTS * 2;
    m_nVertexCount = nTotal;

    m_paPositions = new Vector3[nTotal];
    m_aNormals = new Vector3[nTotal];
    m_paVertices = reinterpret_cast<float*>(new Vector3[nTotal + 1]);
    m_nTexCoordSets++;
    m_aTexCoords = new Vector2[nTotal];

    // Each cap's gather sets all four before its ribbon reads them (its ordinals run 0..7).
    int nV0 K2_UNSET, nV1 K2_UNSET, nV2 K2_UNSET, nV3 K2_UNSET;
    int nOut = 0;

    // Front cap: slot i takes the record whose front-corner ordinal is i.
    for (int i = 0; i < NUM_PIECE_VERTS; i++, nOut++) {
        int n = 0;
        while (static_cast<int>(PostProcess()->m_colors.m_pData[n].x * 255.0f + 0.5f) != i)
            n++;

        switch (i) {
            case 0: nV0 = nOut; break;
            case 1: nV1 = nOut; break;
            case 2: nV2 = nOut; break;
            case 3: nV3 = nOut; break;
        }

        m_aTexCoords[nOut].u = PostProcess()->m_tverts.m_pData[n].x;
        m_aTexCoords[nOut].v = PostProcess()->m_tverts.m_pData[n].y;
        m_aNormals[nOut].x = PostProcess()->m_normals.m_pData[n].x;
        m_aNormals[nOut].y = PostProcess()->m_normals.m_pData[n].y;
        m_aNormals[nOut].z = PostProcess()->m_normals.m_pData[n].z;
        m_paPositions[nOut].x = PostProcess()->m_verts.m_pData[n].x;
        m_paPositions[nOut].y = PostProcess()->m_verts.m_pData[n].y;
        m_paPositions[nOut].z = PostProcess()->m_verts.m_pData[n].z;
        reinterpret_cast<Vector3*>(m_paVertices)[nOut] = m_paPositions[nOut];
    }

    // Front ribbon: the four corner slots, repeated per segment.
    for (int s = 0; s < NUM_FRONT_SEGMENTS; s++) {
        m_aTexCoords[nOut] = m_aTexCoords[nV0];
        m_aNormals[nOut].x = m_aNormals[nV0].x;
        m_aNormals[nOut].y = m_aNormals[nV0].y;
        m_aNormals[nOut].z = m_aNormals[nV0].z;
        m_paPositions[nOut].x = m_paPositions[nV0].x;
        m_paPositions[nOut].y = m_paPositions[nV0].y;
        m_paPositions[nOut].z = m_paPositions[nV0].z;
        reinterpret_cast<Vector3*>(m_paVertices)[nOut] = m_paPositions[nOut];

        m_aTexCoords[nOut + 1] = m_aTexCoords[nV1];
        m_aNormals[nOut + 1].x = m_aNormals[nV1].x;
        m_aNormals[nOut + 1].y = m_aNormals[nV1].y;
        m_aNormals[nOut + 1].z = m_aNormals[nV1].z;
        m_paPositions[nOut + 1].x = m_paPositions[nV1].x;
        m_paPositions[nOut + 1].y = m_paPositions[nV1].y;
        m_paPositions[nOut + 1].z = m_paPositions[nV1].z;
        reinterpret_cast<Vector3*>(m_paVertices)[nOut + 1] = m_paPositions[nOut + 1];

        m_aTexCoords[nOut + 2] = m_aTexCoords[nV2];
        m_aNormals[nOut + 2].x = m_aNormals[nV2].x;
        m_aNormals[nOut + 2].y = m_aNormals[nV2].y;
        m_aNormals[nOut + 2].z = m_aNormals[nV2].z;
        m_paPositions[nOut + 2].x = m_paPositions[nV2].x;
        m_paPositions[nOut + 2].y = m_paPositions[nV2].y;
        m_paPositions[nOut + 2].z = m_paPositions[nV2].z;
        reinterpret_cast<Vector3*>(m_paVertices)[nOut + 2] = m_paPositions[nOut + 2];

        m_aTexCoords[nOut + 3] = m_aTexCoords[nV3];
        m_aNormals[nOut + 3].x = m_aNormals[nV3].x;
        m_aNormals[nOut + 3].y = m_aNormals[nV3].y;
        m_aNormals[nOut + 3].z = m_aNormals[nV3].z;
        m_paPositions[nOut + 3].x = m_paPositions[nV3].x;
        m_paPositions[nOut + 3].y = m_paPositions[nV3].y;
        m_paPositions[nOut + 3].z = m_paPositions[nV3].z;
        reinterpret_cast<Vector3*>(m_paVertices)[nOut + 3] = m_paPositions[nOut + 3];

        nOut += 4;
    }

    // Back cap: the same gather, keyed on the colors record's third float.
    for (int i = 0; i < NUM_PIECE_VERTS; i++, nOut++) {
        int n = 0;
        while (static_cast<int>(PostProcess()->m_colors.m_pData[n].z * 255.0f + 0.5f) != i)
            n++;

        switch (i) {
            case 0: nV0 = nOut; break;
            case 1: nV1 = nOut; break;
            case 2: nV2 = nOut; break;
            case 3: nV3 = nOut; break;
        }

        m_aTexCoords[nOut].u = PostProcess()->m_tverts.m_pData[n].x;
        m_aTexCoords[nOut].v = PostProcess()->m_tverts.m_pData[n].y;
        m_aNormals[nOut].x = PostProcess()->m_normals.m_pData[n].x;
        m_aNormals[nOut].y = PostProcess()->m_normals.m_pData[n].y;
        m_aNormals[nOut].z = PostProcess()->m_normals.m_pData[n].z;
        m_paPositions[nOut].x = PostProcess()->m_verts.m_pData[n].x;
        m_paPositions[nOut].y = PostProcess()->m_verts.m_pData[n].y;
        m_paPositions[nOut].z = PostProcess()->m_verts.m_pData[n].z;
        reinterpret_cast<Vector3*>(m_paVertices)[nOut] = m_paPositions[nOut];
    }

    for (int s = 0; s < NUM_BACK_SEGMENTS; s++) {
        m_aTexCoords[nOut] = m_aTexCoords[nV0];
        m_aNormals[nOut].x = m_aNormals[nV0].x;
        m_aNormals[nOut].y = m_aNormals[nV0].y;
        m_aNormals[nOut].z = m_aNormals[nV0].z;
        m_paPositions[nOut].x = m_paPositions[nV0].x;
        m_paPositions[nOut].y = m_paPositions[nV0].y;
        m_paPositions[nOut].z = m_paPositions[nV0].z;
        reinterpret_cast<Vector3*>(m_paVertices)[nOut] = m_paPositions[nOut];

        m_aTexCoords[nOut + 1] = m_aTexCoords[nV1];
        m_aNormals[nOut + 1].x = m_aNormals[nV1].x;
        m_aNormals[nOut + 1].y = m_aNormals[nV1].y;
        m_aNormals[nOut + 1].z = m_aNormals[nV1].z;
        m_paPositions[nOut + 1].x = m_paPositions[nV1].x;
        m_paPositions[nOut + 1].y = m_paPositions[nV1].y;
        m_paPositions[nOut + 1].z = m_paPositions[nV1].z;
        reinterpret_cast<Vector3*>(m_paVertices)[nOut + 1] = m_paPositions[nOut + 1];

        m_aTexCoords[nOut + 2] = m_aTexCoords[nV2];
        m_aNormals[nOut + 2].x = m_aNormals[nV2].x;
        m_aNormals[nOut + 2].y = m_aNormals[nV2].y;
        m_aNormals[nOut + 2].z = m_aNormals[nV2].z;
        m_paPositions[nOut + 2].x = m_paPositions[nV2].x;
        m_paPositions[nOut + 2].y = m_paPositions[nV2].y;
        m_paPositions[nOut + 2].z = m_paPositions[nV2].z;
        reinterpret_cast<Vector3*>(m_paVertices)[nOut + 2] = m_paPositions[nOut + 2];

        m_aTexCoords[nOut + 3] = m_aTexCoords[nV3];
        m_aNormals[nOut + 3].x = m_aNormals[nV3].x;
        m_aNormals[nOut + 3].y = m_aNormals[nV3].y;
        m_aNormals[nOut + 3].z = m_aNormals[nV3].z;
        m_paPositions[nOut + 3].x = m_paPositions[nV3].x;
        m_paPositions[nOut + 3].y = m_paPositions[nV3].y;
        m_paPositions[nOut + 3].z = m_paPositions[nV3].z;
        reinterpret_cast<Vector3*>(m_paVertices)[nOut + 3] = m_paPositions[nOut + 3];

        nOut += 4;
    }

    m_nVertexStride = 0;
}

// Resolves the flare texture names against the texture manager.
void MdlNodeLight::InternalPostProcess()
{
    m_lFlareTextures.SetSize(m_lFlareNames.m_count);
    int64_t i = static_cast<int64_t>(m_lFlareNames.m_count);
    while (i != 0) {
        i--;
        m_lFlareTextures.m_pData[i] = AurTextureGetReference(m_lFlareNames.m_pData[i], 0);
    }
}

// Raises the owning model's flare distance to at least 40.0f.
__attribute__((noinline))
void MdlNodeEmitter::InternalPostProcess()
{
    Model* pModel = m_pTree->AsModel();
    if (pModel && pModel->m_fRadius < 40.0f)
        pModel->m_fRadius = 40.0f;
}

// Frees this kind's vectors, then the base's temporary arrays.
void MdlNodeAnimMesh::InternalRemoveTemporaryArrays()
{
    for (int64_t i = 0; i < static_cast<MdlNodeTriMesh_PostProcess*>(m_pPostProcess)->m_vertices.m_count; i++) {
        NodeVertexAnim* pAnim = reinterpret_cast<NodeVertexAnim*>(static_cast<MdlNodeTriMesh_PostProcess*>(m_pPostProcess)->m_vertices.m_pData[i]);
        if (pAnim != 0) {
            pAnim->~NodeVertexAnim();
            operator delete(pAnim);
        }
    }
    static_cast<MdlNodeTriMesh_PostProcess*>(m_pPostProcess)->m_vertices.SetSize(0);
    m_lAnimVerts.SetSize(0);
    m_lAnimTVerts.SetSize(0);
    m_lAnimNormals.SetSize(0);
    MdlNodeTriMesh::InternalRemoveTemporaryArrays();
}

void MdlNodeSkin::InternalRemoveTemporaryArrays()
{
    MdlNodeTriMesh::InternalRemoveTemporaryArrays();
}

// libc is opaque in this build, so the epsilon test is spelled as a conditional negate.
#define SEAM_ABS(f) ((f) < 0.0f ? -(f) : (f))

void MdlNode::ProcessSkinSeams()
{
    // A negative tag means the tree is neither a model nor an animation.
    if (m_pTree->m_cType < 0)
        return;

    List<MdlNodeTriMesh*> meshes;
    if (CreateSkinNodeList(meshes) == 0)
        return;

    List<Vector> positions;
    List<Quaternion> orientations;
    for (int64_t i = 0; i < meshes.m_count; i++) {
        Vector v(0.0f, 0.0f, 0.0f);
        Quaternion q;

        for (MdlNode* pNode = meshes.m_pData[i]; pNode != 0;
             pNode = reinterpret_cast<MdlNode*>(pNode->m_pParent)) {
            q = q * pNode->m_qOrientation;
            v = RotateVector(v, pNode->m_qOrientation) + pNode->m_vPosition;
        }

        positions.Add(v);
        orientations.Add(q);
    }

    void* pPool = 0;
    K2_ASSUME(meshes.m_count == 0 || meshes.m_pData[0] != 0);   // CreateSkinNodeList adds only non-null meshes
    if (meshes.m_count > 0)
        pPool = GLRender::LockPool(meshes.m_pData[0]->m_pTree->AsModel()->m_nPool,
                                   0, 0, 16);

    List<unsigned int*> matchVerts;
    List<int> matchMeshes;

    for (int64_t i = 0; i < meshes.m_count - 1; i++) {
        MdlNodeTriMesh* pMesh = meshes.m_pData[i];

        for (int v = 0; v < static_cast<short>(pMesh->m_nVertexCount); v++) {
            unsigned char* pVertex = static_cast<unsigned char*>(pPool) + pMesh->m_nGeomBase
                + pMesh->m_nVertexStride * v;

            Vector vWorld = RotateVector(
                *reinterpret_cast<Vector*>(pVertex + pMesh->m_nVertexOfs),
                orientations.m_pData[i]) + positions.m_pData[i];
            Vector vNormal = RotateVector(
                *reinterpret_cast<Vector*>(pVertex + pMesh->m_nNormalOfs),
                orientations.m_pData[i]);

            matchVerts.SetSize(0);
            matchMeshes.SetSize(0);
            matchVerts.Add(reinterpret_cast<unsigned int*>(pVertex + pMesh->m_nNormalOfs));
            matchMeshes.Add(static_cast<int>(i));

            for (int64_t j = 0; j < meshes.m_count; j++) {
                if (j == i)
                    continue;

                MdlNodeTriMesh* pOther = meshes.m_pData[j];
                for (int u = 0; u <= static_cast<short>(pOther->m_nVertexCount); u++) {
                    unsigned char* pOtherVertex = static_cast<unsigned char*>(pPool) + pOther->m_nGeomBase
                        + pOther->m_nVertexStride * u;
                    Vector vOtherWorld = RotateVector(
                        *reinterpret_cast<Vector*>(pOtherVertex + pOther->m_nVertexOfs),
                        orientations.m_pData[j]) + positions.m_pData[j];

                    float fDx = vOtherWorld.x - vWorld.x;
                    float fDy = vOtherWorld.y - vWorld.y;
                    float fDz = vOtherWorld.z - vWorld.z;
                    if (SEAM_ABS(fDx) < 0.001f && SEAM_ABS(fDy) < 0.001f
                        && SEAM_ABS(fDz) < 0.001f) {
                        matchVerts.Add(
                            reinterpret_cast<unsigned int*>(pOtherVertex + pOther->m_nNormalOfs));
                        matchMeshes.Add(static_cast<int>(j));
                        vNormal = vNormal + RotateVector(
                            *reinterpret_cast<Vector*>(pOtherVertex + pOther->m_nNormalOfs),
                            orientations.m_pData[j]);
                    }
                }
            }

            if (matchVerts.m_count != 1) {
                if (matchVerts.m_count != 0)
                    vNormal = vNormal * (1.0f / static_cast<float>(matchVerts.m_count));

                vNormal.Normalize();

                for (int k = 0; k < matchVerts.m_count; k++)
#if KOTOR1  // K1 rotates by the out-of-line inverse quaternion
                    *reinterpret_cast<Vector*>(matchVerts.m_pData[k]) = RotateVector(
                        vNormal, -orientations.m_pData[matchMeshes.m_pData[k]]);
#else
                    *reinterpret_cast<Vector*>(matchVerts.m_pData[k]) = RotateByTranspose(
                        vNormal, orientations.m_pData[matchMeshes.m_pData[k]]);
#endif
            }
        }
    }

    if (meshes.m_count > 0)
        GLRender::UnlockPool(meshes.m_pData[0]->m_pTree->AsModel()->m_nPool);
}

// Explicit instantiation used by ProcessSkinSeams.
template void List<Vector>::Add(Vector item);


int enabletristrips;
extern int minstripsize;

int countcrappyfaces1;
int countcrappyfaces2;
int countcrappyfaces3;


void AurStripify(List<int>& faceIndices, List<List<int> >& stripList);


// Recomputes each face's normal and plane distance, then records, per corner, the first other
// face sharing that edge. K2 also counts the faces whose three edges are all unshared.
void MdlNodeTriMesh::BuildNeighborLists()
{
    if (m_faces.m_count < 1)
        return;
    for (int64_t i = 0; i < m_faces.m_count; i++) {
        Vector* pVerts = PostProcess()->m_verts.m_pData;
        MaxFace_PostProcess& pFace = PostProcess()->m_faces.m_pData[i];
        Vector vNormal = NormalOf(pVerts[pFace.m_paVertIndices[0]], pVerts[pFace.m_paVertIndices[1]],
                                  pVerts[pFace.m_paVertIndices[2]]);
        MaxFace& face = m_faces.m_pData[i];
        face.Normal() = vNormal;
        Vector& vP0 = pVerts[pFace.m_paVertIndices[0]];
        Vector& vN = face.Normal();
        face.m_fPlaneDist = -dot(vN, vP0);
        for (int k = 0; k < 3; k++) {
            int nV0 = pFace.m_paVertIndices[k];
            int nV1 = pFace.m_paVertIndices[(k + 1) % 3];
            face.m_anNeighbors[k] = -1;
            for (int64_t j = 0; j < m_faces.m_count; j++) {
                if (j != i) {
                    int* pOther = PostProcess()->m_faces.m_pData[j].m_paVertIndices;
                    bool bEdge = false;
                    for (int m = 0; m < 3; m++) {
                        if (pOther[m] != nV0)
                            continue;
                        if (pOther[(m + 1) % 3] == nV1 || pOther[(m + 2) % 3] == nV1) {
                            bEdge = true;
                            break;
                        }
                    }
                    if (bEdge)
                        face.m_anNeighbors[k] = static_cast<short>(j);
                    else
                        face.m_anNeighbors[k] = -1;
                }
                if (face.m_anNeighbors[k] >= 0)
                    break;
            }
        }
#if !KOTOR1  // K1 has no "P_HK47" unshared-edge counters
        if (_stricmp(m_pTree->m_szName, "P_HK47") == 0) {
            int nUnshared = 0;
            if (face.m_anNeighbors[0] == -1)
                nUnshared++;
            if (face.m_anNeighbors[1] == -1)
                nUnshared++;
            if (face.m_anNeighbors[2] == -1)
                nUnshared++;
            if (nUnshared == 3)
                countcrappyfaces3++;
            else if (nUnshared == 2)
                countcrappyfaces2++;
            else if (nUnshared == 1)
                countcrappyfaces1++;
        }
#endif
    }
}

// Collapses the scratchpad's face corners into a deduplicated vertex array, re-points each
// face's corner indices at it, then drops faces that reference one vertex twice.
void MdlNodeTriMesh::InternalGenVertices()
{
    for (int i = 0; i < PostProcess()->m_faces.m_count; i++) {
        for (int j = 0; j < 3; j++) {
            int nVert = PostProcess()->m_faces.m_pData[i].m_paVertIndices[j];
            Vector* pv0 = PostProcess()->m_tverts.m_count ? &PostProcess()->m_tverts.m_pData[PostProcess()->m_faces.m_pData[i].m_paTVertIndices[j]] : 0;
            Vector* pv1 = PostProcess()->m_tverts1.m_count ? &PostProcess()->m_tverts1.m_pData[PostProcess()->m_faces.m_pData[i].m_paTVertIndices[j + 3]] : 0;
            Vector* pv2 = PostProcess()->m_tverts2.m_count ? &PostProcess()->m_tverts2.m_pData[PostProcess()->m_faces.m_pData[i].m_paTVertIndices[j + 6]] : 0;
            Vector* pPos = &PostProcess()->m_verts.m_pData[nVert];
            Vector* pv3 = PostProcess()->m_tverts3.m_count ? &PostProcess()->m_tverts3.m_pData[PostProcess()->m_faces.m_pData[i].m_paTVertIndices[j + 9]] : 0;
            Vector* pNormal = &PostProcess()->m_normals.m_pData[PostProcess()->m_faces.m_pData[i].m_paNormalIndices[j]];
            Vector* pColor = PostProcess()->m_colors.m_count >= 1 ? &PostProcess()->m_colors.m_pData[nVert] : 0;
            NodeVertex* pVertex = new NodeVertex(pPos, pv0, pv1, pv2, pv3, pNormal, pColor);
            int64_t index;
            for (index = 0; index < static_cast<int64_t>(PostProcess()->m_vertices.m_count); index++) {
                if (*reinterpret_cast<NodeVertex*>(PostProcess()->m_vertices.m_pData[index]) == *pVertex) {
                    delete pVertex;
                    break;
                }
            }
            if (index == static_cast<int64_t>(PostProcess()->m_vertices.m_count))
                PostProcess()->m_vertices.Add(reinterpret_cast<unsigned int*>(pVertex));
            m_faces.m_pData[i].m_anVertIndices[j] = static_cast<unsigned short>(index);
        }
    }
    if (AsMdlNodeAABB() != 0)
        return;
    int64_t i = static_cast<int64_t>(m_faces.m_count);
    while (i != 0) {
        i--;
        MaxFace& face = m_faces.m_pData[i];
        if (face.m_anVertIndices[0] == face.m_anVertIndices[1] ||
            face.m_anVertIndices[0] == face.m_anVertIndices[2] ||
            face.m_anVertIndices[1] == face.m_anVertIndices[2]) {
            // inlined List<MaxFace>::DelIndex(i)
            m_faces.m_count--;
            for (int64_t j = i; j < static_cast<int64_t>(m_faces.m_count); j++)
                m_faces.m_pData[j] = m_faces.m_pData[j + 1];
        }
    }
}

// Batch vertex transform, and the first step of every derived kind's post-process: fills the
// shared texcoord index caches, averages each corner's smoothing group into the normal
// list, and updates the bounds, centroid and radius.
void MdlNodeTriMesh::ProcessVerts()
{
    if (MaxFace_PostProcess::texindices1.m_count == 0 && m_bHasLightmap) {
        for (int64_t i = 0; i < PostProcess()->m_faces.m_count; i++) {
            int* pnTvert = PostProcess()->m_faces.m_pData[i].m_paTVertIndices;
            MaxFace_PostProcess::texindices1.Add(
                Vector(static_cast<float>(pnTvert[0]), static_cast<float>(pnTvert[1]), static_cast<float>(pnTvert[2])));
        }
    }
    for (int64_t i = 0; i < PostProcess()->m_faces.m_count; i++) {
        MaxFace_PostProcess& pFace = PostProcess()->m_faces.m_pData[i];
        int nMask = pFace.m_nSmoothGroup;
        for (int64_t j = 0; j < 3; j++) {
            // Each enabled UV set writes its own three indices into the face record.
            if (MaxFace_PostProcess::texindices1.m_count != 0) {
                Vector& vT1 = MaxFace_PostProcess::texindices1.m_pData[i];
                int* pnT = PostProcess()->m_faces.m_pData[i].m_paTVertIndices;
                pnT[3] = static_cast<int>(vT1.x);
                pnT[4] = static_cast<int>(vT1.y);
                pnT[5] = static_cast<int>(vT1.z);
            }
            if (MaxFace_PostProcess::texindices2.m_count != 0) {
                Vector& vT2 = MaxFace_PostProcess::texindices2.m_pData[i];
                int* pnT = PostProcess()->m_faces.m_pData[i].m_paTVertIndices;
                pnT[6] = static_cast<int>(vT2.x);
                pnT[7] = static_cast<int>(vT2.y);
                pnT[8] = static_cast<int>(vT2.z);
            }
            if (MaxFace_PostProcess::texindices3.m_count != 0) {
                Vector& vT3 = MaxFace_PostProcess::texindices3.m_pData[i];
                int* pnT = PostProcess()->m_faces.m_pData[i].m_paTVertIndices;
                pnT[9] = static_cast<int>(vT3.x);
                pnT[10] = static_cast<int>(vT3.y);
                pnT[11] = static_cast<int>(vT3.z);
            }
            pFace.m_paNormalIndices[j] = -1;
            int nVert = pFace.m_paVertIndices[j];
            Vector vAcc;
            for (int64_t k = 0; k < PostProcess()->m_faces.m_count; k++) {
                MaxFace_PostProcess& pOther = PostProcess()->m_faces.m_pData[k];
                int m = 2;
                while (m != -1) {
                    if (pOther.m_paVertIndices[m] == nVert)
                        break;
                    m--;
                }
                if (m < 0)
                    continue;
                if (k != i && (pOther.m_nSmoothGroup & nMask) == 0)
                    continue;
                Vector& v0 = PostProcess()->m_verts.m_pData[pOther.m_paVertIndices[0]];
                Vector& v1 = PostProcess()->m_verts.m_pData[pOther.m_paVertIndices[1]];
                Vector& v2 = PostProcess()->m_verts.m_pData[pOther.m_paVertIndices[2]];
                Vector vN = cross(v1 - v0, v2 - v1);
                vAcc.x += vN.x;
                vAcc.y += vN.y;
                vAcc.z += vN.z;
                if (i <= k)
                    continue;
                if (pOther.m_nSmoothGroup != nMask)
                    continue;
                pFace.m_paNormalIndices[j] = pOther.m_paNormalIndices[m];
                break;
            }
            if (magnitude(vAcc) != 0.0f)
                vAcc.Normalize();
            if (pFace.m_paNormalIndices[j] == -1) {
                pFace.m_paNormalIndices[j] = PostProcess()->m_normals.m_count;
                PostProcess()->m_normals.Add(vAcc);
            }
        }
    }
    Vector vSum;
    for (int64_t i = 0; i < PostProcess()->m_verts.m_count; i++) {
        Vector& v = PostProcess()->m_verts.m_pData[i];
        float* pfV = reinterpret_cast<float*>(&v);
        float* pfMin = reinterpret_cast<float*>(&m_vMin);
        float* pfMax = reinterpret_cast<float*>(&m_vMax);
        // Minimum and maximum per axis, then the sum.
        for (int c = 0; c < 3; c++)
            pfMin[c] = pfV[c] < pfMin[c] ? pfV[c] : pfMin[c];  // raw-offset: a Vector's components by index
        for (int c = 0; c < 3; c++)
            pfMax[c] = pfMax[c] < pfV[c] ? pfV[c] : pfMax[c];  // raw-offset: a Vector's components by index
        vSum.x += v.x;
        vSum.y += v.y;
        vSum.z += v.z;
    }
    if (PostProcess()->m_verts.m_count != 0) {
        float fInv = 1.0f / static_cast<float>(PostProcess()->m_verts.m_count);
        vSum.x *= fInv;
        vSum.y *= fInv;
        vSum.z *= fInv;
    }
    m_vCentroid = vSum;
    for (int64_t i = 0; i < PostProcess()->m_verts.m_count; i++) {
        Vector vDelta = PostProcess()->m_verts.m_pData[i] - m_vCentroid;
        float fDist = magnitude(vDelta);
        if (fDist > m_fRadius)
            m_fRadius = fDist;
    }
}

// Builds the index buffers. With enabletristrips the faces are stripified and each strip becomes
// its own buffer, while the strips shorter than minstripsize are expanded back into triangles;
// otherwise the faces go into one buffer directly. Each buffer is then locked and uploaded
// as 16-bit indices.
void MdlNodeTriMesh::BuildVertexIndices()
{
    if (enabletristrips) {
        List<int> faceIndices;
        List<List<int> > stripList;
        for (int i = 0; i < m_faces.m_count; i++) {
            for (int j = 0; j < 3; j++)
                faceIndices.Add(m_faces.m_pData[i].m_nVIndex[j]);
        }
        AurStripify(faceIndices, stripList);
        for (int i = 0; i < stripList.m_count - 1; i++) {
            if (stripList.m_pData[i].m_count >= minstripsize) {
                m_lVertexIndicesCount.Add(stripList.m_pData[i].m_count);
                unsigned int* paStrip = new unsigned int[stripList.m_pData[i].m_count];
                for (int j = 0; j < stripList.m_pData[i].m_count; j++)
                    paStrip[j] = stripList.m_pData[i].m_pData[j];
                m_lVertexIndices.Add(paStrip);
            } else {
                for (int j = 0; j < 3; j++)
                    m_lLeftOverFaces.Add(stripList.m_pData[i].m_pData[j]);
                for (int j = 3; j < stripList.m_pData[i].m_count; j++) {
                    if (j & 1) {
                        m_lLeftOverFaces.Add(stripList.m_pData[i].m_pData[j]);
                        m_lLeftOverFaces.Add(stripList.m_pData[i].m_pData[j - 1]);
                        m_lLeftOverFaces.Add(stripList.m_pData[i].m_pData[j - 2]);
                    } else {
                        m_lLeftOverFaces.Add(stripList.m_pData[i].m_pData[j - 2]);
                        m_lLeftOverFaces.Add(stripList.m_pData[i].m_pData[j - 1]);
                        m_lLeftOverFaces.Add(stripList.m_pData[i].m_pData[j]);
                    }
                }
            }
        }
        for (int j = 0; j < stripList.m_pData[stripList.m_count - 1].m_count; j++)
            m_lLeftOverFaces.Add(stripList.m_pData[stripList.m_count - 1].m_pData[j]);
        m_nPrimitiveType = 4;
    } else {
        unsigned int* paStrip = new unsigned int[m_faces.m_count * 3];
        for (int i = 0; i < m_faces.m_count; i++) {
            for (int j = 0; j < 3; j++)
                paStrip[i * 3 + j] = m_faces.m_pData[i].m_nVIndex[j];
        }
        m_lVertexIndices.Add(paStrip);
        m_lVertexIndicesCount.Add(m_faces.m_count * 3);
        m_nPrimitiveType = 3;
    }
    if (enabletristrips) {
        for (int i = 0; i < m_lVertexIndices.m_count; i++) {
            unsigned int nPool;
            GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(1),
                                  m_lVertexIndicesCount.m_pData[i] * 2, 2, 0, &nPool);
            m_lIndexPools.Add(nPool);
            unsigned short* pPool = static_cast<unsigned short*>(GLRender::LockPool(nPool, 0, 0, 1));
            for (int j = 0; j < m_lVertexIndicesCount.m_pData[i]; j++)
                pPool[j] = static_cast<unsigned short>(m_lVertexIndices.m_pData[i][j]);
            GLRender::UnlockPool(nPool);
            delete[] m_lVertexIndices.m_pData[i];
        }
        GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(1), m_lLeftOverFaces.m_count * 2, 2, 0,
                              &m_nLeftOverPool);
        unsigned short* pPool = static_cast<unsigned short*>(GLRender::LockPool(m_nLeftOverPool, 0, 0, 1));
        for (int j = 0; j < m_lLeftOverFaces.m_count; j++)
            pPool[j] = static_cast<unsigned short>(m_lLeftOverFaces.m_pData[j]);
        GLRender::UnlockPool(m_nLeftOverPool);
        m_nLeftOverIndexCount = m_lLeftOverFaces.m_count;
        m_lVertexIndices.SetSize(0);
        m_lLeftOverFaces.SetSize(0);
    } else {
        for (int i = 0; i < m_lVertexIndices.m_count; i++) {
            unsigned int nPool;
            GLRender::RequestPool(static_cast<Aurora::AuroraBufferRequestType>(1),
                                  m_lVertexIndicesCount.m_pData[i] * 2, 2, 0, &nPool);
            m_lIndexPools.Add(nPool);
            unsigned short* pPool = static_cast<unsigned short*>(GLRender::LockPool(nPool, 0, 0, 1));
            for (int j = 0; j < m_lVertexIndicesCount.m_pData[i]; j++)
                pPool[j] = static_cast<unsigned short>(m_lVertexIndices.m_pData[i][j]);
            GLRender::UnlockPool(nPool);
            GLRender::UnlockPool(nPool);
            delete[] m_lVertexIndices.m_pData[i];
        }
        m_lVertexIndices.SetSize(0);
        m_nLeftOverPool = 0xffffffff;
        m_nLeftOverIndexCount = 0;
    }
}

// Builds a tangent frame per face, averages the frames of each corner's smoothing group, and
// writes the three basis vectors into the mesh's vertex buffer.
void MdlNodeTriMesh::CalculateTangentSpaceBasis()
{
    unsigned char* pPool = static_cast<unsigned char*>(GLRender::LockPool(m_pTree->AsModel()->m_nPool, 0, 0, 1));

    Matrix* paMatrices = new Matrix[m_faces.m_count];

    for (int64_t i = 0; i < m_faces.m_count; i++) {
        NodeVertex* pV0 = reinterpret_cast<NodeVertex*>(PostProcess()->m_vertices.m_pData[m_faces.m_pData[i].m_nVIndex[0]]);
        NodeVertex* pV1 = reinterpret_cast<NodeVertex*>(PostProcess()->m_vertices.m_pData[m_faces.m_pData[i].m_nVIndex[1]]);
        NodeVertex* pV2 = reinterpret_cast<NodeVertex*>(PostProcess()->m_vertices.m_pData[m_faces.m_pData[i].m_nVIndex[2]]);
        Vector vE1 = pV1->m_vPos - pV0->m_vPos;
        Vector vE2 = pV2->m_vPos - pV0->m_vPos;
        float fDU1 = pV1->m_vTex0.x - pV0->m_vTex0.x;
        float fDU2 = pV2->m_vTex0.x - pV0->m_vTex0.x;
        float fDV1 = pV1->m_vTex0.y - pV0->m_vTex0.y;
        float fDV2 = pV2->m_vTex0.y - pV0->m_vTex0.y;
        Vector vTangent = normalize(vE2 * fDU1 - vE1 * fDU2);
        Vector vBinormal = normalize(vE2 * fDV1 - vE1 * fDV2);
        Vector vNormal = cross(vTangent, vBinormal);
        Vector& vFaceNormal =
            PostProcess()->m_normals.m_pData[*PostProcess()->m_faces.m_pData[i].m_paNormalIndices];
        if (vNormal.x * vFaceNormal.x + vNormal.y * vFaceNormal.y + vNormal.z * vFaceNormal.z < 0.0f) {
            vBinormal = -vBinormal;
            vNormal = -vNormal;
        }
        paMatrices[i].m_f[0] = vTangent.x;
        paMatrices[i].m_f[1] = vTangent.y;
        paMatrices[i].m_f[2] = vTangent.z;
        paMatrices[i].m_f[3] = vBinormal.x;
        paMatrices[i].m_f[4] = vBinormal.y;
        paMatrices[i].m_f[5] = vBinormal.z;
        paMatrices[i].m_f[6] = vNormal.x;
        paMatrices[i].m_f[7] = vNormal.y;
        paMatrices[i].m_f[8] = vNormal.z;
    }

    for (int64_t i = 0; i < m_faces.m_count; i++) {
        for (int j = 0; j < 3; j++) {
            int nIndex = m_faces.m_pData[i].m_nVIndex[j];
            NodeVertex* pVertex = reinterpret_cast<NodeVertex*>(PostProcess()->m_vertices.m_pData[nIndex]);
            Vector vRef = pVertex->m_vPos;
            int nFlags = PostProcess()->m_faces.m_pData[i].m_nSmoothGroup;

            List<int> faces;
            faces.Add(static_cast<int>(i));
            for (int k = 0; k < m_faces.m_count; k++) {
                if (k == i)
                    continue;
                for (int m = 0; m < 3; m++) {
                    NodeVertex* pTest = reinterpret_cast<NodeVertex*>(PostProcess()->m_vertices.m_pData[
                        m_faces.m_pData[k].m_nVIndex[m]]);
                    Vector vTest = pTest->m_vPos;
                    if (equal(vRef, vTest) && (nFlags & PostProcess()->m_faces.m_pData[k].m_nSmoothGroup))
                        faces.Add(k);
                }
            }

            Matrix mtx;
            for (int z = 0; z < 9; z++)
                mtx.m_f[z] = 0.0f;
            for (int64_t l = 0; l < faces.m_count; l++)
                mtx = mtx + paMatrices[faces.m_pData[l]];
            Vector* pRows = mtx.Rows();
            pRows[0].Normalize();
            pRows[1].Normalize();
            pRows[2].Normalize();
            mtx = Transpose(mtx);

            for (int c = 0; c < 3; c++) {
                for (int r = 0; r < 3; r++) {
#if KOTOR1
                    CAlignedPtr<unsigned int>(reinterpret_cast<unsigned int*>(
                        pPool + (c * 3 + r) * 4 + m_nVertexStride * nIndex + m_nTangentOfs + m_nGeomBase))
                        .Set(reinterpret_cast<unsigned int&>(mtx.m_f[r * 3 + c]));
#else
                    *reinterpret_cast<float*>(&pPool[m_nGeomBase + m_nVertexStride * nIndex + m_nTangentOfs
                                    + (c * 3 + r) * 4]) = mtx.m_f[r * 3 + c];
#endif
                }
            }
        }
    }

    delete[] paMatrices;
    GLRender::UnlockPool(m_pTree->AsModel()->m_nPool);
}

// Builds the interleaved vertex buffer this kind draws from: sanitises the name buffers, lays out
// the per-vertex element table, then fills the pooled rows and updates the surface area.
void MdlNodeTriMesh::InternalPostProcess()
{
    if (static_cast<short>(m_nVertexCount) > 0)
        return;

    if (m_szTexture[0] == 0)
        strcpy(m_szTexture, NULL_NAME);
    if (strlen(m_szLightmap) > 16)
        m_szLightmap[16] = 0;

    ProcessVerts();

    if (m_faces.m_count > 0) {
        (*m_pfnGenVertices)(this);
        m_nVertexCount = static_cast<unsigned short>(PostProcess()->m_vertices.m_count);
        BuildNeighborLists();
        BuildVertexIndices();

        NodeVertex* pFirst = static_cast<short>(m_nVertexCount) >= 1
            ? reinterpret_cast<NodeVertex*>(PostProcess()->m_vertices.m_pData[0]) : 0;
#ifdef K2_FIX_ENGINE_BUGS
        if (pFirst == 0)
            return;
#endif

        m_uVertexFlags |= MDX_VERTICES | MDX_VERTEX_NORMALS;
        m_nVertexOfs = 0;
        m_nNormalOfs = 12;
        int nOfs = 24;
        if (pFirst->m_nTexCoordFlags & kNodeVertexTex0) {
            m_nTex0Ofs = nOfs;
            nOfs += 8;
            m_nTexCoordSets++;
            m_uVertexFlags |= MDX_TEX0_VERTICES;
        }
        if (pFirst->m_nTexCoordFlags & kNodeVertexTex1) {
            m_nTex1Ofs = nOfs;
            nOfs += 8;
            m_nTexCoordSets++;
            m_uVertexFlags |= MDX_TEX1_VERTICES;
        }
        if (pFirst->m_nTexCoordFlags & kNodeVertexTex2) {
            m_nTex2Ofs = nOfs;
            nOfs += 8;
            m_nTexCoordSets++;
            m_uVertexFlags |= MDX_TEX2_VERTICES;
        }
        if (pFirst->m_nTexCoordFlags & kNodeVertexTex3) {
            m_nTex3Ofs = nOfs;
            nOfs += 8;
            m_nTexCoordSets++;
            m_uVertexFlags |= MDX_TEX3_VERTICES;
        }

        Material* pMat = new Material();
        pMat->InitializeTextures(m_szTexture, 0, 0, 0);
        int bTangent = 0;
        if (pMat->HasBumpMap()) {
            if (!pMat->HasNormalMap()) {
                m_nTangentOfs = nOfs;
                nOfs += 36;
                m_uVertexFlags |= MDX_TANGENT_SPACE;
                bTangent = 1;
            }
        }
        m_nVertexStride = nOfs;

        unsigned int nLow4 = m_pTree->AsModel()->m_nPoolSize & 0xf;
        GLRender::ResizePool(m_pTree->AsModel()->m_nPool,
                             m_pTree->AsModel()->m_nPoolSize + nLow4
                                 + (static_cast<short>(m_nVertexCount) + 1) * nOfs);
        unsigned int nGeomBase = m_pTree->AsModel()->m_nPoolSize + nLow4;
        m_nGeomBase = nGeomBase;
        m_pTree->AsModel()->m_nPoolSize += (static_cast<short>(m_nVertexCount) + 1) * nOfs + nLow4;

        char* pPool = static_cast<char*>(GLRender::LockPool(m_pTree->AsModel()->m_nPool, 0, 0, 1));
        m_paVertices = reinterpret_cast<float*>(new Vector3[static_cast<short>(m_nVertexCount) + 1]);

        for (int64_t i = 0; i < static_cast<short>(m_nVertexCount); i++) {
            NodeVertex* pVertex = reinterpret_cast<NodeVertex*>(PostProcess()->m_vertices.m_pData[i]);
            if (pFirst->m_nTexCoordFlags & kNodeVertexTex0) {
                *reinterpret_cast<float*>(pPool + i * nOfs + m_nTex0Ofs + nGeomBase) = pVertex->m_vTex0.x;
                *reinterpret_cast<float*>(pPool + i * nOfs + m_nTex0Ofs + 4 + nGeomBase) = pVertex->m_vTex0.y;
            }
            if (pFirst->m_nTexCoordFlags & kNodeVertexTex1) {
                *reinterpret_cast<float*>(pPool + i * nOfs + m_nTex1Ofs + nGeomBase) = pVertex->m_vTex1.x;
                *reinterpret_cast<float*>(pPool + i * nOfs + m_nTex1Ofs + 4 + nGeomBase) = pVertex->m_vTex1.y;
            }
            if (pFirst->m_nTexCoordFlags & kNodeVertexTex2) {
                *reinterpret_cast<float*>(pPool + i * nOfs + m_nTex2Ofs + nGeomBase) = pVertex->m_vTex2.x;
                *reinterpret_cast<float*>(pPool + i * nOfs + m_nTex2Ofs + 4 + nGeomBase) = pVertex->m_vTex2.y;
            }
            if (pFirst->m_nTexCoordFlags & kNodeVertexTex3) {
                *reinterpret_cast<float*>(pPool + i * nOfs + m_nTex3Ofs + nGeomBase) = pVertex->m_vTex3.x;
                *reinterpret_cast<float*>(pPool + i * nOfs + m_nTex3Ofs + 4 + nGeomBase) = pVertex->m_vTex3.y;
            }
            *reinterpret_cast<float*>(pPool + i * nOfs + m_nNormalOfs + nGeomBase) = pVertex->m_vNormal.x;
            *reinterpret_cast<float*>(pPool + i * nOfs + m_nNormalOfs + 4 + nGeomBase) = pVertex->m_vNormal.y;
            *reinterpret_cast<float*>(pPool + i * nOfs + m_nNormalOfs + 8 + nGeomBase) = pVertex->m_vNormal.z;
            *reinterpret_cast<float*>(pPool + i * nOfs + m_nVertexOfs + nGeomBase) = pVertex->m_vPos.x;
            *reinterpret_cast<float*>(pPool + i * nOfs + m_nVertexOfs + 4 + nGeomBase) = pVertex->m_vPos.y;
            *reinterpret_cast<float*>(pPool + i * nOfs + m_nVertexOfs + 8 + nGeomBase) = pVertex->m_vPos.z;
            reinterpret_cast<Vector*>(m_paVertices)[i] = pVertex->m_vPos;
        }

        memset(pPool + static_cast<short>(m_nVertexCount) * nOfs + m_nVertexOfs + nGeomBase,
               0, nOfs);
        Vector* pCap = reinterpret_cast<Vector*>(pPool + static_cast<short>(m_nVertexCount) * nOfs
                                 + m_nVertexOfs + nGeomBase);
        pCap->x = 1.0e7f;
        pCap->y = 1.0e7f;
        pCap->z = 1.0e7f;

        GLRender::UnlockPool(m_pTree->AsModel()->m_nPool);

        pMat->~Material();
        operator delete(pMat);
        if (bTangent)
            CalculateTangentSpaceBasis();
    }

    ComputeLocalSurfaceArea(m_faces, *reinterpret_cast<float*>(&m_fSurfaceArea));
    (*m_pfnRemoveTemporaryArrays)(this);
}

// Accumulates the local surface area of the given face list.
void MdlNodeTriMesh::ComputeLocalSurfaceArea(List<MaxFace>& faces, float& fArea)
{
    fArea = 0.0f;
    unsigned char* pLock = static_cast<unsigned char*>(GLRender::LockPool(m_pTree->AsModel()->m_nPool, 0, 0, 8));
    if (faces.m_count >= 1) {
        unsigned int nGeom = m_nGeomBase;
        for (int64_t i = 0; i < faces.m_count; i++) {
            MaxFace& face = faces.m_pData[i];
            Vector* pa = reinterpret_cast<Vector*>(pLock + nGeom + static_cast<short>(face.m_anVertIndices[0]) * m_nVertexStride);
            Vector* pb = reinterpret_cast<Vector*>(pLock + nGeom + static_cast<short>(face.m_anVertIndices[1]) * m_nVertexStride);
            Vector* pc = reinterpret_cast<Vector*>(pLock + nGeom + static_cast<short>(face.m_anVertIndices[2]) * m_nVertexStride);
            Vector dV2 = *pb - *pa;   // edge P0 -> P1
            Vector dV1 = *pc - *pa;   // edge P0 -> P2
            Vector v = cross(dV1, dV2);
            fArea += magnitude(v) * 0.5f;
        }
    }
    GLRender::UnlockPool(m_pTree->AsModel()->m_nPool);
}

// Frees the scratchpad's vertex array and the shared texcoord index lists, then the scratchpad
// itself.
void MdlNodeTriMesh::InternalRemoveTemporaryArrays()
{
    if (m_pPostProcess != 0) {
        for (int64_t i = 0; i < static_cast<MdlNodeTriMesh_PostProcess*>(m_pPostProcess)->m_vertices.m_count; i++) {
            if (static_cast<MdlNodeTriMesh_PostProcess*>(m_pPostProcess)->m_vertices.m_pData[i] != 0)
                delete static_cast<MdlNodeTriMesh_PostProcess*>(m_pPostProcess)->m_vertices.m_pData[i];
        }
        static_cast<MdlNodeTriMesh_PostProcess*>(m_pPostProcess)->m_vertices.SetSize(0);
    }
    MaxFace_PostProcess::texindices1.m_count = 0;
    MaxFace_PostProcess::texindices2.m_count = 0;
    MaxFace_PostProcess::texindices3.m_count = 0;
    if (m_pPostProcess != 0) {
        MdlNodeTriMesh_PostProcess* p = static_cast<MdlNodeTriMesh_PostProcess*>(m_pPostProcess);
        p->~MdlNodeTriMesh_PostProcess();
        operator delete(p);
    }
    m_pPostProcess = 0;
}


// Builds the pendant mesh's vertex buffer, keeping a pre-animation copy of the positions.
void MdlNodeDanglyMesh::InternalPostProcess()
{
    if (static_cast<short>(m_nVertexCount) > 0)
        return;

    ProcessVerts();

    if (m_faces.m_count > 0) {
        (*m_pfnGenVertices)(this);
        m_nVertexCount = static_cast<unsigned short>(PostProcess()->m_vertices.m_count);
        BuildNeighborLists();
        BuildVertexIndices();

        NodeVertex* pFirst = static_cast<short>(m_nVertexCount) >= 1
            ? reinterpret_cast<NodeVertex*>(PostProcess()->m_vertices.m_pData[0]) : 0;
#ifdef K2_FIX_ENGINE_BUGS
        if (pFirst == 0)
            return;
#endif

        m_nVertexStride = 0;
        m_nVertexOfs = 0;
        m_pVertexSource = static_cast<void*>(new Vector3[static_cast<short>(m_nVertexCount)]);
        m_nNormalOfs = 12;
        m_uVertexFlags |= MDX_VERTICES | MDX_VERTEX_NORMALS;
        int nOfs = 24;
        m_nVertexStride = nOfs;

        if (pFirst->m_nTexCoordFlags & kNodeVertexTex0) {
            m_nTex0Ofs = nOfs;
            nOfs += 8;
            m_nVertexStride = nOfs;
            m_nTexCoordSets++;
            m_uVertexFlags |= MDX_TEX0_VERTICES;
        }
        if (pFirst->m_nTexCoordFlags & kNodeVertexTex1) {
            m_nTex1Ofs = nOfs;
            nOfs += 8;
            m_nVertexStride = nOfs;
            m_nTexCoordSets++;
            m_uVertexFlags |= MDX_TEX1_VERTICES;
        }
        if (pFirst->m_nTexCoordFlags & kNodeVertexTex2) {
            m_nTex2Ofs = nOfs;
            nOfs += 8;
            m_nVertexStride = nOfs;
            m_nTexCoordSets++;
            m_uVertexFlags |= MDX_TEX2_VERTICES;
        }
        if (pFirst->m_nTexCoordFlags & kNodeVertexTex3) {
            m_nTex3Ofs = nOfs;
            nOfs += 8;
            m_nVertexStride = nOfs;
            m_nTexCoordSets++;
            m_uVertexFlags |= MDX_TEX3_VERTICES;
        }

        int nLow4 = m_pTree->AsModel()->m_nPoolSize & 0xf;
        GLRender::ResizePool(m_pTree->AsModel()->m_nPool,
                             m_pTree->AsModel()->m_nPoolSize + nLow4
                                 + (static_cast<short>(m_nVertexCount) + 1) * m_nVertexStride);
        m_nGeomBase = m_pTree->AsModel()->m_nPoolSize + nLow4;
        m_pTree->AsModel()->m_nPoolSize += (static_cast<short>(m_nVertexCount) + 1) * m_nVertexStride + nLow4;

        char* pPool = static_cast<char*>(GLRender::LockPool(m_pTree->AsModel()->m_nPool, 0, 0, 1));
        // Cached once: the pool sizing, the loop bound and the memset row all read the gate.
        int nVertsCount = static_cast<short>(m_nVertexCount);
        m_paVertices = reinterpret_cast<float*>(new Vector3[nVertsCount + 1]);

        int nStride = m_nVertexStride;
        unsigned int nGeom = m_nGeomBase;
        for (int64_t i = 0; i < nVertsCount; i++) {
            NodeVertex* pVertex = reinterpret_cast<NodeVertex*>(PostProcess()->m_vertices.m_pData[i]);
            if (pFirst->m_nTexCoordFlags & kNodeVertexTex0) {
                *reinterpret_cast<float*>(&pPool[nGeom + i * nStride + m_nTex0Ofs]) = pVertex->m_vTex0.x;
                *reinterpret_cast<float*>(&pPool[nGeom + i * nStride + m_nTex0Ofs + 4]) = pVertex->m_vTex0.y;
            }
            if (pFirst->m_nTexCoordFlags & kNodeVertexTex1) {
                *reinterpret_cast<float*>(&pPool[nGeom + i * nStride + m_nTex1Ofs]) = pVertex->m_vTex1.x;
                *reinterpret_cast<float*>(&pPool[nGeom + i * nStride + m_nTex1Ofs + 4]) = pVertex->m_vTex1.y;
            }
            if (pFirst->m_nTexCoordFlags & kNodeVertexTex2) {
                *reinterpret_cast<float*>(&pPool[nGeom + i * nStride + m_nTex2Ofs]) = pVertex->m_vTex2.x;
                *reinterpret_cast<float*>(&pPool[nGeom + i * nStride + m_nTex2Ofs + 4]) = pVertex->m_vTex2.y;
            }
            if (pFirst->m_nTexCoordFlags & kNodeVertexTex3) {
                *reinterpret_cast<float*>(&pPool[nGeom + i * nStride + m_nTex3Ofs]) = pVertex->m_vTex3.x;
                *reinterpret_cast<float*>(&pPool[nGeom + i * nStride + m_nTex3Ofs + 4]) = pVertex->m_vTex3.y;
            }
            *reinterpret_cast<float*>(&pPool[nGeom + i * nStride + m_nNormalOfs]) = pVertex->m_vNormal.x;
            *reinterpret_cast<float*>(&pPool[nGeom + i * nStride + m_nNormalOfs + 4]) = pVertex->m_vNormal.y;
            *reinterpret_cast<float*>(&pPool[nGeom + i * nStride + m_nNormalOfs + 8]) = pVertex->m_vNormal.z;
            *reinterpret_cast<float*>(&pPool[nGeom + i * nStride + m_nVertexOfs]) = pVertex->m_vPos.x;
            *reinterpret_cast<float*>(&pPool[nGeom + i * nStride + m_nVertexOfs + 4]) = pVertex->m_vPos.y;
            *reinterpret_cast<float*>(&pPool[nGeom + i * nStride + m_nVertexOfs + 8]) = pVertex->m_vPos.z;
            reinterpret_cast<Vector*>(m_paVertices)[i] = pVertex->m_vPos;
            Vector3* pSrc = &static_cast<Vector3*>(m_pVertexSource)[i];
            pSrc->x = pVertex->m_vPos.x;
            pSrc->y = pVertex->m_vPos.y;
            pSrc->z = pVertex->m_vPos.z;
        }

        memset(pPool + m_nGeomBase + nVertsCount * m_nVertexStride + m_nVertexOfs,
               0, m_nVertexStride);
        Vector* pCap = reinterpret_cast<Vector*>(pPool + m_nGeomBase + static_cast<short>(m_nVertexCount) * m_nVertexStride
                                 + m_nVertexOfs);
        pCap->x = 1.0e7f;
        pCap->y = 1.0e7f;
        pCap->z = 1.0e7f;

        GLRender::UnlockPool(m_pTree->AsModel()->m_nPool);
    }
    (*m_pfnRemoveTemporaryArrays)(this);
}

// Builds one dangly vertex per face corner, dedups it against the scratchpad's vertex array, and
// rebuilds the per-vertex constraint list in the same vertex order.
void MdlNodeDanglyMesh::InternalGenVertices()
{
    List<float> lConstraints;

    for (int64_t i = 0; i < static_cast<int64_t>(m_faces.m_count); i++) {
        for (int64_t j = 0; j < 3; j++) {
            int nIdx0 = PostProcess()->m_faces.m_pData[i].m_paVertIndices[j];
            Vector* pTex0 = PostProcess()->m_tverts.m_count
                            ? &PostProcess()->m_tverts.m_pData[PostProcess()->m_faces.m_pData[i].m_paTVertIndices[j]] : 0;
            Vector* pTex1 = PostProcess()->m_tverts1.m_count
                            ? &PostProcess()->m_tverts1.m_pData[PostProcess()->m_faces.m_pData[i].m_paTVertIndices[j + 3]] : 0;
            Vector* pTex2 = PostProcess()->m_tverts2.m_count
                            ? &PostProcess()->m_tverts2.m_pData[PostProcess()->m_faces.m_pData[i].m_paTVertIndices[j + 6]] : 0;
            Vector* pTex3 = PostProcess()->m_tverts3.m_count
                            ? &PostProcess()->m_tverts3.m_pData[PostProcess()->m_faces.m_pData[i].m_paTVertIndices[j + 9]] : 0;
            Vector* pNormal = &PostProcess()->m_normals.m_pData[PostProcess()->m_faces.m_pData[i].m_paNormalIndices[j]];

            int64_t index;
            NodeVertexDangly* pVertex = new NodeVertexDangly(
                &PostProcess()->m_verts.m_pData[nIdx0],
                pTex0, pTex1, pTex2, pTex3, pNormal,
                0,
                m_lConstraints.m_pData[nIdx0]);

            for (index = 0; index < static_cast<int64_t>(PostProcess()->m_vertices.m_count); index++) {
                NodeVertex* pOld = reinterpret_cast<NodeVertex*>(PostProcess()->m_vertices.m_pData[index]);
                if (*pOld->AsNodeVertexDangly() == *pVertex)
                    break;
            }
            if (index < static_cast<int64_t>(PostProcess()->m_vertices.m_count)) {
                delete pVertex;
                m_faces.m_pData[i].m_anVertIndices[j] = static_cast<unsigned short>(index);
            } else {
                m_faces.m_pData[i].m_anVertIndices[j] = static_cast<unsigned short>(PostProcess()->m_vertices.m_count);
                PostProcess()->m_vertices.Add(reinterpret_cast<unsigned int*>(pVertex));
                lConstraints.Add(m_lConstraints.m_pData[nIdx0]);
            }
        }
    }

    m_lConstraints.SetSize(0);
    for (int64_t i = 0; i < static_cast<int64_t>(lConstraints.m_count); i++)
        m_lConstraints.Add(lConstraints.m_pData[i]);
}

// Frees the scratchpad's vertex array, then the base's temporary arrays.
void MdlNodeDanglyMesh::InternalRemoveTemporaryArrays()
{
    for (int64_t i = 0; i < static_cast<MdlNodeTriMesh_PostProcess*>(m_pPostProcess)->m_vertices.m_count; i++) {
        unsigned int* pEntry = static_cast<MdlNodeTriMesh_PostProcess*>(m_pPostProcess)->m_vertices.m_pData[i];
        if (pEntry != 0)
            delete pEntry;
    }
    static_cast<MdlNodeTriMesh_PostProcess*>(m_pPostProcess)->m_vertices.SetSize(0);
    MdlNodeTriMesh::InternalRemoveTemporaryArrays();
}
