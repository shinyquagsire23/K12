#include "k2stdint.h"
#include "pathfindinfo.h"
#include "appmanager.h"
#include "Quaternion.h"
#include "exostring.h"
#include "serverexoapp.h"
#include "nwsarea.h"
#include "nwsmodule.h"
#include "nwsobject.h"
#include "nwscreature.h"
#include "SWCollisionMesh.h"
#include "nwscript.h"
#include "math3d.h"

// The lowest reputation that is not hostile (0-10 hostile, 11-89 neutral, 90-100 friendly).
enum {
    REPUTATION_NEUTRAL_MIN = 11,
};

float fPlotAroundCreatureAditionalSpace = 0.200000003f;
int bRenderHexTest;
int nPlotPathAroundCreatureCount;

const int CPathfindInformation::PATH_POINT_STARTPOINTS = 1;
const int CPathfindInformation::PATH_POINT_GRIDSTART = 2;
const int CPathfindInformation::PATH_POINT_GRIDEND = 3;
const int CPathfindInformation::PATH_POINT_PATHFIND = 4;
const int CPathfindInformation::PATH_POINT_SMOOTH = 5;
const int CPathfindInformation::SMOOTHING_START = 0;
const int CPathfindInformation::SMOOTHING_STEP_1 = 1;
const int CPathfindInformation::SMOOTHING_STEP_2 = 2;
const int CPathfindInformation::SMOOTHING_STEP_3 = 3;
const int CPathfindInformation::SMOOTHING_STEP_4 = 4;

// The Vector runs are zeroed a block at a time rather than member by member.
CPathfindInformation::CPathfindInformation()
{
    m_oidIgnoredBlocker = OBJECT_INVALID;
    __builtin_memset(&m_vPathCutPosition, 0, sizeof(Vector) * 2);
    __builtin_memset(&m_vGridTransformT, 0, sizeof(Vector) * 4);
    __builtin_memset(&m_vGridFrom, 0, sizeof(Vector) * 2);
    m_pBestGridPath = 0;
    __builtin_memset(&m_vCurrentFrom, 0, sizeof(Vector) * 2);
    __builtin_memset(&m_vSavedFrom, 0, sizeof(Vector) * 2);
    m_vWalkDestination = Vector();
    m_vPathTarget = Vector();
    Initialize();
}

CPathfindInformation::~CPathfindInformation()
{
    Shutdown();
}

// Seeds every field. The Vector runs are cleared as contiguous blocks.
void CPathfindInformation::Initialize()
{
    m_bClientSidePath = 0;
    m_fPersonalSpace = 0.6f;
    m_fCrePersonalSpace = 0.6f;
    m_fCameraSpace = 0.6f;
    m_fHeight = 0.5f;
    m_nPathType = 0;
    m_nReserved1 = 0;
    m_bGoalWithinReach = 0;
    m_oidCreature = OBJECT_INVALID;
    m_oidGoalObject = OBJECT_INVALID;
    __builtin_memset(&m_nReserved2, 0, sizeof(m_nReserved2) + sizeof(m_savedGoal.m_vPoint));
    m_bReturnClosestResult = 0;
    m_nClientPathId = 0;
    __builtin_memset(&m_oidSavedGoalArea, 0, sizeof(m_oidSavedGoalArea) + sizeof(m_bNewPathRequested) + sizeof(m_goal.m_vPoint) +
                                   sizeof(m_goal.m_fRange) + sizeof(m_nStepTolerance) + sizeof(m_oidGoalArea) +
                                   sizeof(m_vStartPoint));
    m_bPointsReversed = 0;
    __builtin_memset(&m_oidStartArea, 0, sizeof(m_oidStartArea) + sizeof(m_bPointsReversed));
    m_bPlotFinished = 1;
    m_nWayPointFloats = 0;
    m_paWayPoints = 0;
    m_nRawWayPointFloats = 0;
    __builtin_memset(&m_paRawWayPoints, 0, sizeof(m_paRawWayPoints) + sizeof(m_nWayPointCursor) + sizeof(m_bInterAreaPathFound) +
                                    sizeof(m_bKeepInterAreaRoute) + sizeof(m_nInterAreaCount) + sizeof(m_paInterAreaIds) +
                                    sizeof(m_paInterAreaTransitionIds) + sizeof(m_nTraversedTransitions));
    m_paTraversedTransitions = 0;
    m_nInterTileCount = 0;
    __builtin_memset(&m_paInterTileA, 0, sizeof(m_paInterTileA) + sizeof(m_paInterTileB));
    m_nReserved3 = 0;
    __builtin_memset(&m_paInterTileC, 0, sizeof(m_paInterTileC) + sizeof(m_paInterTileD) +
                                       sizeof(m_nReserved4) + sizeof(m_nReserved5) + sizeof(m_nReserved6) +
                                       sizeof(m_nFValueCount) + sizeof(m_pFValueA) + sizeof(m_pFValueB) +
                                       sizeof(m_paFValueSuccessors) + sizeof(m_pReserved1));
    m_bPathCut = 0;
    m_nPathCutCount = 0;
    __builtin_memset(&m_nPathCutWaypointsLeft, 0, sizeof(m_nPathCutWaypointsLeft) + sizeof(m_vPathCutPosition));
    m_vPathCutOrientation = Vector(1.0f, 0.0f, 0.0f);
    m_fGridSearchRadius = 700.0f;
    m_nGridStep = 0;
    __builtin_memset(&m_pGridSavedTable, 0, sizeof(m_pGridSavedTable) + sizeof(m_vGridFrom) + sizeof(m_vGridTo));
    m_nBestGridLen = 0;
    if (m_pBestGridPath)
        delete[] m_pBestGridPath;
    m_pBestGridPath = 0;
    m_nBestGridDist = 700;
    m_bGoalWithinReach = 0;
    m_bStraightLineOnly = 0;
    m_nReserved10 = 0;
    m_nSourcePathPoint = -1;
    m_nDestPathPoint = -1;
    m_pPathPointList = 0;
    m_nPathPointCount = 0;
    m_pPathPointTable = 0;
    m_fCurrentRadius = 0.0f;
    m_fMinGap = 100000.0f;
    m_pGridPoints = 0;
    m_nBestPathPointLen = 0;
    m_nPathState = 1;
    m_pSegFloatsA = 0;
    m_pSegFloatsB = 0;
    m_bIgnoreCrePath = 0;
    __builtin_memset(&m_nSmoothingFirstFloats, 0, sizeof(m_nSmoothingFirstFloats) + sizeof(m_paSmoothingFirstWayPoints) + sizeof(m_nSmoothingSecondFloats) +
                                    sizeof(m_nSmoothingThirdFloats) + sizeof(m_paSmoothingSecondWayPoints) + sizeof(m_paSmoothingThirdWayPoints));
    __builtin_memset(&m_pSmoothResumeFloats, 0,
                     sizeof(m_pSmoothResumeFloats) + sizeof(m_bResendPath) + sizeof(m_bCheckSightToTarget));
    m_nAvoidSide = 0;
    m_oidPathTarget = OBJECT_INVALID;
    m_vPathTarget = Vector();
    m_oidFoundEnemy = OBJECT_INVALID;
    m_bForceMoveDeadline = 0;
    m_nForceMoveDeadlineDay = 0;
    m_nForceMoveDeadlineTimeOfDay = 0;
    m_oidTarget = OBJECT_INVALID;
    m_oidIgnoreDoor = OBJECT_INVALID;
    m_fDesiredOffset = 0.0f;
}

