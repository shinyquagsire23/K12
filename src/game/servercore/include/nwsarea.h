#pragma once

#include "k2stdint.h"
#include "nwarea.h"
#include "CResHelper.h"
#include "gameobjectarray.h"
#include "exostring.h"
#include "exolocstring.h"
#include "Vector.h"
#include "CExoArrayListDtors.h"
#include "exoresman.h"
#include "CExoLinkedList.h"
#include "swvartable.h"
#include "nwsvartable.h"

class CSWSRoom;
class CERFFile;
class CPathfindInformation;
struct CWalkHitInfo;
class CSWCArea;
class CLYT;
class CSWSPlayer;
class CSWSAmbientSound;
class CGameEffect;
class CGameObject;
#include "resare.h"
struct CResStruct;
struct NWAreaExpansion_st;

// One walk-route point. x/y are read as a Vector by the plot code; the third word doubles as the
// DFS neighbour count, and the union keeps both spellings typed.
struct CSWSAreaPathPoint {
    union {
        Vector  m_point;
        struct {
            float         m_x;
            float         m_y;
            unsigned int  m_nNeighborCount;
        } m_node;
    };
    unsigned int m_nNeighborStart;
};

// Entry of one of the two 512-element DFS transition tables, filled and probed by the 2-way
// associative Put/Get pairs.
struct alignas(16) CSWSAreaGridTransEntry {
    int   m_nKeyX;
    int   m_nKeyY;
    int   m_nCostA;
    int   m_nCostB;
};
struct alignas(16) CSWSAreaPathPointTransEntry {
    float m_fKeyX;
    float m_fKeyY;
    float m_fCostA;
    float m_fCostB;
};

// One per-door bounding box, handed to BoxIntersect as {min, max}.
struct CSWSAreaDoorBox {
    Vector m_vMin;
    Vector m_vMax;
};

// One grid-DFS expansion candidate, filled by GridDFSGenerateSuccessors. m_nCostBound is a Manhattan
// distance.
struct CSWSAreaGridSuccessors {
    int m_nCostBound;
    int m_nX;
    int m_nY;
    int m_nZ;
};

// One path-point DFS expansion candidate, filled by PathPointDFSGenerateSuccessors.
struct CSWSPathPointSuccessors {
    unsigned int m_nPoint;     // index into CSWSArea::m_paPathPoints
    float m_fCost;             // step cost
    float m_fCostToDest;       // straight-line gap to the destination point, the insert key
};

// The .are header's reserved id block (LoadAreaHeader).
struct CSWSAreaHeaderIds {
    int32_t  nID;
    int32_t  nCreatorID;
    uint32_t nVersion;
};

class CSWSArea : public CSWArea, public CResHelper<CResARE,2012>, public CGameObject {
public:
    CSWSArea(const CResRef& cResRef, int nFlags, unsigned long long nAreaId);
    virtual ~CSWSArea();

    // Builds the area's CSWSRoom array from the parsed .lyt layout.
    static CSWSRoom* LoadRooms(CLYT* pLyt);

    // Identity downcasts; NWAreaAsSWSArea comes through the primary vtable's NWArea slot.
    virtual CSWSArea* AsSWSArea();
    virtual CSWSArea* NWAreaAsSWSArea();

    // Reads the .are, the room layout and the GIT, then registers the area in the module lookup.
    int LoadArea(int nFlags);

    int LoadAreaHeader(CResStruct* pStruct);
    int LoadRoomInfo(CResStruct* pStruct);
    int LoadGIT(int nFlags);
    int LoadPathPoints(CResStruct* pStruct);
    // Sizes and initializes the per-room audio banks from the "Rooms" list count.
    void InitializeRoomAudioParams(int nRoomCount);

    // -- weather --
    // Per-frame weather tick: ages m_nWeatherTimer and, once it expires, rolls against the rain,
    // snow and lightning chances in that order to flip the weather on or off. Returns 1 when the
    // heartbeat gate fired this tick.
    int  AIUpdate();
    // Broadcasts a weather change to every player in the area. nWeather == 255 derives the
    // transition from m_nSentWeather vs m_nCurrentWeather.
    void SetCurrentWeather(unsigned char nWeather, int bTurnOn);

