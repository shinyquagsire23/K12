#include "k2stdint.h"
#include <algorithm>

#include "nwsarea.h"
#include "swsroom.h"

// The DFS transposition tables' entry counts.
enum {
    GRID_TRANS_TABLE_SIZE       = 512,
    PATH_POINT_TRANS_TABLE_SIZE = 512,
};

// The searches' give-up points: grid DFS expansions, path-point DFS recursion depth (KOTOR 2) and
// path-point DFS expansions.
enum {
    GRID_DFS_EXPANSION_LIMIT       = 63,
    PATH_POINT_DFS_DEPTH_LIMIT     = 51,
    PATH_POINT_DFS_EXPANSION_LIMIT = 4095,
};

// The DFS debug render's square colour (ARGB).
enum : unsigned int {
    DEBUG_COLOR_WHITE = 0xffffffff,
};

// The most players FindSafeLocation counts as seeing a candidate point.
enum {
    VISIBLE_COUNT_CAP = 96,
};

// Multiplying a byte by this repeats it in all four bytes of a word.
enum : unsigned int {
    BYTE_REPEAT_X4 = 0x01010101u,
};
#include "swroomsurfacemesh.h"
#include "nwarea.h"
#include "serverexoapp.h"
#include "exobase.h"
#include "nwsobject.h"
#include "gameobjectarray.h"
#include "CExoArrayListImpl.h"
#include "nwscreature.h"
#include "serveraimaster.h"
#include "appmanager.h"

int DumpRoomBBox;
int RenderTest;

int SmoothTime = 10000;
extern int RenderAi;

extern int bDumpData;
extern int g_bRenderClipWalkable;

int RenderDFS;
int bInitnRenderDEFLines = 1;
unsigned int nRenderDEFLines[500];
int nRenderDFSCount;
extern int LockRenderDev;
uint64_t AurAddDrawableSquare(float* pPoints, uint64_t nColor, const char* pFile, int nLine);
#if KOTOR1
// K1 declares the two-argument form.
uint64_t AurAddDrawableSquare(float* pPoints, uint64_t nColor);
#endif
void AurRemoveDrawableSquare(uint64_t nId);

void AurPostString(char* pszString, int nX, int nY, float fLife);
#ifdef K2_FIX_ENGINE_BUGS
extern "C" void k2_fix_room_index_missing(float fX, float fY);
#endif
#include "port/libcbase.h"
#include "port/libcstring.h"
#include "port/libcmath.h"
#if !KOTOR1
static void snprintff(char* pBuf, uint64_t nSize, const char* pFmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, pFmt);
    __vsprintf_chk(pBuf, 0, nSize, pFmt, ap);
    __builtin_va_end(ap);
}
#endif
#include "pathfindinfo.h"
#include "nwstrigger.h"
#include "nwsencounter.h"
#include "nwsspellimpact.h"
#include "nwsdoor.h"
#include "nwsplaceable.h"
#include "SWCollisionMesh.h"
#include <new>
#include "nwscript.h"
#include "nwsscriptevent.h"
#include "serverexoappinternal.h"
#include "clientexoapp.h"
#include "guiingame.h"
#include "CExoArrayListImpl.h"
#include "nwssoundobject.h"
#include "nwsstore.h"
#include "nwsitem.h"
#include "nwseffect.h"
#include "nwseffectlist.h"
#include "partytables.h"
#include "nwcarea.h"
#include "exostring.h"
#include "nwsrules.h"
#include "nwsmodule.h"
#include "aurtexcontrol.h"
#include "nwsambientsound.h"
#include "worldtimer.h"
#include "CResHelper.h"
#include "resare.h"
#include "resgff.h"
#include "swsareamap.h"
#include "nwswaypoint.h"
#include "exoresman.h"
#include "nwsplayer.h"
#include "nwssendplayermessage.h"
#include "lyt.h"

// Height of the walkmesh under v, resolved through the room that contains v. The room search
// reports its room index through *pi when a hint pointer is supplied.
#if !KOTOR1
float CSWSArea::ComputeHeight(const Vector& v, int n, CSWSRoom** ppRoom, int* pi)
{
    int nIdx;
    CSWSRoom* pRoom = GetRoom(v, &nIdx, pi);
    if (ppRoom != 0)
        *ppRoom = pRoom;
    if (pRoom == 0)
        return 0.0f;
    if (n != 0)
        return pRoom->ComputeHeight(nIdx, v);
    return pRoom->ComputeHeight(v, 0);
}
#endif

#if KOTOR1
// K1 has no room-index hint.
float CSWSArea::ComputeHeight(const Vector& v, int n, CSWSRoom** ppRoom)
{
    int nIdx;
    CSWSRoom* pRoom = GetRoom(v, &nIdx);
    if (ppRoom != 0)
        *ppRoom = pRoom;
    if (pRoom == 0)
        return 0.0f;
    if (n != 0)
        return pRoom->ComputeHeight(nIdx, v);
    return pRoom->ComputeHeight(v, 0);
}
#endif

// Room whose walkmesh the vertical ray through v crosses. The ray spans 1000 units either
// side of v; the hit triangle index is written to *pOutIndex, or -1 when nothing is hit.
#if KOTOR1  // K1 probes the rooms in order
CSWSRoom* CSWSArea::GetRoom(const Vector& v, int* pOutIndex)
{
    Vector vTop = { v.x, v.y, v.z + 1000.0f };
    Vector vBottom = { v.x, v.y, v.z - 1000.0f };
    CSWRoomSurfaceMeshHitInfo hitInfo(CSWRoomSurfaceMeshHitInfo::MATERIAL_MASK_ALL);
    if (pOutIndex != 0)
        *pOutIndex = -1;
    for (int i = 0; i < m_nRoomCount; i++)
    {
        if (m_paRooms[i].CheckAABBWalkable(vTop, vBottom, hitInfo) != 0)
        {
            CSWSRoom* pRoom = &m_paRooms[i];
            if (pOutIndex != 0)
                *pOutIndex = hitInfo.m_triangleIndex;
            return pRoom;
        }
    }
    return 0;
}
#else
// K2 also probes the room named by *pTryIndex first, and updates it on a later hit.
CSWSRoom* CSWSArea::GetRoom(const Vector& v, int* pOutIndex, int* pTryIndex)
{
    Vector vTop = { v.x, v.y, v.z + 1000.0f };
    Vector vBottom = { v.x, v.y, v.z - 1000.0f };
    CSWRoomSurfaceMeshHitInfo hitInfo(CSWRoomSurfaceMeshHitInfo::MATERIAL_MASK_ALL);
    int nRoom = -1;
    if (pOutIndex != 0)
        *pOutIndex = -1;
    if (pTryIndex != 0 && *pTryIndex >= 0 && *pTryIndex < m_nRoomCount)
    {
        nRoom = *pTryIndex;
        if (m_paRooms[nRoom].CheckAABBWalkable(vTop, vBottom, hitInfo) != 0)
        {
            CSWSRoom* pRoom = &m_paRooms[nRoom];
            if (pOutIndex != 0)
                *pOutIndex = hitInfo.m_triangleIndex;
            return pRoom;
        }
    }
    for (int i = 0; i < m_nRoomCount; i++)
    {
        if (i == nRoom)
            continue;
        if (m_paRooms[i].CheckAABBWalkable(vTop, vBottom, hitInfo) != 0)
        {
            CSWSRoom* pRoom = &m_paRooms[i];
            if (pOutIndex != 0)
                *pOutIndex = hitInfo.m_triangleIndex;
            if (pTryIndex != 0)
                *pTryIndex = i;
            return pRoom;
        }
    }
    return 0;
}
#endif

CSWSRoom* CSWSArea::GetRoom(int nRoom)
{
    if (nRoom < 0 || nRoom >= m_nRoomCount)
        return 0;
    return &m_paRooms[nRoom];
}

// Index of the first room whose walkmesh the vertical ray through v crosses. With nRoom == 0
// the surface also has to be walkable.
int CSWSArea::GetRoomIndex(const Vector& v, int nRoom)
{
    Vector vTop = { v.x, v.y, v.z + 1000.0f };
    Vector vBottom = { v.x, v.y, v.z - 1000.0f };
    CSWRoomSurfaceMeshHitInfo hitInfo(CSWRoomSurfaceMeshHitInfo::MATERIAL_MASK_ALL);
    for (int i = 0; i < m_nRoomCount; i++)
    {
        if (nRoom != 0)
        {
            if (m_paRooms[i].CheckAABBAll(vTop, vBottom, hitInfo) != 0)
                return i;
        }
        else
        {
            if (m_paRooms[i].CheckAABBWalkable(vTop, vBottom, hitInfo) != 0)
                return i;
        }
    }
    return -1;
}

// Force rating of the room containing v; 0 when the rating table is not loaded.
#if !KOTOR1
int CSWSArea::GetRoomForceRating(const Vector& v)
{
    if (m_paForceRating == 0)
        return 0;
    int nRoom = GetRoomIndex(v, 0);
#ifdef K2_FIX_ENGINE_BUGS
    // A point in no room gives nRoom == -1, which would index the room table one entry out of
    // bounds. Report it instead.
    if (nRoom < 0) {
        k2_fix_room_index_missing(v.x, v.y);
        return 0;
    }
#endif
    return m_paForceRating[m_paRoomTableIndex[nRoom]];
}

// Weather-disable flag of the room containing v; 0 when the table is not loaded.
char CSWSArea::GetRoomDisableWeather(const Vector& v)
{
    if (m_paRoomDisableWeather == 0)
        return 0;
    int nRoom = GetRoomIndex(v, 0);
#ifdef K2_FIX_ENGINE_BUGS
    // Same out-of-range read as GetRoomForceRating.
    if (nRoom < 0) {
        k2_fix_room_index_missing(v.x, v.y);
        return 0;
    }
#endif
    return m_paRoomDisableWeather[m_paRoomTableIndex[nRoom]];
}
#endif

// Length of the polyline in paPoints; a path of a single point has length 0.
float CSWSArea::ComputePathDistance(int nPoints, float* paPoints)
{
    if (nPoints - 2 <= 0)
        return 0.0f;
    float fTotal = 0.0f;
    float fPrevX = paPoints[0];
    for (int i = 0; i < nPoints - 2; i += 2)
    {
        float fDx = paPoints[i + 2] - fPrevX;
        float fDy = paPoints[i + 3] - paPoints[i + 1];
        fTotal += __builtin_sqrtf(fDx * fDx + fDy * fDy);
        fPrevX = paPoints[i + 2];
    }
    return fTotal;
}

// Sound routes are always considered open.
int CSWSArea::PlotSoundPath(CPathfindInformation* pPathInfo)
{
    K2_UNUSED(pPathInfo);
    return 1;
}


// Point to retreat to when moving away from `target`: eight 45-degree candidates around the
// direction away from the target, each probed with TestDirectLine and scored by its distance
// from the target, weighted 4^min(i, 8-i) so the direction pointing away wins. A candidate
// inside the target's own radius is pushed out one more radius, and a blocked candidate is
// retried on a halving scale.
Vector CSWSArea::ComputeAwayVector(const Vector& pos, const Vector& target, float fRadius,
                                   CPathfindInformation* pPathInfo)
{
    float fDx = target.x - pos.x;
    float fDy = target.y - pos.y;
    float fDz = target.z - pos.z;
    float fDistSq = fDx * fDx + fDy * fDy + fDz * fDz;

    Vector vFlat = { fDx, fDy, 0.0f };
    Vector vDir = normalize(vFlat);

    float fRadiusSq = fRadius * fRadius;
    Vector vAway = pos;
    float fBestScore = 0.0f;

    for (int i = 0; i < 8; i++)
    {
        float fAngle = static_cast<float>(i) * 0.7853982f;
        float fCos = __builtin_cosf(fAngle);
        float fSin = __builtin_sinf(fAngle);

        Vector vRot = { vDir.x * fCos + vDir.y * fSin,
                        vDir.y * fCos - vDir.x * fSin,
                        0.0f };
        Vector vOffset = vRot * fRadius;

        Vector vSample = pos + vOffset;

        float fSampleDx = target.x - vSample.x;
        float fSampleDy = target.y - vSample.y;
        float fSampleDz = target.z - vSample.z;
        if (fSampleDx * fSampleDx + fSampleDy * fSampleDy + fSampleDz * fSampleDz < fRadiusSq)
            vSample = vSample + vOffset;

        if (pPathInfo == 0)
            continue;

        m_pActivePathfindInfo = pPathInfo;

        int nResult;
        {
            int nFraction = pPathInfo->m_nPersonalSpaceBits;
            nResult = TestDirectLine(pos, vSample, *reinterpret_cast<const float*>(&nFraction),
                                     pPathInfo->m_fHeightAlias, 0, 0);
        }

        if (nResult != 1)
        {
            float fScale = 1.0f;

            do
            {
                vSample = pos + vOffset * fScale;
                {
                    int nFraction = pPathInfo->m_nPersonalSpaceBits;
                    nResult = TestDirectLine(pos, vSample, *reinterpret_cast<const float*>(&nFraction),
                                             pPathInfo->m_fHeightAlias, 0, 0);
                }
                fScale = fScale * 0.5f;
            } while (nResult != 1 && fScale >= 0.005f);

            if (nResult != 1)
                continue;

            float fEx = target.x - vSample.x;
            float fEy = target.y - vSample.y;
            float fEz = target.z - vSample.z;
            // Written as !(a >= b) rather than a < b, so a NaN distance drops the candidate.
            if (!(fEx * fEx + fEy * fEy + fEz * fEz >= fDistSq))
                continue;
        }

        float fScore = magnitude(vSample - target);
        if (i == 3 || i == 5)
            fScore = fScore * 64.0f;
        else if (i == 4)
            fScore = fScore * 256.0f;
        else if (i == 2 || i == 6)
            fScore = fScore * 16.0f;
        else if (i == 1 || i == 7)
            fScore = fScore * 4.0f;

        if (fScore >= fBestScore)
        {
            vAway = vSample;
            fBestScore = fScore;
        }
    }

    return vAway;
}

extern int RenderTest;
extern int DumpRoomBBox;

// Walks the from->to segment room by room, clipping it to each room's walkmesh and hopping
// through the crossed edge. Returns 1 when the segment reaches `to` inside one room, 0 when
// the walk runs out of rooms, and -1/-2/-3 for the three walkmesh rejections. The `g`
// argument is unused.
int CSWSArea::TestDirectLine(const Vector& from, const Vector& to, const float& hitFraction,
                             float fMaxDistance, int g, CWalkHitInfo* pHitInfo)
{
    K2_UNUSED(g);
    Vector vStart = from;
    Vector vClipStart;
    Vector vClipEnd;
    CSWSRoom* pRoom = GetRoom(vStart, 0, 0);

    if (DumpRoomBBox != 0)
        DumpRoomBBox = 0;

    int nEdgeIndex;
    unsigned long long oidHit;
    float fZLow = 0.0f;
    float fZHigh = 0.0f;
    float fRange = fMaxDistance + 0.1f;

    int nResult = 0;
#if !KOTOR1
    // Room-hop cap, K2 only.
    int nAttempt = 0;
#endif
    int bRetry;
    do
    {
        if (pRoom == 0)
            return 0;

        bRetry = 1;
        nResult = -1;

        if (pRoom->ClippedLineSegmentWalkable(vStart, to, vClipStart, vClipEnd,
                                              fZLow, fZHigh, nEdgeIndex) == 1)
        {
            fZLow -= 0.1f;
            fZHigh += fRange;

            if (pRoom->NoNonWalkPolysOnRoom(vClipStart, vClipEnd, fZLow, fZHigh, hitFraction,
                                            pHitInfo) != 0)
            {
                if (NoNonWalkPolysDetailed(vClipStart, vClipEnd, fZLow - 1000.0f,
                                           fZHigh + 1000.0f, hitFraction,
                                           m_pActivePathfindInfo->m_oidIgnoreDoor, oidHit, 0, 0, 0, pHitInfo, 0) == 0)
                {
                    nResult = -2;
                }
                else if (m_pActivePathfindInfo == 0)
                {
                    nResult = -3;
                }
                else
                {
                    CPathfindInformation* pPath = m_pActivePathfindInfo;
                    if (pPath->m_bIgnoreCrePath == 0)
                    {
                        if (!(hitFraction > 0.0f))
                            nResult = -3;
                        else if (NoCreaturesOnLine(vClipStart, vClipEnd, pPath, 1, 0, 0, 1,
                                                   pHitInfo) == 0)
                            nResult = -3;
                    }
                    if (nResult == -1)
                    {
                        if (vClipEnd.x >= to.x - CSWArea::EPSILON &&
                            vClipEnd.x <= to.x + CSWArea::EPSILON &&
                            vClipEnd.y >= to.y - CSWArea::EPSILON &&
                            vClipEnd.y <= to.y + CSWArea::EPSILON)
                        {
                            bRetry = 0;
                            nResult = 1;
                        }
                        else
                        {
                            int nAdjacent = pRoom->GetAdjacentRoom(nEdgeIndex);
                            if (nAdjacent == -1 || m_paRooms[nAdjacent].m_bRoomNameIsWildcard != 0)
                            {
                                nResult = 0;
                            }
                            else
                            {
#if KOTOR1
                                // The clip ends are already known equal; K1 compares before hopping.
                                (void)equal(vClipStart, vClipEnd);
                                pRoom = &m_paRooms[nAdjacent];
                                vStart = vClipEnd;
#else
                                pRoom = &m_paRooms[nAdjacent];
                                (void)equal(vClipStart, vClipEnd);
                                vStart = vClipEnd;
#endif
                                nResult = 1;
                            }
                        }
                    }
                }
            }
        }

#if !KOTOR1
        // K2 caps the room hops.
        if (nAttempt > 99)
            nResult = 0;
        if (nAttempt < 100)
            nAttempt++;
#endif

    } while (bRetry != 0 && nResult == 1);

    if (RenderTest != 0)
    {
        Vector vFrom = from;
        Vector vTo = to;
        ComputeHeight(vFrom, 0, 0, 0);
        ComputeHeight(vTo, 0, 0, 0);
    }

    return nResult;
}