// Reset and scratch helpers; every caller reaches them through the header's declarations.

// Logging is compiled out of the release build; both parameters stay for the call sites.
void CPathfindInformation::HandlePathfindError(int nCode, char* pszReason)
{
    K2_UNUSED(nCode);
    K2_UNUSED(pszReason);
}

// Squared-distance cutoff for the grid search, scaled down by the creature's personal space.
void CPathfindInformation::ComputeStepTolerance()
{
    m_nStepTolerance = static_cast<int>(m_fPersonalSpace == 0.0f ? m_goal.m_fRange
                                              : m_goal.m_fRange / m_fPersonalSpace);
}

// Swaps the live goal (m_goal.m_vPoint/m_oidGoalArea) with the source (m_vStartPoint/m_oidStartArea).
void CPathfindInformation::FlipStartEndPoints()
{
    unsigned long long nToArea = m_oidGoalArea;
    float b0 = m_vStartPoint.x;
    float b1 = m_vStartPoint.y;
    float a0 = m_goal.m_vPoint.x;
    float a1 = m_goal.m_vPoint.y;
    float a2 = m_goal.m_vPoint.z;
    float b2 = m_vStartPoint.z;
    unsigned long long nFromArea = m_oidStartArea;
    m_vStartPoint.x = a0;
    m_vStartPoint.y = a1;
    m_vStartPoint.z = a2;
    m_goal.m_vPoint.x = b0;
    m_goal.m_vPoint.y = b1;
    m_goal.m_vPoint.z = b2;
    m_oidGoalArea = nFromArea;
    m_oidStartArea = nToArea;
}

// Swaps the saved goal (m_savedGoal.m_vPoint/m_oidSavedGoalArea) with the live one (m_goal.m_vPoint/m_oidGoalArea).
void CPathfindInformation::FlipTempEndPoints()
{
    unsigned long long nSavedArea = m_oidSavedGoalArea;
    float a0 = m_savedGoal.m_vPoint.x;
    float a1 = m_savedGoal.m_vPoint.y;
    float a2 = m_savedGoal.m_vPoint.z;
    float a3 = m_savedGoal.m_fRange;
    float b0 = m_goal.m_vPoint.x;
    float b1 = m_goal.m_vPoint.y;
    float b2 = m_goal.m_vPoint.z;
    float b3 = m_goal.m_fRange;
    unsigned long long nLiveArea = m_oidGoalArea;
    m_goal.m_vPoint.x = a0;
    m_goal.m_vPoint.y = a1;
    m_goal.m_vPoint.z = a2;
    m_goal.m_fRange = a3;
    m_savedGoal.m_vPoint.x = b0;
    m_savedGoal.m_vPoint.y = b1;
    m_savedGoal.m_vPoint.z = b2;
    m_savedGoal.m_fRange = b3;
    m_oidGoalArea = nSavedArea;
    m_oidSavedGoalArea = nLiveArea;
}

// Drops the cross-plot transition history and re-arms the search.
void CPathfindInformation::ResetInterAreaSearchData()
{
    if (m_paTraversedTransitions)
        delete[] m_paTraversedTransitions;
    m_nPathType = 0;
    m_nReserved1 = 0;
    m_bReturnClosestResult = 0;
    m_nTraversedTransitions = 0;
    m_bPointsReversed = 0;
    m_bPlotFinished = 1;
    m_paTraversedTransitions = 0;
    ResetInterAreaPathSearchData();
}