    // The thirteen per-type GIT loaders. Some must stay defined in this file with bodies that never
    // touch the dead trailing argument, since the call-site setup for it is elided.
    int LoadCreatures(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates);
    int LoadItems(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates);
    int LoadDoors(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates);
    int LoadTriggers(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates);
    int LoadEncounters(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates);
    int LoadWaypoints(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates);
    int LoadSounds(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates);
    int LoadPlaceables(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates);
    int LoadStores(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates);
    int LoadAreaEffects(CResGFF* pGff, CResStruct* pStruct, int nFlags, int nUseTemplates);
    int LoadProperties(CResGFF* pGff, CResStruct* pStruct);
    int LoadMaps(CResGFF* pGff, CResStruct* pStruct);
    int LoadPlaceableCameras(CResGFF* pGff, CResStruct* pStruct);

    // Distance limits, defined with the rest of the area code and declared without initializers: the
    // callers load them from memory.
    static const int   MAX_X_TILES;
    static const int   MAX_Y_TILES;
    static const float MAX_OBJECT_VISIBLE_DISTANCE;
    static const float MAX_OBJECT_CHATTALK_DISTANCE;
    static const float MAX_OBJECT_CHATWHISPER_DISTANCE;
    static const float MAX_OBJECT_CONVERSATION_DISTANCE;
    static const float MAX_OBJECT_START_CONVERSATION_DISTANCE;
    static const float MAX_OBJECT_CHATTALKSILENT_DISTANCE;
    static const float MAX_OBJECT_CHATSHOUTSILENT_DISTANCE;

