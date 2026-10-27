#include "k2stdint.h"
#include "swroomsurfacemesh.h"
#include "CExoArrayListImpl.h"
#include "restype.h"

// The ray walk's step limit through the triangle adjacency.
enum {
    RAY_WALK_STEP_LIMIT = 512,
};

// A vertex read from an ASCII walkmesh snaps to a whole unit when it lies within the 1/512 grid of
// one: the fraction bits below the shift are all zero.
enum {
    VERTEX_SNAP_SHIFT = 9,
    VERTEX_SNAP_MASK  = (1 << VERTEX_SNAP_SHIFT) - 1,
};

float checkaabbtime;
int g_bRenderClipWalkable;

#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/crtapi.h"
#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

// Parses the per-room header of a binary walkmesh: the edge-adjacency, box-tree and border-end
// arrays are views into the resource's data blob, each loaded at most once.
int CSWRoomSurfaceMesh::LoadMeshBinary(CResBWM* pRes)
{
    if (!CSWCollisionMesh::LoadMeshBinary(pRes))
        return 0;

    CResBWMRoomHeader* pHeader = reinterpret_cast<CResBWMRoomHeader*>(pRes->m_pParsed);
    int rootAABBIndex;
    CSWRoomSurfaceExternalEdge* pEdgeAdjacency;
    int edgeCount;
    if (pHeader == 0) {
        rootAABBIndex = 0;
        pEdgeAdjacency = 0;
        edgeCount = 0;
#if KOTOR1  // K1: the pointer is stored without updating the aligned flag
        m_paTriAdjacency.m_p = 0;
#else
        m_paTriAdjacency = 0;
#endif
    } else {
        char* pData = static_cast<char*>(pRes->m_pBWMData);
#if KOTOR1
        m_paTriAdjacency.m_p = reinterpret_cast<int*>(pData + pHeader->offTriAdjacency);
#else
        m_paTriAdjacency = reinterpret_cast<int*>(pData + pHeader->offTriAdjacency);
#endif
        edgeCount = pHeader->nEdgeCount;
        rootAABBIndex = pHeader->nRootAABBIndex;
        pEdgeAdjacency = reinterpret_cast<CSWRoomSurfaceExternalEdge*>(pData + pHeader->offEdgeAdjacency);
    }
    m_rootAABBIndex = rootAABBIndex;

    if (m_aEdgeAdjacency.m_count == 0) {
        m_aEdgeAdjacency.m_count = edgeCount;
        m_aEdgeAdjacency.m_capacity = edgeCount;
        m_aEdgeAdjacency.m_pArray = pEdgeAdjacency;
        m_aEdgeAdjacency.m_bStatic = 1;
    }

    CSWRoomSurfaceMeshAABBNode* pAABBNodes;
    int aabbCount;
    int existingAABBCount;
    if (pHeader == 0) {
        aabbCount = 0;
        pAABBNodes = 0;
        existingAABBCount = m_aAABBNodes.m_count;
    } else {
        aabbCount = pHeader->nAABBNodeCount;
        pAABBNodes = reinterpret_cast<CSWRoomSurfaceMeshAABBNode*>(static_cast<char*>(pRes->m_pBWMData) + pHeader->offAABBNodes);
        existingAABBCount = m_aAABBNodes.m_count;
    }
    if (existingAABBCount == 0) {
        m_aAABBNodes.m_pArray = pAABBNodes;
        m_aAABBNodes.m_count = aabbCount;
        m_aAABBNodes.m_capacity = aabbCount;
        m_aAABBNodes.m_bStatic = 1;
    }

    void* pBorderEnds;
    int nBorderEndCount;
    if (pHeader == 0) {
        pBorderEnds = 0;
        nBorderEndCount = 0;
    } else {
        nBorderEndCount = pHeader->nBorderEndCount;
        pBorderEnds = static_cast<char*>(pRes->m_pBWMData) + pHeader->offBorderEnds;
    }
    if (m_aBorderEnds.m_count == 0) {
        m_aBorderEnds.m_pArray = static_cast<int*>(pBorderEnds);
        m_aBorderEnds.m_count = nBorderEndCount;
        m_aBorderEnds.m_capacity = nBorderEndCount;
        m_aBorderEnds.m_bStatic = 1;
    }
    return 1;
}

// Appends this room's arrays to the BWM file the base SaveBinaryMesh wrote, backpatching the
// header's count and offset fields.
int CSWRoomSurfaceMesh::SaveBinaryMesh(const CExoString& fileName)
{
    int result = CSWCollisionMesh::SaveBinaryMesh(fileName);
    if (result == 1) {
#if KOTOR1  // K1: the open goes through the port's tracing _fopen
        FILE* pFile = _fopen("E:/Work/aspyr/KOTOR_Android/GooglePlay/app/src/main/cpp/src/game/servercore/swroomsurfacemesh.cpp",
                             "SaveBinaryMesh", 251, fileName.CStr(), "rb+");
#else
        FILE* pFile = fopen(fileName.CStr(), "rb+");
#endif
        CResBWMRoomHeader header;
        memset(header.transformBlock, 0, sizeof(header.transformBlock));
        fread(&header, sizeof(header), 1, pFile);
        fseek(pFile, 0, SEEK_END);
        unsigned int aabbCount = m_aAABBNodes.m_count;
        header.nAABBNodeCount = aabbCount;
        header.offAABBNodes = static_cast<unsigned int>(ftell(pFile));
        header.nRootAABBIndex = m_rootAABBIndex;
        fwrite(m_aAABBNodes.m_pArray, sizeof(CSWRoomSurfaceMeshAABBNode), aabbCount, pFile);
        header.offTriAdjacency = static_cast<unsigned int>(ftell(pFile));
        fwrite(m_paTriAdjacency, sizeof(int), header.nTriAdjacencyCount * 3, pFile);
        unsigned int edgeCount = m_aEdgeAdjacency.m_count;
        header.nEdgeCount = edgeCount;
        header.offEdgeAdjacency = static_cast<unsigned int>(ftell(pFile));
        fwrite(m_aEdgeAdjacency.m_pArray, sizeof(CSWRoomSurfaceExternalEdge), edgeCount, pFile);
        unsigned int nBorderEndCount = m_aBorderEnds.m_count;
        header.nBorderEndCount = nBorderEndCount;
        header.offBorderEnds = static_cast<unsigned int>(ftell(pFile));
        fwrite(m_aBorderEnds.m_pArray, sizeof(int), nBorderEndCount, pFile);
        fseek(pFile, 0, SEEK_SET);
        fwrite(&header, sizeof(header), 1, pFile);
        fclose(pFile);
    }
    return result;
}

// Builds the material masks by scanning the surface-material 2DA's line-of-sight, walk and
// walk-check columns.
CSWRoomSurfaceMesh::CSWRoomSurfaceMesh()
    : CSWCollisionMesh(RESTYPE_MDL, RESTYPE_WOK), m_aEdgeAdjacency(0), m_aBorderEnds(0), m_aAABBNodes(0)
{
    m_paTriAdjacency = 0;
    m_aAABBNodes = CExoStaticArrayList<CSWRoomSurfaceMeshAABBNode>(0);
    m_walkCheckMaterialMask = 0;
    m_allMaterialsMask = 0;
    m_losMaterialMask = 0;
    m_walkableMaterialMask = 0;

    for (int row = 0; row != 32; row++) {
        CExoString label;

        C2DA* pTable = g_pRules->m_p2DArrays->m_pSurfaceMaterial;
        {
            CExoString colLabel("Label");
            pTable->GetCExoStringEntry(row, colLabel, &label);
        }

        int value;
        pTable = g_pRules->m_p2DArrays->m_pSurfaceMaterial;
        {
            CExoString colLOS("LineOfSight");
            pTable->GetINTEntry(row, colLOS, &value);
        }
        unsigned int bit = 1 << row;
        if (value == 1)
            m_losMaterialMask += bit;

        pTable = g_pRules->m_p2DArrays->m_pSurfaceMaterial;
        {
            CExoString colWalk("Walk");
            pTable->GetINTEntry(row, colWalk, &value);
        }
        if (value == 1)
            m_walkableMaterialMask += bit;

        pTable = g_pRules->m_p2DArrays->m_pSurfaceMaterial;
        {
            CExoString colWalkCheck("WalkCheck");
            pTable->GetINTEntry(row, colWalkCheck, &value);
        }
        if (value == 1)
            m_walkCheckMaterialMask += bit;

        m_allMaterialsMask += bit;
    }
}

unsigned int CSWRoomSurfaceMesh::GetWalkCheckMaterialMask()
{
    return m_walkCheckMaterialMask;
}

unsigned int CSWRoomSurfaceMesh::GetLOSMaterialMask()
{
    return m_losMaterialMask;
}