void CPathfindInformation::ResetGridSearchData()
{
    m_fGridSearchRadius = 700.0f;
    m_nGridStep = 0;
    __builtin_memset(&m_vGridFrom, 0, sizeof(Vector) * 2);
    m_nBestGridLen = 0;
    delete[] m_pBestGridPath;
    m_pBestGridPath = 0;
    m_nBestGridDist = 700;
    m_fMinGap = 100000.0f;
}

void CPathfindInformation::ResetWayPointData()
{
    delete[] m_paWayPoints;
    delete[] m_paRawWayPoints;
    m_nWayPointFloats = 0;
    m_paWayPoints = 0;
    m_nRawWayPointFloats = 0;
    m_paRawWayPoints = 0;
    m_nWayPointCursor = 0;
    m_bPathCut = 0;
    __builtin_memset(&m_nPathCutWaypointsLeft, 0, sizeof(m_nPathCutWaypointsLeft) + sizeof(m_vPathCutPosition));
    m_vPathCutOrientation = Vector(1.0f, 0.0f, 0.0f);
}

void CPathfindInformation::ResetAtEndOfPath()
{
    m_bGoalWithinReach = 0;
    m_oidStartArea = 0;
    __builtin_memset(&m_nReserved2, 0, sizeof(m_nReserved2) + sizeof(m_savedGoal.m_vPoint));
    __builtin_memset(&m_bNewPathRequested, 0, sizeof(m_bNewPathRequested) + sizeof(m_goal.m_vPoint));
    m_oidSavedGoalArea = 0;
    m_goal.m_fRange = 0.0f;
    __builtin_memset(&m_oidGoalArea, 0, sizeof(m_oidGoalArea) + sizeof(m_vStartPoint));
    ResetInterAreaSearchData();
}

CPathfindInfoIntraTileSuccessors::CPathfindInfoIntraTileSuccessors()
    : m_nReserved1(0)
    , m_nReserved2(0)
    , m_nReserved3(0)
    , m_nReserved4(0)
    , m_nReserved5(0)
    , m_nReserved6(0)
{
}

void CPathfindInformation::ResetPathPointSearchData()
{
    m_nBestPathPointLen = 0;
    __builtin_memset(&m_vCurrentFrom, 0, sizeof(Vector) * 2);
    m_fCurrentRadius = 0.0f;
    if (m_pGridPoints)
        delete[] m_pGridPoints;
    m_pGridPoints = 0;
    m_fSearchRadius = 700.0f;
    m_fMinGap = 3.402823466e+38f;   // FLT_MAX
}

void CPathfindInformation::ResetIntraTileSearchData()
{
    if (m_paFValueSuccessors)
        DeleteFirstTileFValueAlternatives();
    m_nReserved4 = 0;
    m_nReserved5 = 0;
    m_nReserved6 = 0;
    if (m_pReserved1)
        delete[] m_pReserved1;
    m_pReserved1 = 0;
}

void CPathfindInformation::ResetInterTileSearchData()
{
    if (m_paInterTileA) delete[] m_paInterTileA;
    if (m_paInterTileB) delete[] m_paInterTileB;
    if (m_paInterTileC) delete[] m_paInterTileC;
    if (m_paInterTileD) delete[] m_paInterTileD;
    m_nInterTileCount = 0;
    __builtin_memset(&m_paInterTileA, 0, sizeof(m_paInterTileA) + sizeof(m_paInterTileB) + sizeof(m_nReserved3));
    __builtin_memset(&m_paInterTileC, 0, sizeof(m_paInterTileC) + sizeof(m_paInterTileD));
    ResetIntraTileSearchData();
}

void CPathfindInformation::DeleteFirstTileFValueAlternatives()
{
    if (!m_paFValueSuccessors)
        return;
    for (int i = 0; i < m_nFValueCount; i++) {
        if (m_paFValueSuccessors[i]) {
            delete[] m_paFValueSuccessors[i];
            m_paFValueSuccessors[i] = 0;
        }
    }
    if (m_pFValueA) {
        delete[] m_pFValueA;
        m_pFValueA = 0;
    }
    if (m_pFValueB) {
        delete[] m_pFValueB;
        m_pFValueB = 0;
    }
    m_nFValueCount = 0;
    delete[] m_paFValueSuccessors;
    m_paFValueSuccessors = 0;
}

void CPathfindInformation::GetFirstTileFValueAlternatives(int nIndex, int* pnValueA, int* pnValueB,
                                                          CPathfindInfoIntraTileSuccessors** ppSuccessor)
{
    *pnValueA = m_pFValueA[nIndex];
    *pnValueB = m_pFValueB[nIndex];
    *ppSuccessor = m_paFValueSuccessors[nIndex];
}

void CPathfindInformation::SetFirstTileFValueAlternatives(int nIndex, int nValueA, int nValueB,
                                                          CPathfindInfoIntraTileSuccessors* pSuccessor)
{
    m_pFValueA[nIndex] = nValueA;
    m_pFValueB[nIndex] = nValueB;
    m_paFValueSuccessors[nIndex] = pSuccessor;
}

void CPathfindInformation::CreateFirstTileFValueAlternatives(int nCount)
{
    if (m_paFValueSuccessors)
        DeleteFirstTileFValueAlternatives();
    m_paFValueSuccessors = new CPathfindInfoIntraTileSuccessors*[nCount];
    for (int i = 0; i < nCount; i++)
        m_paFValueSuccessors[i] = 0;
    m_pFValueA = new int[nCount];
    m_pFValueB = new int[nCount];
    m_nFValueCount = nCount;
}