// Answers whether a creature can safely stand at vPosition. The probe ray is walked up through the
// room floors until it reaches the top of the ray, then the surviving segment is tested for
// non-walk polygons and for creatures.
int CSWSArea::TestSafeLocationPoint(const Vector& vPosition, CPathfindInformation* pPathInfo)
{
    m_pActivePathfindInfo = pPathInfo;

    int nPersonalSpaceBits = pPathInfo->m_nPersonalSpaceBits;

    Vector rayLow(vPosition.x - 0.01f, vPosition.y - 0.01f, vPosition.z - 0.1f);
    Vector rayHigh(vPosition.x + 0.01f, vPosition.y + 0.01f, vPosition.z + 0.1f);

    float fZ1 = vPosition.z - 0.1f;
    float fZ2 = fZ1 + pPathInfo->m_fHeight + 0.1f;

    CSWSRoom* pRoom = GetRoom(rayLow, 0, 0);
    if (pRoom == 0)
        return 0;

    int bDone = 0;
    while (!bDone)
    {
        Vector vClipLow(0.0f, 0.0f, 0.0f);
        Vector vClipHigh(0.0f, 0.0f, 0.0f);
        int nClipped;
        pRoom->m_pSurfaceMesh->ComputeClippedLineSegment(rayLow, rayHigh, vClipLow, vClipHigh,
                                                         nClipped);
        if (pRoom->NoNonWalkPolysOnRoom(vClipLow, vClipHigh, fZ1, fZ2, *reinterpret_cast<const float*>(&nPersonalSpaceBits),
                                        0) == 0)
            return 0;
        if (vClipHigh.x >= rayHigh.x - CSWArea::EPSILON &&
            vClipHigh.x <= rayHigh.x + CSWArea::EPSILON &&
            vClipHigh.y >= rayHigh.y - CSWArea::EPSILON &&
            vClipHigh.y <= rayHigh.y + CSWArea::EPSILON)
        {
            bDone = 1;
        }
        else
        {
            int nAdjacent = pRoom->GetAdjacentRoom(nClipped);
            if (nAdjacent == -1)
                return 0;
            if (m_paRooms[nAdjacent].m_bRoomNameIsWildcard != 0)
                return 0;
            pRoom = &m_paRooms[nAdjacent];
            bDone = equal(vClipLow, vClipHigh) != 0;
            rayLow = vClipHigh;
        }
    }

    unsigned long long oidIgnore = pPathInfo->m_oidIgnoreDoor;
    if (NoNonWalkPolys(rayLow, rayHigh, fZ1, fZ2, *reinterpret_cast<const float*>(&nPersonalSpaceBits),
                       oidIgnore, 0, 0) == 0)
        return 0;
    return NoCreaturesOnLine(rayLow, rayHigh, pPathInfo, 1, 0, 0, 0, 0) != 0;
}

// A signed remainder on purpose, since masking with 255 changes the result for negative
// coordinates.
int CSWSArea::GridDFSTransTableHash(int nTileX, int nTileY)
{
    return (nTileX * 17 + nTileY * 23 + 6000) % 256;
}

// Twin of GridDFSTransTableHash for the path-point table's coordinates.
int CSWSArea::PathPointDFSTransTableHash(int nTileX, int nTileY)
{
    return (nTileX * 17 + nTileY * 23 + 6000) % 256;
}


#include "port/win32api.h"



// Allocates and blanks the grid DFS trans table; every entry starts on the blank sentinel.
void CSWSArea::GridDFSTransTableInitialize()
{
    if (m_paGridDFSTransTable == 0)
        m_paGridDFSTransTable = new CSWSAreaGridTransEntry[GRID_TRANS_TABLE_SIZE];

    static const CSWSAreaGridTransEntry kBlank = { 0, 0, -2147483647 - 1, 0 };
    CSWSAreaGridTransEntry* const pEnd = m_paGridDFSTransTable + GRID_TRANS_TABLE_SIZE;
    for (CSWSAreaGridTransEntry* p = m_paGridDFSTransTable; p != pEnd; p += 8) {
        p[2] = kBlank;
        p[3] = kBlank;
        p[0] = kBlank;
        p[1] = kBlank;
        p[6] = kBlank;
        p[7] = kBlank;
        p[4] = kBlank;
        p[5] = kBlank;
    }
}

// Same blank fill as the grid table, but an existing block is reused while IsBadWritePtr still
// accepts it. Every entry's third cost field gets the path-point sentinel.
void CSWSArea::PathPointDFSTransTableInitialize()
{
#if KOTOR1  // K1 reuses the block without probing it
    if (m_paPathPointDFSTransTable == 0)
#else
    if (m_paPathPointDFSTransTable == 0 ||
        IsBadWritePtr(m_paPathPointDFSTransTable, sizeof(CSWSAreaPathPointTransEntry) * PATH_POINT_TRANS_TABLE_SIZE) != 0)
#endif
        m_paPathPointDFSTransTable = new CSWSAreaPathPointTransEntry[PATH_POINT_TRANS_TABLE_SIZE];

    static const CSWSAreaPathPointTransEntry kBlank = { 0.0f, 0.0f, 3.40282347e+37f, 0.0f };
    CSWSAreaPathPointTransEntry* const pEnd = m_paPathPointDFSTransTable + PATH_POINT_TRANS_TABLE_SIZE;
    for (CSWSAreaPathPointTransEntry* p = m_paPathPointDFSTransTable; p != pEnd; p += 8) {
        p[2] = kBlank;
        p[3] = kBlank;
        p[0] = kBlank;
        p[1] = kBlank;
        p[6] = kBlank;
        p[7] = kBlank;
        p[4] = kBlank;
        p[5] = kBlank;
    }
}

// Stores the entry in slot h, or in the overflow slot h+256 when the resident entry scores
// better.
void CSWSArea::GridDFSTransTablePut(int nTileX, int nTileY, int nCostA, int nCostB)
{
    int nIndex = GridDFSTransTableHash(nTileX, nTileY);
    if (m_paGridDFSTransTable[nIndex].m_nKeyX == nTileX &&
        m_paGridDFSTransTable[nIndex].m_nKeyY == nTileY)
        goto store;
    if (m_paGridDFSTransTable[nIndex].m_nCostA > nCostA ||
        (m_paGridDFSTransTable[nIndex].m_nCostA == nCostA &&
         m_paGridDFSTransTable[nIndex].m_nCostB > nCostB))
        goto store;
    nIndex += 256;
store:
    m_paGridDFSTransTable[nIndex].m_nKeyX = nTileX;
    m_paGridDFSTransTable[nIndex].m_nKeyY = nTileY;
    m_paGridDFSTransTable[nIndex].m_nCostA = nCostA;
    m_paGridDFSTransTable[nIndex].m_nCostB = nCostB;
}

// Looks the key up in slot h and then h+256; a miss leaves 0 in both costs.
int CSWSArea::GridDFSTransTableGet(int nTileX, int nTileY, int* pOutCostA, int* pOutCostB)
{
    int nIndex = GridDFSTransTableHash(nTileX, nTileY);
    CSWSAreaGridTransEntry* pEntry = &m_paGridDFSTransTable[nIndex];
    if (pEntry->m_nKeyX == nTileX && pEntry->m_nKeyY == nTileY)
    {
        *pOutCostA = pEntry->m_nCostA;
        *pOutCostB = pEntry->m_nCostB;
        return 1;
    }
    pEntry = &m_paGridDFSTransTable[nIndex + 256];
    if (pEntry->m_nKeyX == nTileX && pEntry->m_nKeyY == nTileY)
    {
        *pOutCostA = pEntry->m_nCostA;
        *pOutCostB = pEntry->m_nCostB;
        return 1;
    }
    *pOutCostA = 0;
    *pOutCostB = 0;
    return 0;
}

// Float-keyed twin of GridDFSTransTablePut.
void CSWSArea::PathPointDFSTransTablePut(float fX, float fY, float fCostA, float fCostB)
{
    int nIndex = PathPointDFSTransTableHash(static_cast<int>(fX), static_cast<int>(fY));
    if (m_paPathPointDFSTransTable[nIndex].m_fKeyX == fX &&
        m_paPathPointDFSTransTable[nIndex].m_fKeyY == fY)
        goto store;
    if (m_paPathPointDFSTransTable[nIndex].m_fCostA > fCostA ||
        (m_paPathPointDFSTransTable[nIndex].m_fCostA == fCostA &&
         m_paPathPointDFSTransTable[nIndex].m_fCostB > fCostB))
        goto store;
    nIndex += 256;
store:
    m_paPathPointDFSTransTable[nIndex].m_fKeyX = fX;
    m_paPathPointDFSTransTable[nIndex].m_fKeyY = fY;
    m_paPathPointDFSTransTable[nIndex].m_fCostA = fCostA;
    m_paPathPointDFSTransTable[nIndex].m_fCostB = fCostB;
}

// Float-keyed twin of GridDFSTransTableGet.
int CSWSArea::PathPointDFSTransTableGet(float fX, float fY, float* pOutCostA, float* pOutCostB)
{
    int nIndex = PathPointDFSTransTableHash(static_cast<int>(fX), static_cast<int>(fY));
    CSWSAreaPathPointTransEntry* pEntry = &m_paPathPointDFSTransTable[nIndex];
    if (pEntry->m_fKeyX == fX && pEntry->m_fKeyY == fY)
    {
        *pOutCostA = pEntry->m_fCostA;
        *pOutCostB = pEntry->m_fCostB;
        return 1;
    }
    pEntry = &m_paPathPointDFSTransTable[nIndex + 256];
    if (pEntry->m_fKeyX == fX && pEntry->m_fKeyY == fY)
    {
        *pOutCostA = pEntry->m_fCostA;
        *pOutCostB = pEntry->m_fCostB;
        return 1;
    }
    *pOutCostA = 0.0f;
    *pOutCostB = 0.0f;
    return 0;
}

// Depth-first expansion over the tile grid. Each call lays its tile into the working path and
// recurses into the successors until one reaches the goal or the manhattan budget is spent; a
// trans-table entry that already scores better prunes the branch, and the expansion watchdog
// stops runaway searches.
int CSWSArea::GridDFSearch(int nPathIndex, int nCost, int nCostBound, int nTileX, int nTileY, int nTileZ)
{
    CPathfindInformation* pPathInfo = m_pActivePathfindInfo;
    int* pGridPath = pPathInfo->m_pGridPath;
    pPathInfo->m_nGridPathLen = nPathIndex + 1;
    pGridPath[nPathIndex * 2] = nTileX;
    pGridPath[nPathIndex * 2 + 1] = nTileY;

    int nManhattan = (nTileX < 0 ? -nTileX : nTileX) + (nTileY < 0 ? -nTileY : nTileY);
    if (pPathInfo->m_nBestGridDist > nManhattan)
    {
        pPathInfo->m_nBestGridDist = nManhattan;
        memcpy(pPathInfo->m_pBestGridPath, pGridPath, (nPathIndex + 1) * 8);  // idiom-exempt: fixed-size best-path mirror copy, orig emits a real bl memcpy (fold would break parity)
        pPathInfo->m_nBestGridLen = nPathIndex + 1;
    }

    if ((nTileX == 0 && nTileY == 0 && nTileZ == 0) ||
        (nPathIndex > 0 &&
         static_cast<unsigned>(nTileX * nTileX + nTileY * nTileY + nTileZ * nTileZ) <=
             static_cast<unsigned>(pPathInfo->m_nStepTolerance * pPathInfo->m_nStepTolerance)))
        return 1;

    if (nManhattan - pPathInfo->m_nStepTolerance > nCostBound)
        return 0;

    int h = (nTileX * 17 + nTileY * 23 + 6000) % 256;
    CSWSAreaGridTransEntry* pEntry = &m_paGridDFSTransTable[h];
    if (pEntry->m_nKeyX == nTileX && pEntry->m_nKeyY == nTileY) goto probe;
    pEntry = &m_paGridDFSTransTable[h + 256];
    if (pEntry->m_nKeyX == nTileX && pEntry->m_nKeyY == nTileY) goto probe;
    goto expand;
probe:
    if (pEntry->m_nCostB >= nCostBound || pEntry->m_nCostA < nCost)
        return 0;
expand:
    {
        int nCount = m_nGridDFSExpansions;
        m_nGridDFSExpansions = nCount + 1;
        if (nCount >= GRID_DFS_EXPANSION_LIMIT)
            m_bGridDFSAborted = 1;

        CSWSAreaGridSuccessors* pSuccessors;
        int nSuccessors = GridDFSGenerateSuccessors(nCost, nCostBound, nTileX, nTileY, nTileZ,
                                                    &pSuccessors);
        int nResult = 0;
        if (nSuccessors >= 1)
        {
            for (int i = 0; i < nSuccessors; ++i)
            {
                if (m_bGridDFSAborted != 0)
                    break;
                nResult = GridDFSearch(nPathIndex + 1, nCost + pSuccessors[i].m_nCostBound,
                                       nCostBound - pSuccessors[i].m_nCostBound,
                                       pSuccessors[i].m_nX, pSuccessors[i].m_nY,
                                       pSuccessors[i].m_nZ);
                if (nResult != 0)
                    break;
            }
        }
        delete[] pSuccessors;

        GridDFSTransTablePut(nTileX, nTileY, nCost, nCostBound);
        return nResult;
    }
}

// Path-point counterpart of GridDFSearch, over the walk-route path points. The RenderDFS arms
// draw the expansion on screen.
int CSWSArea::PathPointDFSearch(int nPathIndex, float fCost, float fCostBound,
                                unsigned int nCurrent, unsigned int nTarget)
{
    if (RenderDFS != 0 && bInitnRenderDEFLines != 0 &&
        m_pActivePathfindInfo->m_oidCreature == static_cast<unsigned int>(LockRenderDev))
    {
        memset(nRenderDEFLines, 0xff, sizeof(nRenderDEFLines));
        bInitnRenderDEFLines = 0;
        nRenderDFSCount = 0;
    }

    int nLen = nPathIndex + 1;
    m_pActivePathfindInfo->m_nPathPointCount = nLen;
    m_pActivePathfindInfo->m_pPathPointList[nPathIndex] = nCurrent;

    Vector vFrom = { m_paPathPoints[nCurrent].m_point.x,
                     m_paPathPoints[nCurrent].m_point.y, 0.0f };
    float fGap = magnitude(Vector(m_paPathPoints[nTarget].m_point.x - vFrom.x,
                                  m_paPathPoints[nTarget].m_point.y - vFrom.y, 0.0f));

    if (m_pActivePathfindInfo->m_fMinGap > fGap)
    {
        m_pActivePathfindInfo->m_fSearchRadius = fGap;
        memcpy(m_pActivePathfindInfo->m_pGridPoints, m_pActivePathfindInfo->m_pPathPointList, nLen * sizeof(int));  // idiom-exempt: fixed-size best-path mirror copy, orig emits a real bl memcpy (fold would break parity)
        m_pActivePathfindInfo->m_nBestPathPointLen = nLen;
    }

    if (nCurrent == nTarget)
        return 1;
    if (fGap > fCostBound)
        return 0;

    int h = (static_cast<int>(vFrom.x) * 17 + static_cast<int>(vFrom.y) * 23 + 6000) % 256;
    CSWSAreaPathPointTransEntry* pEntry = &m_paPathPointDFSTransTable[h];
    if (pEntry->m_fKeyX == vFrom.x && pEntry->m_fKeyY == vFrom.y) goto probe;
    pEntry = &m_paPathPointDFSTransTable[h + 256];
    if (pEntry->m_fKeyX == vFrom.x && pEntry->m_fKeyY == vFrom.y) goto probe;
    goto expand;
probe:
    if (pEntry->m_fCostA < fCost || pEntry->m_fCostB >= fCostBound)
        return 0;
expand:
#if !KOTOR1
    // K2 caps the recursion depth.
    if (nPathIndex >= PATH_POINT_DFS_DEPTH_LIMIT)
    {
        m_bPathPointDFSAborted = 1;
        return 0;
    }
#endif
    {
        int nCount = m_nPathPointDFSExpansions;
        m_nPathPointDFSExpansions = nCount + 1;
        if (nCount >= PATH_POINT_DFS_EXPANSION_LIMIT)
            m_bPathPointDFSAborted = 1;

        CSWSPathPointSuccessors* pSuccessors;
        int nSuccessors = PathPointDFSGenerateSuccessors(fGap, nPathIndex, nCurrent,
                                                         &pSuccessors);

        CSWSAreaPathPointTransEntry* pSlot = &m_paPathPointDFSTransTable[h];
        if (pSlot->m_fKeyX == vFrom.x && pSlot->m_fKeyY == vFrom.y)
            goto write;
        if (pSlot->m_fCostA > fCost ||
            (pSlot->m_fCostA == fCost && pSlot->m_fCostB > fCostBound))
            goto write;
        pSlot = &m_paPathPointDFSTransTable[h + 256];
    write:
        pSlot->m_fCostA = fCost;
        pSlot->m_fCostB = fCostBound;
        pSlot->m_fKeyX = vFrom.x;
        pSlot->m_fKeyY = vFrom.y;

        int nResult = 0;
        if (nSuccessors >= 1)
        {
            int i = 1;
            CSWSPathPointSuccessors* pSucc = pSuccessors;
            do
            {
                if (m_bPathPointDFSAborted != 0)
                    break;
                float fStep = pSucc->m_fCost;
                if (RenderDFS != 0 && m_pActivePathfindInfo->m_oidCreature == static_cast<unsigned int>(LockRenderDev))
                {
                    Vector vSquare[4];
                    vSquare[0].x = vFrom.x;
                    vSquare[0].y = vFrom.y;
                    vSquare[0].z = 0.0f;
                    vSquare[1].x = vFrom.x;
                    vSquare[1].y = vFrom.y;
                    vSquare[1].z = 0.0f;
                    vSquare[2].x = m_paPathPoints[pSucc->m_nPoint].m_point.x;
                    vSquare[2].y = m_paPathPoints[pSucc->m_nPoint].m_point.y;
                    vSquare[2].z = 0.0f;
                    vSquare[3].x = m_paPathPoints[pSucc->m_nPoint].m_point.x;
                    vSquare[3].y = m_paPathPoints[pSucc->m_nPoint].m_point.y;
                    vSquare[3].z = 0.0f;
                    CServerExoApp* pApp = g_pAppManager->m_pServerExoApp;
                    vSquare[0].z = pApp->ComputeHeight(pApp->GetModule()->GetArea()->m_resRef,
                                                       vSquare[0], 1);
                    pApp = g_pAppManager->m_pServerExoApp;
                    vSquare[1].z = pApp->ComputeHeight(pApp->GetModule()->GetArea()->m_resRef,
                                                       vSquare[1], 1);
                    pApp = g_pAppManager->m_pServerExoApp;
                    vSquare[2].z = pApp->ComputeHeight(pApp->GetModule()->GetArea()->m_resRef,
                                                       vSquare[2], 1);
                    pApp = g_pAppManager->m_pServerExoApp;
                    vSquare[3].z = pApp->ComputeHeight(pApp->GetModule()->GetArea()->m_resRef,
                                                       vSquare[3], 1);
#if KOTOR1
                    // K1 declares the two-argument form.
                    unsigned int nSquareId = static_cast<unsigned int>(AurAddDrawableSquare(&vSquare[0].x, DEBUG_COLOR_WHITE));
#else
                    unsigned int nSquareId = static_cast<unsigned int>(AurAddDrawableSquare(
                        &vSquare[0].x, DEBUG_COLOR_WHITE,
                        "C:/Users/Vu/Documents/AndroidStudioProjects/KOTOR1-2/KOTOR2_Android/app/src/main/cpp/kotor2/CODE/game/servercore/nwsareapathfind.cpp",
                        7718));
#endif
                    nRenderDEFLines[nRenderDFSCount] = nSquareId;
                    nRenderDFSCount++;
                }
                nResult = PathPointDFSearch(nLen, fStep + fCost, fCostBound - fStep,
                                            pSucc->m_nPoint, nTarget);
                if (RenderDFS != 0 && m_pActivePathfindInfo->m_oidCreature == static_cast<unsigned int>(LockRenderDev))
                {
                    AurRemoveDrawableSquare(nRenderDEFLines[nRenderDFSCount]);
                    nRenderDEFLines[nRenderDFSCount] = 0xffffffff;
                    nRenderDFSCount--;
                }
                if (nResult != 0)
                    break;
                pSucc++;
            } while (i++ < nSuccessors);
        }
        delete[] pSuccessors;
        return nResult;
    }
}