unsigned int CSWRoomSurfaceMesh::GetWalkableMaterialMask()
{
    return m_walkableMaterialMask;
}

unsigned int CSWRoomSurfaceMesh::GetAllMaterialsMask()
{
    return m_allMaterialsMask;
}

int CSWRoomSurfaceMesh::GetAdjacentRoom(int edgeIdx)
{
    return m_aEdgeAdjacency[edgeIdx].room;
}

int CSWRoomSurfaceMesh::GetAdjacentEdge(int edgeIdx)
{
    return m_aEdgeAdjacency[edgeIdx].edge;
}

void CSWRoomSurfaceMesh::GetTriangleVertices(int unused, int triangle, int* a, int* b, int* c)
{
    K2_UNUSED(unused);
    if (triangle < m_nTriangleCount) {
#if KOTOR1  // K1: each element indexed on its own 32-bit index
        unsigned int base = triangle * 3;
        *a = m_paTriVertIndices[static_cast<int>(base)];
        *b = m_paTriVertIndices[static_cast<int>(base + 1)];
        *c = m_paTriVertIndices[static_cast<int>(base + 2)];
#else
        int n = triangle * 3;
        *a = m_paTriVertIndices[n];
        *b = m_paTriVertIndices[n + 1];
        *c = m_paTriVertIndices[n + 2];
#endif
    }
}

int CSWRoomSurfaceMesh::GetEdgeVertex(int edgeIdx, Vector& out)
{
    if (edgeIdx < m_aEdgeAdjacency.m_count) {
#if KOTOR1  // K1: the index and the vertex are read through the aligned-array accessors
        int vertIdx = m_paTriVertIndices.Get(m_aEdgeAdjacency[edgeIdx].edge);
        out = m_paVertices.Get(vertIdx);
#else
        int vertIdx = m_paTriVertIndices[m_aEdgeAdjacency[edgeIdx].edge];
        out = m_paVertices[vertIdx];
#endif
        return 1;
    }
    out.x = 0.0f;
    out.y = 0.0f;
    out.z = 0.0f;
    return 0;
}

// EFFECTIVE (K1): the vertex/index loads and the two stores of v are scheduled in another order (x9/x10
// rename); same loads and stores, armv7 -O3 exact.
// EFFECTIVE: PERM, -g re-probed v2259: K2 arm in K1, SetAt, the index through Get, a named edge local.
int CSWRoomSurfaceMesh::SetEdgeVertex(int edgeIdx, Vector& v)
{
    if (edgeIdx < m_aEdgeAdjacency.m_count) {
        int vertIdx = m_paTriVertIndices[m_aEdgeAdjacency[edgeIdx].edge];
#if KOTOR1  // K1: written through the aligned-array accessor
        m_paVertices.Set(vertIdx, v);
#else
        m_paVertices[vertIdx] = v;
#endif
        return 1;
    }
    return 0;
}

// Answers the edge's two endpoints, this edge's own vertex and the next edge's vertex, which wraps
// to 0 past the last edge.
int CSWRoomSurfaceMesh::GetEdgeVertices(int edgeIdx, Vector& out0, Vector& out1)
{
    if (edgeIdx < m_aEdgeAdjacency.m_count) {
        int nextIdx = (edgeIdx + 1 == m_aEdgeAdjacency.m_count) ? 0 : edgeIdx + 1;
#if KOTOR1  // K1: the indices and both vertices are read through the aligned-array accessors
        int vertIdx0 = m_paTriVertIndices.Get(m_aEdgeAdjacency[edgeIdx].edge);
        out0 = m_paVertices.Get(vertIdx0);
        int vertIdx1 = m_paTriVertIndices.Get(m_aEdgeAdjacency[nextIdx].edge);
        out1 = m_paVertices.Get(vertIdx1);
#else
        int vertIdx0 = m_paTriVertIndices[m_aEdgeAdjacency[edgeIdx].edge];
        out0 = m_paVertices[vertIdx0];
        int vertIdx1 = m_paTriVertIndices[m_aEdgeAdjacency[nextIdx].edge];
        out1 = m_paVertices[vertIdx1];
#endif
        return 1;
    }
    out0.x = 0.0f;
    out0.y = 0.0f;
    out0.z = 0.0f;
    out1.x = 0.0f;
    out1.y = 0.0f;
    out1.z = 0.0f;
    return 0;
}

// Converts each of the triangle's three directed-edge ids (or -1) into an adjacent triangle
// index.
void CSWRoomSurfaceMesh::GetTriangleAdjacency(int unused, int triangle, int* a, int* b, int* c)
{
    K2_UNUSED(unused);
#if KOTOR1  // K1: the bound is m_nTriangleCount, and an out-of-range triangle writes nothing
    if (triangle < m_nTriangleCount && m_paTriAdjacency != 0) {
        unsigned int base = triangle * 3;
        int e0 = m_paTriAdjacency[static_cast<int>(base)];
        *a = (e0 == -1) ? -1 : e0 / 3;
        int e1 = m_paTriAdjacency[static_cast<int>(base + 1)];
        *b = (e1 == -1) ? -1 : e1 / 3;
        int e2 = m_paTriAdjacency[static_cast<int>(base + 2)];
        *c = (e2 == -1) ? -1 : e2 / 3;
    }
#else
    if (triangle >= m_nWalkableTriangleCount) {
        *c = -1;
        *b = -1;
        *a = -1;
        return;
    }
    if (m_paTriAdjacency == 0)
        return;
    int n = triangle * 3;
    int e0 = m_paTriAdjacency[n];
    *a = (e0 == -1) ? -1 : e0 / 3;
    int e1 = m_paTriAdjacency[n + 1];
    *b = (e1 == -1) ? -1 : e1 / 3;
    int e2 = m_paTriAdjacency[n + 2];
    *c = (e2 == -1) ? -1 : e2 / 3;
#endif
}

// Walks the box tree recursively, baking the world offset into both box corners of every node.
void CSWRoomSurfaceMesh::TransformAABBToWorld(int nodeIndex, const Vector& offset)
{
    while (nodeIndex != -1) {
        m_aAABBNodes[nodeIndex].corner1 = m_aAABBNodes[nodeIndex].corner1 + m_localOrigin + offset;
        m_aAABBNodes[nodeIndex].corner0 = m_aAABBNodes[nodeIndex].corner0 + m_localOrigin + offset;
        TransformAABBToWorld(m_aAABBNodes[nodeIndex].child1, offset);
        nodeIndex = m_aAABBNodes[nodeIndex].child2;
    }
}

void CSWRoomSurfaceMesh::TransformToWorld(const Vector& offset)
{
    if (m_bBinaryBacked == 0) {
        CSWCollisionMesh::TransformToWorld(offset);
        if (m_aAABBNodes.m_count != 0) {
            TransformAABBToWorld(m_rootAABBIndex, offset);
        }
    }
}

// The height of the given triangle's plane over the point's x and y.
float CSWRoomSurfaceMesh::ComputeHeight(int triangle, Vector point)
{
#if KOTOR1  // K1: the normal and plane D are read through the aligned-array accessors
    return PlaneHeightAtPoint(m_paTriNormals.Get(triangle), m_paTriPlaneD.Get(triangle),
                              Vector(point.x, point.y, -9.0f));
#else
    return PlaneHeightAtPoint(m_paTriNormals[triangle], m_paTriPlaneD[triangle], Vector(point.x, point.y, -9.0f));
#endif
}

float CSWRoomSurfaceMesh::ComputeHeight(Vector point, int flags)
{
    Vector dir = {1.0f, 1.0f, 0.0f};
    int triangle = FindTriangle(point, flags, dir);
    if (triangle == -1)
        return -9.0f;
#if KOTOR1
    return PlaneHeightAtPoint(m_paTriNormals.Get(triangle), m_paTriPlaneD.Get(triangle),
                              Vector(point.x, point.y, -9.0f));
#else
    return PlaneHeightAtPoint(m_paTriNormals[triangle], m_paTriPlaneD[triangle], Vector(point.x, point.y, -9.0f));
#endif
}

