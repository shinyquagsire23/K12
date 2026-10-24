#include "swsroom.h"
#include "swroomsurfacemesh.h"

// The room owns its walkmesh outright.
CSWSRoom::CSWSRoom()
{
    m_pSurfaceMesh = new CSWRoomSurfaceMesh;
}

CSWSRoom::~CSWSRoom()
{
    delete m_pSurfaceMesh;
    m_pSurfaceMesh = 0;
}

int CSWSRoom::ClippedLineSegmentWalkable(const Vector& segStart, const Vector& segEnd,
                                         Vector& clipStart, Vector& clipEnd, float& tStart,
                                         float& tEnd, int& outFlags)
{
    return m_pSurfaceMesh->ClippedLineSegmentWalkable(segStart, segEnd, clipStart, clipEnd,
                                                      tStart, tEnd, outFlags);
}

int CSWSRoom::ComputeClippedLineSegment(Vector segStart, Vector segEnd, Vector& clipStart,
                                        Vector& clipEnd, int& outFlags)
{
    return m_pSurfaceMesh->ComputeClippedLineSegment(segStart, segEnd, clipStart, clipEnd, outFlags);
}

int CSWSRoom::GetAdjacentRoom(int edgeIdx)
{
    return m_pSurfaceMesh->GetAdjacentRoom(edgeIdx);
}

int CSWSRoom::CheckAABBAll(Vector boxMin, Vector boxMax, CSWRoomSurfaceMeshHitInfo& hitInfo)
{
    return m_pSurfaceMesh->CheckAABBAll(boxMin, boxMax, hitInfo) != 0;
}

// On a hit, folds the local-space hit point up to world space: this room's origin plus the mesh's
// baked translation.
#if KOTOR1
int CSWSRoom::CheckAABBWalkable(Vector boxMin, Vector boxMax, CSWRoomSurfaceMeshHitInfo& hitInfo)
#else
int CSWSRoom::CheckAABBWalkable(const Vector& boxMin, const Vector& boxMax,
                                CSWRoomSurfaceMeshHitInfo& hitInfo)
#endif
{
    if (m_pSurfaceMesh->CheckAABBWalkable(boxMin, boxMax, hitInfo) != 0)
    {
        hitInfo.m_point = Vector{hitInfo.m_point.x + m_vPosition.x + m_pSurfaceMesh->m_translation.x,
                                 hitInfo.m_point.y + m_vPosition.y + m_pSurfaceMesh->m_translation.y,
                                 hitInfo.m_point.z + m_vPosition.z + m_pSurfaceMesh->m_translation.z};
        return 1;
    }
    return 0;
}

// Both tail-forward to the mesh when it exists, else report a height of 0.
float CSWSRoom::ComputeHeight(int triangle, Vector point)
{
    if (m_pSurfaceMesh != 0)
        return m_pSurfaceMesh->ComputeHeight(triangle, point);
    return 0.0f;
}

float CSWSRoom::ComputeHeight(Vector point, int flags)
{
    if (m_pSurfaceMesh != 0)
        return m_pSurfaceMesh->ComputeHeight(point, flags);
    return 0.0f;
}

// The whole-room walkmesh operations CSWSArea::LoadRooms drives, plus the two segment probes. Each
// one opens with the same wildcard guard: a room named "****" has no .wok of its own, so the
// operation is skipped, and reported as trivially successful where there is a return value.

// Points the mesh at this room's own resource name, then loads it. bForceText comes straight from
// the argument; LoadRooms passes 0, to prefer the binary .wok.
int CSWSRoom::LoadWalkMesh(int bForceText)
{
    if (m_bRoomNameIsWildcard)
        return 0;
    m_pSurfaceMesh->m_resRef = m_resRoom;
    return m_pSurfaceMesh->LoadMesh(bForceText);
}

// Rebases the mesh's local-space vertices onto the room's world origin.
void CSWSRoom::TransformToWorld()
{
    if (m_bRoomNameIsWildcard)
        return;
    m_pSurfaceMesh->TransformToWorld(m_vPosition);
}

