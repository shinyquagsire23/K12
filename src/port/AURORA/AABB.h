#ifndef AURORA_AABB_H
#define AURORA_AABB_H

// A walkmesh/model bounding-box tree node. An internal node has m_nIndex -1 and two children; a
// leaf names its surface triangle. The destructor is per TU: AABB_DTOR_HEADER_BODY gives the
// recursive body, AABB_DTOR_DECLARED_ONLY declares it with that exception specification.
#include "Vector.h"

class AABB {
public:
    Vector min;
    Vector max;
    AABB*  pChild0;
    AABB*  pChild1;
    int    m_nIndex;
    int    m_nDir;

    // m_nDir: the axis the children's centres differ most along, and its sign, one bit each.
    enum {
        kDirPosX = 0x1,
        kDirPosY = 0x2,
        kDirPosZ = 0x4,
        kDirNegX = 0x8,
        kDirNegY = 0x10,
        kDirNegZ = 0x20,
    };

#ifdef AABB_DTOR_HEADER_BODY
    __attribute__((used)) ~AABB() { delete pChild0; delete pChild1; }
#elif defined(AABB_DTOR_DECLARED_ONLY)
    ~AABB() AABB_DTOR_DECLARED_ONLY;
#endif
};
K2_LAYOUT_ASSERT(sizeof(AABB) == 0x30, "sizeof pinned by CreateAABB's 0x600 pool advance / 32");

#endif // AURORA_AABB_H