// Box-tree walk, pruned against each node's box. A leaf tests its triangle's material bit
// against the hit info's mask and, on a hit, records the triangle index and material bit.
int CSWRoomSurfaceMesh::CheckAABBNode(int nodeIndex, const Vector& boxMin, Vector& boxMax, CSWRoomSurfaceMeshHitInfo& hitInfo)
{
    if (!BoxIntersect(boxMin, boxMax, m_aAABBNodes[nodeIndex].corner0, m_aAABBNodes[nodeIndex].corner1))
        return 0;

    int leafTriangleIndex = m_aAABBNodes[nodeIndex].leafTriangleIndex;
    if (leafTriangleIndex != -1) {
        unsigned int materialBit = 1 << m_paTriMaterials[leafTriangleIndex];
        unsigned int materialMask = hitInfo.m_materialMask;
        if ((materialMask & materialBit) == 0)
            return 0;

        Vector triVerts[3] = {};
        Vector outNormal = {0.0f, 0.0f, 0.0f};
#if KOTOR1  // K1: 32-bit index; the vertices are read through the aligned-array accessor
        int* pIndices = &m_paTriVertIndices[leafTriangleIndex * 3];
        triVerts[0] = m_paVertices.Get(pIndices[0]);
        triVerts[1] = m_paVertices.Get(pIndices[1]);
        triVerts[2] = m_paVertices.Get(pIndices[2]);
#else
        int64_t triIndex64 = leafTriangleIndex;
        int* pIndices = &m_paTriVertIndices[triIndex64 * 3];
        triVerts[0] = m_paVertices[pIndices[0]];
        triVerts[1] = m_paVertices[pIndices[1]];
        triVerts[2] = m_paVertices[pIndices[2]];
#endif
        if (!polyhit(triVerts, 3, boxMin, boxMax, &boxMax, &outNormal))
            return 0;

        hitInfo.m_triangleIndex = leafTriangleIndex;
        hitInfo.m_materialBit = materialBit;
        return 1;
    }

    int total;
    if ((hitInfo.m_sideSelectMask & m_aAABBNodes[nodeIndex].sideMask) == 0) {
        total = CheckAABBNode(m_aAABBNodes[nodeIndex].child2, boxMin, boxMax, hitInfo);
        total += CheckAABBNode(m_aAABBNodes[nodeIndex].child1, boxMin, boxMax, hitInfo);
    } else {
        total = CheckAABBNode(m_aAABBNodes[nodeIndex].child1, boxMin, boxMax, hitInfo);
        total += CheckAABBNode(m_aAABBNodes[nodeIndex].child2, boxMin, boxMax, hitInfo);
    }
    return total;
}

#if !KOTOR1  // K1: CheckAABBAll is not timed
#include "port/win32api.h"

// Engine microsecond clock; the counter frequency is read once into a function-local static.
static long long QueryFrequency()
{
    long long freq;
    QueryPerformanceFrequency(&freq);
    return freq;
}

static long long GetElapsedMicroseconds()
{
    static long long s_nFrequency = QueryFrequency();
    long long count;
    QueryPerformanceCounter(&count);
    return count * 1000000 / s_nFrequency;
}
#endif

// Traces the query box against the tree using every material. A miss re-searches with the box nudged
// along the normalized {1,1,0}; the elapsed time goes to the checkaabbtime profiling global.
int CSWRoomSurfaceMesh::CheckAABBAll(Vector boxMin, Vector boxMax, CSWRoomSurfaceMeshHitInfo& hitInfo)
{
    if (m_aAABBNodes.m_count == 0)
        return 0;

#if !KOTOR1
    long long startTime = GetElapsedMicroseconds();
#endif
    hitInfo.m_materialMask = m_allMaterialsMask;
    int count = CheckAABBNode(m_rootAABBIndex, boxMin, boxMax, hitInfo);
    if (hitInfo.m_triangleIndex == -1) {
        Vector dir;
        dir.x = 1.0f;
        dir.y = 1.0f;
        dir.z = 0.0f;
        Vector normDir = normalize(dir);
        boxMin.x = normDir.x * 0.001f + boxMin.x;
        boxMin.y = normDir.y * 0.001f + boxMin.y;
        boxMin.z = normDir.z * 0.001f + boxMin.z;
        boxMax.x = normDir.x * 0.001f + boxMax.x;
        boxMax.y = normDir.y * 0.001f + boxMax.y;
        boxMax.z = normDir.z * 0.001f + boxMax.z;
        count = CheckAABBNode(m_rootAABBIndex, boxMin, boxMax, hitInfo);
    }
    if (count > 0) {
        hitInfo.m_bHit = 1;
        hitInfo.m_point = boxMax;
    }
#if !KOTOR1
    long long endTime = GetElapsedMicroseconds();
    checkaabbtime = static_cast<float>(endTime - startTime);
#endif
    return hitInfo.m_bHit;
}

// Same search as CheckAABBAll, but against the walk-check material mask and without the timer.
int CSWRoomSurfaceMesh::CheckAABBWalkable(Vector boxMin, Vector boxMax, CSWRoomSurfaceMeshHitInfo& hitInfo)
{
    if (m_aAABBNodes.m_count == 0)
        return 0;

    hitInfo.m_materialMask = m_walkCheckMaterialMask;
    int count = CheckAABBNode(m_rootAABBIndex, boxMin, boxMax, hitInfo);
    if (hitInfo.m_triangleIndex == -1) {
        Vector dir;
        dir.x = 1.0f;
        dir.y = 1.0f;
        dir.z = 0.0f;
        Vector normDir = normalize(dir);
        boxMin.x = normDir.x * 0.001f + boxMin.x;
        boxMin.y = normDir.y * 0.001f + boxMin.y;
        boxMin.z = normDir.z * 0.001f + boxMin.z;
        boxMax.x = normDir.x * 0.001f + boxMax.x;
        boxMax.y = normDir.y * 0.001f + boxMax.y;
        boxMax.z = normDir.z * 0.001f + boxMax.z;
        count = CheckAABBNode(m_rootAABBIndex, boxMin, boxMax, hitInfo);
    }
    if (count > 0) {
        hitInfo.m_bHit = 1;
        hitInfo.m_point = boxMax;
    }
    return hitInfo.m_bHit;
}

// The walkability ray-cast for a segment through this room, after clipping it to the boundary
// polygon: the segment walks the triangle-adjacency graph toward its end, tracking the min and max
// walk-surface z in tStart and tEnd and classifying the end point against each visited triangle's
// three edges. A crossing moves the walk to the adjacent triangle; leaving through an exterior edge
// is success.
int CSWRoomSurfaceMesh::ClippedLineSegmentWalkable(const Vector& segStart, const Vector& segEnd,
                                                   Vector& clipStart, Vector& clipEnd,
                                                   float& tStart, float& tEnd, int& outFlags)
{
    int ret = ComputeClippedLineSegment(segStart, segEnd, clipStart, clipEnd, outFlags);
    if (ret == 0)
        return 0;

    Vector triVerts[3] = {};
    Vector hitPoint = {};
    Vector normal = {};
    Vector segStartLocal = segStart;
    Vector segEndLocal = segEnd;

    int tri = FindTriangle(segStartLocal, 1,
                           {segEnd.x - segStart.x, segEnd.y - segStart.y, segEnd.z - segStart.z});
    if (tri < 0)
        return 0;

    int v0, v1, v2;
    GetTriangleVertices(0, tri, &v0, &v1, &v2);
    GetVertex(v0, &triVerts[0]);
    GetVertex(v1, &triVerts[1]);
    GetVertex(v2, &triVerts[2]);

    Vector segA = {segStartLocal.x, segStartLocal.y, 1000.0f};
    Vector segB = {segStartLocal.x, segStartLocal.y, -1000.0f};
    if (polyhit(triVerts, 3, segA, segB, &hitPoint, &normal)) {
        segStartLocal.z = hitPoint.z;
        clipStart.z = hitPoint.z;
    }

    int iterCount = 0;
    do {
        int adj[3];
        GetTriangleVertices(0, tri, &v0, &v1, &v2);
        GetTriangleAdjacency(0, tri, &adj[0], &adj[1], &adj[2]);
        float vx[3], vy[3], vz[3];
        GetVertex(v0, &vx[0], &vy[0], &vz[0]);
        GetVertex(v0, &triVerts[0]);
        GetVertex(v1, &vx[1], &vy[1], &vz[1]);
        GetVertex(v1, &triVerts[1]);
        GetVertex(v2, &vx[2], &vy[2], &vz[2]);
        GetVertex(v2, &triVerts[2]);

        if (iterCount == 0) {
            tEnd = segStartLocal.z;
            tStart = segStartLocal.z;
        } else if (segStartLocal.z >= tEnd) {
            tEnd = segStartLocal.z;
        } else if (segStartLocal.z <= tStart) {
            tStart = segStartLocal.z;
        }

        int side[3];
        int count = 0;
        side[0] = ((vx[1] - vx[0]) * (segEnd.y - vy[0]) +
                   (vy[0] - vy[1]) * (segEnd.x - vx[0])) >= 0.0f;
        count += side[0];
        side[1] = ((vx[2] - vx[1]) * (segEnd.y - vy[1]) +
                   (vy[1] - vy[2]) * (segEnd.x - vx[1])) >= 0.0f;
        count += side[1];
        side[2] = ((vx[0] - vx[2]) * (segEnd.y - vy[2]) +
                   (vy[2] - vy[0]) * (segEnd.x - vx[2])) >= 0.0f;
        count += side[2];

        switch (count) {
        case 0:
            return 0;
        case 1: {
            Vector isect = {};
            int found = 0;
            for (int i = 2; i >= 0 && !found; i--) {
                if (side[i] == 0) {
                    if (IntersectLineSegments2d(segStartLocal, segEndLocal, triVerts[i],
                                                triVerts[(i + 1) % 3], isect) == 1) {
                        tri = adj[i];
                        segStartLocal = isect;
                        found = 1;
                    }
                }
            }
            if (!found)
                return 0;
            break;
        }
        case 2: {
            Vector isect = {};
            if (side[2] == 0) {
                if (!IntersectLineSegments2d(segStartLocal, segEndLocal, triVerts[2],
                                             triVerts[0], isect))
                    return 0;
                tri = adj[2];
                segStartLocal = isect;
            }
            if (side[1] == 0) {
                if (!IntersectLineSegments2d(segStartLocal, segEndLocal, triVerts[1],
                                             triVerts[2], isect))
                    return 0;
                tri = adj[1];
                segStartLocal = isect;
            }
            if (side[0] == 0) {
                if (!IntersectLineSegments2d(segStartLocal, segEndLocal, triVerts[0],
                                             triVerts[1], isect))
                    return 0;
                tri = adj[0];
                segStartLocal = isect;
            }
            break;
        }
        case 3: {
            Vector segA2 = {segEndLocal.x, segEndLocal.y, 1000.0f};
            Vector segB2 = {segEndLocal.x, segEndLocal.y, -1000.0f};
            if (polyhit(triVerts, 3, segA2, segB2, &hitPoint, &normal)) {
                segEndLocal.z = hitPoint.z;
                clipEnd.z = hitPoint.z;
            }
            if (segEndLocal.z >= tEnd) {
                tEnd = segEndLocal.z;
            } else if (segEndLocal.z <= tStart) {
                tStart = segEndLocal.z;
            }
            return 1;
        }
        }
    } while (tri >= 0 && ++iterCount < RAY_WALK_STEP_LIMIT);

    if (iterCount >= RAY_WALK_STEP_LIMIT - 1) {
        CExoString s;
        m_resRef.CopyToString(s);
        s.CStr();
        s.CStr();
        return 0;
    }
    if (tri == -1) {
        if (segStartLocal.z >= tEnd) {
            tEnd = segStartLocal.z;
        } else if (segStartLocal.z <= tStart) {
            tStart = segStartLocal.z;
        }
        return 1;
    }
    return 0;
}

