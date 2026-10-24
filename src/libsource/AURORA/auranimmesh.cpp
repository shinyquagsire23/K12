#include "k2stdint.h"
#if KOTOR1
#define K1_AURORA_LIST  // this TU is K1's AURORA library (List.h grows from 8)
#endif
#include "AURORA/aurmodel.h"
#include "auranim.h"
#include "aurglcore.h"
#include "aurprimitive.h"

// Blends this frame's animated vertices into the position and texcoord pools.
void MdlNodeAnimMesh::InternalControl(Part* pPart, float fA, float fB, float fC)
{
    PartAnimMesh* pAnimMesh = pPart->AsPartAnimMesh();

    int nFrame = 0;
    if (m_fSamplePeriod != 0.0f)
        nFrame = static_cast<int>(fB / m_fSamplePeriod);
    float fBlend = fB / m_fSamplePeriod;

    if (nFrame >= 0 && nFrame < m_nVertexFrameCount - 1) {
        VertexPrimitiveAnim* pPrim = pAnimMesh->m_pVertexPrimitive->AsVertexPrimitiveAnim();
        Vector3* pDst = static_cast<Vector3*>(GLRender::LockPool(pPrim->m_nVertexPool, 0, 0, 1));
        if (static_cast<short>(m_nVertexCount) >= 1) {
            float fFrac = fBlend - static_cast<float>(nFrame);
            float* pFrames = reinterpret_cast<float*>(m_paVertexFrames);
            int nCount = m_nVertexFrameCount;
            int64_t nVerts = static_cast<short>(m_nVertexCount);
            for (int64_t j = 0; j < nVerts; j++) {
                float* pF = pFrames;
                int i = static_cast<int>((nFrame + j * nCount) * 3);
                Vector3 vA = { pF[i], pF[i + 1], pF[i + 2] };  // raw-offset: float-granular frame index, as the original computes it
                Vector3 vB = { pF[i + 3], pF[i + 4], pF[i + 5] };  // raw-offset: float-granular frame index, as the original computes it
                Vector3 v = pDst[j];
                v.x = v.x * (1.0f - fC) + (vA.x * (1.0f - fFrac) + vB.x * fFrac) * fC;
                v.y = v.y * (1.0f - fC) + (vA.y * (1.0f - fFrac) + vB.y * fFrac) * fC;
                v.z = v.z * (1.0f - fC) + (vA.z * (1.0f - fFrac) + vB.z * fFrac) * fC;
                pDst[j] = v;
            }
        }
        GLRender::UnlockPool(pPrim->m_nVertexPool);
    } else if (m_nVertexFrameCount != 0) {
        VertexPrimitiveAnim* pPrim = pAnimMesh->m_pVertexPrimitive->AsVertexPrimitiveAnim();
        Vector3* pDst = static_cast<Vector3*>(GLRender::LockPool(pPrim->m_nVertexPool, 0, 0, 1));
        if (nFrame < 0)
            nFrame = 0;
        if (nFrame >= m_nVertexFrameCount)
            nFrame = m_nVertexFrameCount - 1;
        if (static_cast<short>(m_nVertexCount) >= 1) {
            for (int j = 0; j < static_cast<short>(m_nVertexCount); j++) {
                float* pF = reinterpret_cast<float*>(m_paVertexFrames);
                int i = (nFrame + j * m_nVertexFrameCount) * 3;
                Vector3 vA = { pF[i], pF[i + 1], pF[i + 2] };  // raw-offset: float-granular frame index, as the original computes it
                Vector3 v = pDst[j];
                v.x = v.x * (1.0f - fC) + vA.x * fC;
                v.y = v.y * (1.0f - fC) + vA.y * fC;
                v.z = v.z * (1.0f - fC) + vA.z * fC;
                pDst[j] = v;
            }
        }
        GLRender::UnlockPool(pPrim->m_nVertexPool);
    }

    if (m_nTexCoordFrameCount != 0) {
        VertexPrimitiveAnim* pPrim = pAnimMesh->m_pVertexPrimitive->AsVertexPrimitiveAnim();
        Vector2* pDst = static_cast<Vector2*>(GLRender::LockPool(pPrim->m_nTexCoordPool, 0, 0, 1));
        if (nFrame >= 0 && nFrame < m_nTexCoordFrameCount - 1) {
            if (static_cast<short>(m_nVertexCount) >= 1) {
                float fFrac = fB / m_fSamplePeriod - static_cast<float>(nFrame);
                float* pFrames = reinterpret_cast<float*>(m_paTexCoordFrames);
                int nCount = m_nTexCoordFrameCount;
                int64_t nVerts = static_cast<short>(m_nVertexCount);
                for (int64_t j = 0; j < nVerts; j++) {
                    float* pF = pFrames;
                    int i = static_cast<int>((nFrame + j * nCount) * 2);
                    Vector2 vA = { pF[i], pF[i + 1] };  // raw-offset: float-granular frame index, as the original computes it
                    Vector2 vB = { pF[i + 2], pF[i + 3] };  // raw-offset: float-granular frame index, as the original computes it
                    Vector2 v = pDst[j];
                    v.u = v.u * (1.0f - fC) + (vA.u * (1.0f - fFrac) + vB.u * fFrac) * fC;
                    v.v = v.v * (1.0f - fC) + (vA.v * (1.0f - fFrac) + vB.v * fFrac) * fC;
                    pDst[j] = v;
                }
            }
        } else if (m_nTexCoordFrameCount != 0) {
            if (nFrame < 0)
                nFrame = 0;
            if (nFrame >= m_nTexCoordFrameCount)
                nFrame = m_nTexCoordFrameCount - 1;
            if (static_cast<short>(m_nVertexCount) >= 1) {
                float* pFrames = reinterpret_cast<float*>(m_paTexCoordFrames);
                int nCount = m_nTexCoordFrameCount;
                int64_t nVerts = static_cast<short>(m_nVertexCount);
                for (int64_t j = 0; j < nVerts; j++) {
                    float* pF = pFrames;
                    int i = static_cast<int>((nFrame + j * nCount) * 2);
                    Vector2 vA = { pF[i], pF[i + 1] };  // raw-offset: float-granular frame index, as the original computes it
                    Vector2 v = pDst[j];
                    v.u = v.u * (1.0f - fC) + vA.u * fC;
                    v.v = v.v * (1.0f - fC) + vA.v * fC;
                    pDst[j] = v;
                }
            }
        }
        GLRender::UnlockPool(pPrim->m_nTexCoordPool);
    }

#if KOTOR1  // K1 inlines MdlNodeTriMesh::InternalControl, so its cast runs here first
    pPart->AsPartTriMesh();
    MdlNode::InternalControl(pPart, fA, fB, fC);
#else
    MdlNodeTriMesh::InternalControl(pPart, fA, fB, fC);
#endif
}
