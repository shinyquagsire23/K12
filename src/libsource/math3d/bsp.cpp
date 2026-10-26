// The scene BSP's free-function surface.

#include "k2stdint.h"
#define K1_AURORA_LIST  // K1's AURORA library copy
#include "bsp.h"
#include "math3d.h"

#include "port/libcmath.h"

// The split plane's normal. planelineintersection and dot take a `Vector const&` while the node
// stores a whole `Plane`, so the normal is read out of the plane's first three floats.
static inline const Vector& BSPPlaneNormal(const Plane& plane)
{
    return reinterpret_cast<const Vector&>(plane);
}

BSPNode::BSPNode(Vector vNormal, float fDist)
    : m_plane(vNormal.x, vNormal.y, vNormal.z, fDist),
      m_pBack(0),
      m_pFront(0),
      m_nType(0),
      m_pNodeData(0)
{
}

BSPNode::~BSPNode()
{
    delete m_pBack;
    delete m_pFront;
}

int RFaceSplitTest(List<Vector>& verts, const Plane& plane)
{
    int nMask = 0;
    for (int64_t i = 1; i <= verts.m_count; i++)
    {
        float fDist = dot(BSPPlaneNormal(plane), verts.m_pData[i - 1]) + plane.d;
        nMask |= fDist < 0.0f ? 1 : 2;
        if (nMask == 3)
            break;
    }
    return nMask;
}

int HitCheck(BSPNode* pNode, int nSide, Vector vStart, Vector vEnd, Vector* pvHit)
{
    if (!pNode || pNode->m_nType != 0)
    {
        if (nSide && pvHit)
            *pvHit = vStart;
        return nSide;
    }

    float fStart = dot(BSPPlaneNormal(pNode->m_plane), vStart) + pNode->m_plane.d;
    float fEnd = dot(BSPPlaneNormal(pNode->m_plane), vEnd) + pNode->m_plane.d;

    // Either endpoint in front; the straddling case means the plane is crossed.
    if (fStart > 0.0f || fEnd > 0.0f)
    {
        if (!(fStart > 0.0f && fEnd > 0.0f))
        {
            Vector vMid = planelineintersection(BSPPlaneNormal(pNode->m_plane), pNode->m_plane.d, vStart, vEnd);
            if (fStart > 0.0f)
            {
                if (HitCheck(pNode->m_pFront, 0, vStart, vMid, pvHit))
                    return 1;
                return HitCheck(pNode->m_pBack, 1, vMid, vEnd, pvHit);
            }
            if (HitCheck(pNode->m_pBack, 1, vStart, vMid, pvHit))
                return 1;
            return HitCheck(pNode->m_pFront, 0, vMid, vEnd, pvHit);
        }
        return HitCheck(pNode->m_pFront, 0, vStart, vEnd, pvHit);
    }
    return HitCheck(pNode->m_pBack, 1, vStart, vEnd, pvHit);
}

int BSPHitCheck(BSPNode* pNode, Vector vStart, Vector vEnd, Vector* pvHit,
                int (*pfnTest)(BSPNode*, Vector, Vector, Vector*))
{
    if (!pNode)
        return 0;

    // The per-node test writes its hit point straight back into `vEnd`, so the callback is handed its
    // own slot.
    int nHit = 0;
    if (pfnTest && pfnTest(pNode, vStart, vEnd, &vEnd))
    {
        *pvHit = vEnd;
        nHit = 1;
    }

    if (pNode->m_nType != 0)
    {
        // With a callback, a reported hit ends the walk whatever the node's type; without one, only a
        // type-1 leaf reports, at the segment's entry point.
        if (pfnTest)
            return nHit;
        if (pNode->m_nType != 1)
            return 0;
        if (!pvHit)
            return 1;
        *pvHit = vStart;
        return 1;
    }

    float fStart = dot(BSPPlaneNormal(pNode->m_plane), vStart) + pNode->m_plane.d;
    float fEnd = dot(BSPPlaneNormal(pNode->m_plane), vEnd) + pNode->m_plane.d;

    // Same straddle handling as HitCheck, but every descent is ORed with this node's own hit.
    if (fStart > 0.0f || fEnd > 0.0f)
    {
        if (!(fStart > 0.0f && fEnd > 0.0f))
        {
            Vector vMid = planelineintersection(BSPPlaneNormal(pNode->m_plane), pNode->m_plane.d, vStart, vEnd);
            if (fStart > 0.0f)
            {
                if (BSPHitCheck(pNode->m_pFront, vStart, vMid, pvHit, pfnTest))
                    return 1;
                return nHit | BSPHitCheck(pNode->m_pBack, vMid, vEnd, pvHit, pfnTest);
            }
            if (BSPHitCheck(pNode->m_pBack, vStart, vMid, pvHit, pfnTest))
                return 1;
            return nHit | BSPHitCheck(pNode->m_pFront, vMid, vEnd, pvHit, pfnTest);
        }
        return nHit | BSPHitCheck(pNode->m_pFront, vStart, vEnd, pvHit, pfnTest);
    }
    return nHit | BSPHitCheck(pNode->m_pBack, vStart, vEnd, pvHit, pfnTest);
}