// Clips the segment against this room's boundary polygon, writing the clipped endpoints. Both
// endpoints are flattened to 2D first, and a degenerate segment is nudged 1.0f along y before the
// cast. IsPointInPolygon fills the projection-sorted crossing list and returns its count: an odd
// count means the segment starts inside, so clipStart is unchanged and clipEnd is the first
// crossing, while an even count puts the inside span between the first two crossings.
int CSWRoomSurfaceMesh::ComputeClippedLineSegment(Vector segStart, Vector segEnd,
                                                  Vector& clipStart, Vector& clipEnd,
                                                  int& outFlags)
{
    Vector a2 = {segStart.x, segStart.y, 0.0f};
    Vector b2 = {segEnd.x, segEnd.y, 0.0f};

    if (equal(a2, b2)) {
        b2.y += 1.0f;
        int ret = IsPointInPolygon(a2, b2, outFlags) & 1;
        clipStart = segStart;
        clipEnd = segEnd;
        return ret;
    }

    int ret = IsPointInPolygon(a2, b2, outFlags);
    if (ret == 0) {
        clipStart = segStart;
        clipEnd = segEnd;
        return 0;
    }

    float d0 = (b2.x - a2.x) * (b2.x - a2.x) + (b2.y - a2.y) * (b2.y - a2.y) +
               (b2.z - a2.z) * (b2.z - a2.z);
    float d1 = (m_aCrossingPoints[0].x - a2.x) * (m_aCrossingPoints[0].x - a2.x) +
               (m_aCrossingPoints[0].y - a2.y) * (m_aCrossingPoints[0].y - a2.y) +
               (0.0f - a2.z) * (0.0f - a2.z);

    if (ret & 1) {
        clipStart = segStart;
        if (d0 < d1) {
            clipEnd = segEnd;
        } else {
            clipEnd = m_aCrossingPoints[0];
        }
        return 1;
    }

    if (d0 < d1) {
        clipStart = segStart;
        clipEnd = segEnd;
        return 0;
    }

    float d2 = (m_aCrossingPoints[1].x - a2.x) * (m_aCrossingPoints[1].x - a2.x) +
               (m_aCrossingPoints[1].y - a2.y) * (m_aCrossingPoints[1].y - a2.y) +
               (0.0f - a2.z) * (0.0f - a2.z);
    clipStart = m_aCrossingPoints[0];
    if (d0 < d2) {
        clipEnd = segEnd;
    } else {
        clipEnd = m_aCrossingPoints[1];
    }
    return 1;
}

// Locates which triangle contains the point: converts to local space, then searches a thin vertical
// box at the point's XY, retrying once after a nudge along dir. Returns the triangle index, or -1.
int CSWRoomSurfaceMesh::FindTriangle(const Vector& point, int flags, Vector dir)
{
    Vector localPoint = WorldToLocal(point);
    CSWRoomSurfaceMeshHitInfo hitInfo(CSWRoomSurfaceMeshHitInfo::MATERIAL_MASK_ALL);

    Vector boxMin = {localPoint.x, localPoint.y, 1000.0f};
    Vector boxMax = {localPoint.x, localPoint.y, -1000.0f};
    if (flags == 0)
        CheckAABBAll(boxMin, boxMax, hitInfo);
    else
        CheckAABBWalkable(boxMin, boxMax, hitInfo);

    if (hitInfo.m_triangleIndex != -1)
        return hitInfo.m_triangleIndex;
    Vector normDir = normalize(dir);
    float x = localPoint.x + normDir.x * 0.001f;
    float y = localPoint.y + normDir.y * 0.001f;
    boxMin = {x, y, normDir.z * 0.001f + 1000.0f};
    boxMax = {x, y, normDir.z * 0.001f - 1000.0f};
    if (flags == 0)
        CheckAABBAll(boxMin, boxMax, hitInfo);
    else
        CheckAABBWalkable(boxMin, boxMax, hitInfo);
    return hitInfo.m_triangleIndex;
}

// Binary-backed meshes only drop their views into the resource blob; text meshes own those
// arrays and free them here.
CSWRoomSurfaceMesh::~CSWRoomSurfaceMesh()
{
    if (m_bBinaryBacked) {
#if KOTOR1  // K1: the same clears, with the AABB list first
        m_aAABBNodes.m_pArray = 0;
        m_aAABBNodes.m_count = 0;
        m_aAABBNodes.m_capacity = 0;
        m_aAABBNodes.m_bStatic = 0;
        m_paTriAdjacency = 0;
        m_aEdgeAdjacency.m_pArray = 0;
        m_aEdgeAdjacency.m_count = 0;
        m_aEdgeAdjacency.m_capacity = 0;
        m_aEdgeAdjacency.m_bStatic = 0;
        m_aBorderEnds.m_pArray = 0;
        m_aBorderEnds.m_count = 0;
        m_aBorderEnds.m_capacity = 0;
        m_aBorderEnds.m_bStatic = 0;
#else
        m_aAABBNodes.m_pArray = 0;
        m_aAABBNodes.m_count = 0;
        m_aAABBNodes.m_capacity = 0;
        m_aAABBNodes.m_bStatic = 0;
        m_aBorderEnds.m_pArray = 0;
        m_aBorderEnds.m_count = 0;
        m_aBorderEnds.m_capacity = 0;
        m_aBorderEnds.m_bStatic = 0;
        m_aEdgeAdjacency.m_bStatic = 0;
        m_paTriAdjacency = 0;
        m_aEdgeAdjacency.m_pArray = 0;
        m_aEdgeAdjacency.m_count = 0;
        m_aEdgeAdjacency.m_capacity = 0;
#endif
    } else {
#if KOTOR1  // K1: scalar delete; the aligned flag is left as it was
        m_paTriAdjacency.Delete();
#else
        if (m_paTriAdjacency)
            operator delete[](m_paTriAdjacency);
        m_paTriAdjacency = 0;
#endif

        if (!m_aEdgeAdjacency.m_bStatic)
            m_aEdgeAdjacency.Clear();

        if (!m_aBorderEnds.m_bStatic)
            m_aBorderEnds.Clear();
    }
}

