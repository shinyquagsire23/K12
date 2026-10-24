// Pathfinding scratch types shared by the server translation units: CAvoidCreature, the goal and
// position records, and CPathfindInformation.
#pragma once

#ifndef CAVOIDCREATURE_H
#define CAVOIDCREATURE_H

#include "Vector.h"

class CSWSCreature;
class CPathfindInformation;

// Walks around a creature standing in the path: fits a hexagon around it, finds the edges the path
// enters and leaves through, and splices a detour along the perimeter.
class CAvoidCreature {
public:
    CAvoidCreature();

    static const int AVOIDCREATURE_FAILED;
    static const int AVOIDCREATURE_SUCSESS;
    static const int AVOIDCREATURE_STARTINHEX;
    static const int AVOIDCREATURE_ENDINHEX;
    static const float LINEFACTOR;

    void ComputeHexPoints(const Vector& a, const Vector& b, float f);
    int  FindEntryAndExit(CPathfindInformation* pPathInfo);
    int  GetClosestPointTo(Vector& v);
    Vector* FindPath(int& n);
    Vector* FindPath_Left(int& n);
    Vector* FindPath_Right(int& n);
    int  IsPathClear(unsigned long long oid, Vector* pv, int n,
                     unsigned long long& oidOut);
    int  SideClearOfObstructions(unsigned long long oid, const Vector& a, const Vector& b,
                                 unsigned long long& oidOut);
    int  PlotPathAroundCreature(CSWSCreature* pSelf, CSWSCreature* pBlocker);
    int  IsPointInHex(const Vector& v);
    int  InsertPathSegmentBetween(int a, int b, Vector* pv, int c,
                                  CPathfindInformation* pPathInfo);
    void Render();

    int    m_nSide;

    Vector m_vHexPoints[6];

    struct HexTransit {
        int    m_nPathSegment;
        int    m_nHexPoint;
        Vector m_vPoint;
    };
    HexTransit m_transit[2];
};

inline CAvoidCreature::CAvoidCreature()
{
    m_nSide = 0;
    for (int i = 0; i < 6; i++) {
        m_vHexPoints[i].x = 0.0f;
        m_vHexPoints[i].y = 0.0f;
        m_vHexPoints[i].z = 0.0f;
    }
    for (int i = 0; i < 2; i++) {
        m_transit[i].m_nPathSegment = -1;
        m_transit[i].m_nHexPoint = -1;
        m_transit[i].m_vPoint.x = 0.0f;
        m_transit[i].m_vPoint.y = 0.0f;
        m_transit[i].m_vPoint.z = 0.0f;
    }
}

#endif


struct CPathfindPos {
    float x, y, z;

    operator const Vector&() const { return *reinterpret_cast<const Vector*>(this); }  // idiom-exempt: layout twin
    CPathfindPos& operator=(const Vector& v)                          // idiom-exempt: layout twin
    {
        *reinterpret_cast<Vector*>(this) = v;
        return *this;
    }
};

// The point a path is aimed at plus the stand-off it stops at.
struct CPathfindGoalPoint {
    CPathfindPos m_vPoint;
    float        m_fRange;
};

struct CPathfindInfoIntraTileSuccessors {
    int m_nReserved1;
    int m_nReserved2;
    int m_nReserved3;
    int m_nReserved4;
    int m_nReserved5;
    int m_nReserved6;

    CPathfindInfoIntraTileSuccessors();
};

// Per-creature pathfinding scratch, reached through CSWSCreature::m_pPathfindInfo.
class CPathfindInformation {
public:
    void ComputeStepTolerance();
    void ResetAtEndOfPath();
    void ResetInterAreaSearchData();
    void ResetGridSearchData();
    void ResetWayPointData();
    void ResetInterTileSearchData();
    void CreateFirstTileFValueAlternatives(int nCount);
    void DeleteFirstTileFValueAlternatives();
    void GetFirstTileFValueAlternatives(int nIndex, int* pnValueA, int* pnValueB,
                                        CPathfindInfoIntraTileSuccessors** ppSuccessor);
    void SetFirstTileFValueAlternatives(int nIndex, int nValueA, int nValueB,
                                        CPathfindInfoIntraTileSuccessors* pSuccessor);
    void ResetPathPointSearchData();
    void ResetIntraTileSearchData();
    void FlipTempEndPoints();
    void FlipStartEndPoints();
    void ResetInterAreaPathSearchData();
    void HandlePathfindError(int nCode, char* pszReason);