void BSPTraverse(BSPNode* pNode, void (*pfnCallback)(BSPNode*, void*), void* pData)
{
    if (!pNode)
        return;

    pfnCallback(pNode, pData);
    BSPTraverse(pNode->m_pBack, pfnCallback, pData);
    BSPTraverse(pNode->m_pFront, pfnCallback, pData);
}

void BSPTraverse(BSPNode* pNode, Vector v,
                 void (*pfnCallback)(BSPNode*, void*, List<Plane>&), void* pData)
{
    if (!pNode)
        return;

    List<Plane> planes;
    pfnCallback(pNode, pData, planes);

    float fDist = dot(BSPPlaneNormal(pNode->m_plane), v) + pNode->m_plane.d;
    // The far side is walked first.
    BSPNode* pNear = fDist > 0.0f ? pNode->m_pFront : pNode->m_pBack;
    BSPNode* pFar = fDist > 0.0f ? pNode->m_pBack : pNode->m_pFront;

    BSPTraverse(pFar, v, pfnCallback, pData);
    BSPTraverse(pNear, v, pfnCallback, pData);
}

void BSPTraverse(BSPNode* pNode, const Vector& v, float fRadius,
                 void (*pfnCallback)(BSPNode*, const Vector&, float))
{
    if (!pNode)
        return;

    float fRoot = sqrtf(fRadius);

    do
    {
        pfnCallback(pNode, v, fRadius);

        float fDist = dot(v, BSPPlaneNormal(pNode->m_plane)) + pNode->m_plane.d;
        if (fDist > -fRoot)
            BSPTraverse(pNode->m_pFront, v, fRadius, pfnCallback);
        if (fDist < fRoot)
            pNode = pNode->m_pBack;
        else
            break;
    } while (pNode);
}

// Collapses `planes` (the ancestor candidates that still bound this subtree) onto each node's own
// two scratch lists, then descends the side `v` is on and only approximates the far side.
void BSPTraverseVolumeApprox(BSPNode* pNode, const Vector& v, List<Plane>& planes,
                             void (*pfnCallback)(BSPNode*, void*, List<Plane>&), void* pData)
{
    if (!pNode)
        return;

    pfnCallback(pNode, pData, planes);
    if (pNode->m_nType != 0)
        return;

    // Reset the scratch lists, keeping their buffers.
    pNode->m_lBackPlanes.m_count = 0;
    pNode->m_lFrontPlanes.m_count = 0;

    // One flag per scratch list, and they are crossed: a candidate that leaves the node entirely on
    // its positive side sets the first flag but is only recorded once the second is already known,
    // and the other way round. A candidate that bounds only one side records the opposite list.
    // Both flags set means the node's volume is fully bounded and the walk stops where it is.
    bool bBound40 = false;
    bool bBound50 = false;

    // Descending so that an Add which has to grow a list cannot invalidate the index for the rest
    // of the scan.
    for (int64_t i = planes.m_count - 1; i >= 0; i--)
    {
        int nMask = RFaceSplitTest(pNode->m_lVerts, planes.m_pData[i]);

        // A candidate straddling the node's own plane bounds both sides at once.
        if (nMask == 3)
        {
            pNode->m_lFrontPlanes.Add(planes.m_pData[i]);
            pNode->m_lBackPlanes.Add(planes.m_pData[i]);
        }

        float fDot = dot(BSPPlaneNormal(planes.m_pData[i]), BSPPlaneNormal(pNode->m_plane));

        if (nMask == 2)
        {
            if (fDot > 0.0f)
            {
                bBound40 = true;
                if (!bBound50)
                    pNode->m_lBackPlanes.Add(planes.m_pData[i]);
            }
            else
            {
                bBound50 = true;
                if (!bBound40)
                    pNode->m_lFrontPlanes.Add(planes.m_pData[i]);
            }
        }
        if (nMask == 1)
        {
            if (fDot > 0.0f)
            {
                if (!bBound40)
                    pNode->m_lFrontPlanes.Add(planes.m_pData[i]);
            }
            else
            {
                if (!bBound50)
                    pNode->m_lBackPlanes.Add(planes.m_pData[i]);
            }
        }
        if (bBound40 && bBound50)
            return;
    }

    float fDist = dot(v, BSPPlaneNormal(pNode->m_plane)) + pNode->m_plane.d;

    // The first side is the one `v` is on and gets the first recursion; the loop side is walked by
    // continuing this function.
    BSPNode* pFirst = fDist > 0.0f ? pNode->m_pFront : pNode->m_pBack;
    BSPNode* pLoop = fDist > 0.0f ? pNode->m_pBack : pNode->m_pFront;

    // Counts and lists are read straight off the node, so an Add that grows one list cannot
    // invalidate anything the other side still needs.
    if (!(fDist > 0.0f ? bBound40 : bBound50))
    {
        if ((fDist > 0.0f ? pNode->m_lFrontPlanes.m_count : pNode->m_lBackPlanes.m_count) != 0)
            BSPTraverseVolumeApprox(pFirst, v, fDist > 0.0f ? pNode->m_lFrontPlanes : pNode->m_lBackPlanes,
                                    pfnCallback, pData);
        else
            BSPTraverse(pFirst, v, pfnCallback, pData);
    }

    if (fDist > 0.0f ? bBound50 : bBound40)
        return;

    if ((fDist > 0.0f ? pNode->m_lBackPlanes.m_count : pNode->m_lFrontPlanes.m_count) == 0)
    {
        BSPTraverse(pLoop, v, pfnCallback, pData);
        return;
    }

    BSPTraverseVolumeApprox(pLoop, v,
                            fDist > 0.0f ? pNode->m_lBackPlanes : pNode->m_lFrontPlanes,
                            pfnCallback, pData);
}