// Fills m_paTriAdjacency with each triangle's three directed-edge neighbours by vertex-pair matching.
// Each triangle's 7-int directed-edge ring is [v0,v1,v2,v0,v2,v1,v0], whose consecutive pairs
// enumerate its three edges and their reverses; each shared edge is discovered twice, and the
// `toggle` parity flag stores only every second match.
void CSWRoomSurfaceMesh::ComputeTriangleAdjacency()
{
    if (m_nWalkableTriangleCount == 0)
        return;
    m_paTriAdjacency = new int[3 * m_nWalkableTriangleCount];
    int(*edgeVerts)[7] = new int[m_nWalkableTriangleCount][7];
#if KOTOR1  // K1: the adjacency entries are written through the aligned-array accessor
    for (int i = 0; i < m_nWalkableTriangleCount; i++) {
        unsigned int base = i * 3;
        m_paTriAdjacency.SetAt(base + 2, -1);
        m_paTriAdjacency.SetAt(base + 1, -1);
        m_paTriAdjacency.SetAt(base, -1);
        int v0 = m_paTriVertIndices[base];
        edgeVerts[i][6] = v0;
        edgeVerts[i][3] = v0;
        edgeVerts[i][0] = v0;
        int v1 = m_paTriVertIndices[base + 1];
        edgeVerts[i][5] = v1;
        edgeVerts[i][1] = v1;
        int v2 = m_paTriVertIndices[base + 2];
        edgeVerts[i][4] = v2;
        edgeVerts[i][2] = v2;
    }
    int toggle = 0;
    for (int i = 0; i < m_nWalkableTriangleCount; i++) {
        for (int pair = 1; pair < 7; pair++) {
            for (int j = i + 1; j < m_nWalkableTriangleCount; j++) {
                for (int k = 1; k < 7; k++) {
                    if (edgeVerts[i][pair - 1] == edgeVerts[j][k - 1] &&
                        edgeVerts[i][pair] == edgeVerts[j][k]) {
                        if (toggle) {
                            int slotI = (pair < 4) ? (pair % 4) - 1 : 6 - pair;
                            int edgeI = i * 3 + slotI;
                            int slotJ = (k < 4) ? (k % 4) - 1 : 6 - k;
                            int edgeJ = j * 3 + slotJ;
                            m_paTriAdjacency.Set(edgeI, edgeJ);
                            m_paTriAdjacency.Set(edgeJ, edgeI);
                            toggle = 0;
                        } else {
                            toggle = 1;
                        }
                    }
                }
            }
        }
    }
#else
    for (int i = 0; i < m_nWalkableTriangleCount; i++) {
        m_paTriAdjacency[i * 3 + 2] = -1;
        m_paTriAdjacency[i * 3 + 1] = -1;
        m_paTriAdjacency[i * 3 + 0] = -1;
        int v0 = m_paTriVertIndices[i * 3 + 0];
        edgeVerts[i][6] = v0;
        edgeVerts[i][3] = v0;
        edgeVerts[i][0] = v0;
        int v1 = m_paTriVertIndices[i * 3 + 1];
        edgeVerts[i][5] = v1;
        edgeVerts[i][1] = v1;
        int v2 = m_paTriVertIndices[i * 3 + 2];
        edgeVerts[i][4] = v2;
        edgeVerts[i][2] = v2;
    }
    int toggle = 0;
    for (int i = 0; i < m_nWalkableTriangleCount; i++) {
        for (int pair = 1; pair < 7; pair++) {
            for (int j = i + 1; j < m_nWalkableTriangleCount; j++) {
                for (int k = 1; k < 7; k++) {
                    if (edgeVerts[i][pair - 1] == edgeVerts[j][k - 1] &&
                        edgeVerts[i][pair] == edgeVerts[j][k]) {
                        if (toggle) {
                            int slotI = (pair < 4) ? (pair % 4) - 1 : 6 - pair;
                            int edgeI = i * 3 + slotI;
                            int slotJ = (k < 4) ? (k % 4) - 1 : 6 - k;
                            int edgeJ = j * 3 + slotJ;
                            m_paTriAdjacency[edgeI] = edgeJ;
                            m_paTriAdjacency[edgeJ] = edgeI;
                            toggle = 0;
                        } else {
                            toggle = 1;
                        }
                    }
                }
            }
        }
    }
#endif
    delete[] edgeVerts;
}