// Walkable neighbour links of the current path point, one cost per link. A blocking creature
// reroutes the cost through a dodge point offset by the two personal-space radii; a placeable
// or a static door drops the link. fGap and nPathIndex are unused here.
int CSWSArea::PathPointDFSGenerateSuccessors(float fGap, int nPathIndex, unsigned int nCurrent,
                                             CSWSPathPointSuccessors** ppOut)
{
    K2_UNUSED(fGap); K2_UNUSED(nPathIndex);
    unsigned int nCount = m_paPathPoints[nCurrent].m_node.m_nNeighborCount;
    unsigned int nStart = m_paPathPoints[nCurrent].m_nNeighborStart;
    float fPersonalSpace = m_pActivePathfindInfo->m_fPersonalSpace;
    *ppOut = new CSWSPathPointSuccessors[nCount];

    Vector vFrom = { m_paPathPoints[nCurrent].m_point.x,
                     m_paPathPoints[nCurrent].m_point.y, 0.0f };
    Vector vTo = { 0.0f, 0.0f, 0.0f };
    int nSucc = 0;
    for (unsigned int i = nStart; i < nStart + nCount; i++)
    {
        CSWSAreaPathPoint* pPt = &m_paPathPoints[m_paPathPointLinks[i]];
        float fPtX = pPt->m_point.x;
        float fPtY = pPt->m_point.y;
        vTo.x = fPtX;
        vTo.y = fPtY;

        float fCost = 0.0f;
        unsigned long long oidHit;
        if (NoCreaturesOnLine(vFrom, vTo, static_cast<CPathfindInformation*>(m_pPathfindContext), 1, 0,
                              &oidHit, 0, 0) == 0)
        {
            Vector vPerp = { -(vTo.y - vFrom.y), vTo.x - vFrom.x, vTo.z - vFrom.z };
            vPerp = normalize(vPerp);
            CSWSCreature* pCreature =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidHit);
            if (pCreature == 0)
                continue;
            float fRadius = fPersonalSpace + pCreature->m_pPathfindInfo->m_fPersonalSpace;
            Vector vDodge = { pCreature->m_position.x + vPerp.x * fRadius,
                              pCreature->m_position.y + vPerp.y * fRadius, 0.0f };
            Vector vA = { vDodge.x - vFrom.x, vDodge.y - vFrom.y, vDodge.z - vFrom.z };
            fCost = magnitude(vA);
            Vector vB = { vTo.x - vDodge.x, vTo.y - vDodge.y, vTo.z - vDodge.z };
            fCost = fCost + magnitude(vB);
        }

        unsigned long long oidHitPoly;
        float fZLow = (vFrom.z < vTo.z ? vFrom.z : vTo.z) + -1000.0f;
        float fZHigh = (vFrom.z > vTo.z ? vFrom.z : vTo.z) + 1000.0f;
        if (NoNonWalkPolysDetailed(vFrom, vTo, fZLow, fZHigh, fPersonalSpace, OBJECT_INVALID,
                                   oidHitPoly, 1, 0, 0, 0, 0) == 0)
        {
            int nSkip = 0;
            CGameObject* pObj = g_pAppManager->m_pServerExoApp->GetGameObject(oidHitPoly);
            if (pObj != 0 && pObj->AsSWSObject() != 0)
            {
                CSWSPlaceable* pPlaceable = pObj->AsSWSPlaceable();
                CSWSDoor* pDoor = pObj->AsSWSDoor();
#if KOTOR1
                // K1 skips a blocking placeable without the on-screen debug post.
                if ((pDoor != 0 && pDoor->m_bStatic != 0) || pPlaceable != 0)
                    nSkip = 1;
#else
                if (pPlaceable != 0)
                {
                    char szBuf[256];
                    Vector vPos = pPlaceable->m_position;
                    const char* pszTag = pPlaceable->m_sTag.CStr();
                    snprintff(szBuf, sizeof(szBuf), "\"%s\" at ( %.1f, %.1f, %.1f )",
                              pszTag, vPos.x, vPos.y, vPos.z);
                    AurPostString(szBuf, 7, 15, 5.0f);
                    nSkip = 1;
                }
                if (pDoor != 0)
                    nSkip |= pDoor->m_bStatic;
#endif
            }
            if (nSkip != 0)
                continue;
        }

        K2_ASSUME(static_cast<unsigned int>(nSucc) < nCount);
        (*ppOut)[nSucc].m_nPoint = m_paPathPointLinks[i];
        Vector vDest = {
            m_paPathPoints[static_cast<unsigned int>(m_pActivePathfindInfo->m_nDestPathPoint)].m_point.x,
            m_paPathPoints[static_cast<unsigned int>(m_pActivePathfindInfo->m_nDestPathPoint)].m_point.y, 0.0f };
        if (fCost != 0.0f)
            (*ppOut)[nSucc].m_fCost = fCost;
        else
            (*ppOut)[nSucc].m_fCost = magnitude(vTo - vFrom);
        (*ppOut)[nSucc].m_fCostToDest = magnitude(vTo - vDest);

        CSWSPathPointSuccessors* pSucc = *ppOut;
        for (int j = nSucc; j > 0; j--)
        {
            if (pSucc[j].m_fCostToDest <= pSucc[j - 1].m_fCostToDest)
            {
                int nTmpPoint = pSucc[j].m_nPoint;
                float fTmpCost = pSucc[j].m_fCost;
                float fTmpDest = pSucc[j].m_fCostToDest;
                pSucc[j].m_nPoint = pSucc[j - 1].m_nPoint;
                pSucc[j].m_fCost = pSucc[j - 1].m_fCost;
                pSucc[j].m_fCostToDest = pSucc[j - 1].m_fCostToDest;
                pSucc[j - 1].m_nPoint = nTmpPoint;
                pSucc[j - 1].m_fCost = fTmpCost;
                pSucc[j - 1].m_fCostToDest = fTmpDest;
            }
        }
        nSucc++;
    }
    return nSucc;
}

// Finds the widest corner cut across the P1->P2->P3 polyline whose connecting segment still passes
// TestDirectLine. The cut fraction grows or shrinks by fStep, and fStep halves each round until the
// cut is under 0.1. The last passing cut is the one kept.
int CSWSArea::ComputeBestCorner(float fAx, float fAy, float fBx, float fBy, float fCx, float fCy,
                                float fHitFraction, float fRadius, float* pa1, float* pa2,
                                float* pa3, float* pa4)
{
    float fDx1 = fBx - fAx;
    float fDy1 = fBy - fAy;
    float fDist1 = __builtin_sqrtf(fDx1 * fDx1 + fDy1 * fDy1);
    if (fDist1 < 0.0001f)
        return 0;

    float fDx2 = fCx - fBx;
    float fDy2 = fCy - fBy;
    float fDist2 = __builtin_sqrtf(fDx2 * fDx2 + fDy2 * fDy2);
    if (fDist2 < 0.0001f)
        return 0;

    float fCos = (fDx1 * fDx2 + fDy1 * fDy2) / (fDist1 * fDist2);
    if (fCos >= CSWArea::SPLINE_COSINE_LIMIT)
        return 0;

    float fMin = fDist2 < fDist1 ? fDist2 : fDist1;
    float fX1p = fAx - fBx;    // P1 - P2
    float fY1p = fAy - fBy;
    float fStep = 0.25f;       // halving search step
    float fFactor = 0.5f;      // current corner-cut fraction of fMin
    int nFound = 0;

    do
    {
        float fC1x = fBx + fX1p * (fMin * fFactor) / fDist1;
        float fC1y = fBy + fY1p * (fMin * fFactor) / fDist1;
        float fC2x = fBx + fDx2 * (fMin * fFactor) / fDist2;
        float fC2y = fBy + fDy2 * (fMin * fFactor) / fDist2;
        Vector vC1 = { fC1x, fC1y, 0.0f };
        Vector vC2 = { fC2x, fC2y, 0.0f };
        if (TestDirectLine(vC1, vC2, fHitFraction, fRadius, 0, 0) == 1)
        {
            *pa1 = fC1x;
            *pa2 = fC1y;
            *pa3 = fC2x;
            *pa4 = fC2y;
            fFactor = fStep + fFactor;
            nFound = 1;
        }
        else
        {
            fFactor = fFactor - fStep;
        }
        fStep = fStep * 0.5f;
    } while (fMin * fStep > 0.1f);
    return nFound;
}

// Runs three SmoothSelectNodes passes. The first re-selects, the second re-selects with direction
// continuity, and the third writes the result into the output pair.
void CSWSArea::SmoothSelection(int nCount, float* pFloats, int* pOutCount, float** ppOutFloats)
{
    int nCount1;
    float* pFloats1;
    SmoothSelectNodes(nCount, pFloats, &nCount1, &pFloats1, 1, 0);

    int nCount2;
    float* pFloats2;
    SmoothSelectNodes(nCount1, pFloats1, &nCount2, &pFloats2, 1, 1);

    SmoothSelectNodes(nCount2, pFloats2, pOutCount, ppOutFloats, 0, 1);
    delete[] pFloats1;
    delete[] pFloats2;
}

// Corner-cuts the raw point list at every second vertex, through up to three nested
// ComputeBestCorner probes, and appends the trimmed polyline to the output path.
void CSWSArea::SmoothCornerOptimize(int nCount, float* pFloats, int* pOutCount, float** ppOutFloats)
{
    CPathfindInformation* pPathInfo = m_pActivePathfindInfo;
    float fPersonalSpace = pPathInfo->m_fPersonalSpace;
    float fHeight = pPathInfo->m_fHeight;

    float* pOut = new float[nCount * 4];
    *ppOutFloats = pOut;
    (*ppOutFloats)[0] = pFloats[0];
    (*ppOutFloats)[1] = pFloats[1];
    int nOut = 2;
    *pOutCount = 2;

    int i = 2;
    if (nCount - 2 >= 3)
    {
        float* pNext = pFloats + 5;
        for (i = 2; i < nCount - 2; i += 2)
        {
            float c1x, c1y, c2x, c2y;
            int r1 = ComputeBestCorner((*ppOutFloats)[*pOutCount - 2], (*ppOutFloats)[*pOutCount - 1],
                                       pNext[-3], pNext[-2], pNext[-1], pNext[0],
                                       fPersonalSpace, fHeight, &c1x, &c1y, &c2x, &c2y);
            if (r1 == 1)
            {
                float d1x, d1y, d2x, d2y;
                int r2 = ComputeBestCorner((*ppOutFloats)[*pOutCount - 2], (*ppOutFloats)[*pOutCount - 1],
                                           c1x, c1y, c2x, c2y,
                                           fPersonalSpace, fHeight, &d1x, &d1y, &d2x, &d2y);
                if (r2 == 1)
                {
                    float e1x, e1y, e2x, e2y;
                    int r3 = ComputeBestCorner(d2x, d2y, c2x, c2y, pNext[-1], pNext[0],
                                               fPersonalSpace, fHeight, &e1x, &e1y, &e2x, &e2y);
                    int n = *pOutCount;
                    (*ppOutFloats)[n] = d1x;
                    (*ppOutFloats)[n + 1] = d1y;
                    (*ppOutFloats)[n + 2] = d2x;
                    (*ppOutFloats)[n + 3] = d2y;
                    nOut = n + 4;
                    *pOutCount = nOut;
                    if (r3 == 1)
                    {
                        (*ppOutFloats)[*pOutCount] = e1x;
                        (*ppOutFloats)[*pOutCount + 1] = e1y;
                        nOut = *pOutCount + 2;
                        *pOutCount = nOut;
                        (*ppOutFloats)[*pOutCount] = e2x;
                        nOut += 2;
                        (*ppOutFloats)[*pOutCount + 1] = e2y;
                    }
                    else
                    {
                        (*ppOutFloats)[n + 4] = c2x;
                        (*ppOutFloats)[n + 5] = c2y;
                        nOut = n + 6;
                    }
                    *pOutCount = nOut;
                }
                else
                {
                    int r3 = ComputeBestCorner(c1x, c1y, c2x, c2y, pNext[-1], pNext[0],
                                               fPersonalSpace, fHeight, &d1x, &d1y, &d2x, &d2y);
                    int n = *pOutCount;
                    (*ppOutFloats)[n] = c1x;
                    (*ppOutFloats)[n + 1] = c1y;
                    nOut = n + 2;
                    *pOutCount = nOut;
                    if (r3 == 1)
                    {
                        (*ppOutFloats)[n + 2] = d1x;
                        (*ppOutFloats)[n + 3] = d1y;
                        (*ppOutFloats)[n + 4] = d2x;
                        nOut = n + 6;
                        (*ppOutFloats)[n + 5] = d2y;
                    }
                    else
                    {
                        (*ppOutFloats)[n + 2] = c2x;
                        (*ppOutFloats)[n + 3] = c2y;
                        nOut = n + 4;
                    }
                    *pOutCount = nOut;
                }
            }
            else
            {
                (*ppOutFloats)[*pOutCount] = pNext[-3];
                (*ppOutFloats)[*pOutCount + 1] = pNext[-2];
                nOut = *pOutCount + 2;
                *pOutCount = nOut;
            }
            pNext += 2;
        }
    }
    if (nCount > 2)
    {
        (*ppOutFloats)[*pOutCount] = pFloats[i];
        (*ppOutFloats)[*pOutCount + 1] = pFloats[i | 1];
        *pOutCount += 2;
    }
}

// Selects each next vertex of the smoothed path, optionally re-choosing the target with
// TestDirectLine probes and subdividing the edge toward it.
void CSWSArea::SmoothSelectNodes(int nCount, float* pFloats, int* pOutCount,
                                float** ppOutFloats, int bSubdivide, int bReselect)
{
    CPathfindInformation* pPathInfo = m_pActivePathfindInfo;
    float fPersonalSpace = pPathInfo->m_fPersonalSpace;
    float fHeight = pPathInfo->m_fHeight;

    float fDist = ComputePathDistance(nCount, pFloats);
    *pOutCount = 0;
    float* pOut = new float[nCount + static_cast<int>(fDist) * 2];
    *ppOutFloats = pOut;
    pOut[0] = pFloats[0];
    pOut[1] = pFloats[1];
    *pOutCount = 2;

    int nSteps = nCount - 1;
    if (nSteps <= 1)
        return;

    int nHalf = nSteps >> 1;
    int nDiv = nCount >> 1;
    int i = 0;
    int nSel = -1;
    int bLast = 0;
    float* pScratch = 0;
    float fSelX = 0.0f, fSelY = 0.0f;

    for (;;)
    {
        int nAdvance = 0;
        if (bReselect == 1)
        {
            int j = nDiv - 1 - i;
            nAdvance = 0;
            while (j > 0)
            {
                float fLastX = pOut[*pOutCount - 2];
                float fLastY = pOut[*pOutCount - 1];
                Vector vLast(fLastX, fLastY, 0.0f);
                Vector vNext(pFloats[(j + i) * 2], pFloats[(j + i) * 2 + 1], 0.0f);
                if (TestDirectLine(vLast, vNext, fPersonalSpace, fHeight, 0, 0) == 1)
                {
                    nAdvance = j;
                    j = 0;
                }
                else if (j > 4)
                    j /= 2;
                else
                    j--;
            }
            i += nAdvance;
        }
        else
        {
            nAdvance = (nSel <= 0) ? 1 : 0;
            i += nAdvance;
        }

        if (nAdvance != 0)
        {
            fSelX = pFloats[i * 2];
            fSelY = pFloats[i * 2 + 1];
            if (pScratch)
                delete[] pScratch;
            pScratch = 0;
            nSel = 0;
        }
        else if (bLast & 1)
        {
            fSelX = pScratch[nSel * 2 - 2];
            fSelY = pScratch[nSel * 2 - 1];
            nSel--;
        }
        else
        {
            fSelX = pFloats[(i + 1) * 2];
            fSelY = pFloats[(i + 1) * 2 + 1];
            i++;
        }

        if (bSubdivide)
        {
            float fDx = pOut[*pOutCount - 2] - fSelX;
            float fDy = pOut[*pOutCount - 1] - fSelY;
            Vector vDelta(fDx, fDy, 0.0f);
            float fMag = magnitude(vDelta);
            if (static_cast<double>(fMag) >= 1.1)
            {
                float fLastX = pOut[*pOutCount - 2];
                float fLastY = pOut[*pOutCount - 1];
                if (pScratch)
                    delete[] pScratch;
                int n = static_cast<int>(fMag) * 2 + 2;
                float* pNew = new float[n];
                if (fMag > 0.0f)
                {
                    float fStepX = fSelX - fLastX;
                    float fStepY = fSelY - fLastY;
                    int k = 0;
                    float t = fMag;
                    do
                    {
                        pNew[k * 2] = fLastX + fStepX * t / fMag;
                        pNew[k * 2 + 1] = fLastY + fStepY * t / fMag;
                        k++;
                        t -= 1.0f;
                    } while (t > 0.0f);
                    nSel = k - 1;
                    fSelX = pNew[k * 2 - 2];
                    fSelY = pNew[k * 2 - 1];
                }
                else
                {
                    nSel = 0;
                }
                pScratch = pNew;
            }
        }

        (*ppOutFloats)[*pOutCount] = fSelX;
        (*ppOutFloats)[*pOutCount + 1] = fSelY;
        *pOutCount += 2;
        bLast = (nSel > 0);

        if (i < nHalf || nSel > 0)
            continue;
        break;
    }

    if (pScratch)
        delete[] pScratch;
}