BSPNode* BSPFitSphere(BSPNode* pNode, const Vector& v, float fRadius)
{
    while (pNode->m_nType == 0)
    {
        float fDist = dot(BSPPlaneNormal(pNode->m_plane), v) + pNode->m_plane.d;
        if (fDist <= -fRadius)
        {
            pNode = pNode->m_pBack;
        }
        else if (fDist >= fRadius)
        {
            pNode = pNode->m_pFront;
        }
        else
        {
            break;
        }
    }
    return pNode;
}

BSPNode* BSPFitBox(BSPNode* pNode, const Vector& vMin, const Vector& vMax)
{
    if (pNode->m_nType != 0)
        return pNode;

    // The box is tested against the negated plane first, because the most-negative plane value is
    // the farthest corner along the inverted normal.
    Plane planeNeg(-pNode->m_plane.a, -pNode->m_plane.b, -pNode->m_plane.c, -pNode->m_plane.d);
    if (BoxAbovePlane(vMin, vMax, planeNeg) >= -0.02f)
        return BSPFitBox(pNode->m_pBack, vMin, vMax);
    if (BoxAbovePlane(vMin, vMax, pNode->m_plane) >= -0.02f)
        return BSPFitBox(pNode->m_pFront, vMin, vMax);
    return pNode;
}

void BSPSphereCheck(BSPNode* pNode, const Vector& v, float fRadius, List<BSPNode*>& out)
{
    if (pNode->m_nType != 0)
    {
        out.Add(pNode);
        return;
    }

    float fDist = dot(BSPPlaneNormal(pNode->m_plane), v) + pNode->m_plane.d;
    if (fDist <= -fRadius)
    {
        BSPSphereCheck(pNode->m_pBack, v, fRadius, out);
    }
    else if (fDist >= fRadius)
    {
        BSPSphereCheck(pNode->m_pFront, v, fRadius, out);
    }
    else
    {
        BSPSphereCheck(pNode->m_pBack, v, fRadius, out);
        BSPSphereCheck(pNode->m_pFront, v, fRadius, out);
    }
}

BSPNode* BSPSearch(BSPNode* pNode, Vector vPoint, int (*pfnTest)(BSPNode*, void*), void* pData)
{
    for (;;)
    {
        if (pfnTest(pNode, pData))
            return pNode;
        if (pNode->m_nType != 0)
            return 0;

        float fDist = dot(BSPPlaneNormal(pNode->m_plane), vPoint) + pNode->m_plane.d;
        BSPNode* pNear = fDist < 0.0f ? pNode->m_pBack : pNode->m_pFront;
        pNode = fDist < 0.0f ? pNode->m_pFront : pNode->m_pBack;

        BSPNode* pFound = BSPSearch(pNear, vPoint, pfnTest, pData);
        if (pFound)
            return pFound;
    }
}

#include "AABB.h"

// Recursive segment-versus-box-tree hit test that reports the hit surface-index list instead of
// filling a hit-info block. An internal node recurses into both children and returns the sum of their
// counts; a leaf appends its own index and returns 1. The leaf arm tests no per-triangle mask,
// consults no traversal-direction heuristic and touches no hit-info block: it is the plain "which
// faces does this ray cross" collector.
//
// Nothing outside its own recursion references it.
//
// The box is inflated by 0.01f on each side before the test.
int HitCheckAABB(AABB* pAABB, Vector vA, Vector vB, List<int>& out)
{
    Vector vMin = pAABB->min - Vector(0.01f, 0.01f, 0.01f);
    Vector vMax = pAABB->max + Vector(0.01f, 0.01f, 0.01f);
    if (BoxIntersect(vA, vB, vMin, vMax, 0) == 0)
        return 0;

    if (pAABB->m_nIndex != -1)
    {
        out.Add(pAABB->m_nIndex);
        return 1;
    }
    return HitCheckAABB(pAABB->pChild0, vA, vB, out)
         + HitCheckAABB(pAABB->pChild1, vA, vB, out);
}