// The .wok text loader. A "node aabb" block carries position, orientation, verts and faces; on any
// line failure the default mesh is loaded instead. `faces` classifies each triangle by the
// surface-material 2DA's walk column, moving the walkable ones to the front of the triangle arrays
// and appending the exterior ones after them. `aabb` is a depth-first-serialized binary tree, one
// node per line, whose parents are collapsed with a side mask as their leaves close.
int CSWRoomSurfaceMesh::LoadMeshText(unsigned char* pData, unsigned int size)
{
    if (m_paTriVertIndices != 0)
        return 1;

    unsigned char* lineBuf = new unsigned char[MESH_LINE_SIZE];
    unsigned char* cursor = pData;
    unsigned int remaining = size;
    int* faceVerts = 0;
    int* walkableMap = 0;
    int* exteriorMap = 0;
    int gotAABBNode = 0;
    int sawAABBNode = 0;
    float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
    int done = 0;
    unsigned char* p = lineBuf;
    do {
        for (;;) {
            if (remaining == 0 || LoadMeshString(&cursor, &remaining, lineBuf, MESH_LINE_SIZE) != 1) {
                if (lineBuf != 0)
                    delete[] lineBuf;
                if (walkableMap != 0)
                    delete[] walkableMap;
                if (exteriorMap != 0)
                    delete[] exteriorMap;
                if (faceVerts != 0)
                    delete[] faceVerts;
                if (sawAABBNode) {
                    ComputeTriangleAdjacency();
                    ComputeRoomBorder();
                    return 1;
                }
                LoadDefaultMesh();
                return 1;
            }
            p = lineBuf - 1;
            do {
                p++;
            } while (*p == ' ');
            if (strncmp(reinterpret_cast<char*>(p), "node", 4) == 0) {
                p += 4;
                gotAABBNode = (strncmp(reinterpret_cast<char*>(p), " aabb", 5) == 0);
                if (gotAABBNode)
                    sawAABBNode = 1;
            }
            if (strncmp(reinterpret_cast<char*>(p), "endnode", 7) == 0) {
                gotAABBNode = 0;
                p += 7;
            }
            if (gotAABBNode != 1)
                continue;
            if (strncmp(reinterpret_cast<char*>(p), "position", 8) == 0) {
                p += 8;
                static_cast<void>(sscanf(reinterpret_cast<char*>(p), "%f %f %f", &posX, &posY, &posZ));
                m_localOrigin.x = posX;
                m_localOrigin.y = posY;
                m_localOrigin.z = posZ;
            }
            if (strncmp(reinterpret_cast<char*>(p), "orientation", 11) == 0) {
                p += 11;
                float axisX = 0.0f, axisY = 0.0f, axisZ = 0.0f;
                float angle;
                static_cast<void>(sscanf(reinterpret_cast<char*>(p), "%f %f %f %f", &axisX, &axisY, &axisZ, &angle));
                if (axisX != 0.0f || axisY != 0.0f || axisZ != 0.0f) {
                    m_rotation = Quaternion(Vector{axisX, axisY, axisZ}, angle);
                } else {
                    m_rotation.w = 1.0f;
                    m_rotation.x = 0.0f;
                    m_rotation.y = 0.0f;
                    m_rotation.z = 0.0f;
                }
            }
            if (strncmp(reinterpret_cast<char*>(p), "verts", 5) == 0) {
                p += 5;
                static_cast<void>(sscanf(reinterpret_cast<char*>(p), "%d", &m_nVertexCount));
                SetVertexCount(m_nVertexCount);
                for (int i = 0; i < m_nVertexCount; i++) {
                    if (LoadMeshString(&cursor, &remaining, lineBuf, MESH_LINE_SIZE) == 0) {
                        LoadDefaultMesh();
                        if (lineBuf != 0)
                            delete[] lineBuf;
                        return 1;
                    }
#if KOTOR1  // K1: the vertex is read, parsed and snapped in a local, then written back
                    Vector v = m_paVertices.Get(i);
                    static_cast<void>(sscanf(reinterpret_cast<char*>(lineBuf), "%f %f %f", &v.x, &v.y, &v.z));
                    int qx = static_cast<int>(v.x * 512.0f + 0.5f);
                    if ((qx & VERTEX_SNAP_MASK) == 0)
                        v.x = static_cast<float>(qx >> VERTEX_SNAP_SHIFT);
                    int qy = static_cast<int>(v.y * 512.0f + 0.5f);
                    if ((qy & VERTEX_SNAP_MASK) == 0)
                        v.y = static_cast<float>(qy >> VERTEX_SNAP_SHIFT);
                    m_paVertices.Set(i, v);
#else
                    static_cast<void>(sscanf(reinterpret_cast<char*>(lineBuf), "%f %f %f",
                           &m_paVertices[i].x, &m_paVertices[i].y, &m_paVertices[i].z));
                    int qx = static_cast<int>(m_paVertices[i].x * 512.0f + 0.5f);
                    if ((qx & VERTEX_SNAP_MASK) == 0)
                        m_paVertices[i].x = static_cast<float>(qx >> VERTEX_SNAP_SHIFT);
                    int qy = static_cast<int>(m_paVertices[i].y * 512.0f + 0.5f);
                    if ((qy & VERTEX_SNAP_MASK) == 0)
                        m_paVertices[i].y = static_cast<float>(qy >> VERTEX_SNAP_SHIFT);
#endif
                }
            }
            if (strncmp(reinterpret_cast<char*>(p), "faces", 5) == 0) {
                p += 5;
                static_cast<void>(sscanf(reinterpret_cast<char*>(p), "%d", &m_nTriangleCount));
                SetTriangleCount(m_nTriangleCount);
                faceVerts = new int[m_nTriangleCount * 3];
                int* faceMats = new int[m_nTriangleCount];
                int ok = 1;
                for (int f = 0; f < m_nTriangleCount; f++) {
                    if (LoadMeshString(&cursor, &remaining, lineBuf, MESH_LINE_SIZE) == 0) {
                        LoadDefaultMesh();
                        if (lineBuf != 0)
                            delete[] lineBuf;
                        ok = 0;
                        break;
                    }
                    int unused0, unused1, unused2, unused3;
                    static_cast<void>(sscanf(reinterpret_cast<char*>(lineBuf), "%d %d %d %d %d %d %d %d",
                           &faceVerts[f * 3 + 0], &faceVerts[f * 3 + 1], &faceVerts[f * 3 + 2],
                           &unused0, &unused1, &unused2, &unused3, &faceMats[f]));
                }
                if (ok == 0)
                    return 1;
                int* walkableRemap = new int[m_nTriangleCount];
                int* exteriorRemap = new int[m_nTriangleCount];
                walkableMap = new int[m_nTriangleCount];
                exteriorMap = new int[m_nTriangleCount];
                int nWalk = 0;
                int nExterior = 0;
                for (int f = 0; f < m_nTriangleCount; f++) {
                    int walk;
                    {
                        CExoString colWalk("Walk");
                        g_pRules->m_p2DArrays->m_pSurfaceMaterial->GetINTEntry(faceMats[f], colWalk, &walk);
                    }
                    if (walk == 0) {
                        walkableMap[f] = -1;
                        exteriorMap[f] = nExterior;
                        exteriorRemap[nExterior] = f;
                        nExterior++;
                    } else {
                        walkableMap[f] = nWalk;
                        exteriorMap[f] = -1;
                        walkableRemap[nWalk] = f;
                        nWalk++;
                    }
                }
#if KOTOR1  // K1: the compacted triangles are written through the aligned-array accessors
                for (int out = 0; out < nWalk; out++) {
                    m_paTriVertIndices.Set(out * 3 + 0, faceVerts[walkableRemap[out] * 3 + 0]);
                    m_paTriVertIndices.Set(out * 3 + 1, faceVerts[walkableRemap[out] * 3 + 1]);
                    m_paTriVertIndices.Set(out * 3 + 2, faceVerts[walkableRemap[out] * 3 + 2]);
                    m_paTriMaterials.Set(out, faceMats[walkableRemap[out]]);
                }
                for (int out = 0; out < nExterior; out++) {
                    int f = exteriorRemap[out];
                    m_paTriVertIndices.Set((nWalk + out) * 3 + 0, faceVerts[f * 3 + 0]);
                    m_paTriVertIndices.Set((nWalk + out) * 3 + 1, faceVerts[f * 3 + 1]);
                    m_paTriVertIndices.Set((nWalk + out) * 3 + 2, faceVerts[f * 3 + 2]);
                    m_paTriMaterials.Set(nWalk + out, faceMats[f]);
                }
#else
                for (int out = 0; out < nWalk; out++) {
                    m_paTriVertIndices[out * 3 + 0] = faceVerts[walkableRemap[out] * 3 + 0];
                    m_paTriVertIndices[out * 3 + 1] = faceVerts[walkableRemap[out] * 3 + 1];
                    m_paTriVertIndices[out * 3 + 2] = faceVerts[walkableRemap[out] * 3 + 2];
                    m_paTriMaterials[out] = faceMats[walkableRemap[out]];
                }
                for (int out = 0; out < nExterior; out++) {
                    int f = exteriorRemap[out];
                    m_paTriVertIndices[(nWalk + out) * 3 + 0] = faceVerts[f * 3 + 0];
                    m_paTriVertIndices[(nWalk + out) * 3 + 1] = faceVerts[f * 3 + 1];
                    m_paTriVertIndices[(nWalk + out) * 3 + 2] = faceVerts[f * 3 + 2];
                    m_paTriMaterials[nWalk + out] = faceMats[f];
                }
#endif
                m_nWalkableTriangleCount = nWalk;
                delete[] exteriorRemap;
                delete[] walkableRemap;
                delete[] faceMats;
            }
            if (strncmp(reinterpret_cast<char*>(p), "aabb", 4) == 0) {
                p += 4;
                break;
            }
        }

        // The box-tree section is a depth-first-serialized binary tree of box nodes.
        CExoArrayList<CSWRoomSurfaceMeshAABBNode*> stack;
        CExoArrayList<int> stackIndices;
        CExoArrayList<CSWRoomSurfaceMeshAABBNode*> allNodes;
        int nNodes = 0;
        int idx = 0;
        do {
            float minX = 0.0f, minY = 0.0f, minZ = 0.0f;
            float maxX = 0.0f, maxY = 0.0f, maxZ = 0.0f;
#ifdef K2_FIX_ENGINE_BUGS
            int leafTri = -1;
#else
            int leafTri;
#endif
            static_cast<void>(sscanf(reinterpret_cast<char*>(p), "%f %f %f %f %f %f %d",
                   &minX, &minY, &minZ, &maxX, &maxY, &maxZ, &leafTri));
            int remapped = -1;
#ifdef K2_FIX_ENGINE_BUGS
            if (leafTri != -1 && walkableMap != 0) {
#else
            if (leafTri != -1) {
#endif
                remapped = walkableMap[leafTri];
                if (remapped == -1)
                    remapped = m_nWalkableTriangleCount + exteriorMap[leafTri];
            }
            leafTri = remapped;
            if (maxX < minX) {
                float t = minX;
                minX = maxX;
                maxX = t;
            }
            if (maxY < minY) {
                float t = minY;
                minY = maxY;
                maxY = t;
            }
            if (maxZ < minZ) {
                float t = minZ;
                minZ = maxZ;
                maxZ = t;
            }
            CSWRoomSurfaceMeshAABBNode* node = new CSWRoomSurfaceMeshAABBNode;
            allNodes.Add(node);
            nNodes = allNodes.m_count;
            idx = nNodes - 1;
            node->leafTriangleIndex = leafTri;
            node->corner0.x = minX - 0.01f;
            node->corner0.y = minY - 0.01f;
            node->corner0.z = minZ - 0.01f;
            node->corner1.x = maxX + 0.01f;
            node->corner1.y = maxY + 0.01f;
            node->corner1.z = maxZ + 0.01f;
            if (leafTri == -1) {
                stack.Add(node);
                stackIndices.Add(idx);
                if (LoadMeshString(&cursor, &remaining, lineBuf, MESH_LINE_SIZE) == 0) {
                    LoadDefaultMesh();
                    if (lineBuf != 0)
                        delete[] lineBuf;
                    if (walkableMap != 0)
                        delete[] walkableMap;
                    if (exteriorMap != 0)
                        delete[] exteriorMap;
                    if (faceVerts != 0)
                        delete[] faceVerts;
                    return 1;
                }
                p = lineBuf;
            } else if (stack.m_count > 0) {
                // At a leaf, collapse every finished parent above this subtree.
                for (;;) {
                    CSWRoomSurfaceMeshAABBNode* parent = stack.m_pArray[stack.m_count - 1];
                    int parentIdx = stackIndices.m_pArray[stackIndices.m_count - 1];
                    stack.m_pArray[stack.m_count - 1] = 0;
                    stack.DelIndex(stack.m_count - 1);
                    stackIndices.DelIndex(stackIndices.m_count - 1);
                    if (parent->child1 == -1) {
                        parent->child1 = idx;
                        stack.Add(parent);
                        stackIndices.Add(parentIdx);
                        if (LoadMeshString(&cursor, &remaining, lineBuf, MESH_LINE_SIZE) == 0) {
                            LoadDefaultMesh();
                            if (lineBuf != 0)
                                delete[] lineBuf;
                            if (walkableMap != 0)
                                delete[] walkableMap;
                            if (exteriorMap != 0)
                                delete[] exteriorMap;
                            if (faceVerts != 0)
                                delete[] faceVerts;
                            return 1;
                        }
                        p = lineBuf;
                        break;
                    }
                    parent->child2 = idx;
                    CSWRoomSurfaceMeshAABBNode* child1 = allNodes.m_pArray[parent->child1];
                    CSWRoomSurfaceMeshAABBNode* child2 = allNodes.m_pArray[idx];
                    float dx = (child2->corner0.x + child2->corner1.x) -
                               (child1->corner0.x + child1->corner1.x);
                    float dy = (child2->corner0.y + child2->corner1.y) -
                               (child1->corner0.y + child1->corner1.y);
                    float dz = (child2->corner0.z + child2->corner1.z) -
                               (child1->corner0.z + child1->corner1.z);
                    float adx = __builtin_fabsf(dx);
                    float ady = __builtin_fabsf(dy);
                    float adz = __builtin_fabsf(dz);
                    if (adx > ady && adx > adz)
                        parent->sideMask = (dx > 0.0f) ? CSWRoomSurfaceMeshAABBNode::SIDE_POSITIVE_X : CSWRoomSurfaceMeshAABBNode::SIDE_NEGATIVE_X;
                    else if (ady > adz)
                        parent->sideMask = (dy > 0.0f) ? CSWRoomSurfaceMeshAABBNode::SIDE_POSITIVE_Y : CSWRoomSurfaceMeshAABBNode::SIDE_NEGATIVE_Y;
                    else
                        parent->sideMask = (dz > 0.0f) ? CSWRoomSurfaceMeshAABBNode::SIDE_POSITIVE_Z : CSWRoomSurfaceMeshAABBNode::SIDE_NEGATIVE_Z;
                    idx = parentIdx;
                    if (stack.m_count < 1)
                        break;
                }
            }
        } while (stack.m_count > 0);
        m_rootAABBIndex = idx;
        for (int i = 0; i < nNodes; i++) {
            CSWRoomSurfaceMeshAABBNode* node = allNodes.m_pArray[i];
            m_aAABBNodes.Add(*node);
            if (node != 0)
                delete node;
            allNodes.m_pArray[i] = 0;
        }
        done = 0;
    } while (!done);
    return 1;
}

// Builds this room's border data from scratch; binary meshes take the same data from the BWM
// resource. Every exterior directed edge becomes a record, and the records are chained into closed
// rings by vertex: each closure appends the running border-edge count to m_aBorderEnds, and the
// surviving records are copied into m_aEdgeAdjacency.
void CSWRoomSurfaceMesh::ComputeRoomBorder()
{
    if (m_nWalkableTriangleCount == 0)
        return;

    CExoArrayList<CSWRoomSurfaceExternalEdge*> edgePtrs;
    CExoArrayList<CSWRoomSurfaceExternalEdge*> border;

    for (int i = 0; i < m_nWalkableTriangleCount * 3; i++) {
        if (m_paTriAdjacency[i] == -1) {
            CSWRoomSurfaceExternalEdge* record = new CSWRoomSurfaceExternalEdge(i);
            edgePtrs.Add(record);
        }
    }

    int nExterior = edgePtrs.m_count;
    int* indices = new int[nExterior];
    for (int i = 0; i < nExterior; i++)
        indices[i] = edgePtrs.m_pArray[i]->edge;

    if (nExterior > 0) {
        int consumed = 0;
        while (consumed < nExterior) {
            int j = 0;
            while (indices[j] == -1) {
                j++;
                K2_ASSUME(j < nExterior);
            }
            CSWRoomSurfaceExternalEdge* record = edgePtrs.m_pArray[j];
            border.Add(record);
            indices[j] = -1;
            int curEdge = record->edge;
            consumed++;
            int ringStartVertex = m_paTriVertIndices[curEdge];

            bool open = true;
            while (open) {
                int delta = (curEdge % 3 == 2) ? -2 : 1;
                int endVertex = m_paTriVertIndices[curEdge + delta];
                for (j = 0; j < nExterior; j++) {
                    if (indices[j] == -1)
                        continue;
                    CSWRoomSurfaceExternalEdge* cand = edgePtrs.m_pArray[j];
                    if (m_paTriVertIndices[cand->edge] != endVertex)
                        continue;
                    border.Add(cand);
                    indices[j] = -1;
                    curEdge = cand->edge;
                    consumed++;
                    int delta2 = (curEdge % 3 == 2) ? -2 : 1;
                    if (m_paTriVertIndices[curEdge + delta2] == ringStartVertex) {
                        open = false;
                        m_aBorderEnds.Add(consumed);
                    }
                    break;
                }
#ifdef K2_FIX_ENGINE_BUGS
                // A ring with no continuation (a malformed walkmesh) ends where its chain stops.
                if (open && j == nExterior) {
                    open = false;
                    m_aBorderEnds.Add(consumed);
                }
#endif
            }
        }

        for (int i = 0; i < border.m_count; i++) {
            if (m_aEdgeAdjacency.m_bStatic == 0) {
                CSWRoomSurfaceExternalEdge value = *border.m_pArray[i];
                CExoArrayList<CSWRoomSurfaceExternalEdge>& edges =
                    m_aEdgeAdjacency;
                // inlined CExoArrayList<CSWRoomSurfaceExternalEdge>::Add(value)
                int eidx = edges.m_count;
                if (eidx == edges.m_capacity) {
                    CSWRoomSurfaceExternalEdge* pOld = edges.m_pArray;
                    edges.m_capacity = (eidx != 0) ? eidx * 2 : 16;
                    edges.m_pArray = new CSWRoomSurfaceExternalEdge[edges.m_capacity];
                    for (int64_t ei = 0; ei < eidx; ei++) {
                        K2_ASSUME(ei < edges.m_capacity);   // the new capacity is twice the old count
                        edges.m_pArray[ei] = pOld[ei];
                    }
                    delete[] pOld;
                    eidx = edges.m_count;
                }
                edges.m_count = eidx + 1;
                K2_ASSUME(eidx < edges.m_capacity);   // the list grows when the count reaches the capacity
                edges.m_pArray[eidx] = value;
            }
            if (border.m_pArray[i] != 0)
                delete border.m_pArray[i];
        }
    }

    delete[] indices;
}

// Single-pass search against the line-of-sight material mask.
int CSWRoomSurfaceMesh::CheckAABBLineOfSight(Vector boxMin, Vector boxMax, CSWRoomSurfaceMeshHitInfo& hitInfo)
{
    if (m_aAABBNodes.m_count == 0)
        return 0;

    hitInfo.m_materialMask = m_losMaterialMask;
    int count = CheckAABBNode(m_rootAABBIndex, boxMin, boxMax, hitInfo);
    if (count > 0) {
        hitInfo.m_bHit = 1;
        hitInfo.m_point = boxMax;
    }
    return hitInfo.m_bHit;
}

// Vertical search box at the point's local XY, testing every material; returns the hit
// triangle's material bit index, or 0 if nothing was hit.
unsigned int CSWRoomSurfaceMesh::GetSurfaceMaterial(Vector point)
{
    Vector localPoint = WorldToLocal(point);
    CSWRoomSurfaceMeshHitInfo hitInfo(CSWRoomSurfaceMeshHitInfo::MATERIAL_MASK_ALL);

    Vector boxMin = {localPoint.x, localPoint.y, 1000.0f};
    Vector boxMax = {localPoint.x, localPoint.y, -1000.0f};
    CheckAABBAll(boxMin, boxMax, hitInfo);

    if (hitInfo.m_triangleIndex == -1)
        return 0;
    return m_paTriMaterials[hitInfo.m_triangleIndex];
}

// As GetSurfaceMaterial, testing only walkable materials.
unsigned int CSWRoomSurfaceMesh::GetSurfaceMaterialWalkCheckOnly(Vector point)
{
    Vector localPoint = WorldToLocal(point);
    CSWRoomSurfaceMeshHitInfo hitInfo(CSWRoomSurfaceMeshHitInfo::MATERIAL_MASK_ALL);

    Vector boxMin = {localPoint.x, localPoint.y, 1000.0f};
    Vector boxMax = {localPoint.x, localPoint.y, -1000.0f};
    CheckAABBWalkable(boxMin, boxMax, hitInfo);

    if (hitInfo.m_triangleIndex == -1)
        return 0;
    return m_paTriMaterials[hitInfo.m_triangleIndex];
}

void CSWRoomSurfaceMeshHitInfo::ClearOutput()
{
    m_bHit = 0;
    m_point.x = 0.0f;
    m_point.y = 0.0f;
    m_point.z = 0.0f;
    m_materialBit = 0;
}

CSWRoomSurfaceMeshHitInfo::CSWRoomSurfaceMeshHitInfo(unsigned int materialMask)
{
    m_materialMask = materialMask;
    m_sideSelectMask = 0;
    m_bHit = 0;
    m_triangleIndex = -1;
    m_point.x = 0.0f;
    m_point.y = 0.0f;
    m_point.z = 0.0f;
    m_materialBit = 0;
}

void CSWRoomSurfaceMeshHitInfo::ClearAll(unsigned int materialMask)
{
    m_materialMask = materialMask;
    m_sideSelectMask = 0;
    m_bHit = 0;
    m_triangleIndex = -1;
    m_point.x = 0.0f;
    m_point.y = 0.0f;
    m_point.z = 0.0f;
    m_materialBit = 0;
}

// Welds this mesh's edge vertices onto the other room's wherever they coincide within tolerance;
// returns the number of welds. The min/max bounds are unused.
int CSWRoomSurfaceMesh::WeldEdgeVertices(CSWRoomSurfaceMesh* pOther, float tolerance,
                                         const Vector& min, const Vector& max)
{
    K2_UNUSED(min); K2_UNUSED(max);
    if (m_bBinaryBacked)
        return 0;
    if (m_aEdgeAdjacency.m_count < 1)
        return 0;

    int weldCount = 0;
    for (int i = 0; i < m_aEdgeAdjacency.m_count; i++)
    {
        Vector v1;
        GetEdgeVertex(i, v1);
        if (pOther->m_aEdgeAdjacency.m_count < 1)
            continue;
        for (int j = 0; j < pOther->m_aEdgeAdjacency.m_count; j++)
        {
            Vector v2;
            pOther->GetEdgeVertex(j, v2);
            float dx = v1.x - v2.x;
            float dy = v1.y - v2.y;
            float dz = v1.z - v2.z;
            float dist2 = dx * dx + dy * dy + dz * dz;
            if (dist2 < tolerance)
            {
                weldCount++;
                pOther->SetEdgeVertex(j, v1);
            }
        }
    }
    return weldCount;
}

// Links each edge of this room to the opposing edge of the other room when both of its endpoints
// coincide with one within tolerance; returns the number of links. The per-border ring wrap comes
// from m_aBorderEnds. The min/max bounds are unused.
int CSWRoomSurfaceMesh::SetRoomAdjacency(CSWRoomSurfaceMesh* pOther, int myRoomId,
                                         int otherRoomId, const Vector& min, const Vector& max)
{
    K2_UNUSED(min); K2_UNUSED(max);
    if (m_bBinaryBacked)
        return 0;
    if (m_aEdgeAdjacency.m_count < 1)
        return 0;

    int linkCount = 0;
    int border = 0;
    Vector firstA = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < m_aEdgeAdjacency.m_count; i++)
    {
        Vector v1, v1next;
        GetEdgeVertices(i, v1, v1next);
        if (i == 0)
            firstA = v1;
        Vector v1wrap = v1next;
        if (i == m_aBorderEnds[border] - 1)
        {
            v1wrap = firstA;
            firstA = v1next;
            border++;
        }
        if (pOther->m_aEdgeAdjacency.m_count < 1)
            continue;

        int otherBorder = 0;
        Vector firstB = {0.0f, 0.0f, 0.0f};
        for (int j = 0; j < pOther->m_aEdgeAdjacency.m_count; j++)
        {
            Vector v2, v2next;
            pOther->GetEdgeVertices(j, v2, v2next);
            if (j == 0)
                firstB = v2;
            Vector v2wrap = v2next;
            if (j == pOther->m_aBorderEnds[otherBorder] - 1)
            {
                v2wrap = firstB;
                firstB = v2next;
                otherBorder++;
            }

            float dx = v1.x - v2wrap.x;
            float dy = v1.y - v2wrap.y;
            float dz = v1.z - v2wrap.z;
            float dist1sq = dx * dx + dy * dy + dz * dz;
            if (!(dist1sq < 0.01))
                continue;

            float dx2 = v1wrap.x - v2.x;
            float dy2 = v1wrap.y - v2.y;
            float dz2 = v1wrap.z - v2.z;
            float dist2sq = dx2 * dx2 + dy2 * dy2 + dz2 * dz2;
            if (!(dist2sq < 0.01))
                continue;

            m_aEdgeAdjacency[i].room = otherRoomId;
            pOther->m_aEdgeAdjacency[j].room = myRoomId;
            linkCount++;
        }
    }
    return linkCount;
}