void CPathfindInformation::ResetInterAreaPathSearchData()
{
    if (m_paInterAreaIds)
        delete[] m_paInterAreaIds;
    if (m_paInterAreaTransitionIds)
        delete[] m_paInterAreaTransitionIds;
    __builtin_memset(&m_bInterAreaPathFound, 0,
                     sizeof(m_bInterAreaPathFound) + sizeof(m_bKeepInterAreaRoute) + sizeof(m_nInterAreaCount) +
                         sizeof(m_paInterAreaIds) + sizeof(m_paInterAreaTransitionIds));
    ResetGridSearchData();
    ResetWayPointData();
    ResetInterTileSearchData();
    ResetPathPointSearchData();
}

// Releases every owned pointer; the destructor calls this.
void CPathfindInformation::Shutdown() throw()
{
    if (m_paWayPoints)
        delete[] m_paWayPoints;
    if (m_paRawWayPoints)
        delete[] m_paRawWayPoints;
    if (m_paInterAreaIds)
        delete[] m_paInterAreaIds;
    if (m_paInterAreaTransitionIds)
        delete[] m_paInterAreaTransitionIds;
    if (m_paTraversedTransitions)
        delete[] m_paTraversedTransitions;
    if (m_paInterTileA)
        delete[] m_paInterTileA;
    if (m_paInterTileB)
        delete[] m_paInterTileB;
    if (m_paInterTileC)
        delete[] m_paInterTileC;
    if (m_paInterTileD)
        delete[] m_paInterTileD;
    if (m_paFValueSuccessors)
        DeleteFirstTileFValueAlternatives();
    if (m_pFValueA) {
        delete[] m_pFValueA;
        m_pFValueA = 0;
    }
    if (m_pFValueB) {
        delete[] m_pFValueB;
        m_pFValueB = 0;
    }
    if (m_pGridSavedTable) {
        ::operator delete(m_pGridSavedTable);
        m_pGridSavedTable = 0;
    }
    if (m_pSmoothResumeFloats) {
        delete[] m_pSmoothResumeFloats;
        m_pSmoothResumeFloats = 0;
    }
    if (m_pPathPointList) {
        delete[] m_pPathPointList;
        m_pPathPointList = 0;
    }
    m_pPathPointList = 0;
    if (m_paSmoothingFirstWayPoints) {
        m_nSmoothingFirstFloats = 0;
        delete[] m_paSmoothingFirstWayPoints;
        m_paSmoothingFirstWayPoints = 0;
    }
    m_paSmoothingFirstWayPoints = 0;
    if (m_paSmoothingSecondWayPoints) {
        m_nSmoothingSecondFloats = 0;
        delete[] m_paSmoothingSecondWayPoints;
        m_paSmoothingSecondWayPoints = 0;
    }
    m_paSmoothingSecondWayPoints = 0;
    if (m_paSmoothingThirdWayPoints) {
        m_nSmoothingThirdFloats = 0;
        delete[] m_paSmoothingThirdWayPoints;
        m_paSmoothingThirdWayPoints = 0;
    }
    m_paSmoothingThirdWayPoints = 0;
}

// The "walk around the creature standing in my way" solver, a short-lived stack object that models
// the blocker as a hexagon, finds the edges the current path enters and leaves through, and splices
// a detour along the perimeter.

#include "port/libcstring.h"

extern int   quaternionidentityoptimization;
extern int   bRenderHexTest;
extern float fPlotAroundCreatureAditionalSpace;
extern int   nPlotPathAroundCreatureCount;

const int   CAvoidCreature::AVOIDCREATURE_FAILED     = 0;
const int   CAvoidCreature::AVOIDCREATURE_SUCSESS    = 1;
const int   CAvoidCreature::AVOIDCREATURE_STARTINHEX = -1;
const int   CAvoidCreature::AVOIDCREATURE_ENDINHEX   = -2;
const float CAvoidCreature::LINEFACTOR               = 0.866025f;

// Rotates a vector by a quaternion.
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
        r.x = v.x * (1.0f - ((yy + zz) + (yy + zz))) + v.y * ((xy - wz) + (xy - wz)) + v.z * ((xz + wy) + (xz + wy));
        r.y = v.x * ((xy + wz) + (xy + wz)) + v.y * (1.0f - ((xx + zz) + (xx + zz))) + v.z * ((yz - wx) + (yz - wx));
        r.z = v.x * ((xz - wy) + (xz - wy)) + v.y * ((yz + wx) + (yz + wx)) + v.z * (1.0f - ((xx + yy) + (xx + yy)));
        return r;
    }
    return v;
}