// Runs a budgeted slice of SmoothSelectNodes. The walk state (raw-point cursor, pending
// subdivisions and their scratch) lives in the path info, so the next call resumes where this one
// stopped.
int CSWSArea::SmoothSelectNodesRestartable(int nCount, float* pFloats, int* pOutCount,
                                           float** ppOutFloats, int bSubdivide, int bReselect,
                                           float fSmoothThreshold)
{
    unsigned long long nStartTime = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    float fPersonalSpace = m_pActivePathfindInfo->m_fPersonalSpace + 0.001f;
    float fHeight = m_pActivePathfindInfo->m_fHeight;

    int nExtra = static_cast<int>(ComputePathDistance(nCount, pFloats)) * 2;

    int nSel = m_pActivePathfindInfo->m_nSmoothResumeSel;
    float* pScratch = m_pActivePathfindInfo->m_pSmoothResumeFloats;
    int i = m_pActivePathfindInfo->m_nSmoothResumeIndex;

    if (*ppOutFloats == 0)
    {
        *pOutCount = 0;
        float* pOut = new float[nCount + nExtra];
        *ppOutFloats = pOut;
        pOut[0] = pFloats[i * 2];
        pOut[1] = pFloats[i * 2 + 1];
        *pOutCount = 2;
    }

    int nHalf = (nCount - 1) >> 1;
    int nDiv = nCount >> 1;
    float fSelX = 0.0f, fSelY = 0.0f;

    while (i < nHalf || nSel > 0)
    {
        int nAdvance = 0;
        if (bReselect == 1)
        {
            int j = nDiv - 1 - i;
            if (j >= 1)
            {
                nAdvance = 0;
                do
                {
                    float fLastX = (*ppOutFloats)[*pOutCount - 2];
                    float fLastY = (*ppOutFloats)[*pOutCount - 1];
                    Vector vLast(fLastX, fLastY, 0.0f);
                    Vector vNext(pFloats[(j + i) * 2], pFloats[(j + i) * 2 + 1], 0.0f);
                    if (TestDirectLine(vLast, vNext, fPersonalSpace, fHeight, 0, 0) == 1)
                    {
                        nAdvance += j;
                        j = 0;
                    }
                    if (j > 4)
                        j >>= 1;
                    else
                        j--;
                    bDumpData = 0;
                    g_bRenderClipWalkable = 0;
                } while (j > 0);
            }
        }
        else
        {
            nAdvance = (nSel < 1) ? 1 : 0;
        }
        i += nAdvance;

        if (nAdvance != 0)
        {
            fSelX = pFloats[i * 2];
            fSelY = pFloats[i * 2 + 1];
            if (pScratch)
            {
                delete[] pScratch;
                m_pActivePathfindInfo->m_pSmoothResumeFloats = 0;
            }
            pScratch = 0;
            nSel = 0;
        }
        else if (nSel >= 1)
        {
            fSelX = pScratch[nSel * 2 - 2];
            fSelY = pScratch[nSel * 2 - 1];
            nSel--;
        }
        else
        {
            fSelX = pFloats[(i + 1) * 2];
            fSelY = pFloats[(i + 1) * 2 + 1];
            i++;
        }

        if (bSubdivide)
        {
            Vector vDelta;
            vDelta.x = (*ppOutFloats)[*pOutCount - 2] - fSelX;
            vDelta.y = (*ppOutFloats)[*pOutCount - 1] - fSelY;
            vDelta.z = 0.0f;
            float fMag = magnitude(vDelta);
            if (fSmoothThreshold <= fMag)
            {
                float fLastX = (*ppOutFloats)[*pOutCount - 2];
                float fLastY = (*ppOutFloats)[*pOutCount - 1];
                if (pScratch)
                {
                    delete[] pScratch;
                    m_pActivePathfindInfo->m_pSmoothResumeFloats = 0;
                }
                int n = static_cast<int>(fMag) * 2 + 2;
                float* pNew = new float[n];
                m_pActivePathfindInfo->m_pSmoothResumeFloats = pNew;
                if (fMag > 0.0f)
                {
                    float fDx = fSelX - fLastX;
                    float fDy = fSelY - fLastY;
                    int k = 0;
                    float t = fMag;
                    do
                    {
                        K2_ASSUME(k * 2 + 1 < n);
                        pNew[k * 2]     = fLastX + fDx * t / fMag;
                        pNew[k * 2 + 1] = fLastY + fDy * t / fMag;
                        k++;
                        t -= 1.0f;
                    } while (t > 0.0f);
                    nSel = k - 1;
                    fSelX = pNew[k * 2 - 2];
                    fSelY = pNew[k * 2 - 1];
                }
                else
                {
                    nSel = 0;
                }
                pScratch = pNew;
            }
        }

        (*ppOutFloats)[*pOutCount] = fSelX;
        (*ppOutFloats)[*pOutCount + 1] = fSelY;
        *pOutCount += 2;

        if (g_pExoBase->m_pTimers->GetHighResolutionTimer() - nStartTime > static_cast<uint64_t>(SmoothTime))
        {
            m_pActivePathfindInfo->m_nSmoothResumeSel = nSel;
            m_pActivePathfindInfo->m_nSmoothResumeIndex = i;
            return 1;
        }
    }

    if (pScratch)
    {
        delete[] pScratch;
        m_pActivePathfindInfo->m_pSmoothResumeFloats = 0;
    }
    m_pActivePathfindInfo->m_nSmoothResumeSel = -1;
    m_pActivePathfindInfo->m_nSmoothResumeIndex = 0;
    return 0;
}

// Runs one SmoothSelectNodesRestartable pass per call; a pass that runs out of time parks its
// partial spline in the path info and returns 1 so the next call resumes it.
int CSWSArea::SmoothSelectionRestartable(int nCount, float* pFloats, int* pOutCount,
                                         float** ppOutFloats)
{
    CPathfindInformation* pPathInfo = m_pActivePathfindInfo;
    int nParkA = pPathInfo->m_nSmoothingFirstFloats;        // smoothing pass 2 output count
    float* pParkA = pPathInfo->m_paSmoothingFirstWayPoints;    // smoothing pass 2 output floats
    int nParkB = pPathInfo->m_nSmoothingSecondFloats;        // smoothing pass 3 output count
    float* pParkB = pPathInfo->m_paSmoothingSecondWayPoints;    // smoothing pass 3 output floats

    if (pPathInfo->m_nSmoothStep == CPathfindInformation::SMOOTHING_STEP_2)
    {
        int nRet = SmoothSelectNodesRestartable(nCount, pFloats, &nParkA, &pParkA,
                                                1, 0, 5.0f);
        m_pActivePathfindInfo->SetSmoothingFirstWayPoints(pParkA);
        pPathInfo = m_pActivePathfindInfo;
        pPathInfo->m_nSmoothingFirstFloats = nParkA;
        if (nRet == 1)
            return 1;
        pPathInfo->m_nSmoothResumeIndex = 0;
        pPathInfo->m_nSmoothResumeSel = -1;
        pPathInfo->m_nSmoothStep = CPathfindInformation::SMOOTHING_STEP_3;
    }

    if (pPathInfo->m_nSmoothStep == CPathfindInformation::SMOOTHING_STEP_3)
    {
        int nRet = SmoothSelectNodesRestartable(nParkA, pParkA, &nParkB, &pParkB,
                                                1, 1, 1.1f);
        m_pActivePathfindInfo->SetSmoothingSecondWayPoints(pParkB);
        pPathInfo = m_pActivePathfindInfo;
        pPathInfo->m_nSmoothingSecondFloats = nParkB;
        if (nRet == 1)
            return 1;
        pPathInfo->m_nSmoothResumeIndex = 0;
        pPathInfo->m_nSmoothResumeSel = -1;
        pPathInfo->m_nSmoothStep = CPathfindInformation::SMOOTHING_STEP_4;
    }

    if (pPathInfo->m_nSmoothStep == CPathfindInformation::SMOOTHING_STEP_4)
    {
        int nRet = SmoothSelectNodesRestartable(nParkB, pParkB, pOutCount, ppOutFloats,
                                                0, 1, 1.1f);
        if (nRet == 1)
            return 1;
    }

    m_pActivePathfindInfo->SetSmoothingFirstWayPoints(0);
    m_pActivePathfindInfo->SetSmoothingSecondWayPoints(0);
    return 0;
}

// Nearest walkable path point to v, in *nOut. The route array is scanned in a window that
// widens by 10 on each pass, and every candidate must also clear TestDirectLine from v.
int CSWSArea::ClosestPathPoint(const Vector& v, unsigned int& nOut)
{
    if (m_paPathPoints == 0 || m_pActivePathfindInfo == 0) {
        nOut = 0;
        return 1;
    }

    unsigned int nCount = m_nPathPointCount;
    float fFirstX = m_paPathPoints[0].m_node.m_x;
    float fLastX = m_paPathPoints[nCount - 1].m_node.m_x;
    float fHitFraction = m_pActivePathfindInfo->m_fPersonalSpace;

    float fMinX = v.x - 10.0f;
    float fMaxX = v.x + 10.0f;
    float fTinyXP = v.x + 0.01f;
    float fTinyXN = v.x - 0.01f;
    float fTinyYP = v.y + 0.01f;
    float fTinyYN = v.y - 0.01f;
    float fBestDist = 3.402823466e+38f;
    float fMaxY = v.y;
    float fMinY = v.y;

    unsigned int i = 0;
    int j = 0;
    int nBest = -1;
    int nTries = 49;
    float px = 0.0f;
    float py = 0.0f;

    do {
        fMaxY += 10.0f;
        fMinY -= 10.0f;
        if (i >= nCount)
            goto expand;

        j = i;
        nBest = -1;

    inner:
        px = m_paPathPoints[j].m_node.m_x;
        if (px < fMinX)
            goto skip;
        if (!(px > fTinyXP))
            goto region;
        if (!(px < fTinyXN))
            goto region;
        py = m_paPathPoints[j].m_node.m_y;
        if (!(py > fTinyYP))
            goto region;
        if (py < fTinyYN)
            goto step;

    region:
        if (px > fMaxX)
            goto done;
        py = m_paPathPoints[j].m_node.m_y;
        if (py < fMinY)
            goto step;
        if (py > fMaxY)
            goto step;
        {
            float dx = px - v.x;
            float dy = py - v.y;
            float fDist2 = dx * dx + dy * dy;
            if (!(fDist2 < fBestDist))
                goto step;
            Vector v1 = {v.x, v.y, 0.0f};
            Vector v2 = {px, py, 0.0f};
            CWalkHitInfo hitInfo = {};
            hitInfo.m_hitObjectId = OBJECT_INVALID;
            float fHeight = m_pActivePathfindInfo->m_fHeight;
            int nRet = TestDirectLine(v1, v2, fHitFraction, fHeight,
                                     static_cast<int>(reinterpret_cast<uintptr_t>(&hitInfo)), &hitInfo);
            if (nRet == 1 || nRet == -3) {
                nBest = j;
                fBestDist = fDist2;
            }
        }
        goto step;

    skip:
        i = j;
    step:
        j++;
        if (static_cast<unsigned int>(j) >= nCount)
            goto done;
        goto inner;

    done:
        if (nBest != -1)
            goto found;

    expand:
        fMinX -= 10.0f;
        fMaxX += 10.0f;
        fTinyXP -= 10.0f;
        fTinyXN += 10.0f;
        fMinX = (fMinX > fFirstX) ? fMinX : fFirstX;
        fMaxX = (fMaxX < fLastX) ? fMaxX : fLastX;
        fTinyXP = (fTinyXP > fFirstX) ? fTinyXP : fFirstX;
        fTinyXN = (fTinyXN < fLastX) ? fTinyXN : fLastX;
        if (i >= 1) {
            while (m_paPathPoints[i].m_node.m_x > fMinX) {
                i--;
                if (i == 0) break;
            }
        }
        fTinyYP -= 10.0f;
        fTinyYN += 10.0f;
    } while (nTries-- != 0);

    nBest = -1;
found:
    nOut = nBest;
    return (nBest != -1);
}

// Squared distance between two points, z included.
static inline float d2(const Vector& a, const Vector& b)
{
    float x = a.x - b.x;
    float y = a.y - b.y;
    float z = a.z - b.z;
    return x * x + y * y + z * z;
}