void CSWSRoom::ComputeMeshNormals()
{
    if (m_bRoomNameIsWildcard)
        return;
    m_pSurfaceMesh->ComputeMeshNormals();
}

// Snaps this room's edge vertices onto the neighbour's where they are within fTolerance. Either room
// being a wildcard skips the weld.
int CSWSRoom::WeldRoom(CSWSRoom& room, float fTolerance)
{
    if (m_bRoomNameIsWildcard || room.m_bRoomNameIsWildcard)
        return 0;
    return m_pSurfaceMesh->WeldEdgeVertices(room.m_pSurfaceMesh, fTolerance, m_vPosition,
                                            Vector(room.m_vPosition.x, room.m_vPosition.y, room.m_vPosition.z));
}

// Cross-links the two meshes' border edges, recording each room's index in the other's edge table.
// Same wildcard gate and the same neighbour-origin temporary as WeldRoom.
int CSWSRoom::SetRoomAdjacency(CSWSRoom& room, int nThisRoom, int nOtherRoom)
{
    if (m_bRoomNameIsWildcard || room.m_bRoomNameIsWildcard)
        return 0;
    return m_pSurfaceMesh->SetRoomAdjacency(room.m_pSurfaceMesh, nThisRoom, nOtherRoom,
                                            m_vPosition, Vector(room.m_vPosition.x, room.m_vPosition.y, room.m_vPosition.z));
}

// Probes a segment against this room's walkmesh, forwarding the three floats, dereferenced because
// the callee takes them by value.
int CSWSRoom::NoNonWalkPolysOnRoom(const Vector& a, const Vector& b, const float& f1,
                                   const float& f2, const float& f3, CWalkHitInfo* hitInfo)
{
    return m_pSurfaceMesh->NoNonWalkPolys(a, b, f1, f2, f3, hitInfo);
}

// Line-of-sight probe from `from` to `to` across this room's walkmesh. A wildcard room never
// blocks, so it reports the way clear. The hit info is armed with the all-materials mask both by
// its constructor and by the redundant ClearAll that follows; both calls are made. On a hit the
// world-space contact point is handed back through pHitPoint when one was supplied. The
// return is inverted relative to CheckAABBLineOfSight, which reports "hit" where this reports
// "clear".
int CSWSRoom::ClearLineOfSight(Vector from, Vector to, Vector* pHitPoint,
                               unsigned long long* pObjectId)
{
    K2_UNUSED(pObjectId);
    if (m_bRoomNameIsWildcard)
        return 1;
    CSWRoomSurfaceMeshHitInfo hitInfo(CSWRoomSurfaceMeshHitInfo::MATERIAL_MASK_ALL);
    hitInfo.ClearAll(CSWRoomSurfaceMeshHitInfo::MATERIAL_MASK_ALL);
    int bHit = m_pSurfaceMesh->CheckAABBLineOfSight(from, to, hitInfo);
    if (pHitPoint && bHit)
        *pHitPoint = hitInfo.m_point;
    return bHit == 0;
}

// The one query that does not forward to the mesh. It reads the triangle-normal array directly and
// answers (0,0,1) for a negative index and for an unbuilt normal array. The mesh pointer itself is
// dereferenced without a null test; only the array load is guarded.
// The point arrives by value and is spilled because FindTriangle takes it by const reference.
Vector CSWSRoom::GetNormalOfPoint(Vector vPoint)
{
    Vector vNormal(0.0f, 0.0f, 1.0f);
    int nPoint = m_pSurfaceMesh->FindTriangle(vPoint, 1, Vector(1.0f, 1.0f, 0.0f));
    if (nPoint >= 0) {
        Vector* paNormals = m_pSurfaceMesh->m_paTriNormals;
        if (paNormals != 0)
            vNormal = paNormals[nPoint];
    }
    return vNormal;
}

// The index-taking sibling.
Vector CSWSRoom::GetNormalOfPoint(int nPoint)
{
    Vector vNormal(0.0f, 0.0f, 1.0f);
    if (nPoint >= 0) {
        Vector* paNormals = m_pSurfaceMesh->m_paTriNormals;
        if (paNormals != 0)
            vNormal = paNormals[nPoint];
    }
    return vNormal;
}