// The four "is this segment end sitting on that hex corner" probes are one squared distance.
static inline float DistSq(const Vector& a, const Vector& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

// Works out which hex edge the flat xy path enters through (m_transit[0]) and which it leaves
// through (m_transit[1]). Each path segment is tested against all six edges two ways: a corner-touch
// test, which fills the exit record, and an IntersectLineSegments2d hit, which fills the exit record
// on the first hit and rolls entry <- exit on every later one. Whichever end was never found is
// filled from the nearest hex corner.
int CAvoidCreature::FindEntryAndExit(CPathfindInformation* pPathInfo)
{
    Vector vFrom;
    Vector vTo;
    int nTouching = 0;
    if (pPathInfo == 0 || pPathInfo->m_paWayPoints == 0 || pPathInfo->m_nWayPointFloats == 0) return 0;
    vFrom = Vector(pPathInfo->m_paWayPoints[0], pPathInfo->m_paWayPoints[1], 0.0f);
    nTouching = 0;
    if (pPathInfo->m_nWayPointFloats > 2) {
        int i = 2;
        do {
            vTo = Vector(pPathInfo->m_paWayPoints[i], pPathInfo->m_paWayPoints[i + 1], 0.0f);
            Vector vHit;
            for (int k = 0; k < 6; k++) {
                const Vector& vNext = m_vHexPoints[(k + 1) % 6];
                const Vector& vCur = m_vHexPoints[k];
                bool bFromNext = DistSq(vFrom, vNext) < 0.001f;
                bool bFromCur = DistSq(vFrom, vCur) < 0.001f;
                bool bToCur = DistSq(vTo, vCur) < 0.001f;
                bool bToNext = DistSq(vTo, vNext) < 0.001f;
                if (static_cast<int>(bFromNext) + static_cast<int>(bFromCur) + static_cast<int>(bToCur) +
                        static_cast<int>(bToNext) > 1) {
                    if (nTouching == 0) {
                        if (bFromCur || bFromNext) {
                            m_transit[1].m_nPathSegment = i / 2;
                            m_transit[1].m_nHexPoint = k;
                            nTouching = 1;
                            m_transit[1].m_vPoint = vFrom;
                        } else {
                            nTouching = 0;
                        }
                    }
                    if (bToCur || bToNext) {
                        m_transit[1].m_nPathSegment = i / 2;
                        m_transit[1].m_nHexPoint = k;
                        nTouching++;
                        m_transit[1].m_vPoint = vTo;
                    }
                }
                if (IntersectLineSegments2d(vFrom, vTo, vCur, vNext, vHit) != 0) {
                    if (nTouching == 0) {
                        m_transit[1].m_nPathSegment = i / 2;
                        m_transit[1].m_nHexPoint = k;
                        m_transit[1].m_vPoint = vHit;
                        nTouching = 1;
                    } else if (nTouching >= 1) {
                        m_transit[0].m_nPathSegment = m_transit[1].m_nPathSegment;
                        m_transit[0].m_nHexPoint = m_transit[1].m_nHexPoint;
                        m_transit[0].m_vPoint = m_transit[1].m_vPoint;
                        nTouching++;
                        m_transit[1].m_nPathSegment = i / 2;
                        m_transit[1].m_nHexPoint = k;
                        m_transit[1].m_vPoint = vHit;
                    }
                    break;
                }
            }
            vFrom = vTo;
            i += 2;
        } while (i < pPathInfo->m_nWayPointFloats);
    }
    if (m_transit[0].m_nHexPoint == -1) {
        if (m_transit[1].m_nHexPoint == -1) return 0;
        int nStart = pPathInfo->m_nWayPointCursor - 2;
        if (nStart < 0) nStart = 0;
        vTo = Vector(pPathInfo->m_paWayPoints[nStart], pPathInfo->m_paWayPoints[nStart + 1], 0.0f);
        int nBest = GetClosestPointTo(vTo);
        m_transit[0].m_nHexPoint = nBest;
        int nSegment = 0;
        if (pPathInfo->m_nWayPointCursor >= 4) nSegment = (pPathInfo->m_nWayPointCursor - 2) / 2;
        m_transit[0].m_nPathSegment = nSegment;
        m_transit[0].m_vPoint = m_vHexPoints[nBest];
        nTouching++;
    } else {
        if (m_transit[1].m_nHexPoint == -1) {
            int nCount = pPathInfo->m_nWayPointFloats;
            vTo = Vector(pPathInfo->m_paWayPoints[nCount - 2], pPathInfo->m_paWayPoints[nCount - 1], 0.0f);
            int nBest = GetClosestPointTo(vTo);
            m_transit[1].m_nPathSegment = nCount / 2;
            m_transit[1].m_nHexPoint = nBest;
            m_transit[1].m_vPoint = m_vHexPoints[nBest];
            nTouching++;
        }
    }
    bool bResult = nTouching > 1;
    equal(m_transit[1].m_vPoint, m_transit[0].m_vPoint);
    return bResult;
}

// Aims the path target at the blocker, warns it off when it is hostile, fits the hexagon around it,
// then tries both perimeters and keeps whichever walks clear of obstructions. Each trial is ranked
// 2 = not blocked, 1 = blocked by a creature, 0 = blocked with nothing found; the better rank wins
// and ties go to the first trial.
int CAvoidCreature::PlotPathAroundCreature(CSWSCreature* pSelf, CSWSCreature* pBlocker)
{
    nPlotPathAroundCreatureCount++;
    pSelf->m_pPathfindInfo->m_oidPathTarget = pBlocker->m_objectId;
    if (pBlocker->m_objectId == OBJECT_INVALID) {
        pSelf->m_pPathfindInfo->m_vPathTarget = Vector();
    }
    pSelf->m_pPathfindInfo->m_vPathTarget = pBlocker->m_position;
    unsigned long long oidAvoid1 = OBJECT_INVALID;
    unsigned long long oidAvoid2 = OBJECT_INVALID;
    int nPoints1 = 0;
    int nPoints2 = 0;
    CExoString sScript("k_def_pathfail01");
    if (pBlocker->GetReputation(pSelf->m_objectId, 0) < REPUTATION_NEUTRAL_MIN) {
        pSelf->m_pPathfindInfo->m_oidFoundEnemy = pBlocker->m_objectId;
        g_pVirtualMachine->RunScript(&sScript, pSelf->m_objectId, pSelf->m_objectId != OBJECT_INVALID);
        if (pSelf->m_bCommandable != 0) return AVOIDCREATURE_FAILED;
    }
    ComputeHexPoints(pBlocker->m_position, pSelf->m_orientation,
                     pBlocker->m_pPathfindInfo->m_fCrePersonalSpace +
                         pSelf->m_pPathfindInfo->m_fCrePersonalSpace +
                         fPlotAroundCreatureAditionalSpace);
    IsPointInHex(Vector(pSelf->m_pPathfindInfo->m_paWayPoints[0],
                        pSelf->m_pPathfindInfo->m_paWayPoints[1], 0.0f));
    Vector vPathEnd(pSelf->m_pPathfindInfo->m_paWayPoints[pSelf->m_pPathfindInfo->m_nWayPointFloats - 2],
                    pSelf->m_pPathfindInfo->m_paWayPoints[pSelf->m_pPathfindInfo->m_nWayPointFloats - 1], 0.0f);
    if (IsPointInHex(vPathEnd) != 0) return AVOIDCREATURE_ENDINHEX;
    if (FindEntryAndExit(pSelf->m_pPathfindInfo) == 0) return AVOIDCREATURE_FAILED;

    int bBlocked1 = 0;
    Vector* pTrial1 = FindPath(nPoints1);
    {
        Vector vFrom = pTrial1[0];
        Vector vTo;
        oidAvoid1 = OBJECT_INVALID;
        if (nPoints1 >= 2) {
            uint64_t nSeg = static_cast<uint64_t>(nPoints1) - 1;
            Vector* pPt = pTrial1 + 1;
            do {
                vTo = *pPt;
                if (SideClearOfObstructions(pSelf->m_objectId, vFrom, vTo, oidAvoid1) == 0) {
                    bBlocked1 = 1;
                    break;
                }
                nSeg--;
                vFrom = vTo;
                pPt++;
            } while (nSeg != 0);
        }
    }
    int nOldSide = m_nSide;
    m_nSide = (nOldSide == 0);
    int bBlocked2 = 0;
    Vector* pTrial2 = FindPath(nPoints2);
    {
        Vector vFrom = pTrial2[0];
        Vector vTo;
        oidAvoid2 = OBJECT_INVALID;
        if (nPoints2 >= 2) {
            uint64_t nSeg = static_cast<uint64_t>(nPoints2) - 1;
            Vector* pPt = pTrial2 + 1;
            do {
                vTo = *pPt;
                if (SideClearOfObstructions(pSelf->m_objectId, vFrom, vTo, oidAvoid2) == 0) {
                    bBlocked2 = 1;
                    break;
                }
                nSeg--;
                vFrom = vTo;
                pPt++;
            } while (nSeg != 0);
        }
    }
    int nClass1 = 0;
    if (!bBlocked1) {
        nClass1 = 2;
    } else if (oidAvoid1 != OBJECT_INVALID) {
        nClass1 = 1;
    } else {
        nClass1 = 0;
    }
    int nClass2 = 0;
    if (!bBlocked2) {
        nClass2 = 2;
    } else if (oidAvoid2 != OBJECT_INVALID) {
        nClass2 = 1;
    } else {
        nClass2 = 0;
    }
    int nPick = (nClass2 != 0) * 2;
    if (nClass2 <= nClass1) nPick = (nClass1 != 0);
    int nResult = 0;
    if (nPick == 0) {
        pSelf->m_pPathfindInfo->m_oidFoundEnemy = OBJECT_INVALID;
        g_pVirtualMachine->RunScript(&sScript, pSelf->m_objectId, pSelf->m_objectId != OBJECT_INVALID);
    } else if (nPick == 1) {
        InsertPathSegmentBetween(m_transit[0].m_nPathSegment, m_transit[1].m_nPathSegment, pTrial1,
                                 nPoints1, pSelf->m_pPathfindInfo);
        if (m_transit[0].m_nPathSegment * 2 < pSelf->m_pPathfindInfo->m_nWayPointCursor) {
            int nCursor = m_transit[0].m_nPathSegment * 2;
            if (nCursor < 0) nCursor = 0;
            pSelf->m_pPathfindInfo->m_nWayPointCursor = nCursor;
        }
        if (oidAvoid1 != OBJECT_INVALID) {
            CSWSCreature* pOther =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidAvoid1);
            if (pOther != 0 && pOther->GetReputation(pSelf->m_objectId, 0) < REPUTATION_NEUTRAL_MIN) {
                pSelf->m_pPathfindInfo->m_oidFoundEnemy = oidAvoid1;
                g_pVirtualMachine->RunScript(&sScript, pSelf->m_objectId,
                                             pSelf->m_objectId != OBJECT_INVALID);
            }
        }
        nResult = AVOIDCREATURE_SUCSESS;
    } else if (nPick == 2) {
        InsertPathSegmentBetween(m_transit[0].m_nPathSegment, m_transit[1].m_nPathSegment, pTrial2,
                                 nPoints2, pSelf->m_pPathfindInfo);
        if (m_transit[0].m_nPathSegment * 2 < pSelf->m_pPathfindInfo->m_nWayPointCursor) {
            int nCursor = m_transit[0].m_nPathSegment * 2;
            if (nCursor < 0) nCursor = 0;
            pSelf->m_pPathfindInfo->m_nWayPointCursor = nCursor;
        }
        if (oidAvoid2 != OBJECT_INVALID) {
            CSWSCreature* pOther =
                g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oidAvoid2);
            if (pOther != 0 && pOther->GetReputation(pSelf->m_objectId, 0) < REPUTATION_NEUTRAL_MIN) {
                pSelf->m_pPathfindInfo->m_oidFoundEnemy = oidAvoid2;
                g_pVirtualMachine->RunScript(&sScript, pSelf->m_objectId,
                                             pSelf->m_objectId != OBJECT_INVALID);
            }
        }
        nResult = AVOIDCREATURE_SUCSESS;
    }
    delete[] pTrial1;
    delete[] pTrial2;
    return nResult;
}