// Drives a creature's walk path. Advances pPathInfo->m_nPathState through the startpoints,
// pathfind, gridstart, gridend and smooth states, running one budgeted slice per call, and returns
// an ACTION_* code. ACTION_IN_PROGRESS asks to be called again next tick.
int CSWSArea::PlotPathPointPath(CPathfindInformation* pPathInfo, unsigned long long nTimeOut)
{
    Vector vFrom = pPathInfo->m_vStartPoint;
    Vector vTo = pPathInfo->m_goal.m_vPoint;
    int nState = pPathInfo->m_nPathState;
    unsigned long long nTimer = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    int nStartPathPoint;
    int nEndPathPoint;
    int nAction = nState;  // returned unchanged when no arm assigns an action
    const char* sGridFailMsg;

    // In the start-points state, snap both endpoints onto the walkmesh and resolve them to the
    // nearest walk-route path points.
    if (nState == CPathfindInformation::PATH_POINT_STARTPOINTS)
    {
        int bFromSafe = TestSafeLocationPoint(vFrom, pPathInfo);
        int bToSafe = TestSafeLocationPoint(vTo, pPathInfo);
        if (bFromSafe == 0 || bToSafe == 0)
        {
            if (bFromSafe == 0)
            {
                Vector vFromBackup = vFrom;
                ComputeSafeLocation(vFrom, 2.0f, pPathInfo, 1, &vFrom);
                if (vFrom != vFromBackup)
                {
                    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->
                        GetCreatureByGameObjectID(pPathInfo->m_oidCreature);
                    if (pCreature)
                        pCreature->SetPosition(vFrom, 1, 1, 0);
                }
                pPathInfo->m_vStartPoint = vFrom;
                PositionWalkable(vFrom);
            }
            if (bToSafe == 0)
            {
                ComputeSafeLocation(vTo, 2.0f, pPathInfo, 1, &vTo);
                pPathInfo->m_goal.m_vPoint = vTo;
                PositionWalkable(vTo);
            }
        }
        if (ClosestPathPoint(vFrom, reinterpret_cast<unsigned int&>(nStartPathPoint)) == 0 ||
            ClosestPathPoint(vTo, reinterpret_cast<unsigned int&>(nEndPathPoint)) == 0)
        {
            pPathInfo->HandlePathfindError(2,
                const_cast<char*>("Error could not find closest path node to start or end"));
            pPathInfo->m_nPathState = nState;
            return CSWSObject::ACTION_FAILED;
        }
        pPathInfo->m_nSourcePathPoint = nStartPathPoint;
        pPathInfo->m_nDestPathPoint = nEndPathPoint;
        pPathInfo->m_nPathState = CPathfindInformation::PATH_POINT_PATHFIND;
        if (g_pExoBase->m_pTimers->GetHighResolutionTimer() - nTimer > nTimeOut)
            return CSWSObject::ACTION_IN_PROGRESS;
    }
    else
    {
        nStartPathPoint = pPathInfo->m_nSourcePathPoint;
        nEndPathPoint = pPathInfo->m_nDestPathPoint;
    }
    nState = pPathInfo->m_nPathState;

    // When start and end resolve to the same path point, the route is one point, straight to the
    // grid.
    if (nStartPathPoint == nEndPathPoint)
    {
        if (nState == CPathfindInformation::PATH_POINT_PATHFIND)
        {
            pPathInfo->m_nPathPointCount = 1;
            int* pList = new int[1];
            pList[0] = nEndPathPoint;
            pPathInfo->SetPathPointStepsArray(pList);
            pPathInfo->m_nPathState = CPathfindInformation::PATH_POINT_GRIDSTART;
            nState = CPathfindInformation::PATH_POINT_GRIDSTART;
        }
    }

    // In PATH_POINT_PATHFIND, grow the search radius by a quarter of the from->to gap and re-run
    // PathPointDFSearch. The search state is parked on the path info so the next tick resumes
    // the same radius instead of restarting it.
    if (nState == CPathfindInformation::PATH_POINT_PATHFIND)
    {
        pPathInfo->m_nSourcePathPoint = nStartPathPoint;
        pPathInfo->m_nDestPathPoint = nEndPathPoint;

        const float fGap = magnitude(vTo - vFrom);
        const float fScale = fGap * 0.25f;

        m_nPathPointStepCount = m_nPathPointCount;
        {
            float* pPaths = new float[m_nPathPointCount];
            pPathInfo->SetPathPointStepsArray(reinterpret_cast<int*>(pPaths));
        }
        float* pGridPoints = pPathInfo->m_pGridPoints;
        if (pGridPoints == 0)
        {
            pGridPoints = new float[m_nPathPointCount];
            pPathInfo->m_pGridPoints = pGridPoints;
        }

        // Resume the parked search when both endpoints still match it; otherwise blank the
        // table and restart from a base radius.
        float fSearchRadius;
        if (!(d2(vFrom, pPathInfo->m_vCurrentFrom) >= 0.0001f) &&
            !(d2(vTo, pPathInfo->m_vCurrentTo) >= 0.0001f))
        {
            m_paPathPointDFSTransTable =
                static_cast<CSWSAreaPathPointTransEntry*>(pPathInfo->m_pPathPointTable);
            if (m_paPathPointDFSTransTable == 0)
            {
                m_paPathPointDFSTransTable = new CSWSAreaPathPointTransEntry[PATH_POINT_TRANS_TABLE_SIZE];
                static const CSWSAreaPathPointTransEntry kBlankPath = { 0.0f, 0.0f, 3.40282347e+37f, 0.0f };
                CSWSAreaPathPointTransEntry* const pEnd = m_paPathPointDFSTransTable + PATH_POINT_TRANS_TABLE_SIZE;
                for (CSWSAreaPathPointTransEntry* p = m_paPathPointDFSTransTable; p != pEnd; p += 8) {
                    p[2] = kBlankPath;
                    p[3] = kBlankPath;
                    p[0] = kBlankPath;
                    p[1] = kBlankPath;
                    p[6] = kBlankPath;
                    p[7] = kBlankPath;
                    p[4] = kBlankPath;
                    p[5] = kBlankPath;
                }
            }
            fSearchRadius = fScale + pPathInfo->m_fCurrentRadius;
        }
        else
        {
            if (pPathInfo->m_pPathPointTable)
                m_paPathPointDFSTransTable =
                    static_cast<CSWSAreaPathPointTransEntry*>(pPathInfo->m_pPathPointTable);
            PathPointDFSTransTableInitialize();
            fSearchRadius = fScale + 10.0f;
            pPathInfo->m_fSearchRadius = fSearchRadius;
        }

        m_nPathPointDFSExpansions = 0;
        m_bPathPointDFSAborted = 0;
        if (pPathInfo->m_fMinGap > fGap)
            pPathInfo->m_fMinGap = fGap;
        nAction = CSWSObject::ACTION_IN_PROGRESS;
        // The clock is sampled once before the loop and re-sampled at the end of each body,
        // so every budget test reads the previous body's clock.
        unsigned long long nNow = g_pExoBase->m_pTimers->GetHighResolutionTimer();
        int nLoopCount = 0;
        for (;;)
        {
            if (nLoopCount != 0)
            {
                if (nAction != CSWSObject::ACTION_IN_PROGRESS)
                    break;
                if (nNow - nTimer >= nTimeOut)
                    break;
            }
            fSearchRadius += fScale;
            m_nPathPointDFSExpansions = 0;
            m_bPathPointDFSAborted = 0;
            m_pPathfindContext = pPathInfo;
            int nDfs = PathPointDFSearch(0, 0.0f, fSearchRadius, nStartPathPoint, nEndPathPoint);
            if (m_bPathPointDFSAborted != 0)
            {
                // The recursion ceiling was hit, so abandon the walk.
                if (m_paPathPointDFSTransTable)
                    delete[] m_paPathPointDFSTransTable;
                m_paPathPointDFSTransTable = 0;
                pPathInfo->m_fCurrentRadius = 0;
                pPathInfo->m_vCurrentFrom = Vector(0.0f, 0.0f, 0.0f);
                pPathInfo->m_vCurrentTo = Vector(0.0f, 0.0f, 0.0f);
                pPathInfo->m_pPathPointTable = 0;
                static_cast<CPathfindInformation*>(m_pPathfindContext)
                    ->HandlePathfindError(0, const_cast<char*>("we exceeded 4096 levels of searching.. "));
                return CSWSObject::ACTION_FAILED;
            }
            m_pPathfindContext = 0;
            if (nDfs == 1)
                nAction = CSWSObject::ACTION_COMPLETE;
            nNow = g_pExoBase->m_pTimers->GetHighResolutionTimer();
            --nLoopCount;
        }

        // With the budget spent, park the search so the next tick resumes the same radius.
        if (nAction == CSWSObject::ACTION_IN_PROGRESS)
        {
            pPathInfo->m_vCurrentFrom = vFrom;
            pPathInfo->m_vCurrentTo = vTo;
            pPathInfo->m_fCurrentRadius = fSearchRadius;
            pPathInfo->m_pPathPointTable = m_paPathPointDFSTransTable;
            m_paPathPointDFSTransTable = 0;
        }
        else
        {
            if (m_paPathPointDFSTransTable)
                delete[] m_paPathPointDFSTransTable;
            m_paPathPointDFSTransTable = 0;
            pPathInfo->m_fCurrentRadius = 0;
            pPathInfo->m_vCurrentFrom = Vector(0.0f, 0.0f, 0.0f);
            pPathInfo->m_vCurrentTo = Vector(0.0f, 0.0f, 0.0f);
            pPathInfo->m_pPathPointTable = 0;
            if (pGridPoints)
                delete[] pGridPoints;
            pPathInfo->m_pGridPoints = 0;
        }
        // The route is found, but the walk is not. The grid legs run from the next tick.
        if (nAction == CSWSObject::ACTION_COMPLETE)
        {
            pPathInfo->m_nPathState = CPathfindInformation::PATH_POINT_GRIDSTART;
            return CSWSObject::ACTION_IN_PROGRESS;
        }
    }

    // In PATH_POINT_GRIDSTART, solve the from->start-path-point leg. A clear TestDirectLine
    // captures the segment directly, otherwise PlotGridPath walks the creature over and its
    // grid path is taken from the park. On failure the first path point is dropped and the walk
    // is retried here.
    if (nState == CPathfindInformation::PATH_POINT_GRIDSTART)
    {
        const Vector vPathPoint(m_paPathPoints[nStartPathPoint].m_point.x,
                                m_paPathPoints[nStartPathPoint].m_point.y, 0.0f);
        int nHitFraction = pPathInfo->m_nPersonalSpaceBits;
        pPathInfo->m_bIgnoreCrePath = (nStartPathPoint != nEndPathPoint);
        int nGrid = TestDirectLine(vFrom, vPathPoint, *reinterpret_cast<const float*>(&nHitFraction),
                                   pPathInfo->m_fHeight, 0, 0);
        if (nGrid == 1)
        {
            // With a clear line to the path point, capture the segment directly.
            float* pSeg = new float[4];
            pSeg[0] = vFrom.x;
            pSeg[1] = vFrom.y;
            pSeg[2] = vPathPoint.x;
            pSeg[3] = vPathPoint.y;
            pPathInfo->SetPathNodeStartPath(4, pSeg);
            nGrid = CSWSObject::ACTION_COMPLETE;
        }
        else
        {
            pPathInfo->m_vSavedFrom = vFrom;
            pPathInfo->m_vSavedTo = vTo;
            pPathInfo->m_goal.m_vPoint.x = m_paPathPoints[nStartPathPoint].m_node.m_x;
            pPathInfo->m_goal.m_vPoint.y = m_paPathPoints[nStartPathPoint].m_node.m_y;
            pPathInfo->m_nPathType = 1;
            nGrid = PlotGridPath(pPathInfo, nTimeOut);
            pPathInfo->m_nPathType = 0;
            vFrom = pPathInfo->m_vSavedFrom;
            vTo = pPathInfo->m_vSavedTo;
            pPathInfo->m_goal.m_vPoint.x = vTo.x;
            pPathInfo->m_goal.m_vPoint.y = vTo.y;
            if (nGrid == CSWSObject::ACTION_COMPLETE)
            {
                CPathfindInformation* pGridInfo = m_pActivePathfindInfo;
                pPathInfo->SetPathNodeStartPath(pGridInfo->m_nRawWayPointFloats, pGridInfo->m_paRawWayPoints);
                pGridInfo->m_nRawWayPointFloats = 0;
                pGridInfo->m_paRawWayPoints = 0;
            }
        }
        pPathInfo->m_bIgnoreCrePath = 0;
        if (nGrid == CSWSObject::ACTION_COMPLETE)
        {
            // Only a complete leg advances and pays the budget check.
            unsigned long long nGridTimer = g_pExoBase->m_pTimers->GetHighResolutionTimer();
            pPathInfo->m_nPathState = CPathfindInformation::PATH_POINT_GRIDEND;
            if (nGridTimer - nTimer > nTimeOut)
                return CSWSObject::ACTION_IN_PROGRESS;
        }
        if (nGrid == CSWSObject::ACTION_FAILED)
        {
            if (pPathInfo->m_nPathPointCount < 2)
            {
                sGridFailMsg = const_cast<char*>("    failed to grid based pathfind from the creatures position to the starting path point.");
                goto GridPointFail;
            }
            int* pList = pPathInfo->m_pPathPointList;
            for (int i = 1; i < pPathInfo->m_nPathPointCount; i++)
                pList[i - 1] = pList[i];
            --pPathInfo->m_nPathPointCount;
            pPathInfo->m_nSourcePathPoint = pList[0];
            nStartPathPoint = pList[0];
            pPathInfo->m_nPathState = CPathfindInformation::PATH_POINT_GRIDSTART;
            return CSWSObject::ACTION_IN_PROGRESS;
        }
        else if (nGrid == CSWSObject::ACTION_IN_PROGRESS)
        {
            return CSWSObject::ACTION_IN_PROGRESS;
        }
        nState = pPathInfo->m_nPathState;
    }

    // PATH_POINT_GRIDEND is the same as the start leg, for the end-path-point->to segment. On
    // failure the destination path point rolls back one and the walk is retried here.
    if (nState == CPathfindInformation::PATH_POINT_GRIDEND)
    {
        const Vector vPathPoint(m_paPathPoints[nEndPathPoint].m_point.x,
                                m_paPathPoints[nEndPathPoint].m_point.y, 0.0f);
        int nHitFraction = pPathInfo->m_nPersonalSpaceBits;
        pPathInfo->m_bIgnoreCrePath = (nStartPathPoint != nEndPathPoint);
        int nGrid = TestDirectLine(vTo, vPathPoint, *reinterpret_cast<const float*>(&nHitFraction),
                                   pPathInfo->m_fHeight, 0, 0);
        if (nGrid == 1 || nGrid == -3)
        {
            // A clear line, or a clip that ended on the walkmesh, captures the segment.
            float* pSeg = new float[4];
            pSeg[0] = vPathPoint.x;
            pSeg[1] = vPathPoint.y;
            pSeg[2] = vTo.x;
            pSeg[3] = vTo.y;
            pPathInfo->SetPathNodeEndPath(4, pSeg);
            nGrid = CSWSObject::ACTION_COMPLETE;
        }
        else
        {
            pPathInfo->m_vSavedFrom = vFrom;
            pPathInfo->m_vSavedTo = vTo;
            pPathInfo->m_vStartPoint.x = m_paPathPoints[nEndPathPoint].m_node.m_x;
            pPathInfo->m_vStartPoint.y = m_paPathPoints[nEndPathPoint].m_node.m_y;
            pPathInfo->m_nPathType = 1;
            nGrid = PlotGridPath(pPathInfo, nTimeOut);
            pPathInfo->m_nPathType = 0;
            vFrom = pPathInfo->m_vSavedFrom;
            vTo = pPathInfo->m_vSavedTo;
            pPathInfo->m_vStartPoint.x = vFrom.x;
            pPathInfo->m_vStartPoint.y = vFrom.y;
            if (nGrid == CSWSObject::ACTION_COMPLETE)
            {
                CPathfindInformation* pGridInfo = m_pActivePathfindInfo;
                pPathInfo->SetPathNodeEndPath(pGridInfo->m_nRawWayPointFloats, pGridInfo->m_paRawWayPoints);
                pGridInfo->m_nRawWayPointFloats = 0;
                pGridInfo->m_paRawWayPoints = 0;
            }
        }
        pPathInfo->m_bIgnoreCrePath = 0;
        if (nGrid == CSWSObject::ACTION_COMPLETE)
        {
            unsigned long long nGridTimer = g_pExoBase->m_pTimers->GetHighResolutionTimer();
            pPathInfo->m_nSmoothStep = CPathfindInformation::SMOOTHING_START;
            pPathInfo->m_nPathState = CPathfindInformation::PATH_POINT_SMOOTH;
            if (nGridTimer - nTimer > nTimeOut)
                return CSWSObject::ACTION_IN_PROGRESS;
        }
        if (nGrid == CSWSObject::ACTION_FAILED)
        {
            if (pPathInfo->m_nPathPointCount < 2)
            {
                sGridFailMsg = const_cast<char*>("    failed to grid based pathfind from the ending path point ot the destiantion.");
                goto GridPointFail;
            }
            --pPathInfo->m_nPathPointCount;
            pPathInfo->m_nDestPathPoint = pPathInfo->m_pPathPointList[pPathInfo->m_nPathPointCount - 1];
            nEndPathPoint = pPathInfo->m_nDestPathPoint;
            pPathInfo->m_nPathState = CPathfindInformation::PATH_POINT_GRIDEND;
            return CSWSObject::ACTION_IN_PROGRESS;
        }
        else if (nGrid == CSWSObject::ACTION_IN_PROGRESS)
        {
            return CSWSObject::ACTION_IN_PROGRESS;
        }
        nState = pPathInfo->m_nPathState;
    }

    // In PATH_POINT_SMOOTH, concatenate both grid segments and the path points into one array,
    // then run one smoothing slice per frame until SmoothPointsOnPathRestartable reports done.
    if (nState == CPathfindInformation::PATH_POINT_SMOOTH)
    {
        if (pPathInfo->m_nSmoothStep == CPathfindInformation::SMOOTHING_START)
        {
            unsigned long long nSmoothTimer = g_pExoBase->m_pTimers->GetHighResolutionTimer();
            int nSegA = pPathInfo->m_nSegCountA - 2;
            int nPPCount = pPathInfo->m_nPathPointCount;
            int nSegB = pPathInfo->m_nSegCountB;
            int nTotal = nSegA + nPPCount * 2 + nSegB - 2;
            int* pIdx = pPathInfo->m_pPathPointList;
            float* pSegA = pPathInfo->m_pSegFloatsA;
            float* pSegB = pPathInfo->m_pSegFloatsB;
            float* pNew = new float[nTotal];
            *reinterpret_cast<unsigned int*>(pNew) = static_cast<unsigned char>(nTotal) * BYTE_REPEAT_X4;  // count stamped into all four bytes
            int nOff = 0;
            for (; nOff < nSegA; nOff++)
            {
                K2_ASSUME(nOff < nTotal);
                pNew[nOff] = pSegA[nOff];
            }
            for (int i = 0; i < nPPCount; i++, nOff += 2)
            {
                const Vector& m = m_paPathPoints[pIdx[i]].m_point;
                K2_ASSUME(nOff >= 0 && nOff + 1 < nTotal);
                pNew[nOff] = m.x;
                pNew[nOff + 1] = m.y;
            }
            for (int j = 2; j < nSegB; j++)
            {
                K2_ASSUME(nOff < nTotal);
                pNew[nOff++] = pSegB[j];
            }
            pPathInfo->m_nRawWayPointFloats = nTotal;
            pPathInfo->m_paRawWayPoints = pNew;
            pPathInfo->SetPathNodeStartPath(0, 0);
            pPathInfo->SetPathNodeEndPath(0, 0);
            pPathInfo->SetPathPointStepsArray(0);
            pPathInfo->m_nSmoothStep = CPathfindInformation::SMOOTHING_STEP_1;
            if (g_pExoBase->m_pTimers->GetHighResolutionTimer() - nSmoothTimer > static_cast<uint64_t>(SmoothTime))
                return 1;
        }
        if (SmoothPointsOnPathRestartable() == 0)
        {
            // The walk path is complete; the next walk starts from the top of the ladder.
            pPathInfo->m_nPathState = CPathfindInformation::PATH_POINT_STARTPOINTS;
            return CSWSObject::ACTION_COMPLETE;
        }
        return CSWSObject::ACTION_IN_PROGRESS;
    }

    return nAction;

GridPointFail:
    // Both "not enough path points to grid-solve" errors land here.
    pPathInfo->m_nPathState = CPathfindInformation::PATH_POINT_STARTPOINTS;
    pPathInfo->HandlePathfindError(0, const_cast<char*>(sGridFailMsg));
    return CSWSObject::ACTION_FAILED;
}

// Walks the creature over the grid for one leg. Seeds the grid transform frame from the goal
// and the normalized from->to direction, then runs the ring-growing GridDFSearch loop under
// the time budget. A partial search is parked for the next tick; a complete one is expanded to
// world points and smoothed.
int CSWSArea::PlotGridPath(CPathfindInformation* pPathInfo, unsigned long long nGoalDist)
{
    g_pAppManager->m_pServerExoApp->GetServerAIMaster()->m_bRanGridPath = 1;
    unsigned long long nStartTime = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    m_pActivePathfindInfo = pPathInfo;

    Vector vFrom(pPathInfo->m_vStartPoint.x, pPathInfo->m_vStartPoint.y, 0.0f);
    Vector vTo(pPathInfo->m_goal.m_vPoint.x, pPathInfo->m_goal.m_vPoint.y, 0.0f);
    float fRadius = (pPathInfo->m_fPersonalSpace < 0.1f ? 0.1f : pPathInfo->m_fPersonalSpace);
    Vector vDir = normalize(vFrom - vTo);
    // In the grid frame, T is the goal, A and B are the radius-scaled perpendicular and forward
    // steps, and C lifts the tiles off the grid plane.
    Vector vRight(fRadius * vDir.y, -fRadius * vDir.x, 0.0f);
    Vector vFwd(fRadius * vDir.x, fRadius * vDir.y, fRadius * vDir.z);
    const Vector vUp(0.0f, 0.0f, 1.0f);
    int nSteps = static_cast<int>(magnitude(vFrom - vTo) / fRadius + 0.5);

    pPathInfo->m_nReserved7 = 0;
    pPathInfo->m_nGridSteps = nSteps;
    pPathInfo->m_nReserved8 = 0;
    pPathInfo->m_vGridTransformT = vTo;
    pPathInfo->m_vGridTransformA = vRight;
    pPathInfo->m_vGridTransformB = vFwd;
    pPathInfo->m_vGridTransformC = vUp;
    pPathInfo->m_bReturnClosestResult = 1;

    int* pGridPath = new int[2 * nSteps + 76];
    pPathInfo->m_pGridPath = pGridPath;
    int* pBestGridPath = pPathInfo->m_pBestGridPath;
    if (pBestGridPath == 0)
    {
        pBestGridPath = new int[2 * nSteps + 76];
        pPathInfo->m_pBestGridPath = pBestGridPath;
        pPathInfo->m_nBestGridLen = 0;
    }

    int nGridStep;
    // Only a walk with the same endpoints as the parked search may reuse its trans table.
    Vector vDup = vFrom - pPathInfo->m_vGridFrom;
    if (vDup.x * vDup.x + vDup.y * vDup.y + vDup.z * vDup.z >= 0.0001f)
        goto restart;
    vDup = vTo - pPathInfo->m_vGridTo;
    if (vDup.x * vDup.x + vDup.y * vDup.y + vDup.z * vDup.z >= 0.0001f)
        goto restart;
    m_paGridDFSTransTable = static_cast<CSWSAreaGridTransEntry*>(pPathInfo->m_pGridSavedTable);
    nGridStep = pPathInfo->m_nGridStep;
    goto seedy;
restart:
    if (pPathInfo->m_pGridSavedTable)
        m_paGridDFSTransTable = static_cast<CSWSAreaGridTransEntry*>(pPathInfo->m_pGridSavedTable);
    GridDFSTransTableInitialize();
    nGridStep = nSteps;
seedy:
    pPathInfo->m_fGridSearchRadius = 10.0f;
    unsigned long long nTickTimer = g_pExoBase->m_pTimers->GetHighResolutionTimer();
    unsigned long long nElapsed = nTickTimer - nStartTime;

    int nAction = CSWSObject::ACTION_IN_PROGRESS;
    if (pPathInfo->m_bReturnClosestResult == 1 && pPathInfo->m_bPointsReversed == 1)
    {
        pPathInfo->HandlePathfindError(2,
            const_cast<char*>("we can not return a closest result if points are reversed."));
        nAction = CSWSObject::ACTION_FAILED;
    }

    nGridStep += 2;
    m_bGridDFSAborted = 0;
    m_nGridDFSExpansions = 0;
    int nSavedDist = pPathInfo->m_nBestGridDist;
    if (pPathInfo->m_nBestGridDist > nSteps + 1)
    {
        pPathInfo->m_nBestGridDist = nSteps + 1;
        nSavedDist = nSteps + 1;
    }

    const int nStepMax = nSteps + 36;
    int nItr = 0;
    while (nGridStep < nStepMax && nAction == CSWSObject::ACTION_IN_PROGRESS &&
           nElapsed < nGoalDist)
    {
        nGridStep += 2;
        m_bGridDFSAborted = 0;
        m_nGridDFSExpansions = 0;
        int nResult = GridDFSearch(0, 0, nGridStep, 0, nSteps, 0);
        if (nResult == 1)
            nAction = CSWSObject::ACTION_COMPLETE;
        nElapsed = g_pExoBase->m_pTimers->GetHighResolutionTimer() - nStartTime;

        if (pPathInfo->m_bReturnClosestResult == 1 && pPathInfo->m_nBestGridLen >= 2 &&
            nAction != CSWSObject::ACTION_COMPLETE && nItr == 0 &&
            (nElapsed >= nGoalDist * 4 ||
             (nElapsed >= nGoalDist && nSavedDist == pPathInfo->m_nBestGridDist)))
        {
            // On the first search of the walk only, commit the best path found so far as complete
            // (unless the expansion watchdog tripped).
            pPathInfo->m_nGridPathLen = pPathInfo->m_nBestGridLen;
            memcpy(pPathInfo->m_pGridPath, pPathInfo->m_pBestGridPath,
                   static_cast<uint64_t>(pPathInfo->m_nBestGridLen) * 8);
            if (m_bGridDFSAborted != 1)
                nAction = CSWSObject::ACTION_COMPLETE;
        }
        ++nItr;
    }

    pPathInfo->m_fGridSearchRadius = 700.0f;
    int nReturn = nAction;
    if (nReturn != CSWSObject::ACTION_COMPLETE)
    {
        if (nGridStep >= nStepMax)
        {
            pPathInfo->HandlePathfindError(2, const_cast<char*>("Exceeded limits:Max search deapth"));
            goto limitTail;
        }
        if (nItr == 1)
        {
            if (nElapsed >= static_cast<unsigned long long>(nGoalDist) * 40)
                goto limitTail;
            if (nElapsed >= static_cast<unsigned long long>(nGoalDist) * 10)
            {
                if (nSavedDist == pPathInfo->m_nBestGridDist)
                    goto limitTail;
                goto parkResolve;
            }
            goto parkResolve;
        }
        goto parkResolve;
limitTail:
        // The three exceeded-limit messages share this tail, so a max-depth failure still
        // reaches the time and distance gates.
        if (nItr == 1)
        {
            if (nElapsed >= static_cast<unsigned long long>(nGoalDist) * 40)
                pPathInfo->HandlePathfindError(2, const_cast<char*>("Exceeded limits:Time limit"));
            if (nElapsed >= static_cast<unsigned long long>(nGoalDist) * 10 &&
                nSavedDist == pPathInfo->m_nBestGridDist)
                pPathInfo->HandlePathfindError(2, const_cast<char*>("Exceeded limits:Distance to goal"));
        }
        nReturn = CSWSObject::ACTION_FAILED;
    }
parkResolve:
    // Park the growing search for a later resume, or tear it down when the leg ended.
    if (nReturn == CSWSObject::ACTION_IN_PROGRESS)
    {
        pPathInfo->m_vGridFrom = vFrom;
        pPathInfo->m_vGridTo = vTo;
        pPathInfo->m_nGridStep = nGridStep;
        pPathInfo->m_pGridSavedTable = m_paGridDFSTransTable;
        m_paGridDFSTransTable = 0;
    }
    else
    {
        if (m_paGridDFSTransTable)
            delete[] m_paGridDFSTransTable;
        m_paGridDFSTransTable = 0;
        pPathInfo->m_nGridStep = 0;
        pPathInfo->m_vGridTo = Vector(0.0f, 0.0f, 0.0f);
        pPathInfo->m_vGridFrom = Vector(0.0f, 0.0f, 0.0f);
        pPathInfo->m_pGridSavedTable = 0;
    }

    if (nReturn == CSWSObject::ACTION_COMPLETE)
    {
        // Expand the found tile path to world points, replacing tile 0 with the exact start.
        int nLen = pPathInfo->m_nGridPathLen;
        float* pNew = new float[2 * nLen];
        const int* pSrc = pPathInfo->m_pGridPath;
        pNew[0] = vFrom.x;
        pNew[1] = vFrom.y;
        for (int i = 1; i < nLen; ++i)
        {
            float fGx = static_cast<float>(pSrc[i * 2]);
            float fGy = static_cast<float>(pSrc[i * 2 + 1]);
            Vector vWorld = vTo + vRight * fGx + vFwd * fGy + vUp * 0.0f;
            pNew[i * 2] = vWorld.x;
            pNew[i * 2 + 1] = vWorld.y;
        }
        pPathInfo->m_nRawWayPointFloats = 2 * nLen;
        pPathInfo->m_paRawWayPoints = pNew;
    }

    if (nReturn != CSWSObject::ACTION_IN_PROGRESS)
    {
        if (pBestGridPath)
            delete[] pBestGridPath;
        pPathInfo->m_pBestGridPath = 0;
    }
    operator delete[](pGridPath);
    pPathInfo->m_pGridPath = 0;
    if (nReturn == CSWSObject::ACTION_COMPLETE)
        SmoothPointsOnPath();
    return nReturn;
}