    char                   _pad169[0x16c-0x169];
    int                    m_nPlayersInArea;
    CExoArrayList<unsigned long long> m_mineObjects;
    unsigned int           m_nReserved1;
    unsigned int           m_lastUpdateDay;
    unsigned int           m_lastUpdateTod;
    CExoLinkedList<NWAreaExpansion_st> m_expansionList;
    CExoString             m_sComments;
    CExoLocString          m_areaName;
    CExoString             m_sDebugName;  // the area's tag, lower-cased
    void SetTag(const CExoString& sTag) { m_sDebugName = sTag.LowerCase(); }
    CSWSAreaHeaderIds*     m_pHeaderIds;
    CExoString             m_scriptOnHeartbeat;
    CExoString             m_scriptOnUserDefined;
    CExoString             m_scriptOnEnter;
    CExoString             m_scriptOnExit;
    int                    m_chanceFog;
    int                    m_modSpotCheck;
    int                    m_nListenModifier;
    CExoArrayList<unsigned long long> m_aObjectsByX;
    int                    m_nObjectsByXIndex;
    // The objects with a volume in the area (triggers, encounters, doors, areas of effect).
    CExoArrayList<unsigned long long> m_subAreas;
    void AddSubArea(CGameObject* pObject) { m_subAreas.Add(pObject->m_objectId); }
    void RemoveSubArea(CGameObject* pObject) { m_subAreas.Remove(pObject->m_objectId); }
    CPathfindInformation*  m_pActivePathfindInfo;
    // Appends an inter-tile exit record.
    int    RemoveInterTileExit(int nExitA1, int nExitA2, int nExitB1,
                              int nExitB2);
    CPathfindInformation*  m_pSoundPathInfo;
    CSWSAreaGridTransEntry*  m_paGridDFSTransTable;
    void*                    m_apReserved1[4];
    int                      m_nReserved2;
    // Grid-DFS expansion watchdog: the counter is incremented before every expansion and the flag
    // latches once it exceeds 63 (path-point search: 4095), aborting the search.
    int                      m_bGridDFSAborted;
    int                      m_nGridDFSExpansions;
    unsigned long long     m_oidEnteringObject;
    unsigned long long     m_onExitScriptCallerId;
    int                    m_nUserDefinedEventNumber;
    int                    m_bTransitionTargetMarked;
    CSWSScriptVarTable     m_scriptVars;
    CSWVarTable            m_varTable;
    CSWSAmbientSound*      m_pAmbientSound;
    unsigned char          m_nCurrentWeather;   // 0 none, 1 rain, 2 snow, 255 interior
    char                   _pad301[0x304-0x301];
    unsigned int           m_bWeatherOn;
    unsigned int           m_nWeatherTimer;     // milliseconds until the next roll
    unsigned int           m_nWeatherDay;       // last weather tick: world day
    unsigned int           m_nWeatherTod;       // last weather tick: time of day
    unsigned char          m_nWeatherOverride;  // 255 = roll normally, anything else pins it
    unsigned char          m_nSentWeather;
    unsigned char          m_nLightingScheme;
    unsigned char          m_nTileBitmapBytes;
    unsigned int           m_bAreaHeaderLoaded;
    unsigned short         m_nLoadScreenID;  // transition bitmap area id, passed to SetAreaTransitionBMP
    int                    m_nRoomCount;
    CSWSRoom*              m_paRooms;
    int                    m_miniGame;
    unsigned int                m_nPathPointCount;             // path-point count, m_paPathPoints entries
    CSWSAreaPathPoint*           m_paPathPoints;
    unsigned int              m_nPathPointLinkCount;          // path-point link count
    unsigned int*                m_paPathPointLinks;
    int                          m_bPathPointDFSAborted;            // PathPointDFSearch completion latch
    int                          m_nReserved3;
    CSWSAreaPathPointTransEntry* m_paPathPointDFSTransTable;
    int                          m_nPathPointDFSExpansions;
    int                          m_nPathPointStepCount;
    CExoString*                  m_paRoomNames;
    int*                         m_paRoomEnvAudio;
    float*                       m_paRoomAmbientScale;
    int                          m_nRoomAudioCount;      // shared element count
    int                          m_nPartSoundCount;          // running total of the "PartSounds" list counts
#if !KOTOR1  // K2-only: K1 has no per-room force rating or weather tables
    int*                   m_paForceRating;
    unsigned char*         m_paRoomDisableWeather;
    unsigned char*         m_paRoomTableIndex;      // room -> table-row byte map
#endif
    CExoArrayList<CExoString>  m_lstPartSoundRoomNames;
    CExoArrayList<int>         m_lstPartSoundLooping;
    CExoArrayList<CExoString>  m_lstPartSoundModelParts;
    CExoArrayList<CExoString>  m_lstPartSoundOmenEvents;
    CExoArrayList<CResRef>     m_lstPartSoundResRefs;  // LoadAreaHeader adds the "Sound" list
    unsigned int           m_nUnescapable;    // LoadProperties "Unescapable"
#if !KOTOR1  // K2-only: K1 has no "DisableTransit"
    int                    m_disableTransit;
#endif
    int                    m_nPlayerRestrictMode;
    unsigned int           m_maxStealthXP;
    unsigned int           m_currentStealthXP;
    int                    m_stealthXPDecrement;
    int                    m_bStealthXPEnabled;
    int                    m_bTransitionPending;  // nonzero: HideAreaTransition backs the panel out instead
    unsigned char          m_transitionPendingID;  // rolling 1..255 counter, never 0
    unsigned char          m_activeTransitionID;   // id of the transition in flight
#if !KOTOR1  // K2-only: K1 has no per-transition dirty arrays
    char                   _pad412[0x414-0x412];
    int                    m_dirtyARGB[3];
    int                    m_dirtySize[3];
    int                    m_dirtyFormula[3];   // toggles 0<->1 on the way in
    int                    m_dirtyFunction[3];
#endif
    void*                  m_pPathfindContext;
    CSWCArea*              m_pClientArea;