// Fits the hexagon around a. The first corner is a + vStep with no height query; the remaining five
// walk the step vector round a further -60 degrees each and resolve their ground height.
void CAvoidCreature::ComputeHexPoints(const Vector& a, const Vector& b, float f)
{
    __builtin_memset(&m_vHexPoints, 0, sizeof(m_vHexPoints));
    for (int i = 0; i < 2; i++) {
        m_transit[i].m_nPathSegment = -1;
        m_transit[i].m_nHexPoint = -1;
        m_transit[i].m_vPoint.x = 0.0f;
        m_transit[i].m_vPoint.y = 0.0f;
        m_transit[i].m_vPoint.z = 0.0f;
    }
    Vector vStep = normalize(-b) * (f / LINEFACTOR);
    Quaternion q(Vector(0.0f, 0.0f, 1.0f), -1.0471976f);
    Vector vPoint = a + vStep;
    m_vHexPoints[0] = vPoint;
#pragma clang loop unroll(full)
    for (int i = 1; i < 6; i++) {
        vStep = RotateVector(vStep, q);
        vPoint = a + vStep;
        vPoint.z = g_pAppManager->m_pServerExoApp->ComputeHeight(
            static_cast<CSWSArea*>(g_pAppManager->m_pServerExoApp->GetModule()->GetArea())->m_resRef, vPoint,
            1);
        m_vHexPoints[i] = vPoint;
    }
}