// Probes the from->to segment for blocking creatures. Returns 1 when no creature's body circle
// overlaps the line, 0 on the first blocker, whose id is written to *pnHitId and described in
// *pHit. Equal endpoints return 1 immediately.
int CSWSArea::NoCreaturesOnLine(const Vector& from, const Vector& to,
                                CPathfindInformation* pPathInfo, int a, int b,
                                unsigned long long* pnHitId, int c, CWalkHitInfo* pHit)
{
    if (equal(to, from) != 0)
        return 1;
    if (pPathInfo == 0)
        return 0;

    unsigned long long nSelfId = pPathInfo->m_oidCreature;
    float fCreSpace = pPathInfo->m_fCrePersonalSpace;
    CSWSCreature* pSelf =
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nSelfId);
    unsigned long long nIgnoreId2 = pPathInfo->m_oidGoalObject;
    unsigned long long nIgnoreId3 = pPathInfo->m_oidIgnoredBlocker;

    float fMaxX = to.x > from.x ? to.x : from.x;
    float fMinX = to.x < from.x ? to.x : from.x;
    float fMaxY = to.y > from.y ? to.y : from.y;
    float fMinY = to.y < from.y ? to.y : from.y;
    float fDx = to.x - from.x;
    float fSlope, fIntercept, fNormalize K2_UNSET;
    if (fDx == 0.0f)
    {
        fIntercept = 0.0f;
        fSlope = 0.0f;
    }
    else
    {
        fSlope = (to.y - from.y) / fDx;
        fIntercept = from.y - from.x * fSlope;
        fNormalize = __builtin_sqrtf(1.0f / (fSlope * fSlope + 1.0f));
    }

    int nIndex;
    int nLast = m_aObjectsByX.m_count - 1;
    if (GetFirstObjectIndiceByX(&nIndex, fMaxX + 6.0f) != 0)
        nLast = nIndex;
    int nFirst = 0;
    if (GetFirstObjectIndiceByX(&nIndex, fMinX - 6.0f) != 0)
        nFirst = nIndex;

    if (nFirst <= nLast)
    {
#if KOTOR1
    // K1 has no y-band prefilter; these components only feed the discarded magnitude() below.
    float fSegX = from.x - to.x;
    float fSegY = from.y - to.y;
#else
    float fMaxYBound = fMaxY + 6.0f;
    float fMinYBound = fMinY - 6.0f;
#endif
    fCreSpace = fCreSpace + 0.01f;
    int bSkipBump = (pSelf == 0 || b == 0);

    for (int i = nFirst; i <= nLast; i++)
    {
        unsigned long long oid = m_aObjectsByX.m_pArray[i];
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oid);
        if (oid == nSelfId || oid == nIgnoreId2 || oid == nIgnoreId3)
            continue;
        if (pCreature == 0)
            continue;
        if (pCreature->GetDead() != 0)
            continue;
#if KOTOR1
        // K1 rejects the dead-temp creatures instead of party members below 1 HP.
        if (pCreature->GetDeadTemp() != 0)
            continue;
        if (!bSkipBump && pSelf->GetIsCreatureBumpable(pCreature) != 0)
            continue;

        float fCombined = fCreSpace + pCreature->m_pPathfindInfo->m_fCrePersonalSpace;
        Vector vPos = pCreature->m_position;
        vPos.z = 0.0f;
        Vector vSegment = { fSegX, fSegY, 0.0f };
        magnitude(vSegment);
        float fCombinedSq = fCombined * fCombined;
#else
        // K2 skips party members below 1 HP, and both sides' ignore-path flags.
        if (pCreature->GetDeadTemp() != 0)
            continue;
        if (!bSkipBump && pSelf->GetIsCreatureBumpable(pCreature) != 0)
            continue;
        if (pCreature->m_bIgnoreCrePath != 0)
            continue;
        if (pPathInfo->m_bIgnoreCrePath != 0)
            continue;

        float fCombined = fCreSpace + pCreature->m_pPathfindInfo->m_fCrePersonalSpace;
        float fCombinedSq = fCombined * fCombined;
        Vector vPos = pCreature->m_position;
        vPos.z = 0.0f;
#endif
#if !KOTOR1
        // K2 bounds the y window before the circle tests.
        if (vPos.y > fMaxYBound)
            continue;
        if (vPos.y < fMinYBound)
            continue;
#endif

        // A vertical line is tested against the endpoints and the x band, and does not fill
        // in *pHit.
        if (fDx == 0.0f)
        {
            if (vPos.y <= fMinY)
            {
                float dX = fMinX - vPos.x;
                float dY = fMinY - vPos.y;
                if (!(dX * dX + dY * dY + 0.0f <= fCombinedSq))
                    continue;
            }
            else if (vPos.y >= fMaxY)
            {
                float dX = fMaxX - vPos.x;
                float dY = fMaxY - vPos.y;
                if (!(dX * dX + dY * dY + 0.0f <= fCombinedSq))
                    continue;
            }
            else
            {
                if (!(vPos.x >= fMinX - fCombined))
                    continue;
                if (!(vPos.x <= fMaxX + fCombined))
                    continue;
            }
            if (EvaluateOverlappingTargets(pPathInfo, vPos, oid, from, fCombined, a, c,
                                           pnHitId) != 0)
                continue;
            return 0;
        }

        float fLineY = fIntercept + fSlope * vPos.x;
        float fDist = __builtin_fabsf(vPos.y - fLineY) * fNormalize;
        if (!(fDist < fCombined))
            continue;
        float fSign = (vPos.y < fLineY) ? -1.0f : 1.0f;
        float fMid = vPos.x + fSign * fDist * fSlope * fNormalize;
        float fHalf = fNormalize * __builtin_sqrtf(fCombinedSq - fDist * fDist);
        float fX1 = fMid - fHalf;
        float fX2 = fMid + fHalf;
        if (!(fX1 >= fMinX || fX2 >= fMinX))
            continue;
        if (!(fX1 <= fMaxX || fX2 <= fMaxX))
            continue;
        if (EvaluateOverlappingTargets(pPathInfo, vPos, oid, from, fCombined, a, c, pnHitId) != 0)
            continue;

        SetHitInfoForCreature(pCreature->m_position, fCombined, from, to, pHit);
        if (pnHitId != 0)
            *pnHitId = oid;
        if (pHit != 0)
            pHit->m_hitObjectId = oid;
        return 0;
    }
    }

    if (pnHitId != 0)
        *pnHitId = OBJECT_INVALID;  // OBJECT_INVALID
    return 1;
}

// Safe point inside fRadius along `dir`: probe pos + dir itself, then sweep a band of
// personal-space steps perpendicular to the direction. Falls back to ComputeSafeLocation when
// the sweep finds nothing or the radius is already inside the personal space.
int CSWSArea::ComputeSafeLocationInDirection(const Vector& pos, const Vector& dir, float fRadius,
                                             CPathfindInformation* pPathInfo, int flag, Vector* pOut)
{
    Vector vTemp = { 0.0f, 0.0f, 0.0f };
    m_pActivePathfindInfo = pPathInfo;
    if (pPathInfo == 0)
        return 0;

    float fPersonalSpace = pPathInfo->m_fPersonalSpace;
    vTemp = pos + dir;

    vTemp.z = ComputeHeight(vTemp, 0, 0, 0);
    if (TestSafeLocationPoint(vTemp, pPathInfo) == 1)
    {
        *pOut = vTemp;
        return 1;
    }

    unsigned long long r;
    int bKeep = 0;
    if (PositionWalkable(vTemp) != 0 &&
        NoNonWalkPolysInDoors(vTemp + Vector(-0.01f, -0.01f, 0.0f),
                              vTemp + Vector(0.01f, 0.01f, 0.0f), vTemp.z, vTemp.z + pPathInfo->m_fHeight,
                              pPathInfo->m_fPersonalSpace, OBJECT_INVALID, r, 0, 1, 0) != 0)
        bKeep = 1;
    if (bKeep == 0)
        flag = 0;

    Vector vDir = normalize(Vector(dir.x, dir.y, 0.0f));
    Vector perp = { vDir.y, -vDir.x, 0.0f };
    if (fPersonalSpace < fRadius)
    {
        float fDist = fPersonalSpace;
        do
        {
            int nI = static_cast<int>((fDist + 0.01) / fPersonalSpace);
            int nJ = -nI;
            if (nJ <= nI)
            {
                for (;;)
                {
                    vTemp = pos + vDir * static_cast<float>(nI) + perp * static_cast<float>(nJ);
                    vTemp.z = ComputeHeight(vTemp, 0, 0, 0);
                    if (TestSafeLocationPoint(vTemp, pPathInfo) == 1)
                    {
                        int bAccept = 1;
                        if (flag != 0)
                        {
                            float fLow = vTemp.z < pos.z ? vTemp.z : pos.z;
                            bAccept = NoNonWalkPolysInDoors(pos, vTemp, fLow - 0.1f,
                                                            fLow + pPathInfo->m_fHeight,
                                                            fPersonalSpace, OBJECT_INVALID, r, 0, 1,
                                                            0) == 1;
                        }
                        if (bAccept)
                        {
                            *pOut = vTemp;
                            return 1;
                        }
                    }
                    if (nJ >= nI)
                        break;
                    nJ++;
                }
            }
            fDist += fPersonalSpace;
        } while (fDist < fRadius);
    }
    return ComputeSafeLocation(pos, fRadius, pPathInfo, flag, pOut);
}

// Safe point on the walkmesh within fRadius of pos. Each ring is scanned as a perimeter:
// phase 1 the bottom and top rows, phase 2 the left and right columns; the radius grows by
// the personal space between rings. Returns 0 when no ring yields a safe point.
int CSWSArea::ComputeSafeLocation(const Vector& pos, float fRadius, CPathfindInformation* pPathInfo,
                                  int flag, Vector* pOut)
{
    *pOut = pos;
#if KOTOR1  // K1 treats the origin as unsafe
    if (pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f)
        return 0;
#endif
    Vector vTemp = { 0.0f, 0.0f, 0.0f };
    m_pActivePathfindInfo = pPathInfo;
    if (pPathInfo == 0)
        return 0;

    float fPersonalSpace = pPathInfo->m_fPersonalSpace;
    unsigned long long r;
    vTemp = pos;

    vTemp.z = ComputeHeight(vTemp, 1, 0, 0);
    if (TestSafeLocationPoint(vTemp, pPathInfo) == 1)
    {
        *pOut = vTemp;
        return 1;
    }

    if (PositionWalkable(vTemp) != 0)
    {
        Vector vA = vTemp + Vector(-0.01f, -0.01f, 0.0f);
        Vector vB = vTemp + Vector(0.01f, 0.01f, 0.0f);
        if (NoNonWalkPolysInDoors(vA, vB, vTemp.z, vTemp.z + pPathInfo->m_fHeight, pPathInfo->m_fPersonalSpace,
                                  OBJECT_INVALID, r, 0, 1, 0) == 0)
            flag = 0;
    }
    else
    {
        flag = 0;
    }

    if (fRadius > 0.0f)
    {
        if (!(fRadius > 1.0f))
            return 0;
        float fR = 1.0f;
        do
        {
            float fLeft = pos.x - fR;
            float fRight = pos.x + fR;
            float fBottom = pos.y - fR;
            float fTop = pos.y + fR;
            float fSpan = fTop - fBottom;
            for (float fX = fLeft; fX <= fRight; fX += fPersonalSpace)
            {
                for (float fY = fBottom; fY <= fTop; fY += fSpan)
                {
                    vTemp.x = fX;
                    vTemp.y = fY;
                    vTemp.z = ComputeHeight(vTemp, 0, 0, 0);
                    if (TestSafeLocationPoint(vTemp, pPathInfo) == 1)
                    {
                        float fHigh = vTemp.z < pos.z ? pos.z : vTemp.z;
                        float fLow = vTemp.z < pos.z ? vTemp.z : pos.z;
                        if (flag != 0)
                        {
                            float fH = pPathInfo->m_fHeight;
                            int nLine = TestDirectLine(vTemp, pos, static_cast<float>(pPathInfo->m_fPersonalSpace), fH, 0, 0);
                            if ((nLine == 1 || nLine == -3) &&
                                NoNonWalkPolysInDoors(vTemp, pos, fLow - 0.1f, fHigh + fH,
                                                      fPersonalSpace, OBJECT_INVALID, r, 0, 1, 0) != 0)
                            {
                                *pOut = vTemp;
                                return 1;
                            }
                        }
                        else
                        {
                            *pOut = vTemp;
                            return 1;
                        }
                    }
                }
            }
            float fWidth = fRight - fLeft;
            for (float fX = fLeft; fX <= fRight; fX += fWidth)
            {
                for (float fY = fBottom + fPersonalSpace; fY <= fTop - fPersonalSpace;
                     fY += fPersonalSpace)
                {
                    vTemp.x = fX;
                    vTemp.y = fY;
                    vTemp.z = ComputeHeight(vTemp, 0, 0, 0);
                    if (TestSafeLocationPoint(vTemp, pPathInfo) == 1)
                    {
                        float fHigh = vTemp.z < pos.z ? pos.z : vTemp.z;
                        float fLow = vTemp.z < pos.z ? vTemp.z : pos.z;
                        if (flag != 0)
                        {
                            float fH = pPathInfo->m_fHeight;
                            if (TestDirectLine(vTemp, pos, static_cast<float>(pPathInfo->m_fPersonalSpace), fH, 0, 0) != 0 &&
                                NoNonWalkPolysInDoors(vTemp, pos, fLow - 0.1f, fHigh + fH,
                                                      fPersonalSpace, OBJECT_INVALID, r, 0, 1, 0) != 0)
                            {
                                *pOut = vTemp;
                                return 1;
                            }
                        }
                        else
                        {
                            *pOut = vTemp;
                            return 1;
                        }
                    }
                }
            }
            fR += fPersonalSpace;
        } while (fR < fRadius);
        return 0;
    }
#if !KOTOR1  // K2 also handles a non-positive radius
    else
    {
        float fLimit = fPersonalSpace > -fRadius ? fPersonalSpace : -fRadius;
        if (fPersonalSpace <= fLimit)
        {
            float fR = fPersonalSpace;
            do
            {
                float fLeft = pos.x - fR;
                float fRight = pos.x + fR;
                float fBottom = pos.y - fR;
                float fTop = pos.y + fR;
                float fSpan = fTop - fBottom;
                for (float fX = fLeft; fX <= fRight; fX += fPersonalSpace)
                {
                    for (float fY = fBottom; fY <= fTop; fY += fSpan)
                    {
                        vTemp.x = fX;
                        vTemp.y = fY;
                        vTemp.z = ComputeHeight(vTemp, 0, 0, 0);
                        if (TestSafeLocationPoint(vTemp, pPathInfo) == 1)
                        {
                            float fHigh = vTemp.z < pos.z ? pos.z : vTemp.z;
                            float fLow = vTemp.z < pos.z ? vTemp.z : pos.z;
                            if (flag != 0)
                            {
                                float fH = pPathInfo->m_fHeight;
                                int nLine = TestDirectLine(vTemp, pos, static_cast<float>(pPathInfo->m_fPersonalSpace), fH, 0, 0);
                                if ((nLine == 1 || nLine == -3) &&
                                    NoNonWalkPolysInDoors(vTemp, pos, fLow - 0.1f, fHigh + fH,
                                                          fPersonalSpace, OBJECT_INVALID, r, 0, 1, 0) != 0)
                                {
                                    *pOut = vTemp;
                                    return 1;
                                }
                            }
                            else
                            {
                                *pOut = vTemp;
                                return 1;
                            }
                        }
                    }
                }
                float fWidth = fRight - fLeft;
                for (float fX = fLeft; fX <= fRight; fX += fWidth)
                {
                    for (float fY = fBottom + fPersonalSpace; fY <= fTop - fPersonalSpace;
                         fY += fPersonalSpace)
                    {
                        vTemp.x = fX;
                        vTemp.y = fY;
                        vTemp.z = ComputeHeight(vTemp, 0, 0, 0);
                        if (TestSafeLocationPoint(vTemp, pPathInfo) == 1)
                        {
                            float fHigh = vTemp.z < pos.z ? pos.z : vTemp.z;
                            float fLow = vTemp.z < pos.z ? vTemp.z : pos.z;
                            if (flag != 0)
                            {
                                float fH = pPathInfo->m_fHeight;
                                if (TestDirectLine(vTemp, pos, static_cast<float>(pPathInfo->m_fPersonalSpace), fH, 0, 0) != 0 &&
                                    NoNonWalkPolysInDoors(vTemp, pos, fLow - 0.1f, fHigh + fH,
                                                          fPersonalSpace, OBJECT_INVALID, r, 0, 1, 0) != 0)
                                {
                                    *pOut = vTemp;
                                    return 1;
                                }
                            }
                            else
                            {
                                *pOut = vTemp;
                                return 1;
                            }
                        }
                    }
                }
                fR += fPersonalSpace;
            } while (fR <= fLimit);
            return 0;
        }
    }
#endif
    return 0;
}