    // -- client/server attachment --
    // Re-files an object in the area's spatial object arrays after it has moved.
    int  UpdatePositionInObjectsArray(CGameObject* pObject);
    void AttachmentFromClientArea(CSWCArea* pClientArea);
    void DetachFromClientArea();
    // Resolves and attaches the matching client-side area on first use.
    CSWCArea* GetClientArea();

    // -- player presence / AI level --
    // Promotes the area's AI priority on the 0->1 edge.
    void IncrementPlayersInArea();
    // Demotes the area's AI priority on the 1->0 edge.
    void DecrementPlayersInArea();
    // Raises the AI level of every un-leveled creature and door corpse in the X index.
    void IncreaseAILevelPriority();
    void DecreaseAILevelPriority();
    void SetPlayerRestrictMode(int nMode);

    // -- fog --
    // Packs an RGB Vector (0..1 per channel) into the int fog color (B<<16|G<<8|R, times 255).
    void SetFogColor(const Vector& color);


    // The area's script/event pump. Deliberately non-virtual.
    void EventHandler(unsigned int nEventType, unsigned long long nCallerId, void* pData,
                      unsigned int nDay, unsigned int nTod);

    // -- stealth XP / transition state --
    void SetMaxStealthXP(unsigned int xp);
    void SetCurrentStealthXP(unsigned int xp);
    unsigned char GetNextTransitionPendingID();
    void SetTransitionPending(int bState, unsigned char nTransitionID);

    // -- per-transition dirty arrays --
#if !KOTOR1  // K2-only
    void SetDirtyARGB(int index, unsigned int argb);
    void SetDirtySize(int index, unsigned int size);
    void SetDirtyFormula(int index, unsigned int formula);  // toggles 0<->1
    void SetDirtyFunction(int index, unsigned int fn);
#endif