// Walks the polyline one segment at a time; the first blocked segment wins.
int CAvoidCreature::IsPathClear(unsigned long long oid, Vector* pv, int n,
                                unsigned long long& oidOut)
{
    oidOut = OBJECT_INVALID;
    Vector vFrom = pv[0];
    Vector vTo;
    for (int i = 1; i < n; i++) {
        vTo = pv[i];
        if (SideClearOfObstructions(oid, vFrom, vTo, oidOut) == 0) return 0;
        vFrom = vTo;
    }
    return 1;
}

// A two-point path when entry and exit share a hex corner, otherwise the side-dependent perimeter
// walk. Returns the newly allocated point array.
Vector* CAvoidCreature::FindPath(int& n)
{
    if (m_transit[0].m_nHexPoint == m_transit[1].m_nHexPoint) {
        Vector* pResult = new Vector[2];
        pResult[0] = m_transit[0].m_vPoint;
        pResult[1] = m_transit[1].m_vPoint;
        n = 2;
        return pResult;
    }
    if (m_nSide != 0) return FindPath_Left(n);
    return FindPath_Right(n);
}

// Walks the hex perimeter downwards from the entry corner, wrapping a negative index to 5.
#if KOTOR1  // K1 keeps it out of line in PlotPathAroundCreature
__attribute__((noinline))
#endif
Vector* CAvoidCreature::FindPath_Left(int& n)
{
    Vector aPoints[10];
    n = 0;
    int nCorner = m_transit[0].m_nHexPoint;
    aPoints[n++] = m_transit[0].m_vPoint;
    aPoints[n++] = m_vHexPoints[nCorner];
    if (nCorner != (m_transit[1].m_nHexPoint + 1) % 6) {
        do {
            nCorner = (nCorner - 1) % 6;
            if (nCorner < 0) nCorner = 5;
            aPoints[n++] = m_vHexPoints[nCorner];
        } while (nCorner != (m_transit[1].m_nHexPoint + 1) % 6);
    }
    aPoints[n++] = m_transit[1].m_vPoint;
    Vector* pResult = new Vector[n];
    memcpy(pResult, aPoints, n * static_cast<int>(sizeof(Vector)));
    return pResult;
}

// The mirror of FindPath_Left, where the corner index steps up with `% 6` and never goes negative.
#if KOTOR1  // K1 keeps it out of line in PlotPathAroundCreature
__attribute__((noinline))
#endif
Vector* CAvoidCreature::FindPath_Right(int& n)
{
    Vector aPoints[10];
    n = 0;
    int nCorner = (m_transit[0].m_nHexPoint + 1) % 6;
    aPoints[n++] = m_transit[0].m_vPoint;
    aPoints[n++] = m_vHexPoints[nCorner];
    if (nCorner != m_transit[1].m_nHexPoint) {
        do {
            nCorner = (nCorner + 1) % 6;
            aPoints[n++] = m_vHexPoints[nCorner];
        } while (nCorner != m_transit[1].m_nHexPoint);
    }
    aPoints[n++] = m_transit[1].m_vPoint;
    Vector* pResult = new Vector[n];
    memcpy(pResult, aPoints, n * static_cast<int>(sizeof(Vector)));
    return pResult;
}