// A companion of ComputeSafeLocation that finds a walkable, line-of-sight-clear point that the
// fewest players can see, within 30 units of pos and inside a hardcoded 20-unit ring limit.
// Returns 1 only for a point that no player can see. `flag` and `radius` are unused.
int CSWSArea::ComputeNonVisibleLocation(const Vector& pos, CPathfindInformation* pPathInfo,
                                        int flag, Vector* out, float radius)
{
    K2_UNUSED(flag); K2_UNUSED(radius);
    Vector vTemp = { 0.0f, 0.0f, 0.0f };
    if (pPathInfo == 0)
        return 0;
    *out = Vector(-1.0f, -1.0f, -1.0f);
    m_pActivePathfindInfo = pPathInfo;
    float fPersonalSpace = pPathInfo->m_fPersonalSpace;
    vTemp = pos;

    int nBest;
    vTemp.z = ComputeHeight(vTemp, 0, 0, 0);
    if (TestSafeLocationPoint(vTemp, pPathInfo) == 1)
    {
        int nCount = CountVisibleToPlayers(pos, VISIBLE_COUNT_CAP);
        if (nCount < VISIBLE_COUNT_CAP)
        {
            nBest = nCount;
            *out = vTemp;
            if (nCount == 0)
                return 1;
        }
        else
        {
            nBest = VISIBLE_COUNT_CAP;
        }
    }
    else
    {
        nBest = VISIBLE_COUNT_CAP;
    }

    Vector vOut = { 0.0f, 0.0f, 0.0f };
    unsigned long long nHit;
    float fR = 1.0f;
    do
    {
        float fLeft = pos.x - fR;
        float fRight = pos.x + fR;
        float fBottom = pos.y - fR;
        float fTop = pos.y + fR;
        float fSpan = fTop - fBottom;
        for (float fX = fLeft; fX <= fRight; fX += fPersonalSpace)
        {
            for (float fY = fBottom; fY <= fTop; fY += fSpan)
            {
                vTemp.x = fX;
                vTemp.y = fY;
                vTemp.z = ComputeHeight(vTemp, 0, 0, 0);
                if (TestSafeLocationPoint(vTemp, pPathInfo) == 1)
                {
                    Vector vDelta = { pos.x - vTemp.x, pos.y - vTemp.y, pos.z - vTemp.z };
                    if (magnitude(vDelta) <= 30.0f &&
                        ClearLineOfSight(pos, vTemp, &vOut, &nHit, OBJECT_INVALID, OBJECT_INVALID, 0, 0) == 1)
                    {
                        int nCount = CountVisibleToPlayers(pos, nBest);
                        if (nBest > nCount)
                        {
                            nBest = nCount;
                            *out = vTemp;
                        }
                        if (nCount == 0)
                            return 1;
                    }
                }
            }
        }
        float fWidth = fRight - fLeft;
        for (float fX = fLeft; fX <= fRight; fX += fWidth)
        {
            for (float fY = fBottom + fPersonalSpace; fY <= fTop - fPersonalSpace;
                 fY += fPersonalSpace)
            {
                vTemp.x = fX;
                vTemp.y = fY;
                vTemp.z = ComputeHeight(vTemp, 0, 0, 0);
                if (TestSafeLocationPoint(vTemp, pPathInfo) == 1)
                {
                    Vector vDelta = { pos.x - vTemp.x, pos.y - vTemp.y, pos.z - vTemp.z };
                    if (magnitude(vDelta) <= 30.0f &&
                        ClearLineOfSight(pos, vTemp, &vOut, &nHit, OBJECT_INVALID, OBJECT_INVALID, 0, 0) == 1)
                    {
                        int nCount = CountVisibleToPlayers(pos, nBest);
                        if (nBest > nCount)
                        {
                            nBest = nCount;
                            *out = vTemp;
                        }
                        if (nCount == 0)
                            return 1;
                    }
                }
            }
        }
        fR += fPersonalSpace;
    } while (fR < 20.0f);
    return 0;
}

// Counts the objects in this area within 30 units of pos that a player has line of sight to,
// stopping at nMax.
int CSWSArea::CountVisibleToPlayers(const Vector& pos, int nMax)
{
    unsigned long long nObjectId;
    int nCount;
    if (GetFirstObjectInArea(nObjectId) == 1)
    {
        Vector vPos = pos;
        nCount = 0;
        if (nMax >= 1)
        {
            do
            {
                CGameObjectArray* pObjectArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
                CGameObject* pObject;
                if (pObjectArray->GetGameObject(nObjectId, &pObject) == CGameObjectArray::SUCCESS)
                {
                    CSWSCreature* pCreature = pObject->AsSWSCreature();
                    if (pCreature != 0 &&
                        g_pAppManager->m_pServerExoApp->GetClientObjectByObjectId(
                            m_objectId) != 0)
                    {
                        Vector vOther = pObject->AsSWSCreature()->m_position;
                        float fDx = vPos.x - vOther.x;
                        float fDy = vPos.y - vOther.y;
                        float fDz = vPos.z - vOther.z;
                        if (fDx * fDx + fDy * fDy + fDz * fDz <= 900.0f)
                        {
                            // Eye height; vPos accumulates the raise over the objects counted
                            // so far.
                            vPos.z += 1.5f;
                            vOther.z += 1.5f;
                            unsigned long long nHit;
                            Vector vOut = { 0.0f, 0.0f, 0.0f };
                            if (ClearLineOfSight(vPos, vOther, &vOut, &nHit,
                                                 OBJECT_INVALID, OBJECT_INVALID, 0, 0) == 1)
                                nCount++;
                        }
                    }
                }
                } while (GetNextObjectInArea(nObjectId) == 1 && nCount < nMax);
        }
    }
    else
    {
        nCount = 0;
    }
    return nCount;
}

// Collects into pOut the id of every area object whose region the from->to segment crosses:
// triggers, then areas-of-effect, then encounter activate areas, in that order of precedence.
// An id that no longer resolves is skipped silently. Returns nonzero when anything was
// appended.
int CSWSArea::IntersectingLineSegment(const Vector& from, const Vector& to,
                                      CExoArrayList<unsigned long long>* pOut)
{
    int nFound = 0;
    if (m_subAreas.m_count < 1)
        return 0;

    for (int i = 0; i < m_subAreas.m_count; i++)
    {
        CGameObject* pObj =
            g_pAppManager->m_pServerExoApp->GetGameObject(m_subAreas.m_pArray[i]);
        if (pObj == 0)
            continue;

        if (pOut->Contains(m_subAreas.m_pArray[i]) != 0)
            continue;

        if (pObj->AsSWSTrigger() != 0)
        {
            if (pObj->AsSWSTrigger()->LineSegmentIntersectTrigger(from, to) != 0)
            {
                unsigned long long id = m_subAreas.m_pArray[i];
                pOut->Add(id);
                nFound = 1;
            }
        }
        else if (pObj->AsSWSAreaOfEffectObject() != 0)
        {
            if (pObj->AsSWSAreaOfEffectObject()->LineSegmentIntersectAreaOfEffect(from, to) != 0)
            {
                unsigned long long id = m_subAreas.m_pArray[i];
                pOut->Add(id);
                nFound = 1;
            }
        }
        else if (pObj->AsSWSEncounter() != 0)
        {
            if (pObj->AsSWSEncounter()->LineSegmentIntersectActivateArea(from, to) != 0)
            {
                unsigned long long id = m_subAreas.m_pArray[i];
                pOut->Add(id);
                nFound = 1;
            }
        }
    }
    return nFound;
}

// Collects into pOut the id of every subarea (trigger, area-of-effect, encounter or door)
// whose region contains pos, and prunes ids that no longer resolve to an object. Returns
// nonzero when anything was appended.
int CSWSArea::InSubAreas(const Vector& pos, CExoArrayList<unsigned long long>* pOut)
{
    int nFound = 0;
    if (m_subAreas.m_count < 1)
        return 0;

    unsigned long long nStrayId = OBJECT_INVALID;
    for (int i = 0; i < m_subAreas.m_count; i++)
    {
        if (pOut->Contains(m_subAreas.m_pArray[i]) != 0)
            continue;

        CGameObject* pObj =
            g_pAppManager->m_pServerExoApp->GetGameObject(m_subAreas.m_pArray[i]);
        if (pObj == 0)
        {
            nStrayId = m_subAreas.m_pArray[i];
            continue;
        }

        if (pObj->AsSWSTrigger() != 0)
        {
            if (pObj->AsSWSTrigger()->InTrigger(pos) != 0)
            {
                unsigned long long id = m_subAreas.m_pArray[i];
                pOut->Add(id);
                nFound = 1;
            }
        }
        else if (pObj->AsSWSAreaOfEffectObject() != 0)
        {
            if (pObj->AsSWSAreaOfEffectObject()->InAreaOfEffect(pos) != 0)
            {
                unsigned long long id = m_subAreas.m_pArray[i];
                pOut->Add(id);
                nFound = 1;
            }
        }
        else if (pObj->AsSWSEncounter() != 0)
        {
            if (pObj->AsSWSEncounter()->GetInActivateArea(pos) != 0)
            {
                unsigned long long id = m_subAreas.m_pArray[i];
                pOut->Add(id);
                nFound = 1;
            }
        }

        if (pObj->AsSWSDoor() != 0)
        {
            if (pObj->AsSWSDoor()->InDoor(pos) != 0)
            {
                unsigned long long id = m_subAreas.m_pArray[i];
                pOut->Add(id);
                nFound = 1;
            }
        }
    }

    if (nStrayId != OBJECT_INVALID)
        m_subAreas.Remove(nStrayId);
    return nFound;
}

// Pulls the `to` end of the from->to segment back toward `from` until it clears every creature
// whose body circle overlaps the endpoint. Passes repeat until one reports no hit, since moving
// the endpoint can put it back inside an earlier creature. Returns nonzero when `out` moved.
int CSWSArea::ShrinkLineByCreatures(const Vector& from, const Vector& to, Vector& out)
{
    int nIndex;
    float fMinY = std::min(to.y, from.y) - 3.0f;
    float fMaxY = std::max(to.y, from.y) + 3.0f;
    int nFirst = 0;
    int nLast = m_aObjectsByX.m_count - 1;
    if (GetFirstObjectIndiceByX(&nIndex, std::max(to.x, from.x) + 3.0f) != 0)
        nLast = nIndex;
    if (GetFirstObjectIndiceByX(&nIndex, std::min(to.x, from.x) - 3.0f) != 0)
        nFirst = nIndex;

    int nModified = 0;
    if (nFirst > nLast)
    {
        out = to;
        return 0;
    }

    CExoArrayList<CSWSCreature*> creatures;
    for (int i = nFirst; i <= nLast; i++)
    {
        CSWSCreature* pCreature =
            g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(m_aObjectsByX.m_pArray[i]);
        if (pCreature == 0)
            continue;
        if (!(pCreature->m_position.y >= fMinY))
            continue;
        if (!(pCreature->m_position.y <= fMaxY))
            continue;
        float fDx = pCreature->m_position.x - from.x;
        float fDy = pCreature->m_position.y - from.y;
        float fSpace = pCreature->m_pPathfindInfo->m_fPersonalSpace;
        if (!(fDx * fDx + fDy * fDy > fSpace * fSpace))
            continue;
        creatures.Add(pCreature);
    }

    out = to;
    if (creatures.m_count == 0)
        return 0;

    const float kShrink = 0.001f;
    const float kStep = -0.01f;
    for (;;)
    {
        bool bHit = false;
        for (int i = 0; i < creatures.m_count; i++)
        {
            CSWSCreature* pCreature = creatures.m_pArray[i];
            float fDx = pCreature->m_position.x - out.x;
            float fDy = pCreature->m_position.y - out.y;
            float fCam = pCreature->m_pPathfindInfo->m_fCameraSpace;
            if (!(fDx * fDx + fDy * fDy + 0.0f < fCam * fCam))
                continue;

            Vector vP1 = { 0.0f, 0.0f, 0.0f };
            Vector vP2 = { 0.0f, 0.0f, 0.0f };
            Vector vFrom(from.x, from.y, 0.0f);
            Vector vOut(out.x, out.y, 0.0f);
            Vector vDir = normalize(vOut - vFrom);
            Vector vPos(pCreature->m_position.x, pCreature->m_position.y, 0.0f);
            int nHit = IntersectCircle2d(vPos, fCam, vFrom, vOut, vP1, vP2, 1);
            float fKx = vDir.x * kShrink;
            float fKy = vDir.y * kShrink;
            float fKz = vDir.z * kShrink;
            if (nHit == 1)
            {
                if (out.x != vP1.x || out.y != vP1.y)
                {
                    out.x = vP1.x - fKx;
                    out.y = vP1.y - fKy;
                    out.z = vP1.z - fKz;
                    nModified = 1;
                    bHit = true;
                }
                if (bHit)
                {
                    float fLx = vFrom.x - vOut.x;
                    float fLy = vFrom.y - vOut.y;
                    float fLz = vFrom.z - vOut.z;
                    float fLen = fLx * fLx + fLy * fLy + fLz * fLz;
                    if (fLen != 0.0f)
                    {
                        float fNx = vFrom.x - out.x;
                        float fNy = vFrom.y - out.y;
                        float fNz = vFrom.z - out.z;
                        float fT = (fNx * fNx + fNy * fNy + fNz * fNz) / fLen;
                        float fLoZ = std::min(to.z, from.z);
                        float fHiZ = std::max(to.z, from.z);
                        out.z = fLoZ + (fT * (fHiZ - fLoZ) + kStep);
                    }
                }
            }
            else if (nHit == 2)
            {
                float fD1x = from.x - vP1.x;
                float fD1y = from.y - vP1.y;
                float fD1z = from.z - vP1.z;
                float fD2x = from.x - vP2.x;
                float fD2y = from.y - vP2.y;
                float fD2z = from.z - vP2.z;
                float fDist1 = fD1x * fD1x + fD1y * fD1y + fD1z * fD1z;
                float fDist2 = fD2x * fD2x + fD2y * fD2y + fD2z * fD2z;
                Vector vHit;
                if (fDist1 < fDist2)
                    vHit = vP1;
                else
                    vHit = vP2;
                out.x = vHit.x - fKx;
                out.y = vHit.y - fKy;
                out.z = vHit.z - fKz;
                nModified = 1;
                bHit = true;

                float fLx = vFrom.x - vOut.x;
                float fLy = vFrom.y - vOut.y;
                float fLz = vFrom.z - vOut.z;
                float fLen = fLx * fLx + fLy * fLy + fLz * fLz;
                if (fLen != 0.0f)
                {
                    float fNx = vFrom.x - out.x;
                    float fNy = vFrom.y - out.y;
                    float fNz = vFrom.z - out.z;
                    float fT = (fNx * fNx + fNy * fNy + fNz * fNz) / fLen;
                    float fLoZ = std::min(to.z, from.z);
                    float fHiZ = std::max(to.z, from.z);
                    out.z = fLoZ + (fT * (fHiZ - fLoZ) + kStep);
                }
            }
        }
        if (!bHit)
            break;
    }
    return nModified;
}

int CSWSArea::PlotPath(CPathfindInformation* pPathInfo, unsigned long long nTimeOut)
{
    K2_UNUSED(pPathInfo); K2_UNUSED(nTimeOut);
    return 0;
}

int CSWSArea::TestLineWalkable(float fX1, float fY1, float fZ1, float fX2, float fY2)
{
    K2_UNUSED(fX1); K2_UNUSED(fY1); K2_UNUSED(fZ1); K2_UNUSED(fX2); K2_UNUSED(fY2);
    return 1;
}

int CSWSArea::NoNoneWalkPolysInStaticObject(unsigned long long nObjectId, const Vector& a,
                                            const Vector& b, float f1, float f2, float f3,
                                            unsigned long long nIgnoreId, int nFlag,
                                            CWalkHitInfo* pHit)
{
    K2_UNUSED(nIgnoreId); K2_UNUSED(nFlag);
    CGameObject* pObj = g_pAppManager->m_pServerExoApp->GetGameObject(nObjectId);
    if (pObj != 0)
    {
        if (pObj->m_objectType == GAME_OBJECT_TYPE_PLACEABLE)
        {
            CSWSPlaceable* pPlaceable = pObj->AsSWSPlaceable();
            return pPlaceable->NoNonWalkPolys(a, b, f1, f2, f3, pHit);
        }
        if (pObj->m_objectType == GAME_OBJECT_TYPE_DOOR)
        {
            CSWSDoor* pDoor = pObj->AsSWSDoor();
            return pDoor->NoNonWalkPolysInDoor(a, b, f1, f2, f3, pHit);
        }
    }
    return 0;
}

// Fills *pHit from an IntersectCircle2d result: the hit point, its normal and the two tangent
// points a half-width either side of it. When the segment crosses the circle twice the nearer
// crossing wins.
int CSWSArea::SetHitInfoForCreature(const Vector& pos, float fCombined, const Vector& from,
                                    const Vector& to, CWalkHitInfo* pHit)
{
    if (pHit == 0)
        return 1;

    Vector vP1 = { 0.0f, 0.0f, 0.0f };
    Vector vP2 = { 0.0f, 0.0f, 0.0f };
    int nHit = IntersectCircle2d(pos, fCombined, from, to, vP1, vP2, true);
    const Vector* pPoint;
    Vector vN;
    if (nHit == 2)
    {
        float fD1x = vP1.x - from.x;
        float fD1y = vP1.y - from.y;
        float fD1z = vP1.z - from.z;
        float fD2x = vP2.x - from.x;
        float fD2y = vP2.y - from.y;
        float fD2z = vP2.z - from.z;
        float fDist1 = fD1x * fD1x + fD1y * fD1y + fD1z * fD1z;
        float fDist2 = fD2x * fD2x + fD2y * fD2y + fD2z * fD2z;
        if (fDist2 < fDist1)
        {
            vN = normalize(vP2 - pos);
            pPoint = &vP2;
        }
        else
        {
            vN = normalize(vP1 - pos);
            pPoint = &vP1;
        }
    }
    else if (nHit == 1)
    {
        vN = normalize(vP1 - pos);
        pPoint = &vP1;
    }
    else
    {
        return nHit;
    }
    Vector vPerp = { vN.y, -vN.x, -vN.z };
    pHit->m_hitType = 1;
    pHit->m_hitPosition = *pPoint;
    pHit->m_hitNormal = vN;
    pHit->m_hitPointA = *pPoint + vPerp;
    pHit->m_hitPointB = *pPoint - vPerp;
    pHit->m_triNormalLocal = vN;
    pHit->m_hitSubType = 1;
    return nHit;
}