// Ray-casts from a toward b in 2D and counts the boundary-polygon crossings, which m_aBorderEnds
// splits into runs of edge indices. Each crossing goes through AddCrossing, keeping the
// projection-sorted lists; outEdge receives the first crossing's edge index. The `c0 == ca` block
// handles a ray that passes exactly through a vertex, de-duplicating vertex touches through the
// carried prevC side state.
int CSWRoomSurfaceMesh::IsPointInPolygon(const Vector& a, const Vector& b, int& outEdge)
{
    int count = 0;
    m_aCrossingPoints.m_count = 0;
    m_aCrossingEdges.m_count = 0;

    Vector dir = normalize(Vector(b.x - a.x, b.y - a.y, 0.0f));

    if (m_aBorderEnds.m_count > 0) {
        float dotA = dir.x * a.x + dir.y * a.y + dir.z * a.z;
        float ca = dir.x * a.y - dir.y * a.x + 0.0f * a.z;
        float negDirY = -dir.y;

        int bSetOut = 0;
        float prevC = ca;
        int start = 0;
        for (int k = 0; k < m_aBorderEnds.m_count; k++) {
            int end = m_aBorderEnds[k];
            for (int e = start; e < end; e++) {
                int prev = (e > start) ? e : end;
                int next = (e + 1 == end) ? start : e + 1;

                const Vector& v0 = m_paVertices[m_paTriVertIndices[m_aEdgeAdjacency[e].edge]];
                const Vector& v1 = m_paVertices[m_paTriVertIndices[m_aEdgeAdjacency[next].edge]];
                const Vector& vp = m_paVertices[m_paTriVertIndices[m_aEdgeAdjacency[prev - 1].edge]];

                float c0 = v0.x * negDirY + v0.y * dir.x + v0.z * 0.0f;
                float c1 = v1.x * negDirY + v1.y * dir.x + v1.z * 0.0f;
                float proj0 = dir.x * v0.x + dir.y * v0.y + dir.z * v0.z;

                if ((c0 < ca && ca < c1) || (ca < c0 && c1 < ca)) {
                    float t = (ca - c1) / (c0 - c1);
                    float hx = v0.x * t;
                    float hy = v0.y * t;
                    float hz = v0.z * t;
                    Vector hit = {hx + v1.x * (1.0f - t),
                                  hy + v1.y * (1.0f - t),
                                  hz + v1.z * (1.0f - t)};
                    float proj = dir.x * hit.x + dir.y * hit.y + dir.z * hit.z;
                    if (dotA <= proj) {
                        AddCrossing(a, dir, hit, count, e);
                        if (!bSetOut) {
                            outEdge = e;
                            bSetOut = 1;
                        }
                    }
                }

                if (c0 == ca) {
                    if (dotA <= proj0) {
                        float cp = vp.x * negDirY + vp.y * dir.x + vp.z * 0.0f;
                        if (c0 != c1 || cp != c0) {
                            if (prevC == ca) {
                                c0 = (cp != ca) ? cp : prevC;
                            } else if (prevC < ca) {
                                c0 = ca;
                                if (!(c1 < ca)) {
                                    prevC = c0;
                                    continue;
                                }
                            } else if (prevC > ca) {
                                c0 = ca;
                                if (!(ca < c1)) {
                                    prevC = c0;
                                    continue;
                                }
                            } else {
                                prevC = ca;
                                continue;
                            }
                            AddCrossing(a, dir, v0, count, e);
                            if (!bSetOut) {
                                outEdge = e;
                                bSetOut = 1;
                            }
                            if (proj0 != dotA &&
                                ((c1 < ca && cp < ca) || (ca < c1 && cp > ca))) {
                                AddCrossing(a, dir, v0, count, e);
                            }
                            prevC = c0;
                        }
                    }
                }
            }
            start = end;
        }
        if (count > 1)
            outEdge = m_aCrossingEdges[0];
    }
    return count;
}

// Inserts the point into the direction-projection-sorted m_aCrossingPoints, with its edge at the
// same index in m_aCrossingEdges. The segStart argument is unused.
void CSWRoomSurfaceMesh::AddCrossing(const Vector& segStart, const Vector& dir,
                                     const Vector& point, int& outCount, int value)
{
    K2_UNUSED(segStart);
    CExoArrayList<Vector>& points = m_aCrossingPoints;
    int i = 0;
    if (m_aCrossingPoints.m_count > 0)
    {
        for (; i < m_aCrossingPoints.m_count; i++)
        {
            if (dot(point, dir) < dot(points.m_pArray[i], dir))
                break;
        }
    }

    points.Insert(point, i);

    m_aCrossingEdges.Insert(value, i);
    outCount++;
}