    // -- walkmesh / geometry queries --
    // The walkmesh and pathfind methods live in nwsareapathfind.cpp.
#if KOTOR1
    // K1 has no room-index hint; the three-argument spelling is a K1-only forwarder.
    CSWSRoom* GetRoom(const Vector& v, int* pa);
    CSWSRoom* GetRoom(const Vector& v, int* pa, int*) { return GetRoom(v, pa); }
#else
    CSWSRoom* GetRoom(const Vector& v, int* pa, int* pb);
#endif
    // Eight-angle retreat-point sweep used by the flee AI and the ActionMoveAwayFromLocation script
    // handler; returns the best candidate by value, defaulting to `pos`.
    Vector    ComputeAwayVector(const Vector& pos, const Vector& target,
                                float fRadius, CPathfindInformation* pPathInfo);
    // Best-corner sweep around vertex P2 of the P1->P2->P3 polyline, used to cut a corner when the
    // flee path allows it.
    int       ComputeBestCorner(float fAx, float fAy, float fBx, float fBy, float fCx, float fCy,
                                float fHitFraction, float fRadius, float* pa1, float* pa2,
                                float* pa3, float* pa4);
    CSWSRoom* GetRoom(int nRoom);
    int       GetRoomIndex(const Vector& v, int nRoom);
    unsigned int GetSurfaceMaterial(Vector pos);
    float        ComputeZFromLineSegment(const Vector& a, const Vector& b,
                                         const Vector& c);
    float        ComputePathDistance(int nPoints, float* paPoints);
    void ApplyEffect(CGameEffect* pEffect, const Vector& vPosition,
                     const Vector& vOrientation);
    // Nearest walk-route path point to v; writes its index to nOut and returns a found flag.
    int       ClosestPathPoint(const Vector& v, unsigned int& nOut);
    // Area-level line-of-sight walkmesh query: 1 = clear, 0 = blocked (pOut then gets the contact
    // point). The two id arguments take OBJECT_INVALID when there is no object to exclude.
    int       ClearLineOfSight(const Vector& from, const Vector& to, Vector* pOut,
                               unsigned long long* pIds, unsigned long long idA,
                               unsigned long long idB, int f, int g);
    // Walkmesh sound-route probe over m_pSoundPathInfo, whose from/to points are filled in first.
    // Returns 1 when a route exists.
    int       PlotSoundPath(CPathfindInformation* pPathInfo);
    // The grid DFS search engine. GridDFSearch is the recursive core and PlotGridPath the top-level
    // driver; GridDFSGenerateSuccessors fills a fresh candidate list for it to recurse into.
    int       GridDFSGenerateSuccessors(int nCost, int nCostBound, int nX, int nY, int nZ,
                                        CSWSAreaGridSuccessors** ppOut);
    int       GridDFSearch(int a, int b, int c, int nX, int nY, int nZ);
    // Path-point twin of the grid DFS engine. Its GenerateSuccessors parameters are dead in the
    // callee.
    int       PathPointDFSGenerateSuccessors(float fGap, int nPathIndex, unsigned int nCurrent,
                                             CSWSPathPointSuccessors** ppOut);
    // Turns the area's current raw path point list into the smoothed walk path. Path type 1
    // deep-copies the raw array; anything else runs the smoothing passes over it.
    int       SmoothPointsOnPath();
    void      SmoothSelection(int nCount, float* pFloats, int* pOutCount,
                              float** ppOutFloats);
    void      SmoothCornerOptimize(int nCount, float* pFloats, int* pOutCount,
                                   float** ppOutFloats);
    void      SmoothSelectNodes(int nCount, float* pFloats, int* pOutCount,
                                float** ppOutFloats, int a, int b);
    // Restartable smoothing driver: dispatches on CPathfindInformation's m_nSmoothStep across the
    // STEP_2..STEP_4 ladder, parking the partial splines between frames. On the last step the
    // completed trim goes to the caller's pOutCount/ppOutFloats.
    int       SmoothSelectionRestartable(int nCount, float* pFloats, int* pOutCount,
                                         float** ppOutFloats);
    // Restartable SmoothSelectNodes: the walk state lives in CPathfindInformation and each iteration
    // is bounded by a SmoothTime budget. Returns 1 (resume next frame) until done.
    int       SmoothSelectNodesRestartable(int nCount, float* pFloats, int* pOutCount,
                                           float** ppOutFloats, int bSubdivide, int bReselect,
                                           float fSmoothThreshold);
    // Full point-path plot state machine: drives the start-points, pathfind, grid-start, grid-end and
    // smooth states through pPathInfo->m_nPathState over multiple frames. Returns an action code.
    int       PlotPathPointPath(CPathfindInformation* pPathInfo,
                                uint64_ll_t nTimeOut);
    // Counts the area's transition triggers and linked doors over the m_aObjectsByX index, records
    // the survivors into pOut when it is non-null, and marks each target area as visited.
    int       CountAreaTransitionTriggers(unsigned long long* pOut, int nMax,
                                          unsigned long long* pExclude);
    // Dead stubs in this build: PlotPath returns 0 unconditionally, TestLineWalkable returns 1.
    int       PlotPath(CPathfindInformation* pPathInfo,
                       uint64_ll_t nTimeOut);
    int       TestLineWalkable(float fX1, float fY1, float fZ1, float fX2,
                               float fY2);
    // Path-point depth-first search, the core of the pathfind state.
    int       PathPointDFSearch(int nPathIndex, float fCost, float fCostBound,
                                unsigned int nCurrent, unsigned int nTarget);
    // Grid-based path search driver, the core of the grid-start and grid-end states.
    int       PlotGridPath(CPathfindInformation* pPathInfo,
                           uint64_ll_t nTimeOut);
    // One path-smoothing slice, the core of the smooth state.
    int       SmoothPointsOnPathRestartable();
    // 2D hash of a tile into the 256-entry DFS transition table (the same x*17+y*23+6000,
    // modulo 255 formula for both tables).
    int       GridDFSTransTableHash(int nTileX, int nTileY);
    int       PathPointDFSTransTableHash(int nTileX, int nTileY);
    // Lazily allocates and blanks the two 512-element DFS transition tables.
    void      GridDFSTransTableInitialize();
    void      PathPointDFSTransTableInitialize();
    // 2-way associative store and lookup over the tables: slot hash(x, y), then the second slot.
    // Put replaces the primary entry when the key already lives there or the stored costs are
    // strictly worse; Get returns 1 and writes the costs on a key match in either slot.
    void      GridDFSTransTablePut(int nTileX, int nTileY, int nCostA, int nCostB);
    int       GridDFSTransTableGet(int nTileX, int nTileY, int* pOutCostA,
                                   int* pOutCostB);
    void      PathPointDFSTransTablePut(float fX, float fY, float fCostA, float fCostB);
    int       PathPointDFSTransTableGet(float fX, float fY, float* pOutCostA,
                                        float* pOutCostB);
    // Per-room force rating at a world position, fed to CSWSpell::GetAdjustedForcePoints.
    int       GetRoomForceRating(const Vector& v);
    // Per-room weather-disable query.
    char      GetRoomDisableWeather(const Vector& v);
    int       GetNumMinesSetByPlayerParty();
    int       GetPlayerCanSetMines();
#if KOTOR1
    // K1 has no room-index hint; the four-argument K2 spelling is a K1-only forwarder.
    float     ComputeHeight(const Vector& v, int n, CSWSRoom** ppRoom);
    float     ComputeHeight(const Vector& v, int n, CSWSRoom** ppRoom, int*) { return ComputeHeight(v, n, ppRoom); }
#else
    float     ComputeHeight(const Vector& v, int n, CSWSRoom** ppRoom, int* pi);
#endif
    int       TestDirectLine(const Vector& from, const Vector& to, const float& hitFraction,
                             float f, int g, CWalkHitInfo* h);
    // Detailed walkmesh and creature line tests, both taking a segment plus a CWalkHitInfo
    // out-param; the two z bounds bracket the segment.
    int       NoNonWalkPolysDetailed(const Vector& from, const Vector& to, float zLow, float zHigh,
                                     float radius, unsigned long long nIgnoreId,
                                     unsigned long long& nHitId, int a,
                                     CExoArrayList<CGameObject*>* pList, int b,
                                     CWalkHitInfo* pHit, int c);
    int       NoCreaturesOnLine(const Vector& from, const Vector& to, CPathfindInformation* pPathInfo,
                                int a, int b, unsigned long long* pnHitId, int c,
                                CWalkHitInfo* pHit);
    // Creature-circle overlap query: nonzero means the target list still overlaps, zero that the
    // line is blocked and the hit recorded.
    int       EvaluateOverlappingTargets(CPathfindInformation* pPathInfo, const Vector& pos,
                                         unsigned long long nObjectId, const Vector& from,
                                         float fCombined, int a, int c,
                                         unsigned long long* pnHitId);
    // Fills pHit for a creature-blocked walk segment (pos plus combined radius against from->to).
    // Returns int, though NoCreaturesOnLine discards it.
    int       SetHitInfoForCreature(const Vector& pos, float fCombined, const Vector& from,
                                    const Vector& to, CWalkHitInfo* pHit);
    // Object id of the door whose bounding geometry the segment crosses, or OBJECT_INVALID.
    unsigned long long GetDoorCrossed(const Vector& from, const Vector& to);
    // Takes the Vector by value; no caller takes its address.
    int       PositionWalkable(Vector pos);
    int       ShrinkLineByCreatures(const Vector& from, const Vector& to, Vector& out);
    // Appends the ids of every object whose region the segment crosses; the return value is a
    // nonzero "found something" flag.
    int       IntersectingLineSegment(const Vector& from, const Vector& to,
                                      CExoArrayList<unsigned long long>* pOut);
    // Appends the ids of every subarea containing pos to pOut; the return value is a nonzero
    // count/flag.
    int       InSubAreas(const Vector& pos, CExoArrayList<unsigned long long>* pOut);
    // Returns int, not unsigned long long, and the result is compared against 1. The forwarder
    // CServerExoAppInternal::TestSafeLocation widens it to its own return type.
    int TestSafeLocationPoint(const Vector& pos, CPathfindInformation* pPathInfo);
    // Walkmesh segment probe (from, to, z band f1/f2, personal space, ignored object id, hit
    // out-param, flag) used by the safe-location cluster.
    int NoNonWalkPolys(const Vector& a, const Vector& b, float f1, float f2, float f3,
                       unsigned long long nIgnoreId, CWalkHitInfo* pHit, int nFlag);
    // Path-point graph self-test driven by the `testpathpoints` console command through
    // CSWSModule::TestPathPoint. Returns 1 when the graph is bad, 0 when every link checks out;
    // the trailing int is unused.
    int TestPathPoint(float fHitFraction, int nFlag);
    // Linear scan of the "already hit, ignore these" object list for one id. Returns 1 on a hit, 0
    // when the list is empty or null. Overrides CSWArea's virtual of the same name.
    int virtual GetIsIDInExcludeList(unsigned long long nObjectId,
                                     CExoArrayList<CGameObject*>* pExcludeList);
    // Static-object (placeable/door) twin of NoNonWalkPolys: dispatches on the object's type byte to
    // the placeable or door walkmesh probe.
    int NoNoneWalkPolysInStaticObject(unsigned long long nObjectId, const Vector& a,
                                      const Vector& b, float f1, float f2, float f3,
                                      unsigned long long nIgnoreId, int nFlag,
                                      CWalkHitInfo* pHit);
    int       ComputeSafeLocation(const Vector& pos, float radius, CPathfindInformation* pPathInfo,
                                  int flag, Vector* out);
    // Same query, sweeping along `dir` from `pos` instead of radially.
    int       ComputeSafeLocationInDirection(const Vector& pos, const Vector& dir, float radius,
                                             CPathfindInformation* pPathInfo, int flag,
                                             Vector* out);
    // Walkmesh probe along the segment a->b at the given height band. Returns an int code, not a
    // bool: only an exact 1 is accepted.
    int       NoNonWalkPolysInDoors(const Vector& a, const Vector& b, float z, float height,
                                    float radius, unsigned long long oidIgnore,
                                    unsigned long long& oidHit,
                                    CExoArrayList<CGameObject*>* pHits, int flag1,
                                    int flag2);
    // See-through-door retry: when a probe hits a door whose doortypes row has BlockSight = 0, the
    // door joins the hit list and the probe re-runs with it excluded.
    virtual int HandleTransparentDoors(const Vector& from, const Vector& to, float zLow, float zHigh,
                                     float radius, unsigned long long oidIgnore,
                                     unsigned long long& oidHit, int nFlag,
                                     CExoArrayList<CGameObject*>* pHits, int nUnused);
    // Returns 1 on success and writes the location out, or (-1,-1,-1) when no candidate exists.
    int       ComputeNonVisibleLocation(const Vector& pos, CPathfindInformation* pPathInfo,
                                        int flag, Vector* out, float radius);
    // Visible-player count around a world position.
    int       CountVisibleToPlayers(const Vector& pos, int nRadius);
    void      BudgeCreatures(const Vector& a, const Vector& b, const Vector& c,
                             unsigned long long objectId, int isClosing);