// 1 when the two creatures' personal-space circles overlap along the segment (or when their
// combined creature spacing is larger than the distance between them), 0 when they clear each
// other.
int CSWSArea::EvaluateOverlappingTargets(CPathfindInformation* pPathInfo, const Vector& pos,
                                         unsigned long long nObjectId, const Vector& from,
                                         float fCombined, int a, int c,
                                         unsigned long long* pnHitId)
{
    if (RenderAi != 0)
    {
        // Debug overlay; every result here is discarded.
        CSWSCreature* pTarget = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nObjectId);
        CSWSCreature* pSelf = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
            pPathInfo->m_oidCreature);
        Vector vTargetPos = pTarget->m_position;
        Vector vSelfPos = pSelf->m_position;
        Vector vDelta = vTargetPos - vSelfPos;
        normalize(vDelta);
        vDelta = vSelfPos - vTargetPos;
        vDelta = normalize(vDelta);
        g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(nObjectId);
    }

    float fDx = pos.x - from.x;
    float fDy = pos.y - from.y;
    float fDz = pos.z - from.z;
    float fDist2 = fDx * fDx + fDy * fDy + fDz * fDz;
    float fCombinedSq = fCombined * fCombined;
    if (fDist2 <= fCombinedSq)
        return 0;
    if (a != 0 && !(fDist2 > fCombinedSq))
        return 1;
    if (pnHitId != 0)
        *pnHitId = nObjectId;
    if (pPathInfo != 0 && c == 1)
    {
        CSWSCreature* pTarget = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
            pPathInfo->m_oidGoalObject);
        CSWSCreature* pSelf = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(
            nObjectId);
        if (pTarget != 0 && pSelf != 0)
        {
            float fCdx = pTarget->m_position.x - pSelf->m_position.x;
            float fCdy = pTarget->m_position.y - pSelf->m_position.y;
            float fSpaces = pTarget->m_pPathfindInfo->m_fCrePersonalSpace +
                            pSelf->m_pPathfindInfo->m_fCrePersonalSpace;
            if (fCdx * fCdx + fCdy * fCdy + 0.0f < fSpaces * fSpaces)
                return 1;
        }
    }
    return 0;
}

#define COUNTTRANS_COLLECT(oid, pArea)                                                \
    {                                                                                 \
        if (nMax >= 1)                                                                \
        {                                                                             \
            int bFound = 0;                                                           \
            for (int j = 0; j < nMax; j++)                                            \
            {                                                                         \
                if (pExclude[j] == (oid))                                             \
                    bFound = 1;                                                       \
            }                                                                         \
            if (bFound)                                                               \
                continue;                                                             \
        }                                                                             \
        if (pOut != 0)                                                                \
        {                                                                             \
            pOut[nCount] = (oid);                                                     \
            (pArea)->m_bTransitionTargetMarked = 1;                                   \
        }                                                                             \
        nCount++;                                                                     \
    }
// Counts the triggers (and linked doors) that lead to an area not marked yet, writing their
// ids to pOut, up to nMax, and skipping the ids listed in pExclude. Each target area is marked
// so it is only counted once.
int CSWSArea::CountAreaTransitionTriggers(unsigned long long* pOut, int nMax,
                                          unsigned long long* pExclude)
{
    CGameObjectArray* pArray = g_pAppManager->m_pServerExoApp->GetObjectArray();
    int nObjects = m_aObjectsByX.m_count;
    int nCount = 0;
    if (nObjects >= 1)
    {
        unsigned long long i = 0;
        do
        {
            unsigned long long oid = m_aObjectsByX.m_pArray[i];
            CGameObject* pObj;
            if (pArray->GetGameObject(oid, &pObj) != CGameObjectArray::SUCCESS)
                continue;
            if (pObj->m_objectType == CGameObject::OBJECT_TYPE_TRIGGER)
            {
                CSWSTrigger* pTrigger = pObj->AsSWSTrigger();
                if (pTrigger->m_bAreaTransition != 1)
                    continue;
                CGameObject* pTarget;
                if (pArray->GetGameObject(pTrigger->GetTargetArea(), &pTarget) !=
                    CGameObjectArray::SUCCESS)
                    continue;
                CSWSArea* pArea = pTarget->AsSWSArea();
                if (pArea == 0)
                    continue;
                if (pArea->m_bTransitionTargetMarked != 0)
                    continue;
                COUNTTRANS_COLLECT(oid, pArea)
            }
            else if (pObj->m_objectType == GAME_OBJECT_TYPE_DOOR)
            {
                CSWSDoor* pDoor = pObj->AsSWSDoor();
                if (pDoor->GetIsLinked() == 0)
                    continue;
                CSWSObject* pLinked = pDoor->GetLinkedObject();
                if (pLinked == 0)
                    continue;
                CSWSArea* pArea = static_cast<CSWSArea*>(pLinked->GetArea());
                if (pArea == 0)
                    continue;
                if (pArea->m_bTransitionTargetMarked != 0)
                    continue;
                COUNTTRANS_COLLECT(oid, pArea)
            }
        }
        while (++i != static_cast<unsigned int>(nObjects));
    }
    return nCount;
}
#undef COUNTTRANS_COLLECT

// Adds one inter-tile exit record. Both exit arrays grow by one entry, the existing records are
// copied over and the new one is appended.
int CSWSArea::RemoveInterTileExit(int nExitA1, int nExitA2, int nExitB1, int nExitB2)
{
    CPathfindInformation* pInfo = m_pActivePathfindInfo;
    int nCross = pInfo->m_nInterTileCount;
    int nSlots = nCross * 2;
    int* pNewB = new int[nSlots + 2];
    int* pNewA = new int[nSlots + 2];
    int* pOldB = pInfo->m_paInterTileB;
    if (nCross > 0)
    {
        int* pOldA = pInfo->m_paInterTileA;
        for (int i = 0; i < nSlots; i++)
        {
            pNewB[i] = pOldB[i];
            pNewA[i] = pOldA[i];
        }
    }
    pInfo->m_nInterTileCount++;
    pNewB[nSlots] = nExitB1;
    pNewB[nSlots + 1] = nExitB2;
    pNewA[nSlots] = nExitA1;
    pNewA[nSlots + 1] = nExitA2;
    if (pOldB)
    {
        delete[] pOldB;
        m_pActivePathfindInfo->m_paInterTileB = 0;
    }
    if (m_pActivePathfindInfo->m_paInterTileA)
    {
        delete[] m_pActivePathfindInfo->m_paInterTileA;
        m_pActivePathfindInfo->m_paInterTileA = 0;
    }
    m_pActivePathfindInfo->m_paInterTileB = pNewB;
    m_pActivePathfindInfo->m_paInterTileA = pNewA;
    return 1;
}

// Replaces the area's walk path with a smoothed copy of its raw point list. A straight-line
// path (type 1) is deep-copied; anything else is re-plotted through SmoothSelection and
// SmoothCornerOptimize. Always reports success.
int CSWSArea::SmoothPointsOnPath()
{
    CPathfindInformation* pPathInfo = m_pActivePathfindInfo;
    if (pPathInfo->m_paWayPoints != 0)
    {
        delete[] pPathInfo->m_paWayPoints;
        pPathInfo = m_pActivePathfindInfo;
        pPathInfo->m_paWayPoints = 0;
        pPathInfo->m_nWayPointFloats = 0;
    }
    int nCount = pPathInfo->m_nRawWayPointFloats;
    if (nCount == 0)
    {
        pPathInfo->m_nWayPointFloats = 0;
        pPathInfo->m_paWayPoints = 0;
    }
    else
    {
        float* pSource = pPathInfo->m_paRawWayPoints;
        if (pPathInfo->m_nPathType == 1)
        {
            float* pCopy = new float[nCount];
            for (int n = 0; n < nCount; n++)
                pCopy[n] = pSource[n];
            pPathInfo->m_paWayPoints = pCopy;
            pPathInfo->m_nWayPointFloats = nCount;
        }
        else
        {
            int nSmooth;
            float* pSmooth;
            SmoothSelection(nCount, pSource, &nSmooth, &pSmooth);
            int nFinal;
            float* pFinal;
            SmoothCornerOptimize(nSmooth, pSmooth, &nFinal, &pFinal);
            pPathInfo = m_pActivePathfindInfo;
            pPathInfo->m_paWayPoints = pFinal;
            pPathInfo->m_nWayPointFloats = nFinal;
            delete[] pSmooth;
        }
    }
    return 1;
}

// Runs one budgeted slice of the smoothing pipeline per call. m_nSmoothStep picks the phase, and 1
// is returned until the walk path is complete (0).
int CSWSArea::SmoothPointsOnPathRestartable()
{
    CPathfindInformation* pPathInfo = m_pActivePathfindInfo;
    int nRawCount = pPathInfo->m_nRawWayPointFloats;

    if (nRawCount == 0)
    {
        pPathInfo->m_nWayPointFloats = 0;
        pPathInfo->m_paWayPoints = 0;
        return 0;
    }

    float* pRaw = pPathInfo->m_paRawWayPoints;

    if (pPathInfo->m_nSmoothStep == CPathfindInformation::SMOOTHING_STEP_1)
    {
        if (pPathInfo->m_paWayPoints)
        {
            delete[] pPathInfo->m_paWayPoints;
            pPathInfo = m_pActivePathfindInfo;
            pPathInfo->m_paWayPoints = 0;
            pPathInfo->m_nWayPointFloats = 0;
        }

        if (pPathInfo->m_nPathType == 1)
        {
            float* pCopy = new float[nRawCount];
            for (int i = 0; i < nRawCount; i++)
                pCopy[i] = pRaw[i];
            pPathInfo->m_paWayPoints = pCopy;
            pPathInfo->m_nWayPointFloats = nRawCount;
            return 0;
        }

        pPathInfo->m_nSmoothResumeIndex = 0;
        pPathInfo->m_nSmoothResumeSel = -1;
        pPathInfo->m_nSmoothStep = CPathfindInformation::SMOOTHING_STEP_2;

        pPathInfo->SetSmoothingFirstWayPoints(0);
        pPathInfo = m_pActivePathfindInfo;
        pPathInfo->SetSmoothingSecondWayPoints(0);
        pPathInfo = m_pActivePathfindInfo;
        pPathInfo->SetSmoothingThirdWayPoints(0);
        pPathInfo = m_pActivePathfindInfo;
    }

    int nCornerCount;
    int nSel = pPathInfo->m_nSmoothingThirdFloats;
    float* pSel = pPathInfo->m_paSmoothingThirdWayPoints;
    float* pCorner = 0;
    int nRet = SmoothSelectionRestartable(nRawCount, pRaw, &nSel, &pSel);
    pPathInfo = m_pActivePathfindInfo;
    pPathInfo->m_nSmoothingThirdFloats = nSel;
    pPathInfo->SetSmoothingThirdWayPoints(pSel);
    if (nRet == 1)
        return 1;

    SmoothCornerOptimize(nSel, pSel, &nCornerCount, &pCorner);
    pPathInfo = m_pActivePathfindInfo;
    pPathInfo->m_paWayPoints = pCorner;
    pPathInfo->m_nSmoothingThirdFloats = 0;
    pPathInfo->m_nWayPointFloats = nCornerCount;
    pPathInfo->SetSmoothingThirdWayPoints(0);
    return 0;
}

// Generates the neighbours of one grid tile for the DFS, the eight surrounding tiles that
// TestDirectLine reaches, plus, inside the dominant cost-budget windows, a straight shot at the
// goal or a fractional step toward it. The ring is insertion-sorted so the tile closest to the goal
// is tried first.
int CSWSArea::GridDFSGenerateSuccessors(int nCost, int nCostBound, int nX, int nY, int nZ,
                                        CSWSAreaGridSuccessors** ppOut)
{
    const float fRadius = m_pActivePathfindInfo->m_fHeightAlias;
    Vector vTo;
    const float fHeight = m_pActivePathfindInfo->m_fPersonalSpace;  // TestDirectLine's hit fraction, by reference
    Vector vFrom;
    *ppOut = new CSWSAreaGridSuccessors[9];
    if (nCost != 0)
    {
        vFrom = m_pActivePathfindInfo->m_vGridTransformT
              + m_pActivePathfindInfo->m_vGridTransformA * static_cast<float>(nX)
              + m_pActivePathfindInfo->m_vGridTransformB * static_cast<float>(nY)
              + m_pActivePathfindInfo->m_vGridTransformC * static_cast<float>(nZ);
    }
    else
    {
        vFrom.x = m_pActivePathfindInfo->m_vStartPoint.x;
        vFrom.y = m_pActivePathfindInfo->m_vStartPoint.y;
    }

    if (nCostBound == 4 || (nCostBound >= 5 && nCost % 6 == 4))
    {
        vTo = m_pActivePathfindInfo->m_vGridTransformT;
        int nFracX;
        int nFracY;
        if (m_pActivePathfindInfo->m_nStepTolerance <= 1)
        {
            if (TestDirectLine(vFrom, vTo, fHeight, fRadius, 0, 0) == 1)
            {
                (*ppOut)[0].m_nCostBound = (nX < 0 ? -nX : nX) + (nY < 0 ? -nY : nY);
                (*ppOut)[0].m_nX = 0;
                (*ppOut)[0].m_nY = 0;
                (*ppOut)[0].m_nZ = 0;
                return 1;
            }
            nFracX = 0;
            nFracY = 0;
        }
        else
        {
            int nSq = nX * nX + nY * nY + nZ * nZ;
            float fDist = __builtin_sqrtf(static_cast<float>(nSq));
            nFracX = static_cast<int>(static_cast<float>(nX) / fDist * static_cast<float>(m_pActivePathfindInfo->m_nStepTolerance) / 1.2f);
            nFracY = static_cast<int>(static_cast<float>(nY) / fDist * static_cast<float>(m_pActivePathfindInfo->m_nStepTolerance) / 1.2f);
            if (static_cast<float>(nSq) >= static_cast<float>(m_pActivePathfindInfo->m_nStepTolerance * m_pActivePathfindInfo->m_nStepTolerance))
            {
                vTo = m_pActivePathfindInfo->m_vGridTransformT
                    + m_pActivePathfindInfo->m_vGridTransformA * static_cast<float>(nFracX)
                    + m_pActivePathfindInfo->m_vGridTransformB * static_cast<float>(nFracY)
                    + m_pActivePathfindInfo->m_vGridTransformC * 0.0f;
                if (TestDirectLine(vFrom, vTo, fHeight, fRadius, 0, 0) == 1)
                {
                    (*ppOut)[0].m_nCostBound =
                        (nX - nFracX < 0 ? -(nX - nFracX) : nX - nFracX) +
                        (nY - nFracY < 0 ? -(nY - nFracY) : nY - nFracY);
                    (*ppOut)[0].m_nX = nFracX;
                    (*ppOut)[0].m_nY = nFracY;
                    (*ppOut)[0].m_nZ = 0;
                    return 1;
                }
            }
        }

        // Refine by halving, tightening the resolution until a fraction lands on a walkable point.
        float fStep = 0.25f;
        float fFactor = 0.5f;
        int nFound = 0;
        do
        {
            int nOffX = static_cast<int>(static_cast<float>(nFracX - nX) * fFactor);
            int nOffY = static_cast<int>(static_cast<float>(nFracY - nY) * fFactor);
            if (nOffX != 0 || nOffY != 0)
            {
                int nCandX = nX + nOffX;
                int nCandY = nY + nOffY;
                vTo = m_pActivePathfindInfo->m_vGridTransformT
                    + m_pActivePathfindInfo->m_vGridTransformA * static_cast<float>(nCandX)
                    + m_pActivePathfindInfo->m_vGridTransformB * static_cast<float>(nCandY);
                if (TestDirectLine(vFrom, vTo, fHeight, fRadius, 0, 0) == 1)
                {
                    (*ppOut)[0].m_nCostBound =
                        (nOffX < 0 ? -nOffX : nOffX) + (nOffY < 0 ? -nOffY : nOffY);
                    (*ppOut)[0].m_nX = nCandX;
                    (*ppOut)[0].m_nY = nCandY;
                    (*ppOut)[0].m_nZ = 0;
                    fFactor = fStep + fFactor;
                    nFound = 1;
                }
                else
                    fFactor = fFactor - fStep;
            }
            fStep = fStep * 0.5f;
        } while (fStep > 0.005f);
        if (nFound == 1)
            return 1;
    }

    // The eight surrounding tiles, insertion-sorted by distance to the goal.
    int nCount = 0;
    for (int dy = -1; dy <= 1; ++dy)
    {
        int nCandY = nY + dy;
        for (int dx = -1; dx <= 1; ++dx)
        {
            if (dx == 0 && dy == 0)
                continue;
            int nCandX = nX + dx;
            vTo = m_pActivePathfindInfo->m_vGridTransformT
                + m_pActivePathfindInfo->m_vGridTransformA * static_cast<float>(nCandX)
                + m_pActivePathfindInfo->m_vGridTransformB * static_cast<float>(nCandY)
                + m_pActivePathfindInfo->m_vGridTransformC * static_cast<float>(nZ);
            if (TestDirectLine(vFrom, vTo, fHeight, fRadius, 0, 0) == 1)
            {
                (*ppOut)[nCount].m_nCostBound = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
                (*ppOut)[nCount].m_nX = nCandX;
                (*ppOut)[nCount].m_nY = nCandY;
                (*ppOut)[nCount].m_nZ = 0;
                {
                    int nKeyX = nCandX;
                    int nKeyY = nCandY;
                    for (int i = nCount; i > 0; --i)
                    {
                        int nPrevX = (*ppOut)[i - 1].m_nX;
                        int nPrevY = (*ppOut)[i - 1].m_nY;
                        int nPrevKey = (nPrevX < 0 ? -nPrevX : nPrevX) + (nPrevY < 0 ? -nPrevY : nPrevY);
                        int nCurKey = (nKeyX < 0 ? -nKeyX : nKeyX) + (nKeyY < 0 ? -nKeyY : nKeyY);
                        if (nCurKey <= nPrevKey)
                        {
                            int nHoldCost = (*ppOut)[i - 1].m_nCostBound;
                            int nHoldZ = (*ppOut)[i - 1].m_nZ;
                            (*ppOut)[i - 1].m_nCostBound = (*ppOut)[i].m_nCostBound;
                            (*ppOut)[i - 1].m_nX = nKeyX;
                            (*ppOut)[i - 1].m_nY = nKeyY;
                            (*ppOut)[i - 1].m_nZ = (*ppOut)[i].m_nZ;
                            (*ppOut)[i].m_nCostBound = nHoldCost;
                            (*ppOut)[i].m_nX = nPrevX;
                            (*ppOut)[i].m_nY = nPrevY;
                            (*ppOut)[i].m_nZ = nHoldZ;
                        }
                        else
                        {
                            nKeyX = nPrevX;
                            nKeyY = nPrevY;
                        }
                    }
                }
                ++nCount;
            }
        }
    }
    return nCount;
}
