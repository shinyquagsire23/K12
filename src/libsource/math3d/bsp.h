#ifndef BSP_H
#define BSP_H

#include "Vector.h"
#include "Plane.h"
#include "List.h"

// The per-leaf payload; not owned here.
struct nodedata;

// A BSP node in an Aurora scene, holding the split plane, the two children, a leaf flag and the
// node's world-space bounding box, followed by three scratch lists and the leaf payload. The box
// lives here rather than in CAurRoom because a derived class cannot place members inside its base.
struct BSPNode {
    Plane m_plane;
    BSPNode* m_pBack;
    BSPNode* m_pFront;
    int m_nType;
    Vector m_bboxMin;
    Vector m_bboxMax;

    List<Plane> m_lBackPlanes;
    List<Plane> m_lFrontPlanes;
    List<Vector> m_lVerts;

    nodedata* m_pNodeData;

    BSPNode(Vector vNormal, float fDist);
    ~BSPNode();
};

// Whether `verts` straddles `plane`: 1 = every vertex at or below it, 2 = every vertex at or above
// it, 3 = both (split).
int RFaceSplitTest(List<Vector>& verts, const Plane& plane);

// Walks the segment recursively without a per-node callback. A leaf reached through the
// `nSide == 1` (back) child reports a hit at the segment's entry point. `nSide` is the child's
// side, not the segment's, so every descent passes the constant for the child it is entering.
int HitCheck(BSPNode* pNode, int nSide, Vector vStart, Vector vEnd, Vector* pvHit);

// The same walk with the leaf test supplied as a callback.
int BSPHitCheck(BSPNode* pNode, Vector vStart, Vector vEnd, Vector* pvHit,
                int (*pfnTest)(BSPNode*, Vector, Vector, Vector*));

// Walks the tree in pre-order; the callback runs on every node, both children are recursed.
void BSPTraverse(BSPNode* pNode, void (*pfnCallback)(BSPNode*, void*), void* pData);

// Walks the half-spaces, descending the near child of `v` first, then the far one. Each node gets
// its own local plane list, handed fresh to the callback.
void BSPTraverse(BSPNode* pNode, Vector v,
                 void (*pfnCallback)(BSPNode*, void*, List<Plane>&), void* pData);

// Walks sphere-versus-half-space. `fRadius` is a squared radius, and the near child is entered when
// the node's plane value exceeds -sqrt(fRadius).
void BSPTraverse(BSPNode* pNode, const Vector& v, float fRadius,
                 void (*pfnCallback)(BSPNode*, const Vector&, float));

// Collapses `planes` down to each node's own two scratch lists, calling the callback on every node.
void BSPTraverseVolumeApprox(BSPNode* pNode, const Vector& v, List<Plane>& planes,
                             void (*pfnCallback)(BSPNode*, void*, List<Plane>&),
                             void* pData);

// Descend to the split node whose plane separates `v` from the tree, or stop at the node found.
BSPNode* BSPFitSphere(BSPNode* pNode, const Vector& v, float fRadius);
BSPNode* BSPFitBox(BSPNode* pNode, const Vector& vMin, const Vector& vMax);

// Append every leaf the sphere overlaps to `out`. A node straddling the sphere is descended on both
// sides.
void BSPSphereCheck(BSPNode* pNode, const Vector& v, float fRadius,
                    List<BSPNode*>& out);

// First node (near side first) for which `pfnTest` returns non-zero.
BSPNode* BSPSearch(BSPNode* pNode, Vector vPoint, int (*pfnTest)(BSPNode*, void*),
                   void* pData);

#endif // BSP_H