    // -- per-area object list / spatial index --
    int  RemoveObjectFromArea(unsigned long long objectId);
    // Registers objectId in the X-sorted object index. Returns 1 on success, 0 when the object cannot
    // be resolved.
    int    AddObjectToArea(unsigned long long objectId, int n);
    // Binary-search estimate for the X-sorted object index, declared always_inline.
    __attribute__((always_inline)) int  GetFirstObjectIndiceByX(int* pOutIndex, float x);
    // RemoveStaticBoundingBox/AddStaticBoundingBox are CSWArea members, not CSWSArea ones: they
    // are declared in CSWArea.h and reached straight off GetArea()'s CSWArea* result.
    // Returns int, not void: CSWGuiDebugMenu::OpenAllDoors branches on whether the walk had any
    // elements at all.
    int  GetFirstObjectInArea(unsigned long long& objId);
    int  GetNextObjectInArea(unsigned long long& objId);

    // -- misc --
    // Serializes the area (rooms, static geometry, the visible object set) into the message being
    // built, tailored to one player. Returns int, not void: a send that does not get exactly 1 is
    // abandoned.
    int  PackAreaIntoMessage(int nX, int nY, int nZ, CSWSPlayer* pPlayer);

    // -- save serialization --
    // Builds a fresh CResGFF ("GIT "/"V2.0"), sorts every object of m_aObjectsByX into ten object-id
    // lists, writes the script variable tables and the transition and weather bytes, then fans out
    // to the per-section savers.
    void SaveGIT(CERFFile* pERF, const CExoString& sFileName, CExoArrayList<unsigned long long>& aSectionLists);
    void SaveArea(CERFFile* pERF, const CExoString& sFileName);
    // The ten per-type list savers iterate the object-id section list SaveGIT built. SaveMaps and
    // SavePlaceableCameras do not read `this`.
    void SaveCreatures(CResGFF* pGFF, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList);
    void SaveItems(CResGFF* pGFF, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList);
    void SaveDoors(CResGFF* pGFF, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList);
    void SaveTriggers(CResGFF* pGFF, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList);
    void SaveEncounters(CResGFF* pGFF, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList);
    void SaveWaypoints(CResGFF* pGFF, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList);
    void SaveSounds(CResGFF* pGFF, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList);
    void SavePlaceables(CResGFF* pGFF, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList);
    void SaveStores(CResGFF* pGFF, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList);
    void SaveAreaEffects(CResGFF* pGFF, CResStruct* pStruct, CExoArrayList<unsigned long long>& aList);
    void SaveProperties(CResGFF* pGFF, CResStruct* pStruct);
    void SaveMaps(CResGFF* pGFF, CResStruct* pStruct);
    void SavePlaceableCameras(CResGFF* pGFF, CResStruct* pStruct);
    void EnforceNoPCHeadCloning(int headId);