    int                m_bClientSidePath;
    union {
        int            m_nPersonalSpaceBits;
        float          m_fPersonalSpace;
    };
    float              m_fCrePersonalSpace;
    float              m_fDesiredOffset;
    float              m_fCameraSpace;
    union {
        float          m_fHeightAlias;
        float          m_fHeight;
    };
    float              m_fHitDistance;
    int                m_nPathType;
    int                m_nReserved1;
    int                m_bReturnClosestResult;
    int                m_nClientPathId;
    char               _pad02c[0x30-0x2c];
    unsigned long long m_oidCreature;
    unsigned long long m_oidGoalObject;
    unsigned long long m_oidIgnoredBlocker;
    unsigned long long m_oidTarget;
    int                m_nPlotInAreaPasses;
    int                m_nReserved2;
    CPathfindGoalPoint m_savedGoal;      // the goal parked while a door is opened
    unsigned long long m_oidSavedGoalArea;
    int                m_bNewPathRequested;
    CPathfindGoalPoint m_goal;           // the live goal and the range it stops at
    int                m_nStepTolerance;
    unsigned long long m_oidGoalArea;
    CPathfindPos       m_vStartPoint;
    char               _pad09c[0xa0-0x9c];
    unsigned long long m_oidStartArea;
    int                m_bPointsReversed;
    int                m_bPlotFinished;
    int                m_nWayPointFloats;
    char               _pad0b4[0xb8-0xb4];
    float*             m_paWayPoints;
    int                m_nRawWayPointFloats;
    char               _pad0c4[0xc8-0xc4];
    float*             m_paRawWayPoints;
    int                m_nWayPointCursor;
    int                m_bInterAreaPathFound;
    int                m_bKeepInterAreaRoute;
    int                m_nInterAreaCount;
    unsigned long long* m_paInterAreaIds;
    unsigned long long* m_paInterAreaTransitionIds;
    int                m_nTraversedTransitions;
    char               _pad0f4[0xf8-0xf4];
    unsigned long long* m_paTraversedTransitions;
    unsigned long long m_oidIgnoreDoor;
    int                m_nInterTileCount;
    int*               m_paInterTileA;
    int*               m_paInterTileB;
    int                m_nReserved3;
    int*               m_paInterTileC;
    int*               m_paInterTileD;
    int                m_nReserved4;
    int                m_nReserved5;
    int                m_nReserved6;
    int                m_nFValueCount;
    int*               m_pFValueA;
    int*               m_pFValueB;
    CPathfindInfoIntraTileSuccessors** m_paFValueSuccessors;
    int*               m_pReserved1;
    int                m_bPathCut;
    int                m_nReserved11;
    unsigned char      m_nPathCutCount;
    char               _pad171[0x174-0x171];
    int                m_nPathCutWaypointsLeft;
    Vector             m_vPathCutPosition;
    Vector             m_vPathCutOrientation;
    int                m_nReserved7;
    int                m_nGridSteps;
    int                m_nReserved8;
    Vector             m_vGridTransformT;
    Vector             m_vGridTransformA;
    Vector             m_vGridTransformB;
    Vector             m_vGridTransformC;
    char                 _pad1cc[0x1d0-0x1cc];
    int*                 m_pGridPath;
    int                  m_nGridPathLen;
    float                m_fGridSearchRadius;
    int                  m_nGridStep;
    int                  m_nReserved9;
    void*                m_pGridSavedTable;
    Vector               m_vGridFrom;
    Vector               m_vGridTo;
    int                  m_nBestGridLen;
    int*                 m_pBestGridPath;
    int                  m_nBestGridDist;
    int                m_bGoalWithinReach;
    int                m_bStraightLineOnly;
    int                m_nDirectLineCode;
    char               m_nReserved10;
    char               _pad229[0x22c-0x229];
    int                m_nSourcePathPoint;
    int                m_nDestPathPoint;
    char               _pad234[0x238-0x234];
    int*               m_pPathPointList;
    int                m_nPathPointCount;
    void*              m_pPathPointTable;
    Vector             m_vCurrentFrom;
    Vector             m_vCurrentTo;
    float              m_fCurrentRadius;
    float              m_fMinGap;
    float*             m_pGridPoints;
    int                m_nBestPathPointLen;
    int                m_nPathState;
    Vector             m_vSavedFrom;
    Vector             m_vSavedTo;
    int                m_nSegCountA;
    float*             m_pSegFloatsA;
    int                m_nSegCountB;
    float*             m_pSegFloatsB;
    float              m_fSearchRadius;
    int                m_bIgnoreCrePath;
    int                m_nSmoothStep;
    int                m_nSmoothingFirstFloats;
    float*             m_paSmoothingFirstWayPoints;
    int                m_nSmoothingSecondFloats;
    int                m_nSmoothingThirdFloats;
    float*             m_paSmoothingSecondWayPoints;
    float*             m_paSmoothingThirdWayPoints;
    int                m_nSmoothResumeIndex;
    int                m_nSmoothResumeSel;
    float*             m_pSmoothResumeFloats;
    int                m_bResendPath;
    int                m_bCheckSightToTarget;
    int                m_nAvoidSide;
    Vector             m_vWalkDestination;
    unsigned long long m_oidPathTarget;
    Vector             m_vPathTarget;
    char               _pad324[0x328-0x324];
    unsigned long long m_oidFoundEnemy;
    int                m_nBumpCount;
    int                m_bForceMoveDeadline;
    int                m_nForceMoveDeadlineDay;
    int                m_nForceMoveDeadlineTimeOfDay;
    // Accessors shared by the pathfinding translation units.
    void SetSmoothingFirstWayPoints(float* p)
    {
        if (m_paSmoothingFirstWayPoints && m_paSmoothingFirstWayPoints != p) {
            m_nSmoothingFirstFloats = 0;
            delete[] m_paSmoothingFirstWayPoints;
            m_paSmoothingFirstWayPoints = 0;
        }
        m_paSmoothingFirstWayPoints = p;
    }
    void SetSmoothingSecondWayPoints(float* p)
    {
        if (m_paSmoothingSecondWayPoints && m_paSmoothingSecondWayPoints != p) {
            m_nSmoothingSecondFloats = 0;
            delete[] m_paSmoothingSecondWayPoints;
            m_paSmoothingSecondWayPoints = 0;
        }
        m_paSmoothingSecondWayPoints = p;
    }
    void SetSmoothingThirdWayPoints(float* p)
    {
        if (m_paSmoothingThirdWayPoints && m_paSmoothingThirdWayPoints != p) {
            m_nSmoothingThirdFloats = 0;
            delete[] m_paSmoothingThirdWayPoints;
            m_paSmoothingThirdWayPoints = 0;
        }
        m_paSmoothingThirdWayPoints = p;
    }
    void SetPathPointStepsArray(int* p)
    {
        if (m_pPathPointList) {
            delete[] m_pPathPointList;
            m_pPathPointList = 0;
        }
        m_pPathPointList = p;
    }
    void SetPathNodeStartPath(int n, float* p)
    {
        m_nSegCountA = n;
        delete[] m_pSegFloatsA;
        m_pSegFloatsA = p;
    }
    void SetPathNodeEndPath(int n, float* p)
    {
        m_nSegCountB = n;
        delete[] m_pSegFloatsB;
        m_pSegFloatsB = p;
    }
    CPathfindInformation();
    ~CPathfindInformation();
    void Initialize();
    void Shutdown() throw();

    // Path-point state selectors, then the smoothing-phase sentinels. No in-class initializer, so a
    // comparison emits a real load.
    static const int PATH_POINT_STARTPOINTS;   // 1
    static const int PATH_POINT_GRIDSTART;     // 2
    static const int PATH_POINT_GRIDEND;       // 3
    static const int PATH_POINT_PATHFIND;      // 4
    static const int PATH_POINT_SMOOTH;        // 5
    static const int SMOOTHING_START;          // 0
    static const int SMOOTHING_STEP_1;         // 1
    static const int SMOOTHING_STEP_2;         // 2
    static const int SMOOTHING_STEP_3;         // 3
    static const int SMOOTHING_STEP_4;         // 4
};