// Line-of-sight probe for one hex edge. Only the clipped and success codes are meaningful; the rest
// all map to 0.
int CAvoidCreature::SideClearOfObstructions(unsigned long long oid, const Vector& a,
                                            const Vector& b, unsigned long long& oidOut)
{
    CWalkHitInfo hitInfo;
    oidOut = OBJECT_INVALID;
    hitInfo.m_hitSubType = 0;
    hitInfo.m_hitObjectId = OBJECT_INVALID;
    __builtin_memset(&hitInfo, 0, __builtin_offsetof(CWalkHitInfo, m_hitObjectId));
    int nResult;
    CSWSCreature* pCreature = g_pAppManager->m_pServerExoApp->GetCreatureByGameObjectID(oid);
    if (pCreature == 0) {
        nResult = -4;
    } else {
        nResult = g_pAppManager->m_pServerExoApp->TestDirectLine(
            oid,
            static_cast<CSWSArea*>(g_pAppManager->m_pServerExoApp->GetModule()->GetArea())->m_resRef, a, b,
            static_cast<float>(pCreature->m_pPathfindInfo->m_fPersonalSpace),
            pCreature->m_pPathfindInfo->m_fHeight, 1, &hitInfo);
    }
    if (bRenderHexTest != 0) {
        Vector vA = a;
        Vector vB = b;
        vA.z = g_pAppManager->m_pServerExoApp->ComputeHeight(
                   static_cast<CSWSArea*>(g_pAppManager->m_pServerExoApp->GetModule()->GetArea())->m_resRef,
                   vA, 0) +
               0.2f;
        vB.z = g_pAppManager->m_pServerExoApp->ComputeHeight(
            static_cast<CSWSArea*>(g_pAppManager->m_pServerExoApp->GetModule()->GetArea())->m_resRef, vB, 0);
    }
    switch (nResult) {
    case 1:
        break;
    case -3:
        nResult = 0;
        oidOut = hitInfo.m_hitObjectId;
        break;
    case -2:
        nResult = 0;
        break;
    case -1:
        nResult = 0;
        break;
    case 0:
        nResult = 0;
        break;
    default:
        nResult = 0;
        break;
    }
    return nResult;
}

// Six-corner nearest-point scan. -1 is the "nothing was ever closer than FLT_MAX" seed.
int CAvoidCreature::GetClosestPointTo(Vector& v)
{
    float fMin = 3.4028235e+38f;
    int nBest = -1;
    for (int i = 0; i < 6; i++) {
        float fX = m_vHexPoints[i].x - v.x;
        float fY = m_vHexPoints[i].y - v.y;
        float fZ = m_vHexPoints[i].z - v.z;
        float fDist = fX * fX + fY * fY + fZ * fZ;
        if (fDist < fMin) {
            fMin = fDist;
            nBest = i;
        }
    }
    return nBest;
}

// An outside-the-convex-polygon test, where for each edge the point must lie on the hexagon's
// winding side.
int CAvoidCreature::IsPointInHex(const Vector& v)
{
    for (int i = 0; i < 6; i++) {
        const Vector& a = m_vHexPoints[i];
        const Vector& b = m_vHexPoints[(i + 1) % 6];
        Vector vNormal =
            normalize(-cross(Vector(0.0f, 0.0f, 1.0f), Vector(b.x - a.x, b.y - a.y, b.z - a.z)));
        Vector vDelta = normalize(Vector(v.x - a.x, v.y - a.y, v.z - a.z));
        if (vNormal.x * vDelta.x + vNormal.y * vDelta.y + vNormal.z * vDelta.z < 0.0f) return 0;
    }
    return 1;
}

// Rebuilds the flat xy path as prefix [0, nStart) + the n new points + suffix [nEnd, old) and takes
// ownership of the old buffer. The new count is (nStart - nEnd) * 2 only when the two indices
// differ; the degenerate nStart == nEnd case drops a point instead of duplicating the seam.
int CAvoidCreature::InsertPathSegmentBetween(int nStart, int nEnd, Vector* pv, int n,
                                             CPathfindInformation* pPathInfo)
{
    float* pOld = pPathInfo->m_paWayPoints;
    int nOldCount = pPathInfo->m_nWayPointFloats;
    int nDiff = nStart - nEnd;
    int nExtra;
    int nSame;
    if (nDiff == 0) {
        nExtra = n * 2 - 2;
        nSame = 1;
    } else {
        nExtra = (nDiff + n) * 2;
        nSame = 0;
    }
    int nNewCount = nExtra + nOldCount;
    float* pNew = new float[nNewCount];
    // FindEntryAndExit keeps nStart and nEnd + nSame within nOldCount / 2, so the prefix and the new
    // points fit in nNewCount, and the suffix fills it exactly.
    int i = 0;
    for (int j = 0; j < nStart; j++) {
        K2_ASSUME(i >= 0 && i + 1 < nNewCount);
        pNew[i] = pOld[i];
        pNew[i + 1] = pOld[i + 1];
        i += 2;
    }
    for (int j = 0; j < n; j++) {
        K2_ASSUME(i >= 0 && i + 1 < nNewCount);
        pNew[i] = pv[j].x;
        pNew[i + 1] = pv[j].y;
        i += 2;
    }
    if (nEnd + nSame < (nOldCount < 0 ? nOldCount + 1 : nOldCount) / 2) {
        for (int j = nEnd + nSame; j < (nOldCount < 0 ? nOldCount + 1 : nOldCount) / 2; j++) {
            K2_ASSUME(i >= 0 && i + 1 < nNewCount);
            pNew[i] = pOld[j * 2];
            pNew[i + 1] = pOld[j * 2 + 1];
            i += 2;
        }
        pPathInfo->m_paWayPoints = 0;
        pPathInfo->m_nWayPointFloats = 0;
    } else {
        pPathInfo->m_paWayPoints = 0;
        pPathInfo->m_nWayPointFloats = 0;
        if (pOld == 0) goto done;
    }
    delete[] pOld;
done:
    pPathInfo->m_paWayPoints = pNew;
    pPathInfo->m_nWayPointFloats = nNewCount;
    return 1;
}

// The debug renderer is compiled out of the release build.
void CAvoidCreature::Render()
{
}