    // -- object index teardown / mine queries --
    void UnloadArea();
    void ClearObjects();
    // Banks accumulated stealth XP onto a creature and the party, guarded, then resets.
    void AwardStealthXP(unsigned long long objectId);
    // Spawns a one-shot area visual effect at vPosition (PlayVisualAreaEffect).
    void PlayVisualEffect(unsigned short nEffectId, const Vector& vPosition);
};
#if KOTOR1  // K1 CSWSArea field offsets
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSArea, m_pAmbientSound) == 0x2d8, "K1 CSWSArea layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSArea, m_nTileBitmapBytes) == 0x2f7, "K1 CSWSArea layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSArea, m_nRoomCount) == 0x300, "K1 CSWSArea layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSArea, m_paPathPointDFSTransTable) == 0x338, "K1 CSWSArea layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSArea, m_paRoomNames) == 0x348, "K1 CSWSArea layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSArea, m_paRoomAmbientScale) == 0x358, "K1 CSWSArea layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSArea, m_lstPartSoundRoomNames) == 0x368, "K1 CSWSArea layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSArea, m_lstPartSoundResRefs) == 0x3a8, "K1 CSWSArea layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSArea, m_nUnescapable) == 0x3b8, "K1 CSWSArea layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSArea, m_nPlayerRestrictMode) == 0x3bc, "K1 CSWSArea layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSArea, m_bStealthXPEnabled) == 0x3cc, "K1 CSWSArea layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSArea, m_bTransitionPending) == 0x3d0, "K1 CSWSArea layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSArea, m_activeTransitionID) == 0x3d5, "K1 CSWSArea layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSArea, m_pPathfindContext) == 0x3d8, "K1 CSWSArea layout");
K2_LAYOUT_ASSERT(__builtin_offsetof(CSWSArea, m_pClientArea) == 0x3e0, "K1 CSWSArea layout");
K2_LAYOUT_ASSERT(sizeof(CSWSArea) == 0x3e8, "K1 CSWSArea size");
#endif
